# Kiến trúc Giai đoạn 1 — Mở khả năng kiểm thử (T1.1–T1.4)

> Nguồn: bản tư vấn kiến trúc G1 (2026-09-09), cập nhật lần 2 (2026-09-09) sau khi
> chốt 2 quyết định: **(1) decouple hazard_core khỏi sensor_reading_t + bỏ
> `sensor_model_classify`**, **(2) `tools/guard/arch_guard.py` sinh kèm step 1**.
> Roadmap chi tiết (DoD/lệnh verify) nằm ở `docs/roadmaps/next-branch.roadmap.json`.
> File note về config cứng/khó bảo trì: `docs/HARDCODED_CONFIG_NOTES.md`.

## Vấn đề cốt lõi

1. **UI logic gắn chặt LVGL**: `evaluate_hazard()` (`ui_dashboard.c:134`) vừa quyết định
   DANGER/CAUTION/heuristic crossing vừa ghi trực tiếp widget → không host-test được,
   không tái dùng cho simulator.
2. **`sensor_model.c` phụ thuộc FreeRTOS** kể cả logic thuần (classify) → rào cản nhỏ
   cho host test/sim.
3. **3 công cụ (T1.1/T1.2/T1.4) dễ duplicate**: cùng "kịch bản" (approach/crossing/slam/
   normal), mỗi nơi tự sinh chuỗi khoảng cách sẽ lệch nhau.
4. **`test_mqtt_coreiot.py` đã đúng schema V2** (`tools/test_mqtt_coreiot.py:64`) — chỉ
   thiếu tầng scenario.

## Kiến trúc: 3 lớp + 1 khuôn mẫu chung (+ 1 guard)

### Lớp 1 — Pure decision logic (MỚI) `components/hazard_core/`
C thuần, **zero OS/LVGL dep** → host-compile tức thì. Chỉ include `thresholds.h` +
`espnow_protocol.h` (R2/R3). **Input là primitive array** (`uint16_t*`/`bool*`), KHÔNG
phụ thuộc `sensor_reading_t` của sensor_model → đổi struct sensor_model không lây sang
core/tests.

```c
/* hazard_core.h */
#include "espnow_protocol.h"   /* espnow_slot_t */
#include "thresholds.h"        /* sensor_zone_t — đã định nghĩa tại thresholds.h:37 */

#define CROSSING_DELTA_CM 40
#define CROSSING_FRONT_THRESHOLD_CM 150   /* chuyển từ ui_dashboard_theme.h (1 nguồn) */

typedef struct {
    bool active;
    espnow_slot_t sensor;                  /* slot "fast-change" (LEFT_FRONT..RIGHT_REAR) */
} hazard_crossing_result_t;

sensor_zone_t hazard_classify(uint16_t distance_cm);
sensor_zone_t hazard_worst_zone(const uint16_t *dist_cm, const bool *is_stale, size_t n);
hazard_crossing_result_t hazard_eval_crossing(const uint16_t *cur_cm, const uint16_t *prev_cm, size_t n);
```

- `hazard_classify` — nhận body từ `sensor_model_classify` (giữ nguyên semantics:
  `x < DANGER` → DANGER; `x <= CAUTION` → CAUTION; else SAFE).
- `hazard_worst_zone` — nhận logic "worst + skip stale" (`ui_dashboard.c:139-150`).
- `hazard_eval_crossing` — nhận heuristic crossing (`ui_dashboard.c:170-181`):
  `front_close = cur[ESPNOW_SLOT_FRONT] < 150`; quét side slots, `|cur−prev| ≥ 40`.
  KHÔNG xử lý stale (giữ hành vi cũ).

**Bỏ `sensor_model_classify`** (quyết định 2026-09-09): 3 call-site `ui_dashboard.c:78/148/210`
chuyển sang `hazard_classify`. Sensor_model chỉ là state container; không giữ wrapper để
tránh 2 symbol cùng body (API gọn, ownership rõ).

Seam cho **T2.3**: phần nhận firmware ĐÃ sẵn sàng — `main.c:84-87` parse
`crossing_hazard` → `ui_dashboard_set_hazard_warning()` → `s_forced_crossing_warning`
→ OR trong `evaluate_hazard()` (`ui_dashboard.c:183`) → `hazard_eval_crossing(...).active`.
Phần "dead-path" THỰC SỰ là **rule-chain**: snapshot
`cloud/coreiot/rule_chain/supersonic_rule_chain.json` hiện KHÔNG xuất `crossing_hazard`
(grep = 0). T2.3 = thêm node JS xuất field này trên console (snapshot mới) — không cần
sửa phía nhận firmware, chỉ đưa nhánh heuristic vào hazard_core (step 1).

### Lớp 2 — State container `sensor_model` (giảm mặt API)
Mutex-guarded, chỉ thao tác qua API; xoá `sensor_model_classify` khỏi `.c` + `.h`.

### Lớp 3 — UI render `ui_dashboard*.c`
Thuần LVGL widget, áp kết quả hazard_core, không còn quyết định logic.

### Khuôn mẫu chung `tools/scenarios.py`
Một spec `t → distances[6] (cm)`; các tên `approach/crossing/slam/normal` định nghĩa
**một lần**, 3 nơi tiêu thụ:
- T1.1: `test_mqtt_coreiot.py --scenario X` duyệt chuỗi theo `--interval`
- T1.4: `record_telemetry.py` (subscribe → JSONL) + `replay_telemetry.py`
  (JSONL → publish đúng nhịp, `--dry-run` validate schema)
- T1.2: `host_sim --scenario X` / `host_sim --replay tb.jsonl`

**JSONL schema = payload V2 telemetry** (`d1..d6`, `nearest_cm`, `has_nearest`,
`timestamp`, `seq`) pin ngay trong `scenarios.py` để recorder/replay/sim khớp nhau
không cần chờ T1.4 xong.

### Guard `tools/guard/arch_guard.py` (sinh kèm step 1)
Ép buộc bằng máy các quy tắc B1–B7 (mục dưới), chạy trong `/verify` local + CI security
job. Thêm test vào `tools/guard/test_guard.py`.

## Bộ quy tắc giữ code sạch/dễ sửa (B1–B7)

| # | Quy tắc | Cơ chế ép buộc |
|---|---|---|
| B1 | **R-dep** — phụ thuộc một chiều `shared → hazard_core → sensor_model → ui_dashboard → main`; cấm vòng ngược | `arch_guard.py`: hazard_core KHÔNG include sensor_model.h; ui_dashboard không include header coreiot_client/espnow_receiver nội bộ |
| B2 | **R-core-stateless** — hazard_core thuần: không static mutable, không FreeRTOS/LVGL | `arch_guard.py`: `static .* = ` trong hazard_core.c = 0; review arch-guard |
| B3 | **R-owner** — mỗi state/type một chủ; không `extern` vượt ranh giới component | pattern `ui_dashboard_private.h` nội bộ; hazard_core không private header |
| B4 | **R-small-api** — public API nhỏ, prefix `hazard_*`, signature ổn định | `hazard_core.h` ≤ 4 hàm (chốt: 3) |
| B5 | **R-single-truth** — ngưỡng → `thresholds.h` (R3); ngữ nghĩa phân loại → hazard_core là gốc chuẩn; python/rule-chain đối chiếu | `arch_guard.py` (python mirror) + `check_rulechain_thresholds.py` |
| B6 | **R-test-adjacent** (mở rộng R10) — đổi logic phải đổi test kèm theo | test colocated `host_sim/tests/`, chạy trong CI |
| B7 | **R7 + AGENTS log** — ≤400 dòng/file; mỗi task có log | check_size.py + docs/logs/ |

Xem chi tiết + bảng "blast radius" (sửa 1 chỗ, đụng 1 chỗ) ở `docs/HARDCODED_CONFIG_NOTES.md`.

## Host simulator (`firmware/waveshare-screen/host_sim/`)
- CMake standalone: `FetchContent` LVGL **v9.1.0** (khớp `idf_component.yml`) +
  SDL2; compile các file **thật**: `ui_dashboard.c/layout.c/system.c`,
  `sensor_model.c`, `hazard_core.c`. Ở step 1, `host_sim/CMakeLists.txt` chỉ mang target
  `hazard_core_tests` (skeleton — LVGL/SDL bổ sung ở step 3).
- `stub/` mỏng (~10 file): `freertos/{FreeRTOS.h, semphr.h}` +
  `{esp_log, esp_wifi, esp_system, esp_timer, esp_chip_info, esp_flash,
  esp_app_desc, coreiot_client}.h` trả dữ liệu giả (SYSTEM page không thấy token
  thật — R1).
- `lv_conf.h`: `LV_USE_SDL=1`, 800x480.
- CLI: `--exit-after N` (CI headless `xvfb-run`), `--scenario <name>`,
  `--replay <jsonl>`.

## Unit test T1.3
Target `hazard_core_tests` trong `host_sim/` (gcc, **mini assert-runner** — không Unity,
tránh FetchContent thừa ở bước test-core; C thuần + exit code): bounds 19/30/31,
99/100/101, skip-stale, crossing delta=39/40/41, FRONT 149/150.

## CI
CI firmware job: build host_sim `hazard_core_tests` (gcc, sau khi hazard_core tồn tại) +
chạy binary; security job: `arch_guard.py` + `pytest tools/guard`. Step 3 thêm apt
`libsdl2-dev`, build đủ UI sim, `xvfb-run umt_dash_sim --exit-after 3`. Bổ sung
CHECKLIST/PROGRESS.

## Thứ tự thực thi đề xuất (đã đảo 2026-09-09 — xem next-branch.roadmap.json)
1. `hazard_core` (decouple primitives, bỏ wrapper) + `arch_guard.py` + test boundary
2. `tools/scenarios.py` + `--scenario` trong `test_mqtt_coreiot.py`
3. `host_sim` (LVGL SDL, `--scenario`, `--replay`)
4. `record_telemetry.py` + `replay_telemetry.py`
5. CI gating + CHECKLIST/PROGRESS
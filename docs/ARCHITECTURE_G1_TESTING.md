# Kiến trúc Giai đoạn 1 — Mở khả năng kiểm thử (T1.1–T1.4)

> Nguồn: bản tư vấn kiến trúc G1 (2026-09-09). Sửa theo đúng file này trước khi code;
> roadmap chi tiết (DoD/lệnh verify) sẽ nằm ở `docs/roadmaps/g1-testability.roadmap.json`
> khi bắt đầu thực thi qua skill `dev-orchestrator`.

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

## Kiến trúc: 3 lớp + 1 khuôn mẫu chung

### Lớp 1 — Pure decision logic (MỚI) `components/hazard_core/`
C thuần, **zero OS/LVGL dep** → host-compile tức thì. Chỉ include `thresholds.h` +
`espnow_protocol.h` (R2/R3).
- `hazard_classify(uint16_t)` — nhận body từ `sensor_model_classify`; `sensor_model_classify`
  thành wrapper mỏng (API công khai không vỡ, R2 không symbol trùng).
- `hazard_worst_zone(readings[], n)` — nhận logic "worst + skip stale"
  (`ui_dashboard.c:139-150`).
- `hazard_eval_crossing(cur[], prev[], n)` → `{active, sensor_id}` — nhận heuristic crossing
  (`ui_dashboard.c:170-181`); hằng `CROSSING_DELTA_CM`/`CROSSING_FRONT_THRESHOLD_CM`
  chuyển từ `ui_dashboard_theme.h` vào hazard_core (1 nguồn).

Seam cho **T2.3**: `s_forced_crossing_warning` (từ rule-chain `crossing_hazard`, đang
dead-path ở `main.c:84`) OR yếu tố heuristic → tái sử dụng hazard_core.

### Lớp 2 — State container `sensor_model` (giữ nguyên)
Mutex-guarded, chỉ thao tác qua API; không đổi hành vi.

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

## Host simulator (`firmware/waveshare-screen/host_sim/`)
- CMake standalone: `FetchContent` LVGL **v9.1.0** (khớp `idf_component.yml`) +
  SDL2; compile các file **thật**: `ui_dashboard.c/layout.c/system.c`,
  `sensor_model.c`, `hazard_core.c`.
- `stub/` mỏng (~10 file): `freertos/{FreeRTOS.h, semphr.h}` +
  `{esp_log, esp_wifi, esp_system, esp_timer, esp_chip_info, esp_flash,
  esp_app_desc, coreiot_client}.h` trả dữ liệu giả (SYSTEM page không thấy token
  thật — R1).
- `lv_conf.h`: `LV_USE_SDL=1`, 800x480.
- CLI: `--exit-after N` (CI headless `xvfb-run`), `--scenario <name>`,
  `--replay <jsonl>`.

## Unit test T1.3
CMake test target trong `host_sim/` (gcc, không đụng `pio native` để tránh xung
framework espidf): bounds 19/30/31, 99/100/101, skip-stale, crossing delta=40 / FRONT<150.

## CI
Thêm apt `libsdl2-dev`, build host_sim, `xvfb-run umt_dash_sim --exit-after 3`, chạy
test hazard_core. Bổ sung CHECKLIST/PROGRESS.

## Thứ tự thực thi đề xuất (xem phân tích ở câu trả lời giai đoạn)
1. `hazard_core` + refactor wrapper + test boundary
2. `tools/scenarios.py` + `--scenario` trong `test_mqtt_coreiot.py`
3. `host_sim` (LVGL SDL, `--scenario`, `--replay`)
4. `record_telemetry.py` + `replay_telemetry.py`
5. CI gating + CHECKLIST/PROGRESS
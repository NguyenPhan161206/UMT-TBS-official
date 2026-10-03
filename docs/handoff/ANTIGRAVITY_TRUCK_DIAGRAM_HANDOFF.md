# HANDOFF cho Antigravity — Sơ đồ xe tải EX8 (T3.2) + marker vật thể (T3.1)

> Tài liệu tự đủ: agent chưa từng thấy cuộc trò chuyện vẫn làm được. Làm **tuần tự từng step**,
> xong step nào verify step đó rồi mới sang step kế. Roadmap: `docs/roadmaps/truck-diagram-markers.roadmap.json`,
> ledger: `docs/roadmaps/truck-diagram-markers.state.md` (cập nhật status sau mỗi step).

## 0. Bối cảnh
Hệ thống cảnh báo điểm mù xe tải: `firmware/sensor-node` (6 cảm biến siêu âm, ESP-NOW) → `firmware/waveshare-screen`
(màn 7" 800x480, ESP-IDF + LVGL v9). Màn hình có trang COLLISION: canvas giữa 440x440 px vẽ xe + 6 cung quét.

**Hiện trạng (sai so với yêu cầu):**
- `components/ui_dashboard/ui_dashboard_layout.c` `build_center_canvas()` vẽ *xe con* = 1 hình chữ nhật 160x260 + 3 label
  ("FRONT HOOD", "CABIN", "REAR TRUNK"), và bảng `k_layout[]` **toạ độ cung gắn cứng** (`{220, 60, 270}`...).
- Chưa có icon/marker tại điểm phát hiện; mỗi cảm biến chỉ hiện text + cung đổi màu theo zone.

**Mục tiêu:**
- **T3.2** vẽ sơ đồ xe tải EX8; kích thước và vị trí cảm biến lấy từ *hồ sơ xe* (struct), tính tỷ lệ động,
  **không gắn cứng toạ độ trong code UI**.
- **T3.1** hiện marker tại vị trí vật thể phát hiện (dọc trục búp cảm biến, cùng tỷ lệ với thân xe).
- (Nền cho T4.1a: cấu trúc hồ sơ xe + hồ sơ EX8 nhập cứng.)

**Số đo EX8 thật chưa có** → dùng PLACEHOLDER ở §3, đặt ở **một chỗ duy nhất** (`vehicle_profile.c`) để thay sau.
Không được suy diễn thêm số đo ở nơi nào khác.

## 1. Luật bắt buộc của repo (đọc `AGENTS.md`, `.opencode/docs/CONSTITUTION.md` nếu cần)
- **Quy ước slot (QUAN TRỌNG):** wire slot `espnow_slot_t` (`firmware/shared/espnow_protocol.h`):
  FRONT=0, REAR=1, LEFT_FRONT=2, LEFT_REAR=3, RIGHT_FRONT=4, RIGHT_REAR=5. Mọi mảng theo cảm biến phía màn hình
  lập chỉ mục theo thứ tự này (dùng designated initializer `[ESPNOW_SLOT_FRONT] = ...`). **Không** nhầm với thứ tự
  chân vật lý `SENSOR_PINS[]` của sensor-node (thứ tự khác, không đụng).
- **R2/R3:** không định nghĩa lại struct/ngưỡng dùng chung. Ngưỡng zone: `SENSOR_CAUTION_CM=100`, `SENSOR_DANGER_CM=30`,
  `SENSOR_BEAM_FOV_DEG=75` (`firmware/shared/thresholds.h`). Dùng `hazard_classify()` (`components/hazard_core`) để ra zone.
- **R7:** mỗi file ≤ 400 dòng. `ui_dashboard.c` đang 393 dòng, `ui_dashboard_layout.c` 341 dòng → logic mới đặt ở file mới.
- **Safety:** dữ liệu `DISCONNECTED/STALE` không được hiển thị như vật thể thật. `ui_dashboard_clear_sensor()` là đường
  duy nhất xoá slot (main.c gọi khi stale/mất link) → marker phải ẩn trong hàm đó.
- LVGL **v9** API (`lv_obj_remove_flag`, `lv_obj_add_flag`, …). Mọi hàm `ui_dashboard_*` được gọi dưới LVGL lock; không tự thêm lock.
- Comment tiếng Việt như code xung quanh; giữ style SPDX header (`SPDX-FileCopyrightText: 2026 Vehicle Warning System`, MIT).
- Cấm `git add -A`/`git add .`. Chỉ sửa file trong "Target files" của step. Không đụng `sdkconfig*`, `platformio.ini`, credentials.
- Sau cùng bắt buộc có log `docs/logs/WAVESHARE_SCREEN_TRUCK_DIAGRAM_LOG.md` (step 8).

## 2. Cách build/verify (chạy từ thư mục gốc repo `E:\Truck_Blind_Sight`)
Host sim (không cần board). Trên Windows cần MinGW/Ninja + mạng (FetchContent kéo SDL2 prebuilt MinGW + LVGL v9.1.0
hoặc dùng `firmware/waveshare-screen/managed_components/lvgl__lvgl` nếu có sẵn):
```bash
cmake -S firmware/waveshare-screen/host_sim -B build/host_sim      # thêm -G "MinGW Makefiles" hoặc -G Ninja nếu cần
cmake --build build/host_sim
ctest --test-dir build/host_sim --output-on-failure                 # hazard_core_tests, vehicle_layout_tests, umt_dash_sim_render
SDL_VIDEODRIVER=dummy build/host_sim/umt_dash_sim --scenario approach --exit-after 3 --interval 200
```
(`build/` đã gitignore.) Firmware thật: `cd firmware/waveshare-screen && pio run -e yolo_uno`.
Guard: `python3 tools/guard/arch_guard.py`, `python3 tools/guard/scan_secrets.py`, `python3 -m pytest tools/guard/test_guard.py -q`.
Nếu máy không build được host sim: ghi rõ vào báo cáo, **không** tuyên bố PASS giả; ít nhất chạy được `pio run`.

## 3. Contract (chữ ký CHÍNH XÁC — các step sau phụ thuộc)

Component mới: `firmware/waveshare-screen/components/vehicle_profile/` (C thuần, không include lvgl).
`CMakeLists.txt` theo mẫu `components/hazard_core/CMakeLists.txt`:
`idf_component_register(SRCS "vehicle_profile.c" "vehicle_layout.c" INCLUDE_DIRS "include" "../../../shared" REQUIRES)`
(step 1 chỉ có `vehicle_profile.c`, step 2 thêm `vehicle_layout.c`). Cần link `m` trong host sim (libm cho sinf/cosf).

### `include/vehicle_profile.h`
```c
#include <stdbool.h>
#include <stdint.h>
#include "espnow_protocol.h"          /* ESPNOW_SENSOR_SLOT_COUNT, espnow_slot_t */

/* Hệ toạ độ xe: gốc = tâm hình chữ nhật thân xe nhìn từ trên xuống.
 * x_mm: dương = sang PHẢI xe; y_mm: dương = về phía ĐẦU xe.
 * angle_deg: hướng búp theo quy ước LVGL (0=phải, 90=xuống/đuôi, 180=trái, 270=lên/đầu) */
typedef struct {
    int16_t x_mm;
    int16_t y_mm;
    int16_t angle_deg;
} vehicle_sensor_pose_t;

typedef struct {
    const char *name;
    uint16_t length_mm;               /* đầu → đuôi */
    uint16_t width_mm;
    uint16_t cab_length_mm;           /* chiều dài cabin tính từ đầu xe */
    vehicle_sensor_pose_t sensors[ESPNOW_SENSOR_SLOT_COUNT];   /* index = espnow_slot_t */
} vehicle_profile_t;

const vehicle_profile_t *vehicle_profile_active(void);          /* hiện tại: EX8, hằng static const */
bool vehicle_profile_validate(const vehicle_profile_t *p);      /* false nếu NULL/kích thước 0/cab>=length/angle ngoài 0..359/sensor ngoài bbox xe quá 100mm */
```
`vehicle_profile.c` có `_Static_assert(sizeof(((vehicle_profile_t*)0)->sensors)/sizeof(vehicle_sensor_pose_t) == ESPNOW_SENSOR_SLOT_COUNT, ...)`.

**Hồ sơ EX8 PLACEHOLDER** (ghi comment `/* PLACEHOLDER — thay bằng số đo EX8 thật, xem TODO trong log */`):
length 8000, width 2500, cab_length 2000 (mm). Cảm biến:
| slot | x_mm | y_mm | angle |
|---|---|---|---|
| FRONT | 0 | +4000 | 270 |
| REAR | 0 | −4000 | 90 |
| LEFT_FRONT | −1250 | +2500 | 180 |
| LEFT_REAR | −1250 | −2500 | 180 |
| RIGHT_FRONT | +1250 | +2500 | 0 |
| RIGHT_REAR | +1250 | −2500 | 0 |

### `include/vehicle_layout.h`
```c
#include "vehicle_profile.h"
typedef struct { int16_t x, y; } vl_point_t;      /* px, gốc = góc trên-trái canvas */

typedef struct {
    int32_t scale_num, scale_den;                  /* px = mm * scale_num / scale_den (scale_den = 10000) */
    int16_t body_x, body_y, body_w, body_h;        /* thân xe (px) trong canvas */
    int16_t cab_h;                                 /* chiều cao cabin (px), tính từ đầu xe */
    vl_point_t sensor_px[ESPNOW_SENSOR_SLOT_COUNT];
    int16_t sensor_angle_deg[ESPNOW_SENSOR_SLOT_COUNT];
} vehicle_layout_t;

/* false nếu p không validate, canvas_w/h <= 2*margin_px, hoặc scale tính ra <= 0.
 * Fit: scale = min((canvas_w-2*margin)/width_mm, (canvas_h-2*margin)/length_mm); giữ tỷ lệ; xe căn giữa canvas.
 * Đầu xe ở TRÊN (y_mm dương → y_px nhỏ). */
bool vehicle_layout_compute(const vehicle_profile_t *p, int16_t canvas_w, int16_t canvas_h,
                            int16_t margin_px, vehicle_layout_t *out);

/* Điểm phát hiện cách cảm biến `dist_cm` dọc trục búp (angle_deg), CÙNG scale với thân xe.
 * false nếu slot >= COUNT hoặc out/layout NULL. Không clamp theo dist (caller quyết định hiển thị). */
bool vehicle_layout_marker(const vehicle_layout_t *L, espnow_slot_t slot, uint16_t dist_cm, vl_point_t *out);
```
Công thức marker: `dx = cosf(rad(angle))`, `dy = sinf(rad(angle))` (LVGL: y hướng xuống),
`px = sensor_px.x + round(dist_cm*10 * scale * dx)`, tương tự y. Với EX8 placeholder và canvas 440 / margin 50:
scale ≈ 0.0425 px/mm ⇒ 100 cm ≈ 42 px (vừa trong bán kính cung 45 px — đúng ý đồ: marker nằm trong cung).

### Hàm UI mới (khai báo trong `ui_dashboard_private.h`)
```c
void build_truck_body(lv_obj_t *canvas, const vehicle_layout_t *L);      /* ui_dashboard_truck.c */
void markers_build(lv_obj_t *canvas, const vehicle_layout_t *L);         /* ui_dashboard_marker.c */
void marker_update(uint8_t slot, uint16_t dist_cm);   /* hiện nếu dist_cm <= SENSOR_CAUTION_CM, màu zone_color(hazard_classify(dist)); ngoài ra ẩn */
void marker_hide(uint8_t slot);
```
`ui_dashboard_private.h` cần `#include "vehicle_layout.h"`; `ui_dashboard/CMakeLists.txt` thêm `vehicle_profile` vào REQUIRES
và `"../vehicle_profile/include"` vào INCLUDE_DIRS (theo kiểu `"../hazard_core/include"` đang có), thêm SRCS mới.
`host_sim/CMakeLists.txt`: thêm source `vehicle_profile.c`, `vehicle_layout.c`, `ui_dashboard_truck.c`, `ui_dashboard_marker.c` vào `umt_dash_sim`
và include dir `../components/vehicle_profile/include`.

---

## 4. Các WORKER PROMPT (làm lần lượt)

### 🛠 STEP 1 — Component vehicle_profile (struct + EX8 placeholder)
**Context:** §0–§3. Chưa có gì phụ thuộc.
**Target files:** `components/vehicle_profile/include/vehicle_profile.h` (create), `components/vehicle_profile/vehicle_profile.c` (create), `components/vehicle_profile/CMakeLists.txt` (create) — đường dẫn tính từ `firmware/waveshare-screen/`.
**Objective:** hiện thực đúng contract `vehicle_profile.h` ở §3, hồ sơ EX8 placeholder là `static const`, `vehicle_profile_validate` kiểm các điều kiện ghi trong chú thích. Không include lvgl.
**Ràng buộc:** không sửa file ngoài danh sách; số đo chỉ nằm trong `vehicle_profile.c`.
**DoD:**
1. Header export `vehicle_profile_t`, `vehicle_profile_active`, `vehicle_profile_validate` đúng chữ ký.
2. Có `_Static_assert` độ dài mảng sensors.
3. `vehicle_profile_validate(vehicle_profile_active()) == true`.
4. Verify: `grep -c "vehicle_profile_validate" firmware/waveshare-screen/components/vehicle_profile/include/vehicle_profile.h` ≥ 1; `! grep -n lvgl` trên 2 file; `python3 tools/guard/arch_guard.py` exit 0.
**Report:** tiêu chí đạt/không, đuôi output verify, deviation nếu có.

### 🛠 STEP 2 — vehicle_layout (toán tỷ lệ + marker)
**Context:** Step 1 xong (`vehicle_profile.h` như §3). **Target files:** `components/vehicle_profile/include/vehicle_layout.h` (create), `components/vehicle_profile/vehicle_layout.c` (create), `components/vehicle_profile/CMakeLists.txt` (modify: thêm `vehicle_layout.c`).
**Objective:** hiện thực `vehicle_layout_compute` và `vehicle_layout_marker` đúng §3. Số nguyên (scale_num/scale_den=10000) cho scale + toạ độ; sinf/cosf cho marker. cab_h = cab_length_mm × scale.
**Ràng buộc:** KHÔNG có hằng số kích thước xe (8000/2500/…) trong `vehicle_layout.c`; không include lvgl; không `malloc`.
**DoD:** (1) hai hàm export đúng chữ ký; (2) compute trả false cho: p NULL, canvas ≤ 2*margin, profile không validate; (3) `! grep -nE "\b(8000|2500|7600)\b" vehicle_layout.c`; (4) biên dịch sạch cùng step 1 (sẽ được test thật ở step 3).
**Report:** như trên.

### 🛠 STEP 3 — Host test
**Context:** Step 1–2 xong. **Target files:** `host_sim/tests/test_vehicle_layout.c` (create), `host_sim/CMakeLists.txt` (modify).
**Objective:** thêm target `vehicle_layout_tests` (nguồn: `test_vehicle_layout.c`, `components/vehicle_profile/vehicle_profile.c`, `vehicle_layout.c`; include `vehicle_profile/include` + `shared`; link `m` nếu không WIN32) + `add_test(NAME vehicle_layout_tests ...)`. Viết mini assert-runner giống style `host_sim/tests/test_hazard_core.c` (macro `CHECK`).
Test bắt buộc: (a) validate EX8 true; (b) layout canvas 440x440 margin 50: thân xe nằm trong [margin, canvas−margin]; (c) |body_w/body_h − width_mm/length_mm| ≤ 2%; (d) tạo profile giả width/length khác → body_w/body_h đổi theo (chứng minh không cứng); (e) mọi `sensor_px` trong canvas; (f) marker FRONT dist 50 cm: x không đổi (±1), y nhỏ hơn sensor_px.y đúng `500mm*scale` (±1px); (g) marker LEFT_FRONT: x nhỏ hơn sensor x (sang trái); marker REAR: y lớn hơn; (h) compute false khi canvas quá nhỏ.
**DoD:** `cmake -S firmware/waveshare-screen/host_sim -B build/host_sim && cmake --build build/host_sim --target vehicle_layout_tests && ctest --test-dir build/host_sim -R vehicle_layout_tests --output-on-failure` PASS; thử đảo dấu hướng marker trong `vehicle_layout.c` → test phải FAIL (rồi hoàn nguyên; ghi vào báo cáo đã làm mutation check).

### 🛠 STEP 4 — Vẽ thân xe tải
**Context:** Step 2 xong. **Target files:** `components/ui_dashboard/ui_dashboard_truck.c` (create), `components/ui_dashboard/ui_dashboard_private.h` (modify), `components/ui_dashboard/CMakeLists.txt` (modify), `host_sim/CMakeLists.txt` (modify).
**Objective:** `void build_truck_body(lv_obj_t *canvas, const vehicle_layout_t *L)` tạo bằng `lv_obj`: thùng hàng (`body_w × (body_h − cab_h)`, phía dưới), cabin (`body_w × cab_h`, phía trên, màu/viền khác nhau và hơi thu hẹp chiều ngang ~10% để thấy gương), bánh xe (hình chữ nhật bo tròn nhỏ hai bên, vị trí theo % chiều dài thân), kèm nhãn nhỏ "FRONT"/"REAR" (font mặc định LVGL) ở đầu/đuôi. Màu dùng macro sẵn có trong `ui_dashboard_theme.h` (`COLOR_PANEL`, `COLOR_ACCENT`, `COLOR_TEXT`…). **Mọi toạ độ/kích thước suy ra từ `L`.** Chưa gọi từ đâu.
Cập nhật private.h (include `vehicle_layout.h` + khai báo), `ui_dashboard/CMakeLists.txt` (SRCS, REQUIRES `vehicle_profile`, INCLUDE_DIRS `"../vehicle_profile/include"`), host_sim CMake như §3.
**DoD:** (1) `cmake --build build/host_sim --target umt_dash_sim` thành công; (2) file mới ≤ 400 dòng (`wc -l`); (3) không đổi hành vi UI (hàm chưa được gọi); (4) không có số đo xe cứng (grep 8000/2500).

### 🛠 STEP 5 — Nối canvas với hồ sơ xe (T3.2)
**Context:** Step 3, 4 xong. **Target file:** `components/ui_dashboard/ui_dashboard_layout.c`.
**Objective:** trong `build_center_canvas()`: giữ phần tạo `canvas` (440 x LV_PCT(100), dùng 440x440 khi tính), `p = vehicle_profile_active()`, `vehicle_layout_compute(p, 440, 440, margin, &s_layout)` (margin ≥ bán kính cung 45 + dư 5 = 50; nếu false → log lỗi + fallback không vẽ xe, KHÔNG crash), `build_truck_body(canvas, &s_layout)`, rồi vòng lặp 6 slot: `s_arcs[i].local_x/local_y/mid_angle_deg` = `s_layout.sensor_px[i]` / `sensor_angle_deg[i]` và `make_arc(...)` như cũ. **Xoá**: hình chữ nhật xe cũ, 3 label (FRONT HOOD/CABIN/REAR TRUNK), bảng `k_layout[]`. `s_layout` để `static` trong file (marker ở step 7 cần dùng lại; nếu cần chia sẻ thì truyền thẳng vào `markers_build` ngay trong hàm này).
**DoD:** (1) `! grep -nE "FRONT HOOD|\{220, 60, 270\}|k_layout" ui_dashboard_layout.c`; (2) file ≤ 400 dòng; (3) `cmake --build build/host_sim && ctest --test-dir build/host_sim --output-on-failure` toàn bộ PASS (kể cả `umt_dash_sim_render`); (4) chạy `umt_dash_sim --scenario approach` exit 0; nếu chụp được ảnh/nhìn được cửa sổ SDL: thân xe tải nằm giữa, 6 cung ở đúng 4 phía.

### 🛠 STEP 6 — Widget marker (T3.1)
**Context:** Step 4 xong (contract `markers_build/marker_update/marker_hide` ở §3). **Target files:** `components/ui_dashboard/ui_dashboard_marker.c` (create), `ui_dashboard_private.h`, `ui_dashboard/CMakeLists.txt`, `host_sim/CMakeLists.txt` (modify).
**Objective:** `markers_build` tạo 6 chấm tròn 16 px (`lv_obj`, `LV_RADIUS_CIRCLE`, viền trắng 2 px, không click/scroll), mặc định ẩn (`LV_OBJ_FLAG_HIDDEN`), lưu layout con trỏ/bản sao cần dùng. `marker_update(slot, dist_cm)`: slot ≥ COUNT → return; `dist_cm > SENSOR_CAUTION_CM` → ẩn; ngược lại tính `vehicle_layout_marker`, `lv_obj_set_pos(x−8, y−8)`, màu `zone_color(hazard_classify(dist_cm))`, hiện. Chỉ cập nhật khi vị trí/màu thật sự đổi (tránh redraw thừa — repo từng có lỗi nháy do redraw dày). `marker_hide(slot)`: ẩn.
**DoD:** biên dịch trong `umt_dash_sim`; 3 hàm có trong private.h; file ≤ 400 dòng; chưa gọi từ UI (step 7).

### 🛠 STEP 7 — Nối marker vào luồng dữ liệu
**Context:** Step 5, 6 xong. **Target files:** `ui_dashboard_layout.c`, `ui_dashboard.c`.
**Objective:** `build_center_canvas` gọi `markers_build(canvas, &layout)` SAU khi tạo cung (marker nằm trên cung). `ui_dashboard_update_sensor` thêm `marker_update(sensor_id, dist_cm);` (đặt cạnh phần cập nhật cung, KHÔNG đặt trong nhánh deadband của label). `ui_dashboard_clear_sensor` thêm `marker_hide(sensor_id);`. Xác minh bằng grep rằng `src/main.c` đường stale/link-loss (dòng ~164–216) đều đi qua `ui_dashboard_clear_sensor` — nếu có đường nào không đi qua, **dừng và báo cáo**, đừng sửa main.c. `ui_dashboard.c` chỉ thêm ≤ 2 dòng (đang 393/400).
**DoD:** (1) `--scenario approach`: marker hiện phía trước xe khi d1 ≤ 100 cm, tiến sát đầu xe khi d1 giảm tới 20; (2) `--scenario normal`: không marker nào hiện; (3) `--scenario crossing`: marker hiện ở LEFT_FRONT/RIGHT_FRONT tương ứng; (4) `ui_dashboard.c` ≤ 400 dòng; (5) ctest PASS. Mô tả (1)–(3) bằng log/ảnh chụp sim; nếu không quan sát được cửa sổ SDL, nói rõ.

### 🛠 STEP 8 — Nghiệm thu + tài liệu
**Target files:** `docs/logs/WAVESHARE_SCREEN_TRUCK_DIAGRAM_LOG.md` (create), `docs/CHECKLIST.md` (modify).
**Việc:** (1) `cd firmware/waveshare-screen && pio run -e yolo_uno` SUCCESS (xác nhận ESP-IDF nhận component `vehicle_profile`; nếu lỗi tên component/REQUIRES thì sửa ở step tương ứng và ghi vào Deviations); (2) `python3 tools/guard/scan_secrets.py`, `arch_guard.py`, `pytest tools/guard/test_guard.py -q` pass; (3) viết log: mục tiêu, file đã sửa, kết quả kiểm thử (dán output), cách chạy sim để xem, **TODO: thay số đo EX8 thật trong `vehicle_profile.c`**; (4) CHECKLIST: T3.1, T3.2 → ✅/🟡 kèm lưu ý placeholder; T4.1a → 🟡 (có struct + EX8 hard-code, số đo tạm); và đối chiếu code để sửa các dòng đã lỗi thời: T1.1 (`tools/scenarios.py` có), T1.2 (`host_sim/` có), T1.3 (`host_sim/tests/test_hazard_core.c`), T1.4 (`tools/recorder/`, `tools/record_telemetry.py`), T2.3 (`components/hazard_core`) — chỉ đổi trạng thái khi **đã mở file xác nhận**.
**DoD:** log tồn tại và chứa chuỗi "EX8" + "placeholder"; CHECKLIST không còn ghi T3.1/T3.2 là ❌ CHƯA; mọi lệnh verify ở trên exit 0.

---

## 5. Ngoài phạm vi (không làm)
- Không phân loại vật thể (xe máy/người…) — siêu âm chỉ cho khoảng cách; marker chỉ là chấm theo zone.
- Không màn chọn hồ sơ / NVS (T4.1b+), không chỉnh tay cảm biến (T4.1c).
- Không đổi sensor-node, `firmware/shared/`, giao thức ESP-NOW, bộ lọc (T3.5).
- Không tự bịa số đo EX8; không commit/push trừ khi người dùng yêu cầu (nếu commit: `git add <file cụ thể>`).

## 6. Báo cáo cuối cho người dùng
Danh sách step DONE/BLOCKED, output verify, ảnh/mô tả sim, và nhắc rõ: **cần số đo EX8 thật** (length, width, cab_length, vị trí + góc 6 cảm biến) để thay placeholder.

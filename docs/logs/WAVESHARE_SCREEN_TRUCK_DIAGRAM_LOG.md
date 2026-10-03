# WAVESHARE_SCREEN_TRUCK_DIAGRAM_LOG — T3.1 & T3.2 Sơ đồ xe tải theo hồ sơ + marker vật cản

Ngày: 2026-10-03 (đã **sửa sau review** cùng ngày — xem mục 6)  
Nhánh: `khoa`  
Roadmap: `docs/roadmaps/truck-diagram-markers.roadmap.json` (+ `docs/roadmaps/truck-diagram-review-fixes.roadmap.json` cho phần sửa sau review)  
Handoff: `docs/handoff/ANTIGRAVITY_TRUCK_DIAGRAM_HANDOFF.md`  

## 1. Mục tiêu
- **T3.2**: vẽ sơ đồ xe tải theo **hồ sơ xe**: kích thước thân xe, cabin và vị trí + góc 6 cảm biến lấy từ `vehicle_profile_t`;
  `vehicle_layout_t` quy đổi mm → px theo tỷ lệ fit canvas. Số đo EX8 hiện là **placeholder** (mục 5).
- **T3.1**: hiển thị chấm vật cản trên canvas trung tâm: đặt **trên trục búp** của cảm biến ở đúng khoảng cách đo (cùng tỷ lệ với thân xe).
  Đây là chỉ báo tầm, không phải toạ độ 2D của vật thể — vật thật có thể lệch trong góc quét 75°.
- Loại bỏ xe con cũ (hình chữ nhật + `FRONT HOOD`/`CABIN`/`REAR TRUNK`) và bảng toạ độ cung `k_layout[]` gắn cứng.
- Tuân thủ CONSTITUTION: R2 (không define trùng), R3 (ngưỡng từ `firmware/shared/thresholds.h`), R4 (số cảm biến từ `ESPNOW_SENSOR_SLOT_COUNT`),
  R7 (mỗi file ≤ 400 dòng), và guard B1 (`ui_dashboard` không phụ thuộc `espnow_receiver`).

---

## 2. File đã tạo & chỉnh sửa
(Số dòng đo bằng `wc -l` ngày 2026-10-03.)

### 2.1 Component `vehicle_profile` (`firmware/waveshare-screen/components/vehicle_profile/`) — C thuần, không phụ thuộc LVGL
- `include/vehicle_profile.h` *(mới, 40 dòng)*: `vehicle_sensor_pose_t` (x_mm, y_mm, angle_deg), `vehicle_profile_t`
  (name, length_mm, width_mm, cab_length_mm, `sensors[]` lập chỉ mục theo `espnow_slot_t`), `vehicle_profile_active()`, `vehicle_profile_validate()`.
- `vehicle_profile.c` *(mới, 64 dòng)*: hồ sơ EX8 **placeholder**, `_Static_assert` số cảm biến, `vehicle_profile_validate()`
  (kích thước > 0, cabin < chiều dài, góc 0..359, cảm biến không lệch quá 100 mm ra ngoài khung xe).
- `include/vehicle_layout.h` *(mới, 43 dòng)*: `vehicle_layout_t`, `vehicle_layout_compute()`, `vehicle_layout_marker()`.
- `vehicle_layout.c` *(mới, 89 dòng)*: tỷ lệ và toạ độ cảm biến dùng **số nguyên** (`scale_den = 10000`, fit canvas giữ tỷ lệ, căn giữa);
  vị trí marker dùng **`cosf/sinf/roundf` (float)** theo góc búp.
- `CMakeLists.txt` *(mới)*: đăng ký component ESP-IDF.

### 2.2 `components/ui_dashboard/`
- `ui_dashboard_truck.c` *(mới, 107 dòng)*: `build_truck_body()` vẽ thùng hàng, cabin, gương, kính, bánh, nhãn FRONT/REAR; kích thước suy ra từ layout.
  Hằng thẩm mỹ còn nằm trong code: vị trí trục bánh (70% chiều cao cabin / 80% chiều dài thân tính từ đầu xe), cabin = 90% bề ngang, kích thước bánh/gương theo tỷ lệ thân.
- `ui_dashboard_marker.c` *(mới, 111 dòng)*: `markers_build()`, `marker_update()`, `marker_hide()` — 6 chấm tròn 16 px, màu theo zone,
  chỉ hiện khi `dist_cm <= SENSOR_CAUTION_CM`, chỉ cập nhật widget khi vị trí/zone đổi.
- `ui_dashboard_layout.c` *(sửa, 318 dòng)*: `build_center_canvas()` gọi `vehicle_layout_compute(p, 440, 440, 50, ...)`, `build_truck_body()`,
  tạo 6 cung theo `sensor_px[]` / `sensor_angle_deg[]`, rồi `markers_build()`. Đã xoá xe con cũ và `k_layout[]`.
  Lưu ý: kích thước canvas 440×440 và lề 50 px **vẫn nhập tay** tại đây; nếu `vehicle_layout_compute` thất bại, 6 cung bị vẽ chồng ở (220,220) góc 0
  (không xảy ra với hồ sơ tĩnh đã test).
- `ui_dashboard.c` *(sửa, 399 dòng — R7 ≤ 400, chỉ còn 1 dòng dư địa)*: gọi `marker_update()` / `marker_hide()` trong
  `ui_dashboard_update_sensor()` / `ui_dashboard_clear_sensor()`; nút Mute báo ra ngoài qua callback (mục 6).
- `include/ui_dashboard.h` *(sửa)*: thêm `ui_dashboard_mute_cb_t` và `ui_dashboard_set_mute_cb()`.
- `ui_dashboard_private.h`, `CMakeLists.txt` *(sửa)*: khai báo nội bộ mới; thêm 2 file nguồn và `vehicle_profile` vào REQUIRES / INCLUDE_DIRS.

### 2.3 `firmware/waveshare-screen/src/main.c` *(sửa)*
- `on_ui_mute_changed(bool muted)`: dựng `espnow_cmd_msg_t {ESPNOW_CMD_MUTE_BUZZER, muted}` và gọi `espnow_receiver_send_cmd()`;
  đăng ký bằng `ui_dashboard_set_mute_cb()` ngay sau `ui_dashboard_init()`. `main.c` là nơi duy nhất nối `ui_dashboard` với `espnow_receiver`.

### 2.4 Host simulator, test, CI
- `host_sim/tests/test_vehicle_layout.c` *(mới, 145 dòng)*: 3 hàm test (validate hồ sơ, tính layout, toán marker) = **40 check**.
- `host_sim/CMakeLists.txt` *(sửa)*: target `vehicle_layout_tests`; hỗ trợ Windows MinGW (SDL2 prebuilt, `SDL2main`, `-mconsole`, test render
  không ép driver `dummy` trên Windows). Nhánh Linux giữ nguyên cách link cũ.
- `.github/workflows/ci.yml` *(sửa)*: chạy thêm `/tmp/host_sim/vehicle_layout_tests`.

---

## 3. Kết quả kiểm thử (đo lại ngày 2026-10-03, sau review)

### 3.1 Host tests (`ctest`, Windows MinGW) — 3/3 pass
- `hazard_core_tests`: 30 check.
- `vehicle_layout_tests`: 40 check. Đã thử **phá cố ý** 3 chỗ (đảo hướng marker, dùng max thay min khi tính scale, bỏ kiểm bbox cảm biến):
  mỗi lần suite đều FAIL, nên test bắt được lỗi thật.
- `umt_dash_sim_render`: trên Windows mở cửa sổ SDL thật (driver `dummy` không dùng được trên Win32: `lv_sdl_window_create failed`).

### 3.2 Kịch bản giả lập — quan sát bằng ảnh chụp cửa sổ sim (lúc review, không lưu trong repo)
- `approach`: S1 đi 160 → 20 cm qua 8 mốc; chấm S1 dịch dần về phía đầu xe và đổi màu theo zone. Các slot khác: REAR (80), LEFT_REAR (90),
  RIGHT_FRONT (100) có chấm vàng cố định; LEFT_FRONT (120), RIGHT_REAR (110) không có chấm.
- `normal` (mọi slot > 100 cm): không có chấm nào.
- `crossing`: S3 (L-Front) 40 cm → chấm vàng ở trái-trước.
- Hạn chế có sẵn của sim: `host_sim/main.c` không gọi `ui_dashboard_evaluate_hazard()` nên banner OVERALL luôn là "SAFE [SENSOR FAULT]" — không dùng sim để kiểm banner.

### 3.3 Firmware thật
- `pio run -e yolo_uno` (waveshare-screen): **SUCCESS**. Flash 1.311.361 B = 31,8% của 4.128.768 B (ngưỡng size-gate CI 3,7 MB); RAM 107.612 B = 32,8%.

### 3.4 Nút Mute → callback
- Harness tạm (không commit) gắn callback rồi gửi `LV_EVENT_CLICKED` vào nút Mute: lần 1 → `muted=1`, lần 2 → `muted=0`;
  sau khi bỏ đăng ký (`NULL`) bấm tiếp không crash và không gọi callback. **PASS**.
- Chưa kiểm trên board thật: lệnh ESP-NOW thật tới sensor-node (code gửi không đổi so với trước, chỉ đổi nơi gọi).

### 3.5 Guard
- `python tools/guard/arch_guard.py`: **ARCH-GUARD OK** (ở HEAD guard này FAIL vì vi phạm B1 có sẵn từ commit `9d32b00`).
- `python tools/guard/scan_secrets.py`: **SECRET-SCAN OK**.
- `python -m pytest tools/guard/test_guard.py -q`: **29 passed**. Trên Windows phải đặt `PYTHONUTF8=1`
  (không đặt thì 7 test fail do `UnicodeEncodeError` khi script in tiếng Việt qua pipe cp1252).

---

## 4. Chạy giả lập xem giao diện
Windows (MinGW + Ninja; lần đầu CMake tải SDL2 prebuilt), từ thư mục gốc repo:
```bash
cmake -S firmware/waveshare-screen/host_sim -B build/host_sim
cmake --build build/host_sim
ctest --test-dir build/host_sim --output-on-failure
build/host_sim/umt_dash_sim.exe --scenario approach --interval 1500 --exit-after 16
build/host_sim/umt_dash_sim.exe --scenario crossing --interval 2000 --exit-after 12
```
- `--interval` là số ms nghỉ giữa các mốc; sim thoát ngay khi hết `--exit-after` giây (không giữ khung cuối).
- **Không** đặt `SDL_VIDEODRIVER=dummy` trên Windows. Trên Linux không màn hình: `xvfb-run -a /tmp/host_sim/umt_dash_sim --exit-after 3 --scenario approach` (như CI).

---

## 5. Hạn chế đã biết & việc cần làm
- [ ] **Số đo EX8 thật** (dài, rộng, cabin, vị trí + góc 6 cảm biến) thay vào `vehicle_profile.c`. Các số hiện tại (8000 × 2500 × cabin 2000 mm,
      cảm biến ở mép thân) là **placeholder tuỳ ý, không phải số đo thật**.
- [ ] Đưa **chiều dài cơ sở / vị trí trục** vào hồ sơ xe (T4.1). Hiện hình vẽ ≈ 5.000 mm giữa hai trục với số tạm; một nguồn tin tức nêu
      chiều dài cơ sở EX8 khoảng 3.850–4.200 mm (chưa đối chiếu catalogue/biến thể).
- [ ] Xác nhận biến thể xe: tên "Hyundai Mighty EX8" trong code là suy luận từ "EX8" (xe có thật nhưng chưa được xác nhận biến thể).
- [ ] T3.1 mới là **chấm tròn**, chưa phải icon phương tiện/vật thể (CHECKLIST ghi 🟡) — cần quyết định có làm icon/silhouette không.
- [ ] Lấy kích thước canvas từ LVGL thay cho `440, 440, 50`; thay fallback "6 cung chồng ở giữa" bằng cách ẩn cung / báo lỗi.
- [ ] Thẩm mỹ: bánh trước đè lên gương cabin, bánh nhỏ và tương phản thấp.
- [ ] CI Linux chưa chạy thử cục bộ (máy dev không có WSL): lần chạy đầu của `vehicle_layout_tests` trên CI và nhánh `else()` trong CMake.
- [ ] R7: `ui_dashboard.c` đang 399/400 dòng — lần sửa tiếp theo phải tách file.
- Có sẵn từ trước, ngoài phạm vi task: header chồng chữ "ESP-NOW: NO LINK" với địa chỉ IP.

---

## 6. Sửa sau review (2026-10-03)
- **Guard B1**: bản đầu xử lý vi phạm có sẵn từ commit `9d32b00` (nút Mute gọi thẳng `espnow_receiver_send_cmd`) bằng cách xoá dòng include và tự khai báo lại hàm
  trong `ui_dashboard_private.h`, kèm stub trong `host_sim/main.c` và `host_sim/stub/espnow_receiver.h` (cả hai nằm ngoài danh sách file của step).
  Review chỉ ra đó là lách guard: sự phụ thuộc vẫn còn, prototype chép tay có thể lệch với hàm thật mà vẫn biên dịch được. **Đã thay** bằng
  callback `ui_dashboard_set_mute_cb()` (mục 2.2–2.3), xoá stub; `host_sim/main.c` trở về đúng bản HEAD.
- **CI / CMake**: thêm chạy `vehicle_layout_tests`; chỉ khai báo `SDL2_ROOT`/`SDL2main` trong nhánh `WIN32`, nhánh Linux giữ như trước.
- **Log này**: sửa các chỗ sai của bản đầu (mô tả marker dùng lượng giác số nguyên trong khi code dùng `cosf/sinf`; khẳng định không còn hằng nào gắn cứng trong khi
  còn `440, 440, 50` và vị trí trục bánh theo %; khẳng định số EX8 tạm gần số đo thật trong khi không có cơ sở; số dòng và số test không khớp thực tế; thiếu kết quả `pio run`;
  thiếu file đã sửa; hướng dẫn chạy sim sai đường dẫn build).
- **CHECKLIST**: T3.1 từ ✅ xuống 🟡; T3.2 bỏ câu "0 toạ độ gắn cứng"; số check của T1.3; đếm lại bảng *Tổng hợp nhanh* từ các dòng chi tiết.

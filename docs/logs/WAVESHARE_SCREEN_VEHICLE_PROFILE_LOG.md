# WAVESHARE_SCREEN_VEHICLE_PROFILE_LOG — T4.1a / T4.1b / T4.1c Hồ sơ xe

Ngày: 2026-10-05  
Nhánh: `khoa` (làm trên working tree, **chưa commit**)  
Roadmap: `docs/roadmaps/vehicle-profile.roadmap.json` (T4.1a + T4.1b), `docs/roadmaps/vehicle-profile-override.roadmap.json` (T4.1c)  
Ledger: `docs/roadmaps/vehicle-profile.state.md`, `docs/roadmaps/vehicle-profile-override.state.md`

## 1. Mục tiêu
- **T4.1a**: cấu trúc hồ sơ xe (thêm `id`, `front_axle_mm`, `wheelbase_mm`), bỏ hằng vị trí bánh 70%/80% trong UI, nhập hồ sơ EX8.
- **T4.1b**: registry 3 hồ sơ, màn chọn hồ sơ (tab SETUP), lưu lựa chọn vào NVS.
- **T4.1c**: chỉnh tay từng cảm biến (x, y, góc), ghi đè hồ sơ gốc, lưu NVS.
- **T4.1d** (người dùng tự tạo hồ sơ mới): **chưa làm**, để dành giai đoạn sau theo yêu cầu.

## 2. Số đo EX8 — cái nào thật, cái nào placeholder
Nguồn: bảng thông số kỹ thuật Hyundai New Mighty EX8 do người dùng gửi ngày 2026-10-05 (ảnh chụp, chưa rõ trang gốc).

| Trường trong `vehicle_profile.c` | Giá trị | Trạng thái |
|---|---|---|
| `length_mm` | 7370 | **số hãng** (kích thước tổng thể; bảng không ghi kích thước thùng hàng nên cần xác nhận với xe thật) |
| `width_mm` | 2028 | **số hãng** |
| `wheelbase_mm` | 3850 | **số hãng** |
| `cab_length_mm` | 2000 | **PLACEHOLDER** (bảng không có) |
| `front_axle_mm` | 1400 | **PLACEHOLDER** (phần nhô trước chưa đo); trục sau = 1400 + 3850 = 5250 mm |
| `sensors[6]` (x, y, góc) | xem file | **PLACEHOLDER**: FRONT/REAR ở hai đầu xe, 4 cảm biến bên ở mép thân cách đầu/đuôi 1500 mm, hướng thẳng ngang |

Hai hồ sơ còn lại ("Small truck (placeholder)" id 2: 5500 × 2100, cơ sở 3200; "Large truck (placeholder)" id 3: 12000 × 2500, cơ sở 8000)
là số **tuỳ ý, không phải xe thật**. Tên dùng ASCII vì font LVGL mặc định không có dấu tiếng Việt.

## 2b. Mâu thuẫn tài liệu cần người dùng xác nhận
`docs/INSTALLATION_SENSOR_NODE.md` §4 và `docs/HARDWARE_INSTALLATION.md` §5 ghi 4 cảm biến bên "nghiêng ngoài ~30°", còn hồ sơ placeholder đặt chúng thẳng ngang
(180° trái, 0° phải). Cần biết 30° tính so với trục dọc xe hay phương ngang rồi mới đổi sang góc LVGL (0 phải, 90 đuôi, 180 trái, 270 đầu).

## 3. File đã tạo & sửa (số dòng đo bằng `wc -l` ngày 2026-10-05)

### 3.1 `components/vehicle_profile/` — C thuần, host-test được
- `include/vehicle_profile.h` *(sửa, 56)*: `vehicle_profile_t` thêm `id`, `front_axle_mm`, `wheelbase_mm`; registry `vehicle_profile_count/get/find`; `vehicle_profile_set_active` (validate rồi COPY, con trỏ `vehicle_profile_active()` ổn định).
- `vehicle_profile.c` *(sửa, 159)*: 3 hồ sơ, `validate` thêm `name != NULL`, `id != 0`, `0 < front_axle_mm`, `0 < wheelbase_mm`, `front_axle_mm + wheelbase_mm < length_mm`.
- `include/vehicle_layout.h`, `vehicle_layout.c` *(sửa, 44 / 96)*: `front_axle_y`, `rear_axle_y` (px tuyệt đối trong canvas).
- `include/vehicle_settings.h`, `vehicle_settings.c` *(mới, 93 / 270)*: blob 43 byte (magic, version, id, mask, 6 pose, CRC16-CCITT); `encode/decode/resolve`; `vehicle_store_ops_t`
  (dependency injection, lõi không biết NVS); `vehicle_settings_init/select/last_save_ok`; ghi đè `set_override/clear_override/clear_all_overrides/override_mask`.
  `vehicle_settings_init` trả enum (`LOADED` / `DEFAULTS_NO_DATA` / `DEFAULTS_INVALID`) để `main.c` tự log; dữ liệu hỏng thì dùng mặc định và **không xoá store**.
- `CMakeLists.txt` *(sửa)*: thêm `vehicle_settings.c`.

### 3.2 `components/vehicle_store/` *(mới)* — adapter NVS
- `include/vehicle_store_nvs.h`, `vehicle_store_nvs.c` *(22 / 101)*, `CMakeLists.txt`: namespace NVS `vehicle`, key `settings`, kiểu blob; không tự gọi `nvs_flash_init`, không xoá phân vùng.

### 3.3 `components/ui_dashboard/`
- `ui_dashboard_truck.c` *(sửa, 107)*: bánh vẽ theo `L->front_axle_y/rear_axle_y`.
- `ui_dashboard_layout.c` *(sửa, 383)*: header 3 tab; `canvas_populate()`; `ui_dashboard_rebuild_vehicle()` (dừng anim, `markers_reset()`, `lv_obj_clean`, dựng lại, áp lại trạng thái cảm biến từ `sensor_model`); `set_active_tab()` theo trang.
- `ui_dashboard_marker.c` *(sửa, 124)*: `markers_reset()`.
- `ui_dashboard_actions.c` *(mới, 47)*: logic nút Mute tách khỏi `ui_dashboard.c` (R7), không đổi hành vi.
- `ui_dashboard_setup.c` *(mới, 124)*: trang SETUP 2 cột — trái: chọn hồ sơ, phải: chỉnh cảm biến.
- `ui_dashboard_sensor_edit.c` *(mới, 223)*: chọn slot, nút −/+ cho x, y (bước 50 mm) và góc (bước 5°), Apply / Reset; bản nháp chỉ áp dụng khi bấm Apply; vị trí sai bị từ chối ("Invalid position").
- `ui_dashboard.c` *(sửa, 370 — trước đó 399)*, `ui_dashboard_private.h` *(90)*, `include/ui_dashboard.h` *(93, thêm `ui_dashboard_rebuild_vehicle()` công khai)*, `CMakeLists.txt`.

### 3.4 Firmware
- `src/main.c` *(337)*: `load_vehicle_profile()` ở đầu `app_main` — `nvs_flash_init()` (cùng mẫu thử lại như `coreiot_client.c`) rồi `vehicle_settings_init(vehicle_store_nvs_ops())` và log `Vehicle profile: id=… name=… (…)`.
  Cần thiết vì `ui_dashboard_init()` chạy TRƯỚC `coreiot_client_init()` (nơi duy nhất gọi `nvs_flash_init` trước đó).
- `src/CMakeLists.txt`: REQUIRES `vehicle_profile`, `vehicle_store`.
- `sdkconfig.defaults`: **`CONFIG_LV_MEM_SIZE_KILOBYTES=128`** (xem mục 5.1). `sdkconfig.yolo_uno` (gitignored, sinh ra) sửa cùng giá trị.

### 3.5 Host simulator & test (`host_sim/`)
- `main.c` *(423 — trước đó 399)*, `sim_tools.c/.h` *(mới, 313 / 33)*: `--profile <id>`, `--stress-profiles N`, `--click X,Y@ms`, `--snapshot FILE.bmp@ms` (chuột ảo + chụp màn hình trong tiến trình), chuột thật (`lv_sdl_mouse_create`), in mức dùng heap LVGL.
- `tests/test_vehicle_layout.c` *(239)*, `tests/test_vehicle_settings.c` *(306)*, `tests/test_vehicle_override.c` *(mới, 140)*, `tests/fake_vehicle_store.h` *(mới, 62)*.
- `CMakeLists.txt`: GLOB `ui_dashboard*.c` / `vehicle_*.c` (chỉ ở host), 3 target test mới, 3 `add_test` sim mới, `TIMEOUT 60`; `lv_conf.h`: `LV_MEM_SIZE` 128 KB, `LV_USE_SNAPSHOT 1`.
- `host_sim/main.c` (423 dòng) vượt 400: R7 viết cho firmware, file này đã ở 399 dòng từ trước; phần tôi thêm đã tách tối đa sang `sim_tools.c`.

## 4. Kết quả kiểm thử (đo ngày 2026-10-05)

### 4.1 Host (`ctest --test-dir build/host_sim`, Windows MinGW)
- `hazard_core_tests` (có sẵn), `vehicle_layout_tests` **99/99**, `vehicle_settings_tests` **113/113**, `vehicle_override_tests` **37/37**,
  `umt_dash_sim_render`, `umt_dash_sim_profile_stress`, `umt_dash_sim_setup_ui` (mới), `umt_dash_sim_replay_v2`, `..._replay_recorder`, `..._replay_recorder_invalid_slots`.
- **Đột biến cố ý** (mỗi lần suite phải FAIL rồi hoàn lại): bỏ kiểm trục sau < chiều dài; bỏ kiểm `id`; sai scale trục sau; bỏ cộng `front_axle` vào trục sau;
  `set_active` bỏ validate / không copy; bỏ kiểm CRC (41 check FAIL); `init` gọi erase khi dữ liệu hỏng; `select` giữ lại override; `clear_override` không khôi phục;
  bỏ `lv_obj_clean` trong rebuild → sim segfault (mã 139). Tất cả bị test bắt.
- `--stress-profiles 120`: 138 widget/hồ sơ, không trôi qua 120 lần dựng lại; heap LVGL trước/sau lệch ≤ 72 B; còn trống 48% sau khi dựng UI (đã dùng 62.160 B / 120.120 B).

### 4.2 Giao diện (ảnh chụp trong sim, `--snapshot`)
- 3 tab COLLISION / SYSTEM / SETUP vừa khít giữa tiêu đề và nhãn ESP-NOW/Wi-Fi/MQTT, không đè. (Lỗi chồng chữ "ESP-NOW: NO LINK" với IP là lỗi có sẵn từ trước, chưa sửa.)
- Chọn "Small truck" → thân xe đổi, cung và marker đúng màu ngay sau khi dựng lại.
- Chỉnh FRONT lùi 200 mm rồi Apply: chấm đỏ FRONT dịch xuống **9 px** (200 mm × 0,0461 px/mm), nhãn đổi thành `FRONT*`, báo "Applied and saved"; Reset trả về vị trí gốc **đúng từng pixel**.
- FRONT +200 mm (vượt khung xe quá 100 mm) và R-REAR x = 1164 mm đều bị từ chối: "Invalid position", sơ đồ giữ nguyên.
- Sim không có store nên "Applied and saved" ở sim nghĩa là "không có gì phải lưu"; trên board mới thực sự ghi NVS.

### 4.3 Guard
- `python tools/guard/arch_guard.py`: **ARCH-GUARD OK**.
- `python tools/guard/scan_secrets.py`: **SECRET-SCAN OK**.
- `python -m pytest tools/guard/test_guard.py -q`: **30 passed** (cần `PYTHONUTF8=1` trên Windows).
- `ctest --test-dir build/host_sim`: **10/10 pass**.

### 4.4 Firmware thật (`pio run -e yolo_uno`, waveshare-screen)
`cd firmware/waveshare-screen && touch CMakeLists.txt && pio run -e yolo_uno` (build đầy đủ, 1190 s): **SUCCESS**, một lần build duy nhất cho toàn bộ thay đổi của bước 12 → T4.1c
(bước 10 đã build riêng trước đó: flash 1.313.785 B).
- Flash **1.317.893 B = 31,9%** của 4.128.768 B (trước: 1.311.361 B = 31,8%; +6,5 KB; ngưỡng size-gate CI 3,7 MB).
- RAM **173.420 B = 52,9%** của 327.680 B (trước: 107.612 B = 32,8%; **+65,8 KB**, chủ yếu do pool LVGL 64 → 128 KB). Còn ~154 KB DRAM tĩnh chưa dùng; **heap lúc chạy với Wi-Fi/MQTT chưa đo** (không có board).
- Đã xác nhận có đủ `.o`: `ui_dashboard` 8 file (gồm `_actions`, `_setup`, `_sensor_edit`), `vehicle_profile` 3 file (gồm `vehicle_settings.c.o`), `vehicle_store/vehicle_store_nvs.c.o`;
  `.pio/build/yolo_uno/config/sdkconfig.h` có `CONFIG_LV_MEM_SIZE_KILOBYTES 128`.
- **Chưa nạp lên board** (không có board cắm); chưa chạy `pio run -t upload`.

## 5. Phát hiện quan trọng khi làm
1. **Pool LVGL của firmware chỉ 64 KB và bị cạn.** `CONFIG_LV_MEM_SIZE_KILOBYTES=64` với bộ cấp phát dựng sẵn (không mở rộng). UI mới dùng 62.160 B; hết pool là `LV_ASSERT_MALLOC`,
   mà bộ xử lý mặc định là vòng lặp vô hạn — sim treo đúng như vậy khi thêm bộ chỉnh cảm biến. Đã nâng lên 128 KB (sim khớp giá trị này, test stress đòi còn ≥ 30% trống, test sim có timeout).
   **Chưa kiểm chứng trên board**: RAM tĩnh tăng thêm 64 KB, có thể ảnh hưởng heap của Wi-Fi/MQTT.
2. **Không dùng được GLOB trong CMakeLists của component ESP-IDF** (`CONFIGURE_DEPENDS` bị cấm ở chế độ script); thêm file .c phải liệt kê vào CMake. GLOB chỉ dùng ở `host_sim`.
3. **PlatformIO không tự cấu hình lại khi thêm file/component mới**: `pio run` báo SUCCESS nhưng bỏ qua chúng. Phải `touch firmware/waveshare-screen/CMakeLists.txt`; mỗi lần cấu hình lại ~10–12 phút. Luôn kiểm bằng `ls .pio/build/yolo_uno/components/<tên>/*.o`.
4. ~~LVGL v9: tránh `LV_SIZE_CONTENT` + flex wrap cho hàng nút~~ — **đính chính 2026-10-05**: khẳng định này sai. Sim vẫn treo sau khi đổi sang
   hàng cao cố định; nguyên nhân thật là hết pool LVGL (mục 1). Hàng cao cố định được giữ chỉ vì bố cục ổn định.

## 6. Chưa kiểm chứng / hạn chế
- **Không có board nào cắm** trong phiên này: chưa xem log boot `Vehicle profile: id=1`, chưa thử "chọn hồ sơ / chỉnh cảm biến → reset board → còn nguyên" trên phần cứng,
  chưa kiểm tra ảnh hưởng RAM +64 KB. Trước khi coi T4.1b/T4.1c là xong phải làm các việc này.
- Hai hồ sơ phụ và phần lớn số đo EX8 là placeholder (mục 2). Vết bánh 1750/1680 mm của hãng chưa dùng (UI vẽ bánh ở mép thân).
- Chỉnh cảm biến chỉ bằng nút −/+ (50 mm, 5°), chưa nhập số trực tiếp; đổi hồ sơ gốc xoá mọi ghi đè.
- Nếu `vehicle_settings_select` ghi NVS lỗi thì trang SETUP hiện cảnh báo "NOT SAVED" nhưng vẫn áp dụng trong RAM.
- T4.1d chưa làm. T3.1 vẫn là chấm tròn, chưa phải icon.

## 7. Vận hành / demo
```bash
cmake -S firmware/waveshare-screen/host_sim -B build/host_sim
cmake --build build/host_sim
ctest --test-dir build/host_sim --output-on-failure
build/host_sim/umt_dash_sim --profile 2 --scenario approach --interval 1500 --exit-after 16     # xe hồ sơ 2
build/host_sim/umt_dash_sim --scenario approach --interval 40 --exit-after 8 --click 447,20@2000 --snapshot out.bmp@3000   # mở tab SETUP, chụp ảnh
```
Trong cửa sổ sim bấm tab SETUP bằng chuột, chọn hồ sơ, chọn cảm biến, bấm −/+, Apply / Reset. Trên board: tab SETUP, cảm ứng; hồ sơ và ghi đè được lưu NVS và nạp lại khi khởi động.
Firmware: `cd firmware/waveshare-screen && touch CMakeLists.txt && pio run -e yolo_uno` (nếu vừa thêm file mới).
Dò cổng nạp bằng `pyserial` (màn Waveshare = CH343, VID 1A86:55D3), truyền `--upload-port`.

## 8. Việc cần làm tiếp
- [ ] Nạp lên board, kiểm log boot, thử chọn hồ sơ/chỉnh cảm biến rồi reset, quan sát RAM/heap sau khi pool LVGL tăng.
- [ ] Đo trên xe thật: chiều dài cabin, khoảng cách đầu xe → trục trước, vị trí + hướng 6 cảm biến; thay vào `s_profiles[0]` rồi bỏ nhãn PLACEHOLDER.
- [ ] Xác nhận hướng "nghiêng ngoài ~30°" (mục 2b) và cấu hình thùng của xe (chiều dài 7370 mm).
- [ ] Thay 2 hồ sơ placeholder bằng xe thật (hoặc bỏ) — T4.1d nếu muốn người dùng tự tạo.
- [ ] Commit (dùng `git add <file cụ thể>`, không `git add -A`).

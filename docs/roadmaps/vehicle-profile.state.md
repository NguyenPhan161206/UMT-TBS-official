# vehicle-profile — Orchestration State (T4.1a + T4.1b)
Roadmap: docs/roadmaps/vehicle-profile.roadmap.json
Roadmap tiếp theo (T4.1c): docs/roadmaps/vehicle-profile-override.roadmap.json — chỉ bắt đầu khi step 8 và 15 ở đây DONE.
Updated: 2026-10-05

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Build: GLOB nguồn ui_dashboard/vehicle_profile | DONE (sửa lại ở step 9) | host ctest 6/6; pio SUCCESS flash 1.311.361 B | GLOB chỉ còn ở host_sim; xem Deviations #1 |
| 2 | vehicle_profile: id + trục bánh + validate + test | DONE | vehicle_layout_tests 46/46; 2 đột biến FAIL rồi hoàn lại | |
| 3 | vehicle_layout: trục bánh mm→px + test | DONE | vehicle_layout_tests 53/53; 2 đột biến FAIL | |
| 4 | ui_dashboard_truck: vẽ bánh theo trục trong layout | DONE | grep 0; ctest 6/6; ảnh sim | bánh trước 59→60 px (làm tròn), bánh sau 272 px giữ nguyên |
| 5 | vehicle_profile: registry 3 hồ sơ + set_active + test | DONE | vehicle_layout_tests 99/99; 2 đột biến FAIL | |
| 6 | vehicle_settings: codec blob + resolve | DONE | gcc -std=c11 -Wall -Wextra -Werror OK | |
| 7 | Test host cho vehicle_settings codec | DONE | vehicle_settings_tests 82/82; bỏ CRC → 41 FAIL | |
| 8 | vehicle_settings: init/select qua store ops + test | DONE | vehicle_settings_tests 113/113; 2 đột biến FAIL | init trả enum (Deviations #2) |
| 9 | Component vehicle_store (adapter NVS) | DONE | pio SUCCESS (flash 1.311.641 B); có `vehicle_store_nvs.c.o`, `vehicle_settings.c.o`; arch_guard OK | |
| 10 | main.c: nvs_flash_init sớm + vehicle_settings_init | DONE (build) / CHƯA kiểm chứng phần cứng | pio SUCCESS (flash 1.313.785 B) | KHÔNG có board cắm → chưa xem log boot `Vehicle profile: id=1` |
| 11 | host_sim: --profile <id> | DONE | `--profile 2/3` chạy, `99`/`0` thoát mã 2; ảnh sim | |
| 12 | Tách mute sang ui_dashboard_actions.c | DONE | ctest pass; ui_dashboard.c 370 dòng (mục tiêu ≤360 chưa đạt, vẫn dư 30 so với 400); firmware build cuối SUCCESS | |
| 13 | Dựng lại canvas khi đổi hồ sơ (rebuild_vehicle) | DONE (host) | `--stress-profiles 120` (ctest `umt_dash_sim_profile_stress`): widget/hồ sơ không trôi, heap delta ≤ 72 B; bỏ `lv_obj_clean` → segfault 139 | API công khai `ui_dashboard_rebuild_vehicle()` trong ui_dashboard.h (sai khác so với plan) |
| 14 | Trang SETUP chọn hồ sơ | DONE (host) | ảnh sim: 3 hồ sơ, hàng đang dùng tô nổi | |
| 15 | Nối tab SETUP | DONE (host) / CHƯA kiểm chứng NVS trên board | ảnh sim: 3 tab không đè nhãn ESP-NOW/WiFi/MQTT; chọn "Small truck" → xe đổi, cung/marker đúng màu | cần board để thử reset giữ hồ sơ |
| 16 | EX8: áp số đo hãng công bố (dài 7370, rộng 2028, cơ sở 3850) | DONE (host) | vehicle_layout_tests 99/99 | cabin/phần nhô trước/6 cảm biến vẫn PLACEHOLDER |
| 17 | Log + CHECKLIST + kiểm tra toàn bộ | DONE | pio SUCCESS (flash 1.317.893 B, RAM 173.420 B); ctest 10/10; arch_guard OK; scan_secrets OK; pytest guard 30 passed | log `WAVESHARE_SCREEN_VEHICLE_PROFILE_LOG.md`; CHECKLIST T4.1a–c đều 🟡 |
| 18 | Số đo cabin, phần nhô trước, vị trí/góc 6 cảm biến | BLOCKED (hoãn) | — | 2026-10-05: người dùng chưa có xe, số đo chỉ ước lượng → hoãn, ưu tiên tính năng chính; giữ placeholder, không chặn việc khác |

Critical path: 2 → 3 → 4 (xong T4.1a phần cấu trúc); 2 → 5 → 6 → 7 → 8 → 9 → 10; 5 → 16; 8 → 11; 1 → 12; {4, 12} → 13; {8, 13} → 14; {12, 13, 14} → 15; {15, 16} → 17. Có thể chạy song song: nhánh 2–4, nhánh 1+12, nhánh 6–10.

## Số đo EX8 (cập nhật 2026-10-05)
Nguồn: ảnh bảng "Thông số kỹ thuật xe tải Hyundai New Mighty EX8" do người dùng gửi (chưa rõ trang gốc).

| Có trong bảng hãng | Giá trị | Dùng cho |
|---|---|---|
| Kích thước tổng thể D×R×C | 7370 × 2028 × 2310 mm | `length_mm`=7370, `width_mm`=2028 (cao 2310 không dùng) |
| Chiều dài cơ sở | 3850 mm | `wheelbase_mm`=3850 |
| Vết bánh trước/sau | 1750 / 1680 mm | chưa dùng (UI vẽ bánh ở mép thân) |
| Khoảng sáng gầm, bán kính quay, tải trọng, số chỗ | 220 mm, 7,5 m, 8 tấn, 3 chỗ | không dùng |
| Kích thước thùng hàng | "—/-" (không có) | — |

**Còn thiếu (bảng hãng không có, phải đo trên xe thật — step 18):** chiều dài cabin; khoảng cách đầu xe → trục trước; vị trí (x,y) và hướng búp của 6 cảm biến đang lắp.
**Mâu thuẫn tài liệu cần người dùng xác nhận:** `docs/INSTALLATION_SENSOR_NODE.md` §4 và `docs/HARDWARE_INSTALLATION.md` §5 ghi 4 cảm biến bên "nghiêng ngoài ~30°", trong khi hồ sơ placeholder đặt thẳng ngang (180°/0°). Cần biết 30° là so với trục dọc xe hay phương ngang trước khi đổi sang góc LVGL.
Lưu ý: chiều dài tổng 7370 có thể phụ thuộc loại thùng; cần xác nhận xe thật.

## Baseline (đã có trước roadmap này, commit 9bba9c2)
T4.1a đã làm MỘT PHẦN: component `vehicle_profile` có `vehicle_sensor_pose_t`, `vehicle_profile_t` (name, length_mm, width_mm, cab_length_mm, sensors[6]),
`vehicle_profile_active()`, `vehicle_profile_validate()`, hồ sơ EX8 với số PLACEHOLDER (8000 × 2500, cabin 2000, cảm biến ở mép thân).
Còn thiếu so với T4.1a đầy đủ: vị trí trục bánh vẫn là hằng thẩm mỹ 70%/80% trong `ui_dashboard_truck.c` (step 2–4) và số đo EX8 thật (step 16 cho dài/rộng/cơ sở từ bảng hãng; step 18 cho cabin, phần nhô trước, cảm biến).

## Contracts established
(Hiện có, nguyên văn từ code ngày 2026-10-05.)
- `typedef struct { int16_t x_mm, y_mm, angle_deg; } vehicle_sensor_pose_t;` — gốc = tâm thân xe, x phải +, y đầu xe +, góc theo quy ước LVGL (0 phải, 90 xuống/đuôi, 180 trái, 270 lên/đầu).
- `vehicle_profile_t { const char *name; uint16_t length_mm, width_mm, cab_length_mm; vehicle_sensor_pose_t sensors[ESPNOW_SENSOR_SLOT_COUNT]; }` — `components/vehicle_profile/include/vehicle_profile.h`.
- `const vehicle_profile_t *vehicle_profile_active(void)`, `bool vehicle_profile_validate(const vehicle_profile_t *p)`.
- `vehicle_layout_t`, `bool vehicle_layout_compute(p, canvas_w, canvas_h, margin_px, out)`, `bool vehicle_layout_marker(L, slot, dist_cm, out)` — `.../include/vehicle_layout.h`.
- `build_truck_body(canvas, L)` (ui_dashboard_truck.c); `markers_build/marker_update/marker_hide` (ui_dashboard_marker.c).
- Wire slot `espnow_slot_t`: FRONT=0, REAR=1, LEFT_FRONT=2, LEFT_REAR=3, RIGHT_FRONT=4, RIGHT_REAR=5.

### Hợp đồng dự kiến (chốt khi step tương ứng DONE, sửa lại cho khớp code thật)
- Step 2: `vehicle_profile_t` thêm `uint8_t id; uint16_t front_axle_mm, wheelbase_mm;` (`front_axle_mm` tính từ đầu xe; trục sau = `front_axle_mm + wheelbase_mm`).
- Step 3: `vehicle_layout_t` thêm `int16_t front_axle_y, rear_axle_y;` (px tuyệt đối trong canvas).
- Step 5: `size_t vehicle_profile_count(void); const vehicle_profile_t *vehicle_profile_get(size_t); const vehicle_profile_t *vehicle_profile_find(uint8_t id); bool vehicle_profile_set_active(const vehicle_profile_t *)`.
- Step 6/8: `vehicle_settings_t`, `vehicle_settings_{defaults,crc16,encode,decode,resolve}`, `vehicle_store_ops_t {load,save,erase}`, `vehicle_settings_{init,current,select}` — `components/vehicle_profile/include/vehicle_settings.h`.
- Step 9: `const vehicle_store_ops_t *vehicle_store_nvs_ops(void)` — `components/vehicle_store/`.
- Step 12: `void display_cache_reset(void)` (ui_dashboard_private.h).
- Step 13: `void ui_dashboard_rebuild_vehicle(void)` (ui_dashboard_private.h).
- Step 14: `lv_obj_t *build_setup_page(lv_obj_t *parent); void setup_page_refresh(void)`.

## Quyết định kiến trúc đã chốt (không hỏi lại)
1. **Hai hồ sơ thêm là placeholder có tên chung** ("Small truck (placeholder)" id=2, "Large truck (placeholder)" id=3), không bịa tên xe thật. Tên **ASCII** vì font LVGL của UI không có dấu tiếng Việt.
2. **`id` là khoá ổn định** lưu NVS (EX8=1), không dùng chỉ số mảng, để thêm/đổi thứ tự hồ sơ không làm hỏng dữ liệu đã lưu.
3. **Lưu NVS = 1 blob versioned + CRC16** (43 byte, key `vehicle/settings`), đã có chỗ cho override (T4.1c) ngay từ version 1 để khỏi đổi schema. Phân vùng nvs 0x6000 (24 KB) đủ.
4. **Lõi `vehicle_profile` là C thuần, host-test được; lưu trữ qua `vehicle_store_ops_t` (dependency injection)**: firmware dùng adapter NVS, host/test dùng store giả. Không để lõi gọi NVS trực tiếp.
5. **NVS phải init sớm trong `app_main`**: `ui_dashboard_init()` (main.c ~289) chạy TRƯỚC `coreiot_client_init()` (main.c ~305), nơi duy nhất gọi `nvs_flash_init()`. Step 10 thêm một lần init ở đầu `app_main` (gọi lần hai trong coreiot_client phải vô hại — kiểm chứng bằng log boot). Không sửa coreiot_client.c.
6. **Đổi hồ sơ lúc chạy = dựng lại canvas tại chỗ** (`ui_dashboard_rebuild_vehicle`), không reboot. Phương án dự phòng nếu step 13 không đạt DoD sau 2 lần thử: lưu NVS rồi `esp_restart()`.
7. **Đổi hồ sơ gốc xoá toàn bộ override** (quy ước cho T4.1c).
8. **GLOB nguồn (step 1)** để các step thêm file .c mới không phải sửa CMake và giữ ≤ 3 file/step.
9. **R7**: `ui_dashboard.c` đang 399/400 dòng → step 12 tách ra trước khi thêm trang SETUP.
10. **Hồ sơ lưu `wheelbase_mm` (đại lượng hãng công bố) + `front_axle_mm`, không lưu vị trí trục sau**: số hãng nhập thẳng được, chỉ phần nhô trước còn phải đo (quyết định 2026-10-05, sau khi nhận bảng thông số EX8).
11. **Step 2–4 giữ EX8 tạm (front 1400, cơ sở 5000) để hình không đổi** và chứng minh refactor sạch; số hãng vào riêng ở step 16. Test không được ghim số cụ thể nào của EX8.
12. **T4.1d (người dùng tự tạo hồ sơ) KHÔNG nằm trong roadmap này** — để dành giai đoạn sau theo yêu cầu. Trường `version` của blob là chỗ nâng cấp.

## Rủi ro đã nhận diện
- Step 13 (xoá/dựng widget khi LVGL còn anim): cung đang nhấp nháy phải `lv_anim_delete` trước `lv_obj_clean`; cache chống vẽ lại phải reset, nếu không cung kẹt màu "không dữ liệu" khi khoảng cách không đổi.
- Step 15: header 3 tab có thể đè nhãn ESP-NOW/WiFi/MQTT (đã có lỗi chồng chữ ESP-NOW từ trước) — phải xác nhận bằng ảnh chụp sim.
- Step 1/9: PlatformIO có thể không nhận file/component mới nếu CMake không chạy lại.
- Step 10/15 cần board thật; không có board thì phải ghi "chưa kiểm chứng trên phần cứng", không báo PASS.
- Quy tắc từ memory: khi review kết quả của Antigravity thì tự chạy lại build/test/sim, đối chiếu `git status` với danh sách Target files, không tin log.

## Deviations from plan
1. **Step 1 (GLOB) bị hoàn tác một phần ở step 9.** ESP-IDF đánh giá CMakeLists của component ở chế độ script, nơi `file(GLOB … CONFIGURE_DEPENDS)` bị cấm
   (`CONFIGURE_DEPENDS is invalid for script and find package modes`, làm `pio run` fail khi cấu hình lại); còn GLOB thường thì PlatformIO không tự thấy file mới.
   Quyết định 8 cũ không còn đúng: **GLOB chỉ ở `host_sim/CMakeLists.txt`**; `ui_dashboard/CMakeLists.txt` và `vehicle_profile/CMakeLists.txt` trở về danh sách tường minh
   (thêm `vehicle_settings.c`), nên mỗi file .c mới của component phải thêm vào CMakeLists. Bước 1 "xong" trên host nhưng phần component là vô ích; không có hại, chỉ tốn thời gian.
2. **`vehicle_settings_init` trả `vehicle_settings_init_result_t`** (LOADED / DEFAULTS_NO_DATA / DEFAULTS_INVALID) thay vì `void`, và thêm `bool vehicle_settings_last_save_ok(void)`:
   lõi C thuần không có hàm log; để `main.c` tự log kết quả và UI báo "chưa lưu được" khi NVS lỗi ghi. Step 10 log theo enum này.
3. **PlatformIO không tự cấu hình lại khi thêm file/component mới**: sau khi thêm `vehicle_settings.c` và component `vehicle_store`, `pio run` báo SUCCESS nhưng KHÔNG biên dịch chúng
   (không có `vehicle_settings.c.o`, không có thư mục `vehicle_store`). Phải `touch firmware/waveshare-screen/CMakeLists.txt` để ép cấu hình lại. DoD của step 9 vì vậy kiểm bằng sự tồn tại của file .o.
4. Test của step 3 thêm 1 trường hợp (profile giả dời trục) ngoài mô tả; vẫn trong target_files.
5. **LVGL heap của firmware chỉ 64 KB và bị cạn** (`CONFIG_LV_MEM_SIZE_KILOBYTES=64`, bộ cấp phát dựng sẵn, không mở rộng). Khi thêm trang SETUP + bộ chỉnh cảm biến, sim treo
   (LV_ASSERT_MALLOC = vòng lặp vô hạn); đo được UI dùng 62.160 B. Đã nâng lên **128 KB** trong `sdkconfig.defaults` (tracked) và `sdkconfig.yolo_uno` (gitignored, sinh ra) — file sdkconfig
   vốn nằm ngoài phạm vi, sửa vì có số đo. `host_sim/lv_conf.h` đặt `LV_MEM_SIZE` 128 KB cho khớp; test stress kiểm dư địa heap ≥ 30%; test sim có `TIMEOUT 60`.
   **Chưa kiểm chứng trên board**: RAM tĩnh tăng thêm 64 KB (trước: 107.740 B / 327.680 B).
6. **GLOB phải bỏ ở component firmware** (xem #1) và **thêm file .c vào component phải `touch firmware/waveshare-screen/CMakeLists.txt`** (xem #3).
7. host_sim bổ sung (ngoài plan, vẫn trong host_sim): chuột (`lv_sdl_mouse_create`), `--stress-profiles N`, và kịch bản UI tự động `--click X,Y@ms` / `--snapshot FILE.bmp@ms`
   (chụp màn hình ngay trong sim, vì điều khiển chuột thật của Windows không đáng tin: cửa sổ Chrome của người dùng đè lên sim). Script PowerShell tạm nằm ngoài repo.
8. Bước 1 → quyết định ban đầu "GLOB ESP-IDF" sai; **hậu quả**: mất thêm 2 lần build firmware dài (~12 phút/lần).

## TRẠNG THÁI CUỐI PHIÊN (2026-10-05): roadmap này và roadmap T4.1c đã làm hết phần làm được trên máy không có board
Còn lại (không làm được ở phiên này): step 18 (BLOCKED, cần số đo trên xe thật) và mọi nghiệm thu trên phần cứng (log boot, giữ hồ sơ/ghi đè sau reset, RAM sau khi pool LVGL +64 KB).
Mục "Tiến độ hiện tại" dưới đây là bản ghi giữa phiên, đã lỗi thời.

## Tiến độ hiện tại (ghi lúc 2026-10-05, giữa phiên — đã lỗi thời)
- Đã xong và kiểm chứng trên host: steps 1–16 (xem bảng). T4.1c (roadmap `vehicle-profile-override`): API ghi đè `vehicle_settings_set_override/clear_override/clear_all_overrides/override_mask`
  đã cài và có test (`vehicle_settings_tests` 150/150, 3 đột biến bị bắt); widget `ui_dashboard_sensor_edit.c` đã viết và gắn vào trang SETUP (2 cột), **chưa kiểm chứng giao diện bằng ảnh**.
- **Còn lại**: (1) chụp ảnh sim trang SETUP + thử chỉnh cảm biến bằng `--click/--snapshot`; (2) `touch firmware/waveshare-screen/CMakeLists.txt` rồi `pio run -e yolo_uno` (build NỀN, ~12 phút, KHÔNG sửa
  nguồn firmware khi đang chạy) — gồm CMake mới (`ui_dashboard_actions.c`, `ui_dashboard_setup.c`, `ui_dashboard_sensor_edit.c`), `sdkconfig` 128 KB; (3) kiểm tra flash/RAM; (4) 3 guard + ctest;
  (5) viết `docs/logs/WAVESHARE_SCREEN_VEHICLE_PROFILE_LOG.md` + cập nhật `docs/CHECKLIST.md` (T4.1a 🟡, T4.1b/T4.1c chỉ ✅ phần host, ghi rõ chưa kiểm chứng phần cứng); (6) cập nhật ledger T4.1c.
- Chưa commit gì (làm trên nhánh `khoa`, working tree). Các file mới chưa add: xem `git status`.

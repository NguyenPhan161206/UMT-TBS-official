# WAVESHARE_SCREEN_BLACK_SCREEN_RAM_FIX_LOG — Màn đen sau khi flash (thiếu RAM nội) + crash khi lưu NVS từ UI

Ngày: 2026-10-05  
Nhánh: `khoa` (working tree, **chưa commit**)  
Liên quan: `WAVESHARE_SCREEN_VEHICLE_PROFILE_LOG.md` (T4.1b/T4.1c thêm tab SETUP, chỉnh cảm biến, lưu NVS)

## 1. Triệu chứng & nguyên nhân (đọc từ log serial COM9, không suy đoán)
Sau khi flash bản có tab SETUP + chỉnh cảm biến, màn 7" đen hoàn toàn. Log boot cho thấy board **reboot liên tục**:
```
E (1019) lcd.rgb: lcd_rgb_panel_alloc_frame_buffers(223): no mem for bounce buffer
ESP_ERROR_CHECK failed: esp_err_t 0x101 (ESP_ERR_NO_MEM) ... waveshare_rgb_lcd_port.c line 220
abort() ... Rebooting...
```
- `sdkconfig.defaults` đã đặt `CONFIG_LV_MEM_SIZE_KILOBYTES=128` (vì UI mới dùng ~62 KB, pool 64 KB bị cạn trên host_sim).
- Pool LVGL builtin là **mảng tĩnh trong `.dram0.bss` (RAM nội)** → `.dram0.bss` tăng lên 150 KB, heap nội lúc boot chỉ còn 146 KiB ở vùng lớn nhất.
- Bounce buffer RGB cần `2 × 800×40 px × 2 B = 125 KB` RAM nội có DMA → `esp_lcd_new_rgb_panel()` trả `ESP_ERR_NO_MEM` → `ESP_ERROR_CHECK` abort trước khi màn kịp sáng.

**Lỗi thứ hai (tiềm ẩn, sẽ gặp ngay khi màn sáng):** nút **Apply** (chỉnh cảm biến) và chọn hồ sơ ở SETUP gọi
`vehicle_settings_*` → `nvs_set_blob` **trong event LVGL**. Task LVGL có stack ở PSRAM (`adapter_config.stack_in_psram = true`),
mà ESP-IDF 6.0.1 `spi_flash/cache_utils.c` có `assert(esp_task_stack_is_sane_cache_disabled())` (stack phải ở DRAM nội khi tắt cache) → abort.

## 2. Cách sửa
| File | Thay đổi |
|---|---|
| `firmware/waveshare-screen/sdkconfig.defaults` | `LV_MEM_SIZE_KILOBYTES` 128 → **64** (giữ đúng ngân sách RAM nội đã chạy ổn trước đó); thêm `LV_MEM_POOL_EXPAND_SIZE_KILOBYTES=64` (để TLSF chấp nhận pool phụ). Ghi chú lý do ngay trong file. |
| `firmware/waveshare-screen/src/main.c` | `lvgl_add_psram_pool()`: cấp 64 KB từ PSRAM và `lv_mem_add_pool()` ngay sau `esp_lv_adapter_init()` (trước khi task LVGL chạy). Tổng heap LVGL vẫn 128 KB. Thêm log `Internal heap after init` cuối `app_main` để theo dõi biên RAM nội. |
| `firmware/waveshare-screen/components/vehicle_store/vehicle_store_nvs.c` | load/save/erase đi qua `run_on_internal_stack()`: nếu SP của task gọi không ở DRAM → chạy trên task tạm `vstore_io` (stack nội 4 KB) và chờ bằng semaphore riêng (KHÔNG dùng task notification vì `esp_lv_adapter` dùng notify cho task LVGL). Gọi từ task có stack nội (vd `app_main`) thì chạy trực tiếp. |

`sdkconfig.yolo_uno` (gitignored, sinh ra) cũng được sửa tay 2 dòng tương ứng vì giá trị trong defaults không ghi đè sdkconfig đã có.
**Máy khác đã từng build với 128 KB phải xoá `firmware/waveshare-screen/sdkconfig.yolo_uno` để sinh lại.**

## 3. Kiểm thử (R10 — flash-and-observe)
- `pio run -e yolo_uno`: SUCCESS. RAM tĩnh 107 292 B (32.7%) — trước khi sửa `.dram0.data + .dram0.bss` ≈ 172 KB.
- Flash COM9, reset, đọc log 20 s: **1 lần reset duy nhất (POWERON), không còn vòng reboot**.
```
I (1019) bsp_lcd_port: Initialize RGB LCD panel
I (1449) collision_dashboard: LVGL heap: 64 KB internal + 64 KB PSRAM
I (2059) ui_dashboard: Collision dashboard UI initialized
I (2379) collision_dashboard: Internal heap after init: free 71 KB, largest block 31 KB
```
- Wi-Fi báo `Reason code: 201` (không thấy AP hotspot lúc test) và `ESP-NOW link DOWN` (sensor-node không phát) — không liên quan tới bản sửa.
- **Chưa kiểm** (cần người bấm màn hình): SETUP → chỉnh cảm biến → Apply phải hiện "Applied and saved", không reboot; reset board → log boot phải ghi
  `Vehicle profile: ... (loaded from NVS)`.

## 4. Tuân thủ CONSTITUTION
- R8: `sdkconfig.defaults` được phép track (`.gitignore` có `!**/sdkconfig.defaults`; CI R8 chỉ chặn `sdkconfig`, `sdkconfig.old`, `keys.json`). `sdkconfig.yolo_uno` vẫn gitignored.
- R7: `src/main.c` 358 dòng, `vehicle_store_nvs.c` 184 dòng (≤ 400).
- Ngoài phạm vi, chưa sửa: `main.c` còn con trỏ ảo "để debug lỗi lệch cảm ứng" (`lv_indev_set_cursor`) — nếu áp R5 chặt thì nên gỡ/đưa sau macro.

# WAVESHARE_SCREEN_SCAFFOLD_LOG — B4 waveshare-screen (V2)

> Ngày: 2026-09-02 · Component: `firmware/waveshare-screen/` · Bước roadmap: **B4** ✅

## Mục tiêu
Dựng scaffold đầy đủ cho board waveshare-screen (màn hình 7" RGB LVGL v9):
sensor_model (dùng 100% shared contract R2/R3), ui_dashboard tách file ≤400 dòng (R7),
coreiot_client không hardcode secret (R1), main.c LVGL + MQTT path.
ESP-NOW receiver (đường chính) là B5 — scaffold này chưa bật.

## File đã tạo/sửa
- `firmware/waveshare-screen/platformio.ini` — env `yolo_uno`, `framework = espidf` (IDF ≥5.5).
- `firmware/waveshare-screen/sdkconfig.defaults` — 8MB flash QIO, PSRAM octal, LVGL 9.1, KHÔNG demo (R5).
- `firmware/waveshare-screen/partitions.csv` — nvs(24K) / phy(4K) / factory app(4MB).
- `firmware/waveshare-screen/CMakeLists.txt`, `src/CMakeLists.txt`,
  `src/idf_component.yml` (lvgl ^9.1.0, esp_lvgl_adapter ^0.5.2, esp_lcd_touch_gt911, mqtt, cjson).
- `firmware/waveshare-screen/boards/yolo_uno.json` — board custom (copy từ cựu repo).
- `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.{c,h}` — port RGB 800x480 + GT911 (195/84 dòng).
- `firmware/waveshare-screen/src/main.c` — init BSP + LVGL adapter, `ui_dashboard_init()`,
  wiring CoreIoT (MQTT path) qua `coreiot_client_*` callbacks (thread-safe lock LVGL).
- `firmware/waveshare-screen/components/sensor_model/` — thread-safe state container;
  `sensor_id_t` = `espnow_slot_t`, `SENSOR_MODEL_COUNT` = `ESPNOW_SENSOR_SLOT_COUNT`;
  `sensor_model_classify()` dùng `SENSOR_CAUTION_CM/SENSOR_DANGER_CM` (R3, không define trùng).
- `firmware/waveshare-screen/components/ui_dashboard/`
  - `include/ui_dashboard.h` (public API),
    `ui_dashboard_theme.h` (design tokens + `k_sensor_labels[]` theo wire slot + `zone_color()`),
    `ui_dashboard_private.h` (state + internals).
  - `ui_dashboard.c` (330) — API, hazard evaluation, mute button.
  - `ui_dashboard_layout.c` (330) — header, sidebars, 6-sensor "No-Zone" arcs, tabs.
  - `ui_dashboard_system.c` (162) — SYSTEM page (broker/token hiển thị có mask_secret).
- `firmware/waveshare-screen/components/coreiot_client/` — WiFi STA + esp-mqtt CoreIoT;
  **R1**: credential chỉ đọc từ `credentials.h` (AUTO-GENERATED, gitignored) — không literal token nào.

## Kết quả kiểm thử (DoD B4)
```bash
# Build
cd firmware/waveshare-screen
~/.venv-platformio/bin/pio run -e yolo_uno
# → [SUCCESS] Took 89.14s
# RAM:   12.8% (41932 / 327680 bytes)
# Flash: 31.2% (1289029 / 4128768 bytes) — partition app 4MB, size-gate OK

# R1 — quét secret (exit 0)
/usr/bin/python3 tools/guard/scan_secrets.py
# → SECRET-SCAN OK: no secret patterns found.

# R2 — espnow_sensor_msg_t định nghĩa đúng 1 nơi (firmware/shared/)
grep -rn "} espnow_sensor_msg_t;" firmware/ --include="*.h" --include="*.c" --include="*.cpp"
# → firmware/shared/espnow_protocol.h:58

# R7 — mọi source tự viết ≤ 400 dòng (src/, components/, shared/, sensor-node src)
find firmware/waveshare-screen/src firmware/waveshare-screen/components firmware/shared \
     firmware/sensor-node/src firmware/sensor-node/include -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" \) \
     | xargs wc -l | awk '$1 > 400'

# Git hygiene — secret không bị stage
git check-ignore config/keys.json firmware/waveshare-screen/components/coreiot_client/include/credentials.h
# → cả 2 đều bị ignore (exit 0)
```

## Hướng dẫn vận hành / demo
1. **Bắt buộc trước khi flash**: điền token MỚI vào `config/keys.json`
   (B0b — do chủ tài khoản CoreIoT tự sinh, KHÔNG tái dùng token cũ), sau đó:
   ```bash
   /usr/bin/python3 tools/guard/gen_credentials.py \
     --out firmware/waveshare-screen/components/coreiot_client/include/credentials.h
   ```
   >
   > ⚠️ Hiện tại `config/keys.json` đang chứa **giá trị dummy** (`test-build-*`) dùng để build thử B4.
   > Nhất định thay bằng token thật trước khi nạp firmware thật.
2. Flash + monitor:
   ```bash
   cd firmware/waveshare-screen
   ~/.venv-platformio/bin/pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1
   ~/.venv-platformio/bin/pio device monitor -p /dev/ttyACM1 -b 115200
   ```
3. Quan sát: màn hình hiện dashboard (header, 6 arc No-Zone, sidebar).
   Với MQTT path, màn hình nhận `distances`/`relay`/`buzzer` từ rule-chain → cập nhật UI.
   ESP-NOW path (B5) sẽ là đường chính.

## Ghi chú / lỗi gặp phải
- `esp_flash.h` thuộc component **`spi_flash`** (không phải `esp_flash`) trong IDF 6.0.1 —
  REQUIRES đúng tên `spi_flash`.
- PlatformIO không tự reconfigure khi chỉ sửa CMakeLists component → cần `pio run -t clean` trước.
- `mute_btn_cb` cần là cross-file (bỏ `static`) + khai báo ở `ui_dashboard_private.h`.
- R7: `ui_dashboard_layout.c` ban đầu 487 dòng → tách `ui_dashboard_system.c` (162 dòng), còn 330 dòng.

## Đề xuất bước tiếp theo
- **B5**: ESP-NOW receiver trên waveshare-screen (dùng `firmware/shared/espnow_protocol.h`,
  `espnow_sensor_msg_t`, slot `ESPNOW_SLOT_*`) → `ui_dashboard_update_sensor()`; grep define trùng == 1;
  flash-and-observe 2 board.
- Sau đó: B7 (CI) + B9 (nghiệm thu CoreIoT với token thật).
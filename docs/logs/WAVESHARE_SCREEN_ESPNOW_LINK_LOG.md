# WAVESHARE_SCREEN_ESPNOW_LINK_LOG — B5 ESP-NOW receiver (V2)

> Ngày: 2026-09-02 · Component: `firmware/waveshare-screen/` · Bước roadmap: **B5** ✅ (build/wiring; flash-and-observe chờ board)

## Mục tiêu
Nối **đường chính ESP-NOW** từ sensor-node → waveshare-screen dùng **duy nhất**
`firmware/shared/espnow_protocol.h` (R2). Cựu repo định nghĩa trùng struct + channel
ngay trong main.c — V2 tách thành component `espnow_receiver` (protocol-only) và
không còn `#define ESPNOW_*` nào ngoài shared (grep == 0).

## File tạo/sửa
- **MỚI** `firmware/waveshare-screen/components/espnow_receiver/`
  - `include/espnow_receiver.h` — API: `espnow_receiver_init(cb)`,
    `espnow_receiver_is_linked()`, `espnow_receiver_last_rx_ms(slot)`.
  - `espnow_receiver.c` — init esp-now SAU `esp_wifi_start()`; recv cb validate
    kích thước gói == `sizeof(espnow_sensor_msg_t)`; ghi timestamp per-slot
    (chỉ khi `valid[i]==1`); `esp_now_init` trả `ESP_ERR_ESPNOW_EXIST` được coi
    như đã init nơi khác. **Component không biết LVGL/sensor_model**.
  - `CMakeLists.txt` — `REQUIRES esp_wifi esp_timer`, include `../../../shared`.
    (Trong IDF 6.0.1, esp-now nằm trong component **`esp_wifi`**, không còn component `esp_now`.)
- **SỬA** `firmware/waveshare-screen/src/main.c`
  - `on_espnow_rx()` — chạy trên WiFi task, lấy LVGL lock có timeout; `valid[i]` →
    `ui_dashboard_update_sensor(i, dist)`; `valid[i]==0` → `ui_dashboard_clear_sensor(i)`.
  - `espnow_link_watchdog_cb()` — LVGL timer 500ms: badge LINKED theo
    `espnow_receiver_is_linked()`; slot nào quá `ESPNOW_LINK_TIMEOUT_MS` → clear.
  - Init: `espnow_receiver_init(on_espnow_rx)` SAU `coreiot_client_init()`.
- **SỬA** `firmware/waveshare-screen/src/CMakeLists.txt` — `REQUIRES espnow_receiver`.
- **SỬA** `components/ui_dashboard/` — thêm label header riêng
  `s_lbl_espnow_status` (right -140) để không đè badge MQTT:
  `ui_dashboard_private.h` (extern), `ui_dashboard.c` (def + `set_espnow_status`
  dùng label mới), `ui_dashboard_layout.c` (tạo label).

## Kết quả kiểm thử (DoD B5)
```bash
# Build waveshare-screen (đã clean để CMake nhận component mới)
cd firmware/waveshare-screen
~/.venv-platformio/bin/pio run -e yolo_uno
# → [SUCCESS]  RAM: 12.8% (42060 B) / Flash: 31.3% (1293513 B)

# R1 — secret
/usr/bin/python3 tools/guard/scan_secrets.py          # → SECRET-SCAN OK

# R2 — espnow_sensor_msg_t định nghĩa đúng 1 nơi (shared)
grep -rn "} espnow_sensor_msg_t;" firmware/ --include="*.h" --include="*.c" --include="*.cpp"
# → firmware/shared/espnow_protocol.h:58

# R2 — không define trùng channel/timeout/slot-count ngoài shared
grep -rn "#define ESPNOW_CHANNEL\|#define ESPNOW_LINK_TIMEOUT_MS\|#define ESPNOW_SEND_INTERVAL_MS\|#define ESPNOW_SENSOR_SLOT_COUNT" firmware/ --include="*.h" --include="*.c" --include="*.cpp" | grep -v "\.pio/" | grep -v "firmware/shared/"
# → (rỗng) ✅

# R7 — mọi source tự viết ≤ 400 dòng
find firmware/waveshare-screen/src firmware/waveshare-screen/components firmware/shared -type f \( -name "*.c" -o -name "*.h" \) | xargs wc -l | awk '$1 > 400'
# → (rỗng) ✅
```

## Hướng dẫn vận hành / demo (flash-and-observe — cần board)
1. **Bắt buộc**: `config/keys.json` phải có token/Wi-Fi **thật** (không phải dummy
   `test-build-*`), sinh lại credentials.h:
   ```bash
   /usr/bin/python3 tools/guard/gen_credentials.py \
     --out firmware/waveshare-screen/components/coreiot_client/include/credentials.h
   ```
2. Flash 2 board:
   ```bash
   cd firmware/sensor-node
   ~/.venv-platformio/bin/pio run -e yolo_uno -t upload --upload-port /dev/ttyACM0
   cd ../waveshare-screen
   ~/.venv-platformio/bin/pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1
   ~/.venv-platformio/bin/pio device monitor -p /dev/ttyACM1 -b 115200
   ```
3. Quan sát:
   - Header hiện `ESP-NOW: LINKED` (xanh) khi sensor-node gửi gói (mỗi 500ms);
     mất link > 1.5s → `ESP-NOW: NO LINK` (đỏ), slot quá hạn về `-- cm`/nodata.
   - Di chuyển vật thể trước cảm biến → arc/row đổi màu theo zone shared
     (>100 SAFE xanh, 30–100 CAUTION vàng, <30 DANGER đỏ + blink).
   - Nếu sensor-node báo `valid[i]=0` cho slot nào → slot đó về trạng thái nodata.

## Ghi chú kiến trúc
- **Channel**: Khi STA chưa kết nối AP, screen cố định `ESPNOW_CHANNEL` (=1) khớp
  sensor-node (cả 2 đều "STA trần" cho ESP-NOW). Khi bật Wi-Fi thật (B9), esp-now
  tự bám channel của AP → **2 board phải cùng AP** (hoặc đặt AP channel 1), nếu
  không ESP-NOW và MQTT không thể đồng thời đúng channel.
- **R6**: `espnow_receiver` được build + gọi từ main.c (không dead code).
- Manual `flash-and-observe` chưa thực hiện vì máy dev không có board (AGENTS.md);
  chuẩn bị sẵn hướng dẫn ở trên cho lần nghiệm thu phần cứng.

## Đề xuất bước tiếp theo
- **B7**: CI (GitHub Actions) build cả 2 firmware × 2 env + host tests + Gitleaks…
- **B9**: nghiệm thu CoreIoT với token thật; kiểm tra channel khi 2 board cùng AP.
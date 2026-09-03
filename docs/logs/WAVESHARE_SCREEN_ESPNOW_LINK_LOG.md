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

---

# BỔ SUNG 2026-09-03 — Fix NO LINK (broadcast) + MQTT up/down (debounce label)

## Mục tiêu
1. **ESP-NOW NO LINK** — sensor-node gửi unicast tới chính nó (sai peer MAC) → waveshare
   không nhận.
2. **Internet/MQTT hiển thị up/down liên tục** — 2 callback Wi-Fi/MQTT cùng overwrite 1
   state `is_connected` (set_iot_status), và broker/network drop MQTT định kỳ ~10s làm
   reconnect loop.

## File sửa
- `firmware/shared/espnow_protocol.h` — `ESPNOW_PEER_MAC` từ unicast
  `{0x64,0xe8,0x33,0x7c,0x3f,0xe0}` (MAC sensor-node — gửi tới chính nó) → **broadcast
  `{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}`** + comment giải thích. R2 single-source.
- `firmware/sensor-node/src/espnow_client.cpp` — broadcast không bắt buộc `add_peer`;
  `add_peer` fail chỉ cảnh báo (không dừng); `sendReading()` vẫn `esp_now_send(ESPNOW_PEER_MAC,..)`.
- `firmware/waveshare-screen/components/ui_dashboard/include/ui_dashboard.h` +
  `ui_dashboard.c` — **tách** `ui_dashboard_set_iot_status(conn, ip)` → 2 hàm riêng:
  `ui_dashboard_set_wifi_status(conn, ip)` (chỉ wifi/sys_wifi) và
  `ui_dashboard_set_mqtt_status(conn)` (chỉ mqtt/sys_mqtt). Không còn overwrite lẫn nhau.
- `firmware/waveshare-screen/src/main.c` — `on_wifi_status`→`set_wifi_status`,
  `on_mqtt_status`→`set_mqtt_status`.
- `firmware/waveshare-screen/components/coreiot_client/coreiot_client.c` — **debounce MQTT
  DOWN**: `MQTT_EVENT_CONNECTED` → báo UP ngay + huỷ timer; `MQTT_EVENT_DISCONNECTED` →
  chỉ báo DOWN nếu không reconnect được trong `MQTT_DOWN_DEBOUNCE_MS` (6s). Chống nhấp
  nháy UP/DOWN khi esp-mqtt auto-reconnect nhanh.

## Kết quả kiểm thử (DoD)
```bash
# Build 3 (waveshare + sensor-node × 2 env) — TẤT CẢ SUCCESS
cd firmware/waveshare-screen && ~/.venv-platformio/bin/pio run -e yolo_uno   # RAM 12.8% Flash 31.3%
cd firmware/sensor-node && ~/.venv-platformio/bin/pio run -e yolo_uno \
  && ~/.venv-platformio/bin/pio run -e yolo_uno_coreiot

# Host tests sensor-node: 10/10 PASSED (R10)
cd firmware/sensor-node && ~/.venv-platformio/bin/pio test -e native

# R1 secret
/usr/bin/python3 tools/guard/scan_secrets.py   # SECRET-SCAN OK
# R2 single-source
grep -rn "ESPNOW_PEER_MAC\[6\]" firmware/ --include=*.h → firmware/shared/espnow_protocol.h:37 (1 nơi)
# R7 ≤400 dòng: coreiot_client.c 231, ui_dashboard.c 335, main.c 221, espnow_client.cpp 45 ✅
```

## Flash waveshare + quan sát (2026-09-03)
- Flash waveshare (`/dev/ttyACM0`, MAC `ec:da:3b:51:77:94`) SUCCESS ×2 (bản tách label,
  rồi bản + debounce).
- Log boot waveshare:
  - `espnow_receiver: esp_wifi_set_channel(1) rc=-1 (bỏ qua...)` — OK vì AP "Bamos Coffee
    2G" **channel 1** khớp `ESPNOW_CHANNEL`.
  - `ESPNOW: espnow [version: 2.0] init` → `ESP-NOW receiver ready on channel 1`.
  - `coreiot_client: Wi-Fi Connected ... IP: 10.0.11.237` → `MQTT Connected to CoreIoT`.
- **MQTT drop định kỳ (phát hiện khi nghiệm thu):** sau `Published reboot telemetry`
  ~10.5s có `transport_read(): EOF · errno=128 (ENOTCONN)` → `MQTT Disconnected` → reconnect
  OK ~10s sau. **Chu kỳ lặp lại liên tục = bản chất mạng/broker** (mạng "Bamos Coffee 2G"
  qua NAT, hoặc CoreIoT broker đóng TCP): không phải bug firmware. Debounce 6s giúp UI
  không nhấp nháy DOWN trong từng cửa sổ reconnect → màn ổn định hơn.

## Trạng thái phần cứng (chặn nghiệm thu ESP-NOW end-to-end)
- **Sensor-node vẫn KHÔNG enumerate** trong kernel: `lsusb` chỉ thấy 1 thiết bị Espressif
  (`Bus 001 Device 029`, waveshare `/dev/ttyACM0`). `journalctl -k` không ghi sự kiện USB
  mới nào sau `15:41:49` (lúc `usb 3-2` — sensor-node cũ — disconnect, device 99).
- → Chưa flash/verify sensor-node và chưa xác nhận **ESP-NOW: LINKED** trên màn (cần cả 2
  board). Chờ người dùng cắm lại sensor-node bằng cáp USB-data/đầu nối ok để enum được.

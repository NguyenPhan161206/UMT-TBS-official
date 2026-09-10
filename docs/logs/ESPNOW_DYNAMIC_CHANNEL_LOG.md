# ESPNOW_DYNAMIC_CHANNEL_LOG — sync peer channel theo home WiFi

Ngày: 2026-09-10
Nhánh: `nguyen` (chưa merge main)
Roadmap: `docs/roadmaps/espnow-dynamic-channel.roadmap.json` (2 steps, DONE)

## Mục tiêu
Sửa lỗi ESP-NOW `Peer channel is not equal to the home channel, send fail!`.
Nguyên nhân: cả 2 board kết nối AP (iPhone hotspot) chạy **STA mode**, AP tự đổi
kênh qua CSA (log `sta rx csa 1->11`). ESP32-S3 single-radio bắt buộc bám home
channel thật, nhưng sensor-node cứng `peerInfo.channel = ESPNOW_CHANNEL 6` →
khi AP dạt qual 11, esp_now_send fail.

## File đã sửa
- `firmware/sensor-node/src/espnow_client.cpp` — thêm `currentHomeChannel()`
  (`esp_wifi_get_channel`, fallback `ESPNOW_CHANNEL`) + `syncPeerChannelToHome()`
  (`esp_now_get_peer` + `esp_now_mod_peer`), gọi trước mỗi `esp_now_send()`.
  `begin()` vẫn ghim radio ở `ESPNOW_CHANNEL` cho trường hợp chưa nối AP.
- `firmware/shared/espnow_protocol.h` — doc rõ `ESPNOW_CHANNEL` = DEFAULT/
  FALLBACK khi chưa associate AP; khi nối AP thì theo kênh AP, không fix 6.
- `firmware/waveshare-screen/components/espnow_receiver/espnow_receiver.c` —
  log init đổi thành "default/fallback channel" (không đổi hành vi: STA connected
  thì driver tự bám AP channel nên RX luôn đúng channel).

## Kết quả kiểm thử (2026-09-10)
- Build: `pio run -e yolo_uno` + `yolo_uno_coreiot` (sensor-node) SUCCESS;
  waveshare `yolo_uno` SUCCESS.
- Guard: `pytest test_guard.py` 29 passed; arch_guard OK; scan_secrets OK.
- **Nghiệm thu thực tế (flash 2 board, 2026-09-10)**: sensor-node `yolo_uno_coreiot`
  → `/dev/ttyACM1` (Espressif), waveshare `yolo_uno` → `/dev/ttyACM0` (QinHeng).
  Loog sensor-node sau reset: `[ESPNOW] Peer channel sync 1 (WIFI home)` — peer tự
  bám kênh AP (không cố định 6); `Send FAILED count: 0` (trước fix: fail mỗi 500ms
  khi AP đổi channel). Waveshare nhận `ESP-NOW frame rssi=-31..-42 dBm`, 1 gói/500ms;
  MQTT CoreIoT connected song song.

## Hướng dẫn vận hành / demo
```bash
export PATH="/home/binhnguyen/.venv-pio/bin:$PATH"
cd firmware/sensor-node && pio run -e yolo_uno_coreiot -t upload --upload-port <PORT_SN>
cd ../waveshare-screen && pio run -e yolo_uno -t upload --upload-port <PORT_WS>
# Monitor sensor-node: chờ "Peer channel sync <n> (WIFI home)" + "Send OK"
# khi AP (hotspot) tự đổi channel; nếu AP giữ channel thì chỉ 1 dòng sync khi nối AP.
```
Đổi kênh AP tự lành trong ≤ 500ms (chu kỳ gửi ESP-NOW).
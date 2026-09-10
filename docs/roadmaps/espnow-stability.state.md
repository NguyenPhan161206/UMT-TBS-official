# ESP-NOW Stability Fix — Orchestration State
Roadmap: docs/roadmaps/espnow-stability.roadmap.json
Updated: 2026-09-10

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Tắt Wi-Fi Power-Save trên waveshare-screen (receiver) | DONE | grep + `pio run -e yolo_uno` SUCCESS | coreiot_client.c:249 `ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE))` |
| 2 | Tắt Wi-Fi Power-Save trên sensor-node (sender) | DONE | grep + build 2 env SUCCESS | espnow_client.cpp:56 `esp_wifi_set_ps(WIFI_PS_NONE)` |
| 3 | Giảm nghẽn radio sender: COREIOT_PUBLISH_INTERVAL_MS 500 -> 2000 | DONE | grep + `pio run -e yolo_uno_coreiot` SUCCESS | coreiot_client.h:17 |
| 4 | Log transition ESP-NOW link UP/DOWN | DONE | grep + build SUCCESS | main.c:151 |
| 5 | Flash 2 board + soak 120s + nghiệm thu + log | DONE | soak 135s vòng 1: link_DOWN=19 / vòng 2 (sau 6-8): link_DOWN=0, frame=270/271 | Vòng 1 lộ gốc rễ: AP đá STA mỗi ~30s; vòng 2 đạt tiêu chí |
| 6 | Waveshare: exponential backoff WiFi reconnect + pin last AP channel | DONE | grep + `pio run -e yolo_uno` SUCCESS | coreiot_client.c: s_reconnect_delay_ms x2/cap 30s, s_last_ap_channel |
| 7 | Tăng ESPNOW_LINK_TIMEOUT_MS 1500 -> 3000 (shared) | DONE | grep==1 + build waveshare + sensor 2 env SUCCESS | espnow_protocol.h:55 |
| 8 | Re-flash 2 board + re-soak 120s + nghiệm thu + log | DONE | `grep -c "link DOWN" ESPNOW_STABILITY_LOG.md` = 0 (≤2 ✅) | Send_FAILED=0, MQTT waveshare giữ kết nối; sensor MQTT (phụ) chưa nối — ghi chú log |

## Contracts established
- (không có contract API mới — các thay đổi là cấu hình radio/hằng số)
- Hằng số liên quan: `ESPNOW_SEND_INTERVAL_MS 500` (espnow_protocol.h:54), `ESPNOW_LINK_TIMEOUT_MS 3000` (espnow_protocol.h:55).
- Kết quả vòng soak 1 (135s): sensor Send_OK=271 / Send_FAILED=0; waveshare frame=93, link_DOWN=19. Gốc rễ: WiFi STA waveshare bị iPhone hotspot đá mỗi ~30s (reason 205/2, channel 11).
- Kết quả vòng soak 2 (135s, sau step 6-7): waveshare frame=270/271, link_DOWN=0; sensor Send_OK=271, Send_FAILED=0. WiFi waveshare channel 11 ổn định; RSSI -30..-34 dBm.

## Deviations from plan
- Step 5 (vòng soak 1) KHÔNG đạt tiêu chí (19 lần link_DOWN): data lộ gốc rễ là iPhone hotspot đá STA của waveshare mỗi ~30s. Không phải lỗi code gửi/nhận ESP-NOW mà do môi trường AP. → Thêm steps 6-8 để gia cố (backoff reconnect + pin last AP channel + LINK_TIMEOUT 3000); vòng 2 đạt 0 link_DOWN.
- Sensor-node MQTT (đường phụ) không kết nối được CoreIoT trong cả 2 vòng soak (WiFi STA không giữ được với iPhone hotspot) — ngoài scope ESP-NOW, ghi chú ở ESPNOW_STABILITY_LOG.md.

## Context gốc (đừng suy lại mỗi lần)
- Triệu chứng: "bật lên ít nhưng tắt nhiều và lâu" — LINKED ngắn, NO LINK kéo dài.
- Root cause: cả 2 board STA mode, Wi-Fi Power-Save mặc định BẬT (WIFI_PS_MIN_MODEM):
  waveshare ngủ giữa các cửa sổ beacon (iPhone hotspot beacon ~102ms) → miss broadcast ESP-NOW;
  sensor-node (_coreiot) associate cùng AP, TX ESP-NOW bị trễ theo cửa sổ + nghẽn với MQTT publish 500ms.
- RF tốt: RSSI -31..-42 dBm trên waveshare → không phải lỗi phần cứng/đường truyền.
- Phiên bản trước đã sửa: đồng bộ peer channel theo home channel (espnow-dynamic-channel) — giữ nguyên.
- Khi 2 board cùng cắm: sensor-node = /dev/ttyACM1 (Espressif 303a:1001), waveshare = /dev/ttyACM0 (QinHeng 1a86:55d3).
- pio device monitor lỗi non-tty → đọc serial bằng script pyserial `serial.Serial(port, 115200)` + DTR/RTS toggle reset.
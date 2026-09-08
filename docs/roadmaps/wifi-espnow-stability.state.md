# WiFi/ESP-NOW Stability Fix — Orchestration State
Roadmap: docs/roadmaps/wifi-espnow-stability.roadmap.json
Updated: 2026-09-08

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Cập nhật WiFi credential trong config/keys.json | DONE | `gen_credentials.py --check` + grep (KEYS OK) | iPhone/anh131106 |
| 2 | Sinh lại credentials.h cho cả 2 firmware | DONE | `scan_secrets.py` + grep `"iPhone"` ×2 | cả 2 file = iPhone/anh131106 |
| 3 | Thêm log channel WiFi thực tế sau GOT_IP | DONE | grep + `pio run -e yolo_uno` SUCCESS | log `Wi-Fi channel primary=%u` coreiot_client.c:166 |
| 4 | Căn ESPNOW_CHANNEL theo channel AP thực tế | DONE | grep 1 dòng + 3 build SUCCESS | channel=6 (từ flash-and-observe) |
| 5 | Refactor điều khiển channel trong espnow_receiver | DONE | grep vị trí + build SUCCESS | `espnow_receiver_force_channel()` espnow_receiver.c:106 |
| 6 | Wire channel switch + backoff reconnect WiFi | DONE | grep vị trí + build SUCCESS | retry 3s, force_channel trong DISCONNECTED |
| 7 | Build toàn bộ + flash 2 board + nghiệm thu + log | TODO | — | |

## Contracts established
- `esp_err_t espnow_receiver_force_channel(void)` — waveshare-screen/components/espnow_receiver/include/espnow_receiver.h:58 (step 5, impl .c:106)
  - Hành vi: nếu `esp_wifi_sta_get_ap_info()` == ESP_OK (đang/kết nối AP) → return ESP_OK không ép; ngược lại ép `esp_wifi_set_channel(ESPNOW_CHANNEL)`.
- `ESPNOW_CHANNEL = 6` — firmware/shared/espnow_protocol.h:26 (step 4; AP iPhone channel 6)

## Deviations from plan
- Step 6: Working thêm `CMakeLists.txt` của coreiot_client (REQUIRES `espnow_receiver`) để include `espnow_receiver.h` — cần thiết, đã xác nhận.
- Step 6: Worker cũng đổi nhánh `WIFI_EVENT_STA_START` sang `wifi_reconnect_arm()` (thay vì esp_wifi_connect trực tiếp) để thoả "esp_wifi_connect chỉ 1 nơi" — chấp nhận, hợp lý.
- Step 6: Worker tự tạo `docs/logs/WAVESHARE_SCREENESPNOW_LINK_LOG.md` (sai tên, ngoài scope) — ĐÃ XOÁ; step 7 sẽ tạo log chính thức.

## Context gốc (đừng suy lại mỗi lần)
- keys.json đã đúng iPhone/anh131106 (step 1 DONE 2026-09-08).
- Triệu chứng: ESP-NOW nhấp nháy (flapping), MQTT luôn DOWN vì credentials.h stale (Bamos Coffee 2G) + xung đột kênh radio giữa STA và ESPNOW_CHANNEL=1.
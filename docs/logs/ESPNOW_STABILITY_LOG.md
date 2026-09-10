# ESP-NOW Stability Fix Log

Ngày: 2026-09-10 | Nhánh: nguyen | Roadmap: `espnow-stability` (8 steps)

## Mục tiêu
Chữa link ESP-NOW chập chờn ("bật lên ít, tắt nhiều và lâu") giữa sensor-node
(sender, env `yolo_uno_coreiot`) và waveshare-screen (receiver, env `yolo_uno`).

## Vòng đo 1 (trước fix reconnect) — 135s
| Metric | Kết quả |
|---|---|
| waveshare `link_DOWN` | 19 lần |
| waveshare frame nhận | 93 / ~271 (≈34%) |
| sensor `Send_FAILED` | 0 (271 OK) |
| RSSI | -47..-52 dBm |

**Kết luận vòng 1:** gửi TX hoàn hảo (đã fix power-save), nhận kém vì **Wi-Fi STA
của waveshare bị iPhone hotspot đá mỗi ~30s** (`reason 205/2`, `wifi:new:<11,0>,
old:<6,0>`): mỗi lần reconnect (fixed 3s) = scan+auth → radio mất 1-2s → mất
frame và nhấp nháy NO LINK. Khi WiFi ổn định, frame đến đúng 2/s.

## Fix áp dụng (roadmap steps 1-7)
1. Tắt Wi-Fi Power-Save trên receiver (coreiot_client.c) và sender (espnow_client.cpp).
2. `COREIOT_PUBLISH_INTERVAL_MS` 500 → 2000 (giảm nghẽn 1-radio với ESP-NOW).
3. Log transition `ESP-NOW link UP/DOWN` (main.c) để đo soak từ serial.
4. Waveshare reconnect WiFi **backoff luỹ thừa** 3s→30s (cap) + **pin last AP
   channel** khi mất kết nối (chỉ fallback ESPNOW_CHANNEL nếu chưa từng nối AP).
5. `ESPNOW_LINK_TIMEOUT_MS` 1500 → 3000 (dung sai micro-gap do AP chuyển kênh).

## Vòng đo 2 (sau fix) — 135s
| Metric | Kết quả | Tiêu chí |
|---|---|---|
| waveshare `link_DOWN` | **0** lần | ≤ 2 ✅ |
| waveshare frame nhận | **270 / 271** (≈100%) | — |
| sensor `Send_OK` | 271 | — |
| sensor `Send_FAILED` | **0** | 0 ✅ |
| RSSI | -30..-34 dBm | — |
| WiFi waveshare | kết nối ổn định, channel 11, không disconnect 1 lần | — |
| MQTT waveshare | Connected (giữ được kết nối) | — |

**Kết luận vòng 2:** link ESP-NOW ổn định tuyệt đối trong 135s — 0 lần tụt link,
nhận gần 100% frame, gói đến đều 2/s. DoD roadmap step 8 đạt.

## Ghi chú vận hành / giới hạn
- **Sensor-node MQTT (đường phụ) còn chưa nối được CoreIoT trong 2 vòng đo**
  (không có dòng `[NET] MQTT connected`; WiFi STA sensor-node không giữ được kết
  nối với iPhone hotspot). Không ảnh hưởng đường chính ESP-NOW; cần kiểm tra khi
  làm việc với môi trường demo khác (AP ổn định hơn).
- Vòng 2 không xuất hiện sự kiện disconnect của AP — mức cải thiện có thể một
  phần do môi trường WiFi êm hơn. Change backoff/pin channel còn nguyên tác dụng
  phòng thủ khi AP có đá client; nên quan sát 1-2 phiên dài hơn (10-30 phút)
  trước khi đánh giá tổng.
- Đo lại khi dùng AP/router thật thay iPhone hotspot để khử biến thiên.

## File đã sửa
- `firmware/waveshare-screen/components/coreiot_client/coreiot_client.c` (PS off,
  backoff reconnect, pin last AP channel)
- `firmware/sensor-node/src/espnow_client.cpp` (PS off)
- `firmware/sensor-node/src/plugins/coreiot/coreiot_client.h` (interval 2000)
- `firmware/waveshare-screen/src/main.c` (log transition link)
- `firmware/shared/espnow_protocol.h` (LINK_TIMEOUT 3000)

## Lệnh đã chạy
- Build: `pio run -e yolo_uno` (waveshare), `pio run -e yolo_uno -e yolo_uno_coreiot` (sensor)
- Flash: sensor `/dev/ttyACM1` (yolo_uno_coreiot), waveshare `/dev/ttyACM0` (yolo_uno)
- Soak: `/home/binhnguyen/.venv-pio/bin/python3 /tmp/opencode/soak_espnow.py 135`
# SOAK — Heartbeat soak test 24 h (DMXT-58)

- **Ngày**: 2026-10-06
- **Trạng thái**: firmware + công cụ xong, **chưa chạy soak trên board**.

## Mục tiêu
Có số liệu 24 h liên tục cho bài: số lần reset + lý do, heap (đầu/cuối/min, độ dốc), tỷ lệ nhận ESP-NOW,
số lần mất link, số lần MQTT kết nối lại, stack HWM. Heartbeat bật trong firmware thường (không env riêng).

## File đã sửa
| File | Thay đổi |
|---|---|
| `firmware/shared/soak_diag.h` (mới) | `TBS_SOAK_HEARTBEAT_INTERVAL_MS` = 60000; `tbs_reset_reason_name()` |
| `firmware/sensor-node/include/soak_heartbeat.h`, `src/soak_heartbeat.cpp` (mới) | Bộ đếm boot (Preferences `diag/boot`), dòng `BOOT node`, `SOAK node` mỗi 60 s (heap/minheap/blk, tx, mqtt_rc, HWM 3 task) |
| `firmware/sensor-node/src/espnow_client.cpp`, `include/espnow_client.h` | Đếm `tx_ok`/`tx_fail` (onDataSent + esp_now_send lỗi) |
| `firmware/sensor-node/src/plugins/coreiot/coreiot_client.{h,cpp}` | `reconnectCount()` |
| `firmware/sensor-node/src/main.cpp` | Gọi `soakHeartbeatBoot()` trong setup, `soakHeartbeatPoll()` trong networkTask |
| `firmware/waveshare-screen/src/soak_heartbeat.{h,c}` (mới), `src/CMakeLists.txt` | Bộ đếm boot NVS `diag/boot`, `BOOT scr`, task `soak` in `SOAK scr` mỗi 60 s (heap nội/PSRAM, rx, linkdn, maxgap, mqtt_rc, HWM task lvgl) |
| `firmware/waveshare-screen/components/espnow_receiver/espnow_receiver.{h,c}` | `espnow_receiver_rx_count()`, `espnow_receiver_take_max_gap_ms()` |
| `firmware/waveshare-screen/components/coreiot_client/coreiot_client.{h,c}` | `coreiot_client_reconnect_count()` |
| `firmware/waveshare-screen/src/main.c` | `soak_heartbeat_start()` sau NVS init; đếm link UP→DOWN trong watchdog |
| `tools/soak/soak_logger.py`, `test_soak_logger.py` (mới) | record (2 cổng, không DTR/RTS, tự mở lại cổng, CSV flush từng dòng, chặn spam lỗi) + analyze |
| `docs/SOAK_TEST.md` (mới) | Quy trình, chuẩn bị PC, tiêu chí |

## Kiểm thử (không cần board)
- Build sensor-node: `yolo_uno`, `yolo_uno_coreiot`, `yolo_uno_latency`, `yolo_uno_coreiot_latency` — SUCCESS (06/10).
- Build màn hình: xem mục "Build màn hình" bên dưới.
- `pytest tools/soak`: 9 passed (gồm test mở lại cổng khi USB mất, đếm reset bị lỡ dòng BOOT, cộng dồn bộ đếm qua reset).

## Build màn hình
- `yolo_uno` — SUCCESS (06/10, 21 phút 45 giây do CMake cấu hình lại khi thêm `soak_heartbeat.c`); RAM 32,8 %, Flash 32,0 %.
  Đã kiểm: có `.pio/build/yolo_uno/src/soak_heartbeat.c.o`; `firmware.elf` chứa `SOAK scr up=` và `BOOT scr boot=`,
  không chứa chuỗi của bản đo độ trễ.
- `yolo_uno_latency` — xem `docs/logs/LATENCY_DMXT57_LOG.md`.

## Chưa kiểm chứng trên phần cứng
- Dòng BOOT/SOAK thật trên COM4/COM9, giá trị `hwm_lvgl` (tên task `lvgl`), hành vi tự mở lại cổng khi board reset thật.

## Kết quả soak
_(chưa có)_

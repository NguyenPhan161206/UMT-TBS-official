# SENSOR-NODE — Sửa thứ tự d1..d6 trong telemetry MQTT

- **Ngày**: 2026-10-07
- **Trạng thái**: đã sửa + build; chưa kiểm trên CoreIoT (cần nạp `yolo_uno_coreiot` / `_coreiot_latency`).

## Lỗi
Quy ước dự án: telemetry `d1..d6` theo **thứ tự slot ESP-NOW** — d1=Front, d2=Rear, d3=L-Front, d4=L-Rear,
d5=R-Front, d6=R-Rear (`firmware/shared/espnow_protocol.h`, `tools/scenarios.py`, `docs/G1_TESTING_GUIDE.md:121`,
recorder/replayer, host_sim). Nhưng `coreiotTask` (`firmware/sensor-node/src/main.cpp`) gửi theo **thứ tự chân
`SENSOR_PINS`**: d1=Front, d2=L-Front, d3=R-Front, d4=L-Rear, d5=R-Rear, d6=Rear.

Hệ quả: d2, d3, d5, d6 trên CoreIoT sai nhãn (chỉ d1, d4 trùng). Không ảnh hưởng: ESP-NOW/màn hình (đã ánh xạ qua
`SENSOR_ESPNOW_SLOT[]`), cảnh báo rule-chain (dùng `nearest_cm` / giá trị nhỏ nhất). Ảnh hưởng: nhãn từng cảm biến
trên dashboard và dữ liệu lấy từ CoreIoT (MQTT/REST) để phát lại.

**Dữ liệu telemetry CoreIoT ghi trước ngày sửa có d2/d3/d5/d6 sai nhãn** — muốn dùng lại thì hoán vị:
slot = [d1, d6, d2, d4, d3, d5] (Front, Rear, L-Front, L-Rear, R-Front, R-Rear) theo dữ liệu cũ.

Phát hiện thêm: `sharedStateGetValue()` chú thích "0 nếu chưa hợp lệ" nhưng trả giá trị cũ còn lưu khi `valid=false`.

## Sửa
`firmware/sensor-node/src/main.cpp`:
- Gom giá trị vào `slotCm[SENSOR_ESPNOW_SLOT[i]]` rồi in d1..d6 theo `ESPNOW_SLOT_FRONT..RIGHT_REAR` — cùng bảng ánh xạ
  với ESP-NOW.
- `sharedStateGetValue()` trả 0 khi cảm biến không hợp lệ.

## Kiểm thử
- `pio run` sensor-node `yolo_uno`, `yolo_uno_coreiot`, `yolo_uno_latency`, `yolo_uno_coreiot_latency`,
  `yolo_uno_accuracy` — SUCCESS (07/10).
- Chưa kiểm trên board: nạp `yolo_uno_coreiot`, đặt vật trước cảm biến cổng 9/10 (R-Front) → CoreIoT phải thấy giá trị
  ở **d5** (trước đây hiện ở d3).

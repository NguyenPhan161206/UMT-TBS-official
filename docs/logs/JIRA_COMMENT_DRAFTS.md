# Nháp comment Jira — CHƯA GỬI, chờ duyệt (2026-10-06)

Các comment chỉ nói trạng thái thật: công cụ xong, **chưa có số đo trên board**. Khi có số đo sẽ soạn comment kết quả riêng.

---

## DMXT-55 — Độ chính xác/ổn định 6 khoảng cách (raw vs lọc)

Cập nhật 06/10: đã xong phần chuẩn bị đo, **chưa đo**.
- Firmware: env `yolo_uno_accuracy` của sensor-node in mỗi lần đọc cả giá trị raw và giá trị sau lọc (dòng `ACC`), kèm lý do khi mẫu bị loại.
- Công cụ: `tools/accuracy/measure_accuracy.py` ghi từng mốc ra CSV và gộp bảng (n, % reject, bias/σ/MAE/sai số max cho raw và đã lọc).
- Quy trình: `docs/ACCURACY_TEST.md` — 6 mốc 30/50/100/150/200/300 cm, vật chắn phẳng ≥ 30×30 cm vuông góc, 1 cảm biến, ≥ 300 mẫu/mốc, ghi nhiệt độ phòng.
- Việc tiếp theo: đo trên bàn (~30 phút), cần 1 sensor-node + 1 cảm biến.

## DMXT-57 — Độ trễ đầu–cuối ESP-NOW và MQTT (bản "chưa đo" — KHÔNG gửi, đã thay bằng bản kết quả bên dưới)

Cập nhật 06/10: đã xong phần chuẩn bị đo, **chưa đo**.
- ESP-NOW: đo RTT/2 bằng echo (màn hình gửi lại seq, sensor-node đo bằng đồng hồ của chính nó), không cần logic analyzer. Env `yolo_uno_latency` cho cả 2 board.
- MQTT: telemetry có `seq`, rule-chain chuyển `seq` sang màn hình; PC đọc 2 cổng serial cùng lúc để lấy độ trễ.
- Công cụ `tools/latency/measure_latency.py` (min/p50/p95/p99/max, tỷ lệ nhận). Quy trình `docs/LATENCY_TEST.md`.
- Build: sensor-node 4 env SUCCESS; màn hình `yolo_uno_latency` SUCCESS.
- Việc tiếp theo: import lại rule-chain lên CoreIoT, rồi đo 3 lượt (ESP-NOW 1 m, 5 m, hybrid có MQTT), tổng ~25 phút.

## DMXT-58 — Soak test 24 h

Cập nhật 06/10: đã xong phần chuẩn bị, **chưa chạy soak**.
- Firmware thường (không env riêng) in dòng `BOOT` lúc khởi động và `SOAK` mỗi 60 s trên cả 2 board: số lần boot + lý do reset, heap, tx/rx ESP-NOW, số lần mất link, khoảng hở lớn nhất, số lần MQTT kết nối lại, stack HWM.
- Công cụ `tools/soak/soak_logger.py` ghi 2 cổng, tự mở lại cổng khi board reset, tổng kết reset/heap/tỷ lệ nhận.
- Quy trình + tiêu chí: `docs/SOAK_TEST.md` (0 reset ngoài ý muốn, min heap đi ngang, không mất link > 1500 ms).
- Việc tiếp theo: flash `yolo_uno_coreiot` (sensor-node) + `yolo_uno` (màn hình), chạy thử 30 phút, rồi chạy 24 h từ tối 06/10 hoặc sáng 07/10.

---

## DMXT-57 — KẾT QUẢ — ✅ ĐÃ GỬI 06/10 (comment 10044, bản gửi có thêm đối chiếu mục tiêu ESP-NOW < 50 ms / MQTT 100–500 ms)

Đã đo độ trễ V2 trên bàn, 2 board cách 1 m (log: `docs/logs/LATENCY_DMXT57_LOG.md`, dữ liệu `data/latency/`):
- **ESP-NOW một chiều (RTT/2)**: Wi-Fi tắt — p50 2,73 ms, p95 6,16 ms, nhận 99,83 % (2999 gói, 5 phút);
  Wi-Fi bật (hybrid) — p50 2,03 ms, p95 4,01 ms, nhận 99,93 % (8932 gói, 15 phút). Gói mất đều rời rạc (khoảng hở ≤ ~200 ms).
- **MQTT đầu–cuối** (sensor-node → CoreIoT rule-chain → màn hình): p50 420 ms, p95 1001 ms, max 2,34 s, 450/450 bản tin (15 phút),
  qua hotspot PC + Wi-Fi công cộng → phụ thuộc mạng.
- Từ lúc đo đến lúc màn hình nhận (tuổi mẫu + ESP-NOW): p50 ≈ 54 ms, p95 ≈ 104 ms — chủ yếu do chu kỳ đo/gửi 100 ms.
- Sửa trong lúc đo: thiếu nhánh xử lý echo ở sensor-node; màn hình chỉ quét kênh 6 khi chưa biết kênh AP; `credentials.h` cũ
  (mật khẩu Wi-Fi); device CoreIoT đặt tên sai `waveshare-sreen`.
- Chưa đo: thời gian vẽ UI trên màn hình; khoảng cách xa/vật cản như trên xe.
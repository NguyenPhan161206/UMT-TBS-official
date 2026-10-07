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
---

## DMXT-55 — KẾT QUẢ (nháp 07/10, CHỜ DUYỆT — chưa gửi)

**Kết quả đo độ chính xác & ổn định V2 — 07/10/2026** (trên bàn, trong phòng 27 °C)

Điều kiện: 1 cảm biến JSN-SR04T (cổng 9/10), cố định; tấm bìa phẳng ≥ 30×30 cm, vuông góc; mốc đo bằng thước từ mặt đầu dò; mỗi mốc 35 s (350 mẫu, bỏ 2 s đầu). Firmware env `yolo_uno_accuracy` (log raw + đầu ra bộ lọc mỗi lần đọc).

| Mốc (cm) | Reject | Raw mean | Bias raw | σ raw | Bias lọc | σ lọc |
|---|---|---|---|---|---|---|
| 30 | 0 % | 28,96 | −1,04 | 0,08 | −1,00 | 0,02 |
| 50 | 0 % | 49,22 | −0,78 | 0,04 | −0,79 | 0,03 |
| 100 | 0 % | 96,02 | −3,98 | 0,20 | −3,98 | 0,16 |
| 150 | 0 % | 146,46 | −3,54 | 0,05 | −3,54 | 0,05 |
| 200 | 0 % | 194,70 | −5,30 | 0,13 | −5,27 | 0,08 |
| 300 | 0 % | 293,42 | −6,58 | 0,08 | −6,59 | 0,03 |

**Nhận xét**
- Reject 0 % ở cả 6 mốc (30–300 cm, tấm chắn phẳng). Độ lặp lại tốt: σ raw ≤ 0,20 cm; bộ lọc giảm dao động (σ lọc ≤ 0,16 cm) nhưng không sửa bias.
- Bias mang tính hệ thống, tăng tuyến tính: đo ≈ 0,9785 × thật − 0,56 cm (−2,15 %), phần dư RMS 0,69 cm. MAE 3,54 cm, lớn nhất 6,58 cm (300 cm).
- Ở 27 °C, hằng số tốc độ âm cố định của firmware (≈ 20 °C) giải thích ≈ 57 % sai lệch tỉ lệ; bù nhiệt độ hậu xử lý (ước tính, firmware chưa có) giảm MAE còn 1,85 cm, lớn nhất 2,91 cm. Phần ≈ −0,9 % còn lại chưa tách được (độ ẩm, sai số thước nối đoạn ở mốc xa, đặc tính module).
- Quan sát cho mục Hạn chế: một lượt 200 cm có vật lạ trong vùng búp → 6,9 % mẫu là Echo ảo ở ~92 cm; bộ lọc nhận (cơ chế "nhanh vào") → đầu ra < 150 cm tổng 2,9 s, tương đương báo CAUTION sai. Đã đo lại sạch; giữ file làm bằng chứng.

**Hạn chế:** sai số đặt mốc bằng tay (các lần đặt cùng mốc 100 cm chênh tới 7,7 cm) lớn hơn σ cảm biến nên bias từng mốc gồm cả sai số mốc; 1 cảm biến, 1 loại vật chắn, trong phòng; chưa đo góc búp (DMXT-56).

Chi tiết + các lượt không dùng (kèm lý do): `docs/logs/ACCURACY_DMXT55_LOG.md`; bảng `data/accuracy/acc_final_S2.md`; dữ liệu thô `data/accuracy/acc*_S2_*.csv` (nhánh `khoa`).

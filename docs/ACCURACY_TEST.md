# Đo độ chính xác, tầm đo và góc búp JSN-SR04T (DMXT-55, DMXT-56)

Đo trên bàn/sàn trong phòng, không cần xe. Chỉ báo cáo số đo từ quy trình này (V2), không trộn số V1.

## 1. Firmware

> **Dò cổng trước khi nạp/đo** — số COM đổi theo máy/cổng USB:
> `python -m serial.tools.list_ports -v` → sensor-node là `USB Serial Device` (VID:PID 303A:1001),
> màn hình là `USB-Enhanced-SERIAL CH343` (1A86:55D3). Luôn ghi rõ `--upload-port` (PlatformIO có thể tự chọn
> nhầm sensor-node khi nạp màn hình). Script đo **tự dò cổng** theo VID:PID (`auto`); chỉ định tay bằng
> `--sensor-port/--screen-port` (hoặc `--port`) nếu cần. Máy dev ngày 06/10: **sensor-node = COM7, màn hình = COM9**
> (COM4 trên máy này là cổng Bluetooth). Lệnh nạp dưới đây dùng các cổng đó.

Env `yolo_uno_accuracy` (`TBS_ACCURACY_PROBE=1`) — **không dùng cho bản phát hành**:

```bash
cd firmware/sensor-node && pio run -e yolo_uno_accuracy -t upload --upload-port COM7
```

Mỗi lần đọc (10 lần/giây mỗi cảm biến) in một dòng:

| Trường hợp | Dòng |
|---|---|
| Mẫu hợp lệ | `ACC S<i> raw=<cm> out=<cm\|nan> has=<0/1> status=<trạng thái lọc> n=<số phiếu cụm> pulse=<µs>` |
| Mẫu bị loại | `ACC S<i> raw=nan pulse=<µs> calc=<cm\|nan> rej=<lý do>` |

- `raw`: khoảng cách từ 1 xung (`pulse × 0,0343 / 2`, `SOUND_SPEED_CM_PER_US` cố định ≈ 20 °C).
- `out`: đầu ra bộ lọc DistanceFilter (giá trị gửi qua ESP-NOW); `has=0` khi bộ lọc chưa có đầu ra (WARMUP…).
- `calc`: khoảng cách tính được của mẫu bị loại (ngoài `MIN_DISTANCE_CM` 15 / `MAX_DISTANCE_CM` 500) — dùng cho tầm đo.
- Ở env này, cảm biến bị đánh dấu DISCONNECTED vẫn được đọc **mỗi chu kỳ** (bản thường chỉ thăm dò 1 lần/giây),
  để tỷ lệ phát hiện không bị lệch.

## 2. Bố trí chung

- **Chỉ cắm 1 cảm biến** (tránh nhiễu chéo giữa các đầu dò). Ghi lại cảm biến nào (S0 = cặp chân đầu tiên
  trong `SENSOR_PINS`).
- Đầu dò kẹp cố định, mặt đầu dò thẳng đứng, cách sàn ≥ 50 cm (giảm vọng từ sàn); ghi độ cao.
- Vật chắn **phẳng, cứng, ≥ 30 × 30 cm**, đặt **vuông góc** với trục đầu dò.
- Khoảng cách chuẩn đo bằng **thước dây, tính từ mặt đầu dò** tới mặt vật chắn.
- Ghi **nhiệt độ phòng** (và độ ẩm nếu có) — tốc độ âm thanh đổi ≈ 0,17 %/°C, giải thích được bias.
- Không có người/vật khác trong vùng búp khi đo.

## 3. DMXT-55 — Độ chính xác & ổn định ở 6 mốc

Mốc: **30, 50, 100, 150, 200, 300 cm**. Mỗi mốc ≥ 300 mẫu (35 s ở 10 mẫu/s), bỏ 2 s đầu (WARMUP bộ lọc).

```bash
pip install pyserial
```

```bash
python tools/accuracy/measure_accuracy.py record --sensor 0 --true-cm 30 --duration 35 --skip 2 --tag acc
```

Lặp lại với `--true-cm 50`, `100`, `150`, `200`, `300`. Mỗi lần ghi `data/accuracy/acc_S0_<mốc>cm_<giờ>.csv`
và in bảng của mốc đó. Gộp bảng 6 mốc:

```bash
python tools/accuracy/measure_accuracy.py analyze "data/accuracy/acc_S2_*.csv" --json data/accuracy/acc_S2.json --md data/accuracy/acc_S2.md
```

`--md` lưu bảng ra file Markdown UTF-8 (kèm danh sách file nguồn) để dán vào log/bài báo; `--json` lưu số liệu.
Bảng có, theo từng mốc: n, % reject; **raw**: mean, bias (mean − mốc), σ, MAE, |sai số| lớn nhất;
**đã lọc**: n, mean, bias, σ, MAE, % mẫu hợp lệ có đầu ra. Báo cáo raw và đã lọc cạnh nhau để thấy tác dụng
của bộ lọc. Lưu ý: σ của chuỗi đã lọc tính trên các mẫu liên tiếp có tương quan (bộ lọc giữ giá trị).

## 4. DMXT-56 — Tầm đo và góc búp (nếu kịp)

Vật chắn cho góc búp: nên dùng **ống/cột tròn** (vd ống PVC Ø 5–10 cm, cao > búp) để kết quả ít phụ thuộc
hướng mặt phẳng; ghi rõ vật dùng.

**Tầm đo** (góc 0°): đặt vật ở 300, 350, 400, 450, 500 cm, mỗi điểm 20 s:

```bash
python tools/accuracy/measure_accuracy.py record --sensor 0 --true-cm 400 --angle-deg 0 --height-cm 50 --duration 20 --tag beam
```

**Góc búp**: ở 1–3 độ cao đầu dò (vd 30, 50, 80 cm so với sàn), khoảng cách 100 cm và 200 cm, góc
0, ±10, ±20, ±30, ±40° (đo góc bằng thước đo độ/đánh dấu cung trên sàn), mỗi ô 20 s:

```bash
python tools/accuracy/measure_accuracy.py record --sensor 0 --true-cm 100 --angle-deg 20 --height-cm 50 --duration 20 --tag beam
```

Tổng hợp:

```bash
python tools/accuracy/measure_accuracy.py analyze --beam "data/accuracy/beam_*.csv" --tol-cm 15
```

"Phát hiện" = lần đọc hợp lệ có |raw − mốc| ≤ `--tol-cm` (loại vọng từ sàn/tường). Đề xuất định nghĩa khi viết:
**tầm đo** = khoảng cách lớn nhất có % phát hiện ≥ 90 % ở 0°; **nửa góc búp** = góc lớn nhất có % phát hiện ≥ 50 %
(ghi rõ ngưỡng đã dùng).

## 5. Ghi kết quả

- Dán bảng vào `docs/PROGRESS.md` (T5.1) và `docs/logs/ACCURACY_DMXT55_LOG.md`, kèm tên file CSV, nhiệt độ,
  cảm biến, vật chắn, độ cao.
- Comment tóm tắt lên Jira DMXT-55 / DMXT-56.

## 6. Kiểm thử công cụ (không cần board)

```bash
python -m pytest tools/accuracy -q
```

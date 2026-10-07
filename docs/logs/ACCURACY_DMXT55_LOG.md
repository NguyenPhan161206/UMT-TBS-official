# ACCURACY — Độ chính xác raw vs đã lọc (DMXT-55), tầm đo & góc búp (DMXT-56)

- **Ngày**: 2026-10-06
- **Trạng thái**: firmware + công cụ xong, **chưa đo**.

## Mục tiêu
Số đo V2 ở 6 mốc 30/50/100/150/200/300 cm: tỷ lệ reject, bias/σ/MAE của raw và của đầu ra bộ lọc;
(nếu kịp) tầm đo và tỷ lệ phát hiện theo góc ở 1–3 độ cao.

## File đã sửa
| File | Thay đổi |
|---|---|
| `firmware/sensor-node/platformio.ini` | Env `yolo_uno_accuracy` (`-D TBS_ACCURACY_PROBE=1`) |
| `firmware/sensor-node/src/main.cpp` | (`TBS_ACCURACY_PROBE`) dòng `ACC S<i> raw= out= has= status= n= pulse=` / `ACC S<i> raw=nan pulse= calc= rej=`; đọc cả cảm biến DISCONNECTED mỗi chu kỳ |
| `tools/accuracy/measure_accuracy.py`, `test_measure_accuracy.py` (mới) | record 1 mốc + analyze nhiều CSV, chế độ `--beam` |
| `docs/ACCURACY_TEST.md` (mới) | Quy trình 6 mốc + tầm đo + góc búp |
| `docs/TEST_PROTOCOL.md` | Mục 2.1/2.2 khớp log hiện tại (`DIST`, `REJECT`, `BOOT`/`SOAK`, `ESP-NOW link UP`) |

Khác bản kế hoạch: dòng reject đặt `rej=` **cuối dòng** (lý do có dấu cách) và thêm `calc=` (khoảng cách tính được
của mẫu bị loại, dùng cho tầm đo).

## Kiểm thử (không cần board)
- `pio run -e yolo_uno_accuracy` — SUCCESS (06/10); `yolo_uno` vẫn SUCCESS.
- `pytest tools/accuracy`: 8 passed.

## Chạy thử trên board (06/10 21:24)
- `smoke_S0_100cm_20261006_212450.csv`: 46 mẫu/10 s, 100 % timeout ở S0. Nguyên nhân: (1) cảm biến cắm ở cổng
  S2 (GPIO 9/10), S0 trống; đọc serial thấy S2 ổn định 498,9 cm — chưa nhắm vào tấm chắn 100 cm. (2) Firmware đo đọc
  cả 5 cổng trống mỗi chu kỳ (5 × timeout 40 ms) → chu kỳ ~240 ms thay vì 100 ms.
- Sửa (2): cổng **chưa từng có xung Echo** từ lúc boot vẫn thăm dò 1 lần/10 chu kỳ như bản thường; cổng đã có Echo thì
  đọc mọi chu kỳ. Build lại `yolo_uno_accuracy` + `yolo_uno` SUCCESS 21:27. File smoke giữ lại, không dùng làm số đo.

## Kết quả đo (07/10/2026 14:12–14:54)

**Điều kiện:** 1 cảm biến JSN-SR04T cổng 9/10 (`--sensor 2`), đặt cố định; tấm bìa phẳng vuông góc; mốc đo bằng thước từ mặt
đầu dò; mỗi mốc 35 s, bỏ 2 s đầu (WARMUP), 350 mẫu. **Nhiệt độ phòng: 27 °C** (người đo cung cấp; độ ẩm không đo).
Firmware `yolo_uno_accuracy`. Bảng chính thức: `data/accuracy/acc_final_S2.md` / `.json`.

### Bảng chính thức (lượt cuối cùng của mỗi mốc)

| Mốc (cm) | File | n | Reject | Raw mean | Bias | σ raw | MAE | \|err\| max | Lọc mean | Bias lọc | σ lọc |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 30 | `acc_S2_30cm_20261007_141244` | 350 | 0 % | 28,96 | −1,04 | 0,08 | 1,04 | 1,40 | 29,00 | −1,00 | 0,02 |
| 50 | `acc_S2_50cm_20261007_141920` | 350 | 0 % | 49,22 | −0,78 | 0,04 | 0,78 | 0,80 | 49,21 | −0,79 | 0,03 |
| 100 | `acc2_S2_100cm_20261007_144603` | 350 | 0 % | 96,02 | −3,98 | 0,20 | 3,98 | 4,20 | 96,02 | −3,98 | 0,16 |
| 150 | `acc2_S2_150cm_20261007_145120` | 350 | 0 % | 146,46 | −3,54 | 0,05 | 3,54 | 3,60 | 146,46 | −3,54 | 0,05 |
| 200 | `acc2_S2_200cm_20261007_145334` | 350 | 0 % | 194,70 | −5,30 | 0,13 | 5,30 | 5,70 | 194,73 | −5,27 | 0,08 |
| 300 | `acc_S2_300cm_20261007_143907` | 350 | 0 % | 293,42 | −6,58 | 0,08 | 6,58 | 6,70 | 293,41 | −6,59 | 0,03 |

- **Reject 0 %** ở cả 6 mốc (30–300 cm, tấm chắn phẳng).
- **Độ lặp lại:** σ raw 0,04–0,20 cm; bộ lọc giảm còn 0,02–0,16 cm.
- **Sai lệch hệ thống tuyến tính:** hồi quy 6 mốc: *đo = 0,9785 × thật − 0,56 cm* (tỉ lệ −2,15 %), phần dư RMS 0,69 cm,
  lớn nhất 1,27 cm (mốc 100).
- **Phần do nhiệt độ:** firmware dùng 0,0343 cm/µs (343 m/s ≈ 20 °C); ở 27 °C c ≈ 347,3 m/s → lý thuyết −1,23 % → giải thích
  ≈ 57 % sai lệch tỉ lệ. Phần còn lại ≈ −0,9 % chưa tách được (độ ẩm ~+0,3 % tốc độ âm, sai số thước khi đo nối đoạn ở mốc xa,
  đặc tính trễ của module). *(Nhận định "≈ 32–33 °C" trong bản trước đã bị thay sau khi có nhiệt độ thật.)*
- **Bù nhiệt độ hậu xử lý (ước tính, firmware chưa có):** nhân 1,0125 → MAE trung bình 3,54 → **1,85 cm**, lớn nhất 6,58 → **2,91 cm**;
  sau bù còn *đo = 0,9907 × thật − 0,57*.

### Các lượt KHÔNG dùng trong bảng chính thức (giữ nguyên file)
| File | Mốc | Raw mean | Lý do |
|---|---|---|---|
| `acc_S2_100cm_20261007_142229` | 100 | 103,73 | Đặt bìa không lặp lại được: cùng mốc 100 cm, các lần đặt cho 97,99 (smoke 14:05) / 103,73 / 96,02 → chênh tới 7,7 cm trong khi σ cảm biến ≤ 0,5 cm → lỗi đặt mốc |
| `acc_S2_150cm_20261007_143050` | 150 | 145,83 | Số đo trôi 146,1 → 145,3 trong 35 s (bìa nghiêng/võng), σ 1,11 |
| `acc_S2_150cm_20261007_144446` | 150 | 144,49 | Lượt đo lại trung gian (tag `acc`), sau đó đo lại lần nữa với tag `acc2` |
| `acc_S2_200cm_20261007_143632` | 200 | 191,62 | **Echo ảo:** 24/350 mẫu (6,9 %) đọc ≈ 92,5 cm (có vật trong vùng búp ở ~92 cm); cụm chính 198,9 cm. Bộ lọc nhận Echo ảo (fast-track "nhanh vào") → đầu ra lọc < 150 cm tổng 2,9 s → tương đương báo CAUTION sai. **Giữ làm bằng chứng cho mục Hạn chế (báo nhầm do đa đường).** |

### Hạn chế
- Sai số chuẩn của mốc (đặt bìa bằng tay + thước, đo nối đoạn ở 200/300 cm) cỡ vài cm — lớn hơn σ cảm biến; bias từng mốc
  bao gồm cả sai số đặt mốc.
- 1 cảm biến, 1 loại vật chắn (phẳng, vuông góc), trong phòng; chưa đo góc búp (DMXT-56); firmware chưa bù nhiệt độ (hướng phát triển: cảm biến nhiệt độ + hệ số tốc độ âm).

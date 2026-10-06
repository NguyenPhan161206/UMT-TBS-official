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

## Kết quả đo
_(chưa có)_

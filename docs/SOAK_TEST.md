# Soak test 24 h (DMXT-58)

Chạy liên tục 2 board trong 24 h, ghi heap, số lần reset (kèm lý do), tỷ lệ nhận ESP-NOW, số lần mất link
và số lần MQTT kết nối lại. Chỉ báo cáo số đo được từ quy trình này (V2), không trộn số V1.

## 1. Firmware: dùng bản thường

> **Dò cổng trước khi nạp/đo** — số COM đổi theo máy/cổng USB:
> `python -m serial.tools.list_ports -v` → sensor-node là `USB Serial Device` (VID:PID 303A:1001),
> màn hình là `USB-Enhanced-SERIAL CH343` (1A86:55D3). Luôn ghi rõ `--upload-port` (PlatformIO có thể tự chọn
> nhầm sensor-node khi nạp màn hình). Script đo **tự dò cổng** theo VID:PID (`auto`); chỉ định tay bằng
> `--sensor-port/--screen-port` (hoặc `--port`) nếu cần. Máy dev ngày 06/10: **sensor-node = COM7, màn hình = COM9**
> (COM4 trên máy này là cổng Bluetooth). Lệnh nạp dưới đây dùng các cổng đó.

Heartbeat được bật sẵn trong firmware thường, **không dùng env `*_latency`** (env đó phát thêm gói echo,
làm sai điều kiện thật).

| Board | Env | Cổng |
|---|---|---|
| sensor-node | `yolo_uno_coreiot` (Hybrid: ESP-NOW + MQTT) | COM7 |
| màn hình | `yolo_uno` (bật Wi-Fi, cùng AP với sensor-node) | COM9 |

```bash
cd firmware/sensor-node && pio run -e yolo_uno_coreiot -t upload --upload-port COM7
```

```bash
cd firmware/waveshare-screen && pio run -e yolo_uno -t upload --upload-port COM9
```

Dòng log (chu kỳ `TBS_SOAK_HEARTBEAT_INTERVAL_MS` = 60 s, `firmware/shared/soak_diag.h`):

| Dòng | Trường |
|---|---|
| `BOOT node boot=N rr=R` | Lúc khởi động. `boot`: bộ đếm boot lưu NVS; `rr`: `esp_reset_reason()` |
| `SOAK node up= boot= rr= heap= minheap= blk= tx_ok= tx_fail= mqtt_rc= hwm_sensor= hwm_net= hwm_buzz=` | `up` giây từ boot; heap/minheap/blk: heap trống, thấp nhất từ boot, block lớn nhất (byte); `tx_ok`/`tx_fail` gói ESP-NOW từ boot; `mqtt_rc` số lần MQTT kết nối lại (−1 nếu build không có CoreIoT); `hwm_*` stack còn trống thấp nhất (byte) |
| `BOOT scr boot=N rr=R` | Như trên, cho màn hình |
| `SOAK scr up= boot= rr= iheap= iminheap= iblk= psram= psrammin= rx= linkdn= maxgap_ms= mqtt_rc= hwm_lvgl=` | `i*` heap RAM nội; `psram*` PSRAM; `rx` gói ESP-NOW hợp lệ từ boot; `linkdn` số lần link UP→DOWN; `maxgap_ms` khoảng hở lớn nhất giữa 2 gói **trong 60 s vừa qua**; `hwm_lvgl` stack task LVGL |

Gửi broadcast không có ACK, nên `tx_ok` chỉ có nghĩa là gói đã được phát đi. Tỷ lệ nhận thật là `rx / tx_ok`.

## 2. Chuẩn bị PC (người chạy tự làm)

> **Đổi mật khẩu Wi-Fi/token trong `config/keys.json`?** Phải sinh lại header rồi build + nạp lại, nếu không firmware vẫn
> mang giá trị cũ (06/10: board thấy hotspot nhưng lỗi xác thực "Reason code 15" vì `credentials.h` cũ):
> `python tools/guard/gen_credentials.py --out firmware/sensor-node/include/credentials.h` và
> `python tools/guard/gen_credentials.py --out firmware/waveshare-screen/components/coreiot_client/include/credentials.h`.
> Hotspot Windows: băng 2,4 GHz; phát cùng kênh với Wi-Fi PC đang nối — màn hình quét mọi kênh khi chưa biết kênh AP.

- Cắm sạc laptop, tắt Sleep/Hibernate khi cắm điện (Settings → System → Power).
- Tắt USB selective suspend: Control Panel → Power Options → Change plan settings → Change advanced power
  settings → USB settings → USB selective suspend setting → **Disabled**.
- Tạm dừng Windows Update ≥ 2 ngày (Settings → Windows Update → Pause updates).
- Đóng mọi serial monitor đang mở cổng của 2 board (mỗi cổng chỉ một chương trình được mở).
- Nguồn cho 2 board ổn định (không cắm qua hub chung với thiết bị tốn điện).

## 3. Bố trí

- Hybrid: sensor-node và màn hình nối cùng AP Wi-Fi; CoreIoT hoạt động (dashboard thấy device Active).
- Gắn ít nhất 1 cảm biến, đặt vật cố định trước nó để luồng đo/lọc/gửi luôn chạy.
- 2 board đặt cố định, ghi khoảng cách giữa 2 board.
- **Chụp ảnh trang System trên màn hình lúc bắt đầu và lúc kết thúc.**

## 4. Chạy

```bash
pip install pyserial
```

Chạy thử 30 phút trước (phải thấy dòng SOAK của **cả 2 board** mỗi phút):

```bash
python tools/soak/soak_logger.py record --duration 1800 --tag try30m
```

Chạy chính thức (24 h 10 phút):

```bash
python tools/soak/soak_logger.py record --duration 87000 --tag soak24h
```

- CSV ghi vào `data/soak/<tag>_<giờ>.csv`, flush từng dòng (PC tắt đột ngột vẫn còn dữ liệu tới lúc đó).
- Chỉ lưu dòng SOAK/BOOT, dòng lỗi (Guru Meditation, watchdog, brownout, `E (…)`, mất link/MQTT,
  tối đa 30 dòng/phút) và sự kiện cổng (`PORT_OPEN`, `PORT_LOST`).
- Thêm `--raw` để ghi toàn bộ log thô (file lớn, chỉ dùng khi cần điều tra crash).
- Board reset làm USB-CDC biến mất: script tự mở lại cổng. Dòng `BOOT` có thể bị lỡ nếu cổng chưa mở lại kịp,
  nhưng reset vẫn được đếm qua `boot=` trong dòng SOAK kế tiếp.

Tổng kết lại từ file:

```bash
python tools/soak/soak_logger.py analyze data/soak/soak24h_20261006_210000.csv --json data/soak/soak24h.json
```

Báo cáo theo từng board: số lần reset + lý do + thời điểm, heap đầu/cuối/thấp nhất, độ dốc heap trống
(KB/h, hồi quy tuyến tính, bỏ 10 phút đầu sau mỗi boot), stack HWM thấp nhất, số lần MQTT kết nối lại; ESP-NOW:
rx/tx, số lần mất link, khoảng hở lớn nhất.

## 5. Tiêu chí đề xuất

| Tiêu chí | Ngưỡng |
|---|---|
| Reset ngoài ý muốn (PANIC, *_WDT, BROWNOUT, SW, …) | 0 |
| Min heap | Đi ngang: giá trị "sau 10 phút đầu" ≈ "cuối", độ dốc heap trống ≈ 0 |
| Mất link ESP-NOW | Không cửa sổ nào có `maxgap_ms` > 1500 (`ESPNOW_LINK_TIMEOUT_MS`) |

Ghi kết quả vào `docs/PROGRESS.md` (T5.3) và `docs/logs/SOAK_DMXT58_LOG.md`, kèm tên file CSV và ảnh trang System.

## 6. Kiểm thử công cụ (không cần board)

```bash
python -m pytest tools/soak -q
```

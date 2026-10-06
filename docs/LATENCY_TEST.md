# Đo độ trễ ESP-NOW & MQTT (DMXT-57)

Hướng dẫn đo độ trễ của V2 trên bàn (không cần xe). Chỉ dùng số đo từ quy trình này cho bài báo, **không
trộn với số V1**.

## 1. Đo cái gì

| Đại lượng | Định nghĩa | Cách đo | Sai số chính |
|---|---|---|---|
| **ESP-NOW một chiều** | `esp_now_send` trên sensor-node → callback nhận trên màn hình | **RTT/2**: màn hình echo `seq` ngay trong callback nhận; sensor-node đo RTT bằng `esp_timer` (µs) | Giả định 2 chiều đối xứng; RTT có cả thời gian màn hình gửi echo |
| ESP-NOW theo mốc PC (đối chiếu) | Mốc PC lúc màn hình in `RX` − mốc PC lúc sensor-node in `TX` | 2 cổng serial cùng 1 PC | Jitter USB/serial vài ms — chỉ dùng để đối chiếu |
| **MQTT đầu–cuối** | sensor-node publish → CoreIoT → rule-chain → shared attribute → màn hình nhận | Mốc PC: `MQTT_RX` (màn hình) − `MQTT_TX` (sensor-node) | Jitter serial vài ms (nhỏ so với trễ cloud) |
| Tuổi mẫu lúc gửi | `millis()` lúc đóng gói − lúc mẫu cảm biến được ghi | Log `age_min_ms` / `age_max_ms` | Lượng tử theo chu kỳ đo/gửi 100 ms |
| Tỷ lệ nhận | Số `seq` màn hình nhận / số `seq` đã gửi | Đếm seq | — |

Khi viết bài: độ trễ "từ đo tới màn hình nhận" ≈ tuổi mẫu + ESP-NOW một chiều; ghi rõ chưa gồm thời gian
vẽ LVGL.

## 2. Firmware

> **Dò cổng trước khi nạp/đo** — số COM đổi theo máy/cổng USB:
> `python -m serial.tools.list_ports -v` → sensor-node là `USB Serial Device` (VID:PID 303A:1001),
> màn hình là `USB-Enhanced-SERIAL CH343` (1A86:55D3). Luôn ghi rõ `--upload-port` (PlatformIO có thể tự chọn
> nhầm sensor-node khi nạp màn hình). Script đo **tự dò cổng** theo VID:PID (`auto`); chỉ định tay bằng
> `--sensor-port/--screen-port` (hoặc `--port`) nếu cần. Máy dev ngày 06/10: **sensor-node = COM7, màn hình = COM9**
> (COM4 trên máy này là cổng Bluetooth). Lệnh nạp dưới đây dùng các cổng đó.

Các env `*_latency` bật `TBS_LATENCY_PROBE=1`. **Không dùng cho bản phát hành** (màn hình phát thêm gói echo).

| Kịch bản | sensor-node (COM7) | màn hình (COM9) |
|---|---|---|
| A — chỉ ESP-NOW | `yolo_uno_latency` | `yolo_uno_latency`, **tắt hotspot/AP** để màn hình không nối Wi-Fi |
| B — Hybrid (ESP-NOW + MQTT) | `yolo_uno_coreiot_latency` | `yolo_uno_latency`, bật Wi-Fi/AP |

```bash
cd firmware/sensor-node
pio run -e yolo_uno_latency -t upload --upload-port COM7
```

```bash
cd firmware/waveshare-screen
pio run -e yolo_uno_latency -t upload --upload-port COM9
```

> **Đổi mật khẩu Wi-Fi/token trong `config/keys.json`?** Phải sinh lại header rồi build + nạp lại, nếu không firmware vẫn
> mang giá trị cũ (06/10: board thấy hotspot nhưng lỗi xác thực "Reason code 15" vì `credentials.h` cũ):
> `python tools/guard/gen_credentials.py --out firmware/sensor-node/include/credentials.h` và
> `python tools/guard/gen_credentials.py --out firmware/waveshare-screen/components/coreiot_client/include/credentials.h`.
> Hotspot Windows: băng 2,4 GHz; phát cùng kênh với Wi-Fi PC đang nối — màn hình quét mọi kênh khi chưa biết kênh AP.

Kịch bản B cần thêm: **import lại rule-chain** `cloud/coreiot/rule_chain/supersonic_rule_chain.json` lên
CoreIoT (đã thêm trường `seq`). Chưa import thì màn hình không in `MQTT_RX` và phần MQTT sẽ trống.

Kiểm tra nhanh bằng serial monitor (đóng monitor trước khi chạy script):
- Cổng sensor-node phải thấy `LAT TX seq=…` và `LAT RTT seq=… us=…` (khoảng 10 dòng/giây mỗi loại).
- Cổng màn hình phải thấy `I (…) LAT: RX seq=…`; kịch bản B thêm `LAT: MQTT_RX seq=…` mỗi 2 giây.

Đặt một vật trước ít nhất 1 cảm biến để có tuổi mẫu (không có cảm biến hợp lệ thì `age = -1`, bị bỏ qua).

## 3. Chạy đo

```bash
pip install pyserial
```

| Lượt | Lệnh | Điều kiện |
|---|---|---|
| A1 | `python tools/latency/measure_latency.py record --tag espnow_1m --duration 300` | Kịch bản A, 2 board cách 1 m |
| A2 | `python tools/latency/measure_latency.py record --tag espnow_5m --duration 300` | Kịch bản A, cách 5 m |
| B1 | `python tools/latency/measure_latency.py record --tag hybrid --duration 900` | Kịch bản B (≈ 450 bản tin MQTT) |

Cổng được tự dò theo VID:PID. Mỗi lượt ghi `data/latency/<tag>_<giờ>.csv` và in bảng
thống kê (n, min, p50, p95, p99, max, mean, std). Phân tích lại một file:

```bash
python tools/latency/measure_latency.py analyze data/latency/espnow_1m_20261007_100000.csv --json out.json
```

Ghi lại cho mỗi lượt: khoảng cách 2 board, kênh Wi-Fi, loại mạng (hotspot/router), giờ đo, vật cản giữa 2 board.

## 4. Ghi kết quả

- Dán bảng thống kê vào `docs/PROGRESS.md` (mục T5.2) và `docs/logs/LATENCY_DMXT57_LOG.md`, kèm tên file CSV.
- Comment tóm tắt lên Jira DMXT-57.
- Báo cáo **p50 và p95** (không chỉ trung bình), kèm tỷ lệ nhận.

## 5. Kiểm thử công cụ (không cần board)

```bash
python -m pytest tools/latency -q
```

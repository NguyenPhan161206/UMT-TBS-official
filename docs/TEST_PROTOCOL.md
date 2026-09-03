# TEST_PROTOCOL — Quy trình kiểm thử hệ thống (UMT-TBS V2)

> Quy trình kiểm thử chia 3 cấp: **Cấp 1** — host/unit (không cần board),
> **Cấp 2** — flash-and-observe (cần board thật), **Cấp 3** — nghiệm thu
> mạng/cloud (CoreIoT B9). Mỗi bước đều có lệnh và tiêu chí PASS/FAIL (R10).

Tiện ích: sensor node = máy đo khoảng cách (6× JSN-SR04T); waveshare-screen =
màn hình hiển thị cảnh báo (LVGL v9). Xem `README.md` để setup local trước.

---

## Cấp 1 — Host / Unit test (không cần board)

Chạy trên máy dev sau khi cấu hình secret (theo README).

```bash
# 1. Guard tools — secret sạch + keys.json hợp lệ (R1/R11)
python3 tools/guard/scan_secrets.py                    # → SECRET-SCAN OK
python3 tools/guard/gen_credentials.py --check         # → GEN-CREDENTIALS OK

# 2. Đối chiếu ngưỡng rule-chain vs thresholds.h (R3/R11)
python3 tools/guard/check_rulechain_thresholds.py       # → CHECK-RULECHAIN OK

# 3. Unit test guard tools (16 tests)
~/.venv-platformio/bin/python -m pytest tools/guard/test_guard.py -q   # → 16 passed

# 4. Host unit test firmware sensor-node (DistanceFilter, thresholds) (10 tests)
cd firmware/sensor-node && pio test -e native           # → 10/10 PASS
```

**Tiêu chí PASS:** tất cả lệnh trên exit 0, không có `FAILED`/`ERROR`.

---

## Cấp 2 — Flash-and-observe (cần board thật)

> Yêu cầu: 2 board ESP32-S3 cấp nguồn, kết nối `/dev/ttyACM0` (sensor) và
> `/dev/ttyACM1` (waveshare-screen), USB cáp dữ liệu. CMake tự nạp driver
> RGB/GT911 qua `idf_component.yml`.

### 2.1 Sensor node
```bash
cd firmware/sensor-node
pio run -e yolo_uno                              # build (ESP-NOW only)
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM0
pio device monitor -p /dev/ttyACM0 -b 115200
```
**Quan sát:** serial in `Supersonic sensor array started (6 cam bien)`; mỗi 100 ms
in dòng `[S0..S5]` với `Pulse`, `Raw`, `Stable`. Đưa tay trước từng cảm biến:
`Stable` phải giảm tương ứng và ổn định sau ~5 mẫu (lọc cluster-EMA).

### 2.2 Waveshare screen
```bash
cd firmware/waveshare-screen
pio run -e yolo_uno
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1
pio device monitor -p /dev/ttyACM1 -b 115200
```
**Quan sát:** screen sáng, title "Collision-Avoidance Dashboard"; banner
`OVERALL`. Khi 2 board cùng bật, dòng `ESP-NOW frame rssi=... dBm` xuất hiện
(khi sensor gửi) và nhãn `ESP-NOW: LINKED`.

### 2.3 End-to-end cảnh báo (2 board)
Đặt vật cản ở khoảng cách **gần/thân/khối** khác nhau:
- **~80–100 cm** → `OVERALL: CAUTION` (vàng `0xFFD600`).
- **< 30 cm** → `OVERALL: DANGER` (đỏ `0xFF1744`) + buzzer kêu
  (sensor node, chế độ WARNING 3 s / DANGER 1 s tuỳ < 50 cm / < 20 cm).
- **> 100 cm / không vật** → `OVERALL: SAFE`.

**Tiêu chí PASS:** banner đổi đúng zone theo khoảng cách; NOT mặc định `--` cho
slot không lắp phần cứng (giữ `valid=0`).

---

## Cấp 3 — Nghiệm thu Mạng/Cloud (CoreIoT B9, cần token + Internet)

### 3.1 Chuẩn bị token CoreIoT (R11 — token MỚI, không tái dùng repo cũ)

> `https://app.coreiot.io` là nền tảng **CoreIoT** (fork ThingsBoard). Trang chủ
> sau đăng nhập là `/home` (dashboard tổng) — **không phải** nơi tạo token.
> Token thiết bị được tạo trong menu **Devices** (`/devices`).

Từng bước:

1. **Đăng nhập** `https://app.coreiot.io` — dùng Google/GitHub/Apple hoặc email
   đăng ký tài khoản.
2. Menu trái chọn **Devices** (URL `…/devices`) — danh sách terminal
   thiết bị.
3. Bấm nút **+** (góc phải trên bảng) để thêm device mới:
   - Tên: `sensor-node` → bật **"Is device"** → lưu.
   - Lặp lại tạo `waveshare-screen`.
4. Với **mỗi device**, bấm vào tên để mở chi tiết, rồi nút biểu tượng **máy
   chìa khóa / Manage Credentials**:
   - Chọn tab **Access Token** → nút **generate** (hoặc sao chép token hiện có).
   - Đây là **Device Access Token** MỚI — copy cẩn thận, chưa bao giờ commit.
5. Điền token vào `config/keys.json` (`SENSOR_NODE_DEVICE_TOKEN` /
   `WAVESHARE_SCREEN_DEVICE_TOKEN`) + Wi-Fi SSID/password.
6. Sinh header firmware + verify:

```bash
python3 tools/guard/gen_credentials.py --out firmware/sensor-node/include/credentials.h
python3 tools/guard/gen_credentials.py --out firmware/waveshare-screen/components/coreiot_client/include/credentials.h
python3 tools/guard/gen_credentials.py --check
```

7. Import rule-chain vào CoreIoT (nếu chưa):
   - Menu trái **Rule Chains** (URL `…/ruleChains`) → **+** → **Import**.
   - Chọn `cloud/coreiot/rule_chain/supersonic_rule_chain.json` → lưu.
   - Vào device `sensor-node` → **Manage Credentials / Rule Chain** (hoặc tab
     liên quan) → chọn rule-chain vừa import làm **root** cho device này.

> LƯU Ý: chạy CoreIoT cần env build có cờ:
> `pio run -e yolo_uno_coreiot` (sensor-node). Waveshare-screen dùng MQTT qua
> CoreIoT client ở mức mặc định.

### 3.2 Thử gửi telemetry (tool V2)
```bash
pip install -r tools/requirements.txt     # cài paho-mqtt

# Không cần token — chỉ in payload mẫu
python3 tools/test_mqtt_coreiot.py --dry-run --distance 25

# Gửi thật lên broker (lấy token từ config/keys.json)
python3 tools/test_mqtt_coreiot.py --distance 15.5    # → warning_status = DANGER
python3 tools/test_mqtt_coreiot.py --distance 60      # → CAUTION
python3 tools/test_mqtt_coreiot.py --distance 150     # → NORMAL
python3 tools/test_mqtt_coreiot.py --loop --interval 2   # diễn biến liên tục
```

### 3.3 Kiểm tra dashboard
- Mở device `waveshare-screen` trên console → **Latest telemetry / Shared
  attributes**.
- **Tiêu chí PASS:** `warning_status` đổi tương ứng theo distance gửi vào;
  `nearest_cm`/`d1..d6` hiển thị đúng; sau khi gửi `--loop`, dashboard cập nhật
  liên tục.

---

## So sánh tiêu chí ngưỡng (R3 — single source)

| Zone | Ngưỡng (cm) | Giá trị shared |
|---|---|---|
| SAFE | `> 100` | `NORMAL` |
| CAUTION | `<= 100` và `> 30` | `CAUTION` |
| DANGER | `<= 30` | `DANGER` |

Nguồn duy nhất: `firmware/shared/thresholds.h` (`SENSOR_CAUTION_CM`,
`SENSOR_DANGER_CM`). Rule-chain là snapshot khớp thông qua
`check_rulechain_thresholds.py` (Cấp 1).

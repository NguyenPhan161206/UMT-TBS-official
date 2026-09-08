# Hướng dẫn lắp đặt thiết bị phần cứng — Truck Blind-spot Warning System (V2)

> Tài liệu này mô tả chi tiết từng bước lắp đặt phần cứng cho hệ thống
> **Cảnh báo va chạm xe tải (Hybrid V2)**: 2 board **ESP32-S3**, 6 cảm biến
> **JSN-SR04T**, màn hình **Waveshare 7" RGB LCD**, còi **buzzer**.
>
> Nguồn tham chiếu: `firmware/shared/thresholds.h` (GPIO map, ngưỡng), `firmware/shared/espnow_protocol.h`
> (kênh ESP-NOW), `README.md`, `report/chapters/04_he_thong.tex`.

---

## 1. Danh mục thiết bị (Bill of Materials)

| # | Thiết bị | Số lượng | Ghi chú |
|---|----------|:--------:|---------|
| 1 | Board **ESP32-S3** (Yolo_Uno_S3 — Dual-Core 240 MHz, Wi-Fi/BLE, 8 MB PSRAM) | 2 | 1 board **sensor-node**, 1 board **waveshare-screen** |
| 2 | Cảm biến siêu âm chống nước **JSN-SR04T** | 6 | Dải đo 20–600 cm (`SENSOR_RANGE_MIN/MAX_CM`), FOV 75° |
| 3 | Màn hình **Waveshare 7" RGB Touch (800×480, GT911)** | 1 | Gắn trong cabin, kèm board waveshare-screen |
| 4 | Còi **buzzer** 5V | 1 | Kết nối **GPIO11** trên sensor-node |
| 5 | Nguồn 5V (cổng USB / Bộ đổi nguồn 12V→5V) | 2 | sensor-node ≥ 1 A; screen ≥ 2 A |
| 6 | Dây dupont / cáp cắm, dây nối dài cho cảm biến | — | Chạy từ sensor-node ra 6 vị trí quanh xe |
| 7 | Cầu phân áp Echo 1kΩ + 2kΩ (khuyến nghị) | 6 | Bảo vệ chân GPIO khỏi mức 5V của Echo |
| 8 | Máy tính (Windows/Linux) để flash + giám sát | 1 | PlatformIO; cổng `/dev/ttyACM*` (Linux) / COM (Windows) |
| 9 | Hotspot Wi-Fi (điện thoại) | 1 | SSID vd. `iPhone`, cấu hình ở mục 6 |

> **Lưu ý pin:** board ESP32-S3 có PSRAM nhúng trên **GPIO47/48** và flash SPI0 trên
> **GPIO26–32**, USB trên **GPIO19/20** — những chân này **không được dùng** làm chân cảm biến.

---

## 2. Sơ đồ khối hệ thống

```text
 6× JSN-SR04T (quanh xe)
 ┌────────────┬───────┬──────┬────────┬──────┬─────┐
 │ FRONT      │L.FRONT│R.FRONT│L.REAR  │R.REAR│REAR │
 └─────┬──────┴───┬───┴──┬───┴───┬────┴──┬───┴──┬──┘
       │ (Trig/Echo — 6 cặp GPIO, xem mục 4)
       ▼
 ┌───────────────────────────┐
 │ ESP32-S3  SENSOR-NODE     │  Buzzer (GPIO11) — còi cục bộ
 ├── ESP-NOW ──(chính, kênh 6)──▶ Waveshare Screen
 └── MQTT ────(phụ, CoreIoT)───▶ app.coreiot.io:1883
             ▼
 ┌───────────────────────────┐
 │ ESP32-S3  WAVESHARE-SCREEN│  7" RGB LVGL v9 + GT911 touch
 └───────────────────────────┘
```

Đường **chính** = ESP-NOW (độ trễ thấp, không cần Internet) → màn hình cảnh báo ngay.
Đường **phụ** = MQTT/CoreIoT → giám sát từ xa khi có Wi-Fi.

---

## 3. Chuẩn bị

1. **Cấp nguồn riêng** cho 2 board: dùng nguồn 5V ổn định (không cấp nguồn qua cổng debug khi chạy thật).
2. Cắm cảm biến vào đúng vị trí **trước khi** cấp nguồn board.
3. Đảm bảo **chung GND** giữa: sensor-node — từng cảm biến — buzzer.
4. Gắn buzzer và màn hình theo hướng dẫn ở mục 5, 7.
5. Cấu hình Wi-Fi + token (mục 6) rồi flash firmware (mục 8).

---

## 4. Bảng đấu dây GPIO (sensor-node)

Map dưới đây lấy **duy nhất** từ `firmware/shared/thresholds.h::SENSOR_PINS`. Đấu sai chân
⇒ sai nhãn hiển thị trên màn hình hoặc không đo được.

| STT cảm biến | Vị trí trên xe | Chân **Trig** (GPIO) | Chân **Echo** (GPIO) | Slot ESP-NOW |
|:---:|---|---|---:|---:|---|
| S0 | **FRONT** (đầu xe) | GPIO5  | GPIO6  | FRONT |
| S1 | **LEFT_FRONT** (trái-trước) | GPIO7  | GPIO8  | LEFT_FRONT |
| S2 | **RIGHT_FRONT** (phải-trước) | GPIO9  | GPIO10 | RIGHT_FRONT |
| S3 | **LEFT_REAR** (trái-sau) | GPIO17 | GPIO18 | LEFT_REAR |
| S4 | **RIGHT_REAR** (phải-sau) | GPIO21 | GPIO38 | RIGHT_REAR |
| S5 | **REAR** (đuôi xe) | GPIO3  | GPIO4  | REAR |

### 4.1 Đấu từng cảm biến JSN-SR04T

```
 JSN-SR04T                    ESP32-S3 (sensor-node)
 ──────────                   ─────────────────────
  VCC  (đỏ)        ───────►   5V  (nguồn chung)
  GND  (đen)       ───────►   GND (chung)
  TRIG (vàng)      ───────►   GPIO Trig  (kích đo: xung 20 µs)
  ECHO (trắng)     ───────►   [Phân áp] → GPIO Echo   (xem 4.2)
```

> **JSN-SR04T là thiết bị mức 5V.** Chân TRIG nhận xung 3.3V từ ESP32-S3 (OK đa số module).
> Chân **ECHO trả về mức 5V** — ESP32-S3 **không chịu 5V**, nên bắt buộc hạ áp trước GPIO.

### 4.2 Cầu phân áp Echo (khuyến nghị 5V → ~3.3V)

```
 Echo (5V) ──┬─[ R1 = 1kΩ ]──┬── GPIO Echo
             │               │
             │          [ R2 = 2kΩ ]
             │               │
            GND─────────────┴── GND
```

Điện áp tại GPIO ≈ `5V × R2/(R1+R2) = 5V × 2/3 ≈ 3.33V`. Lắp **trên từng chân Echo** của cả 6 cảm biến.

### 4.3 Buzzer

| Chân buzzer | Kết nối |
|---|---|
| `+` | GPIO11 (qua transistor/driver nếu buzzer ≥ 100 mA) |
| `−` | GND |

> Buzzer kêu cục bộ theo ngưỡng: **WARNING < 50 cm** (chu kỳ 3 s), **DANGER < 20 cm** (chu kỳ 1 s)
> — nguồn duy nhất `firmware/shared/thresholds.h`.

---

## 5. Lắp đặt cảm biến quanh xe

| Vị trí | Đặc điểm | Gợi ý độ cao / góc |
|---|---|:---:|
| **FRONT** | Cách mặt đất ~40–60 cm, hướng thẳng về phía trước | 0° (song song mặt đường) |
| **LEFT_FRONT** | Góc trái trước, che điểm mù gương trái | nghiêng ngoài ~30° |
| **RIGHT_FRONT** | Góc phải trước, che điểm mù gương phải | nghiêng ngoài ~30° |
| **LEFT_REAR** | Góc trái sau, che điểm mù sau trái | nghiêng ra ngoài ~30° |
| **RIGHT_REAR** | Góc phải sau | nghiêng ra ngoài ~30° |
| **REAR** | Đuôi xe, hướng thẳng ra sau | 0° |

Quy tắc khi lắp:
- **Bề mặt cảm nhận không bị che** (keo, khung, bùn bám).
- Cảnh báo **không đặt sát nhau** (nhiễu chéo): ≥ 20 cm giữa các đầu dò.
- Vùng quét FOV 75° phải **hướng ra không gian xe đang di chuyển**, không hướng vào thùng xe.
- Dây nối dài: dùng cáp xoắn/được bọc, đi xa nguồn nhiễu (động cơ, đèn HID/LED).

---

## 6. Cấu hình mạng (Wi-Fi + CoreIoT)

Hệ thống V2 dùng **1 radio** trên 2 board ⇒ cả 2 board phải ở **cùng channel**.

| Tham số | Giá trị đang dùng |
|---|---|
| Hotspot Wi-Fi | SSID `iPhone` (ví dụ), pass theo cấu hình trong `config/keys.json` |
| Channel WiFi của hotspot | **6** (khớp `ESPNOW_CHANNEL` trong `firmware/shared/espnow_protocol.h`) |
| ESP-NOW | Broadcast, `ESPNOW_SEND_INTERVAL_MS = 500` ms, `ESPNOW_LINK_TIMEOUT_MS = 1500` ms |
| MQTT broker | `app.coreiot.io:1883`, topic `v1/devices/me/telemetry` |

> **Quan trọng:** nếu hotspot đổi channel khác 6, phải cập nhật `#define ESPNOW_CHANNEL`
> trong `firmware/shared/espnow_protocol.h` và rebuild cả 2 firmware, rồi flame lại.
> Mạng không cho ra 1883 (firewall) ⇒ MQTT phụ sẽ không lên — ESP-NOW vẫn hoạt động độc lập.

### Cách kiểm tra channel thực tế
Sau khi wavehare-screen kết nối, đọc serial (115200) sẽ thấy:
```
I (XXXX) coreiot_client: Wi-Fi Connected Successfully! IP Address: 172.20.10.6
I (XXXX) coreiot_client: Wi-Fi channel primary=6 secondary=0
```

---

## 7. Lắp đặt màn hình waveshare-screen

1. Gắn board waveshare-screen vào mặt sau màn hình 7" (connector RGB FPC + I2C cảm ứng GT911).
2. Gắn màn hình **trong cabin**, vị trí tài xế quan sát được, tránh chắn tầm nhìn.
3. Nguồn: **5V / ≥ 2 A** (màn RGB + LVGL + PSRAM ngốn dòng; nguồn yếu ⇒ brownout/reboot).
4. Đảm bảo anten Wi-Fi của board không bị che bởi khung sắt.

---

## 8. Flash firmware & kiểm thử nhanh

```bash
# 1) Cấu hình secret (chỉ làm 1 lần)
cp config/keys.template.json config/keys.json   # điền token CoreIoT + Wi-Fi thật

# 2) Sinh credentials.h (file gitignored)
python3 tools/guard/gen_credentials.py --out firmware/sensor-node/include/credentials.h
python3 tools/guard/gen_credentials.py --out firmware/waveshare-screen/components/coreiot_client/include/credentials.h

# 3) Build
cd firmware/sensor-node && pio run -e yolo_uno                   # sensor-node (ESP-NOW)
cd ../waveshare-screen && pio run -e yolo_uno                    # waveshare-screen

# 4) Flash (cổng phát hiện tại)
cd firmware/sensor-node && pio run -e yolo_uno -t upload --upload-port <PORT>
cd ../waveshare-screen && pio run -e yolo_uno -t upload --upload-port <PORT>

# 5) Theo dõi
pio device monitor -p <PORT> -b 115200
```

**Checklist nghiệm thu nhanh (boot):**

| Kiểm tra | Kết quả mong đợi trên serial |
|---|---|
| Sensor-node bật | `Supersonic sensor array started (6 cam bien)`; `[ESPNOW] Send OK` mỗi 500 ms |
| Screen bật + WiFi | `Wi-Fi Connected Successfully! IP ...`; `Wi-Fi channel primary=6` |
| MQTT lên | `MQTT Connected to CoreIoT (mqtt://app.coreiot.io:1883)` |
| ESP-NOW nhận | Screen log `ESP-NOW frame rssi=...` |
| Ép tay vật vào cảm biến | Màn hình cập nhật khoảng cách; buzzer kêu theo vùng |

---

## 9. Đấu nối sai thường gặp & cách xử lý

| Triệu chứng | Nguyên nhân có thể | Cách xử lý |
|---|---|---|
| Màn hình không sáng / reboot vòng | Nguồn yếu cho screen | Dùng nguồn 5V ≥ 2 A |
| `[Sx] REJECT: Khong nhan duoc Echo hop le` | Đấu sai Trig/Echo; Echo 5V làm hỏng/không ổn GPIO; không chung GND | Soi bảng chân mục 4; thêm phân áp Echo; nối GND |
| Cảm biến chỉ báo `--` | Cảm biến chưa lắp/valid=0; nguồn cảm biến thiếu | Cấp nguồn 5V cho JSN-SR04T; kiểm tra dây |
| ESP-NOW nhấp nháy ON/OFF | 2 board lệch channel WiFi | Căn `ESPNOW_CHANNEL` = channel hotspot (mục 6), rebuild + flash cả 2 |
| MQTT luôn DOWN | Sai SSID/pass; firewall chặn 1883 | Kiểm tra `credentials.h` đã sinh đúng; thử mạng khác |
| Buzzer không kêu | Đấu sai cực; cần driver | Đổi cực; dùng transistor khi buzzer ngốn dòng |

---

## 10. An toàn & bảo trì

- **Chống nước:** JSN-SR04T chống nước nhưng **đầu nối/giắc không** — bịt kín mối nối ngoài trời.
- **Cách ly điện cao áp:** không đi dây cảm biến chung ống với dây điện ô tô.
- **Chung GND, không chung nguồn cảm ứng:** cấp nguồn riêng cho màn hình.
- Bảo trì định kỳ: vệ sinh bề mặt cảm biến, kiểm tra giắc sau khi rửa xe.
- Mọi thay đổi GPIO/ngưỡng chỉ được sửa tại `firmware/shared/` (R2/R3/R4) — **không** sửa rải rác.

---

*Tài liệu song song: `docs/TEST_PROTOCOL.md` (quy trình kiểm thử 3 cấp), `docs/PROGRESS.md` (tiến độ).*
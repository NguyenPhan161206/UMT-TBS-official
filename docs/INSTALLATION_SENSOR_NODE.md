# Hướng dẫn lắp đặt Sensor-node + 6 cảm biến JSN-SR04T

> Tài liệu chuyên về **bo mạch sensor-node** (ESP32-S3) và **6 cảm biến siêu âm chống nước
> JSN-SR04T**. Dành cho người lắp ráp phần cứng. Mọi GPIO/ngưỡng lấy nguồn duy nhất từ
> `firmware/shared/thresholds.h` (R2/R3, không đoán giá trị).

## Mục lục

1. Chuẩn bị: BOM, công cụ, an toàn
2. Đấu dây GPIO (6 cảm biến + buzzer)
3. Nguồn cấp & cầu phân áp Echo
4. Lắp cơ khí 6 cảm biến quanh xe
5. Đi dây & chống nước
6. Flash firmware & kiểm tra nhanh bằng serial
7. Xử lý sự cố & nghiệm thu

---

## 1. Chuẩn bị: BOM, công cụ, an toàn

### 1.1 Bill of Materials (BOM)

| # | Linh kiện | Số lượng | Ghi chú |
|---|-----------|:---:|---------|
| 1 | Board **ESP32-S3** sensor-node (Yolo_Uno_S3, 8 MB PSRAM) | 1 | Board đo 6 cảm biến, còi cục bộ |
| 2 | Cảm biến **JSN-SR04T** chống nước | 6 | Dải đo 20–600 cm, FOV 75° |
| 3 | Buzzer 5V | 1 | Kêu cảnh báo theo vùng, nối **GPIO11** |
| 4 | Nguồn 5V ≥ 1 A | 1 | Cấp riêng cho board + cảm biến |
| 5 | Điện trở 1kΩ | 6 | Cầu phân áp cho chân Echo |
| 6 | Điện trở 2kΩ | 6 | Cầu phân áp cho chân Echo |
| 7 | Dây dupont / dây rút, giắc cắm | — | Đấu chân cảm biến ~board |
| 8 | Co nhiệt / keo chống ẩm | — | Bịt kín mối nối ngoài trời |

### 1.2 Công cụ

- Máy tính (Windows/Linux) cài PlatformIO để build/flash/monitor.
- Đồng hồ vạn năng (kiểm tra nguồn, thông mạch).
- Kìm, tua vít, băng keo, co nhiệt.

### 1.3 Cảnh báo an toàn

- **Ngắt nguồn** trước khi đấu/ngắt dây.
- Không đấu chân Echo 5V trực tiếp vào GPIO — nguy cơ hỏng ESP32-S3 (xem mục 3).
- Kiểm tra chính xác **chân Trig/Echo theo bảng GPIO** trước khi cấp nguồn.
- Đảm bảo **chung GND** giữa board, cảm biến và buzzer.

---

## 2. Đấu dây GPIO (6 cảm biến + buzzer)

Bảng dưới lấy **chính xác** từ `firmware/shared/thresholds.h::SENSOR_PINS`. Đấu sai chân
⇒ sai nhãn trên màn hình hoặc không đo được.

| STT | Vị trí | Chân Trig | Chân Echo | Slot ESP-NOW |
|:---:|--------|:---:|:---:|:---:|
| S0 | FRONT (đầu xe)        | GPIO5  | GPIO6  | FRONT |
| S1 | LEFT_FRONT (trái trước) | GPIO7 | GPIO8  | LEFT_FRONT |
| S2 | RIGHT_FRONT (phải trước)| GPIO9 | GPIO10 | RIGHT_FRONT |
| S3 | LEFT_REAR (trái sau)    | GPIO17| GPIO18 | LEFT_REAR |
| S4 | RIGHT_REAR (phải sau)   | GPIO21| GPIO38 | RIGHT_REAR |
| S5 | REAR (đuôi xe)          | GPIO3 | GPIO4  | REAR |

### 2.1 Buzzer

| Chân buzzer | Kết nối |
|:---:|:---:|
| `+` | **GPIO11** (qua transistor/driver nếu buzzer ≥ 100 mA) |
| `−` | GND |

> Ngưỡng còi nguồn duy nhất ở `firmware/shared/thresholds.h`: WARNING < 50 cm (chu kỳ 3 s),
> DANGER < 20 cm (chu kỳ 1 s).

### 2.2 GPIO bị chiếm dụng — KHÔNG dùng

Các chân sau **không được dùng** làm GPIO cảm biến (đã chặn ở runtime):

- **GPIO47 / GPIO48** — Octal PSRAM 8 MB (SPICLK_P/N_DIFF).
- GPIO26 – GPIO32 — SPI0 flash/PSRAM.
- GPIO19 / GPIO20 — USB D-/D+ (native USB CDC đang dùng serial).

---

## 3. Nguồn cấp & cầu phân áp Echo

### 3.1 Nguồn

- Cấp **VCC 5V** riêng cho từng cảm biến JSN-SR04T.
- Nguồn 5V dùng chung (≥ 1 A) cho board + cảm biến, đảm bảo **CHUNG GND** với board.
- Không cấp nguồn cảm biến từ chân 3.3V của board (JSN-SR04T cần 5V).

### 3.2 Cầu phân áp Echo (5V → ~3.33V)

JSN-SR04T là thiết bị **mức 5V**: chân Echo trả xung 5V. ESP32-S3 **không chịu 5V**,
nên PHẢI hạ áp trước khi vào GPIO bằng cầu phân áp (lắp trên cả 6 chân Echo):

```text
 Echo (5V) ──┬────[ R1 = 1kΩ ]────┬── GPIO Echo
             │                    │
             │               [ R2 = 2kΩ ]
             │                    │
            GND───────────────────┴──── GND
```

Điện áp tại GPIO ≈ `5V × R2/(R1+R2) = 5V × 2/3 ≈ 3.33 V`.

> **Cảnh báo:** đấu Echo trực tiếp vào GPIO có thể làm **hỏng vĩnh viễn** chân ESP32-S3.

---

## 4. Lắp cơ khí 6 cảm biến quanh xe

| Vị trí | Độ cao gợi ý so với mặt đất | Góc lắp |
|--------|:---:|:---:|
| **FRONT** | 40–70 cm | **0°** (song song mặt đường, hướng về trước) |
| **LEFT_FRONT** | 40–70 cm | Nghiêng ngoài ~**30°** (che điểm mù gương trái) |
| **RIGHT_FRONT** | 40–70 cm | Nghiêng ngoài ~**30°** (che điểm mù gương phải) |
| **LEFT_REAR** | 40–70 cm | Nghiêng ngoài ~**30°** (che điểm mù sau trái) |
| **RIGHT_REAR** | 40–70 cm | Nghiêng ngoài ~**30°** (che điểm mù sau phải) |
| **REAR** | 40–70 cm | **0°** (song song, hướng thẳng ra sau) |

Quy tắc:

- Vùng quét **FOV 75°** phải hướng ra không gian xe di chuyển, **không** hướng vào thùng xe.
- Giữ **≥ 20 cm** giữa hai đầu dò gần nhau để tránh nhiễu chéo.
- Bề mặt cảm nhận không bị khung/keo/bùn che.

---

## 5. Đi dây & chống nước

- Dùng **cáp xoắn / được bọc**, càng ngắn càng tốt, đi xa nguồn nhiễu (động cơ, đèn).
- Cố định dây chắc chắn, tránh cọ xát gây đứt ngầm.
- **JSN-SR04T thân chống nước nhưng đầu giắc và mối nối KHÔNG chống nước**:
  - Bịt mối nối bằng **co nhiệt**, băng keo chống ẩm hoặc hộp đấu dây kín.
  - Không để nước chảy dọc thân cáp vào đầu giắc.

---

## 6. Flash firmware & kiểm tra nhanh bằng serial

### 6.1 Build & flash (theo AGENTS.md)

```bash
cd firmware/sensor-node
pio run -e yolo_uno                                          # build bản ESP-NOW
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM0     # flash (thay cổng cho đúng)
pio device monitor -p /dev/ttyACM0 -b 115200                 # xem log
```

> Trên Windows dùng cổng COM tương ứng; bản CoreIoT dùng env `yolo_uno_coreiot`.

### 6.2 Log khởi động mong đợi

```
========================================
Supersonic sensor array started (6 cam bien)
  [S0] Trig=GPIO5 Echo=GPIO6 -> ESP-NOW slot 0
  [S1] Trig=GPIO7 Echo=GPIO8 -> ESP-NOW slot 1
  ...
========================================
[ESPNOW] Send OK
```

- Mỗi 500 ms thấy `[ESPNOW] Send OK` ⇒ đường chính ESP-NOW hoạt động.
- Chưa thấy giá trị ⇒ **không phải lỗi**: cảm biến cần ít mẫu để lọc trước khi hợp lệ,
  hoặc có chuỗi `[Sx] REJECT` (mục 7).

---

## 7. Xử lý sự cố & nghiệm thu

### 7.1 Bảng troubleshooting

| Triệu chứng | Nguyên nhân có thể | Cách xử lý |
|---|---------|---------|
| Không nhận Echo / `[Sx] REJECT` | Đấu sai Trig/Echo; quên CHUNG GND; Echo 5V làm GPIO không ổn | Soi bảng mục 2; thêm phân áp mục 3; nối GND chung |
| ESP-NOW nhấp nháy ON/OFF | Lệch WiFi channel giữa 2 board | Căn `ESPNOW_CHANNEL` = channel của AP (khớp `firmware/shared/espnow_protocol.h`), rebuild + flash cả 2 |
| Reboot vòng / nguồn yếu | Nguồn không đủ dòng | Dùng nguồn 5V ≥ 1 A, tách nguồn cấp cảm biến |
| Buzzer không kêu | Đấu sai cực; cần driver khi buzzer ngốn dòng | Đổi cực; thêm transistor |
| Cảm biến chỉ báo `--` | Cảm biến chưa lắp / valid=0; mất nguồn 5V cho cảm biến | Kiểm tra nguồn cảm biến; ép tay vật vào để có giá trị |
| MQTT luôn DOWN (bản CoreIoT) | Sai SSID/pass; firewall chặn cổng 1883 | Kiểm tra `credentials.h` đã sinh đúng; thử mạng khác |

### 7.2 Nghiệm thu ép thử từng cảm biến

1. Bật nguồn board + mở monitor `pio device monitor`.
2. Với **từng** vị trí (S0→S5): đưa tay/vật vào trước đầu dò ở khoảng 30–60 cm.
3. Trên serial thấy giá trị giảm về vùng CAUTION/DANGER và không còn `REJECT`.
4. Trên waveshare-screen: đúng slot tương ứng (FRONT/LEFT_FRONT/.../REAR) cập nhật giá trị.
5. Buzzer kêu theo vùng (WARNING < 50 cm / DANGER < 20 cm).

### 7.3 Tài liệu liên quan

- Mô tả toàn hệ thống + màn hình: [`docs/HARDWARE_INSTALLATION.md`](HARDWARE_INSTALLATION.md)
- Quy trình kiểm thử 3 cấp: [`docs/TEST_PROTOCOL.md`](TEST_PROTOCOL.md)
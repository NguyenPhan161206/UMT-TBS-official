# WAVESHARE_SCREEN_BACKLIGHT_CH422G_LOG.md

> **Mục tiêu:** Làm màn hình Waveshare 7" lên UI trên board thật (vấn đề cấp cao nhất hiện tại).
> Trạng thái: **KẾ HOẠCH — chưa thực thi.** Đây là nhiệm vụ việc tiếp theo khi mở lại session.
> Ngày tạo: 2026-09-03.

---

## 1. Bối cảnh & bài học (đọc kỹ trước khi làm)

### Tóm tắt vấn đề
- Board waveshare (ttyACM1, `ESP32-S3-Touch-LCD-7 v1.1`) **màn tối/xanh đen** dù UI (LVGL), Wi-Fi,
  CoreIoT (MQTT) đều khởi động và kết nối OK.
- Nguyên nhân nghi ngờ ban đầu là **CH422G hỏng phần cứng** → ĐÃ BÁC BỎ.

### Bằng chứng "board KHÔNG hỏng" (quan trọng!)
- **Repo của người lạ** `paulhamsh/Waveshare-ESP32-S3-LCD-7-LVGL` (local `/home/binhnguyen/Waveshare-ESP32-S3-LCD-7-LVGL`)
  → **lên UI được trên CÙNG đúng màn hình này** (user xác nhận "cùng 1 màn hình").
- Repo đó **KHÔNG đụng CH422G** cho backlight (backlight GPIO comment bỏ; không có `0x38`/`0x24`/CH422G).
  → nghĩa là **backlight tự sáng nếu không bị tác động** (CH422G EXIO2/DISP mặc định active).
- RGB panel pins + timing của V2 **giống hệt** repo paulhamsh (D0..D15 = 14,38,18,17,10,39,0,45,48,47,21,1,2,42,41,40; PCLK=7, HSYNC=46, VSYNC=3, DE=5; 16MHz) → phần LCD không phải vấn đề.
- Wiki Waveshare xác nhận: **backlight enable = CH422G EXIO2 → DISP**. Backlight không phải HW-on không-cần-chip, mà đi qua CH422G (EXIO2 mặc định HIGH lúc power-up).

### Kết luận
- **Vấn đề là PHẦN MỀM V2 phụ thuộc CH422G sai cách**, không phải board hỏng.
- V2 gọi `ch422g_init_for_output()` (0x24), `i2c_master_write_to_device_ng(0x38, 0x1E)` → CH422G NACK (0x108)
  → trong bản tolerate vẫn `i2c_master_init_if_needed()` mở bus I2C rồi fail → backlight không bật đúng → màn tối.

### ⚠️ Bài học an toàn (rất quan trọng)
- **KHÔNG quét probe nhiều cặp chân I2C.** Đã 2 lần làm **mất USB board** (ttyACM re-enumerate).
- Chỉ probe **1 cặp chân 8/9** nếu cần, 1-pass, tránh lặp nhiều cặp.

---

## 2. Nơi cần sửa khi tiếp tục

### Files
1. `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.c`
2. `firmware/waveshare-screen/src/main.c`

### Cấu trúc BSP hiện tại (`waveshare_rgb_lcd_port.c`)
- `i2c_master_init_if_needed()`: bus I2C_NUM_0, SDA=8/SCL=9 (I2C_MASTER_SDA_IO/SCL_IO = GPIO8/9).
- `waveshare_esp32_s3_touch_reset()`: ghi CH422G 0x24(init) + GT911 0x38 0x2C/0x2E, dùng GPIO4 (EXAMPLE_TOUCH_RESET_GPIO).
- `waveshare_rgb_lcd_backlight_on()`: init I2C + `ch422g_init_for_output()` (0x24<-0x01) + ghi `0x38<-0x1E`.
- `waveshare_esp32_s3_rgb_lcd_init()`: tạo RGB panel; nếu GT911 I2C fail → `*touch_handle=NULL` continue.
- Header `waveshare_rgb_lcd_port.h`: SDA=8/SCL=9, CH422G addr 0x24, backlight 0x38, GT911 reset GPIO4.

Main.c (hiện tolerate): `waveshare_rgb_lcd_backlight_on()` fail → logWARN "Backlight over CH422G failed...".

---

## 3. KẾ HOẠCH THỰC THI (thứ tự — làm từng bước, xác nhận màn hình)

### Bước 1 — Thực nghiệm quyết định: VÔ HIỆU HOÁ hoàn toàn CH422G
Sửa `waveshare_rgb_lcd_port.c`:
- `waveshare_rgb_lcd_backlight_on()`:
  - BỎ `i2c_master_init_if_needed()`, `ch422g_init_for_output()`, `i2c_master_write_to_device_ng(0x38, 0x1E)`.
  - Thay bằng: `ESP_LOGI(TAG, "backlight: để mặc định HW (CH422G EXIO2/DISP high) — không cần I2C"); return ESP_OK;`
  - (có thể bọc sau macro `CONFIG_..._BACKLIGHT_BYPASS_CH422G` để R5 rõ ràng).
- `waveshare_esp32_s3_touch_reset()`:
  - BỎ ghi CH422G 0x24 và ghi 0x38 0x2C/0x2E.
  - GIỮ `touch_reset_gpio_init()` + `gpio_set_level(GPIO4, LOW/HIGH ...)` (reset GT911 = GPIO4, rẻ).
- Build + flash waveshare (ttyACM1): `pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1`.
- **Quan sát màn:**
  - SÁNG + UI lên → ✅ **chốt: bỏ CH422G path là đủ**, vấn đề giải quyết. Sang Bước 3.
  - VẪN TỐI → nghĩa board cần CH422G ACK để KÉO/giữ EXIO2 high sau boot → sang Bước 2.

### Bước 2 — (chỉ nếu Bước 1 vẫn tối) Tìm địa chỉ CH422G đúng
- CH422G 7-bit addr nằm băng `0x20-0x27` và `0x30-0x3f` (xác nhận Wiki). V2 dùng 0x24 — có thể SAI addr cho lô board này.
- Probe **CHỈ 1 cặp 8/9** (an toàn, 1-pass) quét `0x20..0x2f` (+ có thể 0x30..0x3f) → tìm addr CH422G thật ACK.
- Đổi addr trong `ch422g_init_for_output()` (và backlight 0x38) theo addr tìm được → backlight bật.

### Bước 3 — Sau khi màn lên: nghiệm thu & hoàn thiện
- Xác nhận UI hiển thị + Wi-Fi/CoreIoT (đã connect, IP 10.0.11.237).
- Touch GT911 addr 0x14 trên 8/9: kiểm tra hoạt động (repo lạ/Makerfabs dùng 0x14). Đảm bảo reset bằng GPIO4.
- Nghiệm thu 6 sensor (sensor-node đã Wi-Fi + MQTT OK).
- **Revert DEBUG DEMO**: sau khi backlight OK, khôi phục `ESP_ERROR_CHECK` bản gốc (bỏ tolerate) HOẶC giữ bỏ-CH422G (nếu bỏ hoàn toàn cần thiết). Chọn theo hướng "production chuẩn".
- Cập nhật docs/PROGRESS.md + ROADMAP_CHECKLIST.md + THÁC này (mark hoàn thành).
- Chạy `/verify` (scan_secrets, selfcheck) trước commit.

### Files thay đổi khi sửa Bước 1
- `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.c`
- `firmware/waveshare-screen/src/main.c`

---

## 4. Lệnh build/flash/monitor

```bash
cd firmware/waveshare-screen
pio run -e yolo_uno                                          # build
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1      # flash waveshare
pio device monitor -p /dev/ttyACM1 -b 115200                  # log
```
Xem log qua: USB-Serial-JTAG console (production waveshare đọc qua ttyACM1).

---

## 5. Ghi chú liên quan (session này đã làm)
- Wi-Fi mới (SSID "Bamos Coffee 2G"; pass chỉ trong `config/keys.json`/`credentials.h`, gitignored) đã regen credentials cho cả 2 firmware, scan_secrets OK, wifi connect thành công.
- Sensor-node: Wi-Fi + MQTT CoreIoT connected, telemetry `{d1..d6, nearest_cm}` publish mỗi 500ms, 6 sensor scan OK (REJECT khi không vật cản = đúng).
- Trạng thái git hiện tại (chưa commit):
  - M: `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.c`, `firmware/waveshare-screen/src/main.c` (DEBUG DEMO tolerate).
  - ?? `picture/`, `prototypes/` (untracked). `prototypes/i2c_probe/` đã chuyển sang i2c_master probe nhiều cặp — ĐANG NGUY HIỂM, tránh dùng/bỏ.

---

## 6. TIẾP THEO (việc đầu tiên khi mở lại session)
1. Thực hiện **Bước 1** ở trên (bỏ phụ thuộc CH422G trong backlight + touch reset), build, flash ttyACM1, quan sát màn.
2. Nếu sáng → chốt + nghiệm thu (Bước 3). Nếu tối → Bước 2 (probe addr 1 cặp 8/9).
3. Sau khi nghiệm thu xong: revert DEBUG DEMO, cập nhật log này + PROGRESS + ROADMAP, rồi mới tính commit/push.
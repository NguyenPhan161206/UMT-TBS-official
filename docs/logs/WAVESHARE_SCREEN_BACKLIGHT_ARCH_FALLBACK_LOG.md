# WAVESHARE_SCREEN_BACKLIGHT_ARCH_FALLBACK_LOG.md

> **Mục tiêu:** Làm màn waveshare 7" lên UI, **KHÔNG phụ thuộc CH422G** (fallback để EXIO2/DISP
> default high — giống repo paulhamsh), NHƯNG **giữ khả năng điều khiển backlight** (on/off/dim)
> khi CH422G hoạt động đúng addr.
> **Trạng thái 🟢 CODE ĐÃ IMPLEMENT + BUILD XANH (cả 2 branch); ⏳ flash-and-observe chờ board.**
> Ngày tạo: 2026-09-03. Cập nhật implement: 2026-09-03.

---

## A. Kiến trúc kết hợp (fallback + kiểm soát)

### Nguyên lý (đã xác minh logical)
- **Màn LÊN: không phụ thuộc CH422G.** Dùng fallback để EXIO2/DISP giữ default high.
  Bằng chứng: repo `paulhamsh` lên UI cùng board mà không đụng CH422G.
- **KIỂM SOÁT backlight: nếu CH422G ACK đúng addr → dùng để on/off/dim** (tiện ích phụ cho production).

### Logic cốt lõi (tại `waveshare_rgb_lcd_backlight_on()`)
```c
if (ch422g_backlight_on() != ESP_OK) {
    /* fallback: để EXIO2 default high, không hạ backlight */
    ESP_LOGW("CH422G fail -> backlight để HW default high (màn vẫn lên)");
    return ESP_OK;   /* màn luôn SÁNG dù CH422G NACK */
}
return ESP_OK;       /* màn SÁNG + kiểm soát backlight khi CH422G OK */
```

---

## A2. ✅ IMPLEMENTED (2026-09-03) — kiến trúc kết hợp chính thức

> Code đã viết xong, **build OK cả 2 branch**; chưa flash (chờ board `/dev/ttyACM1`).

### File đã sửa
| File | Thay đổi |
|------|----------|
| `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.h` | Thêm macro `CONFIG_WAVESHARE_BACKLIGHT_FALLBACK` (1=hybrid mặc định / 0=legacy) + doc contract cho `backlight_on()`. |
| `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.c` | Tách 2 nhánh `#if CONFIG_WAVESHARE_BACKLIGHT_FALLBACK` với đúng logic hybrid. |

### `waveshare_rgb_lcd_backlight_on()` — HYBRID (default, `=1`)
```c
esp_err_t err = i2c_master_init_if_needed();   if (err != ESP_OK) { log; return ESP_OK; }
err = ch422g_init_for_output();                if (err != ESP_OK) { log; return ESP_OK; }
err = i2c_master_write_to_device_ng(0x38,0x1E);if (err != ESP_OK) { log; return ESP_OK; }
return ESP_OK;   /* màn luôn SÁNG dù CH422G NACK + kiểm soát backlight khi OK */
```

### `waveshare_esp32_s3_touch_reset()` — HYBRID
- Chỉ **bắt buộc** `touch_reset_gpio_init()` (GPIO4 reset GT911).
- CH422G / GT911 I2C ghi `0x2C`/`0x2E` đều **optional** (`(void)` + chấp nhận fail).

### `waveshare_esp32_s3_rgb_lcd_init()` — HYBRID
- RGB panel init vẫn `ESP_ERROR_CHECK` (nhánh quan trọng — màn phải lên).
- Khối touch GT911: mọi step I2C đều **best-effort**, fail → log + `*touch_handle=NULL` + return `ESP_OK`. Màn/UI vẫn lên dù touch/CH422G không ACK.

### LEGACY (`=0`) — giữ nguyên `ESP_ERROR_CHECK` stock (R5, không xoá code)

### Kết quả verify
- `pio run -e yolo_uno` (hybrid): **SUCCESS** — RAM 12.8% / Flash 31.3%.
- `pio run -e yolo_uno` với `-DCONFIG_WAVESHARE_BACKLIGHT_FALLBACK=0` (legacy): **SUCCESS** — RAM 12.8% / Flash 31.3%.
- `scan_secrets.py`: OK (R1). Line count: `port.c` 296 / `port.h` 110 (R7 < 400).

---

## B. Các bước thực thi (từng bước)

### Bước 0 — Chuẩn bị
- Board waveshare cắm `/dev/ttyACM1`.
- `git status` sạch 2 file waveshare (đã commit hướng cũ — xem Phần D).
- Chạy `python3 tools/guard/scan_secrets.py` trước mọi commit.

### Bước 1 — Thực nghiệm quyết định (vô hiệu hoá HOÀN TOÀN call CH422G)
Sửa `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.c`:
- `waveshare_rgb_lcd_backlight_on()`:
  - BỎ `i2c_master_init_if_needed()`, `ch422g_init_for_output()`,
    `i2c_master_write_to_device_ng(0x38, 0x1E)`.
  - Log: `ESP_LOGI(TAG, "backlight: để HW default (CH422G EXIO2/DISP high) — không cần I2C");`
  - `return ESP_OK;`
- `waveshare_esp32_s3_touch_reset()`:
  - BỎ ghi `0x24` (ch422g_init) và ghi `0x38` `0x2C`/`0x2E`.
  - GIỮ `touch_reset_gpio_init()` + `gpio_set_level(EXAMPLE_TOUCH_RESET_GPIO=GPIO4, ...)` (reset GT911).

Build + flash:
```bash
cd firmware/waveshare-screen
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1
```

**QUAN SÁT MÀN:**
- **SÁNG + UI lên** → ✅ chốt: màn lên KHÔNG cần CH422G. Tiếp **Bước 3**.
- **VẪN TỐI** → EXIO2 cần được kéo high (không default) → cần CH422G đúng addr → **Bước 2**.

### Bước 2 — (CHỈ khi Bước 1 tối) Tìm addr CH422G đúng (AN TOÀN)
- Probe **CHỈ 1 cặp 8/9** (1-pass, KHÔNG quét nhiều cặp — ⚠️ đã 2 lần mất USB).
- Quét addr khả dĩ `0x20..0x3f` (Wiki xác nhận CH422G nằm băng này). V2 dùng `0x24` — có thể sai lô.
- Tìm addr CH422G ACK → đổi addr trong `ch422g_init_for_output()` + backlight → backlight bật qua CH422G.

### Bước 3 — Tích hợp kiến trúc kết hợp (fallback chính thức)
- Xây `waveshare_rgb_lcd_backlight_on()` theo logic phần A: thử CH422G → fail thì fallback ESP_OK.
- **TOUCH**: GT911 addr `0x14` trên 8/9 (repo lạ/Makerfabs dùng) — reset bằng GPIO4; không phụ thuộc CH422G.
- Bọc 2 chế độ sau **macro** để "không xóa code cũ":
  ```c
  #define CONFIG_WAVESHARE_BACKLIGHT_FALLBACK 1   // bật fallback (mục tiêu)
  // khi =0 → dùng CH422G thuần (hướng cũ) — giữ code trong git.
  ```

---

## C. Nghiệm thu & hoàn thiện
- UI hiển thị + Wi-Fi/CoreIoT (**IP 10.0.11.237** đã connect) + telemetry.
- Nghiệm thu **6 sensor** (sensor-node đã Wi-Fi + MQTT OK).
- Revert DEBUG DEMO về `ESP_ERROR_CHECK` khi backlight ổn (hoặc giữ fallback nếu board cần).
- Cập nhật `docs/PROGRESS.md` + `docs/.opencode/ROADMAP_CHECKLIST.md`.
- Chạy `/verify` (scan_secrets, selfcheck) trước commit.
- Commit hướng mới truyền thống.

---

## D. Git trạng thái (khi mở lại — SAU khi commit+back HEAD từ phiên trước)
- Phiên trước đã **commit hướng cũ** lên `origin main` (2 file waveshare tolerate + log).
- Sau đó **back HEAD** (2 file waveshare về bản gốc `ESP_ERROR_CHECK`) → working tree sạch.
- Code hướng cũ **vẫn nằm trong lịch sử git** (không xóa) → quay lại được bất cứ lúc nào.
- Giờ sửa theo hướng mới (Bước 1+3) không đụng hướng cũ.

---

## E. Lệnh build/flash/monitor
```bash
cd firmware/waveshare-screen
pio run -e yolo_uno
pio run -e yolo_uno -t upload --upload-port /dev/ttyACM1
pio device monitor -p /dev/ttyACM1 -b 115200
```
- Log qua USB-Serial-JTAG console (production waveshare đọc qua ttyACM1).

---

## F. ⚠️ Bài học an toàn (rất quan trọng)
- **KHÔNG probe quét nhiều cặp chân I2C** — đã 2 lần làm **mất USB board** (ttyACM re-enumerate).
- Nếu cần probe: **CHỈ 1 cặp 8/9, 1-pass**, tránh lặp nhiều cặp.
- Trước commit: chạy `scan_secrets` (R1) + `/verify`.

---

## G. Ghi chú session tạo file này (bối cảnh)
- Wi-Fi mới "Bamos Coffee 2G" đã regen credentials (gitignored) cho cả 2 firmware; scan OK; wifi connect.
- Sensor-node: Wi-Fi + MQTT CoreIoT connected; telemetry `{d1..d6, nearest_cm}` mỗi 500ms; 6 sensor scan OK.
- Trạng thái git trước khi commit/push phiên này: `M` 2 file waveshare (DEBUG DEMO tolerate) + `??` log + `picture/` + `prototypes/`.
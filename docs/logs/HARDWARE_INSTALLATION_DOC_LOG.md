# HARDWARE_INSTALLATION_DOC_LOG

## Mục tiêu
Tạo tài liệu hướng dẫn lắp đặt thiết bị phần cứng cho hệ thống Truck Blind-spot Warning System V2
(khi người dùng chưa có sẵn tài liệu lắp đặt, chỉ có mô tả trong README/báo cáo).

## File đã tạo
- `docs/HARDWARE_INSTALLATION.md` — hướng dẫn đầy đủ: BOM, sơ đồ khối, bảng GPIO map
  6 JSN-SR04T + buzzer, cầu phân áp Echo 5V→3.3V, hướng lắp cảm biến quanh xe, cấu hình
  mạng (channel 6), lắp màn hình, flash & checklist nghiệm thu, xử lý sự cố, an toàn.

## Nguồn dữ liệu
- `firmware/shared/thresholds.h` — GPIO map (SENSOR_PINS), ngưỡng CAUTION/DANGER, buzzer GPIO11.
- `firmware/shared/espnow_protocol.h` — ESPNOW_CHANNEL=6, interval 500ms, timeout 1500ms.
- `report/chapters/04_he_thong.tex` — mô tả thiết bị, GPIO map, lưu ý PSRAM GPIO47/48.
- `README.md` — kiến trúc, các bước cấu hình/flash.

## Kết quả kiểm thử
- Không có mã nguồn thay đổi — không cần build.
- Tham chiếu chân GPIO đối chiếu trực tiếp với `thresholds.h` (nguồn duy nhất, R2/R3/R4).
- Không chứa secret (chỉ nhắc người dùng tự điền vào `config/keys.json`).

## Hướng dẫn vận hành
- In/bám theo mục 4 (bảng đấu dây) và mục 8 (flash & checklist) khi lắp xe thật.
- Khi đổi WiFi hotspot channel khác 6: cập nhật `ESPNOW_CHANNEL` trong `firmware/shared/espnow_protocol.h`
  và rebuild cả 2 firmware (xem mục 6 trong tài liệu).
# pictures/ — ảnh raster cho Báo cáo Chi tiết Code

Thư mục này **được track** (khác với build artifacts). Đặt ảnh thật tại đây
rồi tham chiếu bằng `\pic{pictures/ten_anh.png}{0.8}` — macro `\pic` trong
`main.tex` tự bỏ qua ảnh khi file chưa tồn tại (build không gãy).

## Danh sách ảnh mong muốn (thả vào khi có)

| File dự kiến | Nội dung | Nguồn |
|---|---|---|
| `pictures/hardware_sensor_node.jpg` | Ảnh sensor-node + 6 cảm biến JSN-SR04T | Chụp board thật (T0.4) |
| `pictures/hardware_screen.jpg` | Ảnh waveshare-screen 7" RGB | Chụp board thật |
| `pictures/dashboard_b9.png` | Dashboard CoreIoT hiện `warning_status` | Nghiệm thu B9 |
| `pictures/ui_dashboard.png` | Ảnh chụp màn hình LVGL chạy thật | Board thật |
| `pictures/pcb_wiring.png` | Sơ đồ đấu dây GPIO (6 cặp Trig/Echo + buzzer) | T0.4 / mạch thật |

## Quy tắc
- Không đặt ảnh chứa secret/token (R1 — `scan_secrets.py` sẽ báo nếu hit).
- Mỗi ảnh ≤ 2 MB, đặt tên ASCII, ghi nguồn vào bảng trên.
## [Goal Description] Kế hoạch Thực thi Giai đoạn 3 & Tiền đề T1

Theo đúng định hướng, chúng ta sẽ không làm mò mẫm trên mạch phần cứng (vì mỗi lần build và flash mất rất nhiều thời gian), mà sẽ giải quyết các **Tiền đề T1** trước khi làm đồ họa UI.

Kế hoạch này vạch ra chiến lược để từng bước hoàn thành Giai đoạn 3:

### Bước 1: Tiền đề T1.2 - Xây dựng Simulator LVGL trên PC
Thay vì lập trình giao diện (UI) và nạp thẳng xuống mạch ESP32, chúng ta sẽ tạo một môi trường giả lập (SDL) chạy trực tiếp trên Linux.
- **Tác dụng:** Giúp chúng ta vẽ xe tải, vẽ icon (T3.1, T3.2) và nhìn thấy kết quả ngay lập tức trên màn hình máy tính mà không cần cắm board.
- **Mục tiêu thay đổi:** Tạo thư mục `firmware/waveshare-screen/host_sim/` chứa code giả lập, dùng CMake để build chạy nội bộ (Native Build).

### Bước 2: T3.2 - Vẽ sơ đồ xe tải EX8 linh hoạt
Dựa trên môi trường Simulator ở Bước 1, chúng ta sẽ thêm Cấu trúc Dữ liệu (Struct) chứa kích thước thật của xe tải EX8.
- **Tác dụng:** Code giao diện tự động chia tỷ lệ pixel màn hình dựa trên kích thước xe tải thật, cho phép dễ dàng tái sử dụng nếu dự án nâng cấp lên xe khác.
- **Mục tiêu thay đổi:** Sửa `ui_dashboard_layout.c` để tự động render kích thước xe và các vạch khoảng cách dựa theo `sensor_model`.

### Bước 3: T3.1 - Biểu tượng (Icon) phương tiện/vật thể tại vị trí phát hiện
Tiếp tục trên Simulator, ta sẽ lập trình để các ô cảm biến không chỉ hiện chữ `DANG` mà sẽ hiển thị icon vật cản.
- **Tác dụng:** Khi cảm biến báo khoảng cách 50cm, một icon chấm đỏ sẽ dịch chuyển đúng tọa độ 50cm (theo tỷ lệ) trên màn hình.
- **Mục tiêu thay đổi:** Thêm Logic di chuyển vị trí Icon (Lv_obj_set_pos) liên tục theo giá trị trả về của `sensor_model_classify()`.

### Bước 4: Tiền đề T1.4 - Tool ghi & phát lại dữ liệu thật
Mạch Cảm Biến sẽ gửi Data lên CoreIoT. Ta sẽ dùng một script Python (`record_telemetry.py`) kết nối vào MQTT để **Ghi âm** toàn bộ dữ liệu bạn đo ngoài đường, và lưu thành file.
- **Tác dụng:** Mang dữ liệu ngoài đường về phòng thí nghiệm, dùng `replay_telemetry.py` để bắn ngược lại vào Simulator, mô phỏng đúng y hệt cảnh xe chạy.
- **Mục tiêu thay đổi:** Viết 2 file Python trong thư mục `tools/`.

### Bước 5: T3.5 - Cân chỉnh độ trễ bộ lọc (Filter Tuning)
Dựa vào dữ liệu thật đã thu ở Bước 4, ta sẽ chạy và tinh chỉnh thông số Filter trên máy tính.
- **Tác dụng:** Khắc phục tình trạng "đo mực nước" chậm chạp, giúp hệ thống phản xạ siêu tốc (< 0.2s) khi có xe tạt ngang.
- **Mục tiêu thay đổi:** Tối ưu hóa `DistanceFilter` trong code của Mạch Cảm Biến (có Unit Test chứng minh độ nhạy).

---

## User Review Required
> [!IMPORTANT] 
> 1. **Về T3.3 (Buzzer):** Đây là công việc hàn mạch phần cứng, không thể xử lý bằng Code. Bạn cần tự mua Transistor (NPN hoặc MOSFET) để kích nguồn 5V/12V cho còi to hơn. Chân điều khiển hiện đang là `GPIO 11`.
> 2. **Về Kích thước EX8:** Để thực thi Bước 2, bạn cần cung cấp cho tôi chiều Dài x Rộng của xe tải, và vị trí chính xác của 6 cảm biến sẽ được lắp (vd: Đầu xe, Đuôi xe cách lề bao nhiêu cm).

## Verification Plan

### Automated Tests
- Build giả lập: `cmake -S host_sim -B /tmp/host_sim && cmake --build /tmp/host_sim`
- Build firmware mạch: `pio run -e yolo_uno`

### Manual Verification
- Bạn sẽ chạy file thực thi Simulator trên Linux và nhìn thấy màn hình Cảnh báo hiển thị y hệt như trên mạch thật.
- Gửi lệnh MQTT ảo vào Simulator và xem xe tải/icon thay đổi ra sao.

---

**Bạn có đồng ý với chiến lược đi từ Simulator (T1) rồi mới vẽ UI (T3) này không? Nếu có, hãy nhấn PROCEED để chúng ta bắt đầu Bước 1!**

## [Giải thích & Định hướng] Giai đoạn 3: Hoàn thiện các yêu cầu trong Đề xuất

Mục 3 (Giai đoạn 3) trong `CHECKLIST.md` tập trung vào việc **nâng cấp trải nghiệm người lái** và **tối ưu thuật toán** để đáp ứng đúng cam kết trong hồ sơ đề tài nghiên cứu. 

Dưới đây là giải thích cặn kẽ bằng ngôn ngữ dễ hiểu nhất cho từng đầu việc, và bạn (với tư cách là người nắm phần cứng) cần làm gì:

---

### T3.1: Hiển thị Biểu tượng vật cản trên màn hình
- **Đang có gì:** Hiện tại trên màn hình, khi có vật cản, UI chỉ hiện các dòng chữ khô khan (ví dụ: `DANG` - Nguy hiểm, kèm khoảng cách) ở 6 ô chữ nhật tĩnh.
- **Yêu cầu:** Phải có một biểu tượng (icon) chấm đỏ/vàng hoặc hình chiếc xe mô tô/chướng ngại vật xuất hiện *đúng tại vị trí xung quanh hình chiếc xe tải* trên màn hình. Nếu vật cản gần lại, icon này cũng xê dịch lại gần xe tải.
- **Bạn cần làm gì:** Việc vẽ UI (giao diện) tôi sẽ lo phần Code. Bạn chỉ cần đánh giá xem Icon xuất hiện có đúng hướng của cảm biến thực tế khi bạn đưa tay che không.

### T3.2: Vẽ sơ đồ xe tải EX8 linh hoạt (Không gắn cứng)
- **Đang có gì:** Màn hình chỉ hiển thị các khối cảm biến cứng nhắc. Hình dạng xe tải chưa được thể hiện tỷ lệ chuẩn.
- **Yêu cầu:** Hệ thống phải load được "Hồ sơ" (Profile) của loại xe tải Hyundai Mighty EX8 (Kích thước thật: Dài x Rộng, vị trí lắp 6 cảm biến ở đâu). Từ hồ sơ này, code sẽ tự tính toán để vẽ ra chiếc xe tải trên màn hình sao cho cân đối, thay vì gõ "chết" các con số pixel (tọa độ x, y) vào code. Việc này giúp sau này gắn lên xe khác chỉ cần đổi Profile.
- **Bạn cần làm gì:** Bạn cần cung cấp cho tôi thông số kích thước thật của xe tải EX8, và vị trí chính xác (đo bằng mét) mà bạn dự định gắn 6 cảm biến JSN-SR04T trên thân chiếc xe tải này.

### T3.3: Cảnh báo âm thanh trong cabin (Phần cứng)
- **Đang có gì:** Trong code (T2.1) tôi đã viết logic còi kêu rồi (DANGER thì kêu liên tục, CAUTION thì kêu ngắt quãng). Chân còi (Buzzer) được đổi sang `Pin 11` để hết bị xung đột.
- **Yêu cầu:** Buzzer cắm trực tiếp vào ESP32 kêu quá bé để nghe được trong cabin ồn ào. Bạn phải lắp thêm một mạch khuếch đại (dùng Transistor để nâng lên nguồn 5V/12V cho còi to).
- **Bạn cần làm gì:** Đây là **việc của bạn 100%**. Bạn cần hàn mạch điện khuếch đại cho còi (Buzzer), chụp ảnh tài liệu lắp đặt thực tế để bổ sung vào báo cáo nghiệm thu.

### T3.4: Thống nhất các ngưỡng cảnh báo (ĐÃ XONG ✅)
- **Giải thích:** Còi và Màn hình từng dùng 2 bộ khoảng cách (ngưỡng) khác nhau để báo động, gây loạn.
- **Trạng thái:** Mục này **tôi đã làm xong** cho bạn ở các phiên trước! Ngưỡng DANGER (30cm) và CAUTION (100cm) giờ đã đồng bộ từ một file duy nhất (`thresholds.h`) và được truyền trực tiếp xuống Còi.

### T3.5: Cân chỉnh độ trễ bộ lọc (Filter) cho vật chuyển động
- **Đang có gì:** Cảm biến siêu âm thường bị nhiễu (đang 100cm tự nhiên nhảy về 0cm rồi lại 100cm). Hệ thống đang dùng thuật toán lọc theo kiểu "đo mực nước" (chờ 5 mẫu liên tiếp giống nhau thì mới tin). Cách này quá chậm (mất 0.5 - 0.8 giây để báo động), xe tải chạy nhanh thì tai nạn đã xảy ra rồi.
- **Yêu cầu:** Tinh chỉnh lại thuật toán Kalman/Cluster để vừa lọc được nhiễu, vừa phản ứng chớp nhoáng (độ trễ < 0.2s) khi có xe máy tạt ngang.
- **Bạn cần làm gì:** Để làm được, chúng ta cần dữ liệu thực tế! Bạn cần chạy xe, cầm vật cản lướt qua cảm biến để tôi thu thập dữ liệu (Record Telemetry), sau đó tôi sẽ chỉnh thuật toán dựa trên dữ liệu thật của bạn.

---

### Định hướng tiếp theo: Chúng ta nên làm gì trước?
Mục **T3.1 và T3.2** liên quan trực tiếp đến giao diện hiển thị xe tải và vật cản. 
Nhưng để code UI đồ họa vẽ xe và vị trí một cách mượt mà, **không nên code mù rồi cứ mỗi lần sửa lại mất công bấm nạp (flash) xuống Board 10 phút**.

**Khuyến nghị:** Chúng ta nên ưu tiên **T1.2 (Làm trình giả lập Simulator SDL trên PC)**. Tôi sẽ thiết lập môi trường để giao diện Màn Hình (LVGL) có thể chạy trực tiếp như một phần mềm trên Linux của bạn. Sửa code UI xong, bấm `Run` là giao diện hiện lên màn hình máy tính ngay lập tức, code nhanh gấp 10 lần!

Bạn có muốn tôi lên Master Plan cho T1.2 (Simulator) để làm bàn đạp giải quyết toàn bộ Giai đoạn 3 không?

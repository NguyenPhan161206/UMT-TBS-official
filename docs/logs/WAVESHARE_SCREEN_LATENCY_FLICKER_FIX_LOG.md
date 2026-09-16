# Nhật Ký: Tối Ưu Độ Trễ Cảm Biến & Chẩn Đoán Lỗi Màn Hình Chập Chờn / Giật Hình

- **Dự án**: Hệ thống Cảnh Báo Điểm Mù Xe Tải (Truck Blind Sight)
- **Thời gian**: 16/09/2026
- **Thành phần**: `firmware/sensor-node`, `firmware/waveshare-screen`, `firmware/shared`

---

## 1. Nội Dung Đã Hoàn Thành Trong Buổi Làm Việc

### Tối ưu đường truyền & Giảm triệt để độ trễ phản hồi (> 3s xuống ~0.3s)
- [firmware/shared/thresholds.h](file:///e:/Truck_Blind_Sight/firmware/shared/thresholds.h):
  - Giảm `FILTER_JUMP_CONFIRM_COUNT` từ 3 xuống 2: Xác nhận bước nhảy khoảng cách xa nhanh hơn.
- [firmware/shared/espnow_protocol.h](file:///e:/Truck_Blind_Sight/firmware/shared/espnow_protocol.h):
  - Tăng tần số gửi ESP-NOW: `ESPNOW_SEND_INTERVAL_MS` giảm từ 500ms xuống 100ms (10 gói/giây).
  - Điều chỉnh `ESPNOW_LINK_TIMEOUT_MS` từ 3000ms xuống 1500ms.
- [firmware/sensor-node/src/main.cpp](file:///e:/Truck_Blind_Sight/firmware/sensor-node/src/main.cpp):
  - **Adaptive Polling cho cảm biến chưa cắm**: Loại bỏ thời gian chờ timeout 200ms (5 slot x 40ms) của các cảm biến không kết nối trên bàn test. Chu kỳ đo của cảm biến S2 giảm từ 205ms xuống đúng 100ms.
  - **Kết quả**: Khi bỏ vật cản/rút tay ra khỏi cảm biến, trạng thái hạ từ `DANGER` về `SAFE` phản hồi cực nhạy trong **~0.3s** (trước đây trễ > 3s).

### Tối ưu cấu hình nạp và Wi-Fi trên màn hình
- [firmware/waveshare-screen/platformio.ini](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/platformio.ini):
  - Thêm `upload_speed = 921600` giúp tốc độ flash firmware chỉ mất ~12 giây.
- [firmware/waveshare-screen/components/coreiot_client/coreiot_client.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/coreiot_client/coreiot_client.c):
  - Khóa kênh Wi-Fi cố định ở kênh 6 (`ESPNOW_CHANNEL`), giới hạn tối đa 3 lần thử kết nối để tránh phát xung RF liên tục.
- [firmware/waveshare-screen/src/main.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/src/main.c):
  - Tăng `LV_LOCK_TIMEOUT_TICKS` lên 500ms tránh nghẽn luồng render.
  - Thêm transition guard chỉ cập nhật UI khi trạng thái thực sự thay đổi.

---

## 2. Vấn Đề Tồn Tại: Màn Hình Vẫn Chập Chờn / Giật Hình

### Hiện trạng quan sát thực tế:
- Màn hình hiển thị đầy đủ giao diện, tuy nhiên **thỉnh thoảng vẫn bị chập chờn / giật nhẹ**.
- **Đặc biệt khi chuyển đổi trạng thái từ `SAFE` sang `DANGER` thì màn hình bị giật rất mạnh**.

---

## 3. Nghi Ngờ Các Nguyên Nhân Gây Ra Hiện Tượng Chập Chờn

Qua phân tích mã nguồn và đặc tính phần cứng màn hình RGB 800x480 trên ESP32-S3, các nguyên nhân gây giật chập chờn được xác định như sau:

### 1. Xung đột vòng lặp Animation chớp nháy (Nghi ngờ số 1 - Gây giật mạnh khi sang DANGER)
- Trong toàn bộ giao diện [ui_dashboard_layout.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c), chỉ duy nhất vùng `DANGER` mới bật hiệu ứng chớp nháy `blink_anim`.
- Hàm `arc_set_zone()` hiện tại **chưa có cờ lưu `current_zone`**. Vì ESP-NOW gửi 10 gói/giây (100ms/lần), nên khi ở vùng DANGER, cứ mỗi 100ms code lại:
  ```c
  lv_anim_delete(a->arc, blink_anim_cb); // Xóa animation cũ đang chạy dở
  lv_anim_start(&a->blink_anim);         // Tạo và chạy lại animation mới
  ```
- Việc liên tục xóa và khởi tạo lại animation 10 lần mỗi giây làm engine vẽ của LVGL bị xung đột luồng và ép vẽ lại toàn bộ vùng cung tròn liên tục.

### 2. Nghẽn băng thông bộ nhớ PSRAM do hiệu ứng Alpha Blending (Nghi ngờ số 2)
- Animation chớp nháy đang dùng cơ chế thay đổi độ mờ đục `lv_obj_set_style_arc_opa` từ 100% về 30% (`LV_OPA_COVER` $\leftrightarrow$ `LV_OPA_30`).
- Màn hình 7" RGB565 chạy trên RAM ngoài Octal PSRAM. Kênh DMA phần cứng phải đọc liên tục từ PSRAM ra màn hình ở tốc độ ~30 MB/s.
- Hiệu ứng làm mờ (Alpha Blending) trên một widget diện tích lớn như `lv_arc` buộc CPU phải đọc từng pixel nền từ PSRAM, tính toán màu trong suốt rồi ghi đè lại vào PSRAM ở tốc độ 30-60 FPS.
- Sự tranh chấp bus dữ liệu PSRAM giữa CPU vẽ alpha và DMA LCD làm bộ đệm DMA bị rỗng (FIFO Underflow), sinh ra hiện tượng xé hình / chớp giật ngang trên màn hình.

### 3. Sụt áp nguồn do phát sóng Wi-Fi ngầm (Nghi ngờ số 3 - Gây chập chờn ngắt quãng)
- Module `coreiot_client` chạy tác vụ nền cố gắng tìm và kết nối Wi-Fi.
- Mỗi lần chip ESP32-S3 phát xung sóng RF 2.4GHz công suất cao để tìm AP, dòng tiêu thụ tức thời tăng vọt 300-400mA, gây sụt áp trên cổng USB cấp nguồn cho màn hình 7" (vốn tiêu thụ dòng đèn nền khá lớn).

### 4. Spam hàm tính toán `evaluate_hazard()`
- Trong hàm nhận dữ liệu `on_espnow_rx`, vòng lặp 6 cảm biến đang gọi `evaluate_hazard()` tới 6 lần liên tiếp trong cùng 1 chu kỳ 100ms, khiến các nhãn văn bản và màu sắc bị ép gán lại nhiều lần không cần thiết.

---

## 4. Hướng Khắc Phục Tiếp Theo

1. **Thêm Guard Zone**: Trong `sensor_arc_t`, lưu lại `current_zone`. Nếu cảm biến chưa đổi zone thì không bao giờ reset hoặc gọi lại animation.
2. **Chuyển sang Nháy Màu Tĩnh (Solid Color Blink)**: Thay vì làm mờ đục liên tục (Alpha Blending ngốn bus PSRAM), chuyển sang nháy chuyển màu (Đỏ $\leftrightarrow$ Trong suốt/Viền đỏ) bằng bộ đếm thời gian tĩnh, loại bỏ hoàn toàn tính toán alpha.
3. **Gộp lệnh cập nhật UI**: Chỉ gọi `evaluate_hazard()` duy nhất 1 lần sau khi vòng lặp 6 cảm biến hoàn tất.

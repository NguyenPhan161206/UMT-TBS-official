# Tài liệu Cập nhật: Tích hợp Gửi Dữ liệu Test Thuật toán lên CoreIoT

**Nhánh áp dụng:** `khuong`
**File bị ảnh hưởng chính:** `firmware/sensor-node/src/main.cpp`
**Mục đích:** Ghi nhận và phân tích hiệu năng của thuật toán lọc khoảng cách bằng cách đối chiếu 3 luồng dữ liệu (Khoảng cách chuẩn - Dữ liệu thô - Dữ liệu đã qua bộ lọc) trực tiếp trên Web Dashboard (CoreIoT).

---

## 1. Chi tiết các thay đổi

### 1.1. Thêm các mảng biến toàn cục (Global Arrays)
Khai báo 3 mảng để lưu trạng thái của toàn bộ 6 cảm biến cùng lúc:
```cpp
volatile float g_test_raw_cm[6] = {0};
volatile float g_test_filtered_cm[6] = {0};
volatile float g_test_standard_cm[6] = {100.0f, 150.0f, 50.0f, 120.0f, 200.0f, 80.0f}; 
```
*   **`g_test_raw_cm`**: Mảng lưu giá trị gốc, chưa qua xử lý (bị nhiễu).
*   **`g_test_filtered_cm`**: Mảng lưu giá trị sau khi đã đi qua bộ lọc thuật toán (DistanceFilter).
*   **`g_test_standard_cm`**: Mảng lưu các mốc khoảng cách CHUẨN (Khoảng cách tĩnh thực tế từ vật cản đến cảm biến do người dùng tự setup và đo bằng thước). Người test có thể sửa các con số này cho khớp với thực địa.

### 1.2. Thu thập dữ liệu tại `sensorTask`
Tại đoạn code đọc cảm biến, dữ liệu thô và dữ liệu sau lọc được "bắt" lại và gán vào đúng vị trí tương ứng (`i` từ 0 đến 5) của các mảng toàn cục trên.
```cpp
g_test_raw_cm[i] = reading.distanceCm; 
g_test_filtered_cm[i] = result.outputCm; 
```

### 1.3. Cập nhật cấu trúc Payload MQTT tại `coreiotTask`
*   **Tăng kích thước bộ đệm (Buffer):** Tăng từ 256 bytes lên `512 bytes` để có đủ bộ nhớ chứa gói tin JSON lớn hơn.
*   **Thiết kế lại mã JSON:** Thay vì gửi format chuẩn (`d1`, `d2`,... và `nearest_cm`), code hiện tại đóng gói đồng thời **18 thông số** (3 thông số x 6 cảm biến) với quy ước đặt tên:
    *   `std0`, `raw0`, `flt0`: Số liệu của cảm biến thứ nhất (index 0).
    *   Tương tự kéo dài đến `std5`, `raw5`, `flt5` (cảm biến thứ sáu).

---

## 2. Ý nghĩa và Tác dụng (Impacts)

Việc thay đổi này chuyển đổi chế độ hoạt động của mạch từ **"Vận hành tiêu chuẩn"** sang **"Chế độ Đo lường & Hiệu chuẩn (Calibration)"**. Cụ thể:

1. **Hiển thị độ nhiễu trực quan (Raw vs Filtered):** 
   Khi đẩy lên CoreIoT, người dùng có thể vẽ 2 đồ thị trồng lên nhau. Nếu đường `raw` nhảy liên tục (nhiễu gai) nhưng đường `flt` đi ngang ổn định, điều này chứng minh và nghiệm thu được độ hiệu quả của thuật toán `DistanceFilter`.
   
2. **Đánh giá sai số tuyệt đối (Standard vs Filtered):**
   Biến `std` (khoảng cách chuẩn được đo bằng thước) đóng vai trò làm đường tham chiếu (Baseline). Việc vẽ đường `flt` cạnh đường `std` giúp nhóm nghiên cứu tính toán được độ lệch (Error) của cảm biến siêu âm ở các mốc khoảng cách khác nhau (VD: đo 150cm thật nhưng mạch báo 148cm $\rightarrow$ sai số 2cm).

3. **Phân tích song song 6 kênh:**
   Nhờ gửi đủ 18 biến, Dashboard có thể tạo ra 6 bảng biểu đồ độc lập cho 6 góc của xe, giúp phát hiện ra nếu có 1 cảm biến cụ thể nào đó bị hỏng hoặc bị can nhiễu từ môi trường (góc nghiêng, vật cản bề mặt hẹp).

## 3. Cách đã gộp vào `nguyen` (thay đổi so với bản gốc ở mục 1)

Để không phá hệ thống đang chạy, tính năng này **chỉ bật khi build env `yolo_uno_coreiot_algotest`** (cờ `TBS_ALGO_TEST=1`), cùng mẫu với `TBS_LATENCY_PROBE` / `TBS_ACCURACY_PROBE`:

- Payload **giữ nguyên** `d1..d6, nearest_cm, has_nearest, seq`; 18 trường `std/raw/fltN` được **nối thêm** vào cuối (không còn thay thế) → rule-chain, màn hình, `host_sim`, `tools/record_telemetry.py` và công cụ đo độ trễ vẫn đọc được. Bản build thường không đổi gì.
- `coreiot_client.cpp` gọi `setBufferSize(512)` khi bật cờ: PubSubClient mặc định giới hạn 256 B (gồm topic + header), payload 18 trường có thể vượt ngưỡng này và `publish()` trả `false` âm thầm.
- Khôi phục `sharedStateSetHealth(i, h)` trong `sensorTask` (bản gốc xoá nhầm; thiếu nó cảm biến không chuyển lại `OK`/`OUT_OF_RANGE`).
- Mốc chuẩn có thể ghi đè khi build: `-D TBS_ALGO_STD_CM_INIT="{100.0f,150.0f,50.0f,120.0f,200.0f,80.0f}"`; thứ tự theo `SENSOR_PINS[]`.
- Lưu ý: CoreIoT chỉ nhận 1 gói mỗi `COREIOT_PUBLISH_INTERVAL_MS` (2000 ms), tức mỗi 2 s chỉ có 1 ảnh chụp giá trị mới nhất, không phải mọi mẫu 10 Hz.

> ⚠️ **Lưu ý trong tương lai:** Vì payload này đã thay đổi hoàn toàn so với format V2 gốc, nếu chạy hệ thống Rule Chain cũ trên CoreIoT (cái mà bóc tách `d1..d6` và `warning_status`) thì Rule Chain sẽ không hoạt động. Khi test xong và muốn đưa vào nghiệm thu chạy thực tế, cần Revert (Hoàn tác) lại đoạn payload này.


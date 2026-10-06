# Log Nhiệm vụ: Xử lý Cảm biến Mất kết nối & Dữ liệu Cũ (Sensor Fault & Stale Data Handling)

**Thời gian:** 2026-09-16  
**Mục tiêu:** Giải quyết triệt để lỗi Safety-Critical: Khi đang nhận tín hiệu cảnh báo (DANGER/CAUTION) mà sensor bị rút nguồn / đứt dây hoặc mất kết nối, hệ thống vẫn lưu và hiển thị cảnh báo dựa trên dữ liệu khoảng cách cũ (stale data).

---

## 1. Nguyên nhân gốc rễ (Root Cause)
1. **Sensor-node:** 
   - Hàm `measure_raw()` có cơ chế lọc nhiễu cần tới 15 chu kỳ miss liên tiếp mới xác nhận mất tín hiệu (~1500ms).
   - Khi cảm biến bị ngắt nguồn, chân Echo trôi nổi (floating) hoặc timeout, buffer vẫn giữ khoảng cách hợp lệ gần nhất.
   - Còi báo `buzzer.cpp` đọc trực tiếp từ cache mà không kiểm tra tính hợp lệ tức thời của khoảng cách.
2. **ESP-NOW Link:**
   - Gói tin chỉ gửi mảng `valid[6]`, thiếu nhãn trạng thái sức khỏe cảm biến rõ ràng (Health: OK, OUT_OF_RANGE, DISCONNECTED, STALE) và số thứ tự freshness `seq`.
3. **Waveshare-screen (Bộ thu & Logic xử lý):**
   - `on_espnow_rx()` khi nhận gói có `valid=0` lại bỏ qua (không cập nhật vào `sensor_model`), dẫn tới `sensor_model` lưu giữ giá trị khoảng cách nguy hiểm trước đó.
   - Không có bộ giám sát watchdog kiểm tra tuổi thọ dữ liệu từng cảm biến (per-sensor stale watchdog).
   - Thuật toán `hazard_worst_zone()` trong `hazard_core` chỉ lọc cờ `is_stale`, chưa loại bỏ slot lỗi kết nối / hỏng nguồn.
   - Giao diện UI chưa phân biệt được trạng thái "An toàn (đường thoáng)" vs "Mất cảm biến / Cảm biến bị hỏng".

---

## 2. Các thay đổi đã thực hiện (Implementation Details)

### 2.1. Shared Contract (`firmware/shared/`)
- [thresholds.h](file:///e:/Truck_Blind_Sight/firmware/shared/thresholds.h):
  - Định nghĩa enum [sensor_health_t](file:///e:/Truck_Blind_Sight/firmware/shared/thresholds.h#L55-L60): `SENSOR_HEALTH_OK`, `SENSOR_HEALTH_OUT_OF_RANGE`, `SENSOR_HEALTH_DISCONNECTED`, `SENSOR_HEALTH_STALE`.
  - Hằng số timing chuẩn hoá: `SENSOR_FAULT_CONSECUTIVE_MISS` (3 lần ~300ms) và `SENSOR_STALE_TIMEOUT_MS` (1000ms).
- [espnow_protocol.h](file:///e:/Truck_Blind_Sight/firmware/shared/espnow_protocol.h):
  - Cập nhật struct [espnow_sensor_msg_t](file:///e:/Truck_Blind_Sight/firmware/shared/espnow_protocol.h#L79-L85): thêm `uint16_t seq` và `uint8_t health[ESPNOW_SENSOR_SLOT_COUNT]`.
  - Cập nhật `static_assert` bảo toàn packed layout 38 byte trên cả 2 board.

### 2.2. Sensor-Node (`firmware/sensor-node/`)
- [ultrasonic_sensor.cpp](file:///e:/Truck_Blind_Sight/firmware/sensor-node/src/ultrasonic_sensor.cpp):
  - Khởi tạo chân Echo với `INPUT_PULLDOWN` chống nhiễu trôi nổi khi rút giắc cắm.
  - Hạ ngưỡng miss xuống 3 lần liên tiếp: chuyển trạng thái `SENSOR_HEALTH_DISCONNECTED`, reset filter và đánh dấu `valid = false`.
- [shared_state.h](file:///e:/Truck_Blind_Sight/firmware/sensor-node/include/shared_state.h) & [shared_state.cpp](file:///e:/Truck_Blind_Sight/firmware/sensor-node/src/shared_state.cpp):
  - Lưu trữ mảng `health[6]`, tăng sequence `seq` mỗi chu kỳ truyền.
- [buzzer.cpp](file:///e:/Truck_Blind_Sight/firmware/sensor-node/src/buzzer.cpp):
  - Lập tức ngắt còi buzzer khi cảm biến gần nhất chuyển sang `DISCONNECTED` hoặc không còn `valid`.
- [espnow_client.cpp](file:///e:/Truck_Blind_Sight/firmware/sensor-node/src/espnow_client.cpp):
  - Đóng gói đầy đủ `seq`, `valid`, `health` truyền qua ESP-NOW.

### 2.3. Waveshare-Screen (`firmware/waveshare-screen/`)
- [sensor_model.h](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/sensor_model/include/sensor_model.h) & [sensor_model.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/sensor_model/sensor_model.c):
  - Bổ sung trường `sensor_health_t health` trong `sensor_reading_t`.
  - Hàm `sensor_model_set_health()` và `sensor_model_clear()` đặt `is_stale = true` ngay khi mất kết nối.
- [espnow_receiver.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/espnow_receiver/espnow_receiver.c):
  - Xử lý gói tin: nếu slot có `health == DISCONNECTED`, gọi ngay `sensor_model_set_health()` và `sensor_model_clear()` để xoá dữ liệu cũ tức thì.
- [main.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/src/main.c):
  - Tích hợp Watchdog định kỳ 100ms kiểm tra timestamp của từng sensor: nếu quá 1000ms không có dữ liệu mới, đánh dấu `SENSOR_HEALTH_STALE` và clear slot.
- [hazard_core.h](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/hazard_core/include/hazard_core.h) & [hazard_core.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/hazard_core/hazard_core.c):
  - Cập nhật [hazard_worst_zone](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/hazard_core/include/hazard_core.h#L58-L62): Bỏ qua hoàn toàn các slot `is_stale` hoặc có health là `DISCONNECTED`/`STALE`.
  - Thêm [hazard_has_sensor_fault](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/hazard_core/include/hazard_core.h#L65): phát hiện xà hệ thống có ít nhất 1 cảm biến bị lỗi.
  - Tuân thủ nghiêm ngặt quy tắc B4: hazard_core giữ đúng 4 hàm public API thuần C (`hazard_classify`, `hazard_worst_zone`, `hazard_has_sensor_fault`, `hazard_eval_crossing`).
- [ui_dashboard.c](file:///e:/Truck_Blind_Sight/firmware/waveshare-screen/components/ui_dashboard/ui_dashboard.c):
  - Hiển thị nhãn sensor: nếu `DISCONNECTED` hiển thị `DISC`, nếu `STALE` hiển thị `STALE`, không có vật cản hiển thị `-- cm`.
  - Banner trung tâm: Khi có cảm biến bị lỗi, gắn thêm cảnh báo `| SENSOR FAULT` để tài xế nhận diện xe đang có điểm mù mất tầm nhìn.

---

## 3. Kết quả Kiểm thử & Xác minh (Verification)

1. **Host unit tests sensor-node (`pio test -e native`):**
   - **15/15 test cases PASSED** (Bao gồm kiểm tra `sizeof(espnow_sensor_msg_t) == 38 bytes`, bộ lọc nhiễu khoảng cách, ngưỡng cảnh báo R3).
2. **Build sensor-node:**
   - `pio run -e yolo_uno`: **SUCCESS** (RAM 13.6%, Flash 20.8%).
   - `pio run -e yolo_uno_coreiot`: **SUCCESS** (RAM 14.0%, Flash 21.5%).
3. **Build waveshare-screen:**
   - `pio run -e yolo_uno`: **SUCCESS** (RAM 32.7%, Flash 31.5%).
4. **Kiểm tra kiến trúc & bảo mật:**
   - `python tools/guard/arch_guard.py`: **[arch-guard] ARCH-GUARD OK** (Tuân thủ toàn bộ B1-B7, R2 contract).
   - `python tools/guard/scan_secrets.py`: **SECRET-SCAN OK: no secret patterns found.**
   - `python tools/guard/gen_credentials.py --check`: **GEN-CREDENTIALS OK**.
   - `python tools/guard/check_rulechain_thresholds.py`: **CHECK-RULECHAIN OK: thresholds.h {100.0, 30.0} đều có mặt trong rule-chain.**
   - `python -m pytest tools/guard/test_guard.py -q`: **29/29 passed in 6.41s**.

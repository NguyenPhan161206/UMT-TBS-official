# Log: Triển khai 2-way Mute Alarm & Gỡ lỗi GT911 Touch Controller

**Ngày:** 2026-09-21
**Thành phần:** `sensor-node`, `waveshare-screen`, `shared`
**Trạng thái:** HOÀN THÀNH

## Mục tiêu
1. Bổ sung tính năng tắt tiếng còi Buzzer (Mute Alarm) không dây từ xa thông qua giao diện chạm (Touch UI) trên màn hình Waveshare 7".
2. Giải quyết tình trạng màn hình không phản hồi cảm ứng (Touch Unresponsive) do lỗi I2C.
3. Sửa lỗi giao diện khi ấn Mute khiến các cảm biến bị ngắt kết nối hiển thị màu Đỏ (DANGER).
4. Đồng bộ hóa kênh giao tiếp ESP-NOW giữa 2 board trên cùng dải mạng Wi-Fi CoreIoT.

## Các file đã sửa đổi
- `firmware/shared/espnow_protocol.h`: Bổ sung cấu trúc gói tin `espnow_cmd_msg_t` và mã lệnh `ESPNOW_CMD_MUTE_BUZZER`.
- `firmware/sensor-node/include/shared_state.h`, `firmware/sensor-node/src/shared_state.cpp`: Bổ sung biến trạng thái `s_isMuted` và Mutex Thread-safe.
- `firmware/sensor-node/src/espnow_client.cpp`: Tích hợp bộ bắt gói tin (Receiver Callback) và parse tín hiệu MUTE.
- `firmware/sensor-node/src/buzzer.cpp`: Thay đổi logic `buzzerTask` để tôn trọng cờ `s_isMuted`, lập tức dùng `noTone()` khi còi bị cấm.
- `firmware/waveshare-screen/src/bsp/waveshare_rgb_lcd_port.c`: Sửa lại `dev_addr` của chip cảm ứng GT911 thành `0x14` (Địa chỉ dự phòng) thay vì `0x5D` chuẩn để lách lỗi treo I2C do thiếu cờ Reset INT.
- `firmware/waveshare-screen/components/espnow_receiver/espnow_receiver.c`: Cấu hình Broadcast Peer và hàm truyền `espnow_receiver_send_cmd()`.
- `firmware/waveshare-screen/components/ui_dashboard/ui_dashboard.c`: 
  - Khởi tạo tín hiệu gọi ESP-NOW trong `mute_btn_cb`.
  - Cắt bỏ vòng lặp `arc_set_zone` vô nghĩa khi ấn Mute (gây sai lệch màu đỏ).
  - Tích hợp logic `BUZZER: MUTED` và `BUZZER: ON/OFF` vào hàm `evaluate_hazard()`.

## Kết quả kiểm thử (Thực hiện bởi User & Agent)
- [x] Tính năng hiển thị Con trỏ Ảo (Virtual Cursor) thành công. Cảm ứng hoạt động mượt mà.
- [x] Khi bấm Mute, chữ `BUZZER` đổi thành `MUTED`.
- [x] Các cảm biến ngắt kết nối giữ nguyên trạng thái màu Xám (Ngắt kết nối) thay vì biến thành màu Đỏ cảnh báo nguy hiểm.
- [x] Lệnh Mute đi xuyên mạng không dây và tắt tiếng vật lý thành công (Theo báo cáo của User).
- [x] Code pass toàn bộ các Guard Rules (Quét Secret, Quét File Keys).

## Hướng dẫn Vận hành / Demo
- Mã nguồn đã được commit lên nhánh `nguyen` (Commit `9d32b00`) và hợp nhất thành công sang nhánh `main` (Commit `b090062`).
- Người dùng chỉ cần bật nguồn cả 2 mạch. Chờ mạch đồng bộ Wi-Fi (khoảng 3-5 giây).
- Khi có báo động nguy hiểm, ấn "Mute Alarm" ở viền trái màn hình để tắt còi tức thời. Mọi cảm biến đo xa vẫn hoạt động ngầm. Ấn "Unmute Alarm" để khôi phục cảnh báo bằng âm thanh.

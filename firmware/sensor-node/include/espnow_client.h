#pragma once

#include "espnow_protocol.h"

// Client ESP-NOW mỏng để gửi struct khoảng cách cảm biến trực tiếp tới
// waveshare-screen. Không blocking: esp_now_send() là non-blocking, kết
// quả gửi thành công/thất bại được báo qua callback nội bộ (onDataSent),
// không chặn networkTask.
// Contract ESP-NOW (struct/channel/MAC) lấy DUY NHẤT từ espnow_protocol.h
// (firmware/shared) — không định nghĩa lại ở đây (R2).
class EspNowClient {
public:
    // Khởi tạo WiFi STA (không kết nối AP) + esp_now + đăng ký peer
    // waveshare-screen. Gọi 1 lần trong networkTask.
    void begin();

    // Gửi 1 bản ghi khoảng cách 6 slot tới waveshare-screen.
    bool sendReading(const espnow_sensor_msg_t &msg);
};
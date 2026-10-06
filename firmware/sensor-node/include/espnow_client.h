#pragma once

#include <stdint.h>

#include "espnow_protocol.h"

// Một mẫu RTT ESP-NOW (sensor-node -> màn hình -> echo về), chỉ có khi TBS_LATENCY_PROBE=1.
struct EspNowRtt {
    uint16_t seq;
    uint32_t rttUs;
};

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

    // Số gói đã phát / thất bại (gồm cả esp_now_send lỗi) kể từ boot — cho dòng SOAK.
    uint32_t txOk() const;
    uint32_t txFail() const;

    // Lấy 1 mẫu RTT đã đo (không blocking). Luôn false nếu không build TBS_LATENCY_PROBE.
    bool pollRtt(EspNowRtt &out);
};
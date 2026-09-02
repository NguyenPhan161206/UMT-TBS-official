/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * espnow_receiver.h — ESP-NOW receiver (đường chính) cho waveshare-screen.
 *
 * R2: payload/struct/channel/MAC lấy DUY NHẤT từ firmware/shared/espnow_protocol.h.
 * Component này KHÔNG biết LVGL/sensor_model: nhận gói đã parse, ghi timestamp
 * per-slot, gọi callback đăng ký (chạy trên WiFi task). Main sẽ lấy LVGL lock.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "espnow_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback invoked on the Wi-Fi/esp-now task for every validated packet.
 *
 * KHÔNG gọi hàm LVGL ở đây trực tiếp — hãy lấy LVGL lock (esp_lv_adapter_lock)
 * trước khi chạm vào UI. Tham số rssi (dBm) là signal strength của gói.
 */
typedef void (*espnow_rx_cb_t)(const espnow_sensor_msg_t *msg, int8_t rssi);

/**
 * @brief Khởi tạo ESP-NOW receiver.
 *
 * Phải gọi SAU khi esp_wifi_start() (coreiot_client_init() đã start WiFi).
 * Nếu WiFi chưa kết nối AP, radio được cố định ở ESPNOW_CHANNEL để khớp
 * sensor-node; khi kết nối AP thật (B9), esp-now tự chạy trên channel của AP
 * nên sensor-node và screen phải cùng AP (hoặc AP channel 1).
 *
 * @param cb callback nhận dữ liệu (nullable -> chỉ track link, không gọi)
 * @return ESP_OK hoặc mã lỗi esp_now_init (ESP_ERR_ESPNOW_EXIST nếu đã init)
 */
esp_err_t espnow_receiver_init(espnow_rx_cb_t cb);

/**
 * @brief true nếu có gói ESP-NOW hợp lệ trong ESPNOW_LINK_TIMEOUT_MS gần nhất.
 */
bool espnow_receiver_is_linked(void);

/**
 * @brief Timestamp (ms kể từ boot) của gói cuối có valid[slot]==1.
 *        0 = chưa bao giờ nhận dữ liệu slot đó.
 */
uint32_t espnow_receiver_last_rx_ms(uint8_t slot);

#ifdef __cplusplus
}
#endif
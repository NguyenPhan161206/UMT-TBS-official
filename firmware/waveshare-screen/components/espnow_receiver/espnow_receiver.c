/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * espnow_receiver.c — ESP-NOW receive path (protocol-only, không LVGL).
 * Nhận espnow_sensor_msg_t từ sensor-node (firmware/shared/espnow_protocol.h),
 * validate kích thước, ghi last-rx per-slot, gọi callback đăng ký.
 */

#include "espnow_receiver.h"

#include "esp_log.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include <string.h>

static const char *TAG = "espnow_receiver";

static espnow_rx_cb_t s_rx_cb = NULL;

/* Timestamp gói gần nhất (bất kỳ gói hợp lệ nào) — dùng cho LINKED badge. */
static int64_t s_last_any_rx_us = 0;
/* Timestamp gói có valid[slot]==1 — dùng để clear slot khi quá ESPNOW_LINK_TIMEOUT_MS. */
static int64_t s_slot_rx_us[ESPNOW_SENSOR_SLOT_COUNT];

static bool s_espnow_ready = false;

static void on_data_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len != (int)sizeof(espnow_sensor_msg_t)) {
        ESP_LOGW(TAG, "Dropped packet with unexpected size %d (expect %d)", len, (int)sizeof(espnow_sensor_msg_t));
        return;
    }

    /* info là con trỏ tạm do driver cấp trong callback — copy nếu cần dùng lâu,
     * nhưng ta chỉ đọc rssi ngay nên không cần giữ. */
    if (info == NULL) {
        return;
    }

    espnow_sensor_msg_t msg;
    memcpy(&msg, data, sizeof(msg));

    s_last_any_rx_us = esp_timer_get_time();
    int64_t now_us = s_last_any_rx_us;

    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        if (msg.valid[i]) {
            s_slot_rx_us[i] = now_us;
        }
    }

    if (s_rx_cb) {
        int rssi = (info->rx_ctrl != NULL) ? (int)info->rx_ctrl->rssi : 0;
        s_rx_cb(&msg, (int8_t)rssi);
    }
}

esp_err_t espnow_receiver_init(espnow_rx_cb_t cb)
{
    s_rx_cb = cb;
    memset(s_slot_rx_us, 0, sizeof(s_slot_rx_us));
    s_last_any_rx_us = 0;

    /* Ở chế độ STA "trần" (chưa kết nối AP), cố định channel khớp sensor-node.
     * Nếu sau này kết nối AP thành công (B9), esp-now tự bám channel của AP;
     * khi đó cả 2 board phải cùng AP (hoặc AP đặt channel 1). */
    esp_err_t err = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_wifi_set_channel(%d) rc=%d (bỏ qua: WiFi đang connect AP hoặc chưa start)",
                 ESPNOW_CHANNEL, (int)err);
    }

    err = esp_now_init();
    if (err == ESP_ERR_ESPNOW_EXIST) {
        /* Đã được init bởi module khác — chấp nhận, chỉ đăng ký recv cb. */
        ESP_LOGW(TAG, "esp_now already initialized elsewhere");
        err = ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init failed: %d", (int)err);
        return err;
    }

    err = esp_now_register_recv_cb(on_data_recv);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_register_recv_cb failed: %d", (int)err);
        return err;
    }

    s_espnow_ready = true;
    ESP_LOGI(TAG, "ESP-NOW receiver ready on channel %d (peer/sender per shared protocol)",
             ESPNOW_CHANNEL);
    return ESP_OK;
}

bool espnow_receiver_is_linked(void)
{
    if (!s_espnow_ready || s_last_any_rx_us == 0) {
        return false;
    }
    int64_t now_us = esp_timer_get_time();
    return (now_us - s_last_any_rx_us) <= (int64_t)ESPNOW_LINK_TIMEOUT_MS * 1000;
}

uint32_t espnow_receiver_last_rx_ms(uint8_t slot)
{
    if (slot >= ESPNOW_SENSOR_SLOT_COUNT || s_slot_rx_us[slot] == 0) {
        return 0;
    }
    return (uint32_t)(s_slot_rx_us[slot] / 1000);
}
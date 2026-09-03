/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * main.c — waveshare-screen (V2).
 *
 * Hai đường dữ liệu:
 *   - ĐƯỜNG CHÍNH: ESP-NOW receiver (components/espnow_receiver) — sensor-node
 *     gửi espnow_sensor_msg_t (firmware/shared/espnow_protocol.h, R2) mỗi 500ms.
 *   - ĐƯỜNG PHỤ: CoreIoT MQTT (components/coreiot_client) qua Wi-Fi STA.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_timer.h"

#include "coreiot_client.h"
#include "espnow_receiver.h"
#include "sensor_model.h"
#include "ui_dashboard.h"
#include "waveshare_rgb_lcd_port.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "collision_dashboard";

/* Callers (coreiot data cb) chạy trên MQTT task, không phải LVGL task — phải
 * lấy LVGL lock với timeout để không block luồng MQTT mãi mãi. */
#define LV_LOCK_TIMEOUT_TICKS pdMS_TO_TICKS(100)

/* =========================================================
 * COREIOT DATA PATH (đường phụ)
 * Parse JSON nhận từ CoreIoT (rule-chain) và đẩy vào dashboard:
 *   - "distances": [6] float cm → ui_dashboard_update_sensor
 *   - "relay" / "warning_status" / "buzzer" → server status
 * ========================================================= */

static void on_coreiot_data(const char *topic, int topic_len, const char *payload, int payload_len)
{
    (void)topic;
    (void)topic_len;

    cJSON *root = cJSON_ParseWithLength(payload, payload_len);
    if (root == NULL) {
        ESP_LOGW(TAG, "Ignored non-JSON payload");
        return;
    }

    if (esp_lv_adapter_lock(LV_LOCK_TIMEOUT_TICKS) != ESP_OK) {
        cJSON_Delete(root);
        return;
    }

    cJSON *dist = cJSON_GetObjectItem(root, "distances");
    if (cJSON_IsArray(dist)) {
        int n = cJSON_GetArraySize(dist);
        if (n > (int)SENSOR_MODEL_COUNT) {
            n = (int)SENSOR_MODEL_COUNT;
        }
        for (int i = 0; i < n; i++) {
            cJSON *item = cJSON_GetArrayItem(dist, i);
            if (cJSON_IsNumber(item)) {
                ui_dashboard_update_sensor((uint8_t)i, (uint16_t)cJSON_GetNumberValue(item));
            }
        }
    }

    cJSON *relay = cJSON_GetObjectItem(root, "relay");
    if (cJSON_IsString(relay)) {
        ui_dashboard_set_relay_state(strcmp(relay->valuestring, "ON") == 0, "N/A (rule-chain)");
    }

    cJSON *buzzer = cJSON_GetObjectItem(root, "buzzer");
    if (cJSON_IsString(buzzer)) {
        ui_dashboard_set_buzzer_state(strcmp(buzzer->valuestring, "ON") == 0);
    }

    cJSON *crossing = cJSON_GetObjectItem(root, "crossing_hazard");
    if (cJSON_IsBool(crossing)) {
        ui_dashboard_set_hazard_warning(cJSON_IsTrue(crossing));
    }

    esp_lv_adapter_unlock();
    cJSON_Delete(root);
}

static void on_wifi_status(bool is_connected, const char *ip)
{
    if (esp_lv_adapter_lock(LV_LOCK_TIMEOUT_TICKS) != ESP_OK) {
        return;
    }
    ui_dashboard_set_iot_status(is_connected, ip);
    esp_lv_adapter_unlock();
}

static void on_mqtt_status(bool is_connected)
{
    if (esp_lv_adapter_lock(LV_LOCK_TIMEOUT_TICKS) != ESP_OK) {
        return;
    }
    ui_dashboard_set_iot_status(is_connected, NULL);
    esp_lv_adapter_unlock();
}

/* =========================================================
 * ESP-NOW PATH (đường chính, độ trễ thấp)
 * Sensor-node gửi espnow_sensor_msg_t (firmware/shared) mỗi 500ms.
 * Callback chạy trên WiFi task -> lấy LVGL lock có timeout (như MQTT path).
 * ========================================================= */

static void on_espnow_rx(const espnow_sensor_msg_t *msg, int8_t rssi)
{
    ESP_LOGI(TAG, "ESP-NOW frame rssi=%d dBm", (int)rssi);

    if (esp_lv_adapter_lock(LV_LOCK_TIMEOUT_TICKS) != ESP_OK) {
        return;
    }

    ui_dashboard_set_espnow_status(true);

    for (uint8_t i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        if (msg->valid[i]) {
            ui_dashboard_update_sensor(i, (uint16_t)msg->distance_cm[i]);
        } else {
            /* valid=0: sensor-node báo "null" cho slot này (chưa lắp / mất tín hiệu)
             * -> clear thay vì giữ số cũ trên màn hình. */
            ui_dashboard_clear_sensor(i);
        }
    }

    esp_lv_adapter_unlock();
}

/* Watchdog chạy trên LVGL task (timer 500ms): nếu vừa hết link hoặc slot quá
 * ESPNOW_LINK_TIMEOUT_MS, đưa về NO LINK / clear slot tương ứng. */
static void espnow_link_watchdog_cb(lv_timer_t *timer)
{
    (void)timer;

    bool linked = espnow_receiver_is_linked();
    ui_dashboard_set_espnow_status(linked);
    if (!linked) {
        return;
    }

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    for (uint8_t i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        uint32_t last = espnow_receiver_last_rx_ms(i);
        if (last != 0 && (now_ms - last) > ESPNOW_LINK_TIMEOUT_MS) {
            ui_dashboard_clear_sensor(i);
        }
    }
}

void app_main(void)
{
    const esp_lv_adapter_rotation_t rotation = ESP_LV_ADAPTER_ROTATE_0;
    const esp_lv_adapter_tear_avoid_mode_t tear_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_RGB;

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_touch_handle_t touch_handle = NULL;

    ESP_ERROR_CHECK(waveshare_esp32_s3_rgb_lcd_init(
        tear_mode,
        rotation,
        &panel_handle,
        &touch_handle));
    /* DEBUG DEMO (R5): board này CH422G hiện không ACK (phần cứng) -> không abort.
     * Revert ESP_ERROR_CHECK sau khi backlight hardware khôi phục. */
    if (waveshare_rgb_lcd_backlight_on() != ESP_OK) {
        ESP_LOGW(TAG, "Backlight over CH422G failed (bus 8/9 no CH422G ACK) -> continuing for UI debug");
    }

    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_config.task_stack_size = 12 * 1024;
    adapter_config.stack_in_psram = true;
    ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter_config));

    esp_lv_adapter_display_config_t disp_config = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
        panel_handle,
        NULL,
        EXAMPLE_LCD_H_RES,
        EXAMPLE_LCD_V_RES,
        rotation);
    disp_config.profile.use_psram = true;

    lv_display_t *disp = esp_lv_adapter_register_display(&disp_config);
    assert(disp != NULL);

    if (touch_handle != NULL) {
        esp_lv_adapter_touch_config_t touch_config = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handle);
        lv_indev_t *touch = esp_lv_adapter_register_touch(&touch_config);
        assert(touch != NULL);
    }

    ESP_ERROR_CHECK(esp_lv_adapter_start());

    ESP_LOGI(TAG, "Initializing Collision-Avoidance Dashboard");
    if (esp_lv_adapter_lock(-1) == ESP_OK) {
        ui_dashboard_init();
        ui_dashboard_set_relay_state(false, "N/A (MQTT path)");
        ui_dashboard_set_espnow_status(false);

        /* Watchdog ESP-NOW: chạy trên LVGL task → gọi UI trực tiếp an toàn. */
        lv_timer_create(espnow_link_watchdog_cb, 500, NULL);

        esp_lv_adapter_unlock();
    }

    /* Đường phụ: CoreIoT MQTT (Wi-Fi STA). */
    coreiot_client_set_callbacks(on_wifi_status, on_mqtt_status, on_coreiot_data);
    coreiot_client_init();

    /* Đường chính: ESP-NOW receiver — phải gọi SAU esp_wifi_start()
     * (coreiot_client_init() đã start WiFi). */
    esp_err_t en_err = espnow_receiver_init(on_espnow_rx);
    if (en_err != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW receiver init failed: %d", (int)en_err);
    }
}
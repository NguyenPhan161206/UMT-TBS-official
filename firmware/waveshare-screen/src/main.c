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
#include "sys_settings_manager.h"
#include "waveshare_rgb_lcd_port.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "collision_dashboard";

/* Callers (espnow rx cb, coreiot cb) chạy trên network tasks — lấy LVGL lock
 * có timeout để đồng bộ an toàn với tác vụ vẽ giao diện. */
#define LV_LOCK_TIMEOUT_TICKS pdMS_TO_TICKS(500)

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
        ui_dashboard_evaluate_hazard();
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
    ui_dashboard_set_wifi_status(is_connected, ip);
    esp_lv_adapter_unlock();
}

static void on_mqtt_status(bool is_connected)
{
    if (esp_lv_adapter_lock(LV_LOCK_TIMEOUT_TICKS) != ESP_OK) {
        return;
    }
    ui_dashboard_set_mqtt_status(is_connected);
    esp_lv_adapter_unlock();
}

/* =========================================================
 * ESP-NOW PATH (đường chính, độ trễ thấp)
 * Sensor-node gửi espnow_sensor_msg_t (firmware/shared) mỗi 100ms.
 * Thread-Safe Reactive Pipeline: Callback chạy trên Wi-Fi task chỉ ghi snapshot
 * vào RAM và thoát (< 1us). Tác vụ LVGL tự đọc và dispatch qua timer 50ms.
 * Tuyệt đối không gọi esp_lv_adapter_lock từ callback Wi-Fi để tránh nghẽn luồng.
 * ========================================================= */

static espnow_sensor_msg_t s_latest_espnow_msg;
static volatile bool s_espnow_msg_pending = false;
static portMUX_TYPE s_espnow_mux = portMUX_INITIALIZER_UNLOCKED;

static void on_espnow_rx(const espnow_sensor_msg_t *msg, int8_t rssi)
{
    (void)rssi;
    taskENTER_CRITICAL(&s_espnow_mux);
    memcpy(&s_latest_espnow_msg, msg, sizeof(espnow_sensor_msg_t));
    s_espnow_msg_pending = true;
    taskEXIT_CRITICAL(&s_espnow_mux);
}

/* Dispatcher chạy trực tiếp trên LVGL task (chu kỳ 50ms): tiêu thụ dữ liệu ESP-NOW
 * mà không cần lock mutex giữa 2 Core, chấm dứt hoàn toàn hiện tượng giật do tranh chấp luồng. */
static void espnow_ui_dispatch_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    espnow_sensor_msg_t msg;
    bool pending = false;

    taskENTER_CRITICAL(&s_espnow_mux);
    if (s_espnow_msg_pending) {
        memcpy(&msg, &s_latest_espnow_msg, sizeof(espnow_sensor_msg_t));
        s_espnow_msg_pending = false;
        pending = true;
    }
    taskEXIT_CRITICAL(&s_espnow_mux);

    if (!pending) {
        return;
    }

    ui_dashboard_set_espnow_status(true);

    for (uint8_t i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        sensor_health_t h = (sensor_health_t)msg.health[i];

        if (msg.valid[i]) {
            /* Cảm biến hoạt động tốt: cập nhật khoảng cách và health (đã có Deadband 3cm bên trong). */
            ui_dashboard_update_sensor(i, (uint16_t)msg.distance_cm[i]);
        } else if (h == SENSOR_HEALTH_DISCONNECTED) {
            /* Mất kết nối: chỉ xóa và cập nhật UI khi trạng thái thay đổi. */
            sensor_reading_t cur = sensor_model_get((sensor_id_t)i);
            if (cur.health != SENSOR_HEALTH_DISCONNECTED || !cur.is_stale) {
                sensor_model_set_health((sensor_id_t)i, SENSOR_HEALTH_DISCONNECTED);
                ui_dashboard_clear_sensor(i);
                ESP_LOGI(TAG, "Slot %u DISCONNECTED (sensor-node reported fault)", i);
            }
        }
    }

    /* Đánh giá tổng thể 1 lần duy nhất sau khi duyệt hết 6 cảm biến */
    ui_dashboard_evaluate_hazard();
}

/* Watchdog chạy trên LVGL task (timer 250ms): nếu vừa hết link hoặc slot quá
 * SENSOR_STALE_TIMEOUT_MS, đưa về NO LINK / clear slot tương ứng. */
static void espnow_link_watchdog_cb(lv_timer_t *timer)
{
    (void)timer;

    bool linked = espnow_receiver_is_linked();

    /* Transition UP/DOWN: Chỉ cập nhật UI và clear slots khi trạng thái link THAY ĐỔI.
     * Tránh gọi clear lặp lại mỗi 250ms gây nghẽn bus PSRAM và trigger Task Watchdog. */
    static int s_prev_linked = -1;
    if ((int)linked != s_prev_linked) {
        ESP_LOGI(TAG, "ESP-NOW link %s", linked ? "UP" : "DOWN");
        s_prev_linked = (int)linked;
        ui_dashboard_set_espnow_status(linked);
        if (!linked) {
            for (uint8_t i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
                sensor_model_set_health((sensor_id_t)i, SENSOR_HEALTH_STALE);
                ui_dashboard_clear_sensor(i);
            }
            ui_dashboard_evaluate_hazard();
            return;
        }
    }

    if (!linked) {
        return;
    }

    /* Per-sensor timeout: SENSOR_STALE_TIMEOUT_MS (1000ms) riêng từng slot.
     * Chặt hơn ESPNOW_LINK_TIMEOUT_MS (3000ms) để phát hiện nhanh khi
     * 1 cảm biến cụ thể bị rút dây trong khi các cảm biến khác vẫn tốt. */
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    bool any_cleared = false;
    for (uint8_t i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        uint32_t last = espnow_receiver_last_rx_ms(i);
        if (last != 0 && (now_ms - last) > SENSOR_STALE_TIMEOUT_MS) {
            sensor_reading_t r = sensor_model_get((sensor_id_t)i);
            if (!r.is_stale) {
                sensor_model_set_health((sensor_id_t)i, SENSOR_HEALTH_STALE);
                ui_dashboard_clear_sensor(i);
                any_cleared = true;
            }
        }
    }
    if (any_cleared) {
        ui_dashboard_evaluate_hazard();
    }
}

void app_main(void)
{
    sys_settings_init();
    
    const esp_lv_adapter_rotation_t rotation = ESP_LV_ADAPTER_ROTATE_0;
    /* Chế độ NONE: Single PSRAM buffer, vẽ cục bộ (partial), không block task chờ VSYNC,
     * giảm tải bus PSRAM 300 lần so với full-frame mode và tương thích hoàn hảo khi bật Wi-Fi. */
    const esp_lv_adapter_tear_avoid_mode_t tear_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_NONE;

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_touch_handle_t touch_handle = NULL;

    ESP_ERROR_CHECK(waveshare_esp32_s3_rgb_lcd_init(
        tear_mode,
        rotation,
        &panel_handle,
        &touch_handle));
    ESP_ERROR_CHECK(waveshare_rgb_lcd_backlight_on());

    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_config.task_stack_size = 12 * 1024;
    adapter_config.stack_in_psram = true;
    adapter_config.task_min_delay_ms = 5;
    ESP_ERROR_CHECK(esp_lv_adapter_init(&adapter_config));

    esp_lv_adapter_display_config_t disp_config = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
        panel_handle,
        NULL,
        EXAMPLE_LCD_H_RES,
        EXAMPLE_LCD_V_RES,
        rotation);
    disp_config.tear_avoid_mode = tear_mode;
    disp_config.profile.use_psram = true;

    lv_display_t *disp = esp_lv_adapter_register_display(&disp_config);
    assert(disp != NULL);

    if (touch_handle != NULL) {
        esp_lv_adapter_touch_config_t touch_config = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handle);
        lv_indev_t *touch = esp_lv_adapter_register_touch(&touch_config);
        assert(touch != NULL);
        
        // Bật con trỏ chuột ảo để debug lỗi lệch cảm ứng
        lv_obj_t * cursor_obj = lv_label_create(lv_screen_active());
        lv_label_set_text(cursor_obj, LV_SYMBOL_GPS);
        lv_obj_set_style_text_color(cursor_obj, lv_color_hex(0xFF0000), 0);
        lv_indev_set_cursor(touch, cursor_obj);
    }

    ESP_ERROR_CHECK(esp_lv_adapter_start());

    ESP_LOGI(TAG, "Initializing Collision-Avoidance Dashboard");
    if (esp_lv_adapter_lock(-1) == ESP_OK) {
        ui_dashboard_init();
        ui_dashboard_set_relay_state(false, "N/A (MQTT path)");
        ui_dashboard_set_espnow_status(false);

        /* Watchdog ESP-NOW: chạy trên LVGL task → gọi UI trực tiếp an toàn. */
        lv_timer_create(espnow_link_watchdog_cb, 250, NULL);

        /* ESP-NOW UI Dispatcher: chạy định kỳ 50ms trên LVGL task → đọc snapshot từ Wi-Fi task. */
        lv_timer_create(espnow_ui_dispatch_timer_cb, 50, NULL);

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
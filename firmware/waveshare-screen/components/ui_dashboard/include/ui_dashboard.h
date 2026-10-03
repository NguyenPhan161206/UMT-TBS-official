/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard.h — public API of the waveshare-screen dashboard component.
 * Implementation split: ui_dashboard.c (state/hazard), ui_dashboard_layout.c
 * (builder), ui_dashboard_system.c (SYSTEM page) — R7 mỗi file <= 400 dòng.
 *
 * LƯU Ý quy ước thread: các hàm này phải được gọi từ LVGL task (hoặc có
 * LVGL lock — xem main.c dùng esp_lv_adapter_lock).
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the full dashboard UI (header, collision page, system page).
 *        Must be called after LVGL is started, on the LVGL task.
 */
void ui_dashboard_init(void);

/**
 * @brief Feed a distance reading (cm) for one sensor slot into the dashboard.
 */
void ui_dashboard_update_sensor(uint8_t sensor_id, uint16_t dist_cm);

/**
 * @brief Mark a sensor slot as no-data (lost/unwired).
 */
void ui_dashboard_clear_sensor(uint8_t sensor_id);

/**
 * @brief Update Wi-Fi status badge (header + system page). Chỉ tác động Wi-Fi.
 */
void ui_dashboard_set_wifi_status(bool is_connected, const char *ip);

/**
 * @brief Update MQTT/CoreIoT status badge (header + system page). Chỉ tác động MQTT.
 */
void ui_dashboard_set_mqtt_status(bool is_connected);

/**
 * @brief Force/clear the pedestrian crossing hazard banner.
 */
void ui_dashboard_set_hazard_warning(bool is_pedestrian_crossing_risk);

/**
 * @brief Update relay + warning_status text (server path).
 */
void ui_dashboard_set_relay_state(bool relay_on, const char *warning_status);

/**
 * @brief Update buzzer state text.
 */
void ui_dashboard_set_buzzer_state(bool buzzer_on);

/**
 * @brief Update ESP-NOW link badge (đường chính, ESP-NOW receiver — bước B5).
 */
void ui_dashboard_set_espnow_status(bool linked);

/**
 * @brief Re-evaluate overall hazard & crossing hazard across all sensors and update banners.
 */
void ui_dashboard_evaluate_hazard(void);

/**
 * @brief Callback báo "người dùng vừa bấm Mute/Unmute" ra ngoài component.
 *        ui_dashboard KHÔNG biết đường truyền (ESP-NOW...): tầng main đăng ký
 *        callback và tự gửi lệnh (arch_guard B1 — UI không phụ thuộc driver).
 * @param muted Trạng thái SAU khi bấm: true = đang tắt tiếng.
 */
typedef void (*ui_dashboard_mute_cb_t)(bool muted);

/**
 * @brief Đăng ký callback Mute (NULL = bỏ đăng ký). Callback chạy trên LVGL task.
 */
void ui_dashboard_set_mute_cb(ui_dashboard_mute_cb_t cb);

#ifdef __cplusplus
}
#endif
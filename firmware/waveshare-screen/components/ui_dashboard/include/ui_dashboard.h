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
 * @brief Update Wi-Fi / MQTT status badges (header + system page).
 */
void ui_dashboard_set_iot_status(bool is_connected, const char *ip);

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

#ifdef __cplusplus
}
#endif
/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard.c — public API + hazard evaluation + mute state.
 * Widget-builder code nằm ở ui_dashboard_layout.c (R7: tách file <= 400 dòng).
 */

#include "ui_dashboard_private.h"

#include "coreiot_client.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ui_dashboard";

/* Toàn bộ trạng thái widget toàn cục — định nghĩa tại đây, extern ở private. */
lv_obj_t *s_screen;
lv_obj_t *s_tab_btn_collision;
lv_obj_t *s_tab_btn_system;
lv_obj_t *s_page_collision;
lv_obj_t *s_page_system;
lv_obj_t *s_lbl_wifi_status;
lv_obj_t *s_lbl_espnow_status;
lv_obj_t *s_lbl_mqtt_status;
lv_obj_t *s_lbl_hazard_overall;
lv_obj_t *s_lbl_crossing_risk;
lv_obj_t *s_lbl_relay_state;
lv_obj_t *s_lbl_buzzer_state;
lv_obj_t *s_lbl_sys_wifi;
lv_obj_t *s_lbl_sys_mqtt;
lv_obj_t *s_lbl_sys_device;
lv_obj_t *s_lbl_sys_broker;
lv_obj_t *s_lbl_sys_token;
lv_obj_t *s_lbl_sys_fw_version;
lv_obj_t *s_lbl_sys_idf_version;
lv_obj_t *s_lbl_sys_build;
lv_obj_t *s_lbl_sys_flash;
lv_obj_t *s_lbl_sys_heap;
lv_obj_t *s_lbl_sys_min_heap;
lv_obj_t *s_lbl_sys_uptime;
lv_timer_t *s_sys_info_timer;
bool s_alarm_muted = false;
lv_obj_t *s_mute_btn;
lv_obj_t *s_mute_btn_lbl;

sensor_arc_t s_arcs[SENSOR_MODEL_COUNT];
sensor_row_t s_rows[SENSOR_MODEL_COUNT];
uint16_t s_prev_distance_cm[SENSOR_MODEL_COUNT];
bool s_forced_crossing_warning = false;

/* Reflects s_alarm_muted on the button itself - otherwise "Mute Alarm" always
 * reads the same regardless of state and there is no way to tell from the
 * dashboard whether the alarm is currently silenced or live.
 */
void update_mute_button_visual(void)
{
    if (s_mute_btn_lbl) {
        lv_label_set_text(s_mute_btn_lbl, s_alarm_muted ? "Unmute Alarm" : "Mute Alarm");
    }
    if (s_mute_btn) {
        lv_obj_set_style_bg_color(s_mute_btn, lv_color_hex(s_alarm_muted ? COLOR_DANGER : COLOR_PANEL), 0);
    }
}

void mute_btn_cb(lv_event_t *e)
{
    (void)e;
    s_alarm_muted = !s_alarm_muted;
    update_mute_button_visual();

    sensor_reading_t readings[SENSOR_MODEL_COUNT];
    sensor_model_get_all(readings);
    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        arc_set_zone(&s_arcs[i], sensor_model_classify(readings[i].distance_cm));
    }
    evaluate_hazard();
}

void ui_dashboard_init(void)
{
    sensor_model_init();
    memset(s_prev_distance_cm, 0, sizeof(s_prev_distance_cm));

    s_screen = lv_screen_active();
    if (s_screen == NULL) {
        s_screen = lv_obj_create(NULL);
        lv_screen_load(s_screen);
    }
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    build_header(s_screen);

    lv_obj_t *content = lv_obj_create(s_screen);
    lv_obj_set_size(content, LV_PCT(100), 440);
    lv_obj_align(content, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_column(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);

    s_page_collision = lv_obj_create(content);
    lv_obj_set_size(s_page_collision, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(s_page_collision, 0, 0);
    lv_obj_set_style_pad_column(s_page_collision, 0, 0);
    lv_obj_set_style_border_width(s_page_collision, 0, 0);
    lv_obj_set_style_bg_opa(s_page_collision, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(s_page_collision, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(s_page_collision, LV_FLEX_FLOW_ROW);

    build_left_sidebar(s_page_collision);
    build_center_canvas(s_page_collision);
    build_right_sidebar(s_page_collision);

    s_page_system = build_system_page(content);
    lv_obj_add_flag(s_page_system, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(s_tab_btn_collision, tab_collision_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(s_tab_btn_system, tab_system_cb, LV_EVENT_CLICKED, NULL);

    s_sys_info_timer = lv_timer_create(sys_info_timer_cb, 2000, NULL);

    ESP_LOGI(TAG, "Collision dashboard UI initialized");
}

void evaluate_hazard(void)
{
    sensor_reading_t readings[SENSOR_MODEL_COUNT];
    sensor_model_get_all(readings);

    sensor_zone_t worst = SENSOR_ZONE_SAFE;
    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        /* Skip sensors that have never reported a reading (is_stale) - e.g. slots with no
         * physical hardware attached yet in ESP-NOW mode - otherwise their default distance_cm=0
         * classifies as DANGER and permanently pins the OVERALL banner.
         */
        if (readings[i].is_stale) {
            continue;
        }
        sensor_zone_t z = sensor_model_classify(readings[i].distance_cm);
        if (z > worst) worst = z;
    }

    if (s_lbl_hazard_overall) {
        const char *text = worst == SENSOR_ZONE_DANGER ? "OVERALL: DANGER"
                          : worst == SENSOR_ZONE_CAUTION ? "OVERALL: CAUTION"
                          : "OVERALL: SAFE";
        /* Alarm state only silences the blink/animation on DANGER zones (see mute_btn_cb) -
         * the banner itself must still say so, otherwise a muted DANGER looks identical to SAFE.
         */
        if (worst == SENSOR_ZONE_DANGER && s_alarm_muted) {
            lv_label_set_text(s_lbl_hazard_overall, "OVERALL: DANGER (MUTED)");
        } else {
            lv_label_set_text(s_lbl_hazard_overall, text);
        }
        lv_obj_set_style_text_color(s_lbl_hazard_overall, zone_color(worst), 0);
    }

    /* Crossing-traffic heuristic: the front sensor is close AND a side sensor's reading is
     * moving fast (large frame-to-frame delta), suggesting cross traffic passing the flank.
     */
    bool front_close = readings[ESPNOW_SLOT_FRONT].distance_cm < CROSSING_FRONT_THRESHOLD_CM;

    bool side_changing_fast = false;
    const char *crossing_sensor = NULL;
    for (int i = ESPNOW_SLOT_LEFT_FRONT; i <= ESPNOW_SLOT_RIGHT_REAR; i++) {
        int32_t delta = (int32_t)readings[i].distance_cm - (int32_t)s_prev_distance_cm[i];
        if (delta < 0) delta = -delta;
        if (delta >= CROSSING_DELTA_CM) {
            side_changing_fast = true;
            crossing_sensor = k_sensor_labels[i];
        }
    }

    bool crossing_hazard = s_forced_crossing_warning || (front_close && side_changing_fast);

    if (s_lbl_crossing_risk) {
        if (crossing_hazard) {
            char buf[64];
            snprintf(buf, sizeof(buf), "CROSSING TRAFFIC HAZARD%s%s",
                     crossing_sensor ? " - " : "", crossing_sensor ? crossing_sensor : "");
            lv_label_set_text(s_lbl_crossing_risk, buf);
            lv_obj_set_style_text_color(s_lbl_crossing_risk, lv_color_hex(COLOR_DANGER), 0);
        } else {
            lv_label_set_text(s_lbl_crossing_risk, "None detected");
            lv_obj_set_style_text_color(s_lbl_crossing_risk, lv_color_hex(COLOR_TEXT), 0);
        }
    }

    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        s_prev_distance_cm[i] = readings[i].distance_cm;
    }
}

void ui_dashboard_update_sensor(uint8_t sensor_id, uint16_t dist_cm)
{
    if (sensor_id >= SENSOR_MODEL_COUNT) {
        return;
    }

    sensor_model_set_distance((sensor_id_t)sensor_id, dist_cm);
    sensor_zone_t zone = sensor_model_classify(dist_cm);

    if (s_rows[sensor_id].row_value_lbl) {
        const char *suffix = zone == SENSOR_ZONE_DANGER ? " DANG" : "";
        lv_label_set_text_fmt(s_rows[sensor_id].row_value_lbl, "%s: %u cm%s",
                               k_sensor_labels[sensor_id], dist_cm, suffix);
        lv_obj_set_style_text_color(s_rows[sensor_id].row_value_lbl, zone_color(zone), 0);
    }

    if (s_arcs[sensor_id].arc) {
        arc_set_zone(&s_arcs[sensor_id], zone);
    }

    evaluate_hazard();
}

void ui_dashboard_clear_sensor(uint8_t sensor_id)
{
    if (sensor_id >= SENSOR_MODEL_COUNT) {
        return;
    }

    sensor_model_clear((sensor_id_t)sensor_id);

    if (s_rows[sensor_id].row_value_lbl) {
        lv_label_set_text_fmt(s_rows[sensor_id].row_value_lbl, "%s: -- cm", k_sensor_labels[sensor_id]);
        lv_obj_set_style_text_color(s_rows[sensor_id].row_value_lbl, lv_color_hex(COLOR_TEXT), 0);
    }

    if (s_arcs[sensor_id].arc) {
        arc_set_nodata(&s_arcs[sensor_id]);
    }

    evaluate_hazard();
}

void ui_dashboard_set_iot_status(bool is_connected, const char *ip)
{
    wifi_ap_record_t ap_info;
    bool have_ap_info = is_connected && (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK);
    const char *ssid = have_ap_info ? (const char *)ap_info.ssid : NULL;

    if (s_lbl_wifi_status) {
        if (is_connected && ssid) {
            lv_label_set_text_fmt(s_lbl_wifi_status, LV_SYMBOL_WIFI " %s", ssid);
            lv_obj_set_style_text_color(s_lbl_wifi_status, lv_color_hex(COLOR_SAFE), 0);
        } else if (is_connected && ip) {
            lv_label_set_text_fmt(s_lbl_wifi_status, LV_SYMBOL_WIFI " %s", ip);
            lv_obj_set_style_text_color(s_lbl_wifi_status, lv_color_hex(COLOR_SAFE), 0);
        } else {
            lv_label_set_text(s_lbl_wifi_status, LV_SYMBOL_WIFI " --");
            lv_obj_set_style_text_color(s_lbl_wifi_status, lv_color_hex(COLOR_DANGER), 0);
        }
    }

    if (s_lbl_mqtt_status) {
        lv_label_set_text(s_lbl_mqtt_status, is_connected ? "MQTT: UP" : "MQTT: DOWN");
        lv_obj_set_style_text_color(s_lbl_mqtt_status, is_connected ? lv_color_hex(COLOR_SAFE) : lv_color_hex(COLOR_DANGER), 0);
    }

    if (s_lbl_sys_wifi) {
        if (is_connected) {
            lv_label_set_text_fmt(s_lbl_sys_wifi, "Wi-Fi: %s | %s | RSSI %d dBm",
                                   ssid ? ssid : "connected",
                                   ip ? ip : "--",
                                   have_ap_info ? ap_info.rssi : 0);
        } else {
            lv_label_set_text(s_lbl_sys_wifi, "Wi-Fi: disconnected");
        }
        lv_obj_set_style_text_color(s_lbl_sys_wifi, is_connected ? lv_color_hex(COLOR_SAFE) : lv_color_hex(COLOR_TEXT), 0);
    }
    if (s_lbl_sys_mqtt) {
        lv_label_set_text_fmt(s_lbl_sys_mqtt, "MQTT/CoreIoT: %s", is_connected ? "up" : "down");
        lv_obj_set_style_text_color(s_lbl_sys_mqtt, is_connected ? lv_color_hex(COLOR_SAFE) : lv_color_hex(COLOR_TEXT), 0);
    }
}

void ui_dashboard_set_hazard_warning(bool is_pedestrian_crossing_risk)
{
    s_forced_crossing_warning = is_pedestrian_crossing_risk;
    evaluate_hazard();
}

void ui_dashboard_set_relay_state(bool relay_on, const char *warning_status)
{
    if (!s_lbl_relay_state) {
        return;
    }

    lv_label_set_text_fmt(s_lbl_relay_state, "RELAY: %s | %s",
                           relay_on ? "ON" : "OFF",
                           warning_status ? warning_status : "--");

    uint32_t color = COLOR_SAFE;
    if (warning_status) {
        if (strcmp(warning_status, "DANGER") == 0) {
            color = COLOR_DANGER;
        } else if (strcmp(warning_status, "WARNING") == 0) {
            color = COLOR_CAUTION;
        }
    }
    lv_obj_set_style_text_color(s_lbl_relay_state, lv_color_hex(color), 0);
}

void ui_dashboard_set_buzzer_state(bool buzzer_on)
{
    if (!s_lbl_buzzer_state) {
        return;
    }

    lv_label_set_text_fmt(s_lbl_buzzer_state, "BUZZER: %s", buzzer_on ? "ON" : "OFF");
    lv_obj_set_style_text_color(s_lbl_buzzer_state, buzzer_on ? lv_color_hex(COLOR_CAUTION) : lv_color_hex(COLOR_TEXT), 0);
}

void ui_dashboard_set_espnow_status(bool linked)
{
    if (!s_lbl_espnow_status) {
        return;
    }

    lv_label_set_text(s_lbl_espnow_status, linked ? "ESP-NOW: LINKED" : "ESP-NOW: NO LINK");
    lv_obj_set_style_text_color(s_lbl_espnow_status, linked ? lv_color_hex(COLOR_SAFE) : lv_color_hex(COLOR_DANGER), 0);
}
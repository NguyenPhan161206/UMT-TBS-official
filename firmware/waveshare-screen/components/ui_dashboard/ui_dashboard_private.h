/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_private.h — shared internal state + cross-file declarations for
 * the ui_dashboard split. KHÔNG phải public API (chỉ nội bộ component).
 */

#pragma once

#include "ui_dashboard_theme.h"

/* ----------------------------- widget handles ----------------------------- */
extern lv_obj_t *s_screen;
extern lv_obj_t *s_tab_btn_collision;
extern lv_obj_t *s_tab_btn_system;
extern lv_obj_t *s_page_collision;
extern lv_obj_t *s_page_system;
extern lv_obj_t *s_lbl_wifi_status;
extern lv_obj_t *s_lbl_mqtt_status;
extern lv_obj_t *s_lbl_hazard_overall;
extern lv_obj_t *s_lbl_crossing_risk;
extern lv_obj_t *s_lbl_relay_state;
extern lv_obj_t *s_lbl_buzzer_state;
extern lv_obj_t *s_lbl_sys_wifi;
extern lv_obj_t *s_lbl_sys_mqtt;
extern lv_obj_t *s_lbl_sys_device;
extern lv_obj_t *s_lbl_sys_broker;
extern lv_obj_t *s_lbl_sys_token;
extern lv_obj_t *s_lbl_sys_fw_version;
extern lv_obj_t *s_lbl_sys_idf_version;
extern lv_obj_t *s_lbl_sys_build;
extern lv_obj_t *s_lbl_sys_flash;
extern lv_obj_t *s_lbl_sys_heap;
extern lv_obj_t *s_lbl_sys_min_heap;
extern lv_obj_t *s_lbl_sys_uptime;
extern lv_timer_t *s_sys_info_timer;
extern bool s_alarm_muted;
extern lv_obj_t *s_mute_btn;
extern lv_obj_t *s_mute_btn_lbl;

typedef struct {
    lv_obj_t *arc;
    int16_t local_x;
    int16_t local_y;
    int16_t mid_angle_deg; /* LVGL angle convention: 0=right, 90=down, 180=left, 270=up */
    lv_anim_t blink_anim;
    bool blink_running;
} sensor_arc_t;

typedef struct {
    lv_obj_t *row_value_lbl;
} sensor_row_t;

extern sensor_arc_t s_arcs[SENSOR_MODEL_COUNT];
extern sensor_row_t s_rows[SENSOR_MODEL_COUNT];
extern uint16_t s_prev_distance_cm[SENSOR_MODEL_COUNT];
extern bool s_forced_crossing_warning;

/* ------------------- cross-file functions (ui_dashboard_layout) ----------- */
void arc_set_zone(sensor_arc_t *a, sensor_zone_t zone);
void arc_set_nodata(sensor_arc_t *a);
void build_header(lv_obj_t *parent);
lv_obj_t *build_left_sidebar(lv_obj_t *parent);
lv_obj_t *build_center_canvas(lv_obj_t *parent);
lv_obj_t *build_right_sidebar(lv_obj_t *parent);
lv_obj_t *build_system_page(lv_obj_t *parent);
void tab_collision_cb(lv_event_t *e);
void tab_system_cb(lv_event_t *e);
void sys_info_timer_cb(lv_timer_t *timer);

/* ------------------- cross-file functions (ui_dashboard.c) --------------- */
void evaluate_hazard(void);
void update_mute_button_visual(void);
void mute_btn_cb(lv_event_t *e);
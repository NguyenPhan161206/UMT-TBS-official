/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_layout.c — LVGL widget builders (header, sidebars, canvas,
 * tab switching) + sensor arc zone/nodata styling. SYSTEM page nằm ở
 * ui_dashboard_system.c. Tách từ ui_dashboard.c cựu repo (837 dòng) để tuân
 * R7 (<= 400 dòng/file).
 */

#include "ui_dashboard_private.h"

/* --------------------------- sensor arc styling --------------------------- */

static void blink_anim_cb(void *var, int32_t value)
{
    lv_obj_set_style_arc_opa((lv_obj_t *)var, (lv_opa_t)value, LV_PART_INDICATOR);
}

void arc_set_zone(sensor_arc_t *a, sensor_zone_t zone)
{
    lv_obj_set_style_arc_color(a->arc, zone_color(zone), LV_PART_INDICATOR);

    if (a->blink_running) {
        lv_anim_delete(a->arc, blink_anim_cb);
        a->blink_running = false;
        lv_obj_set_style_arc_opa(a->arc, LV_OPA_COVER, LV_PART_INDICATOR);
    }

    if (zone == SENSOR_ZONE_DANGER && !s_alarm_muted) {
        lv_anim_init(&a->blink_anim);
        lv_anim_set_var(&a->blink_anim, a->arc);
        lv_anim_set_exec_cb(&a->blink_anim, blink_anim_cb);
        lv_anim_set_values(&a->blink_anim, LV_OPA_COVER, LV_OPA_30);
        lv_anim_set_time(&a->blink_anim, 400);
        lv_anim_set_playback_time(&a->blink_anim, 400);
        lv_anim_set_repeat_count(&a->blink_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a->blink_anim);
        a->blink_running = true;
    } else {
        lv_obj_set_style_arc_opa(a->arc, zone == SENSOR_ZONE_SAFE ? (LV_OPA_60) : (LV_OPA_80), LV_PART_INDICATOR);
    }
}

/* Neutral "no data" style for a slot that has never reported or just went from
 * valid=1 to valid=0 - distinct from SENSOR_ZONE_SAFE so an unwired/lost sensor
 * isn't mistaken for "confirmed clear".
 */
void arc_set_nodata(sensor_arc_t *a)
{
    if (a->blink_running) {
        lv_anim_delete(a->arc, blink_anim_cb);
        a->blink_running = false;
    }
    lv_obj_set_style_arc_color(a->arc, lv_color_hex(COLOR_NODATA), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(a->arc, LV_OPA_30, LV_PART_INDICATOR);
}

static lv_obj_t *make_arc(lv_obj_t *parent, int16_t local_x, int16_t local_y, int16_t mid_angle_deg)
{
    const int32_t radius = 90;
    const int32_t half_fov = 37; /* ~75deg / 2 */

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, radius, radius);
    lv_obj_set_pos(arc, local_x - radius / 2, local_y - radius / 2);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    int32_t start = mid_angle_deg - half_fov;
    int32_t end = mid_angle_deg + half_fov;
    while (start < 0) start += 360;
    while (end < 0) end += 360;
    start %= 360;
    end %= 360;

    lv_arc_set_bg_angles(arc, start, end);
    lv_arc_set_angles(arc, start, end);
    lv_obj_set_style_arc_width(arc, 24, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 24, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_SAFE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(arc, LV_OPA_60, LV_PART_INDICATOR);

    return arc;
}

/* -------------------------------- header --------------------------------- */

void build_header(lv_obj_t *parent)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, LV_PCT(100), 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, lv_color_hex(COLOR_BORDER), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_pad_hor(header, 12, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, LV_SYMBOL_WARNING " Collision Dashboard");
    lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    s_tab_btn_collision = lv_btn_create(header);
    lv_obj_set_size(s_tab_btn_collision, 100, 28);
    lv_obj_align(s_tab_btn_collision, LV_ALIGN_CENTER, -55, 0);
    lv_obj_t *lbl1 = lv_label_create(s_tab_btn_collision);
    lv_label_set_text(lbl1, "COLLISION");
    lv_obj_center(lbl1);

    s_tab_btn_system = lv_btn_create(header);
    lv_obj_set_size(s_tab_btn_system, 100, 28);
    lv_obj_align(s_tab_btn_system, LV_ALIGN_CENTER, 55, 0);
    lv_obj_t *lbl2 = lv_label_create(s_tab_btn_system);
    lv_label_set_text(lbl2, "SYSTEM");
    lv_obj_center(lbl2);

    s_lbl_wifi_status = lv_label_create(header);
    lv_label_set_text(s_lbl_wifi_status, LV_SYMBOL_WIFI " --");
    lv_obj_set_style_text_color(s_lbl_wifi_status, lv_color_hex(COLOR_DANGER), 0);
    lv_obj_align(s_lbl_wifi_status, LV_ALIGN_RIGHT_MID, -70, 0);

    s_lbl_mqtt_status = lv_label_create(header);
    lv_label_set_text(s_lbl_mqtt_status, "MQTT: DOWN");
    lv_obj_set_style_text_color(s_lbl_mqtt_status, lv_color_hex(COLOR_DANGER), 0);
    lv_obj_align(s_lbl_mqtt_status, LV_ALIGN_RIGHT_MID, 0, 0);
}

/* --------------------------- collision page ------------------------------ */

lv_obj_t *build_left_sidebar(lv_obj_t *parent)
{
    lv_obj_t *sidebar = lv_obj_create(parent);
    lv_obj_set_size(sidebar, 180, LV_PCT(100));
    lv_obj_set_style_bg_color(sidebar, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(sidebar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sidebar, 0, 0);
    lv_obj_set_style_radius(sidebar, 0, 0);
    lv_obj_set_style_pad_all(sidebar, 8, 0);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidebar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *hdr = lv_label_create(sidebar);
    lv_label_set_text(hdr, "SENSOR READINGS");
    lv_obj_set_style_text_color(hdr, lv_color_hex(COLOR_ACCENT), 0);

    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        lv_obj_t *row = lv_label_create(sidebar);
        lv_label_set_text_fmt(row, "%s: -- cm", k_sensor_labels[i]);
        lv_obj_set_style_text_color(row, lv_color_hex(COLOR_TEXT), 0);
        s_rows[i].row_value_lbl = row;
    }

    lv_obj_t *action_hdr = lv_label_create(sidebar);
    lv_label_set_text(action_hdr, "ACTIONS");
    lv_obj_set_style_text_color(action_hdr, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_pad_top(action_hdr, 12, 0);

    s_mute_btn = lv_btn_create(sidebar);
    lv_obj_set_size(s_mute_btn, LV_PCT(100), 32);
    lv_obj_add_event_cb(s_mute_btn, mute_btn_cb, LV_EVENT_CLICKED, NULL);
    s_mute_btn_lbl = lv_label_create(s_mute_btn);
    lv_obj_center(s_mute_btn_lbl);
    update_mute_button_visual();

    lv_obj_t *calib_btn = lv_btn_create(sidebar);
    lv_obj_set_size(calib_btn, LV_PCT(100), 32);
    lv_obj_t *calib_lbl = lv_label_create(calib_btn);
    lv_label_set_text(calib_lbl, "Calibrate");
    lv_obj_center(calib_lbl);

    return sidebar;
}

lv_obj_t *build_center_canvas(lv_obj_t *parent)
{
    lv_obj_t *canvas = lv_obj_create(parent);
    lv_obj_set_size(canvas, 440, LV_PCT(100));
    lv_obj_set_style_bg_color(canvas, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(canvas, 0, 0);
    lv_obj_set_style_radius(canvas, 0, 0);
    lv_obj_set_style_pad_all(canvas, 0, 0);
    lv_obj_remove_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *car = lv_obj_create(canvas);
    lv_obj_set_size(car, 160, 260);
    lv_obj_align(car, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(car, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(car, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(car, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_border_width(car, 2, 0);
    lv_obj_set_style_radius(car, 12, 0);
    lv_obj_remove_flag(car, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hood = lv_label_create(car);
    lv_label_set_text(hood, "FRONT HOOD");
    lv_obj_set_style_text_color(hood, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(hood, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *cabin = lv_label_create(car);
    lv_label_set_text(cabin, "CABIN");
    lv_obj_set_style_text_color(cabin, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(cabin, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *trunk = lv_label_create(car);
    lv_label_set_text(trunk, "REAR TRUNK");
    lv_obj_set_style_text_color(trunk, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(trunk, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* Sensor beam arcs laid out like the standard truck "No-Zone" diagram:
     * one sensor centered front, one centered rear, two per side (front-half
     * and rear-half of that side). Angle convention: LVGL 0deg=right(3 o'clock),
     * 90=down, 180=left, 270=up. Car body spans local (140,90)-(300,350).
     */
    static const struct {
        int16_t x, y, angle;
    } k_layout[SENSOR_MODEL_COUNT] = {
        [ESPNOW_SLOT_FRONT]       = {220, 60, 270},
        [ESPNOW_SLOT_REAR]        = {220, 380, 90},
        [ESPNOW_SLOT_LEFT_FRONT]  = {90, 150, 180},
        [ESPNOW_SLOT_LEFT_REAR]   = {90, 290, 180},
        [ESPNOW_SLOT_RIGHT_FRONT] = {350, 150, 0},
        [ESPNOW_SLOT_RIGHT_REAR]  = {350, 290, 0},
    };

    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        s_arcs[i].local_x = k_layout[i].x;
        s_arcs[i].local_y = k_layout[i].y;
        s_arcs[i].mid_angle_deg = k_layout[i].angle;
        s_arcs[i].arc = make_arc(canvas, k_layout[i].x, k_layout[i].y, k_layout[i].angle);
        s_arcs[i].blink_running = false;
    }

    return canvas;
}

lv_obj_t *build_right_sidebar(lv_obj_t *parent)
{
    lv_obj_t *sidebar = lv_obj_create(parent);
    lv_obj_set_size(sidebar, 180, LV_PCT(100));
    lv_obj_set_style_bg_color(sidebar, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(sidebar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sidebar, 0, 0);
    lv_obj_set_style_radius(sidebar, 0, 0);
    lv_obj_set_style_pad_all(sidebar, 8, 0);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidebar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *hazard_hdr = lv_label_create(sidebar);
    lv_label_set_text(hazard_hdr, "HAZARD STATE");
    lv_obj_set_style_text_color(hazard_hdr, lv_color_hex(COLOR_ACCENT), 0);

    s_lbl_hazard_overall = lv_label_create(sidebar);
    lv_label_set_text(s_lbl_hazard_overall, "OVERALL: SAFE");
    lv_obj_set_style_text_color(s_lbl_hazard_overall, lv_color_hex(COLOR_SAFE), 0);

    lv_obj_t *risk_hdr = lv_label_create(sidebar);
    lv_label_set_text(risk_hdr, "CROSSING RISK");
    lv_obj_set_style_text_color(risk_hdr, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_pad_top(risk_hdr, 12, 0);

    s_lbl_crossing_risk = lv_label_create(sidebar);
    lv_label_set_text(s_lbl_crossing_risk, "None detected");
    lv_obj_set_style_text_color(s_lbl_crossing_risk, lv_color_hex(COLOR_TEXT), 0);
    lv_label_set_long_mode(s_lbl_crossing_risk, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_lbl_crossing_risk, LV_PCT(100));

    lv_obj_t *relay_hdr = lv_label_create(sidebar);
    lv_label_set_text(relay_hdr, "SERVER STATUS");
    lv_obj_set_style_text_color(relay_hdr, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_pad_top(relay_hdr, 12, 0);

    s_lbl_relay_state = lv_label_create(sidebar);
    lv_label_set_text(s_lbl_relay_state, "RELAY: -- | --");
    lv_obj_set_style_text_color(s_lbl_relay_state, lv_color_hex(COLOR_TEXT), 0);

    s_lbl_buzzer_state = lv_label_create(sidebar);
    lv_label_set_text(s_lbl_buzzer_state, "BUZZER: --");
    lv_obj_set_style_text_color(s_lbl_buzzer_state, lv_color_hex(COLOR_TEXT), 0);

    lv_obj_t *legend_hdr = lv_label_create(sidebar);
    lv_label_set_text(legend_hdr, "LEGEND");
    lv_obj_set_style_text_color(legend_hdr, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_pad_top(legend_hdr, 12, 0);

    static const struct { uint32_t color; const char *text; } k_legend[] = {
        {COLOR_SAFE, "> 100cm : Safe"},
        {COLOR_CAUTION, "30-100cm : Caution"},
        {COLOR_DANGER, "< 30cm : Danger"},
    };
    for (size_t i = 0; i < sizeof(k_legend) / sizeof(k_legend[0]); i++) {
        lv_obj_t *lbl = lv_label_create(sidebar);
        lv_label_set_text(lbl, k_legend[i].text);
        lv_obj_set_style_text_color(lbl, lv_color_hex(k_legend[i].color), 0);
    }

    return sidebar;
}

/* ------------------------------- tabs ------------------------------------ */

static void set_active_tab(bool collision)
{
    if (s_page_collision) lv_obj_add_flag(s_page_collision, LV_OBJ_FLAG_HIDDEN);
    if (s_page_system) lv_obj_add_flag(s_page_system, LV_OBJ_FLAG_HIDDEN);

    if (collision && s_page_collision) {
        lv_obj_remove_flag(s_page_collision, LV_OBJ_FLAG_HIDDEN);
    } else if (!collision && s_page_system) {
        lv_obj_remove_flag(s_page_system, LV_OBJ_FLAG_HIDDEN);
    }
}

void tab_collision_cb(lv_event_t *e)
{
    (void)e;
    set_active_tab(true);
}

void tab_system_cb(lv_event_t *e)
{
    (void)e;
    set_active_tab(false);
}
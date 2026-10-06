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
#include "ui_dashboard.h"
#include "hazard_core.h"

/* --------------------------- sensor arc styling --------------------------- */

void arc_set_zone(sensor_arc_t *a, sensor_zone_t zone)
{
    if (a->has_zone && a->current_zone == zone) {
        return;
    }
    a->has_zone = true;
    a->current_zone = zone;

    lv_obj_set_style_arc_color(a->arc, zone_color(zone), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(a->arc, LV_OPA_COVER, LV_PART_INDICATOR);
}

/* Neutral "no data" style for a slot that has never reported or just went from
 * valid=1 to valid=0 - distinct from SENSOR_ZONE_SAFE so an unwired/lost sensor
 * isn't mistaken for "confirmed clear".
 */
void arc_set_nodata(sensor_arc_t *a)
{
    if (a->has_zone && a->current_zone == (sensor_zone_t)-1) {
        return;
    }
    a->has_zone = true;
    a->current_zone = (sensor_zone_t)-1;

    lv_obj_set_style_arc_color(a->arc, lv_color_hex(COLOR_NODATA), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(a->arc, LV_OPA_COVER, LV_PART_INDICATOR);
}

static lv_obj_t *make_arc(lv_obj_t *parent, int16_t local_x, int16_t local_y, int16_t mid_angle_deg)
{
    const int32_t radius = 90;
    const int32_t half_fov = SENSOR_BEAM_FOV_DEG / 2;

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
    lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);

    return arc;
}

/* -------------------------------- header --------------------------------- */

void build_header(lv_obj_t *parent)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, LV_PCT(100), UI_HEADER_H);
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

    /* 3 tab 90 px, cách nhau 4 px, nằm giữa tiêu đề (bên trái) và nhãn ESP-NOW (bên phải). */
    s_tab_btn_collision = lv_btn_create(header);
    lv_obj_set_size(s_tab_btn_collision, 90, 28);
    lv_obj_align(s_tab_btn_collision, LV_ALIGN_CENTER, -141, 0);
    lv_obj_t *lbl1 = lv_label_create(s_tab_btn_collision);
    lv_label_set_text(lbl1, "COLLISION");
    lv_obj_center(lbl1);

    s_tab_btn_system = lv_btn_create(header);
    lv_obj_set_size(s_tab_btn_system, 90, 28);
    lv_obj_align(s_tab_btn_system, LV_ALIGN_CENTER, -47, 0);
    lv_obj_t *lbl2 = lv_label_create(s_tab_btn_system);
    lv_label_set_text(lbl2, "SYSTEM");
    lv_obj_center(lbl2);

    s_tab_btn_setup = lv_btn_create(header);
    lv_obj_set_size(s_tab_btn_setup, 90, 28);
    lv_obj_align(s_tab_btn_setup, LV_ALIGN_CENTER, 47, 0);
    lv_obj_t *lbl3 = lv_label_create(s_tab_btn_setup);
    lv_label_set_text(lbl3, "SETUP");
    lv_obj_center(lbl3);

    s_lbl_espnow_status = lv_label_create(header);
    lv_label_set_text(s_lbl_espnow_status, "ESP-NOW: --");
    lv_obj_set_style_text_color(s_lbl_espnow_status, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(s_lbl_espnow_status, LV_ALIGN_RIGHT_MID, -140, 0);

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

    /* T5.2: "Calibrate" = chỉnh vị trí/hướng cảm biến theo xe thật → mở trang SETUP (bộ chỉnh cảm biến, T4.1c). */
    lv_obj_t *calib_btn = lv_btn_create(sidebar);
    lv_obj_set_size(calib_btn, LV_PCT(100), 32);
    lv_obj_add_event_cb(calib_btn, tab_setup_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *calib_lbl = lv_label_create(calib_btn);
    lv_label_set_text(calib_lbl, "Calibrate");
    lv_obj_center(calib_lbl);

    return sidebar;
}

static lv_obj_t *s_center_canvas;
static vehicle_layout_t s_layout;

/* Dựng thân xe + 6 cung + marker trong canvas rỗng theo hồ sơ xe đang dùng. */
static void canvas_populate(lv_obj_t *canvas)
{
    const vehicle_profile_t *p = vehicle_profile_active();
    bool layout_ok = vehicle_layout_compute(p, UI_CANVAS_W, UI_CANVAS_H, UI_CANVAS_MARGIN, &s_layout);

    if (layout_ok) {
        build_truck_body(canvas, &s_layout);
    } else {
        LV_LOG_ERROR("Failed to compute vehicle layout for profile %s", p->name);
    }

    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        int16_t x = layout_ok ? s_layout.sensor_px[i].x : UI_CANVAS_W / 2;
        int16_t y = layout_ok ? s_layout.sensor_px[i].y : UI_CANVAS_H / 2;
        int16_t angle = layout_ok ? s_layout.sensor_angle_deg[i] : 0;

        s_arcs[i].local_x = x;
        s_arcs[i].local_y = y;
        s_arcs[i].mid_angle_deg = angle;
        s_arcs[i].arc = make_arc(canvas, x, y, angle);
        s_arcs[i].has_zone = false;
        s_arcs[i].current_zone = (sensor_zone_t)-1;
    }

    if (layout_ok) {
        markers_build(canvas, &s_layout);
    }
}

lv_obj_t *build_center_canvas(lv_obj_t *parent)
{
    lv_obj_t *canvas = lv_obj_create(parent);
    lv_obj_set_size(canvas, UI_CANVAS_W, LV_PCT(100));
    lv_obj_set_style_bg_color(canvas, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(canvas, 0, 0);
    lv_obj_set_style_radius(canvas, 0, 0);
    lv_obj_set_style_pad_all(canvas, 0, 0);
    lv_obj_remove_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);

    s_center_canvas = canvas;
    canvas_populate(canvas);
    return canvas;
}

void ui_dashboard_rebuild_vehicle(void)
{
    if (s_center_canvas == NULL) {
        return;
    }

    /* Mọi con trỏ widget trong canvas sắp thành dangling: quên cung/marker cũ TRƯỚC khi xoá,
     * để không có đường nào chạm vào widget đã bị xoá. */
    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        s_arcs[i].arc = NULL;
    }
    markers_reset();

    lv_obj_clean(s_center_canvas);
    canvas_populate(s_center_canvas);

    /* Áp lại trạng thái cảm biến hiện có (cung mới ra đời không có dữ liệu): cảm biến cũ/mất kết nối
     * thì xám "no data"; còn lại tô theo zone và đặt marker. Không phụ thuộc khung ESP-NOW kế tiếp. */
    sensor_reading_t readings[SENSOR_MODEL_COUNT];
    sensor_model_get_all(readings);
    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        if (s_arcs[i].arc == NULL) {
            continue;
        }
        if (readings[i].is_stale) {
            arc_set_nodata(&s_arcs[i]);
        } else {
            arc_set_zone(&s_arcs[i], hazard_classify(readings[i].distance_cm));
            marker_update((uint8_t)i, readings[i].distance_cm);
        }
    }
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
    lv_label_set_long_mode(s_lbl_hazard_overall, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_lbl_hazard_overall, LV_PCT(100));

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

    /* Legend — ngưỡng đọc TRỰC TIẾP từ thresholds.h (R3), không copy string
     * cứng (xem docs/HARDCODED_CONFIG_NOTES.md mục A). Đổi ngưỡng = tự cập nhật. */
    lv_obj_t *lbl;
    lbl = lv_label_create(sidebar);
    lv_label_set_text_fmt(lbl, "> %dcm : Safe", SENSOR_CAUTION_CM);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_SAFE), 0);

    lbl = lv_label_create(sidebar);
    lv_label_set_text_fmt(lbl, "%d-%dcm : Caution", SENSOR_DANGER_CM, SENSOR_CAUTION_CM);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_CAUTION), 0);

    lbl = lv_label_create(sidebar);
    lv_label_set_text_fmt(lbl, "<= %dcm : Danger", SENSOR_DANGER_CM);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_DANGER), 0);

    return sidebar;
}

/* ------------------------------- tabs ------------------------------------ */

static void set_active_tab(lv_obj_t *page)
{
    lv_obj_t *const pages[] = { s_page_collision, s_page_system, s_page_setup };
    for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); i++) {
        if (pages[i] == NULL) {
            continue;
        }
        if (pages[i] == page) {
            lv_obj_remove_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void tab_collision_cb(lv_event_t *e)
{
    (void)e;
    set_active_tab(s_page_collision);
}

void tab_system_cb(lv_event_t *e)
{
    (void)e;
    set_active_tab(s_page_system);
}

void tab_setup_cb(lv_event_t *e)
{
    (void)e;
    setup_page_refresh();
    set_active_tab(s_page_setup);
}
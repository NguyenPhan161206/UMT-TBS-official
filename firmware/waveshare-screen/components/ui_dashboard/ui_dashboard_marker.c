/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_marker.c — Hiển thị biểu tượng marker vật thể tại vị trí phát hiện (T3.1).
 */

#include "ui_dashboard_private.h"
#include "hazard_core.h"

typedef struct {
    lv_obj_t *dot;
    int16_t prev_x;
    int16_t prev_y;
    sensor_zone_t prev_zone;
    bool is_visible;
} marker_slot_state_t;

static marker_slot_state_t s_markers[SENSOR_MODEL_COUNT];
static vehicle_layout_t s_marker_layout;
static bool s_marker_layout_valid = false;

void markers_build(lv_obj_t *canvas, const vehicle_layout_t *L)
{
    if (canvas == NULL || L == NULL) {
        return;
    }

    s_marker_layout = *L;
    s_marker_layout_valid = true;

    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        lv_obj_t *dot = lv_obj_create(canvas);
        lv_obj_set_size(dot, 16, 16);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, zone_color(SENSOR_ZONE_SAFE), 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(dot, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(dot, 2, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);

        s_markers[i].dot = dot;
        s_markers[i].prev_x = -1;
        s_markers[i].prev_y = -1;
        s_markers[i].prev_zone = (sensor_zone_t)-1;
        s_markers[i].is_visible = false;
    }
}

void marker_update(uint8_t slot, uint16_t dist_cm)
{
    if (slot >= SENSOR_MODEL_COUNT || !s_marker_layout_valid) {
        return;
    }

    if (dist_cm > SENSOR_CAUTION_CM) {
        marker_hide(slot);
        return;
    }

    vl_point_t pt;
    if (!vehicle_layout_marker(&s_marker_layout, (espnow_slot_t)slot, dist_cm, &pt)) {
        marker_hide(slot);
        return;
    }

    marker_slot_state_t *m = &s_markers[slot];
    if (m->dot == NULL) {
        return;
    }

    sensor_zone_t zone = hazard_classify(dist_cm);
    int16_t target_x = pt.x - 8;
    int16_t target_y = pt.y - 8;

    if (!m->is_visible) {
        lv_obj_remove_flag(m->dot, LV_OBJ_FLAG_HIDDEN);
        m->is_visible = true;
        lv_obj_set_pos(m->dot, target_x, target_y);
        lv_obj_set_style_bg_color(m->dot, zone_color(zone), 0);
        m->prev_x = target_x;
        m->prev_y = target_y;
        m->prev_zone = zone;
    } else {
        if (m->prev_x != target_x || m->prev_y != target_y) {
            lv_obj_set_pos(m->dot, target_x, target_y);
            m->prev_x = target_x;
            m->prev_y = target_y;
        }
        if (m->prev_zone != zone) {
            lv_obj_set_style_bg_color(m->dot, zone_color(zone), 0);
            m->prev_zone = zone;
        }
    }
}

void marker_hide(uint8_t slot)
{
    if (slot >= SENSOR_MODEL_COUNT) {
        return;
    }
    marker_slot_state_t *m = &s_markers[slot];
    if (m->dot != NULL && m->is_visible) {
        lv_obj_add_flag(m->dot, LV_OBJ_FLAG_HIDDEN);
        m->is_visible = false;
        m->prev_zone = (sensor_zone_t)-1;
    }
}

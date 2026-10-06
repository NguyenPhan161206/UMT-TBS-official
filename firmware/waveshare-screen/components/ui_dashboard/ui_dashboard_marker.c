/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_marker.c — Biểu tượng vật thể tại vị trí phát hiện (T3.1): nhãn bo tròn hiện khoảng cách (cm),
 * nền màu theo zone, đặt trên trục búp cảm biến ở đúng khoảng cách đo (cùng tỷ lệ với thân xe).
 * Cảm biến siêu âm chỉ cho khoảng cách: đây là chỉ báo tầm, vật thật có thể lệch trong góc quét.
 */

#include "ui_dashboard_private.h"
#include "hazard_core.h"

#define MARKER_W 36                    /* đủ cho "100" (khoảng cách lớn nhất được hiện = SENSOR_CAUTION_CM) */
#define MARKER_H 20

typedef struct {
    lv_obj_t *badge;
    int16_t prev_x;
    int16_t prev_y;
    sensor_zone_t prev_zone;
    uint16_t prev_dist;
    bool is_visible;
} marker_slot_state_t;

static marker_slot_state_t s_markers[SENSOR_MODEL_COUNT];
static vehicle_layout_t s_marker_layout;
static bool s_marker_layout_valid = false;

static void marker_state_clear(marker_slot_state_t *m)
{
    m->prev_x = -1;
    m->prev_y = -1;
    m->prev_zone = (sensor_zone_t)-1;
    m->prev_dist = 0xFFFF;
    m->is_visible = false;
}

static void marker_set_zone_style(lv_obj_t *badge, sensor_zone_t zone)
{
    lv_obj_set_style_bg_color(badge, zone_color(zone), 0);
    /* Chữ sáng trên nền đỏ, chữ tối trên nền vàng/xanh cho dễ đọc. */
    lv_obj_set_style_text_color(badge, lv_color_hex(zone == SENSOR_ZONE_DANGER ? COLOR_TEXT : COLOR_BG), 0);
}

/* Quên mọi nhãn đã dựng. Gọi TRƯỚC khi xoá canvas: các con trỏ sắp dangling. */
void markers_reset(void)
{
    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        s_markers[i].badge = NULL;
        marker_state_clear(&s_markers[i]);
    }
    s_marker_layout_valid = false;
}

void markers_build(lv_obj_t *canvas, const vehicle_layout_t *L)
{
    if (canvas == NULL || L == NULL) {
        return;
    }

    s_marker_layout = *L;
    s_marker_layout_valid = true;

    for (int i = 0; i < SENSOR_MODEL_COUNT; i++) {
        lv_obj_t *badge = lv_label_create(canvas);
        lv_label_set_text(badge, "");
        lv_obj_set_size(badge, MARKER_W, MARKER_H);
        lv_obj_set_style_radius(badge, MARKER_H / 2, 0);
        lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(badge, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(badge, 2, 0);
        lv_obj_set_style_pad_all(badge, 0, 0);
        lv_obj_set_style_pad_top(badge, 1, 0);
        lv_obj_set_style_text_align(badge, LV_TEXT_ALIGN_CENTER, 0);
        marker_set_zone_style(badge, SENSOR_ZONE_SAFE);
        lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(badge, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);

        s_markers[i].badge = badge;
        marker_state_clear(&s_markers[i]);
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
    if (m->badge == NULL) {
        return;
    }

    /* Chỉ chạm widget khi vị trí / zone / số thay đổi (tránh vẽ lại liên tục ở 10 khung/s). */
    sensor_zone_t zone = hazard_classify(dist_cm);
    int16_t target_x = (int16_t)(pt.x - MARKER_W / 2);
    int16_t target_y = (int16_t)(pt.y - MARKER_H / 2);

    if (!m->is_visible) {
        lv_obj_remove_flag(m->badge, LV_OBJ_FLAG_HIDDEN);
        m->is_visible = true;
    }
    if (m->prev_x != target_x || m->prev_y != target_y) {
        lv_obj_set_pos(m->badge, target_x, target_y);
        m->prev_x = target_x;
        m->prev_y = target_y;
    }
    if (m->prev_zone != zone) {
        marker_set_zone_style(m->badge, zone);
        m->prev_zone = zone;
    }
    if (m->prev_dist != dist_cm) {
        lv_label_set_text_fmt(m->badge, "%u", (unsigned)dist_cm);
        m->prev_dist = dist_cm;
    }
}

void marker_hide(uint8_t slot)
{
    if (slot >= SENSOR_MODEL_COUNT) {
        return;
    }
    marker_slot_state_t *m = &s_markers[slot];
    if (m->badge != NULL && m->is_visible) {
        lv_obj_add_flag(m->badge, LV_OBJ_FLAG_HIDDEN);
        marker_state_clear(m);
    }
}

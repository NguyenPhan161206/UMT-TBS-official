/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_layout.c — Tính toán bố cục màn hình từ hồ sơ xe và toạ độ marker.
 */

#include "vehicle_layout.h"
#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool vehicle_layout_compute(const vehicle_profile_t *p, int16_t canvas_w, int16_t canvas_h,
                            int16_t margin_px, vehicle_layout_t *out)
{
    if (p == NULL || out == NULL) {
        return false;
    }
    if (!vehicle_profile_validate(p)) {
        return false;
    }
    if (canvas_w <= 2 * margin_px || canvas_h <= 2 * margin_px) {
        return false;
    }

    const int32_t scale_den = 10000;
    int32_t avail_w = (int32_t)canvas_w - 2 * (int32_t)margin_px;
    int32_t avail_h = (int32_t)canvas_h - 2 * (int32_t)margin_px;

    int32_t scale_w = (avail_w * scale_den) / (int32_t)p->width_mm;
    int32_t scale_h = (avail_h * scale_den) / (int32_t)p->length_mm;

    int32_t scale_num = (scale_w < scale_h) ? scale_w : scale_h;
    if (scale_num <= 0) {
        return false;
    }

    out->scale_num = scale_num;
    out->scale_den = scale_den;

    out->body_w = (int16_t)(((int32_t)p->width_mm * scale_num) / scale_den);
    out->body_h = (int16_t)(((int32_t)p->length_mm * scale_num) / scale_den);
    out->cab_h  = (int16_t)(((int32_t)p->cab_length_mm * scale_num) / scale_den);

    out->body_x = (int16_t)(((int32_t)canvas_w - (int32_t)out->body_w) / 2);
    out->body_y = (int16_t)(((int32_t)canvas_h - (int32_t)out->body_h) / 2);

    /* Trục bánh: đo từ đầu xe (mm) → px, làm tròn cùng kiểu với sensor_px. Trục sau cộng mm trước
     * rồi mới đổi sang px để không cộng dồn sai số làm tròn. */
    int32_t front_axle_mm = (int32_t)p->front_axle_mm;
    int32_t rear_axle_mm  = front_axle_mm + (int32_t)p->wheelbase_mm;
    out->front_axle_y = (int16_t)(out->body_y + (front_axle_mm * scale_num + scale_den / 2) / scale_den);
    out->rear_axle_y  = (int16_t)(out->body_y + (rear_axle_mm * scale_num + scale_den / 2) / scale_den);

    int16_t cx = (int16_t)(canvas_w / 2);
    int16_t cy = (int16_t)(canvas_h / 2);

    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        int32_t ox = ((int32_t)p->sensors[i].x_mm * scale_num);
        ox = (ox >= 0) ? (ox + scale_den / 2) / scale_den : (ox - scale_den / 2) / scale_den;

        int32_t oy = ((int32_t)p->sensors[i].y_mm * scale_num);
        oy = (oy >= 0) ? (oy + scale_den / 2) / scale_den : (oy - scale_den / 2) / scale_den;

        out->sensor_px[i].x = (int16_t)(cx + ox);
        out->sensor_px[i].y = (int16_t)(cy - oy);
        out->sensor_angle_deg[i] = p->sensors[i].angle_deg;
    }

    return true;
}

bool vehicle_layout_marker(const vehicle_layout_t *L, espnow_slot_t slot, uint16_t dist_cm, vl_point_t *out)
{
    if (L == NULL || out == NULL || (uint32_t)slot >= ESPNOW_SENSOR_SLOT_COUNT) {
        return false;
    }
    if (L->scale_den <= 0 || L->scale_num <= 0) {
        return false;
    }

    float dist_mm = (float)dist_cm * 10.0f;
    float scale_f = (float)L->scale_num / (float)L->scale_den;
    float rad = (float)L->sensor_angle_deg[slot] * ((float)M_PI / 180.0f);

    float dx = cosf(rad);
    float dy = sinf(rad);

    out->x = (int16_t)roundf((float)L->sensor_px[slot].x + dist_mm * scale_f * dx);
    out->y = (int16_t)roundf((float)L->sensor_px[slot].y + dist_mm * scale_f * dy);

    return true;
}

/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_layout.h — Tính toán tỷ lệ hình học mm → px và toạ độ marker.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "vehicle_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t x;
    int16_t y;
} vl_point_t;      /* px, gốc = góc trên-trái canvas */

typedef struct {
    int32_t scale_num, scale_den;                  /* px = mm * scale_num / scale_den (scale_den = 10000) */
    int16_t body_x, body_y, body_w, body_h;        /* thân xe (px) trong canvas */
    int16_t cab_h;                                 /* chiều cao cabin (px), tính từ đầu xe */
    vl_point_t sensor_px[ESPNOW_SENSOR_SLOT_COUNT];
    int16_t sensor_angle_deg[ESPNOW_SENSOR_SLOT_COUNT];
} vehicle_layout_t;

/* false nếu p không validate, canvas_w/h <= 2*margin_px, hoặc scale tính ra <= 0.
 * Fit: scale = min((canvas_w-2*margin)/width_mm, (canvas_h-2*margin)/length_mm); giữ tỷ lệ; xe căn giữa canvas.
 * Đầu xe ở TRÊN (y_mm dương → y_px nhỏ). */
bool vehicle_layout_compute(const vehicle_profile_t *p, int16_t canvas_w, int16_t canvas_h,
                            int16_t margin_px, vehicle_layout_t *out);

/* Điểm phát hiện cách cảm biến `dist_cm` dọc trục búp (angle_deg), CÙNG scale với thân xe.
 * false nếu slot >= COUNT hoặc out/layout NULL. Không clamp theo dist (caller quyết định hiển thị). */
bool vehicle_layout_marker(const vehicle_layout_t *L, espnow_slot_t slot, uint16_t dist_cm, vl_point_t *out);

#ifdef __cplusplus
}
#endif

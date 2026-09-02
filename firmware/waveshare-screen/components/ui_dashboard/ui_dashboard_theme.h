/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_theme.h — design tokens + sensor metadata shared by the
 * ui_dashboard split files (R7: giữ mỗi file <= 400 dòng).
 */

#pragma once

#include "lvgl.h"
#include "sensor_model.h"

/* Color tokens (dark slate canvas, industrial alert palette). */
#define COLOR_BG          0x0F172A
#define COLOR_PANEL       0x1E293B
#define COLOR_BORDER      0x334155
#define COLOR_ACCENT      0x38BDF8
#define COLOR_TEXT        0xF8FAFC
#define COLOR_SAFE        0x00C853
#define COLOR_CAUTION     0xFFD600
#define COLOR_DANGER      0xFF1744
#define COLOR_NODATA      0x64748B

/* Fast-change threshold (cm) used by the crossing-traffic hazard heuristic. */
#define CROSSING_DELTA_CM 40
#define CROSSING_FRONT_THRESHOLD_CM 150

/* Nhãn hiển thị theo thứ tự WIRE SLOT (fr=FRONT, rr=REAR, lf=LEFT_FRONT, ...)
 * — khớp espnow_slot_t / sensor_id_t. KHÔNG đổi thứ tự (sai nhãn cảm biến). */
static const char *const k_sensor_labels[SENSOR_MODEL_COUNT] = {
    "S1 (Front)", "S2 (Rear)", "S3 (L-Front)", "S4 (L-Rear)", "S5 (R-Front)", "S6 (R-Rear)",
};

static inline lv_color_t zone_color(sensor_zone_t zone)
{
    switch (zone)
    {
        case SENSOR_ZONE_DANGER: return lv_color_hex(COLOR_DANGER);
        case SENSOR_ZONE_CAUTION: return lv_color_hex(COLOR_CAUTION);
        default: return lv_color_hex(COLOR_SAFE);
    }
}
/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * hazard_core.h — pure decision logic cho cảnh báo va chạm (lớp 1, kiến trúc
 * G1 — xem docs/ARCHITECTURE_G1_TESTING.md).
 *
 * C thuần, ZERO OS/LVGL dependency → host-compile & unit-test tức thì
 * (host_sim hazard_core_tests, G1 T1.3). Input là PRIMITIVE array
 * (uint16_t* / bool*), KHÔNG phụ thuộc sensor_reading_t của sensor_model
 * → đổi struct sensor_model không lây sang core/tests.
 *
 * R2/R3/B1: chỉ include shared contract — espnow_protocol.h (slot) +
 * thresholds.h (zone + ngưỡng). CẤM include sensor_model.h.
 *
 * B5 (single-truth): hằng heuristic crossing CROSSING_* ĐÃ CHUYỂN TỪ
 * ui_dashboard_theme.h về đây (1 nguồn, grep toàn firmware == 1).
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "espnow_protocol.h" /* espnow_slot_t */
#include "thresholds.h"      /* sensor_zone_t + SENSOR_*_CM (R3) */

#ifdef __cplusplus
extern "C"
{
#endif

/* Heuristic cảnh báo xe cắt ngang (T2.3) — nguồn duy nhất (B5). */
#define CROSSING_DELTA_CM 40
#define CROSSING_FRONT_THRESHOLD_CM 150

/* Giá trị "không xác định sensor" cho hazard_crossing_result_t.sensor. */
#define HAZARD_CROSSING_NO_SENSOR ((espnow_slot_t)ESPNOW_SENSOR_SLOT_COUNT)

typedef struct {
    bool active;          /* front_close && side fast-change (heuristic cũ) */
    espnow_slot_t sensor; /* slot side "fast-change"; HAZARD_CROSSING_NO_SENSOR nếu không có */
} hazard_crossing_result_t;

/* Phân loại 1 khoảng cách (cm) theo SENSOR_*_CM (R3):
 *   x <  DANGER_CM  -> DANGER
 *   x <= CAUTION_CM -> CAUTION
 *   else            -> SAFE
 * Semantics giữ nguyên hàm classify cũ — thuần, không FreeRTOS. */
sensor_zone_t hazard_classify(uint16_t distance_cm);

/* Zone tệ nhất giữa n slot; skip slot stale (is_stale[i] == true) — giữ hành
 * vi evaluate_hazard() cũ (distance_cm=0 mặc định không bị tính thành DANGER
 * khi sensor chưa report). dist/stale NULL hoặc n==0 -> SAFE. */
sensor_zone_t hazard_worst_zone(const uint16_t *dist_cm, const bool *is_stale, size_t n);

/* Heuristic crossing-traffic (T2.3): front_close = cur[FRONT] < 150; quét side
 * slots (LEFT_FRONT..RIGHT_REAR), active khi có slot |cur-prev| >= 40.
 * KHÔNG xử lý stale (giữ hành vi cũ). NULL -> inactive, NO_SENSOR. */
hazard_crossing_result_t hazard_eval_crossing(const uint16_t *cur_cm,
                                              const uint16_t *prev_cm,
                                              size_t n);

#ifdef __cplusplus
}
#endif
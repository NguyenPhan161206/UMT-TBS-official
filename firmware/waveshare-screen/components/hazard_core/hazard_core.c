/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * hazard_core.c — pure decision logic, ZERO OS/LVGL dependency.
 * Chỉ include hazard_core.h (+ shared qua đó). KHÔNG static mutable (B2). */
#include "hazard_core.h"

sensor_zone_t hazard_classify(uint16_t distance_cm)
{
    if (distance_cm < SENSOR_DANGER_CM)
    {
        return SENSOR_ZONE_DANGER;
    }
    if (distance_cm <= SENSOR_CAUTION_CM)
    {
        return SENSOR_ZONE_CAUTION;
    }
    return SENSOR_ZONE_SAFE;
}

sensor_zone_t hazard_worst_zone(const uint16_t *dist_cm, const bool *is_stale, size_t n)
{
    sensor_zone_t worst = SENSOR_ZONE_SAFE;
    if (dist_cm == NULL || is_stale == NULL)
    {
        return worst;
    }

    for (size_t i = 0; i < n; i++)
    {
        if (is_stale[i])
        {
            continue;
        }
        sensor_zone_t z = hazard_classify(dist_cm[i]);
        if (z > worst)
        {
            worst = z;
        }
    }
    return worst;
}

hazard_crossing_result_t hazard_eval_crossing(const uint16_t *cur_cm,
                                              const uint16_t *prev_cm,
                                              size_t n)
{
    hazard_crossing_result_t result = {
        .active = false,
        .sensor = HAZARD_CROSSING_NO_SENSOR,
    };

    if (cur_cm == NULL || prev_cm == NULL || n <= (size_t)ESPNOW_SLOT_RIGHT_REAR)
    {
        return result;
    }

    /* front_close theo heuristic cũ (ui_dashboard.c): cur[FRONT] < 150. */
    bool front_close = cur_cm[ESPNOW_SLOT_FRONT] < CROSSING_FRONT_THRESHOLD_CM;

    espnow_slot_t fast_slot = HAZARD_CROSSING_NO_SENSOR;
    for (espnow_slot_t i = ESPNOW_SLOT_LEFT_FRONT; i <= ESPNOW_SLOT_RIGHT_REAR; i++)
    {
        int32_t delta = (int32_t)cur_cm[i] - (int32_t)prev_cm[i];
        if (delta < 0)
        {
            delta = -delta;
        }
        if (delta >= CROSSING_DELTA_CM)
        {
            fast_slot = i; /* giữ slot cuối chuyển nhanh (hành vi cũ) */
        }
    }

    result.active = front_close && fast_slot != HAZARD_CROSSING_NO_SENSOR;
    result.sensor = fast_slot;
    return result;
}
/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * hazard_core.c — pure decision logic, ZERO OS/LVGL dependency.
 * Chỉ include hazard_core.h (+ shared qua đó). KHÔNG static mutable (B2). */
#include "hazard_core.h"

sensor_zone_t hazard_classify(uint16_t distance_cm)
{
    if (distance_cm <= SENSOR_DANGER_CM)
    {
        return SENSOR_ZONE_DANGER;
    }
    if (distance_cm <= SENSOR_CAUTION_CM)
    {
        return SENSOR_ZONE_CAUTION;
    }
    return SENSOR_ZONE_SAFE;
}

sensor_zone_t hazard_worst_zone(const uint16_t *dist_cm,
                                const bool     *is_stale,
                                const uint8_t  *health,
                                size_t          n)
{
    sensor_zone_t worst = SENSOR_ZONE_SAFE;
    if (dist_cm == NULL || is_stale == NULL)
    {
        return worst;
    }

    for (size_t i = 0; i < n; i++)
    {
        /* Bỏ qua slot nếu: (1) is_stale, (2) health là DISCONNECTED hoặc STALE */
        if (is_stale[i])
        {
            continue;
        }
        if (health != NULL && (health[i] == (uint8_t)SENSOR_HEALTH_DISCONNECTED ||
                               health[i] == (uint8_t)SENSOR_HEALTH_STALE))
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

bool hazard_has_sensor_fault(const uint8_t *health, size_t n)
{
    if (health == NULL)
    {
        return false;
    }
    for (size_t i = 0; i < n; i++)
    {
        if (health[i] == (uint8_t)SENSOR_HEALTH_DISCONNECTED ||
            health[i] == (uint8_t)SENSOR_HEALTH_STALE)
        {
            return true;
        }
    }
    return false;
}

static bool slot_ok(const bool *ok, size_t i)
{
    return ok == NULL || ok[i];
}

hazard_crossing_result_t hazard_eval_crossing(hazard_crossing_state_t *st,
                                              const uint16_t *cur_cm,
                                              const bool *ok,
                                              size_t n,
                                              uint32_t now_ms)
{
    hazard_crossing_result_t result = {
        .active = false,
        .sensor = HAZARD_CROSSING_NO_SENSOR,
    };

    if (st == NULL || cur_cm == NULL || n < (size_t)ESPNOW_SENSOR_SLOT_COUNT)
    {
        return result;
    }

    /* Slot không hợp lệ có khoảng cách 0: không được tính là "gần" hay "đổi nhanh". */
    bool front_ok = slot_ok(ok, ESPNOW_SLOT_FRONT);
    bool front_close = front_ok && cur_cm[ESPNOW_SLOT_FRONT] < CROSSING_FRONT_THRESHOLD_CM;

    /* Chỉ hai góc TRƯỚC: vật cắt ngang trước đầu xe đi qua góc trước. Slot bên sau đổi nhanh (xe chạy dọc
     * hông) không phải "cắt ngang phía trước" — trước đây được tính và gây báo nhầm. */
    static const espnow_slot_t k_front_corners[] = {ESPNOW_SLOT_LEFT_FRONT, ESPNOW_SLOT_RIGHT_FRONT};

    espnow_slot_t fast_slot = HAZARD_CROSSING_NO_SENSOR;
    if (st->has_ref)
    {
        for (size_t k = 0; k < sizeof(k_front_corners) / sizeof(k_front_corners[0]); k++)
        {
            espnow_slot_t i = k_front_corners[k];
            if (!slot_ok(ok, i) || !st->ref_ok[i])
            {
                continue;
            }
            int32_t delta = (int32_t)cur_cm[i] - (int32_t)st->ref_cm[i];
            if (delta < 0)
            {
                delta = -delta;
            }
            if (delta >= CROSSING_DELTA_CM)
            {
                fast_slot = i; /* giữ slot cuối chuyển nhanh (hành vi cũ) */
            }
        }
    }

    if (front_close && fast_slot != HAZARD_CROSSING_NO_SENSOR)
    {
        st->holding = true;
        st->hold_until_ms = now_ms + CROSSING_HOLD_MS;
        st->hold_sensor = fast_slot;
    }
    else if (!front_ok)
    {
        st->holding = false; /* mất FRONT: không còn đánh giá được "cắt ngang phía trước" */
    }
    else if (st->holding && (int32_t)(now_ms - st->hold_until_ms) >= 0)
    {
        st->holding = false; /* hết thời gian giữ (so sánh chống tràn uint32) */
    }

    /* Làm mới mốc tham chiếu SAU khi so sánh, khi mốc đã cũ >= CROSSING_WINDOW_MS. */
    if (!st->has_ref || (uint32_t)(now_ms - st->ref_ms) >= (uint32_t)CROSSING_WINDOW_MS)
    {
        for (size_t i = 0; i < (size_t)ESPNOW_SENSOR_SLOT_COUNT; i++)
        {
            st->ref_cm[i] = cur_cm[i];
            st->ref_ok[i] = slot_ok(ok, i);
        }
        st->ref_ms = now_ms;
        st->has_ref = true;
    }

    result.active = st->holding;
    result.sensor = st->holding ? st->hold_sensor : HAZARD_CROSSING_NO_SENSOR;
    return result;
}
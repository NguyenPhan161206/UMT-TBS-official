/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_profile.c — Hồ sơ kích thước xe và toạ độ cảm biến (EX8 placeholder).
 */

#include "vehicle_profile.h"
#include <stddef.h>

_Static_assert(sizeof(((vehicle_profile_t*)0)->sensors)/sizeof(vehicle_sensor_pose_t) == ESPNOW_SENSOR_SLOT_COUNT,
               "sensors count mismatch");

/* PLACEHOLDER — thay bằng số đo EX8 thật, xem TODO trong log */
static const vehicle_profile_t s_profile_ex8 = {
    .name = "Hyundai Mighty EX8",
    .length_mm = 8000,
    .width_mm = 2500,
    .cab_length_mm = 2000,
    .sensors = {
        [ESPNOW_SLOT_FRONT]       = {     0,  4000, 270 },
        [ESPNOW_SLOT_REAR]        = {     0, -4000,  90 },
        [ESPNOW_SLOT_LEFT_FRONT]  = { -1250,  2500, 180 },
        [ESPNOW_SLOT_LEFT_REAR]   = { -1250, -2500, 180 },
        [ESPNOW_SLOT_RIGHT_FRONT] = {  1250,  2500,   0 },
        [ESPNOW_SLOT_RIGHT_REAR]  = {  1250, -2500,   0 },
    },
};

const vehicle_profile_t *vehicle_profile_active(void)
{
    return &s_profile_ex8;
}

bool vehicle_profile_validate(const vehicle_profile_t *p)
{
    if (p == NULL) {
        return false;
    }
    if (p->length_mm == 0 || p->width_mm == 0 || p->cab_length_mm == 0) {
        return false;
    }
    if (p->cab_length_mm >= p->length_mm) {
        return false;
    }

    int32_t half_w = (int32_t)(p->width_mm / 2) + 100;
    int32_t half_l = (int32_t)(p->length_mm / 2) + 100;

    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        const vehicle_sensor_pose_t *s = &p->sensors[i];
        if (s->angle_deg < 0 || s->angle_deg > 359) {
            return false;
        }
        if (s->x_mm < -half_w || s->x_mm > half_w) {
            return false;
        }
        if (s->y_mm < -half_l || s->y_mm > half_l) {
            return false;
        }
    }

    return true;
}

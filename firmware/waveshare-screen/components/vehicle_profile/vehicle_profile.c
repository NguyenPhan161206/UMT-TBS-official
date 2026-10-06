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

/* Quy tắc đặt cảm biến của MỌI hồ sơ placeholder: FRONT/REAR ở giữa hai đầu xe; 4 cảm biến bên ở mép thân,
 * cách đầu/đuôi xe 1500 mm, hướng thẳng ngang (180 = trái, 0 = phải). Chưa phải vị trí lắp thật. */

/* id = khoá lưu NVS: KHÔNG đổi, KHÔNG dùng lại.
 *
 * EX8: dài, rộng, chiều dài cơ sở lấy từ bảng thông số kỹ thuật Hyundai New Mighty EX8 do người dùng cung cấp
 * (2026-10-05; kích thước tổng thể 7370 x 2028 x 2310 mm, chiều dài cơ sở 3850 mm; bảng KHÔNG có kích thước thùng
 * hàng nên chiều dài tổng cần xác nhận với xe thật). Các trường còn lại CHƯA có số đo thật — PLACEHOLDER:
 * cab_length_mm, front_axle_mm và toạ độ + góc 6 cảm biến (đặt theo quy tắc chung ở trên, chưa phải vị trí lắp thật).
 * Cần đo trên xe: xem docs/logs/WAVESHARE_SCREEN_VEHICLE_PROFILE_LOG.md. */
static const vehicle_profile_t s_profiles[] = {
    {
        .id = 1,
        .name = "Hyundai Mighty EX8",
        .length_mm = 7370,           /* bảng hãng */
        .width_mm = 2028,            /* bảng hãng */
        .cab_length_mm = 2000,       /* PLACEHOLDER */
        .front_axle_mm = 1400,       /* PLACEHOLDER (phần nhô trước chưa đo); trục sau = 1400 + 3850 = 5250 mm */
        .wheelbase_mm = 3850,        /* bảng hãng */
        .sensors = {                 /* PLACEHOLDER: ±3685 = nửa chiều dài; bên: ±1014 = nửa chiều rộng, cách đầu/đuôi 1500 */
            [ESPNOW_SLOT_FRONT]       = {     0,  3685, 270 },
            [ESPNOW_SLOT_REAR]        = {     0, -3685,  90 },
            [ESPNOW_SLOT_LEFT_FRONT]  = { -1014,  2185, 180 },
            [ESPNOW_SLOT_LEFT_REAR]   = { -1014, -2185, 180 },
            [ESPNOW_SLOT_RIGHT_FRONT] = {  1014,  2185,   0 },
            [ESPNOW_SLOT_RIGHT_REAR]  = {  1014, -2185,   0 },
        },
    },
    {   /* PLACEHOLDER tuỳ ý, không phải xe thật. Tên ASCII vì font LVGL không có dấu tiếng Việt. */
        .id = 2,
        .name = "Small truck (placeholder)",
        .length_mm = 5500,
        .width_mm = 2100,
        .cab_length_mm = 1500,
        .front_axle_mm = 1100,
        .wheelbase_mm = 3200,        /* trục sau ở 4300 mm */
        .sensors = {
            [ESPNOW_SLOT_FRONT]       = {     0,  2750, 270 },
            [ESPNOW_SLOT_REAR]        = {     0, -2750,  90 },
            [ESPNOW_SLOT_LEFT_FRONT]  = { -1050,  1250, 180 },
            [ESPNOW_SLOT_LEFT_REAR]   = { -1050, -1250, 180 },
            [ESPNOW_SLOT_RIGHT_FRONT] = {  1050,  1250,   0 },
            [ESPNOW_SLOT_RIGHT_REAR]  = {  1050, -1250,   0 },
        },
    },
    {   /* PLACEHOLDER tuỳ ý, không phải xe thật. */
        .id = 3,
        .name = "Large truck (placeholder)",
        .length_mm = 12000,
        .width_mm = 2500,
        .cab_length_mm = 2200,
        .front_axle_mm = 1600,
        .wheelbase_mm = 8000,        /* trục sau ở 9600 mm */
        .sensors = {
            [ESPNOW_SLOT_FRONT]       = {     0,  6000, 270 },
            [ESPNOW_SLOT_REAR]        = {     0, -6000,  90 },
            [ESPNOW_SLOT_LEFT_FRONT]  = { -1250,  4500, 180 },
            [ESPNOW_SLOT_LEFT_REAR]   = { -1250, -4500, 180 },
            [ESPNOW_SLOT_RIGHT_FRONT] = {  1250,  4500,   0 },
            [ESPNOW_SLOT_RIGHT_REAR]  = {  1250, -4500,   0 },
        },
    },
};

#define PROFILE_COUNT (sizeof(s_profiles) / sizeof(s_profiles[0]))

/* Hồ sơ đang dùng: bản copy, con trỏ ổn định. Chưa set thì lười khởi tạo bằng hồ sơ đầu (EX8). */
static vehicle_profile_t s_active;
static bool s_active_valid = false;

size_t vehicle_profile_count(void)
{
    return PROFILE_COUNT;
}

const vehicle_profile_t *vehicle_profile_get(size_t index)
{
    return (index < PROFILE_COUNT) ? &s_profiles[index] : NULL;
}

const vehicle_profile_t *vehicle_profile_find(uint8_t id)
{
    for (size_t i = 0; i < PROFILE_COUNT; i++) {
        if (s_profiles[i].id == id) {
            return &s_profiles[i];
        }
    }
    return NULL;
}

bool vehicle_profile_set_active(const vehicle_profile_t *p)
{
    if (!vehicle_profile_validate(p)) {
        return false;
    }
    s_active = *p;
    s_active_valid = true;
    return true;
}

const vehicle_profile_t *vehicle_profile_active(void)
{
    if (!s_active_valid) {
        s_active = s_profiles[0];
        s_active_valid = true;
    }
    return &s_active;
}

bool vehicle_profile_validate(const vehicle_profile_t *p)
{
    if (p == NULL) {
        return false;
    }
    if (p->name == NULL || p->id == 0) {
        return false;
    }
    if (p->length_mm == 0 || p->width_mm == 0 || p->cab_length_mm == 0) {
        return false;
    }
    if (p->cab_length_mm >= p->length_mm) {
        return false;
    }
    if (p->front_axle_mm == 0 || p->wheelbase_mm == 0 ||
        (uint32_t)p->front_axle_mm + p->wheelbase_mm >= p->length_mm) {
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

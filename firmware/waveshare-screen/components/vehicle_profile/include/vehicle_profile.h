/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_profile.h — Định nghĩa cấu trúc hồ sơ kích thước xe và bố trí cảm biến.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "espnow_protocol.h"          /* ESPNOW_SENSOR_SLOT_COUNT, espnow_slot_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Hệ toạ độ xe: gốc = tâm hình chữ nhật thân xe nhìn từ trên xuống.
 * x_mm: dương = sang PHẢI xe; y_mm: dương = về phía ĐẦU xe.
 * angle_deg: hướng búp theo quy ước LVGL (0=phải, 90=xuống/đuôi, 180=trái, 270=lên/đầu) */
typedef struct {
    int16_t x_mm;
    int16_t y_mm;
    int16_t angle_deg;
} vehicle_sensor_pose_t;

typedef struct {
    const char *name;
    uint16_t length_mm;               /* đầu → đuôi */
    uint16_t width_mm;
    uint16_t cab_length_mm;           /* chiều dài cabin tính từ đầu xe */
    vehicle_sensor_pose_t sensors[ESPNOW_SENSOR_SLOT_COUNT];   /* index = espnow_slot_t */
} vehicle_profile_t;

const vehicle_profile_t *vehicle_profile_active(void);          /* hiện tại: EX8, hằng static const */
bool vehicle_profile_validate(const vehicle_profile_t *p);      /* false nếu NULL/kích thước 0/cab>=length/angle ngoài 0..359/sensor ngoài bbox xe quá 100mm */

#ifdef __cplusplus
}
#endif

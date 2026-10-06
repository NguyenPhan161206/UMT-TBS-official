/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_profile.h — Định nghĩa cấu trúc hồ sơ kích thước xe và bố trí cảm biến.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
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
    uint8_t id;                       /* khoá ổn định (lưu NVS), 0 = không hợp lệ; KHÔNG đổi khi thêm hồ sơ khác */
    const char *name;
    uint16_t length_mm;               /* đầu → đuôi */
    uint16_t width_mm;
    uint16_t cab_length_mm;           /* chiều dài cabin tính từ đầu xe */
    uint16_t front_axle_mm;           /* đầu xe → tâm trục trước (phần nhô trước) */
    uint16_t wheelbase_mm;            /* chiều dài cơ sở; trục sau nằm ở front_axle_mm + wheelbase_mm tính từ đầu xe */
    vehicle_sensor_pose_t sensors[ESPNOW_SENSOR_SLOT_COUNT];   /* index = espnow_slot_t */
} vehicle_profile_t;

/* Hồ sơ đang dùng: bản COPY nội bộ, con trỏ ỔN ĐỊNH (không đổi sau vehicle_profile_set_active).
 * Mặc định = hồ sơ đầu tiên của registry (EX8). Mọi truy cập từ LVGL task. */
const vehicle_profile_t *vehicle_profile_active(void);

/* Registry các hồ sơ dựng sẵn (hằng static const). */
size_t vehicle_profile_count(void);
const vehicle_profile_t *vehicle_profile_get(size_t index);     /* NULL nếu index >= count */
const vehicle_profile_t *vehicle_profile_find(uint8_t id);      /* NULL nếu không có id đó */

/* validate rồi COPY p vào hồ sơ đang dùng. false (NULL/không hợp lệ) thì giữ nguyên hồ sơ cũ. */
bool vehicle_profile_set_active(const vehicle_profile_t *p);
/* false nếu NULL/name NULL/id 0/kích thước 0/cab>=length/trục bánh không nằm trong thân xe
 * (front_axle_mm>0, wheelbase_mm>0, front_axle_mm+wheelbase_mm<length)/angle ngoài 0..359/sensor ngoài bbox xe quá 100mm */
bool vehicle_profile_validate(const vehicle_profile_t *p);

#ifdef __cplusplus
}
#endif

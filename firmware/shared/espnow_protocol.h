/*
 * SPDX-FileCopyrightText: 2026 Truck Blind-spot Warning System
 * SPDX-License-Identifier: MIT
 *
 * espnow_protocol.h — SHARED ESP-NOW WIRE CONTRACT (R2).
 *
 * SINGLE definition of the ESP-NOW payload exchanged between
 * sensor-node -> waveshare-screen. Previously this struct + channel + MAC
 * were copy-pasted in both firmwares and "kept in sync by hand" — a
 * mismatch source (R2 violation in the old repo). Now it lives HERE only.
 *
 * Cấm định nghĩa lại espnow_sensor_msg_t / espnow_slot_t / channel / MAC
 * ở bất kỳ đâu khác; grep toàn firmware phải == 1.
 */
#pragma once

#include <stdint.h>

#include "thresholds.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Cả 2 board phải ở cùng WiFi channel cố định. */
#define ESPNOW_CHANNEL 1

/* MAC đích (waveshare-screen, receiver) mà sensor-node gửi tới.
 * Cập nhật theo docs/architecture/ESPNOW_NETWORK.md nếu board đổi. */
static const uint8_t ESPNOW_PEER_MAC[6] = {0x64, 0xe8, 0x33, 0x7c, 0x3f, 0xe0};

/* Tần suất gửi ESP-NOW (tách biệt MEASURE_INTERVAL_MS - vòng đo/lọc cục bộ). */
#define ESPNOW_SEND_INTERVAL_MS 500
#define ESPNOW_LINK_TIMEOUT_MS 1500

/* Số vị trí cảm biến trên "dây" — PHẢI khớp SENSOR_COUNT (thresholds.h).
 * Cố định 6: khớp mô hình sensor_model/ui_dashboard bên waveshare-screen. */
#define ESPNOW_SENSOR_SLOT_COUNT 6

/* Thứ tự slot trên dây == thứ tự vật lý trong thresholds.h::SENSOR_PINS.
 * Ánh xạ thẳng sang sensor_id_t bên waveshare-screen. */
typedef enum {
    ESPNOW_SLOT_FRONT = 0,       /* S0  */
    ESPNOW_SLOT_REAR = 1,        /* S1  */
    ESPNOW_SLOT_LEFT_FRONT = 2,  /* S2  */
    ESPNOW_SLOT_LEFT_REAR = 3,   /* S3  */
    ESPNOW_SLOT_RIGHT_FRONT = 4, /* S4  */
    ESPNOW_SLOT_RIGHT_REAR = 5,  /* S5  */
} espnow_slot_t;

/* Payload nhị phân gửi qua ESP-NOW - packed, kích thước cố định.
 * Luôn gửi đủ ESPNOW_SENSOR_SLOT_COUNT vị trí; valid[i]=0 nghĩa là "null"
 * cho slot đó (cảm biến lỗi/mất tín hiệu hoặc chưa lắp) - bên waveshare
 * phải xử lý hiển thị khi valid[i]=0, không đọc distance_cm[i] làm số hợp lệ. */
typedef struct __attribute__((packed)) {
    float distance_cm[ESPNOW_SENSOR_SLOT_COUNT];
    uint8_t valid[ESPNOW_SENSOR_SLOT_COUNT];
} espnow_sensor_msg_t;

/* Chặn lệch giữa số slot trên dây và số cảm biến vật lý (R2/R4). */
TBS_STATIC_ASSERT(ESPNOW_SENSOR_SLOT_COUNT == SENSOR_COUNT,
                  "ESP-NOW wire slot count must match physical SENSOR_COUNT");

#ifdef __cplusplus
}
#endif

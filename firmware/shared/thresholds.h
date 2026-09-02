/*
 * SPDX-FileCopyrightText: 2026 Truck Blind-spot Warning System
 * SPDX-License-Identifier: MIT
 *
 * thresholds.h — SHARED CONTRACT (R2/R3/R4).
 *
 * SINGLE source of truth for every threshold, zone semantic, measurement
 * parameter and physical sensor layout used by BOTH firmwares:
 *   - firmware/sensor-node        (Arduino / C++, ESP32-S3)
 *   - firmware/waveshare-screen   (ESP-IDF / C,   ESP32-S3)
 *
 * Cấm định nghĩa lại bất kỳ symbol nào trong file này tại nơi khác (R2/R3);
 * nếu cần đổi ngưỡng => sửa DUY NHẤT tại đây.
 *
 * SENSOR_COUNT được suy từ SENSOR_PINS bằng sizeof() (R4), không gõ tay,
 * kèm static_assert để chặn lệch khi thêm/bớt cảm biến.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================
 * Vùng phân loại cảnh báo (dùng chung cả 2 board)
 * ================================================================== */

/* Dải do hợp lệ của JSN-SR04T (datasheet 20–600 cm). */
#define SENSOR_RANGE_MIN_CM 20
#define SENSOR_RANGE_MAX_CM 600
#define SENSOR_BEAM_FOV_DEG 75

typedef enum {
    SENSOR_ZONE_SAFE = 0,   /* > CAUTION_CM            */
    SENSOR_ZONE_CAUTION = 1,/* DANGER_CM < x <= CAUTION */
    SENSOR_ZONE_DANGER = 2, /* x <= DANGER_CM          */
} sensor_zone_t;

/* Ngưỡng zone (cm) — nguồn duy nhất (R3). */
#define SENSOR_CAUTION_CM 100
#define SENSOR_DANGER_CM 30

/* ==================================================================
 * Cảnh báo còi (buzzer) — vật lý nằm trên sensor-node
 * ================================================================== */

#define BUZZER_PIN 11
#define BUZZER_WARNING_DISTANCE_CM 50.0f
#define BUZZER_DANGER_DISTANCE_CM 20.0f
#define BUZZER_WARNING_PERIOD_MS 3000
#define BUZZER_DANGER_PERIOD_MS 1000
#define BUZZER_BEEP_ON_MS 120

/* ==================================================================
 * Tham số đo/lọc cảm biến (sensor-node)
 * ================================================================== */

#define MIN_DISTANCE_CM 15.0f
#define MAX_DISTANCE_CM 500.0f
#define SOUND_SPEED_CM_PER_US 0.0343f
#define ECHO_TIMEOUT_US 40000
#define ECHO_LOW_TIMEOUT_US 5000
#define TRIGGER_LOW_US 5
#define TRIGGER_HIGH_US 20
#define MEASURE_INTERVAL_MS 100

/* Bộ lọc cluster-EMA */
#define FILTER_HISTORY_SIZE 9
#define FILTER_MIN_SAMPLES 5
#define FILTER_MIN_CLUSTER_SIZE 5
#define FILTER_BASE_CLUSTER_TOLERANCE_CM 8.0f
#define FILTER_CLUSTER_TOLERANCE_RATIO 0.08f
#define FILTER_EMA_ALPHA 0.30f
#define FILTER_MIN_JUMP_THRESHOLD_CM 30.0f
#define FILTER_JUMP_THRESHOLD_RATIO 0.25f
#define FILTER_JUMP_CONFIRM_COUNT 3
#define FILTER_BASE_JUMP_TOLERANCE_CM 12.0f
#define FILTER_JUMP_TOLERANCE_RATIO 0.10f
#define FILTER_RESET_AFTER_INVALID 15

/* ==================================================================
 * Layout vật lý 6 cảm biến (sensor-node)
 *
 * KHÔNG dùng GPIO47/48: chip Embedded PSRAM 8MB chiếm 2 chân đó làm
 * SPICLK_P/N_DIFF cho PSRAM (xem log cũ GPIO47_48_PSRAM) -> REAR dùng
 * {3,4}.
 *
 * LƯU Ý: thứ tự mảng này KHÔNG trùng espnow_protocol.h::espnow_slot_t.
 * Sensor-node phải giữ ánh xạ SENSOR_ESPNOW_SLOT[] (vật lý -> wire) ở
 * main.cpp; xoá ánh xạ đó = sai nhãn trên dashboard.
 * ================================================================== */

typedef struct {
    uint8_t trigPin;
    uint8_t echoPin;
} sensor_pin_cfg_t;

static const sensor_pin_cfg_t SENSOR_PINS[] = {
    {5, 6},   /* 0 FRONT        */
    {7, 8},   /* 1 LEFT_FRONT   */
    {9, 10},  /* 2 RIGHT_FRONT  */
    {17, 18}, /* 3 LEFT_REAR    */
    {21, 38}, /* 4 RIGHT_REAR   */
    {3, 4},   /* 5 REAR         */
};

#define SENSOR_COUNT (sizeof(SENSOR_PINS) / sizeof(SENSOR_PINS[0]))

#if defined(__cplusplus)
#define TBS_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define TBS_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* Chặn lệch số cảm biến vật lý so với 6 slot (R4). */
TBS_STATIC_ASSERT(SENSOR_COUNT == 6, "SENSOR_COUNT must equal 6 sensor slots");

#ifdef __cplusplus
}
#endif

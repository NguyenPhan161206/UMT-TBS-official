/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * sensor_model.h — thread-safe sensor state container for waveshare-screen.
 *
 * R2/R3: Số slot, vùng zone, ngưỡng, dải đo LẤY từ firmware/shared/
 * (espnow_protocol.h + thresholds.h) — KHÔNG định nghĩa lại. Số cảm biến
 * = ESPNOW_SENSOR_SLOT_COUNT (đã khớp SENSOR_COUNT nhờ static_assert).
 * sensor_id_t đặt trùng espnow_slot_t (FRONT=0, REAR=1, LEFT_FRONT=2, ...).
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "espnow_protocol.h"
#include "thresholds.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Vị trí cảm biến khớp wire slot; ánh xạ thẳng espnow_slot_t. */
typedef espnow_slot_t sensor_id_t;
#define SENSOR_MODEL_COUNT ESPNOW_SENSOR_SLOT_COUNT

/* Layout mirror tiêu chuẩn "No-Zone": 1 trước, 1 sau, 2 mỗi bên. */
#define SENSOR_BEAM_FOV_DEG 75

typedef struct {
    uint16_t distance_cm;
    int16_t offset_deg;
    bool is_stale;
} sensor_reading_t;

/**
 * @brief Initialize the thread-safe sensor model (mutex + default values).
 */
void sensor_model_init(void);

/**
 * @brief Update the distance reading for a single sensor. Thread-safe.
 */
void sensor_model_set_distance(sensor_id_t id, uint16_t distance_cm);

/**
 * @brief Mark a sensor as having no data ("null" reading - sensor-node reported
 *        valid=0 for this slot, e.g. no hardware wired or a lost echo). Resets
 *        is_stale to true so callers (hazard evaluation, UI) stop treating the
 *        last distance_cm as live. Thread-safe.
 */
void sensor_model_clear(sensor_id_t id);

/**
 * @brief Read a single sensor's current reading. Thread-safe.
 */
sensor_reading_t sensor_model_get(sensor_id_t id);

/**
 * @brief Snapshot all sensor readings into the provided array. Thread-safe.
 */
void sensor_model_get_all(sensor_reading_t out[SENSOR_MODEL_COUNT]);

/**
 * @brief Classify a distance reading into a hazard zone using the shared
 *        thresholds (R3: SENSOR_CAUTION_CM / SENSOR_DANGER_CM).
 */
sensor_zone_t sensor_model_classify(uint16_t distance_cm);

#ifdef __cplusplus
}
#endif
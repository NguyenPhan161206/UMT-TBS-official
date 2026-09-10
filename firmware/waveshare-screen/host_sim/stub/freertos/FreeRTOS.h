/*
 * SPDX-PackageName: umt_host_sim (stub)
 * Stub của FreeRTOS.h — CHỈ dùng cho host_sim (T1.2), không phải firmware thật.
 * Cung cấp đủ symbol để sensor_model.c compile trên host.
 */
#pragma once

#include <stdint.h>

#define portMAX_DELAY (UINT32_MAX)
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS))

typedef uint32_t TickType_t;
#define portTICK_PERIOD_MS 1
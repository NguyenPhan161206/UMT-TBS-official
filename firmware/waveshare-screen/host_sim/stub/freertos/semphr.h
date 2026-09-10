/*
 * SPDX-PackageName: umt_host_sim (stub)
 * Stub của freertos/semphr.h — CHỈ dùng cho host_sim (T1.2).
 * Mutex no-op: trên host mọi truy cập là single-thread.
 */
#pragma once

#include "freertos/FreeRTOS.h"

typedef void *SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    static int s_dummy;
    return (SemaphoreHandle_t)&s_dummy;
}

static inline void xSemaphoreTake(SemaphoreHandle_t mutex, uint32_t ticks)
{
    (void)mutex;
    (void)ticks;
}

static inline void xSemaphoreGive(SemaphoreHandle_t mutex)
{
    (void)mutex;
}
/*
 * SPDX-PackageName: umt_host_sim (stub)
 * esp_timer.h stub — đồng hồ monotonic (microseconds).
 */
#pragma once

#include <stdint.h>
#include <time.h>

static inline int64_t esp_timer_get_time(void)
{
    static clockid_t clk = CLOCK_MONOTONIC;
    struct timespec ts;
    clock_gettime(clk, &ts);
    return (int64_t)ts.tv_sec * 1000000 + (int64_t)ts.tv_nsec / 1000;
}
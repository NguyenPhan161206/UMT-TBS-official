/*
 * SPDX-PackageName: umt_host_sim (stub)
 * esp_chip_info.h stub — ESP32-S3 fake.
 */
#pragma once

typedef struct {
    int cores;
    int revision;
} esp_chip_info_t;

static inline void esp_chip_info(esp_chip_info_t *out_info)
{
    out_info->cores = 2;
    out_info->revision = 0;
}
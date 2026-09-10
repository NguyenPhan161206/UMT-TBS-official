/*
 * SPDX-PackageName: umt_host_sim (stub)
 * esp_flash.h stub — báo flash 8 MB fake cho SYSTEM page.
 */
#pragma once

#include "esp_err.h"

#include <stdint.h>

static inline esp_err_t esp_flash_get_size(const void *chip, uint32_t *out_size)
{
    (void)chip;
    *out_size = 8 * 1024 * 1024; /* 8 MB */
    return ESP_OK;
}
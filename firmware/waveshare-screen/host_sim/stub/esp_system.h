/*
 * SPDX-PackageName: umt_host_sim (stub)
 * esp_system.h stub — heap/idf version fake cho SYSTEM page.
 */
#pragma once

#include "esp_err.h"

#include <stddef.h>

static inline size_t esp_get_free_heap_size(void)
{
    return 245760; /* 240 KB fake */
}

static inline size_t esp_get_minimum_free_heap_size(void)
{
    return 122880; /* 120 KB fake */
}

static const char *esp_get_idf_version(void)
{
    return "v5.5.0 (host_sim)";
}
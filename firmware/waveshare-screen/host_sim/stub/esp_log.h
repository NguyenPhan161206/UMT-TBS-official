/*
 * SPDX-PackageName: umt_host_sim (stub)
 * esp_log.h stub — in ra stdout (host_sim chạy native).
 */
#pragma once

#include <stdio.h>
#include <string.h>

#define ESP_LOGI(tag, fmt, ...) printf("[%s] " fmt "\n", (tag), ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("[%s][WARN] " fmt "\n", (tag), ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("[%s][ERROR] " fmt "\n", (tag), ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) /* no debug on host */
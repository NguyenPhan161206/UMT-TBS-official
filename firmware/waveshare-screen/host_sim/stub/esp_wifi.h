/*
 * SPDX-PackageName: umt_host_sim (stub)
 * esp_wifi.h stub — ui_dashboard.c gọi esp_wifi_sta_get_ap_info; host trả
 * ESP_FAIL để UI hiển thị trạng thái "disconnected/no AP info" như mong đợi.
 */
#pragma once

#include "esp_err.h"

#include <stdint.h>

typedef struct {
    char ssid[33];
    int8_t rssi;
} wifi_ap_record_t;

static inline esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info)
{
    (void)ap_info;
    return ESP_FAIL;
}
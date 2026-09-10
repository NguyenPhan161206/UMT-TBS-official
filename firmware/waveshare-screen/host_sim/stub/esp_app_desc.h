/*
 * SPDX-PackageName: umt_host_sim (stub)
 * esp_app_desc.h stub — giữ PROJECT_NAME/VERSION đúng nghĩa đủ cho SYSTEM page.
 */
#pragma once

#define PROJECT_NAME "umt_dash_sim"
#define PROJECT_VER  "0.0.0-sim"

typedef struct {
    char project_name[32];
    char version[16];
    char date[16];
    char time[16];
} esp_app_desc_t;

static const esp_app_desc_t *esp_app_get_description(void)
{
    static const esp_app_desc_t desc = {
        .project_name = PROJECT_NAME,
        .version = PROJECT_VER,
        .date = __DATE__,
        .time = __TIME__,
    };
    return &desc;
}
/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * soak_heartbeat.c — dòng BOOT/SOAK cho soak test 24 h (DMXT-58).
 * In bằng printf (không qua ESP_LOG) để không bị lọc theo log level và giữ đúng định dạng
 * "SOAK scr k=v ..." mà tools/soak/soak_logger.py đọc.
 */

#include "soak_heartbeat.h"

#include <stdint.h>
#include <stdio.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

#include "coreiot_client.h"
#include "espnow_receiver.h"
#include "soak_diag.h"

/* tbs_reset_reason_name() nhận int theo thứ tự esp_reset_reason_t — chặn lệch nếu IDF đổi enum. */
_Static_assert(ESP_RST_POWERON == 1 && ESP_RST_PANIC == 4 && ESP_RST_TASK_WDT == 6 &&
                   ESP_RST_BROWNOUT == 9,
               "esp_reset_reason_t khac bang trong firmware/shared/soak_diag.h");

#define SOAK_TASK_STACK 4096
/* Tên task LVGL do esp_lvgl_adapter đặt (esp_lv_adapter.c). */
#define LVGL_TASK_NAME "lvgl"

static const char *TAG = "soak";
static uint32_t s_boot_count = 0;
static const char *s_reset_reason = "UNKNOWN";
static volatile uint32_t s_link_down = 0;

void soak_heartbeat_note_link_down(void)
{
    s_link_down = s_link_down + 1;
}

static void soak_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(TBS_SOAK_HEARTBEAT_INTERVAL_MS));

        TaskHandle_t lvgl = xTaskGetHandle(LVGL_TASK_NAME);
        long hwm_lvgl = lvgl != NULL ? (long)uxTaskGetStackHighWaterMark(lvgl) : -1L;

        printf("SOAK scr up=%lu boot=%lu rr=%s iheap=%u iminheap=%u iblk=%u psram=%u psrammin=%u "
               "rx=%lu linkdn=%lu maxgap_ms=%lu mqtt_rc=%lu hwm_lvgl=%ld\n",
               (unsigned long)(esp_timer_get_time() / 1000000LL),
               (unsigned long)s_boot_count, s_reset_reason,
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
               (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM),
               (unsigned long)espnow_receiver_rx_count(),
               (unsigned long)s_link_down,
               (unsigned long)espnow_receiver_take_max_gap_ms(),
               (unsigned long)coreiot_client_reconnect_count(),
               hwm_lvgl);
    }
}

void soak_heartbeat_start(void)
{
    s_reset_reason = tbs_reset_reason_name((int)esp_reset_reason());

    nvs_handle_t h;
    if (nvs_open("diag", NVS_READWRITE, &h) == ESP_OK) {
        uint32_t n = 0;
        nvs_get_u32(h, "boot", &n); /* chưa có key -> giữ 0 */
        s_boot_count = n + 1;
        nvs_set_u32(h, "boot", s_boot_count);
        nvs_commit(h);
        nvs_close(h);
    } else {
        ESP_LOGW(TAG, "nvs_open(diag) failed — boot counter = 0");
    }
    printf("BOOT scr boot=%lu rr=%s\n", (unsigned long)s_boot_count, s_reset_reason);

    if (xTaskCreate(soak_task, "soak", SOAK_TASK_STACK, NULL, 1, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Cannot create soak task");
    }
}

/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_system.c — SYSTEM page builder + periodic info refresher.
 * Tách từ ui_dashboard_layout.c (R7: <= 400 dòng/file).
 */

#include "ui_dashboard_private.h"

#include "coreiot_client.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *make_sys_column(lv_obj_t *parent)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, LV_PCT(50), LV_PCT(100));
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 8, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    return col;
}

static void make_sys_section_header(lv_obj_t *parent, const char *text)
{
    lv_obj_t *hdr = lv_label_create(parent);
    lv_label_set_text(hdr, text);
    lv_obj_set_style_text_color(hdr, lv_color_hex(COLOR_ACCENT), 0);
    lv_obj_set_style_pad_top(hdr, 12, 0);
}

static lv_obj_t *make_sys_info_row(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_TEXT), 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, LV_PCT(100));
    return lbl;
}

/* Masks a secret so only the first/last few characters are visible (R1: không
 * in toàn bộ token lên màn hình). */
static void mask_secret(const char *secret, char *out, size_t out_size)
{
    size_t len = strlen(secret);
    if (len <= 8) {
        snprintf(out, out_size, "%s", secret);
        return;
    }
    snprintf(out, out_size, "%.4s...%s", secret, secret + len - 4);
}

static void format_uptime(char *out, size_t out_size)
{
    int64_t uptime_s = esp_timer_get_time() / 1000000;
    int hours = (int)(uptime_s / 3600);
    int mins = (int)((uptime_s % 3600) / 60);
    int secs = (int)(uptime_s % 60);
    snprintf(out, out_size, "Uptime: %02d:%02d:%02d", hours, mins, secs);
}

static void refresh_system_info(void)
{
    if (s_lbl_sys_heap) {
        lv_label_set_text_fmt(s_lbl_sys_heap, "Free heap: %u KB", (unsigned)(esp_get_free_heap_size() / 1024));
    }
    if (s_lbl_sys_min_heap) {
        lv_label_set_text_fmt(s_lbl_sys_min_heap, "Min free heap: %u KB", (unsigned)(esp_get_minimum_free_heap_size() / 1024));
    }
    if (s_lbl_sys_uptime) {
        char buf[32];
        format_uptime(buf, sizeof(buf));
        lv_label_set_text(s_lbl_sys_uptime, buf);
    }
}

void sys_info_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    refresh_system_info();
}

lv_obj_t *build_system_page(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(page, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 16, 0);
    lv_obj_set_style_pad_column(page, 8, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *col_left = make_sys_column(page);
    lv_obj_t *col_right = make_sys_column(page);

    /* --- Left column: device --- */
    make_sys_section_header(col_left, "DEVICE");
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    char device_buf[64];
    snprintf(device_buf, sizeof(device_buf), "ESP32-S3, %d cores, rev v%d.%d",
             chip_info.cores, chip_info.revision / 100, chip_info.revision % 100);
    s_lbl_sys_device = make_sys_info_row(col_left, device_buf);

    make_sys_section_header(col_left, "NETWORK");
    s_lbl_sys_wifi = make_sys_info_row(col_left, "Wi-Fi: disconnected");
    s_lbl_sys_mqtt = make_sys_info_row(col_left, "MQTT/CoreIoT: down");

    make_sys_section_header(col_left, "BROKER");
    char broker_buf[64];
    snprintf(broker_buf, sizeof(broker_buf), "Broker: %s", coreiot_broker_uri_display());
    s_lbl_sys_broker = make_sys_info_row(col_left, broker_buf);

    char token_masked[32];
    mask_secret(coreiot_token_display(), token_masked, sizeof(token_masked));
    char token_buf[48];
    snprintf(token_buf, sizeof(token_buf), "Access token: %s", token_masked);
    s_lbl_sys_token = make_sys_info_row(col_left, token_buf);

    /* --- Right column: firmware, storage --- */
    make_sys_section_header(col_right, "FIRMWARE");
    const esp_app_desc_t *app_desc = esp_app_get_description();
    char fw_buf[96];
    snprintf(fw_buf, sizeof(fw_buf), "App: %s v%s", app_desc->project_name, app_desc->version);
    s_lbl_sys_fw_version = make_sys_info_row(col_right, fw_buf);

    char idf_buf[48];
    snprintf(idf_buf, sizeof(idf_buf), "ESP-IDF: %s", esp_get_idf_version());
    s_lbl_sys_idf_version = make_sys_info_row(col_right, idf_buf);

    char build_buf[64];
    snprintf(build_buf, sizeof(build_buf), "Built: %s %s", app_desc->date, app_desc->time);
    s_lbl_sys_build = make_sys_info_row(col_right, build_buf);

    s_lbl_sys_uptime = make_sys_info_row(col_right, "Uptime: 00:00:00");

    make_sys_section_header(col_right, "STORAGE");
    uint32_t flash_size_bytes = 0;
    esp_flash_get_size(NULL, &flash_size_bytes);
    char flash_buf[48];
    snprintf(flash_buf, sizeof(flash_buf), "Flash: %u MB", (unsigned)(flash_size_bytes / (1024 * 1024)));
    s_lbl_sys_flash = make_sys_info_row(col_right, flash_buf);

    s_lbl_sys_heap = make_sys_info_row(col_right, "Free heap: -- KB");
    s_lbl_sys_min_heap = make_sys_info_row(col_right, "Min free heap: -- KB");

    refresh_system_info();

    return page;
}
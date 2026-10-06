/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_setup.c — Trang SETUP: chọn hồ sơ xe (T4.1b).
 * Mỗi hồ sơ trong registry là một nút; chọn nút → vehicle_settings_select() (lưu NVS) rồi dựng lại sơ đồ xe.
 * Chuỗi hiển thị chỉ dùng ASCII (font LVGL mặc định không có dấu tiếng Việt).
 */

#include "ui_dashboard_private.h"
#include "ui_dashboard.h"
#include "vehicle_settings.h"

#include <stdint.h>

#define SETUP_MAX_PROFILES 8

static lv_obj_t *s_lbl_active;
static lv_obj_t *s_lbl_save_warn;
static lv_obj_t *s_btn[SETUP_MAX_PROFILES];
static uint8_t s_btn_id[SETUP_MAX_PROFILES];
static size_t s_btn_count;

static void profile_btn_cb(lv_event_t *e)
{
    uint8_t id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if (vehicle_settings_select(id)) {
        ui_dashboard_rebuild_vehicle();
    }
    setup_page_refresh();
}

void setup_page_refresh(void)
{
    const vehicle_profile_t *active = vehicle_profile_active();

    if (s_lbl_active != NULL) {
        lv_label_set_text_fmt(s_lbl_active, "Active: %s", active->name);
    }
    if (s_lbl_save_warn != NULL) {
        if (vehicle_settings_last_save_ok()) {
            lv_obj_add_flag(s_lbl_save_warn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(s_lbl_save_warn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    sensor_editor_refresh();                     /* nạp lại bản nháp + dấu '*' theo hồ sơ hiện tại */

    for (size_t i = 0; i < s_btn_count; i++) {
        bool is_active = (s_btn_id[i] == active->id);
        lv_obj_set_style_bg_color(s_btn[i], lv_color_hex(is_active ? COLOR_ACCENT : COLOR_PANEL), 0);
        lv_obj_set_style_border_color(s_btn[i], lv_color_hex(is_active ? COLOR_ACCENT : COLOR_BORDER), 0);
        lv_obj_t *lbl = lv_obj_get_child(s_btn[i], 0);
        if (lbl != NULL) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(is_active ? COLOR_BG : COLOR_TEXT), 0);
        }
    }
}

lv_obj_t *build_setup_page(lv_obj_t *parent)
{
    /* Hai cột: trái = chọn hồ sơ (380 px), phải = chỉnh tay cảm biến (388 px). 12+380+8+388+12 = 800. */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 12, 0);
    lv_obj_set_style_pad_column(page, 8, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *left = lv_obj_create(page);
    lv_obj_set_size(left, 380, LV_PCT(100));
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_set_style_pad_row(left, 8, 0);
    lv_obj_remove_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *hdr = lv_label_create(left);
    lv_label_set_text(hdr, "VEHICLE PROFILE");
    lv_obj_set_style_text_color(hdr, lv_color_hex(COLOR_ACCENT), 0);

    s_lbl_active = lv_label_create(left);
    lv_label_set_text(s_lbl_active, "Active: --");
    lv_obj_set_style_text_color(s_lbl_active, lv_color_hex(COLOR_TEXT), 0);

    s_lbl_save_warn = lv_label_create(left);
    lv_label_set_text(s_lbl_save_warn, "NOT SAVED: NVS write failed,\nlost on reboot");
    lv_obj_set_style_text_color(s_lbl_save_warn, lv_color_hex(COLOR_DANGER), 0);
    lv_obj_add_flag(s_lbl_save_warn, LV_OBJ_FLAG_HIDDEN);

    s_btn_count = 0;
    size_t n = vehicle_profile_count();
    for (size_t i = 0; i < n && s_btn_count < SETUP_MAX_PROFILES; i++) {
        const vehicle_profile_t *p = vehicle_profile_get(i);
        if (p == NULL) {
            continue;
        }

        lv_obj_t *btn = lv_btn_create(left);
        lv_obj_set_size(btn, 380, 64);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_add_event_cb(btn, profile_btn_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)p->id);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text_fmt(lbl, "%s\n%u x %u mm, wheelbase %u mm", p->name,
                              (unsigned)p->length_mm, (unsigned)p->width_mm, (unsigned)p->wheelbase_mm);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);

        s_btn[s_btn_count] = btn;
        s_btn_id[s_btn_count] = p->id;
        s_btn_count++;
    }

    build_sensor_editor(page);
    setup_page_refresh();
    return page;
}

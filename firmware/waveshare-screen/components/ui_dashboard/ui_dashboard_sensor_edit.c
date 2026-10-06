/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_sensor_edit.c — Chỉnh tay vị trí/hướng từng cảm biến, ghi đè hồ sơ xe (T4.1c).
 *
 * Màn cảm ứng nên chỉ dùng nút −/+ (không bàn phím): chọn 1 trong 6 slot, tăng/giảm bản NHÁP (x, y, góc),
 * bấm Apply mới ghi đè (vehicle_settings_set_override: kiểm hợp lệ + lưu NVS) rồi dựng lại sơ đồ xe.
 * Reset bỏ ghi đè của slot đang chọn. Chuỗi hiển thị chỉ ASCII (font LVGL mặc định không có dấu tiếng Việt).
 */

#include "ui_dashboard_private.h"
#include "ui_dashboard.h"
#include "vehicle_settings.h"

#include <stdint.h>

#define STEP_MM   50
#define STEP_DEG  5
#define POS_LIMIT 30000                         /* chặn tràn int16; giới hạn thật do validate quyết định */

enum { FIELD_X = 0, FIELD_Y = 1, FIELD_ANGLE = 2, FIELD_COUNT = 3 };

static const char *const k_slot_names[ESPNOW_SENSOR_SLOT_COUNT] = {
    [ESPNOW_SLOT_FRONT]       = "FRONT",
    [ESPNOW_SLOT_REAR]        = "REAR",
    [ESPNOW_SLOT_LEFT_FRONT]  = "L-FRONT",
    [ESPNOW_SLOT_LEFT_REAR]   = "L-REAR",
    [ESPNOW_SLOT_RIGHT_FRONT] = "R-FRONT",
    [ESPNOW_SLOT_RIGHT_REAR]  = "R-REAR",
};

static lv_obj_t *s_slot_btn[ESPNOW_SENSOR_SLOT_COUNT];
static lv_obj_t *s_slot_lbl[ESPNOW_SENSOR_SLOT_COUNT];
static lv_obj_t *s_val_lbl[FIELD_COUNT];
static lv_obj_t *s_msg_lbl;
static int s_slot = ESPNOW_SLOT_FRONT;
static vehicle_sensor_pose_t s_draft;            /* bản nháp, chỉ áp dụng khi bấm Apply */

static void set_msg(const char *text, uint32_t color)
{
    if (s_msg_lbl != NULL) {
        lv_label_set_text(s_msg_lbl, text);
        lv_obj_set_style_text_color(s_msg_lbl, lv_color_hex(color), 0);
    }
}

static void load_draft(void)
{
    s_draft = vehicle_profile_active()->sensors[s_slot];
}

static void update_widgets(void)
{
    uint8_t mask = vehicle_settings_override_mask();
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        if (s_slot_btn[i] == NULL) {
            continue;
        }
        lv_label_set_text_fmt(s_slot_lbl[i], "%s%s", k_slot_names[i], (mask & (1u << i)) ? "*" : "");
        bool sel = (i == s_slot);
        lv_obj_set_style_bg_color(s_slot_btn[i], lv_color_hex(sel ? COLOR_ACCENT : COLOR_PANEL), 0);
        lv_obj_set_style_text_color(s_slot_lbl[i], lv_color_hex(sel ? COLOR_BG : COLOR_TEXT), 0);
    }
    if (s_val_lbl[FIELD_X] != NULL) {
        lv_label_set_text_fmt(s_val_lbl[FIELD_X], "%d", (int)s_draft.x_mm);
        lv_label_set_text_fmt(s_val_lbl[FIELD_Y], "%d", (int)s_draft.y_mm);
        lv_label_set_text_fmt(s_val_lbl[FIELD_ANGLE], "%d", (int)s_draft.angle_deg);
    }
}

void sensor_editor_refresh(void)
{
    load_draft();
    update_widgets();
    set_msg("", COLOR_TEXT);
}

static void slot_btn_cb(lv_event_t *e)
{
    s_slot = (int)(uintptr_t)lv_event_get_user_data(e);
    sensor_editor_refresh();
}

static int clamp_pos(int v)
{
    return v > POS_LIMIT ? POS_LIMIT : (v < -POS_LIMIT ? -POS_LIMIT : v);
}

/* user_data = (field << 8) | (dir == +1 ? 1 : 0) */
static void adjust_cb(lv_event_t *e)
{
    uintptr_t ud = (uintptr_t)lv_event_get_user_data(e);
    int field = (int)(ud >> 8);
    int dir = (ud & 1u) ? 1 : -1;

    if (field == FIELD_X) {
        s_draft.x_mm = (int16_t)clamp_pos(s_draft.x_mm + dir * STEP_MM);
    } else if (field == FIELD_Y) {
        s_draft.y_mm = (int16_t)clamp_pos(s_draft.y_mm + dir * STEP_MM);
    } else {
        s_draft.angle_deg = (int16_t)((s_draft.angle_deg + dir * STEP_DEG + 360) % 360);
    }
    update_widgets();
    set_msg("Not applied yet", COLOR_CAUTION);
}

static void apply_cb(lv_event_t *e)
{
    (void)e;
    if (!vehicle_settings_set_override((espnow_slot_t)s_slot, &s_draft)) {
        set_msg("Invalid position (outside the vehicle frame?)", COLOR_DANGER);
        return;                                  /* không đổi gì: bản nháp giữ nguyên để người dùng sửa tiếp */
    }
    ui_dashboard_rebuild_vehicle();
    load_draft();
    update_widgets();
    if (vehicle_settings_last_save_ok()) {
        set_msg("Applied and saved", COLOR_SAFE);
    } else {
        set_msg("Applied, NOT SAVED (NVS write failed)", COLOR_DANGER);
    }
}

static void reset_cb(lv_event_t *e)
{
    (void)e;
    if (vehicle_settings_clear_override((espnow_slot_t)s_slot)) {
        ui_dashboard_rebuild_vehicle();
    }
    load_draft();
    update_widgets();
    set_msg("Sensor reset to profile position", COLOR_TEXT);
}

static lv_obj_t *make_btn(lv_obj_t *parent, int w, int h, const char *text, lv_event_cb_t cb, uintptr_t ud,
                          lv_obj_t **out_lbl)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)ud);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_center(lbl);
    if (out_lbl != NULL) {
        *out_lbl = lbl;
    }
    return btn;
}

/* Hàng cao cố định để bố cục trang SETUP ổn định trên màn 800x440. (Lần sim treo khi viết widget này là do
 * HẾT pool LVGL 64 KB — LV_ASSERT_MALLOC lặp vô hạn — chứ không phải do LV_SIZE_CONTENT; pool đã nâng 128 KB.) */
static lv_obj_t *make_row(lv_obj_t *parent, int height, lv_flex_flow_t flow)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), height);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_column(row, 4, 0);
    lv_obj_set_style_pad_row(row, 4, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, flow);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return row;
}

lv_obj_t *build_sensor_editor(lv_obj_t *parent)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_set_size(col, 388, LV_PCT(100));
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(col, 0, 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_style_pad_row(col, 6, 0);
    lv_obj_remove_flag(col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *hdr = lv_label_create(col);
    lv_label_set_text(hdr, "SENSOR POSITION (* = overridden)");
    lv_obj_set_style_text_color(hdr, lv_color_hex(COLOR_ACCENT), 0);

    /* 6 nút chọn slot, 3 nút mỗi hàng */
    lv_obj_t *grid = make_row(col, 64, LV_FLEX_FLOW_ROW_WRAP);        /* 2 hàng nút 30 px + khe 4 px */
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        s_slot_btn[i] = make_btn(grid, 120, 30, k_slot_names[i], slot_btn_cb, (uintptr_t)i, &s_slot_lbl[i]);
    }

    static const char *const k_field_names[FIELD_COUNT] = { "X (+right)", "Y (+front)", "Angle (deg)" };
    for (int f = 0; f < FIELD_COUNT; f++) {
        lv_obj_t *row = make_row(col, 34, LV_FLEX_FLOW_ROW);
        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, k_field_names[f]);
        lv_obj_set_width(name, 108);
        lv_obj_set_style_text_color(name, lv_color_hex(COLOR_TEXT), 0);

        make_btn(row, 56, 32, "-", adjust_cb, ((uintptr_t)f << 8) | 0u, NULL);
        s_val_lbl[f] = lv_label_create(row);
        lv_label_set_text(s_val_lbl[f], "0");
        lv_obj_set_width(s_val_lbl[f], 96);
        lv_obj_set_style_text_align(s_val_lbl[f], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(s_val_lbl[f], lv_color_hex(COLOR_TEXT), 0);
        make_btn(row, 56, 32, "+", adjust_cb, ((uintptr_t)f << 8) | 1u, NULL);
    }

    lv_obj_t *hint = lv_label_create(col);
    lv_label_set_text(hint, "Angle: 0=right 90=rear 180=left 270=front");
    lv_obj_set_style_text_color(hint, lv_color_hex(COLOR_NODATA), 0);

    lv_obj_t *actions = make_row(col, 38, LV_FLEX_FLOW_ROW);
    make_btn(actions, 120, 36, "Apply", apply_cb, 0, NULL);
    make_btn(actions, 120, 36, "Reset", reset_cb, 0, NULL);

    s_msg_lbl = lv_label_create(col);
    lv_label_set_text(s_msg_lbl, "");
    lv_label_set_long_mode(s_msg_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_msg_lbl, LV_PCT(100));

    sensor_editor_refresh();
    return col;
}

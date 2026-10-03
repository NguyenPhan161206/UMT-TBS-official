/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_truck.c — Vẽ sơ đồ thân xe tải EX8 (cabin, thùng hàng, bánh) từ layout.
 */

#include "ui_dashboard_private.h"

static lv_obj_t *create_box(lv_obj_t *parent, int16_t x, int16_t y, int16_t w, int16_t h,
                            uint32_t bg_col, uint32_t border_col, int16_t radius)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(bg_col), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(border_col), 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

void build_truck_body(lv_obj_t *canvas, const vehicle_layout_t *L)
{
    if (canvas == NULL || L == NULL) {
        return;
    }

    /* 1. Bánh xe (vẽ trước để nằm lớp dưới thân xe) */
    int16_t wheel_w = L->body_w / 7;
    if (wheel_w < 6) wheel_w = 6;
    int16_t wheel_h = L->body_h / 10;
    if (wheel_h < 12) wheel_h = 12;

    int16_t front_axle_y = L->body_y + (L->cab_h * 7) / 10;
    int16_t rear_axle_y  = L->body_y + (L->body_h * 8) / 10;

    /* Bánh trước trái / phải */
    create_box(canvas, L->body_x - wheel_w / 2, front_axle_y - wheel_h / 2,
               wheel_w, wheel_h, COLOR_BG, COLOR_ACCENT, 3);
    create_box(canvas, L->body_x + L->body_w - wheel_w / 2, front_axle_y - wheel_h / 2,
               wheel_w, wheel_h, COLOR_BG, COLOR_ACCENT, 3);

    /* Bánh sau trái / phải (cụm trục sau) */
    create_box(canvas, L->body_x - wheel_w / 2, rear_axle_y - wheel_h / 2,
               wheel_w, wheel_h, COLOR_BG, COLOR_ACCENT, 3);
    create_box(canvas, L->body_x + L->body_w - wheel_w / 2, rear_axle_y - wheel_h / 2,
               wheel_w, wheel_h, COLOR_BG, COLOR_ACCENT, 3);

    /* 2. Thùng hàng (phía sau / bên dưới cabin) */
    int16_t cargo_h = L->body_h - L->cab_h;
    int16_t cargo_y = L->body_y + L->cab_h;
    lv_obj_t *cargo = create_box(canvas, L->body_x, cargo_y, L->body_w, cargo_h,
                                 COLOR_PANEL, COLOR_BORDER, 4);

    /* Vạch gân gia cường thùng hàng (tạo cảm giác thùng xe tải) */
    if (cargo_h > 30) {
        int16_t mid_line_y = cargo_h / 2;
        lv_obj_t *cargo_line = lv_obj_create(cargo);
        lv_obj_set_pos(cargo_line, 8, mid_line_y);
        lv_obj_set_size(cargo_line, L->body_w - 16, 1);
        lv_obj_set_style_border_width(cargo_line, 1, 0);
        lv_obj_set_style_border_color(cargo_line, lv_color_hex(COLOR_BORDER), 0);
        lv_obj_remove_flag(cargo_line, LV_OBJ_FLAG_SCROLLABLE);
    }

    /* Nhãn REAR ở đuôi thùng */
    lv_obj_t *lbl_rear = lv_label_create(cargo);
    lv_label_set_text(lbl_rear, "REAR");
    lv_obj_set_style_text_color(lbl_rear, lv_color_hex(COLOR_NODATA), 0);
    lv_obj_align(lbl_rear, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* 3. Cabin (phía trước / bên trên, thu hẹp ~10% bề ngang để tạo khe gương) */
    int16_t cab_w = (L->body_w * 90) / 100;
    int16_t cab_x = L->body_x + (L->body_w - cab_w) / 2;
    int16_t cab_y = L->body_y;
    lv_obj_t *cab = create_box(canvas, cab_x, cab_y, cab_w, L->cab_h,
                               COLOR_PANEL, COLOR_ACCENT, 8);

    /* Gương chiếu hậu 2 bên cabin */
    int16_t mirror_w = (L->body_w - cab_w) / 2 + 2;
    int16_t mirror_h = L->cab_h / 4;
    if (mirror_h < 4) mirror_h = 4;
    int16_t mirror_y = cab_y + L->cab_h / 3;
    create_box(canvas, L->body_x - 2, mirror_y, mirror_w, mirror_h,
               COLOR_PANEL, COLOR_ACCENT, 2);
    create_box(canvas, L->body_x + L->body_w - mirror_w + 2, mirror_y, mirror_w, mirror_h,
               COLOR_PANEL, COLOR_ACCENT, 2);

    /* Kính chắn gió phía trước cabin */
    if (L->cab_h > 20) {
        int16_t glass_w = (cab_w * 75) / 100;
        int16_t glass_h = L->cab_h / 3;
        lv_obj_t *glass = create_box(cab, (cab_w - glass_w) / 2, 6, glass_w, glass_h,
                                     COLOR_BG, COLOR_BORDER, 3);
        (void)glass;
    }

    /* Nhãn FRONT ở đầu xe */
    lv_obj_t *lbl_front = lv_label_create(cab);
    lv_label_set_text(lbl_front, "FRONT");
    lv_obj_set_style_text_color(lbl_front, lv_color_hex(COLOR_TEXT), 0);
    lv_obj_align(lbl_front, LV_ALIGN_CENTER, 0, 4);
}

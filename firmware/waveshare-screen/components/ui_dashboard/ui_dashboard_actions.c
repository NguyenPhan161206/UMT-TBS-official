/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * ui_dashboard_actions.c — Nút Mute: trạng thái hiển thị + callback báo ra ngoài.
 * Tách từ ui_dashboard.c (R7: <= 400 dòng/file) để có chỗ thêm trang SETUP; không đổi hành vi.
 */

#include "ui_dashboard_private.h"
#include "ui_dashboard.h"

/* Callback Mute do tầng main đăng ký: UI chỉ báo ý định, không biết đường truyền. */
static ui_dashboard_mute_cb_t s_mute_cb = NULL;

void ui_dashboard_set_mute_cb(ui_dashboard_mute_cb_t cb)
{
    s_mute_cb = cb;
}

/* Reflects s_alarm_muted on the button itself - otherwise "Mute Alarm" always
 * reads the same regardless of state and there is no way to tell from the
 * dashboard whether the alarm is currently silenced or live.
 */
void update_mute_button_visual(void)
{
    if (s_mute_btn_lbl) {
        lv_label_set_text(s_mute_btn_lbl, s_alarm_muted ? "Unmute Alarm" : "Mute Alarm");
    }
    if (s_mute_btn) {
        lv_obj_set_style_bg_color(s_mute_btn, lv_color_hex(s_alarm_muted ? COLOR_DANGER : COLOR_PANEL), 0);
    }
}

void mute_btn_cb(lv_event_t *e)
{
    (void)e;
    s_alarm_muted = !s_alarm_muted;
    update_mute_button_visual();

    // Báo ra ngoài: tầng main gửi lệnh MUTE về sensor-node (xem on_ui_mute_changed).
    if (s_mute_cb != NULL) {
        s_mute_cb(s_alarm_muted);
    }

    // Chỉ cập nhật banner/status, KHÔNG thay màu arc của sensor
    evaluate_hazard();
}

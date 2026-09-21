import re

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c", "r") as f:
    content = f.read()

target = """    s_tab_btn_system = lv_btn_create(header);
    lv_obj_set_size(s_tab_btn_system, 100, 28);
    lv_obj_align(s_tab_btn_system, LV_ALIGN_RIGHT_MID, 0, 0);"""

repl = """    s_tab_btn_system = lv_btn_create(header);
    lv_obj_set_size(s_tab_btn_system, 100, 28);
    lv_obj_align(s_tab_btn_system, LV_ALIGN_RIGHT_MID, -40, 0);

    lv_obj_t *settings_btn = lv_btn_create(header);
    lv_obj_set_size(settings_btn, 32, 28);
    lv_obj_align(settings_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_t *settings_lbl = lv_label_create(settings_btn);
    lv_label_set_text(settings_lbl, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_lbl);
    
    extern void ui_dashboard_create_settings(lv_obj_t *parent);
    static void open_settings_cb(lv_event_t *e) {
        ui_dashboard_create_settings(lv_scr_act());
    }
    lv_obj_add_event_cb(settings_btn, open_settings_cb, LV_EVENT_CLICKED, NULL);
"""

content = content.replace(target, repl)

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c", "w") as f:
    f.write(content)

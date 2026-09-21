import re

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c", "r") as f:
    content = f.read()

# Add #include "sys_settings_manager.h"
if 'sys_settings_manager.h' not in content:
    content = content.replace('#include "ui_dashboard_private.h"', '#include "ui_dashboard_private.h"\n#include "sys_settings_manager.h"')

target = """    lv_label_set_text_fmt(lbl, "> %dcm : Safe", SENSOR_CAUTION_CM);

    /* --- Caution --- */
    lbl = lv_label_create(box);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_WARNING), 0);
    lv_label_set_text_fmt(lbl, "%d-%dcm : Caution", SENSOR_DANGER_CM, SENSOR_CAUTION_CM);

    /* --- Danger --- */
    lbl = lv_label_create(box);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_DANGER), 0);
    lv_label_set_text_fmt(lbl, "< %dcm : Danger", SENSOR_DANGER_CM);"""

repl = """    sys_settings_t settings;
    sys_settings_get(&settings);

    lv_label_set_text_fmt(lbl, "> %dcm : Safe", settings.caution_cm);

    /* --- Caution --- */
    lbl = lv_label_create(box);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_WARNING), 0);
    lv_label_set_text_fmt(lbl, "%d-%dcm : Caution", settings.danger_cm, settings.caution_cm);

    /* --- Danger --- */
    lbl = lv_label_create(box);
    lv_obj_set_style_text_color(lbl, lv_color_hex(COLOR_DANGER), 0);
    lv_label_set_text_fmt(lbl, "< %dcm : Danger", settings.danger_cm);"""

content = content.replace(target, repl)

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c", "w") as f:
    f.write(content)

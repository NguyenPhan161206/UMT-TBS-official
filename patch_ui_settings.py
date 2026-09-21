import re

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard.c", "r") as f:
    content = f.read()

# Add #include "sys_settings_manager.h"
if 'sys_settings_manager.h' not in content:
    content = content.replace('#include "ui_dashboard_private.h"', '#include "ui_dashboard_private.h"\n#include "sys_settings_manager.h"')

# Replace hazard_classify(readings[i].distance_cm) with hazard_classify(..., settings.danger_cm, settings.caution_cm)
if 'hazard_classify(readings[i].distance_cm)' in content:
    content = content.replace('hazard_classify(readings[i].distance_cm)', 'hazard_classify(readings[i].distance_cm, settings.danger_cm, settings.caution_cm)')

# Replace hazard_worst_zone
if 'hazard_worst_zone(dist_cm, is_stale, health, SENSOR_MODEL_COUNT)' in content:
    content = content.replace('hazard_worst_zone(dist_cm, is_stale, health, SENSOR_MODEL_COUNT)', 'hazard_worst_zone(dist_cm, is_stale, health, SENSOR_MODEL_COUNT, settings.danger_cm, settings.caution_cm)')

# Add sys_settings_t settings; sys_settings_get(&settings); to evaluate_hazard
eval_target = """void evaluate_hazard(void)
{
    sensor_reading_t readings[SENSOR_MODEL_COUNT];
    sensor_model_get_all(readings);"""
    
eval_repl = """void evaluate_hazard(void)
{
    sys_settings_t settings;
    sys_settings_get(&settings);

    sensor_reading_t readings[SENSOR_MODEL_COUNT];
    sensor_model_get_all(readings);"""
    
if eval_target in content:
    content = content.replace(eval_target, eval_repl)

# Add sys_settings_t settings; sys_settings_get(&settings); to update_dashboard
update_target = """void update_dashboard(void)
{
    sensor_reading_t readings[SENSOR_MODEL_COUNT];
    sensor_model_get_all(readings);"""
    
update_repl = """void update_dashboard(void)
{
    sys_settings_t settings;
    sys_settings_get(&settings);

    sensor_reading_t readings[SENSOR_MODEL_COUNT];
    sensor_model_get_all(readings);"""
    
if update_target in content:
    content = content.replace(update_target, update_repl)

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard.c", "w") as f:
    f.write(content)

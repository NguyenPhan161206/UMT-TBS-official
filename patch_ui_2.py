import re

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard.c", "r") as f:
    content = f.read()

# Fix hazard_classify in ui_dashboard_update_sensor
update_target = """void ui_dashboard_update_sensor(espnow_slot_t slot, uint16_t dist_cm, sensor_health_t health)
{
    sensor_model_update(slot, dist_cm, health);
    sensor_zone_t zone = hazard_classify(dist_cm);"""

update_repl = """void ui_dashboard_update_sensor(espnow_slot_t slot, uint16_t dist_cm, sensor_health_t health)
{
    sys_settings_t settings;
    sys_settings_get(&settings);
    sensor_model_update(slot, dist_cm, health);
    sensor_zone_t zone = hazard_classify(dist_cm, settings.danger_cm, settings.caution_cm);"""
    
content = content.replace(update_target, update_repl)

with open("firmware/waveshare-screen/components/ui_dashboard/ui_dashboard.c", "w") as f:
    f.write(content)

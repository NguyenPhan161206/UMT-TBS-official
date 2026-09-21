import re

with open("firmware/sensor-node/src/buzzer.cpp", "r") as f:
    content = f.read()

if '<Preferences.h>' not in content:
    content = content.replace('#include "shared_state.h"', '#include "shared_state.h"\n#include <Preferences.h>\n#include "sys_settings.h"')

target = """static void buzzerTask(void *pvParameters)
{
    (void)pvParameters;

    while (true)"""

repl = """static void buzzerTask(void *pvParameters)
{
    (void)pvParameters;

    uint16_t danger_cm = DEFAULT_SENSOR_DANGER_CM;
    uint16_t caution_cm = DEFAULT_SENSOR_CAUTION_CM;
    
    Preferences pref;
    pref.begin("sys_settings", true);
    sys_settings_t set;
    size_t len = pref.getBytes("sys_settings", &set, sizeof(set));
    pref.end();
    
    if (len == sizeof(sys_settings_t)) {
        danger_cm = set.danger_cm;
        caution_cm = set.caution_cm;
    }

    while (true)"""

content = content.replace(target, repl)
content = content.replace('nearestCm <= (float)SENSOR_DANGER_CM', 'nearestCm <= (float)danger_cm')
content = content.replace('nearestCm <= (float)SENSOR_CAUTION_CM', 'nearestCm <= (float)caution_cm')

with open("firmware/sensor-node/src/buzzer.cpp", "w") as f:
    f.write(content)

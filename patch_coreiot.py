import re

with open("firmware/sensor-node/src/plugins/coreiot/coreiot_client.cpp", "r") as f:
    content = f.read()

# Add Preferences.h
if '<Preferences.h>' not in content:
    content = content.replace('#include <PubSubClient.h>', '#include <PubSubClient.h>\n#include <Preferences.h>\n#include "sys_settings.h"')

target = """void CoreiotClient::begin()
{
    s_mqttClient.setServer(COREIOT_BROKER, COREIOT_PORT);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[NET] Connecting to WiFi SSID: %s...\\n", WIFI_SSID);
}"""

repl = """void CoreiotClient::begin()
{
    s_mqttClient.setServer(COREIOT_BROKER, COREIOT_PORT);
    WiFi.mode(WIFI_STA);

    Preferences pref;
    pref.begin("sys_wifi", true);
    sys_wifi_config_t wifi_cfg;
    size_t len = pref.getBytes("sys_wifi", &wifi_cfg, sizeof(wifi_cfg));
    pref.end();

    if (len == sizeof(sys_wifi_config_t) && strlen(wifi_cfg.ssid) > 0) {
        WiFi.begin(wifi_cfg.ssid, wifi_cfg.password);
        Serial.printf("[NET] Connecting to UI-Configured WiFi SSID: %s...\\n", wifi_cfg.ssid);
    } else {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.printf("[NET] Connecting to Hardcoded WiFi SSID: %s...\\n", WIFI_SSID);
    }
}"""

content = content.replace(target, repl)

with open("firmware/sensor-node/src/plugins/coreiot/coreiot_client.cpp", "w") as f:
    f.write(content)

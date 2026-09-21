import re

with open("firmware/sensor-node/src/espnow_client.cpp", "r") as f:
    content = f.read()

# Add Preferences.h
if '<Preferences.h>' not in content:
    content = content.replace('#include "espnow_client.h"', '#include "espnow_client.h"\n#include <Preferences.h>\n#include <WiFi.h>')

target = """static void onDataRecv(const uint8_t *macAddr, const uint8_t *data, int len)
{
    Serial.printf("[ESPNOW] RECV pkt len: %d\\n", len);
    if (len == sizeof(espnow_cmd_msg_t))
    {
        espnow_cmd_msg_t cmd;
        memcpy(&cmd, data, sizeof(espnow_cmd_msg_t));
        Serial.printf("[ESPNOW] RECV CMD type: %d, payload: %d\\n", cmd.cmd_type, cmd.payload);
        if (cmd.cmd_type == ESPNOW_CMD_MUTE_BUZZER)
        {
            sharedStateSetMute(cmd.payload != 0);
        }
    }
}"""

repl = """static void onDataRecv(const uint8_t *macAddr, const uint8_t *data, int len)
{
    Serial.printf("[ESPNOW] RECV pkt len: %d\\n", len);
    if (len == sizeof(espnow_cmd_msg_t))
    {
        espnow_cmd_msg_t cmd;
        memcpy(&cmd, data, sizeof(espnow_cmd_msg_t));
        Serial.printf("[ESPNOW] RECV CMD type: %d, payload: %d\\n", cmd.cmd_type, cmd.payload);
        if (cmd.cmd_type == ESPNOW_CMD_MUTE_BUZZER)
        {
            sharedStateSetMute(cmd.payload != 0);
        }
    }
    else if (len == sizeof(espnow_sync_settings_msg_t))
    {
        espnow_sync_settings_msg_t sync;
        memcpy(&sync, data, sizeof(sync));
        if (sync.cmd_type == ESPNOW_CMD_TYPE_SYNC_SETTINGS)
        {
            Serial.printf("[ESPNOW] RECV Settings: danger=%d, caution=%d\\n", sync.settings.danger_cm, sync.settings.caution_cm);
            Preferences pref;
            pref.begin("sys_settings", false);
            pref.putBytes("sys_settings", &sync.settings, sizeof(sys_settings_t));
            pref.end();
            // TODO: Apply live if needed, or reboot
            ESP.restart();
        }
    }
    else if (len == sizeof(espnow_sync_wifi_msg_t))
    {
        espnow_sync_wifi_msg_t sync;
        memcpy(&sync, data, sizeof(sync));
        if (sync.cmd_type == ESPNOW_CMD_TYPE_SYNC_WIFI)
        {
            Serial.printf("[ESPNOW] RECV WiFi config: SSID=%s\\n", sync.wifi.ssid);
            Preferences pref;
            pref.begin("sys_wifi", false);
            pref.putBytes("sys_wifi", &sync.wifi, sizeof(sys_wifi_config_t));
            pref.end();
            ESP.restart();
        }
    }
}"""

content = content.replace(target, repl)

with open("firmware/sensor-node/src/espnow_client.cpp", "w") as f:
    f.write(content)

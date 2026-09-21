#include "espnow_client.h"
#include "sys_settings.h"
#include <Preferences.h>
#include <WiFi.h>
#include "shared_state.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <string.h>


static void onDataRecv(const uint8_t *macAddr, const uint8_t *data, int len)
{
    Serial.printf("[ESPNOW] RECV pkt len: %d\n", len);
    if (len == sizeof(espnow_cmd_msg_t))
    {
        espnow_cmd_msg_t cmd;
        memcpy(&cmd, data, sizeof(espnow_cmd_msg_t));
        Serial.printf("[ESPNOW] RECV CMD type: %d, payload: %d\n", cmd.cmd_type, cmd.payload);
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
            Serial.printf("[ESPNOW] RECV Settings: danger=%d, caution=%d\n", sync.settings.danger_cm, sync.settings.caution_cm);
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
            Serial.printf("[ESPNOW] RECV WiFi config: SSID=%s\n", sync.wifi.ssid);
            Preferences pref;
            pref.begin("sys_wifi", false);
            pref.putBytes("sys_wifi", &sync.wifi, sizeof(sys_wifi_config_t));
            pref.end();
            ESP.restart();
        }
    }
}
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    (void)mac_addr;
    if (status != ESP_NOW_SEND_SUCCESS)
    {
        Serial.println("[ESPNOW] Send FAILED");
    }
}

/* Kênh WiFi hiện tại (theo AP khi STA đã nối). Trước khi associate dùng
 * ESPNOW_CHANNEL làm fallback (begin() ghim radio ở đó). */
static uint8_t currentHomeChannel()
{
    uint8_t primary = 0;
    wifi_second_chan_t secondary = WIFI_SECOND_CHAN_NONE;
    if (esp_wifi_get_channel(&primary, &secondary) == ESP_OK && primary != 0)
    {
        return primary;
    }
    return ESPNOW_CHANNEL;
}

/* Đồng bộ peer (broadcast) theo home channel. AP tự đổi kênh qua CSA (log
 * "sta rx csa") — ESP-NOW single-radio phải bám home channel, không cố định
 * ESPNOW_CHANNEL, nếu không esp_now_send fail "Peer channel is not equal to
 * the home channel". Gọi trước mỗi send (500ms) = tự lành sau đổi kênh. */
static void syncPeerChannelToHome()
{
    esp_now_peer_info_t peer = {};
    if (esp_now_get_peer(ESPNOW_PEER_MAC, &peer) != ESP_OK)
    {
        return;
    }
    const uint8_t home = currentHomeChannel();
    if (peer.channel == home)
    {
        return;
    }
    peer.channel = home;
    esp_now_mod_peer(&peer);
    Serial.printf("[ESPNOW] Peer channel sync %u (WIFI home)\n", home);
}

void EspNowClient::begin()
{
#if USE_COREIOT
    // Không ngắt WiFi vì đang dùng chung với CoreIoT
    WiFi.mode(WIFI_STA);
#else
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
#endif
    esp_wifi_set_ps(WIFI_PS_NONE);

    // Tắt modem-sleep: khi env _coreiot associate vào AP, PS mặc định làm trễ
    // TX ESP-NOW theo cửa sổ ngủ/thức → rơi gói. Giữ radio thức mọi lúc.
    esp_wifi_set_ps(WIFI_PS_NONE);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("[ESPNOW] Init failed");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    // Broadcast (ESPNOW_PEER_MAC = FF:FF:FF:FF:FF:FF) không bắt buộc add_peer,
    // esp_now_send() tới broadcast làm việc trực tiếp. Cố gắng add_peer tương
    // thích (một vài IDF chấp nhận); nếu fail thì chỉ cảnh báo, không dừng.
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, ESPNOW_PEER_MAC, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("[ESPNOW] Add peer failed (bỏ qua: broadcast vẫn gửi được)");
    }
}

bool EspNowClient::sendReading(const espnow_sensor_msg_t &msg)
{
    syncPeerChannelToHome();
    esp_err_t result = esp_now_send(ESPNOW_PEER_MAC, (const uint8_t *)&msg, sizeof(msg));
    return result == ESP_OK;
}
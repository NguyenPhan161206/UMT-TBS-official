#include "espnow_client.h"
#include "shared_state.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <string.h>

#if TBS_LATENCY_PROBE
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

/* Đo độ trễ (DMXT-57): mốc esp_timer lúc gửi theo seq (vòng 64 > số gói có thể đang bay),
 * callback nhận echo tính RTT rồi đẩy vào queue để networkTask in ra (không in Serial trong
 * WiFi task). */
static const size_t LAT_SLOTS = 64;
static int64_t s_sendUs[LAT_SLOTS];
static uint16_t s_sendSeq[LAT_SLOTS];
static QueueHandle_t s_rttQueue = nullptr;

static void handleEcho(const uint8_t *data)
{
    const int64_t nowUs = esp_timer_get_time();
    espnow_echo_msg_t echo;
    memcpy(&echo, data, sizeof(echo));
    if (echo.type != ESPNOW_ECHO_LATENCY)
    {
        return;
    }
    const size_t slot = echo.seq % LAT_SLOTS;
    if (s_sendSeq[slot] != echo.seq || s_sendUs[slot] == 0)
    {
        return; // echo quá muộn (slot đã bị gói mới ghi đè) -> bỏ, script tính là mất
    }
    EspNowRtt r = {echo.seq, (uint32_t)(nowUs - s_sendUs[slot])};
    s_sendUs[slot] = 0; // chống đếm trùng nếu echo lặp
    xQueueSend(s_rttQueue, &r, 0);
}
#endif


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
}
/* Đếm cho dòng SOAK (DMXT-58). Gửi broadcast không có ACK nên "ok" = MAC đã phát gói đi. */
static volatile uint32_t s_txOk = 0;
static volatile uint32_t s_txFail = 0;

static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    (void)mac_addr;
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        s_txOk = s_txOk + 1;
    }
    else
    {
        s_txFail = s_txFail + 1;
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

#if TBS_LATENCY_PROBE
    s_rttQueue = xQueueCreate(32, sizeof(EspNowRtt));
    Serial.println("[ESPNOW] LATENCY PROBE build: do RTT qua echo (khong dung cho ban phat hanh)");
#endif

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
#if TBS_LATENCY_PROBE
    const size_t slot = msg.seq % LAT_SLOTS;
    s_sendSeq[slot] = msg.seq;
    s_sendUs[slot] = esp_timer_get_time();
#endif
    esp_err_t result = esp_now_send(ESPNOW_PEER_MAC, (const uint8_t *)&msg, sizeof(msg));
    if (result != ESP_OK)
    {
        s_txFail = s_txFail + 1; // không vào hàng đợi -> onDataSent sẽ không được gọi
    }
    return result == ESP_OK;
}

uint32_t EspNowClient::txOk() const
{
    return s_txOk;
}

uint32_t EspNowClient::txFail() const
{
    return s_txFail;
}

bool EspNowClient::pollRtt(EspNowRtt &out)
{
#if TBS_LATENCY_PROBE
    return s_rttQueue != nullptr && xQueueReceive(s_rttQueue, &out, 0) == pdTRUE;
#else
    (void)out;
    return false;
#endif
}

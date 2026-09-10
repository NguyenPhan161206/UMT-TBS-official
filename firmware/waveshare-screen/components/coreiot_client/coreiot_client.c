/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * coreiot_client.c — non-blocking WiFi STA + MQTT (esp-mqtt) cho waveshare-screen.
 * Credential đọc từ `credentials.h` (AUTO-GENERATED, gitignored — R1).
 */

#include "coreiot_client.h"
#include "credentials.h"
#include "espnow_receiver.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "coreiot_client";
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_wifi_connected = false;
static int64_t s_last_mqtt_rx_time_ms = 0;
static char s_broker_uri[128] = {0};
static char s_token_display[128] = {0};

static coreiot_wifi_status_cb_t s_wifi_cb = NULL;
static coreiot_mqtt_status_cb_t s_mqtt_cb = NULL;
static coreiot_data_cb_t s_data_cb = NULL;

/* Debounce MQTT "DOWN" để tránh nhấp nháy UP/DOWN trên màn khi esp-mqtt
 * auto-reconnect nhanh (broker/network drop TCP định kỳ ~10s):
 * - MQTT_EVENT_CONNECTED  -> báo UP ngay, huỷ timer debounce.
 * - MQTT_EVENT_DISCONNECTED -> chỉ báo DOWN nếu không reconnect được trong
 *   MQTT_DOWN_DEBOUNCE_MS. Nếu reconnect xong trước đó -> giữ UP. */
#define MQTT_DOWN_DEBOUNCE_MS (6000)
/* Reconnect WiFi có backoff luỹ thừa: base 3s, mỗi lần DISCONNECTED chưa thành
 * công thì tăng gấp đôi, cap 30s, reset về base khi GOT_IP. Giảm số lần radio
 * rời kênh để nhận ESP-NOW (iPhone hotspot hay đá client mỗi ~30s — nếu retry
 * cố định 3s thì radio chạy scan/auth liên tục, gây flapping ESP-NOW). */
#define WIFI_RECONNECT_BASE_MS (3000)
#define WIFI_RECONNECT_MAX_MS (30000)
static uint32_t s_reconnect_delay_ms = WIFI_RECONNECT_BASE_MS;
static uint8_t s_last_ap_channel = 0;
static esp_timer_handle_t s_mqtt_down_timer = NULL;
static bool s_mqtt_reported_up = false;

static void mqtt_debounce_timer_cb(void *arg)
{
    (void)arg;
    /* Hết hạn debounce mà vẫn chưa reconnect -> báo DOWN một lần. */
    if (s_mqtt_reported_up && s_mqtt_cb) {
        s_mqtt_reported_up = false;
        s_mqtt_cb(false);
    }
}

static void mqtt_debounce_arm(void)
{
    if (s_mqtt_down_timer == NULL) {
        esp_timer_create_args_t args = {
            .callback = mqtt_debounce_timer_cb,
            .name = "mqtt_down_debounce",
        };
        if (esp_timer_create(&args, &s_mqtt_down_timer) != ESP_OK) {
            s_mqtt_down_timer = NULL;
        }
    }
    if (s_mqtt_down_timer != NULL) {
        esp_timer_stop(s_mqtt_down_timer);
        esp_timer_start_once(s_mqtt_down_timer, MQTT_DOWN_DEBOUNCE_MS * 1000);
    }
}

static void mqtt_debounce_cancel(void)
{
    if (s_mqtt_down_timer != NULL) {
        esp_timer_stop(s_mqtt_down_timer);
    }
}

static esp_timer_handle_t s_wifi_reconnect_timer = NULL;

static void wifi_reconnect_timer_cb(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Wi-Fi reconnect retry after %u ms...", (unsigned)s_reconnect_delay_ms);
    esp_wifi_connect();
}

static void wifi_reconnect_arm(void)
{
    if (s_wifi_reconnect_timer == NULL) {
        esp_timer_create_args_t args = {
            .callback = wifi_reconnect_timer_cb,
            .name = "wifi_reconnect",
        };
        if (esp_timer_create(&args, &s_wifi_reconnect_timer) != ESP_OK) {
            s_wifi_reconnect_timer = NULL;
        }
    }
    if (s_wifi_reconnect_timer != NULL) {
        esp_timer_stop(s_wifi_reconnect_timer);
        esp_timer_start_once(s_wifi_reconnect_timer,
                             (uint64_t)s_reconnect_delay_ms * 1000);
    }
}

void coreiot_client_set_callbacks(coreiot_wifi_status_cb_t wifi_cb,
                                   coreiot_mqtt_status_cb_t mqtt_cb,
                                   coreiot_data_cb_t data_cb)
{
    s_wifi_cb = wifi_cb;
    s_mqtt_cb = mqtt_cb;
    s_data_cb = data_cb;
}

const char *coreiot_broker_uri_display(void)
{
    return s_broker_uri;
}

const char *coreiot_token_display(void)
{
    return s_token_display;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected to CoreIoT (%s)", s_broker_uri);
        mqtt_debounce_cancel();
        if (!s_mqtt_reported_up && s_mqtt_cb) {
            s_mqtt_reported_up = true;
            s_mqtt_cb(true);
        }

        esp_mqtt_client_subscribe(client, COREIOT_TELEMETRY_TOPIC, 1);
        esp_mqtt_client_subscribe(client, "v1/devices/me/attributes", 1);
        esp_mqtt_client_subscribe(client, "v1/devices/me/attributes/response/+", 1);

        char reboot_msg[192];
        snprintf(reboot_msg, sizeof(reboot_msg),
                 "{\"reboot_event\":1,\"status\":\"ONLINE\",\"reboot_reason\":\"POWER_ON_RESET\",\"wifi_ssid\":\"%s\"}",
                 WIFI_SSID);
        int msg_id = esp_mqtt_client_publish(client, COREIOT_TELEMETRY_TOPIC, reboot_msg, 0, 1, 0);
        ESP_LOGI(TAG, "Published reboot telemetry message (msg_id=%d)", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT Disconnected");
        /* Debounce: chưa báo DOWN ngay — chờ reconnect trong MQTT_DOWN_DEBOUNCE_MS. */
        mqtt_debounce_arm();
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT DATA received from topic %.*s: %.*s",
                 event->topic_len, event->topic, event->data_len, event->data);
        s_last_mqtt_rx_time_ms = esp_timer_get_time() / 1000;
        if (s_data_cb) {
            s_data_cb(event->topic, event->topic_len, event->data, event->data_len);
        }
        break;

    default:
        break;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi STA started, connecting to SSID: %s...", WIFI_SSID);
        wifi_reconnect_arm();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *dis_event = (wifi_event_sta_disconnected_t *)event_data;
        s_wifi_connected = false;
        ESP_LOGW(TAG, "Wi-Fi disconnected! Reason code: %d, retrying connection...", dis_event ? dis_event->reason : -1);
        if (s_wifi_cb) {
            s_wifi_cb(false, NULL);
        }
        /* Giữ ESP-NOW listener trên kênh AP cuối (nơi sensor-node đang phát);
         * chỉ xoay về ESPNOW_CHANNEL nếu chưa từng nối AP (single radio). */
        if (s_last_ap_channel != 0) {
            esp_wifi_set_channel(s_last_ap_channel, WIFI_SECOND_CHAN_NONE);
        } else {
            espnow_receiver_force_channel();
        }
        /* Reconnect có backoff luỹ thừa — tránh loop dồn dập làm flapping ESP-NOW. */
        wifi_reconnect_arm();
        if (s_reconnect_delay_ms < WIFI_RECONNECT_MAX_MS) {
            s_reconnect_delay_ms *= 2;
            if (s_reconnect_delay_ms > WIFI_RECONNECT_MAX_MS) {
                s_reconnect_delay_ms = WIFI_RECONNECT_MAX_MS;
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ip_str[32];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Wi-Fi Connected Successfully! IP Address: %s", ip_str);

        uint8_t primary_ch = 0;
        wifi_second_chan_t second_ch = WIFI_SECOND_CHAN_NONE;
        if (esp_wifi_get_channel(&primary_ch, &second_ch) == ESP_OK) {
            s_last_ap_channel = primary_ch;
            ESP_LOGI(TAG, "Wi-Fi channel primary=%u secondary=%d", (unsigned)primary_ch, (int)second_ch);
        } else {
            ESP_LOGW(TAG, "esp_wifi_get_channel failed");
        }

        s_wifi_connected = true;
        /* Kết nối thành công -> reset backoff cho lần mất kết nối sau. */
        s_reconnect_delay_ms = WIFI_RECONNECT_BASE_MS;
        if (s_wifi_cb) {
            s_wifi_cb(true, ip_str);
        }

        if (s_mqtt_client != NULL) {
            esp_mqtt_client_start(s_mqtt_client);
        }
    }
}

void coreiot_client_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    snprintf(s_broker_uri, sizeof(s_broker_uri), "mqtt://%s:%d", COREIOT_BROKER, COREIOT_PORT);
    snprintf(s_token_display, sizeof(s_token_display), "%s", WAVESHARE_SCREEN_DEVICE_TOKEN);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Tắt modem-sleep: ESP-NOW là đường chính, receiver phải thức liên tục
     * để nhận broadcast từ sensor-node (PS mặc định rơi gói giữa beacon ~102ms). */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = s_broker_uri,
        .credentials.username = WAVESHARE_SCREEN_DEVICE_TOKEN,
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

int coreiot_client_publish_telemetry(const char *json_payload)
{
    if (s_mqtt_client == NULL || !s_wifi_connected) {
        return -1;
    }
    return esp_mqtt_client_publish(s_mqtt_client, COREIOT_TELEMETRY_TOPIC, json_payload, 0, 1, 0);
}

bool coreiot_client_has_recent_data(uint32_t max_age_ms)
{
    if (s_last_mqtt_rx_time_ms == 0) {
        return false;
    }
    int64_t now_ms = esp_timer_get_time() / 1000;
    return (now_ms - s_last_mqtt_rx_time_ms) <= (int64_t)max_age_ms;
}
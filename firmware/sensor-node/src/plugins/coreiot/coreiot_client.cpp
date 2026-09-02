#include "coreiot_client.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Credential từ credentials.h (sinh bởi gen_credentials.py, gitignored):
//   WIFI_SSID, WIFI_PASSWORD, COREIOT_BROKER, COREIOT_PORT,
//   SENSOR_NODE_DEVICE_TOKEN, COREIOT_TELEMETRY_TOPIC
static WiFiClient s_wifiClient;
static PubSubClient s_mqttClient(s_wifiClient);

void CoreiotClient::begin()
{
    s_mqttClient.setServer(COREIOT_BROKER, COREIOT_PORT);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[NET] Connecting to WiFi SSID: %s...\n", WIFI_SSID);
}

void CoreiotClient::ensureWifiConnected()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }
    // WiFi.begin() ở chế độ STA tự thử kết nối lại nền; không gọi lại
    // WiFi.begin() liên tục để tránh reset trạng thái kết nối đang thử.
}

void CoreiotClient::ensureMqttConnected()
{
    if (s_mqttClient.connected())
    {
        return;
    }

    uint32_t now = millis();
    if (now - _lastMqttRetryMs < COREIOT_MQTT_RETRY_INTERVAL_MS)
    {
        return;
    }
    _lastMqttRetryMs = now;

    Serial.println("[NET] Connecting to CoreIoT MQTT broker...");
    // CoreIoT/ThingsBoard: access token = MQTT username, không cần password.
    if (s_mqttClient.connect("sensor-node", SENSOR_NODE_DEVICE_TOKEN, nullptr))
    {
        Serial.println("[NET] MQTT connected");
    }
    else
    {
        Serial.printf("[NET] MQTT connect failed, state=%d\n", s_mqttClient.state());
    }
}

void CoreiotClient::loop()
{
    ensureWifiConnected();

    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    ensureMqttConnected();
    s_mqttClient.loop();
}

bool CoreiotClient::isConnected() const
{
    return s_mqttClient.connected();
}

bool CoreiotClient::publishTelemetry(const char *jsonPayload)
{
    if (!s_mqttClient.connected())
    {
        return false;
    }
    return s_mqttClient.publish(COREIOT_TELEMETRY_TOPIC, jsonPayload);
}
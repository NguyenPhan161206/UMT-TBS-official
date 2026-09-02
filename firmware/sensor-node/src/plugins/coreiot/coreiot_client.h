#pragma once
#include <stdbool.h>
#include <stdint.h>

// =========================================================
// CẤU HÌNH KẾT NỐI COREIOT (ThingsBoard MQTT) — V2
//
// KHÔNG hardcode credential nào ở đây (R1). Toàn bộ secret đến từ
// credentials.h — file SINH bởi tools/guard/gen_credentials.py từ
// config/keys.json (gitignored). File này KHÔNG được commit.
// =========================================================
#include "credentials.h"

// Tần suất publish telemetry lên CoreIoT (tách biệt MEASURE_INTERVAL_MS).
#define COREIOT_PUBLISH_INTERVAL_MS 500

// Khoảng cách tối thiểu giữa các lần thử kết nối lại MQTT khi mất kết nối.
#define COREIOT_MQTT_RETRY_INTERVAL_MS 3000

// Client MQTT mỏng để gửi telemetry lên CoreIoT (ThingsBoard).
// Không blocking: kết nối lại WiFi/MQTT được thử theo chu kỳ trong
// coreiotClientLoop(), không dùng delay() để không chặn task khác.
class CoreiotClient {
public:
    // Khởi tạo WiFi STA + cấu hình MQTT client. Gọi 1 lần trong task.
    void begin();

    // Gọi liên tục trong vòng lặp của task: duy trì WiFi/MQTT,
    // xử lý PubSubClient::loop().
    void loop();

    bool isConnected() const;

    // Gửi payload JSON lên COREIOT_TELEMETRY_TOPIC. Trả về false nếu
    // chưa kết nối MQTT hoặc publish thất bại.
    bool publishTelemetry(const char *jsonPayload);

private:
    uint32_t _lastMqttRetryMs = 0;

    void ensureWifiConnected();
    void ensureMqttConnected();
};
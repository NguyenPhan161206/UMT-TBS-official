// =========================================================
// Supersonic sensor array — V2 (Arduino framework + FreeRTOS)
// Đọc N cảm biến JSN-SR04T độc lập (mảng cặp chân Trig/Echo trong
// firmware/shared/thresholds.h::SENSOR_PINS), mỗi cảm biến có
// UltrasonicSensor + DistanceFilter riêng, không dùng std::vector
// để giảm cấp phát động và tăng tốc xử lý trên vi điều khiển.
//
// Tasks:
//   SensorTask  (core 1)  đọc/lọc lần lượt từng cảm biến -> SharedState
//   BuzzerTask  (core 0)  kêu còi theo khoảng cách gần nhất (non-blocking)
//   NetworkTask (core 0)  gửi ESP-NOW tới waveshare-screen (đường chính)
//   [CoreIoTTask] (core 0) publish telemetry MQTT — chỉ khi USE_COREIOT=1
//
// KHÔNG còn appTask demo (R5): demo/auto-play phải nằm ở prototypes/.
// =========================================================

#include <Arduino.h>
#include "buzzer.h"
#include "distance_filter.h"
#include "espnow_client.h"
#include "espnow_protocol.h"
#include "shared_state.h"
#include "thresholds.h"
#include "ultrasonic_sensor.h"

#if USE_COREIOT
#include "coreiot_client.h"
#endif

static TaskHandle_t s_sensorTaskHandle = nullptr;
static TaskHandle_t s_networkTaskHandle = nullptr;
static TaskHandle_t s_buzzerTaskHandle = nullptr;

static EspNowClient s_espNowClient;

#if USE_COREIOT
static CoreiotClient s_coreiotClient;
static TaskHandle_t s_coreiotTaskHandle = nullptr;
#endif

// Mảng tĩnh, kích thước cố định = SENSOR_COUNT (thresholds.h) - không dùng
// std::vector nên không có cấp phát heap/mảnh vụn bộ nhớ khi chạy.
static UltrasonicSensor s_sensors[SENSOR_COUNT];
static DistanceFilter s_filters[SENSOR_COUNT];
static int s_invalidCount[SENSOR_COUNT] = {0};

// =========================================================
// ÁNH XẠ VẬT LÝ -> SLOT ESP-NOW (wire)
// Thứ tự SENSOR_PINS[] KHÔNG trùng espnow_slot_t (xem thresholds.h).
// Mảng này là nguồn duy nhất cho việc gửi đúng slot; xoá = sai nhãn.
// =========================================================

static const uint8_t SENSOR_ESPNOW_SLOT[SENSOR_COUNT] = {
    ESPNOW_SLOT_FRONT,       // SENSOR_PINS[0] (5,6)   Front
    ESPNOW_SLOT_LEFT_FRONT,  // SENSOR_PINS[1] (7,8)   Left-Front
    ESPNOW_SLOT_RIGHT_FRONT, // SENSOR_PINS[2] (9,10)  Right-Front
    ESPNOW_SLOT_LEFT_REAR,   // SENSOR_PINS[3] (17,18) Left-Rear
    ESPNOW_SLOT_RIGHT_REAR,  // SENSOR_PINS[4] (21,38) Right-Rear
    ESPNOW_SLOT_REAR,        // SENSOR_PINS[5] (3,4)   Rear
};

// =========================================================
// CẢNH BÁO GPIO ĐÃ BỊ CHIẾM DỤNG NỘI BỘ CHIP (runtime boot check)
// =========================================================

struct ReservedPin
{
    uint8_t pin;
    const char *reason;
};

static const ReservedPin RESERVED_PINS[] = {
    {47, "Octal PSRAM SPICLK_P_DIFF (chip Embedded PSRAM 8MB) - khong dung duoc lam GPIO"},
    {48, "Octal PSRAM SPICLK_N_DIFF (chip Embedded PSRAM 8MB) - khong dung duoc lam GPIO"},
    {26, "SPI0 flash/PSRAM CS"},
    {27, "SPI0 flash/PSRAM"},
    {28, "SPI0 flash/PSRAM"},
    {29, "SPI0 flash/PSRAM"},
    {30, "SPI0 flash/PSRAM"},
    {31, "SPI0 flash/PSRAM"},
    {32, "SPI0 flash/PSRAM"},
    {19, "USB D- (native USB CDC dang dung de Serial)"},
    {20, "USB D+ (native USB CDC dang dung de Serial)"},
};

static const char *reservedPinReason(uint8_t pin)
{
    for (size_t i = 0; i < sizeof(RESERVED_PINS) / sizeof(RESERVED_PINS[0]); ++i)
    {
        if (RESERVED_PINS[i].pin == pin)
        {
            return RESERVED_PINS[i].reason;
        }
    }
    return nullptr;
}

static void warnIfReservedPin(size_t sensorIndex, uint8_t pin, const char *role)
{
    const char *reason = reservedPinReason(pin);
    if (reason != nullptr)
    {
        Serial.printf(
            "  [S%u] CANH BAO: %s=GPIO%u da bi chiem dung noi bo (%s) - doi sang chan khac!\n",
            (unsigned)sensorIndex, role, pin, reason);
    }
}

// =========================================================
// HÀM HỖ TRỢ
// =========================================================

static String distanceToText(bool valid, float cm)
{
    if (!valid)
    {
        return "--";
    }
    return String(cm, 2) + " cm";
}

// =========================================================
// SENSOR TASK (core 1)
// =========================================================

static void sensorTask(void *pvParameters)
{
    (void)pvParameters;
    for (size_t i = 0; i < SENSOR_COUNT; ++i)
    {
        s_sensors[i].begin(SENSOR_PINS[i].trigPin, SENSOR_PINS[i].echoPin);
        s_filters[i].reset();
    }

    // Chờ cảm biến ổn định sau khi cấp nguồn (không block task khác)
    vTaskDelay(pdMS_TO_TICKS(500));

    Serial.println("========================================");
    Serial.printf("Supersonic sensor array started (%u cam bien)\n", (unsigned)SENSOR_COUNT);
    for (size_t i = 0; i < SENSOR_COUNT; ++i)
    {
        Serial.printf("  [S%u] Trig=GPIO%u Echo=GPIO%u -> ESP-NOW slot %u\n",
                      (unsigned)i, SENSOR_PINS[i].trigPin, SENSOR_PINS[i].echoPin,
                      (unsigned)SENSOR_ESPNOW_SLOT[i]);
        warnIfReservedPin(i, SENSOR_PINS[i].trigPin, "Trig");
        warnIfReservedPin(i, SENSOR_PINS[i].echoPin, "Echo");
    }
    warnIfReservedPin(SENSOR_COUNT, BUZZER_PIN, "Buzzer");
    Serial.printf("Valid range: %.1f - %.1f cm\n", MIN_DISTANCE_CM, MAX_DISTANCE_CM);
    Serial.println("========================================");

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        for (size_t i = 0; i < SENSOR_COUNT; ++i)
        {
            SensorReading reading = s_sensors[i].readOnce();

            if (reading.error != nullptr)
            {
                s_invalidCount[i]++;

                float stableCm;
                bool hasStable = s_filters[i].getStable(stableCm);
                Serial.printf(
                    "[S%u] REJECT: %s | Pulse: %lu us | Raw: %s | Stable: %s | Invalid: %d\n",
                    (unsigned)i,
                    reading.error,
                    (unsigned long)reading.durationUs,
                    distanceToText(reading.durationUs > 0, reading.distanceCm).c_str(),
                    distanceToText(hasStable, stableCm).c_str(),
                    s_invalidCount[i]);

                if (s_invalidCount[i] >= FILTER_RESET_AFTER_INVALID)
                {
                    s_filters[i].reset();
                    sharedStateSet(i, 0.0f, false);
                    s_invalidCount[i] = 0;
                }
            }
            else
            {
                s_invalidCount[i] = 0;

                FilterResult result = s_filters[i].process(reading.distanceCm);

                sharedStateSet(i, result.outputCm, result.hasOutput);
            }
        }

        // vTaskDelayUntil giữ chu kỳ đo đều đặn, không cộng dồn độ trễ
        // và không chặn các task khác trong lúc chờ.
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(MEASURE_INTERVAL_MS));
    }
}

// =========================================================
// NETWORK TASK (core 0) — ESP-NOW tới waveshare-screen
// =========================================================

static void networkTask(void *pvParameters)
{
    (void)pvParameters;
    s_espNowClient.begin();

    uint32_t lastSendMs = 0;

    for (;;)
    {
        uint32_t now = millis();
        if (now - lastSendMs >= ESPNOW_SEND_INTERVAL_MS)
        {
            lastSendMs = now;

            // Message luôn mang đủ ESPNOW_SENSOR_SLOT_COUNT (6) vị trí.
            // Chỉ slot có phần cứng thật được set valid=1; slot không lắp
            // giữ valid=0 -> waveshare-screen hiển thị "--".
            espnow_sensor_msg_t msg = {};

            for (size_t i = 0; i < SENSOR_COUNT; ++i)
            {
                float distanceCm;
                uint8_t slot = SENSOR_ESPNOW_SLOT[i];
                if (sharedStateGet(i, distanceCm))
                {
                    msg.distance_cm[slot] = distanceCm;
                    msg.valid[slot] = 1;
                }
            }

            s_espNowClient.sendReading(msg);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// =========================================================
// COREIOT TASK (chỉ khi USE_COREIOT=1)
// =========================================================

#if USE_COREIOT
// Hỗ trợ đọc giá trị thô cho telemetry (0 nếu chưa hợp lệ)
static float sharedStateGetValue(size_t sensorIndex)
{
    float v = 0.0f;
    sharedStateGet(sensorIndex, v);
    return v;
}

static void coreiotTask(void *pvParameters)
{
    (void)pvParameters;
    s_coreiotClient.begin();

    uint32_t lastPublishMs = 0;

    for (;;)
    {
        s_coreiotClient.loop();

        uint32_t now = millis();
        if (now - lastPublishMs >= COREIOT_PUBLISH_INTERVAL_MS)
        {
            lastPublishMs = now;

            // Telemetry: khoảng cách 6 slot + giá trị gần nhất (cm).
            // JSON encode đơn giản, không dùng thư viện JSON trên Arduino.
            char payload[256];
            float nearestCm = 0.0f;
            bool hasNearest = sharedStateGetNearest(nearestCm);
            int len = snprintf(
                payload, sizeof(payload),
                "{\"d1\":%.1f,\"d2\":%.1f,\"d3\":%.1f,\"d4\":%.1f,\"d5\":%.1f,\"d6\":%.1f,\"nearest_cm\":%.1f,\"has_nearest\":%s}",
                sharedStateGetValue(0), sharedStateGetValue(1), sharedStateGetValue(2),
                sharedStateGetValue(3), sharedStateGetValue(4), sharedStateGetValue(5),
                nearestCm, hasNearest ? "true" : "false");
            (void)len;

            if (!s_coreiotClient.publishTelemetry(payload))
            {
                // Bỏ qua: loop() sẽ tự duy trì kết nối.
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
#endif // USE_COREIOT

// =========================================================
// SETUP / LOOP
// =========================================================

void setup()
{
    Serial.begin(115200);
    delay(200);

    sharedStateInit();

    // Task đọc/lọc cảm biến - ưu tiên cao hơn vì có ràng buộc thời gian
    // (timeout Echo tính bằng us). Core 1.
    xTaskCreatePinnedToCore(
        sensorTask,
        "SensorTask",
        4096,
        nullptr,
        2,
        &s_sensorTaskHandle,
        1);

    // Task mạng (ESP-NOW) - core 0, tách khỏi core 1 (đo/lọc cảm biến)
    // để gửi không ảnh hưởng timing đo.
    xTaskCreatePinnedToCore(
        networkTask,
        "NetworkTask",
        4096,
        nullptr,
        1,
        &s_networkTaskHandle,
        0);

    // Task buzzer - core 0 (toggle GPIO millis()).
    xTaskCreatePinnedToCore(
        buzzerTask,
        "BuzzerTask",
        2048,
        nullptr,
        1,
        &s_buzzerTaskHandle,
        0);

#if USE_COREIOT
    // Task CoreIoT/MQTT - core 0 (publish telemetry).
    xTaskCreatePinnedToCore(
        coreiotTask,
        "CoreIoTTask",
        4096,
        nullptr,
        1,
        &s_coreiotTaskHandle,
        0);
#endif
}

void loop()
{
    // Toàn bộ xử lý đã chuyển vào FreeRTOS task ở trên,
    // nên không cần dùng loop() nữa.
    vTaskDelete(nullptr);
}
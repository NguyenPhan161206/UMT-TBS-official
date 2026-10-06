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
#include "soak_heartbeat.h"
#include "task_cfg.h"
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

// Chu kỳ gói ESP-NOW (sequence counter)
static uint16_t s_espnow_seq = 0;

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
    vTaskDelay(pdMS_TO_TICKS(SENSOR_SETTLE_DELAY_MS));

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
    uint32_t loopCount = 0;

    for (;;)
    {
        ++loopCount;
        for (size_t i = 0; i < SENSOR_COUNT; ++i)
        {
            /* Cảm biến đã xác nhận DISCONNECTED: chỉ thăm dò lại 1 lần mỗi 10 chu kỳ (~1s).
             * Tránh phí 40ms timeout trên mỗi chân chưa cắm, giúp vòng lặp đo của cảm biến
             * đang hoạt động chạy đúng chuẩn 100ms siêu nhạy. */
            sensor_health_t curHealth = sharedStateGetHealth(i);
#if TBS_ACCURACY_PROBE
            // Env yolo_uno_accuracy: cổng đã từng có xung Echo (có cảm biến) được đọc mọi chu kỳ kể cả
            // khi DISCONNECTED, để tỷ lệ phát hiện khi đo góc búp/tầm đo không lệch vì bỏ nhịp. Cổng
            // trống vẫn thăm dò 1 lần/10 chu kỳ như bản thường, giữ nhịp 10 mẫu/s cho cảm biến đang đo.
            static bool s_everEcho[SENSOR_COUNT] = {false};
            if (!s_everEcho[i] && curHealth == SENSOR_HEALTH_DISCONNECTED && ((loopCount + i) % 10 != 0))
#else
            if (curHealth == SENSOR_HEALTH_DISCONNECTED && ((loopCount + i) % 10 != 0))
#endif
            {
                continue;
            }

            SensorReading reading = s_sensors[i].readOnce();
#if TBS_ACCURACY_PROBE
            if (reading.durationUs > 0)
            {
                s_everEcho[i] = true;
            }
#endif

            if (reading.error != nullptr)
            {
#if TBS_ACCURACY_PROBE
                // DMXT-55/56: calc = khoảng cách tính từ xung (nan nếu không có Echo); rej để cuối
                // vì lý do có dấu cách. Định dạng: docs/ACCURACY_TEST.md.
                if (reading.durationUs > 0)
                    Serial.printf("ACC S%u raw=nan pulse=%lu calc=%.1f rej=%s\n", (unsigned)i,
                                  (unsigned long)reading.durationUs, reading.distanceCm, reading.error);
                else
                    Serial.printf("ACC S%u raw=nan pulse=0 calc=nan rej=%s\n", (unsigned)i, reading.error);
#endif
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

                /* Dynamic Fast-disconnect (Asymmetric Timeout):
                 * Nếu xe đang ở gần (< 150cm) mà bị mất tín hiệu, xe máy đã đi khuất vào hư không.
                 * Chỉ chờ 2 nhịp (200ms) để ngắt cảnh báo thay vì chờ 3 nhịp (300ms). */
                int missThreshold = (hasStable && stableCm < 150.0f) ? 2 : SENSOR_FAULT_CONSECUTIVE_MISS;

                if (s_invalidCount[i] >= missThreshold)
                {
                    s_filters[i].reset();
                    sharedStateSet(i, 0.0f, false);
                    sharedStateSetHealth(i, SENSOR_HEALTH_DISCONNECTED);
                    s_invalidCount[i] = 0;
                    Serial.printf("[S%u] DISCONNECTED (x%d miss)\n",
                                  (unsigned)i, missThreshold);
                }
                /* Fallback (vẫn giữ FILTER_RESET_AFTER_INVALID cũ làm an toàn lưới bộ lọc). */
                else if (s_invalidCount[i] >= FILTER_RESET_AFTER_INVALID)
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
#if TBS_ACCURACY_PROBE
                if (result.hasOutput)
                    Serial.printf("ACC S%u raw=%.1f out=%.1f has=1 status=%s n=%d pulse=%lu\n", (unsigned)i,
                                  reading.distanceCm, result.outputCm, result.status, result.clusterCount,
                                  (unsigned long)reading.durationUs);
                else
                    Serial.printf("ACC S%u raw=%.1f out=nan has=0 status=%s n=%d pulse=%lu\n", (unsigned)i,
                                  reading.distanceCm, result.status, result.clusterCount,
                                  (unsigned long)reading.durationUs);
#endif

                sharedStateSet(i, result.outputCm, result.hasOutput);

                /* Xác định health theo kết quả đo. */
                sensor_health_t h = result.hasOutput
                                    ? SENSOR_HEALTH_OK
                                    : SENSOR_HEALTH_OUT_OF_RANGE;
                sharedStateSetHealth(i, h);
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
            // Chỉ slot có phần cứng thật được set valid=1 và health=OK;
            // slot không lắp giữ valid=0 và health=DISCONNECTED.
            espnow_sensor_msg_t msg = {};
            ++s_espnow_seq;
            msg.seq = s_espnow_seq;

            for (size_t i = 0; i < SENSOR_COUNT; ++i)
            {
                float distanceCm;
                uint8_t slot = SENSOR_ESPNOW_SLOT[i];
                sensor_health_t h = sharedStateGetHealth(i);
                msg.health[slot] = (uint8_t)h;
                if (sharedStateGet(i, distanceCm))
                {
                    msg.distance_cm[slot] = distanceCm;
                    msg.valid[slot] = 1;
                }
            }

            s_espNowClient.sendReading(msg);
            Serial.printf("DIST: [%.1f, %.1f, %.1f, %.1f, %.1f, %.1f]\n",
                          msg.distance_cm[0], msg.distance_cm[1], msg.distance_cm[2],
                          msg.distance_cm[3], msg.distance_cm[4], msg.distance_cm[5]);
#if TBS_LATENCY_PROBE
            // Tuổi mẫu lúc đóng gói: mẫu mới nhất / cũ nhất trong các cảm biến hợp lệ
            // (-1 nếu không có cảm biến nào hợp lệ). Xem docs/LATENCY_TEST.md.
            long ageMin = -1, ageMax = -1;
            for (size_t i = 0; i < SENSOR_COUNT; ++i)
            {
                if (!msg.valid[SENSOR_ESPNOW_SLOT[i]])
                {
                    continue;
                }
                long age = (long)(now - sharedStateGetUpdatedMs(i));
                if (ageMin < 0 || age < ageMin) ageMin = age;
                if (age > ageMax) ageMax = age;
            }
            Serial.printf("LAT TX seq=%u age_min_ms=%ld age_max_ms=%ld\n",
                          (unsigned)msg.seq, ageMin, ageMax);
#endif
        }

        SoakNodeCounters soak = {s_espNowClient.txOk(), s_espNowClient.txFail(), -1};
#if USE_COREIOT
        soak.mqttReconnects = (int32_t)s_coreiotClient.reconnectCount();
#endif
        soakHeartbeatPoll(millis(), soak);

#if TBS_LATENCY_PROBE
        EspNowRtt rtt;
        while (s_espNowClient.pollRtt(rtt))
        {
            Serial.printf("LAT RTT seq=%u us=%lu\n", (unsigned)rtt.seq, (unsigned long)rtt.rttUs);
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(TASK_POLL_INTERVAL_MS));
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
            // "seq": rule-chain chuyển tiếp sang màn hình để đo độ trễ đường MQTT (DMXT-57).
            static uint32_t s_mqttSeq = 0;
            ++s_mqttSeq;
            char payload[256];
            float nearestCm = 0.0f;
            bool hasNearest = sharedStateGetNearest(nearestCm);
            int len = snprintf(
                payload, sizeof(payload),
                "{\"d1\":%.1f,\"d2\":%.1f,\"d3\":%.1f,\"d4\":%.1f,\"d5\":%.1f,\"d6\":%.1f,\"nearest_cm\":%.1f,\"has_nearest\":%s,\"seq\":%lu}",
                sharedStateGetValue(0), sharedStateGetValue(1), sharedStateGetValue(2),
                sharedStateGetValue(3), sharedStateGetValue(4), sharedStateGetValue(5),
                nearestCm, hasNearest ? "true" : "false", (unsigned long)s_mqttSeq);
            (void)len;

#if TBS_LATENCY_PROBE
            // In TRƯỚC publish: mốc PC của dòng này là thời điểm bắt đầu gửi. Chỉ in khi MQTT đang
            // kết nối, nếu không lần publish chắc chắn thất bại bị script tính nhầm là gói mất.
            if (s_coreiotClient.isConnected())
            {
                Serial.printf("LAT MQTT_TX seq=%lu\n", (unsigned long)s_mqttSeq);
            }
#endif
            if (!s_coreiotClient.publishTelemetry(payload))
            {
                // Bỏ qua: loop() sẽ tự duy trì kết nối.
            }
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_POLL_INTERVAL_MS));
    }
}
#endif // USE_COREIOT

// =========================================================
// SETUP / LOOP
// =========================================================

void setup()
{
    Serial.begin(115200);
    delay(SERIAL_SETUP_DELAY_MS);
    soakHeartbeatBoot();

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

    soakHeartbeatSetTasks(s_sensorTaskHandle, s_networkTaskHandle, s_buzzerTaskHandle);
}

void loop()
{
    // Toàn bộ xử lý đã chuyển vào FreeRTOS task ở trên,
    // nên không cần dùng loop() nữa.
    vTaskDelete(nullptr);
}
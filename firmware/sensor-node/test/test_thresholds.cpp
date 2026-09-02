// test_thresholds.cpp — Host unit test cho shared contract (R3/R4).
// Verify: SENSOR_COUNT từ sizeof, zone thresholds, buzzer thresholds,
// ESP-NOW wire schema khớp. Chạy bởi test_runner.cpp (main duy nhất).
#include <unity.h>

#include "espnow_protocol.h"
#include "thresholds.h"

void test_sensor_count_from_sizeof(void)
{
    // R4: SENSOR_COUNT suy từ sizeof(SENSOR_PINS), không gõ tay
    TEST_ASSERT_EQUAL_INT(6, SENSOR_COUNT);
    TEST_ASSERT_EQUAL_INT(6, ESPNOW_SENSOR_SLOT_COUNT);
}

void test_zone_thresholds(void)
{
    // R3: ngưỡng zone 1 nguồn (giữ giá trị cũ: 100/30)
    TEST_ASSERT_EQUAL_INT(100, SENSOR_CAUTION_CM);
    TEST_ASSERT_EQUAL_INT(30, SENSOR_DANGER_CM);
    // Range JSN-SR04T
    TEST_ASSERT_EQUAL_INT(20, SENSOR_RANGE_MIN_CM);
    TEST_ASSERT_EQUAL_INT(600, SENSOR_RANGE_MAX_CM);
}

void test_buzzer_thresholds(void)
{
    // Buzzer (sensor-node local): 50/20
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 50.0f, BUZZER_WARNING_DISTANCE_CM);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, BUZZER_DANGER_DISTANCE_CM);
    TEST_ASSERT_EQUAL_INT(11, BUZZER_PIN);
}

void test_sensor_pins_map_to_slots(void)
{
    // PINS: FRONT(0), LEFT_FRONT(1), RIGHT_FRONT(2), LEFT_REAR(3), RIGHT_REAR(4), REAR(5)
    // Wire  : FRONT(0), REAR(1), LEFT_FRONT(2), LEFT_REAR(3), RIGHT_FRONT(4), RIGHT_REAR(5)
    // Ánh xạ vật lý->slot KHÔNG trùng thứ tự — bộ đếm slot phải đủ 6 slot duy nhất.
    bool seen[6] = {false};
    const uint8_t map[SENSOR_COUNT] = {
        ESPNOW_SLOT_FRONT,       // PINS[0]
        ESPNOW_SLOT_LEFT_FRONT,  // PINS[1]
        ESPNOW_SLOT_RIGHT_FRONT, // PINS[2]
        ESPNOW_SLOT_LEFT_REAR,   // PINS[3]
        ESPNOW_SLOT_RIGHT_REAR,  // PINS[4]
        ESPNOW_SLOT_REAR,        // PINS[5]
    };
    for (size_t i = 0; i < SENSOR_COUNT; ++i)
    {
        TEST_ASSERT_LESS_THAN(6, map[i]);
        seen[map[i]] = true;
    }
    for (int s = 0; s < 6; ++s)
    {
        TEST_ASSERT_TRUE_MESSAGE(seen[s], "slot không được ánh xạ -> sai nhãn dashboard");
    }
}

void test_espnow_msg_size(void)
{
    // Packed struct: 6*float + 6*uint8 = 30 byte. Lệch layout = lệch dữ liệu giữa 2 board.
    TEST_ASSERT_EQUAL_INT(30, (int)sizeof(espnow_sensor_msg_t));
}

// setup()/loop() nằm ở test_runner.cpp (main duy nhất cho C++ host).
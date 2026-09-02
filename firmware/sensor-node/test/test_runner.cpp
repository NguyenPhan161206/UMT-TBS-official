// test_runner.cpp — Host runner duy nhất cho unity (native).
// PlatformIO native test links MỌI file test/ vào một binary; Unity cần
// đúng một entry point. Đây là nơi duy nhất định nghĩa main().
#include <unity.h>

// Test functions defined in the other test_*.cpp files.
void test_sensor_count_from_sizeof(void);
void test_zone_thresholds(void);
void test_buzzer_thresholds(void);
void test_sensor_pins_map_to_slots(void);
void test_espnow_msg_size(void);

void test_filter_stable_readings(void);
void test_filter_warmup(void);
void test_filter_noise_keeps_old(void);
void test_filter_jump_hold_then_accept(void);
void test_filter_reset(void);

int main(void)
{
    UNITY_BEGIN();

    // thresholds / shared contract (R3/R4)
    RUN_TEST(test_sensor_count_from_sizeof);
    RUN_TEST(test_zone_thresholds);
    RUN_TEST(test_buzzer_thresholds);
    RUN_TEST(test_sensor_pins_map_to_slots);
    RUN_TEST(test_espnow_msg_size);

    // distance_filter
    RUN_TEST(test_filter_stable_readings);
    RUN_TEST(test_filter_warmup);
    RUN_TEST(test_filter_noise_keeps_old);
    RUN_TEST(test_filter_jump_hold_then_accept);
    RUN_TEST(test_filter_reset);

    return UNITY_END();
}
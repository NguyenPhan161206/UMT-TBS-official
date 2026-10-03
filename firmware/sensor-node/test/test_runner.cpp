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

// distance_filter: test cases bổ sung
void test_filter_two_instances_independent(void);
void test_filter_history_full(void);
void test_filter_max_range_input(void);
void test_filter_fast_track_crossing(void);
void test_filter_stable_at_danger_boundary(void);
void test_filter_cluster_dynamic_tolerance(void);

// distance_filter: nhả cảnh báo cần N mẫu xa ("nhanh vào – chậm ra")
void test_filter_release_ignores_isolated_far_echoes(void);
void test_filter_release_after_confirmed_far_samples(void);
void test_filter_near_target_with_ghost_echoes_stays_near(void);

// Unity stubs: cân bằng giới hạn API của các trình chạy chất lượng (GCC,
// MinGW). Hàm bỏ trống vì các test dùng fixture cục bộ, không state chung.
void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();

    // thresholds / shared contract (R3/R4)
    RUN_TEST(test_sensor_count_from_sizeof);
    RUN_TEST(test_zone_thresholds);
    RUN_TEST(test_buzzer_thresholds);
    RUN_TEST(test_sensor_pins_map_to_slots);
    RUN_TEST(test_espnow_msg_size);

    // distance_filter — hành vi cơ bản
    RUN_TEST(test_filter_stable_readings);
    RUN_TEST(test_filter_warmup);
    RUN_TEST(test_filter_noise_keeps_old);
    RUN_TEST(test_filter_jump_hold_then_accept);
    RUN_TEST(test_filter_reset);

    // distance_filter — edge case & robustness
    RUN_TEST(test_filter_two_instances_independent);
    RUN_TEST(test_filter_history_full);
    RUN_TEST(test_filter_max_range_input);
    RUN_TEST(test_filter_fast_track_crossing);
    RUN_TEST(test_filter_stable_at_danger_boundary);
    RUN_TEST(test_filter_cluster_dynamic_tolerance);

    // distance_filter — nhả cảnh báo cần N mẫu xa
    RUN_TEST(test_filter_release_ignores_isolated_far_echoes);
    RUN_TEST(test_filter_release_after_confirmed_far_samples);
    RUN_TEST(test_filter_near_target_with_ghost_echoes_stays_near);

    return UNITY_END();
}
/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * test_hazard_core.c — mini assert-runner cho hazard_core (G1 T1.3).
 * KHÔNG dùng Unity (tránh FetchContent thừa ở bước test-core; xem
 * ARCHITECTURE_G1_TESTING.md). C thuần + exit code:
 *   exit 0 = pass; exit 1 = có FAIL (stderr in chi tiết).
 *
 * Run: cmake -S firmware/waveshare-screen/host_sim -B /tmp/host_sim \
 *          && cmake --build /tmp/host_sim && /tmp/host_sim/hazard_core_tests
 */

#include <stdio.h>
#include <stdint.h>

#include "hazard_core.h"

static int s_pass = 0;
static int s_fail = 0;

#define CHECK(cond, msg)                                                    \
    do {                                                                    \
        if (cond) {                                                         \
            s_pass++;                                                       \
        } else {                                                            \
            s_fail++;                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);   \
        }                                                                   \
    } while (0)

static void test_classify_bounds(void)
{
    CHECK(hazard_classify(19) == SENSOR_ZONE_DANGER, "19cm -> DANGER");
    CHECK(hazard_classify(29) == SENSOR_ZONE_DANGER, "29cm (< DANGER_CM) -> DANGER");
    CHECK(hazard_classify(30) == SENSOR_ZONE_CAUTION, "30cm NOT < DANGER_CM -> CAUTION (boundary dưới)");
    CHECK(hazard_classify(31) == SENSOR_ZONE_CAUTION, "31cm -> CAUTION");
    CHECK(hazard_classify(99) == SENSOR_ZONE_CAUTION, "99cm -> CAUTION");
    CHECK(hazard_classify(100) == SENSOR_ZONE_CAUTION, "100cm == CAUTION_CM -> CAUTION (x <= CAUTION)");
    CHECK(hazard_classify(101) == SENSOR_ZONE_SAFE, "101cm -> SAFE");
    CHECK(hazard_classify(0) == SENSOR_ZONE_DANGER, "0cm -> DANGER (nhưng UI skip stale)");
    CHECK(hazard_classify(600) == SENSOR_ZONE_SAFE, "600cm range max -> SAFE");
}

static void test_worst_zone_skip_stale(void)
{
    /* Sensor chưa report (stale) với distance_cm=0 KHÔNG được tính DANGER. */
    const uint16_t dist_all_stale[] = {0, 0, 0, 0, 0, 0};
    const bool stale_all[] = {true, true, true, true, true, true};
    CHECK(hazard_worst_zone(dist_all_stale, stale_all, 6) == SENSOR_ZONE_SAFE, "all stale -> SAFE");

    const uint16_t dist_one_live_danger[] = {0, 20, 0, 0, 0, 0};
    const bool stale_one_live[] = {true, false, true, true, true, true};
    CHECK(hazard_worst_zone(dist_one_live_danger, stale_one_live, 6) == SENSOR_ZONE_DANGER,
          "1 live DANGER -> DANGER");

    const uint16_t dist_live_caution[] = {55, 0, 0, 0, 0, 0};
    const bool stale_live_caution[] = {false, true, true, true, true, true};
    CHECK(hazard_worst_zone(dist_live_caution, stale_live_caution, 6) == SENSOR_ZONE_CAUTION,
          "1 live CAUTION -> CAUTION");

    const uint16_t dist_mixed[] = {55, 20, 150, 0, 0, 0};
    const bool stale_mixed[] = {false, false, false, true, true, true};
    CHECK(hazard_worst_zone(dist_mixed, stale_mixed, 6) == SENSOR_ZONE_DANGER,
          "live 20cm -> DANGER beats CAUTION");

    /* Edge: n = 0 và NULL -> SAFE (không crash). */
    CHECK(hazard_worst_zone(NULL, NULL, 0) == SENSOR_ZONE_SAFE, "NULL/NULL/0 -> SAFE");
}

static void test_crossing_delta(void)
{
    /* Front close (100 < 150), side LEFT_FRONT delta 39/40/41. */
    const uint16_t cur[] = {100, 1000, 100, 1000, 1000, 1000};
    const uint16_t prev_delta39[] = {100, 1000, 61, 1000, 1000, 1000};
    hazard_crossing_result_t r39 = hazard_eval_crossing(cur, prev_delta39, 6);
    CHECK(r39.active == false, "delta 39 -> inactive");

    const uint16_t prev_delta40[] = {100, 1000, 60, 1000, 1000, 1000};
    hazard_crossing_result_t r40 = hazard_eval_crossing(cur, prev_delta40, 6);
    CHECK(r40.active == true, "delta 40 -> active");
    CHECK(r40.sensor == ESPNOW_SLOT_LEFT_FRONT, "delta40 sensor = LEFT_FRONT");

    const uint16_t prev_delta41[] = {100, 1000, 59, 1000, 1000, 1000};
    hazard_crossing_result_t r41 = hazard_eval_crossing(cur, prev_delta41, 6);
    CHECK(r41.active == true, "delta 41 -> active");

    /* delta âm (vật đi tới gần) cũng tính trị tuyệt đối. */
    const uint16_t cur_neg[] = {100, 1000, 60, 1000, 1000, 1000};
    const uint16_t prev_neg[] = {100, 1000, 100, 1000, 1000, 1000};
    hazard_crossing_result_t r_neg = hazard_eval_crossing(cur_neg, prev_neg, 6);
    CHECK(r_neg.active == true, "delta -40 (tiến gần) -> active");
}

static void test_crossing_front_threshold(void)
{
    /* FRONT giới hạn 150: 149 close, 150/151 không. */
    const uint16_t prev[] = {0, 1000, 60, 1000, 1000, 1000};

    const uint16_t cur_front149[] = {149, 1000, 100, 1000, 1000, 1000};
    CHECK(hazard_eval_crossing(cur_front149, prev, 6).active == true, "FRONT=149 -> active");

    const uint16_t cur_front150[] = {150, 1000, 100, 1000, 1000, 1000};
    CHECK(hazard_eval_crossing(cur_front150, prev, 6).active == false, "FRONT=150 -> inactive");

    const uint16_t cur_front151[] = {151, 1000, 100, 1000, 1000, 1000};
    CHECK(hazard_eval_crossing(cur_front151, prev, 6).active == false, "FRONT=151 -> inactive");

    /* Không có side fast -> NO_SENSOR, kể cả khi front close. */
    const uint16_t cur_no_fast[] = {100, 1000, 100, 1000, 1000, 1000};
    const uint16_t prev_no_fast[] = {100, 1000, 100, 1000, 1000, 1000}; /* delta 0 */
    hazard_crossing_result_t r_no_fast = hazard_eval_crossing(cur_no_fast, prev_no_fast, 6);
    CHECK(r_no_fast.active == false, "front close nhưng no side fast -> inactive");
    CHECK(r_no_fast.sensor == HAZARD_CROSSING_NO_SENSOR, "no fast -> NO_SENSOR");

    /* NULL guard -> inactive + NO_SENSOR (không crash). */
    hazard_crossing_result_t r_null = hazard_eval_crossing(NULL, NULL, 6);
    CHECK(r_null.active == false && r_null.sensor == HAZARD_CROSSING_NO_SENSOR,
          "NULL cur/prev -> inactive + NO_SENSOR");
}

int main(void)
{
    test_classify_bounds();
    test_worst_zone_skip_stale();
    test_crossing_delta();
    test_crossing_front_threshold();

    if (s_fail != 0) {
        fprintf(stderr, "hazard_core_tests: %d/%d FAILED\n", s_fail, s_pass + s_fail);
        return 1;
    }
    printf("hazard_core_tests: %d checks PASSED\n", s_pass);
    return 0;
}
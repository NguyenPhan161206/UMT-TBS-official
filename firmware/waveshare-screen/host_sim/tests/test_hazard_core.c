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
#include <string.h>

#include "hazard_core.h"
#include "scenarios_gen.h"   /* sinh từ tools/scenarios.py (cùng nguồn với sim và test MQTT) */

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
    CHECK(hazard_classify(30) == SENSOR_ZONE_DANGER, "30cm == DANGER_CM -> DANGER (x <= DANGER_CM)");
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
    CHECK(hazard_worst_zone(dist_all_stale, stale_all, NULL, 6) == SENSOR_ZONE_SAFE, "all stale -> SAFE");

    const uint16_t dist_one_live_danger[] = {0, 20, 0, 0, 0, 0};
    const bool stale_one_live[] = {true, false, true, true, true, true};
    CHECK(hazard_worst_zone(dist_one_live_danger, stale_one_live, NULL, 6) == SENSOR_ZONE_DANGER,
          "1 live DANGER -> DANGER");

    const uint16_t dist_live_caution[] = {55, 0, 0, 0, 0, 0};
    const bool stale_live_caution[] = {false, true, true, true, true, true};
    CHECK(hazard_worst_zone(dist_live_caution, stale_live_caution, NULL, 6) == SENSOR_ZONE_CAUTION,
          "1 live CAUTION -> CAUTION");

    const uint16_t dist_mixed[] = {55, 20, 150, 0, 0, 0};
    const bool stale_mixed[] = {false, false, false, true, true, true};
    CHECK(hazard_worst_zone(dist_mixed, stale_mixed, NULL, 6) == SENSOR_ZONE_DANGER,
          "live 20cm -> DANGER beats CAUTION");

    /* Safety-Critical: Sensor có health=DISCONNECTED hoặc STALE không được dùng khoảng cách cũ để báo DANGER */
    const uint16_t dist_fault[] = {20, 20, 20, 20, 20, 20};
    const bool not_stale[6] = {false, false, false, false, false, false};
    const uint8_t health_all_disconnected[6] = {
        (uint8_t)SENSOR_HEALTH_DISCONNECTED, (uint8_t)SENSOR_HEALTH_DISCONNECTED,
        (uint8_t)SENSOR_HEALTH_DISCONNECTED, (uint8_t)SENSOR_HEALTH_DISCONNECTED,
        (uint8_t)SENSOR_HEALTH_DISCONNECTED, (uint8_t)SENSOR_HEALTH_DISCONNECTED
    };
    CHECK(hazard_worst_zone(dist_fault, not_stale, health_all_disconnected, 6) == SENSOR_ZONE_SAFE,
          "all disconnected sensors -> SAFE even if old dist was 20cm");
    CHECK(hazard_has_sensor_fault(health_all_disconnected, 6) == true, "has_sensor_fault -> true");

    const uint8_t health_one_ok_danger[6] = {
        (uint8_t)SENSOR_HEALTH_OK, (uint8_t)SENSOR_HEALTH_DISCONNECTED,
        (uint8_t)SENSOR_HEALTH_STALE, (uint8_t)SENSOR_HEALTH_DISCONNECTED,
        (uint8_t)SENSOR_HEALTH_DISCONNECTED, (uint8_t)SENSOR_HEALTH_DISCONNECTED
    };
    CHECK(hazard_worst_zone(dist_fault, not_stale, health_one_ok_danger, 6) == SENSOR_ZONE_DANGER,
          "one live OK sensor with 20cm -> DANGER");
    CHECK(hazard_has_sensor_fault(health_one_ok_danger, 6) == true, "has_sensor_fault -> true");

    const uint8_t health_all_ok[6] = {0, 0, 0, 0, 0, 0};
    CHECK(hazard_has_sensor_fault(health_all_ok, 6) == false, "all OK -> no sensor fault");

    /* Edge: n = 0 và NULL -> SAFE (không crash). */
    CHECK(hazard_worst_zone(NULL, NULL, NULL, 0) == SENSOR_ZONE_SAFE, "NULL/NULL/NULL/0 -> SAFE");
}

/* Hai khung "trước → sau" cách nhau 100 ms trên state mới (khung đầu đặt mốc tham chiếu). */
static hazard_crossing_result_t crossing_pair(const uint16_t *prev, const bool *ok_prev,
                                              const uint16_t *cur, const bool *ok_cur)
{
    hazard_crossing_state_t st = {0};
    (void)hazard_eval_crossing(&st, prev, ok_prev, 6, 1000);
    return hazard_eval_crossing(&st, cur, ok_cur, 6, 1100);
}

static void test_crossing_delta(void)
{
    /* Front close (100 < 150), side LEFT_FRONT delta 39/40/41. */
    const uint16_t cur[] = {100, 1000, 100, 1000, 1000, 1000};
    const uint16_t prev_delta39[] = {100, 1000, 61, 1000, 1000, 1000};
    CHECK(crossing_pair(prev_delta39, NULL, cur, NULL).active == false, "delta 39 -> inactive");

    const uint16_t prev_delta40[] = {100, 1000, 60, 1000, 1000, 1000};
    hazard_crossing_result_t r40 = crossing_pair(prev_delta40, NULL, cur, NULL);
    CHECK(r40.active == true, "delta 40 -> active");
    CHECK(r40.sensor == ESPNOW_SLOT_LEFT_FRONT, "delta40 sensor = LEFT_FRONT");

    const uint16_t prev_delta41[] = {100, 1000, 59, 1000, 1000, 1000};
    CHECK(crossing_pair(prev_delta41, NULL, cur, NULL).active == true, "delta 41 -> active");

    /* delta âm (vật đi tới gần) cũng tính trị tuyệt đối. */
    const uint16_t cur_neg[] = {100, 1000, 60, 1000, 1000, 1000};
    const uint16_t prev_neg[] = {100, 1000, 100, 1000, 1000, 1000};
    CHECK(crossing_pair(prev_neg, NULL, cur_neg, NULL).active == true, "delta -40 (tiến gần) -> active");

    /* Góc phải trước cũng được xét. */
    const uint16_t prev_rf[] = {100, 1000, 1000, 1000, 200, 1000};
    const uint16_t cur_rf[] = {100, 1000, 1000, 1000, 150, 1000};
    hazard_crossing_result_t r_rf = crossing_pair(prev_rf, NULL, cur_rf, NULL);
    CHECK(r_rf.active == true && r_rf.sensor == ESPNOW_SLOT_RIGHT_FRONT, "RIGHT_FRONT delta 50 -> active, sensor RF");

    /* Slot bên SAU đổi nhanh (xe chạy dọc hông) khi đầu xe gần: không phải cắt ngang phía trước. */
    const uint16_t prev_rr[] = {100, 1000, 1000, 1000, 1000, 200};
    const uint16_t cur_rr[] = {100, 1000, 1000, 1000, 1000, 150};
    CHECK(crossing_pair(prev_rr, NULL, cur_rr, NULL).active == false, "RIGHT_REAR đổi nhanh -> inactive");
    const uint16_t prev_lr[] = {100, 1000, 1000, 200, 1000, 1000};
    const uint16_t cur_lr[] = {100, 1000, 1000, 120, 1000, 1000};
    CHECK(crossing_pair(prev_lr, NULL, cur_lr, NULL).active == false, "LEFT_REAR đổi nhanh -> inactive");

    /* REAR (không phải slot bên) đổi nhanh không tính. */
    const uint16_t prev_rear[] = {100, 300, 1000, 1000, 1000, 1000};
    const uint16_t cur_rear[] = {100, 100, 1000, 1000, 1000, 1000};
    CHECK(crossing_pair(prev_rear, NULL, cur_rear, NULL).active == false, "REAR đổi nhanh -> inactive");
}

static void test_crossing_front_threshold(void)
{
    /* FRONT giới hạn 150: 149 close, 150/151 không. */
    const uint16_t prev[] = {100, 1000, 60, 1000, 1000, 1000};

    const uint16_t cur_front149[] = {149, 1000, 100, 1000, 1000, 1000};
    CHECK(crossing_pair(prev, NULL, cur_front149, NULL).active == true, "FRONT=149 -> active");

    const uint16_t cur_front150[] = {150, 1000, 100, 1000, 1000, 1000};
    CHECK(crossing_pair(prev, NULL, cur_front150, NULL).active == false, "FRONT=150 -> inactive");

    const uint16_t cur_front151[] = {151, 1000, 100, 1000, 1000, 1000};
    CHECK(crossing_pair(prev, NULL, cur_front151, NULL).active == false, "FRONT=151 -> inactive");

    /* Không có side fast -> NO_SENSOR, kể cả khi front close. */
    const uint16_t still[] = {100, 1000, 100, 1000, 1000, 1000};
    hazard_crossing_result_t r_no_fast = crossing_pair(still, NULL, still, NULL);
    CHECK(r_no_fast.active == false, "front close nhưng no side fast -> inactive");
    CHECK(r_no_fast.sensor == HAZARD_CROSSING_NO_SENSOR, "no fast -> NO_SENSOR");

    /* Khung đầu tiên trên state mới chỉ đặt mốc tham chiếu, không bao giờ kích hoạt. */
    hazard_crossing_state_t st = {0};
    CHECK(hazard_eval_crossing(&st, cur_front149, NULL, 6, 0).active == false, "khung đầu tiên -> inactive");

    /* NULL / n nhỏ -> inactive + NO_SENSOR (không crash, không đổi state). */
    hazard_crossing_state_t st2 = {0};
    hazard_crossing_result_t r_null = hazard_eval_crossing(NULL, still, NULL, 6, 0);
    CHECK(r_null.active == false && r_null.sensor == HAZARD_CROSSING_NO_SENSOR, "NULL state -> inactive + NO_SENSOR");
    CHECK(hazard_eval_crossing(&st2, NULL, NULL, 6, 0).active == false && st2.has_ref == false,
          "NULL cur -> inactive, state không đổi");
    CHECK(hazard_eval_crossing(&st2, still, NULL, 5, 0).active == false && st2.has_ref == false,
          "n=5 -> inactive, state không đổi");
}

/* Slot bị xoá (mất kết nối/stale) có khoảng cách 0 — không được gây báo xe cắt ngang giả. */
static void test_crossing_invalid_slots(void)
{
    const uint16_t normal[] = {100, 120, 120, 120, 120, 120};
    const bool all_ok[] = {true, true, true, true, true, true};
    const uint16_t zeros[] = {0, 0, 0, 0, 0, 0};
    const bool none_ok[] = {false, false, false, false, false, false};

    /* Mất link: watchdog xoá mọi slot rồi đánh giá. Cách cũ: FRONT 0 < 150 và |0-120| >= 40 → báo giả. */
    CHECK(crossing_pair(normal, all_ok, zeros, none_ok).active == false, "mất link (mọi slot 0, ok=false) -> inactive");

    /* FRONT stale (0) + slot bên đổi thật 60 cm: không đánh giá được "phía trước" -> inactive. */
    const uint16_t front_stale[] = {0, 120, 60, 120, 120, 120};
    const bool ok_front_stale[] = {false, true, true, true, true, true};
    CHECK(crossing_pair(normal, all_ok, front_stale, ok_front_stale).active == false, "FRONT stale -> inactive");

    /* Slot bên LF rớt (0, ok=false) khi FRONT vẫn gần -> inactive. */
    const uint16_t lf_drop[] = {100, 120, 0, 120, 120, 120};
    const bool ok_lf_drop[] = {true, true, false, true, true, true};
    CHECK(crossing_pair(normal, all_ok, lf_drop, ok_lf_drop).active == false, "slot bên rớt -> inactive");

    /* LF nối lại: mốc tham chiếu của LF đang không hợp lệ -> khung nối lại không được báo. */
    hazard_crossing_state_t st = {0};
    (void)hazard_eval_crossing(&st, lf_drop, ok_lf_drop, 6, 0);          /* mốc: LF không hợp lệ */
    CHECK(hazard_eval_crossing(&st, normal, all_ok, 6, 100).active == false, "slot bên nối lại -> inactive");
    /* ... sau khi mốc làm mới (>= cửa sổ) với LF hợp lệ, chuyển động thật vẫn được phát hiện. */
    (void)hazard_eval_crossing(&st, normal, all_ok, 6, CROSSING_WINDOW_MS);
    const uint16_t lf_move[] = {100, 120, 70, 120, 120, 120};
    CHECK(hazard_eval_crossing(&st, lf_move, all_ok, 6, CROSSING_WINDOW_MS + 100).active == true,
          "sau khi nối lại, chuyển động thật -> active");
}

/* Độ nhạy không phụ thuộc tốc độ khung: so với mốc tham chiếu cũ tới CROSSING_WINDOW_MS. */
static void test_crossing_window(void)
{
    /* Vật cắt ngang 1 m/s, khung 100 ms (ESP-NOW): LF giảm 10 cm/khung. So 2 khung liên tiếp thì không bao giờ báo. */
    hazard_crossing_state_t st = {0};
    uint16_t cur[] = {100, 1000, 120, 1000, 1000, 1000};
    bool fired_by_400 = false, fired_before_400 = false;
    for (uint32_t k = 0; k <= 5; k++) {
        cur[ESPNOW_SLOT_LEFT_FRONT] = (uint16_t)(120 - 10 * k);
        hazard_crossing_result_t r = hazard_eval_crossing(&st, cur, NULL, 6, 100 * k);
        if (r.active && 100 * k < 400) fired_before_400 = true;
        if (r.active && 100 * k <= 400) fired_by_400 = true;
    }
    CHECK(fired_before_400 == false, "1 m/s @100 ms: chưa đủ 40 cm trước 400 ms -> chưa báo");
    CHECK(fired_by_400 == true, "1 m/s @100 ms: 40 cm sau 400 ms so với mốc -> báo");

    /* Khung 500 ms (MQTT): mốc làm mới mỗi khung → so 2 khung liên tiếp như cũ. */
    hazard_crossing_state_t st5 = {0};
    const uint16_t a[] = {100, 1000, 120, 1000, 1000, 1000};
    const uint16_t b[] = {100, 1000, 90, 1000, 1000, 1000};  /* -30 so với a */
    const uint16_t c[] = {100, 1000, 50, 1000, 1000, 1000};  /* -40 so với b */
    (void)hazard_eval_crossing(&st5, a, NULL, 6, 0);
    CHECK(hazard_eval_crossing(&st5, b, NULL, 6, 500).active == false, "500 ms: delta 30 -> inactive");
    CHECK(hazard_eval_crossing(&st5, c, NULL, 6, 1000).active == true, "500 ms: delta 40 so với khung trước -> active");
}

/* Khung đều 100 ms bắt đầu từ t0: khung đầu = a, các khung sau = b (LF nhảy 120 → 60 rồi đứng yên).
 * b khác mốc tham chiếu (a) cho tới khi mốc làm mới ở t0 + CROSSING_WINDOW_MS, nên lần kích hoạt CUỐI là lúc đó.
 * Trả thời điểm (tương đối t0) của khung đầu tiên hết active, hoặc 0 nếu không bao giờ hết trong `span_ms`. */
static uint32_t crossing_hold_release_ms(uint32_t t0, uint32_t span_ms, bool *ever_active)
{
    const uint16_t a[] = {100, 1000, 120, 1000, 1000, 1000};
    const uint16_t b[] = {100, 1000, 60, 1000, 1000, 1000};
    hazard_crossing_state_t st = {0};
    (void)hazard_eval_crossing(&st, a, NULL, 6, t0);
    *ever_active = false;
    for (uint32_t dt = 100; dt <= span_ms; dt += 100) {
        hazard_crossing_result_t r = hazard_eval_crossing(&st, b, NULL, 6, t0 + dt);
        if (r.active) {
            *ever_active = true;
        } else if (*ever_active) {
            return dt;
        }
    }
    return 0;
}

/* Cảnh báo được giữ CROSSING_HOLD_MS sau lần kích hoạt cuối; mất FRONT thì huỷ. */
static void test_crossing_hold(void)
{
    bool active = false;
    uint32_t release = crossing_hold_release_ms(0, 10000, &active);
    CHECK(active == true, "có kích hoạt");
    CHECK(release == CROSSING_WINDOW_MS + CROSSING_HOLD_MS,
          "khung 100 ms: tắt đúng CROSSING_HOLD_MS sau lần kích hoạt cuối (lúc mốc làm mới)");

    /* Sensor vẫn báo đúng slot trong suốt thời gian giữ. */
    const uint16_t a[] = {100, 1000, 120, 1000, 1000, 1000};
    const uint16_t b[] = {100, 1000, 60, 1000, 1000, 1000};
    hazard_crossing_state_t st = {0};
    (void)hazard_eval_crossing(&st, a, NULL, 6, 0);
    for (uint32_t t = 100; t <= 600; t += 100) {
        (void)hazard_eval_crossing(&st, b, NULL, 6, t);
    }
    hazard_crossing_result_t r = hazard_eval_crossing(&st, b, NULL, 6, 2000);
    CHECK(r.active == true && r.sensor == ESPNOW_SLOT_LEFT_FRONT, "đang giữ: active + sensor LEFT_FRONT");

    /* Mất FRONT trong lúc giữ -> huỷ ngay. */
    hazard_crossing_state_t st2 = {0};
    (void)hazard_eval_crossing(&st2, a, NULL, 6, 0);
    CHECK(hazard_eval_crossing(&st2, b, NULL, 6, 100).active == true, "kích hoạt trước khi mất FRONT");
    const bool front_lost[] = {false, true, true, true, true, true};
    CHECK(hazard_eval_crossing(&st2, b, front_lost, 6, 200).active == false, "mất FRONT khi đang giữ -> huỷ");

    /* Tràn uint32: lần kích hoạt cuối 2000 ms TRƯỚC khi bộ đếm quay vòng, hạn giữ rơi SAU điểm tràn
     * (so sánh `now >= until` không chống tràn sẽ tắt ngay — test này bắt được). */
    bool active_wrap = false;
    uint32_t release_wrap = crossing_hold_release_ms((uint32_t)(0u - 2000u - CROSSING_WINDOW_MS), 10000, &active_wrap);
    CHECK(active_wrap == true && release_wrap == CROSSING_WINDOW_MS + CROSSING_HOLD_MS,
          "tràn uint32: giữ và tắt đúng như không tràn");
}

/* Phát kịch bản qua heuristic: mỗi mốc lặp `repeat` khung cách nhau `frame_ms`
 * (repeat > 1 mô phỏng ESP-NOW gửi lại cùng giá trị mỗi 100 ms). true nếu có lúc active. */
static bool scenario_crossing(const char *name, uint32_t frame_ms, int repeat)
{
    const sim_scenario_t *sc = NULL;
    for (int i = 0; i < SIM_SCENARIO_COUNT; i++) {
        if (strcmp(k_sim_scenarios[i].name, name) == 0) {
            sc = &k_sim_scenarios[i];
        }
    }
    if (sc == NULL) {
        fprintf(stderr, "scenario '%s' không có trong scenarios_gen.h\n", name);
        s_fail++;
        return false;
    }

    hazard_crossing_state_t st = {0};
    uint32_t t = 0;
    bool any = false;
    for (int row = 0; row < sc->n; row++) {
        for (int k = 0; k < repeat; k++) {
            any = hazard_eval_crossing(&st, sc->rows[row], NULL, 6, t).active || any;
            t += frame_ms;
        }
    }
    return any;
}

static void test_crossing_scenarios(void)
{
    /* Có cắt ngang phía trước: phải báo ở cả nhịp MQTT (500 ms) lẫn ESP-NOW (100 ms, lặp 5 khung/mốc). */
    static const char *const k_yes[] = {"crossing", "crossing_right"};
    for (size_t i = 0; i < sizeof(k_yes) / sizeof(k_yes[0]); i++) {
        CHECK(scenario_crossing(k_yes[i], 500, 1) == true, k_yes[i]);
        CHECK(scenario_crossing(k_yes[i], 100, 5) == true, k_yes[i]);
    }

    /* Không có gì cắt ngang trước đầu xe (FRONT thoáng, hoặc chỉ FRONT/REAR đổi): không được báo. */
    static const char *const k_no[] = {"normal", "approach", "fast_pass", "stop_and_go",
                                       "reverse_wall", "narrow_lane", "overtake_left", "overtake_right"};
    for (size_t i = 0; i < sizeof(k_no) / sizeof(k_no[0]); i++) {
        CHECK(scenario_crossing(k_no[i], 500, 1) == false, k_no[i]);
        CHECK(scenario_crossing(k_no[i], 100, 5) == false, k_no[i]);
    }
}

int main(void)
{
    test_classify_bounds();
    test_worst_zone_skip_stale();
    test_crossing_delta();
    test_crossing_front_threshold();
    test_crossing_invalid_slots();
    test_crossing_window();
    test_crossing_hold();
    test_crossing_scenarios();

    if (s_fail != 0) {
        fprintf(stderr, "hazard_core_tests: %d/%d FAILED\n", s_fail, s_pass + s_fail);
        return 1;
    }
    printf("hazard_core_tests: %d checks PASSED\n", s_pass);
    return 0;
}
/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * test_vehicle_layout.c — Test suite cho vehicle_profile và vehicle_layout (T3.2 / T3.1).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "vehicle_profile.h"
#include "vehicle_layout.h"

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

static void test_profile_validate(void)
{
    const vehicle_profile_t *p = vehicle_profile_active();
    CHECK(p != NULL, "vehicle_profile_active != NULL");
    CHECK(vehicle_profile_validate(p) == true, "validate EX8 profile == true");

    CHECK(vehicle_profile_validate(NULL) == false, "validate NULL == false");

    vehicle_profile_t bad = *p;
    bad.length_mm = 0;
    CHECK(vehicle_profile_validate(&bad) == false, "validate length 0 == false");

    bad = *p;
    bad.cab_length_mm = bad.length_mm + 100;
    CHECK(vehicle_profile_validate(&bad) == false, "validate cab >= length == false");

    bad = *p;
    bad.sensors[0].angle_deg = 360;
    CHECK(vehicle_profile_validate(&bad) == false, "validate angle 360 == false");

    bad = *p;
    bad.sensors[0].x_mm = (int16_t)(p->width_mm / 2 + 150);
    CHECK(vehicle_profile_validate(&bad) == false, "validate sensor out of bbox == false");
}

static void test_layout_computation(void)
{
    const vehicle_profile_t *p = vehicle_profile_active();
    vehicle_layout_t L;

    /* (b) canvas 440x440, margin 50: thân xe nằm trong [margin, canvas - margin] */
    bool ok = vehicle_layout_compute(p, 440, 440, 50, &L);
    CHECK(ok == true, "compute layout for EX8");
    CHECK(L.body_x >= 50, "body_x >= margin (50)");
    CHECK(L.body_y >= 50, "body_y >= margin (50)");
    CHECK(L.body_x + L.body_w <= 440 - 50, "body right edge <= 440 - 50");
    CHECK(L.body_y + L.body_h <= 440 - 50, "body bottom edge <= 440 - 50");

    /* (c) aspect ratio chênh lệch <= 2% */
    float aspect_body = (float)L.body_w / (float)L.body_h;
    float aspect_prof = (float)p->width_mm / (float)p->length_mm;
    float aspect_diff = fabsf(aspect_body - aspect_prof) / aspect_prof;
    CHECK(aspect_diff <= 0.02f, "aspect ratio diff <= 2%");

    /* (d) profile giả width/length khác -> body_w/body_h đổi theo */
    vehicle_profile_t mock = *p;
    mock.width_mm = 3000;
    mock.length_mm = 6000;
    mock.cab_length_mm = 1500;
    mock.sensors[ESPNOW_SLOT_FRONT].y_mm = 3000;
    mock.sensors[ESPNOW_SLOT_REAR].y_mm = -3000;
    mock.sensors[ESPNOW_SLOT_LEFT_FRONT].x_mm = -1500;
    mock.sensors[ESPNOW_SLOT_LEFT_REAR].x_mm = -1500;
    mock.sensors[ESPNOW_SLOT_RIGHT_FRONT].x_mm = 1500;
    mock.sensors[ESPNOW_SLOT_RIGHT_REAR].x_mm = 1500;

    vehicle_layout_t L_mock;
    CHECK(vehicle_layout_compute(&mock, 440, 440, 50, &L_mock) == true, "compute mock layout");
    CHECK(L_mock.body_w != L.body_w || L_mock.body_h != L.body_h, "mock layout dims different from EX8");
    float aspect_mock_body = (float)L_mock.body_w / (float)L_mock.body_h;
    float aspect_mock_prof = (float)mock.width_mm / (float)mock.length_mm;
    CHECK(fabsf(aspect_mock_body - aspect_mock_prof) / aspect_mock_prof <= 0.02f, "mock aspect ratio diff <= 2%");

    /* (e) mọi sensor_px nằm trong canvas [0, 440] */
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        CHECK(L.sensor_px[i].x >= 0 && L.sensor_px[i].x <= 440, "sensor_px x within canvas");
        CHECK(L.sensor_px[i].y >= 0 && L.sensor_px[i].y <= 440, "sensor_px y within canvas");
    }

    /* (h) compute false khi canvas quá nhỏ */
    vehicle_layout_t L_small;
    CHECK(vehicle_layout_compute(p, 80, 80, 50, &L_small) == false, "compute false when canvas <= 2*margin");
    CHECK(vehicle_layout_compute(NULL, 440, 440, 50, &L_small) == false, "compute false when p is NULL");
}

static void test_marker_math(void)
{
    const vehicle_profile_t *p = vehicle_profile_active();
    vehicle_layout_t L;
    vehicle_layout_compute(p, 440, 440, 50, &L);

    /* (f) marker FRONT dist 50 cm: x không đổi (±1), y nhỏ hơn sensor_px.y đúng 500mm*scale (±1px) */
    vl_point_t pt_front;
    CHECK(vehicle_layout_marker(&L, ESPNOW_SLOT_FRONT, 50, &pt_front) == true, "marker FRONT 50cm");
    CHECK(abs(pt_front.x - L.sensor_px[ESPNOW_SLOT_FRONT].x) <= 1, "FRONT marker x unchanged (+-1)");
    int expected_dy = (int)roundf(500.0f * (float)L.scale_num / (float)L.scale_den);
    int actual_dy = (int)L.sensor_px[ESPNOW_SLOT_FRONT].y - (int)pt_front.y;
    CHECK(abs(actual_dy - expected_dy) <= 1, "FRONT marker y is smaller by 500mm*scale (+-1)");

    /* (g) marker LEFT_FRONT: x nhỏ hơn sensor x (sang trái) */
    vl_point_t pt_lf;
    CHECK(vehicle_layout_marker(&L, ESPNOW_SLOT_LEFT_FRONT, 50, &pt_lf) == true, "marker LEFT_FRONT 50cm");
    CHECK(pt_lf.x < L.sensor_px[ESPNOW_SLOT_LEFT_FRONT].x, "LEFT_FRONT marker x < sensor x (left)");

    /* (g) marker REAR: y lớn hơn (hướng xuống đuôi xe) */
    vl_point_t pt_rear;
    CHECK(vehicle_layout_marker(&L, ESPNOW_SLOT_REAR, 50, &pt_rear) == true, "marker REAR 50cm");
    CHECK(pt_rear.y > L.sensor_px[ESPNOW_SLOT_REAR].y, "REAR marker y > sensor y (down/back)");

    /* Invalid parameters */
    vl_point_t dummy;
    CHECK(vehicle_layout_marker(NULL, ESPNOW_SLOT_FRONT, 50, &dummy) == false, "NULL layout -> false");
    CHECK(vehicle_layout_marker(&L, (espnow_slot_t)99, 50, &dummy) == false, "slot >= COUNT -> false");
    CHECK(vehicle_layout_marker(&L, ESPNOW_SLOT_FRONT, 50, NULL) == false, "NULL out -> false");
}

int main(void)
{
    printf("=== Running vehicle_layout_tests ===\n");
    test_profile_validate();
    test_layout_computation();
    test_marker_math();

    printf("Result: %d passed, %d failed\n", s_pass, s_fail);
    return (s_fail == 0) ? 0 : 1;
}

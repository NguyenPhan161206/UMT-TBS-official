/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * test_vehicle_override.c — Test suite cho ghi đè từng cảm biến của vehicle_settings (T4.1c).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "vehicle_profile.h"
#include "vehicle_settings.h"
#include "fake_vehicle_store.h"

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

static bool pose_eq(const vehicle_sensor_pose_t *a, const vehicle_sensor_pose_t *b)
{
    return a->x_mm == b->x_mm && a->y_mm == b->y_mm && a->angle_deg == b->angle_deg;
}

/* T4.1c: chỉnh tay từng cảm biến, ghi đè hồ sơ gốc */
static void test_override(void)
{
    vehicle_settings_t st;
    const vehicle_profile_t *base2 = vehicle_profile_find(2);

    fake_reset();
    vehicle_settings_init(&k_fake_ops);
    CHECK(vehicle_settings_select(2) == true, "select(2) before overrides");
    CHECK(vehicle_settings_override_mask() == 0, "no override after select");
    int saves = g_save_n;

    /* 1. override hợp lệ: chỉ slot đó đổi, lưu xuống store */
    vehicle_sensor_pose_t front = { 100, 2700, 260 };
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_FRONT, &front) == true, "set_override FRONT");
    CHECK(pose_eq(&vehicle_profile_active()->sensors[ESPNOW_SLOT_FRONT], &front), "active FRONT == override pose");
    int others_ok = 1;
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        if (i != ESPNOW_SLOT_FRONT && !pose_eq(&vehicle_profile_active()->sensors[i], &base2->sensors[i])) {
            others_ok = 0;
        }
    }
    CHECK(others_ok, "other slots keep the base pose");
    CHECK(vehicle_settings_override_mask() == (1u << ESPNOW_SLOT_FRONT), "mask has only FRONT");
    CHECK(vehicle_profile_active()->id == 2, "override keeps the base profile id");
    CHECK(g_save_n == saves + 1, "override saved once");
    CHECK(vehicle_settings_decode(g_store, g_store_len, &st) && st.selected_id == 2 &&
          st.override_mask == (1u << ESPNOW_SLOT_FRONT) && pose_eq(&st.override_pose[ESPNOW_SLOT_FRONT], &front),
          "stored blob carries the override");

    /* 2. khởi động lại: ghi đè còn nguyên */
    vehicle_profile_set_active(vehicle_profile_get(0));
    CHECK(vehicle_settings_init(&k_fake_ops) == VEHICLE_SETTINGS_INIT_LOADED, "re-init loads overrides");
    CHECK(vehicle_profile_active()->id == 2 && pose_eq(&vehicle_profile_active()->sensors[ESPNOW_SLOT_FRONT], &front),
          "override survives re-init");
    CHECK(vehicle_settings_override_mask() == (1u << ESPNOW_SLOT_FRONT), "mask survives re-init");

    /* 3. pose sai bị từ chối, không đổi gì và không ghi */
    saves = g_save_n;
    vehicle_profile_t before = *vehicle_profile_active();
    uint8_t mask_before = vehicle_settings_override_mask();
    vehicle_sensor_pose_t bad_x = { 5000, 0, 0 };
    vehicle_sensor_pose_t bad_ang = { 0, 0, 360 };
    vehicle_sensor_pose_t bad_neg = { 0, 0, -1 };
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_REAR, &bad_x) == false, "reject x out of bbox");
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_REAR, &bad_ang) == false, "reject angle 360");
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_REAR, &bad_neg) == false, "reject negative angle");
    CHECK(vehicle_settings_set_override((espnow_slot_t)ESPNOW_SENSOR_SLOT_COUNT, &front) == false, "reject slot >= COUNT");
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_REAR, NULL) == false, "reject NULL pose");
    CHECK(memcmp(&before, vehicle_profile_active(), sizeof(before)) == 0 &&
          vehicle_settings_override_mask() == mask_before && g_save_n == saves,
          "rejected overrides change nothing and write nothing");

    /* 4. thêm override thứ hai, rồi bỏ riêng cái đầu: khôi phục đúng toạ độ gốc */
    vehicle_sensor_pose_t lrear = { -1000, -1300, 200 };
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_LEFT_REAR, &lrear) == true, "second override LEFT_REAR");
    CHECK(vehicle_settings_override_mask() == ((1u << ESPNOW_SLOT_FRONT) | (1u << ESPNOW_SLOT_LEFT_REAR)),
          "mask has FRONT and LEFT_REAR");
    CHECK(vehicle_settings_clear_override(ESPNOW_SLOT_FRONT) == true, "clear FRONT override");
    CHECK(pose_eq(&vehicle_profile_active()->sensors[ESPNOW_SLOT_FRONT], &base2->sensors[ESPNOW_SLOT_FRONT]),
          "FRONT restored to the base pose exactly");
    CHECK(pose_eq(&vehicle_profile_active()->sensors[ESPNOW_SLOT_LEFT_REAR], &lrear), "LEFT_REAR override kept");
    CHECK(vehicle_settings_override_mask() == (1u << ESPNOW_SLOT_LEFT_REAR), "mask has only LEFT_REAR");
    CHECK(vehicle_settings_clear_override((espnow_slot_t)ESPNOW_SENSOR_SLOT_COUNT) == false, "clear slot >= COUNT -> false");

    /* 5. đổi hồ sơ gốc xoá mọi override */
    CHECK(vehicle_settings_select(3) == true, "select(3) with overrides present");
    CHECK(vehicle_settings_override_mask() == 0, "select resets all overrides");
    CHECK(memcmp(vehicle_profile_active()->sensors, vehicle_profile_find(3)->sensors,
                 sizeof(vehicle_profile_active()->sensors)) == 0, "profile 3 sensors are the base ones");
    CHECK(vehicle_settings_decode(g_store, g_store_len, &st) && st.selected_id == 3 && st.override_mask == 0,
          "stored blob follows select: id 3, no override");

    /* 6. clear_all */
    vehicle_sensor_pose_t a = { 0, 5900, 270 }, b = { 1200, 4400, 10 };
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_FRONT, &a) && vehicle_settings_set_override(ESPNOW_SLOT_RIGHT_FRONT, &b),
          "two overrides on profile 3");
    CHECK(vehicle_settings_clear_all_overrides() == true && vehicle_settings_override_mask() == 0, "clear_all -> mask 0");
    CHECK(memcmp(vehicle_profile_active()->sensors, vehicle_profile_find(3)->sensors,
                 sizeof(vehicle_profile_active()->sensors)) == 0, "clear_all restores every base pose");

    /* 7. lỗi ghi: vẫn áp dụng trong RAM, báo last_save_ok == false */
    g_save_fail = true;
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_FRONT, &a) == true, "override applies even if save fails");
    CHECK(pose_eq(&vehicle_profile_active()->sensors[ESPNOW_SLOT_FRONT], &a), "active FRONT == override despite save failure");
    CHECK(vehicle_settings_last_save_ok() == false, "last_save_ok == false after failed override save");
    g_save_fail = false;

    /* 8. không có store: override vẫn chạy */
    vehicle_settings_init(NULL);
    vehicle_settings_select(2);
    CHECK(vehicle_settings_set_override(ESPNOW_SLOT_FRONT, &front) == true &&
          pose_eq(&vehicle_profile_active()->sensors[ESPNOW_SLOT_FRONT], &front), "override works without store");

    vehicle_settings_init(NULL);                               /* trả về mặc định */
    CHECK(vehicle_profile_active()->id == 1 && vehicle_settings_override_mask() == 0, "restore default");
}

int main(void)
{
    printf("=== Running vehicle_override_tests ===\n");
    test_override();

    printf("Result: %d passed, %d failed\n", s_pass, s_fail);
    return (s_fail == 0) ? 0 : 1;
}

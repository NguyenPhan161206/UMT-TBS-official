/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * test_vehicle_settings.c — Test suite cho vehicle_settings (codec blob, resolve, runtime) — T4.1b.
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

#define BLOB_N VEHICLE_SETTINGS_BLOB_SIZE

static void fix_crc(uint8_t *b)
{
    uint16_t c = vehicle_settings_crc16(b, BLOB_N - 2);
    b[BLOB_N - 2] = (uint8_t)(c & 0xFF);
    b[BLOB_N - 1] = (uint8_t)(c >> 8);
}

static bool settings_equal(const vehicle_settings_t *a, const vehicle_settings_t *b)
{
    if (a->selected_id != b->selected_id || a->override_mask != b->override_mask) {
        return false;
    }
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        if (a->override_pose[i].x_mm != b->override_pose[i].x_mm ||
            a->override_pose[i].y_mm != b->override_pose[i].y_mm ||
            a->override_pose[i].angle_deg != b->override_pose[i].angle_deg) {
            return false;
        }
    }
    return true;
}

static void make_sample(vehicle_settings_t *s)
{
    vehicle_settings_defaults(s);
    s->selected_id = 2;
    s->override_mask = 0x29;                                   /* slot 0, 3, 5 */
    s->override_pose[0].x_mm = -120;  s->override_pose[0].y_mm = 2700;  s->override_pose[0].angle_deg = 265;
    s->override_pose[3].x_mm = -1000; s->override_pose[3].y_mm = -1300; s->override_pose[3].angle_deg = 359;
    s->override_pose[5].x_mm = 1049;  s->override_pose[5].y_mm = -1250; s->override_pose[5].angle_deg = 0;
}

static void test_crc_and_defaults(void)
{
    /* vector chuẩn CRC-16/CCITT-FALSE */
    CHECK(vehicle_settings_crc16((const uint8_t *)"123456789", 9) == 0x29B1, "crc16 of '123456789' == 0x29B1");

    vehicle_settings_t d;
    vehicle_settings_defaults(&d);
    CHECK(d.selected_id == vehicle_profile_get(0)->id, "defaults select the first registry profile");
    CHECK(d.override_mask == 0, "defaults have no override");
}

static void test_roundtrip(void)
{
    uint8_t buf[VEHICLE_SETTINGS_BLOB_MAX];
    vehicle_settings_t in, out;

    vehicle_settings_defaults(&in);
    CHECK(vehicle_settings_encode(&in, buf, sizeof(buf)) == BLOB_N, "encode defaults -> BLOB_SIZE");
    CHECK(vehicle_settings_decode(buf, BLOB_N, &out) == true, "decode defaults");
    CHECK(settings_equal(&in, &out), "roundtrip defaults equal");

    make_sample(&in);
    CHECK(vehicle_settings_encode(&in, buf, sizeof(buf)) == BLOB_N, "encode sample");
    CHECK(vehicle_settings_decode(buf, BLOB_N, &out) == true, "decode sample");
    CHECK(settings_equal(&in, &out), "roundtrip sample (negative x/y, angle 359) equal");
}

static void test_corruption(void)
{
    uint8_t good[VEHICLE_SETTINGS_BLOB_MAX];
    vehicle_settings_t in, out;
    make_sample(&in);
    CHECK(vehicle_settings_encode(&in, good, sizeof(good)) == BLOB_N, "encode for corruption tests");

    /* lật từng byte một → CRC (hoặc magic/version) phải bắt được */
    for (int i = 0; i < BLOB_N; i++) {
        uint8_t b[VEHICLE_SETTINGS_BLOB_MAX];
        memcpy(b, good, BLOB_N);
        b[i] ^= 0x01;
        CHECK(vehicle_settings_decode(b, BLOB_N, &out) == false, "decode rejects any single flipped byte");
    }

    CHECK(vehicle_settings_decode(good, BLOB_N - 1, &out) == false, "decode rejects len 42");
    CHECK(vehicle_settings_decode(good, BLOB_N + 1, &out) == false, "decode rejects len 44");
    CHECK(vehicle_settings_decode(good, 0, &out) == false, "decode rejects len 0");
    CHECK(vehicle_settings_decode(NULL, BLOB_N, &out) == false, "decode rejects NULL buf");
    CHECK(vehicle_settings_decode(good, BLOB_N, NULL) == false, "decode rejects NULL out");

    uint8_t b[VEHICLE_SETTINGS_BLOB_MAX];

    memcpy(b, good, BLOB_N);
    b[2] = 2;                                                   /* version lạ, CRC được tính lại */
    fix_crc(b);
    CHECK(vehicle_settings_decode(b, BLOB_N, &out) == false, "decode rejects unknown version (valid CRC)");

    memcpy(b, good, BLOB_N);
    b[0] ^= 0xFF;                                               /* magic sai, CRC được tính lại */
    fix_crc(b);
    CHECK(vehicle_settings_decode(b, BLOB_N, &out) == false, "decode rejects wrong magic (valid CRC)");

    memcpy(b, good, BLOB_N);
    b[4] = 0x40;                                                /* bit 6 >= SLOT_COUNT */
    fix_crc(b);
    CHECK(vehicle_settings_decode(b, BLOB_N, &out) == false, "decode rejects mask bit 6 (valid CRC)");

    memcpy(b, good, BLOB_N);
    b[4] = 0x80;
    fix_crc(b);
    CHECK(vehicle_settings_decode(b, BLOB_N, &out) == false, "decode rejects mask bit 7 (valid CRC)");

    /* decode thất bại không được ghi vào out */
    vehicle_settings_t sentinel;
    vehicle_settings_defaults(&sentinel);
    sentinel.selected_id = 77;
    vehicle_settings_t probe = sentinel;
    memcpy(b, good, BLOB_N);
    b[10] ^= 0x55;
    CHECK(vehicle_settings_decode(b, BLOB_N, &probe) == false && settings_equal(&probe, &sentinel),
          "failed decode leaves out untouched");
}

static void test_encode_limits(void)
{
    uint8_t buf[VEHICLE_SETTINGS_BLOB_MAX];
    vehicle_settings_t s;
    make_sample(&s);

    CHECK(vehicle_settings_encode(&s, buf, BLOB_N - 1) == 0, "encode cap 42 -> 0");
    CHECK(vehicle_settings_encode(&s, buf, BLOB_N) == BLOB_N, "encode cap 43 -> 43");
    CHECK(vehicle_settings_encode(NULL, buf, sizeof(buf)) == 0, "encode NULL settings -> 0");
    CHECK(vehicle_settings_encode(&s, NULL, sizeof(buf)) == 0, "encode NULL buf -> 0");

    s.override_mask = 0x40;
    CHECK(vehicle_settings_encode(&s, buf, sizeof(buf)) == 0, "encode mask bit 6 -> 0");
}

static void test_resolve(void)
{
    vehicle_settings_t s;
    vehicle_profile_t out;
    const vehicle_profile_t *base2 = vehicle_profile_find(2);

    /* không override: giống hồ sơ gốc */
    vehicle_settings_defaults(&s);
    s.selected_id = 2;
    CHECK(vehicle_settings_resolve(&s, &out) == true, "resolve id 2 without override");
    CHECK(out.id == 2 && out.name == base2->name && out.length_mm == base2->length_mm &&
          out.wheelbase_mm == base2->wheelbase_mm, "resolved profile copies the base profile");
    CHECK(memcmp(out.sensors, base2->sensors, sizeof(out.sensors)) == 0, "resolved sensors == base sensors");

    /* override slot FRONT: chỉ slot đó đổi */
    s.override_mask = 1u << ESPNOW_SLOT_FRONT;
    s.override_pose[ESPNOW_SLOT_FRONT].x_mm = 100;
    s.override_pose[ESPNOW_SLOT_FRONT].y_mm = 2700;
    s.override_pose[ESPNOW_SLOT_FRONT].angle_deg = 260;
    CHECK(vehicle_settings_resolve(&s, &out) == true, "resolve id 2 + FRONT override");
    CHECK(out.sensors[ESPNOW_SLOT_FRONT].x_mm == 100 && out.sensors[ESPNOW_SLOT_FRONT].y_mm == 2700 &&
          out.sensors[ESPNOW_SLOT_FRONT].angle_deg == 260, "overridden slot takes the override pose");
    int others_equal = 1;
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        if (i == ESPNOW_SLOT_FRONT) {
            continue;
        }
        if (memcmp(&out.sensors[i], &base2->sensors[i], sizeof(out.sensors[i])) != 0) {
            others_equal = 0;
        }
    }
    CHECK(others_equal, "non-overridden slots keep the base pose");

    /* lỗi: không ghi vào out */
    vehicle_profile_t keep = out;
    s.selected_id = 99;
    CHECK(vehicle_settings_resolve(&s, &out) == false, "resolve unknown id -> false");
    CHECK(memcmp(&keep, &out, sizeof(out)) == 0, "failed resolve leaves out untouched");

    s.selected_id = 0;
    CHECK(vehicle_settings_resolve(&s, &out) == false, "resolve id 0 -> false");

    s.selected_id = 2;
    s.override_pose[ESPNOW_SLOT_FRONT].x_mm = 5000;             /* ngoài bbox xe */
    CHECK(vehicle_settings_resolve(&s, &out) == false, "resolve override out of bbox -> false");

    s.override_pose[ESPNOW_SLOT_FRONT].x_mm = 0;
    s.override_pose[ESPNOW_SLOT_FRONT].angle_deg = 360;
    CHECK(vehicle_settings_resolve(&s, &out) == false, "resolve override angle 360 -> false");

    s.override_pose[ESPNOW_SLOT_FRONT].angle_deg = 270;
    s.override_mask = 0x80;
    CHECK(vehicle_settings_resolve(&s, &out) == false, "resolve mask bit 7 -> false");

    CHECK(vehicle_settings_resolve(NULL, &out) == false, "resolve NULL settings -> false");
    CHECK(vehicle_settings_resolve(&s, NULL) == false, "resolve NULL out -> false");
}

static void test_runtime(void)
{
    vehicle_settings_t st;

    /* 1. store rỗng → mặc định EX8, không ghi gì */
    fake_reset();
    CHECK(vehicle_settings_init(&k_fake_ops) == VEHICLE_SETTINGS_INIT_DEFAULTS_NO_DATA, "init empty store -> NO_DATA");
    CHECK(vehicle_profile_active()->id == 1, "default active profile is EX8 (id 1)");
    CHECK(vehicle_settings_current()->selected_id == 1 && vehicle_settings_current()->override_mask == 0,
          "default settings: id 1, no override");
    CHECK(g_save_n == 0 && g_erase_n == 0, "init does not write or erase");

    /* 2. select(2) → áp dụng + lưu blob decode ra đúng */
    CHECK(vehicle_settings_select(2) == true, "select(2)");
    CHECK(vehicle_profile_active()->id == 2, "active id == 2 after select(2)");
    CHECK(g_save_n == 1 && g_has, "select saved exactly once");
    CHECK(vehicle_settings_decode(g_store, g_store_len, &st) && st.selected_id == 2 && st.override_mask == 0,
          "stored blob decodes to selected_id 2, no override");
    CHECK(vehicle_settings_last_save_ok() == true, "last_save_ok after good save");

    /* 3. khởi động lại: RAM về EX8 rồi init nạp lại hồ sơ 2 */
    CHECK(vehicle_profile_set_active(vehicle_profile_get(0)) == true, "simulate reboot: RAM back to EX8");
    CHECK(vehicle_profile_active()->id == 1, "RAM active is EX8 before init");
    CHECK(vehicle_settings_init(&k_fake_ops) == VEHICLE_SETTINGS_INIT_LOADED, "re-init -> LOADED");
    CHECK(vehicle_profile_active()->id == 2 && vehicle_settings_current()->selected_id == 2,
          "selection survives re-init");

    /* 4. blob hỏng → mặc định, KHÔNG xoá store */
    g_store[10] ^= 0x5A;
    vehicle_profile_set_active(vehicle_profile_get(1));
    CHECK(vehicle_settings_init(&k_fake_ops) == VEHICLE_SETTINGS_INIT_DEFAULTS_INVALID, "corrupt blob -> INVALID");
    CHECK(vehicle_profile_active()->id == 1, "corrupt blob falls back to EX8");
    CHECK(g_erase_n == 0 && g_has, "corrupt blob is NOT erased");

    /* 5. id lạ nhưng CRC đúng → INVALID */
    vehicle_settings_t unk;
    vehicle_settings_defaults(&unk);
    unk.selected_id = 99;
    g_store_len = vehicle_settings_encode(&unk, g_store, sizeof(g_store));
    g_has = true;
    CHECK(vehicle_settings_init(&k_fake_ops) == VEHICLE_SETTINGS_INIT_DEFAULTS_INVALID, "unknown id in blob -> INVALID");
    CHECK(vehicle_profile_active()->id == 1 && g_erase_n == 0, "unknown id falls back to EX8, no erase");

    /* 6. select id lạ → false, không ghi, không đổi */
    int saves = g_save_n;
    CHECK(vehicle_settings_select(99) == false, "select(99) == false");
    CHECK(vehicle_settings_select(0) == false, "select(0) == false");
    CHECK(g_save_n == saves && vehicle_profile_active()->id == 1, "rejected select writes nothing, active unchanged");

    /* 7. lỗi đọc → NO_DATA */
    g_load_fail = true;
    CHECK(vehicle_settings_init(&k_fake_ops) == VEHICLE_SETTINGS_INIT_DEFAULTS_NO_DATA, "load failure -> NO_DATA");
    g_load_fail = false;

    /* 8. lỗi ghi: vẫn áp dụng trong RAM nhưng báo last_save_ok == false */
    fake_reset();
    vehicle_settings_init(&k_fake_ops);
    g_save_fail = true;
    CHECK(vehicle_settings_select(3) == true, "select(3) applies even if save fails");
    CHECK(vehicle_profile_active()->id == 3, "active id == 3 although save failed");
    CHECK(vehicle_settings_last_save_ok() == false, "last_save_ok == false after failed save");
    g_save_fail = false;
    CHECK(vehicle_settings_select(1) == true && vehicle_settings_last_save_ok() == true, "last_save_ok recovers after good save");
    CHECK(vehicle_settings_decode(g_store, g_store_len, &st) && st.selected_id == 1, "stored blob follows latest select");

    /* 9. ops NULL: vẫn chạy, không đụng store */
    int saves2 = g_save_n;
    CHECK(vehicle_settings_init(NULL) == VEHICLE_SETTINGS_INIT_DEFAULTS_NO_DATA, "init(NULL) -> NO_DATA");
    CHECK(vehicle_settings_select(3) == true && vehicle_profile_active()->id == 3, "select works without store");
    CHECK(vehicle_settings_last_save_ok() == true && g_save_n == saves2, "no store: nothing written, save_ok stays true");

    /* trả về mặc định để không ảnh hưởng test khác */
    vehicle_settings_init(NULL);
    CHECK(vehicle_profile_active()->id == 1, "restore default");
}

int main(void)
{
    printf("=== Running vehicle_settings_tests ===\n");
    test_crc_and_defaults();
    test_roundtrip();
    test_corruption();
    test_encode_limits();
    test_resolve();
    test_runtime();

    printf("Result: %d passed, %d failed\n", s_pass, s_fail);
    return (s_fail == 0) ? 0 : 1;
}

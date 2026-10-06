/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_settings.c — Mã hoá/giải mã blob cài đặt hồ sơ xe và dựng hồ sơ hiệu lực.
 *
 * Blob version 1 (43 byte, little-endian ghi từng byte — KHÔNG memcpy struct):
 *   [0..1]  magic 0x5648
 *   [2]     version = 1
 *   [3]     selected_id
 *   [4]     override_mask
 *   [5..40] 6 × { x_mm, y_mm, angle_deg } (mỗi trường int16)
 *   [41..42] CRC16-CCITT trên 41 byte đầu
 */

#include "vehicle_settings.h"
#include <stddef.h>

#define BLOB_MAGIC      0x5648u
#define BLOB_VERSION    1u
#define BLOB_POSE_OFF   5
#define BLOB_POSE_BYTES 6                                   /* 3 × int16 */
#define BLOB_CRC_OFF    (BLOB_POSE_OFF + ESPNOW_SENSOR_SLOT_COUNT * BLOB_POSE_BYTES)
#define SLOT_MASK_ALL   ((1u << ESPNOW_SENSOR_SLOT_COUNT) - 1u)

_Static_assert(BLOB_CRC_OFF + 2 == VEHICLE_SETTINGS_BLOB_SIZE, "blob size mismatch");
_Static_assert(VEHICLE_SETTINGS_BLOB_SIZE <= VEHICLE_SETTINGS_BLOB_MAX, "blob exceeds VEHICLE_SETTINGS_BLOB_MAX");

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}

static uint16_t get16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

void vehicle_settings_defaults(vehicle_settings_t *s)
{
    if (s == NULL) {
        return;
    }
    const vehicle_profile_t *first = vehicle_profile_get(0);
    s->selected_id = (first != NULL) ? first->id : 0;
    s->override_mask = 0;
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        s->override_pose[i].x_mm = 0;
        s->override_pose[i].y_mm = 0;
        s->override_pose[i].angle_deg = 0;
    }
}

uint16_t vehicle_settings_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; data != NULL && i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

size_t vehicle_settings_encode(const vehicle_settings_t *s, uint8_t *buf, size_t cap)
{
    if (s == NULL || buf == NULL || cap < VEHICLE_SETTINGS_BLOB_SIZE) {
        return 0;
    }
    if ((s->override_mask & ~SLOT_MASK_ALL) != 0) {
        return 0;
    }

    put16(&buf[0], BLOB_MAGIC);
    buf[2] = (uint8_t)BLOB_VERSION;
    buf[3] = s->selected_id;
    buf[4] = s->override_mask;
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        uint8_t *q = &buf[BLOB_POSE_OFF + i * BLOB_POSE_BYTES];
        put16(&q[0], (uint16_t)s->override_pose[i].x_mm);
        put16(&q[2], (uint16_t)s->override_pose[i].y_mm);
        put16(&q[4], (uint16_t)s->override_pose[i].angle_deg);
    }
    put16(&buf[BLOB_CRC_OFF], vehicle_settings_crc16(buf, BLOB_CRC_OFF));
    return VEHICLE_SETTINGS_BLOB_SIZE;
}

bool vehicle_settings_decode(const uint8_t *buf, size_t len, vehicle_settings_t *out)
{
    if (buf == NULL || out == NULL || len != VEHICLE_SETTINGS_BLOB_SIZE) {
        return false;
    }
    if (get16(&buf[0]) != BLOB_MAGIC || buf[2] != BLOB_VERSION) {
        return false;
    }
    if (get16(&buf[BLOB_CRC_OFF]) != vehicle_settings_crc16(buf, BLOB_CRC_OFF)) {
        return false;
    }
    if ((buf[4] & ~SLOT_MASK_ALL) != 0) {
        return false;
    }

    vehicle_settings_t tmp;
    tmp.selected_id = buf[3];
    tmp.override_mask = buf[4];
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        const uint8_t *q = &buf[BLOB_POSE_OFF + i * BLOB_POSE_BYTES];
        tmp.override_pose[i].x_mm = (int16_t)get16(&q[0]);
        tmp.override_pose[i].y_mm = (int16_t)get16(&q[2]);
        tmp.override_pose[i].angle_deg = (int16_t)get16(&q[4]);
    }
    *out = tmp;
    return true;
}

/* ---- Runtime ------------------------------------------------------------------------------------ */

static vehicle_settings_t s_cur;
static const vehicle_store_ops_t *s_ops = NULL;
static bool s_last_save_ok = true;

/* Ghi s_cur xuống store (nếu có). Chỉ cập nhật cờ s_last_save_ok. */
static void persist(void)
{
    s_last_save_ok = true;
    if (s_ops == NULL || s_ops->save == NULL) {
        return;
    }
    uint8_t buf[VEHICLE_SETTINGS_BLOB_MAX];
    size_t n = vehicle_settings_encode(&s_cur, buf, sizeof(buf));
    s_last_save_ok = (n != 0) && s_ops->save(buf, n);
}

/* resolve + set_active + lưu; chỉ đổi s_cur khi cả hai bước đầu thành công. */
static bool commit(const vehicle_settings_t *next)
{
    vehicle_profile_t eff;
    if (!vehicle_settings_resolve(next, &eff) || !vehicle_profile_set_active(&eff)) {
        return false;
    }
    s_cur = *next;
    persist();
    return true;
}

vehicle_settings_init_result_t vehicle_settings_init(const vehicle_store_ops_t *ops)
{
    s_ops = ops;
    s_last_save_ok = true;

    vehicle_settings_init_result_t result = VEHICLE_SETTINGS_INIT_DEFAULTS_NO_DATA;
    vehicle_settings_t st;
    vehicle_profile_t eff;

    if (ops != NULL && ops->load != NULL) {
        uint8_t buf[VEHICLE_SETTINGS_BLOB_MAX];
        size_t n = 0;
        if (ops->load(buf, sizeof(buf), &n)) {
            if (vehicle_settings_decode(buf, n, &st) && vehicle_settings_resolve(&st, &eff)) {
                result = VEHICLE_SETTINGS_INIT_LOADED;
            } else {
                result = VEHICLE_SETTINGS_INIT_DEFAULTS_INVALID;
            }
        }
    }

    if (result != VEHICLE_SETTINGS_INIT_LOADED) {
        vehicle_settings_defaults(&st);
        if (!vehicle_settings_resolve(&st, &eff)) {
            /* Không xảy ra với registry hợp lệ; vẫn giữ hồ sơ đầu tiên cho chắc. */
            const vehicle_profile_t *first = vehicle_profile_get(0);
            if (first != NULL) {
                eff = *first;
            }
        }
    }

    if (vehicle_profile_set_active(&eff)) {
        s_cur = st;
    } else {
        vehicle_settings_defaults(&s_cur);
    }
    return result;
}

const vehicle_settings_t *vehicle_settings_current(void)
{
    return &s_cur;
}

bool vehicle_settings_select(uint8_t id)
{
    if (vehicle_profile_find(id) == NULL) {
        return false;
    }
    vehicle_settings_t next;
    vehicle_settings_defaults(&next);
    next.selected_id = id;                       /* override_mask = 0: đổi hồ sơ gốc thì xoá hết override */
    return commit(&next);
}

bool vehicle_settings_last_save_ok(void)
{
    return s_last_save_ok;
}

bool vehicle_settings_set_override(espnow_slot_t slot, const vehicle_sensor_pose_t *pose)
{
    if ((unsigned)slot >= ESPNOW_SENSOR_SLOT_COUNT || pose == NULL) {
        return false;
    }
    vehicle_settings_t next = s_cur;
    next.override_mask = (uint8_t)(next.override_mask | (1u << slot));
    next.override_pose[slot] = *pose;
    return commit(&next);                        /* commit validate hồ sơ hiệu lực, sai thì không đổi gì */
}

bool vehicle_settings_clear_override(espnow_slot_t slot)
{
    if ((unsigned)slot >= ESPNOW_SENSOR_SLOT_COUNT) {
        return false;
    }
    vehicle_settings_t next = s_cur;
    next.override_mask = (uint8_t)(next.override_mask & ~(1u << slot));
    next.override_pose[slot].x_mm = 0;
    next.override_pose[slot].y_mm = 0;
    next.override_pose[slot].angle_deg = 0;
    return commit(&next);
}

bool vehicle_settings_clear_all_overrides(void)
{
    vehicle_settings_t next;
    vehicle_settings_defaults(&next);
    next.selected_id = s_cur.selected_id;
    return commit(&next);
}

uint8_t vehicle_settings_override_mask(void)
{
    return s_cur.override_mask;
}

bool vehicle_settings_resolve(const vehicle_settings_t *s, vehicle_profile_t *out)
{
    if (s == NULL || out == NULL) {
        return false;
    }
    if ((s->override_mask & ~SLOT_MASK_ALL) != 0) {
        return false;
    }
    const vehicle_profile_t *base = vehicle_profile_find(s->selected_id);
    if (base == NULL) {
        return false;
    }

    vehicle_profile_t eff = *base;
    for (int i = 0; i < ESPNOW_SENSOR_SLOT_COUNT; i++) {
        if (s->override_mask & (1u << i)) {
            eff.sensors[i] = s->override_pose[i];
        }
    }
    if (!vehicle_profile_validate(&eff)) {
        return false;
    }
    *out = eff;
    return true;
}

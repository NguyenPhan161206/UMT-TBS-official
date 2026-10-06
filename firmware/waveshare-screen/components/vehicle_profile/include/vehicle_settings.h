/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_settings.h — Hồ sơ xe đang chọn + ghi đè từng cảm biến, và blob lưu trữ (NVS).
 * C thuần: không phụ thuộc ESP-IDF/LVGL. Việc lưu thật nằm ở component vehicle_store (qua vehicle_store_ops_t).
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "vehicle_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VEHICLE_SETTINGS_BLOB_MAX   64     /* chặn trên cho bộ đệm của người gọi */
#define VEHICLE_SETTINGS_BLOB_SIZE  43     /* kích thước blob version 1 (xem vehicle_settings.c) */

typedef struct {
    uint8_t selected_id;                   /* id hồ sơ gốc trong registry */
    uint8_t override_mask;                 /* bit i = cảm biến slot i bị ghi đè bằng override_pose[i] */
    vehicle_sensor_pose_t override_pose[ESPNOW_SENSOR_SLOT_COUNT];
} vehicle_settings_t;

/* selected_id = id của hồ sơ đầu tiên trong registry (EX8), không có override. */
void vehicle_settings_defaults(vehicle_settings_t *s);

/* CRC16-CCITT (poly 0x1021, init 0xFFFF) — phơi ra để test tự dựng blob. */
uint16_t vehicle_settings_crc16(const uint8_t *data, size_t len);

/* Ghi blob version 1 (little-endian từng byte). Trả số byte (VEHICLE_SETTINGS_BLOB_SIZE), 0 nếu
 * s/buf NULL, cap quá nhỏ, hoặc override_mask có bit >= ESPNOW_SENSOR_SLOT_COUNT. */
size_t vehicle_settings_encode(const vehicle_settings_t *s, uint8_t *buf, size_t cap);

/* false nếu len sai, magic/version sai, CRC sai hoặc mask có bit lạ; *out không bị ghi khi false.
 * KHÔNG kiểm selected_id có trong registry hay không (việc của vehicle_settings_resolve). */
bool vehicle_settings_decode(const uint8_t *buf, size_t len, vehicle_settings_t *out);

/* Dựng hồ sơ hiệu lực = hồ sơ gốc (vehicle_profile_find(selected_id)) + override theo mask.
 * false nếu out/s NULL, id không có trong registry, mask có bit lạ, hoặc kết quả không validate. */
bool vehicle_settings_resolve(const vehicle_settings_t *s, vehicle_profile_t *out);

/* ---- Runtime: nạp/lưu qua store do bên ngoài cấp (dependency injection) ------------------------- */

/* Lưu trữ bền vững. Firmware dùng adapter NVS (component vehicle_store), test dùng store giả trong RAM. */
typedef struct {
    bool (*load)(uint8_t *buf, size_t cap, size_t *out_len);   /* false nếu chưa có dữ liệu hoặc lỗi */
    bool (*save)(const uint8_t *buf, size_t len);
    bool (*erase)(void);                                       /* lõi KHÔNG tự gọi (không xoá khi dữ liệu hỏng) */
} vehicle_store_ops_t;

typedef enum {
    VEHICLE_SETTINGS_INIT_LOADED = 0,          /* nạp được từ store */
    VEHICLE_SETTINGS_INIT_DEFAULTS_NO_DATA,    /* store rỗng/lỗi đọc/ops NULL → mặc định (EX8) */
    VEHICLE_SETTINGS_INIT_DEFAULTS_INVALID,    /* có dữ liệu nhưng hỏng (CRC/version/id lạ/resolve sai) → mặc định (EX8) */
} vehicle_settings_init_result_t;

/* ops == NULL nghĩa là không lưu. Nạp → decode → resolve → vehicle_profile_set_active; mọi lỗi dùng mặc định,
 * KHÔNG xoá store, không crash. Gọi lại nhiều lần được (mỗi lần đặt lại trạng thái RAM, mô phỏng khởi động lại). */
vehicle_settings_init_result_t vehicle_settings_init(const vehicle_store_ops_t *ops);

const vehicle_settings_t *vehicle_settings_current(void);

/* Đổi hồ sơ gốc sang `id`: xoá toàn bộ override, set_active, lưu. false (id không có) thì không đổi gì.
 * Lưu thất bại vẫn trả true (đã áp dụng trong RAM) — hỏi vehicle_settings_last_save_ok(). */
bool vehicle_settings_select(uint8_t id);

/* Lần ghi gần nhất (select/override) có xuống store thành công không; true nếu chưa ghi hoặc không có store. */
bool vehicle_settings_last_save_ok(void);

/* ---- Ghi đè từng cảm biến (T4.1c) --------------------------------------------------------------- */

/* Thay vị trí/hướng của MỘT cảm biến trên hồ sơ đang chọn. Dựng thử hồ sơ = gốc + override rồi
 * vehicle_profile_validate: slot >= COUNT, pose NULL hoặc kết quả không hợp lệ thì false và KHÔNG đổi gì.
 * Thành công: bật bit trong mask, set_active, lưu (lưu lỗi vẫn trả true — xem last_save_ok). */
bool vehicle_settings_set_override(espnow_slot_t slot, const vehicle_sensor_pose_t *pose);

/* Bỏ ghi đè của một cảm biến → trở về đúng toạ độ hồ sơ gốc. false nếu slot >= COUNT. */
bool vehicle_settings_clear_override(espnow_slot_t slot);

/* Bỏ mọi ghi đè (giữ nguyên hồ sơ gốc đang chọn). */
bool vehicle_settings_clear_all_overrides(void);

/* Bit i = cảm biến slot i đang bị ghi đè. */
uint8_t vehicle_settings_override_mask(void);

#ifdef __cplusplus
}
#endif

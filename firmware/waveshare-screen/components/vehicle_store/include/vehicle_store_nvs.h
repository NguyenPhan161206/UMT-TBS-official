/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_store_nvs.h — Adapter NVS cho vehicle_store_ops_t (lưu blob cài đặt hồ sơ xe).
 */

#pragma once

#include "vehicle_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Ops dùng NVS namespace "vehicle", key "settings" (blob). Người gọi PHẢI gọi nvs_flash_init() trước;
 * nếu chưa, load/save/erase trả false (kèm cảnh báo log). Không bao giờ tự xoá phân vùng NVS. */
const vehicle_store_ops_t *vehicle_store_nvs_ops(void);

#ifdef __cplusplus
}
#endif

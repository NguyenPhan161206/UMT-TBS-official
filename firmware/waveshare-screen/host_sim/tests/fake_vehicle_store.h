/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * fake_vehicle_store.h — Store giả trong RAM cho test vehicle_settings (đếm số lần save/erase, giả lập lỗi đọc/ghi).
 * Chỉ include vào MỘT file test (mỗi file test có bản static riêng).
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "vehicle_settings.h"

static uint8_t g_store[VEHICLE_SETTINGS_BLOB_MAX];
static size_t g_store_len;
static bool g_has;
static int g_save_n, g_erase_n;
static bool g_save_fail, g_load_fail;

static void fake_reset(void)
{
    memset(g_store, 0, sizeof(g_store));
    g_store_len = 0;
    g_has = false;
    g_save_n = g_erase_n = 0;
    g_save_fail = g_load_fail = false;
}

static bool fake_load(uint8_t *buf, size_t cap, size_t *out_len)
{
    if (g_load_fail || !g_has || g_store_len > cap) {
        return false;
    }
    memcpy(buf, g_store, g_store_len);
    *out_len = g_store_len;
    return true;
}

static bool fake_save(const uint8_t *buf, size_t len)
{
    g_save_n++;
    if (g_save_fail || len > sizeof(g_store)) {
        return false;
    }
    memcpy(g_store, buf, len);
    g_store_len = len;
    g_has = true;
    return true;
}

static bool fake_erase(void)
{
    g_erase_n++;
    g_has = false;
    return true;
}

static const vehicle_store_ops_t k_fake_ops = { fake_load, fake_save, fake_erase };

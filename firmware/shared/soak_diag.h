/*
 * SPDX-FileCopyrightText: 2026 Truck Blind-spot Warning System
 * SPDX-License-Identifier: MIT
 *
 * soak_diag.h — SHARED soak/heartbeat contract (R2, DMXT-58).
 *
 * Cả sensor-node và waveshare-screen in 1 dòng "SOAK ..." mỗi TBS_SOAK_HEARTBEAT_INTERVAL_MS
 * và 1 dòng "BOOT ..." lúc khởi động; tools/soak/soak_logger.py đọc 2 dòng này. Bật trong
 * firmware thường (không cần env riêng). Định dạng dòng: docs/SOAK_TEST.md.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Chu kỳ in dòng SOAK (60 s → 1440 dòng/board trong 24 h). */
#define TBS_SOAK_HEARTBEAT_INTERVAL_MS 60000

/* Tên ngắn cho esp_reset_reason_t (giá trị theo ESP-IDF 5.x, ổn định từ IDF 4.x với 0..10).
 * Nhận int để header không phụ thuộc esp_system.h; mỗi firmware static_assert vài giá trị. */
static inline const char *tbs_reset_reason_name(int rr)
{
    switch (rr) {
    case 1:  return "POWERON";
    case 2:  return "EXT";
    case 3:  return "SW";
    case 4:  return "PANIC";
    case 5:  return "INT_WDT";
    case 6:  return "TASK_WDT";
    case 7:  return "WDT";
    case 8:  return "DEEPSLEEP";
    case 9:  return "BROWNOUT";
    case 10: return "SDIO";
    case 11: return "USB";
    case 12: return "JTAG";
    case 13: return "EFUSE";
    case 14: return "PWR_GLITCH";
    case 15: return "CPU_LOCKUP";
    default: return "UNKNOWN";
    }
}

#ifdef __cplusplus
}
#endif

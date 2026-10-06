/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * soak_heartbeat.h — heartbeat soak test (DMXT-58) cho waveshare-screen.
 * Bật trong firmware thường; chu kỳ TBS_SOAK_HEARTBEAT_INTERVAL_MS (firmware/shared/soak_diag.h).
 * Định dạng dòng BOOT/SOAK: docs/SOAK_TEST.md.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tăng bộ đếm boot trong NVS (namespace "diag"), in "BOOT scr boot=<n> rr=<reason>",
 *        rồi tạo task in "SOAK scr ..." mỗi chu kỳ. Gọi SAU nvs_flash_init().
 */
void soak_heartbeat_start(void);

/**
 * @brief Ghi nhận 1 lần link ESP-NOW chuyển UP -> DOWN (gọi từ watchdog link).
 */
void soak_heartbeat_note_link_down(void);

#ifdef __cplusplus
}
#endif

#pragma once

/*
 * task_cfg.h — timing/scheduling config cho NHẤT sensor-node (Arduino).
 *
 * KHÔNG đưa vào firmware/shared/: các giá trị này chỉ sensor-node dùng,
 * waveshare-screen không cần (R2 — shared contract dành cho ký sinh chéo).
 * Đặt tên để dễ đọc/grep thay vì literal (docs/HARDCODED_CONFIG_NOTES.md mục C).
 */

/* Chờ cảm biến ổn định sau power-on (SensorTask). */
#define SENSOR_SETTLE_DELAY_MS 500

/* Chờ Serial/USB CDC sẵn sàng trong setup(). */
#define SERIAL_SETUP_DELAY_MS 200

/* Poll tick chung các task loop (network/coreiot/buzzer) — chỉ "yield",
 * không phải chu kỳ đo (chu kỳ đo = MEASURE_INTERVAL_MS ở thresholds.h). */
#define TASK_POLL_INTERVAL_MS 20

/* Timeout giữ mutex dùng chung (shared_state). */
#define MUTEX_TIMEOUT_MS 10
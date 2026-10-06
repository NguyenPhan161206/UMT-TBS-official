#pragma once

#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Heartbeat soak test (DMXT-58) — bật trong firmware thường, chu kỳ TBS_SOAK_HEARTBEAT_INTERVAL_MS
// (firmware/shared/soak_diag.h). Định dạng dòng: docs/SOAK_TEST.md; tools/soak/soak_logger.py đọc.

// Bộ đếm do các module khác cung cấp, gom lại mỗi lần in.
struct SoakNodeCounters {
    uint32_t txOk;
    uint32_t txFail;
    int32_t mqttReconnects; // -1 khi build không có CoreIoT
};

// Gọi 1 lần trong setup() sau Serial.begin(): tăng bộ đếm boot trong NVS (Preferences),
// in "BOOT node boot=<n> rr=<reason>".
void soakHeartbeatBoot();

// Đăng ký task để in stack high-water mark (byte còn trống thấp nhất). Handle null -> in -1.
void soakHeartbeatSetTasks(TaskHandle_t sensor, TaskHandle_t net, TaskHandle_t buzz);

// Gọi trong vòng lặp một task (networkTask). Tự in dòng SOAK khi đủ chu kỳ.
void soakHeartbeatPoll(uint32_t nowMs, const SoakNodeCounters &c);

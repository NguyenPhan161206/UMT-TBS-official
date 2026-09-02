#pragma once

#include "thresholds.h"

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo chân buzzer (GPIO output, LOW).
void buzzerInit();

// Task FreeRTOS: đọc khoảng cách gần nhất qua sharedStateGetNearest(),
// kêu theo ngưỡng WARNING/DANGER (thresholds.h). Non-blocking (millis()).
// Dùng làm xTaskCreatePinnedToCore(..., "BuzzerTask", ...).
void buzzerTask(void *pvParameters);

#ifdef __cplusplus
}
#endif
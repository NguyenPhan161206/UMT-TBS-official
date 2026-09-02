#pragma once
#include <stddef.h>

#include "thresholds.h"

// Khởi tạo mutex bảo vệ dữ liệu dùng chung. Gọi 1 lần trong setup().
void sharedStateInit();

// Ghi khoảng cách ổn định mới nhất của cảm biến sensorIndex (gọi từ SensorTask)
void sharedStateSet(size_t sensorIndex, float distanceCm, bool valid);

// Đọc khoảng cách ổn định gần nhất của cảm biến sensorIndex (gọi từ bất kỳ
// task nào khác). Trả về true nếu giá trị hợp lệ.
bool sharedStateGet(size_t sensorIndex, float &distanceCm);

// Lấy khoảng cách NHỎ NHẤT (gần nhất) trong các cảm biến hợp lệ.
// Trả về true nếu có ít nhất một cảm biến có giá trị hợp lệ.
// Dùng cho buzzer/telemetry: chỉ cần vật cản gần nhất trong tầm.
bool sharedStateGetNearest(float &nearestCm);
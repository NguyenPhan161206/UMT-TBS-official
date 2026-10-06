/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * hazard_core.h — pure decision logic cho cảnh báo va chạm (lớp 1, kiến trúc
 * G1 — xem docs/ARCHITECTURE_G1_TESTING.md).
 *
 * C thuần, ZERO OS/LVGL dependency → host-compile & unit-test tức thì
 * (host_sim hazard_core_tests, G1 T1.3). Input là PRIMITIVE array
 * (uint16_t* / bool*), KHÔNG phụ thuộc sensor_reading_t của sensor_model
 * → đổi struct sensor_model không lây sang core/tests.
 *
 * R2/R3/B1: chỉ include shared contract — espnow_protocol.h (slot) +
 * thresholds.h (zone + ngưỡng). CẤM include sensor_model.h.
 *
 * B5 (single-truth): hằng heuristic crossing CROSSING_* ĐÃ CHUYỂN TỪ
 * ui_dashboard_theme.h về đây (1 nguồn, grep toàn firmware == 1).
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "espnow_protocol.h" /* espnow_slot_t */
#include "thresholds.h"      /* sensor_zone_t + SENSOR_*_CM (R3) */

#ifdef __cplusplus
extern "C"
{
#endif

/* Heuristic cảnh báo xe cắt ngang (T2.3) — nguồn duy nhất (B5). */
#define CROSSING_DELTA_CM 40
#define CROSSING_FRONT_THRESHOLD_CM 150
/* So sánh mỗi khung với MỐC THAM CHIẾU chỉ làm mới khi đã cũ >= ngần này ms → độ nhạy không phụ thuộc
 * tốc độ khung (ESP-NOW gửi mỗi 100 ms, MQTT ~500 ms). So 2 khung liên tiếp ở 100 ms thì vật cắt ngang
 * 1 m/s chỉ đổi 10 cm/khung và không bao giờ đạt CROSSING_DELTA_CM. Giá trị ước lượng, chỉnh khi có dữ liệu thật. */
#define CROSSING_WINDOW_MS 500
/* Giữ cảnh báo sau lần kích hoạt cuối: một khung 100 ms thì tài xế không kịp thấy. Giá trị ước lượng. */
#define CROSSING_HOLD_MS 3000

/* Giá trị "không xác định sensor" cho hazard_crossing_result_t.sensor. */
#define HAZARD_CROSSING_NO_SENSOR ((espnow_slot_t)ESPNOW_SENSOR_SLOT_COUNT)

typedef struct {
    bool active;          /* đang kích hoạt hoặc đang trong thời gian giữ */
    espnow_slot_t sensor; /* slot bên gây kích hoạt; HAZARD_CROSSING_NO_SENSOR khi không active */
} hazard_crossing_result_t;

/* Trạng thái của heuristic crossing do NGƯỜI GỌI giữ (hazard_core không có static mutable — B2).
 * Khởi tạo bằng = {0} (chưa có lịch sử: khung đầu tiên chỉ đặt mốc tham chiếu, không bao giờ kích hoạt). */
typedef struct {
    uint16_t ref_cm[ESPNOW_SENSOR_SLOT_COUNT];  /* mốc tham chiếu */
    bool ref_ok[ESPNOW_SENSOR_SLOT_COUNT];      /* slot hợp lệ tại mốc tham chiếu */
    uint32_t ref_ms;
    bool has_ref;
    bool holding;                               /* đang giữ cảnh báo */
    uint32_t hold_until_ms;
    espnow_slot_t hold_sensor;
} hazard_crossing_state_t;

/* Phân loại 1 khoảng cách (cm) theo SENSOR_*_CM (R3):
 *   x <= DANGER_CM  -> DANGER   (x == DANGER_CM là DANGER — khớp
 *                                thresholds.h "x <= DANGER_CM", mirror
 *                                rule-chain T5 & python mirror)
 *   x <= CAUTION_CM -> CAUTION
 *   else            -> SAFE
 * Thuần, không FreeRTOS. */
sensor_zone_t hazard_classify(uint16_t distance_cm);

/* Zone tệ nhất giữa n slot; skip slot stale (is_stale[i] == true) hoặc slot
 * có health là DISCONNECTED/STALE (nếu health != NULL). Safety-Critical:
 * slot hỏng/mất nguồn không được dùng khoảng cách cũ để báo DANGER.
 * dist/stale NULL hoặc n==0 -> SAFE. */
sensor_zone_t hazard_worst_zone(const uint16_t *dist_cm,
                                const bool     *is_stale,
                                const uint8_t  *health,
                                size_t          n);

/* Kiểm tra xà hệ thống có ít nhất 1 cảm biến lỗi/mất kết nối.
 * Trả về true nếu bất kỳ slot nào ở DISCONNECTED hoặc STALE. */
bool hazard_has_sensor_fault(const uint8_t *health, size_t n);

/* Heuristic xe cắt ngang (T2.3). Mỗi lần gọi = một khung dữ liệu tại now_ms (ms tăng dần, được phép tràn uint32).
 *  - ok[i]: slot i đang có dữ liệu hợp lệ (không stale/mất kết nối); ok == NULL nghĩa là mọi slot hợp lệ.
 *    Slot bị xoá có khoảng cách 0 — KHÔNG được coi là vật ở 0 cm (trước đây gây báo giả khi mất link).
 *  - "gần phía trước": ok[FRONT] && cur[FRONT] < CROSSING_FRONT_THRESHOLD_CM.
 *  - góc trước (LEFT_FRONT, RIGHT_FRONT) "đổi nhanh": hợp lệ cả lúc này lẫn ở mốc tham chiếu và
 *    |cur - ref| >= CROSSING_DELTA_CM (cả hai thì lấy RIGHT_FRONT). Slot bên SAU không xét (xe chạy dọc hông
 *    không phải cắt ngang phía trước).
 *  - kích hoạt = gần phía trước && có slot đổi nhanh → giữ thêm CROSSING_HOLD_MS; mất FRONT thì huỷ giữ.
 *  - mốc tham chiếu làm mới (sau khi so sánh) khi đã cũ >= CROSSING_WINDOW_MS.
 * st/cur NULL hoặc n < ESPNOW_SENSOR_SLOT_COUNT → inactive + NO_SENSOR, không đổi st. */
hazard_crossing_result_t hazard_eval_crossing(hazard_crossing_state_t *st,
                                              const uint16_t *cur_cm,
                                              const bool *ok,
                                              size_t n,
                                              uint32_t now_ms);

#ifdef __cplusplus
}
#endif
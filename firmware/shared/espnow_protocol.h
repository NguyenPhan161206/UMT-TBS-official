/*
 * SPDX-FileCopyrightText: 2026 Truck Blind-spot Warning System
 * SPDX-License-Identifier: MIT
 *
 * espnow_protocol.h — SHARED ESP-NOW WIRE CONTRACT (R2).
 *
 * SINGLE definition of the ESP-NOW payload exchanged between
 * sensor-node -> waveshare-screen. Previously this struct + channel + MAC
 * were copy-pasted in both firmwares and "kept in sync by hand" — a
 * mismatch source (R2 violation in the old repo). Now it lives HERE only.
 *
 * Cấm định nghĩa lại espnow_sensor_msg_t / espnow_slot_t / channel / MAC
 * ở bất kỳ đâu khác; grep toàn firmware phải == 1.
 */
#pragma once

#include <stdint.h>

#include "thresholds.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WiFi channel dùng cho ESP-NOW.
 *
 * Đây là kênh DEFAULT/FALLBACK khi STA CHƯA nối AP (hoặc board chạy không kết
 * nối Wi-Fi): cả 2 board ghim radio ở kênh này để khớp nhau.
 *
 * Khi STA ĐÃ nối AP (CoreIoT/Wi-Fi): kênh thật do AP quyết định và có thể tự
 * đổi bất kỳ lúc nào (ví dụ iPhone hotspot phát Channel Switch Announcement,
 * log "sta rx csa 1->11"). ESP32-S3 single radio nên mọi board PHẢI bám kênh
 * home thật; sensor-node sync peer theo esp_wifi_get_channel trước mỗi send
 * (xem espnow_client.cpp::syncPeerChannelToHome) — KHÔNG được cố định kênh 6
 * khi đã nối AP (gây "Peer channel is not equal to the home channel"). */
#define ESPNOW_CHANNEL 6

/* Địa chỉ ESP-NOW đích mà sensor-node gửi tới.
 *
 * BROADCAST (FF:FF:FF:FF:FF:FF): waveshare-screen (`espnow_receiver`) nhận mọi
 * gói, kể cả broadcast, nên không cần khớp MAC waveshare theo board/cổng (board
 * tháo/cắm hoặc đổi cổng USB sẽ đổi MAC Wi-Fi → unicast dễ lỗi). Mạng 2-node
 * cục bộ, broadcast là lựa chọn robust (quyết định 2026-09-03).
 *
 * Cả 2 board phải ở cùng WiFi channel (ESPNOW_CHANNEL) để nhận được.
 */
static const uint8_t ESPNOW_PEER_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* Tần suất gửi ESP-NOW: 100ms (10 gói/giây) cho phản xạ cảnh báo tức thì. */
#define ESPNOW_SEND_INTERVAL_MS 100
/* Link timeout (receiver): 1500ms (dung sai 15 gói miss liên tiếp). */
#define ESPNOW_LINK_TIMEOUT_MS 1500

/* Số vị trí cảm biến trên "dây" — PHẢI khớp SENSOR_COUNT (thresholds.h).
 * Cố định 6: khớp mô hình sensor_model/ui_dashboard bên waveshare-screen. */
#define ESPNOW_SENSOR_SLOT_COUNT 6

/* Thứ tự slot trên dây == thứ tự vật lý trong thresholds.h::SENSOR_PINS.
 * Ánh xạ thẳng sang sensor_id_t bên waveshare-screen. */
typedef enum {
    ESPNOW_SLOT_FRONT = 0,       /* S0  */
    ESPNOW_SLOT_REAR = 1,        /* S1  */
    ESPNOW_SLOT_LEFT_FRONT = 2,  /* S2  */
    ESPNOW_SLOT_LEFT_REAR = 3,   /* S3  */
    ESPNOW_SLOT_RIGHT_FRONT = 4, /* S4  */
    ESPNOW_SLOT_RIGHT_REAR = 5,  /* S5  */
} espnow_slot_t;

/* Payload nhị phân gửi qua ESP-NOW — packed, kích thước cố định.
 * Luôn gửi đủ ESPNOW_SENSOR_SLOT_COUNT vị trí;
 *   valid[i] = 0 và health[i] = SENSOR_HEALTH_DISCONNECTED: slot hỏng/mất
 *   valid[i] = 0 và health[i] = SENSOR_HEALTH_OUT_OF_RANGE: thoáng, không vật cản
 *   valid[i] = 1 và health[i] = SENSOR_HEALTH_OK: đo tốt, distance_cm[i] hợp lệ
 * Bên waveshare-screen KHÔNG được đọc distance_cm[i] làm số hợp lệ khi
 * valid[i]=0 hoặc health[i] ở trạng thái lỗi. */
typedef struct __attribute__((packed)) {
    uint16_t seq;                                   /* Số thứ tự gói (freshness) */
    float    distance_cm[ESPNOW_SENSOR_SLOT_COUNT]; /* Khoảng cách (cm) — chỉ hợp lệ khi valid[i]==1 */
    uint8_t  valid[ESPNOW_SENSOR_SLOT_COUNT];       /* 1 = có đị; 0 = null slot */
    uint8_t  health[ESPNOW_SENSOR_SLOT_COUNT];      /* sensor_health_t: OK / OUT_OF_RANGE / DISCONNECTED / STALE */
} espnow_sensor_msg_t;

/* Chặn lệch giữa số slot trên dây và số cảm biến vật lý (R2/R4). */
TBS_STATIC_ASSERT(ESPNOW_SENSOR_SLOT_COUNT == SENSOR_COUNT,
                  "ESP-NOW wire slot count must match physical SENSOR_COUNT");
/* Chặn vỡ layout khi thêm trường mới mà quên cập nhật cả hai board. */
TBS_STATIC_ASSERT(sizeof(espnow_sensor_msg_t) ==
                  sizeof(uint16_t) +
                  sizeof(float) * ESPNOW_SENSOR_SLOT_COUNT +
                  sizeof(uint8_t) * ESPNOW_SENSOR_SLOT_COUNT * 2,
                  "espnow_sensor_msg_t size mismatch — update both firmwares");

#ifdef __cplusplus
}
#endif

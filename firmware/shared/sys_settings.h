#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SENSOR_DANGER_CM 30
#define DEFAULT_SENSOR_CAUTION_CM 100
#define DEFAULT_BACKLIGHT_LEVEL 100

/* Core settings that can be customized by the user */
typedef struct __attribute__((packed)) {
    uint16_t danger_cm;
    uint16_t caution_cm;
    uint8_t  backlight_level;
    uint8_t  _reserved; // Pad to even bytes
} sys_settings_t;

/* Wi-Fi Configuration */
#define MAX_WIFI_SSID_LEN 32
#define MAX_WIFI_PASS_LEN 64

typedef struct __attribute__((packed)) {
    char ssid[MAX_WIFI_SSID_LEN];
    char password[MAX_WIFI_PASS_LEN];
} sys_wifi_config_t;

#define ESPNOW_CMD_TYPE_SYNC_SETTINGS 2
#define ESPNOW_CMD_TYPE_SYNC_WIFI 3

typedef struct __attribute__((packed)) {
    uint8_t cmd_type; /* ESPNOW_CMD_TYPE_SYNC_SETTINGS */
    sys_settings_t settings;
} espnow_sync_settings_msg_t;

typedef struct __attribute__((packed)) {
    uint8_t cmd_type; /* ESPNOW_CMD_TYPE_SYNC_WIFI */
    sys_wifi_config_t wifi;
} espnow_sync_wifi_msg_t;

#ifdef __cplusplus
}
#endif

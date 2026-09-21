#pragma once
#include "sys_settings.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo NVS và load settings
void sys_settings_init(void);

// Đọc cài đặt hệ thống (Thresholds, UI)
void sys_settings_get(sys_settings_t *out_settings);
// Lưu cài đặt hệ thống
void sys_settings_save(const sys_settings_t *settings);

// Đọc cấu hình Wi-Fi
// Trả về true nếu NVS có chứa Wi-Fi hợp lệ
bool sys_settings_get_wifi(sys_wifi_config_t *out_wifi);
// Lưu cấu hình Wi-Fi
void sys_settings_save_wifi(const sys_wifi_config_t *wifi);
// Xoá cấu hình Wi-Fi
void sys_settings_clear_wifi(void);

#ifdef __cplusplus
}
#endif

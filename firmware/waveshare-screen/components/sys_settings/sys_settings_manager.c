#include "sys_settings_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SYS_SETTINGS";

static sys_settings_t g_settings;
static bool g_has_wifi = false;
static sys_wifi_config_t g_wifi;

void sys_settings_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    size_t required_size = sizeof(sys_settings_t);
    err = nvs_get_blob(my_handle, "sys_settings", &g_settings, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error (%s) reading sys_settings!", esp_err_to_name(err));
    }
    
    if (err == ESP_ERR_NVS_NOT_FOUND || required_size != sizeof(sys_settings_t)) {
        ESP_LOGI(TAG, "sys_settings not found or size mismatch, applying defaults");
        g_settings.danger_cm = DEFAULT_SENSOR_DANGER_CM;
        g_settings.caution_cm = DEFAULT_SENSOR_CAUTION_CM;
        g_settings.backlight_level = DEFAULT_BACKLIGHT_LEVEL;
        sys_settings_save(&g_settings);
    } else {
        ESP_LOGI(TAG, "Loaded sys_settings: danger=%d, caution=%d", g_settings.danger_cm, g_settings.caution_cm);
    }

    // Load Wi-Fi
    required_size = sizeof(sys_wifi_config_t);
    err = nvs_get_blob(my_handle, "sys_wifi", &g_wifi, &required_size);
    if (err == ESP_OK && required_size == sizeof(sys_wifi_config_t)) {
        g_has_wifi = true;
        ESP_LOGI(TAG, "Loaded sys_wifi: SSID=%s", g_wifi.ssid);
    } else {
        g_has_wifi = false;
        ESP_LOGI(TAG, "sys_wifi not found in NVS (Fallback will be used)");
    }

    nvs_close(my_handle);
}

void sys_settings_get(sys_settings_t *out_settings)
{
    if (out_settings) {
        memcpy(out_settings, &g_settings, sizeof(sys_settings_t));
    }
}

void sys_settings_save(const sys_settings_t *settings)
{
    if (!settings) return;
    memcpy(&g_settings, settings, sizeof(sys_settings_t));

    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_blob(my_handle, "sys_settings", &g_settings, sizeof(sys_settings_t));
        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "sys_settings saved to NVS");
    }
}

bool sys_settings_get_wifi(sys_wifi_config_t *out_wifi)
{
    if (!g_has_wifi || !out_wifi) return false;
    memcpy(out_wifi, &g_wifi, sizeof(sys_wifi_config_t));
    return true;
}

void sys_settings_save_wifi(const sys_wifi_config_t *wifi)
{
    if (!wifi) return;
    memcpy(&g_wifi, wifi, sizeof(sys_wifi_config_t));
    g_has_wifi = true;

    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_blob(my_handle, "sys_wifi", &g_wifi, sizeof(sys_wifi_config_t));
        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "sys_wifi saved to NVS");
    }
}

void sys_settings_clear_wifi(void)
{
    g_has_wifi = false;
    memset(&g_wifi, 0, sizeof(sys_wifi_config_t));
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_erase_key(my_handle, "sys_wifi");
        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "sys_wifi cleared from NVS");
    }
}

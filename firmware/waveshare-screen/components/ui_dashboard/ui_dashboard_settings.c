#include "ui_dashboard_private.h"
#include "sys_settings_manager.h"
#include "espnow_receiver.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <stdio.h>

static lv_obj_t *s_settings_win;
static lv_obj_t *s_kb;
static lv_obj_t *s_ta_ssid;
static lv_obj_t *s_ta_pass;
static lv_obj_t *s_slider_danger;
static lv_obj_t *s_slider_caution;
static lv_obj_t *s_lbl_danger;
static lv_obj_t *s_lbl_caution;

static void close_cb(lv_event_t * e)
{
    lv_obj_delete(s_settings_win);
    s_settings_win = NULL;
}

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(s_kb, ta);
        lv_obj_remove_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
    if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(s_kb, NULL);
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void slider_danger_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    lv_label_set_text_fmt(s_lbl_danger, "Danger: %d cm", val);
}

static void slider_caution_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    lv_label_set_text_fmt(s_lbl_caution, "Caution: %d cm", val);
}

static void save_cb(lv_event_t * e)
{
    // Save to NVS
    sys_settings_t set;
    set.danger_cm = lv_slider_get_value(s_slider_danger);
    set.caution_cm = lv_slider_get_value(s_slider_caution);
    set.backlight_level = 100;
    set._reserved = 0;
    sys_settings_save(&set);

    sys_wifi_config_t wifi;
    snprintf(wifi.ssid, sizeof(wifi.ssid), "%s", lv_textarea_get_text(s_ta_ssid));
    snprintf(wifi.password, sizeof(wifi.password), "%s", lv_textarea_get_text(s_ta_pass));
    sys_settings_save_wifi(&wifi);

    // Sync via ESP-NOW
    espnow_sync_settings_msg_t sync_set;
    sync_set.cmd_type = ESPNOW_CMD_TYPE_SYNC_SETTINGS;
    sync_set.settings = set;
    
    // Switch to Channel 1 for Handshake
    uint8_t old_chan;
    wifi_second_chan_t old_sec;
    esp_wifi_get_channel(&old_chan, &old_sec);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    
    espnow_receiver_send_cmd((espnow_cmd_msg_t*)&sync_set); // Cần cast nếu hàm yêu cầu, hoặc thêm hàm mới
    
    // Wait briefly for send
    vTaskDelay(pdMS_TO_TICKS(50));
    
    if (strlen(wifi.ssid) > 0) {
        espnow_sync_wifi_msg_t sync_wifi;
        sync_wifi.cmd_type = ESPNOW_CMD_TYPE_SYNC_WIFI;
        sync_wifi.wifi = wifi;
        espnow_receiver_send_cmd((espnow_cmd_msg_t*)&sync_wifi);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    esp_wifi_set_channel(old_chan, old_sec); // Khôi phục kênh
    
    close_cb(NULL);
    // Restart device to apply WiFi? Or let user do it
}

void ui_dashboard_create_settings(lv_obj_t *parent)
{
    if (s_settings_win) return;

    s_settings_win = lv_obj_create(parent);
    lv_obj_set_size(s_settings_win, lv_pct(90), lv_pct(90));
    lv_obj_center(s_settings_win);
    lv_obj_set_flex_flow(s_settings_win, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(s_settings_win, lv_color_hex(0x202020), 0);

    lv_obj_t * header = lv_obj_create(s_settings_win);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "System Settings (Danger >= 20cm)");
    
    lv_obj_t * close_btn = lv_button_create(header);
    lv_obj_t * close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "Close");
    lv_obj_add_event_cb(close_btn, close_cb, LV_EVENT_CLICKED, NULL);

    // Wi-Fi inputs
    s_ta_ssid = lv_textarea_create(s_settings_win);
    lv_textarea_set_placeholder_text(s_ta_ssid, "Wi-Fi SSID");
    lv_obj_set_width(s_ta_ssid, lv_pct(100));
    lv_obj_add_event_cb(s_ta_ssid, ta_event_cb, LV_EVENT_ALL, NULL);
    
    s_ta_pass = lv_textarea_create(s_settings_win);
    lv_textarea_set_placeholder_text(s_ta_pass, "Wi-Fi Password");
    lv_textarea_set_password_mode(s_ta_pass, true);
    lv_obj_set_width(s_ta_pass, lv_pct(100));
    lv_obj_add_event_cb(s_ta_pass, ta_event_cb, LV_EVENT_ALL, NULL);

    // Load current settings
    sys_wifi_config_t curr_wifi;
    if (sys_settings_get_wifi(&curr_wifi)) {
        lv_textarea_set_text(s_ta_ssid, curr_wifi.ssid);
        lv_textarea_set_text(s_ta_pass, curr_wifi.password);
    }
    
    sys_settings_t curr_set;
    sys_settings_get(&curr_set);

    // Danger slider
    s_lbl_danger = lv_label_create(s_settings_win);
    lv_label_set_text_fmt(s_lbl_danger, "Danger: %d cm", curr_set.danger_cm);
    s_slider_danger = lv_slider_create(s_settings_win);
    lv_obj_set_width(s_slider_danger, lv_pct(100));
    lv_slider_set_range(s_slider_danger, 20, 150); // Hard Constraint!
    lv_slider_set_value(s_slider_danger, curr_set.danger_cm, LV_ANIM_OFF);
    lv_obj_add_event_cb(s_slider_danger, slider_danger_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Caution slider
    s_lbl_caution = lv_label_create(s_settings_win);
    lv_label_set_text_fmt(s_lbl_caution, "Caution: %d cm", curr_set.caution_cm);
    s_slider_caution = lv_slider_create(s_settings_win);
    lv_obj_set_width(s_slider_caution, lv_pct(100));
    lv_slider_set_range(s_slider_caution, 100, 300); // Hard Constraint!
    lv_slider_set_value(s_slider_caution, curr_set.caution_cm, LV_ANIM_OFF);
    lv_obj_add_event_cb(s_slider_caution, slider_caution_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Save Button
    lv_obj_t * save_btn = lv_button_create(s_settings_win);
    lv_obj_t * save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "Save & Sync");
    lv_obj_add_event_cb(save_btn, save_cb, LV_EVENT_CLICKED, NULL);

    // Keyboard
    s_kb = lv_keyboard_create(parent);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
}

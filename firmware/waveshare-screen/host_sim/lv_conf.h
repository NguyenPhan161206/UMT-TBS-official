/*
 * SPDX-PackageName: umt_host_sim (T1.2)
 * lv_conf.h — cấu hình LVGL v9.1 cho simulator trên máy tính (SDL2).
 * Đây là bản HOST ONLY (không ảnh hưởng firmware thật dùng sdkconfig).
 * Kích thước màn hình: 800x480 (khớp waveshare-screen).
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_OS 0

/* ----------------------------- color & memory ---------------------------- */
#define LV_COLOR_DEPTH 16
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64 * 1024)          /* 64 KB: host thừa RAM, thoải mái */

/* -------------------------------- drivers -------------------------------- */
#define LV_USE_SDL 1                     /* LVGL dùng SDL2 render (xem main.c) */
#define LV_COLOR_16_SWAP 0

/* Tick nguồn = SDL_GetTicks (host không có ESP-IDF tick). */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "SDL2/SDL.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (SDL_GetTicks())

/* ------------------ widgets được dùng bởi ui_dashboard* ------------------- */
#define LV_USE_ARC 1
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_IMG 1

/* ------------------------------ layout/anim ------------------------------- */
#define LV_USE_FLEX 1
#define LV_USE_GRID 1
#define LV_USE_ANIM 1
#define LV_USE_TIMER 1
#define LV_USE_REFR 1

/* -------------------------------- fonts ----------------------------------- */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_FONT_EXT_PLACEHOLDERS 1

/* ------------------------------- logging --------------------------------- */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* ------------------------------- misc ------------------------------------ */
#define LV_DEF_REFR_PERIOD 30            /* 30ms gần với refresh thật */
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 1
#define LV_USE_ASSERT_MEM_INTEGRITY 0

#endif /* LV_CONF_H */
/*
 * SPDX-PackageName: umt_host_sim (G1 T1.2)
 *
 * main.c — LVGL v9.1.0 + SDL2 headless simulator của waveshare-screen UI.
 *
 * Chạy file UI THẬT (ui_dashboard.c + ui_dashboard_layout.c +
 * ui_dashboard_system.c) trên host, feed dữ liệu từ 4 kịch bản G1
 * (tools/scenarios.py -> scenarios_gen.h) hoặc --replay JSONL (schema V2:
 *  {"d1":..,"d2":..,...,"d6":..}).
 *
 * Môi trường không có X (CI/headless): SDL_VIDEODRIVER=dummy là ĐỦ để render
 * offscreen và thoát 0 sau --exit-after giây.
 *
 * CLI:
 *   umt_dash_sim [--scenario <name>] [--replay <path>]
 *                [--exit-after <sec>] [--interval <ms>]
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SDL2/SDL.h"

#include "lvgl.h"

#include "ui_dashboard.h"
#include "scenarios_gen.h"
#include "sensor_model.h"

#define SIM_W 800
#define SIM_H 480

#define DEFAULT_EXIT_AFTER_MS 3000u
#define DEFAULT_INTERVAL_MS 500u

/* Replay fixture: tìm `"d1"`..`"d6"` trong một dòng JSON rồi parse số. */
static bool replay_row_distance(const char *line, uint16_t out[6])
{
    static const char *keys[6] = {"\"d1\"", "\"d2\"", "\"d3\"",
                                  "\"d4\"", "\"d5\"", "\"d6\""};
    for (int i = 0; i < 6; i++) {
        const char *p = strstr(line, keys[i]);
        if (p == NULL) {
            return false;
        }
        p = strchr(p, ':');
        if (p == NULL) {
            return false;
        }
        p++; /* skip ':' */
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        double v = strtod(p, NULL);
        if (v < 0.0) {
            return false;
        }
        out[i] = (uint16_t)(v + 0.5);
    }
    return true;
}

static void feed_one_step(const uint16_t dists[6], uint32_t tick_ms)
{
    /* Giả lập một mốc thời gian: đẩy cả vào sensor_model (hazard core) lẫn UI. */
    for (int id = 0; id < 6; id++) {
        sensor_model_set_distance((sensor_id_t)id, dists[id]);
        ui_dashboard_update_sensor((uint8_t)id, dists[id]);
    }
    (void)tick_ms;
    printf("[sim] feed step: d1=%u d2=%u d3=%u d4=%u d5=%u d6=%u\n",
           dists[0], dists[1], dists[2], dists[3], dists[4], dists[5]);
}

static void feed_status_badges(void)
{
    /* V1 đã wire các badge này (xem ui_dashboard.c) — đặt giá trị giả, KHÔNG
     * đụng secret thật (R1): ip trống + không bật espnow để SYSTEM page render. */
    ui_dashboard_set_wifi_status(true, "192.168.1.99");
    ui_dashboard_set_mqtt_status(true);
    ui_dashboard_set_espnow_status(false);
    ui_dashboard_set_buzzer_state(false);
    ui_dashboard_set_relay_state(false, "NORMAL");
    ui_dashboard_set_hazard_warning(false);
}

static int run_scenario(const char *name, uint32_t exit_after_ms, uint32_t interval_ms)
{
    const sim_scenario_t *sc = NULL;
    for (int i = 0; i < SIM_SCENARIO_COUNT; i++) {
        if (strcmp(k_sim_scenarios[i].name, name) == 0) {
            sc = &k_sim_scenarios[i];
            break;
        }
    }
    if (sc == NULL) {
        fprintf(stderr, "[sim] unknown scenario '%s' (available:", name);
        for (int i = 0; i < SIM_SCENARIO_COUNT; i++) {
            fprintf(stderr, " %s", k_sim_scenarios[i].name);
        }
        fprintf(stderr, ")\n");
        return 2;
    }

    uint32_t start = SDL_GetTicks();
    int step = 0;
    while ((SDL_GetTicks() - start) < exit_after_ms) {
        if (step < sc->n) {
            feed_one_step(sc->rows[step], SDL_GetTicks());
            step++;
        }
        int32_t delay = lv_timer_handler();
        SDL_Delay((uint32_t)(delay > 0 && delay < 50 ? delay : 1));
        if ((SDL_GetTicks() - start) < exit_after_ms && interval_ms > 0) {
            SDL_Delay(interval_ms);
        } else {
            break;
        }
    }
    printf("[sim] scenario '%s' replayed %d/%d steps in %lums\n", name, step,
           sc->n, (unsigned long)(SDL_GetTicks() - start));
    return 0;
}

static int run_replay(const char *path, uint32_t exit_after_ms, uint32_t interval_ms)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "[sim] cannot open replay '%s'\n", path);
        return 2;
    }
    char line[512];
    uint32_t start = SDL_GetTicks();
    int n = 0;
    while (fgets(line, sizeof(line), f) != NULL &&
           (SDL_GetTicks() - start) < exit_after_ms) {
        if (line[0] == '\0' || line[0] == '\n') {
            continue;
        }
        uint16_t dists[6] = {0};
        if (!replay_row_distance(line, dists)) {
            fprintf(stderr, "[sim] skip invalid replay row %d: %s", n + 1, line);
            continue;
        }
        feed_one_step(dists, SDL_GetTicks());
        n++;
        int32_t delay = lv_timer_handler();
        SDL_Delay((uint32_t)(delay > 0 && delay < 50 ? delay : 1));
        if (interval_ms > 0) {
            SDL_Delay(interval_ms);
        }
    }
    fclose(f);
    printf("[sim] replay '%s' fed %d row(s) in %lums\n", path, n,
           (unsigned long)(SDL_GetTicks() - start));
    return 0;
}

int main(int argc, char **argv)
{
    const char *scenario = NULL;
    const char *replay = NULL;
    uint32_t exit_after_ms = DEFAULT_EXIT_AFTER_MS;
    uint32_t interval_ms = DEFAULT_INTERVAL_MS;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            scenario = argv[++i];
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            replay = argv[++i];
        } else if (strcmp(argv[i], "--exit-after") == 0 && i + 1 < argc) {
            exit_after_ms = (uint32_t)atoi(argv[++i]) * 1000u;
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            interval_ms = (uint32_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: umt_dash_sim [--scenario <name>|--replay <jsonl>] "
                   "[--exit-after <sec>] [--interval <ms>]\n");
            printf("Scenarios:");
            for (int j = 0; j < SIM_SCENARIO_COUNT; j++) {
                printf(" %s", k_sim_scenarios[j].name);
            }
            printf("\n");
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            return 2;
        }
    }
    if (scenario == NULL && replay == NULL) {
        scenario = "normal";
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 3;
    }

    lv_init();
    lv_display_t *disp = lv_sdl_window_create(SIM_W, SIM_H);
    if (disp == NULL) {
        fprintf(stderr, "lv_sdl_window_create failed\n");
        SDL_Quit();
        return 3;
    }
    lv_display_set_default(disp);

    ui_dashboard_init();
    /* Batch một số badge dù scenario không feed */
    feed_status_badges();

    int rc = (replay != NULL)
                 ? run_replay(replay, exit_after_ms, interval_ms)
                 : run_scenario(scenario, exit_after_ms, interval_ms);

    lv_deinit();
    SDL_Quit();
    printf("[sim] exiting rc=%d (OK)\n", rc);
    return rc;
}
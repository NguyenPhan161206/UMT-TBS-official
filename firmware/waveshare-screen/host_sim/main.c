/*
 * SPDX-PackageName: umt_host_sim (G1 T1.2)
 *
 * main.c — LVGL v9.1.0 + SDL2 headless simulator của waveshare-screen UI.
 *
 * Chạy file UI THẬT (ui_dashboard.c + ui_dashboard_layout.c +
 * ui_dashboard_system.c) trên host, feed dữ liệu từ 4 kịch bản G1
 * (tools/scenarios.py -> scenarios_gen.h) hoặc --replay JSONL, hai định dạng:
 *  - payload V2 {"d1":..,...,"d6":..} (tools/record_telemetry.py)
 *  - file tools/recorder {"elapsed_ms":..,"distances":[..],"valid":[..]} (valid=0 -> không dữ liệu).
 *
 * Môi trường không có X (CI/headless): SDL_VIDEODRIVER=dummy là ĐỦ để render
 * offscreen và thoát 0 sau --exit-after giây.
 *
 * CLI:
 *   umt_dash_sim [--scenario <name>] [--replay <path>] [--list]
 *                [--exit-after <sec>] [--interval <ms>] [--speed <x>]
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
#define REPLAY_TAIL_MS 1000u

/* Một dòng replay đã parse: khoảng cách 6 slot, cờ hợp lệ từng slot, thời điểm ghi (ms) nếu có. */
typedef struct {
    uint16_t dist[6];
    bool valid[6];
    bool has_elapsed;
    uint32_t elapsed_ms;
} replay_row_t;

/* Con trỏ ngay sau ':' của khóa `key` (đã gồm dấu ngoặc kép), bỏ khoảng trắng; NULL nếu không có. */
static const char *json_value(const char *line, const char *key)
{
    const char *p = strstr(line, key);
    if (p == NULL) {
        return NULL;
    }
    p = strchr(p + strlen(key), ':');
    if (p == NULL) {
        return NULL;
    }
    p++; /* skip ':' */
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return p;
}

/* Đọc "[a, b, c, d, e, f]" gồm đúng 6 phần tử là số (hoặc true/false khi allow_bool). */
static bool json_array6(const char *p, double out[6], bool allow_bool)
{
    if (p == NULL || *p != '[') {
        return false;
    }
    p++;
    for (int i = 0; i < 6; i++) {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (allow_bool && strncmp(p, "true", 4) == 0) {
            out[i] = 1.0;
            p += 4;
        } else if (allow_bool && strncmp(p, "false", 5) == 0) {
            out[i] = 0.0;
            p += 5;
        } else {
            char *end = NULL;
            out[i] = strtod(p, &end);
            if (end == p) {
                return false;
            }
            p = end;
        }
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (i < 5) {
            if (*p != ',') {
                return false;
            }
            p++;
        } else if (*p != ']') {
            return false;
        }
    }
    return true;
}

static uint16_t to_u16(double v)
{
    return (v >= 65535.0) ? (uint16_t)65535 : (uint16_t)(v + 0.5);
}

/* Hai định dạng một dòng JSON:
 *  - payload V2 (tools/record_telemetry.py): {"d1":..,...,"d6":..}
 *  - file của tools/recorder: {"elapsed_ms":..,"distances":[6 số],"valid":[6 cờ],...}
 *    valid[i]=0 nghĩa là slot KHÔNG có dữ liệu (khoảng cách 0.0 không phải vật ở 0 cm). */
static bool replay_parse_row(const char *line, replay_row_t *row)
{
    memset(row, 0, sizeof(*row));
    for (int i = 0; i < 6; i++) {
        row->valid[i] = true;
    }

    const char *dp = json_value(line, "\"distances\"");
    if (dp != NULL) {
        double d[6];
        double v[6];
        if (!json_array6(dp, d, false)) {
            return false;
        }
        const char *vp = json_value(line, "\"valid\"");
        if (vp != NULL) {
            if (!json_array6(vp, v, true)) {
                return false;
            }
            for (int i = 0; i < 6; i++) {
                row->valid[i] = (v[i] != 0.0);
            }
        }
        for (int i = 0; i < 6; i++) {
            if (row->valid[i]) {
                if (d[i] < 0.0) {
                    return false;
                }
                row->dist[i] = to_u16(d[i]);
            }
        }
    } else {
        static const char *keys[6] = {"\"d1\"", "\"d2\"", "\"d3\"",
                                      "\"d4\"", "\"d5\"", "\"d6\""};
        for (int i = 0; i < 6; i++) {
            const char *p = json_value(line, keys[i]);
            if (p == NULL) {
                return false;
            }
            double v = strtod(p, NULL);
            if (v < 0.0) {
                return false;
            }
            row->dist[i] = to_u16(v);
        }
    }

    const char *ep = json_value(line, "\"elapsed_ms\"");
    if (ep != NULL) {
        char *end = NULL;
        double e = strtod(ep, &end);
        if (end != ep && e >= 0.0) {
            row->has_elapsed = true;
            row->elapsed_ms = (uint32_t)e;
        }
    }
    return true;
}

static void feed_row(const replay_row_t *row)
{
    /* Giả lập một mốc thời gian: đẩy cả vào sensor_model (hazard core) lẫn UI.
     * Slot không hợp lệ -> "không dữ liệu" (cung xám, ẩn chấm), như firmware khi mất tín hiệu. */
    char txt[6][12];
    for (int id = 0; id < 6; id++) {
        if (row->valid[id]) {
            sensor_model_set_distance((sensor_id_t)id, row->dist[id]);
            ui_dashboard_update_sensor((uint8_t)id, row->dist[id]);
            snprintf(txt[id], sizeof(txt[id]), "%u", row->dist[id]);
        } else {
            ui_dashboard_clear_sensor((uint8_t)id);
            snprintf(txt[id], sizeof(txt[id]), "--");
        }
    }
    printf("[sim] feed step: d1=%s d2=%s d3=%s d4=%s d5=%s d6=%s\n",
           txt[0], txt[1], txt[2], txt[3], txt[4], txt[5]);
}

static void feed_one_step(const uint16_t dists[6], uint32_t tick_ms)
{
    replay_row_t row;
    memset(&row, 0, sizeof(row));
    for (int i = 0; i < 6; i++) {
        row.dist[i] = dists[i];
        row.valid[i] = true;
    }
    (void)tick_ms;
    feed_row(&row);
}

/* Chờ `ms` mili-giây nhưng vẫn cho LVGL xử lý (vẽ, sự kiện cửa sổ) để cửa sổ không "treo". */
static void sleep_pump(uint32_t ms)
{
    uint32_t t0 = SDL_GetTicks();
    while ((SDL_GetTicks() - t0) < ms) {
        lv_timer_handler();
        uint32_t left = ms - (SDL_GetTicks() - t0);
        SDL_Delay(left < 10u ? left : 10u);
    }
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

/* Nhịp phát: mặc định theo chênh lệch elapsed_ms giữa các dòng (nếu có), --interval ép nhịp cố định;
 * --speed chia thời gian chờ (2.0 = nhanh gấp đôi). Không có --exit-after thì phát tới hết file. */
static int run_replay(const char *path, uint32_t exit_after_ms, bool exit_after_set,
                      uint32_t interval_ms, bool interval_set, double speed)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "[sim] cannot open replay '%s'\n", path);
        return 2;
    }
    char line[2048];
    uint32_t start = SDL_GetTicks();
    uint32_t limit_ms = exit_after_set ? exit_after_ms : 0xFFFFFFFFu;
    int n = 0;
    int skipped = 0;
    uint32_t prev_elapsed = 0;
    bool have_prev = false;
    while (fgets(line, sizeof(line), f) != NULL && (SDL_GetTicks() - start) < limit_ms) {
        if (line[0] == '\0' || line[0] == '\n') {
            continue;
        }
        replay_row_t row;
        if (!replay_parse_row(line, &row)) {
            skipped++;
            fprintf(stderr, "[sim] skip invalid replay row %d: %s", n + skipped, line);
            continue;
        }
        if (n > 0) {
            double wait_ms = (double)interval_ms;
            if (!interval_set && row.has_elapsed && have_prev) {
                wait_ms = (row.elapsed_ms > prev_elapsed) ? (double)(row.elapsed_ms - prev_elapsed) : 0.0;
            }
            sleep_pump((uint32_t)(wait_ms / speed));
        }
        feed_row(&row);
        n++;
        if (row.has_elapsed) {
            prev_elapsed = row.elapsed_ms;
            have_prev = true;
        }
        lv_timer_handler();
    }
    fclose(f);
    if (n > 0) {
        sleep_pump(REPLAY_TAIL_MS); /* giữ khung cuối để nhìn/chụp */
    }
    printf("[sim] replay '%s' fed %d row(s) in %lums\n", path, n,
           (unsigned long)(SDL_GetTicks() - start));
    if (n == 0) {
        fprintf(stderr, "[sim] replay '%s': 0 valid row(s) (%d skipped) -> FAIL\n", path, skipped);
        return 4;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *scenario = NULL;
    const char *replay = NULL;
    uint32_t exit_after_ms = DEFAULT_EXIT_AFTER_MS;
    uint32_t interval_ms = DEFAULT_INTERVAL_MS;
    bool exit_after_set = false;
    bool interval_set = false;
    double speed = 1.0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            scenario = argv[++i];
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            replay = argv[++i];
        } else if (strcmp(argv[i], "--exit-after") == 0 && i + 1 < argc) {
            exit_after_ms = (uint32_t)atoi(argv[++i]) * 1000u;
            exit_after_set = true;
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            interval_ms = (uint32_t)atoi(argv[++i]);
            interval_set = true;
        } else if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) {
            speed = atof(argv[++i]);
            if (speed <= 0.0) {
                speed = 1.0;
            }
        } else if (strcmp(argv[i], "--list") == 0) {
            /* In tên + số mốc từng kịch bản rồi thoát (không mở cửa sổ). */
            for (int j = 0; j < SIM_SCENARIO_COUNT; j++) {
                printf("%s (%d mốc)\n", k_sim_scenarios[j].name, k_sim_scenarios[j].n);
            }
            return 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: umt_dash_sim [--scenario <name>|--replay <jsonl>|--list] "
                   "[--exit-after <sec>] [--interval <ms>] [--speed <x>]\n");
            printf("Replay: doc payload V2 (d1..d6) hoac file tools/recorder (distances[]+valid[]); "
                   "mac dinh phat dung nhip elapsed_ms, --interval ep nhip co dinh.\n");
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
                 ? run_replay(replay, exit_after_ms, exit_after_set, interval_ms, interval_set, speed)
                 : run_scenario(scenario, exit_after_ms, interval_ms);

    lv_deinit();
    SDL_Quit();
    printf("[sim] exiting rc=%d (%s)\n", rc, rc == 0 ? "OK" : "FAIL");
    return rc;
}
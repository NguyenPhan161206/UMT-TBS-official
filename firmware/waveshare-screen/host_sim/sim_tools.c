/*
 * SPDX-PackageName: umt_host_sim
 *
 * sim_tools.c — công cụ kiểm thử cho umt_dash_sim (tách khỏi main.c để giữ R7 <= 400 dòng/file):
 *   - kịch bản UI: --click X,Y@ms (chuột ảo) và --snapshot FILE.bmp@ms (chụp màn hình trong tiến trình);
 *   - --stress-profiles N: đổi hồ sơ xe + dựng lại sơ đồ N lần, đo số widget và heap LVGL.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL2/SDL.h"

#include "lvgl.h"

#include "sim_tools.h"
#include "ui_dashboard.h"
#include "vehicle_settings.h"

#define SIM_W 800
#define SIM_H 480

/* ---- Kịch bản UI tự động: chuột ảo + chụp màn hình trong tiến trình --------------------------------
 * --click X,Y@MS          : nhấn giữ ~120 ms tại (X,Y) (toạ độ màn hình 800x480) vào thời điểm MS (ms từ lúc khởi động)
 * --snapshot FILE@MS      : lưu màn hình hiện tại ra FILE (BMP 24-bit) vào thời điểm MS
 * --expect-text TEXT@MS   : tại MS phải có label ĐANG HIỂN THỊ (bản thân + mọi cha không HIDDEN) chứa TEXT
 * --expect-no-text TEXT@MS: tại MS không label đang hiển thị nào chứa TEXT
 * Không dùng chuột/cửa sổ thật của máy dev nên chạy ổn định kể cả khi cửa sổ sim bị che. Cần nhịp bơm LVGL
 * đủ dày: dùng cùng --interval nhỏ (vd 50) và --exit-after lớn hơn thời điểm cuối. */
#define SCRIPT_MAX 24

typedef struct {
    sim_item_kind_t kind;
    int x, y;
    char path[200];                                /* đường dẫn ảnh (SNAPSHOT) hoặc chuỗi cần tìm (EXPECT_*) */
    uint32_t at_ms;
    bool started;
    bool done;
} script_item_t;

static script_item_t g_script[SCRIPT_MAX];
static int g_script_n = 0;
static uint32_t g_script_t0 = 0;
static struct { int x, y; bool pressed; } g_vptr;
static bool g_script_failed = false;               /* có --snapshot lưu lỗi / --expect-* sai → rc 1 */

static void vptr_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->point.x = g_vptr.x;
    data->point.y = g_vptr.y;
    data->state = g_vptr.pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

bool sim_script_add(sim_item_kind_t kind, const char *arg)
{
    if (g_script_n >= SCRIPT_MAX) {
        return false;
    }
    script_item_t *it = &g_script[g_script_n];
    memset(it, 0, sizeof(*it));
    it->kind = kind;
    const char *at = strrchr(arg, '@');
    if (at == NULL) {
        return false;
    }
    it->at_ms = (uint32_t)strtoul(at + 1, NULL, 10);
    if (kind != SIM_ITEM_CLICK) {
        size_t n = (size_t)(at - arg);
        if (n == 0 || n >= sizeof(it->path)) {
            return false;
        }
        memcpy(it->path, arg, n);
        it->path[n] = '\0';
    } else if (sscanf(arg, "%d,%d", &it->x, &it->y) != 2) {
        return false;
    }
    g_script_n++;
    return true;
}

static bool save_bmp(const char *path)
{
    /* Buffer 768 KB lấy từ malloc của host (pool LVGL chỉ 128 KB, giống firmware, không đủ chỗ cho ảnh chụp). */
    const uint32_t w = SIM_W, h = SIM_H, stride = SIM_W * 2u;
    uint8_t *raw = (uint8_t *)malloc(stride * h + 128u);
    if (raw == NULL) {
        return false;
    }
    lv_draw_buf_t snap;
    uint8_t *aligned = (uint8_t *)lv_draw_buf_align(raw, LV_COLOR_FORMAT_RGB565);
    if (lv_draw_buf_init(&snap, w, h, LV_COLOR_FORMAT_RGB565, stride, aligned, stride * h) != LV_RESULT_OK ||
        lv_snapshot_take_to_draw_buf(lv_screen_active(), LV_COLOR_FORMAT_RGB565, &snap) != LV_RESULT_OK) {
        free(raw);
        return false;
    }
    lv_draw_buf_t *buf = &snap;
    uint32_t row = (w * 3u + 3u) & ~3u;
    uint32_t size = 54u + row * h;
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        free(raw);
        return false;
    }
    uint8_t hdr[54] = {'B', 'M'};
    hdr[2] = (uint8_t)size; hdr[3] = (uint8_t)(size >> 8); hdr[4] = (uint8_t)(size >> 16); hdr[5] = (uint8_t)(size >> 24);
    hdr[10] = 54; hdr[14] = 40;
    hdr[18] = (uint8_t)w; hdr[19] = (uint8_t)(w >> 8);
    hdr[22] = (uint8_t)h; hdr[23] = (uint8_t)(h >> 8);
    hdr[26] = 1; hdr[28] = 24;
    fwrite(hdr, 1, sizeof(hdr), f);
    uint8_t *line = (uint8_t *)calloc(1, row);
    for (int32_t y = (int32_t)h - 1; line != NULL && y >= 0; y--) {
        const uint16_t *src = (const uint16_t *)(buf->data + (uint32_t)y * stride);
        for (uint32_t x = 0; x < w; x++) {
            uint16_t p = src[x];
            line[x * 3u + 0] = (uint8_t)(((p & 0x1Fu) * 255u) / 31u);          /* B */
            line[x * 3u + 1] = (uint8_t)((((p >> 5) & 0x3Fu) * 255u) / 63u);   /* G */
            line[x * 3u + 2] = (uint8_t)((((p >> 11) & 0x1Fu) * 255u) / 31u);  /* R */
        }
        fwrite(line, 1, row, f);
    }
    free(line);
    fclose(f);
    free(raw);
    return true;
}

/* Label có đang hiển thị không: bản thân và mọi cha đều không HIDDEN (trang ẩn không tính). */
static bool obj_visible(lv_obj_t *o)
{
    for (; o != NULL; o = lv_obj_get_parent(o)) {
        if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) {
            return false;
        }
    }
    return true;
}

static bool visible_text_contains(lv_obj_t *o, const char *needle)
{
    if (lv_obj_check_type(o, &lv_label_class) && obj_visible(o)) {
        const char *txt = lv_label_get_text(o);
        if (txt != NULL && strstr(txt, needle) != NULL) {
            return true;
        }
    }
    uint32_t n = lv_obj_get_child_count(o);
    for (uint32_t i = 0; i < n; i++) {
        if (visible_text_contains(lv_obj_get_child(o, i), needle)) {
            return true;
        }
    }
    return false;
}

static void script_tick_cb(lv_timer_t *t)
{
    (void)t;
    uint32_t now = SDL_GetTicks() - g_script_t0;
    for (int i = 0; i < g_script_n; i++) {
        script_item_t *it = &g_script[i];
        if (it->done || now < it->at_ms) {
            continue;
        }
        if (it->kind == SIM_ITEM_SNAPSHOT) {
            bool ok = save_bmp(it->path);
            printf("[sim] snapshot %s: %s\n", it->path, ok ? "ok" : "FAILED");
            g_script_failed = g_script_failed || !ok;
            it->done = true;
        } else if (it->kind == SIM_ITEM_EXPECT_TEXT || it->kind == SIM_ITEM_EXPECT_NO_TEXT) {
            bool want = (it->kind == SIM_ITEM_EXPECT_TEXT);
            bool found = visible_text_contains(lv_screen_active(), it->path);
            bool ok = (found == want);
            printf("[sim] expect%s-text '%s' @%u ms (checked at %u ms): %s\n", want ? "" : "-no", it->path,
                   (unsigned)it->at_ms, (unsigned)now, ok ? "ok" : "FAIL");
            g_script_failed = g_script_failed || !ok;
            it->done = true;
        } else if (!it->started) {
            g_vptr.x = it->x;
            g_vptr.y = it->y;
            g_vptr.pressed = true;
            it->started = true;
        } else if (now >= it->at_ms + 120u) {
            g_vptr.pressed = false;
            it->done = true;
        }
        if (!it->done && it->kind == SIM_ITEM_CLICK) {
            break;                                /* đang nhấn giữ: chưa xử lý mục kế (tránh nhấn chồng) */
        }
    }
}

static int count_objs(lv_obj_t *o)
{
    int n = 1;
    uint32_t c = lv_obj_get_child_count(o);
    for (uint32_t i = 0; i < c; i++) {
        n += count_objs(lv_obj_get_child(o, i));
    }
    return n;
}

/* --stress-profiles <N>: đổi hồ sơ xe + dựng lại sơ đồ N lần (id 1,2,3,1,2,3,...).
 * Đo được: (1) số widget của mỗi hồ sơ không đổi giữa các lần lặp (không dồn widget), (2) heap LVGL
 * không giảm quá 1 KB sau vòng lặp (không rò) và còn >= 30% trống, (3) về lại hồ sơ gốc ở cuối. FAIL = rc 1. */
int sim_run_profile_stress(int rounds)
{
    const size_t n_prof = vehicle_profile_count();
    if (n_prof == 0 || n_prof > 8) {
        fprintf(stderr, "[stress] unexpected profile count %u\n", (unsigned)n_prof);
        return 1;
    }

    /* Có dữ liệu thật ở vài slot để rebuild phải áp lại trạng thái (slot khác để trống = cũ/mất kết nối). */
    ui_dashboard_update_sensor(0, 25);
    ui_dashboard_update_sensor(2, 60);
    ui_dashboard_update_sensor(5, 180);

    int base_count[8] = {0};
    int rc = 0;

    /* Khởi động nóng: một vòng qua mọi hồ sơ để heap về trạng thái ổn định trước khi đo. */
    for (size_t i = 0; i < n_prof; i++) {
        vehicle_settings_select(vehicle_profile_get(i)->id);
        ui_dashboard_rebuild_vehicle();
        lv_timer_handler();
    }

    lv_mem_monitor_t m0, m1;
    lv_mem_monitor(&m0);

    /* Dư địa heap: pool LVGL của firmware cố định (hết pool = LV_ASSERT_MALLOC = treo). Phải còn >= 30% trống. */
    if ((uint64_t)m0.free_size * 100u < (uint64_t)m0.total_size * 30u) {
        fprintf(stderr, "[stress] FAIL: LVGL heap headroom %u%% < 30%% (free %u of %u B) — nâng CONFIG_LV_MEM_SIZE_KILOBYTES\n",
                (unsigned)((uint64_t)m0.free_size * 100u / m0.total_size), (unsigned)m0.free_size, (unsigned)m0.total_size);
        rc = 1;
    }

    for (int r = 0; r < rounds; r++) {
        size_t idx = (size_t)r % n_prof;
        uint8_t id = vehicle_profile_get(idx)->id;
        if (!vehicle_settings_select(id) || vehicle_profile_active()->id != id) {
            fprintf(stderr, "[stress] select(%u) failed at round %d\n", id, r);
            return 1;
        }
        ui_dashboard_rebuild_vehicle();
        lv_timer_handler();

        int n = count_objs(lv_screen_active());
        if (base_count[idx] == 0) {
            base_count[idx] = n;
        } else if (n != base_count[idx]) {
            fprintf(stderr, "[stress] widget count drift: profile %u had %d, now %d (round %d)\n",
                    id, base_count[idx], n, r);
            rc = 1;
            break;
        }
    }

    lv_mem_monitor(&m1);
    long leaked = (long)m0.free_size - (long)m1.free_size;
    printf("[stress] %d rebuilds over %u profiles; widgets per profile:", rounds, (unsigned)n_prof);
    for (size_t i = 0; i < n_prof; i++) {
        printf(" id%u=%d", vehicle_profile_get(i)->id, base_count[i]);
    }
    printf("; lvgl heap free %u -> %u (delta %ld B)\n", (unsigned)m0.free_size, (unsigned)m1.free_size, leaked);
    if (leaked > 1024) {
        fprintf(stderr, "[stress] FAIL: heap shrank by %ld B (> 1024)\n", leaked);
        rc = 1;
    }

    vehicle_settings_select(vehicle_profile_get(0)->id);
    ui_dashboard_rebuild_vehicle();
    lv_timer_handler();
    return rc;
}

void sim_script_start(void)
{
    if (g_script_n == 0) {
        return;
    }
    lv_indev_t *vptr = lv_indev_create();     /* chuột ảo cho --click (chạy song song với chuột thật) */
    lv_indev_set_type(vptr, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(vptr, vptr_read_cb);
    g_script_t0 = SDL_GetTicks();
    lv_timer_create(script_tick_cb, 20, NULL);
}

bool sim_script_failed(void)
{
    return g_script_failed;
}

int sim_parse_option(int argc, char **argv, int *i, sim_opts_t *o)
{
    const char *a = argv[*i];
    bool has_arg = (*i + 1 < argc);
    if (strcmp(a, "--profile") == 0 && has_arg) {
        o->profile_id = atoi(argv[++*i]);
        return 1;
    }
    if (strcmp(a, "--stress-profiles") == 0 && has_arg) {
        o->stress_rounds = atoi(argv[++*i]);
        return 1;
    }
    static const struct { const char *opt; sim_item_kind_t kind; const char *fmt; } k_items[] = {
        {"--click", SIM_ITEM_CLICK, "X,Y"},
        {"--snapshot", SIM_ITEM_SNAPSHOT, "FILE"},
        {"--expect-text", SIM_ITEM_EXPECT_TEXT, "TEXT"},
        {"--expect-no-text", SIM_ITEM_EXPECT_NO_TEXT, "TEXT"},
    };
    for (size_t k = 0; k < sizeof(k_items) / sizeof(k_items[0]); k++) {
        if (strcmp(a, k_items[k].opt) != 0 || !has_arg) {
            continue;
        }
        if (!sim_script_add(k_items[k].kind, argv[*i + 1])) {
            fprintf(stderr, "Bad %s argument: %s (expected %s@<ms>)\n", a, argv[*i + 1], k_items[k].fmt);
            return -1;
        }
        ++*i;
        return 1;
    }
    return 0;
}

void sim_print_usage_extra(void)
{
    printf("Them: [--profile <id>] [--stress-profiles <N>] [--click X,Y@ms]... [--snapshot FILE.bmp@ms]...\n"
           "      [--expect-text TEXT@ms]... [--expect-no-text TEXT@ms]...\n");
    printf("Profile id (ho so xe): ");
    for (size_t j = 0; j < vehicle_profile_count(); j++) {
        printf("%s%u=%s", j ? ", " : "", vehicle_profile_get(j)->id, vehicle_profile_get(j)->name);
    }
    printf("\n");
}

int sim_init_profile(const sim_opts_t *o)
{
    /* Không có store (NULL) nên không ghi gì xuống đĩa. Chọn trước khi dựng UI. */
    vehicle_settings_init(NULL);
    if (o->profile_id >= 0 && !(o->profile_id <= 255 && vehicle_settings_select((uint8_t)o->profile_id))) {
        fprintf(stderr, "Unknown profile id: %d\n", o->profile_id);
        return 2;
    }
    return 0;
}

void sim_print_heap(void)
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("[sim] lvgl heap after UI init: used %u of %u B (%u%% free)\n",
           (unsigned)(mon.total_size - mon.free_size), (unsigned)mon.total_size,
           (unsigned)(mon.total_size ? (mon.free_size * 100u) / mon.total_size : 0u));
}


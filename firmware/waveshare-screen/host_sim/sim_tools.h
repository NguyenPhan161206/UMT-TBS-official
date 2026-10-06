/*
 * SPDX-PackageName: umt_host_sim
 *
 * sim_tools.h — công cụ kiểm thử của umt_dash_sim (xem sim_tools.c).
 */
#pragma once

#include <stdbool.h>

typedef enum {
    SIM_ITEM_CLICK = 0,       /* --click X,Y@ms */
    SIM_ITEM_SNAPSHOT,        /* --snapshot FILE@ms */
    SIM_ITEM_EXPECT_TEXT,     /* --expect-text TEXT@ms : phải có label ĐANG HIỂN THỊ chứa TEXT */
    SIM_ITEM_EXPECT_NO_TEXT,  /* --expect-no-text TEXT@ms : không label đang hiển thị nào chứa TEXT */
} sim_item_kind_t;

/* arg: "X,Y@ms" (click) hoặc "<chuỗi>@ms" (còn lại). false nếu sai cú pháp hoặc quá SCRIPT_MAX mục. */
bool sim_script_add(sim_item_kind_t kind, const char *arg);
/* Gọi SAU khi tạo màn hình: tạo chuột ảo + timer chạy kịch bản (không làm gì nếu chưa có mục nào). */
void sim_script_start(void);
/* true nếu có --snapshot lưu lỗi hoặc --expect-* không đạt. */
bool sim_script_failed(void);
/* Kiểm thử dựng lại sơ đồ xe khi đổi hồ sơ; 0 = đạt, 1 = FAIL. */
int sim_run_profile_stress(int rounds);

typedef struct {
    int profile_id;       /* --profile <id>; -1 = mặc định (hồ sơ đầu của registry) */
    int stress_rounds;    /* --stress-profiles <N>; 0 = tắt */
} sim_opts_t;

/* Xử lý các tuỳ chọn --profile / --stress-profiles / --click / --snapshot / --expect-text / --expect-no-text
 * tại argv[*i] (tăng *i nếu có tham số).
 * Trả 1 = đã xử lý, 0 = không phải tuỳ chọn của sim_tools, -1 = sai cú pháp (đã in lỗi, main thoát mã 2). */
int sim_parse_option(int argc, char **argv, int *i, sim_opts_t *o);
/* Dòng usage bổ sung (danh sách id hồ sơ + tuỳ chọn của sim_tools). */
void sim_print_usage_extra(void);
/* vehicle_settings_init(NULL) rồi chọn hồ sơ theo o->profile_id. Trả 0 hoặc 2 (id không tồn tại, đã in lỗi). */
int sim_init_profile(const sim_opts_t *o);
/* In mức dùng heap LVGL (pool cố định như firmware: hết pool = LV_ASSERT_MALLOC = treo). */
void sim_print_heap(void);


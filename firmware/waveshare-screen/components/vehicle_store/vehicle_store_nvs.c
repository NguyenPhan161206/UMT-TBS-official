/*
 * SPDX-FileCopyrightText: 2026 Vehicle Warning System
 * SPDX-License-Identifier: MIT
 *
 * vehicle_store_nvs.c — Lưu/đọc blob cài đặt hồ sơ xe bằng NVS.
 */

#include "vehicle_store_nvs.h"
#include "nvs.h"
#include "esp_cpu.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "vehicle_store";

#define NVS_NAMESPACE "vehicle"
#define NVS_KEY       "settings"

static bool nvs_load_direct(uint8_t *buf, size_t cap, size_t *out_len)
{
    if (buf == NULL || out_len == NULL) {
        return false;
    }
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return false;                                  /* namespace chưa có = chưa lưu gì, không phải lỗi */
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open(read) failed: %s", esp_err_to_name(err));
        return false;
    }

    size_t len = cap;
    err = nvs_get_blob(h, NVS_KEY, buf, &len);
    nvs_close(h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_blob failed: %s", esp_err_to_name(err));
        return false;
    }
    *out_len = len;
    return true;
}

static bool nvs_save_direct(const uint8_t *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return false;
    }
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open(write) failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_set_blob(h, NVS_KEY, buf, len);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "save failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

static bool nvs_erase_direct(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open(erase) failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_erase_key(h, NVS_KEY);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = ESP_OK;                                  /* đã không có thì coi như đã xoá */
    }
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "erase failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

/* ---- Chạy thao tác NVS trên stack RAM nội --------------------------------------------------------
 * Đọc/ghi flash tắt cache; ESP-IDF assert stack của task gọi phải nằm trong DRAM nội
 * (spi_flash/cache_utils.c: esp_task_stack_is_sane_cache_disabled). Task LVGL có stack ở PSRAM
 * (main.c: stack_in_psram) mà nút Apply / chọn xe gọi save ngay trong event LVGL ⇒ khi stack không ở DRAM,
 * chạy thao tác trên một task tạm (stack nội) rồi chờ kết quả. Đồng bộ bằng semaphore riêng — KHÔNG dùng
 * task notification vì esp_lv_adapter đã dùng notify để đánh thức task LVGL. */

#define STORE_TASK_STACK 4096

typedef enum { OP_LOAD, OP_SAVE, OP_ERASE } store_op_t;

typedef struct {
    store_op_t op;
    uint8_t *rbuf;          /* LOAD */
    const uint8_t *wbuf;    /* SAVE */
    size_t len;             /* LOAD: dung lượng rbuf; SAVE: số byte wbuf */
    size_t *out_len;        /* LOAD */
    bool ok;
    SemaphoreHandle_t done;
} store_job_t;

static bool run_job(const store_job_t *job)
{
    switch (job->op) {
    case OP_LOAD:
        return nvs_load_direct(job->rbuf, job->len, job->out_len);
    case OP_SAVE:
        return nvs_save_direct(job->wbuf, job->len);
    default:
        return nvs_erase_direct();
    }
}

static void job_task(void *arg)
{
    store_job_t *job = arg;
    job->ok = run_job(job);
    xSemaphoreGive(job->done);
    vTaskDelete(NULL);
}

static bool run_on_internal_stack(store_job_t *job)
{
    if (esp_ptr_in_dram(esp_cpu_get_sp())) {
        return run_job(job);
    }
    job->done = xSemaphoreCreateBinary();
    if (job->done == NULL ||
        xTaskCreate(job_task, "vstore_io", STORE_TASK_STACK, job, uxTaskPriorityGet(NULL), NULL) != pdPASS) {
        ESP_LOGW(TAG, "cannot start NVS worker task (out of internal RAM?)");
        if (job->done != NULL) {
            vSemaphoreDelete(job->done);
        }
        return false;
    }
    xSemaphoreTake(job->done, portMAX_DELAY);
    vSemaphoreDelete(job->done);
    return job->ok;
}

static bool nvs_load(uint8_t *buf, size_t cap, size_t *out_len)
{
    store_job_t job = {.op = OP_LOAD, .rbuf = buf, .len = cap, .out_len = out_len};
    return run_on_internal_stack(&job);
}

static bool nvs_save(const uint8_t *buf, size_t len)
{
    store_job_t job = {.op = OP_SAVE, .wbuf = buf, .len = len};
    return run_on_internal_stack(&job);
}

static bool nvs_erase(void)
{
    store_job_t job = {.op = OP_ERASE};
    return run_on_internal_stack(&job);
}

static const vehicle_store_ops_t s_ops = {
    .load = nvs_load,
    .save = nvs_save,
    .erase = nvs_erase,
};

const vehicle_store_ops_t *vehicle_store_nvs_ops(void)
{
    return &s_ops;
}

/*
 * SPDX-PackageName: umt_host_sim (stub)
 * coreiot_client.h stub — R1: KHÔNG trả token thật. Chỉ dùng cho SYSTEM page
 * hiển thị broker + token MASKED giả.
 */
#pragma once

#define COREIOT_DEFAULT_BROKER_URI "app.coreiot.io (host_sim)"

static const char *coreiot_broker_uri_display(void)
{
    return COREIOT_DEFAULT_BROKER_URI;
}

/* Giá trị giả — tuyệt đối không phải credential thật (R1): chuỗi này giúp mắt
 * người vẫn thấy SYSTEM page render đúng cấu trúc mà không lộ secret nào. */
static const char *coreiot_token_display(void)
{
    return "sim.masked-placeholder-value-0000";
}
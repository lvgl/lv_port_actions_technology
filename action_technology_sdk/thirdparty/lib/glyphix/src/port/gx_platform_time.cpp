/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "kernel.h"
#include "platform/gx_os.h"
#include <ctime>
#include <sys/time.h>
#include <logging/log.h>
#include "mas_rpc.h"
#include <cassert>
#include "gx_logger.h"
#include "sys/time_units.h"
#include <drivers/rtc.h>

namespace gx {
namespace os {

static int g_mas_timezone = 8; 

uint32_t clock_ms() {
    return sys_clock_tick_get();
}

uint64_t clock_us() {
    while (true) {
        auto ms1 = static_cast<uint64_t>(sys_clock_tick_get()) * 1000;
        auto us = k_cyc_to_us_floor32(k_cycle_get_32()) % 1000;
        auto ms2 = static_cast<uint64_t>(sys_clock_tick_get()) * 1000;
        if (ms1 == ms2)
            return ms1 + us;
    }
}

void delay_ms(int time) { 
    k_sleep(K_MSEC(time));
}

int64_t timestamp_ms() {
    timeval tv{};
    const struct device *rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
    struct rtc_time rtc_time = {0};
    int ret = rtc_get_time(rtc_dev, &rtc_time);
    if (!ret) {
        uint32_t tv_sec;
        rtc_tm_to_time(&rtc_time, &tv_sec);
        tv.tv_sec = tv_sec;
        tv.tv_usec = rtc_time.tm_ms * 1000;
    }
    return ((int64_t)tv.tv_sec) * 1000 + ((int64_t)tv.tv_usec) /1000 - g_mas_timezone*60*60*1000;
}

int timezone_offset(int64_t timestamp) {
    return -(g_mas_timezone * 3600);
}

struct mas_rpc_blob* time_sync_svc(struct mas_rpc_blob* input)
{
    cJSON* root = nullptr;
    const cJSON* item_timestamp = nullptr;
    const cJSON* item_timezone = nullptr;
    const cJSON* item_minute_offset = nullptr;
    time_t timestamp = 0;
    int timezone = 0;
    int minute_offset = 0;
    struct mas_rpc_blob* output_blob = mas_rpc_create_result_blob(GX_ERROR);
    assert(output_blob);

    // the input object is empty, return directly.
    if ((input == nullptr) || (input->buf == nullptr))
    {
        LogError() << "time sync failed, input is null.";
        return output_blob;
    }
    // input object converted to json object.
    
    root = mas_rpc_blob_to_json(input, root);
    if (root == nullptr)
    {
        LogError() << "time sync failed, format input to json failed.";
        return output_blob;
    }
    // root is not empty.
    item_timestamp = cJSON_GetObjectItem(root, "timestamp");
    item_timezone = cJSON_GetObjectItem(root, "timezone");
    item_minute_offset = cJSON_GetObjectItem(root, "minute_offset");
    // is the json object correct.
    if ((item_timestamp == nullptr) || (cJSON_IsNumber(item_timestamp) == false))
    {
        gx::LogError() << "time sync failed, args timestamp is not a number.";
        cJSON_Delete(root);
        return output_blob;
    }
    else
    {
        timestamp = (time_t)item_timestamp->valueint;
    }
    if ((item_timezone == nullptr) || (cJSON_IsNumber(item_timezone) == false))
    {
        gx::LogError() << "time sync failed, args timezone is not a number.";
        cJSON_Delete(root);
        return output_blob;
    }
    else
    {
        timezone = (int)item_timezone->valueint;
        g_mas_timezone = timezone;
    }
    if ((item_minute_offset == nullptr) || (cJSON_IsNumber(item_minute_offset) == false))
    {
        minute_offset = 0;
    }
    else
    {
        minute_offset = (int)item_minute_offset->valueint;
    }
    gx::LogInfo() << "time sync svc timestamp:" << timestamp << ", timezone:" << timezone << ", minute offset:" <<  minute_offset;
    // sync the time.

    struct rtc_time rtc_time = {0};
    rtc_time_to_tm(timestamp + g_mas_timezone*60*60,&rtc_time);
    const struct device *rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
    rtc_set_time(rtc_dev,&rtc_time);

    // release the json object.
    cJSON_Delete(root);
    // return result blob
    mas_rpc_blob_free(output_blob);
    output_blob = mas_rpc_create_result_blob(GX_EOK);
    assert(output_blob);
    return output_blob;
}

extern "C" void gx_time_sync_init(){
    gx::LogInfo() << "register time service";
    mas_rpc_svc_register("time_sync_svc", time_sync_svc);
}

} // namespace os
} // namespace gx

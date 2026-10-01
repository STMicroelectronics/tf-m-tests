/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis_os2.h>
#include <test_app.h>
#include <tfm_nsid_manager.h>
#include <tfm_log.h>

#include <tfm_platform_system.h>
#include <uapi/tfm_ioctl_api.h>
#include <errno.h>

#define MSEC_PER_SEC		1000L

#define TIMEOUT_MIN_MS      1 * MSEC_PER_SEC

void wdt_task(void *argument)
{
    struct wdt_info wdt_info;
    uint32_t ping_ms, tk_ping, tk_freq;
    int err;

    UNUSED_VARIABLE(argument);

    err = tfm_platform_wdt_info(&wdt_info);
    if (err != TFM_PLATFORM_ERR_SUCCESS) {
        LOG_MSG("[NS] [ERR] watchdog get info fail:%d\r\n", err);
        goto out_err;
    }

    if (wdt_info.status == WDT_DISABLED) {
        err = tfm_platform_wdt_start();
        if (err != TFM_PLATFORM_ERR_SUCCESS) {
            LOG_MSG("[NS] [ERR] watchdog start fail:%d\r\n", err);
            goto out_err;
        }
    }

    if (wdt_info.timeout < TIMEOUT_MIN_MS) {
        LOG_MSG("[NS] [ERR] watchdog timeout too small\r\n");
        goto out_err;
    }

    tk_freq = osKernelGetTickFreq();
    ping_ms = (wdt_info.timeout / 4) * 3;
    tk_ping = (tk_freq * ping_ms) / MSEC_PER_SEC;

    LOG_MSG("[NS] [INF] watchdog timeout: %dms ping: %dms\r\n",
            wdt_info.timeout, ping_ms);

    while (true) {
        err = tfm_platform_wdt_ping();
        if (err != TFM_PLATFORM_ERR_SUCCESS)
            LOG_MSG("[NS] [ERR] watchdog ping fail:%d\r\n", err);

        osDelay(tk_ping);
    }

out_err:
    osThreadExit();
}

static osThreadFunc_t wdt_thread_func = wdt_task;
static const osThreadAttr_t wdt_thread_attr = {
    .name = "watchdog_thread",
    .stack_size = 1024U,
    .tz_module = ((TZ_ModuleId_t)TFM_DEFAULT_NSID),
    .priority = osPriorityHigh,
};

int wdt_init(void)
{
    osThreadNew(wdt_thread_func, NULL, &wdt_thread_attr);

    return 0;
}

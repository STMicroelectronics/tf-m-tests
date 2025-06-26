/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis_os2.h>
#include <cmsis_compiler.h>
#include <test_app.h>
#include <tfm_nsid_manager.h>
#include <tfm_log.h>

#include <tfm_platform_system.h>
#include <uapi/tfm_ioctl_api.h>
#include <errno.h>

#define MSEC_PER_SEC		1000L

#define COPRO_TIMEOUT_MS	1 * MSEC_PER_SEC

__WEAK const char *cpu_status_str[] = {
	"offline",
	"suspended",
	"started",
	"running",
	"crashed",
	"unknow",
};

static int32_t copro_wait_status(struct cpu_info_res *cpu_info,
				 uint32_t timeout_ms, int32_t status)
{
	uint32_t tk_timeout, tk_start, tk_curr;
	int err;

	/* Polling status */
	tk_timeout = (osKernelGetTickFreq() * timeout_ms) / MSEC_PER_SEC;
	tk_start = osKernelGetTickCount();

	do {
		err = tfm_platform_cpu_info(0, cpu_info);
		if (err != TFM_PLATFORM_ERR_SUCCESS) {
			cpu_info->status = CPU_LAST;
			break;
		}

		if (cpu_info->status == status)
			break;

		tk_curr = osKernelGetTickCount();

	} while (((tk_curr - tk_start) < tk_timeout));

	return (cpu_info->status == status) ? 0 : -ETIMEDOUT;
}

void copro_start_task(void *argument)
{
	struct cpu_info_res cpu_info;
	int32_t status, err;

	UNUSED_VARIABLE(argument);

	err = tfm_platform_cpu_info(0, &cpu_info);
	if (err != TFM_PLATFORM_ERR_SUCCESS) {
		LOG_MSG("[NS] [ERR] get cpu 0 info fail: %d\r\n", err);
		goto out;
	}

	if (cpu_info.status < 0 || cpu_info.status >= CPU_LAST) {
		LOG_MSG("[NS] [ERR] cpu %s error %d\r\n",
                        cpu_info.name, cpu_info.status);
		goto out;
	}

	if (cpu_info.status != CPU_OFFLINE) {
		LOG_MSG("[NS] [INF] cpu %s already started\r\n", cpu_info.name);
		LOG_MSG("[NS] [INF] cpu %s status: %s\r\n",
			cpu_info.name, cpu_status_str[cpu_info.status]);
		goto out;
	}

	err = tfm_platform_cpu_start(0, &status);
	if (err != TFM_PLATFORM_ERR_SUCCESS) {
		LOG_MSG("[NS] [ERR] cpu %s start fail err: %d\r\n", cpu_info.name, err);
		goto out;
	}

	err = copro_wait_status(&cpu_info, COPRO_TIMEOUT_MS, CPU_RUNNING);
	if (err) {
		LOG_MSG("[NS] [ERR] cpu %s not running status: %s\r\n",
			cpu_info.name, cpu_status_str[status]);

		err = tfm_platform_cpu_stop(0, &status);
		if (err)
			LOG_MSG("[NS] [ERR] cpu %s stop fail err: %d\r\n", cpu_info.name, err);

		goto out;
	}

	LOG_MSG("[NS] [INF] cpu %s started\r\n", cpu_info.name);

out:
	osThreadExit();
}

static osThreadFunc_t copro_thread_func = copro_start_task;
static const osThreadAttr_t copro_thread_attr = {
    .name = "copro_thread",
    .stack_size = 1024U,
    .tz_module = ((TZ_ModuleId_t)TFM_DEFAULT_NSID),
    .priority = osPriorityHigh,
};

int copro_init(void)
{
	osThreadNew(copro_thread_func, NULL, &copro_thread_attr);

	return 0;
}

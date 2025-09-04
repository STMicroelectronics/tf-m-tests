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

#include "copro_task.h"

#define MSEC_PER_SEC		1000L

#define COPRO_TIMEOUT_MS	1 * MSEC_PER_SEC

#define COPRO_ID_A35		0

__WEAK const char *cpu_status_str[] = {
	"offline",
	"suspended",
	"started",
	"running",
	"crashed",
	"unknow",
};

static int32_t copro_wait_status(uint32_t cpu_id, struct cpu_info_res *cpu_info,
				 uint32_t timeout_ms, int32_t status)
{
	uint32_t tk_timeout, tk_start, tk_curr;
	int err;

	/* Polling status */
	tk_timeout = (osKernelGetTickFreq() * timeout_ms) / MSEC_PER_SEC;
	tk_start = osKernelGetTickCount();

	do {
		err = tfm_platform_cpu_info(cpu_id, cpu_info);
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

typedef enum tfm_platform_err_t (*cpu_cmd_fn_t)(uint32_t, int32_t *);

static void _copro_cmd_wait(cpu_cmd_fn_t cpu_fn, uint32_t cpu_id, int32_t wait_status)
{
        struct cpu_info_res cpu_info;
	int32_t status, err;

	if (!cpu_fn)
		return;

	err = tfm_platform_cpu_info(cpu_id, &cpu_info);
	if (err != TFM_PLATFORM_ERR_SUCCESS) {
		LOG_MSG("[NS] [COPRO] [ERR] get cpu 0 info fail: %d\r\n", err);
		return;
	}

	if (cpu_info.status < 0 || cpu_info.status >= CPU_LAST) {
		LOG_MSG("[NS] [COPRO] [ERR] cpu %s error %d\r\n",
                        cpu_info.name, cpu_info.status);
		return;
	}

	if (cpu_info.status == wait_status) {
		LOG_MSG("[NS] [COPRO] [INF] cpu %s is already %s\r\n",
			cpu_info.name, cpu_status_str[cpu_info.status]);
		return;
	}

	err = cpu_fn(cpu_id, &status);
	if (err != TFM_PLATFORM_ERR_SUCCESS) {
		LOG_MSG("[NS] [COPRO] [ERR] cpu %s cmd fail: %d\r\n", cpu_info.name, err);
		return;
	}

	err = copro_wait_status(cpu_id, &cpu_info, COPRO_TIMEOUT_MS, wait_status);
	if (err == -ETIMEDOUT) {
		LOG_MSG("[NS] [COPRO] [ERR] cpu %s timeout\r\n", cpu_info.name);
		return;
	}

	LOG_MSG("[NS] [COPRO] [INF] cpu %s now %s\r\n",
		cpu_info.name, cpu_status_str[cpu_info.status]);
}

static void _copro_start(void)
{
	_copro_cmd_wait(tfm_platform_cpu_start, COPRO_ID_A35, CPU_RUNNING);
}

static void _copro_stop(void)
{
	_copro_cmd_wait(tfm_platform_cpu_stop, COPRO_ID_A35, CPU_OFFLINE);
}

void copro_ctrl_task(void *argument)
{
	uint32_t cmd;

	UNUSED_VARIABLE(argument);

	for (;;) {
		cmd = osThreadFlagsWait(COPRO_START | COPRO_STOP, osFlagsWaitAny, osWaitForever);
		if (cmd == COPRO_START)
			_copro_start();
		else if (cmd == COPRO_STOP)
			_copro_stop();
		else
			LOG_MSG("[NS] [COPRO] [ERR] unknown cmd\r\n");
	}
}

osThreadId_t tid_copro = NULL;

static osThreadFunc_t copro_thread_func = copro_ctrl_task;
static const osThreadAttr_t copro_thread_attr = {
    .name = "copro_thread",
    .stack_size = 1024U,
    .tz_module = ((TZ_ModuleId_t)TFM_DEFAULT_NSID),
    .priority = osPriorityHigh,
};

int copro_init(void)
{
	tid_copro = osThreadNew(copro_thread_func, NULL, &copro_thread_attr);

	osThreadFlagsSet(tid_copro, COPRO_START);

	return 0;
}

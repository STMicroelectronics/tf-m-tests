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

#include <stm32mp2xx_hal_cortex.h>
#include <stm32mp2xx_hal_pwr.h>
#include <tfm_platform_system.h>
#include <lib/mmio.h>
#include <uapi/tfm_ioctl_api.h>
#include <uapi/tfm_pm_api.h>
#include <errno.h>

#include "copro_task.h"

#define MSEC_PER_SEC		1000L

#define COPRO_TIMEOUT_MS	1 * MSEC_PER_SEC

#define COPRO_ID_A35		0

typedef enum tfm_platform_err_t (*cpu_cmd_fn_t)(uint32_t, int32_t *);

__WEAK const char *cpu_status_str[] = {
	"offline",
	"suspended",
	"started",
	"running",
	"crashed",
	"unknow",
};

typedef struct ns_context {
	uint32_t VTOR_NS;
	uint32_t MSPLIM_NS;
	uint32_t PSPLIM_NS;
	uint32_t CONTROL_NS;
	uint32_t MSP_NS;
	uint32_t PSP_NS;
	uint32_t primask_ns;
} ns_context_t;

static ns_context_t saved_context;

static void save_it_status(void)
{
	/* Save Vector */
	saved_context.VTOR_NS = SCB->VTOR;
	saved_context.primask_ns = __get_PRIMASK();
}

static void restore_it_status(void)
{
	io_write32((uint32_t)&SCB->VTOR, saved_context.VTOR_NS);
	__DSB();
	__ISB();
	__set_PRIMASK(saved_context.primask_ns);
}

static void save_stack(void)
{
	saved_context.MSPLIM_NS =  __get_MSPLIM();
	saved_context.PSPLIM_NS =  __get_PSPLIM();
	saved_context.CONTROL_NS = __get_CONTROL();
	saved_context.MSP_NS = __get_MSP();
	saved_context.PSP_NS = __get_PSP();
}

static void restore_stack(void)
{
	__set_MSPLIM(saved_context.MSPLIM_NS);
	__set_PSPLIM(saved_context.PSPLIM_NS);
	__set_MSP(saved_context.MSP_NS);
	__set_PSP(saved_context.PSP_NS);
	__set_CONTROL(saved_context.CONTROL_NS);
}

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

	} while ((tk_curr - tk_start) < tk_timeout);

	return (cpu_info->status == status) ? 0 : -ETIMEDOUT;
}

static int32_t _copro_wait_D1_state(uint32_t timeout_ms, uint32_t state)
{
	uint32_t tk_timeout, tk_start, tk_curr;

	tk_timeout = (osKernelGetTickFreq() * timeout_ms) / MSEC_PER_SEC;
	tk_start = osKernelGetTickCount();

	do {
		osDelay(1);
		if (HAL_PWR_D1State() == state) {
			return 0;
		}

		tk_curr = osKernelGetTickCount();

	} while ((tk_curr - tk_start) < tk_timeout);

	return -ETIMEDOUT;
}

static int32_t _copro_cmd_wait(cpu_cmd_fn_t cpu_fn, uint32_t cpu_id, int32_t wait_status)
{
	struct cpu_info_res cpu_info;
	int32_t status, err;

	if (!cpu_fn)
		return -TFM_PLATFORM_ERR_INVALID_PARAM;

	err = tfm_platform_cpu_info(cpu_id, &cpu_info);
	if (err != TFM_PLATFORM_ERR_SUCCESS) {
		LOG_MSG("[NS] [COPRO] [ERR] get cpu 0 info fail: %d\r\n", err);
		return err;
	}

	if (cpu_info.status < 0 || cpu_info.status >= CPU_LAST) {
		LOG_MSG("[NS] [COPRO] [ERR] cpu %s error %d\r\n",
			cpu_info.name, cpu_info.status);
		return TFM_PLATFORM_ERR_SYSTEM_ERROR;
	}

	if (cpu_info.status == wait_status) {
		LOG_MSG("[NS] [COPRO] [INF] cpu %s is already %s\r\n",
			cpu_info.name, cpu_status_str[cpu_info.status]);
		return TFM_PLATFORM_ERR_SYSTEM_ERROR;
	}

	err = cpu_fn(cpu_id, &status);
	if (err != TFM_PLATFORM_ERR_SUCCESS) {
		LOG_MSG("[NS] [COPRO] [ERR] cpu %s cmd fail: %d\r\n", cpu_info.name, err);
		return err;
	}

	err = copro_wait_status(cpu_id, &cpu_info, COPRO_TIMEOUT_MS, wait_status);
	if (err) {
		LOG_MSG("[NS] [COPRO] [ERR] cpu %s error %d\r\n", cpu_info.name, err);
		return err;
	}

	LOG_MSG("[NS] [COPRO] [INF] cpu %s now %s\r\n",
		cpu_info.name, cpu_status_str[cpu_info.status]);

	return 0;
}

static void _copro_start(void)
{
	int32_t err = _copro_cmd_wait(tfm_platform_cpu_start, COPRO_ID_A35, CPU_RUNNING);

	if (err)
		LOG_MSG("[NS] [COPRO] [ERR] CPU START failure (%d)\r\n", err);
}

static void _copro_stop(void)
{
	int32_t err = _copro_cmd_wait(tfm_platform_cpu_stop, COPRO_ID_A35, CPU_OFFLINE);

	if (err)
		LOG_MSG("[NS] [COPRO] [ERR] CPU STOP failure (%d)\r\n", err);
}

static void _copro_suspend(void)
{

	uint32_t rcc_c1bootrsts;
	uint32_t rcc_c2bootrsts;
	uint32_t pwr_cpu2cr;
	uint32_t sleep;
	int32_t err;

	if (_copro_wait_D1_state(COPRO_TIMEOUT_MS, PWR_D1_DSTANDBY)) {
		LOG_MSG("[NS] [COPRO] [ERR] D1 DStandby timeout\r\n");
		return;
	}

	err = _copro_cmd_wait(tfm_platform_cpu_suspend, COPRO_ID_A35, CPU_SUSPENDED);
	if (err) {
		LOG_MSG("[NS] [COPRO] [ERR] CPU SUSPEND failure (%d)\r\n", err);
		return;
	}

	/* Suspend RTX thread scheduler, including Ticks */
	sleep = osKernelSuspend();
	if (sleep) {
		save_it_status();
		save_stack();

		err = psa_pm_suspend(PM_STOP2);

		restore_stack();
		restore_it_status();

		if (err != PSA_SUCCESS)
			LOG_MSG("[NS] [COPRO] [INF] PM SUSPEND failure (%d)\r\n", err);

		/* TODO = After Wake-up adjust with cycles slept */
	}
	osKernelResume(sleep);

	pwr_cpu2cr = READ_REG(PWR->CPU2CR);
	rcc_c1bootrsts = READ_REG(RCC->C1BOOTRSTSCLRR);
	rcc_c2bootrsts = READ_REG(RCC->C2BOOTRSTSCLRR);

	if (rcc_c2bootrsts & RCC_C2BOOTRSTSCLRR_D2STBYRSTF)
		LOG_MSG("[NS] [COPRO] [INF] Standby exit\r\n");
	else if (pwr_cpu2cr & PWR_CPU2CR_STOPF)
		LOG_MSG("[NS] [COPRO] [INF] Stop exit\r\n");
	else if (rcc_c1bootrsts & RCC_C1BOOTRSTSCLRR_D1STBYRSTF)
		LOG_MSG("[NS] [COPRO] [INF] Run2/D1 DStandby exit\r\n");
	else
		LOG_MSG("[NS] [COPRO] [INF] Suspend aborted\r\n");

	err = _copro_cmd_wait(tfm_platform_cpu_resume, COPRO_ID_A35, CPU_RUNNING);
	if (err) {
		LOG_MSG("[NS] [COPRO] [ERR] CPU RESUME failure (%d)\r\n", err);
		_copro_stop();
		_copro_start();
	}
}

void copro_ctrl_task(void *argument)
{
	uint32_t cmd;

	UNUSED_VARIABLE(argument);

	for (;;) {
		cmd = osThreadFlagsWait(COPRO_START | COPRO_STOP | COPRO_SUSPEND,
					osFlagsWaitAny, osWaitForever);
		if (cmd == COPRO_START)
			_copro_start();
		else if (cmd == COPRO_STOP)
			_copro_stop();
		else if (cmd == COPRO_SUSPEND)
			_copro_suspend();
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

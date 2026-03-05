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

/*
 * The low power mode can be limited by CA35-NS directly only for TEST
 * with backup register 107, the BKR107 expected value is 0xFFFFFFXX
 * - XX = the max allowed mode with enum pm_suspend_mode_t
 * - XX = 0xFF when pm mode is not allowed
 * No impact for cold boot value = 0x00000000 or other strange value
 */
#define BKPR_PM_DISABLED	0xFFFFFFFF
#define BKPR_PM_MASK		0x000000FF

/* EXTI1 CPU2 wake-up with interrupt mask register */
#define EXTI1_C2IMR1_GPIO		GENMASK_32(15, 0)
#define EXTI1_C2IMR1_PVD		BIT_32(16)
#define EXTI1_C2IMR1_PVM		BIT_32(17)
#define EXTI1_C2IMR2_WKUP_MASK		GENMASK_32(57 % 32, 52 % 32)
#define EXTI1_C2IMR3_C1SEV		BIT_32(65 % 32)

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

static bool pm_is_allowed = true;
static enum pm_suspend_mode_t pm_allowed = PM_STANDBY1;

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

static enum pm_suspend_mode_t _copro_get_pm_suspend_mode(void)
{
	uint32_t cpu1cr = PWR->CPU1CR;
	uint32_t c2imr1 = EXTI1->C2IMR1;
	uint32_t c2imr2 = EXTI1->C2IMR2;
	uint32_t c2imr3 = EXTI1->C2IMR3;
	enum pm_suspend_mode_t max_mode = PM_STANDBY1;

	/* Verify the max level supported according to the activated EXTI1 */
	if ((c2imr1 & (EXTI1_C2IMR1_GPIO | EXTI1_C2IMR1_PVD | EXTI1_C2IMR1_PVM)) != 0U) {
		max_mode = PM_LPLV_STOP2;
	}

	/* Wake-up pin are connected directly to PWR */
	if (((c2imr1 & ~(EXTI1_C2IMR1_GPIO | EXTI1_C2IMR1_PVD | EXTI1_C2IMR1_PVM)) != 0U) ||
	    ((c2imr2 & ~EXTI1_C2IMR2_WKUP_MASK) != 0U) ||
	    ((c2imr3 & ~EXTI1_C2IMR3_C1SEV) != 0U)) {
		max_mode = PM_LP_STOP2;
	}

	/* Standby not allowed by CA35 */
	if ((cpu1cr & PWR_CPU1CR_PDDS_D2) == 0U) {
		max_mode = PM_LP_STOP2;
	}

	/* The low power mode are limited by other threads */
	if (max_mode > pm_allowed)
		max_mode = pm_allowed;

	/* Trace to debug low power mode restriction */
	LOG_MSG("[NS] [COPRO] [INF] max_mode=%x C2IMR1=%x C2IMR2=%x C2IMR3=%x CPU1CR=%x allowed=%x\n",
		max_mode, c2imr1, c2imr2, c2imr3, cpu1cr, pm_allowed);

	return max_mode;
}

static void _copro_suspend(void)
{
	enum pm_suspend_mode_t mode;
	uint32_t rcc_c1bootrsts;
	uint32_t rcc_c2bootrsts;
	uint32_t pwr_cpu2cr;
	uint32_t sleep;
	int32_t err;

	if (_copro_wait_D1_state(COPRO_TIMEOUT_MS, PWR_D1_DSTANDBY)) {
		LOG_MSG("[NS] [COPRO] [ERR] D1 DStandby timeout\r\n");
		return;
	}

	/* Only for TEST: the low power mode is limited by CA35-NS with BKP107R value 0xFFFFFFXX */
	if ((TAMP->BKP107R & ~BKPR_PM_MASK) == ~BKPR_PM_MASK) {
		if (TAMP->BKP107R == BKPR_PM_DISABLED) {
			pm_is_allowed = false;
			LOG_MSG("[NS] [COPRO] [INF] CPU SUSPEND disabled by CA35\r\n");
		} else {
			pm_is_allowed = true;
			pm_allowed = (enum pm_suspend_mode_t)(TAMP->BKP107R & BKPR_PM_MASK);
			LOG_MSG("[NS] [COPRO] [INF] CA35 force mode=%x\r\n", pm_allowed);
		}
	}

	if (!pm_is_allowed) {
		LOG_MSG("[NS] [COPRO] [INF] CPU SUSPEND refused, low power limited at D1 DStandby\r\n");
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

		mode = _copro_get_pm_suspend_mode();
		LOG_MSG("[NS] [COPRO] [INF] PM SUSPEND with mode %d\r\n",mode);
		err = psa_pm_suspend(mode);

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
		cmd = osThreadFlagsWait(COPRO_START | COPRO_STOP | COPRO_SUSPEND |
					COPRO_PM_STOP2 | COPRO_PM_LP_STOP2 |
					COPRO_PM_LPLV_STOP2 | COPRO_PM_STANDBY1,
					osFlagsWaitAny, osWaitForever);
		if (cmd == COPRO_START) {
			_copro_start();
		} else if (cmd == COPRO_STOP) {
			_copro_stop();
		} else if (cmd == COPRO_SUSPEND) {
			_copro_suspend();
		} else if (cmd == COPRO_PM_STOP2) {
			pm_is_allowed = true;
			pm_allowed = PM_STOP2;
		} else if (cmd == COPRO_PM_LP_STOP2) {
			pm_is_allowed = true;
			pm_allowed = PM_LP_STOP2;
		} else if (cmd == COPRO_PM_LPLV_STOP2) {
			pm_is_allowed = true;
			pm_allowed = PM_LPLV_STOP2;
		} else if (cmd == COPRO_PM_STANDBY1) {
			pm_is_allowed = true;
			pm_allowed = PM_STANDBY1;
		} else if (cmd == COPRO_PM_DISABLED) {
			pm_is_allowed = false;
		} else
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

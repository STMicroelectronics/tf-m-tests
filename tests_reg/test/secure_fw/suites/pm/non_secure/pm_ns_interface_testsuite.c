/*
 * Copyright (C) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cmsis_os2.h>
#include <stdio.h>
#include <errno.h>

#include <test_framework.h>
#include <uapi/tfm_pm_api.h>
#include <psa/error.h>
#include <copro_task.h>
#include <util_macro.h>

int32_t _set_wakeup_source(void)
{
	/* TODO
	 * set a wake up source to test power resume device
	 */
	return 0;
}

static void delay(unsigned int second)
{
	uint32_t tk_freq = osKernelGetTickFreq();
	osDelay(second * tk_freq);
}

static void _suspend_resume(uint32_t flag, struct test_result_t *ret)
{
	int32_t err;

	if (!IS_ENABLED(TFM_PLATFORM_CPU_API)) {
		TEST_LOG("CPU_API require for low power tests\r\n");
		goto err;
	}

	osThreadFlagsSet(tid_copro, COPRO_STOP);

	err = _set_wakeup_source();
	if (err) {
		TEST_LOG("set a wakeup source failed:%d\r\n", err);
		goto err;
	}

	osThreadFlagsSet(tid_copro, flag);

	osThreadFlagsSet(tid_copro, COPRO_SUSPEND);

	/* Waiting Low Power mode execution */
	delay(1);

	osThreadFlagsSet(tid_copro, COPRO_START);

	ret->val = TEST_PASSED;
	return;
err:
	ret->val = TEST_FAILED;
}

void pm_suspend_stop2(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_STOP2, ret);
}

void pm_suspend_lp_stop2(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_LP_STOP2, ret);
}

void pm_suspend_lp_lv_stop2(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_LPLV_STOP2, ret);
}

void pm_suspend_standby(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_STANDBY1, ret);
}

void pm_power_off(struct test_result_t *ret)
{
	/* this function not return and must power off the platform */
	psa_pm_power_off();
	ret->val = TEST_PASSED;
}

static struct test_t pm_tests[] = {
#ifdef TEST_NS_PM_SUSPEND
	{&pm_suspend_stop2, "TFM_NS_PM_TEST_0101", "try stop2 suspend mode"},
	{&pm_suspend_lp_stop2, "TFM_NS_STM32_PM_TEST_0102", "try lp_stop2 suspend mode"},
	{&pm_suspend_lp_lv_stop2, "TFM_NS_STM32_PM_TEST_0103", "try lp lv stop2 suspend mode"},
	{&pm_suspend_standby, "TFM_NS_STM32_PM_TEST_0104", "try standby suspend mode"},
#endif
#ifdef TEST_NS_PM_OFF
	{&pm_power_off, "TFM_NS_STM32_PM_TEST_0201", "call platform power off"},
#endif
};

void register_testsuite_ns_pm_interface(struct test_suite_t *p_test_suite)
{
    uint32_t list_size;

    list_size = (sizeof(pm_tests) / sizeof(pm_tests[0]));

    set_testsuite("PM power management NS interface tests (TFM_NS_PM_XXXX)",
                  pm_tests, list_size, p_test_suite);
}

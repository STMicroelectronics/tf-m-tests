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

#include <stm32mp2xx_hal_cortex.h>
#include <stm32mp2xx_hal_rtc.h>
#include <stm32mp2xx_hal_rtc_ex.h>
#include <stm32mp2xx_hal_exti.h>

#define DEFAULT_IRQ_PRIO	1U

RTC_HandleTypeDef RtcHandle;

/* RTC interrupt handler */
void RTC_IRQHandler(void)
{
	HAL_RTC_AlarmIRQHandler(&RtcHandle);
}

static void RTC_SetAlarm(uint8_t delay)
{
	RTC_TimeTypeDef sCurrentTime = {0};
	RTC_DateTypeDef sCurrentDate = {0};
	RTC_AlarmTypeDef sAlarm = {0};
	uint8_t new_min, new_sec;

	/* Read current time */
	HAL_RTC_GetTime(&RtcHandle, &sCurrentTime, RTC_FORMAT_BCD);
	HAL_RTC_GetDate(&RtcHandle, &sCurrentDate, RTC_FORMAT_BCD);

	/* Configure alarm time */
	new_sec = RTC_Bcd2ToByte(sCurrentTime.Seconds) + delay;
	new_min = RTC_Bcd2ToByte(sCurrentTime.Minutes);
	if (new_sec >= 60) {
		new_sec -= 60;
		new_min += 1;
	}
	if (new_min >= 60) {
		new_min -= 60;
		/* don't increase hours, since they are masked */
	}
	sAlarm.AlarmTime.Seconds = RTC_ByteToBcd2(new_sec) & 0x7F;
	sAlarm.AlarmTime.Minutes = RTC_ByteToBcd2(new_min) & 0x7F;
	/* Mask date, weekday and hours so we check only minutes and seconds. */
	sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY | RTC_ALARMMASK_HOURS;
	sAlarm.Alarm = RTC_ALARM_B;

	/* Enable alarm interrupt */
	HAL_RTC_SetAlarm_IT(&RtcHandle, &sAlarm, RTC_FORMAT_BCD);
}

void configure_it_rtc_wakeup(void)
{
	EXTI_HandleTypeDef hexti;
	EXTI_ConfigTypeDef EXTI_ConfigStructure;

	/* Set Interrupt configuration of EXTI */
	EXTI_ConfigStructure.Line = EXTI2_LINE_19;
	EXTI_ConfigStructure.Trigger = EXTI_TRIGGER_RISING;
	EXTI_ConfigStructure.Mode = EXTI_MODE_INTERRUPT;
	HAL_EXTI_SetConfigLine(&hexti, &EXTI_ConfigStructure);

	/* Configure the interrupt for RTC */
	HAL_NVIC_SetPriority(RTC_IRQn, DEFAULT_IRQ_PRIO, 0);
	HAL_NVIC_EnableIRQ(RTC_IRQn);

	/* Configure RTC Alarm */
	RTC_SetAlarm(10);
}

void unconfigure_it_rtc_wakeup(void)
{
	HAL_RTC_DeactivateAlarm(&RtcHandle, RTC_ALARM_B);
	HAL_NVIC_DisableIRQ(RTC_IRQn);
}


int32_t _set_wakeup_source(void)
{
	configure_it_rtc_wakeup();

	return 0;
}

int32_t _clr_wakeup_source(void)
{
	unconfigure_it_rtc_wakeup();

	return 0;
}

static void delay(unsigned int second)
{
	uint32_t tk_freq = osKernelGetTickFreq();
	osDelay(second * tk_freq);
}

static void _suspend_resume(uint32_t flag, uint32_t expected_cpu2cr,
			    struct test_result_t *ret)
{
	uint32_t pwr_cpu2cr = 0;
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

	err = _clr_wakeup_source();
	if (err) {
		TEST_LOG("clr a wakeup source failed:%d\r\n", err);
	}

	pwr_cpu2cr = PWR->CPU2CR;
	TEST_LOG("CPU1CR = 0x%x\r\n", PWR->CPU1CR);
	TEST_LOG("CPU2CR = 0x%x\r\n", pwr_cpu2cr);

	osThreadFlagsSet(tid_copro, COPRO_START);

	/* Check PWR_CPU2CR flags
	 *	#17.LVDS_D2
	 *	#16.LPDS_D2
	 *	#15.DEEPSLEEP
	 *	#9.CSSF
	 *	#8.SBF_D3
	 *	#7.SBF_D2
	 *	#6.SBF
	 *	#5.STOPF
	 *	#4.VBF
	 *	#0.PDDS_D2
	 */

	if (pwr_cpu2cr == expected_cpu2cr)
		ret->val = TEST_PASSED;
	else
		ret->val = TEST_FAILED;

	return;
err:
	ret->val = TEST_FAILED;
}

void pm_suspend_stop2(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_STOP2, 0x00000020, ret);
}

void pm_suspend_lp_stop2(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_LP_STOP2, 0x00010020, ret);
}

void pm_suspend_lp_lv_stop2(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_LPLV_STOP2, 0x00030020, ret);
}

void pm_suspend_standby(struct test_result_t *ret)
{
	_suspend_resume(COPRO_PM_STANDBY1, 0x00000081, ret);
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

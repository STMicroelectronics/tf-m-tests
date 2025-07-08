/*
 * Copyright (C) 2024, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cmsis_os2.h>
#include <stdio.h>
#include <errno.h>

#include <test_framework.h>
#include <uapi/tfm_ioctl_api.h>
#include <psa/error.h>

#define MSEC_PER_SEC		1000L

#define TEST_ITERATIONS 3U
#define DELAY_ITERATION_S	(10)
#define TIMEOUT_STATUS_MS	(1000)

const char *cpu_status_str[] = {
	"offline",
	"suspended",
	"started",
	"running",
	"crashed",
	"unknow",
};

const char *cpu_method_str[] = {
	"none",
	"remoteproc",
	"invalid",
};

void cpu_list(struct test_result_t *ret)
{
	static struct cpu_serv_info serv_info;
	struct cpu_info_res cpu_info;
	int32_t i, err;
	char tmp[70];

	err = tfm_platform_cpu_service_info(&serv_info);
	if (err != TFM_PLATFORM_ERR_SUCCESS || serv_info.nb_cpu < 0)
		goto err;

	TEST_LOG("List of %d cpu(s) supported:\r\n", serv_info.nb_cpu);
	snprintf(tmp, sizeof(tmp), "%2.2s %-20.20s %-20.20s %-20.20s\r\n",
		 "id", "name", "status", "ctrl method");
	TEST_LOG(tmp);

	for (i = 0; i < serv_info.nb_cpu; i++) {

		err = tfm_platform_cpu_info(i, &cpu_info);
		if (err != TFM_PLATFORM_ERR_SUCCESS) {
			snprintf(tmp, sizeof(tmp), "%2.2d %-20.20s %-20.20s %-20.20s\r\n",
				 i, "error" "error" "error");
			goto err;
		} else if (cpu_info.status < 0 || cpu_info.status >= CPU_LAST) {
			snprintf(tmp, sizeof(tmp), "%2.2d %-20.20s %-20.20s %-20.20s\r\n",
				 i, cpu_info.name,
				 cpu_status_str[CPU_LAST],
				 cpu_method_str[cpu_info.method]);
			goto err;
		}

		err = snprintf(tmp, sizeof(tmp), "%2.2d %-20.20s %-20.20s %-20.20s\r\n",
			 i, cpu_info.name,
			 cpu_status_str[cpu_info.status],
			 cpu_method_str[cpu_info.method]);
		TEST_LOG(tmp);
	}

	ret->val = TEST_PASSED;
	return;

err:
	TEST_LOG(tmp);
	ret->val = TEST_FAILED;
}

static int32_t cpu_wait_status(uint32_t cpu_id, struct cpu_info_res *cpu_info,
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

static int _cpu_cmd_wait(cpu_cmd_fn_t cpu_fn, uint32_t cpu_id,
			 struct cpu_info_res *cpu_info, int32_t wait_status)
{
	int32_t status = cpu_info->status;
	int32_t err;

	if (!cpu_fn)
		return -EINVAL;

	err = cpu_fn(cpu_id, &status);
	if (err != TFM_PLATFORM_ERR_SUCCESS)
		return -EAGAIN;

	err = cpu_wait_status(cpu_id, cpu_info, TIMEOUT_STATUS_MS, wait_status);
	if (err)
		return err;

	return 0;
}

static int _cpu_start_stop(uint32_t cpu_id, struct cpu_info_res *cpu_info)
{
	uint32_t tk_freq = osKernelGetTickFreq();
	int32_t i, err = 0;

	for (i = 1; i <= TEST_ITERATIONS; i++) {
		TEST_LOG("\r    | Iteration %d of %d", i, TEST_ITERATIONS);

		if (cpu_info->status != CPU_OFFLINE) {

			err = _cpu_cmd_wait(tfm_platform_cpu_stop, cpu_id, cpu_info, CPU_OFFLINE);
			if (err) {
				TEST_LOG(" cmd stop fail (err:%d)\n",
					 cpu_info->name, err);
				goto out;
			}
		}

		err = _cpu_cmd_wait(tfm_platform_cpu_start, cpu_id, cpu_info, CPU_RUNNING);
		if (err) {
			TEST_LOG(" cmd start fail (err:%d)\n",
				 cpu_info->name, err);
			goto out;
		}

		osDelay(DELAY_ITERATION_S * tk_freq);
	}

	TEST_LOG("\r\n");

out:
	return err;
}

static bool _cpu_has_enable_method(struct cpu_info_res *cpu_info)
{
	if (cpu_info->method <= ENABLE_METHOD_NONE ||
	    cpu_info->method >= ENABLE_METHOD_INVAL)
		return false;

	return true;
}

void cpu_start_stop(struct test_result_t *result)
{
	static struct cpu_serv_info serv_info;
	struct cpu_info_res cpu_info;
	int32_t i, err;

	result->val = TEST_PASSED;

	err = tfm_platform_cpu_service_info(&serv_info);
	if (err != TFM_PLATFORM_ERR_SUCCESS || serv_info.nb_cpu < 0) {
		result->val = TEST_FAILED;
		return;
	}

	for (i = 0; i < serv_info.nb_cpu; i++) {

		err = tfm_platform_cpu_info(i, &cpu_info);
		if (err != TFM_PLATFORM_ERR_SUCCESS) {
			TEST_LOG("  > test cpu %d, info failed\r\n", i);
			result->val = TEST_FAILED;
			continue;
		}

		if (!_cpu_has_enable_method(&cpu_info)) {
			TEST_LOG("  > test cpu %s, has no enable method (skipped)\r\n",
				 cpu_info.name);
			continue;
		}

		TEST_LOG("  > test cpu %s\r\n", cpu_info.name);

		err = _cpu_start_stop(i, &cpu_info);
		if (err)
			result->val = TEST_FAILED;
	}
}

static struct test_t cpu_tests[] = {
	{&cpu_list, "TFM_NS_STM32_CPU_TEST_0201", "show platform cpus information"},
	{&cpu_start_stop, "TFM_NS_STM32_CPU_TEST_0202", "multiple start stop on remoteproc cpu"},
};

void register_testsuite_ns_cpu_interface(struct test_suite_t *p_test_suite)
{
    uint32_t list_size;

    list_size = (sizeof(cpu_tests) / sizeof(cpu_tests[0]));

    set_testsuite("CPU management NS interface tests (TFM_NS_CPU_XXXX)",
                  cpu_tests, list_size, p_test_suite);
}

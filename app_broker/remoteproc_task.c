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
#include "os_wrapper/semaphore.h"
#include <errno.h>
#include "remoteproc_driver.h"
#include "copro_task.h"

static void *semaphore_remoteproc;

void remoteproc_task_crash_cb(uint32_t cpu_id)
{
	os_wrapper_semaphore_release(semaphore_remoteproc);
}

void remoteproc_task(void *argument)
{
	UNUSED_VARIABLE(argument);
	int err;

	LOG_MSG("[NS] [INF] remoteproc task started\r\n");
	err = remoteproc_driver.init();
	if (err) {
		LOG_MSG("[NS] [INF] remoteproc task : driver init failed\r\n");
		goto out;
	}
	remoteproc_driver.register_crash_callback(remoteproc_task_crash_cb);
	LOG_MSG("[NS] [INF] remoteproc task : running\r\n");

	for(;;) {
		err = os_wrapper_semaphore_acquire(semaphore_remoteproc, OS_WRAPPER_WAIT_FOREVER);
		LOG_MSG("[NS] [INF] remoteproc task : Cortex A35 Crash\r\n");
		osThreadFlagsSet(tid_copro, COPRO_STOP);
		LOG_MSG("[NS] [INF] remoteproc task : Cortex A35 Stopped\r\n");
		osThreadFlagsSet(tid_copro, COPRO_START);
	}
out:
	osThreadExit();
}

static osThreadFunc_t remoteproc_thread_func = remoteproc_task;
static const osThreadAttr_t remoteproc_thread_attr = {
	.name = "remoteproc_thread",
	.stack_size = 1024U,
	.tz_module = ((TZ_ModuleId_t)TFM_DEFAULT_NSID),
	.priority = osPriorityHigh,
};

int remoteproc_init(void)
{
	semaphore_remoteproc = os_wrapper_semaphore_create(1, 0, "remoteproc");

	if (osThreadNew(remoteproc_thread_func, NULL, &remoteproc_thread_attr))
		LOG_MSG("[NS] [INF] remoteproc task : launched\r\n");
	else
		LOG_MSG("[NS] [INF] remoteproc task : failed\r\n");

	return 0;
}

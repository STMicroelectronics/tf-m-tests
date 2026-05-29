/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis_os2.h>
#include <device.h>
#include <errno.h>
#include <lib/timeout.h>
#include <remoteproc.h>
#include <uapi/tfm_ioctl_cpu_api.h>
#include <test_app.h>
#include <tfm_nsid_manager.h>
#include <tfm_log.h>
#include <tfm_platform_system.h>
#include "remoteproc_task.h"
#include "scmi_task.h"

#define RPROC_MSG_QUEUE_DEPTH	4U
#define RPROC_RESP_QUEUE_DEPTH	1U

#define RPROC_TIMEOUT_MS	1 * MSEC_PER_SEC

struct remoteproc_msg {
	enum remoteproc_cmd cmd;
};

struct remoteproc_resp {
	int ret;
};

struct remoteproc_thread_arg {
	const struct device *dev;
	osMessageQueueId_t mq;
	const char *name;
};

static struct remoteproc_thread_arg rproc_thd_arg;
static osMessageQueueId_t rproc_msgq = NULL;
static osMessageQueueId_t rproc_respq = NULL;
static osMutexId_t rproc_cmd_lock = NULL;

static int _remoteproc_status(const struct device *dev, int wait_status, uint32_t timeout_ms)
{
	uint32_t tk_timeout, tk_start, tk_curr;
	int status;

	status = rproc_status(dev);

	if (!timeout_ms)
		return status;

	/* Polling status */
	tk_timeout = (osKernelGetTickFreq() * timeout_ms) / MSEC_PER_SEC;
	tk_start = osKernelGetTickCount();

	do {
		if (status == wait_status)
			break;

		status = rproc_status(dev);
		tk_curr = osKernelGetTickCount();

	} while ((tk_curr - tk_start) < tk_timeout);

	return (status == wait_status) ? status : -ETIMEDOUT;
}

static int _remoteproc_cmd(struct remoteproc_thread_arg * rproc_arg, enum remoteproc_cmd cmd)
{
	uint32_t state_expected = UINT32_MAX;
	uint32_t wait_timeout_ms = 0;
	int status, err = -ENOTSUP;

	switch (cmd) {
	case RPROC_CMD_START:
		state_expected = CPU_RUNNING;
		wait_timeout_ms = RPROC_TIMEOUT_MS;
		err = rproc_boot(rproc_arg->dev);
		break;

	case RPROC_CMD_STOP:
		/* Disable CA35 SCMI Service to avoid processing useless request */
		scmi_ca35_disable();

		err = rproc_shutdown(rproc_arg->dev);

		/* Clear CA35 SCMI shared memory, enable/clean CA35 SCMI pending request */
		scmi_ca35_clean_enable();

		break;

	case RPROC_CMD_SUSPEND:
		err = rproc_suspend(rproc_arg->dev);
		break;

	case RPROC_CMD_RESUME:
		err = rproc_resume(rproc_arg->dev);
		break;
	}

	if (err) {
		LOG_MSG("[NS] [ERR] %s: cmd %d error: %d\r\n", rproc_arg->name, cmd, err);
		return err;
	}

	status = _remoteproc_status(rproc_arg->dev, state_expected, wait_timeout_ms);
	if (status  < 0) {
		LOG_MSG("[NS] [ERR] %s: status error: %d\r\n", rproc_arg->name, status);
		return status;
	}

	LOG_MSG("[NS] [INF] %s: %s now %s\r\n", rproc_arg->name, rproc_arg->dev->name,
		cpu_status_str[status]);

	return 0;
}

void remoteproc_task(void *arg)
{
	struct remoteproc_thread_arg *rproc_arg = arg;
	struct remoteproc_msg msg;
	struct remoteproc_resp resp;
	osStatus_t status;

	for (;;) {
		status = osMessageQueueGet(rproc_arg->mq, &msg, NULL, osWaitForever);
		if (status != osOK) {
			LOG_MSG("[NS] [ERR] %s: msg receive failed (%d)\r\n",
				rproc_arg->name, status);
			continue;
		}

		resp.ret = _remoteproc_cmd(rproc_arg, msg.cmd);

		status = osMessageQueuePut(rproc_respq, &resp, 0U, osWaitForever);
		if (status != osOK)
			LOG_MSG("[NS] [ERR] %s: cmd %d failed (%d)\r\n",
				rproc_arg->name, msg.cmd, status);
	}
}

static osThreadFunc_t rproc_thd_fn = remoteproc_task;
static const osThreadAttr_t rproc_thd_attr = {
	.name = "remoteproc_thread",
	.stack_size = 1024U,
	.tz_module = ((TZ_ModuleId_t)TFM_DEFAULT_NSID),
	.priority = osPriorityHigh,
};

/*
 * Send a remoteproc command to rproc_task and wait for completion.
 *
 * This API is synchronous from the client point of view: it returns only once
 * rproc_task has executed the requested command. It must be called from thread
 * context only, not from an interrupt context.
 *
 * On success or command failure, the returned value is the status returned by the
 * underlying remoteproc operation. A negative errno value is returned if the
 * command cannot be sent or if the response cannot be received.
 */
int remoteproc_send_cmd(enum remoteproc_cmd cmd)
{
	struct remoteproc_msg msg;
	struct remoteproc_resp resp;
	osStatus_t status;

	if (!rproc_msgq || !rproc_respq || !rproc_cmd_lock)
		return -EINVAL;

	status = osMutexAcquire(rproc_cmd_lock, osWaitForever);
	if (status != osOK)
		return -EIO;

	msg.cmd = cmd;

	status = osMessageQueuePut(rproc_msgq, &msg, 0U, osWaitForever);
	if (status != osOK) {
		osMutexRelease(rproc_cmd_lock);
		return -EIO;
	}

	status = osMessageQueueGet(rproc_respq, &resp, NULL, osWaitForever);
	osMutexRelease(rproc_cmd_lock);
	if (status != osOK)
		return -EIO;

	return resp.ret;
}

/*
 * Initialize the remoteproc service.
 *
 * This creates the command/response message queues used by clients calling
 * remoteproc_send_cmd(), the mutex that serializes synchronous commands. It then
 * starts the remoteproc command thread.
 */
int remoteproc_init(void)
{
	rproc_cmd_lock = osMutexNew(NULL);
	if (!rproc_cmd_lock) {
		LOG_MSG("[NS] [ERR] remoteproc cmd lock : failed\r\n");
		return -EINVAL;
	}

	rproc_msgq = osMessageQueueNew(RPROC_MSG_QUEUE_DEPTH,
					 sizeof(struct remoteproc_msg), NULL);
	if (!rproc_msgq) {
		LOG_MSG("[NS] [ERR] remoteproc msg queue : failed\r\n");
		return -EINVAL;
	}

	rproc_respq = osMessageQueueNew(RPROC_RESP_QUEUE_DEPTH,
					  sizeof(struct remoteproc_resp), NULL);
	if (!rproc_respq) {
		LOG_MSG("[NS] [ERR] remoteproc resp queue : failed\r\n");
		return -EINVAL;
	}

	rproc_thd_arg.dev = DEVICE_DT_GET(DT_NODELABEL(a35_rproc));
	rproc_thd_arg.mq = rproc_msgq;
	rproc_thd_arg.name = rproc_thd_attr.name;

	if (!osThreadNew(rproc_thd_fn, &rproc_thd_arg, &rproc_thd_attr)) {
		LOG_MSG("[NS] [ERR] %s : failed\r\n", rproc_thd_attr.name);
		return -EINVAL;
	}

	return 0;
}

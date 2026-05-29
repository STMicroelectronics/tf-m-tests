/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_REMOTEPROC_TASK_H__
#define __TFM_REMOTEPROC_TASK_H__

#include <util_macro.h>

enum remoteproc_cmd {
	RPROC_CMD_START,
	RPROC_CMD_STOP,
	RPROC_CMD_SUSPEND,
	RPROC_CMD_RESUME,
};

/* remoteproc task API */
#if (IS_ENABLED(TFM_PLATFORM_CPU_API))
int remoteproc_init(void);
int remoteproc_send_cmd(enum remoteproc_cmd cmd);
#else
static inline int remoteproc_init(void)
{
	return 0;
}

static inline int remoteproc_send_cmd(enum remoteproc_cmd cmd)
{
	return 0;
}
#endif

#endif /* __TFM_REMOTEPROC_TASK_H__ */

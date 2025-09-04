/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_COPRO_TASK_H__
#define __TFM_COPRO_TASK_H__

#define COPRO_START	(0x1U)
#define COPRO_STOP	(0x2U)

extern osThreadId_t tid_copro;

int copro_init(void);

#endif /* __TFM_COPRO_TASK_H__ */

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
#define COPRO_SUSPEND	(0x4U)
#define COPRO_SHUTDOWN	(0x8U)

/* Limit the next SUSPEND request done with COPRO_SUSPEND */
#define COPRO_PM_STOP2		(0x010U)
#define COPRO_PM_LP_STOP2	(0x020U)
#define COPRO_PM_LPLV_STOP2	(0x040U)
#define COPRO_PM_STANDBY1	(0x080U)
#define COPRO_PM_DISABLED	(0x100U)

extern osThreadId_t tid_copro;

int copro_init(void);

#endif /* __TFM_COPRO_TASK_H__ */

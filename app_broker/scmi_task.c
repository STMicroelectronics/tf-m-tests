/*
 * Copyright (C) 2025, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis_os2.h>
#include <cmsis_compiler.h>
#include <copro_task.h>
#include <stdio.h>
#include <test_app.h>
#include <tfm_nsid_manager.h>
#include <tfm_log.h>
#include <tfm_platform_system.h>
#include <tfm_scmi_api.h>
#include <tfm_ns_notif_api.h>
#include <uapi/tfm_ioctl_api.h>
#include <util_macro.h>
#include <scmi_task.h>

void scmi_ca35_disable(void)
{
	/* Stop ns notification */
	tfm_ns_notif_set_mask(~0);
}

void scmi_ca35_clean_enable(void)
{
	uint32_t event_trashed;

	/* Clean smt shared memory */
	tfm_secure_scmi_reset();

	/* Re-enable notif */
	tfm_ns_notif_set_mask(0);

	/* remove notif received in race condition */
	tfm_ns_notif_get(&event_trashed);
	tfm_ns_notif_get_pending(~0);
}

void tfm_sys_power_state_notifier(uint32_t agent_id, bool graceful, enum scmi_sys_power event)
{
	const char *graceful_str = graceful ? " - GRACEFUL" : "";

	switch (event) {
	case  SYS_POWER_SHUTDOWN:
		printf("[NS] [SCMI] [INF] agent %d SYS_POWER_SHUTDOWN%s\r\n",
		       agent_id, graceful_str);
		if (IS_ENABLED(TFM_PLATFORM_CPU_API)) {
			printf("[NS] [SCMI] [INF] shutdown command\r\n");
			osThreadFlagsSet(tid_copro, COPRO_SHUTDOWN);
		}
		break;

	case  SYS_POWER_COLD_RESET:
		printf("[NS] [SCMI] [INF] agent %d SYS_POWER_COLD_RESET%s\r\n",
		       agent_id, graceful_str);
		tfm_platform_system_reset();
		break;

	case  SYS_POWER_WARM_RESET:
		printf("[NS] [SCMI] [INF] agent %d SYS_POWER_WARM_RESET%s\r\n",
		       agent_id, graceful_str);
		if (IS_ENABLED(TFM_PLATFORM_CPU_API)) {
			osThreadFlagsSet(tid_copro, COPRO_STOP);
			printf("[NS] [SCMI] [INF] stop command done\r\n");
			osThreadFlagsSet(tid_copro, COPRO_START);
			printf("[NS] [SCMI] [INF] start command done\r\n");
		}
		break;

	case  SYS_POWER_SUSPEND:
		printf("[NS] [SCMI] [INF] agent %d SYS_POWER_SUSPEND%s\r\n",
		       agent_id, graceful_str);
		if (IS_ENABLED(TFM_PLATFORM_CPU_API)) {
			printf("[NS] [SCMI] [INF] suspend command\r\n");
			osThreadFlagsSet(tid_copro, COPRO_SUSPEND);
		}
		break;

	default:
		printf("[NS] [SCMI] [ERR] Unsupported sys power notification %x%s\r\n",
		       event, graceful_str);
		break;
	}
}

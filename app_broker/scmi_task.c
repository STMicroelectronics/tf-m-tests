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
	const char *graceful_str = graceful ? "GRACEFUL" : "";

	switch (event) {
	case  SYS_POWER_SHUTDOWN:
		printf("agent %d %s SYS_POWER_SHUTDOWN\r\n", agent_id, graceful_str);
		if (IS_ENABLED(TFM_PLATFORM_CPU_API)) {
			printf("shutdown command\r\n");
			osThreadFlagsSet(tid_copro, COPRO_SHUTDOWN);
		}
		break;

	case  SYS_POWER_COLD_RESET:
		printf("agent %d %s SYS_POWER_COLD_RESET\r\n", agent_id, graceful_str);
		tfm_platform_system_reset();
		break;

	case  SYS_POWER_WARM_RESET:
		printf("agent %d %s SYS_POWER_WARM_RESET\r\n", agent_id, graceful_str);
		if (IS_ENABLED(TFM_PLATFORM_CPU_API)) {
			printf("stop command\r\n");
			osThreadFlagsSet(tid_copro, COPRO_STOP);
			printf("stop command done\r\n");
			osThreadFlagsSet(tid_copro, COPRO_START);
			printf("start command done\r\n");
		}
		break;

	case  SYS_POWER_SUSPEND:
		printf("agent %d %s SYS_POWER_SUSPEND\r\n", agent_id, graceful_str);
		if (IS_ENABLED(TFM_PLATFORM_CPU_API)) {
			printf("suspend command\r\n");
			osThreadFlagsSet(tid_copro, COPRO_SUSPEND);
		}
		break;

	default:
		printf("Unsupported sys power notification %x\r\n", event);
		break;
	}
}

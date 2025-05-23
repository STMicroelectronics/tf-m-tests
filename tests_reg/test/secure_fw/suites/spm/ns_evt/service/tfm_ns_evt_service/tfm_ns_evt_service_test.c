/*
 * Copyright (c) 2025 STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <cmsis.h>
#include <assert.h>
#include <stdbool.h>
#include "psa/client.h"
#include "psa/service.h"
#include "psa_manifest/tfm_ns_evt_service_test.h"
#include "spm_test_defs.h"
#include "tfm_hal_isolation.h"
#include "tfm_sp_log.h"

#include "tfm_hal_multi_core.h"
#include "tfm_plat_defs.h"
#include "internal_status_code.h"
#include "tfm_ns_notif.h"
#include "ns_evt.h"
#include "tfm_peripherals_def.h"

void ns_evt_service_test_main(void *param)
{
  uint32_t signals = 0;
  psa_msg_t msg;
  psa_status_t ret;
  psa_irq_enable(TEST_SEC_PRIV_IRQ_SIGNAL);
  while (1) {
    ret = PSA_ERROR_GENERIC_ERROR;
    signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
    psa_get(signals, &msg);
    if (signals & NS_EVT_SERVICE_PRIV_NOTIF_NS_SIGNAL) {
      ret = tfm_ns_notif(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT);
    }
    if (signals & NS_EVT_SERVICE_FLIH_PRIV_NOTIF_NS_SIGNAL) {
      NVIC_SetPendingIRQ(TEST_SEC_PRIV_IRQ);
      ret = PSA_SUCCESS;
    }
    if (signals & NS_EVT_SERVICE_FLIH_NPRIV_NOTIF_NS_SIGNAL) {
      NVIC_SetPendingIRQ(TEST_SEC_NPRIV_IRQ);
      ret = PSA_SUCCESS;
    }
    if (signals & NS_EVT_SERVICE_TRY_NOTIF_NS_SIGNAL) {
      ret = tfm_ns_notif(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT);
    }
    psa_reply(msg.handle, ret);
  }
}

psa_flih_result_t test_sec_priv_irq_flih(void)
{
	tfm_ns_notif_flih(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_FLIH_NS_EVT);
  return PSA_FLIH_NO_SIGNAL;
}

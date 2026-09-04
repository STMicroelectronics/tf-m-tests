/*
 * Copyright (c) 2025 STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include "psa/client.h"
#include "psa/service.h"
#include "psa_manifest/sid.h"
#include "psa_manifest/tfm_ns_evt_helper_test.h"
#include "tfm_hal_isolation.h"
#include "utilities.h"
#include <service_api.h>
#include "ns_evt.h"
#include <service_api.h>
/*
 * Create a global const data, so that it is stored in code
 * section which is read only.
 */

#if TFM_SP_NS_EVT_HELPER_TEST_MODEL_IPC == 1

void ns_evt_helper_test_main(void)
{
    psa_msg_t msg;
    psa_status_t ret;
    uint32_t signals = 0;
    psa_irq_enable(TEST_SEC_NPRIV_IRQ_SIGNAL);
    while (1) {
      ret = PSA_ERROR_GENERIC_ERROR;
      signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
      psa_get(signals, &msg);
      if (signals & NS_HELPER_NOTIFY_NS_SIGNAL) {
        ret = tfm_ns_notif(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT);
      }
      if (signals & NS_HELPER_TRY_NOTIFY_NS_SIGNAL)
      {
        ret = tfm_ns_notif(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT);
      }
      psa_reply(msg.handle, ret);
    }
}

#elif TFM_SP_NS_EVT_HELPER_TEST_MODEL_SFN == 1

psa_status_t ns_evt_helper_test_init(void)
{
  psa_irq_enable(TEST_SEC_NPRIV_IRQ_SIGNAL);
  return PSA_SUCCESS;
}

psa_status_t ns_helper_notify_ns_sfn(const psa_msg_t *msg)
{
  (void)msg;
  return tfm_ns_notif(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT);
}

psa_status_t ns_helper_try_notify_ns_sfn(const psa_msg_t *msg)
{
  (void)msg;
  return tfm_ns_notif(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT);
}

#endif /* TFM_SP_NS_EVT_HELPER_TEST_MODEL_IPC == 1 */


psa_flih_result_t test_sec_npriv_irq_flih(void)
{
	tfm_ns_notif_flih(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_FLIH_NS_EVT);
  return PSA_FLIH_NO_SIGNAL;
}

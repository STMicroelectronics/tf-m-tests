/*
 * Copyright (c) 2025 STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "ns_evt_ns_tests.h"
#include "psa/client.h"
#include "psa/framework_feature.h"
#include "ns_evt.h"
#include "tfm_ns_notif_api.h"
#include "test_framework_helpers.h"
#include "psa_manifest/sid.h"

/* List of tests */
static void tfm_ns_evt_test_1001(struct test_result_t *ret);
static void tfm_ns_evt_test_1002(struct test_result_t *ret);
static void tfm_ns_evt_test_1003(struct test_result_t *ret);
static void tfm_ns_evt_test_1004(struct test_result_t *ret);
static void tfm_ns_evt_test_1005(struct test_result_t *ret);
static void tfm_ns_evt_test_1006(struct test_result_t *ret);
static void tfm_ns_evt_test_1007(struct test_result_t *ret);


static struct test_t ns_evt_tests[] = {
    {&tfm_ns_evt_test_1001, "TFM_NS_EVT_TEST_1001",
     "Send an event to non secure from an unpriv partition"},
    {&tfm_ns_evt_test_1002, "TFM_NS_EVT_TEST_1002",
     "Send an event to non secure from an FILH unpriv handler"},
    {&tfm_ns_evt_test_1003, "TFM_NS_EVT_TEST_1003",
     "Fail to send an event not handled by an unpriv partition"},
    {&tfm_ns_evt_test_1004, "TFM_NS_EVT_TEST_1004",
      "Send an event to non secure from a priv partition"},
    {&tfm_ns_evt_test_1005, "TFM_NS_EVT_TEST_1005",
      "Send an event to non secure from an FLIH priv partition"},
    {&tfm_ns_evt_test_1006, "TFM_NS_EVT_TEST_1006",
     "Fail to send an event not handled by an priv partition"},
    {&tfm_ns_evt_test_1007, "TFM_NS_EVT_TEST_1007",
     "test set ns evt mask interface"},

};

void register_testsuite_ns_ns_evt_interface(struct test_suite_t *p_test_suite)
{
    uint32_t list_size;

    list_size = (sizeof(ns_evt_tests) / sizeof(ns_evt_tests[0]));

    set_testsuite("NS_EVT non-secure test (TFM_NS_NS_EVT_TEST_1XXX)",
                  ns_evt_tests, list_size, p_test_suite);
}

/**
 * \brief
 *
 */
static void tfm_ns_evt_test_1001(struct test_result_t *ret)

{
  psa_status_t status;

  ret->val = TEST_FAILED;
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT) {
    TEST_FAIL("NOTIF from unpriv secure FAILED: notif already there!\r\n");
    return;
  }
  status = psa_call(NS_HELPER_NOTIFY_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if (status != PSA_SUCCESS) {
    TEST_FAIL("NOTIF from unpriv secure FAILED: psa_call return failed!\r\n");
    return;
  }
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
      != TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
    TEST_FAIL("NOTIF from unpriv secure FAILED!\r\n");
  else
    ret->val = TEST_PASSED;
}

/**
 * \brief
 *
 */
static void tfm_ns_evt_test_1002(struct test_result_t *ret)
{
  psa_status_t status;

  ret->val = TEST_FAILED;
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_FLIH_NS_EVT)
       == TFM_SP_NS_EVT_HELPER_TEST_NPRIV_FLIH_NS_EVT) {
    TEST_FAIL("NOTIF from flih unpriv secure FAILED: notif already there!\r\n");
    return;
  }
  status = psa_call(NS_EVT_SERVICE_FLIH_NPRIV_NOTIF_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if (status != PSA_SUCCESS) {
    TEST_FAIL("NOTIF from flih unpriv secure FAILED: psa_call return failed!\r\n");
    return;
  }
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_FLIH_NS_EVT)
       != TFM_SP_NS_EVT_HELPER_TEST_NPRIV_FLIH_NS_EVT) {
    TEST_FAIL("NOTIF from flih unpriv secure FAILED: event not received\r\n");
  } else
    ret->val = TEST_PASSED;
}

/**
 * \brief
 *
 */
static void tfm_ns_evt_test_1003(struct test_result_t *ret)
{
  psa_status_t status;

  ret->val = TEST_FAILED;
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT) {
    TEST_FAIL("NOTIF from priv secure : notif already there!\r\n");
    return;
  }
  status = psa_call(NS_HELPER_TRY_NOTIFY_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if (status != PSA_ERROR_NOT_PERMITTED) {
    TEST_FAIL("TRY NOTIF from priv form unpriv secure: psa_call does not return failed!\r\n");
    return;
  }
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT)
    TEST_FAIL("NOTIF from priv secure: event received\r\n");
  else
    ret->val = TEST_PASSED;
}

/**
 * \brief
 *
 */
static void tfm_ns_evt_test_1004(struct test_result_t *ret)
{
  psa_status_t status;

  ret->val = TEST_FAILED;
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT) {
    TEST_FAIL("NOTIF from priv secure: notif already there!\r\n");
    return;
  }
  status = psa_call(NS_EVT_SERVICE_PRIV_NOTIF_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if (status != PSA_SUCCESS) {
    TEST_FAIL("NOTIF from priv secure sent: psa__call does not return failed\r\n");
    return;
  }
  if ( tfm_ns_notif_get_pending(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT)
       != TFM_SP_NS_EVT_SERVICE_TEST_PRIV_CALL_NS_EVT)
    TEST_FAIL("NOTIF from priv secure FAILED : event not received\r\n");
  else
    ret->val = TEST_PASSED;
}

/**
 * \brief
 *
 */
static void tfm_ns_evt_test_1005(struct test_result_t *ret)
{
  psa_status_t status;

  ret->val = TEST_FAILED;
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_FLIH_NS_EVT)
      ==TFM_SP_NS_EVT_SERVICE_TEST_PRIV_FLIH_NS_EVT) {
    TEST_FAIL("NOTIF from flih priv secure: notif already there!\r\n");
    return;
  }
  status = psa_call(NS_EVT_SERVICE_FLIH_PRIV_NOTIF_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if ((status != PSA_SUCCESS) ||
      (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_SERVICE_TEST_PRIV_FLIH_NS_EVT)
       != TFM_SP_NS_EVT_SERVICE_TEST_PRIV_FLIH_NS_EVT))
    TEST_FAIL("NOTIF from flih priv secure FAILED!\r\n");
  else
    ret->val = TEST_PASSED;
}

/**
 * \brief
 *
 */
static void tfm_ns_evt_test_1006(struct test_result_t *ret)
{
  psa_status_t status;

  ret->val = TEST_FAILED;
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT) {
    TEST_FAIL("NOTIF from npriv secure: notif already there!\r\n");
    return;
  }
  status = psa_call(NS_EVT_SERVICE_TRY_NOTIF_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if (status != PSA_ERROR_NOT_PERMITTED) {
    TEST_FAIL("NOTIF from npriv secure sent: psa_call does not return failed!\r\n");
    return;
  }
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
    TEST_FAIL("NOTIF from unpriv secure  : event received\r\n");
  else
    ret->val = TEST_PASSED;
}

/**
 * \brief
 *
 */
static void tfm_ns_evt_test_1007(struct test_result_t *ret)

{
  psa_status_t status;

  ret->val = TEST_FAILED;
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT) {
    TEST_FAIL("NOTIF from unpriv secure FAILED: notif already there!\r\n");
    return;
  }
  if (tfm_ns_notif_set_mask(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)) {
    TEST_FAIL("ns notif set mask return wrong value\r\n");
    return;
  }

  status = psa_call(NS_HELPER_NOTIFY_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if (status != PSA_ERROR_BAD_STATE) {
    TEST_FAIL("NOTIF from unpriv secure FAILED: psa_call return wrong value!\r\n");
    return;
  }
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
      == TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT) {
    TEST_FAIL("NOTIF from unpriv secure received\r\n");
    return;
  }
  if (tfm_ns_notif_set_mask(0)) {
    TEST_FAIL("ns notif set mask return wrong value\r\n");
    return;
  }
  status = psa_call(NS_HELPER_NOTIFY_NS_HANDLE, PSA_IPC_CALL, NULL, 0, NULL, 0);
  if (status != PSA_SUCCESS) {
    TEST_FAIL("NOTIF from unpriv secure FAILED: psa_call does not return success!\r\n");
    return;
  }
  if (tfm_ns_notif_get_pending(TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT)
      != TFM_SP_NS_EVT_HELPER_TEST_NPRIV_CALL_NS_EVT) {
    TEST_FAIL("NOTIF from unpriv secure received\r\n");
    return;
  }
  ret->val = TEST_PASSED;
}


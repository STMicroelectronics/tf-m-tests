/*
 * Copyright (c) 2025 STMicroelectronics. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __NS_EVT_NS_TESTS_H__
#define __NS_EVT_NS_TESTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "test_framework.h"

/**
 * \brief Register testsuite for ns_evt non-secure interface.
 *
 * \param[in] p_test_suite The test suite to be executed.
 */
void register_testsuite_ns_ns_evt_interface(struct test_suite_t *p_test_suite);

#ifdef __cplusplus
}
#endif

#endif /* __NS_EVT_NS_TESTS_H__ */

/*
 * Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef  __PM_NS_TESTS_H
#define  __PM_NS_TESTS_H

#include "test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Register test suite for power management (PM) interface
 *
 * \param[in] p_test_suite The test suite to be executed.
 */
void register_testsuite_ns_pm_interface(struct test_suite_t *p_test_suite);

#ifdef __cplusplus
}
#endif

#endif /* __PM_NS_TESTS_H */

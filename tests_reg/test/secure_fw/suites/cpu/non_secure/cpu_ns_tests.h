/*
 * Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef  __CPU_NS_TESTS_H
#define  __CPU_NS_TESTS_H

#include "test_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Register test suite for CPU management interface
 *
 * \param[in] p_test_suite The test suite to be executed.
 */
void register_testsuite_ns_cpu_interface(struct test_suite_t *p_test_suite);

#ifdef __cplusplus
}
#endif

#endif /* __CPU_NS_TESTS_H */

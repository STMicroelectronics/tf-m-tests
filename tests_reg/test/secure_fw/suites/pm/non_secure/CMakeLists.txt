#-------------------------------------------------------------------------------
# Copyright (c) 2020-2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if (NOT TEST_NS_PM)
    return()
endif()

add_library(tfm_test_suite_pm_ns STATIC EXCLUDE_FROM_ALL)

target_sources(tfm_test_suite_pm_ns
    PRIVATE
        ./pm_ns_interface_testsuite.c
)

target_include_directories(tfm_test_suite_pm_ns
    PUBLIC
        ./
)

target_compile_definitions(tfm_test_suite_pm_ns
    PRIVATE
        DOMAIN_NS=1
        $<$<BOOL:${TEST_NS_PM_SUSPEND}>:TEST_NS_PM_SUSPEND>
        $<$<BOOL:${TEST_NS_PM_OFF}>:TEST_NS_PM_OFF>
    INTERFACE
        TEST_NS_PM
)

target_link_libraries(tfm_test_suite_pm_ns
    PRIVATE
        tfm_test_framework_ns
        RTX_OS
)

target_link_libraries(tfm_ns_tests
    INTERFACE
        tfm_test_suite_pm_ns
)

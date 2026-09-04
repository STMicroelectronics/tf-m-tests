#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if(NOT TEST_NS_NS_EVT)
    return()
endif()

add_library(tfm_test_suite_ns_evt_ns STATIC EXCLUDE_FROM_ALL)

target_sources(tfm_test_suite_ns_evt_ns
    PRIVATE
        ns_evt_interface_testsuite.c
)

target_include_directories(tfm_test_suite_ns_evt_ns
    PUBLIC
        .
)

target_compile_definitions(tfm_test_suite_ns_evt_ns
    PRIVATE
        DOMAIN_NS=1
    INTERFACE
        TEST_NS_NS_EVT
)

if (CONFIG_TFM_SPM_BACKEND STREQUAL "SFN")
    # TFM_NS_EVT_TEST_1001/1002/1003 exercise the unpriv HELPER partition
    # notification path, which is not covered under the SFN backend.
    target_compile_definitions(tfm_test_suite_ns_evt_ns
        PRIVATE
            NS_EVT_SFN_BACKEND
    )
endif()

target_link_libraries(tfm_test_suite_ns_evt_ns
    PRIVATE
        tfm_test_framework_ns
)

target_link_libraries(tfm_ns_tests
    INTERFACE
        tfm_test_suite_ns_evt_ns
)

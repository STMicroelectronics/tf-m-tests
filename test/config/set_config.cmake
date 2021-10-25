#-------------------------------------------------------------------------------
# Copyright (c) 2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

########################## TEST SYNC ###########################################

if ((NOT TFM_PARTITION_PROTECTED_STORAGE AND NOT FORWARD_PROT_MSG))
    set(TEST_NS_PS              OFF        CACHE BOOL      "Whether to build NS regression PS tests")
    set(TEST_S_PS               OFF        CACHE BOOL      "Whether to build S regression PS tests")
endif()

if (NOT TFM_PARTITION_INTERNAL_TRUSTED_STORAGE AND NOT FORWARD_PROT_MSG)
    set(TEST_NS_ITS             OFF        CACHE BOOL      "Whether to build NS regression ITS tests")
    set(TEST_S_ITS              OFF        CACHE BOOL      "Whether to build S regression ITS tests")

    # TEST_NS_PS relies on TEST_NS_ITS
    set(TEST_NS_PS              OFF        CACHE BOOL      "Whether to build NS regression PS tests")
endif()

if (NOT TFM_PARTITION_CRYPTO AND NOT FORWARD_PROT_MSG)
    set(TEST_NS_CRYPTO          OFF        CACHE BOOL      "Whether to build NS regression Crypto tests")
    set(TEST_S_CRYPTO           OFF        CACHE BOOL      "Whether to build S regression Crypto tests")
endif()

if (NOT TFM_PARTITION_INITIAL_ATTESTATION AND NOT FORWARD_PROT_MSG)
    set(TEST_NS_ATTESTATION     OFF        CACHE BOOL      "Whether to build NS regression Attestation tests")
    set(TEST_S_ATTESTATION      OFF        CACHE BOOL      "Whether to build S regression Attestation tests")
    set(TEST_NS_QCBOR           OFF        CACHE BOOL      "Whether to build NS regression QCBOR tests")
    set(TEST_NS_T_COSE          OFF        CACHE BOOL      "Whether to build NS regression t_cose tests")
endif()

if (SYMMETRIC_INITIAL_ATTESTATION)
    set(TEST_NS_T_COSE          OFF        CACHE BOOL      "Whether to build NS regression t_cose tests")
endif()

if (NOT TFM_PARTITION_PLATFORM AND NOT FORWARD_PROT_MSG)
    set(TEST_NS_PLATFORM        OFF        CACHE BOOL      "Whether to build NS regression Platform tests")
    set(TEST_S_PLATFORM         OFF        CACHE BOOL      "Whether to build S regression Platform tests")
endif()

if (NOT TFM_PARTITION_FIRMWARE_UPDATE)
    set(TEST_NS_FWU             OFF        CACHE BOOL      "Whether to build NS regression FWU tests")
    set(TEST_S_FWU              OFF        CACHE BOOL      "Whether to build S regression FWU tests")
endif()

if (NOT TFM_PARTITION_AUDIT_LOG)
    set(TEST_NS_AUDIT           OFF        CACHE BOOL      "Whether to build NS regression Audit log tests")
    set(TEST_S_AUDIT            OFF        CACHE BOOL      "Whether to build S regression Audit log tests")
endif()

if (TFM_LIB_MODEL)
    set(TEST_NS_IPC             OFF        CACHE BOOL      "Whether to build NS regression IPC tests")
    set(TEST_S_IPC              OFF        CACHE BOOL      "Whether to build S regression IPC tests")

    set(TEST_NS_SLIH_IRQ        OFF        CACHE BOOL      "Whether to build NS regression Second-Level Interrupt Handling tests")
    set(TEST_NS_FLIH_IRQ        OFF        CACHE BOOL      "Whether to build NS regression First-Level Interrupt Handling tests")
endif()

if (NOT TFM_MULTI_CORE_TOPOLOGY)
    set(TEST_NS_MULTI_CORE      OFF        CACHE BOOL      "Whether to build NS regression multi-core tests")
endif()

if (NOT TFM_NS_MANAGE_NSID)
    set(TEST_NS_MANAGE_NSID     OFF        CACHE BOOL      "Whether to build NS regression NSID management tests")
endif()

########################## Test framework sync #################################

get_cmake_property(CACHE_VARS CACHE_VARIABLES)
# Force TEST_FRAMEWORK_NS ON if single NS test ON
foreach(CACHE_VAR ${CACHE_VARS})
    string(REGEX MATCH "^TEST_NS_.*" _NS_TEST_FOUND "${CACHE_VAR}")
    if (_NS_TEST_FOUND AND "${${CACHE_VAR}}")
        set(TEST_FRAMEWORK_NS       ON        CACHE BOOL      "Whether to build NS regression tests framework")
        break()
    endif()
endforeach()

# Force TEST_FRAMEWORK_S ON if single S test ON
foreach(CACHE_VAR ${CACHE_VARS})
    string(REGEX MATCH "^TEST_S_.*" _S_TEST_FOUND "${CACHE_VAR}")
    if (_S_TEST_FOUND AND "${${CACHE_VAR}}")
        set(TEST_FRAMEWORK_S        ON        CACHE BOOL      "Whether to build S regression tests framework")
        break()
    endif()
endforeach()

########################## Extra test suites ###################################

if (EXTRA_NS_TEST_SUITES_PATHS)
    set(TEST_FRAMEWORK_NS       ON        CACHE BOOL      "Whether to build NS regression tests framework")
endif()

if (EXTRA_S_TEST_SUITES_PATHS)
    set(TEST_FRAMEWORK_S        ON        CACHE BOOL      "Whether to build S regression tests framework")
endif()

########################## Test profile ########################################

if (TFM_PROFILE)
    include(${TFM_TEST_PATH}/config/profile/${TFM_PROFILE}_test.cmake)
endif()

########################## Load default config #################################

if (TEST_S)
    include(${TFM_TEST_PATH}/config/default_s_test_config.cmake)
endif()
if (TEST_NS)
    include(${TFM_TEST_PATH}/config/default_ns_test_config.cmake)
endif()

include(${TFM_TEST_PATH}/config/default_test_config.cmake)

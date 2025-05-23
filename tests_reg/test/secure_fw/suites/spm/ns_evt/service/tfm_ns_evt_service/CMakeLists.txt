#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_library(tfm_psa_rot_partition_ns_evt_service STATIC)

add_dependencies(tfm_psa_rot_partition_ipc_service manifest_tool)

target_sources(tfm_psa_rot_partition_ns_evt_service
    PRIVATE
       ./tfm_ns_evt_service_test.c
)

# The generated sources
target_sources(tfm_psa_rot_partition_ns_evt_service
    PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_service/auto_generated/intermedia_tfm_ns_evt_service_test.c
  )

target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_service/auto_generated/load_info_tfm_ns_evt_service_test.c
)

# Set include directory
target_include_directories(tfm_psa_rot_partition_ns_evt_service
    PUBLIC
        .
    PRIVATE
        ../../../common
        ../../../common/service
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_service
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_service
)

target_link_libraries(tfm_psa_rot_partition_ns_evt_service
    PRIVATE
        platform_s
        tfm_sprt
)

target_link_libraries(tfm_spm
    PRIVATE
        tfm_psa_rot_partition_ipc_service
)

target_compile_definitions(tfm_psa_rot_partition_ipc_service
    PRIVATE
)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_psa_rot_partition_ns_evt_service
)

target_compile_definitions(tfm_config
    INTERFACE
    TFM_PARTITION_TEST_CORE_NS_EVT
)

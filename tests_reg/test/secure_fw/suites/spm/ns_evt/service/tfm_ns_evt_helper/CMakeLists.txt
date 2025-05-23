#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_library(tfm_app_rot_partition_ns_evt_helper STATIC)

add_dependencies(tfm_app_rot_partition_ns_evt_helper manifest_tool)

target_sources(tfm_app_rot_partition_ns_evt_helper
    PRIVATE
        ./tfm_ns_evt_helper_test.c
)

# The generated sources
target_sources(tfm_app_rot_partition_ns_evt_helper
  PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_helper/auto_generated/intermedia_tfm_ns_evt_helper_test.c
)

target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_helper/auto_generated/load_info_tfm_ns_evt_helper_test.c
)

# Set include directory
target_include_directories(tfm_app_rot_partition_ns_evt_helper
  PUBLIC
        .
    PRIVATE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_helper
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/generated/secure_fw/test_services/tfm_ns_evt_helper
)

target_link_libraries(tfm_app_rot_partition_ns_evt_helper
  PRIVATE
        tfm_sprt
        platform_s
)

target_link_libraries(tfm_spm
    PRIVATE
        tfm_app_rot_partition_ns_evt_helper)

############################ Partition Defs ####################################

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_ns_evt_helper
)

target_compile_definitions(tfm_config
    INTERFACE
    TFM_PARTITION_TEST_CORE_NS_EVT
)

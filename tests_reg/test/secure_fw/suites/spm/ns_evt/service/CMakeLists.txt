#-------------------------------------------------------------------------------
# Copyright (c) 2025 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Add ipc test service.
if (TFM_PARTITION_NS_EVT_TEST)
    add_subdirectory(tfm_ns_evt_helper)
    add_subdirectory(tfm_ns_evt_service)
endif()

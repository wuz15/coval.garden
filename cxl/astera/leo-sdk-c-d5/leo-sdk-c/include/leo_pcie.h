/*
 * Copyright 2022 Astera Labs, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file leo_pcie.h
 * @brief Definition of PCIe types for the SDK.
 */

#ifndef ASTERA_LEO_SDK_PCIE_H_
#define ASTERA_LEO_SDK_PCIE_H_

#include "../include/misc.h"
#include "astera_log.h"
#include "leo_api_types.h"
#include "leo_connection.h"
#include "leo_error.h"
#include "leo_globals.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHECK_SUCCESS(rc)                                                      \
  {                                                                            \
    if (rc != LEO_SUCCESS) {                                                   \
      ASTERA_ERROR("Unexpected return code: %d", rc);                          \
      return rc;                                                               \
    }                                                                          \
  }

#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr)-offsetof(type, member)))

#define PRINT_ERROR                                                            \
  do {                                                                         \
    fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,         \
            __FILE__, errno, strerror(errno));                                 \
    exit(1);                                                                   \
  } while (0)

/**
 * @brief Write a data word at specified address to Leo over PCIe. Returns a
 * negative error code if unsuccessful, else zero on success.
 *
 * @param[in]  barfd      sysfs file descriptor for PCIe BAR
 * @param[in]  baroff     Offset to read from in the PCIe BAR
 * @param[in]  value      32-bit word to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteWordPcie(const char *barfile, off_t baroff,
                              uint32_t writeval);

/**
 * @brief Write a data word at specified address to Leo over PCIe. Returns a
 * negative error code if unsuccessful, else zero on success.
 *
 * @param[in]  barfd      sysfs file descriptor for PCIe BAR
 * @param[in]  baroff     Offset to read from in the PCIe BAR
 * @param[in]  value      Pointer to 32-bit value
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadWordPcie(const char *barfile, off_t baroff,
                             uint32_t *value);

/**
 * @brief PCIe types
 */

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_PCIE_H_ */

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
 * @file board.h
 * @brief board related settings for examples.
 */
#ifndef _EXAMPLES_BOARD_H
#define _EXAMPLES_BOARD_H

#include "../../include/leo_api_types.h"
#include "../../include/leo_error.h"
#include "../../include/leo_i2c.h"

#include <getopt.h>
#include <bits/getopt_ext.h>

void usage(char *name, struct option *long_options, const char **help_string);
extern int IS_SVB;
extern int IS_A1;
extern int IS_A2;
extern int IS_LEO;

typedef struct DefaultArgs {
  int switchAddress;
  int leoAddress;
  int switchHandle;
  int switchMode;
  char *serialnum;
  char *bdf;
} DefaultArgsType;

#define DEFAULT_ENUMS                                                          \
  ENUM_HELP_e, ENUM_SWITCH_e, ENUM_SERIALNUM_e, ENUM_BDF_e, ENUM_SVB_e,        \
      ENUM_A1_e, ENUM_A2_e, ENUM_LEO_e

#define DEFAULT_OPTIONS                                                        \
  {"help", no_argument, 0, 0}, {"switch", required_argument, 0, 0},            \
      {"serialnum", required_argument, 0, 0},                                  \
      {"bdf", required_argument, 0, 0}, {"svb", no_argument, 0, 0},            \
      {"aurora1000", no_argument, 0, 0}, {"aurora2000", no_argument, 0, 0}, {  \
    "leo", required_argument, 0, 0                                             \
  }

#define DEFAULT_HELPSTRINGS                                                    \
  "This page", "(Optional) Set Switch/MUX addr (default 0x77)",                \
      "(Optional) Choose target connected to the I2C Serial Number(serialnum " \
      "in    "                                                                 \
      "                                   \t\t\t\t\t\t  the format "           \
      "xxxx-xxxxxx)",                                                          \
      "PCIe Bus/Device/Function of the CXL Memory \n"                          \
      "                         \t\t\t\t\t\t          example 17:00.0 (one "   \
      "bdf)\n"                                                                 \
      "                         \t\t\t\t\t\t          17:00.0,17:00.1 (multi " \
      "bdf)",                                                                  \
      "Configure target as SVB", "Configure target as Aurora 1",               \
      "Configure target as Aurora 2000", "(Optional) Specify leo ID [0, 1]"

#define DEFAULT_SWITCH_CASES(defaultArgs, long_options, help_string)           \
  case ENUM_HELP_e:                                                            \
    usage(argv[0], long_options, help_string);                                 \
    break;                                                                     \
  case ENUM_SWITCH_e:                                                          \
    defaultArgs.switchAddress = strtoul(optarg, NULL, 16);                     \
    if ((defaultArgs.switchAddress != 0x70) &&                                 \
        (defaultArgs.switchAddress != LEO_DEV_MUX)) {                          \
      ASTERA_INFO("Invalid switch address, it should be 0x70 or 0x77");        \
      usage(argv[0], long_options, help_string);                               \
    }                                                                          \
    ASTERA_INFO("Set switch addr 0x%x", defaultArgs.switchAddress);            \
    break;                                                                     \
  case ENUM_SERIALNUM_e:                                                       \
    defaultArgs.serialnum = optarg;                                            \
    ASTERA_INFO("Using target connected to I2C Serial Number %s",              \
                defaultArgs.serialnum);                                        \
    break;                                                                     \
  case ENUM_BDF_e:                                                             \
    defaultArgs.bdf = optarg;                                                  \
    ASTERA_INFO("Using target CXL device(s) at %s", defaultArgs.bdf);          \
    break;                                                                     \
  case ENUM_SVB_e:                                                             \
    IS_SVB = 1;                                                                \
    IS_A1 = 0;                                                                 \
    IS_A2 = 0;                                                                 \
    defaultArgs.switchMode = 0x3;                                              \
    break;                                                                     \
  case ENUM_A1_e:                                                              \
    IS_SVB = 0;                                                                \
    IS_A1 = 1;                                                                 \
    IS_A2 = 0;                                                                 \
    defaultArgs.switchMode = 0x3;                                              \
    break;                                                                     \
  case ENUM_A2_e:                                                              \
    IS_SVB = 0;                                                                \
    IS_A1 = 0;                                                                 \
    IS_A2 = 1;                                                                 \
    break;                                                                     \
  case ENUM_LEO_e:                                                             \
    IS_LEO = 1;                                                                \
    if (strtoul(optarg, NULL, 16) == 1) {                                      \
      defaultArgs.leoAddress = LEO_DEV_LEO_1;                                  \
      defaultArgs.switchMode = 0x5;                                            \
    } else {                                                                   \
      defaultArgs.leoAddress = LEO_DEV_LEO_0;                                  \
      defaultArgs.switchMode = 0x3;                                            \
    }                                                                          \
    ASTERA_INFO("Set Leo I2C addr 0x%x", defaultArgs.leoAddress);              \
    break;

/**
 * @brief Set the I2C mux switch for the Leo SVB.
 *
 * @param handle I2C device handle.
 * @param mode At the moment, leo 0 (0x03).
 * @return LeoErrorType.
 */
LeoErrorType svbP1SetI2cSwitch(int handle, int mode);

/**
 * @brief Set the I2C mux switch for the Leo SVB.
 *
 * @param handle I2C device handle.
 * @param mode for the Aurora2000 board, leo 0 (0x03) or leo 1 (0x05).
 * @return LeoErrorType.
 */
LeoErrorType aurora2000SetI2cSwitch(int handle, int mode);

/**
 * @brief Set default I2C mux switch automatically based on the board type.
 *
 * @param i2cbus I2C bus #.
 * @param [in,out] DefaultArgsType.
 * @param [in] conn connectivity details.
 * @return LeoErrorType.
 */
LeoErrorType leoSetMuxAddress(int i2cBus, DefaultArgsType *defaultArgs,
                              conn_t conn);

/**
  * @brief Decode ID Code of manufacturer according to JEDEC spec
  *
  * @param [in] dimmManufacturerIdCode
  * @param [out] manufacturerString - max size 40 chars
  * @return LeoErrorType.
  */
LeoErrorType decodeDimmManufacturer(uint32_t dimmManufacturerIdCode, char* manufacturerString);

#endif // _EXAMPLES_BOARD_H

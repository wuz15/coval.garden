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
 * @file leo_fw_update_example.c
 * @brief Example application to perform firmware update in Leo Tunnel Mode.
 * This is recommended for:
 *        - Setting up of I2C driver
 *        - Setting up the LEO SPI interface for accessing the QSPI flash device
 *        - Performing sanity tests related to flashing ( erase, program, and
 * verification)
 *       - Performing firmware update (supports svb, aurora1000 and aurora2000)
 *  For more details on the args supported & usage
 * sudo ./leo_fw_update_example -help
 */

#include "../include/DW_apb_ssi.h"
#include "../include/leo_api.h"
#include "../include/leo_api_types.h"
#include "../include/leo_common.h"
#include "../include/leo_error.h"
#include "../include/leo_globals.h"
#include "../include/leo_i2c.h"
#include "../include/leo_spi.h"
#include "include/aa.h"
#include "include/board.h"
#include "include/libi2c.h"

#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  int ii;
  LeoErrorType rc;

  char filename[128];
  int leoHandle;
  int switchHandle;
  int gpioHandle;
  uint32_t curLeoAddr;
  conn_t conn;
  int option_index;
  int option;
  int is_program = 0;
  int is_verify = 0;
  int is_all = 0;
  int is_clean = 0;
  int is_force = 0;
  int leoId;
  uint8_t readSwitch;
  char *leoSbdf = NULL;

  LeoI2CDriverType *i2cDriver;
  LeoDeviceType *leoDevice;
  DefaultArgsType defaultArgs = {.leoAddress = LEO_DEV_LEO_0,
                                 .switchAddress = LEO_DEV_MUX,
                                 .switchMode = 0x03, // 0x03 for Leo 0
                                 .serialnum = NULL,
                                 .bdf = NULL};
  enum { DEFAULT_ENUMS, ENUM_PROGRAM_e, ENUM_VERIFY_e, ENUM_CLEAN_e, ENUM_FORCE_e, ENUM_ALL_e, ENUM_EOL_e };

  struct option long_options[] = {DEFAULT_OPTIONS,
                                  {"program", required_argument, 0, 0},
                                  {"verify", required_argument, 0, 0},
                                  {"clean", no_argument, 0, 0},
                                  {"force", no_argument, 0, 0},
                                  {"all", no_argument, 0, 0},
                                  {0, 0, 0, 0}};

  const char *help_string[] = {
      DEFAULT_HELPSTRINGS, "Program SPI flash using target .mem file",
      "Verify SPI flash using target .mem file",
      "Overwrite persistent data (needed for downgrading FW version to below 0.6)",
      "Force programming, ignoring asic version compatibility check",
      "If board is Aurora 2, program both Leo devices",
  };

  while (1) {
    option = getopt_long_only(argc, argv, "hv", long_options, &option_index);
    if (option == -1)
      break;

    switch (option) {
    case 'h':
      usage(argv[0], long_options, help_string);
      break;
    case 0:
      switch (option_index) {
        DEFAULT_SWITCH_CASES(defaultArgs, long_options, help_string)
      case ENUM_PROGRAM_e:
        is_program = 1;
        strcpy(filename, optarg);
        break;
      case ENUM_VERIFY_e:
        is_verify = 1;
        strcpy(filename, optarg);
        break;
      case ENUM_CLEAN_e:
        is_clean = 1;
        break;
      case ENUM_FORCE_e:
        is_force = 1;
        break;
      case ENUM_ALL_e:
        is_all = 1;
        break;
      default:
        ASTERA_ERROR("option_index = %d undecoded", option_index);
      } // switch (option_index)
      break;
    default:
      ASTERA_ERROR("Default option = %d", option);
      usage(argv[0], long_options, help_string);
    }
  }

  if (defaultArgs.serialnum != NULL) {
    if (strlen(defaultArgs.serialnum) != SERIAL_VALID_SIZE) {
      usage(argv[0], long_options, help_string);
    }
    strcpy(conn.serialnum, defaultArgs.serialnum);
  }

  asteraLogSetLevel(2);

  if ((0 == is_program) && (0 == is_verify)) {
    usage(argv[0], long_options, help_string);
  }
  if (is_all) {
    if (IS_LEO) {
      ASTERA_ERROR("-leo and -all specified together, please choose one.");
      usage(argv[0], long_options, help_string);
    }
    IS_SVB = 0;
    IS_A1 = 0;
    IS_A2 = 1;
    defaultArgs.switchMode = 0x03; // 0x03 for Leo 0
  }

  if (defaultArgs.bdf != NULL) {
    int ret = -1;

    char *nextbdf = strtok(defaultArgs.bdf, ",");
    while (nextbdf != NULL) {
      ASTERA_INFO("Using device BDF %s", nextbdf);
      char *sysbdf = NULL;
      ret = bdfToSysfs(nextbdf, &sysbdf);
      if (ret != 0) {
        ASTERA_ERROR("Device BDF %s is not found in /sys/devices", nextbdf);
        exit(1);
      }

      strcpy(conn.bdf, sysbdf);
      strcat(conn.bdf, "/resource2");
      leoSbdf = basename(sysbdf);

      if (is_all) {
        ASTERA_ERROR("-all option is not supported with -bdf. "
                     "Multiple Leo Inband(PCIe) FW updates can be done by"
                     "comma separated BDFs, example: -bdf 17:00.0,17:00.1");
        exit(1);
      }
      i2cDriver = (LeoI2CDriverType *)calloc(1,sizeof(LeoI2CDriverType));
      leoDevice = (LeoDeviceType *)calloc(1,sizeof(LeoDeviceType));
      leoDevice->i2cDriver = i2cDriver;
      leoDevice->ignorePersistentDataFlag = is_clean;
      leoDevice->ignoreCompatibilityCheckFlag = is_force;

      char cmd[128];
      strcpy(cmd, "sudo setpci -s ");
      strcat(cmd, leoSbdf);
      strcat(cmd, " 0x4.b=0x42");
      // FIXME find a better way/library to enable PCIe Memory BARs
      rc = system(cmd);
      leoDevice->i2cDriver->pciefile = conn.bdf;
      if (0 != rc) {
        ASTERA_INFO("Leo device %s, setpci failed", leoSbdf);
      }

      if (is_program) {
        if (is_clean) {
          rc = leoFwUpdateFromFile(leoDevice, filename);
        } else {
          rc = leoFwUpdateTarget(leoDevice, filename, 0, 1);
        }
      }
      else if (is_verify) {
        leo_spi_verify_crc(leoDevice, filename);
      }

      free(leoDevice);
      free(i2cDriver);
      nextbdf = strtok(NULL, ",");
    }
  } else {

    for (ii = 0; ii < 1 + is_all; ii++) {
      if (ii == 1) {
        if (defaultArgs.leoAddress == LEO_DEV_LEO_1) {
          curLeoAddr = LEO_DEV_LEO_0;
          defaultArgs.switchMode = 0x03; // 0x03 for Leo 0
          leoId = 0;
        } else {
          curLeoAddr = LEO_DEV_LEO_1;
          defaultArgs.switchMode = 0x05; // 0x05 for Leo 1
          leoId = 1;
        }
      } else {
        curLeoAddr = defaultArgs.leoAddress;
        leoId = defaultArgs.leoAddress == LEO_DEV_LEO_0 ? 0 : 1;
        defaultArgs.switchMode = leoId ? 0x05 : 0x03;
      }
      ASTERA_INFO("FW update: Leo %d", leoId);

      rc = leoSetMuxAddress(i2cBus, &defaultArgs, conn);
      if (rc != LEO_SUCCESS) {
        ASTERA_ERROR("Failed to set Mux address");
        return rc;
      }

      if (defaultArgs.serialnum == NULL) {
        gpioHandle = asteraI2COpenConnection(i2cBus, LEO_DEV_GPIO_0);
      } else {
        gpioHandle = asteraI2COpenConnectionExt(i2cBus, LEO_DEV_GPIO_0, conn);
      }

      rc = setGpioSpiMux(gpioHandle, !leoId);
      asteraI2CCloseConnection(gpioHandle);
      if (0 != rc) {
        ASTERA_INFO("Failed to set SPI MUX");
        return rc;
      }

      i2cDriver = (LeoI2CDriverType *)calloc(1,sizeof(LeoI2CDriverType));
      i2cDriver->slaveAddr = curLeoAddr;
      i2cDriver->pciefile = NULL;
      leoDevice = (LeoDeviceType *)calloc(1,sizeof(LeoDeviceType));
      leoDevice->i2cDriver = i2cDriver;
      leoDevice->i2cBus = i2cBus;
      rc = leoInitDevice(leoDevice);
      if (rc != LEO_SUCCESS) {
         ASTERA_ERROR("Init device failed");
        return rc;
      }
      leoDevice->ignorePersistentDataFlag = is_clean;
      leoDevice->ignoreCompatibilityCheckFlag = is_force;

      if (defaultArgs.serialnum == NULL) {
        leoHandle = asteraI2COpenConnection(i2cBus, i2cDriver->slaveAddr);
      } else {
        leoHandle =
            asteraI2COpenConnectionExt(i2cBus, i2cDriver->slaveAddr, conn);
      }

      // Check connection
      i2cDriver->handle = leoHandle;
      rc = leoGetFWVersion(leoDevice);
      if (rc != 0) {
        ASTERA_ERROR("I2C connection failed");
        ASTERA_ERROR("leoId %d leoAddr %x switchAddr %x switchMode %d", leoId,
                     i2cDriver->slaveAddr, defaultArgs.switchAddress,
                     defaultArgs.switchMode);
        free(i2cDriver);
        free(leoDevice);
        return rc;
      }

      // Give Leo the SPI line
      aa_target_power(leoHandle, AA_TARGET_POWER_NONE);

      if (is_program) {
        if (is_clean) {
          rc = leoFwUpdateFromFile(leoDevice, filename);
        } else {
          rc = leoFwUpdateTarget(leoDevice, filename, 0, 1);
        }
      }
      else if (is_verify) {
        rc += leo_spi_verify_crc(leoDevice, filename);
      }

      asteraI2CCloseConnection(leoHandle);
      free(leoDevice);
      free(i2cDriver);

      if (0 != rc) {
        ASTERA_INFO("Leo firmware update failed.");
        exit(1);
      }
    }
  } // if (defaultArgs.bdf != NULL)
}

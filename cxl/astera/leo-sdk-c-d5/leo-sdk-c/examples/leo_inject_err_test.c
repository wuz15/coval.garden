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
 * @file leo_inject_err_test.c
 * @brief api test demonstrates usage of Leo SDK Inject Error API calls.
 * This is recommended for:
 *       - Print SDK version
 *       - Iinitialize Leo device
 *       - Set/Clear Error Inject registers
 * example UC error sudo ./leo_inject_err_test -switch 70 -addr 0 -errtype CE
 * -enable
 * @param[1-in]  start address, default 0
 * @param[2-in]  Error Type UCE or CE
 * @param[3-in]  (enable Or disable)
 * For more details on the args supported & usage
 * sudo ./leo_inject_err_test -help
 *
 */

#include "../include/leo_api.h"
#include "../include/leo_common.h"
#include "../include/leo_error.h"
#include "../include/leo_globals.h"
#include "../include/leo_i2c.h"
#include "../include/leo_spi.h"
#include "include/board.h"
#include "include/libi2c.h"

#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

uint64_t startAddr = 0;
uint8_t eccErrType = 0;
uint8_t eccEn = 0;

LeoErrorType doLeoInjectErrTest(LeoDeviceType *leoDevice, int leoHandle);

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  LeoErrorType rc;
  int leoHandle;
  int switchHandle;
  conn_t conn;
  char *leoSbdf = NULL;

  LeoLinkStatusType leoLinkStatus;
  LeoMemoryStatusType leoMemoryStatus;
  LeoI2CDriverType *i2cDriver;
  LeoDeviceType *leoDevice;
  LeoScrubConfigInfoType scrubConfigInfo;

  int option_index;
  int option;
  DefaultArgsType defaultArgs = {.leoAddress = LEO_DEV_LEO_0,
                                 .switchAddress = LEO_DEV_MUX,
                                 .switchHandle = -1,
                                 .switchMode = 0x03, // 0x03 for Leo 0
                                 .serialnum = NULL,
                                 .bdf = NULL};

  enum {
    DEFAULT_ENUMS,
    ENUM_ADDR_e,
    ENUM_ERRTYPE_e,
    ENUM_ENABLE_e,
    ENUM_EOL_e
  };

  struct option long_options[] = {DEFAULT_OPTIONS,
                                  {"addr", required_argument, 0, 0},
                                  {"errtype", required_argument, 0, 0},
                                  {"enable", no_argument, 0, 0},
                                  {0, 0, 0, 0}};

  const char *help_string[] = {DEFAULT_HELPSTRINGS, "start address, default 0",
                               "UCE or CE", "enable"};

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
      case ENUM_ADDR_e:
        startAddr = strtoul(optarg, NULL, 16);
        break;
      case ENUM_ERRTYPE_e:
        if (!strncmp(optarg, "UCE", 3))
          eccErrType = 1;
        else if (!strncmp(optarg, "CE", 2))
          eccErrType = 0;
        else
          eccErrType = 0;
        break;
      case ENUM_ENABLE_e:
        eccEn = 1;
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

  asteraLogSetLevel(ASTERA_LOG_LEVEL_INFO); // setting print level INFO
  ASTERA_INFO("\n\n LEO C SDK VERSION %s \n\n", leoGetSDKVersion());
  ASTERA_INFO("\n\n LEO Inject Error Test: \n\n");
  ASTERA_INFO("StartAddr: 0x%llx eccErrType: %d eccEn: %d\n", startAddr,
              eccErrType, eccEn);

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

      i2cDriver = (LeoI2CDriverType *)calloc(1,sizeof(LeoI2CDriverType));
      i2cDriver->pciefile = conn.bdf;
      char cmd[128];
      strcpy(cmd, "sudo setpci -s ");
      strcat(cmd, leoSbdf);
      strcat(cmd, " 0x4.b=0x42");
      // FIXME find a better way/library to enable PCIe Memory BARs
      rc = system(cmd);
      if (0 != rc) {
        ASTERA_INFO("Leo device %s, setpci failed", leoSbdf);
      }
      leoDevice = (LeoDeviceType *)calloc(1,sizeof(LeoDeviceType));
      leoDevice->i2cDriver = i2cDriver;

      doLeoInjectErrTest(leoDevice, leoHandle);
      nextbdf = strtok(NULL, ",");
    }
  } else {
    rc = leoSetMuxAddress(i2cBus, &defaultArgs, conn);
    if (rc != LEO_SUCCESS) {
      ASTERA_ERROR("Failed to set Mux address");
      return rc;
    }
    if (defaultArgs.serialnum == NULL) {
      leoHandle = asteraI2COpenConnection(i2cBus, defaultArgs.leoAddress);
    } else {
      leoHandle =
          asteraI2COpenConnectionExt(i2cBus, defaultArgs.leoAddress, conn);
    }

    i2cDriver = (LeoI2CDriverType *)calloc(1,sizeof(LeoI2CDriverType));
    i2cDriver->handle = leoHandle;
    i2cDriver->slaveAddr = defaultArgs.leoAddress;
    i2cDriver->i2cFormat = LEO_I2C_FORMAT_ASTERA;
    i2cDriver->pciefile = NULL;

    leoDevice = (LeoDeviceType *)calloc(1,sizeof(LeoDeviceType));
    leoDevice->i2cDriver = i2cDriver;
    leoDevice->i2cBus = i2cBus;
    rc = leoInitDevice(leoDevice);
    if (rc != LEO_SUCCESS) {
         ASTERA_ERROR("Init device failed");
        return rc;
    }
    doLeoInjectErrTest(leoDevice, leoHandle);
    asteraI2CCloseConnection(leoHandle);
  }

  char *str = (eccEn == 1) ? "Enable" : "Disable";

  ASTERA_INFO("Error Inject %s Done!", str);
}

LeoErrorType doLeoInjectErrTest(LeoDeviceType *leoDevice, int leoHandle) {
  LeoLinkStatusType leoLinkStatus;
  LeoMemoryStatusType leoMemoryStatus;
  LeoErrorType rc;

  leoDevice->controllerIndex = 0;
  // Flag to indicate lock has not been initialized.
  // later to initialize.
  leoDevice->i2cDriver->lockInit = 0;

  rc = leoInitDevice(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("Init device failed");
    return rc;
  }
  ASTERA_INFO("Leo leoInitDevice Passed: FW Running");

  rc = leoFWStatusCheck(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("Init device failed");
    return rc;
  }
  ASTERA_INFO("Leo FWStatusCheck Passed: FW Running");

  rc = leoGetFWVersion(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("leoGetFWVersion failed");
    return rc;
  }
  ASTERA_INFO("Leo leoGetFWVersion Passed: FW Running");
  ASTERA_INFO("Leo:CSDK:DEBUG:Leo FW Version:%02d.%02d, Build: %08ld",
              leoDevice->fwVersion.major, leoDevice->fwVersion.minor,
              leoDevice->fwVersion.build);

  rc = leoInjectError(leoDevice, startAddr, eccErrType, eccEn);
  if (rc != LEO_SUCCESS) {

    ASTERA_ERROR("leoPoisonChecker failed");
    return rc;
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }
}

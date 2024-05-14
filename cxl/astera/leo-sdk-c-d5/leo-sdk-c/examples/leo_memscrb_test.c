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
 * @file leo_memscrb_test.c
 * @brief api test demonstrates usage of  Leo SDK API calls related to
 * memory scrubbing.
 *
 *
 *  This is recommended for 3 types of scrubbing:
 *  background - command line argument when passed at running of test
 *  starts a background/patrol scrubbing test and this test can be
 *  configured to wait for a certain period (can be configured in
 *  leo_memscrb_test #line 127 bkgrdScrbTimeout = x secs), the test
 *  waits for the number seconds and reports status. The current default
 *  wait time is 5 seconds.
 *
 *  request - command line argument when passed at running of test
 *  starts Request scrubbing test - User can enter the start and end address
 *  of the request scrub test, the test waits for the completion and
 *  reports status.
 *
 *  OnDemand scrubbing is the default option when the test is run with
 *  no arguments, This test injects error and intitiates the TGC (Pattern
 *  generator and checker) followed by onDemand scrubbing.
 *  *  For more details on the args supported & usage
 * sudo ./leo_memscrb_test -help
 *
 */

#include "../include/DW_apb_ssi.h"
#include "../include/leo_api.h"
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

#define MAX_ERRORS 10

int isBackground = 0;
int isRequest = 0;
int isOndemand = 1;
int isInterval = 0;
int bkgrdScrbTimeout = 5; /* default is 5 seconds*/
LeoErrorType doLeoMemScrubTest(LeoDeviceType *leoDevice, int leoHandle);

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  LeoErrorType rc;

  int i;

  int leoHandle;
  int switchHandle;
  conn_t conn;

  LeoLinkStatusType leoLinkStatus;
  LeoMemoryStatusType leoMemoryStatus;
  LeoI2CDriverType *i2cDriver;
  LeoDeviceType *leoDevice;
  char *leoSbdf = NULL;

  uint8_t readSwitch;
  int option_index;
  int option;
  DefaultArgsType defaultArgs = {.leoAddress = LEO_DEV_LEO_0,
                                 .switchAddress = LEO_DEV_MUX,
                                 .switchHandle = -1,
                                 .switchMode = 0x03, // 0x03 for leo 0
                                 .serialnum = NULL,
                                 .bdf = NULL};

  enum {
    DEFAULT_ENUMS,
    ENUM_BACKGROUND_e,
    ENUM_REQUEST_e,
    ENUM_ONDEMAND_e,
    ENUM_STATUS_INTERVAL_e,
    ENUM_EOL_e
  };

  struct option long_options[] = {DEFAULT_OPTIONS,
                                  {"background", no_argument, 0, 0},
                                  {"request", no_argument, 0, 0},
                                  {"ondemand", no_argument, 0, 0},
                                  {"interval", required_argument, 0, 0},
                                  {0, 0, 0, 0}};

  const char *help_string[] = {
      DEFAULT_HELPSTRINGS,
      "Background(or Patrol) scrubbing,",                     // background
      "Request Scrubbing",                                    // request
      "On-demand scrubbing, that uses inject errors and TGC", // ondemand
      "Status interval for background scrubbing"              // interval
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
      case ENUM_BACKGROUND_e:
        isBackground = 1;
        isOndemand = 0;
        break;
      case ENUM_REQUEST_e:
        isRequest = 1;
        isOndemand = 0;
        break;
      case ENUM_ONDEMAND_e:
        isOndemand = 1;
        break;
      case ENUM_STATUS_INTERVAL_e:
        bkgrdScrbTimeout = strtoul(optarg, NULL, 10);
        isInterval = 1;
        break;
      default:
        ASTERA_ERROR("option_index = %d undecoded", option_index);
        usage(argv[0], long_options, help_string);
      } // switch (option_index)
      break;

    default:
      ASTERA_ERROR("Default option = %d", option);
      usage(argv[0], long_options, help_string);
    }
  }

  if (isInterval && !isBackground) {
    ASTERA_ERROR("Interval option is valid only for background scrubbing");
    usage(argv[0], long_options, help_string);
  }
  asteraLogSetLevel(2);

  /*  Standalone test that takes command line args for
      Option1. background(or Patrol) scrubbing,
      Option2. Req Scrubbing
      Option3 or default. on-demand scurbbing, that uses inject errors and TGC
   */
  if (!argv[1]) {
    ASTERA_INFO(
        " Now running default option of Err Inj + TGC + onDemand Scrub ");
  }

  if (defaultArgs.serialnum != NULL) {
    if (strlen(defaultArgs.serialnum) != SERIAL_VALID_SIZE) {
      usage(argv[0], long_options, help_string);
    }
    strcpy(conn.serialnum, defaultArgs.serialnum);
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
      doLeoMemScrubTest(leoDevice, leoHandle);
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
    doLeoMemScrubTest(leoDevice, leoHandle);
    asteraI2CCloseConnection(leoHandle);
  }
  ASTERA_INFO("Scrub test done");
}

LeoErrorType doLeoMemScrubTest(LeoDeviceType *leoDevice, int leoHandle) {
  LeoErrorType rc;
  LeoScrubConfigInfoType scrubConfigInfo;
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
    ASTERA_ERROR("Leo FWStatusCheck failed");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
  ASTERA_INFO("Leo FWStatusCheck Passed: FW is Running successfully");

  rc = leoGetFWVersion(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("leoGetFWVersion failed");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
  ASTERA_INFO("Leo:CSDK:Leo FW Version:%02d.%02d, Build: %08ld",
              leoDevice->fwVersion.major, leoDevice->fwVersion.minor,
              leoDevice->fwVersion.build);

  rc = leoGetAsicVersion(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("leoGetAsicVersion failed");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
  ASTERA_INFO("Leo:CSDK: Leo Asic Version %s",
              (LEO_ASIC_A0 == leoDevice->asicVersion.minor) ? "A0":"D5");

  ASTERA_INFO("");
  
  if (isBackground) {
    ASTERA_INFO("Option1 - Background(or Patrol) Scrub is enabled");

    /* Start Background scrubbing (Patrol)  */

    /* Default values for background scrubbing */
    scrubConfigInfo.bkgrdScrbEn =
        1; /*Change this to 1 or 0  to enable/disable bkgrd scrubbing*/
    scrubConfigInfo.onDemandScrbEn = 0;
    scrubConfigInfo.reqScrbEn = 0;
    scrubConfigInfo.bkgrdDpaEndAddr = 0xffffffffc0;
    scrubConfigInfo.bkgrdRoundInterval = 0x0a8c0;
    scrubConfigInfo.bkgrdCmdInterval = 0xf;
    scrubConfigInfo.bkgrdWrPoisonIfUncorrErr = 0x0;
    scrubConfigInfo.bkgrdScrbTimeout = bkgrdScrbTimeout;

    rc = leoMemoryScrubbing(leoDevice, &scrubConfigInfo);
    if (rc != LEO_SUCCESS) {
      ASTERA_ERROR("leoMemoryScrubbing-Background/Patrol scrubbing failed");
      return rc;
    }
  } else if (isRequest) {
    ASTERA_INFO("Option2 - Starting Request Scrub enable");
    /* Start Request scrubbing */
    scrubConfigInfo.ReqScrbDpaStartAddr = 0x0;
    scrubConfigInfo.ReqScrbDpaEndAddr = 0x040;
    scrubConfigInfo.ReqScrbCmdInterval = 0x100;
    scrubConfigInfo.bkgrdScrbEn =
        0; /*Change this to 1 or 0  to enable/disable bkgrd scrubbing*/
    scrubConfigInfo.reqScrbEn =
        1; /*Change this to 1 or 0  to enable/disable Req scrubbing*/
    scrubConfigInfo.onDemandScrbEn =
        0; /*Change this to 0x1 to enable Req scrubbing*/
    rc = leoMemoryScrubbing(leoDevice, &scrubConfigInfo);
    if (rc != LEO_SUCCESS) {
      ASTERA_ERROR("leoMemoryScrubbing-req scrb failed");
      return rc;
    }
  } else if (isOndemand) {
    ASTERA_INFO(
        "\nDefault option - Starting OnDemand Scrub enable with Inject Error");
    /* Default USE CASE: start OnDemand scrubbing */

    /*
     *  @param1: Error inject start address (default is 0x0)
     *  @param2: Error type CE or UE 1- Uncorrectable error (default is CE)
     *  @param3: Enable or disable error injection, 1 - Enable
     */

    rc = leoInjectError(leoDevice, 0x0, 0, 1);
    if (rc != LEO_SUCCESS) {
      ASTERA_ERROR("leoPoisonChecker failed");
      return rc;
    }

    /*
     * TGC
     *  @param1: TGC start address (default is 0x0)
     *  @param2: Data pattern (default 0xaabbccdd)
     *  @param3: Trans count (default 0x100)
     *  @param4: WritEnable (Enable -1, Disable -0)
     *  @param5: ReadEnable (Enable -1, Disable -0)
     *
     */
    LeoConfigTgc_t test_tgc = {.wrEn = 0,
                               .rdEn = 0,
                               .tranCnt = 0x100,
                               .startAddr = 0,
                               .dataPattern = 0xaabbccdd};
    LeoResultsTgc_t tgcResults;

    rc = leoPatternGeneratorandChecker(leoDevice, &test_tgc, &tgcResults);

    scrubConfigInfo.bkgrdScrbEn =
        0; /*Change this to 1 or 0  to enable/disable bkgrd scrubbing*/
    scrubConfigInfo.reqScrbEn =
        0; /*Change this to 1 or 0  to enable/disable Req scrubbing*/
    scrubConfigInfo.onDemandScrbEn =
        1; /*Change this to 0x1 to enable Req scrubbing*/
    scrubConfigInfo.onDemandWrPoisonIfUncorrErr = 1;

    rc = leoMemoryScrubbing(leoDevice, &scrubConfigInfo);
    if (rc != LEO_SUCCESS) {
      ASTERA_ERROR("leoMemoryScrubbing-OnDemand failed");
      return rc;
    }
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }
}

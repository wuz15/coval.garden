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
 * @file leo_tgc_test.c
 * @brief api test demonstrates usage of Leo SDK TGC test API calls.
 * This is recommended for:
 *       - Print SDK version
 *       - Iinitialize Leo device
 *       - Run TGC test
 * example sudo ./leo_tgc_test 0 0xaabbccdd 1 1 1
 * @param[1-in]  start address, default 0
 * @param[2-in]  dataPattern, default value 0xaabbccdd
 * @param[3-in]  Transaction count[0 to 15], 0 for 256 ... and 15 for 32G
 * @param[4-in]  Write enable 0-disable 1-Enable
 * @param[5-in]  Read enable 0-disable 1-Enable
 *
 * For more details on the args supported & usage
 * sudo ./leo_tgc_test -help
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

LeoConfigTgc_t test_tgc = {.wrEn = 0,
                           .rdEn = 0,
                           .tranCnt = 0x100,
                           .startAddr = 0,
                           .dataPattern = 0xaabbccdd};

LeoErrorType doLeoTgcTest(LeoDeviceType *leoDevice, int leoHandle);

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  LeoErrorType rc;

  uint32_t startAddr = 0;

  int option_index;
  int option;

  DefaultArgsType defaultArgs = {.leoAddress = LEO_DEV_LEO_0,
                                 .switchAddress = LEO_DEV_MUX,
                                 .switchHandle = -1,
                                 .switchMode = 0x03, // 0x03 for Leo 0
                                 .serialnum = NULL,
                                 .bdf = NULL};

  int leoHandle;
  int switchHandle;
  conn_t conn;
  char *leoSbdf = NULL;

  enum {
    DEFAULT_ENUMS,
    ENUM_ADDR_e,
    ENUM_PATTERN_e,
    ENUM_TCOUNT_e,
    ENUM_WRITE_e,
    ENUM_READ_e,
    ENUM_EOL_e
  };

  struct option long_options[] = {DEFAULT_OPTIONS,
                                  {"addr", required_argument, 0, 0},
                                  {"pattern", required_argument, 0, 0},
                                  {"tcount", required_argument, 0, 0},
                                  {"write", no_argument, 0, 0},
                                  {"read", no_argument, 0, 0},
                                  {0, 0, 0, 0}};

  const char *help_string[] = {
      DEFAULT_HELPSTRINGS,
      "start address, default 0",
      "dataPattern, default value 0xaabbccdd",
      "Transaction count[0 to 15], 0 for 256 ... and 15 for 32G",
      "Write enable",
      "Read enable "};

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
        test_tgc.startAddr = strtoul(optarg, NULL, 16);
        break;
      case ENUM_PATTERN_e:
        test_tgc.dataPattern = strtoul(optarg, NULL, 16);
        break;
      case ENUM_TCOUNT_e:
        test_tgc.tranCnt = strtoul(optarg, NULL, 16);
        break;
      case ENUM_WRITE_e:
        test_tgc.wrEn = 1;
        break;
      case ENUM_READ_e:
        test_tgc.rdEn = 1;
        break;
      default:
        ASTERA_ERROR("option_index = %d undecoded", option_index);
      } // switch (option_index)
      break;
    default:
      /* ASTERA_INFO("Using default TGC values\n"); */
      ASTERA_ERROR("Default option = %d", option);
      usage(argv[0], long_options, help_string);
    }
  }

  LeoLinkStatusType leoLinkStatus;
  LeoMemoryStatusType leoMemoryStatus;
  LeoI2CDriverType *i2cDriver;
  LeoDeviceType *leoDevice;

  LeoResultsTgc_t tgcResults;

  asteraLogSetLevel(ASTERA_LOG_LEVEL_INFO); // setting print level INFO

  ASTERA_INFO("\n\n LEO C SDK VERSION %s \n\n", leoGetSDKVersion());
  ASTERA_INFO("\n\n LEO TGC Error Test: \n\n");
  ASTERA_INFO(
      "StartAddr: 0x%x pattern: 0x%x transCnt: 0x%x wrEn: %d rdEr: %d \n",
      test_tgc.startAddr, test_tgc.dataPattern, test_tgc.tranCnt, test_tgc.wrEn,
      test_tgc.rdEn);

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
      doLeoTgcTest(leoDevice, leoHandle);
      nextbdf = strtok(NULL, ",");
    }
  } else {
    if (defaultArgs.serialnum != NULL) {
      if (strlen(defaultArgs.serialnum) != SERIAL_VALID_SIZE) {
        usage(argv[0], long_options, help_string);
      }
      strcpy(conn.serialnum, defaultArgs.serialnum);
    }

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
    doLeoTgcTest(leoDevice, leoHandle);
  }
  asteraI2CCloseConnection(leoHandle);
  ASTERA_INFO("TGC Test done!");
}

LeoErrorType doLeoTgcTest(LeoDeviceType *leoDevice, int leoHandle) {
  LeoErrorType rc;
  LeoResultsTgc_t tgcResults;

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

  /* Get preboot DDR and CXL config */
  LeoDdrConfigType ddrConfig;
  leoGetPrebootDdrConfig(leoDevice, &ddrConfig);

  ASTERA_INFO("DQ Width  : %d", ddrConfig.dqWidth);

  test_tgc.tranCnt |= ddrConfig.dqWidth;

  rc = leoPatternGeneratorandChecker(leoDevice, &test_tgc, &tgcResults);
  if (rc != LEO_SUCCESS) {

    ASTERA_ERROR("leoPatternGeneratorandChecker failed");
    return rc;
  }
  ASTERA_INFO("%x %x %x %x %x %x %x %x %x %x", tgcResults.tgcStatus,
              tgcResults.wrRdupperCnt, tgcResults.rdReqLowCnt,
              tgcResults.rdSubchnlCnt, tgcResults.wrSubchnlCnt,
              tgcResults.wrReqLowCnt, tgcResults.tgcError0,
              tgcResults.tgcError1, tgcResults.tgcDataMismatchVec,
              tgcResults.tgcDataMismatch);
  ASTERA_INFO("TGC STATUS");
  ASTERA_INFO("%s\t :0x%x", "tgc done                   ",
              tgcResults.tgcStatus & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "data mismatch corr error   ",
              (tgcResults.tgcStatus >> 1) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "data mismatch no error     ",
              (tgcResults.tgcStatus >> 2) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "error with no data mismatch",
              (tgcResults.tgcStatus >> 3) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "mem ecc uncorr error       ",
              (tgcResults.tgcStatus >> 4) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "error with data mismatch   ",
              (tgcResults.tgcStatus >> 5) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "mem curr active addr       ",
              (tgcResults.tgcStatus >> 8) & 0x0000007f);
  ASTERA_INFO("%s\t :0x%x", "wr resp error received     ",
              (tgcResults.tgcStatus >> 16) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "wr resp error sram addr    ",
              (tgcResults.tgcStatus >> 17) & 0x0000007f);
  ASTERA_INFO("%s\t :0x%x", "tgc state                  ",
              (tgcResults.tgcStatus >> 24) & 0x000000ff);
  ASTERA_INFO("%s\t :0x%x%x", "tgc wr requests count(bug) ",
              (tgcResults.wrRdupperCnt & 0x00000007), tgcResults.wrReqLowCnt);
  ASTERA_INFO("%s\t :0x%x%x", "tgc rd requests count(bug) ",
              (tgcResults.wrRdupperCnt >> 16) & 0x00000007,
              tgcResults.rdReqLowCnt);
  ASTERA_INFO("%s\t :0x%x", "tgc wr requests count(subchnl_cnt)",
              tgcResults.wrSubchnlCnt);
  ASTERA_INFO("%s\t :0x%x", "tgc rd requests count(subchnl_cnt)",
              tgcResults.rdSubchnlCnt);
  ASTERA_INFO("------------------------TGC ERROR---------------------");
  ASTERA_INFO("%s\t :0x%x", "csr expected meta                     ",
              tgcResults.tgcError0 & 0x00000003);
  ASTERA_INFO("%s\t :0x%x", "csr received meta                     ",
              (tgcResults.tgcError0 >> 2) & 0x00000003);
  ASTERA_INFO("%s\t :0x%x", "csr expected poison                   ",
              (tgcResults.tgcError0 >> 4) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "csr received poison                   ",
              (tgcResults.tgcError0 >> 5) & 0x00000001);
  ASTERA_INFO("%s\t :0x%x", "error count                           ",
              (tgcResults.tgcError0 >> 8) & 0x000000ff);
  ASTERA_INFO("%s\t :0x%x", "upper address                         ",
              (tgcResults.tgcError0 >> 16) & 0x0000ffff);
  ASTERA_INFO("%s\t :0x%x", "lower address                         ",
              tgcResults.tgcError1);
  ASTERA_INFO("%s\t :0x%x", "tgc data mismatch vec                 ",
              tgcResults.tgcDataMismatchVec);
  ASTERA_INFO("%s\t :0x%x", "tgc data mismatch expected data 16bits",
              tgcResults.tgcDataMismatch & 0x0000ffff);
  ASTERA_INFO("%s\t :0x%x", "tgc data mismatch received data 16bits",
              (tgcResults.tgcDataMismatch >> 16) & 0x0000ffff);
  ASTERA_INFO("-------------------------------------------------------");

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }
}

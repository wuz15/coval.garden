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
 * @file leo_read_tsod_example.c
 * @brief Read DIMM TSOD data
 *
 * When running this example for the first time, expect to wait for about 2
 * minutes for the DIMM temperature to be measured and reported.
 *
 * For more details on the args supported & usage
 * ./leo_read_tsod_example -help
 */

#include "../include/hal.h"
#include "../include/leo_api.h"
#include "../include/leo_api_internal.h"
#include "../include/leo_api_types.h"
#include "../include/leo_common.h"
#include "../include/leo_i2c.h"
#include "include/board.h"
#include <libgen.h>
#include <unistd.h>

/**
 * The following code for throttling is completely experimental and is specific
 * to the GSA test we ran for 30sec The actual values to be used should be
 * determined by the customer
 */
#define LEO_BW_THROTTLE_10 2  // 10%
#define LEO_BW_THROTTLE_25 4  // 22 which is approximately 25%
#define LEO_BW_THROTTLE_50 10 // 32 which is 50% of 64 hex
#define LEO_BW_THROTTLE_75 12 // approximately 48 which is 75% of 100 ( 64 hex)

LeoErrorType doLeoTsodRead(LeoDeviceType *leoDevice, int leoHandle,
                           int thresholdTemp, int is_throttle);

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  LeoErrorType rc;

  int i;

  int option_index;
  int option;
  int leoHandle;
  int switchHandle;
  int userThresholdTemp;
  uint8_t readSwitch;
  int is_throttle = 0;
  conn_t conn;
  char *leoSbdf = NULL;
  DefaultArgsType defaultArgs = {.leoAddress = LEO_DEV_LEO_0,
                                 .switchAddress = LEO_DEV_MUX,
                                 .switchHandle = -1,
                                 .switchMode = 0x03,
                                 .serialnum = NULL,
                                 .bdf = NULL};

  LeoI2CDriverType *i2cDriver;
  LeoDeviceType *leoDevice;

  LeoDimmTsodDataType leoDimmTsodData;

  enum { DEFAULT_ENUMS, ENUM_THROTTLE_e, ENUM_EOL_e };

  struct option long_options[] = {
      DEFAULT_OPTIONS, {"throttle", required_argument, 0, 0}, {0, 0, 0, 0}};

  const char *help_string[] = {DEFAULT_HELPSTRINGS,
                               "Start throttling bandwidth at specificed "
                               "threshold temperature in range [32, 105] (C)"};
  asteraLogSetLevel(ASTERA_LOG_LEVEL_INFO); // setting print level INFO

  while (1) {
    option = getopt_long_only(argc, argv, "h", long_options, &option_index);
    if (option == -1)
      break;

    switch (option) {
    case 'h':
      usage(argv[0], long_options, help_string);
      break;
    case 0:
      switch (option_index) {
        DEFAULT_SWITCH_CASES(defaultArgs, long_options, help_string)
      case ENUM_THROTTLE_e:
        is_throttle = 1;
        userThresholdTemp = strtoul(optarg, NULL, 10);
        ASTERA_INFO("userthresholdtemp: %d", userThresholdTemp);
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
      doLeoTsodRead(leoDevice, leoHandle, userThresholdTemp, is_throttle);
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
    doLeoTsodRead(leoDevice, leoHandle, userThresholdTemp, is_throttle);
    asteraI2CCloseConnection(leoHandle);
  }

  ASTERA_INFO("Exiting");
}

LeoErrorType doLeoTsodRead(LeoDeviceType *leoDevice, int leoHandle,
                           int thresholdTemp, int is_throttle) {
  int tsodReadSuccess = 0;
  int timeout = 0;
  int maxTimeout = 12; // 12 * 10 seconds = 2 minutes
  uint32_t dimm_idx;
  LeoErrorType rc;
  int maxTemp = 0;

  leoDevice->controllerIndex = 0;
  // Flag to indicate lock has not been initialized.
  // later to initialize.
  leoDevice->i2cDriver->lockInit = 0;

  // Enable TSOD polling
  leoWriteWordData(leoDevice->i2cDriver, GENERIC_CFG_REG_13, 1);

  /* Get preboot DDR and CXL config */
  LeoDdrConfigType ddrConfig;
  leoGetPrebootDdrConfig(leoDevice, &ddrConfig);

  /* Get TSOD Info */

  LeoDimmTsodDataType leoDimmTsodData;
  while (0 == tsodReadSuccess && timeout <= maxTimeout) {
    for (dimm_idx = 0; dimm_idx < ddrConfig.dpc * 2; dimm_idx++) {
      rc = leoGetTsodData(leoDevice, &leoDimmTsodData, dimm_idx);
      if (rc == LEO_SUCCESS) {
        tsodReadSuccess = 1;
      } else {
        continue;
      }
      ASTERA_INFO("DIMM #%d TSOD 0: %d.%02d", dimm_idx,
                  leoDimmTsodData.ts0WholeNum, leoDimmTsodData.ts0Decimal);
      ASTERA_INFO("DIMM #%d TSOD 1: %d.%02d", dimm_idx,
                  leoDimmTsodData.ts1WholeNum, leoDimmTsodData.ts1Decimal);

      if (is_throttle) {
        leoGetMaxDimmTemp(leoDevice, leoHandle, &maxTemp, thresholdTemp,
                          &leoDimmTsodData);
      }
    }

    if (0 == tsodReadSuccess) {
      ASTERA_INFO("TSOD Data not available yet. Waiting... (%d/%d)", timeout,
                  maxTimeout);
      timeout++;
      sleep(10);
    }
  }

  if (is_throttle) {
    int range = DDR_TEMP_THEORETICAL_MAX - thresholdTemp;
    int rangeIncrement = (float)range * .125;
    int throttleAmount = 100;

    ASTERA_INFO("maxtemp %d rangeincrement %d range %d", maxTemp,
                rangeIncrement, range);
    if (maxTemp > ((rangeIncrement * 0) + thresholdTemp) &&
        maxTemp <= ((rangeIncrement * 1) + thresholdTemp)) {
      throttleAmount = LEO_BW_THROTTLE_75;
    } else if (maxTemp > ((rangeIncrement * 1) + thresholdTemp) &&
               maxTemp <= ((rangeIncrement * 2) + thresholdTemp)) {
      throttleAmount = LEO_BW_THROTTLE_50;
    } else if (maxTemp > ((rangeIncrement * 2) + thresholdTemp) &&
               maxTemp <= ((rangeIncrement * 4) + thresholdTemp)) {
      throttleAmount = LEO_BW_THROTTLE_25;
    } else if (maxTemp > ((rangeIncrement * 4) + thresholdTemp)) {
      throttleAmount = LEO_BW_THROTTLE_10;
    }
    throttleAmount = LEO_BW_THROTTLE_75;
    // leoCmndThrottle(leoDevice, 8, 100, 1); //force 75% throttle
    leoCmndThrottle(leoDevice, throttleAmount, 100, 1); // cannot be 0
    double throttlePercent;
    ASTERA_INFO("Max temp %d, throttling %.2f%% (threshold %d)", maxTemp,
                (float)(100 - (float)throttleAmount), thresholdTemp);
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }
  return LEO_SUCCESS;
}

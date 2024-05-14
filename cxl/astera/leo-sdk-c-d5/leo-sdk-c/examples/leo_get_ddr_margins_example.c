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
 * @file leo_get_ddr_margins_example.c
 * @brief api test demonstrates usage of Leo SDK API calls.
 * This is recommended for:
 *       - Get ddrphy training margin data
 * init status etc)
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

#define TX_STEP_MV 5.5
#define RX_STEP_MV 4.169
#define CS_STEP_MV 5.5
#define CA_STEP_MV 5.5

static void printDdrphyTrainingMarginTable(uint8_t data1[2][4],
                                           uint8_t data2[2][4], float units,
                                           char *printUnits);
void doLeoGetDdrMarginsExample(LeoDeviceType *leoDevice);

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  LeoErrorType rc;
  int option_index;
  int option;
  DefaultArgsType defaultArgs = {.leoAddress = LEO_DEV_LEO_0,
                                 .switchAddress = LEO_DEV_MUX,
                                 .switchHandle = -1,
                                 .switchMode = 0x03, // 0x03 for Leo 0
                                 .serialnum = NULL,
                                 .bdf = NULL};

  int leoHandle;
  conn_t conn;
  char *leoSbdf = NULL;

  enum { DEFAULT_ENUMS, ENUM_EOL_e };

  struct option long_options[] = {DEFAULT_OPTIONS, {0, 0, 0, 0}};

  const char *help_string[] = {DEFAULT_HELPSTRINGS};

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
      default:
        ASTERA_ERROR("option_index = %d undecoded", option_index);
      } // switch (option_index)
      break;
    default:
      ASTERA_ERROR("Default option = %d", option);
      usage(argv[0], long_options, help_string);
    }
  }

  LeoI2CDriverType *i2cDriver;
  LeoDeviceType *leoDevice;

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

      doLeoGetDdrMarginsExample(leoDevice);
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
    if (leoHandle == -1) {
      ASTERA_ERROR("Failed to access Leo device");
      return 0;
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

    doLeoGetDdrMarginsExample(leoDevice);
    asteraI2CCloseConnection(leoHandle);
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }

  return rc;
}

void doLeoGetDdrMarginsExample(LeoDeviceType *leoDevice) {
  int rc;
  LeoDdrphyRxTxMarginType rxTxMargin[2];
  LeoDdrphyCsCaMarginType csCaMargin[2];
  LeoDdrphyQcsQcaMarginType qcsQcaMargin[2];
  rc = leoGetDdrphyRxTxTrainingMargins(leoDevice, 0, &rxTxMargin[0]);
  rc = leoGetDdrphyRxTxTrainingMargins(leoDevice, 1, &rxTxMargin[1]);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("Failed to get RxTx training margin data");
  }
  rc = leoGetDdrphyCsCaTrainingMargins(leoDevice, 0, &csCaMargin[0]);
  rc = leoGetDdrphyCsCaTrainingMargins(leoDevice, 1, &csCaMargin[1]);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("Failed to get CsCa training margin data");
  }
  rc = leoGetDdrphyQcsQcaTrainingMargins(leoDevice, 0, &qcsQcaMargin[0]);
  rc = leoGetDdrphyQcsQcaTrainingMargins(leoDevice, 1, &qcsQcaMargin[1]);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("Failed to get QcsQca training margin data");
  }

  /* RxTx Training Margins */
  ASTERA_INFO("\t RX Clk Delay Margin");
  printDdrphyTrainingMarginTable(rxTxMargin[0].rxClkDlyMargin,
                                 rxTxMargin[1].rxClkDlyMargin, 2.0 / 64.0,
                                 "UI");
  ASTERA_INFO("\t VREF Dac Margin");
  printDdrphyTrainingMarginTable(rxTxMargin[0].vrefDacMargin,
                                 rxTxMargin[1].vrefDacMargin, RX_STEP_MV * 2,
                                 "mV");
  ASTERA_INFO("\t TX DQ Delay Margin");
  printDdrphyTrainingMarginTable(rxTxMargin[0].txDqDlyMargin,
                                 rxTxMargin[1].txDqDlyMargin, 2.0 / 64.0,
                                 "UI");
  ASTERA_INFO("\t Dev VREF Margin");
  printDdrphyTrainingMarginTable(rxTxMargin[0].devVrefMargin, rxTxMargin[1].devVrefMargin, TX_STEP_MV * 2, "mV");


  /* CsCa Training Margins */
  ASTERA_INFO("\t Cs Delay Margin");
  printDdrphyTrainingMarginTable(csCaMargin[0].csDlyMargin, csCaMargin[1].csDlyMargin, 2.0 / 128.0, "clk");
  ASTERA_INFO("\t Cs VREF Margin");
  printDdrphyTrainingMarginTable(csCaMargin[0].csVrefMargin, csCaMargin[1].csVrefMargin, CS_STEP_MV * 2, "mV");
  ASTERA_INFO("\t Ca Delay Margin");
  printDdrphyTrainingMarginTable(csCaMargin[0].caDlyMargin, csCaMargin[1].caDlyMargin, 2.0 / 128.0, "clk");
  ASTERA_INFO("\t Ca VREF Margin");
  printDdrphyTrainingMarginTable(csCaMargin[0].caVrefMargin, csCaMargin[1].caVrefMargin, CA_STEP_MV * 2, "mV");

  /* QcsQca Training Margins */
  ASTERA_INFO("\t Qcs Delay Margin");
  printDdrphyTrainingMarginTable(qcsQcaMargin[0].qcsDlyMargin, qcsQcaMargin[1].qcsDlyMargin, 2.0 / 64.0, "UI");
  ASTERA_INFO("\t Qca Delay Margin");
  printDdrphyTrainingMarginTable(qcsQcaMargin[0].qcaDlyMargin, qcsQcaMargin[1].qcaDlyMargin, 2.0 / 64.0, "UI");
}

static void printDdrphyTrainingMarginTable(uint8_t data1[2][4],
                                           uint8_t data2[2][4], float units,
                                           char *printUnits) {
  int i;
  ASTERA_INFO("        rank (%s)", printUnits);
  ASTERA_INFO("ddrc ch 0       1       2       3 ");
  for (i = 0; i < 2; i++) {
    if (0 == strcmp(printUnits, "UI")) {
      ASTERA_INFO("%-4d %-2d %-7.02f %-7.02f %-7.02f %-7.02f ", 
        0, i, 
        data1[i][0] * units, 
        data1[i][1] * units, 
        data1[i][2] * units, 
        data1[i][3] * units);
    }
    else {
      ASTERA_INFO("%-4d %-2d %-7.2f %-7.2f %-7.2f %-7.2f ", 
        0, i, 
        data1[i][0] * units, 
        data1[i][1] * units, 
        data1[i][2] * units, 
        data1[i][3] * units);
    }
  }

  for (i = 0; i < 2; i++) {
    if (0 == strcmp(printUnits, "UI")) {
      ASTERA_INFO("%-4d %-2d %-7.02f %-7.02f %-7.02f %-7.02f ", 
        1, i, 
        data2[i][0] * units, 
        data2[i][1] * units, 
        data2[i][2] * units, 
        data2[i][3] * units);
    }
    else {
      ASTERA_INFO("%-4d %-2d %-7.2f %-7.2f %-7.2f %-7.2f ", 
        1, i, 
        data2[i][0] * units, 
        data2[i][1] * units, 
        data2[i][2] * units, 
        data2[i][3] * units);
    }
  }
}

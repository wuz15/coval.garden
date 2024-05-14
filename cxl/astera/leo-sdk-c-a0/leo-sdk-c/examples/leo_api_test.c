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
 * @file leo_api_test.c
 * @brief api test demonstrates usage of Leo SDK API calls.
 * This is recommended for:
 *       - Print SDK version
 *       - Iinitialize Leo device
 *       - Read, print FW version and status check
 *       - Link status check (Get details on the Speed, Width, linkstatus and
 * Mode)
 *       - Get details on the memory status (Channel number, training status,
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

LeoErrorType doLeoApiTest(LeoDeviceType *leoDevice, int leoHandle);
static void printDdrphyTrainingMarginTable(uint8_t data1[2][4],
                                           uint8_t data2[2][4]);

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
  // int switchHandle;
  conn_t conn;
  char *leoSbdf = NULL;

  enum { DEFAULT_ENUMS, ENUM_EOL_e };

  struct option long_options[] = {DEFAULT_OPTIONS, {0, 0, 0, 0}};

  const char *help_string[] = {DEFAULT_HELPSTRINGS};

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

  uint8_t readSwitch;

  if (defaultArgs.serialnum != NULL) {
    if (strlen(defaultArgs.serialnum) != SERIAL_VALID_SIZE) {
      usage(argv[0], long_options, help_string);
    }
    strcpy(conn.serialnum, defaultArgs.serialnum);
  }

  if (defaultArgs.bdf != NULL) {
    int ret = -1;

    char *nextbdf = strtok(defaultArgs.bdf, ",");
    ASTERA_INFO("\n\n LEO C SDK VERSION %s\n", leoGetSDKVersion());
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

      doLeoApiTest(leoDevice, leoHandle);
      nextbdf = strtok(NULL, ",");
    }
  } else {
    ASTERA_INFO("\n\n LEO C SDK VERSION %s\
      \n\n I2C Switch address: 0x%x (default: 0x%x)\
      \n usage: %s -switch [addr]\n\n",
                leoGetSDKVersion(), defaultArgs.switchAddress, LEO_DEV_MUX,
                argv[0]);

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

    doLeoApiTest(leoDevice, leoHandle);
    asteraI2CCloseConnection(leoHandle);
  }

  ASTERA_INFO("Exiting");
}

LeoErrorType doLeoApiTest(LeoDeviceType *leoDevice, int leoHandle) {
  int i;
  LeoErrorType rc;
  LeoLinkStatusType leoLinkStatus;
  LeoMemoryStatusType leoMemoryStatus;
  static char intToStr[2047];
  static char minorVerStr[10];

  leoDevice->controllerIndex = 0;
  // Flag to indicate lock has not been initialized.
  // later to initialize.
  leoDevice->i2cDriver->lockInit = 0;

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
    ASTERA_ERROR("If Leo ASIC used is D5 revision, use D5 version of SDK");
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

#if defined CHIP_A0
  if (leoDevice->asicVersion.minor == LEO_ASIC_D5) {
    ASTERA_ERROR("ASIC Version is D5 and SDK version is A0, please use Leo-D5 version of SDK");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
#endif

  // uint32_t fwMinorVersion = (int) leoDevice->fwVersion.minor & 0xFF;
  // sprintf(intToStr, "%02d", leoDevice->fwVersion.major);
  // sprintf(minorVerStr, ".%02d", leoDevice->fwVersion.minor);
  snprintf(intToStr, sizeof(intToStr), "%02d",
           (uint8_t)leoDevice->fwVersion.major);
  snprintf(minorVerStr, sizeof(minorVerStr), ".%02d",
           (uint8_t)leoDevice->fwVersion.minor);
  strcat(intToStr, minorVerStr);

  ASTERA_DEBUG("Leo:CSDK:Leo Firmware Version:%s", intToStr);

  /* if fw version is lower than LEO_MIN_FW_VERSION_SUPPORTED, we mark it not
   * compatible*/
  // strcpy (intToStr, "00.03"); /* fw version, this line of code is for manual
  // testing */
  if (leoSDKVersionCompatibilityCheck(intToStr, LEO_MIN_FW_VERSION_SUPPORTED) <
      0) {
    ASTERA_ERROR("Leo Firmware Version %s is not compatible with this SDK",
                 intToStr);
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
  /* if fw version is higher than LEO_MIN_FW_VERSION_SUPPORTED, we mark it
     compatible*/
  else if (leoSDKVersionCompatibilityCheck(intToStr,
                                           LEO_MIN_FW_VERSION_SUPPORTED) > 0) {
    ASTERA_INFO("Leo Firmware Version %s is higher than min FW ver required "
                "and compatible with this SDK",
                intToStr);
  }
  /* if fw version is equal to LEO_MIN_FW_VERSION_SUPPORTED, we mark it
     compatible*/
  else {
    ASTERA_INFO("Leo Firmware Version %s is compatible with this SDK",
                intToStr);
  }

  rc = leoGetLinkStatus(leoDevice, &leoLinkStatus);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("leoGetLinkStatus failed");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
  ASTERA_INFO(
      "Leo:CSDK:Leo Link Status:Speed=Gen%d, Width=x%d, Status=%s, Mode=%s",
      leoLinkStatus.linkSpeed, leoLinkStatus.linkWidth,
      leoLinkStatus.linkStatus ? "In Training Mode" : "Active",
      leoLinkStatus.linkMode ? "PCIE" : "CXL");
  for (i = 0; i < leoLinkStatus.linkWidth; ++i) {
    ASTERA_INFO("Leo:CSDK:Leo Link FOM:Lane=%d, FOM=0x%08x", i,
                leoLinkStatus.linkFoM[i]);
  }
  ASTERA_INFO("Leo leoGetLinkStatus Passed: FW Running");

  rc = leoGetMemoryStatus(leoDevice, &leoMemoryStatus);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("leoGetMemoryStatus failed");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
  ASTERA_INFO("Leo:CSDK:Leo Memory Status:");
  for (i = 0; i < LEO_MEM_CHANNEL_COUNT; ++i) {
    ASTERA_INFO(
        "Leo:CSDK:Leo Memory Channel=%d, initDone=0x%d, trainingDone=0x%d", i,
        leoMemoryStatus.ch[i].initDone, leoMemoryStatus.ch[i].trainingDone);
  }
  ASTERA_INFO("Leo leoGetMemoryStatus Passed: FW Running");

  /* Load Current FW */
  // TODO YW: function not working, review to see if still needed
  // rc = leoLoadCurrentFW(leoDevice);
  // if (rc != LEO_SUCCESS) {
  //   ASTERA_ERROR("leoLoadCurrentFW failed");
  //   asteraI2CCloseConnection(leoHandle);
  //   return rc;
  // }
  // ASTERA_INFO("Leo leoLoadCurrentFW Passed: ");

  
  rc = leoGetFWVersion(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("leoGetFWVersion failed");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }

  /* Get preboot DDR and CXL config */
  LeoDdrConfigType ddrConfig;
  LeoCxlConfigType cxlConfig;
  leoGetPrebootDdrConfig(leoDevice, &ddrConfig);
  leoGetPrebootCxlConfig(leoDevice, &cxlConfig);
  ASTERA_INFO("FW(Pre-Boot) Config:");
  ASTERA_INFO("DDR: # Ranks   : %d", ddrConfig.numRanks);
  ASTERA_INFO("     DDR Speed : %d", ddrConfig.ddrSpeed * 400 + 3200);
  ASTERA_INFO("     DQ Width  : %d", ddrConfig.dqWidth);
  ASTERA_INFO("     DPC       : %d DPC", ddrConfig.dpc);
  ASTERA_INFO("     Capacity  : %d GB",
              ddrConfig.capacity * 2 * 2); // 2 channels, 2 subchannels
  ASTERA_INFO("CXL: # Links   : %d", cxlConfig.numLinks);
  ASTERA_INFO("CXL: Link width: %d", cxlConfig.linkWidth);

  /* Get Spd Info */
  LeoSpdInfoType leoSpdInfo;
  int dimm_idx;
  char dimm_type[8];
  uint32_t dimm_map[4] = {0, 2, 1, 3};
  ASTERA_INFO("-------SPD--------");
  for (dimm_idx = 0; dimm_idx < ddrConfig.dpc * 2; dimm_idx++) {
    rc = leoGetSpdInfo(leoDevice, &leoSpdInfo, dimm_map[dimm_idx]);
    rc += leoGetSpdManufacturingInfo(leoDevice, &leoSpdInfo, dimm_map[dimm_idx]);
    if (rc != LEO_SUCCESS) {
      ASTERA_ERROR("SPD info not ready yet");
      continue;
    }

    if (1 == leoSpdInfo.ddr5SDRAM) {
      ASTERA_INFO("DIMM #%d DDR5 SDRAM", dimm_idx);
    } else {
      ASTERA_ERROR("Undecoded DIMM");
      continue;
    }
    switch (leoSpdInfo.dimmType) {
    case 0x01:
      strcpy(dimm_type, "RDIMM  ");
      break;
    case 0x02:
      strcpy(dimm_type, "UDIMM  ");
      break;
    case 0x03:
      strcpy(dimm_type, "SODIMM ");
      break;
    case 0x04:
      strcpy(dimm_type, "LRDIMM ");
      break;
    default:
      ASTERA_ERROR("Undecoded dimm type");
      break;
    }

    char manufacturerString[40];
    decodeDimmManufacturer(leoSpdInfo.manufacturerIdCode, manufacturerString);
    ASTERA_INFO("%s - 20%02x Week %02x ", manufacturerString, leoSpdInfo.dimmDateCodeBcd >> 8, leoSpdInfo.dimmDateCodeBcd & 0xff);
    ASTERA_INFO("[PN] %s",  leoSpdInfo.partNumber);
    ASTERA_INFO("[SN] %04X%02X%04X%08X", leoSpdInfo.manufacturerIdCode, leoSpdInfo.manufactureringLoc, leoSpdInfo.dimmDateCodeBcd, leoSpdInfo.serialNumber);
    
    
    ASTERA_INFO("DIMM TYPE: %s", dimm_type);
    ASTERA_INFO("%d Gb, %d die", leoSpdInfo.densityInGb,
                leoSpdInfo.diePerPackage);
    ASTERA_INFO("%d GB %dRx%d", leoSpdInfo.capacityInGb, leoSpdInfo.rank,
                leoSpdInfo.ioWidth);
    if (leoSpdInfo.ddrSpeedMax != 0){
      uint32_t speed = 2 * (1000000.0/leoSpdInfo.ddrSpeedMax);
      int rest = speed % 100;
      int maxSpeed = speed -  rest;
      if (rest > 50) {
        maxSpeed += 100;
      }
      ASTERA_INFO("%d MT/s", maxSpeed);
    }
    ASTERA_INFO("------------------");
  }

  /* DDR sanity check */
  if (leoSpdInfo.rank != ddrConfig.numRanks / ddrConfig.dpc) {
    ASTERA_ERROR("Config mismatch (# ranks): SPD: %d, config: %d",
                 leoSpdInfo.rank, ddrConfig.numRanks);
  }
  if (leoSpdInfo.ioWidth != ddrConfig.dqWidth) {
    ASTERA_ERROR("Config mismatch (DQ width): SPD: %d, config: %d",
                 leoSpdInfo.ioWidth, ddrConfig.dqWidth);
  }
  if (leoSpdInfo.capacityInGb != ddrConfig.capacity * 2 / ddrConfig.dpc) {
    ASTERA_ERROR("Config mismatch (capacity): %dGB, config: %dGB",
                 leoSpdInfo.capacityInGb,
                 ddrConfig.capacity * 2 * ddrConfig.dpc);
  }

  /* Get Leo ID */
  LeoIdInfoType leoIdInfo;
  char boardType[10] = "Undecoded";
  LeoErrorType err = leoGetLeoIdInfo(leoDevice, &leoIdInfo);

  if (err == LEO_SUCCESS) {
    switch (leoIdInfo.boardType) {
    case LEO_SVB:
      strcpy(boardType, "Leo SVB  ");
      break;
    case LEO_A1:
      strcpy(boardType, "Aurora 1 ");
      break;
    case LEO_A2:
      strcpy(boardType, "Aurora 2 ");
      break;
    default:
      strcpy(boardType, "Undecoded");
      break;
    }
    ASTERA_INFO("Board type: %s", boardType);
    ASTERA_INFO("LEO ID %d", leoIdInfo.leoId);
  } else {
    ASTERA_WARN("Board type and Leo ID undecoded.");
  }

  /* Get DSID */
  uint64_t dsid;
  rc = leoGetDsid(leoDevice, &dsid);
  ASTERA_INFO("DSID %08x %08x", (uint32_t)(dsid >> 32),
              (uint32_t)dsid & 0xffffffff);

  /* Get CMAL stats */
  LeoCmalStatsType leoCmalStats;
  leoGetCmalStats(leoDevice, &leoCmalStats);
  ASTERA_INFO("CMAL: RRSP DDR returned poison count");
  ASTERA_INFO("      link [0, 1] = [%d, %d]",
              leoCmalStats.rrspDdrReturnedPoisonCount[0],
              leoCmalStats.rrspDdrReturnedPoisonCount[1]);
  ASTERA_INFO("      RRSP RDP   caused poison count");
  ASTERA_INFO("      link [0, 1] = [%d, %d]",
              leoCmalStats.rrspRdpCausedPoisonCount[0],
              leoCmalStats.rrspRdpCausedPoisonCount[1]);

  LeoFailVectorType leoFailVector;
  leoGetFailVector(leoDevice, &leoFailVector);
  if(leoFailVector.failVector) 
  {
    if(leoFailVector.failVector & (1 << LEO_FAIL_VEC_CATRIP)) 
    {
         ASTERA_ERROR("LEO: FAIL VECTOR CATTRIP SET");
    }
    if(leoFailVector.failVector & (1 << LEO_FAIL_VEC_BAD_DSID)) 
    {
         ASTERA_ERROR("LEO: Unprogrammed Leo ASIC");
    }
    if(leoFailVector.failVector & (1 << LEO_FAIL_VEC_GUARD_CH0_FAIL)) 
    {
         ASTERA_ERROR("LEO: Failure on DDR guard rails on channel 0");
    }
    if(leoFailVector.failVector & (1 << LEO_FAIL_VEC_GUARD_CH1_FAIL)) 
    {
         ASTERA_ERROR("LEO: Failure on DDR guard rails on channel 1");
    }
    if(leoFailVector.failVector & (1 << LEO_FAIL_VEC_GUARD_CHECK_FAIL)) 
    {
         ASTERA_ERROR("LEO: DDR guard rail check failed.. Leo not turned on as a Type 3 device");
    }
  }

  /* Get cattrip count */
  LeoPersistentDataValueType cattrip;
  rc = leoGetPersistentData(leoDevice, PERSISTENT_DATA_ID_CATTRIP, &cattrip);
  if (cattrip.value != 0) {
    ASTERA_WARN("cattrip event counter: since boot: %d, total: %d",
                cattrip.countSinceBoot, cattrip.countTotal);
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }

  return rc;
}

/*
 * Copyright 2023 Astera Labs, Inc. or its affiliates. All Rights Reserved.
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
 * @file leo_telemetry.c
 * @brief reference/example to gather DDR and CXL telemetry from a Leo Device
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
#include <inttypes.h>
#include <string.h>

LeoErrorType doLeoTelemetryCxlSampling(LeoDeviceType *leoDevice,
                                       int cxllink, int seconds);
LeoErrorType doLeoTelemetryDdrSampling(LeoDeviceType *leoDevice,
                                       int ddrch, int seconds);
LeoErrorType doLeoTelemetryDatapathSampling(LeoDeviceType *leoDevice,
                                       int seconds);
static int all = 0;

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  LeoErrorType rc;
  int option_index;
  int option;
  int seconds = 0;
  int ddrch = -1;
  int cxllink = -1;
  int datapath = -1;
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

  enum {
    DEFAULT_ENUMS,
    SAMPLE_e,
    DDRCH_e,
    DATAPATH_e,
    CXL_e,
    ALL_e,
  };


  struct option long_options[] = {
      DEFAULT_OPTIONS, {"sample", required_argument, 0, 0},
                       {"ddr", required_argument, 0, 0},
                       {"datapath", no_argument, 0, 0},
                       {"cxl", required_argument, 0, 0},
                       {"all", no_argument, 0, 0}, {0, 0, 0, 0}};

  const char *help_string[] = {DEFAULT_HELPSTRINGS, "(Optional) number of seconds to sample the counters",
                                                    "(Optional) include DDR channel [0, 1] counters",
                                                    "(Optional) include data-path counters",
                                                    "(Optional) include CXL Link [0, 1] counters",
                                                    "include all the above counters"};

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
      case SAMPLE_e:
        seconds = strtoul(optarg, NULL, 10);
        break;
      case DDRCH_e:
        ddrch = strtoul(optarg, NULL, 10);
        break;
      case DATAPATH_e:
        datapath = 1;
        break;
      case CXL_e:
        cxllink = strtoul(optarg, NULL, 10);
        break;
      case ALL_e:
        all = 1;
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

  LeoI2CDriverType *i2cDriver;
  LeoDeviceType *leoDevice;

  uint8_t readSwitch;

  if (defaultArgs.serialnum != NULL) {
    if (strlen(defaultArgs.serialnum) != SERIAL_VALID_SIZE) {
      usage(argv[0], long_options, help_string);
    }
    strcpy(conn.serialnum, defaultArgs.serialnum);
  }

  if (!all) {
    if ((cxllink < 0) && (datapath < 0) && (ddrch < 0)) {
      ASTERA_ERROR("checkl usage");
    }
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

      if (all) {
        doLeoTelemetryCxlSampling(leoDevice, cxllink, seconds);
        doLeoTelemetryDdrSampling(leoDevice, ddrch, seconds);
        doLeoTelemetryDatapathSampling(leoDevice, seconds);
      } else if (cxllink == 0 || cxllink == 1) {
        doLeoTelemetryCxlSampling(leoDevice, cxllink, seconds);
      } else if (ddrch == 0 || ddrch == 1) {
        doLeoTelemetryDdrSampling(leoDevice, ddrch, seconds);
      } else if (datapath == 1) {
        doLeoTelemetryDatapathSampling(leoDevice, seconds);
      }

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

    if (all) {
      doLeoTelemetryCxlSampling(leoDevice, cxllink, seconds);
      doLeoTelemetryDdrSampling(leoDevice, ddrch, seconds);
      doLeoTelemetryDatapathSampling(leoDevice, seconds);
    } else if (cxllink == 0 || cxllink == 1) {
      doLeoTelemetryCxlSampling(leoDevice, cxllink, seconds);
    } else if (ddrch == 0 || ddrch == 1) {
      doLeoTelemetryDdrSampling(leoDevice, ddrch, seconds);
    } else if (datapath == 1) {
      doLeoTelemetryDatapathSampling(leoDevice, seconds);
    }

    asteraI2CCloseConnection(leoHandle);
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }

  ASTERA_INFO("Exiting");
}

void printCxlTelemetry(int cxllink, LeoCxlTelemetryType tel)
{
  ASTERA_INFO("CXL Link%d S2M NDR Count : %" PRIu64, cxllink, (tel.s2m_ndr_c));
  ASTERA_INFO("CXL LINK%d S2M DRS Count : %" PRIu64, cxllink, (tel.s2m_drs_c));
  ASTERA_INFO("CXL LINK%d M2S REQ Count : %" PRIu64, cxllink, (tel.m2s_req_c));
  ASTERA_INFO("CXL LINK%d M2S RWD Count : %" PRIu64, cxllink, (tel.m2s_rwd_c));
  ASTERA_INFO("CXL Link%d RAS Stat Rx Correctable: %" PRIu64, cxllink, (tel.rasRxCe));
  ASTERA_INFO("CXL Link%d RAS Stat Rx UnCorrectable[M2s_RwD.HDR]: %" PRIu64, cxllink, (tel.rasRxUeRwdHdr));
  ASTERA_INFO("CXL Link%d RAS Stat Rx UnCorrectable[M2s_Req.HDR]: %" PRIu64, cxllink, (tel.rasRxUeReqHdr));
  ASTERA_INFO("CXL Link%d RAS Stat Rx UnCorrectable[M2s_RwD.BE]: %" PRIu64, cxllink, (tel.rasRxUeRwdBe));
  ASTERA_INFO("CXL Link%d RAS Stat Rx UnCorrectable[M2s_RwD.DATA]:  %" PRIu64, cxllink, (tel.rasRxUeRwdData));
  ASTERA_INFO("CXL Link%d M2s_RwD header Uncorrectable Errors:  %" PRIu64, cxllink, (tel.rwdHdrUe));
  ASTERA_INFO("CXL Link%d M2s RwD header HDM Errors:  %" PRIu64, cxllink, (tel.rwdHdrHdm));
  ASTERA_INFO("CXL Link%d M2s RwD header Unsupported field Errors:  %" PRIu64, cxllink, (tel.rwdHdrUfe));
  ASTERA_INFO("CXL Link%d M2s Req header Uncorrectable Errors:  %" PRIu64, cxllink, (tel.reqHdrUe));
  ASTERA_INFO("CXL Link%d M2s Req header HDM Errors:  %" PRIu64, cxllink, (tel.reqHdrHdm));
  ASTERA_INFO("CXL Link%d M2s Req header Unsupported field Errors:  %" PRIu64, cxllink, (tel.reqHdrUfe));
}

void printDdrTelemetry(int ddrch, LeoDdrTelemetryType tel)
{
  ASTERA_INFO("DDR channel%d wrsp addr err Count: %" PRIu64, ddrch, tel.ddrchwaec);
  ASTERA_INFO("DDR channel%d rrsp ddr crc err Count: %" PRIu64, ddrch, tel.ddrchrdcrc);
  ASTERA_INFO("DDR channel%d rrsp ddr uncorr err Count: %" PRIu64, ddrch, tel.ddrchrduec);
  ASTERA_INFO("DDR channel%d rrsp ddr corr err Count: %" PRIu64, ddrch, tel.ddrchrdcec);
  ASTERA_INFO("DDR channel%d Read Activate Count :  %" PRIu64, ddrch, tel.ddrRdActCount);
  ASTERA_INFO("DDR channel%d Precharge Count :  %" PRIu64, ddrch, tel.ddrPreChCount);
  ASTERA_INFO("DDR channel%d Refresh Count :  %" PRIu64, ddrch, tel.ddrRefCount);
}

void printDatapathTelemetry(LeoDatapathTelemetryType tel)
{
  ASTERA_INFO("DDR TGC corr err Count: %" PRIu64, tel.ddrTgcCe);
  ASTERA_INFO("DDR TGC uncorr err Count: %" PRIu64, tel.ddrTgcUe);
  ASTERA_INFO("DDR Ondemand scrub served Count: %" PRIu64, tel.ddrOssc);
  ASTERA_INFO("DDR Ondemand scrub dropped Count: %" PRIu64, tel.ddrOsdc);
  ASTERA_INFO("DDR Background scrub corr err Count: %llu", tel.ddrbscec);
  ASTERA_INFO("DDR Background scrub uncorr err Count: %llu", tel.ddrbsuec);
  ASTERA_INFO("DDR Background scrub other err Count: %llu", tel.ddrbsoec);
}

LeoErrorType doLeoTelemetryCxlSampling(LeoDeviceType *leoDevice,
                                       int cxllink,
                                       int seconds)
{
  LeoErrorType rc;
  LeoCxlTelemetryType full, sample;

  if (all == 1) {
    for (cxllink = 0; cxllink <= 1; cxllink++) {
      rc = leoGetCxlTelemetry(leoDevice, cxllink, &full, &sample, seconds);
      printCxlTelemetry(cxllink, full);
      if (seconds) {
        ASTERA_INFO("=== Sampled Values after %d seconds ===", seconds);
        printCxlTelemetry(cxllink, sample);
        ASTERA_INFO("=== Done Sampled Values after %d seconds ===", seconds);
      }
    }
  } else {
      rc = leoGetCxlTelemetry(leoDevice, cxllink, &full, &sample, seconds);
      printCxlTelemetry(cxllink, full);
      if (seconds) {
        ASTERA_INFO("=== Sampled Values after %d seconds ===", seconds);
        printCxlTelemetry(cxllink, sample);
        ASTERA_INFO("=== Done Sampled Values after %d seconds ===", seconds);
      }
  }
  return rc;
}

LeoErrorType doLeoTelemetryDdrSampling(LeoDeviceType *leoDevice,
                                       int ddrch,
                                       int seconds)
{
  LeoErrorType rc;
  LeoDdrTelemetryType full, sample;
  if (all == 1) {
    for (ddrch = 0; ddrch <= 1; ddrch++) {
      rc = leoGetDdrTelemetry(leoDevice, ddrch, &full, &sample, seconds);
      printDdrTelemetry(ddrch, full);
      if (seconds) {
        ASTERA_INFO("=== Sampled Values after %d seconds ===", seconds);
        printDdrTelemetry(ddrch, sample);
        ASTERA_INFO("=== Done Sampled Values after %d seconds ===", seconds);
      }
    }
  } else {
      rc = leoGetDdrTelemetry(leoDevice, ddrch, &full, &sample, seconds);
      printDdrTelemetry(ddrch, full);
      if (seconds) {
        ASTERA_INFO("=== Sampled Values after %d seconds ===", seconds);
        printDdrTelemetry(ddrch, sample);
        ASTERA_INFO("=== Done Sampled Values after %d seconds ===", seconds);
      }
  }


  return rc;
}

LeoErrorType doLeoTelemetryDatapathSampling(LeoDeviceType *leoDevice,
                                       int seconds)
{
  LeoErrorType rc;
  LeoDatapathTelemetryType full, sample;

  leoDevice->controllerIndex = 0;

  rc = leoGetDatapathTelemetry(leoDevice, &full, &sample, seconds);
  printDatapathTelemetry(full);
  if (seconds) {
    ASTERA_INFO("=== Sampled Values after %d seconds ===", seconds);
    printDatapathTelemetry(sample);
    ASTERA_INFO("=== Done Sampled Values after %d seconds ===", seconds);
  }

  return rc;
}


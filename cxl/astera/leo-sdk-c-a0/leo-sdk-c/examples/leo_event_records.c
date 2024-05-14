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
 * @file leo_event_record.c
 * @brief reference demonstrates usage of Leo event logs Get and Clear.
 * This is recommended for:
 *       - Initialize Leo device
 *       - Read, print FW version and status check
 *       - Leo error events get
 *       - Leo error events clear
 */

#include "../include/DW_apb_ssi.h"
#include "../include/leo_api.h"
#include "../include/leo_common.h"
#include "../include/leo_error.h"
#include "../include/leo_evt_rec_mgr.h"
#include "../include/leo_globals.h"
#include "../include/leo_i2c.h"
#include "../include/leo_spi.h"
#include "include/aa.h"
#include "include/board.h"
#include "include/libi2c.h"
#include "include/leo_common_global.h"

#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LeoErrorType doLeoEvents(LeoDeviceType *leoDevice, int leoHandle, int action,
                int log, uint32_t clrAll, uint32_t numRecs, uint16_t *handles);
LeoErrorType doLeoEventsGet(LeoDeviceType *leoDevice, int log);
LeoErrorType doLeoEventsClear(LeoDeviceType *leoDevice, int log,
                uint32_t clrAll, uint32_t numRecs, uint16_t *handles);

const char *help_string[] = {DEFAULT_HELPSTRINGS,
                              "action, get -or- clear",
                              "logtype, info, warn, failure or fatal",
                              "clrall, (optional) to clear all records alltogether",
                              "numrecs, (optional) no of event records to clear, valid only if clrall is not used",
                              "handles, (optional) comma separate event handles list, valid only if clrall is not used"};

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  int actionType = -1;
  int logType = ALL_LOGS;
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
  uint32_t numRecs = 0;
  uint32_t num_handles = 0;
  uint32_t clrAll = 0;
  char str_handles[256];
  uint16_t handles[MAX_EVT_RECS];

  enum { DEFAULT_ENUMS, ENUM_ACTION_e, ENUM_LOGTYPE_e, ENUM_CLRALL_e, ENUM_NUMRECS_e, ENUM_HANDLE_e, ENUM_EOL_e };

  struct option long_options[] = {DEFAULT_OPTIONS,
                                  {"action", required_argument, 0, 0},
                                  {"logtype", required_argument, 0, 0},
                                  {"clrall", no_argument, 0, 0},
                                  {"numrecs", required_argument, 0, 0},
                                  {"handles", required_argument, 0, 0},
                                  {0, 0, 0, 0}};

  memset(str_handles, 0, 250);

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
      case ENUM_ACTION_e:
        if (!strncmp(optarg, "get", 3))
          actionType = GET_EVENTS;
        else if (!strncmp(optarg, "clear", 5))
          actionType = CLEAR_EVENTS;
        else
          actionType = -1;
        break;
      case ENUM_LOGTYPE_e:
        if (!strncmp(optarg, "info", 4))
          logType = CXL_PMBOX_INFO_LOG;
        else if (!strncmp(optarg, "warn", 4))
          logType = CXL_PMBOX_WARN_LOG;
        else if (!strncmp(optarg, "failure", 7))
          logType = CXL_PMBOX_FAILURE_LOG;
        else if (!strncmp(optarg, "fatal", 5))
          logType = CXL_PMBOX_FATAL_LOG;
        else
          logType = -1;
        break;
      case ENUM_CLRALL_e:
          clrAll = 1;
        break;
      case ENUM_NUMRECS_e:
          numRecs = atoi(optarg);
        break;
      case ENUM_HANDLE_e:
        memset(handles, 0, MAX_EVT_RECS*sizeof(uint16_t));
        strcpy(str_handles, optarg);
        num_handles = populate_handles(str_handles, handles);
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

  if (logType < 0) {
    ASTERA_ERROR("invalid logtype! please check usage");
    exit(1);
  }

  if (actionType == CLEAR_EVENTS) {
    if (clrAll && (numRecs != 0)) {
      ASTERA_ERROR("numrecs must be 0 if clearing all the records");
      exit(1);
    }
    if (numRecs != num_handles) {
      ASTERA_ERROR("numrecs does not match with no of handles provided");
      exit(1);
    }
    if (!clrAll && !numRecs) {
      ASTERA_ERROR("either clrall or clear numrecs are required, both cannot be 0");
      exit(1);
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

      doLeoEvents(leoDevice, leoHandle, actionType, logType, clrAll, numRecs, handles);
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

    doLeoEvents(leoDevice, leoHandle, actionType, logType, clrAll, numRecs, handles);
    asteraI2CCloseConnection(leoHandle);
  }

  ASTERA_INFO("Exiting");
}

LeoErrorType doLeoEvents(LeoDeviceType *leoDevice, int leoHandle, int action,
                         int log, uint32_t clrAll, uint32_t numRecs, uint16_t *handles) {
  int i;
  LeoErrorType rc;
  LeoLinkStatusType leoLinkStatus;
  LeoMemoryStatusType leoMemoryStatus;

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

  rc = leoGetFWVersion(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("leoGetFWVersion failed");
    asteraI2CCloseConnection(leoHandle);
    return rc;
  }
  ASTERA_INFO("Leo:CSDK:Leo FW Version:%02d.%02d, Build: %08ld",
              leoDevice->fwVersion.major, leoDevice->fwVersion.minor,
              leoDevice->fwVersion.build);
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

  if (leoDevice->i2cDriver->pciefile != NULL) {

    if (action == GET_EVENTS) {
      doLeoEventsGet(leoDevice, log);
    } else if (action == CLEAR_EVENTS) {
      doLeoEventsClear(leoDevice, log, clrAll, numRecs, handles);
    } else {
      ASTERA_ERROR("Leo unknown action requested, please see help [-h]");
    }
  } else {
    ASTERA_ERROR("Leo Get/Clear events is not supported via I2C, see help -h");
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }

  return LEO_SUCCESS;
}

LeoErrorType doLeoEventsGet(LeoDeviceType *leoDevice, int log) {
  uint32_t readval = 1;
  uint32_t outl = 0;
  uint8_t payload[256] = {0};
  leo_one_evt_log_t *eventlog = (leo_one_evt_log_t *)payload;
  int ii = 0;
  int jj = 0;
  LeoErrorType status = LEO_SUCCESS;
  uint64_t cmd = 0;

  if (log > CXL_PMBOX_FATAL_LOG) {
    ASTERA_ERROR("Leo incorrect log event requested, please see help [-h]");
    return LEO_FAILURE;
  }

  ii = 0;
  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d] waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG, log);

  cmd = (1 << 16);
  cmd |= CXL_PMBOX_GET_EVT_RECS;
  ASTERA_INFO("Issuing GET_EVENT_RECORDS command to primary mailbox, cmd: 0x%llx", cmd);
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG, (uint32_t)cmd);
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG + 4, (uint32_t)(cmd >> 32));
  usleep(100);

  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, 0x1);
  usleep(100);

  ii = 0;
  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d] waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  readval = 1;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_STS_REG + 4, &readval);
  readval &= 0xFFFF;
  if (readval != 0) {
    ASTERA_ERROR("GET_EVENT_RECORDS command failed with status 0x%x", readval);
    return LEO_FAILURE;
  }

  readval = 0;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG, &readval);
  outl = readval >> CXL_PMBOX_GET_CMD_LEN_SHIFT;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG + 4, &readval);
  outl |= ((readval & 0x1F) << CXL_PMBOX_GET_CMD_LEN_SHIFT);
  ASTERA_INFO("GET_EVENT_RECORDS payload length = %d bytes", outl);

  memset(payload, 0, LEO_CXL_MBOX_MAX_PAYLOAD_SIZE);

  for (ii = 0; ii < outl; ii += 4) {
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + ii,
                    &readval);
    *(uint32_t *)(payload + ii) = readval;
  }

  printevent(eventlog, log);

  // check for both to avoid spurious returns, rec_cnt is 1 for now
  if (eventlog->more_evt_recs) {
    status = doLeoEventsGet(leoDevice, log);
  } else {
    ASTERA_INFO("Leo CXL GET_EVENTS_RECORDS successful");
  }

  return status;
}

LeoErrorType doLeoEventsClear(LeoDeviceType *leoDevice, int log,
          uint32_t clrAll, uint32_t numRecs, uint16_t *handles)
{
  uint32_t readval = 1;
  uint8_t clrpayl[CXL_PMBOX_CLRALL_PAYL_SZ];
  int ii = 0;
  uint64_t cmd = 0;
  uint64_t payload_length = 8;

  if (log > CXL_PMBOX_FATAL_LOG) {
    ASTERA_ERROR("Leo incorrect log event requested, please see help [-h]");
    return LEO_FAILURE;
  }

  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d] waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  memset(clrpayl, 0, CXL_PMBOX_CLRALL_PAYL_SZ);
  clrpayl[0] = log;
  if (clrAll) {
    clrpayl[1] = 1;
    cmd |= (payload_length << 16);
  } else {
    clrpayl[2] = numRecs;
    memcpy(clrpayl+6, handles, numRecs*sizeof(uint16_t));
    payload_length = 6 + (numRecs*sizeof(uint16_t));
  }

  for (ii=0; ii<(6+(numRecs*sizeof(uint16_t))); ii+=4) {
    leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + ii,
          *(uint32_t *)(clrpayl+ii));
  }

  cmd = (payload_length << 16);
  cmd |= CXL_PMBOX_CLR_EVT_RECS;
  ASTERA_INFO("Issuing CLEAR_EVENT_RECORDS command to primary mailbox, cmd = 0x%llx", cmd);
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG, (uint32_t)cmd);
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG + 4, (uint32_t)(cmd >> 32));
  usleep(100);

  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, 0x1);
  usleep(100);
  ii = 0;
  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s] waitted 1 second for doorbell clear, quitting",
                   __FUNCTION__);
      return LEO_FAILURE;
    }
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  readval = 1;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_STS_REG + 4, &readval);
  readval &= 0xFFFF;
  if (readval != 0) {
    ASTERA_ERROR("CLEAR_EVENT_RECORDS command failed with status 0x%x",
                 readval);
    return LEO_FAILURE;
  }

  ASTERA_INFO("Leo CXL CLEAR_EVENT_RECORDS successful");

  return LEO_SUCCESS;
}

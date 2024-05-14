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
 * @file leo_poison_list.c
 * @brief reference demonstrates usage of mailbox commands inject, clear and get
 * poison. This is recommended for:
 *       - Initialize Leo device
 *       - Read, print FW version and status check
 *       - Leo inject poison
 *       - Leo clear poison
 *       - Leo get poison list
 */

#include "../include/DW_apb_ssi.h"
#include "../include/leo_api.h"
#include "../include/leo_common.h"
#include "../include/leo_error.h"
#include "../include/leo_globals.h"
#include "../include/leo_i2c.h"
#include "../include/leo_mbox_cmds.h"
#include "../include/leo_spi.h"
#include "include/aa.h"
#include "include/board.h"
#include "include/libi2c.h"

#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INJECT_POISON (0)
#define CLEAR_POISON (1)
#define GET_POISON_LIST (2)

LeoErrorType leoPoisonList(LeoDeviceType *leoDevice, int leoHandle, int action,
                           uint64_t dpa, uint64_t range);
LeoErrorType leoInjectPoison(LeoDeviceType *leoDevice, uint64_t dpa);
LeoErrorType leoClearPoison(LeoDeviceType *leoDevice, uint64_t dpa);
LeoErrorType leoGetPoisonList(LeoDeviceType *leoDevice, uint64_t dpa, uint64_t range);

const char *help_string[] = {DEFAULT_HELPSTRINGS, "inject -or- clear -or- get",
                             "dpa - device_physical_address",
                             "range - dpa range in units of 64 bytes"};

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  int actionType = -1;
  uint64_t dpa = 0;
  uint64_t range = 0;
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
  char *ptr;

  enum { DEFAULT_ENUMS, ENUM_ACTION_e, ENUM_DPA_e, ENUM_RANGE_e, ENUM_EOL_e };

  struct option long_options[] = {DEFAULT_OPTIONS,
                                  {"action", required_argument, 0, 0},
                                  {"dpa", required_argument, 0, 0},
                                  {"range", required_argument, 0, 0},
                                  {0, 0, 0, 0}};

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
        if (!strncmp(optarg, "inject", 6))
          actionType = INJECT_POISON;
        else if (!strncmp(optarg, "clear", 5))
          actionType = CLEAR_POISON;
        else if (!strncmp(optarg, "get", 3))
          actionType = GET_POISON_LIST;
        else
          actionType = -1;
        break;
      case ENUM_DPA_e:
        dpa = strtoul(optarg, &ptr, 16);
        break;
      case ENUM_RANGE_e:
        range = strtoul(optarg, &ptr, 16);
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

  if ((actionType == GET_POISON_LIST) && (!range)) {
    ASTERA_ERROR("provide valid range for GET_POISON_LIST");
    usage(argv[0], long_options, help_string);
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

      leoPoisonList(leoDevice, leoHandle, actionType, dpa, range);
      nextbdf = strtok(NULL, ",");
    }
    ASTERA_INFO("Exiting");
  } else {
    ASTERA_ERROR(
        "CXL mailbox commands can only be tested using BDF on native system");
  }
}

LeoErrorType leoPoisonList(LeoDeviceType *leoDevice, int leoHandle, int action,
                           uint64_t dpa, uint64_t range) {
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
    if (action == INJECT_POISON) {
      leoInjectPoison(leoDevice, dpa);
    } else if (action == CLEAR_POISON) {
      leoClearPoison(leoDevice, dpa);
    } else if (action == GET_POISON_LIST) {
      leoGetPoisonList(leoDevice, dpa, range);
    } else {
      ASTERA_ERROR("Leo unknown action requested, please see help [-h]");
    }
  } else {
    ASTERA_ERROR("Leo mailbox commands are not supported via I2C, see help -h");
  }

  if (leoDevice) {
    if (leoDevice->i2cDriver) {
      free(leoDevice->i2cDriver);
    }
    free(leoDevice);
  }

  return LEO_SUCCESS;
}

LeoErrorType leoInjectPoison(LeoDeviceType *leoDevice, uint64_t dpa) {
  uint32_t readval = 1;
  uint32_t val = 0;
  int ii = 0;

  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d]waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    readval = 0;
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  val = (0x8 << 16) | CXL_PMBOX_INJECT_POISON;
  ASTERA_INFO("Issuing INJECT_POISON command to primary mailbox");
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG, val);
  usleep(100);
  val = dpa;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG, val);
  val = dpa >> 32;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + 4, val);
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, 0x1);
  usleep(100);
  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d]waitted 1 second for doorbell clear, quitting",
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
    ASTERA_ERROR("[%s:%d] mailbox command failed with status 0x%x", __func__,
                 __LINE__, readval);
    return LEO_FAILURE;
  }
  ASTERA_INFO("Leo CXL INJECT_POISON successful");

  return LEO_SUCCESS;
}

LeoErrorType leoClearPoison(LeoDeviceType *leoDevice, uint64_t dpa) {
  uint32_t readval = 1;
  uint32_t val = 0;
  int ii = 0;

  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d]waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    readval = 0;
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  val = (0x48 << 16) | CXL_PMBOX_CLEAR_POISON;
  ASTERA_INFO("Issuing CLEAR_POISON command to primary mailbox");
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG, val);
  usleep(100);
  val = dpa;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG, val);
  val = dpa >> 32;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + 4, val);

  for (ii = 0; ii < 64; ii += 4) {
    val = 0;
    leoWriteWordData(leoDevice->i2cDriver,
                     CXL_BAR2_PMBOX_PAYL_REG + 8 + (ii * 4), val);
  }

  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, 0x1);
  usleep(100);
  ii = 0;
  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d]waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    readval = 0;
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  readval = 1;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_STS_REG + 4, &readval);
  readval &= 0xFFFF;
  if (readval != 0) {
    ASTERA_ERROR("[%s:%d] mailbox command failed with status 0x%x", __func__,
                 __LINE__, readval);
    return LEO_FAILURE;
  }
  ASTERA_INFO("Leo CXL CLEAR_POISON successful");

  return LEO_SUCCESS;
}

static void leoPrintPoisonList(cxl_poison_list_t *pl) {
  int idx = 0;
  ASTERA_INFO("\tMode Media Error Records         : %d", pl->more_err_recs);
  ASTERA_INFO("\tPoison List Overflow             : %d", pl->overflow);
  ASTERA_INFO("\tMedia Scan In Progress           : %d", pl->scan_in_progress);
  ASTERA_INFO("\tPoison List Overflow Timestamp   : %llu",
              *(uint64_t *)(pl->overflow_timestamp));
  ASTERA_INFO("\tPoison List Media Error Count    : %u",
              *(uint16_t *)(pl->err_cnt));

  for (idx = 0; idx < *(uint16_t *)(pl->err_cnt); idx++) {
    ASTERA_INFO("\tMedia Error Record[%02d]", idx);
    ASTERA_INFO("\t\tDevice Physical Address    : 0x%llx",
                pl->err_rec[idx].dpa.qw);
    ASTERA_INFO("\t\tRange of Adjescent DPAs    : 0x%x", pl->err_rec[idx].len);
  }
}

LeoErrorType leoGetPoisonList(LeoDeviceType *leoDevice, uint64_t dpa, uint64_t range) {
  uint32_t readval = 1;
  uint32_t outl = 0;
  uint32_t val = 0;
  uint8_t payload[LEO_CXL_MBOX_MAX_PAYLOAD_SIZE];
  int ii = 0;
  cxl_poison_list_t *pl = (cxl_poison_list_t *)payload;
  LeoErrorType status = LEO_SUCCESS;

  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d]waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    readval = 0;
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  val = (0x10 << 16) | CXL_PMBOX_GET_POISON_LIST;
  ASTERA_INFO("Issuing GET_POISON_LIST to Primary Mailbox");
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG, val);
  usleep(100);
  val = dpa;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG, val);
  val = dpa >> 32;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + 4, val);
  val = range;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + 8, val);
  val = range >> 32;
  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + 12, val);

  leoWriteWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, 0x1);
  usleep(100);

  ii = 0;
  do {
    if (ii > 10) {
      ASTERA_ERROR("[%s:%d]waitted 1 second for doorbell clear, quitting",
                   __func__, __LINE__);
      return LEO_FAILURE;
    }
    readval = 0;
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CTL_REG, &readval);
    ii++;
    usleep(100);
  } while (readval != 0);

  readval = 1;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_STS_REG + 4, &readval);
  readval &= 0xFFFF;
  if (readval != 0) {
    ASTERA_ERROR("[%s:%d] mailbox command failed with status 0x%x", __func__,
                 __LINE__, readval);
    return LEO_FAILURE;
  }
  readval = 0;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG, &readval);
  outl = readval >> 16;
  leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_CMD_REG + 4, &readval);
  outl |= ((readval & 0x1F) << 16);
  ASTERA_INFO("CXL GET_POISON_LIST output payload length = %d bytes", outl);

  memset(payload, 0, LEO_CXL_MBOX_MAX_PAYLOAD_SIZE);

  for (ii = 0; ii < outl; ii += 4) {
    leoReadWordData(leoDevice->i2cDriver, CXL_BAR2_PMBOX_PAYL_REG + ii,
                    &readval);
    *(uint32_t *)(payload + ii) = readval;
  }

  leoPrintPoisonList(pl);

  if (pl->more_err_recs) {
    status = leoGetPoisonList(leoDevice, dpa, range);
  }

  ASTERA_INFO("Leo CXL GET_POISON_LIST successful");

  return status;
}

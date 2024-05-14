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
 * @file leo_read_fruprom_example.c
 * @brief api test demonstrates reading Leo FRUPROM
 * This is recommended for:
 *       - Print board information such as serial number and board type
 * For more details on the args supported & usage
 * ./leo_read_fruprom_example -help
 *
 */

#include <ctype.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/astera_log.h"
#include "./aardvark/aardvark.h"
#include "./aardvark/aardvark_setup.h"
#include "./include/aa.h"
#include "./include/libi2c.h"
#include "include/board.h"
#include "../include/leo_api.h"

#define PROM_MARKER_START 0
#define PROM_MARKER_LENGTH 2
#define PROM_VERSION_START 2
#define PROM_VERSION_LENGTH 2
#define PROM_BOARD_START 4
#define PROM_BOARD_LENGTH 4
#define PROM_DATE_START 8
#define PROM_DATE_LENGTH 8
#define PROM_COMPANY_START 16
#define PROM_COMPANY_LENGTH 16
#define PROM_SERIAL_START 32
#define PROM_SERIAL_LENGTH 16
#define PROM_PCA_START 48
#define PROM_PCA_LENGTH 16
#define PROM_SID_START 64
#define PROM_SID_LENGTH 16
#define PROM_HWBITS_START 80
#define PROM_HWBITS_LENGTH 8
#define PROM_DEVIATION_START 88
#define PROM_DEVIATION_NUM 16
#define PROM_CHECKSUM_START 142

enum board_type {
  AL_BOARD_LEO_SVB_e = 0, // LEO SVB
  AL_BOARD_LEO_LBB_e = 1, // Loopback test board
  AL_BOARD_AURORA1_e = 2, // Aurora1000 (1-Leo)
  AL_BOARD_AURORA2_e = 3, // Aurora2000 (2-Leo)
  AL_BOARD_LAST_e
};

char board_name_str[][16] = {
    "LEO-SVB", // 0: AL_BOARD_LEO_SVB_e
    "LEO-LBB", // 1: AL_BOARD_LEO_LBB_e
    "AURORA1", // 2: AL_BOARD_AURORA1_e
    "AURORA2", // 3: AL_BOARD_AURORA2_e
    "INVALID", // LAST
};

int main(int argc, char *argv[]) {
  int rc;
  int i2cBus = 1;
  Aardvark handle;
  int addr;
  int bytesToRead = 256;
  uint8_t buf[bytesToRead];
  int ii;
  int checksum;
  int checksum_prom;
  int board_type;
  uint8_t tmp08;
  uint32_t tmp32;
  uint64_t tmp64;
  int option_index;
  int option;
  uint8_t line[32];
  int frupromAddress = LEO_DEV_FRU;
  conn_t conn;
  char *serialnum = NULL;
  LeoDeviceType 	*leoDevice;
  LeoI2CDriverType 	*i2cDriver;

  enum { ENUM_HELP_e, ENUM_SERIALNUM_e, ENUM_FRU_e, ENUM_EOL_e };

  struct option long_options[] = {{"help", no_argument, 0, 0},
                                  {"serialnum", required_argument, 0, 0},
                                  {"dev", required_argument, 0, 0},
                                  {0, 0, 0, 0}};

  const char *help_string[] = {
      "This page", // help
      "Choose target connected to the I2C Serial Number(serialnum format "
      "xxxx-xxxxxx)",
      "Set FRU PROM device address (default 0x56, old cards use 0x57)", // FRU
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
      case ENUM_HELP_e:
        usage(argv[0], long_options, help_string);
        break;
      case ENUM_SERIALNUM_e:
        serialnum = optarg;
        ASTERA_INFO("Using target connected to I2C Serial Number %s",
                    serialnum);
        break;
      case ENUM_FRU_e:
        frupromAddress = strtoul(optarg, NULL, 16);
        ASTERA_INFO("Set FRU PROM target to %x", frupromAddress);
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

  if (serialnum != NULL) {
    if (strlen(serialnum) != SERIAL_VALID_SIZE) {
      usage(argv[0], long_options, help_string);
    }
    strcpy(conn.serialnum, serialnum);
  }

  if (serialnum == NULL) {
    handle = asteraI2COpenConnection(i2cBus, frupromAddress);
  } else {
    handle = asteraI2COpenConnectionExt(i2cBus, frupromAddress, conn);
  }

  i2cDriver = (LeoI2CDriverType *)calloc(1,sizeof(LeoI2CDriverType));
  if(i2cDriver == NULL) {
    asteraI2CCloseConnection(handle);
    return 1;
  }
  i2cDriver->handle = handle;
  i2cDriver->slaveAddr = frupromAddress;
  i2cDriver->i2cFormat = LEO_I2C_FORMAT_SMBUS;
  i2cDriver->pciefile = NULL;

  leoDevice = (LeoDeviceType *)calloc(1,sizeof(LeoDeviceType));
  if (leoDevice == NULL)
  {
    asteraI2CCloseConnection(handle);
    free(i2cDriver);
    return 1;
  }
  leoDevice->i2cDriver = i2cDriver;
  leoDevice->i2cBus = i2cBus;
  rc = leoInitDevice(leoDevice);
  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("Init device failed");
    asteraI2CCloseConnection(handle);
	free(i2cDriver);
    free(leoDevice);
    return rc;
  }

  for (addr = 0; addr < bytesToRead; addr++) {
    rc = leoReadByteData(leoDevice->i2cDriver, addr, &tmp08);
    if (0 != rc) {
      usage(argv[0], long_options, help_string);
    }
    CHECK_SUCCESS(rc)
    buf[addr] = tmp08;
  }

  ASTERA_INFO("FRUPROM Read:");
  ASTERA_INFO("MARKER:        %02x%02x", buf[PROM_MARKER_START],
              buf[PROM_MARKER_START + 1]);
  ASTERA_INFO("VERSION:       %02x.%02x", buf[PROM_VERSION_START],
              buf[PROM_VERSION_START + 1]);
  board_type = (buf[PROM_BOARD_START] << 24) |
               (buf[PROM_BOARD_START + 1] << 16) |
               (buf[PROM_BOARD_START + 2] << 8) | (buf[PROM_BOARD_START + 3]);
  if (board_type > AL_BOARD_LAST_e)
    board_type = AL_BOARD_LAST_e;
  ASTERA_INFO("BOARD:         %d (%s)", board_type, board_name_str[board_type]);
  memset(line, 0, 32);

  for (ii = 0; ii < PROM_COMPANY_LENGTH; ii++) {
    if (0 == buf[PROM_COMPANY_START + ii]) {
      break;
    } else if (isprint(buf[PROM_COMPANY_START + ii])) {
      line[ii] = buf[PROM_COMPANY_START + ii];
    } else {
      line[ii] = '.';
    }
  }
  ASTERA_INFO("COMPANY:       %s", line);

  for (ii = 0; ii < PROM_SERIAL_LENGTH; ii++) {
    if (0 == buf[PROM_SERIAL_START + ii]) {
      break;
    } else if (isprint(buf[PROM_SERIAL_START + ii])) {
      line[ii] = buf[PROM_SERIAL_START + ii];
    } else {
      line[ii] = '.';
    }
  }
  ASTERA_INFO("SERIAL_NUMBER: %s", line);
  memset(line, 0, 32);

  for (ii = 0; ii < PROM_PCA_LENGTH; ii++) {
    if (0 == buf[PROM_PCA_START + ii]) {
      break;
    } else if (isprint(buf[PROM_PCA_START + ii])) {
      line[ii] = buf[PROM_PCA_START + ii];
    } else {
      line[ii] = '.';
    }
  }
  ASTERA_INFO("PCA_NUMBER:    %s", line);
  memset(line, 0, 32);

  for (ii = 0; ii < PROM_SID_LENGTH; ii++) {
    if (0 == buf[PROM_SID_START + ii]) {
      break;
    } else if (isprint(buf[PROM_SID_START + ii])) {
      line[ii] = buf[PROM_SID_START + ii];
    } else {
      line[ii] = '.';
    }
  }
  ASTERA_INFO("SYSTEM_ID:     %s", line);
  memset(line, 0, 32);

  for (ii = 0; ii < PROM_DATE_LENGTH; ii++) {
    if (0 == buf[PROM_DATE_START + ii]) {
      break;
    } else if (isprint(buf[PROM_DATE_START + ii])) {
      line[ii] = buf[PROM_DATE_START + ii];
    } else {
      line[ii] = '.';
    }
  }
  ASTERA_INFO("DATE:          %s", line);

  asteraI2CCloseConnection(handle);
  free(i2cDriver);
  free(leoDevice);
  return 0;
}

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
 * @file board.c
 * @brief board related utilities and settings for examples.
 */
#include "../include/board.h"
#include "../include/leo_connection.h"
#include "misc.h"

int IS_SVB = 0;
int IS_A1 = 0;
int IS_A2 = 0;
int IS_LEO = 0;

LeoErrorType svbP1SetI2cSwitch(int handle, int mode) {
  uint8_t cmd;
  uint8_t values[1];
  int rc;

  if (mode == 1) {
    cmd = 0x1;
  } else if (mode == 2) {
    cmd = 0x2;
  } else if (mode == 3) {
    cmd = 0x3;
  } else {
    return LEO_INVALID_ARGUMENT;
  }

  rc = asteraI2CWriteBlockData(handle, cmd, 1, &cmd);
  return rc;
}

LeoErrorType aurora2000SetI2cSwitch(int handle, int mode) {
  uint8_t cmd;
  uint8_t values[1];
  LeoErrorType rc;

  switch (mode) {
  case 1:
    cmd = 0x1;
    break;
  case 2:
    cmd = 0x2;
    break;
  case 3:
    cmd = 0x3;
    break;
  case 5:
    cmd = 0x5;
    break;
  default:
    return LEO_INVALID_ARGUMENT;
    break;
  }

  rc = asteraI2CWriteBlockData(handle, cmd, 1, &cmd);
  return rc;
}

LeoErrorType leoSetMuxAddress(int i2cBus, DefaultArgsType *defaultArgs,
                              conn_t conn) {
  #ifdef CHIP_A0
  return LEO_SUCCESS;
  #endif

  LeoErrorType rc;
  ASTERA_DEBUG("default Mux Address is %x", defaultArgs->switchAddress);
  ASTERA_DEBUG("default serialnum is %s", defaultArgs->serialnum);
  ASTERA_DEBUG("default leoAddress is %x", defaultArgs->leoAddress);
  ASTERA_DEBUG("default bdf is %s", defaultArgs->bdf);

  if (defaultArgs->serialnum == NULL) {
    defaultArgs->switchHandle =
        asteraI2COpenConnection(i2cBus, defaultArgs->switchAddress);
    ASTERA_INFO("asteraI2COpen on %x switchHandle %d",
                defaultArgs->switchAddress, defaultArgs->switchHandle);
  } else {
    defaultArgs->switchHandle =
        asteraI2COpenConnectionExt(i2cBus, defaultArgs->switchAddress, conn);
    ASTERA_INFO("asteraI2COpenConExt switchHandle %d",
                defaultArgs->switchAddress, defaultArgs->switchHandle);
  }

  if (IS_A2) {
    rc = aurora2000SetI2cSwitch(defaultArgs->switchHandle,
                                defaultArgs->switchMode);
  } else {
    rc = svbP1SetI2cSwitch(defaultArgs->switchHandle, defaultArgs->switchMode);
  }

  asteraI2CCloseConnection(defaultArgs->switchHandle);

  if (rc == LEO_SUCCESS) {
    return rc;
  } else {
    if (defaultArgs->switchAddress == LEO_DEV_MUX) {
      defaultArgs->switchAddress = 0x70;
    } else {
      defaultArgs->switchAddress = LEO_DEV_MUX;
    }
    ASTERA_INFO("Invalid switch address, trying %x",
                defaultArgs->switchAddress);
  }

  if (defaultArgs->serialnum == NULL) {
    defaultArgs->switchHandle =
        asteraI2COpenConnection(i2cBus, defaultArgs->switchAddress);
    ASTERA_INFO("asteraI2COpen on %x switchHandle %d",
                defaultArgs->switchAddress, defaultArgs->switchHandle);
  } else {
    defaultArgs->switchHandle =
        asteraI2COpenConnectionExt(i2cBus, defaultArgs->switchAddress, conn);
    ASTERA_INFO("asteraI2COpenConExt switchHandle %d",
                defaultArgs->switchAddress, defaultArgs->switchHandle);
  }

  if (IS_A2) {
    rc = aurora2000SetI2cSwitch(defaultArgs->switchHandle,
                                defaultArgs->switchMode);
  } else {
    rc = svbP1SetI2cSwitch(defaultArgs->switchHandle, defaultArgs->switchMode);
  }
  asteraI2CCloseConnection(defaultArgs->switchHandle);

  if (rc != LEO_SUCCESS) {
    ASTERA_ERROR("Failed to set Mux mode");
    return rc;
  }

  if (defaultArgs->switchHandle < 0) {
    return LEO_I2C_OPEN_FAILURE;
  } else {
    ASTERA_INFO("switchHandle %d", defaultArgs->switchHandle);
    return LEO_SUCCESS;
  }
}

LeoErrorType decodeDimmManufacturer(uint32_t dimmManufacturerIdCode, char* manufacturerString) {
  uint8_t continuation = dimmManufacturerIdCode >> 8;
  uint8_t id = dimmManufacturerIdCode & 0xff;

  if (0x80 == continuation) {
    switch(id) {
      case 0x2c:
        strcpy(manufacturerString, "Micron Technology");
        break;
      case 0xad:
        strcpy(manufacturerString, "SK Hynix");
        break;
      case 0xce:
        strcpy(manufacturerString, "Samsung");
        break;
      default:
        strcpy(manufacturerString, "Undecoded");
        return LEO_FUNCTION_UNSUCCESSFUL;      
    }
  }
  return LEO_SUCCESS;
}

void usage(char *name, struct option *long_options, const char **help_string) {
  int i;
  ASTERA_INFO("Usage: %s <args>", name);
  for (i = 0;; i++) {
    if (0 != long_options[i].name) {
      switch (long_options[i].has_arg) {
      case required_argument:
        ASTERA_INFO("    [-%-12s  arg]  - %s", long_options[i].name,
                    help_string[i]);
        break;
      default:
        ASTERA_INFO("    [-%-12s     ]  - %s", long_options[i].name,
                    help_string[i]);
      }
    } else {
      break;
    }
  }
  exit(0);
} // void usage()

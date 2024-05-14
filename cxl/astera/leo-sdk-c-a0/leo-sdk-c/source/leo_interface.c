/*
 * Copyright 2020 Astera Labs, Inc. or its affiliates. All Rights Reserved.
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
 * @file leo_i2c.c
 * @brief Implementation of public functions for the SDK I2C interface.
 */

#include <stdint.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/leo_api_internal.h"
#include "../include/leo_common.h"
#include "../include/leo_i2c.h"
#include "../include/leo_mailbox.h"
#include "../include/leo_pcie.h"
#include "../include/libi2c.h"

#define CHECK_LOCK_SUCCESS(rc) {\
    if (rc != LEO_SUCCESS) {\
        ASTERA_ERROR("Could not do I2C lock: %d", rc);\
        return rc;\
    }\
}

#define CHECK_UNLOCK_SUCCESS(rc) {\
    if (rc != LEO_SUCCESS) {\
        ASTERA_ERROR("Could not do I2C unlock: %d", rc);\
        return rc;\
    }\
}


/*
 *  Lock I2C driver
 * 
 */
LeoErrorType leoLock(
        LeoI2CDriverType* i2cDriver)
{
    LeoErrorType rc;
    rc = 0;
    if (i2cDriver->lock >= 1)
    {
        i2cDriver->lock++;
    }
    else
    {
        rc = asteraI2CBlock(i2cDriver->handle);
        i2cDriver->lock = 1;
    }

    return rc;
}


/*
 *  Unlock I2C driver
 *   
 */
LeoErrorType leoUnlock(
        LeoI2CDriverType* i2cDriver)
{
    LeoErrorType rc;
    rc = 0;
    if (i2cDriver->lock > 1)
    {
        i2cDriver->lock--;
    }
    else
    {
        rc = asteraI2CUnblock(i2cDriver->handle);
        i2cDriver->lock = 0;
    }

    return rc;
}

/*
 * Write multiple data bytes to Leo over I2C
 */
LeoErrorType leoWriteBlockData(LeoI2CDriverType *i2cDriver, uint32_t address,
                               uint8_t lengthBytes, uint8_t *values) {
  int handle = i2cDriver->handle;
  int rc;
  /* ASTERA_INFO("**INFO : leoWriteBlockData\n"); */

  /**
   * @TODO: After removing padding functionality from libi2c, enable it here.
   */
  size_t i;
  size_t addrLen = 3;
  size_t wrDataLen = lengthBytes + addrLen + 3;
  uint8_t cmdCode = 0;
  uint8_t dataOut[wrDataLen];
  dataOut[0] = 0x4f;  //was 0xf
  dataOut[1] = 0x8;
  /* dataOut[1] = 1 + addrLen + lengthBytes; */
  dataOut[2] = 0x4e;
  /* dataOut[2] = ((addrLen + lengthBytes) << 1) | 0x40; */
  dataOut[3] = (address >> 16) & 0xff;
  dataOut[4] = (address >> 8) & 0xff;
  dataOut[5] = (address)&0xff;
  for (i = 0; i < lengthBytes; i++) {
    dataOut[i + 6] = values[lengthBytes - i - 1];
  }
  rc = leoLock(i2cDriver);
  CHECK_LOCK_SUCCESS(rc);
  rc = asteraI2CWriteBlockData(handle, cmdCode, wrDataLen, dataOut);
  rc = leoUnlock(i2cDriver);
  CHECK_UNLOCK_SUCCESS(rc);

  return rc;
}

LeoErrorType leoWriteWordData(LeoI2CDriverType *i2cDriver, uint32_t address,
                              uint32_t value) {
  uint8_t buffer[4];
  int rcl;
  int rc;
  buffer[0] = (value >> 24) & 0xff;
  buffer[1] = (value >> 16) & 0xff;
  buffer[2] = (value >> 8) & 0xff;
  buffer[3] = (value)&0xff;

#ifdef LEO_CSDK_DEBUG
  printf("\nWrite address = 0x%x data = 0x%x\n", address, value);
#endif

  if (i2cDriver->pciefile != NULL) { // use PCIe
    if (isAddressMailbox(address)) {
      uint32_t dataIn[16];
      uint32_t dataOut[16] = {0};
      dataOut[0] = value;
      execOperation(i2cDriver, address, FW_API_MMB_CMD_OPCODE_MMB_CSR_WRITE,
                    dataOut, 1, dataIn, 1);
    } else {
      // Delay to sync PCIe writes with SPI
      usleep(100);
      return leoWriteWordPcie(i2cDriver->pciefile, address, value);
    }
  } else { // use I2C
    rc = leoWriteBlockData(i2cDriver, address, 4, buffer);
    return rc;
  }
}

/*
 * Write a data byte to Leo over I2C
 */
LeoErrorType leoWriteByteData(LeoI2CDriverType *i2cDriver, uint32_t address,
                              uint8_t *value) {
  return leoWriteBlockData(i2cDriver, address, 1, value);
}

/*
 * Read multiple data bytes from Leo over I2C
 */
LeoErrorType leoReadBlockData(LeoI2CDriverType *i2cDriver, uint32_t address,
                              uint8_t numBytes, uint8_t *values) {
  int handle = i2cDriver->handle;
  int rc;
  int rcl;
  rc = leoLock(i2cDriver);
  CHECK_LOCK_SUCCESS(rc);
  if (i2cDriver->i2cFormat == LEO_I2C_FORMAT_ASTERA) {
    rc = asteraI2CReadBlockData(handle, address, numBytes, values);
  }
  else if (i2cDriver->i2cFormat == LEO_I2C_FORMAT_SMBUS) {
    if (numBytes == 1) {
      rc = i2c_read_byte(i2cDriver->handle, i2cDriver->slaveAddr,
                        address, values);
    }
    else {
      ASTERA_ERROR("ERROR: Unsupported numBytes: %d\n", numBytes);
      return -1;
    }
  }
  else {
    ASTERA_ERROR("ERROR: Unknown I2C format (%d)\n", i2cDriver->i2cFormat);
    return -1;
  }

  rcl = leoUnlock(i2cDriver);
  CHECK_UNLOCK_SUCCESS(rcl);
  return rc;
}

/*
 * Read a data byte from Leo over I2C
 */
LeoErrorType leoReadByteData(LeoI2CDriverType *i2cDriver, uint32_t address,
                             uint8_t *values) {
  return leoReadBlockData(i2cDriver, address, 1, values);
}

LeoErrorType leoReadWordData(LeoI2CDriverType *i2cDriver, uint32_t address,
                             uint32_t *value) {
  int rc;
  int rcl;

  if (i2cDriver->pciefile != NULL) { // use PCIe
    if (isAddressMailbox(address)) {
      uint32_t read_word;
      execOperation(i2cDriver, address, FW_API_MMB_CMD_OPCODE_MMB_CSR_READ,
                    NULL, 0, value, 1);
    } else {
      usleep(100);
      rc = leoReadWordPcie(i2cDriver->pciefile, address, value);
    }
  } else { // use I2C
    rc = leoReadBlockData(i2cDriver, address, 4, (uint8_t *)value);
    return rc;
  }
#ifdef LEO_CSDK_DEBUG
  printf("\nRead address = 0x%x data = 0x%x\n", address, *value);
#endif

  return rc;
}




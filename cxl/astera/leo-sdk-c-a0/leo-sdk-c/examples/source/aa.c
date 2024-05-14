
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

#include "../include/aa.h"
#include <signal.h>
#include <stdint.h>
static uint8_t aaSlaveAddr;
static Aardvark aaHandle;
#define MAX_READ_SIZE 32

/* @brief Write a byte to the Aardvark I2C bus
 * @param handle Aardvark handle
 * @param device I2C device address
 * @param addr I2C register address
 * @param data Data to write
 * @return 0 on success, -1 on failure
 */
int asteraI2CWriteBlockData(Aardvark handle, uint32_t cmdCode, uint8_t bufLen,
                            uint8_t *buf) {
  int rc;
  size_t i;
  uint16_t count;

  rc = aa_i2c_write_ext(handle, aaSlaveAddr, AA_I2C_NO_FLAGS, bufLen, buf,
                        &count);
  if (0 != rc) {
    printf("error (%d): %s\n", rc, aa_status_string(rc));
  } else if (bufLen != count) {
    rc = 1;
    printf("WARNING: Write %d, expected %d\n", count, bufLen);
  }
  return rc;
}

int asteraI2CReadBlockData(int handle, uint32_t cmdCode, uint8_t bufLen,
                           uint8_t *buf) {
  int rc;
  rc = i2c_read_block_32(handle, aaSlaveAddr, cmdCode, bufLen, buf);
  return rc;
}

int asteraI2COpenConnection(int bus, int slaveAddress) {
  Aardvark handle;
  ulong port = 0;
  int i2c_bitrate = 400;  // in KHz
  int spi_bitrate = 1000; // in KHz
  int rc;

  setSlaveAddress(slaveAddress);
  setHandle(handle);
  rc = aardvark_setup((int)port, i2c_bitrate, spi_bitrate, &handle);
  if (0 != rc) {
    printf("ERROR: aardvark_setup() failed\n");
    return (-1);
  }
  enableSignalHandler(1);
  return handle;
} // asteraI2COpenConnection()

int asteraI2COpenConnectionExt(int bus, int slaveAddress, conn_t conn) {
  Aardvark handle;
  int i2c_bitrate = 400;  // in KHz
  int spi_bitrate = 1000; // in KHz
  int rc;

  setSlaveAddress(slaveAddress);
  setHandle(handle);
  rc = aardvark_setup_ext(i2c_bitrate, spi_bitrate, &handle, conn);
  if (0 != rc) {
    printf("ERROR: aardvark_setup() failed\n");
    return (-1);
  }
  return handle;
} // asteraI2COpenConnection()

void asteraI2CCloseConnection(Aardvark aardvark) {
  aardvark_close(aardvark);
  enableSignalHandler(0);
  return;
}

void setSlaveAddress(uint8_t address) { aaSlaveAddr = address; }
void setHandle(Aardvark handle) { aaHandle = handle; }

void signalHandler(int signum) {
  aa_target_power(aaHandle, AA_TARGET_POWER_NONE);
  raise(signum);
  return;
}

void enableSignalHandler(int enable) {
  struct sigaction sa;
  sa.sa_handler = (1 == enable) ? signalHandler : SIG_DFL;
  sa.sa_flags = SA_RESETHAND;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGKILL, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);
}

int setGpioSpiMux(Aardvark aardvark, int outputVal) {
  //TODO: ask Xueming
  #ifdef CHIP_A0
  return LEO_SUCCESS;
  #endif

  int dev_addr = LEO_DEV_GPIO_0;
  int index = 14;
  int pin = index % 8;
  int rc = 0;
  int config;
  int output;
  uint8_t mask;
  uint8_t r_config;
  uint8_t r_output;
  uint8_t addr_output = 0x03;
  uint8_t addr_config = 0x07;

  rc += i2c_read_byte(aardvark, aaSlaveAddr, addr_config, &r_config);
  config = (r_config >> pin) & 0x1;

  rc += i2c_read_byte(aardvark, aaSlaveAddr, addr_output, &r_output);
  output = (r_output >> pin) & 0x1;

  ASTERA_INFO("id %d pin %d output %d config %d", index, pin, output, config);

  if (output == outputVal) {
    ASTERA_INFO("OUTPUT value already set to requested value");
  } else {
    mask = 0xff ^ (1 << pin);
    r_output = r_output & mask;
    r_output = r_output | (outputVal << pin);
    printf("pin %d r_output %d\n", pin, r_output);
    rc += i2c_write_byte(aardvark, dev_addr, addr_output, r_output);
  }

  if (0 != config) {
    mask = 0xff ^ (1 << pin);
    r_config = r_config & mask;
    rc += i2c_write_byte(aardvark, dev_addr, addr_config, r_config);
  }
  return rc;
}

int asteraI2CBlock(int handle) {
  return 0; // Equivalent to SUCCESS
}

int asteraI2CUnblock(int handle) {
  return 0; // Equivalent to SUCCESS
}

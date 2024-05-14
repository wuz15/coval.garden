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
 * @file leo_i2c.h
 * @brief Definition of I2C/SMBus types for the SDK.
 */

#ifndef ASTERA_LEO_SDK_I2C_H_
#define ASTERA_LEO_SDK_I2C_H_

#include "../include/misc.h"
#include "astera_log.h"
#include "leo_api_types.h"
#include "leo_connection.h"
#include "leo_error.h"
#include "leo_globals.h"
#include "leo_logger.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHECK_SUCCESS(rc)                                                      \
  {                                                                            \
    if (rc != LEO_SUCCESS) {                                                   \
      ASTERA_ERROR("Unexpected return code: %d", rc);                          \
      return rc;                                                               \
    }                                                                          \
  }

#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr)-offsetof(type, member)))

#define PRINT_ERROR                                                            \
  do {                                                                         \
    fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__,         \
            __FILE__, errno, strerror(errno));                                 \
    exit(1);                                                                   \
  } while (0)

/**
 * @brief Low-level I2C method to open a connection. THIS FUNCTION MUST BE
 * IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * @param[in]  i2cBus           I2C bus number
 * @param[in]  slaveAddress     I2C (7-bit) address of retimer
 * @return     int - handle
 *
 */
int asteraI2COpenConnection(int i2cBus, int slaveAddress);

/**
 * @brief Low-level I2C method to open a connection. THIS FUNCTION MUST BE
 * IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * @param[in]  i2cBus           I2C bus number
 * @param[in]  slaveAddress     I2C (7-bit) address of retimer
 * @param[in]  connection       structure that has more connectivity details
 * @return     int - handle
 *
 */
int asteraI2COpenConnectionExt(int i2cBus, int slaveAddress, conn_t conn);

/**
 * @brief Low-level I2C method to close a connection. THIS FUNCTION MUST BE
 * IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * @param[in]  handle           I2C handle
 * @return     int - LEO_SUCCESS or LEO_ERROR
 *
 */
void asteraI2CCloseConnection(int handle);

/**
 * @brief Low-level I2C write method. THIS FUNCTION MUST BE IMPLEMENTED IN
 * THE USER'S APPLICATION. For example, if using linux/i2c.h, then the
 * implementation would be:
 *   int asteraI2CWriteBlockData(
 *       int handle,
 *       uint8_t cmdCode,
 *       uint8_t bufLen,
 *       uint8_t *buf)
 *   {
 *       return i2c_smbus_write_i2c_block_data(handle, cmdCode, bufLen, buf);
 *   }
 *
 * @param[in]  handle     Handle to I2C driver
 * @param[in]  cmdCode    8-bit command code
 * @param[in]  bufLen     Data buffer length, in bytes
 * @param[in]  *buf       Pointer to data buffer (byte array)
 * @return     int - Error code
 *
 */
int asteraI2CWriteBlockData(int handle, uint32_t cmdCode, uint8_t bufLen,
                            uint8_t *buf);

/**
 * @brief Low-level I2C read method. THIS FUNCTION MUST BE IMPLEMENTED IN
 * THE USER'S APPLICATION. For example, if using linux/i2c.h, then the
 * implementation would be:
 *   int asteraI2CReadBlockData(
 *       int handle,
 *       uint8_t cmdCode,
 *       uint8_t bufLen,
 *       uint8_t *buf)
 *   {
 *       return i2c_smbus_read_i2c_block_data(handle, cmdCode, bufLen, buf);
 *   }
 *
 * @param[in]  handle     Handle to I2C driver
 * @param[in]  cmdCode    8-bit command code
 * @param[in]  bufLen     Number of bytes to read
 * @param[out] *buf       Pointer to data buffer into which the read data will
 *                        be placed
 * @return     int - Error code
 *
 */
int asteraI2CReadBlockData(int handle, uint32_t cmdCode, uint8_t bufLen,
                           uint8_t *buf);

/**
 * @brief Low-level I2C method to implement a lock around I2C transactions such
 * that a set of transactions can be atomic. THIS FUNCTION MUST BE IMPLEMENTED
 * IN THE USER'S APPLICATION.
 *
 * @param[in] handle Handle to I2C driver
 *
 * @return int - Error code
 */
int asteraI2CBlock(int handle);

/**
 * @brief Low-level I2C method to unlock a previous lock around I2C
 * transactions such that a set of transactions can be atomic. THIS
 * FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * @param[in] handle Handle to I2C driver
 *
 * @return int - Error code
 */
int asteraI2CUnblock(int handle);

/**
 * @brief Set Slave address to user-specified value: new7bitSmbusAddr
 *
 * @param[in] handle            Handle to I2C driver
 * @param[in] new7bitSmbusAddr  DesiredI2C (7-bit) address of retimer
 * @return     LeoErrorType -  error code
 */
LeoErrorType leoRunArp(int handle, uint8_t new7bitSmbusAddr);

/**
 * @brief Write multiple data bytes to Leo over I2C  This function retuns a
 * negative error code on error, and zero on success.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      17-bit address to write
 * @param[in]  lengthBytes  Number of bytes to write (maximum 16 bytes)
 * @param[in]  values       Pointer to array of data (bytes) you wish to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteBlockData(LeoI2CDriverType *i2cDriver, uint32_t address,
                               uint8_t lengthBytes, uint8_t *values);

/**
 * @brief Write a data byte at specified address to Leo over I2C. Returns a
 * negative error code if unsuccessful, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    17-bit address to write
 * @param[in]  value      Pointer to single element array of data you wish
 *                        to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteByteData(LeoI2CDriverType *i2cDriver, uint32_t address,
                              uint8_t *value);

/**
 * @brief Read multiple bytes of data at specified address from Leo over I2C.
 * If unsuccessful, return a negative error code, else zero.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      17-bit address from which to read
 * @param[in]  lengthBytes  Number of bytes to read (maximum 16 bytes)
 * @param[out] values       Pointer to array of read data (bytes)
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadBlockData(LeoI2CDriverType *i2cDriver, uint32_t address,
                              uint8_t lengthBytes, uint8_t *values);

/**
 * @brief Read a data byte from Leo over I2C
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    17-bit address from which to read
 * @param[out] value      Pointer to single element array of read data (byte)
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadByteData(LeoI2CDriverType *i2cDriver, uint32_t address,
                             uint8_t *value);

/**
 * @brief Read multiple (up to eight) data bytes from micro SRAM over I2C on A0.
 * Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset   Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to read
 * @param[in]  lengthBytes  Number of bytes to read
 * @param[out] values     Pointer to single element array of read data
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadBlockDataMainMicroIndirectA0(LeoI2CDriverType *i2cDriver,
                                                 uint32_t microIndStructOffset,
                                                 uint32_t address,
                                                 uint8_t lengthBytes,
                                                 uint8_t *values);

/**
 * @brief Read multiple (up to eight) data bytes from micro SRAM over I2C.
 * Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to read
 * @param[in]  lengthBytes  Number of bytes to read
 * @param[out] values     Pointer to single element array of read data
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadBlockDataMainMicroIndirectMPW(LeoI2CDriverType *i2cDriver,
                                                  uint32_t microIndStructOffset,
                                                  uint32_t address,
                                                  uint8_t lengthBytes,
                                                  uint8_t *values);

/**
 * @brief Write multiple data bytes to specified address from micro SRAM over
 * I2C for A0. Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to write
 * @param[in]  lengthBytes  Number of bytes to write
 * @param[out] values      Pointer to array of data (bytes) you wish to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteBlockDataMainMicroIndirectA0(LeoI2CDriverType *i2cDriver,
                                                  uint32_t microIndStructOffset,
                                                  uint32_t address,
                                                  uint8_t lengthBytes,
                                                  uint8_t *values);

/**
 * @brief Write multiple (up to eight) data bytes to specified address from
 * micro SRAM over I2C. Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to write
 * @param[in]  lengthBytes  Number of bytes to write
 * @param[out] values      Pointer to array of data (bytes) you wish to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteBlockDataMainMicroIndirectMPW(
    LeoI2CDriverType *i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t lengthBytes, uint8_t *values);

/**
 * @brief Read one byte of data from specified address from Main micro SRAM
 * over I2C. Returns a negative error code, else the number byte data read.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    16-bit address from which to read
 * @param[out] values      Pointer to single element array of read data
 *                        (one byte)
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadByteDataMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                              uint32_t address,
                                              uint8_t *values);

/**
 * @brief Read multiple (up to eight) data bytes from speified address from
 * Main micro SRAM over I2C. Returns a negative error code, else zero on
 * success
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      16-bit address from which to read
 * @param[in]  lengthBytes  Number of bytes to read (maximum 8 bytes)
 * @param[out] values       Pointer to array storing up to 8 bytes of data
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadBlockDataMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                               uint32_t address,
                                               uint8_t lengthBytes,
                                               uint8_t *values);

/**
 * @brief Write a data byte at specifed address to Main micro SRAM Leo
 * over I2C. Returns a negative error code, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    16-bit address to write
 * @param[in]  value      Byte data to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteByteDataMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                               uint32_t address,
                                               uint8_t *value);

/**
 * @brief Write multiple (up to eight) data bytes at specified address to
 * Main micro SRAM over I2C. Returns a negative error code, else zero on
 * success.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      16-bit address to write
 * @param[in]  lengthBytes  Number of bytes to write (maximum 16 bytes)
 * @param[in]  values       Byte array which will be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteBlockDataMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                                uint32_t address,
                                                uint8_t lengthBytes,
                                                uint8_t *values);

/**
 * @brief Read a data byte at specified address from Path micro SRAM over I2C.
 * Returns a negative error code, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  pathID     Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address    16-bit address from which to read
 * @param[out] value      Pointer to array of one byte of read data
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadByteDataPathMicroIndirect(LeoI2CDriverType *i2cDriver,
                                              uint8_t pathID, uint32_t address,
                                              uint8_t *value);

/**
 * @brief Read multiple (up to eight) data bytes at specified address from
 * Path micro SRAM over I2C. Returns a negative error code, else the number
 * of bytes read.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  pathID       Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address      16-bit address from which to read
 * @param[in]  lengthBytes  Number of bytes to read (maximum 16 bytes)
 * @param[out] values       Pointer to array storing up to 16 bytes of data
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadBlockDataPathMicroIndirect(LeoI2CDriverType *i2cDriver,
                                               uint8_t pathID, uint32_t address,
                                               uint8_t lengthBytes,
                                               uint8_t *values);

/**
 * @brief Write one byte of data byte at specified address to Path micro SRAM
 * Leo over I2C. Returns a negative error code, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  pathID     Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address    16-bit address to write
 * @param[in]  value      Byte data to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteByteDataPathMicroIndirect(LeoI2CDriverType *i2cDriver,
                                               uint8_t pathID, uint32_t address,
                                               uint8_t *value);

/**
 * @brief Write multiple (up to eight) data bytes at specified address to
 * Path micro SRAM over I2C. Returns a negative error code, else zero on
 * success.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  pathID       Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address      16-bit address to write
 * @param[in]  lengthBytes  Number of bytes to write (maximum 16 bytes)
 * @param[in]  values       Byte array which will be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteBlockDataPathMicroIndirect(LeoI2CDriverType *i2cDriver,
                                                uint8_t pathID,
                                                uint32_t address,
                                                uint8_t lengthBytes,
                                                uint8_t *values);

/**
 * @brief Read 2 bytes of data from PMA register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  address      16-bit address from which to read
 * @param[in]  values       Byte array which will be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadWordPmaIndirect(LeoI2CDriverType *i2cDriver, int side,
                                    int quadSlice, uint16_t address,
                                    uint8_t *values);

/**
 * @brief Write 2 bytes of data from PMA register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  address      16-bit address to write
 * @param[in]  values       Byte array which will be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteWordPmaIndirect(LeoI2CDriverType *i2cDriver, int side,
                                     int quadSlice, uint16_t address,
                                     uint8_t *values);

/**
 * @brief Read 2 bytes of data from PMA lane register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  regOffset    16-bit ref offset from which to read
 * @param[in]  values       Byte array to store read data
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadWordPmaLaneIndirect(LeoI2CDriverType *i2cDriver, int side,
                                        int quadSlice, int lane,
                                        uint16_t regOffset, uint8_t *values);

/**
 * @brief Write 2 bytes of data to PMA lane register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  regOffset    16-bit ref offset from which to read
 * @param[in]  values       Byte array which will be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteWordPmaLaneIndirect(LeoI2CDriverType *i2cDriver, int side,
                                         int quadSlice, int lane,
                                         uint16_t regOffset, uint8_t *values);

/**
 * @brief Read 2 bytes of data from PMA lane register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  pmaAddr      16-bit PMA reg offset from which to read
 * @param[in,out]  data       Byte array which will be contain data read
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadWordPmaMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                             int side, int qs, uint16_t pmaAddr,
                                             uint8_t *data);

/**
 * @brief Write 2 bytes of data to PMA lane register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  pmaAddr      16-bit PMA reg offset from which to read
 * @param[in,out]  data       Byte array which will be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteWordPmaMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                              int side, int qs,
                                              uint16_t pmaAddr, uint8_t *data);

/**
 * @brief Read 2 bytes of data from PMA lane register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  pmaAddr      16-bit PMA reg offset from which to read
 * @param[in,out]  data       Byte array which will be contain data read
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadWordPmaLaneMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                                 int side, int qs, int lane,
                                                 uint16_t pmaAddr,
                                                 uint8_t *data);

/**
 * @brief Write 2 bytes of data to PMA lane register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  regOffset    16-bit ref offset from which to read
 * @param[in]  values       Byte array which will be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteWordPmaLaneMainMicroIndirect(LeoI2CDriverType *i2cDriver,
                                                  int side, int qs, int lane,
                                                  uint16_t pmaAddr,
                                                  uint8_t *data);

/**
 * @brief Read N bytes of data from a Retimer (gbl, ln0, or ln1) CSR.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1)
 * @param[in]  lane         Absolute lane number (15:0)
 * @param[in]  baseAddr     16-bit base address
 * @param[in]  lengthBytes  Number of bytes to read
 * @param[in]  data         Byte array which will be contain data read
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadRetimerRegister(LeoI2CDriverType *i2cDriver, int side,
                                    int lane, uint16_t baseAddr,
                                    uint8_t lengthBytes, uint8_t *data);

/**
 * @brief Write N bytes of data to a Retimer (gbl, ln0, or ln1) CSR.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1)
 * @param[in]  lane         Absolute lane number (15:0)
 * @param[in]  baseAddr     16-bit base address
 * @param[in]  lengthBytes  Number of bytes to write
 * @param[in]  data         Byte array containing data to be written
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteRetimerRegister(LeoI2CDriverType *i2cDriver, int side,
                                     int lane, uint16_t baseAddr,
                                     uint8_t lengthBytes, uint8_t *data);

/**
 * @brief Write a data word at specified address to Leo over I2C. Returns a
 * negative error code if unsuccessful, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    17-bit address to write
 * @param[in]  value      32-bit word to write
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoWriteWordData(LeoI2CDriverType *i2cDriver, uint32_t address,
                              uint32_t value);

/**
 * @brief Write a data word at specified address to Leo over I2C. Returns a
 * negative error code if unsuccessful, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    17-bit address to write
 * @param[in]  value      Pointer to 32-bit value
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoReadWordData(LeoI2CDriverType *i2cDriver, uint32_t address,
                             uint32_t *value);

/**
 * @brief Set lock on bus (Leo transaction)
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoLock(LeoI2CDriverType *i2cDriver);

/**
 * @brief Unlock bus after lock (Leo transaction)
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoUnlock(LeoI2CDriverType *i2cDriver);

/**
 * @brief I2C/SMBus types
 */

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_I2C_H_ */

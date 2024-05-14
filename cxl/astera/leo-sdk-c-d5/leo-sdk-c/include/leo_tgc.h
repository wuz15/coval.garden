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
 * @file leo_tgc.h
 * @brief Leo tgc
 */
#ifndef ASTERA_LEO_SDK_TGC_H_
#define ASTERA_LEO_SDK_TGC_H_






#include "stdio.h"
#include "string.h"

#include "leo_error.h"
#include "leo_globals.h"
#include "leo_i2c.h"
#include "misc.h"

/**
 * @file leo_tgc.h
 * @brief Definitions related to TGC data structure and functions.
 */

/**
 * @brief Structure for sram entry.
 */
struct tgcSramEntryStruct {
  uint16_t seqEnd; // Value = 1 indicates last entry in RAM
  uint16_t rsvd126121;
  uint16_t encryptDis; // (Used as initialize in the seq) Value = 1 indicates
                       // CMAL not to encrypt the write data with write commands
  uint16_t rsvd119;
  uint16_t dataCtrl; // Indicates the kind of data pattern to generate
  uint16_t rsvd115102;
  uint16_t wrEn; // Enable writes
  uint16_t rdEn; // Enable reads
  uint16_t rsvd9997;
  uint16_t addrInc;  // 1 = Address counter with increment, 0 = Address counter
                     // with decrement
  uint16_t addrSkip; // 0 = Issue all rd and wr to same address, 1 = Increment
                     // address sequentially >1 New address = Old address +/-
                     // Address skip
  uint16_t rsvd8781;
  uint16_t tranCnt; // Transaction count
  uint16_t rsvd71;
  uint16_t poison;
  uint16_t meta;
  uint16_t rsvd67;
  uint64_t startAddr;
  uint32_t dataPattern; // will use this pattern to create 512b write data based
                        // on [119:116]
};

/**
 * @brief Sram Entry
 *
 * @param[in]  data containing sram entry data structure
 * @return     LeoErrorType - Leo error code
 */
static uint32_t *packedSramEntry(struct tgcSramEntryStruct data);

/**
 * @brief Indirect TGC sram access
 *
 * @param[in]  device Struct containing device information
 * @param[in]  entryNum entry number
 * @param[in]  tgcSramEntryDataArray sram data array
 * @return     LeoErrorType - Leo error code
 */
static int indirectTgcSramAccess(LeoDeviceType *device, uint32_t entryNum,
                                 uint32_t *tgcSramEntryDataArray);

/**
 * @brief function to configure TGC registers
 *
 * @param[in]  device Struct containing device information
 * @param[in]  wrEn To enable write
 * @param[in]  RdEn To enable Read
 * @param[in]  tranCnt Transaction count[0 to 15], 0 for 256 and 15 for 32G
 * @param[in]  startAddr start address for TGC tests
 * @param[in]  dataPattern data pattern for TGC tests, default 0xaabbccdd
 * @return     LeoErrorType - Leo error code
 */
static int configTgc(LeoDeviceType *device, uint16_t wrEn, uint16_t rdEn,
                     uint16_t tranCnt, uint64_t startAddr,
                     uint32_t dataPattern);

/**
 * @brief initialize TGC test
 *
 * @param[in]  device Struct containing device information
 * @return     LeoErrorType - Leo error code
 */
static int initTgc(LeoDeviceType *device);

/**
 * @brief To wait for TGC done
 *
 * @param[in]  device Struct containing device information
 * @return     LeoErrorType - Leo error code
 */
static int wait4tgcDone(LeoDeviceType *device);

/**
 * @brief To read TGC Status registers
 *
 * @param[in]  device Struct containing device information
 * @param[in]  tgcResults Struct containing TGC results ans status
 * @return     LeoErrorType - Leo error code
 */
static int readTgcStatus(LeoDeviceType *device, LeoResultsTgc_t *tgcResults);

#endif /* ASTERA_LEO_SDK_TGC_H_ */

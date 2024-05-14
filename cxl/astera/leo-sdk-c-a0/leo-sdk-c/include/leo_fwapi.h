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
 * @file leo_fwapi.h
 * @brief Definition of public functions for the SDK.
 */

#ifndef ASTERA_LEO_SDK_FWAPI_H_
#define ASTERA_LEO_SDK_FWAPI_H_

#include "leo_api_types.h"
#include "leo_error.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LeoFWUpdateMode_t {
  uint8_t raw;
  uint8_t direct;

} LeoFWUpdateModeType;

typedef struct LeoFwImageFormat_t {
  uint8_t ImageA;
  uint8_t ImageB;
  uint8_t ImageG;
} LeoFwImageFormatType;

/**
 * @brief Check status of FW loaded
 *
 * Check the code load register and the Main Micro heartbeat. If all
 * is good, then read the firmware version
 *
 * @param[in,out]  device  Leo device struct
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoFWStatusCheck(LeoDeviceType *device);

/**
 * @brief Set FW update mode (Raw or Direct Mode)
 *
 *
 * @param[in,out]  device  Leo device struct
 * @param[in]      mode    LeoFWUpdateModeType - FW update mode (Raw or Direct
 * Mode)
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoFWUpdateMode(LeoDeviceType *device, LeoFWUpdateModeType mode);

/**
 * @brief Update the firmware image on the flash connected to Leo device using
 * the SPI interface
 *
 * @param[in,out]  device  Leo device struct
 * @param[in]      fwImagePath   Pointer to the firmware image
 * @param[in]      char* FW Image format type - A, B or G
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoDoFwUpdate(LeoDeviceType *device,
                           LeoFWImageFormatType fwImageFormatType,
                           const char *fwImagePath);

/**
 * @brief Update the firmware image on the flash connected to Leo device using
 * the Leo Tunneling mode (I2C interface) The Leo Tunnel mode SDK API will
 * support firmware update in-band via the CXL through the PCIe interface in the
 * later release (Not supported yet) Only planned for the future releases.
 *
 * @param[in,out]  device  Leo device struct
 * @param[in]      fwImageFormatType FW Image format type - A, B or G
 * @param[in]      uint8_t*   Address or Pointer to the firmware image
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoDoFwUpdateinLTModeRaw(LeoDeviceType *device,
                                      LeoFWImageFormatType fwImageFormatType,
                                      const uint8_t *fwImageBuffer);

/**
 * @brief Check if there is an updated firmware image available
 *
 * @param[out]      fwImageFormatType FW Image format type - A, B or G
 * @param[out]      fwImagePath   Pointer to the firmware image
 * @return     LeoErrorType - Leo error code
 */
void leoCheckforFwUpdate(LeoFWImageFormatType fwImageFormatType,
                         const char *fwImagePath);

/**
 * @brief Get current firmware version/type
 *
 * @param[in,out]  device  Leo device struct
 * @return     LeoFwImageFormatType - FW image format type
 */
LeoFwImageFormatType leoGetCurrentFWVersion(LeoDeviceType *device);

/**
 * @brief Load current firmware version/type
 *
 * @param[in]  fwImageFormatType FW Image format type - A, B or G
 * @param[in]  fwImagePath   Pointer to the firmware image
 */
void LeoLoadCurrentFW(LeoFWImageFormatType fwImageFormatType,
                      const char *fwImagePath);

/**
 * @brief Switch the current firmware version/type (A, B or G)
 *
 * @param[in]  fwImageFormatType FW Image format type - A, B or G
 * @return     unit8_t - FW versio
 */
void LeoSwitchCurrentFW(LeoFWImageFormatType fwImageFormatType);

/**
 * @brief Get the block size of erasable flash
 *
 * @param[in,out]  device  Leo device struct
 * @return     Block Size_t - size of an erasable block
 */
int8_t LeoGetErasableBlockSize(void);

/**
 * @brief dump selected flash area or the entire flash device
 *
 * @param[in]  startAddress  start address of the flash area to be dumped
 * @param[in]  endAddress    end address of the flash area to be dumped
 * @param[in]  dumpFilePath  file path to dump the flash area
 *
 */
void LeoFlashIndexDump(uint32_t startAddress, uint32_t endAddress,
                       const char *fileName);

/**
 * @brief speed up access to wrote to the flash device
 *
 * @param[in]  BlockSize_t block size to be used for the write
 *
 */
void LeoFlashIndexWrite(int8_t blockSize);

/**
 * @brief System Configuration reset to factory default values
 *
 * @param[in]  fwImageFormatType FW Image format type - A, B or G
 *
 */
void LeoSysConfigReset(LeoFWImageFormatType fwImageFormatType);

/**
 * @brief System Configuration Write
 *
 * @param[in] fwImageFormatType FW Image format type - A, B or G
 * @param[in] BlockSize_t block size to be used for the write
 *
 */
void LeoSysConfigWrite(LeoFWImageFormatType fwImageFormatType,
                       int8_t blockSize);

/**
 * @brief System Configuration dump block
 *
 * @param[in] fwImageFormatType FW Image format type - A, B or G
 * @param[in] BlockSize_t block size to be used for the read/dump
 *
 */
void LeoSysConfigDump(LeoFWImageFormatType fwImageFormatType, int8_t blockSize);

/**
 * @brief Persistent data dump from the flash device
 *
 */
void LeoPersistentDataDump(void);

/**
 * @brief Persistent data write to the flash device
 *
 * @param[in]  BlockSize_t block size to be used for the write
 *
 */
void LeoPersistentDataWrite(int8_t blockSize);

/**
 * @brief Persistent data erase from the flash device
 *
 */
void LeoPersistentDataErase(void);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_FWAPI_H_ */
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
 * @file leo_api.h
 * @brief Definition of public functions for the SDK.
 */

#ifndef ASTERA_LEO_SDK_API_H_
#define ASTERA_LEO_SDK_API_H_

#include "astera_log.h"
#include "leo_api_types.h"
#include "leo_error.h"
#include "leo_globals.h"
#include "leo_i2c.h"
#include "leo_logger.h"
#include "leo_mailbox.h"
#include "leo_scrb.h"
#include "leo_spi.h"
#include "leo_evt_rec_mgr.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LEO_SDK_VERSION "0.97" 
#define LEO_MIN_FW_VERSION_SUPPORTED "00.03"

#define DDR_WRITE (0)
#define DDR_READ (1)

/**
 * @brief Return the SDK version
 *
 * @return     uint8_t* - SDK version number
 */
const uint8_t *leoGetSDKVersion(void);

/**
 * @brief Return the running FW version
 *
 * @param[in,out]  device  Leo device struct
 * @return     LeoErrorType: LEO_SUCCESS or LEO_FAILURE
 *             FW Version updated in device.FWVersion
 */
LeoErrorType leoGetFWVersion(LeoDeviceType *device);

/**
 * @brief Return the ASIC version
 *
 * @param[in,out]  device  Leo device struct
 * @return     LeoErrorType: LEO_SUCCESS or LEO_FAILURE
 *             ASIC Version updated in device.asicVersion
 */
LeoErrorType leoGetAsicVersion(LeoDeviceType *device);

/**
 * @brief Check the SDK and FW version compatibility
 *       SDK version must be >= FW version
 * @return    LeoErrorType - Leo error code
 */
int leoSDKVersionCompatibilityCheck(char *fwVersion, char *sdkVersion);

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
 * @brief Initialize Leo device
 *
 * Capture the FW version, device id, vendor id and revision id and store
 * the values in the device struct
 *
 * @param[in,out]  device  Leo device struct
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoInitDevice(LeoDeviceType *device);

/**
 * @brief Leo DDR Memory scrubbing
 *
 * Start patrol (background) scrubbing to systematically correct errors
 * that are correctable.
 *
 * reqScrbEn ( starts from start to end addr)
 * on demand
 * launch it and wait for status
 *
 * @param[in] reqScrbEn - 1 to enable Request scrubbing,
 *                             0 to just Background scrubbing
 * @return    LeoErrorType - Leo Error code
 *
 */
LeoErrorType leoMemoryScrubbing(LeoDeviceType *device,
                                LeoScrubConfigInfoType *scrubConfig);

/**
 * @brief Leo start traffic generator and checker (TGC)
 *
 * @param[in] device  Struct containing device information
 * @param[in] LeoConfigTgc_t TGC settings Wr enable, Rd enable, trans count,
 * start address and data pattern
 * @param[out] LeoResultsTgc_t TGC results status, refer structre documentation
 * @param[out] LeoErrortype
 *
 */

LeoErrorType leoPatternGeneratorandChecker(LeoDeviceType *device,
                                           LeoConfigTgc_t *trafficGenInfo,
                                           LeoResultsTgc_t *tgcResults);

/**
 * @brief Get the key SPD-Serial Presence detect values for the DDR modules
 *
 * Get a quick read of the SPD values
 *
 * @param[in] LeoDeviceType
 * @param[in] LeoSpdInfoType
 * @param[in] dimmIdx
 * @param[out] LeoErrortype
 *
 */

LeoErrorType leoGetSpdInfo(LeoDeviceType *device, LeoSpdInfoType *spdInfo,
                           uint8_t dimmIdx);

/**
 * @brief Get the key SPD-Serial Presence detect values for the DDR modules
 *
 * This API gets the manufacturing related info from SPD
 *
 * @param[in] LeoDeviceType
 * @param[in] LeoSpdInfoType
 * @param[in] dimmIdx
 * @param[out] LeoErrortype
 *
 */
LeoErrorType leoGetSpdManufacturingInfo(LeoDeviceType *device, LeoSpdInfoType *spdInfo,
                           uint8_t dimmIdx);

/**
 * @brief Leo Get the TSOD data for the DDR modules
 *
 * Get the TSOD data for the DDR modules
 * Must be called after DDR modules are trained, and several seconds after
 * leoInitDevice is called, otherwise data may be inaccurate.
 *
 * @param[in] LeoDeviceType
 * @param[in] LeoDimmTsodDataType
 * @param[in] dimmIdx
 * @param[out] LeoErrortype
 *
 */
LeoErrorType leoGetTsodData(LeoDeviceType *device,
                            LeoDimmTsodDataType *tsodData, uint8_t dimmIdx);

/**
 * @brief Leo Set the capability for errory inejction in DDR modules
 *
 * Set the capability to inject ECC errors for DDR on Leo
 *
 * @param[in] device  Struct containing device information
 * @param[in] startAddr Error inject start address
 * @param[in] errType Error type CE or UE 1- Uncorrectable error
 * @param[in] enable Enable or disable error injection, 1 - Enable
 * @param[out] LeoErrortype LeoErrorType
 *
 */

LeoErrorType leoInjectError(LeoDeviceType *device, uint64_t startAddr,
                            uint32_t errType, uint32_t enable);

/**
 * @brief Leo Inject Viral
 *
 * Enables and injects viral in Leo
 * @param[in]  device  Struct containing device information
 * @param[out] LeoErrortype
 *
 */
LeoErrorType leoInjectViral(LeoDeviceType *device);

/**
 * @brief Leo get link status
 *
 * Reads FoM values for each side and lane and populates an array
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  controllerIndex  index of the controller
 * @param[out]  pointer to linkStatus array of 16
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetLinkStatus(LeoDeviceType *device,
                              LeoLinkStatusType *linkStatus);

/**
 * @brief check Memory channel status
 * @param[in] Leo Device pointer
 * @param[in] pointer to memStatus
 * @return    LeoErrorType - Leo error code
 */
LeoErrorType leoGetMemoryStatus(LeoDeviceType *device,
                                LeoMemoryStatusType *memStatus);
/**
 * @brief Update FW from a .mem file
 *
 * @param[in]  device        Pointer to Leo Device struct object
 * @param[in]  flashFileName file path to the flash .mem file
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoFwUpdateFromFile(LeoDeviceType *device, char *flashFileName);

/**
 * @brief Update FW from a .mem file
 *
 * @param[in]  device        Pointer to Leo Device struct object
 * @param[in]  flashFileName file path to the flash .mem file
 * @param[in]  target        0/1/2 Which slot to update (not implemented in 0.8)
 * @param[in]  verify        Verify the flash contents after update
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoFwUpdateTarget(LeoDeviceType *device, char *flashFileName, int target, int verify);

/**
 * @brief Update FW from a .mem file
 *
 * @param[in]  device        Pointer to Leo Device struct object
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoLoadCurrentFW(LeoDeviceType *device);

/**
 * @brief Perform an all-in-one health check on a given Link.
 *
 * Check critical parameters like junction temperature, Link LTSSM state,
 * and per-lane eye height/width against certain alert thresholds. Update
 * link.linkOkay member (bool).
 *
 * @param[in]  link    Pointer to Leo Link struct object
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoCheckLinkHealth(LeoLinkType *link);

/**
 * @brief Leo get Preboot DDR config
 *
 * Reads and decodes DDR config information from Leo registers
 *
 * @param[in]  LeoDeviceType
 * @param[in]  LeoDdrConfigType
 * @return     LeoErrorType
 */
LeoErrorType leoGetPrebootDdrConfig(LeoDeviceType *device,
                                    LeoDdrConfigType *ddrConfig);

/**
 * @brief Leo get Preboot CXL config
 *
 * Reads and decodes CXL config information from Leo registers
 *
 * @param[in]  LeoDeviceType
 * @param[in]  LeoCxlConfigType
 * @return     LeoErrorType
 */
LeoErrorType leoGetPrebootCxlConfig(LeoDeviceType *device,
                                    LeoCxlConfigType *cxlConfig);

/**
 * @brief Leo get ID Info
 *
 * Reads Leo EEPROM information from attached EEPROM
 *
 * @param[in] LeoDeviceType
 * @param[in] LeoIdInfoType
 * @return    LeoErrorType
 */
LeoErrorType leoGetLeoIdInfo(LeoDeviceType *device, LeoIdInfoType *leoIdInfo);

LeoErrorType leoGetSpdDump(LeoDeviceType *device, uint32_t dimm_idx,
                           uint32_t *data);

/**
 * @brief Leo get CMAL Stats
 *
 * Reads Leo registers to get CMAL stats
 * @param[in] LeoDeviceType
 * @param[in] LeoCmalStatsType
 * @return    LeoErrorType
 */
LeoErrorType leoGetCmalStats(LeoDeviceType *device,
                             LeoCmalStatsType *leoCmalStats);

/**
 * @brief Leo get fail vector information
 * return information about fatal errors on the Leo ASIC
 * Reads Leo registers to get CMAL stats
 * @param[in] LeoDeviceType
 * @param[in] LeoFailVectorType
 * @return    LeoErrorType
 */
LeoErrorType leoGetFailVector(LeoDeviceType *device,
                             LeoFailVectorType *leoFailVector);

/**
 * @brief Set Leo Compliance mode for compliance testing
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  cmplMode  Compliance mdoe (1 = CXL Spec 1.1) or (2 =
 * CXL Spec 2.0)
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoSetCxlComplianceMode(LeoDeviceType *devicei, int cmplMode);

/**
 * @brief Get CXL stats including bandwidth
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetCxlStats(LeoDeviceType *device, LeoCxlStatsType *cxlStats,
                            int linkNum);

/**
 * @brief Sample CXL stats over a period of time
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoSampleCxlStats(LeoDeviceType *device,
                               LeoCxlStatsType *resCxlStats, int seconds);

/**
 * @brief Sample Telemetry stats over a period of time
 * @param[in]  device  Struct containing device information
 * @param[in]  seconds Time period to sample the counters
 * @param[in]  cxllink CXL Link to sample the counters
 * @param[out] full pointer to structure holding raw telemetry values
 * @param[out] sample pointer to structure holding sampled telemetry values
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetCxlTelemetry(LeoDeviceType *device, int cxllink,
                                LeoCxlTelemetryType *full,
                                LeoCxlTelemetryType *sample, int seconds);

/**
 * @brief Sample Telemetry stats over a period of time
 * @param[in]  device Struct containing device information
 * @param[in]  seconds Time period to sample the counters
 * @param[in]  ddrch DDR Channel number to sample the counters
 * @param[out] full pointer to structure holding raw telemetry values
 * @param[out] sample pointer to structure holding sampled telemetry values
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetDdrTelemetry(LeoDeviceType *device, int ddrch,
                                LeoDdrTelemetryType *full,
                                LeoDdrTelemetryType *sample, int seconds);

/**
 * @brief Sample Telemetry stats over a period of time
 * @param[in]  device Struct containing device information
 * @param[in]  seconds Time period to sample the counters
 * @param[out] full pointer to structure holding raw telemetry values
 * @param[out] sample pointer to structure holding sampled telemetry values
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetDatapathTelemetry(LeoDeviceType *device,
                                     LeoDatapathTelemetryType *full,
                                     LeoDatapathTelemetryType *sample,
                                     int seconds);

/**
 * @brief Read Leo device serial ID
 * @param[in]  device  Struct containing device information
 * @param[out] dsid - value of dsid
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetDsid(LeoDeviceType *device, uint64_t *dsid);

/**
 * @brief Get persistent data via ID
 * @param[in]  device  Struct containing device information
 * @param[in]  persistentDataId - the ID of data to get
 * @param[out] persistentDataValue - value of persistent data field
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType
leoGetPersistentData(LeoDeviceType *device, uint32_t persistentDataId,
                     LeoPersistentDataValueType *persistentDataValue);

/**
 * @brief Get max DIMM temperature on Leo card
 * @param[in]  device  Struct containing device information
 * @param[in]  handle  Handle to the device
 * @param[in, out]  maxTemp  Pointer to max DIMM temperature variable
 * @param[in]  thresoldTemp  Thresold temperature
 * @param[in, out]  getleoDimmTsodData  Pointer to DIMM TSOD data
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetMaxDimmTemp(LeoDeviceType *leoDevice, int leoHandle,
                               int *maxTemp, uint32_t thresoldTemp,
                               LeoDimmTsodDataType *getleoDimmTsodData);

/**
 * @brief set the throttle count and max BW
 * @param[in]  device  Struct containing device information
 * @param[in]  count  throttle count
 * @param[in]  max_BW  max BW
 * @param[in]  enable  enable/disable throttle
 */
LeoErrorType leoCmndThrottle(LeoDeviceType *leoDevice, uint32_t count,
                             uint32_t max_BW, uint32_t enable);

/*
 * @brief Fuction to throttle the bandwidth of DDR based on temperature
 *        threshold.
 * Threshold setting will change the BW available to the required percentage
 * of the total BW.
 * For example, Full is 100% of available BW,
 *              75 is 75% of available BW,
 *              50 is 50% of available BW,
 *              25 is 25% of available BW.
 *              10 is 12.5% of available BW.
 *
 * @param[in] leoDevice - Leo device structure
 * @param[in] leoHandle - Leo device handle
 * @param[in] Max - is Full BW (100->64)
 * @param[in] count - is throttling percentage
 *            (25->16, 50->32, 75-> or 100->64)
 */
LeoErrorType leoTempBasedBWThrottle(LeoDeviceType *leoDevice, int leoHandle,
                                    int count, int max);

/**
  * @brief Get RxTx training margin
  * @param[in]  device  Struct containing device information
  * @param[in]  ddrcId - ddr controller 0 or 1
  * @param[out] ddrphyRxTxMargin - output if rx_tx margin type 
  * @return     LeoErrorType - Leo error code
  */
LeoErrorType leoGetDdrphyRxTxTrainingMargins(LeoDeviceType *device, uint32_t ddrcId, LeoDdrphyRxTxMarginType *ddrphyRxTxMargin);

/**
  * @brief Get ddrphy training margin
  * @param[in]  device  Struct containing device information
  * @param[in]  ddrcId - ddr controller 0 or 1
  * @param[out] ddrphyCsCaMargin - output if cs_ca margin type
  * @return     LeoErrorType - Leo error code
  */
LeoErrorType leoGetDdrphyCsCaTrainingMargins(LeoDeviceType *device, uint32_t ddrcId, LeoDdrphyCsCaMarginType *ddrphyCsCaMargin);

/**
  * @brief Get ddrphy training margin
  * @param[in]  device  Struct containing device information
  * @param[in]  ddrcId - ddr controller 0 or 1
  * @param[out] ddrphyQcsQcaMargin - output if QcsQca margin type
  * @return     LeoErrorType - Leo error code
  */
LeoErrorType leoGetDdrphyQcsQcaTrainingMargins(LeoDeviceType *device, uint32_t ddrcId, LeoDdrphyQcsQcaMarginType *ddrphyQcsQcaMargin);

/**
 * @brief Set Leo DDR Access to test write
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  dpa  Device Physical Address
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoWriteDDR(LeoDeviceType *device, uint64_t dpa);

/**
 * @brief Set Leo DDR Access to test read
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  dpa  Device Physical Address
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoReadDDR(LeoDeviceType *device, uint64_t dpa);


/**
 * @brief Read Leo EEPROM word
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  addr    Address of the EEPROM to read
 * @param[out] data    Value of EEPROM at given address
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoReadEepromWord(LeoDeviceType *device, uint32_t addr, uint32_t* data);

/**
 * @brief Read recent UART rx character
 *
 * @param[in]  device     Struct containing device information
 * @param[out] uart_char  Most recent uart character rx
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetRecentUartRx(LeoDeviceType *device, char* uart_char);

/**
 * @brief Read Leo error information
 *
 * @param[in]  device       Struct containing device information
 * @param[out] leo_err_info address of leo error info
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetErrInfo(LeoDeviceType *device, leo_err_info_t *leo_err_info);

/**
 * @brief Leo repair given row in a DIMM
 *
 * Takes the channel, rank, bankgroup, bank and row to repair if possible
 *
 * @param[in]  repairRecord that has the inputs
 * @param[in]  sets pStatus to LeoSuccess if SPPR is successful else LeoFailure

 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoRunSPPR(LeoDeviceType *device,
                          LeoRepairRecordType *repairRecord);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_API_H_ */

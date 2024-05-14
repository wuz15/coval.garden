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
 * @file leo_scrb.h
 * @brief Definitions related to DDR Scrubbing functions.
 */

#ifndef ASTERA_LEO_SDK_SCRB_H_
#define ASTERA_LEO_SDK_SCRB_H_

#include "leo_api_types.h"
#include "leo_i2c.h"
//#include "misc.h"
#include <stdint.h>
#include <stdio.h>


/**
 * @brief low level implementation of Configure background scrub
 *
 * @param[in]  LeoI2CDriver     I2C device driver
 * @param[in]  bkgrd_scrb_en    1 or 0 - wheter to enable/disable
 *                              background scrub
 * @param[in]  bkgrd_dpa_end_addr size of total memory capacity
 *                               (End of dpa address for background scrub)
 * @param[in]  bkgrd_round_interval This is the interval in seconds between two
 *              consecutive rounds of scrubbing rounds (default = 12 hrs)
 * @param[in]  bkgrd_cmd_interval Number of cycles between two consecutive
 *              scrubbing commands (default 1024 cycles)
 * @param[in]  bkgrd_wrpoison_if_uncorr_err Poison DDR Cache line if
 * uncorrectable error is detected.
 */
void configBkgrdScrb(LeoI2CDriverType *LeoI2CDriver, uint32_t bkgrd_scrb_en,
                     uint64_t bkgrd_dpa_end_addr, uint32_t bkgrd_round_interval,
                     uint32_t bkgrd_cmd_interval,
                     uint32_t bkgrd_wrpoison_if_uncorr_err);

/**
 * @brief low level implementation of Configure Request scrub
 *
 * @param[in]  LeoI2CDriver     I2C device driver
 * @param[in]  req_scrb_en    1 or 0 - Request Scrub enable/disable
 * @param[in]  req_scrb_dpa_start_addr start of address to start req scrub
 * @param[in]  req_scrb_dpa_end_addr (End of address for request scrub)
 * @param[in]  req_scrb_cmd_interval command interval
 *
 */
void configReqScrb(LeoI2CDriverType *LeoI2CDriver, uint32_t req_scrb_en,
                   uint64_t req_scrb_dpa_start_addr,
                   uint64_t req_scrb_dpa_end_addr,
                   uint32_t req_scrb_cmd_interval);
/**
 * @brief low level implementation of OnDemand Scrub configuration
 *
 * @param[in]  LeoDriver     I2C device driver
 * @param[in]  onDemand_scrb_en    1 or 0 - OnDemand Scrub enable/disable
 * @param[in]  wrpoison_if_uncorr_err Poison DDR Cache line if uncorrectable
 *              error is detected.
 */
void configOnDemandScrb(LeoI2CDriverType *leoDriver, uint32_t onDemand_scrb_en,
                        uint32_t wrpoison_if_uncorr_err);

/**
 * @brief low level implementation of wait for request scrub done signal
 *
 * @param[in]  LeoDriver     Leo I2C device driver
 */
void wait4ReqScrbDone(LeoI2CDriverType *leoDriver);

/**
 * @brief low level implementation to read status of scrubbing activity.
 *
 * @param[in]  device     Struct containing device information
 *
 */
void readScrbStatus(LeoDeviceType *device);

#endif /* ASTERA_LEO_SDK_SCRB_H_ */

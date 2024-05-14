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
 * @file leo_logger.h
 * @brief Definition of helper functions used by Leo SDK
 */

#ifndef ASTERA_LEO_SDK_LOGGER_H_
#define ASTERA_LEO_SDK_LOGGER_H_

#include "leo_api_types.h"
#include "leo_error.h"
#include "leo_globals.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t leoGetPecByte(uint8_t *polynomial, uint8_t length);
int leoGetStartLane(LeoLinkType *link);
/**
 * @brief Check if one batch mode in the LTSSM logger is enabled or not
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  oneBatchModeEn    One batch mode enabled or not
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoGetLoggerOneBatchModeEn(LeoLinkType *link,
                                        LeoLTSSMLoggerEnumType loggerType,
                                        int *oneBatchModeEn);
/**
 * @brief Check if one batch write in the LTSSM logger is enabled or not
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  oneBatchWrEn    One batch write enabled or not
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoGetLoggerOneBatchWrEn(LeoLinkType *link,
                                      LeoLTSSMLoggerEnumType loggerType,
                                      int *oneBatchWrEn);

/**
 * @brief Check if one batch write in the LTSSM logger is enabled or not
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  oneBatchWrEn    One batch write enabled or not
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoGetLoggerOneBatchWrEn(LeoLinkType *link,
                                      LeoLTSSMLoggerEnumType loggerType,
                                      int *oneBatchWrEn);

/**
 * @brief Get the write pointer location in the LTSSM logger
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  writeOffset    Current write offset
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoGetLoggerWriteOffset(LeoLinkType *link,
                                     LeoLTSSMLoggerEnumType loggerType,
                                     int *writeOffset);

/**
 * @brief Get the format ID at offset int he print buffer
 *
 * @param[in]  link         Link struct created by user
 * @param[in]  loggerType   Logger type (main or which path)
 * @param[in]  offset       Print buffer offset
 * @param[in/out]  fmtID    Format ID from logger
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoGetLoggerFmtID(LeoLinkType *link,
                               LeoLTSSMLoggerEnumType loggerType, int offset,
                               int *fmtID);

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

#ifdef __cplusplus
}
#endif

#endif // ASTERA_LEO_SDK_LOGGER_H_
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
 * @file leo_logger.c
 * @brief Implementation of miscellaneous functions for the SDK.
 */
#include "../include/leo_logger.h"

/* Revisit and fix this as these need to have LEO specific values*/
#define LEO_PRINT_INFO_STRUCT_ONE_BATCH_MODE_EN_OFFSET 45

#ifdef __cplusplus
extern "C" {
#endif

uint8_t leoGetPecByte(uint8_t *polynomial, uint8_t length) {
  uint8_t crc;
  int byteIndex;
  int bitIndex;

  // Shift polynomial by 1 so that it is 8 bit
  // Take this extra bit into account later
  uint8_t poly = LEO_CRC8_POLYNOMIAL >> 1;
  crc = polynomial[0];

  for (byteIndex = 1; byteIndex < length; byteIndex++) {
    uint8_t nextByte = polynomial[byteIndex];
    for (bitIndex = 7; bitIndex >= 0; bitIndex--) {
      // Check if MSB in CRC is 1
      if (crc & 0x80) {
        // Perform subtraction of first 8 bits
        crc = (crc ^ poly) << 1;
        // Add final bit of mod2 subtraction and place in pos 0 of CRC
        crc = crc + (((nextByte >> bitIndex) & 1) ^ 1);
      } else {
        // Shift out 0
        crc = crc << 1;
        // Shift in next bit
        crc = crc + ((nextByte >> bitIndex) & 1);
      }
    }
  }
  return crc;
}

int leoGetStartLane(LeoLinkType *link) {
  int startLane = link->config.startLane;
  if (link->config.partNumber == LEO_PTX08) {
    startLane += 4;
  }

  return startLane;
}

/*
 * Get One Batch Mode En from current location
 */
LeoErrorType leoGetLoggerOneBatchModeEn(LeoLinkType *link,
                                        LeoLTSSMLoggerEnumType loggerType,
                                        int *oneBatchModeEn) {
  return LEO_FUNCTION_NOT_IMPLEMENTED; /*Stub for now*/
}

/*
 * Get One Batch Write En from current location
 */
LeoErrorType leoGetLoggerOneBatchWrEn(LeoLinkType *link,
                                      LeoLTSSMLoggerEnumType loggerType,
                                      int *oneBatchWrEn) {
  return LEO_FUNCTION_NOT_IMPLEMENTED; /*Stub for now*/
}
/*
 * Check link health.
 * Check for eye height, width against thresholds
 * Check for temperature thresholds
 */
LeoErrorType leoCheckLinkHealth(LeoLinkType *link) {
  return LEO_FUNCTION_NOT_IMPLEMENTED; /*Stub for now*/
}

#ifdef __cplusplus
}
#endif

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
 * @file leo_error.h
 * @brief Definition of error types for the SDK.
 */

#ifndef ASTERA_LEO_SDK_ERROR_H_
#define ASTERA_LEO_SDK_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Used to avoid warnings in case of unused parameters in function pointers */
#define LEO_UNUSED(x) (void)(x)

/**
 * @brief Leo Error enum
 *
 * Enumeration of return values from the leo* functions within the SDK.
 * Value of 0 is a generic success response
 * Values less than 1 are specific error return codes
 */
typedef enum {
  /* General purpose errors */
  /** Success return value, no error occured */
  LEO_SUCCESS = 0,
  /** Generic failure code */
  LEO_FAILURE = -1,
  /** Invalid function argument */
  LEO_INVALID_ARGUMENT = -2,
  /** Failed to open connection to I2C slave */
  LEO_I2C_OPEN_FAILURE = -3,

  /* I2C Access failures*/
  /** I2C read transaction to Leo failure */
  LEO_I2C_BLOCK_READ_FAILURE = -4,
  /** I2C write transaction to Leo failure */
  LEO_I2C_BLOCK_WRITE_FAILURE = -5,
  /** Indirect SRAM access mechanism did not provide expected status */
  LEO_FAILURE_SRAM_IND_ACCESS_TIMEOUT = -6,
  /* Main Micro access failures */
  /** Failure in PMA access via Main Micro */
  LEO_PMA_MM_ACCESS_FAILURE = -7,
  /** SDK Function not yet implemented */
  LEO_FUNCTION_NOT_IMPLEMENTED = -8,

  /** FW not running */
  LEO_FW_NOT_RUNNING = -9,

  /** FW and SDK version mismatch */
  LEO_FW_SDK_VERSION_MISMATCH = -10,

  /** Function's operation was unsuccessful */
  LEO_FUNCTION_UNSUCCESSFUL = -16
} LeoErrorType;

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_ERROR_H_ */

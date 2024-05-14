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
 * @file leo_globals.h
 * @brief Definition of enums and structs globally used by the SDK.
 */

#ifndef ASTERA_LEO_SDK_GLOBALS_H
#define ASTERA_LEO_SDK_GLOBALS_H

/** CRC8 polynomial used in PEC computation */
#define LEO_CRC8_POLYNOMIAL 0x107

/** Leo SMBus device map */
#define LEO_DEV_MUX 0x77      //TODO: A0 access through Leo
#define LEO_DEV_GPIO_0 0x20   
#define LEO_DEV_GPIO_1 0x21


#ifdef CHIP_D5
#define LEO_DEV_LEO_0 0x27
#define LEO_DEV_LEO_1 0x26
#define LEO_DEV_FRU   0x56   
#define LEO_DEV_DPM   0x11
#define LEO_DEV_VRM   0x60
#elif CHIP_A0
#define LEO_DEV_LEO_0 0x1D
#define LEO_DEV_LEO_1 0x26   //TODO YW: later to ERROR when used
#define LEO_DEV_FRU   0x53   
#define LEO_DEV_DPM   0x33
#define LEO_DEV_VRM   0x68
#else
#error No chip revision provided
#endif

#define LEO_DEV_DIM0 0x50
#define LEO_DEV_DIM1 0x51
#define LEO_DEV_DIM2 0x50
#define LEO_DEV_DIM3 0x51

//////////////////////////////////////
////////// Memory parameters //////////
//////////////////////////////////////

/** Main Micro num of print class enables in logger module */
#define LEO_MM_PRINT_INFO_NUM_PRINT_CLASS_EN 8

/** Main Micro print buffer size */
#define LEO_MM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE 512

/** Path Micro num of print class enables in logger module */
#define LEO_PM_PRINT_INFO_NUM_PRINT_CLASS_EN 16

/** Path Micro print buffer size */
#define LEO_PM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE 256

/**
 * @brief Enumeration of Astera Product device types
 * this typedef is not specific to Leo but applies to other Astera products as
 * well.
 */
typedef enum AsteraProduct {
  ASTERA_ARIES,  /**< Astera Aries retimer product */
  ASTERA_TAURUS, /**< Astera Ethernet Smart Cable Module */
  ASTERA_LEO     /**< Astera Leo CXL product */
} AsteraProductType;

#endif /* ASTERA_LEO_SDK_GLOBALS_H */

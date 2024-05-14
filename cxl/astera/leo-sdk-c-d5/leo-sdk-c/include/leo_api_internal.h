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
 * @file leo_api_internal.h
 * @brief Definition of Internal registers.
 */

#ifndef ASTERA_LEO_SDK_API_INTERNAL_H_
#define ASTERA_LEO_SDK_API_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

// Copy of Internal registers
//TODO YW: these are values leave it for now
#define LEO_BOOT_STAT_FW_RUNNING 0x4
#define LEO_MUC_RST_MASK 0x1000

// #define LEO_MISC_RST_ADDR 0x00008 //TODO YW: check if still needed

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_API_INTERNAL_H_ */

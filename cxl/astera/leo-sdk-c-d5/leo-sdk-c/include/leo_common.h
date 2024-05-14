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
 * @file leo_common.h
 * @brief Definition of enums and structs globally used by the SDK.
 */

#ifndef ASTERA_LEO_SDK_COMMON_H
#define ASTERA_LEO_SDK_COMMON_H

#define PCIE_SEGMENT "0000:"
#define CXL_CTRL1 0x6
#define CXL_CTRL2 0x7
#define DDR_PHY1 0xc
#define DDR_PHY2 0xd
#define CXL_PHY 0x2
#define CXL_PHY_CH1 0x8
#define CXL_PHY_CH2 0x9
#define CXL_PHY_CH3 0xa
#define CXL_PHY_CH4 0xb

int bdfToSysfs(char *bdf, char **retSysBdf);
int isAddressMailbox(uint32_t address);

#endif /* ASTERA_LEO_SDK_COMMON_H */

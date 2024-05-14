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
 * @file leo_common.c
 * @brief Implementation of common utility functions used by the SDK.
 */

#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/leo_common.h"

char devicefile[4096];
char inbdf[32];

static int walk_sysfs(const char *sysfs, const struct stat *sb, int tflag,
                      struct FTW *ftwbuf) {
  char *leo = NULL;
  char *skip = NULL;

  skip = strstr(sysfs, "iommu");
  if (skip != NULL) {
    return 0; /* continue tree walk */
  }

  skip = NULL;
  skip = strstr(sysfs, "ivhd");
  if (skip != NULL) {
    return 0; /* continue tree walk */
  }

  leo = strstr(sysfs, inbdf);
  if (leo != NULL) {
    strcpy(devicefile, sysfs);
    return 1; /* end tree walk by returning non-zero */
  }

  return 0; /* continue tree walk */
}

int bdfToSysfs(char *bdf, char **retSysBdf) {
  devicefile[0] = '\0';
  inbdf[0] = '\0';

  if (bdf == NULL) {
    *retSysBdf = NULL;
    return -1;
  }

  strcpy(inbdf, PCIE_SEGMENT);
  strcat(inbdf, bdf);

  if (nftw("/sys/devices", walk_sysfs, 64, FTW_PHYS) == -1) {
    *retSysBdf = NULL;
    return -1;
  }

  if (devicefile[0] == '\0') {
    *retSysBdf = NULL;
    return -1;
  }

  *retSysBdf = devicefile;
  return 0;
}

int isAddressMailbox(uint32_t address) {
  int hwMod = (address & 0xf00000) >> 20;
  int hwSubMod = (address & 0x0f0000) >> 16;

  if (hwMod == CXL_CTRL1 || hwMod == CXL_CTRL2 || hwMod == DDR_PHY1 ||
      hwMod == DDR_PHY2) {
    return 1;
  }

  if (hwMod == CXL_PHY) {
    if (hwSubMod == CXL_PHY_CH1 || hwSubMod == CXL_PHY_CH2 ||
        hwSubMod == CXL_PHY_CH3 || hwSubMod == CXL_PHY_CH4) {
      return 1;
    }
  }

  return 0;
}

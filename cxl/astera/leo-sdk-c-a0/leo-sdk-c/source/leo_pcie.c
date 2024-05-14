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
 * @file leo_pcie.c
 * @brief Implementation of public functions for the PCIe interface.
 */

#include "../include/leo_pcie.h"
#include <stdint.h>

#define WORD_SIZE 4

LeoErrorType leoReadWordPcie(const char *leoSysfsPath, off_t baroff,
                             uint32_t *value) {

  int mapsz = 4096UL;
  off_t aligned_baroff;
  void *base_va, *data_va;

  // FIXME - open/close once
  int barfd = open(leoSysfsPath, O_RDWR | O_SYNC);
  if (barfd == -1) {
    printf("Error in opening %s\n", leoSysfsPath);
    PRINT_ERROR;
  }

  aligned_baroff = baroff & ~(sysconf(_SC_PAGE_SIZE) - 1);
  if (baroff + WORD_SIZE - aligned_baroff > mapsz)
    mapsz = baroff + WORD_SIZE - aligned_baroff;

  base_va =
      mmap(0, mapsz, PROT_READ | PROT_WRITE, MAP_SHARED, barfd, aligned_baroff);
  if (base_va == (void *)-1)
    PRINT_ERROR;

  data_va = (uint8_t *)base_va + baroff - aligned_baroff;
  *value = *((uint32_t *)data_va);

  if (munmap(base_va, mapsz) == -1)
    PRINT_ERROR;
  close(barfd);

  return LEO_SUCCESS;
}

LeoErrorType leoWriteWordPcie(const char *leoSysfsPath, off_t baroff,
                              uint32_t writeval) {

  int mapsz = 4096UL;
  off_t aligned_baroff;
  void *base_va, *data_va;

  int barfd = open(leoSysfsPath, O_RDWR | O_SYNC);
  if (barfd == -1)
    PRINT_ERROR;

  aligned_baroff = baroff & ~(sysconf(_SC_PAGE_SIZE) - 1);
  if (baroff + WORD_SIZE - aligned_baroff > mapsz)
    mapsz = baroff + WORD_SIZE - aligned_baroff;

  base_va =
      mmap(0, mapsz, PROT_READ | PROT_WRITE, MAP_SHARED, barfd, aligned_baroff);
  if (base_va == (void *)-1)
    PRINT_ERROR;

  data_va = (uint8_t *)base_va + baroff - aligned_baroff;
  *((uint32_t *)data_va) = writeval;

  if (munmap(base_va, mapsz) == -1)
    PRINT_ERROR;
  close(barfd);

  return LEO_SUCCESS;
}

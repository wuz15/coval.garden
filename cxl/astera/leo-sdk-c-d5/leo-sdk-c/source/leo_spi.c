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
 * @file leo_spi.c
 * @brief Implementation of public functions for the SDK SPI interface.
 */

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "../include/DW_apb_ssi.h"
#include "../include/hal.h"
#include "../include/leo_api_types.h"
#include "../include/leo_globals.h"
#include "../include/leo_i2c.h"
#include "../include/leo_mailbox.h"
#include "../include/leo_spi.h"
#include "../include/leo_api.h"
//#include "../include/misc.h"

#define FW_ASSIST 1

flash_boot_load_ctrl_info_t g_flash_boot_load_ctrl_info;

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
void flash_boot_load_init_ctrl_info_struct(void);

LeoErrorType flash_read_32(
    LeoI2CDriverType *leoDriver,
    uint32_t       addr,
    uint32_t       *data
    );

LeoErrorType find_block_end(
    LeoI2CDriverType *leoDriver,
    uint32_t       block_start_addr,
    uint32_t       *block_end_addr,
    uint8_t        *mem_data
    );

LeoErrorType find_next_block(
    LeoI2CDriverType *leoDriver,
    uint32_t      addr,
    uint32_t      skip,
    uint32_t      *next_block_addr,
    uint8_t       *mem_data
    );

LeoErrorType get_block_size(
    LeoI2CDriverType *leoDriver,
    uint32_t addr,
    uint32_t *block_size,
    uint8_t  *mem_data
    );

LeoErrorType find_block_by_type(
    LeoI2CDriverType *leoDriver,
    BLOCKTYPE    block_type,
    block_info_t *block_info,
    uint8_t           *mem_data
    );

LeoErrorType get_block_info(
    LeoI2CDriverType *leoDriver,
    uint32_t block_start_addr,
    block_info_t *block_info,
    uint8_t  *mem_data
    );

LeoErrorType read_block_data(
    LeoI2CDriverType *leoDriver,
    block_info_t block_info,
    uint32_t *block_data,
    uint8_t  *mem_data
    );

//------------------------------------------------------------------------------
// Function: flash_boot_load_init_ctrl_info_struct
// Description:  This routine sets SSI ctrl info to default values.
//------------------------------------------------------------------------------
void flash_boot_load_init_ctrl_info_struct() {
  g_flash_boot_load_ctrl_info.curr_read_addr = 0;

  g_flash_boot_load_ctrl_info.dw_apb_ssi_baudr.word = 0;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_baudr.SCKDV = DW_APB_SSI_BAUDR_SCKDV;

  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.word = 0;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.SCPOL =
      DW_APB_SSI_CTRL_R0_SCLK_LOW_SCPOL;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.SCPH =
      DW_APB_SSI_CTRL_R0_SCPH_MIDDLE_SCPH;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.FRF =
      DW_APB_SSI_CTRL_R0_MOTOROLA_SPI_FRF;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.SPI_FRF =
      DW_APB_SSI_CTRL_R0_STD_SPI_FRF;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.TMOD =
      DW_APB_SSI_CTRL_R0_TX_ONLY_TMOD;
  /* g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.TMOD           =
   * DW_APB_SSI_CTRL_R0_EEPROM_RD_TMOD; */
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.DFS_32 =
      DW_APB_SSI_CTRL_R0_32_BIT_FRAME_DFS_32;

  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_r1.word = 0;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_r1.NDF =
      DW_APB_SSI_RX_FIFO_SIZE - 1;

  g_flash_boot_load_ctrl_info.dw_apb_ssi_rx_sample_dly.word = 0;

  //
  // XXX just in case we want to experiment with different mode
  //
  g_flash_boot_load_ctrl_info.dw_apb_ssi_spi_ctrl_ro.word = 0;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_spi_ctrl_ro.INST_L = 2;
  //
  // TODO
  //
  /* g_flash_boot_load_ctrl_info.timeout_loop_cnt = 0xffff; */
  g_flash_boot_load_ctrl_info.timeout_loop_cnt = 0x100;
  g_flash_boot_load_ctrl_info.status = FLASH_BOOT_LOAD_STATUS_OK;
}

/**
 * @brief : dw apb ssi init
 */
void leoSpiInit(LeoI2CDriverType *leoDriver) {
  flash_boot_load_init_ctrl_info_struct();
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset;
  uint32_t drx_word;
  uint32_t tx_ok;
  DW_apb_ssi_ssienr_t dw_apb_ssi_ssienr;
  DW_apb_ssi_baudr_t dw_apb_ssi_baudr;
  DW_apb_ssi_ctrl_ro_t dw_apb_ssi_ctrl_ro;
  DW_apb_ssi_ctrl_r1_t dw_apb_ssi_ctrl_r1;
  DW_apb_ssi_ser_t dw_apb_ssi_ser;
  volatile DW_apb_ssi_sr_t dw_apb_ssi_sr;
  int i;
  uint32_t read_word;

  // clear error status
  g_flash_boot_load_ctrl_info.status = FLASH_BOOT_LOAD_STATUS_OK;
  dw_apb_ssi_mem_map_offset = offsetof(DW_apb_ssi_mem_map_t, ICR);
  leoReadWordData(leoDriver,
                  (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                  &read_word);

  dw_apb_ssi_ssienr.word = 0;
  dw_apb_ssi_ssienr.SSI_EN = FALSE;
  dw_apb_ssi_SSIENR(leoDriver, dw_apb_ssi_ssienr.word);

  g_flash_boot_load_ctrl_info.dw_apb_ssi_baudr.SCKDV = DW_APB_SSI_BAUDR_SCKDV;
  dw_apb_ssi_mem_map_offset = offsetof(DW_apb_ssi_mem_map_t, BAUDR);
  leoWriteWordData(leoDriver,
                   (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                   g_flash_boot_load_ctrl_info.dw_apb_ssi_baudr.word);

  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.TMOD =
      DW_APB_SSI_CTRL_R0_TX_ONLY_TMOD;
  g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.DFS_32 =
      DW_APB_SSI_CTRL_R0_32_BIT_FRAME_DFS_32;
  dw_apb_ssi_CTRLR0(leoDriver,
                    g_flash_boot_load_ctrl_info.dw_apb_ssi_ctrl_ro.word);

  uint32_t ndf = 0;
  dw_apb_ssi_ctrl_r1.word = 0;
  dw_apb_ssi_ctrl_r1.NDF = ndf;
  dw_apb_ssi_CTRLR1(leoDriver, dw_apb_ssi_ctrl_r1.word);

  dw_apb_ssi_mem_map_offset = offsetof(DW_apb_ssi_mem_map_t, SPI_CTRLRO);
  leoWriteWordData(leoDriver,
                   (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                   g_flash_boot_load_ctrl_info.dw_apb_ssi_spi_ctrl_ro.word);

  dw_apb_ssi_ser.word = 0;
  dw_apb_ssi_ser.SER = TRUE;
  dw_apb_ssi_SER(leoDriver, dw_apb_ssi_ser.word);

  dw_apb_ssi_ssienr.word = 0;
  dw_apb_ssi_ssienr.SSI_EN = TRUE;
  dw_apb_ssi_SSIENR(leoDriver, dw_apb_ssi_ssienr.word);

  /*  FLASH_RESET_ENABLE COMMAND */
  drx_word = 0x66;
  dw_apb_ssi_DRx(leoDriver, drx_word);

  dw_apb_ssi_mem_map_offset = offsetof(DW_apb_ssi_mem_map_t, SR);
  tx_ok = 0;
  for (i = 0; i < g_flash_boot_load_ctrl_info.timeout_loop_cnt; i++) {
    leoReadWordData(leoDriver,
                    (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                    &read_word);
    dw_apb_ssi_sr.word = read_word;
    fflush(stdout);
    if (dw_apb_ssi_sr.TFE && (!dw_apb_ssi_sr.BUSY)) {
      tx_ok = 1;
      break;
    }
  }
  if (tx_ok != 1) {
    /* flash_boot_load_check_error (); */
    g_flash_boot_load_ctrl_info.status |= FLASH_BOOT_LOAD_ERROR_TIMEOUT;
    ASTERA_WARN("** warning : init: reset enable timeout: %x",
                g_flash_boot_load_ctrl_info.status);
    return;
  }

  /* FLASH_RESET_MEMORY COMMAND */
  drx_word = 0x99;
  dw_apb_ssi_DRx(leoDriver, drx_word);

  tx_ok = 0;
  for (i = 0; i < g_flash_boot_load_ctrl_info.timeout_loop_cnt; i++) {
    leoReadWordData(leoDriver,
                    (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                    &read_word);
    dw_apb_ssi_sr.word = read_word;
    fflush(stdout);
    if (dw_apb_ssi_sr.TFE && (!dw_apb_ssi_sr.BUSY)) {
      tx_ok = 1;
      break;
    }
  }
  if (tx_ok != 1) {
    /* flash_boot_load_check_error (); */
    g_flash_boot_load_ctrl_info.status |= FLASH_BOOT_LOAD_ERROR_TIMEOUT;
    ASTERA_WARN("** warning : init: reset memory timeout: %x",
                g_flash_boot_load_ctrl_info.status);
    return;
  }
}

void dw_apb_ssi_SSIENR(LeoI2CDriverType *leoDriver, uint32_t enable) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, SSIENR); // 0x8
  leoWriteWordData(
      leoDriver, (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), enable);
}

void dw_apb_ssi_SER(LeoI2CDriverType *leoDriver, uint32_t enable) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, SER); // 0x10
  leoWriteWordData(
      leoDriver, (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), enable);
}

void dw_apb_ssi_CTRLR0(LeoI2CDriverType *leoDriver, uint32_t CTRLR0_val) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, CTRLR0); // 0x00
  leoWriteWordData(leoDriver,
                   (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                   CTRLR0_val);
}

void dw_apb_ssi_CTRLR1(LeoI2CDriverType *leoDriver, uint32_t CTRLR1_val) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, CTRLR1); // 0x04
  leoWriteWordData(leoDriver,
                   (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                   CTRLR1_val);
}

void dw_apb_ssi_RXFLR(LeoI2CDriverType *leoDriver, uint32_t *value) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, RXFLR); // 0x24
  leoReadWordData(leoDriver,
                  (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), value);
}

void dw_apb_ssi_TXFLR(LeoI2CDriverType *leoDriver, uint32_t *value) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, TXFLR); // 0x20
  leoReadWordData(leoDriver,
                  (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), value);
}

void dw_apb_ssi_DRx_read_16_dw(LeoI2CDriverType *leoDriver, uint32_t *values) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, DRx[0]); // 0x60
#if FW_ASSIST
  int mb_sts;
  mb_sts = execOperation(
      leoDriver, (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
      (FW_API_MMB_CMD_OPCODE_MMB_CSR_READ), NULL, 0, values, 16);
#endif
}

void dw_apb_ssi_DRx_read(LeoI2CDriverType *leoDriver, uint32_t *value) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, DRx[0]); // 0x60
  uint32_t read_buf[16];
#if FW_ASSIST
  int mb_sts;
  mb_sts = execOperation(
      leoDriver, (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
      (FW_API_MMB_CMD_OPCODE_MMB_CSR_READ), NULL, 0, read_buf, 1);
  *value = read_buf[0];
#else
  leoReadWordData(leoDriver,
                  (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), value);

#endif
  dw_apb_ssi_mem_map_offset = offsetof(DW_apb_ssi_mem_map_t, ICR); // 0x48
  uint32_t tmp32;
  leoReadWordData(
      leoDriver, (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), &tmp32);
}

void dw_apb_ssi_DRx(LeoI2CDriverType *leoDriver, uint32_t DRx_val) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, DRx[0]); // 0x60
  leoWriteWordData(leoDriver,
                   (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset),
                   DRx_val);
}

void dw_apb_ssi_DRx_read_direct(LeoI2CDriverType *leoDriver, uint32_t *value) {
  uint32_t dw_apb_ssi_mem_map_addr = DW_APB_SSI_ADDRESS; // 0x6000
  uint32_t dw_apb_ssi_mem_map_offset =
      offsetof(DW_apb_ssi_mem_map_t, DRx[0]); // 0x60
  uint32_t read_buf[16];
  leoReadWordData(leoDriver,
                  (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), value);

  dw_apb_ssi_mem_map_offset = offsetof(DW_apb_ssi_mem_map_t, ICR); // 0x48
  uint32_t tmp32;
  leoReadWordData(
      leoDriver, (dw_apb_ssi_mem_map_addr + dw_apb_ssi_mem_map_offset), &tmp32);
}

void flash_read_sts_reg(LeoI2CDriverType *leoDriver, uint32_t *value) {
  dw_apb_ssi_SER(leoDriver, 0);
  dw_apb_ssi_SSIENR(leoDriver, 0);
  dw_apb_ssi_CTRLR0(leoDriver, 0x070300);
  dw_apb_ssi_SSIENR(leoDriver, 1);
  uint32_t DRx_val = 0x05;
  dw_apb_ssi_DRx(leoDriver, DRx_val);
  dw_apb_ssi_SER(leoDriver, 1);
  uint32_t rxflr = 0;
  while (rxflr == 0) {
    dw_apb_ssi_RXFLR(leoDriver, &rxflr);
  }
  uint32_t buffer[1];
  dw_apb_ssi_DRx_read_direct(leoDriver, buffer);
  *value = (buffer[0] & 0xff);
}

void leo_spi_flash_write_block_protect(LeoI2CDriverType *leoDriver,
                                       int spi_device,
                                       uint8_t block_protect_en) {
#ifdef LEO_CSDK_DEBUG
  ASTERA_DEBUG("**INFO : leo_spi_flash_write_block_protect(%d)\n",
               block_protect_en);
#endif
  uint32_t DRx_val;
  size_t ii;
  uint8_t block_protect_en_buffer[] = {0x55, 0x55, 0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  if (SPI_DEVICE_SST26WF064C_e != spi_device) {
      ASTERA_INFO("Not SST26WF064C.  No write block protect command\n");
      return;
  }

  flash_write_enable(leoDriver);
  dw_apb_ssi_SSIENR(leoDriver, 0);
  dw_apb_ssi_SER(leoDriver, 0);
  dw_apb_ssi_CTRLR1(leoDriver, 0);
  dw_apb_ssi_CTRLR0(leoDriver, 0x070100);
  dw_apb_ssi_SSIENR(leoDriver, 1);

  DRx_val = 0x42;
  dw_apb_ssi_DRx(leoDriver, DRx_val);
  for (ii = 0; ii < 18; ii++) {
    DRx_val = 0;
    if (1 == block_protect_en) {
      DRx_val = block_protect_en_buffer[ii];
    }
    dw_apb_ssi_DRx(leoDriver, DRx_val);
  }
  dw_apb_ssi_SER(leoDriver, 1);
  uint32_t txflr = 1;
  while (0 != txflr) {
    dw_apb_ssi_TXFLR(leoDriver, &txflr);
  }
}

void flash_check_write_in_prog(LeoI2CDriverType *leoDriver, uint32_t *value) {
  uint32_t flash_sts_reg;
  flash_read_sts_reg(leoDriver, &flash_sts_reg);
  *value = flash_sts_reg & 0x1;
}

void flash_write_enable(LeoI2CDriverType *leoDriver) {
  dw_apb_ssi_SER(leoDriver, 0);
  dw_apb_ssi_SSIENR(leoDriver, 0);
  dw_apb_ssi_CTRLR0(leoDriver, 0x070100);
  dw_apb_ssi_SSIENR(leoDriver, 1);
  uint32_t DRx_val = 0x06;
  dw_apb_ssi_DRx(leoDriver, DRx_val);
  dw_apb_ssi_SER(leoDriver, 1);
  uint32_t txflr = 1;
  while (txflr != 0) {
    dw_apb_ssi_TXFLR(leoDriver, &txflr);
  }
}

void flash_subsector_erase(LeoI2CDriverType *leoDriver, uint32_t addr, int spiDevice, int protect) {
  leo_spi_flash_write_block_protect(leoDriver, spiDevice, 0);

  flash_write_enable(leoDriver);
  uint32_t subsector_addr = addr & (~(FLASH_SUBSECTOR_SIZE - 1));
  dw_apb_ssi_SSIENR(leoDriver, 0);
  dw_apb_ssi_SER(leoDriver, 0);
  dw_apb_ssi_CTRLR1(leoDriver, 0);
  dw_apb_ssi_CTRLR0(leoDriver, 0x1F0100);
  dw_apb_ssi_SSIENR(leoDriver, 1);

  size_t i;
  uint32_t DRx_val;
  dw_apb_ssi_SER(leoDriver, 0);
  DRx_val = 0x20;
  DRx_val = (DRx_val << 24) | (subsector_addr);
  dw_apb_ssi_DRx(leoDriver, DRx_val);
  dw_apb_ssi_SER(leoDriver, 1);
  uint32_t txflr = 1;
  while (txflr != 0) {
    dw_apb_ssi_TXFLR(leoDriver, &txflr);
  }
  uint32_t fwip = 1;
  while (fwip != 0) {
    flash_check_write_in_prog(leoDriver, &fwip);
  }
  if (0 != protect) {
    leo_spi_flash_write_block_protect(leoDriver, spiDevice, 1);
  }
}

void flash_block_erase(LeoI2CDriverType *leoDriver, uint32_t addr, int spiDevice, int protect) {
  leo_spi_flash_write_block_protect(leoDriver, spiDevice, 0);

  flash_write_enable(leoDriver);
  dw_apb_ssi_SSIENR(leoDriver, 0);
  dw_apb_ssi_SER(leoDriver, 0);
  dw_apb_ssi_CTRLR1(leoDriver, 0);
  dw_apb_ssi_CTRLR0(leoDriver, 0x1F0100);
  dw_apb_ssi_SSIENR(leoDriver, 1);

  size_t i;
  uint32_t DRx_val;
  dw_apb_ssi_SER(leoDriver, 0);
  // BLOCK ERASE
  DRx_val = 0xD8;
  DRx_val = (DRx_val << 24) | (addr);
  dw_apb_ssi_DRx(leoDriver, DRx_val);
  dw_apb_ssi_SER(leoDriver, 1);
  uint32_t txflr = 1;
  while (txflr != 0) {
    dw_apb_ssi_TXFLR(leoDriver, &txflr);
  }
  uint32_t fwip = 1;
  while (fwip != 0) {
    flash_check_write_in_prog(leoDriver, &fwip);
  }

  if (0 != protect) {
    leo_spi_flash_write_block_protect(leoDriver, spiDevice, 1);
  }
}

LeoErrorType flash_write(LeoI2CDriverType *leoDriver, uint32_t start_addr,
                       size_t num_words_to_write, uint32_t *word_arr) {
  LeoErrorType rc = LEO_SUCCESS;
  uint32_t addr = start_addr;
  uint32_t DRx_val;
  size_t num_words_written = 0;
  size_t i;
  size_t curr_fifo_write_size;
  uint32_t curr_flash_page_end_addr;
  uint32_t num_words_to_page_boundary;
  uint32_t end_addr = start_addr + num_words_to_write * 4;
  int print = (num_words_to_write) > 0x1000;

  uint32_t burst_len = DW_APB_SSI_TX_FIFO_SIZE / 2;

  while (num_words_written < num_words_to_write) {
    flash_write_enable(leoDriver);
    dw_apb_ssi_SSIENR(leoDriver, 0);
    dw_apb_ssi_SER(leoDriver, 0);
    dw_apb_ssi_CTRLR1(leoDriver, 0);
    dw_apb_ssi_CTRLR0(leoDriver, 0x1F0100);
    dw_apb_ssi_SSIENR(leoDriver, 1);

    /* dw_apb_ssi_SER(leoDriver, 0); */

    curr_flash_page_end_addr =
        ((addr + FLASH_PAGE_SIZE) >> FLASH_PAGE_SIZE_SHIFT)
        << FLASH_PAGE_SIZE_SHIFT;
    num_words_to_page_boundary = (curr_flash_page_end_addr - addr) >> 2;
    // changed from python code, which uses 31 insatead of 32
    curr_fifo_write_size =
        MIN((num_words_to_write - num_words_written), burst_len);
    curr_fifo_write_size =
        MIN(num_words_to_page_boundary, curr_fifo_write_size);

    /* ASTERA_INFO("flash_write:\n\ */
    /*        curr_fifo_write_size = %zd\n\ */
    /*        curr_flash_page_end_addr = %06x\n\ */
    /*        num_words_to_page_boundary = %06x\n\ */
    /*        addr = %06x\n", */
    /*        curr_fifo_write_size, curr_flash_page_end_addr,
     * num_words_to_page_boundary, addr); */
    DRx_val = 0x02;
    DRx_val = (DRx_val << 24) | (addr);
    dw_apb_ssi_DRx(leoDriver, DRx_val);
    /* ASTERA_INFO("flash_write - Page program command: %x\n", DRx_val);
     */

    for (i = 0; i < curr_fifo_write_size; i++) {
      DRx_val = word_arr[num_words_written + i];
      dw_apb_ssi_DRx(leoDriver, DRx_val);
      addr = addr + 4;
    }
    dw_apb_ssi_SER(leoDriver, 1);

    if (print && 0 == (addr & 0x1FF)) {
      printf("\rWriting %06x of %06x", addr, end_addr);
      fflush(stdout);
    }

    uint32_t txflr = 1;
    while (txflr != 0) {
      dw_apb_ssi_TXFLR(leoDriver, &txflr);
    }
    uint32_t fwip = 1;
    while (fwip != 0) {
      flash_check_write_in_prog(leoDriver, &fwip);
    }
    num_words_written = num_words_written + curr_fifo_write_size;
    /* ASTERA_INFO("flash_write: curr_prog_status:%zx of %zx\n",
     * (num_words_written << 2), (num_words_to_write << 2)); */
  }
  if (print) {
    printf("\n");
  }
  return rc;
}

LeoErrorType flash_read(LeoI2CDriverType *leoDriver, uint32_t start_addr,
                      size_t num_words_to_read, uint32_t *values) {
  LeoErrorType rc = 0;
  uint32_t addr = start_addr;
  uint32_t num_words_read = 0;
  uint32_t word_arr[num_words_to_read];
  uint32_t DRx_val;
  int curr_fifo_read_size;
  int i;
  int j;

  while (num_words_read < num_words_to_read) {
    dw_apb_ssi_SSIENR(leoDriver, 0);
    dw_apb_ssi_SER(leoDriver, 0);
    dw_apb_ssi_CTRLR1(leoDriver, 0x1F);
    dw_apb_ssi_CTRLR0(leoDriver, 0x1F0300);
    dw_apb_ssi_SSIENR(leoDriver, 1);

    /* dw_apb_ssi_SER(leoDriver, 0); */
    DRx_val = 0x03;
    DRx_val = (DRx_val << 24) | (addr);
    dw_apb_ssi_DRx(leoDriver, DRx_val);
    dw_apb_ssi_SER(leoDriver, 1);

    uint32_t DRx_buf[16];

    uint32_t txflr = 1;
    while (txflr != 0) {
      dw_apb_ssi_TXFLR(leoDriver, &txflr);
    }

    curr_fifo_read_size =
        MIN((num_words_to_read - num_words_read), DW_APB_SSI_RX_FIFO_SIZE);

    uint32_t rxflr = 0;
    while (rxflr < curr_fifo_read_size) {
      dw_apb_ssi_RXFLR(leoDriver, &rxflr);
    }

    for (i = 0; i < curr_fifo_read_size; i++) {
#if FW_ASSIST
      /* ASTERA_INFO("FW_ASSIST: read[%06x]: curr_fifo_read_size: (%d/%d)\n", */
      /*   addr, i,  curr_fifo_read_size); */
      if (curr_fifo_read_size - i >= 16) {
        dw_apb_ssi_DRx_read_16_dw(leoDriver, DRx_buf);
        for (j = 0; j < 16; j++) {
          word_arr[i + j + num_words_read] = DRx_buf[j];
        }
        i += 15;
        addr = addr + (4 * 16);
        continue;
      }
#endif
      dw_apb_ssi_DRx_read(leoDriver, &DRx_val);
      word_arr[i + num_words_read] = DRx_val;
      addr = addr + 4;
    }
    num_words_read = num_words_read + curr_fifo_read_size;
    /* ASTERA_INFO("flash_read: curr_read_status:%x of %zx\n",
     * num_words_read<<2, num_words_to_read<<2); */
  }
#ifdef LEO_CSDK_DEBUG
  ASTERA_DEBUG("flash_read: data received");
#endif
  for (i = 0; i < (num_words_read); i++) {
#ifdef LEO_CSDK_DEBUG
    if (!(i % 8))
      ASTERA_DEBUG("\n[%06x]: ", (i << 2) + start_addr);
    ASTERA_DEBUG("%08x ", word_arr[i]);
#endif

    values[i] = word_arr[i];
  }
  return rc;
}

void leo_spi_flash_bulk_erase(LeoI2CDriverType *leoDriver) {
  flash_write_enable(leoDriver);
  dw_apb_ssi_SSIENR(leoDriver, 0);
  dw_apb_ssi_SER(leoDriver, 0);
  dw_apb_ssi_CTRLR1(leoDriver, 0x0);
  dw_apb_ssi_CTRLR0(leoDriver, 0x070100);
  dw_apb_ssi_SSIENR(leoDriver, 1);
  uint32_t DRx_val = 0xC7;
  dw_apb_ssi_DRx(leoDriver, DRx_val);
  dw_apb_ssi_SER(leoDriver, 1);
  uint32_t txflr = 1;
  while (txflr != 0) {
    dw_apb_ssi_TXFLR(leoDriver, &txflr);
  }
  uint32_t fwip = 1;
  while (fwip != 0) {
    flash_check_write_in_prog(leoDriver, &fwip);
  }
}

void flash_read_jedec(LeoI2CDriverType *leoDriver, uint32_t *value) {
  dw_apb_ssi_SER(leoDriver, 0);
  dw_apb_ssi_SSIENR(leoDriver, 0);
  dw_apb_ssi_CTRLR0(leoDriver, 0x070300);
  dw_apb_ssi_CTRLR1(leoDriver, 0x2); // read 3 data frames

  dw_apb_ssi_SSIENR(leoDriver, 1);
  uint32_t DRx_val = 0x9f;
  dw_apb_ssi_DRx(leoDriver, DRx_val);
  dw_apb_ssi_SER(leoDriver, 1);
  uint32_t rxflr = 0;
  while (rxflr == 0) {
    dw_apb_ssi_RXFLR(leoDriver, &rxflr);
  }
  uint32_t DRx_buf[16];
  dw_apb_ssi_DRx_read_16_dw(leoDriver, DRx_buf);
  *value = (DRx_buf[0] & 0xff) << 16 | (DRx_buf[1] & 0xff) << 8 |
           (DRx_buf[2] & 0xff);
}

void read_fw_version(LeoI2CDriverType *leoDriver, uint32_t *values) {
  leoReadWordData(leoDriver, (0x80180), &values[0]);

  leoReadWordData(leoDriver, (0x80184), &values[1]);

  leoReadWordData(leoDriver, (0x80188), &values[2]);
}

LeoErrorType flash_verify_block_crc(LeoI2CDriverType *leoDriver, uint32_t startAddr,
                           size_t lenDWords, uint32_t crc) {
  MailboxStatusType mb_sts;
  uint32_t bufferOut[16];
  uint32_t bufferIn[16];

  bufferOut[0] = startAddr;
  bufferOut[1] = lenDWords;
  bufferOut[2] = crc;
  mb_sts = execOperation(leoDriver, 0, FW_API_MMB_CMD_OPCODE_MMB_FW_CRC_VERIFY,
                bufferOut, 3, bufferIn, 4);
  if (mb_sts != AL_MM_STS_SUCCESS) {
    ASTERA_WARN("Flash CRC verification failure: unable to communicate with mailbox");
    return LEO_FAILURE;
  }

  ASTERA_INFO("Verifying block at %06x length %06x crc = %08x expected %08x",
               startAddr, lenDWords >> 2, bufferIn[1], crc);
  if ((0 != bufferIn[0]) || (bufferIn[1] != crc) || (bufferIn[2] != 0) ||  (0x5050a0a0 != bufferIn[3])) {
    ASTERA_ERROR("Flash CRC verification failure: %08x %08x %08x %08x",
                 bufferIn[0], bufferIn[1], bufferIn[2], bufferIn[3]);
    return LEO_FAILURE;
  }
  return LEO_SUCCESS;
}

static uint32_t leo_flash_mem_min = 0xfffffff;
static uint32_t leo_flash_mem_max = 0x0000000;
static unsigned char leo_flash_fw_buffer[SPI_FLASH_SIZE]; // 8MB
static int flash_mem_done_reading = 0;

static int buf_eq(const uint8_t a[], const uint8_t b[], size_t n) {
  size_t i;
  for (i = 0; i < n; i++) {
    if (a[i] != b[i])
      return 0;
  }
  return 1;
}

static int decode_mem_line(char *line) {
  char *token, *str1, *saveptr;
  int jj;
  unsigned int address;
  unsigned int value = 0;
  for (jj = 0, str1 = line;; jj++, str1 = NULL) {
    token = strtok_r(str1, " ", &saveptr);
    if (token == NULL)
      break;
    if (0 == jj) {
      address = strtoul((token + 1), NULL, 16);
      if (address < leo_flash_mem_min)
        leo_flash_mem_min = address;
    } else {
      value = strtoul(token, NULL, 16);
      leo_flash_fw_buffer[address] = (unsigned char)value;
      address++;
    }
  } // for (jj=0, str1 = line; ; jj++, str1 = NULL)
  if (address > leo_flash_mem_max)
    leo_flash_mem_max = address;
  return (0);
} // int decode_mem_line()

static LeoErrorType readFWImageFromFile(const char *filename) {
  int rc;
  FILE *fp;
  int num_errors = 0;
  char line[128];
  int count;
  if (0 != flash_mem_done_reading) {
    return 0;
  }

  fp = fopen(filename, "r");
  if (NULL == fp) {
    ASTERA_ERROR("Couldn't open file %s", filename);
    return (1);
  }
  memset(leo_flash_fw_buffer, 0xff, (8 * 1024 * 1024));
  while (1) {
    if (feof(fp))
      break;
    rc = fscanf(fp, "%[^\n]\n", line);
    rc = decode_mem_line(line);
    if (0 != rc) {
      ASTERA_ERROR("decode_line failed");
      num_errors++;
      break;
    }
    count++;
  } // while (1)
  fclose(fp);
  if (0 != num_errors) {
    ASTERA_WARN("WARNING(ERROR): decode_line failed");
    return (1);
  }
  ASTERA_INFO("**INFO : Counted %d lines, address ranges from  %08x to %08x",
              count, leo_flash_mem_min, leo_flash_mem_max);
  flash_mem_done_reading = 1;
  return rc;
}

static int leo_verify_flash_crc(LeoI2CDriverType *leoDriver,
                                const char *filename) {
  readFWImageFromFile(filename);

  const uint8_t startPat[] = {0x5a, 0xa5, 0x5a, 0xa5, 0x5a, 0xa5, 0x5a, 0xa5};
  const uint8_t endPat[] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
  size_t imageLen =
      sizeof(leo_flash_fw_buffer) / sizeof(leo_flash_fw_buffer[0]);
  int foundStart = 0;
  uint32_t lenDWords = 0;
  uint32_t startAddr = 0;
  uint8_t blockType;
  uint32_t crc;

  size_t i;
  for (i = 0; i < imageLen - 8; i++) {
    if (!foundStart) {
      foundStart = buf_eq(startPat, (uint8_t *)&leo_flash_fw_buffer[i], 8);
      if (foundStart) {
        blockType = leo_flash_fw_buffer[i + 11];
        if (blockType == 0x6) {
          foundStart = 0;
          continue;
        }
        startAddr = i;
        i += 7;
      }
      continue;
    }
    if (!buf_eq(endPat, (uint8_t *)&leo_flash_fw_buffer[i], 8)) {
      // TODO: Can probably increment i by 3 extra bytes
      continue;
    }
    crc = leo_flash_fw_buffer[i - 4] << 24 | leo_flash_fw_buffer[i - 3] << 16 |
          leo_flash_fw_buffer[i - 2] << 8 | leo_flash_fw_buffer[i - 1];

    lenDWords = (i + 8 - startAddr) / 4;
    if (LEO_SUCCESS != flash_verify_block_crc(leoDriver, startAddr, lenDWords, crc)) {
      ASTERA_ERROR("CRC Verify failed at addr[%06x]", startAddr);
      return 1;
    }
    ASTERA_INFO("CRC Verify succeeded at addr[%06x]", startAddr);
    foundStart = 0;
  }
  return 0;
}

static int leo_verify_flash(LeoI2CDriverType *leoDriver) {
#define ONE_KB (1024)
#define MAX_ERRORS 10
  uint32_t *read_buffer;
  uint32_t addr = leo_flash_mem_min;
  uint32_t length = ONE_KB;
  uint32_t num_errors = 0;
  uint32_t i;
  int rc;

  ASTERA_DEBUG("Reading back FW image from flash, to %x", leo_flash_mem_max);
  read_buffer = (uint32_t *)malloc(8 * 1024 * 1024);

  if (NULL == read_buffer) {
    ASTERA_ERROR("%s %s %d: malloc failed", __FILE__, __FUNCTION__, __LINE__);
    return (1);
  }

  for (addr = leo_flash_mem_min; addr < leo_flash_mem_max;
       addr = addr + ONE_KB) {
    printf("reading flash at [%06x] \r", addr);
    fflush(stdout);
    flash_read(leoDriver, addr, length >> 2, &read_buffer[addr >> 2]);
  }
  printf("\n");

  ASTERA_DEBUG("Verifying content (%06x-%06x)", leo_flash_mem_min,
               leo_flash_mem_max);
  uint32_t fw_buf_dword;
  uint32_t rd_buf_dword;
  uint32_t rx_idx = leo_flash_mem_min >> 2;
  for (i = leo_flash_mem_min; i < leo_flash_mem_max; i = i + 4) {
    fw_buf_dword = ((leo_flash_fw_buffer[i] & 0xff) << 24) |
                   ((leo_flash_fw_buffer[i + 1] & 0xff) << 16) |
                   ((leo_flash_fw_buffer[i + 2] & 0xff) << 8) |
                   ((leo_flash_fw_buffer[i + 3] & 0xff));
    rd_buf_dword = read_buffer[rx_idx];
    if (fw_buf_dword != rd_buf_dword) {
      ASTERA_ERROR("Mismatch at %06x.  Read %08x, expected %08x", i,
                   rd_buf_dword, fw_buf_dword);
      num_errors++;
    }
    if (num_errors > MAX_ERRORS) {
      ASTERA_ERROR("Stopping compare after %d errors", MAX_ERRORS);
      break;
    }
    rx_idx++;
  }
  free(read_buffer);
  return num_errors;
}

LeoErrorType leo_spi_program_flash(LeoDeviceType *leoDevice,
                                   const char *filename) {
  LeoI2CDriverType *leoDriver = leoDevice->i2cDriver;
  int rc;
  FILE *fp;
  uint32_t count = 0;
  uint32_t i;
  uint32_t j;
  uint32_t addr;
  uint32_t tx_idx = 0;
  uint32_t *read_buffer;
  uint32_t *write_buffer;
  uint32_t dt;
  uint32_t num_errors = 0;
  char line[132];
  char now_string[32];
  block_info_t persistent_data_block_info_flash;
  block_info_t persistent_data_block_info_mem;
  uint32_t *persistent_data_block_buf;
  struct timeval tv_start;
  struct timeval tv_mark;
  struct timeval tv_now;

  gettimeofday(&tv_start, NULL);
  strcpy(now_string, ctime(&(tv_start.tv_sec)));
  now_string[24] = '\0';
  ASTERA_INFO("Reading FW flash image file %s (%s)", filename, now_string);

  rc = readFWImageFromFile(filename);
  if (rc != 0) {
    printf("Failed to read FW image from file %s", filename);
    return rc;
  }

  rc = leoSpiCheckCompatibility(leoDevice, leo_flash_fw_buffer);
  if (0 != rc) {
    return rc;
  }

  if (0 == leoDevice->ignorePersistentDataFlag) {
    // Start looking for persistent data at 0x20000 to speed up search in older versions (<0.8)
    rc += get_block_info(leoDevice->i2cDriver, 0x20000, &persistent_data_block_info_flash, NULL);
    while (BT_PERSISTENT_DATA_e != persistent_data_block_info_flash.type) {
      rc += find_next_block(leoDevice->i2cDriver, persistent_data_block_info_flash.start_addr, 1, &persistent_data_block_info_flash.start_addr, NULL);
      if (rc != 0) {
        ASTERA_ERROR("Failed to find persistent data block, please perform a clean update");
        return rc;
      }
      rc += get_block_info(leoDevice->i2cDriver, persistent_data_block_info_flash.start_addr, &persistent_data_block_info_flash, NULL);
    }
    rc = find_block_by_type(leoDevice->i2cDriver, BT_PERSISTENT_DATA_e, &persistent_data_block_info_mem, leo_flash_fw_buffer);
    persistent_data_block_buf = (uint32_t *)malloc(persistent_data_block_info_flash.length);

    ASTERA_INFO("Reading persistent data block from flash");
    rc += read_block_data(leoDevice->i2cDriver, persistent_data_block_info_flash, persistent_data_block_buf, NULL);
    if (rc != 0) {
      ASTERA_ERROR("Failed to read persistent data block from flash");
      free(persistent_data_block_buf);
      return rc;
    }

    for (i = 0; i < persistent_data_block_info_flash.length; i+=4) {
      addr = persistent_data_block_info_mem.start_addr + LEO_SPI_FLASH_HEADER_BYTE_CNT + i;
      leo_flash_fw_buffer[addr + 0] = (persistent_data_block_buf[i >> 2] >> 24) & 0xff;
      leo_flash_fw_buffer[addr + 1] = (persistent_data_block_buf[i >> 2] >> 16) & 0xff;
      leo_flash_fw_buffer[addr + 2] = (persistent_data_block_buf[i >> 2] >>  8) & 0xff;
      leo_flash_fw_buffer[addr + 3] = (persistent_data_block_buf[i >> 2] >>  0) & 0xff;
    }
    free(persistent_data_block_buf);
  }

  // Disable write block protect
  leo_spi_flash_write_block_protect(leoDevice->i2cDriver, leoDevice->spiDevice, 0);

  // Erase the flash
  leo_spi_flash_bulk_erase(leoDevice->i2cDriver);

  ASTERA_INFO("Writing FW image to SPI flash");

  // Program only the blocks, nothing in between
  block_info_t curr_block_info_mem;
  rc = find_next_block(leoDevice->i2cDriver, 0, 0, &addr, leo_flash_fw_buffer);
  if (rc != 0) {
    ASTERA_ERROR("Failed to find first block");
    return rc;
  }

  while (rc == 0) {
    rc += get_block_info(leoDevice->i2cDriver, addr, &curr_block_info_mem, leo_flash_fw_buffer);
    if (rc != 0) {
      ASTERA_ERROR("Failed to get block info for block at 0x%06x", addr);
      break;
    }

    write_buffer = (uint32_t *)malloc(curr_block_info_mem.end_addr - curr_block_info_mem.start_addr);
    tx_idx = 0;
    for (i = curr_block_info_mem.start_addr; i < curr_block_info_mem.end_addr; i+=4) {
      write_buffer[tx_idx] = (leo_flash_fw_buffer[i + 0] << 24) |
                             (leo_flash_fw_buffer[i + 1] << 16) |
                             (leo_flash_fw_buffer[i + 2] <<  8) |
                             (leo_flash_fw_buffer[i + 3] <<  0);
      tx_idx++;
    }

    ASTERA_INFO("Writing block at 0x%06x", addr);
    rc += flash_write(leoDevice->i2cDriver, curr_block_info_mem.start_addr, (curr_block_info_mem.end_addr - curr_block_info_mem.start_addr) >> 2, write_buffer);
    free(write_buffer);
    if (rc != 0) {
      ASTERA_ERROR("Failed to write block at 0x%06x", addr);
      break;
    }

    if (BT_END_e == curr_block_info_mem.type) {
      break;
    }
    rc += find_next_block(leoDevice->i2cDriver, addr, 1, &addr, leo_flash_fw_buffer);
  }

  // Enable write block protect
  leo_spi_flash_write_block_protect(leoDevice->i2cDriver, leoDevice->spiDevice, 1);

  gettimeofday(&tv_now, NULL);
  dt = tv_now.tv_sec - tv_start.tv_sec;
  ASTERA_INFO("Elapsed time: %d seconds", dt);
  strcpy(now_string, ctime(&(tv_now.tv_sec)));
  now_string[24] = '\0';
  ASTERA_INFO("SPI image update done %s", now_string);

  // Verify image
  num_errors += leo_verify_flash_crc(leoDriver, filename);

  gettimeofday(&tv_now, NULL);
  dt = tv_now.tv_sec - tv_start.tv_sec;
  ASTERA_INFO("Total Elapsed time: %d seconds", dt);
  strcpy(now_string, ctime(&(tv_now.tv_sec)));
  now_string[24] = '\0';
  ASTERA_INFO("SPI image verify done %s", now_string);
  return (rc);
}

LeoErrorType flash_read_32(
  LeoI2CDriverType *leoDriver,
  uint32_t       addr,
  uint32_t       *data
  ) {
  LeoErrorType rc;
  uint32_t read_buf[16] = {0};
  rc = flash_read(leoDriver, addr, 1, read_buf);
  *data = read_buf[0];
  /* printf("flash_read_32: addr %06x data %08x\n", addr, *data); */
  return rc;
}

/*********************************************************************
 ********************************************************************/
LeoErrorType find_block_end(
    LeoI2CDriverType *leoDriver,
    uint32_t       block_start_addr,
    uint32_t       *block_end_addr,
    uint8_t        *mem_data
    ) { 
    uint32_t footer_pat = 0xaa55aa55;
    uint32_t block_size;
    uint32_t addr;
    uint32_t timeout = 0x200;
    uint32_t read_buf[2] = {0};
    uint32_t tmp32;
    LeoErrorType rc;

    rc = get_block_size(leoDriver, block_start_addr, &block_size, mem_data);
    if (0 != rc) {
        printf("find_block_end: Error getting block size for block at %06x\n", block_start_addr);
        return 1;
    }
    addr = block_start_addr + block_size;

    /* printf("find_block_end: addr = %06x block_len = %08x\n", block_start_addr, block_size); */
    while (timeout > 0) {
        if (NULL == mem_data) {
            rc += flash_read_32(leoDriver, addr, &tmp32);
            if (0 != rc) {
                printf("find_block_end: Error reading from SPI flash %06x\n", addr);
                return 1;
            }
        } else {
            tmp32 = (mem_data[addr + 0] << 24) |
                    (mem_data[addr + 1] << 16) |
                    (mem_data[addr + 2] <<  8) |
                    (mem_data[addr + 3]      );
        }

        read_buf[0] = read_buf[1];
        read_buf[1] = tmp32;

        if (footer_pat == read_buf[0] && footer_pat == read_buf[1]) {
            *block_end_addr = addr + 4;
            return rc;
        }

        addr += 4;
        timeout--;
    }
    return 1; 
} // LeoErrorType find_block_end()

/*********************************************************************
 ********************************************************************/
LeoErrorType find_next_block(
    LeoI2CDriverType *leoDriver,
    uint32_t      block_start_addr,
    uint32_t      skip,
    uint32_t      *next_block_addr,
    uint8_t       *mem_data
    ) { 
    const uint32_t header_pat = 0x5aa55aa5;
    /* int ii; */
    LeoErrorType rc = 0;
    uint32_t tmp32;
    uint32_t read_buf[2] = {0};
    uint32_t addr = block_start_addr;

    /* printf("find_next_block: block_start_addr = %06x\n", block_start_addr); */

    while (1) {
        if (NULL == mem_data) {
            rc = flash_read_32(leoDriver, addr, &tmp32);
            if (0 != rc) {
                printf("Error reading from SPI flash\n");
                return rc;
            }
        } else {
            tmp32 = (mem_data[addr + 0] << 24) |
                    (mem_data[addr + 1] << 16) |
                    (mem_data[addr + 2] <<  8) |
                    (mem_data[addr + 3]      );
        }

        read_buf[0] = read_buf[1];
        read_buf[1] = tmp32;
        /* printf("find_next_block: addr = %06x read_buf[0] = %08x, read_buf[1] = %08x\n", addr, read_buf[0], read_buf[1]); */

        if (header_pat == read_buf[0] && header_pat == read_buf[1]) {
            if (0 < skip) {
                skip--;
                rc = find_block_end(leoDriver, addr - 4, &addr, mem_data);
                if (0 != rc) {
                    return rc;
                }
                /* printf("find_next_block: skipping to %06x\n", addr); */
                read_buf[0] = 0;
                read_buf[1] = 0;
                continue;
            } else {
                *next_block_addr = addr - 4;
                return rc;
            }
        }
        addr += 4;
    }
    return 1; 
} // LeoErrorType find_next_block()

/*********************************************************************
 ********************************************************************/
LeoErrorType get_block_size(
    LeoI2CDriverType *leoDriver,
    uint32_t addr,
    uint32_t *block_size,
    uint8_t  *mem_data
    ) {
    LeoErrorType rc = 0;
    uint32_t read_buf[2];
    uint32_t size;
    uint32_t size_cpy;

    if (NULL == mem_data) {
        rc += flash_read_32(leoDriver, addr, &read_buf[0]);
        rc += flash_read_32(leoDriver, addr + 4, &read_buf[1]);
        if (0 != rc || 0x5aa55aa5 != read_buf[0] || 0x5aa55aa5 != read_buf[1]) {
            return 1;
        }
    
        rc += flash_read_32(leoDriver, addr + 16, &size);
        rc += flash_read_32(leoDriver, addr + 20, &size_cpy);
        if (0 != rc) {
            printf("get_block_size: error reading from SPI flash rc = %d addr = %06x\n", rc, addr+20);
            return 1;
        }
    } else {
        size     = (mem_data[addr + 16] << 24) |
                   (mem_data[addr + 17] << 16) |
                   (mem_data[addr + 18] <<  8) |
                   (mem_data[addr + 19]      );

        size_cpy = (mem_data[addr + 20] << 24) |
                   (mem_data[addr + 21] << 16) |
                   (mem_data[addr + 22] <<  8) |
                   (mem_data[addr + 23]      );
    }

    if (size != size_cpy) {
        printf("get_block_size: size != size_cpy\n");
        return 1;
    }

    /* printf("get_block_size [%06x]: %08x\n", addr, size); */
    *block_size = size;
    return rc;
} // LeoErrorType get_block_size()

/*********************************************************************
 ********************************************************************/
LeoErrorType find_block_by_type(
    LeoI2CDriverType *leoDriver,
    BLOCKTYPE    block_type,
    block_info_t *block_info,
    uint8_t           *mem_data
    ) {
    LeoErrorType rc = 0;
    uint32_t block_start_addr = 0;

    /* printf("find_block_by_type: block_type = %x\n", block_type); */
    rc = find_next_block(leoDriver, 0, 0, &block_start_addr, mem_data);
    if (0 != rc) {
        printf("find_block_by_type: Error finding block at 000000\n");
        return 1;
    }
    while (0 == rc) {
        rc += get_block_info(leoDriver, block_start_addr, block_info, mem_data);
        if (0 != rc) {
            printf("find_block_by_type: Error getting block info\n");
            break;
        }
        if (block_type == block_info->type) {
            return 0;
        }
        rc = find_next_block(leoDriver, block_start_addr, 1, &block_start_addr, mem_data);
        if (0 != rc) {
            printf("find_block_by_type: Error finding block after %06x\n", block_start_addr);
            break;
        }
    }
    return 1;
} // LeoErrorType find_next_block_by_type()

/*********************************************************************
 ********************************************************************/
LeoErrorType get_block_info(
    LeoI2CDriverType *leoDriver,
    uint32_t block_start_addr,
    block_info_t *block_info,
    uint8_t  *mem_data
    ) {
    LeoErrorType rc;
    int ii;
    uint32_t block_end_addr;
    uint32_t tmp32;
    uint32_t buf[12];

    if (NULL == block_info) {
        printf("get_block_info: block_info is NULL\n");
        return 1;
    }

    /* printf("get_block_info: block_start_addr = %06x\n", block_start_addr); */

    rc = find_block_end(leoDriver, block_start_addr, &block_end_addr, mem_data);
    if (0 != rc) {
        /* printf("get_block_info: error finding block end at %06x\n", block_start_addr); */
        return rc;
    }

    if (NULL == mem_data) {
        rc += flash_read_32(leoDriver, block_start_addr + (0 * 4), &block_info->header[0]);
        rc += flash_read_32(leoDriver, block_start_addr + (1 * 4), &block_info->header[1]);
        rc += flash_read_32(leoDriver, block_start_addr + (2 * 4), &tmp32);
        block_info->type = tmp32 & 0xff;
        block_info->subtype = tmp32 >> 16;
        rc += flash_read_32(leoDriver, block_start_addr + (3 * 4), &block_info->version);
        rc += flash_read_32(leoDriver, block_start_addr + (4 * 4), &block_info->length);
        rc += flash_read_32(leoDriver, block_start_addr + (5 * 4), &block_info->length_copy);
        rc += flash_read_32(leoDriver, block_start_addr + (6 * 4), &block_info->address);
        rc += flash_read_32(leoDriver, block_start_addr + (7 * 4), &block_info->config_data[0]);
        rc += flash_read_32(leoDriver, block_start_addr + (8 * 4), &block_info->config_data[1]);

        rc += flash_read_32(leoDriver, block_end_addr - (3 * 4), &block_info->crc);
        rc += flash_read_32(leoDriver, block_end_addr - (2 * 4), &block_info->trailer[0]);
        rc += flash_read_32(leoDriver, block_end_addr - (1 * 4), &block_info->trailer[1]);

        if (0 != rc) {
            printf("get_block_info: error reading from SPI flash rc = %d\n", rc);
            return 1;
        }
    } else {
        for (ii = 0; ii < 36; ii+=4) {
            buf[ii/4] = mem_data[block_start_addr + ii + 0] << 24 | mem_data[block_start_addr + ii + 1] << 16 | mem_data[block_start_addr + ii + 2] << 8 | mem_data[block_start_addr + ii + 3];
        }
        buf[9] = mem_data[block_end_addr - 12 + 0] << 24 | mem_data[block_end_addr - 12 + 1] << 16 | mem_data[block_end_addr - 12 + 2] << 8 | mem_data[block_end_addr - 12 + 3];
        buf[10] = mem_data[block_end_addr - 8 + 0] << 24 | mem_data[block_end_addr - 8 + 1] << 16 | mem_data[block_end_addr - 8 + 2] << 8 | mem_data[block_end_addr - 8 + 3];
        buf[11] = mem_data[block_end_addr - 4 + 0] << 24 | mem_data[block_end_addr - 4 + 1] << 16 | mem_data[block_end_addr - 4 + 2] << 8 | mem_data[block_end_addr - 4 + 3];
        block_info->header[0]      = buf[0];
        block_info->header[1]      = buf[1];
        block_info->type           = buf[2] & 0xff;
        block_info->subtype        = buf[2] >> 16;
        block_info->version        = buf[3];
        block_info->length         = buf[4];
        block_info->length_copy    = buf[5];
        block_info->address        = buf[6];
        block_info->config_data[0] = buf[7];
        block_info->config_data[1] = buf[8];
        block_info->crc            = buf[9];
        block_info->trailer[0]     = buf[10];
        block_info->trailer[1]     = buf[11];
    }
    if (block_info->length != block_info->length_copy) {
        printf("get_block_info: length mismatch\n");
        return 1;
    }
    block_info->start_addr = block_start_addr;
    block_info->end_addr = block_end_addr;
    return 0;
} // LeoErrorType get_block_info()

/*********************************************************************
 ********************************************************************/
LeoErrorType read_block_data(
  LeoI2CDriverType *leoDriver,
  block_info_t block_info,
  uint32_t *block_data,
  uint8_t  *mem_data
  ) {
  LeoErrorType rc = 0;
  int ii;

  if (NULL == mem_data) {
    rc = flash_read(leoDriver, block_info.start_addr + LEO_SPI_FLASH_HEADER_BYTE_CNT, block_info.length >> 2, block_data);
  } else {
    for (ii = 0; ii < block_info.length; ii+=4) {
      block_data[ii/4] = mem_data[block_info.start_addr + LEO_SPI_FLASH_HEADER_BYTE_CNT + ii + 0] << 24 |
                          mem_data[block_info.start_addr + LEO_SPI_FLASH_HEADER_BYTE_CNT + ii + 1] << 16 |
                          mem_data[block_info.start_addr + LEO_SPI_FLASH_HEADER_BYTE_CNT + ii + 2] << 8 |
                          mem_data[block_info.start_addr + LEO_SPI_FLASH_HEADER_BYTE_CNT + ii + 3];
    }
  }
  /* printf("read_block_data: start = %06x block_info.length = %08x block_info.end_addr = %08x\n", block_info.start_addr + LEO_SPI_FLASH_HEADER_BYTE_CNT, block_info.length, block_info.end_addr); */

  return rc;
} // LeoErrorType read_block_data()

LeoErrorType flash_erase_range(
    LeoDeviceType *leoDevice,
    uint32_t      mem_min,
    uint32_t      mem_max
    ) {
    LeoErrorType rc;
    uint32_t total_size;
    uint32_t addr;
    total_size = mem_max - mem_min;
    for (addr = mem_min; addr < mem_max; addr = addr + 65536) {
        ASTERA_INFO("Erasing block at %x", addr);
        flash_block_erase(leoDevice->i2cDriver, addr, leoDevice->spiDevice, 0);
    }
    /*
     *  Microchip device has 8K-8K-8K-8K-32K-64Kxn-32K-8K-8K-8K-8K
     *  org, so we need to handle the first and last 64K differently
     *  This won't get an A in algorithms class, but it should work.
     */
    if (SPI_DEVICE_SST26WF064C_e == leoDevice->spiDevice) {
        ASTERA_INFO("Additional checks for Microchip/SST26WF064C top/bottom");
        if (mem_min < 0x10000) {
            if (mem_min < 0x8000) { // <32K, erase 32K @ 32K
                ASTERA_INFO("Erasing 32K block @ 8000");
                flash_block_erase(leoDevice->i2cDriver, 0x008000, leoDevice->spiDevice, 0);
            }
            if (mem_min < 0x6000) { // <24K, erase 8K @ 24K
                ASTERA_INFO("Erasing 8K block @ 6000");
                flash_block_erase(leoDevice->i2cDriver, 0x006000, leoDevice->spiDevice, 0);
            }
            if (mem_min < 0x4000) { // <16K, erase 8K @ 16K
                ASTERA_INFO("Erasing 8K block @ 4000");
                flash_block_erase(leoDevice->i2cDriver, 0x004000, leoDevice->spiDevice, 0);
            }
            if (mem_min < 0x2000) { //  <8K, erase 8K @  8K
                ASTERA_INFO("Erasing 8K block @ 2000");
                flash_block_erase(leoDevice->i2cDriver, 0x002000, leoDevice->spiDevice, 0);
            }
        }

        if (mem_max >= 0x7f0000) {     // 32K @ 7f0000 already handled.
            if (mem_max >= 0x7f8000) { // erase 8K @ 7f8000
                ASTERA_INFO("Erasing 8K block @ 7f8000");
                flash_block_erase(leoDevice->i2cDriver, 0x7f8000, leoDevice->spiDevice, 0);
            }
            if (mem_max >= 0x7fa000) { // erase 8K @ 7fa000
                ASTERA_INFO("Erasing 8K block @ 7fa000");
                flash_block_erase(leoDevice->i2cDriver, 0x7fa000, leoDevice->spiDevice, 0);
            }
            if (mem_max >= 0x7fc000) { // erase 8K @ 7fc000
                ASTERA_INFO("Erasing 8K block @ 7fc000");
                flash_block_erase(leoDevice->i2cDriver, 0x7fc000, leoDevice->spiDevice, 0);
            }
            if (mem_max >= 0x7fe000) { // erase 8K @ 7fe000
                ASTERA_INFO("Erasing 8K block @ 7fe000");
                flash_block_erase(leoDevice->i2cDriver, 0x7fe000, leoDevice->spiDevice, 0);
            }
        }
    }
    return LEO_SUCCESS;
}

LeoErrorType leoSpiCheckCompatibility(LeoDeviceType *device, uint8_t *fwBuf)
{
  uint32_t *descBlockDataMem;
  block_info_t descBlockInfoMem = {0};
  LeoSpiDescriptionBlockDataType *pDescBlockDataMem;
  LeoErrorType rc;
  uint32_t memAsicVersion;
  uint32_t fwAsicVersion;

  if (device->ignoreCompatibilityCheckFlag) {
    return LEO_SUCCESS;
  }

  rc = leoGetAsicVersion(device);
  if (0 != rc) {
    ASTERA_ERROR("Failed to read firmware type");
    return rc;
  }

  fwAsicVersion = device->asicVersion.minor;

  rc = find_block_by_type(device->i2cDriver, BT_DESCRIPTION_e, &descBlockInfoMem,
                             fwBuf);
  if (0 != rc) {
    ASTERA_ERROR("Failed to find description block in .mem");
    return rc;
  }

  descBlockDataMem = (uint32_t *)malloc(descBlockInfoMem.length);
  rc += read_block_data(device->i2cDriver, descBlockInfoMem, descBlockDataMem, 
                            fwBuf);
  pDescBlockDataMem = (LeoSpiDescriptionBlockDataType *)descBlockDataMem;
  if (0 != rc) {
    ASTERA_ERROR("Failed to read description block in .mem");
    free(descBlockDataMem);
    return rc;
  }

  memAsicVersion = (0xa0 == pDescBlockDataMem->entries[0].asic_version) ? LEO_ASIC_A0 : LEO_ASIC_D5;
  free(descBlockDataMem);

  if (fwAsicVersion != memAsicVersion) {
    ASTERA_ERROR(".mem firmware type %s does not match active firmware type %s",
                 (LEO_ASIC_A0 == memAsicVersion) ? "A0":"D5", (LEO_ASIC_A0 == fwAsicVersion) ? "A0":"D5");
    return LEO_FAILURE;
  }

  return LEO_SUCCESS;
}

/*********************************************************************
 ********************************************************************/
LeoErrorType leo_spi_update_target(LeoDeviceType *device, char *filename, int target, int verify) {
    LeoErrorType rc = 0;
    int  count      = 0;
    int  num_errors = 0;
    int  ii;
    char line[132];
    char now_string[32];
    FILE *fp;
    uint32_t dt;
    uint32_t addr;
    uint32_t prev_addr;
    struct timeval tv_start;
    struct timeval tv_mark;
    struct timeval tv_now;

    block_info_t toc_block_info_mem      = {0};
    block_info_t toc_block_info_flash    = {0};
    block_info_t syscfg_block_info_mem   = {0};
    block_info_t syscfg_block_info_flash = {0};
    block_info_t code_block_info_mem     = {0};
    block_info_t code_block_info_flash   = {0};
    block_info_t end_block_info_mem      = {0};
    block_info_t end_block_info_flash    = {0};
    block_info_t tmp_block_info          = {0};
    uint32_t code_write_end_addr;
    uint32_t syscfg_write_end_addr;
    uint32_t code_length_flash;
    uint32_t code_length_mem;
    uint32_t *block_data_mem;
    uint32_t *block_data_flash;
    toc_data_t *toc_data_mem;
    toc_data_t *toc_data_flash;
    uint32_t *fw_flash_buffer_dwords;
    uint32_t check_flash_empty_buffer[2];

    gettimeofday(&tv_start, NULL);
    strcpy(now_string, ctime(&(tv_start.tv_sec)));
    now_string[24] = '\0';

    rc = flash_read(device->i2cDriver, 0, 2, check_flash_empty_buffer);
    if (0 != rc || 0x5aa55aa5 != check_flash_empty_buffer[0] ||
        0x5aa55aa5 != check_flash_empty_buffer[1]) {
      ASTERA_ERROR("Flash is corrupted, performing clean update rc %d [%x, %x]", rc, check_flash_empty_buffer[0], check_flash_empty_buffer[1]);
      device->ignorePersistentDataFlag = 1;
      rc = leo_spi_program_flash(device, filename);
      return rc;
    }

    rc += readFWImageFromFile(filename);
    if (rc != 0) {
      printf("Failed to read FW image from file %s", filename);
      return rc;
    }

    rc = leoSpiCheckCompatibility(device, leo_flash_fw_buffer);
    if (0 != rc) {
      return rc;
    }

    //
    // Find TOC in flash and .mem file
    //
    rc = find_block_by_type(device->i2cDriver, BT_TOC_e, &toc_block_info_mem, leo_flash_fw_buffer);
    if (0 != rc) {
        ASTERA_ERROR("Failed to find TOC block in .mem");
        return rc;
    }
    rc = find_block_by_type(device->i2cDriver, BT_TOC_e, &toc_block_info_flash, NULL);
    if (0 != rc) {
        ASTERA_ERROR("Failed to find TOC block in flash");
        return rc;
    }

    // If updating from <0.8
    if (0 == toc_block_info_flash.config_data[0]) {
        if (1 == toc_block_info_mem.config_data[0]) {
          ASTERA_INFO("Upgrading to 0.8+ from <0.8, this might take a while (up to 5x longer than normal)");
        }
        rc = leo_spi_program_flash(device, filename);
        return rc;
    }
    if (1 != toc_block_info_mem.config_data[0] || 0 != toc_block_info_mem.config_data[1]) {
        ASTERA_ERROR("ERROR: TOC version %x update not supported, use -clean or another method to perform a clean image write.", toc_block_info_mem.config_data[0]);
        return 1;
    }

    if (1 != toc_block_info_mem.version) {
        ASTERA_ERROR("ERROR: TOC block in .mem has wrong version %d", toc_block_info_mem.version);
        return 1;
    }

    //
    // Use TOC to find code and syscfg blocks in flash and .mem file
    //
    block_data_mem = (uint32_t *)malloc(toc_block_info_mem.length);
    rc += read_block_data(device->i2cDriver, toc_block_info_mem, block_data_mem, leo_flash_fw_buffer);
    toc_data_mem = (toc_data_t *) block_data_mem;
    if (0 != rc) {
        ASTERA_ERROR("Failed to read TOC block in .mem");
        free(block_data_mem);
        return rc;
    }

    // to save time for now, only read the amount of toc that is needed
    block_data_flash = (uint32_t *)malloc(toc_block_info_flash.length);
    rc += flash_read(device->i2cDriver, toc_block_info_flash.start_addr + 0x24, sizeof(toc_data_t) / 4, block_data_flash);
    toc_data_flash = (toc_data_t *) block_data_flash;
    if (0 != rc) {
        ASTERA_ERROR("Failed to read TOC block in flash");
        free(block_data_flash);
        free(block_data_mem);
        return rc;
    }

    rc += get_block_info(device->i2cDriver, toc_data_mem->syscfg_data[target].addr_pointer, &syscfg_block_info_mem, leo_flash_fw_buffer);
    rc += get_block_info(device->i2cDriver, toc_data_mem->code_data[target].addr_pointer, &code_block_info_mem, leo_flash_fw_buffer);
    if (0 != rc) {
        ASTERA_ERROR("Failed to get block info for code or syscfg block in .mem");
        free(block_data_flash);
        free(block_data_mem);
        return rc;
    }

    rc += get_block_info(device->i2cDriver, toc_data_flash->syscfg_data[target].addr_pointer, &syscfg_block_info_flash, NULL);
    rc += get_block_info(device->i2cDriver, toc_data_flash->code_data[target].addr_pointer, &code_block_info_flash, NULL);
    if (0 != rc) {
        ASTERA_ERROR("Failed to get block info for code or syscfg block in flash");
        code_block_info_flash.start_addr = toc_data_flash->code_data[target].addr_pointer;
        syscfg_block_info_flash.start_addr = toc_data_flash->syscfg_data[target].addr_pointer;

        code_block_info_flash.end_addr = 0;
        syscfg_block_info_flash.end_addr = 0;
        rc = 0;
    }

    // Find end block in flash
    addr = toc_data_flash->code_data[target].addr_pointer;
    while (BT_END_e != end_block_info_flash.type) {
        rc += find_next_block(device->i2cDriver, addr, 1, &addr, NULL);
        if (0 != rc) {
            ASTERA_ERROR("Failed to find end block in flash");
            break;
        }
        rc += get_block_info(device->i2cDriver, addr, &end_block_info_flash, NULL);
    }

    // Find end block in .mem
    addr = toc_data_mem->code_data[target].addr_pointer;
    while (BT_END_e != end_block_info_mem.type) {
        rc += find_next_block(device->i2cDriver, addr, 1, &addr, leo_flash_fw_buffer);
        if (0 != rc) {
            ASTERA_ERROR("Failed to find end block in .mem");
            break;
        }
        rc += get_block_info(device->i2cDriver, addr, &end_block_info_mem, leo_flash_fw_buffer);
    }
    if (0 != rc) {
        free(block_data_flash);
        free(block_data_mem);
        return rc;
    }

    //
    // The size of the write will be the larger of the size of the .mem file block or the size of the flash block
    //
    code_length_flash = end_block_info_flash.end_addr - code_block_info_flash.start_addr;
    code_length_mem = end_block_info_mem.end_addr - code_block_info_mem.start_addr;
    code_write_end_addr = (code_length_flash >= code_length_mem) 
        ? end_block_info_flash.end_addr 
        : code_block_info_flash.start_addr + code_length_mem;

    syscfg_write_end_addr = (syscfg_block_info_flash.length >= syscfg_block_info_mem.length) 
        ? syscfg_block_info_flash.end_addr 
        : syscfg_block_info_flash.start_addr + syscfg_block_info_mem.end_addr - syscfg_block_info_mem.start_addr;
    //
    // Erase current code block and syscfg block from flash
    //
    rc += flash_erase_range(device, syscfg_block_info_flash.start_addr, syscfg_write_end_addr);
    if (0 != rc) {
        ASTERA_ERROR("Failed to erase syscfg block in flash\n");
    }
    rc += flash_erase_range(device, code_block_info_flash.start_addr, code_write_end_addr);
    if (0 != rc) {
        ASTERA_ERROR("Failed to erase code block in flash\n");
    }

    //
    // Write code block and syscfg block from .mem file to flash
    //
    fw_flash_buffer_dwords = (uint32_t *)malloc(SPI_FLASH_SIZE);
    for (ii = 0; ii < SPI_FLASH_SIZE; ii+=4) {
      fw_flash_buffer_dwords[ii>>2] = leo_flash_fw_buffer[ii] << 24 | leo_flash_fw_buffer[ii+1] << 16 | leo_flash_fw_buffer[ii+2] << 8 | leo_flash_fw_buffer[ii+3];
    }

    ASTERA_INFO("Writing syscfg block to flash from 0x%x to 0x%x", syscfg_block_info_flash.start_addr, syscfg_write_end_addr);
    rc += flash_write(device->i2cDriver, syscfg_block_info_flash.start_addr, (syscfg_write_end_addr - syscfg_block_info_flash.start_addr) >> 2, &fw_flash_buffer_dwords[syscfg_block_info_mem.start_addr >> 2]);
    ASTERA_INFO("Writing code block to flash from 0x%x to 0x%x", code_block_info_flash.start_addr, code_write_end_addr);
    rc += flash_write(device->i2cDriver, code_block_info_flash.start_addr, (code_write_end_addr - code_block_info_flash.start_addr) >> 2, &fw_flash_buffer_dwords[code_block_info_mem.start_addr >> 2]);
    if (0 != rc) {
        ASTERA_ERROR("Failed to write code or syscfg block to flash");
    }

    if (0 != verify && 0 == rc) {
      ASTERA_INFO ("Verifying firmware update ...");
      // Use length of sysconfig block in .mem for CRC verification
      rc += flash_verify_block_crc(device->i2cDriver, syscfg_block_info_flash.start_addr, (syscfg_block_info_mem.end_addr - syscfg_block_info_mem.start_addr) >> 2, syscfg_block_info_mem.crc);

      addr = code_block_info_flash.start_addr;
      // Use length of code block in .mem for CRC verification
      rc += flash_verify_block_crc(device->i2cDriver, addr, (code_block_info_mem.end_addr - code_block_info_mem.start_addr) >> 2, code_block_info_mem.crc);
      // Verify each block of the code section until the end block
      prev_addr = addr;
      while (addr < end_block_info_flash.start_addr) {
          rc += find_next_block(device->i2cDriver, prev_addr, 1, &addr, NULL);
          rc += get_block_info(device->i2cDriver, addr, &tmp_block_info, NULL);
          rc += flash_verify_block_crc(device->i2cDriver, addr, (tmp_block_info.end_addr - tmp_block_info.start_addr) >> 2, tmp_block_info.crc);
          prev_addr = addr;
          if (tmp_block_info.type == BT_END_e) {
              break;
          }
      }
      if (0 != rc) {
        ASTERA_ERROR("Failed to verify code or syscfg block in flash");
      } else {
        ASTERA_INFO("Firmware update verified successfully");
      }
    }

    gettimeofday(&tv_now, NULL);
    dt = tv_now.tv_sec - tv_start.tv_sec;
    ASTERA_INFO("Total Elapsed time: %d seconds", dt);

    free(fw_flash_buffer_dwords);
    free(block_data_flash);
    free(block_data_mem);

    return rc;
}

LeoErrorType leo_spi_verify_crc(LeoDeviceType *device, char *filename) {
  LeoErrorType rc;
  int ii;
  uint32_t *block_data_flash;
  uint32_t *block_data_mem;
  toc_data_t *toc_data_flash;
  toc_data_t *toc_data_mem;
  block_info_t tmp_block_info;
  block_info_t end_block_info_mem;
  block_info_t end_block_info_flash;
  uint32_t code_slot_primary;
  uint32_t addr;
  uint32_t prev_addr;

  ASTERA_INFO("Verifying flash image blocks ...");

  rc = readFWImageFromFile(filename);
  if (rc != 0) {
    printf("Failed to read FW image from file %s", filename);
    return rc;
  }

  block_info_t block_info_flash = {0};
  block_info_t block_info_mem = {0};
  block_info_t toc_block_info_flash = {0};
  block_info_t toc_block_info_mem = {0};

  rc += find_block_by_type(device->i2cDriver, BT_TOC_e, &toc_block_info_flash, NULL);
  rc += find_block_by_type(device->i2cDriver, BT_TOC_e, &toc_block_info_mem, leo_flash_fw_buffer);
  rc += flash_verify_block_crc(device->i2cDriver, toc_block_info_flash.start_addr, (toc_block_info_flash.end_addr - toc_block_info_flash.start_addr) >> 2, toc_block_info_mem.crc);
  if (0 != rc) {
    ASTERA_ERROR("Failed to verify TOC block in flash");
    return rc;
  }

  if (toc_block_info_flash.config_data[0] != toc_block_info_mem.config_data[0]) {
    ASTERA_ERROR("TOC block in flash does not match TOC block in FW image");
    return 1;
  }

  if (1 == toc_block_info_flash.config_data[0]) {
    rc += find_block_by_type(device->i2cDriver, BT_DESCRIPTION_e, &block_info_flash, NULL);
    rc += find_block_by_type(device->i2cDriver, BT_DESCRIPTION_e, &block_info_mem, leo_flash_fw_buffer);
    if (0 != rc) {
      ASTERA_ERROR("Failed to find description block in flash");
      return rc;
    }

    rc += flash_verify_block_crc(device->i2cDriver, block_info_flash.start_addr, (block_info_flash.end_addr - block_info_flash.start_addr) >> 2, block_info_mem.crc);
    if (0 != rc) {
      ASTERA_INFO("block_info_flash start %06x end %06x len %06x", block_info_flash.start_addr, block_info_flash.end_addr, block_info_flash.length);
      ASTERA_ERROR("Failed to verify description block in flash");
      return rc;
    }
  }

  rc += find_block_by_type(device->i2cDriver, BT_FLASH_CTRL_e, &block_info_flash, NULL);
  rc += find_block_by_type(device->i2cDriver, BT_FLASH_CTRL_e, &block_info_mem, leo_flash_fw_buffer);
  rc += flash_verify_block_crc(device->i2cDriver, block_info_flash.start_addr, (block_info_flash.end_addr - block_info_flash.start_addr) >> 2, block_info_mem.crc);
  if (0 != rc) {
    ASTERA_ERROR("Failed to verify flash ctrl block in flash");
    return rc;
  }

  // to save time for now, only read the amount of toc that is needed
  block_info_flash = toc_block_info_flash;
  block_info_mem = toc_block_info_mem;
  block_data_flash = (uint32_t *)malloc(block_info_flash.length);
  if (NULL == block_data_flash) {
    ASTERA_ERROR("Failed to allocate memory for block_data_flash");
  }
  /* rc += read_block_data(device->i2cDriver, block_info, block_data_flash, NULL); */
  rc += flash_read(device->i2cDriver, block_info_flash.start_addr + 0x24, sizeof(toc_data_t) / 4, block_data_flash);
  toc_data_flash = (toc_data_t *) block_data_flash;
  if (0 != rc) {
      ASTERA_ERROR("Failed to read TOC block in flash");
      free(block_data_flash);
      return rc;
  }

  block_data_mem  = (uint32_t *)malloc(block_info_mem.length);
  if (NULL == block_data_mem) {
    ASTERA_ERROR("Failed to allocate memory for block_data_mem");
  }
  rc += read_block_data(device->i2cDriver, block_info_mem, block_data_mem, leo_flash_fw_buffer);
  toc_data_mem = (toc_data_t *) block_data_mem;
  if (0 != rc) {
    ASTERA_ERROR("Failed to read TOC block in .mem");
    free(block_data_flash);
    free(block_data_mem);
    return rc;
  }

  rc += get_block_info(device->i2cDriver, toc_data_mem->code_data[0].addr_pointer, &block_info_mem, leo_flash_fw_buffer);
  for (ii = 0; ii < 3; ii++) {
    // check if primary
    if (0 == (toc_data_flash->code_data[ii].config & 0x01010000)) {
      continue;
    }
    code_slot_primary = ii;
    rc += get_block_info(device->i2cDriver, toc_data_flash->code_data[ii].addr_pointer, &block_info_flash, NULL);

    // Find end block in flash
    addr = toc_data_flash->code_data[ii].addr_pointer;
    while (BT_END_e != end_block_info_flash.type) {
        rc += find_next_block(device->i2cDriver, addr, 1, &addr, NULL);
        if (0 != rc) {
            ASTERA_ERROR("Failed to find end block in flash");
            break;
        }
        rc += get_block_info(device->i2cDriver, addr, &end_block_info_flash, NULL);
    }

    addr = block_info_flash.start_addr;
    rc += flash_verify_block_crc(device->i2cDriver, addr, (block_info_flash.end_addr - block_info_flash.start_addr) >> 2, block_info_mem.crc);
    // Verify each block of the code section until the end block
    prev_addr = addr;
    while (addr < end_block_info_flash.start_addr) {
        rc += find_next_block(device->i2cDriver, prev_addr, 1, &addr, NULL);
        rc += get_block_info(device->i2cDriver, addr, &tmp_block_info, NULL);
        rc += get_block_info(device->i2cDriver, toc_data_mem->code_data[0].addr_pointer + (addr - block_info_flash.start_addr), &block_info_mem, leo_flash_fw_buffer);
        rc += flash_verify_block_crc(device->i2cDriver, addr, (tmp_block_info.end_addr - tmp_block_info.start_addr) >> 2, block_info_mem.crc);
        prev_addr = addr;
    }

    if (0 != rc) {
      ASTERA_ERROR("Failed to verify code block in flash at %06x", block_info_flash.start_addr);
      free(block_data_flash);
      free(block_data_mem);
      return rc;
    }
  }

  rc += get_block_info(device->i2cDriver, toc_data_mem->syscfg_data[0].addr_pointer, &block_info_mem, leo_flash_fw_buffer);
  for (ii = 0; ii < 3; ii++) {
    // check if valid
    if (0 == (toc_data_flash->syscfg_data[ii].valid)) {
      continue;
    }
    // check if correlated to primary code
    if (toc_data_flash->syscfg_data[ii].code_slot_correlation != code_slot_primary) {
      continue;
    }
    rc += get_block_info(device->i2cDriver, toc_data_flash->syscfg_data[ii].addr_pointer, &block_info_flash, NULL);
    rc += flash_verify_block_crc(device->i2cDriver, 
                                  toc_data_flash->syscfg_data[ii].addr_pointer, 
                                  (block_info_flash.end_addr - block_info_flash.start_addr) >> 2, 
                                  block_info_mem.crc);
    if (0 != rc) {
      ASTERA_ERROR("Failed to verify syscfg block in flash at %06x", block_info_flash.start_addr);
      free(block_data_flash);
      free(block_data_mem);
      return rc;
    }

  }

  ASTERA_INFO("CRC verification %s\n", (0 == rc) ? "passed" : "failed");
  free(block_data_flash);
  free(block_data_mem);

  return(rc);
}

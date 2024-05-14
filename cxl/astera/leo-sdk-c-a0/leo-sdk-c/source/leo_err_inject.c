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
 * @file leo_err_inject.c
 * @brief Implementation of public functions for the SDK err inject interface.
 */
#include "../include/hal.h"
#include "../include/leo_err_inject.h"
#include "../include/leo_api_internal.h"
#include "../include/misc.h"


static void ecc_poison_config(LeoDeviceType *device, uint32_t channel,
                              struct errorInjectStruct errInject);
static uint32_t get_ctrl_addr(uint32_t channel);

/*
 * @brief: function to get base address
 * @param1: Channel
 * @return: base address
 */
static uint32_t get_ctrl_addr(uint32_t channel) {
  uint32_t ctrl_base_addr;
  if (channel == 0) {
    ctrl_base_addr = CSR_DDR_CTLR_0_IP_BASE_ADDRESS;
  } else {
    ctrl_base_addr = CSR_DDR_CTLR_1_IP_BASE_ADDRESS;
  }
  return ctrl_base_addr;
}

/*
 * @brief configure ecc poison registers
 * @param1: I2C device handle
 * @param2: channel
 * @param3: Error Inject parameters
 */
static void ecc_poison_config(LeoDeviceType *device, uint32_t channel,
                              struct errorInjectStruct errInject) {
  // ECCCFG0
  uint32_t ecc_mode;  // 000 : disabled 100 : SEC/DED 101 : Advanced
  uint32_t test_mode; // if set ecc_mode is being ignored so tied to 0 for now
  uint32_t ecc_type;  // 00 : sideband ECC 01 : sideband Multi beat ECC (only
                      // supported for SEC/DED)
  uint32_t ecccfg0_val = 0;
  // ECCCFG1
  uint32_t poison_advecc_kdb = 0; // Poison ADVECC KDB
  uint32_t poison_chip_en = 0;  // 0: disbale 1 : Persistently poison the write data
                            // to rank address matchhes ecc_poison_rank
  uint32_t data_poison_bit = 0; // 0 : poinson 2 bits 1 : poison 1 bit
  uint32_t data_poison_en = 0;  // Enable ECC data poisoning
  uint32_t ecccfg1_val = 0;
  // ECCPOISONADDR0
  uint32_t ecc_poison_col = 0;  // indicate colum address for ECC poisoning
                            // ecc_poison_col[N:0] = 0 (N=2 full bus width, N=3
                            // half N=4 quarter)
  uint32_t ecc_poison_cid = 0;  // Chip ID of address poisoning (only DDR4 3DS)
  uint32_t ecc_poison_rank = 0; // Rank aaddress of ECC poisoning
  uint32_t eccpoisonaddr0_val = 0;
  // ECCPOISONADDR1
  uint32_t ecc_poison_row = 0;
  uint32_t ecc_poison_bank = 0;
  uint32_t ecc_poison_bg = 0;
  uint32_t eccpoisonaddr1_val = 0;
  // ADVECCINDEX
  uint32_t ecc_syndrom_sel = 0; // selector of which DRAM beat output to ECCCSYN0/1/2,
                                // ECCUSYN0/1/2, ECCCDATA0/1 ECCUDATA0/1, ECCSYMBOL
  uint32_t ecc_err_symbol_sel = 0;  // selector of which err symbol's status out put
                                    // to ADVECCSTAT.advecc_err_symbol_pos
  uint32_t ecc_poison_beats_sel = 0;    // selector of which DRAM beat's poison pattern
                                        // will be set by ECCPOISONPAT0/1/2 registeors
  uint32_t adveccindex_val = 0;
  // ECCPOISONPAT0
  uint32_t ecc_poison_data_31_0 =0; 
  uint32_t eccpoisonpat0_val = 0;
  // ECCPOISONPAT1
  uint32_t ecc_poison_data_63_32 = 0;
  uint32_t eccpoisonpat1_val = 0;
  // ECCPOISONPAT2
  uint32_t ecc_poison_data_71_64 = 0;
  uint32_t ecc_poison_data_79_72 = 0;
  uint32_t eccpoisonpat2_val = 0;
  // ECCCFG2
  uint32_t flip_bit_pos0 = 0; // bit position 0 for error poisoning
  uint32_t flip_bit_pos1 = 1; // bit position 1 for 2 bit error
  uint32_t ecccfg2_val = 0;
  uint32_t eccctl_val = 0;

  ecc_mode = LEO_ADV_ECC;
  test_mode = FALSE;
  ecc_type = LEO_ECC_SIDE_BAND;

  poison_advecc_kdb = 0;
  data_poison_bit = 0;

  if (errInject.errinj_enable) {
    poison_chip_en = FALSE;
    data_poison_en = TRUE;
  } else {
    poison_chip_en = FALSE;
    data_poison_en = FALSE;
  }

  ecc_poison_col = errInject.col;
  ecc_poison_cid = FALSE;
  ecc_poison_rank = errInject.rank;

  ecc_poison_row = errInject.row;
  ecc_poison_bank = errInject.bank;
  ecc_poison_bg = errInject.bg;

  ecc_syndrom_sel = 0;
  ecc_err_symbol_sel = 0;
  ecc_poison_beats_sel = 0;

  if (errInject.errinj_enable) {
    if (errInject.uncorr) {
      ecc_poison_data_31_0 = LEO_DDR_POISON_DATA_BITS;
      data_poison_bit = LEO_ECC_UE;
    } else {
      ecc_poison_data_31_0 = 1;
      data_poison_bit = LEO_ECC_CE;
    }
  }

  ecc_poison_data_63_32 = 0;

  ecc_poison_data_71_64 = 0;
  ecc_poison_data_79_72 = 0;

  if (errInject.uncorr) {
    flip_bit_pos0 = 1;
    flip_bit_pos1 = 0;
  } else {
    flip_bit_pos0 = 0;
    flip_bit_pos1 = 1;
  }

  ecccfg0_val = 0;
  ecccfg0_val = DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_TYPE_MODIFY(
      ecccfg0_val, ecc_type);
  ecccfg0_val = DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_MODIFY(
      ecccfg0_val, test_mode);
  ecccfg0_val = DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_MODIFY(
      ecccfg0_val, ecc_mode);

  ecccfg1_val = 0;
  ecccfg1_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_POISON_CHIP_EN_MODIFY(
          ecccfg1_val, poison_chip_en);
  ecccfg1_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_DATA_POISON_EN_MODIFY(
          ecccfg1_val, data_poison_en);
  ecccfg1_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_DATA_POISON_BIT_MODIFY(
          ecccfg1_val, data_poison_bit);

  eccpoisonaddr0_val = 0;
  eccpoisonaddr0_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ECC_POISON_RANK_MODIFY(
          eccpoisonaddr0_val, ecc_poison_rank);
  eccpoisonaddr0_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ECC_POISON_CID_MODIFY(
          eccpoisonaddr0_val, ecc_poison_cid);
  eccpoisonaddr0_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ECC_POISON_COL_MODIFY(
          eccpoisonaddr0_val, ecc_poison_col);

  eccpoisonaddr1_val = 0;
  eccpoisonaddr1_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ECC_POISON_BG_MODIFY(
          eccpoisonaddr1_val, ecc_poison_bg);
  eccpoisonaddr1_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ECC_POISON_BANK_MODIFY(
          eccpoisonaddr1_val, ecc_poison_bank);
  eccpoisonaddr1_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ECC_POISON_ROW_MODIFY(
          eccpoisonaddr1_val, ecc_poison_row);

  adveccindex_val = 0;
  adveccindex_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ECC_POISON_BEATS_SEL_MODIFY(
          adveccindex_val, ecc_poison_beats_sel);
  adveccindex_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ECC_ERR_SYMBOL_SEL_MODIFY(
          adveccindex_val, ecc_err_symbol_sel);
  adveccindex_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ECC_SYNDROME_SEL_MODIFY(
          adveccindex_val, ecc_syndrom_sel);

  eccpoisonpat0_val = 0;
  eccpoisonpat0_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT0_ECC_POISON_DATA_31_0_MODIFY(
          eccpoisonpat0_val, ecc_poison_data_31_0);
  eccpoisonpat1_val = 0;
  eccpoisonpat1_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT1_ECC_POISON_DATA_63_32_MODIFY(
          eccpoisonpat2_val, ecc_poison_data_63_32);
  eccpoisonpat2_val = 0;
  eccpoisonpat2_val =
      DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT2_ECC_POISON_DATA_71_64_MODIFY(
          eccpoisonpat2_val, ecc_poison_data_71_64);

  ecccfg2_val = 0;
  ecccfg2_val = DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG2_FLIP_BIT_POS0_MODIFY(
      ecccfg2_val, flip_bit_pos0);
  ecccfg2_val = DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG2_FLIP_BIT_POS1_MODIFY(
      ecccfg2_val, flip_bit_pos1);
  eccctl_val = LEO_DDR_ECCCTL_VAL;

  ASTERA_DEBUG("ECC POISON CONFIGS");
  ASTERA_DEBUG("ECCCFG0");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_type", ecc_type);
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_mode", ecc_mode);
  ASTERA_DEBUG("| %-50s : %10d |\n", "test_mode", test_mode);
  ASTERA_DEBUG("ECCCFG1");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "poison_chip_en", poison_chip_en);
  ASTERA_DEBUG("| %-50s : %10d |\n", "data_poison_en", data_poison_en);
  ASTERA_DEBUG("| %-50s : %10d |\n", "data_poison_bit", data_poison_bit);
  ASTERA_DEBUG("ECCPOISONADDR0");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_rank", ecc_poison_rank);
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_cid", ecc_poison_cid);
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_col", ecc_poison_col);
  ASTERA_DEBUG("ECCPOISONADDR1");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_bg", ecc_poison_bg);
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_bank", ecc_poison_bank);
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_row", ecc_poison_row);
  ASTERA_DEBUG("ECCPOISONPAT0");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_data_31_0",
               ecc_poison_data_31_0);
  ASTERA_DEBUG("ECCPOISONPAT1");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_data_63_32",
               ecc_poison_data_63_32);
  ASTERA_DEBUG("ECCPOISONPAT1");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "ecc_poison_data_71_64",
               ecc_poison_data_71_64);
  ASTERA_DEBUG("ECCCFG2");
  ASTERA_DEBUG("| %-50s : %-10s |\n", "FIELD NAME", "VALUE");
  ASTERA_DEBUG("| %-50s : %10d |\n", "flip_bit_pos0", flip_bit_pos0);
  ASTERA_DEBUG("| %-50s : %10d |\n", "flip_bit_pos1", flip_bit_pos1);
//  ASTERA_DEBUG("ECCCTL : ALL INTR ENABLED");

  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_SWCTL_ADDRESS,
                   0);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ADDRESS,
                   ecccfg0_val);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_ADDRESS,
                   ecccfg1_val);
  leoWriteWordData(
      device->i2cDriver,
      get_ctrl_addr(channel) +
          DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ADDRESS,
      eccpoisonaddr0_val);
  leoWriteWordData(
      device->i2cDriver,
      get_ctrl_addr(channel) +
          DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ADDRESS,
      eccpoisonaddr1_val);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ADDRESS,
                   adveccindex_val);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT0_ADDRESS,
                   eccpoisonpat0_val);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT1_ADDRESS,
                   eccpoisonpat1_val);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT2_ADDRESS,
                   eccpoisonpat2_val);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG2_ADDRESS,
                   ecccfg2_val);
//  leoWriteWordData(device->i2cDriver,
//                   get_ctrl_addr(channel) +
//                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCTL_ADDRESS,
//                   eccctl_val);
  leoWriteWordData(device->i2cDriver,
                   get_ctrl_addr(channel) +
                       DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_SWCTL_ADDRESS,
                   1);
}

/**
 * @brief Leo set or clear Error Inject
 * @param1: I2C device handle
 * @param2: Error inject start address
 * @param3: Error type CE or UE 1- Uncorrectable error
 * @param4: Enable or disable error injection, 1 - Enable
 * Returns LeoErrorType
 */
LeoErrorType leoInjectError(LeoDeviceType *device, uint64_t startAddr,
                            uint32_t errType, uint32_t enable) {
  uint32_t channel = 0;
  uint32_t subch = 0;
  struct errorInjectStruct leoErrorInj = {.col = 0,
                                          .row = 0,
                                          .rank = 0,
                                          .bg = 0,
                                          .bank = 0,
                                          .uncorr = errType,
                                          .errinj_enable = enable};
  uint32_t temp = 0;

  ASTERA_INFO("Error Inject Test");
  /* Get preboot DDR and CXL config */
  LeoDdrConfigType ddrConfig;
  leoGetPrebootDdrConfig(device, &ddrConfig);
  leoErrorInj.dqWidth = ddrConfig.dqWidth;

  if (startAddr) {
    if (leoErrorInj.dqWidth == 8) {
      leoErrorInj.row = (startAddr >> 19) & 0xffff;
      leoErrorInj.bank = (startAddr >> 17) & 0x3;
      leoErrorInj.col = (startAddr >> 11) & 0x3f;
      leoErrorInj.col <<= 4;
      leoErrorInj.bg = (startAddr >> 8) & 0x7;
      if (ddrConfig.numRanks == 2) {
          leoErrorInj.rank = (startAddr >> 35) & 0x1;
      } else if (ddrConfig.numRanks == 4) {
          leoErrorInj.rank = (startAddr >> 35) & 0x3;
      }
    } else if (leoErrorInj.dqWidth == 4) {
      leoErrorInj.row = (startAddr >> 20) & 0xffff;
      leoErrorInj.bank = (startAddr >> 18) & 0x3;
      leoErrorInj.col = (startAddr >> 11) & 0x7f;
      leoErrorInj.col <<= 4;
      leoErrorInj.bg = (startAddr >> 8) & 0x7;
      if (ddrConfig.numRanks == 2) {
          leoErrorInj.rank = (startAddr >> 36) & 0x1;
      } else if (ddrConfig.numRanks == 4) {
          leoErrorInj.rank = (startAddr >> 36) & 0x3;
      }
    }

    // FIXME: hard-coding to 4 way interleaving
    subch = (startAddr >> 6) & 0x3;
    channel = 1;
    if ((0 == subch) || (1 == subch)) {
        channel = 0;
    }

    ASTERA_INFO("Input values received: DQ Width:%d, startAddr:0x%llx, row:0x%x, "
                 "col:0x%x, bg:0x%x, bank:0x%x, rank:0x%x, subch:0x%x, channel:0x%x",
                 leoErrorInj.dqWidth, startAddr, leoErrorInj.row,
                 leoErrorInj.col, leoErrorInj.bg, leoErrorInj.bank,
                 leoErrorInj.rank, subch, channel);
  }
  ecc_poison_config(device, channel, leoErrorInj);

  return LEO_SUCCESS;
}

/**
 * @brief Leo Inject Viral
 *
 * Enables and injects viral in Leo
 * @param1: I2C device handle
 * Returns LeoErrorType
 */

LeoErrorType leoInjectViral(LeoDeviceType *device) {
  return (
      leoWriteWordData(device->i2cDriver,
                       leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_MINI_CXL_VIRAL_REQUEST_ADDRESS,
                                        device->controllerIndex), 
                        1));
}

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
 * @file leo_err_inject.h
 * @brief Definitions related to error inject data structure and functions.
 */

#include "stdio.h"
#include "string.h"

#include "leo_api.h"
#include "leo_error.h"
#include "leo_globals.h"
#include "leo_i2c.h"
#include "misc.h"

/**
 * @brief Macro for LEO register address.
 */
#define LEO_DDR_ECCCTL_VAL 0x71f
#define LEO_DDR_POISON_DATA_BITS 0xfff
#define LEO_ADV_ECC 0x5
#define LEO_ECC_UE 0
#define LEO_ECC_CE 1
#define LEO_ECC_SIDE_BAND 0

#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_TYPE_MODIFY(r, x)       \
  ((((x) << 4) & 0x00000030) | ((r)&0xffffffcf))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_POISON_CHIP_EN_MODIFY(r, x) \
  ((((x) << 2) & 0x00000004) | ((r)&0xfffffffb))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_DATA_POISON_EN_MODIFY(r, x) \
  (((x)&0x00000001) | ((r)&0xfffffffe))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_DATA_POISON_BIT_MODIFY(r,   \
                                                                          x)   \
  ((((x) << 1) & 0x00000002) | ((r)&0xfffffffd))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ECC_POISON_RANK_MODIFY( \
    r, x)                                                                         \
  ((((x) << 24) & 0x03000000) | ((r)&0xfcffffff))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ECC_POISON_CID_MODIFY( \
    r, x)                                                                        \
  ((((x) << 16) & 0x000f0000) | ((r)&0xfff0ffff))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ECC_POISON_COL_MODIFY( \
    r, x)                                                                        \
  (((x)&0x00000fff) | ((r)&0xfffff000))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ECC_POISON_BG_MODIFY( \
    r, x)                                                                       \
  ((((x) << 28) & 0x70000000) | ((r)&0x8fffffff))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ECC_POISON_BANK_MODIFY( \
    r, x)                                                                         \
  ((((x) << 24) & 0x03000000) | ((r)&0xfcffffff))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ECC_POISON_ROW_MODIFY( \
    r, x)                                                                        \
  (((x)&0x0003ffff) | ((r)&0xfffc0000))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ECC_POISON_BEATS_SEL_MODIFY( \
    r, x)                                                                           \
  ((((x) << 5) & 0x000001e0) | ((r)&0xfffffe1f))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ECC_ERR_SYMBOL_SEL_MODIFY( \
    r, x)                                                                         \
  ((((x) << 3) & 0x00000018) | ((r)&0xffffffe7))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ECC_SYNDROME_SEL_MODIFY( \
    r, x)                                                                       \
  (((x)&0x00000007) | ((r)&0xfffffff8))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT0_ECC_POISON_DATA_31_0_MODIFY( \
    r, x)                                                                             \
  ((x)&0xffffffff)
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT1_ECC_POISON_DATA_63_32_MODIFY( \
    r, x)                                                                              \
  ((x)&0xffffffff)
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT2_ECC_POISON_DATA_71_64_MODIFY( \
    r, x)                                                                              \
  (((x)&0x000000ff) | ((r)&0xffffff00))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG2_FLIP_BIT_POS0_MODIFY(r, x)  \
  ((((x) << 16) & 0x007f0000) | ((r)&0xff80ffff))
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG2_FLIP_BIT_POS1_MODIFY(r, x)  \
  ((((x) << 24) & 0x7f000000) | ((r)&0x80ffffff))

/* Field member: DWC_ddrctl_APB_slave::REGB_DDRC_CH0::ECCCFG0.test_mode    */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_MSB 3
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_LSB 3
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_WIDTH 1
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_READ_ACCESS 1
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_WRITE_ACCESS 1
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_RESET 0x0
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_FIELD_MASK        \
  0x00000008
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_GET(x)            \
  (((x)&0x00000008) >> 3)
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_SET(x)            \
  (((x) << 3) & 0x00000008)
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_TEST_MODE_MODIFY(r, x)      \
  ((((x) << 3) & 0x00000008) | ((r)&0xfffffff7))
/* Field member: DWC_ddrctl_APB_slave::REGB_DDRC_CH0::ECCCFG0.ecc_mode     */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_MSB 2
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_LSB 0
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_WIDTH 3
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_READ_ACCESS 1
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_WRITE_ACCESS 1
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_RESET 0x0
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_FIELD_MASK         \
  0x00000007
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_GET(x)             \
  ((x)&0x00000007)
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_SET(x)             \
  ((x)&0x00000007)
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ECC_MODE_MODIFY(r, x)       \
  (((x)&0x00000007) | ((r)&0xfffffff8))

/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.SWCTL                      */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_SWCTL_ADDRESS 0x10c80
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_SWCTL_BYTE_ADDRESS 0x10c80
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCCFG0                    */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_ADDRESS 0x10600
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG0_BYTE_ADDRESS 0x10600
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCCFG1                    */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_ADDRESS 0x10604
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG1_BYTE_ADDRESS 0x10604
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCPOISONADDR0             */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_ADDRESS 0x10648
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR0_BYTE_ADDRESS 0x10648
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCPOISONADDR1             */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_ADDRESS 0x1064c
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONADDR1_BYTE_ADDRESS 0x1064c
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ADVECCINDEX                */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_ADDRESS 0x10650
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ADVECCINDEX_BYTE_ADDRESS 0x10650
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCPOISONPAT0              */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT0_ADDRESS 0x10658
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT0_BYTE_ADDRESS 0x10658
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCPOISONPAT1              */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT1_ADDRESS 0x1065c
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT1_BYTE_ADDRESS 0x1065c
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCPOISONPAT2              */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT2_ADDRESS 0x10660
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCPOISONPAT2_BYTE_ADDRESS 0x10660
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCCFG2                    */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG2_ADDRESS 0x10668
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCFG2_BYTE_ADDRESS 0x10668
/* Register: DWC_ddrctl.APB_slave.REGB_DDRC_CH0.ECCCTL                     */
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCTL_ADDRESS 0x1060c
#define DWC_DDRCTL_APB_SLAVE_REGB_DDRC_CH0_ECCCTL_BYTE_ADDRESS 0x1060c
/**
 * @brief structure for error inject entries.
 */
struct errorInjectStruct {
  uint32_t startAddr;
  uint32_t row;
  uint32_t col;
  uint32_t bg;
  uint32_t bank;
  uint32_t rank;
  uint8_t uncorr;
  uint8_t errinj_enable;
  uint8_t dqWidth;
};


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
 * @file hal.h
 * @brief Definitions related Hardware abstraction layer for Leo CPU interfaces
 * to hardware (e.g. CSRs, IP registers and Memory etc.).
 */

#ifndef _HAL_H
#define _HAL_H

#include "misc.h"


//TODO YW: copied from leo_api_internal.h
#define LEO_BOOT_STAT_OFFSET  0xf0
#define LEO_FW_VERSION_OFFSET 0xf4
#define LEO_FW_BUILD_OFFSET   0xf8


//TODO YW: same solution for all MINI registers?
/*********************************************/
/* Keep using A0 register nameing convention */
/*********************************************/
#ifdef CHIP_D5
#define LEO_TOP_CSR_CMAL_SECOND_SLV_STS_RRSP_DDR_RETURNED_POISON_CNT_ADDRESS LEO_TOP_CSR_CMAL_STS_RRSP_DDR_RETURNED_POISON_CNT_ADDRESS
#define LEO_TOP_CSR_CMAL_SECOND_SLV_STS_RRSP_RDP_CAUSED_POISON_CNT_ADDRESS   LEO_TOP_CSR_CMAL_STS_RRSP_RDP_CAUSED_POISON_CNT_ADDRESS
#define LEO_TOP_CSR_CXL_CTR_MINI_CXL_VIRAL_REQUEST_ADDRESS                   LEO_TOP_CSR_CXL_CTR_CXL_VIRAL_REQUEST_ADDRESS      
#elif CHIP_A0
#else
#error NO chip revision provided
#endif


//TODO YW: DDR related, copied from leo_api_internal.h
#define DDR_TEMP_EXPECTED_NO_WORKLOAD 32
#define DDR_TEMP_THEORETICAL_MAX 105

#define ENABLE 1
#define DISABLE 0
#define CLEAR 0
#define SRAM_ANA_CTR_RD_ACTIVATE_OFFSET 10
#define SRAM_ANA_CTR_PRECHARGE_OFFSET 13
#define SRAM_ANA_CTR_REFRESH_OFFSET 37
#define SRAM_ANA_CTR_CRIT_REFRESH_OFFSET 38
#define SRAM_ANA_CTR_SPEC_REFRESH_OFFSET 39

#define SRAM_ANA_CTR_RD_ACTIVATE_OFFSET_SUBCHN1 74
#define SRAM_ANA_CTR_PRECHARGE_OFFSET_SUBCHN1 77
#define SRAM_ANA_CTR_REFRESH_OFFSET_SUBCHN1 101
#define SRAM_ANA_CTR_CRIT_REFRESH_OFFSET_SUBCHN1 102
#define SRAM_ANA_CTR_SPEC_REFRESH_OFFSET_SUBCHN1 103




/*
 *****************************************************************************
 * Leo CSR base addresses
 ******************************************************************************
 */

#define CSR_CXL_MISC_BASE_ADDRESS 0x000000
#define DW_APB_SSI_OFFSET (0x6000)
#define DW_APB_SSI_ADDRESS (CSR_CXL_MISC_BASE_ADDRESS + DW_APB_SSI_OFFSET)
#define CSR_CXL_CMAL_BASE_ADDRESS 0x100000

#define CSR_CXL_PHY_BASE_ADDRESS 0x200000
#define CXL_PHY_INTERNAL_PMA_REG_ACCESS 0x080000
#define CXL_PHY_INTERNAL_PMA_ID_SHIFT (16)
#define CXL_PHY_INTERNAL_PMA_ID_MASK (0x030000)

#define CSR_CXL_CTLR_0_CSR_BASE_ADDRESS 0x400000
#define CSR_CXL_CTLR_1_CSR_BASE_ADDRESS 0x500000

#define CSR_CXL_CTLR_0_DBI_BASE_ADDRESS (0x600000)
#define CSR_CXL_CTLR_1_DBI_BASE_ADDRESS (0x700000)

#define CSR_DWC_CXL_CTLR_0_DBI_CS2_ACCESS_ADDR_PAT (0x040000)
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_PCIE_4K_CONFIG_SPACE_BASE_ADDR_PAT  \
  (0x0)
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_ELBI_64K_MBAR0_SPACE_BASE_ADDR_PAT  \
  (0x1)
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_IATU_REGS_SPACE_BASE_ADDR_PAT       \
  (0x1 | CSR_DWC_CXL_CTLR_0_DBI_CS2_ACCESS_ADDR_PAT)
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_RCRB_4K_CONFIG_SPACE_BASE_ADDR_PAT  \
  ((0x80001) |                                                                 \
   CSR_DWC_CXL_CTLR_0_DBI_CS2_ACCESS_ADDR_PAT) /* csr_addr[19] = dbi_addr[21] &
                                                  csr_addr[17] = dbi_addr[20] */
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_RCRB_64K_MBAR0_SPACE_BASE_ADDR_PAT  \
  ((0xA0001) |                                                                 \
   CSR_DWC_CXL_CTLR_0_DBI_CS2_ACCESS_ADDR_PAT) /* csr_addr[19] = dbi_addr[21] &
                                                  csr_addr[17] = dbi_addr[20] */
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_REG_ADDR_SHIFT                      \
  (2) /* register offset in the csr dbi address always starts from csr_addr[2]
       since we specify a dword offset */

#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_PCIE_4K_CONFIG_SPACE_ADDR(          \
    csr_dbi_base_addr, reg_offset)                                             \
  ((csr_dbi_base_addr) |                                                       \
   (CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_PCIE_4K_CONFIG_SPACE_BASE_ADDR_PAT) |   \
   (reg_offset << CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_REG_ADDR_SHIFT))
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_ELBI_64K_MBAR0_SPACE_ADDR(          \
    csr_dbi_base_addr, reg_offset)                                             \
  ((csr_dbi_base_addr) |                                                       \
   (CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_ELBI_64K_MBAR0_SPACE_BASE_ADDR_PAT) |   \
   (reg_offset << CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_REG_ADDR_SHIFT))
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_IATU_REGS_SPACE_ADDR(               \
    csr_dbi_base_addr, reg_offset)                                             \
  ((csr_dbi_base_addr) |                                                       \
   (CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_IATU_REGS_SPACE_BASE_ADDR_PAT) |        \
   (reg_offset << CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_REG_ADDR_SHIFT))
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_RCRB_4K_CONFIG_SPACE_ADDR(          \
    csr_dbi_base_addr, reg_offset)                                             \
  ((csr_dbi_base_addr) |                                                       \
   (CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_RCRB_4K_CONFIG_SPACE_BASE_ADDR_PAT) |   \
   (reg_offset << CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_REG_ADDR_SHIFT))
#define CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_RCRB_64K_MBAR0_SPACE_ADDR(          \
    csr_dbi_base_addr, reg_offset)                                             \
  ((csr_dbi_base_addr) |                                                       \
   (CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_RCRB_64K_MBAR0_SPACE_BASE_ADDR_PAT) |   \
   (reg_offset << CSR_DWC_CXL_CTRL_DBI_ADDRESS_SPACE_REG_ADDR_SHIFT))

#define CSR_DDR_CTLR_0_CSR_BASE_ADDRESS 0x800000
#define CSR_DDR_CTLR_1_CSR_BASE_ADDRESS 0x900000
#define CSR_DDR_CTLR_0_IP_BASE_ADDRESS 0xa00000
#define CSR_DDR_CTLR_1_IP_BASE_ADDRESS 0xb00000
#define CSR_DDR_CTLR_0_PHY_IP_BASE_ADDRESS 0xc00000
#define CSR_DDR_CTLR_1_PHY_IP_BASE_ADDRESS 0xd00000

//******************************************************************************
// Macros for CSR address generation
//******************************************************************************
#define CSR_DWC_CXL_CTRL_DBI_ADDR_TO_CSR_ADDR(DWC_DBI_ADDR)                    \
  ((DWC_DBI_ADDR & 0xffff) | ((DWC_DBI_ADDR & 0x100000) >> 3) |                \
   ((DWC_DBI_ADDR & 0x200000) >> 2))
#define COMPRESS_CXL_MBAR0_DBI_ADDR_TO_CSR_ADDR(DWC_DBI_ADDR)                  \
  (CSR_DWC_CXL_CTRL_DBI_ADDR_TO_CSR_ADDR(DWC_DBI_ADDR) |                       \
   CSR_DWC_CXL_CTLR_0_DBI_CS2_ACCESS_ADDR_PAT | 1)
#define COMPRESS_CXL_DBI2_ADDR_TO_CSR(DWC_DBI_ADDR)                            \
  (CSR_DWC_CXL_CTRL_DBI_ADDR_TO_CSR_ADDR(DWC_DBI_ADDR) |                       \
   CSR_DWC_CXL_CTLR_0_DBI_CS2_ACCESS_ADDR_PAT)

/*
 * For DV_SIM FW state notification
 */
#define DV_SIM_FW_STATE_NOTIF_ADDR (uint32_t volatile *)(0x1fffdc)
/* defines for DV sync */
#define DV_SIM_FW_STATE_RESET 0x8000
#define DV_SIM_FW_STATE_IDLE 0x8001
#define DV_SIM_FW_STATE_DDR_ACTIVE 0x8002
#define DV_SIM_FW_STATE_CXL_ACTIVE 0x8004
#define DV_SIM_FW_STATE_LINK_ACTIVE 0x8008 // ready to process data traffic
#define DV_SIM_FW_STATE_INITIAL_CFG_DONE 0x8010
#define DV_SIM_FW_STATE_FAST_CLK_EN 0x8020
#define DV_SIM_FW_STATE_FAST_CLK_DIS 0x8040
#define DV_SIM_FW_DDRPHY_CFG_DONE 0x8080
#define DV_SIM_FW_DDRPHY_INIT_START 0x8100
#define DV_SIM_FW_DDRPHY_INIT_END 0x8200
/*
 *****************************************************************************
 * Defines
 ******************************************************************************
 */
typedef struct {
  uint32_t addr;
  uint32_t data;
} cfg_data_t;

/*
 *****************************************************************************
 * ARC internal CSR base addresses
 ******************************************************************************
 */
#define ARC_CSR_RGN 0xf0000000
#define ARC_CSR_RGN_BRC 0xf4000000
//#define ARC_CSR_RGN_BRC 0xf0000000
//#define ARC_CSR_RGN 0x000000

typedef struct {
  uint32_t error;
} hal_status_t;

/*
 *****************************************************************************
 * Function prototypes ("Public" functions provided for users of this HAL)
 ******************************************************************************
 */

/*
 * Function: hal_init
 * Description: Initialize the HAL layer
 * Parameters: None
 * Returns: None
 */
void hal_init(void);

/*
 * Function: csr_check_error
 * Description: Get CSR access error
 * Parameters: None
 * Returns: CSR access error type
 */
uint32_t csr_check_error(void);

/*
 * Function: csr_rd
 * Description: Read a CSR register
 * Parameters: addr - CSR register address
 * Returns: CSR register value
 */
uint32_t csr_rd(uint32_t addr);

/*
 * Function: csr_wr
 * Description: Write a CSR register
 * Parameters: addr - CSR register address
 *             data - CSR register value
 * Returns: Status of the operation
 */
uint32_t csr_wr(uint32_t addr, uint32_t data);

/*
 * Function: csr_wr_no_chk
 * Description: Write to a CSR register without checking for errors
 * Parameters: addr - CSR register address
 *             data - CSR register value
 * Returns: None
 */
void csr_wr_no_chk(uint32_t addr, uint32_t data);

/*
 * Function: csr_brc_wr
 * Description: Write to multiple instance of the same module
 *              (broadcast)
 * Parameters: addr - CSR register address
 *             data - CSR register value
 * Returns: Status of the operation
 */
uint32_t csr_brc_wr(uint32_t addr, uint32_t data);

/*
 * Function: csr_brc_wr_no_chk
 * Description: Write to multiple instance of the same module
 *              without checking for errors
 * Parameters: addr - CSR register address
 *             data - CSR register value
 * Returns: None
 */
void csr_brc_wr_no_chk(uint32_t addr, uint32_t data);

// ------------------------------------
// CSR wr field: use read modify write to update a field
// ------------------------------------
/*
 * Function: csr_wr_field
 * Description: Write to a CSR register field by using
 *             read-modify-write
 * Parameters: addr - CSR register address
 *             data - CSR register value
 *             mask - CSR register field mask
 *             shift - CSR register field shift
 * Returns: Status of the operation
 */
uint32_t csr_wr_field(uint32_t addr, uint32_t field_mask, uint32_t field_lsb,
                      uint32_t field_data);

/*
 * Function: csr_wr_array
 * Description: Write an array of CSR data
 *              each entry of the array is an addr/data pair
 *             base_addr will be added to the addr when writing to the CSR
 *             The last element of the array must be: addr == 0xffffffff, data
 * == 0 Example of cfg_data_t array: cfg_data_t cfg_0[] = { {0x1000, 0},
 * {0x1004, 1},
 * {0xffffffff, 0}
 * };
 * Parameters: base_addr - CSR register base address
 *             cfg_data - CSR register array
 * Returns: Status of the operation
 */
uint32_t csr_wr_array(uint32_t base_addr, cfg_data_t *p_cfg);

/*
 * Function: csr_brc_wr_array
 * Description: Write an array of CSR data to multiple instances of the same
 * module Parameters: base_addr - CSR register base address cfg_data - CSR
 * register array Returns: Status of the operation
 */
uint32_t csr_brc_wr_array(uint32_t base_addr, cfg_data_t *p_cfg);

uint32_t cxl_ctr_dbi_csr_wr(uint32_t csr_dbi_base_addr, uint32_t dbi_addr,
                            uint32_t dbi_cs2, uint32_t data);

uint32_t cxl_ctr_dbi_csr_rd(uint32_t csr_dbi_base_addr, uint32_t dbi_addr,
                            uint32_t dbi_cs2);

uint32_t leoGetDdrCtlAddr(uint32_t top_csr_addr, uint8_t ctl_id);
uint32_t leoGetCxlCtrAddr(uint32_t top_csr_addr, uint8_t ctl_id);

void hal_send_dv_sync(uint32_t val);
#endif

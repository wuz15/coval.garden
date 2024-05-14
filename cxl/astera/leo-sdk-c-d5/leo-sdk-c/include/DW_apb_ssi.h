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
 * @file DW_apb_ssi.h
 * @brief Definitions related to flash subsector size, Page sizes.
 *        This file has definitions related to the registers required to
 * configure for setting up the QSPI flash as slave device from Leo perspective.
 *        This file also has data structures related to clock, baudrate, macros
 * and all the relevant function prototypes.
 */

#ifndef _DW_APB_SSI_H_
#define _DW_APB_SSI_H_

#include <stdint.h>

#define DW_APB_SSI_TX_FIFO_SIZE (32)
#define DW_APB_SSI_RX_FIFO_SIZE (32)
#define FLASH_SUBSECTOR_SIZE (0x1000)
#define FLASH_PAGE_SIZE 256
#define FLASH_PAGE_SIZE_SHIFT 8

// DW_apb_ssi_ctrl_ro_t.SPI_FRF
#define DW_APB_SSI_CTRL_R0_STD_SPI_FRF (0)
#define DW_APB_SSI_CTRL_R0_DUAL_SPI_FRF (1)
#define DW_APB_SSI_CTRL_R0_QUAD_SPI_FRF (2)
#define DW_APB_SSI_CTRL_R0_OCTAL_SPI_FRF (3)

// DW_apb_ssi_ctrl_ro_t.FRF
#define DW_APB_SSI_CTRL_R0_MOTOROLA_SPI_FRF (0)
#define DW_APB_SSI_CTRL_R0_TEXAS_SSP_FRF (1)
#define DW_APB_SSI_CTRL_R0_NS_MICROWIRE_FRF (2)

// DW_apb_ssi_ctrl_ro_t.SCPOL
#define DW_APB_SSI_CTRL_R0_SCLK_LOW_SCPOL (0)
#define DW_APB_SSI_CTRL_R0_SCLK_HIGH_SCPOL (1)

// DW_apb_ssi_ctrl_ro_t.SCPH
#define DW_APB_SSI_CTRL_R0_SCPH_MIDDLE_SCPH (0)
#define DW_APB_SSI_CTRL_R0_SCPH_START_SCPH (1)

// DW_apb_ssi_ctrl_ro_t.TMOD
#define DW_APB_SSI_CTRL_R0_TX_AND_RX_TMOD (0x0)
#define DW_APB_SSI_CTRL_R0_TX_ONLY_TMOD (0x1)
#define DW_APB_SSI_CTRL_R0_RX_ONLY_TMOD (0x2)
#define DW_APB_SSI_CTRL_R0_EEPROM_RD_TMOD (0x3)

// DW_apb_ssi_ctrl_ro_t.DFS_32
#define DW_APB_SSI_CTRL_R0_8_BIT_FRAME_DFS_32 (0x7)
#define DW_APB_SSI_CTRL_R0_32_BIT_FRAME_DFS_32 (0x1F)

// Generic cfg reg
#define GENERIC_CFG_REG_0 (0x8008c)
#define GENERIC_CFG_REG_3 (GENERIC_CFG_REG_0 + (4 * 3))
#define GENERIC_CFG_REG_4 (GENERIC_CFG_REG_0 + (4 * 4))
#define GENERIC_CFG_REG_10 (GENERIC_CFG_REG_0 + (4 * 10))
#define GENERIC_CFG_REG_13 (GENERIC_CFG_REG_0 + (4 * 13))
#define GENERIC_CFG_REG_40 (GENERIC_CFG_REG_0 + (4 * 40))
#define GENERIC_CFG_REG_58 (GENERIC_CFG_REG_0 + (4 * 58))
#define GENERIC_CFG_REG_59 (GENERIC_CFG_REG_0 + (4 * 59))
#define GENERIC_CFG_REG_64 (GENERIC_CFG_REG_0 + (4 * 64))

typedef struct {
  union {
    struct {
      uint32_t DFS : 4;
      uint32_t FRF : 2;
      uint32_t SCPH : 1;
      uint32_t SCPOL : 1;
      uint32_t TMOD : 2;
      uint32_t SLV_OE : 1;
      uint32_t SRL : 1;
      uint32_t CFS : 4;
      uint32_t DFS_32 : 5;
      uint32_t SPI_FRF : 2;
      uint32_t RSVD_CTRLR0_23 : 1;
      uint32_t SSTE : 1;
      uint32_t SECONV : 1;
      uint32_t RSVD_CTRLRO : 6;
    };
    uint32_t word;
  };
} DW_apb_ssi_ctrl_ro_t;

typedef struct {
  union {
    struct {
      uint32_t NDF : 16;
      uint32_t RSVD_CTRLR1 : 16;
    };
    uint32_t word;
  };
} DW_apb_ssi_ctrl_r1_t;

typedef struct {
  union {
    struct {
      uint32_t SSI_EN : 1;
      uint32_t RSVD_SSIENR : 31;
    };
    uint32_t word;
  };
} DW_apb_ssi_ssienr_t;

typedef struct {
  union {
    struct {
      uint32_t MWMOD : 1;
      uint32_t MDD : 1;
      uint32_t MHS : 1;
      uint32_t RSVD_MWCR : 29;
    };
    uint32_t word;
  };
} DW_apb_ssi_mwcr_t;

typedef struct {
  union {
    struct {
      uint32_t SER : 1;
      uint32_t RSVD_SER : 31;
    };
    uint32_t word;
  };
} DW_apb_ssi_ser_t;

// DW_apb_ssi_baudr_t.SCKDV
#ifdef FW_DV_NO_SIM_SPEEDUP
#define DW_APB_SSI_BAUDR_APB_CLK_FREQ                                          \
  (700) // 10000 MHz.  Artificially bumped up clock speed for simulations.
#else
#define DW_APB_SSI_BAUDR_APB_CLK_FREQ                                          \
  (10000) // 10000 MHz.  Artificially bumped up clock speed for simulations.
#endif

#ifdef FW_DV_SSI_BAUDR_50_MHZ
#define DW_APB_SSI_BAUDR_APB_SPI_FREQ (50) // 50 MHz
#else
#define DW_APB_SSI_BAUDR_APB_SPI_FREQ (10) // 10 MHz
#endif
#define DW_APB_SSI_BAUDR_SCKDV                                                 \
  (DW_APB_SSI_BAUDR_APB_CLK_FREQ / DW_APB_SSI_BAUDR_APB_SPI_FREQ)
/* #define DW_APB_SSI_BAUDR_SCKDV       (2) */

typedef struct {
  union {
    struct {
      uint32_t SCKDV : 16; // SSI Clock Divider.
      uint32_t RSVD_BAUDR : 16;
    };
    uint32_t word;
  };
} DW_apb_ssi_baudr_t;

typedef struct {
  union {
    struct {
      uint32_t TFT : 5; // Transmit FIFO Threshold.
      uint32_t RSVD_TXFTLR : 27;
    };
    uint32_t word;
  };
} DW_apb_ssi_txftlr_t;

typedef struct {
  union {
    struct {
      uint32_t RFT : 5; // Receive FIFO Threshold.
      uint32_t RSVD_RXFTLR : 27;
    };
    uint32_t word;
  };
} DW_apb_ssi_rxftlr_t;

typedef struct {
  union {
    struct {
      uint32_t TXTFL : 6; // Transmit FIFO Threshold.
      uint32_t RSVD_TXFLR : 26;
    };
    uint32_t word;
  };
} DW_apb_ssi_txflr_t;

typedef struct {
  union {
    struct {
      uint32_t RXTFL : 5; // Receive FIFO Threshold.
      uint32_t RSVD_RXFLR : 27;
    };
    uint32_t word;
  };
} DW_apb_ssi_rxflr_t;

typedef struct {
  union {
    struct {
      uint32_t BUSY : 1; // SSI Busy Flag.
      uint32_t TFNF : 1; // Transmit FIFO Not Full.
      uint32_t TFE : 1;  // Transmit FIFO Empty.
      uint32_t RFNE : 1; // Receive FIFO Not Empty.
      uint32_t RFF : 1;  // Receive FIFO Full.
      uint32_t TXE : 1;  // Transmission Error.
      uint32_t DCOL : 1; // Data Collision Error.
      uint32_t RSVD_SR : 25;
    };
    uint32_t word;
  };
} DW_apb_ssi_sr_t;

typedef struct {
  union {
    struct {
      uint32_t TXEIM : 1; // Transmit FIFO Empty Interrupt Mask
      uint32_t TXOIM : 1; // Transmit FIFO Overflow Interrupt Mask
      uint32_t RXUIM : 1; // Receive FIFO Underflow Interrupt Mask
      uint32_t RXOIM : 1; // Receive FIFO Overflow Interrupt Mask
      uint32_t RXFIM : 1; // Receive FIFO Full Interrupt Mask
      uint32_t MSTM : 1;  // Multi-Master Contention Interrupt Mask.
      uint32_t RSVD_IMR : 26;
    };
    uint32_t word;
  };
} DW_apb_ssi_imr_t;

typedef struct {
  union {
    struct {
      uint32_t TXEIS : 1; // Transmit FIFO Empty Interrupt Status
      uint32_t TXOIS : 1; // Transmit FIFO Overflow Interrupt Status
      uint32_t RXUIS : 1; // Receive FIFO Underflow Interrupt Status
      uint32_t RXOIS : 1; // Receive FIFO Overflow Interrupt Status
      uint32_t RXFIS : 1; // Receive FIFO Full Interrupt Status
      uint32_t MSTS : 1;  // Multi-Master Contention Interrupt Status.
      uint32_t RSVD_ISR : 26;
    };
    uint32_t word;
  };
} DW_apb_ssi_isr_t;

typedef struct {
  union {
    struct {
      uint32_t TXEIR : 1; // Transmit FIFO Empty Raw Interrupt Status
      uint32_t TXOIR : 1; // Transmit FIFO Overflow Raw Interrupt Status
      uint32_t RXUIR : 1; // Receive FIFO Underflow Raw Interrupt Status
      uint32_t RXOIR : 1; // Receive FIFO Overflow Raw Interrupt Status
      uint32_t RXFIR : 1; // Receive FIFO Full Raw Interrupt Status
      uint32_t MSTR : 1;  // Multi-Master Contention Raw Interrupt Status.
      uint32_t RSVD_ISR : 26;
    };
    uint32_t word;
  };
} DW_apb_ssi_risr_t;

typedef struct {
  union {
    struct {
      uint32_t TXOICR : 1; // Clear Transmit FIFO Overflow Interrupt.
      uint32_t RSVD_TXOICR : 31;
    };
    uint32_t word;
  };
} DW_apb_ssi_txoicr_t;

typedef struct {
  union {
    struct {
      uint32_t RXOICR : 1; // Clear Receive FIFO Overflow Interrupt.
      uint32_t RSVD_RXOICR : 31;
    };
    uint32_t word;
  };
} DW_apb_ssi_rxoicr_t;

typedef struct {
  union {
    struct {
      uint32_t RXUICR : 1; // Clear Receive FIFO Underflow Interrupt.
      uint32_t RSVD_RXUICR : 31;
    };
    uint32_t word;
  };
} DW_apb_ssi_rxuicr_t;

typedef struct {
  union {
    struct {
      uint32_t MSTICR : 1; // Clear Multi-Master Contention Interrupt.
      uint32_t RSVD_MSTICR : 31;
    };
    uint32_t word;
  };
} DW_apb_ssi_msticr_t;

typedef struct {
  union {
    struct {
      uint32_t ICR : 1; // Clear Interrupts.
      uint32_t RSVD_ICR : 31;
    };
    uint32_t word;
  };
} DW_apb_ssi_icr_t;

typedef struct {
  union {
    struct {
      uint32_t IDCODE;
    };
    uint32_t word;
  };
} DW_apb_ssi_idr_t;

typedef struct {
  union {
    struct {
      uint32_t SSI_COMP_VERSION;
    };
    uint32_t word;
  };
} DW_apb_ssi_version_id_t;

typedef struct {
  union {
    struct {
      uint32_t DR; // Data Register.
    };
    uint32_t word;
  };
} DW_apb_ssi_drx_t;

typedef struct {
  union {
    struct {
      uint32_t RSD : 8; // Rxd Sample Delay.
      uint32_t RSVD_RX_SAMPLE_DLY : 24;
    };
    uint32_t word;
  };
} DW_apb_ssi_rx_sample_dly_t;

// DW_apb_ssi_spi_ctrl_ro_t.INST_L
#define DW_APB_SSI_SPI_CTRL_R0_INST_L_0 (0) // 0-bit (No Instruction)
#define DW_APB_SSI_SPI_CTRL_R0_INST_L_1 (1) // 4-bit Instruction
#define DW_APB_SSI_SPI_CTRL_R0_INST_L_2 (2) // 8-bit Instruction
#define DW_APB_SSI_SPI_CTRL_R0_INST_L_3 (3) // 16-bit Instruction

// DW_apb_ssi_spi_ctrl_ro_t.TRANS_TYPE
// Instruction and Address will be sent in Standard SPI Mode.
#define DW_APB_SSI_SPI_CTRL_R0_STD_TRANS_TYPE (0x0)
// Instruction will be sent in Standard SPI Mode and
// Address will be sent in the mode specified by
// CTRLR0.SPI_FRF.
#define DW_APB_SSI_SPI_CTRL_R0_INS_STD_ADDR_SPI_FRF_TRANS_TYPE (0x1)
// Both Instruction and Address will be sent in the mode
// specified by SPI_FRF.
#define DW_APB_SSI_SPI_CTRL_R0_INS_SPI_FRF_ADDR_SPI_FRF_TRANS_TYPE (0x2)

#define DW_APB_SSI_SPI_CTRL_R0_16_BIT_ADDR_L                                   \
  (0x4) // (ADDR_L_4): 16-bit Address Width
#define DW_APB_SSI_SPI_CTRL_R0_20_BIT_ADDR_L                                   \
  (0x5) // (ADDR_L_4): 20-bit Address Width
#define DW_APB_SSI_SPI_CTRL_R0_24_BIT_ADDR_L                                   \
  (0x6) // (ADDR_L_4): 24-bit Address Width

typedef struct {
  union {
    struct {
      uint32_t TRANS_TYPE : 2;
      uint32_t ADDR_L : 4;
      uint32_t RSVD_SPI_CTRLRO_6_7 : 2;
      uint32_t INST_L : 2;
      uint32_t RSVD_SPI_CTRLRO_10 : 1;
      uint32_t WAIT_CYCLES : 5;
      uint32_t SPI_DDR_EN : 1;
      uint32_t INST_DDR_EN : 1;
      uint32_t SPI_RXDS_EN : 1;
      uint32_t RSVD_SPI_CTRLR0 : 13;
    };
    uint32_t word;
  };
} DW_apb_ssi_spi_ctrl_ro_t;

typedef struct {
  union {
    struct {
      uint32_t TDE : 2;
      uint32_t RSVD_TXD_DRIVE_EDGE : 30;
    };
    uint32_t word;
  };
} DW_apb_ssi_txd_drive_edge_t;

typedef struct {
  union {
    struct {
      uint32_t RSVD; // RSVD 31to0 Reserved address location
    };
    uint32_t word;
  };
} DW_apb_ssi_rsvd_t;

typedef struct {
  DW_apb_ssi_ctrl_ro_t CTRLR0; // 0x0
  DW_apb_ssi_ctrl_r1_t CTRLR1; // 0x4
  DW_apb_ssi_ssienr_t SSIENR;  // 0x8
  DW_apb_ssi_mwcr_t MWCR;      // 0xC
  DW_apb_ssi_ser_t SER;        // 0x10
  DW_apb_ssi_baudr_t BAUDR;    // 0x14
  DW_apb_ssi_txftlr_t TXFTLR;  // 0x18
  DW_apb_ssi_rxftlr_t RXFTLR;  // 0x1C
  DW_apb_ssi_txflr_t TXFLR;    // 0x20
  DW_apb_ssi_rxflr_t RXFLR;    // 0x24
  DW_apb_ssi_sr_t SR;          // 0x28
  DW_apb_ssi_imr_t IMR;        // 0x2C
  DW_apb_ssi_isr_t ISR;        // 0x30
  DW_apb_ssi_risr_t RISR;      // 0x34
  DW_apb_ssi_txoicr_t TXOICR;  // 0x38
  DW_apb_ssi_rxoicr_t RXOICR;  // 0x3C
  DW_apb_ssi_rxuicr_t RXUICR;  // 0x40
  DW_apb_ssi_msticr_t MSTICR;  // 0x44
  DW_apb_ssi_icr_t ICR;        // 0x48
  uint32_t rsvd[3];     // 0x4c - DMA control registers.  TODO: Confirm if its
                        // enabled in LEO and add definitions.
  DW_apb_ssi_idr_t IDR; // 0x58
  DW_apb_ssi_version_id_t VERSION_ID;         // 0x5C
  DW_apb_ssi_drx_t DRx[36];                   // 0x60
  DW_apb_ssi_rx_sample_dly_t RX_SAMPLE_DLY;   // 0xF0
  DW_apb_ssi_spi_ctrl_ro_t SPI_CTRLRO;        // 0xF4
  DW_apb_ssi_txd_drive_edge_t TXD_DRIVE_EDGE; // 0xF8
  DW_apb_ssi_rsvd_t RSVD;                     // 0xFC
} DW_apb_ssi_mem_map_t;

typedef struct {
  union {
    struct {
      uint32_t addr : 24;
      uint32_t op_code : 8;
    };
    uint32_t word;
  };
} DW_apb_ssi_micron_flash_cmd_t;

#endif

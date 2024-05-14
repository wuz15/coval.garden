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
 * @file leo_spi.h
 * @brief Definition of SPI types for the SDK.
 */

#ifndef ASTERA_LEO_SDK_SPI_H_
#define ASTERA_LEO_SDK_SPI_H_

#include "DW_apb_ssi.h"
#include "leo_api_types.h"
#include "leo_error.h"
#include "leo_globals.h"
//#include "misc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_FLASH_SIZE (8 * 1024 * 1024)
#define LEO_SPI_FLASH_HEADER_BYTE_CNT (0x24)

typedef enum {
    BT_MAIN_e = 0x01,
    BT_SYSTEM_CONFIG_e = 0x04,
    BT_PERSISTENT_DATA_e = 0x06,
    BT_DESCRIPTION_e = 0x07,
    BT_TOC_e = 0x86,
    BT_FLASH_CTRL_e = 0x87,
    BT_FLASH_RSVD_e = 0xdd,
    BT_END_e = 0xff
} BLOCKTYPE;

enum {
    SPI_DEVICE_SST26WF064C_e,           // 0 Microchip 64Mbit
    SPI_DEVICE_MX25U6432_e,             // 1 Macronix  64Mbit
    SPI_DEVICE_EOL_e
};

typedef struct block_info {
    uint32_t header[2];
    uint32_t type;
    uint32_t subtype;
    uint32_t version;
    uint32_t length;
    uint32_t length_copy;
    uint32_t address;
    uint32_t config_data[2];
    uint32_t crc;
    uint32_t trailer[2];
    uint32_t start_addr;
    uint32_t end_addr;
} block_info_t;

typedef struct toc_code_data {
    uint32_t slot_id;
    uint32_t img_version;
    uint32_t img_version_copy;
    uint32_t config;             // is primary bits 24 and 16, is valid bits 8 and 0
    uint32_t buffer;
    uint32_t addr_pointer;
    uint32_t addr_pointer_copy;
} toc_code_data_t;

typedef struct toc_syscfg_data {
    uint32_t slot_id;
    uint32_t valid;
    uint32_t buffer;
    uint32_t addr_pointer;
    uint32_t addr_pointer_copy;
    uint32_t code_slot_correlation;
    uint32_t load_priority;
} toc_syscfg_data_t;

typedef struct toc_config {
    uint32_t rsvd[7];
    uint32_t num_syscfg_entries;
} toc_config_t;

typedef struct toc_data {
    toc_code_data_t code_data[3];
    toc_config_t toc_config;
    toc_syscfg_data_t syscfg_data[3];
} toc_data_t;

typedef struct {
  uint32_t rsvd_1[6];
  uint32_t asic_version;
  uint32_t rsvd_2;
  uint32_t build_id;
  uint32_t builder;
  uint32_t fw_version;
  uint32_t rsvd_3[3];
} LeoSpiDescriptionEntryType;

typedef struct {
  LeoSpiDescriptionEntryType entries[3];
} LeoSpiDescriptionBlockDataType;

enum {
  FLASH_BOOT_LOAD_STATUS_OK = 0,
  FLASH_BOOT_LOAD_READ_DW_STATUS_NO_CMD_ACTIVE = 0x0001,
  FLASH_BOOT_LOAD_DW_STATUS_TX_FIFO_OVERFLOW = 0x0002,
  FLASH_BOOT_LOAD_DW_STATUS_RX_FIFO_UNDERFLOW = 0x0004,
  FLASH_BOOT_LOAD_READ_DW_STATUS_RX_FIFO_OVERFLOW = 0x0008,
  FLASH_BOOT_LOAD_ERROR_TIMEOUT = 0x0010,
  FLASH_BOOT_LOAD_ERROR_STILL_BUSY = 0x0020,
  FLASH_BOOT_LOAD_ERROR_ADDR_MIS_MATCH = 0x0040

};

typedef struct {
  DW_apb_ssi_baudr_t dw_apb_ssi_baudr;
  DW_apb_ssi_ctrl_ro_t dw_apb_ssi_ctrl_ro;
  DW_apb_ssi_ctrl_r1_t dw_apb_ssi_ctrl_r1;
  DW_apb_ssi_rx_sample_dly_t dw_apb_ssi_rx_sample_dly;

  // XXX added here just in case
  DW_apb_ssi_spi_ctrl_ro_t dw_apb_ssi_spi_ctrl_ro;

  // uint32_t controller_sts;
  uint32_t timeout_loop_cnt;

  uint32_t curr_read_addr;

  uint32_t status;

} flash_boot_load_ctrl_info_t;

/**
 * @brief Low-level SPI Initialization and setup.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * This function has to be called before any other SPI function.
 * Initialize and Open the SPI driver, setup the clocks, Pin signals,
 * baud rate, and other SPI parameters
 *
 */
void asteraSPISetup(AsteraProductType product);

/**
 * @brief Low-level SPI Initialization and setup.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * This function is called to close the SPI device.
 * @param[in] ProductType (Leo)
 */
void asteraSPIClose(AsteraProductType product);

void asteraSPIReset(AsteraProductType product);
/**
 * @brief Low-level SPI API to completely bulk erase the flash chip
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * This function erases the entire flash chip.
 *
 */
void asteraSPIChipBulkErase(AsteraProductType product);

/**
 * @brief Low-level block erase SPI function.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * This function erases one or more blocks of memory specified by the address.
 *
 * @param block_addr : Address of the block to erase
 * @param block_count : Number of blocks to erase
 */
void asteraSPIBlockErase(AsteraProductType product, uint32_t block_addr,
                         uint32_t block_count);

/**
 * @brief Low-level SPI Read.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * reads one byte from the SPI bus at the address specified
 *
 * @param addr : Address of the byte to read from
 */
unsigned char asteraSPIRead(AsteraProductType product, uint32_t addr);

/**
 * @brief Low-level SPI Read Block.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * reads a block of bytes into the given array from the SPI bus at the address
 * specified
 *
 * @param addr : address to read from
 * @param data : pointer to the array to store the data that was read
 * @param size : number of bytes to read
 */
void asteraSPIReadBlock(AsteraProductType product, uint32_t addr, uint8_t *data,
                        uint32_t len);
/**
 * @brief Low-level SPI Read Page.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * Reads the number of pages desired from SPI bus at the address specified
 *
 * @param addr : Address of the byte to read from
 * @param data : pointer to the array to store the data that was read
 * @param size : number of bytes to read
 */
void asteraSPIReadPage(AsteraProductType product, uint32_t addr, uint8_t *data,
                       uint32_t len);

/**
 * @brief Low-level SPI Write.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * writes a sequence of bytes to the SPI bus at the address specified
 * the flash cells are not autmatically erased, it is important to clear the
 * flash cells by calling the erase function before writing to the flash
 *
 * @param addr : The position in the flash to write the first byte
 * @param data : The data to write to the flash
 * @param len : The number of bytes to write
 */
void asteraSPIWrite(AsteraProductType product, uint32_t addr, uint8_t *data,
                    uint32_t len);

/**
 * @brief Low-level SPI Write Block.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * writes a block of bytes to the SPI bus at the address specified
 *
 * @param addr : The position in the flash to write the first byte
 * @param data : The data to write to the flash
 * @param len : The number of bytes to write
 */
void asteraSPIWriteBlock(AsteraProductType product, uint32_t addr,
                         uint8_t *data, uint32_t len);

/**
 * @brief Low-level SPI Write Page.
 * THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 * Writes the number of pages desired to the SPI bus at the address specified
 *
 * @param addr : The position in the flash to write the first byte
 * @param data : The data to write to the flash
 * @param len : The number of bytes to write
 */
void asteraSPIWritePage(AsteraProductType product, uint32_t addr, uint8_t *data,
                        uint32_t len);

/**
 * @brief Low-level code to initialize the DW APB
 * @param leoDriver: pointer to the Leo driver
 */
void leoSpiInit(LeoI2CDriverType *leoDriver);

/**
 * @brief low-level code to enable/disable the DW APB SSI
 *
 * @param leoDriver  pointer to the Leo driver
 * @param enable: 1 to enable, 0 to disable
 */
void dw_apb_ssi_SSIENR(LeoI2CDriverType *leoDriver, uint32_t enable);

/**
 * @brief low-level code to select/not select slave device by
 * using the Slave Enable Register in DW APB SSI
 * Valid only when DW APB SSI is configured as a master device.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param enable: 1 means slave is selected, 0 means no slave is selected
 */
void dw_apb_ssi_SER(LeoI2CDriverType *leoDriver, uint32_t enable);

/**
 * @brief low-level code to control the serial data transfer
 * Cannot write into this register while DW APB SSI is enabled
 * Valid only when DW APB SSI is configured as a master device.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param CTRLR0_val: SPI Frame format, Data & Control Frame Size, transfer
 * mode, Serial Clock Polarity, Serial Clock Phase.
 */
void dw_apb_ssi_CTRLR0(LeoI2CDriverType *leoDriver, uint32_t CTRLR0_val);

/**
 * @brief low-level code to control the end of serial data transfers when in Rx
 * mode. Cannot write into this register while DW APB SSI is enabled Valid only
 * when DW APB SSI is configured as a master device. The DW_apb_ssi continues to
 * rx serial data until the number of data frames received is equal to this
 * register value plus 1.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param CTRLR1_val: Number of data frames in a transfer
 *
 */
void dw_apb_ssi_CTRLR1(LeoI2CDriverType *leoDriver, uint32_t CTRLR1_val);

/**
 * @brief low-level code to check Rx FIFO level register
 * read the register to get the number of valid data entries in the Rx FIFO
 * memory.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: number of valid data entries in the Rx FIFO memory
 */
void dw_apb_ssi_RXFLR(LeoI2CDriverType *leoDriver, uint32_t *value);

/**
 * @brief low-level code to check Tx FIFO level register
 * read the register to get the number of valid data entries in the Tx FIFO
 * memory.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: number of valid data entries in the Transmit FIFO memory
 */
void dw_apb_ssi_TXFLR(LeoI2CDriverType *leoDriver, uint32_t *value);

/**
 * @brief low-level code to read buffer from the transmit/receive FIFOs.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: value read from the transmit/receive FIFOs
 */
void dw_apb_ssi_DRx_read(LeoI2CDriverType *leoDriver, uint32_t *value);

/**
 * @brief low-level code to read (16b) buffer from the transmit/receive FIFOs.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: value read (16b) from the transmit/receive FIFOs
 */
void dw_apb_ssi_DRx_read_16_dw(LeoI2CDriverType *leoDriver, uint32_t *values);

/**
 * @brief low-level code to write buffer to the transmit/receive FIFOs.
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: value to write to the transmit/receive FIFOs
 */
void dw_apb_ssi_DRx(LeoI2CDriverType *leoDriver, uint32_t DRx_val);

/**
 * @brief low-level code to enable/disable the flash write block protection
 *
 * @param leoDriver  pointer to the Leo driver
 * @param spi_device type of spi device - some don't support write block protect command
 * @param value: 1 to enable, 0 to disable
 */
void leo_spi_flash_write_block_protect(LeoI2CDriverType *leoDriver,
                                       int spi_device,
                                       uint8_t block_protect_en);

/**
 * @brief low-level code to read flash status register
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: value read from the flash status register
 */
void flash_read_sts_reg(LeoI2CDriverType *leoDriver, uint32_t *value);

/**
 * @brief low-level code to check flash write in progress bit
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: value determines if flash write is in progress
 */
void flash_check_write_in_prog(LeoI2CDriverType *leoDriver, uint32_t *value);

/**
 * @brief low-level code to enable flash write
 *
 * @param leoDriver  pointer to the Leo driver
 */
void flash_write_enable(LeoI2CDriverType *leoDriver);

/**
 * @brief low-level code to erase flash subsectors (4KB)
 *
 * @param leoDriver  pointer to the Leo driver
 * @param addr: address of the flash subsector to be erased
 */
void flash_subsector_erase(LeoI2CDriverType *leoDriver, uint32_t addr, int spiDevice, int protect);

void flash_block_erase(LeoI2CDriverType *leoDriver, uint32_t addr, int spiDdevice, int protect);

LeoErrorType flash_erase_range(
    LeoDeviceType *leoDevice,
    uint32_t      mem_min,
    uint32_t      mem_max
    );

/**
 * @brief low-level code to write words to flash
 *
 * @param leoDriver  pointer to the Leo driver
 * @param start_addr: start address of the flash to be written
 * @param num_words: number of words to be written
 * @param word_arr: pointer to the data to be written
 * @return LeoErrorType
 */
LeoErrorType flash_write(LeoI2CDriverType *leoDriver, uint32_t start_addr,
                       size_t num_words_to_write, uint32_t *word_arr);

/**
 * @brief low-level code to read words from flash
 *
 * @param leoDriver  pointer to the Leo driver
 * @param start_addr: start address of the flash to be read
 * @param num_words: number of words to be read
 * @param word_arr: pointer to the data to be read
 * @return LeoErrorType
 */
LeoErrorType flash_read(LeoI2CDriverType *leoDriver, uint32_t start_addr,
                      size_t num_words_to_read, uint32_t *values);

/**
 * @brief low-level code to erase entire flash chip. The Chip-Erase instruction
 * clears all bits in the device to ‘F’
 * The Chip-Erase instruction is ignored if any of the memory area is protected.
 * Prior to any write operation, it is required to execute the
 * flash_write_enable().
 *
 * @param leoDriver  pointer to the Leo driver
 */
void leo_spi_flash_bulk_erase(LeoI2CDriverType *leoDriver);

/**
 * @brief low-level code to read the unique JEDEC ID of the flash chip
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: value read (JEDEC ID)
 */
void flash_read_jedec(LeoI2CDriverType *leoDriver, uint32_t *value);

/**
 * @brief low-level code to read the unique version of the FW
 *
 * @param leoDriver  pointer to the Leo driver
 * @param value: value read (FW version)
 */
void read_fw_version(LeoI2CDriverType *leoDriver, uint32_t *values);
LeoErrorType flash_verify_block_crc(LeoI2CDriverType *leoDriver, uint32_t startAddr,
                           size_t lenDWords, uint32_t crc);

/**
 * @brief program Leo SPI with the new flash memory
 *
 * @param[in] leoDevice         pointer to the device
 * @param[in] flashFileName     filepath name
 * @param[in] persistentDataBuf data to be written into persistent data block
 */
LeoErrorType leo_spi_program_flash(LeoDeviceType *leoDevice,
                                   const char *filename);


LeoErrorType leo_spi_update_target(LeoDeviceType *device, 
                                   char *filename, 
                                   int target,
                                   int verify);

LeoErrorType leo_spi_verify_crc(LeoDeviceType *device, char *filename);

LeoErrorType leoSpiCheckCompatibility(LeoDeviceType *device, uint8_t *fwBuf);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_LEO_SDK_SPI_H_ */

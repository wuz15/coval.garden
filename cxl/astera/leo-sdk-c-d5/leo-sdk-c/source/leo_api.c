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
 * @file leo_api.c
 * @brief Implementation of public functions for the SDK.
 */
#include "../include/leo_api.h"
#include "../include/leo_api_internal.h"
#include "../include/hal.h"
#include "../include/DWC_pcie_dbi_mbar0.h"
#include "../include/misc.h"


#include <inttypes.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Return the SDK version
 */
const uint8_t *leoGetSDKVersion(void) { return (LEO_SDK_VERSION); }

LeoErrorType leoFWStatusCheck(LeoDeviceType *device) {
  uint32_t read_word;

  // FIXME_FUTURE ASSERT leoDevice->i2cDriver
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_MUC_GENERIC_CFG_REG_ADDRESS + LEO_FW_BUILD_OFFSET, &read_word);
#ifdef LEO_CSDK_DEBUG
  ASTERA_DEBUG("Leo:CSDK:DEBUG:Leo Boot Stat:%08x", read_word);
#endif
  return (read_word > LEO_BOOT_STAT_FW_RUNNING) ? LEO_SUCCESS
                                                : LEO_FW_NOT_RUNNING;
}

/*
 * Get Version of the FW currently running
 */
LeoErrorType leoGetFWVersion(LeoDeviceType *device) {
  uint32_t readWord;
  LeoErrorType rc = 0;
  rc += leoReadWordData(device->i2cDriver, LEO_TOP_CSR_MUC_GENERIC_CFG_REG_ADDRESS + LEO_FW_VERSION_OFFSET, &readWord);
  device->fwVersion.minor = readWord & 0xff;
  device->fwVersion.major = (readWord >> 8) & 0xff;
  rc += leoReadWordData(device->i2cDriver, LEO_TOP_CSR_MUC_GENERIC_CFG_REG_ADDRESS + LEO_FW_BUILD_OFFSET, &readWord);
  device->fwVersion.build = readWord;
#ifdef LEO_CSDK_DEBUG
  ASTERA_DEBUG("Leo:CSDK:DEBUG:Leo FW Version:%02d.%02d, Build: %08ld",
               device->fwVersion.major, device->fwVersion.minor,
               device->fwVersion.build);
#endif
  return rc;
}

/*
 * Get Version of the ASIC currently running
 */
LeoErrorType leoGetAsicVersion(LeoDeviceType *device) {
  uint32_t readWord;
  LeoErrorType rc = 0;
  rc += leoReadWordData(device->i2cDriver, LEO_TOP_CSR_MISC_LEO_MISC_ADDRESS, &readWord);
  device->asicVersion.minor = ((readWord & 0xff) == 0xa0) ? LEO_ASIC_A0 : LEO_ASIC_D5;
  device->asicVersion.major = 0x1;
#ifdef LEO_CSDK_DEBUG
  ASTERA_DEBUG("Leo:CSDK:DEBUG:Leo ASIC Version:%02d.%02d",
               device->asicVersion.major, device->asicVersion.minor)
#endif
  return rc;
}

/*
 * Compare fw and sdk versions. Return 0 if same, 1 if fw version is greater
 * than min sdk version that is supported and compatible, -1 if fw version is
 * less than min sdk version supported and incompatible
 */

int leoSDKVersionCompatibilityCheck(char *fwVersion, char *sdkVersion) {
  /* store only numeric portion of the version number*/
  int fwN = 0, sdkNum = 0;

  for (int i = 0, j = 0; (i < strlen(fwVersion) || j < strlen(sdkVersion));) {
    while (i < strlen(fwVersion) && fwVersion[i] != '.') {
      fwN = fwN * 10 + (fwVersion[i] - '0');
      i++;
    }
    while (j < strlen(sdkVersion) && sdkVersion[j] != '.') {
      sdkNum = sdkNum * 10 + (sdkVersion[j] - '0');
      j++;
    }

    if (fwN > sdkNum)
      return 1;
    if (sdkNum > fwN)
      return -1;

    fwN = sdkNum = 0; /* reset if same and proceed further*/
    i++;
    j++;
  }
  return 0;
}

LeoErrorType leoInitDevice(LeoDeviceType *device) {
  /* nothing to be done in this release */
  /* no device initialization as the release of firmware is plug and play */
  // Set lock = 0 (if it hasnt been set before)
  if (device->i2cDriver->lockInit == 0)
  {
      device->i2cDriver->lock = 0;
      device->i2cDriver->lockInit = 1;
  }
  
  return 0;
}

LeoErrorType leoFwUpdateFromFile(LeoDeviceType *device, char *flashFileName) {
  LeoErrorType rc;
  uint32_t jedecID;
  int spiDevice;
  int ii;
  int mb_sts;

  rc = leoGetFWVersion(device);

  // Store persistent data to re-write after erasing if using FW0.6 or newer
  if (0 == device->ignorePersistentDataFlag && (device->fwVersion.major > 0 || device->fwVersion.minor >= 6)) {
    ASTERA_INFO("Reading persistent data");
    uint32_t dataIn[16];
    uint32_t dataOut[16];
    mb_sts = execOperation(device->i2cDriver, 0, FW_API_MMB_CMD_OPCODE_MMB_PING,
                           dataOut, 0, dataIn, 1);
    if (mb_sts != AL_MM_STS_SUCCESS) {
      ASTERA_ERROR("Failed to communicate with Mailbox, unable to read persistent data");
      return LEO_FAILURE;
    }
  } else {
    ASTERA_INFO("No persistent data found, overwriting block.");
    device->ignorePersistentDataFlag = 1;
  }

  // Initialize Leo tunnelling
  leoSpiInit(device->i2cDriver);

  // Read JEDEC ID
  flash_read_jedec(device->i2cDriver, &jedecID);
  switch(jedecID) {
  case 0xbf2653:
      spiDevice = SPI_DEVICE_SST26WF064C_e;
      printf("SST 64Mb\n");
      break;
  case 0xc22537:
      spiDevice = SPI_DEVICE_MX25U6432_e;
      printf("MX25U6432F\n");
      break;
  default:
      printf("Undecoded device %06x\n", jedecID);
      return LEO_FAILURE;
      break;
  }

  device->spiDevice = spiDevice;

  // Program and verify the flash
  rc = leo_spi_program_flash(device, flashFileName);

  return rc;
}

LeoErrorType leoFwUpdateTarget(LeoDeviceType *device, char *flashFileName, int target, int verify) {

  LeoErrorType rc;
  uint32_t jedecID;
  int spiDevice;

  // Initialize Leo tunnelling
  leoSpiInit(device->i2cDriver);

  // Read JEDEC ID
  flash_read_jedec(device->i2cDriver, &jedecID);
  switch(jedecID) {
  case 0xbf2653:
      spiDevice = SPI_DEVICE_SST26WF064C_e;
      printf("SST 64Mb\n");
      break;
  case 0xc22537:
      spiDevice = SPI_DEVICE_MX25U6432_e;
      printf("MX25U6432F\n");
      break;
  default:
      printf("Undecoded device %06x\n", jedecID);
      return LEO_FAILURE;
      break;
  }

  device->spiDevice = spiDevice;

  // Program and verify the flash
  rc = leo_spi_update_target(device, flashFileName, target, verify);
  return rc;
}

LeoErrorType leoDoFwUpdateinLTModeRaw(LeoDeviceType *device,
                                      LeoFWImageFormatType fwImageFormatType,
                                      const uint8_t *fwImageBuffer) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

//TODO YW: function not working - FIXME
// LeoErrorType leoLoadCurrentFW(LeoDeviceType *device) {
//   uint32_t read_word;
//   /* nothing to be done in this release */
//   /* no loading selection FW as there is no ABG */
//   leoReadWordData(device->i2cDriver, LEO_MISC_RST_ADDR, &read_word);
//   // check that LEO_MUC_RST_MASK is not set - FIXME
//   leoWriteWordData(device->i2cDriver, LEO_MISC_RST_ADDR,
//                    (read_word | LEO_MUC_RST_MASK));
//   leoReadWordData(device->i2cDriver, LEO_MISC_RST_ADDR, &read_word);
//   // check that LEO_MUC_RST_MASK is set - FIXME
//   leoWriteWordData(device->i2cDriver, LEO_MISC_RST_ADDR,
//                    (read_word & (~(LEO_MUC_RST_MASK))));
//   return leoGetFWVersion(device);
// }

LeoErrorType leoResetAsic(LeoDeviceType *device) {
  // Not supported as of this point - Cannot assume SVB here
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

LeoErrorType leoGetLinkStatus(LeoDeviceType *device,
                              LeoLinkStatusType *linkStatus) {
  int i;
  uint32_t read_word;
  uint32_t write_word;
  /* nothing to be done in this release */
  /* no loading selection FW as there is no ABG */
  leoReadWordData(device->i2cDriver,
                  device->controllerIndex ? csr_cxl_mbar0_1_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG
                                          : csr_cxl_mbar0_0_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG,
                  &read_word);
  linkStatus->linkSpeed = (read_word >> 16) & 0xf;
  linkStatus->linkWidth = (read_word >> 20) & 0x1f;
  linkStatus->linkMode = 0; // FW does not support any other thing as of now
  linkStatus->linkStatus = (read_word >> 27) & 0x1;
#ifdef LEO_CSDK_DEBUG
  ASTERA_DEBUG("Leo:CSDK:DEBUG:Leo Link Status:Speed=Gen%d, Width=x%d, "
               "Status=%s, Mode=%s",
               linkStatus->linkSpeed, linkStatus->linkWidth,
               linkStatus->linkStatus ? "InTraining Mode" : "Active",
               linkStatus->linkMode ? "PCIE" : "CXL");
#endif

  for (i = 0; i < linkStatus->linkWidth; ++i) {
    write_word = ((linkStatus->linkSpeed - 3) << 4) + i;
    leoWriteWordData(device->i2cDriver,
                     device->controllerIndex ? LEO_CXL1_EQ_CONTROL1_ADDR
                                             : LEO_CXL0_EQ_CONTROL1_ADDR,
                     write_word);
    leoReadWordData(device->i2cDriver,
                    device->controllerIndex ? LEO_CXL1_EQ_STATUS2_ADDR
                                            : LEO_CXL0_EQ_STATUS2_ADDR,
                    &read_word);
    linkStatus->linkFoM[i] = (read_word >> 24) & 0xff;
#ifdef LEO_CSDK_DEBUG
    ASTERA_DEBUG("Leo:CSDK:DEBUG:Leo Link FOM:Lane=%d, FOM=0x%08x", i,
                 linkStatus->linkFoM[i]);
#endif
  }
  return LEO_SUCCESS;
}

/*
 * @brief check Memory channel status
 * @param[in] Leo Device pointer
 * @param[in] pointer to memStatus
 * @return    LeoErrorType - Leo error code
 */
LeoErrorType leoGetMemoryStatus(LeoDeviceType *device,
                                LeoMemoryStatusType *memStatus) {
  int i;
  uint32_t read_word;
  /* nothing to be done in this release */
  for (i = 0; i < 2; ++i) {
    leoReadWordData(device->i2cDriver,
                    i ? LEO_TOP_CSR_MUC_MAIN_DDR1_CURR_STATE_ADDRESS
                      : LEO_TOP_CSR_MUC_MAIN_DDR0_CURR_STATE_ADDRESS,
                    &read_word);
    if ((read_word & 0xf000) == 0x1000) {
      memStatus->ch[i].initDone = 1;
      memStatus->ch[i].trainingDone = 1;
    }
    // 0x2000 = rcd fail, 0x3000 = training fail
    if ((read_word & 0x2000) == 0x2000) {
      memStatus->ch[i].trainingDone = 0;
    }
  }
  return LEO_SUCCESS;
}

LeoErrorType leoMemoryScrubbing(LeoDeviceType *device,
                                LeoScrubConfigInfoType *scrubConfig) {

  if (scrubConfig->reqScrbEn) { // enable Request Scrb
    ASTERA_INFO("Leo:CSDK:INFO:Starting Leo Request Scrubbing");
    configReqScrb(device->i2cDriver, scrubConfig->reqScrbEn,
                  scrubConfig->ReqScrbDpaStartAddr,
                  scrubConfig->ReqScrbDpaEndAddr,
                  scrubConfig->ReqScrbCmdInterval);
    wait4ReqScrbDone(device->i2cDriver);
    readScrbStatus(device);
  } else if (scrubConfig->bkgrdScrbEn) {
    ASTERA_INFO("Leo:CSDK:INFO:Starting Leo background or patrol scrubbing");
    configBkgrdScrb(
        device->i2cDriver, scrubConfig->bkgrdScrbEn,
        scrubConfig->bkgrdDpaEndAddr, scrubConfig->bkgrdRoundInterval,
        scrubConfig->bkgrdCmdInterval, scrubConfig->bkgrdWrPoisonIfUncorrErr);
    ASTERA_INFO("Leo:CSDK:INFO:Waiting for %d secs to request status",
                scrubConfig->bkgrdScrbTimeout);
    sleep(scrubConfig->bkgrdScrbTimeout);
    readScrbStatus(device);
  }
  if (scrubConfig->onDemandScrbEn) {
    ASTERA_INFO("Leo:CSDK:INFO:Starting Leo OnDemand Scrubbing");
    configOnDemandScrb(device->i2cDriver, scrubConfig->onDemandScrbEn,
                       scrubConfig->onDemandWrPoisonIfUncorrErr);
    readScrbStatus(device);
  }
  return LEO_SUCCESS;
}

#ifndef ES0
/*
 * @brief Initialize the TSSM (Pcie Link Training and Status Machine) Logging
 * @param[out]
 * @param[in] oneBatchMode - 0: Continuous logging, 1: One batch logging
 * @param[in] verbosity
 * @return    LeoErrorType - Leo error code
 */

LeoErrorType leoLTSSMLoggerInit(LeoLinkType *link, uint8_t oneBatchMode,
                                LeoLTSSMVerbosityType verbosity) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

/**
 * @brief Enable or disable LTSSM logger.
 *
 * @param[in]  link     Pointer to Leo Link struct object
 * @param[in]  printEn  Enable (1) or disable (0) printing for this Link
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoLTSSMLoggerPrintEn(LeoLinkType *link, uint8_t printEn) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

/**
 * @brief Perform an all-in-one health check on the device.
 *
 * Check if regular accesses to the EEPROM are working, and if the EEPROM has
 * loaded correctly (i.e. Main Mircocode, Path Microcode and PMA code have
 * loaded correctly).
 *
 * @param[in]  device    Pointer to Leo Device struct object
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoCheckDeviceHealth(LeoDeviceType *device) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

/**
 * @brief Get the current detailed Link state, including electrical parameters.
 *
 * Read the current Link state and return the parameters for the current links
 * Returns a negative error code, else zero on success.
 *
 * @param[in,out]  link   Pointer to Leo Link struct object
 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoGetLinkStateDetailed(LeoLinkType *link) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

/**
 * @brief Read an entry from the LTSSM logger.
 *
 * The LTSSM log entry starting at offset is returned, and offset
 * is updated to point to the next entry. Returns a negative error code,
 * including LEO_LTSSM_INVALID_ENTRY if the the end of the log is reached,
 * else zero on success.
 *
 * @param[in]      link    Pointer to Leo Link struct object
 * @param[in]      log     The specific log to read from
 * @param[in,out]  offset  Pointer to the log offset value
 * @param[out]     entry   Pointer to Leo LTSSM Logger Entry struct returned
 *                         by this function
 * @return         LeoErrorType - Leo error code
 */
LeoErrorType leoLTSSMLoggerReadEntry(LeoLinkType *link,
                                     LeoLTSSMLoggerEnumType log, int *offset,
                                     LeoLTSSMEntryType *entry) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

/**
 * @brief Get the format ID at offset int he print buffer
 *
 * @param[in]  link         Link struct created by user
 * @param[in]  loggerType   Logger type (main or which path)
 * @param[in]  offset       Print buffer offset
 * @param[in/out]  fmtID    Format ID from logger
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoGetLoggerFmtID(LeoLinkType *link,
                               LeoLTSSMLoggerEnumType loggerType, int offset,
                               int *fmtID) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

/**
 * @brief Get the write pointer location in the LTSSM logger
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  writeOffset    Current write offset
 * @return     LeoErrorType - Leo error code
 *
 */
LeoErrorType leoGetLoggerWriteOffset(LeoLinkType *link,
                                     LeoLTSSMLoggerEnumType loggerType,
                                     int *writeOffset) {
  return LEO_FUNCTION_NOT_IMPLEMENTED;
}

LeoErrorType leoGetSpdInfo(LeoDeviceType *device, LeoSpdInfoType *spdInfo,
                           uint8_t dimmIdx) {
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  bufferOut[0] = dimmIdx;
  ASTERA_DEBUG("Get SPD Info");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_CHK_DIMM_SPD_INFO,
                bufferOut, 1,
                bufferIn, 16);

  if (bufferIn[0] != MMB_STS_OK) {
    return LEO_FUNCTION_UNSUCCESSFUL;
  }

  spdInfo->ddr5SDRAM = bufferIn[1];
  spdInfo->dimmType = bufferIn[2];
  spdInfo->diePerPackage = bufferIn[3];
  spdInfo->densityInGb = bufferIn[4];
  spdInfo->rank = bufferIn[5];
  spdInfo->capacityInGb = bufferIn[6];
  spdInfo->channel = bufferIn[7];
  spdInfo->channelWidth = bufferIn[8];
  spdInfo->ioWidth = bufferIn[9];
  spdInfo->ddrSpeedMin = bufferIn[10];
  spdInfo->ddrSpeedMax = bufferIn[11];
  return LEO_SUCCESS;
}

LeoErrorType leoGetSpdManufacturingInfo(LeoDeviceType *device, LeoSpdInfoType *spdInfo,
                           uint8_t dimmIdx) {
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  bufferOut[0] = dimmIdx | (1 << 8);
  ASTERA_DEBUG("Get SPD Manufacturing Info");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_CHK_DIMM_SPD_INFO,
                bufferOut, 1,
                bufferIn, 16);

  if (bufferIn[0] != MMB_STS_OK) {
    return LEO_FUNCTION_UNSUCCESSFUL;
  }

  strncpy(spdInfo->partNumber, (char*)&bufferIn[1], 30);
  for (int i = 0; i < sizeof(spdInfo->partNumber); i++)
  {
    if (isspace(spdInfo->partNumber[i]))
    {
      spdInfo->partNumber[i] = '\0';
      break;
    }
  }

  spdInfo->manufacturerIdCode = bufferIn[9];
  spdInfo->dimmDateCodeBcd    = bufferIn[10];
  spdInfo->manufactureringLoc = bufferIn[11];
  spdInfo->serialNumber       = bufferIn[12];

  return LEO_SUCCESS;
}

LeoErrorType leoGetSpdDump(LeoDeviceType *device, uint32_t dimm_idx,
                           uint32_t *data) {
  uint32_t bufferOut[16];
  bufferOut[0] = dimm_idx;
  bufferOut[1] = 0;
  ASTERA_DEBUG("Dump SPD");
  execOperation(device->i2cDriver, 0, FW_API_MMB_CMD_OPCODE_MMB_DUMP_DIMM_SPD,
                bufferOut, 2, data, 16);
  bufferOut[1] = 64;
  execOperation(device->i2cDriver, 0, FW_API_MMB_CMD_OPCODE_MMB_DUMP_DIMM_SPD,
                bufferOut, 2, &data[16], 16);

  return LEO_SUCCESS;
}

LeoErrorType leoGetTsodData(LeoDeviceType *device,
                            LeoDimmTsodDataType *tsodData, uint8_t dimmIdx) {
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  bufferOut[0] = dimmIdx;
  ASTERA_DEBUG("Get TSOD Data");
  execOperation(device->i2cDriver, 0, FW_API_MMB_CMD_OPCODE_MMB_CHK_DIMM_TSOD,
                bufferOut, 1, bufferIn, 5);
  if (bufferIn[0] == MMB_STS_PENDING) {
    return LEO_FUNCTION_UNSUCCESSFUL;
  }
  tsodData->ts0WholeNum = bufferIn[1];
  tsodData->ts0Decimal = bufferIn[2];
  tsodData->ts1WholeNum = bufferIn[3];
  tsodData->ts1Decimal = bufferIn[4];

  if (0 == tsodData->ts0WholeNum + tsodData->ts0Decimal +
               tsodData->ts1WholeNum + tsodData->ts1Decimal) {
    return LEO_FUNCTION_UNSUCCESSFUL;
  }
  return LEO_SUCCESS;
}

LeoErrorType leoGetPrebootDdrConfig(LeoDeviceType *device,
                                    LeoDdrConfigType *ddrConfig) {
  uint32_t ddrtypeStr;
  uint32_t rankmapStr;
  uint32_t cxlmemStr;
  uint32_t dpc;
  uint32_t rc;

  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_3, &ddrtypeStr);
  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_40, &cxlmemStr);
  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_4, &rankmapStr);
  CHECK_SUCCESS(rc)

  ddrConfig->numRanks = (ddrtypeStr >> 3) & 0x1f;
  ddrConfig->ddrSpeed = (ddrtypeStr >> 8) & 0x3f;
  ddrConfig->dqWidth = (ddrtypeStr >> 14) & 0x1f;
  ddrConfig->capacity = (cxlmemStr)&0xff;
  dpc = (rankmapStr >> 27) & 0x1f;
  ddrConfig->dpc = 1;
  if (dpc == 15 || dpc == 5 || dpc == 10) {
    ddrConfig->dpc = 2;
  }
  return LEO_SUCCESS;
}

LeoErrorType leoGetPrebootCxlConfig(LeoDeviceType *device,
                                    LeoCxlConfigType *cxlConfig) {
  uint32_t numLinkStr;
  uint32_t linkWidthStr;
  uint32_t linkWidthDec;
  uint32_t rc;

  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_0, &numLinkStr);
  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_10, &linkWidthStr);
  linkWidthDec = (linkWidthStr >> 11) & 0xf;

  cxlConfig->numLinks = numLinkStr & 0x3;
  if (cxlConfig->numLinks == 3) {
    cxlConfig->numLinks = 1;
  }

  switch (linkWidthDec) {
  case 0:
    cxlConfig->linkWidth = 16;
    break;
  case 1:
    cxlConfig->linkWidth = 8;
    break;
  case 2:
    cxlConfig->linkWidth = 4;
    break;
  case 3:
    cxlConfig->linkWidth = 2;
    break;
  case 4:
    cxlConfig->linkWidth = 1;
    break;
  default:
    cxlConfig->linkWidth = 16;
    break;
  }
  return LEO_SUCCESS;
}

LeoErrorType leoGetLeoIdInfo(LeoDeviceType *device, LeoIdInfoType *leoIdInfo) {
  uint32_t sts;
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  ASTERA_DEBUG("Get Leo ID");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_CHK_EEPROM_LEO_ID, bufferOut, 0,
                bufferIn, 3);
  sts = bufferIn[0];
  if (1 != sts) {
    ASTERA_DEBUG("sts %d", sts);
    leoIdInfo->leoId = -1;
    return LEO_FW_NOT_RUNNING;
  }
  if (2 == bufferIn[2]) {
    return LEO_FUNCTION_UNSUCCESSFUL;
  }
  leoIdInfo->boardType = bufferIn[1];
  leoIdInfo->leoId = bufferIn[2];
  return LEO_SUCCESS;
}

LeoErrorType leoGetCmalStats(LeoDeviceType *device,
                             LeoCmalStatsType *leoCmalStats) {
  LeoErrorType rc;
  uint32_t sts_rrsp_ddr;
  uint32_t sts_rrsp_rdp;

  rc = leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SECOND_SLV_STS_RRSP_DDR_RETURNED_POISON_CNT_ADDRESS,
                       &sts_rrsp_ddr);
  CHECK_SUCCESS(rc);
  leoCmalStats->rrspDdrReturnedPoisonCount[0] = sts_rrsp_ddr & 0xff;
  leoCmalStats->rrspDdrReturnedPoisonCount[1] = (sts_rrsp_ddr >> 8) & 0xff;

  rc = leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SECOND_SLV_STS_RRSP_RDP_CAUSED_POISON_CNT_ADDRESS,
                       &sts_rrsp_rdp);
  CHECK_SUCCESS(rc);
  leoCmalStats->rrspRdpCausedPoisonCount[0] = sts_rrsp_rdp & 0xff;
  leoCmalStats->rrspRdpCausedPoisonCount[1] = (sts_rrsp_rdp >> 8) & 0xff;

  return LEO_SUCCESS;
}

LeoErrorType leoGetFailVector(LeoDeviceType *device,
                             LeoFailVectorType *leoFailVector) {
  LeoErrorType rc;
  uint32_t failVector;

  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_64, &failVector);
  CHECK_SUCCESS(rc);
  leoFailVector->failVector = failVector;
  return LEO_SUCCESS;
}
#endif

LeoErrorType leoSetCxlComplianceMode(LeoDeviceType *device, int cmplMode) {

  LeoCxlDvSecHdr1InfoType leoCxlDvsecHdr1;
  LeoCxlFlexbusRange2SizeLowType leoCxlFlexbusRange2Size;
  LeoErrorType rc;

  rc = leoReadWordData(device->i2cDriver,
                       CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_DVSEC_HDR_1_OFF,
                       (uint32_t *)&leoCxlDvsecHdr1);
  CHECK_SUCCESS(rc);

  ASTERA_INFO("Before compliance mode change");
  ASTERA_INFO("CXL Dvsec HDR 1: 0x%x", *(uint32_t *)&leoCxlDvsecHdr1);

  rc = leoReadWordData(
      device->i2cDriver,
      CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_FLEXBUS_R2_SIZE_LOW_OFF,
      (uint32_t *)&leoCxlFlexbusRange2Size);

  CHECK_SUCCESS(rc);

  ASTERA_INFO("CXL Flexbus Range 2 Size: 0x%x",
              *(uint32_t *)&leoCxlFlexbusRange2Size);

  if (cmplMode == 1) {
    leoCxlDvsecHdr1.dvsecRev = 0;
    leoCxlFlexbusRange2Size.mediaType = 0x1;
    leoCxlFlexbusRange2Size.memClass = 0;
  } else {
    leoCxlDvsecHdr1.dvsecRev = 0x1;
    leoCxlFlexbusRange2Size.mediaType = 0x2;
    leoCxlFlexbusRange2Size.memClass = 0x2;
  }

  rc = leoWriteWordData(device->i2cDriver,
                        CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_DVSEC_HDR_1_OFF,
                        *(uint32_t *)&leoCxlDvsecHdr1);

  CHECK_SUCCESS(rc);

  rc = leoWriteWordData(
      device->i2cDriver,
      CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_FLEXBUS_R2_SIZE_LOW_OFF,
      *(uint32_t *)&leoCxlFlexbusRange2Size);

  CHECK_SUCCESS(rc);

  rc = leoReadWordData(device->i2cDriver,
                       CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_DVSEC_HDR_1_OFF,
                       (uint32_t *)&leoCxlDvsecHdr1);
  CHECK_SUCCESS(rc);

  rc = leoReadWordData(
      device->i2cDriver,
      CSR_CXL_DBI_0_PF0_CXL_DEVICE_CAP_RCIEP_FLEXBUS_R2_SIZE_LOW_OFF,
      (uint32_t *)&leoCxlFlexbusRange2Size);

  CHECK_SUCCESS(rc);

  ASTERA_INFO("After compliance mode change");
  ASTERA_INFO("CXL Dvsec HDR 1: 0x%x", *(uint32_t *)&leoCxlDvsecHdr1);
  ASTERA_INFO("CXL Flexbus Range 2 Size: 0x%x",
              *(uint32_t *)&leoCxlFlexbusRange2Size);

  return LEO_SUCCESS;
}

static int printDiff[2];
static LeoCxlStatsType recentCxlStats;
LeoErrorType leoGetCxlStats(LeoDeviceType *device, LeoCxlStatsType *cxlStats,
                            int linkNum) {
  int rc = 0;
  int ii;
  uint32_t readData0;
  uint32_t readData1;
  uint64_t readData;
  LeoCxlStatsType prevCxlStats = recentCxlStats;
  uint32_t baseAddr;

  uint8_t csr_cxl_ctlr_id;

  if (0 == linkNum) {
    csr_cxl_ctlr_id = 0;
  } else {
    csr_cxl_ctlr_id = 1;
  }

  for (ii = 0; ii < 9; ii++) {
    rc += leoWriteWordData(device->i2cDriver,
                          leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_NUM_ADDRESS, csr_cxl_ctlr_id), ii);
    rc += leoWriteWordData(device->i2cDriver,
                          leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_RD_ADDRESS, csr_cxl_ctlr_id), 0);
    rc += leoWriteWordData(device->i2cDriver,
                          leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_RD_ADDRESS, csr_cxl_ctlr_id), 1);

    rc += leoReadWordData(device->i2cDriver,
                          leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W0_ADDRESS, csr_cxl_ctlr_id),
                          &readData0);
    rc += leoReadWordData(device->i2cDriver,
                          leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W1_ADDRESS, csr_cxl_ctlr_id),
                          &readData1);
    readData = (uint64_t)readData1 << 32 | readData0;
    /* ASTERA_INFO("Read data[%x]", leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W1_ADDRESS, csr_cxl_ctlr_id)); 
    /* ASTERA_INFO("CXL_CTR read %" PRIu64 "\n", readData); */
    if (ii == 1) {
      recentCxlStats.s2mNdrCount[linkNum] = readData;
      /* ASTERA_INFO("s2mNdrCount read %" PRIu64 "\n", readData); */
    }
    if (ii == 3) {
      recentCxlStats.s2mDrsCount[linkNum] = readData;
      /* ASTERA_INFO("s2mDrsCount read %" PRIu64 "\n", readData); */
    }
    if (ii == 5) {
      recentCxlStats.m2sReqCount[linkNum] = readData;
      /* ASTERA_INFO("m2sReqCount read %" PRIu64 "\n", readData); */
    }
    if (ii == 7) {
      recentCxlStats.m2sRwdCount[linkNum] = readData;
      /* ASTERA_INFO("m2sRwdCount read %" PRIu64 "\n", readData); */
    }
  }
  if (printDiff[linkNum]) {
    ASTERA_INFO("*************************");
    ASTERA_INFO("Delta(from last invocation):");
    ASTERA_INFO("*************************");
    ASTERA_INFO("CXL Link %d:", linkNum);
    ASTERA_INFO("S2M NDR Counter:  %lu", (recentCxlStats.s2mNdrCount[linkNum] -
                                         prevCxlStats.s2mNdrCount[linkNum]));
    ASTERA_INFO("S2M DRS Counter:  %lu", (recentCxlStats.s2mDrsCount[linkNum] -
                                         prevCxlStats.s2mDrsCount[linkNum]));
    ASTERA_INFO("M2S REQ Counter:  %lu", (recentCxlStats.m2sReqCount[linkNum] -
                                         prevCxlStats.m2sReqCount[linkNum]));
    ASTERA_INFO("M2S RWD Counter:  %lu", (recentCxlStats.m2sRwdCount[linkNum] -
                                         prevCxlStats.m2sRwdCount[linkNum]));
    ASTERA_INFO("*************************");
  }
  printDiff[linkNum] = 1;
  *cxlStats = recentCxlStats;
  return LEO_SUCCESS;
}

uint64_t leoGetDdrSramTelemetry(LeoDeviceType *device,
                                int ddrch, int sramOffset)
{
  int rc = 0;
  uint32_t readData0;
  uint64_t readData1;
  uint64_t readData;

  if (ddrch == 0) {
    rc += leoWriteWordData(device->i2cDriver,
                           leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_ADDR_ADDRESS, 0),
                           sramOffset);
    rc += leoWriteWordData(device->i2cDriver,
                           leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_CMD_RD_ADDRESS, 0),
                           ENABLE);
    rc += leoReadWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_CMD_DONE_ADDRESS, 0),
                          &readData0);
    while(readData0 == 0) {
      ASTERA_INFO("\nWaiting for Refresh Counter Read command to complete");
    }

    rc += leoReadWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_RD_VAL_W0_ADDRESS, 0),
                          &readData0);
    rc += leoReadWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_RD_VAL_W1_ADDRESS, 0),
                          (uint32_t *)&readData1);

    readData = ((uint64_t)readData1 << 32) | readData0;
  } else {


    rc += leoWriteWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_ADDR_ADDRESS, 1),
                          sramOffset);
    rc += leoWriteWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_CMD_RD_ADDRESS, 1),
                          ENABLE);
    rc += leoReadWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_CMD_DONE_ADDRESS, 1),
                          &readData0);


    while(readData0 == 0) {
      ASTERA_INFO("\nWaiting for Refresh Counter Read command to complete");
    }

    rc += leoReadWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_RD_VAL_W0_ADDRESS , 1),
                          &readData0);
    rc += leoReadWordData(device->i2cDriver,
                          leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_ANA_CTR_RD_VAL_W1_ADDRESS, 1),
                          (uint32_t *)&readData1);


    readData = ((uint64_t)readData1 << 32) | readData0;
  }
  return readData;
}

LeoErrorType leoGetDatapathTelemetryInt(LeoDeviceType *device,
                                        LeoDatapathTelemetryType *tel)
{
  int rc = 0;
  int ii;
  uint32_t val;
  uint32_t readData0;
  uint32_t readData1;
  uint64_t readData;
  static LeoDatapathTelemetryType trecent = { 0 };
  leo_err_info_t leo_err_info = { 0 };

  val = 0;
  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_CMAL_STS_TGC_MEM_ERR_CNT_ADDRESS,
                        &val);

  trecent.ddrTgcCe = (val & 0xff);
  trecent.ddrTgcUe = (val >> 8) & 0xff;

  val = 0;
  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_CMAL_STS_ONDMD_SCRB_CNT_ADDRESS,
                        &val);
  trecent.ddrOssc = (val & 0xff);
  trecent.ddrOsdc = (val >> 8) & 0xff;

  if (leoGetErrInfo(device, &leo_err_info) == LEO_SUCCESS) {
    trecent.ddrbscec = leo_err_info.scrb_corr_errcnt;
    trecent.ddrbsuec = leo_err_info.scrb_uncorr_errcnt;
    trecent.ddrbsoec = leo_err_info.scrb_other_errcnt;
  }

  *tel = trecent;
  return LEO_SUCCESS;
}

LeoErrorType leoGetDdrTelemetryInt(LeoDeviceType *device,
                                   LeoDdrTelemetryType *tel, int ddrch)
{
  int rc = 0;
  int ii;
  uint32_t val;
  uint32_t readData0;
  uint32_t readData1;
  uint64_t readData;

  static LeoDdrTelemetryType trecent[2];

  val = 0;
  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_CMAL_STS_WRSP_DDR_ADDR_ERR_CNT_ADDRESS,
                        &val);
  trecent[ddrch].ddrS0waec = (val & 0xff);
  trecent[ddrch].ddrS1waec = (val >> 8) & 0xff;
  trecent[ddrch].ddrS2waec = ((val >> 16) & 0xff);
  trecent[ddrch].ddrS3waec = (val >> 24) & 0xff;
  if (ddrch == 0)
    trecent[ddrch].ddrchwaec = trecent[ddrch].ddrS0waec + trecent[ddrch].ddrS1waec;
  else
    trecent[ddrch].ddrchwaec = trecent[ddrch].ddrS2waec + trecent[ddrch].ddrS3waec;

  val = 0;

  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_CMAL_STS_RRSP_DDR_CRC_ERR_CNT_ADDRESS,
                        &val);
  


  trecent[ddrch].ddrS0rdcrc = (val & 0xff);
  trecent[ddrch].ddrS1rdcrc = (val >> 8) & 0xff;
  trecent[ddrch].ddrS2rdcrc = (val >> 16) & 0xff;
  trecent[ddrch].ddrS3rdcrc = (val >> 24) & 0xff;
  if (ddrch == 0)
    trecent[ddrch].ddrchrdcrc = trecent[ddrch].ddrS0rdcrc + trecent[ddrch].ddrS1rdcrc;
  else
    trecent[ddrch].ddrchrdcrc = trecent[ddrch].ddrS2rdcrc + trecent[ddrch].ddrS3rdcrc;

  val = 0;

  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_CMAL_STS_RRSP_DDR_UNCORR_ERR_CNT_ADDRESS,
                        &val);
  


  trecent[ddrch].ddrS0rduec = (val & 0xff);
  trecent[ddrch].ddrS1rduec = (val >> 8) & 0xff;
  trecent[ddrch].ddrS2rduec = (val >> 16) & 0xff;
  trecent[ddrch].ddrS3rduec = (val >> 24) & 0xff;
  if (ddrch == 0)
    trecent[ddrch].ddrchrduec = trecent[ddrch].ddrS0rduec + trecent[ddrch].ddrS1rduec;
  else
    trecent[ddrch].ddrchrduec = trecent[ddrch].ddrS2rduec + trecent[ddrch].ddrS3rduec;

  val = 0;

  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_CMAL_STS_RRSP_DDR_CORR_ERR_CNT_ADDRESS,
                        &val);
  


  trecent[ddrch].ddrS0rdcec = (val & 0xff);
  trecent[ddrch].ddrS1rdcec = (val >> 8) & 0xff;
  trecent[ddrch].ddrS2rdcec = (val >> 16) & 0xff;
  trecent[ddrch].ddrS3rdcec = (val >> 24) & 0xff;
  if (ddrch == 0)
    trecent[ddrch].ddrchrdcec = trecent[ddrch].ddrS0rdcec + trecent[ddrch].ddrS1rdcec;
  else
    trecent[ddrch].ddrchrdcec = trecent[ddrch].ddrS2rdcec + trecent[ddrch].ddrS3rdcec;

  if (ddrch == 0) {
    trecent[ddrch].ddrPreChCount = leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_PRECHARGE_OFFSET) +
                                   leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_PRECHARGE_OFFSET_SUBCHN1);

    trecent[ddrch].ddrRdActCount = leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_RD_ACTIVATE_OFFSET) +
                                   leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_RD_ACTIVATE_OFFSET_SUBCHN1);

    trecent[ddrch].ddrRefCount = leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_REFRESH_OFFSET) +
                                 leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_REFRESH_OFFSET_SUBCHN1);

  } else {
    trecent[ddrch].ddrPreChCount = leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_PRECHARGE_OFFSET) +
                                   leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_PRECHARGE_OFFSET_SUBCHN1);

    trecent[ddrch].ddrRdActCount = leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_RD_ACTIVATE_OFFSET) +
                                   leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_RD_ACTIVATE_OFFSET_SUBCHN1);

    trecent[ddrch].ddrRefCount = leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_REFRESH_OFFSET) +
                                 leoGetDdrSramTelemetry(device, ddrch,
                                                 SRAM_ANA_CTR_REFRESH_OFFSET_SUBCHN1);

  }

  *tel = trecent[ddrch];
  return LEO_SUCCESS;
}

LeoErrorType leoGetClockTelemetry(LeoDeviceType *device, LeoCxlTelemetryType *tel)
{
  int rc = 0;
  uint32_t readData0;
  uint64_t readData1;
  uint64_t readData;
  static LeoCxlTelemetryType trecent;

  rc += leoWriteWordData(device->i2cDriver,
                        LEO_TOP_CSR_MUC_TIMER_EN_ADDRESS, ENABLE);
  rc += leoWriteWordData(device->i2cDriver,
                        LEO_TOP_CSR_MUC_TIMER_RD_ADDRESS, ENABLE);
  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_MUC_TIMER_VAL_HI_ADDRESS,
                        (uint32_t *)&readData1);
  rc += leoReadWordData(device->i2cDriver,
                        LEO_TOP_CSR_MUC_TIMER_VAL_LO_ADDRESS, &readData0);

  trecent.clock_ticks = (readData1 << 32) | readData0;

  ASTERA_INFO("Leo Clock Ticks Count so far: %" PRIu64,
              (trecent.clock_ticks));

  *tel = trecent;
  return LEO_SUCCESS;
}

LeoErrorType leoGetCxlLinkErrTelemetry(LeoDeviceType *device,
                                   LeoCxlTelemetryType *tel,
                                   int linkNum)
{
  int rc = 0;
  uint32_t readData0;
  uint64_t readData1;
  uint64_t readData;
  static LeoCxlTelemetryType trecent[2];
  uint32_t ctr_addr;
  uint32_t rd_addr;
  uint32_t val_w0_addr;
  uint32_t val_w1_addr;
  int numLinks;
  uint32_t linkSts, linkSts1;
  uint32_t linkWidth, linkWidth1;
  uint32_t linkTraining, linkTraining1;

  rc += leoReadWordData(device->i2cDriver,
           csr_cxl_mbar0_0_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG,
           &linkSts);


  linkWidth = (linkSts >> 20) & 0x1f;
  linkTraining = (linkSts >> 27) & 0x1;


  rc += leoReadWordData(device->i2cDriver,
           csr_cxl_mbar0_1_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG,
           &linkSts1);
  
  linkWidth1 = (linkSts1 >> 20) & 0x1f;
  numLinks = 1;
  if (linkWidth == 8 && linkWidth1 == 8) {
    numLinks = 2;
    linkTraining1 = (linkSts1 >> 27) & 0x1;
  } else if (linkWidth == 16) {
    numLinks = 1;
  } else if (linkWidth1 == 8 && linkTraining1 == 0) {
    numLinks = 1;
  }

  if (linkNum == 0 && linkTraining) {
    ASTERA_INFO("CXL Link%d is not up, returning zeroes\r", linkNum);
    memset(tel, 0, sizeof(LeoCxlTelemetryType));
    return rc;
  }
  
  if (linkNum == 1 && linkTraining1) {
    ASTERA_INFO("CXL Link%d is not up, returning zeroes\r", linkNum);
    memset(tel, 0, sizeof(LeoCxlTelemetryType));
    return rc;
  }

  trecent[0] = *tel;
  trecent[1] = *tel;

  if (linkNum == 0) {
    ctr_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_NUM_ADDRESS, 0);
    rd_addr  = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_RD_ADDRESS,  0);
    val_w0_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W0_ADDRESS, 0);
    val_w1_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W1_ADDRESS, 0);
  } else {

    
    ctr_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_NUM_ADDRESS, 1);
    rd_addr  = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_RD_ADDRESS,  1);
    val_w0_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W0_ADDRESS, 1);
    val_w1_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W1_ADDRESS, 1);
  }

  for (int ii = 9; ii < 32; ii++) {
    rc += leoWriteWordData(device->i2cDriver, rd_addr, DISABLE);
    rc += leoWriteWordData(device->i2cDriver, ctr_addr, ii);
    rc += leoWriteWordData(device->i2cDriver, rd_addr, ENABLE);
    leoReadWordData(device->i2cDriver, val_w0_addr, &readData0);
    leoReadWordData(device->i2cDriver, val_w1_addr, (uint32_t *)&readData1);
    readData = (readData1 << 32) + readData0;
    if (ii == 9) {
      trecent[linkNum].rasRxCe = readData;
    } else if (ii == 10) {
      trecent[linkNum].rasRxUeRwdHdr = readData;
    } else if (ii == 11) {
      trecent[linkNum].rasRxUeReqHdr = readData;
    } else if (ii == 12) {
      trecent[linkNum].rasRxUeRwdBe = readData;
    } else if (ii == 13) {
      trecent[linkNum].rasRxUeRwdData = readData;
    } 
  }

  if (linkNum == 0)
    ctr_addr = LEO_TOP_CSR_CMAL_STS_LINK0_M2SRWD_ERR_CNT_ADDRESS;
  else
    ctr_addr = LEO_TOP_CSR_CMAL_STS_LINK1_M2SRWD_ERR_CNT_ADDRESS;

  leoReadWordData(device->i2cDriver, ctr_addr, (uint32_t *)&readData);
  trecent[linkNum].rwdHdrUe = readData & 0xffff;
  trecent[linkNum].rwdHdrHdm = (readData >> 16) & 0xff;
  trecent[linkNum].rwdHdrUfe = (readData >> 24) & 0xff;

  if (linkNum == 0)
    ctr_addr = LEO_TOP_CSR_CMAL_STS_LINK0_M2SREQ_ERR_CNT_ADDRESS;
  else
    ctr_addr = LEO_TOP_CSR_CMAL_STS_LINK1_M2SREQ_ERR_CNT_ADDRESS;

  leoReadWordData(device->i2cDriver, ctr_addr, (uint32_t *)&readData);
  trecent[linkNum].reqHdrUe = readData & 0xffff;
  trecent[linkNum].reqHdrHdm = (readData >> 16) & 0xff;
  trecent[linkNum].reqHdrUfe = (readData >> 24) & 0xff;

  *tel = trecent[linkNum];
  return LEO_SUCCESS;
}

LeoErrorType leoGetCxlLinkTelemetry(LeoDeviceType *device,
                                LeoCxlTelemetryType *tel,
                                int linkNum)
{
  int rc = 0;
  uint32_t readData0;
  uint64_t readData1;
  uint64_t readData;
  static LeoCxlTelemetryType trecent[2];
  uint32_t ctr_addr;
  uint32_t rd_addr;
  uint32_t val_w0_addr;
  uint32_t val_w1_addr;
  int numLinks;
  uint32_t linkSts, linkSts1;
  uint32_t linkWidth, linkWidth1;
  uint32_t linkTraining, linkTraining1;

  rc += leoReadWordData(device->i2cDriver,
           csr_cxl_mbar0_0_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG,
           &linkSts);

  linkWidth = (linkSts >> 20) & 0x1f;
  linkTraining = (linkSts >> 27) & 0x1;

  rc += leoReadWordData(device->i2cDriver,
           csr_cxl_mbar0_1_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG,
           &linkSts1);

  linkWidth1 = (linkSts1 >> 20) & 0x1f;
  numLinks = 1;
  if (linkWidth == 8 && linkWidth1 == 8) {
    numLinks = 2;
    linkTraining1 = (linkSts1 >> 27) & 0x1;
  } else if (linkWidth == 16) {
    numLinks = 1;
  } else if (linkWidth1 == 8 && linkTraining1 == 0) {
    numLinks = 1;
  }

  if (linkNum == 0 && linkTraining) {
    return rc;
  }

  if (linkNum == 1 && linkTraining1) {
    return rc;
  }

  trecent[0] = *tel;
  trecent[1] = *tel;

  if (linkNum == 0) {
    ctr_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_NUM_ADDRESS, 0);
    rd_addr  = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_RD_ADDRESS,  0);
    val_w0_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W0_ADDRESS, 0);
    val_w1_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W1_ADDRESS, 0);
  } else {
    ctr_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_NUM_ADDRESS, 1);
    rd_addr  = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_RD_ADDRESS,  1);
    val_w0_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W0_ADDRESS, 1);
    val_w1_addr = leoGetCxlCtrAddr(LEO_TOP_CSR_CXL_CTR_ANA_CTR_VAL_OUT_W1_ADDRESS, 1);

  }

  for (int ii = 0; ii < 9; ii++) {
    rc += leoWriteWordData(device->i2cDriver, rd_addr, DISABLE);
    rc += leoWriteWordData(device->i2cDriver, ctr_addr, ii);
    rc += leoWriteWordData(device->i2cDriver, rd_addr, ENABLE);
    leoReadWordData(device->i2cDriver, val_w0_addr, &readData0);
    leoReadWordData(device->i2cDriver, val_w1_addr, (uint32_t *)&readData1);
    readData = (readData1 << 32) + readData0;
    if (ii == 1) {
      trecent[linkNum].s2m_ndr_c = readData;
    } else if (ii == 3) {
      trecent[linkNum].s2m_drs_c = readData;
    } else if (ii == 5) {
      trecent[linkNum].m2s_req_c = readData;
    } else if (ii == 7) {
      trecent[linkNum].m2s_rwd_c = readData;
    }
  }

  *tel = trecent[linkNum];
  return LEO_SUCCESS;
}

LeoErrorType leoGetCxlTelemetryInt(LeoDeviceType *device,
                                   LeoCxlTelemetryType *tel,
                                   int numLinks, int cxllink)
{
  int rc = 0; 
  rc += leoGetClockTelemetry(device, tel);

  if (numLinks == 2 || cxllink == -1) {
    rc += leoGetCxlLinkTelemetry(device, tel, 0);
    rc += leoGetCxlLinkErrTelemetry(device, tel, 0);
    rc += leoGetCxlLinkTelemetry(device, tel, 1);
    rc += leoGetCxlLinkErrTelemetry(device, tel, 1);
  } else {
    rc += leoGetCxlLinkTelemetry(device, tel, cxllink);
    rc += leoGetCxlLinkErrTelemetry(device, tel, cxllink);
  }

  return rc;
}

LeoErrorType leoGetDdrTelemetry(LeoDeviceType *device, int ddrch,
                                LeoDdrTelemetryType *full,
                                LeoDdrTelemetryType *sample,
                                int seconds)
{
  int ii;
  LeoDdrTelemetryType tafter;
  LeoDdrTelemetryType tbefore;

  int rc = 0;
  memset(&tbefore, 0, sizeof(LeoDdrTelemetryType));
  memset(&tafter, 0, sizeof(LeoDdrTelemetryType));

  leoGetDdrTelemetryInt(device, &tbefore, ddrch);
  *full = tbefore;
  if (seconds <= 0) {
    memset(sample, 0, sizeof(LeoDdrTelemetryType));
    goto out;
  }
  
  for (ii = 0; ii < seconds; ii++) {
    ASTERA_INFO("Got full DDR counters, waiting to sample(%d)\r", seconds - ii);
    fflush(stdout);
    sleep(1);
  }

  leoGetDdrTelemetryInt(device, &tafter, ddrch);

  sample->ddrchwaec = tafter.ddrchwaec - tbefore.ddrchwaec;
  sample->ddrchrdcrc = tafter.ddrchrdcrc - tbefore.ddrchrdcrc;
  sample->ddrchrduec = tafter.ddrchrduec - tbefore.ddrchrduec;
  sample->ddrchrdcec = tafter.ddrchrdcec - tbefore.ddrchrdcec;
  sample->ddrPreChCount = llabs(tafter.ddrPreChCount - tbefore.ddrPreChCount);
  sample->ddrRdActCount = llabs(tafter.ddrRdActCount - tbefore.ddrRdActCount);
  sample->ddrRefCount = llabs(tafter.ddrRefCount - tbefore.ddrRefCount);

out:
  ASTERA_INFO("******************************************");

  return LEO_SUCCESS;
}

LeoErrorType leoGetDatapathTelemetry(LeoDeviceType *device,
                                     LeoDatapathTelemetryType *full,
                                     LeoDatapathTelemetryType *sample,
                                     int seconds)
{
  int ii;
  LeoDatapathTelemetryType tafter;
  LeoDatapathTelemetryType tbefore;

  int rc = 0;
  memset(&tbefore, 0, sizeof(LeoDatapathTelemetryType));
  memset(&tafter, 0, sizeof(LeoDatapathTelemetryType));

  leoGetDatapathTelemetryInt(device, full);
  if (seconds <= 0) {
    memset(sample, 0, sizeof(LeoDatapathTelemetryType));
    goto out;
  }

  memcpy(&tbefore, full, sizeof(LeoDatapathTelemetryType));

  for (ii = 0; ii < seconds; ii++) {
    ASTERA_INFO("waiting (%d)\r", seconds - ii);
    fflush(stdout);
    sleep(1);
  }

  leoGetDatapathTelemetryInt(device, &tafter);

  sample->ddrTgcCe = tafter.ddrTgcCe - tbefore.ddrTgcCe;
  sample->ddrTgcUe = tafter.ddrTgcUe - tbefore.ddrTgcUe;
  sample->ddrOssc = tafter.ddrOssc - tbefore.ddrOssc;
  sample->ddrOsdc = tafter.ddrOsdc - tbefore.ddrOsdc;
  sample->ddrbscec = tafter.ddrbscec - tbefore.ddrbscec;
  sample->ddrbsuec = tafter.ddrbsuec - tbefore.ddrbsuec;
  sample->ddrbsoec = tafter.ddrbsoec - tbefore.ddrbsoec;

out:
  ASTERA_INFO("******************************************");

  return LEO_SUCCESS;
}

LeoErrorType leoGetCxlTelemetry(LeoDeviceType *device, int cxllink,
                                LeoCxlTelemetryType *full,
                                LeoCxlTelemetryType *sample,
                                int seconds)
{
  int ii;
  LeoCxlTelemetryType tafter;
  LeoCxlTelemetryType tbefore;
  LeoCxlTelemetryType tmp;

  int rc = 0;
  int numLinks;
  uint32_t linkSts, linkSts1;
  uint32_t linkWidth, linkWidth1;
  uint32_t linkTraining, linkTraining1;

  ASTERA_INFO("******************************************");
  

  rc += leoReadWordData(device->i2cDriver,
           csr_cxl_mbar0_0_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG,
           &linkSts);

  linkWidth = (linkSts >> 20) & 0x1f;
  ASTERA_INFO("CXL Link0 Width: %d", linkWidth);
  linkTraining = (linkSts >> 27) & 0x1;
  if (linkTraining)
    ASTERA_INFO("CXL Link0 Training: In Progress");
  else
    ASTERA_INFO("CXL Link0 Training: Done");

  rc += leoReadWordData(device->i2cDriver,
           csr_cxl_mbar0_1_PF0_PCIE_CAP_LINK_CONTROL_LINK_STATUS_REG,
           &linkSts1);

  linkWidth1 = (linkSts1 >> 20) & 0x1f;

  numLinks = 1;
  if (linkWidth == 8 && linkWidth1 == 8) {
    ASTERA_INFO("CXL Link1 Width: %d", linkWidth1);
    numLinks = 2;
    linkTraining1 = (linkSts1 >> 27) & 0x1;
    if (linkTraining1) {
      ASTERA_INFO("CXL Link1 Training: In Progress\n");
    } else
      ASTERA_INFO("CXL Link1 Training: Done\n");
  } else if (linkWidth == 16) {
    numLinks = 1;
  } else if (linkWidth1 == 8 && linkTraining1 == 0) {
    numLinks = 1;
  }

  ASTERA_INFO("******************************************");

  memset(&tbefore, 0, sizeof(LeoCxlTelemetryType));
  memset(&tafter, 0, sizeof(LeoCxlTelemetryType));

  leoGetCxlTelemetryInt(device, &tbefore, numLinks, cxllink);
  *full = tbefore;

  if (seconds <= 0) {
    memset(sample, 0, sizeof(LeoCxlTelemetryType));
    goto out;
  }
  
  for (ii = 0; ii < seconds; ii++) {
    ASTERA_INFO("Got full CXL counters, waiting to sample(%d)\r", seconds - ii);
    fflush(stdout);
    sleep(1);
  }

  leoGetCxlTelemetryInt(device, &tafter, numLinks, cxllink);

  sample->clock_ticks = tafter.clock_ticks - tbefore.clock_ticks;
  sample->s2m_ndr_c = tafter.s2m_ndr_c - tbefore.s2m_ndr_c;
  sample->s2m_drs_c = tafter.s2m_drs_c - tbefore.s2m_drs_c;
  sample->m2s_req_c = tafter.m2s_req_c - tbefore.m2s_req_c;
  sample->m2s_rwd_c = tafter.m2s_rwd_c - tbefore.m2s_rwd_c;
  sample->rasRxCe = tafter.rasRxCe - tbefore.rasRxCe;
  sample->rasRxUeRwdHdr = tafter.rasRxUeRwdHdr - tbefore.rasRxUeRwdHdr;
  sample->rasRxUeReqHdr = tafter.rasRxUeReqHdr - tbefore.rasRxUeReqHdr;
  sample->rasRxUeRwdBe = tafter.rasRxUeRwdBe - tbefore.rasRxUeRwdBe;
  sample->rasRxUeRwdData = tafter.rasRxUeRwdData - tbefore.rasRxUeRwdData;
  sample->rwdHdrUe = tafter.rwdHdrUe - tbefore.rwdHdrUe;
  sample->rwdHdrHdm = tafter.rwdHdrHdm - tbefore.rwdHdrHdm;
  sample->rwdHdrUfe = tafter.rwdHdrUfe - tbefore.rwdHdrUfe;
  sample->reqHdrUe = tafter.reqHdrUe - tbefore.reqHdrUe;
  sample->reqHdrHdm = tafter.reqHdrHdm - tbefore.reqHdrHdm;
  sample->reqHdrUfe = tafter.reqHdrUfe - tbefore.reqHdrUfe;

out:
  ASTERA_INFO("******************************************");

  return LEO_SUCCESS;
}

LeoErrorType leoSampleCxlStats(LeoDeviceType *device,
                               LeoCxlStatsType *resCxlStats,
                               int seconds) {
  int ii;
  uint64_t deltaNDR;
  uint64_t deltaDRS;
  LeoCxlStatsType cxlStatsBefore;
  LeoCxlStatsType cxlStatsAfter;
  leoGetCxlStats(device, &cxlStatsBefore, 0);
  leoGetCxlStats(device, &cxlStatsBefore, 1);
  
  for (ii = 0; ii < seconds; ii++) {
    printf("waiting (%02d)\r", seconds - ii);
    fflush(stdout);
    sleep(1);
  } 
  
  leoGetCxlStats(device, &cxlStatsAfter, 0);
  leoGetCxlStats(device, &cxlStatsAfter, 1);
  for (ii = 0; ii < 2; ii++) {
    if (0 == seconds)
      break; 
    deltaNDR = cxlStatsAfter.s2mNdrCount[ii] - cxlStatsBefore.s2mNdrCount[ii];
    deltaDRS = cxlStatsAfter.s2mDrsCount[ii] - cxlStatsBefore.s2mDrsCount[ii];
    cxlStatsAfter.linkBandwidth[ii] =
        (uint64_t)(deltaNDR + deltaDRS) * 64 / seconds;
  }
  *resCxlStats = cxlStatsAfter;
  return LEO_SUCCESS;
}

LeoErrorType leoGetDsid(LeoDeviceType *device, uint64_t *dsid) {
  LeoErrorType rc = 0;
  uint32_t dsidHi;
  uint32_t dsidLo;
  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_58, &dsidHi);
  rc = leoReadWordData(device->i2cDriver, GENERIC_CFG_REG_59, &dsidLo);

  *dsid = ((uint64_t)dsidHi << 32) | dsidLo;
  return rc;
}

LeoErrorType
leoGetPersistentData(LeoDeviceType *device, uint32_t persistentDataId,
                     LeoPersistentDataValueType *persistentDataValue) {
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];

  bufferOut[0] = persistentDataId;
  ASTERA_DEBUG("Get Persistent Data");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_CHK_PERSISTENT_DATA,
                bufferOut, 1,
                bufferIn, 3);

  persistentDataValue->countTotal = bufferIn[1];
  persistentDataValue->countSinceBoot = bufferIn[2];
  if (1 != bufferIn[0]) {
    return LEO_FAILURE;
  }

  return LEO_SUCCESS;
}

LeoErrorType leoCmndThrottle(LeoDeviceType *leoDevice, uint32_t count,
                             uint32_t max_BW, uint32_t enable) {
  uint32_t rc;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl;
  LeoDDRSubChannelThrottleContrlConfigType leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl;

  leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS, 0),
                        *(uint32_t *)&leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl);
  CHECK_SUCCESS(rc);

  leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS, 0),
                        *(uint32_t *)&leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl);
  CHECK_SUCCESS(rc);

  leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS, 0),
                        *(uint32_t *)&leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl);
  CHECK_SUCCESS(rc);

  leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS, 0),
                        *(uint32_t *)&leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl);
  CHECK_SUCCESS(rc);

  leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS, 1),
                        *(uint32_t *)&leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl);
  CHECK_SUCCESS(rc);

  leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS, 1),
                        *(uint32_t *)&leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl);
  CHECK_SUCCESS(rc);

  leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS, 1),
                        *(uint32_t *)&leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl);
  CHECK_SUCCESS(rc);

  leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl.throtCtrlEnable = enable;
  leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl.maxCmdCntPerWindow = max_BW;
  leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl.windowLength = count;
  rc = leoWriteWordData(leoDevice->i2cDriver,
                        leoGetDdrCtlAddr(LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS, 1),
                        *(uint32_t *)&leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl);
  CHECK_SUCCESS(rc);

#if 1
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_0RCMDThrotCtrl.windowLength);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_0WCMDThrotCtrl.windowLength);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_1RCMDThrotCtrl.windowLength);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS (CTL0) = %d",
      leoDDRCtl_0CfgSubchn_1WCMDThrotCtrl.windowLength);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_RCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_0RCMDThrotCtrl.windowLength);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN0_WCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_0WCMDThrotCtrl.windowLength);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_RCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_1RCMDThrotCtrl.windowLength);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl.throtCtrlEnable);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl.maxCmdCntPerWindow);
  ASTERA_DEBUG(
      "DDR Command Throttle LEO_TOP_CSR_DDR_CTL_CFG_SUBCHN1_WCMD_THROTTLE_CTRL_ADDRESS (CTL1) = %d",
      leoDDRCtl_1CfgSubchn_1WCMDThrotCtrl.windowLength);
#endif

  return rc;
}

LeoErrorType leoGetMaxDimmTemp(LeoDeviceType *leoDevice, int leoHandle,
                               int *maxTemp, uint32_t thresoldTemp,
                               LeoDimmTsodDataType *getleoDimmTsodData) {
  if (getleoDimmTsodData->ts0WholeNum > *maxTemp) {
    *maxTemp = getleoDimmTsodData->ts0WholeNum;
  }
  if (getleoDimmTsodData->ts1WholeNum > *maxTemp) {
    *maxTemp = getleoDimmTsodData->ts1WholeNum;
  }

  if ((getleoDimmTsodData->ts0WholeNum > thresoldTemp) ||
      (getleoDimmTsodData->ts1WholeNum > thresoldTemp)) {

    ASTERA_ERROR("DIMM temperature is above threshold value of %dC",
                 thresoldTemp);
  } else {
    ASTERA_INFO("DIMM temperature is below threshold value of %dC",
                thresoldTemp);
  }

  return LEO_SUCCESS;
}

LeoErrorType leoTempBasedBWThrottle(LeoDeviceType *leoDevice, int leoHandle,
                                    int max, int count) {

  switch (count) {
  case 10:
    leoCmndThrottle(leoDevice, 8, 100, 1);
    break;
  case 25:
    leoCmndThrottle(leoDevice, 16, 100, 1);
    break;
  case 50:
    leoCmndThrottle(leoDevice, 50, 100, 1); // 32 is 50% of 100
    break;
  case 75:
    leoCmndThrottle(leoDevice, 72, 100, 1); // 48 is 75% of 100
    break;
  default:
    ASTERA_ERROR("Invalid throttle percentage");
    return LEO_FAILURE;
  }
}

LeoErrorType leoGetDdrphyRxTxTrainingMargins(LeoDeviceType *device, uint32_t ddrcId, LeoDdrphyRxTxMarginType *ddrphyRxTxMargin) {
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  uint32_t marginType = 0;
  int ii;

  bufferOut[0] = ddrcId | (marginType << 8);
  ASTERA_DEBUG("Get RxTx training margin");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_GET_DDRPHY_TRAINING_MARGIN,
                bufferOut, 1,
                bufferIn, 8);
  for (ii = 0; ii < 8; ii++) {
    ddrphyRxTxMargin->dw[ii] = bufferIn[ii];
  }

  return LEO_SUCCESS;
}

LeoErrorType leoGetDdrphyCsCaTrainingMargins(LeoDeviceType *device, uint32_t ddrcId, LeoDdrphyCsCaMarginType *ddrphyCsCaMargin) {
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  uint32_t marginType = 1;
  int ii;

  bufferOut[0] = ddrcId | (marginType << 8);
  ASTERA_DEBUG("Get CsCa training margin");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_GET_DDRPHY_TRAINING_MARGIN,
                bufferOut, 1,
                bufferIn, 8);
  for (ii = 0; ii < 8; ii++) {
    ddrphyCsCaMargin->dw[ii] = bufferIn[ii];
  }

  return LEO_SUCCESS;
}

LeoErrorType leoGetDdrphyQcsQcaTrainingMargins(LeoDeviceType *device, uint32_t ddrcId, LeoDdrphyQcsQcaMarginType *ddrphyQcsQcaMargin) {
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  uint32_t marginType = 2;
  int ii;

  bufferOut[0] = ddrcId | (marginType << 8);
  ASTERA_DEBUG("Get QcsQca training margin");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_GET_DDRPHY_TRAINING_MARGIN,
                bufferOut, 1,
                bufferIn, 4);
  for (ii = 0; ii < 4; ii++) {
    ddrphyQcsQcaMargin->dw[ii] = bufferIn[ii];
  }

  return LEO_SUCCESS;
}

LeoErrorType leoWriteDDR(LeoDeviceType *device, uint64_t dpa)
{
  LeoErrorType sts = LEO_SUCCESS;
  uint32_t bufferIn[16] = { 0 };
  uint32_t bufferOut[16] = { 0 };
  uint32_t link_id = device->controllerIndex ? 1 : 0;
  int ii = 0;

  bufferOut[0] = link_id;
  bufferOut[1] = DDR_WRITE;
  bufferOut[2] = (uint32_t)dpa;
  bufferOut[3] = (uint32_t)(dpa >> 32);

  ASTERA_INFO("Link[%d]: %s, dpa_hi=0x%08x, dpa_lo=0x%08x",
      bufferOut[0], bufferOut[1] ? "DDR_READ" : "DDR_WRITE",
      bufferOut[3], bufferOut[2]);

  for (ii = 0; ii < 12; ii++) {
    bufferOut[ii+4] = 0x12345600 + ii;
  }

  for (ii = 0; ii < 16; ii++) {
    ASTERA_INFO("Write Data[%2d]  : 0x%08x", ii, bufferOut[ii]);
  }

  sts = execOperation(device->i2cDriver, 0,
      FW_API_MMB_CMD_OPCODE_MMB_ACCESS_DRAM_DBG_INTF,
      bufferOut, 16,
      bufferIn, 0);

  return (sts);
}

LeoErrorType leoReadDDR(LeoDeviceType *device, uint64_t dpa)
{
  LeoErrorType sts = LEO_SUCCESS;
  uint32_t bufferIn[16] = { 0 };
  uint32_t bufferOut[16] = { 0 };
  uint32_t link_id = device->controllerIndex ? 1 : 0;

  bufferOut[0] = link_id;
  bufferOut[1] = DDR_READ;
  bufferOut[2] = (uint32_t)dpa;
  bufferOut[3] = (uint32_t)(dpa >> 32);

  ASTERA_INFO("Link[%d]: %s, dpa_hi=0x%08x, dpa_lo=0x%08x",
      bufferOut[0], bufferOut[1] ? "DDR_READ" : "DDR_WRITE",
      bufferOut[3], bufferOut[2]);

  sts = execOperation(device->i2cDriver, 0,
      FW_API_MMB_CMD_OPCODE_MMB_ACCESS_DRAM_DBG_INTF,
      bufferOut, 4,
      bufferIn, 16);

  for (int ii = 0; ii < 16; ii++) {
    ASTERA_INFO("Read Data[%2d]   : 0x%08x", ii, bufferIn[ii]);
  }

  return (sts);
}

LeoErrorType leoReadEepromWord(LeoDeviceType *device, uint32_t addr, uint32_t* data)
{
  uint32_t sts;
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  execOperation(device->i2cDriver, addr,
                FW_API_MMB_CMD_OPCODE_MMB_RD_EEPROM,
                bufferOut, 0,
                bufferIn, 2);
  sts = bufferIn[0];
  if (1 != sts) {
    return LEO_FUNCTION_UNSUCCESSFUL;
  }
  *data = bufferIn[1];
  return LEO_SUCCESS;
}

LeoErrorType leoGetRecentUartRx(LeoDeviceType *device, char* uart_char)
{
  uint32_t sts;
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  execOperation(device->i2cDriver, 0, FW_API_MMB_CMD_OPCODE_MMB_GET_RECENT_UART_RX, 
                bufferOut, 0, bufferIn, 2);
  sts = bufferIn[0];
  if (1 != sts) {
    return LEO_FUNCTION_UNSUCCESSFUL;
  }
  *uart_char = bufferIn[1];

  return LEO_SUCCESS;
}

LeoErrorType leoGetErrInfo(LeoDeviceType *device, leo_err_info_t *leo_err_info)
{
  LeoErrorType sts = LEO_SUCCESS;
  uint32_t bufferIn[16];
  uint32_t bufferOut[16];
  uint32_t link_id = device->controllerIndex ? 1 : 0;

  bufferOut[0] = link_id;

  sts = execOperation(device->i2cDriver, 0,
      FW_API_MMB_CMD_OPCODE_MMB_GET_LEO_ERR_INFO,
      bufferOut, 1,
      bufferIn, 11);

  if (!sts) {
    memcpy(leo_err_info, bufferIn, sizeof(leo_err_info_t));
  }

  return (sts);
}

/**
 * @brief Leo repair given row in a DIMM
 *
 * Takes the channel, rank, bankgroup, bank and row to repair if possible
 *
 * @param[in]  repairRecord that has the inputs
 * @param[in]  sets pStatus to LeoSuccess if SPPR is successful else LeoFailure

 * @return     LeoErrorType - Leo error code
 */
LeoErrorType leoRunSPPR(LeoDeviceType *device,
                          LeoRepairRecordType *repairRecord)
{
  uint32_t bufferIn[4];
  uint32_t bufferOut[8];
  bufferOut[0] = repairRecord->channel;
  bufferOut[1] = repairRecord->bank;
  bufferOut[2] = repairRecord->bankGroup;
  bufferOut[3] = repairRecord->rank;
  bufferOut[4] = repairRecord->row;
  bufferOut[5] = repairRecord->CID;
  bufferOut[6] = repairRecord->nibbleMask;
  bufferOut[7] = 0;
  ASTERA_DEBUG("Run SPPR");
  execOperation(device->i2cDriver, 0,
                FW_API_MMB_CMD_OPCODE_MMB_SPPR,
                bufferOut, 8,
                bufferIn, 2);
  repairRecord->pStatus = bufferIn[0];
  repairRecord->pResource = 0;

  return repairRecord->pStatus;
}



uint32_t leoGetDdrCtlAddr(uint32_t top_csr_addr, uint8_t ctl_id)
{
  if (ctl_id == 0) {
    return top_csr_addr - LEO_TOP_CSR_DDR_CTL_ADDRESS + CSR_DDR_CTLR_0_CSR_BASE_ADDRESS;
  }
  else if (ctl_id == 1) {
    return top_csr_addr - LEO_TOP_CSR_DDR_CTL_ADDRESS + CSR_DDR_CTLR_1_CSR_BASE_ADDRESS;
  }
  return 0;
}


uint32_t leoGetCxlCtrAddr(uint32_t top_csr_addr, uint8_t ctl_id) 
{
  if (ctl_id == 0) {
    return top_csr_addr - LEO_TOP_CSR_CXL_CTR_ADDRESS + CSR_CXL_CTLR_0_CSR_BASE_ADDRESS;
  }
  else if (ctl_id == 1) {
    return top_csr_addr - LEO_TOP_CSR_CXL_CTR_ADDRESS + CSR_CXL_CTLR_1_CSR_BASE_ADDRESS;
  }
  return 0;
}

#ifdef __cplusplus
}
#endif

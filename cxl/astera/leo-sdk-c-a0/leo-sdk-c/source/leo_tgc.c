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
 * @file leo_tgc.c
 * @brief Implementation of TGC test
 */

#include "../include/leo_tgc.h"

static int pollCsr(LeoDeviceType *device, uint32_t addr, uint32_t data) {
  uint32_t rdData;
  ASTERA_DEBUG("Poll CSR start(0x%x,0x%x)", addr, data);
  do {
    leoReadWordData(device->i2cDriver, addr, &rdData);
  } while (data != rdData);
  ASTERA_DEBUG("Poll CSR done (0x%x,0x%x)", addr, data);
}

// Function to return sram entry
static uint32_t *packedSramEntry(struct tgcSramEntryStruct data) {
  static uint32_t tgcData[4];

  tgcData[3] = data.seqEnd; // 1
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 6) | (0x0000003f & data.rsvd126121);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 1) | (0x00000001 & data.encryptDis);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 1) | (0x00000001 & data.rsvd119);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 3) | (0x00000007 & data.dataCtrl);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 14) | (0x00003fff & data.rsvd115102);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 1) | (0x00000001 & data.wrEn);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 1) | (0x00000001 & data.rdEn);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 3) | (0x00000007 & data.rsvd9997);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[3] = (tgcData[3] << 1) | (0x00000001 & data.addrInc);
  // ASTERA_INFO("tgcData 3: %x \n", tgcData[3]);
  tgcData[2] = data.addrSkip; // 8
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[2] = (tgcData[2] << 7) | (0x0000007f & data.rsvd8781);
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[2] = (tgcData[2] << 9) | (0x000001ff & data.tranCnt);
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[2] = (tgcData[2] << 1) | (0x00000001 & data.rsvd71);
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[2] = (tgcData[2] << 1) | (0x00000001 & data.poison);
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[2] = (tgcData[2] << 2) | (0x00000003 & data.meta);
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[2] = (tgcData[2] << 1) | (0x00000001 & data.rsvd67);
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[2] = (tgcData[2] << 3) | (0x00000007 & (data.startAddr >> 32));
  // ASTERA_INFO("tgcData 2: %x \n", tgcData[2]);
  tgcData[1] = data.startAddr; // 32
  // ASTERA_INFO("tgcData 1: %x \n", tgcData[1]);
  tgcData[0] = data.dataPattern; // 32
  return (tgcData);
}

static int configTgc(LeoDeviceType *device, uint16_t wrEn, uint16_t rdEn,
                     uint16_t tranCnt, uint64_t startAddr,
                     uint32_t dataPattern) {
  struct tgcSramEntryStruct tgcSramEntry;
  uint32_t *tgcSramEntryDataArray;
  uint32_t k, rdData;
  uint32_t entryNum = 0;
  uint16_t seqEnd = 1;
  uint16_t encryptDis = 0;
  uint16_t dataCtrl = 0;
  uint16_t addrInc = 1;
  uint16_t addrSkip = 1;
  uint16_t poison = 0;
  uint16_t meta = 0;

  ASTERA_DEBUG("configTgc read=%d, tranCnt=%x", rdEn, tranCnt);

  // reset subchannel counter
  rdData = 0xffffffff;
  ASTERA_DEBUG("Write LEO_TOP_CSR_CMAL_CFG_PERF_CNTR_ADDRESS Data:0x%x",
               rdData);
  leoWriteWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_CFG_PERF_CNTR_ADDRESS,
                   rdData);

  // Configure SRAM entry
  tgcSramEntry.seqEnd = seqEnd; // Value = 1 indicates last entry in RAM
  tgcSramEntry.rsvd126121 = 0x0;
  tgcSramEntry.encryptDis =
      encryptDis; // (Used as initialize in the seq) Value = 1 indicates CMAL
                  // not to encrypt the write data with write commands
  tgcSramEntry.rsvd119 = 0x0;
  tgcSramEntry.dataCtrl =
      dataCtrl; // Indicates the kind of data pattern to generate
  tgcSramEntry.rsvd115102 = 0x0;
  tgcSramEntry.wrEn = wrEn; // Enable writes
  tgcSramEntry.rdEn = rdEn; // Enable reads
  tgcSramEntry.rsvd9997 = 0x0;
  tgcSramEntry.addrInc = addrInc;   // 1 = Address counter with increment, 0 =
                                    // Address counter with decrement
  tgcSramEntry.addrSkip = addrSkip; // 0 = Issue all rd and wr to same address,
                                    // 1 = Increment address sequentially >1 New
                                    // address = Old address +/- Address skip
  tgcSramEntry.rsvd8781 = 0x0;
  tgcSramEntry.tranCnt = tranCnt; // Transaction count = 4k
  tgcSramEntry.rsvd71 = 0x0;
  tgcSramEntry.poison = poison;
  tgcSramEntry.meta = meta;
  tgcSramEntry.rsvd67 = 0x0;
  tgcSramEntry.startAddr = startAddr;
  tgcSramEntry.dataPattern = dataPattern;

  tgcSramEntryDataArray = packedSramEntry(tgcSramEntry);
  for (k = 0; k < 4; k++) {
    ASTERA_DEBUG("tgcSramEntryDataArray[%d] = %x ", k,
                 tgcSramEntryDataArray[k]);
  }

  // Indirect TGC SRAM access
  indirectTgcSramAccess(device, entryNum, tgcSramEntryDataArray);
}

static int indirectTgcSramAccess(LeoDeviceType *device, uint32_t entryNum,
                                 uint32_t *tgcSramEntryDataArray) {
  uint32_t rdData;
  ASTERA_DEBUG("TGC entry[%d]: %x%x%x%x", entryNum, tgcSramEntryDataArray[3],
               tgcSramEntryDataArray[2], tgcSramEntryDataArray[1],
               tgcSramEntryDataArray[0]);
  ASTERA_DEBUG("Write:LEO_TOP_CSR_CMAL_CSR_TGC_MEM_ADDR_ADDRESS Data:0x%x",
               entryNum);
  leoWriteWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_CSR_TGC_MEM_ADDR_ADDRESS,
                   entryNum);

  ASTERA_DEBUG("Write:LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS Data:0x%x",
               tgcSramEntryDataArray[0]);
  leoWriteWordData(device->i2cDriver,
                   LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS,
                   tgcSramEntryDataArray[0]);
  ASTERA_DEBUG("Write:LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS + 4 Data:0x%x",
               tgcSramEntryDataArray[1]);
  leoWriteWordData(device->i2cDriver,
                   LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS,
                   tgcSramEntryDataArray[1]);
  ASTERA_DEBUG("Write:LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS + 8 Data:0x%x",
               tgcSramEntryDataArray[2]);
  leoWriteWordData(device->i2cDriver,
                   LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS,
                   tgcSramEntryDataArray[2]);
  ASTERA_DEBUG(
      "Write:LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS + 12 Data:0x%x",
      tgcSramEntryDataArray[3]);
  leoWriteWordData(device->i2cDriver,
                   LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WDATA_ADDRESS,
                   tgcSramEntryDataArray[3]);
  ASTERA_DEBUG("Write:LEO_TOP_CSR_CMAL_CSR_TGC_MEM_WR_REQ_ADDRESS Data:0x%x",
               0x1);
  leoWriteWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_CSR_TGC_MEM_ADDR_ADDRESS,
                   0x1);
  ASTERA_DEBUG("Poll:LEO_TOP_CSR_CMAL_CSR_TGC_MEM_ACK_STS_ADDRESS Data:0x%x",
               0x1);
  pollCsr(device, LEO_TOP_CSR_CMAL_CSR_TGC_MEM_ACK_STS_ADDRESS, 0x1);
}

static int initTgc(LeoDeviceType *device) {
  ASTERA_DEBUG("initTgc\n");
  ASTERA_DEBUG(
      "Write:LEO_TOP_CSR_CMAL_CMAL_INTR_GRP_TGC_SCRB_CLR_ADDRESS Data:0x%x",
      0xffffffff);
  leoWriteWordData(device->i2cDriver,
                   LEO_TOP_CSR_CMAL_CMAL_INTR_GRP_TGC_SCRB_CLR_ADDRESS,
                   0xffffffff);
  ASTERA_DEBUG(
      "Write:LEO_TOP_CSR_CMAL_CMAL_INTR_GRP_TGC_SCRB_EN_ADDRESS Data:0x%x",
      0x0000000c);
  leoWriteWordData(device->i2cDriver,
                   LEO_TOP_CSR_CMAL_CMAL_INTR_GRP_TGC_SCRB_EN_ADDRESS,
                   0x0000000c);
  ASTERA_DEBUG("Write:LEO_TOP_CSR_CMAL_TGC_ODM_CTRL_ADDRESS Data:0x%x",
               0x00000000);
  leoWriteWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_TGC_ODM_CTRL_ADDRESS,
                   0x00000000);
  ASTERA_DEBUG("Write:LEO_TOP_CSR_CMAL_TGC_ODM_CTRL_ADDRESS Data:0x%x",
               0x00010010);
  leoWriteWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_TGC_ODM_CTRL_ADDRESS,
                   0x00010010);
}

static int wait4tgcDone(LeoDeviceType *device) {
  uint32_t tgcDone;
  uint32_t rdData;
  uint32_t time_taken = 0;
  ASTERA_INFO("waitinng For tgcDone\an");
  ASTERA_DEBUG(
      "Poll:LEO_TOP_CSR_CMAL_CMAL_INTR_GRP_TGC_SCRB_LOG_ADDRESS Data:0x%x",
      0x00000008);
  do {
    leoReadWordData(device->i2cDriver,
                    LEO_TOP_CSR_CMAL_CMAL_INTR_GRP_TGC_SCRB_LOG_ADDRESS,
                    &rdData);

    tgcDone = (rdData >> 3) & 0x1;
    time_taken += 2;
    usleep(2000000);
    if (tgcDone == 0)
      ASTERA_INFO("Waiting %d data:%d", time_taken, rdData);
    if (time_taken > 60) {
      ASTERA_INFO("ERROR %d Timeout %d", rdData, time_taken);
      break;
    }
  } while (tgcDone == 0);
  ASTERA_DEBUG(
      "Poll:LEO_TOP_CSR_CMAL_CMAL_INTR_GRP_TGC_SCRB_LOG_ADDRESS Data:0x%x",
      rdData);
  ASTERA_INFO("TGC Done");
}

static int readTgcStatus(LeoDeviceType *device, LeoResultsTgc_t *tgcResults) {
  uint32_t rdData;
  uint32_t wrReqCnt0, wrReqCnt1, wrReqCnt2, wrReqCnt3;
  uint32_t rdReqCnt0, rdReqCnt1, rdReqCnt2, rdReqCnt3;
  ASTERA_DEBUG("readTgcStatus\n");

  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_STATUS_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_TGC_STATUS_ADDRESS,
                  &tgcResults->tgcStatus);

  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_STATUS_ADDRESS Data:0x%x",
               tgcResults->tgcStatus);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_ERR0_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_TGC_ERR0_ADDRESS,
                  &tgcResults->tgcError0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_ERR0_ADDRESS Data:0x%x",
               tgcResults->tgcError0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_ERR1_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_TGC_ERR1_ADDRESS,
                  &tgcResults->tgcError1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_ERR1_ADDRESS Data:0x%x",
               tgcResults->tgcError1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_DATA_MISMATCH_VEC_ADDRESS");
  leoReadWordData(device->i2cDriver,
                  LEO_TOP_CSR_CMAL_TGC_DATA_MISMATCH_VEC_ADDRESS,
                  &tgcResults->tgcDataMismatchVec);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_DATA_MISMATCH_VEC_ADDRESS Data:0x%x",
               tgcResults->tgcDataMismatchVec);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_DATA_MISMATCH_BYTE_ADDRESS");
  leoReadWordData(device->i2cDriver,
                  LEO_TOP_CSR_CMAL_TGC_DATA_MISMATCH_BYTE_ADDRESS,
                  &tgcResults->tgcDataMismatch);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_DATA_MISMATCH_BYTE_ADDRESS Data:0x%x",
               tgcResults->tgcDataMismatch);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_ODM_WR_GEN_LOWER_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver,
                  LEO_TOP_CSR_CMAL_TGC_ODM_WR_GEN_LOWER_CNT_ADDRESS,
                  &tgcResults->wrReqLowCnt);
  ASTERA_DEBUG(
      "Read:LEO_TOP_CSR_CMAL_TGC_ODM_WR_GEN_LOWER_CNT_ADDRESS Data:0x%x",
      tgcResults->wrReqLowCnt);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_ODM_RD_GEN_LOWER_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver,
                  LEO_TOP_CSR_CMAL_TGC_ODM_RD_GEN_LOWER_CNT_ADDRESS,
                  &tgcResults->rdReqLowCnt);
  ASTERA_DEBUG(
      "Read:LEO_TOP_CSR_CMAL_TGC_ODM_RD_GEN_LOWER_CNT_ADDRESS Data:0x%x",
      tgcResults->rdReqLowCnt);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_TGC_ODM_WR_RD_GEN_UPPER_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver,
                  LEO_TOP_CSR_CMAL_TGC_ODM_WR_RD_GEN_UPPER_CNT_ADDRESS,
                  &tgcResults->wrRdupperCnt);
  ASTERA_DEBUG(
      "Read:LEO_TOP_CSR_CMAL_TGC_ODM_WR_RD_GEN_UPPER_CNT_ADDRESS Data:0x%x",
      tgcResults->wrRdupperCnt);

  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_RREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN0_RREQ_CNT_ADDRESS,
                  &rdReqCnt0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN1_RREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN1_RREQ_CNT_ADDRESS,
                  &rdReqCnt1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN1_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN2_RREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN2_RREQ_CNT_ADDRESS,
                  &rdReqCnt2);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN2_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt2);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN3_RREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN3_RREQ_CNT_ADDRESS,
                  &rdReqCnt3);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt3);

  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_WREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN0_WREQ_CNT_ADDRESS,
                  &wrReqCnt0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN1_WREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN1_WREQ_CNT_ADDRESS,
                  &wrReqCnt1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN1_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN2_WREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN2_WREQ_CNT_ADDRESS,
                  &wrReqCnt2);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN2_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt2);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN3_WREQ_CNT_ADDRESS");
  leoReadWordData(device->i2cDriver, LEO_TOP_CSR_CMAL_SUBCHN3_WREQ_CNT_ADDRESS,
                  &wrReqCnt3);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt3);
  tgcResults->wrSubchnlCnt = wrReqCnt0 + wrReqCnt1 + wrReqCnt2 + wrReqCnt3;
  tgcResults->rdSubchnlCnt = rdReqCnt0 + rdReqCnt1 + rdReqCnt2 + rdReqCnt3;

  if ((tgcResults->tgcStatus & 0x00000001) &
      ((tgcResults->tgcError0 >> 8) & 0x000000ff) == 0x0) {
    ASTERA_INFO("TGC TEST PASS");
    fflush(stdout);
  } else {
    ASTERA_INFO("TGC TEST FAIL");
    fflush(stdout);
  }
  ASTERA_INFO("TGC STATUS");
  ASTERA_INFO("%s\t :0x%x", "tgc done                   ",
              tgcResults->tgcStatus & 0x00000001);
  fflush(stdout);
}

/*
 * @brief Leo start traffic generator and checker (TGC)
 *
 * Generate TGC on DDR on the CXL interface
 * @param[in] LeoDeviceType I2C device handle
 * @param[in] LeoConfigTgc_t TGC settings
 * @param[out] LeoResultsTgc_t TGC results status
 * @param[out] LeoErrortype
 *
 */
LeoErrorType leoPatternGeneratorandChecker(LeoDeviceType *device,
                                           LeoConfigTgc_t *trafficGenInfo,
                                           LeoResultsTgc_t *tgcResults) {
  ASTERA_INFO("tgcTest\n");

  // Config TGC
  configTgc(device, trafficGenInfo->wrEn, trafficGenInfo->rdEn,
            trafficGenInfo->tranCnt, trafficGenInfo->startAddr,
            trafficGenInfo->dataPattern);

  // START TGC
  initTgc(device);
  // Wait for INTR
  wait4tgcDone(device);
  // READ Status CSR
  readTgcStatus(device, tgcResults);
  return LEO_SUCCESS;
}

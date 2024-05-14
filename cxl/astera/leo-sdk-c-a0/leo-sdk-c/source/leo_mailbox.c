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
 * @file leo_mailbox.c
 * @brief Implementation of mailbox related functions for the SDK.
 */
#include "../include/leo_mailbox.h"
#include <stdint.h>

static LeoMailboxInfoType LeoMailboxInfo_s = {.operationInProgress = 0,
                                              .timeoutCnt = 10000};

void sendMailboxCmd(LeoI2CDriverType *leoDriver, uint32_t addr, uint32_t cmd,
                    size_t payloadLen) {
  leoWriteWordData(leoDriver, LEO_TOP_CSR_MUC_MAIL_BOX_ADDR_ADDRESS, addr);
  uint32_t data =
      cmd | (payloadLen << 24) | (1 << 16); // bit 16 is the doorbell
  leoWriteWordData(leoDriver, LEO_TOP_CSR_MUC_MAIL_BOX_CMD_ADDRESS, data);
}

size_t sendMailboxPayload(LeoI2CDriverType *leoDriver, uint32_t *payload, size_t inPayloadLen) {
  size_t idx = 0;
  size_t payloadLen = (inPayloadLen > 16) ? 16 : inPayloadLen;
  // printPayload(payload, payloadLen);
  for (idx = 0; idx < payloadLen; idx++) {
    /* LeoMailboxInfo_s.payloadReg.payload[idx] = payload.payload[idx]; */
    leoWriteWordData(leoDriver, (LEO_TOP_CSR_MUC_MAIL_BOX_DATA_ADDRESS + (idx << 2)),
                     payload[idx]);
  }
  return idx;
}

uint32_t checkDoorbell(LeoI2CDriverType *leoDriver) {
  uint32_t data;
  leoReadWordData(leoDriver, LEO_TOP_CSR_MUC_MAIL_BOX_CMD_ADDRESS, &data);
  return ((data >> 16) & 1);
}

MailboxStatusType getMailboxStatus(LeoI2CDriverType *leoDriver) {
  uint32_t data;
  leoReadWordData(leoDriver, LEO_TOP_CSR_MUC_MAIL_BOX_STATUS_ADDRESS, &data);
  return (data > 24) & 0xf;
}

size_t getReturnData(LeoI2CDriverType *leoDriver, uint32_t *data,
                     size_t expDataLen) {
  size_t i;
  uint32_t read_data;

  for (i = 0; i < expDataLen; i++) {
    leoReadWordData(leoDriver, (LEO_TOP_CSR_MUC_MAIL_BOX_DATA_ADDRESS + (i << 2)),
                    &read_data);
    /* ASTERA_INFO("Get return data: read data: %08x\n", read_data); */
    data[i] = read_data;
  }
  return expDataLen;
}

MailboxStatusType execOperation(LeoI2CDriverType *leoDriver, uint32_t addr,
                                uint32_t cmd, uint32_t *dataIn, size_t inPayloadLen,
                                uint32_t *dataOut, size_t expReturnDataLen) {
  size_t dataLen = (inPayloadLen == 0) ? expReturnDataLen : inPayloadLen;
  size_t retLen = 0;
  uint32_t buffer[4];
  int i;

  /* ASTERA_INFO("MM: in execOperation: cmd: 0x%x addr: 0x%x\n", cmd, addr); */
  if (LeoMailboxInfo_s.operationInProgress == 1) {
    return AL_MM_STS_ERROR;
  }

  LeoMailboxInfo_s.operationInProgress = 1;

  /* ASTERA_INFO("MM: in execOperation: wait for doorbell\n"); */
  buffer[0] = waitForDoorbell(leoDriver);
  if (0 != buffer[0]) {
    ASTERA_ERROR(
        "MM: execOperation: timed out while waiting for doorbell to clear.\n");
    return AL_MM_STS_ERROR;
  }

  if ((inPayloadLen) && (dataIn != NULL)) {
    dataLen = sendMailboxPayload(leoDriver, dataIn, inPayloadLen);
  }
  sendMailboxCmd(leoDriver, addr, cmd, dataLen);

  buffer[0] = waitForDoorbell(leoDriver);
  if (0 != buffer[0]) {
    ASTERA_ERROR(
        "MM: execOperation: timed out while waiting for doorbell to clear.\n");
    return AL_MM_STS_ERROR;
  }

  buffer[1] = getMailboxStatus(leoDriver);
  if (AL_MM_STS_ERROR == buffer[1]) {
    ASTERA_ERROR("AL_MM_STS_ERROR(%d)\n", buffer[1]);
  }

  retLen = getReturnData(leoDriver, dataOut, expReturnDataLen);
  LeoMailboxInfo_s.operationInProgress = 0;
  return buffer[1]; // mailbox status
}

int waitForDoorbell(LeoI2CDriverType *leoDriver) {
  size_t i;
  uint32_t doorbell = 0;
  /* wait for door bell to be 0 */
  for (i = 0; i < LeoMailboxInfo_s.timeoutCnt; i++) {
    doorbell = checkDoorbell(leoDriver);
    if (0 == doorbell) {
      break;
    }
    if (leoDriver->pciefile != NULL)
      usleep(1000);
  }
  if (0 != doorbell) {
    LeoMailboxInfo_s.operationInProgress = 0;
    return -1;
  }
  return 0;
}

void printPayload(uint32_t *payload, size_t payloadLen) {
  size_t i;
  ASTERA_INFO("payload contents: \n\t");
  for (i = 0; i < payloadLen; i++) {
    if (i % 8 == 0) {
      ASTERA_INFO("\n\t");
    }
    ASTERA_INFO("%08x ", payload[i]);
  }
}

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
 * @file leo_mailbox.h
 * @brief Definition of enums and structs used by the mailbox.
 */
#ifndef _LEO_SDK_MAILBOX_H
#define _LEO_SDK_MAILBOX_H

#include "leo_api_types.h"
#include "leo_i2c.h"
#include "misc.h"


#include <stdint.h>

#define MMB_STS_OK 0x1
#define MMB_STS_PENDING 0x40000000
#define MMB_STS_ERROR 0x80000000


#define FW_API_MMB_CMD_OPCODE_MMB_CSR_READ 0x1
#define FW_API_MMB_CMD_OPCODE_MMB_CSR_WRITE 0x11
#define FW_API_MMB_CMD_OPCODE_MMB_PING 0x16
#define FW_API_MMB_CMD_OPCODE_MMB_FW_TESTS 0x17
#define FW_API_MMB_CMD_OPCODE_MMB_FW_CRC_VERIFY 0x21
#define FW_API_MMB_CMD_OPCODE_MMB_CHK_DIMM_TSOD 0x23
#define FW_API_MMB_CMD_OPCODE_MMB_CHK_DIMM_SPD_INFO 0x24
#define FW_API_MMB_CMD_OPCODE_MMB_DUMP_DIMM_SPD 0x25
#define FW_API_MMB_CMD_OPCODE_MMB_CHK_EEPROM_LEO_ID 0x26
#define FW_API_MMB_CMD_OPCODE_MMB_CHK_PERSISTENT_DATA 0x27
#define FW_API_MMB_CMD_OPCODE_MMB_GET_DDRPHY_TRAINING_MARGIN 0x28
#define FW_API_MMB_CMD_OPCODE_MMB_RD_EEPROM 0x29
#define FW_API_MMB_CMD_OPCODE_MMB_GET_RECENT_UART_RX 0x30
#define FW_API_MMB_CMD_OPCODE_MMB_GET_LEO_ERR_INFO 0x31
#define FW_API_MMB_CMD_OPCODE_MMB_ACCESS_DRAM_DBG_INTF 0x32
#define FW_API_MMB_CMD_OPCODE_MMB_SPPR 0x40

#define FW_API_MMB_PAYLOAD_MMB_FW_TESTS_FLASH_ACCESS_TEST_PAYLOAD 0x1
#define FW_API_MMB_PAYLOAD_MMB_FW_TESTS_PHY_CONTEXT_SEL_SWITCH_DEFAULT 0x0100
#define FW_API_MMB_PAYLOAD_MMB_FW_TESTS_PHY_CONTEXT_SEL_SWITCH_MPLL_A_B_DIRECT \
  0x0101
#define FW_API_MMB_RET_FW_TESTS_FLASH_ACCESS_TESTS_SUCCESS 0x0
#define FW_API_MMB_RET_FW_TESTS_FLASH_ACCESS_TESTS_FAIL 0xFFFFFFFF

#define CXL_BAR2_PMBOX_CAP_REG 0x01012000
#define CXL_BAR2_PMBOX_CTL_REG 0x01012004
#define CXL_BAR2_PMBOX_CMD_REG 0x01012008
#define CXL_BAR2_PMBOX_STS_REG 0x01012010
#define CXL_BAR2_PMBOX_BCS_REG 0x01012018
#define CXL_BAR2_PMBOX_PAYL_REG 0x01012020

#define CXL_PMBOX_GET_EVT_RECS       (0x0100)
#define CXL_PMBOX_CLR_EVT_RECS       (0x0101)
#define CXL_PMBOX_GET_INTR_POLICY (0x0102)
#define CXL_PMBOX_SET_INTR_POLICY (0x0103)
#define CXL_PMBOX_GET_POISON_LIST   (0x4300)
#define CXL_PMBOX_INJECT_POISON     (0x4301)
#define CXL_PMBOX_CLEAR_POISON      (0x4302)
#define CXL_PMBOX_GET_ALERT_CONFIG  (0x4201)
#define CXL_PMBOX_SET_ALERT_CONFIG  (0x4202)

#define CXL_PMBOX_GET_CMD_IN_LEN 0x1
#define CXL_PMBOX_CLR_CMD_IN_LEN 0x8
#define CXL_PMBOX_GET_CMD_LEN_SHIFT 16
#define CXL_PMBOX_CLR_CMD_LEN_SHIFT 16
#define CXL_PMBOX_CLRALL_PAYL_SZ (256)

#define CXL_PMBOX_INFO_LOG      (0)
#define CXL_PMBOX_WARN_LOG      (1)
#define CXL_PMBOX_FAILURE_LOG   (2)
#define CXL_PMBOX_FATAL_LOG     (3)

#define CXL_PMBOX_CLR_ALL 0x1
#define CXL_EVENT_REC_SZ 160

#define LEO_CXL_MBOX_MAX_PAYLOAD_SIZE (256)

typedef struct LeoMailboxInfo {
  int operationInProgress;
  size_t timeoutCnt;
  uint32_t *payloadReg;
} LeoMailboxInfoType;

typedef enum MailboxStatus {
  AL_MM_STS_SUCCESS = 0,
  AL_MM_STS_ERROR = 0x4,
} MailboxStatusType;

void sendMailboxCmd(LeoI2CDriverType *leoDriver, uint32_t addr, uint32_t cmd,
                    size_t payloadLen);
size_t sendMailboxPayload(LeoI2CDriverType *leoDriver, uint32_t *payload, size_t inPayloadLen);

uint32_t checkDoorbell(LeoI2CDriverType *leoDriver);
MailboxStatusType getMailboxStatus(LeoI2CDriverType *leoDriver);
size_t getReturnData(LeoI2CDriverType *leoDriver, uint32_t *data,
                     size_t expDataLen);
MailboxStatusType execOperation(LeoI2CDriverType *leoDriver, uint32_t addr,
                                uint32_t cmd, uint32_t *dataIn, size_t inPayloadLen,
                                uint32_t *dataOut, size_t expReturnDataLen);
void printPayload(uint32_t *payload, size_t payloadLen);
int waitForDoorbell(LeoI2CDriverType *leoDriver);

#endif // _LEO_SDK_MAILBOX_H

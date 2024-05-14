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
 * @file leo_scrb.c
 * @brief Implementation of Leo memory scrubbing APIs.
 */

#include "../include/leo_scrb.h"
#include "../include/leo_api.h"
#include "../include/misc.h"



void configBkgrdScrb(LeoI2CDriverType *leoDriver, uint32_t bkgrd_scrb_en,
                     uint64_t bkgrd_dpa_end_addr, uint32_t bkgrd_round_interval,
                     uint32_t bkgrd_cmd_interval,
                     uint32_t bkgrd_wrpoison_if_uncorr_err) {
  uint32_t bkgrd_scrb_cfg = 0x0;
  ASTERA_INFO("configScrb");

  // Disable the background scrubber
  bkgrd_scrb_cfg = bkgrd_round_interval & 0x3ffff;
  bkgrd_scrb_cfg = (bkgrd_scrb_cfg << 12) | (bkgrd_cmd_interval & 0xfff);
  bkgrd_scrb_cfg = (bkgrd_scrb_cfg << 1) | (bkgrd_wrpoison_if_uncorr_err & 0x1);
  bkgrd_scrb_cfg = (bkgrd_scrb_cfg << 1);

  ASTERA_DEBUG(
      "Write LEO_TOP_CSR_CMAL_CFG_BKGRD_SCRB_ADDRESS\nAddr(0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_CFG_BKGRD_SCRB_ADDRESS, bkgrd_scrb_cfg);
  leoWriteWordData(leoDriver, LEO_TOP_CSR_CMAL_CFG_BKGRD_SCRB_ADDRESS,
                   bkgrd_scrb_cfg);

  ASTERA_DEBUG(
      "Write LEO_TOP_CSR_CMAL_BKGRD_SCRB_DPA_RANGE_END_ADDR_W0_ADDRESS "
      "\nAddr(0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_BKGRD_SCRB_DPA_RANGE_END_ADDR_W0_ADDRESS,
      (uint32_t)(bkgrd_dpa_end_addr & 0xffffffff));
  leoWriteWordData(leoDriver,
                   LEO_TOP_CSR_CMAL_BKGRD_SCRB_DPA_RANGE_END_ADDR_W0_ADDRESS,
                   (bkgrd_dpa_end_addr & 0xffffffff));
  ASTERA_DEBUG(
      "Write LEO_TOP_CSR_CMAL_BKGRD_SCRB_DPA_RANGE_END_ADDR_W1_ADDRESS "
      "\nAddr(0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_BKGRD_SCRB_DPA_RANGE_END_ADDR_W1_ADDRESS,
      (uint32_t)((bkgrd_dpa_end_addr >> 32) & 0xffffffff));
  leoWriteWordData(leoDriver,
                   LEO_TOP_CSR_CMAL_BKGRD_SCRB_DPA_RANGE_END_ADDR_W1_ADDRESS,
                   ((bkgrd_dpa_end_addr >> 32) & 0xffffffff));

  // Enable the background scrubber
  bkgrd_scrb_cfg = bkgrd_round_interval & 0x3ffff;
  bkgrd_scrb_cfg = (bkgrd_scrb_cfg << 12) | (bkgrd_cmd_interval & 0xfff);
  bkgrd_scrb_cfg = (bkgrd_scrb_cfg << 1) | (bkgrd_wrpoison_if_uncorr_err & 0x1);
  bkgrd_scrb_cfg = (bkgrd_scrb_cfg << 1) | (bkgrd_scrb_en & 0x1);

  ASTERA_DEBUG(
      "Write LEO_TOP_CSR_CMAL_CFG_BKGRD_SCRB_ADDRESS\nAddr(0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_CFG_BKGRD_SCRB_ADDRESS, bkgrd_scrb_cfg);
  leoWriteWordData(leoDriver, LEO_TOP_CSR_CMAL_CFG_BKGRD_SCRB_ADDRESS,
                   bkgrd_scrb_cfg);
}

void configReqScrb(LeoI2CDriverType *leoDriver, uint32_t req_scrb_en,
                   uint64_t req_scrb_dpa_start_addr,
                   uint64_t req_scrb_dpa_end_addr,
                   uint32_t req_scrb_cmd_interval) {
  ASTERA_INFO("configScrb\n");

  ASTERA_DEBUG(
      "Write "
      "LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_START_ADDR_W0_ADDRESS\nAddr("
      "0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_START_ADDR_W0_ADDRESS,
      (uint32_t)(req_scrb_dpa_start_addr & 0xffffffff));
  leoWriteWordData(leoDriver,
                   LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_START_ADDR_W0_ADDRESS,
                   (req_scrb_dpa_start_addr & 0xffffffff));
  ASTERA_DEBUG(
      "Write "
      "LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_START_ADDR_W1_ADDRESS\nAddr("
      "0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_START_ADDR_W1_ADDRESS,
      (uint32_t)((req_scrb_dpa_start_addr >> 32) & 0xffffffff));
  leoWriteWordData(leoDriver,
                   LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_START_ADDR_W1_ADDRESS,
                   ((req_scrb_dpa_start_addr >> 32) & 0xffffffff));

  ASTERA_DEBUG("Write "
               "LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_END_ADDR_W0_ADDRESS\nAddr("
               "0x%x) Data(0x%x)",
               LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_END_ADDR_W0_ADDRESS,
               (uint32_t)(req_scrb_dpa_end_addr & 0xffffffff));
  leoWriteWordData(leoDriver,
                   LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_END_ADDR_W0_ADDRESS,
                   (req_scrb_dpa_end_addr & 0xffffffff));
  ASTERA_DEBUG("Write "
               "LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_END_ADDR_W1_ADDRESS\nAddr("
               "0x%x) Data(0x%x)",
               LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_END_ADDR_W1_ADDRESS,
               (uint32_t)((req_scrb_dpa_end_addr >> 32) & 0xffffffff));
  leoWriteWordData(leoDriver,
                   LEO_TOP_CSR_CMAL_REQ_SCRB_DPA_RANGE_END_ADDR_W1_ADDRESS,
                   ((req_scrb_dpa_end_addr >> 32) & 0xffffffff));

  ASTERA_DEBUG("Write LEO_TOP_CSR_CMAL_REQ_SCRB_CMD_INTERVAL_ADDRESS\n "
               "Addr(0x%x) Data(0x%x)",
               LEO_TOP_CSR_CMAL_REQ_SCRB_CMD_INTERVAL_ADDRESS,
               (req_scrb_cmd_interval & 0xfff));
  leoWriteWordData(leoDriver, LEO_TOP_CSR_CMAL_REQ_SCRB_CMD_INTERVAL_ADDRESS,
                   (req_scrb_cmd_interval & 0xfff));

  ASTERA_DEBUG(
      "Write LEO_TOP_CSR_CMAL_REQ_SCRB_EN_ADDRESS\n Addr(0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_REQ_SCRB_EN_ADDRESS, (req_scrb_en & 0x1));
  leoWriteWordData(leoDriver, LEO_TOP_CSR_CMAL_REQ_SCRB_EN_ADDRESS,
                   (req_scrb_en & 0x1));
}

void wait4ReqScrbDone(LeoI2CDriverType *leoDriver) {
  uint32_t rdData;
  uint32_t reqScrbDone;
  uint32_t time_taken = 0;
  ASTERA_INFO("wait4ReqScrbne\n");

  do {
    leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_REQ_SCRB_DONE_ADDRESS, &rdData);
    reqScrbDone = rdData & 0x1;
    time_taken += 2;
    usleep(200);

    if (rdData == 0)
      ASTERA_INFO("Waiting for req scrb done %d\n", time_taken);
  } while (reqScrbDone == 0);
  ASTERA_INFO("Poll:LEO_TOP_CSR_CMAL_REQ_SCRB_DONE_ADDRESS Data:0x%x", rdData);
}

void readScrbStatus(LeoDeviceType *device) {
  LeoI2CDriverType *leoDriver = device->i2cDriver;
  uint32_t rdData, bkgrd_scrb_cnt;
  uint32_t wrReqLowCnt, rdReqLowCnt, wrRdupperCnt;
  uint32_t wrReqCnt0, wrReqCnt1, wrReqCnt2, wrReqCnt3, wrSubchnlCnt;
  uint32_t rdReqCnt0, rdReqCnt1, rdReqCnt2, rdReqCnt3, rdSubchnlCnt;
  uint32_t stsOnDemandScrbCnt;
  leo_err_info_t leo_err_info = { 0 };

  ASTERA_INFO("readScrbStatus\n");

  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_STS_ONDMD_SCRB_CNT_ADDRESS,
                  &stsOnDemandScrbCnt);
  ASTERA_DEBUG(
      "Read LEO_TOP_CSR_CMAL_STS_ONDMD_SCRB_CNT_ADDRESS\nAddr(0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_STS_ONDMD_SCRB_CNT_ADDRESS, stsOnDemandScrbCnt);
  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN0_RREQ_CNT_ADDRESS,
                  &rdReqCnt0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt0);
  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN1_RREQ_CNT_ADDRESS,
                  &rdReqCnt1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN1_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt1);
  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN2_RREQ_CNT_ADDRESS,
                  &rdReqCnt2);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN2_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt2);
  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN3_RREQ_CNT_ADDRESS,
                  &rdReqCnt3);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN3_RREQ_CNT_ADDRESS Data:0x%x",
               rdReqCnt3);

  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN0_WREQ_CNT_ADDRESS,
                  &wrReqCnt0);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN0_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt0);
  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN1_WREQ_CNT_ADDRESS,
                  &wrReqCnt1);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN1_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt1);
  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN2_WREQ_CNT_ADDRESS,
                  &wrReqCnt2);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN2_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt2);
  leoReadWordData(leoDriver, LEO_TOP_CSR_CMAL_SUBCHN3_WREQ_CNT_ADDRESS,
                  &wrReqCnt3);
  ASTERA_DEBUG("Read:LEO_TOP_CSR_CMAL_SUBCHN3_WREQ_CNT_ADDRESS Data:0x%x",
               wrReqCnt3);
  wrSubchnlCnt = wrReqCnt0 + wrReqCnt1 + wrReqCnt2 + wrReqCnt3;
  rdSubchnlCnt = rdReqCnt0 + rdReqCnt1 + rdReqCnt2 + rdReqCnt3;

  ASTERA_DEBUG("-------------------------------------------------------\n");
  if (bkgrd_scrb_cnt == 0x0) {
    ASTERA_DEBUG("--------------------SCRB TEST PASS----------------------\n");
    fflush(stdout);
  } else {
    ASTERA_DEBUG("--------------------SCRB TEST FAIL----------------------\n");
    fflush(stdout);
  }
  ASTERA_DEBUG("-------------------------------------------------------\n");
  ASTERA_INFO("--------------------SCRB STATUS-------------------------\n");

  if (leoGetErrInfo(device, &leo_err_info) == LEO_SUCCESS) {
    ASTERA_INFO("Background scrub correctable error count (cumulative): %u", leo_err_info.scrb_corr_errcnt);
    ASTERA_INFO("Background scrub uncorrectable error count (cumulative): %u", leo_err_info.scrb_uncorr_errcnt);
    ASTERA_INFO("Background scrub other error count (cumulative): %u", leo_err_info.scrb_other_errcnt);
  }

  ASTERA_INFO("%s\t :0x%x", "OnDemand scrb served  cnt",
              (stsOnDemandScrbCnt)&0xff);
  ASTERA_INFO("%s\t :0x%x", "OnDemand scrb dropped cnt",
              (stsOnDemandScrbCnt >> 8) & 0xff);
  ASTERA_DEBUG("%s\t :0x%x", "wr requests count(subchnl_cnt)", wrSubchnlCnt);
  ASTERA_DEBUG("%s\t :0x%x", "rd requests count(subchnl_cnt)", rdSubchnlCnt);
  ASTERA_INFO("-------------------------------------------------------\n");
  fflush(stdout);
}

void configOnDemandScrb(LeoI2CDriverType *leoDriver, uint32_t onDemand_scrb_en,
                        uint32_t wrpoison_if_uncorr_err) {
  uint32_t cfg_ondemand;
  ASTERA_INFO("configOndemandScrb");

  // Enable the onDemand scrubber
  cfg_ondemand = wrpoison_if_uncorr_err & 0x1;
  cfg_ondemand = (cfg_ondemand << 1) | (onDemand_scrb_en & 0x1);

  ASTERA_DEBUG(
      "Write LEO_TOP_CSR_CMAL_CFG_ONDMD_SCRB_ADDRESS \nAddr(0x%x) Data(0x%x)",
      LEO_TOP_CSR_CMAL_CFG_ONDMD_SCRB_ADDRESS, cfg_ondemand);
  leoWriteWordData(leoDriver, LEO_TOP_CSR_CMAL_CFG_ONDMD_SCRB_ADDRESS,
                   cfg_ondemand);
}

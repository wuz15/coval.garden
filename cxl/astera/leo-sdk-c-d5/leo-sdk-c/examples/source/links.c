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

/*
 * @file links.c
 * @brief Helper functions used by link status examples
 */

#include "../include/links.h"
#include "../../include/leo_api.h"
#include "../../include/leo_i2c.h"
#include "../../include/leo_logger.h"

void writeLogToFile(FILE *fp, LeoLTSSMEntryType *entry) {
  fprintf(fp, "            {\n");
  fprintf(fp, "                'data': %d,\n", entry->data);
  fprintf(fp, "                'offset': %d\n", entry->offset);
  fprintf(fp, "            },\n");
}

LeoErrorType leoPrintMicroLogs(LeoLinkType *link) {
  LeoErrorType rc;
  char filename[25];
  FILE *fp;

  int startLane = leoGetStartLane(link);

  // Create link_logs folder if it doesnt exist
  struct stat st;
  if (stat("link_logs", &st) == -1) {
    mkdir("link_logs", 0700);
  }
  sprintf(filename, "link_logs/ltssm_log_%d.py", link->config.linkId);

  fp = fopen(filename, "w");
  fprintf(fp, "# AUTOGENERATED FILE. DO NOT EDIT #\n");
  fprintf(fp, "# GENERATED WTIH C SDK VERSION %s #\n\n\n", leoGetSDKVersion());

  fprintf(fp, "fw_version_major = %d\n", link->device->fwVersion.major);
  fprintf(fp, "fw_version_minor = %d\n", link->device->fwVersion.minor);
  fprintf(fp, "fw_version_build = %d\n", link->device->fwVersion.build);

  // Initialise logger
  rc = leoLTSSMLoggerInit(link, 0, LEO_LTSSM_VERBOSITY_HIGH);
  if (rc != LEO_SUCCESS) {
    fprintf(fp, "# Encountered error during "
                "leoPrintMicroLogs->leoLTSSMLoggerInit. Closing file.\n");
    fclose(fp);
    return rc;
  }

  // Enable Macros to print
  rc = leoLTSSMLoggerPrintEn(link, 1);
  if (rc != LEO_SUCCESS) {
    fprintf(fp, "# Encountered error during "
                "leoPrintMicroLogs->leoLTSSMLoggerPrintEn. Closing file.\n");
    fclose(fp);
    return rc;
  }

  fprintf(fp, "leo_micro_logs = [\n");
  fprintf(fp, "    {\n");
  fprintf(fp, "        'log_type': %d,\n", LEO_LTSSM_LINK_LOGGER);
  fprintf(fp, "        'log': [      # Open MM log\n");
  // Print Main micro log
  rc = leoPrintLog(link, LEO_LTSSM_LINK_LOGGER, fp);
  fprintf(fp, "        ],    # Close MM log\n");
  fprintf(fp, "    },\n");
  if (rc != LEO_SUCCESS) {
    fprintf(fp, "# Encountered error during leoPrintMicroLogs->leoPrintLog. "
                "Closing file.\n");
    fclose(fp);
    return rc;
  }

  // Print path micro logs
  int laneNum;
  int laneIdx;
  rc = LEO_SUCCESS;
  for (laneIdx = 0; laneIdx < link->state.width; laneIdx++) {
    laneNum = laneIdx + startLane;
    fprintf(fp, "    {\n");
    fprintf(fp, "        'log_type': %d,\n", laneNum);
    fprintf(fp, "        'log': [      # Open PM %d log\n", laneNum);
    rc |= leoPrintLog(link, laneNum, fp);
    CHECK_SUCCESS(rc);
    fprintf(fp, "        ],    # Close PM %d log\n", laneNum);
    fprintf(fp, "    },\n");
  }
  fprintf(fp, "]\n");

  if (rc != LEO_SUCCESS) {
    fprintf(fp, "# Encountered error during leoPrintMicroLogs->leoPrintLog. "
                "Closing file.\n");
    fclose(fp);
    return rc;
  }
  fclose(fp);
  return LEO_SUCCESS;
}

// Capture the detailed link state and print it to file
LeoErrorType leoPrintLinkDetailedState(LeoLinkType *link) {
  LeoErrorType rc;
  char fname[40];
  FILE *fp;

  // Check overall device health
  rc = leoCheckDeviceHealth(link->device);
  CHECK_SUCCESS(rc);

  // Get Link state
  rc = leoGetLinkStateDetailed(&link[0]);
  CHECK_SUCCESS(rc);

  // Create "link_logs" directory if it doesnt exist
  struct stat st = {0};
  if (stat("link_logs", &st) == -1) {
    mkdir("link_logs", 0700);
  }

  int startLane = leoGetStartLane(link);

  // Write link state detailed output to a file
  // Must have link_state_detailed dir in leo-sdk-c
  // File is printed as a python dict so that we can post-process it to a
  // readable format as a table
  sprintf(fname, "link_logs/link_state_detailed_log_%d.py",
          link->config.linkId);

  fp = fopen(fname, "w");
  fprintf(fp, "# AUTOGENERATED FILE. DO NOT EDIT #\n");
  fprintf(fp, "# GENERATED WTIH C SDK VERSION %s #\n\n\n", leoGetSDKVersion());

  // Leo device struct parameters
  fprintf(fp, "leo_device = {}\n");
  int b = 0;
  for (b = 0; b < 12; b += 1) {
    fprintf(fp, "leo_device['chipID_%d'] = %d\n", b, link->device->chipID[b]);
  }
  for (b = 0; b < 6; b += 1) {
    fprintf(fp, "leo_device['lotNumber_%d'] = %d\n", b,
            link->device->lotNumber[b]);
  }
  fprintf(fp, "leo_device['deviceOkay'] = %s\n",
          link->device->deviceOkay ? "True" : "False");
  fprintf(fp, "leo_device['allTimeMaxTempC'] = %f\n", link->device->maxTempC);
  fprintf(fp, "leo_device['currentTempC'] = %f\n", link->device->currentTempC);
  fprintf(fp, "leo_device['tempAlertThreshC'] = %f\n\n",
          link->device->tempAlertThreshC);

  // Leo link struct parameters
  fprintf(fp, "leo_link = {}\n");
  fprintf(fp, "leo_link['link_id'] = %d\n", link->config.linkId);
  fprintf(fp, "leo_link['linkOkay'] = %s\n",
          link->state.linkOkay ? "True" : "False");
  fprintf(fp, "leo_link['state'] = %d\n", link->state.state);
  fprintf(fp, "leo_link['width'] = %d\n", link->state.width);
  fprintf(fp, "leo_link['linkMinFoM'] = %d\n", link->state.linkMinFoM);
  fprintf(fp, "leo_link['linkMinFoMRx'] = '%s'\n", link->state.linkMinFoMRx);
  fprintf(fp, "leo_link['recoveryCount'] = %d\n", link->state.recoveryCount);
  // fprintf(fp, "leo_link['uspp_speed'] = %2.1f\n", link->state.usppSpeed);
  // fprintf(fp, "leo_link['dspp_speed'] = %2.1f\n", link->state.dsppSpeed);
  fprintf(fp, "leo_link['uspp'] = {}\n");
  fprintf(fp, "leo_link['uspp']['tx'] = {}\n");
  fprintf(fp, "leo_link['uspp']['rx'] = {}\n");
  fprintf(fp, "leo_link['dspp'] = {}\n");
  fprintf(fp, "leo_link['dspp']['tx'] = {}\n");
  fprintf(fp, "leo_link['dspp']['rx'] = {}\n");
  fprintf(fp, "leo_link['rt_core'] = {}\n");
  fprintf(fp, "leo_link['rt_core']['us'] = {}\n");
  fprintf(fp, "leo_link['rt_core']['ds'] = {}\n");

  int phyLaneNum;
  int laneIndex;
  for (laneIndex = 0; laneIndex < link->state.width; laneIndex++) {
    phyLaneNum = laneIndex + startLane;

    fprintf(fp, "\n\n### Stats for logical lane %d (physical lane %d) ###\n",
            laneIndex, phyLaneNum);

    fprintf(fp, "leo_link['uspp']['tx'][%d] = {}\n", laneIndex);

    fprintf(fp, "leo_link['uspp']['tx'][%d]['logical_lane'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].logicalLaneNum);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['physical_pin'] = '%s'\n",
            laneIndex,
            link->state.usppState.txState[laneIndex].physicalPinName);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['de'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].de);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['pre'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].pre);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['cur'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].cur);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['pst'] = %f\n", laneIndex,
            link->state.usppState.txState[laneIndex].pst);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['last_eq_rate'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].lastEqRate);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.usppState.txState[laneIndex].lastPresetReq);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['last_pre_req'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].lastPreReq);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['last_cur_req'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].lastCurReq);
    fprintf(fp, "leo_link['uspp']['tx'][%d]['last_pst_req'] = %d\n", laneIndex,
            link->state.usppState.txState[laneIndex].lastPstReq);
    fprintf(fp, "\n");

    fprintf(fp, "leo_link['uspp']['rx'][%d] = {}\n", laneIndex);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['logical_lane'] = %d\n", laneIndex,
            link->state.usppState.rxState[laneIndex].logicalLaneNum);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['physical_pin'] = '%s'\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].physicalPinName);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['termination'] = %d\n", laneIndex,
            link->state.usppState.rxState[laneIndex].termination);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['polarity'] = %d\n", laneIndex,
            link->state.usppState.rxState[laneIndex].polarity);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['att_db'] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].attdB);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['ctle_boost_db'] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].ctleBoostdB);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['ctle_pole'] = %d\n", laneIndex,
            link->state.usppState.rxState[laneIndex].ctlePole);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['vga_db'] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].vgadB);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'] = {}\n", laneIndex);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][1] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe1);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][2] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe2);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][3] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe3);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][4] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe4);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][5] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe5);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][6] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe6);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][7] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe7);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['dfe'][8] = %f\n", laneIndex,
            link->state.usppState.rxState[laneIndex].dfe8);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_eq_rate'] = %d\n", laneIndex,
            link->state.usppState.rxState[laneIndex].lastEqRate);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.usppState.rxState[laneIndex].lastPresetReq);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req_fom'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFom);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req_m1'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqM1);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req_fom_m1'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFomM1);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req_m2'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqM2);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req_fom_m2'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFomM2);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req_m3'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqM3);
    fprintf(fp, "leo_link['uspp']['rx'][%d]['last_preset_req_fom_m3'] = %d\n",
            laneIndex,
            link->state.usppState.rxState[laneIndex].lastPresetReqFomM3);
    fprintf(fp, "\n");

    fprintf(fp, "leo_link['rt_core']['us'][%d] = {}\n", laneIndex);
    /*
        fprintf(fp, "leo_link['rt_core']['us'][%d]['tj_c'] = %2.2f\n",
            laneIndex, link->state.coreState.usppTempC[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['us'][%d]['skew_ns'] = %d\n",
            laneIndex, link->state.coreState.usDeskewNs[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['us'][%d]['tj_c_alert'] = %d\n",
            laneIndex, link->state.coreState.usppTempAlert[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['us'][%d]['pth_fw_state'] = %d\n",
            laneIndex, link->state.coreState.usppPathFWState[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['us'][%d]['pth_hw_state'] = %d\n",
            laneIndex, link->state.coreState.usppPathHWState[laneIndex]);

        fprintf(fp, "leo_link['rt_core']['ds'][%d] = {}\n", laneIndex);
        fprintf(fp, "leo_link['rt_core']['ds'][%d]['tj_c'] = %2.2f\n",
            laneIndex, link->state.coreState.dsppTempC[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['ds'][%d]['skew_ns'] = %d\n",
            laneIndex, link->state.coreState.dsDeskewNs[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['ds'][%d]['tj_c_alert'] = %d\n",
            laneIndex, link->state.coreState.dsppTempAlert[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['ds'][%d]['pth_fw_state'] = %d\n",
            laneIndex, link->state.coreState.dsppPathFWState[laneIndex]);
        fprintf(fp, "leo_link['rt_core']['ds'][%d]['pth_hw_state'] = %d\n",
            laneIndex, link->state.coreState.dsppPathHWState[laneIndex]);
    */
    fprintf(fp, "\n");

    fprintf(fp, "leo_link['dspp']['tx'][%d] = {}\n", laneIndex);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['logical_lane'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].logicalLaneNum);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['physical_pin'] = '%s'\n",
            laneIndex,
            link->state.dsppState.txState[laneIndex].physicalPinName);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['de'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].de);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['pre'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].pre);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['cur'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].cur);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['pst'] = %f\n", laneIndex,
            link->state.dsppState.txState[laneIndex].pst);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['last_eq_rate'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].lastEqRate);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.dsppState.txState[laneIndex].lastPresetReq);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['last_pre_req'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].lastPreReq);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['last_cur_req'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].lastCurReq);
    fprintf(fp, "leo_link['dspp']['tx'][%d]['last_pst_req'] = %d\n", laneIndex,
            link->state.dsppState.txState[laneIndex].lastPstReq);
    fprintf(fp, "\n");

    fprintf(fp, "leo_link['dspp']['rx'][%d] = {}\n", laneIndex);

    fprintf(fp, "leo_link['dspp']['rx'][%d]['logical_lane'] = %d\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].logicalLaneNum);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['physical_pin'] = '%s'\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].physicalPinName);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['termination'] = %d\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].termination);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['polarity'] = %d\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].polarity);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['att_db'] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].attdB);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['ctle_boost_db'] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].ctleBoostdB);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['ctle_pole'] = %d\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].ctlePole);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['vga_db'] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].vgadB);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'] = {}\n", laneIndex);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][1] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe1);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][2] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe2);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][3] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe3);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][4] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe4);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][5] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe5);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][6] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe6);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][7] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe7);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['dfe'][8] = %f\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].dfe8);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_eq_rate'] = %d\n", laneIndex,
            link->state.dsppState.rxState[laneIndex].lastEqRate);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req'] = %d\n",
            laneIndex, link->state.dsppState.rxState[laneIndex].lastPresetReq);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req_fom'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFom);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req_m1'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqM1);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req_fom_m1'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFomM1);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req_m2'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqM2);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req_fom_m2'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFomM2);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req_m3'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqM3);
    fprintf(fp, "leo_link['dspp']['rx'][%d]['last_preset_req_fom_m3'] = %d\n",
            laneIndex,
            link->state.dsppState.rxState[laneIndex].lastPresetReqFomM3);
    fprintf(fp, "\n");
  }
  fclose(fp);

  return LEO_SUCCESS;
}

// Print the micro logger entries
LeoErrorType leoPrintLog(LeoLinkType *link, LeoLTSSMLoggerEnumType log,
                         FILE *fp) {
  LeoLTSSMEntryType ltssmEntry;
  int offset = 0;
  int oneBatchModeEn;
  int oneBatchWrEn;
  int batchComplete;
  int currFmtID;
  int currWriteOffset;
  LeoErrorType rc;
  int full = 0;

  ltssmEntry.logType = log;

  // Buffer size different for main and path micros
  int bufferSize;
  if (log == LEO_LTSSM_LINK_LOGGER) {
    bufferSize = LEO_MM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE;
  } else {
    bufferSize = LEO_PM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE;
  }

  // Get One batch mode enable
  rc = leoGetLoggerOneBatchModeEn(link, log, &oneBatchModeEn);
  CHECK_SUCCESS(rc);

  // Get One batch write enable
  rc = leoGetLoggerOneBatchWrEn(link, log, &oneBatchWrEn);
  CHECK_SUCCESS(rc);

  if (oneBatchModeEn == 0) {
    // Stop Micros from printing
    rc = leoLTSSMLoggerPrintEn(link, 0);
    CHECK_SUCCESS(rc);

    // In this mode we print the logger from current write offset
    // and reset the offset to zero once we reach the end of the buffer
    // Capture current write offset
    rc = leoGetLoggerWriteOffset(link, log, &currWriteOffset);
    CHECK_SUCCESS(rc);
    // Start offset from the current (paused) offset
    offset = currWriteOffset;

    // Print logger from current offset
    while (offset < bufferSize) {
      rc = leoLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
      CHECK_SUCCESS(rc);
      writeLogToFile(fp, &ltssmEntry);
    }

    // Wrap around and continue reading the log entries
    if (currWriteOffset != 0) {
      // Reset offset to start from beginning
      offset = 0;

      // Print logger from start of print buffer
      while (offset < currWriteOffset) {
        // Read logger entry
        rc = leoLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
        CHECK_SUCCESS(rc);
        writeLogToFile(fp, &ltssmEntry);
      }
    }

    // Enable Macros to print
    rc = leoLTSSMLoggerPrintEn(link, 1);
    CHECK_SUCCESS(rc);
  } else {
    // Check if batch is complete
    batchComplete = (oneBatchModeEn == 1) && (oneBatchWrEn == 0);

    // Read format ID at current offset
    rc = leoGetLoggerFmtID(link, log, offset, &currFmtID);
    CHECK_SUCCESS(rc);

    if (batchComplete == 1) {
      full = 1;
      while ((currFmtID != 0) && (offset < bufferSize)) {
        // Get logger entry
        rc = leoLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
        CHECK_SUCCESS(rc);
        writeLogToFile(fp, &ltssmEntry);
        // Read Fmt ID
        rc = leoGetLoggerFmtID(link, log, offset, &currFmtID);
        CHECK_SUCCESS(rc);
      }
    } else {
      // Print from start of the buffer until the end
      while ((currFmtID != 0) && (offset < bufferSize) &&
             (offset < currWriteOffset)) {
        // Get logger entry
        rc = leoLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
        CHECK_SUCCESS(rc);
        writeLogToFile(fp, &ltssmEntry);

        // Read Fmt ID
        rc = leoGetLoggerFmtID(link, log, offset, &currFmtID);
        CHECK_SUCCESS(rc);

        // Read current write offset
        rc = leoGetLoggerWriteOffset(link, log, &currWriteOffset);
        CHECK_SUCCESS(rc);
      }
    }
  }

  if (full == 0) {
    ASTERA_INFO("There is more to print ...");
  }

  return LEO_SUCCESS;
}

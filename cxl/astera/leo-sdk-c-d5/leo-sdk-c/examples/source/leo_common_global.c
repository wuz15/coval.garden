/*
 * Copyright 2023 Astera Labs, Inc. or its affiliates. All Rights Reserved.
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
 * @file leo_common_global.c
 * @brief Function used common across the example files.
 * This is recommended for:
 *       - Leo print event records
 */

#include "../../include/DW_apb_ssi.h"
#include "../../include/leo_api.h"
#include "../../include/leo_common.h"
#include "../../include/leo_error.h"
#include "../../include/leo_evt_rec_mgr.h"
#include "../../include/leo_globals.h"
#include "../../include/leo_i2c.h"
#include "../../include/leo_spi.h"
#include "../include/aa.h"
#include "../include/board.h"
#include "../include/libi2c.h"

#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *log_str[4] = { "INFORMATIONAL", "WARNING", "FAILURE", "FATAL" };

/* 601dcbb3-9c06-4eab-b8af-4e9bfb5c9624 – DRAM Event Record UUID */
static const uint32_t dram_evt_UUID[4] = { 0xB3CB1D60, 0xAB4E069C, 0x9B4EAFB8, 0x24965CFB };
/* fe927475-dd59-4339-a586-79bab113b774 – Memory Module Event Record UUID */
static const uint32_t mem_evt_UUID[4] = { 0x757492FE, 0X394359DD, 0XBA7986A5, 0X74B713B1 };

static void print_dram_events(leo_one_evt_log_t *event, int rec_idx)
{
  char *txn_type = "Unknown";
  char *evt_type = "Unknown";
  int validity = 0;

  ASTERA_INFO("\tusing DRAM Event Record Format");

  ASTERA_INFO("\t\tevent[%d] handle: %d", rec_idx, event->rec.handle);
  ASTERA_INFO("\t\tevent[%d] timestamp = %llu",
      rec_idx, event->rec.timestamp.qw);
  ASTERA_INFO("\t\tevent[%d] dpa = 0x%llx",
      rec_idx, event->rec.evt_data.dram_rec.dpa.qw);
  ASTERA_INFO("\t\tevent[%d] uncorr_event = %s", rec_idx,
      event->rec.evt_data.dram_rec.uncorr_evt ? "1" : "0");
  ASTERA_INFO("\t\tevent[%d] threshold_evt = %s", rec_idx,
      event->rec.evt_data.dram_rec.threshold_evt ? "1" : "0");
  ASTERA_INFO("\t\tevent[%d] poison_list_overflow = %s", rec_idx,
      event->rec.evt_data.dram_rec.poison_list_overflow ? "1" : "0");

  switch (event->rec.evt_data.dram_rec.evt_type) {
  case 0:
    evt_type = "Media ECC error";
    break;
  case 1:
    evt_type = "Scrub Media ECC error";
    break;
  case 2:
    evt_type = "Invalid Address";
    break;
  case 3:
    evt_type = "Data Path Error";
    break;
  default:
    evt_type = "Unknown";
    break;
  }
  ASTERA_INFO("\t\tevent[%d] event type = %s", rec_idx, evt_type);

  switch (event->rec.evt_data.dram_rec.txn_type) {
  case 0:
    txn_type = "Unknown/Unreported";
    break;
  case 1:
    txn_type = "Host Read";
    break;
  case 2:
    txn_type = "Host Write";
    break;
  case 3:
    txn_type = "Host Scan Media";
    break;
  case 4:
    txn_type = "Host Inject Poison";
    break;
  case 5:
    txn_type = "Internal Media Scrub";
    break;
  case 6:
    txn_type = "Internal Media Management";
    break;
  default:
    evt_type = "Invalid";
    break;
  }
  ASTERA_INFO("\t\tevent[%d] transaction type = %s", rec_idx,
      txn_type);

  validity = *(uint16_t *)(event->rec.evt_data.dram_rec.validity);

  if (validity & 0x1) {
    ASTERA_INFO("\t\tevent[%d] transaction channel = 0x%x", rec_idx,
                event->rec.evt_data.dram_rec.channel);
  }
  if (validity & 0x2) {
    ASTERA_INFO("\t\tevent[%d] transaction rank = 0x%x", rec_idx,
                event->rec.evt_data.dram_rec.rank);
  }
  if (validity & 0x4) {
    ASTERA_INFO(
        "\t\tevent[%d] transaction nibble mask = 0x%x", rec_idx,
        *(uint32_t *)event->rec.evt_data.dram_rec.nibble_mask & 0xffffff);
  }
  if (validity & 0x8) {
    ASTERA_INFO("\t\tevent[%d] transaction bank group = 0x%x",
                rec_idx, event->rec.evt_data.dram_rec.bank_group);
  }
  if (validity & 0x10) {
    ASTERA_INFO("\t\tevent[%d] transaction bank = 0x%x", rec_idx,
                event->rec.evt_data.dram_rec.bank);
  }
  if (validity & 0x20) {
    ASTERA_INFO("\t\tevent[%d] transaction row = 0x%x", rec_idx,
                *(uint32_t *)event->rec.evt_data.dram_rec.row & 0xffffff);
  }
  if (validity & 0x40) {
    ASTERA_INFO("\t\tevent[%d] transaction column = 0x%x", rec_idx,
                *(uint32_t *)event->rec.evt_data.dram_rec.col & 0xffff);
  }
  if (validity & 0x80) {
    ASTERA_INFO("\t\tevent[%d] correction mask is valid", rec_idx);
  }
}


void printevent(leo_one_evt_log_t *event, int log)
{
  static int rec_idx = 1;

  ASTERA_INFO("%s event log:", log_str[log]);

  ASTERA_INFO("\tevent log overflow             : %d", event->overflow);
  ASTERA_INFO("\tmore events records            : %d", event->more_evt_recs);
  ASTERA_INFO("\tevent overflow record count    : %u", event->overflow_err_cnt);
  ASTERA_INFO("\tevent overflow first timestamp : %llu", event->first_overflow_time.qw);
  ASTERA_INFO("\tevent overflow last timestamp  : %llu", event->last_overflow_time.qw);
  ASTERA_INFO("\tevent record count             : %u", event->evt_rec_cnt);

  if (!event->evt_rec_cnt) {
    return;
  }

  if (!memcmp(&(event->rec.id[0]), &dram_evt_UUID[0], 16)) {
    print_dram_events(event, rec_idx);
  } else if (!memcmp(&(event->rec.id[0]), &mem_evt_UUID[0], 16)) {
    ASTERA_INFO("Memory Module Event Record Formart not supported yet");
  }

  rec_idx++;
  if (!event->more_evt_recs) {
    rec_idx = 1;
  }
}

uint32_t populate_handles(char *str_handles, uint16_t *handles)
{
  const char delim[2] = ",";
  char *token;
  uint32_t idx = 0;
  if(handles==NULL){
    ASTERA_ERROR("Error NULL handles");
    return(idx);
  }
  token = strtok(str_handles, delim);
  while ((token != NULL) && (idx < MAX_EVT_RECS)) {
    handles[idx] = atoi(token);
    ASTERA_ERROR("handles[idx]:%d[%d]",handles[idx], idx);
    idx++;
    token = strtok(NULL, delim);
  }

  return (idx);
}

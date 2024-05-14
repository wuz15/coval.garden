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
 * @file leo_mbox_cmds.h
 * @brief Definition related to suported CXL mailbox commands
 */


#ifndef _LEO_MBOX_CMDS_H_
#define _LEO_MBOX_CMDS_H_

#include "misc.h"

/* poison list error record */
typedef struct media_err_rec {
  qword_t dpa;
  uint32_t len; // must be non-zero, no of adjescent DPAs
  uint32_t rsvd;
} media_err_rec_t;

/* Poison record list, to be used for output payload to get_poison_list mailbox
 * command */
typedef struct cxl_poison_list {
  uint8_t
      more_err_recs : 1; // more records in log then host can fetch in payload
  uint8_t overflow : 1;  // poison list overflow occurred
  uint8_t scan_in_progress : 1; // media scan in progress
  uint8_t rsvd0 : 5;
  uint8_t rsvd1;
  uint8_t overflow_timestamp[8]; // poison list overflow timestamp
  uint8_t err_cnt[2];            // no of error record in poison list payload
  uint32_t rsvd2[5];
  media_err_rec_t err_rec[16];
} cxl_poison_list_t;

#endif

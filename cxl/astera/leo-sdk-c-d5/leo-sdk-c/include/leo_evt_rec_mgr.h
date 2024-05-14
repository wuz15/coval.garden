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
 * @file leo_evt_rec_mgr.h
 * @brief definitions related to Leo supported CXL event log/records
 */


#ifndef _EVT_REC_MGR_H
#define _EVT_REC_MGR_H

#include "misc.h"

#define MAX_EVT_RECS        (16)

//******************************************************************************
// Enums
//******************************************************************************
typedef enum evt_type {
  INFO_EVENT = 0,
  WARN_EVENT,
  FAILURE_EVENT,
  FATAL_EVENT
} evt_type_e;

typedef enum dram_evt_type {
  MEDIA_ECC_ERR = 0,
  SCRUB_MEDIA_ECC_ERR,
  INVALID_ADDR,
  DATA_PATH_ERR
} dram_evt_type_e;

typedef enum dram_transaction_type {
  UNKNOWN = 0,
  HOST_READ,
  HOST_WRITE,
  HOST_SCAN_MEDIA,
  HOST_INJECT_POISON,
  INTERNAL_MEDIA_SCRUB,
  INTERNAL_MEDIA_MGT
} dram_transaction_type_e;

typedef enum media_err_rec_type {
  UNKNOWN_ERR = 0,
  EXTERNAL_POISON,
  INTERNAL_POISON,
  INJECTED_POISON,
  VENDOR_SPECIFIC
} media_err_rec_type_e;

typedef enum evt_rec_type {
  GEN_MEDIA_EVT_REC,
  DRAM_EVT_REC,
  MEM_MODULE_EVT_REC
} evt_rec_type_e;

//******************************************************************************
// Structures
//******************************************************************************

/* DRAM related event record */
typedef struct dram_evt_rec {
  qword_t dpa;
  uint8_t uncorr_evt : 1;
  uint8_t threshold_evt : 1;
  uint8_t poison_list_overflow : 1;
  uint8_t rsvd0 : 5;
  uint8_t evt_type;
  uint8_t txn_type;
  uint8_t validity[2];
  uint8_t channel;
  uint8_t rank;
  uint8_t nibble_mask[3];
  uint8_t bank_group;
  uint8_t bank;
  uint8_t row[3];
  uint8_t col[2];
  uint8_t corr_mask[32];
  uint8_t rsvd1[23];
} dram_evt_rec_t;

/* general device related event record */
typedef struct mem_mdl_evt_rec {
  uint8_t evt_type;
  uint8_t health_sts;
  uint8_t media_sts;
  uint8_t extra_sts;
  uint8_t life_used;
  uint8_t dev_temp[2];
  uint8_t dirty_shn_cnt[4];
  uint8_t corr_vol_err_cnt[4];
  uint8_t corr_pers_err_cnt[4];
  uint8_t rsvd[61];
} mem_mdl_evt_rec_t;

/* general media related event record */
typedef struct gen_media_evt_rec {
  qword_t     dpa;
  uint8_t     uncorr_evt              : 1;    /* 1=uncorr, 0=corr */
  uint8_t     threshold_evt           : 1;
  uint8_t     poison_list_overflow    : 1;
  uint8_t     rsvd0                   : 5;
  uint8_t     evt_type;           /* memory event type, media or scrub */
  uint8_t     txn_type;           /* host rw or scrub rw or poison injection */
  uint8_t     validity[2];
  uint8_t     channel;
  uint8_t     rank;
  uint8_t     device[3];
  uint8_t     comp_id[16];
  uint8_t     rsvd1[46];
} gen_media_evt_rec_t;

typedef union evt_spc_data {
  uint32_t data[20];
  dram_evt_rec_t dram_rec;
  mem_mdl_evt_rec_t mem_mdl_rec;
  gen_media_evt_rec_t media_rec;
} evt_spc_data_t;

/* common event record */
typedef struct leo_evt_rec {
  uint32_t id[4];        /* Specific Event Record format UUID */
  uint32_t len : 8;      /* Event Record length including data and all fields */
  uint32_t severity : 2; /* Severity flag */
  uint32_t pc : 1;       /* Permanent Condition flag */
  uint32_t mn : 1;       /* Maintenance Needed flag */
  uint32_t pd : 1;       /* Performance Degraded flag */
  uint32_t hrn : 1;      /* Hardware Replacement Needed flag */
  uint32_t rsvd0 : 18;
  uint32_t handle : 16;         /* non-zero Unique Handle for Event Record */
  uint32_t related_handle : 16; /* Handle of another event of same type and log,
                                   Optional */
  qword_t timestamp;        /* nanoseconds since 01-Jan-1970, Optional */
  uint32_t rsvd[4];
  evt_spc_data_t evt_data; /* Specific Event log type Event Record */
} leo_evt_rec_t;

/**
 * Leo event log:
 * return payload structure for CXL mbox cmd Get_Event_Records (opcode: 0100h)
 */
typedef struct leo_one_evt_log {
  uint32_t overflow : 1;      /* log overflow occurred */
  uint32_t more_evt_recs : 1; /* more evt rec available to fetch than can fit in
                                 payload */
  uint32_t rsvd0 : 6;
  uint32_t rsvd1 : 8;
  uint32_t overflow_err_cnt : 16;  /* no of events occurred since overflow */
  qword_t first_overflow_time; /* timestamp of first event that caused
                                      overflow */
  qword_t last_overflow_time;  /* timestamp of last event detected since
                                      overflow */
  uint32_t evt_rec_cnt : 16; /* no of event records in list, 0 = no events present */
  uint32_t rsvd2 : 16;
  uint32_t rsvd3[2];
  leo_evt_rec_t rec;
} __attribute__((packed, aligned(1))) leo_one_evt_log_t;

/** Leo error info
 * for FW housekeeping
 */
typedef struct leo_err_info {
  uint32_t    corr_ecc_errcnt;            /* correctable error count, cumulative */
  uint32_t    uncorr_ecc_errcnt;          /* uncorrectable error count, cumulative*/
  uint32_t    scrb_corr_errcnt;
  uint32_t    scrb_uncorr_errcnt;
  uint32_t    scrb_other_errcnt;
} __attribute__((packed, aligned(1))) leo_err_info_t;
#endif

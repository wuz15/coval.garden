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
 * @file leo_cxl_mailbox_test.c
 * @brief api test demonstrates leo CXL mailbox features.
 * This is recommended for:
 *       - Print SDK version
 *       - Iinitialize Leo device
 *       - Read, print FW version and status check
 *       - Link status check (Get details on the Speed, Width, linkstatus and
 * Mode)
 *       - Get/Clear event records using Linux Open Source IOCTL calls
 *       - Get/Set Alert Config using Linux Open Source IOCTL calls
 *       - sudo ./leo_cxl_mailbox_test -help
 */

#include "../include/DW_apb_ssi.h"
#include "../include/leo_api.h"
#include "../include/leo_common.h"
#include "../include/leo_error.h"
#include "../include/leo_globals.h"
#include "include/board.h"
#include "../include/leo_evt_rec_mgr.h"
#include "include/leo_common_global.h"

#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/cxl_mem.h>
#include <poll.h>
#include<pthread.h>

#define BIT_SET(pos)                            (1UL<<(pos))
#define TEST_BIT_SET(value, bit)                (value&BIT_SET(bit))
#define SIGN_BIT_CLEAR_VAL(bits_limit)          (BIT_SET(bits_limit-1)-1)

#define AL_LEO_DEVICE "/dev/cxl/mem"

#define AL_INTR_POLICY_MSI_SET 0x01
#define AL_INTR_POLICY_MSI_MASK 0x01

#define GET_EVT_REC_PAYLOAD_SIZE 0xA0
#define CLR_EVT_REC_PAYLOAD_SIZE 0x8
#define SET_ALERT_PAYLOAD_SIZE 0x0C
#define GET_ALERT_PAYLOAD_SIZE 0x10

typedef struct evRecArgs {
  uint16_t action;
  uint16_t logtype;
  int clrAll;
  uint32_t numRecs; 
  uint16_t handles[MAX_EVT_RECS];
  char str_handles[256];
  char device;
} evRecArgsType;

typedef struct cxl_set_alert_config {
  uint8_t     valid_alert_actions;
  uint8_t     enable_alert_actions;
  uint8_t     life_used_warning_threshold;
  uint8_t     rsvd;
  uint16_t    dev_over_temp_warning_threshold;
  uint16_t    dev_under_temp_warning_threshold;
  uint16_t    volatile_mem_corr_err_threshold;
  uint16_t    persistent_mem_corr_err_threshold;
} cxl_set_alert_config_t;

typedef struct cxl_get_alert_config {
  uint8_t     valid_alerts;
  uint8_t     programmable_alerts;
  uint8_t     life_used_critical_threshold;
  uint8_t     life_used_warning_threshold;
  uint16_t    dev_over_temp_critical_threshold;
  uint16_t    dev_under_temp_critical_threshold;
  uint16_t    dev_over_temp_warning_threshold;
  uint16_t    dev_under_temp_warning_threshold;
  uint16_t    volatile_mem_corr_err_threshold;
  uint16_t    persistent_mem_corr_err_threshold;
} cxl_get_alert_config_t;

int16_t do2sComplementToDecimal(uint16_t value, uint8_t bits_limit) {

  if ( TEST_BIT_SET(value,(bits_limit-1)) )
    return (int16_t)(((~value)+1) & SIGN_BIT_CLEAR_VAL(bits_limit));

  return -((int16_t)value);

}

/* Global to share with thread function, to keep example code simple*/
static evRecArgsType gEvRecArgs;
static cxl_set_alert_config_t gAlertConfig;

LeoErrorType doLeoEdacTest(evRecArgsType evRecArgs);
LeoErrorType doLeoEdacRun(evRecArgsType evRecArgs, cxl_set_alert_config_t alertConfig);
LeoErrorType doLeoGetinput(void);
LeoErrorType doCxlCom(struct cxl_send_command cxl_payload);

const char *help_string[] = {DEFAULT_HELPSTRINGS,
                              "action, (optional) get -or- clear",
                              "logtype, (optional) info, warn, failure or fatal",
                              "clrall, (optional) to clear all records alltogether",
                              "numrecs, (optional) no of event records to clear, valid only if clrall is not used",
                              "handles, (optional) comma separate event handles list, valid only if clrall is not used",
                              "device, (optional) device node path ex:0 or 1 or 2 etc...",
                              "over_temp, (optional) Device Over-Temperature Programmable Warning Threshold Enable Alert, supported 0-125ºC",
                              "under_temp, (optional) Device Under-Temperature Programmable Warning Threshold Enable Alert, supported 0-125ºC",
                              "corr_threshold, (optional) Corrected Volatile Memory Error Programmable Warning Threshold Enable Alert"};

static LeoErrorType doCxlGetPolicy(void);
static LeoErrorType doCxlSetPolicy(void);
static LeoErrorType doCxlGetEvents(int logt);
static LeoErrorType doCxlClrEvents(evRecArgsType evRecArgs);
static LeoErrorType doCxlSetAlertConfig(cxl_set_alert_config_t alertConfig);
static LeoErrorType doCxlGetAlertConfig(cxl_get_alert_config_t *getalertConfig);

int main(int argc, char *argv[]) {
  int i2cBus = 1;
  LeoErrorType status = LEO_SUCCESS;
  int option_index;
  int option;
  DefaultArgsType defaultArgs = {.leoAddress = LEO_DEV_LEO_0,
                                 .switchAddress = LEO_DEV_MUX,
                                 .switchHandle = -1,
                                 .switchMode = 0x03, // 0x03 for Leo 0
                                 .serialnum = NULL,
                                 .bdf = NULL};

  int leoHandle;
  char *leoSbdf = NULL;
  int isMemDevice = 0;
  int isGetEvt = 0;
  int isSetEvt = 0;
  int isClrEvt = 0;
  int isAlert = 0;

  uint32_t num_handles = 0;
  cxl_set_alert_config_t alertConfig;
  cxl_get_alert_config_t getalertConfig;

  enum { DEFAULT_ENUMS, ENUM_ACTION_e, ENUM_LOGTYPE_e, ENUM_CLRALL_e, ENUM_NUMRECS_e, ENUM_HANDLE_e, ENUM_DEVICE_e, ENUM_OTEMPA_e, ENUM_UTEMPA_e, ENUM_CTD_e, ENUM_EOL_e };

  struct option long_options[] = {DEFAULT_OPTIONS,
                                  {"action", required_argument, 0, 0},
                                  {"logtype", required_argument, 0, 0},
                                  {"clrall", no_argument, 0, 0},
                                  {"numrecs", required_argument, 0, 0},
                                  {"handles", required_argument, 0, 0},
                                  {"device", required_argument, 0, 0},
                                  {"over_temp", required_argument, 0, 0},
                                  {"under_temp", required_argument, 0, 0},
                                  {"corr_threshold", required_argument, 0, 0},
                                  {0, 0, 0, 0}};
  
//asteraLogSetLevel(ASTERA_LOG_LEVEL_INFO); // setting print level INFO
  memset(&gEvRecArgs, 0, sizeof (evRecArgsType));
  memset(&alertConfig, 0, sizeof(cxl_set_alert_config_t));
  memset(&getalertConfig, 0, sizeof(cxl_get_alert_config_t));
  while (1) {
    option = getopt_long_only(argc, argv, "h", long_options, &option_index);
    if (option == -1)
      break;

    switch (option) {
    case 'h':
      usage(argv[0], long_options, help_string);
      break;
    case 0:
      switch (option_index) {
        DEFAULT_SWITCH_CASES(defaultArgs, long_options, help_string)
      case ENUM_ACTION_e:
        if (!strncmp(optarg, "get", 3)){
          gEvRecArgs.action = GET_EVENTS;
          isGetEvt = 1;
        }
        else if (!strncmp(optarg, "clear", 5)){
          gEvRecArgs.action = CLEAR_EVENTS;
          isClrEvt = 1;
        }
        else if (!strncmp(optarg, "set", 3)){
          gEvRecArgs.action = GET_EVENTS;
          isSetEvt = 1;
        }
        break;
      case ENUM_LOGTYPE_e:
        if (!strncmp(optarg, "warn", 4))
          gEvRecArgs.logtype = CXL_PMBOX_WARN_LOG;
        else if (!strncmp(optarg, "fatal", 5))
          gEvRecArgs.logtype = CXL_PMBOX_FATAL_LOG;
        else if (!strncmp(optarg, "info", 4))
          gEvRecArgs.logtype = CXL_PMBOX_INFO_LOG;
        else if (!strncmp(optarg, "failure", 7))
          gEvRecArgs.logtype = CXL_PMBOX_FAILURE_LOG;
        else
          gEvRecArgs.logtype = -1;
        break;
      case ENUM_CLRALL_e:
          gEvRecArgs.clrAll = 1;
        break;
      case ENUM_NUMRECS_e:
          gEvRecArgs.numRecs = atoi(optarg);
        break;
      case ENUM_HANDLE_e:
        memset(gEvRecArgs.handles, 0, MAX_EVT_RECS*sizeof(uint16_t));
        strcpy(gEvRecArgs.str_handles, optarg);
        num_handles = populate_handles(gEvRecArgs.str_handles, gEvRecArgs.handles);
          if(!num_handles)
            return LEO_FAILURE;
        break;
      case ENUM_DEVICE_e:
        gEvRecArgs.device = *optarg;
        isMemDevice = 1;
      break;
      case ENUM_OTEMPA_e:
        if(optarg){
          alertConfig.valid_alert_actions |= ALERT_CFG_OT_MASK;
          alertConfig.enable_alert_actions |= ALERT_CFG_OT_MASK;
          alertConfig.dev_over_temp_warning_threshold = atoi(optarg);
        }
        isAlert = 1;
      break;
      case ENUM_UTEMPA_e:
        if(optarg){
          alertConfig.valid_alert_actions |= ALERT_CFG_UT_MASK;
          alertConfig.enable_alert_actions |= ALERT_CFG_UT_MASK;
          alertConfig.dev_under_temp_warning_threshold = atoi(optarg);
        }
        isAlert = 1;
      break;
      case ENUM_CTD_e:
        if(optarg){
          alertConfig.valid_alert_actions |= ALERT_CFG_CVMEM_MASK;
          alertConfig.enable_alert_actions |= ALERT_CFG_CVMEM_MASK;
          alertConfig.volatile_mem_corr_err_threshold = atoi(optarg);
        }
        isAlert = 1;
      break;
      default:
        ASTERA_ERROR("option_index = %d undecoded", option_index);
      } // switch (option_index)
      break;
    default:
      ASTERA_ERROR("Default option = %d", option);
      usage(argv[0], long_options, help_string);
    }
  }
  if(!isMemDevice){
    gEvRecArgs.device = '0';
  }

    if(isAlert){
      ASTERA_INFO("\n'\n User Input Values\
        \nvalid_alert_actions:                 0x%x\
        \nenable_alert_actions:                0x%x\
        \nlife_used_warning_threshold:         %d\
        \ndev_over_temp_warning_threshold:     %d°c\
        \ndev_under_temp_warning_threshold:    %d°c\
        \nvolatile_mem_corr_err_threshold:     %d \n'\n",\
        alertConfig.valid_alert_actions, \
        alertConfig.enable_alert_actions, alertConfig.life_used_warning_threshold, \
        alertConfig.dev_over_temp_warning_threshold, alertConfig.dev_under_temp_warning_threshold, \
        alertConfig.volatile_mem_corr_err_threshold);
        isAlert = false;
      if(isGetEvt){
        doCxlGetAlertConfig(&getalertConfig);
        isGetEvt = false;
      } else if(isSetEvt){
        doCxlSetAlertConfig(alertConfig);
        isSetEvt = false;
      }
    }

  if(isGetEvt){
      status = doCxlGetEvents(gEvRecArgs.logtype);
      if(status != LEO_SUCCESS)
        goto errMain;
  }
  if(isClrEvt){
    status = doCxlClrEvents(gEvRecArgs);
  }
  status = doLeoGetinput();
  if(status != LEO_SUCCESS)
    goto errMain;
  errMain:
  ASTERA_INFO("Exiting status:%x", status);
}

LeoErrorType doLeoGetinput(void) {

  LeoErrorType status = LEO_SUCCESS;
  char feature;
  int logt, err, input;
  int* pthread;
  uint32_t num_handles = 0;
  cxl_set_alert_config_t alertConfig;
  cxl_get_alert_config_t getalertConfig;

  ASTERA_INFO(" \n'\t\t\t\t\t\t---> Press '1' - Get Interrupt policy', \
                  \n'\t\t\t\t\t\t---> Press '2' - Set Interrupt policy', \
                  \n'\t\t\t\t\t\t---> Press '3' - Get event records', \
                  \n'\t\t\t\t\t\t---> Press '4' - clear event records' \
                  \n'\t\t\t\t\t\t---> Press '5' - Get All (Warn, Fatal and clear) event records' \
                  \n'\t\t\t\t\t\t---> Press '6' - Get Alert configuration' \
                  \n'\t\t\t\t\t\t---> Press '7' - Set Alert configuration' \
                  \n\t\t\t\t\t\tpress Ctrl+D to stop");
  gEvRecArgs.action = GET_ALL_EVENTS; //Default get all events set
  gEvRecArgs.logtype = ALL_LOGS;
  while ((feature = getc(stdin)) != EOF){
    ASTERA_INFO(" \n'\t\t\t\t\t\t---> Press '1' - Get Interrupt policy', \
                  \n'\t\t\t\t\t\t---> Press '2' - Set Interrupt policy', \
                  \n'\t\t\t\t\t\t---> Press '3' - Get event records', \
                  \n'\t\t\t\t\t\t---> Press '4' - clear event records' \
                  \n'\t\t\t\t\t\t---> Press '5' - Get All (Warn, Fatal and clear) event records' \
                  \n'\t\t\t\t\t\t---> Press '6' - Get Alert configuration' \
                  \n'\t\t\t\t\t\t---> Press '7' - Set Alert configuration' \
                  \n'\t\t\t\t\t\tpress Ctrl+D to stop");

    switch (feature) {
      case '1':
        ASTERA_INFO("\n\t\t\t\t\t\t---> Get Interrupt policy");
        gEvRecArgs.action = GET_INTR_POLICY;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        gEvRecArgs.action = GET_ALL_EVENTS;
        gEvRecArgs.logtype = logt;
        break;
      case '2':
        ASTERA_INFO("\n\t\t\t\t\t\t---> Set Interrupt policy");
        gEvRecArgs.action = SET_INTR_POLICY;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        gEvRecArgs.action = GET_ALL_EVENTS;
        gEvRecArgs.logtype = logt;
        break;
      case '3':
        ASTERA_INFO("\n\t\t\t\t\t\t---> press '0'-Info \t'1'-warning \t'2'-Failure \t'3'-Fatal");
        scanf("%d", &logt);
        gEvRecArgs.action = GET_EVENTS;
        gEvRecArgs.logtype = logt;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        break;
      case '4':
        ASTERA_INFO("\n\t\t\t\t\t\t---> press '0'-Info \t'1'-warning \t'2'-Failure \t'3'-Fatal");
        scanf("%d", &logt);
        gEvRecArgs.logtype = logt;
        ASTERA_INFO("\n\t\t\t\t\t\t---> Enter number of records to clear, '0' for clear all:");
        scanf("%d", &gEvRecArgs.numRecs);
        if(gEvRecArgs.numRecs){
          memset(gEvRecArgs.handles, 0, MAX_EVT_RECS*sizeof(uint16_t));
          ASTERA_INFO("\n\t\t\t\t\t\t---> Enter handles ',' separated:");
          scanf("%s", gEvRecArgs.str_handles);
          num_handles = populate_handles(gEvRecArgs.str_handles, gEvRecArgs.handles);
          if(!num_handles)
            return status;
        } else {
          gEvRecArgs.clrAll = 1;
        }
        gEvRecArgs.action = CLEAR_EVENTS;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        gEvRecArgs.action = GET_ALL_EVENTS;
        gEvRecArgs.logtype = logt;
        break;
      case '5':
        ASTERA_INFO("\n\t\t\t\t\t\t---> Get All Events");
        gEvRecArgs.action = GET_ALL_EVENTS;
        gEvRecArgs.logtype = 1;
        gEvRecArgs.action = GET_EVENTS;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        gEvRecArgs.action = GET_EVENTS;
        gEvRecArgs.logtype = 3;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        gEvRecArgs.action = CLEAR_EVENTS;
        gEvRecArgs.logtype = 1;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        gEvRecArgs.action = CLEAR_EVENTS;
        gEvRecArgs.logtype = 3;
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        break;
      case '6':
        ASTERA_INFO("\n\t\t\t\t\t\t---> Set Alert Configuration");
        gEvRecArgs.action = GET_ALERT_CFG;
        memset(&getalertConfig, 0, sizeof(cxl_get_alert_config_t));

        status = doCxlGetAlertConfig(&getalertConfig);
        gEvRecArgs.action = GET_ALL_EVENTS;
        gEvRecArgs.logtype = logt;
        break;
      case '7':
        ASTERA_INFO("\n\t\t\t\t\t\t---> Set Alert Configuration");
        gEvRecArgs.action = SET_ALERT_CFG;
        memset(&alertConfig, 0, sizeof(cxl_set_alert_config_t));
        ASTERA_INFO("\n\t\t\t\t\t\t---> Enter '0' for disable all \n'1' for \
          Device Over-Temperature Programmable Warning Threshold Enable \
          \n'2' for Device Under-Temperature Programmable Warning Threshold Enable \
          \n'3' for Corrected Volatile Memory Error Programmable Warning Threshold Enable \
          \n'4' for Over & Under Temperature Enable");
        scanf("%d", &input);

        if(4 == input){
          alertConfig.valid_alert_actions |= ALERT_CFG_ALL_MASK;
          alertConfig.enable_alert_actions |= ALERT_CFG_ALL_MASK;
          ASTERA_INFO("\n\t\t\t\t\t\t---> Enter Over Temperature threshold");
          scanf("%hu", &alertConfig.dev_over_temp_warning_threshold);
          ASTERA_INFO("\n\t\t\t\t\t\t---> Enter Under Temperature threshold");
          scanf("%hu", &alertConfig.dev_under_temp_warning_threshold);
          ASTERA_INFO("\n\t\t\t\t\t\t---> Enter Corrected Volatile Memory Error");
          scanf("%hu", &alertConfig.volatile_mem_corr_err_threshold);

        } else if(1 == input){
          alertConfig.valid_alert_actions |= ALERT_CFG_OT_MASK;
          alertConfig.enable_alert_actions |= ALERT_CFG_OT_MASK;
          ASTERA_INFO("\n\t\t\t\t\t\t---> Enter Over Temperature threshold");
          scanf("%hu", &alertConfig.dev_over_temp_warning_threshold);

        } else if(2 == input){
          alertConfig.valid_alert_actions |= ALERT_CFG_UT_MASK;
          alertConfig.enable_alert_actions |= ALERT_CFG_UT_MASK;
          ASTERA_INFO("\n\t\t\t\t\t\t---> Enter Under Temperature threshold");
          scanf("%hu", &alertConfig.dev_under_temp_warning_threshold);
        } else if(3 == input){
          alertConfig.valid_alert_actions |= ALERT_CFG_CVMEM_MASK;
          alertConfig.enable_alert_actions |= ALERT_CFG_CVMEM_MASK;
          ASTERA_INFO("\n\t\t\t\t\t\t---> Enter Corrected Volatile Memory Error Threshold");
          scanf("%hu", &alertConfig.volatile_mem_corr_err_threshold);
        } else {
          alertConfig.enable_alert_actions = 0;
          alertConfig.valid_alert_actions = 0;
        }
        status = doLeoEdacRun(gEvRecArgs, alertConfig);
        break;
      default:
        break;
    }
  }
  return status;
}

LeoErrorType doLeoEdacRun(evRecArgsType evRecArgs, cxl_set_alert_config_t alertConfig){
  LeoErrorType status = LEO_SUCCESS;
  int cnt;

  if(evRecArgs.action == GET_INTR_POLICY) {
    /*Get Interrupt Policy*/
    doCxlGetPolicy();

  } else if(evRecArgs.action == SET_INTR_POLICY) {
    /*Set interrupt Policy*/
    doCxlSetPolicy();
   } else if (evRecArgs.action == GET_EVENTS) {
    /*Get event record*/
    doCxlGetEvents(evRecArgs.logtype);
  } else if (evRecArgs.action == CLEAR_EVENTS) {
    /*Clear event record*/
    doCxlClrEvents(evRecArgs);

  } else if (evRecArgs.action == SET_ALERT_CFG) {
    /*Clear event record*/
    doCxlSetAlertConfig(alertConfig);

  } else {
      ASTERA_ERROR("Leo unknown action requested, please see help [-h]");
    }
  return status;
}

LeoErrorType doCxlCom(struct cxl_send_command cxl_payload){
  
  LeoErrorType status = LEO_SUCCESS;
  int leoFd = -1;
  char deviceNode[50];
  
  strcpy(deviceNode, AL_LEO_DEVICE);
  strncat(deviceNode, &gEvRecArgs.device,1);
  ASTERA_INFO("The LEO device node: = %s", deviceNode);

  leoFd = open(deviceNode, O_RDWR);
  if (leoFd == -1){
      ASTERA_ERROR("\nError could not open CXL mem");
      return LEO_FAILURE;
  }

  status = ioctl(leoFd, CXL_MEM_SEND_COMMAND, (struct cxl_send_command *)&cxl_payload);
  if(status != 0)
    ASTERA_ERROR("\n%x IOCTL return status", status);

  close(leoFd);
  return status;
}

static LeoErrorType doCxlGetPolicy(void)
{
  /*Get Interrupt Policy*/
  LeoErrorType status = LEO_SUCCESS;
  struct cxl_send_command cxl_payload;
  uint8_t outbuf[256];
  leo_one_evt_log_t *eventlog = (leo_one_evt_log_t *)outbuf;
  uint8_t infoevt;
  uint8_t warnevt;
  uint8_t failevt;
  uint8_t fatalevt; 

  memset(&cxl_payload, 0, sizeof(cxl_payload));
  cxl_payload.id  = CXL_MEM_COMMAND_ID_RAW;
  cxl_payload.flags = 0x0;
  cxl_payload.raw.opcode = CXL_PMBOX_GET_INTR_POLICY;
  cxl_payload.in.size = 0;

  memset(outbuf, 0, 256);
  
  cxl_payload.out.size = 0x4;
  cxl_payload.out.payload = (uint64_t)outbuf;
  doCxlCom(cxl_payload);
  infoevt = outbuf[0];
  warnevt = outbuf[1];
  failevt = outbuf[2];
  fatalevt = outbuf[3];

  if(!(infoevt & AL_INTR_POLICY_MSI_MASK)){
    ASTERA_INFO("Info Interrupt policy MSIx not supported:    0x%x",infoevt);
  }
  if(!(warnevt & AL_INTR_POLICY_MSI_MASK)){
    ASTERA_INFO("Warning Interrupt policy MSIx not supported: 0x%x",warnevt);
  }
  if(!(failevt & AL_INTR_POLICY_MSI_MASK)){
    ASTERA_INFO("Fail Interrupt policy MSIx not supported:    0x%x",failevt);
  }
  if(!(fatalevt & AL_INTR_POLICY_MSI_MASK)){
    ASTERA_INFO("Fatal Interrupt policy MSIx not supported:   0x%x",fatalevt);
  }

  ASTERA_INFO("\
  \nInformational Event Log Interrupt settings: 0x%x\
  \nWarning Event Log Interrupt settings:       0x%x\
  \nFail Event Log Interrupt settings:          0x%x\
  \nFatal Event Log Interrupt settings:         0x%x\n",\
  infoevt, warnevt, failevt, fatalevt);
  return status;

}

static LeoErrorType doCxlSetPolicy(void)
{
    /*Set Interrupt Policy*/
    struct cxl_send_command cxl_payload;
    uint8_t outbuf[256];
    leo_one_evt_log_t *eventlog = (leo_one_evt_log_t *)outbuf;
    LeoErrorType status = LEO_SUCCESS;

    memset(&cxl_payload, 0, sizeof(cxl_payload));
    cxl_payload.id  = CXL_MEM_COMMAND_ID_RAW;
    cxl_payload.flags = 0x0;
    cxl_payload.raw.opcode = CXL_PMBOX_SET_INTR_POLICY;
    cxl_payload.in.size = 4;

    memset(outbuf, 0, 256);
    outbuf[0] = AL_INTR_POLICY_MSI_SET; //MSIx
    outbuf[1] = AL_INTR_POLICY_MSI_SET; //MSIx
    outbuf[2] = AL_INTR_POLICY_MSI_SET; //MSIx
    outbuf[3] = AL_INTR_POLICY_MSI_SET; //MSIx

    cxl_payload.in.payload = (uint64_t)outbuf;
    cxl_payload.out.size = 0;
    cxl_payload.out.payload = (uint64_t)outbuf;
    doCxlCom(cxl_payload);
    return status;

}

static LeoErrorType doCxlGetEvents(int logt)
{
    /*Get event record*/
    struct cxl_send_command cxl_payload;
    uint8_t outbuf[256];
    leo_one_evt_log_t *eventlog = (leo_one_evt_log_t *)outbuf;
    LeoErrorType status = LEO_SUCCESS;

  do{
      memset(&cxl_payload, 0, sizeof(cxl_payload));
      cxl_payload.id  = CXL_MEM_COMMAND_ID_RAW;
      cxl_payload.flags = 0x0;
      cxl_payload.raw.opcode = CXL_PMBOX_GET_EVT_RECS;
      cxl_payload.in.size = 1;

      memset(outbuf, 0, 256);
      outbuf[0] = logt; //1 log type CE/warn log
      cxl_payload.in.payload = (uint64_t)outbuf;
      cxl_payload.out.size = GET_EVT_REC_PAYLOAD_SIZE;
      cxl_payload.out.payload = (uint64_t)outbuf;
      doCxlCom(cxl_payload);
      printevent(eventlog, logt);
    }while (eventlog->more_evt_recs); 

    ASTERA_INFO("%s events retrieved","WARN_LOG or FATAL_LOG");
    ASTERA_INFO("Leo CXL GET_EVENTS_RECORDS successful");
    return status;
}

static LeoErrorType doCxlClrEvents(evRecArgsType evRecArgs)
{
    /*Clear event record*/
    struct cxl_send_command cxl_payload;
    uint8_t outbuf[256];
    leo_one_evt_log_t *eventlog = (leo_one_evt_log_t *)outbuf;
    LeoErrorType status = LEO_SUCCESS;

    memset(&cxl_payload, 0, sizeof(cxl_payload));
    cxl_payload.id  = CXL_MEM_COMMAND_ID_RAW;
    cxl_payload.flags = 0x0;
    cxl_payload.raw.opcode = CXL_PMBOX_CLR_EVT_RECS;
    
    memset(outbuf, 0, 256);
    outbuf[0] = evRecArgs.logtype; //log type CE/warn log
    if(evRecArgs.clrAll){
      ASTERA_INFO("Leo CXL clrall received");
      outbuf[1] = 1; //Clear all events
      cxl_payload.in.size = CLR_EVT_REC_PAYLOAD_SIZE;
    } else {
      outbuf[2] = evRecArgs.numRecs;
      memcpy(outbuf+6, evRecArgs.handles, evRecArgs.numRecs*sizeof(uint16_t));
      cxl_payload.in.size = 6 + (evRecArgs.numRecs*sizeof(uint16_t));
    }
  
    cxl_payload.in.payload = (uint64_t)outbuf;
    cxl_payload.out.size = 0;
    cxl_payload.out.payload = (uint64_t)outbuf;
    status = doCxlCom(cxl_payload);
    if(status == LEO_SUCCESS){
      ASTERA_INFO("Events cleared and set to 'Get All Events'");
    }
    gEvRecArgs.action = GET_ALL_EVENTS;
    return status;

}

static LeoErrorType doCxlSetAlertConfig(cxl_set_alert_config_t alertConfig)
{
    /*Clear event record*/
    struct cxl_send_command cxl_payload;
    uint8_t outbuf[256];
    LeoErrorType status = LEO_SUCCESS;
    cxl_get_alert_config_t getalertConfig;
    
    //doCxlGetAlertConfig(&getalertConfig);

    memset(&cxl_payload, 0, sizeof(cxl_payload));
    //cxl_payload.id  = CXL_MEM_COMMAND_ID_SET_ALERT_CONFIG;
    cxl_payload.id  = CXL_MEM_COMMAND_ID_RAW;
    cxl_payload.flags = 0x0;
    cxl_payload.raw.opcode = CXL_PMBOX_SET_ALERT_CONFIG;
    cxl_payload.in.size = SET_ALERT_PAYLOAD_SIZE;

    memset(outbuf, 0, 256);
    if(ALERT_TEMP_MAX < alertConfig.dev_over_temp_warning_threshold){
      alertConfig.dev_over_temp_warning_threshold = ALERT_TEMP_MAX;
    } else if (ALERT_TEMP_MIN > alertConfig.dev_over_temp_warning_threshold){
      alertConfig.dev_over_temp_warning_threshold = ALERT_TEMP_MIN;
    }
    
    if(ALERT_TEMP_MAX < alertConfig.dev_under_temp_warning_threshold){
      alertConfig.dev_under_temp_warning_threshold = ALERT_TEMP_MAX;
    }else if (ALERT_TEMP_MIN> alertConfig.dev_under_temp_warning_threshold){
      alertConfig.dev_under_temp_warning_threshold = ALERT_TEMP_MIN;
    }

    cxl_payload.in.payload = (uint64_t)(&alertConfig);
    cxl_payload.out.size = 0;
    cxl_payload.out.payload = (uint64_t)outbuf;

    status = doCxlCom(cxl_payload);
    if(status == LEO_SUCCESS){
      ASTERA_INFO("Set Alert Configuration done");
    }
    gEvRecArgs.action = GET_ALL_EVENTS;
    doCxlGetAlertConfig(&getalertConfig);
    return status;

}

static LeoErrorType doCxlGetAlertConfig(cxl_get_alert_config_t *getalertConfig)
{
    /*Clear event record*/
    struct cxl_send_command cxl_payload;
    uint8_t outbuf[256];
    LeoErrorType status = LEO_SUCCESS;

    memset(&cxl_payload, 0, sizeof(cxl_payload));
    cxl_payload.id  = CXL_MEM_COMMAND_ID_RAW;
    cxl_payload.flags = 0x0;
    cxl_payload.raw.opcode = CXL_PMBOX_GET_ALERT_CONFIG;
    cxl_payload.in.size = 0x0;

    memset(outbuf, 0, 256);

    cxl_payload.in.payload = (uint64_t)outbuf;
    cxl_payload.out.size = GET_ALERT_PAYLOAD_SIZE;
    cxl_payload.out.payload = (uint64_t)(getalertConfig);
    
    status = doCxlCom(cxl_payload);

    ASTERA_INFO("\n'\nGet Alert Configuration:\
      \nvalid_alerts:                         0x%x\
      \nprogrammable_alerts:                  0x%x\
      \nlife_used_critical_threshold:         %d\
      \nlife_used_warning_threshold:          %d\
      \ndev_over_temp_critical_threshold:     %d°c\
      \ndev_under_temp_critical_threshold:    %d°c\
      \ndev_over_temp_warning_threshold:      %d°c\
      \ndev_under_temp_warning_threshold:     %d°c\
      \nvolatile_mem_corr_err_threshold:      %d\
      \npersistent_mem_corr_err_threshold:    %d\n'\n",\
      getalertConfig->valid_alerts, \
      getalertConfig->programmable_alerts, getalertConfig->life_used_critical_threshold, \
      getalertConfig->life_used_warning_threshold, do2sComplementToDecimal(getalertConfig->dev_over_temp_critical_threshold, 16), \
      do2sComplementToDecimal(getalertConfig->dev_under_temp_critical_threshold, 16), \
      do2sComplementToDecimal(getalertConfig->dev_over_temp_warning_threshold, 16), \
      do2sComplementToDecimal(getalertConfig->dev_under_temp_warning_threshold, 16), \
      getalertConfig->volatile_mem_corr_err_threshold, \
      getalertConfig->persistent_mem_corr_err_threshold);    

    if(status == LEO_SUCCESS){
      ASTERA_INFO("Set Alert Configuration done");
    }
    gEvRecArgs.action = GET_ALL_EVENTS;
    return status;

}
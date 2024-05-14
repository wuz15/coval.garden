/******************************************************************************
 *
 *  srmtUserKernelSharedData.h
 *
 * Copyright (C) 2017 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *   Author Name <cjparsons@soft-forge.com>
 *
******************************************************************************/

/*!
*****************************************************************************
\file srmtUserKernelSharedData.h

\brief Contains the data and definitions common for SRMT-Linux
       kernel module and user application parts

\par NOTES:

*****************************************************************************
\par REVISION HISTORY:

07/18/2014       borisv          Original Draft
07/06/2017       mbogochow       Added ioctl definitions
*****************************************************************************/
#ifndef SRMT_USERKERNEL_DATA_HH_
#define SRMT_USERKERNEL_DATA_HH_

#define MODULE_NAME "srmt_module"   // create /proc/{MODULE_NAME}
#define CSTATE_COMMAND_NAME   "cstate_command"  // create /proc/{MODULE_NAME}/{CSTATE_COMMAND_NAME}
#define MSR_READER_NAME   "msr"  // create /proc/{MODULE_NAME}/{MSR_READER_NAME}
#define LKM_NAME  "srmtMwaitLKM"

#define PROC_MODULE_NAME_MSR_FILE "/proc/" MODULE_NAME "/" MSR_READER_NAME
#define PROC_MODULE_NAME_CSTATE_FILE "/proc/" MODULE_NAME "/" CSTATE_COMMAND_NAME

#define MAX_COMMAND_LEN 32
#define MAX_COMMANDS 32
// TODO is this limitation necessary?  this limit could very plausibly be reached by a system
#define MAX_CORES 512  // must be a multiple of 32 for correct computation of core masks
#define MAX_CORES_PP 64  // per package cores, currently 28, so 32 was toо close to set it as a limit
#define TRIGGER_MEMORY_DEFAULT_SIZE 64
#define MSR_COLLECTION_SIZE 50   // max of how many MSRs we keep under watch

#define MAX_CS_STATE_NUM 9  // just to have something to compare against CS1, CS3, CS7. Let's  set CS9  as the last one
#define MAX_TEST_STEP_LEN 80  // in text form

#define STEP_END 0
#define STEP_P 'p'
#define STEP_C 'c'
#define STEP_DEBUG 'd'

#define MY_IOC_MAGIC 'I'
#define MY_IOCTL_FILE_NAME "/dev/" LKM_NAME

/**
 * Enumeration types which provide common interface between kernel driver and
 * user app layer ioctl functions.
 */
enum {
    SRMT_READ_MPERF_APERF = 0xe5,
};

struct msr_aperf_mperf {
    uint64_t aperf;
    uint64_t mperf;
};

#define SRMT_IOCTL_READ_MPERF_APERF _IOR(MY_IOC_MAGIC, SRMT_READ_MPERF_APERF, struct msr_aperf_mperf *)

// We can't use cpu_set_t in LKM, so we make two different data types: User and Kernel
struct stateInstructionKernel_t {
    char type; // one of the step types defined above
    uint32_t coreMaskArray[MAX_CORES / 32]; // CPU cores to be used in this test step (bitmask)
	uint8_t state;  // numerical value for the state: 3 for C3, 6 for C6, etc
    uint32_t time; // duration in microseconds
};

struct msrMsg_t {
	uint32_t coreId;
    uint32_t msrId;
    uint64_t msrValue;
    bool status;
};

#endif /* SRMT_USERKERNEL_DATA_HH_ */

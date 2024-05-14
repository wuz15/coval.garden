/*!
*
* pcstateMwaitKM.c
*
* Copyright (C) 2017 Intel Corporation. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version
* 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
* 02110-1301, USA.
*
* Author Name <cjparsons@soft-forge.com>


*****************************************************************************
\file pcstateMwaitLKM.c

\brief Linux Kernel Module to perform MWAIT/MONITOR commands

\par NOTES:

*****************************************************************************
\par REVISION HISTORY:

06/23/2014       borisv          Original Draft
*****************************************************************************/

#define _GNU_SOURCE

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>  // kmalloc()
#include <linux/kthread.h>
#include <linux/unistd.h> //usleep()
#include <linux/delay.h> //msleep()
#include <linux/sched.h>  // sched_setaffinity()
#include <linux/syscalls.h>  // sys_getpid()
#include <asm/mwait.h>
#include <linux/cpumask.h>
#include <linux/cdev.h>		/* for cdev for ioctl */
#include <linux/device.h>	/* for dev for ioctl */

// include shared data with application part
#include "pcstateUserKernelSharedData.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Intel Corporation, Inc. / Software Forge, Inc.");
MODULE_DESCRIPTION("Simple method to access MONITOR/MWAIT commands via procfs");
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,12,0)
MODULE_SUPPORTED_DEVICE("None");
#endif

#define MY_FIRST_MINOR 0
#define MY_MINOR_CNT 1

static struct proc_dir_entry *pcstate_dir, *cstate_command_file, *msr_command_file;
#define CORE_TASK_NAME "pcstateCoreTask"
#define MWAIT_TASK_NAME "pcstateMwaitTask"

#define TRIGGER_MEMORY_SIZE_INT (TRIGGER_MEMORY_DEFAULT_SIZE/(sizeof (int)))

struct mWaitData
{
  uint32_t coreId;
  char *watchedArea;
  uint32_t longevity;  // in microsec
  uint8_t state;
};

/* run-time flag to dynamically control a volume of debug printouts */
bool pcstateDebugStatus = false;  


/* If application decided not to use /dev/cpu/X/msr pseudo files to read MSRs,
   but instead to call LKM to get that info, then we use msrReq/msrResp.
   We also assume that we have one MSR request from user at any moment of time */
static struct msrMsg_t m;

// Bunch of MSRs we watch inside MWAIT loop
static struct msrMsg_t mArr[MSR_COLLECTION_SIZE][MAX_CORES];
bool coresInMwait[MAX_CORES];

/*****************************************************************/

int msrReadFuncNoMwait(void *data)
{
  struct msrMsg_t *mPtr = (struct msrMsg_t *)data;
  uint32_t hi,lo; 
  asm volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(mPtr->msrId));
  mPtr->msrValue = (uint64_t) hi << 32 | lo;
  mPtr->status = true;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,19,0)
  return(SIGCHLD);
#else
  do_exit(SIGCHLD);
#endif
}

void msrReadFuncInMwait(struct msrMsg_t *data)
{
  uint32_t hi,lo; 
  asm volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(data->msrId));
  data->msrValue = (uint64_t) hi << 32 | lo;
  data->status = true;
}

uint64_t msrReadFuncQuick(uint32_t msrId)
{
  uint32_t hi,lo; 
  asm volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(msrId));
  return (uint64_t) hi << 32 | lo;
}

void msrReadAPerfMPerf(uint64_t *aperf, uint64_t *mperf)
{
    *mperf = msrReadFuncQuick(MSR_IA32_MPERF);
    *aperf = msrReadFuncQuick(MSR_IA32_APERF);
}

uint64_t readTsc(void)
{
    uint32_t hi,lo;
    asm volatile("rdtsc":"=a"(lo),"=d"(hi));
    return (uint64_t) hi << 32 | lo;
}


/***********************************************************/
void cleanMsrArray (void)
{
  memset(mArr, 0, sizeof (mArr));
}


/* finds either the filled structure with required MSR, or the first empty slot */ 
struct msrMsg_t *findMsrPlace (uint32_t id, uint32_t core)
{
  int i=0;
  struct msrMsg_t *notOccupied = NULL;
  
  if (core <= MAX_CORES)
    {
      for (i=0; i<MSR_COLLECTION_SIZE; i++)
        {
          if (mArr[i][core].msrId==id)
            return &mArr[i][core];
          else if (notOccupied==NULL && mArr[i][core].msrId==0)
            {
              // take this slot
              notOccupied = &mArr[i][core];
            }
        }
      if (notOccupied)
        {
          memset(notOccupied, 0, sizeof (struct msrMsg_t));
          notOccupied->msrId = id;
          notOccupied->coreId = core;
          return notOccupied;
        }
    }
  
  return NULL;
}

/*-----  read all MSRs needed ------------*/
void fillMsrArray(uint32_t core)
{
  int i;
  for (i=0; i<MSR_COLLECTION_SIZE; i++)
    {
      if (core < MAX_CORES)
        msrReadFuncInMwait(&(mArr[i][core]));
    }
}



void collectCStateStats(uint64_t *storage)
{
  storage[0] = msrReadFuncQuick(MSR_IA32_MPERF);
  storage[3] = msrReadFuncQuick(MSR_CORE_C3_RESIDENCY);
  storage[6] = msrReadFuncQuick(MSR_CORE_C6_RESIDENCY);
  storage[7] = msrReadFuncQuick(MSR_CORE_C7_RESIDENCY);
  storage[10] = readTsc();
  storage[1] = storage[10] - (storage[0]+storage[3]+storage[6]+storage[7]);
}


void customSleep(uint32_t usec)
{
    // use different delay methods based on the length of the delay
    if (usec < 10)
        udelay(usec);
    else if (usec < 10000)
        usleep_range(usec-1, usec+1);
    else // 10ms+
        msleep(usec/1000);
}

/*! 
*******************************************************************************
* 
*  \brief   MWAIT and MONITOR instructions
* 
******************************************************************************/
static inline void monitor(const void *eax, unsigned long ecx,
			     unsigned long edx)
/* eax points to the watched area,
   ecx specifies optional extensions,
   edx specify optional hints */
{
	/* "monitor %eax, %ecx, %edx;" */
	asm volatile(".byte 0x0f, 0x01, 0xc8;"
		     :: "a" (eax), "c" (ecx), "d"(edx));
}


static inline void mwait(unsigned long eax, unsigned long ecx)
{
	/* "mwait %eax, %ecx;" */
    // eax may contain hint, ecx specifies optional extensions
	asm volatile(".byte 0x0f, 0x01, 0xc9;"
		     :: "a" (eax), "c" (ecx));
}



/*! 
*******************************************************************************
* 
*  \brief   task to perform MWAIT and MONITOR
* 
******************************************************************************/
int funcMwaitTask (void* data)
{
    struct mWaitData *myData = (struct mWaitData *) data;
    bool stillWaiting = true;
    bool firstReset = true;
    uint32_t countResets = 0;
    uint64_t cstateStats0[11]; // temp objects for debug
    uint64_t cstateStats1[11];

    if (pcstateDebugStatus)
      {
        printk(KERN_INFO "pcstate: funcMwaitTask core %d to c%1.1d\n", myData->coreId, myData->state);
        collectCStateStats(cstateStats0);
      }
    while (stillWaiting)
      {
            int i;
            uint32_t hint = 0;
            // go down to C-state
            
            /*  those codes didn't work on C3, C6:
                hint = 0xf0 & (((myData.state&0xf) - 1)<<4);
                so we use the same as Windows version */
            switch (myData->state & 0xf)
              {
                case 0:
                  hint = 0xf0;
                  break;
                case 1:
                  hint = 0x00;
                  break;
                case 3:
                  hint = 0x10;
                  break;
                case 6:
                  hint = 0x20;
                  break;
                case 7:
                  hint = 0x30;
                  break;
                default:
                  printk(KERN_WARNING "pcstate: invalid C-State %#x\n",  myData->state);
                  goto getOut;
              }
            if (firstReset)
                {
                  if (pcstateDebugStatus) {
                    printk(KERN_INFO "pcstate: call mwait(%#x,%d) on core %d\n",
                                                           hint, 0, myData->coreId);
		  }
                    monitor(myData->watchedArea, 0, 0);
                }
            
            mwait(hint, 0);
            
            // came out from mwait, let's check why
            if (firstReset && pcstateDebugStatus)
                printk(KERN_INFO "pcstate: mwait state ended\n");
            for (i=0; i<TRIGGER_MEMORY_SIZE_INT; i++)
                {
                    int *ptr = (int*)(myData->watchedArea);
                    if (ptr[i] != 0)
                        {
                            stillWaiting = false;
                            coresInMwait[myData->coreId]=false;
                            break;
                        }
                }
            
            if (stillWaiting) // no trigger happened, reset the monitor
                {
                    countResets++;
                    fillMsrArray(myData->coreId);
                    monitor(myData->watchedArea, 0, 0);
                    if (firstReset)
                        {
                            if (pcstateDebugStatus)
                              printk(KERN_INFO "pcstate: reset the monitor in %d\n", myData->coreId);
                            firstReset = false;
                        }
                }
        }
    
    if (pcstateDebugStatus)
        {
            printk(KERN_INFO "pcstate: core %d out of mwait after %d resets\n", myData->coreId, countResets);
            collectCStateStats(cstateStats1);
            printk(KERN_INFO "pcstate: C0 delta %12lld\n", cstateStats1[0]-cstateStats0[0]);
            printk(KERN_INFO "pcstate: C1 delta %12lld\n", cstateStats1[1]-cstateStats0[1]);
            printk(KERN_INFO "pcstate: C3 delta %12lld\n", cstateStats1[3]-cstateStats0[3]);
            printk(KERN_INFO "pcstate: C6 delta %12lld\n", cstateStats1[6]-cstateStats0[6]);
            printk(KERN_INFO "pcstate: C7 delta %12lld\n", cstateStats1[7]-cstateStats0[7]);
        }
 getOut:
    if (!(IS_ERR(myData)))
      {
        if (!(IS_ERR(myData->watchedArea)))
          kfree(myData->watchedArea);
        kfree(myData);
      }
    else if (pcstateDebugStatus)
      printk(KERN_ERR "pcstate: funcMwaitTask got bad data\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,19,0)
  return(SIGCHLD);
#else
  do_exit(SIGCHLD);
#endif
}

/*! 
*******************************************************************************
* 
*  \brief   Per core task function
* 
*   \return  int error status 
* 
*   \warning  Only MWAIT executing task must be 1) separate thread;
*                                               2) use affinity
*             This task has no such requirements
* 
******************************************************************************/
int cTaskFunc (void* data)
{
  struct mWaitData *mwdata = (struct mWaitData *) data;
  struct task_struct * mTask;
  char name[64];
  uint32_t core;
  char *watchedArea = NULL;
 
  core = mwdata->coreId;
   
  // create watched area we will monitor
  watchedArea = kmalloc(TRIGGER_MEMORY_DEFAULT_SIZE, GFP_KERNEL);
  if (watchedArea==NULL || mwdata==NULL)
      return (-ENOMEM);
  else
      mwdata->watchedArea = watchedArea;
  memset(watchedArea, 0, TRIGGER_MEMORY_DEFAULT_SIZE);

  // create MWAIT task, kick it and wait it's finished
  sprintf(name, "%s%d", MWAIT_TASK_NAME, core);

  mTask = kthread_create(&funcMwaitTask, mwdata, name);
  // we have to use _create instead of _run because we will immediately do _bind here
  if (IS_ERR(mTask))
    {
      printk(KERN_ERR "pcstate: can't create trigger task %d\n", core);
      return (-ENOMEM);
    }
  else
    {
      kthread_bind(mTask, core);
      if (pcstateDebugStatus)
        printk(KERN_INFO "pcstate: task %s=%p\n", name, (void*)mTask);
      wake_up_process (mTask);
    }

  customSleep(mwdata->longevity);
  watchedArea[0] = 1; /* trip the trigger
                         tripping the trigger again "to make sure" would cause race condition:
                         task activated, frees memory, and then we try to write to it with that second "tripping" */

  if (pcstateDebugStatus)
      printk(KERN_INFO "pcstate: out of trigger on core %d\n", core);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,19,0)
  return(SIGCHLD);
#else
  do_exit(SIGCHLD);
#endif
}


/*! 
*******************************************************************************
* 
*  \brief   Based on core mask, create separate task for each core. Make per core trigger area
* 
*   \param   struct stateInstructionKernel_t * ptr 
* 
*   \return  int error status (0=OK)
*
*   \warning Assumption: 64-bit core mask (up to 64 cores altogether)
*
*******************************************************************************/
int startCState (struct stateInstructionKernel_t * si)
{
    struct task_struct * cTask;
    uint32_t coreDoes;
    uint32_t i;
    char cName[64];
    
    uint32_t maskPart;
    ///int cpu = smp_processor_id();

    // figure out what cores do we have to use
    for (i=0; i<MAX_CORES; i++)
        {
            maskPart = si->coreMaskArray[i/32]; // "active" word
            coreDoes = maskPart & (1UL << i%32 );
            if (coreDoes)
                {
                    /* if (core==cpu) */
                    /*     printk(KERN_INFO "pcstate: startCState excludes core %d\n", i); */
                    /* else */
                        {
                            // these are per core actions
                            struct mWaitData *mwdata = kmalloc(sizeof (struct mWaitData), GFP_KERNEL);
                            if (mwdata==NULL)
                                return (-ENOMEM);
                            sprintf(cName, "%s%d", CORE_TASK_NAME, i);
                            mwdata->coreId = i;
                            mwdata->state = si->state;
                            mwdata->longevity = si->time;
                            
                            coresInMwait[i] = true;
                            if (pcstateDebugStatus)
                                printk(KERN_INFO "pcstate: run %s cpu=%d state=%d\n", cName, i, mwdata->state );
                            cTask = kthread_run(&cTaskFunc, mwdata, cName);
                            if (IS_ERR(cTask))
                                {
                                    printk(KERN_ERR "pcstate: can't create cTask on core %d\n", i);
                                    return (-ENOMEM);
                                }
                            else if (pcstateDebugStatus)
                                {
                                  printk(KERN_INFO "pcstate: task %s=%p\n", cName, (void*)cTask);
                                }

                              
                        }
                }
            else
              coresInMwait[i] = false;
        }

    return 0;
}

/*! 
*******************************************************************************
* 
*  \brief   Data transfer from user to kernel, carrying MSR params
* 
*   \return  int,  if positive: amount of data transfered
* 
*   \warning  
* 
*******************************************************************************/
ssize_t msrRequest (struct file *filp,const char __user *buf, size_t count, loff_t *offp)
{
    int ret = 0;
    char msg[2*sizeof (struct msrMsg_t)]; // some extra space just in case

    if (count == sizeof (struct msrMsg_t))
      {
          if (copy_from_user(msg, buf, count))
              return -EFAULT;
          else
              {
                struct task_struct * msrReadTask;
                ret = count;
                memcpy(&m, msg, count);
                if (pcstateDebugStatus)
                  printk(KERN_INFO "msrReq: need core %d msr %08x ", m.coreId, m.msrId);
                if (coresInMwait[m.coreId]==false) // we should read independently from MWAIT
                  {
                    if (pcstateDebugStatus)
                      printk(KERN_INFO " - read ");
                    msrReadTask = kthread_create(&msrReadFuncNoMwait, &m, "msrReadTask");
                  
 
                    if (IS_ERR(msrReadTask))
                      {
                        printk(KERN_ERR "pcstate: can't create msrRead task %d\n", m.coreId);
                        return (-ENOMEM);
                      }
                    else
                      {
                        kthread_bind(msrReadTask, m.coreId);
                        printk(KERN_INFO "pcstate: msrRequest task %p", (void *) msrReadTask );
                      }
                      
                    m.status = false;
                    wake_up_process (msrReadTask);
                    while (m.status==false)
                      customSleep(1);
                 
                if (pcstateDebugStatus)
                    printk(KERN_INFO " val = 0x%llx\n",(long long unsigned int)m.msrValue);
                   
                  }
                else  // here just use the data collected by MWAIT
                  {
                    struct msrMsg_t *ptr;
                    ptr = findMsrPlace(m.msrId, m.coreId);
                    /* if (pcstateDebugStatus) */
                    /*   printk(KERN_INFO " use mArr %08x val = %08x\n", */
                    /*          ptr, (ptr? ptr->msrValue:0)); */
                    if (m.coreId<MAX_CORES && ptr)
                      m = *ptr;
                    else
                      memset(&m, 0, sizeof (struct msrMsg_t));
                    m.status = true;
                  }
                
              }
      }
    else
      return -ENOSPC;
    
    return ret;
}

/*! 
*******************************************************************************
* 
*  \brief   Data transfer from kernel to user, carrying MSR value
* 
*   \return  int,  if positive: amount of data transfered
* 
*   \warning  
* 
*******************************************************************************/
ssize_t msrResponse (struct file *filp, char __user *buf, size_t count, loff_t *offp ) 
{
  int res;

  res = copy_to_user(buf, (void*)&m, sizeof(struct msrMsg_t));
  if (res == 0)
    {
      *offp = 0;
      if (pcstateDebugStatus)
        printk(KERN_INFO "msrResp: msr %08x on core %d = 0x%llx\n",
               m.msrId, m.coreId, (long long unsigned int)m.msrValue);
      return sizeof(struct msrMsg_t);
    }
  else
    {
      if (pcstateDebugStatus)
        printk(KERN_INFO "msrResp: copy_to_user = %d\n", res);
      return 0;
    }
}

int bit_count (uint32_t mask)
{
	int sum, i;

	sum = 0;
	for (i = 0; i < 32; i++) {
		if (mask & (1 << i))
			sum++;
	}
	return sum;
}

/*! 
*******************************************************************************
* 
*  \brief   Data transfer from user to kernel, carrying cstate instruction
* 
*   \param    
*   \param    
* 
*   \return  int,  if positive: amount of data transfered
* 
*   \warning  
* 
*******************************************************************************/
ssize_t writeCstateCommand (struct file *filp,const char __user *buf, size_t count, loff_t *offp)
{
    int ret = 0;
    int i, j, len, sum;
    uint32_t mask;
    struct stateInstructionKernel_t si;

    if (count == sizeof (struct stateInstructionKernel_t)) {

        if (copy_from_user((char *) &si, buf, count)) {
	    printk (KERN_INFO "Error: copy_from_user() failed\n");
            return -EFAULT;
        }
        else {
            ret = len = count;

            // security check starts
            if (si.type != STEP_C) {
                printk (KERN_INFO "Error: type is not STEP_C\n");
                return -EFAULT;
            }
            if (si.state > 12) {
                printk (KERN_INFO "Error: state is bigger than 12\n");
                return -EFAULT;
            }

            // check for all of range cores
            sum = num_online_cpus();
            if (sum < MAX_CORES) {
                i = sum / 32;
                j = sum % 32;
                mask = (uint32_t) (1 << j) - 1;
                if (si.coreMaskArray [i] & ~mask) {
                    printk (KERN_INFO "Error: CPU cores specified out of range\n");
                    return -EFAULT;
                }

		i++;
		for (; i < MAX_CORES / 32; i++) {
                    if (si.coreMaskArray [i] != 0) {
                        printk (KERN_INFO "Error: CPU cores specified out of range\n");
                        return -EFAULT;
                    }
		}
            }
            else {
                printk (KERN_INFO "Error: number of CPU cores is more than MAX_CORES=%d\n", MAX_CORES);
                return -EFAULT;
            }
			
            if (si.time > 15000000) {
                printk (KERN_INFO "Error: time is bigger than 15000000\n");
                return -EFAULT;
            }

            // security check ends
            else if (si.type == STEP_C) {
                // printk(KERN_INFO "pcstate: keep state %c%d on cores 0x%x for %u microsec\n",
                //        si.type, si.state, si.coreMaskArray[0], si.time);
                startCState(&si);
            }
        }
    }
    else {
        printk (KERN_INFO "Error: size of c-state command is not correct.\n");
        return -ENOSPC;
    }
    
    return ret;
}

/*! 
*******************************************************************************
* 
*  \brief   Data transfer from kernel to user, carrying cstate instruction
* 
*   \param    
*   \param    
* 
*   \return  int,  if positive: amount of data transfered
* 
*   \warning  
* 
*******************************************************************************/
ssize_t readCstateCommand (struct file *filp, char __user *buf, size_t count, loff_t *offp ) 
{
    int temp=0, len=0;
    char msg[512]; 
    
    if (pcstateDebugStatus)
      printk(KERN_INFO "pcstate: into readCstateCommand\n");
    if(count>temp)
        {
            count=temp;
        }
    temp=temp-count;
    if (copy_to_user(buf, msg, count))
        {
            if(count==0)
                temp=len;
            return count;
        }
    else
        return 0;
}

/*
**********************************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)

struct proc_ops proc_fops = {
 .proc_read = readCstateCommand,
 .proc_write = writeCstateCommand,
};

struct proc_ops msr_fops = {
 .proc_read = msrResponse,
 .proc_write = msrRequest,
};

#else

struct file_operations proc_fops = {
 .owner = THIS_MODULE,
 .read = readCstateCommand,
 .write = writeCstateCommand,
};

struct file_operations msr_fops = {
 .owner = THIS_MODULE,
 .read = msrResponse,
 .write = msrRequest,
};

#endif

static void cleanProcFiles(void)
{
    printk(KERN_INFO "removing %s subtree \n", MODULE_NAME);
    //remove_proc_subtree(MODULE_NAME, NULL);
    // instead of one call, do it in steps since some kernels don't have that function
    if (msr_command_file)
        remove_proc_entry(MSR_READER_NAME, pcstate_dir);
    if (cstate_command_file)
        remove_proc_entry(CSTATE_COMMAND_NAME, pcstate_dir);

    remove_proc_entry(MODULE_NAME, NULL);
}

/*! 
*******************************************************************************
* 
*  \brief   pcstateMwaitmonModuleInit 
* 
*   \param    
*   \param    
* 
*   \return  int, error code
* 
*   \warning  
* 
*******************************************************************************/
static int __init pcstateMwaitmonModuleInit(void)
{
    int ret;

    printk(KERN_INFO "Loading %s module\n", MODULE_NAME);
     
    /* create a directory */
    pcstate_dir = proc_mkdir(MODULE_NAME, NULL);
    if(pcstate_dir == NULL)
    {
      printk(KERN_ERR "failed to create %s proc entry\n", MODULE_NAME);
      return -ENOMEM;
    }
    
    /* create a file */

// don't use deprecated function
//  cstate_command_file = create_proc_entry(CSTATE_COMMAND_NAME, 0644, pcstate_dir);
// instead use
// proc_create(file_name, permission_bits, parent_dir, file_operations)
    
    cstate_command_file = proc_create(CSTATE_COMMAND_NAME, 0666, pcstate_dir, &proc_fops);
    if(cstate_command_file == NULL)
      printk(KERN_ERR "failed to create %s proc entry\n", CSTATE_COMMAND_NAME);

    msr_command_file = proc_create(MSR_READER_NAME, 0666, pcstate_dir, &msr_fops);
    if(msr_command_file == NULL)
      printk(KERN_ERR "failed to create %s proc entry\n", MSR_READER_NAME);

    if (cstate_command_file == NULL || msr_command_file == NULL)
      {
        remove_proc_entry(MODULE_NAME, NULL);
        return -ENOMEM;
      }

    cleanMsrArray();

    // Set up the dev file for ioctl
    ret = 0;	/* everything ok */
    return ret;
}

/*! 
*******************************************************************************
* 
*  \brief   pcstateMwaitmonModuleExit 
* 
*   \param    
*   \param    
* 
*   \return  int, error code
* 
*   \warning  
* 
*******************************************************************************/
static void __exit pcstateMwaitmonModuleExit(void)
{
    printk(KERN_INFO "Exit %s module \n", MODULE_NAME);

    cleanProcFiles();
}

/* Module specifiers */

module_init(pcstateMwaitmonModuleInit);
module_exit(pcstateMwaitmonModuleExit);

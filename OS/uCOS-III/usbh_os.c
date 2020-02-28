/*
*********************************************************************************************************
*                                             uC/USB-Host
*                                     The Embedded USB Host Stack
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                   USB HOST OPERATING SYSTEM LAYER
*                                          Micrium uC/OS-III
*
* Filename : usbh_ucos.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_UCOS_MODULE
#define   MICRIUM_SOURCE
#include  <Source/os.h>
#include  <os_cfg.h>
#include  <usbh_cfg.h>
#include  "../../Source/usbh_os.h"


/*
*****************************************   *****************************************   ***********************
*                                        CONFIGURATION ERRORS
*****************************************   *****************************************   ***********************
*/

#if (OS_CFG_MUTEX_EN != 1u)
#error  "OS_CFG_MUTEX_EN                    illegally #define'd in 'os_cfg.h'"
#error  "                                   [MUST be = 1]              "
#endif

#if (OS_CFG_MUTEX_DEL_EN != 1u)
#error  "OS_CFG_MUTEX_DEL_EN                illegally #define'd in 'os_cfg.h'"
#error  "                                   [MUST be = 1]              "
#endif

#if (OS_CFG_SEM_EN != 1u)
#error  "OS_CFG_SEM_EN                      illegally #define'd in 'os_cfg.h'"
#error  "                                   [MUST be = 1]              "
#endif

#if (OS_CFG_SEM_DEL_EN != 1u)
#error  "OS_CFG_SEM_DEL_EN                  illegally #define'd in 'os_cfg.h'"
#error  "                                   [MUST be = 1]              "
#endif

#if (OS_CFG_SEM_PEND_ABORT_EN != 1u)
#error  "OS_CFG_SEM_PEND_ABORT_EN           illegally #define'd in 'os_cfg.h'"
#error  "                                   [MUST be = 1]              "
#endif

#if (OS_CFG_Q_EN != 1u)
#error  "OS_CFG_Q_EN                        illegally #define'd in 'os_cfg.h'"
#error  "                                   [MUST be = 1]              "
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_OS_MUTEX_POOL_SIZE            ((((USBH_CFG_MAX_NBR_EPS * USBH_CFG_MAX_NBR_IFS) + 1u) * USBH_CFG_MAX_NBR_DEVS) + \
                                             USBH_CFG_MAX_NBR_DEVS + USBH_CFG_MAX_NBR_HC +                                    \
                                             USBH_CDC_CFG_MAX_DEV + USBH_HID_CFG_MAX_DEV + USBH_MSC_CFG_MAX_DEV +             \
                                            (USBH_FTDI_CFG_MAX_DEV * 2u) + 1u)
#define  USBH_OS_SEM_POOL_SIZE              (3u + (((USBH_CFG_MAX_NBR_EPS * USBH_CFG_MAX_NBR_IFS) + 1u) * USBH_CFG_MAX_NBR_DEVS))
#define  USBH_OS_TCB_POOL_SIZE                7u
#define  USBH_OS_Q_POOL_SIZE                (3u + (((USBH_CFG_MAX_NBR_EPS * USBH_CFG_MAX_NBR_IFS) + 1u) * USBH_CFG_MAX_NBR_DEVS))
#define  USBH_OS_TMR_POOL_SIZE              (3u + (((USBH_CFG_MAX_NBR_EPS * USBH_CFG_MAX_NBR_IFS) + 1u) * USBH_CFG_MAX_NBR_DEVS))

#define  USBH_OS_TASK_STK_LIMIT_PCT_FULL     90u


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

                                                                /* Milliseconds converted to ticks (rounded up).                */
#define USBH_OS_TICK_TIMEOUT_mS(ms_timeout)  (((ms_timeout) > 0u) ? (OS_TICK)((((ms_timeout) * OSCfg_TickRate_Hz) + DEF_TIME_NBR_mS_PER_SEC - 1u) / DEF_TIME_NBR_mS_PER_SEC) : 0u)

                                                                /* Microseconds converted to ticks (rounded up).                */
#define USBH_OS_TICK_TIMEOUT_uS(us_timeout)  (((us_timeout) > 0u) ? (OS_TICK)((((us_timeout) * OSCfg_TickRate_Hz) + DEF_TIME_NBR_uS_PER_SEC - 1u) / DEF_TIME_NBR_uS_PER_SEC) : 0u)


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/
#if (OS_CFG_TMR_EN == DEF_ENABLED)
typedef  struct  usbh_tmr_handle {
    OS_TMR    Tmr;                                              /* OS-III tmr obj.                                      */

    void    (*CallbackFnct)(void  *p_arg);                      /* Tmr registered callback fnct.                        */
    void     *CallbackArg;                                      /* Arg to pass to callback fnct.                        */
} USBH_TMR_HANDLE;
#endif


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

MEM_POOL  USBH_OS_MutexPool;
MEM_POOL  USBH_OS_QPool;
MEM_POOL  USBH_OS_SemPool;
MEM_POOL  USBH_OS_TCB_Pool;
#if (OS_CFG_TMR_EN == DEF_ENABLED)
MEM_POOL  USBH_OS_TmrPool;
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         USBH_OS_LayerInit()
*
* Description : Initialize OS layer.
*
* Argument(s) : none.
*
* Return(s)   : USBH_ERR_NONE, if successful.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_LayerInit (void)
{
    LIB_ERR     err_lib;
    CPU_SIZE_T  octets_reqd;


    Mem_PoolCreate(      &USBH_OS_MutexPool,                    /* Init mutex mem pool.                                 */
                  (void *)0,
                          USBH_OS_MUTEX_POOL_SIZE * sizeof(OS_MUTEX),
                          USBH_OS_MUTEX_POOL_SIZE,
                          sizeof(OS_MUTEX),
                          sizeof(CPU_ALIGN),
                         &octets_reqd,
                         &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    Mem_PoolCreate(       &USBH_OS_QPool,                       /* Init Q mem pool.                                    */
                   (void *)0,
                           USBH_OS_Q_POOL_SIZE * sizeof(OS_Q),
                           USBH_OS_Q_POOL_SIZE,
                           sizeof(OS_Q),
                           sizeof(CPU_ALIGN),
                          &octets_reqd,
                          &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    Mem_PoolCreate(       &USBH_OS_SemPool,                     /* Init sem mem pool.                                   */
                   (void *)0,
                           USBH_OS_SEM_POOL_SIZE * sizeof(OS_SEM),
                           USBH_OS_SEM_POOL_SIZE,
                           sizeof(OS_SEM),
                           sizeof(CPU_ALIGN),
                          &octets_reqd,
                          &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    Mem_PoolCreate(       &USBH_OS_TCB_Pool,                    /* Init TCB mem pool.                                   */
                   (void *)0,
                           USBH_OS_TCB_POOL_SIZE * sizeof(OS_TCB),
                           USBH_OS_TCB_POOL_SIZE,
                           sizeof(OS_TCB),
                           sizeof(CPU_ALIGN),
                          &octets_reqd,
                          &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

#if (OS_CFG_TMR_EN == DEF_ENABLED)
    Mem_PoolCreate(       &USBH_OS_TmrPool,                     /* Init Timer mem pool.                                 */
                   (void *)0,
                           USBH_OS_TMR_POOL_SIZE * sizeof(OS_TMR),
                           USBH_OS_TMR_POOL_SIZE,
                           sizeof(OS_TMR),
                           sizeof(CPU_ALIGN),
                          &octets_reqd,
                          &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }
#endif

    (void)octets_reqd;

    return (USBH_ERR_NONE);
}


/*
**************************************************************************************************************
*                                       USBH_OS_VirToBus()
*
* Description : Convert from virtual address to physical address if the operating system uses
*               virtual memory
*
* Arguments   : x   Virtual address to convert
*
* Returns     :     The corresponding physical address
*
* Note(s)     : (1) uC/OS-III doesn't use a virtual memory.
**************************************************************************************************************
*/

void  *USBH_OS_VirToBus (void  *x)
{
    return (x);                                                 /* See Note #1                                                  */
}


/*
**************************************************************************************************************
*                                       USBH_OS_BusToVir()
*
* Description : Convert from physical address to virtual address if the operating system uses
*               virtual memory
*
* Arguments   : x   Physical address to convert
*
* Returns     :     The corresponding virtual address
*
* Note(s)     : (1) uC/OS-III doesn't use a virtual memory.
**************************************************************************************************************
*/

void  *USBH_OS_BusToVir (void  *x)
{
    return (x);                                                 /* See Note #1                                                  */
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           DELAY FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           USBH_OS_DlyMS()
*
* Description : Delay the current task by specified delay.
*
* Argument(s) : dly       Delay, in milliseconds.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBH_OS_DlyMS (CPU_INT32U  dly)
{
    OS_ERR  err;


    OSTimeDlyHMSM(0u, 0u, 0u, dly,
                  OS_OPT_TIME_HMSM_STRICT,
                 &err);

    (void)err;
}


/*
*********************************************************************************************************
*                                           USBH_OS_DlyUS()
*
* Description : Delay the current task by specified delay.
*
* Argument(s) : dly       Delay, in microseconds.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  USBH_OS_DlyUS (CPU_INT32U  dly)
{
    OS_TICK  ticks;
    OS_ERR   err;


    ticks = USBH_OS_TICK_TIMEOUT_uS(dly);

    if (ticks != 0u) {
        OSTimeDly(ticks,
                  OS_OPT_TIME_DLY,
                 &err);
    } else {
        OSTimeDly(1u,                                           /* If current OS config does NOT allow this dly ...     */
                  OS_OPT_TIME_DLY,                              /* ... perform minimal OS delay (1 tick).               */
                 &err);
    }

    (void)err;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           MUTEX FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        USBH_OS_MutexCreate()
*
* Description : Create a mutex.
*
* Argument(s) : p_mutex    Pointer to variable that will receive handle of the mutex.
*
* Return(s)   : USBH_ERR_NONE,              if successful.
*               USBH_ERR_ALLOC,
*               USBH_ERR_OS_SIGNAL_CREATE,
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexCreate (USBH_HMUTEX  *p_mutex)
{
    OS_MUTEX  *p_os_mutex;
    OS_ERR     err;
    LIB_ERR    err_lib;


    p_os_mutex = (OS_MUTEX *)Mem_PoolBlkGet(&USBH_OS_MutexPool,
                                             sizeof(OS_MUTEX),
                                            &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    OSMutexCreate(p_os_mutex, "USB Mutex", &err);
    if (err != OS_ERR_NONE) {
        return (USBH_ERR_OS_SIGNAL_CREATE);
    }

   *p_mutex = (USBH_HMUTEX)p_os_mutex;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                         USBH_OS_MutexLock()
*
* Description : Acquire a mutex.
*
* Argument(s) : mutex     Mutex handle which passes mutex object (OS_MUTEX) to the
*                         uCOS-III function
*
* Return(s)   : USBH_ERR_NONE,          if successful.
*               USBH_ERR_OS_TIMEOUT,    if timeout.
*               USBH_ERR_OS_FAIL,       otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexLock (USBH_HMUTEX  mutex)
{
    OS_ERR  err_os;


    OSMutexPend((OS_MUTEX *)mutex,
                            0u,
                            OS_OPT_PEND_BLOCKING,
                (CPU_TS   *)0,
                           &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
             return (USBH_ERR_NONE);
             break;

        case OS_ERR_TIMEOUT:
             return (USBH_ERR_OS_TIMEOUT);
             break;

        case OS_ERR_OBJ_DEL:
        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_OBJ_TYPE:
        case OS_ERR_OPT_INVALID:
        case OS_ERR_PEND_ISR:
        case OS_ERR_PEND_WOULD_BLOCK:
        case OS_ERR_SCHED_LOCKED:
        case OS_ERR_STATE_INVALID:
        case OS_ERR_STATUS_INVALID:
        case OS_ERR_MUTEX_OWNER:
        default:
             return (USBH_ERR_OS_FAIL);
             break;
    }
}


/*
*********************************************************************************************************
*                                        USBH_OS_MutexUnlock()
*
* Description : Releases a mutex.
*
* Argument(s) : mutex     Mutex handle which passes mutex object (OS_MUTEX) to the
*                         uCOS-III function
*
* Return(s)   : USBH_ERR_NONE,      if successful.
*               USBH_ERR_OS_FAIL,   otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexUnlock (USBH_HMUTEX  mutex)
{
    OS_ERR  err_os;


    OSMutexPost((OS_MUTEX *)mutex, OS_OPT_POST_NONE, &err_os);
    if (err_os != OS_ERR_NONE) {
        return (USBH_ERR_OS_FAIL);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MutexDestroy()
*
* Description : Destroys a mutex only when no task is pending on the mutex.
*
* Argument(s) : mutex     Mutex handle which passes mutex object (OS_MUTEX) to the
*                         uCOS-III function
*
* Return(s)   : USBH_ERR_NONE,      if successful.
*               USBH_ERR_OS_FAIL,   if mutex delete failed.
*               USBH_ERR_FREE,      if mutex free failed.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexDestroy (USBH_HMUTEX  mutex)
{
    OS_ERR   err_os;
    LIB_ERR  err_lib;


    OSMutexDel((OS_MUTEX *)mutex, OS_OPT_DEL_NO_PEND, &err_os);
    if (err_os != OS_ERR_NONE) {
        return (USBH_ERR_OS_FAIL);
    }

    Mem_PoolBlkFree(       &USBH_OS_MutexPool,
                    (void *)mutex,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_FREE);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         SEMAPHORE FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBH_OS_SemCreate()
*
* Description : Create semaphore with given count.
*
* Argument(s) : p_sem      Pointer that will receive handle for managing semaphore.

*               cnt        Value with which the semaphore will be initialized.
*
* Return(s)   : USBH_ERR_NONE,              if successful.
*               USBH_ERR_OS_SIGNAL_CREATE,
*               USBH_ERR_ALLOC,
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemCreate (USBH_HSEM   *p_sem,
                             CPU_INT32U   cnt)
{
    OS_SEM   *p_os_sem;
    OS_ERR    err;
    LIB_ERR   err_lib;


    p_os_sem = (OS_SEM *)Mem_PoolBlkGet(&USBH_OS_SemPool,
                                         sizeof(OS_SEM),
                                        &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    OSSemCreate((OS_SEM *)p_os_sem, "USB Sem", cnt, &err);
    if (err != OS_ERR_NONE) {
        return (USBH_ERR_OS_SIGNAL_CREATE);
    }

   *p_sem = (USBH_HSEM)p_os_sem;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                         USBH_OS_SemDestroy()
*
* Description : Destroy a semphore if no tasks are waiting on it.
*
* Argument(s) : sem       Semaphore handle which passes semaphore object (OS_SEM) to the
*                         uCOS-III function
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemDestroy (USBH_HSEM  sem)
{
    OS_ERR   err_os;
    LIB_ERR  err_lib;


    OSSemDel((OS_SEM *)sem, OS_OPT_DEL_NO_PEND, &err_os);
    if (err_os != OS_ERR_NONE) {
        return (USBH_ERR_OS_FAIL);
    }

    Mem_PoolBlkFree(&USBH_OS_SemPool, (void *)sem, &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_FREE);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_OS_SemWait()
*
* Description : Wait on a semaphore to become available.
*
* Argument(s) : sem       Semaphore handle which passes semaphore object (OS_SEM) to the
*                         uCOS-III function
*
*               timeout   Timeout.
*
* Return(s)   : USBH_ERR_NONE,          if successful.
*               USBH_ERR_OS_TIMEOUT,    if timeout error.
*               USBH_ERR_OS_ABORT,
*               USBH_ERR_OS_FAIL,
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemWait (USBH_HSEM   sem,
                           CPU_INT32U  timeout)
{
    OS_ERR  err_os;


    OSSemPend((OS_SEM *)sem,
                        USBH_OS_TICK_TIMEOUT_mS(timeout),
                        OS_OPT_PEND_BLOCKING,
              (CPU_TS *)0,
                       &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
             return (USBH_ERR_NONE);
             break;

        case OS_ERR_TIMEOUT:
             return (USBH_ERR_OS_TIMEOUT);
             break;

        case OS_ERR_PEND_ABORT:
             return (USBH_ERR_OS_ABORT);
             break;

        case OS_ERR_OBJ_DEL:
        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_OBJ_TYPE:
        case OS_ERR_OPT_INVALID:
        case OS_ERR_PEND_ISR:
        case OS_ERR_PEND_WOULD_BLOCK:
        case OS_ERR_SCHED_LOCKED:
        case OS_ERR_STATUS_INVALID:
        default:
             return (USBH_ERR_OS_FAIL);
             break;
    }
}


/*
*********************************************************************************************************
*                                       USBH_OS_SemWaitAbort()
*
* Description : Resume all tasks waiting on specified semaphore.
*
* Argument(s) : sem       Semaphore handle which passes semaphore object (OS_SEM) to the
*                         uCOS-III function
*
* Return(s)   : USBH_ERR_NONE,          if successful.
*               USBH_ERR_OS_TIMEOUT,    if timeout error.
*               USBH_ERR_OS_ABORT,
*               USBH_ERR_OS_FAIL,
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemWaitAbort (USBH_HSEM  sem)
{
    OS_ERR  err_os;


    (void)OSSemPendAbort((OS_SEM *)sem,
                                   OS_OPT_PEND_ABORT_ALL,
                                  &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
             return (USBH_ERR_NONE);
             break;

        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_OBJ_TYPE:
        case OS_ERR_OPT_INVALID:
        case OS_ERR_PEND_ABORT_ISR:
        case OS_ERR_PEND_ABORT_NONE:
        default:
             return (USBH_ERR_OS_FAIL);
             break;
    }
}


/*
*********************************************************************************************************
*                                          USBH_OS_SemPost()
*
* Description : Post a semaphore.
*
* Argument(s) : sem       Semaphore handle which passes semaphore object (OS_SEM) to the
*                         uCOS-III function
*
* Return(s)   : USBH_ERR_NONE,      if successful.
*               USBH_ERR_OS_FAIL,   otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemPost (USBH_HSEM  sem)
{
    OS_ERR   err_os;


    OSSemPost((OS_SEM *)sem, OS_OPT_POST_1, &err_os);
    if(err_os != OS_ERR_NONE) {
        return (USBH_ERR_OS_FAIL);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          THREAD FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        USBH_OS_ThreadCreate()
*
* Description : Create a thread (or task).
*
* Argument(s) : p_name          Pointer to name to assign to thread.
*
*               prio            Priority of the thread to be created.
*
*               thread_fnct     Pointer to the function that will be executed in this thread.
*
*               p_data          Pointer to the data that is passed to the thread function.
*
*               p_stk           Pointer to the beginning of the stack used by the thread.
*
*               stk_size        Size of the stack.
*
*               p_thread        Pointer that will receive handle for managing thread.
*
* Return(s)   : USBH_ERR_NONE,               if successful.
*               USBH_ERR_ALLOC,
*               USBH_ERR_OS_TASK_CREATE,
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_TaskCreate (CPU_CHAR        *p_name,
                              CPU_INT32U       prio,
                              USBH_TASK_FNCT   task_fnct,
                              void            *p_data,
                              CPU_INT32U      *p_stk,
                              CPU_INT32U       stk_size,
                              USBH_HTASK      *p_task)
{
    OS_TCB   *p_tcb;
    OS_ERR    err_os;
    LIB_ERR   err_lib;


    p_tcb = (OS_TCB *)Mem_PoolBlkGet(&USBH_OS_TCB_Pool,
                                      sizeof(OS_TCB),
                                     &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    OSTaskCreate(        p_tcb,
                         p_name,
                         task_fnct,
                         p_data,
                         prio,
                        &p_stk[0],
                        (stk_size * (100u - USBH_OS_TASK_STK_LIMIT_PCT_FULL)) / 100u,
                         stk_size,
                         0u,
                         0u,
                 (void *)0,
                        (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                        &err_os);
    if (err_os != OS_ERR_NONE) {
        return (USBH_ERR_OS_TASK_CREATE);
    }

   *p_task = (USBH_HTASK)prio;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            MESSAGE QUEUE
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       USBH_OS_MsgQueueCreate()
*
* Description : Create a message queue.
*
* Argument(s) : p_start         Pointer to the base address of the message queue storage area.
*
*               size            Number of elements in the storage area.
*
*               p_err           Pointer to variable that will receive the return error code from this function:
*
*                                   USBH_ERR_NONE               Message queue created.
*                                   USBH_ERR_ALLOC              Message queue creation failed.
*                                   USBH_ERR_OS_SIGNAL_CREATE
*
* Return(s)   : The handle of the message queue.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_HQUEUE  USBH_OS_MsgQueueCreate (void        **p_start,
                                     CPU_INT16U    size,
                                     USBH_ERR     *p_err)
{
    OS_Q     *p_q;
    OS_ERR    err_os;
    LIB_ERR   err_lib;


    (void)p_start;

    p_q = (OS_Q *)Mem_PoolBlkGet(&USBH_OS_QPool,
                                  sizeof(OS_Q),
                                 &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return ((USBH_HQUEUE)0);
    }

    OSQCreate(p_q, "USB Q", size, &err_os);
    if (err_os != OS_ERR_NONE) {
       *p_err = USBH_ERR_OS_SIGNAL_CREATE;
        return ((USBH_HQUEUE)0);
    }

   *p_err = USBH_ERR_NONE;

    return ((USBH_HQUEUE)p_q);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MsgQueuePut()
*
* Description : Post a message to a message queue.
*
* Argument(s) : msg_q     Message queue handle which passes a queue object (OS_Q) to the
*                         uCOS-III function
*
*               p_msg     Pointer to the message to send.
*
* Return(s)   : USBH_ERR_NONE,      if successful.
*               USBH_ERR_OS_FAIL,   otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MsgQueuePut (USBH_HQUEUE   msg_q,
                               void         *p_msg)
{
    OS_ERR  err;


    OSQPost((OS_Q *)msg_q,
                    p_msg,
                    sizeof(void *),
                    OS_OPT_POST_1,
                   &err);
    if (err != OS_ERR_NONE) {
        return (USBH_ERR_OS_FAIL);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MsgQueueGet()
*
* Description : Get a message from a message queue and blocks forever until a message is posted in the
*               queue
*
* Argument(s) : msg_q     Message queue handle which passes a queue object (OS_Q) to the
*                         uCOS-III function
*
*               timeout   Time to wait for a message, in milliseconds.
*
*               p_err     Pointer to variable that will receive the return error code from this function :
*
*                         USBH_ERR_NONE,           if successful.
*                         USBH_ERR_OS_TIMEOUT,
*                         USBH_ERR_OS_ABORT,
*                         USBH_ERR_OS_FAIL,
*
* Return(s)   : Pointer to message, if successful.
*               Pointer to null,    otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  *USBH_OS_MsgQueueGet (USBH_HQUEUE   msg_q,
                            CPU_INT32U    timeout,
                            USBH_ERR     *p_err)
{
    OS_ERR        err_os;
    OS_MSG_SIZE   msg_size;
    void         *p_msg;


    p_msg = OSQPend((OS_Q   *)msg_q,
                              timeout,
                              OS_OPT_PEND_BLOCKING,
                             &msg_size,
                    (CPU_TS *)0,
                             &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
            *p_err = USBH_ERR_NONE;
             break;

        case OS_ERR_TIMEOUT:
            *p_err = USBH_ERR_OS_TIMEOUT;
             break;

        case OS_ERR_PEND_ABORT:
            *p_err = USBH_ERR_OS_ABORT;
             break;

        case OS_ERR_OBJ_PTR_NULL:
        case OS_ERR_OBJ_TYPE:
        case OS_ERR_PEND_ISR:
        case OS_ERR_PEND_WOULD_BLOCK:
        case OS_ERR_SCHED_LOCKED:
        default:
            *p_err = USBH_ERR_OS_FAIL;
             break;
    }

    (void)msg_size;

    return (p_msg);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                    INTERNAL USE TIMER FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBH_OS_TmrCreate()
*
* Description : Create a one-shot timer.
*
* Argument(s) : p_name          Pointer to name of the timer.
*
*               interval_ms     Delay used by the timer, in milliseconds.
*
*               p_callback      Pointer to the callback function that will be called on completion of timer.
*                               The callback function must be declared as follows:
*
*                               void MyCallback (void *ptmr, void *p_arg);
*
*               p_callback_arg  Argument passed to callback function.
*
*               p_err           Pointer to variable that will receive the return error code from this function:
*
*                                   USBH_ERR_NONE               Software timer created.
*                                   USBH_ERR_ALLOC              Software timer creation failed.
*                                   USBH_ERR_OS_SIGNAL_CREATE
*
* Return(s)   : Handle on On-Shot Timer.
*
* Note(s)     : This function should never be called by your application.
*********************************************************************************************************
*/

USBH_HTMR  USBH_OS_TmrCreate (CPU_CHAR     *p_name,
                              CPU_INT32U    interval_ms,
                              void        (*p_callback)(void *p_tmr, void *p_arg),
                              void         *p_callback_arg,
                              USBH_ERR     *p_err)
{
#if (OS_CFG_TMR_EN == DEF_ENABLED)
    OS_TMR   *p_tmr;
    OS_ERR    err_os;
    LIB_ERR   err_lib;


    p_tmr = (OS_TMR *)Mem_PoolBlkGet(&USBH_OS_TmrPool,
                                      sizeof(OS_TMR),
                                     &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    OSTmrCreate(                        p_tmr,
                                       "USB-Host Tmr",
                                        USBH_OS_TICK_TIMEOUT_mS(interval_ms),
                                        0u,
                                        OS_OPT_TMR_ONE_SHOT,
                 (OS_TMR_CALLBACK_PTR)(*p_callback),
                                        p_callback_arg,
                                       &err_os);
    if (err_os != OS_ERR_NONE) {
       *p_err = USBH_ERR_OS_SIGNAL_CREATE;
        return ((USBH_HTMR)0);
    }

   *p_err = USBH_ERR_NONE;
    return ((USBH_HTMR)p_tmr);
#else
   *p_err = USBH_ERR_OS_FAIL;
    return ((USBH_HTMR)0);
#endif
}


/*
*********************************************************************************************************
*                                         USBH_OS_TmrStart()
*
* Description : Start a timer.
*
* Argument(s) : tmr     Handle on timer.
*
* Return(s)   : USBH_ERR_NONE           If timer start was successful.
*               USBH_ERR_INVALID_ARG    If invalid argument passed to 'tmr'.
*               USBH_ERR_OS_FAIL        Otherwise.
*
* Note(s)     : This function should never be called by your application.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_TmrStart (USBH_HTMR  tmr)
{
    USBH_ERR  err;
#if (OS_CFG_TMR_EN == DEF_ENABLED)
    OS_ERR    err_os;


    OSTmrStart((OS_TMR *) tmr,
                         &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_TMR_INVALID:
        case OS_ERR_OBJ_TYPE:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_TMR_ISR:
        case OS_ERR_TMR_INACTIVE:
        case OS_ERR_TMR_INVALID_STATE:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

#else
    err = USBH_ERR_OS_FAIL;
#endif
    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_OS_TmrDel()
*
* Description : Delete a timer.
*
* Argument(s) : tmr     Handle on timer.
*
* Return(s)   : USBH_ERR_NONE           If timer delete was successful.
*               USBH_ERR_OS_FAIL        If timer delete failed
*               USBH_ERR_FREE           If timer free failed.
*
* Note(s)     : This function should never be called by your application.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_TmrDel (USBH_HTMR  tmr)
{
#if (OS_CFG_TMR_EN == DEF_ENABLED)
    OS_ERR   err_os;
    LIB_ERR  err_lib;


    OSTmrDel((OS_TMR *) tmr,
                       &err_os);
    if (err_os != OS_ERR_NONE) {
        return (USBH_ERR_OS_FAIL);
    }

    Mem_PoolBlkFree(&USBH_OS_TmrPool, (void *)tmr, &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_FREE);
    }
#endif

    return (USBH_ERR_NONE);
}

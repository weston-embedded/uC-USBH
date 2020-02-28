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
*                                              TEMPLATE
*
* Filename : usbh_os.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_OS_MODULE
#define   MICRIUM_SOURCE

#include  <usbh_cfg.h>
#include  "../../Source/usbh_os.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_OS_MUTEX_REQUIRED             ((((USBH_CFG_MAX_NBR_EPS * USBH_CFG_MAX_NBR_IFS) + 1u) * USBH_CFG_MAX_NBR_DEVS) + \
                                             USBH_CFG_MAX_NBR_DEVS + USBH_CFG_MAX_NBR_HC +                                    \
                                             USBH_CDC_CFG_MAX_DEV + USBH_HID_CFG_MAX_DEV + USBH_MSC_CFG_MAX_DEV)
#define  USBH_OS_SEM_REQUIRED               (3u + (((USBH_CFG_MAX_NBR_EPS * USBH_CFG_MAX_NBR_IFS) + 1u) * USBH_CFG_MAX_NBR_DEVS))
#define  USBH_OS_TCB_REQUIRED                 4u
#define  USBH_OS_Q_REQUIRED                   1u


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


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
    return (USBH_ERR_NONE);
}


/*
**************************************************************************************************************
*                                       USBH_OS_VirToBus()
*
* Description : Convert from virtual address to physical address if the operating system uses
*               virtual memory
*
* Arguments   : x       Virtual address to convert
*
* Returns     : The corresponding physical address
*
* Note(s)     : None.
**************************************************************************************************************
*/

void  *USBH_OS_VirToBus (void  *x)
{
    return (x);
}


/*
**************************************************************************************************************
*                                       USBH_OS_BusToVir()
*
* Description : Convert from physical address to virtual address if the operating system uses
*               virtual memory
*
* Arguments   : x       Physical address to convert
*
* Returns     : The corresponding virtual address
*
* Note(s)     : None.
**************************************************************************************************************
*/

void  *USBH_OS_BusToVir (void  *x)
{
    return (x);
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
    (void)dly;
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
    (void)dly;
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
    (void)p_mutex;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                         USBH_OS_MutexLock()
*
* Description : Acquire a mutex.
*
* Argument(s) : mutex     Mutex handle.
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
    (void)mutex;

    return (USBH_ERR_NONE);

}


/*
*********************************************************************************************************
*                                        USBH_OS_MutexUnlock()
*
* Description : Releases a mutex.
*
* Argument(s) : mutex     Mutex handle.
*
* Return(s)   : USBH_ERR_NONE,      if successful.
*               USBH_ERR_OS_FAIL,   otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexUnlock (USBH_HMUTEX  mutex)
{
    (void)mutex;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MutexDestroy()
*
* Description : Destroys a mutex only when no task is pending on the mutex.
*
* Argument(s) : mutex     Mutex handle.
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
    (void)mutex;

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
    (void)p_sem;
    (void)cnt;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                         USBH_OS_SemDestroy()
*
* Description : Destroy a semphore if no tasks are waiting on it.
*
* Argument(s) : sem       Semaphore handle.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemDestroy (USBH_HSEM  sem)
{
    (void)sem;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_OS_SemWait()
*
* Description : Wait on a semaphore to become available.
*
* Argument(s) : sem       Semaphore handle.
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
    (void)sem;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                       USBH_OS_SemWaitAbort()
*
* Description : Resume all tasks waiting on specified semaphore.
*
* Argument(s) : sem       Semaphore handle.
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
    (void)sem;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_OS_SemPost()
*
* Description : Post a semaphore.
*
* Argument(s) : sem       Semaphore handle.
*
* Return(s)   : USBH_ERR_NONE,      if successful.
*               USBH_ERR_OS_FAIL,   otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemPost (USBH_HSEM  sem)
{
    (void)sem;

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
    (void)p_name;
    (void)prio;
    (void)task_fnct;
    (void)p_data;
    (void)p_stk;
    (void)stk_size;
    (void)p_task;

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
    (void)p_start;
    (void)size;

   *p_err = USBH_ERR_NONE;

    return ((USBH_HQUEUE)0);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MsgQueuePut()
*
* Description : Post a message to a message queue.
*
* Argument(s) : msg_q     Message queue handle.
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
    (void)msg_q;
    (void)p_msg;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MsgQueueGet()
*
* Description : Get a message from a message queue and blocks forever until a message is posted in the
*               queue.
*
* Argument(s) : msg_q     Message queue handle.
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
    (void)msg_q;
    (void)timeout;

   *p_err = USBH_ERR_NONE;

    return ((void *)0);
}


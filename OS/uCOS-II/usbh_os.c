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
*                                          Micrium uC/OS-II
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
#include  <cpu.h>
#include  <Source/ucos_ii.h>
#include  "../../Source/usbh_os.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


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
#if OS_TMR_EN > 0
typedef  struct  usbh_tmr_handle {
    OS_TMR   *TmrPtr;                                           /* Ptr to an OS-II tmr obj.                             */

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


/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/

                                                                /* Ms converted to ticks (rounded up).                  */
#define  USBH_OS_TICK_TIMEOUT(ms_timeout)    (((ms_timeout) > 0u) ? ((((ms_timeout) * OS_TICKS_PER_SEC) + 1000u - 1u) / 1000u) : 0u)


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
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_LayerInit (void)
{
    return (USBH_ERR_NONE);
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
* Description : Delay current task by specified delay in milliseconds.
*
* Argument(s) : dly         Delay, in milliseconds.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_OS_DlyMS (CPU_INT32U  dly)
{
    OSTimeDly(USBH_OS_TICK_TIMEOUT(dly));
}


/*
*********************************************************************************************************
*                                           USBH_OS_DlyUS()
*
* Description : Delay current task by specified delay in microseconds.
*
* Argument(s) : dly         Delay, in microseconds.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_OS_DlyUS (CPU_INT32U  dly)
{
    CPU_INT32U  temp;


    temp = dly / 1000000u;

    if (temp == 0u) {
        OSTimeDly(1u);
    } else {                                                    /* uC/OS-II does not provide microsecond resolution.   */
        OSTimeDly(temp);
    }
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
* Argument(s) : p_mutex         Pointer that will receive handle of mutex.
*
* Return(s)   : USBH_ERR_NONE,                  if successful.
*               USBH_ERR_OS_SIGNAL_CREATE,      if mutex creation failed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexCreate (USBH_HMUTEX  *p_mutex)
{
    OS_EVENT  *p_event;


    p_event = OSSemCreate(1u);                                  /* Binary sem to create mutex service.                  */
    if (p_event == (OS_EVENT *)0) {
        return (USBH_ERR_OS_SIGNAL_CREATE);
    }

   *p_mutex = (USBH_HMUTEX)p_event;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                         USBH_OS_MutexLock()
*
* Description : Acquire a mutex.
*
* Argument(s) : mutex       Handle on mutex.
*
* Return(s)   : USBH_ERR_NONE,          If mutex acquired successfully.
*               USBH_ERR_INVALID_ARG,   If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,      If mutex wait aborted.
*               USBH_ERR_OS_FAIL,       Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexLock (USBH_HMUTEX  mutex)
{
    INT8U      err_os;
    OS_EVENT  *p_event;
    USBH_ERR   err;


    p_event = (OS_EVENT *)mutex;

    OSSemPend (p_event,
               0u,
              &err_os);
    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_PEND_ABORT:
             err = USBH_ERR_OS_ABORT;
             break;

        case OS_ERR_PEND_LOCKED:
        case OS_ERR_PEND_ISR:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MutexUnlock()
*
* Description : Release a mutex.
*
* Argument(s) : mutex       Handle on mutex.
*
* Return(s)   : USBH_ERR_NONE,          If mutex released successfully.
*               USBH_ERR_INVALID_ARG,   If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_FAIL,       Otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexUnlock (USBH_HMUTEX  mutex)
{
    OS_EVENT  *p_event;
    INT8U      err_os;
    USBH_ERR   err;


    p_event = (OS_EVENT *)mutex;

    err_os = OSSemPost(p_event);
    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
             err = USBH_ERR_INVALID_ARG;
             break;

        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MutexDestroy()
*
* Description : Destroy a mutex.
*
* Argument(s) : mutex       Handle on mutex.
*
* Return(s)   : USBH_ERR_NONE,          If mutex destroyed successfully.
*               USBH_ERR_INVALID_ARG,   If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_FAIL,       Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MutexDestroy (USBH_HMUTEX  mutex)
{
    OS_EVENT  *p_event;
    INT8U      err_os;
    USBH_ERR   err;


    p_event = (OS_EVENT *)mutex;

    OSSemDel(p_event,
             OS_DEL_NO_PEND,
            &err_os);
    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_DEL_ISR:
        case OS_ERR_TASK_WAITING:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

    return (err);
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
* Description : Create a semaphore with given count.
*
* Argument(s) : p_sem       Pointer to handle on semaphore.
*
*               cnt         Count of semaphore.
*
* Return(s)   : USBH_ERR_NONE,                  if semaphore created successfully.
*               USBH_ERR_OS_SIGNAL_CREATE,      If semaphore creation failed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemCreate (USBH_HSEM   *p_sem,
                             CPU_INT32U   cnt)
{
    OS_EVENT  *p_event;


    p_event = OSSemCreate((CPU_INT16U)cnt);
    if (p_event == (OS_EVENT *)0) {
        return (USBH_ERR_OS_SIGNAL_CREATE);
    }

   *p_sem = (USBH_HSEM)p_event;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_OS_SemWait()
*
* Description : Pend on semaphore until it becomes available.
*
* Argument(s) : sem             Handle on semaphore.
*
*               timeout         Timeout in milliseconds.
*
* Return(s)   : USBH_ERR_NONE           If semaphore pend was successful.
*               USBH_ERR_INVALID_ARG    If invalid argument passed to 'sem'.
*               USBH_ERR_OS_TIMEOUT     If semaphore pend reached specified timeout.
*               USBH_ERR_OS_ABORT       If pend on semaphore was aborted.
*               USBH_ERR_OS_FAIL        Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemWait (USBH_HSEM   sem,
                           CPU_INT32U  timeout)
{
    OS_EVENT  *p_event;
    INT8U      err_os;
    USBH_ERR   err;


    p_event = (OS_EVENT *)sem;

    OSSemPend(p_event,
              USBH_OS_TICK_TIMEOUT(timeout),
             &err_os);
    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_TIMEOUT:
             err = USBH_ERR_OS_TIMEOUT;
             break;

        case OS_ERR_PEND_ABORT:
             err = USBH_ERR_OS_ABORT;
             break;

        case OS_ERR_PEND_ISR:
        case OS_ERR_PEND_LOCKED:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

    return (err);
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
    OS_EVENT  *p_event;
    INT8U      err_os;


    p_event = (OS_EVENT *)sem;

    (void)OSSemPendAbort(p_event,
                         OS_PEND_OPT_BROADCAST,
                        &err_os);
    switch (err_os) {
        case OS_ERR_NONE:
             return (USBH_ERR_NONE);
             break;

        case OS_ERR_PEND_ABORT:
        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
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
* Argument(s) : sem             Handle on semaphore.
*
* Return(s)   : USBH_ERR_NONE,              if semaphore post was successful.
*               USBH_ERR_INVALID_ARG,       if invalid argument passed to 'sem'.
*               USBH_ERR_OS_FAIL,           otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemPost (USBH_HSEM  sem)
{
    OS_EVENT  *p_event;
    INT8U      err_os;
    USBH_ERR   err;


    p_event = (OS_EVENT *)sem;

    err_os = OSSemPost(p_event);
    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_SEM_OVF:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_OS_SemDestroy()
*
* Description : Destroy a semphore if no tasks are waiting on it.
*
* Argument(s) : sem         Handle on semaphore.
*
* Return(s)   : USBH_ERR_NONE,          if semaphore successfully destroyed.
*               USBH_ERR_INVALID_ARG,   if invalid argument passed to 'sem'.
*               USBH_ERR_OS_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_SemDestroy (USBH_HSEM  sem)
{
    OS_EVENT  *p_event;
    INT8U      err_os;
    USBH_ERR   err;


    p_event = (OS_EVENT *)sem;

    OSSemDel(p_event,
             OS_DEL_NO_PEND,
            &err_os);
    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_DEL_ISR:
        case OS_ERR_TASK_WAITING:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

    return (err);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           TASK FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        USBH_OS_TaskCreate()
*
* Description : Create a task.
*
* Argument(s) : p_name          Pointer to name to assign to task.
*
*               prio            Priority of task to be created.
*
*               task_fnct       Pointer to function that will be executed in this task.
*
*               p_data          Pointer to data that is passed to the task function.
*
*               p_stk           Pointer to beginning of the stack used by the task.
*
*               stk_size        Size of stack in octets.
*
*               p_task          Pointer that will receive handle for managing task.
*
* Returns     : USBH_ERR_NONE,                  if task created successfully.
*               USBH_ERR_OS_TASK_CREATE,        otherwise.
*
* Note(s)     : None.
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
    OS_STK      *os_stk;
    CPU_INT32U   os_stk_len;
    INT8U        err_os;
    USBH_ERR     err;


    os_stk     = (OS_STK *)p_stk;
    os_stk_len =  stk_size * sizeof (CPU_INT32U) / sizeof (OS_STK);
   *p_task     =  prio;

#if (OS_STK_GROWTH == 0)                                        /* Stack ptr grows up.                                  */
    err_os = OSTaskCreateExt(          task_fnct,
                                       p_data,
                                      &os_stk[0],
                             (INT8U   )prio,
                             (INT16U  )prio,
                                      &os_stk[os_stk_len - 1],
                                       os_stk_len,
                             (void   *)0,
                                      (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
#else                                                           /* Stack ptr grows down.                                */
    err_os = OSTaskCreateExt(          task_fnct,
                                       p_data,
                                      &os_stk[os_stk_len - 1],
                             (INT8U   )prio,
                             (INT16U  )prio,
                                      &os_stk[0],
                                       os_stk_len,
                             (void   *)0,
                                      (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
#endif
    if (err_os == OS_ERR_NONE) {
        err = USBH_ERR_NONE;
    } else {
        err = USBH_ERR_OS_TASK_CREATE;
        return (err);
    }

#if (OS_VERSION < 287)
#if (OS_TASK_NAME_SIZE > 1)
    if (p_name != (CPU_CHAR *)0) {
        OSTaskNameSet((INT8U  )prio,
                      (INT8U *)p_name,
                              &err_os);
    }
#endif
#else
#if (OS_TASK_NAME_EN > 0)
    if (p_name != (CPU_CHAR *)0) {
        OSTaskNameSet((INT8U  )prio,
                      (INT8U *)p_name,
                              &err_os);
    }
#endif
#endif

    return (err);
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
* Argument(s) : p_start         Pointer to base address of the message queue storage area.
*
*               size            Number of elements in storage area.
*
*               p_err           Pointer to variable that will receive the return error code from this function:
*
*                                   USBH_ERR_NONE                   Message queue created.
*                                   USBH_ERR_OS_SIGNAL_CREATE       Message queue creation failed.
*
* Return(s)   : Handle on message queue.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_HQUEUE  USBH_OS_MsgQueueCreate (void        **p_start,
                                     CPU_INT16U    size,
                                     USBH_ERR     *p_err)
{
    OS_EVENT  *p_event;


    p_event = OSQCreate(p_start, size);

    if (p_event == (OS_EVENT *)0) {
       *p_err = USBH_ERR_OS_SIGNAL_CREATE;
        return ((USBH_HQUEUE )0);
    }

   *p_err = USBH_ERR_NONE;
    return ((USBH_HQUEUE)p_event);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MsgQueuePut()
*
* Description : Post message in message queue.
*
* Argument(s) : msg_q       Handle on message queue.
*
*               p_msg       Pointer to message to post.
*
* Return(s)   : USBH_ERR_NONE,              If message successfully posted in message queue.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'msg_q'.
*               USBH_ERR_OS_FAIL,           Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_MsgQueuePut (USBH_HQUEUE   msg_q,
                               void         *p_msg)
{
    OS_EVENT  *p_event;
    INT8U      err_os;
    USBH_ERR   err;


    p_event = (OS_EVENT *)msg_q;

    err_os = OSQPost(p_event, p_msg);
    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_Q_FULL:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_OS_MsgQueueGet()
*
* Description : Get message from message queue.
*
* Argument(s) : msg_q           Handle on message queue.
*
*               timeout         Timeout, in ms.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE,              Message from message queue successfully retrieved.
*                       USBH_ERR_INVALID_ARG,       Invalid argument passed to 'msg_q'.
*                       USBH_ERR_OS_TIMEOUT,        Specified timeout reached.
*                       USBH_ERR_OS_ABORT,          Pend on queue aborted.
*                       USBH_ERR_OS_FAIL,           Otherwise.
*
* Return(s)   : Pointer to message, if successful.
*               NULL,               otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  *USBH_OS_MsgQueueGet (USBH_HQUEUE   msg_q,
                            CPU_INT32U    timeout,
                            USBH_ERR     *p_err)
{
    OS_EVENT  *p_event;
    INT8U      err_os;
    void      *p_msg;


    p_event = (OS_EVENT *)msg_q;

    p_msg  = OSQPend(p_event,
                     USBH_OS_TICK_TIMEOUT(timeout),
                    &err_os);
    switch (err_os) {
        case OS_ERR_NONE:
            *p_err = USBH_ERR_NONE;
             break;

        case OS_ERR_EVENT_TYPE:
        case OS_ERR_PEVENT_NULL:
            *p_err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_TIMEOUT:
            *p_err = USBH_ERR_OS_TIMEOUT;
             break;

        case OS_ERR_PEND_ABORT:
            *p_err = USBH_ERR_OS_ABORT;
             break;

        case OS_ERR_PEND_ISR:
        case OS_ERR_PEND_LOCKED:
        default:
            *p_err = USBH_ERR_OS_FAIL;
             break;
    }

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
*                                   USBH_ERR_NONE                   Timer created.
*                                   USBH_ERR_OS_FAIL                Timer creation failed.
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
#if OS_TMR_EN > 0
    INT8U    err_os;
    OS_TMR  *p_tmr;


    p_tmr = OSTmrCreate(                   USBH_OS_TICK_TIMEOUT(interval_ms),
                                           0u,
                                           OS_TMR_OPT_ONE_SHOT,
                        (OS_TMR_CALLBACK)(*p_callback),
                                           p_callback_arg,
                                 (INT8U*)  p_name,
                                          &err_os);

    if (p_tmr == (OS_TMR *)0u) {
       *p_err = USBH_ERR_OS_FAIL;
        return ((USBH_HTMR )0);
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
* Argument(s) : tmr             Handle on timer.
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
#if OS_TMR_EN > 0
    INT8U     err_os;


    OSTmrStart((OS_TMR *) tmr,
                         &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_TMR_INVALID:
        case OS_ERR_TMR_INVALID_TYPE:
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
* Argument(s) : tmr             Handle on timer.
*
* Return(s)   : USBH_ERR_NONE           If timer delete was successful.
*               USBH_ERR_INVALID_ARG    If invalid argument passed to 'tmr'.
*               USBH_ERR_OS_FAIL        Otherwise.
*
* Note(s)     : This function should never be called by your application.
*********************************************************************************************************
*/

USBH_ERR  USBH_OS_TmrDel (USBH_HTMR  tmr)
{
    USBH_ERR  err;
#if OS_TMR_EN > 0
    INT8U     err_os;


    OSTmrDel((OS_TMR *) tmr,
                       &err_os);

    switch (err_os) {
        case OS_ERR_NONE:
             err = USBH_ERR_NONE;
             break;

        case OS_ERR_TMR_INVALID:
        case OS_ERR_TMR_INVALID_TYPE:
             err = USBH_ERR_INVALID_ARG;
             break;

        case OS_ERR_TMR_ISR:
        case OS_ERR_TMR_INACTIVE:
        case OS_ERR_TMR_INVALID_STATE:
        case OS_ERR_ILLEGAL_DEL_RUN_TIME:
        default:
             err = USBH_ERR_OS_FAIL;
             break;
    }
#else
    err = USBH_ERR_NONE;
#endif
    return (err);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            MISCELLANEOUS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
**************************************************************************************************************
*                                         USBH_OS_VirToBus()
*
* Description : Convert from virtual address to physical address if operating system uses virtual memory.
*
* Arguments   : x       Virtual address to convert
*
* Returns     : Corresponding physical address
*
* Note(s)     : (1) uC/OS-II doesn't use virtual memory.
**************************************************************************************************************
*/

void  *USBH_OS_VirToBus (void  *x)
{
    return (x);                                                 /* See Note #1.                                         */
}


/*
**************************************************************************************************************
*                                         USBH_OS_BusToVir()
*
* Description : Convert from physical address to virtual address if operating system uses virtual memory.
*
* Arguments   : x       Physical address to convert
*
* Returns     : Corresponding virtual address
*
* Note(s)     : (1) uC/OS-II doesn't use virtual memory.
**************************************************************************************************************
*/

void  *USBH_OS_BusToVir (void  *x)
{
    return (x);                                                 /* See Note #1.                                         */
}

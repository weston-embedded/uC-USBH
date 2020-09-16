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
*                                      USB HOST CORE OPERATIONS
*
* Filename : usbh_core.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_CORE_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_core.h"
#include  "usbh_class.h"
#include  "usbh_hub.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      USB HOST DRIVER FUNCTIONS
*********************************************************************************************************
*/

#define  USBH_HCD_Init(p_hc, p_err)                     do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->Init(&(p_hc)->HC_Drv, (p_err));                                   \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)                                                                                   \

#define  USBH_HCD_Start(p_hc, p_err)                    do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->Start(&(p_hc)->HC_Drv, (p_err));                                  \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)                                                                                   \

#define  USBH_HCD_Stop(p_hc, p_err)                     do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->Stop(&(p_hc)->HC_Drv, (p_err));                                   \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)                                                                                   \

#define  USBH_HCD_SpdGet(p_hc, p_spd, p_err)            do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            *(p_spd) = (p_hc)->HC_Drv.API_Ptr->SpdGet(&(p_hc)->HC_Drv, (p_err));                      \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)                                                                                   \

#define  USBH_HCD_Suspend(p_hc, p_err)                  do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->Suspend(&(p_hc)->HC_Drv, (p_err));                                \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)                                                                                   \

#define  USBH_HCD_Resume(p_hc, p_err)                   do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->Resume(&(p_hc)->HC_Drv, (p_err));                                 \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)                                                                                   \

#define  USBH_HCD_FrmNbrGet(p_hc, p_frm_nbr, p_err)     do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            *(p_frm_nbr) = (p_hc)->HC_Drv.API_Ptr->FrmNbrGet(&(p_hc)->HC_Drv, (p_err));               \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)                                                                                   \

#define  USBH_HCD_EP_Open(p_hc, p_ep, p_err)            do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->EP_Open(&(p_hc)->HC_Drv,                                          \
                                                                                            (p_ep),                                                   \
                                                                                            (p_err));                                                 \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)

#define  USBH_HCD_EP_Close(p_hc, p_ep, p_err)           do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->EP_Close(&(p_hc)->HC_Drv,                                         \
                                                                                             (p_ep),                                                  \
                                                                                             (p_err));                                                \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)

#define  USBH_HCD_EP_Abort(p_hc, p_ep, p_err)           do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->EP_Abort(&(p_hc)->HC_Drv,                                         \
                                                                                             (p_ep),                                                  \
                                                                                             (p_err));                                                \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)

#define  USBH_HCD_EP_IsHalt(p_hc, p_ep, b_ret, p_err)   do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            *(b_ret) = (p_hc)->HC_Drv.API_Ptr->EP_IsHalt(&(p_hc)->HC_Drv,                             \
                                                                                                         (p_ep),                                      \
                                                                                                         (p_err));                                    \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)

#define  USBH_HCD_URB_Submit(p_hc, p_urb, p_err)        do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->URB_Submit(&(p_hc)->HC_Drv,                                       \
                                                                                               (p_urb),                                               \
                                                                                               (p_err));                                              \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)


#define  USBH_HCD_URB_Complete(p_hc, p_urb, p_err)      do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->URB_Complete(&(p_hc)->HC_Drv,                                     \
                                                                                                (p_urb),                                              \
                                                                                                (p_err));                                             \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)

#define  USBH_HCD_URB_Abort(p_hc, p_urb, p_err)         do {                                                                                          \
                                                            USBH_OS_MutexLock((p_hc)->HCD_Mutex);                                                     \
                                                            (p_hc)->HC_Drv.API_Ptr->URB_Abort(&(p_hc)->HC_Drv,                                        \
                                                                                             (p_urb),                                                 \
                                                                                             (p_err));                                                \
                                                            USBH_OS_MutexUnlock((p_hc)->HCD_Mutex);                                                   \
                                                        } while (0)


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

static  volatile  USBH_URB   *USBH_URB_HeadPtr;
static  volatile  USBH_URB   *USBH_URB_TailPtr;
static  volatile  USBH_HSEM   USBH_URB_Sem;


/*
*********************************************************************************************************
*                                         HOST MAIN STRUCTURE
*********************************************************************************************************
*/

static  USBH_HOST   USBH_Host;
static  CPU_INT32U  USBH_Version;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  USBH_ERR        USBH_EP_Open     (USBH_DEV        *p_dev,
                                          USBH_IF         *p_if,
                                          USBH_EP_TYPE     ep_type,
                                          USBH_EP_DIR      ep_dir,
                                          USBH_EP         *p_ep);

static  CPU_INT32U      USBH_SyncXfer    (USBH_EP         *p_ep,
                                          void            *p_buf,
                                          CPU_INT32U       buf_len,
                                          USBH_ISOC_DESC  *p_isoc_desc,
                                          USBH_TOKEN       token,
                                          CPU_INT32U       timeout_ms,
                                          USBH_ERR        *p_err);

static  USBH_ERR        USBH_AsyncXfer   (USBH_EP         *p_ep,
                                          void            *p_buf,
                                          CPU_INT32U       buf_len,
                                          USBH_ISOC_DESC  *p_isoc_desc,
                                          USBH_TOKEN       token,
                                          void            *p_fnct,
                                          void            *p_fnct_arg);

static  CPU_INT16U      USBH_SyncCtrlXfer(USBH_EP         *p_ep,
                                          CPU_INT08U       b_req,
                                          CPU_INT08U       bm_req_type,
                                          CPU_INT16U       w_val,
                                          CPU_INT16U       w_ix,
                                          void            *p_arg,
                                          CPU_INT16U       w_len,
                                          CPU_INT32U       timeout_ms,
                                          USBH_ERR        *p_err);

static  void            USBH_URB_Abort   (USBH_URB        *p_urb);

static  void            USBH_URB_Notify  (USBH_URB        *p_urb);

static  USBH_ERR        USBH_URB_Submit  (USBH_URB        *p_urb);

static  void            USBH_URB_Clr     (USBH_URB        *p_urb);

static  USBH_ERR        USBH_DfltEP_Open (USBH_DEV        *p_dev);

static  USBH_ERR        USBH_DevDescRd   (USBH_DEV        *p_dev);

static  USBH_ERR        USBH_CfgRd       (USBH_DEV        *p_dev,
                                          CPU_INT08U       cfg_ix);

static  USBH_ERR        USBH_CfgParse    (USBH_DEV        *p_dev,
                                          USBH_CFG        *p_cfg);

static  USBH_ERR        USBH_DevAddrSet  (USBH_DEV        *p_dev);

static  CPU_INT32U      USBH_StrDescGet  (USBH_DEV        *p_dev,
                                          CPU_INT08U       desc_ix,
                                          CPU_INT16U       lang_id,
                                          void            *p_buf,
                                          CPU_INT32U       buf_len,
                                          USBH_ERR        *p_err);

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
static  void            USBH_StrDescPrint(USBH_DEV        *p_dev,
                                          CPU_INT08U      *p_str_prefix,
                                          CPU_INT08U       desc_idx);
#endif

static  USBH_DESC_HDR  *USBH_NextDescGet (void            *p_buf,
                                          CPU_INT32U      *p_offset);

static  void            USBH_FmtSetupReq (USBH_SETUP_REQ  *p_setup_req,
                                          void            *p_buf_dest);

static  void            USBH_ParseDevDesc(USBH_DEV_DESC   *p_dev_desc,
                                          void            *p_buf_src);

static  void            USBH_ParseCfgDesc(USBH_CFG_DESC   *p_cfg_desc,
                                          void            *p_buf_src);

static  void            USBH_ParseIF_Desc(USBH_IF_DESC    *p_if_desc,
                                          void            *p_buf_src);

static  void            USBH_ParseEP_Desc(USBH_EP_DESC    *p_ep_desc,
                                          void            *p_buf_src);

static  void            USBH_AsyncTask   (void            *p_arg);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          USBH_VersionGet()
*
* Description : Get the uC/USB-Host software version.
*
* Argument(s) : None.
*
* Return(s)   : uC/USB-Host version.
*
* Note(s)     : The value returned is multiplied by 10000. For example, version 3.40.02, would be
*               returned as 34002.
*********************************************************************************************************
*/

CPU_INT32U  USBH_VersionGet (void)
{
    CPU_INT32U  version;


    version = USBH_VERSION;

    return (version);
}


/*
*********************************************************************************************************
*                                             USBH_Init()
*
* Description : Allocates and initializes resources required by USB Host stack.
*
* Argument(s) : async_task_info     Information on asynchronous task.
*
*               hub_task_info       Information on hub task.
*
* Return(s)   : USBH_ERR_NONE,                  If host stack initialization succeed.
*               USBH_ERR_ALLOC,                 If memory pool allocation failed.
*
*                                               ----- RETURNED BY USBH_ClassDrvReg() : -----
*               USBH_ERR_NONE,                  If the class driver is registered.
*               USBH_ERR_INVALID_ARG,           If invalid argument(s) passed to 'p_host'/ 'p_class_drv'.
*               USBH_ERR_CLASS_DRV_ALLOC,       If maximum class driver limit reached.
*
*                                               ----- RETURNED BY USBH_OS_MutexCreate() : -----
*               USBH_ERR_OS_SIGNAL_CREATE,      if mutex creation failed.
*
*                                               ----- RETURNED BY USBH_OS_SemCreate() : -----
*               USBH_ERR_OS_SIGNAL_CREATE,      If semaphore creation failed.
*
*                                               ----- RETURNED BY USBH_OS_TaskCreate() : -----
*               USBH_ERR_OS_TASK_CREATE,        Task failed to be created.
*
* Note(s)     : USBH_Init() must be called:
*               (1) Only once from a product s application.
*               (2) After product s OS has been initialized.
*********************************************************************************************************
*/

USBH_ERR  USBH_Init (USBH_KERNEL_TASK_INFO  *async_task_info,
                     USBH_KERNEL_TASK_INFO  *hub_task_info)
{
    USBH_ERR    err;
    LIB_ERR     err_lib;
    CPU_SIZE_T  octets_reqd;
    CPU_INT08U  ix;


    USBH_Version = USBH_VERSION;
    (void)USBH_Version;

    USBH_URB_HeadPtr = (USBH_URB *)0;
    USBH_URB_TailPtr = (USBH_URB *)0;

    err = USBH_OS_LayerInit();
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    USBH_Host.HC_NbrNext = 0u;
    USBH_Host.State      = USBH_HOST_STATE_NONE;

    for (ix = 0u; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {     /* Clr class drv struct table.                          */
        USBH_ClassDrvList[ix].ClassDrvPtr   = (USBH_CLASS_DRV       *)0;
        USBH_ClassDrvList[ix].NotifyFnctPtr = (USBH_CLASS_NOTIFY_FNCT)0;
        USBH_ClassDrvList[ix].NotifyArgPtr  = (void                 *)0;
        USBH_ClassDrvList[ix].InUse         =  0u;
    }

    err = USBH_ClassDrvReg(       &USBH_HUB_Drv,                /* Reg HUB class drv.                                   */
                                   USBH_HUB_ClassNotify,
                           (void *)0);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_OS_SemCreate((USBH_HSEM *)&USBH_URB_Sem,         /* Create a Semaphore for sync I/O req.                 */
                                          0u);
    if (err != USBH_ERR_NONE) {
        return (err);
    }
                                                                /* Create a task for processing async req.              */
    err = USBH_OS_TaskCreate(             "USBH_Asynctask",
                                           async_task_info->Prio,
                                           USBH_AsyncTask,
                             (void       *)0,
                             (CPU_INT32U *)async_task_info->StackPtr,
                                           async_task_info->StackSize,
                                          &USBH_Host.HAsyncTask);
    if (err != USBH_ERR_NONE) {
        return (err);
    }
                                                                /* Create a task for processing hub events.             */
    err = USBH_OS_TaskCreate(             "USBH_HUB_EventTask",
                                           hub_task_info->Prio,
                                           USBH_HUB_EventTask,
                             (void       *)0,
                             (CPU_INT32U *)hub_task_info->StackPtr,
                                           hub_task_info->StackSize,
                                          &USBH_Host.HHubTask);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    for (ix = 0u; ix < USBH_MAX_NBR_DEVS; ix++) {               /* Init USB dev list.                                   */
        USBH_Host.DevList[ix].DevAddr = ix + 1;                 /* USB addr is ix + 1. Addr 0 is rsvd.                  */
        err = USBH_OS_MutexCreate(&USBH_Host.DevList[ix].DfltEP_Mutex);

        if (err != USBH_ERR_NONE) {
            USBH_PRINT_ERR(err);
            return (err);
        }
    }

    Mem_PoolCreate (       &USBH_Host.IsocDescPool,             /* Create mem pool for USB isoc desc struct.            */
                    (void *)USBH_Host.IsocDesc,
                            sizeof(USBH_Host.IsocDesc),
                            USBH_CFG_MAX_ISOC_DESC,
                            sizeof(USBH_ISOC_DESC),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    Mem_PoolCreate (       &USBH_Host.DevPool,                  /* Create mem pool for USB device struct.               */
                    (void *)USBH_Host.DevList,
                            sizeof(USBH_Host.DevList),
                            USBH_MAX_NBR_DEVS,
                            sizeof(USBH_DEV),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    Mem_PoolCreate (       &USBH_Host.AsyncURB_Pool,            /* Create mem pool for extra URB used in async comm.    */
                    (void *)0,
                           (USBH_CFG_MAX_NBR_DEVS * USBH_CFG_MAX_EXTRA_URB_PER_DEV * sizeof(USBH_URB)),
                           (USBH_CFG_MAX_NBR_DEVS * USBH_CFG_MAX_EXTRA_URB_PER_DEV),
                            sizeof(USBH_URB),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    err = USBH_ERR_NONE;

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_Suspend()
*
* Description : Suspends USB Host Stack by calling suspend for every class driver loaded
*               and then calling the host controller suspend.
*
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE                       If host is suspended.
*               Host controller driver error,       Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_Suspend (void)
{
    CPU_INT08U   ix;
    USBH_HC     *hc;
    USBH_ERR     err;


    for (ix = 0u; ix < USBH_Host.HC_NbrNext; ix++) {
        hc = &USBH_Host.HC_Tbl[ix];

        USBH_ClassSuspend(hc->HC_Drv.RH_DevPtr);                /* Suspend RH, and all downstream dev.                  */
        USBH_HCD_Suspend(hc, &err);                             /* Suspend HC.                                          */
    }

    USBH_Host.State = USBH_HOST_STATE_SUSPENDED;

    return (err);
}


/*
*********************************************************************************************************
*                                            USBH_Resume()
*
* Description : Resumes USB Host Stack by calling host controller resume and then
*               calling resume for every class driver loaded.
*
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE,                      If host is resumed.
*               Host controller driver error,       Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_Resume (void)
{
    CPU_INT08U   ix;
    USBH_HC     *hc;
    USBH_ERR     err;


    for (ix = 0u; ix < USBH_Host.HC_NbrNext; ix++) {
        hc = &USBH_Host.HC_Tbl[ix];

        USBH_HCD_Resume(hc, &err);                              /* Resume HC.                                           */
        USBH_ClassResume(hc->HC_Drv.RH_DevPtr);                 /* Resume RH, and all downstream dev.                   */
    }

    USBH_Host.State = USBH_HOST_STATE_RESUMED;

    return (err);
}


/*
*********************************************************************************************************
*                                            USBH_HC_Add()
*
* Description : Add a host controller.
*
* Argument(s) : p_hc_cfg        Pointer to specific USB host controller configuration.
*
*               p_drv_api       Pointer to specific USB host controller driver API.
*
*               p_hc_rh_api     Pointer to specific USB host controller root hub driver API.
*
*               p_hc_bsp_api    Pointer to specific USB host controller board-specific API.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                       Host controller successfully added.
*                           USBH_ERR_INVALID_ARG                Invalid argument passed to 'p_hc_cfg' / 'p_drv_api' /
*                                                               'p_hc_rh_api' / 'p_hc_bsp_api'.
*                           USBH_ERR_HC_ALLOC                   Maximum number of host controller reached.
*                           USBH_ERR_DEV_ALLOC                  Cannot allocate device structure for root hub.
*                           Host controller driver error,       Otherwise.
*
*                                                               ----- RETURNED BY USBH_OS_MutexCreate() : -----
*                           USBH_ERR_OS_SIGNAL_CREATE,          if mutex creation failed.
*
* Return(s)   : Host Controller index,   if host controller successfully added.
*               USBH_HC_NBR_NONE,        Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT08U  USBH_HC_Add (USBH_HC_CFG      *p_hc_cfg,
                         USBH_HC_DRV_API  *p_drv_api,
                         USBH_HC_RH_API   *p_hc_rh_api,
                         USBH_HC_BSP_API  *p_hc_bsp_api,
                         USBH_ERR         *p_err)
{
    USBH_DEV     *p_rh_dev;
    CPU_INT08U    hc_nbr;
    LIB_ERR       err_lib;
    USBH_HC      *p_hc;
    USBH_HC_DRV  *p_hc_drv;
    CPU_SR_ALLOC();


    if ((p_hc_cfg     == (USBH_HC_CFG     *)0) ||               /* ------------------ VALIDATE ARGS ------------------- */
        (p_drv_api    == (USBH_HC_DRV_API *)0) ||
        (p_hc_rh_api  == (USBH_HC_RH_API  *)0) ||
        (p_hc_bsp_api == (USBH_HC_BSP_API *)0)) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (USBH_HC_NBR_NONE);
    }

    CPU_CRITICAL_ENTER();
    hc_nbr = USBH_Host.HC_NbrNext;
    if (hc_nbr >= USBH_CFG_MAX_NBR_HC) {                        /* Chk if HC nbr is valid.                              */
        CPU_CRITICAL_EXIT();
       *p_err = USBH_ERR_HC_ALLOC;
        return (USBH_HC_NBR_NONE);
    }
    USBH_Host.HC_NbrNext++;
    CPU_CRITICAL_EXIT();

    p_hc     = &USBH_Host.HC_Tbl[hc_nbr];
    p_hc_drv = &p_hc->HC_Drv;

                                                                /* Alloc dev struct for RH.                             */
    p_rh_dev = (USBH_DEV *)Mem_PoolBlkGet(           &USBH_Host.DevPool,
                                          (CPU_SIZE_T)sizeof(USBH_DEV),
                                                     &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_DEV_ALLOC;
        return (USBH_HC_NBR_NONE);
    }

    p_rh_dev->IsRootHub = (CPU_BOOLEAN)DEF_TRUE;
    p_rh_dev->HC_Ptr    =  p_hc;

    p_hc->HostPtr = &USBH_Host;

    if (p_hc_rh_api == (USBH_HC_RH_API *)0) {
        p_hc->IsVirRootHub = DEF_FALSE;
    } else {
        p_hc->IsVirRootHub = DEF_TRUE;
    }

    p_hc_drv->HC_CfgPtr   = p_hc_cfg;
    p_hc_drv->DataPtr     = (void *)0;
    p_hc_drv->RH_DevPtr   = p_rh_dev;
    p_hc_drv->API_Ptr     = p_drv_api;
    p_hc_drv->BSP_API_Ptr = p_hc_bsp_api;
    p_hc_drv->RH_API_Ptr  = p_hc_rh_api;
    p_hc_drv->Nbr         = hc_nbr;

   *p_err = USBH_OS_MutexCreate(&p_hc->HCD_Mutex);              /* Create mutex to sync access to HCD.                  */
    if (*p_err != USBH_ERR_NONE) {
        return (USBH_HC_NBR_NONE);
    }

    USBH_HCD_Init(p_hc, p_err);                                 /* Init HCD.                                            */
    if (*p_err != USBH_ERR_NONE) {
        return (USBH_HC_NBR_NONE);
    }

    USBH_HCD_SpdGet(p_hc, &p_rh_dev->DevSpd, p_err);

    return (hc_nbr);
}


/*
*********************************************************************************************************
*                                           USBH_HC_Start()
*
* Description : Start given host controller.
*
* Argument(s) : hc_nbr      Host controller number.
*
* Return(s)   : USBH_ERR_NONE                       If host controller successfully started.
*               USBH_ERR_INVALID_ARG                If invalid argument passed to 'hc_nbr'.
*               Host controller driver error,       Otherwise.
*
*                                                   ----- RETURNED BY USBH_DevConn() : -----
*               USBH_ERR_DESC_INVALID               If device contains 0 configurations
*               USBH_ERR_CFG_ALLOC                  If maximum number of configurations reached.
*               USBH_ERR_DESC_INVALID,              If invalid descriptor was fetched.
*               USBH_ERR_CFG_MAX_CFG_LEN,           If cannot allocate descriptor
*               USBH_ERR_UNKNOWN,                   Unknown error occurred.
*               USBH_ERR_INVALID_ARG,               Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,          Endpoint is not opened.
*               USBH_ERR_HC_IO,                     Root hub input/output error.
*               USBH_ERR_EP_STALL,                  Root hub does not support request.
*               USBH_ERR_DRIVER_NOT_FOUND           If no class driver was found.
*               USBH_ERR_OS_SIGNAL_CREATE,          If semaphore or mutex creation failed.
*               Host controller driver error,       Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HC_Start (CPU_INT08U  hc_nbr)
{
    USBH_ERR   err;
    USBH_HC   *p_hc;
    USBH_DEV  *p_rh_dev;


    if (hc_nbr >= USBH_Host.HC_NbrNext) {                       /* Chk if HC nbr is valid.                              */
        return (USBH_ERR_INVALID_ARG);
    }

    p_hc     = &USBH_Host.HC_Tbl[hc_nbr];
    p_rh_dev =  p_hc->HC_Drv.RH_DevPtr;

    err = USBH_DevConn(p_rh_dev);                               /* Add RH of given HC.                                  */
    if (err == USBH_ERR_NONE) {
        USBH_Host.State = USBH_HOST_STATE_RESUMED;
    } else {
        USBH_DevDisconn(p_rh_dev);
    }

    USBH_HCD_Start(p_hc, &err);

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_HC_Stop()
*
* Description : Stop the given host controller.
*
* Argument(s) : hc_nbr      Host controller number.
*
* Return(s)   : USBH_ERR_NONE,                      If host controller successfully stoped.
*               USBH_ERR_INVALID_ARG,               If invalid argument passed to 'hc_nbr'.
*               Host controller driver error,       Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HC_Stop (CPU_INT08U  hc_nbr)
{
    USBH_ERR   err;
    USBH_HC   *p_hc;
    USBH_DEV  *p_rh_dev;


    if (hc_nbr >= USBH_Host.HC_NbrNext) {                       /* Chk if HC nbr is valid.                              */
        return (USBH_ERR_INVALID_ARG);
    }

    p_hc     = &USBH_Host.HC_Tbl[hc_nbr];
    p_rh_dev =  p_hc->HC_Drv.RH_DevPtr;

    USBH_HCD_Stop(p_hc, &err);
    USBH_DevDisconn(p_rh_dev);                                  /* Disconn RH dev.                                      */

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_HC_PortEn()
*
* Description : Enable given port of given host controller's root hub.
*
* Argument(s) : hc_nbr    Host controller number.
*
*               port_nbr  Port number.
*
* Return(s)   : USBH_ERR_NONE,                          If port successfully enabled.
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'hc_nbr'.
*
*                                                       ----- RETURNED BY USBH_HUB_PortEn() : -----
*               USBH_ERR_INVALID_ARG,                   If invalid parameter passed to 'p_hub_dev'.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HC_PortEn (CPU_INT08U  hc_nbr,
                          CPU_INT08U  port_nbr)
{
    USBH_ERR   err;
    USBH_HC   *p_hc;


    if (hc_nbr >= USBH_Host.HC_NbrNext) {                       /* Chk if HC nbr is valid.                              */
        return (USBH_ERR_INVALID_ARG);
    }

    p_hc = &USBH_Host.HC_Tbl[hc_nbr];
    err  =  USBH_HUB_PortEn(p_hc->RH_ClassDevPtr, port_nbr);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_HC_PortDis()
*
* Description : Disable given port of given host controller's root hub.
*
* Argument(s) : hc_nbr    Host controller number.
*
*               port_nbr  Port number.
*
* Return(s)   : USBH_ERR_NONE,                          If port successfully disabled.
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'hc_nbr'.
*
*                                                       ----- RETURNED BY USBH_HUB_PortDis() : -----
*               USBH_ERR_INVALID_ARG,                   If invalid parameter passed to 'p_hub_dev'.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None
*********************************************************************************************************
*/

USBH_ERR  USBH_HC_PortDis (CPU_INT08U  hc_nbr,
                           CPU_INT08U  port_nbr)
{
    USBH_ERR   err;
    USBH_HC   *p_hc;


    if (hc_nbr >= USBH_Host.HC_NbrNext) {                       /* Chk if HC nbr is valid.                              */
        return (USBH_ERR_INVALID_ARG);
    }

    p_hc = &USBH_Host.HC_Tbl[hc_nbr];
    err  =  USBH_HUB_PortDis(p_hc->RH_ClassDevPtr, port_nbr);
    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_HC_FrameNbrGet()
*
* Description : Get current frame number.
*
* Argument(s) : hc_nbr  Index of Host Controller.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Frame number successfully fetched.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to hc_nbr.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Curent frame number processed by Host Controller, If success.
*               0,                                                Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_HC_FrameNbrGet (CPU_INT08U   hc_nbr,
                                 USBH_ERR    *p_err)
{
    CPU_INT32U   frame_nbr;
    USBH_HC     *p_hc;


    if (hc_nbr >= USBH_Host.HC_NbrNext) {                       /* Chk if HC nbr is valid.                              */
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    p_hc = &USBH_Host.HC_Tbl[hc_nbr];

    USBH_HCD_FrmNbrGet(p_hc, &frame_nbr, p_err);

    return (frame_nbr);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                    DEVICE ENUMERATION FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           USBH_DevConn()
*
* Description : Enumerates newly connected USB device. Reads device and configuration descriptor from
*               device and loads appropriate class driver(s).
*
* Argument(s) : p_dev       Pointer to USB device structure.
*
* Return(s)   : USBH_ERR_NONE                           If device connection is successful.
*               USBH_ERR_DESC_INVALID                   If device contains 0 configurations
*               USBH_ERR_CFG_ALLOC                      If maximum number of configurations reached.
*
*                                                       ----- RETURNED BY USBH_CfgRd() : -----
*               USBH_ERR_DESC_INVALID,                  If invalid configuration descriptor was fetched.
*               USBH_ERR_CFG_MAX_CFG_LEN,               If configuration descriptor length > USBH_CFG_MAX_CFG_DATA_LEN
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_CDC_RespRx() : -----
*               USBH_ERR_DESC_INVALID,                  If an invalid device descriptor was fetched.
*               USBH_ERR_DEV_NOT_RESPONDING,            If device is not responding.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_DevAddrSet() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*
*                                                       ----- RETURNED BY USBH_ClassDrvConn() : -----
*               USBH_ERR_DRIVER_NOT_FOUND               If no class driver was found.
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_DfltEP_Open() : -----
*               USBH_ERR_OS_SIGNAL_CREATE,              If semaphore or mutex creation failed.
*               Host controller driver error,           Otherwise.
*
*                                                       -------- RETURNED BY USBH_CfgRd() : --------
*               USBH_ERR_NULL_PTR                       If configuration read returns a null pointer.
*               Host controller driver error,           Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_DevConn (USBH_DEV  *p_dev)
{
    USBH_ERR     err;
    CPU_INT08U   nbr_cfgs;
    CPU_INT08U   cfg_ix;


    p_dev->SelCfg = 0u;

    p_dev->ClassDrvRegPtr = (USBH_CLASS_DRV_REG *)0;
    Mem_Clr(p_dev->DevDesc, USBH_LEN_DESC_DEV);

    err = USBH_DfltEP_Open(p_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_DevDescRd(p_dev);                                /* ------------------- RD DEV DESC -------------------- */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_DevAddrSet(p_dev);                               /* -------------- ASSIGN NEW ADDR TO DEV -------------- */
    if (err != USBH_ERR_NONE) {
        return (err);
    }
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
    USBH_PRINT_LOG("Port %d: Device Address: %d.\r\n",
                    p_dev->PortNbr,
                    p_dev->DevAddr);
#endif

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                                                                /* -------- PRINT MANUFACTURER AND PRODUCT STR -------- */
    if(p_dev->DevDesc[14] != 0u) {                              /* iManufacturer = 0 -> no str desc for manufacturer.   */
        USBH_StrDescPrint(               p_dev,
                          (CPU_INT08U *)"Manufacturer : ",
                                         p_dev->DevDesc[14]);
    }

    if(p_dev->DevDesc[15] != 0u) {                              /* iProduct = 0 -> no str desc for product.             */
        USBH_StrDescPrint(               p_dev,
                          (CPU_INT08U *)"Product      : ",
                                         p_dev->DevDesc[15]);
    }
#endif

    nbr_cfgs = USBH_DevCfgNbrGet(p_dev);                        /* ---------- GET NBR OF CFG PRESENT IN DEV ----------- */
    if (nbr_cfgs == 0u) {
        return (USBH_ERR_DESC_INVALID);
    } else if (nbr_cfgs > USBH_CFG_MAX_NBR_CFGS) {
        return (USBH_ERR_CFG_ALLOC);
    } else {
                                                                /* Empty Else Statement                                 */
    }

    for (cfg_ix = 0u; cfg_ix < nbr_cfgs; cfg_ix++) {            /* -------------------- RD ALL CFG -------------------- */
        err = USBH_CfgRd(p_dev, cfg_ix);
        if (err != USBH_ERR_NONE) {
            return (err);
        }
    }

    err = USBH_ClassDrvConn(p_dev);                             /* ------------- PROBE/LOAD CLASS DRV(S) -------------- */

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_DevDisconn()
*
* Description : Unload class drivers & close default endpoint.
*
* Argument(s) : p_dev       Pointer to USB device structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_DevDisconn (USBH_DEV  *p_dev)
{
    USBH_ClassDrvDisconn(p_dev);                                /* Unload class drv(s).                                 */

    USBH_EP_Close(&p_dev->DfltEP);                              /* Close dflt EPs.                                      */
}


/*
*********************************************************************************************************
*                                          USBH_DevCfgNbrGet()
*
* Description : Get number of configurations supported by specified device.
*
* Argument(s) : p_dev       Pointer to USB device.
*
* Return(s)   : Number of configurations.
*
* Note(s)     : (1) USB2.0 spec, section 9.6.1 states that offset 17 of standard device descriptor
*                   contains number of configurations.
*********************************************************************************************************
*/

CPU_INT08U  USBH_DevCfgNbrGet (USBH_DEV  *p_dev)
{
    return (p_dev->DevDesc[17]);                                /* See Note (1).                                        */
}


/*
*********************************************************************************************************
*                                          USBH_DevDescGet()
*
* Description : Get device descriptor of specified USB device.
*
* Argument(s) : p_dev            Pointer to USB device.
*
*               p_dev_desc       Pointer to device descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_DevDescGet (USBH_DEV       *p_dev,
                       USBH_DEV_DESC  *p_dev_desc)
{
    USBH_ParseDevDesc(        p_dev_desc,
                      (void *)p_dev->DevDesc);
}


/*
*********************************************************************************************************
*                                             USBH_CfgSet()
*
* Description : Select a configration in specified device.
*
* Argument(s) : p_dev       Pointer to USB device
*
*               cfg_nbr     Configuration number to be selected
*
* Return(s)   : USBH_ERR_NONE,                          If given configuration was successfully selected.
*
*                                                       ----- RETURNED BY USBH_SET_CFG() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) (a) The SET_CONFIGURATION request is described in "Universal Serial Bus Specification
*                       Revision 2.0", section 9.4.6.
*
*                   (b) If the device is in the default state, the behaviour of the device is not
*                       specified. Implementations stall such request.
*
*                   (c) If the device is in the addressed state &
*
*                       (1) ... the configuration number is zero, then the device remains in the address state.
*                       (2) ... the configuration number is non-zero & matches a valid configuration
*                               number, then that configuration is selected & the device enters the
*                               configured state.
*                       (3) ... the configuration number is non-zero & does NOT match a valid configuration
*                               number, then the device responds with a request error.
*
*                   (d) If the device is in the configured state &
*
*                       (1) ... the configuration number is zero, then the device enters the address state.
*                       (2) ... the configuration number is non-zero & matches a valid configuration
*                               number, then that configuration is selected & the device remains in the
*                               configured state.
*                       (3) ... the configuration number is non-zero & does NOT match a valid configuration
*                               number, then the device responds with a request error.
*********************************************************************************************************
*/

USBH_ERR  USBH_CfgSet (USBH_DEV    *p_dev,
                       CPU_INT08U   cfg_nbr)
{
    USBH_ERR  err;


    USBH_SET_CFG(p_dev, cfg_nbr, &err);                         /* See Note (1).                                        */

    if (err == USBH_ERR_NONE) {
        p_dev->SelCfg = cfg_nbr;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                            USBH_CfgGet()
*
* Description : Get a pointer to specified configuration data of specified device.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               cfg_ix      Zero based index of configuration.
*
* Return(s)   : Pointer to configuration,  If configuration number is valid.
*               0,                         Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_CFG  *USBH_CfgGet (USBH_DEV    *p_dev,
                        CPU_INT08U   cfg_ix)
{
    CPU_INT08U  nbr_cfgs;


    if (p_dev == (USBH_DEV *)0) {
        return ((USBH_CFG *)0);
    }

    nbr_cfgs = USBH_DevCfgNbrGet(p_dev);                        /* Get nbr of cfg(s) present in dev.                    */
    if ((cfg_ix   >= nbr_cfgs) ||
        (nbr_cfgs == 0u)) {
        return ((USBH_CFG *)0);
    }

    return (&p_dev->CfgList[cfg_ix]);                           /* Get cfg struct.                                      */
}


/*
*********************************************************************************************************
*                                          USBH_CfgIF_NbrGet()
*
* Description : Get number of interfaces in given configuration.
*
* Argument(s) : p_cfg       Pointer to configuration
*
* Return(s)   : Number of interfaces.
*
* Note(s)     : (1) USB2.0 spec, section 9.6.1 states that offset 4 of standard configuration descriptor
*                   represents number of interfaces in the configuration.
*********************************************************************************************************
*/

CPU_INT08U  USBH_CfgIF_NbrGet (USBH_CFG  *p_cfg)
{
    if (p_cfg != (USBH_CFG *)0) {
        return (p_cfg->CfgData[4]);                             /* See Note (1).                                        */
    } else {
        return (0u);
    }
}


/*
*********************************************************************************************************
*                                          USBH_CfgDescGet()
*
* Description : Get configuration descriptor data.
*
* Argument(s) : p_cfg           Pointer to USB configuration
*
*               p_cfg_desc      Pointer to a variable that will contain configuration descriptor.
*
* Return(s)   : USBH_ERR_NONE            If a valid configuration descriptor is found.
*               USBH_ERR_INVALID_ARG     If invalid argument passed to 'p_cfg' / 'p_cfg_desc'.
*               USBH_ERR_DESC_INVALID    If an invalid configuration descriptor was found.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CfgDescGet (USBH_CFG       *p_cfg,
                           USBH_CFG_DESC  *p_cfg_desc)
{
    USBH_DESC_HDR  *p_desc;


    if ((p_cfg      == (USBH_CFG      *)0) ||
        (p_cfg_desc == (USBH_CFG_DESC *)0)) {
        return (USBH_ERR_INVALID_ARG);
    }

    p_desc = (USBH_DESC_HDR *)p_cfg->CfgData;                   /* Check for valid cfg desc.                            */

    if ((p_desc->bLength         == USBH_LEN_DESC_CFG ) &&
        (p_desc->bDescriptorType == USBH_DESC_TYPE_CFG)) {
        USBH_ParseCfgDesc(        p_cfg_desc,
                          (void *)p_desc);

        return (USBH_ERR_NONE);
    } else {
        return (USBH_ERR_DESC_INVALID);
    }
}


/*
*********************************************************************************************************
*                                       USBH_CfgExtraDescGet()
*
* Description : Get extra descriptor immediately following configuration descriptor.
*
* Argument(s) : p_cfg       Pointer to USB configuration.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE,                   If a valid extra descriptor is present.
*                       USBH_ERR_INVALID_ARG,            If invalid argument passed to 'p_cfg'.
*                       USBH_ERR_DESC_EXTRA_NOT_FOUND,   If extra descriptor is not present.
*
* Return(s)   : Pointer to extra descriptor,  If a valid extra descriptor is present.
*               0,                            Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_DESC_HDR  *USBH_CfgExtraDescGet(USBH_CFG  *p_cfg,
                                     USBH_ERR  *p_err)
{
    USBH_DESC_HDR  *p_desc;
    USBH_DESC_HDR  *p_extra_desc;
    CPU_INT32U      cfg_off;


    if (p_cfg == (USBH_CFG *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return ((USBH_DESC_HDR *)0);
    }

    p_desc = (USBH_DESC_HDR *)p_cfg->CfgData;                   /* Get config desc data.                                */

    if ((p_desc->bLength         == USBH_LEN_DESC_CFG    ) &&
        (p_desc->bDescriptorType == USBH_DESC_TYPE_CFG) &&
        (p_cfg->CfgDataLen       >  (p_desc->bLength + 2u))) {

        cfg_off      = p_desc->bLength;
        p_extra_desc = USBH_NextDescGet(p_desc, &cfg_off);      /* Get desc that follows config desc.                   */

                                                                /* No extra desc present.                               */
        if (p_extra_desc->bDescriptorType != USBH_DESC_TYPE_IF) {
           *p_err = USBH_ERR_NONE;
            return (p_extra_desc);
        }
    }

   *p_err = USBH_ERR_DESC_EXTRA_NOT_FOUND;

    return ((USBH_DESC_HDR *)0);
}


/*
*********************************************************************************************************
*                                            USBH_IF_Set()
*
* Description : Select specified alternate setting of interface.
*
* Argument(s) : p_if        Pointer to interface.
*
*               alt_nbr     Alternate setting number to select.
*
* Return(s)   : USBH_ERR_NONE,                          If specified alternate setting was successfully selected.
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'p_if' / 'alt_nbr'.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The standard SET_INTERFACE request is defined in the Universal Serial Bus
*                   specification revision 2.0, section 9.4.10
*********************************************************************************************************
*/

USBH_ERR  USBH_IF_Set (USBH_IF     *p_if,
                       CPU_INT08U   alt_nbr)
{
    CPU_INT08U   nbr_alts;
    CPU_INT08U   if_nbr;
    USBH_DEV    *p_dev;
    USBH_ERR     err;


    if (p_if == (USBH_IF *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    nbr_alts = USBH_IF_AltNbrGet(p_if);                         /* Get nbr of alternate settings in IF.                 */
    if (alt_nbr >= nbr_alts) {
        return (USBH_ERR_INVALID_ARG);
    }

    if_nbr = USBH_IF_NbrGet(p_if);                              /* Get IF nbr.                                          */
    p_dev  = p_if->DevPtr;

    USBH_SET_IF(p_dev, if_nbr, alt_nbr, &err);                  /* See Note (1).                                        */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    p_if->AltIxSel = alt_nbr;                                   /* Update selected alternate setting.                   */

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_IF_Get()
*
* Description : Get specified interface from given configuration.
*
* Argument(s) : p_cfg       Pointer to configuration.
*
*               if_ix       Zero based index of the Interface.
*
* Return(s)   : Pointer to interface data,  If interface number is valid.
*               0,                          Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_IF  *USBH_IF_Get (USBH_CFG    *p_cfg,
                       CPU_INT08U   if_ix)
{
    CPU_INT08U  nbr_ifs;


    nbr_ifs = USBH_CfgIF_NbrGet(p_cfg);                         /* Get nbr of IFs.                                      */

    if ((if_ix < nbr_ifs) && (if_ix < USBH_CFG_MAX_NBR_IFS))  {
        return (&p_cfg->IF_List[if_ix]);                        /* Return IF structure at selected ix.                  */
    } else {
        return ((USBH_IF *)0);
    }
}


/*
*********************************************************************************************************
*                                         USBH_IF_AltNbrGet()
*
* Description : Get number of alternate settings supported by the given interface.
*
* Argument(s) : p_if        Pointer to interface.
*
* Return(s)   : Number of alternate settings.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT08U  USBH_IF_AltNbrGet (USBH_IF  *p_if)
{
    USBH_DESC_HDR  *p_desc;
    CPU_INT32U      if_off;
    CPU_INT08U      nbr_alts;


    nbr_alts =  0u;
    if_off   =  0u;
    p_desc   = (USBH_DESC_HDR *)p_if->IF_DataPtr;

    while (if_off < p_if->IF_DataLen) {                         /* Cnt nbr of alternate settings.                       */
        p_desc = USBH_NextDescGet((void *)p_desc, &if_off);

        if (p_desc->bDescriptorType == USBH_DESC_TYPE_IF) {
            nbr_alts++;
        }
    }

    return (nbr_alts);
}


/*
*********************************************************************************************************
*                                           USBH_IF_NbrGet()
*
* Description : Get number of given interface.
*
* Argument(s) : p_if        Pointer to interface.
*
* Return(s)   : Interface number.
*
* Note(s)     : (1) USB2.0 spec, section 9.6.5 states that offset 2 (bInterfaceNumber) of standard
*                   interface descriptor contains the interface number of this interface.
*********************************************************************************************************
*/

CPU_INT08U  USBH_IF_NbrGet (USBH_IF  *p_if)
{
    return (p_if->IF_DataPtr[2]);                               /* See Note (1)                                         */
}


/*
*********************************************************************************************************
*                                         USBH_IF_EP_NbrGet()
*
* Description : Determine number of endpoints in given alternate setting of interface.
*
* Argument(s) : p_if      Pointer to interface.
*
*               alt_ix    Alternate setting index.
*
* Return(s)   : Number of endpoints.
*
* Note(s)     : (1) USB2.0 spec, section 9.6.5 states that offset 4 of standard interface descriptor
*                   (bNumEndpoints) contains the number of endpoints in this interface descriptor.
*********************************************************************************************************
*/

CPU_INT08U  USBH_IF_EP_NbrGet (USBH_IF     *p_if,
                               CPU_INT08U   alt_ix)
{
    USBH_DESC_HDR  *p_desc;
    CPU_INT32U      if_off;


    if_off =  0u;
    p_desc = (USBH_DESC_HDR *)p_if->IF_DataPtr;

    while (if_off < p_if->IF_DataLen) {
        p_desc = USBH_NextDescGet((void *)p_desc, &if_off);
                                                                /* IF desc.                                             */
        if (p_desc->bDescriptorType == USBH_DESC_TYPE_IF) {

            if (alt_ix == ((CPU_INT08U *)p_desc)[3]) {          /* Chk alternate setting.                               */
                return (((CPU_INT08U *)p_desc)[4]);             /* IF desc offset 4 contains nbr of EPs.                */
            }
        }
    }

    return (0u);
}


/*
*********************************************************************************************************
*                                          USBH_IF_DescGet()
*
* Description : Get descriptor of interface at specified alternate setting index.
*
* Argument(s) : p_if        Pointer to USB interface.
*
*               alt_ix      Alternate setting index.
*
*               p_if_desc   Pointer to a variable to hold the interface descriptor data.
*
* Return(s)   : USBH_ERR_NONE          If interface descritor was found.
*               USBH_ERR_INVALID_ARG   Invalid argument passed to 'alt_ix'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_IF_DescGet (USBH_IF       *p_if,
                           CPU_INT08U     alt_ix,
                           USBH_IF_DESC  *p_if_desc)
{
    USBH_DESC_HDR  *p_desc;
    CPU_INT32U      if_off;


    if_off = 0u;
    p_desc = (USBH_DESC_HDR *)p_if->IF_DataPtr;

    while (if_off < p_if->IF_DataLen) {
        p_desc = USBH_NextDescGet((void *)p_desc, &if_off);

        if ((p_desc->bLength         ==  USBH_LEN_DESC_IF ) &&
            (p_desc->bDescriptorType ==  USBH_DESC_TYPE_IF) &&
            (alt_ix                  == ((CPU_INT08U *)p_desc)[3])) {

            USBH_ParseIF_Desc(p_if_desc, (void *)p_desc);
            return (USBH_ERR_NONE);
        }
    }

    return (USBH_ERR_INVALID_ARG);
}


/*
*********************************************************************************************************
*                                       USBH_IF_ExtraDescGet()
*
* Description : Get the descriptor immediately following the interface descriptor.
*
* Argument(s) : p_if            Pointer to USB interface.
*
*               alt_ix          Alternate setting number.
*
*               p_data_len      Length of extra interface descriptor.
*
* Return(s)   : Pointer to extra descriptor,  If success.
*               0,                            Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT08U  *USBH_IF_ExtraDescGet (USBH_IF     *p_if,
                                   CPU_INT08U   alt_ix,
                                   CPU_INT16U  *p_data_len)
{
    USBH_DESC_HDR  *p_desc;
    CPU_INT08U     *p_data;
    CPU_INT32U      if_off;


    if ((p_if             == (USBH_IF    *)0) ||
        (p_if->IF_DataPtr == (CPU_INT08U *)0)) {
        return ((CPU_INT08U *)0);
    }

    if_off = 0u;
    p_desc = (USBH_DESC_HDR *)p_if->IF_DataPtr;

    while (if_off < p_if->IF_DataLen) {
        p_desc = USBH_NextDescGet((void* )p_desc, &if_off);     /* Get next desc from IF.                               */

        if ((p_desc->bLength         ==  USBH_LEN_DESC_IF ) &&
            (p_desc->bDescriptorType ==  USBH_DESC_TYPE_IF) &&
            (alt_ix                  == ((CPU_INT08U *)p_desc)[3])) {

            if (if_off < p_if->IF_DataLen) {                    /* Get desc that follows selected alternate setting.    */
                p_desc     = USBH_NextDescGet((void *)p_desc, &if_off);
                p_data     = (CPU_INT08U  *)p_desc;
               *p_data_len = 0u;

                while ((p_desc->bDescriptorType != USBH_DESC_TYPE_IF) &&
                       (p_desc->bDescriptorType != USBH_DESC_TYPE_EP)) {

                   *p_data_len += p_desc->bLength;
                    p_desc = USBH_NextDescGet((void* )p_desc,   /* Get next desc from IF.                               */
                                                     &if_off);
                    if (if_off >= p_if->IF_DataLen) {
                        break;
                    }
                }

                if (*p_data_len == 0) {
                    return ((CPU_INT08U *)0);
                } else {
                    return ((CPU_INT08U *)p_data);
                }
            }
        }
    }

    return ((CPU_INT08U *)0);
}


/*
*********************************************************************************************************
*                                          USBH_BulkInOpen()
*
* Description : Open a bulk IN endpoint.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to USB interface.
*
*               p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE                       If the bulk IN endpoint is opened successfully.
*
*                                                   ----- RETURNED BY USBH_EP_Open() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_BulkInOpen (USBH_DEV  *p_dev,
                           USBH_IF   *p_if,
                           USBH_EP   *p_ep)
{
    USBH_ERR  err;


    err = USBH_EP_Open(p_dev,
                       p_if,
                       USBH_EP_TYPE_BULK,
                       USBH_EP_DIR_IN,
                       p_ep);

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_BulkOutOpen()
*
* Description : Open a bulk OUT endpoint.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to USB interface.
*
*               p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE                       If the bulk OUT endpoint is opened successfully.
*
*                                                   ----- RETURNED BY USBH_EP_Open() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_BulkOutOpen (USBH_DEV  *p_dev,
                            USBH_IF   *p_if,
                            USBH_EP   *p_ep)
{
    USBH_ERR  err;


    err = USBH_EP_Open(p_dev,
                       p_if,
                       USBH_EP_TYPE_BULK,
                       USBH_EP_DIR_OUT,
                       p_ep);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_IntrInOpen()
*
* Description : Open an interrupt IN endpoint.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to USB interface.
*
*               p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE                       If the interrupt IN endpoint is opened successfully.
*
*                                                   ----- RETURNED BY USBH_EP_Open() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_IntrInOpen (USBH_DEV  *p_dev,
                           USBH_IF   *p_if,
                           USBH_EP   *p_ep)
{
    USBH_ERR  err;


    err = USBH_EP_Open(p_dev,
                       p_if,
                       USBH_EP_TYPE_INTR,
                       USBH_EP_DIR_IN,
                       p_ep);

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_IntrOutOpen()
*
* Description : Open and interrupt OUT endpoint.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to USB interface.
*
*               p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE                       If the interrupt OUT endpoint is opened successfully.
*
*                                                   ----- RETURNED BY USBH_EP_Open() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_IntrOutOpen (USBH_DEV  *p_dev,
                            USBH_IF   *p_if,
                            USBH_EP   *p_ep)
{
    USBH_ERR  err;


    err = USBH_EP_Open(p_dev,
                       p_if,
                       USBH_EP_TYPE_INTR,
                       USBH_EP_DIR_OUT,
                       p_ep);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_IsocInOpen()
*
* Description : Open an isochronous IN endpoint.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to USB interface.
*
*               p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE                       If the isochronous IN endpoint is opened successfully.
*
*                                                   ----- RETURNED BY USBH_EP_Open() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_IsocInOpen (USBH_DEV  *p_dev,
                           USBH_IF   *p_if,
                           USBH_EP   *p_ep)
{
    USBH_ERR  err;


    err = USBH_EP_Open(p_dev,
                       p_if,
                       USBH_EP_TYPE_ISOC,
                       USBH_EP_DIR_IN,
                       p_ep);

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_IsocOutOpen()
*
* Description : Open an isochronous OUT endpoint.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to USB interface.
*
*               p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE                       If the isochronous OUT endpoint is opened successfully.
*
*                                                   ----- RETURNED BY USBH_EP_Open() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_IsocOutOpen (USBH_DEV  *p_dev,
                            USBH_IF   *p_if,
                            USBH_EP   *p_ep)
{
    USBH_ERR  err;


    err = USBH_EP_Open(p_dev,
                       p_if,
                       USBH_EP_TYPE_ISOC,
                       USBH_EP_DIR_OUT,
                       p_ep);

    return (err);
}


/*
*********************************************************************************************************
*                                            USBH_CtrlTx()
*
* Description : Issue control request to device and send data to it.
*
* Argument(s) : p_dev            Pointer to USB device.
*
*               b_req            One-byte value that specifies request.
*
*               bm_req_type      One-byte value that specifies direction, type and the recipient of
*                                request.
*
*               w_val            Two-byte value used to pass information to device.
*
*               w_ix             Two-byte value used to pass an index or offset (such as interface or
*                                endpoint number) to device.
*
*               p_data           Pointer to data buffer to transmit.
*
*               w_len            Two-byte value containing buffer length in octets.
*
*               timeout_ms       Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*                           USBH_ERR_NONE                           Control transfer successfully transmitted.
*
*                                                                   ----- RETURNED BY USBH_SyncCtrlXfer() : -----
*                           USBH_ERR_NONE                           Control transfer successfully completed.
*                           USBH_ERR_UNKNOWN                        Unknown error occurred.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_HUB_RHCtrlReq() : -----
*                           USBH_ERR_HC_IO,                         Root hub input/output error.
*                           USBH_ERR_EP_STALL,                      Root hub does not support request.
*
* Return(s)   : Number of octets transmitted.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16U  USBH_CtrlTx (USBH_DEV    *p_dev,
                         CPU_INT08U   b_req,
                         CPU_INT08U   bm_req_type,
                         CPU_INT16U   w_val,
                         CPU_INT16U   w_ix,
                         void        *p_data,
                         CPU_INT16U   w_len,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT16U  xfer_len;


    (void)USBH_OS_MutexLock(p_dev->DfltEP_Mutex);

    if ((p_dev->IsRootHub            == DEF_TRUE) &&            /* Check if RH features are supported.                  */
        (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE)) {
        xfer_len = USBH_HUB_RH_CtrlReq(p_dev->HC_Ptr,           /* Send req to virtual HUB.                             */
                                       b_req,
                                       bm_req_type,
                                       w_val,
                                       w_ix,
                                       p_data,
                                       w_len,
                                       p_err);
    } else {
        xfer_len = USBH_SyncCtrlXfer(&p_dev->DfltEP,
                                      b_req,
                                      bm_req_type,
                                      w_val,
                                      w_ix,
                                      p_data,
                                      w_len,
                                      timeout_ms,
                                      p_err);
    }

    (void)USBH_OS_MutexUnlock(p_dev->DfltEP_Mutex);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                            USBH_CtrlRx()
*
* Description : Issue control request to device and receive data from it.
*
* Argument(s) : p_dev            Pointer to USB device.
*
*               b_req            One-byte value that specifies request.
*
*               bm_req_type      One-byte value that specifies direction, type and the recipient of
*                                request.
*
*               w_val            Two-byte value used to pass information to device.
*
*               w_ix             Two-byte value used to pass an index or offset (such as interface or
*                                endpoint number) to device.
*
*               p_data           Pointer to destination buffer to receive data.
*
*               w_len            Two-byte value containing buffer length in octets.
*
*               timeout_ms       Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Control transfer successfully received.
*
*                                                                   ----- RETURNED BY USBH_SyncCtrlXfer() : -----
*                           USBH_ERR_NONE                           Control transfer successfully completed.
*                           USBH_ERR_UNKNOWN                        Unknown error occurred.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_HUB_RHCtrlReq() : -----
*                           USBH_ERR_HC_IO,                         Root hub input/output error.
*                           USBH_ERR_EP_STALL,                      Root hub does not support request.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT16U  USBH_CtrlRx (USBH_DEV    *p_dev,
                         CPU_INT08U   b_req,
                         CPU_INT08U   bm_req_type,
                         CPU_INT16U   w_val,
                         CPU_INT16U   w_ix,
                         void        *p_data,
                         CPU_INT16U   w_len,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT16U  xfer_len;


    (void)USBH_OS_MutexLock(p_dev->DfltEP_Mutex);

    if ((p_dev->IsRootHub            == DEF_TRUE) &&            /* Check if RH features are supported.                  */
        (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE)) {
        xfer_len = USBH_HUB_RH_CtrlReq(p_dev->HC_Ptr,           /* Send req to virtual HUB.                             */
                                       b_req,
                                       bm_req_type,
                                       w_val,
                                       w_ix,
                                       p_data,
                                       w_len,
                                       p_err);
    } else {
        xfer_len = USBH_SyncCtrlXfer(&p_dev->DfltEP,
                                      b_req,
                                      bm_req_type,
                                      w_val,
                                      w_ix,
                                      p_data,
                                      w_len,
                                      timeout_ms,
                                      p_err);
    }

    (void)USBH_OS_MutexUnlock(p_dev->DfltEP_Mutex);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                             USBH_BulkTx()
*
* Description : Issue bulk request to transmit data to device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer to transmit.
*
*               buf_len         Buffer length in octets.
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Bulk transfer successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not OUT.
*
*                                                                   ----- RETURNED BY USBH_SyncXfer() : -----
*                           USBH_ERR_NONE                           Transfer successfully completed.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets transmitted.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_BulkTx (USBH_EP     *p_ep,
                         void        *p_buf,
                         CPU_INT32U   buf_len,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT08U  ep_type;
    CPU_INT08U  ep_dir;
    CPU_INT32U  xfer_len;


    if (p_ep == (USBH_EP *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_BULK) ||
        (ep_dir  != USBH_EP_DIR_OUT  )) {
       *p_err = USBH_ERR_EP_INVALID_TYPE;
        return (0u);
    }

    xfer_len = USBH_SyncXfer(                  p_ep,
                                               p_buf,
                                               buf_len,
                             (USBH_ISOC_DESC *)0,
                                               USBH_TOKEN_OUT,
                                               timeout_ms,
                                               p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_BulkTxAsync()
*
* Description : Issue asynchronous bulk request to transmit data to device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               buf             Pointer to data buffer to transmit.
*
*               buf_len         Buffer length in octets.
*
*               p_fnct          Function that will be invoked upon completion of transmit operation.
*
*               p_fnct_arg      Pointer to argument that will be passed as parameter of 'p_fnct'.
*
* Return(s)   : USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not bulk or direction is not OUT.
*
*                                                       ----- RETURNED BY USBH_AsyncXfer() : -----
*               USBH_ERR_NONE                           If transfer successfully submitted.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This function returns immediately. The URB completion will be invoked by the
*                   asynchronous I/O task when transfer is completed.
*********************************************************************************************************
*/

USBH_ERR  USBH_BulkTxAsync (USBH_EP              *p_ep,
                            void                 *p_buf,
                            CPU_INT32U            buf_len,
                            USBH_XFER_CMPL_FNCT   fnct,
                            void                 *p_fnct_arg)
{
    USBH_ERR    err;
    CPU_INT08U  ep_type;
    CPU_INT08U  ep_dir;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_BULK) ||
        (ep_dir  != USBH_EP_DIR_OUT  )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    err = USBH_AsyncXfer(                  p_ep,
                                           p_buf,
                                           buf_len,
                         (USBH_ISOC_DESC *)0,
                                           USBH_TOKEN_OUT,
                         (void           *)fnct,
                                           p_fnct_arg);

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_BulkRx()
*
* Description : Issue bulk request to receive data from device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to destination buffer to receive data.
*
*               buf_len         Buffer length in octets.
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Bulk transfer successfully received.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*
*                                                                   ----- RETURNED BY USBH_SyncXfer() : -----
*                           USBH_ERR_NONE                           Transfer successfully completed.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_BulkRx (USBH_EP     *p_ep,
                         void        *p_buf,
                         CPU_INT32U   buf_len,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT08U  ep_type;
    CPU_INT08U  ep_dir;
    CPU_INT32U  xfer_len;


    if (p_ep == (USBH_EP *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_BULK) ||
        (ep_dir  != USBH_EP_DIR_IN   )) {
       *p_err = USBH_ERR_EP_INVALID_TYPE;
        return (0u);
    }

    xfer_len = USBH_SyncXfer(                  p_ep,
                                               p_buf,
                                               buf_len,
                             (USBH_ISOC_DESC *)0,
                                               USBH_TOKEN_IN,
                                               timeout_ms,
                                               p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_BulkRxAsync()
*
* Description : Issue asynchronous bulk request to receive data from device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to destination buffer to receive data.
*
*               buf_len         Buffer length in octets.
*
*               fnct            Function that will be invoked upon completion of receive operation.
*
*               p_fnct_arg      Pointer to argument that will be passed as parameter of 'p_fnct'.
*
* Return(s)   : USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not Bulk or direction is not IN.
*
*                                                       ----- RETURNED BY USBH_AsyncXfer() : -----
*               USBH_ERR_NONE                           If transfer successfully submitted.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This function returns immediately. The URB completion will be invoked by the
*                   asynchronous I/O task when the transfer is completed.
*********************************************************************************************************
*/

USBH_ERR  USBH_BulkRxAsync (USBH_EP              *p_ep,
                            void                 *p_buf,
                            CPU_INT32U            buf_len,
                            USBH_XFER_CMPL_FNCT   fnct,
                            void                 *p_fnct_arg)
{
    USBH_ERR     err;
    CPU_INT08U   ep_type;
    CPU_INT08U   ep_dir;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_BULK) ||
        (ep_dir  != USBH_EP_DIR_IN   )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    err = USBH_AsyncXfer(                  p_ep,
                                           p_buf,
                                           buf_len,
                         (USBH_ISOC_DESC *)0,
                                           USBH_TOKEN_IN,
                         (void           *)fnct,
                                           p_fnct_arg);

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_IntrTx()
*
* Description : Issue interrupt request to transmit data to device
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer to transmit.
*
*               buf_len         Buffer length in octets.
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Interrupt transfer successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not interrupt or direction is not OUT.
*
*                                                                   ----- RETURNED BY USBH_SyncXfer() : -----
*                           USBH_ERR_NONE                           Transfer successfully completed.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets transmitted.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_IntrTx (USBH_EP     *p_ep,
                         void        *p_buf,
                         CPU_INT32U   buf_len,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT08U  ep_type;
    CPU_INT08U  ep_dir;
    CPU_INT32U  xfer_len;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_INTR) ||
        (ep_dir  != USBH_EP_DIR_OUT  )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    xfer_len = USBH_SyncXfer(                  p_ep,
                                               p_buf,
                                               buf_len,
                             (USBH_ISOC_DESC *)0,
                                               USBH_TOKEN_OUT,
                                               timeout_ms,
                                               p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_IntrTxAsync()
*
* Description : Issue asynchronous interrupt request to transmit data to device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer to transmit.
*
*               buf_len         Buffer length in octets.
*
*               fnct            Function that will be invoked upon completion of transmit operation.
*
*               p_fnct_arg      Pointer to argument that will be passed as parameter of 'p_fnct'.
*
* Return(s)   : USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not interrupt or direction is not OUT.
*
*                                                       ----- RETURNED BY USBH_AsyncXfer() : -----
*               USBH_ERR_NONE                           If transfer successfully submitted.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This function returns immediately. The URB completion will be invoked by the
*                   asynchronous I/O task when the transfer is completed.
*********************************************************************************************************
*/

USBH_ERR  USBH_IntrTxAsync (USBH_EP              *p_ep,
                            void                 *p_buf,
                            CPU_INT32U            buf_len,
                            USBH_XFER_CMPL_FNCT   fnct,
                            void                 *p_fnct_arg)
{
    USBH_ERR     err;
    CPU_INT08U   ep_type;
    CPU_INT08U   ep_dir;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_INTR) ||
        (ep_dir  != USBH_EP_DIR_OUT  )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    err = USBH_AsyncXfer(                  p_ep,
                                           p_buf,
                                           buf_len,
                         (USBH_ISOC_DESC *)0,
                                           USBH_TOKEN_OUT,
                         (void           *)fnct,
                                           p_fnct_arg);

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_IntrRx()
*
* Description : Issue interrupt request to receive data from device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to destination buffer to receive data.
*
*               buf_len         Buffer length in octets.
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Interrupt transfer successfully received.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not interrupt or direction is not IN.
*
*                                                                   ----- RETURNED BY USBH_SyncXfer() : -----
*                           USBH_ERR_NONE                           Transfer successfully completed.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_IntrRx (USBH_EP     *p_ep,
                         void        *p_buf,
                         CPU_INT32U   buf_len,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT08U  ep_type;
    CPU_INT08U  ep_dir;
    CPU_INT32U  xfer_len;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_INTR) ||
        (ep_dir  != USBH_EP_DIR_IN   )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    xfer_len = USBH_SyncXfer(                  p_ep,
                                               p_buf,
                                               buf_len,
                             (USBH_ISOC_DESC *)0,
                                               USBH_TOKEN_IN,
                                               timeout_ms,
                                               p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_IntrRxAsync()
*
* Description : Issue asynchronous interrupt request to receive data from device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer to transmit.
*
*               buf_len         Buffer length in octets.
*
*               fnct            Function that will be invoked upon completion of receive operation.
*
*               p_fnct_arg      Pointer to argument that will be passed as parameter of 'p_fnct'.
*
* Return(s)   : USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not interrupt or direction is not IN.
*
*                                                       ----- RETURNED BY USBH_AsyncXfer() : -----
*               USBH_ERR_NONE                           If transfer successfully submitted.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This function returns immediately. The URB completion will be invoked by the
*                   asynchronous I/O task when the transfer is completed.
*********************************************************************************************************
*/

USBH_ERR  USBH_IntrRxAsync (USBH_EP              *p_ep,
                            void                 *p_buf,
                            CPU_INT32U            buf_len,
                            USBH_XFER_CMPL_FNCT   fnct,
                            void                 *p_fnct_arg)
{
    USBH_ERR    err;
    CPU_INT08U  ep_type;
    CPU_INT08U  ep_dir;

                                                                /* Argument checks for valid settings                   */
    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    if (p_ep->IsOpen == DEF_FALSE) {
        return (USBH_ERR_EP_INVALID_STATE);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_INTR) ||
        (ep_dir  != USBH_EP_DIR_IN   )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    err = USBH_AsyncXfer(                  p_ep,
                                           p_buf,
                                           buf_len,
                         (USBH_ISOC_DESC *)0,
                                           USBH_TOKEN_IN,
                         (void           *)fnct,
                                           p_fnct_arg);

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_IsocTx()
*
* Description : Issue isochronous request to transmit data to device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer to transmit.
*
*               buf_len         Buffer length in octets.
*
*               start_frm       Transfer start frame number.
*
*               nbr_frm         Number of frames.
*
*               p_frm_len
*
*               p_frm_err
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Isochronous transfer successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not isochronous or direction is not OUT.
*
*                                                                   ----- RETURNED BY USBH_SyncXfer() : -----
*                           USBH_ERR_NONE                           Transfer successfully completed.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets transmitted.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_IsocTx (USBH_EP     *p_ep,
                         CPU_INT08U  *p_buf,
                         CPU_INT32U   buf_len,
                         CPU_INT32U   start_frm,
                         CPU_INT32U   nbr_frm,
                         CPU_INT16U  *p_frm_len,
                         USBH_ERR    *p_frm_err,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT08U      ep_type;
    CPU_INT08U      ep_dir;
    CPU_INT32U      xfer_len;
    USBH_ISOC_DESC  isoc_desc;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_ISOC) ||
        (ep_dir  != USBH_EP_DIR_OUT  )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    isoc_desc.BufPtr   = p_buf;
    isoc_desc.BufLen   = buf_len;
    isoc_desc.StartFrm = start_frm;
    isoc_desc.NbrFrm   = nbr_frm;
    isoc_desc.FrmLen   = p_frm_len;
    isoc_desc.FrmErr   = p_frm_err;

    xfer_len = USBH_SyncXfer(p_ep,
                             p_buf,
                             buf_len,
                            &isoc_desc,
                             USBH_TOKEN_OUT,
                             timeout_ms,
                             p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_IsocTxAsync()
*
* Description : Issue asynchronous isochronous request to transmit data to device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer to transmit.
*
*               buf_len         Buffer length in octets.
*
*               start_frm
*
*               nbr_frm
*
*               p_frm_len
*
*               p_frm_err
*
*               fnct            Function that will be invoked upon completion of transmit operation.
*
*               p_fnct_arg      Pointer to argument that will be passed as parameter of 'p_fnct'.
*
* Return(s)   : USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not isochronous or direction is not OUT.
*
*                                                       ----- RETURNED BY USBH_AsyncXfer() : -----
*               USBH_ERR_NONE                           If transfer successfully submitted.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This function returns immediately. The URB completion will be invoked by the
*                   asynchronous I/O task when the transfer is completed.
*********************************************************************************************************
*/

USBH_ERR  USBH_IsocTxAsync (USBH_EP              *p_ep,
                            CPU_INT08U           *p_buf,
                            CPU_INT32U            buf_len,
                            CPU_INT32U            start_frm,
                            CPU_INT32U            nbr_frm,
                            CPU_INT16U           *p_frm_len,
                            USBH_ERR             *p_frm_err,
                            USBH_ISOC_CMPL_FNCT   fnct,
                            void                 *p_fnct_arg)
{
    USBH_ERR         err;
    CPU_INT08U       ep_type;
    CPU_INT08U       ep_dir;
    LIB_ERR          err_lib;
    USBH_ISOC_DESC  *p_isoc_desc;
    MEM_POOL        *p_isoc_desc_pool;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_ISOC) ||
        (ep_dir  != USBH_EP_DIR_OUT  )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    p_isoc_desc_pool = &p_ep->DevPtr->HC_Ptr->HostPtr->IsocDescPool;
    p_isoc_desc      =  Mem_PoolBlkGet(p_isoc_desc_pool,
                                       sizeof(USBH_ISOC_DESC),
                                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_DESC_ALLOC);
    }

    p_isoc_desc->BufPtr   = p_buf;
    p_isoc_desc->BufLen   = buf_len;
    p_isoc_desc->StartFrm = start_frm;
    p_isoc_desc->NbrFrm   = nbr_frm;
    p_isoc_desc->FrmLen   = p_frm_len;
    p_isoc_desc->FrmErr   = p_frm_err;

    err = USBH_AsyncXfer(        p_ep,
                                 p_buf,
                                 buf_len,
                                 p_isoc_desc,
                                 USBH_TOKEN_IN,
                         (void *)fnct,
                                 p_fnct_arg);
    if (err != USBH_ERR_NONE) {
        Mem_PoolBlkFree(p_isoc_desc_pool,
                        p_isoc_desc,
                       &err_lib);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_IsocRx()
*
* Description : Issue isochronous request to receive data from device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to destination buffer to receive data.
*
*               buf_len         Buffer length in octets.
*
*               start_frm
*
*               nbr_frm
*
*               p_frm_len
*
*               p_frm_err
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Isochronous transfer successfully received.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not isochronous or direction is not IN.
*
*                                                                   ----- RETURNED BY USBH_SyncXfer() : -----
*                           USBH_ERR_NONE                           Transfer successfully completed.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_IsocRx (USBH_EP     *p_ep,
                         CPU_INT08U  *p_buf,
                         CPU_INT32U   buf_len,
                         CPU_INT32U   start_frm,
                         CPU_INT32U   nbr_frm,
                         CPU_INT16U  *p_frm_len,
                         USBH_ERR    *p_frm_err,
                         CPU_INT32U   timeout_ms,
                         USBH_ERR    *p_err)
{
    CPU_INT08U      ep_type;
    CPU_INT08U      ep_dir;
    CPU_INT32U      xfer_len;
    USBH_ISOC_DESC  isoc_desc;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_ISOC) ||
        (ep_dir  != USBH_EP_DIR_IN   )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    isoc_desc.BufPtr   = p_buf;
    isoc_desc.BufLen   = buf_len;
    isoc_desc.StartFrm = start_frm;
    isoc_desc.NbrFrm   = nbr_frm;
    isoc_desc.FrmLen   = p_frm_len;
    isoc_desc.FrmErr   = p_frm_err;

    xfer_len = USBH_SyncXfer(p_ep,
                             p_buf,
                             buf_len,
                            &isoc_desc,
                             USBH_TOKEN_IN,
                             timeout_ms,
                             p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_IsocRxAsync()
*
* Description : Issue asynchronous isochronous request to receive data from device.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to destination buffer to receive data.
*
*               buf_len         Buffer length in octets.
*
*               start_frm
*
*               nbr_frm
*
*               p_frm_len
*
*               p_frm_err
*
*               fnct            Function that will be invoked upon completion of receive operation.
*
*               p_fnct_arg      Pointer to argument that will be passed as parameter of 'p_fnct'.
*
* Return(s)   : USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not isochronous or direction is not IN.
*
*                                                       ----- RETURNED BY USBH_AsyncXfer() : -----
*               USBH_ERR_NONE                           If transfer successfully submitted.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This function returns immediately. The URB completion will be invoked by the
*                   asynchronous I/O task when the transfer is completed.
*********************************************************************************************************
*/

USBH_ERR  USBH_IsocRxAsync (USBH_EP              *p_ep,
                            CPU_INT08U           *p_buf,
                            CPU_INT32U            buf_len,
                            CPU_INT32U            start_frm,
                            CPU_INT32U            nbr_frm,
                            CPU_INT16U           *p_frm_len,
                            USBH_ERR             *p_frm_err,
                            USBH_ISOC_CMPL_FNCT   fnct,
                            void                 *p_fnct_arg)
{
    USBH_ERR         err;
    CPU_INT08U       ep_type;
    CPU_INT08U       ep_dir;
    USBH_ISOC_DESC  *p_isoc_desc;
    MEM_POOL        *p_isoc_desc_pool;
    LIB_ERR          err_lib;


    if (p_ep == (USBH_EP *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    ep_type = USBH_EP_TypeGet(p_ep);
    ep_dir  = USBH_EP_DirGet(p_ep);

    if ((ep_type != USBH_EP_TYPE_ISOC) ||
        (ep_dir  != USBH_EP_DIR_IN   )) {
        return (USBH_ERR_EP_INVALID_TYPE);
    }

    p_isoc_desc_pool = &p_ep->DevPtr->HC_Ptr->HostPtr->IsocDescPool;
    p_isoc_desc      =  Mem_PoolBlkGet(p_isoc_desc_pool,
                                       sizeof(USBH_ISOC_DESC),
                                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_DESC_ALLOC);
    }

    p_isoc_desc->BufPtr   = p_buf;
    p_isoc_desc->BufLen   = buf_len;
    p_isoc_desc->StartFrm = start_frm;
    p_isoc_desc->NbrFrm   = nbr_frm;
    p_isoc_desc->FrmLen   = p_frm_len;
    p_isoc_desc->FrmErr   = p_frm_err;

    err = USBH_AsyncXfer(        p_ep,
                                 p_buf,
                                 buf_len,
                                 p_isoc_desc,
                                 USBH_TOKEN_IN,
                         (void *)fnct,
                                 p_fnct_arg);
    if (err != USBH_ERR_NONE) {
        Mem_PoolBlkFree(p_isoc_desc_pool,
                        p_isoc_desc,
                       &err_lib);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_EP_LogNbrGet()
*
* Description : Get logical number of given endpoint.
*
* Argument(s) : p_ep        Pointer to endpoint
*
* Return(s)   : Logical number of given endpoint,
*
* Note(s)     : (1) USB2.0 spec, section 9.6.6 states that bits 3...0 in offset 2 of standard endpoint
*                   descriptor (bEndpointAddress) contains the endpoint logical number.
*********************************************************************************************************
*/

CPU_INT08U  USBH_EP_LogNbrGet (USBH_EP  *p_ep)
{
    return ((CPU_INT08U )p_ep->Desc.bEndpointAddress & 0x7Fu);  /* See Note (1).                                        */
}


/*
*********************************************************************************************************
*                                          USBH_EP_DirGet()
*
* Description : Get the direction of given endpoint.
*
* Argument(s) : p_ep        Pointer to endpoint structure.
*
* Return(s)   : USBH_EP_DIR_NONE,   If endpoint is of type control.
*               USBH_EP_DIR_OUT,    If endpoint direction is OUT.
*               USBH_EP_DIR_IN,     If endpoint direction is IN.
*
* Note(s)     : (1) USB2.0 spec, section 9.6.6 states that bit 7 in offset 2 of standard endpoint
*                   descriptor (bEndpointAddress) gives the direction of that endpoint.
*
*                   (a) 0 = OUT endpoint
*                   (b) 1 = IN  endpoint
*********************************************************************************************************
*/

CPU_INT08U  USBH_EP_DirGet (USBH_EP  *p_ep)
{
    CPU_INT08U  ep_type;


    ep_type = USBH_EP_TypeGet(p_ep);
    if (ep_type == USBH_EP_TYPE_CTRL) {
        return (USBH_EP_DIR_NONE);
    }

    if (((CPU_INT08U )p_ep->Desc.bEndpointAddress & 0x80u) != 0u) {   /* See Note (1).                                        */
        return (USBH_EP_DIR_IN);
    } else {
        return (USBH_EP_DIR_OUT);
    }
}


/*
*********************************************************************************************************
*                                            USBH_EP_MaxPktSizeGet()
*
* Description : Get the maximum packet size of the given endpoint
*
* Argument(s) : p_ep        Pointer to endpoint structure
*
* Return(s)   : Maximum Packet Size of the given endpoint
*
* Note(s)     : (1) USB2.0 spec, section 9.6.6 states that offset 4 of standard endpoint descriptor
*                   (wMaxPacketSize) gives the maximum packet size supported by this endpoint.
*********************************************************************************************************
*/

CPU_INT16U  USBH_EP_MaxPktSizeGet (USBH_EP  *p_ep)
{
    return ((CPU_INT16U)p_ep->Desc.wMaxPacketSize & 0x07FFu);   /* See Note (1).                                        */
}


/*
*********************************************************************************************************
*                                            USBH_EP_TypeGet()
*
* Description : Get type of the given endpoint
*
* Argument(s) : p_ep          Pointer to endpoint
*
* Return(s)   : Type of the given endpoint
*
* Note(s)     : (1) USB2.0 spec, section 9.6.6 states that bits 1:0 in offset 3 of standard endpoint
*                   descriptor (bmAttributes) gives type of the endpoint.
*
*                   (a) 00 = Control
*                   (b) 01 = Isochronous
*                   (c) 10 = Bulk
*                   (d) 11 = Interrupt
*********************************************************************************************************
*/

CPU_INT08U  USBH_EP_TypeGet (USBH_EP  *p_ep)
{
    return ((CPU_INT08U )p_ep->Desc.bmAttributes & 0x03u);      /* See Note (1).                                        */
}


/*
*********************************************************************************************************
*                                            USBH_EP_Get()
*
* Description : Get endpoint specified by given index / alternate setting / interface.
*
* Argument(s) : p_if    Pointer to interface.
*
*               alt_ix  Alternate setting index.
*
*               ep_ix   Endpoint index.
*
*               p_ep    Pointer to endpoint structure.
*
* Return(s)   : USBH_ERR_NONE,           If endpoint was found.
*               USBH_ERR_INVALID_ARG,    If invalid argument passed to 'p_if' / 'p_ep'.
*               USBH_ERR_EP_NOT_FOUND,   If endpoint was not found.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_EP_Get (USBH_IF     *p_if,
                       CPU_INT08U   alt_ix,
                       CPU_INT08U   ep_ix,
                       USBH_EP     *p_ep)
{
    USBH_DESC_HDR  *p_desc;
    CPU_INT32U      if_off;
    CPU_INT08U      ix;


    if ((p_if == (USBH_IF *)0) ||
        (p_ep == (USBH_EP *)0)) {
        return (USBH_ERR_INVALID_ARG);
    }

    ix     = 0u;
    if_off = 0u;
    p_desc = (USBH_DESC_HDR *)p_if->IF_DataPtr;

    while (if_off < p_if->IF_DataLen) {
        p_desc = USBH_NextDescGet((void *)p_desc, &if_off);

        if (p_desc->bDescriptorType == USBH_DESC_TYPE_IF) {     /* Chk if IF desc.                                      */
            if (alt_ix == ((CPU_INT08U *)p_desc)[3]) {          /* Compare alternate setting ix.                        */
                break;
            }
        }
    }

    while (if_off < p_if->IF_DataLen) {
        p_desc = USBH_NextDescGet((void *)p_desc, &if_off);

        if (p_desc->bDescriptorType == USBH_DESC_TYPE_EP) {     /* Chk if EP desc.                                      */
            if (ix == ep_ix) {                                  /* Compare EP ix.                                       */
                USBH_ParseEP_Desc(&p_ep->Desc, (void *)p_desc);
                return (USBH_ERR_NONE);
            }
            ix++;
        }
    }

    return (USBH_ERR_EP_NOT_FOUND);
}


/*
*********************************************************************************************************
*                                         USBH_EP_StallSet()
*
* Description : Set the STALL condition on endpoint.
*
* Argument(s) : p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE,                          If endpoint STALL state is set.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_EP_StallSet (USBH_EP  *p_ep)
{
    USBH_ERR   err;
    USBH_DEV  *p_dev;


    p_dev = p_ep->DevPtr;

    (void)USBH_CtrlTx(        p_dev,
                              USBH_REQ_SET_FEATURE,
                              USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_STD | USBH_REQ_RECIPIENT_EP,
                              USBH_FEATURE_SEL_EP_HALT,
                              p_ep->Desc.bEndpointAddress,
                      (void *)0,
                              0u,
                              USBH_CFG_STD_REQ_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_dev, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_EP_StallClr()
*
* Description : Clear the STALL condition on endpoint.
*
* Argument(s) : p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE,                          If endpoint STALL state is cleared.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The standard CLEAR_FEATURE request is defined in the Universal Serial Bus
*                   specification revision 2.0, section 9.4.1.
*********************************************************************************************************
*/

USBH_ERR  USBH_EP_StallClr (USBH_EP  *p_ep)
{
    USBH_ERR   err;
    USBH_DEV  *p_dev;


    p_dev = p_ep->DevPtr;

    (void)USBH_CtrlTx(        p_dev,
                              USBH_REQ_CLR_FEATURE,             /* See Note (1)                                         */
                              USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_STD | USBH_REQ_RECIPIENT_EP,
                              USBH_FEATURE_SEL_EP_HALT,
                              p_ep->Desc.bEndpointAddress,
                      (void *)0,
                              0u,
                              USBH_CFG_STD_REQ_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_dev, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                            USBH_EP_Reset()
*
* Description : This function is used to reset the opened endpoint
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_ep        Pointer to endpoint. Default endpoint if null is passed.
*
* Return(s)   : USBH_ERR_NONE,                  If endpoint successfully reseted.
*               Host controller driver error,   otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_EP_Reset (USBH_DEV  *p_dev,
                         USBH_EP   *p_ep)
{
    USBH_EP   *p_ep_t;
    USBH_ERR   err;


    if (p_ep == (USBH_EP *)0) {
        p_ep_t = &p_dev->DfltEP;
    } else {
        p_ep_t =  p_ep;
    }

    if ((p_dev->IsRootHub            == DEF_TRUE) &&            /* Do nothing if virtual RH.                            */
        (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE)) {
        return (USBH_ERR_NONE);
    }

    USBH_HCD_EP_Abort(p_dev->HC_Ptr,                            /* Abort pending xfers on EP.                           */
                      p_ep_t,
                     &err);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    USBH_HCD_EP_Close(p_dev->HC_Ptr,                            /* Close / open EP.                                     */
                      p_ep_t,
                     &err);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    USBH_HCD_EP_Open(p_dev->HC_Ptr,
                     p_ep_t,
                    &err);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                            USBH_EP_Close()
*
* Description : Closes given endpoint and makes it unavailable for I/O transfers.
*
* Argument(s) : p_ep        Pointer to endpoint.
*
* Return(s)   : USBH_ERR_NONE,                  If endpoint successfully closed.
*               USBH_ERR_INVALID_ARG,           If invalid argument passed to 'p_ep'.
*               Host controller driver error,   Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_EP_Close (USBH_EP  *p_ep)
{
    USBH_ERR   err;
    USBH_DEV  *p_dev;
    USBH_URB  *p_async_urb;
    LIB_ERR    err_lib;


    if(p_ep == (USBH_EP *)0) {
       return(USBH_ERR_INVALID_ARG);
    }

    p_ep->IsOpen = DEF_FALSE;
    p_dev        = p_ep->DevPtr;
    err          = USBH_ERR_NONE;

    if (!((p_dev->IsRootHub            == DEF_TRUE) &&
          (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE))){
        USBH_URB_Abort(&p_ep->URB);                             /* Abort any pending URB.                               */
    }

    if (p_ep->URB.Sem != (USBH_HSEM )0) {                       /* Close EP sem and mutex.                              */
        (void)USBH_OS_SemDestroy(p_ep->URB.Sem);
        p_ep->URB.Sem = (USBH_HSEM )0;
    }

    if (p_ep->Mutex != (USBH_HMUTEX )0) {
        (void)USBH_OS_MutexDestroy(p_ep->Mutex);
        p_ep->Mutex = (USBH_HMUTEX )0;
    }

    if (!((p_dev->IsRootHub            == DEF_TRUE) &&          /* Close EP on HC.                                      */
          (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE))){
        USBH_HCD_EP_Close(p_ep->DevPtr->HC_Ptr,
                          p_ep,
                         &err);
    }

    if(p_ep->XferNbrInProgress > 1u) {

        p_async_urb = p_ep->URB.AsyncURB_NxtPtr;
        while (p_async_urb != 0) {
                                                                /* Free extra URB.                                      */
            Mem_PoolBlkFree(       &p_ep->DevPtr->HC_Ptr->HostPtr->AsyncURB_Pool,
                            (void *)p_async_urb,
                                   &err_lib);

            p_async_urb = p_async_urb->AsyncURB_NxtPtr;
        }

        p_ep->XferNbrInProgress = 0u;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_URB_Done()
*
* Description : Handle a USB request block (URB) that has been completed by host controller.
*
* Argument(s) : p_urb       Pointer to URB.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_URB_Done (USBH_URB  *p_urb)
{

    CPU_SR_ALLOC();


    if (p_urb->State == USBH_URB_STATE_SCHEDULED) {             /* URB must be in scheduled state.                      */
        p_urb->State = USBH_URB_STATE_QUEUED;                   /* Set URB state to done.                               */

        if (p_urb->FnctPtr != (void *)0) {                      /* Check if req is async.                               */
            CPU_CRITICAL_ENTER();
            p_urb->NxtPtr = (USBH_URB *)0;

            if (USBH_URB_HeadPtr == (USBH_URB *)0) {
                USBH_URB_HeadPtr = p_urb;
                USBH_URB_TailPtr = p_urb;
            } else {
                USBH_URB_TailPtr->NxtPtr = p_urb;
                USBH_URB_TailPtr         = p_urb;
            }

            CPU_CRITICAL_EXIT();

            (void)USBH_OS_SemPost(USBH_URB_Sem);
        } else {
            (void)USBH_OS_SemPost(p_urb->Sem);                  /* Post notification to waiting task.                   */
        }
    }
}


/*
*********************************************************************************************************
*                                         USBH_URB_Complete()
*
* Description : Handle a URB after transfer has been completed or aborted.
*
* Argument(s) : p_urb       Pointer to URB.
*
* Return(s)   : USBH_ERR_NONE
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_URB_Complete (USBH_URB  *p_urb)
{
    USBH_DEV  *p_dev;
    USBH_ERR   err;
    LIB_ERR    err_lib;
    USBH_URB  *p_async_urb_to_remove;
    USBH_URB  *p_prev_async_urb;
    USBH_URB   urb_temp;
    USBH_EP   *p_ep;
    CPU_SR_ALLOC();


    p_ep  = p_urb->EP_Ptr;
    p_dev = p_ep->DevPtr;

    if (p_urb->State == USBH_URB_STATE_QUEUED) {
        USBH_HCD_URB_Complete(p_dev->HC_Ptr,                    /* Call HC to cleanup completed URB.                    */
                              p_urb,
                             &err);
    } else if (p_urb->State == USBH_URB_STATE_ABORTED) {
        USBH_HCD_URB_Abort(p_dev->HC_Ptr,                       /* Call HC to abort URB.                                */
                           p_urb,
                          &err);

        p_urb->Err     = USBH_ERR_URB_ABORT;
        p_urb->XferLen = 0u;
    } else {
                                                                /* Empty Else Statement                                 */
    }

    Mem_Copy((void *)&urb_temp,                                 /* Copy urb locally before freeing it.                  */
             (void *) p_urb,
                      sizeof(USBH_URB));

                                                                /* --------- FREE URB BEFORE NOTIFYING CLASS ---------- */
    if((p_urb          != &p_ep->URB) &&                        /* Is the URB an extra URB for async function?          */
       (p_urb->FnctPtr != 0         )) {

        p_async_urb_to_remove = &p_ep->URB;
        p_prev_async_urb      = &p_ep->URB;

        while (p_async_urb_to_remove->AsyncURB_NxtPtr != 0) {   /* Srch extra URB to remove.                            */
                                                                /* Extra URB found                                      */
            if(p_async_urb_to_remove->AsyncURB_NxtPtr == p_urb) {
                                                                /* Remove from Q.                                       */
                p_prev_async_urb->AsyncURB_NxtPtr = p_urb->AsyncURB_NxtPtr;
                break;
            }
            p_prev_async_urb      = p_async_urb_to_remove;
            p_async_urb_to_remove = p_async_urb_to_remove->AsyncURB_NxtPtr;
        }
                                                                /* Free extra URB.                                      */
        Mem_PoolBlkFree(       &p_dev->HC_Ptr->HostPtr->AsyncURB_Pool,
                        (void *)p_urb,
                               &err_lib);
    }

    CPU_CRITICAL_ENTER();
    if (p_ep->XferNbrInProgress > 0u) {
        p_ep->XferNbrInProgress--;
    }
    CPU_CRITICAL_EXIT();

    if ((urb_temp.State == USBH_URB_STATE_QUEUED) ||
        (urb_temp.State == USBH_URB_STATE_ABORTED)) {
        USBH_URB_Notify(&urb_temp);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                            USBH_StrGet()
*
* Description : Read specified string descriptor, remove header and extract string data.
*
* Argument(s) : p_dev            Pointer to USB device.
*
*               desc_ix          Index of string descriptor.
*
*               lang_id          Language identifier.
*
*               p_buf            Buffer in which the string descriptor will be stored.
*
*               buf_len          Buffer length in octets.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE                           String descriptor successfully fetched.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'desc_ix'.
*                       USBH_ERR_DESC_INVALID                   Invalid string / lang id descriptor.
*
*                                                               ----- RETURNED BY USBH_StrDescGet() : -----
*                       USBH_ERR_DESC_INVALID,                  Invalid string descriptor fetched.
*                       USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                       USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                       USBH_ERR_HC_IO,                         Root hub input/output error.
*                       USBH_ERR_EP_STALL,                      Root hub does not support request.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : Length of string received.
*
* Note(s)     : (1) The string descriptor MUST be at least four bytes :
*
*                   (a) Byte  0   = Length
*                   (b) Byte  1   = Type
*                   (c) Bytes 2-3 = Default language ID
*********************************************************************************************************
*/

CPU_INT32U  USBH_StrGet (USBH_DEV    *p_dev,
                         CPU_INT08U   desc_ix,
                         CPU_INT16U   lang_id,
                         CPU_INT08U  *p_buf,
                         CPU_INT32U   buf_len,
                         USBH_ERR    *p_err)
{
    CPU_INT32U      ix;
    CPU_INT32U      str_len;
    CPU_INT08U     *p_str;
    USBH_DESC_HDR  *p_hdr;


    if (desc_ix == 0u) {                                        /* Invalid desc ix.                                     */
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    lang_id = p_dev->LangID;

    if (lang_id == 0u) {                                        /* If lang ID is zero, get dflt used by the dev.        */
        str_len = USBH_StrDescGet(p_dev,
                                  0u,
                                  0u,
                                  p_buf,
                                  buf_len,
                                  p_err);
        if (str_len < 4u)  {                                    /* See Note #1.                                         */
           *p_err = USBH_ERR_DESC_INVALID;
            return (0u);
        }

        lang_id = MEM_VAL_GET_INT16U(&p_buf[2u]);               /* Rd language ID into CPU endianness.                  */

        if (lang_id == 0u) {
           *p_err = USBH_ERR_DESC_INVALID;
            return (0u);
        } else {
            p_dev->LangID = lang_id;
        }
    }

    p_str    =  p_buf;
    p_hdr    = (USBH_DESC_HDR *)p_buf;
    str_len  =  USBH_StrDescGet(p_dev,                          /* Rd str desc with lang ID.                            */
                                desc_ix,
                                lang_id,
                                p_hdr,
                                buf_len,
                                p_err);

    if (str_len > USBH_LEN_DESC_HDR) {
        str_len = (p_hdr->bLength - 2u);                        /* Remove 2-byte header.                                */

        if (str_len > (buf_len - 2u)) {
            str_len = (buf_len - 2u);
        }

        for (ix = 0u; ix < str_len; ix++) {
            p_str[ix] = p_str[2u + ix];                         /* Str starts from byte 3 in desc.                      */
        }

        p_str[ix]   = 0u;
        p_str[++ix] = 0u;
        str_len     = str_len / 2u;                             /* Len of ANSI str.                                     */

        return (str_len);
    } else {
       *p_err = USBH_ERR_DESC_INVALID;
        return (0u);
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           USBH_EP_Open()
*
* Description : Open an endpoint.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to interface containing endpoint.
*
*               ep_type     Endpoint type.
*
*               ep_dir      Endpoint direction.
*
*               p_ep        Pointer to endpoint passed by class.
*
* Return(s)   : USBH_ERR_NONE                       If endpoint is successfully opened.
*               USBH_ERR_EP_ALLOC                   If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND               If endpoint with given type and direction not found.
*               Host controller drivers error,      Otherwise.
*
*                                                   ----- RETURNED BY USBH_OS_MutexCreate() : -----
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex creation failed.
*
*                                                   ----- RETURNED BY USBH_OS_SemCreate() : -----
*               USBH_ERR_OS_SIGNAL_CREATE,          If semaphore creation failed.
*
* Note(s)     : (1) A full-speed interrupt endpoint can specify a desired period from 1 ms to 255 ms.
*                   Low-speed endpoints are limited to specifying only 10 ms to 255 ms. High-speed
*                   endpoints can specify a desired period 2^(bInterval-1) x 125us, where bInterval is in
*                   the range [1 .. 16].
*
*               (2) Full-/high-speed isochronous endpoints must specify a desired period as
*                   2^(bInterval-1) x F, where bInterval is in the range [1 .. 16] and F is 125us for
*                   high-speed and 1ms for full-speed.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_EP_Open (USBH_DEV      *p_dev,
                                USBH_IF       *p_if,
                                USBH_EP_TYPE   ep_type,
                                USBH_EP_DIR    ep_dir,
                                USBH_EP       *p_ep)
{
    CPU_INT08U   ep_desc_dir;
    CPU_INT08U   ep_desc_type;
    CPU_INT08U   ep_ix;
    CPU_INT08U   nbr_eps;
    CPU_BOOLEAN  ep_found;
    USBH_ERR     err;


    if (p_ep->IsOpen == DEF_TRUE) {
        return (USBH_ERR_NONE);
    }

    USBH_URB_Clr(&p_ep->URB);

    ep_found     = DEF_FALSE;
    ep_desc_type = 0u;
    nbr_eps      = USBH_IF_EP_NbrGet(p_if, p_if->AltIxSel);

    if (nbr_eps > USBH_CFG_MAX_NBR_EPS) {
        err = USBH_ERR_EP_ALLOC;
        return (err);
    }

    for (ep_ix = 0u; ep_ix < nbr_eps; ep_ix++) {
        USBH_EP_Get(p_if,
                    p_if->AltIxSel,
                    ep_ix,
                    p_ep);

        ep_desc_type = p_ep->Desc.bmAttributes     & 0x03u;     /* EP type from desc.                                   */
        ep_desc_dir  = p_ep->Desc.bEndpointAddress & 0x80u;     /* EP dir from desc.                                    */

        if (ep_desc_type == ep_type) {
            if ((ep_desc_type == USBH_EP_TYPE_CTRL) ||
                (ep_desc_dir  == ep_dir)) {
                ep_found = DEF_TRUE;
                break;
            }
        }
    }

    if (ep_found == DEF_FALSE) {
        return (USBH_ERR_EP_NOT_FOUND);                         /* Class specified EP not found.                        */
    }

    p_ep->Interval = 0u;
    if (ep_desc_type == USBH_EP_TYPE_INTR) {                    /* ------------ DETERMINE POLLING INTERVAL ------------ */

        if (p_ep->Desc.bInterval > 0) {                         /* See Note #1.                                         */

            if ((p_dev->DevSpd == USBH_DEV_SPD_LOW ) ||
                (p_dev->DevSpd == USBH_DEV_SPD_FULL)) {

                if (p_dev->HubHS_Ptr != (USBH_HUB_DEV *) 0) {
                    p_ep->Interval = 8u * p_ep->Desc.bInterval; /* 1 (1ms)frame = 8 (125us)microframe.                  */
                } else {
                    p_ep->Interval = p_ep->Desc.bInterval;
                }
            } else {                                            /* DevSpd == USBH_DEV_SPD_HIGH                          */
                p_ep->Interval = 1 << (p_ep->Desc.bInterval - 1);  /* For HS, interval is 2 ^ (bInterval - 1).          */
            }
        }
    } else if (ep_desc_type == USBH_EP_TYPE_ISOC) {
        p_ep->Interval = 1 << (p_ep->Desc.bInterval - 1);       /* Isoc interval is 2 ^ (bInterval - 1). See Note #2.   */
    } else {
                                                                /* Empty Else Statement                                 */
    }

    p_ep->DevAddr = p_dev->DevAddr;
    p_ep->DevSpd  = p_dev->DevSpd;
    p_ep->DevPtr  = p_dev;

    if (!((p_dev->IsRootHub            == DEF_TRUE) &&
          (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE))){

        USBH_HCD_EP_Open(p_dev->HC_Ptr,
                         p_ep,
                        &err);
        if (err != USBH_ERR_NONE) {
            return (err);
        }
    }

    err = USBH_OS_SemCreate(&p_ep->URB.Sem, 0u);                /* Sem for I/O wait.                                    */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_OS_MutexCreate(&p_ep->Mutex);                    /* Mutex to sync I/O req on same EP.                    */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    p_ep->IsOpen     = DEF_TRUE;
    p_ep->URB.EP_Ptr = p_ep;

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_SyncTransfer()
*
* Description : Perform synchronous transfer on endpoint.
*
* Argument(s) : p_ep                 Pointer to endpoint.
*
*               p_buf                Pointer to data buffer.
*
*               buf_len              Number of data bytes in buffer.
*
*               p_isoc_desc          Pointer to isochronous descriptor.
*
*               token                USB token to issue.
*
*               timeout_ms           Timeout, in milliseconds.
*
*               p_err                Variable that will receive the return error code from this function.
*                           USBH_ERR_NONE                           Transfer successfully completed.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*
*                                                                   ----- RETURNED BY USBH_URB_Submit() : -----
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           USBH_ERR_EP_INVALID_STATE,              Endpoint is in halt state.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_OS_SemWait() : -----
*                           USBH_ERR_INVALID_ARG                    If invalid argument passed to 'sem'.
*                           USBH_ERR_OS_TIMEOUT                     If semaphore pend reached specified timeout.
*                           USBH_ERR_OS_ABORT                       If pend on semaphore was aborted.
*                           USBH_ERR_OS_FAIL                        Otherwise.
*
* Return(s)   : Number of bytes transmitted or received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_SyncXfer (USBH_EP         *p_ep,
                                   void            *p_buf,
                                   CPU_INT32U       buf_len,
                                   USBH_ISOC_DESC  *p_isoc_desc,
                                   USBH_TOKEN       token,
                                   CPU_INT32U       timeout_ms,
                                   USBH_ERR        *p_err)
{
    CPU_INT32U   len;
    USBH_URB    *p_urb;

                                                                /* Argument checks for valid settings                   */
    if (p_ep == (USBH_EP *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    if (p_ep->IsOpen == DEF_FALSE) {
       *p_err = USBH_ERR_EP_INVALID_STATE;
        return (0u);
    }

    (void)USBH_OS_MutexLock(p_ep->Mutex);

    p_urb              = &p_ep->URB;
    p_urb->EP_Ptr      =  p_ep;
    p_urb->IsocDescPtr =  p_isoc_desc;
    p_urb->UserBufPtr  =  p_buf;
    p_urb->UserBufLen  =  buf_len;
    p_urb->DMA_BufLen  =  0u;
    p_urb->DMA_BufPtr  = (void *)0;
    p_urb->XferLen     =  0u;
    p_urb->FnctPtr     =  0u;
    p_urb->FnctArgPtr  =  0u;
    p_urb->State       =  USBH_URB_STATE_NONE;
    p_urb->ArgPtr      = (void *)0;
    p_urb->Token       =  token;

   *p_err = USBH_URB_Submit(p_urb);

    if (*p_err == USBH_ERR_NONE) {                              /* Transfer URB to HC.                                  */
       *p_err = USBH_OS_SemWait(p_urb->Sem, timeout_ms);        /* Wait on URB completion notification.                 */
    }

    if (*p_err == USBH_ERR_NONE) {
        USBH_URB_Complete(p_urb);
       *p_err = p_urb->Err;
    } else {
        USBH_URB_Abort(p_urb);
    }

    len          = p_urb->XferLen;
    p_urb->State = USBH_URB_STATE_NONE;
    (void)USBH_OS_MutexUnlock(p_ep->Mutex);

    return (len);
}


/*
*********************************************************************************************************
*                                           USBH_AsyncXfer()
*
* Description : Perform asynchronous transfer on endpoint.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer.
*
*               buf_len         Number of data bytes in buffer.
*
*               p_isoc_desc     Pointer to isochronous descriptor.
*
*               token           USB token to issue.
*
*               p_fnct          Function that will be invoked upon completion of receive operation.
*
*               p_fnct_arg      Pointer to argument that will be passed as parameter of 'p_fnct'.
*
* Return(s)   : USBH_ERR_NONE                           If transfer successfully submitted.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*
*                                                       ----- RETURNED BY USBH_URB_Submit() : -----
*               USBH_ERR_NONE,                          If URB is successfully submitted to host controller.
*               USBH_ERR_EP_INVALID_STATE,              If endpoint is in halt state.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This function returns immediately. The URB completion will be invoked by the
*                   asynchronous I/O task when the transfer is completed.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_AsyncXfer (USBH_EP         *p_ep,
                                  void            *p_buf,
                                  CPU_INT32U       buf_len,
                                  USBH_ISOC_DESC  *p_isoc_desc,
                                  USBH_TOKEN       token,
                                  void            *p_fnct,
                                  void            *p_fnct_arg)
{
    USBH_ERR     err;
    USBH_URB    *p_urb;
    USBH_URB    *p_async_urb;
    MEM_POOL    *p_async_urb_pool;
    LIB_ERR      err_lib;


    if (p_ep->IsOpen == DEF_FALSE) {
        return (USBH_ERR_EP_INVALID_STATE);
    }

    if((p_ep->URB.State         != USBH_URB_STATE_SCHEDULED) && /* Chk if no xfer is pending or in progress on EP.      */
       (p_ep->XferNbrInProgress == 0u                      )) {

        p_urb = &p_ep->URB;                                     /* Use URB struct associated to EP.                     */

    } else if (p_ep->XferNbrInProgress >= 1u) {
                                                                /* Get a new URB struct from the URB async pool.        */
        p_async_urb_pool = &p_ep->DevPtr->HC_Ptr->HostPtr->AsyncURB_Pool;
        p_urb            = (USBH_URB *)Mem_PoolBlkGet(p_async_urb_pool,
                                                      sizeof(USBH_URB),
                                                     &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (USBH_ERR_ALLOC);
        }

        USBH_URB_Clr(p_urb);

        p_async_urb = &p_ep->URB;                               /* Get head of extra async URB Q.                       */

        while (p_async_urb->AsyncURB_NxtPtr != 0) {             /* Srch tail of extra async URB Q.                      */
            p_async_urb = p_async_urb->AsyncURB_NxtPtr;
        }

        p_async_urb->AsyncURB_NxtPtr = p_urb;                   /* Insert new URB at end of extra async URB Q.          */
    } else {
        return (USBH_ERR_UNKNOWN);
    }
    p_ep->XferNbrInProgress++;

    p_urb->EP_Ptr      =  p_ep;                                 /* ------------------- PREPARE URB -------------------- */
    p_urb->IsocDescPtr =  p_isoc_desc;
    p_urb->UserBufPtr  =  p_buf;
    p_urb->UserBufLen  =  buf_len;
    p_urb->XferLen     =  0u;
    p_urb->FnctPtr     = (void *)p_fnct;
    p_urb->FnctArgPtr  =  p_fnct_arg;
    p_urb->State       =  USBH_URB_STATE_NONE;
    p_urb->ArgPtr      = (void *)0;
    p_urb->Token       =  token;

    err = USBH_URB_Submit(p_urb);                               /* See Note (1).                                        */

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_SyncCtrlXfer()
*
* Description : Perform synchronous control transfer on endpoint.
*
* Argument(s) : p_ep              Pointer to endpoint.
*
*               b_req             One-byte value that specifies request.
*
*               bm_req_type       One-byte value that specifies direction, type and recipient of request.
*
*               w_value           Two-byte value used to pass information to device.
*
*               w_ix              Two-byte value used to pass an index or offset (such as an interface or
*                                 endpoint number) to device.
*
*               p_arg             Pointer to data buffer.
*
*               w_len             Two-byte value containing number of data bytes in data stage.
*
*               timeout_ms        Timeout, in milliseconds.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                           USBH_ERR_NONE                           Control transfer successfully completed.
*                           USBH_ERR_UNKNOWN                        Unknown error occurred.
*
*                                                                   ----- RETURNED BY USBH_SyncXfer() : -----
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of bytes transfered.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT16U  USBH_SyncCtrlXfer (USBH_EP     *p_ep,
                                       CPU_INT08U   b_req,
                                       CPU_INT08U   bm_req_type,
                                       CPU_INT16U   w_val,
                                       CPU_INT16U   w_ix,
                                       void        *p_arg,
                                       CPU_INT16U   w_len,
                                       CPU_INT32U   timeout_ms,
                                       USBH_ERR    *p_err)
{
    USBH_SETUP_REQ   setup;
    CPU_INT08U       setup_buf[8];
    CPU_BOOLEAN      is_in;
    CPU_INT32U       len;
    CPU_INT16U       rtn_len;
    CPU_INT08U      *p_data_08;


    setup.bmRequestType = bm_req_type;                          /* ------------------- SETUP STAGE -------------------- */
    setup.bRequest      = b_req;
    setup.wValue        = w_val;
    setup.wIndex        = w_ix;
    setup.wLength       = w_len;

    USBH_FmtSetupReq(&setup, setup_buf);
    is_in = (bm_req_type & USBH_REQ_DIR_MASK) != 0u ? DEF_TRUE : DEF_FALSE;

    len = USBH_SyncXfer(                   p_ep,
                        (void           *)&setup_buf[0u],
                                           USBH_LEN_SETUP_PKT,
                        (USBH_ISOC_DESC *) 0,
                                           USBH_TOKEN_SETUP,
                                           timeout_ms,
                                           p_err);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    if (len != USBH_LEN_SETUP_PKT) {
       *p_err = USBH_ERR_UNKNOWN;
        return (0u);
    }

    if (w_len > 0u) {                                           /* -------------------- DATA STAGE -------------------- */
        p_data_08 = (CPU_INT08U *)p_arg;

        rtn_len = USBH_SyncXfer(                  p_ep,
                                (void           *)p_data_08,
                                                  w_len,
                                (USBH_ISOC_DESC *)0,
                                                 (is_in ? USBH_TOKEN_IN : USBH_TOKEN_OUT),
                                                  timeout_ms,
                                                  p_err);
        if (*p_err != USBH_ERR_NONE) {
            return (0u);
        }
    } else {
        rtn_len = 0u;
    }

    (void)USBH_SyncXfer(                  p_ep,                /* ------------------- STATUS STAGE ------------------- */
                        (void           *)0,
                                          0u,
                        (USBH_ISOC_DESC *)0,
                                         ((w_len && is_in) ? USBH_TOKEN_OUT : USBH_TOKEN_IN),
                                          timeout_ms,
                                          p_err);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    return (rtn_len);
}


/*
*********************************************************************************************************
*                                          USBH_URB_Abort()
*
* Description : Abort pending URB.
*
* Argument(s) : p_urb       Pointer to URB.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_URB_Abort (USBH_URB  *p_urb)
{
    CPU_BOOLEAN  cmpl;
    CPU_SR_ALLOC();


    cmpl = DEF_FALSE;

    CPU_CRITICAL_ENTER();

    if (p_urb->State == USBH_URB_STATE_SCHEDULED) {
        p_urb->State = USBH_URB_STATE_ABORTED;                  /* Abort scheduled URB.                                 */
        cmpl         = DEF_TRUE;                                /* Mark URB as completion pending.                      */

    } else if (p_urb->State == USBH_URB_STATE_QUEUED) {         /* Is URB queued in async Q?                            */
                                                                /* URB is in async lst.                                 */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("URB is in ASYNC QUEUE\r\n");
#endif

        p_urb->State = USBH_URB_STATE_ABORTED;
    } else {
                                                                /* Empty Else Statement                                 */
    }

    CPU_CRITICAL_EXIT();

    if (cmpl == DEF_TRUE) {
        USBH_URB_Complete(p_urb);
    }
}


/*
*********************************************************************************************************
*                                          USBH_URB_Notify()
*
* Description : Notify application about state of given URB.
*
* Argument(s) : p_urb       Pointer to URB.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_URB_Notify (USBH_URB  *p_urb)
{
    CPU_INT16U            nbr_frm;
    CPU_INT16U           *p_frm_len;
    CPU_INT32U            buf_len;
    CPU_INT32U            xfer_len;
    CPU_INT32U            start_frm;
    void                 *p_buf;
    void                 *p_arg;
    USBH_EP              *p_ep;
    USBH_ISOC_DESC       *p_isoc_desc;
    USBH_XFER_CMPL_FNCT   p_xfer_fnct;
    USBH_ISOC_CMPL_FNCT   p_isoc_fnct;
    USBH_ERR             *p_frm_err;
    USBH_ERR              err;
    LIB_ERR               err_lib;
    CPU_SR_ALLOC();


    p_ep        = p_urb->EP_Ptr;
    p_isoc_desc = p_urb->IsocDescPtr;

    CPU_CRITICAL_ENTER();

    if ((p_urb->State   == USBH_URB_STATE_ABORTED) &&
        (p_urb->FnctPtr == (void  *)0            )) {
         p_urb->State = USBH_URB_STATE_NONE;
         (void)USBH_OS_SemWaitAbort(p_urb->Sem);
    }

    if (p_urb->FnctPtr != (void  *)0) {                         /*  Save URB info.                                       */

        p_buf        = p_urb->UserBufPtr;
        buf_len      = p_urb->UserBufLen;
        xfer_len     = p_urb->XferLen;
        p_arg        = p_urb->FnctArgPtr;
        err          = p_urb->Err;
        p_urb->State = USBH_URB_STATE_NONE;

        if (p_isoc_desc == (USBH_ISOC_DESC *)0) {
            p_xfer_fnct  = (USBH_XFER_CMPL_FNCT)p_urb->FnctPtr;
            CPU_CRITICAL_EXIT();

            p_xfer_fnct(p_ep,
                        p_buf,
                        buf_len,
                        xfer_len,
                        p_arg,
                        err);
        } else {
            p_isoc_fnct  = (USBH_ISOC_CMPL_FNCT)p_urb->FnctPtr;
            start_frm    = p_isoc_desc->StartFrm;
            nbr_frm      = p_isoc_desc->NbrFrm;
            p_frm_len    = p_isoc_desc->FrmLen;
            p_frm_err    = p_isoc_desc->FrmErr;
            CPU_CRITICAL_EXIT();

            Mem_PoolBlkFree(&p_ep->DevPtr->HC_Ptr->HostPtr->IsocDescPool,
                             p_isoc_desc,
                            &err_lib);

            p_isoc_fnct(p_ep,
                        p_buf,
                        buf_len,
                        xfer_len,
                        start_frm,
                        nbr_frm,
                        p_frm_len,
                        p_frm_err,
                        p_arg,
                        err);
        }
    } else {
        CPU_CRITICAL_EXIT();
    }
}


/*
*********************************************************************************************************
*                                           USBH_URB_Submit()
*
* Description : Submit given URB to host controller.
*
* Argument(s) : p_urb       Pointer to URB.
*
* Return(s)   : USBH_ERR_NONE,                          If URB is successfully submitted to host controller.
*               USBH_ERR_EP_INVALID_STATE,              If endpoint is in halt state.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_URB_Submit (USBH_URB  *p_urb)
{
    USBH_ERR      err;
    CPU_BOOLEAN   ep_is_halt;
    USBH_DEV     *p_dev;


    p_dev = p_urb->EP_Ptr->DevPtr;
    USBH_HCD_EP_IsHalt(p_dev->HC_Ptr,                           /* Check EP's state.                                    */
                       p_urb->EP_Ptr,
                      &ep_is_halt,
                      &err);
    if ((ep_is_halt == DEF_TRUE) &&
        (err        == USBH_ERR_NONE)) {
        return (USBH_ERR_EP_INVALID_STATE);
    }

    p_urb->State = USBH_URB_STATE_SCHEDULED;                    /* Set URB state to scheduled.                          */
    p_urb->Err   = USBH_ERR_NONE;

    USBH_HCD_URB_Submit(p_dev->HC_Ptr,
                        p_urb,
                       &err);

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_URB_Clr()
*
* Description : Clear URB structure
*
* Argument(s) : p_urb       Pointer to URB structure to clear.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_URB_Clr (USBH_URB  *p_urb)
{
    p_urb->Err             =  USBH_ERR_NONE;
    p_urb->State           =  USBH_URB_STATE_NONE;
    p_urb->AsyncURB_NxtPtr = (USBH_URB *)0;
}


/*
*********************************************************************************************************
*                                          USBH_DfltEP_Open()
*
* Description : Open default control endpoint of given USB device.
*
* Argument(s) : p_dev       Pointer to USB device
*
* Return(s)   : USBH_ERR_NONE,                      If control endpoint was successfully opened.
*               Host controller driver error,       otherwise.
*
*                                                   ----- RETURNED BY USBH_OS_SemCreate() : -----
*               USBH_ERR_OS_SIGNAL_CREATE,          If semaphore creation failed.
*
*                                                   ----- RETURNED BY USBH_OS_MutexCreate() : -----
*               USBH_ERR_OS_SIGNAL_CREATE,          If mutex creation failed.
*
* Note(s)     : (1) The maximum packet size is unknown until device descriptor is read. For initial
*                   transfer, the "Universal Serial Bus Specification Revision 2.0", Section 5.5.3 states
*                   that a maximum packet size of 8 should be used for low speed devices and
*                   a maximum packet size of 64 should be used for high speed devices.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_DfltEP_Open (USBH_DEV  *p_dev)
{
    CPU_INT16U   ep_max_pkt_size;
    USBH_EP     *p_ep;
    USBH_ERR     err;


    p_ep = &p_dev->DfltEP;

    if (p_ep->IsOpen == DEF_TRUE) {
        return (USBH_ERR_NONE);
    }

    p_ep->DevAddr = 0u;
    p_ep->DevSpd  = p_dev->DevSpd;
    p_ep->DevPtr  = p_dev;

    if (p_dev->DevSpd == USBH_DEV_SPD_LOW) {                    /* See Note (1).                                        */
        ep_max_pkt_size = 8u;
    } else {
        ep_max_pkt_size = 64u;
    }

    p_ep->Desc.bLength          = 7u;
    p_ep->Desc.bDescriptorType  = USBH_DESC_TYPE_EP;
    p_ep->Desc.bEndpointAddress = 0u;
    p_ep->Desc.bmAttributes     = USBH_EP_TYPE_CTRL;
    p_ep->Desc.wMaxPacketSize   = ep_max_pkt_size;
    p_ep->Desc.bInterval        = 0u;

    if (!((p_dev->IsRootHub            == DEF_TRUE) &&          /* Chk if RH fncts are supported before calling HCD.    */
          (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE))){

        USBH_HCD_EP_Open(p_dev->HC_Ptr,                         /* Open EP.                                             */
                         p_ep,
                        &err);
        if (err != USBH_ERR_NONE) {
            return (err);
        }
    }

    err = USBH_OS_SemCreate(&p_ep->URB.Sem, 0u);                /* Create OS resources needed for EP.                   */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_OS_MutexCreate(&p_ep->Mutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    p_ep->URB.EP_Ptr = p_ep;
    p_ep->IsOpen     = DEF_TRUE;

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_DevDescRd()
*
* Description : Read device descriptor from device.
*
* Argument(s) : p_dev       Pointer to USB device
*
* Return(s)   : USBH_ERR_NONE,                          If a valid device descriptor was fetched.
*               USBH_ERR_DESC_INVALID,                  If an invalid device descriptor was fetched.
*               USBH_ERR_DEV_NOT_RESPONDING,            If device is not responding.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) (a) The GET_DESCRIPTOR request is described in "Universal Serial Bus Specification
*                       Revision 2.0", section 9.4.3.
*
*                   (b) According the USB Spec, when host doesn't know the control max packet size, it
*                       should assume 8 byte pkt size and request only 8 bytes from device descriptor.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_DevDescRd (USBH_DEV  *p_dev)
{
    USBH_ERR       err;
    CPU_INT08U     retry;
    USBH_DEV_DESC  dev_desc;


    retry = 3u;
    while (retry > 0u) {
        retry--;
                                                                /* ---------- READ FIRST 8 BYTES OF DEV DESC ---------- */
        USBH_GET_DESC(p_dev,
                      USBH_DESC_TYPE_DEV,
                      0u,
                      p_dev->DevDesc,
                      8u,                                       /* See Note (1).                                        */
                     &err);
        if (err != USBH_ERR_NONE) {
            USBH_EP_Reset(p_dev, (USBH_EP *)0);
            USBH_OS_DlyMS(100u);
        } else {
            break;
        }
    }
    if (err != (USBH_ERR_NONE)) {
        return (err);
    }

    if (!((p_dev->IsRootHub            == DEF_TRUE) &&
          (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE))){

                                                                /* Retrieve EP 0 max pkt size.                          */
        p_dev->DfltEP.Desc.wMaxPacketSize = MEM_VAL_GET_INT08U(&p_dev->DevDesc[7u]);
        if (p_dev->DfltEP.Desc.wMaxPacketSize > 64u) {
            return (USBH_ERR_DESC_INVALID);
        }

        USBH_HCD_EP_Close(p_dev->HC_Ptr,
                         &p_dev->DfltEP,
                         &err);

        USBH_HCD_EP_Open(p_dev->HC_Ptr,                         /* Modify EP with new max pkt size.                     */
                        &p_dev->DfltEP,
                        &err);
        if (err != USBH_ERR_NONE) {
            USBH_PRINT_ERR(err);
            return (err);
        }
    }

    retry = 3u;
    while(retry > 0u) {
        retry--;
                                                                /* ---------------- RD FULL DEV DESC. ----------------- */
        USBH_GET_DESC(p_dev,
                      USBH_DESC_TYPE_DEV,
                      0u,
                      p_dev->DevDesc,
                      USBH_LEN_DESC_DEV,
                     &err);
        if (err != USBH_ERR_NONE) {
            USBH_EP_Reset(p_dev, (USBH_EP *)0);
            USBH_OS_DlyMS(100u);
        } else {
            break;
        }
    }
    if (err != USBH_ERR_NONE) {
        return (err);
    }

                                                                /* ---------------- VALIDATE DEV DESC ----------------- */
    USBH_DevDescGet(p_dev, &dev_desc);

    if ((dev_desc.bLength             < USBH_LEN_DESC_DEV)  ||
        (dev_desc.bDescriptorType    != USBH_DESC_TYPE_DEV) ||
        (dev_desc.bNbrConfigurations == 0u)) {
        return (USBH_ERR_DESC_INVALID);
    }

    if ((dev_desc.bDeviceClass != USBH_CLASS_CODE_USE_IF_DESC        ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_AUDIO              ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_CDC_CTRL           ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_HID                ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_PHYSICAL           ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_IMAGE              ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_PRINTER            ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_MASS_STORAGE       ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_HUB                ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_CDC_DATA           ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_SMART_CARD         ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_CONTENT_SECURITY   ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_VIDEO              ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_PERSONAL_HEALTHCARE) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_DIAGNOSTIC_DEV     ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_WIRELESS_CTRLR     ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_MISCELLANEOUS      ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_APP_SPECIFIC       ) &&
        (dev_desc.bDeviceClass != USBH_CLASS_CODE_VENDOR_SPECIFIC    )) {
        return (USBH_ERR_DESC_INVALID);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                             USBH_CfgRd()
*
* Description : Read configuration descriptor for a given configuration number.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               cfg_ix      Configuration number.
*
* Return(s)   : USBH_ERR_NONE,                          If valid configuration descriptor was fetched.
*               USBH_ERR_DESC_INVALID,                  If invalid configuration descriptor was fetched.
*               USBH_ERR_CFG_MAX_CFG_LEN,               If configuration descriptor length > USBH_CFG_MAX_CFG_DATA_LEN
*               USBH_ERR_NULL_PTR                       If an invalid null pointer was obtain for 'p_cfg'.
*
*                                                       ----- RETURNED BY USBH_GET_DESC() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The standard GET_DESCRIPTOR request is described in "Universal Serial Bus
*                   Specification Revision 2.0", section 9.4.3.
*
*               (2) According to "Universal Serial Bus Specification Revision 2.0",  the standard
*                   configuration descriptor is of length 9 bytes.
*
*               (3) Offset 2 of standard configuration descriptor contains the total length of that
*                   configuration which includes all interfaces and endpoint descriptors.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CfgRd (USBH_DEV    *p_dev,
                              CPU_INT08U   cfg_ix)
{
    CPU_INT16U   w_tot_len;
    CPU_INT16U   b_read;
    USBH_ERR     err;
    USBH_CFG    *p_cfg;
    CPU_INT08U   retry;


    p_cfg = USBH_CfgGet(p_dev, cfg_ix);
    if (p_cfg == (USBH_CFG *)0) {
        return (USBH_ERR_NULL_PTR);
    }

    retry = 3u;
    while(retry > 0u) {
        retry--;
        b_read = USBH_GET_DESC(p_dev,                           /* See Note (1).                                        */
                               USBH_DESC_TYPE_CFG,
                               cfg_ix,
                               p_cfg->CfgData,
                               USBH_LEN_DESC_CFG,               /* See Note (2).                                        */
                              &err);
        if (err != USBH_ERR_NONE) {
            USBH_EP_Reset(p_dev, (USBH_EP *)0);
            USBH_OS_DlyMS(100u);
        } else {
            break;
        }
    }
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (b_read <  USBH_LEN_DESC_CFG) {
         return (USBH_ERR_DESC_INVALID);
    }

    if (p_cfg->CfgData[1] != USBH_DESC_TYPE_CFG) {
        return (USBH_ERR_DESC_INVALID);
    }

    w_tot_len = MEM_VAL_GET_INT16U_LITTLE(&p_cfg->CfgData[2]);  /* See Note (3).                                        */

    if (w_tot_len > USBH_CFG_MAX_CFG_DATA_LEN) {                /* Chk total len of config desc.                        */
        return (USBH_ERR_CFG_MAX_CFG_LEN);
    }

    retry = 3u;
    while(retry > 0u) {
        retry--;
        b_read = USBH_GET_DESC(p_dev,                           /* Read full config desc.                               */
                               USBH_DESC_TYPE_CFG,
                               cfg_ix,
                               p_cfg->CfgData,
                               w_tot_len,
                              &err);
        if (err != USBH_ERR_NONE) {
            USBH_EP_Reset(p_dev, (USBH_EP *)0);
            USBH_OS_DlyMS(100u);
        } else {
            break;
        }
    }
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (b_read  < w_tot_len) {
        return (USBH_ERR_DESC_INVALID);
    }

    if (p_cfg->CfgData[1] != USBH_DESC_TYPE_CFG) {              /* Validate config desc.                                */
        return (USBH_ERR_DESC_INVALID);
    }

    p_cfg->CfgDataLen = w_tot_len;
    err               = USBH_CfgParse(p_dev, p_cfg);

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_CfgParse()
*
* Description : Parse given configuration descriptor.
*
* Argument(s) : p_dev       Pointer to USB Device.
*
*               p_cfg       Pointer to USB configuration.
*
* Return(s)   : USBH_ERR_NONE           If configuration descriptor successfully parsed.
*               USBH_ERR_IF_ALLOC       If number of interfaces > USBH_CFG_MAX_NBR_IFS.
*               USBH_ERR_DESC_INVALID   If any descriptor in this configuration is invalid.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CfgParse (USBH_DEV  *p_dev,
                                 USBH_CFG  *p_cfg)
{
    CPU_INT08S      if_ix;
    CPU_INT08U      nbr_ifs;
    CPU_INT32U      cfg_off;
    USBH_IF        *p_if;
    USBH_DESC_HDR  *p_desc;
    USBH_CFG_DESC   cfg_desc;
    USBH_IF_DESC    if_desc;
    USBH_EP_DESC    ep_desc;


    cfg_off = 0u;
    p_desc  = (USBH_DESC_HDR *)p_cfg->CfgData;

                                                                /* ---------------- VALIDATE CFG DESC ----------------- */
    USBH_ParseCfgDesc(&cfg_desc, p_desc);
    if ((cfg_desc.bMaxPower       > 250u) ||
        (cfg_desc.bNbrInterfaces == 0u)) {
        return (USBH_ERR_DESC_INVALID);
    }

    nbr_ifs = USBH_CfgIF_NbrGet(p_cfg);                         /* Nbr of IFs present in config.                        */
    if (nbr_ifs > USBH_CFG_MAX_NBR_IFS) {
        return (USBH_ERR_IF_ALLOC);
    }

    if_ix =  0u;
    p_if  = (USBH_IF *)0;

    while (cfg_off < p_cfg->CfgDataLen) {
        p_desc = USBH_NextDescGet((void *)p_desc,
                                         &cfg_off);

                                                                /* ---------- VALIDATE INTERFACE DESCRIPTOR ----------- */
        if (p_desc->bDescriptorType == USBH_DESC_TYPE_IF) {
            USBH_ParseIF_Desc(&if_desc, p_desc);
            if ((if_desc.bInterfaceClass != USBH_CLASS_CODE_AUDIO              ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_CDC_CTRL           ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_HID                ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_PHYSICAL           ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_IMAGE              ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_PRINTER            ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_MASS_STORAGE       ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_HUB                ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_CDC_DATA           ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_SMART_CARD         ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_CONTENT_SECURITY   ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_VIDEO              ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_PERSONAL_HEALTHCARE) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_DIAGNOSTIC_DEV     ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_WIRELESS_CTRLR     ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_MISCELLANEOUS      ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_APP_SPECIFIC       ) &&
                (if_desc.bInterfaceClass != USBH_CLASS_CODE_VENDOR_SPECIFIC    )) {
                return (USBH_ERR_DESC_INVALID);
            }

            if ((if_desc.bNbrEndpoints > 30u)) {
                return (USBH_ERR_DESC_INVALID);
            }

            if (if_desc.bAlternateSetting == 0u) {
                p_if             = &p_cfg->IF_List[if_ix];
                p_if->DevPtr     = (USBH_DEV   *)p_dev;
                p_if->IF_DataPtr = (CPU_INT08U *)p_desc;
                p_if->IF_DataLen =  0u;
                if_ix++;
            }
        }

        if (p_desc->bDescriptorType == USBH_DESC_TYPE_EP) {
            USBH_ParseEP_Desc(&ep_desc, p_desc);

            if ((ep_desc.bEndpointAddress == 0x00u) ||
                (ep_desc.bEndpointAddress == 0x80u) ||
                (ep_desc.wMaxPacketSize   == 0u)){
                return (USBH_ERR_DESC_INVALID);
            }
        }

        if (p_if != (USBH_IF *)0) {
            p_if->IF_DataLen += p_desc->bLength;
        }
    }

    if (if_ix != nbr_ifs) {                                     /* IF count must match max nbr of IFs.                  */
        return (USBH_ERR_DESC_INVALID);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_DevAddrSet()
*
* Description : Assign address to given USB device.
*
* Argument(s) : p_dev       Pointer to USB device.
*
* Return(s)   : USBH_ERR_NONE,                          If device address was successfully set.
*               Host controller driver error,           Otherwise.
*
*                                                       ----- RETURNED BY USBH_SET_ADDR() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SET_ADDRESS request is described in "Universal Serial Bus Specification
*                   Revision 2.0", section 9.4.6.
*
*               (2) USB 2.0 spec states that after issuing SET_ADDRESS request, the host must wait
*                   at least 2 millisecs. During this time the device will go into the addressed state.
*********************************************************************************************************
*/

static USBH_ERR  USBH_DevAddrSet (USBH_DEV  *p_dev)
{
    USBH_ERR    err;
    CPU_INT08U  retry;


    retry = 3u;
    while(retry > 0u) {
        retry--;

        USBH_SET_ADDR(p_dev, p_dev->DevAddr, &err);             /* See Note (1).                                        */
        if (err != USBH_ERR_NONE) {
            USBH_EP_Reset(p_dev, (USBH_EP *)0);
            USBH_OS_DlyMS(100u);
        } else {
            break;
        }
    }
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if ((p_dev->IsRootHub            == DEF_TRUE) &&
        (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE)) {
        return (USBH_ERR_NONE);
    }

    USBH_HCD_EP_Close(p_dev->HC_Ptr,
                     &p_dev->DfltEP,
                     &err);

    p_dev->DfltEP.DevAddr = p_dev->DevAddr;                     /* Update addr.                                         */

    USBH_HCD_EP_Open(p_dev->HC_Ptr,                             /* Modify ctrl EP with new USB addr.                    */
                    &p_dev->DfltEP,
                    &err);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    USBH_OS_DlyMS(2u);                                          /* See Note (2).                                        */

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_StrDescGet()
*
* Description : Read specified string descriptor from USB device.
*
* Argument(s) : p_dev            Pointer to USB device.
*
*               desc_ix          Index of the string descriptor.
*
*               lang_id          Language identifier.
*
*               p_buf            Buffer in which the string descriptor will be stored.
*
*               buf_len          Buffer length, in octets.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE,                          String descriptor successfully fetched.
*                       USBH_ERR_DESC_INVALID,                  Invalid string descriptor fetched.
*
*                                                               ----- RETURNED BY USBH_CtrlRx() : -----
*                       USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                       USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                       USBH_ERR_HC_IO,                         Root hub input/output error.
*                       USBH_ERR_EP_STALL,                      Root hub does not support request.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : Length of string descriptor.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_StrDescGet (USBH_DEV    *p_dev,
                                     CPU_INT08U   desc_ix,
                                     CPU_INT16U   lang_id,
                                     void        *p_buf,
                                     CPU_INT32U   buf_len,
                                     USBH_ERR    *p_err)
{
    CPU_INT32U      len;
    CPU_INT32U      req_len;
    CPU_INT08U      i;
    USBH_DESC_HDR  *p_hdr;


    if(desc_ix == USBH_STRING_DESC_LANGID) {
        req_len = 0x04u;                                        /* Size of lang ID = 4.                                 */
    } else {
        req_len = USBH_LEN_DESC_HDR;
    }
    req_len = DEF_MIN(req_len, buf_len);

    for (i = 0u; i < USBH_CFG_STD_REQ_RETRY; i++) {             /* Retry up to 3 times.                                 */
        len = USBH_CtrlRx(p_dev,
                          USBH_REQ_GET_DESC,
                          USBH_REQ_DIR_DEV_TO_HOST  | USBH_REQ_RECIPIENT_DEV,
                         (USBH_DESC_TYPE_STR << 8u) | desc_ix,
                          lang_id,
                          p_buf,
                          req_len,
                          USBH_CFG_STD_REQ_TIMEOUT,
                          p_err);
        if ((len    == 0u               ) ||
            (*p_err == USBH_ERR_EP_STALL)) {
            USBH_EP_Reset(p_dev, (USBH_EP *)0);                 /* Rst EP to clr HC halt state.                         */
        } else {
            break;
        }
    }

    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    p_hdr = (USBH_DESC_HDR *)p_buf;

    if ((len                    == req_len           ) &&       /* Chk desc hdr.                                        */
        (p_hdr->bLength         != 0u                ) &&
        (p_hdr->bDescriptorType == USBH_DESC_TYPE_STR)) {

        len = p_hdr->bLength;
        if (desc_ix == USBH_STRING_DESC_LANGID) {
            return (len);
        }
    } else {
       *p_err = USBH_ERR_DESC_INVALID;
        return (0u);
    }

    if (len > buf_len) {
        len = buf_len;
    }
                                                                /* Get full str desc.                                   */
    for (i = 0u; i < USBH_CFG_STD_REQ_RETRY; i++) {             /* Retry up to 3 times.                                 */
        len = USBH_CtrlRx(p_dev,
                          USBH_REQ_GET_DESC,
                          USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_RECIPIENT_DEV,
                         (USBH_DESC_TYPE_STR << 8) | desc_ix,
                          lang_id,
                          p_buf,
                          len,
                          USBH_CFG_STD_REQ_TIMEOUT,
                          p_err);

        if ((len    == 0u               ) ||
            (*p_err == USBH_ERR_EP_STALL)) {
            USBH_EP_Reset(p_dev, (USBH_EP *)0);
        } else {
            break;
        }
    }

    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    if (len == 0u) {
       *p_err = USBH_ERR_DESC_INVALID;
        return (0u);
    }

    return (len);
}



/*
*********************************************************************************************************
*                                            USBH_StrDescPrint()
*
* Description : Print specified string index to default output terminal.
*
* Argument(s) : p_dev           Pointer to USB device.
*
*               str_prefix      Caller's specific prefix string to add.
*
*               desc_ix         String descriptor index.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
static  void  USBH_StrDescPrint (USBH_DEV    *p_dev,
                                 CPU_INT08U  *str_prefix,
                                 CPU_INT08U   desc_ix)
{
    USBH_ERR    err;
    CPU_INT32U  str_len;
    CPU_INT08U  str[USBH_CFG_MAX_STR_LEN];
    CPU_INT16U  ch;
    CPU_INT32U  ix;
    CPU_INT32U  buf_len;


    str_len = USBH_StrGet(p_dev,
                          desc_ix,
                          USBH_STRING_DESC_LANGID,
                         &str[0],
                          USBH_CFG_MAX_STR_LEN,
                         &err);


    USBH_PRINT_LOG("%s", str_prefix);                           /* Print prefix str.                                    */

    if (str_len > 0u) {                                         /* Print unicode string rd from the dev.                */
        buf_len = str_len * 2u;
        for (ix = 0u; (buf_len - ix) >= 2u; ix += 2u) {
            ch = MEM_VAL_GET_INT16U_LITTLE(&str[ix]);
            if (ch == 0u) {
                break;
            }
            USBH_PRINT_LOG("%c", ch);
        }
    }
    USBH_PRINT_LOG("\r\n");
}
#endif

/*
*********************************************************************************************************
*                                         USBH_NextDescGet()
*
* Description : Get pointer to next descriptor in buffer that contains configuration data.
*
* Argument(s) : p_buf       Pointer to current header in buffer.
*
*               p_off       Pointer to buffer offset.
*
* Return(s)   : Pointer to next descriptor header.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_DESC_HDR  *USBH_NextDescGet (void        *p_buf,
                                          CPU_INT32U  *p_offset)
{
    USBH_DESC_HDR  *p_next_hdr;
    USBH_DESC_HDR  *p_hdr;


    p_hdr = (USBH_DESC_HDR *)p_buf;                             /* Current desc hdr.                                    */

    if (*p_offset == 0u)  {                                     /* 1st desc in buf.                                     */
        p_next_hdr = p_hdr;
    } else {                                                    /* Next desc is at end of current desc.                 */
        p_next_hdr = (USBH_DESC_HDR *)((CPU_INT08U *)p_buf + p_hdr->bLength);
    }

   *p_offset += p_hdr->bLength;                                 /* Update buf offset.                                   */

    return (p_next_hdr);
}


/*
*********************************************************************************************************
*                                          USBH_FmtSetupReq()
*
* Description : Format setup request from setup request structure.
*
* Argument(s) : p_setup_req      Variable that holds setup request information.
*
*               p_buf_dest       Pointer to buffer that will receive setup request.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FmtSetupReq (USBH_SETUP_REQ  *p_setup_req,
                                void            *p_buf_dest)
{
    USBH_SETUP_REQ  *p_buf_dest_setup_req;


    p_buf_dest_setup_req = (USBH_SETUP_REQ *)p_buf_dest;

    p_buf_dest_setup_req->bmRequestType = p_setup_req->bmRequestType;
    p_buf_dest_setup_req->bRequest      = p_setup_req->bRequest;
    p_buf_dest_setup_req->wValue        = MEM_VAL_GET_INT16U_LITTLE(&p_setup_req->wValue);
    p_buf_dest_setup_req->wIndex        = MEM_VAL_GET_INT16U_LITTLE(&p_setup_req->wIndex);
    p_buf_dest_setup_req->wLength       = MEM_VAL_GET_INT16U_LITTLE(&p_setup_req->wLength);
}


/*
*********************************************************************************************************
*                                          USBH_ParseDevDesc()
*
* Description : Parse device descriptor into device descriptor structure.
*
* Argument(s) : p_dev_desc       Variable that will hold parsed device descriptor.
*
*               p_buf_src        Pointer to buffer that holds device descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ParseDevDesc (USBH_DEV_DESC  *p_dev_desc,
                                 void           *p_buf_src)
{
    USBH_DEV_DESC  *p_buf_src_dev_desc;


    p_buf_src_dev_desc = (USBH_DEV_DESC *)p_buf_src;

    p_dev_desc->bLength            = p_buf_src_dev_desc->bLength;
    p_dev_desc->bDescriptorType    = p_buf_src_dev_desc->bDescriptorType;
    p_dev_desc->bcdUSB             = MEM_VAL_GET_INT16U_LITTLE(&p_buf_src_dev_desc->bcdUSB);
    p_dev_desc->bDeviceClass       = p_buf_src_dev_desc->bDeviceClass;
    p_dev_desc->bDeviceSubClass    = p_buf_src_dev_desc->bDeviceSubClass;
    p_dev_desc->bDeviceProtocol    = p_buf_src_dev_desc->bDeviceProtocol;
    p_dev_desc->bMaxPacketSize0    = p_buf_src_dev_desc->bMaxPacketSize0;
    p_dev_desc->idVendor           = MEM_VAL_GET_INT16U_LITTLE(&p_buf_src_dev_desc->idVendor);
    p_dev_desc->idProduct          = MEM_VAL_GET_INT16U_LITTLE(&p_buf_src_dev_desc->idProduct);
    p_dev_desc->bcdDevice          = MEM_VAL_GET_INT16U_LITTLE(&p_buf_src_dev_desc->bcdDevice);
    p_dev_desc->iManufacturer      = p_buf_src_dev_desc->iManufacturer;
    p_dev_desc->iProduct           = p_buf_src_dev_desc->iProduct;
    p_dev_desc->iSerialNumber      = p_buf_src_dev_desc->iSerialNumber;
    p_dev_desc->bNbrConfigurations = p_buf_src_dev_desc->bNbrConfigurations;
}


/*
*********************************************************************************************************
*                                          USBH_ParseCfgDesc()
*
* Description : Parse configuration descriptor into configuration descriptor structure.
*
* Argument(s) : p_cfg_desc       Variable that will hold parsed configuration descriptor.
*
*               p_buf_src        Pointer to buffer that holds configuration descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ParseCfgDesc (USBH_CFG_DESC  *p_cfg_desc,
                                 void           *p_buf_src)
{
    USBH_CFG_DESC  *p_buf_src_cfg_desc;


    p_buf_src_cfg_desc = (USBH_CFG_DESC *)p_buf_src;

    p_cfg_desc->bLength             = p_buf_src_cfg_desc->bLength;
    p_cfg_desc->bDescriptorType     = p_buf_src_cfg_desc->bDescriptorType;
    p_cfg_desc->wTotalLength        = MEM_VAL_GET_INT16U_LITTLE(&p_buf_src_cfg_desc->wTotalLength);
    p_cfg_desc->bNbrInterfaces      = p_buf_src_cfg_desc->bNbrInterfaces;
    p_cfg_desc->bConfigurationValue = p_buf_src_cfg_desc->bConfigurationValue;
    p_cfg_desc->iConfiguration      = p_buf_src_cfg_desc->iConfiguration;
    p_cfg_desc->bmAttributes        = p_buf_src_cfg_desc->bmAttributes;
    p_cfg_desc->bMaxPower           = p_buf_src_cfg_desc->bMaxPower;
}


/*
*********************************************************************************************************
*                                         USBH_ParseIF_Desc()
*
* Description : Parse interface descriptor into interface descriptor structure.
*
* Argument(s) : p_if_desc        Variable that will hold parsed interface descriptor.
*
*               p_buf_src        Pointer to buffer that holds interface descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ParseIF_Desc (USBH_IF_DESC  *p_if_desc,
                                 void          *p_buf_src)
{
    USBH_IF_DESC  *p_buf_src_if_desc;


    p_buf_src_if_desc = (USBH_IF_DESC *)p_buf_src;

    p_if_desc->bLength            = p_buf_src_if_desc->bLength;
    p_if_desc->bDescriptorType    = p_buf_src_if_desc->bDescriptorType;
    p_if_desc->bInterfaceNumber   = p_buf_src_if_desc->bInterfaceNumber;
    p_if_desc->bAlternateSetting  = p_buf_src_if_desc->bAlternateSetting;
    p_if_desc->bNbrEndpoints      = p_buf_src_if_desc->bNbrEndpoints;
    p_if_desc->bInterfaceClass    = p_buf_src_if_desc->bInterfaceClass;
    p_if_desc->bInterfaceSubClass = p_buf_src_if_desc->bInterfaceSubClass;
    p_if_desc->bInterfaceProtocol = p_buf_src_if_desc->bInterfaceProtocol;
    p_if_desc->iInterface         = p_buf_src_if_desc->iInterface;
}


/*
*********************************************************************************************************
*                                          USBH_ParseEP_Desc()
*
* Description : Parse endpoint descriptor into endpoint descriptor structure.
*
* Argument(s) : p_ep_desc        Variable that will hold the parsed endpoint descriptor.
*
*               p_buf_src        Pointer to buffer that holds endpoint descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ParseEP_Desc (USBH_EP_DESC  *p_ep_desc,
                                 void          *p_buf_src)
{
    USBH_EP_DESC  *p_buf_desc;


    p_buf_desc = (USBH_EP_DESC *)p_buf_src;
    p_ep_desc->bLength          = p_buf_desc->bLength;
    p_ep_desc->bDescriptorType  = p_buf_desc->bDescriptorType;
    p_ep_desc->bEndpointAddress = p_buf_desc->bEndpointAddress;
    p_ep_desc->bmAttributes     = p_buf_desc->bmAttributes;
    p_ep_desc->wMaxPacketSize   = MEM_VAL_GET_INT16U_LITTLE(&p_buf_desc->wMaxPacketSize);
    p_ep_desc->bInterval        = p_buf_desc->bInterval;
                                                                /* Following fields only relevant for isoc EPs.         */
    if ((p_ep_desc->bmAttributes & 0x03) == USBH_EP_TYPE_ISOC) {
        p_ep_desc->bRefresh      = p_buf_desc->bRefresh;
        p_ep_desc->bSynchAddress = p_buf_desc->bSynchAddress;
    }
}


/*
*********************************************************************************************************
*                                          USBH_AsyncTask()
*
* Description : Task that process asynchronous URBs
*
* Argument(s) : p_arg       Pointer to a variable (Here it is 0)
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_AsyncTask (void  *p_arg)
{
    USBH_URB  *p_urb;
    CPU_SR_ALLOC();


    (void)p_arg;

    while (DEF_TRUE) {
        (void)USBH_OS_SemWait(USBH_URB_Sem, 0u);                /* Wait for URBs processed by HC.                       */

        CPU_CRITICAL_ENTER();
        p_urb = (USBH_URB *)USBH_URB_HeadPtr;

        if (USBH_URB_HeadPtr == USBH_URB_TailPtr) {
            USBH_URB_HeadPtr = (USBH_URB *)0;
            USBH_URB_TailPtr = (USBH_URB *)0;
        } else {
            USBH_URB_HeadPtr = USBH_URB_HeadPtr->NxtPtr;
        }
        CPU_CRITICAL_EXIT();

        if (p_urb != (USBH_URB *)0) {
            USBH_URB_Complete(p_urb);
        }
    }
}

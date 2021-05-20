/*
*********************************************************************************************************
*                                             uC/USB-Host
*                                     The Embedded USB Host Stack
*
*                    Copyright 2004-2021 Silicon Laboratories Inc. www.silabs.com
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
*                                    HUMAN INTERFACE DEVICE CLASS
*
* Filename : usbh_hid.c
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_HID_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_hid.h"
#include  "usbh_hidparser.h"
#include  "../../Source/usbh_core.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_HID_MAX_DUR                               1020u
#define  USBH_HID_DUR_RESOLUTION                           4u
#define  USBH_HID_LEN_HID_DESC                             9u


/*
*********************************************************************************************************
*                                           SUBCLASS CODES
*
* Note(s) : (1) See 'Device Class Definition for Human Interface Devices (HID), 6/27/01, Version 1.11',
*               section 4.2 for more details about subclass codes.
*********************************************************************************************************
*/

#define  USBH_HID_SUBCLASS_CODE_NONE                    0x00u
#define  USBH_HID_SUBCLASS_CODE_BOOT_IF                 0x01u


/*
*********************************************************************************************************
*                                     CLASS-SPECIFIC DESCRIPTORS
*
* Note(s) : (1) See 'Device Class Definition for Human Interface Devices (HID), 6/27/01, Version 1.11',
*               section 7.1 for more details about class-specific descriptors.
*
*           (2) For a 'get descriptor' setup request, the low byte of the 'wValue' field may contain
*               one of these values.
*********************************************************************************************************
*/

#define  USBH_HID_DESC_TYPE_HID                         0x21u
#define  USBH_HID_DESC_TYPE_REPORT                      0x22u
#define  USBH_HID_DESC_TYPE_PHYSICAL                    0x23u


/*
*********************************************************************************************************
*                                       CLASS-SPECIFIC REQUESTS
*
* Note(s) : (1) See 'Device Class Definition for Human Interface Devices (HID), 6/27/01, Version 1.11',
*               section 7.2 for more details about class-s[ecific requests.
*
*           (2) The 'bRequest' field of a class-specific setup request may contain one of these values.
*********************************************************************************************************
*/

#define  USBH_HID_REQ_GET_REPORT                        0x01u
#define  USBH_HID_REQ_GET_IDLE                          0x02u
#define  USBH_HID_REQ_GET_PROTOCOL                      0x03u
#define  USBH_HID_REQ_SET_REPORT                        0x09u
#define  USBH_HID_REQ_SET_IDLE                          0x0Au
#define  USBH_HID_REQ_SET_PROTOCOL                      0x0Bu


/*
*********************************************************************************************************
*                                      GET REPORT REQUEST TYPES
*
* Note(s) : (1) See 'Device Class Definition for Human Interface Devices (HID), 6/27/01, Version 1.11',
*               section 7.2.1 for more details about get report request types.
*
*           (2) The upper byte of the 'wValue' field of a get report setup request may contain one of
*               these values.
*********************************************************************************************************
*/

#define  USBH_HID_REPORT_TYPE_IN                        0x01u
#define  USBH_HID_REPORT_TYPE_OUT                       0x02u
#define  USBH_HID_REPORT_TYPE_FEATURE                   0x03u


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

static  USBH_HID_DEV  USBH_HID_DevArr[USBH_HID_CFG_MAX_DEV];
static  MEM_POOL      USBH_HID_DevPool;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void         USBH_HID_GlobalInit       (USBH_ERR      *p_err);

static  void        *USBH_HID_ProbeIF          (USBH_DEV      *p_dev,
                                                USBH_IF       *p_if,
                                                USBH_ERR      *p_err);

static  void         USBH_HID_Suspend          (void          *p_class_dev);

static  void         USBH_HID_Resume           (void          *p_class_dev);

static  void         USBH_HID_Disconn          (void          *p_class_dev);

static  void         USBH_HID_DevClr           (USBH_HID_DEV  *p_hid_dev);

static  USBH_ERR     USBH_HID_DevLock          (USBH_HID_DEV  *p_hid_dev);

static  void         USBH_HID_DevUnlock        (USBH_HID_DEV  *p_hid_dev);

static  CPU_INT32U   USBH_HID_TxData           (USBH_HID_DEV  *p_hid_dev,
                                                CPU_INT08U     report_id,
                                                void          *p_buf,
                                                CPU_INT32U     buf_len,
                                                CPU_INT16U     timeout_ms,
                                                USBH_ERR      *p_err);

static  CPU_INT32U   USBH_HID_RxData           (USBH_HID_DEV  *p_hid_dev,
                                                CPU_INT08U     report_id,
                                                void          *p_buf,
                                                CPU_INT32U     buf_len,
                                                CPU_INT16U     timeout_ms,
                                                USBH_ERR      *p_err);

static  USBH_ERR     USBH_HID_MemReadHIDDesc   (USBH_HID_DEV  *p_hid_dev);

static  void         USBH_HID_IntrRxCB         (USBH_EP       *p_ep,
                                                void          *p_buf,
                                                CPU_INT32U     buf_len,
                                                CPU_INT32U     xfer_len,
                                                void          *p_arg,
                                                USBH_ERR       err);

static  void         USBH_HID_DispatchReport   (USBH_HID_DEV  *p_hid_dev,
                                                CPU_INT08U    *p_buf,
                                                CPU_INT32U     buf_len,
                                                CPU_INT32U     xfer_len,
                                                USBH_ERR       err);

static  USBH_ERR     USBH_HID_ProcessReportDesc(USBH_HID_DEV  *p_hid_dev);

static  USBH_ERR     USBH_HID_RxReportAsync    (USBH_HID_DEV  *p_hid_dev);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          HID CLASS DRIVER
*********************************************************************************************************
*/

USBH_CLASS_DRV  USBH_HID_ClassDrv = {
    (CPU_INT08U *)"HID",
                  USBH_HID_GlobalInit,
                  0,
                  USBH_HID_ProbeIF,
                  USBH_HID_Suspend,
                  USBH_HID_Resume,
                  USBH_HID_Disconn
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTION
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           USBH_HID_Init()
*
* Description : Initialize the HID device, read & parse report descriptor and create report ID list.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE                           If device successfully initialized.
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'p_hid_dev'.
*
*                                                       ----- RETURNED BY USBH_HID_ProcessReportDesc() : -----
*               USBH_ERR_DESC_ALLOC,                    Failed to allocate memory for descriptor.
*               USBH_ERR_HID_RD_PARSER_FAIL,            Cannot parse report descriptor.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               USBH_ERR_DESC_EXTRA_NOT_FOUND,          HID descriptor not found.
*               USBH_ERR_DESC_INVALID,                  HID descriptor contains invalid value(s).
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,                          If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
*                                                       ----- RETURNED BY USBH_HID_CreateReportID -----
*               USBH_ERR_ALLOC,                         if report ID cannot be allocated.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_Init (USBH_HID_DEV  *p_hid_dev)
{
    USBH_ERR  err;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_HID_ProcessReportDesc(p_hid_dev);                /* Read and parse report desc.                          */
    if (err == USBH_ERR_NONE) {
        err = USBH_HID_CreateReportID(p_hid_dev);               /* Create report ID list.                               */

        if (err == USBH_ERR_NONE) {
            p_hid_dev->MaxReportPtr = USBH_HID_MaxReport(p_hid_dev, USBH_HID_MAIN_ITEM_TAG_IN);
            p_hid_dev->IsInit       = DEF_TRUE;
        }
    }

    USBH_HID_DevUnlock(p_hid_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_HID_RefAdd()
*
* Description : Increment the application reference count to HID device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE               If access to device is successful.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_hid_dev'.
*
*                                           ----- RETURNED BY USBH_OS_MutexLock() : -----
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,          If mutex wait aborted.
*               USBH_ERR_OS_FAIL,           Otherwise.
*
*                                           ----- RETURNED BY USBH_OS_MutexUnlock() : -----
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_FAIL,           Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_RefAdd (USBH_HID_DEV  *p_hid_dev)
{
    USBH_ERR  err;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_OS_MutexLock(p_hid_dev->HMutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    p_hid_dev->AppRefCnt++;

    err = USBH_OS_MutexUnlock(p_hid_dev->HMutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_HID_RefRel()
*
* Description : Decrement reference count of a HID device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE               If reference release is successful.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_hid_dev'.
*
*                                           ----- RETURNED BY USBH_OS_MutexLock() : -----
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,          If mutex wait aborted.
*               USBH_ERR_OS_FAIL,           Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_RefRel (USBH_HID_DEV  *p_hid_dev)
{
    USBH_ERR  err;
    LIB_ERR   err_lib;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_OS_MutexLock(p_hid_dev->HMutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->AppRefCnt > 0) {
        p_hid_dev->AppRefCnt--;

        if ((p_hid_dev->AppRefCnt == 0                           ) &&
            (p_hid_dev->State     == USBH_CLASS_DEV_STATE_DISCONN)) {
            (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);
            Mem_PoolBlkFree(       &USBH_HID_DevPool,           /* If no more ref on dev, release it.                   */
                            (void *)p_hid_dev,
                                   &err_lib);

            return (USBH_ERR_NONE);
        }
    }

    (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                     USBH_HID_GetReportIDArray()
*
* Description : Returns report id structure array and number of report id structures.
*
* Argument(s) : p_hid_dev           Pointer to HID device.
*
*               p_report_id         Pointer to variable that will receive the report ID structure
*                                   array base.
*
*               p_nbr_report_id     Pointer to variable that will receive the number of report ID
*                                   structures.
*
* Return(s)   : USBH_ERR_NONE               If report ID array successfully retrieved.
*               USBH_ERR_DEV_NOT_READY      If device is not ready.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_hid_dev'.
*
*                                           ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,              If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,     If HID device not ready.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_GetReportIDArray (USBH_HID_DEV         *p_hid_dev,
                                     USBH_HID_REPORT_ID  **p_report_id,
                                     CPU_INT08U           *p_nbr_report_id)
{
    USBH_ERR  err;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_DEV_NOT_READY);
    }

   *p_report_id     = p_hid_dev->ReportID;                      /* Rtn report ID struct array base addr.                */
   *p_nbr_report_id = p_hid_dev->NbrReportID;                   /* Rtn nbr of report ID struct.                         */

    USBH_HID_DevUnlock(p_hid_dev);

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                     USBH_HID_GetAppCollArray()
*
* Description : Returns application collection structure array and number of application collection
*               structures.
*
* Argument(s) : p_hid_dev           Pointer to HID device.
*
*               p_app_coll          Pointer to the application collection structure array base.
*
*               p_nbr_app_coll      Pointer to number of application collection structures.
*
* Return(s)   : USBH_ERR_NONE               If application collection array successfully retrieved.
*               USBH_ERR_DEV_NOT_READY      If device is not ready.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_hid_dev'/'p_app_coll'/
*                                           'p_nbr_app_coll'.
*
*                                           ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,              If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,     If HID device not ready.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_GetAppCollArray (USBH_HID_DEV        *p_hid_dev,
                                    USBH_HID_APP_COLL  **p_app_coll,
                                    CPU_INT08U          *p_nbr_app_coll)
{
    USBH_ERR  err;


    if ((p_hid_dev      == (USBH_HID_DEV       *)0) ||
        (p_app_coll     == (USBH_HID_APP_COLL **)0) ||
        (p_nbr_app_coll == (CPU_INT08U         *)0)) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_DEV_NOT_READY);
    }

   *p_app_coll     = p_hid_dev->AppColl;                        /* Return  App collection structure array base address  */
   *p_nbr_app_coll = p_hid_dev->NbrAppColl;                     /* Return the number of App collection structure        */

    USBH_HID_DevUnlock(p_hid_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_HID_IsBootDev()
*
* Description : Test whether HID interface belongs to boot subclass.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               p_err           Pointer to variable that will receive the return error code from this function.
*
*                       USBH_ERR_NONE,              If information retrieved successfully.
*                       USBH_ERR_DEV_NOT_READY      If device is not ready.
*                       USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_hid_dev'.
*
*                                                   ----- RETURNED BY USBH_HID_DevLock -----
*                       USBH_ERR_NONE,              If HID device successfully locked.
*                       USBH_ERR_DEV_NOT_READY,     If HID device not ready.
*
* Return(s)   : DEF_TRUE        Device belongs to boot subclass.
*               DEF_FALSE       Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  USBH_HID_IsBootDev (USBH_HID_DEV  *p_hid_dev,
                                 USBH_ERR      *p_err)
{
    CPU_BOOLEAN  is_boot;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (DEF_FALSE);
    }

   *p_err = USBH_HID_DevLock(p_hid_dev);
    if (*p_err != USBH_ERR_NONE) {
        return (DEF_FALSE);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_DEV_NOT_READY);
    }

    is_boot = (p_hid_dev->SubClass == USBH_HID_SUBCLASS_CODE_BOOT_IF) ? DEF_TRUE : DEF_FALSE;

    USBH_HID_DevUnlock(p_hid_dev);

    return (is_boot);
}


/*
*********************************************************************************************************
*                                         USBH_HID_RxReport()
*
* Description : Receive input report from device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               report_id       Report id.
*
*               p_buf           Pointer to buffer that will receive the report.
*
*               buf_len         Buffer length, in octets.
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err           Variable that will receive the return error code from this function.
*                   USBH_ERR_NONE,                          Report successfully received.
*                   USBH_ERR_DEV_NOT_READY,                 Device not ready.
*                   USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_hid_dev'.
*
*                                                           ----- RETURNED BY USBH_HID_RxData() : -----
*                   USBH_ERR_NULL_PTR                       Invalid null pointer passed to 'p_buf'.
*                   USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                   USBH_ERR_EP_INVALID_TYPE                Endpoint type is not interrupt or direction is not IN.
*                   USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                   USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                   USBH_ERR_HC_IO,                         Root hub input/output error.
*                   USBH_ERR_EP_STALL,                      Root hub does not support request.
*                   Host controller drivers error code,     Otherwise.
*
*                                                           ----- RETURNED BY USBH_HID_DevLock -----
*                   USBH_ERR_NONE,                          If HID device successfully locked.
*                   USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : 1) p_buf contains only the report data without the report id.
*********************************************************************************************************
*/

CPU_INT08U  USBH_HID_RxReport (USBH_HID_DEV  *p_hid_dev,
                               CPU_INT08U     report_id,
                               void          *p_buf,
                               CPU_INT08U     buf_len,
                               CPU_INT16U     timeout_ms,
                               USBH_ERR      *p_err)
{
    CPU_INT08U  xfer_len;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

   *p_err = USBH_HID_DevLock(p_hid_dev);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
       *p_err = USBH_ERR_DEV_NOT_READY;
        return (0u);
    }

    xfer_len = (CPU_INT08U)USBH_HID_RxData(p_hid_dev,           /* Receive input report.                                */
                                           report_id,
                                           p_buf,
                                           buf_len,
                                           timeout_ms,
                                           p_err);

    USBH_HID_DevUnlock(p_hid_dev);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                         USBH_HID_TxReport()
*
* Description : Send the report to the device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               report_id       Report id.
*
*               p_buf           Pointer to the buffer that contains the report.
*
*               buf_len         Buffer length, in octets.
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err           Variable that will receive the return error code from this function.
*
*                           USBH_ERR_NONE                           Report successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_hid_dev'.
*                           USBH_ERR_DEV_NOT_READY                  Device is not ready.
*
*                                                                   ----- RETURNED BY USBH_HID_TxData() : -----
*                           USBH_ERR_NONE                           Report successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'buf_len'.
*                           USBH_ERR_NULL_PTR                       Invalid null pointer passed to 'p_buf'.
*                           USBH_ERR_CLASS_DRV_NOT_FOUND            Interface is not of HID type.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not interrupt or direction is not OUT.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_UNKNOWN                        Unknown error occurred.
*                           USBH_ERR_HC_IO,                         Root hub input/output error.
*                           USBH_ERR_EP_STALL,                      Root hub does not support request.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_HID_DevLock -----
*                           USBH_ERR_NONE,                          If HID device successfully locked.
*                           USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
* Return(s)   : Number of octets sent.
*
* Note(s)     : 1) Do not add the report id to p_buf, it will be added automatically.
*********************************************************************************************************
*/

CPU_INT08U  USBH_HID_TxReport (USBH_HID_DEV  *p_hid_dev,
                               CPU_INT08U     report_id,
                               void          *p_buf,
                               CPU_INT08U     buf_len,
                               CPU_INT16U     timeout_ms,
                               USBH_ERR      *p_err)
{
    CPU_INT08U  xfer_len;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

   *p_err = USBH_HID_DevLock(p_hid_dev);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
       *p_err = USBH_ERR_DEV_NOT_READY;
        return (0u);
    }

    xfer_len = (CPU_INT08U)USBH_HID_TxData(p_hid_dev,           /* Send the output report.                              */
                                           report_id,
                                           p_buf,
                                           buf_len,
                                           timeout_ms,
                                           p_err);

    USBH_HID_DevUnlock(p_hid_dev);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                         USBH_HID_RegRxCB()
*
* Description : Register a callback function to receive reports from device asynchronously.
*
* Argument(s) : p_hid_dev        Pointer to HID device.
*
*               report_id        Report id.
*
*               async_fnct       Callback function.
*
*               p_async_arg      Pointer to context that will be passed to callback function.
*
* Return(s)   : USBH_ERR_NONE,                  If callback successfully registered.
*               USBH_ERR_INVALID_ARG,           If invalid arguemnt passed to 'async_fnct'/'p_hid_dev'.
*               USBH_ERR_DEV_NOT_READY,         If device is not ready.
*               USBH_ERR_HID_NOT_IN_REPORT,     If incorrect report descriptor provided by device.
*               USBH_ERR_ALLOC,                 If rx callback cannot be allocated.
*               USBH_ERR_HID_REPORT_ID,         If callback with same report ID is already registered.
*
*                                               ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,                  If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,         If HID device not ready.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_RegRxCB (USBH_HID_DEV        *p_hid_dev,
                            CPU_INT08U           report_id,
                            USBH_HID_RXCB_FNCT   async_fnct,
                            void                *p_async_arg)
{
    CPU_INT08U      ix;
    CPU_INT32U      report_len_bytes;
    USBH_HID_RXCB  *p_rx_cb;
    USBH_ERR        err;


    if ((p_hid_dev  == (USBH_HID_DEV     *)0) ||
        (async_fnct == (USBH_HID_RXCB_FNCT)0)) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_DEV_NOT_READY);
    }

    if (p_hid_dev->MaxReportPtr == (USBH_HID_REPORT_ID *)0) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_HID_NOT_IN_REPORT);
    }
                                                                /* Check max buf size to recv data.                     */
    report_len_bytes = (p_hid_dev->MaxReportPtr->Size / 8u);
    if ((p_hid_dev->MaxReportPtr->Size % 8u) != 0u) {
        report_len_bytes++;
    }

    if (report_len_bytes > USBH_HID_CFG_MAX_RX_BUF_SIZE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_ALLOC);
    }

    for (ix = 0u; ix < USBH_HID_CFG_MAX_NBR_RXCB; ix++) {       /* Check already reg callback with same report id.      */

        if ((p_hid_dev->RxCB[ix].InUse    == DEF_TRUE) &&
            (p_hid_dev->RxCB[ix].ReportID == report_id)) {
            USBH_HID_DevUnlock(p_hid_dev);
            return (USBH_ERR_HID_REPORT_ID);
        }
    }

    p_rx_cb = (USBH_HID_RXCB *)0;                               /* Search for empty callback structure.                 */
    for (ix = 0u; ix < USBH_HID_CFG_MAX_NBR_RXCB; ix++) {

        if (p_hid_dev->RxCB[ix].InUse == DEF_FALSE) {
            p_rx_cb = &p_hid_dev->RxCB[ix];
            break;
        }
    }

    if (p_rx_cb == (USBH_HID_RXCB *)0) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_ALLOC);
    }
                                                                /* Fill callback structure.                             */
    p_rx_cb->ReportID    = report_id;
    p_rx_cb->AsyncFnct   = async_fnct;
    p_rx_cb->AsyncArgPtr = p_async_arg;
    p_rx_cb->InUse       = DEF_TRUE;

    if (p_hid_dev->RxInProg == DEF_FALSE) {                     /* Receive Async if not started                         */
        err = USBH_HID_RxReportAsync(p_hid_dev);
    } else {
        err = USBH_ERR_NONE;
    }

    USBH_HID_DevUnlock(p_hid_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_HID_UnregRxCB()
*
* Description : Unregisters the callback function for the given report ID.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               report_id       Report id
*
* Return(s)   : USBH_ERR_NONE                           If success
*               USBH_ERR_DEV_NOT_READY                  If the device is not ready
*               USBH_ERR_HID_REPORTID_NOT_REGISTERED    If report ID is not registered
*
*                                                       ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,                          If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_UnregRxCB (USBH_HID_DEV  *p_hid_dev,
                              CPU_INT08U     report_id)
{
    CPU_INT08U  ix;
    USBH_ERR    err;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_DEV_NOT_READY);
    }

    for (ix = 0u; ix < USBH_HID_CFG_MAX_NBR_RXCB; ix++) {       /* Search and close callback structure                  */

        if ((p_hid_dev->RxCB[ix].InUse == DEF_TRUE    ) &&
            (p_hid_dev->RxCB[ix].ReportID == report_id)) {

            p_hid_dev->RxCB[ix].InUse = DEF_FALSE;
            USBH_HID_DevUnlock(p_hid_dev);

            return (USBH_ERR_NONE);
        }
    }

    USBH_HID_DevUnlock(p_hid_dev);

    return (USBH_ERR_HID_REPORT_ID);
}


/*
*********************************************************************************************************
*                                       USBH_HID_ProtocolSet()
*
* Description : Set protocol (boot/report descriptor) of HID device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               protocol        Protocol to set.
*
* Return(s)   : USBH_ERR_NONE,                          If protocol is successfully set.
*               USBH_ERR_DEV_NOT_READY,                 If device is not ready.
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'p_hid_dev'.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,                          If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
* Note(s)     : For more information on the 'Set_Protocol' request, see 'Device Class Definition for
*               Human Interface Devices (HID), 6/27/01, Version 1.11, section 7.2.6'.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_ProtocolSet (USBH_HID_DEV  *p_hid_dev,
                                CPU_INT16U     protocol)
{
    USBH_ERR  err;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_DEV_NOT_READY);
    }

    (void)USBH_CtrlTx(        p_hid_dev->DevPtr,                /* Send Set protocol request.                           */
                              USBH_HID_REQ_SET_PROTOCOL,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                              protocol,
                              p_hid_dev->IfNbr,
                      (void *)0,
                              0u,
                              USBH_CFG_STD_REQ_TIMEOUT,
                             &err);
    if (err == USBH_ERR_NONE) {
        p_hid_dev->Boot = DEF_TRUE;
    }

    USBH_HID_DevUnlock(p_hid_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                       USBH_HID_ProtocolGet()
*
* Description : Get protocol (boot/report) of the HID device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               p_protocol      Variable that will receive protocol of the device.
*
* Return(s)   : USBH_ERR_NONE,                          If protocol was successfully retrieved.
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'p_hid_dev'.
*               USBH_ERR_DEV_NOT_READY,                 If device is not ready.
*
*                                                       ----- RETURNED BY USBH_CtrlRx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,                          If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
* Note(s)     : For more information on the 'Get_Protocol' request, see 'Device Class Definition for
*               Human Interface Devices (HID), 6/27/01, Version 1.11, section 7.2.5'.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_ProtocolGet (USBH_HID_DEV  *p_hid_dev,
                                CPU_INT16U    *p_protocol)
{
    USBH_ERR  err;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
        return (USBH_ERR_DEV_NOT_READY);
    }

    (void)USBH_CtrlRx(        p_hid_dev->DevPtr,                /* Send Get protocol request.                           */
                              USBH_HID_REQ_GET_PROTOCOL,
                             (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                              0u,
                              p_hid_dev->IfNbr,
                      (void *)p_protocol,
                              1u,
                              USBH_CFG_STD_REQ_TIMEOUT,
                             &err);

    USBH_HID_DevUnlock(p_hid_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_HID_IdleSet()
*
* Description : Set idle state duration for a given report ID.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               report_id       Report id.
*
*               dur             Duration of the idle state in milliseconds.
*
* Return(s)   : USBH_ERR_NONE,                          If duration is successfully set.
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'p_hid_dev'/'dur'.
*               USBH_ERR_DEV_NOT_READY.                 If device is not ready.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_HID_DevLock -----
*               USBH_ERR_NONE,                          If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
* Note(s)     : For more information on the 'Set_Idle' request, see 'Device Class Definition for
*               Human Interface Devices (HID), 6/27/01, Version 1.11, section 7.2.4'.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_IdleSet (USBH_HID_DEV  *p_hid_dev,
                            CPU_INT08U     report_id,
                            CPU_INT32U     dur)
{
    USBH_ERR  err;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    if (dur > USBH_HID_MAX_DUR) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HID_DevLock(p_hid_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    dur = dur / USBH_HID_DUR_RESOLUTION;                        /* Convert into resolution units.                       */
                                                                /* Send class-specific request.                         */
    (void)USBH_CtrlTx(        p_hid_dev->DevPtr,
                              USBH_HID_REQ_SET_IDLE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                             (((dur << 8u) & 0xFF00u) | report_id),
                              p_hid_dev->IfNbr,
                      (void *)0,
                              0u,
                              USBH_CFG_STD_REQ_TIMEOUT,
                             &err);
    if (err == USBH_ERR_EP_STALL) {
        (void)USBH_EP_Reset(           p_hid_dev->DevPtr,
                            (USBH_EP *)0);
    }

    USBH_HID_DevUnlock(p_hid_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_HID_IdleGet()
*
* Description : Get idle duration for given report ID.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               report_id       Report id.
*
*               p_err           Variable that will receive the return error code from this function.
*
*                   USBH_ERR_NONE                           Idle duration successfully retrieved.
*                   USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_hid_dev'.
*                   USBH_ERR_DEV_NOT_READY                  Device is not ready.
*
*                                                           ----- RETURNED BY USBH_CtrlRx() : -----
*                   USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                   USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                   USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                   USBH_ERR_HC_IO,                         Root hub input/output error.
*                   USBH_ERR_EP_STALL,                      Root hub does not support request.
*                   Host controller drivers error code,     Otherwise.
*
*                                                           ----- RETURNED BY USBH_HID_DevLock -----
*                   USBH_ERR_NONE,                          If HID device successfully locked.
*                   USBH_ERR_DEV_NOT_READY,                 If HID device not ready.
*
* Return(s)   : Idle duration in milliseconds.
*
* Note(s)     : For more information on the 'Get_Idle' request, see 'Device Class Definition for
*               Human Interface Devices (HID), 6/27/01, Version 1.11, section 7.2.3'.
*********************************************************************************************************
*/

CPU_INT32U  USBH_HID_IdleGet (USBH_HID_DEV  *p_hid_dev,
                              CPU_INT08U     report_id,
                              USBH_ERR      *p_err)
{
    CPU_INT32U  dur;


    if (p_hid_dev == (USBH_HID_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

   *p_err = USBH_HID_DevLock(p_hid_dev);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    if (p_hid_dev->IsInit == DEF_FALSE) {
        USBH_HID_DevUnlock(p_hid_dev);
       *p_err = USBH_ERR_DEV_NOT_READY;
        return (0u);
    }

    (void)USBH_CtrlRx(         p_hid_dev->DevPtr,               /* Send the class-specific request.                     */
                               USBH_HID_REQ_GET_IDLE,
                              (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                               report_id,
                               p_hid_dev->IfNbr,
                      (void *)&dur,
                               1u,
                               USBH_CFG_STD_REQ_TIMEOUT,
                               p_err);

    dur = (dur) * USBH_HID_DUR_RESOLUTION;                      /* Convert to ms.                             */

    USBH_HID_DevUnlock(p_hid_dev);

    return (dur);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTION
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        USBH_HID_GlobalInit()
*
* Description : This function is used to initialize the global variables
*
* Argument(s) : p_err       Variable that will receive the return error code from this function.
*                           USBH_ERR_NONE,                  Idle duration successfully retrieved.
*                           USBH_ERR_INVALID_ARG,           Invalid argument passed to 'p_hid_dev'.
*                           USBH_ERR_DEV_NOT_READY,         Device is not ready.
*
*                                                           ----- RETURNED BY USBH_HID_ParserGlobalInit -----
*                           USBH_ERR_ALLOC,                 if global item or collection list allocation failed.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_GlobalInit (USBH_ERR  *p_err)
{
    CPU_INT08U  ix;
    LIB_ERR     err_lib;
    CPU_SIZE_T  octets_reqd;

                                                                /* --------------- INIT HID DEV STRUCT ---------------- */
    for (ix = 0u; ix < USBH_HID_CFG_MAX_DEV; ix++) {
        (void)USBH_OS_MutexCreate(&USBH_HID_DevArr[ix].HMutex); /* Mutex for protection from multiple app access.       */
    }

    Mem_PoolCreate (       &USBH_HID_DevPool,                   /* POOL for managing HID dev struct.                    */
                    (void *)USBH_HID_DevArr,
                            sizeof(USBH_HID_DEV) * USBH_HID_CFG_MAX_DEV,
                            USBH_HID_CFG_MAX_DEV,
                            sizeof(USBH_HID_DEV),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

   *p_err = USBH_HID_ParserGlobalInit();                        /* Init HID parser struct.                              */
}


/*
*********************************************************************************************************
*                                         USBH_HID_ProbeIF()
*
* Description : Determine if the interface is HID class interface.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to interface.
*
*               p_err       Variable that will receive the return error code from this function.
*                           USBH_ERR_NONE                       Interface is of HID type.
*                           USBH_ERR_DEV_ALLOC                  Cannot allocate HID device.
*                           USBH_ERR_CLASS_DRV_NOT_FOUND        Interface is not of HID type.
*
*                                                               ----- RETURNED BY USBH_IF_DescGet() : -----
*                           USBH_ERR_INVALID_ARG,               Invalid argument passed to 'alt_ix'.
*
*                                                               ----- RETURNED BY USBH_IntrInOpen() : -----
*                           USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*                           USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*                           USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*                           Host controller drivers error,      Otherwise.
*
*                                                               ----- RETURNED BY USBH_IntrOutOpen() : -----
*                           USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*                           USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*                           USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*                           Host controller drivers error,      Otherwise.
*
* Return(s)   : p_hid_dev,      If device has a HID class interface.
*               0,              Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  *USBH_HID_ProbeIF (USBH_DEV  *p_dev,
                                 USBH_IF   *p_if,
                                 USBH_ERR  *p_err)
{
    USBH_IF_DESC   p_if_desc;
    USBH_HID_DEV  *p_hid_dev;
    LIB_ERR        err_lib;


   *p_err = USBH_IF_DescGet(p_if, 0u, &p_if_desc);              /* Get IF desc from IF.                                 */
    if (*p_err != USBH_ERR_NONE) {
        return ((void *)0);
    }
                                                                /* Check for HID class code.                            */
    if (p_if_desc.bInterfaceClass == USBH_CLASS_CODE_HID){
                                                                /* Alloc a dev from HID dev pool.                       */
        p_hid_dev = (USBH_HID_DEV *)Mem_PoolBlkGet(           &USBH_HID_DevPool,
                                                   (CPU_SIZE_T)sizeof(USBH_HID_DEV),
                                                              &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
           *p_err = USBH_ERR_DEV_ALLOC;
            return ((void *)0);
        }

        USBH_HID_DevClr(p_hid_dev);
        p_hid_dev->State    = USBH_CLASS_DEV_STATE_CONN;
        p_hid_dev->RxInProg = DEF_FALSE;
        p_hid_dev->IsInit   = DEF_FALSE;
        p_hid_dev->DevPtr   = p_dev;
        p_hid_dev->IfPtr    = p_if;
        p_hid_dev->IfNbr    = p_if_desc.bInterfaceNumber;
        p_hid_dev->SubClass = p_if_desc.bInterfaceSubClass;
        p_hid_dev->Protocol = p_if_desc.bInterfaceProtocol;

       *p_err = USBH_IntrInOpen(p_hid_dev->DevPtr,              /* Open intr IN EP.                                     */
                                p_hid_dev->IfPtr,
                               &p_hid_dev->IntrInEP);
        if (*p_err != USBH_ERR_NONE) {
            Mem_PoolBlkFree(       &USBH_HID_DevPool,
                            (void *)p_hid_dev,
                                   &err_lib);
            return ((void *)0);
        }

       *p_err = USBH_IntrOutOpen(p_hid_dev->DevPtr,             /* Try to open optional intr OUT EP.                    */
                                 p_hid_dev->IfPtr,
                                &p_hid_dev->IntrOutEP);
        if (*p_err != USBH_ERR_NONE) {
            p_hid_dev->IsOutEP_Present = DEF_FALSE;
           *p_err                      = USBH_ERR_NONE;
        } else {
            p_hid_dev->IsOutEP_Present = DEF_TRUE;
        }
    } else {
        p_hid_dev = (USBH_HID_DEV *)0;
       *p_err     =  USBH_ERR_CLASS_DRV_NOT_FOUND;
    }

    return ((void *)p_hid_dev);
}


/*
*********************************************************************************************************
*                                         USBH_HID_Disconn()
*
* Description : Handle disconnection of HID device.
*
* Argument(s) : p_class_dev     Pointer to HID device.
*
* Return(s)   : None.
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  USBH_HID_Disconn (void  *p_class_dev)
{
    LIB_ERR        err_lib;
    USBH_HID_DEV  *p_hid_dev;


    p_hid_dev = (USBH_HID_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_hid_dev->HMutex);

    p_hid_dev->State = USBH_CLASS_DEV_STATE_DISCONN;

                                                                /* -------------------- CLOSE EPS --------------------- */
    USBH_EP_Close(&p_hid_dev->IntrInEP);

    if (p_hid_dev->IsOutEP_Present == DEF_TRUE) {
        USBH_EP_Close(&p_hid_dev->IntrOutEP);
    }

    if (p_hid_dev->AppRefCnt == 0u) {                           /* Release HID dev if app reference count is zero.      */
        (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);
        Mem_PoolBlkFree(       &USBH_HID_DevPool,               /* App refcnt is 0 and Dev is removed, release HID dev. */
                        (void *)p_hid_dev,
                               &err_lib);
    }

    (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);
}


/*
*********************************************************************************************************
*                                         USBH_HID_Suspend()
*
* Description : Suspend HID device, waits for completion of any pending I/O.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_Suspend (void  *p_class_dev)
{
    USBH_HID_DEV  *p_hid_dev;


    p_hid_dev = (USBH_HID_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_hid_dev->HMutex);

    p_hid_dev->State = USBH_CLASS_DEV_STATE_SUSPEND;

    (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);
}


/*
*********************************************************************************************************
*                                          USBH_HID_Resume()
*
* Description : Resume HID device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_Resume (void  *p_class_dev)
{
    USBH_HID_DEV  *p_hid_dev;


    p_hid_dev = (USBH_HID_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_hid_dev->HMutex);

    p_hid_dev->State = USBH_CLASS_DEV_STATE_CONN;

    (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);
}


/*
*********************************************************************************************************
*                                          USBH_HID_DevClr()
*
* Description : Clear HID device structure.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_DevClr (USBH_HID_DEV  *p_hid_dev)
{
    USBH_HMUTEX  h_mutex;


    h_mutex = p_hid_dev->HMutex;                                /* Save mutex var.                                      */

    Mem_Clr((void *)p_hid_dev, sizeof(USBH_HID_DEV));

    p_hid_dev->State  = USBH_CLASS_DEV_STATE_NONE;
    p_hid_dev->HMutex = h_mutex;                                /* Restore mutex var.                                   */
}


/*
*********************************************************************************************************
*                                         USBH_HID_DevLock()
*
* Description : Lock HID device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE,              If HID device successfully locked.
*               USBH_ERR_DEV_NOT_READY,     If HID device not ready.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_DevLock (USBH_HID_DEV  *p_hid_dev)
{
    CPU_INT08U  state;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    state = p_hid_dev->State;
    CPU_CRITICAL_EXIT();

    if (state != USBH_CLASS_DEV_STATE_CONN) {
        return (USBH_ERR_DEV_NOT_READY);
    }

    (void)USBH_OS_MutexLock(p_hid_dev->HMutex);
    if (p_hid_dev->State != USBH_CLASS_DEV_STATE_CONN) {
        (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);
        return (USBH_ERR_DEV_NOT_READY);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HID_DevUnlock()
*
* Description : Unlock HID device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_DevUnlock (USBH_HID_DEV  *p_hid_dev)
{
    (void)USBH_OS_MutexUnlock(p_hid_dev->HMutex);
}


/*
*********************************************************************************************************
*                                          USBH_HID_TxData()
*
* Description : Send data (report) to HID device.
*
* Argument(s) : p_hid_dev        Pointer to HID device.
*
*               report_id        Report Id.
*
*               p_buf            Pointer to buffer that contains the report.
*
*               buf_len          Buffer length, in octets.
*
*               timeout_ms       Timeout, in milliseconds.
*
*               p_err       Variable that will receive the return error code from this function.
*
*                           USBH_ERR_NONE                           Report successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'buf_len'.
*                           USBH_ERR_NULL_PTR                       Invalid null pointer passed to 'p_buf'.
*                           USBH_ERR_CLASS_DRV_NOT_FOUND            Interface is not of HID type.
*
*                                                                   ----- RETURNED BY USBH_IntrTx() : -----
*                           USBH_ERR_NONE                           Interrupt transfer successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not interrupt or direction is not OUT.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_CtrlTx() : -----
*                           USBH_ERR_UNKNOWN                        Unknown error occurred.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_HC_IO,                         Root hub input/output error.
*                           USBH_ERR_EP_STALL,                      Root hub does not support request.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets sent.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_HID_TxData (USBH_HID_DEV  *p_hid_dev,
                                     CPU_INT08U     report_id,
                                     void          *p_buf,
                                     CPU_INT32U     buf_len,
                                     CPU_INT16U     timeout_ms,
                                     USBH_ERR      *p_err)
{
    CPU_INT32U  xfer_len;


    if ((p_buf   == (void *)0 ) ||
        (buf_len ==         0u)) {
        *p_err = USBH_ERR_NULL_PTR;
         return (0u);
    }
                                                                /* Use optional intr OUT EP if available.               */
    if (p_hid_dev->IsOutEP_Present == DEF_TRUE) {

        if (report_id != 0u) {                                  /* Copy report id and buf if report is non zero.        */

            if (buf_len > USBH_HID_CFG_MAX_TX_BUF_SIZE) {
               *p_err = USBH_ERR_INVALID_ARG;
                return (0u);
            }

            p_hid_dev->TxBuf[0] = report_id;                    /* First byte should be report id.                      */
            Mem_Copy((void *)&p_hid_dev->TxBuf[1],
                              p_buf,
                              buf_len);

            xfer_len = USBH_IntrTx(&p_hid_dev->IntrOutEP,
                                    p_hid_dev->TxBuf,
                                    buf_len + 1u,
                                    timeout_ms,
                                    p_err);
            if ((*p_err    == USBH_ERR_NONE) &&
                ( xfer_len != 0u           )) {
                xfer_len--;
            }
        } else {
            xfer_len = USBH_IntrTx(&p_hid_dev->IntrOutEP,
                                    p_buf,
                                    buf_len,
                                    timeout_ms,
                                    p_err);
        }
    } else {
                                                                /* Send report via ctrl EP if intr OUT not available.   */
        xfer_len = USBH_CtrlTx(p_hid_dev->DevPtr,
                               USBH_HID_REQ_SET_REPORT,
                              (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                              (((USBH_HID_REPORT_TYPE_OUT << 8u) & 0xFF00u) | report_id),
                               p_hid_dev->IfNbr,
                               p_buf,
                               buf_len,
                               timeout_ms,
                               p_err);
    }

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_HID_RxData()
*
* Description : Receive data (report) from device.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               report_id       Report id.
*
*               p_buf           Pointer to buffer that will receive the report.
*
*               buf_len         Buffer length, in octets.
*
*               timeout_ms      Timeout, in milliseconds.
*
*               p_err       Variable that will receive the return error code from this function.
*
*                           USBH_ERR_NONE                           Report successfully received.
*                           USBH_ERR_NULL_PTR                       Invalid null pointer passed to 'p_buf'.
*
*                                                                   ----- RETURNED BY USBH_IntrRx() : -----
*                           USBH_ERR_NONE                           Interrupt transfer successfully received.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not interrupt or direction is not IN.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_CtrlRx() : -----
*                           USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                           USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                           USBH_ERR_HC_IO,                         Root hub input/output error.
*                           USBH_ERR_EP_STALL,                      Root hub does not support request.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_HID_RxData (USBH_HID_DEV  *p_hid_dev,
                                     CPU_INT08U     report_id,
                                     void          *p_buf,
                                     CPU_INT32U     buf_len,
                                     CPU_INT16U     timeout_ms,
                                     USBH_ERR      *p_err)
{
    CPU_INT32U  xfer_len;


    if ((p_buf   == (void *)0 ) ||
        (buf_len ==         0u)) {
       *p_err = USBH_ERR_NULL_PTR;
        return (0u);
    }

    if ((report_id           !=       0u) ||                    /* If report ID 0 specified or intr IN EP busy, use ... */
        (p_hid_dev->RxInProg == DEF_TRUE)) {                    /* ... ctrl EP.                                         */
        xfer_len = USBH_CtrlRx(p_hid_dev->DevPtr,
                               USBH_HID_REQ_GET_REPORT,
                              (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                              (((USBH_HID_REPORT_TYPE_IN << 8u) & 0xFF00u) | report_id),
                               p_hid_dev->IfNbr,
                               p_buf,
                               buf_len,
                               timeout_ms,
                               p_err);
    } else {                                                    /* Rcv report on intr IN EP.                            */
        xfer_len = USBH_IntrRx(&p_hid_dev->IntrInEP,
                                p_buf,
                                buf_len,
                                timeout_ms,
                                p_err);
    }

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                      USBH_HID_MemReadHIDDesc()
*
* Description : Retrieve and parse HID descriptor.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE,                      HID descriptor successfully retrieved and parsed.
*               USBH_ERR_DESC_EXTRA_NOT_FOUND,      HID descriptor not found.
*               USBH_ERR_DESC_INVALID,              HID descriptor contains invalid value(s).
*
* Note(s)     : (1) The HID descriptor must have at least report descriptor type/length pair.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_MemReadHIDDesc (USBH_HID_DEV  *p_hid_dev)
{
    USBH_HID_DESC  *p_hid_desc;
    CPU_INT08U     *p_mem;
    CPU_INT08U      len;
    CPU_INT08U      b_type;
    CPU_INT16U      tot_len;
    CPU_INT16U      w_len;


                                                                /* Get HID desc from extra desc.                        */
    p_mem  = (CPU_INT08U *)USBH_IF_ExtraDescGet(p_hid_dev->IfPtr,
                                                0u,
                                               &tot_len);
    if (p_mem == (CPU_INT08U *)0) {
        return (USBH_ERR_DESC_EXTRA_NOT_FOUND);
    }

    p_hid_desc = &p_hid_dev->Desc;

    p_hid_desc->bLength = MEM_VAL_GET_INT08U_LITTLE((void *)p_mem);/* Get desc len.                                        */
    len                 = p_hid_desc->bLength;
    if (len < USBH_HID_LEN_HID_DESC) {                          /* Chk len.                                             */
        return (USBH_ERR_DESC_INVALID);
    }
    p_mem += sizeof(CPU_INT08U);
    len   -= sizeof(CPU_INT08U);
                                                                /* Get desc type.                                       */
    p_hid_desc->bDescriptorType = MEM_VAL_GET_INT08U_LITTLE((void *)p_mem);
    if (p_hid_desc->bDescriptorType != USBH_HID_DESC_TYPE_HID) {/* Type should be USBH_HID_DESC_TYPE_HID.               */
        return (USBH_ERR_DESC_INVALID);
    }
    p_mem += sizeof(CPU_INT08U);
    len   -= sizeof(CPU_INT08U);
                                                                /* -------------------- GET bcdHID -------------------- */
    p_hid_desc->bcdHID = MEM_VAL_GET_INT16U_LITTLE((void *)p_mem);
    p_mem += sizeof(CPU_INT16U);
    len  -= sizeof(CPU_INT16U);
                                                                /* ----------------- GET COUNTRY CODE ----------------- */
    p_hid_desc->bCountryCode = MEM_VAL_GET_INT08U_LITTLE((void *)p_mem);
    p_mem += sizeof(CPU_INT08U);
    len   -= sizeof(CPU_INT08U);
                                                                /* -------------- GET NBR OF CLASS DESC --------------- */
    p_hid_desc->bNbrDescriptors = MEM_VAL_GET_INT08U_LITTLE((void *)p_mem);
    p_mem += sizeof(CPU_INT08U);
    len   -= sizeof(CPU_INT08U);

    if (p_hid_desc->bNbrDescriptors < 1u) {                     /* Nbr of desc should be at least one.                  */
        return (USBH_ERR_DESC_INVALID);
    }
                                                                /* ----------------- GET REPORT DESC ------------------ */
    while (len > 0) {
        b_type = MEM_VAL_GET_INT08U_LITTLE((void *)p_mem);
        p_mem += sizeof(CPU_INT08U);
        len   -= sizeof(CPU_INT08U);

        w_len  = MEM_VAL_GET_INT16U_LITTLE((void *)p_mem);
        p_mem += sizeof(CPU_INT16U);
        len   -= sizeof(CPU_INT16U);
                                                                /* If this is a report desc ...                         */
        if ((b_type == USBH_HID_DESC_TYPE_REPORT) &&
            (w_len   > 0u                       )) {
            p_hid_desc->bDescriptorType        = b_type;
            p_hid_desc->wClassDescriptorLength = w_len;
            return (USBH_ERR_NONE);                             /* ... assign len & rtn.                                */
        }
    }

    return (USBH_ERR_DESC_EXTRA_NOT_FOUND);
}


/*
*********************************************************************************************************
*                                          USBH_HID_IntrRxCB()
*
* Description : Interrupt callback funtion :
*               (1) Check transfer status.
*               (2) Dispatch report.
*
* Argument(s) : p_ep           Pointer to endpoint.
*
*               p_buf          Pointer to receive buffer.
*
*               buf_len        Length of received buffer in octets.
*
*               xfer_len       Number of octets received.
*
*               p_arg          Context variable.
*
*               err            Receive status.
*
* Return(s)   : None.
*
* Note(s)     : (1) The bInterval period associated to periodic transfer can be managed by hardware
*                   (i.e. host controller) or by sofware (i.e. host stack).
*
*                   (a) Hardware: host controller will emit the periodic IN request every bInterval
*                       written into a register. Essentially, a list-based host controller will use this
*                       method (e.g. OHCI host controller).
*
*                   (b) Software: host controller doesn't provide a hardware management of the bInterval
*                       period. In this case, the stack will wait bInterval frame before re-submiting an
*                       IN request.
*
*               (2) If endpoint has no interrupt data to transmit when accessed by the host, it
*                   responds with NAK. Next polling from the Host will take place at the next period
*                   (i.e. bInterval of the endpoint).
*********************************************************************************************************
*/

static  void  USBH_HID_IntrRxCB (USBH_EP     *p_ep,
                                 void        *p_buf,
                                 CPU_INT32U   buf_len,
                                 CPU_INT32U   xfer_len,
                                 void        *p_arg,
                                 USBH_ERR     err)
{
    USBH_ERR       temp_err;
    USBH_HID_DEV  *p_hid_dev;
    USBH_ERR       lock_err;
    CPU_INT08U     state;
    CPU_INT08U     ix;
    CPU_SR_ALLOC();


    (void)p_ep;

    p_hid_dev = (USBH_HID_DEV *)p_arg;                          /* Get HID dev.                                         */

    if (p_hid_dev == (USBH_HID_DEV *)0) {
        return;
    }

    CPU_CRITICAL_ENTER();
    state = p_hid_dev->State;
    CPU_CRITICAL_EXIT();

    if (state != USBH_CLASS_DEV_STATE_CONN) {
        for (ix = 0u; ix < USBH_HID_CFG_MAX_NBR_RXCB; ix++) {   /* Notify all callback about state.                     */

            if (p_hid_dev->RxCB[ix].InUse == DEF_TRUE) {
                p_hid_dev->RxCB[ix].AsyncFnct((void *)p_hid_dev->RxCB[ix].AsyncArgPtr,
                                              (void *)0,
                                                      0u,
                                                      USBH_ERR_DEV_NOT_READY);
            }
        }

        return;
    }

    switch (err) {
        case USBH_ERR_EP_NACK:                                  /* NAK is not an error.  Device has just no valid data  */
             err      = USBH_ERR_NONE;
             temp_err = USBH_ERR_EP_NACK;
        case USBH_ERR_NONE:
             p_hid_dev->ErrCnt = 0u;
             break;

        case USBH_ERR_EP_INVALID_STATE:
        case USBH_ERR_HC_IO:
        case USBH_ERR_EP_STALL:
             USBH_EP_Reset(p_ep->DevPtr, p_ep);
             p_hid_dev->ErrCnt++;
             break;

        case USBH_ERR_URB_ABORT:
        default:
             p_hid_dev->ErrCnt = USBH_HID_CFG_MAX_ERR_CNT;
             break;
    }
                                                                /* Dispatch data and err to app.                        */
    USBH_HID_DispatchReport(p_hid_dev,
                            p_buf,
                            buf_len,
                            xfer_len,
                            err);

    lock_err = USBH_HID_DevLock(p_hid_dev);
    if (lock_err != USBH_ERR_NONE) {
        return;
    }

    if (p_hid_dev->ErrCnt < USBH_HID_CFG_MAX_ERR_CNT) {         /* If err cnt > MAX, do not resubmit.                   */

        if(((p_hid_dev->DevPtr->DevSpd == USBH_DEV_SPD_LOW ) ||
            (p_hid_dev->DevPtr->DevSpd == USBH_DEV_SPD_FULL))
           && (temp_err == USBH_ERR_EP_NACK)) {                 /* See Note #1.                                         */

            USBH_OS_DlyMS(p_hid_dev->IntrInEP.Desc.bInterval);  /* Wait bInterval period associated to IN endpoint      */
                                                                /* before resubmitting xfer. See Note #2.               */
        }

        USBH_HID_RxReportAsync(p_hid_dev);                      /* Resubmit xfer if dev is in operational state.        */
    }

    USBH_HID_DevUnlock(p_hid_dev);
}


/*
*********************************************************************************************************
*                                      USBH_HID_DispatchReport()
*
* Description : Dispatch reports received asynchronously from device:
*               (1) Identify report ID.
*               (2) Find callback for report ID.
*               (3) Invoke callback.
*
* Argument(s) : p_hid_dev      Pointer to HID device.
*
*               p_buf          Pointer to buffer containing.
*
*               buf_len        Length of report buffer in octets.
*
*               xfer_len       Number of octets received.
*
*               p_arg          Context variable.
*
*               err            Receive status.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_DispatchReport (USBH_HID_DEV  *p_hid_dev,
                                       CPU_INT08U    *p_buf,
                                       CPU_INT32U     buf_len,
                                       CPU_INT32U     xfer_len,
                                       USBH_ERR       err)
{
    CPU_INT08U           report_id;
    CPU_INT08U           ix;
    void                *p_arg;
    USBH_ERR             lock_err;
    USBH_HID_RXCB_FNCT   fnct;


    (void)buf_len;

    lock_err = USBH_HID_DevLock(p_hid_dev);
    if (lock_err != USBH_ERR_NONE) {
        return;
    }

    if (err != USBH_ERR_NONE) {
        for (ix = 0u; ix < USBH_HID_CFG_MAX_NBR_RXCB; ix++) {   /* Notify all reg's callback of err.                    */

            if (p_hid_dev->RxCB[ix].InUse == DEF_TRUE) {
                p_arg = p_hid_dev->RxCB[ix].AsyncArgPtr;
                fnct  = p_hid_dev->RxCB[ix].AsyncFnct;

                USBH_HID_DevUnlock(p_hid_dev);                  /* Unlock dev before invoking callback.                 */

                fnct(        p_arg,
                     (void *)0,
                             0u,
                             err);

                lock_err = USBH_HID_DevLock(p_hid_dev);
                if (lock_err != USBH_ERR_NONE) {
                    return;
                }
            }
        }

        USBH_HID_DevUnlock(p_hid_dev);
        return;
    }

    if (xfer_len == 0u) {                                       /* Nothing to dispatch.                                 */
        USBH_HID_DevUnlock(p_hid_dev);
        return;
    }

    if (p_hid_dev->Boot == DEF_TRUE) {                          /* If boot dev, report ID is 0.                         */
        report_id = 0u;
    } else {
        if (p_hid_dev->ReportID[0].ReportID == 0u) {            /* If first report ID is 0, there is no report ID.      */
            report_id = 0u;
        } else {                                                /* Otherwise, report ID is first byte of buf.           */
            report_id = p_buf[0];
        }
    }
                                                                /* Find callback with same report ID.                   */
    for (ix = 0u; ix < USBH_HID_CFG_MAX_NBR_RXCB; ix++) {
        if ((p_hid_dev->RxCB[ix].InUse    == DEF_TRUE ) &&
            (p_hid_dev->RxCB[ix].ReportID == report_id)) {

            p_arg = p_hid_dev->RxCB[ix].AsyncArgPtr;
            fnct  = p_hid_dev->RxCB[ix].AsyncFnct;

            USBH_HID_DevUnlock(p_hid_dev);                      /* Unlock dev before invoking callback.                 */

            if (report_id != 0u) {
                fnct(         p_arg,
                     (void *)&p_buf[1],
                              xfer_len - 1u,
                              err);
            } else {
                fnct(         p_arg,
                     (void *)&p_buf[0],
                              xfer_len,
                              err);
            }

            return;
        }
    }

    USBH_HID_DevUnlock(p_hid_dev);
}


/*
*********************************************************************************************************
*                                    USBH_HID_ProcessReportDesc()
*
* Description : Read and parse report descriptor.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE,                          Report descriptor successfully processed.
*               USBH_ERR_DESC_ALLOC,                    Failed to allocate memory for descriptor.
*               USBH_ERR_HID_RD_PARSER_FAIL,            Cannot parse report descriptor.
*
*                                                       ----- RETURNED BY USBH_CtrlRx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_HID_MemReadHIDDesc() : -----
*               USBH_ERR_DESC_EXTRA_NOT_FOUND,          HID descriptor not found.
*               USBH_ERR_DESC_INVALID,                  HID descriptor contains invalid value(s).
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_ProcessReportDesc (USBH_HID_DEV  *p_hid_dev)
{
    CPU_INT32U  len;
    USBH_ERR    err;


    err = USBH_HID_MemReadHIDDesc(p_hid_dev);                   /* Read HID desc.                                       */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_hid_dev->Desc.wClassDescriptorLength > USBH_HID_CFG_MAX_REPORT_DESC_LEN) {
        return (USBH_ERR_DESC_ALLOC);
    }

    len = USBH_CtrlRx(         p_hid_dev->DevPtr,               /* Read report desc.                                    */
                               USBH_REQ_GET_DESC,
                              (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_STD | USBH_REQ_RECIPIENT_IF),
                              ((USBH_HID_DESC_TYPE_REPORT << 8u) & 0xFF00u),
                               p_hid_dev->IfNbr,
                      (void *)&p_hid_dev->ReportDesc,
                               p_hid_dev->Desc.wClassDescriptorLength,
                               USBH_CFG_STD_REQ_TIMEOUT,
                              &err);

    if ((err == USBH_ERR_NONE                         ) &&
        (len == p_hid_dev->Desc.wClassDescriptorLength)) {

        err = USBH_HID_ItemParser(p_hid_dev,                    /* Parse report desc.                                   */
                                  p_hid_dev->ReportDesc,
                                  len);
        if (err != USBH_ERR_NONE) {
            err = USBH_ERR_HID_RD_PARSER_FAIL;
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_HID_RxReportAsync()
*
* Description : Read report on interrupt in endpoint.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE,                          if read request successfully transmitted.
*
*                                                       ----- RETURNED BY USBH_IntrRxAsync() : -----
*               USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not interrupt or direction is not IN.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_RxReportAsync (USBH_HID_DEV  *p_hid_dev)
{
    USBH_ERR    err;
    CPU_INT08U  len;


    len = p_hid_dev->MaxReportPtr->Size / 8u;
    if (p_hid_dev->MaxReportPtr->ReportID != 0u) {
        len++;
    }
                                                                /* Start receiving in data.                             */
    err = USBH_IntrRxAsync(       &p_hid_dev->IntrInEP,
                           (void *)p_hid_dev->RxBuf,
                                   len,
                                   USBH_HID_IntrRxCB,
                           (void *)p_hid_dev);
    if (err != USBH_ERR_NONE) {
        p_hid_dev->RxInProg = DEF_FALSE;
        USBH_PRINT_ERR(err);
    } else {
        p_hid_dev->RxInProg = DEF_TRUE;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                                 END
*********************************************************************************************************
*/

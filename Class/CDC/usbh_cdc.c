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
*                                  COMMUNICATIONS DEVICE CLASS (CDC)
*
* Filename : usbh_cdc.c
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_CDC_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_cdc.h"
#include  "../../Source/usbh_core.h"


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

/*
*********************************************************************************************************
*                                     UNION DESCRIPTOR DATA TYPE
*
* Note(s) : (1) See 'USB Class Definitions for Communication Devices Revision 1.1', Section 5.2.3.8, Table 33.
*********************************************************************************************************
*/

typedef  struct  usbh_cdc_union_desc {
    CPU_INT08U  bFunctionLength;                                /* Size of desc in bytes.                               */
    CPU_INT08U  bDescriptorType;                                /* IF desc type.                                        */
    CPU_INT08U  bDescriptorSubtype;                             /* Union functional desc subtype.                       */
    CPU_INT08U  bMasterInterface;                               /* Nbr of comm or Data Class IF, designated as master.  */
    CPU_INT08U  bSlaveInterface0;                               /* IF nbr of first slave or associated IF in union.     */
} USBH_CDC_UNION_DESC;


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

static  USBH_CDC_DEV  USBH_CDC_DevArr[USBH_CDC_CFG_MAX_DEV];
static  MEM_POOL      USBH_CDC_DevPool;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void       USBH_CDC_GlobalInit     (USBH_ERR             *p_err);

static  void      *USBH_CDC_ProbeDev       (USBH_DEV             *p_dev,
                                            USBH_ERR             *p_err);

static  USBH_ERR   USBH_CDC_EP_Open        (USBH_CDC_DEV         *p_cdc_dev);

static  USBH_ERR   USBH_CDC_UnionDescParse (USBH_CDC_UNION_DESC  *p_union_desc,
                                            USBH_IF              *p_if);

static  void       USBH_CDC_Suspend        (void                 *p_class_dev);

static  void       USBH_CDC_Resume         (void                 *p_class_dev);

static  void       USBH_CDC_Disconn        (void                 *p_class_dev);

static  void       USBH_CDC_DIC_DataTxCmpl (USBH_EP              *p_ep,
                                            void                 *p_buf,
                                            CPU_INT32U            buf_len,
                                            CPU_INT32U            xfer_len,
                                            void                 *p_arg,
                                            USBH_ERR              err);

static  void       USBH_CDC_DIC_DataRxCmpl (USBH_EP              *p_ep,
                                            void                 *p_buf,
                                            CPU_INT32U            buf_len,
                                            CPU_INT32U            xfer_len,
                                            void                 *p_arg,
                                            USBH_ERR              err);

static  USBH_ERR   USBH_CDC_EventRxAsync   (USBH_CDC_DEV         *p_cdc_dev);

static  void       USBH_CDC_CIC_EventRxCmpl(USBH_EP              *p_ep,
                                            void                 *p_buf,
                                            CPU_INT32U            buf_len,
                                            CPU_INT32U            xfer_len,
                                            void                 *p_arg,
                                            USBH_ERR              err);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          CDC CLASS DRIVER
*********************************************************************************************************
*/

USBH_CLASS_DRV  USBH_CDC_ClassDrv = {
    (CPU_INT08U *)"CDC",
                  USBH_CDC_GlobalInit,
                  USBH_CDC_ProbeDev,
                  0,
                  USBH_CDC_Suspend,
                  USBH_CDC_Resume,
                  USBH_CDC_Disconn
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBH_CDC_RefAdd()
*
* Description : Increment application reference count to CDC device.
*
* Argument(s) : p_cdc_dev         Pointer to CDC device.
*
* Return(s)   : USBH_ERR_NONE,          if reference added successfully.
*               USBH_ERR_INVALID_ARG,   if invalid argument passed to 'p_cdc_dev'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_RefAdd (USBH_CDC_DEV  *p_cdc_dev)
{
    if (p_cdc_dev == (USBH_CDC_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    (void)USBH_OS_MutexLock(p_cdc_dev->HMutex);
    p_cdc_dev->RefCnt++;                                        /* Increment app ref cnt to CDC dev.                    */
    (void)USBH_OS_MutexUnlock(p_cdc_dev->HMutex);

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_CDC_RefRel()
*
* Description : Decrement application reference count to CDC device. Free device if there is no more
*               reference to it.
*
* Argument(s) : p_cdc_dev         Pointer to CDC device.
*
* Return(s)   : USBH_ERR_NONE,          if reference released successfully.
*               USBH_ERR_INVALID_ARG,   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_FREE,          if device not successfully freed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_RefRel (USBH_CDC_DEV  *p_cdc_dev)
{
    LIB_ERR  err_lib;


    if (p_cdc_dev == (USBH_CDC_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    (void)USBH_OS_MutexLock(p_cdc_dev->HMutex);

    if (p_cdc_dev->RefCnt > 0) {
        p_cdc_dev->RefCnt--;

        if ((p_cdc_dev->RefCnt ==                           0u) &&
            (p_cdc_dev->State  == USBH_CLASS_DEV_STATE_DISCONN)) {
            (void)USBH_OS_MutexUnlock(p_cdc_dev->HMutex);

            Mem_PoolBlkFree(       &USBH_CDC_DevPool,
                            (void *)p_cdc_dev,
                                   &err_lib);
            if (err_lib != LIB_MEM_ERR_NONE) {
                return (USBH_ERR_FREE);
            } else {
                return (USBH_ERR_NONE);
            }
        }
    }

    (void)USBH_OS_MutexUnlock(p_cdc_dev->HMutex);

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                       USBH_CDC_EvtNotifyReg()
*
* Description : Register application / subclass event callback functions.
*
* Argument(s) : p_cdc_dev           Pointer to CDC device.
*
*               p_evt_notify        Pointer to application's / sublasse's notify function.
*
* Return(s)   : USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev' / 'p_evt_notify'.
*
*                                                       ---- RETURNED BY USBH_CDC_EventRxAsync ----
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

USBH_ERR  USBH_CDC_EventNotifyReg (USBH_CDC_DEV     *p_cdc_dev,
                                   SUBCLASS_NOTIFY   p_evt_notify,
                                   void             *p_arg)
{
    USBH_ERR  err;


    if ((p_cdc_dev    == (USBH_CDC_DEV  *)0) ||
        (p_evt_notify == (SUBCLASS_NOTIFY)0)) {
        return (USBH_ERR_INVALID_ARG);
    }

    p_cdc_dev->EvtNotifyPtr    = p_evt_notify;                  /* Reg callback fnct provided by subclass.     */
    p_cdc_dev->EvtNotifyArgPtr = p_arg;

    err = USBH_CDC_EventRxAsync(p_cdc_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_CDC_CmdTx()
*
* Description : Send CDC control command.
*
* Argument(s) : p_cdc_dev           Pointer to CDC device.
*
*               b_req               One-byte value that specifies request.
*
*               bm_req_type         One-byte value that specifies direction, type and the recipient of
*                                   request.
*
*               w_val               Two-byte value used to pass information to device.
*
*               p_buf               Pointer to data buffer to transmit.
*
*               buf_len             Two-byte value containing buffer length in octets.
*
* Return(s)   : USBH_ERR_NONE,                          if control command successfully sent.
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if device is not connected.
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

USBH_ERR  USBH_CDC_CmdTx (USBH_CDC_DEV  *p_cdc_dev,
                          CPU_INT08U     b_req,
                          CPU_INT08U     bm_req_type,
                          CPU_INT16U     w_val,
                          void          *p_buf,
                          CPU_INT32U     buf_len)
{
    USBH_ERR  err;


    if (p_cdc_dev == (USBH_CDC_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    if (p_cdc_dev->State != USBH_CLASS_DEV_STATE_CONN) {
        return (USBH_ERR_DEV_NOT_READY);
    }

    USBH_CtrlTx(p_cdc_dev->DevPtr,                              /* Issue req to dev.                                    */
                b_req,
                bm_req_type,
                w_val,
                p_cdc_dev->CIC_IF_Nbr,
                p_buf,
                buf_len,
                USBH_CFG_STD_REQ_TIMEOUT,
               &err);
    if (err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(           p_cdc_dev->DevPtr,
                            (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_CDC_RespRx()
*
* Description : Send CDC control rx request.
*
* Argument(s) : p_cdc_dev           Pointer to CDC device.
*
*               b_req               One-byte value that specifies request.
*
*               bm_req_type         One-byte value that specifies direction, type and the recipient of
*                                   request.
*
*               w_val               Two-byte value used to pass information to device.
*
*               p_buf               Pointer to data buffer to hold received data.
*
*               buf_len             Two-byte value containing buffer length in octets.
*
* Return(s)   : USBH_ERR_NONE,                          if control command successfully received.
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if device is not connected.
*
*                                                       ----- RETURNED BY USBH_CtrlRx() : -----
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

USBH_ERR  USBH_CDC_RespRx (USBH_CDC_DEV  *p_cdc_dev,
                           CPU_INT08U     b_req,
                           CPU_INT08U     bm_req_type,
                           CPU_INT16U     w_val,
                           void          *p_buf,
                           CPU_INT32U     buf_len)
{
    USBH_ERR  err;


    if (p_cdc_dev == (USBH_CDC_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    if (p_cdc_dev->State  != USBH_CLASS_DEV_STATE_CONN) {
        return (USBH_ERR_DEV_NOT_READY);
    }

    USBH_CtrlRx(p_cdc_dev->DevPtr,
                b_req,
                bm_req_type,
                w_val,
                p_cdc_dev->CIC_IF_Nbr,
                p_buf,
                buf_len,
                USBH_CFG_STD_REQ_TIMEOUT,
               &err);
    if (err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(           p_cdc_dev->DevPtr,
                            (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_CDC_DataTx()
*
* Description : Transfer raw buffer to device.
*
* Argument(s) : p_cdc_dev   Pointer to CDC device.
*
*               p_buf       Pointer to buffer of data that will be sent.
*
*               buf_len     Buffer length in octets.
*
*               p_err       Variable that will receive the return error code from this function.
*
*                                                                   ----- RETURNED BY USBH_BulkTx() : -----
*                           USBH_ERR_NONE                           Bulk transfer successfully transmitted.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not OUT.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets transferred.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_CDC_DataTx (USBH_CDC_DEV  *p_cdc_dev,
                             CPU_INT08U    *p_buf,
                             CPU_INT32U     buf_len,
                             USBH_ERR      *p_err)
{
    CPU_INT32U  xfer_len;


    xfer_len = USBH_BulkTx(       &p_cdc_dev->DIC_BulkOut,
                           (void *)p_buf,
                                   buf_len,
                                   0u,
                                   p_err);
    if (*p_err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr,
                           &p_cdc_dev->DIC_BulkOut);

        if (*p_err == USBH_ERR_EP_STALL) {
            USBH_EP_StallClr(&p_cdc_dev->DIC_BulkOut);
        }

        return (0u);
    }

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                       USBH_CDC_DataTxAsync()
*
* Description : Transfer raw buffer to device asynchronously.
*
* Argument(s) : p_cdc_dev           Pointer to CDC device.
*
*               p_buf               Pointer to buffer of data that will be sent.
*
*               buf_len             Buffer length in octets.
*
*               tx_cmpl_notify      Function that will be invoked upon completion of transmit operation.
*
*               p_arg               Pointer to argument that will be passed as parameter of 'tx_cmpl_notify'.
*
* Return(s)   : USBH_ERR_NONE,                          if data successfully transmitted.
*
*                                                       ----- RETURNED BY USBH_BulkTxAsync() : -----
*               USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not bulk or direction is not OUT.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_DataTxAsync (USBH_CDC_DEV     *p_cdc_dev,
                                CPU_INT08U       *p_buf,
                                CPU_INT32U        buf_len,
                                SUBCLASS_NOTIFY   tx_cmpl_notify,
                                void             *p_arg)
{
    USBH_ERR  err;


    p_cdc_dev->DataTxNotifyPtr    = tx_cmpl_notify;
    p_cdc_dev->DataTxNotifyArgPtr = p_arg;

    err = USBH_BulkTxAsync(       &p_cdc_dev->DIC_BulkOut,
                           (void *)p_buf,
                                   buf_len,
                                   USBH_CDC_DIC_DataTxCmpl,
                           (void *)p_cdc_dev);
    if (err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr,
                           &p_cdc_dev->DIC_BulkOut);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_cdc_dev->DIC_BulkOut);
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_CDC_DataRx()
*
* Description : Receive raw data from device.
*
* Argument(s) : p_cdc_dev   Pointer to CDC device.
*
*               p_buf       Pointer to data buffer to hold received data.
*
*               buf_len     Buffer length in octets.
*
*               p_err       Variable that will receive the return error code from this function.
*
*                                                                   ----- RETURNED BY USBH_BulkRx() : -----
*                           USBH_ERR_NONE                           Bulk transfer successfully received.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_CDC_DataRx (USBH_CDC_DEV  *p_cdc_dev,
                             CPU_INT08U    *p_buf,
                             CPU_INT32U     buf_len,
                             USBH_ERR      *p_err)
{
    CPU_INT32U  xfer_len;


    xfer_len = USBH_BulkRx(       &p_cdc_dev->DIC_BulkIn,
                           (void *)p_buf,
                                   buf_len,
                                   0u,
                                   p_err);
    if (*p_err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr,
                           &p_cdc_dev->DIC_BulkIn);

        if (*p_err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_cdc_dev->DIC_BulkIn);
        }

        return (0u);
    }

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                       USBH_CDC_DataRxAsync()
*
* Description : Receive raw data from device asynchronously.
*
* Argument(s) : p_cdc_dev           Pointer to CDC device.
*
*               p_buf               Pointer to data buffer to hold received data.
*
*               buf_len             Buffer length in octets.
*
*               rx_cmpl_notify      Function that will be invoked upon completion of receive operation.
*
*               p_arg               Pointer to argument that will be passed as parameter of 'tx_cmpl_notify'.
*
* Return(s)   : USBH_ERR_NONE,                          if data successfully received.
*
*                                                       ----- RETURNED BY USBH_BulkRxAsync() : -----
*               USBH_ERR_NONE                           If request is successfully submitted.
*               USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                If endpoint type is not Bulk or direction is not IN.
*               USBH_ERR_EP_INVALID_STATE               If endpoint is not opened.
*               USBH_ERR_ALLOC                          If URB cannot be allocated.
*               USBH_ERR_UNKNOWN                        If unknown error occured.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_DataRxAsync (USBH_CDC_DEV     *p_cdc_dev,
                                CPU_INT08U       *p_buf,
                                CPU_INT32U        buf_len,
                                SUBCLASS_NOTIFY   rx_cmpl_notify,
                                void             *p_arg)
{
    USBH_ERR  err;


    p_cdc_dev->DataRxNotifyPtr    = rx_cmpl_notify;
    p_cdc_dev->DataRxNotifyArgPtr = p_arg;

    err = USBH_BulkRxAsync(       &p_cdc_dev->DIC_BulkIn,
                           (void *)p_buf,
                                   buf_len,
                                   USBH_CDC_DIC_DataRxCmpl,
                           (void *)p_cdc_dev);
    if (err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr,
                           &p_cdc_dev->DIC_BulkIn);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_cdc_dev->DIC_BulkIn);
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_CDC_EventRxAsync()
*
* Description : Receive serial state of device.
*
* Argument(s) : p_cdc_dev       Pointer to the CDC device.
*
* Return(s)   : USBH_ERR_NONE,                          if data successfully received.
*
*                                                       ----- RETURNED BY USBH_IntrRxAsync() : -----
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

static  USBH_ERR  USBH_CDC_EventRxAsync (USBH_CDC_DEV  *p_cdc_dev)
{
    USBH_ERR  err;


    err = USBH_IntrRxAsync(       &p_cdc_dev->CIC_IntrIn,
                           (void *)p_cdc_dev->EventNotifyBuf,
                                   USBH_CDC_LEN_EVENT_BUF,
                                   USBH_CDC_CIC_EventRxCmpl,
                           (void *)p_cdc_dev);
    if (err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr,
                           &p_cdc_dev->CIC_IntrIn);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_cdc_dev->CIC_IntrIn);
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_CDC_CommIF_Get()
*
* Description : Get pointer to communication interface.
*
* Argument(s) : p_cdc_dev       Pointer to CDC device.
*
* Return(s)   : Pointer to communication interface.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_IF  *USBH_CDC_CommIF_Get (USBH_CDC_DEV  *p_cdc_dev)
{
    return (p_cdc_dev->CIC_IF_Ptr);
}


/*
*********************************************************************************************************
*                                       USBH_CDC_SubclassGet()
*
* Description : Get subclass code.
*
* Argument(s) : p_cdc_dev       Pointer to CDC device.
*
*               p_subclass      Pointer to variable that will receive subclass code.
*
* Return(s)   : USBH_ERR_NONE,              if subclass code successfully fetched.
*
*                                           ----- RETURNED BY USBH_IF_DescGet() : -----
*               USBH_ERR_INVALID_ARG,       Invalid argument passed to 'alt_ix'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_SubclassGet(USBH_CDC_DEV  *p_cdc_dev,
                               CPU_INT08U    *p_subclass)
{
    USBH_IF_DESC  if_desc;
    USBH_ERR      err;


    err = USBH_IF_DescGet(p_cdc_dev->CIC_IF_Ptr,
                          0u,
                         &if_desc);
    if (err != USBH_ERR_NONE) {
       *p_subclass = 0u;
        return (err);
    }

   *p_subclass = if_desc.bInterfaceSubClass;

    return (err);
}


/*
*********************************************************************************************************
*                                       USBH_CDC_ProtocolGet()
*
* Description : Get protocol code.
*
* Argument(s) : p_cdc_dev       Pointer to CDC device.
*
*               p_protocol      Pointer to variable that will receive protocol code.
*
* Return(s)   : USBH_ERR_NONE,              if protocol code successfully fetched.
*
*                                           ----- RETURNED BY USBH_IF_DescGet() : -----
*               USBH_ERR_INVALID_ARG,       Invalid argument passed to 'alt_ix'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ProtocolGet(USBH_CDC_DEV  *p_cdc_dev,
                               CPU_INT08U    *p_protocol)
{
    USBH_IF_DESC  if_desc;
    USBH_ERR      err;


    err = USBH_IF_DescGet(p_cdc_dev->CIC_IF_Ptr,
                          0u,
                         &if_desc);
    if (err != USBH_ERR_NONE) {
       *p_protocol = 0u;
        return (err);
    }

   *p_protocol = if_desc.bInterfaceProtocol;

    return (err);
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
*                                        USBH_CDC_GlobalInit()
*
* Description : Initialize the internal variables and structure required by CDC.
*
* Argument(s) : p_err       Variable that will receive the return error code from this function.
*                       USBH_ERR_NONE,                  Initialization was succesfull.
*                       USBH_ERR_ALLOC,                 Memory pool creation failed.
*
*                                                       ----- RETURNED BY USBH_OS_MutexCreate() : -----
*                       USBH_ERR_OS_SIGNAL_CREATE,      if mutex creation failed.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_GlobalInit (USBH_ERR  *p_err)
{
    CPU_INT08U  ix;
    CPU_SIZE_T  octets_reqd;
    CPU_SIZE_T  cdc_len;
    LIB_ERR     err_lib;


    cdc_len = sizeof(USBH_CDC_DEV) * USBH_CDC_CFG_MAX_DEV;

    Mem_Clr((void *)USBH_CDC_DevArr,
                    cdc_len);

    for (ix = 0u; ix < USBH_CDC_CFG_MAX_DEV; ix++) {            /* --------------- INIT CDC DEV STRUCT ---------------- */

       *p_err = USBH_OS_MutexCreate(&USBH_CDC_DevArr[ix].HMutex);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }

        USBH_CDC_DevArr[ix].State = USBH_CLASS_DEV_STATE_NONE;

        Mem_Clr((void *)USBH_CDC_DevArr[ix].EventNotifyBuf,
                        USBH_CDC_LEN_RESPONSE_AVAIL);
    }

    Mem_PoolCreate (       &USBH_CDC_DevPool,
                    (void *)USBH_CDC_DevArr,
                            cdc_len,
                            USBH_CDC_CFG_MAX_DEV,
                            sizeof(USBH_CDC_DEV),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
    } else {
       *p_err = USBH_ERR_NONE;
    }
}


/*
*********************************************************************************************************
*                                         USBH_CDC_ProbeDev()
*
* Description : Determine if device implements CDC.
*
* Argument(s) : p_dev       Pointer to device.
*
*               p_err       Variable that will receive the return error code from this function.
*
*                   USBH_ERR_NONE                           Device is CDC.
*                   USBH_ERR_CLASS_PROBE_FAIL               Device is not CDC.
*                   USBH_ERR_ALLOC                          Cannot allocate CDC structure.
*                   USBH_ERR_DESC_INVALID                   Invalid union descriptor.
*
*                                                           ----- RETURNED BY USBH_CfgSet() : -----
*                   USBH_ERR_UNKNOWN                        Unknown error occurred.
*                   USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                   USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                   USBH_ERR_HC_IO,                         Root hub input/output error.
*                   USBH_ERR_EP_STALL,                      Root hub does not support request.
*                   Host controller drivers error code,     Otherwise.
*
*                                                           ---- RETURNED BY USBH_CDC_EP_Open ----
*                   USBH_ERR_EP_ALLOC,                      If USBH_CFG_MAX_NBR_EPS reached.
*                   USBH_ERR_EP_NOT_FOUND,                  If endpoint with given type and direction not found.
*                   USBH_ERR_OS_SIGNAL_CREATE,              if mutex or semaphore creation failed.
*                   Host controller drivers error,          Otherwise.
*
*                                                           ---- RETURNED BY USBH_CDC_UnionDescParse ----
*                   USBH_ERR_DESC_INVALID,                  if union descriptor is invalid.
*
* Return(s)   : pcdc_acm_dev,   if device implements CDC.
*               0,              otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  *USBH_CDC_ProbeDev (USBH_DEV  *p_dev,
                                  USBH_ERR  *p_err)
{
    CPU_INT08U            cfg_ix;
    CPU_INT08U            if_ix;
    CPU_INT08U            alt_if_idx;
    CPU_INT08U            cfg_nbr    = 0u;
    CPU_INT08U            cic_if_nbr = 0u;
    CPU_INT08U            dic_if_nbr = 0u;
    CPU_INT08U            nbr_cfgs;
    CPU_INT08U            nbr_ifs;
    CPU_INT08U            nbr_alt_ifs;
    USBH_CFG             *p_cfg;
    USBH_DEV_DESC         dev_desc;
    USBH_CFG_DESC         cfg_desc;
    USBH_IF_DESC          if_desc;
    USBH_IF_DESC          alt_if_desc;
    USBH_IF              *p_if;
    USBH_IF              *p_cic_if;
    USBH_IF              *p_dic_if;
    USBH_CDC_DEV         *p_cdc_dev;
    USBH_CDC_UNION_DESC   union_desc;
    LIB_ERR               err_lib;


    p_cic_if = (USBH_IF *)0;
    p_dic_if = (USBH_IF *)0;

    USBH_DevDescGet(p_dev, &dev_desc);
    if (dev_desc.bDeviceClass != USBH_CLASS_CODE_CDC_CTRL &&
        dev_desc.bDeviceClass != USBH_CLASS_CODE_USE_IF_DESC) { /* CDC can be defined at the interface.                 */
       *p_err = USBH_ERR_CLASS_PROBE_FAIL;
        return ((void *)0);
    }

    nbr_cfgs = USBH_DevCfgNbrGet(p_dev);
    for (cfg_ix = 0u; cfg_ix < nbr_cfgs; cfg_ix++) {

        p_cfg = USBH_CfgGet(p_dev, cfg_ix);
        if (p_cfg == (USBH_CFG *) 0) {
           *p_err = USBH_ERR_CLASS_PROBE_FAIL;
            return ((void *)0);
        }
        USBH_CfgDescGet(p_cfg, &cfg_desc);

        nbr_ifs = USBH_CfgIF_NbrGet(p_cfg);
        for (if_ix = 0u; if_ix < nbr_ifs; if_ix++) {

             p_if = USBH_IF_Get(p_cfg, if_ix);
             USBH_IF_DescGet(p_if,                                  /* Get IF desc from IF.                             */
                             0u,
                            &if_desc);

             switch (if_desc.bInterfaceClass) {
                 case USBH_CLASS_CODE_CDC_CTRL:
                      p_cic_if   = p_if;
                      cic_if_nbr = if_desc.bInterfaceNumber;
                      cfg_nbr    = cfg_desc.bConfigurationValue;   /* Keep track of cfg for data transfer.              */

                     *p_err = USBH_CDC_UnionDescParse(&union_desc, p_if);
                      if (*p_err != USBH_ERR_NONE) {
                          return ((void *)0);
                      }

                      if (union_desc.bMasterInterface != cic_if_nbr) {
                         *p_err = USBH_ERR_DESC_INVALID;
                          return ((void *)0);
                      }

                      dic_if_nbr = union_desc.bSlaveInterface0;
                      break;

                 case USBH_CLASS_CODE_CDC_DATA:
                      if (p_cic_if != (USBH_IF *)0) {
                          if (if_desc.bInterfaceNumber == dic_if_nbr) {  /* Find IF and alt IF for data.                */

                              nbr_alt_ifs = USBH_IF_AltNbrGet(p_if);
                              for (alt_if_idx = 0u; alt_if_idx < nbr_alt_ifs; alt_if_idx++) {

                                  USBH_IF_DescGet(p_if, alt_if_idx, &alt_if_desc);
                                  if (alt_if_desc.bNbrEndpoints == 2) {  /* Match the first alt IF with 2 EPs (IN/OUT). */
                                      p_dic_if            = p_if;
                                      p_dic_if->AltIxSel  = alt_if_idx;
                                      break;
                                  }
                              }
                          }
                      }
                      break;

                 default:
                      break;
            }

            if ((p_cic_if != (USBH_IF *)0) &&
                (p_dic_if != (USBH_IF *)0)) {
                break;
            }
        }
    }

    if ((p_cic_if == (USBH_IF *)0) ||
        (p_dic_if == (USBH_IF *)0)) {
       *p_err = USBH_ERR_CLASS_PROBE_FAIL;
        return ((void *)0);
    }

   *p_err = USBH_CfgSet(p_dev, 1u);                             /* Select 1st cfg.                                      */
    if (*p_err != USBH_ERR_NONE) {
        return ((void *)0);
    }

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
    USBH_PRINT_LOG("CDC device connected\r\n");
#endif
                                                                /* Alloc a dev from CDC dev pool.                       */
    p_cdc_dev = (USBH_CDC_DEV *)Mem_PoolBlkGet(&USBH_CDC_DevPool,
                                                sizeof(USBH_CDC_DEV),
                                               &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return ((void *)0);
    }

    p_cdc_dev->DevPtr     = p_dev;
    p_cdc_dev->Cfg_Nbr    = cfg_nbr;
    p_cdc_dev->CIC_IF_Nbr = cic_if_nbr;
    p_cdc_dev->CIC_IF_Ptr = p_cic_if;
    p_cdc_dev->DIC_IF_Nbr = dic_if_nbr;
    p_cdc_dev->DIC_IF_Ptr = p_dic_if;

   *p_err = USBH_CDC_EP_Open(p_cdc_dev);
    if (*p_err != USBH_ERR_NONE) {
        Mem_PoolBlkFree(       &USBH_CDC_DevPool,
                        (void *)p_cdc_dev,
                               &err_lib);

        p_cdc_dev = (USBH_CDC_DEV *)0;
        return ((void *)p_cdc_dev);
    }

    p_cdc_dev->State = USBH_CLASS_DEV_STATE_CONN;

    return ((void *)p_cdc_dev);
}


/*
*********************************************************************************************************
*                                         USBH_CDC_Disconn()
*
* Description : Handle disconnection of a CDC device.
*
* Argument(s) : p_class_dev         Pointer to CDC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_Disconn (void  *p_class_dev)
{
    LIB_ERR        err_lib;
    USBH_CDC_DEV  *p_cdc_dev;


    p_cdc_dev = (USBH_CDC_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_cdc_dev->HMutex);

    p_cdc_dev->State = USBH_CLASS_DEV_STATE_DISCONN;

    USBH_EP_Close(&p_cdc_dev->CIC_IntrIn);                      /* Close EPs.                                           */
    USBH_EP_Close(&p_cdc_dev->DIC_BulkIn);
    USBH_EP_Close(&p_cdc_dev->DIC_BulkOut);

    if (p_cdc_dev->RefCnt == 0) {                               /* Release CDC dev if app ref cnt is 0.                 */
        (void)USBH_OS_MutexUnlock(p_cdc_dev->HMutex);

        Mem_PoolBlkFree(       &USBH_CDC_DevPool,
                        (void *)p_cdc_dev,
                               &err_lib);
        return;
    }

    (void)USBH_OS_MutexUnlock(p_cdc_dev->HMutex);
}


/*
*********************************************************************************************************
*                                         USBH_CDC_Suspend()
*
* Description : Put CDC device in suspended state.
*
* Argument(s) : p_class_dev     Pointer to CDC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_Suspend (void  *p_class_dev)
{
    USBH_CDC_DEV  *p_cdc_dev;


    p_cdc_dev = (USBH_CDC_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_cdc_dev->HMutex);
    p_cdc_dev->State = USBH_CLASS_DEV_STATE_SUSPEND;
    (void)USBH_OS_MutexUnlock(p_cdc_dev->HMutex);
}


/*
*********************************************************************************************************
*                                              USBH_CDC_Resume()
*
* Description : Put CDC device back to "connected" state.
*
* Argument(s) : p_class_dev     Pointer to CDC device.
*
* Return(s)   : None..
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_Resume (void  *p_class_dev)
{
    USBH_CDC_DEV  *p_cdc_dev;


    p_cdc_dev = (USBH_CDC_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_cdc_dev->HMutex);
    p_cdc_dev->State = USBH_CLASS_DEV_STATE_CONN;
    (void)USBH_OS_MutexUnlock(p_cdc_dev->HMutex);
}


/*
*********************************************************************************************************
*                                          USBH_CDC_EP_Open()
*
* Description : Open device endpoints.
*
* Argument(s) : p_cdc_dev       Pointer to CDC device.
*
* Return(s)   : USBH_ERR_NONE.                      If endpoints opened successfully.
*
*                                                   ----- RETURNED BY USBH_IntrInOpen() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
*                                                   ----- RETURNED BY USBH_BulkInOpen() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
*                                                   ----- RETURNED BY USBH_BulkOutOpen() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CDC_EP_Open (USBH_CDC_DEV  *p_cdc_dev)
{
    USBH_ERR  err;


    err = USBH_IntrInOpen(p_cdc_dev->DevPtr,
                          p_cdc_dev->CIC_IF_Ptr,
                         &p_cdc_dev->CIC_IntrIn);
    if (err != USBH_ERR_NONE) {
       return (err);
    }

    err = USBH_BulkInOpen(p_cdc_dev->DevPtr,
                          p_cdc_dev->DIC_IF_Ptr,
                         &p_cdc_dev->DIC_BulkIn);
    if (err != USBH_ERR_NONE) {
        (void)USBH_EP_Close(&p_cdc_dev->CIC_IntrIn);
        return (err);
    }

    err = USBH_BulkOutOpen(p_cdc_dev->DevPtr,
                           p_cdc_dev->DIC_IF_Ptr,
                          &p_cdc_dev->DIC_BulkOut);
    if (err != USBH_ERR_NONE) {
        (void)USBH_EP_Close(&p_cdc_dev->CIC_IntrIn);
        (void)USBH_EP_Close(&p_cdc_dev->DIC_BulkIn);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_CDC_UnionDescParse()
*
* Description : Parse union functional descriptor into USBH_CDC_UNION_DESC structure.
*
* Argument(s) : p_union_desc    Variable that will receive union descriptor.
*
*               p_if            Pointer to interface.
*
* Return(s)   : USBH_ERR_NONE.            if parsing is successful.
*               USBH_ERR_DESC_INVALID,    if union descriptor is invalid.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CDC_UnionDescParse (USBH_CDC_UNION_DESC  *p_union_desc,
                                           USBH_IF              *p_if)
{
    USBH_ERR     err;
    CPU_INT08U  *p_if_desc;


    err       = USBH_ERR_DESC_INVALID;
    p_if_desc = (CPU_INT08U *)p_if->IF_DataPtr;

    while (p_if_desc[1] != USBH_DESC_TYPE_EP) {

        if (p_if_desc[2] == USBH_CDC_FNCTL_DESC_SUB_UFD) {
            p_union_desc->bFunctionLength    = p_if_desc[0];
            p_union_desc->bDescriptorType    = p_if_desc[1];
            p_union_desc->bDescriptorSubtype = p_if_desc[2];
            p_union_desc->bMasterInterface   = p_if_desc[3];
            p_union_desc->bSlaveInterface0   = p_if_desc[4];
            err                              = USBH_ERR_NONE;
            break;

        } else {
            p_if_desc += p_if_desc[0];
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_CDC_DIC_DataTxCmpl()
*
* Description : Asynchronous data transfer completion function.
*
* Argument(s) : p_ep        Pointer to endpoint.
*
*               p_buf       Pointer to transmit buffer.
*
*               buf_len     Buffer length in octets.
*
*               xfer_len    Number of octets transferred.
*
*               p_arg       Asynchronous function argument.
*
*               err         Status of transaction.
*
* Return(s)   : none
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBH_CDC_DIC_DataTxCmpl (USBH_EP     *p_ep,
                                       void        *p_buf,
                                       CPU_INT32U   buf_len,
                                       CPU_INT32U   xfer_len,
                                       void        *p_arg,
                                       USBH_ERR     err)
{
    USBH_CDC_DEV  *p_cdc_dev;


    (void)buf_len;

    p_cdc_dev = (USBH_CDC_DEV *)p_arg;

    if (err != USBH_ERR_NONE) {                                 /* Chk status of transaction.                           */
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr,
                            p_ep);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(p_ep);
        }
    }

    p_cdc_dev->DataTxNotifyPtr(              p_cdc_dev->DataTxNotifyArgPtr,
                               (CPU_INT08U *)p_buf,
                                             xfer_len,
                                             err);
}


/*
*********************************************************************************************************
*                                      USBH_CDC_DIC_DataRxCmpl()
*
* Description : Asynchronous data receive completion function.
*
* Argument(s) : p_ep        Pointer to endpoint.
*
*               p_buf       Pointer to receive buffer.
*
*               buf_len     Buffer length in octets.
*
*               xfer_len    Number of octets received.
*
*               p_arg       Asynchronous function argument.
*
*               err         Status of transaction.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_DIC_DataRxCmpl (USBH_EP     *p_ep,
                                       void        *p_buf,
                                       CPU_INT32U   buf_len,
                                       CPU_INT32U   xfer_len,
                                       void        *p_arg,
                                       USBH_ERR     err)
{
    USBH_CDC_DEV  *p_cdc_dev;


    (void)buf_len;

    p_cdc_dev = (USBH_CDC_DEV *)p_arg;

    if (err != USBH_ERR_NONE) {                                 /* Check status of transaction.                         */
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr,
                            p_ep);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(p_ep);
        }
    }

    p_cdc_dev->DataRxNotifyPtr(              p_cdc_dev->DataRxNotifyArgPtr,
                               (CPU_INT08U *)p_buf,
                                             xfer_len,
                                             err);
}


/*
*********************************************************************************************************
*                                     USBH_CDC_CIC_EventRxCmpl()
*
* Description : Asynchronous serial state receive completion function.
*
* Argument(s) : p_ep        Pointer to endpoint.
*
*               p_buf       Pointer to receive buffer.
*
*               buf_len     Buffer length in octets.
*
*               xfer_len    Number of octets received.
*
*               p_arg       Asynchronous function argument.
*
*               err         Status of transaction.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_CIC_EventRxCmpl (USBH_EP    *p_ep,
                                       void        *p_buf,
                                       CPU_INT32U   buf_len,
                                       CPU_INT32U   xfer_len,
                                       void        *p_arg,
                                       USBH_ERR     err)
{
    USBH_CDC_DEV  *p_cdc_dev;


    (void)buf_len;

    p_cdc_dev = (USBH_CDC_DEV*)p_arg;

    if (err != USBH_ERR_NONE) {                                 /* Chk status of transaction.                           */
        (void)USBH_EP_Reset(p_cdc_dev->DevPtr, p_ep);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(p_ep);
        }
    }

    if (p_cdc_dev->EvtNotifyPtr != (SUBCLASS_NOTIFY)0) {
        p_cdc_dev->EvtNotifyPtr(              p_cdc_dev->EvtNotifyArgPtr,
                                (CPU_INT08U *)p_buf,
                                              xfer_len,
                                              err);
    }

    (void)USBH_CDC_EventRxAsync(p_cdc_dev);
}


/*
*********************************************************************************************************
*                                                 END
*********************************************************************************************************
*/

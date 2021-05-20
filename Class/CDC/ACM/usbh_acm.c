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
*                                    ABSTRACT CONTROL MODEL (ACM)
*
* Filename : usbh_acm.c
* Version  : V3.42.01
*********************************************************************************************************
*/

#define  USBH_CDC_ACM_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   MICRIUM_SOURCE
#include  "usbh_acm.h"
#include  "../../../Source/usbh_core.h"


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
*                          ABSTRACT CONTROL MANAGEMENT DESCRIPTOR DATA TYPE
*
* Note(s) : (1) See 'USB Class Definitions for Communication Devices Revision 1.1', Section 5.2.3.3, Table 28.
*********************************************************************************************************
*/

typedef  struct  usb_cdc_acm_desc {
    CPU_INT08U  bFunctionLength;                                /* Desc len in bytes.                                   */
    CPU_INT08U  bDescriptorType;                                /* IF desc type.                                        */
    CPU_INT08U  bDescriptorSubtype;                             /* ACM functional desc subtype.                         */
    CPU_INT08U  bmCapabilities;                                 /* Capabilities this config supports.                   */
} USBH_CDC_ACM_DESC;


/*
*********************************************************************************************************
*                                        LINE CODING DATA TYPE
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.1 Section 6.2.13.
*
*           (2) The 'bCharFormat' in the number of stop bits in the data.
*                 (a) 0 = 1   stop bit.
*                 (b) 1 = 1.5 stop bits.
*                 (c) 2 = 2   stop bits.
*
*           (3) The 'bParityType' indicates the type of parity used.
*                 (a) 0 = None.
*                 (b) 1 = Odd.
*                 (c) 2 = Even.
*                 (d) 3 = Mark.
*                 (e) 4 = Space.
*********************************************************************************************************
*/

typedef  struct  usbh_cdc_linecoding {
    CPU_INT32U  dwDTERate;                                      /* Data terminal rate in bps.                           */
    CPU_INT08U  bCharFormat;                                    /* Stop bits. (See Notes #2)                            */
    CPU_INT08U  bParityTtpe;                                    /* Parity type. (See Notes #3)                          */
    CPU_INT08U  bDataBits;                                      /* Nbr of character bits(5,6,7,8 or 16).                */
} USBH_CDC_LINECODING;


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

static  MEM_POOL          USBH_CDC_ACM_DevPool;
static  USBH_CDC_ACM_DEV  USBH_CDC_ACM_DevArr[USBH_CDC_ACM_CFG_MAX_DEV];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CDC_ACM_DescParse          (USBH_CDC_ACM_DESC    *p_cdc_acm_desc,
                                                   USBH_IF              *p_if);

static  void      USBH_CDC_ACM_SupportedReqEvtsGet(USBH_CDC_ACM_DEV     *p_cdc_acm_dev,
                                                   USBH_CDC_ACM_DESC    *p_cdc_acm_desc);

static  void      USBH_CDC_ACM_DataTxCmpl         (void                 *p_context,
                                                   CPU_INT08U           *p_buf,
                                                   CPU_INT32U            xfer_len,
                                                   USBH_ERR              err);

static  void      USBH_CDC_ACM_DataRxCmpl         (void                 *p_context,
                                                   CPU_INT08U           *p_buf,
                                                   CPU_INT32U            xfer_len,
                                                   USBH_ERR              err);

static  void      USBH_CDC_ACM_EventRxCmpl        (void                 *p_context,
                                                   CPU_INT08U           *p_buf,
                                                   CPU_INT32U            xfer_len,
                                                   USBH_ERR              err);

static void       USBH_CDC_ACM_ParseLineCoding    (USBH_CDC_LINECODING  *p_linecoding,
                                                   void                 *p_buf_src);

static void       USBH_CDC_ACM_FmtLineCoding      (USBH_CDC_LINECODING  *p_linecoding,
                                                   void                 *p_buf_dest);


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
*                                      USBH_CDC_ACM_GlobalInit()
*
* Description : Initializes all the USB CDC ACM structures and global variables
*
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE,      if success
*               USBH_ERR_ALLOC,     if memory pool creation fails
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_GlobalInit (void)
{
    CPU_SIZE_T  octets_reqd;
    CPU_SIZE_T  dev_size;
    USBH_ERR    err;
    LIB_ERR     err_lib;


    dev_size = sizeof(USBH_CDC_ACM_DEV) * USBH_CDC_ACM_CFG_MAX_DEV;

    Mem_Clr((void *)USBH_CDC_ACM_DevArr, dev_size);             /* Reset all ACM dev structures.                        */

    Mem_PoolCreate (       &USBH_CDC_ACM_DevPool,               /* Create mem pool for ACM dev struct.                  */
                    (void *)USBH_CDC_ACM_DevArr,
                            dev_size,
                            USBH_CDC_ACM_CFG_MAX_DEV,
                            sizeof(USBH_CDC_ACM_DEV),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        err = USBH_ERR_ALLOC;
    } else {
        err = USBH_ERR_NONE;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_CDC_ACM_Add()
*
* Description : Allocates memory for a CDC ACM device structure, registers a callback function to receive
*               interrupt events, reads ACM descriptor from the interface and reads ACM events and requests
*               supported by the device
*
* Argument(s) : p_cdc_dev       Pointer to the USB CDC device.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE               CDC ACM device added successfully.
*                               USBH_ERR_ALLOC              Device allocation failed.
*
*                                                           ---- RETURNED BY USBH_CDC_ACM_DescParse ----
*                               USBH_ERR_DESC_INVALID,      Descriptor contains invalid value(s).
*
* Return(s)   : Pointer to CDC ACM device.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_CDC_ACM_DEV  *USBH_CDC_ACM_Add (USBH_CDC_DEV  *p_cdc_dev,
                                     USBH_ERR      *p_err)
{
    USBH_CDC_ACM_DEV   *p_cdc_acm_dev;
    USBH_IF            *p_cif;
    USBH_CDC_ACM_DESC   acm_desc;
    LIB_ERR             err_lib;


    if (p_cdc_dev == (USBH_CDC_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return ((USBH_CDC_ACM_DEV *)0);
    }
                                                                /* Allocate CDC ACM dev from mem pool.                  */
    p_cdc_acm_dev = (USBH_CDC_ACM_DEV *)Mem_PoolBlkGet(&USBH_CDC_ACM_DevPool,
                                                        sizeof(USBH_CDC_ACM_DEV),
                                                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return ((USBH_CDC_ACM_DEV *)0);
    }

    p_cdc_acm_dev->CDC_DevPtr = p_cdc_dev;

    p_cif = USBH_CDC_CommIF_Get(p_cdc_acm_dev->CDC_DevPtr);
   *p_err = USBH_CDC_ACM_DescParse(&acm_desc, p_cif);           /* Get ACM desc from IF.                                */
    if (*p_err != USBH_ERR_NONE) {
        return ((USBH_CDC_ACM_DEV *)0);
    }

    USBH_CDC_ACM_SupportedReqEvtsGet(p_cdc_acm_dev,             /* Get events and requests supported by dev.            */
                                    &acm_desc);

    return (p_cdc_acm_dev);
}


/*
*********************************************************************************************************
*                                         USBH_CDC_ACM_Remove()
*
* Description : Free CDC ACM device structure.
*
* Argument(s) : p_cdc_acm_dev   Pointer to the USB CDC ACM device structure to remove.
*
* Return(s)   : USBH_ERR_NONE,              if CDC-ACM device successfully removed.
*               USBH_ERR_FREE,              if device could not be freed.
*               USBH_ERR_INVALID_ARG,       if invalid argument passed to 'p_cdc_acm_dev'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_Remove (USBH_CDC_ACM_DEV  *p_cdc_acm_dev)
{
    LIB_ERR  err;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    Mem_PoolBlkFree(       &USBH_CDC_ACM_DevPool,
                    (void *)p_cdc_acm_dev,
                           &err);
    if (err != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_FREE);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                   USBH_CDC_ACM_EventRxNotifyReg()
*
* Description : Register callback function to be called when serial state is received from device.
*
* Argument(s) : p_cdc_acm_dev               Pointer to CDC ACM device.
*
*               p_serial_state_notify       Function to be called.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_CDC_ACM_EventRxNotifyReg (USBH_CDC_ACM_DEV              *p_cdc_acm_dev,
                                     USBH_CDC_SERIAL_STATE_NOTIFY   p_serial_state_notify)
{
    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return;
    }

    p_cdc_acm_dev->EvtSerialStateNotifyPtr = p_serial_state_notify;

    USBH_CDC_EventNotifyReg(        p_cdc_acm_dev->CDC_DevPtr,
                                    USBH_CDC_ACM_EventRxCmpl,
                            (void *)p_cdc_acm_dev);
}


/*
*********************************************************************************************************
*                                    USBH_CDC_ACM_LineCodingSet()
*
* Description : Allows host to specify typical asynchronous line-character formatting properties.
*
* Argument(s) : p_cdc_acm_dev       Pointer to CDC ACM device.
*
*               baud_rate           Baud rate in bps.
*
*               stop_bits           Stop Bit value.
*
*               parity_val          Parity value.
*
*               data_bits           Number of data bits.
*
* Return(s)   : USBH_ERR_NONE,              if line-character formatting properties are successfully set.
*               USBH_ERR_NOT_SUPPORTED,     if request not supported by device.
*               USBH_ERR_INVALID_ARG,       if invalid argument passed to 'p_cdc_acm_dev'.
*               Specific error code,        otherwise.
*
* Note(s)     : (1) The GET_LINE_CODING request is described in "Universal Serial Bus Communications
*                   Class Subclass Specification for PSTN Devices", revision 1.2 section 6.3.10.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_LineCodingSet (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                                      CPU_INT32U         baud_rate,
                                      CPU_INT08U         stop_bits,
                                      CPU_INT08U         parity_val,
                                      CPU_INT08U         data_bits)
{
    USBH_CDC_LINECODING  line_coding;
    USBH_ERR             err;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }
                                                                /* Chk whether device supports this request.            */
    if (p_cdc_acm_dev->SupportedRequests.SetLineCoding == DEF_FALSE) {
        err = USBH_ERR_NOT_SUPPORTED;
        return (err);
    }

    line_coding.dwDTERate   = baud_rate;
    line_coding.bCharFormat = stop_bits;
    line_coding.bParityTtpe = parity_val;
    line_coding.bDataBits   = data_bits;

    USBH_CDC_ACM_FmtLineCoding(&line_coding,                    /* Write line coding parameters into buffer.            */
                                p_cdc_acm_dev->LineCodingBuf);

    err = USBH_CDC_CmdTx (        p_cdc_acm_dev->CDC_DevPtr,
                                  USBH_CDC_SET_LINECODING,
                                 (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                                  0u,
                          (void *)p_cdc_acm_dev->LineCodingBuf,
                                  USBH_CDC_LEN_LINECODING);

    return (err);
}


/*
*********************************************************************************************************
*                                    USBH_CDC_ACM_LineCodingGet()
*
* Description : Retrieves current device s asynchronous line-character formatting properties.
*
* Argument(s) : p_cdc_acm_dev         Pointer to CDC ACM device.
*
*               p_baud_rate           Variable that will receive the baud rate value.
*
*               p_stop_bits           Variable that will receive the stop bits value.
*
*               p_parity_val          Variable that will receive the parity value.
*
*               p_data_bits           Variable that will receive the data bits value.
*
* Return(s)   : USBH_ERR_NONE,                          if line-character formatting properties successfully retrieved.
*               USBH_ERR_NOT_SUPPORTED,                 if request not supported by device.
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_acm_dev'.
*
*                                                       ----- RETURNED BY USBH_CDC_RespRx() : -----
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if device is not connected.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The GET_LINE_CODING request is described in "Universal Serial Bus Communications
*                   Class Subclass Specification for PSTN Devices", revision 1.2 section 6.3.11.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_LineCodingGet (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                                      CPU_INT32U        *p_baud_rate,
                                      CPU_INT08U        *p_stop_bits,
                                      CPU_INT08U        *p_parity_val,
                                      CPU_INT08U        *p_data_bits)
{
    USBH_CDC_LINECODING  line_coding;
    USBH_ERR             err;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }
                                                                /* Chk if dev supports this request.                    */
    if (p_cdc_acm_dev->SupportedRequests.GetLineCoding == DEF_FALSE) {
        err = USBH_ERR_NOT_SUPPORTED;
        return (err);
    }

    err = USBH_CDC_RespRx (        p_cdc_acm_dev->CDC_DevPtr,
                                   USBH_CDC_GET_LINECODING,
                                  (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                                   0u,
                           (void *)p_cdc_acm_dev->LineCodingBuf,
                                   USBH_CDC_LEN_LINECODING);
    if (err == USBH_ERR_NONE) {
        USBH_CDC_ACM_ParseLineCoding(&line_coding,
                                      p_cdc_acm_dev->LineCodingBuf);

       *p_baud_rate  = line_coding.dwDTERate;
       *p_stop_bits  = line_coding.bCharFormat;
       *p_parity_val = line_coding.bParityTtpe;
       *p_data_bits  = line_coding.bDataBits;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                     USBH_CDC_ACM_LineStateSet()
*
* Description : Generates RS-232/V.24 style control signals.
*
* Argument(s) : p_cdc_acm_dev   Pointer to the CDC ACM device.
*
*               dtr_bit         Indicates to DCE if DTE is present or not. This signal corresponds to V.24
*                               signal 108/2 and RS-232 signal DTR.
*
*               rts_bit         Carrier control for half duplex modems. This signal corresponds to V.24 signal
*                               105 and RS-232 signal RTS.
*
* Return(s)   : USBH_ERR_NONE,              if Control Signals are successfully set.
*               USBH_ERR_NOT_SUPPORTED,     if request not supported by device.
*               USBH_ERR_INVALID_ARG,       if invalid argument passed to 'p_cdc_acm_dev'.
*
*                                                       ----- RETURNED BY USBH_CDC_CmdTx() : -----
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if device is not connected.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SET_CONTROL_LINE_STATE request is described in "Universal Serial Bus
*                   Communications Class Subclass Specification for PSTN Devices", revision 1.2
*                   section 6.3.12.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_LineStateSet (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                                     CPU_INT08U         dtr_bit,
                                     CPU_INT08U         rts_bit)
{
    USBH_ERR    err;
    CPU_INT08U  bitmap_value;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }
                                                                /* Chk if dev supports this request.                    */
    if (p_cdc_acm_dev->SupportedRequests.SetControlLineState == DEF_FALSE) {
        err = USBH_ERR_NOT_SUPPORTED;
        return (err);
    }

    if ((dtr_bit > 1u) ||                                       /* Chk if dev supports these signals.                   */
        (rts_bit > 1u)) {
        err = USBH_ERR_NOT_SUPPORTED;
        return (err);
    }

    bitmap_value = ((CPU_INT08U)(rts_bit << 1u) | dtr_bit);

    err = USBH_CDC_CmdTx (        p_cdc_acm_dev->CDC_DevPtr,
                                  USBH_CDC_SET_CONTROL_LINESTATE,
                                 (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                                  bitmap_value,
                          (void *)0,
                                  0u);

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_CDC_ACM_BreakSend()
*
* Description : Sends special carrier modulation that generates an RS-232 style break.
*
* Argument(s) : p_cdc_acm_dev       Pointer to the CDC ACM device.
*
*               break_time          Duration of the break signal, in milliseconds.
*
* Return(s)   : USBH_ERR_NONE,              if break signal successfully set.
*               USBH_ERR_NOT_SUPPORTED,     if this request is not supported by the device.
*               USBH_ERR_INVALID_ARG,       if invalid argument passed to 'p_cdc_acm_dev'.
*
*                                                       ----- RETURNED BY USBH_CDC_CmdTx() : -----
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if device is not connected.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SEND_BREAK request is described in "Universal Serial Bus Communications Class
*                   Subclass Specification for PSTN Devices", revision 1.2 section 6.3.13.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_BreakSend (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                                  CPU_INT16U         break_time)
{
    USBH_ERR  err;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    if (p_cdc_acm_dev->SupportedRequests.SendBreak == DEF_FALSE) {
        err = USBH_ERR_NOT_SUPPORTED;
        return (err);
    }

    err = USBH_CDC_CmdTx (        p_cdc_acm_dev->CDC_DevPtr,
                                  USBH_CDC_SEND_BREAK,
                                 (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                                  break_time,
                          (void *)0,
                                  0u);

    return (err);
}


/*
*********************************************************************************************************
*                                       USBH_CDC_ACM_CmdSend()
*
* Description : Send command to device.
*
* Argument(s) : p_cdc_acm_dev       Pointer to CDC ACM device.
*
*               p_buf               Pointer to buffer that contains the command.
*
*               buf_len             Buffer length in octets.
*
* Return(s)   : USBH_ERR_NONE,              if command sent to device successfully.
*               USBH_ERR_INVALID_ARG,       if invalid argument passed to 'p_cdc_acm_dev'.
*
*                                                       ----- RETURNED BY USBH_CDC_CmdTx() : -----
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if device is not connected.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SEND_ENCAPSULATED_COMMAND request is described in "USB Class Definitiopns for
*                   Communication Devices Specification", version 1.1 section 6.2.1.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_CmdSend (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                                CPU_INT08U        *p_buf,
                                CPU_INT32U         buf_len)
{
    USBH_ERR  err;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_CDC_CmdTx (        p_cdc_acm_dev->CDC_DevPtr,    /* See Note #1.                                         */
                                  USBH_CDC_SEND_ENCAPSULATED_COMMAND,
                                 (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                                  0u,
                          (void *)p_buf,
                                  buf_len);
    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_CDC_ACM_RespRx()
*
* Description : Receive response from device.
*
* Argument(s) : p_cdc_acm_dev           Pointer to CDC ACM device.
*
*               p_buf                   Pointer to receive buffer.
*
*               buf_len                 Buffer length in octets.
*
* Return(s)   : USBH_ERR_NONE                           if response from device received successfully.
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_acm_dev'.
*
*                                                       ----- RETURNED BY USBH_CDC_RespRx() : -----
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_cdc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if device is not connected.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The "GetEncapsulatedResponse" request is described in "USB Class Definitiopns for
*                   Communication Devices Specification", version 1.1 section 6.2.2.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ACM_RespRx (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                               CPU_INT08U        *p_buf,
                               CPU_INT32U         buf_len)
{
    USBH_ERR  err;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_CDC_RespRx (        p_cdc_acm_dev->CDC_DevPtr,   /* See Note #1.                                         */
                                   USBH_CDC_GET_ENCAPSULATED_RESPONSE,
                                  (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                                   0u,
                           (void *)p_buf,
                                   buf_len);

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_CDC_ACM_DataTx()
*
* Description : Sends data to CDC ACM device. This function is blocking.
*
* Argument(s) : p_cdc_acm_dev       Pointer to CDC ACM device.
*
*               p_buf               Pointer to transmit buffer.
*
*               buf_len             Buffer length in octets.
*
*               p_err               Variable that will receive the return error code from this function.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_cdc_acm_dev'.
*
*                                                               ----- RETURNED BY USBH_CDC_DataTx() : -----
*                       USBH_ERR_NONE                           Bulk transfer successfully transmitted.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not OUT.
*                       USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets transmitted.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_CDC_ACM_DataTx (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                                 void              *p_buf,
                                 CPU_INT32U         buf_len,
                                 USBH_ERR          *p_err)
{
    CPU_INT32U  xfer_len;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    xfer_len = USBH_CDC_DataTx(p_cdc_acm_dev->CDC_DevPtr,
                               p_buf,
                               buf_len,
                               p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                        USBH_CDC_ACM_DataRx()
*
* Description : Receives data from CDC ACM device. This function is synchronous.
*
* Argument(s) : p_cdc_acm_dev       Pointer to CDC ACM device.
*
*               p_buf               Pointer to buffer that contains received data.
*
*               buf_len             Received buffer length in octets.
*
*               p_err           Variable that will receive the return error code from this function.
*
*                               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_cdc_acm_dev'.
*
*                                                                       ----- RETURNED BY USBH_CDC_DataRx() : -----
*                               USBH_ERR_NONE                           Bulk transfer successfully received.
*                               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                               USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*                               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                               Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_CDC_ACM_DataRx (USBH_CDC_ACM_DEV  *p_cdc_acm_dev,
                                 void              *p_buf,
                                 CPU_INT32U         buf_len,
                                 USBH_ERR          *p_err)
{
    CPU_INT32U  xfer_len;


    if (p_cdc_acm_dev == (USBH_CDC_ACM_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    xfer_len = USBH_CDC_DataRx(p_cdc_acm_dev->CDC_DevPtr,
                               p_buf,
                               buf_len,
                               p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                     USBH_CDC_ACM_DataTxAsync()
*
* Description : Sends data to CDC ACM device. This function is asynchronous.
*
* Argument(s) : p_cdc_acm_dev         Pointer to CDC ACM device.
*
*               p_buf                 Pointer to buffer that contains data to send.
*
*               buf_len               Buffer length in octets.
*
*               p_tx_cmpl_notify      Function that will be invoked upon completion of transmit operation.
*
*               p_tx_cmpl_arg_ptr     Pointer to argument that will be passed as parameter of 'p_tx_cmpl_notify'.
*
* Return(s)   : USBH_ERR_NONE                           if data is sent to device successfully.
*               USBH_ERR_INVALID_ARG                    if invalid argument passed to 'p_cdc_acm_dev'/'p_buf'/
*                                                       'p_tx_cmpl_notify'.
*
*                                                       ----- RETURNED BY USBH_CDC_DataTxAsync() : -----
*               USBH_ERR_NONE,                          if data successfully transmitted.
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

USBH_ERR  USBH_CDC_ACM_DataTxAsync (USBH_CDC_ACM_DEV      *p_cdc_acm_dev,
                                    void                  *p_buf,
                                    CPU_INT32U             buf_len,
                                    USBH_CDC_DATA_NOTIFY   tx_cmpl_notify,
                                    void                  *p_tx_cmpl_arg)
{
    USBH_ERR  err;


    if ((p_cdc_acm_dev   == (USBH_CDC_ACM_DEV   *)0) ||
        (p_buf           == (void               *)0) ||
        (tx_cmpl_notify  == (USBH_CDC_DATA_NOTIFY)0)) {
        return (USBH_ERR_INVALID_ARG);
    }

    p_cdc_acm_dev->DataTxNotifyPtr = tx_cmpl_notify;
    p_cdc_acm_dev->DataTxArgPtr    = p_tx_cmpl_arg;

    err = USBH_CDC_DataTxAsync(        p_cdc_acm_dev->CDC_DevPtr,
                                       p_buf,
                                       buf_len,
                                       USBH_CDC_ACM_DataTxCmpl,
                               (void *)p_cdc_acm_dev);

    return (err);
}


/*
*********************************************************************************************************
*                                     USBH_CDC_ACM_DataRxAsync()
*
* Description : Receives data from CDC ACM device. This function is asynchronous.
*
* Argument(s) : p_cdc_acm_dev         Pointer to CDC ACM device.
*
*               p_buf                 Pointer to buffer that contains data to send.
*
*               buf_len               Buffer length in octets.
*
*               p_rx_cmpl_notify      Function that will be invoked upon completion of transmit operation.
*
*               p_rx_cmpl_arg_ptr     Pointer to argument that will be passed as parameter of 'p_tx_cmpl_notify'.
*
* Return(s)   : USBH_ERR_NONE                           if data is received from device successfully.
*               USBH_ERR_INVALID_ARG                    if invalid argument passed to 'p_cdc_acm_dev'/'p_buf'/
*                                                       'p_rx_cmpl_notify'.
*
*                                                       ----- RETURNED BY USBH_CDC_DataRxAsync() : -----
*               USBH_ERR_NONE,                          if data successfully received.
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

USBH_ERR  USBH_CDC_ACM_DataRxAsync (USBH_CDC_ACM_DEV      *p_cdc_acm_dev,
                                    void                  *p_buf,
                                    CPU_INT32U             buf_len,
                                    USBH_CDC_DATA_NOTIFY   rx_cmpl_notify,
                                    void                  *p_rx_cmpl_arg)
{
    USBH_ERR  err;


    if ((p_cdc_acm_dev   == (USBH_CDC_ACM_DEV   *)0) ||
        (p_buf           == (void               *)0) ||
        (rx_cmpl_notify  == (USBH_CDC_DATA_NOTIFY)0)) {
        return (USBH_ERR_INVALID_ARG);
    }

    p_cdc_acm_dev->DataRxNotifyPtr = rx_cmpl_notify;
    p_cdc_acm_dev->DataRxArgPtr    = p_rx_cmpl_arg;

    err = USBH_CDC_DataRxAsync(        p_cdc_acm_dev->CDC_DevPtr,
                                       p_buf,
                                       buf_len,
                                       USBH_CDC_ACM_DataRxCmpl,
                               (void *)p_cdc_acm_dev);

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
*                                      USBH_CDC_ACM_DescParse()
*
* Description : Parse Abstract control management functional descriptor into USBH_CDC_ACM_DESC structure.
*
* Argument(s) : p_acm_desc      Variable that receives parsed descriptor.
*
*               p_if            Buffer that contains Abstract control management functional descriptor.
*
* Return(s)   : USBH_ERR_NONE,            if parsing found Abstract control management functional descriptor.
*               USBH_ERR_DESC_INVALID,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CDC_ACM_DescParse (USBH_CDC_ACM_DESC  *p_cdc_acm_desc,
                                          USBH_IF            *p_if)
{
    USBH_ERR     err;
    CPU_INT08U  *p_if_desc;


    err       = USBH_ERR_DESC_INVALID;
    p_if_desc = (CPU_INT08U *)p_if->IF_DataPtr;

    while (p_if_desc[1] != USBH_DESC_TYPE_EP) {

        if (p_if_desc[2] == USBH_CDC_FNCTL_DESC_SUB_ACMFD) {
            p_cdc_acm_desc->bFunctionLength    = p_if_desc[0];
            p_cdc_acm_desc->bDescriptorType    = p_if_desc[1];
            p_cdc_acm_desc->bDescriptorSubtype = p_if_desc[2];
            p_cdc_acm_desc->bmCapabilities     = p_if_desc[3];
            err = USBH_ERR_NONE;
            break;
        } else {
            p_if_desc += p_if_desc[0];
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                 USBH_CDC_ACM_SupportedReqEvtsGet()
*
* Description : Retrieves requests and notifications supported by device.
*
* Argument(s) : p_cdc_acm_dev      Pointer to CDC ACM device.
*
*               p_cdc_acm_desc     Variable that contains Abstract control management functional descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_ACM_SupportedReqEvtsGet (USBH_CDC_ACM_DEV   *p_cdc_acm_dev,
                                                USBH_CDC_ACM_DESC  *p_cdc_acm_desc)
{
    if ((p_cdc_acm_desc->bmCapabilities & 0x01) != 0u) {
        p_cdc_acm_dev->SupportedRequests.SetCommFeature = DEF_TRUE;
        p_cdc_acm_dev->SupportedRequests.GetCommFeature = DEF_TRUE;
        p_cdc_acm_dev->SupportedRequests.ClrCommFeature = DEF_TRUE;
    }

    if ((p_cdc_acm_desc->bmCapabilities & 0x02) != 0u) {
        p_cdc_acm_dev->SupportedRequests.SetLineCoding       = DEF_TRUE;
        p_cdc_acm_dev->SupportedRequests.GetLineCoding       = DEF_TRUE;
        p_cdc_acm_dev->SupportedRequests.SetControlLineState = DEF_TRUE;
        p_cdc_acm_dev->SupportedEvents.SerialState           = DEF_TRUE;
    }

    if ((p_cdc_acm_desc->bmCapabilities & 0x04) != 0u) {
        p_cdc_acm_dev->SupportedRequests.SendBreak = DEF_TRUE;
    }

    if ((p_cdc_acm_desc->bmCapabilities & 0x08) != 0u) {
        p_cdc_acm_dev->SupportedEvents.NetworkConnection = DEF_TRUE;
    }
}


/*
*********************************************************************************************************
*                                      USBH_CDC_ACM_DataTxCmpl()
*
* Description : Callback function invoked when data transfer is completed.
*
* Argument(s) : p_context       Pointer to CDC ACM device.
*
*               p_buf           Pointer to buffer that contains data sent.
*
*               buf_len         Number of octets sent.
*
*               err             Transmit status.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_ACM_DataTxCmpl (void        *p_context,
                                       CPU_INT08U  *p_buf,
                                       CPU_INT32U   xfer_len,
                                       USBH_ERR     err)
{
    USBH_CDC_ACM_DEV  *p_cdc_acm_dev;


    p_cdc_acm_dev = (USBH_CDC_ACM_DEV *)p_context;

    p_cdc_acm_dev->DataTxNotifyPtr(p_cdc_acm_dev->DataTxArgPtr,
                                   p_buf,
                                   xfer_len,
                                   err);
}


/*
*********************************************************************************************************
*                                      USBH_CDC_ACM_DataRxCmpl()
*
* Description : Callback function invoked when data reception is completed.
*
* Argument(s) : p_context       Pointer to CDC ACM device.
*
*               p_buf           Pointer to buffer that contains data received.
*
*               buf_len         Number of octets received.
*
*               err             Receive status.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_ACM_DataRxCmpl (void        *p_context,
                                       CPU_INT08U  *p_buf,
                                       CPU_INT32U   xfer_len,
                                       USBH_ERR     err)
{
    USBH_CDC_ACM_DEV  *p_cdc_acm_dev;


    p_cdc_acm_dev = (USBH_CDC_ACM_DEV *)p_context;

    p_cdc_acm_dev->DataRxNotifyPtr(p_cdc_acm_dev->DataRxArgPtr,
                                   p_buf,
                                   xfer_len,
                                   err);
}


/*
*********************************************************************************************************
*                                     USBH_CDC_ACM_EventRxCmpl()
*
* Description : Callback function invoked when status reception is completed.
*
* Argument(s) : p_context       Pointer to CDC ACM device.
*
*               p_buf           Pointer to buffer that contains status received.
*
*               buf_len         Number of octets received.
*
*               err             Receive status.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_ACM_EventRxCmpl (void        *p_context,
                                        CPU_INT08U  *p_buf,
                                        CPU_INT32U   xfer_len,
                                        USBH_ERR     err)
{
    USBH_CDC_ACM_DEV       *p_cdc_acm_dev;
    USBH_CDC_SERIAL_STATE   serial_state;
    CPU_INT08U              req;
    CPU_INT16U              value;


    p_cdc_acm_dev = (USBH_CDC_ACM_DEV *)p_context;

    if (err == USBH_ERR_NONE) {
        if (xfer_len < 1u) {
            return;
        }

        req = MEM_VAL_GET_INT08U(p_buf + 1);

        if (req == USBH_CDC_NOTIFICATION_SERIAL_STATE) {
            if (xfer_len < 10u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                USBH_PRINT_LOG("Serial state incorrect length\r\n");
#endif
                return;
            }

            value = MEM_VAL_GET_INT16U_LITTLE(p_buf + 8);

            serial_state.RxCarrier  = DEF_BIT_IS_SET(value, DEF_BIT_00);
            serial_state.TxCarrier  = DEF_BIT_IS_SET(value, DEF_BIT_01);
            serial_state.Break      = DEF_BIT_IS_SET(value, DEF_BIT_02);
            serial_state.RingSignal = DEF_BIT_IS_SET(value, DEF_BIT_03);
            serial_state.Framing    = DEF_BIT_IS_SET(value, DEF_BIT_04);
            serial_state.Parity     = DEF_BIT_IS_SET(value, DEF_BIT_05);
            serial_state.OverRun    = DEF_BIT_IS_SET(value, DEF_BIT_06);

            if (p_cdc_acm_dev->EvtSerialStateNotifyPtr != (USBH_CDC_SERIAL_STATE_NOTIFY)0) {
                p_cdc_acm_dev->EvtSerialStateNotifyPtr(serial_state);
            }
        } else {
            return;
        }
    } else {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("CDC ACM Event Rx err: %d\r\n", err);
#endif
    }
}


/*
*********************************************************************************************************
*                                   USBH_CDC_ACM_ParseLineCoding()
*
* Description : Parse LineCoding buffer into LineCoding structure.
*
* Argument(s) : p_linecoding        Variable that will receive the parsed line coding.
*
*               p_buf_src           Pointer to buffer that contains LineCoding.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_ACM_ParseLineCoding (USBH_CDC_LINECODING  *p_linecoding,
                                            void                *p_buf_src)
{
    USBH_CDC_LINECODING  *p_buf_src_linecoding;


    p_buf_src_linecoding = (USBH_CDC_LINECODING *)p_buf_src;

    p_linecoding->dwDTERate   = MEM_VAL_GET_INT32U_LITTLE(&p_buf_src_linecoding->dwDTERate);
    p_linecoding->bCharFormat = p_buf_src_linecoding->bCharFormat;
    p_linecoding->bParityTtpe = p_buf_src_linecoding->bParityTtpe;
    p_linecoding->bDataBits   = p_buf_src_linecoding->bDataBits;

}


/*
*********************************************************************************************************
*                                    USBH_CDC_ACM_FmtLineCoding()
*
* Description : Format LineCoding buffer from LineCoding structure.
*
* Argument(s) : p_linecoding     Structure that holds LineCoding information.
*
*               p_buf_dest       Pointer to buffer that will receive LineCoding.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_CDC_ACM_FmtLineCoding (USBH_CDC_LINECODING  *p_linecoding,
                                          void                 *p_buf_dest)
{
    USBH_CDC_LINECODING  *p_buf_dest_linecoding;


    p_buf_dest_linecoding = (USBH_CDC_LINECODING *)p_buf_dest;

    p_buf_dest_linecoding->dwDTERate   = MEM_VAL_GET_INT32U_LITTLE(&p_linecoding->dwDTERate);
    p_buf_dest_linecoding->bCharFormat = p_linecoding->bCharFormat;
    p_buf_dest_linecoding->bParityTtpe = p_linecoding->bParityTtpe;
    p_buf_dest_linecoding->bDataBits   = p_linecoding->bDataBits;
}

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
*                             FUTURE TECHNOLOGY DEVICES INTERNATIONAL CLASS
*
* Filename : usbh_ftdi.c
* Version  : V3.42.00
* Note(s)  : The reference document "API for FTxxxx Devices Application Note AN_115" can be
*            requested from FTDI after signing a Non-Disclosure Agreement (NDA).
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_FTDI_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_ftdi.h"
#include  "../../Source/usbh_core.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           FTDI VENDOR ID
*
* Note(s) : (1) See "API for FTxxxx Devices Application Note AN_115", April 04 2011, Version 1.1,
*               section 2.
*
*           (2) The 'idVendor' field of the device descriptor may contain this value.
*
*********************************************************************************************************
*/

#define  USBH_FTDI_ID_VENDOR                            0x0403u


/*
*********************************************************************************************************
*                                           FTDI PRODUCT IDs
*
* Note(s) : (1) See "API for FTxxxx Devices Application Note AN_115", April 04 2011, Version 1.1,
*               section 2.
*
*           (2) The 'idProduct' field of the device descriptor may contain one of these values.
*
*********************************************************************************************************
*/

#define  USBH_FTDI_ID_PRODUCT_FT8U232AM                 0x6001u
#define  USBH_FTDI_ID_PRODUCT_FT232B                    0x6001u
#define  USBH_FTDI_ID_PRODUCT_FT2232D                   0x6010u
#define  USBH_FTDI_ID_PRODUCT_FT232R                    0x6001u
#define  USBH_FTDI_ID_PRODUCT_FT2232H                   0x6010u
#define  USBH_FTDI_ID_PRODUCT_FT4232H                   0x6011u
#define  USBH_FTDI_ID_PRODUCT_FT232H                    0x6014u


/*
*********************************************************************************************************
*                                       CLASS-SPECIFIC REQUESTS
*
* Note(s) : (1) See "API for FTxxxx Devices Application Note AN_115", April 04 2011, Version 1.1,
*               section 3.
*
*           (2) The 'bRequest' field of a class-specific setup request may contain one of these values.
*
*********************************************************************************************************
*/

#define  USBH_FTDI_REQ_RESET                            0x00u
#define  USBH_FTDI_REQ_SET_MODEM_CTRL                   0x01u
#define  USBH_FTDI_REQ_SET_FLOW_CTRL                    0x02u
#define  USBH_FTDI_REQ_SET_BAUD_RATE                    0x03u
#define  USBH_FTDI_REQ_SET_DATA                         0x04u
#define  USBH_FTDI_REQ_GET_MODEM_STATUS                 0x05u


/*
*********************************************************************************************************
*                                            USBH_FTDI_FNCT
*
* Note(s) : (1) FTDI device main structure.
*
*********************************************************************************************************
*/

typedef  struct  usbh_ftdi_fnct {
    USBH_FTDI_HANDLE          Handle;                           /* Handle of FTDI dev struct.                           */
    USBH_DEV                 *DevPtr;                           /* Ptr to dev struct.                                   */
    USBH_IF                  *IF_Ptr;                           /* Ptr to IF struct.                                    */
    USBH_EP                   BulkInEP;                         /* Bulk IN EP.                                          */
    USBH_EP                   BulkOutEP;                        /* Bulk OUT EP.                                         */

    CPU_INT08U                Port;                             /* Holds cur port.                                      */
    CPU_INT08U                State;                            /* Holds cur dev state.                                 */
    CPU_INT32U                MaxPktSize;                       /* Holds max pkt size.                                  */

    USBH_HMUTEX               RxHMutex;                         /* Handle on RX mutex.                                  */

    USBH_FTDI_ASYNC_TX_FNCT   DataTxNotifyPtr;                  /* Ptr to TX callback fnct.                             */
    void                     *DataTxNotifyArgPtr;               /* Ptr to TX notify arg.                                */
    USBH_FTDI_ASYNC_RX_FNCT   DataRxNotifyPtr;                  /* Ptr to RX callback fnct.                             */
    void                     *DataRxNotifyArgPtr;               /* Ptr to RX notify arg.                                */
    void                     *DataRxBuf;                        /* Ptr to data RX buf.                                  */
    USBH_FTDI_SERIAL_STATUS  *SerialStatusPtr;                  /* Ptr to serial status.                                */
    CPU_INT32U                RxBufLen;                         /* Size of RX buf.                                      */
    CPU_INT32U                XferLen;                          /* Tot len of data rx'd in octets.                      */
    CPU_INT16U                LastBytes;                        /* Last two bytes of last RX pkt.                       */
} USBH_FTDI_FNCT;


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

static  USBH_CLASS_DRV  USBH_FTDI_ClassDrv;                     /* FTDI Class drv API.                                  */


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

static  USBH_FTDI_FNCT   USBH_FTDI_DevTbl[USBH_FTDI_CFG_MAX_DEV];
static  MEM_POOL         USBH_FTDI_DevPool;

static  USBH_DEV        *USBH_FTDI_DevPtrPrev;                  /* Last conn'd dev addr.                                */
static  CPU_INT08U       USBH_FTDI_PortCnt;                     /* Cnt nbr of port of a dev.                            */


/*
*********************************************************************************************************
*                                            LOCAL MACROS
*********************************************************************************************************
*/

#define  USBH_FTDI_HANDLE_IX_GET(ftdi_handle)        ((CPU_INT08U)ftdi_handle);


/*
*********************************************************************************************************
*                                   USBH CLASS DRIVER API PROTOTYPES
*********************************************************************************************************
*/

static  void   USBH_FTDI_GlobalInit    (USBH_ERR          *p_err);

static  void  *USBH_FTDI_ProbeIF       (USBH_DEV          *p_dev,
                                        USBH_IF           *p_if,
                                        USBH_ERR          *p_err);

static  void   USBH_FTDI_Suspend       (void              *p_class_dev);

static  void   USBH_FTDI_Resume        (void              *p_class_dev);

static  void   USBH_FTDI_Disconn       (void              *p_class_dev);

static  void   USBH_FTDI_ClassDevNotify(void              *p_class_dev,
                                        CPU_INT08U         state,
                                        void              *p_ctx);


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void   USBH_FTDI_StdReq        (USBH_FTDI_HANDLE   ftdi_handle,
                                        CPU_INT08U         request,
                                        CPU_INT16U         value,
                                        CPU_INT08U         index,
                                        USBH_ERR          *p_err);

static  void   USBH_FTDI_DataTxCmpl    (USBH_EP           *p_ep,
                                        void              *p_buf,
                                        CPU_INT32U         buf_len,
                                        CPU_INT32U         xfer_len,
                                        void              *p_arg,
                                        USBH_ERR           err);

static  void   USBH_FTDI_DataRxCmpl    (USBH_EP           *p_ep,
                                        void              *p_buf,
                                        CPU_INT32U         buf_len,
                                        CPU_INT32U         xfer_len,
                                        void              *p_arg,
                                        USBH_ERR           err);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          FTDI CLASS DRIVER
*********************************************************************************************************
*/

static  USBH_CLASS_DRV  USBH_FTDI_ClassDrv = {
    (CPU_INT08U *)"FTDI",
                   USBH_FTDI_GlobalInit,
                   0,
                   USBH_FTDI_ProbeIF,
                   USBH_FTDI_Suspend,
                   USBH_FTDI_Resume,
                   USBH_FTDI_Disconn
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          USBH_FTDI_Init()
*
* Description : Initialize the global variables of the FTDI Class.
*
* Argument(s) : p_ftdi_callbacks  Callback functions structure to notify application when a FTDI device
*                                 is connected/disconnected.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                  Interface is of FTDI type.
*                                 USBH_ERR_DEV_ALLOC,             Cannot allocate FTDI device.
*
*                                                                 -----     RETURNED BY USBH_ClassDrvReg() :    -----
*                                 USBH_ERR_INVALID_ARG,           If invalid argument(s) passed to 'p_host'/'p_class_drv'.
*                                 USBH_ERR_CLASS_DRV_ALLOC,       If maximum class driver limit reached.
*
*                                                                 -----   RETURNED BY USBH_OS_MutexCreate() :   -----
*                                 USBH_ERR_ALLOC,                 Cannot allocate mutex.
*                                 USBH_ERR_OS_SIGNAL_CREATE       Cannot create mutex.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_FTDI_Init (USBH_FTDI_CALLBACKS  *p_ftdi_callbacks,
                      USBH_ERR             *p_err)
{
    CPU_SIZE_T  octets_reqd;
    CPU_INT08U  ix;
    LIB_ERR     err_lib;


    Mem_PoolCreate(       &USBH_FTDI_DevPool,
                   (void *)USBH_FTDI_DevTbl,
                          (sizeof(USBH_FTDI_FNCT) * USBH_FTDI_CFG_MAX_DEV),
                           USBH_FTDI_CFG_MAX_DEV,
                           sizeof(USBH_FTDI_FNCT),
                           sizeof(CPU_ALIGN),
                          &octets_reqd,
                          &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

    for (ix = 0u; ix < USBH_FTDI_CFG_MAX_DEV; ix++) {
       *p_err = USBH_OS_MutexCreate(&USBH_FTDI_DevTbl[ix].RxHMutex);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }

        USBH_FTDI_DevTbl[ix].Handle = (USBH_FTDI_HANDLE)ix;
        USBH_FTDI_DevTbl[ix].State  =  USBH_CLASS_DEV_STATE_NONE;
    }

    USBH_FTDI_DevPtrPrev = (void *)0;
    USBH_FTDI_PortCnt    =  0u;

   *p_err = USBH_ClassDrvReg(       &USBH_FTDI_ClassDrv,
                                     USBH_FTDI_ClassDevNotify,
                             (void *)p_ftdi_callbacks);
}


/*
*********************************************************************************************************
*                                          USBH_FTDI_Reset()
*
* Description : Reset the communication port.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               reset_ctrl        Select buffer to reset. This is a bitmap of the following possible values:
*
*                                 USBH_FTDI_RESET_CTRL_SIO,               Reset SIO (Resets both RX and TX buffer).
*                                 USBH_FTDI_RESET_CTRL_RX,                Purge RX buffer.
*                                 USBH_FTDI_RESET_CTRL_TX                 Purge TX buffer.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'reset_ctrl'.
*
*                                                                         -----  RETURNED BY USBH_FTDI_StdReq() : -----
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 Host controller error code              Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : For more information on the 'Reset' request, see 'API for FTxxxx Devices Application
*               Note AN_115, April 04 2011, Version 1.1, section 3.1'.
*********************************************************************************************************
*/

void  USBH_FTDI_Reset (USBH_FTDI_HANDLE   ftdi_handle,
                       CPU_INT16U         reset_ctrl,
                       USBH_ERR          *p_err)
{
    if ((reset_ctrl != USBH_FTDI_RESET_CTRL_SIO) &&
        (reset_ctrl != USBH_FTDI_RESET_CTRL_RX)  &&
        (reset_ctrl != USBH_FTDI_RESET_CTRL_TX)) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    USBH_FTDI_StdReq(ftdi_handle,                               /* Send Reset req.                                      */
                     USBH_FTDI_REQ_RESET,
                     reset_ctrl,
                     0u,
                     p_err);
}


/*
*********************************************************************************************************
*                                      USBH_FTDI_ModemCtrlSet()
*
* Description : Set modem control register of the communication port.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               modem_ctrl        Modem control value. This is a bitmap of the following possible values:
*
*                                 USBH_FTDI_MODEM_CTRL_DTR_SET,           Set     DTR state.
*                                 USBH_FTDI_MODEM_CTRL_DTR_RESET,         Reset   DTR state.
*                                 USBH_FTDI_MODEM_CTRL_DTR_ENABLED,       Use     DTR state.
*                                 USBH_FTDI_MODEM_CTRL_DTR_DISABLED,      Disable DTR state.
*
*                                 USBH_FTDI_MODEM_CTRL_RTS_SET,           Set     RTS state.
*                                 USBH_FTDI_MODEM_CTRL_RTS_RESET,         Reset   RTS state.
*                                 USBH_FTDI_MODEM_CTRL_RTS_ENABLED,       Use     RTS state.
*                                 USBH_FTDI_MODEM_CTRL_RTS_DISABLED       Disable RTS state.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'modem_ctrl'.
*
*                                                                         -----  RETURNED BY USBH_FTDI_StdReq() : -----
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 Host controller error code              Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : For more information on the 'ModemCtrl' request, see 'API for FTxxxx Devices Application
*               Note AN_115, April 04 2011, Version 1.1, section 3.2'.
*********************************************************************************************************
*/

void  USBH_FTDI_ModemCtrlSet (USBH_FTDI_HANDLE   ftdi_handle,
                              CPU_INT16U         modem_ctrl,
                              USBH_ERR          *p_err)
{
    if ((modem_ctrl & 0xFCFCu) != 0u) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    USBH_FTDI_StdReq(ftdi_handle,                               /* Send ModemCtrl req.                                  */
                     USBH_FTDI_REQ_SET_MODEM_CTRL,
                     modem_ctrl,
                     0u,
                     p_err);
}


/*
*********************************************************************************************************
*                                       USBH_FTDI_FlowCtrlSet()
*
* Description : Set flow control handshaking for the communication port.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               protocol          Protocol to set. This is a bitmap of the following possible values:
*
*                                 USBH_FTDI_PROTOCOL_RTS_CTS,             Enable output handshaking using RTS/CTS.
*                                 USBH_FTDI_PROTOCOL_DTR_DSR,             Enable output handshaking using DTR/DSR.
*                                 USBH_FTDI_PROTOCOL_XON_XOFF             Enable Xon/Xoff handshaking.
*
*               xon_char          Xon character.
*
*               xoff_char         Xoff character.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'xon_char'/
*                                                                         'xoff_char'/'protocol'.
*
*                                                                         -----  RETURNED BY USBH_FTDI_StdReq() : -----
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 Host controller error code              Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : For more information on the 'SetFlowCtrl' request, see 'API for FTxxxx Devices
*               Application Note AN_115, April 04 2011, Version 1.1, section 3.3'.
*********************************************************************************************************
*/

void  USBH_FTDI_FlowCtrlSet (USBH_FTDI_HANDLE   ftdi_handle,
                             CPU_INT08U         protocol,
                             CPU_INT08U         xon_char,
                             CPU_INT08U         xoff_char,
                             USBH_ERR          *p_err)
{
    CPU_INT16U  value;


    if ((protocol != USBH_FTDI_PROTOCOL_RTS_CTS)  &&
        (protocol != USBH_FTDI_PROTOCOL_DTR_DSR)  &&
        (protocol != USBH_FTDI_PROTOCOL_XON_XOFF) &&
        (protocol != DEF_BIT_NONE)) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    value = (CPU_INT16U)(((CPU_INT16U)xoff_char << 8u) | xon_char);

    USBH_FTDI_StdReq(ftdi_handle,                               /* Send SetFlowCtrl req.                                */
                     USBH_FTDI_REQ_SET_FLOW_CTRL,
                     value,
                     protocol,
                     p_err);
}


/*
*********************************************************************************************************
*                                        USBH_FTDI_BaudRateSet()
*
* Description : Set baud rate of the communication port.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               baud_rate         Baud rate to set.
*
*                                 USBH_FTDI_BAUD_RATE_300,                Set baud rate to 300    bits/sec.
*                                 USBH_FTDI_BAUD_RATE_600,                Set baud rate to 600    bits/sec.
*                                 USBH_FTDI_BAUD_RATE_1200,               Set baud rate to 1200   bits/sec.
*                                 USBH_FTDI_BAUD_RATE_2400,               Set baud rate to 2400   bits/sec.
*                                 USBH_FTDI_BAUD_RATE_4800,               Set baud rate to 4800   bits/sec.
*                                 USBH_FTDI_BAUD_RATE_9600,               Set baud rate to 9600   bits/sec.
*                                 USBH_FTDI_BAUD_RATE_19200,              Set baud rate to 19200  bits/sec.
*                                 USBH_FTDI_BAUD_RATE_38400,              Set baud rate to 38400  bits/sec.
*                                 USBH_FTDI_BAUD_RATE_57600,              Set baud rate to 57600  bits/sec.
*                                 USBH_FTDI_BAUD_RATE_115200,             Set baud rate to 115200 bits/sec.
*                                 USBH_FTDI_BAUD_RATE_230400,             Set baud rate to 230400 bits/sec.
*                                 USBH_FTDI_BAUD_RATE_460800,             Set baud rate to 460800 bits/sec.
*                                 USBH_FTDI_BAUD_RATE_921600              Set baud rate to 921600 bits/sec.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'baud_rate'.
*
*                                                                         -----  RETURNED BY USBH_FTDI_StdReq() : -----
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 Host controller error code              Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : For more information on the 'SetBaudRate' request, see 'API for FTxxxx Devices
*               Application Note AN_115, April 04 2011, Version 1.1, section 3.4'.
*********************************************************************************************************
*/

void  USBH_FTDI_BaudRateSet (USBH_FTDI_HANDLE   ftdi_handle,
                             CPU_INT16U         baud_rate,
                             USBH_ERR          *p_err)
{
    if ((baud_rate != USBH_FTDI_BAUD_RATE_300)    &&
        (baud_rate != USBH_FTDI_BAUD_RATE_600)    &&
        (baud_rate != USBH_FTDI_BAUD_RATE_1200)   &&
        (baud_rate != USBH_FTDI_BAUD_RATE_2400)   &&
        (baud_rate != USBH_FTDI_BAUD_RATE_4800)   &&
        (baud_rate != USBH_FTDI_BAUD_RATE_9600)   &&
        (baud_rate != USBH_FTDI_BAUD_RATE_19200)  &&
        (baud_rate != USBH_FTDI_BAUD_RATE_38400)  &&
        (baud_rate != USBH_FTDI_BAUD_RATE_57600)  &&
        (baud_rate != USBH_FTDI_BAUD_RATE_115200) &&
        (baud_rate != USBH_FTDI_BAUD_RATE_230400) &&
        (baud_rate != USBH_FTDI_BAUD_RATE_460800) &&
        (baud_rate != USBH_FTDI_BAUD_RATE_921600)) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    USBH_FTDI_StdReq(ftdi_handle,                               /* Send SetBaudRate req.                                */
                     USBH_FTDI_REQ_SET_BAUD_RATE,
                     baud_rate,
                     0u,
                     p_err);
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_DataSet()
*
* Description : Set data characteristics of the communication port.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               data_size         Number of data bits.
*
*               parity            Parity to use.
*
*                                 USBH_FTDI_DATA_PARITY_NONE,             Do not use the parity bit.
*                                 USBH_FTDI_DATA_PARITY_ODD,              Use odd        parity bit.
*                                 USBH_FTDI_DATA_PARITY_EVEN,             Use even       parity bit.
*                                 USBH_FTDI_DATA_PARITY_MARK,             Use mark       parity bit.
*                                 USBH_FTDI_DATA_PARITY_SPACE             Use space      parity bit.
*
*               stop_bits         Number of stop bits.
*
*                                 USBH_FTDI_DATA_STOP_BITS_1,             Use 1 stop bit.
*                                 USBH_FTDI_DATA_STOP_BITS_2              Use 2 stop bit.
*
*               break_bit         Send break.
*
*                                 USBH_FTDI_DATA_BREAK_ENABLED,           Send break.
*                                 USBH_FTDI_DATA_BREAK_DISABLED           Stop break.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'parity'/
*                                                                         'stop_bits'/'break_bit'.
*
*                                                                         -----  RETURNED BY USBH_FTDI_StdReq() : -----
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 Host controller error code              Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : For more information on the 'SetData' request, see 'API for FTxxxx Devices Application
*               Note AN_115, April 04 2011, Version 1.1, section 3.5'.
*********************************************************************************************************
*/

void  USBH_FTDI_DataSet (USBH_FTDI_HANDLE   ftdi_handle,
                         CPU_INT08U         data_size,
                         CPU_INT08U         parity,
                         CPU_INT08U         stop_bits,
                         CPU_INT08U         break_bit,
                         USBH_ERR          *p_err)
{
    CPU_INT16U  value;


    if ((parity != USBH_FTDI_DATA_PARITY_NONE) &&
        (parity != USBH_FTDI_DATA_PARITY_ODD)  &&
        (parity != USBH_FTDI_DATA_PARITY_EVEN) &&
        (parity != USBH_FTDI_DATA_PARITY_MARK) &&
        (parity != USBH_FTDI_DATA_PARITY_SPACE)) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if ((stop_bits != USBH_FTDI_DATA_STOP_BITS_1) &&
        (stop_bits != USBH_FTDI_DATA_STOP_BITS_2)) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if ((break_bit != USBH_FTDI_DATA_BREAK_ENABLED) &&
        (break_bit != USBH_FTDI_DATA_BREAK_DISABLED)) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    value = (CPU_INT16U)(((CPU_INT16U)(parity | stop_bits | break_bit) << 8u) | data_size);

    USBH_FTDI_StdReq(ftdi_handle,                               /* Send SetData req.                                    */
                     USBH_FTDI_REQ_SET_DATA,
                     value,
                     0u,
                     p_err);
}


/*
*********************************************************************************************************
*                                     USBH_FTDI_ModemStatusGet()
*
* Description : Retrieve the current value of the serial status register.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               p_serial_status   Pointer to USBH_FTDI_SERIAL_STATUS structure that will be filled by this function.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'/
*                                                                         'p_serial_status'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 Host controller drivers error code,     Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) The device updates the serial status every 16 milliseconds.
*
*               (2) The following masks may be used to determine if a bit of the modem status is set:
*
*                       USBH_FTDI_MODEM_STATUS_FS
*                       USBH_FTDI_MODEM_STATUS_HS
*                       USBH_FTDI_MODEM_STATUS_CLEAR_TO_SEND
*                       USBH_FTDI_MODEM_STATUS_DATA_SET_READY
*                       USBH_FTDI_MODEM_STATUS_RING_INDICATOR
*                       USBH_FTDI_MODEM_STATUS_DATA_CARRIER_DETECT
*
*               (3) The following masks may be used to determine if a bit of the line status is set:
*
*                       USBH_FTDI_LINE_STATUS_RX_OVERFLOW_ERROR
*                       USBH_FTDI_LINE_STATUS_PARITY_ERROR
*                       USBH_FTDI_LINE_STATUS_FRAMING_ERROR
*                       USBH_FTDI_LINE_STATUS_BREAK_INTERRUPT
*                       USBH_FTDI_LINE_STATUS_TX_REGISTER_EMPTY
*                       USBH_FTDI_LINE_STATUS_TX_EMPTY
*
*               (4) For more information on the 'GetModemStat' request, see 'API for FTxxxx Devices
*                   Application Note AN_115, April 04 2011, Version 1.1, section 3.6'.
*********************************************************************************************************
*/

void  USBH_FTDI_ModemStatusGet (USBH_FTDI_HANDLE          ftdi_handle,
                                USBH_FTDI_SERIAL_STATUS  *p_serial_status,
                                USBH_ERR                 *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    CPU_INT08U       ftdi_ix;
    CPU_INT08U       status[USBH_FTDI_SERIAL_STATUS_LEN];


    if (p_serial_status == (void *)0){
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    ftdi_ix = USBH_FTDI_HANDLE_IX_GET(ftdi_handle);
    if (ftdi_ix >= USBH_FTDI_CFG_MAX_DEV) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    p_ftdi_fnct = &USBH_FTDI_DevTbl[ftdi_ix];
    if (p_ftdi_fnct->DevPtr == (void *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if (p_ftdi_fnct->State != USBH_CLASS_DEV_STATE_CONN) {
       *p_err = USBH_ERR_DEV_NOT_READY;
        return;
    }

    (void)USBH_CtrlRx(         p_ftdi_fnct->DevPtr,             /* Send GET_MODEM_STATUS req.                           */
                               USBH_FTDI_REQ_GET_MODEM_STATUS,
                              (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_VENDOR | USBH_REQ_RECIPIENT_DEV),
                               0u,
                               p_ftdi_fnct->Port,
                      (void *)&status,
                               USBH_FTDI_SERIAL_STATUS_LEN,
                               USBH_CFG_STD_REQ_TIMEOUT,
                               p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_serial_status->ModemStatus = status[0u];
    p_serial_status->LineStatus  = status[1u];
}


/*
*********************************************************************************************************
*                                            USBH_FTDI_Tx()
*
* Description : Send data to FTDI device.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               p_buf             Pointer to buffer that contains the data to transfer.
*
*               buf_len           Buffer length, in octets.
*
*               timeout           Timeout, in milliseconds.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Data successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'/
*                                                                         'buf_len'.
*                                 USBH_ERR_NULL_PTR,                      Invalid null pointer passed to 'p_buf'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*
*                                                                         -----     RETURNED BY USBH_BulkTx() :    -----
*                                 USBH_ERR_NONE,                          Bulk transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                                 USBH_ERR_EP_INVALID_TYPE,               Endpoint type is not Bulk or direction is not OUT.
*                                 USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                                 Host controller drivers error code      Otherwise.
*
* Return(s) : Number of octets transferred, if NO error(s).
*
*             0, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_FTDI_Tx (USBH_FTDI_HANDLE   ftdi_handle,
                          void              *p_buf,
                          CPU_INT32U         buf_len,
                          CPU_INT16U         timeout,
                          USBH_ERR          *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    CPU_INT08U       ftdi_ix;
    CPU_INT32U       xfer_len;


    if (buf_len == 0u) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    if (p_buf == (void *)0) {
       *p_err = USBH_ERR_NULL_PTR;
        return (0u);
    }

    ftdi_ix = USBH_FTDI_HANDLE_IX_GET(ftdi_handle);
    if (ftdi_ix >= USBH_FTDI_CFG_MAX_DEV) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    p_ftdi_fnct = &USBH_FTDI_DevTbl[ftdi_ix];
    if (p_ftdi_fnct->DevPtr == (void *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    if (p_ftdi_fnct->State != USBH_CLASS_DEV_STATE_CONN) {
       *p_err = USBH_ERR_DEV_NOT_READY;
        return (0u);
    }

    xfer_len = USBH_BulkTx(&p_ftdi_fnct->BulkOutEP,             /* Send data through specified EP to FTDI dev.          */
                            p_buf,
                            buf_len,
                            timeout,
                            p_err);
    if (*p_err != USBH_ERR_NONE) {                              /* Reset EP if err occurred.                            */
        (void)USBH_EP_Reset(p_ftdi_fnct->DevPtr,
                           &p_ftdi_fnct->BulkOutEP);

        if (*p_err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_ftdi_fnct->BulkOutEP);
        }

        return (0u);
    }

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_TxAsync()
*
* Description : Send data to FTDI device. This function is non blocking.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               p_buf             Pointer to buffer of data that will be sent.
*
*               buf_len           Buffer length in octets.
*
*               tx_cmpl_notify    Function that will be invoked upon completion of transmit operation.
*
*               p_arg             Pointer to argument that will be passed as parameter of 'tx_cmpl_notify'.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          If data successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'/
*                                                                         'buf_len'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 USBH_ERR_NULL_PTR,                      Invalid null pointer passed to 'p_buf'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*
*                                                                         ----- RETURNED BY USBH_BulkTxAsync() : -----
*                                 USBH_ERR_NONE,                          If request is successfully submitted.
*                                 USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'p_ep'.
*                                 USBH_ERR_EP_INVALID_TYPE,               If endpoint type is not bulk or direction is
*                                                                         not OUT.
*                                 USBH_ERR_EP_INVALID_STATE,              If endpoint is not opened.
*                                 USBH_ERR_ALLOC,                         If URB cannot be allocated.
*                                 USBH_ERR_UNKNOWN,                       If unknown error occured.
*                                 Host controller drivers error code      Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) Asynchronous transmit is not thread safe. Hence, it is not safe to call this function
*                   from more than 1 application task at the same time for the same device. It is however
*                   safe to call it from the completion callback.
*********************************************************************************************************
*/

void  USBH_FTDI_TxAsync (USBH_FTDI_HANDLE          ftdi_handle,
                         void                     *p_buf,
                         CPU_INT32U                buf_len,
                         USBH_FTDI_ASYNC_TX_FNCT   tx_cmpl_notify,
                         void                     *p_arg,
                         USBH_ERR                 *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    CPU_INT08U       ftdi_ix;


    if (buf_len == 0u) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if (p_buf == (void *)0) {
       *p_err = USBH_ERR_NULL_PTR;
        return;
    }

    ftdi_ix = USBH_FTDI_HANDLE_IX_GET(ftdi_handle);
    if (ftdi_ix >= USBH_FTDI_CFG_MAX_DEV) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    p_ftdi_fnct = &USBH_FTDI_DevTbl[ftdi_ix];
    if (p_ftdi_fnct->DevPtr == (void *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if (p_ftdi_fnct->State != USBH_CLASS_DEV_STATE_CONN) {
       *p_err = USBH_ERR_DEV_NOT_READY;
        return;
    }

    p_ftdi_fnct->DataTxNotifyPtr    = tx_cmpl_notify;
    p_ftdi_fnct->DataTxNotifyArgPtr = p_arg;

   *p_err = USBH_BulkTxAsync(       &p_ftdi_fnct->BulkOutEP,    /* Send data through specified EP to FTDI dev.          */
                                     p_buf,
                                     buf_len,
                                     USBH_FTDI_DataTxCmpl,
                             (void *)p_ftdi_fnct);
    if (*p_err != USBH_ERR_NONE) {                              /* Reset EP if an err occurred.                         */
        (void)USBH_EP_Reset(p_ftdi_fnct->DevPtr,
                           &p_ftdi_fnct->BulkOutEP);

        if (*p_err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_ftdi_fnct->BulkOutEP);
        }
    }
}


/*
*********************************************************************************************************
*                                           USBH_FTDI_Rx()
*
* Description : Receive data from FTDI device.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               p_buf             Pointer to data buffer to hold received data.
*
*               buf_len           Buffer length, in octets.
*
*               timeout           Timeout, in milliseconds.
*
*               p_serial_status   Pointer to USBH_FTDI_SERIAL_STATUS structure that will be filled by this function.
*                                 This argument can be null.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Data successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'/
*                                                                         'buf_len'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 USBH_ERR_NULL_PTR,                      Invalid null pointer passed to 'p_buf'.
*
*
*                                                                         -----      RETURNED BY USBH_BulkRx() :      -----
*                                 USBH_ERR_NONE                           Bulk transfer successfully received.
*                                 USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                                 USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*                                 USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                                 Host controller drivers error code      Otherwise.
*
* Return(s)   : Number of octets received, if NO error(s).
*
*               0, otherwise.
*
* Note(s)     : (1) The following masks may be used to determine if a bit of the modem status is set:
*
*                       USBH_FTDI_MODEM_STATUS_FS
*                       USBH_FTDI_MODEM_STATUS_HS
*                       USBH_FTDI_MODEM_STATUS_CLEAR_TO_SEND
*                       USBH_FTDI_MODEM_STATUS_DATA_SET_READY
*                       USBH_FTDI_MODEM_STATUS_RING_INDICATOR
*                       USBH_FTDI_MODEM_STATUS_DATA_CARRIER_DETECT
*
*               (2) The following masks may be used to determine if a bit of the line status is set:
*
*                       USBH_FTDI_LINE_STATUS_RX_OVERFLOW_ERROR
*                       USBH_FTDI_LINE_STATUS_PARITY_ERROR
*                       USBH_FTDI_LINE_STATUS_FRAMING_ERROR
*                       USBH_FTDI_LINE_STATUS_BREAK_INTERRUPT
*                       USBH_FTDI_LINE_STATUS_TX_REGISTER_EMPTY
*                       USBH_FTDI_LINE_STATUS_TX_EMPTY
*
*               (3) The received data in 'p_buf' is in little-endian.
*
*               (4) The FTDI device always add a status of two bytes at the beginning of every packet.
*                   This function manage those status bytes in order to get the latest status followed
*                   by the data in 'p_buf'.
*                   This only applies when the application requests more than max packet size (MPS) - 2 bytes of data.
*
*                                        +----+----+----+----+-----+---------+---------+-----+-----
*   (a) Receive first pkt in buffer.     | S0 | S1 | D0 | D1 | ... | MPS - 2 | MPS - 1 | MPS | ...
*                                        +----+----+----+----+-----+---------+---------+-----+-----
*                                                                                 |       |
*                                                                                 v       v
*                                                                            +---------+-----+
*   (b) Save the last two bytes of data.                                     | MPS - 1 | MPS |
*                                                                            +---------+-----+
*
*                                        +----+----+----+----+-----+---------+-----+-----+---------+---------+-----
*   (c) Receive next pkt in buffer.      | S0 | S1 | D0 | D1 | ... | MPS - 2 | S2  | S3  | MPS + 1 | MPS + 2 | ...
*                                        +----+----+----+----+-----+---------+-----+-----+---------+---------+-----
*
*                                        +----+----+----+----+-----+---------+---------+-----+---------+---------+-----
*   (d) Update the status.               | S2 | S3 | D0 | D1 | ... | MPS - 2 |   S2    | S3  | MPS + 1 | MPS + 2 | ...
*                                        +----+----+----+----+-----+---------+---------+-----+---------+---------+-----
*                                                                                 ^       ^
*                                                                                 |       |
*                                                                            +---------+-----+
*   (e) Restore the two bytes of data.                                       | MPS - 1 | MPS |
*                                                                            +---------+-----+
*
*   (f) Repeat step (b) to (e) until the end of the buffer is reached or the last transfer size is
*       smaller than maxPktSize.
*
*********************************************************************************************************
*/

CPU_INT32U  USBH_FTDI_Rx (USBH_FTDI_HANDLE          ftdi_handle,
                          void                     *p_buf,
                          CPU_INT32U                buf_len,
                          CPU_INT16U                timeout,
                          USBH_FTDI_SERIAL_STATUS  *p_serial_status,
                          USBH_ERR                 *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    CPU_INT08U       ftdi_ix;
    CPU_INT32U       xfer_len;
    CPU_INT32U       xfer_len_cur;
    CPU_INT32U       buf_len_cur;
    CPU_INT16U       data;
    CPU_INT08U      *p_buf_cur08;
    CPU_INT32U       pkt_size_rem;
    CPU_INT32U       nbr_pkt;
    CPU_INT32U       ix;


    if (buf_len < 2u) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    if (p_buf == (void *)0) {
       *p_err = USBH_ERR_NULL_PTR;
        return (0u);
    }

    ftdi_ix = USBH_FTDI_HANDLE_IX_GET(ftdi_handle);
    if (ftdi_ix >= USBH_FTDI_CFG_MAX_DEV) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    p_ftdi_fnct = &USBH_FTDI_DevTbl[ftdi_ix];
    if (p_ftdi_fnct->DevPtr == (void *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

    if (p_ftdi_fnct->State != USBH_CLASS_DEV_STATE_CONN) {
       *p_err = USBH_ERR_DEV_NOT_READY;
        return (0u);
    }

    p_buf_cur08  = (CPU_INT08U *)p_buf;
                                                                /* Get max nbr of rx pkt allowed.                       */
    nbr_pkt      =  1u + ((buf_len - 1u) / p_ftdi_fnct->MaxPktSize);

    (void)USBH_OS_MutexLock(p_ftdi_fnct->RxHMutex);

    for (ix = 0u; ix < nbr_pkt; ix++) {

        if (ix == 0u) {                                         /* If first pkt.                                        */
            buf_len_cur = (nbr_pkt > 1u) ? p_ftdi_fnct->MaxPktSize : buf_len;

        } else {                                                /* Ensure rx buf is large enough for another pkt.       */
            if (buf_len < (xfer_len + p_ftdi_fnct->MaxPktSize)) {
                buf_len_cur = (buf_len - xfer_len);
            } else {
                buf_len_cur = p_ftdi_fnct->MaxPktSize;
            }
                                                                /* Save last two bytes of data.                         */
            MEM_VAL_COPY_16(&data, (p_buf_cur08 - USBH_FTDI_SERIAL_STATUS_LEN));

            if (p_serial_status != (void *)0) {                 /* Chk for line status error.                           */
                if ((DEF_BIT_IS_SET(p_serial_status->LineStatus, USBH_FTDI_LINE_STATUS_RX_OVERFLOW_ERROR) == DEF_YES) ||
                    (DEF_BIT_IS_SET(p_serial_status->LineStatus, USBH_FTDI_LINE_STATUS_PARITY_ERROR)      == DEF_YES) ||
                    (DEF_BIT_IS_SET(p_serial_status->LineStatus, USBH_FTDI_LINE_STATUS_FRAMING_ERROR)     == DEF_YES)) {

                    MEM_VAL_COPY_16(p_buf, p_buf_cur08);
                   *p_err = USBH_ERR_FTDI_LINE;
                    break;
                }
            }

            p_buf_cur08 -= USBH_FTDI_SERIAL_STATUS_LEN;
        }
                                                                /* Rx pkt through specified EP.                         */
        xfer_len_cur = USBH_BulkRx(       &p_ftdi_fnct->BulkInEP,
                                   (void *)p_buf_cur08,
                                           buf_len_cur,
                                           timeout,
                                           p_err);
        if (*p_err != USBH_ERR_NONE) {                          /* Reset EP if err occurred.                            */
            (void)USBH_EP_Reset(p_ftdi_fnct->DevPtr,
                               &p_ftdi_fnct->BulkInEP);

            if (*p_err == USBH_ERR_EP_STALL) {
                (void)USBH_EP_StallClr(&p_ftdi_fnct->BulkInEP);
            }

            (void)USBH_OS_MutexUnlock(p_ftdi_fnct->RxHMutex);

            return (0u);
        }

        if (ix == 0u) {                                         /* If first pkt.                                        */
            xfer_len = xfer_len_cur;
        } else {
            xfer_len += (xfer_len_cur - USBH_FTDI_SERIAL_STATUS_LEN);
            MEM_VAL_COPY_16(p_buf, p_buf_cur08);                /* Update status.                                       */
            MEM_VAL_COPY_16(p_buf_cur08, &data);                /* Restore two bytes of data.                           */
        }
        p_buf_cur08 += p_ftdi_fnct->MaxPktSize;

        if (p_serial_status != (void *)0) {
            p_serial_status->ModemStatus = *((CPU_INT08U *)p_buf);
            p_serial_status->LineStatus  = *((CPU_INT08U *)p_buf + 1u);
        }

        pkt_size_rem = ((xfer_len - USBH_FTDI_SERIAL_STATUS_LEN) % (p_ftdi_fnct->MaxPktSize - USBH_FTDI_SERIAL_STATUS_LEN));
        if ((xfer_len_cur == USBH_FTDI_SERIAL_STATUS_LEN) ||    /* Stop transfer if RX size is smaller than maxPktSize. */
            (pkt_size_rem != 0u)) {
            break;
        }
    }

    (void)USBH_OS_MutexUnlock(p_ftdi_fnct->RxHMutex);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_RxAsync()
*
* Description : Receive data from FTDI device. This function is non blocking.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               p_buf             Pointer to data buffer to hold received data.
*
*               buf_len           Buffer length in octets.
*
*               p_serial_status   Pointer to USBH_FTDI_SERIAL_STATUS structure that will be filled by this function.
*                                 This argument can be null.
*
*               rx_cmpl_notify    Function that will be invoked upon completion of receive operation.
*
*               p_arg             Pointer to argument that will be passed as parameter of 'rx_cmpl_notify'.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          If data successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'/
*                                                                         'buf_len'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*                                 USBH_ERR_NULL_PTR,                      Invalid null pointer passed to 'p_buf'.
*
*                                                                         ----- RETURNED BY USBH_BulkRxAsync() : -----
*                                 USBH_ERR_NONE,                          If request is successfully submitted.
*                                 USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'p_ep'.
*                                 USBH_ERR_EP_INVALID_TYPE,               If endpoint type is not Bulk or direction is
*                                                                         not IN.
*                                 USBH_ERR_EP_INVALID_STATE,              If endpoint is not opened.
*                                 USBH_ERR_ALLOC,                         If URB cannot be allocated.
*                                 USBH_ERR_UNKNOWN,                       If unknown error occured.
*                                 Host controller drivers error code      Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) The following masks may be used to determine if a bit of the modem status is set:
*
*                       USBH_FTDI_MODEM_STATUS_FS
*                       USBH_FTDI_MODEM_STATUS_HS
*                       USBH_FTDI_MODEM_STATUS_CLEAR_TO_SEND
*                       USBH_FTDI_MODEM_STATUS_DATA_SET_READY
*                       USBH_FTDI_MODEM_STATUS_RING_INDICATOR
*                       USBH_FTDI_MODEM_STATUS_DATA_CARRIER_DETECT
*
*               (2) The following masks may be used to determine if a bit of the line status is set:
*
*                       USBH_FTDI_LINE_STATUS_RX_OVERFLOW_ERROR
*                       USBH_FTDI_LINE_STATUS_PARITY_ERROR
*                       USBH_FTDI_LINE_STATUS_FRAMING_ERROR
*                       USBH_FTDI_LINE_STATUS_BREAK_INTERRUPT
*                       USBH_FTDI_LINE_STATUS_TX_REGISTER_EMPTY
*                       USBH_FTDI_LINE_STATUS_TX_EMPTY
*
*               (3) Asynchronous receive is not thread safe. Hence, it is not safe to call this function
*                   from more than 1 application task at the same time for the same device. It is however
*                   safe to call it from the completion callback.
*********************************************************************************************************
*/

void  USBH_FTDI_RxAsync (USBH_FTDI_HANDLE          ftdi_handle,
                         void                     *p_buf,
                         CPU_INT32U                buf_len,
                         USBH_FTDI_SERIAL_STATUS  *p_serial_status,
                         USBH_FTDI_ASYNC_RX_FNCT   rx_cmpl_notify,
                         void                     *p_arg,
                         USBH_ERR                 *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    CPU_INT08U       ftdi_ix;


    if (buf_len < 2u) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if (p_buf == (void *)0) {
       *p_err = USBH_ERR_NULL_PTR;
        return;
    }

    ftdi_ix = USBH_FTDI_HANDLE_IX_GET(ftdi_handle);
    if (ftdi_ix >= USBH_FTDI_CFG_MAX_DEV) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    p_ftdi_fnct = &USBH_FTDI_DevTbl[ftdi_ix];
    if (p_ftdi_fnct->DevPtr == (void *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if (p_ftdi_fnct->State != USBH_CLASS_DEV_STATE_CONN) {
       *p_err = USBH_ERR_DEV_NOT_READY;
        return;
    }

    p_ftdi_fnct->DataRxNotifyPtr    = rx_cmpl_notify;
    p_ftdi_fnct->DataRxNotifyArgPtr = p_arg;
    p_ftdi_fnct->DataRxBuf          = p_buf;
    p_ftdi_fnct->RxBufLen           = buf_len;
    p_ftdi_fnct->SerialStatusPtr    = p_serial_status;
    p_ftdi_fnct->XferLen            = 0u;

    if (buf_len > p_ftdi_fnct->MaxPktSize) {
        buf_len = p_ftdi_fnct->MaxPktSize;
    }

   *p_err = USBH_BulkRxAsync(       &p_ftdi_fnct->BulkInEP,     /* Rx first pkt through specified EP.                   */
                                     p_buf,
                                     buf_len,
                                     USBH_FTDI_DataRxCmpl,
                             (void *)p_ftdi_fnct);
    if (*p_err != USBH_ERR_NONE) {
        (void)USBH_EP_Reset(p_ftdi_fnct->DevPtr,
                           &p_ftdi_fnct->BulkInEP);

        if (*p_err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_ftdi_fnct->BulkInEP);
        }
    }
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_DevGet()
*
* Description : Retrieves the device informations.
*
* Argument(s) : ftdi_handle       Handle on FTDI device.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          If device informations retrieved successfully.
*                                 USBH_ERR_INVALID_ARG                    Invalid argument passed to 'ftdi_handle'
*
* Return(s)   : Pointer to FTDI device.
*
* Note(s)     : None.
*
*********************************************************************************************************
*/
USBH_DEV  *USBH_FTDI_DevGet (USBH_FTDI_HANDLE   ftdi_handle,
                             USBH_ERR          *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    CPU_INT08U       ftdi_ix;


   *p_err   = USBH_ERR_NONE;
    ftdi_ix = USBH_FTDI_HANDLE_IX_GET(ftdi_handle);
    if (ftdi_ix >= USBH_FTDI_CFG_MAX_DEV) {
       *p_err = USBH_ERR_INVALID_ARG;
        return ((USBH_DEV *)0);
    }

    p_ftdi_fnct = &USBH_FTDI_DevTbl[ftdi_ix];
    if (p_ftdi_fnct->DevPtr == (void *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return ((USBH_DEV *)0);
    }

    return(p_ftdi_fnct->DevPtr);
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
*                                        USBH_FTDI_GlobalInit()
*
* Description : Not used but it has to be called by the host stack.
*
* Argument(s) : p_err          Returns 'USBH_ERR_NONE'.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FTDI_GlobalInit (USBH_ERR  *p_err)
{
   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_ProbeIF()
*
* Description : Determine if the interface is FTDI class interface.
*
* Argument(s) : p_dev       Pointer to USB device.
*
*               p_if        Pointer to interface.
*
*               p_err       Variable that will receive the return error code from this function.
*                           USBH_ERR_NONE,                      Interface is of FTDI type.
*                           USBH_ERR_DEV_ALLOC,                 Cannot allocate FTDI device.
*                           USBH_ERR_CLASS_DRV_NOT_FOUND,       Interface is not of FTDI type.
*
*                                                               ----- RETURNED BY USBH_IF_DescGet() : -----
*                           USBH_ERR_INVALID_ARG,               Invalid argument passed to 'alt_ix'.
*
*                                                               ----- RETURNED BY USBH_BulkInOpen() : -----
*                           USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*                           USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*                           USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*                           Host controller drivers error,      Otherwise.
*
*                                                               ----- RETURNED BY USBH_BulkOutOpen() : -----
*                           USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*                           USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*                           USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*                           Host controller drivers error       Otherwise.
*
* Return(s)   : p_ftdi_dev,     If device has a FTDI class interface.
*               0               Otherwise.
*
* Note(s)     : (1) The 'bInterfaceClass' field of a FTDI device is 'Vendor Specific'. So, the 'Vendor ID'
*                   and the 'Product ID' must be parsed as well to ensure connected device is FTDI.
*
*               (2) The port number is determined by increasing 'USBH_FTDI_PortCnt' every time a device
*                   with the same address is probed. If a new device is connected, this counter is reset
*                   to '0'.
*********************************************************************************************************
*/

static  void  *USBH_FTDI_ProbeIF (USBH_DEV  *p_dev,
                                  USBH_IF   *p_if,
                                  USBH_ERR  *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    USBH_DEV_DESC    dev_desc;
    USBH_IF_DESC     if_desc;
    LIB_ERR          err_lib;


   *p_err = USBH_IF_DescGet(p_if, 0u, &if_desc);                /* Get IF desc from IF.                                 */
    if (*p_err != USBH_ERR_NONE) {
        return ((void *)0);
    }

    USBH_DevDescGet(p_dev, &dev_desc);                          /* Get dev desc.                                        */

                                                                /* bInterfaceClass must be Vendor Specific.             */
    if (if_desc.bInterfaceClass != USBH_CLASS_CODE_VENDOR_SPECIFIC) {

        p_ftdi_fnct = (USBH_FTDI_FNCT *)0;
       *p_err       =  USBH_ERR_CLASS_DRV_NOT_FOUND;
                                                                /* idVendor must be FTDI or burned Vendor ID.           */
    } else if ((dev_desc.idVendor != USBH_FTDI_ID_VENDOR) &&
               (dev_desc.idVendor != USBH_FTDI_CFG_ID_VENDOR_CUSTOM)) {

        p_ftdi_fnct = (USBH_FTDI_FNCT *)0;
       *p_err       =  USBH_ERR_CLASS_DRV_NOT_FOUND;
                                                                /* idProduct must be FT8U232AM, FT232B, FT2232D, FT232R,*/
                                                                /* FT2232H, FT4232H, FT232H or burned Product ID.       */
    } else if ((dev_desc.idProduct != USBH_FTDI_ID_PRODUCT_FT232R)  &&
               (dev_desc.idProduct != USBH_FTDI_ID_PRODUCT_FT2232D) &&
               (dev_desc.idProduct != USBH_FTDI_ID_PRODUCT_FT4232H) &&
               (dev_desc.idProduct != USBH_FTDI_ID_PRODUCT_FT232H)  &&
               (dev_desc.idProduct != USBH_FTDI_CFG_ID_PRODUCT_CUSTOM)) {

        p_ftdi_fnct = (USBH_FTDI_FNCT *)0;
       *p_err       =  USBH_ERR_CLASS_DRV_NOT_FOUND;

    } else {
                                                                /* Alloc a dev from FTDI fnct pool.                     */
        p_ftdi_fnct = (USBH_FTDI_FNCT *)Mem_PoolBlkGet(&USBH_FTDI_DevPool,
                                                        sizeof(USBH_FTDI_FNCT),
                                                       &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
           *p_err = USBH_ERR_DEV_ALLOC;
            return ((void *)0);
        }

        p_ftdi_fnct->State  = USBH_CLASS_DEV_STATE_CONN;
        p_ftdi_fnct->DevPtr = p_dev;
        p_ftdi_fnct->IF_Ptr = p_if;

        if (USBH_FTDI_DevPtrPrev == p_dev) {                    /* Determine serial port of actual IF. See note (2).    */
            USBH_FTDI_PortCnt++;
            p_ftdi_fnct->Port = USBH_FTDI_PortCnt;
        } else {
            p_ftdi_fnct->Port = 0u;
            USBH_FTDI_PortCnt = 1u;
        }
        USBH_FTDI_DevPtrPrev = p_dev;

       *p_err = USBH_BulkInOpen(p_ftdi_fnct->DevPtr,            /* Open Bulk IN EP.                                     */
                                p_ftdi_fnct->IF_Ptr,
                               &p_ftdi_fnct->BulkInEP);
        if (*p_err != USBH_ERR_NONE) {
            USBH_FTDI_Disconn(p_ftdi_fnct);

            return ((void *)0);
        }

        p_ftdi_fnct->MaxPktSize = (CPU_INT32U)USBH_EP_MaxPktSizeGet(&p_ftdi_fnct->BulkInEP);

       *p_err = USBH_BulkOutOpen(p_ftdi_fnct->DevPtr,           /* Open Bulk OUT EP.                                    */
                                 p_ftdi_fnct->IF_Ptr,
                                &p_ftdi_fnct->BulkOutEP);
        if (*p_err != USBH_ERR_NONE) {
            USBH_FTDI_Disconn(p_ftdi_fnct);

            return ((void *)0);
        }
    }

    return ((void *)p_ftdi_fnct);
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_Suspend()
*
* Description : Suspend FTDI device.
*
* Argument(s) : p_class_dev       Pointer to FTDI device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FTDI_Suspend (void  *p_class_dev)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;


    p_ftdi_fnct = (USBH_FTDI_FNCT *)p_class_dev;

    p_ftdi_fnct->State = USBH_CLASS_DEV_STATE_SUSPEND;
}


/*
*********************************************************************************************************
*                                          USBH_FTDI_Resume()
*
* Description : Resume FTDI device.
*
* Argument(s) : p_class_dev       Pointer to FTDI device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FTDI_Resume (void  *p_class_dev)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;


    p_ftdi_fnct = (USBH_FTDI_FNCT *)p_class_dev;

    p_ftdi_fnct->State = USBH_CLASS_DEV_STATE_CONN;
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_Disconn()
*
* Description : Handle disconnection of FTDI device.
*
* Argument(s) : p_class_dev       Pointer to FTDI device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FTDI_Disconn (void  *p_class_dev)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    LIB_ERR          err_lib;


    p_ftdi_fnct = (USBH_FTDI_FNCT *)p_class_dev;

    (void)USBH_EP_Close(&p_ftdi_fnct->BulkInEP);                /* Close bulk EPs.                                      */
    (void)USBH_EP_Close(&p_ftdi_fnct->BulkOutEP);

    p_ftdi_fnct->State = USBH_CLASS_DEV_STATE_DISCONN;

    if (USBH_FTDI_DevPtrPrev == p_ftdi_fnct->DevPtr) {
        USBH_FTDI_DevPtrPrev = (USBH_DEV *)0;
    }
    USBH_FTDI_PortCnt = 0u;

    Mem_PoolBlkFree(       &USBH_FTDI_DevPool,
                    (void *)p_ftdi_fnct,
                           &err_lib);
}


/*
*********************************************************************************************************
*                                     USBH_FTDI_ClassDevNotify()
*
* Description : Handle device state change notification.
*
* Argument(s) : p_class_dev,      Pointer to FTDI device.
*
*               state,            State of the FTDI device.
*
*               p_ctx             Pointer to FTDI functions.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FTDI_ClassDevNotify (void        *p_class_dev,
                                        CPU_INT08U   state,
                                        void        *p_ctx)
{
    USBH_FTDI_CALLBACKS  *p_ftdi_callbacks;
    USBH_FTDI_FNCT       *p_ftdi_fnct;


    p_ftdi_callbacks = (USBH_FTDI_CALLBACKS *)p_ctx;
    p_ftdi_fnct      = (USBH_FTDI_FNCT      *)p_class_dev;

    switch (state) {
        case USBH_CLASS_DEV_STATE_CONN:
             p_ftdi_callbacks->ConnNotifyPtr(p_ftdi_fnct->Handle);
             break;


        case USBH_CLASS_DEV_STATE_DISCONN:
             p_ftdi_callbacks->DisconnNotifyPtr(p_ftdi_fnct->Handle);
             break;


        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                         USBH_FTDI_StdReq()
*
* Description : Send standard FTDI control request to device.
*
* Argument(s) : ftdi_handle,      Pointer to FTDI device.
*
*               request,          FTDI control request type.
*
*               value,            Value of the wValue field.
*
*               h_index,          High-Byte value of the wIndex field.
*
*               p_err             Variable that will receive the return error code from this function.
*
*                                 USBH_ERR_NONE,                          Control transfer successfully transmitted.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'ftdi_handle'.
*                                 USBH_ERR_DEV_NOT_READY,                 Device is not ready.
*
*                                                                         -----     RETURNED BY USBH_CtrlTx() :    -----
*                                 USBH_ERR_NONE,                          Control transfer successfully completed.
*                                 USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                                 USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                                 USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                                 USBH_ERR_HC_IO,                         Root hub input/output error.
*                                 USBH_ERR_EP_STALL,                      Root hub does not support request.
*                                 Host controller drivers error code      Otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FTDI_StdReq (USBH_FTDI_HANDLE   ftdi_handle,
                                CPU_INT08U         request,
                                CPU_INT16U         value,
                                CPU_INT08U         h_index,
                                USBH_ERR          *p_err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;
    CPU_INT08U       ftdi_ix;
    CPU_INT16U       index;


    ftdi_ix = USBH_FTDI_HANDLE_IX_GET(ftdi_handle);
    if (ftdi_ix >= USBH_FTDI_CFG_MAX_DEV) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    p_ftdi_fnct = &USBH_FTDI_DevTbl[ftdi_ix];
    if (p_ftdi_fnct->DevPtr == (void *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return;
    }

    if (p_ftdi_fnct->State != USBH_CLASS_DEV_STATE_CONN) {
       *p_err = USBH_ERR_DEV_NOT_READY;
        return;
    }

    index = (CPU_INT16U)(((CPU_INT16U)h_index << 8u) | p_ftdi_fnct->Port);

    (void)USBH_CtrlTx(        p_ftdi_fnct->DevPtr,              /* Send ctrl req.                                       */
                              request,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_VENDOR | USBH_REQ_RECIPIENT_DEV),
                              value,
                              index,
                      (void *)0,
                              0u,
                              USBH_CFG_STD_REQ_TIMEOUT,
                              p_err);
}


/*
*********************************************************************************************************
*                                       USBH_FTDI_DataTxCmpl()
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
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_FTDI_DataTxCmpl (USBH_EP     *p_ep,
                                    void        *p_buf,
                                    CPU_INT32U   buf_len,
                                    CPU_INT32U   xfer_len,
                                    void        *p_arg,
                                    USBH_ERR     err)
{
    USBH_FTDI_FNCT  *p_ftdi_fnct;


    p_ftdi_fnct = (USBH_FTDI_FNCT *)p_arg;

    if (err != USBH_ERR_NONE) {                                 /* Chk status of transaction.                           */
        (void)USBH_EP_Reset(p_ftdi_fnct->DevPtr,
                            p_ep);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(p_ep);
        }
    }

    p_ftdi_fnct->DataTxNotifyPtr(p_ftdi_fnct->Handle,           /* Notify the app.                                      */
                                 p_ftdi_fnct->DataTxNotifyArgPtr,
                                 p_buf,
                                 xfer_len,
                                 err);
}


/*
*********************************************************************************************************
*                                       USBH_FTDI_DataRxCmpl()
*
* Description : Asynchronous data receive completion function.
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
* Return(s)   : None.
*
* Note(s)     : (1) The received data in 'p_buf' is in little-endian.
*
*               (2) The FTDI device always add a status of two bytes at the beginning of every packet.
*                   This function manage those status bytes in order to get the latest status followed
*                   by the data in 'p_buf'.
*                   This only applies when the application requests more than max packet size (MPS) - 2 bytes of data.
*
*                                        +----+----+----+----+-----+---------+---------+-----+-----
*   (a) Receive first pkt in buffer.     | S0 | S1 | D0 | D1 | ... | MPS - 2 | MPS - 1 | MPS | ...
*                                        +----+----+----+----+-----+---------+---------+-----+-----
*                                                                                 |       |
*                                                                                 v       v
*                                                                            +---------+-----+
*   (b) Save the last two bytes of data.                                     | MPS - 1 | MPS |
*                                                                            +---------+-----+
*
*                                        +----+----+----+----+-----+---------+-----+-----+---------+---------+-----
*   (c) Receive next pkt in buffer.      | S0 | S1 | D0 | D1 | ... | MPS - 2 | S2  | S3  | MPS + 1 | MPS + 2 | ...
*                                        +----+----+----+----+-----+---------+-----+-----+---------+---------+-----
*
*                                        +----+----+----+----+-----+---------+---------+-----+---------+---------+-----
*   (d) Update the status.               | S2 | S3 | D0 | D1 | ... | MPS - 2 |   S2    | S3  | MPS + 1 | MPS + 2 | ...
*                                        +----+----+----+----+-----+---------+---------+-----+---------+---------+-----
*                                                                                 ^       ^
*                                                                                 |       |
*                                                                            +---------+-----+
*   (e) Restore the two bytes of data.                                       | MPS - 1 | MPS |
*                                                                            +---------+-----+
*
*   (f) Repeat step (b) to (e) until the end of the buffer is reached or the last transfer size is
*       smaller than maxPktSize.
*
*********************************************************************************************************
*/

static  void  USBH_FTDI_DataRxCmpl (USBH_EP     *p_ep,
                                    void        *p_buf,
                                    CPU_INT32U   buf_len,
                                    CPU_INT32U   xfer_len,
                                    void        *p_arg,
                                    USBH_ERR     err)
{
    USBH_FTDI_FNCT           *p_ftdi_fnct;
    CPU_INT08U               *p_buf_cur;
    CPU_INT32U                rem_buf_size;
    USBH_FTDI_SERIAL_STATUS  *p_serial_status;


    p_ftdi_fnct = (USBH_FTDI_FNCT *)p_arg;

    if (err == USBH_ERR_NONE) {
        p_serial_status =  p_ftdi_fnct->SerialStatusPtr;
        p_buf_cur       = (CPU_INT08U *)p_buf;

        if (p_serial_status != (void *)0) {
            p_serial_status->ModemStatus = *(p_buf_cur);
            p_serial_status->LineStatus  = *(p_buf_cur + 1u);
                                                                /* Chk line status.                                     */
            if ((DEF_BIT_IS_SET(p_serial_status->LineStatus, USBH_FTDI_LINE_STATUS_RX_OVERFLOW_ERROR) == DEF_YES) ||
                (DEF_BIT_IS_SET(p_serial_status->LineStatus, USBH_FTDI_LINE_STATUS_PARITY_ERROR)      == DEF_YES) ||
                (DEF_BIT_IS_SET(p_serial_status->LineStatus, USBH_FTDI_LINE_STATUS_FRAMING_ERROR)     == DEF_YES)) {

                MEM_VAL_COPY_16(p_ftdi_fnct->DataRxBuf, p_buf_cur);
                err = USBH_ERR_FTDI_LINE;
            }
        }
    } else {                                                    /* Chk status of transaction.                           */
        (void)USBH_EP_Reset(p_ftdi_fnct->DevPtr,
                            p_ep);

        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(p_ep);
        }
    }

    if (err == USBH_ERR_NONE) {                                 /* Organize data to get last two bytes status followed  */
                                                                /* by raw data rx'd.                                    */
        if (p_ftdi_fnct->XferLen > (p_ftdi_fnct->MaxPktSize - 1u)) {
            MEM_VAL_COPY_16(p_ftdi_fnct->DataRxBuf, p_buf_cur); /* Update status.                                       */
            MEM_VAL_COPY_16(p_buf_cur, &p_ftdi_fnct->LastBytes);/* Restore two bytes of data.                           */
            p_ftdi_fnct->XferLen += (xfer_len - USBH_FTDI_SERIAL_STATUS_LEN);

        } else {
            p_ftdi_fnct->XferLen += xfer_len;
        }

        p_buf_cur    +=  p_ftdi_fnct->MaxPktSize;
        rem_buf_size  = (p_ftdi_fnct->RxBufLen - p_ftdi_fnct->XferLen);

        if ((xfer_len                == p_ftdi_fnct->MaxPktSize) &&
            (p_ftdi_fnct->MaxPktSize <= rem_buf_size)) {
                                                                /* Save last two bytes of data.                         */
            MEM_VAL_COPY_16(&p_ftdi_fnct->LastBytes, (p_buf_cur - USBH_FTDI_SERIAL_STATUS_LEN));
                                                                /* Req following pkt.                                   */
            err = USBH_BulkRxAsync(       &p_ftdi_fnct->BulkInEP,
                                          (p_buf_cur - USBH_FTDI_SERIAL_STATUS_LEN),
                                           buf_len,
                                           USBH_FTDI_DataRxCmpl,
                                   (void *)p_ftdi_fnct);
            if (err != USBH_ERR_NONE) {                         /* Reset EP if an err occurred.                         */
                (void)USBH_EP_Reset(p_ftdi_fnct->DevPtr,
                                   &p_ftdi_fnct->BulkOutEP);

                if (err == USBH_ERR_EP_STALL) {
                    (void)USBH_EP_StallClr(&p_ftdi_fnct->BulkOutEP);
                }
            } else {
                return;
            }
        }
    }

    p_ftdi_fnct->DataRxNotifyPtr(p_ftdi_fnct->Handle,           /* Notify the app.                                      */
                                 p_ftdi_fnct->DataRxNotifyArgPtr,
                                 p_ftdi_fnct->DataRxBuf,
                                 p_ftdi_fnct->XferLen,
                                 p_ftdi_fnct->SerialStatusPtr,
                                 err);
}



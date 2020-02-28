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
*                                       USB HOST HUB OPERATIONS
*
* Filename : usbh_hub.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_HUB_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_hub.h"
#include  "usbh_core.h"
#include  "usbh_class.h"


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
********************************************************************************************************
*                                     ROOT HUB DEVICE DESCRIPTOR
********************************************************************************************************
*/

static  const  CPU_INT08U  USBH_HUB_RH_DevDesc[18] = {
    USBH_LEN_DESC_DEV,                                          /* bLength                                              */
    USBH_DESC_TYPE_DEV,                                         /* bDescriptorType: Device                              */
    0x10u, 0x01u,                                               /* bcdUSB: v1.1                                         */
    USBH_CLASS_CODE_HUB,                                        /* bDeviceClass: HUB_CLASSCODE                          */
    USBH_SUBCLASS_CODE_USE_IF_DESC,                             /* bDeviceSubClass                                      */
    USBH_PROTOCOL_CODE_USE_IF_DESC,                             /* bDeviceProtocol                                      */
    0x40u,                                                      /* bMaxPacketSize0: 64 Bytes                            */
    0x00u, 0x00u,                                               /* idVendor                                             */
    0x00u, 0x00u,                                               /* idProduct                                            */
    0x00u, 0x00u,                                               /* bcdDevice                                            */
    0x00u,                                                      /* iManufacturer                                        */
    0x00u,                                                      /* iProduct                                             */
    0x00u,                                                      /* iSerialNumber                                        */
    0x01u                                                       /* bNumConfigurations                                   */
};


/*
********************************************************************************************************
*                                  ROOT HUB CONFIGURATION DESCRIPTOR
********************************************************************************************************
*/

static  const  CPU_INT08U  USBH_HUB_RH_FS_CfgDesc[] = {
                                                                /* ------------- CONFIGURATION DESCRIPTOR ------------- */
    USBH_LEN_DESC_CFG,                                          /* bLength                                              */
    USBH_DESC_TYPE_CFG,                                         /* bDescriptorType CONFIGURATION                        */
    0x19u, 0x00u,                                               /* le16 wTotalLength                                    */
    0x01u,                                                      /* bNumInterfaces                                       */
    0x01u,                                                      /* bConfigurationValue                                  */
    0x00u,                                                      /* iConfiguration                                       */
    0xC0u,                                                      /* bmAttributes -> Self-powered|Remote wakeup           */
    0x00u,                                                      /* bMaxPower                                            */

                                                                /* --------------- INTERFACE DESCRIPTOR --------------- */
    USBH_LEN_DESC_IF,                                           /* bLength                                              */
    USBH_DESC_TYPE_IF,                                          /* bDescriptorType: Interface                           */
    0x00u,                                                      /* bInterfaceNumber                                     */
    0x00u,                                                      /* bAlternateSetting                                    */
    0x01u,                                                      /* bNumEndpoints                                        */
    USBH_CLASS_CODE_HUB,                                        /* bInterfaceClass HUB_CLASSCODE                        */
    0x00u,                                                      /* bInterfaceSubClass                                   */
    0x00u,                                                      /* bInterfaceProtocol                                   */
    0x00u,                                                      /* iInterface                                           */

                                                                /* --------------- ENDPOINT DESCRIPTOR ---------------- */
    USBH_LEN_DESC_EP,                                           /* bLength                                              */
    USBH_DESC_TYPE_EP,                                          /* bDescriptorType: Endpoint                            */
    0x81u,                                                      /* bEndpointAddress: IN Endpoint 1                      */
    0x03u,                                                      /* bmAttributes Interrupt                               */
    0x08u, 0x00u,                                               /* wMaxPacketSize                                       */
    0x1u                                                        /* bInterval                                            */
};


/*
********************************************************************************************************
*                                     ROOT HUB STRING DESCRIPTOR
*
* Note(s):  (1) A USB device can store strings in multiple languages. i.e. the string index defines which
*               word or phrase should be communicated to the user and the LANGID code identifies to a
*               device, which language to retrieve that word or phase for. The LANGIDs are defined in the
*               document called "Language Identifiers (LANGIDs), 3/29/00, Version 1.0" available at
*               http://www.usb.org/developers/docs/USB_LANGIDs.pdf.
********************************************************************************************************
*/

static  const  CPU_INT08U  USBH_HUB_RH_LangID[] = {
    0x04u,
    USBH_DESC_TYPE_STR,
    0x09u, 0x04u,                                                /* Identifer for English (United States). See Note #1.  */
};


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

static            CPU_INT08U     USBH_HUB_DescBuf[USBH_HUB_MAX_DESC_LEN];
static            USBH_HUB_DEV   USBH_HUB_Arr[USBH_CFG_MAX_HUBS];
static            MEM_POOL       USBH_HUB_Pool;

static  volatile  USBH_HUB_DEV  *USBH_HUB_HeadPtr;
static  volatile  USBH_HUB_DEV  *USBH_HUB_TailPtr;
static  volatile  USBH_HSEM      USBH_HUB_EventSem;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void       USBH_HUB_GlobalInit      (USBH_ERR              *p_err);

static  void      *USBH_HUB_IF_Probe        (USBH_DEV              *p_dev,
                                             USBH_IF               *p_if,
                                             USBH_ERR              *p_err);

static  void       USBH_HUB_Suspend         (void                  *p_class_dev);

static  void       USBH_HUB_Resume          (void                  *p_class_dev);

static  void       USBH_HUB_Disconn         (void                  *p_class_dev);

static  USBH_ERR   USBH_HUB_Init            (USBH_HUB_DEV          *p_hub_dev);

static  void       USBH_HUB_Uninit          (USBH_HUB_DEV          *p_hub_dev);

static  USBH_ERR   USBH_HUB_EP_Open         (USBH_HUB_DEV          *p_hub_dev);

static  void       USBH_HUB_EP_Close        (USBH_HUB_DEV          *p_hub_dev);

static  USBH_ERR   USBH_HUB_EventReq        (USBH_HUB_DEV          *p_hub_dev);

static  void       USBH_HUB_ISR             (USBH_EP               *p_ep,
                                             void                  *p_buf,
                                             CPU_INT32U             buf_len,
                                             CPU_INT32U             xfer_len,
                                             void                  *p_arg,
                                             USBH_ERR               err);

static  void       USBH_HUB_EventProcess    (void);

static  USBH_ERR   USBH_HUB_DescGet         (USBH_HUB_DEV          *p_hub_dev);

static  USBH_ERR   USBH_HUB_PortsInit       (USBH_HUB_DEV          *p_hub_dev);

static  USBH_ERR   USBH_HUB_PortStatusGet   (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr,
                                             USBH_HUB_PORT_STATUS  *p_port_status);

static  USBH_ERR   USBH_HUB_PortResetSet    (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  USBH_ERR   USBH_HUB_PortRstChngClr  (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  USBH_ERR   USBH_HUB_PortEnChngClr   (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  USBH_ERR   USBH_HUB_PortConnChngClr (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  USBH_ERR   USBH_HUB_PortPwrSet      (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  USBH_ERR   USBH_HUB_PortSuspendClr  (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  USBH_ERR   USBH_HUB_PortEnClr       (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  USBH_ERR   USBH_HUB_PortEnSet       (USBH_HUB_DEV          *p_hub_dev,
                                             CPU_INT16U             port_nbr);

static  void       USBH_HUB_Clr             (USBH_HUB_DEV          *p_hub_dev);

static  USBH_ERR   USBH_HUB_RefAdd          (USBH_HUB_DEV          *p_hub_dev);

static  USBH_ERR   USBH_HUB_RefRel          (USBH_HUB_DEV          *p_hub_dev);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          HUB CLASS DRIVER
*********************************************************************************************************
*/

USBH_CLASS_DRV  USBH_HUB_Drv = {
    (CPU_INT08U *)"HUB",
                   USBH_HUB_GlobalInit,
                   0,
                   USBH_HUB_IF_Probe,
                   USBH_HUB_Suspend,
                   USBH_HUB_Resume,
                   USBH_HUB_Disconn
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
*                                        USBH_HUB_EventTask()
*
* Description : Task that process HUB Events.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_HUB_EventTask (void  *p_arg)
{
    (void)p_arg;

    while (DEF_TRUE) {
        (void)USBH_OS_SemWait(USBH_HUB_EventSem, 0u);

        USBH_HUB_EventProcess();
    }
}


/*
*********************************************************************************************************
*                                         USBH_HUB_PortDis()
*
* Description : Disable given port on hub.
*
* Argument(s) : p_hub_dev       Pointer to hub.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          If port is successfully disabled.
*               USBH_ERR_INVALID_ARG,                   If invalid parameter passed to 'p_hub_dev'.
*
*                                                       ----- RETURNED BY USBH_HUB_PortEnClr() : -----
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

USBH_ERR  USBH_HUB_PortDis (USBH_HUB_DEV  *p_hub_dev,
                            CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    if (p_hub_dev == (USBH_HUB_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HUB_PortEnClr(p_hub_dev, port_nbr);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_HUB_PortEn()
*
* Description : Enable given port on hub.
*
* Argument(s) : p_hub_dev       Pointer to hub.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          If port is successfully enabled.
*               USBH_ERR_INVALID_ARG,                   If invalid parameter passed to 'p_hub_dev'.
*
*                                                       ----- RETURNED BY USBH_HUB_PortEnSet() : -----
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

USBH_ERR  USBH_HUB_PortEn (USBH_HUB_DEV  *p_hub_dev,
                           CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    if (p_hub_dev == (USBH_HUB_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_HUB_PortEnSet(p_hub_dev, port_nbr);

    return (err);
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
*                                        USBH_HUB_GlobalInit()
*
* Description : Initializes all USBH_HUB_DEV structures, device lists and HUB pool.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                   USBH_ERR_NONE                   If initialization is successful.
*                   USBH_ERR_ALLOC                  If failed to create hub memory pool.
*
*                                                   ----- RETURNED BY USBH_OS_SemCreate() : -----
*                   USBH_ERR_OS_SIGNAL_CREATE,      If semaphore creation failed.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HUB_GlobalInit (USBH_ERR  *p_err)
{
    CPU_INT08U  hub_ix;
    CPU_SIZE_T  octets_reqd;
    LIB_ERR     err_lib;


    for (hub_ix = 0u; hub_ix < USBH_CFG_MAX_HUBS; hub_ix++) {   /* Clr all HUB dev structs.                             */
        USBH_HUB_Clr(&USBH_HUB_Arr[hub_ix]);
    }

    Mem_PoolCreate (       &USBH_HUB_Pool,                      /* POOL for managing hub dev structs.                   */
                    (void *)USBH_HUB_Arr,
                            sizeof(USBH_HUB_DEV) * USBH_CFG_MAX_HUBS,
                            USBH_CFG_MAX_HUBS,
                            sizeof(USBH_HUB_DEV),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

   *p_err = USBH_OS_SemCreate((USBH_HSEM *)&USBH_HUB_EventSem,
                                            0u);

    USBH_HUB_HeadPtr = (USBH_HUB_DEV *)0;
    USBH_HUB_TailPtr = (USBH_HUB_DEV *)0;

    Mem_Clr((void *)USBH_HUB_DescBuf,
                    USBH_HUB_MAX_DESC_LEN);
}


/*
*********************************************************************************************************
*                                         USBH_HUB_IF_Probe()
*
* Description : Determine whether connected device implements hub class by examining it's interface
*               descriptor.
*
* Argument(s) : p_dev   Pointer to device.
*
*               p_if    Pointer to interface.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                   USBH_ERR_NONE                           Device implements hub class.
*                   USBH_ERR_DEV_ALLOC                      No more space in hub memory pool.
*                   USBH_ERR_CLASS_DRV_NOT_FOUND            Device does not implement hub class.
*
*                                                           ----- RETURNED BY USBH_IF_DescGet() : -----
*                   USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'alt_ix'.
*
*                                                           ----- RETURNED BY USBH_HUB_Init() : -----
*                   USBH_ERR_DESC_INVALID,                  if hub descriptor is invalid.
*                   USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                   USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                   USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                   USBH_ERR_HC_IO,                         Root hub input/output error.
*                   USBH_ERR_EP_STALL,                      Root hub does not support request.
*                   USBH_ERR_EP_ALLOC,                      If USBH_CFG_MAX_NBR_EPS reached.
*                   USBH_ERR_EP_NOT_FOUND,                  If endpoint with given type and direction not found.
*                   USBH_ERR_OS_SIGNAL_CREATE,              if mutex or semaphore creation failed.
*                   USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                   USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                   USBH_ERR_HC_IO,                         Root hub input/output error.
*                   USBH_ERR_EP_INVALID_TYPE                If endpoint type is not interrupt or direction is not IN.
*                   USBH_ERR_ALLOC                          If URB cannot be allocated.
*                   Host controller drivers error code,     Otherwise.
*
* Return(s)   : Pointer to hub device structure,        If probe is successful.
*               0,                                      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  *USBH_HUB_IF_Probe (USBH_DEV  *p_dev,
                                  USBH_IF   *p_if,
                                  USBH_ERR  *p_err)
{
    USBH_HUB_DEV  *p_hub_dev;
    USBH_IF_DESC   if_desc;
    LIB_ERR        err_lib;


    p_hub_dev = (USBH_HUB_DEV *)0;
   *p_err     =  USBH_IF_DescGet(p_if,                          /* Get IF desc.                                         */
                                 0u,
                                &if_desc);
    if (*p_err != USBH_ERR_NONE) {
        return ((void *)0);
    }

    if (if_desc.bInterfaceClass == USBH_CLASS_CODE_HUB) {       /* If IF is HUB, alloc HUB dev.                         */
        p_hub_dev = (USBH_HUB_DEV *)Mem_PoolBlkGet(&USBH_HUB_Pool,
                                                    sizeof(USBH_HUB_DEV),
                                                   &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
           *p_err = USBH_ERR_DEV_ALLOC;
            return ((void *)0);
        }

        USBH_HUB_Clr(p_hub_dev);
        USBH_HUB_RefAdd(p_hub_dev);

        p_hub_dev->State  = (CPU_INT08U)USBH_CLASS_DEV_STATE_CONN;
        p_hub_dev->DevPtr =  p_dev;
        p_hub_dev->IF_Ptr =  p_if;
        p_hub_dev->ErrCnt =  0u;

        if ((p_dev->IsRootHub            == DEF_TRUE) &&
            (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE)) {
            p_dev->HC_Ptr->RH_ClassDevPtr = p_hub_dev;
        }

       *p_err = USBH_HUB_Init(p_hub_dev);                       /* Init HUB.                                            */
        if (*p_err != USBH_ERR_NONE) {
            USBH_HUB_RefRel(p_hub_dev);
        }
    } else {
       *p_err = USBH_ERR_CLASS_DRV_NOT_FOUND;
    }

    return ((void *)p_hub_dev);
}


/*
*********************************************************************************************************
*                                         USBH_HUB_Suspend()
*
* Description : Suspend given hub and all devices connected to it.
*
* Argument(s) : p_class_dev     Pointer to hub structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HUB_Suspend (void  *p_class_dev)
{
    CPU_INT16U     nbr_ports;
    CPU_INT16U     port_ix;
    USBH_DEV      *p_dev;
    USBH_HUB_DEV  *p_hub_dev;


    p_hub_dev = (USBH_HUB_DEV *)p_class_dev;
    nbr_ports =  DEF_MIN(p_hub_dev->Desc.bNbrPorts,
                         USBH_CFG_MAX_HUB_PORTS);

    for (port_ix = 0u; port_ix < nbr_ports; port_ix++) {
        p_dev = (USBH_DEV *)p_hub_dev->DevPtrList[port_ix];

        if (p_dev != (USBH_DEV *)0){
            USBH_ClassSuspend(p_dev);
        }
    }
}


/*
*********************************************************************************************************
*                                          USBH_HUB_Resume()
*
* Description : Resume given hub and all devices connected to it.
*
* Argument(s) : p_class_dev     Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : (1) According to 'Universal Serial Bus Specification Revision 2.0', Section 11.5.1.10,
*                   "[the resuming state] is a timed state with a nominal duration of 20ms"
*********************************************************************************************************
*/

static  void  USBH_HUB_Resume (void  *p_class_dev)
{
    CPU_INT16U             nbr_ports;
    CPU_INT16U             port_ix;
    USBH_DEV              *p_dev;
    USBH_HUB_DEV          *p_hub_dev;
    USBH_HUB_PORT_STATUS   port_status;


    p_hub_dev = (USBH_HUB_DEV *)p_class_dev;
    nbr_ports =  DEF_MIN(p_hub_dev->Desc.bNbrPorts,
                         USBH_CFG_MAX_HUB_PORTS);

    for (port_ix = 0u; port_ix < nbr_ports; port_ix++) {
        USBH_HUB_PortSuspendClr(p_hub_dev, port_ix + 1u);   /* En resume signaling on port.                         */
    }

    USBH_OS_DlyMS(20u + 12u);                               /* See Note (1).                                        */

    for (port_ix = 0u; port_ix < nbr_ports; port_ix++) {
        p_dev = p_hub_dev->DevPtrList[port_ix];

        if (p_dev != (USBH_DEV *)0){
            USBH_ClassResume(p_dev);
        } else {                                            /* Get port status info.                                */
            (void)USBH_HUB_PortStatusGet(p_hub_dev,
                                         port_ix + 1u,
                                        &port_status);

            if ((port_status.wPortStatus & USBH_HUB_STATUS_PORT_CONN) != 0u) {
                (void)USBH_HUB_PortResetSet(p_hub_dev,
                                            port_ix + 1u);
            }
        }
    }
}


/*
*********************************************************************************************************
*                                         USBH_HUB_Disconn()
*
* Description : Disconnect given hub.
*
* Argument(s) : p_class_dev      Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HUB_Disconn (void  *p_class_dev)
{
    USBH_HUB_DEV  *p_hub_dev;


    p_hub_dev        = (USBH_HUB_DEV *)p_class_dev;
    p_hub_dev->State =  USBH_CLASS_DEV_STATE_DISCONN;

    USBH_HUB_Uninit(p_hub_dev);
    USBH_HUB_RefRel(p_hub_dev);
}


/*
*********************************************************************************************************
*                                           USBH_HUB_Init()
*
* Description : Opens the endpoints, reads hub descriptor, initializes ports and submits request to
*               start receiving hub events.
*
* Argument(s) : p_hub_dev       Pointer to hub structure.
*
* Return(s)   : USBH_ERR_NONE,                          If hub is successfully initialized.
*
*                                                       ----- RETURNED BY USBH_HUB_DescGet() : -----
*               USBH_ERR_DESC_INVALID,                  if hub descriptor is invalid.
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_HUB_EP_Open() : -----
*               USBH_ERR_EP_ALLOC,                      If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,                  If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,              if mutex or semaphore creation failed.
*               Host controller drivers error,          Otherwise.
*
*                                                       ----- RETURNED BY USBH_HUB_PortsInit() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_HUB_EventReq() : -----
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

static  USBH_ERR  USBH_HUB_Init (USBH_HUB_DEV  *p_hub_dev)
{
    USBH_ERR  err;


    err = USBH_HUB_EP_Open(p_hub_dev);                          /* Open intr EP.                                        */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_HUB_DescGet(p_hub_dev);                          /* Get hub desc.                                        */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_HUB_PortsInit(p_hub_dev);                        /* Init hub ports.                                      */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_HUB_EventReq(p_hub_dev);                         /* Start receiving hub evts.                            */

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_HUB_Uninit()
*
* Description : Uninitialize given hub.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HUB_Uninit (USBH_HUB_DEV  *p_hub_dev)
{
    CPU_INT16U   nbr_ports;
    CPU_INT16U   port_ix;
    USBH_DEV    *p_dev;
    LIB_ERR      err;


    USBH_HUB_EP_Close(p_hub_dev);
    nbr_ports = DEF_MIN(p_hub_dev->Desc.bNbrPorts,
                        USBH_CFG_MAX_HUB_PORTS);

    for (port_ix = 0u; port_ix < nbr_ports; port_ix++) {
        p_dev = (USBH_DEV *)p_hub_dev->DevPtrList[port_ix];

        if (p_dev != (USBH_DEV *)0){
            USBH_DevDisconn(p_dev);
            Mem_PoolBlkFree(       &p_hub_dev->DevPtr->HC_Ptr->HostPtr->DevPool,
                            (void *)p_dev,
                                   &err);

            p_hub_dev->DevPtrList[port_ix] = (USBH_DEV *)0;
        }
    }
}


/*
*********************************************************************************************************
*                                         USBH_HUB_EP_Open()
*
* Description : Open interrupt endpoint required to receive hub events.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
* Return(s)   : USBH_ERR_NONE,                      if the endpoints are opened.
*
*                                                   ----- RETURNED BY USBH_IntrInOpen() : -----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_EP_Open (USBH_HUB_DEV  *p_hub_dev)
{
    USBH_ERR   err;
    USBH_DEV  *p_dev;
    USBH_IF   *p_if;


    p_dev = p_hub_dev->DevPtr;
    p_if  = p_hub_dev->IF_Ptr;

    err = USBH_IntrInOpen(p_dev,                                /* Find and open hub intr EP.                           */
                          p_if,
                         &p_hub_dev->IntrEP);

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_HUB_EP_Close()
*
* Description : Close interrupt endpoint.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HUB_EP_Close (USBH_HUB_DEV  *p_hub_dev)
{
    USBH_EP_Close(&p_hub_dev->IntrEP);                          /* Close hub intr EP.                                   */
}


/*
*********************************************************************************************************
*                                         USBH_HUB_EventReq()
*
* Description : Issue an asynchronous interrupt request to receive hub events.
*
* Argument(s) : p_hub_dev            Pointer to the hub device structure.
*
* Return(s)   : USBH_ERR_NONE                           If hub events are started
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
* Note(s)     : (1) Hub reports only as many bits as there are ports on the hub,
*                   subject to the byte-granularity requirement (i.e., round up to the nearest byte)
*                  (See section 11.12.4, USB 2.0 spec)
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_EventReq (USBH_HUB_DEV  *p_hub_dev)
{
    CPU_INT32U       len;
    CPU_BOOLEAN      valid;
    USBH_HC_RH_API  *p_rh_api;
    USBH_ERR         err;
    USBH_DEV        *p_dev;


    p_dev = p_hub_dev->DevPtr;

    if ((p_dev->IsRootHub            == DEF_TRUE) &&            /* Chk if RH fncts are supported before calling HCD.    */
        (p_dev->HC_Ptr->IsVirRootHub == DEF_TRUE)) {

        p_rh_api = p_dev->HC_Ptr->HC_Drv.RH_API_Ptr;
        valid    = p_rh_api->IntEn(&p_dev->HC_Ptr->HC_Drv);
        if (valid != DEF_OK) {
            return (USBH_ERR_HC_IO);
        } else {
            return (USBH_ERR_NONE);
        }
    }

    len = (p_hub_dev->Desc.bNbrPorts / 8u) + 1u;                /* See Note (1).                                        */
    err = USBH_IntrRxAsync(       &p_hub_dev->IntrEP,           /* Start receiving hub events.                          */
                           (void *)p_hub_dev->HubIntrBuf,
                                   len,
                                   USBH_HUB_ISR,
                           (void *)p_hub_dev);
    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_HUB_ISR()
*
* Description : Handles hub interrupt.
*
* Argument(s) : p_ep            Pointer to endpoint.
*
*               p_buf           Pointer to data buffer.
*
*               buf_len         Buffer length in octets.
*
*               xfer_len        Transfered length.
*
*               p_arg           Context Pointer.
*
*               err             Error code.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HUB_ISR (USBH_EP     *p_ep,
                            void        *p_buf,
                            CPU_INT32U   buf_len,
                            CPU_INT32U   xfer_len,
                            void        *p_arg,
                            USBH_ERR     err)
{
    USBH_HUB_DEV  *p_hub_dev;
    CPU_SR_ALLOC();


    (void)buf_len;
    (void)p_buf;
    (void)p_ep;
    (void)xfer_len;

    p_hub_dev = (USBH_HUB_DEV *)p_arg;

    if (err != USBH_ERR_NONE) {
        if (p_hub_dev->State == USBH_CLASS_DEV_STATE_CONN) {
            if (p_hub_dev->ErrCnt < 3u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                USBH_PRINT_LOG("USBH_HUB_ISR() fails. Err=%d errcnt=%d\r\n",
                                err,
                                p_hub_dev->ErrCnt);
#endif

                p_hub_dev->ErrCnt++;
                USBH_HUB_EventReq(p_hub_dev);                   /* Retry URB.                                           */
            }
        }
        return;
    }

    p_hub_dev->ErrCnt = 0u;

    USBH_HUB_RefAdd(p_hub_dev);

    CPU_CRITICAL_ENTER();
    if (USBH_HUB_HeadPtr == (USBH_HUB_DEV *)0) {
        USBH_HUB_HeadPtr = USBH_HUB_TailPtr = p_hub_dev;
    } else {
        USBH_HUB_TailPtr->NxtPtr = p_hub_dev;
        USBH_HUB_TailPtr         = p_hub_dev;
    }
    CPU_CRITICAL_EXIT();

    (void)USBH_OS_SemPost(USBH_HUB_EventSem);
}


/*
*********************************************************************************************************
*                                       USBH_HUB_EventProcess()
*
* Description : Determine status of each of hub ports. Newly connected device will be reset & configured.
*               Appropriate notifications & cleanup will be performed if a device has been disconnected.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : (1) Some device require a delay following connection detection. This is related to the
*                   debounce interval which ensures that power is stable at the device for at least 100 ms
*                   before any requests will be sent to the device (See section 11.8.2, USB 2.0 spec).
*
*               (2) Open Host Controller Interface specification Release 1.0a states that Port Reset Status
*                   Change bit is set at the end of 10 ms port reset signal. See section 7.4.4, PRSC field.
*********************************************************************************************************
*/

static  void  USBH_HUB_EventProcess (void)
{
    CPU_INT16U             nbr_ports;
    CPU_INT16U             port_nbr;
    MEM_POOL              *p_dev_pool;
    LIB_ERR                err_lib;
    USBH_DEV_SPD           dev_spd;
    USBH_HUB_DEV          *p_hub_dev;
    USBH_HUB_PORT_STATUS   port_status;
    USBH_DEV              *p_dev;
    USBH_ERR               err;
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    p_hub_dev = (USBH_HUB_DEV *)USBH_HUB_HeadPtr;

    if (USBH_HUB_HeadPtr == USBH_HUB_TailPtr) {
        USBH_HUB_HeadPtr = (USBH_HUB_DEV *)0;
        USBH_HUB_TailPtr = (USBH_HUB_DEV *)0;
    } else {
        USBH_HUB_HeadPtr = USBH_HUB_HeadPtr->NxtPtr;
    }
    CPU_CRITICAL_EXIT();

    if (p_hub_dev == (USBH_HUB_DEV *)0) {
        return;
    }

    if (p_hub_dev->State == USBH_CLASS_DEV_STATE_DISCONN) {
        err = USBH_HUB_RefRel(p_hub_dev);
        if (err != USBH_ERR_NONE) {
            USBH_PRINT_ERR(err);
        }
        return;
    }

    port_nbr   =  1u;
    p_dev_pool = &p_hub_dev->DevPtr->HC_Ptr->HostPtr->DevPool;
    nbr_ports  =  DEF_MIN(p_hub_dev->Desc.bNbrPorts,
                          USBH_CFG_MAX_HUB_PORTS);

    while (port_nbr <= nbr_ports) {

        err  = USBH_HUB_PortStatusGet(p_hub_dev,                /* Get port status info..                               */
                                      port_nbr,
                                     &port_status);
        if (err != USBH_ERR_NONE) {
            break;
        }
                                                                /* ------------- CONNECTION STATUS CHANGE ------------- */
        if (DEF_BIT_IS_SET(port_status.wPortChange, USBH_HUB_STATUS_C_PORT_CONN) == DEF_TRUE) {

            err = USBH_HUB_PortConnChngClr(p_hub_dev,           /* Clr port conn chng.                                  */
                                           port_nbr);
            if (err != USBH_ERR_NONE) {
                break;
            }
                                                                /* -------------- DEV HAS BEEN CONNECTED -------------- */
            if (DEF_BIT_IS_SET(port_status.wPortStatus, USBH_HUB_STATUS_PORT_CONN) == DEF_TRUE) {

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                USBH_PRINT_LOG("Port %d : Device Connected.\r\n", port_nbr);
#endif

                p_hub_dev->ConnCnt = 0;                         /* Reset re-connection counter                          */
                p_dev              = p_hub_dev->DevPtrList[port_nbr - 1u];
                if (p_dev != (USBH_DEV *)0) {
                    USBH_DevDisconn(p_dev);
                    Mem_PoolBlkFree(        p_dev_pool,
                                    (void *)p_dev,
                                           &err_lib);
                    p_hub_dev->DevPtrList[port_nbr - 1u] = (USBH_DEV *)0;
                }

                USBH_OS_DlyMS(100u);                            /* See Notes #1.                                        */
                err = USBH_HUB_PortResetSet(p_hub_dev,          /* Apply port reset.                                    */
                                            port_nbr);
                if (err != USBH_ERR_NONE) {
                    break;
                }

                USBH_OS_DlyMS(USBH_HUB_DLY_DEV_RESET);          /* See Notes #2.                                        */
                continue;                                       /* Handle port reset status change.                     */

            } else {                                            /* --------------- DEV HAS BEEN REMOVED --------------- */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                USBH_PRINT_LOG("Port %d : Device Removed.\r\n", port_nbr);
#endif
                USBH_OS_DlyMS(10u);                             /* Wait for any pending I/O xfer to rtn err.            */

                p_dev = p_hub_dev->DevPtrList[port_nbr - 1u];

                if (p_dev != (USBH_DEV *)0) {
                    USBH_DevDisconn(p_dev);
                    Mem_PoolBlkFree(        p_dev_pool,
                                    (void *)p_dev,
                                           &err_lib);

                    p_hub_dev->DevPtrList[port_nbr - 1u] = (USBH_DEV *)0;
                }
            }
        }
                                                                /* ------------- PORT RESET STATUS CHANGE ------------- */
        if (DEF_BIT_IS_SET(port_status.wPortChange, USBH_HUB_STATUS_C_PORT_RESET) == DEF_TRUE) {

            err = USBH_HUB_PortRstChngClr(p_hub_dev, port_nbr);
            if (err != USBH_ERR_NONE) {
                break;
            }
                                                                /* Dev has been connected.                              */
            if (DEF_BIT_IS_SET(port_status.wPortStatus, USBH_HUB_STATUS_PORT_CONN) == DEF_TRUE) {

                err = USBH_HUB_PortStatusGet(p_hub_dev,         /* Get port status info.                                */
                                             port_nbr,
                                            &port_status);
                if (err != USBH_ERR_NONE) {
                    break;
                }

                                                                /* Determine dev spd.                                   */
                if (DEF_BIT_IS_SET(port_status.wPortStatus, USBH_HUB_STATUS_PORT_LOW_SPD) == DEF_TRUE) {
                    dev_spd = USBH_DEV_SPD_LOW;
                } else if (DEF_BIT_IS_SET(port_status.wPortStatus, USBH_HUB_STATUS_PORT_HIGH_SPD) == DEF_TRUE) {
                    dev_spd = USBH_DEV_SPD_HIGH;
                } else {
                    dev_spd = USBH_DEV_SPD_FULL;
                }

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                USBH_PRINT_LOG("Port %d : Port Reset complete, device speed is %s\r\n", port_nbr,
                                                         (dev_spd == USBH_DEV_SPD_LOW)  ? "LOW Speed(1.5 Mb/Sec)" :
                                                         (dev_spd == USBH_DEV_SPD_FULL) ? "FULL Speed(12 Mb/Sec)" :
                                                                                          "HIGH Speed(480 Mb/Sec)");
#endif

                p_dev = p_hub_dev->DevPtrList[port_nbr - 1u];

                if (p_dev != (USBH_DEV *)0) {
                    continue;
                }

                if (p_hub_dev->DevPtr->HC_Ptr->HostPtr->State == USBH_HOST_STATE_SUSPENDED) {
                    continue;
                }

                p_dev = (USBH_DEV *)Mem_PoolBlkGet(p_dev_pool,
                                                   sizeof(USBH_DEV),
                                                  &err_lib);
                if (err_lib != LIB_MEM_ERR_NONE) {
                    USBH_HUB_PortDis(p_hub_dev, port_nbr);
                    USBH_HUB_RefRel(p_hub_dev);

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                    USBH_PRINT_LOG("ERROR: Cannot allocate device.\r\n");
#endif
                    USBH_HUB_EventReq(p_hub_dev);               /* Retry URB.                                           */

                    return;
                }

                p_dev->DevSpd    = dev_spd;
                p_dev->HubDevPtr = p_hub_dev->DevPtr;
                p_dev->PortNbr   = port_nbr;
                p_dev->HC_Ptr    = p_hub_dev->DevPtr->HC_Ptr;

                if (dev_spd == USBH_DEV_SPD_HIGH) {
                    p_dev->HubHS_Ptr = p_hub_dev;
                } else {
                    if (p_hub_dev->IntrEP.DevSpd == USBH_DEV_SPD_HIGH) {
                        p_dev->HubHS_Ptr = p_hub_dev;
                    } else {
                        p_dev->HubHS_Ptr = p_hub_dev->DevPtr->HubHS_Ptr;
                    }
                }

                USBH_OS_DlyMS(50u);

                err = USBH_DevConn(p_dev);                      /* Conn dev.                                            */
                if (err != USBH_ERR_NONE) {
                    USBH_PRINT_ERR(err);

                    USBH_HUB_PortDis(p_hub_dev, port_nbr);
                    USBH_DevDisconn(p_dev);

                    Mem_PoolBlkFree(        p_dev_pool,
                                    (void *)p_dev,
                                           &err_lib);

                    if (p_hub_dev->ConnCnt < USBH_CFG_MAX_NUM_DEV_RECONN) {
                                                                /*This condition may happen due to EP_STALL return      */
                         err = USBH_HUB_PortResetSet(p_hub_dev, /* Apply port reset.                                    */
                                                     port_nbr);
                         if (err != USBH_ERR_NONE) {
                             break;
                         }

                         USBH_OS_DlyMS(USBH_HUB_DLY_DEV_RESET); /* See Notes #2.                                        */
                         p_hub_dev->ConnCnt++;;
                         continue;                              /* Handle port reset status change.                     */

                     } else {
                         p_hub_dev->DevPtrList[port_nbr - 1u] = (USBH_DEV *)0;
                     }
                } else {
                    p_hub_dev->DevPtrList[port_nbr - 1u] = p_dev;
                }
            }
        }
                                                                /* ------------ PORT ENABLE STATUS CHANGE ------------- */
        if (DEF_BIT_IS_SET(port_status.wPortChange, USBH_HUB_STATUS_C_PORT_EN) == DEF_TRUE) {
            err = USBH_HUB_PortEnChngClr(p_hub_dev, port_nbr);
            if (err != USBH_ERR_NONE) {
                break;
            }
        }
        port_nbr++;
    }

    USBH_HUB_EventReq(p_hub_dev);                               /* Retry URB.                                           */

    USBH_HUB_RefRel(p_hub_dev);
}


/*
*********************************************************************************************************
*                                         USBH_HUB_DescGet()
*
* Description : Retrieve hub descriptor.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
* Return(s)   : USBH_ERR_NONE,                          if descriptor is successfully obtained.
*               USBH_ERR_DESC_INVALID,                  if hub descriptor is invalid.
*
*                                                       ----- RETURNED BY USBH_CtrlRx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) GET HUB DESCRIPTOR standard request is defined in USB2.0 specification,
*                   section 11.24.2.5. Read 2 bytes of hub descriptor. Offset 0 of Hub Descriptor
*                   contains total length of descriptor and offset 1 represents descriptor type.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_DescGet (USBH_HUB_DEV  *p_hub_dev)
{
    USBH_ERR       err;
    CPU_INT32U     len;
    CPU_INT32U     i;
    USBH_DESC_HDR  hdr;


    for (i = 0u; i < USBH_CFG_STD_REQ_RETRY; i++) {             /* Attempt to get desc hdr 3 times.                     */
        len = USBH_CtrlRx(         p_hub_dev->DevPtr,
                                   USBH_REQ_GET_DESC,           /* See Note (1).                                        */
                                  (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS),
                                  (USBH_HUB_DESC_TYPE_HUB << 8u),
                                   0u,
                          (void *)&hdr,
                                   USBH_LEN_DESC_HDR,
                                   USBH_HUB_TIMEOUT,
                                  &err);
        if ((err == USBH_ERR_EP_STALL) ||
            (len == 0u               )) {
            USBH_EP_Reset(           p_hub_dev->DevPtr,
                          (USBH_EP *)0);
        } else {
            break;
        }
    }

    if (len != USBH_LEN_DESC_HDR) {
        return (USBH_ERR_DESC_INVALID);
    }

    if ((hdr.bLength         == 0u                    ) ||
        (hdr.bLength          > USBH_HUB_MAX_DESC_LEN ) ||
        (hdr.bDescriptorType != USBH_HUB_DESC_TYPE_HUB)) {
        return (USBH_ERR_DESC_INVALID);
    }

    for (i = 0u; i < USBH_CFG_STD_REQ_RETRY; i++) {             /* Attempt to get full desc 3 times.                    */
        len = USBH_CtrlRx(        p_hub_dev->DevPtr,
                                  USBH_REQ_GET_DESC,
                                 (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS),
                                 (USBH_HUB_DESC_TYPE_HUB << 8),
                                  0u,
                          (void *)USBH_HUB_DescBuf,
                                  hdr.bLength,
                                  USBH_HUB_TIMEOUT,
                                 &err);
        if ((err == USBH_ERR_EP_STALL) ||
            (len <  hdr.bLength      )) {
            USBH_EP_Reset(           p_hub_dev->DevPtr,
                          (USBH_EP *)0);
        } else {
            break;
        }
    }

    USBH_HUB_ParseHubDesc(&p_hub_dev->Desc,
                           USBH_HUB_DescBuf);

    if (p_hub_dev->Desc.bNbrPorts > USBH_CFG_MAX_HUB_PORTS) {   /* Warns limit on hub port nbr to max cfg'd.            */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("Only ports [1..%d] are active.\r\n", USBH_CFG_MAX_HUB_PORTS);
#endif
    }

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_HUB_PortsInit()
*
* Description : Enable power on each hub port & initialize device on each port.
*
* Argument(s) : p_hub_dev       Pointer to the hub.
*
* Return(s)   : USBH_ERR_NONE,                          if ports were successfully initialized.
*
*                                                       ----- RETURNED BY USBH_HUB_PortPwrSet() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) USB2.0 specification states that the host must wait for (bPwrOn2PwrGood * 2) ms
*                   before accessing a powered port. See section 11.23.2.1, bPwrOn2PwrGood field in hub
*                   descriptor.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortsInit (USBH_HUB_DEV  *p_hub_dev)
{
    USBH_ERR    err;
    CPU_INT32U  i;
    CPU_INT16U  nbr_ports;


    nbr_ports = DEF_MIN(p_hub_dev->Desc.bNbrPorts,
                        USBH_CFG_MAX_HUB_PORTS);

    for (i = 0u; i < nbr_ports; i++) {
        err = USBH_HUB_PortPwrSet(p_hub_dev, i + 1u);           /* Set port pwr.                                      */

        if (err != USBH_ERR_NONE) {
            USBH_PRINT_ERR(err);
            return (err);
        }
        USBH_OS_DlyMS(p_hub_dev->Desc.bPwrOn2PwrGood * 2u);     /* See Note (1).                                       */
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                      USBH_HUB_PortStatusGet()
*
* Description : Get port status on given hub.
*
* Argument(s) : p_ep             Pointer to hub device.
*
*               port_nbr         Port number.
*
*               p_port_status    Variable that will receive port status.
*
* Return(s)   : USBH_ERR_NONE,                          if request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlRx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) HUB class specific request GET PORT STATUS is defined in USB2.0 specification in
*                   section 11.24.2.7.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortStatusGet (USBH_HUB_DEV          *p_hub_dev,
                                          CPU_INT16U             port_nbr,
                                          USBH_HUB_PORT_STATUS  *p_port_status)
{
    CPU_INT08U            *p_buf;
    USBH_HUB_PORT_STATUS   port_status;
    USBH_ERR               err;


    p_buf = (CPU_INT08U *)&port_status;

    (void)USBH_CtrlRx(        p_hub_dev->DevPtr,                /* See Note (1).                                        */
                              USBH_REQ_GET_STATUS,
                             (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              0u,
                              port_nbr,
                      (void *)p_buf,
                              USBH_HUB_LEN_HUB_PORT_STATUS,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    } else {
        p_port_status->wPortStatus = MEM_VAL_GET_INT16U_LITTLE(&port_status.wPortStatus);
        p_port_status->wPortChange = MEM_VAL_GET_INT16U_LITTLE(&port_status.wPortChange);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                       USBH_HUB_PortResetSet()
*
* Description : Set port reset on given hub.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The HUB class specific request SET PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.13
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortResetSet (USBH_HUB_DEV  *p_hub_dev,
                                         CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note (1).                                        */
                              USBH_REQ_SET_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_PORT_RESET,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_HUB_PortRstChngClr()
*
* Description : Clear port reset change on given hub.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          If request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) HUB class specific request CLEAR PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.2.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortRstChngClr (USBH_HUB_DEV  *p_hub_dev,
                                           CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note (1).                                        */
                              USBH_REQ_CLR_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_C_PORT_RESET,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_HUB_PortEnChngClr()
*
* Description : Clear port enable change on given hub.
*
* Argument(s) : p_hub_dev        Pointer to hub device.
*
*               port_nbr         Port number.
*
* Return(s)   : USBH_ERR_NONE,                          if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) HUB class specific request CLEAR PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.2.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortEnChngClr (USBH_HUB_DEV  *p_hub_dev,
                                          CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,            /* See Note (1).                                        */
                              USBH_REQ_CLR_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_C_PORT_EN,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                     USBH_HUB_PortConnChngClr()
*
* Description : Clear port connection change on given hub.
*
* Argument(s) : p_hub_dev        Pointer to hub device.
*
*               port_nbr         Port number.
*
* Return(s)   : USBH_ERR_NONE,                          if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The HUB class specific request CLEAR PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.2.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortConnChngClr (USBH_HUB_DEV  *p_hub_dev,
                                            CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note #1.                                         */
                              USBH_REQ_CLR_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_C_PORT_CONN,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_HUB_PortPwrSet()
*
* Description : Set power on given hub and port.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The HUB class specific request SET PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.13.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortPwrSet (USBH_HUB_DEV  *p_hub_dev,
                                       CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note #1.                                         */
                              USBH_REQ_SET_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_PORT_PWR,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_HUB_PortSuspendClr()
*
* Description : Clear port suspend on given hub.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The HUB class specific request CLEAR PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.2.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortSuspendClr (USBH_HUB_DEV  *p_hub_dev,
                                           CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note #1.                                         */
                              USBH_REQ_CLR_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_C_PORT_SUSPEND,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_HUB_PortEnClr()
*
* Description : Clear port enable on given hub.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The HUB class specific request CLEAR PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.2
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortEnClr (USBH_HUB_DEV  *p_hub_dev,
                                      CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note #1.                                         */
                              USBH_REQ_CLR_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_PORT_EN,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_HUB_PortEnSet()
*
* Description : Set port enable on given hub.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,       if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The HUB class specific request SET PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.13.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_PortEnSet (USBH_HUB_DEV  *p_hub_dev,
                                      CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note #1.                                         */
                              USBH_REQ_SET_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_PORT_EN,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_HUB_PortSuspendSet()
*
* Description : Set port suspend on given hub.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
*               port_nbr        Port number.
*
* Return(s)   : USBH_ERR_NONE,                          if the request was successfully sent.
*
*                                                       ----- RETURNED BY USBH_CtrlTx() : -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The HUB class specific request SET PORT FEATURE is defined in USB2.0 specification in
*                   section 11.24.2.13.
*********************************************************************************************************
*/

USBH_ERR  USBH_HUB_PortSuspendSet (USBH_HUB_DEV  *p_hub_dev,
                                   CPU_INT16U     port_nbr)
{
    USBH_ERR  err;


    (void)USBH_CtrlTx(        p_hub_dev->DevPtr,                /* See Note #1.                                         */
                              USBH_REQ_SET_FEATURE,
                             (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_OTHER),
                              USBH_HUB_FEATURE_SEL_PORT_SUSPEND,
                              port_nbr,
                      (void *)0,
                              0u,
                              USBH_HUB_TIMEOUT,
                             &err);
    if (err != USBH_ERR_NONE) {
        USBH_EP_Reset(p_hub_dev->DevPtr, (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_HUB_Clr()
*
* Description : Initializes USBH_HUB_DEV structure.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
* Return(s)   : None
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HUB_Clr (USBH_HUB_DEV  *p_hub_dev)
{
    CPU_INT08U  dev_ix;


    p_hub_dev->DevPtr = (USBH_DEV *)0;
    p_hub_dev->IF_Ptr = (USBH_IF  *)0;
                                                                /* Clr dev ptr lst.                                     */
    for (dev_ix = 0u; dev_ix < USBH_CFG_MAX_HUB_PORTS; dev_ix++) {
        p_hub_dev->DevPtrList[dev_ix] = (USBH_DEV *)0;
    }

    p_hub_dev->RefCnt = 0u;
    p_hub_dev->State  = USBH_CLASS_DEV_STATE_NONE;
    p_hub_dev->NxtPtr = 0u;
}


/*
*********************************************************************************************************
*                                          USBH_HUB_RefAdd()
*
* Description : Increment access reference count to a hub device.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
* Return(s)   : USBH_ERR_NONE,          if access is successful.
*               USBH_ERR_INVALID_ARG,   if invalid argument passed to 'p_hub_dev'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_RefAdd (USBH_HUB_DEV  *p_hub_dev)
{
    CPU_SR_ALLOC();


    if (p_hub_dev == (USBH_HUB_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    CPU_CRITICAL_ENTER();
    p_hub_dev->RefCnt++;                                        /* Increment access ref cnt to hub dev.                 */
    CPU_CRITICAL_EXIT();

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          USBH_HUB_RefRel()
*
* Description : Increment the access reference count to a hub device.
*
* Argument(s) : p_hub_dev       Pointer to hub device.
*
* Return(s)   : USBH_ERR_NONE,          if access is successful.
*               USBH_ERR_INVALID_ARG,   if invalid argument passed to 'p_hub_dev'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HUB_RefRel (USBH_HUB_DEV  *p_hub_dev)
{
    LIB_ERR  err;
    CPU_SR_ALLOC();


    if (p_hub_dev == (USBH_HUB_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    CPU_CRITICAL_ENTER();
    if (p_hub_dev->RefCnt > 0u) {
        p_hub_dev->RefCnt--;                                    /* Decrement access ref cnt to hub dev.                 */

        if (p_hub_dev->RefCnt == 0u) {
            Mem_PoolBlkFree(       &USBH_HUB_Pool,              /* Ref count is 0, dev is removed, release HUB dev.     */
                            (void *)p_hub_dev,
                                   &err);
        }
    }
    CPU_CRITICAL_EXIT();

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HUB_RH_CtrlReq()
*
* Description : Process hub request from core layer.
*
* Argument(s) : p_hc            Pointer to host controller.
*
*               b_req           Setup packet b_request.
*
*               bm_req_type     Setup packet bm_request_type.
*
*               w_val           Setup packet w_value.
*
*               w_ix            Setup packet w_index.
*
*               p_buf           Data buffer pointer.
*
*               buf_len         Size of data buffer in bytes.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE,          Root hub control request completed successfully.
*                           USBH_ERR_HC_IO,         Root hub input/output error.
*                           USBH_ERR_EP_STALL,      Root hub does not support request.
*
* Return(s)   : Buffer length in octets.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_HUB_RH_CtrlReq (USBH_HC     *p_hc,
                                 CPU_INT08U   b_req,
                                 CPU_INT08U   bm_req_type,
                                 CPU_INT16U   w_val,
                                 CPU_INT16U   w_ix,
                                 void        *p_buf,
                                 CPU_INT32U   buf_len,
                                 USBH_ERR    *p_err)
{
    CPU_INT32U       len;
    USBH_HC_DRV     *p_hc_drv;
    USBH_HC_RH_API  *p_hc_rh_api;
    CPU_BOOLEAN      valid;


    p_hc_drv    = &p_hc->HC_Drv;
    p_hc_rh_api =  p_hc_drv->RH_API_Ptr;
   *p_err       =  USBH_ERR_NONE;
    len         =  0u;
    valid       =  DEF_OK;

    switch (b_req) {
        case USBH_REQ_GET_STATUS:
                                                                /* Only port status is supported.                       */
             if ((bm_req_type & USBH_REQ_RECIPIENT_OTHER) == USBH_REQ_RECIPIENT_OTHER) {
                 valid = p_hc_rh_api->PortStatusGet(                        p_hc_drv,
                                                                            w_ix,
                                                    (USBH_HUB_PORT_STATUS *)p_buf);
             } else {
                 len = buf_len;
                 Mem_Clr(p_buf, len);                           /* Return 0 for other reqs.                             */
             }
             break;

        case USBH_REQ_CLR_FEATURE:
             switch (w_val) {
                 case USBH_HUB_FEATURE_SEL_PORT_EN:
                      valid = p_hc_rh_api->PortEnClr(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_PORT_PWR:
                      valid = p_hc_rh_api->PortPwrClr(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_C_PORT_CONN:
                      valid = p_hc_rh_api->PortConnChngClr(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_C_PORT_RESET:
                      valid = p_hc_rh_api->PortResetChngClr(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_C_PORT_EN:
                      valid = p_hc_rh_api->PortEnChngClr(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_PORT_INDICATOR:
                 case USBH_HUB_FEATURE_SEL_PORT_SUSPEND:
                 case USBH_HUB_FEATURE_SEL_C_PORT_SUSPEND:
                      valid = p_hc_rh_api->PortSuspendClr(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_C_PORT_OVER_CUR:
                     *p_err = USBH_ERR_EP_STALL;
                      break;

                 default:
                      break;
             }
             break;


        case USBH_REQ_SET_FEATURE:
             switch (w_val) {
                 case USBH_HUB_FEATURE_SEL_PORT_EN:
                      valid = p_hc_rh_api->PortEnSet(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_PORT_RESET:
                      valid = p_hc_rh_api->PortResetSet(p_hc_drv, w_ix);
                      break;

                 case USBH_HUB_FEATURE_SEL_PORT_PWR:
                      valid = p_hc_rh_api->PortPwrSet(p_hc_drv, w_ix);
                      break;
                                                                /* Not supported reqs.                                  */
                 case USBH_HUB_FEATURE_SEL_PORT_SUSPEND:
                 case USBH_HUB_FEATURE_SEL_PORT_TEST:
                 case USBH_HUB_FEATURE_SEL_PORT_INDICATOR:
                 case USBH_HUB_FEATURE_SEL_C_PORT_CONN:
                 case USBH_HUB_FEATURE_SEL_C_PORT_RESET:
                 case USBH_HUB_FEATURE_SEL_C_PORT_EN:
                 case USBH_HUB_FEATURE_SEL_C_PORT_SUSPEND:
                 case USBH_HUB_FEATURE_SEL_C_PORT_OVER_CUR:
                     *p_err = USBH_ERR_EP_STALL;
                      break;

                 default:
                      break;
             }
             break;

        case USBH_REQ_SET_ADDR:
             break;

        case USBH_REQ_GET_DESC:
             switch (w_val >> 8u) {                             /* Desc type.                                           */
                 case USBH_DESC_TYPE_DEV:
                      if (buf_len > sizeof(USBH_HUB_RH_DevDesc)) {
                          len = sizeof(USBH_HUB_RH_DevDesc);
                      } else {
                          len = buf_len;
                      }

                      Mem_Copy((void *)p_buf,
                               (void *)USBH_HUB_RH_DevDesc,
                                       len);
                      break;

                 case USBH_DESC_TYPE_CFG:                       /* Return cfg desc.                                     */
                      if (buf_len > sizeof(USBH_HUB_RH_FS_CfgDesc)) {
                          len = sizeof(USBH_HUB_RH_FS_CfgDesc);
                      } else {
                          len = buf_len;
                      }
                      Mem_Copy((void *)p_buf,
                               (void *)USBH_HUB_RH_FS_CfgDesc,
                                       len);
                      break;

                 case USBH_HUB_DESC_TYPE_HUB:                   /* Return hub desc.                                     */
                      len   = buf_len;
                      valid = p_hc_rh_api->HubDescGet(                 p_hc_drv,
                                                      (USBH_HUB_DESC *)p_buf,
                                                                       len);
                      break;

                 case USBH_DESC_TYPE_STR:

                      if ((w_val & 0x00FF) == 0u) {
                          if (buf_len > sizeof(USBH_HUB_RH_LangID)) {
                              len = sizeof(USBH_HUB_RH_LangID);
                          } else {
                              len = buf_len;
                          }
                          Mem_Copy((void *)p_buf,
                                   (void *)USBH_HUB_RH_LangID,
                                           len);
                      } else {
                         *p_err = USBH_ERR_EP_STALL;
                          break;
                      }
                      break;

                 default:
                      break;
             }
             break;

        case USBH_REQ_SET_CFG:
             break;


        case USBH_REQ_GET_CFG:
        case USBH_REQ_GET_IF:
        case USBH_REQ_SET_IF:
        case USBH_REQ_SET_DESC:
        case USBH_REQ_SYNCH_FRAME:
            *p_err = USBH_ERR_EP_STALL;
             break;

        default:
             break;
    }

    if ((valid  != DEF_OK) &&
        (*p_err == USBH_ERR_NONE)) {
       *p_err = USBH_ERR_HC_IO;
    }

    return (len);
}


/*
*********************************************************************************************************
*                                         USBH_HUB_RH_Event()
*
* Description : Queue a root hub event.
*
* Argument(s) : p_dev       Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_HUB_RH_Event (USBH_DEV  *p_dev)
{
    USBH_HUB_DEV    *p_hub_dev;
    USBH_HC_DRV     *p_hc_drv;
    USBH_HC_RH_API  *p_rh_drv_api;
    CPU_SR_ALLOC();


    p_hub_dev    =  p_dev->HC_Ptr->RH_ClassDevPtr;
    p_hc_drv     = &p_dev->HC_Ptr->HC_Drv;
    p_rh_drv_api =  p_hc_drv->RH_API_Ptr;
    if (p_hub_dev == (USBH_HUB_DEV *)0) {
        (void)p_rh_drv_api->IntDis(p_hc_drv);
        return;
    }

    (void)p_rh_drv_api->IntDis(p_hc_drv);
    USBH_HUB_RefAdd(p_hub_dev);

    CPU_CRITICAL_ENTER();
    if (USBH_HUB_HeadPtr == (USBH_HUB_DEV *)0) {
        USBH_HUB_HeadPtr = p_hub_dev;
        USBH_HUB_TailPtr = p_hub_dev;

    } else {
        USBH_HUB_TailPtr->NxtPtr = p_hub_dev;
        USBH_HUB_TailPtr         = p_hub_dev;
    }
    CPU_CRITICAL_EXIT();

    (void)USBH_OS_SemPost(USBH_HUB_EventSem);
}


/*
**************************************************************************************************************
*                                       USBH_HUB_ClassNotify()
*
* Description : Handle device state change notification for hub class devices.
*
* Argument(s) : p_class_dev     Pointer to class device.
*
*               state           State of device.
*
*               p_ctx           Pointer to context.
*
* Return(s)   : None.
*
* Note(s)     : None.
**************************************************************************************************************
*/

void  USBH_HUB_ClassNotify (void        *p_class_dev,
                            CPU_INT08U   state,
                            void        *p_ctx)
{
    USBH_HUB_DEV  *p_hub_dev;
    USBH_DEV      *p_dev;


    (void)p_ctx;

    p_hub_dev = (USBH_HUB_DEV *)p_class_dev;
    p_dev     =  p_hub_dev->DevPtr;

    if(p_dev->IsRootHub == DEF_TRUE) {                          /* If RH, return immediately.                           */
        return;
    }

    switch (state) {                                            /* External hub has been identified.                    */
        case USBH_CLASS_DEV_STATE_CONN:
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
             USBH_PRINT_LOG("Ext HUB (Addr# %i) connected\r\n", p_dev->DevAddr);
#endif
             break;

        case USBH_CLASS_DEV_STATE_DISCONN:
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
             USBH_PRINT_LOG("Ext HUB (Addr# %i) disconnected\r\n", p_dev->DevAddr);
#endif
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                        USBH_HUB_ParseHubDesc()
*
* Description : Parse hub descriptor into hub descriptor structure.
*
* Argument(s) : p_hub_desc      Variable that will hold the parsed hub descriptor.
*
*               p_buf_src       Pointer to buffer that holds hub descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_HUB_ParseHubDesc (USBH_HUB_DESC  *p_hub_desc,
                             void           *p_buf_src)
{
    USBH_HUB_DESC  *p_buf_src_desc;
    CPU_INT08U      i;


    p_buf_src_desc = (USBH_HUB_DESC *)p_buf_src;

    p_hub_desc->bDescLength         = p_buf_src_desc->bDescLength;
    p_hub_desc->bDescriptorType     = p_buf_src_desc->bDescriptorType;
    p_hub_desc->bNbrPorts           = p_buf_src_desc->bNbrPorts;
    p_hub_desc->wHubCharacteristics = MEM_VAL_GET_INT16U_LITTLE(&p_buf_src_desc->wHubCharacteristics);
    p_hub_desc->bPwrOn2PwrGood      = p_buf_src_desc->bPwrOn2PwrGood;
    p_hub_desc->bHubContrCurrent    = p_buf_src_desc->bHubContrCurrent;
    p_hub_desc->DeviceRemovable     = p_buf_src_desc->DeviceRemovable;

    for (i = 0u; i < USBH_CFG_MAX_HUB_PORTS; i++) {
        p_hub_desc->PortPwrCtrlMask[i] = MEM_VAL_GET_INT32U_LITTLE(&p_buf_src_desc->PortPwrCtrlMask[i]);
    }
}


/*
*********************************************************************************************************
*                                        USBH_HUB_FmtHubDesc()
*
* Description : Format hub descriptor from hub descriptor structure.
*
* Argument(s) : p_hub_desc       Variable that holds hub descriptor information.
*
*               p_buf_dest       Pointer to buffer that will contain hub descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_HUB_FmtHubDesc (USBH_HUB_DESC  *p_hub_desc,
                           void           *p_buf_dest)
{
    USBH_HUB_DESC  *p_buf_dest_desc;
    CPU_INT08U      i;


    p_buf_dest_desc = (USBH_HUB_DESC *)p_buf_dest;

    p_buf_dest_desc->bDescLength         = p_hub_desc->bDescLength;
    p_buf_dest_desc->bDescriptorType     = p_hub_desc->bDescriptorType;
    p_buf_dest_desc->bNbrPorts           = p_hub_desc->bNbrPorts;
    p_buf_dest_desc->wHubCharacteristics = MEM_VAL_GET_INT16U_LITTLE(&p_hub_desc->wHubCharacteristics);
    p_buf_dest_desc->bPwrOn2PwrGood      = p_hub_desc->bPwrOn2PwrGood;
    p_buf_dest_desc->bHubContrCurrent    = p_hub_desc->bHubContrCurrent;
    p_buf_dest_desc->DeviceRemovable     = p_hub_desc->DeviceRemovable;

    for (i = 0u; i < USBH_CFG_MAX_HUB_PORTS; i++) {
        p_buf_dest_desc->PortPwrCtrlMask[i] = p_hub_desc->PortPwrCtrlMask[i];
    }
}

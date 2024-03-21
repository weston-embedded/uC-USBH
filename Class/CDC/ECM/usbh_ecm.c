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
*                                    ETHERNET CONTROL MODEL (ECM)
*
* Filename : usbh_ecm.c
* Version  : V3.42.01
*********************************************************************************************************
*/

#define  USBH_CDC_ECM_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   MICRIUM_SOURCE
#include  "usbh_ecm.h"


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
*                          ETHERNET CONTROL MANAGEMENT DESCRIPTOR DATA TYPE
*
* Note(s) : (1) See 'Subclass Specification for Ethernet Control Model Devices Revision 1.2', Section 5.4, Table 3.
*********************************************************************************************************
*/

typedef  struct  usb_cdc_ecm_desc {
    CPU_INT08U  bFunctionLength;                                /* Desc len in bytes.                                   */
    CPU_INT08U  bDescriptorType;                                /* IF desc type.                                        */
    CPU_INT08U  bDescriptorSubtype;                             /* ECM functional desc subtype.                         */
    CPU_INT08U  iMACAddress;                                    /* Index to MAC address string descriptor.              */
    CPU_INT32U  bmEthernetStatistics;                           /* Bitmap of supported statistics.                      */
    CPU_INT16U  wMaxSegmentSize;                                /* Maximum segment size, typically 1514.                */
    CPU_INT16U  wNumberMCFilters;                               /* Number of multicast filters supported.               */
    CPU_INT08U  bNumberPowerFilters;                            /* Number of power filters supported.                   */
} USBH_CDC_ECM_DESC;


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

static  MEM_POOL          USBH_CDC_ECM_DevPool;
static  USBH_CDC_ECM_DEV  USBH_CDC_ECM_DevArr[USBH_CDC_ECM_CFG_MAX_DEV];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CDC_ECM_DescParse          (USBH_CDC_ECM_DESC    *p_cdc_ecm_desc,
                                                   USBH_IF              *p_if);

static  void      USBH_CDC_ECM_EventRxCmpl        (void                 *p_context,
                                                   CPU_INT08U           *p_buf,
                                                   CPU_INT32U            xfer_len,
                                                   USBH_ERR              err);


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
*                                      USBH_CDC_ECM_GlobalInit()
*
* Description : Initializes all the USB CDC ECM structures and global variables
*
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE,      if success
*               USBH_ERR_ALLOC,     if memory pool creation fails
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ECM_GlobalInit (void)
{
    CPU_SIZE_T  octets_reqd;
    CPU_SIZE_T  dev_size;
    USBH_ERR    err;
    LIB_ERR     err_lib;


    dev_size = sizeof(USBH_CDC_ECM_DEV) * USBH_CDC_ECM_CFG_MAX_DEV;

    Mem_Clr((void *)USBH_CDC_ECM_DevArr, dev_size);             /* Reset all ECM dev structures.                        */

    Mem_PoolCreate (       &USBH_CDC_ECM_DevPool,               /* Create mem pool for ECM dev struct.                  */
                    (void *)USBH_CDC_ECM_DevArr,
                            dev_size,
                            USBH_CDC_ECM_CFG_MAX_DEV,
                            sizeof(USBH_CDC_ECM_DEV),
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
*                                         USBH_CDC_ECM_Add()
*
* Description : Allocates memory for a CDC ECM device structure, registers a callback function to receive
*               interrupt events, reads ECM descriptor from the interface, reads MAC address and sets configuration
*               and alternate interface to allow network traffic.
*
* Argument(s) : p_cdc_dev       Pointer to the USB CDC device.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE               CDC ECM device added successfully.
*                               USBH_ERR_ALLOC              Device allocation failed.
*
*                                                           ---- RETURNED BY USBH_CDC_ECM_DescParse ----
*                               USBH_ERR_DESC_INVALID,      Descriptor contains invalid value(s). This may be the case
*                                                           for other CDC classes.
*
*                                                           ---- RETURNED BY USBH_CfgSet/USBH_SET_IF ----
*                               USBH_ERR_UNKNOWN            Unknown error occurred.
*                               USBH_ERR_INVALID_ARG        Invalid argument passed to 'p_ep'.
*                               USBH_ERR_EP_INVALID_STATE   Endpoint is not opened.
*                               USBH_ERR_HC_IO,             Root hub input/output error.
*                               USBH_ERR_EP_STALL,          Root hub does not support request.
*
* Return(s)   : Pointer to CDC ECM device.
*
* Note(s)     : (1) ECM 1.2 Section 3.3, select alternate interface for network traffic. Default interface does not
*                   have any endpoints and does not support data transfers.
*********************************************************************************************************
*/

USBH_CDC_ECM_DEV  *USBH_CDC_ECM_Add (USBH_CDC_DEV  *p_cdc_dev,
                                     USBH_ERR      *p_err)
{
    USBH_CDC_ECM_DEV   *p_cdc_ecm_dev;
    USBH_IF            *p_cif;
    USBH_CDC_ECM_DESC   ecm_desc;
    LIB_ERR             err_lib;


    if (p_cdc_dev == (USBH_CDC_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return ((USBH_CDC_ECM_DEV *)0);
    }
                                                                                /* Allocate CDC ECM dev from mem pool.  */
    p_cdc_ecm_dev = (USBH_CDC_ECM_DEV *)Mem_PoolBlkGet(&USBH_CDC_ECM_DevPool,
                                                        sizeof(USBH_CDC_ECM_DEV),
                                                       &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return ((USBH_CDC_ECM_DEV *)0);
    }

    p_cdc_ecm_dev->CDC_DevPtr = p_cdc_dev;

    p_cif = USBH_CDC_CommIF_Get(p_cdc_ecm_dev->CDC_DevPtr);
   *p_err = USBH_CDC_ECM_DescParse(&ecm_desc, p_cif);                           /* Get ECM desc from IF.                */
    if (*p_err != USBH_ERR_NONE) {
        goto end_free_return_empty;
    }

                                                                                /* Get MAC address from string desc.    */
    USBH_StrGet(p_cdc_dev->DevPtr,ecm_desc.iMACAddress, 0, p_cdc_ecm_dev->MAC_Addr,
                sizeof(p_cdc_ecm_dev->MAC_Addr), p_err);
    if (*p_err != USBH_ERR_NONE) {
        goto end_free_return_empty;
    }

                                                                                /* Save parameters.                     */
    p_cdc_ecm_dev->Parameters.EthernetStatistics = ecm_desc.bmEthernetStatistics;
    p_cdc_ecm_dev->Parameters.MaxSegmentSize     = ecm_desc.wMaxSegmentSize;
    p_cdc_ecm_dev->Parameters.NumberMCFilters    = ecm_desc.wNumberMCFilters;
    p_cdc_ecm_dev->Parameters.NumberPowerFilters = ecm_desc.bNumberPowerFilters;

                                                                                /* Select CDC ECM configuration.        */
   *p_err = USBH_CfgSet(p_cdc_dev->DevPtr, p_cdc_dev->Cfg_Nbr);
    if (*p_err != USBH_ERR_NONE) {
        goto end_free_return_empty;
    }

                                                                                /* See Note #1.                         */
    USBH_SET_IF(p_cdc_dev->DevPtr, p_cdc_dev->DIC_IF_Nbr, p_cdc_dev->DIC_IF_Ptr->AltIxSel, p_err);
    if (*p_err != USBH_ERR_NONE) {
        goto end_free_return_empty;
    }

    return (p_cdc_ecm_dev);

end_free_return_empty:
    Mem_PoolBlkFree(       &USBH_CDC_ECM_DevPool,
                    (void *)p_cdc_ecm_dev,
                           &err_lib);
    return ((USBH_CDC_ECM_DEV *)0);
}


/*
*********************************************************************************************************
*                                         USBH_CDC_ECM_Remove()
*
* Description : Free CDC ECM device structure.
*
* Argument(s) : p_cdc_ecm_dev   Pointer to the USB CDC ECM device structure to remove.
*
* Return(s)   : USBH_ERR_NONE,              if CDC-ECM device successfully removed.
*               USBH_ERR_FREE,              if device could not be freed.
*               USBH_ERR_INVALID_ARG,       if invalid argument passed to 'p_cdc_ecm_dev'.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_CDC_ECM_Remove (USBH_CDC_ECM_DEV  *p_cdc_ecm_dev)
{
    LIB_ERR  err;


    if (p_cdc_ecm_dev == (USBH_CDC_ECM_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    Mem_PoolBlkFree(       &USBH_CDC_ECM_DevPool,
                    (void *)p_cdc_ecm_dev,
                           &err);
    if (err != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_FREE);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                   USBH_CDC_ECM_EventRxNotifyReg()
*
* Description : Register callback function to be called when serial state is received from device.
*
* Argument(s) : p_cdc_ecm_dev               Pointer to CDC ECM device.
*
*               p_serial_state_notify       Function to be called.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_CDC_ECM_EventRxNotifyReg (USBH_CDC_ECM_DEV              *p_cdc_ecm_dev,
                                     USBH_CDC_ECM_EVENT_NOTIFY      p_ecm_event_notify,
                                     void                          *p_arg)
{
    if (p_cdc_ecm_dev == (USBH_CDC_ECM_DEV *)0) {
        return;
    }

    p_cdc_ecm_dev->EventNotifyPtr    = p_ecm_event_notify;
    p_cdc_ecm_dev->EventNotifyArgPtr = p_arg;

    USBH_CDC_EventNotifyReg(        p_cdc_ecm_dev->CDC_DevPtr,
                                    USBH_CDC_ECM_EventRxCmpl,
                            (void *)p_cdc_ecm_dev);
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
*                                      USBH_CDC_ECM_DescParse()
*
* Description : Parse ethernet control management functional descriptor into USBH_CDC_ECM_DESC structure.
*
* Argument(s) : p_ecm_desc      Variable that receives parsed descriptor.
*
*               p_if            Buffer that contains ethernet control management functional descriptor.
*
* Return(s)   : USBH_ERR_NONE,            if parsing found ethernet control management functional descriptor.
*               USBH_ERR_DESC_INVALID,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_CDC_ECM_DescParse (USBH_CDC_ECM_DESC  *p_cdc_ecm_desc,
                                          USBH_IF            *p_if)
{
    USBH_ERR     err;
    CPU_INT08U  *p_if_desc;


    err       = USBH_ERR_DESC_INVALID;
    p_if_desc = (CPU_INT08U *)p_if->IF_DataPtr;

    while (p_if_desc[1] != USBH_DESC_TYPE_EP) {

                                                  /* ECM 1.2 Section 5.4 Table 3.                                       */
        if (p_if_desc[2] == USBH_CDC_FNCTL_DESC_SUB_ENFD) {
            p_cdc_ecm_desc->bFunctionLength      = p_if_desc[0];
            p_cdc_ecm_desc->bDescriptorType      = p_if_desc[1];
            p_cdc_ecm_desc->bDescriptorSubtype   = p_if_desc[2];
            p_cdc_ecm_desc->iMACAddress          = p_if_desc[3];
            p_cdc_ecm_desc->bmEthernetStatistics = MEM_VAL_GET_INT32U_LITTLE(&p_if_desc[4]);
            p_cdc_ecm_desc->wMaxSegmentSize      = MEM_VAL_GET_INT16U_LITTLE(&p_if_desc[8]);
            p_cdc_ecm_desc->wNumberMCFilters     = MEM_VAL_GET_INT16U_LITTLE(&p_if_desc[10]);
            p_cdc_ecm_desc->bNumberPowerFilters  = p_if_desc[12];
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
*                                     USBH_CDC_ECM_EventRxCmpl()
*
* Description : Callback function invoked when status reception is completed.
*
* Argument(s) : p_context       Pointer to CDC ECM device.
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

static  void  USBH_CDC_ECM_EventRxCmpl (void        *p_context,
                                        CPU_INT08U  *p_buf,
                                        CPU_INT32U   xfer_len,
                                        USBH_ERR     err)
{
    USBH_CDC_ECM_DEV       *p_cdc_ecm_dev;
    CPU_INT08U              req;
    CPU_INT08U              value;
    USBH_CDC_ECM_STATE      ecm_state;


    p_cdc_ecm_dev = (USBH_CDC_ECM_DEV *)p_context;

    if (err == USBH_ERR_NONE) {
        if (xfer_len < 2u) {
            return;
        }

        req   = MEM_VAL_GET_INT08U(p_buf + 1);
        value = MEM_VAL_GET_INT16U(p_buf + 2);

        switch (req) {
              case USBH_CDC_NOTIFICATION_NET_CONN:            /* CDC 1.2 Section 6.3.1.                                 */
                  ecm_state.Event = USBH_CDC_ECM_EVENT_NETWORK_CONNECTION;
                  ecm_state.EventData.ConnectionStatus = value;
                  break;

              case USBH_CDC_NOTIFICATION_RESP_AVAIL:          /* CDC 1.2 Section 6.3.2.                                 */
                  ecm_state.Event = USBH_CDC_ECM_EVENT_RESPONSE_AVAILABLE;
                  break;

              case USBH_CDC_NOTIFICATION_CONN_SPEED_CHNG:     /* CDC 1.2 Section 6.3.3 Table 23.                        */
                  if (xfer_len < 16u) {
                      return;
                  }

                  ecm_state.Event = USBH_CDC_ECM_EVENT_CONNECTION_SPEED_CHANGE;
                  ecm_state.EventData.ConnectionSpeed.DownlinkSpeed = MEM_VAL_GET_INT32U(p_buf + 8);
                  ecm_state.EventData.ConnectionSpeed.UplinkSpeed   = MEM_VAL_GET_INT32U(p_buf + 12);
                  break;

            default:
                 return;

        }

        if (p_cdc_ecm_dev->EventNotifyPtr != (USBH_CDC_ECM_EVENT_NOTIFY)0) {
            p_cdc_ecm_dev->EventNotifyPtr(p_cdc_ecm_dev->EventNotifyArgPtr, ecm_state);
        }
    } else {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("CDC ECM Event Rx err: %d\r\n", err);
#endif
    }
}

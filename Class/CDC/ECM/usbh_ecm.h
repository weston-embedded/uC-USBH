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
* Filename : usbh_ecm.h
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#ifndef  USBH_CDC_ECM_MODULE_PRESENT
#define  USBH_CDC_ECM_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../usbh_cdc.h"


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   USBH_CDC_ECM_MODULE
#define  USBH_CDC_ECM_EXT
#else
#define  USBH_CDC_ECM_EXT  extern
#endif


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define USBH_CDC_ECM_MAC_LEN  13u                  /* Length of the MAC address.                                        */


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/

typedef enum usbh_cdc_ecm_event {
    USBH_CDC_ECM_EVENT_NETWORK_CONNECTION,         /* Event for USBH_CDC_NOTIFICATION_NET_CONN                          */
    USBH_CDC_ECM_EVENT_RESPONSE_AVAILABLE,         /* Event for USBH_CDC_NOTIFICATION_RESP_AVAIL                        */
    USBH_CDC_ECM_EVENT_CONNECTION_SPEED_CHANGE     /* Event for USBH_CDC_NOTIFICATION_CONN_SPEED_CHNG                   */
} USBH_CDC_ECM_EVENT;

typedef struct usbh_cdc_ecm_state {
    USBH_CDC_ECM_EVENT  Event;                     /* Event that occurred.                                              */
    union {
        CPU_BOOLEAN     ConnectionStatus;          /* Status of the network connection.                                 */
        struct {
            CPU_INT32U  DownlinkSpeed;             /* Downlink speed in bits per second.                                */
            CPU_INT32U  UplinkSpeed;               /* Uplink speed in bits per second.                                  */
        }               ConnectionSpeed;
    }                   EventData;                 /* Event data.                                                       */
} USBH_CDC_ECM_STATE;

typedef struct usbh_cdc_ecm_parameters {
    CPU_INT32U  EthernetStatistics;              /* Bitmap of supported statistics.                                     */
    CPU_INT16U  MaxSegmentSize;                   /* Maximum segment size, typically 1514.                              */
    CPU_INT16U  NumberMCFilters;                  /* Number of multicast filters supported.                             */
    CPU_INT08U  NumberPowerFilters;               /* Number of power filters supported.                                 */
} USBH_CDC_ECM_PARAMETERS;

typedef  void  (*USBH_CDC_ECM_EVENT_NOTIFY)    (void                   *p_arg,
                                                USBH_CDC_ECM_STATE      ecm_state);

typedef  struct  usbh_cdc_ecm_dev {
    USBH_CDC_DEV                  *CDC_DevPtr;
    USBH_CDC_ECM_EVENT_NOTIFY      EventNotifyPtr;
    void                          *EventNotifyArgPtr;
    CPU_INT16U                     MAC_Addr[USBH_CDC_ECM_MAC_LEN]; /* MAC address of the device, ECM120 5.4             */
    USBH_CDC_ECM_PARAMETERS        Parameters;
} USBH_CDC_ECM_DEV;


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MACRO'S
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*
* Note(s) : (1) USBH_CDC_ECM does not define DataTx, DataRx, DataTxAsync, DataRxAsync or CmdTx functions
*               these can be accessed through the CDC layer (via USBH_CDC_DEV)
*
*********************************************************************************************************
*/

USBH_ERR           USBH_CDC_ECM_GlobalInit      (void);

void               USBH_CDC_ECM_EventRxNotifyReg(USBH_CDC_ECM_DEV              *p_cdc_ecm_dev,
                                                 USBH_CDC_ECM_EVENT_NOTIFY      p_ecm_event_notify,
                                                 void                          *p_arg);

USBH_CDC_ECM_DEV  *USBH_CDC_ECM_Add             (USBH_CDC_DEV                  *p_cdc_dev,
                                                 USBH_ERR                      *p_err);

USBH_ERR           USBH_CDC_ECM_Remove          (USBH_CDC_ECM_DEV              *p_cdc_ecm_dev);


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 END
*********************************************************************************************************
*/

#endif

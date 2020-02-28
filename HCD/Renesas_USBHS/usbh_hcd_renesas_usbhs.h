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
*                                       HOST CONTROLLER DRIVER
*
*                                       RENESAS USB HIGH-SPEED
*
* Filename : usbh_hcd_renesas_usbhs.h
* Version  : V3.42.00
*********************************************************************************************************
* Note(s)  : (1) With an appropriate BSP, this host controller driver driver will support the USB
*                module on the Renesas RZ.
*********************************************************************************************************
*/

#ifndef  USBH_HCD_RENESAS_USBHS_H
#define  USBH_HCD_RENESAS_USBHS_H


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../Source/usbh_core.h"


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/

#ifdef   USBH_HCD_RENESAS_USBHS_MODULE
#define  USBH_HCD_RENESAS_USBHS_EXT
#else
#define  USBH_HCD_RENESAS_USBHS_EXT  extern
#endif


/*
*********************************************************************************************************
*                                                 DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                   PIPE INFORMATION TABLE DEFINES
*********************************************************************************************************
*/

                                                                /* -------------- PIPE TYPE BIT DEFINES --------------- */
#define  USBH_HCD_PIPE_DESC_TYPE_CTRL           DEF_BIT_00
#define  USBH_HCD_PIPE_DESC_TYPE_ISOC           DEF_BIT_01
#define  USBH_HCD_PIPE_DESC_TYPE_BULK           DEF_BIT_02
#define  USBH_HCD_PIPE_DESC_TYPE_INTR           DEF_BIT_03

                                                                /* ------------ PIPE DIRECTION BIT DEFINES ------------ */
#define  USBH_HCD_PIPE_DESC_DIR_OUT             DEF_BIT_04
#define  USBH_HCD_PIPE_DESC_DIR_IN              DEF_BIT_05

#define  USBH_HCD_PIPE_DESC_DIR_BOTH           (USBH_HCD_PIPE_DESC_DIR_OUT | USBH_HCD_PIPE_DESC_DIR_IN)


/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                   ENDPOINT INFORMATION DATA TYPE
*
* Note(s) : (1) The endpoint information data type provides information about the USB host controller
*               physical EPs (or pipes).
*
*               (a) The 'Attrib' bit-field defines the EP attributes. The EP attributes is combination
*                   of the following flags:
*
*                       USBH_HCD_PIPE_DESC_TYPE_CTRL  Indicate control     type      capable.
*                       USBH_HCD_PIPE_DESC_TYPE_ISOC  Indicate isochronous type      capable.
*                       USBH_HCD_PIPE_DESC_TYPE_BULK  Indicate bulk        type      capable.
*                       USBH_HCD_PIPE_DESC_TYPE_INTR  Indicate interrupt   type      capable.
*                       USBH_HCD_PIPE_DESC_DIR_OUT    Indicate OUT         direction capable.
*                       USBH_HCD_PIPE_DESC_DIR_IN     Indicate IN          direction capable.
*********************************************************************************************************
*/

typedef  const  struct  usbh_hcd_renesas_usbhs_pipe_desc {
    CPU_INT08U  Attrib;                                         /* Endpoint attributes (see Note #1a).                  */
    CPU_INT08U  Nbr;                                            /* Endpoint number.                                     */
    CPU_INT16U  MaxPktSize;                                     /* Endpoint maximum packet size.                        */
} USBH_HCD_RENESAS_USBHS_PIPE_DESC;


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_HCD_RENESAS_USBHS_EXT  USBH_HC_DRV_API  USBH_HCD_API_RenesasRZ_FIFO;
USBH_HCD_RENESAS_USBHS_EXT  USBH_HC_DRV_API  USBH_HCD_API_RenesasRZ_DMA;

USBH_HCD_RENESAS_USBHS_EXT  USBH_HC_DRV_API  USBH_HCD_API_RenesasRX64M_FIFO;
USBH_HCD_RENESAS_USBHS_EXT  USBH_HC_DRV_API  USBH_HCD_API_RenesasRX64M_DMA;

USBH_HCD_RENESAS_USBHS_EXT  USBH_HC_DRV_API  USBH_HCD_API_RenesasRX71M_FIFO;
USBH_HCD_RENESAS_USBHS_EXT  USBH_HC_DRV_API  USBH_HCD_API_RenesasRX71M_DMA;

USBH_HCD_RENESAS_USBHS_EXT  USBH_HC_RH_API   USBH_HCD_RH_API_RenesasUSBHS;


/*
*********************************************************************************************************
*                                                 MACROS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE END
*********************************************************************************************************
*/

#endif

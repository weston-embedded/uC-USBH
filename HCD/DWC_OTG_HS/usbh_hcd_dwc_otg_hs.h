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
*                                       HOST CONTROLLER DRIVER
*                              SYNOPSYS DESIGNWARE CORE USB 2.0 OTG (HS)
*
* File    : usbh_hcd_dwc_otg_hs.h
* Version : V3.42.01
*********************************************************************************************************
*/


#ifndef  USBH_HCD_DWC_OTG_HS_H
#define  USBH_HCD_DWC_OTG_HS_H


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

#ifdef   USBH_HCD_DWC_OTG_HS_MODULE
#define  USBH_HCD_DWC_OTG_HS_EXT
#else
#define  USBH_HCD_DWC_OTG_HS_EXT  extern
#endif


/*
*********************************************************************************************************
*                                                 DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*
* Note(s) : (1) The following MCUs are supported by USBH_STM32FX_FS_HCD_DrvAPI:
*
*
*           (2) The following MCUs are supported by USBH_STM32FX_OTG_HS_HCD_DrvAPI:
*
*                          STMicroelectronics  STM32F20xx series.
*                          STMicroelectronics  STM32F21xx series.
*                          STMicroelectronics  STM32F4xxx series.
*                          STMicroelectronics  STM32F74xx series.
*                          STMicroelectronics  STM32F75xx series.
*
*********************************************************************************************************
*/

USBH_HCD_DWC_OTG_HS_EXT  USBH_HC_DRV_API  USBH_STM32FX_OTG_HS_HCD_DrvAPI;

USBH_HCD_DWC_OTG_HS_EXT  USBH_HC_RH_API   USBH_DWCOTGHS_HCD_RH_API;


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

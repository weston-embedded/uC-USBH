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
*                                  STM32FX_FS HOST CONTROLLER DRIVER
*
* Filename : usbh_hcd_stm32fx_fs.h
* Version  : V3.42.00
*********************************************************************************************************
*/

#ifndef  USBH_HCD_STM32FX_FS_H
#define  USBH_HCD_STM32FX_FS_H


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  "Source/usbh_core.h"


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/

#ifdef   USBH_HCD_STM32FX_FS_MODULE
#define  USBH_HCD_STM32FX_FS_EXT
#else
#define  USBH_HCD_STM32FX_FS_EXT  extern
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
*                          STMicroelectronics  STM32F105xx series.
*                          STMicroelectronics  STM32F107xx series.
*                          STMicroelectronics  STM32F205xx series.
*                          STMicroelectronics  STM32F207xx series.
*                          STMicroelectronics  STM32F215xx series.
*                          STMicroelectronics  STM32F217xx series.
*                          STMicroelectronics  STM32F405xx series.
*                          STMicroelectronics  STM32F407xx series.
*                          STMicroelectronics  STM32F415xx series.
*                          STMicroelectronics  STM32F417xx series.
*
*           (2) The following MCUs are supported by USBH_STM32FX_OTG_FS_HCD_DrvAPI:
*
*                          STMicroelectronics  STM32F74xx series.
*                          STMicroelectronics  STM32F75xx series.
*
*           (3) The following MCUs are support by USBD_DrvAPI_EFM32_OTG_FS API:
*
*                          Silicon Labs        EFM32 Giant   Gecko series.
*                          Silicon Labs        EFM32 Wonder  Gecko series.
*                          Silicon Labs        EFM32 Leopard Gecko series.
*
*********************************************************************************************************
*/

USBH_HCD_STM32FX_FS_EXT  USBH_HC_DRV_API  USBH_STM32FX_FS_HCD_DrvAPI;
USBH_HCD_STM32FX_FS_EXT  USBH_HC_DRV_API  USBH_STM32FX_OTG_FS_HCD_DrvAPI;
USBH_HCD_STM32FX_FS_EXT  USBH_HC_DRV_API  USBH_EFM32_OTG_FS_HCD_DrvAPI;

USBH_HCD_STM32FX_FS_EXT  USBH_HC_RH_API   USBH_STM32FX_FS_HCD_RH_API;


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

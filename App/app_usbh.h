/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                   USB APPLICATION INITIALIZATION
*
*
* Filename : app_usbh.h
* Version  : V3.42.01
*********************************************************************************************************
*/

#ifndef  APP_USBH_MODULE_PRESENT
#define  APP_USBH_MODULE_PRESENT


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  <app_cfg.h>


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/

#ifdef   APP_USBH_MODULE
#define  APP_USBH_MODULE_EXT
#else
#define  APP_USBH_MODULE_EXT  extern
#endif


/*
*********************************************************************************************************
*                                                 DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        DEFAULT USB HOST CONFIGURATION
*********************************************************************************************************
*/

#ifndef  APP_CFG_USBH_MSC_EN
#define  APP_CFG_USBH_MSC_EN                    DEF_DISABLED
#endif

#ifndef  APP_CFG_USBH_HID_EN
#define  APP_CFG_USBH_HID_EN                    DEF_DISABLED
#endif

#ifndef  APP_CFG_USBH_CDC_ACM_EN
#define  APP_CFG_USBH_CDC_ACM_EN                DEF_DISABLED

#ifndef  APP_CFG_USBH_CDC_ECM_EN
#define  APP_CFG_USBH_CDC_ECM_EN                DEF_DISABLED
#endif

#ifndef  APP_CFG_USBH_FTDI_EN
#define  APP_CFG_USBH_FTDI_EN                   DEF_DISABLED
#endif


/*
*********************************************************************************************************
*                                      CONDITIONAL INCLUDE FILES
*********************************************************************************************************
*/

#if (APP_CFG_USBH_EN == DEF_ENABLED)

#if (APP_CFG_USBH_HID_EN == DEF_ENABLED)
#include    "HID/app_usbh_hid.h"
#endif

#if (APP_CFG_USBH_CDC_ACM_EN == DEF_ENABLED)
#include    "CDC/app_usbh_cdc_acm.h"
#endif

#if (APP_CFG_USBH_CDC_ECM_EN == DEF_ENABLED)
#include    "CDC/app_usbh_cdc_ecm.h"
#endif

#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
#include    "MSC/uC-FS-V4/app_usbh_msc.h"
#endif

#if (APP_CFG_USBH_FTDI_EN == DEF_ENABLED)
#include    <FTDI/app_usbh_ftdi.h>
#endif

#endif


/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/

typedef  CPU_STK  USBH_STK;                                     /* Task's stack data type.                              */


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

#if (APP_CFG_USBH_EN == DEF_ENABLED)
CPU_BOOLEAN  App_USBH_Init (void);
#endif


/*
*********************************************************************************************************
*                                          CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef  APP_CFG_USBH_EN
#error  "APP_CFG_USBH_EN                     not #defined in 'app_cfg.h'         "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBH_EN != DEF_ENABLED) && \
        (APP_CFG_USBH_EN != DEF_DISABLED))
#error  "APP_CFG_USBH_EN                     illegally #defined in 'app_cfg.h'   "
#error  "                              [MUST be DEF_ENABLED ]                    "
#error  "                              [     || DEF_DISABLED]                    "

#elif  ((APP_CFG_USBH_EN == DEF_ENABLED   ) && \
        (!defined(USBH_OS_CFG_ASYNC_TASK_PRIO)))
#error  "USBH_OS_CFG_ASYNC_TASK_PRIO         not #defined in 'app_cfg.h'    "
#error  "                              [MUST be > 0u ]                           "

#elif  ((APP_CFG_USBH_EN == DEF_ENABLED   ) && \
        (!defined(USBH_OS_CFG_ASYNC_TASK_STK_SIZE)))
#error  "USBH_OS_CFG_ASYNC_TASK_STK_SIZE         not #defined in 'app_cfg.h'    "
#error  "                              [MUST be > 0u ]                           "

#elif  ((APP_CFG_USBH_EN == DEF_ENABLED   ) && \
        (!defined(USBH_OS_CFG_HUB_TASK_PRIO)))
#error  "USBH_OS_CFG_HUB_TASK_PRIO         not #defined in 'app_cfg.h'          "
#error  "                              [MUST be > 0u ]                           "

#elif  ((APP_CFG_USBH_EN == DEF_ENABLED   ) && \
        (!defined(USBH_OS_CFG_HUB_TASK_STK_SIZE)))
#error  "USBH_OS_CFG_HUB_TASK_STK_SIZE     not #defined in 'app_cfg.h'    "
#error  "                              [MUST be > 0u ]                           "

#elif  ((APP_CFG_USBH_EN          == DEF_ENABLED ) && \
        (APP_CFG_USBH_MSC_EN      == DEF_ENABLED ) && \
        (APP_CFG_FS_EN            == DEF_DISABLED))
#error  "Host MSC demo chosen: APP_CFG_FS_EN MUST be DEF_ENABLED in app_cfg.h.   "
#endif


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif

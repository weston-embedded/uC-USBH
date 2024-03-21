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
*                                 USB HOST APPLICATION INITIALIZATION
*
*
* Filename : app_usbh.c
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define   APP_USBH_MODULE

#include  "app_usbh.h"
#include  <usbh_hc_cfg.h>
#include  <bsp_usbh_template.h>

#if (APP_CFG_USBH_EN == DEF_ENABLED)

/*
*********************************************************************************************************
*                                              LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                 USB HOST STACK INTERNAL TASK'S STACKS
*********************************************************************************************************
*/

static  USBH_STK  App_USBH_AsyncTaskStk[USBH_OS_CFG_ASYNC_TASK_STK_SIZE];
static  USBH_STK  App_USBH_HubTaskStk[USBH_OS_CFG_HUB_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

USBH_KERNEL_TASK_INFO  AsyncTaskInfo = {                        /* ---------------- INFO ON ASYNC TASK ---------------- */
    USBH_OS_CFG_ASYNC_TASK_PRIO,                                /* Async task priority.                                 */
    App_USBH_AsyncTaskStk,                                      /* Ptr to async task stack.                             */
    USBH_OS_CFG_ASYNC_TASK_STK_SIZE                             /* Size of async task stack.                            */
};

USBH_KERNEL_TASK_INFO  HubTaskInfo = {                          /* ----------------- INFO ON HUB TASK ----------------- */
    USBH_OS_CFG_HUB_TASK_PRIO,                                  /* Hub task priority.                                   */
    App_USBH_HubTaskStk,                                        /* Ptr to hub task stack.                               */
    USBH_OS_CFG_HUB_TASK_STK_SIZE                               /* Size of hub task stack.                              */
};


/*
*********************************************************************************************************
*                                        LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            App_USBH_Init()
*
* Description : Initialize USB Host Stack and additional demos.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK      if successfull.
*               DEF_FAIL    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  App_USBH_Init (void)
{
    USBH_ERR    err;
    CPU_INT08U  hc_nbr;


    APP_TRACE_INFO(("\r\n"));
    APP_TRACE_INFO(("=============================\r\n"));
    APP_TRACE_INFO(("=  USB HOST INITIALIZATION  =\r\n"));
    APP_TRACE_INFO(("=============================\r\n"));

    err = USBH_Init(&AsyncTaskInfo,
                    &HubTaskInfo);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not initialize USB HOST Stack  w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }

#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
    APP_TRACE_INFO(("... Initiliazing HOST Mass Storage class ...\r\n"));
    err = App_USBH_MSC_Init();

    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not initialize HOST Mass Storage Class w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }
#endif

#if (APP_CFG_USBH_HID_EN == DEF_ENABLED)
    APP_TRACE_INFO(("... Initiliazing HOST Human Interface Device class ...\r\n"));
    err = App_USBH_HID_Init();

    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not initialize HID class w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }
#endif

#if (APP_CFG_USBH_CDC_ACM_EN == DEF_ENABLED)
    APP_TRACE_INFO(("... Initiliazing HOST Communication Device Class ...\r\n"));
    err = App_USBH_CDC_ACM_Init();

    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not initialize CDC ACM w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }
#endif

#if (APP_CFG_USBH_CDC_ECM_EN == DEF_ENABLED)
    APP_TRACE_INFO(("... Initiliazing HOST Communication Device Class ...\r\n"));
    err = App_USBH_CDC_ECM_Init();

    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not initialize CDC ECM w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }
#endif

#if (APP_CFG_USBH_FTDI_EN == DEF_ENABLED)
    APP_TRACE_INFO(("... Initiliazing HOST FUTURE TECHNOLOGY DEVICES INTL. Class ...\r\n"));
    App_USBH_FTDI_Init(&err);

    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not initialize FTDI w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }
#endif

    hc_nbr = USBH_HC_Add(&USBH_HC_TemplateCfg,
                         &USBH_TemplateHCD_DrvAPI,
                         &USBH_TemplateHCD_RH_API,
                         &USBH_DrvBSP_Template,
                         &err);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not add host controller w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }

    err = USBH_HC_Start(hc_nbr);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_DBG(("...could not start host controller w/err = %d\r\n\r\n", err));
        return (DEF_FAIL);
    }

    return (DEF_OK);
}
#endif

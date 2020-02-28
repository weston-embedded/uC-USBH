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
*                                   TEMPLATE HOST CONTROLLER DRIVER
*
* Filename : usbh_hcd_template.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define  USBH_HCD_TEMPLATE_MODULE
#include  "usbh_hcd_template.h"
#include  "../../Source/usbh_hub.h"


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
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* --------------- DRIVER API FUNCTIONS --------------- */
static  void           USBH_TemplateHCD_Init            (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_Start           (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_Stop            (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  USBH_DEV_SPD   USBH_TemplateHCD_SpdGet          (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_Suspend         (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_Resume          (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  CPU_INT32U     USBH_TemplateHCD_FrameNbrGet     (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_EP_Open         (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_EP_Close        (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_EP_Abort        (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  CPU_BOOLEAN    USBH_TemplateHCD_IsHalt_EP       (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_URB_Submit      (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_URB              *p_urb,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_URB_Complete    (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_URB              *p_urb,
                                                         USBH_ERR              *p_err);

static  void           USBH_TemplateHCD_URB_Abort       (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_URB              *p_urb,
                                                         USBH_ERR              *p_err);

                                                                /* -------------- ROOT HUB API FUNCTIONS -------------- */
static  CPU_BOOLEAN    USBH_TemplateHCD_PortStatusGet   (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr,
                                                         USBH_HUB_PORT_STATUS  *p_port_status);

static  CPU_BOOLEAN    USBH_TemplateHCD_HubDescGet      (USBH_HC_DRV           *p_hc_drv,
                                                         void                  *p_buf,
                                                         CPU_INT08U             buf_len);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortEnSet       (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortEnClr       (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortEnChngClr   (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortPwrSet      (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortPwrClr      (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortResetSet    (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortResetChngClr(USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortSuspendClr  (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_PortConnChngClr (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    USBH_TemplateHCD_RHSC_IntEn      (USBH_HC_DRV           *p_hc_drv);

static  CPU_BOOLEAN    USBH_TemplateHCD_RHSC_IntDis     (USBH_HC_DRV           *p_hc_drv);


/*
*********************************************************************************************************
*                                 INITIALIZED GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_HC_DRV_API  USBH_TemplateHCD_DrvAPI = {
    USBH_TemplateHCD_Init,
    USBH_TemplateHCD_Start,
    USBH_TemplateHCD_Stop,
    USBH_TemplateHCD_SpdGet,
    USBH_TemplateHCD_Suspend,
    USBH_TemplateHCD_Resume,
    USBH_TemplateHCD_FrameNbrGet,

    USBH_TemplateHCD_EP_Open,
    USBH_TemplateHCD_EP_Close,
    USBH_TemplateHCD_EP_Abort,
    USBH_TemplateHCD_IsHalt_EP,

    USBH_TemplateHCD_URB_Submit,
    USBH_TemplateHCD_URB_Complete,
    USBH_TemplateHCD_URB_Abort,
};

USBH_HC_RH_API  USBH_TemplateHCD_RH_API = {
    USBH_TemplateHCD_PortStatusGet,
    USBH_TemplateHCD_HubDescGet,

    USBH_TemplateHCD_PortEnSet,
    USBH_TemplateHCD_PortEnClr,
    USBH_TemplateHCD_PortEnChngClr,

    USBH_TemplateHCD_PortPwrSet,
    USBH_TemplateHCD_PortPwrClr,

    USBH_TemplateHCD_PortResetSet,
    USBH_TemplateHCD_PortResetChngClr,

    USBH_TemplateHCD_PortSuspendClr,
    USBH_TemplateHCD_PortConnChngClr,

    USBH_TemplateHCD_RHSC_IntEn,
    USBH_TemplateHCD_RHSC_IntDis
};


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       USBH_TemplateHCD_Init()
*
* Description : Initialize host controller and allocate driver's internal memory/variables.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD init successful.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_Init (USBH_HC_DRV  *p_hc_drv,
                                     USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_TemplateHCD_Start()
*
* Description : Start Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD start successful.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_Start (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_TemplateHCD_Stop()
*
* Description : Stop Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD stop successful.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_Stop (USBH_HC_DRV  *p_hc_drv,
                                     USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_TemplateHCD_SpdGet()
*
* Description : Returns Host Controller speed.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           Host controller speed retrieved successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : Host controller speed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_DEV_SPD  USBH_TemplateHCD_SpdGet (USBH_HC_DRV  *p_hc_drv,
                                               USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;

    return (USBH_DEV_SPD_NONE);
}


/*
*********************************************************************************************************
*                                     USBH_TemplateHCD_Suspend()
*
* Description : Suspend Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD suspend successful.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_Suspend (USBH_HC_DRV  *p_hc_drv,
                                        USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_TemplateHCD_Resume()
*
* Description : Resume Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD resume successful.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_Resume (USBH_HC_DRV  *p_hc_drv,
                                       USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBH_TemplateHCD_FrameNbrGet()
*
* Description : Retrieve current frame number.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HC frame number retrieved successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : Frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_TemplateHCD_FrameNbrGet (USBH_HC_DRV  *p_hc_drv,
                                                  USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;

    return (0u);
}


/*
*********************************************************************************************************
*                                     USBH_TemplateHCD_EP_Open()
*
* Description : Allocate/open endpoint of given type.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           Endpoint opened successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_EP_Open (USBH_HC_DRV  *p_hc_drv,
                                        USBH_EP      *p_ep,
                                        USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_ep;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_TemplateHCD_EP_Close()
*
* Description : Close specified endpoint.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           Endpoint closed successfully.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_EP_Close (USBH_HC_DRV  *p_hc_drv,
                                         USBH_EP      *p_ep,
                                         USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_ep;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_TemplateHCD_EP_Abort()
*
* Description : Abort all pending URBs on endpoint.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           Endpoint aborted successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_EP_Abort (USBH_HC_DRV  *p_hc_drv,
                                         USBH_EP      *p_ep,
                                         USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_ep;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_IsHalt_EP()
*
* Description : Retrieve endpoint halt state.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           Endpoint halt status retrieved successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : DEF_TRUE,       If endpoint halted.
*
*               DEF_FALSE,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_IsHalt_EP (USBH_HC_DRV  *p_hc_drv,
                                                 USBH_EP      *p_ep,
                                                 USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_ep;

   *p_err = USBH_ERR_NONE;

    return (DEF_FALSE);
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_URB_Submit()
*
* Description : Submit specified URB.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_urb       Pointer to URB structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           URB submitted successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_URB_Submit (USBH_HC_DRV  *p_hc_drv,
                                           USBH_URB     *p_urb,
                                           USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_urb;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBH_TemplateHCD_URB_Complete()
*
* Description : Complete specified URB.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_urb       Pointer to URB structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           URB completed successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_URB_Complete (USBH_HC_DRV  *p_hc_drv,
                                             USBH_URB     *p_urb,
                                             USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_urb;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_URB_Abort()
*
* Description : Abort specified URB.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_urb       Pointer to URB structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           URB aborted successfuly.
*                   Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_TemplateHCD_URB_Abort (USBH_HC_DRV  *p_hc_drv,
                                          USBH_URB     *p_urb,
                                          USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_urb;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         ROOT HUB FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                  USBH_TemplateHCD_PortStatusGet()
*
* Description : Retrieve port status changes and port status.
*
* Argument(s) : p_hc_drv            Pointer to host controller driver structure.
*
*               port_nbr            Port Number.
*
*               p_port_status       Pointer to structure that will receive port status.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortStatusGet (USBH_HC_DRV           *p_hc_drv,
                                                     CPU_INT08U             port_nbr,
                                                     USBH_HUB_PORT_STATUS  *p_port_status)
{
    (void)p_hc_drv;
    (void)port_nbr;
    (void)p_port_status;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_HubDescGet()
*
* Description : Retrieve root hub descriptor.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_buf       Pointer to buffer that will receive hub descriptor.
*
*               buf_len     Buffer length in octets.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_HubDescGet (USBH_HC_DRV  *p_hc_drv,
                                                  void         *p_buf,
                                                  CPU_INT08U    buf_len)
{
    (void)p_hc_drv;
    (void)p_buf;
    (void)buf_len;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_PortEnSet()
*
* Description : Enable given port.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortEnSet (USBH_HC_DRV  *p_hc_drv,
                                                 CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_PortEnClr()
*
* Description : Clear port enable status.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortEnClr (USBH_HC_DRV  *p_hc_drv,
                                                 CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  USBH_TemplateHCD_PortEnChngClr()
*
* Description : Clear port enable status change.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortEnChngClr (USBH_HC_DRV  *p_hc_drv,
                                                     CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_PortPwrSet()
*
* Description : Set port power based on port power mode.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortPwrSet (USBH_HC_DRV  *p_hc_drv,
                                                  CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_PortPwrClr()
*
* Description : Clear port power.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortPwrClr (USBH_HC_DRV  *p_hc_drv,
                                                  CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_TemplateHCD_PortResetSet()
*
* Description : Reset given port.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortResetSet (USBH_HC_DRV  *p_hc_drv,
                                                    CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                 USBH_TemplateHCD_PortResetChngClr()
*
* Description : Clear port reset status change.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortResetChngClr (USBH_HC_DRV  *p_hc_drv,
                                                        CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  USBH_TemplateHCD_PortSuspendClr()
*
* Description : Resume given port if port is suspended.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortSuspendClr (USBH_HC_DRV  *p_hc_drv,
                                                      CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                 USBH_TemplateHCD_PortConnChngClr()
*
* Description : Clear port connect status change.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port Number.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_PortConnChngClr (USBH_HC_DRV  *p_hc_drv,
                                                       CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_TemplateHCD_RHSC_IntEn()
*
* Description : Enable root hub interrupt.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_RHSC_IntEn (USBH_HC_DRV  *p_hc_drv)
{
    (void)p_hc_drv;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_TemplateHCD_RHSC_IntDis()
*
* Description : Disable root hub interrupt.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,         If successful.
*               DEF_FAIL,       otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_TemplateHCD_RHSC_IntDis (USBH_HC_DRV  *p_hc_drv)
{
    (void)p_hc_drv;

    return (DEF_OK);
}


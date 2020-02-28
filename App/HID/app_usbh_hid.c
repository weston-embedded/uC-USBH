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
*                                    USB HOST HID TEST APPLICATION
*
*                                              TEMPLATE
*
* Filename : app_usbh_hid_demo.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   APP_USBH_HID_MODULE
#include  <Class/HID/usbh_hid.h>
#include  "app_usbh_hid.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_HID_APP_USAGE_MOUSE                 0x00000002u
#define  USBH_HID_APP_USAGE_KBD                   0x00000006u


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
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_HID_ClassDevNotify(void          *p_hid_dev,
                                          CPU_INT08U     state,
                                          void          *p_ctx);

static  void  App_USBH_HID_DevConn       (USBH_HID_DEV  *p_hid_dev);

static  void  App_USBH_HID_DevDisconn    (USBH_HID_DEV  *p_hid_dev);


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
*                                         App_USBH_HID_Init()
*
* Description : Register HID notify callback.
*
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE,       if the demo is initialized.
*               Specific error code, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  App_USBH_HID_Init (void)
{
    USBH_ERR  host_err;

                                                                /* ----------- REGISTER DRV & NOTIFICATION ------------ */
    host_err = USBH_ClassDrvReg(       &USBH_HID_ClassDrv,
                                        App_USBH_HID_ClassDevNotify,
                                (void *)0);
    return (host_err);
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
*                                    App_USBH_HID_ClassDevNotify()
*
* Description : Handle device state change notification for HID devices.
*
* Argument(s) : p_class_dev     Pointer to class device.
*
*               state           State.
*
*               pctx            Pointer to context.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_HID_ClassDevNotify (void        *p_class_dev,
                                           CPU_INT08U   state,
                                           void        *p_ctx)
{
    USBH_HID_DEV  *p_hid_dev;


    (void)p_ctx;
    p_hid_dev = (USBH_HID_DEV *)p_class_dev;

    switch (state) {
        case USBH_CLASS_DEV_STATE_CONN:                         /* ---------------- HID DEVICE CONN'D ----------------- */
             APP_TRACE_INFO(("HID Demo: Device Connected\r\n"));
             App_USBH_HID_DevConn(p_hid_dev);
             break;

        case USBH_CLASS_DEV_STATE_DISCONN:                      /* ---------------- HID DEVICE REMOVED ---------------- */
             APP_TRACE_INFO(("HID Demo: Device Removed\r\n"));
             App_USBH_HID_DevDisconn(p_hid_dev);
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                       App_USBH_HID_DevConn()
*
* Description : This function is called when HID device is connected.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_HID_DevConn (USBH_HID_DEV  *p_hid_dev)
{
    CPU_INT08U           nbr_report_id;
    CPU_INT08U           report_id_cnt;
    USBH_HID_REPORT_ID  *p_report_id_array;
    USBH_HID_REPORT_ID  *p_report_id;
    USBH_ERR             err;


    err = USBH_HID_RefAdd(p_hid_dev);                           /* Add ref                                              */
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in App_USBH_HID_DevConn(): %d\r\n", err));
        return;
    }

    err = USBH_HID_IdleSet(p_hid_dev, 0u, 0u);                  /* Set Idle Time.                                       */
    if (err == USBH_ERR_EP_STALL) {
        APP_TRACE_INFO(("Error in App_USBH_HID_DevConn(): %d\r\n", err));
    } else if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in App_USBH_HID_DevConn(): %d\r\n", err));
        return;
    }

    err = USBH_HID_Init(p_hid_dev);                             /* Initialize HID                                       */
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in App_USBH_HID_DevConn(): %d\r\n", err));
        return;
    }

    err = USBH_HID_GetReportIDArray(p_hid_dev,
                                   &p_report_id_array,
                                   &nbr_report_id);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in App_USBH_HID_DevConn(): %d\r\n", err));
        return;
    }

    for (report_id_cnt = 0u; report_id_cnt < nbr_report_id; report_id_cnt++) {
        p_report_id = &p_report_id_array[report_id_cnt];

        if (p_report_id->Type != 0x08u) {                       /* Validate report is of INPUT type.                    */
            continue;
        }

        if (p_hid_dev->Usage == USBH_HID_APP_USAGE_KBD) {

            APP_TRACE_INFO(("HID Demo: Keyboard Connected\r\n"));

            err = USBH_HID_RegRxCB(                    p_hid_dev,
                                                       p_report_id->ReportID,
                                   (USBH_HID_RXCB_FNCT)App_USBH_KBD_CallBack,
                                   (void             *)p_hid_dev);

         } else if (p_hid_dev->Usage == USBH_HID_APP_USAGE_MOUSE) {

             APP_TRACE_INFO(("HID Demo: Mouse Connected\r\n"));

             err = USBH_HID_RegRxCB(                    p_hid_dev,
                                                        p_report_id->ReportID,
                                    (USBH_HID_RXCB_FNCT)App_USBH_MouseCallBack,
                                    (void             *)p_hid_dev);
         }
    }

    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in App_USBH_HID_DevConn(): %d\r\n", err));
    }
}

/*
*********************************************************************************************************
*                                      App_USBH_HID_DevDisconn()
*
* Description : This function is called when HID device is disconnected.
*
* Argument(s) : phid_dev          Pointer to HID device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_HID_DevDisconn (USBH_HID_DEV  *p_hid_dev)
{
    USBH_HID_RefRel(p_hid_dev);                                 /* Rel ref.                                             */
}

/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

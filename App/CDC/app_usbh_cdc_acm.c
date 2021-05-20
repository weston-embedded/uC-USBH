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
*                                  USB HOST CDC ACM TEST APPLICATION
*
*                                              TEMPLATE
*
* Filename : app_usbh_cdc_acm.c
* Version  : V3.42.01
*********************************************************************************************************
*/

#define  APP_USBH_CDC_ACM_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <app_usbh.h>
#include  <lib_str.h>
#include  "app_usbh_cdc_acm.h"


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

static  CPU_INT08U  App_USBH_CDC_ACM_AT_Cmds[][10u] = {
   {'A', 'T', 'Q', '0', 'V', '1', 'E', '0', '\r', '\0'},
   {'A', 'T', 'I', '0', '\r', '\0'},
   {'A', 'T', 'I', '1', '\r', '\0'},
   {'A', 'T', 'I', '2', '\r', '\0'},
   {'A', 'T', 'I', '3', '\r', '\0'},
   {'A', 'T', 'I', '7', '\r', '\0'}
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

static  USBH_STK           App_USBH_CDC_ACM_AT_CmdsTaskStk[APP_CFG_USBH_CDC_MAIN_TASK_STK_SIZE];
static  USBH_HSEM          App_USBH_CDC_ACM_IsConnSem;

static  USBH_CDC_DEV      *App_USBH_CDC_Ptr;
static  USBH_CDC_ACM_DEV  *App_USBH_CDC_ACM_Ptr;

static  CPU_INT08U         App_USBH_CDC_ACM_RxBuf[256u];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_CDC_ClassNotify       (void                   *p_class_dev,
                                              CPU_INT08U              state,
                                              void                   *p_ctx);

static  void  App_USBH_CDC_ACM_DevConn       (USBH_CDC_DEV           *p_cdc_dev);

static  void  App_USBH_CDC_ACM_DevDisconn    (USBH_CDC_DEV           *p_cdc_dev);

static  void  App_USBH_CDC_ACM_ATTask        (void                   *p_ctx);

static  void  App_USBH_CDC_ACM_SerEventNotify(USBH_CDC_SERIAL_STATE   serial_state);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                   INITIALIZED GLOBAL VARIABLES ACCESSES BY OTHER MODULES/OBJECTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTION
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                       App_USBH_CDC_ACM_Init()
*
* Description : Register CDC notify callback.
*
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE,       if application initialized successfully.
*
*               Specific error code, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  App_USBH_CDC_ACM_Init (void)
{
    USBH_HTASK  htask;
    USBH_ERR    err;


    err = USBH_CDC_ACM_GlobalInit();
    if (err != USBH_ERR_NONE) {
        return (err);
    }

                                                                /* ----------- REGISTER DRV & NOTIFICATION ------------ */
    err = USBH_ClassDrvReg(       &USBH_CDC_ClassDrv,
                                   App_USBH_CDC_ClassNotify,
                           (void *)0);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_OS_SemCreate(&App_USBH_CDC_ACM_IsConnSem, 0u);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

                                                                /* Create task for AT Cmds handling.                    */
    err = USBH_OS_TaskCreate(              "Demo AT cmds Task",
                                            APP_CFG_USBH_CDC_MAIN_TASK_PRIO,
                                            App_USBH_CDC_ACM_ATTask,
                             (void       *) 0,
                             (CPU_INT32U *)&App_USBH_CDC_ACM_AT_CmdsTaskStk[0],
                                            APP_CFG_USBH_CDC_MAIN_TASK_STK_SIZE,
                                           &htask);

    return (err);
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
*                                     App_USBH_CDC_ClassNotify()
*
* Description : Handle device state change notification for CDC devices.
*
* Argument(s) : p_class_dev     Pointer to class device.
*
*               state           State.
*
*               p_ctx           Pointer to context.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_CDC_ClassNotify (void        *p_class_dev,
                                        CPU_INT08U   state,
                                        void        *p_ctx)
{
    USBH_CDC_DEV  *p_cdc_dev;


    (void)p_ctx;
    p_cdc_dev = (USBH_CDC_DEV *)p_class_dev;

    switch (state) {
        case USBH_CLASS_DEV_STATE_CONN:                         /* ---------------- CDC DEVICE CONN'D ----------------- */
             APP_TRACE_INFO(("CDC ACM Demo: Device Connected\r\n"));

             App_USBH_CDC_ACM_DevConn(p_cdc_dev);
             USBH_OS_SemPost(App_USBH_CDC_ACM_IsConnSem);       /* Resume CDC task                                      */
             break;

        case USBH_CLASS_DEV_STATE_DISCONN:                      /* ---------------- CDC DEVICE REMOVED ---------------- */
             APP_TRACE_INFO(("CDC ACM Demo: Device Removed\r\n"));

             App_USBH_CDC_ACM_DevDisconn(p_cdc_dev);
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                     App_USBH_CDC_ACM_DevConn()
*
* Description : This function is called when CDC device is connected.
*
* Argument(s) : pcdc_dev          Pointer to CDC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_CDC_ACM_DevConn (USBH_CDC_DEV  *p_cdc_dev)
{
    USBH_ERR    err;
    CPU_INT08U  sub_class;
    CPU_INT08U  protocol;
    CPU_INT08U  current_stop_bits;
    CPU_INT08U  current_parity_val;
    CPU_INT08U  current_data_bits;
    CPU_INT32U  current_baud_rate;


    App_USBH_CDC_Ptr = p_cdc_dev;

    err = USBH_CDC_RefAdd(p_cdc_dev);                           /* Add ref.                                             */
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_CDC_RefAdd(): %d\r\n", err));
        return;
    }

    err = USBH_CDC_SubclassGet(p_cdc_dev, &sub_class);          /* Get CDC subclass of dev.                             */
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_CDC_SubclassGet(): %d\r\n", err));
        (void)USBH_CDC_RefRel(p_cdc_dev);
        return;
    }

    err = USBH_CDC_ProtocolGet(p_cdc_dev, &protocol);           /* Get protocol supported by dev.                       */
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_CDC_ProtocolGet(): %d\r\n", err));
        (void)USBH_CDC_RefRel(p_cdc_dev);
        return;
    }

    if (sub_class != USBH_CDC_CONTROL_SUBCLASS_CODE_ACM) {
        APP_TRACE_INFO(("Subclass not supported\r\n"));
        (void)USBH_CDC_RefRel(p_cdc_dev);
        return;
    }

    if ((protocol != USBH_CDC_CONTROL_PROTOCOL_CODE_USB    ) &&
        (protocol != USBH_CDC_CONTROL_PROTOCOL_CODE_V_25_AT) &&
        (protocol != USBH_CDC_CONTROL_PROTOCOL_CODE_VENDOR )) {
        APP_TRACE_INFO(("Protocol not supported\r\n"));
        (void)USBH_CDC_RefRel(p_cdc_dev);
        return;
    }

    App_USBH_CDC_ACM_Ptr = USBH_CDC_ACM_Add(App_USBH_CDC_Ptr,
                                           &err);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_CDC_ACM_Add(): %d\r\n", err));
        (void)USBH_CDC_RefRel(p_cdc_dev);
        return;
    }

    USBH_CDC_ACM_EventRxNotifyReg(App_USBH_CDC_ACM_Ptr,
                                  App_USBH_CDC_ACM_SerEventNotify);

    err = USBH_CDC_ACM_LineCodingSet(App_USBH_CDC_ACM_Ptr,      /* Set new ser cfg for USB Modem.                       */
                                     USBH_CDC_ACM_LINE_CODING_BAUDRATE_19200,
                                     USBH_CDC_ACM_LINE_CODING_STOP_BIT_2,
                                     USBH_CDC_ACM_LINE_CODING_PARITY_NONE,
                                     USBH_CDC_ACM_LINE_CODING_DATA_BITS_8);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_CDC_ACM_LineCodingSet(): %d\r\n", err));
        App_USBH_CDC_ACM_DevDisconn(p_cdc_dev);
        return;
    }

    USBH_CDC_ACM_LineCodingGet(App_USBH_CDC_ACM_Ptr,            /* Check if new serial cfg has been set.                */
                              &current_baud_rate,
                              &current_stop_bits,
                              &current_parity_val,
                              &current_data_bits);

    if ((current_baud_rate  != USBH_CDC_ACM_LINE_CODING_BAUDRATE_19200) ||
        (current_stop_bits  != USBH_CDC_ACM_LINE_CODING_STOP_BIT_2    ) ||
        (current_parity_val != USBH_CDC_ACM_LINE_CODING_PARITY_NONE   ) ||
        (current_data_bits  != USBH_CDC_ACM_LINE_CODING_DATA_BITS_8   )) {
        APP_TRACE_INFO(("Demo :Line parameters didn't match\r\n"));
    }

    err = USBH_CDC_ACM_LineStateSet(App_USBH_CDC_ACM_Ptr,       /* Activate Carrier and indicates DCE that DTE is present.*/
                                    USBH_CDC_ACM_DTR_SET,
                                    USBH_CDC_ACM_RTS_SET);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_CDC_ACM_LineStateSet(): %d\r\n", err));
        App_USBH_CDC_ACM_DevDisconn(p_cdc_dev);
        return;
    }
}


/*
*********************************************************************************************************
*                                    App_USBH_CDC_ACM_DevDisconn()
*
* Description : This function is called when CDC device is disconnected.
*
* Argument(s) : pcdc_dev          Pointer to CDC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_CDC_ACM_DevDisconn (USBH_CDC_DEV  *p_cdc_dev)
{
    (void)USBH_CDC_ACM_Remove(App_USBH_CDC_ACM_Ptr);
    (void)USBH_CDC_RefRel(p_cdc_dev);
}


/*
*********************************************************************************************************
*                                      App_USBH_CDC_ACM_ATTask()
*
* Description : Sends a set of AT commands to the USB modem. The USB modem will answer to these commands
*               with a success/failed status or a data. The status/data of the AT command is displayed by
*               the function App_USBH_CDC_ACM_SerRxDataAvail().
*               AT commands used in the demo:
*               - ATI0: Four-digit product code.
*               - ATI1: Results of ROM checksum.
*               - ATI2: Results of RAM checksum.
*               - ATI3: Product type.
*               - ATI7: Product configuration.
*
* Argument(s) : pctx            Pointer to the context.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_CDC_ACM_ATTask (void  *p_ctx)
{
    USBH_ERR    err;
    CPU_INT08U  i;
    CPU_INT32U  AT_cmd_len;
    CPU_INT32U  xfer_len;
    CPU_INT32U  ix;


    (void)p_ctx;

    while (1) {
                                                                /* ------------- WAIT FOR DEV CONNECTION. ------------- */
        USBH_OS_SemWait(App_USBH_CDC_ACM_IsConnSem, 0u);

        err = USBH_ERR_NONE;

        for (i = 0u; i < 6u; i++) {
                                                                /* ------------------ PRINT COMMAND ------------------- */
            APP_TRACE_INFO(("========== AT CMD ==========\r\n"));
            APP_TRACE_INFO(("%s\r\n", App_USBH_CDC_ACM_AT_Cmds[i]));

                                                                /* ------------------ PROCESS AT CMD ------------------ */
            AT_cmd_len = Str_Len((CPU_CHAR *)App_USBH_CDC_ACM_AT_Cmds[i]);
            xfer_len   = USBH_CDC_ACM_DataTx(App_USBH_CDC_ACM_Ptr,
                                             App_USBH_CDC_ACM_AT_Cmds[i],
                                             AT_cmd_len,
                                            &err);

            if (err == USBH_ERR_NONE) {
                xfer_len = USBH_CDC_ACM_DataRx(App_USBH_CDC_ACM_Ptr,
                                               App_USBH_CDC_ACM_RxBuf,
                                               256u,
                                              &err);

                                                                /* --------------- PRINT AT CMD RESULT ---------------- */
                if (err == USBH_ERR_NONE) {

                    APP_TRACE_INFO(("====== STATUS / DATA =======\r\n"));
                    for (ix = 0u; ix < xfer_len; ix++) {
                        APP_TRACE_INFO(("%c", App_USBH_CDC_ACM_RxBuf[ix]));
                    }
                    APP_TRACE_INFO(("\r\n\r\n"));
                } else {
                    APP_TRACE_INFO(("Error in USBH_CDC_ACM_DataRx(): %d\r\n", err));
                }
            } else {
                APP_TRACE_INFO(("Error in USBH_CDC_ACM_DataTx(): %d\r\n", err));
            }
        }
    }
}


/*
*********************************************************************************************************
*                                  App_USBH_CDC_ACM_SerEventNotify()
*
* Description : This function is used to notify the application about the state of the device.
*
* Arguments   : line_status     Status of the device.
*
* Return(s)   : USBH_ERR_NONE.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  App_USBH_CDC_ACM_SerEventNotify (USBH_CDC_SERIAL_STATE  serial_state)
{
    if (serial_state.Break == DEF_TRUE) {
        APP_TRACE_INFO(("break signal\r\n"));
    }
    if (serial_state.RingSignal == DEF_TRUE) {
        APP_TRACE_INFO(("Ring indicator\r\n"));
    }
    if (serial_state.Framing == DEF_TRUE) {
        APP_TRACE_INFO(("Frame error\r\n"));
    }
    if (serial_state.Parity == DEF_TRUE) {
        APP_TRACE_INFO(("Parity error\r\n"));
    }
    if (serial_state.OverRun == DEF_TRUE) {
        APP_TRACE_INFO(("Buf over run\r\n"));
    }
}


/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

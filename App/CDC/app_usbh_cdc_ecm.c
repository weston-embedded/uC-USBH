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
*                                  USB HOST CDC ECM TEST APPLICATION
*
*                                              TEMPLATE
*
* Filename : app_usbh_cdc_ecm.c
* Version  : V3.42.01
*********************************************************************************************************
*/

#define  APP_USBH_CDC_ECM_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <app_cfg.h>
#include  <lib_str.h>
#include  <os.h>
#include  "app_usbh_cdc_ecm.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_USBH_ECM_TASK_STK_SIZE              512u
#define  APP_USBH_ECM_TASK_PRIO                  10u

#define  APP_USBH_ECM_MAC_LEN                    6u
#define  APP_USBH_ECM_MAC_ARP_OFFSET_1           6u
#define  APP_USBH_ECM_MAC_ARP_OFFSET_2           22u

#define  APP_USBH_ECM_PCK_SIZE                   1514u

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

static  OS_TCB             App_USBH_ECM_TaskTCB;
static  CPU_STK            App_USBH_ECM_TaskStk[APP_USBH_ECM_TASK_STK_SIZE];
static  USBH_CDC_DEV      *App_USBH_CDC_Ptr;                          /* Pointer to connected CDC device.               */
static  USBH_CDC_ECM_DEV  *App_USBH_ECM_Ptr;                          /* Pointer to connected CDC ECM device.           */
static  CPU_BOOLEAN        App_USBH_ECM_NetworkConnected = DEF_FALSE; /* Flag to indicate network connection status.    */
static  CPU_BOOLEAN        App_USBH_ECM_StartedRxData    = DEF_FALSE; /* Flag to indicate if Rx data has been started.  */
static  CPU_INT08U         App_USBH_ECM_RxBuf[APP_USBH_ECM_PCK_SIZE];
static  CPU_INT08U         App_USBH_ECM_TxBuf[APP_USBH_ECM_PCK_SIZE];
static  CPU_INT08U         App_USBH_ECM_MACAddr[APP_USBH_ECM_MAC_LEN];

static  CPU_INT08U App_ARP_ReqWhoHas[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x08,
                                          0x06, 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xaa, 0xaa, 0xaa, 0xaa,
                                          0xaa, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa9,
                                          0xfe, 0x36, 0x1a};

static  CPU_INT08U App_ARP_announce[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x08,
                                         0x06, 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xaa, 0xaa, 0xaa, 0xaa,
                                         0xaa, 0xaa, 0xa9, 0xfe, 0x36, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa9,
                                         0xfe, 0x36, 0x1a};


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_CDC_ClassNotify   (void                   *p_class_dev,
                                          CPU_INT08U              state,
                                          void                   *p_ctx);

static  void  App_USBH_ECM_DevConn       (USBH_CDC_DEV           *p_cdc_dev);

static  void  App_USBH_ECM_DevDisconn    (USBH_CDC_DEV           *p_cdc_dev);

static  void  App_USBH_ECM_EventNotify   (void                   *p_arg,
                                          USBH_CDC_ECM_STATE      ecm_state);

static  void  App_USBH_ECM_DataRxCmpl    (void                   *p_data,
                                          CPU_INT08U             *p_buf,
                                          CPU_INT32U              xfer_len,
                                          USBH_ERR                err);

static  void  App_USBH_ECM_Task          (void                   *p_arg);

static  void  App_USBH_ECM_ARPSetMAC     (CPU_INT08U             *p_buf,
                                          CPU_INT08U             *p_mac);


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
*                                       App_USBH_CDC_ECM_Init()
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

USBH_ERR  App_USBH_CDC_ECM_Init (void)
{
    USBH_ERR  err;
    OS_ERR    os_err;

    err = USBH_CDC_ECM_GlobalInit();
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


                                                                /* ----------- TASK FOR SENDING ARP MSG --------------- */
    OSTaskCreate(            &App_USBH_ECM_TaskTCB,
                 (CPU_CHAR*)  "Example USBH ECM Task",
                              App_USBH_ECM_Task,
                              DEF_NULL,
                              APP_USBH_ECM_TASK_PRIO,
                              App_USBH_ECM_TaskStk,
                              0,
                              APP_USBH_ECM_TASK_STK_SIZE,
                              0,
                              0,
                              0,
                              (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                             &os_err);


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
             APP_TRACE_INFO(("CDC ECM Demo: Device Connected\r\n"));

             App_USBH_ECM_DevConn(p_cdc_dev);
             break;

        case USBH_CLASS_DEV_STATE_DISCONN:                      /* ---------------- CDC DEVICE REMOVED ---------------- */
             APP_TRACE_INFO(("CDC ECM Demo: Device Removed\r\n"));

             App_USBH_ECM_DevDisconn(p_cdc_dev);
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                     App_USBH_ECM_DevConn()
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

static  void  App_USBH_ECM_DevConn (USBH_CDC_DEV  *p_cdc_dev)
{
    USBH_ERR    err;
    CPU_INT08U  sub_class;


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

    if (sub_class != USBH_CDC_CONTROL_SUBCLASS_CODE_ENCM) {
        APP_TRACE_INFO(("Subclass not supported\r\n"));
        (void)USBH_CDC_RefRel(p_cdc_dev);
        return;
    }

    App_USBH_ECM_Ptr = USBH_CDC_ECM_Add(App_USBH_CDC_Ptr, &err);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_CDC_ECM_Add(): %d\r\n", err));
        (void)USBH_CDC_RefRel(p_cdc_dev);
        return;
    }

                                                                /* Convert UTF16 MAC address to binary.                 */
    for (int i = 0; i < sizeof(App_USBH_ECM_MACAddr); i++) {
        App_USBH_ECM_MACAddr[i] =  Str_ParseNbr_Int32U((CPU_CHAR*) &App_USBH_ECM_Ptr->MAC_Addr[2 * i],     DEF_NULL, 16) << 4;
        App_USBH_ECM_MACAddr[i] += Str_ParseNbr_Int32U((CPU_CHAR*) &App_USBH_ECM_Ptr->MAC_Addr[2 * i + 1], DEF_NULL, 16);
    }

    USBH_CDC_ECM_EventRxNotifyReg(App_USBH_ECM_Ptr, App_USBH_ECM_EventNotify, DEF_NULL);
}


/*
*********************************************************************************************************
*                                    App_USBH_ECM_DevDisconn()
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

static  void  App_USBH_ECM_DevDisconn (USBH_CDC_DEV  *p_cdc_dev)
{
    (void)USBH_CDC_ECM_Remove(App_USBH_ECM_Ptr);
    (void)USBH_CDC_RefRel(p_cdc_dev);
}


/*
*********************************************************************************************************
*                                    App_USBH_ECM_EventNotify()
*
* Description : This function is called when CDC ECM event occurs.
*
* Argument(s) : p_args          Pointer to arguments passed to USBH_CDC_ECM_EventRxNotifyReg
*
*               ecm_state       State of the CDC ECM device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_ECM_EventNotify   (void                   *p_arg,
                                          USBH_CDC_ECM_STATE      ecm_state)
{
    (void)p_arg;

    if (ecm_state.Event == USBH_CDC_ECM_EVENT_NETWORK_CONNECTION) {
        App_USBH_ECM_NetworkConnected = ecm_state.EventData.ConnectionStatus;

        if (App_USBH_ECM_NetworkConnected && !App_USBH_ECM_StartedRxData) {
            USBH_CDC_DataRxAsync(App_USBH_CDC_Ptr,
                                 App_USBH_ECM_RxBuf,
                                 sizeof(App_USBH_ECM_RxBuf),
                                 App_USBH_ECM_DataRxCmpl,
                                 DEF_NULL);

            App_USBH_ECM_StartedRxData = DEF_TRUE;
        }
    }
}

/*
*********************************************************************************************************
*                                    App_USBH_ECM_DataRxCmpl()
*
* Description : This function is called when CDC ECM event occurs.
*
* Argument(s) : p_data          Context data.
*
*               p_buf           Pointer to the buffer containing the received data.
*
*               xfer_len        Length of the received data.
*
*               err             Error code.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_ECM_DataRxCmpl    (void                   *p_data,
                                          CPU_INT08U             *p_buf,
                                          CPU_INT32U              xfer_len,
                                          USBH_ERR                err)
{
    (void)p_data;
    (void)p_buf;
    (void)xfer_len;
    (void)err;

                                                                /* Continue to receive data.                            */
    USBH_CDC_DataRxAsync(App_USBH_CDC_Ptr,
                         App_USBH_ECM_RxBuf,
                         sizeof(App_USBH_ECM_RxBuf),
                         App_USBH_ECM_DataRxCmpl,
                         DEF_NULL);
}


/*
*********************************************************************************************************
*                                    App_USBH_ECM_Task()
*
* Description : Task to send ECM data.
*
* Argument(s) : p_args          Task arguments, not used
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_ECM_Task   (void *p_arg)
{
    USBH_ERR err;

    (void)p_arg;

    while (DEF_ON) {
        if (App_USBH_ECM_NetworkConnected) {
                                                                /* Send ARP request.                                    */
            Mem_Copy(App_USBH_ECM_TxBuf, App_ARP_ReqWhoHas, sizeof(App_ARP_ReqWhoHas));
            App_USBH_ECM_ARPSetMAC(App_USBH_ECM_TxBuf, App_USBH_ECM_MACAddr);
            USBH_CDC_DataTx(App_USBH_CDC_Ptr, App_USBH_ECM_TxBuf, sizeof(App_ARP_ReqWhoHas), &err);

            OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, DEF_NULL);

                                                                /* Send ARP announce.                                   */
            Mem_Copy(App_USBH_ECM_TxBuf, App_ARP_announce, sizeof(App_ARP_announce));
            App_USBH_ECM_ARPSetMAC(App_USBH_ECM_TxBuf, App_USBH_ECM_MACAddr);
            USBH_CDC_DataTx(App_USBH_CDC_Ptr, App_USBH_ECM_TxBuf, sizeof(App_ARP_announce), &err);
        }

        OSTimeDlyHMSM(0, 0, 1, 0, OS_OPT_TIME_HMSM_STRICT, DEF_NULL);
    }
}


/*
*********************************************************************************************************
*                                    App_USBH_ECM_ARPSetMAC()
*
* Description : Set the MAC address in the ARP packet.
*
* Argument(s) : p_buf           ARP packet to update with the MAC address
*
*               p_mac           MAC address to write to the packet
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_ECM_ARPSetMAC     (CPU_INT08U             *p_buf,
                                          CPU_INT08U             *p_mac)
{
    Mem_Copy(&p_buf[APP_USBH_ECM_MAC_ARP_OFFSET_1], p_mac, APP_USBH_ECM_MAC_LEN);
    Mem_Copy(&p_buf[APP_USBH_ECM_MAC_ARP_OFFSET_2], p_mac, APP_USBH_ECM_MAC_LEN);
}


/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

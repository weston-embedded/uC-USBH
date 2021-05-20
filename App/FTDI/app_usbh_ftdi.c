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
*                                    USB HOST FTDI TEST APPLICATION
*
*                                              TEMPLATE
*
* Filename : app_usbh_ftdi.c
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   APP_USBH_FTDI_MODULE
#include  "app_usbh_ftdi.h"
#include  <app_cfg.h>
#include  <app_usbh.h>
#include  <lib_ascii.h>
#include  <Class/FTDI/usbh_ftdi.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_USBH_FTDI_RX_BUF_SIZE                              512u
#define  APP_USBH_FTDI_TX_BUF_SIZE                               64u
#define  APP_USBH_FTDI_TIMEOUT                                   10u
#define  APP_USBH_FTDI_TX_MSG                                 "USB Host - FTDI Class Demo - Handle # \r\n"
#define  APP_USBH_FTDI_TX_HANDLE_OFFSET                          37u


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

typedef  struct  app_usbh_ftdi_handle_node {
            USBH_FTDI_HANDLE            Handle;
            USBH_FTDI_SERIAL_STATUS     LastSerialStatus;
    struct  app_usbh_ftdi_handle_node  *NextPtr;
} APP_USBH_FTDI_HANDLE_NODE;


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_FTDI_Conn         (USBH_FTDI_HANDLE          ftdi_handle);

static  void  App_USBH_FTDI_Disconn      (USBH_FTDI_HANDLE          ftdi_handle);

static  void  App_USBH_FTDI_RxTask       (void                     *p_ctx);

static  void  App_USBH_FTDI_TxTask       (void                     *p_ctx);

static  void  App_USBH_FTDI_DisplayStatus(CPU_INT08U                handle_ix,
                                          USBH_FTDI_SERIAL_STATUS   serial_status);


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/
                                                                /* Stk size for RX/TX task.                             */
static  USBH_STK                    App_USBH_FTDI_RxTaskStk[APP_CFG_USBH_FTDI_RXTX_TASK_STK_SIZE];
static  USBH_STK                    App_USBH_FTDI_TxTaskStk[APP_CFG_USBH_FTDI_RXTX_TASK_STK_SIZE];
                                                                /* RX/TX buf.                                           */
static  CPU_INT08U                  App_USBH_FTDI_RxBuf[APP_USBH_FTDI_RX_BUF_SIZE];
static  CPU_INT08U                  App_USBH_FTDI_TxBuf[APP_USBH_FTDI_TX_BUF_SIZE];

static  USBH_FTDI_CALLBACKS         App_USBH_FTDI_Callbacks = { /* Callbacks fnct to handle conn/disconn.               */
    App_USBH_FTDI_Conn,
    App_USBH_FTDI_Disconn
};

static  USBH_HMUTEX                 APP_USBH_FTDI_HMutex;       /* Handle on app mutex.                                 */
static  APP_USBH_FTDI_HANDLE_NODE  *APP_USBH_FTDI_HandleHead;   /* Ptr to linked list head.                             */
static  MEM_POOL                    APP_USBH_FTDI_HandlePool;   /* FTDI handle pool.                                    */
static  APP_USBH_FTDI_HANDLE_NODE   APP_USBH_FTDI_HandleTbl[USBH_FTDI_CFG_MAX_DEV];


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
*                                         App_USBH_FTDI_Init()
*
* Description : Register FTDI connection/disconnection callbacks and create Rx/Tx tasks.
*
* Argument(s) : p_err                           Variable that will receive the return error code from this function.
*
*                                               -----     RETURNED BY USBH_FTDI_Init() :    -----
*               USBH_ERR_NONE,                  Interface is of FTDI type.
*               USBH_ERR_DEV_ALLOC,             Cannot allocate FTDI device.
*               USBH_ERR_INVALID_ARG,           If invalid argument(s) passed to 'p_host'/'p_class_drv'.
*               USBH_ERR_CLASS_DRV_ALLOC,       If maximum class driver limit reached.
*
*                                               -----   RETURNED BY USBH_OS_TaskCreate() :  -----
*               USBH_ERR_NONE,                  Task created.
*               USBH_ERR_ALLOC,                 Cannot allocate Task Control Block.
*               USBH_ERR_OS_TASK_CREATE         Cannot create task.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  App_USBH_FTDI_Init (USBH_ERR  *p_err)
{
    CPU_SIZE_T  octets_reqd;
    USBH_HTASK  htask;
    LIB_ERR     err_lib;


    USBH_FTDI_Init(&App_USBH_FTDI_Callbacks, p_err);
    if (*p_err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_OS_TaskCreate(): %d\r\n", *p_err));
        return;
    }

    APP_USBH_FTDI_HandleHead = (APP_USBH_FTDI_HANDLE_NODE *)0;
    Mem_PoolCreate(       &APP_USBH_FTDI_HandlePool,            /* Create mem pool for linked list.                     */
                   (void *)APP_USBH_FTDI_HandleTbl,
                          (sizeof(APP_USBH_FTDI_HANDLE_NODE) * USBH_FTDI_CFG_MAX_DEV),
                           USBH_FTDI_CFG_MAX_DEV,
                           sizeof(APP_USBH_FTDI_HANDLE_NODE),
                           1u,
                          &octets_reqd,
                          &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        APP_TRACE_INFO(("Error in Mem_PoolCreate(): %d\r\n", err_lib));
        return;
    }

    *p_err = USBH_OS_MutexCreate(&APP_USBH_FTDI_HMutex);
     if (*p_err != USBH_ERR_NONE) {
         return;
     }
                                                                /* Create task for RX / TX functions.                   */
    *p_err = USBH_OS_TaskCreate(              "Demo FTDI Rx Task",
                                               APP_CFG_USBH_FTDI_RX_TASK_PRIO,
                                               App_USBH_FTDI_RxTask,
                                (void       *) 0,
                                (CPU_INT32U *)&App_USBH_FTDI_RxTaskStk[0u],
                                               APP_CFG_USBH_FTDI_RXTX_TASK_STK_SIZE,
                                              &htask);
    if (*p_err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_OS_TaskCreate(): %d\r\n", *p_err));
        return;
    }

    *p_err = USBH_OS_TaskCreate(              "Demo FTDI Tx Task",
                                               APP_CFG_USBH_FTDI_TX_TASK_PRIO,
                                               App_USBH_FTDI_TxTask,
                                (void       *) 0,
                                (CPU_INT32U *)&App_USBH_FTDI_TxTaskStk[0u],
                                               APP_CFG_USBH_FTDI_RXTX_TASK_STK_SIZE,
                                              &htask);
    if (*p_err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_OS_TaskCreate(): %d\r\n", *p_err));
        return;
    }
}


/*
*********************************************************************************************************
*                                        App_USBH_FTDI_Conn()
*
* Description : Handle FTDI function connection.
*
* Argument(s) : ftdi_handle     Handle on FTDI device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_FTDI_Conn (USBH_FTDI_HANDLE  ftdi_handle)
{
    APP_USBH_FTDI_HANDLE_NODE  *p_ftdi_handle;
    USBH_ERR                    err;
    LIB_ERR                     err_lib;

                                                                /* Add node to linked list.                             */
    p_ftdi_handle = (APP_USBH_FTDI_HANDLE_NODE *)Mem_PoolBlkGet(&APP_USBH_FTDI_HandlePool,
                                                                 sizeof(APP_USBH_FTDI_HANDLE_NODE),
                                                                &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        APP_TRACE_INFO(("Error in Mem_PoolBlkGet(): %d\r\n", err_lib));
        return;
    }

    (void)USBH_OS_MutexLock(APP_USBH_FTDI_HMutex);

    p_ftdi_handle->Handle    =  ftdi_handle;
    p_ftdi_handle->NextPtr   = (void *)APP_USBH_FTDI_HandleHead;
    APP_USBH_FTDI_HandleHead =  p_ftdi_handle;

    (void)USBH_OS_MutexUnlock(APP_USBH_FTDI_HMutex);

    USBH_FTDI_Reset(ftdi_handle,                                /* Purge RX/TX buf.                                     */
                    USBH_FTDI_RESET_CTRL_SIO,
                   &err);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_FTDI_Reset(): %d\r\n", err));
        return;
    }

    USBH_FTDI_ModemCtrlSet(ftdi_handle,                         /* Ignore hw handshake.                                 */
                          (USBH_FTDI_MODEM_CTRL_DTR_DISABLED | USBH_FTDI_MODEM_CTRL_RTS_DISABLED),
                          &err);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_FTDI_ModemCtrlSet(): %d\r\n", err));
        return;
    }

    USBH_FTDI_BaudRateSet(ftdi_handle,                          /* Set baud rate to use.                                */
                          USBH_FTDI_BAUD_RATE_115200,
                         &err);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_FTDI_BaudRateSet(): %d\r\n", err));
        return;
    }

    USBH_FTDI_DataSet(ftdi_handle,                              /* Set conn data characteristics.                       */
                      0x08u,
                      USBH_FTDI_DATA_PARITY_NONE,
                      USBH_FTDI_DATA_STOP_BITS_1,
                      USBH_FTDI_DATA_BREAK_DISABLED,
                     &err);
    if (err != USBH_ERR_NONE) {
        APP_TRACE_INFO(("Error in USBH_FTDI_DataSet(): %d\r\n", err));
    }
}


/*
*********************************************************************************************************
*                                       App_USBH_FTDI_Disconn()
*
* Description : Handle FTDI function disconnection.
*
* Argument(s) : ftdi_handle     Handle on FTDI device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_FTDI_Disconn (USBH_FTDI_HANDLE  ftdi_handle)
{
    APP_USBH_FTDI_HANDLE_NODE  *p_ftdi_handle;
    APP_USBH_FTDI_HANDLE_NODE  *p_ftdi_handle_prev;
    LIB_ERR                     err_lib;


    (void)USBH_OS_MutexLock(APP_USBH_FTDI_HMutex);

    p_ftdi_handle      = APP_USBH_FTDI_HandleHead;
    p_ftdi_handle_prev = APP_USBH_FTDI_HandleHead;

    while (p_ftdi_handle != (APP_USBH_FTDI_HANDLE_NODE *)0) {
        if (p_ftdi_handle->Handle != ftdi_handle) {             /* Parse linked list for node to remove.                */
            p_ftdi_handle_prev = p_ftdi_handle;
            p_ftdi_handle      = p_ftdi_handle->NextPtr;
        } else {                                                /* Remove node from linked list.                        */
            if (p_ftdi_handle == APP_USBH_FTDI_HandleHead) {
                if (APP_USBH_FTDI_HandleHead->NextPtr == (void *)0) {
                    APP_USBH_FTDI_HandleHead = (void *)0;
                } else {
                    APP_USBH_FTDI_HandleHead = APP_USBH_FTDI_HandleHead->NextPtr;
                }
            } else {
                p_ftdi_handle_prev->NextPtr = p_ftdi_handle->NextPtr;
            }

            Mem_PoolBlkFree(       &APP_USBH_FTDI_HandlePool,   /* Free node in mem pool.                               */
                            (void *)p_ftdi_handle,
                                   &err_lib);
            break;
        }
    }

    (void)USBH_OS_MutexUnlock(APP_USBH_FTDI_HMutex);

    APP_TRACE_INFO(("FTDI Demo: Device Disconnected\r\n"));
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
*                                       App_USBH_FTDI_RxTask()
*
* Description : Receive data from the FTDI device at periodic interval.
*
* Argument(s) : p_ctx       Pointer to the context.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_FTDI_RxTask (void  *p_ctx)
{
    APP_USBH_FTDI_HANDLE_NODE  *p_ftdi_handle_node;
    CPU_INT32U                  xfer_len;
    CPU_INT08U                  ascii_ix;
    USBH_FTDI_SERIAL_STATUS     serial_status;
    USBH_ERR                    err;


    (void)p_ctx;

    while (DEF_ON) {
        if (APP_USBH_FTDI_HandleHead != (APP_USBH_FTDI_HANDLE_NODE *)0) {

            (void)USBH_OS_MutexLock(APP_USBH_FTDI_HMutex);

            p_ftdi_handle_node = APP_USBH_FTDI_HandleHead;

            while (p_ftdi_handle_node != (APP_USBH_FTDI_HANDLE_NODE *)0)
            {
                xfer_len = USBH_FTDI_Rx(         p_ftdi_handle_node->Handle,
                                        (void *)&App_USBH_FTDI_RxBuf[0u],
                                                 sizeof(App_USBH_FTDI_RxBuf),
                                                 APP_USBH_FTDI_TIMEOUT,
                                                &serial_status,
                                                &err);
                if (err == USBH_ERR_NONE) {
                    if (xfer_len > USBH_FTDI_SERIAL_STATUS_LEN) {
                                                                /* Disp handle ix in ASCII.                             */
                        ascii_ix  = (CPU_INT08U)(p_ftdi_handle_node->Handle);
                        ascii_ix +=  ASCII_CHAR_DIGIT_ZERO;

                        App_USBH_FTDI_RxBuf[xfer_len] = 0u;
                        APP_TRACE_INFO(("Handle #%c: ", ascii_ix));
                        APP_TRACE_INFO(("%s", &App_USBH_FTDI_RxBuf[USBH_FTDI_SERIAL_STATUS_LEN]));
                        APP_TRACE_INFO(("\r\n"));
                    }
                } else if (err != USBH_ERR_OS_TIMEOUT) {
                    APP_TRACE_INFO(("Error in USBH_FTDI_Rx(): %d\r\n", err));
                }

                if (err != USBH_ERR_OS_TIMEOUT) {
                    if ((serial_status.LineStatus  != p_ftdi_handle_node->LastSerialStatus.LineStatus) ||
                        (serial_status.ModemStatus != p_ftdi_handle_node->LastSerialStatus.ModemStatus)){
                        App_USBH_FTDI_DisplayStatus((CPU_INT08U)p_ftdi_handle_node->Handle,
                                                                serial_status);
                    }

                    p_ftdi_handle_node->LastSerialStatus = serial_status;
                    p_ftdi_handle_node                   = p_ftdi_handle_node->NextPtr;
                }
            }

            (void)USBH_OS_MutexUnlock(APP_USBH_FTDI_HMutex);
        }

        USBH_OS_DlyMS(999u);
    }
}


/*
*********************************************************************************************************
*                                       App_USBH_FTDI_TxTask()
*
* Description : Send data to FTDI device at periodic interval.
*
* Argument(s) : p_ctx       Pointer to the context.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_FTDI_TxTask (void  *p_ctx)
{
    APP_USBH_FTDI_HANDLE_NODE  *p_ftdi_handle_node;
    CPU_INT08U                  ascii_ix;
    USBH_ERR                    err;


    (void)p_ctx;

    Mem_Copy(&App_USBH_FTDI_TxBuf[0u],
             &APP_USBH_FTDI_TX_MSG,
              sizeof(APP_USBH_FTDI_TX_MSG));

    while (DEF_ON) {
        if (APP_USBH_FTDI_HandleHead != (APP_USBH_FTDI_HANDLE_NODE *)0) {

            (void)USBH_OS_MutexLock(APP_USBH_FTDI_HMutex);

            p_ftdi_handle_node = APP_USBH_FTDI_HandleHead;

            while (p_ftdi_handle_node != (APP_USBH_FTDI_HANDLE_NODE *)0)
            {
                                                                /* Convert handle ix in ASCII.                          */
                ascii_ix  = (CPU_INT08U)(p_ftdi_handle_node->Handle);
                ascii_ix +=  ASCII_CHAR_DIGIT_ZERO;
                App_USBH_FTDI_TxBuf[APP_USBH_FTDI_TX_HANDLE_OFFSET] = ascii_ix;

                USBH_FTDI_Tx(         p_ftdi_handle_node->Handle,
                             (void *)&App_USBH_FTDI_TxBuf[0u],
                                      sizeof(APP_USBH_FTDI_TX_MSG),
                                      APP_USBH_FTDI_TIMEOUT,
                                     &err);
                if ((err != USBH_ERR_NONE) &&
                    (err != USBH_ERR_OS_TIMEOUT)) {
                    APP_TRACE_INFO(("Error in USBH_FTDI_Tx(): %d\r\n", err));
                }

                if (err != USBH_ERR_OS_TIMEOUT) {
                    p_ftdi_handle_node = p_ftdi_handle_node->NextPtr;
                }
            }

            (void)USBH_OS_MutexUnlock(APP_USBH_FTDI_HMutex);
        }

        USBH_OS_DlyMS(999u);
    }
}


/*
*********************************************************************************************************
*                                    App_USBH_FTDI_DisplayStatus()
*
* Description : Format and display the serial status on the terminal.
*
* Argument(s) : handle_ix           FTDI handle index.
*
*               serial_status       Serial status to format.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_FTDI_DisplayStatus (CPU_INT08U               handle_ix,
                                           USBH_FTDI_SERIAL_STATUS  serial_status)
{
    APP_TRACE_INFO(("--------- SERIAL STATUS ----------\r\n"));
    APP_TRACE_INFO((" Handle : %d\r\n", handle_ix));
    APP_TRACE_INFO(("---------- MODEM STATUS ----------\r\n"));
    APP_TRACE_INFO((" Full-speed                   : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.ModemStatus, USBH_FTDI_MODEM_STATUS_FS)));
    APP_TRACE_INFO((" High-speed                   : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.ModemStatus, USBH_FTDI_MODEM_STATUS_HS)));
    APP_TRACE_INFO((" Clear to Send                : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.ModemStatus, USBH_FTDI_MODEM_STATUS_CLEAR_TO_SEND)));
    APP_TRACE_INFO((" Data Set Ready               : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.ModemStatus, USBH_FTDI_MODEM_STATUS_DATA_SET_READY)));
    APP_TRACE_INFO((" Ring Indicator               : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.ModemStatus, USBH_FTDI_MODEM_STATUS_RING_INDICATOR)));
    APP_TRACE_INFO((" Receive Line Signal Detect   : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.ModemStatus, USBH_FTDI_MODEM_STATUS_DATA_CARRIER_DETECT)));
    APP_TRACE_INFO(("----------- LINE STATUS ----------\r\n"));
    APP_TRACE_INFO((" Overrun Error                : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.LineStatus, USBH_FTDI_LINE_STATUS_RX_OVERFLOW_ERROR)));
    APP_TRACE_INFO((" Parity Error                 : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.LineStatus, USBH_FTDI_LINE_STATUS_PARITY_ERROR)));
    APP_TRACE_INFO((" Framing Error                : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.LineStatus, USBH_FTDI_LINE_STATUS_FRAMING_ERROR)));
    APP_TRACE_INFO((" Break Interrupt              : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.LineStatus, USBH_FTDI_LINE_STATUS_BREAK_INTERRUPT)));
    APP_TRACE_INFO((" Transmitter Holding Register : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.LineStatus, USBH_FTDI_LINE_STATUS_TX_REGISTER_EMPTY)));
    APP_TRACE_INFO((" Transmitter Empty            : %u \r\n",
                     DEF_BIT_IS_SET(serial_status.LineStatus, USBH_FTDI_LINE_STATUS_TX_EMPTY)));
    APP_TRACE_INFO(("----------------------------------\r\n"));
}


/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

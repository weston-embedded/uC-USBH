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
*                                    USB HOST MSC TEST APPLICATION
*
*                                              TEMPLATE
*
* Filename : app_usbh_msc.c
* Version  : V3.42.01
*********************************************************************************************************
*/

#define  APP_USBH_MSC_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "app_usbh_msc.h"

#if (APP_CFG_USBH_EN     == DEF_ENABLED) && \
    (APP_CFG_USBH_MSC_EN == DEF_ENABLED)


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  APP_USBH_MSC_FS_EXAMPLE_FILE                    "msc:x:\\MSPrint.txt"
#define  APP_USBH_MSC_FS_BUF_SIZE                         54u


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

static  const  CPU_INT08U  App_USBH_MSC_BufTx[] = "This is the USB Mass Storage Demo sample output file.\0";


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

static  USBH_STK     App_USBH_MSC_FileStack[APP_CFG_USBH_MSC_FILE_TASK_STK_SIZE];
static  CPU_INT08U   App_USBH_MSC_BufRx[APP_USBH_MSC_FS_BUF_SIZE];

static  USBH_HQUEUE  App_USBH_MSC_DevQ;
static  CPU_INT32U   App_USBH_MSC_Q_UnitNbr[USBH_MSC_CFG_MAX_DEV];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_MSC_ClassNotify(void        *p_msc_dev,
                                       CPU_INT08U   is_conn,
                                       void        *p_ctx);

static  void  App_USBH_MSC_FileTask   (void        *p_ctx);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
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
*                                         App_USBH_MSC_Init()
*
* Description : Register MSC notify callback, create demo objects & start demo tasks.
*
* Argument(s) : None.
*
* Return(s)   : USBH_ERR_NONE,          if the demo is initialized.
*               Specific error code,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  App_USBH_MSC_Init (void)
{
    USBH_ERR    err;
    USBH_HTASK  htask;


    App_USBH_MSC_DevQ = USBH_OS_MsgQueueCreate((void *)&App_USBH_MSC_Q_UnitNbr[0u],
                                                        USBH_MSC_CFG_MAX_DEV,
                                                       &err);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_OS_TaskCreate(           "MSC Demo File Task",   /* Create the file I/O task.                            */
                                         APP_CFG_USBH_MSC_FILE_TASK_PRIO,
                                         App_USBH_MSC_FileTask,
                             (void    *) 0,
                             (CPU_STK *)&App_USBH_MSC_FileStack[0],
                                         APP_CFG_USBH_MSC_FILE_TASK_STK_SIZE,
                                        &htask);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_ClassDrvReg(       &USBH_MSC_ClassDrv,           /* Register MSC driver.                                 */
                                   App_USBH_MSC_ClassNotify,
                           (void *)0);
    return(err);
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
*                                     App_USBH_MSC_ClassNotify()
*
* Description : Handle device state change notification for mass storage class devices.
*
* Argument(s) : p_class_dev     Pointer to class device.
*
*               is_conn         Indicate connection status of MSC device.
*                       USBH_CLASS_DEV_STATE_CONN       Device is connected.
*                       USBH_CLASS_DEV_STATE_DISCONN    Device is disconnected.
*
*               p_ctx           Pointer to context.
*
* Return(s)   : USBH_ERR_NONE,      if no error
*               Specific error,     otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_MSC_ClassNotify (void        *p_class_dev,
                                        CPU_INT08U   is_conn,
                                        void        *p_ctx)
{
    USBH_MSC_DEV  *p_msc_dev;
    USBH_ERR       usb_err;
    FS_ERR         fs_err;
    CPU_INT32U     unit_nbr;


    (void)p_ctx;

    p_msc_dev = (USBH_MSC_DEV *)p_class_dev;
    switch (is_conn) {
        case USBH_CLASS_DEV_STATE_CONN:                         /* ------------ MASS STORAGE DEVICE CONN'D ------------ */
             usb_err = USBH_MSC_RefAdd(p_msc_dev);
             if (usb_err != USBH_ERR_NONE) {
                 APP_TRACE_INFO(("Cannot add new MSC reference w/err: %d\r\n", usb_err));
                 return;
             }

             unit_nbr = FSDev_MSC_DevOpen(p_msc_dev, &fs_err);
             if (fs_err != FS_ERR_NONE) {
                 FSDev_MSC_DevClose(p_msc_dev);
                 USBH_MSC_RefRel(p_msc_dev);

                 APP_TRACE_INFO(("MSC Demo: Error opening device\r\n"));
                 usb_err = USBH_ERR_UNKNOWN;
                 return;
             }

             (void)USBH_OS_MsgQueuePut(        App_USBH_MSC_DevQ,
                                       (void *)unit_nbr);

             APP_TRACE_INFO(("MSC Demo: Device with unit #%d connected\r\n", unit_nbr));
             break;

        case USBH_CLASS_DEV_STATE_DISCONN:                      /* ----------- MASS STORAGE DEVICE REMOVED ------------ */
             FSDev_MSC_DevClose(p_msc_dev);
             USBH_MSC_RefRel(p_msc_dev);

             APP_TRACE_INFO(("MSC Demo: Device Removed\r\n"));
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                       App_USBH_MSC_FileTask()
*
* Description : Open, Write, Read and compare a file to a connected mass storage device.
*
* Argument(s) : p_ctx       Pointer to the context.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_MSC_FileTask (void  *p_ctx)
{
    CPU_BOOLEAN   cmp;
    CPU_CHAR      name[sizeof(APP_USBH_MSC_FS_EXAMPLE_FILE)];
    CPU_INT32U    unit_nbr;
    CPU_SIZE_T    len_wr;
    CPU_SIZE_T    len_rd;
    FS_ERR        err_fs;
    FS_FILE      *p_file;
    USBH_ERR      err_usbh;


    (void)p_ctx;
    (void)Str_Copy(&name[0u], APP_USBH_MSC_FS_EXAMPLE_FILE);

    while (DEF_TRUE) {
        unit_nbr = (CPU_INT32U)USBH_OS_MsgQueueGet(App_USBH_MSC_DevQ,
                                                   0u,
                                                  &err_usbh);
        if (err_usbh != USBH_ERR_NONE) {
            APP_TRACE_INFO(("Could not find volume w/err %d.\n\r",
                             err_usbh));
            continue;
        }

        name[4u] = ASCII_CHAR_DIGIT_ZERO + (CPU_CHAR)unit_nbr;

        p_file = FSFile_Open(name,                              /* Create a file on the mass storage device.            */
                             FS_FILE_ACCESS_MODE_CREATE | FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_RD,
                            &err_fs);
        if (err_fs != FS_ERR_NONE) {
            APP_TRACE_INFO(("Could not open file %s w/err %d.\n\r",
                             APP_USBH_MSC_FS_EXAMPLE_FILE,
                             err_fs));
            continue;
        }

        APP_TRACE_INFO(("Writing '%s' to USB drive...",
                         APP_USBH_MSC_FS_EXAMPLE_FILE));

        len_wr = FSFile_Wr(         p_file,
                           (void *)&App_USBH_MSC_BufTx[0u],
                                    APP_USBH_MSC_FS_BUF_SIZE,
                                   &err_fs);
        if ((err_fs != FS_ERR_NONE             ) ||
            (len_wr != APP_USBH_MSC_FS_BUF_SIZE)) {
            APP_TRACE_INFO(("\n\rFSFile_Wr() failed. bytes_written = %d\n\r", len_wr));
            FSFile_Close(p_file, &err_fs);
            continue;
        } else {
            APP_TRACE_INFO(("OK\r\n"));
        }

        FSFile_PosSet(p_file,                                   /* Set the current position of the file pointer.        */
                      0u,
                      FS_FILE_ORIGIN_START,
                     &err_fs);
        if (err_fs != FS_ERR_NONE) {
            FSFile_Close(p_file, &err_fs);
            continue;
        }

        APP_TRACE_INFO(("Reading '%s' from USB drive...",
                         APP_USBH_MSC_FS_EXAMPLE_FILE));

        len_rd = FSFile_Rd(         p_file,
                           (void *)&App_USBH_MSC_BufRx[0u],
                                    APP_USBH_MSC_FS_BUF_SIZE,
                                   &err_fs);
        if ((err_fs != FS_ERR_NONE             ) ||
            (len_rd != APP_USBH_MSC_FS_BUF_SIZE)) {
            APP_TRACE_INFO(("\n\rFSFile_Rd() failed. bytes read = %d\n\r", len_rd));
            FSFile_Close(p_file, &err_fs);
            continue;
        } else {
            APP_TRACE_INFO(("OK\n\r"));
        }


        APP_TRACE_INFO(("Comparing original data and data read from USB drive... "));

        cmp = Mem_Cmp((void *)&App_USBH_MSC_BufTx[0u],
                      (void *)&App_USBH_MSC_BufRx[0u],
                               APP_USBH_MSC_FS_BUF_SIZE);
        if (cmp == DEF_YES) {
            APP_TRACE_INFO(("Passed\n\r"));
        } else {
            APP_TRACE_INFO(("Failed!\n\r"));
        }

        FSFile_Close(p_file, &err_fs);
    }
}

#endif


/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

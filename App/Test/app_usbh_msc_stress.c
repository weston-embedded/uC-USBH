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
* Version  : V3.42.00
*********************************************************************************************************
*/

#define  APP_USBH_MSC_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <app_usbh_msc.h>

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_MSC_DEMO_FS_EXAMPLE_FILE                   "\\MSPrint.txt"
#define  USBH_MSC_DEMO_FS_BUF_SIZE                        (131072u * 1u)

/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
CPU_INT08U  App_USBH_MSC_BufTx[USBH_MSC_CFG_MAX_DEV][USBH_MSC_DEMO_FS_BUF_SIZE];
#endif


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

#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
static  CPU_INT32U   App_USBH_MSC_FileStack[USBH_MSC_CFG_MAX_DEV][APP_CFG_USBH_MSC_FILE_TASK_STK_SIZE];
static  USBH_HSEM    App_USBH_MSC_IsConnSem[USBH_MSC_CFG_MAX_DEV];
static  CPU_INT08U   App_USBH_MSC_BufRx[USBH_MSC_CFG_MAX_DEV][USBH_MSC_DEMO_FS_BUF_SIZE];
static  CPU_BOOLEAN  App_USBH_MSC_Conn[USBH_MSC_CFG_MAX_DEV];
static  CPU_INT08U   App_USBH_MSC_UnitNbr[USBH_MSC_CFG_MAX_DEV];
static  CPU_INT32U   App_USBH_MSC_ErrCnt[USBH_MSC_CFG_MAX_DEV];
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_MSC_ClassNotify(USBH_MSC_DEV  *p_msc_dev,
                                       CPU_INT08U     is_conn,
                                       void          *p_ctx);

#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
static  void  App_USBH_MSC_FileTask   (void          *p_ctx);
#endif

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
* Return(s)   : USBH_ERR_NONE,       if the demo is initialized.
*               Specific error code, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  App_USBH_MSC_Init (void)
{
#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
    USBH_ERR    os_err;
    USBH_HTASK  htask;
#endif
    USBH_ERR    err;
    CPU_INT08U  i;
    CPU_INT32U  buf_cnt;
    CPU_INT08U  cnt;


#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
    for (i = 0u; i < USBH_MSC_CFG_MAX_DEV; i++) {

        os_err = USBH_OS_SemCreate(&App_USBH_MSC_IsConnSem[i],
                                    0u);
        if (os_err != USBH_ERR_NONE) {
            return (os_err);
        }

                                                                    /* Create the file I/O task.                            */
        os_err = USBH_OS_TaskCreate(              "Demo File Task",
                                                   APP_CFG_USBH_MSC_FILE_TASK_PRIO,
                                                   App_USBH_MSC_FileTask,
                                    (void       *) i,
                                    (CPU_STK    *)&App_USBH_MSC_FileStack[i][0u],
                                    (CPU_STK_SIZE) APP_CFG_USBH_MSC_FILE_TASK_STK_SIZE,
                                                  &htask);
        if (os_err != USBH_ERR_NONE) {
            return (os_err);
        }

        App_USBH_MSC_Conn[i]    = DEF_FALSE;
        App_USBH_MSC_UnitNbr[i] = 0u;
        App_USBH_MSC_ErrCnt[i]  = 0u;

        cnt = 0u;
        for (buf_cnt = 0u; buf_cnt < USBH_MSC_DEMO_FS_BUF_SIZE; buf_cnt++) {
            App_USBH_MSC_BufTx[i][buf_cnt] = cnt;
            cnt++;
        }
    }
#endif



                                                                /* Register MSC driver.                                 */
    err = USBH_ClassDrvReg(                       &USBH_MSC_ClassDrv,
                           (USBH_CLASS_NOTIFY_FNCT)App_USBH_MSC_ClassNotify,
                           (void                 *)0);
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
* Argument(s) : pclass_dev      Pointer to class device.
*
*               pctx            Pointer to context.
*
* Return(s)   : USBH_ERR_NONE if no error
*               Specific error, otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_MSC_ClassNotify (USBH_MSC_DEV  *p_class_dev,
                                        CPU_INT08U     is_conn,
                                        void          *p_ctx)
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
                 USBH_PRINT_ERR(usb_err);
                 return;
             }

             unit_nbr = FSDev_MSC_DevOpen(p_msc_dev, &fs_err);
             if (fs_err != FS_ERR_NONE) {
                 FSDev_MSC_DevClose(p_msc_dev);
                 USBH_MSC_RefRel(p_msc_dev);

                 USBH_PRINT_LOG("MSC Demo: Error opening device\n");
                 usb_err = USBH_ERR_UNKNOWN;
                 return;
             }

             USBH_PRINT_LOG("MSC Demo: Device with unit #%d connected\n", unit_nbr);

             App_USBH_MSC_Conn[unit_nbr]                      = DEF_TRUE;
             App_USBH_MSC_UnitNbr[p_msc_dev->DevPtr->DevAddr] = unit_nbr;
             (void)USBH_OS_SemPost(App_USBH_MSC_IsConnSem[unit_nbr]);

             break;

        case USBH_CLASS_DEV_STATE_DISCONN:                      /* ----------- MASS STORAGE DEVICE REMOVED ------------ */
             FSDev_MSC_DevClose(p_msc_dev);
             USBH_MSC_RefRel(p_msc_dev);

             App_USBH_MSC_Conn[App_USBH_MSC_UnitNbr[p_msc_dev->DevPtr->DevAddr]] = DEF_FALSE;
             App_USBH_MSC_UnitNbr[p_msc_dev->DevPtr->DevAddr]                    = 0u;

             USBH_PRINT_LOG("MSC Demo: Device Removed\n");
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                       App_USBH_MSC_FileTask()
*
* Description : Write a file to a connected mass storage device.
*
* Argument(s) : pctx            Pointer to the context.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (APP_CFG_USBH_MSC_EN == DEF_ENABLED)
static  void  App_USBH_MSC_FileTask (void  *p_ctx)
{
    FS_ERR        err;
    CPU_SIZE_T    len_wr;
    CPU_SIZE_T    len_rd;
    CPU_SIZE_T    buf_len;
    FS_FILE      *p_file;
    CPU_BOOLEAN   cmp;
    CPU_INT32U    unit_nbr;
    CPU_CHAR      name[sizeof(USBH_MSC_DEMO_FS_EXAMPLE_FILE) + 6u];


    buf_len  =  sizeof(App_USBH_MSC_BufTx[unit_nbr]);
    unit_nbr = (CPU_INT32U)p_ctx;
    err      = FS_ERR_NONE;

    (void)Str_Copy(&name[0u], (CPU_CHAR *)"msc:x:");
    (void)Str_Copy(&name[6u], USBH_MSC_DEMO_FS_EXAMPLE_FILE);
    name[4u] = ASCII_CHAR_DIGIT_ZERO + (CPU_CHAR)unit_nbr;

    while (DEF_TRUE) {
        USBH_OS_SemWait(App_USBH_MSC_IsConnSem[unit_nbr], 0u);

        while (App_USBH_MSC_Conn[unit_nbr] == DEF_TRUE) {

            if (err != FS_ERR_NONE) {
                err = FS_ERR_NONE;
                App_USBH_MSC_ErrCnt[unit_nbr]++;
            }

            USBH_OS_DlyMS(1u);

                                                                    /* Create a file on the mass storage device.            */
            p_file = FSFile_Open((CPU_CHAR *)name,
                                 FS_FILE_ACCESS_MODE_CREATE | FS_FILE_ACCESS_MODE_WR | FS_FILE_ACCESS_MODE_RD,
                                &err);
            if (err != FS_ERR_NONE) {
                USBH_PRINT_LOG("Could not open file %s w/err %d.\n\r",
                                name,
                                err);
                continue;
            }

            len_wr = FSFile_Wr(         p_file,
                               (void *)&App_USBH_MSC_BufTx[unit_nbr][0u],
                                        buf_len,
                                       &err);

            if ((err    != FS_ERR_NONE                        ) ||
                (len_wr != sizeof(App_USBH_MSC_BufTx[unit_nbr]))) {
                USBH_PRINT_LOG("\n\r%s -> FSFile_Wr() FAILED. bytes_written = %d\n\r", name, len_wr);
                FSFile_Close(p_file, &err);
                continue;
            }

                                                                    /* Set the current position of the file pointer.        */
            FSFile_PosSet(p_file,
                          0u,
                          FS_FILE_ORIGIN_START,
                         &err);
            if (err != FS_ERR_NONE) {
                FSFile_Close(p_file, &err);
                continue;
            }

            len_rd = FSFile_Rd(         p_file,
                               (void *)&App_USBH_MSC_BufRx[unit_nbr][0u],
                                        buf_len,
                                       &err);
            if ((err    != FS_ERR_NONE) ||
                (len_rd != buf_len)) {
                USBH_PRINT_LOG("\n\r%s -> FSFile_Rd() FAILED. bytes read = %d\n\r", name, len_rd);
                FSFile_Close(p_file, &err);
                continue;
            }

            //USBH_PRINT_LOG("Comparing %s... ", name);

            cmp = Mem_Cmp((void *)&App_USBH_MSC_BufTx[unit_nbr][0u],
                          (void *)&App_USBH_MSC_BufRx[unit_nbr][0u],
                                   buf_len);
            if (cmp == DEF_YES) {
                USBH_PRINT_LOG(/*"Passed!\n\r"*/".");
            } else {
                USBH_PRINT_LOG("----FAILED----\n\r");
            }

            FSFile_Close(p_file, &err);
        }
    }
}

#endif


/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

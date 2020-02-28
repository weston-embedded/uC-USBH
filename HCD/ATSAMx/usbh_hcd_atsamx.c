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
*                                    ATSAMX HOST CONTROLLER DRIVER
*
* Filename : usbh_hcd_atsamx.c
* Version  : V3.42.00
*********************************************************************************************************
* Note(s)  : (1) Due to hardware limitations, the ATSAM D5x/E5x host controller does not support a
*                combination of Full-Speed HUB + Low-Speed device
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define  USBH_HCD_ATSAMX_MODULE
#include  "usbh_hcd_atsamx.h"
#include  "../../Source/usbh_hub.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_ATSAMX_CTRLA_HOST_MODE                DEF_BIT_07  /* HOST Operation mode                                  */
#define  USBH_ATSAMX_CTRLA_RUNSTBY                  DEF_BIT_02  /* Run in standby mode                                  */
#define  USBH_ATSAMX_CTRLA_ENABLE                   DEF_BIT_01  /* Enable                                               */
#define  USBH_ATSAMX_CTRLA_SWRST                    DEF_BIT_00  /* Software reset                                       */

#define  USBH_ATSAMX_SYNCBUSY_ENABLE                DEF_BIT_01  /* Synchronization enable status                        */
#define  USBH_ATSAMX_SYNCBUSY_SWRST                 DEF_BIT_00  /* Synchronization Software reset status                */
#define  USBH_ATSAMX_SYNCBUSY_MSK                  (USBH_ATSAMX_SYNCBUSY_ENABLE |      \
                                                    USBH_ATSAMX_SYNCBUSY_SWRST)        \

#define  USBH_ATSAMX_QOSCTRL_DQOS_HIGH             (3u << 2u)   /* Data quality of service set to HIGH critical latency */
#define  USBH_ATSAMX_QOSCTRL_CQOS_HIGH             (3u << 0u)   /* Cfg  quality of service set to HIGH critical latency */

#define  USBH_ATSAMX_FSMSTATE_OFF                   0x01u
#define  USBH_ATSAMX_FSMSTATE_ON                    0x02u
#define  USBH_ATSAMX_FSMSTATE_SUSPEND               0x04u
#define  USBH_ATSAMX_FSMSTATE_SLEEP                 0x08u
#define  USBH_ATSAMX_FSMSTATE_DNRESUME              0x10u
#define  USBH_ATSAMX_FSMSTATE_UPRESUME              0x20u
#define  USBH_ATSAMX_FSMSTATE_RESET                 0x40u

#define  USBH_ATSAMX_CTRLB_L1RESUME                 DEF_BIT_11  /* Send USB L1 resume                                   */
#define  USBH_ATSAMX_CTRLB_VBUSOK                   DEF_BIT_10  /* VBUS is ok                                           */
#define  USBH_ATSAMX_CTRLB_BUSRESET                 DEF_BIT_09  /* Send USB reset                                       */
#define  USBH_ATSAMX_CTRLB_SOFE                     DEF_BIT_08  /* Start-of-Frame enable                                */
#define  USBH_ATSAMX_CTRLB_SPDCONF_LSFS            (0x0u << 2u) /* Speed configuration for host                         */
#define  USBH_ATSAMX_CTRLB_SPDCONF_MSK             (0x3u << 2u)
#define  USBH_ATSAMX_CTRLB_RESUME                   DEF_BIT_01  /* Send USB resume                                      */

#define  USBH_ATSAMX_HSOFC_FLENCE                   DEF_BIT_07  /* Frame length control enable                          */

#define  USBH_ATSAMX_STATUS_SPEED_POS               2u
#define  USBH_ATSAMX_STATUS_SPEED_MSK               0xCu
#define  USBH_ATSAMX_STATUS_SPEED_FS                0x0u
#define  USBH_ATSAMX_STATUS_SPEED_LS                0x1u
#define  USBH_ATSAMX_STATUS_LINESTATE_POS           6u
#define  USBH_ATSAMX_STATUS_LINESTATE_MSK           0xC0u

#define  USBH_ATSAMX_FNUM_MSK                       0x3FF8u
#define  USBH_ATSAMX_FNUM_POS                       3u

#define  USBH_ATSAMX_INT_DDISC                      DEF_BIT_09  /* Device disconnection interrupt                       */
#define  USBH_ATSAMX_INT_DCONN                      DEF_BIT_08  /* Device connection interrtup                          */
#define  USBH_ATSAMX_INT_RAMACER                    DEF_BIT_07  /* RAM acces interrupt                                  */
#define  USBH_ATSAMX_INT_UPRSM                      DEF_BIT_06  /* Upstream resume from the device interrupt            */
#define  USBH_ATSAMX_INT_DNRSM                      DEF_BIT_05  /* Downstream resume interrupt                          */
#define  USBH_ATSAMX_INT_WAKEUP                     DEF_BIT_04  /* Wake up interrupt                                    */
#define  USBH_ATSAMX_INT_RST                        DEF_BIT_03  /* Bus reset interrupt                                  */
#define  USBH_ATSAMX_INT_HSOF                       DEF_BIT_02  /* Host Start-of-Frame interrupt                        */
#define  USBH_ATSAMX_INT_MSK                       (USBH_ATSAMX_INT_DDISC | USBH_ATSAMX_INT_DCONN | USBH_ATSAMX_INT_RAMACER |  \
                                                    USBH_ATSAMX_INT_UPRSM | USBH_ATSAMX_INT_DNRSM | USBH_ATSAMX_INT_WAKEUP  |  \
                                                    USBH_ATSAMX_INT_RST   | USBH_ATSAMX_INT_HSOF)                              \

#define  USBH_ATSAMX_PCFG_PTYPE_MSK                (0x7u << 3u)
#define  USBH_ATSAMX_PCFG_PTYPE_DISABLED           (0x0u << 3u) /* Pipe is disabled                                     */
#define  USBH_ATSAMX_PCFG_PTYPE_CONTROL            (0x1u << 3u) /* Pipe is enabled and configured as CONTROL            */
#define  USBH_ATSAMX_PCFG_PTYPE_ISO                (0x2u << 3u) /* Pipe is enabled and configured as ISO                */
#define  USBH_ATSAMX_PCFG_PTYPE_BULK               (0x3u << 3u) /* Pipe is enabled and configured as BULK               */
#define  USBH_ATSAMX_PCFG_PTYPE_INTERRUPT          (0x4u << 3u) /* Pipe is enabled and configured as INTERRUPT          */
#define  USBH_ATSAMX_PCFG_PTYPE_EXTENDED           (0x5u << 3u) /* Pipe is enabled and configured as EXTENDED           */
#define  USBH_ATSAMX_PCFG_BK                        DEF_BIT_02  /* Pipe bank                                            */
#define  USBH_ATSAMX_PCFG_PTOKEN_SETUP              0x0u
#define  USBH_ATSAMX_PCFG_PTOKEN_IN                 0x1u
#define  USBH_ATSAMX_PCFG_PTOKEN_OUT                0x2u
#define  USBH_ATSAMX_PCFG_PTOKEN_MSK                0x3u

#define  USBH_ATSAMX_PSTATUS_BK1RDY                 DEF_BIT_07  /* Bank 1 ready                                         */
#define  USBH_ATSAMX_PSTATUS_BK0RDY                 DEF_BIT_06  /* Bank 0 ready                                         */
#define  USBH_ATSAMX_PSTATUS_PFREEZE                DEF_BIT_04  /* Pipe freeze                                          */
#define  USBH_ATSAMX_PSTATUS_CURBK                  DEF_BIT_02  /* Current bank                                         */
#define  USBH_ATSAMX_PSTATUS_DTGL                   DEF_BIT_00  /* Data toggle                                          */

#define  USBH_ATSAMX_PINT_STALL                     DEF_BIT_05  /* Pipe stall received interrupt                        */
#define  USBH_ATSAMX_PINT_TXSTP                     DEF_BIT_04  /* Pipe transmitted setup interrupt                     */
#define  USBH_ATSAMX_PINT_PERR                      DEF_BIT_03  /* Pipe error interrupt                                 */
#define  USBH_ATSAMX_PINT_TRFAIL                    DEF_BIT_02  /* Pipe transfer fail interrupt                         */
#define  USBH_ATSAMX_PINT_TRCPT                     DEF_BIT_00  /* Pipe transfer complete x interrupt                   */
#define  USBH_ATSAMX_PINT_ALL                      (USBH_ATSAMX_PINT_STALL | USBH_ATSAMX_PINT_TXSTP | USBH_ATSAMX_PINT_PERR |  \
                                                    USBH_ATSAMX_PINT_TRFAIL | USBH_ATSAMX_PINT_TRCPT)                          \

#define  USBH_ATSAMX_PDESC_BYTE_COUNT_MSK           0x3FFFu
#define  USBH_ATSAMX_PDESC_PCKSIZE_BYTE_COUNT_MSK   0x0FFFFFFFu


/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

#define  USBH_ATSAMX_GET_BYTE_CNT(reg)                ((reg & USBH_ATSAMX_PDESC_BYTE_COUNT_MSK) >>  0u)
#define  USBH_ATSAMX_GET_DPID(reg)                    ((reg & USBH_ATSAMX_PSTATUS_DTGL        ) >>  0u)
#define  USBH_ATSAMX_PCFG_PTYPE(value)                ((value + 1u) << 3u)

#define  USBH_ATSAMX_WAIT_FOR_SYNC(p_reg, value)      do {                                                                   \
                                                          ;     /* Wait for synchronizations bits to be completed       */   \
                                                      } while ((p_reg)->SYNCBUSY & (value));                                 \

#define  USBH_ATSAMX_CTRLA_READ(p_reg, value)         do {                                                                   \
                                                          USBH_ATSAMX_WAIT_FOR_SYNC((p_reg), USBH_ATSAMX_SYNCBUSY_MSK);      \
                                                          (value) = (p_reg)->CTRLA;                                          \
                                                      } while (0u);                                                          \

#define  USBH_ATSAMX_CTRLA_WRITE(p_reg, value)        do {                                                                   \
                                                          CPU_CRITICAL_ENTER();                                              \
                                                          (p_reg)->CTRLA = (value);                                          \
                                                          USBH_ATSAMX_WAIT_FOR_SYNC((p_reg), USBH_ATSAMX_SYNCBUSY_MSK);      \
                                                          CPU_CRITICAL_EXIT();                                               \
                                                      } while (0u);                                                          \


/*
*********************************************************************************************************
*                                   ATSAMD5X/ATSAME5X DEFINES
*********************************************************************************************************
*/
                                                                /* NVM software calibration area address                */
#define  ATSAMX_NVM_SW_CAL_AREA           ((CPU_REG32 *)0x00800080u)
#define  ATSAMX_NVM_USB_TRANSN_POS                  32u
#define  ATSAMX_NVM_USB_TRANSP_POS                  37u
#define  ATSAMX_NVM_USB_TRIM_POS                    42u

#define  ATSAMX_NVM_USB_TRANSN_SIZE                  5u
#define  ATSAMX_NVM_USB_TRANSP_SIZE                  5u
#define  ATSAMX_NVM_USB_TRIM_SIZE                    3u

#define  ATSAMX_NVM_USB_TRANSN      ((*(ATSAMX_NVM_SW_CAL_AREA + (ATSAMX_NVM_USB_TRANSN_POS / 32)) >> (ATSAMX_NVM_USB_TRANSN_POS % 32)) & ((1 << ATSAMX_NVM_USB_TRANSN_SIZE) - 1))
#define  ATSAMX_NVM_USB_TRANSP      ((*(ATSAMX_NVM_SW_CAL_AREA + (ATSAMX_NVM_USB_TRANSP_POS / 32)) >> (ATSAMX_NVM_USB_TRANSP_POS % 32)) & ((1 << ATSAMX_NVM_USB_TRANSP_SIZE) - 1))
#define  ATSAMX_NVM_USB_TRIM        ((*(ATSAMX_NVM_SW_CAL_AREA + (ATSAMX_NVM_USB_TRIM_POS   / 32)) >> (ATSAMX_NVM_USB_TRIM_POS   % 32)) & ((1 << ATSAMX_NVM_USB_TRIM_SIZE)   - 1))

#define  ATSAMX_NVM_USB_TRANSN_MSK                  0x1Fu
#define  ATSAMX_NVM_USB_TRANSP_MSK                  0x1Fu
#define  ATSAMX_NVM_USB_TRIM_MSK                    0x07u

#define  ATSAMX_MAX_NBR_PIPE                         8u         /* Maximum number of host pipes                         */
#define  ATSAMX_INVALID_PIPE                      0xFFu         /* Invalid pipe number.                                 */
#define  ATSAMX_DFLT_EP_ADDR                    0xFFFFu         /* Default endpoint address.                            */

                                                                /* If necessary, re-define these values in 'usbh_cfg.h' */
#ifndef  ATSAMX_URB_PROC_TASK_STK_SIZE
#define  ATSAMX_URB_PROC_TASK_STK_SIZE             256u
#endif

#ifndef  ATSAMX_URB_PROC_TASK_PRIO
#define  ATSAMX_URB_PROC_TASK_PRIO                  15u
#endif

#ifndef  ATSAMX_URB_PROC_Q_MAX
#define  ATSAMX_URB_PROC_Q_MAX                       8u
#endif


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

typedef struct usbh_atsamx_desc_bank {
                                                                /* ----------- USB HOST PIPE DESCRIPTOR BANK ---------- */
    CPU_REG32               ADDR;                               /* Address of the data buffer                           */
    CPU_REG32               PCKSIZE;                            /* Packet size                                          */
    CPU_REG16               EXTREG;                             /* Extended register                                    */
    CPU_REG08               STATUS_BK;                          /* Host status bank                                     */
    CPU_REG08               RSVD0;
    CPU_REG16               CTRL_PIPE;                          /* Host control pipe                                    */
    CPU_REG16               STATUS_PIPE;                        /* Host Status pipe                                     */
} USBH_ATSAMX_DESC_BANK;


typedef struct usbh_atsamx_pipe_desc {
    USBH_ATSAMX_DESC_BANK   DescBank[2u];
} USBH_ATSAMX_PIPE_DESC;


typedef struct usbh_atsamx_pipe_reg {
                                                                /* -------------- USB HOST PIPE REGISTERS ------------- */
    CPU_REG08               PCFG;                               /* Host pipe configuration                              */
    CPU_REG08               RSVD0[2u];
    CPU_REG08               BINTERVAL;                          /* Interval for the Bulk-Out/Ping transaction           */
    CPU_REG08               PSTATUSCLR;                         /* Pipe status clear                                    */
    CPU_REG08               PSTATUSSET;                         /* Pipe status set                                      */
    CPU_REG08               PSTATUS;                            /* Pipe status register                                 */
    CPU_REG08               PINTFLAG;                           /* Host pipe interrupt flag register                    */
    CPU_REG08               PINTENCLR;                          /* Host pipe interrupt clear register                   */
    CPU_REG08               PINTENSET;                          /* Host pipe interrupt set register                     */
    CPU_REG08               RSVD1[22u];
} USBH_ATSAMX_PIPE_REG;


typedef struct usbh_atsamx_reg {
                                                                /* ------------ USB HOST GENERAL REGISTERS ------------ */
    CPU_REG08               CTRLA;                              /* Control A                                            */
    CPU_REG08               RSVD0;
    CPU_REG08               SYNCBUSY;                           /* Synchronization Busy                                 */
    CPU_REG08               QOSCTRL;                            /* QOS control                                          */
    CPU_REG32               RSVD1;
    CPU_REG16               CTRLB;                              /* Control B                                            */
    CPU_REG08               HSOFC;                              /* Host Start-of-Frame control                          */
    CPU_REG08               RSVD2;
    CPU_REG08               STATUS;                             /* Status                                               */
    CPU_REG08               FSMSTATUS;                          /* Finite state machine status                          */
    CPU_REG16               RSVD3;
    CPU_REG16               FNUM;                               /* Host frame number                                    */
    CPU_REG08               FLENHIGH;                           /* Host frame length                                    */
    CPU_REG08               RSVD4;
    CPU_REG16               INTENCLR;                           /* Host interrupt enable register clear                 */
    CPU_REG16               RSVD5;
    CPU_REG16               INTENSET;                           /* Host interrupt enable register set                   */
    CPU_REG16               RSVD6;
    CPU_REG16               INTFLAG;                            /* Host interrupt flag status and clear                 */
    CPU_REG16               RSVD7;
    CPU_REG16               PINTSMRY;                           /* Pipe interrupt summary                               */
    CPU_REG16               RSVD8;
    CPU_REG32               DESCADD;                            /* Descriptor address                                   */
    CPU_REG16               PADCAL;                             /* Pad calibration                                      */
    CPU_REG08               RSVD9[214u];
    USBH_ATSAMX_PIPE_REG    HPIPE[ATSAMX_MAX_NBR_PIPE];         /* Host pipes                                           */
} USBH_ATSAMX_REG;


typedef  struct  usbh_atsamx_pinfo {
    CPU_INT16U              EP_Addr;                            /* Device addr | EP DIR | EP NBR.                       */
                                                                /* To ensure the URB EP xfer size is not corrupted ...  */
    CPU_INT16U              AppBufLen;                          /* ... for multi-transaction transfer                   */
    CPU_INT32U              NextXferLen;
    USBH_URB               *URB_Ptr;
} USBH_ATSAMX_PINFO;


typedef  struct  usbh_drv_data {
    USBH_ATSAMX_PIPE_DESC   DescTbl[ATSAMX_MAX_NBR_PIPE];
    USBH_ATSAMX_PINFO       PipeTbl[ATSAMX_MAX_NBR_PIPE];
    CPU_INT16U              PipeUsed;                           /* Bit array for BULK, ISOC, INTR, CTRL pipe mgmt.      */
    CPU_INT08U              RH_Desc[USBH_HUB_LEN_HUB_DESC];     /* RH desc content.                                     */
    CPU_INT16U              RH_PortStat;                        /* Root Hub Port status.                                */
    CPU_INT16U              RH_PortChng;                        /* Root Hub Port status change.                         */
} USBH_DRV_DATA;


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

static            MEM_POOL     ATSAMX_DrvMemPool;
static  volatile  USBH_HQUEUE  ATSAMX_URB_Proc_Q;
static            USBH_URB     ATSAMX_Q_UrbEp[ATSAMX_URB_PROC_Q_MAX];
static            CPU_STK      ATSAMX_URB_ProcTaskStk[ATSAMX_URB_PROC_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                       DRIVER FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* --------------- DRIVER API FUNCTIONS --------------- */
static  void           USBH_ATSAMX_HCD_Init            (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_Start           (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_Stop            (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_ERR               *p_err);

static  USBH_DEV_SPD   USBH_ATSAMX_HCD_SpdGet          (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_Suspend         (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_Resume          (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_ERR               *p_err);

static  CPU_INT32U     USBH_ATSAMX_HCD_FrameNbrGet     (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_EP_Open         (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_EP                *p_ep,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_EP_Close        (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_EP                *p_ep,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_EP_Abort        (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_EP                *p_ep,
                                                        USBH_ERR               *p_err);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_IsHalt_EP       (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_EP                *p_ep,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_URB_Submit      (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_URB               *p_urb,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_URB_Complete    (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_URB               *p_urb,
                                                        USBH_ERR               *p_err);

static  void           USBH_ATSAMX_HCD_URB_Abort       (USBH_HC_DRV            *p_hc_drv,
                                                        USBH_URB               *p_urb,
                                                        USBH_ERR               *p_err);

                                                                /* -------------- ROOT HUB API FUNCTIONS -------------- */
static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortStatusGet   (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr,
                                                        USBH_HUB_PORT_STATUS   *p_port_status);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_HubDescGet      (USBH_HC_DRV            *p_hc_drv,
                                                        void                   *p_buf,
                                                        CPU_INT08U              buf_len);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortEnSet       (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortEnClr       (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortEnChngClr   (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortPwrSet      (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortPwrClr      (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortResetSet    (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortResetChngClr(USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortSuspendClr  (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_PortConnChngClr (USBH_HC_DRV            *p_hc_drv,
                                                        CPU_INT08U              port_nbr);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_RHSC_IntEn      (USBH_HC_DRV            *p_hc_drv);

static  CPU_BOOLEAN    USBH_ATSAMX_HCD_RHSC_IntDis     (USBH_HC_DRV            *p_hc_drv);


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void           USBH_ATSAMX_ISR_Handler         (void                   *p_drv);

static  void           USBH_ATSAMX_URB_ProcTask        (void                   *p_arg);

static  CPU_INT08U     USBH_ATSAMX_GetFreePipe         (USBH_DRV_DATA          *p_drv_data);

static  CPU_INT08U     USBH_ATSAMX_GetPipeNbr          (USBH_DRV_DATA          *p_drv_data,
                                                        USBH_EP                *p_ep);

static  void           USBH_ATSAMX_PipeCfg             (USBH_URB               *p_urb,
                                                        USBH_ATSAMX_PIPE_REG   *p_reg_hpipe,
                                                        USBH_ATSAMX_PINFO      *p_pipe_info,
                                                        USBH_ATSAMX_DESC_BANK  *p_desc_bank);


/*
*********************************************************************************************************
*                                    INITIALIZED GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_HC_DRV_API  USBH_ATSAMX_HCD_DrvAPI = {
    USBH_ATSAMX_HCD_Init,
    USBH_ATSAMX_HCD_Start,
    USBH_ATSAMX_HCD_Stop,
    USBH_ATSAMX_HCD_SpdGet,
    USBH_ATSAMX_HCD_Suspend,
    USBH_ATSAMX_HCD_Resume,
    USBH_ATSAMX_HCD_FrameNbrGet,

    USBH_ATSAMX_HCD_EP_Open,
    USBH_ATSAMX_HCD_EP_Close,
    USBH_ATSAMX_HCD_EP_Abort,
    USBH_ATSAMX_HCD_IsHalt_EP,

    USBH_ATSAMX_HCD_URB_Submit,
    USBH_ATSAMX_HCD_URB_Complete,
    USBH_ATSAMX_HCD_URB_Abort,
};

USBH_HC_RH_API  USBH_ATSAMX_HCD_RH_API = {
    USBH_ATSAMX_HCD_PortStatusGet,
    USBH_ATSAMX_HCD_HubDescGet,

    USBH_ATSAMX_HCD_PortEnSet,
    USBH_ATSAMX_HCD_PortEnClr,
    USBH_ATSAMX_HCD_PortEnChngClr,

    USBH_ATSAMX_HCD_PortPwrSet,
    USBH_ATSAMX_HCD_PortPwrClr,

    USBH_ATSAMX_HCD_PortResetSet,
    USBH_ATSAMX_HCD_PortResetChngClr,

    USBH_ATSAMX_HCD_PortSuspendClr,
    USBH_ATSAMX_HCD_PortConnChngClr,

    USBH_ATSAMX_HCD_RHSC_IntEn,
    USBH_ATSAMX_HCD_RHSC_IntDis
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
*                                       USBH_ATSAMX_HCD_Init()
*
* Description : Initialize host controller and allocate driver's internal memory/variables.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD init successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_Init (USBH_HC_DRV  *p_hc_drv,
                                    USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;
    CPU_SIZE_T      octets_reqd;
    USBH_HTASK      htask;
    LIB_ERR         err_lib;


    p_drv_data = (USBH_DRV_DATA *)Mem_HeapAlloc( sizeof(USBH_DRV_DATA),
                                                 sizeof(CPU_ALIGN),
                                                &octets_reqd,
                                                &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

    Mem_Clr(p_drv_data, sizeof(USBH_DRV_DATA));
    p_hc_drv->DataPtr = (void *)p_drv_data;

    if ((p_hc_drv->HC_CfgPtr->DataBufMaxLen % USBH_MAX_EP_SIZE_TYPE_BULK_FS) != 0u) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

                                                                /* Create Mem pool area to be used for DMA alignment    */
    Mem_PoolCreate(&ATSAMX_DrvMemPool,
                    DEF_NULL,
                    p_hc_drv->HC_CfgPtr->DataBufMaxLen,
                   (p_hc_drv->HC_CfgPtr->MaxNbrEP_BulkOpen + p_hc_drv->HC_CfgPtr->MaxNbrEP_IntrOpen + 1u),
                    p_hc_drv->HC_CfgPtr->DataBufMaxLen,
                    sizeof(CPU_ALIGN),
                   &octets_reqd,
                   &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

    ATSAMX_URB_Proc_Q = USBH_OS_MsgQueueCreate((void *)&ATSAMX_Q_UrbEp[0u],
                                                        ATSAMX_URB_PROC_Q_MAX,
                                                        p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

                                                                /* Create URB Process task for URB handling.            */
   *p_err = USBH_OS_TaskCreate(              "ATSAMX URB Process",
                                              ATSAMX_URB_PROC_TASK_PRIO,
                                             &USBH_ATSAMX_URB_ProcTask,
                               (void       *) p_hc_drv,
                               (CPU_INT32U *)&ATSAMX_URB_ProcTaskStk[0u],
                                              ATSAMX_URB_PROC_TASK_STK_SIZE,
                                             &htask);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_ATSAMX_HCD_Start()
*
* Description : Start Host Controller.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD start successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_Start (USBH_HC_DRV  *p_hc_drv,
                                     USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_HC_BSP_API  *p_bsp_api;
    USBH_DRV_DATA    *p_drv_data;
    CPU_INT32U        pad_transn;
    CPU_INT32U        pad_transp;
    CPU_INT32U        pad_trim;
    CPU_INT08U        reg_val;
    CPU_INT08U        i;
    CPU_SR_ALLOC();


    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;
    p_bsp_api  =                    p_hc_drv->BSP_API_Ptr;

    if (p_bsp_api->Init != (void *)0) {
        p_bsp_api->Init(p_hc_drv, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    if (!(p_reg->SYNCBUSY & USBH_ATSAMX_SYNCBUSY_SWRST)) {      /* Check if synchronization is completed                */
        USBH_ATSAMX_CTRLA_READ(p_reg, reg_val);
        if (reg_val & USBH_ATSAMX_CTRLA_ENABLE) {
            reg_val &= ~USBH_ATSAMX_CTRLA_ENABLE;
            USBH_ATSAMX_CTRLA_WRITE(p_reg, reg_val);            /* Disable USB peripheral                               */
            USBH_ATSAMX_WAIT_FOR_SYNC(p_reg, USBH_ATSAMX_SYNCBUSY_ENABLE)
        }
                                                                /* Resets all regs in the USB to initial state, and ..  */
                                                                /* .. the USB will be disabled                          */
        USBH_ATSAMX_CTRLA_WRITE(p_reg, USBH_ATSAMX_CTRLA_SWRST);
    }

    USBH_ATSAMX_WAIT_FOR_SYNC(p_reg, USBH_ATSAMX_SYNCBUSY_SWRST)

                                                                /* -- LOAD PAD CALIBRATION REG WITH PRODUCTION VALUES-- */
    pad_transn = ATSAMX_NVM_USB_TRANSN;
    pad_transp = ATSAMX_NVM_USB_TRANSP;
    pad_trim   = ATSAMX_NVM_USB_TRIM;

    if ((pad_transn == 0u) || (pad_transn == ATSAMX_NVM_USB_TRANSN_MSK)) {
        pad_transn = 9u;                                        /* Default value                                        */
    }
    if ((pad_transp == 0u) || (pad_transp == ATSAMX_NVM_USB_TRANSP_MSK)) {
        pad_transp = 25u;                                       /* Default value                                        */
    }
    if ((pad_trim == 0u) || (pad_trim ==  ATSAMX_NVM_USB_TRIM_MSK)) {
        pad_trim = 6u;                                          /* Default value                                        */
    }
    p_reg->PADCAL  = ((pad_transp << 0u) | (pad_transn << 6) | (pad_trim << 12u));

                                                                /* Write quality of service RAM access                  */
    p_reg->QOSCTRL = (USBH_ATSAMX_QOSCTRL_DQOS_HIGH | USBH_ATSAMX_QOSCTRL_CQOS_HIGH);
                                                                /* Set Host mode & set USB clk to run in standby mode   */
    USBH_ATSAMX_CTRLA_WRITE(p_reg, (USBH_ATSAMX_CTRLA_HOST_MODE | USBH_ATSAMX_CTRLA_RUNSTBY));

    p_reg->DESCADD = (CPU_INT32U)&p_drv_data->DescTbl[0u];      /* Set Pipe Descriptor address                          */

    p_reg->CTRLB &= ~USBH_ATSAMX_CTRLB_SPDCONF_MSK;             /* Clear USB speed configuration                        */
    p_reg->CTRLB |= (USBH_ATSAMX_CTRLB_SPDCONF_LSFS |           /* Set USB LS/FS speed configuration                    */
                     USBH_ATSAMX_CTRLB_VBUSOK);

    if (p_bsp_api->ISR_Reg != (void *)0) {
        p_bsp_api->ISR_Reg(USBH_ATSAMX_ISR_Handler, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    USBH_ATSAMX_CTRLA_READ(p_reg, reg_val);
    if ((reg_val & USBH_ATSAMX_CTRLA_ENABLE) == 0u) {
        USBH_ATSAMX_CTRLA_WRITE(p_reg, (reg_val | USBH_ATSAMX_CTRLA_ENABLE));
        USBH_ATSAMX_WAIT_FOR_SYNC(p_reg, USBH_ATSAMX_SYNCBUSY_ENABLE)
    }

    for (i = 0u; i < ATSAMX_MAX_NBR_PIPE; i++) {
        p_reg->HPIPE[i].PCFG               = 0u;                /* Disable the pipe                                     */
                                                                /* Set default pipe info fields.                        */
        p_drv_data->PipeTbl[i].AppBufLen   = 0u;
        p_drv_data->PipeTbl[i].NextXferLen = 0u;
        p_drv_data->PipeTbl[i].EP_Addr     = ATSAMX_DFLT_EP_ADDR;
    }

    p_reg->INTFLAG  =  USBH_ATSAMX_INT_MSK;                     /* Clear all interrupts                                 */
    p_reg->INTENSET = (USBH_ATSAMX_INT_DCONN |                  /* Enable interrupts to detect connection               */
                       USBH_ATSAMX_INT_RST   |
                       USBH_ATSAMX_INT_WAKEUP);

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_ATSAMX_HCD_Stop()
*
* Description : Stop Host Controller.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD stop successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_Stop (USBH_HC_DRV  *p_hc_drv,
                                    USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_HC_BSP_API  *p_bsp_api;


    p_reg     = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_bsp_api =  p_hc_drv->BSP_API_Ptr;

    p_reg->INTENCLR = USBH_ATSAMX_INT_MSK;                      /* Disable all interrupts                               */
    p_reg->INTFLAG  = USBH_ATSAMX_INT_MSK;                      /* Clear all interrupts                                 */

    if (p_bsp_api->ISR_Unreg != (void *)0) {
        p_bsp_api->ISR_Unreg(p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    p_reg->CTRLB &= ~USBH_ATSAMX_CTRLB_VBUSOK;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_ATSAMX_HCD_SpdGet()
*
* Description : Returns Host Controller speed.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Host controller speed retrieved successfuly.
*
* Return(s)   : Host controller speed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_DEV_SPD  USBH_ATSAMX_HCD_SpdGet (USBH_HC_DRV  *p_hc_drv,
                                              USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;

    return (USBH_DEV_SPD_FULL);
}


/*
*********************************************************************************************************
*                                      USBH_ATSAMX_HCD_Suspend()
*
* Description : Suspend Host Controller.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD suspend successful.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_Suspend (USBH_HC_DRV  *p_hc_drv,
                                       USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;
    CPU_INT08U        pipe_nbr;


    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;

    for (pipe_nbr = 0u; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
        if (DEF_BIT_IS_SET(p_drv_data->PipeUsed, DEF_BIT(pipe_nbr))) {
            p_reg->HPIPE[pipe_nbr].PSTATUSSET = USBH_ATSAMX_PSTATUS_PFREEZE;  /* Stop transfer                          */
        }
    }

    USBH_OS_DlyMS(1u);                                          /* wait at least 1 complete frame                       */

    p_reg->CTRLB &= ~USBH_ATSAMX_CTRLB_SOFE;                    /* Stop sending start of frames                         */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_ATSAMX_HCD_Resume()
*
* Description : Resume Host Controller.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD resume successful.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_Resume (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;
    CPU_INT08U        pipe_nbr;


    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;

    p_reg->CTRLB    |= USBH_ATSAMX_CTRLB_SOFE;                  /* Start sending start of frames                        */
    p_reg->CTRLB    |= USBH_ATSAMX_CTRLB_RESUME;                /* Do resume to downstream                              */
    p_reg->INTENSET  = USBH_ATSAMX_INT_WAKEUP;                  /* Force a wakeup interrupt                             */

    USBH_OS_DlyMS(20u);

    for (pipe_nbr = 0u; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
        if (DEF_BIT_IS_SET(p_drv_data->PipeUsed, DEF_BIT(pipe_nbr))) {
            p_reg->HPIPE[pipe_nbr].PSTATUSCLR = USBH_ATSAMX_PSTATUS_PFREEZE;  /* Start transfer                         */
        }
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_FrameNbrGet()
*
* Description : Retrieve current frame number.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HC frame number retrieved successfuly.
*
* Return(s)   : Frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_ATSAMX_HCD_FrameNbrGet (USBH_HC_DRV  *p_hc_drv,
                                                 USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    CPU_INT32U        frm_nbr;


    p_reg   = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    frm_nbr = (p_reg->FNUM & USBH_ATSAMX_FNUM_MSK) >> USBH_ATSAMX_FNUM_POS;

   *p_err = USBH_ERR_NONE;

    return (frm_nbr);
}


/*
*********************************************************************************************************
*                                      USBH_ATSAMX_HCD_EP_Open()
*
* Description : Allocate/open endpoint of given type.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Endpoint opened successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_EP_Open (USBH_HC_DRV  *p_hc_drv,
                                       USBH_EP      *p_ep,
                                       USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;


    p_reg         = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_ep->DataPID =  0u;                                        /* Set PID to DATA0                                     */
    p_ep->URB.Err =  USBH_ERR_NONE;
   *p_err         =  USBH_ERR_NONE;

    if (p_reg->STATUS == 0u) {                                  /* Do not open Endpoint if device is disconnected       */
       *p_err = USBH_ERR_FAIL;
        return;
    }
}


/*
*********************************************************************************************************
*                                     USBH_ATSAMX_HCD_EP_Close()
*
* Description : Close specified endpoint.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Endpoint closed successfully.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_EP_Close (USBH_HC_DRV  *p_hc_drv,
                                        USBH_EP      *p_ep,
                                        USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;
    CPU_INT08U        pipe_nbr;
    LIB_ERR           err_lib;


    p_reg         = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data    = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;
    pipe_nbr      =  USBH_ATSAMX_GetPipeNbr(p_drv_data, p_ep);
    p_ep->DataPID =  0u;                                        /* Set PID to DATA0                                      */
   *p_err         =  USBH_ERR_NONE;

    if (p_ep->URB.DMA_BufPtr != (void *)0u) {
        Mem_PoolBlkFree(&ATSAMX_DrvMemPool,
                         p_ep->URB.DMA_BufPtr,
                        &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
           *p_err = USBH_ERR_HC_ALLOC;
            return;
        }
        p_ep->URB.DMA_BufPtr = (void *)0u;
    }

    if (pipe_nbr != ATSAMX_INVALID_PIPE) {
        p_reg->HPIPE[pipe_nbr].PINTENCLR = USBH_ATSAMX_PINT_ALL;/* Disable all pipe interrupts                           */
        p_reg->HPIPE[pipe_nbr].PINTFLAG  = USBH_ATSAMX_PINT_ALL;/* Clear   all pipe interrupt flags                      */
        p_reg->HPIPE[pipe_nbr].PCFG      = 0u;                  /* Disable the pipe                                      */

        p_drv_data->PipeTbl[pipe_nbr].EP_Addr     = ATSAMX_DFLT_EP_ADDR;
        p_drv_data->PipeTbl[pipe_nbr].AppBufLen   = 0u;
        p_drv_data->PipeTbl[pipe_nbr].NextXferLen = 0u;
        DEF_BIT_CLR(p_drv_data->PipeUsed, DEF_BIT(pipe_nbr));
    }
}


/*
*********************************************************************************************************
*                                     USBH_ATSAMX_HCD_EP_Abort()
*
* Description : Abort all pending URBs on endpoint.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Endpoint aborted successfuly.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_EP_Abort (USBH_HC_DRV  *p_hc_drv,
                                        USBH_EP      *p_ep,
                                        USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_ep;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_IsHalt_EP()
*
* Description : Retrieve endpoint halt state.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Endpoint halt status retrieved successfuly.
*
* Return(s)   : DEF_TRUE,       If endpoint halted.
*
*               DEF_FALSE,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_IsHalt_EP (USBH_HC_DRV  *p_hc_drv,
                                                USBH_EP      *p_ep,
                                                USBH_ERR     *p_err)
{
    (void)p_hc_drv;


   *p_err = USBH_ERR_NONE;
    if (p_ep->URB.Err == USBH_ERR_HC_IO) {
        return (DEF_TRUE);
    }

    return (DEF_FALSE);
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_URB_Submit()
*
* Description : Submit specified URB.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_urb        Pointer to URB structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           URB submitted successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) If Device is disconnected & some remaining URBs processed by AsyncThread, ignore it
*               (2) Set minimum BULK interval to 1ms to avoid taking all bandwidth.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_URB_Submit (USBH_HC_DRV  *p_hc_drv,
                                          USBH_URB     *p_urb,
                                          USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;
    CPU_INT08U        ep_type;
    CPU_INT08U        pipe_nbr;
    LIB_ERR           err_lib;


    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;
    ep_type    =  USBH_EP_TypeGet(p_urb->EP_Ptr);


    if (p_urb->State == USBH_URB_STATE_ABORTED) {               /* See note 1.                                          */
       *p_err = USBH_ERR_FAIL;
        return;
    }

    pipe_nbr = USBH_ATSAMX_GetFreePipe(p_drv_data);
    if (pipe_nbr == ATSAMX_INVALID_PIPE){
       *p_err = USBH_ERR_EP_ALLOC;
        return;
    }

    p_drv_data->PipeTbl[pipe_nbr].EP_Addr     = ((p_urb->EP_Ptr->DevAddr << 8u)  |
                                                  p_urb->EP_Ptr->Desc.bEndpointAddress);
    p_drv_data->PipeTbl[pipe_nbr].AppBufLen   = 0u;
    p_drv_data->PipeTbl[pipe_nbr].NextXferLen = 0u;

    if (p_urb->DMA_BufPtr == (void *)0u) {
        p_urb->DMA_BufPtr = Mem_PoolBlkGet(&ATSAMX_DrvMemPool,
                                            ATSAMX_DrvMemPool.BlkSize,
                                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
           *p_err = USBH_ERR_HC_ALLOC;
            return;
        }
    }


    p_reg->HPIPE[pipe_nbr].PCFG      = USBH_ATSAMX_PCFG_PTYPE(ep_type);  /* Set pipe type and single bank               */
    p_reg->HPIPE[pipe_nbr].PINTENCLR = USBH_ATSAMX_PINT_ALL;    /* Disable all interrupts                               */
    p_reg->HPIPE[pipe_nbr].PINTFLAG  = USBH_ATSAMX_PINT_ALL;    /* Clear   all interrupts                               */

                                                                /*enable general error and stall interrupts             */
    p_reg->HPIPE[pipe_nbr].PINTENSET = (USBH_ATSAMX_PINT_STALL | USBH_ATSAMX_PINT_PERR);

    if ((ep_type == USBH_EP_TYPE_BULK) && (p_urb->EP_Ptr->Interval < 1u)) {
        p_reg->HPIPE[pipe_nbr].BINTERVAL = 1u;                  /* See Note 2.                                          */
    } else {
        p_reg->HPIPE[pipe_nbr].BINTERVAL = p_urb->EP_Ptr->Interval;
    }


    p_drv_data->PipeTbl[pipe_nbr].URB_Ptr = p_urb;              /* Store transaction URB                                */

    switch (ep_type) {
        case USBH_EP_TYPE_CTRL:
             if (p_urb->Token == USBH_TOKEN_SETUP) {
                 p_reg->HPIPE[pipe_nbr].PINTENSET  = USBH_ATSAMX_PINT_TXSTP;    /* Enable setup transfer interrupt      */
                 p_reg->HPIPE[pipe_nbr].PSTATUSCLR = USBH_ATSAMX_PSTATUS_DTGL;  /* Set PID to DATA0                     */
             } else {
                 p_reg->HPIPE[pipe_nbr].PINTENSET  = USBH_ATSAMX_PINT_TRCPT;    /* Enable transfer complete interrupt   */
                 p_reg->HPIPE[pipe_nbr].PSTATUSSET = USBH_ATSAMX_PSTATUS_DTGL;  /* Set PID to DATA1                     */
             }
             break;


        case USBH_EP_TYPE_INTR:
        case USBH_EP_TYPE_BULK:
             p_reg->HPIPE[pipe_nbr].PINTENSET  = USBH_ATSAMX_PINT_TRCPT;        /* Enable transfer complete interrupt   */

             if (p_urb->EP_Ptr->DataPID == 0u) {
                 p_reg->HPIPE[pipe_nbr].PSTATUSCLR = USBH_ATSAMX_PSTATUS_DTGL;  /* Set PID to DATA0                     */
             } else {
                 p_reg->HPIPE[pipe_nbr].PSTATUSSET = USBH_ATSAMX_PSTATUS_DTGL;  /* Set PID to DATA1                     */
             }
             break;


        case USBH_EP_TYPE_ISOC:
        default:
            *p_err = USBH_ERR_NOT_SUPPORTED;
             return;
    }

    USBH_ATSAMX_PipeCfg( p_urb,
                        &p_reg->HPIPE[pipe_nbr],
                        &p_drv_data->PipeTbl[pipe_nbr],
                        &p_drv_data->DescTbl[pipe_nbr].DescBank[0]);
                                                                /* Start transfer                                       */
    p_reg->HPIPE[pipe_nbr].PSTATUSCLR = USBH_ATSAMX_PSTATUS_PFREEZE;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBH_ATSAMX_HCD_URB_Complete()
*
* Description : Complete specified URB.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_urb        Pointer to URB structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           URB completed successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_URB_Complete (USBH_HC_DRV  *p_hc_drv,
                                            USBH_URB     *p_urb,
                                            USBH_ERR     *p_err)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;
    CPU_INT08U        pipe_nbr;
    CPU_INT16U        xfer_len;
    LIB_ERR           err_lib;
    CPU_SR_ALLOC();


   *p_err      =  USBH_ERR_NONE;
    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;
    pipe_nbr   =  USBH_ATSAMX_GetPipeNbr(p_drv_data, p_urb->EP_Ptr);

    CPU_CRITICAL_ENTER();
    xfer_len = USBH_ATSAMX_GET_BYTE_CNT(p_drv_data->DescTbl[pipe_nbr].DescBank[0].PCKSIZE);

    if (p_urb->Token == USBH_TOKEN_IN) {                        /* -------------- HANDLE IN TRANSACTIONS -------------- */
        Mem_Copy((void *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen),
                                      p_urb->DMA_BufPtr,
                                      xfer_len);

                                                                /* Check if it rx'd more data than what was expected    */
        if (p_drv_data->PipeTbl[pipe_nbr].AppBufLen > p_urb->UserBufLen) {
            p_urb->XferLen += xfer_len;
            p_urb->Err      = USBH_ERR_HC_IO;
        } else {
            p_urb->XferLen += xfer_len;
        }
    } else {                                                    /* ----------- HANDLE SETUP/OUT TRANSACTIONS ---------- */
        xfer_len = (p_drv_data->PipeTbl[pipe_nbr].AppBufLen - xfer_len);
        if (xfer_len == 0u) {
            p_urb->XferLen += p_drv_data->PipeTbl[pipe_nbr].NextXferLen;
        } else {
            p_urb->XferLen += (p_drv_data->PipeTbl[pipe_nbr].NextXferLen - xfer_len);
        }
    }

    p_reg->HPIPE[pipe_nbr].PINTENCLR = USBH_ATSAMX_PINT_ALL;    /* Disable all pipe interrupts                          */
    p_reg->HPIPE[pipe_nbr].PINTFLAG  = USBH_ATSAMX_PINT_ALL;    /* Clear   all pipe interrupts                          */
    p_reg->HPIPE[pipe_nbr].PCFG      = 0u;                      /* Disable the pipe                                     */
    CPU_CRITICAL_EXIT();

    Mem_PoolBlkFree(&ATSAMX_DrvMemPool,
                     p_urb->DMA_BufPtr,
                    &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_HC_ALLOC;
    }
    p_urb->DMA_BufPtr = (void *)0u;

    p_drv_data->PipeTbl[pipe_nbr].EP_Addr     = ATSAMX_DFLT_EP_ADDR;
    p_drv_data->PipeTbl[pipe_nbr].AppBufLen   = 0u;
    p_drv_data->PipeTbl[pipe_nbr].NextXferLen = 0u;
    p_drv_data->PipeTbl[pipe_nbr].URB_Ptr     = (USBH_URB *)0u;
    DEF_BIT_CLR(p_drv_data->PipeUsed, DEF_BIT(pipe_nbr));
}


/*
*********************************************************************************************************
*                                     USBH_ATSAMX_HCD_URB_Abort()
*
* Description : Abort specified URB.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_urb        Pointer to URB structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           URB aborted successfuly.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_HCD_URB_Abort (USBH_HC_DRV  *p_hc_drv,
                                         USBH_URB     *p_urb,
                                         USBH_ERR     *p_err)
{
    (void)p_hc_drv;

    p_urb->State = USBH_URB_STATE_ABORTED;
    p_urb->Err   = USBH_ERR_URB_ABORT;

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
*                                   USBH_ATSAMX_HCD_PortStatusGet()
*
* Description : Retrieve port status changes and port status.
*
* Argument(s) : p_hc_drv          Pointer to host controller driver structure.
*
*               port_nbr          Port Number.
*
*               p_port_status     Pointer to structure that will receive port status.
*
* Return(s)   : DEF_OK,           If successful.
*               DEF_FAIL,         otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortStatusGet (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr,
                                                    USBH_HUB_PORT_STATUS  *p_port_status)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;
    CPU_INT08U        reg_val;


    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;

    if (port_nbr != 1u) {
        p_port_status->wPortStatus = 0u;
        p_port_status->wPortChange = 0u;
        return (DEF_FAIL);
    }
                                                                /* Bits not used by the stack. Maintain constant value. */
    p_drv_data->RH_PortStat &= ~USBH_HUB_STATUS_PORT_TEST;
    p_drv_data->RH_PortStat &= ~USBH_HUB_STATUS_PORT_INDICATOR;

    reg_val = (p_reg->STATUS & USBH_ATSAMX_STATUS_SPEED_MSK) >> USBH_ATSAMX_STATUS_SPEED_POS;
    if (reg_val == USBH_ATSAMX_STATUS_SPEED_FS) {               /* ------------- FULL-SPEED DEVICE ATTACHED ----------- */
        p_drv_data->RH_PortStat &= ~USBH_HUB_STATUS_PORT_LOW_SPD;      /* PORT_LOW_SPEED  = 0 = FS dev attached.        */
        p_drv_data->RH_PortStat &= ~USBH_HUB_STATUS_PORT_HIGH_SPD;     /* PORT_HIGH_SPEED = 0 = FS dev attached.        */

    } else if (reg_val == USBH_ATSAMX_STATUS_SPEED_LS) {        /* ------------- LOW-SPEED DEVICE ATTACHED ------------ */
        p_drv_data->RH_PortStat |=  USBH_HUB_STATUS_PORT_LOW_SPD;      /* PORT_LOW_SPEED  = 1 = LS dev attached.        */
        p_drv_data->RH_PortStat &= ~USBH_HUB_STATUS_PORT_HIGH_SPD;     /* PORT_HIGH_SPEED = 0 = LS dev attached.        */
    }

    p_port_status->wPortStatus = p_drv_data->RH_PortStat;
    p_port_status->wPortChange = p_drv_data->RH_PortChng;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_HubDescGet()
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
* Note(s)     : (1) For more information about the hub descriptor, see 'Universal Serial Bus Specification
*                   Revision 2.0', Chapter 11.23.2.
*
*               (2) (a) 'bNbrPorts' is assigned the "number of downstream facing ports that this hub
*                       supports".
*
*                   (b) 'wHubCharacteristics' is a bit-mapped variables as follows :
*
*                       (1) Bits 0-1 (Logical Power Switching Mode) :
*                                       00b = All ports' power at once.
*                                       01b = Individual port power switching.
*                                       1xb = No power switching.
*
*                       (2) Bit 2 (Compound Device Identifier).
*
*                       (3) Bits 3-4 (Over-current Protection Mode).
*
*                       (4) Bits 5-6 (TT Think Time).
*
*                       (5) Bit  7 (Port Indicators Support).
*
*                   (c) 'bPwrOn2PwrGood' is assigned the "time (in 2 ms intervals) from the time the
*                       power-on sequence begins on a port until power is good on that port.
*
*                   (d) 'bHubContrCurrent' is assigned the "maximum current requirements of the Hub
*                       Controller electronics in mA."
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_HubDescGet (USBH_HC_DRV  *p_hc_drv,
                                                 void         *p_buf,
                                                 CPU_INT08U    buf_len)
{
    USBH_DRV_DATA  *p_drv_data;
    USBH_HUB_DESC   hub_desc;


    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;

    hub_desc.bDescLength         = USBH_HUB_LEN_HUB_DESC;
    hub_desc.bDescriptorType     = USBH_HUB_DESC_TYPE_HUB;
    hub_desc.bNbrPorts           =   1u;
    hub_desc.wHubCharacteristics =   0u;
    hub_desc.bPwrOn2PwrGood      = 100u;
    hub_desc.bHubContrCurrent    =   0u;

    USBH_HUB_FmtHubDesc(&hub_desc, p_drv_data->RH_Desc);        /* Write the structure in USB format                    */

    buf_len = DEF_MIN(buf_len, sizeof(USBH_HUB_DESC));
    Mem_Copy(p_buf, p_drv_data->RH_Desc, buf_len);              /* Copy the formatted structure into the buffer         */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_ATSAMX_HCD_PortEnSet()
*
* Description : Enable given port.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortEnSet (USBH_HC_DRV  *p_hc_drv,
                                                CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_ATSAMX_HCD_PortEnClr()
*
* Description : Clear port enable status.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortEnClr (USBH_HC_DRV  *p_hc_drv,
                                                CPU_INT08U    port_nbr)
{
    USBH_DRV_DATA  *p_drv_data;


    (void)port_nbr;

    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;

    p_drv_data->RH_PortStat &= ~USBH_HUB_STATUS_PORT_EN;        /* Bit is clr by ClearPortFeature(PORT_ENABLE)          */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_ATSAMX_HCD_PortEnChngClr()
*
* Description : Clear port enable status change.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortEnChngClr (USBH_HC_DRV  *p_hc_drv,
                                                    CPU_INT08U    port_nbr)
{
    USBH_DRV_DATA  *p_drv_data;


    (void)port_nbr;

    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;

    p_drv_data->RH_PortChng &= ~USBH_HUB_STATUS_C_PORT_EN;      /* Bit is clr by ClearPortFeature(C_PORT_ENABLE)        */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_PortPwrSet()
*
* Description : Set port power based on port power mode.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortPwrSet (USBH_HC_DRV  *p_hc_drv,
                                                 CPU_INT08U    port_nbr)
{
    USBH_DRV_DATA  *p_drv_data;


    (void)port_nbr;

    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;

    p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_PWR;        /* Bit is set by SetPortFeature(PORT_POWER) request     */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_PortPwrClr()
*
* Description : Clear port power.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortPwrClr (USBH_HC_DRV  *p_hc_drv,
                                                 CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_ATSAMX_HCD_PortResetSet()
*
* Description : Reset given port.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortResetSet (USBH_HC_DRV  *p_hc_drv,
                                                   CPU_INT08U    port_nbr)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;

   (void)port_nbr;

    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;

    p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_RESET;      /* This bit is set while in the resetting state         */
    p_reg->CTRLB            |= USBH_ATSAMX_CTRLB_BUSRESET;      /* Send USB reset signal.                               */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                 USBH_ATSAMX_HCD_PortResetChngClr()
*
* Description : Clear port reset status change.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortResetChngClr (USBH_HC_DRV  *p_hc_drv,
                                                       CPU_INT08U    port_nbr)
{
    USBH_DRV_DATA  *p_drv_data;


    (void)port_nbr;

    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;

    p_drv_data->RH_PortChng &= ~USBH_HUB_STATUS_C_PORT_RESET;   /* Bit is clr by ClearPortFeature(C_PORT_RESET) request */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  USBH_ATSAMX_HCD_PortSuspendClr()
*
* Description : Resume given port if port is suspended.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortSuspendClr (USBH_HC_DRV  *p_hc_drv,
                                                     CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  USBH_ATSAMX_HCD_PortConnChngClr()
*
* Description : Clear port connect status change.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_PortConnChngClr (USBH_HC_DRV  *p_hc_drv,
                                                      CPU_INT08U    port_nbr)
{
    USBH_DRV_DATA  *p_drv_data;

    (void)port_nbr;

    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    DEF_BIT_CLR(p_drv_data->RH_PortChng, USBH_HUB_STATUS_C_PORT_CONN);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_RHSC_IntEn()
*
* Description : Enable root hub interrupt.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_RHSC_IntEn (USBH_HC_DRV  *p_hc_drv)
{
    (void)p_hc_drv;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_ATSAMX_HCD_RHSC_IntDis()
*
* Description : Disable root hub interrupt.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,      If successful.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_ATSAMX_HCD_RHSC_IntDis (USBH_HC_DRV  *p_hc_drv)
{
    (void)p_hc_drv;

    return (DEF_OK);
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
*                                      USBH_ATSAMX_ISR_Handler()
*
* Description : ISR handler.
*
* Arguments   : p_drv     Pointer to host controller driver structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_ISR_Handler (void  *p_drv)
{
    USBH_ATSAMX_REG  *p_reg;
    USBH_DRV_DATA    *p_drv_data;
    USBH_HC_DRV      *p_hc_drv;
    CPU_INT16U        int_stat;
    CPU_INT16U        pipe_stat;
    CPU_INT16U        pipe_nbr;
    CPU_INT16U        xfer_len;
    CPU_INT16U        max_pkt_size;
    USBH_URB         *p_urb;


    p_hc_drv    = (USBH_HC_DRV     *)p_drv;
    p_reg       = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data  = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;
    int_stat    =  p_reg->INTFLAG;
    int_stat   &=  p_reg->INTENSET;

                                                                /* ----------- HANDLE INTERRUPT FLAG STATUS ----------- */
    if (int_stat & USBH_ATSAMX_INT_RST) {
        p_reg->INTFLAG = USBH_ATSAMX_INT_RST;                   /* Clear BUS RESET interrupt flag                       */

        p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_EN;     /* Bit may be set due to SetPortFeature(PORT_RESET)     */
        p_drv_data->RH_PortChng |= USBH_HUB_STATUS_C_PORT_RESET;/* Transitioned from RESET state to ENABLED State       */

        USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);                 /* Notify the core layer.                               */

    } else if (int_stat & USBH_ATSAMX_INT_DDISC) {
                                                                /* Clear device disconnect/connect interrupt flags      */
        p_reg->INTFLAG  = (USBH_ATSAMX_INT_DDISC | USBH_ATSAMX_INT_DCONN);
        p_reg->INTENCLR = (USBH_ATSAMX_INT_DDISC | USBH_ATSAMX_INT_WAKEUP);  /* Disable disconnect/wakeup interrupt     */

                                                                /* Clear device connect/wakeup interrupt flags          */
        p_reg->INTFLAG  = (USBH_ATSAMX_INT_DCONN | USBH_ATSAMX_INT_WAKEUP);
        p_reg->INTENSET = (USBH_ATSAMX_INT_DCONN | USBH_ATSAMX_INT_WAKEUP);  /* Enable connect/wakeup interrupt         */

        p_drv_data->RH_PortStat  = 0u;                          /* Clear Root hub Port Status bits                      */
        p_drv_data->RH_PortChng |= USBH_HUB_STATUS_C_PORT_CONN; /* Current connect status has changed                   */
        USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);                 /* Notify the core layer.                               */

    } else if (int_stat & USBH_ATSAMX_INT_DCONN) {
        p_reg->INTENCLR = USBH_ATSAMX_INT_DCONN;                /* Disable device connect interrupt                     */

        p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_CONN;   /* Bit reflects if a device is currently connected      */
        p_drv_data->RH_PortChng |= USBH_HUB_STATUS_C_PORT_CONN; /* Bit indicates a Port's current connect status change */

        p_reg->INTFLAG  = USBH_ATSAMX_INT_DDISC;                /* Clear  disconnection interrupt                       */
        p_reg->INTENSET = USBH_ATSAMX_INT_DDISC;                /* Enable disconnection interrupt                       */

        USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);                 /* Notify the core layer.                               */
    }

                                                                /* ----------------- WAKE UP TO POWER ----------------- */
    if (int_stat & (USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_DCONN)) {
        p_reg->INTFLAG = USBH_ATSAMX_INT_WAKEUP;                /* Clear WAKEUP interrupt flag                          */
    }

                                                                /* ---------------------- RESUME ---------------------- */
    if (int_stat & (USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_UPRSM | USBH_ATSAMX_INT_DNRSM)) {
                                                                /* Clear interrupt flag                                 */
        p_reg->INTFLAG  = (USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_UPRSM | USBH_ATSAMX_INT_DNRSM);
                                                                /* Disable interrupts                                   */
        p_reg->INTENCLR = (USBH_ATSAMX_INT_WAKEUP | USBH_ATSAMX_INT_UPRSM | USBH_ATSAMX_INT_DNRSM);
        p_reg->INTENSET = (USBH_ATSAMX_INT_RST | USBH_ATSAMX_INT_DDISC);

        p_reg->CTRLB |= USBH_ATSAMX_CTRLB_SOFE;                 /* enable start of frames */
    }

                                                                /* ----------- HANDLE PIPE INTERRUPT STATUS ----------- */
    pipe_stat = p_reg->PINTSMRY;                                /* Read Pipe interrupt summary                          */
    while (pipe_stat != 0u) {                                   /* Check if there is a pipe to handle                   */
        pipe_nbr  = CPU_CntTrailZeros(pipe_stat);
        int_stat  = p_reg->HPIPE[pipe_nbr].PINTFLAG;
        int_stat &= p_reg->HPIPE[pipe_nbr].PINTENSET;
        p_urb     = p_drv_data->PipeTbl[pipe_nbr].URB_Ptr;
        xfer_len  = USBH_ATSAMX_GET_BYTE_CNT(p_drv_data->DescTbl[pipe_nbr].DescBank[0].PCKSIZE);

        if (int_stat & USBH_ATSAMX_PINT_STALL) {
            p_reg->HPIPE[pipe_nbr].PSTATUSSET = USBH_ATSAMX_PSTATUS_PFREEZE;  /* Stop transfer                          */
            p_reg->HPIPE[pipe_nbr].PINTFLAG   = USBH_ATSAMX_PINT_STALL;       /* Clear Stall interrupt flag             */
            p_urb->Err                        = USBH_ERR_EP_STALL;
            USBH_URB_Done(p_urb);                               /* Notify the Core layer about the URB completion       */
        }

        if (int_stat & USBH_ATSAMX_PINT_PERR) {
            p_reg->HPIPE[pipe_nbr].PSTATUSSET = USBH_ATSAMX_PSTATUS_PFREEZE;  /* Stop transfer                          */
            p_reg->HPIPE[pipe_nbr].PINTFLAG   = USBH_ATSAMX_PINT_PERR;        /* Clear Pipe error interrupt flag        */
            p_urb->Err                        = USBH_ERR_HC_IO;
            USBH_URB_Done(p_urb);                               /* Notify the Core layer about the URB completion       */
        }

        if (int_stat & USBH_ATSAMX_PINT_TXSTP) {                /* --------------- HANDLE SETUP PACKETS --------------- */
            p_reg->HPIPE[pipe_nbr].PINTENCLR = USBH_ATSAMX_PINT_TXSTP; /* Disable Setup transfer interrupt              */
            p_reg->HPIPE[pipe_nbr].PINTFLAG  = USBH_ATSAMX_PINT_TXSTP; /* Clear   Setup transfer flag                   */

            xfer_len   = p_drv_data->PipeTbl[pipe_nbr].AppBufLen + p_urb->XferLen;
            p_urb->Err = USBH_ERR_NONE;

            USBH_URB_Done(p_urb);                               /* Notify the Core layer about the URB completion       */
        }

        if (int_stat & USBH_ATSAMX_PINT_TRCPT) {
            p_reg->HPIPE[pipe_nbr].PINTENCLR = USBH_ATSAMX_PINT_TRCPT; /* Disable transfer complete interrupt           */
            p_reg->HPIPE[pipe_nbr].PINTFLAG  = USBH_ATSAMX_PINT_TRCPT; /* Clear   transfer complete flag                */

                                                                /* Keep track of PID DATA toggle                        */
            p_urb->EP_Ptr->DataPID = USBH_ATSAMX_GET_DPID(p_reg->HPIPE[pipe_nbr].PSTATUS);

            if (p_urb->Token == USBH_TOKEN_IN) {                /* ---------------- IN PACKETS HANDLER ---------------- */
                max_pkt_size                             = USBH_EP_MaxPktSizeGet(p_urb->EP_Ptr);
                p_drv_data->PipeTbl[pipe_nbr].AppBufLen += xfer_len;

                if ((p_drv_data->PipeTbl[pipe_nbr].AppBufLen == p_urb->UserBufLen) ||
                    (xfer_len                                <  max_pkt_size     )) {
                    p_urb->Err = USBH_ERR_NONE;
                    USBH_URB_Done(p_urb);                       /* Notify the Core layer about the URB completion       */

                } else {
                    p_urb->Err = USBH_OS_MsgQueuePut(        ATSAMX_URB_Proc_Q,
                                                     (void *)p_urb);
                    if (p_urb->Err != USBH_ERR_NONE) {
                        USBH_URB_Done(p_urb);                   /* Notify the Core layer about the URB completion       */
                    }
                }

            } else {                                            /* ---------------- OUT PACKETS HANDLER --------------- */
                xfer_len = p_drv_data->PipeTbl[pipe_nbr].AppBufLen + p_urb->XferLen;

                if (xfer_len == p_urb->UserBufLen) {
                    p_urb->Err = USBH_ERR_NONE;
                    USBH_URB_Done(p_urb);                       /* Notify the Core layer about the URB completion       */

                } else {
                    p_urb->Err = USBH_OS_MsgQueuePut(        ATSAMX_URB_Proc_Q,
                                                     (void *)p_urb);
                    if (p_urb->Err != USBH_ERR_NONE) {
                        USBH_URB_Done(p_urb);                   /* Notify the Core layer about the URB completion       */
                    }
                }
            }
        }

        DEF_BIT_CLR(pipe_stat, DEF_BIT(pipe_nbr));
    }
}


/*
*********************************************************************************************************
*                                     USBH_ATSAMX_URB_ProcTask()
*
* Description : The task handles additional EP IN/OUT transactions when needed.
*
* Argument(s) : p_arg     Pointer to the argument passed to 'USBH_ATSAMX_URB_ProcTask()' by
*                         'USBH_OS_TaskCreate()'.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void  USBH_ATSAMX_URB_ProcTask (void  *p_arg)
{
    USBH_HC_DRV      *p_hc_drv;
    USBH_DRV_DATA    *p_drv_data;
    USBH_ATSAMX_REG  *p_reg;
    USBH_URB         *p_urb;
    CPU_INT32U        xfer_len;
    CPU_INT08U        pipe_nbr;
    USBH_ERR          p_err;
    CPU_SR_ALLOC();

    p_hc_drv   = (USBH_HC_DRV     *)p_arg;
    p_drv_data = (USBH_DRV_DATA   *)p_hc_drv->DataPtr;
    p_reg      = (USBH_ATSAMX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    while (DEF_TRUE) {
        p_urb = (USBH_URB *)USBH_OS_MsgQueueGet( ATSAMX_URB_Proc_Q,
                                                 0u,
                                                &p_err);
        if (p_err != USBH_ERR_NONE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("DRV: Could not get URB from Queue.\r\n");
#endif
        }

        pipe_nbr = USBH_ATSAMX_GetPipeNbr(p_drv_data, p_urb->EP_Ptr);

        if (pipe_nbr != ATSAMX_INVALID_PIPE) {
            CPU_CRITICAL_ENTER();
            xfer_len = USBH_ATSAMX_GET_BYTE_CNT(p_drv_data->DescTbl[pipe_nbr].DescBank[0].PCKSIZE);

            if (p_urb->Token == USBH_TOKEN_IN) {                /* -------------- HANDLE IN TRANSACTIONS -------------- */
                Mem_Copy((void *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen),
                                              p_urb->DMA_BufPtr,
                                              xfer_len);
                                                                /* Check if it rx'd more data than what was expected    */
                if (xfer_len > p_drv_data->PipeTbl[pipe_nbr].NextXferLen) {  /* Rx'd more data than what was expected   */
                    p_urb->XferLen += p_drv_data->PipeTbl[pipe_nbr].NextXferLen;
                    p_urb->Err      = USBH_ERR_HC_IO;
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                    USBH_PRINT_LOG("DRV: Rx'd more data than was expected.\r\n");
#endif
                    USBH_URB_Done(p_urb);                       /* Notify the Core layer about the URB completion       */

                } else {
                    p_urb->XferLen += xfer_len;
                }

            } else {                                            /* ------------- HANDLE OUT TRANSACTIONS -------------- */
                xfer_len = (p_drv_data->PipeTbl[pipe_nbr].AppBufLen - xfer_len);
                if (xfer_len == 0u) {
                    p_urb->XferLen += p_drv_data->PipeTbl[pipe_nbr].NextXferLen;
                } else {
                    p_urb->XferLen += (p_drv_data->PipeTbl[pipe_nbr].NextXferLen - xfer_len);
                }
            }

            if (p_urb->Err == USBH_ERR_NONE) {
                USBH_ATSAMX_PipeCfg( p_urb,
                               &p_reg->HPIPE[pipe_nbr],
                               &p_drv_data->PipeTbl[pipe_nbr],
                               &p_drv_data->DescTbl[pipe_nbr].DescBank[0]);

                p_reg->HPIPE[pipe_nbr].PINTENSET  = USBH_ATSAMX_PINT_TRCPT;       /* Enable transfer complete interrupt */
                p_reg->HPIPE[pipe_nbr].PSTATUSCLR = USBH_ATSAMX_PSTATUS_PFREEZE;  /* Start  transfer                    */
            }
            CPU_CRITICAL_EXIT();
        }
    }
}


/*
*********************************************************************************************************
*                                        USBH_ATSAMX_PipeCfg()
*
* Description : Initialize pipe configurations based on the endpoint direction and characteristics.
*
* Argument(s) : p_reg           Pointer to ATSAMX Pipe register structure.
*
*               p_urb           Pointer to URB structure.
*
*               p_pipe_info     Pointer to pipe information.
*
*               p_desc_bank     Pointer to Host pipe descriptor ban.
*
* Return(s)   : None.
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  USBH_ATSAMX_PipeCfg (USBH_URB               *p_urb,
                                   USBH_ATSAMX_PIPE_REG   *p_reg_hpipe,
                                   USBH_ATSAMX_PINFO      *p_pipe_info,
                                   USBH_ATSAMX_DESC_BANK  *p_desc_bank)
{
    CPU_INT08U  ep_nbr;
    CPU_INT08U  reg_val;
    CPU_INT16U  max_pkt_size;
    CPU_SR_ALLOC();


    max_pkt_size             = USBH_EP_MaxPktSizeGet(p_urb->EP_Ptr);
    ep_nbr                   = USBH_EP_LogNbrGet(p_urb->EP_Ptr);
    p_pipe_info->NextXferLen = p_urb->UserBufLen - p_urb->XferLen;
    p_pipe_info->NextXferLen = DEF_MIN(p_pipe_info->NextXferLen, ATSAMX_DrvMemPool.BlkSize);

    Mem_Clr(p_urb->DMA_BufPtr, p_pipe_info->NextXferLen);
    if (p_urb->Token != USBH_TOKEN_IN) {                        /* ---------------- SETUP/OUT PACKETS ----------------- */
        p_pipe_info->AppBufLen = p_pipe_info->NextXferLen;

        Mem_Copy(                     p_urb->DMA_BufPtr,
                 (void *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen),
                                      p_pipe_info->NextXferLen);

        p_desc_bank->PCKSIZE = p_pipe_info->AppBufLen;

    } else {                                                    /* -------------------- IN PACKETS -------------------- */
        p_desc_bank->PCKSIZE = (p_pipe_info->NextXferLen << 14u);
    }

    p_desc_bank->ADDR      = (CPU_INT32U)p_urb->DMA_BufPtr;
    p_desc_bank->PCKSIZE  |= (CPU_CntTrailZeros(max_pkt_size >> 3u) << 28u);
    p_desc_bank->CTRL_PIPE = (p_urb->EP_Ptr->DevAddr |  (ep_nbr << 8u));

    if (p_urb->Token != USBH_TOKEN_IN) {                        /* ---------------- SETUP/OUT PACKETS ----------------- */
        CPU_CRITICAL_ENTER();
        reg_val            =  p_reg_hpipe->PCFG;
        reg_val           &= ~USBH_ATSAMX_PCFG_PTOKEN_MSK;
        reg_val           |= (p_urb->Token ? USBH_ATSAMX_PCFG_PTOKEN_OUT : USBH_ATSAMX_PCFG_PTOKEN_SETUP);
        p_reg_hpipe->PCFG  =  reg_val;
        CPU_CRITICAL_EXIT();

        p_reg_hpipe->PSTATUSSET = USBH_ATSAMX_PSTATUS_BK0RDY;   /* Set Bank0 ready : Pipe can send data to device       */

    } else {                                                    /* -------------------- IN PACKETS -------------------- */
        CPU_CRITICAL_ENTER();
        reg_val           =  p_reg_hpipe->PCFG;
        reg_val          &= ~USBH_ATSAMX_PCFG_PTOKEN_MSK;
        reg_val          |=  USBH_ATSAMX_PCFG_PTOKEN_IN;
        p_reg_hpipe->PCFG =  reg_val;
        CPU_CRITICAL_EXIT();

        p_reg_hpipe->PSTATUSCLR = USBH_ATSAMX_PSTATUS_BK0RDY;   /* Clear Bank0 ready: Pipe can receive data from device */
    }
}


/*
*********************************************************************************************************
*                                      USBH_ATSAMX_GetFreePipe()
*
* Description : Allocate a free host pipe number for the newly opened pipe.
*
* Argument(s) : p_drv_data     Pointer to host driver data structure.
*
* Return(s)   : Free pipe number or 0xFF if no host pipe is found
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  USBH_ATSAMX_GetFreePipe (USBH_DRV_DATA  *p_drv_data)
{
    CPU_INT08U  pipe_nbr;


    for (pipe_nbr = 0u; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
        if (DEF_BIT_IS_CLR(p_drv_data->PipeUsed, DEF_BIT(pipe_nbr))) {
            DEF_BIT_SET(p_drv_data->PipeUsed, DEF_BIT(pipe_nbr));
            return (pipe_nbr);
        }
    }

    return ATSAMX_INVALID_PIPE;
}


/*
*********************************************************************************************************
*                                      USBH_ATSAMX_GetPipeNbr()
*
* Description : Get the host pipe number corresponding to a given device address, endpoint number
*               and direction.
*
* Argument(s) : p_drv_data     Pointer to host driver data structure.
*
*               p_ep           Pointer to endpoint structure.
*
* Return(s)   : Host pipe number or 0xFF if no host pipe is found
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  USBH_ATSAMX_GetPipeNbr (USBH_DRV_DATA  *p_drv_data,
                                            USBH_EP        *p_ep)
{
    CPU_INT08U  pipe_nbr;
    CPU_INT16U  ep_addr;


    ep_addr = ((p_ep->DevAddr << 8u)  |
                p_ep->Desc.bEndpointAddress);

    for (pipe_nbr = 0u; pipe_nbr < ATSAMX_MAX_NBR_PIPE; pipe_nbr++) {
        if (p_drv_data->PipeTbl[pipe_nbr].EP_Addr == ep_addr) {
            return (pipe_nbr);
        }
    }

    return ATSAMX_INVALID_PIPE;
}

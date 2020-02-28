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
*                                  STM32FX_FS HOST CONTROLLER DRIVER
*
* Filename : usbh_hcd_stm32fx_fs.c
* Version  : V3.42.00
*********************************************************************************************************
* Note(s)  : (1) With an appropriate BSP, this host driver will support the OTG_FS host module on
*                the STMicroelectronics STM32F10xxx, STM32F2xx, STM32F4xx, and STM32F7xx MCUs, this
*                applies to the following:
*
*                    STM32F105xx series.
*                    STM32F107xx series.
*                    STM32F205xx series.
*                    STM32F207xx series.
*                    STM32F215xx series.
*                    STM32F217xx series.
*                    STM32F405xx series.
*                    STM32F407xx series.
*                    STM32F415xx series.
*                    STM32F417xx series.
*                    STM32F74xxx series.
*                    STM32F75xxx series.
*
*            (3) With an appropriate BSP, this device driver will support the OTG_FS device module on
*                the Silicon Labs EFM32 MCUs, this applies to the following devices:
*                    EFM32 Giant   Gecko series.
*                    EFM32 Wonder  Gecko series.
*                    EFM32 Leopard Gecko series.
*
*            (4) This device driver DOES NOT support the USB on-the-go high-speed(OTG_HS) device module
*                of the STM32F2xx, STM32F4xx, STM32F7xx, or EFM32 Gecko series.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define  USBH_HCD_STM32FX_FS_MODULE
#include  "usbh_hcd_stm32fx_fs.h"
#include  "Source/usbh_hub.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  REG_GAHBCFG_GINTMSK                       DEF_BIT_00   /* Global interrupt mask.                               */

#define  REG_GAHBCFG_TXFELVL                       DEF_BIT_07   /* Non-periodic TxFIFO empty level.                     */
#define  REG_GAHBCFG_PTXFELVL                      DEF_BIT_08   /* Periodic TxFIFO empty level.                         */

#define  REG_GINTx_CMOD                            DEF_BIT_00   /* Current mode of operation interrupt.                 */
#define  REG_GINTx_OTGINT                          DEF_BIT_02   /* OTG interrupt.                                       */
#define  REG_GINTx_SOF                             DEF_BIT_03   /* Start of frame interrupt.                            */
#define  REG_GINTx_RXFLVL                          DEF_BIT_04   /* RxFIFO non-empty interrupt.                          */
#define  REG_GINTx_NPTXFE                          DEF_BIT_05   /* Non-periodic TxFIFO empty interrupt.                 */
#define  REG_GINTx_IPXFR                           DEF_BIT_21   /* Incomplete periodic transfer interrupt.              */
#define  REG_GINTx_HPRTINT                         DEF_BIT_24   /* Host port interrupt.                                 */
#define  REG_GINTx_HCINT                           DEF_BIT_25   /* Host Channel interrupt.                              */
#define  REG_GINTx_PTXFE                           DEF_BIT_26   /* Periodic TxFIFO empty interrupt.                     */
#define  REG_GINTx_CIDSCHGM                        DEF_BIT_28   /* Connector ID status change mask.                     */
#define  REG_GINTx_DISCINT                         DEF_BIT_29   /* Disconnect detected interrupt.                       */
#define  REG_GINTx_SRQINT                          DEF_BIT_30   /* Session request/new session detected interrupt mask. */
#define  REG_GINTx_WKUPINT                         DEF_BIT_31   /* Resume/remote wakeup detected interrupt mask.        */

#define  REG_GRSTCTL_CORE_SW_RST                   DEF_BIT_00   /* Core soft reset.                                     */
#define  REG_GRSTCTL_HCLK_SW_RST                   DEF_BIT_01   /* HCLK soft reset.                                     */
#define  REG_GRSTCTL_RXFFLUSH                      DEF_BIT_04   /* RxFIFO flush.                                        */
#define  REG_GRSTCTL_TXFFLUSH                      DEF_BIT_05   /* TxFIFO flush.                                        */
#define  REG_GRSTCTL_AHBIDL                        DEF_BIT_31   /* AHB master idle.                                     */

#define  REG_GRXSTS_PKTSTS_IN                      2u           /* IN data packet received.                             */
#define  REG_GRXSTS_PKTSTS_IN_XFER_COMP            3u           /* IN transfer completed(triggers an interrupt).        */
#define  REG_GRXSTS_PKTSTS_DATA_TOGGLE_ERR         5u           /* Data toggle error(triggers an interrupt).            */
#define  REG_GRXSTS_PKTSTS_CH_HALTED               7u           /* Channel halted(triggers an interrupt).               */

#define  REG_GCCFG_PWRDWN                          DEF_BIT_16   /* Power down.                                          */
#define  REG_GCCFG_VBUSASEN                        DEF_BIT_18   /* Enable the VBUS sensing A Device.                    */
#define  REG_GCCFG_VBUSBSEN                        DEF_BIT_19   /* Enable the VBUS sensing B Device.                    */
#define  REG_GCCFG_SOFOUTEN                        DEF_BIT_20   /* SOF output enable.                                   */
#define  REG_GCCFG_VBDEN                           DEF_BIT_21   /* This bit is use for STM32F74xx & STM32F75xx driver.  */
#define  REG_GCCFG_NONVBUSSENS                     DEF_BIT_21   /* This bit is use for STM32F107, STM32F2 & STM32F4.    */

#define  REG_GUSBCFG_PHYSEL                        DEF_BIT_06   /* Full speed serial transceiver select.                */
#define  REG_GUSBCFG_SRPCAP                        DEF_BIT_08   /* SRP-capable.                                         */
#define  REG_GUSBCFG_HNPCAP                        DEF_BIT_09   /* HNP-capable.                                         */
#define  REG_GUSBCFG_FORCE_HOST                    DEF_BIT_29   /* Force host mode.                                     */

#define  REG_HCFG_FS_LS_PHYCLK_SEL_48MHz           0x000000001u /* Select 48MHz PHY clock frequency.                    */
#define  REG_HCFG_FS_LS_PHYCLK_SEL_6MHz            0x000000002u /* Select  6MHz PHY clock frequency.                    */
#define  REG_HCFG_FS_LS_SUPPORT                    DEF_BIT_02   /* FS- and LS-only support.                             */
#define  REG_HCFG_FS_LS_CLK_SEL                   (DEF_BIT_01 | DEF_BIT_00)

#define  REG_HPTXSTS_REQTYPE                      (DEF_BIT_26 | DEF_BIT_25)
#define  REG_HPTXSTS_CH_EP_NBR                    (DEF_BIT_30 | DEF_BIT_29 | DEF_BIT_28 | DEF_BIT_27)
#define  REG_HPTXSTS_REQTYPE_IN_OUT                0u
#define  REG_HPTXSTS_REQTYPE_ZLP                   1u
#define  REG_HPTXSTS_REQTYPE_DIS_CH_CMD            3u

#define  REG_HCCHARx_EPDIR                         DEF_BIT_15   /* Endpoint direction.                                  */
#define  REG_HCCHARx_LOW_SPEED_DEV                 DEF_BIT_17   /* Low speed device.                                    */
#define  REG_HCCHARx_MC_EC                        (DEF_BIT_21 | DEF_BIT_20)
#define  REG_HCCHARx_ODDFRM                        DEF_BIT_29   /* Odd frame.                                           */
#define  REG_HCCHARx_CHDIS                         DEF_BIT_30   /* Channel disable.                                     */
#define  REG_HCCHARx_CHENA                         DEF_BIT_31   /* Channel enable.                                      */
#define  REG_HCCHARx_1_TRANSACTION                 1u           /* 1 transaction.                                       */
#define  REG_HCCHARx_2_TRANSACTION                 2u           /* 2 transaction per frame to be issued.                */
#define  REG_HCCHARx_3_TRANSACTION                 3u           /* 3 transaction per frame to be issued.                */
#define  REG_HCCHARx_EP_TYPE_MSK                   0x000C0000u  /* bits [19:18] Endpoint Type                           */
#define  REG_HCCHARx_EP_NBR_MSK                    0x00007800u  /* bits [14:11} Endpoint Number                         */

#define  REG_HCINTx_XFRC                           DEF_BIT_00   /* transfer complete interrupt.                         */
#define  REG_HCINTx_CH_HALTED                      DEF_BIT_01   /* Channel halted interrupt.                            */
#define  REG_HCINTx_STALL                          DEF_BIT_03   /* STALL response received interrupt.                   */
#define  REG_HCINTx_NAK                            DEF_BIT_04   /* NAK response received interrupt.                     */
#define  REG_HCINTx_ACK                            DEF_BIT_05   /* ACK response received/transmitted interrupt.         */
#define  REG_HCINTx_TXERR                          DEF_BIT_07   /* Transaction error interrupt.                         */
#define  REG_HCINTx_BBERR                          DEF_BIT_08   /* Babble error interrupt.                              */
#define  REG_HCINTx_DTERR                          DEF_BIT_10   /* Data toggle error interrupt.                         */
#define  REG_HCINTx_HALT_SRC_NONE                  0u
#define  REG_HCINTx_HALT_SRC_XFER_CMPL             1u
#define  REG_HCINTx_HALT_SRC_STALL                 2u
#define  REG_HCINTx_HALT_SRC_NAK                   3u
#define  REG_HCINTx_HALT_SRC_TXERR                 4u
#define  REG_HCINTx_HALT_SRC_BBERR                 5u
#define  REG_HCINTx_HALT_SRC_FRMOR                 6u
#define  REG_HCINTx_HALT_SRC_DTERR                 7u
#define  REG_HCINTx_HALT_SRC_ABORT                 8u

#define  REG_PID_DATA0                             0u
#define  REG_PID_DATA1                             2u
#define  REG_PID_DATA2                             1u
#define  REG_PID_SETUP_MDATA                       3u
#define  REG_PID_NONE                              0xFFu
#define  REG_HCTSIZx_PID_MSK                       0x60000000u  /* bits [30:29] PID                                     */
#define  REG_HCTSIZx_PKTCNT_MSK                    0x1FF80000u  /* bits [28:19] Packet Count                            */
#define  REG_HCTSIZx_XFRSIZ_MSK                    0x0007FFFFu  /* bits [18:0] Transfer size                            */
#define  REG_HCTSIZx_XFRSIZ_MAX                    1023u        /* Max transfer Transfer size                           */

                                                                /* ------------- HPRT REGISTER BITS VALUE ------------- */
#define  REG_HPRT_PORT_SPD_HS                      0x00000000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  REG_HPRT_PORT_SPD_FS                      0x00020000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  REG_HPRT_PORT_SPD_LS                      0x00040000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  REG_HPRT_PORT_SPD_NO_SPEED                0x00060000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  REG_HPRT_PORT_NOT_IN_RESET                0u           /* Port Reset (PrtSpd): bit 8                           */
#define  REG_HPRT_PORT_IN_RESET                    1u           /* Port Reset (PrtSpd): bit 8                           */
#define  REG_HPRT_PORT_NOT_IN_SUSPEND_MODE         0u           /* Port Suspend (PrtSusp): bit 7                        */
#define  REG_HPRT_PORT_IN_SUSPEND_MODE             1u           /* Port Suspend (PrtSusp): bit 7                        */
#define  REG_HPRT_PORT_CONN_STS                    DEF_BIT_00   /* Port connect status.                                 */
#define  REG_HPRT_PORT_CONN_DET                    DEF_BIT_01   /* Port connect detected.                               */
#define  REG_HPRT_PORT_EN                          DEF_BIT_02   /* Port enable.                                         */
#define  REG_HPRT_PORT_EN_CHNG                     DEF_BIT_03   /* Port enable/disable change.                          */
#define  REG_HPRT_PORT_OCCHNG                      DEF_BIT_05   /* Port overcurrent change.                             */
#define  REG_HPRT_PORT_SUSP                        DEF_BIT_07   /* Port suspend.                                        */
#define  REG_HPRT_PORT_RST                         DEF_BIT_08   /* Port reset.                                          */
#define  REG_HPRT_PORT_PWR                         DEF_BIT_12   /* Port power.                                          */
#define  REG_HPRT_PORT_SPEED                      (DEF_BIT_17 | DEF_BIT_18)

                                                                /* ----------- GRSTCTL REGISTER BITS VALUE ------------ */
#define  REG_GRSTCTL_NONPER_TXFIFO                 0x00u
#define  REG_GRSTCTL_PER_TXFIFO                    0x01u
#define  REG_GRSTCTL_ALL_TXFIFO                    0x10u


/*
**************************************************************************************************************
*                                                 DEFINES
*
*   Notes:  (1) Allocation of data RAM for Endpoint FIFOs. The OTG-FS controller has a dedicated SPRAM for
*               EP data of 1.25 Kbytes = 1280 bytes = 320 32-bits words available for the endpoints IN and OUT.
*               Host mode features:
*               - 8 host channels (pipes)
*               - 1 shared RX FIFO
*               - 1 periodic TX FIFO
*               - 1 nonperiodic TX FIFO
*
*           (2) Receive data FIFO size = Status information + data IN channel + transfer complete status
*               information Space = ONE 32-bits words
*               (a) Status information    = 1 space
*                   (one space for status information written to the FIFO along with each received packet)
*               (b) data IN channel       = (Largest Packet Size / 4) + 1 spaces
*                   (MINIMUM to receive packets)
*               (c) OR data IN channel    = at least 2*(Largest Packet Size / 4) + 1 spaces
*                   (if high-bandwidth channel is enabled or multiple isochronous channels)
*               (d) transfer complete status information = 1 space per OUT EP
*                   (one space for transfer complete status information also pushed to the FIFO
*                   with each channel's last packet)
*
*           (3) Non-periodic Transmit FIFO RAM allocation:
*               - MIN RAM space = largest max packet size among all supported non-periodic OUT channels.
*               - RECOMMENDED   = two largest packet sizes
*
*           (4) Periodic Transmit FIFO RAM allocation:
*               - MIN RAM space = largest max packet size among all supported periodic OUT channels.
*               - if one high bandwidth isochronous OUT channel, 2 * max packet size of that channel
**************************************************************************************************************
*/

#define  DRV_STM32FX_FS                                    0u
#define  DRV_STM32FX_OTG_FS                                1u
#define  DRV_EFM32_OTG_FS                                  2u

#define  STM32FX_FS_MAX_NBR_CH                             8u   /* Maximum number of host channels                      */
#define  STM32FX_OTG_FS_MAX_NBR_CH                        12u   /* Maximum number of host channels                      */
#define  STM32FX_DFIFO_SIZE                             1024u   /* Number of entries                                    */


#define  STM32FX_MAX_RETRY                             10000u   /* Maximum number of retries                            */

                                                                /* See Note #1. FIFO depth is specified in 32-bit words */
#define  STM32FX_FS_RXFIFO_START_ADDR                      0u
#define  STM32FX_FS_RXFIFO_DEPTH                          64u   /* For all IN channels. See Note #2                     */
#define  STM32FX_FS_NONPER_CH_TXFIFO_DEPTH               128u   /* For all Non-Periodic channels. See Note #3           */
#define  STM32FX_FS_NONPER_CH_TXFIFO_START_ADDR   (STM32FX_FS_RXFIFO_DEPTH)
#define  STM32FX_FS_PER_CH_TXFIFO_DEPTH                  128u   /* For all Periodic channels. See Note #4               */
#define  STM32FX_FS_PER_CH_TXFIFO_START_ADDR      (STM32FX_FS_NONPER_CH_TXFIFO_DEPTH +     \
                                                   STM32FX_FS_NONPER_CH_TXFIFO_START_ADDR)

/*
*********************************************************************************************************
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  struct  usbh_stm32fx_host_ch_reg {                     /* ------------- HOST CHANNEL-X DATA TYPE ------------- */
    CPU_REG32                 HCCHARx;                          /* Host channel-x characteristics                       */
    CPU_REG32                 HCSPLTx;                          /* Host channel-x split control                         */
    CPU_REG32                 HCINTx;                           /* Host channel-x interrupt                             */
    CPU_REG32                 HCINTMSKx;                        /* Host channel-x interrupt mask                        */
    CPU_REG32                 HCTSIZx;                          /* Host channel-x transfer size                         */
    CPU_REG32                 HCDMAx;                           /* Host channel-x DMA address                           */
    CPU_REG32                 RSVD1[2u];
} USBH_STM32FX_HOST_CH_REG;


typedef  struct  usbh_stm32fx_dfifo_reg {                       /* ---------- DATA FIFO ACCESS DATA TYPE -------------- */
    CPU_REG32                 DATA[STM32FX_DFIFO_SIZE];         /* 4K bytes per endpoint                                */
} USBH_STM32FX_DFIFO_REG;


typedef  struct  usbh_stm32fx_reg {
                                                                /* ----- CORE GLOBAL CONTROL AND STATUS REGISTERS ----- */
    CPU_REG32                 GOTGCTL;                          /* 0x0000 Core control and status                       */
    CPU_REG32                 GOTGINT;                          /* 0x0004 Core interrupt                                */
    CPU_REG32                 GAHBCFG;                          /* 0x0008 Core AHB configuration                        */
    CPU_REG32                 GUSBCFG;                          /* 0x000C Core USB configuration                        */
    CPU_REG32                 GRSTCTL;                          /* 0x0010 Core reset                                    */
    CPU_REG32                 GINTSTS;                          /* 0x0014 Core interrupt                                */
    CPU_REG32                 GINTMSK;                          /* 0x0018 Core interrupt mask                           */
    CPU_REG32                 GRXSTSR;                          /* 0x001C Core receive status debug read                */
    CPU_REG32                 GRXSTSP;                          /* 0x0020 Core status read and pop                      */
    CPU_REG32                 GRXFSIZ;                          /* 0x0024 Core receive FIFO size                        */
    CPU_REG32                 HNPTXFSIZ;                        /* 0x0028 Host non-periodic transmit FIFO size          */
    CPU_REG32                 HNPTXSTS;                         /* 0x002C Core Non Periodic Tx FIFO/Queue status        */
    CPU_REG32                 RSVD0[2u];
    CPU_REG32                 GCCFG;                            /* 0x0038 General core configuration                    */
    CPU_REG32                 CID;                              /* 0x003C Core ID register                              */
    CPU_REG32                 RSVD1[48u];
    CPU_REG32                 HPTXFSIZ;                         /* 0x0100 Core Host Periodic Tx FIFO size               */
    CPU_REG32                 RSVD2[191u];
                                                                /* ------ HOST MODE CONTROL AND STATUS REGISTERS ------ */
    CPU_REG32                 HCFG;                             /* 0x0400 Host configuration                            */
    CPU_REG32                 HFIR;                             /* 0x0404 Host frame interval                           */
    CPU_REG32                 HFNUM;                            /* 0x0408 Host frame number/frame time remaining        */
    CPU_REG32                 RSVD3;
    CPU_REG32                 HPTXSTS;                          /* 0x0410 Host periodic transmit FIFO/queue status      */
    CPU_REG32                 HAINT;                            /* 0x0414 Host all channels interrupt                   */
    CPU_REG32                 HAINTMSK;                         /* 0x0418 Host all channels interrupt mask              */
    CPU_REG32                 RSVD4[9u];
    CPU_REG32                 HPRT;                             /* 0x0440 Host port control and status                  */
    CPU_REG32                 RSVD5[47u];
    USBH_STM32FX_HOST_CH_REG  HCH[16u];                         /* 0x0500 Host Channel-x regiters                       */
    CPU_REG32                 RSVD6[448u];
                                                                /* -- POWER & CLOCK GATING CONTROL & STATUS REGISTER -- */
    CPU_REG32                 PCGCR;                            /* 0x0E00 Power anc clock gating control                */
    CPU_REG32                 RSVD7[127u];
                                                                /* --- DATA FIFO (DFIFO) HOST-CH X ACCESS REGISTERS --- */
    USBH_STM32FX_DFIFO_REG    DFIFO[STM32FX_OTG_FS_MAX_NBR_CH]; /* 0x1000 Data FIFO host channel-x access registers     */
} USBH_STM32FX_REG;


/*
*********************************************************************************************************
*                                             LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          DRIVER DATA TYPE
*********************************************************************************************************
*/

typedef  struct  usbh_stm32fx_ch_info {
    CPU_INT16U            EP_Addr;                              /* Device addr | EP DIR | EP NBR.                       */
    CPU_INT32U            EP_PktCnt;                            /* For OUT EP packet count.                             */
    CPU_INT08U            DataTgl;
    CPU_INT32U            CurDataTgl;                           /* Data tggl for retransmit (no-ACK resp from device)   */
    CPU_INT08U            CurXferErrCnt;                        /* For IN and OUT xfer when Transaction Error intr      */
    CPU_INT08U            HaltSrc;                              /* Indicates source of the halt interrupt.              */
    CPU_BOOLEAN           Halting;                              /* Flag indicating channel halt in progress.            */
    CPU_BOOLEAN           LastTransaction;                      /* For IN processing with Rx FIFO Non-Empty intr        */
    CPU_BOOLEAN           Aborted;                              /* Flag indicating channel has been aborted.            */
                                                                /* To ensure the URB EP xfer size is not corrupted ...  */
    CPU_INT32U            AppBufLen;                            /* ... for multi-transaction transfer                   */
    CPU_INT08U           *AppBufPtr;                            /* Ptr to buf supplied by app.                          */
    USBH_URB             *URB_Ptr;
} USBH_STM32FX_CH_INFO;


typedef  struct  usbh_drv_data {
    CPU_INT08U            RH_Desc[USBH_HUB_LEN_HUB_DESC];       /* RH desc content.                                     */
    CPU_INT08U            ChMaxNbr;                             /* Max number of host channels.                         */
    USBH_STM32FX_CH_INFO  ChInfoTbl[STM32FX_OTG_FS_MAX_NBR_CH]; /* Contains information of host channels.               */
    CPU_INT16U            ChUsed;                               /* Bit array for BULK, ISOC, INTR, CTRL channel mgmt.   */
    CPU_INT16U            RH_PortConnStatChng;                  /* Port connection status change for RH.                */
    CPU_INT16U            RH_PortResetChng;                     /* Port reset change for RH.                            */
    CPU_BOOLEAN           RH_Init;
} USBH_DRV_DATA;


/*
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* --------------- DRIVER API FUNCTIONS --------------- */
static  void          USBH_STM32FX_HCD_Init            (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_FS_HCD_Start        (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_OTG_FS_HCD_Start    (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  void          USBH_EFM32_OTG_FS_HCD_Start      (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_StartHandler    (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             drv_type,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_Stop            (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  USBH_DEV_SPD  USBH_STM32FX_HCD_SpdGet          (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_Suspend         (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_Resume          (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  CPU_INT32U    USBH_STM32FX_HCD_FrameNbrGet     (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_EP_Open         (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_EP               *p_ep,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_EP_Close        (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_EP               *p_ep,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_EP_Abort        (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_EP               *p_ep,
                                                        USBH_ERR              *p_err);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_IsHalt_EP       (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_EP               *p_ep,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_URB_Submit      (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_URB              *p_urb,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_URB_Complete    (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_URB              *p_urb,
                                                        USBH_ERR              *p_err);

static  void          USBH_STM32FX_HCD_URB_Abort       (USBH_HC_DRV           *p_hc_drv,
                                                        USBH_URB              *p_urb,
                                                        USBH_ERR              *p_err);

                                                                /* -------------- ROOT HUB API FUNCTIONS -------------- */
static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortStatusGet   (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr,
                                                        USBH_HUB_PORT_STATUS  *p_port_status);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_HubDescGet      (USBH_HC_DRV           *p_hc_drv,
                                                        void                  *p_buf,
                                                        CPU_INT08U             buf_len);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortEnSet       (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortEnClr       (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortEnChngClr   (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortPwrSet      (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortPwrClr      (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortResetSet    (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortResetChngClr(USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortSuspendClr  (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_PortConnChngClr (USBH_HC_DRV           *p_hc_drv,
                                                        CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_RHSC_IntEn      (USBH_HC_DRV           *p_hc_drv);

static  CPU_BOOLEAN   USBH_STM32FX_HCD_RHSC_IntDis     (USBH_HC_DRV           *p_hc_drv);


/*
*********************************************************************************************************
*                                        LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void        STM32FX_ISR_Handler       (void              *p_drv);

static  void        STM32FX_ISR_PortConn      (USBH_STM32FX_REG  *p_reg,
                                               USBH_DRV_DATA     *p_drv_data,
                                               USBH_DEV          *p_dev);

static  void        STM32FX_ISR_PortDisconn   (USBH_STM32FX_REG  *p_reg,
                                               USBH_DRV_DATA     *p_drv_data,
                                               USBH_DEV          *p_dev);

static  void        STM32FX_ISR_RxFIFONonEmpty(USBH_STM32FX_REG  *p_reg,
                                               USBH_DRV_DATA     *p_drv_data);

static  void        STM32FX_ISR_HostChOUT     (USBH_STM32FX_REG  *p_reg,
                                               USBH_DRV_DATA     *p_drv_data,
                                               USBH_HC_DRV       *p_hc_drv,
                                               CPU_INT08U         ch_nbr);

static  void        STM32FX_ISR_HostChIN      (USBH_STM32FX_REG  *p_reg,
                                               USBH_DRV_DATA     *p_drv_data,
                                               USBH_HC_DRV       *p_hc_drv,
                                               CPU_INT08U         ch_nbr);

static  void        STM32FX_SW_Reset          (USBH_DRV_DATA     *p_drv_data);

static  CPU_INT08U  STM32FX_GetHostChNbr      (USBH_DRV_DATA     *p_drv_data,
                                               CPU_INT16U         ep_addr);

static  CPU_INT08U  STM32FX_GetFreeHostChNbr  (USBH_DRV_DATA     *p_drv_data,
                                               USBH_ERR          *p_err);

static  void        STM32FX_ChStartXfer       (USBH_STM32FX_REG  *p_reg,
                                               USBH_DRV_DATA     *p_drv_data,
                                               USBH_URB          *p_urb,
                                               CPU_INT08U         ep_is_in,
                                               CPU_INT16U         ep_max_pkt_size,
                                               CPU_INT08U         data_pid,
                                               CPU_INT08U         ch_nbr,
                                               CPU_INT08U         ep_type);

static  void        STM32FX_WrPkt             (USBH_STM32FX_REG  *p_reg,
                                               CPU_INT32U        *p_src,
                                               CPU_INT08U         ch_nbr,
                                               CPU_INT16U         bytes);

static  void        STM32FX_ChHalt            (USBH_STM32FX_REG  *p_reg,
                                               CPU_INT08U         ch_nbr);


/*
*********************************************************************************************************
*                                 INITIALIZED GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_HC_DRV_API  USBH_STM32FX_FS_HCD_DrvAPI = {
    USBH_STM32FX_HCD_Init,
    USBH_STM32FX_FS_HCD_Start,
    USBH_STM32FX_HCD_Stop,
    USBH_STM32FX_HCD_SpdGet,
    USBH_STM32FX_HCD_Suspend,
    USBH_STM32FX_HCD_Resume,
    USBH_STM32FX_HCD_FrameNbrGet,

    USBH_STM32FX_HCD_EP_Open,
    USBH_STM32FX_HCD_EP_Close,
    USBH_STM32FX_HCD_EP_Abort,
    USBH_STM32FX_HCD_IsHalt_EP,

    USBH_STM32FX_HCD_URB_Submit,
    USBH_STM32FX_HCD_URB_Complete,
    USBH_STM32FX_HCD_URB_Abort,
};

USBH_HC_DRV_API  USBH_STM32FX_OTG_FS_HCD_DrvAPI = {
    USBH_STM32FX_HCD_Init,
    USBH_STM32FX_OTG_FS_HCD_Start,
    USBH_STM32FX_HCD_Stop,
    USBH_STM32FX_HCD_SpdGet,
    USBH_STM32FX_HCD_Suspend,
    USBH_STM32FX_HCD_Resume,
    USBH_STM32FX_HCD_FrameNbrGet,

    USBH_STM32FX_HCD_EP_Open,
    USBH_STM32FX_HCD_EP_Close,
    USBH_STM32FX_HCD_EP_Abort,
    USBH_STM32FX_HCD_IsHalt_EP,

    USBH_STM32FX_HCD_URB_Submit,
    USBH_STM32FX_HCD_URB_Complete,
    USBH_STM32FX_HCD_URB_Abort,
};

USBH_HC_DRV_API  USBH_EFM32_OTG_FS_HCD_DrvAPI = {
    USBH_STM32FX_HCD_Init,
    USBH_EFM32_OTG_FS_HCD_Start,
    USBH_STM32FX_HCD_Stop,
    USBH_STM32FX_HCD_SpdGet,
    USBH_STM32FX_HCD_Suspend,
    USBH_STM32FX_HCD_Resume,
    USBH_STM32FX_HCD_FrameNbrGet,

    USBH_STM32FX_HCD_EP_Open,
    USBH_STM32FX_HCD_EP_Close,
    USBH_STM32FX_HCD_EP_Abort,
    USBH_STM32FX_HCD_IsHalt_EP,

    USBH_STM32FX_HCD_URB_Submit,
    USBH_STM32FX_HCD_URB_Complete,
    USBH_STM32FX_HCD_URB_Abort,
};

USBH_HC_RH_API  USBH_STM32FX_FS_HCD_RH_API = {
    USBH_STM32FX_HCD_PortStatusGet,
    USBH_STM32FX_HCD_HubDescGet,

    USBH_STM32FX_HCD_PortEnSet,
    USBH_STM32FX_HCD_PortEnClr,
    USBH_STM32FX_HCD_PortEnChngClr,

    USBH_STM32FX_HCD_PortPwrSet,
    USBH_STM32FX_HCD_PortPwrClr,

    USBH_STM32FX_HCD_PortResetSet,
    USBH_STM32FX_HCD_PortResetChngClr,

    USBH_STM32FX_HCD_PortSuspendClr,
    USBH_STM32FX_HCD_PortConnChngClr,

    USBH_STM32FX_HCD_RHSC_IntEn,
    USBH_STM32FX_HCD_RHSC_IntDis
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
*                                        USBH_STM32FX_HCD_Init()
*
* Description : Initialize host controller and allocate driver's internal memory/variables.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD init successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_Init (USBH_HC_DRV  *p_hc_drv,
                                     USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;
    CPU_SIZE_T      octets_reqd;
    LIB_ERR         err_lib;


    p_drv_data = (USBH_DRV_DATA *)Mem_HeapAlloc(sizeof(USBH_DRV_DATA),
                                                sizeof(CPU_ALIGN),
                                               &octets_reqd,
                                               &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
    }

    Mem_Clr(p_drv_data, sizeof(USBH_DRV_DATA));

    p_hc_drv->DataPtr = (void *)p_drv_data;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_FS_HCD_Start()
*
* Description : Start STM32FX_FS Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD start successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_FS_HCD_Start (USBH_HC_DRV  *p_hc_drv,
                                         USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;


    p_drv_data           = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    p_drv_data->ChMaxNbr = STM32FX_FS_MAX_NBR_CH;               /* Set max. number of supported host channels.          */

    USBH_STM32FX_HCD_StartHandler(p_hc_drv, DRV_STM32FX_FS, p_err);
}


/*
*********************************************************************************************************
*                                   USBH_STM32FX_OTG_FS_HCD_Start()
*
* Description : Start STM32FX_OTG_FS Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD start successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_OTG_FS_HCD_Start (USBH_HC_DRV  *p_hc_drv,
                                             USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;


    p_drv_data           = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    p_drv_data->ChMaxNbr = STM32FX_OTG_FS_MAX_NBR_CH;           /* Set max. number of supported host channels.          */
    USBH_STM32FX_HCD_StartHandler(p_hc_drv, DRV_STM32FX_OTG_FS, p_err);
}


/*
*********************************************************************************************************
*                                    USBH_EFM32_OTG_FS_HCD_Start()
*
* Description : Start EFM32_OTG_FS Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD start successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_EFM32_OTG_FS_HCD_Start (USBH_HC_DRV  *p_hc_drv,
                                           USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;


    p_drv_data           = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    p_drv_data->ChMaxNbr = STM32FX_OTG_FS_MAX_NBR_CH;           /* Set max. number of supported host channels.          */
    USBH_STM32FX_HCD_StartHandler(p_hc_drv, DRV_EFM32_OTG_FS, p_err);
}


/*
*********************************************************************************************************
*                                   USBH_STM32FX_HCD_StartHandler()
*
* Description : Start Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               drv_type    Driver type:
*                               DRV_STM32FX_FS       Driver for STM32F10x, STM32F2x and STM32F4x
*                               DRV_STM32FX_OTG_FS   Driver for STM32F74xx and STM32F75xx series.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD start successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) Flushes the control logic in the AHB Clock domain. Only AHB Clock Domain pipelines are reset.
*                   - FIFOs are not flushed with this bit.
*                   - All state machines in the AHB clock domain are reset to the Idle state after terminating the
*                     transactions on the AHB, following the protocol.
*                   - CSR control bits used by the AHB clock domain state machines are cleared.
*
*               (2) The application can write to this bit any time it wants to reset the core.
*                   Clears all the interrupts and all the CSR register bits except for the following bits:
*                     RSTPDMODL bit in PCGCCTL
*                     GAYEHCLK  bit in PCGCCTL
*                     PWRCLMP   bit in PCGCCTL
*                     STPPCLK   bit in PCGCCTL
*                     FSLSPCS   bit in HCFG
*                     DSPD      bit in DCFG
*                   All the transmit FIFOs and the receive FIFO are flushed
*                   Any transactions on the AHB Master are terminated as soon as possible.
*                   Any transactions on the USB are terminated immediately.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_StartHandler (USBH_HC_DRV  *p_hc_drv,
                                             CPU_INT08U    drv_type,
                                             USBH_ERR     *p_err)
{
    USBH_STM32FX_REG  *p_reg;
    USBH_HC_BSP_API   *p_bsp_api;
    USBH_DRV_DATA     *p_drv_data;
    CPU_INT08U         i;
    CPU_INT32U         reg_to;
    CPU_INT32U         reg_val;


    p_reg      = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA    *)p_hc_drv->DataPtr;
    p_bsp_api  =  p_hc_drv->BSP_API_Ptr;

    if (p_bsp_api->Init != (void *)0) {

        p_bsp_api->Init(p_hc_drv, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

                                                                /* ---------- DRIVER SOFTWARE INITIALIZATION ---------- */
    STM32FX_SW_Reset(p_drv_data);

    p_drv_data->RH_PortConnStatChng = DEF_BIT_NONE;
    p_drv_data->RH_PortResetChng    = DEF_BIT_NONE;

    DEF_BIT_CLR(p_reg->GAHBCFG, REG_GAHBCFG_GINTMSK);           /* Disable the global interrupt in AHB cfg reg          */

    switch (drv_type) {
        case DRV_STM32FX_FS:
        case DRV_STM32FX_OTG_FS:
             DEF_BIT_SET(p_reg->GUSBCFG, REG_GUSBCFG_PHYSEL);   /* PHY = USB 1.1 Full-Speed Serial Transceiver          */
             break;


        case DRV_EFM32_OTG_FS:
        default:
             break;
    }

                                                                /* ------------------ CORE RESET ---------------------- */
    reg_to  = STM32FX_MAX_RETRY;                                /* Check AHB master IDLE state before resetting core    */
    reg_val = p_reg->GRSTCTL;
    while ((DEF_BIT_IS_CLR(reg_val, REG_GRSTCTL_AHBIDL) == DEF_YES) &&
           (reg_to > 0u)) {
        reg_to--;
        reg_val = p_reg->GRSTCTL;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

    switch (drv_type) {
        case DRV_STM32FX_FS:
        case DRV_STM32FX_OTG_FS:                                /* Clr ctrl logic in the AHB Clk domain. See Note (1)   */
             DEF_BIT_SET(p_reg->GRSTCTL, REG_GRSTCTL_HCLK_SW_RST);
             reg_to  = STM32FX_MAX_RETRY;                       /* Wait all necessary logic is reset in the core        */
             reg_val = p_reg->GRSTCTL;
             while ((DEF_BIT_IS_SET(reg_val, REG_GRSTCTL_HCLK_SW_RST) == DEF_YES) &&
                    (reg_to > 0u)) {
                 reg_to--;
                 reg_val = p_reg->GRSTCTL;
             }
             if (reg_to == 0u) {
                *p_err = USBH_ERR_FAIL;
                 return;
             }
             break;

        case DRV_EFM32_OTG_FS:
        default:
             break;
    }

    DEF_BIT_SET(p_reg->GRSTCTL, REG_GRSTCTL_CORE_SW_RST);       /* Clr all other ctrl logic. See Note (2)               */
    reg_to  = STM32FX_MAX_RETRY;                                /* Wait all necessary logic is reset in the core        */
    reg_val = p_reg->GRSTCTL;
    while ((DEF_BIT_IS_SET(reg_val, REG_GRSTCTL_CORE_SW_RST) == DEF_YES) &&
           (reg_to > 0u)) {
        reg_to--;
        reg_val = p_reg->GRSTCTL;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

                                                                /* Init and Configure the PHY                           */
                                                                /* Dis the I2C interface & deactivate the power down    */
    switch (drv_type) {
        case DRV_STM32FX_FS:
             p_reg->GCCFG = (REG_GCCFG_PWRDWN    |
                             REG_GCCFG_VBUSASEN  |
                             REG_GCCFG_VBUSBSEN);
             break;


        case DRV_STM32FX_OTG_FS:
             p_reg->GCCFG = (REG_GCCFG_PWRDWN    |
                             REG_GCCFG_VBDEN);
             break;


        case DRV_EFM32_OTG_FS:
        default:
             break;
    }

    DEF_BIT_SET(p_reg->GUSBCFG, REG_GUSBCFG_FORCE_HOST);        /* Force the core to be in Host mode                    */
    USBH_OS_DlyMS(50u);                                         /* Wait at least 25ms before core forces the Host mode  */

                                                                /* -------------------- HOST INIT --------------------- */
    p_reg->PCGCR =  0u;                                         /* Reset the Power and Clock Gating Register            */
    p_reg->HCFG  = (REG_HCFG_FS_LS_PHYCLK_SEL_48MHz |
                    REG_HCFG_FS_LS_SUPPORT);                    /* FS- and LS-Only Support                              */

    reg_val = p_drv_data->ChMaxNbr;                             /* Get max. number of host channels.                    */
    for (i = 0u; i < reg_val; i++) {                            /* Clear the interrupts for each channel                */
        p_reg->HCH[i].HCINTMSKx = 0u;
        p_reg->HCH[i].HCINTx    = 0xFFFFFFFFu;
    }

    reg_val = p_reg->HPRT;
                                                                /* To avoid clearing some important interrupts          */
    DEF_BIT_CLR(reg_val, (REG_HPRT_PORT_EN        |
                          REG_HPRT_PORT_CONN_DET  |
                          REG_HPRT_PORT_EN_CHNG   |
                          REG_HPRT_PORT_OCCHNG));

    if (DEF_BIT_IS_CLR(reg_val, REG_HPRT_PORT_PWR) == DEF_YES) {
        DEF_BIT_SET(p_reg->HPRT, REG_HPRT_PORT_PWR);            /* Turn on the Host port power.                         */
    }

    USBH_OS_DlyMS(200u);
                                                                /* Enables the Host mode interrupts                     */
    p_reg->GINTMSK  =  0u;                                      /* Disable all interrupts                               */
    p_reg->GINTSTS  =  0xFFFFFFFFu;                             /* Clear any pending interrupts                         */
    p_reg->GOTGINT  =  0xFFFFFFFFu;                             /* Clear any OTG pending interrupts                     */
    p_reg->GINTMSK  = (REG_GINTx_RXFLVL   |                     /* Unmask Receive FIFO Non-Empty interrupt              */
                       REG_GINTx_WKUPINT  |                     /* Unmask Resume/Remote Wakeup Detected Intr            */
                       REG_GINTx_HPRTINT);

    p_reg->GRXFSIZ   =  STM32FX_FS_RXFIFO_DEPTH;                /* Set Rx FIFO depth                                    */
                                                                /* Set Non-Periodic and Periodic Tx FIFO depths         */
    p_reg->HNPTXFSIZ = (STM32FX_FS_NONPER_CH_TXFIFO_DEPTH << 16u) |
                        STM32FX_FS_NONPER_CH_TXFIFO_START_ADDR;
    p_reg->HPTXFSIZ  = (STM32FX_FS_PER_CH_TXFIFO_DEPTH    << 16u) |
                        STM32FX_FS_PER_CH_TXFIFO_START_ADDR;

    p_reg->GRSTCTL = (REG_GRSTCTL_TXFFLUSH |
                     (REG_GRSTCTL_ALL_TXFIFO << 6u));           /* Flush All Tx FIFOs                                   */
    reg_to         =  STM32FX_MAX_RETRY;                        /* Wait for the complete FIFO flushing                  */
    while ((DEF_BIT_IS_SET(p_reg->GRSTCTL, REG_GRSTCTL_TXFFLUSH) == DEF_YES) &&
           (reg_to >  0u))  {
        reg_to--;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

    p_reg->GRSTCTL = REG_GRSTCTL_RXFFLUSH;                      /* Flush the entire RxFIFO                              */
    reg_to         = STM32FX_MAX_RETRY;                         /* Wait for the complete FIFO flushing                  */
    while ((DEF_BIT_IS_SET(p_reg->GRSTCTL, REG_GRSTCTL_RXFFLUSH) == DEF_YES) &&
           (reg_to > 0u))  {
        reg_to--;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

    DEF_BIT_SET(p_reg->GAHBCFG, REG_GAHBCFG_GINTMSK);           /* Enable the global interrupt in AHB cfg reg           */

    if (p_bsp_api->ISR_Reg != (void *)0) {
        p_bsp_api->ISR_Reg(STM32FX_ISR_Handler, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBH_STM32FX_HCD_Stop()
*
* Description : Stop Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD stop successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_Stop (USBH_HC_DRV  *p_hc_drv,
                                     USBH_ERR     *p_err)
{
    USBH_STM32FX_REG  *p_reg;
    USBH_HC_BSP_API   *p_bsp_api;
    CPU_INT32U         reg_val;


    p_reg     = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_bsp_api =  p_hc_drv->BSP_API_Ptr;

    DEF_BIT_CLR(p_reg->GAHBCFG, REG_GAHBCFG_GINTMSK);           /* Disable the global interrupt in AHB cfg reg          */
    p_reg->GINTMSK = 0u;                                        /* Disable all interrupts                               */
    p_reg->GINTSTS = 0xFFFFFFFFu;                               /* Clear any pending interrupts                         */
    p_reg->GOTGINT = 0xFFFFFFFFu;                               /* Clear any OTG pending interrupts                     */

    if (p_bsp_api->ISR_Unreg != (void *)0) {
        p_bsp_api->ISR_Unreg(p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    reg_val = p_reg->HPRT;
                                                                /* To avoid clearing some important interrupts          */
    DEF_BIT_CLR(reg_val, (REG_HPRT_PORT_EN        |
                          REG_HPRT_PORT_CONN_DET  |
                          REG_HPRT_PORT_EN_CHNG   |
                          REG_HPRT_PORT_OCCHNG));

    if (DEF_BIT_IS_SET(reg_val, REG_HPRT_PORT_PWR) == DEF_YES) {
        DEF_BIT_CLR(p_reg->HPRT, REG_HPRT_PORT_PWR);            /* Turn off the Host port power.                        */
    }

    USBH_OS_DlyMS(200u);

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_STM32FX_HCD_SpdGet()
*
* Description : Returns Host Controller speed.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           Host controller speed retrieved successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : Host controller speed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_DEV_SPD  USBH_STM32FX_HCD_SpdGet (USBH_HC_DRV  *p_hc_drv,
                                               USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;

    return (USBH_DEV_SPD_FULL);
}


/*
*********************************************************************************************************
*                                      USBH_STM32FX_HCD_Suspend()
*
* Description : Suspend Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD suspend successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_Suspend (USBH_HC_DRV  *p_hc_drv,
                                        USBH_ERR     *p_err)
{
    USBH_STM32FX_REG  *p_reg;


    p_reg = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_SET(p_reg->HPRT, REG_HPRT_PORT_SUSP);

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_STM32FX_HCD_Resume()
*
* Description : Resume Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HCD resume successful.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_Resume (USBH_HC_DRV  *p_hc_drv,
                                       USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_STM32FX_HCD_FrameNbrGet()
*
* Description : Retrieve current frame number.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           HC frame number retrieved successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : Frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_STM32FX_HCD_FrameNbrGet (USBH_HC_DRV  *p_hc_drv,
                                                  USBH_ERR     *p_err)
{
    USBH_STM32FX_REG  *p_reg;
    CPU_INT32U         frm_nbr;


    p_reg   = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    frm_nbr = (p_reg->HFNUM & 0x0000FFFFu);                     /* Frame Number [bit 15-0]                              */

   *p_err = USBH_ERR_NONE;

    return (frm_nbr);
}


/*
*********************************************************************************************************
*                                      USBH_STM32FX_HCD_EP_Open()
*
* Description : Allocate/open endpoint of given type.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           Endpoint opened successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) It is a Root Hub which is viewed by the stack as a USB Device. The Root Hub is enumerated
*                   but an Endpoint doesn't need to be physically opened.
*               (2) The Endpoint 0 (EP0) is always used for enumerating and configuring the USB device.
*                   EP0 is the default endpoint. Because the Host stack calls only once the STM32_EPOpen()
*                   while enumerating the connected device, 2 Host channels must be opened for EP0 IN and OUT.
*                   Any channel can be opened and configured as a control channel to communicate with an endpoint
*                   other than EP0. In this case, only one channel must be opened.
*               (3) Bits [21:20] HCCHARx[MC] set to '01'. Only used for non-periodic transfers in DMA mode and periodic
*                   transfers when using split transactions. Even in Slave mode, MC field is set to '01' because
*                   writing '00' yields to undefined results.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_EP_Open (USBH_HC_DRV  *p_hc_drv,
                                        USBH_EP      *p_ep,
                                        USBH_ERR     *p_err)
{
    USBH_STM32FX_REG  *p_reg;
    USBH_DRV_DATA     *p_drv_data;
    CPU_INT08U         dev_addr;
    CPU_INT08U         ch_nbr;
    CPU_INT08U         ep_nbr;
    CPU_INT08U         ep_type;
    CPU_INT08U         ep_dir;
    CPU_SR_ALLOC();


    p_reg      = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA    *)p_hc_drv->DataPtr;

                                                                /* Check if Channel 0 & 1 have been opened and used.    */
    if ((DEF_BIT_IS_SET_ANY(p_drv_data->ChUsed, 0x3u) == DEF_YES) &&
        (USBH_EP_TypeGet(p_ep) == USBH_EP_TYPE_CTRL)) {
       *p_err = USBH_ERR_NONE;
        return;
    }

    ch_nbr = STM32FX_GetFreeHostChNbr(p_drv_data, p_err);
    if (*p_err != USBH_ERR_NONE){
        return;
    }

    dev_addr = p_ep->DevAddr;
    ep_nbr   = USBH_EP_LogNbrGet(p_ep);
    ep_dir   = USBH_EP_DirGet(p_ep);
    ep_type  = USBH_EP_TypeGet(p_ep);

    CPU_CRITICAL_ENTER();

    DEF_BIT_SET(p_reg->GINTMSK, REG_GINTx_HCINT);               /* Make sure Global Host Channel interrupt is enabled.  */

    switch (ep_type) {                                          /* The current transfer and specific init.              */
        case USBH_EP_TYPE_CTRL:                                 /* See Note #2                                          */
             p_drv_data->ChInfoTbl[ch_nbr].EP_Addr = (USBH_EP_DIR_OUT  |
                                                      ep_nbr);

             ch_nbr = STM32FX_GetFreeHostChNbr(p_drv_data,
                                               p_err);          /* Host Channel # IN                                    */
             if (*p_err != USBH_ERR_NONE) {
                 CPU_CRITICAL_EXIT();
                 return;
             }

             p_drv_data->ChInfoTbl[ch_nbr].EP_Addr = (USBH_EP_DIR_IN  |
                                                      ep_nbr);
             break;


        case USBH_EP_TYPE_INTR:
        case USBH_EP_TYPE_BULK:
        case USBH_EP_TYPE_ISOC:
             p_drv_data->ChInfoTbl[ch_nbr].EP_Addr = ((dev_addr << 8u) |
                                                       ep_dir          |
                                                       ep_nbr);
             break;


        default:
             break;
    }
    CPU_CRITICAL_EXIT();

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_STM32FX_HCD_EP_Close()
*
* Description : Close specified endpoint.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           Endpoint closed successfully.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) Channel 0 and 1 are reserved to control endpoints and are used by all devices
*                   connected to the host controller. Control endpoints cannot be completely closed.
*                   Otherwise there is a risk to finish prematurely communication with others USB devices
*                   connected behind a hub for instance. Yet if the control endpoints need to be closed
*                   by the USB host stack, it is safe to clear some specific registers. It will avoid
*                   processing unmanaged interrupts that could cause an unstable behavior.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_EP_Close (USBH_HC_DRV  *p_hc_drv,
                                         USBH_EP      *p_ep,
                                         USBH_ERR     *p_err)
{
    USBH_STM32FX_REG  *p_reg;
    USBH_DRV_DATA     *p_drv_data;
    CPU_INT08U         ep_nbr;
    CPU_INT08U         ep_dir;
    CPU_INT08U         ch_nbr;
    CPU_INT16U         dev_addr;


    p_reg      = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA    *)p_hc_drv->DataPtr;
    ep_nbr     =  USBH_EP_LogNbrGet(p_ep);
    ep_dir     =  USBH_EP_DirGet(p_ep);
    dev_addr   = (p_ep->DevAddr << 8u);

    if (ep_nbr != 0u) {                                         /* Non-Control EPs.                                     */
        ch_nbr = STM32FX_GetHostChNbr(p_drv_data, (dev_addr | ep_nbr | ep_dir));
        if (ch_nbr != 0xFFu) {
                                                                /* Reset registers related to Host channel #n.          */
            p_reg->HCH[ch_nbr].HCCHARx   = 0u;
            p_reg->HCH[ch_nbr].HCINTMSKx = 0u;
            p_reg->HCH[ch_nbr].HCINTx    = 0xFFFFFFFFu;
            p_reg->HCH[ch_nbr].HCTSIZx   = 0u;

            p_drv_data->ChInfoTbl[ch_nbr].EP_Addr   = 0xFFFFu;
            p_drv_data->ChInfoTbl[ch_nbr].EP_PktCnt = 0u;
            p_drv_data->ChInfoTbl[ch_nbr].DataTgl   = REG_PID_DATA1;
            DEF_BIT_CLR(p_drv_data->ChUsed, DEF_BIT(ch_nbr));
        }

    } else {                                                    /* Control EP IN & OUT.                                 */
                                                                /* Reset only some reg (see Note #1).                   */
        p_reg->HCH[0u].HCINTMSKx = 0u;
        p_reg->HCH[0u].HCINTx    = 0xFFFFFFFFu;
        p_reg->HCH[0u].HCTSIZx   = 0u;
        p_reg->HCH[1u].HCINTMSKx = 0u;
        p_reg->HCH[1u].HCINTx    = 0xFFFFFFFFu;
        p_reg->HCH[1u].HCTSIZx   = 0u;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_STM32FX_HCD_EP_Abort()
*
* Description : Abort all pending URBs on endpoint.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           Endpoint aborted successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_EP_Abort (USBH_HC_DRV  *p_hc_drv,
                                         USBH_EP      *p_ep,
                                         USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_ep;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_IsHalt_EP()
*
* Description : Retrieve endpoint halt state.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           Endpoint halt status retrieved successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : DEF_TRUE,       If endpoint halted.
*
*               DEF_FALSE,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_STM32FX_HCD_IsHalt_EP (USBH_HC_DRV  *p_hc_drv,
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
*                                     USBH_STM32FX_HCD_URB_Submit()
*
* Description : Submit specified URB.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_urb       Pointer to URB structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           URB submitted successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) Bits HCCHARx[MC] set to 00. Only used for non-periodic transfers in DMA mode and periodic
*                   transfers when using split transactions
*               (2) Host controller feature: the software must program the PID field with the
*                   initial data PID (to be used on the first OUT transaction or to be expected
*                   from the first IN transaction).
*                   For control transaction:
*                   - SETUP  stage: initial PID = SETUP
*                   - DATA   stage: initial PID = DATA1 (IN or OUT transaction). Then, the data toggle must
*                   alternate between DATA0 and DATA1.
*                   - STATUS stage: initial PID = DATA1 (IN or OUT transaction)
*                   (a) For Bulk transaction: the first transaction is set to DATA0. Then, the data toggle must
*                       alternate between DATA0 and DATA1.
*                   (b) For Interrupt transaction: the first transaction is set to DATA0. Then, the data toggle must
*                   alternate between DATA0 and DATA1.
*               (3) Bit HCCHARx[Oddfrm] set to '1'. OTG host perform a transfer in an odd frame. But the channel
*                   must be enabled in the even frame preceding the odd frame.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_URB_Submit (USBH_HC_DRV  *p_hc_drv,
                                           USBH_URB     *p_urb,
                                           USBH_ERR     *p_err)
{
    USBH_STM32FX_REG  *p_reg;
    USBH_DRV_DATA     *p_drv_data;
    CPU_INT08U         ch_nbr;
    CPU_INT08U         data_pid;
    CPU_INT08U         ep_nbr;
    CPU_INT08U         ep_type;
    CPU_INT08U         ep_dir;
    CPU_INT08U         ep_is_in;
    CPU_INT16U         ep_max_pkt_size;
    CPU_INT16U         dev_addr;
    CPU_INT32U         reg_val;


    p_reg           = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data      = (USBH_DRV_DATA    *)p_hc_drv->DataPtr;
    ep_type         =  USBH_EP_TypeGet(p_urb->EP_Ptr);
    ep_nbr          =  USBH_EP_LogNbrGet(p_urb->EP_Ptr);
    ep_dir          =  USBH_EP_DirGet(p_urb->EP_Ptr);
    ep_max_pkt_size =  USBH_EP_MaxPktSizeGet(p_urb->EP_Ptr);
    dev_addr        = (p_urb->EP_Ptr->DevAddr);
    ep_is_in        = (ep_dir == USBH_EP_DIR_IN) ? 1u : 0u;

                                                                /* If Dev disconn & some remaining URBs processed by .. */
                                                                /* ..AsyncThread, ignore it                             */
    if ((p_drv_data->RH_Init                           == DEF_TRUE               ) ||
        (DEF_BIT_IS_CLR(p_reg->HPRT, REG_HPRT_PORT_EN) == DEF_TRUE               ) ||
        (p_urb->State                                  == USBH_URB_STATE_ABORTED)) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

    reg_val  =  0u;
    reg_val  = (CPU_INT32U)(dev_addr << 22u);                   /* Set the device address to which the channel belongs  */
    reg_val |= (CPU_INT32U)(ep_nbr   << 11u);                   /* Set the associated EP address                        */
    if (ep_dir == USBH_EP_DIR_IN) {                             /* Set the direction of the channel                     */
        DEF_BIT_SET(reg_val, REG_HCCHARx_EPDIR);
    } else {
        DEF_BIT_CLR(reg_val, REG_HCCHARx_EPDIR);
    }
    if (p_urb->EP_Ptr->DevSpd == USBH_DEV_SPD_LOW) {            /* indicate to the channel is communicating to a LS dev */
        DEF_BIT_SET(reg_val, REG_HCCHARx_LOW_SPEED_DEV);
    }
    reg_val |= (CPU_INT32U)(ep_type << 18u);                    /* Set the endpoint type                                */
    reg_val |= (CPU_INT32U)ep_max_pkt_size;                     /* Set the maximum endpoint size                        */
                                                                /* Set the Multi Count field (bits 21-20). See Note #3  */
    reg_val |= (CPU_INT32U)(REG_HCCHARx_1_TRANSACTION << 20u) & REG_HCCHARx_MC_EC;

    switch (ep_type){
        case USBH_EP_TYPE_CTRL:                                 /* ------------------ CONTROL CHANNEL ----------------- */
             ep_is_in = ((p_urb->Token == USBH_TOKEN_SETUP) ? 0u : ((p_urb->Token == USBH_TOKEN_IN) ? 1u : 0u));

             if (ep_is_in == 0u) {
                 ch_nbr = STM32FX_GetHostChNbr(p_drv_data,
                                               p_urb->EP_Ptr->Desc.bEndpointAddress);

                 DEF_BIT_CLR(reg_val, REG_HCCHARx_EPDIR);       /* OUT direction                                        */
             } else {
                 ch_nbr = STM32FX_GetHostChNbr(p_drv_data,
                                              (ep_nbr | USBH_EP_DIR_IN));

                 DEF_BIT_SET(reg_val, REG_HCCHARx_EPDIR);       /* IN direction                                         */
             }

             switch (p_urb->Token) {                            /* ----> CTRL DATA TOGGLE. See Note #2                  */
                 case USBH_TOKEN_SETUP:
                      data_pid = REG_PID_SETUP_MDATA;
                      break;


                 case USBH_TOKEN_IN:
                      if ((p_urb->Err == USBH_ERR_HC_IO) ||
                          (p_urb->Err == USBH_ERR_EP_NACK)) {
                          data_pid = p_drv_data->ChInfoTbl[ch_nbr].CurDataTgl;
                      } else {
                          data_pid = REG_PID_DATA1;
                      }
                      break;


                 case USBH_TOKEN_OUT:
                      data_pid = REG_PID_DATA1;
                      break;


                 default:
                     *p_err = USBH_ERR_UNKNOWN;
                      return;
             }
             break;


        case USBH_EP_TYPE_BULK:                                 /* ------------------- BULK CHANNEL ------------------- */
             ch_nbr = STM32FX_GetHostChNbr(p_drv_data,
                                          ((dev_addr << 8u) | ep_nbr | ep_dir));

             if (ep_dir == USBH_EP_DIR_OUT) {                   /* ----> BULK DATA TOGGLE. See Note #2(a)               */
                 if (p_urb->Err == USBH_ERR_EP_NACK) {          /* Transaction retransmission                           */
                     data_pid = p_drv_data->ChInfoTbl[ch_nbr].CurDataTgl;
                 } else {                                       /* Normal OUT transmit, tggl data between DATA0 & DATA1 */
                     data_pid                              = (p_drv_data->ChInfoTbl[ch_nbr].DataTgl ^ REG_PID_DATA1);
                     p_drv_data->ChInfoTbl[ch_nbr].DataTgl =  data_pid;
                 }

             } else if (ep_dir == USBH_EP_DIR_IN) {
                 if (p_urb->Err == USBH_ERR_EP_NACK) {          /* Transaction retransmission                           */
                     data_pid = p_drv_data->ChInfoTbl[ch_nbr].CurDataTgl;
                 } else {                                       /* Normal IN transmit, tggl data between DATA0 & DATA1  */
                     data_pid                              = (p_drv_data->ChInfoTbl[ch_nbr].DataTgl ^ REG_PID_DATA1);
                     p_drv_data->ChInfoTbl[ch_nbr].DataTgl =  data_pid;
                 }
             }
             break;


        case USBH_EP_TYPE_INTR:                                 /* ------------------- INTERRUPT CHANNEL -------------- */
             ch_nbr = STM32FX_GetHostChNbr(p_drv_data,
                                          ((dev_addr << 8u) | ep_nbr | ep_dir));

             if (ep_dir == USBH_EP_DIR_OUT) {                   /* ----> INTERRUPT DATA TOGGLE. See Note #2(b)          */
                 if (p_urb->Err == USBH_ERR_EP_NACK) {          /* Transaction retransmission                           */
                     data_pid = p_drv_data->ChInfoTbl[ch_nbr].CurDataTgl;
                 } else {                                       /* Normal OUT transmit, tggl data between DATA0 & DATA1 */
                     data_pid                              = (p_drv_data->ChInfoTbl[ch_nbr].DataTgl ^ REG_PID_DATA1);
                     p_drv_data->ChInfoTbl[ch_nbr].DataTgl =  data_pid;
                 }

             } else if (ep_dir == USBH_EP_DIR_IN) {
                 data_pid                              = (p_drv_data->ChInfoTbl[ch_nbr].DataTgl ^ REG_PID_DATA1);
                 p_drv_data->ChInfoTbl[ch_nbr].DataTgl =  data_pid;

                 DEF_BIT_SET(p_reg->GINTMSK, REG_GINTx_IPXFR);
             }

             if (DEF_BIT_IS_CLR(p_reg->HFNUM, DEF_BIT_00) == DEF_YES) {
                 DEF_BIT_SET(reg_val, REG_HCCHARx_ODDFRM);      /* OTG host perform a transfer in an odd (micro)frame.  */
             } else {
                 DEF_BIT_CLR(reg_val, REG_HCCHARx_ODDFRM);      /* OTG host perform a transfer in an even (micro)frame. */
             }
             break;


        case USBH_EP_TYPE_ISOC:
        default:
             break;
    }

    p_drv_data->ChInfoTbl[ch_nbr].URB_Ptr = p_urb;              /* Store transaction URB                                */

    p_reg->HCH[ch_nbr].HCINTx    =  0x000007FFu;                /* Clear old interrupt conditions for this host channel */
                                                                /* Unmask the necessary interrupts                      */
    p_reg->HCH[ch_nbr].HCINTMSKx = (REG_HCINTx_XFRC   |
                                    REG_HCINTx_ACK    |
                                    REG_HCINTx_STALL  |
                                    REG_HCINTx_NAK    |
                                    REG_HCINTx_TXERR  |
                                    REG_HCINTx_DTERR  |
                                    REG_HCINTx_BBERR);

    p_reg->HAINTMSK |= (1u << ch_nbr);                          /* Enable the top level host channel interrupt.         */

    p_reg->HCH[ch_nbr].HCCHARx = reg_val;                       /* Init Host Channel #N                                 */

    if (p_urb->Err != USBH_ERR_EP_NACK) {                       /* if not URB retransmission, save User buffer length.. */
                                                                /* ..for xfer multi-transactions                        */
        p_drv_data->ChInfoTbl[ch_nbr].AppBufLen = p_urb->UserBufLen;
        p_drv_data->ChInfoTbl[ch_nbr].AppBufPtr = (CPU_INT08U *)p_urb->UserBufPtr;
    }

                                                                /* --------------- (2) SUBMIT DATA TO HW -------------- */
    STM32FX_ChStartXfer(p_reg,
                        p_drv_data,
                        p_urb,
                        ep_is_in,
                        ep_max_pkt_size,
                        data_pid,
                        ch_nbr,
                        ep_type);

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_STM32FX_HCD_URB_Complete()
*
* Description : Complete specified URB.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_urb       Pointer to URB structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           URB completed successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_URB_Complete (USBH_HC_DRV  *p_hc_drv,
                                             USBH_URB     *p_urb,
                                             USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_urb;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_URB_Abort()
*
* Description : Abort specified URB.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_urb       Pointer to URB structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                           USBH_ERR_NONE           URB aborted successfuly.
*                           Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_STM32FX_HCD_URB_Abort (USBH_HC_DRV  *p_hc_drv,
                                          USBH_URB     *p_urb,
                                          USBH_ERR     *p_err)
{
    USBH_STM32FX_CH_INFO  *p_ch_info;
    USBH_STM32FX_CH_INFO  *p_ch_info_ctrl_in;
    USBH_STM32FX_CH_INFO  *p_ch_info_ctrl_out;
    USBH_STM32FX_REG      *p_reg;
    USBH_DRV_DATA         *p_drv_data;
    CPU_INT08U             ep_nbr;
    CPU_INT08U             ep_dir;
    CPU_INT08U             ch_nbr_ctrl_in;
    CPU_INT08U             ch_nbr_ctrl_out;
    CPU_INT08U             ch_nbr;
    CPU_INT16U             dev_addr;
    CPU_INT16U             retry_cnt;
    CPU_BOOLEAN            ch_halt_status;
    CPU_SR_ALLOC();


    p_reg      = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA    *)p_hc_drv->DataPtr;
    ep_nbr     =  USBH_EP_LogNbrGet(p_urb->EP_Ptr);
    ep_dir     =  USBH_EP_DirGet(p_urb->EP_Ptr);
    dev_addr   = (p_urb->EP_Ptr->DevAddr << 8u);

    if (ep_dir == USBH_EP_DIR_NONE) {

        ch_nbr_ctrl_in     =  STM32FX_GetHostChNbr(p_drv_data, (ep_nbr | USBH_EP_DIR_IN));
        ch_nbr_ctrl_out    =  STM32FX_GetHostChNbr(p_drv_data, (ep_nbr | USBH_EP_DIR_OUT));
        p_ch_info_ctrl_in  = &p_drv_data->ChInfoTbl[ch_nbr_ctrl_in];
        p_ch_info_ctrl_out = &p_drv_data->ChInfoTbl[ch_nbr_ctrl_out];

    } else if ((ep_dir == USBH_EP_DIR_IN) ||
               (ep_dir == USBH_EP_DIR_OUT)) {

        ch_nbr    =  STM32FX_GetHostChNbr(p_drv_data, (dev_addr | ep_nbr | ep_dir));
        p_ch_info = &p_drv_data->ChInfoTbl[ch_nbr];
    }

                                                                /* --------------- WAIT FOR CH HALT END --------------- */
    retry_cnt      = 0u;
    CPU_CRITICAL_ENTER();
    ch_halt_status = (ep_dir == USBH_EP_DIR_NONE) ? (p_ch_info_ctrl_in->Halting | p_ch_info_ctrl_out->Halting) : p_ch_info->Halting;
    CPU_CRITICAL_EXIT();
    while ((ch_halt_status == DEF_TRUE) &&
           (retry_cnt       < STM32FX_MAX_RETRY)) {

        retry_cnt++;
        CPU_CRITICAL_ENTER();
        ch_halt_status = (ep_dir == USBH_EP_DIR_NONE) ? (p_ch_info_ctrl_in->Halting | p_ch_info_ctrl_out->Halting) : p_ch_info->Halting;
        CPU_CRITICAL_EXIT();
    }
    if (retry_cnt == STM32FX_MAX_RETRY) {
       *p_err = USBH_ERR_HC_IO;
        return;
    }
                                                                /* ----------------------- HALT CH -------------------- */
    if (ep_dir == USBH_EP_DIR_NONE) {

        CPU_CRITICAL_ENTER();
        p_ch_info_ctrl_in->Aborted  = DEF_TRUE;
        p_ch_info_ctrl_out->Aborted = DEF_TRUE;
        p_ch_info_ctrl_in->HaltSrc  = REG_HCINTx_HALT_SRC_ABORT;
        p_ch_info_ctrl_out->HaltSrc = REG_HCINTx_HALT_SRC_ABORT;

        p_reg->HAINTMSK |= (1u << ch_nbr_ctrl_in);              /* Enable the top level host channel interrupt.         */
        p_reg->HAINTMSK |= (1u << ch_nbr_ctrl_out);
        STM32FX_ChHalt(p_reg, ch_nbr_ctrl_in);                  /* Halt (i.e. disable) the channel                      */
        STM32FX_ChHalt(p_reg, ch_nbr_ctrl_out);
        CPU_CRITICAL_EXIT();

    } else if ((ep_dir == USBH_EP_DIR_IN) ||
               (ep_dir == USBH_EP_DIR_OUT)) {

        CPU_CRITICAL_ENTER();
        p_ch_info->Aborted = DEF_TRUE;
        p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_ABORT;

        p_reg->HAINTMSK |= (1u << ch_nbr);                      /* Enable the top level host channel interrupt.         */
        STM32FX_ChHalt(p_reg, ch_nbr);                          /* Halt (i.e. disable) the channel                      */
        CPU_CRITICAL_EXIT();
    }

    p_urb->State = USBH_URB_STATE_ABORTED;
   *p_err        = USBH_ERR_NONE;
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
*                                   USBH_STM32FX_HCD_PortStatusGet()
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
* Note(s)     : (1) The STM32 OTG Host supports only ONE port.
*               (2) Port numbers start from 1.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortStatusGet (USBH_HC_DRV           *p_hc_drv,
                                                     CPU_INT08U             port_nbr,
                                                     USBH_HUB_PORT_STATUS  *p_port_status)
{
    USBH_STM32FX_REG  *p_reg;
    USBH_DRV_DATA     *p_drv_data;
    CPU_INT32U         reg_val;
    CPU_INT16U         status;
    CPU_INT16U         change;


    p_reg      = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA    *)p_hc_drv->DataPtr;

    if (port_nbr != 1u) {
        p_port_status->wPortStatus = 0u;
        p_port_status->wPortChange = 0u;
        return (DEF_FAIL);
    }

    reg_val = p_reg->HPRT;
    status  = DEF_BIT_NONE;
    change  = DEF_BIT_NONE;
                                                                /* Bits not used by the stack. Maintain constant value. */
    DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_OVER_CUR);         /* Over-current (PORT_OVER_CURRENT).                    */
    DEF_BIT_SET(status, USBH_HUB_STATUS_PORT_PWR);              /* Port power (PORT_POWER).                             */
    DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_TEST);             /* Port test mode (PORT_TEST).                          */
    DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_INDICATOR);        /* Port indicator control (PORT_INDICATOR).             */

    if ((reg_val & REG_HPRT_PORT_PWR) != 0u) {
        DEF_BIT_SET(status, USBH_HUB_STATUS_PORT_EN);           /* Port enabled/disabled (PORT_ENABLE).                 */
    } else {
        DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_EN);
    }

    if ((reg_val & REG_HPRT_PORT_CONN_STS) != 0u) {             /* If a device is attached to the port                  */
        DEF_BIT_SET(status, USBH_HUB_STATUS_PORT_CONN);         /* Current connection status (PORT_CONNECTION).         */
    } else {
        DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_CONN);
    }

    if ((reg_val & REG_HPRT_PORT_SPEED) == REG_HPRT_PORT_SPD_FS) {
        DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_LOW_SPD);      /* PORT_LOW_SPEED  = 0 = FS dev attached.               */
        DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_HIGH_SPD);     /* PORT_HIGH_SPEED = 0 = FS dev attached.               */
    } else if ((reg_val & REG_HPRT_PORT_SPEED) == REG_HPRT_PORT_SPD_LS) {
        DEF_BIT_SET(status, USBH_HUB_STATUS_PORT_LOW_SPD);      /* PORT_LOW_SPEED  = 1 = LS dev attached.               */
    } else if ((reg_val & REG_HPRT_PORT_SPEED) == REG_HPRT_PORT_SPD_HS) {
        DEF_BIT_SET(status, USBH_HUB_STATUS_PORT_HIGH_SPD);     /* PORT_HIGH_SPEED = 1 = HS dev attached.               */
    }

    if ((reg_val & REG_HPRT_PORT_EN_CHNG) != 0u) {              /* Port Enable Change                                   */
        DEF_BIT_SET(change, USBH_HUB_STATUS_C_PORT_EN);
    } else {
        DEF_BIT_CLR(change, USBH_HUB_STATUS_C_PORT_EN);
    }

    if (p_drv_data->RH_PortConnStatChng == DEF_TRUE) {          /* Port Connection Change                               */
        DEF_BIT_SET(change, USBH_HUB_STATUS_C_PORT_CONN);
    }

    if (p_drv_data->RH_PortResetChng == DEF_TRUE) {             /* Reset (PORT_RESET).                                  */
        DEF_BIT_SET(change, USBH_HUB_STATUS_C_PORT_RESET);
    } if ((reg_val & REG_HPRT_PORT_RST) == REG_HPRT_PORT_IN_RESET) {
        DEF_BIT_SET(status, USBH_HUB_STATUS_PORT_RESET);
    } else {
        DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_RESET);
    }

                                                                /* Suspend (PORT_SUSPEND).                              */
    if ((reg_val & REG_HPRT_PORT_SUSP) == REG_HPRT_PORT_IN_SUSPEND_MODE) {
        DEF_BIT_SET(status, USBH_HUB_STATUS_PORT_SUSPEND);
    } else {
        DEF_BIT_CLR(status, USBH_HUB_STATUS_PORT_SUSPEND);
    }

    p_port_status->wPortStatus = status;
    p_port_status->wPortChange = change;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_HubDescGet()
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
*
*               (3) The STM32 OTG Host supports only ONE downstream port.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_STM32FX_HCD_HubDescGet (USBH_HC_DRV  *p_hc_drv,
                                                  void         *p_buf,
                                                  CPU_INT08U    buf_len)
{
    USBH_DRV_DATA  *p_drv_data;
    USBH_HUB_DESC   hub_desc;


    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;

    hub_desc.bDescLength         = USBH_HUB_LEN_HUB_DESC;
    hub_desc.bDescriptorType     = USBH_HUB_DESC_TYPE_HUB;
    hub_desc.bNbrPorts           = 1u;                          /* See Note #3                                          */
    hub_desc.wHubCharacteristics = 0u;
    hub_desc.bPwrOn2PwrGood      = 100u;
    hub_desc.bHubContrCurrent    = 0u;

    USBH_HUB_FmtHubDesc(&hub_desc,
                         p_drv_data->RH_Desc);                  /* Write the structure in USB format                    */

    buf_len = DEF_MIN(buf_len, sizeof(USBH_HUB_DESC));

    Mem_Copy(p_buf,
             p_drv_data->RH_Desc,
             buf_len);                                          /* Copy the formatted structure into the buffer         */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_PortEnSet()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortEnSet (USBH_HC_DRV  *p_hc_drv,
                                                 CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_PortEnClr()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortEnClr (USBH_HC_DRV  *p_hc_drv,
                                                 CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_STM32FX_HCD_PortEnChngClr()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortEnChngClr (USBH_HC_DRV  *p_hc_drv,
                                                     CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_PortPwrSet()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortPwrSet (USBH_HC_DRV  *p_hc_drv,
                                                  CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_PortPwrClr()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortPwrClr (USBH_HC_DRV  *p_hc_drv,
                                                  CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_STM32FX_HCD_PortResetSet()
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
* Note(s)     : (1) The application must wait at least 10 ms (+ 10 ms security)
*                   before clearing the reset bit.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortResetSet (USBH_HC_DRV  *p_hc_drv,
                                                    CPU_INT08U    port_nbr)
{
    USBH_STM32FX_REG  *p_reg;
    CPU_INT32U         reg_val;

    (void)port_nbr;

    p_reg   = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    reg_val =  p_reg->HPRT;                                     /* Read host port control and status                    */

    if ((reg_val & REG_HPRT_PORT_PWR) == DEF_FALSE) {           /* Is the port in Power-off state?                      */
        return (DEF_FAIL);                                      /* Yes. So ignore the operation.                        */

    } else {                                                    /* ---- LAUNCH COMPLETE RESET SEQUENCE ON THE PORT ---- */
        DEF_BIT_CLR(reg_val, (REG_HPRT_PORT_EN        |         /* To avoid clearing some important interrupts          */
                              REG_HPRT_PORT_CONN_DET  |
                              REG_HPRT_PORT_EN_CHNG   |
                              REG_HPRT_PORT_OCCHNG));

        DEF_BIT_SET(reg_val, REG_HPRT_PORT_RST);                /* Launch the reset sequence                            */
        p_reg->HPRT = reg_val;

        USBH_OS_DlyMS(30u);                                     /* See Note #1                                          */

        DEF_BIT_CLR(reg_val, REG_HPRT_PORT_RST);                /* Clear the reset bit                                  */
        p_reg->HPRT = reg_val;
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  USBH_STM32FX_HCD_PortResetChngClr()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortResetChngClr (USBH_HC_DRV  *p_hc_drv,
                                                        CPU_INT08U    port_nbr)
{
    USBH_DRV_DATA  *p_drv_data;

    (void)port_nbr;

    p_drv_data                   = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    p_drv_data->RH_PortResetChng =  DEF_FALSE;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_STM32FX_HCD_PortSuspendClr()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortSuspendClr (USBH_HC_DRV  *p_hc_drv,
                                                      CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  USBH_STM32FX_HCD_PortConnChngClr()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_PortConnChngClr (USBH_HC_DRV  *p_hc_drv,
                                                       CPU_INT08U    port_nbr)
{
    USBH_DRV_DATA  *p_drv_data;

    (void)port_nbr;

    p_drv_data                      = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    p_drv_data->RH_PortConnStatChng =  DEF_FALSE;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_STM32FX_HCD_RHSC_IntEn()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_RHSC_IntEn (USBH_HC_DRV  *p_hc_drv)
{
    USBH_STM32FX_REG  *p_reg;


    p_reg = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_SET(p_reg->GINTMSK, REG_GINTx_HPRTINT);
    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_STM32FX_HCD_RHSC_IntDis()
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

static  CPU_BOOLEAN  USBH_STM32FX_HCD_RHSC_IntDis (USBH_HC_DRV  *p_hc_drv)
{
    USBH_STM32FX_REG  *p_reg;


    p_reg = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_CLR(p_reg->GINTMSK, REG_GINTx_HPRTINT);
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
*                                        STM32FX_ISR_Handler()
*
* Description : ISR handler.
*
* Arguments   : p_drv        Pointer to host controller driver structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  STM32FX_ISR_Handler (void  *p_drv)
{
    USBH_STM32FX_REG  *p_reg;
    USBH_DRV_DATA     *p_drv_data;
    USBH_HC_DRV       *p_hc_drv;
    CPU_INT16U         ch_nbr;
    CPU_INT32U         ch_int;
    CPU_INT32U         gintsts_reg;
    CPU_INT32U         reg_val;


    p_hc_drv     = (USBH_HC_DRV      *)p_drv;
    p_reg        = (USBH_STM32FX_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data   = (USBH_DRV_DATA    *)p_hc_drv->DataPtr;
    gintsts_reg  =  p_reg->GINTSTS;
    gintsts_reg &=  p_reg->GINTMSK;                             /* Keep only the unmasked interrupt                     */

                                                                /* ------------- PORT STATUS CHANGE INT --------------- */
    if (DEF_BIT_IS_SET(gintsts_reg, REG_GINTx_HPRTINT) == DEF_YES) {
        DEF_BIT_CLR(p_reg->GAHBCFG, REG_GAHBCFG_GINTMSK);       /* Disable the global interrupt for the OTG controller  */
        STM32FX_ISR_PortConn(p_reg,
                             p_drv_data,
                             p_hc_drv->RH_DevPtr);

        DEF_BIT_SET(p_reg->GAHBCFG, REG_GAHBCFG_GINTMSK);       /* Enable the global interrupt for the OTG controller   */
        return;                                                 /* No other interrupts must be processed                */

    } else if (DEF_BIT_IS_SET(gintsts_reg, REG_GINTx_DISCINT) == DEF_YES) {
        DEF_BIT_CLR(p_reg->GAHBCFG, REG_GAHBCFG_GINTMSK);       /* Disable the global interrupt for the OTG controller  */
        STM32FX_ISR_PortDisconn(p_reg,
                                p_drv_data,
                                p_hc_drv->RH_DevPtr);
        DEF_BIT_SET(p_reg->GAHBCFG, REG_GAHBCFG_GINTMSK);       /* Enable the global interrupt for the OTG controller   */
        return;                                                 /* No other interrupts must be processed                */
    }

                                                                /* ----------------- EP TRANSFER INT ------------------ */
    if (DEF_BIT_IS_SET(gintsts_reg, REG_GINTx_RXFLVL) == DEF_YES) {
        STM32FX_ISR_RxFIFONonEmpty(p_reg, p_drv_data);          /* Handle the RxFIFO Non-Empty interrupt                */
    }

    if (DEF_BIT_IS_SET(gintsts_reg, REG_GINTx_HCINT) == DEF_YES) {
        ch_int  = (p_reg->HAINT & 0x0000FFFFu);                 /* Read HAINT reg to serve all the active ch interrupts */
        ch_nbr  =  CPU_CntTrailZeros(ch_int);
        reg_val =  p_reg->HCH[ch_nbr].HCCHARx;

        while (ch_int != 0u) {                                  /* Handle Host channel interrupt                        */
            if ((reg_val & REG_HCCHARx_EPDIR) == 0u) {          /* OUT Channel                                          */
                STM32FX_ISR_HostChOUT(p_reg,
                                      p_drv_data,
                                      p_hc_drv,
                                      ch_nbr);
            } else if ((reg_val & REG_HCCHARx_EPDIR) != 0u) {   /* IN Channel                                           */
                STM32FX_ISR_HostChIN(p_reg,
                                     p_drv_data,
                                     p_hc_drv,
                                     ch_nbr);
            }

            DEF_BIT_CLR(ch_int, DEF_BIT(ch_nbr));
            ch_int  = (p_reg->HAINT & 0x0000FFFFu);             /* Read HAINT reg to serve all the active ch interrupts */
            ch_nbr  =  CPU_CntTrailZeros(ch_int);
            reg_val =  p_reg->HCH[ch_nbr].HCCHARx;
        }
    }

    if (DEF_BIT_IS_SET(gintsts_reg, REG_GINTx_IPXFR) == DEF_YES) {
        DEF_BIT_CLR(p_reg->GINTMSK, REG_GINTx_IPXFR);           /* Mask Incomplete Periodic Transfer interrupt          */

        ch_nbr = (p_reg->HPTXSTS & REG_HPTXSTS_CH_EP_NBR) >> 27u;
        DEF_BIT_SET(p_reg->HCH[0].HCCHARx, REG_HCCHARx_ODDFRM);

        p_reg->GINTSTS = REG_GINTx_IPXFR;                       /* Acknowledge interrupt by a write clear               */
    }
}


/*
*********************************************************************************************************
*                                        STM32FX_ISR_PortConn()
*
* Description : Handle Host Port interrupt. The core sets this bit to indicate a change in port status
*                of one of the DWC_otg core ports in Host mode.
*
* Argument(s) : p_reg         Pointer to STM32Fx registers structure.
*
*               p_drv_data    Pointer to host driver data structure.
*
*               p_dev         Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : (1) This case is a special case when using an USB traffic analyzer. The core generates
*                   an erroneous connection detected interrupt while there is no device connected to the
*                   Host port.
*
*               (2) When the Device is attached to the port. FIRST, the Device connection interrupt is
*                   detected and launches a complete reset sequence on the port. THEN, the port enabled
*                   change bit in HPRT register triggers another Device connection interrupt and the
*                   PHY clock selection is adjusted to limit the power according to the speed device.
*********************************************************************************************************
*/

static  void  STM32FX_ISR_PortConn (USBH_STM32FX_REG  *p_reg,
                                    USBH_DRV_DATA     *p_drv_data,
                                    USBH_DEV          *p_dev)
{
    CPU_INT32U  hprt_reg;
    CPU_INT32U  hprt_reg_dup;
    CPU_INT32U  hcfg_reg;
    CPU_SR_ALLOC();


    hprt_reg     = p_reg->HPRT;
    hprt_reg_dup = p_reg->HPRT;
                                                                /* To avoid clearing some important interrupts          */
    DEF_BIT_CLR(hprt_reg_dup, (REG_HPRT_PORT_EN        |
                               REG_HPRT_PORT_CONN_DET  |
                               REG_HPRT_PORT_EN_CHNG   |
                               REG_HPRT_PORT_OCCHNG));

    if (((hprt_reg & REG_HPRT_PORT_CONN_DET) != 0u      ) &&    /* --(1)-- Port Connect Detected. See Note#1            */
        ((hprt_reg & REG_HPRT_PORT_CONN_STS) == DEF_TRUE)) {

        DEF_BIT_SET(hprt_reg_dup, REG_HPRT_PORT_CONN_DET);
        p_drv_data->RH_Init             = DEF_FALSE;
        p_drv_data->RH_PortConnStatChng = DEF_TRUE;

        USBH_HUB_RH_Event(p_dev);
                                                                /* Enable the Disconnect interrupt                      */
        DEF_BIT_SET(p_reg->GINTMSK, REG_GINTx_DISCINT);

    } else if (((hprt_reg & REG_HPRT_PORT_CONN_DET) != 0u       ) &&
               ((hprt_reg & REG_HPRT_PORT_CONN_STS) == DEF_FALSE)) { /*    See Note #1                                  */

        DEF_BIT_SET(p_reg->HPRT, REG_HPRT_PORT_CONN_DET);
        return;
    }

    if (((hprt_reg & REG_HPRT_PORT_EN_CHNG)  != 0u      ) &&    /* --(2)-- Port Enable Changed. See Note#2              */
        ((hprt_reg & REG_HPRT_PORT_CONN_STS) == DEF_TRUE)) {

        CPU_CRITICAL_ENTER();

        DEF_BIT_SET(hprt_reg_dup, REG_HPRT_PORT_EN_CHNG);

        if ((hprt_reg & REG_HPRT_PORT_EN) != 0u) {              /* Is the port enabled after the reset sequence due .. */
                                                                /*  ..to the Device connection?                        */
            if (((hprt_reg & REG_HPRT_PORT_SPEED) == REG_HPRT_PORT_SPD_LS) ||
                ((hprt_reg & REG_HPRT_PORT_SPEED) == REG_HPRT_PORT_SPD_FS)) {
                hcfg_reg = p_reg->HCFG;

                if ((hprt_reg & REG_HPRT_PORT_SPEED) == REG_HPRT_PORT_SPD_LS) {
                    p_reg->HFIR = 6000u;

                    if ((hcfg_reg & REG_HCFG_FS_LS_CLK_SEL) != REG_HCFG_FS_LS_PHYCLK_SEL_6MHz) {
                        DEF_BIT_CLR(hcfg_reg, REG_HCFG_FS_LS_CLK_SEL);
                                                                /* Ensure PHY clock  = 6 MHz for LS                     */
                        DEF_BIT_SET(hcfg_reg, REG_HCFG_FS_LS_PHYCLK_SEL_6MHz);
                        p_reg->HCFG = hcfg_reg;
                    }

                } else {
                    p_reg->HFIR = 48000u;

                    if ((hcfg_reg & REG_HCFG_FS_LS_CLK_SEL) != REG_HCFG_FS_LS_PHYCLK_SEL_48MHz) {
                        DEF_BIT_CLR(hcfg_reg, REG_HCFG_FS_LS_CLK_SEL);
                                                                /* Ensure PHY clock  = 48 MHz for FS                    */
                        DEF_BIT_SET(hcfg_reg, REG_HCFG_FS_LS_PHYCLK_SEL_48MHz);
                        p_reg->HCFG = hcfg_reg;
                    }
                }

                p_drv_data->RH_PortResetChng = DEF_TRUE;

                USBH_HUB_RH_Event(p_dev);
            }
        }

        CPU_CRITICAL_EXIT();
    }

    if ((hprt_reg & REG_HPRT_PORT_OCCHNG) != 0u) {              /* Overcurrent Change Interrupt                         */
        DEF_BIT_SET(hprt_reg_dup, REG_HPRT_PORT_OCCHNG);
    }

    if ((hprt_reg & REG_HPRT_PORT_EN_CHNG) != 0u) {             /* Port Enable/Disable Change Interrupt                 */
        DEF_BIT_SET(hprt_reg_dup, REG_HPRT_PORT_EN_CHNG);
    }

    p_reg->HPRT = hprt_reg_dup;
}


/*
*********************************************************************************************************
*                                      STM32FX_ISR_PortDisconn()
*
* Description : Handle Disconnect Detected Interrupt. Asserted when a device disconnect is detected.
*
* Argument(s) : p_reg         Pointer to STM32Fx registers structure.
*
*               p_drv_data    Pointer to host driver data structure.
*
*               p_dev         Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  STM32FX_ISR_PortDisconn (USBH_STM32FX_REG  *p_reg,
                                       USBH_DRV_DATA     *p_drv_data,
                                       USBH_DEV          *p_dev)
{
    CPU_INT08U  i;
    CPU_INT08U  max_ch_nbr;
    CPU_INT32U  reg_val;


    DEF_BIT_SET(p_reg->GINTSTS, REG_GINTx_DISCINT);             /* Ack Device Disconnected interrupt by a write clear   */
    reg_val = p_reg->HPRT;                                      /* Read host port control and status reg                */

    if ((reg_val & REG_HPRT_PORT_CONN_STS) == DEF_FALSE) {      /*  No device is attached to the port                   */
        DEF_BIT_SET(p_reg->HPRT   , REG_HPRT_PORT_CONN_DET);    /* Clear the Port Host Interrupt in GINSTS register     */
        DEF_BIT_SET(p_reg->GINTMSK, REG_GINTx_HPRTINT);         /* Now En Root hub Intr to detect a new device conn     */
        DEF_BIT_CLR(p_reg->GINTMSK, REG_GINTx_DISCINT);         /* Disable the Disconnect interrupt                     */

                                                                /* -------- UNINIT ALL ALLOCATED HOST CHANNELS -------- */
        max_ch_nbr = p_drv_data->ChMaxNbr;
        for (i = 2u; i < max_ch_nbr; i++) {
            if (p_drv_data->ChInfoTbl[i].EP_Addr != 0xFFFFu) {  /* Handle the registers related to Host channel #i      */

                reg_val = p_reg->HCH[i].HCCHARx;
                if ((reg_val & REG_HCCHARx_CHENA) != 0u) {      /* Halt the channel if enabled                          */
                    DEF_BIT_SET(p_reg->HCH[i].HCCHARx, (REG_HCCHARx_CHENA  |
                                                        REG_HCCHARx_CHDIS));
                } else {
                    p_reg->HCH[i].HCCHARx = 0u;                 /* Reset the HCCHARx register                           */
                }
            }

            p_reg->HCH[i].HCINTMSKx = 0u;
            p_reg->HCH[i].HCINTx    = 0xFFFFFFFFu;
            p_reg->HCH[i].HCTSIZx   = 0u;
            p_reg->HAINTMSK        &= ~(1u << i);
        }

                                                                /* --------------- FLUSH TX AND RX FIFO --------------- */
        p_reg->GRSTCTL = (REG_GRSTCTL_TXFFLUSH |
                         (REG_GRSTCTL_ALL_TXFIFO << 6u));       /* Flush All Tx FIFOs                                   */
        reg_val        =  STM32FX_MAX_RETRY;                    /* Wait for the complete FIFO flushing                  */
        while ((DEF_BIT_IS_SET(p_reg->GRSTCTL, REG_GRSTCTL_TXFFLUSH)) &&
               (reg_val >  0u))  {
            reg_val--;
        }
        if (reg_val == 0u) {
           return;
        }

        p_reg->GRSTCTL = REG_GRSTCTL_RXFFLUSH;                  /* Flush the entire RxFIFO                              */
        reg_val        = STM32FX_MAX_RETRY;                     /* Wait for the complete FIFO flushing                  */
        while ((DEF_BIT_IS_SET(p_reg->GRSTCTL, REG_GRSTCTL_RXFFLUSH)) &&
               (reg_val > 0u))  {
            reg_val--;
        }
        if (reg_val == 0u) {
           return;
        }

        STM32FX_SW_Reset(p_drv_data);                           /* Init some global var for the new conn of the device  */
        p_drv_data->RH_PortConnStatChng = DEF_TRUE;             /* Indicate the port connection change for root hub     */

        USBH_HUB_RH_Event(p_dev);
    }
}


/*
*********************************************************************************************************
*                                     STM32FX_ISR_RxFIFONonEmpty()
*
* Description : Handle Receive FIFO Non-Empty interrupt generated when a data IN packet has been received.
*
* Argument(s) : p_reg         Pointer to STM32Fx registers structure.
*
*               p_drv_data    Pointer to host driver data structure.
*
* Return(s)   : None.
*
* Note(s)     : (1) An IN data packet has been received
*               (2) Store the buffer pointer because if the transfer is composed of several packets,
*                   the data of the next packet must be stored following the previous packet's data
*********************************************************************************************************
*/

static  void  STM32FX_ISR_RxFIFONonEmpty (USBH_STM32FX_REG  *p_reg,
                                          USBH_DRV_DATA     *p_drv_data)
{
    USBH_STM32FX_CH_INFO  *p_ch_info;
    USBH_URB              *p_urb;
    CPU_INT32U            *p_data_buf;
    CPU_INT08U             ch_nbr;
    CPU_INT16U             total_pck_nbr;
    CPU_INT32U             reg_val;
    CPU_INT32U             byte_cnt;
    CPU_INT32U             data_pid;
    CPU_INT32U             pkt_stat;
    CPU_INT32U             pkt_cnt;
    CPU_INT32U             word_cnt;
    CPU_INT32U             i;


    DEF_BIT_CLR(p_reg->GINTMSK, REG_GINTx_RXFLVL);              /* --(1)-- Mask RxFLvl interrupt                        */

                                                                /* --(2)-- Handle Packet Status bits in GRXSTSP         */
    reg_val    =  p_reg->GRXSTSP;                               /* Read the Host Mode Status Pop Registers (GRXSTSP)    */
    ch_nbr     =  reg_val & 0x0000000Fu;                        /* bits [3:0] Channel Number                            */
    byte_cnt   = (reg_val & 0x00007FF0u) >>  4u;                /* bits [14:4] Byte Count                               */
    data_pid   = (reg_val & 0x00018000u) >> 15u;                /* bits [16:15] Data PID                                */
    pkt_stat   = (reg_val & 0x001E0000u) >> 17u;                /* bits [20:17] Packet Status                           */
    word_cnt   = ((byte_cnt + 3u) / 4u);

    reg_val    =  p_reg->HCH[ch_nbr].HCTSIZx;
    pkt_cnt    = (reg_val & 0x1FF80000u) >> 19u;                /* bits [28:19] Packet Count                            */
    reg_val    =  p_reg->HCH[ch_nbr].HCCHARx;
    p_ch_info  = &p_drv_data->ChInfoTbl[ch_nbr];
    p_urb      =  p_ch_info->URB_Ptr;
    p_data_buf = (CPU_INT32U *)p_ch_info->AppBufPtr;

    switch (pkt_stat) {
        case REG_GRXSTS_PKTSTS_IN:                              /* See Note #1                                          */
             if ((byte_cnt           >          0u) &&
                 (p_urb->UserBufPtr != (void  *)0)) {

                 for (i = 0u; i < word_cnt; i++) {              /* Read the received IN packet data and store it        */
                    *p_data_buf = *p_reg->DFIFO[0u].DATA;
                     p_data_buf++;
                 }

                                                                /* Update the amount of bytes being received.           */
                 total_pck_nbr         = (p_ch_info->AppBufLen + p_urb->EP_Ptr->Desc.wMaxPacketSize - 1u) / p_urb->EP_Ptr->Desc.wMaxPacketSize;
                 pkt_stat              =  total_pck_nbr * p_urb->EP_Ptr->Desc.wMaxPacketSize;
                 p_urb->XferLen       +=  pkt_stat - (p_reg->HCH[ch_nbr].HCTSIZx & REG_HCTSIZx_XFRSIZ_MSK);
                 p_ch_info->AppBufLen -=  pkt_stat - (p_reg->HCH[ch_nbr].HCTSIZx & REG_HCTSIZx_XFRSIZ_MSK);

                 if (pkt_cnt > 0u) {                            /* See Note#2                                           */
                     p_ch_info->AppBufPtr = (void *)p_data_buf;
                 }

                 if ((pkt_cnt > 0u) && (total_pck_nbr > 1u)) {  /* Transfer composed of several packets                 */
                     DEF_BIT_SET(reg_val, REG_HCCHARx_CHENA);   /* Application must re-enable the channel to write an.. */
                     DEF_BIT_CLR(reg_val, REG_HCCHARx_CHDIS);   /* IN request in the Request Queue for the next IN pkt  */
                     p_reg->HCH[ch_nbr].HCCHARx = reg_val;

                     p_ch_info->LastTransaction = DEF_TRUE;
                 }
             }
             break;


        case REG_GRXSTS_PKTSTS_IN_XFER_COMP:
             if (p_ch_info->LastTransaction == DEF_TRUE) {      /* Store last data PID of the last xfer's transaction   */
                 p_ch_info->DataTgl         = (data_pid ^ REG_PID_DATA0);
                 p_ch_info->LastTransaction = DEF_FALSE;
             }
             break;


        case REG_GRXSTS_PKTSTS_DATA_TOGGLE_ERR:
        case REG_GRXSTS_PKTSTS_CH_HALTED:
        default:
             if ((p_drv_data->RH_Init                           == DEF_TRUE) ||
                 (DEF_BIT_IS_CLR(p_reg->HPRT, REG_HPRT_PORT_EN) == DEF_TRUE)) {
                 p_urb->Err         = USBH_ERR_HC_IO;
                 p_urb->XferLen     = 0u;
                 USBH_URB_Done(p_urb);                          /* Notify the Core layer about the URB abort            */
             }
             break;                                             /* Handled in interrupt, just ignore data               */
    }

    DEF_BIT_SET(p_reg->GINTMSK, REG_GINTx_RXFLVL);              /* --(3)-- Unmask RxFLvl interrupt                      */
}


/*
*********************************************************************************************************
*                                          STM32FX_SW_Reset()
*
* Description : Init some global variables for the new connection of the device.
*
* Argument(s) : p_drv_data    Pointer to host driver data structure.
*
* Return(s)   : none
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  STM32FX_SW_Reset (USBH_DRV_DATA  *p_drv_data)
{
    CPU_INT08U  i;
    CPU_INT08U  max_ch_nbr;


    max_ch_nbr = p_drv_data->ChMaxNbr;                          /* Get max. supported host channels.                    */
    for (i = 0u; i < max_ch_nbr; i++) {
        p_drv_data->ChInfoTbl[i].DataTgl         = REG_PID_DATA1;
        p_drv_data->ChInfoTbl[i].CurDataTgl      = REG_PID_NONE;
        p_drv_data->ChInfoTbl[i].CurXferErrCnt   = 0u;
        p_drv_data->ChInfoTbl[i].LastTransaction = DEF_FALSE;
        p_drv_data->ChInfoTbl[i].AppBufLen       = 0u;
        p_drv_data->ChInfoTbl[i].EP_PktCnt       = 0u;
        p_drv_data->ChInfoTbl[i].HaltSrc         = REG_HCINTx_HALT_SRC_NONE;
        p_drv_data->ChInfoTbl[i].Halting         = DEF_FALSE;
        p_drv_data->ChInfoTbl[i].Aborted         = DEF_FALSE;
    }

    p_drv_data->ChUsed   = 0u;
    p_drv_data->RH_Init  = DEF_TRUE;                            /* When device connects to the port                     */
}


/*
*********************************************************************************************************
*                                             STM32FX_GetHostChNbr()
*
* Description : Get the host channel number corresponding to a given device address, endpoint number
*               and direction.
*
* Argument(s) : p_drv_data   Pointer to host driver data structure.
*
*               ep_addr      Contains Device Address | Endpoint direction | Endpoint number.
*
* Return(s)   : Host channel number or 0xFF if no host channel is found
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  STM32FX_GetHostChNbr (USBH_DRV_DATA  *p_drv_data,
                                          CPU_INT16U      ep_addr)
{
    CPU_INT08U  ch_nbr;
    CPU_INT08U  max_ch_nbr;

    max_ch_nbr = p_drv_data->ChMaxNbr;                          /* Get max. supported host channels.                    */
    for (ch_nbr = 0u; ch_nbr < max_ch_nbr; ch_nbr++) {
        if (p_drv_data->ChInfoTbl[ch_nbr].EP_Addr == ep_addr) {
            return (ch_nbr);
        }
    }

    return (0xFFu);
}


/*
*********************************************************************************************************
*                                      STM32FX_GetFreeHostChNbr()
*
* Description : Allocate a free host channel number for the newly opened channel.
*
* Argument(s) : p_drv_data   Pointer to host driver data structure.
*
*               p_err        pointer to an error.
*                            USBH_ERR_NONE, if a number was successfully allocated
*                            USBH_ERR_EP_ALLOC, if there is no channel available
*
* Return(s)   : Host channel number or 0xFF if no host channel is found
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  STM32FX_GetFreeHostChNbr (USBH_DRV_DATA  *p_drv_data,
                                              USBH_ERR       *p_err)
{
    CPU_INT08U  ch_nbr;
    CPU_INT08U  max_ch_nbr;


    max_ch_nbr = p_drv_data->ChMaxNbr;                          /* Get max. supported host channels.                    */
    for (ch_nbr = 0u; ch_nbr < max_ch_nbr; ch_nbr++) {
        if DEF_BIT_IS_CLR(p_drv_data->ChUsed, DEF_BIT(ch_nbr)) {
            DEF_BIT_SET(p_drv_data->ChUsed, DEF_BIT(ch_nbr));
           *p_err = USBH_ERR_NONE;
            return (ch_nbr);
        }
    }

   *p_err = USBH_ERR_EP_ALLOC;
    return (0xFFu);
}


/*
*********************************************************************************************************
*                                        STM32FX_ChStartXfer()
*
* Description : Start transfer by configuring HCTSIZx and HCCHARx register for the Host channel
*
* Argument(s) : p_reg             Pointer to STM32Fx registers structure.
*
*               p_drv_data        Pointer to host driver data structure.
*
*               p_urb             Pointer to URB structure.
*
*               ep_is_in          Direction of the transfer
*
*               ep_max_pkt_size   Maximum endpoint packet size
*
*               data_pid          Data PID (SETUP, DATA0, DATA1 or DATA2) for the transaction
*
* Return(s)   : None.
*
* Note(s)     : (1) For an IN, HCTSIZx[XferSize] contains the buffer size that the application has
*                   reserved for the transfer. The application is expected to program this field as an
*                   integer multiple of the maximum size for IN transactions (periodic and non-periodic).
*               (2) For multi-transaction transfer, the Core will manage the data toggling for each
*                   transaction sent to the USB Device
*               (3) Bits [21:20] HCCHARx[MC] set to 00. Only used for non-periodic transfers in DMA
*                   mode and periodictransfers when using split transactions
*********************************************************************************************************
*/

static  void  STM32FX_ChStartXfer (USBH_STM32FX_REG  *p_reg,
                                   USBH_DRV_DATA     *p_drv_data,
                                   USBH_URB          *p_urb,
                                   CPU_INT08U         ep_is_in,
                                   CPU_INT16U         ep_max_pkt_size,
                                   CPU_INT08U         data_pid,
                                   CPU_INT08U         ch_nbr,
                                   CPU_INT08U         ep_type)
{
    CPU_INT32U   dev_addr;
    CPU_INT32U   reg_val;
    CPU_INT32U   xfer_len;
    CPU_INT16U   nbr_pkts;
    CPU_INT32U   txsts_reg;
    CPU_INT32U   tx_fifo_space;
    CPU_INT32U   xfer_len_in_words;
    CPU_INT32U  *p_user_buf;
    CPU_SR_ALLOC();


    xfer_len = (p_urb->UserBufLen - p_urb->XferLen);
    dev_addr =  p_urb->EP_Ptr->DevAddr;

    if (p_urb->UserBufLen > 0u) {                               /* Compute expected nbr of pckts associated to the xfer */
        nbr_pkts = (xfer_len + ep_max_pkt_size - 1u) / ep_max_pkt_size;

    } else {                                                    /* For zero-length packet, consider 1 packet            */
        nbr_pkts = 1u;
    }

    if (ep_is_in == 1u) {                                       /* If EP IN?...                                         */
        if (nbr_pkts > REG_HCTSIZx_XFRSIZ_MAX) {
            nbr_pkts = REG_HCTSIZx_XFRSIZ_MAX;                  /* Set register max. receive transfer size.             */
            p_drv_data->ChInfoTbl[ch_nbr].EP_PktCnt = 1u;       /* Xfer EP IN size exceeds max register setup.          */
        }
        xfer_len = nbr_pkts * ep_max_pkt_size;                  /* See Note #1                                          */
    }

    reg_val  = 0u;                                              /* Initialize the HCTSIZx register                      */
    reg_val  = ((CPU_INT32U)data_pid) << 29u;                   /* PID. See Note #2                                     */
    reg_val |= ((CPU_INT32U)nbr_pkts) << 19u;                   /* Packet Count                                         */
    reg_val |= (xfer_len & 0x0007FFFFu);                        /* Transfer Size                                        */
    p_reg->HCH[ch_nbr].HCTSIZx = reg_val;

    if ((ep_is_in == 0u) && xfer_len > 0u) {                    /* Load OUT packet into the appropriate Tx FIFO.        */
        switch(ep_type) {
            case USBH_EP_TYPE_CTRL:
            case USBH_EP_TYPE_BULK:
                 txsts_reg         =  p_reg->HNPTXSTS;
                 tx_fifo_space     = (txsts_reg & 0x0000FFFFu); /* Non-periodic Tx FIFO Space Available                 */
                 xfer_len_in_words = ((xfer_len - 1u) / 4u) + 1u;

                 if (xfer_len_in_words > tx_fifo_space) {
                     xfer_len = tx_fifo_space * 4;
                     nbr_pkts = (xfer_len + ep_max_pkt_size - 1u) / ep_max_pkt_size;
                     xfer_len_in_words = ((xfer_len - 1u) / 4u) + 1u;
                 }

                 do {                                           /* Check Non-periodic Tx FIFO for available space       */
                     tx_fifo_space = (txsts_reg & 0x0000FFFFu);
                 } while (tx_fifo_space < xfer_len_in_words);
                 break;


            case USBH_EP_TYPE_INTR:
            case USBH_EP_TYPE_ISOC:
                 txsts_reg         = p_reg->HPTXSTS;
                 tx_fifo_space     = (txsts_reg & 0x0000FFFFu); /* Periodic Tx FIFO Space Available                     */
                 xfer_len_in_words = ((xfer_len - 1u) / 4u) + 1u;

                 if (xfer_len_in_words > tx_fifo_space) {
                     xfer_len = tx_fifo_space * 4;
                     nbr_pkts = (xfer_len + ep_max_pkt_size - 1u) / ep_max_pkt_size;
                     xfer_len_in_words = ((xfer_len - 1u) / 4u) + 1u;
                 }

                 do {                                           /* Check Periodic Tx FIFO for available space           */
                     tx_fifo_space = (txsts_reg & 0x0000FFFFu);
                 } while (tx_fifo_space < xfer_len_in_words);
                 break;


            default:
                 break;
        }

        reg_val  = 0u;                                          /* Initialize the HCTSIZx register                      */
        reg_val  = ((CPU_INT32U)data_pid) << 29u;               /* PID. See Note #2                                     */
        reg_val |= ((CPU_INT32U)nbr_pkts) << 19u;               /* Packet Count                                         */
        reg_val |= (xfer_len & 0x0007FFFFu);                    /* Transfer Size                                        */
        p_reg->HCH[ch_nbr].HCTSIZx = reg_val;

        if (p_urb->XferLen != 0) {                              /* Keep track of data to be sent                        */
            xfer_len_in_words = ((p_urb->XferLen - 1u) / 4u) + 1u;
            p_user_buf = (CPU_INT32U *)p_urb->UserBufPtr + xfer_len_in_words;
        } else {
            p_user_buf = (CPU_INT32U *)p_urb->UserBufPtr;
        }

        p_drv_data->ChInfoTbl[ch_nbr].EP_PktCnt = nbr_pkts;     /* Save the number of packets to send.                  */
        p_drv_data->ChInfoTbl[ch_nbr].AppBufLen = xfer_len;     /* Save the expected nbr of bytes to be sent            */

        CPU_CRITICAL_ENTER();
        STM32FX_WrPkt(p_reg, p_user_buf, ch_nbr, xfer_len);
        CPU_CRITICAL_EXIT();
    } else if (ep_is_in == 1) {
        p_drv_data->ChInfoTbl[ch_nbr].AppBufLen = xfer_len;
    }

    reg_val  =  p_reg->HCH[ch_nbr].HCCHARx;
    reg_val |= (REG_HCCHARx_CHENA | (dev_addr << 22u));
    DEF_BIT_CLR(reg_val, REG_HCCHARx_CHDIS);
    p_reg->HCH[ch_nbr].HCCHARx = reg_val;                       /* Enable the channel                                   */
}


/*
*********************************************************************************************************
*                                           STM32FX_WrPkt()
*
* Description : Writes a packet into the Tx FIFO associated with the Channel.
*
* Argument(s) : p_reg         Pointer to STM32Fx registers structure.
*
*               p_src         Pointer to data that will be transmitted.
*
*               ch_nbr        Host Channel OUT number which has generated the interrupt.
*
*               bytes         Number of bytes to write.
*
* Return(s)   : None.
*
* Note(s)     : (1) Find the DWORD length, padded by extra bytes as neccessary if MPS is not a
*                   multiple of DWORD
*********************************************************************************************************
*/

static  void  STM32FX_WrPkt (USBH_STM32FX_REG  *p_reg,
                             CPU_INT32U        *p_src,
                             CPU_INT08U         ch_nbr,
                             CPU_INT16U         bytes)
{
    CPU_INT32U   i;
    CPU_INT32U   dword_count;
    CPU_REG32   *p_fifo;
    CPU_INT32U  *p_data_buff;


    dword_count = (bytes + 3u) / 4u;                            /* See Note #1                                          */

    p_fifo      =  p_reg->DFIFO[ch_nbr].DATA;
    p_data_buff = (CPU_INT32U *)p_src;

    for (i = 0u; i < dword_count; i++, p_data_buff++) {
       *p_fifo = *p_data_buff;
    }
}


/*
*********************************************************************************************************
*                                       STM32FX_ISR_HostChOUT()
*
* Description : Handle all the interrupts related to an OUT transaction (success and errors)
*
* Argument(s) : p_reg         Pointer to STM32Fx registers structure.
*
*               p_drv_data    Pointer to host driver data structure.
*
*               p_hc_drv      Pointer to host controller driver structure.
*
*               ch_nbr        Host Channel OUT number which has generated the interrupt.
*
* Return(s)   : None.
*
* Note(s)     :     The controller spec indicates different non-ACK handling according to the transfer
*                   type(Control, Bulk, Interrupt Isochronous). Yet, here, the non-ACK responses are
*                   handled in the same place.
*                   Control/Bulk    NAK, XactErr, STALL, NYET
*                   Interrupt       NAK, XactErr, STALL,     , FrmOvrun
*                   Isochronous                                FrmOvrun
*
*               (1) Indicates the transfer completed abnormally either because of any USB transaction
*                   error or in response to disable request by the application.
*
*               (2) Indicates one of the following errors occurred on the USB (related to physical
*                   signaling)
*                     CRC check failure
*                     Timeout
*                     Bit stuff error
*                     False EOP
*
*               (3) In case of transfer aborted, this code prevents the ISR from notifying the Core
*                   of a transfer completion with error after the URB abortion.
*
*               (4) The OTG-FS controller does not support hardware retransmission if the current
*                   transaction for a control, bulk or interrupt has been NAKed or a Transaction Error
*                   has been detected. Hence, retransmission is managed by the driver.
*********************************************************************************************************
*/

static  void  STM32FX_ISR_HostChOUT (USBH_STM32FX_REG  *p_reg,
                                     USBH_DRV_DATA     *p_drv_data,
                                     USBH_HC_DRV       *p_hc_drv,
                                     CPU_INT08U         ch_nbr)
{
    USBH_STM32FX_CH_INFO  *p_ch_info;
    USBH_URB              *p_urb;
    CPU_INT32U             hcint_reg;
    CPU_BOOLEAN            halt_ch;
    CPU_INT32U             ep_type;
    CPU_INT32U             data_pid;
    CPU_INT32U             pkt_cnt;
    CPU_INT32U             reg_val;
    USBH_ERR               p_err;


    hcint_reg  =  p_reg->HCH[ch_nbr].HCINTx;
    hcint_reg &= (p_reg->HCH[ch_nbr].HCINTMSKx);
    ep_type    = (p_reg->HCH[ch_nbr].HCCHARx & REG_HCCHARx_EP_TYPE_MSK) >> 18u;
    data_pid   = (p_reg->HCH[ch_nbr].HCTSIZx & REG_HCTSIZx_PID_MSK)     >> 29u;
    pkt_cnt    = (p_reg->HCH[ch_nbr].HCTSIZx & REG_HCTSIZx_PKTCNT_MSK)  >> 19u;
    p_ch_info  = &p_drv_data->ChInfoTbl[ch_nbr];
    p_urb      =  p_ch_info->URB_Ptr;
    p_err      =  USBH_ERR_NONE;
    halt_ch    =  DEF_FALSE;
                                                                /* ---------- SUCCESSFUL TRANSFER COMPLETION ---------- */
    if ((hcint_reg & REG_HCINTx_XFRC) != 0u) {

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_XFRC;            /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_XFER_CMPL;
        halt_ch                   = DEF_TRUE;
                                                                /* Mask ACK int.                                        */
        DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_ACK);

                                                                /* ---------------------- ERROR ----------------------- */
    } else if ((hcint_reg & REG_HCINTx_STALL) != 0u) {          /* STALL Response Received Interrupt                    */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_STALL;           /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_STALL;
        halt_ch                   = DEF_TRUE;

    } else if ((hcint_reg & REG_HCINTx_NAK) != 0u) {            /* NAK Response Received Interrupt                      */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_NAK;             /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_NAK;
        p_ch_info->CurXferErrCnt  = 0u;                         /* Reset err cnt.                                       */
        halt_ch                   = DEF_TRUE;

    } else if ((hcint_reg & REG_HCINTx_TXERR) != 0u) {          /* Transaction Error. See Note #2                       */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_TXERR;           /* Ack int.                                             */
        halt_ch                   = DEF_TRUE;

        p_ch_info->CurXferErrCnt++;                             /* Increment err cnt in case of transaction err.        */
        if (p_ch_info->CurXferErrCnt < 3u) {
            p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_NAK;       /* Notify core to retry xfer.                           */
        } else {
            p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_TXERR;     /* Notify core to abort xfer.                           */
        }
                                                                /* Unmask ACK int.                                      */
        DEF_BIT_SET(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_ACK);

                                                                /* ------------------ CHANNEL HALTED ------------------ */
    }  else if ((hcint_reg & REG_HCINTx_CH_HALTED)!= 0u) {

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_CH_HALTED;       /* Ack int.                                             */
                                                                /* Mask ch halted int.                                  */
        DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_CH_HALTED);

        if ((p_ch_info->Aborted == DEF_TRUE) &&                 /* See Note #3.                                         */
            (p_ch_info->HaltSrc != REG_HCINTx_HALT_SRC_ABORT)) {

            p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_NONE;
        }


        switch (p_ch_info->HaltSrc) {
            case REG_HCINTx_HALT_SRC_XFER_CMPL:                 /* Successful xfer completion.                          */

                 if (ep_type == USBH_EP_TYPE_BULK) {            /* Store last data PID of the last xfer's transaction   */
                     p_ch_info->DataTgl = (data_pid ^ REG_PID_DATA1);
                 }

                 p_ch_info->CurXferErrCnt = 0u;                 /* Reset err cnt.                                       */
                 p_ch_info->EP_PktCnt     = 0u;
                 p_urb->Err               = USBH_ERR_NONE;
                                                                /* Update XferLen with bytes sent on Tx complete        */
                 p_urb->XferLen          += p_ch_info->AppBufLen;

                 if (p_urb->XferLen == p_urb->UserBufLen) {
                     USBH_URB_Done(p_urb);                      /* Notify the Core layer about the URB completion       */
                     p_ch_info->URB_Ptr = (USBH_URB *)0;

                 } else if (p_urb->XferLen < p_urb->UserBufLen){/* Send more data                                       */

                     USBH_STM32FX_HCD_URB_Submit(p_hc_drv,      /* Retransmit the URB on the channel                    */
                                                 p_urb,
                                                &p_err);
                 }
                 break;


            case REG_HCINTx_HALT_SRC_STALL:
                 p_ch_info->CurXferErrCnt = 0u;                 /* Reset err cnt.                                       */
                 p_urb->Err               = USBH_ERR_EP_STALL;
                 p_urb->XferLen           = 0u;

                 USBH_URB_Done(p_urb);                          /* Notify the Core layer about the STALL condition      */
                 p_ch_info->URB_Ptr = (USBH_URB *)0;
                 break;


            case REG_HCINTx_HALT_SRC_TXERR:
                 p_ch_info->CurXferErrCnt = 0u;                 /* Reset err cnt.                                       */
                 p_urb->Err               = USBH_ERR_HC_IO;
                 p_urb->XferLen           = 0u;

                 USBH_URB_Done(p_urb);                          /* Notify the Core layer about the URB abort            */
                 p_ch_info->URB_Ptr = (USBH_URB *)0;
                 break;


            case REG_HCINTx_HALT_SRC_NAK:                       /* Retransmit xfer on channel (see Note #4).            */
                 ep_type = USBH_EP_TypeGet(p_urb->EP_Ptr);
                 if ((ep_type == USBH_EP_TYPE_BULK) ||
                     (ep_type == USBH_EP_TYPE_CTRL)) {

                     p_ch_info->CurDataTgl = data_pid;          /* Data toggle not changed for retransmission.          */

                     if (pkt_cnt >= 1u) {                       /* Transfer with more than 1 transaction                */
                         if (pkt_cnt < p_ch_info->EP_PktCnt) {  /* Check if any packets has been sent before EP NACK    */
                             reg_val         = (p_ch_info->EP_PktCnt - pkt_cnt) * p_urb->EP_Ptr->Desc.wMaxPacketSize;
                             p_urb->XferLen +=  reg_val;        /* Update number of bytes sent.                         */
                         }
                         p_ch_info->EP_PktCnt = 0u;

                         p_reg->HCH[ch_nbr].HCTSIZx = 0u;       /* Reset the HCTSIZ register                            */
                                                                /* Flush the matching DATA FIFO                         */
                         p_reg->GRSTCTL = (REG_GRSTCTL_TXFFLUSH |
                                          (REG_GRSTCTL_NONPER_TXFIFO << 6u));
                         reg_val        =  STM32FX_MAX_RETRY;   /* Wait for the complete FIFO flushing                  */
                         while ((DEF_BIT_IS_SET(p_reg->GRSTCTL, REG_GRSTCTL_TXFFLUSH) == DEF_YES) &&
                                (reg_val >  0u))  {
                             reg_val--;
                         }

                         if (reg_val == 0u) {
                             return;
                         }
                     }

                 } else if (ep_type == USBH_EP_TYPE_INTR) {
                     p_ch_info->CurDataTgl = data_pid;
                 }

                 p_urb->Err = USBH_ERR_EP_NACK;
                 USBH_STM32FX_HCD_URB_Submit(p_hc_drv,
                                             p_urb,
                                            &p_err);
                 if (p_err != USBH_ERR_NONE) {
                     p_urb->Err     = USBH_ERR_HC_IO;
                     p_urb->XferLen = 0u;

                     USBH_URB_Done(p_urb);                      /* Notify the Core layer about the URB abort            */
                     p_ch_info->URB_Ptr       = (USBH_URB *)0;
                     p_ch_info->CurXferErrCnt =  0u;            /* Reset error count                                    */
                 }
                 break;


            case REG_HCINTx_HALT_SRC_ABORT:
            case REG_HCINTx_HALT_SRC_NONE:
            default:
                 break;
        }

        p_ch_info->Halting = DEF_FALSE;
        p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_NONE;
        p_ch_info->Aborted = DEF_FALSE;
                                                                /* ------ ACK RESPONSE RECEIVED/TRANSMITTED INT ------- */
    } else if ((hcint_reg & REG_HCINTx_ACK) != 0u) {

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_ACK;             /* Ack int.                                             */
        p_ch_info->CurXferErrCnt  = 0u;                         /* Reset err cnt.                                       */
                                                                /* Mask ACK int.                                        */
        DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_ACK);
    }

    if (halt_ch == DEF_TRUE) {
        p_ch_info->Halting = DEF_TRUE;
        STM32FX_ChHalt(p_reg, ch_nbr);                          /* Halt (i.e. disable) the channel.                     */
    }
}


/*
*********************************************************************************************************
*                                        STM32FX_ISR_HostChIN()
*
* Description : Handle all the interrupts related to an IN transaction (success and errors)
*
* Argument(s) : p_reg         Pointer to STM32Fx registers structure.
*
*               p_drv_data    Pointer to host driver data structure.
*
*               p_hc_drv      Pointer to host controller driver structure.
*
*               ch_nbr        Host Channel IN number which has generated the interrupt
*
* Return(s)   : None.
*
* Note(s)     :     The controller spec indicates different non-ACK handling according to the transfer
*                   type(Control, Bulk, Interrupt Isochronous). Yet, here, the non-ACK responses are
*                   handled in the same place.
*                   Control/Bulk    NAK, XactErr, STALL, BblErr, DataTglErr
*                   Interrupt       NAK, XactErr, STALL, BblErr, DataTglErr, FrmOvrun
*                   Isochronous          XactErr,        BblErr,             FrmOvrun
*
*               (1) Indicates the transfer completed abnormally either because of any USB transaction
*                   error or in response to disable request by the application.
*
*               (2) Indicates one of the following errors occurred on the USB.
*                     CRC check failure
*                     Timeout
*                     Bit stuff error
*                     False EOP
*
*               (3) In case of transfer aborted, this code prevents the ISR from notifying the Core
*                   of a transfer completion with error after the URB abortion.
*
*               (4) The OTG-FS controller does not support hardware retransmission if the current
*                   transaction for a control, bulk or interrupt has been NAKed or a Transaction Error
*                   has been detected. Hence, retransmission is managed by the driver.
*********************************************************************************************************
*/

static  void  STM32FX_ISR_HostChIN (USBH_STM32FX_REG  *p_reg,
                                    USBH_DRV_DATA     *p_drv_data,
                                    USBH_HC_DRV       *p_hc_drv,
                                    CPU_INT08U         ch_nbr)
{
    USBH_STM32FX_CH_INFO  *p_ch_info;
    USBH_URB              *p_urb;
    CPU_INT32U             hcint_reg;
    CPU_BOOLEAN            halt_ch;
    CPU_INT32U             ep_type;
    CPU_INT32U             data_pid;
    USBH_ERR               p_err;


    hcint_reg  =  p_reg->HCH[ch_nbr].HCINTx;
    hcint_reg &= (p_reg->HCH[ch_nbr].HCINTMSKx);
    ep_type    = (p_reg->HCH[ch_nbr].HCCHARx & REG_HCCHARx_EP_TYPE_MSK) >> 18u;
    data_pid   = (p_reg->HCH[ch_nbr].HCTSIZx & REG_HCTSIZx_PID_MSK)     >> 29u;
    p_ch_info  = &p_drv_data->ChInfoTbl[ch_nbr];
    p_urb      =  p_ch_info->URB_Ptr;
    p_err      =  USBH_ERR_NONE;
    halt_ch    =  DEF_FALSE;
                                                                /* ---------- SUCCESSFUL TRANSFER COMPLETION ---------- */
    if ((hcint_reg & REG_HCINTx_XFRC) != 0u) {

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_XFRC;            /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_XFER_CMPL;
        halt_ch                   = DEF_TRUE;
                                                                /* Mask ACK int.                                        */
        DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_ACK);

    } else if ((hcint_reg & REG_HCINTx_STALL) != 0u) {          /* STALL Response Received Interrupt                    */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_STALL;           /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_STALL;
        halt_ch                   = DEF_TRUE;

    } else if ((hcint_reg & REG_HCINTx_NAK) != 0u) {            /* NAK Response Received Interrupt                      */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_NAK;             /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_NAK;
        p_ch_info->CurXferErrCnt  = 0u;                         /* Reset err cnt.                                       */
        halt_ch                   = DEF_TRUE;

    } else if ((hcint_reg & REG_HCINTx_TXERR) != 0u) {          /* Transaction Error. See Note #2                       */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_TXERR;           /* Ack int.                                             */
        halt_ch                   = DEF_TRUE;

        p_ch_info->CurXferErrCnt++;                             /* Increment err cnt in case of transaction err.        */
        if (p_ch_info->CurXferErrCnt < 3u) {
            p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_NAK;       /* Notify core to retry xfer.                           */
        } else {
            p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_TXERR;     /* Notify core to abort xfer.                           */
        }
                                                                /* Unmask ACK int.                                      */
        DEF_BIT_SET(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_ACK);

    } else if ((hcint_reg & REG_HCINTx_BBERR) != 0u) {          /* Babble Error                                         */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_BBERR;           /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_BBERR;
        halt_ch                   = DEF_TRUE;

    } else if ((hcint_reg & REG_HCINTx_DTERR) != 0u) {          /* Data Toggle Error.                                   */

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_DTERR;           /* Ack int.                                             */
        p_ch_info->HaltSrc        = REG_HCINTx_HALT_SRC_DTERR;
        halt_ch                   = DEF_TRUE;
                                                                /* ------------------ CHANNEL HALTED ------------------ */
    }  else if ((hcint_reg & REG_HCINTx_CH_HALTED)!= 0u) {

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_CH_HALTED;       /* Ack int.                                             */
                                                                /* Mask ch halted int.                                  */
        DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_CH_HALTED);

        if ((p_ch_info->Aborted == DEF_TRUE) &&                 /* See Note #3.                                         */
            (p_ch_info->HaltSrc != REG_HCINTx_HALT_SRC_ABORT)) {

            p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_NONE;
        }


        switch (p_ch_info->HaltSrc) {
            case REG_HCINTx_HALT_SRC_XFER_CMPL:                 /* Successful xfer completion.                          */
                 if (ep_type == USBH_EP_TYPE_BULK) {            /* Store last data PID of the last xfer's transaction   */
                     p_ch_info->DataTgl = (data_pid ^ REG_PID_DATA1);
                 }

                 p_ch_info->CurXferErrCnt = 0u;                 /* Reset err cnt.                                       */
                 p_ch_info->EP_PktCnt     = 0u;
                 p_urb->Err               = USBH_ERR_NONE;

                 if ((p_urb->XferLen       < p_urb->UserBufLen) &&
                     (p_ch_info->EP_PktCnt > 0u)) {             /* Receive more data data                               */

                     USBH_STM32FX_HCD_URB_Submit(p_hc_drv,      /* Retransmit the URB on the channel                    */
                                                 p_urb,
                                                &p_err);
                 } else {
                     USBH_URB_Done(p_urb);                      /* Notify the Core layer about the URB completion       */
                     p_ch_info->URB_Ptr = (USBH_URB *)0;
                 }
                 break;


            case REG_HCINTx_HALT_SRC_STALL:
                 p_ch_info->CurXferErrCnt = 0u;                 /* Reset err cnt.                                       */
                 p_urb->Err               = USBH_ERR_EP_STALL;
                 p_urb->XferLen           = 0u;

                 USBH_URB_Done(p_urb);                          /* Notify the Core layer about the STALL condition      */
                 p_ch_info->URB_Ptr = (USBH_URB *)0;
                 break;


            case REG_HCINTx_HALT_SRC_TXERR:
                 p_ch_info->CurXferErrCnt = 0u;                 /* Reset err cnt.                                       */
                 p_urb->Err               = USBH_ERR_HC_IO;
                 p_urb->XferLen           = 0u;

                 USBH_URB_Done(p_urb);                          /* Notify the Core layer about the URB abort            */
                 p_ch_info->URB_Ptr = (USBH_URB *)0;
                 break;


            case REG_HCINTx_HALT_SRC_BBERR:
                 p_ch_info->CurXferErrCnt = 0u;                 /* Reset err cnt.                                       */
                 p_urb->Err               = USBH_ERR_HC_IO;
                 p_urb->XferLen           = 0u;

                 USBH_URB_Done(p_urb);                          /* Notify the Core layer about the babble condition     */
                 p_ch_info->URB_Ptr = (USBH_URB *)0;
                 break;


            case REG_HCINTx_HALT_SRC_DTERR:
            case REG_HCINTx_HALT_SRC_NAK:                       /* Retransmit xfer on channel (see Note #4).            */
                 ep_type = USBH_EP_TypeGet(p_urb->EP_Ptr);
                 if ((ep_type == USBH_EP_TYPE_BULK) ||
                     (ep_type == USBH_EP_TYPE_CTRL)) {

                     p_ch_info->CurDataTgl = data_pid;          /* Data toggle not changed for retransmission.          */
                     p_urb->Err            = USBH_ERR_EP_NACK;

                 } else if (ep_type == USBH_EP_TYPE_INTR) {
                     p_ch_info->DataTgl ^= REG_PID_DATA1;       /* Data Toggle Intr IN                                  */
                 }

                 USBH_STM32FX_HCD_URB_Submit(p_hc_drv,
                                             p_urb,
                                            &p_err);
                 if (p_err != USBH_ERR_NONE) {
                     p_urb->Err     = USBH_ERR_HC_IO;
                     p_urb->XferLen = 0u;

                     USBH_URB_Done(p_urb);                      /* Notify the Core layer about the URB abort            */
                     p_ch_info->URB_Ptr       = (USBH_URB *)0;
                     p_ch_info->CurXferErrCnt =  0u;            /* Reset error count                                    */
                 }
                 break;


            case REG_HCINTx_HALT_SRC_ABORT:
            case REG_HCINTx_HALT_SRC_NONE:
            default:
                 break;
        }

        p_ch_info->Halting = DEF_FALSE;
        p_ch_info->HaltSrc = REG_HCINTx_HALT_SRC_NONE;
        p_ch_info->Aborted = DEF_FALSE;
                                                                /* ------ ACK RESPONSE RECEIVED/TRANSMITTED INT ------- */
    } else if ((hcint_reg & REG_HCINTx_ACK) != 0u) {

        p_reg->HCH[ch_nbr].HCINTx = REG_HCINTx_ACK;             /* Ack int.                                             */
        p_ch_info->CurXferErrCnt  = 0u;                         /* Reset err cnt.                                       */
                                                                /* Mask ACK int.                                        */
        DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_ACK);
    }

    if (halt_ch == DEF_TRUE) {
        p_ch_info->Halting = DEF_TRUE;
        STM32FX_ChHalt(p_reg, ch_nbr);                          /* Halt (i.e. disable) the channel.                     */
    }
}


/*
*********************************************************************************************************
*                                           STM32FX_ChHalt()
*
* Description : Halt channel
*
* Argument(s) : p_reg         Pointer to STM32Fx registers structure.
*
*               ch_nbr        Number of host channel to halt
*
* Return(s)   : None.
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  STM32FX_ChHalt (USBH_STM32FX_REG  *p_reg,
                              CPU_INT08U         ch_nbr)
{
    CPU_INT32U  tx_q_stat;
    CPU_INT32U  hc_char;
    CPU_INT32U  ep_type;
    CPU_INT32U  tx_q_space_avail;


    hc_char =  p_reg->HCH[ch_nbr].HCCHARx;
    DEF_BIT_SET(hc_char, (REG_HCCHARx_CHENA | REG_HCCHARx_CHDIS));
    ep_type = (hc_char & 0x000C0000u) >> 18u;                   /* bits [19:18] Endpoint Type                           */

                                                                /* Check for space in request queue to issue the halt.  */
    if (ep_type == USBH_EP_TYPE_CTRL || ep_type == USBH_EP_TYPE_BULK) {
        tx_q_stat        =  p_reg->HNPTXSTS;                    /* Read non-periodic tx request queue status            */
        tx_q_space_avail = (tx_q_stat & 0x00FF0000u) >> 16u;    /* Non-periodic tx Request Queue Space Available        */
        if (tx_q_space_avail == 0u) {
            DEF_BIT_CLR(hc_char,REG_HCCHARx_CHENA);
        }

    } else {
        tx_q_stat        =  p_reg->HPTXSTS;                     /* Read periodic tx request queue status                */
        tx_q_space_avail = (tx_q_stat & 0x00FF0000u) >> 16u;    /* Periodic Transmit Request Queue Space Available      */
        if (tx_q_space_avail == 0u) {
            DEF_BIT_CLR(hc_char, REG_HCCHARx_CHENA);
        }
    }

                                                                /* Unmask the Channel Halted interrupt                  */
    DEF_BIT_SET(p_reg->HCH[ch_nbr].HCINTMSKx, REG_HCINTx_CH_HALTED);
    p_reg->HCH[ch_nbr].HCCHARx = hc_char;
}

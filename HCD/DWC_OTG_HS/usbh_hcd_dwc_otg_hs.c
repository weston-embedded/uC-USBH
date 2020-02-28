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
*                                       HOST CONTROLLER DRIVER
*                              SYNOPSYS DESIGNWARE CORE USB 2.0 OTG (HS)
*
* File    : usbh_hcd_dwc_otg_hs.c
* Version : V3.42.00
*********************************************************************************************************
* Note(s) : (1) With an appropriate BSP, this host driver will support the OTG_FS host module on  the
*               STMicroelectronics STM32F7xx MCUs, this applies to the following:
*
*                         STM32F74xxx series.
*                         STM32F75xxx series.
*
*           (2) Driver DOES NOT support Isochronous Endpoints.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define  USBH_HCD_DWC_OTG_HS_MODULE
#include  "usbh_hcd_dwc_otg_hs.h"
#include  "../../Source/usbh_hub.h"
#include  <Source/os.h>


/*
*********************************************************************************************************
*                                        REGISTER BIT DEFINES
*********************************************************************************************************
*/
                                                                /* -------------- AHB CONFIGURATION REG --------------- */
#define  DWCOTGHS_GAHBCFG_GINTMSK                  DEF_BIT_00   /* Global interrupt mask.                               */
#define  DWCOTGHS_GAHBCFG_DMAEN                    DEF_BIT_05   /* DMA enabled for USB OTG HS                           */
#define  DWCOTGHS_GAHBCFG_HBSTLEN_MSK              DEF_BIT_FIELD(4u, 1u)
#define  DWCOTGHS_GAHBCFG_HBSTLEN_INCR4           (3u << 1u)    /* Bus transactions target  4x 32-bit accesses          */
#define  DWCOTGHS_GAHBCFG_HBSTLEN_INCR8           (5u << 1u)    /* Bus transactions target  8x 32-bit accesses          */
#define  DWCOTGHS_GAHBCFG_HBSTLEN_INCR16          (7u << 1u)    /* Bus transactions target 16x 32-bit accesses          */
#define  DWCOTGHS_GAHBCFG_TXFELVL                  DEF_BIT_07   /* Non-periodic TxFIFO empty level.                     */
#define  DWCOTGHS_GAHBCFG_PTXFELVL                 DEF_BIT_08   /* Periodic TxFIFO empty level.                         */

                                                                /* ---------------- CORE INTERRUPT REG ---------------- */
                                                                /* ---------------- INTERRUPT MASK REG ---------------- */
#define  DWCOTGHS_GINTx_CMOD                       DEF_BIT_00   /* Current mode of operation interrupt.                 */
#define  DWCOTGHS_GINTx_OTGINT                     DEF_BIT_02   /* OTG interrupt.                                       */
#define  DWCOTGHS_GINTx_SOF                        DEF_BIT_03   /* Start of frame interrupt.                            */
#define  DWCOTGHS_GINTx_RXFLVL                     DEF_BIT_04   /* RxFIFO non-empty interrupt.                          */
#define  DWCOTGHS_GINTx_NPTXFE                     DEF_BIT_05   /* Non-periodic TxFIFO empty interrupt.                 */
#define  DWCOTGHS_GINTx_IPXFR                      DEF_BIT_21   /* Incomplete periodic transfer interrupt.              */
#define  DWCOTGHS_GINTx_HPRTINT                    DEF_BIT_24   /* Host port interrupt.                                 */
#define  DWCOTGHS_GINTx_HCINT                      DEF_BIT_25   /* Host Channel interrupt.                              */
#define  DWCOTGHS_GINTx_PTXFE                      DEF_BIT_26   /* Periodic TxFIFO empty interrupt.                     */
#define  DWCOTGHS_GINTx_CIDSCHGM                   DEF_BIT_28   /* Connector ID status change mask.                     */
#define  DWCOTGHS_GINTx_DISCINT                    DEF_BIT_29   /* Disconnect detected interrupt.                       */
#define  DWCOTGHS_GINTx_SRQINT                     DEF_BIT_30   /* Session request/new session detected interrupt mask. */
#define  DWCOTGHS_GINTx_WKUPINT                    DEF_BIT_31   /* Resume/remote wakeup detected interrupt mask.        */

                                                                /* -------------------- RESET REG --------------------- */
#define  DWCOTGHS_GRSTCTL_CSRST                    DEF_BIT_00   /* Core soft reset.                                     */
#define  DWCOTGHS_GRSTCTL_PSRST                    DEF_BIT_01   /* HCLK soft reset.                                     */
#define  DWCOTGHS_GRSTCTL_RXFFLSH                  DEF_BIT_04   /* RxFIFO flush.                                        */
#define  DWCOTGHS_GRSTCTL_TXFFLSH                  DEF_BIT_05   /* TxFIFO flush.                                        */
#define  DWCOTGHS_GRSTCTL_AHBIDL                   DEF_BIT_31   /* AHB master idle.                                     */
#define  DWCOTGHS_GRSTCTL_NONPER_TXFIFO              0x00u      /* Non-periodic Tx FIFO flush in host mode.             */
#define  DWCOTGHS_GRSTCTL_PER_TXFIFO                 0x01u      /* Periodic Tx FIFO flush in host mode.                 */
#define  DWCOTGHS_GRSTCTL_ALL_TXFIFO                 0x10u      /* Flush all the transmit FIFOs in host mode.           */

                                                                /* ---------- RECEIVE STATUS DEBUG READ REG ----------- */
                                                                /* ----------- OTG STATUS READ AND POP REG ------------ */
                                                                /* bits [3:0] Channel number.                           */
#define  DWCOTGHS_GRXSTS_CHNUM_MSK                 DEF_BIT_FIELD(4u, 0u)
                                                                /* bits [14:4] Byte count.                              */
#define  DWCOTGHS_GRXSTS_BCNT_MSK                  DEF_BIT_FIELD(11u, 4u)
                                                                /* bits [16:15] Data PID                                */
#define  DWCOTGHS_GRXSTS_DPID_MSK                  DEF_BIT_FIELD(2u, 15u)
#define  DWCOTGHS_GRXSTS_DPID_DATA0                     0u
#define  DWCOTGHS_GRXSTS_DPID_DATA1                     2u
#define  DWCOTGHS_GRXSTS_DPID_DATA2                     1u
#define  DWCOTGHS_GRXSTS_DPID_MDATA                     3u
#define  DWCOTGHS_DPID_NONE                          0xFFu
                                                                /* bits [20:17] Packet status.                          */
#define  DWCOTGHS_GRXSTS_PKTSTS_MSK                DEF_BIT_FIELD(4u, 17u)
#define  DWCOTGHS_GRXSTS_PKTSTS_IN                      2u      /* IN data packet received.                             */
#define  DWCOTGHS_GRXSTS_PKTSTS_IN_XFER_COMP            3u      /* IN transfer completed(triggers an interrupt).        */
#define  DWCOTGHS_GRXSTS_PKTSTS_DATA_TGL_ERR            5u      /* Data toggle error(triggers an interrupt).            */
#define  DWCOTGHS_GRXSTS_PKTSTS_CH_HALTED               7u      /* Channel halted(triggers an interrupt).               */

                                                                /* --- NON-PERIODIC TRANSMIT FIFO/QUEUE STATUS REG ---- */
#define  DWCOTGHS_HNPTXSTS_NPTXFSAV_MSK            DEF_BIT_FIELD(16u, 0u)
#define  DWCOTGHS_HNPTXSTS_NPTQXSAV_MSK            DEF_BIT_FIELD(8u, 16u)

                                                                /* ---------- GENERAL CORE CONFIGURATION REG ---------- */
                                                                /* bits [2:0] HS timeout calibration                    */
#define  DWCOTGHS_GCCFG_TOCAL_MSK                  DEF_BIT_FIELD(3u, 0u)
#define  DWCOTGHS_GCCFG_TOCAL_VAL                       7u
#define  DWCOTGHS_GCCFG_PWRDWN                     DEF_BIT_16   /* Power down.                                          */
#define  DWCOTGHS_GCCFG_DCDEN                      DEF_BIT_18   /* Data contact dectection(DCD) mode enable.            */
#define  DWCOTGHS_GCCFG_VBDEN                      DEF_BIT_21   /* This bit is use for STM32F74xx & STM32F75xx driver.  */

                                                                /* -------------- USB CONFIGURATION REG --------------- */
#define  DWCOTGHS_GUSBCFG_TOCAL_MSK                DEF_BIT_FIELD(3u, 0u)
#define  DWCOTGHS_GUSBCFG_TOCAL_VAL                     7u
#define  DWCOTGHS_GUSBCFG_PHYSEL                   DEF_BIT_06   /* Full speed serial transceiver select for USB OTG HS. */
#define  DWCOTGHS_GUSBCFG_SRPCAP                   DEF_BIT_08   /* SRP-capable.                                         */
#define  DWCOTGHS_GUSBCFG_HNPCAP                   DEF_BIT_09   /* HNP-capable.                                         */
#define  DWCOTGHS_GUSBCFG_TRDT_MSK                 DEF_BIT_FIELD(4u, 10u)
#define  DWCOTGHS_GUSBCFG_TRDT_VAL                 (9u << 10u)  /* USB turnaround time in PHY clocks                    */
#define  DWCOTGHS_GUSBCFG_ULPIFSLS                 DEF_BIT_17   /* ULPI FS/LS select for USB OTG HS.                    */
#define  DWCOTGHS_GUSBCFG_ULPICSM                  DEF_BIT_19   /* ULPI clock SuspendM for USB OTG HS.                  */
#define  DWCOTGHS_GUSBCFG_ULPIEVBUSD               DEF_BIT_20   /* ULPI External VBUS Drive for USB OTG HS.             */
#define  DWCOTGHS_GUSBCFG_ULPIEVBUSI               DEF_BIT_21   /* ULPI external VBUS indicator for USB OTG HS          */
#define  DWCOTGHS_GUSBCFG_TSDPS                    DEF_BIT_22   /* TermSel DLine pulsing selection for USB OTG HS       */
#define  DWCOTGHS_GUSBCFG_FHMOD                    DEF_BIT_29   /* Force host mode.                                     */
#define  DWCOTGHS_GUSBCFG_FDMOD                    DEF_BIT_30   /* Force device mode.                                   */

                                                                /* -------------- HOST CONFIGURATION REG -------------- */
#define  DWCOTGHS_HCFG_FSLS_PHYCLK_SEL_48MHz            1u      /* Select 48MHz PHY clock frequency.                    */
#define  DWCOTGHS_HCFG_FSLS_PHYCLK_SEL_6MHz             2u      /* Select  6MHz PHY clock frequency.                    */
#define  DWCOTGHS_HCFG_FSLS_SUPPORT                DEF_BIT_02   /* FS- and LS-only support.                             */
#define  DWCOTGHS_HCFG_FSLS_PHYCLK_SEL_MSK         DEF_BIT_FIELD(2u, 0u)

                                                                /* -------------- HOST FRAME NUMBER REG --------------- */
#define  DWCOTGHS_HFNUM_FRNUM_MSK                  DEF_BIT_FIELD(16u, 0u)
#define  DWCOTGHS_HFNUM_UFRNUM_MSK                 0x00000003u
#define  DWCOTGHS_HFNUM_FRNUM_SHIFT                0x00000003u

                                                                /* --- HOST PERIODIC TRANSMIT FIFO/QUEUE STATUS REG --- */
#define  DWCOTGHS_HPTXSTS_PTXFSAVL_MSK             DEF_BIT_FIELD(16u, 0u)
#define  DWCOTGHS_HPTXSTS_PTQXSAV_MSK              DEF_BIT_FIELD(8u, 16u)
#define  DWCOTGHS_HPTXSTS_REQTYPE                  DEF_BIT_FIELD(2u, 25u)
#define  DWCOTGHS_HPTXSTS_REQTYPE_IN_OUT                0u
#define  DWCOTGHS_HPTXSTS_REQTYPE_ZLP                   1u
#define  DWCOTGHS_HPTXSTS_REQTYPE_DIS_CH_CMD            3u
#define  DWCOTGHS_HPTXSTS_CH_NBR                   DEF_BIT_FIELD(4u, 27u)

                                                                /* --------- HOST ALL CHANNELS INTERRUPT REG ---------- */
#define  DWCOTGHS_HAINT_CH_INT_MSK                 DEF_BIT_FIELD(16u, 0u)

                                                                /* --------- HOST PORT CONTROL AND STATUS REG --------- */
#define  DWCOTGHS_HPRT_PORT_CONN_STS               DEF_BIT_00   /* Port connect status.                                 */
#define  DWCOTGHS_HPRT_PORT_CONN_DET               DEF_BIT_01   /* Port connect detected.                               */
#define  DWCOTGHS_HPRT_PORT_EN                     DEF_BIT_02   /* Port enable.                                         */
#define  DWCOTGHS_HPRT_PORT_ENCHNG                 DEF_BIT_03   /* Port enable/disable change.                          */
#define  DWCOTGHS_HPRT_PORT_OCA                    DEF_BIT_04   /* Port overcurrent active.                             */
#define  DWCOTGHS_HPRT_PORT_OCCHNG                 DEF_BIT_05   /* Port overcurrent change.                             */
#define  DWCOTGHS_HPRT_PORT_RES                    DEF_BIT_06   /* Port resume.                                         */
#define  DWCOTGHS_HPRT_PORT_SUSP                   DEF_BIT_07   /* Port suspend.                                        */
#define  DWCOTGHS_HPRT_PORT_RST                    DEF_BIT_08   /* Port reset.                                          */
#define  DWCOTGHS_HPRT_PORT_PWR                    DEF_BIT_12   /* Port power.                                          */
#define  DWCOTGHS_HPRT_PORT_SPD_MSK                DEF_BIT_FIELD(2u, 17u)
#define  DWCOTGHS_HPRT_PORT_SPD_HS                 0x00000000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  DWCOTGHS_HPRT_PORT_SPD_FS                 0x00020000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  DWCOTGHS_HPRT_PORT_SPD_LS                 0x00040000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  DWCOTGHS_HPRT_PORT_SPD_NO_SPEED           0x00060000u  /* Port Speed (PrtSpd): bits 17-18                      */
#define  DWCOTGHS_HPRT_PORT_NOT_IN_RESET                0u      /* Port Reset (PrtSpd): bit 8                           */
#define  DWCOTGHS_HPRT_PORT_IN_RESET                    1u      /* Port Reset (PrtSpd): bit 8                           */
#define  DWCOTGHS_HPRT_PORT_NOT_IN_SUSPEND_MODE         0u      /* Port Suspend (PrtSusp): bit 7                        */
#define  DWCOTGHS_HPRT_PORT_IN_SUSPEND_MODE             1u      /* Port Suspend (PrtSusp): bit 7                        */
                                                                /* Macro to avoid clearing some bits generating int.    */
#define  DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg)      DEF_BIT_CLR((reg), (DWCOTGHS_HPRT_PORT_EN       |    \
                                                                       DWCOTGHS_HPRT_PORT_CONN_DET |    \
                                                                       DWCOTGHS_HPRT_PORT_ENCHNG   |    \
                                                                       DWCOTGHS_HPRT_PORT_OCCHNG));     \

                                                                /* -------- HOST CHANNEL-X CHARACTERISTICS REG -------- */
                                                                /* bits [10:0] Maximum packet size                      */
#define  DWCOTGHS_HCCHARx_MPSIZ_MSK                DEF_BIT_FIELD(11u, 0u)
                                                                /* bits [14:11] Endpoint Number                         */
#define  DWCOTGHS_HCCHARx_EPNUM_MSK                DEF_BIT_FIELD(4u, 11u)
#define  DWCOTGHS_HCCHARx_EPDIR                    DEF_BIT_15   /* Endpoint direction.                                  */
#define  DWCOTGHS_HCCHARx_LSDEV                    DEF_BIT_17   /* Low speed device.                                    */
#define  DWCOTGHS_HCCHARx_MCNT                     DEF_BIT_FIELD(2u, 20u)
#define  DWCOTGHS_HCCHARx_MCNT_1_TRXN                   1u      /* 1 transaction.                                       */
#define  DWCOTGHS_HCCHARx_MCNT_2_TRXN                   2u      /* 2 transaction per frame to be issued.                */
#define  DWCOTGHS_HCCHARx_MCNT_3_TRXN                   3u      /* 3 transaction per frame to be issued.                */
                                                                /* bits [28:22] Device Address.                         */
#define  DWCOTGHS_HCCHARx_DAD_MSK                  DEF_BIT_FIELD(7u, 22u)
#define  DWCOTGHS_HCCHARx_ODDFRM                   DEF_BIT_29   /* Odd frame.                                           */
#define  DWCOTGHS_HCCHARx_CHDIS                    DEF_BIT_30   /* Channel disable.                                     */
#define  DWCOTGHS_HCCHARx_CHENA                    DEF_BIT_31   /* Channel enable.                                      */
                                                                /* bits [19:18] Endpoint Type                           */
#define  DWCOTGHS_HCCHARx_EPTYP_MSK                DEF_BIT_FIELD(2u, 18u)
#define  DWCOTGHS_HCCHARx_EPTYP_CTRL                    0u
#define  DWCOTGHS_HCCHARx_EPTYP_ISOC                    1u
#define  DWCOTGHS_HCCHARx_EPTYP_BULK                    2u
#define  DWCOTGHS_HCCHARx_EPTYP_INTR                    3u

                                                                /* ---------- HOST CHANNEL-X SPLIT CTRL REG ----------- */
                                                                /* bits [6:0] port address  mask                        */
#define  DWCOTGHS_HCSPLTx_PRTADDR_MSK              DEF_BIT_FIELD(7u, 0u)
                                                                /* bits [13:7] HUB address  mask                        */
#define  DWCOTGHS_HCSPLTx_HUBADDR_MSK              DEF_BIT_FIELD(7u, 7u)
                                                                /* bits [15:14] transaction position mask               */
#define  DWCOTGHS_HCSPLTx_XACTPOS_MSK              DEF_BIT_FIELD(2u, 14u)
#define  DWCOTGHS_HCSPLTx_XACTPOS_ALL                   3u      /* Send all    payloads with each OUT transaction.      */
#define  DWCOTGHS_HCSPLTx_XACTPOS_BEGIN                 2u      /* Send first  payloads with each OUT transaction.      */
#define  DWCOTGHS_HCSPLTx_XACTPOS_MID                   0u      /* Send middle payloads with each OUT transaction.      */
#define  DWCOTGHS_HCSPLTx_XACTPOS_END                   1u      /* Send last   payloads with each OUT transaction.      */
#define  DWCOTGHS_HCSPLTx_COMPLSPLT                DEF_BIT_16   /* Do complete split.                                   */
#define  DWCOTGHS_HCSPLTx_SPLITEN                  DEF_BIT_31   /* Split enable.                                        */


                                                                /* ----------- HOST CHANNEL-X INTERRUPT REG ----------- */
                                                                /* bits [10:0] HCINT interrupt mask                     */
#define  DWCOTGHS_HCINTx_MSK                       DEF_BIT_FIELD(11u, 0u)
#define  DWCOTGHS_HCINTx_XFRC                      DEF_BIT_00   /* transfer complete interrupt.                         */
#define  DWCOTGHS_HCINTx_CHH                       DEF_BIT_01   /* Channel halted interrupt.                            */
#define  DWCOTGHS_HCINTx_AHBERR                    DEF_BIT_02   /* AHB err for USB OTG HS.                              */
#define  DWCOTGHS_HCINTx_STALL                     DEF_BIT_03   /* STALL response received interrupt.                   */
#define  DWCOTGHS_HCINTx_NAK                       DEF_BIT_04   /* NAK response received interrupt.                     */
#define  DWCOTGHS_HCINTx_ACK                       DEF_BIT_05   /* ACK response received/transmitted interrupt.         */
#define  DWCOTGHS_HCINTx_NYET                      DEF_BIT_06   /* Not yet ready response received intr for USB OTG HS  */
#define  DWCOTGHS_HCINTx_TXERR                     DEF_BIT_07   /* Transaction error interrupt.                         */
#define  DWCOTGHS_HCINTx_BBERR                     DEF_BIT_08   /* Babble error interrupt.                              */
#define  DWCOTGHS_HCINTx_FRMOR                     DEF_BIT_09   /* Frame overrun.                                       */
#define  DWCOTGHS_HCINTx_DTERR                     DEF_BIT_10   /* Data toggle error interrupt.                         */

                                                                /* --------- HOST CHANNEL-X TRANSFER SIZE REG --------- */
#define  DWCOTGHS_HCTSIZx_DOPING                   DEF_BIT_31
                                                                /* bits [18:0] Transfer size                            */
#define  DWCOTGHS_HCTSIZx_XFRSIZ_MSK               DEF_BIT_FIELD(19u, 0u)
#define  DWCOTGHS_HCTSIZx_XFRSIZ_MAX                 1023u      /* Max transfer Transfer size                           */
                                                                /* bits [30:29] Data PID                                */
#define  DWCOTGHS_HCTSIZx_DPID_MSK                 DEF_BIT_FIELD(2u, 29u)
                                                                /* bits [28:19] Packet Count                            */
#define  DWCOTGHS_HCTSIZx_PKTCNT_MSK               DEF_BIT_FIELD(10u, 19u)

#define  DWCOTGHS_DRV_DEBUG                        DEF_DISABLED


/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/

#define  DWCOTGHS_GET_XFRSIZ(reg)                ((reg & DWCOTGHS_HCTSIZx_XFRSIZ_MSK) >>  0u)
#define  DWCOTGHS_GET_PKTCNT(reg)                ((reg & DWCOTGHS_HCTSIZx_PKTCNT_MSK) >> 19u)
#define  DWCOTGHS_GET_DPID(reg)                  ((reg & DWCOTGHS_HCTSIZx_DPID_MSK  ) >> 29u)

#define  DWCOTGHS_CH_EN(temp, reg)               {                                              \
                                                     temp           = (reg).HCCHARx;            \
                                                     temp          &= ~DWCOTGHS_HCCHARx_CHDIS;  \
                                                     temp          |=  DWCOTGHS_HCCHARx_CHENA;  \
                                                     (reg).HCCHARx  =  temp;                    \
                                                 }                                              \


/*
**************************************************************************************************************
*                                            LOCAL DEFINES
*
*   Notes:  (1) Allocation of data RAM for Endpoint FIFOs. The OTG-HS controller has a dedicated SPRAM for
*               EP data of 4 Kbytes = 4096 bytes = 1024 32-bit words available for the channels IN and OUT.
*               Host mode features:
*               - 16 host channels (pipes)
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

#define  DRV_STM32FX_OTG_HS                                0u

#define  DWCOTGHS_MAX_NBR_CH                               8u   /* Maximum number of host channels                      */
#define  OTGHS_MAX_NBR_CH                                 16u   /* Maximum number of host channels for STM32F7          */
#define  DWCOTGHS_DFIFO_SIZE                            1024u   /* Number of entries                                    */
#define  DWCOTGHS_INVALID_CH                            0xFFu   /* Invalid channel number.                              */
#define  DWCOTGHS_DFLT_EP_ADDR                        0xFFFFu   /* Default endpoint address.                            */


#define  DWCOTGHS_MAX_RETRY                            10000u   /* Maximum number of retries                            */

                                                                /* See Note #1. FIFO depth is specified in 32-bit words */
#define  DWCOTGHS_RXFIFO_START_ADDR                        0u
#define  DWCOTGHS_RXFIFO_DEPTH                           512u   /* For all IN channels. See Note #2                     */
#define  DWCOTGHS_NONPER_CH_TXFIFO_DEPTH                 256u   /* For all Non-Periodic channels. See Note #3           */
#define  DWCOTGHS_NONPER_CH_TXFIFO_START_ADDR   (DWCOTGHS_RXFIFO_DEPTH)
#define  DWCOTGHS_PER_CH_TXFIFO_DEPTH                    256u   /* For all Periodic channels. See Note #4               */
#define  DWCOTGHS_PER_CH_TXFIFO_START_ADDR      (DWCOTGHS_NONPER_CH_TXFIFO_DEPTH +     \
                                                 DWCOTGHS_NONPER_CH_TXFIFO_START_ADDR)

#define  FRAME_MAX_VALUE                                   8u

                                                                /* If necessary, re-define these values in 'usbh_cfg.h' */
#ifndef  DWCOTGHS_URB_PROC_TASK_STK_SIZE
#define  DWCOTGHS_URB_PROC_TASK_STK_SIZE                 256u
#endif

#ifndef  DWCOTGHS_URB_PROC_TASK_PRIO
#define  DWCOTGHS_URB_PROC_TASK_PRIO                      15u
#endif

#ifndef  DWCOTGHS_URB_PROC_Q_MAX
#define  DWCOTGHS_URB_PROC_Q_MAX                          10u
#endif

#if     (OS_VERSION < 30000u)
#if     (OS_TMR_EN < 1)
#error  "OS_TMR_EN must be equal to 1"
#endif

#if     (OS_TICKS_PER_SEC != 1000u)
#error  "OS_TICKS_PER_SEC must be equal to 1000"
#endif

#if     (OS_TMR_CFG_TICKS_PER_SEC != OS_TICKS_PER_SEC)
#error  "OS_TMR_CFG_TICKS_PER_SEC must be equal to OS_TICKS_PER_SEC"
#endif

#else
#if     (OS_CFG_TMR_EN != DEF_ENABLED)
#error  "OS_CFG_TMR_EN must be equal to DEF_ENABLED"
#endif

#if     (OS_CFG_TICK_RATE_HZ != 1000u)
#error  "OS_CFG_TICK_RATE_HZ must be equal to 1000"
#endif

#if     (OS_CFG_TMR_TASK_RATE_HZ != OS_CFG_TICK_RATE_HZ)
#error  "OS_CFG_TMR_TASK_RATE_HZ must be equal to OS_TICKS_PER_SEC"
#endif
#endif


/*
*********************************************************************************************************
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  struct  usbh_dwcotghs_host_ch_reg {                    /* ------------- HOST CHANNEL-X DATA TYPE ------------- */
    CPU_REG32                  HCCHARx;                         /* Host channel-x characteristics                       */
    CPU_REG32                  HCSPLTx;                         /* Host channel-x split control                         */
    CPU_REG32                  HCINTx;                          /* Host channel-x interrupt                             */
    CPU_REG32                  HCINTMSKx;                       /* Host channel-x interrupt mask                        */
    CPU_REG32                  HCTSIZx;                         /* Host channel-x transfer size                         */
    CPU_REG32                  HCDMAx;                          /* Host channel-x DMA address                           */
    CPU_REG32                  RSVD1[2u];
} USBH_DWCOTGHS_HOST_CH_REG;


typedef  struct  usbh_dwcotghs_dfifo_reg {                      /* ---------- DATA FIFO ACCESS DATA TYPE -------------- */
    CPU_REG32                  DATA[DWCOTGHS_DFIFO_SIZE];       /* 4K bytes per endpoint                                */
} USBH_DWCOTGHS_DFIFO_REG;


typedef  struct  usbh_dwcotghs_reg {
                                                                /* ----- CORE GLOBAL CONTROL AND STATUS REGISTERS ----- */
    CPU_REG32                  GOTGCTL;                         /* 0x0000 Core control and status                       */
    CPU_REG32                  GOTGINT;                         /* 0x0004 Core interrupt                                */
    CPU_REG32                  GAHBCFG;                         /* 0x0008 Core AHB configuration                        */
    CPU_REG32                  GUSBCFG;                         /* 0x000C Core USB configuration                        */
    CPU_REG32                  GRSTCTL;                         /* 0x0010 Core reset                                    */
    CPU_REG32                  GINTSTS;                         /* 0x0014 Core interrupt                                */
    CPU_REG32                  GINTMSK;                         /* 0x0018 Core interrupt mask                           */
    CPU_REG32                  GRXSTSR;                         /* 0x001C Core receive status debug read                */
    CPU_REG32                  GRXSTSP;                         /* 0x0020 Core status read and pop                      */
    CPU_REG32                  GRXFSIZ;                         /* 0x0024 Core receive FIFO size                        */
    CPU_REG32                  HNPTXFSIZ;                       /* 0x0028 Host non-periodic transmit FIFO size          */
    CPU_REG32                  HNPTXSTS;                        /* 0x002C Core Non Periodic Tx FIFO/Queue status        */
    CPU_REG32                  RSVD0[2u];
    CPU_REG32                  GCCFG;                           /* 0x0038 General core configuration                    */
    CPU_REG32                  CID;                             /* 0x003C Core ID register                              */
    CPU_REG32                  RSVD1[48u];
    CPU_REG32                  HPTXFSIZ;                        /* 0x0100 Core Host Periodic Tx FIFO size               */
    CPU_REG32                  RSVD2[191u];
                                                                /* ------ HOST MODE CONTROL AND STATUS REGISTERS ------ */
    CPU_REG32                  HCFG;                            /* 0x0400 Host configuration                            */
    CPU_REG32                  HFIR;                            /* 0x0404 Host frame interval                           */
    CPU_REG32                  HFNUM;                           /* 0x0408 Host frame number/frame time remaining        */
    CPU_REG32                  RSVD3;
    CPU_REG32                  HPTXSTS;                         /* 0x0410 Host periodic transmit FIFO/queue status      */
    CPU_REG32                  HAINT;                           /* 0x0414 Host all channels interrupt                   */
    CPU_REG32                  HAINTMSK;                        /* 0x0418 Host all channels interrupt mask              */
    CPU_REG32                  RSVD4[9u];
    CPU_REG32                  HPRT;                            /* 0x0440 Host port control and status                  */
    CPU_REG32                  RSVD5[47u];
    USBH_DWCOTGHS_HOST_CH_REG  HCH[OTGHS_MAX_NBR_CH];           /* 0x0500 Host Channel-x regiters                       */
    CPU_REG32                  RSVD6[448u];
                                                                /* -- POWER & CLOCK GATING CONTROL & STATUS REGISTER -- */
    CPU_REG32                  PCGCR;                           /* 0x0E00 Power anc clock gating control                */
    CPU_REG32                  RSVD7[127u];
                                                                /* --- DATA FIFO (DFIFO) HOST-CH X ACCESS REGISTERS --- */
    USBH_DWCOTGHS_DFIFO_REG    DFIFO[OTGHS_MAX_NBR_CH];         /* 0x1000 Data FIFO host channel-x access registers     */
} USBH_DWCOTGHS_REG;


/*
*********************************************************************************************************
*                                          DRIVER DATA TYPE
*********************************************************************************************************
*/

typedef  struct  usbh_dwcotghs_ch_info {
    CPU_INT16U             EP_Addr;                             /* Device addr | EP DIR | EP NBR.                       */
    CPU_INT08U             EP_TxErrCnt;
                                                                /* To ensure the URB EP xfer size is not corrupted ...  */
    CPU_INT32U             AppBufLen;                           /* ... for multi-transaction transfer                   */
    USBH_URB              *URB_Ptr;
    CPU_BOOLEAN            DoSplit;                             /* Configure EP split transactions                      */
    CPU_INT32U             NextXferLen;
    USBH_HTMR              Tmr;
    CPU_INT32U             CSPLITCnt;
} USBH_DWCOTGHS_CH_INFO;


typedef  struct  usbh_drv_data {
    CPU_INT08U             RH_Desc[USBH_HUB_LEN_HUB_DESC];      /* RH desc content.                                     */
    CPU_INT08U             ChMaxNbr;                            /* Max number of host channels.                         */
    USBH_DWCOTGHS_CH_INFO  ChInfoTbl[OTGHS_MAX_NBR_CH];         /* Contains information of host channels.               */
    CPU_INT16U             ChUsed;                              /* Bit array for BULK, ISOC, INTR, CTRL channel mgmt.   */
    CPU_INT16U             RH_PortStat;                         /* Root Hub Port status.                                */
    CPU_INT16U             RH_PortChng;                         /* Root Hub Port status change.                         */
    CPU_INT32U             SavedGINTMSK;                        /* Saved masked/unmasked int state in case of...        */
    CPU_INT32U             CSPLITChBmp;
    CPU_INT32U             SOFCtr;                              /* Start of frame counter.                              */
    MEM_POOL               DrvMemPool;                          /* Pool for mem mgmt to keep alignment at the drv level.*/
} USBH_DRV_DATA;


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  volatile  USBH_HQUEUE  DWCOTGHS_URB_Proc_Q;
static            USBH_URB     DWCOTGHS_Q_UrbEp[DWCOTGHS_URB_PROC_Q_MAX];
static            CPU_STK      DWCOTGHS_URB_ProcTaskStk[DWCOTGHS_URB_PROC_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* --------------- DRIVER API FUNCTIONS --------------- */
static  void          USBH_DWCOTGHS_HCD_Init            (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void          USBH_STM32FX_OTG_HS_HCD_Start     (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_StartHandler    (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             drv_type,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_Stop            (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  USBH_DEV_SPD  USBH_DWCOTGHS_HCD_SpdGet          (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_Suspend         (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_Resume          (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  CPU_INT32U    USBH_DWCOTGHS_HCD_FrameNbrGet     (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_EP_Open         (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_EP_Close        (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_EP_Abort        (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_IsHalt_EP       (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_EP               *p_ep,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_URB_Submit      (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_URB              *p_urb,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_URB_Complete    (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_URB              *p_urb,
                                                         USBH_ERR              *p_err);

static  void          USBH_DWCOTGHS_HCD_URB_Abort       (USBH_HC_DRV           *p_hc_drv,
                                                         USBH_URB              *p_urb,
                                                         USBH_ERR              *p_err);

                                                                /* -------------- ROOT HUB API FUNCTIONS -------------- */
static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortStatusGet   (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr,
                                                         USBH_HUB_PORT_STATUS  *p_port_status);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_HubDescGet      (USBH_HC_DRV           *p_hc_drv,
                                                         void                  *p_buf,
                                                         CPU_INT08U             buf_len);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortEnSet       (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortEnClr       (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortEnChngClr   (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortPwrSet      (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortPwrClr      (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortResetSet    (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortResetChngClr(USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortSuspendClr  (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_PortConnChngClr (USBH_HC_DRV           *p_hc_drv,
                                                         CPU_INT08U             port_nbr);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_RHSC_IntEn      (USBH_HC_DRV           *p_hc_drv);

static  CPU_BOOLEAN   USBH_DWCOTGHS_HCD_RHSC_IntDis     (USBH_HC_DRV           *p_hc_drv);


/*
*********************************************************************************************************
*                                        LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void        DWCOTGHS_ISR_Handler    (void                       *p_drv);

static  void        DWCOTGHS_ISR_PortConn   (USBH_DWCOTGHS_REG          *p_reg,
                                             USBH_DRV_DATA              *p_drv_data,
                                             USBH_DEV                   *p_dev);

static  void        DWCOTGHS_ISR_PortDisconn(USBH_DWCOTGHS_REG          *p_reg,
                                             USBH_DRV_DATA              *p_drv_data,
                                             USBH_DEV                   *p_dev);

static  void        DWCOTGHS_ISR_HostChOUT  (USBH_DWCOTGHS_REG          *p_reg,
                                             USBH_DRV_DATA              *p_drv_data,
                                             CPU_INT08U                  ch_nbr);

static  void        DWCOTGHS_ISR_HostChIN   (USBH_DWCOTGHS_REG          *p_reg,
                                             USBH_DRV_DATA              *p_drv_data,
                                             CPU_INT08U                  ch_nbr);

static  void        DWCOTGHS_ChSplitHandler (USBH_DWCOTGHS_REG          *p_reg,
                                             USBH_DRV_DATA              *p_drv_data,
                                             CPU_INT08U                  ch_nbr,
                                             CPU_INT08U                  ep_type);

static  void        DWCOTGHS_ChInit         (USBH_DWCOTGHS_REG          *p_reg,
                                             USBH_URB                   *p_urb,
                                             CPU_INT08U                  ch_nbr,
                                             USBH_ERR                   *p_err);

static  void        DWCOTGHS_ChXferStart    (USBH_DWCOTGHS_REG          *p_reg,
                                             USBH_DWCOTGHS_CH_INFO      *p_ch_info,
                                             USBH_URB                   *p_urb,
                                             CPU_INT08U                  ch_nbr,
                                             USBH_ERR                   *p_err);

static  void        DWCOTGHS_ChEnable       (USBH_DWCOTGHS_HOST_CH_REG  *p_ch_reg,
                                             USBH_URB                   *p_urb);

static  CPU_INT08U  DWCOTGHS_GetChNbr       (USBH_DRV_DATA              *p_drv_data,
                                             USBH_EP                    *p_ep);

static  CPU_INT08U  DWCOTGHS_GetFreeChNbr   (USBH_DRV_DATA              *p_drv_data,
                                             USBH_ERR                   *p_err);

static  void        DWCOTGHS_URB_ProcTask   (void                       *p_arg);

static  void        DWCOTGHS_TmrCallback    (void                       *p_tmr,
                                             void                       *p_arg);


/*
*********************************************************************************************************
*                                    INITIALIZED GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_HC_DRV_API  USBH_STM32FX_OTG_HS_HCD_DrvAPI = {
    USBH_DWCOTGHS_HCD_Init,
    USBH_STM32FX_OTG_HS_HCD_Start,
    USBH_DWCOTGHS_HCD_Stop,
    USBH_DWCOTGHS_HCD_SpdGet,
    USBH_DWCOTGHS_HCD_Suspend,
    USBH_DWCOTGHS_HCD_Resume,
    USBH_DWCOTGHS_HCD_FrameNbrGet,

    USBH_DWCOTGHS_HCD_EP_Open,
    USBH_DWCOTGHS_HCD_EP_Close,
    USBH_DWCOTGHS_HCD_EP_Abort,
    USBH_DWCOTGHS_HCD_IsHalt_EP,

    USBH_DWCOTGHS_HCD_URB_Submit,
    USBH_DWCOTGHS_HCD_URB_Complete,
    USBH_DWCOTGHS_HCD_URB_Abort,
};


USBH_HC_RH_API  USBH_DWCOTGHS_HCD_RH_API = {
    USBH_DWCOTGHS_HCD_PortStatusGet,
    USBH_DWCOTGHS_HCD_HubDescGet,

    USBH_DWCOTGHS_HCD_PortEnSet,
    USBH_DWCOTGHS_HCD_PortEnClr,
    USBH_DWCOTGHS_HCD_PortEnChngClr,

    USBH_DWCOTGHS_HCD_PortPwrSet,
    USBH_DWCOTGHS_HCD_PortPwrClr,

    USBH_DWCOTGHS_HCD_PortResetSet,
    USBH_DWCOTGHS_HCD_PortResetChngClr,

    USBH_DWCOTGHS_HCD_PortSuspendClr,
    USBH_DWCOTGHS_HCD_PortConnChngClr,

    USBH_DWCOTGHS_HCD_RHSC_IntEn,
    USBH_DWCOTGHS_HCD_RHSC_IntDis
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          DRIVER FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      USBH_DWCOTGHS_HCD_Init()
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

static  void  USBH_DWCOTGHS_HCD_Init (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;
    CPU_SIZE_T      octets_reqd;
    USBH_HTASK      htask;
    LIB_ERR         err_lib;


    p_drv_data = (USBH_DRV_DATA *)Mem_HeapAlloc(sizeof(USBH_DRV_DATA),
                                                sizeof(CPU_ALIGN),
                                               &octets_reqd,
                                               &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

    Mem_Clr(p_drv_data, sizeof(USBH_DRV_DATA));

    p_hc_drv->DataPtr = (void *)p_drv_data;

    if ((p_hc_drv->HC_CfgPtr->DataBufMaxLen % 512) != 0u) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }
                                                                /* Create Mem pool area to be used for DMA alignment    */
    Mem_PoolCreate(&p_drv_data->DrvMemPool,
                    DEF_NULL,
                    p_hc_drv->HC_CfgPtr->DataBufMaxLen,
                   (p_hc_drv->HC_CfgPtr->MaxNbrEP_BulkOpen + p_hc_drv->HC_CfgPtr->MaxNbrEP_IntrOpen),
                    p_hc_drv->HC_CfgPtr->DataBufMaxLen,
                    sizeof(CPU_ALIGN),
                   &octets_reqd,
                   &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

    DWCOTGHS_URB_Proc_Q = USBH_OS_MsgQueueCreate((void *)&DWCOTGHS_Q_UrbEp[0u],
                                                          DWCOTGHS_URB_PROC_Q_MAX,
                                                          p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }
                                                                /* Create URB Process task for URB handling.            */
   *p_err = USBH_OS_TaskCreate(              "DWC OTG URB Process",
                                              DWCOTGHS_URB_PROC_TASK_PRIO,
                                             &DWCOTGHS_URB_ProcTask,
                               (void       *) p_hc_drv,
                               (CPU_INT32U *)&DWCOTGHS_URB_ProcTaskStk[0u],
                                              DWCOTGHS_URB_PROC_TASK_STK_SIZE,
                                             &htask);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBH_STM32FX_OTG_HS_HCD_Start()
*
* Description : Start STM32FX_OTG_HS Host Controller.
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

static  void  USBH_STM32FX_OTG_HS_HCD_Start (USBH_HC_DRV  *p_hc_drv,
                                             USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;


    p_drv_data           = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    p_drv_data->ChMaxNbr = OTGHS_MAX_NBR_CH;                    /* Set max. number of supported host channels.          */
    USBH_DWCOTGHS_HCD_StartHandler(p_hc_drv, DRV_STM32FX_OTG_HS, p_err);
}


/*
*********************************************************************************************************
*                                  USBH_DWCOTGHS_HCD_StartHandler()
*
* Description : Start Host Controller.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               drv_type     Driver type:
*                                DRV_STM32FX_OTG_HS      Driver for STM32F74xx and STM32F75xx series.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD start successful.
*                                Specific error code     otherwise.
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

static  void  USBH_DWCOTGHS_HCD_StartHandler (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    drv_type,
                                              USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_HC_BSP_API    *p_bsp_api;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT08U          i;
    CPU_INT32U          reg_to;
    CPU_INT32U          reg_val;


    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    p_bsp_api  =  p_hc_drv->BSP_API_Ptr;

    if (p_bsp_api->Init != (void *)0) {
        p_bsp_api->Init(p_hc_drv, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    DEF_BIT_CLR(p_reg->GAHBCFG, DWCOTGHS_GAHBCFG_GINTMSK);      /* Disable the global interrupt in AHB cfg reg          */
    DEF_BIT_CLR(p_reg->GCCFG,   DWCOTGHS_GCCFG_PWRDWN);         /* Disable USB FS transceiver.                          */

    switch (drv_type) {
        case DRV_STM32FX_OTG_HS:
                                                                /* ------------ INITIALIZE ULPI INTERFACE ------------- */
             DEF_BIT_CLR(p_reg->GUSBCFG, (DWCOTGHS_GUSBCFG_PHYSEL  |    /* PHY = USB 2.0 external ULPI High-speed PHY   */
                                          DWCOTGHS_GUSBCFG_TSDPS   |    /* Data line pulsing using utmi_txvalid         */
                                          DWCOTGHS_GUSBCFG_ULPIFSLS));  /* ULPI interface                               */

             DEF_BIT_CLR(p_reg->GUSBCFG, (DWCOTGHS_GUSBCFG_ULPIEVBUSD |
                                          DWCOTGHS_GUSBCFG_ULPIEVBUSI)); /* PHY uses an internal VBUS valid comparator. */
             p_reg->GUSBCFG |= DWCOTGHS_GUSBCFG_ULPIEVBUSD;     /* PHY driver VBUS using external supply.               */
             break;


        default:
             break;
    }

                                                                /* ------------------ CORE RESET ---------------------- */
    reg_to  = DWCOTGHS_MAX_RETRY;                               /* Check AHB master IDLE state before resetting core    */
    reg_val = p_reg->GRSTCTL;
    while ((DEF_BIT_IS_CLR(reg_val, DWCOTGHS_GRSTCTL_AHBIDL) == DEF_YES) &&
           (reg_to                                            >      0u)) {
        reg_to--;
        reg_val = p_reg->GRSTCTL;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

    DEF_BIT_SET(p_reg->GRSTCTL, DWCOTGHS_GRSTCTL_CSRST);        /* Clr all other ctrl logic. See Note (2)               */
    reg_to  = DWCOTGHS_MAX_RETRY;                               /* Wait all necessary logic is reset in the core        */
    reg_val = p_reg->GRSTCTL;
    while ((DEF_BIT_IS_SET(reg_val, DWCOTGHS_GRSTCTL_CSRST) == DEF_YES) &&
           (reg_to                                           >      0u)) {
        reg_to--;
        reg_val = p_reg->GRSTCTL;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

    p_reg->GAHBCFG |= (DWCOTGHS_GAHBCFG_DMAEN         |         /* The core operates in DMA mode.                       */
                       DWCOTGHS_GAHBCFG_HBSTLEN_INCR4);         /* Bus transactions target 4x 32-bit accesses           */

                                                                /* -------------- INIT HOST MODE OF CORE -------------- */
    DEF_BIT_CLR(p_reg->GUSBCFG,(DWCOTGHS_GUSBCFG_FHMOD | DWCOTGHS_GUSBCFG_FDMOD));
    DEF_BIT_SET(p_reg->GUSBCFG, DWCOTGHS_GUSBCFG_FHMOD);        /* Force the core to be in Host mode                    */
    USBH_OS_DlyMS(50u);                                         /* Wait at least 50ms before core forces the Host mode  */

    DEF_BIT_SET(p_reg->GUSBCFG, DWCOTGHS_GUSBCFG_TOCAL_VAL);    /* Add a few nbr of PHY clk to HS interpacket dly.      */
    p_reg->GUSBCFG &= ~DWCOTGHS_GUSBCFG_TRDT_MSK;
    p_reg->GUSBCFG |=  DWCOTGHS_GUSBCFG_TRDT_VAL;               /* Set USB turnaround time in PHY clocks.               */

    p_reg->PCGCR =  0u;                                         /* Restart the PHY clock                                */
    DEF_BIT_SET(p_reg->GCCFG, DWCOTGHS_GCCFG_VBDEN);            /* VBUS detection enabled.                              */
    DEF_BIT_CLR(p_reg->HCFG, DWCOTGHS_HCFG_FSLS_SUPPORT);       /* Disable FS/LS Support mode only                      */

                                                                /* ---------------- FLUSH ALL TX FIFOs ---------------- */
    p_reg->GRSTCTL = (DWCOTGHS_GRSTCTL_TXFFLSH |
                     (DWCOTGHS_GRSTCTL_ALL_TXFIFO << 6u));
    reg_to         =  DWCOTGHS_MAX_RETRY;                        /* Wait for the complete FIFO flushing                  */
    while ((DEF_BIT_IS_SET(p_reg->GRSTCTL, DWCOTGHS_GRSTCTL_TXFFLSH) == DEF_YES) &&
           (reg_to >  0u))  {
        reg_to--;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }
                                                                /* --------------- FLUSH ENTIRE RX FIFOs --------------- */
    p_reg->GRSTCTL = DWCOTGHS_GRSTCTL_RXFFLSH;
    reg_to         = DWCOTGHS_MAX_RETRY;                         /* Wait for the complete FIFO flushing                  */
    while ((DEF_BIT_IS_SET(p_reg->GRSTCTL, DWCOTGHS_GRSTCTL_RXFFLSH) == DEF_YES) &&
           (reg_to                                                    >      0u))  {
        reg_to--;
    }
    if (reg_to == 0u) {
       *p_err = USBH_ERR_FAIL;
        return;
    }


    for (i = 0u; i < p_drv_data->ChMaxNbr; i++) {               /* Disable and Clear the interrupts for each channel    */
        p_reg->HCH[i].HCINTx    = 0xFFFFFFFFu;
        p_reg->HCH[i].HCINTMSKx = 0u;
                                                                /* Set default channel info fields.                     */
        p_drv_data->ChInfoTbl[i].AppBufLen   = 0u;
        p_drv_data->ChInfoTbl[i].EP_TxErrCnt = 0u;
        p_drv_data->ChInfoTbl[i].EP_Addr     = DWCOTGHS_DFLT_EP_ADDR;
        p_drv_data->ChInfoTbl[i].DoSplit     = DEF_NO;
        p_drv_data->ChInfoTbl[i].CSPLITCnt   = 0u;
    }

                                                                /* --------------- ENABLE VBUS DRIVING ---------------- */
    reg_val = p_reg->HPRT;
    DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg_val);                   /* To avoid clearing some important interrupts          */
    if (DEF_BIT_IS_CLR(reg_val, DWCOTGHS_HPRT_PORT_PWR) == DEF_YES) {
        p_reg->HPRT = (DWCOTGHS_HPRT_PORT_PWR | reg_val);       /* Turn on the Host port power.                         */
    }

    USBH_OS_DlyMS(200u);

    p_reg->GINTMSK   =  0u;                                     /* Disable all interrupts                               */
    p_reg->GINTSTS   =  0xFFFFFFFFu;                            /* Clear any pending interrupts                         */

    p_reg->GRXFSIZ   =  DWCOTGHS_RXFIFO_DEPTH;                  /* Set Rx FIFO depth                                    */
                                                                /* Set Non-Periodic and Periodic Tx FIFO depths         */
    p_reg->HNPTXFSIZ = (DWCOTGHS_NONPER_CH_TXFIFO_DEPTH << 16u) |
                        DWCOTGHS_NONPER_CH_TXFIFO_START_ADDR;
    p_reg->HPTXFSIZ  = (DWCOTGHS_PER_CH_TXFIFO_DEPTH    << 16u) |
                        DWCOTGHS_PER_CH_TXFIFO_START_ADDR;

    p_reg->GINTMSK   = (DWCOTGHS_GINTx_WKUPINT  |               /* Enable Resume/Remote Wakeup Detected Intr            */
                        DWCOTGHS_GINTx_HPRTINT);                /* Enable host port interrupt.                          */

    if (p_bsp_api->ISR_Reg != (void *)0) {
        p_bsp_api->ISR_Reg(DWCOTGHS_ISR_Handler, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    DEF_BIT_SET(p_reg->GAHBCFG, DWCOTGHS_GAHBCFG_GINTMSK);      /* Enable the global interrupt in AHB cfg reg           */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_DWCOTGHS_HCD_Stop()
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

static  void  USBH_DWCOTGHS_HCD_Stop (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_HC_BSP_API    *p_bsp_api;
    CPU_INT32U          reg_val;


    p_reg     = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_bsp_api =  p_hc_drv->BSP_API_Ptr;

    DEF_BIT_CLR(p_reg->GAHBCFG, DWCOTGHS_GAHBCFG_GINTMSK);      /* Disable the global interrupt in AHB cfg reg          */
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
    if (DEF_BIT_IS_SET(reg_val, DWCOTGHS_HPRT_PORT_PWR) == DEF_YES) {
        DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg_val)                /* To avoid clearing some important interrupts          */
        DEF_BIT_CLR(reg_val, DWCOTGHS_HPRT_PORT_PWR);           /* Turn off the Host port power.                        */
        p_reg->HPRT = reg_val;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_DWCOTGHS_HCD_SpdGet()
*
* Description : Returns Host Controller speed.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Host controller speed retrieved successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : Host controller speed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_DEV_SPD  USBH_DWCOTGHS_HCD_SpdGet (USBH_HC_DRV  *p_hc_drv,
                                                USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;

    return (USBH_DEV_SPD_HIGH);
}


/*
*********************************************************************************************************
*                                     USBH_DWCOTGHS_HCD_Suspend()
*
* Description : Suspend Host Controller.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD suspend successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_DWCOTGHS_HCD_Suspend (USBH_HC_DRV  *p_hc_drv,
                                         USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_SR_ALLOC();


    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;

    CPU_CRITICAL_ENTER();
    p_drv_data->SavedGINTMSK = p_reg->GINTMSK;                  /* Save unmasked current intr.                          */
    p_reg->GINTMSK           = DEF_BIT_NONE;                    /* Mask all possible intr.                              */
    CPU_CRITICAL_EXIT();

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_DWCOTGHS_HCD_Resume()
*
* Description : Resume Host Controller.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HCD resume successful.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_DWCOTGHS_HCD_Resume (USBH_HC_DRV  *p_hc_drv,
                                        USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_SR_ALLOC();


    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;

    CPU_CRITICAL_ENTER();
    p_reg->GINTMSK           = p_drv_data->SavedGINTMSK;        /* Restored unmasked current intr.                      */
    p_drv_data->SavedGINTMSK = DEF_BIT_NONE;
    CPU_CRITICAL_EXIT();

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                   USBH_DWCOTGHS_HCD_FrameNbrGet()
*
* Description : Retrieve current frame number.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           HC frame number retrieved successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : Frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_DWCOTGHS_HCD_FrameNbrGet (USBH_HC_DRV  *p_hc_drv,
                                                   USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    CPU_INT32U          frm_nbr;


    p_reg   = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    frm_nbr = (p_reg->HFNUM & DWCOTGHS_HFNUM_FRNUM_MSK);

   *p_err = USBH_ERR_NONE;

    return (frm_nbr);
}


/*
*********************************************************************************************************
*                                     USBH_DWCOTGHS_HCD_EP_Open()
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

static  void  USBH_DWCOTGHS_HCD_EP_Open (USBH_HC_DRV  *p_hc_drv,
                                         USBH_EP      *p_ep,
                                         USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT08U          ch_nbr;
    CPU_SR_ALLOC();


    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;

    ch_nbr = DWCOTGHS_GetFreeChNbr(p_drv_data, p_err);
    if (*p_err != USBH_ERR_NONE){
        return;
    }

    CPU_CRITICAL_ENTER();
    DEF_BIT_SET(p_reg->GINTMSK, DWCOTGHS_GINTx_HCINT);          /* Make sure Global Host Channel interrupt is enabled.  */

    p_ep->DataPID                         = DWCOTGHS_GRXSTS_DPID_DATA0;
    p_drv_data->ChInfoTbl[ch_nbr].EP_Addr = ((p_ep->DevAddr << 8u)  |
                                              p_ep->Desc.bEndpointAddress);
    CPU_CRITICAL_EXIT();

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_DWCOTGHS_HCD_EP_Close()
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

static  void  USBH_DWCOTGHS_HCD_EP_Close (USBH_HC_DRV  *p_hc_drv,
                                          USBH_EP      *p_ep,
                                          USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT08U          ch_nbr;


    p_reg         = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data    = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    ch_nbr        =  DWCOTGHS_GetChNbr(p_drv_data, p_ep);
    p_ep->DataPID =  DWCOTGHS_GRXSTS_DPID_DATA0;

    if (ch_nbr != DWCOTGHS_INVALID_CH) {
                                                                /* ----------------- FREE CHANNEL #N ------------------ */
        p_reg->HCH[ch_nbr].HCINTMSKx = 0u;
        p_reg->HCH[ch_nbr].HCINTx    = 0xFFFFFFFFu;
        p_reg->HCH[ch_nbr].HCTSIZx   = 0u;

        if (p_drv_data->ChInfoTbl[ch_nbr].Tmr != (USBH_HTMR)0u) {
           *p_err = USBH_OS_TmrDel(p_drv_data->ChInfoTbl[ch_nbr].Tmr);
            p_drv_data->ChInfoTbl[ch_nbr].Tmr = (USBH_HTMR)0u;
        }

        p_drv_data->ChInfoTbl[ch_nbr].DoSplit   = DEF_NO;
        p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt = 0u;
        p_drv_data->ChInfoTbl[ch_nbr].EP_Addr   = DWCOTGHS_DFLT_EP_ADDR;
        DEF_BIT_CLR(p_drv_data->ChUsed, DEF_BIT(ch_nbr));
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_DWCOTGHS_HCD_EP_Abort()
*
* Description : Abort all pending URBs on endpoint.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Endpoint aborted successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_DWCOTGHS_HCD_EP_Abort (USBH_HC_DRV  *p_hc_drv,
                                          USBH_EP      *p_ep,
                                          USBH_ERR     *p_err)
{
    (void)p_hc_drv;
    (void)p_ep;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_DWCOTGHS_HCD_IsHalt_EP()
*
* Description : Retrieve endpoint halt state.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_ep         Pointer to endpoint structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           Endpoint halt status retrieved successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : DEF_TRUE,    If endpoint halted.
*               DEF_FALSE,   Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_IsHalt_EP (USBH_HC_DRV  *p_hc_drv,
                                                  USBH_EP      *p_ep,
                                                  USBH_ERR     *p_err)
{
    USBH_DRV_DATA  *p_drv_data;
    CPU_INT08U      ch_nbr;

   *p_err      =  USBH_ERR_NONE;
    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;
    ch_nbr     =  DWCOTGHS_GetChNbr(p_drv_data, p_ep);

    if ((ch_nbr                                    != DWCOTGHS_INVALID_CH) &&
        (p_drv_data->ChInfoTbl[ch_nbr].EP_TxErrCnt  >                  3u)) {
        p_drv_data->ChInfoTbl[ch_nbr].EP_TxErrCnt = 0;
        return (DEF_TRUE);
    }

    return (DEF_FALSE);
}


/*
*********************************************************************************************************
*                                   USBH_DWCOTGHS_HCD_URB_Submit()
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
* Note(s)     : (1) The polling rate timing for Interrupt Endpoints NAKs is shown below:
*                   +------------------------------+---------------------------+---------------------------+
*                   |      Low-Speed Device        |   Full-Speed Device       |       High-Speed Device   |
*                   +-----------+------------------+-----------+---------------+-----------+---------------+
*                   | binterval | Polling Rate(ms) | binterval | Poll Rate(ms) | binterval | Poll Rate(ms) |
*                   +-----------+------------------+-----------+---------------+-----------+---------------+
*                   | 0 to 15   |     8            |  1        |     1         | < 4       | Not supported |
*                   | 16 to 35  |    16            |  2 to 3   |     2         | 4         |      1        |
*                   | 36 to 255 |    32            |  4 to 7   |     4         | 5         |      2        |
*                   +-----------+------------------+  8 to 15  |     8         | 6 to 255  |      4        |
*                                                  | 16 to 31  |    16         +-----------+---------------+
*                                                  | 32 to 255 |    32         |
*                                                  +-----------+---------------+
*
*               (2) A binterval value of greater than 255 is forbidden by the USB specification.
*
*               (3) For High-speed devices a binterval value of less than 4 is not supported with the
*                   driver due to the granularity of using the OS software timers.
*********************************************************************************************************
*/

static  void  USBH_DWCOTGHS_HCD_URB_Submit (USBH_HC_DRV  *p_hc_drv,
                                            USBH_URB     *p_urb,
                                            USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT08U          ch_nbr;
    CPU_INT08U          ep_type;
    CPU_INT16U          poll_per;
    CPU_INT32U          reg_val;


    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    ep_type    =  USBH_EP_TypeGet(p_urb->EP_Ptr);
    ch_nbr     =  DWCOTGHS_GetChNbr(p_drv_data, p_urb->EP_Ptr);

    if (ch_nbr == DWCOTGHS_INVALID_CH) {                        /* Get a new ch if there is no EP associated with it    */
        ch_nbr = DWCOTGHS_GetFreeChNbr(p_drv_data, p_err);
        if (*p_err != USBH_ERR_NONE){
            return;
        }

        p_drv_data->ChInfoTbl[ch_nbr].EP_Addr = ((p_urb->EP_Ptr->DevAddr << 8u)  |
                                                  p_urb->EP_Ptr->Desc.bEndpointAddress);
    }
                                                                /* If Dev disconn & some remaining URBs processed by .. */
                                                                /* ..AsyncThread, ignore it                             */
    reg_val = p_reg->HPRT;
    if ((DEF_BIT_IS_CLR(reg_val, DWCOTGHS_HPRT_PORT_EN) == DEF_TRUE               ) ||
        (p_urb->State                                   == USBH_URB_STATE_ABORTED)) {
       *p_err = USBH_ERR_FAIL;
        return;
    }

    switch (ep_type){
        case USBH_EP_TYPE_CTRL:                                 /* ------------------ CONTROL CHANNEL ----------------- */
             if (p_urb->Token == USBH_TOKEN_SETUP) {
                 p_urb->EP_Ptr->DataPID = DWCOTGHS_GRXSTS_DPID_MDATA;
             } else {
                 p_urb->EP_Ptr->DataPID = DWCOTGHS_GRXSTS_DPID_DATA1;
             }
             break;


        case USBH_EP_TYPE_BULK:                                 /* ------------------- BULK CHANNEL ------------------- */
             break;


        case USBH_EP_TYPE_INTR:                                 /* ------------------- INTERRUPT CHANNEL -------------- */
             if (p_urb->EP_Ptr->DevSpd == USBH_DEV_SPD_LOW) {   /* See Note 1.                                          */
                 poll_per = 8u;                                 /* lowest value in milliseconds for polling period      */
                 if (p_urb->EP_Ptr->Desc.bInterval >= 36u) {
                     poll_per = 32u;
                 } else if (p_urb->EP_Ptr->Desc.bInterval >= 16u) {
                     poll_per = 16u;
                 }

             } else if (p_urb->EP_Ptr->DevSpd == USBH_DEV_SPD_FULL) {
                 poll_per = p_urb->EP_Ptr->Desc.bInterval;      /* lowest value for polling period should be 1ms        */
                 if (p_urb->EP_Ptr->Desc.bInterval >= 32u) {
                     poll_per = 32u;
                 } else if (p_urb->EP_Ptr->Desc.bInterval >= 16u) {
                     poll_per = 16u;
                 } else if (p_urb->EP_Ptr->Desc.bInterval >=  8u) {
                     poll_per = 8u;
                 } else if (p_urb->EP_Ptr->Desc.bInterval >=  4u) {
                     poll_per = 4u;
                 } else if (p_urb->EP_Ptr->Desc.bInterval >=  2u) {
                     poll_per = 2u;
                 }

             } else {                                           /* ---------------- HIGH SPEED DEVICE ----------------- */
                 if (p_urb->EP_Ptr->Desc.bInterval >= 6u) {
                     poll_per = 4u;
                 } else if (p_urb->EP_Ptr->Desc.bInterval >= 5u) {
                     poll_per = 2u;
                 } else if (p_urb->EP_Ptr->Desc.bInterval >= 4u) {
                     poll_per = 1u;                             /* lowest value for polling period should be 1ms        */
                 } else {
                    *p_err = USBH_ERR_NOT_SUPPORTED;            /* See note 3.                                          */
                 }
             }

             p_drv_data->ChInfoTbl[ch_nbr].Tmr = USBH_OS_TmrCreate("INTR EP NAK Timer",
                                                                    poll_per,
                                                                    DWCOTGHS_TmrCallback,
                                                                    p_urb,
                                                                    p_err);
             if (*p_err != USBH_ERR_NONE) {
                 return;
             }
             break;


        case USBH_EP_TYPE_ISOC:
        default:
            *p_err = USBH_ERR_NOT_SUPPORTED;
             return;
    }

    DWCOTGHS_ChInit(p_reg, p_urb, ch_nbr, p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_drv_data->ChInfoTbl[ch_nbr].URB_Ptr = p_urb;              /* Store transaction URB                                */
    p_drv_data->ChInfoTbl[ch_nbr].DoSplit = DEF_NO;

    p_reg->HCH[ch_nbr].HCSPLTx = 0u;                            /* Reset split register.                                */
    if (p_urb->EP_Ptr->DevPtr->HubHS_Ptr->DevPtr->IsRootHub != DEF_YES) {
        if ((p_urb->EP_Ptr->DevPtr->HubHS_Ptr != (void *)0u) &&
            (p_urb->EP_Ptr->DevSpd            != USBH_DEV_SPD_HIGH)) {
                                                                /* ------------- ENABLE SPLIT TRANSACTIONS ------------ */
            reg_val  =  DWCOTGHS_HCSPLTx_SPLITEN;               /* Enable this channel to perform split transactions.   */
            reg_val |=  p_urb->EP_Ptr->DevPtr->HubHS_Ptr->DevPtr->PortNbr;        /* Set port address                   */
            reg_val |= (p_urb->EP_Ptr->DevPtr->HubHS_Ptr->DevPtr->DevAddr << 7u); /* Set HUB address                    */
            reg_val |= (DWCOTGHS_HCSPLTx_XACTPOS_ALL << 14u);
            p_reg->HCH[ch_nbr].HCSPLTx = reg_val;               /* Set split register.                                  */

            p_drv_data->ChInfoTbl[ch_nbr].DoSplit = DEF_YES;

            reg_val  =   p_reg->HCH[ch_nbr].HCCHARx;
            reg_val &= ~(DWCOTGHS_HCCHARx_MCNT);
            reg_val |=  (DWCOTGHS_HCCHARx_MCNT_3_TRXN << 20);
            p_reg->HCH[ch_nbr].HCCHARx = reg_val;
        }
    }
                                                                /* ----------------- SUBMIT DATA TO HW ---------------- */
    DWCOTGHS_ChXferStart( p_reg,
                         &p_drv_data->ChInfoTbl[ch_nbr],
                          p_urb,
                          ch_nbr,
                          p_err);
}


/*
*********************************************************************************************************
*                                  USBH_DWCOTGHS_HCD_URB_Complete()
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

static  void  USBH_DWCOTGHS_HCD_URB_Complete (USBH_HC_DRV  *p_hc_drv,
                                              USBH_URB     *p_urb,
                                              USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT08U          ch_nbr;
    CPU_INT16U          xfer_len;
    CPU_INT32U          pkt_cnt;
    LIB_ERR             err_lib;
    CPU_SR_ALLOC();


   *p_err      =  USBH_ERR_NONE;
    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    ch_nbr     =  DWCOTGHS_GetChNbr(p_drv_data, p_urb->EP_Ptr);

    CPU_CRITICAL_ENTER();
    xfer_len = DWCOTGHS_GET_XFRSIZ(p_reg->HCH[ch_nbr].HCTSIZx);
    xfer_len = (p_drv_data->ChInfoTbl[ch_nbr].AppBufLen - xfer_len);

    if (p_urb->Token   == USBH_TOKEN_IN) {
        Mem_Copy((void *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen),
                                      p_urb->DMA_BufPtr,
                                      xfer_len);

        if (xfer_len > p_drv_data->ChInfoTbl[ch_nbr].NextXferLen) {  /* Received more data than what was expected       */
            p_urb->XferLen += p_drv_data->ChInfoTbl[ch_nbr].NextXferLen;
            p_urb->Err      = USBH_ERR_HC_IO;
        } else {
            p_urb->XferLen += xfer_len;
        }
    } else {
        pkt_cnt = DWCOTGHS_GET_PKTCNT(p_reg->HCH[ch_nbr].HCTSIZx);  /* Get packet count                                 */
        if (pkt_cnt == 0u) {
            p_urb->XferLen += p_drv_data->ChInfoTbl[ch_nbr].NextXferLen;
        } else {
            p_urb->XferLen += (p_drv_data->ChInfoTbl[ch_nbr].NextXferLen - xfer_len);
        }

    }
    CPU_CRITICAL_EXIT();

                                                                /* ------------------ FREE CHANNEL #N ----------------- */
    p_reg->HCH[ch_nbr].HCINTMSKx = 0u;
    p_reg->HCH[ch_nbr].HCINTx    = 0xFFFFFFFFu;
    p_reg->HCH[ch_nbr].HCTSIZx   = 0u;

    if (p_drv_data->ChInfoTbl[ch_nbr].Tmr != (USBH_HTMR)0u) {
       *p_err = USBH_OS_TmrDel(p_drv_data->ChInfoTbl[ch_nbr].Tmr);
        p_drv_data->ChInfoTbl[ch_nbr].Tmr = (USBH_HTMR)0u;
    }

    Mem_PoolBlkFree(&p_drv_data->DrvMemPool,
                     p_urb->DMA_BufPtr,
                    &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_HC_ALLOC;
    }
    p_urb->DMA_BufPtr = (void *)0u;

    p_drv_data->ChInfoTbl[ch_nbr].DoSplit   = DEF_NO;
    p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt = 0u;
    p_drv_data->ChInfoTbl[ch_nbr].EP_Addr   = DWCOTGHS_DFLT_EP_ADDR;
    DEF_BIT_CLR(p_drv_data->ChUsed, DEF_BIT(ch_nbr));
}


/*
*********************************************************************************************************
*                                    USBH_DWCOTGHS_HCD_URB_Abort()
*
* Description : Abort specified URB.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_urb        Pointer to URB structure.
*
*               p_err        Pointer to variable that will receive the return error code from this function
*                                USBH_ERR_NONE           URB aborted successfuly.
*                                Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_DWCOTGHS_HCD_URB_Abort (USBH_HC_DRV  *p_hc_drv,
                                           USBH_URB     *p_urb,
                                           USBH_ERR     *p_err)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT08U          ch_nbr;
    LIB_ERR             err_lib;


    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    ch_nbr     =  DWCOTGHS_GetChNbr(p_drv_data, p_urb->EP_Ptr);
    p_urb->Err =  USBH_ERR_URB_ABORT;
   *p_err      =  USBH_ERR_NONE;

    Mem_PoolBlkFree(&p_drv_data->DrvMemPool,
                     p_urb->DMA_BufPtr,
                    &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_HC_ALLOC;
    }
    p_urb->DMA_BufPtr = (void *)0u;

                                                                /* Reset registers related to Host channel #n.          */
    p_reg->HCH[ch_nbr].HCINTMSKx = 0u;
    p_reg->HCH[ch_nbr].HCINTx    = 0xFFFFFFFFu;
    p_reg->HCH[ch_nbr].HCTSIZx   = 0u;
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
*                                  USBH_DWCOTGHS_HCD_PortStatusGet()
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
* Note(s)     : (1) The DWC OTG HS Host supports only ONE port.
*               (2) Port numbers start from 1.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortStatusGet (USBH_HC_DRV           *p_hc_drv,
                                                      CPU_INT08U             port_nbr,
                                                      USBH_HUB_PORT_STATUS  *p_port_status)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT32U          reg_val;


    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;

    if (port_nbr != 1u) {
        p_port_status->wPortStatus = 0u;
        p_port_status->wPortChange = 0u;
        return (DEF_FAIL);
    }

    reg_val = p_reg->HPRT;
                                                                /* Bits not used by the stack. Maintain constant value. */
    DEF_BIT_CLR(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_TEST);
    DEF_BIT_CLR(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_INDICATOR);

    if (DEF_BIT_IS_SET(reg_val, DWCOTGHS_HPRT_PORT_EN) == DEF_YES) {
        if ((reg_val & DWCOTGHS_HPRT_PORT_SPD_MSK) == DWCOTGHS_HPRT_PORT_SPD_FS) {
                                                                /* ------------- FULL-SPEED DEVICE ATTACHED ----------- */
            DEF_BIT_CLR(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_LOW_SPD);
            DEF_BIT_CLR(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_HIGH_SPD);

        } else if ((reg_val & DWCOTGHS_HPRT_PORT_SPD_MSK) == DWCOTGHS_HPRT_PORT_SPD_LS) {
                                                                /* ------------- LOW-SPEED DEVICE ATTACHED ------------ */
            DEF_BIT_SET(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_LOW_SPD);
            DEF_BIT_CLR(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_HIGH_SPD);

        } else if ((reg_val & DWCOTGHS_HPRT_PORT_SPD_MSK) == DWCOTGHS_HPRT_PORT_SPD_HS) {
                                                                /* ------------- HIGH-SPEED DEVICE ATTACHED ----------- */
            DEF_BIT_CLR(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_LOW_SPD);
            DEF_BIT_SET(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_HIGH_SPD);
        }
    }

    p_port_status->wPortStatus = p_drv_data->RH_PortStat;
    p_port_status->wPortChange = p_drv_data->RH_PortChng;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_DWCOTGHS_HCD_HubDescGet()
*
* Description : Retrieve root hub descriptor.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               p_buf        Pointer to buffer that will receive hub descriptor.
*
*               buf_len      Buffer length in octets.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
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

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_HubDescGet (USBH_HC_DRV  *p_hc_drv,
                                                   void         *p_buf,
                                                   CPU_INT08U    buf_len)
{
    USBH_DRV_DATA  *p_drv_data;
    USBH_HUB_DESC   hub_desc;


    p_drv_data = (USBH_DRV_DATA *)p_hc_drv->DataPtr;

    hub_desc.bDescLength         = USBH_HUB_LEN_HUB_DESC;
    hub_desc.bDescriptorType     = USBH_HUB_DESC_TYPE_HUB;
    hub_desc.bNbrPorts           =   1u;                        /* See Note #3                                          */
    hub_desc.wHubCharacteristics =   0u;
    hub_desc.bPwrOn2PwrGood      = 100u;
    hub_desc.bHubContrCurrent    =   0u;

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
*                                    USBH_DWCOTGHS_HCD_PortEnSet()
*
* Description : Enable given port.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortEnSet (USBH_HC_DRV  *p_hc_drv,
                                                  CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_DWCOTGHS_HCD_PortEnClr()
*
* Description : Clear port enable status.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortEnClr (USBH_HC_DRV  *p_hc_drv,
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
*                                  USBH_DWCOTGHS_HCD_PortEnChngClr()
*
* Description : Clear port enable status change.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortEnChngClr (USBH_HC_DRV  *p_hc_drv,
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
*                                   USBH_DWCOTGHS_HCD_PortPwrSet()
*
* Description : Set port power based on port power mode.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortPwrSet (USBH_HC_DRV  *p_hc_drv,
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
*                                   USBH_DWCOTGHS_HCD_PortPwrClr()
*
* Description : Clear port power.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortPwrClr (USBH_HC_DRV  *p_hc_drv,
                                                   CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  USBH_DWCOTGHS_HCD_PortResetSet()
*
* Description : Reset given port.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : (1) The application must wait at least 10 ms (+ 10 ms safety)
*                   before clearing the reset bit.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortResetSet (USBH_HC_DRV  *p_hc_drv,
                                                     CPU_INT08U    port_nbr)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT32U          reg_val;


    (void)port_nbr;

    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;

                                                                /* ---------------- START RESET PROCESS --------------- */
    reg_val =  p_reg->HPRT;                                     /* Read host port control and status                    */
    DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg_val);                   /* To avoid clearing some important bits.               */
    DEF_BIT_SET(reg_val, DWCOTGHS_HPRT_PORT_RST);
    p_reg->HPRT              = reg_val;                         /* Start reset signaling to dev.                        */
    p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_RESET;      /* This bit is set while in the resetting state         */

    USBH_OS_DlyMS(50u);                                         /* See Note #1                                          */

                                                                /* --------------- FINISH RESET PROCESS --------------- */
    reg_val = p_reg->HPRT;                                      /* Read host port control and status                    */
    DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg_val);                   /* To avoid clearing some important bits.               */
    DEF_BIT_CLR(reg_val, DWCOTGHS_HPRT_PORT_RST);               /* Clr USB reset.                                       */
    p_reg->HPRT = reg_val;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                USBH_DWCOTGHS_HCD_PortResetChngClr()
*
* Description : Clear port reset status change.
*
* Argument(s) : p_hc_drv      Pointer to host controller driver structure.
*
*               port_nbr      Port Number.
*
* Return(s)   : DEF_OK,       If successful.
*               DEF_FAIL,     otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortResetChngClr (USBH_HC_DRV  *p_hc_drv,
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
*                                 USBH_DWCOTGHS_HCD_PortSuspendClr()
*
* Description : Resume given port if port is suspended.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortSuspendClr (USBH_HC_DRV  *p_hc_drv,
                                                       CPU_INT08U    port_nbr)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    CPU_INT32U          reg_val;


    (void)port_nbr;

    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;

    reg_val = p_reg->HPRT;
    DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg_val);                   /* To avoid clearing some important bits.               */
    DEF_BIT_SET(reg_val, DWCOTGHS_HPRT_PORT_RES);               /* Issue resume signal to dev.                          */
    p_reg->HPRT = reg_val;

    USBH_OS_DlyMS(20u);                                         /* Resume signal driven for 20 ms as per USB 2.0 spec.  */

    reg_val = p_reg->HPRT;
    DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg_val);                   /* To avoid clearing some important bits.               */
    DEF_BIT_CLR(reg_val, DWCOTGHS_HPRT_PORT_RES);               /* Clr USB resume signal.                               */
    p_reg->HPRT = reg_val;

    DEF_BIT_CLR(p_drv_data->RH_PortStat, USBH_HUB_STATUS_PORT_SUSPEND);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                 USBH_DWCOTGHS_HCD_PortConnChngClr()
*
* Description : Clear port connect status change.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
*               port_nbr     Port Number.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_PortConnChngClr (USBH_HC_DRV  *p_hc_drv,
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
*                                   USBH_DWCOTGHS_HCD_RHSC_IntEn()
*
* Description : Enable root hub interrupt.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_RHSC_IntEn (USBH_HC_DRV  *p_hc_drv)
{
    USBH_DWCOTGHS_REG  *p_reg;
    CPU_INT32U          reg_val;


    p_reg = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_SET(p_reg->GINTMSK, DWCOTGHS_GINTx_HPRTINT);

                                                                /* --------------- ENABLE VBUS DRIVING ---------------- */
    reg_val = p_reg->HPRT;
    DWCOTGHS_HPRT_BITS_CLR_PRESERVE(reg_val);                   /* To avoid clearing some important interrupts          */
    if (DEF_BIT_IS_CLR(reg_val, DWCOTGHS_HPRT_PORT_PWR) == DEF_YES) {
        p_reg->HPRT = (DWCOTGHS_HPRT_PORT_PWR | reg_val);       /* Turn on the Host port power.                         */
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_DWCOTGHS_HCD_RHSC_IntDis()
*
* Description : Disable root hub interrupt.
*
* Argument(s) : p_hc_drv     Pointer to host controller driver structure.
*
* Return(s)   : DEF_OK,      If successful.
*               DEF_FAIL,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_DWCOTGHS_HCD_RHSC_IntDis (USBH_HC_DRV  *p_hc_drv)
{
    USBH_DWCOTGHS_REG  *p_reg;


    p_reg = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_CLR(p_reg->GINTMSK, DWCOTGHS_GINTx_HPRTINT);
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
*                                       DWCOTGHS_ISR_Handler()
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

static  void  DWCOTGHS_ISR_Handler (void  *p_drv)
{
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_DRV_DATA      *p_drv_data;
    USBH_HC_DRV        *p_hc_drv;
    CPU_INT16U          ch_nbr;
    CPU_INT32U          ch_int;
    CPU_INT32U          gintsts_reg;
    CPU_INT32U          reg_val;


    p_hc_drv     = (USBH_HC_DRV       *)p_drv;
    p_reg        = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_drv_data   = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    gintsts_reg  =  p_reg->GINTSTS;
    gintsts_reg &=  p_reg->GINTMSK;                             /* Keep only the unmasked interrupt                     */


                                                                /* ------------------ START OF FRAME ------------------ */
    if (DEF_BIT_IS_SET(gintsts_reg, DWCOTGHS_GINTx_SOF) == DEF_YES) {
        p_reg->GINTSTS = DWCOTGHS_GINTx_SOF;                    /* Acknowledge interrupt by a write clear.              */

        if (p_drv_data->CSPLITChBmp != DEF_BIT_NONE) {
            ch_int = p_drv_data->CSPLITChBmp;                   /* Read Complete Split Channel Bitmap                   */

            while (ch_int != DEF_BIT_NONE) {
                ch_nbr = CPU_CntTrailZeros(ch_int);
                                                                /* ----------- HANDLE COMPLETE SPLIT PACKET ----------- */
                if (p_drv_data->SOFCtr >= p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt) {
                    DEF_BIT_SET(p_reg->HCH[ch_nbr].HCSPLTx, DWCOTGHS_HCSPLTx_COMPLSPLT);
                    DEF_BIT_CLR(p_drv_data->CSPLITChBmp, DEF_BIT(ch_nbr));

                    p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt = 0u;
                    DWCOTGHS_CH_EN(reg_val, p_reg->HCH[ch_nbr]);
                }
                DEF_BIT_CLR(ch_int, DEF_BIT(ch_nbr));
            }
        }

        p_drv_data->SOFCtr++;
        if (p_drv_data->CSPLITChBmp == DEF_BIT_NONE) {
            p_drv_data->SOFCtr = 0u;
            DEF_BIT_CLR(p_reg->GINTMSK, DWCOTGHS_GINTx_SOF);
        }
    }

    if (DEF_BIT_IS_SET(gintsts_reg, DWCOTGHS_GINTx_DISCINT) == DEF_YES) {
        DEF_BIT_CLR(p_reg->GAHBCFG, DWCOTGHS_GAHBCFG_GINTMSK);  /* Disable the global interrupt for the OTG controller  */
        DWCOTGHS_ISR_PortDisconn(p_reg,
                                 p_drv_data,
                                 p_hc_drv->RH_DevPtr);
        DEF_BIT_SET(p_reg->GAHBCFG, DWCOTGHS_GAHBCFG_GINTMSK);  /* Enable the global interrupt for the OTG controller   */
        return;                                                 /* No other interrupts must be processed                */
    }
                                                                /* ------------- PORT STATUS CHANGE INT --------------- */
    if (DEF_BIT_IS_SET(gintsts_reg, DWCOTGHS_GINTx_HPRTINT) == DEF_YES) {
        DEF_BIT_CLR(p_reg->GAHBCFG, DWCOTGHS_GAHBCFG_GINTMSK);  /* Disable the global interrupt for the OTG controller  */
        DWCOTGHS_ISR_PortConn(p_reg,
                              p_drv_data,
                              p_hc_drv->RH_DevPtr);

        DEF_BIT_SET(p_reg->GAHBCFG, DWCOTGHS_GAHBCFG_GINTMSK);  /* Enable the global interrupt for the OTG controller   */
        return;                                                 /* No other interrupts must be processed                */
    }

    if (DEF_BIT_IS_SET(gintsts_reg, DWCOTGHS_GINTx_HCINT) == DEF_YES) {
        ch_int  = (p_reg->HAINT & 0x0000FFFFu);                 /* Read HAINT reg to serve all the active ch interrupts */
        ch_nbr  =  CPU_CntTrailZeros(ch_int);
        reg_val =  p_reg->HCH[ch_nbr].HCCHARx;

        while (ch_int != 0u) {                                  /* Handle Host channel interrupt                        */
            if ((reg_val & DWCOTGHS_HCCHARx_EPDIR) == 0u) {     /* ---------------- OUT CHANNEL HANDLER --------------- */
                DWCOTGHS_ISR_HostChOUT(p_reg,
                                       p_drv_data,
                                       ch_nbr);
                                                                /* ---------------- IN CHANNEL HANDLER ---------------- */
            } else if ((reg_val & DWCOTGHS_HCCHARx_EPDIR) != 0u) {
                DWCOTGHS_ISR_HostChIN(p_reg,
                                      p_drv_data,
                                      ch_nbr);
            }

            DEF_BIT_CLR(ch_int, DEF_BIT(ch_nbr));
            ch_int  = (p_reg->HAINT & 0x0000FFFFu);             /* Read HAINT reg to serve all the active ch interrupts */
            ch_nbr  =  CPU_CntTrailZeros(ch_int);
            reg_val =  p_reg->HCH[ch_nbr].HCCHARx;
        }
    }
}


/*
*********************************************************************************************************
*                                       DWCOTGHS_ISR_PortConn()
*
* Description : Handle Host Port interrupt. The core sets this bit to indicate a change in port status
*               of one of the DWC OTG HS core ports in Host mode.
*
* Argument(s) : p_reg          Pointer to DWC OTG HS registers structure.
*
*               p_drv_data     Pointer to host driver data structure.
*
*               p_dev          Pointer to hub device.
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

static  void  DWCOTGHS_ISR_PortConn (USBH_DWCOTGHS_REG  *p_reg,
                                     USBH_DRV_DATA      *p_drv_data,
                                     USBH_DEV           *p_dev)
{
    CPU_INT32U  hprt_reg;
    CPU_INT32U  hprt_reg_dup;
    CPU_INT32U  dev_speed;


    hprt_reg     = p_reg->HPRT;
    hprt_reg_dup = p_reg->HPRT;
    DWCOTGHS_HPRT_BITS_CLR_PRESERVE(hprt_reg_dup)               /* To avoid clearing some important interrupts          */

                                                                /* -------------- PORT CONNECT DETECTED --------------- */
    if ((DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_CONN_DET) == DEF_YES) &&
        (DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_CONN_STS) == DEF_YES)) {

        DEF_BIT_SET(hprt_reg_dup, DWCOTGHS_HPRT_PORT_CONN_DET); /* Prepare intr acknowledgement.                        */

        p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_CONN;   /* Bit reflects if a device is currently connected      */
        p_drv_data->RH_PortChng |= USBH_HUB_STATUS_C_PORT_CONN; /* Bit indicates a Port's current connect status change */

        USBH_HUB_RH_Event(p_dev);                               /* Notify the core layer.                               */

        DEF_BIT_SET(p_reg->GINTMSK, DWCOTGHS_GINTx_DISCINT);    /* Enable the Disconnect interrupt                      */

    } else if (DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_CONN_DET) == DEF_YES) { /*    See Note #1                    */
        DEF_BIT_SET(hprt_reg_dup, DWCOTGHS_HPRT_PORT_CONN_DET); /* Prepare intr acknowledgement.                        */
    }

                                                                /* ------------ PORT ENABLE/DISABLE CHANGE ------------ */
                                                                /* See Note #2.                                         */
    if ((DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_ENCHNG)   == DEF_YES) &&
        (DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_CONN_STS) == DEF_YES)) {

        DEF_BIT_SET(hprt_reg_dup, DWCOTGHS_HPRT_PORT_ENCHNG);   /* Prepare intr acknowledgement.                        */
                                                                /* Is port en. after reset sequence due to dev. conn?   */
        if (DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_EN) == DEF_YES) {
            DEF_BIT_CLR(p_reg->HCFG, DWCOTGHS_HCFG_FSLS_PHYCLK_SEL_MSK);

            dev_speed = (p_reg->HPRT & DWCOTGHS_HPRT_PORT_SPD_MSK);
            if (dev_speed == DWCOTGHS_HPRT_PORT_SPD_FS) {
                p_reg->HFIR = 60000;
            }

            p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_EN; /* Bit may be set due to SetPortFeature(PORT_RESET)     */
            p_drv_data->RH_PortChng |= USBH_HUB_STATUS_C_PORT_RESET; /* Transitioned from RESET state to ENABLED State  */

            USBH_HUB_RH_Event(p_dev);                           /* Notify the core layer.                               */

        } else {
                                                                /* --------------- FORCE DISCONNECTION ---------------- */
            DEF_BIT_CLR(hprt_reg_dup, DWCOTGHS_HPRT_PORT_PWR);  /* Power OFF VBUS                                       */
        }
    } else if (DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_ENCHNG) == DEF_YES) {
        DEF_BIT_SET(hprt_reg_dup, DWCOTGHS_HPRT_PORT_ENCHNG);   /* Prepare intr acknowledgement.                        */
    }

                                                                /* ---------------- OVERCURRENT CHANGE ---------------- */
    if (DEF_BIT_IS_SET(hprt_reg, DWCOTGHS_HPRT_PORT_OCCHNG) == DEF_YES) {
        p_drv_data->RH_PortStat |= USBH_HUB_STATUS_PORT_OVER_CUR;
        DEF_BIT_SET(hprt_reg_dup, DWCOTGHS_HPRT_PORT_OCCHNG);
    }

    p_reg->HPRT = hprt_reg_dup;                                 /* Acknowledge serviced interrupt                       */
}


/*
*********************************************************************************************************
*                                     DWCOTGHS_ISR_PortDisconn()
*
* Description : Handle Disconnect Detected Interrupt. Asserted when a device disconnect is detected.
*
* Argument(s) : p_reg          Pointer to STM32Fx registers structure.
*
*               p_drv_data     Pointer to host driver data structure.
*
*               p_dev          Pointer to hub device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DWCOTGHS_ISR_PortDisconn (USBH_DWCOTGHS_REG  *p_reg,
                                        USBH_DRV_DATA      *p_drv_data,
                                        USBH_DEV           *p_dev)
{
    CPU_INT32U  reg_val;


    DEF_BIT_SET(p_reg->GINTSTS, DWCOTGHS_GINTx_DISCINT);        /* Ack Device Disconnected interrupt by a write clear   */
    reg_val = p_reg->HPRT;                                      /* Read host port control and status reg                */

                                                                /*  No device is attached to the port                   */
    if (DEF_BIT_IS_CLR(reg_val, DWCOTGHS_HPRT_PORT_CONN_STS) == DEF_YES) {
        DEF_BIT_SET(p_reg->HPRT , DWCOTGHS_HPRT_PORT_CONN_DET); /* Clear the Port Host Interrupt in GINSTS register     */
        DEF_BIT_CLR(p_reg->GINTMSK, DWCOTGHS_GINTx_DISCINT);    /* Disable the Disconnect interrupt                     */
        DEF_BIT_SET(p_reg->GINTMSK, DWCOTGHS_GINTx_HPRTINT);    /* Now En Root hub Intr to detect a new device conn     */

        p_reg->HAINT   = 0xFFFFFFFF;
        p_reg->GINTSTS = 0xFFFFFFFF;

        p_drv_data->RH_PortStat  = 0u;                          /* Clear Root hub Port Status bits                      */
        p_drv_data->RH_PortChng |= USBH_HUB_STATUS_C_PORT_CONN; /* Current connect status has changed                   */
        p_drv_data->SOFCtr       = 0u;
        p_drv_data->CSPLITChBmp  = 0u;

        USBH_HUB_RH_Event(p_dev);                               /* Notify the core layer.                               */
    }
}


/*
*********************************************************************************************************
*                                      DWCOTGHS_ISR_HostChOUT()
*
* Description : Handle all the interrupts related to an OUT transaction (success and errors)
*
* Argument(s) : p_reg          Pointer to DWC OTG HS registers structure.
*
*               p_drv_data     Pointer to host driver data structure.
*
*               ch_nbr         Host Channel OUT number which has generated the interrupt.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DWCOTGHS_ISR_HostChOUT (USBH_DWCOTGHS_REG  *p_reg,
                                      USBH_DRV_DATA      *p_drv_data,
                                      CPU_INT08U          ch_nbr)
{
    USBH_DWCOTGHS_CH_INFO  *p_ch_info;
    USBH_URB               *p_urb;
    CPU_INT32U              hcint_reg;
    CPU_INT08U              ep_type;
    CPU_INT32U              xfer_len;


    hcint_reg  =  p_reg->HCH[ch_nbr].HCINTx;
    hcint_reg &=  p_reg->HCH[ch_nbr].HCINTMSKx;
    p_ch_info  = &p_drv_data->ChInfoTbl[ch_nbr];
    p_urb      =  p_ch_info->URB_Ptr;
    ep_type    =  USBH_EP_TypeGet(p_urb->EP_Ptr);

    if (hcint_reg & DWCOTGHS_HCINTx_CHH) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_CHH;        /* Clear int.                                           */

        if (p_reg->HCH[ch_nbr].HCINTx & DWCOTGHS_HCINTx_XFRC) {
                                                                /* Disable channel halted int.                          */
            DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, DWCOTGHS_HCINTx_CHH);
            p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_XFRC;   /* Clear int.                                           */

                                                                /* Save next EP DATA PID value                          */
            p_urb->EP_Ptr->DataPID = DWCOTGHS_GET_DPID(p_reg->HCH[ch_nbr].HCTSIZx);
            p_urb->Err             = USBH_ERR_NONE;
            p_ch_info->EP_TxErrCnt = 0u;

            xfer_len = DWCOTGHS_GET_XFRSIZ(p_reg->HCH[ch_nbr].HCTSIZx);
            xfer_len = p_ch_info->AppBufLen + p_urb->XferLen;

            if (xfer_len == p_urb->UserBufLen) {
                USBH_URB_Done(p_urb);                           /* Notify the Core layer about the URB completion       */
            } else {
                p_urb->Err = USBH_OS_MsgQueuePut(        DWCOTGHS_URB_Proc_Q,
                                                 (void *)p_urb);
                if (p_urb->Err != USBH_ERR_NONE) {
                    USBH_URB_Done(p_urb);                       /* Notify the Core layer about the URB completion       */
                }
            }

        } else {
            if (p_ch_info->DoSplit == DEF_YES) {
                p_ch_info->EP_TxErrCnt = 0;
                DWCOTGHS_ChSplitHandler(p_reg,
                                        p_drv_data,
                                        ch_nbr,
                                        ep_type);
            }
        }

    } else if (hcint_reg & DWCOTGHS_HCINTx_STALL) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_STALL;      /* Clear int.                                           */
        p_urb->Err                = USBH_ERR_EP_STALL;
        USBH_URB_Done(p_urb);                                   /* Notify the Core layer about the URB completion       */

    } else if (hcint_reg & DWCOTGHS_HCINTx_DTERR) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_DTERR;      /* Clear int.                                           */
        p_urb->Err                = USBH_ERR_EP_DATA_TOGGLE;
        USBH_URB_Done(p_urb);                                   /* Notify the Core layer about the URB completion       */

    } else if ((hcint_reg & DWCOTGHS_HCINTx_TXERR ) ||
               (hcint_reg & DWCOTGHS_HCINTx_AHBERR)) {

        p_reg->HCH[ch_nbr].HCINTx = (DWCOTGHS_HCINTx_TXERR  |   /* Clear int.                                           */
                                     DWCOTGHS_HCINTx_AHBERR);

        if (p_ch_info->DoSplit == DEF_YES) {                    /* Clear Complete SPLIT bit to resend Start Split       */
            p_reg->HCH[ch_nbr].HCSPLTx &= ~DWCOTGHS_HCSPLTx_COMPLSPLT;
        }

        p_ch_info->EP_TxErrCnt++;

        DEF_BIT_SET(p_reg->HCH[ch_nbr].HCINTMSKx, DWCOTGHS_HCINTx_CHH);
        p_reg->HCH[ch_nbr].HCCHARx |= DWCOTGHS_HCCHARx_CHDIS;
        p_reg->HCH[ch_nbr].HCCHARx |= DWCOTGHS_HCCHARx_CHENA;

        USBH_URB_Done(p_urb);                                   /* Notify the Core layer about the URB completion       */

    } else if (hcint_reg & DWCOTGHS_HCINTx_NAK) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_NAK;        /* Clear int.                                           */

        p_ch_info->EP_TxErrCnt = 0;
        if (ep_type == USBH_EP_TYPE_INTR) {
                                                                /* Save next EP DATA PID value                          */
            p_urb->EP_Ptr->DataPID = DWCOTGHS_GET_DPID(p_reg->HCH[ch_nbr].HCTSIZx);
            p_urb->Err             = USBH_OS_MsgQueuePut(        DWCOTGHS_URB_Proc_Q,
                                                         (void *)p_urb);
            if (p_urb->Err != USBH_ERR_NONE) {
                USBH_URB_Done(p_urb);                           /* Notify the Core layer about the URB completion       */
            } else {
                p_urb->Err = USBH_ERR_EP_NACK;
            }

        } else if (p_ch_info->DoSplit == DEF_YES) {
                                                                /* Save next EP DATA PID value                          */
            p_urb->EP_Ptr->DataPID = DWCOTGHS_GET_DPID(p_reg->HCH[ch_nbr].HCTSIZx);
            DWCOTGHS_ChEnable(&p_reg->HCH[ch_nbr], p_urb);
        }
    }
}


/*
*********************************************************************************************************
*                                       DWCOTGHS_ISR_HostChIN()
*
* Description : Handle all the interrupts related to an IN transaction (success and errors)
*
* Argument(s) : p_reg          Pointer to DWC OTG HS registers structure.
*
*               p_drv_data     Pointer to host driver data structure.
*
*               ch_nbr         Host Channel IN number which has generated the interrupt
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DWCOTGHS_ISR_HostChIN (USBH_DWCOTGHS_REG  *p_reg,
                                     USBH_DRV_DATA      *p_drv_data,
                                     CPU_INT08U          ch_nbr)
{
    USBH_DWCOTGHS_CH_INFO  *p_ch_info;
    USBH_URB               *p_urb;
    CPU_INT32U              hcint_reg;
    CPU_INT32U              reg_val;
    CPU_INT32U              xfer_len;
    CPU_INT16U              ep_pkt_size;
    CPU_INT08U              ep_type;


    hcint_reg  =  p_reg->HCH[ch_nbr].HCINTx;
    hcint_reg &=  p_reg->HCH[ch_nbr].HCINTMSKx;
    p_ch_info  = &p_drv_data->ChInfoTbl[ch_nbr];
    p_urb      =  p_ch_info->URB_Ptr;
    ep_type    =  USBH_EP_TypeGet(p_urb->EP_Ptr);

    if (hcint_reg & DWCOTGHS_HCINTx_CHH) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_CHH;        /* Clear int.                                           */

        if (p_reg->HCH[ch_nbr].HCINTx & DWCOTGHS_HCINTx_XFRC) {
                                                                /* Disable channel halted int.                          */
            DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCINTMSKx, DWCOTGHS_HCINTx_CHH);
            p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_XFRC;   /* Clear int.                                           */

                                                                /* Save next EP DATA PID value                          */
            p_urb->EP_Ptr->DataPID = DWCOTGHS_GET_DPID(p_reg->HCH[ch_nbr].HCTSIZx);
            p_urb->Err             = USBH_ERR_NONE;
            p_ch_info->EP_TxErrCnt = 0u;

            ep_pkt_size = USBH_EP_MaxPktSizeGet(p_urb->EP_Ptr);
            reg_val     = DWCOTGHS_GET_PKTCNT(p_reg->HCH[ch_nbr].HCTSIZx); /* Get channel packet count.                 */
            xfer_len    = DWCOTGHS_GET_XFRSIZ(p_reg->HCH[ch_nbr].HCTSIZx);
            xfer_len    = (p_ch_info->AppBufLen - xfer_len);

            if ((reg_val  ==          0u) ||                    /* if packet count equals zero                          */
                (xfer_len  < ep_pkt_size)) {
                USBH_URB_Done(p_urb);                           /* Notify the Core layer about the URB completion       */
            } else {
                p_urb->Err = USBH_OS_MsgQueuePut(        DWCOTGHS_URB_Proc_Q,
                                                 (void *)p_urb);
                if (p_urb->Err != USBH_ERR_NONE) {
                    USBH_URB_Done(p_urb);                       /* Notify the Core layer about the URB completion       */
                }
            }

        } else {
            if (p_ch_info->DoSplit == DEF_YES) {
                p_ch_info->EP_TxErrCnt = 0;
                DWCOTGHS_ChSplitHandler(p_reg,
                                        p_drv_data,
                                        ch_nbr,
                                        ep_type);
            }
        }

    } else if (hcint_reg & DWCOTGHS_HCINTx_STALL) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_STALL;      /* Clear int.                                           */
        p_urb->Err                = USBH_ERR_EP_STALL;
        USBH_URB_Done(p_urb);                                   /* Notify the Core layer about the URB completion       */

    } else if (hcint_reg & DWCOTGHS_HCINTx_DTERR) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_DTERR;      /* Clear int.                                           */
        p_urb->Err                = USBH_ERR_EP_DATA_TOGGLE;
        USBH_URB_Done(p_urb);                                   /* Notify the Core layer about the URB completion       */

    } else if ((hcint_reg & DWCOTGHS_HCINTx_TXERR ) ||
               (hcint_reg & DWCOTGHS_HCINTx_AHBERR)) {

        p_reg->HCH[ch_nbr].HCINTx = (DWCOTGHS_HCINTx_TXERR  |   /* Clear int.                                           */
                                     DWCOTGHS_HCINTx_AHBERR);

        if (p_ch_info->DoSplit == DEF_YES) {                    /* Clr Complete SPLIT bit to resend Start Split         */
            p_reg->HCH[ch_nbr].HCSPLTx &= ~DWCOTGHS_HCSPLTx_COMPLSPLT;
        }

        p_ch_info->EP_TxErrCnt++;

        DEF_BIT_SET(p_reg->HCH[ch_nbr].HCINTMSKx, DWCOTGHS_HCINTx_CHH);
        p_reg->HCH[ch_nbr].HCCHARx |= DWCOTGHS_HCCHARx_CHDIS;
        p_reg->HCH[ch_nbr].HCCHARx |= DWCOTGHS_HCCHARx_CHENA;

        USBH_URB_Done(p_urb);                                   /* Notify the Core layer about the URB completion       */

    } else if (hcint_reg & DWCOTGHS_HCINTx_FRMOR) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_FRMOR;      /* Clear int.                                           */

        if (p_ch_info->DoSplit == DEF_YES) {                    /* Clr Complete SPLIT bit to resend Start Split         */
            p_reg->HCH[ch_nbr].HCSPLTx &= ~DWCOTGHS_HCSPLTx_COMPLSPLT;
        }

        p_reg->HCH[ch_nbr].HCCHARx |= DWCOTGHS_HCCHARx_ODDFRM;
        DWCOTGHS_CH_EN(reg_val, p_reg->HCH[ch_nbr]);

    } else if (hcint_reg & DWCOTGHS_HCINTx_NAK) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_NAK;        /* Clear int.                                           */

        p_ch_info->EP_TxErrCnt = 0;
        if (ep_type == USBH_EP_TYPE_INTR) {
                                                                /* Save next EP DATA PID value                          */
            p_urb->EP_Ptr->DataPID = DWCOTGHS_GET_DPID(p_reg->HCH[ch_nbr].HCTSIZx);
            p_urb->Err             = USBH_OS_MsgQueuePut(        DWCOTGHS_URB_Proc_Q,
                                                         (void *)p_urb);
            if (p_urb->Err != USBH_ERR_NONE) {
                USBH_URB_Done(p_urb);                           /* Notify the Core layer about the URB completion       */
            } else {
                p_urb->Err = USBH_ERR_EP_NACK;
            }

        } else if (p_ch_info->DoSplit == DEF_YES) {
                                                                /* Save next EP DATA PID value                          */
            p_urb->EP_Ptr->DataPID = DWCOTGHS_GET_DPID(p_reg->HCH[ch_nbr].HCTSIZx);
            DWCOTGHS_ChEnable(&p_reg->HCH[ch_nbr], p_urb);
        }
    }
}


/*
*********************************************************************************************************
*                                      DWCOTGHS_ChSplitHandler()
*
* Description : Schedule when the CSPLIT/SSPLIT packet will be sent related to the Start of frame(SOF)
*
* Argument(s) : p_reg          Pointer to DWC OTG HS registers structure.
*
*               p_drv_data     Pointer to host driver data structure.
*
*               ch_nbr         Host Channel IN number which has generated the interrupt.
*
*               ep_type        Endpoint type (Control/Bulk/Interrupt).
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DWCOTGHS_ChSplitHandler (USBH_DWCOTGHS_REG  *p_reg,
                                       USBH_DRV_DATA      *p_drv_data,
                                       CPU_INT08U          ch_nbr,
                                       CPU_INT08U          ep_type)
{
    CPU_INT32U  reg_val;
    CPU_SR_ALLOC();


    if (p_reg->HCH[ch_nbr].HCINTx & DWCOTGHS_HCINTx_NYET) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_NYET;

        if (ep_type == USBH_EP_TYPE_INTR) {                     /* Resend Start SPLIT packet immediately                */
            DEF_BIT_CLR(p_reg->HCH[ch_nbr].HCSPLTx, DWCOTGHS_HCSPLTx_COMPLSPLT);
            DEF_BIT_CLR(p_drv_data->CSPLITChBmp, DEF_BIT(ch_nbr));
            DWCOTGHS_CH_EN(reg_val, p_reg->HCH[ch_nbr]);

        } else {                                                /* Resend Complete SPLIT packet for bulk/control EP.    */
            CPU_CRITICAL_ENTER();
            if (p_drv_data->CSPLITChBmp == 0u) {
                DEF_BIT_SET(p_reg->GINTMSK, DWCOTGHS_GINTx_SOF);
            }
            DEF_BIT_SET(p_drv_data->CSPLITChBmp, DEF_BIT(ch_nbr));
            p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt = p_drv_data->SOFCtr + 1u;
            CPU_CRITICAL_EXIT();
        }

    } else if ((p_reg->HCH[ch_nbr].HCINTx                & DWCOTGHS_HCINTx_ACK        ) &&
               (DEF_BIT_IS_CLR(p_reg->HCH[ch_nbr].HCSPLTx, DWCOTGHS_HCSPLTx_COMPLSPLT))) {
        p_reg->HCH[ch_nbr].HCINTx = DWCOTGHS_HCINTx_ACK;

        CPU_CRITICAL_ENTER();                                   /* Send Complete SPLIT if Start SPLIT was successful    */
        if (p_drv_data->CSPLITChBmp == 0u) {
            DEF_BIT_SET(p_reg->GINTMSK, DWCOTGHS_GINTx_SOF);
            if (ep_type == USBH_EP_TYPE_INTR) {
                p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt = p_drv_data->SOFCtr + 1u;
            } else {
                p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt = p_drv_data->SOFCtr + 5u;
            }

        } else {
            p_drv_data->ChInfoTbl[ch_nbr].CSPLITCnt += p_drv_data->SOFCtr;
        }
        DEF_BIT_SET(p_drv_data->CSPLITChBmp, DEF_BIT(ch_nbr));
        CPU_CRITICAL_EXIT();
    }
}


/*
*********************************************************************************************************
*                                          DWCOTGHS_ChInit()
*
* Description : Initialize channel configurations based on the endpoint direction and characteristics.
*
* Argument(s) : p_reg      Pointer to DWC OTG HS registers structure.
*
*               p_urb      Pointer to URB structure.
*
*               ch_nbr     Channe number to initialize
*
*               p_err     Pointer to variable that will receive the return error code from this
*                         function.
*                             USBH_ERR_NONE           Channel initialized successfuly.
*                             Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  DWCOTGHS_ChInit (USBH_DWCOTGHS_REG  *p_reg,
                               USBH_URB           *p_urb,
                               CPU_INT08U          ch_nbr,
                               USBH_ERR           *p_err)
{
    USBH_DRV_DATA  *p_drv_data;
    CPU_INT32U      reg_val;
    CPU_INT08U      ep_nbr;
    CPU_INT08U      ep_type;
    CPU_INT16U      ep_pkt_size;
    LIB_ERR         err_lib;


    p_drv_data  = (USBH_DRV_DATA *)p_urb->EP_Ptr->DevPtr->HC_Ptr->HC_Drv.DataPtr;
    ep_nbr      =  USBH_EP_LogNbrGet(p_urb->EP_Ptr);
    ep_type     =  USBH_EP_TypeGet(p_urb->EP_Ptr);
    ep_pkt_size =  USBH_EP_MaxPktSizeGet(p_urb->EP_Ptr);
   *p_err       =  USBH_ERR_NONE;

    if (ep_pkt_size > p_drv_data->DrvMemPool.BlkSize) {
        *p_err = USBH_ERR_HC_ALLOC;
        return;
    }

#if (DWCOTGHS_DRV_DEBUG == DEF_ENABLED)                         /*check if DMA Buffer pointer has not been released     */
    if (p_urb->DMA_BufPtr != (void *)0u) {
        while(1); /* !!!!! should never happen */
    }
#endif

    if (p_urb->DMA_BufPtr == (void *)0u) {
        p_urb->DMA_BufPtr = Mem_PoolBlkGet(&p_drv_data->DrvMemPool,
                                            p_drv_data->DrvMemPool.BlkSize,
                                           &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
           *p_err = USBH_ERR_HC_ALLOC;
            return;
        }
    }

    p_reg->HCH[ch_nbr].HCINTx    =  0x000007FFu;                /* Clear old interrupt conditions for this host channel */
                                                                /* Enable channel interrupts required for this transfer */
    p_reg->HCH[ch_nbr].HCINTMSKx = (DWCOTGHS_HCINTx_STALL   |
                                    DWCOTGHS_HCINTx_TXERR   |
                                    DWCOTGHS_HCINTx_AHBERR  |
                                    DWCOTGHS_HCINTx_NAK     |
                                    DWCOTGHS_HCINTx_DTERR);     /* if the host receives a data toggle error.            */

    if (p_urb->Token == USBH_TOKEN_IN) {
        p_reg->HCH[ch_nbr].HCINTMSKx |= DWCOTGHS_HCINTx_BBERR;  /* if the host receives more data than the max pkt size */
        if (ep_type == USBH_EP_TYPE_INTR) {
            p_reg->HCH[ch_nbr].HCINTMSKx |= DWCOTGHS_HCINTx_FRMOR;
        }
    }

    p_reg->HAINTMSK |= (1u << ch_nbr);                          /* Enable the top level host channel interrupt.         */

    reg_val = ((p_urb->EP_Ptr->DevAddr << 22u)  |               /* Set the device address to which the channel belongs  */
               (ep_nbr                 << 11u)  |               /* Set the associated EP logical number                 */
               (ep_type                << 18u)  |               /* Set the endpoint type                                */
               (ep_pkt_size & DWCOTGHS_HCCHARx_MPSIZ_MSK));     /* Set the maximum endpoint size                        */

    if (p_urb->Token == USBH_TOKEN_IN) {                        /* Set the direction of the channel                     */
        DEF_BIT_SET(reg_val, DWCOTGHS_HCCHARx_EPDIR);
    }

    if (p_urb->EP_Ptr->DevSpd == USBH_DEV_SPD_LOW) {            /* indicate to the channel is communicating to a LS dev */
        DEF_BIT_SET(reg_val, DWCOTGHS_HCCHARx_LSDEV);
    }

    p_reg->HCH[ch_nbr].HCCHARx = reg_val;                       /* Init Host Channel #N                                 */
}


/*
*********************************************************************************************************
*                                        DWCOTGHS_ChXferStart()
*
* Description : Start receive/transmit by configuring HCTSIZx and HCCHARx register for the Host channel
*
* Argument(s) : p_reg         Pointer to DWC OTG HS registers structure.
*
*               p_ch_info     Pointer to host driver channel information data structure.
*
*               p_urb         Pointer to URB structure.
*
*               ch_nbr        Host channel number.
*
*               p_err         Pointer to variable that will receive the return error code from this
*                             function.
*                                 USBH_ERR_NONE           Setup RX transfer channel successfuly.
*                                 Specific error code     otherwise.
*
* Return(s)   : None.
*
* Note(s)     : (1) For multi-transaction transfer, the Core will manage the data toggling for each
*                   transaction sent to the USB Device.
*               (2) For an IN, HCTSIZx[XferSize] contains the buffer size that the application has
*                   reserved for the transfer. The application is expected to program this field as an
*                   integer multiple of the maximum size for IN transactions (periodic and non-periodic).
*********************************************************************************************************
*/

static  void  DWCOTGHS_ChXferStart (USBH_DWCOTGHS_REG      *p_reg,
                                    USBH_DWCOTGHS_CH_INFO  *p_ch_info,
                                    USBH_URB               *p_urb,
                                    CPU_INT08U              ch_nbr,
                                    USBH_ERR               *p_err)
{
    USBH_DRV_DATA  *p_drv_data;
    CPU_INT32U      reg_val;
    CPU_INT16U      nbr_pkts;
    CPU_INT16U      max_pkt_size;
    CPU_INT08U      is_oddframe;
    CPU_BOOLEAN     zlp_flag = DEF_NO;


    p_drv_data             = (USBH_DRV_DATA *)p_urb->EP_Ptr->DevPtr->HC_Ptr->HC_Drv.DataPtr;
    max_pkt_size           =  USBH_EP_MaxPktSizeGet(p_urb->EP_Ptr);
    nbr_pkts               =  1u;                               /* used for zero length packet.                         */
    p_ch_info->NextXferLen =  p_urb->UserBufLen - p_urb->XferLen;
    *p_err                 =  USBH_ERR_NONE;

    if (p_urb->UserBufLen == 0) {
        zlp_flag = DEF_YES;
    }

    if (p_urb->Token == USBH_TOKEN_IN) {
        p_ch_info->NextXferLen = DEF_MIN(p_ch_info->NextXferLen, p_drv_data->DrvMemPool.BlkSize);
    } else {
        if (p_ch_info->DoSplit == DEF_YES) {
            p_ch_info->NextXferLen = DEF_MIN(max_pkt_size, p_ch_info->NextXferLen);
        } else {
            p_ch_info->NextXferLen = DEF_MIN(p_ch_info->NextXferLen, p_drv_data->DrvMemPool.BlkSize);
        }
    }

    if (zlp_flag == DEF_NO) {
        nbr_pkts = (p_ch_info->NextXferLen + max_pkt_size - 1u) / max_pkt_size;
    }

    if (p_urb->Token == USBH_TOKEN_IN) {
        p_ch_info->AppBufLen = nbr_pkts * max_pkt_size;         /* See Note #2                                          */
    } else {
        p_ch_info->AppBufLen = p_ch_info->NextXferLen;
    }

    reg_val  = 0u;                                              /* Initialize the HCTSIZx register                      */
    reg_val  = ((CPU_INT32U)p_urb->EP_Ptr->DataPID) << 29u;     /* PID. See Note #1                                     */
    reg_val |= ((CPU_INT32U)nbr_pkts)               << 19u;     /* Packet Count                                         */
    reg_val |= (p_ch_info->AppBufLen & DWCOTGHS_HCTSIZx_XFRSIZ_MSK); /* Transfer Size                                   */
    p_reg->HCH[ch_nbr].HCTSIZx = reg_val;

    Mem_Clr(p_urb->DMA_BufPtr, p_ch_info->NextXferLen);
    if (p_urb->Token != USBH_TOKEN_IN) {
        Mem_Copy(                     p_urb->DMA_BufPtr,
                 (void *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen),
                                      p_ch_info->NextXferLen);
    }

    is_oddframe = (p_reg->HFNUM & 0x01u) ? 0u : 1u;             /* Host perform a xfer in an odd/even (micro)frame.     */
    p_reg->HCH[ch_nbr].HCCHARx &= ~DWCOTGHS_HCCHARx_ODDFRM;
    p_reg->HCH[ch_nbr].HCCHARx |= (is_oddframe << 29u);

    DEF_BIT_SET(p_reg->HCH[ch_nbr].HCINTMSKx, DWCOTGHS_HCINTx_CHH);
    p_reg->HCH[ch_nbr].HCDMAx = (CPU_INT32U)p_urb->DMA_BufPtr;

    DWCOTGHS_ChEnable(&p_reg->HCH[ch_nbr], p_urb);
}


/*
*********************************************************************************************************
*                                         DWCOTGHS_ChEnable()
*
* Description : Function will clear specific bits before enabling the host channel to start transfer.
*
* Argument(s) : p_ch_reg     Pointer to DWC OTG HS channel registers structure.
*
*               p_urb        Pointer to URB structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  DWCOTGHS_ChEnable (USBH_DWCOTGHS_HOST_CH_REG  *p_ch_reg,
                                 USBH_URB                   *p_urb)
{
    CPU_INT32U  reg_val;


    p_ch_reg->HCINTx   =  0x000007FFu;                          /* Clear old interrupt conditions for this host channel */
    p_ch_reg->HCSPLTx &= ~DWCOTGHS_HCSPLTx_COMPLSPLT;           /* Bit is only use for split transactions.              */
    p_ch_reg->HCTSIZx &= ~DWCOTGHS_HCTSIZx_DOPING;              /* Bit must be clear to continue NAKs                   */

    if (p_urb->Token != USBH_TOKEN_IN) {
        p_ch_reg->HCDMAx = (CPU_INT32U)p_urb->DMA_BufPtr;       /* Rewind buffer.                                       */
    }
                                                                /* --------------- ENABLE HOST CHANNEL ---------------- */
#if (DWCOTGHS_DRV_DEBUG == DEF_ENABLED)                         /* Check if Channel is in unknown state.                */
    do {
        reg_val = p_ch_reg->HCCHARx;
    }
    while (reg_val & (DWCOTGHS_HCCHARx_CHDIS | DWCOTGHS_HCCHARx_CHENA));
#endif

    DWCOTGHS_CH_EN(reg_val, *p_ch_reg);
}


/*
*********************************************************************************************************
*                                         DWCOTGHS_GetChNbr()
*
* Description : Get the host channel number corresponding to a given device address, endpoint number
*               and direction.
*
* Argument(s) : p_drv_data     Pointer to host driver data structure.
*
*               p_ep           Pointer to endpoint structure.
*
* Return(s)   : Host channel number or 0xFF if no host channel is found
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  DWCOTGHS_GetChNbr (USBH_DRV_DATA  *p_drv_data,
                                       USBH_EP        *p_ep)
{
    CPU_INT08U  ch_nbr;
    CPU_INT16U  ep_addr;


    ep_addr = ((p_ep->DevAddr << 8u)  |
                p_ep->Desc.bEndpointAddress);

    for (ch_nbr = 0u; ch_nbr < p_drv_data->ChMaxNbr; ch_nbr++) {
        if (p_drv_data->ChInfoTbl[ch_nbr].EP_Addr == ep_addr) {
            return (ch_nbr);
        }
    }

    return DWCOTGHS_INVALID_CH;
}


/*
*********************************************************************************************************
*                                       DWCOTGHS_GetFreeChNbr()
*
* Description : Allocate a free host channel number for the newly opened channel.
*
* Argument(s) : p_drv_data     Pointer to host driver data structure.
*
*               p_err          Pointer to an error.
*                                  USBH_ERR_NONE, if a number was successfully allocated
*                                  USBH_ERR_EP_ALLOC, if there is no channel available
*
* Return(s)   : Free channel number or 0xFF if no host channel is found
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  DWCOTGHS_GetFreeChNbr (USBH_DRV_DATA  *p_drv_data,
                                           USBH_ERR       *p_err)
{
    CPU_INT08U  ch_nbr;


    for (ch_nbr = 0u; ch_nbr < p_drv_data->ChMaxNbr; ch_nbr++) {
        if (DEF_BIT_IS_CLR(p_drv_data->ChUsed, DEF_BIT(ch_nbr))) {
            DEF_BIT_SET(p_drv_data->ChUsed, DEF_BIT(ch_nbr));
           *p_err = USBH_ERR_NONE;
            return (ch_nbr);
        }
    }

   *p_err = USBH_ERR_EP_ALLOC;
    return DWCOTGHS_INVALID_CH;
}


/*
*********************************************************************************************************
*                                       DWCOTGHS_URB_ProcTask()
*
* Description : The task handles Interrupt EP NAK by starting an OS timer, as well as handles additional
*               EP IN/OUT transactions when needed.
*
* Argument(s) : p_arg     Pointer to the argument passed to 'DWCOTGHS_URB_ProcTask()' by
*                         'USBH_OS_TaskCreate()'.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void  DWCOTGHS_URB_ProcTask (void  *p_arg)
{
    USBH_HC_DRV        *p_hc_drv;
    USBH_DRV_DATA      *p_drv_data;
    USBH_DWCOTGHS_REG  *p_reg;
    USBH_URB           *p_urb;
    CPU_INT32U          xfer_len;
    CPU_INT08U          ch_nbr;
    CPU_INT16U          pkt_cnt;
    USBH_ERR            p_err;
    CPU_SR_ALLOC();


    p_hc_drv   = (USBH_HC_DRV       *)p_arg;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    while (DEF_TRUE) {
        p_urb = (USBH_URB *)USBH_OS_MsgQueueGet( DWCOTGHS_URB_Proc_Q,
                                                 0u,
                                                &p_err);
        if (p_err != USBH_ERR_NONE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("DRV: Could not get URB from Queue.\r\n");
#endif
        }

        ch_nbr = DWCOTGHS_GetChNbr(p_drv_data, p_urb->EP_Ptr);

        if ((p_urb->Err == USBH_ERR_EP_NACK) &&
            (ch_nbr     != DWCOTGHS_INVALID_CH)) {              /* ------------- START INTR EP NAK TIMER -------------- */
            p_err = USBH_OS_TmrStart(p_drv_data->ChInfoTbl[ch_nbr].Tmr);

        } else if (ch_nbr != DWCOTGHS_INVALID_CH) {
            CPU_CRITICAL_ENTER();
            xfer_len = DWCOTGHS_GET_XFRSIZ(p_reg->HCH[ch_nbr].HCTSIZx);
            xfer_len = (p_drv_data->ChInfoTbl[ch_nbr].AppBufLen - xfer_len);

            if (p_urb->Token == USBH_TOKEN_IN) {                /* -------------- HANDLE IN TRANSACTIONS -------------- */
                Mem_Copy((void *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen),
                                              p_urb->DMA_BufPtr,
                                              xfer_len);

                if (xfer_len > p_drv_data->ChInfoTbl[ch_nbr].NextXferLen) {  /* Rx'd more data than what was expected   */
                    p_urb->XferLen += p_drv_data->ChInfoTbl[ch_nbr].NextXferLen;
                    p_urb->Err      = USBH_ERR_HC_IO;
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                    USBH_PRINT_LOG("DRV: Rx'd more data than was expected.\r\n");
#endif
                    USBH_URB_Done(p_urb);                       /* Notify the Core layer about the URB completion       */

                } else {
                    p_urb->XferLen += xfer_len;
                }

            } else {                                            /* ------------- HANDLE OUT TRANSACTIONS -------------- */
                pkt_cnt = DWCOTGHS_GET_PKTCNT(p_reg->HCH[ch_nbr].HCTSIZx); /* Get packet count                          */
                if (pkt_cnt == 0u) {
                    p_urb->XferLen += p_drv_data->ChInfoTbl[ch_nbr].NextXferLen;
                } else {
                    p_urb->XferLen += (p_drv_data->ChInfoTbl[ch_nbr].NextXferLen - xfer_len);
                }
            }

            if (p_urb->Err == USBH_ERR_NONE) {
                DWCOTGHS_ChXferStart( p_reg,
                                     &p_drv_data->ChInfoTbl[ch_nbr],
                                      p_urb,
                                      ch_nbr,
                                     &p_err);
            }
            CPU_CRITICAL_EXIT();
        }
    }
}


/*
*********************************************************************************************************
*                                       DWCOTGHS_TmrCallback()
*
* Description : Timer callback function to re-submit an URB in case of a Interrupt EP NAK.
*
* Argument(s) : p_tmr     Pointer to OS timer.
*
*               p_arg     Pointer to argument passed by 'USBH_OS_TmrCreate()'.
*
* Return(s)   : None.
*
* Note(s)     : (1) OS Scheduler is locked during Tmr callback function.
*********************************************************************************************************
*/

static  void  DWCOTGHS_TmrCallback (void  *p_tmr,
                                    void  *p_arg)
{
    USBH_URB           *p_urb;
    USBH_HC_DRV        *p_hc_drv;
    USBH_DRV_DATA      *p_drv_data;
    USBH_DWCOTGHS_REG  *p_reg;
    CPU_INT08U          ch_nbr;


    p_urb      = (USBH_URB          *)p_arg;
    p_hc_drv   = (USBH_HC_DRV       *)&p_urb->EP_Ptr->DevPtr->HC_Ptr->HC_Drv;
    p_drv_data = (USBH_DRV_DATA     *)p_hc_drv->DataPtr;
    p_reg      = (USBH_DWCOTGHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    ch_nbr = DWCOTGHS_GetChNbr(p_drv_data, p_urb->EP_Ptr);
    if (ch_nbr != DWCOTGHS_INVALID_CH) {                        /* See Note 1                                           */
        DWCOTGHS_ChEnable(&p_reg->HCH[ch_nbr], p_urb);
    }
}

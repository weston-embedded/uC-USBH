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
*
*                                       RENESAS USB HIGH-SPEED
*
* Filename : usbh_hcd_renesas_usbhs.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define  USBH_HCD_RENESAS_USBHS_MODULE
#include  "usbh_hcd_renesas_usbhs.h"
#include  "../../Source/usbh_hub.h"
#include  "../../Source/usbh_os.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*
* Note(s):  (1) Driver works for RZ, RX64M, and RX71M USBHS Host controller. Among them, RZ USBHS has
*               more pipes (16 pipes) than the RX64M/RX71M USBHS (10 pipes).
*
*           (2) USBHS controller has a FIFO buffer area used for all its pipes. Buffer allocation
*               setup requires to configure the first 64-byte block number allocated to a specified
*               pipe in the register PIPEBUF. The first 8 blocks are taken by PIPE0 (i.e. control pipe)
*               and PIPE 6 to 9 (i.e. interrupt pipes). Thus the first block free for bulk or isochronous
*               pipe is block #9 (that is index #8).
*********************************************************************************************************
*/

#define  RENESAS_USBHS_PIPE_QTY                           16u   /* Max nbr of pipes available (see Note #1).            */
#define  RENESAS_USBHS_DFIFO_QTY                           2u   /* Nbr of DFIFO channel avail.                          */

                                                                /* Bit field that represents the avail DFIFO ch.        */
#define  RENESAS_USBHS_DFIFO_MASK     DEF_BIT_FIELD(RENESAS_USBHS_DFIFO_QTY, 0u)
#define  RENESAS_USBHS_CFIFO                       DEF_BIT_07   /* Bit that represents CFIFO.                           */
#define  RENESAS_USBHS_FIFO_NONE                        0xFFu   /* Bit that represents no FIFO.                         */

#define  RENESAS_USBHS_BUF_STARTING_IX                     8u   /* FIFO buf start ix (see Note #2).                     */
#define  RENESAS_USBHS_BUF_LEN                            64u   /* Length of single buf in FIFO.                        */
#define  RENESAS_USBHS_BUF_QTY_AVAIL                     128u   /* FIFO is 8K long -> 128 buf avail (128 * 64).         */
#define  RENESAS_USBHS_BUF_IX_NONE                         0u

#define  RENESAS_USBHS_PIPETRN_IX_NONE                  0xFFu   /* No PIPETRN ix for this pipe.                         */

#define  RENESAS_USBHS_RX_Q_SIZE                           4u   /* Size of the RX Q (useful in dbl buf mode).           */

#define  RENESAS_USBHS_RETRY_QTY                         200u
#define  RENESAS_USBHS_REG_TO                      0x0000FFFFu  /* Timeout val when actively pending.                   */

#define  RENESAS_USBHS_DEV_CONN_STATUS_NONE        DEF_BIT_NONE


/*
*********************************************************************************************************
*                                USB HARDWARE REGISTER BIT DEFINITIONS
*********************************************************************************************************
*/

                                                                /* ------ SYSTEM CONFIGURATION CONTROL REGISTER ------- */
#define  RENESAS_USBHS_SYSCFG0_USBE                   DEF_BIT_00
#define  RENESAS_USBHS_SYSCFG0_UPLLE                  DEF_BIT_01
#define  RENESAS_USBHS_SYSCFG0_UCKSEL                 DEF_BIT_02
#define  RENESAS_USBHS_SYSCFG0_DPRPU                  DEF_BIT_04
#define  RENESAS_USBHS_SYSCFG0_DRPD                   DEF_BIT_05
#define  RENESAS_USBHS_SYSCFG0_DCFM                   DEF_BIT_06
#define  RENESAS_USBHS_SYSCFG0_HSE                    DEF_BIT_07
#define  RENESAS_USBHS_SYSCFG0_CNEN                   DEF_BIT_08


                                                                /* -------------- CPU BUS WAIT REGISTER --------------- */
#define  RENESAS_USBHS_BUSWAIT_BWAIT_MASK             DEF_BIT_FIELD(5u, 0u)


                                                                /* ------- SYSTEM CONFIGURATION STATUS REGISTER ------- */
#define  RENESAS_USBHS_SYSSTS0_LNST_NONE              DEF_BIT_NONE
#define  RENESAS_USBHS_SYSSTS0_LNST_MASK              DEF_BIT_FIELD(2u, 0u)


                                                                /* ---------- DEVICE STATE CONTROL REGISTER ----------- */
#define  RENESAS_USBHS_DVSCTR_RHST_MASK               DEF_BIT_FIELD(3u, 0u)
#define  RENESAS_USBHS_DVSCTR_UACT                    DEF_BIT_04
#define  RENESAS_USBHS_DVSCTR_RESUME                  DEF_BIT_05
#define  RENESAS_USBHS_DVSCTR_USBRST                  DEF_BIT_06
#define  RENESAS_USBHS_DVSCTR_RWUPE                   DEF_BIT_07
#define  RENESAS_USBHS_DVSCTR_WKUP                    DEF_BIT_08

#define  RENESAS_USBHS_DVSCTR_RHST_NONE               DEF_BIT_NONE
#define  RENESAS_USBHS_DVSCTR_RHST_RESET              DEF_BIT_02
#define  RENESAS_USBHS_DVSCTR_RHST_LS                 DEF_BIT_00
#define  RENESAS_USBHS_DVSCTR_RHST_FS                 DEF_BIT_01
#define  RENESAS_USBHS_DVSCTR_RHST_HS                (DEF_BIT_01 | DEF_BIT_00)


                                                                /* ------------ DMA-FIFO BUS CFG REGISTER ------------- */
#define  RENESAS_USBHS_DxFBCFG_TENDE                  DEF_BIT_04


                                                                /* ------------ CFIFO PORT SELECT REGISTER ------------ */
#define  RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK          DEF_BIT_FIELD(4u, 0u)
#define  RENESAS_USBHS_CFIFOSEL_ISEL                  DEF_BIT_05
#define  RENESAS_USBHS_CFIFOSEL_BIGEND                DEF_BIT_08
#define  RENESAS_USBHS_CFIFOSEL_MBW_MASK              DEF_BIT_FIELD(2u, 10u)
#define  RENESAS_USBHS_CFIFOSEL_REW                   DEF_BIT_14
#define  RENESAS_USBHS_CFIFOSEL_RCNT                  DEF_BIT_15

#define  RENESAS_USBHS_CFIFOSEL_MBW_8                 DEF_BIT_NONE
#define  RENESAS_USBHS_CFIFOSEL_MBW_16                DEF_BIT_10
#define  RENESAS_USBHS_CFIFOSEL_MBW_32                DEF_BIT_11


                                                                /* ------------ DFIFO PORT SELECT REGISTER ------------ */
#define  RENESAS_USBHS_DxFIFOSEL_CURPIPE_MASK         DEF_BIT_FIELD(4u, 0u)
#define  RENESAS_USBHS_DxFIFOSEL_BIGEND               DEF_BIT_08
#define  RENESAS_USBHS_DxFIFOSEL_MBW_MASK             DEF_BIT_FIELD(2u, 10u)
#define  RENESAS_USBHS_DxFIFOSEL_DREQE                DEF_BIT_12
#define  RENESAS_USBHS_DxFIFOSEL_DCLRM                DEF_BIT_13
#define  RENESAS_USBHS_DxFIFOSEL_REW                  DEF_BIT_14
#define  RENESAS_USBHS_DxFIFOSEL_RCNT                 DEF_BIT_15


                                                                /* ----------- CFIFO PORT CONTROL REGISTER ------------ */
#define  RENESAS_USBHS_FIFOCTR_DTLN_MASK              DEF_BIT_FIELD(12u, 0u)
#define  RENESAS_USBHS_FIFOCTR_FRDY                   DEF_BIT_13
#define  RENESAS_USBHS_FIFOCTR_BCLR                   DEF_BIT_14
#define  RENESAS_USBHS_FIFOCTR_BVAL                   DEF_BIT_15


                                                                /* ----------- INTERRUPT ENABLE REGISTER 0 ------------ */
#define  RENESAS_USBHS_INTENB0_BRDYE                  DEF_BIT_08
#define  RENESAS_USBHS_INTENB0_NRDYE                  DEF_BIT_09
#define  RENESAS_USBHS_INTENB0_BEMPE                  DEF_BIT_10
#define  RENESAS_USBHS_INTENB0_CTRE                   DEF_BIT_11
#define  RENESAS_USBHS_INTENB0_DVSE                   DEF_BIT_12
#define  RENESAS_USBHS_INTENB0_SOFE                   DEF_BIT_13
#define  RENESAS_USBHS_INTENB0_RSME                   DEF_BIT_14
#define  RENESAS_USBHS_INTENB0_VBSE                   DEF_BIT_15

                                                                /* ----------- INTERRUPT ENABLE REGISTER 1 ------------ */
#define  RENESAS_USBHS_INTENB1_SACKE                  DEF_BIT_04
#define  RENESAS_USBHS_INTENB1_SIGNE                  DEF_BIT_05
#define  RENESAS_USBHS_INTENB1_EOFERRE                DEF_BIT_06
#define  RENESAS_USBHS_INTENB1_ATTCHE                 DEF_BIT_11
#define  RENESAS_USBHS_INTENB1_DTCHE                  DEF_BIT_12
#define  RENESAS_USBHS_INTENB1_BCHGE                  DEF_BIT_14

                                                                /* ------------- SOF OUTPUT CFG REGISTER -------------- */
#define  RENESAS_USBHS_SOFCFG_BRDYM                   DEF_BIT_06
#define  RENESAS_USBHS_SOFCFG_TRNENSEL                DEF_BIT_08

                                                                /* ----------- INTERRUPT STATUS REGISTER 0 ------------ */
#define  RENESAS_USBHS_INTSTS0_CTSQ_MASK              DEF_BIT_FIELD(3u, 0u)
#define  RENESAS_USBHS_INTSTS0_VALID                  DEF_BIT_03
#define  RENESAS_USBHS_INTSTS0_DVSQ_MASK              DEF_BIT_FIELD(2u, 4u)
#define  RENESAS_USBHS_INTSTS0_DVSQ_SUSP              DEF_BIT_06
#define  RENESAS_USBHS_INTSTS0_VBSTS                  DEF_BIT_07
#define  RENESAS_USBHS_INTSTS0_BRDY                   DEF_BIT_08
#define  RENESAS_USBHS_INTSTS0_NRDY                   DEF_BIT_09
#define  RENESAS_USBHS_INTSTS0_BEMP                   DEF_BIT_10
#define  RENESAS_USBHS_INTSTS0_CTRT                   DEF_BIT_11
#define  RENESAS_USBHS_INTSTS0_DVST                   DEF_BIT_12
#define  RENESAS_USBHS_INTSTS0_SOFR                   DEF_BIT_13
#define  RENESAS_USBHS_INTSTS0_RESM                   DEF_BIT_14
#define  RENESAS_USBHS_INTSTS0_VBINT                  DEF_BIT_15

#define  RENESAS_USBHS_INTSTS0_CTSQ_SETUP             DEF_BIT_NONE
#define  RENESAS_USBHS_INTSTS0_CTSQ_RD_DATA           DEF_BIT_00
#define  RENESAS_USBHS_INTSTS0_CTSQ_RD_STATUS         DEF_BIT_01
#define  RENESAS_USBHS_INTSTS0_CTSQ_WR_DATA           (DEF_BIT_00 | DEF_BIT_01)
#define  RENESAS_USBHS_INTSTS0_CTSQ_WR_STATUS         DEF_BIT_02
#define  RENESAS_USBHS_INTSTS0_CTSQ_WR_STATUS_NO_DATA (DEF_BIT_00 | DEF_BIT_02)
#define  RENESAS_USBHS_INTSTS0_CTSQ_SEQ_ERR           (DEF_BIT_01 | DEF_BIT_02)

#define  RENESAS_USBHS_INTSTS0_DVSQ_POWERED           DEF_BIT_NONE
#define  RENESAS_USBHS_INTSTS0_DVSQ_DFLT              DEF_BIT_04
#define  RENESAS_USBHS_INTSTS0_DVSQ_ADDR              DEF_BIT_05
#define  RENESAS_USBHS_INTSTS0_DVSQ_CONFIG           (DEF_BIT_04 | DEF_BIT_05)


                                                                /* ----------- INTERRUPT STATUS REGISTER 1 ------------ */
#define  RENESAS_USBHS_INTSTS1_SACK                   DEF_BIT_04
#define  RENESAS_USBHS_INTSTS1_SIGN                   DEF_BIT_05
#define  RENESAS_USBHS_INTSTS1_EOFERR                 DEF_BIT_06
#define  RENESAS_USBHS_INTSTS1_ATTCH                  DEF_BIT_11
#define  RENESAS_USBHS_INTSTS1_DTCH                   DEF_BIT_12
#define  RENESAS_USBHS_INTSTS1_BCHG                   DEF_BIT_14
#define  RENESAS_USBHS_INTSTS1_OVRCR                  DEF_BIT_15


                                                                /* -------------- FRAME NUMBER REGISTER --------------- */
#define  RENESAS_USBHS_FRMNUM_FRNM_MASK               DEF_BIT_FIELD(11u, 0u)
#define  RENESAS_USBHS_FRMNUM_CRCE                    DEF_BIT_14
#define  RENESAS_USBHS_FRMNUM_OVRN                    DEF_BIT_15


                                                                /* -------------- UFRAME NUMBER REGISTER -------------- */
#define  RENESAS_USBHS_UFRMNUM_UFRNM_MASK             DEF_BIT_FIELD(3u, 0u)


                                                                /* --------------- USB ADDRESS REGISTER --------------- */
#define  RENESAS_USBHS_USBADDR_USBADDR_MASK           DEF_BIT_FIELD(7u, 0u)


                                                                /* ------------ USB REQUEST TYPE REGISTER ------------- */
#define  RENESAS_USBHS_USBREQ_BMREQUESTTYPE_MASK      DEF_BIT_FIELD(8u, 0u)
#define  RENESAS_USBHS_USBREQ_BREQUEST_MASK           DEF_BIT_FIELD(8u, 8u)


                                                                /* ------------ DCP CONFIGURATION REGISTER ------------ */
#define  RENESAS_USBHS_DCPCFG_DIR                     DEF_BIT_04


                                                                /* ------------ DCP MAX PKT SIZE REGISTER ------------- */
#define  RENESAS_USBHS_DCPMAXP_MXPS_MASK              DEF_BIT_FIELD(7u, 0u)
#define  RENESAS_USBHS_DCPMAXP_DEVSEL_MASK            DEF_BIT_FIELD(4u, 12u)


                                                                /* --------------- DCP CONTROL REGISTER --------------- */
#define  RENESAS_USBHS_DCPCTR_PID_MASK                DEF_BIT_FIELD(2u, 0u)
#define  RENESAS_USBHS_DCPCTR_CCPL                    DEF_BIT_02
#define  RENESAS_USBHS_DCPCTR_PINGE                   DEF_BIT_04
#define  RENESAS_USBHS_DCPCTR_PBUSY                   DEF_BIT_05
#define  RENESAS_USBHS_DCPCTR_SQMON                   DEF_BIT_06
#define  RENESAS_USBHS_DCPCTR_SQSET                   DEF_BIT_07
#define  RENESAS_USBHS_DCPCTR_SQCLR                   DEF_BIT_08
#define  RENESAS_USBHS_DCPCTR_SUREQCLR                DEF_BIT_11
#define  RENESAS_USBHS_DCPCTR_CSSTS                   DEF_BIT_12
#define  RENESAS_USBHS_DCPCTR_CSCLR                   DEF_BIT_13
#define  RENESAS_USBHS_DCPCTR_SUREQ                   DEF_BIT_14
#define  RENESAS_USBHS_DCPCTR_BSTS                    DEF_BIT_15


                                                                /* ----------- PIPE WINDOW SELECT REGISTER ------------ */
#define  RENESAS_USBHS_PIPESEL_PIPESEL_MASK           DEF_BIT_FIELD(4u, 0u)


                                                                /* ----------- PIPE CONFIGURATION REGISTER ------------ */
#define  RENESAS_USBHS_PIPCFG_EPNUM_MASK              DEF_BIT_FIELD(4u, 0u)
#define  RENESAS_USBHS_PIPCFG_DIR                     DEF_BIT_04
#define  RENESAS_USBHS_PIPCFG_SHTNAK                  DEF_BIT_07
#define  RENESAS_USBHS_PIPCFG_CNTMD                   DEF_BIT_08
#define  RENESAS_USBHS_PIPCFG_DBLB                    DEF_BIT_09
#define  RENESAS_USBHS_PIPCFG_BFRE                    DEF_BIT_10

#define  RENESAS_USBHS_PIPCFG_TYPE_MASK               DEF_BIT_FIELD(2u, 14u)
#define  RENESAS_USBHS_PIPCFG_TYPE_NONE               DEF_BIT_NONE
#define  RENESAS_USBHS_PIPCFG_TYPE_BULK               DEF_BIT_14
#define  RENESAS_USBHS_PIPCFG_TYPE_INTR               DEF_BIT_15
#define  RENESAS_USBHS_PIPCFG_TYPE_ISOC              (DEF_BIT_14 | DEF_BIT_15)


                                                                /* ----------- PIPE BUFFER SETTING REGISTER ----------- */
#define  RENESAS_USBHS_PIPEBUF_BUFNMB_MASK            DEF_BIT_FIELD(8u, 0u)
#define  RENESAS_USBHS_PIPEBUF_BUFSIZE_MASK           DEF_BIT_FIELD(5u, 10u)


                                                                /* ------------ PIPE MAX PKT SIZE REGISTER ------------ */
#define  RENESAS_USBHS_PIPEMAXP_MXPS_MASK             DEF_BIT_FIELD(11u, 0u)
#define  RENESAS_USBHS_PIPEMAXP_DEVSEL_MASK           DEF_BIT_FIELD(4u, 12u)


                                                                /* ----------- PIPE TIMING CONTROL REGISTER ----------- */
#define  RENESAS_USBHS_PIPERI_IITV_MASK               DEF_BIT_FIELD(3u, 0u)
#define  RENESAS_USBHS_PIPERI_IFIS                    DEF_BIT_12


                                                                /* ------------- PIPE N CONTROL REGISTER -------------- */
#define  RENESAS_USBHS_PIPExCTR_PID_MASK              DEF_BIT_FIELD(2u, 0u)
#define  RENESAS_USBHS_PIPExCTR_PBUSY                 DEF_BIT_05
#define  RENESAS_USBHS_PIPExCTR_SQMON                 DEF_BIT_06
#define  RENESAS_USBHS_PIPExCTR_SQSET                 DEF_BIT_07
#define  RENESAS_USBHS_PIPExCTR_SQCLR                 DEF_BIT_08
#define  RENESAS_USBHS_PIPExCTR_ACLRM                 DEF_BIT_09
#define  RENESAS_USBHS_PIPExCTR_ATREPM                DEF_BIT_10
#define  RENESAS_USBHS_PIPExCTR_CSSTS                 DEF_BIT_12
#define  RENESAS_USBHS_PIPExCTR_CSCLR                 DEF_BIT_13
#define  RENESAS_USBHS_PIPExCTR_INBUFM                DEF_BIT_14
#define  RENESAS_USBHS_PIPExCTR_BSTS                  DEF_BIT_15

#define  RENESAS_USBHS_PIPExCTR_PID_NAK               DEF_BIT_NONE
#define  RENESAS_USBHS_PIPExCTR_PID_BUF               DEF_BIT_00
#define  RENESAS_USBHS_PIPExCTR_PID_STALL1            DEF_BIT_01
#define  RENESAS_USBHS_PIPExCTR_PID_STALL2           (DEF_BIT_00 | DEF_BIT_01)


                                                                /* -------- PIPE N TRANSACTION CNT EN REGISTER -------- */
#define  RENESAS_USBHS_PIPExTRE_TRCLR                 DEF_BIT_08
#define  RENESAS_USBHS_PIPExTRE_TRENB                 DEF_BIT_09


                                                                /* ---------- DEVICE ADDRESS N CFG REGISTER ----------- */
#define  RENESAS_USBHS_DEVADDx_USBSPD_MASK            DEF_BIT_FIELD(2u, 6u)
#define  RENESAS_USBHS_DEVADDx_HUBPORT_MASK           DEF_BIT_FIELD(3u, 8u)
#define  RENESAS_USBHS_DEVADDx_UPPHUB_MASK            DEF_BIT_FIELD(4u, 11u)

#define  RENESAS_USBHS_DEVADDx_USBSPD_NONE            DEF_BIT_NONE
#define  RENESAS_USBHS_DEVADDx_USBSPD_LOW             DEF_BIT_06
#define  RENESAS_USBHS_DEVADDx_USBSPD_FULL            DEF_BIT_07
#define  RENESAS_USBHS_DEVADDx_USBSPD_HIGH           (DEF_BIT_06 | DEF_BIT_07)


                                                                /* ------------ UTMI SUSPEND MODE REGISTER ------------ */
#define  RENESAS_USBHS_SUSPMODE_SUSPM                 DEF_BIT_14


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/
                                                                /* ---------- PIPTRN REG INDEX LOOK-UP TABLE ---------- */
static  const  CPU_INT08U  USBH_HCD_PIPETRN_LUT[][RENESAS_USBHS_PIPE_QTY] =
{
                                                                /* ------------------- TABLE FOR RZ ------------------- */
{
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 0 (DCP) -> No PIPTRN reg.                       */
    0u,                                                         /* PIPE 1       -> PIPTRN reg offset 0.                 */
    1u,                                                         /* PIPE 2       -> PIPTRN reg offset 1.                 */
    2u,                                                         /* PIPE 3       -> PIPTRN reg offset 2.                 */
    3u,                                                         /* PIPE 4       -> PIPTRN reg offset 3.                 */
    4u,                                                         /* PIPE 5       -> PIPTRN reg offset 4.                 */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 6       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 7       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 8       -> No PIPTRN reg.                       */
    10u,                                                        /* PIPE 9       -> PIPTRN reg offset 10.                */
    11u,                                                        /* PIPE A       -> PIPTRN reg offset 11.                */
    5u,                                                         /* PIPE B       -> PIPTRN reg offset 5.                 */
    6u,                                                         /* PIPE C       -> PIPTRN reg offset 6.                 */
    7u,                                                         /* PIPE D       -> PIPTRN reg offset 7.                 */
    8u,                                                         /* PIPE E       -> PIPTRN reg offset 8.                 */
    9u,                                                         /* PIPE F       -> PIPTRN reg offset 9.                 */
},
                                                                /* ----------------- TABLE FOR RX64M ------------------ */
{
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 0 (DCP) -> No PIPTRN reg.                       */
    0u,                                                         /* PIPE 1       -> PIPTRN reg offset 0.                 */
    1u,                                                         /* PIPE 2       -> PIPTRN reg offset 1.                 */
    2u,                                                         /* PIPE 3       -> PIPTRN reg offset 2.                 */
    3u,                                                         /* PIPE 4       -> PIPTRN reg offset 3.                 */
    4u,                                                         /* PIPE 5       -> PIPTRN reg offset 4.                 */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 6       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 7       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 8       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* No more pipes on RX64M.                              */
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
},

                                                                /* ----------------- TABLE FOR RX71M ------------------ */
{
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 0 (DCP) -> No PIPTRN reg.                       */
    0u,                                                         /* PIPE 1       -> PIPTRN reg offset 0.                 */
    1u,                                                         /* PIPE 2       -> PIPTRN reg offset 1.                 */
    2u,                                                         /* PIPE 3       -> PIPTRN reg offset 2.                 */
    3u,                                                         /* PIPE 4       -> PIPTRN reg offset 3.                 */
    4u,                                                         /* PIPE 5       -> PIPTRN reg offset 4.                 */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 6       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 7       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* PIPE 8       -> No PIPTRN reg.                       */
    RENESAS_USBHS_PIPETRN_IX_NONE,                              /* No more pipes on RX71M.                              */
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
    RENESAS_USBHS_PIPETRN_IX_NONE,
},
};

static  const  CPU_INT08U  USBH_HCD_AddrQty[] = {10u, 5u, 5u};  /* Quantity of avail addr for RZ, RX64M, and RX71M.     */


/*
*********************************************************************************************************
*                                            LOCAL MACROS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

                                                                /* --------------- DFIFO SEL & CTR REG ---------------- */
typedef  struct  usbh_renesas_usbhs_dxfifo {
    CPU_REG16  SEL;
    CPU_REG16  CTR;
} USBH_RENESAS_USBHS_DxFIFO;

                                                                /* ----------- PIPE TRANSACTION COUNTER REG ----------- */
typedef  struct  usbh_renesas_usbhs_transaction_ctr {
    CPU_REG16  TRE;
    CPU_REG16  TRN;
} USBH_RENESAS_USBHS_TRANSACTION_CTR;

                                                                /* ---------------- RENESAS USBHS REG ----------------- */
typedef  struct  usbh_renesas_usbhs_reg {
    CPU_REG16                           SYSCFG0;
    CPU_REG16                           BUSWAIT;
    CPU_REG16                           SYSSTS0;
    CPU_REG16                           RSVD_00;

    CPU_REG16                           DVSTCTR0;
    CPU_REG16                           RSVD_01;

    CPU_REG16                           TESTMODE;
    CPU_REG16                           RSVD_02;

    CPU_REG16                           DxFBCFG[RENESAS_USBHS_DFIFO_QTY];

    CPU_REG32                           CFIFO;
    CPU_REG32                           DxFIFO[RENESAS_USBHS_DFIFO_QTY];

    CPU_REG16                           CFIFOSEL;
    CPU_REG16                           CFIFOCTR;
    CPU_REG32                           RSVD_03;

    USBH_RENESAS_USBHS_DxFIFO           DxFIFOn[RENESAS_USBHS_DFIFO_QTY];

    CPU_REG16                           INTENB0;
    CPU_REG16                           INTENB1;
    CPU_REG16                           RSVD_04;

    CPU_REG16                           BRDYENB;
    CPU_REG16                           NRDYENB;
    CPU_REG16                           BEMPENB;
    CPU_REG16                           SOFCFG;
    CPU_REG16                           RSVD_05;

    CPU_REG16                           INTSTS0;
    CPU_REG16                           INTSTS1;
    CPU_REG16                           RSVD_06;

    CPU_REG16                           BRDYSTS;
    CPU_REG16                           NRDYSTS;
    CPU_REG16                           BEMPSTS;

    CPU_REG16                           FRMNUM;
    CPU_REG16                           UFRMNUM;

    CPU_REG16                           USBADDR;
    CPU_REG16                           RSVD_07;

    CPU_REG16                           USBREQ;
    CPU_REG16                           USBVAL;
    CPU_REG16                           USBINDX;
    CPU_REG16                           USBLENG;

    CPU_REG16                           DCPCFG;
    CPU_REG16                           DCPMAXP;
    CPU_REG16                           DCPCTR;
    CPU_REG16                           RSVD_08;

    CPU_REG16                           PIPESEL;
    CPU_REG16                           RSVD_09;
    CPU_REG16                           PIPECFG;
    CPU_REG16                           PIPEBUF;
    CPU_REG16                           PIPEMAXP;
    CPU_REG16                           PIPEPERI;

    CPU_REG16                           PIPExCTR[15u];
    CPU_REG16                           RSVD_10;

    USBH_RENESAS_USBHS_TRANSACTION_CTR  PIPExTR[12u];
    CPU_REG16                           RSVD_11[8u];

    CPU_REG16                           DEVADDx[11u];
    CPU_REG16                           RSVD_12[14u];

    CPU_REG16                           SUSPMODE;
}  USBH_RENESAS_USBHS_REG;

                                                                /* ----------- RENESAS USBHS CTRLR VARIANCE ----------- */
typedef  enum  usbh_renesas_usbhs_ctrlr {
    USBH_RENESAS_USBHS_CTRLR_RZ = 0u,
    USBH_RENESAS_USBHS_CTRLR_RX64M,
    USBH_RENESAS_USBHS_CTRLR_RX71M
} USBH_RENESAS_USBHS_CTRLR;


/*
*********************************************************************************************************
*                                         PIPE INFO DATA TYPE
*********************************************************************************************************
*/

typedef  struct  usbh_hcd_pipe_info {
    CPU_INT08U                         PipebufStartIx;          /* Buf start ix in FIFO for this pipe.                  */

    CPU_INT16U                         MaxPktSize;              /* Max pkt size of EP associated to this pipe.          */
    CPU_INT16U                         TotBufLen;               /* Indicates the total len alloc for this pipe in FIFO. */
    CPU_INT16U                         MaxBufLen;

    CPU_BOOLEAN                        UseDblBuf;               /* Indicates if pipe use double buffering.              */
    CPU_BOOLEAN                        UseContinMode;           /* Indicates if pipe use continuous mode (xfer based).  */

    CPU_INT16U                         LastRxLen;               /* Len of last received pkt.                            */
    CPU_INT08U                         FIFO_IxUsed;             /* Ix of the FIFO channel used with this pipe.          */

    USBH_URB                          *URB_Ptr;                 /* Pointer to current URB.                              */
    CPU_BOOLEAN                        IsFree;                  /* Indicates if pipe is free.                           */

    USBH_HCD_RENESAS_USBHS_PIPE_DESC  *PipeDescPtr;             /* Ptr to pipe desc struct.                             */
} USBH_HCD_PIPE_INFO;


/*
*********************************************************************************************************
*                                         DFIFO INFO DATA TYPE
*********************************************************************************************************
*/

typedef  struct  usbh_hcd_dfifo_info {
    CPU_INT08U    PipeNbr;                                      /* Log nbr of EP currently using this FIFO channel.     */
    CPU_BOOLEAN   XferIsRd;                                     /* Direction of current xfer.                           */
    CPU_INT08U   *BufPtr;                                       /* Ptr to buf currently processed by this fifo ch.      */
    CPU_INT32U    BufLen;                                       /* Len of buf currently processed by this fifo ch.      */

                                                                /* ---------- CNTS & FLAGS USED WITH TX XFER ---------- */
    CPU_INT32U    CopyDataCnt;                                  /* Cnt of data copied and ready to be transmitted.      */
    CPU_INT32U    DMA_CurTxLen;                                 /* Len of cur DMA xfer.                                 */
    CPU_INT08U    RemByteCnt;                                   /* Remaining byte cnt to copy to FIFO manually.         */

                                                                /* ---------- CNTS & FLAGS USED WITH RX XFER ---------- */
    CPU_INT08U    DMA_XferNewestIx;                             /* Index of newest xfer len added to list.              */
    CPU_INT08U    DMA_XferOldestIx;                             /* Index of oldest xfer len added to list.              */
    CPU_INT32U    DMA_XferTbl[RENESAS_USBHS_RX_Q_SIZE];         /* Table that contains list of xfer len for DMA.        */
    CPU_BOOLEAN   XferEndFlag;                                  /* Flag that indicates if xfer is complete.             */

                                                                /* --------- CNTS & FLAGS USED WITH ALL XFER ---------- */
    USBH_ERR      Err;                                          /* Xfer error code.                                     */
    CPU_INT32U    USB_XferByteCnt;                              /* Cur byte cnt of USB xfer.                            */
    CPU_INT32U    DMA_XferByteCnt;                              /* Cur byte cnt of DMA xfer.                            */
} USBH_HCD_DFIFO_INFO;


/*
*********************************************************************************************************
*                                   DRIVER INTERNAL DATA DATA TYPE
*********************************************************************************************************
*/

typedef  struct  usbh_hcd_data {
    CPU_BOOLEAN                DMA_En;                          /* Indicates if DMA should be used when possible.       */
    CPU_INT08U                 AvailDFIFO;                      /* Bitmap indicates available DFIFO ch. (See note 1)    */

    USBH_HCD_PIPE_INFO        *PipeInfoTblPtr;                  /* Ptr to table of pipe info.                           */
    CPU_INT08U                 PipeQty;                         /* Quantity of avail pipes.                             */

                                                                /* Array of DFIFO info.                                 */
    USBH_HCD_DFIFO_INFO        DFIFO_InfoTbl[RENESAS_USBHS_DFIFO_QTY];

    CPU_INT08U                 RH_Desc[sizeof(USBH_HUB_DESC)];  /* RH desc content.                                     */

    CPU_INT16U                 RH_PortStatus;                   /* PortStatus word for RH.                              */
    CPU_INT16U                 RH_PortStatusChng;               /* PortStatusChange word for RH.                        */

    USBH_RENESAS_USBHS_CTRLR   Ctrlr;
} USBH_HCD_DATA;


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
static  void           USBH_HCD_InitRZ_FIFO              (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_InitRZ_DMA               (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_InitRX64M_FIFO           (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_InitRX64M_DMA            (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_InitRX71M_FIFO           (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_InitRX71M_DMA            (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_Start                    (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_Stop                     (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  USBH_DEV_SPD   USBH_HCD_SpdGet                   (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_Suspend                  (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_Resume                   (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  CPU_INT32U     USBH_HCD_FrameNbrGet              (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_EP_Open                  (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_EP                 *p_ep,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_EP_Close                 (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_EP                 *p_ep,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_EP_Abort                 (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_EP                 *p_ep,
                                                          USBH_ERR                *p_err);

static  CPU_BOOLEAN    USBH_HCD_EP_IsHalt                (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_EP                 *p_ep,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_URB_SubmitFIFO           (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_URB                *p_urb,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_URB_SubmitDMA            (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_URB                *p_urb,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_URB_CompleteFIFO         (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_URB                *p_urb,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_URB_CompleteDMA          (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_URB                *p_urb,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_URB_Abort                (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_URB                *p_urb,
                                                          USBH_ERR                *p_err);

static  void           USBH_HCD_ISR_Handle               (void                    *p_data);


                                                                /* -------------- ROOT HUB API FUNCTIONS -------------- */
static  CPU_BOOLEAN    USBH_HCD_PortStatusGet            (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr,
                                                          USBH_HUB_PORT_STATUS    *p_port_status);

static  CPU_BOOLEAN    USBH_HCD_HubDescGet               (USBH_HC_DRV             *p_hc_drv,
                                                          void                    *p_buf,
                                                          CPU_INT08U               buf_len);

static  CPU_BOOLEAN    USBH_HCD_PortEnSet                (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortEnClr                (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortEnChngClr            (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortPwrSet               (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortPwrClr               (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortResetSet             (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortResetChngClr         (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortSuspendClr           (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_PortConnChngClr          (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               port_nbr);

static  CPU_BOOLEAN    USBH_HCD_RHSC_IntEn               (USBH_HC_DRV             *p_hc_drv);

static  CPU_BOOLEAN    USBH_HCD_RHSC_IntDis              (USBH_HC_DRV             *p_hc_drv);


                                                                /* ----------------- LOCAL FUNCTIONS ------------------ */
static  void           USBH_RenesasUSBHS_Init            (USBH_HC_DRV             *p_hc_drv,
                                                          USBH_ERR                *p_err);

static  CPU_BOOLEAN    USBH_RenesasUSBHS_CFIFO_Rd        (USBH_RENESAS_USBHS_REG  *p_reg,
                                                          CPU_INT08U              *p_buf,
                                                          CPU_INT32U               buf_len);

static  CPU_BOOLEAN    USBH_RenesasUSBHS_CFIFO_Wr        (USBH_RENESAS_USBHS_REG  *p_reg,
                                                          USBH_HCD_PIPE_INFO      *p_pipe_info,
                                                          CPU_INT08U               pipe_nbr,
                                                          CPU_INT08U              *p_buf,
                                                          CPU_INT32U               buf_len);

static  CPU_BOOLEAN    USBH_RenesasUSBHS_DFIFO_Rd        (USBH_RENESAS_USBHS_REG  *p_reg,
                                                          USBH_HCD_PIPE_INFO      *p_pipe_info,
                                                          USBH_HCD_DFIFO_INFO     *p_dfifo_info,
                                                          CPU_INT08U               dev_nbr,
                                                          CPU_INT08U               dfifo_nbr,
                                                          CPU_INT08U               pipe_nbr);

static  CPU_BOOLEAN    USBH_RenesasUSBHS_DFIFO_Wr        (USBH_RENESAS_USBHS_REG  *p_reg,
                                                          USBH_HCD_PIPE_INFO      *p_pipe_info,
                                                          USBH_HCD_DFIFO_INFO     *p_dfifo_info,
                                                          CPU_INT08U               dev_nbr,
                                                          CPU_INT08U               dfifo_nbr,
                                                          CPU_INT08U               pipe_nbr);

static  void           USBH_RenesasUSBHS_DFIFO_RemBytesWr(USBH_RENESAS_USBHS_REG  *p_reg,
                                                          CPU_INT08U              *p_buf,
                                                          CPU_INT08U               rem_bytes_cnt,
                                                          CPU_INT08U               dfifo_ch_nbr);

static  void           USBH_RenesasUSBHS_DFIFO_RemBytesRd(USBH_RENESAS_USBHS_REG  *p_reg,
                                                          CPU_INT08U              *p_buf,
                                                          CPU_INT08U               rem_bytes_cnt,
                                                          CPU_INT08U               dfifo_ch_nbr);

static  CPU_INT08U     USBH_RenesasUSBHS_FIFO_Acquire    (USBH_HCD_DATA           *p_hcd_data,
                                                          CPU_INT08U               pipe_nbr);

static  CPU_BOOLEAN    USBH_RenesasUSBHS_PipePID_Set     (USBH_RENESAS_USBHS_REG  *p_reg,
                                                          CPU_INT08U               pipe_nbr,
                                                          CPU_INT08U               resp_pid);

static  CPU_BOOLEAN    USBH_RenesasUSBHS_CurPipeSet      (CPU_REG16               *p_fifosel_reg,
                                                          CPU_INT08U               pipe_nbr,
                                                          CPU_BOOLEAN              is_wr);

static  void           USBH_RenesasUSBHS_BRDY_Event      (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               pipe_nbr);

static  void           USBH_RenesasUSBHS_BEMP_Event      (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               pipe_nbr);

static  void           USBH_RenesasUSBHS_DFIFO_Event     (USBH_HC_DRV             *p_hc_drv,
                                                          CPU_INT08U               dfifo_nbr);


/*
*********************************************************************************************************
*                                  EXTERNAL FUNCTIONS DEFINED IN BSP
*********************************************************************************************************
*/

void                               USBH_BSP_DlyUs          (CPU_INT32U    us);

USBH_DEV_SPD                       USBH_BSP_SpdGet         (void);

CPU_BOOLEAN                        USBH_BSP_DMA_CopyStart  (CPU_INT08U    hc_nbr,
                                                            CPU_INT08U    dfifo_nbr,
                                                            CPU_BOOLEAN   is_rd,
                                                            void         *buf_ptr,
                                                            CPU_REG32    *dfifo_addr,
                                                            CPU_INT32U    xfer_len);

CPU_INT08U                         USBH_BSP_DMA_ChStatusGet(CPU_INT08U    dev_nbr,
                                                            CPU_INT08U    dfifo_nbr);

void                               USBH_BSP_DMA_ChStatusClr(CPU_INT08U    dev_nbr,
                                                            CPU_INT08U    dfifo_nbr);

USBH_HCD_RENESAS_USBHS_PIPE_DESC  *USBH_BSP_PipeDescTblGet (CPU_INT08U    hc_nbr);


/*
*********************************************************************************************************
*                                 INITIALIZED GLOBAL VARIABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                            USB HOST CONTROLLER DRIVER API FOR RENESAS RZ
*********************************************************************************************************
*/
                                                                /* --------------------- FIFO API --------------------- */
USBH_HC_DRV_API  USBH_HCD_API_RenesasRZ_FIFO = {
    USBH_HCD_InitRZ_FIFO,
    USBH_HCD_Start,
    USBH_HCD_Stop,
    USBH_HCD_SpdGet,
    USBH_HCD_Suspend,
    USBH_HCD_Resume,
    USBH_HCD_FrameNbrGet,

    USBH_HCD_EP_Open,
    USBH_HCD_EP_Close,
    USBH_HCD_EP_Abort,
    USBH_HCD_EP_IsHalt,

    USBH_HCD_URB_SubmitFIFO,
    USBH_HCD_URB_CompleteFIFO,
    USBH_HCD_URB_Abort,
};

                                                                /* --------------------- DMA API ---------------------- */
USBH_HC_DRV_API  USBH_HCD_API_RenesasRZ_DMA = {
    USBH_HCD_InitRZ_DMA,
    USBH_HCD_Start,
    USBH_HCD_Stop,
    USBH_HCD_SpdGet,
    USBH_HCD_Suspend,
    USBH_HCD_Resume,
    USBH_HCD_FrameNbrGet,

    USBH_HCD_EP_Open,
    USBH_HCD_EP_Close,
    USBH_HCD_EP_Abort,
    USBH_HCD_EP_IsHalt,

    USBH_HCD_URB_SubmitDMA,
    USBH_HCD_URB_CompleteDMA,
    USBH_HCD_URB_Abort,
};


/*
*********************************************************************************************************
*                          USB HOST CONTROLLER DRIVER API FOR RENESAS RX64M
*********************************************************************************************************
*/
                                                                /* --------------------- FIFO API --------------------- */
USBH_HC_DRV_API  USBH_HCD_API_RenesasRX64M_FIFO = {
    USBH_HCD_InitRX64M_FIFO,
    USBH_HCD_Start,
    USBH_HCD_Stop,
    USBH_HCD_SpdGet,
    USBH_HCD_Suspend,
    USBH_HCD_Resume,
    USBH_HCD_FrameNbrGet,

    USBH_HCD_EP_Open,
    USBH_HCD_EP_Close,
    USBH_HCD_EP_Abort,
    USBH_HCD_EP_IsHalt,

    USBH_HCD_URB_SubmitFIFO,
    USBH_HCD_URB_CompleteFIFO,
    USBH_HCD_URB_Abort,
};

                                                                /* --------------------- DMA API ---------------------- */
USBH_HC_DRV_API  USBH_HCD_API_RenesasRX64M_DMA = {
    USBH_HCD_InitRX64M_DMA,
    USBH_HCD_Start,
    USBH_HCD_Stop,
    USBH_HCD_SpdGet,
    USBH_HCD_Suspend,
    USBH_HCD_Resume,
    USBH_HCD_FrameNbrGet,

    USBH_HCD_EP_Open,
    USBH_HCD_EP_Close,
    USBH_HCD_EP_Abort,
    USBH_HCD_EP_IsHalt,

    USBH_HCD_URB_SubmitDMA,
    USBH_HCD_URB_CompleteDMA,
    USBH_HCD_URB_Abort,
};

                                                                /* ------------------- ROOT HUB API ------------------- */
USBH_HC_RH_API  USBH_HCD_RH_API_RenesasUSBHS = {
    USBH_HCD_PortStatusGet,
    USBH_HCD_HubDescGet,

    USBH_HCD_PortEnSet,
    USBH_HCD_PortEnClr,
    USBH_HCD_PortEnChngClr,

    USBH_HCD_PortPwrSet,
    USBH_HCD_PortPwrClr,

    USBH_HCD_PortResetSet,
    USBH_HCD_PortResetChngClr,

    USBH_HCD_PortSuspendClr,
    USBH_HCD_PortConnChngClr,

    USBH_HCD_RHSC_IntEn,
    USBH_HCD_RHSC_IntDis
};


/*
*********************************************************************************************************
*                          USB HOST CONTROLLER DRIVER API FOR RENESAS RX71M
*********************************************************************************************************
*/
                                                                /* --------------------- FIFO API --------------------- */
USBH_HC_DRV_API  USBH_HCD_API_RenesasRX71M_FIFO = {
    USBH_HCD_InitRX71M_FIFO,
    USBH_HCD_Start,
    USBH_HCD_Stop,
    USBH_HCD_SpdGet,
    USBH_HCD_Suspend,
    USBH_HCD_Resume,
    USBH_HCD_FrameNbrGet,

    USBH_HCD_EP_Open,
    USBH_HCD_EP_Close,
    USBH_HCD_EP_Abort,
    USBH_HCD_EP_IsHalt,

    USBH_HCD_URB_SubmitFIFO,
    USBH_HCD_URB_CompleteFIFO,
    USBH_HCD_URB_Abort,
};

                                                                /* --------------------- DMA API ---------------------- */
USBH_HC_DRV_API  USBH_HCD_API_RenesasRX71M_DMA = {
    USBH_HCD_InitRX71M_DMA,
    USBH_HCD_Start,
    USBH_HCD_Stop,
    USBH_HCD_SpdGet,
    USBH_HCD_Suspend,
    USBH_HCD_Resume,
    USBH_HCD_FrameNbrGet,

    USBH_HCD_EP_Open,
    USBH_HCD_EP_Close,
    USBH_HCD_EP_Abort,
    USBH_HCD_EP_IsHalt,

    USBH_HCD_URB_SubmitDMA,
    USBH_HCD_URB_CompleteDMA,
    USBH_HCD_URB_Abort,
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
*                                       USBH_HCD_InitRZ_FIFO()
*
* Description : Initializes host controller and allocates driver's internal memory/variables.
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

static  void  USBH_HCD_InitRZ_FIFO (USBH_HC_DRV  *p_hc_drv,
                                    USBH_ERR     *p_err)
{
    USBH_HCD_DATA  *p_hcd_data;


    USBH_RenesasUSBHS_Init(p_hc_drv, p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_hcd_data         = (USBH_HCD_DATA *)p_hc_drv->DataPtr;
    p_hcd_data->DMA_En =  DEF_DISABLED;
    p_hcd_data->Ctrlr  =  USBH_RENESAS_USBHS_CTRLR_RZ;          /* Indicates that driver used on RZ.                    */
}


/*
*********************************************************************************************************
*                                         USBH_HCD_InitRZ_DMA()
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

static  void  USBH_HCD_InitRZ_DMA (USBH_HC_DRV  *p_hc_drv,
                                   USBH_ERR     *p_err)
{
    USBH_HCD_DATA  *p_hcd_data;


    USBH_RenesasUSBHS_Init(p_hc_drv, p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_hcd_data         = (USBH_HCD_DATA *)p_hc_drv->DataPtr;
    p_hcd_data->DMA_En =  DEF_ENABLED;
    p_hcd_data->Ctrlr  =  USBH_RENESAS_USBHS_CTRLR_RZ;          /* Indicates that driver used on RZ.                    */
}


/*
*********************************************************************************************************
*                                       USBH_HCD_InitRX64M_FIFO()
*
* Description : Initializes host controller and allocates driver's internal memory/variables.
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

static  void  USBH_HCD_InitRX64M_FIFO (USBH_HC_DRV  *p_hc_drv,
                                       USBH_ERR     *p_err)
{
    USBH_HCD_DATA  *p_hcd_data;


    USBH_RenesasUSBHS_Init(p_hc_drv, p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_hcd_data         = (USBH_HCD_DATA *)p_hc_drv->DataPtr;
    p_hcd_data->DMA_En =  DEF_DISABLED;
    p_hcd_data->Ctrlr  =  USBH_RENESAS_USBHS_CTRLR_RX64M;       /* Indicates that driver used on RX64M.                 */
}


/*
*********************************************************************************************************
*                                       USBH_HCD_InitRX64M_DMA()
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

static  void  USBH_HCD_InitRX64M_DMA (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    USBH_HCD_DATA  *p_hcd_data;


    USBH_RenesasUSBHS_Init(p_hc_drv, p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_hcd_data         = (USBH_HCD_DATA *)p_hc_drv->DataPtr;
    p_hcd_data->DMA_En =  DEF_ENABLED;
    p_hcd_data->Ctrlr  =  USBH_RENESAS_USBHS_CTRLR_RX64M;       /* Indicates that driver used on RX64M.                 */
}


/*
*********************************************************************************************************
*                                       USBH_HCD_InitRX71M_FIFO()
*
* Description : Initializes host controller and allocates driver's internal memory/variables.
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

static  void  USBH_HCD_InitRX71M_FIFO (USBH_HC_DRV  *p_hc_drv,
                                       USBH_ERR     *p_err)
{
    USBH_HCD_DATA  *p_hcd_data;


    USBH_RenesasUSBHS_Init(p_hc_drv, p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_hcd_data         = (USBH_HCD_DATA *)p_hc_drv->DataPtr;
    p_hcd_data->DMA_En =  DEF_DISABLED;
    p_hcd_data->Ctrlr  =  USBH_RENESAS_USBHS_CTRLR_RX71M;       /* Indicates that driver used on RX71M.                 */
}


/*
*********************************************************************************************************
*                                       USBH_HCD_InitRX71M_DMA()
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

static  void  USBH_HCD_InitRX71M_DMA (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    USBH_HCD_DATA  *p_hcd_data;


    USBH_RenesasUSBHS_Init(p_hc_drv, p_err);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_hcd_data         = (USBH_HCD_DATA *)p_hc_drv->DataPtr;
    p_hcd_data->DMA_En =  DEF_ENABLED;
    p_hcd_data->Ctrlr  =  USBH_RENESAS_USBHS_CTRLR_RX71M;       /* Indicates that driver used on RX71M.                 */
}


/*
*********************************************************************************************************
*                                          USBH_HCD_Start()
*
* Description : Starts Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD start successful.
*
* Return(s)   : None.
*
* Note(s)     : (1) RZ and RX71M USBHS host controller supports low/full/high-speed devices.
*                   RX64M USBHS host controller supports only low/full-speed devices.
*
*               (2) The bit SUSPM in register SUSPMODE must be set to 1 in order to write into registers
*                   associated to default control pipe.
*********************************************************************************************************
*/

static  void  USBH_HCD_Start (USBH_HC_DRV  *p_hc_drv,
                              USBH_ERR     *p_err)
{
    CPU_INT08U               cnt;
    USBH_HC_BSP_API         *p_bsp_api;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;
    USBH_HCD_PIPE_INFO      *p_pipe_info;


    p_hcd_data = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg      = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_bsp_api  =  p_hc_drv->BSP_API_Ptr;

    p_hcd_data->RH_PortStatus     = DEF_BIT_NONE;
    p_hcd_data->RH_PortStatusChng = DEF_BIT_NONE;
    p_hcd_data->AvailDFIFO        = RENESAS_USBHS_DFIFO_MASK;

    for (cnt = 0u; cnt < p_hcd_data->PipeQty; cnt++) {
        p_pipe_info              = &p_hcd_data->PipeInfoTblPtr[cnt];
        p_pipe_info->FIFO_IxUsed =  RENESAS_USBHS_FIFO_NONE;
        p_pipe_info->IsFree      =  DEF_YES;
    }

    if (p_hcd_data->Ctrlr == USBH_RENESAS_USBHS_CTRLR_RZ) {
                                                                /* Suspend UTMI.                                        */
        DEF_BIT_CLR(p_reg->SUSPMODE, RENESAS_USBHS_SUSPMODE_SUSPM);

        p_reg->SYSCFG0 = RENESAS_USBHS_SYSCFG0_UPLLE;           /* Enable PLL.                                          */

        USBH_OS_DlyMS(1u);                                      /* Apply short delay to suspend UTMI.                   */

        p_reg->SUSPMODE = RENESAS_USBHS_SUSPMODE_SUSPM;         /* Resume UTMI.                                         */

        USBH_OS_DlyMS(50u);                                     /* Apply short delay so UTMI can resume.                */
    }

    DEF_BIT_SET(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_DCFM);    /* Force USB host mode.                                 */

                                                                /* Enable High-Speed or Low/Full-Speed depending on ... */
                                                                /* ... controller. See Note #1.                         */
    switch (p_hcd_data->Ctrlr) {
        case USBH_RENESAS_USBHS_CTRLR_RZ:
        case USBH_RENESAS_USBHS_CTRLR_RX71M:
             DEF_BIT_SET(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_HSE);
             break;


        case USBH_RENESAS_USBHS_CTRLR_RX64M:
             DEF_BIT_CLR(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_HSE);
             break;


        default:
            *p_err = USBH_ERR_NOT_SUPPORTED;                    /* Return with error if controller is not supported.    */
             return;
    }
                                                                /* Reset dflt ctrl pipe (see Note #2).                  */
    p_reg->DCPCFG  = DEF_BIT_NONE;
    p_reg->DCPCTR  = RENESAS_USBHS_DCPCTR_PINGE;
    p_reg->DCPMAXP = 0u;

    if ((p_bsp_api           != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->ISR_Reg  !=                    0)) {

        p_bsp_api->ISR_Reg(USBH_HCD_ISR_Handle, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    p_reg->INTENB0 = RENESAS_USBHS_INTENB0_BEMPE |
                     RENESAS_USBHS_INTENB0_BRDYE |
                     RENESAS_USBHS_INTENB0_NRDYE;

    p_reg->INTENB1 = RENESAS_USBHS_INTENB1_ATTCHE |
                     RENESAS_USBHS_INTENB1_BCHGE;

    DEF_BIT_SET(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_DRPD);    /* Enable dev conn detection.                           */

    if (p_hcd_data->Ctrlr != USBH_RENESAS_USBHS_CTRLR_RZ) {
        DEF_BIT_SET(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_CNEN);/* Enable single end receiver operation.                */
    }

    DEF_BIT_SET(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_USBE);    /* Enable USB controller.                               */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           USBH_HCD_Stop()
*
* Description : Stops Host Controller.
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

static  void  USBH_HCD_Stop (USBH_HC_DRV  *p_hc_drv,
                             USBH_ERR     *p_err)
{
    USBH_HC_BSP_API         *p_bsp_api;
    USBH_RENESAS_USBHS_REG  *p_reg;


    p_reg      = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_bsp_api  =  p_hc_drv->BSP_API_Ptr;

    DEF_BIT_CLR(p_reg->SUSPMODE,                                /* Suspend UTMI.                                        */
                RENESAS_USBHS_SUSPMODE_SUSPM);

    p_reg->SYSCFG0 = DEF_BIT_NONE;

    p_reg->INTENB0 = RENESAS_USBHS_INTENB0_BEMPE |
                     RENESAS_USBHS_INTENB0_BRDYE |
                     RENESAS_USBHS_INTENB0_NRDYE;

    p_reg->INTENB1 = RENESAS_USBHS_INTENB1_ATTCHE |
                     RENESAS_USBHS_INTENB1_BCHGE;

    if ((p_bsp_api            != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->ISR_Unreg !=  DEF_NULL)) {
        p_bsp_api->ISR_Unreg(p_err);
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          USBH_HCD_SpdGet()
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

static  USBH_DEV_SPD  USBH_HCD_SpdGet (USBH_HC_DRV  *p_hc_drv,
                                       USBH_ERR     *p_err)
{
    USBH_DEV_SPD  hc_speed;


    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;

    hc_speed = USBH_BSP_SpdGet();

    return (hc_speed);
}


/*
*********************************************************************************************************
*                                         USBH_HCD_Suspend()
*
* Description : Suspends Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD suspend successful.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HCD_Suspend (USBH_HC_DRV  *p_hc_drv,
                                USBH_ERR     *p_err)
{
    USBH_RENESAS_USBHS_REG  *p_reg;


    p_reg = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_CLR(p_reg->DVSTCTR0, RENESAS_USBHS_DVSCTR_UACT);    /* Stop generation of SOF.                              */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          USBH_HCD_Resume()
*
* Description : Resumes Host Controller.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HCD resume successful.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HCD_Resume (USBH_HC_DRV  *p_hc_drv,
                               USBH_ERR     *p_err)
{
    CPU_INT16U               dvstctr0;
    USBH_RENESAS_USBHS_REG  *p_reg;


    p_reg  = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_SET(p_reg->DVSTCTR0, RENESAS_USBHS_DVSCTR_RESUME);  /* Issue resume signal to dev.                          */

    USBH_OS_DlyMS(20u);

    dvstctr0 = p_reg->DVSTCTR0;
    DEF_BIT_SET(dvstctr0, RENESAS_USBHS_DVSCTR_UACT);           /* En generation of SOF.                                */
    DEF_BIT_CLR(dvstctr0, RENESAS_USBHS_DVSCTR_RESUME);         /* Clr USB resume signal.                               */
    p_reg->DVSTCTR0 = dvstctr0;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_HCD_FrameNbrGet()
*
* Description : Retrieves current frame number.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           HC frame number retrieved successfuly.
*
* Return(s)   : Frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_HCD_FrameNbrGet (USBH_HC_DRV  *p_hc_drv,
                                          USBH_ERR     *p_err)
{
    USBH_RENESAS_USBHS_REG  *p_reg;


    p_reg = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

   *p_err = USBH_ERR_NONE;

    return (p_reg->FRMNUM);
}


/*
*********************************************************************************************************
*                                         USBH_HCD_EP_Open()
*
* Description : Allocates/opens endpoint of given type.
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

static  void  USBH_HCD_EP_Open (USBH_HC_DRV  *p_hc_drv,
                                USBH_EP      *p_ep,
                                USBH_ERR     *p_err)
{
    CPU_INT08U               pipe_type;
    CPU_INT08U               pipe_ix;
    CPU_INT08U               pipe_nbr;
    CPU_INT08U               ep_dir;
    CPU_INT08U               ep_log_nbr;
    CPU_INT08U               ep_buf_qty;
    CPU_INT16U               devadd_val;
    CPU_INT16U               pipecfg_val;
    CPU_INT16U               pipebuf_val;
    CPU_INT16U               single_buf_len;
    CPU_INT16U               ep_max_pkt_size;
    CPU_INT16U               rounded_up_max_pkt_size;
    CPU_INT16U               ep_interval;
    USBH_DEV                *p_dev;
    USBH_EP_TYPE             ep_type;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;
    USBH_HCD_PIPE_INFO      *p_pipe_info;


    ep_type         =  USBH_EP_TypeGet(p_ep);
    ep_dir          =  USBH_EP_DirGet(p_ep);
    ep_log_nbr      =  USBH_EP_LogNbrGet(p_ep);
    ep_max_pkt_size =  USBH_EP_MaxPktSizeGet(p_ep);
    p_dev           =  p_ep->DevPtr;
    p_reg           = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_hcd_data      = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;

    if (ep_type == USBH_EP_TYPE_CTRL) {                         /* Configure dev.                                       */

        if (p_ep->DevAddr <= USBH_HCD_AddrQty[p_hcd_data->Ctrlr]) {
            switch (p_dev->DevSpd) {
                case USBH_DEV_SPD_LOW:
                     devadd_val = RENESAS_USBHS_DEVADDx_USBSPD_LOW;
                     break;


                case USBH_DEV_SPD_FULL:
                     devadd_val = RENESAS_USBHS_DEVADDx_USBSPD_FULL;
                     break;


                case USBH_DEV_SPD_HIGH:
                     devadd_val = RENESAS_USBHS_DEVADDx_USBSPD_HIGH;
                     break;


                default:
                    *p_err = USBH_ERR_DEV_ALLOC;
                     return;
            }

            if (p_dev->HubDevPtr->IsRootHub == DEF_NO) {
                devadd_val |= ((p_dev->PortNbr            <<  8u) & RENESAS_USBHS_DEVADDx_HUBPORT_MASK);
                devadd_val |= ((p_dev->HubDevPtr->DevAddr << 11u) & RENESAS_USBHS_DEVADDx_UPPHUB_MASK);
            }

            p_reg->DEVADDx[p_ep->DevAddr] =  devadd_val;
            p_ep->ArgPtr                  = (void *)&p_hcd_data->PipeInfoTblPtr[0u];
        } else {
           *p_err = USBH_ERR_DEV_ALLOC;
            return;
        }
    } else {
        switch (ep_type) {
            case USBH_EP_TYPE_BULK:
                 pipe_type   = USBH_HCD_PIPE_DESC_TYPE_BULK;
                 pipecfg_val = RENESAS_USBHS_PIPCFG_TYPE_BULK;
                 break;


            case USBH_EP_TYPE_INTR:
                 pipe_type   = USBH_HCD_PIPE_DESC_TYPE_INTR;
                 pipecfg_val = RENESAS_USBHS_PIPCFG_TYPE_INTR;
                 break;


            case USBH_EP_TYPE_ISOC:
                 pipe_type   = USBH_HCD_PIPE_DESC_TYPE_ISOC;
                 pipecfg_val = RENESAS_USBHS_PIPCFG_TYPE_ISOC;
                 break;


            default:
                 break;
        }

        p_pipe_info = (USBH_HCD_PIPE_INFO *)0;                  /* Alloc a pipe.                                        */
        pipe_ix     =  1u;
        while (pipe_ix < p_hcd_data->PipeQty) {
            if ((p_hcd_data->PipeInfoTblPtr[pipe_ix].IsFree                                         == DEF_YES) &&
                (DEF_BIT_IS_SET(p_hcd_data->PipeInfoTblPtr[pipe_ix].PipeDescPtr->Attrib, pipe_type) == DEF_YES)) {
                p_pipe_info         = &p_hcd_data->PipeInfoTblPtr[pipe_ix];
                p_pipe_info->IsFree =  DEF_NO;

                break;
            }

            pipe_ix++;
        }

        if (p_pipe_info == (USBH_HCD_PIPE_INFO *)0) {
           *p_err = USBH_ERR_EP_ALLOC;
            return;
        }

        pipe_nbr = p_pipe_info->PipeDescPtr->Nbr;

        p_pipe_info->UseDblBuf     = DEF_NO;
        p_pipe_info->UseContinMode = DEF_NO;
        p_pipe_info->MaxBufLen     = p_pipe_info->TotBufLen;
        p_pipe_info->FIFO_IxUsed   = RENESAS_USBHS_FIFO_NONE;

        if (ep_type != USBH_EP_TYPE_INTR) {
                                                                /* Round up max pkt size on buf size base.              */
            rounded_up_max_pkt_size = (((ep_max_pkt_size - 1u) & (~(RENESAS_USBHS_BUF_LEN - 1u))) + RENESAS_USBHS_BUF_LEN);

                                                                /* Determine configuration for this pipe.               */
            single_buf_len = p_pipe_info->TotBufLen;
            if ((single_buf_len / 2u) >= rounded_up_max_pkt_size) {
                                                                /* Use double buffering.                                */
                single_buf_len         /= 2u;
                p_pipe_info->UseDblBuf  = DEF_YES;
                p_pipe_info->MaxBufLen  = single_buf_len;

                if (((single_buf_len / 2u) >= rounded_up_max_pkt_size) &&
                     (ep_type              == USBH_EP_TYPE_BULK)) {
                    p_pipe_info->UseContinMode = DEF_YES;       /* Use continuous mode.                                 */
                }
            }

            if (p_hcd_data->PipeInfoTblPtr[pipe_nbr].PipebufStartIx == RENESAS_USBHS_BUF_IX_NONE) {
               *p_err = USBH_ERR_EP_ALLOC;
                return;
            }

            ep_buf_qty  = ( single_buf_len / RENESAS_USBHS_BUF_LEN) - 1u;
            pipebuf_val = ((ep_buf_qty << 10u) & RENESAS_USBHS_PIPEBUF_BUFSIZE_MASK) |
                          (p_hcd_data->PipeInfoTblPtr[pipe_nbr].PipebufStartIx & RENESAS_USBHS_PIPEBUF_BUFNMB_MASK);
        }

        p_pipe_info->MaxPktSize = ep_max_pkt_size;

        if (p_pipe_info->UseDblBuf == DEF_YES) {
            DEF_BIT_SET(pipecfg_val, RENESAS_USBHS_PIPCFG_DBLB);
        }

        if (p_pipe_info->UseContinMode == DEF_YES) {
            DEF_BIT_SET(pipecfg_val, RENESAS_USBHS_PIPCFG_CNTMD);
        }

        if (ep_dir == USBH_EP_DIR_IN) {
            DEF_BIT_SET(pipecfg_val, RENESAS_USBHS_PIPCFG_SHTNAK);
        } else {
            DEF_BIT_SET(pipecfg_val, RENESAS_USBHS_PIPCFG_DIR);
        }

        pipecfg_val |= (ep_log_nbr & RENESAS_USBHS_PIPCFG_EPNUM_MASK);
        ep_interval  =  DEF_MIN(p_ep->Interval, DEF_BIT(RENESAS_USBHS_PIPERI_IITV_MASK));

        p_reg->PIPESEL  = (pipe_nbr & RENESAS_USBHS_PIPESEL_PIPESEL_MASK);
        p_reg->PIPECFG  =  pipecfg_val;
        p_reg->PIPEBUF  =  pipebuf_val;
        p_reg->PIPEPERI = ((8u - CPU_CntLeadZeros08((CPU_INT08U)ep_interval)) - 1u) & RENESAS_USBHS_PIPERI_IITV_MASK;
        p_reg->PIPEMAXP = ((p_ep->DevAddr << 12u) & RENESAS_USBHS_PIPEMAXP_DEVSEL_MASK) |
                           (ep_max_pkt_size & RENESAS_USBHS_PIPEMAXP_MXPS_MASK);

        DEF_BIT_SET(p_reg->PIPExCTR[pipe_nbr -1u],
                    RENESAS_USBHS_PIPExCTR_SQCLR);

                                                                /* Clr FIFO.                                            */
        DEF_BIT_SET(p_reg->PIPExCTR[pipe_nbr - 1u],
                    RENESAS_USBHS_PIPExCTR_ACLRM);
        DEF_BIT_CLR(p_reg->PIPExCTR[pipe_nbr - 1u],
                    RENESAS_USBHS_PIPExCTR_ACLRM);

        p_ep->ArgPtr = (void *)p_pipe_info;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         USBH_HCD_EP_Close()
*
* Description : Closes specified endpoint.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           Endpoint closed successfully.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HCD_EP_Close (USBH_HC_DRV  *p_hc_drv,
                                 USBH_EP      *p_ep,
                                 USBH_ERR     *p_err)
{
    CPU_INT08U               pipe_nbr;
    USBH_RENESAS_USBHS_REG  *p_reg;
    USBH_HCD_PIPE_INFO      *p_pipe_info;


    p_reg       = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_pipe_info = (USBH_HCD_PIPE_INFO     *)p_ep->ArgPtr;
    if (p_pipe_info == (USBH_HCD_PIPE_INFO *)0) {
       *p_err = USBH_ERR_NONE;
        return;
    }

    pipe_nbr = p_pipe_info->PipeDescPtr->Nbr;

    if (pipe_nbr != 0u) {
        DEF_BIT_SET(p_reg->PIPExCTR[pipe_nbr - 1u],             /* Clr FIFO.                                            */
                    RENESAS_USBHS_PIPExCTR_ACLRM);
        DEF_BIT_CLR(p_reg->PIPExCTR[pipe_nbr - 1u],
                    RENESAS_USBHS_PIPExCTR_ACLRM);

        (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                            pipe_nbr,
                                            RENESAS_USBHS_PIPExCTR_PID_NAK);

        p_pipe_info->IsFree = DEF_YES;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         USBH_HCD_EP_Abort()
*
* Description : Aborts all pending URBs on endpoint.
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

static  void  USBH_HCD_EP_Abort (USBH_HC_DRV  *p_hc_drv,
                                 USBH_EP      *p_ep,
                                 USBH_ERR     *p_err)
{
    CPU_INT08U               pipe_nbr;
    USBH_RENESAS_USBHS_REG  *p_reg;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_HCD_PIPE_INFO      *p_pipe_info;


    p_reg       = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_pipe_info = (USBH_HCD_PIPE_INFO     *)p_ep->ArgPtr;
    p_hcd_data  = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    pipe_nbr    =  p_pipe_info->PipeDescPtr->Nbr;

    DEF_BIT_CLR(p_reg->BEMPENB, DEF_BIT(pipe_nbr));             /* Dis int.                                             */
    DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));
    DEF_BIT_CLR(p_reg->NRDYENB, DEF_BIT(pipe_nbr));

    (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,                  /* Dis pipe.                                            */
                                        pipe_nbr,
                                        RENESAS_USBHS_PIPExCTR_PID_NAK);

    if ((p_pipe_info->FIFO_IxUsed != RENESAS_USBHS_FIFO_NONE) &&
        (p_pipe_info->FIFO_IxUsed != RENESAS_USBHS_CFIFO)) {
        DEF_BIT_SET(p_hcd_data->AvailDFIFO,                     /* Free FIFO channel.                                   */
                    DEF_BIT(p_pipe_info->FIFO_IxUsed));
        p_pipe_info->FIFO_IxUsed = RENESAS_USBHS_FIFO_NONE;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBH_HCD_EP_IsHalt()
*
* Description : Retrieves endpoint halt state.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               p_ep        Pointer to endpoint structure.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*                   USBH_ERR_NONE           Endpoint halt status retrieved successfuly.
*
* Return(s)   : DEF_TRUE,       If endpoint halted.
*
*               DEF_FALSE,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_HCD_EP_IsHalt (USBH_HC_DRV  *p_hc_drv,
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
*                                      USBH_HCD_URB_SubmitFIFO()
*
* Description : Submits specified URB.
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

static  void  USBH_HCD_URB_SubmitFIFO (USBH_HC_DRV  *p_hc_drv,
                                       USBH_URB     *p_urb,
                                       USBH_ERR     *p_err)
{
    CPU_BOOLEAN              valid;
    CPU_INT08U               ep_dir;
    CPU_INT08U               pipe_nbr;
    CPU_INT08U              *p_buf;
    CPU_INT16U               ep_max_pkt_size;
    CPU_INT16U               tx_len;
    USBH_EP                 *p_ep;
    USBH_EP_TYPE             ep_type;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;
    USBH_HCD_PIPE_INFO      *p_pipe_info;
    CPU_SR_ALLOC();


    p_hcd_data      = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg           = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_ep            =  p_urb->EP_Ptr;
    ep_type         =  USBH_EP_TypeGet(p_ep);
    ep_max_pkt_size =  USBH_EP_MaxPktSizeGet(p_ep);
    ep_dir          =  USBH_EP_DirGet(p_ep);
    p_buf           = (CPU_INT08U *)p_urb->UserBufPtr;

    switch (ep_type) {
        case USBH_EP_TYPE_CTRL:
             p_pipe_info              = &p_hcd_data->PipeInfoTblPtr[0u];
             p_pipe_info->URB_Ptr     =  p_urb;
             p_pipe_info->MaxPktSize  =  ep_max_pkt_size;
             p_pipe_info->MaxBufLen   =  RENESAS_USBHS_BUF_LEN;
             p_pipe_info->FIFO_IxUsed =  RENESAS_USBHS_CFIFO;

             switch (p_urb->Token) {
                 case USBH_TOKEN_SETUP:
                                                                /* Cfg ctrl pipe for this EP.                           */
                      p_reg->DCPMAXP =  (ep_max_pkt_size & RENESAS_USBHS_DCPMAXP_MXPS_MASK) |
                                       ((p_ep->DevAddr << 12u) & RENESAS_USBHS_DCPMAXP_DEVSEL_MASK);

                                                                /* Prepare setup pkt.                                   */
                      p_reg->USBREQ  = p_buf[0u] | (p_buf[1u] << DEF_OCTET_NBR_BITS);
                      p_reg->USBVAL  = p_buf[2u] | (p_buf[3u] << DEF_OCTET_NBR_BITS);
                      p_reg->USBINDX = p_buf[4u] | (p_buf[5u] << DEF_OCTET_NBR_BITS);
                      p_reg->USBLENG = p_buf[6u] | (p_buf[7u] << DEF_OCTET_NBR_BITS);

                      DEF_BIT_SET(p_reg->INTENB1, RENESAS_USBHS_INTENB1_SACKE | RENESAS_USBHS_INTENB1_SIGNE);
                      p_reg->DCPCTR = RENESAS_USBHS_DCPCTR_PINGE | RENESAS_USBHS_DCPCTR_SUREQ;
                      break;


                 case USBH_TOKEN_IN:
                      DEF_BIT_CLR(p_reg->DCPCFG,  RENESAS_USBHS_PIPCFG_DIR);
                      DEF_BIT_SET(p_reg->DCPCTR,  RENESAS_USBHS_DCPCTR_SQSET);
                      DEF_BIT_SET(p_reg->NRDYENB, DEF_BIT_00);
                      DEF_BIT_SET(p_reg->BRDYENB, DEF_BIT_00);

                      valid = USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                                            0u,
                                                            RENESAS_USBHS_PIPExCTR_PID_BUF);
                      if (valid != DEF_OK) {
                         *p_err = USBH_ERR_HC_IO;
                          return;
                      }
                      break;


                 case USBH_TOKEN_OUT:
                      tx_len         = DEF_MIN(p_urb->UserBufLen, p_pipe_info->MaxBufLen);
                      p_urb->XferLen = tx_len;

                      DEF_BIT_SET(p_reg->DCPCFG,  RENESAS_USBHS_PIPCFG_DIR);
                      DEF_BIT_SET(p_reg->DCPCTR,  RENESAS_USBHS_DCPCTR_SQSET);
                      DEF_BIT_SET(p_reg->NRDYENB, DEF_BIT_00);
                      DEF_BIT_SET(p_reg->BEMPENB, DEF_BIT_00);

                      CPU_CRITICAL_ENTER();
                      valid = USBH_RenesasUSBHS_CFIFO_Wr(p_reg,
                                                         p_pipe_info,
                                                         0u,
                                                         p_urb->UserBufPtr,
                                                         tx_len);
                      CPU_CRITICAL_EXIT();
                      if (valid != DEF_OK) {
                         *p_err = USBH_ERR_HC_IO;
                          return;
                      }
                      break;


                 default:
                      break;
             }
             break;


        case USBH_EP_TYPE_BULK:
        case USBH_EP_TYPE_INTR:
             p_pipe_info              = (USBH_HCD_PIPE_INFO *)p_ep->ArgPtr;
             pipe_nbr                 =  p_pipe_info->PipeDescPtr->Nbr;
             p_pipe_info->URB_Ptr     =  p_urb;
             p_pipe_info->FIFO_IxUsed =  RENESAS_USBHS_CFIFO;

             DEF_BIT_SET(p_reg->NRDYENB, DEF_BIT(pipe_nbr));

             if (ep_dir == USBH_EP_DIR_IN) {
                 DEF_BIT_SET(p_reg->BRDYENB, DEF_BIT(pipe_nbr));

                 valid = USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                                       pipe_nbr,
                                                       RENESAS_USBHS_PIPExCTR_PID_BUF);
                 if (valid != DEF_OK) {
                    *p_err = USBH_ERR_HC_IO;
                     return;
                 }
             } else {
                 tx_len         = DEF_MIN(p_urb->UserBufLen, p_pipe_info->MaxBufLen);
                 p_urb->XferLen+= tx_len;

                 DEF_BIT_SET(p_reg->BEMPENB, DEF_BIT(pipe_nbr));

                 CPU_CRITICAL_ENTER();
                 valid = USBH_RenesasUSBHS_CFIFO_Wr(              p_reg,
                                                                  p_pipe_info,
                                                                  pipe_nbr,
                                                    (CPU_INT08U *)p_urb->UserBufPtr,
                                                                  tx_len);
                 CPU_CRITICAL_EXIT();
                 if (valid != DEF_OK) {
                    *p_err = USBH_ERR_HC_IO;
                     return;
                 }
             }
             break;


        case USBH_EP_TYPE_ISOC:
        default:
             break;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_HCD_URB_SubmitDMA()
*
* Description : Submit specified URB using DMA controller if possible.
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
* Note(s)     : (1) Only bulk transfers use DMA transfer.
*********************************************************************************************************
*/

static  void  USBH_HCD_URB_SubmitDMA (USBH_HC_DRV  *p_hc_drv,
                                      USBH_URB     *p_urb,
                                      USBH_ERR     *p_err)
{
    CPU_BOOLEAN              valid;
    CPU_INT08U               ep_dir;
    CPU_INT08U               fifo_ix;
    CPU_INT08U               pipe_nbr;
    CPU_INT08U              *p_buf;
    USBH_EP                 *p_ep;
    USBH_EP_TYPE             ep_type;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;
    USBH_HCD_PIPE_INFO      *p_pipe_info;
    USBH_HCD_DFIFO_INFO     *p_dfifo_info;


    p_hcd_data = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg      = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_ep       =  p_urb->EP_Ptr;
    ep_type    =  USBH_EP_TypeGet(p_ep);
    ep_dir     =  USBH_EP_DirGet(p_ep);
    p_buf      = (CPU_INT08U *)p_urb->UserBufPtr;

    switch(ep_type) {
        case USBH_EP_TYPE_CTRL:
        case USBH_EP_TYPE_INTR:
             USBH_HCD_URB_SubmitFIFO(p_hc_drv,
                                     p_urb,
                                     p_err);
             return;


        case USBH_EP_TYPE_BULK:
             p_pipe_info = (USBH_HCD_PIPE_INFO *)p_ep->ArgPtr;
             pipe_nbr    =  p_pipe_info->PipeDescPtr->Nbr;
             fifo_ix     =  USBH_RenesasUSBHS_FIFO_Acquire(p_hcd_data,
                                                           pipe_nbr);
             if (fifo_ix == RENESAS_USBHS_CFIFO) {
                 USBH_HCD_URB_SubmitFIFO(p_hc_drv,
                                         p_urb,
                                         p_err);
                 return;
             }

             p_pipe_info->FIFO_IxUsed = fifo_ix;
             p_pipe_info->URB_Ptr     = p_urb;

                                                                /* Init DFIFO xfer flags.                               */
             p_dfifo_info                   = &p_hcd_data->DFIFO_InfoTbl[fifo_ix];
             p_dfifo_info->BufPtr           =  p_buf;
             p_dfifo_info->BufLen           =  p_urb->UserBufLen;
             p_dfifo_info->USB_XferByteCnt  =  0u;
             p_dfifo_info->DMA_XferByteCnt  =  0u;
             p_dfifo_info->PipeNbr          =  pipe_nbr;
             p_dfifo_info->Err              =  USBH_ERR_NONE;
             p_dfifo_info->DMA_XferNewestIx =  0u;
             p_dfifo_info->DMA_XferOldestIx =  0u;
             p_dfifo_info->XferEndFlag      =  DEF_NO;
             p_dfifo_info->XferIsRd         = (ep_dir == USBH_EP_DIR_IN) ? DEF_YES : DEF_NO;
             p_dfifo_info->CopyDataCnt      =  0u;
             p_dfifo_info->RemByteCnt       =  0u;

                                                                /* Clr FIFO.                                            */
             DEF_BIT_SET(p_reg->PIPExCTR[pipe_nbr - 1u],
                         RENESAS_USBHS_PIPExCTR_ACLRM);
             DEF_BIT_CLR(p_reg->PIPExCTR[pipe_nbr - 1u],
                         RENESAS_USBHS_PIPExCTR_ACLRM);

                                                                /* Set current pipe to allocated DFIFO channel.         */
             valid = USBH_RenesasUSBHS_CurPipeSet(&p_reg->DxFIFOn[p_pipe_info->FIFO_IxUsed].SEL,
                                                   pipe_nbr,
                                                   DEF_NO);
             if (valid != DEF_OK) {
                 DEF_BIT_SET(p_hcd_data->AvailDFIFO,            /* Free FIFO channel.                                   */
                             DEF_BIT(p_pipe_info->FIFO_IxUsed));

                *p_err = USBH_ERR_HC_IO;
                 return;
             }

             DEF_BIT_SET(p_reg->NRDYENB, DEF_BIT(pipe_nbr));

             if (ep_dir == USBH_EP_DIR_IN) {
                 if (USBH_HCD_PIPETRN_LUT[p_hcd_data->Ctrlr][pipe_nbr] != RENESAS_USBHS_PIPETRN_IX_NONE) {
                     p_reg->PIPExTR[USBH_HCD_PIPETRN_LUT[p_hcd_data->Ctrlr][pipe_nbr]].TRN = ((p_urb->UserBufLen - 1u) / p_pipe_info->MaxPktSize) + 1u;
                     p_reg->PIPExTR[USBH_HCD_PIPETRN_LUT[p_hcd_data->Ctrlr][pipe_nbr]].TRE =  RENESAS_USBHS_PIPExTRE_TRENB;
                 }

                 DEF_BIT_SET(p_reg->BRDYENB, DEF_BIT(pipe_nbr));

                 valid = USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                                       pipe_nbr,
                                                       RENESAS_USBHS_PIPExCTR_PID_BUF);
                 if (valid != DEF_OK) {
                     DEF_BIT_SET(p_hcd_data->AvailDFIFO,        /* Free FIFO channel.                                   */
                                 DEF_BIT(p_pipe_info->FIFO_IxUsed));

                    *p_err = USBH_ERR_HC_IO;
                     return;
                 }
             } else {
                                                                /* Enable int.                                          */
                 DEF_BIT_CLR(p_reg->BRDYSTS, DEF_BIT(pipe_nbr));
                 DEF_BIT_SET(p_reg->BEMPENB, DEF_BIT(pipe_nbr));
                 DEF_BIT_SET(p_reg->BRDYENB, DEF_BIT(pipe_nbr));

                                                                /* ---------------- WRITE DATA TO FIFO ---------------- */
                 valid = USBH_RenesasUSBHS_DFIFO_Wr(p_reg,
                                                    p_pipe_info,
                                                    p_dfifo_info,
                                                    p_hc_drv->Nbr,
                                                    p_pipe_info->FIFO_IxUsed,
                                                    pipe_nbr);
                 if (valid != DEF_OK) {
                     DEF_BIT_SET(p_hcd_data->AvailDFIFO,        /* Free FIFO channel.                                   */
                                 DEF_BIT(p_pipe_info->FIFO_IxUsed));

                    *p_err = USBH_ERR_HC_IO;
                     return;
                 }
             }
             break;


        case USBH_EP_TYPE_ISOC:
        default:
            *p_err = USBH_ERR_NOT_SUPPORTED;
             return;
             break;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_HCD_URB_CompleteFIFO()
*
* Description : Completes specified URB in FIFO mode.
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

static  void  USBH_HCD_URB_CompleteFIFO (USBH_HC_DRV  *p_hc_drv,
                                         USBH_URB     *p_urb,
                                         USBH_ERR     *p_err)
{
    CPU_BOOLEAN              valid;
    CPU_INT32U               xfer_len;
    USBH_EP                 *p_ep;
    USBH_RENESAS_USBHS_REG  *p_reg;
    USBH_HCD_PIPE_INFO      *p_pipe_info;
    CPU_SR_ALLOC();


    p_reg = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_ep  =  p_urb->EP_Ptr;

    if (p_urb->Token == USBH_TOKEN_IN) {
        p_pipe_info = (USBH_HCD_PIPE_INFO *)p_ep->ArgPtr;
        xfer_len    =  p_pipe_info->LastRxLen;

        CPU_CRITICAL_ENTER();
        valid = USBH_RenesasUSBHS_CurPipeSet(&p_reg->CFIFOSEL,
                                              p_pipe_info->PipeDescPtr->Nbr,
                                              DEF_NO);

        if (valid == DEF_OK) {
            valid = USBH_RenesasUSBHS_CFIFO_Rd(p_reg,
                                              &((CPU_INT08U *)p_urb->UserBufPtr)[p_urb->XferLen],
                                               xfer_len);
        }
        CPU_CRITICAL_EXIT();
        if (valid == DEF_OK) {
           *p_err = USBH_ERR_NONE;
        } else {
           *p_err = USBH_ERR_HC_IO;
        }

        p_urb->XferLen += xfer_len;
    } else {
       *p_err = USBH_ERR_NONE;
    }
}


/*
*********************************************************************************************************
*                                     USBH_HCD_URB_CompleteDMA()
*
* Description : Completes specified URB in DMA mode.
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

static  void  USBH_HCD_URB_CompleteDMA (USBH_HC_DRV  *p_hc_drv,
                                        USBH_URB     *p_urb,
                                        USBH_ERR     *p_err)
{
    USBH_EP              *p_ep;
    USBH_HCD_DATA        *p_hcd_data;
    USBH_HCD_PIPE_INFO   *p_pipe_info;
    USBH_HCD_DFIFO_INFO  *p_dfifo_info;


    p_ep        =  p_urb->EP_Ptr;
    p_hcd_data  = (USBH_HCD_DATA      *)p_hc_drv->DataPtr;
    p_pipe_info = (USBH_HCD_PIPE_INFO *)p_ep->ArgPtr;

    if (p_pipe_info->FIFO_IxUsed == RENESAS_USBHS_CFIFO) {
        USBH_HCD_URB_CompleteFIFO(p_hc_drv,
                                  p_urb,
                                  p_err);

        return;
    }

    p_dfifo_info   = &p_hcd_data->DFIFO_InfoTbl[p_pipe_info->FIFO_IxUsed];
    p_urb->XferLen =  p_dfifo_info->USB_XferByteCnt;
    p_urb->Err     =  p_dfifo_info->Err;

    DEF_BIT_SET(p_hcd_data->AvailDFIFO,                     /* Free FIFO channel.                                   */
                DEF_BIT(p_pipe_info->FIFO_IxUsed));

    p_pipe_info->FIFO_IxUsed = RENESAS_USBHS_FIFO_NONE;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBH_HCD_URB_Abort()
*
* Description : Aborts specified URB.
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

static  void  USBH_HCD_URB_Abort (USBH_HC_DRV  *p_hc_drv,
                                  USBH_URB     *p_urb,
                                  USBH_ERR     *p_err)
{
    CPU_INT08U               pipe_nbr;
    USBH_EP                 *p_ep;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_HCD_PIPE_INFO      *p_pipe_info;
    USBH_RENESAS_USBHS_REG  *p_reg;


    p_ep        =  p_urb->EP_Ptr;
    p_pipe_info = (USBH_HCD_PIPE_INFO     *)p_ep->ArgPtr;
    p_hcd_data  = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg       = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    pipe_nbr    =  p_pipe_info->PipeDescPtr->Nbr;

    (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,                  /* Disable pipe.                                        */
                                        pipe_nbr,
                                        RENESAS_USBHS_PIPExCTR_PID_NAK);

    DEF_BIT_CLR(p_reg->BEMPENB, DEF_BIT(pipe_nbr));             /* Disable int.                                         */
    DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));
    DEF_BIT_CLR(p_reg->NRDYENB, DEF_BIT(pipe_nbr));

    if ((p_pipe_info->FIFO_IxUsed != RENESAS_USBHS_FIFO_NONE) &&
        (p_pipe_info->FIFO_IxUsed != RENESAS_USBHS_CFIFO)) {

        DEF_BIT_SET(p_hcd_data->AvailDFIFO,                     /* Free FIFO channel.                                   */
                    DEF_BIT(p_pipe_info->FIFO_IxUsed));

        p_pipe_info->FIFO_IxUsed = RENESAS_USBHS_FIFO_NONE;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBH_HCD_ISR_Handle()
*
* Description : ISR handler.
*
* Argument(s) : p_data      Pointer to host controller driver structure.
*
* Return(s)   : None.
*
* Note(s)     : (1) When the BCHG interrupt is generated, the LNST bits are read several times to remove
*                   the chattering effect until the same value is read repeatedly from the bits.
*
*               (2) Two different methods are used to confirm the device connection status depending of
*                   the host controller speed support:
*
*                   (a) For high-speed controller, a reset is performed on the device from the ISR
*                       in order to confirm the connection status by then reading the bits field
*                       'USB Bus Reset Status' in the register DVSTCTR0.
*
*                   (b) For low/full-speed controller, reading the bits field 'USB Data Line Status
*                       Monitor' in the register SYSSTS0 allows to confirm the device connection status.
*
*               (3) USBHS host controller documentation indicates that the bit HSE in register SYSCFG
*                   must be set to 0 when a low-speed device is connected. Consequently, when a full- or
*                   high-speed device is connected after a low-speed device, the bit HSE must be set back
*                   to 1 for enabling high-speed operation. Nevertheless, if the USBHS is used in LS/FS
*                   mode only, the bit HSE must always remain set to 0.
*********************************************************************************************************
*/

static  void  USBH_HCD_ISR_Handle (void  *p_data)
{
              CPU_INT08U               pipe_nbr;
              CPU_INT08U               dfifo_cnt;
              CPU_INT08U               dfifo_status;
              CPU_INT16U               intsts0_reg;
              CPU_INT16U               intsts1_reg;
              CPU_INT16U               brdysts_reg;
              CPU_INT16U               bempsts_reg;
              CPU_INT16U               nrdysts_reg;
              USBH_DEV_SPD             hc_speed;
              CPU_INT16U               dev_conn_status;
              CPU_INT16U               rhst;
              CPU_INT16U               lnsts1;
              CPU_INT16U               lnsts2;
              CPU_INT16U               lnsts3;
    volatile  CPU_INT16U               dummy;
              USBH_URB                *p_urb;
              USBH_HC_DRV             *p_hc_drv;
              USBH_HCD_DATA           *p_hcd_data;
              USBH_RENESAS_USBHS_REG  *p_reg;
              USBH_HCD_PIPE_INFO      *p_pipe_info;
              USBH_HCD_DFIFO_INFO     *p_dfifo_info;


    p_hc_drv   = (USBH_HC_DRV            *)p_data;
    p_hcd_data = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg      = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    intsts0_reg    =  p_reg->INTSTS0;
    intsts0_reg   &=  p_reg->INTENB0;

    intsts1_reg    =  p_reg->INTSTS1;
    intsts1_reg   &=  p_reg->INTENB1;

    bempsts_reg    =  p_reg->BEMPSTS;
    p_reg->BEMPSTS = ~bempsts_reg;

    brdysts_reg    =  p_reg->BRDYSTS;
    p_reg->BRDYSTS = ~brdysts_reg;

    nrdysts_reg    =  p_reg->NRDYSTS;
    p_reg->NRDYSTS = ~nrdysts_reg;


                                                                /* -------------------- BUS CHANGE -------------------- */
    if (DEF_BIT_IS_SET(intsts1_reg, RENESAS_USBHS_INTSTS1_BCHG) == DEF_YES) {
        p_reg->INTSTS1 = (CPU_INT16U)~(RENESAS_USBHS_INTSTS1_BCHG);

        do {                                                    /* Ensure line state is stable. See note (1).           */
            lnsts1 = p_reg->SYSSTS0 & RENESAS_USBHS_SYSSTS0_LNST_MASK;
            USBH_BSP_DlyUs(50u * 1000u);                        /* Wait before next line state rd.                      */
            lnsts2 = p_reg->SYSSTS0 & RENESAS_USBHS_SYSSTS0_LNST_MASK;
            USBH_BSP_DlyUs(50u * 1000u);                        /* Wait before next line state rd.                      */
            lnsts3 = p_reg->SYSSTS0 & RENESAS_USBHS_SYSSTS0_LNST_MASK;
        } while ((lnsts1 != lnsts2) ||
                 (lnsts2 != lnsts3));
    }

                                                                /* ---------------------- ATTACH ---------------------- */
    if (DEF_BIT_IS_SET(intsts1_reg, RENESAS_USBHS_INTSTS1_ATTCH) == DEF_YES) {
        USBH_ERR  err;


                                                                /* See Note #2.                                         */
        hc_speed = USBH_HCD_SpdGet(p_hc_drv, &err);
        if (hc_speed == USBH_DEV_SPD_HIGH) {
            DEF_BIT_SET(p_reg->DVSTCTR0,                        /* Must reset device before detecting its presence.     */
                        RENESAS_USBHS_DVSCTR_USBRST);

            USBH_BSP_DlyUs(10u * 1000u);

            DEF_BIT_CLR(p_reg->DVSTCTR0,
                        RENESAS_USBHS_DVSCTR_USBRST);

            dev_conn_status = p_reg->DVSTCTR0 & RENESAS_USBHS_DVSCTR_RHST_MASK;
        } else {
            dev_conn_status = p_reg->SYSSTS0 & RENESAS_USBHS_SYSSTS0_LNST_MASK;
        }
                                                                /* Ensure dev is connected.                             */
        if (dev_conn_status != RENESAS_USBHS_DEV_CONN_STATUS_NONE) {

            DEF_BIT_CLR(p_reg->INTENB1, RENESAS_USBHS_INTENB1_ATTCHE);
            DEF_BIT_CLR(p_reg->INTENB1, RENESAS_USBHS_INTENB1_BCHGE);
            DEF_BIT_SET(p_reg->INTENB1, RENESAS_USBHS_INTENB1_DTCHE);

                                                                /* Update port status and port status chng.             */
            DEF_BIT_SET(p_hcd_data->RH_PortStatusChng, USBH_HUB_STATUS_C_PORT_CONN);
            DEF_BIT_SET(p_hcd_data->RH_PortStatus,     USBH_HUB_STATUS_PORT_CONN);

            rhst = p_reg->DVSTCTR0 & RENESAS_USBHS_DVSCTR_RHST_MASK;
            if (rhst == RENESAS_USBHS_DVSCTR_RHST_LS) {
                DEF_BIT_CLR(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_HSE);
            } else if (p_hcd_data->Ctrlr != USBH_RENESAS_USBHS_CTRLR_RX64M) {
                                                                /* See Note #3.                                         */
                DEF_BIT_SET(p_reg->SYSCFG0, RENESAS_USBHS_SYSCFG0_HSE);
            }

            USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);
        }

        p_reg->INTSTS1 = (CPU_INT16U)~(RENESAS_USBHS_INTSTS1_ATTCH);
    }

                                                                /* ---------------------- DETACH ---------------------- */
    if (DEF_BIT_IS_SET(intsts1_reg, RENESAS_USBHS_INTSTS1_DTCH) == DEF_YES) {
        USBH_ERR  err;


        hc_speed = USBH_HCD_SpdGet(p_hc_drv, &err);
        if (hc_speed == USBH_DEV_SPD_HIGH) {
            USBH_BSP_DlyUs(10u * 1000u);
            dev_conn_status = p_reg->DVSTCTR0 & RENESAS_USBHS_DVSCTR_RHST_MASK;
        } else {
            dev_conn_status = p_reg->SYSSTS0 & RENESAS_USBHS_SYSSTS0_LNST_MASK;
        }
                                                                /* Ensure no dev is connected.                          */
        if (dev_conn_status == RENESAS_USBHS_DEV_CONN_STATUS_NONE) {

            DEF_BIT_SET(p_reg->INTENB1, RENESAS_USBHS_INTENB1_ATTCHE);
            DEF_BIT_CLR(p_reg->INTENB1, RENESAS_USBHS_INTENB1_DTCHE);
            DEF_BIT_SET(p_reg->INTENB1, RENESAS_USBHS_INTENB1_BCHGE);

                                                                /* Update port status and port status chng.             */
            DEF_BIT_SET(p_hcd_data->RH_PortStatusChng, USBH_HUB_STATUS_C_PORT_CONN);
            DEF_BIT_CLR(p_hcd_data->RH_PortStatus,     USBH_HUB_STATUS_PORT_CONN);

            USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);
        }

        p_reg->INTSTS1 = (CPU_INT16U)~(RENESAS_USBHS_INTSTS1_DTCH);
    }


                                                                /* ----------------- END OF SETUP PKT ----------------- */
    if (DEF_BIT_IS_SET(intsts1_reg, RENESAS_USBHS_INTSTS1_SACK) == DEF_YES) {
        p_urb          = p_hcd_data->PipeInfoTblPtr[0u].URB_Ptr;
        p_urb->Err     = USBH_ERR_NONE;
        p_urb->XferLen = USBH_LEN_SETUP_PKT;

        USBH_URB_Done(p_urb);

        DEF_BIT_CLR(p_reg->INTENB1, RENESAS_USBHS_INTENB1_SACKE | RENESAS_USBHS_INTENB1_SIGNE);
        p_reg->INTSTS1 = (CPU_INT16U)~(RENESAS_USBHS_INTSTS1_SACK);
    }


                                                                /* -------------- END OF SETUP PKT ERROR -------------- */
    if (DEF_BIT_IS_SET(intsts1_reg, RENESAS_USBHS_INTSTS1_SIGN) == DEF_YES) {

        p_urb          = p_hcd_data->PipeInfoTblPtr[0u].URB_Ptr;
        p_urb->Err     = USBH_ERR_HC_IO;
        p_urb->XferLen = 0u;

        USBH_URB_Done(p_urb);

        DEF_BIT_CLR(p_reg->INTENB1, RENESAS_USBHS_INTENB1_SACKE | RENESAS_USBHS_INTENB1_SIGNE);
        p_reg->INTSTS1 = (CPU_INT16U)~(RENESAS_USBHS_INTSTS1_SIGN);
    }


                                                                /* --------------- DMA INTERRUPT STATUS --------------- */
    if (p_hcd_data->DMA_En == DEF_ENABLED) {
        for (dfifo_cnt = 0u; dfifo_cnt < RENESAS_USBHS_DFIFO_QTY; dfifo_cnt++) {
            dfifo_status = USBH_BSP_DMA_ChStatusGet(p_hc_drv->Nbr, dfifo_cnt);

                                                                /* DMA xfer completed.                                  */
            if (DEF_BIT_IS_SET(dfifo_status, DEF_BIT_00) == DEF_YES) {
                USBH_RenesasUSBHS_DFIFO_Event(p_hc_drv, dfifo_cnt);

                USBH_BSP_DMA_ChStatusClr(p_hc_drv->Nbr, dfifo_cnt);
            } else if (DEF_BIT_IS_SET(dfifo_status, DEF_BIT_01) == DEF_YES) {
                p_dfifo_info = &p_hcd_data->DFIFO_InfoTbl[dfifo_cnt];
                p_urb        =  p_hcd_data->PipeInfoTblPtr[p_dfifo_info->PipeNbr].URB_Ptr;

                p_urb->Err = USBH_ERR_HC_IO;
                USBH_URB_Done(p_urb);

                USBH_BSP_DMA_ChStatusClr(p_hc_drv->Nbr, dfifo_cnt);
            }
        }
    }


                                                                /* ------------------- BUFFER EMPTY ------------------- */
    bempsts_reg &= p_reg->BEMPENB;
    while (bempsts_reg != DEF_BIT_NONE) {
        pipe_nbr = CPU_CntTrailZeros(bempsts_reg);

        USBH_RenesasUSBHS_BEMP_Event(p_hc_drv, pipe_nbr);

        DEF_BIT_CLR(bempsts_reg, DEF_BIT(pipe_nbr));
    }


                                                                /* ------------------- BUFFER READY ------------------- */
    brdysts_reg &= p_reg->BRDYENB;
    while (brdysts_reg != DEF_BIT_NONE) {
        pipe_nbr = CPU_CntTrailZeros(brdysts_reg);

        USBH_RenesasUSBHS_BRDY_Event(p_hc_drv, pipe_nbr);

        DEF_BIT_CLR(brdysts_reg, DEF_BIT(pipe_nbr));
    }


                                                                /* ----------------- BUFFER NOT READY ----------------- */
    nrdysts_reg &= p_reg->NRDYENB;
    while (nrdysts_reg != DEF_BIT_NONE) {
        pipe_nbr    =  CPU_CntTrailZeros(nrdysts_reg);
        p_pipe_info = &p_hcd_data->PipeInfoTblPtr[pipe_nbr];

                                                                /* End xfer and signal error to core.                   */
        p_pipe_info->URB_Ptr->Err = USBH_ERR_DEV_NOT_RESPONDING;
        USBH_URB_Done(p_pipe_info->URB_Ptr);

        (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                            pipe_nbr,
                                            RENESAS_USBHS_PIPExCTR_PID_NAK);

        DEF_BIT_CLR(nrdysts_reg, DEF_BIT(pipe_nbr));

        DEF_BIT_CLR(p_reg->NRDYENB, DEF_BIT(pipe_nbr));
        DEF_BIT_CLR(p_reg->BEMPENB, DEF_BIT(pipe_nbr));
        DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));
    }

                                                                /* Memory barrier.                                      */
    dummy = p_reg->INTSTS0;
    dummy = p_reg->INTSTS0;
    dummy = p_reg->INTSTS0;

    dummy = p_reg->INTSTS1;
    dummy = p_reg->INTSTS1;
    dummy = p_reg->INTSTS1;

    dummy = p_reg->BEMPSTS;
    dummy = p_reg->BEMPSTS;
    dummy = p_reg->BEMPSTS;

    dummy = p_reg->BRDYSTS;
    dummy = p_reg->BRDYSTS;
    dummy = p_reg->BRDYSTS;
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
*                                      USBH_HCD_PortStatusGet()
*
* Description : Retrieves port status changes and port status.
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

static  CPU_BOOLEAN  USBH_HCD_PortStatusGet (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr,
                                             USBH_HUB_PORT_STATUS  *p_port_status)
{
    CPU_INT16U               port_status;
    CPU_INT16U               device_state;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;


    (void)port_nbr;

    p_hcd_data = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg      = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    port_status  = p_hcd_data->RH_PortStatus;
    device_state = p_reg->DVSTCTR0;                             /* Read dev state.                                      */

    DEF_BIT_CLR(port_status, DEF_BIT_04);                       /* Clr reset status.                                    */

                                                                /* Determine conn spd.                                  */
    switch (device_state & RENESAS_USBHS_DVSCTR_RHST_MASK) {
        case RENESAS_USBHS_DVSCTR_RHST_LS:
             DEF_BIT_SET(port_status, DEF_BIT_09);              /* LS dev attached (PORT_LOW_SPEED)                     */
             DEF_BIT_CLR(port_status, DEF_BIT_10);              /* LS dev attached (PORT_HIGH_SPEED)                    */
             break;


        case RENESAS_USBHS_DVSCTR_RHST_FS:
             DEF_BIT_CLR(port_status, DEF_BIT_09);              /* FS dev attached (PORT_LOW_SPEED)                     */
             DEF_BIT_CLR(port_status, DEF_BIT_10);              /* FS dev attached (PORT_HIGH_SPEED)                    */
             break;


        case RENESAS_USBHS_DVSCTR_RHST_HS:
             DEF_BIT_CLR(port_status, DEF_BIT_09);              /* HS dev attached (PORT_LOW_SPEED)                     */
             DEF_BIT_SET(port_status, DEF_BIT_10);              /* HS dev attached (PORT_HIGH_SPEED)                    */
             break;


        case RENESAS_USBHS_DVSCTR_RHST_RESET:
             DEF_BIT_SET(port_status, DEF_BIT_04);              /* Reset.                                               */
             break;


        default:
             DEF_BIT_CLR(port_status, DEF_BIT_09);              /* FS dev attached (PORT_LOW_SPEED)                     */
             DEF_BIT_CLR(port_status, DEF_BIT_10);              /* FS dev attached (PORT_HIGH_SPEED)                    */
             break;
    }

    p_hcd_data->RH_PortStatus  = port_status;
    p_port_status->wPortStatus = MEM_VAL_GET_INT16U_LITTLE(&p_hcd_data->RH_PortStatus);
    p_port_status->wPortChange = MEM_VAL_GET_INT16U_LITTLE(&p_hcd_data->RH_PortStatusChng);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        USBH_HCD_HubDescGet()
*
* Description : Retrieves root hub descriptor.
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

static  CPU_BOOLEAN  USBH_HCD_HubDescGet (USBH_HC_DRV  *p_hc_drv,
                                          void         *p_buf,
                                          CPU_INT08U    buf_len)
{
    USBH_HUB_DESC    hub_desc;
    USBH_HCD_DATA  *p_hcd_data;


    p_hcd_data = (USBH_HCD_DATA *)p_hc_drv->DataPtr;

    hub_desc.bDescLength         = USBH_HUB_LEN_HUB_DESC;
    hub_desc.bDescriptorType     = USBH_HUB_DESC_TYPE_HUB;
    hub_desc.bNbrPorts           = 1u;                          /* Only on port on Renesas USBHS controller.            */
    hub_desc.wHubCharacteristics = 0u;
    hub_desc.bPwrOn2PwrGood      = 100u;
    hub_desc.bHubContrCurrent    = 0u;

    USBH_HUB_FmtHubDesc(&hub_desc,                              /* Write struct in USB fmt.                             */
                         p_hcd_data->RH_Desc);

    buf_len = DEF_MIN(buf_len, sizeof(USBH_HUB_DESC));

    Mem_Copy(p_buf,                                             /* Copy formatted struct into buf.                      */
             p_hcd_data->RH_Desc,
             buf_len);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        USBH_HCD_PortEnSet()
*
* Description : Enables given port.
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

static  CPU_BOOLEAN  USBH_HCD_PortEnSet (USBH_HC_DRV  *p_hc_drv,
                                         CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        USBH_HCD_PortEnClr()
*
* Description : Clears port enable status.
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

static  CPU_BOOLEAN  USBH_HCD_PortEnClr (USBH_HC_DRV  *p_hc_drv,
                                         CPU_INT08U    port_nbr)
{
    USBH_HCD_DATA  *p_hcd_data;


    (void)port_nbr;

    p_hcd_data = (USBH_HCD_DATA *)p_hc_drv->DataPtr;

    DEF_BIT_CLR(p_hcd_data->RH_PortStatus,
                USBH_HUB_STATUS_PORT_EN);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      USBH_HCD_PortEnChngClr()
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

static  CPU_BOOLEAN  USBH_HCD_PortEnChngClr (USBH_HC_DRV  *p_hc_drv,
                                             CPU_INT08U    port_nbr)
{
    USBH_HCD_DATA  *p_hcd_data;


    (void)port_nbr;

    p_hcd_data = (USBH_HCD_DATA *)p_hc_drv->DataPtr;

    DEF_BIT_CLR(p_hcd_data->RH_PortStatusChng,
                USBH_HUB_STATUS_PORT_EN);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        USBH_HCD_PortPwrSet()
*
* Description : Sets port power.
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

static  CPU_BOOLEAN  USBH_HCD_PortPwrSet (USBH_HC_DRV  *p_hc_drv,
                                          CPU_INT08U    port_nbr)
{
    USBH_HCD_DATA  *p_hcd_data;


    (void)port_nbr;

    p_hcd_data = (USBH_HCD_DATA *)p_hc_drv->DataPtr;

    DEF_BIT_SET(p_hcd_data->RH_PortStatus,
                USBH_HUB_STATUS_PORT_PWR);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        USBH_HCD_PortPwrClr()
*
* Description : Clears port power.
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

static  CPU_BOOLEAN  USBH_HCD_PortPwrClr (USBH_HC_DRV  *p_hc_drv,
                                          CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;
    (void)port_nbr;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       USBH_HCD_PortResetSet()
*
* Description : Resets given port.
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

static  CPU_BOOLEAN  USBH_HCD_PortResetSet (USBH_HC_DRV  *p_hc_drv,
                                            CPU_INT08U    port_nbr)
{
    CPU_INT16U               port_status;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;


    (void)port_nbr;

    p_hcd_data  = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg       = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    port_status =  p_hcd_data->RH_PortStatus;

                                                                /* Check if port is powered.                            */
    if (DEF_BIT_IS_CLR(port_status, USBH_HUB_STATUS_PORT_PWR) == DEF_YES) {
        DEF_BIT_SET(port_status,
                    USBH_HUB_STATUS_PORT_EN);
        DEF_BIT_SET(port_status,
                    USBH_HUB_STATUS_PORT_RESET);
        DEF_BIT_SET(p_hcd_data->RH_PortStatusChng,
                    USBH_HUB_STATUS_C_PORT_RESET);

        DEF_BIT_SET(p_reg->DVSTCTR0,                            /* Start reset signaling to dev.                        */
                    RENESAS_USBHS_DVSCTR_USBRST);
    } else {
        return (DEF_FAIL);                                      /* Port is not powered, ignore operation.               */
    }

    p_hcd_data->RH_PortStatus = port_status;


    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_HCD_PortResetChngClr()
*
* Description : Clears port reset status change.
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

static  CPU_BOOLEAN  USBH_HCD_PortResetChngClr (USBH_HC_DRV  *p_hc_drv,
                                                CPU_INT08U    port_nbr)
{
    CPU_INT16U               dvstctr0;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;


    (void)port_nbr;

    p_hcd_data = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg      = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_CLR(p_hcd_data->RH_PortStatusChng,
                USBH_HUB_STATUS_PORT_RESET);

    dvstctr0 = p_reg->DVSTCTR0;
    DEF_BIT_SET(dvstctr0, RENESAS_USBHS_DVSCTR_UACT);           /* En generation of SOF.                                */
    DEF_BIT_CLR(dvstctr0, RENESAS_USBHS_DVSCTR_USBRST);         /* Clr USB reset.                                       */
    p_reg->DVSTCTR0 = dvstctr0;

    USBH_OS_DlyMS(20u);                                         /* Give time for HS handshake.                          */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      USBH_HCD_PortSuspendClr()
*
* Description : Resumes given port if port is suspended.
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

static  CPU_BOOLEAN  USBH_HCD_PortSuspendClr (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    port_nbr)
{
    CPU_INT16U               dvstctr0;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_RENESAS_USBHS_REG  *p_reg;


    (void)port_nbr;

    p_hcd_data = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_reg      = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_SET(p_reg->DVSTCTR0, RENESAS_USBHS_DVSCTR_RESUME);  /* Issue resume signal to dev.                          */

    USBH_OS_DlyMS(20u);

    dvstctr0 = p_reg->DVSTCTR0;
    DEF_BIT_SET(dvstctr0, RENESAS_USBHS_DVSCTR_UACT);           /* En generation of SOF.                                */
    DEF_BIT_CLR(dvstctr0, RENESAS_USBHS_DVSCTR_RESUME);         /* Clr USB resume signal.                               */
    p_reg->DVSTCTR0 = dvstctr0;

    DEF_BIT_CLR(p_hcd_data->RH_PortStatus,
                USBH_HUB_STATUS_PORT_SUSPEND);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_HCD_PortConnChngClr()
*
* Description : Clears port connect status change.
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

static  CPU_BOOLEAN  USBH_HCD_PortConnChngClr (USBH_HC_DRV  *p_hc_drv,
                                               CPU_INT08U    port_nbr)
{
    USBH_HCD_DATA  *p_hcd_data;


    (void)port_nbr;

    p_hcd_data = (USBH_HCD_DATA *)p_hc_drv->DataPtr;

    DEF_BIT_CLR(p_hcd_data->RH_PortStatusChng,
                USBH_HUB_STATUS_PORT_CONN);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        USBH_HCD_RHSC_IntEn()
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

static  CPU_BOOLEAN  USBH_HCD_RHSC_IntEn (USBH_HC_DRV  *p_hc_drv)
{
    (void)p_hc_drv;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       USBH_HCD_RHSC_IntDis()
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

static  CPU_BOOLEAN  USBH_HCD_RHSC_IntDis (USBH_HC_DRV  *p_hc_drv)
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
*                                      USBH_RenesasUSBHS_Init()
*
* Description : Initializes host controller and allocate driver's internal memory/variables.
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

static  void  USBH_RenesasUSBHS_Init (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    CPU_INT08U                         pipe_qty;
    CPU_INT08U                         pipe_nbr;
    CPU_INT08U                         next_buf_ix;
    CPU_INT08U                         buf_cnt;
    USBH_HCD_RENESAS_USBHS_PIPE_DESC  *p_pipe_desc;
    USBH_HCD_PIPE_INFO                *p_pipe_info;
    USBH_HCD_DATA                     *p_data;
    USBH_HC_BSP_API                   *p_bsp_api;
    LIB_ERR                            err_lib;


    p_bsp_api = p_hc_drv->BSP_API_Ptr;

                                                                /* Alloc HCD data struct.                               */
    p_data = (USBH_HCD_DATA *)Mem_HeapAlloc(sizeof(USBH_HCD_DATA),
                                            sizeof(CPU_ALIGN),
                                            DEF_NULL,
                                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

    Mem_Clr(p_data, sizeof(USBH_HCD_DATA));

    p_hc_drv->DataPtr = (void *)p_data;

                                                                /* ----------------- INIT PIPE TABLE ------------------ */
    p_pipe_desc = USBH_BSP_PipeDescTblGet(p_hc_drv->Nbr);
    pipe_qty  = 0u;                                             /* Determine qty of pipe.                               */
    while (p_pipe_desc->Attrib != DEF_BIT_NONE) {
        pipe_qty++;
        p_pipe_desc++;
    }
    p_data->PipeQty = pipe_qty;

                                                                /* Alloc pipe tbl.                                      */
    p_data->PipeInfoTblPtr = (USBH_HCD_PIPE_INFO *)Mem_HeapAlloc(sizeof(USBH_HCD_PIPE_INFO) * (pipe_qty),
                                                                 sizeof(CPU_ALIGN),
                                                                 DEF_NULL,
                                                                &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

                                                                /* Determine pipe buf info.                             */
    next_buf_ix = RENESAS_USBHS_BUF_STARTING_IX;
    p_pipe_desc = USBH_BSP_PipeDescTblGet(p_hc_drv->Nbr);
    while (p_pipe_desc->Attrib != DEF_BIT_NONE) {
        pipe_nbr    =  p_pipe_desc->Nbr;
        p_pipe_info = &p_data->PipeInfoTblPtr[pipe_nbr];

        if ((DEF_BIT_IS_SET(p_pipe_desc->Attrib, USBH_HCD_PIPE_DESC_TYPE_ISOC) == DEF_YES) ||
            (DEF_BIT_IS_SET(p_pipe_desc->Attrib, USBH_HCD_PIPE_DESC_TYPE_BULK) == DEF_YES)) {

                                                                /* Determine nbr of buf needed for the pipe.            */
            buf_cnt = (p_pipe_desc->MaxPktSize / RENESAS_USBHS_BUF_LEN);

            if (((CPU_INT08U)(next_buf_ix + buf_cnt)) <= RENESAS_USBHS_BUF_QTY_AVAIL) {
                p_pipe_info->PipebufStartIx  = next_buf_ix;
                p_pipe_info->TotBufLen       = p_pipe_desc->MaxPktSize;
                next_buf_ix                 += buf_cnt;
            } else {
                p_pipe_info->PipebufStartIx  = RENESAS_USBHS_BUF_IX_NONE;
            }
        } else {
            p_pipe_info->TotBufLen = p_pipe_desc->MaxPktSize;   /* Ctrl and intr EP have fixed buf ix/size.             */
        }
        p_pipe_info->PipeDescPtr = p_pipe_desc;

        p_pipe_desc++;
    }

    if ((p_bsp_api       != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->Init !=                    0)) {

        p_bsp_api->Init(p_hc_drv, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                    USBH_RenesasUSBHS_CFIFO_Rd()
*
* Description : Reads data from CFIFO.
*
* Arguments   : p_reg       Pointer to registers structure.
*
*               p_buf       Pointer to buffer.
*
*               buf_len     Buffer length in octets.
*
* Return(s)   : DEF_OK,   if NO error(s),
*
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) This function requires p_buf to be aligned on a 32 bit boundary.
*
*               (2) This function must be called from a critical section.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RenesasUSBHS_CFIFO_Rd (USBH_RENESAS_USBHS_REG  *p_reg,
                                                 CPU_INT08U              *p_buf,
                                                 CPU_INT32U               buf_len)
{
    CPU_INT16U   fifo_ctr;
    CPU_REG16    reg_to;
    CPU_INT08U   nbr_byte;
    CPU_INT08U  *p_buf8;
    CPU_INT32U   cnt;
    CPU_INT32U   nbr_word;
    CPU_INT32U  *p_buf32;
    CPU_INT32U   last_word;


    reg_to = RENESAS_USBHS_REG_TO;
    do {
        fifo_ctr = p_reg->CFIFOCTR;                             /* Chk that FIFO is ready to be rd.                     */
        reg_to--;
    } while (((fifo_ctr & RENESAS_USBHS_FIFOCTR_FRDY) != RENESAS_USBHS_FIFOCTR_FRDY) &&
              (reg_to > 0u));

    if (reg_to == 0u) {
        return (DEF_FAIL);
    }

    nbr_word = buf_len / 4u;
    nbr_byte = buf_len % 4u;

    p_buf32 = (CPU_INT32U *)p_buf;
    for (cnt = 0u; cnt < nbr_word; cnt++) {
        p_buf32[cnt] = p_reg->CFIFO;
    }

    if (nbr_byte > 0u) {
        p_buf8 = (CPU_INT08U *)&p_buf32[cnt];

        last_word = p_reg->CFIFO;
        for (cnt = 0u; cnt < nbr_byte; cnt++) {
            p_buf8[cnt] = (last_word >> (8u * cnt)) & 0x000000FFu;
        }
    }

    DEF_BIT_SET(p_reg->CFIFOCTR, RENESAS_USBHS_FIFOCTR_BCLR);   /* Clr FIFO buffer.                                     */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_RenesasUSBHS_CFIFO_Wr()
*
* Description : Writes data to CFIFO.
*
* Arguments   : p_reg           Pointer to registers structure.
*
*               p_pipe_info     Pointer to pipe information table.
*
*               pipe_nbr        Pipe number.
*
*               p_buf           Pointer to buffer.
*
*               buf_len         Buffer length in octets.
*
* Return(s)   : DEF_OK,   if NO error(s),
*
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) This function requires p_buf to be aligned on a 32 bit boundary.
*
*               (2) This function must be called within a critical section.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RenesasUSBHS_CFIFO_Wr (USBH_RENESAS_USBHS_REG  *p_reg,
                                                 USBH_HCD_PIPE_INFO      *p_pipe_info,
                                                 CPU_INT08U               pipe_nbr,
                                                 CPU_INT08U              *p_buf,
                                                 CPU_INT32U               buf_len)
{
    CPU_INT16U    fifo_ctr;
    CPU_REG16     reg_to;
    CPU_BOOLEAN   valid;
    CPU_INT08U    cnt;
    CPU_INT32U    nbr_word;
    CPU_INT32U    nbr_byte;
    CPU_INT32U   *p_buf32;
    CPU_INT08U   *p_buf8;


    valid = USBH_RenesasUSBHS_CurPipeSet(&p_reg->CFIFOSEL,
                                          pipe_nbr,
                                          DEF_YES);
    if (valid != DEF_OK) {
        return (DEF_FAIL);
    }

    reg_to = RENESAS_USBHS_REG_TO;
    do {
        fifo_ctr = p_reg->CFIFOCTR;                             /* Chk that FIFO is ready to be rd.                     */
        reg_to--;
    } while (((fifo_ctr & RENESAS_USBHS_FIFOCTR_FRDY) != RENESAS_USBHS_FIFOCTR_FRDY) &&
              (reg_to > 0u));

    if (reg_to == 0u) {
        return (DEF_FAIL);
    }

    nbr_word = buf_len / 4u;
    nbr_byte = buf_len % 4u;

    p_buf32 = (CPU_INT32U *)p_buf;
    for (cnt = 0u; cnt < nbr_word; cnt++) {
        p_reg->CFIFO = p_buf32[cnt];
    }

    if (nbr_byte > 0u) {
        p_buf8 = (CPU_INT08U *)&p_buf32[cnt];

        DEF_BIT_CLR(p_reg->CFIFOSEL,                            /* Set FIFO access width to 8 bit.                      */
                    RENESAS_USBHS_CFIFOSEL_MBW_MASK);

        DEF_BIT_SET(p_reg->CFIFOSEL,
                    RENESAS_USBHS_CFIFOSEL_MBW_8);

        for (cnt = 0u; cnt < nbr_byte; cnt++) {
            p_reg->CFIFO = p_buf8[cnt];
        }
    }

    if (( buf_len == 0u) ||                                     /* Indicate wr is cmpl if not a multiple of buf len.    */
        (((buf_len % p_pipe_info->MaxBufLen)                          != 0u) &&
        ( DEF_BIT_IS_CLR(p_reg->CFIFOCTR, RENESAS_USBHS_FIFOCTR_BVAL) == DEF_YES))) {
        p_reg->CFIFOCTR = RENESAS_USBHS_FIFOCTR_BVAL;
    }

    valid = USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                          pipe_nbr,
                                          RENESAS_USBHS_PIPExCTR_PID_BUF);

    return (valid);
}


/*
*********************************************************************************************************
*                                    USBH_RenesasUSBHS_DFIFO_Rd()
*
* Description : Reads data from DFIFO.
*
* Arguments   : p_reg           Pointer to registers structure.
*
*               p_pipe_info     Pointer to pipe information structure.
*
*               p_dfifo_info    Pointer to DFIFO information structure.
*
*               dev_nbr         Device number.
*
*               dfifo_nbr       DFIFO channel number.
*
*               pipe_nbr        Pipe number.
*
* Return(s)   : DEF_OK,   if NO error(s),
*
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) This function requires p_buf to be aligned on a 32 bit boundary.
*
*               (2) DFIFO is read by default with 32-bit access. If the number of bytes to read is not
*                   multiple of 4 bytes, the remaining bytes (1, 2 or 3 bytes) will be read using a 8-bit
*                   access method in USBH_RenesasUSBHS_DFIFO_Event().
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RenesasUSBHS_DFIFO_Rd (USBH_RENESAS_USBHS_REG  *p_reg,
                                                 USBH_HCD_PIPE_INFO      *p_pipe_info,
                                                 USBH_HCD_DFIFO_INFO     *p_dfifo_info,
                                                 CPU_INT08U               dev_nbr,
                                                 CPU_INT08U               dfifo_nbr,
                                                 CPU_INT08U               pipe_nbr)
{
    CPU_BOOLEAN   valid;
    CPU_BOOLEAN   start_dma;
    CPU_BOOLEAN   xfer_end_flag;
    CPU_INT08U   *p_buf;
    CPU_INT16U    xfer_len;
    CPU_INT16U    rx_len;
    CPU_SR_ALLOC();


    valid         =  DEF_OK;
    xfer_end_flag =  DEF_NO;
                                                                /* Get nbr of bytes rxd by controller.                  */
    rx_len        =  p_reg->DxFIFOn[dfifo_nbr].CTR & RENESAS_USBHS_FIFOCTR_DTLN_MASK;
    p_buf         = &p_dfifo_info->BufPtr[p_dfifo_info->DMA_XferByteCnt];
                                                                /* Determine how many bytes can be read at once.        */
    if (rx_len <= (p_dfifo_info->BufLen - p_dfifo_info->USB_XferByteCnt)) {
        p_dfifo_info->RemByteCnt = rx_len % sizeof(CPU_INT32U); /* See Note #2.                                         */
        xfer_len                 = rx_len - p_dfifo_info->RemByteCnt;
    } else {
        xfer_len                 = (p_dfifo_info->BufLen - p_dfifo_info->USB_XferByteCnt);
        p_dfifo_info->RemByteCnt =  xfer_len % sizeof(CPU_INT32U);
        xfer_len                 =  xfer_len - p_dfifo_info->RemByteCnt;
        p_dfifo_info->Err        =  USBH_ERR_HC_IO;
    }

    p_dfifo_info->USB_XferByteCnt += (xfer_len + p_dfifo_info->RemByteCnt);

                                                                /* Compute end of xfer flag.                            */
    if ((p_dfifo_info->USB_XferByteCnt       >= p_dfifo_info->BufLen) ||
        (xfer_len                            == 0u)                   ||
        (xfer_len % p_pipe_info->MaxPktSize) != 0u) {
        xfer_end_flag = DEF_YES;
    }

    if (xfer_len > 0u) {
        CPU_CRITICAL_ENTER();                                   /* Chk if no dma xfer q-ed.                             */
        if (p_dfifo_info->DMA_XferNewestIx == p_dfifo_info->DMA_XferOldestIx) {
            start_dma = DEF_YES;
        } else {
            start_dma = DEF_NO;
        }

        p_dfifo_info->DMA_XferTbl[p_dfifo_info->DMA_XferNewestIx] = xfer_len;

        p_dfifo_info->DMA_XferNewestIx++;
        if (p_dfifo_info->DMA_XferNewestIx >= RENESAS_USBHS_RX_Q_SIZE) {
            p_dfifo_info->DMA_XferNewestIx = 0u;
        }

        p_dfifo_info->XferEndFlag = xfer_end_flag;
        CPU_CRITICAL_EXIT();

        if (start_dma == DEF_YES) {
            valid = USBH_BSP_DMA_CopyStart(dev_nbr,
                                           dfifo_nbr,
                                           DEF_YES,
                                           p_buf,
                                          &p_reg->DxFIFO[dfifo_nbr],
                                           xfer_len);
        }
                                                                /* ZLP received. Xfer has completed.                    */
    } else if (p_dfifo_info->DMA_XferNewestIx == p_dfifo_info->DMA_XferOldestIx) {
        DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));

        USBH_RenesasUSBHS_DFIFO_RemBytesRd(p_reg,
                                           p_buf,
                                           p_dfifo_info->RemByteCnt,
                                           dfifo_nbr);

        USBH_URB_Done(p_pipe_info->URB_Ptr);
    }

    return (valid);
}



/*
*********************************************************************************************************
*                                    USBH_RenesasUSBHS_DFIFO_Wr()
*
* Description : Writes data to DFIFO.
*
* Arguments   : p_reg           Pointer to registers structure.
*
*               p_pipe_info     Pointer to pipe information structure.
*
*               p_dfifo_info    Pointer to DFIFO information structure
*
*               dev_nbr         Device number.
*
*               dfifo_nbr       DFIFO channel number.
*
*               pipe_nbr        Pipe number.
*
* Return(s)   : DEF_OK,   if NO error(s),
*
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) This function requires p_buf to be aligned on a 32 bit boundary.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RenesasUSBHS_DFIFO_Wr (USBH_RENESAS_USBHS_REG  *p_reg,
                                                 USBH_HCD_PIPE_INFO      *p_pipe_info,
                                                 USBH_HCD_DFIFO_INFO     *p_dfifo_info,
                                                 CPU_INT08U               dev_nbr,
                                                 CPU_INT08U               dfifo_nbr,
                                                 CPU_INT08U               pipe_nbr)
{
    CPU_BOOLEAN   valid;
    CPU_INT08U   *p_buf;
    CPU_INT32U    dma_xfer_len;
    CPU_SR_ALLOC();


                                                                /* DMA engine must do 32 bit accesses.                  */
    dma_xfer_len              =  p_dfifo_info->BufLen - p_dfifo_info->DMA_XferByteCnt;
    dma_xfer_len              =  DEF_MIN(dma_xfer_len, p_pipe_info->MaxBufLen);
    p_dfifo_info->RemByteCnt  =  dma_xfer_len % sizeof(CPU_INT32U);
    dma_xfer_len             -=  p_dfifo_info->RemByteCnt;
    p_buf                     = &p_dfifo_info->BufPtr[p_dfifo_info->DMA_XferByteCnt];

    if (dma_xfer_len > 0u) {

        CPU_CRITICAL_ENTER();
        p_dfifo_info->DMA_XferByteCnt += dma_xfer_len;
        p_dfifo_info->DMA_CurTxLen     = dma_xfer_len;
        CPU_CRITICAL_EXIT();

        if ((p_dfifo_info->DMA_XferByteCnt + p_dfifo_info->RemByteCnt) >= p_dfifo_info->BufLen) {
            DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));     /* No notification of avail buf if last xfer.           */
        }

        CPU_MB();                                               /* Ensure USB operations executed before DMA.           */

        valid = USBH_BSP_DMA_CopyStart(dev_nbr,
                                       dfifo_nbr,
                                       DEF_NO,
                                       p_buf,
                                      &p_reg->DxFIFO[dfifo_nbr],
                                       dma_xfer_len);
    } else {
                                                                /* Process any remaining bytes in data xfer.            */
        USBH_RenesasUSBHS_DFIFO_RemBytesWr(              p_reg,
                                           (CPU_INT08U *)p_buf,
                                                         p_dfifo_info->RemByteCnt,
                                                         dfifo_nbr);

        CPU_CRITICAL_ENTER();
        p_dfifo_info->DMA_XferByteCnt += p_dfifo_info->RemByteCnt;
        p_dfifo_info->CopyDataCnt     += p_dfifo_info->RemByteCnt;
        CPU_CRITICAL_EXIT();

        DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));         /* No notification of avail buf if last xfer.           */

        DEF_BIT_SET(p_reg->DxFIFOn[dfifo_nbr].CTR,              /* Inidicates wr to FIFO is complete.                   */
                    RENESAS_USBHS_FIFOCTR_BVAL);

        valid = USBH_RenesasUSBHS_PipePID_Set(p_reg,            /* Set pipe as ready to send data.                      */
                                              pipe_nbr,
                                              RENESAS_USBHS_PIPExCTR_PID_BUF);
    }

    return (valid);
}


/*
*********************************************************************************************************
*                              USBH_RenesasUSBHS_DFIFO_RemBytesWr()
*
* Description : Write remaining bytes on DFIFO channel.
*
* Arguments   : p_reg               Pointer to registers structure.
*
*               p_buf               Pointer to buffer.
*
*               rem_bytes_cnt       Number of bytes remaining.
*
*               dfifo_ch_nbr        DFIFO channel number.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RenesasUSBHS_DFIFO_RemBytesWr (USBH_RENESAS_USBHS_REG  *p_reg,
                                                  CPU_INT08U              *p_buf,
                                                  CPU_INT08U               rem_bytes_cnt,
                                                  CPU_INT08U               dfifo_ch_nbr)
{
    CPU_INT08U  byte_cnt;


    if (rem_bytes_cnt > 0u) {
        DEF_BIT_CLR(p_reg->DxFIFOn[dfifo_ch_nbr].SEL,           /* Set fifo access width to 8 bit.                      */
                    RENESAS_USBHS_CFIFOSEL_MBW_MASK);

        DEF_BIT_SET(p_reg->DxFIFOn[dfifo_ch_nbr].SEL,
                    RENESAS_USBHS_CFIFOSEL_MBW_8);

                                                                /* Wr to FIFO.                                          */
        for (byte_cnt = 0u; byte_cnt < rem_bytes_cnt; byte_cnt++) {
            p_reg->DxFIFO[dfifo_ch_nbr] = p_buf[byte_cnt];
        }
    }
}


/*
*********************************************************************************************************
*                              USBH_RenesasUSBHS_DFIFO_RemBytesRd()
*
* Description : Reads remaining bytes on DFIFO channel.
*
* Arguments   : p_reg               Pointer to registers structure.
*
*               p_buf               Pointer to buffer.
*
*               rem_bytes_cnt       Number of bytes remaining.
*
*               dfifo_ch_nbr        DFIFO channel number.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RenesasUSBHS_DFIFO_RemBytesRd (USBH_RENESAS_USBHS_REG  *p_reg,
                                                  CPU_INT08U              *p_buf,
                                                  CPU_INT08U               rem_bytes_cnt,
                                                  CPU_INT08U               dfifo_ch_nbr)
{
    CPU_INT08U  byte_cnt;
    CPU_INT32U  word;


    if (rem_bytes_cnt > 0u) {
        word = p_reg->DxFIFO[dfifo_ch_nbr];

                                                                /* Rd from temp buf.                                    */
        for (byte_cnt = 0u; byte_cnt < rem_bytes_cnt; byte_cnt++) {
            p_buf[byte_cnt] = (CPU_INT08U)((word >> (DEF_INT_08_NBR_BITS * byte_cnt)) & DEF_INT_08_MASK);
        }
    }
}


/*
*********************************************************************************************************
*                                  USBH_RenesasUSBHS_FIFO_Acquire()
*
* Description : Acquires an available DFIFO channel to use for data transfer.
*
* Arguments   : p_hcd_data      Pointer to driver data.
*
*               pipe_nbr        Pipe number.
*
* Return(s)   : DFIFO number to use, if DMA enabled and one DFIFO channel is available and endpoint
*                                    type is not control,
*
*               RENESAS_USBHS_CFIFO, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  USBH_RenesasUSBHS_FIFO_Acquire (USBH_HCD_DATA  *p_hcd_data,
                                                    CPU_INT08U      pipe_nbr)
{
    CPU_INT08U  fifo_ch_nbr;


                                                                /* ------- DETERMINE WHICH FIFO CHANNEL TO USE -------- */
    if ((pipe_nbr           != 0u) &&
        (p_hcd_data->DMA_En == DEF_ENABLED)) {

        fifo_ch_nbr = CPU_CntTrailZeros08(p_hcd_data->AvailDFIFO);
        if (fifo_ch_nbr < RENESAS_USBHS_DFIFO_QTY) {
            DEF_BIT_CLR(p_hcd_data->AvailDFIFO,                 /* Mark DFIFO as unavailable.                           */
                        DEF_BIT(fifo_ch_nbr));
        } else {
            fifo_ch_nbr = RENESAS_USBHS_CFIFO;                  /* No DFIFO available. Use CFIFO in this case.          */
        }
    } else {
        fifo_ch_nbr = RENESAS_USBHS_CFIFO;
    }

    return (fifo_ch_nbr);
}


/*
*********************************************************************************************************
*                                   USBH_RenesasUSBHS_PipePID_Set()
*
* Description : Sets the current PID (state) of given pipe.
*
* Arguments   : p_reg       Pointer to registers structure.
*
*               pipe_nbr    Pipe number.
*
*               resp_pid    Response PID to set the endpoint to.
*
*                       RENESAS_USBHS_PIPExCTR_PID_NAK
*                       RENESAS_USBHS_PIPExCTR_PID_BUF
*                       RENESAS_USBHS_PIPExCTR_STALL1
*                       RENESAS_USBHS_PIPExCTR_STALL2
*
* Return(s)   : DEF_OK,   if NO error(s),
*
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RenesasUSBHS_PipePID_Set (USBH_RENESAS_USBHS_REG  *p_reg,
                                                    CPU_INT08U               pipe_nbr,
                                                    CPU_INT08U               resp_pid)
{
    CPU_BOOLEAN   valid;
    CPU_INT08U    prev_pid;
    CPU_INT08U    cnt;
    CPU_INT16U    pipe_ctr_val;
    CPU_REG16    *p_pipe_ctr_reg;


    if (pipe_nbr != 0u) {
        p_pipe_ctr_reg = &p_reg->PIPExCTR[pipe_nbr - 1u];
    } else {
        p_pipe_ctr_reg = &p_reg->DCPCTR;
    }

    valid    =  DEF_OK;
    prev_pid = (*p_pipe_ctr_reg & RENESAS_USBHS_PIPExCTR_PID_MASK);

    if (prev_pid == resp_pid) {                                 /* If PID already correct, return immediately.          */
        return (DEF_OK);
    }

    switch (resp_pid) {
        case RENESAS_USBHS_PIPExCTR_PID_BUF:
             if (prev_pid == RENESAS_USBHS_PIPExCTR_PID_STALL2) {
                 pipe_ctr_val = *p_pipe_ctr_reg;
                 DEF_BIT_CLR(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_MASK);
                 DEF_BIT_SET(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_STALL1);
                *p_pipe_ctr_reg = pipe_ctr_val;
             }

             pipe_ctr_val = *p_pipe_ctr_reg;
             DEF_BIT_CLR(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_MASK);
             DEF_BIT_SET(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_NAK);
            *p_pipe_ctr_reg = pipe_ctr_val;

             DEF_BIT_CLR(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_MASK);
             DEF_BIT_SET(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_BUF);
            *p_pipe_ctr_reg = pipe_ctr_val;
             break;


        case RENESAS_USBHS_PIPExCTR_PID_NAK:
             if (prev_pid == RENESAS_USBHS_PIPExCTR_PID_STALL2) {
                 pipe_ctr_val = *p_pipe_ctr_reg;
                 DEF_BIT_CLR(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_MASK);
                 DEF_BIT_SET(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_STALL1);
                *p_pipe_ctr_reg = pipe_ctr_val;
             }

             pipe_ctr_val = *p_pipe_ctr_reg;
             DEF_BIT_CLR(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_MASK);
             DEF_BIT_SET(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_NAK);
            *p_pipe_ctr_reg = pipe_ctr_val;

             if (prev_pid == RENESAS_USBHS_PIPExCTR_PID_BUF) {
                 if (DEF_BIT_IS_SET(*p_pipe_ctr_reg, RENESAS_USBHS_PIPExCTR_CSSTS) == DEF_YES) {
                     cnt = 0u;
                     do {
                         cnt++;
                         USBH_BSP_DlyUs(1u);
                     } while ((DEF_BIT_IS_SET(*p_pipe_ctr_reg, RENESAS_USBHS_PIPExCTR_CSSTS) == DEF_YES) &&
                              (cnt                                                           <  RENESAS_USBHS_RETRY_QTY));

                     if (cnt >= RENESAS_USBHS_RETRY_QTY) {
                         valid = DEF_FAIL;
                     }
                 }

                 if (DEF_BIT_IS_CLR(*p_pipe_ctr_reg, RENESAS_USBHS_PIPExCTR_PBUSY) == DEF_YES) {
                     break;
                 }

                 cnt = 0u;
                 do {
                     cnt++;
                     USBH_BSP_DlyUs(1u);
                 } while ((DEF_BIT_IS_SET(*p_pipe_ctr_reg, RENESAS_USBHS_PIPExCTR_PBUSY) == DEF_YES) &&
                          (cnt                                                           <  RENESAS_USBHS_RETRY_QTY));

                 if (cnt >= RENESAS_USBHS_RETRY_QTY) {
                     valid = DEF_FAIL;
                 }
             }
             break;


        case RENESAS_USBHS_PIPExCTR_PID_STALL1:
        case RENESAS_USBHS_PIPExCTR_PID_STALL2:
             pipe_ctr_val = *p_pipe_ctr_reg;
             DEF_BIT_CLR(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_MASK);

             if (prev_pid == RENESAS_USBHS_PIPExCTR_PID_BUF) {
                 DEF_BIT_SET(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_STALL2);
             } else {
                 DEF_BIT_SET(pipe_ctr_val, RENESAS_USBHS_PIPExCTR_PID_STALL1);
             }

            *p_pipe_ctr_reg = pipe_ctr_val;
             break;


        default:
             break;
    }

    return (valid);
}


/*
*********************************************************************************************************
*                                   USBH_RenesasUSBHS_CurPipeSet()
*
* Description : Binds an endpoint (pipe) to a FIFO channel.
*
* Arguments   : p_fifosel_reg       Pointer to FIFOSEL register.
*
*               pipe_nbr            Pipe number.
*
*               is_wr               Pipe direction (only used with control endpoint).
*
*                               DEF_YES     Endpoint direction is IN.
*                               DEF_NO      Endpoint direction is OUT.
*
* Return(s)   : DEF_OK,   if NO error(s),
*
*               DEF_FAIL, otherwise.
*
* Note(s)     : (1) Depending on the external bus speed of CPU, you may need to wait for 450ns here.
                    For details, look at the data sheet.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RenesasUSBHS_CurPipeSet (CPU_REG16    *p_fifosel_reg,
                                                   CPU_INT08U    pipe_nbr,
                                                   CPU_BOOLEAN   is_wr)
{
    CPU_INT08U  cnt;
    CPU_INT16U  fifosel_val;
    CPU_INT16U  fifosel_val_masked;


    fifosel_val = *p_fifosel_reg;

    DEF_BIT_CLR(fifosel_val, RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK);

                                                                /* Set ISEL bit for ctrl pipe.                          */
    if ((pipe_nbr == 0u) &&
        (is_wr    == DEF_YES)) {
        DEF_BIT_SET(fifosel_val, RENESAS_USBHS_CFIFOSEL_ISEL);
    } else {
        DEF_BIT_CLR(fifosel_val, RENESAS_USBHS_CFIFOSEL_ISEL);
    }
   *p_fifosel_reg = fifosel_val;

    fifosel_val_masked = fifosel_val & (RENESAS_USBHS_CFIFOSEL_ISEL | RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK);

                                                                /* Ensure FIFOSEL reg is cleared.                       */
    if ((*p_fifosel_reg & (RENESAS_USBHS_CFIFOSEL_ISEL | RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK)) != fifosel_val_masked) {

        cnt = 0u;
        do {
            cnt++;
            USBH_BSP_DlyUs(1u);
        } while (((*p_fifosel_reg & (RENESAS_USBHS_CFIFOSEL_ISEL | RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK)) != fifosel_val_masked) &&
                 ( cnt                                                                                    < RENESAS_USBHS_RETRY_QTY));
        if (cnt >= RENESAS_USBHS_RETRY_QTY) {
            return (DEF_FAIL);
        }
    }

                                                                /* Set FIFO to 32 bit width access.                     */
    DEF_BIT_CLR(fifosel_val, RENESAS_USBHS_CFIFOSEL_MBW_MASK);
    DEF_BIT_SET(fifosel_val, RENESAS_USBHS_CFIFOSEL_MBW_32);

    fifosel_val        |= (pipe_nbr & RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK);
   *p_fifosel_reg       =  fifosel_val;
    fifosel_val_masked  =  fifosel_val & (RENESAS_USBHS_CFIFOSEL_ISEL | RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK);

                                                                /* Ensure FIFOSEL reg is correctly assigned.            */
    if ((*p_fifosel_reg & (RENESAS_USBHS_CFIFOSEL_ISEL | RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK)) != fifosel_val_masked) {

        cnt = 0u;
        do {
            cnt++;
            USBH_BSP_DlyUs(1u);
        } while (((*p_fifosel_reg & (RENESAS_USBHS_CFIFOSEL_ISEL | RENESAS_USBHS_CFIFOSEL_CURPIPE_MASK)) != fifosel_val_masked) &&
                 ( cnt                                                                                    < RENESAS_USBHS_RETRY_QTY));
        if (cnt >= RENESAS_USBHS_RETRY_QTY) {
            return (DEF_FAIL);
        }
    }

    USBH_BSP_DlyUs(1u);                                         /* See note (1).                                        */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   USBH_RenesasUSBHS_BRDY_Event()
*
* Description : Handle buffer ready ISR for given pipe number.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               pipe_nbr        Pipe number.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RenesasUSBHS_BRDY_Event (USBH_HC_DRV  *p_hc_drv,
                                            CPU_INT08U    pipe_nbr)
{
    CPU_INT08U              *p_buf;
    CPU_INT32U               rx_len;
    USBH_URB                *p_urb;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_HCD_PIPE_INFO      *p_pipe_info;
    USBH_HCD_DFIFO_INFO     *p_dfifo_info;
    USBH_RENESAS_USBHS_REG  *p_reg;


    p_reg         = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_hcd_data    = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_pipe_info   = &p_hcd_data->PipeInfoTblPtr[pipe_nbr];
    p_dfifo_info  = &p_hcd_data->DFIFO_InfoTbl[p_pipe_info->FIFO_IxUsed];
    p_urb         =  p_pipe_info->URB_Ptr;

    if (p_pipe_info->FIFO_IxUsed != RENESAS_USBHS_CFIFO) {
        if (p_dfifo_info->XferIsRd == DEF_YES) {
            (void)USBH_RenesasUSBHS_DFIFO_Rd(p_reg,             /* Trigger DMA rd.                                      */
                                             p_pipe_info,
                                            &p_hcd_data->DFIFO_InfoTbl[p_pipe_info->FIFO_IxUsed],
                                             p_hc_drv->Nbr,
                                             p_pipe_info->FIFO_IxUsed,
                                             pipe_nbr);
        } else {
            (void)USBH_RenesasUSBHS_DFIFO_Wr(p_reg,             /* Trigger DMA wr.                                      */
                                             p_pipe_info,
                                            &p_hcd_data->DFIFO_InfoTbl[p_pipe_info->FIFO_IxUsed],
                                             p_hc_drv->Nbr,
                                             p_pipe_info->FIFO_IxUsed,
                                             pipe_nbr);
        }
    } else {
        (void)USBH_RenesasUSBHS_CurPipeSet(&p_reg->CFIFOSEL,
                                            pipe_nbr,
                                            DEF_NO);

        rx_len                 = (p_reg->CFIFOCTR & RENESAS_USBHS_FIFOCTR_DTLN_MASK);
        p_pipe_info->LastRxLen =  rx_len;

        if ((rx_len                            !=  0u) &&
            (rx_len % p_pipe_info->MaxPktSize  ==  0u) &&
            (p_urb->UserBufLen                 >  (rx_len + p_urb->XferLen))) {

            rx_len =  DEF_MIN(rx_len, p_urb->UserBufLen - p_urb->XferLen);
            p_buf  = (CPU_INT08U *)p_urb->UserBufPtr;
            (void)USBH_RenesasUSBHS_CFIFO_Rd(p_reg,
                                            &p_buf[p_urb->XferLen],
                                             rx_len);

            p_urb->XferLen += rx_len;
        } else {

            p_pipe_info->URB_Ptr->Err = USBH_ERR_NONE;
            USBH_URB_Done(p_urb);


            (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                                pipe_nbr,
                                                RENESAS_USBHS_PIPExCTR_PID_NAK);

            DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));
            DEF_BIT_CLR(p_reg->NRDYENB, DEF_BIT(pipe_nbr));
        }
    }
}


/*
*********************************************************************************************************
*                                   USBH_RenesasUSBHS_BEMP_Event()
*
* Description : Handles buffer empty ISR for given pipe number.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               pipe_nbr        Pipe number.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RenesasUSBHS_BEMP_Event (USBH_HC_DRV  *p_hc_drv,
                                            CPU_INT08U    pipe_nbr)
{
    CPU_INT32U               tx_len;
    USBH_URB                *p_urb;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_HCD_DFIFO_INFO     *p_dfifo_info;
    USBH_HCD_PIPE_INFO      *p_pipe_info;
    USBH_RENESAS_USBHS_REG  *p_reg;
    CPU_SR_ALLOC();


    p_reg       = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_hcd_data  = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_pipe_info = &p_hcd_data->PipeInfoTblPtr[pipe_nbr];
    p_urb       =  p_pipe_info->URB_Ptr;

    if (p_pipe_info->FIFO_IxUsed != RENESAS_USBHS_CFIFO) {
        p_dfifo_info = &p_hcd_data->DFIFO_InfoTbl[p_pipe_info->FIFO_IxUsed];

        CPU_CRITICAL_ENTER();
        p_dfifo_info->USB_XferByteCnt+= p_dfifo_info->CopyDataCnt;
        p_dfifo_info->CopyDataCnt     = 0u;
        CPU_CRITICAL_EXIT();

        if (p_dfifo_info->USB_XferByteCnt >= p_dfifo_info->BufLen) {

            (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                                pipe_nbr,
                                                RENESAS_USBHS_PIPExCTR_PID_NAK);

            DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));     /* Disable int.                                         */
            DEF_BIT_CLR(p_reg->BEMPENB, DEF_BIT(pipe_nbr));

            USBH_URB_Done(p_pipe_info->URB_Ptr);
        }
    } else {
        if (p_urb->XferLen >= p_urb->UserBufLen) {
            DEF_BIT_CLR(p_reg->BEMPENB, DEF_BIT(pipe_nbr));
            DEF_BIT_CLR(p_reg->NRDYENB, DEF_BIT(pipe_nbr));

            (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,
                                                pipe_nbr,
                                                RENESAS_USBHS_PIPExCTR_PID_NAK);

            if (p_pipe_info->UseDblBuf == DEF_YES) {
                CPU_CRITICAL_ENTER();                           /* Re-enable double buffering.                          */
                p_reg->PIPESEL = (pipe_nbr & RENESAS_USBHS_PIPESEL_PIPESEL_MASK);
                DEF_BIT_SET(p_reg->PIPECFG, RENESAS_USBHS_PIPCFG_DBLB);
                p_reg->PIPESEL =  0u;
                CPU_CRITICAL_EXIT();
            }

            p_urb->Err = USBH_ERR_NONE;
            USBH_URB_Done(p_urb);
        } else {
            tx_len = DEF_MIN(p_pipe_info->MaxBufLen, (p_urb->UserBufLen - p_urb->XferLen));

            (void)USBH_RenesasUSBHS_CFIFO_Wr(p_reg,
                                             p_pipe_info,
                                             pipe_nbr,
                                            &((CPU_INT08U *)p_urb->UserBufPtr)[p_urb->XferLen],
                                             tx_len);

            p_urb->XferLen+= tx_len;
        }
    }
}


/*
*********************************************************************************************************
*                                   USBH_RenesasUSBHS_DFIFO_Event()
*
* Description : Handle DFIFO DMA ISR for given DFIFO channel.
*
* Argument(s) : p_hc_drv    Pointer to host controller driver structure.
*
*               dfifo_nbr   DFIFO channel number.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RenesasUSBHS_DFIFO_Event (USBH_HC_DRV  *p_hc_drv,
                                             CPU_INT08U    dfifo_nbr)
{
    CPU_INT08U               pipe_nbr;
    CPU_INT32U               xfer_len_next;
    USBH_HCD_DATA           *p_hcd_data;
    USBH_HCD_DFIFO_INFO     *p_dfifo_info;
    USBH_HCD_PIPE_INFO      *p_pipe_info;
    USBH_RENESAS_USBHS_REG  *p_reg;
    CPU_SR_ALLOC();


    p_reg        = (USBH_RENESAS_USBHS_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_hcd_data   = (USBH_HCD_DATA          *)p_hc_drv->DataPtr;
    p_dfifo_info = &p_hcd_data->DFIFO_InfoTbl[dfifo_nbr];
    pipe_nbr     =  p_dfifo_info->PipeNbr;
    p_pipe_info  = &p_hcd_data->PipeInfoTblPtr[pipe_nbr];

    if (p_dfifo_info->XferIsRd == DEF_YES) {
        xfer_len_next = 0u;
        CPU_CRITICAL_ENTER();                                   /* Increment DMA xfer cnt with oldest rx xfer.          */
        p_dfifo_info->DMA_XferByteCnt += p_dfifo_info->DMA_XferTbl[p_dfifo_info->DMA_XferOldestIx];
        p_dfifo_info->DMA_XferOldestIx++;
        if (p_dfifo_info->DMA_XferOldestIx >= RENESAS_USBHS_RX_Q_SIZE) {
            p_dfifo_info->DMA_XferOldestIx = 0u;
        }

                                                                /* Determine if DMA xfer start or if xfer is cmpl.      */
        if (p_dfifo_info->DMA_XferOldestIx != p_dfifo_info->DMA_XferNewestIx) {
            xfer_len_next = p_dfifo_info->DMA_XferTbl[p_dfifo_info->DMA_XferOldestIx];
            CPU_CRITICAL_EXIT();
        } else if (p_dfifo_info->XferEndFlag == DEF_YES) {
            CPU_CRITICAL_EXIT();

            DEF_BIT_CLR(p_reg->BRDYENB, DEF_BIT(pipe_nbr));
                                                                /* Read last bytes (if any) from DFIFO using CPU...     */
                                                                /* ...DMA performs only 32-bit access to DFIFO.         */
            USBH_RenesasUSBHS_DFIFO_RemBytesRd(         p_reg,
                                               (void *)&p_dfifo_info->BufPtr[p_dfifo_info->DMA_XferByteCnt],
                                                        p_dfifo_info->RemByteCnt,
                                                        dfifo_nbr);
            USBH_URB_Done(p_pipe_info->URB_Ptr);
        } else {
            CPU_CRITICAL_EXIT();
        }

        if (xfer_len_next > 0u) {
            (void)USBH_BSP_DMA_CopyStart(         p_hc_drv->Nbr,
                                                  dfifo_nbr,
                                                  DEF_YES,
                                         (void *)&p_dfifo_info->BufPtr[p_dfifo_info->DMA_XferByteCnt],
                                                 &p_reg->DxFIFO[dfifo_nbr],
                                                  xfer_len_next);
        }
    } else {
        USBH_RenesasUSBHS_DFIFO_RemBytesWr(p_reg,               /* Process any remaining bytes in data xfer.            */
                                          &p_dfifo_info->BufPtr[p_dfifo_info->DMA_XferByteCnt],
                                           p_dfifo_info->RemByteCnt,
                                           dfifo_nbr);

        CPU_CRITICAL_ENTER();
        p_dfifo_info->CopyDataCnt     += p_dfifo_info->RemByteCnt;
        p_dfifo_info->DMA_XferByteCnt += p_dfifo_info->RemByteCnt;
        p_dfifo_info->CopyDataCnt     += p_dfifo_info->DMA_CurTxLen;
        CPU_CRITICAL_EXIT();

                                                                /* End of wr to FIFO if buf len not multiple of buf len.*/
        if (((p_dfifo_info->DMA_XferByteCnt % p_pipe_info->MaxBufLen)                   != 0u) &&
            ( DEF_BIT_IS_CLR(p_reg->DxFIFOn[dfifo_nbr].CTR, RENESAS_USBHS_FIFOCTR_BVAL) == DEF_YES)) {
            DEF_BIT_SET(p_reg->DxFIFOn[dfifo_nbr].CTR, RENESAS_USBHS_FIFOCTR_BVAL);
        }

        (void)USBH_RenesasUSBHS_PipePID_Set(p_reg,              /* Enable data transmission on pipe.                    */
                                            pipe_nbr,
                                            RENESAS_USBHS_PIPExCTR_PID_BUF);
    }
}

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
*                                           USB HOST DRIVER
*                                         RENESAS RX600 (FS)
*
* Filename : usbh_rx600.h
* Version  : V3.42.00
*********************************************************************************************************
*/

#define  USBH_RX600_MODULE


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   MICRIUM_SOURCE
#include  "usbh_hcd_rx600.h"
#include  "../../Source/usbh_hub.h"
#include  <usbh_cfg.h>


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  union  usbh_rx600_fifo {                               /* ----------------- FIFO DATA STRUCT ----------------- */
    CPU_REG16       WORD;
    struct {
        CPU_REG08   LO;
        CPU_REG08   HI;
    } BYTE;
} USBH_RX600_FIFO;

                                                                /* ---------- RX600 USB CONTROLLER REGISTERS ---------- */
typedef  struct  usbh_rx600_reg {
    CPU_REG16        SYSCFG;                                    /* System configuration control.                        */
    CPU_REG08        RSVD_00[2u];
    CPU_REG16        SYSSTS0;                                   /* System configuration status.                         */
    CPU_REG08        RSVD_01[2u];
    CPU_REG16        DVSTCTR0;                                  /* Device state control.                                */
    CPU_REG08        RSVD_02[10u];
    USBH_RX600_FIFO  CFIFO;                                     /* CFIFO port.                                          */
    CPU_REG08        RSVD_03[2u];
    USBH_RX600_FIFO  D0FIFO;                                    /* D0FIFO port.                                         */
    CPU_REG08        RSVD_04[2u];
    USBH_RX600_FIFO  D1FIFO;                                    /* D1FIFO port.                                         */
    CPU_REG08        RSVD_05[2u];
    CPU_REG16        CFIFOSEL;                                  /* CFIFO port select.                                   */
    CPU_REG16        CFIFOCTR;                                  /* CFIFO port control.                                  */
    CPU_REG08        RSVD_06[4u];
    CPU_REG16        D0FIFOSEL;                                 /* D0FIFO port select.                                  */
    CPU_REG16        D0FIFOCTR;                                 /* D0FIFO port control.                                 */
    CPU_REG16        D1FIFOSEL;                                 /* D0FIFO port select.                                  */
    CPU_REG16        D1FIFOCTR;                                 /* D0FIFO port control.                                 */
    CPU_REG16        INTENB0;                                   /* Interrupt enable register 0.                         */
    CPU_REG16        INTENB1;                                   /* Interrupt enable register 1.                         */
    CPU_REG08        RSVD_07[2u];
    CPU_REG16        BRDYENB;                                   /* BRDY interrupt enable.                               */
    CPU_REG16        NRDYENB;                                   /* NRDY interrupt enable.                               */
    CPU_REG16        BEMPENB;                                   /* BEMP interrupt enable.                               */
    CPU_REG16        SOFCFG;                                    /* SOF output configuration.                            */
    CPU_REG08        RSVD_08[2u];
    CPU_REG16        INTSTS0;                                   /* Interrupt status register 0.                         */
    CPU_REG16        INTSTS1;                                   /* Interrupt status register 1.                         */
    CPU_REG08        RSVD_09[2u];
    CPU_REG16        BRDYSTS;                                   /* BRDY interrupt status.                               */
    CPU_REG16        NRDYSTS;                                   /* NRDY interrupt status.                               */
    CPU_REG16        BEMPSTS;                                   /* BEMP interrupt status.                               */
    CPU_REG16        FRMNUM;                                    /* Frame number.                                        */
    CPU_REG16        DVCHGR;                                    /* Device state change.                                 */
    CPU_REG16        USBADDR;                                   /* USB address.                                         */
    CPU_REG08        RSVD_10[2u];
    CPU_REG16        USBREQ;                                    /* USB request type.                                    */
    CPU_REG16        USBVAL;                                    /* USB request value.                                   */
    CPU_REG16        USBINDX;                                   /* USB request index.                                   */
    CPU_REG16        USBLENG;                                   /* USB request length.                                  */
    CPU_REG16        DCPCFG;                                    /* DCP configuration.                                   */
    CPU_REG16        DCPMAXP;                                   /* DCP maximum packet size.                             */
    CPU_REG16        DCPCTR;                                    /* DCP control register.                                */
    CPU_REG08        RSVD_11[2u];
    CPU_REG16        PIPESEL;                                   /* Pipe window select.                                  */
    CPU_REG08        RSVD_12[2u];
    CPU_REG16        PIPECFG;                                   /* Pipe configuration.                                  */
    CPU_REG08        RSVD_13[2u];
    CPU_REG16        PIPEMAXP;                                  /* Pipe max packet size.                                */
    CPU_REG16        PIPEPERI;                                  /* Pipe cycle control.                                  */
    CPU_REG16        PIPEnCTR[9u];                              /* Pipe n control.                                      */
    CPU_REG08        RSVD_14[2u];
    CPU_REG16        PIPE1TRE;                                  /* Pipe 1 transaction counter enable.                   */
    CPU_REG16        PIPE1TRN;                                  /* Pipe 1 transaction counter.                          */
    CPU_REG16        PIPE2TRE;                                  /* Pipe 2 transaction counter enable.                   */
    CPU_REG16        PIPE2TRN;                                  /* Pipe 2 transaction counter.                          */
    CPU_REG16        PIPE3TRE;                                  /* Pipe 3 transaction counter enable.                   */
    CPU_REG16        PIPE3TRN;                                  /* Pipe 3 transaction counter.                          */
    CPU_REG16        PIPE4TRE;                                  /* Pipe 4 transaction counter enable.                   */
    CPU_REG16        PIPE4TRN;                                  /* Pipe 4 transaction counter.                          */
    CPU_REG16        PIPE5TRE;                                  /* Pipe 5 transaction counter enable.                   */
    CPU_REG16        PIPE5TRN;                                  /* Pipe 5 transaction counter.                          */
    CPU_REG08        RSVD_15[55u];
    CPU_REG16        DEVADDn[6u];                               /* Device address n configuration.                      */
}  USBH_RX600_REG;

typedef  struct  usbh_rx600_pipe_info {
    CPU_INT16U   EP_Nbr;                                        /* Lookup table: pipe_nbr   -> phy_ep_num.              */
    CPU_INT16U   MaxPktSize;                                    /* Lookup table: pipe_nbr   -> max_pkt_size.            */
    USBH_URB    *URB_Ptr;                                       /* Lookup table: pipe_nbr   -> URB ptr.                 */
    USBH_EP     *EP_Ptr;                                        /* Lookup table: pipe_nbr   -> EP ptr.                  */
} USBH_RX600_PIPE_INFO;

typedef  struct  usbh_rx600_dev {
    CPU_INT08U             RH_Desc[sizeof(USBH_HUB_DESC)];      /* RH desc content.                                     */

    CPU_INT16U             PipeUsed;                            /* Bit array for bulk, isoc, and intr pipe mgmt.        */
    CPU_INT08U             EP_Tbl[USBH_CFG_MAX_NBR_DEVS + 2u][32u]; /* Lookup table: phy_ep_num -> pipe_nbr.            */

    USBH_RX600_PIPE_INFO   PipeInfoTbl[10u];                    /* Contains info on pipes.                              */

    CPU_INT16U             RH_PortStatus;                       /* PortStatus word for RH.                              */
    CPU_INT16U             RH_PortStatusChng;                   /* PortStatusChange word for RH.                        */
} USBH_RX600_DEV;

/*
*********************************************************************************************************
*                                        RX600 REGISTER DEFINES
*********************************************************************************************************
*/

#define  USBH_RX600_REG_DPUSR_BASE_ADDR           0x000A0400u
#define  USBH_RX600_REG_DPUSR0R                 (*(CPU_REG32 *)(USBH_RX600_REG_DPUSR_BASE_ADDR + 0x000))
#define  USBH_RX600_REG_DPUSR1R                 (*(CPU_REG32 *)(USBH_RX600_REG_DPUSR_BASE_ADDR + 0x004))


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_RX600_SYSCFG_SCKE                         DEF_BIT_10
#define  USBH_RX600_SYSCFG_DCFM                         DEF_BIT_06
#define  USBH_RX600_SYSCFG_DRPD                         DEF_BIT_05
#define  USBH_RX600_SYSCFG_DPRPU                        DEF_BIT_04
#define  USBH_RX600_SYSCFG_USBE                         DEF_BIT_00

#define  USBH_RX600_SYSSTS0_OCVMON                     (DEF_BIT_15 | DEF_BIT_14)
#define  USBH_RX600_SYSSTS0_OVRCURA                     DEF_BIT_15
#define  USBH_RX600_SYSSTS0_OVRCURB                     DEF_BIT_14
#define  USBH_RX600_SYSSTS0_HTACT                       DEF_BIT_08
#define  USBH_RX600_SYSSTS0_IDMON                       DEF_BIT_02
#define  USBH_RX600_SYSSTS0_LNST                       (DEF_BIT_01 | DEF_BIT_00)

#define  USBH_RX600_LNST_SE0                            0x00u
#define  USBH_RX600_LNST_J_STATE                        0x01u
#define  USBH_RX600_LNST_K_STATE                        0x02u
#define  USBH_RX600_LNST_SE1                            0x03u

#define  USBH_RX600_DVSTCTR0_HNPBTOA                    DEF_BIT_11
#define  USBH_RX600_DVSTCTR0_EXICEN                     DEF_BIT_10
#define  USBH_RX600_DVSTCTR0_VBUSEN                     DEF_BIT_09
#define  USBH_RX600_DVSTCTR0_WKUP                       DEF_BIT_08
#define  USBH_RX600_DVSTCTR0_RWUPE                      DEF_BIT_07
#define  USBH_RX600_DVSTCTR0_USBRST                     DEF_BIT_06
#define  USBH_RX600_DVSTCTR0_RESUME                     DEF_BIT_05
#define  USBH_RX600_DVSTCTR0_UACT                       DEF_BIT_04
#define  USBH_RX600_DVSTCTR0_RHST                       DEF_BIT_FIELD(3u, 0u)

#define  USBH_RX600_RHST_ND                             0x00u
#define  USBH_RX600_RHST_LS                             0x01u
#define  USBH_RX600_RHST_FS                             0x02u
#define  USBH_RX600_RHST_RST                            0x04u

#define  USBH_RX600_CFIFOSEL_RCNT                       DEF_BIT_15
#define  USBH_RX600_CFIFOSEL_REW                        DEF_BIT_14
#define  USBH_RX600_CFIFOSEL_MBW                        DEF_BIT_10
#define  USBH_RX600_CFIFOSEL_BIGEND                     DEF_BIT_08
#define  USBH_RX600_CFIFOSEL_ISEL                       DEF_BIT_05
#define  USBH_RX600_CFIFOSEL_CURPIPE                    DEF_BIT_FIELD(4u, 0u)

#define  USBH_RX600_DFIFOSEL_RCNT                       DEF_BIT_15
#define  USBH_RX600_DFIFOSEL_REW                        DEF_BIT_14
#define  USBH_RX600_DFIFOSEL_DCLRM                      DEF_BIT_13
#define  USBH_RX600_DFIFOSEL_DREQE                      DEF_BIT_12
#define  USBH_RX600_DFIFOSEL_MBW                        DEF_BIT_10
#define  USBH_RX600_DFIFOSEL_BIGEND                     DEF_BIT_08
#define  USBH_RX600_DFIFOSEL_ISEL                       DEF_BIT_05
#define  USBH_RX600_DFIFOSEL_CURPIPE                    DEF_BIT_FIELD(4u, 0u)

#define  USBH_RX600_FIFOCTR_BVAL                        DEF_BIT_15
#define  USBH_RX600_FIFOCTR_BCLR                        DEF_BIT_14
#define  USBH_RX600_FIFOCTR_FRDY                        DEF_BIT_13
#define  USBH_RX600_FIFOCTR_DTLN                        DEF_BIT_FIELD(8u, 0u)

#define  USBH_RX600_INTENB0_VBSE                        DEF_BIT_15
#define  USBH_RX600_INTENB0_RSME                        DEF_BIT_14
#define  USBH_RX600_INTENB0_SOFE                        DEF_BIT_13
#define  USBH_RX600_INTENB0_DVSE                        DEF_BIT_12
#define  USBH_RX600_INTENB0_CTRE                        DEF_BIT_11
#define  USBH_RX600_INTENB0_BEMPE                       DEF_BIT_10
#define  USBH_RX600_INTENB0_NRDYE                       DEF_BIT_09
#define  USBH_RX600_INTENB0_BRDYE                       DEF_BIT_08

#define  USBH_RX600_INTENB1_OVRCRE                      DEF_BIT_15
#define  USBH_RX600_INTENB1_BCHGE                       DEF_BIT_14
#define  USBH_RX600_INTENB1_DTCHE                       DEF_BIT_12
#define  USBH_RX600_INTENB1_ATTCHE                      DEF_BIT_11
#define  USBH_RX600_INTENB1_EOFERRE                     DEF_BIT_06
#define  USBH_RX600_INTENB1_SIGNE                       DEF_BIT_05
#define  USBH_RX600_INTENB1_SACKE                       DEF_BIT_04

#define  USBH_RX600_SOFCFG_TRNENSEL                     DEF_BIT_08
#define  USBH_RX600_SOFCFG_BRDYM                        DEF_BIT_06
#define  USBH_RX600_SOFCFG_EDGESTS                      DEF_BIT_04

#define  USBH_RX600_INTSTS0_VBINT                       DEF_BIT_15
#define  USBH_RX600_INTSTS0_RESM                        DEF_BIT_14
#define  USBH_RX600_INTSTS0_SOFR                        DEF_BIT_13
#define  USBH_RX600_INTSTS0_DVST                        DEF_BIT_12
#define  USBH_RX600_INTSTS0_CTRT                        DEF_BIT_11
#define  USBH_RX600_INTSTS0_BEMP                        DEF_BIT_10
#define  USBH_RX600_INTSTS0_NRDY                        DEF_BIT_09
#define  USBH_RX600_INTSTS0_BRDY                        DEF_BIT_08
#define  USBH_RX600_INTSTS0_VBSTS                       DEF_BIT_07
#define  USBH_RX600_INTSTS0_DVSQ                       (DEF_BIT_06 | DEF_BIT_05)
#define  USBH_RX600_INTSTS0_VALID                       DEF_BIT_03
#define  USBH_RX600_INTSTS0_CTSQ                        DEF_BIT_FIELD(3u, 0u)
#define  USBH_RX600_INTSTS0_ALL                        (USBH_RX600_INTSTS0_VBINT | \
                                                        USBH_RX600_INTSTS0_RESM  | \
                                                        USBH_RX600_INTSTS0_SOFR  | \
                                                        USBH_RX600_INTSTS0_DVST  | \
                                                        USBH_RX600_INTSTS0_CTRT  | \
                                                        USBH_RX600_INTSTS0_VALID)

#define  USBH_RX600_DVSQ_PWR                            0x00u
#define  USBH_RX600_DVSQ_DFLT                           0x10u
#define  USBH_RX600_DVSQ_ADDR                           0x20u
#define  USBH_RX600_DVSQ_CFG                            0x30u

#define  USBH_RX600_CTSQ_SETUP                          0x00u
#define  USBH_RX600_CTSQ_CTRL_RD_DATA                   0x01u
#define  USBH_RX600_CTSQ_CTRL_RD_STATUS                 0x02u
#define  USBH_RX600_CTSQ_CTRL_WR_DATA                   0x03u
#define  USBH_RX600_CTSQ_CTRL_WR_STATUS                 0x04u
#define  USBH_RX600_CTSQ_CTRL_WR_ND_STATUS              0x05u
#define  USBH_RX600_CTSQ_CTRL_TX_SEQ_ERR                0x06u

#define  USBH_RX600_INTSTS1_OVRCR                       DEF_BIT_15
#define  USBH_RX600_INTSTS1_BCHG                        DEF_BIT_14
#define  USBH_RX600_INTSTS1_DTCH                        DEF_BIT_12
#define  USBH_RX600_INTSTS1_ATTCH                       DEF_BIT_11
#define  USBH_RX600_INTSTS1_EOFERR                      DEF_BIT_06
#define  USBH_RX600_INTSTS1_SIGN                        DEF_BIT_05
#define  USBH_RX600_INTSTS1_SACK                        DEF_BIT_04
#define  USBH_RX600_INTSTS1_ALL                        (USBH_RX600_INTSTS1_OVRCR  | \
                                                        USBH_RX600_INTSTS1_BCHG   | \
                                                        USBH_RX600_INTSTS1_DTCH   | \
                                                        USBH_RX600_INTSTS1_ATTCH  | \
                                                        USBH_RX600_INTSTS1_EOFERR | \
                                                        USBH_RX600_INTSTS1_SIGN   | \
                                                        USBH_RX600_INTSTS1_SACK)


#define  USBH_RX600_FRMNUM_OVRN                         DEF_BIT_15
#define  USBH_RX600_FRMNUM_CRCE                         DEF_BIT_14
#define  USBH_RX600_FRMNUM_FRNM                         DEF_BIT_FIELD(11u, 0u)

#define  USBH_RX600_DVCHGR_DVCHG                        DEF_BIT_15

#define  USBH_RX600_USBADDR_STSRECOV                    DEF_BIT_FIELD(4u, 8u)
#define  USBH_RX600_USBADDR_USBADDR                     DEF_BIT_FIELD(7u, 0u)

#define  USBH_RX600_STSRECOV_DFLT                     0x0900u
#define  USBH_RX600_STSRECOV_ADDR                     0x0A00u
#define  USBH_RX600_STSRECOV_CFG                      0x0B00u

#define  USBH_RX600_USBREQ_BREQUEST                     DEF_BIT_FIELD(8u, 8u)
#define  USBH_RX600_USBREQ_BMREQUESTTYPE                DEF_BIT_FIELD(8u, 0u)

#define  USBH_RX600_DCPCFG_SHTNAK                       DEF_BIT_07
#define  USBH_RX600_DCPCFG_DIR                          DEF_BIT_04

#define  USBH_RX600_DCPMAXP_DEVSEL                      DEF_BIT_FIELD(4u, 12u)
#define  USBH_RX600_DCPMAXP_MXPS                        DEF_BIT_FIELD(7u, 0u)

#define  USBH_RX600_DEV_SEL_00                        0x0000u
#define  USBH_RX600_DEV_SEL_01                        0x1000u
#define  USBH_RX600_DEV_SEL_02                        0x2000u
#define  USBH_RX600_DEV_SEL_03                        0x3000u
#define  USBH_RX600_DEV_SEL_04                        0x4000u
#define  USBH_RX600_DEV_SEL_05                        0x5000u

#define  USBH_RX600_DCPCTR_BSTS                         DEF_BIT_15
#define  USBH_RX600_DCPCTR_SUREQ                        DEF_BIT_14
#define  USBH_RX600_DCPCTR_SUREQCLR                     DEF_BIT_11
#define  USBH_RX600_DCPCTR_SQCLR                        DEF_BIT_08
#define  USBH_RX600_DCPCTR_SQSET                        DEF_BIT_07
#define  USBH_RX600_DCPCTR_SQMON                        DEF_BIT_06
#define  USBH_RX600_DCPCTR_PBUSY                        DEF_BIT_05
#define  USBH_RX600_DCPCTR_CCPL                         DEF_BIT_02
#define  USBH_RX600_DCPCTR_PID                         (DEF_BIT_01 | DEF_BIT_00)

#define  USBH_RX600_PID_NAK                             0x00u
#define  USBH_RX600_PID_BUF                             0x01u
#define  USBH_RX600_PID_STALL_1                         0x02u
#define  USBH_RX600_PID_STALL_2                         0x03u

#define  USBH_RX600_PIPECFG_TYPE                       (DEF_BIT_15 | DEF_BIT_14)
#define  USBH_RX600_PIPECFG_BFRE                        DEF_BIT_10
#define  USBH_RX600_PIPECFG_DBLB                        DEF_BIT_09
#define  USBH_RX600_PIPECFG_SHTNAK                      DEF_BIT_07
#define  USBH_RX600_PIPECFG_DIR                         DEF_BIT_04
#define  USBH_RX600_PIPECFG_EPNUM                       DEF_BIT_FIELD(4u, 0u)

#define  USBH_RX600_TYPE_BULK                         0x4000u
#define  USBH_RX600_TYPE_INT                          0x8000u
#define  USBH_RX600_TYPE_ISOC                         0xC000u

#define  USBH_RX600_PIPEMAXP_DEVSEL                     DEF_BIT_FIELD(4u, 0u)
#define  USBH_RX600_PIPEMAXP_MXPS                       DEF_BIT_FIELD(9u, 0u)

#define  USBH_RX600_PIPEPERI_IFIS                       DEF_BIT_12
#define  USBH_RX600_PIPEPERI_IITV                       DEF_BIT_FIELD(3u, 0u)

#define  USBH_RX600_PIPECTR_BSTS                        DEF_BIT_15
#define  USBH_RX600_PIPECTR_INBUFM                      DEF_BIT_14
#define  USBH_RX600_PIPECTR_ATREPM                      DEF_BIT_10
#define  USBH_RX600_PIPECTR_ACLRM                       DEF_BIT_09
#define  USBH_RX600_PIPECTR_SQCLR                       DEF_BIT_08
#define  USBH_RX600_PIPECTR_SQSET                       DEF_BIT_07
#define  USBH_RX600_PIPECTR_SQMON                       DEF_BIT_06
#define  USBH_RX600_PIPECTR_PBUSY                       DEF_BIT_05
#define  USBH_RX600_PIPECTR_PID                        (DEF_BIT_01 | DEF_BIT_00)

#define  USBH_RX600_DEVADD_SBSPD                       (DEF_BIT_07 | DEF_BIT_06)

#define  USBH_RX600_SPD_NONE                            DEF_BIT_NONE
#define  USBH_RX600_SPD_LS                              DEF_BIT_06
#define  USBH_RX600_SPD_FS                              DEF_BIT_07

#define  USBH_RX600_CTRL_PIPE                           DEF_BIT_00

#define  USBH_RX600_ISOC_PIPE_ALL                      (USBH_RX600_ISOC_PIPE_01 | USBH_RX600_ISOC_PIPE_02)
#define  USBH_RX600_ISOC_PIPE_01                        DEF_BIT_01
#define  USBH_RX600_ISOC_PIPE_02                        DEF_BIT_02

#define  USBH_RX600_BULK_PIPE_ALL                      (USBH_RX600_BULK_PIPE_01 | USBH_RX600_BULK_PIPE_02 | USBH_RX600_BULK_PIPE_03)
#define  USBH_RX600_BULK_PIPE_01                        DEF_BIT_03
#define  USBH_RX600_BULK_PIPE_02                        DEF_BIT_04
#define  USBH_RX600_BULK_PIPE_03                        DEF_BIT_05

#define  USBH_RX600_INT_PIPE_ALL                       (USBH_RX600_INT_PIPE_01 | USBH_RX600_INT_PIPE_02 | USBH_RX600_INT_PIPE_03 | USBH_RX600_INT_PIPE_04)
#define  USBH_RX600_INT_PIPE_01                         DEF_BIT_06
#define  USBH_RX600_INT_PIPE_02                         DEF_BIT_07
#define  USBH_RX600_INT_PIPE_03                         DEF_BIT_08
#define  USBH_RX600_INT_PIPE_04                         DEF_BIT_09

#define  USBH_RX600_PIPE_00                             0x00u
#define  USBH_RX600_PIPE_01                             0x01u
#define  USBH_RX600_PIPE_02                             0x02u
#define  USBH_RX600_PIPE_03                             0x03u
#define  USBH_RX600_PIPE_04                             0x04u
#define  USBH_RX600_PIPE_05                             0x05u
#define  USBH_RX600_PIPE_06                             0x06u
#define  USBH_RX600_PIPE_07                             0x07u
#define  USBH_RX600_PIPE_08                             0x08u
#define  USBH_RX600_PIPE_09                             0x09u

#define  USBH_RX600_DPUSR0R_DVBSTS1                     DEF_BIT_31
#define  USBH_RX600_DPUSR0R_DOVCB1                      DEF_BIT_29
#define  USBH_RX600_DPUSR0R_DOVCA1                      DEF_BIT_28
#define  USBH_RX600_DPUSR0R_DM1                         DEF_BIT_25
#define  USBH_RX600_DPUSR0R_DP1                         DEF_BIT_24
#define  USBH_RX600_DPUSR0R_DVBSTS0                     DEF_BIT_23
#define  USBH_RX600_DPUSR0R_DOVCB0                      DEF_BIT_21
#define  USBH_RX600_DPUSR0R_DOVCA0                      DEF_BIT_20
#define  USBH_RX600_DPUSR0R_DM0                         DEF_BIT_17
#define  USBH_RX600_DPUSR0R_DP0                         DEF_BIT_16
#define  USBH_RX600_DPUSR0R_FIXPHY1                     DEF_BIT_12
#define  USBH_RX600_DPUSR0R_SRPC1                       DEF_BIT_08
#define  USBH_RX600_DPUSR0R_FIXPHY0                     DEF_BIT_04
#define  USBH_RX600_DPUSR0R_SRPC0                       DEF_BIT_00

#define  USBH_RX600_DPUSR1R_DVBINT1                     DEF_BIT_31
#define  USBH_RX600_DPUSR1R_DOVRCRB1                    DEF_BIT_29
#define  USBH_RX600_DPUSR1R_DOVRCRA1                    DEF_BIT_28
#define  USBH_RX600_DPUSR1R_DMINT1                      DEF_BIT_25
#define  USBH_RX600_DPUSR1R_DPINT1                      DEF_BIT_24
#define  USBH_RX600_DPUSR1R_DVBINT0                     DEF_BIT_23
#define  USBH_RX600_DPUSR1R_DOVRCRB0                    DEF_BIT_21
#define  USBH_RX600_DPUSR1R_DOVRCRA0                    DEF_BIT_20
#define  USBH_RX600_DPUSR1R_DMINT0                      DEF_BIT_17
#define  USBH_RX600_DPUSR1R_DPINT0                      DEF_BIT_16
#define  USBH_RX600_DPUSR1R_DVBSE1                      DEF_BIT_15
#define  USBH_RX600_DPUSR1R_DOVRCRBE1                   DEF_BIT_13
#define  USBH_RX600_DPUSR1R_DOVRCRAE1                   DEF_BIT_12
#define  USBH_RX600_DPUSR1R_DMINTE1                     DEF_BIT_09
#define  USBH_RX600_DPUSR1R_DPINTE1                     DEF_BIT_08
#define  USBH_RX600_DPUSR1R_DVBSE0                      DEF_BIT_07
#define  USBH_RX600_DPUSR1R_DOVRCRBE0                   DEF_BIT_05
#define  USBH_RX600_DPUSR1R_DOVRCRAE0                   DEF_BIT_04
#define  USBH_RX600_DPUSR1R_DMINTE0                     DEF_BIT_01
#define  USBH_RX600_DPUSR1R_DPINTE0                     DEF_BIT_00


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
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
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* -------------------- DRIVER API -------------------- */
static  void            USBH_RX600_Init            (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_Start           (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_Stop            (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_ERR              *p_err);

static  USBH_DEV_SPD    USBH_RX600_SpdGet          (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_Suspend         (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_Resume          (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_ERR              *p_err);

static  CPU_INT32U      USBH_RX600_FrameNbrGet     (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_EP_Open         (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_EP               *p_ep,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_EP_Close        (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_EP               *p_ep,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_EP_Abort        (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_EP               *p_ep,
                                                    USBH_ERR              *p_err);

static  CPU_BOOLEAN     USBH_RX600_EP_IsHalt       (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_EP               *p_ep,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_URB_Submit      (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_URB              *p_urb,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_URB_Complete    (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_URB              *p_urb,
                                                    USBH_ERR              *p_err);

static  void            USBH_RX600_URB_Abort       (USBH_HC_DRV           *p_hc_drv,
                                                    USBH_URB              *p_urb,
                                                    USBH_ERR              *p_err);

                                                                /* ---------------------- RH API ---------------------- */
static  CPU_BOOLEAN     USBH_RX600_PortStatusGet   (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr,
                                                    USBH_HUB_PORT_STATUS  *p_port_status);

static  CPU_BOOLEAN     USBH_RX600_HubDescGet      (USBH_HC_DRV           *p_hc_drv,
                                                    void                  *p_buf,
                                                    CPU_INT08U             buf_len);

static  CPU_BOOLEAN     USBH_RX600_PortEnSet       (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortEnClr       (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortEnChngClr   (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortPwrSet      (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortPwrClr      (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortResetSet    (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortResetChngClr(USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortSuspendClr  (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_PortConnChngClr (USBH_HC_DRV           *p_hc_drv,
                                                    CPU_INT08U             port_nbr);

static  CPU_BOOLEAN     USBH_RX600_RHSC_IntEn      (USBH_HC_DRV           *p_hc_drv);

static  CPU_BOOLEAN     USBH_RX600_RHSC_IntDis     (USBH_HC_DRV           *p_hc_drv);


/*
*********************************************************************************************************
*                                        LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  CPU_INT08U   USBH_RX600_EP_GetPhyNum (CPU_INT08U    phy_ep_addr);

static  CPU_INT08U   USBH_RX600_EP_GetLogNum (CPU_INT08U    phy_ep_num);

static  CPU_INT08U   USBH_RX600_EP_NextPipe  (USBH_HC_DRV  *p_hc_drv,
                                              USBH_EP      *p_ep);

static  USBH_ERR     USBH_RX600_EP_CfgPipe   (USBH_HC_DRV  *p_hc_drv,
                                              USBH_EP      *p_ep,
                                              CPU_INT08U    pipe_nbr);

static  void         USBH_RX600_EP_SetState  (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    pipe_nbr,
                                              CPU_INT08U    state);

static  void         USBH_RX600_EP_PipeWait  (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    pipe_nbr);

static  void         USBH_RX600_EP_ResetPipe (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    dev_addr,
                                              CPU_INT08U    pipe_nbr);

static  void         USBH_RX600_EP_RemovePipe(USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    dev_addr,
                                              CPU_INT08U    pipe_nbr);

static  CPU_INT32U   USBH_RX600_PipeRd       (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    pipe_nbr,
                                              CPU_INT32U    pkt_buf_len,
                                              USBH_ERR     *p_err);

static  USBH_ERR     USBH_RX600_PipeWr       (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    pipe_nbr,
                                              CPU_INT32U    pkt_buf_len);

static  CPU_INT08U   USBH_RX600_EP_GetState  (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    pipe_nbr);

static  CPU_BOOLEAN  USBH_RX600_PortConn     (USBH_HC_DRV  *p_hc_drv);

        void         USBH_RX600_ISR_Handler  (void         *p_drv);


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

USBH_HC_DRV_API  USBH_RX600_DrvAPI = {
    USBH_RX600_Init,
    USBH_RX600_Start,
    USBH_RX600_Stop,
    USBH_RX600_SpdGet,
    USBH_RX600_Suspend,
    USBH_RX600_Resume,
    USBH_RX600_FrameNbrGet,

    USBH_RX600_EP_Open,
    USBH_RX600_EP_Close,
    USBH_RX600_EP_Abort,
    USBH_RX600_EP_IsHalt,

    USBH_RX600_URB_Submit,
    USBH_RX600_URB_Complete,
    USBH_RX600_URB_Abort
};

USBH_HC_RH_API  USBH_RX600_RH_API = {
    USBH_RX600_PortStatusGet,
    USBH_RX600_HubDescGet,

    USBH_RX600_PortEnSet,
    USBH_RX600_PortEnClr,
    USBH_RX600_PortEnChngClr,

    USBH_RX600_PortPwrSet,
    USBH_RX600_PortPwrClr,

    USBH_RX600_PortResetSet,
    USBH_RX600_PortResetChngClr,

    USBH_RX600_PortSuspendClr,
    USBH_RX600_PortConnChngClr,

    USBH_RX600_RHSC_IntEn,
    USBH_RX600_RHSC_IntDis
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTION
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTION
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          USBH_RX600_Init()
*
* Description : Initialize USB HOST controller.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,      Initialization was succesfull.
*                               USBH_ERR_ALLOC,     Memory heap allocation failed.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_Init (USBH_HC_DRV  *p_hc_drv,
                               USBH_ERR     *p_err)
{
    CPU_SIZE_T       octets_reqd;
    USBH_RX600_DEV  *p_data;
    LIB_ERR          err_lib;


    p_data = (USBH_RX600_DEV  *)Mem_HeapAlloc(sizeof(USBH_RX600_DEV),
                                              sizeof(CPU_ALIGN),
                                             &octets_reqd,
                                             &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
    }

    Mem_Clr(p_data, sizeof(USBH_RX600_DEV));

    p_hc_drv->DataPtr = (void *)p_data;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           USBH_RX600_Start()
*
* Description : Start host controller.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,      Host controller driver succesfully started.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_Start (USBH_HC_DRV  *p_hc_drv,
                                USBH_ERR     *p_err)
{
    CPU_INT08U             cnt;
    CPU_INT08U             ep_cnt;
    CPU_INT08U             dev_cnt;
    CPU_INT08U             lnst1;
    CPU_INT08U             lnst2;
    USBH_RX600_DEV        *p_data;
    USBH_RX600_REG        *p_reg;
    USBH_HC_BSP_API       *p_bsp_api;
    USBH_RX600_PIPE_INFO  *p_pipe_info;


    p_data    = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_reg     = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_bsp_api =  p_hc_drv->BSP_API_Ptr;

    if ((p_bsp_api       != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->Init !=                    0)) {

        p_bsp_api->Init(p_hc_drv, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    DEF_BIT_SET(p_reg->SYSCFG,                                  /* Supply 48-Mhz clock signal to USB module.            */
                USBH_RX600_SYSCFG_DCFM);

    DEF_BIT_SET(p_reg->SYSCFG,                                  /* Supply 48-Mhz clock signal to USB module.            */
                USBH_RX600_SYSCFG_SCKE);

    DEF_BIT_SET(p_reg->SYSCFG,                                  /* Enable D+/D- line pull down.                         */
                USBH_RX600_SYSCFG_DRPD);

                                                                /* Wait for USB line status to stabilize.               */
    lnst1 = (CPU_INT08U)(p_reg->SYSSTS0 & USBH_RX600_SYSSTS0_LNST);
    for (cnt = 0u; cnt < 3u; cnt++) {
        lnst2 = (CPU_INT08U)(p_reg->SYSSTS0 & USBH_RX600_SYSSTS0_LNST);

        if (lnst1 != lnst2) {
            cnt   = 0u;
            lnst1 = lnst2;
        }
    }

    DEF_BIT_SET(p_reg->SYSCFG,                                  /* Enable USB module.                                   */
                USBH_RX600_SYSCFG_USBE);

    p_reg->INTSTS1 = (CPU_INT16U)(~(USBH_RX600_INTSTS1_OVRCR)); /* Clear pending overcurrent int.                       */

    p_reg->INTENB0 = (USBH_RX600_INTENB0_VBSE   |               /* Enable int.                                          */
                      USBH_RX600_INTENB0_RSME   |
                      USBH_RX600_INTENB0_BEMPE  |
                      USBH_RX600_INTENB0_BRDYE);

    p_reg->INTENB1 = (USBH_RX600_INTENB1_OVRCRE |
                      USBH_RX600_INTENB1_BCHGE  |
                      USBH_RX600_INTENB1_ATTCHE |
                      USBH_RX600_INTENB1_SIGNE  |
                      USBH_RX600_INTENB1_SACKE);

    p_reg->BRDYENB =  USBH_RX600_CTRL_PIPE;
    p_reg->BEMPENB =  USBH_RX600_CTRL_PIPE;
    p_reg->NRDYENB =  USBH_RX600_CTRL_PIPE;

                                                                /* Init internal data struct.                           */
    for (dev_cnt = 0u; dev_cnt < USBH_CFG_MAX_NBR_DEVS + 2u; dev_cnt++) {

        p_data->EP_Tbl[dev_cnt][0u] = 0u;
        p_data->EP_Tbl[dev_cnt][1u] = 0u;
        for (ep_cnt = 2u; ep_cnt < 32u; ep_cnt++) {
            p_data->EP_Tbl[dev_cnt][ep_cnt] = 0xFFu;
        }
    }

    for (cnt = 0; cnt < 10u; cnt++) {
        p_pipe_info = &p_data->PipeInfoTbl[cnt];

        p_pipe_info->EP_Nbr     =  0xFFFFu;
        p_pipe_info->MaxPktSize =  0xFFFFu;
        p_pipe_info->EP_Ptr     = (USBH_EP  *)0;
        p_pipe_info->URB_Ptr    = (USBH_URB *)0;

        p_reg->CFIFOSEL  = (USBH_RX600_CFIFOSEL_RCNT |          /* Clear FIFO.                                          */
                            USBH_RX600_DFIFOSEL_MBW  |
                            cnt);
        p_reg->CFIFOCTR |=  USBH_RX600_FIFOCTR_BCLR;
    }

    p_data->EP_Tbl[0u][0u]             = 0u;
    p_data->PipeInfoTbl[0u].EP_Nbr     = 0u;
    p_data->PipeInfoTbl[0u].MaxPktSize = 0x40u;

    if ((p_bsp_api           != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->ISR_Reg  !=                    0)) {

        p_bsp_api->ISR_Reg(USBH_RX600_ISR_Handler, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                          USBH_RX600_Stop()
*
* Description : Stop host controller.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,      Host controller driver succesfully stopped.
*
* Return(s)   : USBH_ERR_NONE
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_Stop (USBH_HC_DRV  *p_hc_drv,
                               USBH_ERR     *p_err)
{
    USBH_RX600_REG   *p_reg;
    USBH_HC_BSP_API  *p_bsp_api;


    p_reg     = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_bsp_api =  p_hc_drv->BSP_API_Ptr;

    p_reg->SYSCFG &= ~(USBH_RX600_SYSCFG_SCKE  |                /* Stop supplying 48-Mhz clock signal to USB module.    */
                       USBH_RX600_SYSCFG_DCFM  |                /* Select USB function ctrlr.                           */
                       USBH_RX600_SYSCFG_DRPD  |                /* Disable pulling down of D+/D- line.                  */
                       USBH_RX600_SYSCFG_DPRPU |                /* Disable pulling up of D+ line.                       */
                       USBH_RX600_SYSCFG_USBE);                 /* Disable USB module.                                  */

    p_reg->INTENB0 = DEF_BIT_NONE;                              /* Disable all ints.                                    */
    p_reg->INTENB1 = DEF_BIT_NONE;
    p_reg->BRDYENB = DEF_BIT_NONE;
    p_reg->BEMPENB = DEF_BIT_NONE;
    p_reg->NRDYENB = DEF_BIT_NONE;

    if ((p_bsp_api            != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->ISR_Unreg !=                    0)) {

        p_bsp_api->ISR_Unreg(p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         USBH_RX600_SpdGet()
*
* Description : Returns host controller's speed.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,      Host controller speed successfully retrieved.
*
* Return(s)   : USBH_DEV_SPD_FULL.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_DEV_SPD  USBH_RX600_SpdGet (USBH_HC_DRV  *p_hc_drv,
                                         USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;

    return (USBH_DEV_SPD_FULL);
}


/*
*********************************************************************************************************
*                                         USBH_RX600_Suspend()
*
* Description : Suspend host controller.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,      Host controller successfully suspended.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_Suspend (USBH_HC_DRV  *p_hc_drv,
                                  USBH_ERR     *p_err)
{
    USBH_RX600_REG  *p_reg;


    p_reg            = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_reg->DVSTCTR0 |=  USBH_RX600_DVSTCTR0_RESUME;             /* Output of USB Resume signal.                         */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         USBH_RX600_Resume()
*
* Description : Resume host controller.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,      Host controller successfully resumed.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_Resume (USBH_HC_DRV  *p_hc_drv,
                                 USBH_ERR     *p_err)
{
    CPU_INT16U       dvstctr0;
    USBH_RX600_REG  *p_reg;


    p_reg     = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    dvstctr0  =  p_reg->DVSTCTR0;

    DEF_BIT_SET(dvstctr0, USBH_RX600_DVSTCTR0_UACT);
    DEF_BIT_CLR(dvstctr0, USBH_RX600_DVSTCTR0_RESUME);

    p_reg->DVSTCTR0 = dvstctr0;                                 /* En USB and end USB Resume signal.                    */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      USBH_RX600_FrameNbrGet()
*
* Description : Returns current frame number.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,      frame number successfully retrieved.
*
* Return(s)   : Current frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_RX600_FrameNbrGet (USBH_HC_DRV  *p_hc_drv,
                                            USBH_ERR     *p_err)
{
    CPU_INT32U       frm_nbr;
    USBH_RX600_REG  *p_reg;


    p_reg   = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    frm_nbr = (p_reg->FRMNUM & USBH_RX600_FRMNUM_FRNM);
   *p_err   =  USBH_ERR_NONE;

    return (frm_nbr);
}


/*
*********************************************************************************************************
*                                        USBH_RX600_EP_Open()
*
* Description : Open an endpoint.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to endpoint structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,              endpoint successfully opened.
*                               USBH_ERR_EP_INVALID_STATE,  invalid endpoint or already in use.
*                               $$$$ List error from RX600_EP_CfgPipe
*
* Return(s)   : None.
*
* Note(s)     : (1) The Root Hub is viewed by the stack as a USB Device but does not need any endpoints
*                   to be physically opened.
*********************************************************************************************************
*/

static  void  USBH_RX600_EP_Open (USBH_HC_DRV  *p_hc_drv,
                                  USBH_EP      *p_ep,
                                  USBH_ERR     *p_err)
{
    CPU_INT08U             dev_addr;
    CPU_INT08U             phy_ep_addr;
    CPU_INT08U             phy_ep_num;
    CPU_INT08U             pipe_nbr;
    USBH_RX600_PIPE_INFO  *p_pipe_info;
    USBH_RX600_DEV        *p_data;


    p_data      = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    dev_addr    =  p_ep->DevAddr;
    phy_ep_addr =  p_ep->Desc.bEndpointAddress;
    phy_ep_num  =  USBH_RX600_EP_GetPhyNum(phy_ep_addr);

    if (USBH_EP_TypeGet(p_ep) != USBH_EP_TYPE_CTRL) {           /* Ctrl EPs opened at init.                             */

        if (p_data->EP_Tbl[dev_addr][phy_ep_num] != 0xFFu) {    /* Validate EP is not in use.                           */
           *p_err = USBH_ERR_EP_INVALID_STATE;
            return;
        }

        pipe_nbr = USBH_RX600_EP_NextPipe(p_hc_drv,             /* Attempt to get unused pipe.                          */
                                          p_ep);
        if (pipe_nbr == 0xFFu) {                                /* Check to see if pipe is valid.                       */
           *p_err = USBH_ERR_EP_INVALID_STATE;
            return;
        }

        p_pipe_info = &p_data->PipeInfoTbl[pipe_nbr];

        p_pipe_info->EP_Nbr     = (phy_ep_num | (dev_addr << 8u));
        p_pipe_info->MaxPktSize =  USBH_EP_MaxPktSizeGet(p_ep);
        p_pipe_info->EP_Ptr     =  p_ep;
        p_data->EP_Tbl[dev_addr][phy_ep_num] =  pipe_nbr;

       *p_err = USBH_RX600_EP_CfgPipe(p_hc_drv,
                                      p_ep,
                                      pipe_nbr);
        if (*p_err != USBH_ERR_NONE) {                          /* Remove pipe on error.                                */
            p_data->EP_Tbl[dev_addr][phy_ep_num] = 0xFFu;

            p_pipe_info->MaxPktSize =  0xFFFFu;
            p_pipe_info->EP_Ptr     = (USBH_EP *)0;

            USBH_RX600_EP_RemovePipe(p_hc_drv,
                                     dev_addr,
                                     pipe_nbr);
            return;
        }
    } else {
        pipe_nbr    =  0u;
        p_pipe_info = &p_data->PipeInfoTbl[pipe_nbr];

        p_data->EP_Tbl[dev_addr][phy_ep_num] =  pipe_nbr;

        p_pipe_info->EP_Nbr     = (phy_ep_num | (dev_addr << 8u));
        p_pipe_info->MaxPktSize =  USBH_EP_MaxPktSizeGet(p_ep);
        p_pipe_info->EP_Ptr     =  p_ep;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBH_RX600_EP_Close()
*
* Description : Close an endpoint.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to endpoint structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,              endpoint successfully closed.
*
* Return(s)   : USBH_ERR_NONE.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_EP_Close (USBH_HC_DRV  *p_hc_drv,
                                   USBH_EP      *p_ep,
                                   USBH_ERR     *p_err)
{
    CPU_INT08U       dev_addr;
    CPU_INT08U       phy_ep_addr;
    CPU_INT08U       phy_ep_num;
    CPU_INT08U       pipe_nbr;
    USBH_RX600_DEV  *p_data;
    CPU_SR_ALLOC();


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    dev_addr    = p_ep->DevAddr;
    phy_ep_addr = p_ep->Desc.bEndpointAddress;
    phy_ep_num  = USBH_RX600_EP_GetPhyNum(phy_ep_addr);
    pipe_nbr    = p_data->EP_Tbl[dev_addr][phy_ep_num];

    CPU_CRITICAL_ENTER();
    USBH_RX600_EP_Abort(p_hc_drv,                               /* Abort any transactions.                              */
                        p_ep,
                        p_err);

    USBH_RX600_EP_RemovePipe(p_hc_drv,                          /* Reset pipe and remove from internal bitmaps.         */
                             dev_addr,
                             pipe_nbr);

    p_data->EP_Tbl[dev_addr][phy_ep_num] = 0xFFu;               /* Remove pipe ref from internal data structs.          */
    CPU_CRITICAL_EXIT();

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                        USBH_RX600_EP_Abort()
*
* Description : Abort pending transactions on endpoint.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to endpoint structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,              endpoint successfully closed.
*
* Return(s)   : USBH_ERR_NONE.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_EP_Abort (USBH_HC_DRV  *p_hc_drv,
                                   USBH_EP      *p_ep,
                                   USBH_ERR     *p_err)
{
    CPU_INT08U       dev_addr;
    CPU_INT08U       phy_ep_addr;
    CPU_INT08U       phy_ep_num;
    CPU_INT08U       pipe_nbr;
    USBH_URB        *p_urb;
    USBH_RX600_DEV  *p_data;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    dev_addr    = p_ep->DevAddr;
    phy_ep_addr = p_ep->Desc.bEndpointAddress;
    phy_ep_num  = USBH_RX600_EP_GetPhyNum(phy_ep_addr);
    pipe_nbr    = p_data->EP_Tbl[dev_addr][phy_ep_num];

    p_urb = p_data->PipeInfoTbl[pipe_nbr].URB_Ptr;
    if (p_urb != (USBH_URB *)0) {
        p_urb->State = USBH_URB_STATE_ABORTED;
    }
   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_RX600_EP_IsHalt()
*
* Description : Retrieve halt status of endpoint.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to endpoint structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,              halt state successfully retrieved.
*
* Return(s)   : DEF_FALSE.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_EP_IsHalt (USBH_HC_DRV  *p_hc_drv,
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
*                                       USBH_RX600_URB_Submit()
*
* Description : Submit a URB.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to URB structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,              URB succesfully submited.
*
* Return(s)   : None.
*
* Note(s)     : (1) Bits HCCHARn[MC] set to 00. Only used for non-periodic transfers in DMA mode and
*                   periodic transfers when using split transactions.
*
*               (2) Host controller feature: software must program the PID field with initial data PID
*                   (to be used on first OUT transaction or to be expected from first IN transaction).
*
*                   (a) For control transactions:
*                       (1) SETUP  stage: initial PID = SETUP
*                       (2) DATA   stage: initial PID = DATA1 (IN or OUT transaction). Then, data toggle
*                                                       must alternate between DATA0 and DATA1.
*                       (3) STATUS stage: initial PID = DATA1 (IN or OUT transaction)
*
*                   (a) For Bulk transaction: first transaction is set to DATA0. Then, data toggle must
*                       alternate between DATA0 and DATA1.
*
*                   (b) For Interrupt transaction: first transaction is set to DATA0. Then, data toggle
*                       must alternate between DATA0 and DATA1.
*
*               (3) Bit HCCHARn[Oddfrm] set to '1'. OTG host perform a transfer in an odd frame. But
*                   channel must be enabled in even frame preceding odd frame.
*********************************************************************************************************
*/

static  void  USBH_RX600_URB_Submit (USBH_HC_DRV  *p_hc_drv,
                                     USBH_URB     *p_urb,
                                     USBH_ERR     *p_err)
{
    USBH_EP         *p_ep;
    CPU_INT16U       phy_ep_addr;
    CPU_INT16U       clr_halt_phy_ep_addr;
    CPU_INT08U       ep_type;
    CPU_INT08U       pipe_nbr;
    CPU_INT16U       ep_max_pkt_size;
    CPU_INT08U       dev_spd;
    CPU_INT08U       ep_dir;
    USBH_ERR         err;
    USBH_SETUP_REQ  *p_setup_req;
    USBH_RX600_DEV  *p_data;
    USBH_RX600_REG  *p_reg;


    p_data      = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_reg       = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_ep        =  p_urb->EP_Ptr;
    phy_ep_addr = (p_ep->DevAddr << 8u) | p_ep->Desc.bEndpointAddress;
    pipe_nbr    =  p_data->EP_Tbl[p_ep->DevAddr][USBH_RX600_EP_GetPhyNum(phy_ep_addr)];

    ep_max_pkt_size = p_data->PipeInfoTbl[pipe_nbr].MaxPktSize;
    ep_dir          = USBH_EP_DirGet(p_ep);
    ep_type         = USBH_EP_TypeGet(p_ep);

    switch (ep_type) {                                          /* Determine EP type.                                   */
        case USBH_EP_TYPE_CTRL:
             switch (p_urb->Token) {                            /* Setup token.                                         */
                 case USBH_TOKEN_SETUP:
                      switch (p_ep->DevSpd) {                   /* Determine dev spd.                                   */

                          case USBH_DEV_SPD_LOW:
                               dev_spd = USBH_RX600_SPD_LS;
                               break;

                          case USBH_DEV_SPD_FULL:
                          case USBH_DEV_SPD_HIGH:
                               dev_spd = USBH_RX600_SPD_FS;
                               break;

                          case USBH_DEV_SPD_NONE:
                          default:
                               dev_spd = USBH_RX600_SPD_NONE;
                               break;
                      }

                      p_reg->DEVADDn[p_ep->DevAddr] = dev_spd;
                      p_reg->DCPMAXP = (ep_max_pkt_size | (p_ep->DevAddr << 12u));

                                                                /* Construct setup packet                               */
                      p_setup_req    = (USBH_SETUP_REQ *)p_urb->UserBufPtr;
                      p_reg->USBREQ  =  MEM_VAL_GET_INT16U_LITTLE(p_setup_req);
                      p_reg->USBVAL  =  MEM_VAL_GET_INT16U_LITTLE(&p_setup_req->wValue);
                      p_reg->USBINDX =  MEM_VAL_GET_INT16U_LITTLE(&p_setup_req->wIndex);
                      p_reg->USBLENG =  MEM_VAL_GET_INT16U_LITTLE(&p_setup_req->wLength);

                                                                /* Clr data toggle after CLR FEATURE HALT EP.           */
                      if ((p_setup_req->bRequest == USBH_REQ_CLR_FEATURE) &&
                          (p_setup_req->wValue   == USBH_FEATURE_SEL_EP_HALT)) {

                           clr_halt_phy_ep_addr = (p_ep->DevAddr << 8u) | (CPU_INT08U)(p_setup_req->wIndex);
                           pipe_nbr             =  p_data->EP_Tbl[p_ep->DevAddr][USBH_RX600_EP_GetPhyNum(clr_halt_phy_ep_addr)];

                           if (pipe_nbr != USBH_RX600_PIPE_00) {
                               p_reg->PIPEnCTR[pipe_nbr - 1u] |= USBH_RX600_PIPECTR_SQCLR;
                           }
                      }

                      p_data->PipeInfoTbl[pipe_nbr].EP_Ptr  = p_ep;
                      p_data->PipeInfoTbl[pipe_nbr].URB_Ptr = p_urb;

                      DEF_BIT_SET(p_reg->DCPCTR,                /* Mark packet as ready to be sent.                     */
                                  USBH_RX600_DCPCTR_SUREQ);
                      break;

                 case USBH_TOKEN_IN:
                      USBH_RX600_EP_SetState(p_hc_drv,          /* Disable pipe.                                        */
                                             pipe_nbr,
                                             USBH_RX600_PID_NAK);

                      DEF_BIT_CLR(p_reg->DCPCFG,                /* Set ctrl pipe xfer dir.                              */
                                  USBH_RX600_DCPCFG_DIR);

                      DEF_BIT_SET(p_reg->DCPCFG,                /* Disable pipe at end of xfer.                         */
                                  USBH_RX600_DCPCFG_SHTNAK);

                      DEF_BIT_SET(p_reg->DCPCTR,                /* Set data toggle for first transaction.               */
                                  USBH_RX600_DCPCTR_SQSET);

                      p_data->PipeInfoTbl[pipe_nbr].EP_Ptr  = p_ep;
                      p_data->PipeInfoTbl[pipe_nbr].URB_Ptr = p_urb;

                      (void)USBH_RX600_PipeRd(p_hc_drv,          /* Perform a ZLP rd.                                    */
                                              pipe_nbr,
                                              0u,
                                             &err);

                      USBH_RX600_EP_SetState(p_hc_drv,          /* Enable pipe to send IN token.                        */
                                             pipe_nbr,
                                             USBH_RX600_PID_BUF);
                      break;

                 case USBH_TOKEN_OUT:
                      USBH_RX600_EP_SetState(p_hc_drv,          /* Disable pipe.                                        */
                                             pipe_nbr,
                                             USBH_RX600_PID_NAK);

                      p_data->PipeInfoTbl[pipe_nbr].EP_Ptr  = p_ep;
                      p_data->PipeInfoTbl[pipe_nbr].URB_Ptr = p_urb;

                      DEF_BIT_SET(p_reg->DCPCFG,                /* Set control pipe xfer dir.                           */
                                  USBH_RX600_DCPCFG_DIR);

                      DEF_BIT_SET(p_reg->DCPCFG,                /* Disable pipe at end of xfer.                         */
                                  USBH_RX600_DCPCFG_SHTNAK);

                      DEF_BIT_SET(p_reg->DCPCTR,                /* Set data toggle for first transaction.               */
                                  USBH_RX600_DCPCTR_SQSET);

                      USBH_RX600_PipeWr(p_hc_drv,
                                        pipe_nbr,
                                        p_urb->UserBufLen);
                      break;
             }
             break;

        case USBH_EP_TYPE_BULK:
        case USBH_EP_TYPE_INTR:

             if (ep_dir == USBH_EP_DIR_OUT) {
                 USBH_RX600_EP_SetState(p_hc_drv,               /* Disable pipe and clear data toggle.                  */
                                        pipe_nbr,
                                        USBH_RX600_PID_NAK);

                 p_data->PipeInfoTbl[pipe_nbr].EP_Ptr  = p_ep;
                 p_data->PipeInfoTbl[pipe_nbr].URB_Ptr = p_urb;

                 USBH_RX600_PipeWr(p_hc_drv,                    /* Perform write request.                               */
                                   pipe_nbr,
                                   p_urb->UserBufLen);

             } else if (ep_dir == USBH_EP_DIR_IN) {
                 USBH_RX600_EP_SetState(p_hc_drv,               /* Disable pipe and clear data toggle.                  */
                                        pipe_nbr,
                                        USBH_RX600_PID_NAK);

                 p_data->PipeInfoTbl[pipe_nbr].EP_Ptr  = p_ep;
                 p_data->PipeInfoTbl[pipe_nbr].URB_Ptr = p_urb;

                 (void)USBH_RX600_PipeRd(p_hc_drv,              /* Send IN token to begin rd request.                   */
                                         pipe_nbr,
                                         0u,
                                        &err);

                 USBH_RX600_EP_SetState(p_hc_drv,               /* Enable pipe.                                         */
                                        pipe_nbr,
                                        USBH_RX600_PID_BUF);
             }
             break;

        case USBH_EP_TYPE_ISOC:                                 /* Currently not implemented.                           */
        default:
             break;
    }

   *p_err = USBH_ERR_NONE;
}

/*
*********************************************************************************************************
*                                      USBH_RX600_URB_Complete()
*
* Description : Complete a URB.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to URB structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,              URB succesfully submited.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_URB_Complete (USBH_HC_DRV  *p_hc_drv,
                                       USBH_URB     *p_urb,
                                       USBH_ERR     *p_err)
{
    USBH_EP         *p_ep;
    CPU_INT08U       pipe_nbr;
    USBH_RX600_DEV  *p_data;


    p_data   = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_ep     =  p_urb->EP_Ptr;
    pipe_nbr =  p_data->EP_Tbl[p_ep->DevAddr][USBH_RX600_EP_GetPhyNum(p_ep->Desc.bEndpointAddress)];

    p_data->PipeInfoTbl[pipe_nbr].URB_Ptr = (USBH_URB *)0;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                       USBH_RX600_URB_Abort()
*
* Description : Abort a URB.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to URB structure.
*
*               p_err           Variable that will receive the return error code from this function.
*                               USBH_ERR_NONE,              URB succesfully submited.
*
* Return(s)   : USBH_ERR_NONE,       if success.
*               Specific error code, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_URB_Abort (USBH_HC_DRV  *p_hc_drv,
                                    USBH_URB     *p_urb,
                                    USBH_ERR     *p_err)
{
    CPU_INT08U       dev_addr;
    CPU_INT16U       phy_ep_addr;
    CPU_INT08U       phy_ep_num;
    CPU_INT08U       pipe_nbr;
    CPU_INT08U       pipe_state;
    USBH_EP         *p_ep;
    USBH_RX600_DEV  *p_data;
    USBH_RX600_REG  *p_reg;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_reg  = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    p_ep        =  p_urb->EP_Ptr;
    dev_addr    =  p_ep->DevAddr;
    phy_ep_addr = (dev_addr << 8u) | p_ep->Desc.bEndpointAddress;
    phy_ep_num  =  USBH_RX600_EP_GetPhyNum(phy_ep_addr);
    pipe_nbr    =  p_data->EP_Tbl[dev_addr][phy_ep_num];
    pipe_state  =  USBH_RX600_EP_GetState(p_hc_drv, pipe_nbr);

    USBH_RX600_EP_SetState(p_hc_drv,
                           pipe_nbr,
                           USBH_RX600_PID_NAK);

    if ((pipe_nbr >= USBH_RX600_PIPE_01) &&
        (pipe_nbr <= USBH_RX600_PIPE_09)) {
                                                                /* Clear FIFO data.                                     */
        p_reg->PIPEnCTR[pipe_nbr - 1u] |=  (USBH_RX600_PIPECTR_ACLRM);
        p_reg->PIPEnCTR[pipe_nbr - 1u] &= ~(USBH_RX600_PIPECTR_ACLRM);
    } else if (pipe_nbr != USBH_RX600_PIPE_00) {
       *p_err = USBH_ERR_EP_NOT_FOUND;
        return;
    }

    p_data->PipeInfoTbl[pipe_nbr].URB_Ptr = (void *)0;

    USBH_RX600_EP_SetState(p_hc_drv,
                           pipe_nbr,
                           pipe_state);

    p_urb->State = USBH_URB_STATE_ABORTED;
    p_urb->Err   = USBH_ERR_URB_ABORT;

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     USBH_RX600_PortStatusGet()
*
* Description : Gets port status change and port status of root hub.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
*               p_port_status   Pointer to port status structure to fill.
*
* return (s)  : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortStatusGet (USBH_HC_DRV           *p_hc_drv,
                                               CPU_INT08U             port_nbr,
                                               USBH_HUB_PORT_STATUS  *p_port_status)
{
    CPU_INT16U       port_status;
    CPU_INT16U       device_state;
    USBH_RX600_DEV  *p_data;
    USBH_RX600_REG  *p_reg;


    (void)port_nbr;                                            /* Only one port avail on RX600.                        */

    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_reg  = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    port_status  = p_data->RH_PortStatus;
    device_state = p_reg->DVSTCTR0;                             /* Read dev state.                                      */

                                                                /* Determine conn spd.                                  */
    if ((device_state & USBH_RX600_DVSTCTR0_RHST) == USBH_RX600_RHST_FS) {
        DEF_BIT_CLR(port_status, DEF_BIT_09);                   /* FS dev attached (PORT_LOW_SPEED)                     */
        DEF_BIT_CLR(port_status, DEF_BIT_10);                   /* FS dev attached (PORT_HIGH_SPEED)                    */
    } else if ((device_state & USBH_RX600_DVSTCTR0_RHST) == USBH_RX600_RHST_LS) {
        DEF_BIT_SET(port_status, DEF_BIT_09);                   /* LS dev attached (PORT_LOW_SPEED)                     */
        DEF_BIT_CLR(port_status, DEF_BIT_10);                   /* LS dev attached (PORT_HIGH_SPEED)                    */
    } else if ((device_state & USBH_RX600_DVSTCTR0_RHST) == USBH_RX600_RHST_ND) {
        DEF_BIT_CLR(port_status, DEF_BIT_09);                   /* FS dev attached (PORT_LOW_SPEED)                     */
        DEF_BIT_CLR(port_status, DEF_BIT_10);                   /* FS dev attached (PORT_HIGH_SPEED)                    */
    }

                                                                /* Chk if dev is outputting reset signal.               */
    if ((device_state & USBH_RX600_DVSTCTR0_RHST) == USBH_RX600_RHST_RST) {
        DEF_BIT_SET(port_status, DEF_BIT_04);
    } else {
        DEF_BIT_CLR(port_status, DEF_BIT_04);
    }

    p_data->RH_PortStatus      = port_status;
    p_port_status->wPortStatus = MEM_VAL_GET_INT16U_LITTLE(&p_data->RH_PortStatus);
    p_port_status->wPortChange = MEM_VAL_GET_INT16U_LITTLE(&p_data->RH_PortStatusChng);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       USBH_RX600_HubDescGet()
*
* Description : Returns root hub descriptor.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Pointer to buffer to fill with hub descriptor.
*
*               p_port_status   Length  of buffer in octets.
*
* Return(s)   : DEF_OK.
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
*                        power-on sequence begins on a port until power is good on that port.
*
*                   (d) 'bHubContrCurrent' is assigned the "maximum current requirements of the Hub
*                        controller electronics in mA."
*
*               (3) This driver supports only ONE downstream port.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_HubDescGet (USBH_HC_DRV  *p_hc_drv,
                                            void         *p_buf,
                                            CPU_INT08U    buf_len)
{
    USBH_HUB_DESC    hub_desc;
    USBH_RX600_DEV  *p_data;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    hub_desc.bDescLength         = USBH_HUB_LEN_HUB_DESC;
    hub_desc.bDescriptorType     = USBH_HUB_DESC_TYPE_HUB;
    hub_desc.bNbrPorts           = 1u;                          /* See Note #3.                                         */
    hub_desc.wHubCharacteristics = 0u;
    hub_desc.bPwrOn2PwrGood      = 100u;
    hub_desc.bHubContrCurrent    = 0u;

    USBH_HUB_FmtHubDesc(&hub_desc,                              /* Write struct in USB fmt.                             */
                         p_data->RH_Desc);

    buf_len = DEF_MIN(buf_len, sizeof(USBH_HUB_DESC));

    Mem_Copy(p_buf,                                             /* Copy formatted struct into buf.                      */
             p_data->RH_Desc,
             buf_len);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       USBH_RX600_PortEnClr()
*
* Description : Set root hub port enable.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortEnSet (USBH_HC_DRV  *p_hc_drv,
                                           CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       USBH_RX600_PortEnClr()
*
* Description : Clear root hub port enable.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortEnClr (USBH_HC_DRV  *p_hc_drv,
                                           CPU_INT08U    port_nbr)
{
    USBH_RX600_DEV  *p_data;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    DEF_BIT_CLR(p_data->RH_PortStatus,
                USBH_HUB_STATUS_PORT_EN);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_RX600_PortEnChngClr()
*
* Description : Clear root hub port enable change.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortEnChngClr (USBH_HC_DRV  *p_hc_drv,
                                               CPU_INT08U    port_nbr)
{
    USBH_RX600_DEV  *p_data;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    DEF_BIT_CLR(p_data->RH_PortStatusChng,
                USBH_HUB_STATUS_PORT_EN);

    return (DEF_TRUE);
}


/*
*********************************************************************************************************
*                                       USBH_RX600_PortPwrSet()
*
* Description : Set root hub port power.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortPwrSet (USBH_HC_DRV  *p_hc_drv,
                                            CPU_INT08U    port_nbr)
{
    USBH_RX600_DEV  *p_data;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    DEF_BIT_SET(p_data->RH_PortStatus,
                USBH_HUB_STATUS_PORT_PWR);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       USBH_RX600_PortPwrClr()
*
* Description : Clear root hub port power.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortPwrClr (USBH_HC_DRV  *p_hc_drv,
                                            CPU_INT08U    port_nbr)
{
    (void)p_hc_drv;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      USBH_RX600_PortResetSet()
*
* Description : Set root hub port reset.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortResetSet (USBH_HC_DRV  *p_hc_drv,
                                              CPU_INT08U    port_nbr)
{
    CPU_INT16U       port_status;
    USBH_RX600_DEV  *p_data;
    USBH_RX600_REG  *p_reg;


    p_data      = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_reg       = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    port_status =  p_data->RH_PortStatus;

                                                                /* Check if port is powered.                            */
    if ((port_status & USBH_HUB_STATUS_PORT_PWR) == USBH_HUB_STATUS_PORT_PWR) {
        DEF_BIT_SET(port_status,
                    USBH_HUB_STATUS_PORT_EN);
        DEF_BIT_SET(port_status,
                    USBH_HUB_STATUS_PORT_RESET);
        DEF_BIT_SET(p_data->RH_PortStatusChng,
                    USBH_HUB_STATUS_PORT_RESET);

        DEF_BIT_SET(p_reg->DVSTCTR0,
                    USBH_RX600_DVSTCTR0_USBRST);
    } else {
        return (DEF_FAIL);                                      /* Port is not powered, ignore operation.               */
    }

    p_data->RH_PortStatus = port_status;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_RX600_PortResetChngClr()
*
* Description : Clear root hub port reset change.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortResetChngClr (USBH_HC_DRV  *p_hc_drv,
                                                  CPU_INT08U    port_nbr)
{
    CPU_INT16U       dvstctr0;
    USBH_RX600_DEV  *p_data;
    USBH_RX600_REG  *p_reg;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_reg  = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_CLR(p_data->RH_PortStatusChng,
                USBH_HUB_STATUS_PORT_RESET);

    dvstctr0 = p_reg->DVSTCTR0;
    DEF_BIT_SET(dvstctr0, USBH_RX600_DVSTCTR0_UACT);
    DEF_BIT_CLR(dvstctr0, USBH_RX600_DVSTCTR0_USBRST);

    p_reg->DVSTCTR0 = dvstctr0;                                 /* En USB simulatanously with USB resume signal end.    */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     USBH_RX600_PortSuspendClr()
*
* Description : Clear root hub port reset change.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_PortSuspendClr (USBH_HC_DRV  *p_hc_drv,
                                                CPU_INT08U    port_nbr)
{
    USBH_RX600_DEV  *p_data;
    USBH_RX600_REG  *p_reg;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    p_reg  = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_CLR(p_reg->DVSTCTR0,                                /* Stop output of the resume signal.                    */
                USBH_RX600_DVSTCTR0_RESUME);

    DEF_BIT_CLR(p_data->RH_PortStatus,
                USBH_HUB_STATUS_PORT_SUSPEND);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    USBH_RX600_PortConnChngClr()
*
* Description : Clear root hub port connection change.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN USBH_RX600_PortConnChngClr (USBH_HC_DRV  *p_hc_drv,
                                                CPU_INT08U    port_nbr)
{
    USBH_RX600_DEV  *p_data;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    DEF_BIT_CLR(p_data->RH_PortStatusChng,
                USBH_HUB_STATUS_PORT_CONN);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       USBH_RX600_RHSC_IntEn()
*
* Description : Enable root hub interrupts.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_RHSC_IntEn (USBH_HC_DRV  *p_hc_drv)
{
    USBH_RX600_REG  *p_reg;


    p_reg = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    DEF_BIT_SET(p_reg->INTENB0, USBH_RX600_INTENB0_VBSE);       /* Enable VBUS int.                                     */

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      USBH_RX600_RHSC_IntDis
*
* Description : Disable root hub interrupts.
*
* Argument(s) : p_hc_drv        Pointer to host controller driver structure.
*
*               port_nbr        Port number.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  USBH_RX600_RHSC_IntDis (USBH_HC_DRV  *p_hc_drv)
{
    USBH_RX600_REG  *p_reg;


    p_reg = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    p_reg->INTENB0 = (CPU_INT16U)~(USBH_RX600_INTENB0_VBSE);    /* Disable VBUS int                                     */

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
*                                        USBH_RX600_PipeRd()
*
* Description : Read data from pipe FIFO.
*
* Arguments   : p_hc_drv            Pointer to host controller driver structure.
*
*               pipe_nbr            Pipe number.
*
*               pkt_buf_len         Buffer length in octets.
*
*               p_err               Variable that will receive the return error code from this function.
*                                   USBH_ERR_NONE,              endpoint read successful.
*
* Return(s)   : Number of octets received.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_RX600_PipeRd (USBH_HC_DRV  *p_hc_drv,
                               CPU_INT08U    pipe_nbr,
                               CPU_INT32U    pkt_buf_len,
                               USBH_ERR     *p_err)
{
    CPU_BOOLEAN      rd_byte;
    CPU_INT08U      *pbuf;
    CPU_INT16U      *pbuf16;
    CPU_INT16U       pkt_len;
    CPU_INT16U       fifo_ctr;
    CPU_INT16U       i;
    USBH_URB        *p_urb;
    USBH_RX600_REG  *p_reg;
    USBH_RX600_DEV  *p_data;
    CPU_SR_ALLOC();


    p_reg   = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_data  = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    i       =  0u;
    rd_byte =  DEF_FALSE;
    p_urb   =  p_data->PipeInfoTbl[pipe_nbr].URB_Ptr;
    pbuf16  = (CPU_INT16U *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen);

    CPU_CRITICAL_ENTER();

    p_reg->CFIFOSEL = (USBH_RX600_CFIFOSEL_RCNT   |             /* Decrement data len on rd.                            */
                       USBH_RX600_CFIFOSEL_MBW    |             /* Port access 16-bit width.                            */
#if (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)
                       USBH_RX600_CFIFOSEL_BIGEND |
#endif
                       pipe_nbr);                               /* Select pipe to be rd from.                           */

    if (pkt_buf_len == 0u) {
        fifo_ctr = p_reg->CFIFOCTR;

        if ((fifo_ctr & USBH_RX600_FIFOCTR_DTLN) == 0u) {
            p_reg->CFIFOCTR |= USBH_RX600_FIFOCTR_BCLR;         /* Clr FIFO for zero pkt rd.                            */
        }
    } else {
        do {
            fifo_ctr = p_reg->CFIFOCTR;                         /* Chk that FIFO is ready to be rd.                     */
        } while ((fifo_ctr & USBH_RX600_FIFOCTR_FRDY) != USBH_RX600_FIFOCTR_FRDY);

        pkt_len = (fifo_ctr & USBH_RX600_FIFOCTR_DTLN);
        pkt_len = DEF_MIN(pkt_len, pkt_buf_len);

        if (pkt_len == 0u) {
            p_reg->CFIFOCTR |= USBH_RX600_FIFOCTR_BCLR;         /* Clear FIFO for zero pkt rd.                          */
        } else {
            if ((pkt_len % 2u) != 0u) {
                pkt_len--;                                      /* Extra pkt has to be rd.                              */
                rd_byte = DEF_TRUE;
            }

            while (i < pkt_len) {
               *pbuf16++ = p_reg->CFIFO.WORD;                   /* Read data from FIFO one word at time.                */
                i += 2u;
            }

            if (rd_byte == DEF_TRUE) {
                pbuf = (CPU_INT08U *)pbuf16;
#if (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)
               *pbuf = p_reg->CFIFO.BYTE.HI;
#else
               *pbuf = p_reg->CFIFO.BYTE.LO;
#endif
                i++;
            }
        }
    }
    p_urb->XferLen += i;                                        /* Update xfer len.                                     */

    CPU_CRITICAL_EXIT();

   *p_err = USBH_ERR_NONE;

    return (i);
}


/*
*********************************************************************************************************
*                                          USBH_RX600_PipeWr()
*
* Description : Write data to endpoint FIFO.
*
* Arguments   : p_hc_drv            Pointer to host controller driver structure.
*
*               pipe_nbr            Pipe number
*
*               pkt_buf_len         Buffer length in octets.
*
* Return(s)   : USBH_ERR_NONE,          if data was written successfully.
*               Specific error code,    otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_RX600_PipeWr (USBH_HC_DRV  *p_hc_drv,
                             CPU_INT08U    pipe_nbr,
                             CPU_INT32U    pkt_buf_len)
{
    CPU_INT08U      *p_buf;
    CPU_INT16U       fifo_ctr;
    CPU_INT16U       max_pkt_len;
    CPU_INT16U      *p_buf16;
    CPU_INT32U       i;
    CPU_BOOLEAN      wr_byte;
    USBH_URB        *p_urb;
    USBH_ERR         err;
    USBH_RX600_DEV  *p_data;
    USBH_RX600_REG  *p_reg;
    CPU_SR_ALLOC();


    p_reg   = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_data  = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    i       =  0u;
    err     =  USBH_ERR_NONE;
    wr_byte =  DEF_FALSE;
    p_urb   =  p_data->PipeInfoTbl[pipe_nbr].URB_Ptr;
    p_buf16 = (CPU_INT16U *)((CPU_INT32U)p_urb->UserBufPtr + p_urb->XferLen);

    CPU_CRITICAL_ENTER();
    USBH_RX600_EP_SetState(p_hc_drv,
                           pipe_nbr,
                           USBH_RX600_PID_NAK);

    p_reg->CFIFOSEL = (USBH_RX600_CFIFOSEL_MBW    |             /* Port access 16-bit width.                            */
                       USBH_RX600_CFIFOSEL_ISEL   |             /* Pipe selected in write mode.                         */
#if (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)
                       USBH_RX600_CFIFOSEL_BIGEND |
#endif
                       pipe_nbr);                               /* Select the pipe to be rd from.                       */

    do {
        fifo_ctr = p_reg->CFIFOCTR;                             /* Check that FIFO is ready to be written.              */
    } while ((fifo_ctr & USBH_RX600_FIFOCTR_FRDY) != USBH_RX600_FIFOCTR_FRDY);

    if (pkt_buf_len == 0u) {                                    /* If zero pkt has to be sent.                          */
        p_reg->CFIFOCTR |= USBH_RX600_FIFOCTR_BVAL;
    } else {
        max_pkt_len = p_data->PipeInfoTbl[pipe_nbr].MaxPktSize;

        if (pkt_buf_len > max_pkt_len) {
            pkt_buf_len = max_pkt_len;
        }

        p_urb->XferLen += pkt_buf_len;                          /* Update xfer len.                                     */

        if ((pkt_buf_len % 2u) != 0u) {
            pkt_buf_len--;
            wr_byte = DEF_TRUE;
        }

        while (i < pkt_buf_len) {
            p_reg->CFIFO.WORD = *p_buf16++;                     /* Write data into FIFO one word at time.               */
            i += 2u;
        }

        if (wr_byte == DEF_TRUE) {
            p_buf = (CPU_INT08U *)p_buf16;
#if (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)
            p_reg->CFIFO.BYTE.HI = *p_buf;                      /* Write one byte of data to FIFO.                      */
#else
            p_reg->CFIFO.BYTE.LO = *p_buf;                      /* Write one byte of data to FIFO.                      */
#endif
            i++;
        }

        if (pkt_buf_len != max_pkt_len) {
            p_reg->CFIFOCTR |= USBH_RX600_FIFOCTR_BVAL;         /* Notify USB ctrlr that short pkt or ZLP is ready.     */
        }
    }

    USBH_RX600_EP_SetState(p_hc_drv,
                           pipe_nbr,
                           USBH_RX600_PID_BUF);

    CPU_CRITICAL_EXIT();

    return (err);
}


/*
*********************************************************************************************************
*                                     USBH_RX600_EP_RemovePipe()
*
* Description : Remove selected pipe.
*
* Arguments   : p_hc_drv            Pointer to host controller driver structure.
*
*               dev_addr            Device's address.
*
*               pipe_nbr            Number of the pipe to remove.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_EP_RemovePipe (USBH_HC_DRV  *p_hc_drv,
                                        CPU_INT08U    dev_addr,
                                        CPU_INT08U    pipe_nbr)
{
    USBH_RX600_DEV        *p_data;
    USBH_RX600_REG        *p_reg;
    USBH_RX600_PIPE_INFO  *p_pipe_info;


    if (pipe_nbr == USBH_RX600_PIPE_00) {
        return;                                                 /* Do not remove ctrl pipe.                             */
    }

    p_reg  = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    USBH_RX600_EP_ResetPipe(p_hc_drv,                           /* Reset pipe to be removed.                            */
                            dev_addr,
                            pipe_nbr);

    switch (pipe_nbr) {                                         /* Mark pipe as unused.                                 */
        case USBH_RX600_PIPE_01:
             p_data->PipeUsed &= ~(USBH_RX600_ISOC_PIPE_01);
             break;

        case USBH_RX600_PIPE_02:
             p_data->PipeUsed &= ~(USBH_RX600_ISOC_PIPE_02);
             break;

        case USBH_RX600_PIPE_03:
             p_data->PipeUsed &= ~(USBH_RX600_BULK_PIPE_01);
             break;

        case USBH_RX600_PIPE_04:
             p_data->PipeUsed &= ~(USBH_RX600_BULK_PIPE_02);
             break;

        case USBH_RX600_PIPE_05:
             p_data->PipeUsed &= ~(USBH_RX600_BULK_PIPE_03);
             break;

        case USBH_RX600_PIPE_06:
             p_data->PipeUsed &= ~(USBH_RX600_INT_PIPE_01);
             break;

        case USBH_RX600_PIPE_07:
             p_data->PipeUsed &= ~(USBH_RX600_INT_PIPE_02);
             break;

        case USBH_RX600_PIPE_08:
             p_data->PipeUsed &= ~(USBH_RX600_INT_PIPE_03);
             break;

        case USBH_RX600_PIPE_09:
             p_data->PipeUsed &= ~(USBH_RX600_INT_PIPE_04);
             break;
    }
                                                                /* Set pipe to unused.                                  */
    p_pipe_info = &p_data->PipeInfoTbl[pipe_nbr];
    p_pipe_info->EP_Nbr     =  0xFFFFu;
    p_pipe_info->MaxPktSize =  0xFFFFu;
    p_pipe_info->EP_Ptr     = (void *)0;
                                                                /* Disable int for pipe.                                */
    p_reg->BRDYENB &= (CPU_INT16U)~(1u << pipe_nbr);
    p_reg->BEMPENB &= (CPU_INT16U)~(1u << pipe_nbr);
    p_reg->NRDYENB &= (CPU_INT16U)~(1u << pipe_nbr);
}


/*
*********************************************************************************************************
*                                      USBH_RX600_EP_ResetPipe()
*
* Description : Reset selected pipe.
*
* Arguments   : p_hc_drv            Pointer to host controller driver structure.
*
*               dev_addr            Device's address.
*
*               pipe_nbr            Number of the pipe to reset.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_EP_ResetPipe (USBH_HC_DRV  *p_hc_drv,
                                       CPU_INT08U    dev_addr,
                                       CPU_INT08U    pipe_nbr)
{
    USBH_RX600_REG  *p_reg;
    CPU_SR_ALLOC();


    if (pipe_nbr == USBH_RX600_PIPE_00) {
        return;                                                 /* Do not reset ctrl pipe.                              */
    }

    p_reg  = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    CPU_CRITICAL_ENTER();

    USBH_RX600_EP_SetState(p_hc_drv,
                           pipe_nbr,
                           USBH_RX600_PID_NAK);

    p_reg->PIPESEL = pipe_nbr;                                  /* Select pipe to be reset.                             */

    if ((pipe_nbr >= USBH_RX600_PIPE_01) &&
        (pipe_nbr <= USBH_RX600_PIPE_09)) {
                                                                /* Set PID bit of corresponding pipe to NAK.            */
        if ((p_reg->PIPEnCTR[pipe_nbr - 1u] & USBH_RX600_PIPECTR_PID) > 0u) {
            p_reg->PIPEnCTR[pipe_nbr - 1u] &= ~(USBH_RX600_PIPECTR_PID);
        }
        p_reg->PIPEnCTR[pipe_nbr - 1u] = DEF_CLR;
    }

    p_reg->PIPECFG  = DEF_CLR;                                  /* Set reg to dflt.                                     */
    p_reg->PIPEMAXP = DEF_CLR;
    p_reg->PIPEPERI = DEF_CLR;

    p_reg->PIPESEL  = USBH_RX600_PIPE_00;                       /* Set pipe select to no selected pipe.                 */
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                      USBH_RX600_EP_NextPipe()
*
* Description : Find next available pipe.
*
* Arguments   : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to endpoint.
*
* Return(s)   : Pipe number,    if a pipe is available.
*               0xFF,           otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  USBH_RX600_EP_NextPipe (USBH_HC_DRV  *p_hc_drv,
                                            USBH_EP      *p_ep)
{
    USBH_RX600_DEV  *p_data;


    p_data = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

    switch (USBH_EP_TypeGet(p_ep)) {
        case USBH_EP_TYPE_CTRL:
             return (USBH_RX600_PIPE_00);                       /* Ctrl pipe is always pipe 0.                          */
             break;

        case USBH_EP_TYPE_BULK:
             if ((p_data->PipeUsed & USBH_RX600_BULK_PIPE_ALL) != USBH_RX600_BULK_PIPE_ALL) {

                if ((p_data->PipeUsed & USBH_RX600_BULK_PIPE_01) == 0u) {
                    p_data->PipeUsed |= USBH_RX600_BULK_PIPE_01;
                    return (USBH_RX600_PIPE_03);
                } else if ((p_data->PipeUsed & USBH_RX600_BULK_PIPE_02) == 0u) {
                    p_data->PipeUsed |= USBH_RX600_BULK_PIPE_02;
                    return (USBH_RX600_PIPE_04);
                } else {
                    p_data->PipeUsed |= USBH_RX600_BULK_PIPE_03;
                    return (USBH_RX600_PIPE_05);
                }
             }
        case USBH_EP_TYPE_ISOC:                                 /* Isoc pipes may be used as Bulk pipe if needed.       */
             if ((p_data->PipeUsed & USBH_RX600_ISOC_PIPE_ALL) != USBH_RX600_ISOC_PIPE_ALL) {

                if ((p_data->PipeUsed & USBH_RX600_ISOC_PIPE_01) == 0u) {
                    p_data->PipeUsed |= USBH_RX600_ISOC_PIPE_01;
                    return (USBH_RX600_PIPE_01);
                } else {
                    p_data->PipeUsed |= USBH_RX600_ISOC_PIPE_02;

                    return (USBH_RX600_PIPE_02);
                }
             }
             break;

        case USBH_EP_TYPE_INTR:
             if ((p_data->PipeUsed & USBH_RX600_INT_PIPE_ALL) != USBH_RX600_INT_PIPE_ALL) {

                if ((p_data->PipeUsed & USBH_RX600_INT_PIPE_01) == 0u) {
                    p_data->PipeUsed |= USBH_RX600_INT_PIPE_01;
                    return (USBH_RX600_PIPE_06);
                } else if ((p_data->PipeUsed & USBH_RX600_INT_PIPE_02) == 0u) {
                    p_data->PipeUsed |= USBH_RX600_INT_PIPE_02;
                    return (USBH_RX600_PIPE_07);
                } else if ((p_data->PipeUsed & USBH_RX600_INT_PIPE_03) == 0u) {
                    p_data->PipeUsed |= USBH_RX600_INT_PIPE_03;
                    return (USBH_RX600_PIPE_08);
                } else {
                    p_data->PipeUsed |= USBH_RX600_INT_PIPE_04;
                    return (USBH_RX600_PIPE_09);
                }
             }
             break;

        default:                                                /* Unknown pipe type specified.                         */
             break;
    }

    return (0xFFu);
}


/*
*********************************************************************************************************
*                                       USBH_RX600_EP_CfgPipe()
*
* Description : Find next available pipe of specified type.
*
* Arguments   : p_hc_drv        Pointer to host controller driver structure.
*
*               p_ep            Pointer to the endpoint structure.
*
*               pipe_nbr        The number of the pipe to be configured.
*
* Return(s)   : Pipe number,    if a pipe of specified type is available.
*               0xFF,           otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_RX600_EP_CfgPipe (USBH_HC_DRV  *p_hc_drv,
                                         USBH_EP      *p_ep,
                                         CPU_INT08U    pipe_nbr)
{
    CPU_INT08U       i;
    CPU_INT16U       phy_ep_addr;
    USBH_RX600_REG  *p_reg;
    USBH_RX600_DEV  *p_data;
    CPU_SR_ALLOC();


    if (pipe_nbr == USBH_RX600_PIPE_00) {                       /* Ctrl pipe does not require config.                   */
        return (USBH_ERR_NONE);
    }

    p_reg        = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_data       = (USBH_RX600_DEV *)p_hc_drv->DataPtr;
    phy_ep_addr  =  p_ep->DevAddr << 8u;
    phy_ep_addr |=  p_ep->Desc.bEndpointAddress;

    CPU_CRITICAL_ENTER();

    USBH_RX600_EP_SetState(p_hc_drv,
                           pipe_nbr,
                           USBH_RX600_PID_NAK);

    p_reg->PIPESEL = pipe_nbr;                                  /* Select pipe nbr to apply settings to.                */

    switch (pipe_nbr) {                                         /* Cfg pipe settings depending on pipe type.            */
        case USBH_RX600_PIPE_01:
        case USBH_RX600_PIPE_02:
        case USBH_RX600_PIPE_03:
        case USBH_RX600_PIPE_04:
        case USBH_RX600_PIPE_05:
             if (USBH_EP_TypeGet(p_ep) == USBH_EP_TYPE_ISOC) {
                 p_reg->PIPECFG = USBH_RX600_TYPE_ISOC;         /* Set pipe as Isoc.                                    */
             } else {
                 p_reg->PIPECFG = USBH_RX600_TYPE_BULK;         /* Set pipe as Bulk.                                    */
             }
             break;

        case USBH_RX600_PIPE_06:
        case USBH_RX600_PIPE_07:
        case USBH_RX600_PIPE_08:
        case USBH_RX600_PIPE_09:
             p_reg->PIPECFG  = USBH_RX600_TYPE_INT;             /* Set pipe as Intr.                                    */

             for (i = 0u; i < 8u; i++) {                        /* Calculate interval value as a power of 2.            */
                 if (p_ep->Interval < (1u << i)) {
                    if (i == 0) {
                        p_reg->PIPEPERI = i;
                    } else {
                        p_reg->PIPEPERI = i - 1u;
                    }
                    break;
                 }
             }
             break;

        default:
             CPU_CRITICAL_EXIT();
             return (USBH_ERR_EP_NOT_FOUND);
    }

                                                                /* Cfg pipe settings depending on pipe type.            */
    p_reg->PIPEnCTR[pipe_nbr - 1u] |= USBH_RX600_PIPECTR_SQCLR;

    p_reg->PIPECFG &= ~(USBH_RX600_PIPECFG_DIR);                /* Set pipe dir.                                        */
    if ((phy_ep_addr & USBH_REQ_DIR_MASK) == USBH_EP_DIR_OUT) {
        p_reg->PIPECFG |= USBH_RX600_PIPECFG_DIR;
    } else {
        p_reg->PIPECFG |= USBH_RX600_PIPECFG_SHTNAK;            /* Disable pipe on end of xfer for IN EP.               */
    }
                                                                /* Set EP nbr.                                          */
    p_reg->PIPECFG |= (phy_ep_addr & USBH_RX600_PIPECFG_EPNUM);

    if ((phy_ep_addr & USBH_REQ_DIR_MASK) == USBH_EP_DIR_IN) {
        p_reg->BRDYENB |= (CPU_INT16U)(1u << pipe_nbr);         /* Enable recv ints.                                    */
    } else {
        p_reg->PIPECFG |= (USBH_RX600_PIPECFG_BFRE);            /* Do not generate BRDY int on xfer completion.         */
        p_reg->BEMPENB |= (CPU_INT16U)(1u << pipe_nbr);         /* Enable transmit int.                                 */
    }
    p_reg->NRDYENB |= (CPU_INT16U)(1u << pipe_nbr);

    p_reg->PIPEMAXP = ((p_ep->DevAddr << 12u) | p_data->PipeInfoTbl[pipe_nbr].MaxPktSize);

    p_reg->PIPESEL = USBH_RX600_PIPE_00;                        /* Select no pipe.                                      */

    CPU_CRITICAL_EXIT();

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                      USBH_RX600_EP_PipeWait()
*
* Description : Wait for selected endpoint to become ready.
*
* Arguments   : p_hc_drv      Pointer to host controller driver structure.
*
*               pipe_nbr      Pipe number.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  USBH_RX600_EP_PipeWait (USBH_HC_DRV  *p_hc_drv,
                                      CPU_INT08U    pipe_nbr)
{
    CPU_INT16U       pipe_ctr;
    USBH_RX600_REG  *p_reg;


    p_reg = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    if (pipe_nbr == USBH_RX600_PIPE_00) {                       /* Wait for pipe to become ready.                       */
        do {
            pipe_ctr = p_reg->DCPCTR;
        } while ((pipe_ctr & USBH_RX600_DCPCTR_PBUSY) == USBH_RX600_DCPCTR_PBUSY);
    } else {
        do {
            pipe_ctr = p_reg->PIPEnCTR[pipe_nbr - 1u];
        } while ((pipe_ctr & USBH_RX600_PIPECTR_PBUSY) == USBH_RX600_PIPECTR_PBUSY);
    }
}


/*
*********************************************************************************************************
*                                      USBH_RX600_EP_SetState()
*
* Description : Set state of selected endpoint.
*
* Arguments   : p_hc_drv        Pointer to host controller driver structure.
*
*               pipe_nbr        Pipe number.
*
*               state           Endpoint state.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_RX600_EP_SetState (USBH_HC_DRV  *p_hc_drv,
                                      CPU_INT08U    pipe_nbr,
                                      CPU_INT08U    state)
{
    CPU_INT16U       pipe_ctr;
    USBH_RX600_REG  *p_reg;


    p_reg = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    if (pipe_nbr == USBH_RX600_PIPE_00) {                       /* Set pipe to new state.                               */
        pipe_ctr = p_reg->DCPCTR;

        if ((pipe_ctr & USBH_RX600_DCPCTR_PID) != state) {
            pipe_ctr      &= ~(USBH_RX600_DCPCTR_PID);
            pipe_ctr      |=   state;
            p_reg->DCPCTR  =   pipe_ctr;
        }
    } else {
        pipe_ctr = p_reg->PIPEnCTR[pipe_nbr - 1u];

        if ((pipe_ctr & USBH_RX600_PIPECTR_PID) != state) {
            pipe_ctr                      &= ~(USBH_RX600_PIPECTR_PID);
            pipe_ctr                      |=   state;
            p_reg->PIPEnCTR[pipe_nbr- 1u]  =   pipe_ctr;
        }
    }

    if (state == USBH_RX600_PID_NAK) {
        USBH_RX600_EP_PipeWait(p_hc_drv, pipe_nbr);
    }
}


/*
*********************************************************************************************************
*                                      USBH_RX600_EP_GetState()
*
* Description : Get state of selected endpoint.
*
* Arguments   : p_hc_drv     Pointer to host controller driver structure.
*
*               pipe_nbr     Pipe number.
*
* Return(s)   : Endpoint's state.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT08U  USBH_RX600_EP_GetState (USBH_HC_DRV  *p_hc_drv,
                                            CPU_INT08U    pipe_nbr)
{
    CPU_INT08U       pipe_state;
    USBH_RX600_REG  *p_reg;


    p_reg = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;

    if (pipe_nbr == USBH_RX600_PIPE_00) {                       /* Get state of desired pipe.                           */
        pipe_state = (p_reg->DCPCTR & USBH_RX600_DCPCTR_PID);
    } else {
        pipe_state = (p_reg->PIPEnCTR[pipe_nbr - 1u] & USBH_RX600_PIPECTR_PID);
    }

    return (pipe_state);
}


/*
*********************************************************************************************************
*                                      USBH_RX600_EP_GetPhyNum()
*
* Description : Get physical endpoint number.
*
* Arguments   : phy_ep_addr         Endpoint address.
*
* Return(s)   : Physical endpoint number,   if phy_ep_addr exists.
*               0xFF,                       otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT08U  USBH_RX600_EP_GetPhyNum (CPU_INT08U  phy_ep_addr)
{
    if ( (phy_ep_addr <= 0x0Fu) ||                              /* Chk EP addr.                                         */
        ((phy_ep_addr >= 0x80u) && (phy_ep_addr <= 0x8Fu))) {
        if ((phy_ep_addr & USBH_REQ_DIR_MASK) == USBH_EP_DIR_IN) {
            return (((phy_ep_addr & 0x0Fu) * 2u) + 1u);
        } else {
            return ((phy_ep_addr & 0x0Fu) * 2u);
        }
    }

    return (0xFFu);
}


/*
*********************************************************************************************************
*                                      USBH_RX600_EP_GetLogNum()
*
* Description : Get logical endpoint number.
*
* Arguments   : phy_ep_num      Physical endpoint number.
*
* Return(s)   : Logical endpoint number,    if phy_ep_num valid.
*               0xFF,                       otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_INT08U  USBH_RX600_EP_GetLogNum (CPU_INT08U  phy_ep_num)
{
    if (phy_ep_num <= 31u) {                                    /* Chk EP nbr.                                          */
        if (phy_ep_num % 2u) {
            return ((phy_ep_num / 2u) | 0x80u);
        } else {
            return (phy_ep_num / 2u);
        }
    }

    return (0xFFu);
}


/*
*********************************************************************************************************
*                                         USBH_RX600_PortConn()
*
* Description : Check port connection status.
*
* Argument(s) : None.
*
* Return(s)   : DEF_TRUE,       if device is connected on port.
*               DEF_FALSE,      otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  USBH_RX600_PortConn (USBH_HC_DRV  *p_hc_drv)
{
    CPU_INT16U       syssts0_reg;
    USBH_RX600_REG  *p_reg;


    p_reg       = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    syssts0_reg =  p_reg->SYSSTS0;

    if ((syssts0_reg & USBH_RX600_SYSSTS0_LNST) != 0u) {        /* If dev connected, data lines different from SE0 state*/
        return (DEF_TRUE);
    }

    return (DEF_FALSE);
}


/*
*********************************************************************************************************
*                                      USBH_RX600_ISR_Handler()
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

void  USBH_RX600_ISR_Handler (void  *p_drv)
{
    CPU_INT08U             i;
    CPU_INT08U             lnst1;
    CPU_INT08U             lnst2;
    CPU_INT16U             phy_ep_addr;
    CPU_INT16U             tmp;
    CPU_INT16U             int_reg0;
    CPU_INT16U             int_reg1;
    CPU_INT16U             brdy_reg;
    CPU_INT16U             bemp_reg;
    CPU_INT16U             nrdy_reg;
    CPU_INT32U             xfer_len;
    USBH_URB              *p_urb;
    USBH_EP               *p_ep;
    USBH_HC_DRV           *p_hc_drv;
    USBH_RX600_DEV        *p_data;
    USBH_RX600_REG        *p_reg;
    USBH_RX600_PIPE_INFO  *p_pipe_info;


    p_hc_drv = (USBH_HC_DRV    *)p_drv;
    p_reg    = (USBH_RX600_REG *)p_hc_drv->HC_CfgPtr->BaseAddr;
    p_data   = (USBH_RX600_DEV *)p_hc_drv->DataPtr;

                                                                /* ---------------- READ EACH INT REG ----------------- */
    int_reg0 = p_reg->INTSTS0;
    int_reg1 = p_reg->INTSTS1;
    brdy_reg = p_reg->BRDYSTS;
    bemp_reg = p_reg->BEMPSTS;
    nrdy_reg = p_reg->NRDYSTS;

    int_reg0 &= p_reg->INTENB0;
    int_reg1 &= p_reg->INTENB1;
    brdy_reg &= p_reg->BRDYENB;
    bemp_reg &= p_reg->BEMPENB;
    nrdy_reg &= p_reg->NRDYENB;

    p_reg->INTSTS0 = ~(int_reg0);
    p_reg->INTSTS1 = ~(int_reg1);
    p_reg->BRDYSTS = ~(brdy_reg);
    p_reg->BEMPSTS = ~(bemp_reg);
    p_reg->NRDYSTS = ~(nrdy_reg);

    if (int_reg0 & USBH_RX600_INTSTS0_VBINT) {                  /* VBUS int.                                            */
        if (int_reg0 & USBH_RX600_INTSTS0_VBSTS) {              /* VBUS pin state, high level.                          */
            DEF_BIT_SET(p_data->RH_PortStatus,
                        USBH_HUB_STATUS_PORT_PWR);
        } else {                                                /* VBUS pin state, low level.                           */
            DEF_BIT_CLR(p_data->RH_PortStatus,
                        USBH_HUB_STATUS_PORT_PWR);
        }
    }

                                                                /* ------------------- RESUME EVENT ------------------- */
    if (int_reg0 & USBH_RX600_INTSTS0_RESM) {
        p_reg->INTSTS0 &= ~(USBH_RX600_INTSTS0_RESM);
    }

                                                                /* ------------------ START OF FRAME ------------------ */
    if (int_reg0 & USBH_RX600_INTSTS0_SOFR) {
        p_reg->INTSTS0 &= ~(USBH_RX600_INTSTS0_SOFR);
    }

                                                                /* ----------- DEVICE STATE CHANGE DETECTED ----------- */
    if (int_reg0 & USBH_RX600_INTSTS0_DVST) {
        p_reg->INTSTS0 &= ~(USBH_RX600_INTSTS0_DVST);
    }

                                                                /* -------------- SETUP PACKET RECEIVED --------------- */
    if (int_reg0 & USBH_RX600_INTSTS0_VALID) {
        p_reg->INTSTS0 &= ~(USBH_RX600_INTSTS0_VALID);
    }

                                                                /* ---------- CONTROL TRANSFER STATUS CHANGE ---------- */
    if (int_reg0 & USBH_RX600_INTSTS0_CTRT) {
        p_reg->INTSTS0 &= ~(USBH_RX600_INTSTS0_CTRT);
    }

                                                                /* ------------------- OVERCURRENT -------------------- */
    if (int_reg1 & USBH_RX600_INTSTS1_OVRCR) {
        DEF_BIT_SET(p_data->RH_PortStatus, USBH_HUB_STATUS_PORT_OVER_CUR);
        p_reg->INTSTS1 = (CPU_REG16)(~(USBH_RX600_INTSTS1_OVRCR));
    }

                                                                /* -------------------- BUS CHANGE -------------------- */
    if (int_reg1 & USBH_RX600_INTSTS1_BCHG) {
        p_reg->INTSTS1 = (CPU_INT16U)~(USBH_RX600_INTSTS1_BCHG);

                                                                /* Check line status, wait until it stabilizes          */
        lnst1 = (CPU_INT08U)(p_reg->SYSSTS0 & USBH_RX600_SYSSTS0_LNST);

        for (i = 0u; i < 3u; i ++) {
            lnst2 = (CPU_INT08U)(p_reg->SYSSTS0 & USBH_RX600_SYSSTS0_LNST);

            if (lnst1 != lnst2) {
                i = 0u;
                lnst1 = lnst2;
            }
        }
    }

                                                                /* ---------------------- ATTACH ---------------------- */
    if (int_reg1 & USBH_RX600_INTSTS1_ATTCH) {

        if (USBH_RX600_PortConn(p_hc_drv) == DEF_TRUE) {        /* Check port to ensure connection occured.             */
                                                                /* Update port status and port status chng.             */
            DEF_BIT_SET(p_data->RH_PortStatusChng, USBH_HUB_STATUS_PORT_CONN);
            DEF_BIT_SET(p_data->RH_PortStatus, USBH_HUB_STATUS_PORT_CONN);

            USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);

            p_reg->INTENB1 &= ~(USBH_RX600_INTENB1_ATTCHE);
            p_reg->INTENB1 |=   USBH_RX600_INTENB1_DTCHE;
        }

        p_reg->INTSTS1 = (CPU_INT16U)~(USBH_RX600_INTSTS1_ATTCH);

        DEF_BIT_CLR(p_reg->INTENB1, USBH_RX600_INTENB1_BCHGE);
    }

                                                                /* ---------------------- DETACH ---------------------- */
    if (int_reg1 & USBH_RX600_INTSTS1_DTCH) {

        if (USBH_RX600_PortConn(p_hc_drv) == DEF_FALSE) {       /* Check port to ensure disconnection occured.          */
                                                                /* Update port status and port status chng.             */
            DEF_BIT_SET(p_data->RH_PortStatusChng, USBH_HUB_STATUS_PORT_CONN);
            DEF_BIT_CLR(p_data->RH_PortStatus, USBH_HUB_STATUS_PORT_CONN);

            USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);

            p_reg->INTENB1 &= ~(USBH_RX600_INTENB1_DTCHE);
            p_reg->INTENB1 |=   USBH_RX600_INTENB1_ATTCHE;

        }

        p_reg->INTSTS1 = (CPU_INT16U)~(USBH_RX600_INTENB1_DTCHE);

        DEF_BIT_SET(p_reg->INTENB1, USBH_RX600_INTENB1_BCHGE);
    }

                                                                /* -------------------- EOF ERROR --------------------- */
    if (int_reg1 & USBH_RX600_INTSTS1_EOFERR) {
        p_reg->INTSTS1 = (CPU_INT16U)~(USBH_RX600_INTSTS1_EOFERR);
    }

                                                                /* ---------------- SETUP PACKET ERROR ---------------- */
    if (int_reg1 & USBH_RX600_INTSTS1_SIGN) {
        p_reg->INTSTS1 = (CPU_INT16U)~(USBH_RX600_INTSTS1_SIGN);
        p_urb          =  p_data->PipeInfoTbl[USBH_RX600_PIPE_00].URB_Ptr;
        p_urb->XferLen =  0u;
        USBH_URB_Done(p_urb);                                   /* Notify stack of xfer.                                */
    }

                                                                /* ------------ SETUP PACKET XFER COMPLETE ------------ */
    if (int_reg1 & USBH_RX600_INTSTS1_SACK) {
        p_reg->INTSTS1 = (CPU_INT16U)~(USBH_RX600_INTSTS1_SACK);
        p_urb          =  p_data->PipeInfoTbl[USBH_RX600_PIPE_00].URB_Ptr;
        p_urb->XferLen =  USBH_LEN_SETUP_PKT;                   /* Update packet len.                                   */
        USBH_URB_Done(p_urb);                                   /* Notify stack of xfer.                                */
    }

                                                                /* --------------------- RECV INT --------------------- */
    if (brdy_reg != 0u) {

        for (i = 0u; brdy_reg != 0u; i++) {                     /* Determine which pipe(s) require attention.           */

            if (DEF_BIT_IS_SET(brdy_reg, DEF_BIT_00) == DEF_TRUE) {
                p_pipe_info = &p_data->PipeInfoTbl[i];

                if (i == 0) {                                   /* Pipe zero is always ctrl pipe.                       */
                    phy_ep_addr = 0x80u;
                } else {
                    tmp          = p_pipe_info->EP_Nbr;
                    phy_ep_addr  = USBH_RX600_EP_GetLogNum((CPU_INT08U)(tmp & 0xFFu));
                    phy_ep_addr |= (tmp & 0xFF00u);
                }

                USBH_RX600_EP_SetState(p_hc_drv,
                                       i,
                                       USBH_RX600_PID_NAK);

                                                                /* If pipe is a recv pipe...                            */
                if ((phy_ep_addr & USBH_REQ_DIR_MASK) == USBH_EP_DIR_IN) {
                    p_ep = p_pipe_info->EP_Ptr;                 /* Get EP ptr for this pipe.                            */

                    if (p_ep != (USBH_EP *)0) {
                        p_urb = p_pipe_info->URB_Ptr;

                        if (p_urb != (USBH_URB *)0) {
                            xfer_len = USBH_RX600_PipeRd(             p_hc_drv,
                                                                      i,
                                                                     (p_urb->UserBufLen - p_urb->XferLen),
                                                         (USBH_ERR *)&p_urb->Err);

                                                                /* Chk if transfer is complete...                       */
                            if ((p_urb->XferLen >= p_urb->UserBufLen) ||
                                (xfer_len       <  p_pipe_info->MaxPktSize)) {

                                USBH_URB_Done(p_urb);           /* Notify stack.                                        */

                                if (USBH_EP_TypeGet(p_ep) != USBH_EP_TYPE_INTR) {
                                    p_pipe_info->EP_Ptr = (void *)0;
                                }
                            } else if (p_urb->XferLen < p_urb->UserBufLen) {
                                                                /* Enable pipe to continue receiving.                   */
                                USBH_RX600_EP_SetState(p_hc_drv,
                                                       i,
                                                       USBH_RX600_PID_BUF);
                            }
                        }
                    }
                }
            }

            brdy_reg >>= 1u;
        }
    }

                                                                /* --------------------- ERR INT ---------------------- */
    if (nrdy_reg != 0u) {

        for (i = 0u; nrdy_reg != 0u; i++) {                     /* Determine which pipe(s) require attention.           */

            if (DEF_BIT_IS_SET(nrdy_reg, DEF_BIT_00) == DEF_TRUE) {
                p_pipe_info = &p_data->PipeInfoTbl[i];

                if (i == 0) {                                   /* Pipe zero is always ctrl pipe.                       */
                    phy_ep_addr = 0x80u;
                } else {
                    tmp          = p_pipe_info->EP_Nbr;
                    phy_ep_addr  = USBH_RX600_EP_GetLogNum((CPU_INT08U)(tmp & 0xFFu));
                    phy_ep_addr |= (tmp & 0xFF00u);
                }

                USBH_RX600_EP_SetState(p_hc_drv,
                                       i,
                                       USBH_RX600_PID_NAK);

                                                                /* If pipe is a recv pipe...                            */
                if ((phy_ep_addr & USBH_REQ_DIR_MASK) == USBH_EP_DIR_IN) {
                    p_ep = p_pipe_info->EP_Ptr;                 /* Get EP ptr for this pipe.                            */

                    if (p_ep != (USBH_EP *)0) {
                        p_urb = p_pipe_info->URB_Ptr;

                        if (p_urb != (USBH_URB *)0) {
                            p_urb->Err = USBH_ERR_DEV_NOT_RESPONDING;
                            USBH_URB_Done(p_urb);               /* Notify stack.                                        */
                        }
                    }
                }
            }

            nrdy_reg >>= 1u;
        }
    }

                                                                /* --------------------- XFER INT --------------------- */
    if (bemp_reg != 0u) {

        for (i = 0u; bemp_reg != 0u; i++) {                     /* Determine which pipe(s) require attention.           */

            if (DEF_BIT_IS_SET(bemp_reg, DEF_BIT_00) == DEF_TRUE) {
                p_pipe_info = &p_data->PipeInfoTbl[i];

                if (i == 0u) {                                  /* Pipe zero is always ctrl pipe.                       */
                    phy_ep_addr = 0x00u;
                } else {
                    tmp          = p_pipe_info->EP_Nbr;
                    phy_ep_addr  = USBH_RX600_EP_GetLogNum((CPU_INT08U)(tmp & 0xFFu));
                    phy_ep_addr |= (tmp & 0xFF00u);
                }

                                                                /* Chk if pipe is a transfer pipe...                    */
                if ((phy_ep_addr & USBH_REQ_DIR_MASK) == USBH_EP_DIR_OUT) {
                    p_ep = p_pipe_info->EP_Ptr;

                    if (p_ep != (void *)0) {
                        p_urb = p_pipe_info->URB_Ptr;

                                                                /* Chk if transaction is complete...                    */
                        if (p_urb->XferLen >= p_urb->UserBufLen) {
                            USBH_URB_Done(p_urb);               /* Notify stack.                                        */

                            if (USBH_EP_TypeGet(p_ep) != USBH_EP_TYPE_INTR) {
                                p_pipe_info->EP_Ptr = (void *)0;
                            }
                        } else {
                            USBH_RX600_PipeWr(p_hc_drv,         /* Continue transmitting.                               */
                                              i,
                                             (p_urb->UserBufLen - p_urb->XferLen));
                        }
                    }
                }
            }

            bemp_reg >>= 1u;
        }
    }
}


/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/


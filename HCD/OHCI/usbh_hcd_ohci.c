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
*                                         GENERIC OHCI DRIVER
*
* Filename : usbh_ohci.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_OHCI_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_hcd_ohci.h"
#include  "../../Source/usbh_hub.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

                                                                /* ------------- OPERATIONAL REGISTER ACCESS ---------- */
#define  HcRevision(ohcibase)                       ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x00u))
#define  HcCtrl(ohcibase)                           ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x04u))
#define  HcCmdStatus(ohcibase)                      ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x08u))
#define  HcIntStatus(ohcibase)                      ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x0Cu))
#define  HcIntEn(ohcibase)                          ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x10u))
#define  HcIntDis(ohcibase)                         ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x14u))
#define  HcHCCA(ohcibase)                           ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x18u))
#define  HcPeriodCurED(ohcibase)                    ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x1Cu))
#define  HcCtrlHeadED(ohcibase)                     ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x20u))
#define  HcCtrlCurED(ohcibase)                      ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x24u))
#define  HcBulkHeadED(ohcibase)                     ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x28u))
#define  HcBulkCurED(ohcibase)                      ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x2Cu))
#define  HcDoneHead(ohcibase)                       ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x30u))
#define  HcFrameInterval(ohcibase)                  ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x34u))
#define  HcFrameRem(ohcibase)                       ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x38u))
#define  HcFrameNbr(ohcibase)                       ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x3Cu))
#define  HcPeriodicStart(ohcibase)                  ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x40u))
#define  HcLSTh(ohcibase)                           ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x44u))
#define  HcRhDescA(ohcibase)                        ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x48u))
#define  HcRhDescB(ohcibase)                        ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x4Cu))
#define  HcRhStatus(ohcibase)                       ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x50u))
#define  HcRhPortStatus(ohcibase, i)                ((CPU_INT32U *)((CPU_INT32U)(ohcibase) + 0x54u + (4 * (i))))

                                                                /* ----------- OHCI REGISTER OFFSET DEFINES ----------- */
#define  OHCI_OR_HC_REVISION                            0x00u   /* HCI specification implemented by HC.                 */
#define  OHCI_OR_HC_CTRL                                0x04u   /* Operating modes for the HC.                          */
#define  OHCI_OR_HC_CMD_STATUS                          0x08u   /* Used by HC to rx cmds from HCD & reflect HC status.  */
#define  OHCI_OR_HC_INT_STATUS                          0x0Cu   /* Provide status on events that cause HW interrupts.   */
#define  OHCI_OR_HC_INT_EN                              0x10u   /* Used to enable  HW interrupts.                       */
#define  OHCI_OR_HC_INT_DIS                             0x14u   /* Used to disable HW interrupts.                       */
#define  OHCI_OR_HC_HCCA                                0x18u   /* Physical addr of HCCA.                               */
#define  OHCI_OR_HC_PERIOD_CUR_ED                       0x1Cu   /* Physical addr of current isoc or intr ED.            */
#define  OHCI_OR_HC_CTRL_HEAD_ED                        0x20u   /* Physical addr of first   ED of ctrl list.            */
#define  OHCI_OR_HC_CTRL_CUR_ED                         0x24u   /* Physical addr of current ED of ctrl list.            */
#define  OHCI_OR_HC_BULK_HEAD_ED                        0x28u   /* Physical addr of first   ED of bulk list.            */
#define  OHCI_OR_HC_BULK_CUR_ED                         0x2Cu   /* Physical addr of current ED of bulk list.            */
#define  OHCI_OR_HC_DONE_HEAD                           0x30u   /* Physical addr of last compeleted TD added to done q. */
#define  OHCI_OR_HC_FRAME_INTERVAL                      0x34u   /* Bit time interval in frame & max pkt size w/o ovrun. */
#define  OHCI_OR_HC_FRAME_REM                           0x38u   /* Bit time remaining in current frame.                 */
#define  OHCI_OR_HC_FRAME_NBR                           0x3Cu   /* Frame number counter.                                */
#define  OHCI_OR_HC_PERIODIC_START                      0x40u   /* Earliest time HC should start processing per list.   */
#define  OHCI_OR_HC_LS_TH                               0x44u   /* Used by HC to determine whether to tx LS pkt.        */
#define  OHCI_OR_HC_RH_DESC_A                           0x48u   /* Describes characteristics of the root hub.           */
#define  OHCI_OR_HC_RH_DESC_B                           0x4Cu   /* Describes characteristics of the root hub.           */
#define  OHCI_OR_HC_RH_STATUS                           0x50u   /* Hub status & status change.                          */
#define  OHCI_OR_HC_RH_PORT_STATUS(n)                  (0x54u + (n) * 4)

                                                                /* ------------- OHCI REGISTER BIT DEFINES ------------ */
                                                                /* --------------- HcRevision REGISTER ---------------- */
#define  OHCI_OR_REVISION_MASK                    0x000000FFu
                                                                /* ---------------- HcControl REGISTER ---------------- */
#define  OHCI_OR_CTRL_CBSR_MASK                   0x00000003u   /* Control Bulk Service Ratio.                          */
#define  OHCI_OR_CTRL_CBSR_1_1                    0x00000000u
#define  OHCI_OR_CTRL_CBSR_2_1                    0x00000001u
#define  OHCI_OR_CTRL_CBSR_3_1                    0x00000002u
#define  OHCI_OR_CTRL_CBSR_4_1                    0x00000003u
#define  OHCI_OR_CTRL_PLE                         0x00000004u   /* Periodic List Enable.                                */
#define  OHCI_OR_CTRL_IE                          0x00000008u   /* Isochronous   Enable.                                */
#define  OHCI_OR_CTRL_CLE                         0x00000010u   /* Control  List Enable.                                */
#define  OHCI_OR_CTRL_BLE                         0x00000020u   /* Bulk     List Enable.                                */
#define  OHCI_OR_CTRL_HCFS                        0x000000C0u   /* Host Controller Functional State.                    */
#define  OHCI_OR_CTRL_HCFS_RESET                  0x00000000u
#define  OHCI_OR_CTRL_HCFS_RESUME                 0x00000040u
#define  OHCI_OR_CTRL_HCFS_OPER                   0x00000080u
#define  OHCI_OR_CTRL_HCFS_SUSPEND                0x000000C0u
#define  OHCI_OR_CTRL_IR                          0x00000100u   /* Interrupt Routing.                                       */
#define  OHCI_OR_CTRL_RWC                         0x00000200u   /* Remote Wakeup Connected.                                 */
#define  OHCI_OR_CTRL_RWE                         0x00000400u   /* Remote Wakeup Enable.                                    */

                                                                /* -------------- HcCommandStatus REGISTER ------------ */
#define  OHCI_OR_CMD_STATUS_HCR                   0x00000001u   /* Host Controller Reset.                               */
#define  OHCI_OR_CMD_STATUS_CLF                   0x00000002u   /* Control List Filled.                                 */
#define  OHCI_OR_CMD_STATUS_BLF                   0x00000004u   /* Bulk    List Filled.                                 */
#define  OHCI_OR_CMD_STATUS_OCR                   0x00000008u   /* Ownership Change Request.                            */
#define  OHCI_OR_CMD_STATUS_S0C                   0x00030000u   /* Scheduling Overrun Count.                                */

                                                                /* ------------ HcInterruptStatus REGISTER ------------ */
#define  OHCI_OR_INT_STATUS_SO                    0x00000001u   /* Scheduling Overrun.                                  */
#define  OHCI_OR_INT_STATUS_WDH                   0x00000002u   /* Writeback Done Head.                                 */
#define  OHCI_OR_INT_STATUS_SF                    0x00000004u   /* Start of Frame.                                      */
#define  OHCI_OR_INT_STATUS_RD                    0x00000008u   /* Resume Detected.                                     */
#define  OHCI_OR_INT_STATUS_UE                    0x00000010u   /* Unrecoverable Error.                                 */
#define  OHCI_OR_INT_STATUS_FNO                   0x00000020u   /* Frame Number Overflow.                               */
#define  OHCI_OR_INT_STATUS_RHSC                  0x00000040u   /* Root Hub Status Change.                              */
#define  OHCI_OR_INT_STATUS_OC                    0x40000000u   /* Ownership Change.                                    */

                                                                /* ------------ HcInterruptEnable REGISTER ------------ */
#define  OHCI_OR_INT_EN_SO                        0x00000001u   /* Scheduling Overrun.                                  */
#define  OHCI_OR_INT_EN_WDH                       0x00000002u   /* Writeback Done Head.                                 */
#define  OHCI_OR_INT_EN_SF                        0x00000004u   /* Start of Frame.                                      */
#define  OHCI_OR_INT_EN_RD                        0x00000008u   /* Resume Detected.                                     */
#define  OHCI_OR_INT_EN_UE                        0x00000010u   /* Unrecoverable Error.                                 */
#define  OHCI_OR_INT_EN_FNO                       0x00000020u   /* Frame Number Overflow.                               */
#define  OHCI_OR_INT_EN_RHSC                      0x00000040u   /* Root Hub Status Change.                              */
#define  OHCI_OR_INT_EN_OC                        0x40000000u   /* Ownership Change.                                    */
#define  OHCI_OR_INT_EN_MIE                       0x80000000u   /* Master Interrupt Enable.                             */

                                                                /* ----------- HcInterruptDisable REGISTER ------------ */
#define  OHCI_OR_INT_DISABLE_SO                   0x00000001u   /* Scheduling Overrun.                                  */
#define  OHCI_OR_INT_DISABLE_WDH                  0x00000002u   /* Writeback Done Head.                                 */
#define  OHCI_OR_INT_DISABLE_SF                   0x00000004u   /* Start of Frame.                                      */
#define  OHCI_OR_INT_DISABLE_RD                   0x00000008u   /* Resume Detected.                                     */
#define  OHCI_OR_INT_DISABLE_UE                   0x00000010u   /* Unrecoverable Error.                                 */
#define  OHCI_OR_INT_DISABLE_FNO                  0x00000020u   /* Frame Number Overflow.                               */
#define  OHCI_OR_INT_DISABLE_RHSC                 0x00000040u   /* Root Hub Status Change.                              */
#define  OHCI_OR_INT_DISABLE_OC                   0x40000000u   /* Ownership Change.                                    */
#define  OHCI_OR_INT_DISABLE_MIE                  0x80000000u   /* Master Interrupt Enable.                             */
                                                                /* ----------- HcPeriodCurrentED REGISTER ------------- */
#define  OHCI_OR_PERIOD_CUR_ED_PCED               0xFFFFFFF0u   /* Period Current ED.                                   */

                                                                /* ------------- HcControlHeadED REGISTER ------------- */
#define  OHCI_OR_CTRL_HEAD_ED_CHED                0xFFFFFFF0u   /* Control Head ED.                                     */

                                                                /* ------------ HcControlCurrentED REGISTER ----------- */
#define  OHCI_OR_CTRL_CUR_ED_CCED                 0xFFFFFFF0u   /* Control Current ED.                                  */

                                                                /* -------------- HcBulkHeadED REGISTER --------------- */
#define  OHCI_OR_BULK_HEAD_ED_BHED                0xFFFFFFF0u   /* Bulk Head ED.                                        */

                                                                /* -------------- HcBulkCurrentED REGISTER ------------ */
#define  OHCI_OR_BULK_CUR_ED_BCED                 0xFFFFFFF0u   /* Bulk Current ED.                                     */

                                                                /* --------------- HcDoneHead REGISTER ---------------- */
#define  OHCI_OR_DONEHEAD_DH                      0xFFFFFFF0u   /* Done Head.                                           */

                                                                /* ---------------- HcFmInterval REGISTER ------------- */
#define  OHCI_OR_FRAME_INTERVAL_FI                0x00003FFFu   /* Frame Interval.                                      */
#define  OHCI_OR_FRAME_INTERVAL_FSMPS             0x7FFF0000u   /* FS Largest Data Packet.                              */
#define  OHCI_OR_FRAME_INTERVAL_FIT               0x80000000u   /* Frame Interval Toggle.                               */

                                                                /* --------------- HcFmRemaining REGISTER ------------- */
#define  OHCI_OR_FRAME_REM_FR                     0x00003FFFu   /* Frame Remaining.                                     */
#define  OHCI_OR_FRAME_REM_FRT                    0x80000000u   /* Frame Remaining Toggle.                              */

                                                                /* ----------------- HcFmNumber REGISTER -------------- */
#define  OHCI_OR_FRAME_NBR_FN                     0x0000FFFFu   /* Frame Number.                                        */

                                                                /* -------------- HcPeriodicStart REGISTER ------------ */
#define  OHCI_OR_PERIODIC_START_PS                0x00003FFFu   /* Periodic Start.                                      */

                                                                /* -------------- HcLSThreshold REGISTER -------------- */
#define  OHCI_OR_LS_TH_LST                        0x000007FFu   /* LS Threshold.                                        */

                                                                /* ------------ HcRhDescriptorA REGISTER -------------- */
#define  OHCI_OR_RH_DA_NDP                        0x000000FFu   /* Number Downstream Ports.                             */
#define  OHCI_OR_RH_DA_PSM                        0x00000100u   /* Power Switching Mode.                                */
#define  OHCI_OR_RH_DA_NPS                        0x00000200u   /* No Power Switching.                                  */
#define  OHCI_OR_RH_DA_DT                         0x00000400u   /* Device Type.                                         */
#define  OHCI_OR_RH_DA_OCPM                       0x00000800u   /* Over Current Protection Mode.                        */
#define  OHCI_OR_RH_DA_NOCP                       0x00001000u   /* No Over Current Protection.                          */
#define  OHCI_OR_RH_DA_POTPGT                     0xFF000000u   /* Power On To Power Good Time.                         */

                                                                /* ------------- HcRhDescriptorB REGISTER ------------- */
#define  OHCI_OR_RH_DB_DR                         0x0000FFFFu   /* Device Removable.                                    */
#define  OHCI_OR_RH_DB_PPCM                       0xFFFF0000u   /* Port Power Control Mask.                             */

                                                                /* ---------------- HcRhStatus REGISTER --------------- */
#define  OHCI_OR_RH_STATUS_LPS                    0x00000001u   /* Local Power Status.                                  */
#define  OHCI_OR_RH_STATUS_OCI                    0x00000002u   /* Over Current indicator.                              */
#define  OHCI_OR_RH_STATUS_DRWE                   0x00008000u   /* Device Remote Wakeup Enable.                         */
#define  OHCI_OR_RH_STATUS_LPSC                   0x00010000u   /* Local Power Status Change.                           */
#define  OHCI_OR_RH_STATUS_OCIC                   0x00020000u   /* Over Current Indicator Change.                       */
#define  OHCI_OR_RH_STATUS_CRWE                   0x80000000u   /* Clear Remote Wakeup Enable.                          */

                                                                /* ---------- HcRhPortStatus[1:NDP] REGISTER ---------- */
#define  OHCI_OR_RH_PORT_CCS                      0x00000001u   /* Current Connect Status.                              */
#define  OHCI_OR_RH_PORT_PES                      0x00000002u   /* Port Enable     Status.                              */
#define  OHCI_OR_RH_PORT_PSS                      0x00000004u   /* Port Suspend    Status.                              */
#define  OHCI_OR_RH_PORT_POCI                     0x00000008u   /* Port Over Current Indicator.                         */
#define  OHCI_OR_RH_PORT_PRS                      0x00000010u   /* Port Reset Status.                                   */
#define  OHCI_OR_RH_PORT_PPS                      0x00000100u   /* Port Power Status.                                   */
#define  OHCI_OR_RH_PORT_LSDA                     0x00000200u   /* Low Speed Device Attached.                           */
#define  OHCI_OR_RH_PORT_CSC                      0x00010000u   /* Connect      Status    Change.                       */
#define  OHCI_OR_RH_PORT_PESC                     0x00020000u   /* Port Enable  Status    Change.                       */
#define  OHCI_OR_RH_PORT_PSSC                     0x00040000u   /* Port Suspend Status    Change.                       */
#define  OHCI_OR_RH_PORT_OCIC                     0x00080000u   /* Over Current Indicator Change.                       */
#define  OHCI_OR_RH_PORT_PRSC                     0x00100000u   /* Port Reset   Status    Change.                       */

                                                                /* --------- OHCI CONDITION CODE DEFINES -------------- */
                                                                /* Note(s) : (1) See 'OpenHCI - Open Host Controller    */
                                                                /*               Interface Specification for USB',      */
                                                                /*               Revision 1.0a, Section 4.3.3.          */
                                                                /*           (2) The 'CC' or 'Condition Code' field of  */
                                                                /*               a Transfer Descriptor contains one of  */
                                                                /*               these values.                          */
#define  OHCI_CODE_NOERROR                              0x00u   /* TD processed without errors.                         */
#define  OHCI_CODE_CRC                                  0x01u   /* Last data pkt from EP contained a CRC error.         */
#define  OHCI_CODE_BITSTUFFING                          0x02u   /* Last data pkt from EP contained a bitstuffing viol.  */
#define  OHCI_CODE_DATATOGGLEMISMATCH                   0x03u   /* Last pkt from EP had unexpected data toggle PID.     */
#define  OHCI_CODE_STALL                                0x04u   /* TD was moved to Done Queue because EP returned stall.*/
#define  OHCI_CODE_DEVICENOTRESPONDING                  0x05u   /* Dev didn't respond to token (IN) or provide hs (OUT).*/
#define  OHCI_CODE_PIDCHECKFAILURE                      0x06u   /* Chk bits on PID from EP failed on data PID or hs.    */
#define  OHCI_CODE_UNEXPECTEDPID                        0x07u   /* Rx PID was not valid or PID value not defined.       */
#define  OHCI_CODE_DATAOVERRUN                          0x08u   /* EP rtn'd more than max pkt size or more than buf.    */
#define  OHCI_CODE_DATAUNDERRUN                         0x09u   /* EP rtn'd less than max pkt size &  less than buf.    */
#define  OHCI_CODE_BUFFEROVERRUN                        0x0Cu   /* During IN,  HC could not write    data fast enough.  */
#define  OHCI_CODE_BUFFERUNDERRUN                       0x0Du   /* During OUT, HC could not retrieve data fast enough.  */
#define  OHCI_CODE_NOTACCESSED                          0x0Fu   /* Set by software before TD is placed on list.         */

                                                                /* ------------ FRAME INTERVAL DEFINES ---------------- */
#define  OHCI_FMIINTERVAL                             0x2EDFu   /* 12000 bits per frame.                                */
#define  OHCI_FMINTERVAL_DFLT                    ((((6u * (OHCI_FMIINTERVAL - 210u)) / 7u) << 16u) | OHCI_FMIINTERVAL)
#define  OHCI_FMINTERVAL_PERIODIC_START               0x2A2Fu

                                                                /* ------ OHCI ENDPOINT DESCRIPTOR FIELD DEFINES ------ */
                                                                /* Note(s) : (1) See 'OpenHCI - Open Host Controller    */
                                                                /*               Interface Specification for USB',      */
                                                                /*               Revision 1.0a, Section 4.2.2.          */
                                                                /* ------------------- CONTROL WORD ------------------- */
#define  OHCI_ED_FUNC_ADDR(x)                          ((x))    /* Usb Function Address.                                */
#define  OHCI_ED_EP_NUM(x)                        ((x) << 7)    /* EndPoint Number.                                     */
#define  OHCI_ED_DIR_TD                           0x00000000u   /* Get Direction from TD.                               */
#define  OHCI_ED_DIR_OUT                          0x00000800u   /* Direction Out.                                       */
#define  OHCI_ED_DIR_IN                           0x00001000u   /* Direction In.                                        */
#define  OHCI_ED_SPD_FULL                         0x00000000u   /* Full Speed.                                          */
#define  OHCI_ED_SPD_LOW                          0x00002000u   /* Low Speed.                                           */
#define  OHCI_ED_SKIP                             0x00004000u   /* Skip.                                                */
#define  OHCI_ED_FMT_GEN_TD                       0x00000000u   /* General Format.                                      */
#define  OHCI_ED_FMT_ISO_TD                       0x00008000u   /* Isochronous Format.                                  */
#define  OHCI_ED_MAXPKTSIZE(x)                 (((x) << 16u))   /* Maximum Packet Size.                                 */

                                                                /* ----------------- HEAD POINTER WORD ---------------- */
#define  OHCI_ED_HALT                             0x00000001u   /* Halted.                                              */
#define  OHCI_ED_TOGGLECARRY                      0x00000002u   /* Toggle Carry.                                        */

                                                                /* ------ OHCI TRANSMIT DESCRIPTOR FIELD DEFINES ------ */
                                                                /* Note(s) : (1) See 'OpenHCI - Open Host Controller    */
                                                                /*               Interface Specification for USB',      */
                                                                /*               Revision 1.0a, Sections 4.3.1.2.       */
#define  OHCI_TD_ROUNDING                         0x00040000u   /* Buffer Rounding.                                     */
#define  OHCI_TD_DIR_SETUP                        0x00000000u   /* Direction of Setup Packet.                           */
#define  OHCI_TD_DIR_OUT                          0x00080000u   /* Direction Out.                                       */
#define  OHCI_TD_DIR_IN                           0x00100000u   /* Direction In.                                        */
#define  OHCI_TD_DELAY_INT(x)        ((CPU_INT32U)(x) << 21u)   /* Delay Interrupt.                                     */
#define  OHCI_TD_TOGGLE_0                         0x02000000u   /* Toggle 0.                                            */
#define  OHCI_TD_TOGGLE_1                         0x03000000u   /* Toggle 1.                                            */
#define  OHCI_TD_CC               (((CPU_INT32U)0x0Fu << 28u))  /* Completion Code.                                     */

#define  OHCI_TDMASK_EC(x)          ((((x) & 0x0C000000u) >> 26))
#define  OHCI_TDMASK_CC(x)          ((((x) & 0xF0000000u) >> 28))

                                                                /* - OHCI ISOCHRONOUS TRANSMIT DESCRIPTOR FIELD DEFINES */
                                                                /* Note(s) : (1) See 'OpenHCI - Open Host Controller    */
                                                                /*               Interface Specification for USB',      */
                                                                /*               Revision 1.0a, Sections 4.3.2.1.       */
#define  OHCI_ITD_OFFSET_4K_PAGE_BOUNDARY             0x0FFFu   /* OffsetN field                                        */
#define  OHCI_ITD_OFFSET_BIT12_LOWER_WORD             0x1000u   /* OffsetN field                                        */
#define  OHCI_ITD_OFFSET_BIT12_UPPER_WORD         0x10000000u   /* OffsetN field                                        */
#define  OHCI_ITD_PSWN_CC_NOT_ACCESSED                0xE000u   /* Packet Status Word (Condition Code)                  */
#define  OHCI_TD_SF(x)                                 ((x))    /* Start of Frame.                                      */
#define  OHCI_TD_FC(x)               (((CPU_INT32U)x) << 24u)   /* Frame Count.                                         */

                                                                /* ------------- OHCI HCD_TD STATE DEFINES ------------ */
#define  OHCI_HCD_TD_STATE_NONE                            0u
#define  OHCI_HCD_TD_STATE_COMPLETED                       1u
#define  OHCI_HCD_TD_STATE_CANCELLED                       2u

                                                                /* ----------- OHCI POWER POWER METHOD DEFINES -------- */
#define  OHCI_PORT_PWRD_ALWAYS                             0u
#define  OHCI_PORT_PWRD_GLOBAL                             1u
#define  OHCI_PORT_PWRD_INDIVIDUAL                         2u

                                                                /* --------------- MEMORY MAPPING MACROS -------------- */
#define  VIR2BUS(x)                 USBH_OS_VirToBus((x))       /* Conversion of Address from Virtual to Physical       */
#define  BUS2VIR(x)                 USBH_OS_BusToVir((x))       /* Conversion of Address from Physical to Virtual       */

                                                                /* ---------- REGISTER ACCESS FUNCTION MACROS --------- */
#define  Rd32(reg)               (*((CPU_REG32 *)(reg)))
#define  Wr32(reg, val)         (*((CPU_REG32 *)(reg)) = val)

                                                                /* ----------------- ALIGNMENT MACROS ----------------- */
#define  DEF_ALIGN(x, a)        ((CPU_INT32U)(x) % (a) ? (a) - ((CPU_INT32U)(x) % (a)) +\
                                 (CPU_INT32U)(x) : (CPU_INT32U)(x))
#define  USB_ALIGNED(x, a)      (void *)USBH_OS_BusToVir((void *)DEF_ALIGN(USBH_OS_VirToBus((void *)(x)), (a)))

/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

                                                                /* ------------ OHCI 32MS INTERVAL LIST ORDER --------- */
static  const  CPU_INT32U  OHCI_BranchArray[] = {
                                       0u,  16u, 8u,  24u,
                                       4u,  20u, 12u, 28u,
                                       2u,  18u, 10u, 26u,
                                       6u,  22u, 14u, 30u,
                                       1u,  17u, 9u,  25u,
                                       5u,  21u, 13u, 29u,
                                       3u,  19u, 11u, 27u,
                                       7u,  23u, 15u, 31u
};


/*
*********************************************************************************************************
*                                           LOCAL DATA TYPES
*********************************************************************************************************
*/


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
static  void           OHCI_Init            (USBH_HC_DRV           *p_hc_drv,
                                             USBH_ERR              *p_err);

static  void           OHCI_Start           (USBH_HC_DRV           *p_hc_drv,
                                             USBH_ERR              *p_err);

static  void           OHCI_Stop            (USBH_HC_DRV           *p_hc_drv,
                                             USBH_ERR              *p_err);

static  USBH_DEV_SPD   OHCI_SpdGet          (USBH_HC_DRV           *p_hc_drv,
                                             USBH_ERR              *p_err);

static  void           OHCI_Suspend         (USBH_HC_DRV           *p_hc_drv,
                                             USBH_ERR              *p_err);

static  void           OHCI_Resume          (USBH_HC_DRV           *p_hc_drv,
                                             USBH_ERR              *p_err);

static  CPU_INT32U     OHCI_FrameNbrGet     (USBH_HC_DRV           *p_hc_drv,
                                             USBH_ERR              *p_err);

static  void           OHCI_EP_Open         (USBH_HC_DRV           *p_hc_drv,
                                             USBH_EP               *p_ep,
                                             USBH_ERR              *p_err);

static  void           OHCI_EP_Close        (USBH_HC_DRV           *p_hc_drv,
                                             USBH_EP               *p_ep,
                                             USBH_ERR              *p_err);

static  void           OHCI_EP_Abort        (USBH_HC_DRV           *p_hc_drv,
                                             USBH_EP               *p_ep,
                                             USBH_ERR              *p_err);

static  CPU_BOOLEAN    OHCI_IsHalt_EP       (USBH_HC_DRV           *p_hc_drv,
                                             USBH_EP               *p_ep,
                                             USBH_ERR              *p_err);

static  void           OHCI_URB_Submit      (USBH_HC_DRV           *p_hc_drv,
                                             USBH_URB              *p_urb,
                                             USBH_ERR              *p_err);

static  void           OHCI_URB_Complete    (USBH_HC_DRV           *p_hc_drv,
                                             USBH_URB              *p_urb,
                                             USBH_ERR              *p_err);

static  void           OHCI_URB_Abort       (USBH_HC_DRV           *p_hc_drv,
                                             USBH_URB              *p_urb,
                                             USBH_ERR              *p_err);

                                                                /* ---------------- INTERNAL FUNCTIONS ---------------- */
static  USBH_ERR       OHCI_DMA_Init        (USBH_HC_DRV           *p_hc_drv);

static  void           OHCI_HCD_ED_Clr      (OHCI_HCD_ED           *p_hcd_ed);

static  void           OHCI_HC_ED_Clr       (OHCI_HC_ED            *p_hc_ed);

static  OHCI_HCD_ED   *OHCI_HCD_ED_Create   (USBH_HC_DRV           *p_hc_drv);

static  void           OHCI_HCD_ED_Destroy  (USBH_HC_DRV           *p_hc_drv,
                                             OHCI_HCD_ED           *p_hcd_ed);

static  void           OHCI_HCD_TD_Clr      (OHCI_HCD_TD           *p_hcd_td);

static  void           OHCI_HC_TD_Clr       (OHCI_HC_TD            *p_hc_td);

static  OHCI_HCD_TD   *OHCI_HCD_TD_Create   (USBH_HC_DRV           *p_hc_drv);

static  void           OHCI_HCD_TD_Destroy  (USBH_HC_DRV           *p_hc_drv,
                                             OHCI_HCD_TD           *p_hcd_td);

static  USBH_ERR       OHCI_PeriodicListInit(USBH_HC_DRV           *p_hc_drv);

static  USBH_ERR       OHCI_PeriodicEPOpen  (USBH_HC_DRV           *p_hc_drv,
                                             OHCI_HCD_ED           *p_ed,
                                             CPU_INT32U             interval);

static  OHCI_HCD_TD   *OHCI_HC_TD_Insert    (USBH_HC_DRV           *p_hc_drv,
                                             USBH_EP               *p_ep,
                                             USBH_URB              *p_urb,
                                             CPU_INT32U             td_ctrl,
                                             CPU_INT32U             buf_ptr,
                                             CPU_INT32U             buf_end,
                                             CPU_INT32U            *p_offsets);

static  USBH_ERR       OHCI_EPPause         (USBH_HC_DRV           *p_hc_drv,
                                             USBH_EP               *p_ep);

static  void           OHCI_EPResume        (USBH_HC_DRV           *p_hc_drv,
                                             USBH_EP               *p_ep);

static  void           OHCI_EPHalt          (USBH_EP               *p_ep);

static  void           OHCI_HaltEPClr       (USBH_EP               *p_ep);

static  USBH_ERR       OHCI_DoneHeadProcess (USBH_HC_DRV           *p_hc_drv);

static  void           OHCI_ISR             (void                  *p_arg);

                                                                /* -------------- ROOT HUB API FUNCTIONS -------------- */
static  CPU_BOOLEAN    OHCI_PortStatusGet   (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr,
                                             USBH_HUB_PORT_STATUS  *p_port_status);

static  CPU_BOOLEAN    OHCI_HubDescGet      (USBH_HC_DRV           *p_hc_drv,
                                             void                  *p_buf,
                                             CPU_INT08U             buf_len);

static  CPU_BOOLEAN    OHCI_PortEnSet       (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortEnClr       (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortEnChngClr   (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortPwrSet      (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortPwrClr      (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortResetSet    (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortResetChngClr(USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortSuspendClr  (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_PortConnChngClr (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  CPU_BOOLEAN    OHCI_RHSC_IntEn      (USBH_HC_DRV           *p_hc_drv);

static  CPU_BOOLEAN    OHCI_RHSC_IntDis     (USBH_HC_DRV           *p_hc_drv);

static  CPU_INT08S     OHCI_PortPwrModeGet  (USBH_HC_DRV           *p_hc_drv,
                                             CPU_INT08U             port_nbr);

static  void           OHCI_SetDataPtr      (OHCI_DEV              *p_ohci,
                                             OHCI_HCD_TD           *p_hcd_td,
                                             OHCI_HC_TD            *p_hc_td);

static  OHCI_HCD_TD   *OHCI_GetDataPtr      (OHCI_DEV              *p_ohci,
                                             OHCI_HC_TD            *p_hc_td);


/*
*********************************************************************************************************
*                                 INITIALIZED GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_HC_DRV_API  OHCI_DrvAPI = {
    OHCI_Init,
    OHCI_Start,
    OHCI_Stop,
    OHCI_SpdGet,
    OHCI_Suspend,
    OHCI_Resume,
    OHCI_FrameNbrGet,

    OHCI_EP_Open,
    OHCI_EP_Close,
    OHCI_EP_Abort,
    OHCI_IsHalt_EP,

    OHCI_URB_Submit,
    OHCI_URB_Complete,
    OHCI_URB_Abort,
};

USBH_HC_RH_API  OHCI_RH_API = {
    OHCI_PortStatusGet,
    OHCI_HubDescGet,

    OHCI_PortEnSet,
    OHCI_PortEnClr,
    OHCI_PortEnChngClr,

    OHCI_PortPwrSet,
    OHCI_PortPwrClr,

    OHCI_PortResetSet,
    OHCI_PortResetChngClr,

    OHCI_PortSuspendClr,
    OHCI_PortConnChngClr,

    OHCI_RHSC_IntEn,
    OHCI_RHSC_IntDis
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
*                                             OHCI_Init()
*
* Description : Initialize OHCI host controller, issue hardware reset and software reset,
*
*               initalize frame interval, and enable interrupts
*
* Argument(s) : hc_nbr                  Host controller number
*
*               p_base_addr             Base address of the host controller
*
*               p_hc_dev                Pointer to the OHCI device structure
*
*               p_rh_dev                Pointer to the root hub device structure
*
* Return(s)   : USBH_ERR_NONE            If initialization is success
*               USBH_ERR_HC_INIT_FAILED  If HC reset failed
*               Specific error code      Otherwise
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_Init (USBH_HC_DRV  *p_hc_drv,
                         USBH_ERR     *p_err)
{
    OHCI_DEV         *p_ohci;
    USBH_HC_CFG      *p_hc_cfg;
    USBH_HC_BSP_API  *p_bsp_api;
    CPU_INT32U        hc_ctrl;
    CPU_SIZE_T        octets_reqd;
    CPU_INT32U        cnt;
    LIB_ERR           err_lib;
    CPU_BOOLEAN       valid;


    p_ohci = (OHCI_DEV *)Mem_HeapAlloc(sizeof(OHCI_DEV),
                                       sizeof(CPU_ALIGN),
                                      &octets_reqd,
                                      &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }

    Mem_Clr(p_ohci, sizeof(OHCI_DEV));

    p_hc_drv->DataPtr = (void *)p_ohci;
    p_bsp_api         = p_hc_drv->BSP_API_Ptr;

    if ((p_bsp_api       != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->Init !=                    0)) {
        p_bsp_api->Init(p_hc_drv, p_err);
        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

   *p_err = OHCI_DMA_Init(p_hc_drv);                            /* Initialize OHCI DMA structures                       */
    if (*p_err != USBH_ERR_NONE) {
        return;
    }

    p_hc_cfg = p_hc_drv->HC_CfgPtr;
                                                                /* Disable all interrupts                               */
    Wr32(HcIntDis(p_hc_cfg->BaseAddr), OHCI_OR_INT_DISABLE_MIE  |
                                       OHCI_OR_INT_DISABLE_SO   |
                                       OHCI_OR_INT_DISABLE_WDH  |
                                       OHCI_OR_INT_DISABLE_SF   |
                                       OHCI_OR_INT_DISABLE_RD   |
                                       OHCI_OR_INT_DISABLE_UE   |
                                       OHCI_OR_INT_DISABLE_FNO  |
                                       OHCI_OR_INT_DISABLE_RHSC |
                                       OHCI_OR_INT_DISABLE_OC);

    valid = OHCI_RHSC_IntDis(p_hc_drv);                         /* Disable Root Hub interrupt                           */
    if (valid == DEF_FAIL) {
       *p_err = USBH_ERR_HC_INIT;
        return;
    }

    hc_ctrl = Rd32( HcCtrl(p_hc_cfg->BaseAddr));
    if ((hc_ctrl & OHCI_OR_CTRL_RWC) != 0u) {
        hc_ctrl = OHCI_OR_CTRL_RWE;                             /* Enable remote wakeup                                 */
        p_ohci->CanWakeUp = 1u;
    } else {
        hc_ctrl = 0u;
    }
                                                                /* ------------- RESET HOST CONTROLLER ---------------- */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
    USBH_PRINT_LOG("OHCI Applying Hardware Reset...\r\n");
#endif

    USBH_OS_DlyMS(50u);                                         /* Wait 50 ms before applying reset.                    */

    hc_ctrl |= OHCI_OR_CTRL_HCFS_RESET;                         /* Apply hardware reset.                                */
    Wr32(HcCtrl(p_hc_cfg->BaseAddr), hc_ctrl);

    Wr32(HcCtrlHeadED(p_hc_cfg->BaseAddr), 0u);                  /* Initialize control list head    to zero.             */
    Wr32(HcBulkHeadED(p_hc_cfg->BaseAddr), 0u);                  /* Initialize bulk    list head    to zero.             */
    Wr32(HcHCCA(p_hc_cfg->BaseAddr), 0u);                        /* Initialize HCCA interrupt table to zero.             */

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
    USBH_PRINT_LOG("OHCI Applying Software Reset...\r\n");
#endif
                                                                /* Apply software reset.                                */
    Wr32(HcCmdStatus(p_hc_cfg->BaseAddr), OHCI_OR_CMD_STATUS_HCR);
    cnt = 30u;                                                   /* HC software reset may take 10 us, wait 30 us.        */

    while ((Rd32(HcCmdStatus(p_hc_cfg->BaseAddr)) & OHCI_OR_CMD_STATUS_HCR) != 0u) {

        if (cnt == 0u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("Host controller Software RESET failed.\r\n");
#endif
           *p_err = USBH_ERR_HC_INIT;
            return;
        }
        cnt--;
        USBH_OS_DlyUS(1u);
    }
                                                                /* HC goto OPERATIONAL state in 2 ms time limit         */
                                                                /* Write frame interval & largest data pkt ctr.         */
    Wr32(HcFrameInterval(p_hc_cfg->BaseAddr), OHCI_FMINTERVAL_DFLT);
                                                                /* Write periodic start.                                */
    Wr32(HcPeriodicStart(p_hc_cfg->BaseAddr), OHCI_FMINTERVAL_PERIODIC_START);
                                                                /* Root hub number of ports                             */
    p_ohci->NbrPorts = Rd32(HcRhDescA(p_hc_cfg->BaseAddr)) & OHCI_OR_RH_DA_NDP;

   *p_err = OHCI_PeriodicListInit(p_hc_drv);                    /* Initialize Periodic lists                            */
    if (*p_err != USBH_ERR_NONE) {
        return;
    }
                                                                /* HCCA DMA                                             */
    Wr32(HcHCCA(p_hc_cfg->BaseAddr), (CPU_INT32U)VIR2BUS((void *)p_ohci->DMA_OHCI.HCCAPtr));

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                            OHCI_Start()
*
* Description : Start OHCI HC
*
* Argument(s) : p_ohci           Pointer to OHCI device structure
*
* Return(s)   : USBH_ERR_NONE
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_Start (USBH_HC_DRV  *p_hc_drv,
                          USBH_ERR     *p_err)
{
    CPU_INT32U        hc_ctrl;
    USBH_HC_BSP_API  *p_bsp_api;
    USBH_HC_CFG      *p_hc_cfg;


    p_hc_cfg  = p_hc_drv->HC_CfgPtr;
    p_bsp_api = p_hc_drv->BSP_API_Ptr;
    hc_ctrl   = Rd32(HcCtrl(p_hc_cfg->BaseAddr));
    if ((hc_ctrl & OHCI_OR_CTRL_RWC) != 0u) {
        hc_ctrl = OHCI_OR_CTRL_RWE;                             /* Enable remote wakeup                                 */
    }

    hc_ctrl &= ~OHCI_OR_CTRL_HCFS;
    hc_ctrl |=  OHCI_OR_CTRL_HCFS_OPER;                         /* Put HC in operational state.                         */

    Wr32(HcCtrl(p_hc_cfg->BaseAddr), hc_ctrl);
    Wr32(HcIntDis(p_hc_cfg->BaseAddr), 0u);

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
    USBH_PRINT_LOG("OHCI Enabling Interrupts...\r\n");
#endif

    if ((p_bsp_api          != (USBH_HC_BSP_API *)0) &&
        (p_bsp_api->ISR_Reg !=                    0)) {
        p_bsp_api->ISR_Reg(OHCI_ISR, p_err);

        if (*p_err != USBH_ERR_NONE) {
            return;
        }
    }

    Wr32(HcIntEn(p_hc_cfg->BaseAddr), OHCI_OR_INT_EN_MIE  |     /* Enable int.                                          */
                                      OHCI_OR_INT_EN_WDH  |
                                      OHCI_OR_INT_EN_FNO  |
                                      OHCI_OR_INT_EN_RD   |
                                      OHCI_OR_INT_EN_SO   |
                                      OHCI_OR_INT_EN_UE   |
                                      OHCI_OR_INT_EN_OC);

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                             OHCI_Stop()
*
* Description : Stop OHCI HC
*
* Argument(s) : p_ohci           Pointer to OHCI device structure
*
* Return(s)   : USBH_ERR_NONE
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_Stop (USBH_HC_DRV  *p_hc_drv,
                         USBH_ERR     *p_err)
{
    USBH_HC_CFG      *p_hc_cfg;
    USBH_HC_BSP_API  *p_bsp_api;


    p_hc_cfg  = p_hc_drv->HC_CfgPtr;
    p_bsp_api = p_hc_drv->BSP_API_Ptr;

    Wr32(HcCtrl(p_hc_cfg->BaseAddr), OHCI_OR_CTRL_HCFS_RESET);  /* Apply hw reset.                                      */

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
*                                            OHCI_SpeedGet()
*
* Description : Returns Host Controller Speed.
*
* Argument(s) : p_hc_dev            Pointer to OHCI device structure
*
* Return(s)   : USBH_DEV_SPD_FULL
*
* Note(s)     : None.
*
*********************************************************************************************************
*/

static  USBH_DEV_SPD  OHCI_SpdGet (USBH_HC_DRV  *p_hc_drv,
                                   USBH_ERR     *p_err)
{
    (void)p_hc_drv;

   *p_err = USBH_ERR_NONE;
    return (USBH_DEV_SPD_FULL);                                                 /*Since OHCI controller standard supports upto FULL speed */
}


/*
*********************************************************************************************************
*                                           OHCI_Suspend()
*
* Description : Suspend HC
*
* Argument(s) : p_dev            Pointer to root hub device structure
*
* Return(s)   : USBH_ERR_NONE     If the HC is suspended
*               USBH_ERR_UNKNOWN  If the HC state is not among the 4 states described in USB spec
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_Suspend (USBH_HC_DRV  *p_hc_drv,
                            USBH_ERR     *p_err)
{
    CPU_INT32U    hc_ctrl;
    CPU_INT32U    cnt;
    CPU_INT32U    cur_frame_nbr;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    hc_ctrl  = Rd32( HcCtrl(p_hc_cfg->BaseAddr));               /* Read the Host controller state                       */

    switch ((hc_ctrl & OHCI_OR_CTRL_HCFS)) {
        case OHCI_OR_CTRL_HCFS_RESET:
        case OHCI_OR_CTRL_HCFS_SUSPEND:
            *p_err = USBH_ERR_NONE;                             /* HC is already in suspend state                       */
             return;

        case OHCI_OR_CTRL_HCFS_RESUME:                          /* Current state is resume or operational,...           */
        case OHCI_OR_CTRL_HCFS_OPER:                            /* ...goto suspend state.                               */
             break;

        default:
            *p_err = USBH_ERR_UNKNOWN;
             return;
    }
                                                                /* Stop list processing; take effect at the next frame  */
    hc_ctrl &=  ~(OHCI_OR_CTRL_PLE |
                  OHCI_OR_CTRL_IE  |
                  OHCI_OR_CTRL_CLE |
                  OHCI_OR_CTRL_BLE);

    cnt           = 0u;
    cur_frame_nbr = Rd32(HcFrameNbr(p_hc_cfg->BaseAddr));       /* Get the current frame number                         */

                                                                /* Wait for the current frame to complete               */
    while (Rd32(HcFrameNbr(p_hc_cfg->BaseAddr)) == cur_frame_nbr) {
        ++cnt;

        if (cnt > 3u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("OHCI_Suspend() FmNumber does not increment !!! ERROR\r\n");
#endif
           *p_err = USBH_ERR_UNKNOWN;
            return;
        }
        USBH_OS_DlyMS(1u);
     }

    hc_ctrl &= ~OHCI_OR_CTRL_HCFS;
    hc_ctrl |=  OHCI_OR_CTRL_HCFS_SUSPEND;                      /* Enable suspend                                       */

    Wr32( HcCtrl(p_hc_cfg->BaseAddr), hc_ctrl);                 /* Write the changes to control register                */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                            OHCI_Resume()
*
* Description : Resume HC
*
* Argument(s) : p_hc_dev         Pointer to OHCI device structure
*
* Return(s)   : USBH_ERR_NONE     If the HC is resumed
*               USBH_ERR_UNKNOWN  If the HC state is not among the 4 states described in USB spec
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_Resume (USBH_HC_DRV  *p_hc_drv,
                           USBH_ERR     *p_err)
{
    CPU_INT32U    hc_ctrl;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    hc_ctrl  = Rd32( HcCtrl(p_hc_cfg->BaseAddr));               /* Read the Host controller  state                      */

    switch ((hc_ctrl & OHCI_OR_CTRL_HCFS)) {
        case OHCI_OR_CTRL_HCFS_RESUME:
        case OHCI_OR_CTRL_HCFS_OPER:
            *p_err = USBH_ERR_NONE;
             return;

        case OHCI_OR_CTRL_HCFS_RESET:
        case OHCI_OR_CTRL_HCFS_SUSPEND:
             break;

        default:
            *p_err = USBH_ERR_UNKNOWN;
             return;
    }

    hc_ctrl &= ~OHCI_OR_CTRL_HCFS;                              /* Enable Resume                                        */
    hc_ctrl |=  OHCI_OR_CTRL_HCFS_RESUME;
    Wr32( HcCtrl(p_hc_cfg->BaseAddr), hc_ctrl);

    hc_ctrl &= ~OHCI_OR_CTRL_HCFS;                              /* Enable Operational                                   */
    hc_ctrl |=  OHCI_OR_CTRL_HCFS_OPER;
    Wr32( HcCtrl(p_hc_cfg->BaseAddr), hc_ctrl);

    Wr32( HcIntEn(p_hc_cfg->BaseAddr), OHCI_OR_INT_EN_RHSC);

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         OHCI_FrameNbrGet()
*
* Description : Retrieve frame number.
*
* Argument(s) : p_dev            Pointer to device structure.
*
* Return(s)   : Frame number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  OHCI_FrameNbrGet (USBH_HC_DRV  *p_hc_drv,
                                      USBH_ERR     *p_err)
{
    USBH_HC_CFG  *p_hc_cfg;
    CPU_INT32U    frm_nbr;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    frm_nbr  = Rd32(HcFrameNbr(p_hc_cfg->BaseAddr));            /* Get the current frame number                         */

   *p_err = USBH_ERR_NONE;
    return (frm_nbr);
}


/*
*********************************************************************************************************
*                                            OHCI_EP_Open()
*
* Description : Create OHCI endpoint descriptor structure for the given endpoint
*
* Argument(s) : p_dev            Pointer to device structure
*
*               p_ep             Pointer to endpoint structure
*
* Return(s)   : USBH_ERR_NONE          If endpoint open is success
*               USBH_ERR_INVALID_ARGS  If p_dev or p_ep is 0
*               USBH_ERR_HW_DESC_ALLOC If ED is not created due to insufficient memory
*               Specific error code    Otherwise
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_EP_Open (USBH_HC_DRV  *p_hc_drv,
                            USBH_EP      *p_ep,
                            USBH_ERR     *p_err)
{
    OHCI_HCD_ED  *p_hcd_ed_list;
    OHCI_HC_ED   *p_new_hc_ed;
    OHCI_HCD_ED  *p_new_hcd_ed;
    OHCI_DEV     *p_ohci;
    CPU_INT32U    list_idx;
    CPU_INT08U    ep_nbr;
    CPU_INT08U    ep_type;
    CPU_INT16U    ep_max_pkt_size;
    CPU_INT08U    ep_dir;


    p_ohci          = (OHCI_DEV *)p_hc_drv->DataPtr;
    ep_nbr          = USBH_EP_LogNbrGet(p_ep);                  /* Get endpoint attributes                              */
    ep_dir          = USBH_EP_DirGet(p_ep);
    ep_type         = USBH_EP_TypeGet(p_ep);
    ep_max_pkt_size = USBH_EP_MaxPktSizeGet(p_ep);
    p_new_hcd_ed    = OHCI_HCD_ED_Create(p_hc_drv);

    if (p_new_hcd_ed == (OHCI_HCD_ED *)0) {
       *p_err = USBH_ERR_ALLOC;
        return;
    }
                                                                /* Initialize HC_ED structure                           */
    p_new_hc_ed       = p_new_hcd_ed->HC_EDPtr;
    p_new_hc_ed->Ctrl = OHCI_ED_FUNC_ADDR(p_ep->DevAddr)                                               |
                        OHCI_ED_EP_NUM(ep_nbr)                                                         |
                        ((ep_dir == USBH_EP_DIR_IN) ? OHCI_ED_DIR_IN :
                         ((ep_dir       == USBH_EP_DIR_OUT)   ? OHCI_ED_DIR_OUT : OHCI_ED_DIR_TD))     |
                         ((p_ep->DevSpd == USBH_DEV_SPD_FULL) ? OHCI_ED_SPD_FULL : OHCI_ED_SPD_LOW)    |
                        (((ep_type == USBH_EP_TYPE_CTRL) || (ep_type == USBH_EP_TYPE_BULK) || (ep_type == USBH_EP_TYPE_INTR)) ?
                           OHCI_ED_FMT_GEN_TD : OHCI_ED_FMT_ISO_TD)    |
                        OHCI_ED_MAXPKTSIZE(ep_max_pkt_size);

    p_new_hc_ed->Next            = 0u;
    p_new_hc_ed->TailTD          = 0u;
    p_new_hc_ed->HeadTD          = 0u;
    p_new_hcd_ed->Head_HCD_TDPtr = 0;
    p_new_hcd_ed->Tail_HCD_TDPtr = 0;
    p_new_hcd_ed->NextPtr        = 0;
                                                                /* Ctrl or Bulk ep open                                 */
    if ((ep_type == USBH_EP_TYPE_CTRL) ||
        (ep_type == USBH_EP_TYPE_BULK)) {

        if (ep_type == USBH_EP_TYPE_CTRL) {
            list_idx = OHCI_ED_LIST_CTRL;
        } else {
            list_idx = OHCI_ED_LIST_BULK;
        }

        p_hcd_ed_list = p_ohci->EDLists[list_idx];

        if (p_hcd_ed_list == (OHCI_HCD_ED *)0) {                /* Check for empty list                                 */
            p_hcd_ed_list              = p_new_hcd_ed;
            p_ohci->EDLists[list_idx]  = p_new_hcd_ed;
        } else {

            while (p_hcd_ed_list->NextPtr) {                    /* Goto end of Ed List                                  */
                p_hcd_ed_list = p_hcd_ed_list->NextPtr;
            }
            p_hcd_ed_list->NextPtr        = p_new_hcd_ed;       /* Insert ed at the end                                 */
            p_hcd_ed_list->HC_EDPtr->Next = p_new_hcd_ed->DMA_HC_ED;
        }

    } else {
       *p_err = OHCI_PeriodicEPOpen(p_hc_drv,                     /* Periodic ep open                                     */
                                    p_new_hcd_ed,
                                    p_ep->Interval);
        if (*p_err != USBH_ERR_NONE) {
            OHCI_HCD_ED_Destroy(p_hc_drv, p_new_hcd_ed);
            return;
        }
    }

    p_ep->ArgPtr = (void *)p_new_hcd_ed;
    OHCI_HC_TD_Insert(p_hc_drv, p_ep, 0, 0u, 0u, 0u, 0);           /* Insert Dummy TD in the list                                      */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           OHCI_EP_Close()
*
* Description : Close the endpoint by unlinking the OHCI endpoint descriptor
*
* Argument(s) : p_dev            Pointer to device structure
*
*               p_ep             Pointer to endpoint structure
*
* Return(s)   : USBH_ERR_NONE               If endpoint closed successfully
*               USBH_ERR_INVALID_ARGS       If p_dev or p_ep is 0
*               Specific error code         Otherwise
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_EP_Close (USBH_HC_DRV  *p_hc_drv,
                             USBH_EP      *p_ep,
                             USBH_ERR     *p_err)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;
    OHCI_HCD_ED  *p_hcd_ed;
    OHCI_HCD_ED  *p_hcd_ed_list;
    CPU_INT32U    list_idx;
    CPU_INT32U    list_begin_idx;
    CPU_INT32U    list_end_idx;
    CPU_INT32U    mask;
    CPU_INT32U    hc_ctrl;
    CPU_INT32U   *p_hc_cur_ed;
    CPU_INT08U    ep_type;


    p_hc_cfg      = p_hc_drv->HC_CfgPtr;
    p_ohci        = (OHCI_DEV    *)p_hc_drv->DataPtr;
    p_hcd_ed      = (OHCI_HCD_ED *)p_ep->ArgPtr;
    p_hcd_ed_list = (OHCI_HCD_ED *)0;
    ep_type       = USBH_EP_TypeGet(p_ep);

    if (ep_type == USBH_EP_TYPE_CTRL) {                         /* Determine which list                                 */
        list_begin_idx = OHCI_ED_LIST_CTRL;                     /* There is only one Control list                       */
        list_end_idx   = OHCI_ED_LIST_CTRL;                     /* There is only one Control list                       */
        mask           = OHCI_OR_CTRL_CLE;
        p_hc_cur_ed    = HcCtrlCurED(p_hc_cfg->BaseAddr);

    } else if (ep_type == USBH_EP_TYPE_BULK) {
        list_begin_idx =  OHCI_ED_LIST_BULK;                    /* There is only one Bulk list                          */
        list_end_idx   =  OHCI_ED_LIST_BULK;                    /* There is only one Bulk list                          */
        mask           =  OHCI_OR_CTRL_BLE;
        p_hc_cur_ed    =   HcBulkCurED(p_hc_cfg->BaseAddr);

    } else {
        list_begin_idx = OHCI_ED_LIST_32MS;                     /* Update bandwidth usage                               */
        list_end_idx   = OHCI_ED_LIST_01MS;                     /* There are 32 periodic lists                          */
        mask           = OHCI_OR_CTRL_PLE;
        p_hc_cur_ed    = HcPeriodCurED(p_hc_cfg->BaseAddr);
    }
                                                                /* Disable list processing by HC                        */
    hc_ctrl  =  Rd32( HcCtrl(p_hc_cfg->BaseAddr));
    hc_ctrl &= ~mask;
    Wr32( HcCtrl(p_hc_cfg->BaseAddr), hc_ctrl);

   *p_err = OHCI_EPPause(p_hc_drv, p_ep);
    if (*p_err != USBH_ERR_NONE) {
        return;
    }
                                                                /* Go through each list                                 */
    for (list_idx = list_begin_idx; list_idx <= list_end_idx; list_idx++) {
        p_hcd_ed_list = p_ohci->EDLists[list_idx];              /* Get the Head ed of the list                          */

        if (p_hcd_ed_list == p_hcd_ed) {                        /* The current ed found at head                         */
            break;
        }
                                                                /* Locate the previous Ed to the current ed             */
        while ((p_hcd_ed_list          != 0u      ) &&
               (p_hcd_ed_list->NextPtr != p_hcd_ed)) {
            p_hcd_ed_list = p_hcd_ed_list->NextPtr;
        }
                                                                /* The current ed found in the middle                   */
        if ((p_hcd_ed_list          != 0u      ) &&
            (p_hcd_ed_list->NextPtr == p_hcd_ed)) {
            break;
        }
    }


    if (p_hcd_ed_list == p_hcd_ed) {                            /* Unlink this ed from the list                         */
        p_ohci->EDLists[list_idx] = p_hcd_ed_list->NextPtr;     /* Current ed is head. Update head                      */

        if (ep_type == USBH_EP_TYPE_CTRL) {

            if (p_ohci->EDLists[list_idx]) {
                Wr32(HcCtrlHeadED(p_hc_cfg->BaseAddr), p_ohci->EDLists[OHCI_ED_LIST_CTRL]->DMA_HC_ED);
            } else {
                Wr32(HcCtrlHeadED(p_hc_cfg->BaseAddr), 0u);
            }

        } else if (ep_type == USBH_EP_TYPE_BULK) {

            if (p_ohci->EDLists[list_idx]) {
                Wr32(HcBulkHeadED(p_hc_cfg->BaseAddr), p_ohci->EDLists[OHCI_ED_LIST_BULK]->DMA_HC_ED);
            } else {
                Wr32(HcBulkHeadED(p_hc_cfg->BaseAddr), 0u);
            }

        } else {                                                /* ed found in Periodic head?                           */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG(" #######ERROR!!! ED found in Periodic head ???######\r\n");
#endif
        }

    } else if (p_hcd_ed_list && (p_hcd_ed_list->NextPtr == p_hcd_ed)) {
        p_hcd_ed_list->NextPtr        = p_hcd_ed->NextPtr;      /* Unlink ed. The Previous ed should point to next ...  */
        p_hcd_ed_list->HC_EDPtr->Next = p_hcd_ed->HC_EDPtr->Next;   /* of current ed                                    */
        p_hcd_ed->HC_EDPtr->HeadTD    = 0u;                     /* Empy TD List                                         */
        p_hcd_ed->HC_EDPtr->TailTD    = 0u;
    }

    OHCI_HCD_TD_Destroy(p_hc_drv, p_hcd_ed->Head_HCD_TDPtr);   /* Free dummy TD                                        */
    OHCI_HCD_ED_Destroy(p_hc_drv, p_hcd_ed);
    p_ep->ArgPtr = (void *)0;
    Wr32(p_hc_cur_ed, 0u);                                      /* Current ed regiser should not point to this ed       */
                                                                /* Enable list processing by HC                         */
    hc_ctrl  = Rd32( HcCtrl(p_hc_cfg->BaseAddr));
    hc_ctrl |= mask;
    Wr32( HcCtrl(p_hc_cfg->BaseAddr), hc_ctrl);

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           OHCI_EP_Abort()
*
* Description : Abort all pending URBs in the queue head.
*
* Argument(s) : p_dev            Pointer to device structure.
*
*               p_ep             Pointer to endpoint structure.
*
* Return(s)   : USBH_ERR_NONE           If success
*               USBH_ERR_INVALID_ARGS   If p_dev or p_ep is 0
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_EP_Abort (USBH_HC_DRV  *p_hc_drv,
                             USBH_EP      *p_ep,
                             USBH_ERR     *p_err)
{
    OHCI_HCD_ED  *p_hcd_ed;
    USBH_URB     *p_urb;
    OHCI_HCD_TD  *p_temp_td;


    p_hcd_ed = (OHCI_HCD_ED *)p_ep->ArgPtr;
    p_urb    = (USBH_URB    *)0;

    p_temp_td = p_hcd_ed->Head_HCD_TDPtr;                       /* Flush all TDs and make TD list empty in this ED.     */

    while ((p_temp_td         != (OHCI_HCD_TD *)0) &&
           (p_temp_td->URBPtr != (USBH_URB    *)0)) {

        p_urb = p_temp_td->URBPtr;

        OHCI_EPPause(p_hc_drv, p_ep);                           /* Pause EP, HC skips processing ED pointed to by EP.   */
        p_urb->Err = USBH_ERR_URB_ABORT;
        USBH_URB_Complete(p_urb);

        p_temp_td = p_hcd_ed->Head_HCD_TDPtr;                   /* Get the updated headTd                               */
    }

    p_hcd_ed->HC_EDPtr->HeadTD = p_hcd_ed->HC_EDPtr->TailTD;    /* Empty TD List                                        */
    OHCI_EPResume(p_hc_drv, p_ep);                              /* Resume EP, HC can now process this ED                */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           OHCI_IsHalt_EP()
*
* Description : Retrieve endpoint halt state.
*
* Argument(s) : p_dev            Pointer to device structure.
*
*               p_ep             Pointer to endpoint structure.
*
* Return(s)   : DEF_TRUE   If halted.
*
*               DEF_FALSE  Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_IsHalt_EP (USBH_HC_DRV  *p_hc_drv,
                                     USBH_EP      *p_ep,
                                     USBH_ERR     *p_err)
{
    OHCI_HCD_ED  *p_hcd_ed;
    OHCI_HC_ED   *p_hc_ed;


    (void)p_hc_drv;

    p_hcd_ed         = (OHCI_HCD_ED *)p_ep->ArgPtr;
    p_hc_ed          =  p_hcd_ed->HC_EDPtr;
    p_hcd_ed->IsHalt = (((p_hc_ed->HeadTD & OHCI_ED_HALT) != 0u) ? DEF_TRUE : DEF_FALSE);

   *p_err = USBH_ERR_NONE;
    return (p_hcd_ed->IsHalt);
}


/*
*********************************************************************************************************
*                                          OHCI_URB_Submit()
*
* Description : Create the TD for the URB, insert the TDs into the appropriate endpoint descriptor,
*               and enable the host controller to begin processing the lists
*
* Argument(s) : p_dev            Pointer to device structure
*
*               p_ep             Pointer to endpoint structure
*
*               p_urb            Pointer to URB structure
*
* Return(s)   : USBH_ERR_NONE           If successful
*               USBH_ERR_INVALID_ARGS   If p_dev, p_ep, or p_urb is 0
*               USBH_ERR_DMA_BUF_ALLOC  If insufficient memory for DMA buffer
*               USBH_ERR_HW_DESC_ALLOC  If insufficient memory for hardware descriptor
*
* Note(s)     : (1) Both the General TD and the Isochronous TD provide a means of specifying a buffer
*                   that is from 0 to 8,192 bytes long. Additionally, the data buffer described in a
*                   single TD can span up to two physically disjoint pages.
*********************************************************************************************************
*/

static  void  OHCI_URB_Submit (USBH_HC_DRV  *p_hc_drv,
                               USBH_URB     *p_urb,
                               USBH_ERR     *p_err)
{
    OHCI_DEV     *p_ohci;
    OHCI_HCD_TD  *p_hcd_td;
    CPU_INT32U    buf;
    CPU_INT32U    buf_len;
    CPU_INT32U    max_td_len;
    CPU_INT08U    dly_int;
    CPU_INT32U    td_toggle;
    CPU_INT32U    frame;
    CPU_INT32U    temp;
    CPU_INT32U    token;
    CPU_INT08U    ep_type;
    CPU_INT16U    ep_max_pkt_size;
    CPU_INT32U    buf_start;
    CPU_INT32U    offsets[4];
    CPU_INT32U    td_ix;
    CPU_INT32U    frm_ix;
    CPU_INT32U    off_ix;
    CPU_INT16U    nbr_tds;
    CPU_INT16U    nbr_hcd_tds_avail;
    CPU_INT16U    nbr_hc_tds_avail;
    CPU_INT32U    nbr_bytes;
    CPU_INT32U    td_ctrl;
    LIB_ERR       err_lib;
    USBH_HC_CFG  *p_hc_cfg;
    USBH_EP      *p_ep;
    CPU_SR_ALLOC();


    p_ohci     = (OHCI_DEV *)p_hc_drv->DataPtr;
    p_hc_cfg   = p_hc_drv->HC_CfgPtr;
    p_ep       = p_urb->EP_Ptr;
    max_td_len = 0u;
    dly_int    = 0u;
    td_toggle  = 0u;
    frame      = 0u;

    ep_type         = USBH_EP_TypeGet(p_ep);
    ep_max_pkt_size = USBH_EP_MaxPktSizeGet(p_ep);


    if ((p_hc_cfg->DedicatedMemAddr    != (CPU_ADDR)0) &&
        (p_hc_cfg->DataBufFromSysMemEn == DEF_DISABLED)) {

        if (p_urb->UserBufLen != 0u) {
            p_urb->DMA_BufPtr = Mem_PoolBlkGet((MEM_POOL  *)&p_ohci->BufPool,
                                               (CPU_SIZE_T ) p_hc_cfg->DataBufMaxLen,
                                               (LIB_ERR   *)&err_lib);
            if (err_lib != LIB_MEM_ERR_NONE) {
               *p_err = USBH_ERR_ALLOC;
                return;
            }

            p_urb->DMA_BufLen = DEF_MIN(p_urb->UserBufLen, p_hc_cfg->DataBufMaxLen);

            if ((p_urb->Token == USBH_TOKEN_OUT  ) ||
                (p_urb->Token == USBH_TOKEN_SETUP)) {

                Mem_Copy((void      *)p_urb->DMA_BufPtr,
                         (void      *)p_urb->UserBufPtr,
                         (CPU_SIZE_T )p_urb->DMA_BufLen);
            }
        }
    } else {
        p_urb->DMA_BufPtr = p_urb->UserBufPtr;
        p_urb->DMA_BufLen = p_urb->UserBufLen;
    }

    buf_len =  p_urb->DMA_BufLen;
    buf     = (p_urb->DMA_BufLen) ? (CPU_INT32U) VIR2BUS(p_urb->DMA_BufPtr) : 0u;


    nbr_tds = ((buf + buf_len) - (buf & 0xFFFFF000u)) / 0x2000u;  /* Calculate number of TDs needed for this xfer         */
    if ((((buf + buf_len) - (buf & 0xFFFFF000u)) % 0x2000u) != 0u) {
        nbr_tds++;
    }

    CPU_CRITICAL_ENTER();
    if (ep_type == USBH_EP_TYPE_ISOC) {

        if(p_urb->IsocDescPtr->StartFrm == 0u) {
           frame  =  Rd32(HcFrameNbr(p_hc_cfg->BaseAddr)) + 1u;   /* Start this xfer immediately after current frame nbr  */
        } else {
           frame  = p_urb->IsocDescPtr->StartFrm;               /* Start this xfer at the caller specified frame number */
        }

        frm_ix = 0u;
                                                                /* Init each iTD describing the Isochronous xfer        */
        for (td_ix = 0u; (td_ix < nbr_tds) && (frm_ix < p_urb->IsocDescPtr->NbrFrm); td_ix++) {
            buf_start     =  buf & 0xFFFFF000u;                 /* 4K aligned start address of the buffer               */
                                                                /* Number of bytes in this TD                           */
            nbr_bytes      = DEF_MIN(((buf_start + 0x2000u) - buf), buf_len);
            offsets[0] = 0u;
            offsets[1] = 0u;
            offsets[2] = 0u;
            offsets[3] = 0u;
                                                                /* Init each Isochronous data packet                    */
            for (off_ix = 0u; (off_ix < 8u) && (frm_ix < p_urb->IsocDescPtr->NbrFrm) && (nbr_bytes >= p_urb->IsocDescPtr->FrmLen[frm_ix]); off_ix++) {

                if ((off_ix % 2u) == 0u) {
                                                                /* Even offset. Write to lower word                     */
                    offsets[off_ix/2u] |= (buf & OHCI_ITD_OFFSET_4K_PAGE_BOUNDARY) | OHCI_ITD_PSWN_CC_NOT_ACCESSED;
                                                                /* if buffer address crosses the 4K-page boundary...    */
                    if ((buf - buf_start) > OHCI_ITD_OFFSET_4K_PAGE_BOUNDARY) {
                                                                /* .... Set bit 12 to use Upper 20 bits of Buffer End   */
                        offsets[off_ix/2u] |= OHCI_ITD_OFFSET_BIT12_LOWER_WORD;
                   }
                } else {
                                                                /* Odd offset Write to higher word                      */
                    offsets[off_ix/2u] |= ((buf & OHCI_ITD_OFFSET_4K_PAGE_BOUNDARY) | OHCI_ITD_PSWN_CC_NOT_ACCESSED) << 16;
                                                                /* if buffer address crosses the 4K-page boundary...    */
                    if ((buf - buf_start) > OHCI_ITD_OFFSET_4K_PAGE_BOUNDARY) {
                                                                /* Set Upperword bit 12 to use Upper 20 bits of Buffer End */
                        offsets[off_ix/2u] |= OHCI_ITD_OFFSET_BIT12_UPPER_WORD;
                    }
                }
                buf       += p_urb->IsocDescPtr->FrmLen[frm_ix];/* Increment the buffer to point to next frame          */
                nbr_bytes -= p_urb->IsocDescPtr->FrmLen[frm_ix];/* update remaining bytes                               */
                buf_len   -= p_urb->IsocDescPtr->FrmLen[frm_ix++];
            }
            if ((td_ix + 1u) == nbr_tds) {
                dly_int = 0u;                                   /* Enable interrupt on Last TD                          */
            } else {
                dly_int = 7u;                                   /* No interrupt for TDs in the middle                   */
            }
            frame &= 0xFFFFu;                                   /* Lower 16 bits of Frame number                        */

            td_ctrl  = 0u;
            td_ctrl  = OHCI_TD_CC;
            td_ctrl |= OHCI_TD_FC(off_ix - 1u);
            td_ctrl |= OHCI_TD_DELAY_INT(dly_int);
            td_ctrl |= OHCI_TD_SF(frame);

            p_hcd_td = OHCI_HC_TD_Insert(p_hc_drv,
                                         p_ep,
                                         p_urb,
                                         td_ctrl,
                                         buf_start,
                                         buf - 1u,
                                        &offsets[0]);
            if (p_hcd_td == (OHCI_HCD_TD *)0) {
                CPU_CRITICAL_EXIT();
               *p_err = USBH_ERR_ALLOC;
                return;
            }

            frame += off_ix;
        }
    } else {
                                                                /* TD for the Status Phase                              */

        if ((ep_type == USBH_EP_TYPE_CTRL) &&
            (buf_len ==                0u)) {
                                                                /* If xfer_dir IN  status TD is OUT                     */
            if (p_urb->Token == USBH_TOKEN_OUT) {
                token = OHCI_TD_DIR_OUT;
            } else {
                token = OHCI_TD_DIR_IN;
            }
                                                                /* If xfer_dir OUT  status TD is IN                     */
            p_hcd_td = OHCI_HC_TD_Insert(p_hc_drv,
                                         p_ep,
                                         p_urb,
                                        (token | OHCI_TD_CC | OHCI_TD_DELAY_INT(0) | OHCI_TD_TOGGLE_1),
                                         0u,
                                         0u,
                                         0);

            if (p_hcd_td == (OHCI_HCD_TD *)0) {
                CPU_CRITICAL_EXIT();
               *p_err = USBH_ERR_ALLOC;
                return;
            }
        }

        nbr_hcd_tds_avail = (CPU_INT16U)Mem_PoolBlkGetNbrAvail(&p_ohci->HCD_TDPool,
                                                               &err_lib);
        nbr_hc_tds_avail  = (CPU_INT16U)Mem_PoolBlkGetNbrAvail(&p_ohci->HC_TDPool,
                                                               &err_lib);

        if (err_lib != LIB_MEM_ERR_NONE) {
            CPU_CRITICAL_EXIT();
           *p_err = USBH_ERR_ALLOC;
            return;
        }

        if ((nbr_tds > nbr_hcd_tds_avail) ||
            (nbr_tds > nbr_hc_tds_avail)) {                     /* Make sure there are enough TDs avail.                */
            CPU_CRITICAL_EXIT();
           *p_err = USBH_ERR_ALLOC;
            return;
        }

        while (buf_len) {
                                                                /* Max TD Len is 8192 if the buffer is 4K aligned       */
            max_td_len = 0x2000u - (buf & 0x00000FFFu);

            if (max_td_len < buf_len) {
                max_td_len -= max_td_len % ep_max_pkt_size;
            } else {
                max_td_len = buf_len;
            }

            if (buf_len - max_td_len) {
                dly_int = 7u;                                   /* No interrupt for TDs in the middle                   */
            } else {
                dly_int = 0u;                                   /* Interrupt on last TD. No delay                       */
            }

            if (ep_type == USBH_EP_TYPE_CTRL) {
                if (p_urb->Token == USBH_TOKEN_SETUP) {
                    td_toggle = OHCI_TD_TOGGLE_0;
                } else {
                    td_toggle = OHCI_TD_TOGGLE_1;
                }
            } else {
                td_toggle = 0u;
            }

            if (p_urb->Token == USBH_TOKEN_SETUP) {
                token = OHCI_TD_DIR_SETUP;

            } else if (p_urb->Token == USBH_TOKEN_OUT) {
                token = OHCI_TD_DIR_OUT;
            } else {
                token = OHCI_TD_DIR_IN;
            }

            p_hcd_td = OHCI_HC_TD_Insert(p_hc_drv,              /* Insert TD into List                                  */
                                         p_ep,
                                         p_urb,
                                        (OHCI_TD_ROUNDING | token | OHCI_TD_CC | OHCI_TD_DELAY_INT(dly_int) | td_toggle),
                                         buf,
                                         buf + max_td_len - 1u,
                                         0);

            if (p_hcd_td == (OHCI_HCD_TD *)0) {
                CPU_CRITICAL_EXIT();
               *p_err = USBH_ERR_ALLOC;
                return;
            }
            buf_len -= max_td_len;                              /* update current buffer                                */
            buf     += max_td_len;


        }
    }
    CPU_CRITICAL_EXIT();

    switch (ep_type) {
        case USBH_EP_TYPE_CTRL:
             Wr32(HcCtrlHeadED(p_hc_cfg->BaseAddr),             /* Write control list addr to HcControlHeadEd           */
                  p_ohci->EDLists[OHCI_ED_LIST_CTRL]->DMA_HC_ED);
             Wr32(HcCmdStatus(p_hc_cfg->BaseAddr),              /* Enable Control List Filled                           */
                  Rd32(HcCmdStatus(p_hc_cfg->BaseAddr)) | OHCI_OR_CMD_STATUS_CLF);
             Wr32(HcCtrl(p_hc_cfg->BaseAddr),                   /* Enable Control List Processing.                      */
                  Rd32( HcCtrl(p_hc_cfg->BaseAddr))| OHCI_OR_CTRL_CLE);
             break;

        case USBH_EP_TYPE_BULK:
             Wr32(HcBulkHeadED(p_hc_cfg->BaseAddr),             /* Write control list addr to HcControlHeadEd           */
                  p_ohci->EDLists[OHCI_ED_LIST_BULK]->DMA_HC_ED);
             Wr32(HcCmdStatus(p_hc_cfg->BaseAddr),              /* Enable Bulk List Filled                              */
                  Rd32(HcCmdStatus(p_hc_cfg->BaseAddr))| OHCI_OR_CMD_STATUS_BLF);
             Wr32(HcCtrl(p_hc_cfg->BaseAddr),                   /* Enable Bulk List Processing                          */
                  Rd32( HcCtrl(p_hc_cfg->BaseAddr)) | OHCI_OR_CTRL_BLE);
             break;

        case USBH_EP_TYPE_INTR:
             Wr32(HcCtrl(p_hc_cfg->BaseAddr),                   /* Enable INTERRUPT list processing                     */
                  Rd32( HcCtrl(p_hc_cfg->BaseAddr)) | OHCI_OR_CTRL_PLE);
             break;

        case USBH_EP_TYPE_ISOC:                                 /* Enable ISOCH list processing                         */
             temp = Rd32(HcCtrl(p_hc_cfg->BaseAddr));

             if((temp & OHCI_OR_CTRL_PLE) != 0u) {              /* If Periodic List Schedule already enabled            */
                                                                /* Enable only processing of isochronous EDs            */
                Wr32(HcCtrl(p_hc_cfg->BaseAddr), temp | OHCI_OR_CTRL_IE);
             } else {
                                                                /* Enable Periodic List Schedule AND processing of ...  */
                                                                /* ...isochronous EDs                                   */
                 while ((temp & (OHCI_OR_CTRL_IE | OHCI_OR_CTRL_PLE)) == 0u) {

                     Wr32(HcCtrl(p_hc_cfg->BaseAddr),
                          temp | OHCI_OR_CTRL_IE | OHCI_OR_CTRL_PLE);
                     temp = Rd32( HcCtrl(p_hc_cfg->BaseAddr));
                 }
             }
             break;

        default:
             break;
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                         OHCI_URB_Complete()
*
* Description : Determine the number bytes transfered in this URB, remove the TD containing this URB,
*               set the transfer condition to successful or failed.
*
* Argument(s) : p_dev            Pointer to device structure.
*
*               p_ep             Pointer to endpoint structure.
*
*               p_urb            Pointer to URB structure.
*
* Return(s)   : USBH_ERR_NONE           If successful
*               USBH_ERR_INVALID_ARGS   If p_dev, p_ep, or p_urb is 0
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_URB_Complete (USBH_HC_DRV  *p_hc_drv,
                                 USBH_URB     *p_urb,
                                 USBH_ERR     *p_err)
{
    OHCI_HCD_TD     *p_temp_td;
    OHCI_HCD_TD     *p_del_td;
    OHCI_HC_TD      *p_hc_td;
    CPU_INT32U       buf_len;
    CPU_INT32U       actual_len;
    CPU_BOOLEAN      update_ed_head;
    CPU_INT32U       head;
    CPU_INT32S       cond_code;
    CPU_INT32S       completion_code;
    OHCI_DEV        *p_ohci;
    OHCI_HCD_ED     *p_hcd_ed;
    USBH_ISOC_DESC  *p_isoc_desc;
    CPU_BOOLEAN      is_halt;
    CPU_BOOLEAN      clr_halt;
    CPU_INT32U       frm_ix;
    CPU_INT08U       off_ix;
    CPU_INT32U       buf_end;
    CPU_INT08U       ep_type;
    CPU_INT08U       ep_dir;
    CPU_INT32U       i;
    LIB_ERR          err_lib;
    USBH_EP         *p_ep;
    USBH_HC_CFG     *p_hc_cfg;


    p_ohci          = (OHCI_DEV *)p_hc_drv->DataPtr;
    p_hc_cfg        = p_hc_drv->HC_CfgPtr;
    p_ep            = p_urb->EP_Ptr;
    update_ed_head  = DEF_FALSE;
    completion_code = 0;
    p_hcd_ed        = (OHCI_HCD_ED *)p_ep->ArgPtr;
    p_isoc_desc     = p_urb->IsocDescPtr;
    is_halt         = OHCI_IsHalt_EP(p_hc_drv, p_ep, p_err);
    clr_halt        = DEF_FALSE;
    ep_type         = USBH_EP_TypeGet(p_ep);
    ep_dir          = USBH_EP_DirGet(p_ep);
    frm_ix          = 0u;

    head = p_hcd_ed->HC_EDPtr->HeadTD & (~0xFu);                /* Phy address of current ed Head                       */
    if (p_hcd_ed->Head_HCD_TDPtr == p_hcd_ed->Tail_HCD_TDPtr) { /* TD list is Empty                                     */
       *p_err = USBH_ERR_NONE;
        return;
    }

    p_temp_td = p_hcd_ed->Head_HCD_TDPtr;                       /* Current Head Td in TD list                           */

    while ((p_temp_td         !=    0u) &&
           (p_temp_td->URBPtr != p_urb)) {                      /* Goto the TD containing the URB to be cancelled       */

        if (p_temp_td->DMA_HC_TD == head) {                     /* URB contains TDs beyond current ed head.             */
            update_ed_head = DEF_TRUE;                          /* Update ed Head                                       */
        }
        p_temp_td = p_temp_td->NextPtr;
    }

    buf_len         = p_urb->DMA_BufLen;
    actual_len      = 0u;
    completion_code = OHCI_CODE_NOTACCESSED;

    while (p_temp_td->URBPtr == p_urb) {

        if (p_temp_td->DMA_HC_TD == head) {                     /* URB contains TDs beyond current ed head.             */
            update_ed_head = DEF_TRUE;                          /* Update ed Head                                       */
        }
        p_hc_td   = p_temp_td->HC_TdPtr;
        cond_code = OHCI_TDMASK_CC(p_hc_td->Ctrl);              /* Condition code tells transfer success/fail           */
                                                                /* Determine the bytes transfered in DATA stage         */

        if (ep_type == USBH_EP_TYPE_ISOC) {
            for (off_ix = 0u; (off_ix < 8u) && (frm_ix < p_isoc_desc->NbrFrm); off_ix++) {
                if (p_hc_td->Offsets[off_ix / 2u] == 0u) {
                    break;
                }

                if ((off_ix % 2) == 0) {
                    completion_code               = (p_hc_td->Offsets[off_ix / 2] >> 12) & 0xFu;
                    p_isoc_desc->FrmLen[frm_ix++] =  p_hc_td->Offsets[off_ix / 2] & 0xFFFu;          /* Actual Length transfered   */
                } else {
                    completion_code               = (p_hc_td->Offsets[off_ix / 2] >> 28) & 0xFu;
                    p_isoc_desc->FrmLen[frm_ix++] = (p_hc_td->Offsets[off_ix / 2] >> 16) & 0x0FFFu;  /* Actual Length transfered   */
                }

                switch (completion_code) {
                    case OHCI_CODE_NOERROR:
                         p_isoc_desc->FrmErr[frm_ix] = USBH_ERR_NONE;
                         break;

                    case OHCI_CODE_CRC:
                    case OHCI_CODE_BITSTUFFING:
                    case OHCI_CODE_BUFFEROVERRUN:
                    case OHCI_CODE_BUFFERUNDERRUN:
                    case OHCI_CODE_DATATOGGLEMISMATCH:
                    case OHCI_CODE_PIDCHECKFAILURE:
                    case OHCI_CODE_UNEXPECTEDPID:
                    case OHCI_CODE_DATAOVERRUN:
                    case OHCI_CODE_DATAUNDERRUN:
                         p_isoc_desc->FrmErr[frm_ix] = USBH_ERR_HC_IO;
                         break;

                    case OHCI_CODE_STALL:
                         p_isoc_desc->FrmErr[frm_ix] = USBH_ERR_EP_STALL;
                         break;

                    case OHCI_CODE_DEVICENOTRESPONDING:
                         p_isoc_desc->FrmErr[frm_ix] = USBH_ERR_DEV_NOT_RESPONDING;
                         break;

                    default:
                         p_isoc_desc->FrmErr[frm_ix] = USBH_ERR_UNKNOWN;
                         break;
                }
            }
            actual_len = buf_len;

        } else if ((p_hc_td->BufEnd != 0u                   ) &&
                   (cond_code       != OHCI_CODE_NOTACCESSED)) {
                                                                /* currBufPtr != buf_end If last TD buffer is           */
                                                                /* ... not transfered completely                        */
            buf_end = p_hc_td->BufEnd;

            if ((p_hc_td->CurBuf != 0u) &&
                (p_hc_td->CurBuf != buf_end)) {
                actual_len = buf_len - (1u + (CPU_INT32U)buf_end - (CPU_INT32U)p_hc_td->CurBuf);
            } else {
                actual_len = buf_len;
            }

        }

        p_del_td = p_temp_td;                                   /* Free memory used by completed TDs                    */

        if (p_temp_td == p_hcd_ed->Head_HCD_TDPtr) {            /* Head TD? update head to point to next                */
            p_hcd_ed->Head_HCD_TDPtr = p_hcd_ed->Head_HCD_TDPtr->NextPtr;
        }
        p_temp_td = p_temp_td->NextPtr;                         /* Move to next TD                                      */
                                                                /* Free TD in the following cases :                     */
                                                                /* (1) TD is Completed, ISR called                      */
                                                                /* (2) (TD NOTACCESSED or DEVICENOTRESPONDING)          */
                                                                /* ... and ed is in HALT state                          */
        if ((p_del_td->State != OHCI_HCD_TD_STATE_COMPLETED) &&
            (is_halt == DEF_FALSE)) {
            OHCI_EPPause(p_hc_drv, p_ep);
            is_halt    = DEF_TRUE;
            clr_halt   = DEF_TRUE;
        }

        OHCI_HCD_TD_Destroy(p_hc_drv, p_del_td);
        if (cond_code != OHCI_CODE_NOTACCESSED) {
            completion_code = cond_code;
        }
    }

    if (update_ed_head && p_temp_td) {
        p_hcd_ed->HC_EDPtr->HeadTD = p_temp_td->DMA_HC_TD;
    }

    if ((ep_type == USBH_EP_TYPE_ISOC) &&
        (ep_dir  == USBH_EP_DIR_IN   )) {

        actual_len = 0u;
        for (i = 0u; i < p_isoc_desc->NbrFrm; i++) {
            actual_len += p_isoc_desc->FrmLen[i];
        }

    }

    p_urb->XferLen = actual_len;

    if (completion_code == OHCI_CODE_DATAUNDERRUN) {            /* Short packet is OK                                   */
        completion_code =  OHCI_CODE_NOERROR;
        clr_halt        =  DEF_TRUE;
    }

    if (clr_halt == DEF_TRUE) {
        OHCI_HaltEPClr(p_ep);
    }

    if (p_urb->Err == 0) {
        switch (completion_code) {
            case OHCI_CODE_NOERROR:
                 p_urb->Err = USBH_ERR_NONE;
                 break;

            case OHCI_CODE_CRC:
            case OHCI_CODE_BITSTUFFING:
            case OHCI_CODE_BUFFEROVERRUN:
            case OHCI_CODE_BUFFERUNDERRUN:
            case OHCI_CODE_DATATOGGLEMISMATCH:
            case OHCI_CODE_PIDCHECKFAILURE:
            case OHCI_CODE_UNEXPECTEDPID:
            case OHCI_CODE_DATAOVERRUN:
            case OHCI_CODE_DATAUNDERRUN:
                 p_urb->Err = USBH_ERR_HC_IO;
                 break;

            case OHCI_CODE_STALL:
                 p_urb->Err = USBH_ERR_EP_STALL;
                 break;

            case OHCI_CODE_DEVICENOTRESPONDING:
                 p_urb->Err = USBH_ERR_DEV_NOT_RESPONDING;
                 break;

            default:
                 p_urb->Err = USBH_ERR_UNKNOWN;
                 break;
        }
    }

    if ((p_hc_cfg->DedicatedMemAddr    != (CPU_ADDR)0) &&
        (p_hc_cfg->DataBufFromSysMemEn == DEF_DISABLED)) {

        if (p_urb->DMA_BufPtr != (void *)0) {

            if ((p_urb->Token   == USBH_TOKEN_IN) &&
                (p_urb->XferLen != 0u)) {
                Mem_Copy((void      *)p_urb->UserBufPtr,
                         (void      *)p_urb->DMA_BufPtr,
                         (CPU_SIZE_T )p_urb->XferLen);
            }

            Mem_PoolBlkFree((MEM_POOL *)&p_ohci->BufPool,
                            (void     *) p_urb->DMA_BufPtr,
                            (LIB_ERR  *)&err_lib);
        }
    }

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                           OHCI_URB_Abort()
*
* Description : Abort pending transfer.
*
* Argument(s) : p_dev            Pointer to device structure.
*
*               p_ep             Pointer to endpoint structure.
*
*               p_urb            Pointer to URB structure.
*
* Return(s)   : USBH_ERR_NONE           If successful
*               USBH_ERR_INVALID_ARGS   If p_dev, p_ep, or p_urb is 0
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_URB_Abort (USBH_HC_DRV  *p_hc_drv,
                              USBH_URB     *p_urb,
                              USBH_ERR     *p_err)
{
    USBH_EP  *p_ep;


    p_ep = p_urb->EP_Ptr;

    if (p_ep->ArgPtr == DEF_NULL) {                              /* EP was closed.                                       */
       *p_err = USBH_ERR_NONE;
        return;
    }

    OHCI_EPPause(p_hc_drv, p_ep);                               /* Pause ED, host cntrlr skips processing this ED.      */
    OHCI_URB_Complete(p_hc_drv, p_urb, p_err);
    OHCI_EPResume(p_hc_drv, p_ep);                              /* Resume processing this ED.                           */

   *p_err = USBH_ERR_NONE;
}


/*
*********************************************************************************************************
*                                             OHCI_ISR()
*
* Description : OHCI interrupt service routine.
*
* Argument(s) : p_arg           Pointer to OHCI_DEV structure.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_ISR (void  *p_arg)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_DRV  *p_hc_drv;
    USBH_HC_CFG  *p_hc_cfg;
    CPU_INT32U    int_status;
    CPU_INT32U    int_en;
    USBH_ERR      err;
    CPU_INT32U    hcca_frame_nbr;


    p_hc_drv    = (USBH_HC_DRV *)p_arg;
    p_ohci      = p_hc_drv->DataPtr;
    p_hc_cfg    = p_hc_drv->HC_CfgPtr;
    int_status  = Rd32(HcIntStatus(p_hc_cfg->BaseAddr));        /* Read Interrupt Status                                */
    int_en      = Rd32(HcIntEn(p_hc_cfg->BaseAddr));            /* Read Interrupt enable status                         */
    int_status &= int_en;

    if (int_status == 0u) {
        return;
    }

    if ((int_status & OHCI_OR_INT_STATUS_SO) != 0u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("# Schedule OverRun #\r\n");             /* TODO - Schedule overrun. Adjust BW                   */
#endif
    }

    if ((int_status & OHCI_OR_INT_STATUS_UE) != 0u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("# Unrecoverable Error #\r\n");
#endif
    }

    if ((int_status & OHCI_OR_INT_STATUS_RHSC) != 0u) {         /* Root hub status change interrupt                     */
        USBH_HUB_RH_Event(p_hc_drv->RH_DevPtr);
    }

    if ((int_status & OHCI_OR_INT_STATUS_WDH) != 0u) {          /* Handle Done TD                                       */
        err = OHCI_DoneHeadProcess(p_hc_drv);

        if (err != USBH_ERR_NONE) {
            Wr32( HcIntDis(p_hc_cfg->BaseAddr), OHCI_OR_INT_STATUS_WDH);
        }
    }

    if ((int_status & OHCI_OR_INT_EN_FNO) != 0u) {              /* Handle Frame Number Overflow                         */

        hcca_frame_nbr = p_ohci->DMA_OHCI.HCCAPtr->FrameNbr;

        if(DEF_BIT_IS_CLR(hcca_frame_nbr, DEF_BIT_15) == DEF_YES) {
            p_ohci->FrameNbr++;
        }
    }
                                                                /* TODO - Disable all lists.                            */
    Wr32(HcIntStatus(p_hc_cfg->BaseAddr), int_status);          /* Acknowledge interrupt                                */
}


/*
*********************************************************************************************************
*                                          OHCI_HCD_ED_Clr()
*
* Description : Initialize OHCI Driver Endpoint Descriptor.
*
* Argument(s) : p_hcd_ed         Pointer to host controller driver endpoint descriptor.
*
* Return(s)   : None
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_HCD_ED_Clr (OHCI_HCD_ED  *p_hcd_ed)
{
    p_hcd_ed->HC_EDPtr       = (OHCI_HC_ED  *)0;
    p_hcd_ed->DMA_HC_ED      = (CPU_INT32U   )0;
    p_hcd_ed->Head_HCD_TDPtr = (OHCI_HCD_TD *)0;
    p_hcd_ed->Tail_HCD_TDPtr = (OHCI_HCD_TD *)0;
    p_hcd_ed->NextPtr        = (OHCI_HCD_ED *)0;
    p_hcd_ed->ListInterval   = (CPU_INT32U   )0;
    p_hcd_ed->BW             = (CPU_INT32U   )0;
    p_hcd_ed->IsHalt         = (CPU_BOOLEAN  )DEF_FALSE;
}


/*
*********************************************************************************************************
*                                          OHCI_HC_ED_Clr()
*
* Description : Initialize host controller endpoint descriptor.
*
* Argument(s) : p_hc_ed          Pointer to host controller endpoint descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_HC_ED_Clr (OHCI_HC_ED  *p_hc_ed)
{
    p_hc_ed->Ctrl   = 0u;
    p_hc_ed->TailTD = 0u;
    p_hc_ed->HeadTD = 0u;
    p_hc_ed->Next   = 0u;
}


/*
*********************************************************************************************************
*                                        OHCI_HCD_ED_Create()
*
* Description : Create OHCI Driver Endpoint Descriptor.
*
* Argument(s) : p_ohci           OHCI Device structure pointer.
*
* Return(s)   : Pointer to OHCI_HCD_ED structure  If success
*
*               0                                 Otherwise
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  OHCI_HCD_ED  *OHCI_HCD_ED_Create (USBH_HC_DRV  *p_hc_drv)
{
    OHCI_HCD_ED  *p_new_hcd_ed;
    OHCI_HC_ED   *p_new_hc_ed;
    OHCI_DEV     *p_ohci;
    LIB_ERR       err_lib;


    p_ohci       = (OHCI_DEV    *)p_hc_drv->DataPtr;
    p_new_hcd_ed = (OHCI_HCD_ED *)Mem_PoolBlkGet((MEM_POOL  *)&p_ohci->HCD_EDPool,
                                                 (CPU_SIZE_T ) sizeof(OHCI_HCD_ED),
                                                 (LIB_ERR   *)&err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return ((OHCI_HCD_ED *)0);
    }

    OHCI_HCD_ED_Clr(p_new_hcd_ed);

    p_new_hc_ed = (OHCI_HC_ED *)Mem_PoolBlkGet((MEM_POOL  *)&p_ohci->HC_EDPool,
                                               (CPU_SIZE_T ) sizeof(OHCI_HC_ED),
                                               (LIB_ERR   *)&err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        Mem_PoolBlkFree((MEM_POOL *)&p_ohci->HCD_EDPool,
                        (void     *) p_new_hcd_ed,
                        (LIB_ERR  *)&err_lib);
        return ((OHCI_HCD_ED *)0);
    }

    OHCI_HC_ED_Clr(p_new_hc_ed);

    p_new_hcd_ed->NextPtr   = 0;
    p_new_hc_ed->Next       = 0u;
    p_new_hcd_ed->HC_EDPtr  = p_new_hc_ed;
    p_new_hcd_ed->DMA_HC_ED = (CPU_INT32U)VIR2BUS(p_new_hc_ed);
    p_new_hc_ed->Ctrl       = 0u;

    return (p_new_hcd_ed);
}


/*
*********************************************************************************************************
*                                        OHCI_HCD_ED_Destroy()
*
* Description : Destroy OHCI Driver Endpoint Descriptor.
*
* Argument(s) : p_ohci           OHCI Device structure pointer
*
*             : p_hcd_ed         Pointer to OHCI_HCD_ED structure
*
* Return(s)   : None
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_HCD_ED_Destroy (USBH_HC_DRV  *p_hc_drv,
                                   OHCI_HCD_ED  *p_hcd_ed)
{
    OHCI_DEV  *p_ohci;
    LIB_ERR    err_lib;


    p_ohci = (OHCI_DEV *)p_hc_drv->DataPtr;

    Mem_PoolBlkFree((MEM_POOL *)&p_ohci->HC_EDPool,
                    (void     *) p_hcd_ed->HC_EDPtr,
                    (LIB_ERR  *)&err_lib);

    Mem_PoolBlkFree((MEM_POOL *)&p_ohci->HCD_EDPool,
                    (void     *) p_hcd_ed,
                    (LIB_ERR  *)&err_lib);
}


/*
*********************************************************************************************************
*                                          OHCI_HCD_TD_Clr()
*
* Description : Initialize OHCI driver transfer descriptor
*
* Argument(s) : p_hcd_td         Pointer to OHCI_HCD_TD structure
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_HCD_TD_Clr (OHCI_HCD_TD  *p_hcd_td)
{
    p_hcd_td->HC_TdPtr  = (OHCI_HC_TD  *)0;
    p_hcd_td->DMA_HC_TD = (CPU_INT32U   )0;
    p_hcd_td->NextPtr   = (OHCI_HCD_TD *)0;
    p_hcd_td->URBPtr    = (USBH_URB    *)0;
}


/*
*********************************************************************************************************
*                                          OHCI_HC_TD_Clr()
*
* Description : Initialize OHCI transfer descriptor.
*
* Argument(s) : p_hc_td          Pointer to the OHCI_HC_TD structure
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_HC_TD_Clr (OHCI_HC_TD  *p_hc_td)
{
    CPU_INT08U  ix;


    p_hc_td->Ctrl    = 0u;
    p_hc_td->CurBuf  = 0u;
    p_hc_td->Next    = 0u;
    p_hc_td->BufEnd  = 0u;

    for (ix = 0u; ix < 4u; ix++) {
        p_hc_td->Offsets[ix] = 0u;
    }
}


/*
*********************************************************************************************************
*                                        OHCI_HCD_TD_Create()
*
* Description : Create Host controller driver Transfer Descriptor
*
* Argument(s) : p_ohci           OHCI Device structure pointer
*
* Return(s)   : Pointer to OHCI_HCD_TD structure  If success
*
*               0                                 Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  OHCI_HCD_TD  *OHCI_HCD_TD_Create (USBH_HC_DRV  *p_hc_drv)
{
    OHCI_HCD_TD  *p_new_hcd_td;
    OHCI_HC_TD   *p_new_hc_td;
    OHCI_DEV     *p_ohci;
    LIB_ERR       err_lib;


    p_ohci = (OHCI_DEV *)p_hc_drv->DataPtr;
                                                                /* Allocate HC DRIVER TD                                */
    p_new_hcd_td = (OHCI_HCD_TD *)Mem_PoolBlkGet((MEM_POOL  *)&p_ohci->HCD_TDPool,
                                                 (CPU_SIZE_T ) sizeof(OHCI_HCD_TD),
                                                 (LIB_ERR   *)&err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return ((OHCI_HCD_TD *)0);
    }

    OHCI_HCD_TD_Clr(p_new_hcd_td);
                                                                /* Allocate HC DMA TD                                   */
    p_new_hc_td = (OHCI_HC_TD *)Mem_PoolBlkGet((MEM_POOL  *)&p_ohci->HC_TDPool,
                                               (CPU_SIZE_T ) sizeof(OHCI_HC_TD),
                                               (LIB_ERR   *)&err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        Mem_PoolBlkFree(       &p_ohci->HCD_TDPool,
                        (void *)p_new_hcd_td,
                               &err_lib);
        return ((OHCI_HCD_TD *)0);
    }

    OHCI_HC_TD_Clr(p_new_hc_td);

    p_new_hcd_td->DMA_HC_TD = (CPU_INT32U)VIR2BUS(p_new_hc_td); /* HC TD phy address                                    */
    p_new_hc_td->Ctrl       = 0u;                               /* Initialize new HC TD                                 */
    p_new_hc_td->CurBuf     = 0u;
    p_new_hc_td->BufEnd     = 0u;
    p_new_hc_td->Next       = 0u;
    p_new_hcd_td->HC_TdPtr  = p_new_hc_td;
    p_new_hcd_td->NextPtr   = 0;

    OHCI_SetDataPtr(p_ohci,
                    p_new_hcd_td,
                    p_new_hc_td);

    return (p_new_hcd_td);
}


/*
*********************************************************************************************************
*                                        OHCI_HCD_TD_Destroy()
*
* Description : Destroy a Host controller driver Transfer Descriptor.
*
* Argument(s) : p_ohci           OHCI Device structure pointer.
*
*               p_hcd_td         OHCI Driver Transfer Descriptor.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_HCD_TD_Destroy (USBH_HC_DRV  *p_hc_drv,
                                   OHCI_HCD_TD  *p_hcd_td)
{
    OHCI_HC_TD  *p_hc_td;
    LIB_ERR      err_lib;
    OHCI_DEV    *p_ohci;


    p_ohci  = (OHCI_DEV *)p_hc_drv->DataPtr;
    p_hc_td = p_hcd_td->HC_TdPtr;                               /* First lookup general TD pool                         */

    Mem_PoolBlkFree(       &p_ohci->HC_TDPool,
                    (void *)p_hc_td,
                           &err_lib);

    Mem_PoolBlkFree(       &p_ohci->HCD_TDPool,                 /* Remove OHCI Driver TD                                */
                    (void *)p_hcd_td,
                           &err_lib);
}


/*
*********************************************************************************************************
*                                       OHCI_PeriodicListInit()
*
* Description : Initialize the periodic list corresponding to 32ms, 16ms, 8ms, 4ms, 2ms and 1ms intervals.
*
* Argument(s) : p_ohci           OHCI Device Structure Pointer.
*
* Return(s)   : USBH_ERR_NONE           If successful.
*               USBH_ERR_HW_DESC_ALLOC  If ED is not created due to insufficient memory
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  OHCI_PeriodicListInit (USBH_HC_DRV  *p_hc_drv)
{
    CPU_INT32U    list_ix;
    CPU_INT32U    ix_prev;
    OHCI_HCD_ED  *p_ed;
    OHCI_DEV     *p_ohci;


    p_ohci  = (OHCI_DEV *)p_hc_drv->DataPtr;
    ix_prev = 0u;

    for (list_ix = OHCI_ED_LIST_32MS; list_ix <= OHCI_ED_LIST_01MS; list_ix++) {
        p_ed = OHCI_HCD_ED_Create(p_hc_drv);                    /* Allocate driver ED.                                  */

        if (p_ed == (OHCI_HCD_ED *)0) {
            return (USBH_ERR_ALLOC);
        }

        p_ed->HC_EDPtr->Ctrl     = OHCI_ED_SKIP;
        p_ohci->EDLists[list_ix] = p_ed;
                                                                /* For each 16MS list.                                  */
        if ((list_ix >= OHCI_ED_LIST_16MS) && (list_ix < OHCI_ED_LIST_08MS)) {
            ix_prev = OHCI_ED_LIST_32MS + ((list_ix - OHCI_ED_LIST_16MS) * 2u);
            p_ohci->EDLists[list_ix]->ListInterval = 16u;
                                                                /* 32MS lists point to 16MS lists.                      */
            p_ohci->EDLists[OHCI_BranchArray[ix_prev]]->NextPtr             = p_ohci->EDLists[list_ix];
            p_ohci->EDLists[OHCI_BranchArray[ix_prev]]->HC_EDPtr->Next      = p_ohci->EDLists[list_ix]->DMA_HC_ED;
            p_ohci->EDLists[OHCI_BranchArray[ix_prev + 1u]]->NextPtr        = p_ohci->EDLists[list_ix];
            p_ohci->EDLists[OHCI_BranchArray[ix_prev + 1u]]->HC_EDPtr->Next = p_ohci->EDLists[list_ix]->DMA_HC_ED;


        } else if ((list_ix >= OHCI_ED_LIST_08MS) && (list_ix <= OHCI_ED_LIST_01MS)) {

            if ((list_ix >= OHCI_ED_LIST_08MS) &&
                (list_ix < (OHCI_ED_LIST_04MS))) {
                                                                /* 16MS list points to 8MS lists.                       */
                ix_prev = OHCI_ED_LIST_16MS + ((list_ix - OHCI_ED_LIST_08MS) * 2u);
                p_ohci->EDLists[list_ix]->ListInterval = 8u;

            } else if ((list_ix >= OHCI_ED_LIST_04MS) &&
                       (list_ix < OHCI_ED_LIST_02MS)) {
                                                                /* 8MS list points to 4MS lists.                        */
                ix_prev = OHCI_ED_LIST_08MS + ((list_ix - OHCI_ED_LIST_04MS) * 2u);
                p_ohci->EDLists[list_ix]->ListInterval = 4u;

            } else if ((list_ix >= OHCI_ED_LIST_02MS) &&
                       (list_ix < OHCI_ED_LIST_01MS)) {
                                                                /* 4MS list points to 2MS lists.                        */
                ix_prev = OHCI_ED_LIST_04MS + ((list_ix - OHCI_ED_LIST_02MS) * 2u);
                p_ohci->EDLists[list_ix]->ListInterval = 2u;

            } else if (list_ix == OHCI_ED_LIST_01MS) {
                                                                /* 2MS list points to 1MS lists.                        */
                ix_prev = OHCI_ED_LIST_02MS + ((list_ix - OHCI_ED_LIST_01MS) * 2u);
                p_ohci->EDLists[list_ix]->ListInterval = 1u;
            }

            p_ohci->EDLists[ix_prev]->NextPtr           = p_ohci->EDLists[list_ix];
            p_ohci->EDLists[ix_prev]->HC_EDPtr->Next    = p_ohci->EDLists[list_ix]->DMA_HC_ED;
            p_ohci->EDLists[ix_prev+1u]->NextPtr        = p_ohci->EDLists[list_ix];
            p_ohci->EDLists[ix_prev+1u]->HC_EDPtr->Next = p_ohci->EDLists[list_ix]->DMA_HC_ED;
        }

    }
                                                                /* Initialize HCCA interrupt table.                     */
    for (list_ix = OHCI_ED_LIST_32MS; list_ix < OHCI_ED_LIST_16MS; list_ix++) {
        p_ohci->DMA_OHCI.HCCAPtr->IntTbl[list_ix] = p_ohci->EDLists[OHCI_BranchArray[list_ix]]->DMA_HC_ED;
        p_ohci->EDLists[OHCI_BranchArray[list_ix]]->ListInterval = 32u;
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        OHCI_PeriodicEPOpen()
*
* Description : Insert the periodic endpoint descriptor into periodic list according to its interval
*
* Argument(s) : p_ohci           OHCI Device structure pointer
*
*               p_ed             OHCI Endpoint Descriptor pointer
*
*               interval         Periodic interval
*
* Return(s)   : USBH_ERR_NONE     If successful
*               USBH_ERR_NO_BW    If failed to allocate bandwidth
*
* Note(s)     : None
*********************************************************************************************************
*/

static  USBH_ERR  OHCI_PeriodicEPOpen (USBH_HC_DRV  *p_hc_drv,
                                       OHCI_HCD_ED  *p_ed,
                                       CPU_INT32U    interval)
{
    USBH_ERR      err;
    CPU_INT32U    sel_bw;
    CPU_INT32S    sel_ix;
    CPU_INT32S    sel_start_ix;
    CPU_INT32U    branch_max_bw;
    CPU_INT32U    branch_min_bw;
    CPU_INT32U    branch_start_ix;
    CPU_INT32U    ix;
    CPU_INT32S    sel_branch_ix;
    CPU_INT32U    n_branches;
    OHCI_HCD_ED  *p_lprev;
    OHCI_HCD_ED  *p_lnext;
    OHCI_DEV     *p_ohci;


    err          = USBH_ERR_NONE;
    sel_bw       = OHCI_MAX_PERIODIC_BW;
    sel_ix       = -1;
    sel_start_ix = -1;
    p_ohci       = (OHCI_DEV *)p_hc_drv->DataPtr;
                                                                /* Interval must be <= 32 and power of 2                */
    interval = (interval >= 32u) ? 32u :
               (interval >= 16u) ? 16u :
               (interval >=  8u) ? 8u  :
               (interval >=  4u) ? 4u  :
               (interval >=  2u) ? 2u  : 1u;

    p_ed->ListInterval = interval;
    n_branches         = 32u / interval;
                                                                /* Find the BW used by the Sub branches                 */
    for (branch_start_ix = 0u; branch_start_ix < 32u; branch_start_ix += n_branches) {
        branch_min_bw = p_ohci->Branch[branch_start_ix];
        branch_max_bw = p_ohci->Branch[branch_start_ix];
        sel_branch_ix = branch_start_ix;

        for (ix = 0u; ix < n_branches; ix++) {                  /* Find the maximum BW used by this group               */

            if (branch_max_bw < p_ohci->Branch[branch_start_ix + ix]) {
                branch_max_bw = p_ohci->Branch[branch_start_ix + ix];
            }
                                                                /* Minimum BW in the group                              */
            if (branch_min_bw > p_ohci->Branch[branch_start_ix + ix]) {
                branch_min_bw = p_ohci->Branch[branch_start_ix + ix];
                sel_branch_ix = branch_start_ix + ix;
            }
        }

        if (sel_bw > branch_max_bw) {                           /* Max BW in use in this group < the previous groups    */
            sel_bw       = branch_max_bw;
            sel_ix       = sel_branch_ix;
            sel_start_ix = branch_start_ix;
        }
    }


                                                                /* Is selected branch has enough BW for the ed          */
    if ((sel_ix   >= 0                            ) &&
        (p_ed->BW <  OHCI_MAX_PERIODIC_BW - sel_bw)) {
        p_lprev = p_lnext = p_ohci->EDLists[OHCI_ED_LIST_32MS + OHCI_BranchArray[sel_ix]];
                                                                /* Goto the last ed of this interval                    */
        while (p_lnext && (p_lnext->ListInterval >= interval)) {
            p_lprev = p_lnext;
            p_lnext = p_lnext->NextPtr;
        }

        if (p_lnext) {                                          /* Now insert ed next to the last ed in the interval    */
            p_ed->HC_EDPtr->Next = p_lnext->DMA_HC_ED;
            p_ed->NextPtr        = p_lnext;
        } else {
            p_ed->HC_EDPtr->Next = (CPU_INT32U   )0;
            p_ed->NextPtr        = (OHCI_HCD_ED *)0;
        }

        p_lprev->HC_EDPtr->Next = p_ed->DMA_HC_ED;              /* Make previous ed point to this ed                    */
        p_lprev->NextPtr        = p_ed;
                                                                /* Update the BW of branches in this sub group          */
        for (ix = sel_start_ix; ix < sel_start_ix + n_branches; ix++) {
            p_ohci->Branch[ix] += p_ed->BW;
        }

        err = USBH_ERR_NONE;
    } else {                                                    /* BW is not available, return ERROR                    */
        err = USBH_ERR_BW_NOT_AVAIL;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         OHCI_HC_TD_Insert()
*
* Description : Insert a transfer descriptor into corresponding OHCI Endpoint descriptor
*
* Argument(s) : p_ohci           Pointer to OHCI structure.
*
*               p_ep             Pointer to endpoint structure.
*
*               p_urb            Pointer to URB structure.
*
*               td_ctrl          Transfer descriptor's Control field.
*
*               buf_ptr          Current buffer pointer.
*
*               buf_end          End address of that buffer.
*
*               isoch_psw0       Isoch psw0.
*
* Return(s)   : Pointer to tailTD in the TD list  If success
*               0                                 If H/W descriptor allocation failed
*
* Note(s)     : (1) For further details, see section "5.2.8.1 The NULL or Empty Queue" of OHCI spec release 1.0a
*
*               (2) For further details, see section "5.2.8.4 Cancel" of OHCI spec release 1.0a
*
*               (3) For further details, see section "5.2.8.2 Adding to a Queue" of OHCI spec release 1.0a
*********************************************************************************************************
*/

static  OHCI_HCD_TD  *OHCI_HC_TD_Insert (USBH_HC_DRV  *p_hc_drv,
                                         USBH_EP      *p_ep,
                                         USBH_URB     *p_urb,
                                         CPU_INT32U    td_ctrl,
                                         CPU_INT32U    buf_ptr,
                                         CPU_INT32U    buf_end,
                                         CPU_INT32U   *p_offsets)
{
    OHCI_HCD_ED           *p_hcd_ed;
    volatile  OHCI_HC_ED  *p_hc_ed;
    OHCI_HCD_TD           *p_new_hcd_td;
    volatile  OHCI_HC_TD  *p_new_hc_td;
    OHCI_HCD_TD           *p_tail_hcd_td;
    CPU_INT08U             ep_type;
    CPU_BOOLEAN            EP_paused;
    CPU_BOOLEAN            is_halt;
    CPU_INT32U             cur_periodic_ED;
    CPU_INT32U             cur_async_ED;
    CPU_INT32U             hc_ed_addr;
    USBH_HC_CFG           *p_hc_cfg;
    USBH_ERR               err;


    EP_paused = DEF_FALSE;
    ep_type   = USBH_EP_TypeGet(p_ep);
    p_hcd_ed  = (OHCI_HCD_ED *)p_ep->ArgPtr;
    p_hc_ed   = p_hcd_ed->HC_EDPtr;
    p_hc_cfg  = p_hc_drv->HC_CfgPtr;

    if (p_hcd_ed == (OHCI_HCD_ED *)0) {
        return ((OHCI_HCD_TD *)0);
    }

    p_new_hcd_td = OHCI_HCD_TD_Create(p_hc_drv);                /* Create a TD struct for HCD                           */

    if(p_new_hcd_td == (OHCI_HCD_TD *)0) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("%s fails. OHCI_CreateHCTD return NULL\r\n",__FUNCTION__);
#endif
        return ((OHCI_HCD_TD *)0);
    }

    p_new_hc_td = p_new_hcd_td->HC_TdPtr;                       /* Get the created TD struct for Host Controller        */

                                                                /* (1)-------- NULL or Empty Queue. Note #1 ----------- */
    if (p_hcd_ed->Head_HCD_TDPtr == (OHCI_HCD_TD *)0) {         /* TD queue is empty, create the 1st place holder       */
                                                                /* HCD Head and Tail ptrs point to same TD struct       */
        p_hcd_ed->Head_HCD_TDPtr          = p_new_hcd_td;
        p_hcd_ed->Tail_HCD_TDPtr          = p_new_hcd_td;
        p_hcd_ed->Head_HCD_TDPtr->NextPtr = p_new_hcd_td;
                                                                /* HC Head and Tail ptrs point to same TD struct        */
        p_hc_ed->HeadTD                   = p_new_hcd_td->DMA_HC_TD;
        p_hc_ed->TailTD                   = p_new_hcd_td->DMA_HC_TD;
        p_new_hc_td->Next                 = p_new_hcd_td->DMA_HC_TD;

        return ((OHCI_HCD_TD *)0);                              /* The first TD is a dummy TD                           */
    }

                                                                /* (2)----------- Cancel operation. Note #2 ----------- */
    is_halt = OHCI_IsHalt_EP(p_hc_drv, p_ep, &err);

    if ((ep_type == USBH_EP_TYPE_ISOC) ||
        (ep_type == USBH_EP_TYPE_INTR)) {
        cur_periodic_ED = Rd32(HcPeriodCurED(p_hc_cfg->BaseAddr));
        cur_periodic_ED = (CPU_INT32U) BUS2VIR((void *)cur_periodic_ED);
    } else if (ep_type == USBH_EP_TYPE_CTRL) {
        cur_async_ED = Rd32(HcCtrlCurED(p_hc_cfg->BaseAddr));
        cur_async_ED = (CPU_INT32U) BUS2VIR((void *)cur_async_ED);
    } else if (ep_type == USBH_EP_TYPE_BULK) {
        cur_async_ED = Rd32(HcBulkCurED(p_hc_cfg->BaseAddr));
        cur_async_ED = (CPU_INT32U) BUS2VIR((void *)cur_async_ED);
    }

    hc_ed_addr  = (CPU_INT32U) p_hc_ed;
    hc_ed_addr &= OHCI_OR_BULK_CUR_ED_BCED;
                                                                /* Is Endpoint Halted due to an error in processing TD? */
    if ((is_halt          == DEF_FALSE                                  )  &&
        ((cur_periodic_ED == hc_ed_addr) || (cur_async_ED == hc_ed_addr))) {

        OHCI_EPPause(p_hc_drv, p_ep);                           /* NO. So pause the endpoint processing by the HC       */
        EP_paused = DEF_TRUE;
    }
                                                                /* (3)----------- Adding operation. Note #3 ----------- */
                                                                /*  Current tail pointer                                */
    p_tail_hcd_td                     = p_hcd_ed->Tail_HCD_TDPtr;
    p_hcd_ed->Tail_HCD_TDPtr->NextPtr = p_new_hcd_td;           /* Insert new TD place holder at the tail               */
    p_hcd_ed->Tail_HCD_TDPtr          = p_new_hcd_td;
    p_tail_hcd_td->HC_TdPtr->Ctrl     = td_ctrl;                /* Prepare TD with the parameters passed in             */
    p_tail_hcd_td->HC_TdPtr->CurBuf   = buf_ptr;
    p_tail_hcd_td->HC_TdPtr->BufEnd   = buf_end;
    p_tail_hcd_td->URBPtr             = p_urb;                  /* Store the URB associated with this TD                */

    if (ep_type == USBH_EP_TYPE_ISOC) {
        p_tail_hcd_td->HC_TdPtr->Offsets[0] = p_offsets[0];
        p_tail_hcd_td->HC_TdPtr->Offsets[1] = p_offsets[1];
        p_tail_hcd_td->HC_TdPtr->Offsets[2] = p_offsets[2];
        p_tail_hcd_td->HC_TdPtr->Offsets[3] = p_offsets[3];
    }
                                                                /* Insert new HC TD at the tail                         */
    p_tail_hcd_td->HC_TdPtr->Next = p_new_hcd_td->DMA_HC_TD;
    p_hc_ed->TailTD               = p_new_hcd_td->DMA_HC_TD;

                                                                /* (4)----------- Cancel operation. Note #2 ----------- */
    if (EP_paused == DEF_TRUE) {                                /* If EP paused                                         */
        OHCI_EPResume(p_hc_drv, p_ep);                          /* Resume the endpoint processing by the HC             */
        EP_paused = DEF_FALSE;
    }

    return (p_tail_hcd_td);
}


/*
*********************************************************************************************************
*                                           OHCI_EPPause()
*
* Description : Halt process of endpoint descriptors
*
* Argument(s) : p_ohci            Pointer to OHCI Device structure
*
*               p_ep              Pointer to endpoint structure
*
* Return(s)   : USBH_ERR_NONE      If successful
*             : USBH_ERR_UNKNOWN   If Frame number doesn't increment
*
* Note(s)     : None
*********************************************************************************************************
*/

static  USBH_ERR  OHCI_EPPause (USBH_HC_DRV  *p_hc_drv,
                                USBH_EP      *p_ep)
{
    CPU_INT32U    cur_frame_nbr;
    CPU_INT32U    cnt;
    CPU_BOOLEAN   b_ret;
    USBH_ERR      err;
    USBH_HC_CFG  *p_hc_cfg;


    cnt      = 0u;
    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    b_ret    = OHCI_IsHalt_EP(p_hc_drv, p_ep, &err);
    if (b_ret == DEF_TRUE) {
        return (USBH_ERR_NONE);
    }

    OHCI_EPHalt(p_ep);                                          /* HALT this ed.                                        */
                                                                /* OHCI stops processing this ed from next frame        */
    cur_frame_nbr = Rd32(HcFrameNbr(p_hc_cfg->BaseAddr));       /* Get the current frame number                         */
                                                                /* Wait until current frame is completed                */
    while (Rd32(HcFrameNbr(p_hc_cfg->BaseAddr)) == cur_frame_nbr) {
        ++cnt;
        if (cnt > 3u) {
            break;
        }
        USBH_OS_DlyMS(1u);
    }
    if (cnt > 3u) {                                             /* Frame number not advanced !!!.                       */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("Failed to abort ep.\r\n");
#endif
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                           OHCI_EPResume()
*
* Description : Resume endpoint operation
*
* Argument(s) : p_ohci           Pointer to OHCI Device structure
*
*               p_ep             Pointer to endpoint structure
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_EPResume (USBH_HC_DRV  *p_hc_drv,
                             USBH_EP      *p_ep)
{
    (void)p_hc_drv;

    OHCI_HaltEPClr(p_ep);                                       /* Clear Halt ep                                        */
}


/*
*********************************************************************************************************
*                                            OHCI_EPHalt()
*
* Description : Halt endpoint operation
*
* Argument(s) : phost           Pointer to host structure
*
*               p_ep            Pointer to endpoint structure
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  OHCI_EPHalt (USBH_EP  *p_ep)
{
    OHCI_HCD_ED  *p_hcd_ed;
    OHCI_HC_ED   *p_hc_ed;


    p_hcd_ed          = (OHCI_HCD_ED *)p_ep->ArgPtr;
    p_hc_ed           = p_hcd_ed->HC_EDPtr;
    p_hc_ed->Ctrl    |= OHCI_ED_SKIP;                           /* HC skips this EP & continues to the next ED          */
    p_hcd_ed->IsHalt  = (((p_hc_ed->HeadTD & OHCI_ED_HALT) != 0u) ? DEF_TRUE : DEF_FALSE);
}


/*
*********************************************************************************************************
*                                          OHCI_HaltEPClr()
*
* Description : Clear endpoint halt condition
*
* Argument(s) : p_host           Pointer to host structure
*
*               p_ep             Pointer to endpoint structure
*
* Return(s)   : None
*
* Note(s)     : None
*
*********************************************************************************************************
*/

static  void  OHCI_HaltEPClr (USBH_EP  *p_ep)
{
    OHCI_HCD_ED  *p_hcd_ed;
    OHCI_HC_ED   *p_hc_ed;


    p_hcd_ed          = (OHCI_HCD_ED *)p_ep->ArgPtr;
    p_hc_ed           =  p_hcd_ed->HC_EDPtr;
    p_hc_ed->Ctrl    &= (~OHCI_ED_SKIP);
    p_hc_ed->HeadTD  &= (~OHCI_ED_HALT);
    p_hcd_ed->IsHalt  = (((p_hc_ed->HeadTD & OHCI_ED_HALT) != 0u) ? DEF_TRUE : DEF_FALSE);
}


/*
*********************************************************************************************************
*                                       OHCI_DoneHeadProcess()
*
* Description : Read the done head and traverse the done td list
*
* Argument(s) : p_ohci           Pointer to OHCI Device structure
*
* Return(s)   : USBH_ERR_NONE
*
* Note(s)     : None
*********************************************************************************************************
*/

static  USBH_ERR  OHCI_DoneHeadProcess (USBH_HC_DRV  *p_hc_drv)
{
    OHCI_HC_TD   *p_head_td;                                    /* Head of done TD list pointed by HCCA doneHead        */
    OHCI_HCD_TD  *p_head_hcd_td;                                /* Head of done TD list pointed by HCCA doneHead        */
    CPU_INT32U    dma_head;                                     /* Head of done TD list pointed by HCCA doneHead        */
    CPU_INT32U    dma_rev_head;                                 /* Reverse Head of TD List                              */
    CPU_INT32U    dma_head_next;                                /* Reverse Head of TD List                              */
    USBH_URB     *p_urb;
    USBH_URB     *p_prev_urb;
    CPU_INT32S    cond_code;
    OHCI_DEV     *p_ohci;


    p_urb         = (USBH_URB *)0;
    p_prev_urb    = (USBH_URB *)0;
    dma_rev_head  = 0u;
    dma_head_next = 0u;
    p_ohci        = (OHCI_DEV *)p_hc_drv->DataPtr;
                                                                /* Done Head. First 24 bits address                     */
    dma_head      = p_ohci->DMA_OHCI.HCCAPtr->DoneHead & (~0x0Fu);
                                                                /* Traverse done TD queue in reverse order              */
    while (dma_head) {
        p_head_td       = (OHCI_HC_TD *)BUS2VIR((void *)dma_head);
        dma_head_next   = p_head_td->Next;
        p_head_td->Next = dma_rev_head;
        dma_rev_head    = dma_head;
        dma_head        = dma_head_next;
    }
                                                                /* Traverse the list in correct order                   */
    dma_head = dma_rev_head;
    while (dma_head) {
        p_head_td     = (OHCI_HC_TD *)BUS2VIR((void *)dma_head);
        dma_head_next = p_head_td->Next;
        dma_head      = dma_head_next;
                                                                /* Determine TD pool the head_td belongs to             */
                                                                /* First lookup general TD pool                         */
        p_head_hcd_td  = (OHCI_HCD_TD *)OHCI_GetDataPtr(p_ohci, p_head_td);

        if (p_head_hcd_td == (OHCI_HCD_TD *)0) {                /* Lookup isoch TD pool                                 */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("# OHCI_GetDataPtr return NULL #\r\n");
#endif
            continue;
        }
        cond_code = OHCI_TDMASK_CC(p_head_td->Ctrl);            /* Get Condition code                                   */
                                                                /* Is TD cancelled  ?                                   */

        if (p_head_hcd_td->State == OHCI_HCD_TD_STATE_CANCELLED) {
            OHCI_HCD_TD_Destroy(p_hc_drv, p_head_hcd_td);

        } else {
            p_head_hcd_td->State = OHCI_HCD_TD_STATE_COMPLETED;
            p_urb                = p_head_hcd_td->URBPtr;

            if (cond_code) {
                OHCI_EPHalt(p_urb->EP_Ptr);
            }

            if (p_urb != p_prev_urb) {                          /* URB may be present in more than one TD               */
                p_prev_urb = p_urb;
                USBH_URB_Done(p_urb);                           /* Notify I/O task to complete URB                      */
            }
        }
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                           OHCI_DMA_Init()
*
* Description : Allocate all structures used by the OHCI driver
*
* Argument(s) : p_ohci           Pointer to OHCI Device structure
*
* Return(s)   : USBH_ERR_NONE   If success
*               USBH_ERR_ALLOC  If fails due to insufficient memory
*
* Note(s)     : (1) The Host Controller Communications Area (HCCA) is a 256-byte structure of system memory
*                   that is used by HCD to send and receive specific control and status information to and
*                   from the Host Controller.
*                   This structure MUST be located on a 256-byte boundary.
*********************************************************************************************************
*/

static  USBH_ERR  OHCI_DMA_Init (USBH_HC_DRV  *p_hc_drv)
{
    CPU_SIZE_T    octets_reqd;
    LIB_ERR       err_lib;
    CPU_INT32U    max_nbr_ed;
    CPU_INT32U    nbr_td_per_xfer;
    CPU_INT32U    max_nbr_td;
    CPU_INT32U    max_data_buf;
    CPU_INT08U   *p_dedicated_mem;
    CPU_INT32U    total_mem_req;
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg  = p_hc_drv->HC_CfgPtr;
    p_ohci    = (OHCI_DEV *)p_hc_drv->DataPtr;

    nbr_td_per_xfer = (p_hc_cfg->DataBufMaxLen / (4u * 1024u));
    if ((p_hc_cfg->DataBufMaxLen % (4u * 1024u)) != 0u) {
        nbr_td_per_xfer++;
    }

    max_nbr_ed      = OHCI_ED_LIST_SIZE + p_hc_cfg->MaxNbrEP_BulkOpen + p_hc_cfg->MaxNbrEP_IntrOpen + p_hc_cfg->MaxNbrEP_IsocOpen;
                                                                /* Each ED contains 2 TD (1 dummy + 1 functional).      */
    max_nbr_td      = ((nbr_td_per_xfer + 1u) * (p_hc_cfg->MaxNbrEP_BulkOpen + p_hc_cfg->MaxNbrEP_IntrOpen + p_hc_cfg->MaxNbrEP_IsocOpen)) + 4u;
    max_data_buf    = p_hc_cfg->MaxNbrEP_BulkOpen + p_hc_cfg->MaxNbrEP_IntrOpen + p_hc_cfg->MaxNbrEP_IsocOpen + 2u;

                                                                /* ---------------- DEDICATED MEMORY ------------------ */
    if (p_hc_cfg->DedicatedMemAddr != (CPU_ADDR)0) {            /* Allocate HCCA, EDs and TDs from a dedicated memory   */

        if (p_hc_cfg->DataBufFromSysMemEn == DEF_DISABLED) {    /* Data buffers allocated from dedicated memory         */

            total_mem_req = (sizeof(OHCI_HCCA) + 256u)        +
                            (max_nbr_td * sizeof(OHCI_HC_TD)) +
                            (max_nbr_ed * sizeof(OHCI_HC_ED)) +
                            (p_hc_cfg->DataBufMaxLen * max_data_buf);

        } else {                                                /* Data buffers allocated from main memory              */

            total_mem_req = (sizeof(OHCI_HCCA) + 256u)        +
                            (max_nbr_td * sizeof(OHCI_HC_TD)) +
                            (max_nbr_ed * sizeof(OHCI_HC_ED));
        }

        if (total_mem_req > p_hc_cfg->DedicatedMemSize) {
            return (USBH_ERR_ALLOC);
        }

        p_dedicated_mem = (CPU_INT08U *)p_hc_cfg->DedicatedMemAddr;
                                                                /* See Note #1.                                         */
        p_ohci->DMA_OHCI.HCCAPtr = (OHCI_HCCA  *)USB_ALIGNED(p_dedicated_mem, 256u);
        p_dedicated_mem         += sizeof(OHCI_HCCA) + 256u;
        p_ohci->DMA_OHCI.TDPtr   = (OHCI_HC_TD *)p_dedicated_mem;
        p_dedicated_mem         += (max_nbr_td + 1u) * sizeof(OHCI_HC_TD);
        p_ohci->DMA_OHCI.EDPtr   = (OHCI_HC_ED *)p_dedicated_mem;
        p_dedicated_mem         += (max_nbr_ed + 1u) * sizeof(OHCI_HC_ED);
        p_ohci->DMA_OHCI.BufPtr  = (CPU_INT08U *)p_dedicated_mem;

        Mem_PoolCreate ((MEM_POOL   *)&p_ohci->HC_EDPool,       /* Pool of EDs ...                                      */
                        (void       *) p_ohci->DMA_OHCI.EDPtr,  /*... allocated from DEDICATED memory                   */
                        (CPU_SIZE_T  ) (max_nbr_ed + 1u) * 16u,
                        (CPU_SIZE_T  ) max_nbr_ed,
                        (CPU_SIZE_T  ) sizeof(OHCI_HC_ED),
                        (CPU_SIZE_T  ) 16u,
                        (CPU_SIZE_T *)&octets_reqd,
                        (LIB_ERR    *)&err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (USBH_ERR_ALLOC);
        }


        Mem_PoolCreate ((MEM_POOL   *)&p_ohci->HC_TDPool,        /* Pool of TDs ...                                     */
                        (void       *) p_ohci->DMA_OHCI.TDPtr,   /*... allocated from DEDICATED memory                  */
                        (CPU_SIZE_T  ) (max_nbr_td + 1u) * 32u,
                        (CPU_SIZE_T  ) max_nbr_td,
                        (CPU_SIZE_T  ) sizeof(OHCI_HC_TD),
                        (CPU_SIZE_T  ) 32u,
                        (CPU_SIZE_T *)&octets_reqd,
                        (LIB_ERR    *)&err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (USBH_ERR_ALLOC);
        }
                                                                /* ---------------- SYSTEM MEMORY --------------------- */
    } else {                                                    /* Allocate HCCA, EDs and TDs from the system memory    */


                                                                /* Get a single block from the heap for HCCA            */
        p_dedicated_mem = (CPU_INT08U *)Mem_HeapAlloc((CPU_SIZE_T  )(sizeof(OHCI_HCCA) + 256u),
                                                      (CPU_SIZE_T  ) sizeof(CPU_ALIGN),
                                                      (CPU_SIZE_T *)&octets_reqd,
                                                      (LIB_ERR    *)&err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (USBH_ERR_ALLOC);
        }
                                                                /* See Note #1.                                         */
        p_ohci->DMA_OHCI.HCCAPtr = (OHCI_HCCA  *)USB_ALIGNED(p_dedicated_mem, 256u);

        Mem_PoolCreate ((MEM_POOL   *)&p_ohci->HC_EDPool,       /* Pool of EDs ...                                      */
                        (void       *) 0,                       /*... allocated from the HEAP (SYSTEM memory)           */
                        (CPU_SIZE_T  ) (max_nbr_ed + 1u) * 16u,
                        (CPU_SIZE_T  ) max_nbr_ed,
                        (CPU_SIZE_T  ) sizeof(OHCI_HC_ED),
                        (CPU_SIZE_T  ) 16u,
                        (CPU_SIZE_T *)&octets_reqd,
                        (LIB_ERR    *)&err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (USBH_ERR_ALLOC);
        }

        p_ohci->DMA_OHCI.EDPtr   = (OHCI_HC_ED *)p_ohci->HC_EDPool.PoolAddrStart;

        Mem_PoolCreate ((MEM_POOL   *)&p_ohci->HC_TDPool,       /* Pool of TDs ...                                      */
                        (void       *) 0,                       /*... allocated from the HEAP (SYSTEM memory)           */
                        (CPU_SIZE_T  ) (max_nbr_td + 1u) * 32u,
                        (CPU_SIZE_T  ) max_nbr_td,
                        (CPU_SIZE_T  ) sizeof(OHCI_HC_TD),
                        (CPU_SIZE_T  ) 32u,
                        (CPU_SIZE_T *)&octets_reqd,
                        (LIB_ERR    *)&err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (USBH_ERR_ALLOC);
        }

        p_ohci->DMA_OHCI.TDPtr = (OHCI_HC_TD *)p_ohci->HC_TDPool.PoolAddrStart;
    }

    p_ohci->OHCI_Lookup = (OHCI_HCD_TD **)Mem_HeapAlloc((CPU_SIZE_T  )(sizeof(void *) * (max_nbr_td + 1u)),
                                                        (CPU_SIZE_T  ) sizeof(CPU_ALIGN),
                                                        (CPU_SIZE_T *)&octets_reqd,
                                                        (LIB_ERR    *)&err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }
                                                                /* Initialize the HCCA structure                        */
    Mem_Set((void      *)p_ohci->DMA_OHCI.HCCAPtr,
            (CPU_INT08U )0,
            (CPU_SIZE_T )sizeof(OHCI_HCCA));

    Mem_PoolCreate ((MEM_POOL   *)&p_ohci->HCD_EDPool,          /* POOL for managing HCDED structures  ...              */
                    (void       *) 0,                           /*... allocated from the HEAP (SYSTEM memory)           */
                    (CPU_SIZE_T  ) (max_nbr_ed + 1u) * sizeof(OHCI_HCD_ED),
                    (CPU_SIZE_T  ) max_nbr_ed,
                    (CPU_SIZE_T  ) sizeof(OHCI_HCD_ED),
                    (CPU_SIZE_T  ) sizeof(CPU_ALIGN),
                    (CPU_SIZE_T *)&octets_reqd,
                    (LIB_ERR    *)&err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    Mem_PoolCreate ((MEM_POOL   *)&p_ohci->HCD_TDPool,          /* POOL for managing HCDTD structures  ...              */
                    (void       *) 0,                           /*... allocated from the HEAP (SYSTEM memory)           */
                    (CPU_SIZE_T  ) (max_nbr_td + 1u) * sizeof(OHCI_HCD_TD),
                    (CPU_SIZE_T  ) max_nbr_td,
                    (CPU_SIZE_T  ) sizeof(OHCI_HCD_TD),
                    (CPU_SIZE_T  ) sizeof(CPU_ALIGN),
                    (CPU_SIZE_T *)&octets_reqd,
                    (LIB_ERR    *)&err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }

    if ((p_hc_cfg->DedicatedMemAddr    != (CPU_ADDR)0) &&
        (p_hc_cfg->DataBufFromSysMemEn == DEF_DISABLED)) {

        Mem_PoolCreate ((MEM_POOL   *)&p_ohci->BufPool,         /* POOL for managing HCDTD structures                   */
                        (void       *) p_ohci->DMA_OHCI.BufPtr, /*... allocated from DEDICATED memory                   */
                        (CPU_SIZE_T  ) p_hc_cfg->DataBufMaxLen * max_data_buf,
                        (CPU_SIZE_T  ) max_data_buf,
                        (CPU_SIZE_T  ) p_hc_cfg->DataBufMaxLen,
                        (CPU_SIZE_T  ) sizeof(CPU_ALIGN),
                        (CPU_SIZE_T *)&octets_reqd,
                        (LIB_ERR    *)&err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
            return (USBH_ERR_ALLOC);
        }
    }

    return (USBH_ERR_NONE);
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
*                                        OHCI_PortStatusGet()
*
* Description : Retrieve port status changes and port status.
*
* Argument(s) : p_ohci                   Pointer to OHCI Device structure
*
*               port_nbr                 Port Number
*
*               port_status              Pointer to the port Status structure
*
* Return(s)   : USBH_ERR_NONE               If successful.
*               USBH_ERR_INVALID_PORT_NBR   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortStatusGet (USBH_HC_DRV           *p_hc_drv,
                                         CPU_INT08U             port_nbr,
                                         USBH_HUB_PORT_STATUS  *p_port_status)
{
    OHCI_DEV     *p_ohci;
    CPU_INT32U    status;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    status                     = Rd32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1));
                                                                /* bit16 to bit 31 indicate port status change          */
    p_port_status->wPortChange = (status & 0xFFFF0000u) >> 16u;
    p_port_status->wPortChange =  MEM_VAL_GET_INT16U_LITTLE(&p_port_status->wPortChange);
                                                                /* bit0 to bit15 indicate port status                   */
    p_port_status->wPortStatus = (status & 0xFFFFu);
    p_port_status->wPortStatus =  MEM_VAL_GET_INT16U_LITTLE(&p_port_status->wPortStatus);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          OHCI_HubDescGet()
*
* Description : Return root hub descriptor.
*
* Argument(s) : p_ohci           Pointer to OHCI Device structure.
*
*               p_buf            Pointer to buffer to read descriptor.
*
*               buf_len          Length of buffer in bytes.
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_HubDescGet (USBH_HC_DRV  *p_hc_drv,
                                      void         *p_buf,
                                      CPU_INT08U    buf_len)
{
    OHCI_DEV       *p_ohci;
    CPU_INT32U      hc_rh_desc_a;
    CPU_INT08U      port_always_powered;
    USBH_HUB_DESC   hub_desc;
    USBH_HC_CFG    *p_hc_cfg;


    p_hc_cfg     = p_hc_drv->HC_CfgPtr;
    p_ohci       = (OHCI_DEV *)p_hc_drv->DataPtr;
    hc_rh_desc_a = Rd32(HcRhDescA(p_hc_cfg->BaseAddr));         /* Determine nbr of ports present in HC                 */
                                                                /* Port Power switching.                                */
    if ((Rd32(HcRhDescA(p_hc_cfg->BaseAddr)) & OHCI_OR_RH_DA_NPS) != 0u) {
        port_always_powered = 1u;
    } else {
        port_always_powered = 0u;
    }

    Mem_Clr(&hub_desc, sizeof(USBH_HUB_DESC));

    hub_desc.bDescLength         =  USBH_HUB_LEN_HUB_DESC;
    hub_desc.bDescriptorType     =  USBH_HUB_DESC_TYPE_HUB;
    hub_desc.bNbrPorts           =  hc_rh_desc_a & OHCI_OR_RH_DA_NDP;
    hub_desc.wHubCharacteristics = (port_always_powered & 0x3u);
    hub_desc.wHubCharacteristics =  MEM_VAL_GET_INT16U_LITTLE(&hub_desc.wHubCharacteristics);

                                                                /* Power on to power good time                          */
    hub_desc.bPwrOn2PwrGood       = (Rd32(HcRhDescA(p_hc_cfg->BaseAddr)) & OHCI_OR_RH_DA_POTPGT) >> 24;
    hub_desc.bHubContrCurrent     = 0u;

                                                                /* Write the structure in USB format                    */
    USBH_HUB_FmtHubDesc(&hub_desc, (void *)p_ohci->OHCI_HubBuf);

    if (buf_len > sizeof(USBH_HUB_DESC)) {
        buf_len = sizeof(USBH_HUB_DESC);
    }

    Mem_Copy((void      *)p_buf,
             (void      *)p_ohci->OHCI_HubBuf,
             (CPU_SIZE_T )buf_len);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          OHCI_PortEnSet()
*
* Description : Enable the given port.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortEnSet (USBH_HC_DRV  *p_hc_drv,
                                     CPU_INT08U    port_nbr)
{
    OHCI_DEV  *p_ohci;


    p_ohci = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr == 0               ) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          OHCI_PortEnClr()
*
* Description : Clear port enable status.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortEnClr (USBH_HC_DRV  *p_hc_drv,
                                     CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),      /* Clear Port enable                                    */
         OHCI_OR_RH_PORT_CCS);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        OHCI_PortEnChngClr()
*
* Description : Clear port enable status change.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortEnChngClr (USBH_HC_DRV  *p_hc_drv,
                                         CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),      /* Clear Port reset status change                       */
         OHCI_OR_RH_PORT_PESC);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          OHCI_PortPwrSet()
*
* Description : Set port power based on port power mode.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortPwrSet (USBH_HC_DRV  *p_hc_drv,
                                      CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    CPU_INT08U    pwr_mode;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    pwr_mode = OHCI_PortPwrModeGet(p_hc_drv, port_nbr);         /* Determine port power mode.                           */

    if (pwr_mode == OHCI_PORT_PWRD_GLOBAL) {                    /* Set global power.                                    */
        Wr32(HcRhStatus(p_hc_cfg->BaseAddr), OHCI_OR_RH_STATUS_LPSC);
    } else if (pwr_mode == OHCI_PORT_PWRD_INDIVIDUAL) {         /* Set port power.                                      */
        Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),
             OHCI_OR_RH_PORT_PPS);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          OHCI_PortPwrClr()
*
* Description : Clear port power.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortPwrClr (USBH_HC_DRV  *p_hc_drv,
                                      CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    CPU_INT08U    pwr_mode;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    pwr_mode = OHCI_PortPwrModeGet(p_hc_drv, port_nbr);         /* Determine port power mode.                           */

    if (pwr_mode == OHCI_PORT_PWRD_GLOBAL) {                    /* Clear global power.                                  */
        Wr32(HcRhStatus(p_hc_cfg->BaseAddr),
             OHCI_OR_RH_STATUS_LPS);
    } else if (pwr_mode == OHCI_PORT_PWRD_INDIVIDUAL) {         /* Clear port power.                                    */
        Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),
             OHCI_OR_RH_PORT_LSDA);
    }

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         OHCI_PortResetSet()
*
* Description : Reset the given port.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortResetSet (USBH_HC_DRV  *p_hc_drv,
                                        CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),      /* Start port reset operation.                          */
         OHCI_OR_RH_PORT_PRS);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       OHCI_PortResetChngClr()
*
* Description : Clear port reset status change.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortResetChngClr (USBH_HC_DRV  *p_hc_drv,
                                            CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),      /* Clear Port reset status change                       */
         OHCI_OR_RH_PORT_PRSC);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        OHCI_PortSuspendClr()
*
* Description : Resume the given port if the port is suspended.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortSuspendClr (USBH_HC_DRV  *p_hc_drv,
                                          CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),      /* Start port reset operation.                          */
         OHCI_OR_RH_PORT_POCI);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                       OHCI_PortConnChngClr()
*
* Description : Clear port connect status change.
*
* Argument(s) : p_ohci                     Pointer to OHCI Device structure
*
*               port_nbr                   Port Number
*
* Return(s)   : DEF_OK,     If successful.
*               DEF_FAIL,   If invalid port number.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_PortConnChngClr (USBH_HC_DRV  *p_hc_drv,
                                           CPU_INT08U    port_nbr)
{
    OHCI_DEV     *p_ohci;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;
    p_ohci   = (OHCI_DEV *)p_hc_drv->DataPtr;

    if ((port_nbr ==                0) ||                       /* Port number starts from 1                            */
        (port_nbr >  p_ohci->NbrPorts)) {
        return (DEF_FAIL);
    }

    Wr32(HcRhPortStatus(p_hc_cfg->BaseAddr, port_nbr - 1),      /* Clear Port connection status change                  */
         OHCI_OR_RH_PORT_CSC);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          OHCI_RHSC_IntEn()
*
* Description : Enable root hub interrupt for the given OHCI controller
*
* Argument(s) : p_hc_dev  Pointer to OHCI device structure
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_RHSC_IntEn (USBH_HC_DRV  *p_hc_drv)
{
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;

    Wr32(HcIntEn(p_hc_cfg->BaseAddr), OHCI_OR_INT_EN_RHSC);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                          OHCI_RHSC_IntDis()
*
* Description : Disable root hub interrupt for the given OHCI controller
*
* Argument(s) : p_hc_dev  Pointer to OHCI device structure
*
* Return(s)   : DEF_OK.
*
* Note(s)     : None
*********************************************************************************************************
*/

static  CPU_BOOLEAN  OHCI_RHSC_IntDis (USBH_HC_DRV  *p_hc_drv)
{
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;

    Wr32(HcIntDis(p_hc_cfg->BaseAddr), OHCI_OR_INT_DISABLE_RHSC);

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        OHCI_PortPwrModeGet()
*
* Description : Retrieve whether the given port is individually powered, globally powered or always powered
*
* Argument(s) : p_ohci                   Pointer to OHCI Device structure
*
*               port_nbr                 Port Number
*
* Return(s)   : PORT_ALWAYS_POWERED      Port is always powered. No switiching
*               PORT_INDIVIDUAL_POWERED  Port is individually powered
*               PORT_GLOBAL_POWERED      Port is global powered
*
* Note(s)     : None
*********************************************************************************************************
*/

static  CPU_INT08S  OHCI_PortPwrModeGet (USBH_HC_DRV  *p_hc_drv,
                                         CPU_INT08U    port_nbr)
{
    CPU_INT16U    ppc_mask;
    USBH_HC_CFG  *p_hc_cfg;


    p_hc_cfg = p_hc_drv->HC_CfgPtr;

    if ((Rd32(HcRhDescA(p_hc_cfg->BaseAddr)) & OHCI_OR_RH_DA_NPS) != 0u) {
        return (OHCI_PORT_PWRD_ALWAYS);                         /* Ports are always powered.                            */
    }

    if ((Rd32(HcRhDescA(p_hc_cfg->BaseAddr)) & OHCI_OR_RH_DA_PSM) != 0u) {
                                                                /* Power switching.                                     */
        ppc_mask = (Rd32(HcRhDescB(p_hc_cfg->BaseAddr)) & OHCI_OR_RH_DB_PPCM) >> 16;

        if ((ppc_mask & (1 << (port_nbr - 1))) != 0u) {
            return (OHCI_PORT_PWRD_INDIVIDUAL);                 /* Port is controlled individualy.                      */
        }
    }

    return (OHCI_PORT_PWRD_GLOBAL);                             /* Port is global powered.                              */
}


/*
*********************************************************************************************************
*                                          OHCI_SetDataPtr()
*
* Description : !!!! description to add
*
* Argument(s) : p_ohci           Pointer to OHCI Device structure.
*
*               p_buf            Pointer to buffer to read descriptor.
*
*               buf_len          Length of buffer in bytes.
*
* Return(s)   : USBH_ERR_NONE.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  OHCI_SetDataPtr (OHCI_DEV     *p_ohci,
                               OHCI_HCD_TD  *p_hcd_td,
                               OHCI_HC_TD   *p_hc_td)
{
    CPU_INT32U  ix;


    ix = (p_hc_td - p_ohci->DMA_OHCI.TDPtr) % sizeof(OHCI_HC_TD);

    p_ohci->OHCI_Lookup[ix] = p_hcd_td;

    return;
}

/*
*********************************************************************************************************
*                                          OHCI_GetDataPtr()
*
* Description : !!!! description to add
*
* Argument(s) : p_ohci           Pointer to OHCI Device structure.
*
*               p_buf            Pointer to buffer to read descriptor.
*
*               buf_len          Length of buffer in bytes.
*
* Return(s)   : USBH_ERR_NONE.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  OHCI_HCD_TD  *OHCI_GetDataPtr (OHCI_DEV    *p_ohci,
                                       OHCI_HC_TD  *p_hc_td)
{
    CPU_INT32U  ix;


    ix = (p_hc_td - p_ohci->DMA_OHCI.TDPtr) % sizeof(OHCI_HC_TD);

    return ((OHCI_HCD_TD *)p_ohci->OHCI_Lookup[ix]);
}

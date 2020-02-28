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
*                                        STANDARD OHCI DRIVER
*
* Filename : usbh_ohci.h
* Version  : V3.42.00
*********************************************************************************************************
*/

#ifndef  USBH_OHCI_MODULE_PRESENT
#define  USBH_OHCI_MODULE_PRESENT


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../Source/usbh_core.h"


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/

#ifdef   USBH_OHCI_MODULE
#define  USBH_OHCI_EXT
#else
#define  USBH_OHCI_EXT  extern
#endif


/*
*********************************************************************************************************
*                                                 DEFINES
*********************************************************************************************************
*/

                                                                /* ----------- OHCI ED LIST NUMBER DEFINES ------------ */
#define  OHCI_ED_LIST_32MS                                 0u
#define  OHCI_ED_LIST_16MS                                32u
#define  OHCI_ED_LIST_08MS                                48u
#define  OHCI_ED_LIST_04MS                                56u
#define  OHCI_ED_LIST_02MS                                60u
#define  OHCI_ED_LIST_01MS                                62u
#define  OHCI_ED_LIST_CTRL                                63u
#define  OHCI_ED_LIST_BULK                                64u
#define  OHCI_ED_LIST_SIZE           (OHCI_ED_LIST_BULK + 1u)

#define  OHCI_MAX_PERIODIC_BW                             90u


/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/

typedef  struct  ohci_hcd_td  OHCI_HCD_TD;
typedef  struct  ohci_hcd_ed  OHCI_HCD_ED;

typedef  struct  ohci_oper_reg {
    CPU_REG32  Revision;
    CPU_REG32  Control;
    CPU_REG32  CommandStatus;
    CPU_REG32  InterruptStatus;
    CPU_REG32  InterruptEnable;
    CPU_REG32  InterruptDisable;
    CPU_REG32  HCCA;
    CPU_REG32  PeriodCurrentED;
    CPU_REG32  ControlHeadED;
    CPU_REG32  ControlCurrentED;
    CPU_REG32  BulkHeadED;
    CPU_REG32  BulkCurrentED;
    CPU_REG32  DoneHead;
    CPU_REG32  FmInterval;
    CPU_REG32  FmRemaining;
    CPU_REG32  FmNumber;
    CPU_REG32  PeriodicStart;
    CPU_REG32  LSThreshold;
    CPU_REG32  RhDescriptorA;
    CPU_REG32  RhDescriptorB;
    CPU_REG32  RhStatus;
    CPU_REG32  RhPortStatus[1];
} OHCI_OPER_REG;
                                                                /* --- HOST CONTROLLER COMMUNICATION AREA DATA TYPE --- */
typedef  struct  ohci_hcca {
    CPU_REG32  IntTbl[32];                                      /* Interrupt Table                                      */
    CPU_REG32  FrameNbr;                                        /* Frame Number                                         */
    CPU_REG32  DoneHead;                                        /* Done Head                                            */
    CPU_REG08  Rsvd[116];                                       /* Reserved for future use                              */
    CPU_REG08  Unknown[4];                                      /* Unused                                               */
} OHCI_HCCA;

                                                                /* --- HOST CONTROLLER ENDPOINT DESCRIPTOR DATA TYPE -- */
typedef  struct  ohci_hc_ed {
    CPU_REG32  Ctrl;                                            /* Endpoint descriptor control                          */
    CPU_REG32  TailTD;                                          /* Physical address of tail in Transfer descriptor list */
    CPU_REG32  HeadTD;                                          /* Physcial address of head in Transfer descriptor list */
    CPU_REG32  Next;                                            /* Physical address of next Endpoint descriptor         */
} OHCI_HC_ED;

                                                                /* --- HOST CONTROLLER TRANSFER DESCRIPTOR DATA TYPE -- */
typedef  struct  ohci_hc_td {
    CPU_REG32  Ctrl;                                            /* Transfer descriptor control                          */
    CPU_REG32  CurBuf;                                          /* Physical address of current buffer pointer           */
    CPU_REG32  Next;                                            /* Physical pointer to next Transfer Descriptor         */
    CPU_REG32  BufEnd;                                          /* Physical address of end of buffer                    */
    CPU_REG32  Offsets[4];                                      /* Isochronous offsets                                  */
} OHCI_HC_TD;

                                                                /* --- HOST CONTROLLER ENDPOINT TRANSFER DESCRIPTOR --- */
struct  ohci_hcd_ed {
    OHCI_HC_ED   *HC_EDPtr;
    CPU_INT32U    DMA_HC_ED;
    OHCI_HCD_TD  *Head_HCD_TDPtr;
    OHCI_HCD_TD  *Tail_HCD_TDPtr;
    OHCI_HCD_ED  *NextPtr;
    CPU_INT32U    ListInterval;
    CPU_INT32U    BW;
    CPU_BOOLEAN   IsHalt;
};

                                                                /* HOST CONTROLLER DRIVER TRANSFER DESCRIPTOR DATA TYPE */
struct  ohci_hcd_td {
    OHCI_HC_TD   *HC_TdPtr;
    CPU_INT32U    DMA_HC_TD;
    OHCI_HCD_TD  *NextPtr;
    USBH_URB     *URBPtr;                                       /* URB associated with this TD                          */
    CPU_REG08     State;
};

                                                                /* -------------- OHCI DMA Structure ------------------ */
typedef  struct  ohci_dma {
    OHCI_HCCA        *HCCAPtr;                                  /* DMA memory HCCA descriptor                           */
    OHCI_HC_TD       *TDPtr;                                    /* DMA memory CTRL,BULK and INTR endpoint TD            */
    OHCI_HC_ED       *EDPtr;                                    /* DMA memory for Endpoint Descriptors (ED)             */
    CPU_INT08U       *BufPtr;
} OHCI_DMA;

                                                                /* -------------- OHCI DEVICE DATA TYPE --------------- */
typedef  struct  ohci_dev {
    OHCI_DMA         DMA_OHCI;
    CPU_INT08U       OHCI_HubBuf[sizeof(USBH_HUB_DESC)];
    OHCI_HCD_ED     *EDLists[OHCI_ED_LIST_SIZE];                /* OHCI EndPoint Descriptor Lists                       */
    CPU_INT08U       CanWakeUp;                                 /* Port Power                                           */
    CPU_INT08U       NbrPorts;                                  /* Number of Ports in RootHub                           */
    CPU_INT32U       TotBW;                                     /* Periodic list parameters                             */
    CPU_INT32U       Branch[32];                                /* Bandwidth of branches                                */
    MEM_POOL         HC_EDPool;                                 /* Memory pool for allocating DMA ednpoint descriptor   */
    MEM_POOL         HC_TDPool;                                 /* Memory pool for allocating DMA general TDs           */
    OHCI_HCD_ED     *HCD_ED;
    MEM_POOL         HCD_EDPool;                                /* Memory pool for allocating OHCI driver EDs           */
    OHCI_HCD_TD     *HCD_TD;
    MEM_POOL         HCD_TDPool;                                /* Memory pool for allocating OHCI driver TDs           */
    MEM_POOL         BufPool;
    CPU_INT32U       FrameNbr;
    OHCI_HCD_TD    **OHCI_Lookup;
} OHCI_DEV;


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_OHCI_EXT  USBH_HC_DRV_API  OHCI_DrvAPI;
USBH_OHCI_EXT  USBH_HC_RH_API   OHCI_RH_API;


/*
*********************************************************************************************************
*                                                 MACROS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE END
*********************************************************************************************************
*/

#endif

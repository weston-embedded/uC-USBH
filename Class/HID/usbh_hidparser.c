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
*                                 HUMAN INTERFACE DEVICE CLASS PARSER
*
* Filename : usbh_hidparser.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_HIDPARSER_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_hid.h"
#include  "usbh_hidparser.h"
#include  "../../Source/usbh_core.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_HID_ITEM_SIZE(x)                (((x) & 0x03u) == 3u  ? 4u : ((x) & 0x03u))

#define  USBH_HID_ITEM_TYPE(x)               ((((x) & 0x0Cu) >> 2u) & 0x03u)

#define  USBH_HID_ITEM_TAG(x)                ((((x) & 0xF0u) >> 4u) & 0x0Fu)


#define  USBH_HID_SIGNED_DATA(p_parser)      (((p_parser)->Size == 1u) ? p_parser->Data.S08 : \
                                              ((p_parser)->Size == 2u) ? p_parser->Data.S16 : \
                                              ((p_parser)->Size == 4u) ? p_parser->Data.S32 : 0u)

#define  USBH_HID_UNSIGNED_DATA(p_parser)    (((p_parser)->Size == 1u) ? p_parser->Data.U08 : \
                                              ((p_parser)->Size == 2u) ? p_parser->Data.U16 : \
                                              ((p_parser)->Size == 4u) ? p_parser->Data.U32 : 0u)

                                                                /* ----------------- UNDEFINED VALUES ----------------- */
#define  USBH_HID_USAGEPAGE_UNDEFINED             0x00000000u
#define  USBH_HID_LOG_MIN_UNDEFINED               0x7FFFFFFFu
#define  USBH_HID_LOG_MAX_UNDEFINED               0x80000000u
#define  USBH_HID_PHY_MIN_UNDEFINED               0x7FFFFFFFu
#define  USBH_HID_PHY_MAX_UNDEFINED               0x80000000u
#define  USBH_HID_UNIT_EXP_UNDEFINED              0x7FFFFFFFu
#define  USBH_HID_USAGE_MIN_UNDEFINED             0xFFFFFFFFu
#define  USBH_HID_USAGE_MAX_UNDEFINED             0xFFFFFFFFu
#define  USBH_HID_UNIT_UNDEFINED                  0xFFFFFFFFu
#define  USBH_HID_REPORT_SIZE_UNDEFINED                    0u
#define  USBH_HID_REPORT_CNT_UNDEFINED                     0u


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

                                                                /* -------------- GLOBAL ITEM DATA TYPE --------------- */
typedef  struct  usbh_hid_global  USBH_HID_GLOBAL;

struct  usbh_hid_global {
    CPU_INT16U        UsagePage;                                /* Usage Page.                                          */
    CPU_INT08U        ReportID;                                 /* Report ID.                                           */
    CPU_INT32S        LogMin;                                   /* Logical Minimun.                                     */
    CPU_INT32S        LogMax;                                   /* Logical Maximum.                                     */
    CPU_INT32S        PhyMin;                                   /* Physical Minimum.                                    */
    CPU_INT32S        PhyMax;                                   /* Physical Maximum.                                    */
    CPU_INT32S        UnitExp;                                  /* Unit exponent.                                       */
    CPU_INT32S        Unit;                                     /* Unit.                                                */
    CPU_INT32U        ReportSize;                               /* Report Size (bits).                                  */
    CPU_INT32U        ReportCnt;                                /* Report Count.                                        */
    USBH_HID_GLOBAL  *NextPtr;                                  /* Pointer to next global this for global stack.        */
};

                                                                /* --------------- LOCAL ITEM DATA TYPE --------------- */
typedef  struct  usbh_hid_local {
    CPU_INT32U  Usage[USBH_HID_CFG_MAX_NBR_USAGE];              /* Usage array.                                         */
    CPU_INT32U  NbrUsage;                                       /* Number of usages.                                    */
    CPU_INT32U  UsageMin;                                       /* Usage Minimum.                                       */
    CPU_INT32U  UsageMax;                                       /* Usage Maximum.                                       */
} USBH_HID_LOCAL;

                                                                /* ---------------------- PARSER ---------------------- */
typedef  struct  usbh_hid_parser {
    USBH_HID_LOCAL    Local;                                    /* Local items.                                         */
    USBH_HID_GLOBAL   Global;                                   /* Global items.                                        */
    USBH_HID_GLOBAL  *GlobalStkPtr;                             /* Global items stack for push and pop.                 */
    USBH_HID_COLL    *CollStkPtr;                               /* Collection stack.                                    */
    CPU_INT08U        AppCollNbr;                               /* Present Apllication Collection number.               */
    CPU_INT08U        Type;                                     /* Item Type.                                           */
    CPU_INT08U        Tag;                                      /* Item Tag.                                            */
    CPU_INT08U        Size;                                     /* Item Size.                                           */
    union {
        CPU_INT08U    U08;
        CPU_INT08S    S08;
        CPU_INT16U    U16;
        CPU_INT16S    S16;
        CPU_INT32U    U32;
        CPU_INT32S    S32;
    }                 Data;                                     /* Item Data.                                           */
} USBH_HID_PARSER;


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

static  MEM_POOL         USBH_HID_CollPool;
static  USBH_HID_COLL    USBH_HID_CollList[USBH_HID_CFG_MAX_COLL];

static  MEM_POOL         USBH_HID_GlobalItemPool;
static  USBH_HID_GLOBAL  USBH_HID_GlobalItemList[USBH_HID_CFG_MAX_GLOBAL];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void      USBH_HID_InitGlobalItem(USBH_HID_GLOBAL      *p_global);

static  void      USBH_HID_InitLocalItem (USBH_HID_LOCAL       *p_local);

static  void      USBH_HID_InitItemParser(USBH_HID_PARSER      *p_parser);

static  void      USBH_HID_FreeItemParser(USBH_HID_PARSER      *p_parser);

static  USBH_ERR  USBH_HID_ParseMain     (USBH_HID_DEV         *p_hid_dev,
                                          USBH_HID_PARSER      *p_parser);

static  USBH_ERR  USBH_HID_ParseGlobal   (USBH_HID_PARSER      *p_parser);

static  USBH_ERR  USBH_HID_ParseLocal    (USBH_HID_PARSER      *p_parser);

static  USBH_ERR  USBH_HID_OpenColl      (USBH_HID_DEV         *p_hid_dev,
                                          USBH_HID_PARSER      *p_parser);

static  USBH_ERR  USBH_HID_CloseColl     (USBH_HID_PARSER      *p_parser);

static  USBH_ERR  USBH_HID_AddReport     (USBH_HID_DEV         *p_hid_dev,
                                          USBH_HID_PARSER      *p_parser);

static  USBH_ERR  USBH_HID_ValidateReport(USBH_HID_REPORT_FMT  *p_report_fmt);

static  void      USBH_HID_InitReport    (USBH_HID_PARSER      *p_parser,
                                          USBH_HID_REPORT_FMT  *p_report_fmt);

static  void      USBH_HID_SetData       (USBH_HID_PARSER      *p_parser,
                                          void                 *p_buf);


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
*                                     USBH_HID_ParserGlobalInit()
*
* Description : Initialize global data.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE,      if initialization is succesful.
*               USBH_ERR_ALLOC,     if global item or collection list allocation failed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_ParserGlobalInit (void)
{
    LIB_ERR     err_lib;
    CPU_SIZE_T  octets_reqd;


    Mem_PoolCreate (       &USBH_HID_CollPool,                  /* POOL for managing collection structures.             */
                    (void *)USBH_HID_CollList,
                           (sizeof(USBH_HID_COLL) * USBH_HID_CFG_MAX_COLL),
                            USBH_HID_CFG_MAX_COLL,
                            sizeof(USBH_HID_COLL),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("%d octets required\r\n", octets_reqd);
#endif
        return (USBH_ERR_ALLOC);
    }

    Mem_PoolCreate (       &USBH_HID_GlobalItemPool,            /* POOL for managing global item structures.            */
                    (void *)USBH_HID_GlobalItemList,
                           (sizeof(USBH_HID_GLOBAL) * USBH_HID_CFG_MAX_GLOBAL),
                            USBH_HID_CFG_MAX_GLOBAL,
                            sizeof(USBH_HID_GLOBAL),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("%d octets required\r\n", octets_reqd);
#endif
        return (USBH_ERR_ALLOC);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HID_ItemParser()
*
* Description : Parse report descriptor.
*
* Argument(s) : p_hid_dev           Pointer to HID device.
*
*               p_report_desc       Pointer to report descriptor buffer.
*
*               desc_len            Length of report descriptor buffer in octets.
*
* Return(s)   : USBH_ERR_NONE,                      if report successfully parsed.
*               USBH_ERR_UNKNOWN,                   if unknown error occurred.
*               USBH_ERR_HID_ITEM_LONG,             if long item present (not supported).
*               USBH_ERR_HID_ITEM_UNKNOWN,          if unknown item present.
*               USBH_ERR_HID_MISMATCH_COLL,         if there is a collection mismatch.
*               USBH_ERR_HID_MISMATCH_PUSH_POP,     if global stack error occured.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_ItemParser (USBH_HID_DEV  *p_hid_dev,
                               CPU_INT08U    *p_report_desc,
                               CPU_INT32U     desc_len)
{
    USBH_HID_PARSER  parser;
    CPU_INT32U       pos;
    CPU_INT08U       item;
    USBH_ERR         err;


                                                                /* ----------------- INITIALIZE DATA ------------------ */
    USBH_HID_InitItemParser(&parser);
    pos = 0u;
    err = USBH_ERR_UNKNOWN;

                                                                /* ------------- PARSE REPORT DESCRIPTOR -------------- */
    while (pos < desc_len) {
        item         = p_report_desc[pos++];
        parser.Size  = USBH_HID_ITEM_SIZE(item);
        parser.Type  = USBH_HID_ITEM_TYPE(item);
        parser.Tag   = USBH_HID_ITEM_TAG(item);
        USBH_HID_SetData(&parser, (void *)&p_report_desc[pos]);
        pos         += parser.Size;

        if (item == USBH_HID_ITEM_LONG) {                       /* If item is a long item ...                           */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("Long Item found\r\n");
#endif
            err = USBH_ERR_HID_ITEM_LONG;                       /* ... rtn err.                                         */
            break;
        }

        switch (parser.Type) {
            case USBH_HID_ITEM_TYPE_MAIN:                       /* Parse main item.                                     */
                 err = USBH_HID_ParseMain(p_hid_dev, &parser);
                 break;

            case USBH_HID_ITEM_TYPE_GLOBAL:                     /* Parse global item.                                   */
                 err = USBH_HID_ParseGlobal(&parser);
                 break;

            case USBH_HID_ITEM_TYPE_LOCAL:                      /* Parse local item.                                    */
                 err = USBH_HID_ParseLocal(&parser);

                 if (pos <= 6u) {                               /* Store device usage.                                  */
                     p_hid_dev->Usage = parser.Local.Usage[0u];
                 }
                 break;

            default:
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("Unknown item type\r\n");
#endif
                 err = USBH_ERR_HID_ITEM_UNKNOWN;
                 break;
        }

        if (err != USBH_ERR_NONE) {
            USBH_PRINT_ERR(err);
            break;
        }
    }

                                                                /* --------------- HANDLE PARSER ERRORS --------------- */
    if (err != USBH_ERR_NONE) {                                 /* Parsing err.                                         */
        USBH_HID_FreeItemParser(&parser);

    } else if (parser.CollStkPtr != (USBH_HID_COLL *)0) {       /* Collection stack err.                                */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("Missmatch in collections desc_len = %d\r\n",desc_len);
#endif
        USBH_HID_FreeItemParser(&parser);
        err = USBH_ERR_HID_MISMATCH_COLL;

    } else if (parser.GlobalStkPtr != (USBH_HID_GLOBAL *)0) {   /* Global stack err.                                    */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("Missmatch in push and pop items\r\n");
#endif
        USBH_HID_FreeItemParser(&parser);
        err = USBH_ERR_HID_MISMATCH_PUSH_POP;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_HID_CreateReportID()
*
* Description : Creates report id list from pre-parsed data.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
* Return(s)   : USBH_ERR_NONE,      if report ID list successfully created.
*               USBH_ERR_ALLOC,     if report ID cannot be allocated.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_HID_CreateReportID (USBH_HID_DEV  *p_hid_dev)
{
    CPU_BOOLEAN           found;
    CPU_INT08U            coll_ix;
    CPU_INT08U            report_fmt_ix;
    CPU_INT08U            report_id_ix;
    USBH_HID_REPORT_FMT  *p_report_fmt;
    USBH_HID_REPORT_ID   *p_report_id;


    p_hid_dev->NbrReportID = 0u;
                                                                /* For each app collection.                     */
    for (coll_ix = 0u; coll_ix < p_hid_dev->NbrAppColl; coll_ix++) {

        for (report_fmt_ix = 0u; report_fmt_ix < p_hid_dev->AppColl[coll_ix].NbrReportFmt; report_fmt_ix++) {

            p_report_fmt = &p_hid_dev->AppColl[coll_ix].ReportFmt[report_fmt_ix];
            found        =  DEF_FALSE;

            for (report_id_ix = 0u; report_id_ix < p_hid_dev->NbrReportID; report_id_ix++) {
                p_report_id = &p_hid_dev->ReportID[report_id_ix];
                                                                /* If report ID/type match, increment size.      */
                if ((p_report_id->ReportID == p_report_fmt->ReportID  ) &&
                    (p_report_id->Type     == p_report_fmt->ReportType)) {

                    p_report_id->Size += (p_report_fmt->ReportCnt * p_report_fmt->ReportSize);
                    found              =  DEF_TRUE;
                    break;
                }
            }

            if (found == DEF_FALSE) {                           /* No match found, create new report ID.             */

                if (p_hid_dev->NbrReportID >= USBH_HID_CFG_MAX_NBR_REPORT_ID) {
                    return (USBH_ERR_ALLOC);
                }

                p_report_id            = &p_hid_dev->ReportID[p_hid_dev->NbrReportID];
                p_report_id->ReportID  =  p_report_fmt->ReportID;
                p_report_id->Type      =  p_report_fmt->ReportType;
                p_report_id->Size      = (p_report_fmt->ReportCnt * p_report_fmt->ReportSize);
                p_hid_dev->NbrReportID++;
            }
        }
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HID_MaxReport()
*
* Description : Find report of given type that has the greater size.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               type            Report Type.
*
* Return(s)   : Max report id structure pointer.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_HID_REPORT_ID  *USBH_HID_MaxReport (USBH_HID_DEV  *p_hid_dev,
                                         CPU_INT08U     type)
{
    USBH_HID_REPORT_ID  *p_report_id;
    CPU_INT32U           size;
    CPU_INT08U           report_id_ix;


    size        = 0u;
    p_report_id = (USBH_HID_REPORT_ID *)0;

    for (report_id_ix = 0u; report_id_ix < p_hid_dev->NbrReportID; report_id_ix++) {

        if (p_hid_dev->ReportID[report_id_ix].Type == type) {

            if (p_hid_dev->ReportID[report_id_ix].Size > size) {
                size        =  p_hid_dev->ReportID[report_id_ix].Size;
                p_report_id = &p_hid_dev->ReportID[report_id_ix];
            }
        }
    }

    return (p_report_id);
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
*                                      USBH_HID_InitItemParser()
*
* Description : Initialize HID parser.
*
* Argument(s) : p_parser        Pointer to the parser.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_InitItemParser (USBH_HID_PARSER  *p_parser)
{

    Mem_Clr((void *)p_parser,
                    sizeof(USBH_HID_PARSER));

    USBH_HID_InitGlobalItem(&p_parser->Global);
    USBH_HID_InitLocalItem(&p_parser->Local);
}


/*
*********************************************************************************************************
*                                      USBH_HID_FreeItemParser()
*
* Description : Free memory allocated for parser structure.
*
* Argument(s) : p_parser        Pointer to parser.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_FreeItemParser (USBH_HID_PARSER  *p_parser)
{
    USBH_HID_GLOBAL  *p_global;
    USBH_HID_GLOBAL  *p_global_free;
    USBH_HID_COLL    *p_coll;
    USBH_HID_COLL    *p_coll_free;
    LIB_ERR           err_lib;

                                                                /* --------------- FREE THE GLOBAL STACK -------------- */
    p_global = p_parser->GlobalStkPtr;
    while (p_global != (USBH_HID_GLOBAL *)0) {
        p_global_free = p_global;
        p_global      = p_global->NextPtr;

        Mem_PoolBlkFree(       &USBH_HID_GlobalItemPool,
                        (void *)p_global_free,
                               &err_lib);
    }
                                                                /* ------------- FREE THE COLLECTION STACK ------------ */
    p_coll = p_parser->CollStkPtr;
    while (p_coll != (USBH_HID_COLL *)0) {
        p_coll_free = p_coll;
        p_coll      = p_coll->NextPtr;

        Mem_PoolBlkFree(       &USBH_HID_CollPool,
                        (void *)p_coll_free,
                               &err_lib);
    }
}


/*
*********************************************************************************************************
*                                      USBH_HID_InitGlobalItem()
*
* Description : Initialize global item.
*
* Argument(s) : p_global    Pointer to global item.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_InitGlobalItem (USBH_HID_GLOBAL  *p_global)
{

    Mem_Clr((void *)p_global,
                    sizeof(USBH_HID_GLOBAL));

    p_global->LogMin  = (CPU_INT32S)USBH_HID_LOG_MIN_UNDEFINED;
    p_global->LogMax  = (CPU_INT32S)USBH_HID_LOG_MAX_UNDEFINED;
    p_global->PhyMin  = (CPU_INT32S)USBH_HID_PHY_MIN_UNDEFINED;
    p_global->PhyMax  = (CPU_INT32S)USBH_HID_PHY_MAX_UNDEFINED;
    p_global->UnitExp = (CPU_INT32S)USBH_HID_UNIT_EXP_UNDEFINED;
    p_global->Unit    = (CPU_INT32S)USBH_HID_UNIT_UNDEFINED;
}


/*
*********************************************************************************************************
*                                      USBH_HID_InitLocalItem()
*
* Description : Initialize the local item.
*
* Argument(s) : p_local         Pointer to the local item.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_InitLocalItem (USBH_HID_LOCAL  *p_local)
{
    Mem_Clr((void *)p_local,
                    sizeof(USBH_HID_LOCAL));

    p_local->UsageMin = USBH_HID_USAGE_MIN_UNDEFINED;
    p_local->UsageMax = USBH_HID_USAGE_MAX_UNDEFINED;
}


/*
*********************************************************************************************************
*                                        USBH_HID_ParseMain()
*
* Description : Parse a main item.
*
* Argument(s) : p_hid_dev       Pointer to the HID device.
*
*               p_parser        Pointer to the parser.
*
* Return(s)   : USBH_ERR_NONE,                  if item is successfully parsed.
*               USBH_ERR_HID_ITEM_UNKNOWN,      if unknown item.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_ParseMain (USBH_HID_DEV     *p_hid_dev,
                                      USBH_HID_PARSER  *p_parser)
{
    USBH_ERR  err;


                                                                /* -------------------- PARSE ITEM -------------------- */
    switch (p_parser->Tag) {
        case USBH_HID_MAIN_ITEM_TAG_FEATURE:
        case USBH_HID_MAIN_ITEM_TAG_IN:
        case USBH_HID_MAIN_ITEM_TAG_OUT:
             err = USBH_HID_AddReport(p_hid_dev, p_parser);     /* Add report to report list.                           */
             break;

        case USBH_HID_MAIN_ITEM_TAG_COLL:
             err = USBH_HID_OpenColl(p_hid_dev, p_parser);      /* Open new collection.                                 */
             break;

        case USBH_HID_MAIN_ITEM_TAG_ENDCOLL:
             err = USBH_HID_CloseColl(p_parser);                /* Close collection.                                    */
             break;

        default:
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
             USBH_PRINT_LOG("Unknown main tag\r\n");
#endif
             err = USBH_ERR_HID_ITEM_UNKNOWN;
    }
                                                                /* ---------------- RE-INIT LOCAL ITEM ---------------- */
    USBH_HID_InitLocalItem(&p_parser->Local);

    return (err);
}


/*
*********************************************************************************************************
*                                       USBH_HID_ParseGlobal()
*
* Description : Parse a global item.
*
* Argument(s) : p_parser        Pointer to parser.
*
* Return(s)   : USBH_ERR_NONE,                      if global item successfully parsed.
*               USBH_ERR_HID_USAGE_PAGE_INVALID,    if usage page is out of range.
*               USBH_ERR_HID_REPORT_ID,             if report ID is invalid.
*               USBH_ERR_HID_REPORTID_OUTRANGE,     if report ID is out of range.
*               USBH_ERR_HID_REPORT_CNT,            if report count is zero.
*               USBH_ERR_HID_PUSH_SIZE,             if push item with zero size.
*               USBH_ERR_HID_POP_SIZE,              if pop  item with zero size.
*               USBH_ERR_HID_MISMATCH_PUSH_POP,     if mismatch in push and pop items.
*               USBH_ERR_HID_ITEM_UNKNOWN,          if unknown global item.
*               USBH_ERR_ALLOC,                     if global item cannot be allocated.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_ParseGlobal (USBH_HID_PARSER  *p_parser)
{
    USBH_HID_GLOBAL  *p_global_new;
    USBH_HID_GLOBAL  *p_global_free;
    LIB_ERR           err_lib;


    switch (p_parser->Tag) {
        case USBH_HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
             p_parser->Global.UsagePage = USBH_HID_UNSIGNED_DATA(p_parser);

             if (p_parser->Size > 2u) {                         /* Rtn err if parser size is too large.                 */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM USAGE PAGE: OUT OF RANGE\r\n");
#endif
                 return (USBH_ERR_HID_USAGE_PAGE_INVALID);
             }

             if ((p_parser->Global.UsagePage >= 0x0092u) &&     /* Warn if usage page is a reserved value.              */
                 (p_parser->Global.UsagePage <  0xFEFFu)) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM USAGE PAGE: RESERVED VALUE\r\n");
#endif
             }
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_LOG_MIN:
             p_parser->Global.LogMin = USBH_HID_SIGNED_DATA(p_parser);
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_LOG_MAX:
             p_parser->Global.LogMax = USBH_HID_SIGNED_DATA(p_parser);
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_PHY_MIN:
             p_parser->Global.PhyMin = USBH_HID_SIGNED_DATA(p_parser);
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_PHY_MAX:
             p_parser->Global.PhyMax = USBH_HID_SIGNED_DATA(p_parser);
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT:
             p_parser->Global.UnitExp = USBH_HID_SIGNED_DATA(p_parser);
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_UNIT:
             p_parser->Global.Unit = USBH_HID_SIGNED_DATA(p_parser);
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_REPORT_SIZE:
             p_parser->Global.ReportSize = USBH_HID_UNSIGNED_DATA(p_parser);
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_REPORT_ID:
             p_parser->Global.ReportID = USBH_HID_UNSIGNED_DATA(p_parser);

             if (p_parser->Global.ReportID == 0u) {             /* Report ID should not be zero.                        */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM REPORT ID: REPORT ID == 0\r\n");
#endif
                 return (USBH_ERR_HID_REPORT_ID);
             }

             if (p_parser->Size > 1u) {                         /* Report ID should be 1-byte value.                    */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM REPORT ID: REPORT ID > 255\r\n");
#endif
                 return (USBH_ERR_HID_REPORT_ID);
             }
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_REPORT_COUNT:
             p_parser->Global.ReportCnt = USBH_HID_UNSIGNED_DATA(p_parser);

             if (p_parser->Global.ReportCnt == 0u) {            /* Report cnt should not be zero.                       */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM REPORT COUNT: REPORT COUNT ZERO\r\n");
#endif
                 return (USBH_ERR_HID_REPORT_CNT);
             }
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_PUSH:                     /* Push item to stack.                                  */
             if (p_parser->Size != 0u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM PUSH: SIZE ZERO\r\n");
#endif
                 return (USBH_ERR_HID_PUSH_SIZE);
             }
                                                                /* Allocate memory for new entry.                       */
             p_global_new = (USBH_HID_GLOBAL *)Mem_PoolBlkGet ((MEM_POOL *)&USBH_HID_GlobalItemPool,
                                                                            sizeof(USBH_HID_GLOBAL),
                                                                           &err_lib);
             if (err_lib != LIB_MEM_ERR_NONE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM PUSH: OUT OF MEMORY\r\n");
#endif
                 return (USBH_ERR_ALLOC);
             }

            *p_global_new           = p_parser->Global;         /* Copy global to new entry.                            */
             p_global_new->NextPtr  = p_parser->GlobalStkPtr;
             p_parser->GlobalStkPtr = p_global_new;
             break;

        case USBH_HID_GLOBAL_ITEM_TAG_POP:                      /* Pop item from stack.                                 */
             if (p_parser->Size != 0u) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM POP: SIZE ZERO\r\n");
#endif
                 return (USBH_ERR_HID_POP_SIZE);
             }
                                                                /* Global stack should be not empty.                    */
             if (p_parser->GlobalStkPtr == (USBH_HID_GLOBAL *)0) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("GLOBAL ITEM POP: MISMATCHED PUSH/POP\r\n");
#endif
                 return (USBH_ERR_HID_MISMATCH_PUSH_POP);
             }
             p_parser->Global       = *(p_parser->GlobalStkPtr);/* Copy top element to global.                          */
             p_global_free          =   p_parser->GlobalStkPtr;
             p_parser->GlobalStkPtr =   p_global_free->NextPtr;

             Mem_PoolBlkFree(       &USBH_HID_GlobalItemPool,   /* Free top item.                                       */
                             (void *)p_global_free,
                                    &err_lib);
             break;

        default:
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
             USBH_PRINT_LOG("GLOBAL ITEM: UNKNOWN\r\n");
#endif
             return (USBH_ERR_HID_ITEM_UNKNOWN);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HID_ParseLocal()
*
* Description : Parse a local item.
*
* Argument(s) : p_parser        Pointer to the parser.
*
* Return(s)   : USBH_ERR_NONE,      if local item is successfully parsed.
*               USBH_ERR_ALLOC,     if usage array would have overflowed.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_ParseLocal (USBH_HID_PARSER  *p_parser)
{
    switch (p_parser->Tag) {
        case USBH_HID_LOCAL_ITEM_TAG_USAGE:
             p_parser->Local.NbrUsage++;

             if (p_parser->Local.NbrUsage > USBH_HID_CFG_MAX_NBR_USAGE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                 USBH_PRINT_LOG("LOCAL ITEM USAGE: ARRAY OVERFLOW\r\n");
#endif
                 return (USBH_ERR_ALLOC);
             }
             p_parser->Local.Usage[p_parser->Local.NbrUsage - 1u] = USBH_HID_UNSIGNED_DATA(p_parser);
             break;

        case USBH_HID_LOCAL_ITEM_TAG_USAGE_MIN:
             p_parser->Local.UsageMin = USBH_HID_UNSIGNED_DATA(p_parser);
             break;

        case USBH_HID_LOCAL_ITEM_TAG_USAGE_MAX:
             p_parser->Local.UsageMax =  USBH_HID_UNSIGNED_DATA(p_parser);
             break;

        default:
             break;
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                         USBH_HID_OpenColl()
*
* Description : Add new collection item to collection stack.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               p_parser        Pointer to parser.
*
* Return(s)   : USBH_ERR_NONE,                  if collection opened successfully.
*               USBH_ERR_HID_NOT_APP_COLL,      if top level collection not an application collection.
*               USBH_ERR_ALLOC,                 if collection cannot be freed/allocated.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_OpenColl (USBH_HID_DEV     *p_hid_dev,
                                     USBH_HID_PARSER  *p_parser)
{
    USBH_HID_COLL  *p_coll_new;
    LIB_ERR         err_lib;


                                                                /* --------------- INIT NEW COLLECTION ---------------- */
    p_coll_new = (USBH_HID_COLL *)Mem_PoolBlkGet(&USBH_HID_CollPool,
                                                  sizeof(USBH_HID_COLL),
                                                 &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
        return (USBH_ERR_ALLOC);
    }
                                                                /* Initialize collection parameters.                    */
    p_coll_new->Usage = (((CPU_INT32U)p_parser->Global.UsagePage << 16u) & 0xFFFF0000u) | (CPU_INT32U)p_parser->Local.Usage[0];
    p_coll_new->Type  = USBH_HID_UNSIGNED_DATA(p_parser);

    if ((p_parser->CollStkPtr == (USBH_HID_COLL *)0) &&         /* Check if first collection is app.                    */
        (p_coll_new->Type     !=  USBH_HID_COLL_APP)) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("OPEN COLLECTION: FIRST COLLECTION NOT APP COLLECTION\r\n");
#endif
        Mem_PoolBlkFree(       &USBH_HID_CollPool,
                        (void *)p_coll_new,
                               &err_lib);

        return (USBH_ERR_HID_NOT_APP_COLL);
    }

                                                                /* --------- PUT COLLECTION IN APP USAGE ARAY --------- */
    if (p_coll_new->Type == USBH_HID_COLL_APP) {
        if (p_parser->CollStkPtr != (USBH_HID_COLL *)0) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("OPEN COLLECTION: NESTED\r\n");
#endif
            Mem_PoolBlkFree(       &USBH_HID_CollPool,
                            (void *)p_coll_new,
                                   &err_lib);

            return (USBH_ERR_HID_NOT_APP_COLL);
        }
                                                                /* Max app collections exceeded.                        */
        if (p_hid_dev->NbrAppColl == USBH_HID_CFG_MAX_NBR_APP_COLL) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("OPEN COLLECTION: MAX APP COLLECTION EXCEEDED\r\n");
#endif
            Mem_PoolBlkFree(       &USBH_HID_CollPool,
                            (void *)p_coll_new,
                                   &err_lib);

            return (USBH_ERR_ALLOC);
        }
        p_hid_dev->AppColl[p_hid_dev->NbrAppColl].Usage = p_coll_new->Usage;
        p_hid_dev->AppColl[p_hid_dev->NbrAppColl].Type  = p_coll_new->Type;
        p_hid_dev->NbrAppColl++;
    }

                                                                /* ------------- PUT COLLECTION IN STACK -------------- */
    p_coll_new->NextPtr  = p_parser->CollStkPtr;
    p_parser->CollStkPtr = p_coll_new;

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HID_CloseColl()
*
* Description : Remove top collection from collection stack.
*
* Argument(s) : p_parser         Pointer to the parser.
*
* Return(s)   : USBH_ERR_NONE,                  if collection is successfully closed.
*               USBH_ERR_HID_MISMATCH_COLL,     if collection stack is empty.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_CloseColl (USBH_HID_PARSER  *p_parser)
{
    USBH_HID_COLL  *p_coll_free;
    LIB_ERR         err_lib;


    if (p_parser->CollStkPtr == (USBH_HID_COLL *)0) {           /* Collection stack should not be empty.                */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("CLOSE COLLECTION: MISMATCH\r\n");
#endif
        return (USBH_ERR_HID_MISMATCH_COLL);
    }

    p_coll_free          = p_parser->CollStkPtr;                /* Remove top element from collection stack.            */
    p_parser->CollStkPtr = p_coll_free->NextPtr;

    Mem_PoolBlkFree(       &USBH_HID_CollPool,
                    (void *)p_coll_free,
                           &err_lib);

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HID_AddReport()
*
* Description : Add new report to report list in application collection.
*
* Argument(s) : p_hid_dev       Pointer to HID device.
*
*               p_parser        Pointer to parser.
*
* Return(s)   : USBH_ERR_NONE,                          if report is successfully added.
*               USBH_ERR_ALLOC,                         if memory could not be allocated for report format.
*               USBH_ERR_HID_REPORT_OUTSIDE_COLL,       if no application collection.
*
*                                                       ----- RETURNED BY USBH_HID_ValidateReport -----
*               USBH_ERR_HID_REPORT_INVALID_VAL,        if any of report's value is invalid.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_AddReport (USBH_HID_DEV     *p_hid_dev,
                                      USBH_HID_PARSER  *p_parser)
{
    USBH_HID_REPORT_FMT  *p_report_new;
    USBH_HID_APP_COLL    *p_app_coll;
    USBH_ERR              err;


    if (p_hid_dev->NbrAppColl == 0u) {                           /* Rtn error if no app collections exist.               */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("ADD REPORT: NO APP COLLECTION's\r\n");
#endif
        return (USBH_ERR_HID_REPORT_OUTSIDE_COLL);
    }
                                                                /* Current Application Collection                       */
    p_app_coll = &p_hid_dev->AppColl[p_hid_dev->NbrAppColl - 1u];

    if (p_app_coll->NbrReportFmt == USBH_HID_CFG_MAX_NBR_REPORT_FMT) {
        return (USBH_ERR_ALLOC);
    }
                                                                /* Get new Report Format                                */
    p_report_new = &p_app_coll->ReportFmt[p_app_coll->NbrReportFmt];
    p_app_coll->NbrReportFmt++;

                                                                /* ----------------- INITIALIZE REPORT ---------------- */
    USBH_HID_InitReport(p_parser, p_report_new);

                                                                /* ------------------ VALIDATE REPORT ----------------- */
    err = USBH_HID_ValidateReport(p_report_new);
    if (err != USBH_ERR_NONE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("HID DEVICE REPORT VALIDATION FAILED, ERR = %d\r\n", err);
#endif
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                      USBH_HID_ValidateReport()
*
* Description : Validate report by checking report values.
*
* Argument(s) : p_report_fmt        Pointer to report.
*
* Return(s)   : USBH_ERR_NONE,                      if report is valid.
*               USBH_ERR_HID_REPORT_INVALID_VAL,    if any of report's value is invalid.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_HID_ValidateReport (USBH_HID_REPORT_FMT  *p_report_fmt)
{
    CPU_BOOLEAN  x_def;
    CPU_BOOLEAN  y_def;

                                                                /* --------------- CHK MANDATORY FIELDS --------------- */
    if ((p_report_fmt->UsagePage  == USBH_HID_USAGE_PAGE_UNDEFINED ) ||
        (p_report_fmt->LogMax     == USBH_HID_LOG_MIN_UNDEFINED    ) ||
        (p_report_fmt->LogMin     == USBH_HID_LOG_MAX_UNDEFINED    ) ||
        (p_report_fmt->ReportCnt  == USBH_HID_REPORT_CNT_UNDEFINED ) ||
        (p_report_fmt->ReportSize == USBH_HID_REPORT_SIZE_UNDEFINED)) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("VALIDATE REPORT: MANDATORY FIELD NOT DEFINED\r\n");
#endif
        return (USBH_ERR_HID_REPORT_INVALID_VAL);
    }

                                                                /* -------------- CHK LOGICAL MIN & MAX --------------- */
    if (p_report_fmt->LogMin >= p_report_fmt->LogMax) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("VALIDATE REPORT: LOG MIN > LOG MAX\r\n");
#endif
        return (USBH_ERR_HID_REPORT_INVALID_VAL);
    }
                                                                /* Chk presence/value of physical min & max.            */
    x_def = p_report_fmt->PhyMax == USBH_HID_PHY_MAX_UNDEFINED ? DEF_FALSE : DEF_TRUE;
    y_def = p_report_fmt->PhyMin == USBH_HID_PHY_MIN_UNDEFINED ? DEF_FALSE : DEF_TRUE;

    if (((x_def == DEF_TRUE) && (y_def == DEF_FALSE)) ||
        ((y_def == DEF_TRUE) && (x_def == DEF_FALSE))) {

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("VALIDATE REPORT: ONLY ONE OF PHY MIN & MAX DEF'd\r\n");
#endif
        return (USBH_ERR_HID_REPORT_INVALID_VAL);

    } else if ((x_def == DEF_FALSE) && (y_def == DEF_FALSE)) {

        p_report_fmt->PhyMin = p_report_fmt->LogMin;
        p_report_fmt->PhyMax = p_report_fmt->LogMax;

    } else if (p_report_fmt->PhyMax < p_report_fmt->PhyMin) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("VALIDATE REPORT: PHY MIN > PHY MAX\r\n");
#endif
        return (USBH_ERR_HID_REPORT_INVALID_VAL);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        USBH_HID_InitReport()
*
* Description : Initialize report.
*
* Argument(s) : p_parser            Pointer to parser.
*
*               p_report_fmt        Pointer to report.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/
static  void  USBH_HID_InitReport (USBH_HID_PARSER      *p_parser,
                                   USBH_HID_REPORT_FMT  *p_report_fmt)
{
    USBH_HID_COLL  *p_coll_tmp;
    CPU_INT08U      usage_ix;

                                                                /* Copy usages.                                         */
    for (usage_ix = 0u; usage_ix < p_parser->Local.NbrUsage; usage_ix++) {
        p_report_fmt->Usage[usage_ix] = ((CPU_INT32U)p_parser->Global.UsagePage << 16u) |
                                         (CPU_INT32U)p_parser->Local.Usage[usage_ix];
    }

    p_report_fmt->UsagePage  = p_parser->Global.UsagePage;
    p_report_fmt->NbrUsage   = p_parser->Local.NbrUsage;
    p_report_fmt->ReportType = p_parser->Tag;
    p_report_fmt->Flag       = USBH_HID_UNSIGNED_DATA(p_parser);
    p_report_fmt->LogMin     = p_parser->Global.LogMin;
    p_report_fmt->LogMax     = p_parser->Global.LogMax;
    p_report_fmt->PhyMin     = p_parser->Global.PhyMin;
    p_report_fmt->PhyMax     = p_parser->Global.PhyMax;
    p_report_fmt->ReportSize = p_parser->Global.ReportSize;
    p_report_fmt->ReportCnt  = p_parser->Global.ReportCnt;
    p_report_fmt->UnitExp    = p_parser->Global.UnitExp;
    p_report_fmt->Unit       = p_parser->Global.Unit;
    p_report_fmt->ReportID   = p_parser->Global.ReportID;
    p_report_fmt->UsageMin   = p_parser->Local.UsageMin;
    p_report_fmt->UsageMax   = p_parser->Local.UsageMax;
    p_report_fmt->PhyUsage   = USBH_HID_USAGE_PAGE_UNDEFINED;   /* Init collection usage to undefined values.           */
    p_report_fmt->AppUsage   = USBH_HID_USAGE_PAGE_UNDEFINED;
    p_report_fmt->LogUsage   = USBH_HID_USAGE_PAGE_UNDEFINED;

    p_coll_tmp = p_parser->CollStkPtr;                          /* Find values for collection usage.                    */
    while (p_coll_tmp != (USBH_HID_COLL *)0) {
        switch (p_coll_tmp->Type) {
            case USBH_HID_COLL_PHYSICAL:
                 p_report_fmt->PhyUsage = p_coll_tmp->Usage;
                 break;

            case USBH_HID_COLL_APP:
                 p_report_fmt->AppUsage = p_coll_tmp->Usage;
                 break;

            case USBH_HID_COLL_LOGICAL:
                 p_report_fmt->LogUsage = p_coll_tmp->Usage;
                 break;

            default:
                 break;
        }
        p_coll_tmp = p_coll_tmp->NextPtr;
    }
}


/*
*********************************************************************************************************
*                                         USBH_HID_SetData()
*
* Description : Set data in parser.
*
* Argument(s) : p_parser        Pointer to parser.
*
*               p_buf           Pointer to data buffer.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_HID_SetData (USBH_HID_PARSER  *p_parser,
                                void             *p_buf)
{
    switch (p_parser->Size) {
        case 1:
             p_parser->Data.U08 =             MEM_VAL_GET_INT08U_LITTLE(p_buf);
             p_parser->Data.S08 = (CPU_INT08S)MEM_VAL_GET_INT08U_LITTLE(p_buf);
             break;

        case 2:
             p_parser->Data.U16 =             MEM_VAL_GET_INT16U_LITTLE(p_buf);
             p_parser->Data.S16 = (CPU_INT16S)MEM_VAL_GET_INT16U_LITTLE(p_buf);
             break;

        case 4:
             p_parser->Data.U32 =             MEM_VAL_GET_INT32U_LITTLE(p_buf);
             p_parser->Data.S32 = (CPU_INT32S)MEM_VAL_GET_INT32U_LITTLE(p_buf);
             break;

        default:
             break;
    }
}

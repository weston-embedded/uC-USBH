/*
*********************************************************************************************************
*                                             uC/USB-Host
*                                     The Embedded USB Host Stack
*
*                    Copyright 2004-2021 Silicon Laboratories Inc. www.silabs.com
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
*                                      USB HOST CLASS OPERATIONS
*
* Filename : usbh_class.c
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_CLASS_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_class.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


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
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  USBH_ERR  USBH_ClassProbeDev(USBH_DEV     *p_dev);

static  USBH_ERR  USBH_ClassProbeIF (USBH_DEV     *p_dev,
                                     USBH_IF      *p_if);

static  void      USBH_ClassNotify  (USBH_DEV     *p_dev,
                                     USBH_IF      *p_if,
                                     void         *p_class_dev,
                                     CPU_BOOLEAN   is_conn);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBH_ClassDrvReg()
*
* Description : Registers a class driver to the USB host stack.
*
* Argument(s) : p_class_drv          Pointer to the class driver.
*
*               class_notify_fnct    Class notification function pointer.
*
*               p_class_notify_ctx   Class notification function context pointer.
*
* Return(s)   : USBH_ERR_NONE               If the class driver is registered.
*               USBH_ERR_INVALID_ARG        If invalid argument(s) passed to 'p_host'/ 'p_class_drv'.
*               USBH_ERR_CLASS_DRV_ALLOC    If maximum class driver limit reached.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_ClassDrvReg (USBH_CLASS_DRV          *p_class_drv,
                            USBH_CLASS_NOTIFY_FNCT   class_notify_fnct,
                            void                    *p_class_notify_ctx)
{
    CPU_INT32U  ix;
    USBH_ERR    err;
    CPU_SR_ALLOC();


    if (p_class_drv == (USBH_CLASS_DRV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    if (p_class_drv->NamePtr == (CPU_INT08U *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    if ((p_class_drv->ProbeDev == 0) &&
        (p_class_drv->ProbeIF  == 0)) {
         return (USBH_ERR_INVALID_ARG);
    }

    CPU_CRITICAL_ENTER();
    for (ix = 0u; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {     /* Find first empty element in the class drv list.      */

        if (USBH_ClassDrvList[ix].InUse == 0u) {                /* Insert class drv if it is empty location.            */
            USBH_ClassDrvList[ix].ClassDrvPtr   = p_class_drv;
            USBH_ClassDrvList[ix].NotifyFnctPtr = class_notify_fnct;
            USBH_ClassDrvList[ix].NotifyArgPtr  = p_class_notify_ctx;
            USBH_ClassDrvList[ix].InUse         = 1u;
            break;
        }
    }
    CPU_CRITICAL_EXIT();

    if (ix >= USBH_CFG_MAX_NBR_CLASS_DRVS) {                    /* List is full.                                        */
        return (USBH_ERR_CLASS_DRV_ALLOC);
    }

    p_class_drv->GlobalInit(&err);

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_ClassDrvUnreg()
*
* Description : Unregisters a class driver from the USB host stack.
*
* Argument(s) : p_class_drv     Pointer to the class driver.
*
* Return(s)   : USBH_ERR_NONE                   If the class driver is unregistered.
*               USBH_ERR_INVALID_ARG            If invalid argument(s) passed to 'p_host'/ 'p_class_drv'.
*               USBH_ERR_CLASS_DRV_NOT_FOUND    If no class driver found.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_ClassDrvUnreg (USBH_CLASS_DRV  *p_class_drv)
{
    CPU_INT32U  ix;
    CPU_SR_ALLOC();


    if (p_class_drv == (USBH_CLASS_DRV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    CPU_CRITICAL_ENTER();
    for (ix = 0u; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {     /* Find the element in the class driver list.           */

        if ((USBH_ClassDrvList[ix].InUse       !=          0u) &&
            (USBH_ClassDrvList[ix].ClassDrvPtr == p_class_drv)) {

            USBH_ClassDrvList[ix].ClassDrvPtr   = (USBH_CLASS_DRV       *)0;
            USBH_ClassDrvList[ix].NotifyFnctPtr = (USBH_CLASS_NOTIFY_FNCT)0;
            USBH_ClassDrvList[ix].NotifyArgPtr  = (void                 *)0;
            USBH_ClassDrvList[ix].InUse         = 0u;
            break;
        }
    }
    CPU_CRITICAL_EXIT();

    if (ix >= USBH_CFG_MAX_NBR_CLASS_DRVS) {
        return (USBH_ERR_CLASS_DRV_NOT_FOUND);
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                         USBH_ClassSuspend()
*
* Description : Suspend all class drivers associated to the device.
*
* Argument(s) : p_dev       Pointer to USB device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_ClassSuspend (USBH_DEV  *p_dev)
{
    CPU_INT08U       if_ix;
    CPU_INT08U       nbr_ifs;
    USBH_CFG        *p_cfg;
    USBH_IF         *p_if;
    USBH_CLASS_DRV  *p_class_drv;


    if ((p_dev->ClassDevPtr    != (void               *)0) &&   /* If class drv is present at dev level.                */
        (p_dev->ClassDrvRegPtr != (USBH_CLASS_DRV_REG *)0)) {
        p_class_drv = p_dev->ClassDrvRegPtr->ClassDrvPtr;
                                                                /* Chk if class drv is present at dev level.            */
        if ((p_class_drv          != (USBH_CLASS_DRV *)0) &&
            (p_class_drv->Suspend != (void           *)0)) {
            p_class_drv->Suspend(p_dev->ClassDevPtr);
            return;
        }
    }

    p_cfg   = USBH_CfgGet(p_dev, 0u);                           /* Get first cfg.                                       */
    nbr_ifs = USBH_CfgIF_NbrGet(p_cfg);

    for (if_ix = 0u; if_ix < nbr_ifs; if_ix++) {
        p_if = USBH_IF_Get(p_cfg, if_ix);
        if (p_if == (USBH_IF *)0) {
            return;
        }

        if ((p_if->ClassDevPtr    != (void               *)0) &&
            (p_if->ClassDrvRegPtr != (USBH_CLASS_DRV_REG *)0)) {
            p_class_drv = p_if->ClassDrvRegPtr->ClassDrvPtr;

            if ((p_class_drv          != (USBH_CLASS_DRV *)0) &&
                (p_class_drv->Suspend != (void           *)0)) {
                p_class_drv->Suspend(p_if->ClassDevPtr);
                return;
            }
        }
    }
}


/*
*********************************************************************************************************
*                                          USBH_ClassResume()
*
* Description : Resume all class drivers associated to the device.
*
* Argument(s) : p_dev   Pointer to USB device.
*
* Return(s)   : None
*
* Note(s)     : None
*********************************************************************************************************
*/

void  USBH_ClassResume (USBH_DEV  *p_dev)
{
    CPU_INT08U       if_ix;
    CPU_INT08U       nbr_ifs;
    USBH_CFG        *p_cfg;
    USBH_IF         *p_if;
    USBH_CLASS_DRV  *p_class_drv;


    if ((p_dev->ClassDevPtr    != (void               *)0) &&   /* If class drv is present at dev level.                */
        (p_dev->ClassDrvRegPtr != (USBH_CLASS_DRV_REG *)0)) {
        p_class_drv = p_dev->ClassDrvRegPtr->ClassDrvPtr;
                                                                /* Chk if class drv is present at dev level.            */
        if ((p_class_drv         != (USBH_CLASS_DRV *)0) &&
            (p_class_drv->Resume != (void           *)0)) {
            p_class_drv->Resume(p_dev->ClassDevPtr);
            return;
        }
    }

    p_cfg   = USBH_CfgGet(p_dev, 0u);                           /* Get 1st cfg.                                         */
    nbr_ifs = USBH_CfgIF_NbrGet(p_cfg);

    for (if_ix = 0u; if_ix < nbr_ifs; if_ix++) {
        p_if = USBH_IF_Get(p_cfg, if_ix);
        if (p_if == (USBH_IF *)0) {
            return;
        }

        if ((p_if->ClassDevPtr    != (void               *)0) &&
            (p_if->ClassDrvRegPtr != (USBH_CLASS_DRV_REG *)0)) {
            p_class_drv = p_if->ClassDrvRegPtr->ClassDrvPtr;

            if ((p_class_drv         != (USBH_CLASS_DRV *)0) &&
                (p_class_drv->Resume != (void           *)0)) {
                p_class_drv->Resume(p_if->ClassDevPtr);
                return;
            }
        }
    }
}


/*
*********************************************************************************************************
*                                         USBH_ClassDrvConn()
*
* Description : Once a device is connected, attempts to find a class driver matching the device descriptor.
*               If no class driver matches the device descriptor, it will attempt to find a class driver
*               for each interface present in the active configuration.
*
* Argument(s) : p_dev   Pointer to USB device.
*
* Return(s)   : USBH_ERR_NONE                           If a class driver was found.
*               USBH_ERR_DRIVER_NOT_FOUND               If no class driver was found.
*
*                                                       ----- RETURNED BY USBH_CfgSet() : -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_IF_Get() : -----
*               USBH_ERR_NULL_PTR                       If a null pointer was set for 'p_if'.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_ClassDrvConn (USBH_DEV  *p_dev)
{

    CPU_INT08U    if_ix;
    CPU_INT08U    nbr_if;
    CPU_BOOLEAN   drv_found;
    USBH_ERR      err;
    USBH_CFG     *p_cfg;
    USBH_IF      *p_if;


    err = USBH_ClassProbeDev(p_dev);
    if (err == USBH_ERR_NONE) {
        p_if = (USBH_IF *)0;
        USBH_ClassNotify(p_dev,                                 /* Find a class drv matching dev desc.                  */
                         p_if,
                         p_dev->ClassDevPtr,
                         USBH_CLASS_DEV_STATE_CONN);
        return (USBH_ERR_NONE);
    } else if ((err != USBH_ERR_CLASS_DRV_NOT_FOUND) &&
               (err != USBH_ERR_CLASS_PROBE_FAIL)) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("ERROR: Probe class driver. #%d\r\n", err);
#endif
    } else {
                                                                /* Empty Else Statement                                 */        
    }

    err = USBH_CfgSet(p_dev, 1u);                               /* Select first cfg.                                    */
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    drv_found = DEF_FALSE;
    p_cfg     = USBH_CfgGet(p_dev, (p_dev->SelCfg - 1));        /* Get active cfg struct.                               */
    nbr_if    = USBH_CfgIF_NbrGet(p_cfg);

    for (if_ix = 0u; if_ix < nbr_if; if_ix++) {                 /* For all IFs present in cfg.                          */
        p_if = USBH_IF_Get(p_cfg, if_ix);
        if (p_if == (USBH_IF *)0) {
            return(USBH_ERR_NULL_PTR);
        }

        err  = USBH_ClassProbeIF(p_dev, p_if);                  /* Find class driver matching IF.                       */

        if (err == USBH_ERR_NONE) {
            drv_found = DEF_TRUE;
        } else if (err != USBH_ERR_CLASS_DRV_NOT_FOUND) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("ERROR: Probe class driver. #%d\r\n", err);
#endif
        } else {
                                                                /* Empty Else Statement                                 */            
        }
    }
    if (drv_found == DEF_FALSE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("No Class Driver Found.\r\n");
#endif
        return (err);
    }

    for (if_ix = 0u; if_ix < nbr_if; if_ix++) {                 /* For all IFs present in this cfg, notify app.         */
        p_if = USBH_IF_Get(p_cfg, if_ix);
        if (p_if == (USBH_IF *)0) {
            return(USBH_ERR_NULL_PTR);
        }

        if (p_if->ClassDevPtr != 0) {
            USBH_ClassNotify(p_dev,
                             p_if,
                             p_if->ClassDevPtr,
                             USBH_CLASS_DEV_STATE_CONN);
        }
    }

    return (USBH_ERR_NONE);
}


/*
*********************************************************************************************************
*                                       USBH_ClassDrvDisconn()
*
* Description : Disconnect all the class drivers associated to the specified USB device.
*
* Argument(s) : p_dev   Pointer to USB device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  USBH_ClassDrvDisconn (USBH_DEV  *p_dev)
{
    CPU_INT08U       if_ix;
    CPU_INT08U       nbr_ifs;
    USBH_CFG        *p_cfg;
    USBH_IF         *p_if        = (USBH_IF *)0;
    USBH_CLASS_DRV  *p_class_drv;


    if ((p_dev->ClassDevPtr    != (void               *)0) &&   /* If class drv is present at dev level.                */
        (p_dev->ClassDrvRegPtr != (USBH_CLASS_DRV_REG *)0)) {
        p_class_drv = p_dev->ClassDrvRegPtr->ClassDrvPtr;

        if ((p_class_drv          != DEF_NULL) &&
            (p_class_drv->Disconn != DEF_NULL)) {
            USBH_ClassNotify(p_dev,
                             p_if,
                             p_dev->ClassDevPtr,
                             USBH_CLASS_DEV_STATE_DISCONN);

            p_class_drv->Disconn((void *)p_dev->ClassDevPtr);
        }

        p_dev->ClassDrvRegPtr = (USBH_CLASS_DRV_REG *)0;
        p_dev->ClassDevPtr    = (void               *)0;
        return;
    }

    p_cfg   = USBH_CfgGet(p_dev, 0u);                           /* Get first cfg.                                       */
    nbr_ifs = USBH_CfgIF_NbrGet(p_cfg);
    for (if_ix = 0u; if_ix < nbr_ifs; if_ix++) {                /* For all IFs present in first cfg.                    */
        p_if = USBH_IF_Get(p_cfg, if_ix);
        if (p_if == (USBH_IF *)0) {
            return;
        }

        if ((p_if->ClassDevPtr    != (void               *)0) &&
            (p_if->ClassDrvRegPtr != (USBH_CLASS_DRV_REG *)0)) {
            p_class_drv = p_if->ClassDrvRegPtr->ClassDrvPtr;

            if ((p_class_drv          != DEF_NULL) &&
                (p_class_drv->Disconn != DEF_NULL)) {
                USBH_ClassNotify(p_dev,
                                 p_if,
                                 p_if->ClassDevPtr,
                                 USBH_CLASS_DEV_STATE_DISCONN);

                                                                /* Disconnect the class driver.                         */
                p_class_drv->Disconn((void *)p_if->ClassDevPtr);
            }

            p_if->ClassDrvRegPtr = (USBH_CLASS_DRV_REG *)0;
            p_if->ClassDevPtr    = (void               *)0;
        }
    }
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         USBH_ClassProbeDev()
*
* Description : Find a class driver matching device descriptor of the USB device.
*
* Argument(s) : p_dev   Pointer to USB device.
*
* Return(s)   : USBH_ERR_NONE                   If class driver for this device was found.
*               USBH_ERR_CLASS_DRV_NOT_FOUND    If class driver for this device was not found.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_ClassProbeDev (USBH_DEV  *p_dev)
{
    CPU_INT32U       ix;
    USBH_CLASS_DRV  *p_class_drv;
    USBH_ERR         err;
    void            *p_class_dev;


    err = USBH_ERR_CLASS_DRV_NOT_FOUND;

    for (ix = 0u; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {     /* For each class drv present in list.                  */

        if (USBH_ClassDrvList[ix].InUse != 0) {
            p_class_drv = USBH_ClassDrvList[ix].ClassDrvPtr;

            if (p_class_drv->ProbeDev != (void *)0) {
                p_dev->ClassDrvRegPtr = &USBH_ClassDrvList[ix];
                p_class_dev           =  p_class_drv->ProbeDev(p_dev, &err);

                if (err == USBH_ERR_NONE) {
                    p_dev->ClassDevPtr = p_class_dev;           /* Drv found, store class dev ptr.                      */
                    return(err);
                }
                p_dev->ClassDrvRegPtr = (USBH_CLASS_DRV_REG *)0;
            }
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_ClassProbeIF()
*
* Description : Finds a class driver matching interface descriptor of an interface.
*
* Argument(s) : p_dev   Pointer to USB device.
*
*               p_if    Pointer to USB interface.
*
* Return(s)   : USBH_ERR_NONE                   If class driver for this interface was found.
*               USBH_ERR_CLASS_DRV_NOT_FOUND    If class driver for this interface was not found.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_ClassProbeIF (USBH_DEV *p_dev,
                                     USBH_IF  *p_if)
{
    CPU_INT32U       ix;
    USBH_CLASS_DRV  *p_class_drv;
    void            *p_class_dev;
    USBH_ERR         err;


    err = USBH_ERR_CLASS_DRV_NOT_FOUND;

    for (ix = 0u; ix < USBH_CFG_MAX_NBR_CLASS_DRVS; ix++) {     /* Search drv list for matching IF class.               */

        if (USBH_ClassDrvList[ix].InUse != 0u) {
            p_class_drv = USBH_ClassDrvList[ix].ClassDrvPtr;

            if (p_class_drv->ProbeIF != (void *)0) {
                p_if->ClassDrvRegPtr = &USBH_ClassDrvList[ix];
                p_class_dev          =  p_class_drv->ProbeIF(p_dev, p_if, &err);

                if (err == USBH_ERR_NONE) {
                    p_if->ClassDevPtr = p_class_dev;            /* Drv found, store class dev ptr.                      */
                    return (err);
                }

                p_if->ClassDrvRegPtr = (USBH_CLASS_DRV_REG *)0;
            }
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_ClassNotify()
*
* Description : Notifies application about connection and disconnection events.
*
* Argument(s) : p_dev           Pointer to USB device.
*
*               p_if            Pointer to USB interface.
*
*               p_class_dev     Pointer to class device structure.
*
*               is_conn         Flag indicating connection or disconnection.
*
* Return(s)   : None.
*
* Note(s)     : None
*********************************************************************************************************
*/

static  void  USBH_ClassNotify (USBH_DEV     *p_dev,
                                USBH_IF      *p_if,
                                void         *p_class_dev,
                                CPU_BOOLEAN   is_conn)
{
    USBH_CLASS_DRV_REG  *p_class_drv_reg;


    p_class_drv_reg = p_dev->ClassDrvRegPtr;

    if (p_class_drv_reg == (USBH_CLASS_DRV_REG *)0) {
        p_class_drv_reg = p_if->ClassDrvRegPtr;
    }

    if (p_class_drv_reg->NotifyFnctPtr != (USBH_CLASS_NOTIFY_FNCT)0) {
        p_class_drv_reg->NotifyFnctPtr(p_class_dev,             /* Call app notification callback fnct.                 */
                                       is_conn,
                                       p_class_drv_reg->NotifyArgPtr);
    }
}

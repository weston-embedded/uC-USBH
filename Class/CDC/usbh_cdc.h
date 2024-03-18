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
*                                  COMMUNICATIONS DEVICE CLASS (CDC)
*
* Filename : usbh_cdc.h
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/

#ifndef  USBH_CDC_MODULE_PRESENT
#define  USBH_CDC_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../Source/usbh_os.h"
#include  "../../Source/usbh_class.h"


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   USBH_CDC_MODULE
#define  USBH_CDC_EXT
#else
#define  USBH_CDC_EXT  extern
#endif


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  USBH_CDC_LEN_EVENT_BUF                           16u

#define  USBH_CDC_LEN_FNCTLHEADER_DESC                  0x05u   /* Len of functional hdr desc.                          */
#define  USBH_CDC_LEN_CALL_MANAGEMENT_FNCTL_DESC        0x05u   /* Len of call mgmt functional desc.                    */
#define  USBH_CDC_LEN_ACM_FNCTL_DESC                    0x04u   /* Len of ACM functional desc.                          */
#define  USBH_CDC_LEN_UNION_FNCTL_DESC                  0x05u   /* Len of union functional desc.                        */
#define  USBH_CDC_LEN_LINECODING                        0x07u   /* Len of line coding params.                           */
#define  USBH_CDC_LEN_SERSTATE                          0x0Au   /* Len of dev ser state.                                */
#define  USBH_CDC_LEN_RESPONSE_AVAIL                    0x08u   /* Len of dev response avail.                           */


/*
*********************************************************************************************************
*                          SUBCLASS CODES FOR COMMUNICATION INTERFACE CLASS
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.2,
*               Section 4.3, Table 4.
*********************************************************************************************************
*/

#define  USBH_CDC_CONTROL_SUBCLASS_CODE_DLCM            0x01u
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_ACM             0x02u
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_TCM             0x03u
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_MCCM            0x04u
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_CAPICM          0x05u
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_ENCM            0x06u
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_ATMNCM          0x07u
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_WHCM            0x08u  /* Wireless Handset Control Model.                      */
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_DEV_MGMT        0x09u  /* Device Management.                                   */
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_MDLM            0x0Au  /* Mobile Direct Line Model.                            */
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_OBEX            0x0Bu  /* OBEX.                                                */
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_EEM             0x0Cu  /* Ethernet Emulation Model.                            */
#define  USBH_CDC_CONTROL_SUBCLASS_CODE_NCM             0x0Du  /* Network Control Model.                               */


/*
*********************************************************************************************************
*                          PROTOCOL CODES FOR COMMUNICATION INTERFACE CLASS
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.2,
*               Section 4.4, Table 5.
*********************************************************************************************************
*/

#define  USBH_CDC_CONTROL_PROTOCOL_CODE_USB             0x00u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_V_25_AT         0x01u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_PCCA_101_AT     0x02u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_PCCA_101_O_AT   0x03u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_GSM_7_07_AT     0x04u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_3GPP_27_007_AT  0x05u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_TIA_CDMA_AT     0x06u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_EEM             0x07u
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_EXTERNAL        0xFEu
#define  USBH_CDC_CONTROL_PROTOCOL_CODE_VENDOR          0xFFu


/*
*********************************************************************************************************
*                               SUBCLASS CODES FOR DATA INTERFACE CLASS
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.2,
*               Section 4.6.
*********************************************************************************************************
*/

#define  USBH_CDC_DATA_SUBCLASS_CODE_NONE               0x00u


/*
*********************************************************************************************************
*                               PROTOCOL CODES FOR DATA INTERFACE CLASS
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.2,
*               Section 4.7, Table 7.
*********************************************************************************************************
*/

#define  USBH_CDC_DATA_PROTOCOL_CODE_USB                0x00u
#define  USBH_CDC_DATA_PROTOCOL_CODE_NTB                0x01u
#define  USBH_CDC_DATA_PROTOCOL_CODE_ISDN_BRI           0x30u
#define  USBH_CDC_DATA_PROTOCOL_CODE_ISO_IEC            0x31u
#define  USBH_CDC_DATA_PROTOCOL_CODE_TRANSPARENT        0x32u
#define  USBH_CDC_DATA_PROTOCOL_CODE_Q_921M             0x50u
#define  USBH_CDC_DATA_PROTOCOL_CODE_Q_921_NUM          0x51u
#define  USBH_CDC_DATA_PROTOCOL_CODE_Q921TM             0x52u
#define  USBH_CDC_DATA_PROTOCOL_CODE_V_42bits           0x90u
#define  USBH_CDC_DATA_PROTOCOL_CODE_E_ISDN             0x91u
#define  USBH_CDC_DATA_PROTOCOL_CODE_V_24_ISDN          0x92u
#define  USBH_CDC_DATA_PROTOCOL_CODE_CAPI               0x93u
#define  USBH_CDC_DATA_PROTOCOL_CODE_CDC                0xFEu
#define  USBH_CDC_DATA_PROTOCOL_CODE_TM_USB             0xFFu


/*
*********************************************************************************************************
*                                       FUNCTIONAL DESCRIPTORS
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.2,
*               Section 5.2.3.
*           (2) The functional descriptors descriptor type is given in "Table 13".
*           (3) The functional descriptors descriptor subtype is given in "Table 14".
*********************************************************************************************************
*/

#define  USBH_CDC_FNCTL_DESC_INTERFACE                  0x24u
#define  USBH_CDC_FNCTL_DESC_ENDPOINT                   0x25u

#define  USBH_CDC_FNCTL_DESC_SUB_HFD                    0x00u
#define  USBH_CDC_FNCTL_DESC_SUB_CMFD                   0x01u
#define  USBH_CDC_FNCTL_DESC_SUB_ACMFD                  0x02u
#define  USBH_CDC_FNCTL_DESC_SUB_DLMFD                  0x03u
#define  USBH_CDC_FNCTL_DESC_SUB_TRFD                   0x04u
#define  USBH_CDC_FNCTL_DESC_SUB_TC_LSRCFD              0x05u
#define  USBH_CDC_FNCTL_DESC_SUB_UFD                    0x06u
#define  USBH_CDC_FNCTL_DESC_SUB_CSFD                   0x07u
#define  USBH_CDC_FNCTL_DESC_SUB_TOMFD                  0x08u
#define  USBH_CDC_FNCTL_DESC_SUB_USB_TFD                0x09u
#define  USBH_CDC_FNCTL_DESC_SUB_NCTD                   0x0Au
#define  USBH_CDC_FNCTL_DESC_SUB_PUFD                   0x0Bu
#define  USBH_CDC_FNCTL_DESC_SUB_EUFD                   0x0Cu
#define  USBH_CDC_FNCTL_DESC_SUB_MCMFD                  0x0Du
#define  USBH_CDC_FNCTL_DESC_SUB_CAPI_CMFD              0x0Eu
#define  USBH_CDC_FNCTL_DESC_SUB_ENFD                   0x0Fu
#define  USBH_CDC_FNCTL_DESC_SUB_ATM_NFD                0x10u
#define  USBH_CDC_FNCTL_DESC_SUB_WHCMFD                 0x11u  /* Wireless Handset Control Model.                      */
#define  USBH_CDC_FNCTL_DESC_SUB_MDLMFD                 0x12u  /* Mobile Direct Line Model.                            */
#define  USBH_CDC_FNCTL_DESC_SUB_MDLMDFD                0x13u  /* Mobile Direct Line Model Details.                    */
#define  USBH_CDC_FNCTL_DESC_SUB_DEV_MGMTFD             0x14u  /* Device Management.                                   */
#define  USBH_CDC_FNCTL_DESC_SUB_OBEXFD                 0x15u  /* OBEX.                                                */
#define  USBH_CDC_FNCTL_DESC_SUB_CMDSETFD               0x16u  /* Command Set.                                         */
#define  USBH_CDC_FNCTL_DESC_SUB_CMDSETDFD              0x17u  /* Command Set Details.                                 */
#define  USBH_CDC_FNCTL_DESC_SUB_TCMFD                  0x18u  /* Telephone Control Model.                             */
#define  USBH_CDC_FNCTL_DESC_SUB_OBEXSIDFD              0x19u  /* OBEX Service Identifier.                             */
#define  USBH_CDC_FNCTL_DESC_SUB_NCMFD                  0x1Au  /* Network Control Model.                               */


/*
*********************************************************************************************************
*                                       CLASS-SPECIFIC REQUESTS
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.2 Section 6.2,
*               Table 19.
*
*           (2) The 'bRequest' field of a class-specific setup request may contain one of these values.
*
*           (3) The set line coding request is "used to configure the serial driver with new line coding
*               values".  The setup request packet will consist of :
*
*               (a) bmRequestType = 00100001b (class, interface, host-to-device)
*               (b) bRequest      =     0x20
*               (c) wValue        =   0x0000
*               (d) wIndex        = Interface number
*               (e) wLength       =     0x07
*
*           (4) The get line coding request is "used to know the serial driver line coding values".
*               The setup request packet will consist of :
*
*               (a) bmRequestType = 10100001b (class, interface, device-to-host)
*               (b) bRequest      =     0x21
*               (c) wValue        =   0x0000
*               (d) wIndex        = Interface number
*               (e) wLength       =     0x07
*
*           (5) The set control line state request is "used to know the presence of RTS and DTR".
*               The setup request packet will consist of :
*
*               (a) bmRequestType = 00100001b (class, interface, host-to-device)
*               (b) bRequest      =     0x23
*               (c) wValue        =   0x0000
*               (d) wIndex        = Interface number
*               (e) wLength       =     0x00
*
*               (f) The 'wValue' field is a bit mapped datum with three sub fields:
*                   (1) Bit 15-2: Reserved. Should be set to zero.
*                   (2) Bit 1   : Carrier control and corresponds to RS-232 signal RTS.
*                        (a) 0 =  Deactivate carrier.
*                        (b) 1 =  Activate carrier.
*
*                   (3) Bit 0   : Indicates to DCE if DTE is present or not and corresponds to RS-232
*                                 signal DTR.
*                        (a) 0 =  Not present.
*                        (b) 1 =  Present.
*
*           (6) The send break request is "used to generate the break".
*               The setup request packet will consist of :
*
*               (a) bmRequestType = 00100001b (class, interface, host-to-device)
*               (b) bRequest      =     0x24
*               (c) wValue        =   0x0000
*               (d) wIndex        = Interface number
*               (e) wLength       =     0x00
*
*               (f) The 'wValue' field indicates the duration of break in milli seconds.
*********************************************************************************************************
*/

#define  USBH_CDC_SEND_ENCAPSULATED_COMMAND             0x00u   /* Std brequest val for SEND_ENCAPSULATED_COMMAND.      */
#define  USBH_CDC_GET_ENCAPSULATED_RESPONSE             0x01u   /* Std brequest val for GET_ENCAPSULATED_RESPONSE.      */
#define  USBH_CDC_SET_COMM_FEATURE                      0x02u   /* Std brequest val for SET_COMM_FEATURE.               */
#define  USBH_CDC_GET_COMM_FEATURE                      0x03u   /* Std brequest val for GET_COMM_FEATURE.               */
#define  USBH_CDC_CLEAR_COMM_FEATURE                    0x04u   /* Std brequest val for CLEAR_COMM_FEATURE.             */
#define  USBH_CDC_SET_AUX_LINE_STATE                    0x10u   /* Std brequest val for SET_AUX_LINE_STATE.             */
#define  USBH_CDC_SET_HOOK_STATE                        0x11u   /* Std brequest val for SET_HOOK_STATE.                 */
#define  USBH_CDC_PULSE_SETUP                           0x12u   /* Std brequest val for PULSE_SETUP.                    */
#define  USBH_CDC_SEND_PULSE                            0x13u   /* Std brequest val for SEND_PULSE.                     */
#define  USBH_CDC_SET_PULSE_TIME                        0x14u   /* Std brequest val for SET_PULSE_TIME.                 */
#define  USBH_CDC_RING_AUX_JACK                         0x15u   /* Std brequest val for RING_AUX_JACK.                  */
#define  USBH_CDC_SET_LINECODING                        0x20u   /* Std brequest val for SET_LINE_CODING.                */
#define  USBH_CDC_GET_LINECODING                        0x21u   /* Std brequest val for GET_LINE_CODING.                */
#define  USBH_CDC_SET_CONTROL_LINESTATE                 0x22u   /* Std brequest val for SET_CONTROL_LINE_STATE.         */
#define  USBH_CDC_SEND_BREAK                            0x23u   /* Std brequest val for SEND_BREAK.                     */
#define  USBH_CDC_SET_RINGER_PARMS                      0x30u   /* Std brequest val for SET_RINGER_PARMS.               */
#define  USBH_CDC_GET_RINGER_PARMS                      0x31u   /* Std brequest val for GET_RINGER_PARMS.               */
#define  USBH_CDC_SET_OPERATION_PARMS                   0x32u   /* Std brequest val for SET_OPERATION_PARMS.            */
#define  USBH_CDC_GET_OPERATION_PARMS                   0x33u   /* Std brequest val for GET_OPERATION_PARMS.            */
#define  USBH_CDC_SET_LINE_PARMS                        0x34u   /* Std brequest val for SET_LINE_PARMS.                 */
#define  USBH_CDC_GET_LINE_PARMS                        0x35u   /* Std brequest val for GET_LINE_PARMS.                 */
#define  USBH_CDC_DIAL_DIGITS                           0x36u   /* Std brequest val for DIAL_DIGITS.                    */
#define  USBH_CDC_SET_UNIT_PARAMETER                    0x37u   /* Std brequest val for SET_UNIT_PARAMETER.             */
#define  USBH_CDC_GET_UNIT_PARAMETER                    0x38u   /* Std brequest val for GET_UNIT_PARAMETER.             */
#define  USBH_CDC_CLEAR_UNIT_PARAMETER                  0x39u   /* Std brequest val for CLEAR_UNIT_PARAMETER.           */
#define  USBH_CDC_GET_PROFILE                           0x3Au   /* Std brequest val for GET_PROFILE.                    */
#define  USBH_CDC_SET_ETHERNET_MULTICAST_FILTERS        0x40u   /* Std brequest val for SET_ETHERNET_MULTICAST_FILTERS. */
#define  USBH_CDC_SET_ETHERNET_POWER_MANAGEMENT_PATTERN 0x41u   /* Std brequest val for SET_ETHERNET_POWER_MANAGEMENT.  */
#define  USBH_CDC_GET_ETHERNET_POWER_MANAGEMENT_PATTERN 0x42u   /* Std brequest val for GET_ETHERNET_POWER_MANAGEMENT.  */
#define  USBH_CDC_SET_ETHERNET_PACKET_FILTER            0x43u   /* Std brequest val for SET_ETHERNET_PACKET_FILTER.     */
#define  USBH_CDC_GET_ETHERNET_STATISTIC                0x44u   /* Std brequest val for GET_ETHERNET_STATISTIC.         */
#define  USBH_CDC_SET_ATM_DATA_FORMAT                   0x50u   /* Std brequest val for SET_ATM_DATA_FORMAT.            */
#define  USBH_CDC_GET_ATM_DEVICE_STATISTICS             0x51u   /* Std brequest val for GET_ATM_DEVICE_STATISTICS.      */
#define  USBH_CDC_SET_ATM_DEFAULT_VC                    0x52u   /* Std brequest val for SET_ATM_DEFAULT_VC.             */
#define  USBH_CDC_GET_ATM_VC_STATISTICS                 0x53u   /* Std brequest val for GET_ATM_VC_STATISTICS.          */
#define  USBH_CDC_GET_NTB_PARAMETERS                    0x80u   /* Std brequest val for GET_NTB_PARAMETERS.             */
#define  USBH_CDC_GET_NET_ADDRESS                       0x81u   /* Std brequest val for GET_NET_ADDRESS.                */
#define  USBH_CDC_SET_NET_ADDRESS                       0x82u   /* Std brequest val for SET_NET_ADDRESS.                */
#define  USBH_CDC_GET_NTB_FORMAT                        0x83u   /* Std brequest val for GET_NTB_FORMAT.                 */
#define  USBH_CDC_SET_NTB_FORMAT                        0x84u   /* Std brequest val for SET_NTB_FORMAT.                 */
#define  USBH_CDC_GET_NTB_INPUT_SIZE                    0x85u   /* Std brequest val for GET_NTB_INPUT_SIZE.             */
#define  USBH_CDC_SET_NTB_INPUT_SIZE                    0x86u   /* Std brequest val for SET_NTB_INPUT_SIZE.             */
#define  USBH_CDC_GET_MAX_DATAGRAM_SIZE                 0x87u   /* Std brequest val for GET_MAX_DATAGRAM_SIZE.          */
#define  USBH_CDC_SET_MAX_DATAGRAM_SIZE                 0x88u   /* Std brequest val for SET_MAX_DATAGRAM_SIZE.          */
#define  USBH_CDC_GET_CRC_MODE                          0x89u   /* Std brequest val for GET_CRC_MODE.                   */
#define  USBH_CDC_SET_CRC_MODE                          0x8Au   /* Std brequest val for SET_CRC_MODE.                   */


/*
*********************************************************************************************************
*                                         NOTIFICATION EVENTS
*
* Note(s) : (1) See "USB Class Definitions for Communication Devices Specification", version 1.2 Section 6.3.
 *              Table 20.
*
*           (2) The 'bRequest' field of a notification event setup request may contain this value.
*
*           (3) The serial state notification is "used to send the current device state to host".
*               The notification has header structure of setup request packet format and serial state :
*
*               (a) bmRequestType = 10100001b (class, interface, device-to-host)
*               (b) bRequest      =     0x20
*               (c) wValue        =   0x0000
*               (d) wIndex        = Interface number
*               (e) wLength       =     0x0a
*
*           (4) The 'serialstate0' field is a bit mapped datum with eight subfields:
*               (a) Bit 15-7: Reserved.    Should be set to zero.
*               (b) Bit 6   : bOverRun.    Received data has been discarded due to overrun in the device.
*               (c) Bit 5   : bParity.     A parity error has occurred.
*               (d) Bit 4   : bFraming.    A framing error has occurred.
*               (e) Bit 3   : bRingSignal. State of ring signal detection of the device.
*               (f) Bit 2   : bBreak.      State of break detection mechanism of the device.
*               (g) Bit 1   : bTxCarrier.  State of transmission carrier and corresponds to RS-232
*                                          signal DSR.
*               (h) Bit 0   : bRxCarrier.  State of transmission carrier and corresponds to RS-232
*                                          signal DCD.
*********************************************************************************************************
*/

#define  USBH_CDC_NOTIFICATION_NET_CONN                 0x00u
#define  USBH_CDC_NOTIFICATION_RESP_AVAIL               0x01u
#define  USBH_CDC_NOTIFICATION_AUX_JACK_HOOK_STATE      0x08u
#define  USBH_CDC_NOTIFICATION_RING_DETECT              0x09u
#define  USBH_CDC_NOTIFICATION_SERIAL_STATE             0x20u
#define  USBH_CDC_NOTIFICATION_CALL_STATE_CHNG          0x28u
#define  USBH_CDC_NOTIFICATION_LINE_STATE_CHNG          0x29u
#define  USBH_CDC_NOTIFICATION_CONN_SPEED_CHNG          0x2Au


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/

typedef  void  (*SUBCLASS_NOTIFY) (void        *p_data,
                                   CPU_INT08U  *p_buf,
                                   CPU_INT32U   xfer_len,
                                   USBH_ERR     err);

typedef  struct  usbh_cdc_dev {
    USBH_DEV         *DevPtr;
    CPU_INT08U        State;
    CPU_INT08U        RefCnt;
    USBH_HMUTEX       HMutex;
    CPU_INT08U        EventNotifyBuf[USBH_CDC_LEN_EVENT_BUF];   /* Buf used to recv notifications of dev.               */

    USBH_EP           CIC_IntrIn;
    USBH_EP           DIC_BulkIn;
    USBH_EP           DIC_BulkOut;

    CPU_INT08U        Cfg_Nbr;
    USBH_IF          *CIC_IF_Ptr;
    CPU_INT08U        CIC_IF_Nbr;
    USBH_IF          *DIC_IF_Ptr;
    CPU_INT08U        DIC_IF_Nbr;

    SUBCLASS_NOTIFY   EvtNotifyPtr;
    void             *EvtNotifyArgPtr;
    SUBCLASS_NOTIFY   DataTxNotifyPtr;
    void             *DataTxNotifyArgPtr;
    SUBCLASS_NOTIFY   DataRxNotifyPtr;
    void             *DataRxNotifyArgPtr;
} USBH_CDC_DEV;


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/

USBH_CDC_EXT  USBH_CLASS_DRV  USBH_CDC_ClassDrv;


/*
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

USBH_ERR     USBH_CDC_RefAdd        (USBH_CDC_DEV     *p_cdc_dev);

USBH_ERR     USBH_CDC_RefRel        (USBH_CDC_DEV     *p_cdc_dev);

USBH_ERR     USBH_CDC_EvtNotifyReg  (USBH_CDC_DEV     *p_cdc_dev,
                                     SUBCLASS_NOTIFY   p_evt_notify,
                                     void             *p_data);

USBH_ERR     USBH_CDC_DevReg        (USBH_CDC_DEV     *p_cdc_dev,
                                     void             *p_data);

USBH_ERR     USBH_CDC_EventNotifyReg(USBH_CDC_DEV     *p_cdc_dev,
                                     SUBCLASS_NOTIFY   p_evt_notify,
                                     void             *p_arg);

USBH_ERR     USBH_CDC_SubclassGet   (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U       *p_subclass);

USBH_ERR     USBH_CDC_ProtocolGet   (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U       *p_protocol);

USBH_ERR     USBH_CDC_CmdTx         (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U        b_req,
                                     CPU_INT08U        bm_req_type,
                                     CPU_INT16U        w_val,
                                     void             *p_buf,
                                     CPU_INT32U        buf_len);

USBH_ERR     USBH_CDC_RespRx        (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U        b_req,
                                     CPU_INT08U        bm_req_type,
                                     CPU_INT16U        w_val,
                                     void             *p_buf,
                                     CPU_INT32U        buf_len);

CPU_INT32U   USBH_CDC_DataTx        (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U       *p_buf,
                                     CPU_INT32U        buf_len,
                                     USBH_ERR          *p_err);

USBH_ERR     USBH_CDC_DataTxAsync   (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U       *p_buf,
                                     CPU_INT32U        buf_len,
                                     SUBCLASS_NOTIFY   tx_cmpl_notify,
                                     void             *p_arg);

CPU_INT32U   USBH_CDC_DataRx        (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U       *p_buf,
                                     CPU_INT32U        buf_len,
                                     USBH_ERR          *p_err);

USBH_ERR     USBH_CDC_DataRxAsync   (USBH_CDC_DEV     *p_cdc_dev,
                                     CPU_INT08U       *p_buf,
                                     CPU_INT32U        buf_len,
                                     SUBCLASS_NOTIFY   rx_cmpl_notify,
                                     void             *p_arg);

USBH_IF     *USBH_CDC_CommIF_Get    (USBH_CDC_DEV     *p_cdc_dev);


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                              MODULE END
*********************************************************************************************************
*/

#endif

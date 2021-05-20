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
*                                      MASS STORAGE CLASS (MSC)
*
* Filename : usbh_msc.c
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   USBH_MSC_MODULE
#define   MICRIUM_SOURCE
#include  "usbh_msc.h"
#include  "../../Source/usbh_core.h"


/*
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  USBH_MSC_SIG_CBW                             0x43425355u
#define  USBH_MSC_SIG_CSW                             0x53425355u

#define  USBH_MSC_LEN_CBW                                     31u
#define  USBH_MSC_LEN_CSW                                     13u

#define  USBH_MSC_MAX_TRANSFER_RETRY                        1000u


/*
*********************************************************************************************************
*                                           SUBCLASS CODES
*
* Note(s) : (1) See 'USB Mass Storage Class Specification Overview', Revision 1.2, Section 2.
*********************************************************************************************************
*/

#define  USBH_MSC_SUBCLASS_CODE_RBC                         0x01u
#define  USBH_MSC_SUBCLASS_CODE_SFF_8020i                   0x02u
#define  USBH_MSC_SUBCLASS_CODE_MMC_2                       0x02u
#define  USBH_MSC_SUBCLASS_CODE_QIC_157                     0x03u
#define  USBH_MSC_SUBCLASS_CODE_UFI                         0x04u
#define  USBH_MSC_SUBCLASS_CODE_SFF_8070i                   0x05u
#define  USBH_MSC_SUBCLASS_CODE_SCSI                        0x06u


/*
*********************************************************************************************************
*                                           PROTOCOL CODES
*
* Note(s) : (1) See 'USB Mass Storage Class Specification Overview', Revision 1.2, Section 3.
*********************************************************************************************************
*/

#define  USBH_MSC_PROTOCOL_CODE_CTRL_BULK_INTR_CMD_INTR     0x00u
#define  USBH_MSC_PROTOCOL_CODE_CTRL_BULK_INTR              0x01u
#define  USBH_MSC_PROTOCOL_CODE_BULK_ONLY                   0x50u


/*
*********************************************************************************************************
*                                       CLASS-SPECIFIC REQUESTS
*
* Note(s) : (1) See 'USB Mass Storage Class - Bulk Only Transport', Section 3.
*
*           (2) The 'bRequest' field of a class-specific setup request may contain one of these values.
*
*           (3) The mass storage reset request is "used to reset the mass storage device and its
*               associated interface".  The setup request packet will consist of :
*
*               (a) bmRequestType = 00100001b (class, interface, host-to-device)
*               (b) bRequest      =     0xFF
*               (c) wValue        =   0x0000
*               (d) wIndex        = Interface number
*               (e) wLength       =   0x0000
*
*           (4) The get max LUN is used to determine the number of LUN's supported by the device.  The
*               setup request packet will consist of :
*
*               (a) bmRequestType = 10100001b (class, interface, device-to-host)
*               (b) bRequest      =     0xFE
*               (c) wValue        =   0x0000
*               (d) wIndex        = Interface number
*               (e) wLength       =   0x0001
*********************************************************************************************************
*/

#define  USBH_MSC_REQ_MASS_STORAGE_RESET                    0xFFu   /* See Note #3.                                        */
#define  USBH_MSC_REQ_GET_MAX_LUN                           0xFEu   /* See Note #4.                                        */


/*
*********************************************************************************************************
*                                      COMMAND BLOCK FLAG VALUES
*
* Note(s) : (1) See 'USB Mass Storage Class - Bulk Only Transport', Section 5.1.
*
*           (2) The 'bmCBWFlags' field of a command block wrapper may contain one of these values.
*********************************************************************************************************
*/

#define  USBH_MSC_BMCBWFLAGS_DIR_HOST_TO_DEVICE             0x00u
#define  USBH_MSC_BMCBWFLAGS_DIR_DEVICE_TO_HOST             0x80u


/*
*********************************************************************************************************
*                                     COMMAND BLOCK STATUS VALUES
*
* Note(s) : (1) See 'USB Mass Storage Class - Bulk Only Transport', Section 5.3, Table 5.3.
*
*           (2) The 'bCSWStatus' field of a command status wrapper may contain one of these values.
*********************************************************************************************************
*/

#define  USBH_MSC_BCSWSTATUS_CMD_PASSED                     0x00u
#define  USBH_MSC_BCSWSTATUS_CMD_FAILED                     0x01u
#define  USBH_MSC_BCSWSTATUS_PHASE_ERROR                    0x02u


/*
*********************************************************************************************************
*                                            SCSI DEFINES
*********************************************************************************************************
*/

                                                                /* ------------------ SCSI OPCODES ------------------------ */
#define  USBH_SCSI_CMD_TEST_UNIT_READY                      0x00u
#define  USBH_SCSI_CMD_REWIND                               0x01u
#define  USBH_SCSI_CMD_REZERO_UNIT                          0x01u
#define  USBH_SCSI_CMD_REQUEST_SENSE                        0x03u
#define  USBH_SCSI_CMD_FORMAT_UNIT                          0x04u
#define  USBH_SCSI_CMD_FORMAT_MEDIUM                        0x04u
#define  USBH_SCSI_CMD_FORMAT                               0x04u
#define  USBH_SCSI_CMD_READ_BLOCK_LIMITS                    0x05u
#define  USBH_SCSI_CMD_REASSIGN_BLOCKS                      0x07u
#define  USBH_SCSI_CMD_INITIALIZE_ELEMENT_STATUS            0x07u
#define  USBH_SCSI_CMD_READ_06                              0x08u
#define  USBH_SCSI_CMD_RECEIVE                              0x08u
#define  USBH_SCSI_CMD_GET_MESSAGE_06                       0x08u
#define  USBH_SCSI_CMD_WRITE_06                             0x0Au
#define  USBH_SCSI_CMD_SEND_06                              0x0Au
#define  USBH_SCSI_CMD_SEND_MESSAGE_06                      0x0Au
#define  USBH_SCSI_CMD_PRINT                                0x0Au
#define  USBH_SCSI_CMD_SEEK_06                              0x0Bu
#define  USBH_SCSI_CMD_SET_CAPACITY                         0x0Bu
#define  USBH_SCSI_CMD_SLEW_AND_PRINT                       0x0Bu
#define  USBH_SCSI_CMD_READ_REVERSE_06                      0x0Fu

#define  USBH_SCSI_CMD_WRITE_FILEMARKS_06                   0x10u
#define  USBH_SCSI_CMD_SYNCHRONIZE_BUFFER                   0x10u
#define  USBH_SCSI_CMD_SPACE_06                             0x11u
#define  USBH_SCSI_CMD_INQUIRY                              0x12u
#define  USBH_SCSI_CMD_VERIFY_06                            0x13u
#define  USBH_SCSI_CMD_RECOVER_BUFFERED_DATA                0x14u
#define  USBH_SCSI_CMD_MODE_SELECT_06                       0x15u
#define  USBH_SCSI_CMD_RESERVE_06                           0x16u
#define  USBH_SCSI_CMD_RESERVE_ELEMENT_06                   0x16u
#define  USBH_SCSI_CMD_RELEASE_06                           0x17u
#define  USBH_SCSI_CMD_RELEASE_ELEMENT_06                   0x17u
#define  USBH_SCSI_CMD_COPY                                 0x18u
#define  USBH_SCSI_CMD_ERASE_06                             0x19u
#define  USBH_SCSI_CMD_MODE_SENSE_06                        0x1Au
#define  USBH_SCSI_CMD_START_STOP_UNIT                      0x1Bu
#define  USBH_SCSI_CMD_LOAD_UNLOAD                          0x1Bu
#define  USBH_SCSI_CMD_SCAN_06                              0x1Bu
#define  USBH_SCSI_CMD_STOP_PRINT                           0x1Bu
#define  USBH_SCSI_CMD_OPEN_CLOSE_IMPORT_EXPORT_ELEMENT     0x1Bu
#define  USBH_SCSI_CMD_RECEIVE_DIAGNOSTIC_RESULTS           0x1Cu
#define  USBH_SCSI_CMD_SEND_DIAGNOSTIC                      0x1Du
#define  USBH_SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL         0x1Eu

#define  USBH_SCSI_CMD_READ_FORMAT_CAPACITIES               0x23u
#define  USBH_SCSI_CMD_SET_WINDOW                           0x24u
#define  USBH_SCSI_CMD_READ_CAPACITY_10                     0x25u
#define  USBH_SCSI_CMD_READ_CAPACITY                        0x25u
#define  USBH_SCSI_CMD_READ_CARD_CAPACITY                   0x25u
#define  USBH_SCSI_CMD_GET_WINDOW                           0x25u
#define  USBH_SCSI_CMD_READ_10                              0x28u
#define  USBH_SCSI_CMD_GET_MESSAGE_10                       0x28u
#define  USBH_SCSI_CMD_READ_GENERATION                      0x29u
#define  USBH_SCSI_CMD_WRITE_10                             0x2Au
#define  USBH_SCSI_CMD_SEND_10                              0x2Au
#define  USBH_SCSI_CMD_SEND_MESSAGE_10                      0x2Au
#define  USBH_SCSI_CMD_SEEK_10                              0x2Bu
#define  USBH_SCSI_CMD_LOCATE_10                            0x2Bu
#define  USBH_SCSI_CMD_POSITION_TO_ELEMENT                  0x2Bu
#define  USBH_SCSI_CMD_ERASE_10                             0x2Cu
#define  USBH_SCSI_CMD_READ_UPDATED_BLOCK                   0x2Du
#define  USBH_SCSI_CMD_WRITE_AND_VERIFY_10                  0x2Eu
#define  USBH_SCSI_CMD_VERIFY_10                            0x2Fu

#define  USBH_SCSI_CMD_SEARCH_DATA_HIGH_10                  0x30u
#define  USBH_SCSI_CMD_SEARCH_DATA_EQUAL_10                 0x31u
#define  USBH_SCSI_CMD_OBJECT_POSITION                      0x31u
#define  USBH_SCSI_CMD_SEARCH_DATA_LOW_10                   0x32u
#define  USBH_SCSI_CMD_SET_LIMITS_10                        0x33u
#define  USBH_SCSI_CMD_PRE_FETCH_10                         0x34u
#define  USBH_SCSI_CMD_READ_POSITION                        0x34u
#define  USBH_SCSI_CMD_GET_DATA_BUFFER_STATUS               0x34u
#define  USBH_SCSI_CMD_SYNCHRONIZE_CACHE_10                 0x35u
#define  USBH_SCSI_CMD_LOCK_UNLOCK_CACHE_10                 0x36u
#define  USBH_SCSI_CMD_READ_DEFECT_DATA_10                  0x37u
#define  USBH_SCSI_CMD_INIT_ELEMENT_STATUS_WITH_RANGE       0x37u
#define  USBH_SCSI_CMD_MEDIUM_SCAN                          0x38u
#define  USBH_SCSI_CMD_COMPARE                              0x39u
#define  USBH_SCSI_CMD_COPY_AND_VERIFY                      0x3Au
#define  USBH_SCSI_CMD_WRITE_BUFFER                         0x3Bu
#define  USBH_SCSI_CMD_READ_BUFFER                          0x3Cu
#define  USBH_SCSI_CMD_UPDATE_BLOCK                         0x3Du
#define  USBH_SCSI_CMD_READ_LONG_10                         0x3Eu
#define  USBH_SCSI_CMD_WRITE_LONG_10                        0x3Fu

#define  USBH_SCSI_CMD_CHANGE_DEFINITION                    0x40u
#define  USBH_SCSI_CMD_WRITE_SAME_10                        0x41u
#define  USBH_SCSI_CMD_READ_SUBCHANNEL                      0x42u
#define  USBH_SCSI_CMD_READ_TOC_PMA_ATIP                    0x43u
#define  USBH_SCSI_CMD_REPORT_DENSITY_SUPPORT               0x44u
#define  USBH_SCSI_CMD_READ_HEADER                          0x44u
#define  USBH_SCSI_CMD_PLAY_AUDIO_10                        0x45u
#define  USBH_SCSI_CMD_GET_CONFIGURATION                    0x46u
#define  USBH_SCSI_CMD_PLAY_AUDIO_MSF                       0x47u
#define  USBH_SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION        0x4Au
#define  USBH_SCSI_CMD_PAUSE_RESUME                         0x4Bu
#define  USBH_SCSI_CMD_LOG_SELECT                           0x4Cu
#define  USBH_SCSI_CMD_LOG_SENSE                            0x4Du
#define  USBH_SCSI_CMD_STOP_PLAY_SCAN                       0x4Eu

#define  USBH_SCSI_CMD_XDWRITE_10                           0x50u
#define  USBH_SCSI_CMD_XPWRITE_10                           0x51u
#define  USBH_SCSI_CMD_READ_DISC_INFORMATION                0x51u
#define  USBH_SCSI_CMD_XDREAD_10                            0x52u
#define  USBH_SCSI_CMD_READ_TRACK_INFORMATION               0x52u
#define  USBH_SCSI_CMD_RESERVE_TRACK                        0x53u
#define  USBH_SCSI_CMD_SEND_OPC_INFORMATION                 0x54u
#define  USBH_SCSI_CMD_MODE_SELECT_10                       0x55u
#define  USBH_SCSI_CMD_RESERVE_10                           0x56u
#define  USBH_SCSI_CMD_RESERVE_ELEMENT_10                   0x56u
#define  USBH_SCSI_CMD_RELEASE_10                           0x57u
#define  USBH_SCSI_CMD_RELEASE_ELEMENT_10                   0x57u
#define  USBH_SCSI_CMD_REPAIR_TRACK                         0x58u
#define  USBH_SCSI_CMD_MODE_SENSE_10                        0x5Au
#define  USBH_SCSI_CMD_CLOSE_TRACK_SESSION                  0x5Bu
#define  USBH_SCSI_CMD_READ_BUFFER_CAPACITY                 0x5Cu
#define  USBH_SCSI_CMD_SEND_CUE_SHEET                       0x5Du
#define  USBH_SCSI_CMD_PERSISTENT_RESERVE_IN                0x5Eu
#define  USBH_SCSI_CMD_PERSISTENT_RESERVE_OUT               0x5Fu

#define  USBH_SCSI_CMD_EXTENDED_CDB                         0x7Eu
#define  USBH_SCSI_CMD_VARIABLE_LENGTH_CDB                  0x7Fu

#define  USBH_SCSI_CMD_XDWRITE_EXTENDED_16                  0x80u
#define  USBH_SCSI_CMD_WRITE_FILEMARKS_16                   0x80u
#define  USBH_SCSI_CMD_REBUILD_16                           0x81u
#define  USBH_SCSI_CMD_READ_REVERSE_16                      0x81u
#define  USBH_SCSI_CMD_REGENERATE_16                        0x82u
#define  USBH_SCSI_CMD_EXTENDED_COPY                        0x83u
#define  USBH_SCSI_CMD_RECEIVE_COPY_RESULTS                 0x84u
#define  USBH_SCSI_CMD_ATA_COMMAND_PASS_THROUGH_16          0x85u
#define  USBH_SCSI_CMD_ACCESS_CONTROL_IN                    0x86u
#define  USBH_SCSI_CMD_ACCESS_CONTROL_OUT                   0x87u
#define  USBH_SCSI_CMD_READ_16                              0x88u
#define  USBH_SCSI_CMD_WRITE_16                             0x8Au
#define  USBH_SCSI_CMD_ORWRITE                              0x8Bu
#define  USBH_SCSI_CMD_READ_ATTRIBUTE                       0x8Cu
#define  USBH_SCSI_CMD_WRITE_ATTRIBUTE                      0x8Du
#define  USBH_SCSI_CMD_WRITE_AND_VERIFY_16                  0x8Eu
#define  USBH_SCSI_CMD_VERIFY_16                            0x8Fu

#define  USBH_SCSI_CMD_PREFETCH_16                          0x90u
#define  USBH_SCSI_CMD_SYNCHRONIZE_CACHE_16                 0x91u
#define  USBH_SCSI_CMD_SPACE_16                             0x91u
#define  USBH_SCSI_CMD_LOCK_UNLOCK_CACHE_16                 0x92u
#define  USBH_SCSI_CMD_LOCATE_16                            0x92u
#define  USBH_SCSI_CMD_WRITE_SAME_16                        0x93u
#define  USBH_SCSI_CMD_ERASE_16                             0x93u
#define  USBH_SCSI_CMD_SERVICE_ACTION_IN_16                 0x9Eu
#define  USBH_SCSI_CMD_SERVICE_ACTION_OUT_16                0x9Fu

#define  USBH_SCSI_CMD_REPORT_LUNS                          0xA0u
#define  USBH_SCSI_CMD_BLANK                                0xA1u
#define  USBH_SCSI_CMD_ATA_COMMAND_PASS_THROUGH_12          0xA1u
#define  USBH_SCSI_CMD_SECURITY_PROTOCOL_IN                 0xA2u
#define  USBH_SCSI_CMD_MAINTENANCE_IN                       0xA3u
#define  USBH_SCSI_CMD_SEND_KEY                             0xA3u
#define  USBH_SCSI_CMD_MAINTENANCE_OUT                      0xA4u
#define  USBH_SCSI_CMD_REPORT_KEY                           0xA4u
#define  USBH_SCSI_CMD_MOVE_MEDIUM                          0xA5u
#define  USBH_SCSI_CMD_PLAY_AUDIO_12                        0xA5u
#define  USBH_SCSI_CMD_EXCHANGE_MEDIUM                      0xA6u
#define  USBH_SCSI_CMD_LOAD_UNLOAD_CDVD                     0xA6u
#define  USBH_SCSI_CMD_MOVE_MEDIUM_ATTACHED                 0xA7u
#define  USBH_SCSI_CMD_SET_READ_AHEAD                       0xA7u
#define  USBH_SCSI_CMD_READ_12                              0xA8u
#define  USBH_SCSI_CMD_GET_MESSAGE_12                       0xA8u
#define  USBH_SCSI_CMD_SERVICE_ACTION_OUT_12                0xA9u
#define  USBH_SCSI_CMD_WRITE_12                             0xAAu
#define  USBH_SCSI_CMD_SEND_MESSAGE_12                      0xAAu
#define  USBH_SCSI_CMD_SERVICE_ACTION_IN_12                 0xABu
#define  USBH_SCSI_CMD_ERASE_12                             0xACu
#define  USBH_SCSI_CMD_GET_PERFORMANCE                      0xACu
#define  USBH_SCSI_CMD_READ_DVD_STRUCTURE                   0xADu
#define  USBH_SCSI_CMD_WRITE_AND_VERIFY_12                  0xAEu
#define  USBH_SCSI_CMD_VERIFY_12                            0xAFu

#define  USBH_SCSI_CMD_SEARCH_DATA_HIGH_12                  0xB0u
#define  USBH_SCSI_CMD_SEARCH_DATA_EQUAL_12                 0xB1u
#define  USBH_SCSI_CMD_SEARCH_DATA_LOW_12                   0xB2u
#define  USBH_SCSI_CMD_SET_LIMITS_12                        0xB3u
#define  USBH_SCSI_CMD_READ_ELEMENT_STATUS_ATTACHED         0xB4u
#define  USBH_SCSI_CMD_SECURITY_PROTOCOL_OUT                0xB5u
#define  USBH_SCSI_CMD_REQUEST_VOLUME_ELEMENT_ADDRESS       0xB5u
#define  USBH_SCSI_CMD_SEND_VOLUME_TAG                      0xB6u
#define  USBH_SCSI_CMD_SET_STREAMING                        0xB6u
#define  USBH_SCSI_CMD_READ_DEFECT_DATA_12                  0xB7u
#define  USBH_SCSI_CMD_READ_ELEMENT_STATUS                  0xB8u
#define  USBH_SCSI_CMD_READ_CD_MSF                          0xB9u
#define  USBH_SCSI_CMD_REDUNDANCY_GROUP_IN                  0xBAu
#define  USBH_SCSI_CMD_SCAN                                 0xBAu
#define  USBH_SCSI_CMD_REDUNDANCY_GROUP_OUT                 0xBBu
#define  USBH_SCSI_CMD_SET_CD_SPEED                         0xBBu
#define  USBH_SCSI_CMD_SPARE_IN                             0xBCu
#define  USBH_SCSI_CMD_SPARE_OUT                            0xBDu
#define  USBH_SCSI_CMD_MECHANISM_STATUS                     0xBDu
#define  USBH_SCSI_CMD_VOLUME_SET_IN                        0xBEu
#define  USBH_SCSI_CMD_READ_CD                              0xBEu
#define  USBH_SCSI_CMD_VOLUME_SET_OUT                       0xBFu
#define  USBH_SCSI_CMD_SEND_DVD_STRUCTURE                   0xBFu

                                                                /* ------------------- SCSI STATUS CODES ------------------ */
#define  USBH_SCSI_STATUS_GOOD                              0x00u
#define  USBH_SCSI_STATUS_CHECK_CONDITION                   0x02u
#define  USBH_SCSI_STATUS_CONDITION_MET                     0x04u
#define  USBH_SCSI_STATUS_BUSY                              0x08u
#define  USBH_SCSI_STATUS_RESERVATION_CONFLICT              0x18u
#define  USBH_SCSI_STATUS_TASK_SET_FULL                     0x28u
#define  USBH_SCSI_STATUS_ACA_ACTIVE                        0x30u
#define  USBH_SCSI_STATUS_TASK_ABORTED                      0x40u

                                                                /* ------------------- SCSI SENSE KEYS -------------------- */
#define  USBH_SCSI_SENSE_KEY_NO_SENSE                       0x00u
#define  USBH_SCSI_SENSE_KEY_RECOVERED_ERROR                0x01u
#define  USBH_SCSI_SENSE_KEY_NOT_RDY                        0x02u
#define  USBH_SCSI_SENSE_KEY_MEDIUM_ERROR                   0x03u
#define  USBH_SCSI_SENSE_KEY_HARDWARE_ERROR                 0x04u
#define  USBH_SCSI_SENSE_KEY_ILLEGAL_REQUEST                0x05u
#define  USBH_SCSI_SENSE_KEY_UNIT_ATTENTION                 0x06u
#define  USBH_SCSI_SENSE_KEY_DATA_PROTECT                   0x07u
#define  USBH_SCSI_SENSE_KEY_BLANK_CHECK                    0x08u
#define  USBH_SCSI_SENSE_KEY_VENDOR_SPECIFIC                0x09u
#define  USBH_SCSI_SENSE_KEY_COPY_ABORTED                   0x0Au
#define  USBH_SCSI_SENSE_KEY_ABORTED_COMMAND                0x0Bu
#define  USBH_SCSI_SENSE_KEY_VOLUME_OVERFLOW                0x0Du
#define  USBH_SCSI_SENSE_KEY_MISCOMPARE                     0x0Eu

                                                                /* ------------- SCSI ADDITIONAL SENSE CODES -------------- */
#define  USBH_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO             0x00u
#define  USBH_SCSI_ASC_NO_INDEX_SECTOR_SIGNAL               0x01u
#define  USBH_SCSI_ASC_NO_SEEK_COMPLETE                     0x02u
#define  USBH_SCSI_ASC_PERIPHERAL_DEV_WR_FAULT              0x03u
#define  USBH_SCSI_ASC_LOG_UNIT_NOT_RDY                     0x04u
#define  USBH_SCSI_ASC_LOG_UNIT_NOT_RESPOND_TO_SELECTION    0x05u
#define  USBH_SCSI_ASC_NO_REFERENCE_POSITION_FOUND          0x06u
#define  USBH_SCSI_ASC_MULTIPLE_PERIPHERAL_DEVS_SELECTED    0x07u
#define  USBH_SCSI_ASC_LOG_UNIT_COMMUNICATION_FAIL          0x08u
#define  USBH_SCSI_ASC_TRACK_FOLLOWING_ERR                  0x09u
#define  USBH_SCSI_ASC_ERR_LOG_OVERFLOW                     0x0Au
#define  USBH_SCSI_ASC_WARNING                              0x0Bu
#define  USBH_SCSI_ASC_WR_ERR                               0x0Cu
#define  USBH_SCSI_ASC_ERR_DETECTED_BY_THIRD_PARTY          0x0Du
#define  USBH_SCSI_ASC_INVALID_INFO_UNIT                    0x0Eu

#define  USBH_SCSI_ASC_ID_CRC_OR_ECC_ERR                    0x10u
#define  USBH_SCSI_ASC_UNRECOVERED_RD_ERR                   0x11u
#define  USBH_SCSI_ASC_ADDR_MARK_NOT_FOUND_FOR_ID           0x12u
#define  USBH_SCSI_ASC_ADDR_MARK_NOT_FOUND_FOR_DATA         0x13u
#define  USBH_SCSI_ASC_RECORDED_ENTITY_NOT_FOUND            0x14u
#define  USBH_SCSI_ASC_RANDOM_POSITIONING_ERR               0x15u
#define  USBH_SCSI_ASC_DATA_SYNCHRONIZATION_MARK_ERR        0x16u
#define  USBH_SCSI_ASC_RECOVERED_DATA_NO_ERR_CORRECT        0x17u
#define  USBH_SCSI_ASC_RECOVERED_DATA_ERR_CORRECT           0x18u
#define  USBH_SCSI_ASC_DEFECT_LIST_ERR                      0x19u
#define  USBH_SCSI_ASC_PARAMETER_LIST_LENGTH_ERR            0x1Au
#define  USBH_SCSI_ASC_SYNCHRONOUS_DATA_TRANSFER_ERR        0x1Bu
#define  USBH_SCSI_ASC_DEFECT_LIST_NOT_FOUND                0x1Cu
#define  USBH_SCSI_ASC_MISCOMPARE_DURING_VERIFY_OP          0x1Du
#define  USBH_SCSI_ASC_RECOVERED_ID_WITH_ECC_CORRECTION     0x1Eu
#define  USBH_SCSI_ASC_PARTIAL_DEFECT_LIST_TRANSFER         0x1Fu

#define  USBH_SCSI_ASC_INVALID_CMD_OP_CODE                  0x20u
#define  USBH_SCSI_ASC_LOG_BLOCK_ADDR_OUT_OF_RANGE          0x21u
#define  USBH_SCSI_ASC_ILLEGAL_FUNCTION                     0x22u
#define  USBH_SCSI_ASC_INVALID_FIELD_IN_CDB                 0x24u
#define  USBH_SCSI_ASC_LOG_UNIT_NOT_SUPPORTED               0x25u
#define  USBH_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST      0x26u
#define  USBH_SCSI_ASC_WR_PROTECTED                         0x27u
#define  USBH_SCSI_ASC_NOT_RDY_TO_RDY_CHANGE                0x28u
#define  USBH_SCSI_ASC_POWER_ON_OR_BUS_DEV_RESET            0x29u
#define  USBH_SCSI_ASC_PARAMETERS_CHANGED                   0x2Au
#define  USBH_SCSI_ASC_CANNOT_COPY_CANNOT_DISCONNECT        0x2Bu
#define  USBH_SCSI_ASC_CMD_SEQUENCE_ERR                     0x2Cu
#define  USBH_SCSI_ASC_OVERWR_ERR_ON_UPDATE_IN_PLACE        0x2Du
#define  USBH_SCSI_ASC_INSUFFICIENT_TIME_FOR_OP             0x2Eu
#define  USBH_SCSI_ASC_CMDS_CLEARED_BY_ANOTHER_INIT         0x2Fu

#define  USBH_SCSI_ASC_INCOMPATIBLE_MEDIUM_INSTALLED        0x30u
#define  USBH_SCSI_ASC_MEDIUM_FORMAT_CORRUPTED              0x31u
#define  USBH_SCSI_ASC_NO_DEFECT_SPARE_LOCATION_AVAIL       0x32u
#define  USBH_SCSI_ASC_TAPE_LENGTH_ERR                      0x33u
#define  USBH_SCSI_ASC_ENCLOSURE_FAIL                       0x34u
#define  USBH_SCSI_ASC_ENCLOSURE_SERVICES_FAIL              0x35u
#define  USBH_SCSI_ASC_RIBBON_INK_OR_TONER_FAIL             0x36u
#define  USBH_SCSI_ASC_ROUNDED_PARAMETER                    0x37u
#define  USBH_SCSI_ASC_EVENT_STATUS_NOTIFICATION            0x38u
#define  USBH_SCSI_ASC_SAVING_PARAMETERS_NOT_SUPPORTED      0x39u
#define  USBH_SCSI_ASC_MEDIUM_NOT_PRESENT                   0x3Au
#define  USBH_SCSI_ASC_SEQUENTIAL_POSITIONING_ERR           0x3Bu
#define  USBH_SCSI_ASC_INVALID_BITS_IN_IDENTIFY_MSG         0x3Du
#define  USBH_SCSI_ASC_LOG_UNIT_HAS_NOT_SELF_CFG_YET        0x3Eu
#define  USBH_SCSI_ASC_TARGET_OP_CONDITIONS_HAVE_CHANGED    0x3Fu

#define  USBH_SCSI_ASC_RAM_FAIL                             0x40u
#define  USBH_SCSI_ASC_DATA_PATH_FAIL                       0x41u
#define  USBH_SCSI_ASC_POWER_ON_SELF_TEST_FAIL              0x42u
#define  USBH_SCSI_ASC_MSG_ERR                              0x43u
#define  USBH_SCSI_ASC_INTERNAL_TARGET_FAIL                 0x44u
#define  USBH_SCSI_ASC_SELECT_OR_RESELECT_FAIL              0x45u
#define  USBH_SCSI_ASC_UNSUCCESSFUL_SOFT_RESET              0x46u
#define  USBH_SCSI_ASC_SCSI_PARITY_ERR                      0x47u
#define  USBH_SCSI_ASC_INIT_DETECTED_ERR_MSG_RECEIVED       0x48u
#define  USBH_SCSI_ASC_INVALID_MSG_ERR                      0x49u
#define  USBH_SCSI_ASC_CMD_PHASE_ERR                        0x4Au
#define  USBH_SCSI_ASC_DATA_PHASE_ERR                       0x4Bu
#define  USBH_SCSI_ASC_LOG_UNIT_FAILED_SELF_CFG             0x4Cu
#define  USBH_SCSI_ASC_OVERLAPPED_CMDS_ATTEMPTED            0x4Eu

#define  USBH_SCSI_ASC_WR_APPEND_ERR                        0x50u
#define  USBH_SCSI_ASC_ERASE_FAIL                           0x51u
#define  USBH_SCSI_ASC_CARTRIDGE_FAULT                      0x52u
#define  USBH_SCSI_ASC_MEDIA_LOAD_OR_EJECT_FAILED           0x53u
#define  USBH_SCSI_ASC_SCSI_TO_HOST_SYSTEM_IF_FAIL          0x54u
#define  USBH_SCSI_ASC_SYSTEM_RESOURCE_FAIL                 0x55u
#define  USBH_SCSI_ASC_UNABLE_TO_RECOVER_TOC                0x57u
#define  USBH_SCSI_ASC_GENERATION_DOES_NOT_EXIST            0x58u
#define  USBH_SCSI_ASC_UPDATED_BLOCK_RD                     0x59u
#define  USBH_SCSI_ASC_OP_REQUEST_OR_STATE_CHANGE_INPUT     0x5Au
#define  USBH_SCSI_ASC_LOG_EXCEPT                           0x5Bu
#define  USBH_SCSI_ASC_RPL_STATUS_CHANGE                    0x5Cu
#define  USBH_SCSI_ASC_FAIL_PREDICTION_TH_EXCEEDED          0x5Du
#define  USBH_SCSI_ASC_LOW_POWER_CONDITION_ON               0x5Eu

#define  USBH_SCSI_ASC_LAMP_FAIL                            0x60u
#define  USBH_SCSI_ASC_VIDEO_ACQUISITION_ERR                0x61u
#define  USBH_SCSI_ASC_SCAN_HEAD_POSITIONING_ERR            0x62u
#define  USBH_SCSI_ASC_END_OF_USER_AREA_ENCOUNTERED         0x63u
#define  USBH_SCSI_ASC_ILLEGAL_MODE_FOR_THIS_TRACK          0x64u
#define  USBH_SCSI_ASC_VOLTAGE_FAULT                        0x65u
#define  USBH_SCSI_ASC_AUTO_DOCUMENT_FEEDER_COVER_UP        0x66u
#define  USBH_SCSI_ASC_CONFIGURATION_FAIL                   0x67u
#define  USBH_SCSI_ASC_LOG_UNIT_NOT_CONFIGURED              0x68u
#define  USBH_SCSI_ASC_DATA_LOSS_ON_LOG_UNIT                0x69u
#define  USBH_SCSI_ASC_INFORMATIONAL_REFER_TO_LOG           0x6Au
#define  USBH_SCSI_ASC_STATE_CHANGE_HAS_OCCURRED            0x6Bu
#define  USBH_SCSI_ASC_REBUILD_FAIL_OCCURRED                0x6Cu
#define  USBH_SCSI_ASC_RECALCULATE_FAIL_OCCURRED            0x6Du
#define  USBH_SCSI_ASC_CMD_TO_LOG_UNIT_FAILED               0x6Eu
#define  USBH_SCSI_ASC_COPY_PROTECTION_KEY_EXCHANGE_FAIL    0x6Fu
#define  USBH_SCSI_ASC_DECOMPRESSION_EXCEPT_LONG_ALGO_ID    0x71u
#define  USBH_SCSI_ASC_SESSION_FIXATION_ERR                 0x72u
#define  USBH_SCSI_ASC_CD_CONTROL_ERR                       0x73u
#define  USBH_SCSI_ASC_SECURITY_ERR                         0x74u

                                                                /* ----------------- SCSI PAGE PARAMETERS ----------------- */
#define USBH_SCSI_PAGE_CODE_READ_WRITE_ERROR_RECOVERY       0x01u
#define USBH_SCSI_PAGE_CODE_FORMAT_DEVICE                   0x03u
#define USBH_SCSI_PAGE_CODE_FLEXIBLE_DISK                   0x05u
#define USBH_SCSI_PAGE_CODE_INFORMATIONAL_EXCEPTIONS        0x1Cu
#define USBH_SCSI_PAGE_CODE_ALL                             0x3Fu

#define USBH_SCSI_PAGE_LENGTH_INFORMATIONAL_EXCEPTIONS      0x0Au
#define USBH_SCSI_PAGE_LENGTH_READ_WRITE_ERROR_RECOVERY     0x0Au
#define USBH_SCSI_PAGE_LENGTH_FLEXIBLE_DISK                 0x1Eu
#define USBH_SCSI_PAGE_LENGTH_FORMAT_DEVICE                 0x16u


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
*                                   COMMAND BLOCK WRAPPER DATA TYPE
*
* Note(s) : (1) See 'USB Mass Storage Class - Bulk Only Transport', Section 5.1.
*
*           (2) The 'bmCBWFlags' field is a bit-mapped datum with three subfields :
*
*               (a) Bit  7  : Data transfer direction :
*
*                   (1) 0 = Data-out from host   to device.
*                   (2) 1 = Data-in  from device to host.
*
*               (b) Bit  6  : Obsolete.  Should be set to zero.
*               (c) Bits 5-0: Reserved.  Should be set to zero.
*********************************************************************************************************
*/

typedef  struct  usbh_msc_cbw {
    CPU_INT32U  dCBWSignature;                                  /* Signature to identify this data pkt as CBW.          */
    CPU_INT32U  dCBWTag;                                        /* Command block tag sent by host.                      */
    CPU_INT32U  dCBWDataTransferLength;                         /* Number of bytes of data that host expects to xfer.   */
    CPU_INT08U  bmCBWFlags;                                     /* Flags (see Notes #2).                                */
    CPU_INT08U  bCBWLUN;                                        /* LUN to which the command block is being sent.        */
    CPU_INT08U  bCBWCBLength;                                   /* Length of CBWCB in bytes.                            */
    CPU_INT08U  CBWCB[16];                                      /* Command block to be executed by device.              */
} USBH_MSC_CBW;


/*
*********************************************************************************************************
*                                  COMMAND STATUS WRAPPER DATA TYPE
*
* Note(s) : (1) See 'USB Mass Storage Class - Bulk Only Transport', Section 5.2.
*********************************************************************************************************
*/

typedef  struct  usbh_msc_csw {
    CPU_INT32U  dCSWSignature;                                  /* Signature to identify this data pkt as CSW.          */
    CPU_INT32U  dCSWTag;                                        /* Device shall set this to value in CBW's dCBWTag.     */
    CPU_INT32U  dCSWDataResidue;                                /* Difference between expected & actual nbr data bytes. */
    CPU_INT08U  bCSWStatus;                                     /* Indicates success or failure of command.             */
} USBH_MSC_CSW;


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

static  USBH_MSC_DEV  USBH_MSC_DevArr[USBH_MSC_CFG_MAX_DEV];
static  MEM_POOL      USBH_MSC_DevPool;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void         USBH_MSC_GlobalInit         (USBH_ERR               *p_err);

static  void        *USBH_MSC_ProbeIF            (USBH_DEV               *p_dev,
                                                  USBH_IF                *p_if,
                                                  USBH_ERR               *p_err);

static  void         USBH_MSC_Disconn            (void                   *p_class_dev);

static  void         USBH_MSC_Suspend            (void                   *p_class_dev);

static  void         USBH_MSC_Resume             (void                   *p_class_dev);

static  void         USBH_MSC_DevClr             (USBH_MSC_DEV           *p_msc_dev);

static  USBH_ERR     USBH_MSC_EP_Open            (USBH_MSC_DEV           *p_msc_dev);

static  void         USBH_MSC_EP_Close           (USBH_MSC_DEV           *p_msc_dev);

static  CPU_INT32U   USBH_MSC_XferCmd            (USBH_MSC_DEV           *p_msc_dev,
                                                  CPU_INT08U              lun,
                                                  USBH_MSC_DATA_DIR       dir,
                                                  void                   *p_cb,
                                                  CPU_INT08U              cb_len,
                                                  void                   *p_arg,
                                                  CPU_INT32U              data_len,
                                                  USBH_ERR               *p_err);

static  USBH_ERR     USBH_MSC_TxCBW              (USBH_MSC_DEV           *p_msc_dev,
                                                  USBH_MSC_CBW           *p_msc_cbw);

static  USBH_ERR     USBH_MSC_RxCSW              (USBH_MSC_DEV           *p_msc_dev,
                                                  USBH_MSC_CSW           *p_msc_csw);

static  USBH_ERR     USBH_MSC_TxData             (USBH_MSC_DEV           *p_msc_dev,
                                                  void                   *p_arg,
                                                  CPU_INT32U              data_len);

static  USBH_ERR     USBH_MSC_RxData             (USBH_MSC_DEV           *p_msc_dev,
                                                  void                   *p_arg,
                                                  CPU_INT32U              data_len);

static  USBH_ERR     USBH_MSC_ResetRecovery      (USBH_MSC_DEV           *p_msc_dev);

static  USBH_ERR     USBH_MSC_BulkOnlyReset      (USBH_MSC_DEV           *p_msc_dev);

static  void         USBH_MSC_FmtCBW             (USBH_MSC_CBW           *p_cbw,
                                                  void                   *p_buf_dest);

static  void         USBH_MSC_ParseCSW           (USBH_MSC_CSW           *p_csw,
                                                  void                   *p_buf_src);

static  USBH_ERR     USBH_SCSI_CMD_TestUnitReady (USBH_MSC_DEV           *p_msc_dev,
                                                  CPU_INT08U              lun);

static  USBH_ERR     USBH_SCSI_CMD_StdInquiry    (USBH_MSC_DEV           *p_msc_dev,
                                                  USBH_MSC_INQUIRY_INFO  *p_msc_inquiry_info,
                                                  CPU_INT08U              lun);

static  CPU_INT32U   USBH_SCSI_CMD_ReqSense      (USBH_MSC_DEV           *p_msc_dev,
                                                  CPU_INT08U              lun,
                                                  CPU_INT08U             *p_arg,
                                                  CPU_INT32U              data_len,
                                                  USBH_ERR               *p_err);

static  USBH_ERR     USBH_SCSI_GetSenseInfo      (USBH_MSC_DEV           *p_msc_dev,
                                                  CPU_INT08U              lun,
                                                  CPU_INT08U             *p_sense_key,
                                                  CPU_INT08U             *p_asc,
                                                  CPU_INT08U             *p_ascq);

static  USBH_ERR     USBH_SCSI_CMD_CapacityRd    (USBH_MSC_DEV           *p_msc_dev,
                                                  CPU_INT08U              lun,
                                                  CPU_INT32U             *p_nbr_blks,
                                                  CPU_INT32U             *p_blk_size);

static  CPU_INT32U   USBH_SCSI_Rd                (USBH_MSC_DEV           *p_msc_dev,
                                                  CPU_INT08U              lun,
                                                  CPU_INT32U              blk_addr,
                                                  CPU_INT16U              nbr_blks,
                                                  CPU_INT32U              blk_size,
                                                  void                   *p_arg,
                                                  USBH_ERR               *p_err);

static  CPU_INT32U   USBH_SCSI_Wr                (USBH_MSC_DEV           *p_msc_dev,
                                                  CPU_INT08U              lun,
                                                  CPU_INT32U              blk_addr,
                                                  CPU_INT16U              nbr_blks,
                                                  CPU_INT32U              blk_size,
                                                  const  void            *p_arg,
                                                  USBH_ERR               *p_err);


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

USBH_CLASS_DRV  USBH_MSC_ClassDrv =  {
    (CPU_INT08U *)"MASS STORAGE",
                  USBH_MSC_GlobalInit,
                  0,
                  USBH_MSC_ProbeIF,
                  USBH_MSC_Suspend,
                  USBH_MSC_Resume,
                  USBH_MSC_Disconn
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
*                                           USBH_MSC_Init()
*
* Description : Initialize a mass storage device instance.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               lun             Device logical unit number to initialize.
*
* Return(s)   : USBH_ERR_NONE,                          if MSC device initialization is successful.
*               USBH_ERR_INVALID_ARG,                   if invalid argument passed to 'p_msc_dev'.
*               USBH_ERR_DEV_NOT_READY,                 if MSC device not ready for communication.
*
*                                                       ----- RETURNED BY USBH_OS_MutexLock() : -----
*               USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,                      If mutex wait aborted.
*               USBH_ERR_OS_FAIL,                       Otherwise.
*
*                                                       ----- RETURNED BY USBH_SCSI_CMD_TestUnitReady -----
*               USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*               USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*               USBH_ERR_MSC_IO                         Unable to receive CSW.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_SCSI_GetSenseInfo -----
*               USBH_ERR_UNKNOWN,                       if unknown error occured.
*               USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*               USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*               USBH_ERR_MSC_IO                         Unable to receive CSW.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) This block of code will send TEST_UNIT_READY SCSI command to know if MSC device is
*                   ready for communication. Host will try up to 20 times in case of MSC device not ready.
*                   If MSC device is not ready, host sends REQUEST_SENSE SCSI command to know
*                   exactly why device is not ready.
*
*               (2) SENSE KEY, ADDITIONAL SENSE CODE (ASC) and ADDITIONAL SENSE CODE QUALIFIER (ASCQ)
*                   fields provide a hierarchy of information.
*                   See "SCSI Primary Commands - 4 (SPC-4)", section "4.5.2.1 Descriptor format sense
*                   data overview" for more details about REQUEST_SENSE response. Document is
*                   available at http://www.t10.org/drafts.htm#SPC_Family.
*********************************************************************************************************
*/

USBH_ERR  USBH_MSC_Init (USBH_MSC_DEV  *p_msc_dev,
                         CPU_INT08U     lun)
{
    CPU_INT08U   retry;
    CPU_INT08U   sense_key;
    CPU_INT08U   asc;
    CPU_INT08U   ascq;
    CPU_BOOLEAN  unit_ready;
    USBH_ERR     err;

                                                                /* ------------------- VALIDATE ARG ------------------- */
    if (p_msc_dev == (USBH_MSC_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_OS_MutexLock(p_msc_dev->HMutex);                 /* Acquire MSC dev lock to avoid multiple access.       */
    if (err != USBH_ERR_NONE) {
        return (err);
    }
                                                                /* See Note #1.                                         */
    if ((p_msc_dev->State  == USBH_CLASS_DEV_STATE_CONN) &&
        (p_msc_dev->RefCnt  > 0u                       )) {

#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("Mass Storage device (LUN %d) is initializing ...\r\n", lun);
#endif
        retry      = 40u;                                       /* Host attempts 20 times to know if MSC dev ready.     */
        unit_ready = DEF_FALSE;

        while (retry > 0u) {
                                                                /* --------------- TEST_UNIT_READY CMD ---------------- */
            err = USBH_SCSI_CMD_TestUnitReady(p_msc_dev, lun);
            if (err == USBH_ERR_NONE) {                         /* MSC dev ready for comm.                              */
                unit_ready = DEF_TRUE;
                break;
            } else if (err == USBH_ERR_MSC_IO) {                /* Bulk xfers for the BOT protocol failed.              */
                USBH_PRINT_ERR(err);
                break;
            } else {
                if (err != USBH_ERR_MSC_CMD_FAILED) {           /* MSC dev not ready ...                                */
                    USBH_PRINT_ERR(err);
                }

                err = USBH_SCSI_GetSenseInfo(p_msc_dev,         /* ...determine reason of failure.                      */
                                             lun,
                                            &sense_key,
                                            &asc,
                                            &ascq);
                if (err == USBH_ERR_NONE) {
                    switch (sense_key) {                        /* See Note #2.                                         */
                        case USBH_SCSI_SENSE_KEY_UNIT_ATTENTION:/* MSC dev is initializing internally but not rdy.      */
                             switch (asc) {
                                 case USBH_SCSI_ASC_MEDIUM_NOT_PRESENT:
                                      USBH_OS_DlyMS(500u);
                                      break;
                                                                /* MSC dev changed its internal state to ready.         */
                                 case USBH_SCSI_ASC_NOT_RDY_TO_RDY_CHANGE:
                                      USBH_OS_DlyMS(500u);
                                      break;

                                 default:                       /* Other Additional Sense Code values not supported.    */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                                      USBH_PRINT_LOG("SENSE KEY1 : %02X, ASC : %02X, ASCQ : %02X\r\n", sense_key, asc, ascq);
#endif
                                      break;
                             }
                             break;

                        case USBH_SCSI_SENSE_KEY_NOT_RDY:       /* MSC dev not ready.                                   */
                             USBH_OS_DlyMS(500u);
                             break;

                        default:                                /* Other Sense Key values not supported.                */
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
                             USBH_PRINT_LOG("SENSE KEY2 : %02X, ASC : %02X, ASCQ : %02X\r\n", sense_key, asc, ascq);
#endif
                             break;
                    }

                    retry--;
                } else {
                    USBH_PRINT_ERR(err);
                    break;
                }
            }
        }

        if (unit_ready == DEF_FALSE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
            USBH_PRINT_LOG("Device is not ready\r\n");
#endif
            err = USBH_ERR_DEV_NOT_READY;
        }

    } else {
        err = USBH_ERR_DEV_NOT_READY;                           /* MSC dev enum not completed by host.                  */
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);               /* Unlock access to MSC dev.                            */

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_MSC_MaxLUN_Get()
*
* Description : Get maximum logical unit number (LUN) supported by MSC device.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE                           Maximum LUN successfully retrieved.
*                       USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_msc_dev'.
*                       USBH_ERR_DEV_NOT_READY                  If device enumeration not completed.
*
*                                                               ----- RETURNED BY USBH_CtrlRx() : -----
*                       USBH_ERR_UNKNOWN,                       Unknown error occurred.
*                       USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*                       USBH_ERR_HC_IO,                         Root hub input/output error.
*                       USBH_ERR_EP_STALL,                      Root hub does not support request.
*                       Host controller drivers error code,     Otherwise.
*
*                                                               ----- RETURNED BY USBH_OS_MutexLock() : -----
*                       USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'mutex'.
*                       USBH_ERR_OS_ABORT,                      If mutex wait aborted.
*                       USBH_ERR_OS_FAIL,                       Otherwise.
*
* Return(s)   : Maximum number of logical unit.
*
* Note(s)     : (1) The "GET MAX LUN" is an MSC specific command and is documented in "USB MSC Bulk Only
*                   Transport specification", Section 3.2.
*********************************************************************************************************
*/

CPU_INT08U  USBH_MSC_MaxLUN_Get (USBH_MSC_DEV  *p_msc_dev,
                                 USBH_ERR      *p_err)
{
    CPU_INT08U  if_nbr;
    CPU_INT08U  lun_nbr = 0u;

                                                                /* ------------------ VALIDATE ARG -------------------- */
    if (p_msc_dev == (USBH_MSC_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

   *p_err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }
                                                                /* --------------- GET_MAX_LUN REQUEST ---------------- */
    if ((p_msc_dev->State == USBH_CLASS_DEV_STATE_CONN) &&
        (p_msc_dev->RefCnt > 0u                       )) {

        if_nbr = USBH_IF_NbrGet(p_msc_dev->IF_Ptr);             /* Get IF nbr matching to MSC dev.                      */

        USBH_CtrlRx(         p_msc_dev->DevPtr,                 /* Send GET_MAX_LUN request via a Ctrl xfer.            */
                             USBH_MSC_REQ_GET_MAX_LUN,          /* See Note #1.                                         */
                            (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                             0u,
                             if_nbr,
                    (void *)&lun_nbr,
                             1u,
                             USBH_MSC_TIMEOUT,
                             p_err);
        if (*p_err != USBH_ERR_NONE) {
            (void)USBH_EP_Reset(           p_msc_dev->DevPtr,   /* Reset dflt EP if ctrl xfer failed.                   */
                                (USBH_EP *)0);

            if (*p_err == USBH_ERR_EP_STALL) {                  /* Device may stall if no multiple LUNs support.        */
                lun_nbr = 0u;
               *p_err   = USBH_ERR_NONE;
            }
        }
    } else {                                                    /* MSC dev enumeration not completed by host.           */
       *p_err = USBH_ERR_DEV_NOT_READY;
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);

    return (lun_nbr);
}

/*
*********************************************************************************************************
*                                        USBH_MSC_UnitRdyTest()
*
* Description : Test if a certain logical unit within the MSC device is ready for communication.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               lun             Logical unit number.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE                           Maximum LUN successfully retrieved.
*                       USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_msc_dev'.
*                       USBH_ERR_DEV_NOT_READY                  If device enumeration not completed.
*
*                                                               ----- RETURNED BY USBH_OS_MutexLock() : -----
*                       USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'mutex'.
*                       USBH_ERR_OS_ABORT,                      If mutex wait aborted.
*                       USBH_ERR_OS_FAIL,                       Otherwise.
*
*                                                               ---- RETURNED BY USBH_SCSI_CMD_TestUnitReady() ----
*                       USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*                       USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*                       USBH_ERR_MSC_IO                         Unable to receive CSW.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*                       USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : DEF_YES, if logical unit ready.
*
*               DEF_NO,  otherwise.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  USBH_MSC_UnitRdyTest (USBH_MSC_DEV  *p_msc_dev,
                                   CPU_INT08U     lun,
                                   USBH_ERR      *p_err)
{
    CPU_BOOLEAN  unit_rdy = DEF_NO;

                                                                /* ------------------- VALIDATE PTR ------------------- */
    if (p_msc_dev == (USBH_MSC_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (unit_rdy);
    }

   *p_err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (*p_err != USBH_ERR_NONE) {
        return (unit_rdy);
    }
                                                                /* --------------- TEST_UNIT_READY REQ ---------------- */
    if ((p_msc_dev->State == USBH_CLASS_DEV_STATE_CONN) &&
        (p_msc_dev->RefCnt > 0u                       )) {

       *p_err = USBH_SCSI_CMD_TestUnitReady(p_msc_dev, 0u);
        if (*p_err == USBH_ERR_NONE) {
            unit_rdy = DEF_YES;
        } else if (*p_err == USBH_ERR_MSC_CMD_FAILED) {
           *p_err    = USBH_ERR_NONE;                           /* CSW reporting cmd failed for this req NOT an err.    */
            unit_rdy = DEF_NO;
        } else {
                                                                /* Empty Else Statement                                 */
        }
    } else {                                                    /* MSC dev enumeration not completed by host.           */
       *p_err = USBH_ERR_DEV_NOT_READY;
    }

    USBH_OS_MutexUnlock(p_msc_dev->HMutex);

    return (unit_rdy);
}

/*
*********************************************************************************************************
*                                        USBH_MSC_CapacityRd()
*
* Description : Read mass storage device capacity (i.e number of blocks and block size) of specified LUN
*               by sending READ_CAPACITY SCSI command.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               lun             Logical unit number.
*
*               p_nbr_blks      Pointer to variable that receives number of blocks.
*
*               p_blk_size      Pointer to variable that receives block size.
*
* Return(s)   : USBH_ERR_NONE,              If capacity of MSC device retrieved successfully.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_msc_dev' / 'p_nbr_blks' /
*                                           'p_blk_size'.
*               USBH_ERR_DEV_NOT_READY,     If device enumeration not completed.
*
*                                           ----- RETURNED BY USBH_OS_MutexLock() : -----
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,          If mutex wait aborted.
*               USBH_ERR_OS_FAIL,           Otherwise.
*               List error codes from USBH_SCSI_CMD_CapacityRd
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_MSC_CapacityRd (USBH_MSC_DEV  *p_msc_dev,
                               CPU_INT08U     lun,
                               CPU_INT32U    *p_nbr_blks,
                               CPU_INT32U    *p_blk_size)
{
    USBH_ERR  err;

                                                                /* ------------------- VALIDATE PTR ------------------- */
    if ((p_msc_dev  == (USBH_MSC_DEV *)0) ||
        (p_nbr_blks == (CPU_INT32U   *)0) ||
        (p_blk_size == (CPU_INT32U   *)0)) {
        err = USBH_ERR_INVALID_ARG;
        return (err);
    }

    err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

                                                                /* -------------- READ_CAPACITY REQUEST --------------- */
    if ((p_msc_dev->State == USBH_CLASS_DEV_STATE_CONN) &&
        (p_msc_dev->RefCnt > 0u                       )) {

        err = USBH_SCSI_CMD_CapacityRd(p_msc_dev,               /* Issue READ_CAPACITY SCSI cmd using bulk xfers.       */
                                       lun,
                                       p_nbr_blks,
                                       p_blk_size);
    } else {                                                    /* MSC dev enumeration not completed by host.           */
        err = USBH_ERR_DEV_NOT_READY;
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);               /* Unlock access to MSC dev.                            */

    return (err);
}


/*
*********************************************************************************************************
*                                        USBH_MSC_StdInquiry()
*
* Description : Retrieve various information about a specific logical unit inside mass storage device
*               such as device type, if device is removable, vendor/product identification, etc.
*               INQUIRY SCSI command is used.
*
* Argument(s) : p_msc_dev               Pointer to MSC device.
*
*               p_msc_inquiry_info      Pointer to structure that will receive the informations.
*
*               lun                     Logical unit number.
*
* Return(s)   : USBH_ERR_NONE,              If information about logical unit retrieved successfully.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_msc_dev' / 'p_msc_inquiry_info'.
*               USBH_ERR_NOT_SUPPORTED,     If INQUIRY command has failed.
*               USBH_ERR_DEV_NOT_READY,     If device enumeration not completed.
*
*                                           ----- RETURNED BY USBH_OS_MutexLock() : -----
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,          If mutex wait aborted.
*               USBH_ERR_OS_FAIL,           Otherwise.

* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_MSC_StdInquiry (USBH_MSC_DEV           *p_msc_dev,
                               USBH_MSC_INQUIRY_INFO  *p_msc_inquiry_info,
                               CPU_INT08U              lun)
{
    USBH_ERR  err;

                                                                /* ------------------- VALIDATE PTR ------------------- */
    if ((p_msc_dev          == (USBH_MSC_DEV          *)0) ||
        (p_msc_inquiry_info == (USBH_MSC_INQUIRY_INFO *)0)) {
        err = USBH_ERR_INVALID_ARG;
        return (err);
    }

    err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

                                                                /* ----------------- INQUIRY REQUEST ------------------ */
    if ((p_msc_dev->State == USBH_CLASS_DEV_STATE_CONN) &&
        (p_msc_dev->RefCnt > 0u                       )) {

        err = USBH_SCSI_CMD_StdInquiry(p_msc_dev,               /* Issue INQUIRY SCSI command using Bulk xfers.         */
                                       p_msc_inquiry_info,
                                       lun);
        if (err == USBH_ERR_NONE) {
            err = USBH_ERR_NONE;
        } else {
            err = USBH_ERR_NOT_SUPPORTED;
        }
    } else {
        err = USBH_ERR_DEV_NOT_READY;
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_RefAdd()
*
* Description : Increment counter of connected mass storage devices.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : USBH_ERR_NONE,              If counter successfully incremented.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_msc_dev'.
*
*                                           ----- RETURNED BY USBH_OS_MutexLock() : -----
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,          If mutex wait aborted.
*               USBH_ERR_OS_FAIL,           Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_MSC_RefAdd (USBH_MSC_DEV  *p_msc_dev)
{
    USBH_ERR  err;

                                                                /* ------------------- VALIDATE ARG ------------------- */
    if (p_msc_dev == (USBH_MSC_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    p_msc_dev->RefCnt++;

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_RefRel()
*
* Description : Decrement counter of connected mass storage devices and free device if counter = 0.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : USBH_ERR_NONE,              If counter successfully decremented.
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'p_msc_dev'.
*
*                                           ----- RETURNED BY USBH_OS_MutexLock() : -----
*               USBH_ERR_INVALID_ARG,       If invalid argument passed to 'mutex'.
*               USBH_ERR_OS_ABORT,          If mutex wait aborted.
*               USBH_ERR_OS_FAIL,           Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

USBH_ERR  USBH_MSC_RefRel (USBH_MSC_DEV  *p_msc_dev)
{
    USBH_ERR  err;
    LIB_ERR   err_lib;

                                                                /* ------------------- VALIDATE PTR ------------------- */
    if (p_msc_dev == (USBH_MSC_DEV *)0) {
        return (USBH_ERR_INVALID_ARG);
    }

    err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    if (p_msc_dev->RefCnt > 0u) {
        p_msc_dev->RefCnt--;

        if ((p_msc_dev->RefCnt ==                           0u) &&
            (p_msc_dev->State  == USBH_CLASS_DEV_STATE_DISCONN)) {
                                                                /* Release MSC dev if no more ref on it.                */
            Mem_PoolBlkFree(       &USBH_MSC_DevPool,
                            (void *)p_msc_dev,
                                   &err_lib);
        }
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);

    return (err);
}


/*
*********************************************************************************************************
*                                             USBH_MSC_Rd()
*
* Description : Read specified number of blocks from device using READ_10 SCSI command.
*
* Argument(s) : p_msc_dev        Pointer to MSC device.
*
*               lun              Logical unit number.
*
*               blk_addr         Block address.
*
*               nbr_blks         Number of blocks to read.
*
*               blk_size         Block size.
*
*               p_arg            Pointer to data buffer.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE                           Block(s) read successfully.
*                           USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_msc_dev'.
*                           USBH_ERR_DEV_NOT_READY                  If device enumeration not completed.
*
*                                                                   ----- RETURNED BY USBH_OS_MutexLock() : -----
*                           USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'mutex'.
*                           USBH_ERR_OS_ABORT,                      If mutex wait aborted.
*                           USBH_ERR_OS_FAIL,                       Otherwise.
*
*                                                                   ----- RETURNED BY USBH_SCSI_Rd -----
*                           USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*                           USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*                           USBH_ERR_MSC_IO                         Unable to receive CSW.
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets read.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_MSC_Rd (USBH_MSC_DEV  *p_msc_dev,
                         CPU_INT08U     lun,
                         CPU_INT32U     blk_addr,
                         CPU_INT16U     nbr_blks,
                         CPU_INT32U     blk_size,
                         void          *p_arg,
                         USBH_ERR      *p_err)

{
    CPU_INT32U  xfer_len;


    if (p_msc_dev == (USBH_MSC_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

   *p_err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    if ((p_msc_dev->State == USBH_CLASS_DEV_STATE_CONN) &&
        (p_msc_dev->RefCnt > 0u                       )) {
        xfer_len = USBH_SCSI_Rd(p_msc_dev,
                                lun,
                                blk_addr,
                                nbr_blks,
                                blk_size,
                                p_arg,
                                p_err);
    } else {
        xfer_len = 0u;
       *p_err    = USBH_ERR_DEV_NOT_READY;
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                             USBH_MSC_Wr()
*
* Description : Write specified number of blocks to the device.
*
* Argument(s) : p_msc_dev        Pointer to mass storage device.
*
*               lun              Logical unit number.
*
*               blk_addr         Block address.
*
*               nbr_blks         Number of blocks to write.
*
*               blk_size         Block size.
*
*               p_arg            Pointer to data buffer.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE                           Block(s) write successfully.
*                       USBH_ERR_INVALID_ARG                    If invalid argument passed to 'p_msc_dev'.
*                       USBH_ERR_DEV_NOT_READY                  If device enumeration not completed.
*
*                                                               ----- RETURNED BY USBH_OS_MutexLock() : -----
*                       USBH_ERR_INVALID_ARG,                   If invalid argument passed to 'mutex'.
*                       USBH_ERR_OS_ABORT,                      If mutex wait aborted.
*                       USBH_ERR_OS_FAIL,                       Otherwise.
*
*                                                               ----- RETURNED BY USBH_SCSI_Rd -----
*                       USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*                       USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*                       USBH_ERR_MSC_IO                         Unable to receive CSW.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*                       USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets written.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  USBH_MSC_Wr (       USBH_MSC_DEV  *p_msc_dev,
                                CPU_INT08U     lun,
                                CPU_INT32U     blk_addr,
                                CPU_INT16U     nbr_blks,
                                CPU_INT32U     blk_size,
                         const  void          *p_arg,
                                USBH_ERR      *p_err)

{
    CPU_INT32U  xfer_len;


    if (p_msc_dev == (USBH_MSC_DEV *)0) {
       *p_err = USBH_ERR_INVALID_ARG;
        return (0u);
    }

   *p_err = USBH_OS_MutexLock(p_msc_dev->HMutex);
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    if ((p_msc_dev->State == USBH_CLASS_DEV_STATE_CONN     ) &&
        (p_msc_dev->RefCnt > 0                             )) {
        xfer_len = USBH_SCSI_Wr(p_msc_dev,
                                lun,
                                blk_addr,
                                nbr_blks,
                                blk_size,
                                p_arg,
                                p_err);
    } else {
        xfer_len = 0u;
       *p_err    = USBH_ERR_DEV_NOT_READY;
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);

    return (xfer_len);
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
*                                        USBH_MSC_GlobalInit()
*
* Description : Initialize MSC.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           USBH_ERR_NONE       MSC successfully initalized.
*                           USBH_ERR_ALLOC      If MSC device cannot be allocated.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_GlobalInit (USBH_ERR  *p_err)
{

    CPU_INT08U  ix;
    CPU_SIZE_T  octets_reqd;
    LIB_ERR     err_lib;

                                                                /* --------------- INIT MSC DEV STRUCT ---------------- */
    for (ix = 0u; ix < USBH_MSC_CFG_MAX_DEV; ix++) {
        USBH_MSC_DevClr(&USBH_MSC_DevArr[ix]);
        USBH_OS_MutexCreate(&USBH_MSC_DevArr[ix].HMutex);
    }

    Mem_PoolCreate (       &USBH_MSC_DevPool,                   /* POOL for managing MSC dev struct.                    */
                    (void *)USBH_MSC_DevArr,
                           (sizeof(USBH_MSC_DEV) * USBH_MSC_CFG_MAX_DEV),
                            USBH_MSC_CFG_MAX_DEV,
                            sizeof(USBH_MSC_DEV),
                            sizeof(CPU_ALIGN),
                           &octets_reqd,
                           &err_lib);
    if (err_lib != LIB_MEM_ERR_NONE) {
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
        USBH_PRINT_LOG("%d octets required\r\n", octets_reqd);
#endif
       *p_err = USBH_ERR_ALLOC;
    } else {
       *p_err = USBH_ERR_NONE;
    }
}


/*
*********************************************************************************************************
*                                         USBH_MSC_ProbeIF()
*
* Description : Determine if interface is mass storage class interface.
*
* Argument(s) : p_dev      Pointer to USB device.
*
*               p_if       Pointer to interface.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                       USBH_ERR_NONE                       MSC successfully initalized.
*                       USBH_ERR_DEV_ALLOC                  MSC device cannot be allocated.
*                       USBH_ERR_CLASS_DRV_NOT_FOUND        IF type is not MSC.
*
*                                                           ----- RETURNED BY USBH_IF_DescGet() : -----
*                       USBH_ERR_INVALID_ARG,               Invalid argument passed to 'alt_ix'.
*
*                                                           ----- RETURNED BY USBH_MSC_EP_Open -----
*                       USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*                       USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*                       USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*                       Host controller drivers error,      Otherwise.
*
* Return(s)   : p_msc_dev,      if device has a mass storage class interface.
*               0,              otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  *USBH_MSC_ProbeIF (USBH_DEV  *p_dev,
                                 USBH_IF   *p_if,
                                 USBH_ERR  *p_err)
{
    USBH_IF_DESC   p_if_desc;
    USBH_MSC_DEV  *p_msc_dev;
    LIB_ERR        err_lib;


    p_msc_dev = (USBH_MSC_DEV *)0;
   *p_err     =  USBH_IF_DescGet(p_if, 0u, &p_if_desc);
    if (*p_err != USBH_ERR_NONE) {
        return ((void *)0);
    }
                                                                /* Chk for class, sub class and protocol.               */
    if ((p_if_desc.bInterfaceClass     == USBH_CLASS_CODE_MASS_STORAGE)      &&
        ((p_if_desc.bInterfaceSubClass == USBH_MSC_SUBCLASS_CODE_SCSI)       ||
         (p_if_desc.bInterfaceSubClass == USBH_MSC_SUBCLASS_CODE_SFF_8070i)) &&
        (p_if_desc.bInterfaceProtocol  == USBH_MSC_PROTOCOL_CODE_BULK_ONLY)) {

                                                                /* Alloc dev from MSC dev pool.                         */
        p_msc_dev = (USBH_MSC_DEV *)Mem_PoolBlkGet(&USBH_MSC_DevPool,
                                                    sizeof(USBH_MSC_DEV),
                                                   &err_lib);
        if (err_lib != LIB_MEM_ERR_NONE) {
           *p_err  = USBH_ERR_DEV_ALLOC;
            return ((void *)0);
        }

        USBH_MSC_DevClr(p_msc_dev);
        p_msc_dev->RefCnt = (CPU_INT08U  )0;
        p_msc_dev->State  = USBH_CLASS_DEV_STATE_CONN;
        p_msc_dev->DevPtr = p_dev;
        p_msc_dev->IF_Ptr = p_if;

        *p_err = USBH_MSC_EP_Open(p_msc_dev);                   /* Open Bulk in/out EPs.                                */
        if (*p_err != USBH_ERR_NONE) {
            Mem_PoolBlkFree(       &USBH_MSC_DevPool,
                            (void *)p_msc_dev,
                                   &err_lib);
        }
    } else {
       *p_err = USBH_ERR_CLASS_DRV_NOT_FOUND;
    }

    if (*p_err != USBH_ERR_NONE) {
        p_msc_dev = (void *)0;
    }

    return ((void *)p_msc_dev);
}


/*
*********************************************************************************************************
*                                         USBH_MSC_Disconn()
*
* Description : Handle disconnection of mass storage device.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_Disconn (void  *p_class_dev)
{
    LIB_ERR        err_lib;
    USBH_MSC_DEV  *p_msc_dev;


    p_msc_dev = (USBH_MSC_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_msc_dev->HMutex);

    p_msc_dev->State = USBH_CLASS_DEV_STATE_DISCONN;
    USBH_MSC_EP_Close(p_msc_dev);                               /* Close bulk in/out EPs.                               */

    if (p_msc_dev->RefCnt == 0u) {                              /* Release MSC dev.                                     */
        (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);
        Mem_PoolBlkFree(       &USBH_MSC_DevPool,
                        (void *)p_msc_dev,
                               &err_lib);
    }

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);
}


/*
*********************************************************************************************************
*                                         USBH_MSC_Suspend()
*
* Description : Suspend MSC device. Waits for completion of any pending I/O.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_Suspend (void  *p_class_dev)
{
    USBH_MSC_DEV  *p_msc_dev;


    p_msc_dev = (USBH_MSC_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_msc_dev->HMutex);

    p_msc_dev->State = USBH_CLASS_DEV_STATE_SUSPEND;

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_Resume()
*
* Description : Resume MSC device.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_Resume (void  *p_class_dev)
{
    USBH_MSC_DEV  *p_msc_dev;


    p_msc_dev = (USBH_MSC_DEV *)p_class_dev;

    (void)USBH_OS_MutexLock(p_msc_dev->HMutex);

    p_msc_dev->State = USBH_CLASS_DEV_STATE_CONN;

    (void)USBH_OS_MutexUnlock(p_msc_dev->HMutex);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_DevClr()
*
* Description : Clear USBH_MSC_DEV structure.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_DevClr (USBH_MSC_DEV  *p_msc_dev)
{
    p_msc_dev->DevPtr = (USBH_DEV *)0;
    p_msc_dev->IF_Ptr = (USBH_IF  *)0;
    p_msc_dev->State  =  USBH_CLASS_DEV_STATE_NONE;
    p_msc_dev->RefCnt =  0u;
}


/*
*********************************************************************************************************
*                                         USBH_MSC_EP_Open()
*
* Description : Open bulk IN & OUT endpoints.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : USBH_ERR_NONE,                      if endpoints successfully opened.
*
*                                                   ---- RETURNED BY USBH_BulkInOpen ----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
*                                                   ---- RETURNED BY USBH_BulkOutOpen ----
*               USBH_ERR_EP_ALLOC,                  If USBH_CFG_MAX_NBR_EPS reached.
*               USBH_ERR_EP_NOT_FOUND,              If endpoint with given type and direction not found.
*               USBH_ERR_OS_SIGNAL_CREATE,          if mutex or semaphore creation failed.
*               Host controller drivers error,      Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_MSC_EP_Open (USBH_MSC_DEV  *p_msc_dev)
{
    USBH_ERR  err;


    err = USBH_BulkInOpen(p_msc_dev->DevPtr,
                          p_msc_dev->IF_Ptr,
                         &p_msc_dev->BulkInEP);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_BulkOutOpen(p_msc_dev->DevPtr,
                           p_msc_dev->IF_Ptr,
                          &p_msc_dev->BulkOutEP);
    if (err != USBH_ERR_NONE) {
        USBH_MSC_EP_Close(p_msc_dev);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                         USBH_MSC_EP_Close()
*
* Description : Close bulk IN & OUT endpoints.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_EP_Close (USBH_MSC_DEV  *p_msc_dev)
{
    USBH_EP_Close(&p_msc_dev->BulkInEP);
    USBH_EP_Close(&p_msc_dev->BulkOutEP);
}


/*
*********************************************************************************************************
*                                         USBH_MSC_XferCmd()
*
* Description : Executes MSC command cycle. Sends command (CBW), followed by data stage (if present),
*               and then receive status (CSW).
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               lun             Logical Unit Number.
*
*               dir             Direction of data transfer, if present.
*
*               p_cb            Pointer to command block.
*
*               cb_len          Command block length, in octets.
*
*               p_arg           Pointer to data buffer, if data stage present.
*
*               data_len        Length of data buffer in octets, if data stage present.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*                           USBH_ERR_NONE                           Command successfully transmitted.
*                           USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*                           USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*                           USBH_ERR_MSC_IO                         Unable to receive CSW.
*
*                                                                   ----- RETURNED BY USBH_MSC_RxCSW() : -----
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_MSC_RxData() : -----
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_MSC_TxCBW() : -----
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not OUT.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
*                                                                   ----- RETURNED BY USBH_MSC_TxData() : -----
*                           USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                           USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not OUT.
*                           USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                           Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets transferred.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_MSC_XferCmd (USBH_MSC_DEV       *p_msc_dev,
                                      CPU_INT08U          lun,
                                      USBH_MSC_DATA_DIR   dir,
                                      void               *p_cb,
                                      CPU_INT08U          cb_len,
                                      void               *p_arg,
                                      CPU_INT32U          data_len,
                                      USBH_ERR           *p_err)
{
    CPU_INT32U     xfer_len;
    USBH_MSC_CBW   msc_cbw;
    USBH_MSC_CSW   msc_csw;

                                                                /* Prepare CBW.                                         */
    msc_cbw.dCBWSignature          =  USBH_MSC_SIG_CBW;
    msc_cbw.dCBWTag                =  0u;
    msc_cbw.dCBWDataTransferLength =  data_len;
    msc_cbw.bmCBWFlags             = (dir == USBH_MSC_DATA_DIR_NONE) ? 0u : dir;
    msc_cbw.bCBWLUN                =  lun;
    msc_cbw.bCBWCBLength           =  cb_len;

    Mem_Copy((void *)msc_cbw.CBWCB,
                     p_cb,
                     cb_len);

   *p_err = USBH_MSC_TxCBW(p_msc_dev, &msc_cbw);                /* Send CBW to dev.                                     */
    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    switch (dir) {
        case USBH_MSC_DATA_DIR_OUT:
            *p_err = USBH_MSC_TxData(p_msc_dev,
                                     p_arg,
                                     data_len);                 /* Send data to dev.                                    */
             break;

        case USBH_MSC_DATA_DIR_IN:
            *p_err = USBH_MSC_RxData(p_msc_dev,
                                     p_arg,
                                     data_len);                 /* Receive data from dev.                               */
             break;

        default:
            *p_err = USBH_ERR_NONE;
             break;
    }

    if (*p_err != USBH_ERR_NONE) {
        return (0u);
    }

    Mem_Set((void *)&msc_csw,
                     0u,
                     USBH_MSC_LEN_CSW);

    *p_err = USBH_MSC_RxCSW(p_msc_dev, &msc_csw);               /* Receive CSW.                                         */

    if ((msc_csw.dCSWSignature != USBH_MSC_SIG_CSW               ) ||
        (msc_csw.bCSWStatus    == USBH_MSC_BCSWSTATUS_PHASE_ERROR) ||
        (msc_csw.dCSWTag       != msc_cbw.dCBWTag                )) {
        USBH_MSC_ResetRecovery(p_msc_dev);                      /* Invalid CSW, issue reset recovery.                   */
    }

    if (*p_err == USBH_ERR_NONE) {
        switch (msc_csw.bCSWStatus) {
            case USBH_MSC_BCSWSTATUS_CMD_PASSED:
                *p_err = USBH_ERR_NONE;
                 break;

            case USBH_MSC_BCSWSTATUS_CMD_FAILED:
                *p_err = USBH_ERR_MSC_CMD_FAILED;
                 break;

            case USBH_MSC_BCSWSTATUS_PHASE_ERROR:
                *p_err = USBH_ERR_MSC_CMD_PHASE;
                 break;

            default:
                *p_err = USBH_ERR_MSC_CMD_FAILED;
                 break;
        }
    } else {
       *p_err = USBH_ERR_MSC_IO;
    }

                                                                /* Actual len of data xfered to dev.                    */
    xfer_len = msc_cbw.dCBWDataTransferLength - msc_csw.dCSWDataResidue;

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_TxCBW()
*
* Description : Send Command Block Wrapper (CBW) to device through bulk OUT endpoint.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               p_msc_cbw       Pointer to Command Block Wrapper (CBW).
*
* Return(s)   : USBH_ERR_NONE                           If CBW successfully transmitted.
*
*                                                       ----- RETURNED BY USBH_BulkTx() : -----
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not OUT.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_MSC_TxCBW (USBH_MSC_DEV  *p_msc_dev,
                                  USBH_MSC_CBW  *p_msc_cbw)
{
    CPU_INT08U  cmd_buf[USBH_MSC_LEN_CBW];
    USBH_ERR    err;
    CPU_INT32U  len;


    Mem_Clr((void *)cmd_buf,
                    USBH_MSC_LEN_CBW);

    USBH_MSC_FmtCBW(p_msc_cbw, cmd_buf);                        /* Format CBW.                                          */

    len = USBH_BulkTx(        &p_msc_dev->BulkOutEP,            /* Send CBW through bulk OUT EP.                        */
                      (void *)&cmd_buf[0],
                               USBH_MSC_LEN_CBW,
                               USBH_MSC_TIMEOUT,
                              &err);
    if (len != USBH_MSC_LEN_CBW) {
        if (err != USBH_ERR_NONE) {
            USBH_PRINT_ERR(err);
            USBH_EP_Reset(p_msc_dev->DevPtr,
                         &p_msc_dev->BulkOutEP);                /* Clear EP err on host side.                           */
        }

        if (err == USBH_ERR_EP_STALL) {
            USBH_MSC_ResetRecovery(p_msc_dev);
        }
    } else {
        err = USBH_ERR_NONE;
    }

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_RxCSW()
*
* Description : Receive Command Status Word (CSW) from device through bulk IN endpoint.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               p_msc_csw       Pointer to Command Status Word (CSW).
*
* Return(s)   : USBH_ERR_NONE,                          If CSW successfully received.
*
*                                                       ----- RETURNED BY USBH_BulkRx() : -----
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_MSC_RxCSW (USBH_MSC_DEV  *p_msc_dev,
                                  USBH_MSC_CSW  *p_msc_csw)
{
    CPU_INT32U  retry;
    CPU_INT08U  status_buf[USBH_MSC_LEN_CSW];
    USBH_ERR    err;
    CPU_INT32U  len;


    err   = USBH_ERR_NONE;
    retry = 2u;

                                                                /* Receive CSW from dev through bulk IN EP.             */
    while (retry > 0u) {
        len = USBH_BulkRx(        &p_msc_dev->BulkInEP,
                          (void *)&status_buf[0],
                                   USBH_MSC_LEN_CSW,
                                   USBH_MSC_TIMEOUT,
                                  &err);
        if (len != USBH_MSC_LEN_CSW) {
            USBH_EP_Reset(p_msc_dev->DevPtr,
                         &p_msc_dev->BulkInEP);                 /* Clear err on host side.                              */

            if (err == USBH_ERR_EP_STALL) {
                (void)USBH_EP_StallClr(&p_msc_dev->BulkInEP);
                retry--;
            } else {
                break;
            }
        } else {
            err = USBH_ERR_NONE;
            USBH_MSC_ParseCSW(p_msc_csw, status_buf);
            break;
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_TxData()
*
* Description : Send data to device through bulk OUT endpoint.
*
* Argument(s) : p_msc_dev        Pointer to MSC device.
*
*               p_arg            Pointer to data buffer.
*
*               data_len         Length of data buffer in octets.
*
* Return(s)   : USBH_ERR_NONE,                          if data successfully transmitted.
*
*                                                       ----- RETURNED BY USBH_BulkTx() : -----
*               USBH_ERR_NONE                           Bulk transfer successfully transmitted.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not OUT.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_NONE,                          URB is successfully submitted to host controller.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_MSC_TxData (USBH_MSC_DEV  *p_msc_dev,
                                   void          *p_arg,
                                   CPU_INT32U     data_len)
{
    USBH_ERR      err;
    CPU_BOOLEAN   retry;
    CPU_INT08U   *p_data_08;
    CPU_INT16U    retry_cnt;
    CPU_INT32U    p_data_ix;
    CPU_INT32U    data_len_rem;
    CPU_INT32U    data_len_tx;


    err          =  USBH_ERR_NONE;
    p_data_ix    =  0u;
    p_data_08    = (CPU_INT08U *)p_arg;
    data_len_rem =  data_len;
    retry_cnt    =  0u;
    retry        =  DEF_TRUE;

    while ((data_len_rem  > 0u      ) &&
           (retry        == DEF_TRUE)) {

        data_len_tx = USBH_BulkTx(        &p_msc_dev->BulkOutEP,
                                  (void *)(p_data_08 + p_data_ix),
                                           data_len_rem,
                                           USBH_MSC_TIMEOUT,
                                          &err);
        switch (err) {
            case USBH_ERR_DEV_NOT_RESPONDING:
            case USBH_ERR_HC_IO:
                 retry_cnt++;
                 if (retry_cnt >= USBH_MSC_MAX_TRANSFER_RETRY) {
                     retry = DEF_FALSE;
                 }
                 break;

            case USBH_ERR_NONE:
                 if (data_len_tx < data_len_rem) {
                     data_len_rem -= data_len_tx;
                     p_data_ix    += data_len_tx;
                 } else {
                     data_len_rem = 0u;
                 }
                 break;

            default:
                 retry = DEF_FALSE;
                 break;
        }
    }

    if (err != USBH_ERR_NONE) {
        USBH_PRINT_ERR(err);

        USBH_EP_Reset(p_msc_dev->DevPtr,
                     &p_msc_dev->BulkOutEP);
        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_msc_dev->BulkOutEP);
        } else {
            (void)USBH_MSC_ResetRecovery(p_msc_dev);
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_RxData()
*
* Description : Receive data from device through bulk IN endpoint.
*
* Argument(s) : p_msc_dev        Pointer to MSC device.
*
*               p_arg            Pointer to data buffer.
*
*               data_len         Length of data buffer in octets.
*
* Return(s)   : USBH_ERR_NONE,                          if data successfully received.
*
*                                                       ----- RETURNED BY USBH_BulkRx() : -----
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type is not Bulk or direction is not IN.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_MSC_RxData (USBH_MSC_DEV  *p_msc_dev,
                                   void          *p_arg,
                                   CPU_INT32U     data_len)

{
    USBH_ERR      err;
    CPU_BOOLEAN   retry;
    CPU_INT08U   *p_data_08;
    CPU_INT16U    retry_cnt;
    CPU_INT32U    p_data_ix;
    CPU_INT32U    data_len_rem;
    CPU_INT32U    data_len_rx;


    err          =  USBH_ERR_NONE;
    p_data_ix    =  0u;
    p_data_08    = (CPU_INT08U *)p_arg;
    data_len_rem =  data_len;
    retry_cnt    =  0u;
    retry        =  DEF_TRUE;

    while ((data_len_rem  > 0u      ) &&
           (retry        == DEF_TRUE)) {

        data_len_rx = USBH_BulkRx(        &p_msc_dev->BulkInEP,
                                  (void *)(p_data_08 + p_data_ix),
                                           data_len_rem,
                                           USBH_MSC_TIMEOUT,
                                          &err);
        switch (err) {
            case USBH_ERR_DEV_NOT_RESPONDING:
            case USBH_ERR_HC_IO:
                 retry_cnt++;
                 if (retry_cnt >= USBH_MSC_MAX_TRANSFER_RETRY) {
                     retry = DEF_FALSE;
                 }
                 break;

            case USBH_ERR_NONE:
                 if (data_len_rx < data_len_rem) {
                     data_len_rem -= data_len_rx;               /* Update remaining nbr of octets to read.              */
                     p_data_ix    += data_len_rx;               /* Update buf ix.                                       */
                 } else {
                     data_len_rem = 0u;
                 }
                 break;

            default:
                 retry = DEF_FALSE;
                 break;
        }
    }

    if (err != USBH_ERR_NONE) {
        USBH_PRINT_ERR(err);

        (void)USBH_EP_Reset(p_msc_dev->DevPtr,
                           &p_msc_dev->BulkInEP);               /* Clr err on host side EP.                             */
        if (err == USBH_ERR_EP_STALL) {
            (void)USBH_EP_StallClr(&p_msc_dev->BulkInEP);
            err = USBH_ERR_NONE;
        } else {
            (void)USBH_MSC_ResetRecovery(p_msc_dev);
            err = USBH_ERR_MSC_IO;
        }
    }

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_MSC_ResetRecovery()
*
* Description : Apply bulk-only reset recovery to device on phase error & clear stalled endpoints.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : USBH_ERR_NONE,                          If reset recovery procedure is successful.
*
*                                                       ----- RETURNED BY USBH_MSC_BulkOnlyReset -----
*               USBH_ERR_UNKNOWN,                       Unknown error occurred.
*               USBH_ERR_INVALID_ARG,                   Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE,              Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
*                                                       ----- RETURNED BY USBH_EP_StallClr -----
*               USBH_ERR_UNKNOWN                        Unknown error occurred.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               USBH_ERR_HC_IO,                         Root hub input/output error.
*               USBH_ERR_EP_STALL,                      Root hub does not support request.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_MSC_ResetRecovery (USBH_MSC_DEV  *p_msc_dev)
{
    USBH_ERR  err;


    err = USBH_MSC_BulkOnlyReset(p_msc_dev);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_EP_StallClr(&p_msc_dev->BulkInEP);
    if (err != USBH_ERR_NONE) {
        return (err);
    }

    err = USBH_EP_StallClr(&p_msc_dev->BulkOutEP);

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_MSC_BulkOnlyReset()
*
* Description : Issue bulk-only reset.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
* Return(s)   : Specific error code.
*
* Note(s)     : (1)The "BULK ONLY MASS STORAGE RESET" is an MSC specific request is documented in USB
                   MSC Bulk only transport specification, section 3.1
*********************************************************************************************************
*/

static  USBH_ERR  USBH_MSC_BulkOnlyReset (USBH_MSC_DEV  *p_msc_dev)
{
    USBH_ERR    err;
    CPU_INT08U  if_nbr;


    err    = USBH_ERR_NONE;
    if_nbr = USBH_IF_NbrGet(p_msc_dev->IF_Ptr);

    USBH_CtrlTx(        p_msc_dev->DevPtr,
                        USBH_MSC_REQ_MASS_STORAGE_RESET, /*  See Note(s) #1                                      */
                       (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS | USBH_REQ_RECIPIENT_IF),
                        0u,
                        if_nbr,
                (void *)0,
                        0u,
                        USBH_MSC_TIMEOUT,
                       &err);
    if (err != USBH_ERR_NONE) {
        USBH_PRINT_ERR(err);
        USBH_EP_Reset(           p_msc_dev->DevPtr,
                      (USBH_EP *)0);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                     USBH_SCSI_CMD_StdInquiry()
*
* Description : Read inquiry data of device.
*
* Argument(s) : p_msc_dev               Pointer to MSC device.
*
*               p_msc_inquiry_info      Pointer to inquiry info structure.
*
*               lun                     Logical unit number.
*
* Return(s)   : USBH_ERR_NONE,                          if command is successful.
*
*                                                       ---- RETURNED BY USBH_MSC_XferCmd ----
*               USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*               USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*               USBH_ERR_MSC_IO                         Unable to receive CSW.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SCSI "INQUIRY" command is documented in 'SCSI Primary Commands - 3
*                   (SPC-3)', Section 6.33.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_SCSI_CMD_StdInquiry (USBH_MSC_DEV           *p_msc_dev,
                                            USBH_MSC_INQUIRY_INFO  *p_msc_inquiry_info,
                                            CPU_INT08U              lun)
{
    USBH_ERR    err;
    CPU_INT08U  cmd[6];
    CPU_INT08U  data[0x24];

                                                                /* ------- PREPARE SCSI CMD BLOCK (SEE NOTE #1) ------- */
    cmd[0] = USBH_SCSI_CMD_INQUIRY;                             /* Operation code (0x12).                               */
    cmd[1] = 0u;                                                /* Std inquiry data.                                    */
    cmd[2] = 0u;                                                /* Page code.                                           */
    cmd[3] = 0u;                                                /* Alloc len.                                           */
    cmd[4] = 0x24u;                                             /* Alloc len.                                           */
    cmd[5] = 0u;                                                /* Ctrl.                                                */

                                                                /* ------------------ SEND SCSI CMD ------------------- */
    USBH_MSC_XferCmd(         p_msc_dev,
                              lun,
                              USBH_MSC_DATA_DIR_IN,
                     (void *)&cmd[0],
                              6u,
                     (void *) data,
                              0x24u,
                             &err);
    if (err == USBH_ERR_NONE) {
        p_msc_inquiry_info->DevType      = data[0] &  0x1Fu;
        p_msc_inquiry_info->IsRemovable  = data[1] >> 7u;

        Mem_Copy((void *) p_msc_inquiry_info->Vendor_ID,
                 (void *)&data[8],
                          8u);

        Mem_Copy((void *) p_msc_inquiry_info->Product_ID,
                 (void *)&data[16],
                          16u);

        p_msc_inquiry_info->ProductRevisionLevel = MEM_VAL_GET_INT32U(&data[32]);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                    USBH_SCSI_CMD_TestUnitReady()
*
* Description : Read number of sectors & sector size.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               lun             Logical unit number.
*
* Return(s)   : USBH_ERR_NONE,                          if the command is successful.
*
*                                                       ---- RETURNED BY USBH_MSC_XferCmd ----
*               USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*               USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*               USBH_ERR_MSC_IO                         Unable to receive CSW.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SCSI "TEST UNIT READY" command is documented in 'SCSI Primary Commands - 3
*                   (SPC-3)', Section 6.33.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_SCSI_CMD_TestUnitReady (USBH_MSC_DEV  *p_msc_dev,
                                               CPU_INT08U     lun)
{
    USBH_ERR    err;
    CPU_INT08U  cmd[6];
                                                                /* See Note #1.                                         */
                                                                /* ------------ PREPARE SCSI COMMAND BLOCK ------------ */
    cmd[0] = USBH_SCSI_CMD_TEST_UNIT_READY;                     /* Operation code (0x00).                               */
    cmd[1] = 0u;                                                /* Reserved.                                            */
    cmd[2] = 0u;                                                /* Reserved.                                            */
    cmd[3] = 0u;                                                /* Reserved.                                            */
    cmd[4] = 0u;                                                /* Reserved.                                            */
    cmd[5] = 0u;                                                /* Control.                                             */

    USBH_MSC_XferCmd(         p_msc_dev,                        /* ---------------- SEND SCSI COMMAND ----------------- */
                              lun,
                              USBH_MSC_DATA_DIR_NONE,
                     (void *)&cmd[0],
                              6u,
                     (void *) 0,
                              0u,
                             &err);

    return (err);
}


/*
*********************************************************************************************************
*                                      USBH_SCSI_CMD_ReqSense()
*
* Description : Issue command to obtain sense data.
*
* Argument(s) : p_msc_dev       Pointer to USBH_MSC_DEV device.
*
*               lun             Logical unit number
*
*               p_arg           Pointer to data buffer.
*
*               data_len        Number of data bytes to receive.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*                       USBH_ERR_NONE                           Command successfully transmitted.
*
*                                                               ---- RETURNED BY USBH_MSC_XferCmd ----
*                       USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*                       USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*                       USBH_ERR_MSC_IO                         Unable to receive CSW.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*                       USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets sent.
*
* Note(s)     : (1) The SCSI "REQUEST SENSE" command is documented in 'SCSI Primary Commands - 3
*                   (SPC-3)', Section 6.27.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_SCSI_CMD_ReqSense (USBH_MSC_DEV  *p_msc_dev,
                                            CPU_INT08U     lun,
                                            CPU_INT08U    *p_arg,
                                            CPU_INT32U     data_len,
                                            USBH_ERR      *p_err)
{
    CPU_INT08U  cmd[6];
    CPU_INT32U  xfer_len;

                                                                /* See Note(s) #1                                       */
                                                                /* ------------ PREPARE SCSI COMMAND BLOCK ------------ */
    cmd[0] =  USBH_SCSI_CMD_REQUEST_SENSE;                      /* Operation code (0x03).                               */
    cmd[1] =  0u;                                               /* Reserved.                                            */
    cmd[2] =  0u;                                               /* Reserved.                                            */
    cmd[3] =  0u;                                               /* Reserved.                                            */
    cmd[4] = (CPU_INT08U)data_len;                              /* Allocation length.                                   */
    cmd[5] =  0u;                                               /* Ctrl.                                                */

                                                                /* ------------------ SEND SCSI CMD ------------------- */
    xfer_len = USBH_MSC_XferCmd(        p_msc_dev,
                                        lun,
                                        USBH_MSC_DATA_DIR_IN,
                                (void *)cmd,
                                        6u,
                                (void *)p_arg,
                                        data_len,
                                        p_err);

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                      USBH_SCSI_GetSenseInfo()
*
* Description : Obtain sense data.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               lun             Logical unit number.
*
*               p_sense_key     Pointer to variable that will receive sense key.
*
*               p_asc           Pointer to variable that will receive additional sense code.
*
*               p_ascq          Pointer to variable that will receive additional sense code qualifier.
*
* Return(s)   : USBH_ERR_NONE,                          if command is successful.
*               USBH_ERR_UNKNOWN,                       if unknown error occured.
*
*                                                       ---- RETURNED BY USBH_SCSI_CMD_ReqSense ----
*               USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*               USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*               USBH_ERR_MSC_IO                         Unable to receive CSW.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SCSI "REQUEST SENSE" command is documented in 'SCSI Primary Commands - 3
*                   (SPC-3)', Section 6.27.
*
*               (2) The format of the sense data is documented in 'SCSI Primary Commands - 3 (SPC-3)',
*                   Section 4.5.3. :
*                   (a) The response code in the 0th byte of the sense data should be either 0x70 or 0x71.
*                   (b) The lower nibble of the  2nd byte of the sense data is the sense key.
*                   (c) The 12th byte of the sense data is the additional sense code (ASC).
*                   (d) The 13th byte of the sense data is the additional sense code (ASC) qualifier.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_SCSI_GetSenseInfo (USBH_MSC_DEV  *p_msc_dev,
                                          CPU_INT08U     lun,
                                          CPU_INT08U    *p_sense_key,
                                          CPU_INT08U    *p_asc,
                                          CPU_INT08U    *p_ascq)
{
    USBH_ERR    err;
    CPU_INT32U  xfer_len;
    CPU_INT08U  sense_data[18];
    CPU_INT08U  resp_code;


    xfer_len = USBH_SCSI_CMD_ReqSense(p_msc_dev,                     /* Issue SCSI request sense cmd.                        */
                                      lun,
                                      sense_data,
                                      18u,
                                     &err);
    if (err == USBH_ERR_NONE) {
        resp_code = sense_data[0] & 0x7Fu;

        if ( (xfer_len  >=   13u) &&
            ((resp_code == 0x70u) || (resp_code == 0x71u))) {
            *p_sense_key = sense_data[2] & 0x0Fu;
            *p_asc       = sense_data[12];
            *p_ascq      = sense_data[13];
             err         = USBH_ERR_NONE;

        } else {
            *p_sense_key = 0u;
            *p_asc       = 0u;
            *p_ascq      = 0u;
             err         = USBH_ERR_UNKNOWN;
#if (USBH_CFG_PRINT_LOG == DEF_ENABLED)
             USBH_PRINT_LOG("ERROR Invalid SENSE response from device - ");
             USBH_PRINT_LOG("xfer_len : %d, sense_data[0] : %02X\r\n", xfer_len, sense_data[0]);
#endif
        }

    } else {
        *p_sense_key = 0u;
        *p_asc       = 0u;
        *p_ascq      = 0u;
         USBH_PRINT_ERR(err);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                     USBH_SCSI_CMD_CapacityRd()
*
* Description : Read number of sectors & sector size.
*
* Argument(s) : p_msc_dev       Pointer to MSC device.
*
*               lun             Logical unit number.
*
*               p_nbr_blks      Pointer to variable that will receive number of blocks.
*
*               p_blk_size      Pointer to variable that will receive block size.
*
* Return(s)   : USBH_ERR_NONE,                          if command is successful.
*
*                                                       ---- RETURNED BY USBH_MSC_XferCmd ----
*               USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*               USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*               USBH_ERR_MSC_IO                         Unable to receive CSW.
*               USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*               USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*               USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*               Host controller drivers error code,     Otherwise.
*
* Note(s)     : (1) The SCSI "READ CAPAPITY (10)" command is documented in 'SCSI Block Commands - 2
*                   (SBC-2)', Section 5.10.
*********************************************************************************************************
*/

static  USBH_ERR  USBH_SCSI_CMD_CapacityRd (USBH_MSC_DEV  *p_msc_dev,
                                            CPU_INT08U     lun,
                                            CPU_INT32U    *p_nbr_blks,
                                            CPU_INT32U    *p_blk_size)
{
    CPU_INT08U  cmd[10];
    CPU_INT08U  data[8];
    USBH_ERR    err;
                                                                /* See Note #1.                                         */
                                                                /* -------------- PREPARE SCSI CMD BLOCK -------------- */
    cmd[0] = USBH_SCSI_CMD_READ_CAPACITY;                       /* Operation code (0x25).                               */
    cmd[1] = 0u;                                                /* Reserved.                                            */
    cmd[2] = 0u;                                                /* Logical Block Address (MSB).                         */
    cmd[3] = 0u;                                                /* Logical Block Address.                               */
    cmd[4] = 0u;                                                /* Logical Block Address.                               */
    cmd[5] = 0u;                                                /* Logical Block Address (LSB).                         */
    cmd[6] = 0u;                                                /* Reserved.                                            */
    cmd[7] = 0u;                                                /* Reserved.                                            */
    cmd[8] = 0u;                                                /* ----------------------- PMI. ----------------------- */
    cmd[9] = 0u;                                                /* Control.                                             */

    USBH_MSC_XferCmd (        p_msc_dev,                        /* ------------------ SEND SCSI CMD ------------------- */
                              lun,
                              USBH_MSC_DATA_DIR_IN,
                      (void *)cmd,
                              10u,
                      (void *)data,
                              8u,
                             &err);
    if (err == USBH_ERR_NONE) {                                 /* -------------- HANDLE DEVICE RESPONSE -------------- */
        MEM_VAL_COPY_GET_INT32U_BIG(p_nbr_blks, &data[0]);
       *p_nbr_blks += 1u;
        MEM_VAL_COPY_GET_INT32U_BIG(p_blk_size, &data[4]);
    }

    return (err);
}


/*
*********************************************************************************************************
*                                           USBH_SCSI_Rd()
*
* Description : Read specified number of blocks from device.
*
* Argument(s) : p_msc_dev        Pointer to MSC device.
*
*               lun              Logical unit number.
*
*               blk_addr         Block address.
*
*               nbr_blks         Number of blocks to read.
*
*               blk_size         Block size.
*
*               p_arg            Pointer to data buffer.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*                       USBH_ERR_NONE                           Data successfully received.
*
*                                                               ---- RETURNED BY USBH_MSC_XferCmd ----
*                       USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*                       USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*                       USBH_ERR_MSC_IO                         Unable to receive CSW.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*                       USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of bytes read.
*
* Note(s)     : (1) The SCSI "READ (10): command is documented in 'SCSI Block Commands - 2 (SBC-2)',
*                   Section 5.6.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_SCSI_Rd (USBH_MSC_DEV  *p_msc_dev,
                                  CPU_INT08U     lun,
                                  CPU_INT32U     blk_addr,
                                  CPU_INT16U     nbr_blks,
                                  CPU_INT32U     blk_size,
                                  void          *p_arg,
                                  USBH_ERR      *p_err)
{
    CPU_INT08U  cmd[10];
    CPU_INT32U  data_len;
    CPU_INT32U  xfer_len;


    data_len = nbr_blks * blk_size;
                                                                /* See Note #1.                                         */
                                                                /* -------------- PREPARE SCSI CMD BLOCK -------------- */
    cmd[0] = USBH_SCSI_CMD_READ_10;                             /* Operation code (0x28).                               */
    cmd[1] = 0u;                                                /* Reserved.                                            */
    MEM_VAL_COPY_SET_INT32U_BIG(&cmd[2], &blk_addr);            /* Logcal Block Address (LBA).                          */
    cmd[6] = 0u;                                                /* Reserved.                                            */
    MEM_VAL_COPY_SET_INT16U_BIG(&cmd[7], &nbr_blks);            /* Transfer length (number of logical blocks).          */
    cmd[9] = 0u;                                                /* Control.                                             */

    xfer_len = USBH_MSC_XferCmd(         p_msc_dev,             /* ---------------- SEND SCSI COMMAND ----------------- */
                                         lun,
                                         USBH_MSC_DATA_DIR_IN,
                                (void *)&cmd[0],
                                         10u,
                                         p_arg,
                                         data_len,
                                         p_err);
    if (*p_err != USBH_ERR_NONE) {
        xfer_len = (CPU_INT32U)0;
    }

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                           USBH_SCSI_Wr()
*
* Description : Write specified number of blocks to device.
*
* Argument(s) : p_msc_dev        Pointer to MSC device.
*
*               lun              Logical unit number.
*
*               blk_addr         Block address.
*
*               nbr_blks         Number of blocks to write.
*
*               blk_size         Block size.
*
*               p_arg            Pointer to data buffer.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*                       USBH_ERR_NONE                           Data successfully transmitted.
*
*                                                               ---- RETURNED BY USBH_MSC_XferCmd ----
*                       USBH_ERR_MSC_CMD_FAILED                 Device reports command failed.
*                       USBH_ERR_MSC_CMD_PHASE                  Device reports command phase error.
*                       USBH_ERR_MSC_IO                         Unable to receive CSW.
*                       USBH_ERR_INVALID_ARG                    Invalid argument passed to 'p_ep'.
*                       USBH_ERR_EP_INVALID_TYPE                Endpoint type or direction is incorrect.
*                       USBH_ERR_EP_INVALID_STATE               Endpoint is not opened.
*                       Host controller drivers error code,     Otherwise.
*
* Return(s)   : Number of octets written.
*
* Note(s)     : (1) The SCSI "WRITE (10)" command is documented in 'SCSI Block Commands - 2 (SBC-2)',
*                   Section 5.25.
*********************************************************************************************************
*/

static  CPU_INT32U  USBH_SCSI_Wr (USBH_MSC_DEV  *p_msc_dev,
                                  CPU_INT08U     lun,
                                  CPU_INT32U     blk_addr,
                                  CPU_INT16U     nbr_blks,
                                  CPU_INT32U     blk_size,
                                  const  void   *p_arg,
                                  USBH_ERR      *p_err)

{
    CPU_INT08U  cmd[10];
    CPU_INT32U  data_len;
    CPU_INT32U  xfer_len;


    data_len = nbr_blks * blk_size;
                                                                /* See Note #1.                                         */
                                                                /* -------------- PREPARE SCSI CMD BLOCK -------------- */
    cmd[0] = USBH_SCSI_CMD_WRITE_10;                            /* Operation code (0x2A).                               */
    cmd[1] = 0u;                                                /* Reserved.                                            */
    MEM_VAL_COPY_SET_INT32U_BIG(&cmd[2], &blk_addr);            /* Logical Block Address (LBA).                         */
    cmd[6] = 0u;                                                /* Reserved.                                            */
    MEM_VAL_COPY_SET_INT16U_BIG(&cmd[7], &nbr_blks);            /* Transfer length (number of logical blocks).          */
    cmd[9] = 0u;                                                /* Control.                                             */

    xfer_len = USBH_MSC_XferCmd(         p_msc_dev,             /* ------------------ SEND SCSI CMD ------------------- */
                                         lun,
                                         USBH_MSC_DATA_DIR_OUT,
                                (void *)&cmd[0],
                                         10u,
                                (void *) p_arg,
                                         data_len,
                                         p_err);
    if (*p_err != USBH_ERR_NONE) {
        xfer_len = 0u;
    }

    return (xfer_len);
}


/*
*********************************************************************************************************
*                                          USBH_MSC_FmtCBW()
*
* Description : Format CBW from CBW structure.
*
* Argument(s) : p_cbw           Variable that contains CBW information.
*
*               p_buf_dest      Pointer to buffer that will receive CBW content.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_FmtCBW (USBH_MSC_CBW  *p_cbw,
                               void          *p_buf_dest)
{
    CPU_INT08U     i;
    USBH_MSC_CBW  *p_buf_dest_cbw;


    p_buf_dest_cbw = (USBH_MSC_CBW *)p_buf_dest;

    p_buf_dest_cbw->dCBWSignature          = MEM_VAL_GET_INT32U_LITTLE(&p_cbw->dCBWSignature);
    p_buf_dest_cbw->dCBWTag                = MEM_VAL_GET_INT32U_LITTLE(&p_cbw->dCBWTag);
    p_buf_dest_cbw->dCBWDataTransferLength = MEM_VAL_GET_INT32U_LITTLE(&p_cbw->dCBWDataTransferLength);
    p_buf_dest_cbw->bmCBWFlags             = p_cbw->bmCBWFlags;
    p_buf_dest_cbw->bCBWLUN                = p_cbw->bCBWLUN;
    p_buf_dest_cbw->bCBWCBLength           = p_cbw->bCBWCBLength;

    for (i = 0u; i < 16u; i++) {
        p_buf_dest_cbw->CBWCB[i] = p_cbw->CBWCB[i];
    }
}


/*
*********************************************************************************************************
*                                         USBH_MSC_ParseCSW()
*
* Description : Parse CSW into CSW structure.
*
* Argument(s) : p_csw           Variable that will receive CSW parsed in this function.
*
*               p_buf_src       Pointer to buffer that contains CSW.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  USBH_MSC_ParseCSW (USBH_MSC_CSW  *p_csw,
                                 void          *p_buf_src)
{
    USBH_MSC_CSW  *p_buf_src_cbw;


    p_buf_src_cbw = (USBH_MSC_CSW *)p_buf_src;

    p_csw->dCSWSignature   = MEM_VAL_GET_INT32U_LITTLE(&p_buf_src_cbw->dCSWSignature);
    p_csw->dCSWTag         = MEM_VAL_GET_INT32U_LITTLE(&p_buf_src_cbw->dCSWTag);
    p_csw->dCSWDataResidue = MEM_VAL_GET_INT32U_LITTLE(&p_buf_src_cbw->dCSWDataResidue);
    p_csw->bCSWStatus      = p_buf_src_cbw->bCSWStatus;
}

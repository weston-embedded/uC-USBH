/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                               USB HOST HID KEYBOARD TEST APPLICATION
*
*                                              TEMPLATE
*
* Filename : app_usbh_keyboard.c
* Version  : V3.42.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   APP_USBH_KEYBOARD_MODULE
#include  "app_usbh_keyboard.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  KBD_LEFT_CTRL                                  0x01u
#define  KBD_LEFT_SHIFT                                 0x02u
#define  KBD_LEFT_ALT                                   0x04u
#define  KBD_LEFT_GUI                                   0x08u
#define  KBD_RIGHT_CTRL                                 0x10u
#define  KBD_RIGHT_SHIFT                                0x20u
#define  KBD_RIGHT_ALT                                  0x40u
#define  KBD_RIGHT_GUI                                  0x80u
#define  KBR_MAX_NBR_PRESSED                               6u


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/

static  const  CPU_INT08U  App_USBH_KBD_HID_ToKBD[] = {
      0u,    0u,    0u,    0u,   31u,   50u,   48u,   33u,   19u,   34u,   35u,   36u,   24u,   37u,   38u,   39u,       /* 0x00 - 0x0F */
     52u,   51u,   25u,   26u,   17u,   20u,   32u,   21u,   23u,   49u,   18u,   47u,   22u,   46u,    2u,    3u,       /* 0x10 - 0x1F */
      4u,    5u,    6u,    7u,    8u,    9u,   10u,   11u,   43u,  110u,   15u,   16u,   61u,   12u,   13u,   27u,       /* 0x20 - 0x2F */
     28u,   29u,   42u,   40u,   41u,    1u,   53u,   54u,   55u,   30u,  112u,  113u,  114u,  115u,  116u,  117u,       /* 0x30 - 0x3F */
    118u,  119u,  120u,  121u,  122u,  123u,  124u,  125u,  126u,   75u,   80u,   85u,   76u,   81u,   86u,   89u,       /* 0x40 - 0x4F */
     79u,   84u,   83u,   90u,   95u,  100u,  105u,  106u,  108u,   93u,   98u,  103u,   92u,   97u,  102u,   91u,       /* 0x50 - 0x5F */
     96u,  101u,   99u,  104u,   45u,  129u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0x60 - 0x6F */
      0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0x70 - 0x7F */
      0u,    0u,    0u,    0u,    0u,  107u,    0u,   56u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0x80 - 0x8F */
      0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0x90 - 0x9F */
      0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0xA0 - 0xAF */
      0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0xB0 - 0xBF */
      0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0xC0 - 0xCF */
      0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,    0u,       /* 0xD0 - 0xDF */
     58u,   44u,   60u,  127u,   64u,   57u,   62u,  128u                                                                /* 0xE0 - 0xE7 */
};

static  const  CPU_INT08S  App_USBH_KBD_Key[] = {
    '\0',  '`',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\0', '\0',
    '\t',  'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\\',
    '\0',  'a',  's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  '\'', '\0', '\n',
    '\0',  '\0', 'z',  'x',  'c',  'v',  'b',  'n',  'm',  ',',  '.',  '/',  '\0', '\0',
    '\0',  '\0', '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0',  '\0', '\0', '\0', '\0', '\r', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0',  '\0', '7',  '4',  '1',
    '\0',  '/',  '8',  '5',  '2',
    '0',   '*',  '9',  '6',  '3',
    '.',   '-',  '+',  '\0', '\n', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

static  const  CPU_INT08S  App_USBH_KBD_ShiftKey[] = {
    '\0', '~',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+',  '\0', '\0',
    '\0', 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '|',
    '\0', 'A',  'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',  '\0', '\n',
    '\0', '\0', 'Z',  'X',  'C',  'V',  'B',  'N',  'M',  '<',  '>',  '?',  '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};


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

static  CPU_BOOLEAN  App_USBH_KBD_Shift;
static  CPU_INT08U   App_USBH_KBD_Keys[KBR_MAX_NBR_PRESSED];
static  CPU_INT08U   App_USBH_KBD_KeysNew[KBR_MAX_NBR_PRESSED];
static  CPU_INT08U   App_USBH_KBD_KeysLast[KBR_MAX_NBR_PRESSED];
static  CPU_INT08U   App_USBH_KBD_NbrKeys;
static  CPU_INT08U   App_USBH_KBD_KeyNewest;
static  CPU_INT08U   App_USBH_KBD_NbrKeysNew;
static  CPU_INT08U   App_USBH_KBD_NbrKeysLast;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_KBD_PrintKeys (CPU_INT08U  *p_buf);


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
*                                       App_USBH_KBD_CallBack()
*
* Description : Callback function for keyboard events.
*
* Argument(s) : p_hid_dev       Pointer to the HID class device.
*
*               p_buf           Pointer to the data sent by the keyboard.
*
*               buf_len         Buffer length.
*
*               err             Status of the callback.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  App_USBH_KBD_CallBack (USBH_HID_DEV  *p_hid_dev,
                             void          *p_buf,
                             CPU_INT08U     buf_len,
                             USBH_ERR       err)
{
    (void)p_hid_dev;

    if ((err     == USBH_ERR_NONE) &&
        (buf_len >= 3u)) {
        App_USBH_KBD_PrintKeys(p_buf);
    }
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
*                                      App_USBH_KBD_PrintKeys()
*
* Description : Print keys pressed.
*
* Argument(s) : p_buf           Buffer pointer.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  App_USBH_KBD_PrintKeys (CPU_INT08U  *p_buf)
{
    CPU_INT08U  ix;
    CPU_INT08U  jx;
    CPU_INT08U  error;


    App_USBH_KBD_NbrKeys     = 0u;                              /* Initialize variables & key data.                     */
    App_USBH_KBD_NbrKeysNew  = 0u;
    App_USBH_KBD_NbrKeysLast = 0u;
    App_USBH_KBD_KeyNewest   = 0u;

    if ((p_buf[0u] == KBD_LEFT_SHIFT) ||                        /* Determine if shift key pressed.                      */
        (p_buf[0u] == KBD_RIGHT_SHIFT)) {
        App_USBH_KBD_Shift = DEF_TRUE;
    } else {
        App_USBH_KBD_Shift = DEF_FALSE;
    }

    error = DEF_FALSE;
    for (ix = 2u; ix < (2u + KBR_MAX_NBR_PRESSED); ix++) {      /* Determine if error encountered.                      */
        if ((p_buf[ix] == 0x01u) ||
            (p_buf[ix] == 0x02u) ||
            (p_buf[ix] == 0x03u)) {
            error = DEF_TRUE;
        }
    }

    if (error == DEF_TRUE) {
        return;
    }

    App_USBH_KBD_NbrKeys    = 0u;                               /* Determine which keys are pressed.                    */
    App_USBH_KBD_NbrKeysNew = 0u;
    for (ix = 2u; ix < (2u + KBR_MAX_NBR_PRESSED); ix++) {
        if (p_buf[ix] != 0u) {
            App_USBH_KBD_Keys[App_USBH_KBD_NbrKeys] = p_buf[ix];/* If pressed, add to array.                            */
            App_USBH_KBD_NbrKeys++;
            for (jx = 0u; jx < App_USBH_KBD_NbrKeysLast; jx++) {/* Determine if key was already pressed.                */
                if (p_buf[ix] == App_USBH_KBD_KeysLast[jx]) {
                    break;
                }
            }
                                                                /* If key was not already pressed, add to array.        */
            if (jx == App_USBH_KBD_NbrKeysLast) {
                App_USBH_KBD_KeysNew[App_USBH_KBD_NbrKeysNew] = p_buf[ix];
                App_USBH_KBD_NbrKeysNew++;
            }
        }
    }

    if (App_USBH_KBD_NbrKeysNew == 1u) {                        /* If only one new key pressed, print character.        */
        App_USBH_KBD_KeyNewest = App_USBH_KBD_KeysNew[0u];
        if (App_USBH_KBD_Shift == DEF_TRUE) {
            APP_TRACE_INFO(("%c", App_USBH_KBD_ShiftKey[App_USBH_KBD_HID_ToKBD[App_USBH_KBD_KeyNewest]]));
        } else {
            APP_TRACE_INFO(("%c", App_USBH_KBD_Key[App_USBH_KBD_HID_ToKBD[App_USBH_KBD_KeyNewest]]));
        }
    } else {
        App_USBH_KBD_KeyNewest = 0x00u;
    }

                                                                /* Prepare for next loop.                               */
    App_USBH_KBD_NbrKeysLast = App_USBH_KBD_NbrKeys;
    for (ix = 0u; ix < KBR_MAX_NBR_PRESSED; ix++) {
        App_USBH_KBD_KeysLast[ix] = App_USBH_KBD_Keys[ix];
    }
}


/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

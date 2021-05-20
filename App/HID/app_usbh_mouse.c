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
*                                 USB HOST HID MOUSE TEST APPLICATION
*
*                                              TEMPLATE
*
* Filename : app_usbh_mouse.c
* Version  : V3.42.01
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define   APP_USBH_MOUSE_MODULE
#include  "app_usbh_mouse.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  MOUSE_BUTTON_1                                 0x01u
#define  MOUSE_BUTTON_2                                 0x02u
#define  MOUSE_BUTTON_3                                 0x04u


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

static  CPU_INT08U  App_USBH_MouseStatePrev = 0u;
static  CPU_INT32S  App_USBH_MousePosX      = 0u;
static  CPU_INT32S  App_USBH_MousePosY      = 0u;


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  App_USBH_MouseDisplayData (CPU_INT08S  *p_buf);


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
*                                      App_USBH_MouseCallBack()
*
* Description : Callback function for mouse events.
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

void  App_USBH_MouseCallBack (USBH_HID_DEV  *p_hid_dev,
                              void          *p_buf,
                              CPU_INT08U     buf_len,
                              USBH_ERR       err)
{
    (void)p_hid_dev;

    if ((err     == USBH_ERR_NONE) &&
        (buf_len >= 3u)) {
        App_USBH_MouseDisplayData(p_buf);
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
*                                     App_USBH_MouseDisplayData()
*
* Description : Print buttons pressed & pointer location.
*
* Argument(s) : pbuf            Buffer pointer.
*
* Return(s)   : None.
*
* Note(s)     : This function parses the report buffer of a simple/basic mouse. It might require to be
*               modified when used with a complex mouse
*********************************************************************************************************
*/

static  void  App_USBH_MouseDisplayData (CPU_INT08S  *p_buf)
{
    CPU_INT08U   state;
    CPU_BOOLEAN  pressed_prev;
    CPU_BOOLEAN  pressed;


    state        = (CPU_INT08U)p_buf[0];
    pressed      = (state                   & MOUSE_BUTTON_1) == MOUSE_BUTTON_1 ? DEF_TRUE : DEF_FALSE;
    pressed_prev = (App_USBH_MouseStatePrev & MOUSE_BUTTON_1) == MOUSE_BUTTON_1 ? DEF_TRUE : DEF_FALSE;

    if ((pressed      == DEF_TRUE) &&
        (pressed_prev == DEF_FALSE)) {
        APP_TRACE_INFO(("Left   button pressed\r\n"));
    }
    if ((pressed      == DEF_FALSE) &&
        (pressed_prev == DEF_TRUE)) {
        APP_TRACE_INFO(("Left   button released\r\n"));
    }

    pressed      = (state                   & MOUSE_BUTTON_3) == MOUSE_BUTTON_3 ? DEF_TRUE : DEF_FALSE;
    pressed_prev = (App_USBH_MouseStatePrev & MOUSE_BUTTON_3) == MOUSE_BUTTON_3 ? DEF_TRUE : DEF_FALSE;

    if ((pressed      == DEF_TRUE) &&
        (pressed_prev == DEF_FALSE)) {
        APP_TRACE_INFO(("Middle button pressed\r\n"));
    }
    if ((pressed      == DEF_FALSE) &&
        (pressed_prev == DEF_TRUE)) {
        APP_TRACE_INFO(("Middle button released\r\n"));
    }

    pressed      = (state                   & MOUSE_BUTTON_2) == MOUSE_BUTTON_2 ? DEF_TRUE : DEF_FALSE;
    pressed_prev = (App_USBH_MouseStatePrev & MOUSE_BUTTON_2) == MOUSE_BUTTON_2 ? DEF_TRUE : DEF_FALSE;

    if ((pressed      == DEF_TRUE) &&
        (pressed_prev == DEF_FALSE)) {
        APP_TRACE_INFO(("Right  button pressed\r\n"));
    }
    if ((pressed      == DEF_FALSE) &&
        (pressed_prev == DEF_TRUE)) {
        APP_TRACE_INFO(("Right  button released\r\n"));
    }

    App_USBH_MouseStatePrev = state;

    if ((p_buf[1u] != 0u) ||
        (p_buf[2u] != 0u)) {
        App_USBH_MousePosX += p_buf[1u];
        App_USBH_MousePosY += p_buf[2u];

        APP_TRACE_INFO(("Pointer at (x, y) = (%d, %d)\r\n",
                        App_USBH_MousePosX,
                        App_USBH_MousePosY));
    }
}

/*
*********************************************************************************************************
*                                                  END
*********************************************************************************************************
*/

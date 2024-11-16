#include "stc.h"
#include "usb.h"
#include "usb_req_class.h"
#include "util.h"
#include "uart.h"

LINECODING LineCoding;

void usb_req_class()
{
    switch (Setup.bRequest)
    {
    case SET_LINE_CODING:
        usb_set_line_coding();
        break;
    case GET_LINE_CODING:
        usb_get_line_coding();
        break;
    case SET_CONTROL_LINE_STATE:
        usb_set_ctrl_line_state();
        break;
    default:
        usb_setup_stall();
        return;
    }
}

void usb_set_line_coding()
{
    if ((DeviceState != DEVSTATE_CONFIGURED) ||
        (Setup.bmRequestType != (OUT_DIRECT | CLASS_REQUEST | INTERFACE_RECIPIENT)))
    {
        usb_setup_stall();
        return;
    }

    Ep0State.pData = (BYTE *)&LineCoding;
    Ep0State.wSize = Setup.wLength;

    usb_setup_out();
}

void usb_get_line_coding()
{
    if ((DeviceState != DEVSTATE_CONFIGURED) ||
        (Setup.bmRequestType != (IN_DIRECT | CLASS_REQUEST | INTERFACE_RECIPIENT)))
    {
        usb_setup_stall();
        return;
    }

    Ep0State.pData = (BYTE *)&LineCoding;
    Ep0State.wSize = Setup.wLength;

    usb_setup_in();
}

void usb_set_ctrl_line_state()
{
    if ((DeviceState != DEVSTATE_CONFIGURED) ||
        (Setup.bmRequestType != (OUT_DIRECT | CLASS_REQUEST | INTERFACE_RECIPIENT)))
    {
        usb_setup_stall();
        return;
    }
    
    //可以从这里读取DTR和RTS的状态
    // Setup.wValueL&0x01	//DTR
    // Setup.wValueL&0x02	//RTS
    CDC_DTR=!!(Setup.wValueL&0x01);
	CDC_RTS=!!(Setup.wValueL&0x02);
	
    usb_setup_status();
}

void usb_uart_settings()
{
}
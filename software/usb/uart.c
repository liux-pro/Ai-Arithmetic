/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/* --- BBS: www.STCAIMCU.com  -----------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了STC的资料及程序            */
/* 本例程为复杂应用程序, 已测试通过, 用户自行理解领悟, 不提供技术支持      */
/*---------------------------------------------------------------------*/

#include "stc.h"
#include "uart.h"
#include "usb.h"

BOOL CDC_DTR;
BOOL CDC_RTS;

//当RxFlag为1时,表示已接收到CDC串口数据
//接收的数据大小保存在RxCount里面,每个包最多接收64字节
//数据保存在RxBuffer缓冲区
BOOL RxFlag;
BYTE RxCount;   
BYTE xdata RxBuffer[64];

BYTE xdata TxBuffer[1024];

void uart_init()
{
    RxFlag = 0;
    RxCount = 0;
}

void uart_send(int size)
{
    BYTE cnt;
    int addr;
    
    IE2 &= ~EUSB;
    usb_write_reg(INDEX, 1);
    
    addr = 0;
    do
    {
        cnt = size > 64 ? 64 : size;
        while (usb_read_reg(INCSR1) & INIPRDY);
        usb_bulk_intr_in(&TxBuffer[addr], cnt, 1);
        addr += cnt;
        size -= cnt;
    } while (cnt >= 64);
    
    IE2 |= EUSB;
}

//对接收的数据处理完成后,一定要调用一次这个函数,以便CDC接收下一笔串口数据
void uart_recv_done()
{
    IE2 &= ~EUSB;
    
    RxFlag = 0;
    usb_write_reg(INDEX, 1);
    usb_write_reg(OUTCSR1, 0);
    
    IE2 |= EUSB;
}

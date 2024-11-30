/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/* --- BBS: www.STCAIMCU.com  -----------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* ���Ҫ�ڳ�����ʹ�ô˴���,���ڳ�����ע��ʹ����STC�����ϼ�����            */
/* ������Ϊ����Ӧ�ó���, �Ѳ���ͨ��, �û������������, ���ṩ����֧��      */
/*---------------------------------------------------------------------*/

#include "stc.h"
#include "uart.h"
#include "usb.h"

BOOL CDC_DTR;
BOOL CDC_RTS;

//��RxFlagΪ1ʱ,��ʾ�ѽ��յ�CDC��������
//���յ����ݴ�С������RxCount����,ÿ����������64�ֽ�
//���ݱ�����RxBuffer������
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

//�Խ��յ����ݴ�����ɺ�,һ��Ҫ����һ���������,�Ա�CDC������һ�ʴ�������
void uart_recv_done()
{
    IE2 &= ~EUSB;
    
    RxFlag = 0;
    usb_write_reg(INDEX, 1);
    usb_write_reg(OUTCSR1, 0);
    
    IE2 |= EUSB;
}

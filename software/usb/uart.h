#ifndef __UART_H__
#define __UART_H__

//当RxFlag为1时,表示已接收到CDC串口数据
//接收的数据大小保存在RxCount里面,每个包最多接收64字节
//数据保存在RxBuffer缓冲区

void uart_init();
void uart_recv_done();
void uart_send(int size);

//只读
extern BOOL CDC_DTR;
//只读
extern BOOL CDC_RTS;

extern BOOL RxFlag;
extern BYTE RxCount;
extern BYTE xdata RxBuffer[64];
extern BYTE xdata TxBuffer[1024];


#endif

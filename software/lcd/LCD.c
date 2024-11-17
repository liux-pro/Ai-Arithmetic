#include "lcd.h"
#include "stdlib.h"
#include "font.h"
#include "string.h"

// LCD的画笔颜色和背景色
u16 POINT_COLOR = 0x0000; // 画笔颜色
u16 BACK_COLOR = 0xFFFF;  // 背景色

// 管理LCD重要参数
// 默认为竖屏
_lcd_dev lcddev;

//-----------------变量定义----------------
bit B_SPI_DMA_busy;	   // SPI DMA忙标志， 1标志SPI-DMA忙，SPI DMA中断中清除此标志，使用SPI DMA前要确认此标志为0
u16 SPI_TxAddr;		   // SPI DMA要发送数据的首地址
u8 xdata DisTmp[3200]; // 显示缓冲，将要显示的内容放在显存里，启动DMA即可. 由于LCM DMA有4字节对齐问题，所以这里定位对地址为4的倍数

// DMA_SPI_CR 	SPI_DMA控制寄存器
#define DMA_ENSPI (1 << 7)	// SPI DMA功能使能控制位，    bit7, 0:禁止SPI DMA功能，  1：允许SPI DMA功能。
#define SPI_TRIG_M (1 << 6) // SPI DMA主机模式触发控制位，bit6, 0:写0无效，          1：写1开始SPI DMA主机模式操作。
#define SPI_TRIG_S (0 << 5) // SPI DMA从机模式触发控制位，bit5, 0:写0无效，          1：写1开始SPI DMA从机模式操作。
#define SPI_CLRFIFO 1		// 清除SPI DMA接收FIFO控制位，bit0, 0:写0无效，          1：写1复位FIFO指针。

// DMA_SPI_CFG 	SPI_DMA配置寄存器
#define DMA_SPIIE (1 << 7)	// SPI DMA中断使能控制位，bit7, 0:禁止SPI DMA中断，     1：允许中断。
#define SPI_ACT_TX (1 << 6) // SPI DMA发送数据控制位，bit6, 0:禁止SPI DMA发送数据，主机只发时钟不发数据，从机也不发. 1：允许发送。
#define SPI_ACT_RX (0 << 5) // SPI DMA接收数据控制位，bit5, 0:禁止SPI DMA接收数据，主机只发时钟不收数据，从机也不收. 1：允许接收。
#define DMA_SPIIP (0 << 2)	// SPI DMA中断优先级控制位，bit3~bit2, (最低)0~3(最高).
#define DMA_SPIPTY 0		// SPI DMA数据总线访问优先级控制位，bit1~bit0, (最低)0~3(最高).

// DMA_SPI_CFG2 	SPI_DMA配置寄存器2
#define SPI_WRPSS (0 << 2) // SPI DMA过程中使能SS脚控制位，bit2, 0: SPI DMA传输过程不自动控制SS脚。  1：自动拉低SS脚。
#define SPI_SSS 3		   // SPI DMA过程中自动控制SS脚选择位，bit1~bit0, 0: P1.4,  1：P2.4,  2: P4.0,  3:P3.5。

// DMA_SPI_STA 	SPI_DMA状态寄存器
#define SPI_TXOVW (1 << 2)	// SPI DMA数据覆盖标志位，bit2, 软件清0.
#define SPI_RXLOSS (1 << 1) // SPI DMA接收数据丢弃标志位，bit1, 软件清0.
#define DMA_SPIIF 1			// SPI DMA中断请求标志位，bit0, 软件清0.

// HSSPI_CFG  高速SPI配置寄存器
#define SS_HOLD (0 << 4) // 高速模式时SS控制信号的HOLD时间， 0~15, 默认3. 在DMA中会增加N个系统时钟，当SPI速度为系统时钟/2时执行DMA，SS_HOLD、SS_SETUP和SS_DACT都必须设置大于2的值.
#define SS_SETUP 3		 // 高速模式时SS控制信号的SETUP时间，0~15, 默认3. 在DMA中不影响时间，       当SPI速度为系统时钟/2时执行DMA，SS_HOLD、SS_SETUP和SS_DACT都必须设置大于2的值.

// HSSPI_CFG2  高速SPI配置寄存器2
#define SPI_IOSW (1 << 6) // bit6:交换MOSI和MISO脚位，0：不交换，1：交换
#define HSSPIEN (0 << 5)  // bit5:高速SPI使能位，0：关闭高速模式，1：使能高速模式
#define FIFOEN (1 << 4)	  // bit4:高速SPI的FIFO模式使能位，0：关闭FIFO模式，1：使能FIFO模式，使能FIFO模式在DMA中减少13个系统时间。
#define SS_DACT 3		  // bit3~0:高速模式时SS控制信号的DEACTIVE时间，0~15, 默认3, 不影响DMA时间.  当SPI速度为系统时钟/2时执行DMA，SS_HOLD、SS_SETUP和SS_DACT都必须设置大于2的值.

#define DMA_SPI_ITVH (*(unsigned char volatile far *)0x7efa2e) /*  SPI_DMA时间间隔寄存器高字节 */
#define DMA_SPI_ITVL (*(unsigned char volatile far *)0x7efa2f) /*  SPI_DMA时间间隔寄存器低字节 */
void SPI_DMA_TRIG(u8 xdata *TxBuf)
{
	//@40MHz, Fosc/4, 200字节258us，100字节  130us，50字节66us，N个字节耗时 N*1.280+2 us, 51T一个字节，其中状态机19T, 传输耗时32T.
	//@40MHz, Fosc/2, 200字节177us，100字节 89.5us，50字节46us，N个字节耗时 N*0.875+2 us, 35T一个字节，其中状态机19T, 传输耗时16T.
	//@40MHz, Fosc/2, SPI DMA传输一个字节, FIFO=1, HOLD=0，耗时16+3=19T(0.475us), HOLD=3，耗时16+6=22T(0.55us).
	//@40MHz, Fosc/4, SPI DMA传输一个字节, FIFO=1, HOLD=0，耗时32+3=35T(0.875us), HOLD=3，耗时32+6=38T(0.95us).
	HSSPI_CFG = SS_HOLD | SS_SETUP;			  // SS_HOLD会增加N个系统时钟, SS_SETUP没有增加时钟。驱动OLED 40MHz时SS_HOLD可以设置为0，
	HSSPI_CFG2 = SPI_IOSW | FIFOEN | SS_DACT; // FIFOEN允许FIFO会减小13个时钟.

	SPI_DC = 1; // 写数据
	// P_LCD_CS  = 0;	//片选
	B_SPI_DMA_busy = 1; // 标志SPI-DMA忙，SPI DMA中断中清除此标志，使用SPI DMA前要确认此标志为0

	SPI_TxAddr = (u16)TxBuf;			   // 要发送数据的首地址
	DMA_SPI_TXAH = (u8)(SPI_TxAddr >> 8);  // 发送地址寄存器高字节
	DMA_SPI_TXAL = (u8)SPI_TxAddr;		   // 发送地址寄存器低字节
	DMA_SPI_AMTH = (u8)((3200 - 1) / 256); // 设置传输总字节数(高8位),	设置传输总字节数 = N+1
	DMA_SPI_AMT = (u8)(3200 - 1);		   // 设置传输总字节数(低8位).
	DMA_SPI_ITVH = 0;
	DMA_SPI_ITVL = 0;
	DMA_SPI_STA = 0x00;
	DMA_SPI_CFG = DMA_SPIIE | SPI_ACT_TX | SPI_ACT_RX | DMA_SPIIP | DMA_SPIPTY;
	DMA_SPI_CFG2 = SPI_WRPSS | SPI_SSS;
	DMA_SPI_CR = DMA_ENSPI | SPI_TRIG_M | SPI_TRIG_S | SPI_CLRFIFO; // 启动SPI DMA发送命令
}

//========================================================================
// 函数: void SPI_DMA_ISR (void) interrupt DMA_SPI_VECTOR
// 描述:  SPI_DMA中断函数.
// 参数: none.
// 返回: none.
// 版本: V1.0, 2024-1-5
//========================================================================
void SPI_DMA_ISR(void)
#ifdef __MODEL__
	interrupt DMA_SPI_VECTOR
#endif
{
	DMA_SPI_CR = 0;		  // 关闭SPI DMA
	B_SPI_DMA_busy = 0;	  // 清除SPI-DMA忙标志，SPI DMA中断中清除此标志，使用SPI DMA前要确认此标志为0
	SPSTAT = 0x80 + 0x40; // 清0 SPIF和WCOL标志
	HSSPI_CFG2 = SS_DACT; // 使用SPI查询或中断方式时，要禁止FIFO
	// P_LCD_CS = 1;
	DMA_SPI_STA = 0; // 清除中断标志
}

//========================================================================
// 函数: void  SPI_Config(u8 SPI_io, u8 SPI_speed)
// 描述: SPI初始化函数。
// 参数: io: 切换到的IO,            SS  MOSI MISO SCLK
//                       0: 切换到 P1.4 P1.5 P1.6 P1.7
//                       1: 切换到 P2.4 P2.5 P2.6 P2.7
//                       2: 切换到 P4.0 P4.1 P4.2 P4.3
//                       3: 切换到 P3.5 P3.4 P3.3 P3.2
//       SPI_speed: SPI的速度, 0: fosc/4,  1: fosc/8,  2: fosc/16,  3: fosc/2
// 返回: none.
// 版本: VER1.0
// 日期: 2024-8-13
// 备注:
//========================================================================
void SPI_Config(u8 SPI_io, u8 SPI_speed)
{
	SPI_io &= 3;

	SPCTL = SPI_speed & 3; // 配置SPI 速度, 这条指令先执行, 顺便Bit7~Bit2清0
	SSIG = 1;			   // 1: 忽略SS脚，由MSTR位决定主机还是从机		0: SS脚用于决定主机还是从机。
	SPEN = 1;			   // 1: 允许SPI，								0：禁止SPI，所有SPI管脚均为普通IO
	DORD = 0;			   // 1：LSB先发，								0：MSB先发
	MSTR = 1;			   // 1：设为主机								0：设为从机
	CPOL = 1;			   // 1: 空闲时SCLK为高电平，					0：空闲时SCLK为低电平
	CPHA = 1;			   // 1: 数据在SCLK前沿驱动,后沿采样.			0: 数据在SCLK前沿采样,后沿驱动.
	//	SPR1 = 0;	//SPR1,SPR0   00: fosc/4,     01: fosc/8
	//	SPR0 = 0;	//            10: fosc/16,    11: fosc/2
	P_SW1 = (P_SW1 & ~0x0c) | ((SPI_io << 2) & 0x0c); // 切换IO

	HSCLKDIV = 1;		  // HSCLKDIV主时钟分频
	SPI_CLKDIV = 1;		  // SPI_CLKDIV主时钟分频
	SPSTAT = 0x80 + 0x40; // 清0 SPIF和WCOL标志
}

void SoftSPI_WriteByte(u8 Byte)
{
	SPDAT = Byte; // 发送一个字节
	while (SPIF == 0)
		;				  // 等待发送完成
	SPSTAT = 0x80 + 0x40; // 清0 SPIF和WCOL标志
}

// 写寄存器函数
void LCD_WR_REG(u8 REG)
{
	SPI_DC = 0; // RS=0 命令
	SoftSPI_WriteByte(REG);
}
// 写数据函数
void LCD_WR_DATA(u8 DATA)
{
	SPI_DC = 1; // RS=1 数据
	SoftSPI_WriteByte(DATA);
}

// 写寄存器
// LCD_Reg:寄存器编号
// LCD_RegValue:要写入的值
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
{
	LCD_WR_REG(LCD_Reg);
	LCD_WriteRAM(LCD_RegValue);
}

// 开始写GRAM
void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
}
// LCD写GRAM
// RGB_Code:颜色值
void LCD_WriteRAM(u16 RGB_Code)
{
	SPI_DC = 1;
	SoftSPI_WriteByte((u8)(RGB_Code >> 8));
	SoftSPI_WriteByte((u8)(RGB_Code & 0xff));
}

// 当mdk -O1时间优化时需要设置
// 延时i
void opt_delay(u8 i)
{
	while (i--)
		;
}

// LCD开启显示
void LCD_DisplayOn(void)
{
	LCD_WR_REG(0X29); // 开启显示
}
// LCD关闭显示
void LCD_DisplayOff(void)
{
	LCD_WR_REG(0X28); // 关闭显示
}
// 设置光标位置
// Xpos:横坐标
// Ypos:纵坐标
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(Xpos >> 8);
	LCD_WR_DATA(Xpos & 0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(Ypos >> 8);
	LCD_WR_DATA(Ypos & 0XFF);
}

// 画点
// x,y:坐标
// POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x, u16 y)
{
	LCD_SetCursor(x, y);	// 设置光标位置
	LCD_WriteRAM_Prepare(); // 开始写入GRAM
	LCD_WriteRAM(POINT_COLOR);
}
// 快速画点
// x,y:坐标
// color:颜色
void LCD_Fast_DrawPoint(u16 x, u16 y, u16 color)
{
	// 设置光标位置
	LCD_SetCursor(x, y);
	// 写入颜色
	LCD_WriteReg(lcddev.wramcmd, color);
}

// dir:方向选择 	0-0度旋转，1-180度旋转，2-270度旋转，3-90度旋转
void LCD_Display_Dir(u8 dir)
{
	if (dir == 0 || dir == 1) // 竖屏
	{
		lcddev.dir = 0; // 竖屏
		lcddev.width = 240;
		lcddev.height = 320;

		lcddev.wramcmd = 0X2C;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;

		if (dir == 0) // 0-0度旋转
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (0 << 7) | (0 << 6) | (0 << 5));
		}
		else // 1-180度旋转
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (1 << 7) | (1 << 6) | (0 << 5));
		}
	}
	else if (dir == 2 || dir == 3)
	{

		lcddev.dir = 1; // 横屏
		lcddev.width = 320;
		lcddev.height = 240;

		lcddev.wramcmd = 0X2C;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;

		if (dir == 2) // 2-270度旋转
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (1 << 7) | (0 << 6) | (1 << 5));
		}
		else // 3-90度旋转
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (0 << 7) | (1 << 6) | (1 << 5));
		}
	}

	// 设置显示区域
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(0);
	LCD_WR_DATA(0);
	LCD_WR_DATA((lcddev.width - 1) >> 8);
	LCD_WR_DATA((lcddev.width - 1) & 0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(0);
	LCD_WR_DATA(0);
	LCD_WR_DATA((lcddev.height - 1) >> 8);
	LCD_WR_DATA((lcddev.height - 1) & 0XFF);
}
// 设置窗口,并自动设置画点坐标到窗口左上角(sx,sy).
// sx,sy:窗口起始坐标(左上角)
// width,height:窗口宽度和高度,必须大于0!!
// 窗体大小:width*height.
void LCD_Set_Window(u16 sx, u16 sy, u16 width, u16 height)
{
	u16 twidth, theight;
	twidth = sx + width - 1;
	theight = sy + height - 1;

	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(sx >> 8);
	LCD_WR_DATA(sx & 0XFF);
	LCD_WR_DATA(twidth >> 8);
	LCD_WR_DATA(twidth & 0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(sy >> 8);
	LCD_WR_DATA(sy & 0XFF);
	LCD_WR_DATA(theight >> 8);
	LCD_WR_DATA(theight & 0XFF);
}

// 初始化lcd
void LCD_Init(void)
{ // P2.5 P2.7 准双向，上拉，强驱动电流，快转换速度
	P2M0 &= ~0xa0;
	P2M1 &= ~0xa0;
	P2PU |= 0xa0;
	P2SR &= ~0xa0;
	P2DR &= ~0xa0;
	// P0.5 准双向
	P0M0 &= ~0x20;
	P0M1 &= ~0x20;
	// P5.1 准双向
	P5M0 &= ~0x02;
	P5M1 &= ~0x02;

	SPI_Config(1, 3);

	SPI_RST = 1;
	delay_ms(10);
	SPI_RST = 0;
	delay_ms(10);
	SPI_RST = 1;
	delay_ms(120);

	//************* Start Initial Sequence **********//
	LCD_WR_REG(0x11);
	delay_ms(120); // Delay 120ms
	//--------display and color format setting-----------//
	LCD_WR_REG(0x36);
	LCD_WR_DATA(0x00);
	LCD_WR_REG(0x3a);
	LCD_WR_DATA(0x05);
	//--------ST7789V Frame rate setting------------//
	LCD_WR_REG(0xb2);
	LCD_WR_DATA(0x0c);
	LCD_WR_DATA(0x0c);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x33);
	LCD_WR_DATA(0x33);
	LCD_WR_REG(0xb7);
	LCD_WR_DATA(0x35);
	//-----------ST7789V Power setting---------------//
	LCD_WR_REG(0xbb);
	LCD_WR_DATA(0x28);
	LCD_WR_REG(0xc0);
	LCD_WR_DATA(0x2c);
	LCD_WR_REG(0xc2);
	LCD_WR_DATA(0x01);
	LCD_WR_REG(0xc3);
	LCD_WR_DATA(0x0b);
	LCD_WR_REG(0xc4);
	LCD_WR_DATA(0x20);
	LCD_WR_REG(0xc6);
	LCD_WR_DATA(0x0f);
	LCD_WR_REG(0xd0);
	LCD_WR_DATA(0xa4);
	LCD_WR_DATA(0xa1);
	//------------ST7789V gamma setting-------------//
	LCD_WR_REG(0xe0);
	LCD_WR_DATA(0xd0);
	LCD_WR_DATA(0x01);
	LCD_WR_DATA(0x08);
	LCD_WR_DATA(0x0f);
	LCD_WR_DATA(0x11);
	LCD_WR_DATA(0x2a);
	LCD_WR_DATA(0x36);
	LCD_WR_DATA(0x55);
	LCD_WR_DATA(0x44);
	LCD_WR_DATA(0x3a);
	LCD_WR_DATA(0x0b);
	LCD_WR_DATA(0x06);
	LCD_WR_DATA(0x11);
	LCD_WR_DATA(0x20);
	LCD_WR_REG(0xe1);
	LCD_WR_DATA(0xd0);
	LCD_WR_DATA(0x02);
	LCD_WR_DATA(0x07);
	LCD_WR_DATA(0x0a);
	LCD_WR_DATA(0x0b);
	LCD_WR_DATA(0x18);
	LCD_WR_DATA(0x34);
	LCD_WR_DATA(0x43);
	LCD_WR_DATA(0x4a);
	LCD_WR_DATA(0x2b);
	LCD_WR_DATA(0x1b);
	LCD_WR_DATA(0x1c);
	LCD_WR_DATA(0x22);
	LCD_WR_DATA(0x1f);
	LCD_WR_REG(0x29);
}

// //读取个某点的颜色值
// //x,y:坐标
// //返回值:此点的颜色
// u16 LCD_ReadPoint(u16 x,u16 y)
// {
// u8 i,r,g,b,reg=0x2e;
//  	u16 color;
// 	if(x>=lcddev.width||y>=lcddev.height)return 0;	//超过了范围,直接返回
// 	LCD_SetCursor(x,y);
// 	SPI_CS = 0;
// 	SPI_DC = 0;

// 		for(i=0; i<8; i++)
// 		{
// 			if (reg & 0x80)
// 			 SPI_SDI = 1;
// 			else
// 			 SPI_SDI = 0;

// 			reg <<= 1;
// 			SPI_SCK = 0;
// 			SPI_SCK = 1;
// 		}

// 		for(i=0; i<8; i++)							//第一次空读 后三次分别为R G B
// 		{
// 			SPI_SCK = 0;
// 			SPI_SCK = 1;
// 		}

// 		for(i=0; i<8; i++)
// 		{
// 			SPI_SCK = 0;		r=r << 1 | SPI_SDO;
// 			SPI_SCK = 1;
// 		}

// 		for(i=0; i<8; i++)
// 		{
// 			SPI_SCK = 0;		g=g << 1 | SPI_SDO;
// 			SPI_SCK = 1;
// 		}

// 		for(i=0; i<8; i++)
// 		{
// 			SPI_SCK = 0;		b=b << 1 | SPI_SDO;
// 			SPI_SCK = 1;
// 		}

// 		color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

// 		SPI_CS=1;

// return color;
// }

// 清屏函数
// color:要清屏的填充色
void LCD_Clear(u16 color)
{
	u32 index = 0;
	u32 totalpoint;

	totalpoint = lcddev.width;
	totalpoint *= lcddev.height; // 得到总点数
	LCD_SetCursor(0, 0);		 // 设置光标位置
	LCD_WriteRAM_Prepare();		 // 开始写入GRAM
	SPI_DC = 1;
	for (index = 0; index < totalpoint / 1600; index++)
	{
		SPI_DMA_TRIG(DisTmp); // 触发SPI DAM发送一个图片
		while (B_SPI_DMA_busy)
			; // 等待图片发送完毕
	}
}
// 在指定区域内填充指定颜色
// 区域大小:(xend-xsta+1)*(yend-ysta+1)
// xsta
// color:要填充的颜色
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
	u16 i, j;
	u16 xlen = 0;

	xlen = ex - sx + 1;
	for (i = sy; i <= ey; i++)
	{
		LCD_SetCursor(sx, i);	// 设置光标位置
		LCD_WriteRAM_Prepare(); // 开始写入GRAM
		for (j = 0; j < xlen; j++)
			LCD_WriteRAM(color); // 设置光标位置
	}
}

// 画线
// x1,y1:起点坐标
// x2,y2:终点坐标
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1; // 计算坐标增量
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)
		incx = 1; // 设置单步方向
	else if (delta_x == 0)
		incx = 0; // 垂直线
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0; // 水平线
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x; // 选取基本增量坐标轴
	else
		distance = delta_y;
	for (t = 0; t <= distance + 1; t++) // 画线输出
	{
		LCD_DrawPoint(uRow, uCol); // 画点
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}
// 画矩形
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}
// 在指定位置画一个指定大小的圆
//(x,y):中心点
// r    :半径
void LCD_Draw_Circle(u16 x0, u16 y0, u8 r)
{
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1); // 判断下个点位置的标志
	while (a <= b)
	{
		LCD_DrawPoint(x0 + a, y0 - b); // 5
		LCD_DrawPoint(x0 + b, y0 - a); // 0
		LCD_DrawPoint(x0 + b, y0 + a); // 4
		LCD_DrawPoint(x0 + a, y0 + b); // 6
		LCD_DrawPoint(x0 - a, y0 + b); // 1
		LCD_DrawPoint(x0 - b, y0 + a);
		LCD_DrawPoint(x0 - a, y0 - b); // 2
		LCD_DrawPoint(x0 - b, y0 - a); // 7
		a++;
		// 使用Bresenham算法画圆
		if (di < 0)
			di += 4 * a + 6;
		else
		{
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}
// 在指定位置显示一个字符
// x,y:起始坐标
// num:要显示的字符:" "--->"~"
// size:字体大小 12/16/24
// mode:叠加方式(1)还是非叠加方式(0)
void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t1, t;
	u16 y0 = y;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); // 得到字体一个字符对应点阵集所占的字节数
	num = num - ' ';										   // 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
	for (t = 0; t < csize; t++)
	{
		if (size == 12)
			temp = asc2_1206[num][t]; // 调用1206字体
		else if (size == 16)
			temp = asc2_1608[num][t]; // 调用1608字体
		else if (size == 24)
			temp = asc2_2412[num][t]; // 调用2412字体
		else
			return; // 没有的字库
		for (t1 = 0; t1 < 8; t1++)
		{
			if (temp & 0x80)
				LCD_Fast_DrawPoint(x, y, POINT_COLOR);
			else if (mode == 0)
				LCD_Fast_DrawPoint(x, y, BACK_COLOR);
			temp <<= 1;
			y++;
			if (y >= lcddev.height)
				return; // 超区域了
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				if (x >= lcddev.width)
					return; // 超区域了
				break;
			}
		}
	}
}
// m^n函数
// 返回值:m^n次方.
u32 LCD_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--)
		result *= m;
	return result;
}
// 显示数字,高位为0,则不显示
// x,y :起点坐标
// len :数字的位数
// size:字体大小
// color:颜色
// num:数值(0~4294967295);
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size)
{
	u8 t, temp;
	u8 enshow = 0;
	for (t = 0; t < len; t++)
	{
		temp = (num / LCD_Pow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1))
		{
			if (temp == 0)
			{
				LCD_ShowChar(x + (size / 2) * t, y, ' ', size, 0);
				continue;
			}
			else
				enshow = 1;
		}
		LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, 0);
	}
}
////显示数字,高位为0,还是显示
////x,y:起点坐标
////num:数值(0~999999999);
////len:长度(即要显示的位数)
////size:字体大小
////mode:
////[7]:0,不填充;1,填充0.
////[6:1]:保留
////[0]:0,非叠加显示;1,叠加显示.
// void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
//{
//	u8 t,temp;
//	u8 enshow=0;
//	for(t=0;t<len;t++)
//	{
//		temp=(num/LCD_Pow(10,len-t-1))%10;
//		if(enshow==0&&t<(len-1))
//		{
//			if(temp==0)
//			{
//				if(mode&0X80)LCD_ShowChar(x+(size/2)*t,y,'0',size,mode&0X01);
//				else LCD_ShowChar(x+(size/2)*t,y,' ',size,mode&0X01);
//  				continue;
//			}else enshow=1;
//
//		}
//	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,mode&0X01);
//	}
// }
// 显示字符串
// x,y:起点坐标
// width,height:区域大小
// size:字体大小
//*p:字符串起始地址
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p)
{
	u8 x0 = x;
	width += x;
	height += y;
	while ((*p <= '~') && (*p >= ' ')) // 判断是不是非法字符!
	{
		if (x >= width)
		{
			x = x0;
			y += size;
		}
		if (y >= height)
			break; // 退出
		LCD_ShowChar(x, y, *p, size, 0);
		x += size / 2;
		p++;
	}
}

// 汉字 16*16
void GUI_DrawFont16(u16 x, u16 y, u8 *s, u8 mode)
{
	u8 i, j;
	u16 k;
	u16 HZnum;
	u16 x0 = x;
	HZnum = sizeof(tfont16) / sizeof(typFNT_GB16); // 自动统计汉字数目

	for (k = 0; k < HZnum; k++)
	{
		if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s + 1)))
		{
			LCD_Set_Window(x, y, 16, 16);
			LCD_WriteRAM_Prepare();
			for (i = 0; i < 16 * 2; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!mode) // 非叠加方式
					{
						if (tfont16[k].Msk[i] & (0x80 >> j))
							LCD_WriteRAM(POINT_COLOR);
						else
							LCD_WriteRAM(BACK_COLOR);
					}
					else
					{
						// POINT_COLOR=fc;
						if (tfont16[k].Msk[i] & (0x80 >> j))
							LCD_DrawPoint(x, y); // 画一个点
						x++;
						if ((x - x0) == 16)
						{
							x = x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue; // 查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}

	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // 恢复窗口为全屏
}

// 汉字 24*24
void GUI_DrawFont24(u16 x, u16 y, u8 *s, u8 mode)
{
	u8 i, j;
	u16 k;
	u16 HZnum;
	u16 x0 = x;
	HZnum = sizeof(tfont24) / sizeof(typFNT_GB24); // 自动统计汉字数目

	for (k = 0; k < HZnum; k++)
	{
		if ((tfont24[k].Index[0] == *(s)) && (tfont24[k].Index[1] == *(s + 1)))
		{
			LCD_Set_Window(x, y, 24, 24);
			LCD_WriteRAM_Prepare();
			for (i = 0; i < 24 * 3; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!mode) // 非叠加方式
					{
						if (tfont24[k].Msk[i] & (0x80 >> j))
							LCD_WriteRAM(POINT_COLOR);
						else
							LCD_WriteRAM(BACK_COLOR);
					}
					else
					{
						// POINT_COLOR=fc;
						if (tfont24[k].Msk[i] & (0x80 >> j))
							LCD_DrawPoint(x, y); // 画一个点
						x++;
						if ((x - x0) == 24)
						{
							x = x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue; // 查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}

	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // 恢复窗口为全屏
}

// 汉字 32*32
void GUI_DrawFont32(u16 x, u16 y, u8 *s, u8 mode)
{
	u8 i, j;
	u16 k;
	u16 HZnum;
	u16 x0 = x;
	HZnum = sizeof(tfont32) / sizeof(typFNT_GB32); // 自动统计汉字数目
	for (k = 0; k < HZnum; k++)
	{
		if ((tfont32[k].Index[0] == *(s)) && (tfont32[k].Index[1] == *(s + 1)))
		{
			LCD_Set_Window(x, y, 32, 32);
			LCD_WriteRAM_Prepare();
			for (i = 0; i < 32 * 4; i++)
			{
				for (j = 0; j < 8; j++)
				{
					if (!mode) // 非叠加方式
					{
						if (tfont32[k].Msk[i] & (0x80 >> j))
							LCD_WriteRAM(POINT_COLOR);
						else
							LCD_WriteRAM(BACK_COLOR);
					}
					else
					{
						// POINT_COLOR=fc;
						if (tfont32[k].Msk[i] & (0x80 >> j))
							LCD_DrawPoint(x, y); // 画一个点
						x++;
						if ((x - x0) == 32)
						{
							x = x0;
							y++;
							break;
						}
					}
				}
			}
		}
		continue; // 查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}

	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // 恢复窗口为全屏
}

// 显示汉字或者字符串
void Show_Str(u16 x, u16 y, u8 *str, u8 size, u8 mode)
{
	u16 x0 = x;
	u8 bHz = 0;		  // 字符或者中文
	while (*str != 0) // 数据未结束
	{
		if (!bHz)
		{
			if (x > (lcddev.width - size / 2) || y > (lcddev.height - size))
				return;
			if (*str > 0x80)
				bHz = 1; // 中文
			else		 // 字符
			{
				if (*str == 0x0D) // 换行符号
				{
					y += size;
					x = x0;
					str++;
				}
				else
				{
					if (size >= 24) // 字库中没有集成12X24 16X32的英文字体,用8X16代替
					{
						LCD_ShowChar(x, y, *str, 24, mode);
						x += 12; // 字符,为全字的一半
					}
					else
					{
						LCD_ShowChar(x, y, *str, size, mode);
						x += size / 2; // 字符,为全字的一半
					}
				}
				str++;
			}
		}
		else // 中文
		{
			if (x > (lcddev.width - size) || y > (lcddev.height - size))
				return;
			bHz = 0; // 有汉字库
			if (size == 32)
				GUI_DrawFont32(x, y, str, mode);
			else if (size == 24)
				GUI_DrawFont24(x, y, str, mode);
			else
				GUI_DrawFont16(x, y, str, mode);

			str += 2;
			x += size; // 下一个汉字偏移
		}
	}
}

// 显示40*40图片
void Gui_Drawbmp16(u16 x, u16 y, const unsigned char *p) // 显示40*40图片
{
	int i;
	unsigned char picH, picL;
	LCD_Set_Window(x, y, 40, 40);
	LCD_WriteRAM_Prepare();

	for (i = 0; i < 40 * 40; i++)
	{
		picL = *(p + i * 2); // 数据低位在前
		picH = *(p + i * 2 + 1);
		LCD_WriteRAM(picH << 8 | picL);
	}
	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // 恢复显示窗口为全屏
}

// 居中显示
void Gui_StrCenter(u16 x, u16 y, u8 *str, u8 size, u8 mode)
{
	u16 x1;
	u16 len = strlen((const char *)str);
	if (size > 16)
	{
		x1 = (lcddev.width - len * (size / 2)) / 2;
	}
	else
	{
		x1 = (lcddev.width - len * 8) / 2;
	}

	Show_Str(x + x1, y, str, size, mode);
}

void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);	// 清屏
	POINT_COLOR = BLUE; // 设置字体为蓝色
	BACK_COLOR = WHITE;
	LCD_ShowString(lcddev.width - 24, 0, 200, 16, 16, "RST"); // 显示清屏区域
	POINT_COLOR = RED;										  // 设置画笔蓝色
}
////////////////////////////////////////////////////////////////////////////////
// 电容触摸屏专有部分
// 画水平线
// x0,y0:坐标
// len:线长度
// color:颜色
void gui_draw_hline(u16 x0, u16 y0, u16 len, u16 color)
{
	if (len == 0)
		return;
	LCD_Fill(x0, y0, x0 + len - 1, y0, color);
}
// 画实心圆
// x0,y0:坐标
// r:半径
// color:颜色
void gui_fill_circle(u16 x0, u16 y0, u16 r, u16 color)
{
	u32 i;
	u32 imax = ((u32)r * 707) / 1000 + 1;
	u32 sqmax = (u32)r * (u32)r + (u32)r / 2;
	u32 x = r;
	gui_draw_hline(x0 - r, y0, 2 * r, color);
	for (i = 1; i <= imax; i++)
	{
		if ((i * i + x * x) > sqmax) // draw lines from outside
		{
			if (x > imax)
			{
				gui_draw_hline(x0 - i + 1, y0 + x, 2 * (i - 1), color);
				gui_draw_hline(x0 - i + 1, y0 - x, 2 * (i - 1), color);
			}
			x--;
		}
		// draw lines from inside (center)
		gui_draw_hline(x0 - x, y0 + i, 2 * x, color);
		gui_draw_hline(x0 - x, y0 - i, 2 * x, color);
	}
}

// 画一条粗线
//(x1,y1),(x2,y2):线条的起始坐标
// size：线条的粗细程度
// color：线条的颜色
void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2, u8 size, u16 color)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	if (x1 < size || x2 < size || y1 < size || y2 < size)
		return;
	delta_x = x2 - x1; // 计算坐标增量
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)
		incx = 1; // 设置单步方向
	else if (delta_x == 0)
		incx = 0; // 垂直线
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0; // 水平线
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x; // 选取基本增量坐标轴
	else
		distance = delta_y;
	for (t = 0; t <= distance + 1; t++) // 画线输出
	{
		gui_fill_circle(uRow, uCol, size, color); // 画点
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance)
		{
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance)
		{
			yerr -= distance;
			uCol += incy;
		}
	}
}

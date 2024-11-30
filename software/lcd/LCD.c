#include "lcd.h"
#include "stdlib.h"
#include "font.h"
#include "string.h"

// LCD�Ļ�����ɫ�ͱ���ɫ
u16 POINT_COLOR = 0x0000; // ������ɫ
u16 BACK_COLOR = 0xFFFF;  // ����ɫ

// ����LCD��Ҫ����
// Ĭ��Ϊ����
_lcd_dev lcddev;

//-----------------��������----------------
bit B_SPI_DMA_busy;	   // SPI DMAæ��־�� 1��־SPI-DMAæ��SPI DMA�ж�������˱�־��ʹ��SPI DMAǰҪȷ�ϴ˱�־Ϊ0
u16 SPI_TxAddr;		   // SPI DMAҪ�������ݵ��׵�ַ
u8 xdata DisTmp[3200]; // ��ʾ���壬��Ҫ��ʾ�����ݷ����Դ������DMA����. ����LCM DMA��4�ֽڶ������⣬�������ﶨλ�Ե�ַΪ4�ı���

// DMA_SPI_CR 	SPI_DMA���ƼĴ���
#define DMA_ENSPI (1 << 7)	// SPI DMA����ʹ�ܿ���λ��    bit7, 0:��ֹSPI DMA���ܣ�  1������SPI DMA���ܡ�
#define SPI_TRIG_M (1 << 6) // SPI DMA����ģʽ��������λ��bit6, 0:д0��Ч��          1��д1��ʼSPI DMA����ģʽ������
#define SPI_TRIG_S (0 << 5) // SPI DMA�ӻ�ģʽ��������λ��bit5, 0:д0��Ч��          1��д1��ʼSPI DMA�ӻ�ģʽ������
#define SPI_CLRFIFO 1		// ���SPI DMA����FIFO����λ��bit0, 0:д0��Ч��          1��д1��λFIFOָ�롣

// DMA_SPI_CFG 	SPI_DMA���üĴ���
#define DMA_SPIIE (1 << 7)	// SPI DMA�ж�ʹ�ܿ���λ��bit7, 0:��ֹSPI DMA�жϣ�     1�������жϡ�
#define SPI_ACT_TX (1 << 6) // SPI DMA�������ݿ���λ��bit6, 0:��ֹSPI DMA�������ݣ�����ֻ��ʱ�Ӳ������ݣ��ӻ�Ҳ����. 1�������͡�
#define SPI_ACT_RX (0 << 5) // SPI DMA�������ݿ���λ��bit5, 0:��ֹSPI DMA�������ݣ�����ֻ��ʱ�Ӳ������ݣ��ӻ�Ҳ����. 1��������ա�
#define DMA_SPIIP (0 << 2)	// SPI DMA�ж����ȼ�����λ��bit3~bit2, (���)0~3(���).
#define DMA_SPIPTY 0		// SPI DMA�������߷������ȼ�����λ��bit1~bit0, (���)0~3(���).

// DMA_SPI_CFG2 	SPI_DMA���üĴ���2
#define SPI_WRPSS (0 << 2) // SPI DMA������ʹ��SS�ſ���λ��bit2, 0: SPI DMA������̲��Զ�����SS�š�  1���Զ�����SS�š�
#define SPI_SSS 3		   // SPI DMA�������Զ�����SS��ѡ��λ��bit1~bit0, 0: P1.4,  1��P2.4,  2: P4.0,  3:P3.5��

// DMA_SPI_STA 	SPI_DMA״̬�Ĵ���
#define SPI_TXOVW (1 << 2)	// SPI DMA���ݸ��Ǳ�־λ��bit2, �����0.
#define SPI_RXLOSS (1 << 1) // SPI DMA�������ݶ�����־λ��bit1, �����0.
#define DMA_SPIIF 1			// SPI DMA�ж������־λ��bit0, �����0.

// HSSPI_CFG  ����SPI���üĴ���
#define SS_HOLD (0 << 4) // ����ģʽʱSS�����źŵ�HOLDʱ�䣬 0~15, Ĭ��3. ��DMA�л�����N��ϵͳʱ�ӣ���SPI�ٶ�Ϊϵͳʱ��/2ʱִ��DMA��SS_HOLD��SS_SETUP��SS_DACT���������ô���2��ֵ.
#define SS_SETUP 3		 // ����ģʽʱSS�����źŵ�SETUPʱ�䣬0~15, Ĭ��3. ��DMA�в�Ӱ��ʱ�䣬       ��SPI�ٶ�Ϊϵͳʱ��/2ʱִ��DMA��SS_HOLD��SS_SETUP��SS_DACT���������ô���2��ֵ.

// HSSPI_CFG2  ����SPI���üĴ���2
#define SPI_IOSW (1 << 6) // bit6:����MOSI��MISO��λ��0����������1������
#define HSSPIEN (0 << 5)  // bit5:����SPIʹ��λ��0���رո���ģʽ��1��ʹ�ܸ���ģʽ
#define FIFOEN (1 << 4)	  // bit4:����SPI��FIFOģʽʹ��λ��0���ر�FIFOģʽ��1��ʹ��FIFOģʽ��ʹ��FIFOģʽ��DMA�м���13��ϵͳʱ�䡣
#define SS_DACT 3		  // bit3~0:����ģʽʱSS�����źŵ�DEACTIVEʱ�䣬0~15, Ĭ��3, ��Ӱ��DMAʱ��.  ��SPI�ٶ�Ϊϵͳʱ��/2ʱִ��DMA��SS_HOLD��SS_SETUP��SS_DACT���������ô���2��ֵ.

// #define DMA_SPI_ITVH (*(unsigned char volatile far *)0x7efa2e) /*  SPI_DMAʱ�����Ĵ������ֽ� */
// #define DMA_SPI_ITVL (*(unsigned char volatile far *)0x7efa2f) /*  SPI_DMAʱ�����Ĵ������ֽ� */
void SPI_DMA_TRIG(u8 xdata *TxBuf, u16 size)
{
	//@40MHz, Fosc/4, 200�ֽ�258us��100�ֽ�  130us��50�ֽ�66us��N���ֽں�ʱ N*1.280+2 us, 51Tһ���ֽڣ�����״̬��19T, �����ʱ32T.
	//@40MHz, Fosc/2, 200�ֽ�177us��100�ֽ� 89.5us��50�ֽ�46us��N���ֽں�ʱ N*0.875+2 us, 35Tһ���ֽڣ�����״̬��19T, �����ʱ16T.
	//@40MHz, Fosc/2, SPI DMA����һ���ֽ�, FIFO=1, HOLD=0����ʱ16+3=19T(0.475us), HOLD=3����ʱ16+6=22T(0.55us).
	//@40MHz, Fosc/4, SPI DMA����һ���ֽ�, FIFO=1, HOLD=0����ʱ32+3=35T(0.875us), HOLD=3����ʱ32+6=38T(0.95us).
	HSSPI_CFG = SS_HOLD | SS_SETUP; // SS_HOLD������N��ϵͳʱ��, SS_SETUPû������ʱ�ӡ�����OLED 40MHzʱSS_HOLD��������Ϊ0��
	HSSPI_CFG2 = FIFOEN | SS_DACT;	// FIFOEN����FIFO���С13��ʱ��.

	SPI_DC = 1; // д����
	// P_LCD_CS  = 0;	//Ƭѡ
	B_SPI_DMA_busy = 1; // ��־SPI-DMAæ��SPI DMA�ж�������˱�־��ʹ��SPI DMAǰҪȷ�ϴ˱�־Ϊ0

	SPI_TxAddr = (u16)TxBuf;			   // Ҫ�������ݵ��׵�ַ
	DMA_SPI_TXAH = (u8)(SPI_TxAddr >> 8);  // ���͵�ַ�Ĵ������ֽ�
	DMA_SPI_TXAL = (u8)SPI_TxAddr;		   // ���͵�ַ�Ĵ������ֽ�
	DMA_SPI_AMTH = (u8)((size - 1) / 256); // ���ô������ֽ���(��8λ),	���ô������ֽ��� = N+1
	DMA_SPI_AMT = (u8)(size - 1);		   // ���ô������ֽ���(��8λ).
	DMA_SPI_ITVH = 0;
	DMA_SPI_ITVL = 0;
	DMA_SPI_STA = 0x00;
	DMA_SPI_CFG = DMA_SPIIE | SPI_ACT_TX | SPI_ACT_RX | DMA_SPIIP | DMA_SPIPTY;
	DMA_SPI_CFG2 = SPI_WRPSS | SPI_SSS;
	DMA_SPI_CR = DMA_ENSPI | SPI_TRIG_M | SPI_TRIG_S | SPI_CLRFIFO; // ����SPI DMA��������
}

//========================================================================
// ����: void SPI_DMA_ISR (void) interrupt DMA_SPI_VECTOR
// ����:  SPI_DMA�жϺ���.
// ����: none.
// ����: none.
// �汾: V1.0, 2024-1-5
//========================================================================
void SPI_DMA_ISR(void)
#ifdef __MODEL__
	interrupt DMA_SPI_VECTOR
#endif
{
	DMA_SPI_CR = 0;		  // �ر�SPI DMA
	B_SPI_DMA_busy = 0;	  // ���SPI-DMAæ��־��SPI DMA�ж�������˱�־��ʹ��SPI DMAǰҪȷ�ϴ˱�־Ϊ0
	SPSTAT = 0x80 + 0x40; // ��0 SPIF��WCOL��־
	HSSPI_CFG2 = SS_DACT; // ʹ��SPI��ѯ���жϷ�ʽʱ��Ҫ��ֹFIFO
	// P_LCD_CS = 1;
	DMA_SPI_STA = 0; // ����жϱ�־
}

//========================================================================
// ����: void  SPI_Config(u8 SPI_io, u8 SPI_speed)
// ����: SPI��ʼ��������
// ����: io: �л�����IO,            SS  MOSI MISO SCLK
//                       0: �л��� P1.4 P1.5 P1.6 P1.7
//                       1: �л��� P2.4 P2.5 P2.6 P2.7
//                       2: �л��� P4.0 P4.1 P4.2 P4.3
//                       3: �л��� P3.5 P3.4 P3.3 P3.2
//       SPI_speed: SPI���ٶ�, 0: fosc/4,  1: fosc/8,  2: fosc/16,  3: fosc/2
// ����: none.
// �汾: VER1.0
// ����: 2024-8-13
// ��ע:
//========================================================================
void SPI_Config(u8 SPI_io, u8 SPI_speed)
{
	SPI_io &= 3;

	SPCTL = SPI_speed & 3; // ����SPI �ٶ�, ����ָ����ִ��, ˳��Bit7~Bit2��0
	SPCTL |= SSIG;	//1: ����SS�ţ���MSTRλ�����������Ǵӻ�		0: SS�����ھ����������Ǵӻ���
	SPCTL |= SPEN;	//1: ����SPI��								0����ֹSPI������SPI�ܽž�Ϊ��ͨIO
	SPCTL &= ~DORD;	//1��LSB�ȷ���								0��MSB�ȷ�
	SPCTL |= MSTR;	//1����Ϊ����								0����Ϊ�ӻ�
	SPCTL |= CPOL;	//1: ����ʱSCLKΪ�ߵ�ƽ��					0������ʱSCLKΪ�͵�ƽ
	SPCTL |= CPHA;	//1: ������SCLKǰ������,���ز���.			0: ������SCLKǰ�ز���,��������.
	//	SPR1 = 0;	//SPR1,SPR0   00: fosc/4,     01: fosc/8
	//	SPR0 = 0;	//            10: fosc/16,    11: fosc/2
	P_SW1 = (P_SW1 & ~0x0c) | ((SPI_io << 2) & 0x0c); // �л�IO

	HSCLKDIV = 1;		  // HSCLKDIV��ʱ�ӷ�Ƶ
	SPI_CLKDIV = 1;		  // SPI_CLKDIV��ʱ�ӷ�Ƶ
	SPSTAT = 0x80 + 0x40; // ��0 SPIF��WCOL��־
}

void SoftSPI_WriteByte(u8 Byte)
{
	SPDAT = Byte; // ����һ���ֽ�
	while((SPSTAT & SPIF)==0)	;			//�ȴ��������
	SPSTAT = 0x80 + 0x40; // ��0 SPIF��WCOL��־
}

// д�Ĵ�������
void LCD_WR_REG(u8 REG)
{
	SPI_DC = 0; // RS=0 ����
	SoftSPI_WriteByte(REG);
}
// д���ݺ���
void LCD_WR_DATA(u8 DATA)
{
	SPI_DC = 1; // RS=1 ����
	SoftSPI_WriteByte(DATA);
}

// д�Ĵ���
// LCD_Reg:�Ĵ������
// LCD_RegValue:Ҫд���ֵ
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
{
	LCD_WR_REG(LCD_Reg);
	LCD_WriteRAM(LCD_RegValue);
}

// ��ʼдGRAM
void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
}
// LCDдGRAM
// RGB_Code:��ɫֵ
void LCD_WriteRAM(u16 RGB_Code)
{
	SPI_DC = 1;
	SoftSPI_WriteByte((u8)(RGB_Code >> 8));
	SoftSPI_WriteByte((u8)(RGB_Code & 0xff));
}

// ��mdk -O1ʱ���Ż�ʱ��Ҫ����
// ��ʱi
void opt_delay(u8 i)
{
	while (i--)
		;
}

// LCD������ʾ
void LCD_DisplayOn(void)
{
	LCD_WR_REG(0X29); // ������ʾ
}
// LCD�ر���ʾ
void LCD_DisplayOff(void)
{
	LCD_WR_REG(0X28); // �ر���ʾ
}
// ���ù��λ��
// Xpos:������
// Ypos:������
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(Xpos >> 8);
	LCD_WR_DATA(Xpos & 0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(Ypos >> 8);
	LCD_WR_DATA(Ypos & 0XFF);
}

// ����
// x,y:����
// POINT_COLOR:�˵����ɫ
void LCD_DrawPoint(u16 x, u16 y)
{
	LCD_SetCursor(x, y);	// ���ù��λ��
	LCD_WriteRAM_Prepare(); // ��ʼд��GRAM
	LCD_WriteRAM(POINT_COLOR);
}
// ���ٻ���
// x,y:����
// color:��ɫ
void LCD_Fast_DrawPoint(u16 x, u16 y, u16 color)
{
	// ���ù��λ��
	LCD_SetCursor(x, y);
	// д����ɫ
	LCD_WriteReg(lcddev.wramcmd, color);
}

// dir:����ѡ�� 	0-0����ת��1-180����ת��2-270����ת��3-90����ת
void LCD_Display_Dir(u8 dir)
{
	if (dir == 0 || dir == 1) // ����
	{
		lcddev.dir = 0; // ����
		lcddev.width = 240;
		lcddev.height = 320;

		lcddev.wramcmd = 0X2C;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;

		if (dir == 0) // 0-0����ת
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (0 << 7) | (0 << 6) | (0 << 5));
		}
		else // 1-180����ת
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (1 << 7) | (1 << 6) | (0 << 5));
		}
	}
	else if (dir == 2 || dir == 3)
	{

		lcddev.dir = 1; // ����
		lcddev.width = 320;
		lcddev.height = 240;

		lcddev.wramcmd = 0X2C;
		lcddev.setxcmd = 0X2A;
		lcddev.setycmd = 0X2B;

		if (dir == 2) // 2-270����ת
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (1 << 7) | (0 << 6) | (1 << 5));
		}
		else // 3-90����ת
		{
			LCD_WR_REG(0x36);
			LCD_WR_DATA((0 << 3) | (0 << 7) | (1 << 6) | (1 << 5));
		}
	}

	// ������ʾ����
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
// ���ô���,���Զ����û������굽�������Ͻ�(sx,sy).
// sx,sy:������ʼ����(���Ͻ�)
// width,height:���ڿ�Ⱥ͸߶�,�������0!!
// �����С:width*height.
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

// ��ʼ��lcd
void LCD_Init(void)
{ // P2.5 P2.7 ׼˫��������ǿ������������ת���ٶ�
	P2M0 &= ~0xa0;
	P2M1 &= ~0xa0;
	P2PU |= 0xa0;
	P2SR &= ~0xa0;
	P2DR &= ~0xa0;
	// P0.5 ׼˫��
	P0M0 &= ~0x20;
	P0M1 &= ~0x20;
	// P5.1 ׼˫��
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

// //��ȡ��ĳ�����ɫֵ
// //x,y:����
// //����ֵ:�˵����ɫ
// u16 LCD_ReadPoint(u16 x,u16 y)
// {
// u8 i,r,g,b,reg=0x2e;
//  	u16 color;
// 	if(x>=lcddev.width||y>=lcddev.height)return 0;	//�����˷�Χ,ֱ�ӷ���
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

// 		for(i=0; i<8; i++)							//��һ�οն� �����ηֱ�ΪR G B
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

// ��������
// color:Ҫ���������ɫ
void LCD_Clear(u16 color)
{
	LCD_Fill_LARGE(0, 0, lcddev.width - 1, lcddev.height - 1, color);
}
// ��ָ�����������ָ����ɫ
// �����С:(xend-xsta+1)*(yend-ysta+1)
// xsta
// color:Ҫ������ɫ
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
	u16 i, j;
	u16 xlen = 0;

	xlen = ex - sx + 1;
	for (i = sy; i <= ey; i++)
	{
		LCD_SetCursor(sx, i);	// ���ù��λ��
		LCD_WriteRAM_Prepare(); // ��ʼд��GRAM
		for (j = 0; j < xlen; j++)
			LCD_WriteRAM(color); // ���ù��λ��
	}
}

// ��ָ�����������ָ����ɫ
// �����С:(xend-xsta+1)*(yend-ysta+1)
// xsta
// color:Ҫ������ɫ
void LCD_Fill_LARGE(u16 sx, u16 sy, u16 ex, u16 ey, u16 color)
{
	u32 index = 0;
	u32 w, h;
	w = ex - sx + 1;
	h = ey - sy + 1;

	LCD_Set_Window(sx, sy, w, h);
	LCD_WriteRAM_Prepare();
	for (index = 0; index < 800 / 2; index++)
	{
		((u16 *)(DisTmp))[index] = color;
	}
	for (index = 0; index < w * h * 2 / 800 + 1; index++)
	{
		SPI_DMA_TRIG(DisTmp, 800); // ����SPI DAM����һ��ͼƬ
		while (B_SPI_DMA_busy)
			; // �ȴ�ͼƬ�������
	}
}

// ����
// x1,y1:�������
// x2,y2:�յ�����
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1; // ������������
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)
		incx = 1; // ���õ�������
	else if (delta_x == 0)
		incx = 0; // ��ֱ��
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0; // ˮƽ��
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x; // ѡȡ��������������
	else
		distance = delta_y;
	for (t = 0; t <= distance + 1; t++) // �������
	{
		LCD_DrawPoint(uRow, uCol); // ����
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
// ������
//(x1,y1),(x2,y2):���εĶԽ�����
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}
// ��ָ��λ�û�һ��ָ����С��Բ
//(x,y):���ĵ�
// r    :�뾶
void LCD_Draw_Circle(u16 x0, u16 y0, u8 r)
{
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1); // �ж��¸���λ�õı�־
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
		// ʹ��Bresenham�㷨��Բ
		if (di < 0)
			di += 4 * a + 6;
		else
		{
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}
// ��ָ��λ����ʾһ���ַ�
// x,y:��ʼ����
// num:Ҫ��ʾ���ַ�:" "--->"~"
// size:�����С 12/16/24
// mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t1, t;
	u16 y0 = y;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); // �õ�����һ���ַ���Ӧ������ռ���ֽ���
	num = num - ' ';										   // �õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩
	for (t = 0; t < csize; t++)
	{
		if (size == 12)
			temp = asc2_1206[num][t]; // ����1206����
		else if (size == 16)
			temp = asc2_1608[num][t]; // ����1608����
		else if (size == 24)
			temp = asc2_2412[num][t]; // ����2412����
		else
			return; // û�е��ֿ�
		for (t1 = 0; t1 < 8; t1++)
		{
			if (temp & 0x80)
				LCD_Fast_DrawPoint(x, y, POINT_COLOR);
			else if (mode == 0)
				LCD_Fast_DrawPoint(x, y, BACK_COLOR);
			temp <<= 1;
			y++;
			if (y >= lcddev.height)
				return; // ��������
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				if (x >= lcddev.width)
					return; // ��������
				break;
			}
		}
	}
}
// m^n����
// ����ֵ:m^n�η�.
u32 LCD_Pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--)
		result *= m;
	return result;
}
// ��ʾ����,��λΪ0,����ʾ
// x,y :�������
// len :���ֵ�λ��
// size:�����С
// color:��ɫ
// num:��ֵ(0~4294967295);
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
////��ʾ����,��λΪ0,������ʾ
////x,y:�������
////num:��ֵ(0~999999999);
////len:����(��Ҫ��ʾ��λ��)
////size:�����С
////mode:
////[7]:0,�����;1,���0.
////[6:1]:����
////[0]:0,�ǵ�����ʾ;1,������ʾ.
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
// ��ʾ�ַ���
// x,y:�������
// width,height:�����С
// size:�����С
//*p:�ַ�����ʼ��ַ
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p)
{
	u8 x0 = x;
	width += x;
	height += y;
	while ((*p <= '~') && (*p >= ' ')) // �ж��ǲ��ǷǷ��ַ�!
	{
		if (x >= width)
		{
			x = x0;
			y += size;
		}
		if (y >= height)
			break; // �˳�
		LCD_ShowChar(x, y, *p, size, 0);
		x += size / 2;
		p++;
	}
}

// ���� 16*16
void GUI_DrawFont16(u16 x, u16 y, u8 *s, u8 mode)
{
	u8 i, j;
	u16 k;
	u16 HZnum;
	u16 x0 = x;
	HZnum = sizeof(tfont16) / sizeof(typFNT_GB16); // �Զ�ͳ�ƺ�����Ŀ

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
					if (!mode) // �ǵ��ӷ�ʽ
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
							LCD_DrawPoint(x, y); // ��һ����
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
		continue; // ���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
	}

	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // �ָ�����Ϊȫ��
}

// ���� 24*24
void GUI_DrawFont24(u16 x, u16 y, u8 *s, u8 mode)
{
	u8 i, j;
	u16 k;
	u16 HZnum;
	u16 x0 = x;
	HZnum = sizeof(tfont24) / sizeof(typFNT_GB24); // �Զ�ͳ�ƺ�����Ŀ

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
					if (!mode) // �ǵ��ӷ�ʽ
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
							LCD_DrawPoint(x, y); // ��һ����
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
		continue; // ���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
	}

	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // �ָ�����Ϊȫ��
}

// ���� 32*32
void GUI_DrawFont32(u16 x, u16 y, u8 *s, u8 mode)
{
	u8 i, j;
	u16 k;
	u16 HZnum;
	u16 x0 = x;
	HZnum = sizeof(tfont32) / sizeof(typFNT_GB32); // �Զ�ͳ�ƺ�����Ŀ
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
					if (!mode) // �ǵ��ӷ�ʽ
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
							LCD_DrawPoint(x, y); // ��һ����
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
		continue; // ���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
	}

	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // �ָ�����Ϊȫ��
}

// ��ʾ���ֻ����ַ���
void Show_Str(u16 x, u16 y, u8 *str, u8 size, u8 mode)
{
	u16 x0 = x;
	u8 bHz = 0;		  // �ַ���������
	while (*str != 0) // ����δ����
	{
		if (!bHz)
		{
			if (x > (lcddev.width - size / 2) || y > (lcddev.height - size))
				return;
			if (*str > 0x80)
				bHz = 1; // ����
			else		 // �ַ�
			{
				if (*str == 0x0D) // ���з���
				{
					y += size;
					x = x0;
					str++;
				}
				else
				{
					if (size >= 24) // �ֿ���û�м���12X24 16X32��Ӣ������,��8X16����
					{
						LCD_ShowChar(x, y, *str, 24, mode);
						x += 12; // �ַ�,Ϊȫ�ֵ�һ��
					}
					else
					{
						LCD_ShowChar(x, y, *str, size, mode);
						x += size / 2; // �ַ�,Ϊȫ�ֵ�һ��
					}
				}
				str++;
			}
		}
		else // ����
		{
			if (x > (lcddev.width - size) || y > (lcddev.height - size))
				return;
			bHz = 0; // �к��ֿ�
			if (size == 32)
				GUI_DrawFont32(x, y, str, mode);
			else if (size == 24)
				GUI_DrawFont24(x, y, str, mode);
			else
				GUI_DrawFont16(x, y, str, mode);

			str += 2;
			x += size; // ��һ������ƫ��
		}
	}
}

// ��ʾ40*40ͼƬ
void Gui_Drawbmp16(u16 x, u16 y, const unsigned char *p) // ��ʾ40*40ͼƬ
{
	int i;
	unsigned char picH, picL;
	LCD_Set_Window(x, y, 40, 40);
	LCD_WriteRAM_Prepare();

	for (i = 0; i < 40 * 40; i++)
	{
		picL = *(p + i * 2); // ���ݵ�λ��ǰ
		picH = *(p + i * 2 + 1);
		LCD_WriteRAM(picH << 8 | picL);
	}
	LCD_Set_Window(0, 0, lcddev.width, lcddev.height); // �ָ���ʾ����Ϊȫ��
}

// ������ʾ
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
	LCD_Clear(WHITE);	// ����
	POINT_COLOR = BLUE; // ��������Ϊ��ɫ
	BACK_COLOR = WHITE;
	LCD_ShowString(lcddev.width - 24, 0, 200, 16, 16, "RST"); // ��ʾ��������
	POINT_COLOR = RED;										  // ���û�����ɫ
}
////////////////////////////////////////////////////////////////////////////////
// ���ݴ�����ר�в���
// ��ˮƽ��
// x0,y0:����
// len:�߳���
// color:��ɫ
void gui_draw_hline(u16 x0, u16 y0, u16 len, u16 color)
{
	if (len == 0)
		return;
	LCD_Fill(x0, y0, x0 + len - 1, y0, color);
}
// ��ʵ��Բ
// x0,y0:����
// r:�뾶
// color:��ɫ
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

// ��һ������
//(x1,y1),(x2,y2):��������ʼ����
// size�������Ĵ�ϸ�̶�
// color����������ɫ
void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2, u8 size, u16 color)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	if (x1 < size || x2 < size || y1 < size || y2 < size)
		return;
	delta_x = x2 - x1; // ������������
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0)
		incx = 1; // ���õ�������
	else if (delta_x == 0)
		incx = 0; // ��ֱ��
	else
	{
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0; // ˮƽ��
	else
	{
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x; // ѡȡ��������������
	else
		distance = delta_y;
	for (t = 0; t <= distance + 1; t++) // �������
	{
		gui_fill_circle(uRow, uCol, size, color); // ����
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

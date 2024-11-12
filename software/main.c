#include "STC32G.H"
#include "stc.h"
#include "usb.h"
#include "uart.h"
#include "stdio.h"
#include "intrins.h"
#include "STC32G_GPIO.h"
#include "LCD.h"
#include "test code.h"
#include "STC32G_SPI.h"
#include "STC32G_Switch.h"
#include "STC32G_NVIC.h"
#include "STC32G_Clock.h"
#include <string.h>
#include "STC32G_PWM.h"
#include "STC32G_Timer.h"
#include "tinymaix.h"

/****************  SPI初始化函数 *****************/
void SPI_config1(void)
{
#ifdef LOONG
	P2_MODE_IO_PU(GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1);
	P2_SPEED_HIGH(GPIO_Pin_3 | GPIO_Pin_5); // 电平转换速度快（提高IO口翻转速度）
#else
	P1_MODE_IO_PU(GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_6);
	P4_MODE_IO_PU(GPIO_Pin_7);
	P1_SPEED_HIGH(GPIO_Pin_3 | GPIO_Pin_5); // 电平转换速度快（提高IO口翻转速度）
#endif
}

/**
 * 检查是否接收到特定的ISP固件升级请求。
 * 需要配合usb cdc使用,每次接收到信息,先检查这个
 */
void checkISP()
{
	// 怕编译器优化太傻，不能优化strlen,直接写8
	if (memcmp("@STCISP#", RxBuffer, 8) == 0)
	{
		usb_write_reg(OUTCSR1, 0);
		USBCON = 0x00;
		USBCLK = 0x00;
		IRC48MCR = 0x00;

		P3M0 &= ~0x03; // 设置为高阻
		P3M1 |= 0x03;
		delay_ms(10); // 留足时间给usb线插好
		// 复位到bootloader
		IAP_CONTR = 0x60;
		while (1)
			;
	}
}

char putchar(char c)
{
	TxBuffer[0] = c;
	uart_send(1);
	return c;
}

void configBlackLightPWM(u8 brightness)
{
	const u16 period = (u32)MAIN_Fosc / (u32)(20 * 1000);
	PWMx_InitDefine PWMx_InitStructure;

	// PWM1P_3 P6.0
	P6_MODE_IO_PU(GPIO_Pin_0);

	PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;							// 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
	PWMx_InitStructure.PWM_Duty = (u32)brightness * (u32)period / (u32)255; // PWM占空比时间, 0~Period
	PWMx_InitStructure.PWM_EnoSelect = ENO1P;								// 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
	PWM_Configuration(PWM1, &PWMx_InitStructure);							// 初始化PWM1

	PWMx_InitStructure.PWM_Period = period;		   // 周期时间,   0~65535
	PWMx_InitStructure.PWM_DeadTime = 0;		   // 死区发生器设置, 0~255
	PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 主输出使能, ENABLE,DISABLE
	PWMx_InitStructure.PWM_CEN_Enable = ENABLE;	   // 使能计数器, ENABLE,DISABLE
	PWM_Configuration(PWMA, &PWMx_InitStructure);  // 初始化PWM通用寄存器,  PWMA,PWMB

	PWM1_USE_P60P61();
	NVIC_PWM_Init(PWMA, DISABLE, Priority_0);
}
u16 Get_ADC12bitResult(u8 channel) // channel = 0~15
{
	ADC_RES = 0;
	ADC_RESL = 0;

	ADC_CONTR = (ADC_CONTR & 0xf0) | channel; // 设置ADC转换通道
	ADC_START = 1;							  // 启动ADC转换
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while (ADC_FLAG == 0)
		;		  // wait for ADC finish
	ADC_FLAG = 0; // 清除ADC结束标志
	ADC_CONTR = (ADC_CONTR & 0xf0) | (u8)15;
	return (((u16)ADC_RES << 8) | ADC_RESL);
}

#define ADC_SPEED 15	 /* 0~15, ADC转换时间(CPU时钟数) = (n+1)*32  ADCCFG */
#define RES_FMT (1 << 5) /* ADC结果格式 0: 左对齐, ADC_RES: D11 D10 D9 D8 D7 D6 D5 D4, ADC_RESL: D3 D2 D1 D0 0 0 0 0 */
						 /* ADCCFG      1: 右对齐, ADC_RES: 0 0 0 0 D11 D10 D9 D8, ADC_RESL: D7 D6 D5 D4 D3 D2 D1 D0 */

void xy_reset()
{
	// 0123推挽
	P0M0 |= 0x0f;
	P0M1 &= ~0x0f;
	// 0123拉低
	P0 &= ~0x0f;
	NOP4();
}

u16 x_read()
{
	xy_reset();

	// 0和2推挽，1，3输入
	P0M0 = (P0M0 & ~0x0a) | 0x05;
	P0M1 = (P0M1 & ~0x05) | 0x0a;
	P00 = 0;
	P02 = 1;

	return Get_ADC12bitResult(11);
}

u16 y_read()
{
	xy_reset();

	// 1和3推挽，0，2输入
	P0M0 = (P0M0 & ~0x05) | 0x0a;
	P0M1 = (P0M1 & ~0x0a) | 0x05;
	P01 = 1;
	P03 = 0;

	return Get_ADC12bitResult(8);
}

// 映射adc到实际坐标
u16 remap(u16 adc_value, u16 adc_min, u16 adc_max, u16 output_min, u16 output_max)
{
	u32 normalized;
	// 确保 ADC 输入范围有效
	if (adc_min >= adc_max)
	{
		return output_min; // 返回最小值
	}

	// 检查 adc_value 是否在有效范围内
	if (adc_value < adc_min)
	{
		adc_value = adc_min; // 如果低于最小值，设为最小值
	}
	else if (adc_value > adc_max)
	{
		adc_value = adc_max; // 如果超过最大值，设为最大值
	}

	// 将 ADC 值映射到 [0, 1] 区间（使用整数运算）
	normalized = (u32)(adc_value - adc_min) * (output_max - output_min) / (adc_max - adc_min);

	// 映射到实际的输出范围
	return output_min + normalized;
}

#define SAMP_CNT 4
#define SAMP_CNT_DIV2 2
u16 X, Y;
// 抄的adc滤波代码  https://www.cnblogs.com/yuphone/archive/2010/11/28/1890239.html
u8 touch_scan(void)
{
	u8 i, j, k, min;
	u16 temp;
	u16 tempXY[2][SAMP_CNT], XY[2];
	// 采样
	for (i = 0; i < SAMP_CNT; i++)
	{
		tempXY[0][i] = x_read();
		tempXY[1][i] = y_read();
	}
	// 滤波
	for (k = 0; k < 2; k++)
	{ // 降序排列
		for (i = 0; i < SAMP_CNT - 1; i++)
		{
			min = i;
			for (j = i + 1; j < SAMP_CNT; j++)
			{
				if (tempXY[k][min] > tempXY[k][j])
					min = j;
			}
			temp = tempXY[k][i];
			tempXY[k][i] = tempXY[k][min];
			tempXY[k][min] = temp;
		}
		// 设定阈值
		if ((tempXY[k][SAMP_CNT_DIV2] - tempXY[k][SAMP_CNT_DIV2 - 1]) > 5)
			return 0;
		// 求中间值的均值
		XY[k] = (tempXY[k][SAMP_CNT_DIV2] + tempXY[k][SAMP_CNT_DIV2 - 1]) / 2;
	}
	X = XY[0];
	Y = XY[1];
	return 1;
}

static uint8_t xdata mnist_pic[28 * 28] = {0};

#include "TinyMaix/mnist_valid_q_be.h"
bit busy;
char wptr;
char rptr;
char buffer[16];

#define IMG_L (28)
#define IMG_CH (1)
#define CLASS_N (14)
static uint8_t xdata mdl_buf[MDL_BUF_LEN];
static tm_err_t layer_cb(tm_mdl_t *mdl, tml_head_t *lh)
{
#if TM_ENABLE_STAT
	// dump middle result
	int x, y, c;
	int h = lh->out_dims[1];
	int w = lh->out_dims[2];
	int ch = lh->out_dims[3];
	mtype_t *output = TML_GET_OUTPUT(mdl, lh);
	return TM_OK;
	TM_PRINTF("Layer %d callback ========\n", mdl->layer_i);
#if 1
	for (y = 0; y < h; y++)
	{
		TM_PRINTF("[");
		for (x = 0; x < w; x++)
		{
			TM_PRINTF("[");
			for (c = 0; c < ch; c++)
			{
#if TM_MDL_TYPE == TM_MDL_FP32
				TM_PRINTF("%.3f,", output[(y * w + x) * ch + c]);
#else
				TM_PRINTF("%.3f,", TML_DEQUANT(lh, output[(y * w + x) * ch + c]));
#endif
			}
			TM_PRINTF("],");
		}
		TM_PRINTF("],\n");
	}
	TM_PRINTF("\n");
#endif
#endif
	return TM_OK;
}

static void parse_output(tm_mat_t *outs)
{
	tm_mat_t out = outs[0];
	float *dat = (float *)out.dat;
	float maxp = 0;
	int maxi = -1;
	int i = 0;
	for (; i < CLASS_N; i++)
	{
		if (dat[i] > maxp)
		{
			maxi = i;
			maxp = dat[i];
		}
	}
	TM_PRINTF("### Predict output is: Number %d , Prob %.3f\r\n", maxi, maxp);
	return;
}

// 插入usb线时，由于usb接口机械设计，vcc会先接通，然后才是d+d-
// 而stc的逻辑是开机检测d+d-接好，且boot引脚P3.2为0，才进入usb下载模式
// 这样按住boot，插usb，大概率由于这个机械结构，在检测d+d-时，d+d-大概率没有插到位
// 导致进入boot失败
// 在程序最前面首先执行这个函数，重新检测boot，然后强跳转程序到下载模式
// 就可以实现，按住boot插usb，一定跳转到下载模式
// 同时，由于他在代码最前面执行，即使后续程序会死机，这段代码也能先执行，完美救砖
void check_boot()
{
	// P3.2 输入模式  开启上拉电阻
	P3M0 &= ~0x04;
	P3M1 |= 0x04;
	P3PU |= 0x04;
	delay_ms(1);
	if (P32 == 0)
	{
		usb_write_reg(OUTCSR1, 0);
		USBCON = 0x00;
		USBCLK = 0x00;
		IRC48MCR = 0x00;

		P3M0 &= ~0x03; // 设置为高阻
		P3M1 |= 0x03;
		delay_ms(500); // 留足时间给usb线插好
		// 复位到bootloader
		IAP_CONTR = 0x60;
		while (1)
			;
	}
	// 关闭上拉电阻
	P3PU &= ~0x04;
}

void main(void)
{
	tm_mdl_t mdl;
	tm_mat_t in_uint8;
	tm_mat_t in;
	tm_mat_t outs[1];
	tm_err_t res;

	in_uint8.dims = 3;
	in_uint8.h = IMG_L;
	in_uint8.w = IMG_L;
	in_uint8.c = IMG_CH;
	in_uint8.dat = (mtype_t *)mnist_pic;

	in.dims = 3;
	in.h = IMG_L;
	in.w = IMG_L;
	in.c = IMG_CH;
	in.dat = NULL;

	WTST = 0;  // 设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快
	EAXFR = 1; // 扩展寄存器(XFR)访问使能
	CKCON = 0; // 提高访问XRAM速度
	check_boot();

	uart_init();
	usb_init();
	EA = 1;

	ADCTIM = 0x3f; // 设置通道选择时间、保持时间、采样时间
	ADCCFG = RES_FMT + ADC_SPEED;
	// ADC模块电源打开后，需等待1ms，MCU内部ADC电源稳定后再进行AD转换
	ADC_CONTR = 0x80 + 0; // ADC on + channel

	SPI_config1();

	LCD_Init();			// 初始化LCD
	LCD_Display_Dir(2); // 屏幕方向

	LCD_Clear(GREEN);
	configBlackLightPWM(255);
	LCD_Set_Window(0, 0, 320, 240);
	LCD_WriteRAM_Prepare();
	SPI_DC = 1;
	configBlackLightPWM(255);

	res = tm_load(&mdl, mdl_data, mdl_buf, layer_cb, &in);
	if (res != TM_OK)
	{
		TM_PRINTF("tm model load err %d\r\n", res);
		return;
	}

	while (1)
	{

		if (RxFlag)
		{
			checkISP();
			// printf("touch_x: %d touch_y:%d\n", X, Y);
			uart_recv_done(); // 对接收的数据处理完成后,一定要调用一次这个函数,以便CDC接收下一笔串口数据
							  // TM_DBGT_START();
#if TM_ENABLE_STAT
			{
				int xxx;
				tm_stat((tm_mdlbin_t *)mdl_data);
				for (xxx = 0; xxx < 28 * 28; xxx++)
				{
					TM_PRINTF("%3d,", mnist_pic[xxx]);
					if (xxx % 28 == 27)
						printf("\r\n");
				}
			}
#endif
			res = tm_preprocess(&mdl, TMPP_UINT2INT, &in_uint8, &in);

			res = tm_run(&mdl, &in, outs);

			if (res == TM_OK)
			{
				parse_output(outs);
			}
			{
				int xxx;
				for (xxx = 0; xxx < 28 * 28; xxx++)
				{

					mnist_pic[xxx] = 0;
				}
			}
			LCD_Clear(GREEN);
		}

		if (touch_scan())
		{
			u16 x, y;
			x = remap(X, 535, 3600, 0, 320);
			y = remap(Y, 600, 3300, 0, 240);
			if ((x == 0) || (y == 0))
			{
				continue;
			}

			if (x > 16 && x < 96)
			{
				if (y > 16 && y < 96)
				{
					// 			if ( x < 112)
					// {
					// 	if (y < 112)
					// 	{
					// todo 优化
					LCD_Fill(x, y, x + 4, y + 4, BLACK);
					// mnist_pic[(x / 4 + 1) + (y / 4) * 28] = 255;
					// mnist_pic[(x / 4) + (y / 4 + 1) * 28] = 255;
					// mnist_pic[(x / 4 + 1) + (y / 4 + 1) * 28] = 255;
					mnist_pic[(x / 4) + (y / 4) * 28] = 255;
				}
			}
		}
	}
}
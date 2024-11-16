#include "common.h"
#include "touch.h"
#include "usb.h"
#include "uart.h"
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
#include "expression.h"
#include "backLight.h"

#define COLLECT_MODE 0

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
	// 如果usb没有连接，发送会卡住，这里检查是否连接再发送
	if (DeviceState != DEVSTATE_CONFIGURED)
	{
		return;
	}
	
	// 如果usb已连接，但电脑上没有打开该串口，这时发送也会卡住
	// 观察到在win11上，打开串口后，CDC_DTR会变成1，反之为0
	// 不知道这个行为能否推广到其他系统。
	if (CDC_DTR==0)
	{
		return;
	}
	
	// 无优化，这样打印会很慢
	TxBuffer[0] = c;
	uart_send(1);
	return c;
}

tm_mdl_t mdl;
tm_mat_t in_uint8;
tm_mat_t in;
tm_mat_t outs[1];
tm_err_t res;
bit busy;
char wptr;
char rptr;
char buffer[16];

#define IMG_L (28)
#define IMG_CH (1)
#define CLASS_N (14)
#include "TinyMaix/mnist_valid_q_be.h"

static uint8_t xdata mdl_buf[MDL_BUF_LEN];
static uint8_t xdata mnist_pic[28 * 28] = {0};
// 这里可以用bitmap优化，但是没必要，内存够用
static uint8_t xdata mnist_pic_large[24 * 80] = {0};
static uint8_t xdata expression[16] = {0};
static uint8_t xdata expression_n = 0;
static uint8_t xdata strBuffer[50] = {0};

//
static uint8_t parse_output(tm_mat_t *outs)
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
	return maxi;
}

void clean_mnist_pic()
{
	memset(mnist_pic, 0, 28 * 28 * sizeof(mnist_pic[0]));
}
void clean_mnist_pic_large()
{
	memset(mnist_pic_large, 0, 24 * 80 * sizeof(mnist_pic_large[0]));
}
#define IMG_WIDTH 80
#define IMG_HEIGHT 24
#define CHAR_IMG_SIZE 28
#define BAD_RECOGNIZE 255
// recognize 函数定义
uint8_t recognize(uint32_t start_col, uint32_t end_col)
{
	uint32_t col, row;
	uint32_t top_row, bottom_row;
	uint32_t hor_offset, ver_offset;
	uint32_t effective_height;

	// 处理宽度只有一列的情况，区分点和有效字符
	if (end_col - start_col <= 3)
	{
		uint32_t count = 0;
		for (col = start_col; col <= end_col; col++)
		{
			for (row = 0; row < IMG_HEIGHT; row++)
			{
				if (mnist_pic_large[row * IMG_WIDTH + col] > 0)
				{
					count++;
				}
			}
		}

		if (count < 10)
		{
			return BAD_RECOGNIZE; // 认为是干扰点，跳过
		}
	}

	clean_mnist_pic();

	// 计算水平偏移量
	hor_offset = (CHAR_IMG_SIZE - (end_col - start_col + 1)) / 2;

	// 计算有效的字符高度范围
	top_row = IMG_HEIGHT;
	bottom_row = 0;
	for (row = 0; row < IMG_HEIGHT; row++)
	{
		for (col = start_col; col <= end_col; col++)
		{
			if (mnist_pic_large[row * IMG_WIDTH + col] > 0)
			{
				if (row < top_row)
					top_row = row;
				if (row > bottom_row)
					bottom_row = row;
			}
		}
	}

	// 计算有效字符区域的高度
	effective_height = bottom_row - top_row + 1;

	// 计算垂直偏移量（基于有效高度居中）
	ver_offset = (CHAR_IMG_SIZE - effective_height) / 2;

	// 分割字符区域并扩展到 28x28 大小，居中处理
	for (col = start_col; col <= end_col; col++)
	{
		for (row = 0; row < IMG_HEIGHT; row++)
		{
			if (mnist_pic_large[row * IMG_WIDTH + col] > 0)
			{
				// 将字符区域扩展到 28x28 大小并居中
				uint32_t new_col = col - start_col + hor_offset;
				uint32_t new_row = row - top_row + ver_offset;

				// 如果字符高度和位置不适合居中，进行调整
				if (new_row < 0)
				{
					new_row = 0;
				}
				else if (new_row >= CHAR_IMG_SIZE)
				{
					new_row = CHAR_IMG_SIZE - 1;
				}

				mnist_pic[new_row * CHAR_IMG_SIZE + new_col] = mnist_pic_large[row * IMG_WIDTH + col];
			}
		}
	}

#if COLLECT_MODE
	{
		int xxx;
		for (xxx = 0; xxx < 28 * 28; xxx++)
		{
			TM_PRINTF("%3d,", mnist_pic[xxx]);
			if (xxx % 28 == 27)
				printf("\r\n");
		}
		printf("@\r\n");
	}
	return 0;
#else
	{
		// uint32_t xx;
		// printf("@");
		// for (xx = 0; xx < 300; xx++)
		// {
		tm_preprocess(&mdl, TMPP_UINT2INT, &in_uint8, &in);

		tm_run(&mdl, &in, outs);
		// }
		// printf("!");
	}
	return parse_output(outs);
#endif
}

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

void interrupt0()
// 用这个判断是否位keil编译器，避免vscode里不认keil语法报错
// https://developer.arm.com/documentation/101655/0961/Cx51-User-s-Guide/Preprocessor/Macros/Predefined-Macros
#ifdef __MODEL__
	interrupt INT0_VECTOR
#endif
{
	//这个按钮没啥用，姑且配置为按下进下载模式
	check_boot();
}

int8 btn0;
void main(void)
{

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

	{
		// 配置boot按钮用作正常按钮
		// 按下后会进入外部中断 interrupt0
		// P3.2(interrupt 0)准双向
		P3M0 &= ~0x04;
		P3M1 &= ~0x04;
		IT0 = 1; // INT0 下降沿中断
		EX0 = 1; // 使能 INT0 中断
	}

	uart_init();
	usb_init();
	EA = 1;

	touch_init();

	LCD_Init();			// 初始化LCD
	LCD_Display_Dir(2); // 屏幕方向

	// 先点屏幕数据，后点亮背光，避免显示出雪花屏
	LCD_Clear(WHITE);
	
	// 点亮背光，最高亮度
	configBackLightPWM(255);

	res = tm_load(&mdl, mdl_data, mdl_buf, layer_cb, &in);
	if (res != TM_OK)
	{
		TM_PRINTF("tm model load err %d\r\n", res);
		return;
	}

	while (1)
	{

		if (btn0)
		{
			btn0 = 0;
			{
				uint32_t x, y;
				uint8_t sum;
				int in_char = 0;		// 标记当前是否在字符区域
				int char_count = 0;		// 字符数量计数
				uint32_t start_col = 0; // 字符起始列
				expression_n = 0;

				for (x = 0; x < 80; x++)
				{
					sum = 0;
					for (y = 0; y < 24; y++)
					{
						sum |= mnist_pic_large[y * 80 + x];
					}

					if (sum)
					{
						// 当前列有非零值，表明在字符区域内
						if (!in_char)
						{
							in_char = 1;   // 标记进入字符区域
							start_col = x; // 记录字符起始列
							char_count++;  // 字符数量增加
						}
					}
					else
					{
						// 当前列全为零，表明不在字符区域内
						if (in_char)
						{
							uint32_t end_col;
							uint8_t temp;
							in_char = 0; // 标记离开字符区域
							end_col = x - 1;
							temp = recognize(start_col, end_col);
							if (temp != BAD_RECOGNIZE)
							{
								expression[expression_n++] = temp;
							}
							// printf("字符 %d ", char_count);
							// printf("起始列: %d,", start_col);
							// printf("结束列: %d\n", end_col);
						}
					}

					// printf("%d ", !!sum);
				}

				// 处理最后一个字符到图片末尾的情况
				if (in_char)
				{
					// 最后一个字符的结束列为图片末尾
					uint32_t end_col = 79;
					uint8_t temp;

					if (temp != BAD_RECOGNIZE)
					{
						expression[expression_n++] = temp;
					}
					// printf("字符 %d ", char_count);
					// printf("起始列: %d,", start_col);
					// printf("结束列: %d\n", end_col);
				}

				// printf("\n字符数\xFD量: %d\n", char_count);

				LCD_Clear(WHITE);
				clean_mnist_pic_large();
#if COLLECT_MODE
#else
				sprintf(strBuffer, "result: %.2f", expression_calc(expression, expression_n));
				Show_Str(10, 200, strBuffer, 24, 0);
#endif
			}
		}

		if (RxFlag)
		{
			checkISP();
			uart_recv_done(); // 对接收的数据处理完成后,一定要调用一次这个函数,以便CDC接收下一笔串口数据
			btn0++;
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

			// 这里没考虑边缘越界，但是好像没事
			if (x > 0 && x < 80 * 4)
			{
				if (y > 0 && y < 24 * 4)
				{
					LCD_Fill(x, y, x + 4, y + 4, BLACK);
					mnist_pic_large[(x / 4) + (y / 4) * 80] = 255;
				}
			}

			if (y > 24 * 4)
			{
				btn0++;
			}
		}
	}
}
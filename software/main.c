#include "common.h"
#include "touch.h"
#include "stc.h"
#include "usb.h"
#include "uart.h"
#include "LCD.h"
#include <string.h>
#include "expression.h"
#include "backLight.h"
#include "Ai.h"
#include "canvas.h"

uint8_t xdata expression[16] = {0};
uint8_t xdata expression_n = 0;
static uint8_t xdata strBuffer[50] = {0};

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
		return c;
	}

	// 如果usb已连接，但电脑上没有打开该串口，这时发送也会卡住
	// 观察到在win11上，打开串口后，CDC_DTR会变成1，反之为0
	// 不知道这个行为能否推广到其他系统。
	if (CDC_DTR == 0)
	{
		return c;
	}

	// 无优化，这样打印会很慢
	TxBuffer[0] = c;
	uart_send(1);
	return c;
}

/**
 * @brief 扩展字符串至指定长度，使用指定字符填充不足的部分
 *
 * 该函数将原始字符串扩展到指定的新长度。如果原始字符串的长度小于新长度，
 * 它将在字符串末尾添加指定字符，直到达到新长度。扩展后的字符串会以 '\0' 结尾。
 *
 * @param buffer 输入的字符串缓冲区，原始字符串会被拓展
 * @param c 用于填充字符串的字符
 * @param new_length 扩展后的目标字符串长度
 *
 * @note 如果原始字符串的长度已经大于或等于目标长度，函数不会做任何改变。
 */
void paddingString(uint8_t *buffer, char c, uint8_t new_length)
{
	uint8_t current_length;
	uint8_t i;

	current_length = (uint8_t)strlen((char *)buffer); // 获取当前字符串长度

	// 如果当前长度已经大于或等于新长度，则无需填充
	if (current_length >= new_length)
	{
		return;
	}

	// 从当前字符串末尾开始填充，直到达到新长度
	for (i = current_length; i < new_length; ++i)
	{
		buffer[i] = c;
	}

	// 添加字符串终止符
	buffer[new_length] = '\0';
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
int8 btn0;

void interrupt0()
// 用这个判断是否位keil编译器，避免vscode里不认keil语法报错
// https://developer.arm.com/documentation/101655/0961/Cx51-User-s-Guide/Preprocessor/Macros/Predefined-Macros
#ifdef __MODEL__
	interrupt INT0_VECTOR
#endif
{
	// 这个按钮没啥用，姑且配置为按下进下载模式
	// check_boot();
	btn0++;
}

void main(void)
{
	bool beginner = true;
	
	EAXSFR();	//允许访问扩展寄存器
	WTST  = 0;
	CKCON = 0;

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
	LCD_Fill_LARGE(0, 96 + 1, 319, 96 + 3, BLACK); // 再屏幕中间画一条线
	POINT_COLOR = GRAY;
	Show_Str(10, 30, "在这里输入表达式", 32, 0);
	Show_Str(10, 150, "点击这里开始运算", 32, 0);
	POINT_COLOR = BLACK;

	// 点亮背光，最高亮度
	configBackLightPWM(255);

	Ai_init();

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

				for (x = 0; x < CANVAS_WIDTH; x++)
				{
					sum = 0;
					for (y = 0; y < CANVAS_HEIGHT; y++)
					{
						sum |= canvas[y * CANVAS_WIDTH + x];
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
							in_char = 0; // 标记离开字符区域
							end_col = x - 1;

							if (canvas_process_character(start_col, end_col))
							{
								expression[expression_n++] = Ai_run();
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
					uint32_t end_col = CANVAS_WIDTH - 1;

					if (canvas_process_character(start_col, end_col))
					{
						expression[expression_n++] = Ai_run();
					}
					// printf("字符 %d ", char_count);
					// printf("起始列: %d,", start_col);
					// printf("结束列: %d\n", end_col);
				}

				// printf("\n字符数\xFD量: %d\n", char_count);

				if (beginner)
				{
					beginner = false;
					LCD_Fill(0, 96 + 3, 319, 239, WHITE);
				}

				sprintf(strBuffer, "result: %.2f", expression_calc(expression, expression_n));
				// 用在字符后面拼接空格的方法，清理残留字符，避免全屏刷新造成的闪烁。
				paddingString(strBuffer, ' ', 20);
				Show_Str(10, 200, strBuffer, 24, 0);
				expression_to_string(expression, expression_n, strBuffer);
				paddingString(strBuffer, ' ', 20);
				Show_Str(10, 170, strBuffer, 24, 0);

				LCD_Fill_LARGE(0, 0, 319, 96, WHITE); // 屏幕上边清空
				clean_canvas();
			}
		}

		if (RxFlag)
		{
			checkISP();
			uart_recv_done(); // 对接收的数据处理完成后,一定要调用一次这个函数,以便CDC接收下一笔串口数据
#if TM_ENABLE_STAT
			{
				tm_stat((tm_mdlbin_t *)mdl_data);
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
			if (x > 0 && x < CANVAS_WIDTH * 4)
			{
				if (y > 0 && y < CANVAS_HEIGHT * 4)
				{
					LCD_Fill(x, y, x + 4, y + 4, BLACK);
					canvas[(x / 4) + (y / 4) * CANVAS_WIDTH] = 255;
				}
			}

			// 整个个屏幕下部是是个大按钮
			if (y > CANVAS_HEIGHT * 4)
			{
				btn0++;
			}
		}
	}
}
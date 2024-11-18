#ifndef __LCD_H
#define __LCD_H
#include "common.h"
#include "delay.h"

// LCD重要参数集
typedef struct
{
	u16 width;	 // LCD 宽度
	u16 height;	 // LCD 高度
	u16 id;		 // LCD ID
	u8 dir;		 // 横屏还是竖屏控制：0，竖屏；1，横屏。
	u16 wramcmd; // 开始写gram指令
	u16 setxcmd; // 设置x坐标指令
	u16 setycmd; // 设置y坐标指令
} _lcd_dev;

// LCD参数
extern _lcd_dev lcddev; // 管理LCD重要参数
// LCD的画笔颜色和背景色
extern u16 POINT_COLOR; // 默认红色
extern u16 BACK_COLOR;	// 背景颜色.默认为白色

////////////////////////////////////////////////////////////////////
//-----------------LCD端口定义----------------

// 如果修改引脚，还要改LCD_Init函数里对io的初始化
sbit SPI_SDA = P2 ^ 5; // 数据输入
sbit SPI_RST = P0 ^ 5; // 复位
sbit SPI_SCK = P2 ^ 7; // 时钟
sbit SPI_DC = P5 ^ 1;  // 数据/命令
// sbit LCD_LED    = P0^4; 		//背光 开关  不启用，使用pwm控制

// 画笔颜色
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0XF81F
#define GRED 0XFFE0
#define GBLUE 0X07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0XBC40 // 棕色
#define BRRED 0XFC07 // 棕红色
#define GRAY 0X8430	 // 灰色

void LCD_Init(void);									  // 初始化
void LCD_DisplayOn(void);								  // 开显示
void LCD_DisplayOff(void);								  // 关显示
void LCD_Clear(u16 Color);								  // 清屏
void LCD_SetCursor(u16 Xpos, u16 Ypos);					  // 设置光标
void LCD_DrawPoint(u16 x, u16 y);						  // 画点
void LCD_Fast_DrawPoint(u16 x, u16 y, u16 color);		  // 快速画点
void LCD_Draw_Circle(u16 x0, u16 y0, u8 r);				  // 画圆
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);		  // 画线
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);	  // 画矩形
void LCD_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 color); // 填充单色
void LCD_Fill_LARGE(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);
void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode);				  // 显示一个字符
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size);				  // 显示一个数字
void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode);		  // 显示 数字
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p); // 显示一个字符串,12/16字体

void LCD_WR_REG(u16 REG);
void LCD_WR_DATA(u16 DATA);
void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue);
void LCD_WriteRAM_Prepare(void);
void LCD_WriteRAM(u16 RGB_Code);

void SoftSPI_WriteByte(u8 Byte);

void Load_Drow_Dialog(void);
void LCD_Set_Window(u16 sx, u16 sy, u16 width, u16 height);				 // 设置窗口
void Show_Str(u16 x, u16 y, u8 *str, u8 size, u8 mode);					 // 显示中文
void Gui_Drawbmp16(u16 x, u16 y, const unsigned char *p);				 // 显示40*40 图片
void Gui_StrCenter(u16 x, u16 y, u8 *str, u8 size, u8 mode);			 // 居中显示
void LCD_Display_Dir(u8 dir);											 // 设置LCD显示方向
void lcd_draw_bline(u16 x1, u16 y1, u16 x2, u16 y2, u8 size, u16 color); // 画一条粗线
void gui_fill_circle(u16 x0, u16 y0, u16 r, u16 color);					 // 画实心圆
void gui_draw_hline(u16 x0, u16 y0, u16 len, u16 color);				 // 画水平线 电容触摸屏专有部分

#endif

#ifndef __TEST_CODE_H
#define __TEST_CODE_H
#include "sys.h"


void rtp_test(void);						//电阻触摸屏测试函数
void ctp_test(void);						//电容触摸屏测试函数
void DrawTestPage(u8 *str);			//主固定栏
void Chinese_Font_test(void); 	//中文测试
void main_test(u8 *id);					//测试主页
void FillRec_Test(void);				//图形测试
void English_Font_test(void);		//英文测试
void Pic_test(void);						//图片测试
void Rotate_Test(void);					//角度测试
void Color_Test(void);					//纯色测试
void Switch_test(void);					//显示开关测试
void Read_Test(void);						//读颜色测试


#endif


#ifndef TOUCH_H
#define TOUCH_H

#include "common.h"
extern u16 X, Y;
void touch_init();
void xy_reset();
u16 x_read();
u16 y_read();
u16 remap(u16 adc_value, u16 adc_min, u16 adc_max, u16 output_min, u16 output_max);
u8 touch_scan(void);

#endif
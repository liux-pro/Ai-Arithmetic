#ifndef __CANVAS__H
#define __CANVAS__H
#include "stdint.h"
#include "stdbool.h"

extern uint8_t xdata canvas[24 * 80];

bool canvas_process_character(uint32_t start_col, uint32_t end_col);
void clean_canvas();

#endif
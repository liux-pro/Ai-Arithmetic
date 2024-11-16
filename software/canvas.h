#ifndef __CANVAS__H
#define __CANVAS__H
#include "stdint.h"
#include "stdbool.h"

#define COLLECT_MODE 0

extern uint8_t xdata canvas[CANVAS_HEIGHT * CANVAS_WIDTH];

bool canvas_process_character(uint32_t start_col, uint32_t end_col);
void clean_canvas();

#endif
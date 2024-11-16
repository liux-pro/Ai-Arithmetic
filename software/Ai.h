#ifndef __Ai__H
#define __Ai__H
#include "tinymaix.h"


#define CANVAS_WIDTH 80
#define CANVAS_HEIGHT 24
#define CHAR_IMG_SIZE 28

#define IMAGE_WIDTH (28)
#define IMAGE_HEIGHT (28)

#define IMAGE_CHANNEL (1)
#define CLASS_N (14)

void Ai_init();
uint8_t Ai_run();
uint8_t parse_output(tm_mat_t *outs);
void clean_input_image();

#endif
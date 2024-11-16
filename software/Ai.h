#ifndef __Ai__H
#define __Ai__H
#include "tinymaix.h"


#define IMG_WIDTH 80
#define IMG_HEIGHT 24
#define CHAR_IMG_SIZE 28
#define BAD_RECOGNIZE 255

#define IMG_L (28)
#define IMG_CH (1)
#define CLASS_N (14)

void Ai_init();
uint8_t Ai_run();
uint8_t parse_output(tm_mat_t *outs);
void clean_input_image();

#endif
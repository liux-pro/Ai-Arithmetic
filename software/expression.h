#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <stdint.h>

// º¯ÊýÉùÃ÷
float expression_calc(uint8_t item[], uint8_t n);
void expression_to_string(uint8_t *expression, uint8_t expr_length, uint8_t *buffer);
    
#endif // EXPRESSION_H

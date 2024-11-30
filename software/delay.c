#include "delay.h"
void delay_ms(int count)
{
    do
    {
        unsigned long i;
        _nop_();
        i = 8293UL;
        while (i)
            i--;
    } while (--count);
}

void delay_us(int count)
{
    int i, j;
    for (i = 0; i < count; i++)
        for (j = 0; j < 1; j++)
            ;
}
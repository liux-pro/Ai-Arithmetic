#include "sys.h"
void delay_ms(int count)  // /* X1ms */
{
     u16 i;
     do{
          i = MAIN_Fosc / 6000;
          while(--i);
     }while(--count);
}

void delay_us(int count)  // /* X1us */
{
        int i,j;
        for(i=0;i<count;i++)
                for(j=0;j<1;j++);
}
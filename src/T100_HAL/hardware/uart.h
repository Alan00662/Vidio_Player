#ifndef  __UART_H_
#define  __UART_H_

#ifdef  __cplusplus
    extern "C" {
#endif

#include "stm32f4xx.h"


void uart1_init(uint32_t bps);

void put_char(int c);
int get_char(void);

int  uart1_put_char(int ch);
int  uart1_put_buff(void *buff, int len);
int  uart1_write(void *buff, int len);


//=====================================
void uart3_init(void);
int uart3_write(void *buff, int len);

#ifdef  __cplusplus
}  
#endif

#endif



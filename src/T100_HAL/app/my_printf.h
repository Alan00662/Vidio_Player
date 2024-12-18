#ifndef  __MY_PRINTF_H_
#define  __MY_PRINTF_H_

#ifdef  __cplusplus
    extern "C" {
#endif


#include <stdarg.h>
#include <stdint.h>

int my_printf(const char *fmt, ...);
void my_printf_arr(const void *buff, int len, uint32_t start_addr);

//=========================
int d_printf(const char *fmt, ...);
void d_printf_arr(const void *buff, int len, uint32_t start_addr);
		
#ifdef  __cplusplus
}  
#endif

#endif



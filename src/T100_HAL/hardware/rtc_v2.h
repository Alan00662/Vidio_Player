#ifndef  __RTC_V2_H_
#define  __RTC_V2_H_

#include "stm32f4xx.h"


int rtc_init(void);

void set_time(int year, int month, int day, int hour, int minute, int second);
uint32_t get_time(int *t, int len);

void print_time(int flag);


#endif




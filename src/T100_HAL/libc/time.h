/*
 * lib_time.h
 *
 *  Created on: 2020年1月28日
 *      Author: WXD
 */

#ifndef __TIME_H_
#define __TIME_H_

#include <stdint.h>

int is_leap_year(int year);
uint32_t my_mktime(int year, int mon, int day, int hour, int min, int sec);
void     my_gmtime(uint32_t s,  int  t[]);

int utc2bj(int *t, int len);
int bj2utc(int *t, int len);

int get_week(int year, int month, int date);
int get_days_in_month(int year, int month);

#endif /* LIB_TIME_H_ */



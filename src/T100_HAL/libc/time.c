/*
 * lib_timm.c
 *
 *  Created on: 2020年1月28日
 *      Author: WXD
 */
#include "time.h"

//时间转换基于1970.01.01-00:00:00
#define  BASE_YEAR  1970

//公历(格里历)纪年
static const unsigned short days[13]=
{
    0,
    31,
    31+28,
    31+28+31,
    31+28+31+30,
    31+28+31+30+31,
    31+28+31+30+31+30,
    31+28+31+30+31+30+31,
    31+28+31+30+31+30+31+31,
    31+28+31+30+31+30+31+31+30,
    31+28+31+30+31+30+31+31+30+31,
    31+28+31+30+31+30+31+31+30+31+30,
    31+28+31+30+31+30+31+31+30+31+30+31,
};

static const unsigned char days_in_month[13] =
{
    0,
    31,
    28,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31,
};

/*
 * 闰年判断的基本规则：四年一闰, 百年不闰, 四百年再闰，这就是说:
 * (1)如果是世纪年(1800 1900 2000 2100等)，必须能被400整除才是闰年
 *    (其实不管是否为世纪年,能被400整除就是闰年)
 * (2)如果不是世纪年，能被4整除也是闰年。
 */
inline int is_leap_year(int year)
{
	if((year%400==0) || (year%100!=0 && year%4==0))
		return 1;
	else
		return 0;
}

/*
 * 年月日时分秒转化为秒(自1970年1月1日0时0分0秒算起)
 */
uint32_t my_mktime(int year, int mon, int day, int hour, int min, int sec)
{
	int i;
    unsigned int  result=0 ;

    if(mon < 0x01) mon = 0x01;
    if(day < 0x01) day = 0x01;

    while(mon > 12)
    {
        mon-= 12;
        year++;
    }

    for(i=BASE_YEAR; i<year; i++)
    {
    	if(is_leap_year(i))
    		result++;
    }
    if(is_leap_year(year) && (mon>=3))
    	result++;

    year -= BASE_YEAR;
    result += days[mon-1];
    result  = (((result + day -1 +  year*365
                )*24 + hour /* now have hours */
               )*60 + min /* now have minutes */
              )*60 + sec; /* finally seconds */
    return(result);
}

/*
 * 秒(自1970年1月1日0时0分0秒算起)转化为年月日时分秒
 */
void     my_gmtime(uint32_t s,  int  t[])
{
    unsigned short dayofyear;
    unsigned short dayofmonth;
    int i;

    t[0] = s % 60;   //second
    s /= 60;
    t[1] = s % 60;   //min
    s /= 60;
    t[2] = s % 24;   //hour
    s /= 24;         //total days

    t[5] = BASE_YEAR;
    while(1)         //calculate year
    {
        if(is_leap_year(t[5])) dayofyear = 366;
        else  dayofyear = 365;

        if(s >= dayofyear)
        {
            s -= dayofyear;
            t[5]++;
        }
        else
            break;
    }

    for(i = 11 ; i >= 0x00 ; i--)
    {
        dayofmonth =days[i];
        if(is_leap_year(t[5]) && (i>=2)) dayofmonth++;

        if(s >= dayofmonth)
        {
            t[4] = i+1;
            s -= dayofmonth;
            t[3]= s+1;
            break;
        }
    }
}

/*
 * @brief: UCT时间转化为北京时间(东8区)
 */
int utc2bj(int *t, int len)
{
	if(len < 6*sizeof(int))
		return -1;

	uint32_t sec = my_mktime(t[5], t[4], t[3], t[2], t[1], t[0]);
	my_gmtime(sec+8*3600,  t);
	return 0;
}

/*
 * @brief: UCT时间转化为北京时间(东8区)
 */
int bj2utc(int *t, int len)
{
	if(len < 6*sizeof(int))
		return -1;

	uint32_t sec = my_mktime(t[5], t[4], t[3], t[2], t[1], t[0]);
	my_gmtime(sec-8*3600,  t);
	return 0;
}

//得到现在是星期几  1-星期一 ... 7-星期日 
int get_week(int year, int month, int date)
{	
    int week = 0;
    if (month==1 || month==2)
    {
        month += 12;
        year--;
    }
    week = (date + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400)%7 + 1;
    return week;	
}

//获取1个月有多少天
int get_days_in_month(int year, int month)
{
	if(month==2 && is_leap_year(year))
		return 29;
	else
		return days_in_month[month];
}







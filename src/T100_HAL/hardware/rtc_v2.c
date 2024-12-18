#include "rtc_v2.h"
#include "systick.h"
#include "my_printf.h"
#include "time.h"


//自定义用户数据, 随便一个16bit的数值即可
#define  BKP_VAL1  0x1234

int rtc_init(void)
{
	int cnt = 0;
	RTC_InitTypeDef RTC_InitStructure;
	RTC_TimeTypeDef RTC_TimeTypeInitStructure;
	RTC_DateTypeDef RTC_DateTypeInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);  //使能PWR时钟
	PWR_BackupAccessCmd(ENABLE);	                     //使能后备寄存器访问 
	
	/* 备份寄存器BKP_DR0来记录RTC是否配置过 */
	if(RTC_ReadBackupRegister(RTC_BKP_DR0) != BKP_VAL1)
	{	//以下代码只需要在RTC初始化的时候执行一次即可, 不需要每次上电都执行
		RCC_LSEConfig(RCC_LSE_ON);       //LSE开启
		delay_ms(2);
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)	//等待低速晶振就绪
		{
			cnt++;
			delay_us(1000);
			if(cnt == 4096) {           //LSE掉电后重新开启耗时约2s(比如扣掉VBAT电池后, 首次上电)
				my_printf("[%s] LSE Error! RTC Config fail!!\n\r", __FUNCTION__);
				return -1;
			}
		}
		my_printf("[%s] RTC config, cnt=%d\n\r", __FUNCTION__, cnt);

		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);   //选择LSE作为RTC时钟源 
		RCC_RTCCLKCmd(ENABLE);	                  //使能RTC时钟

		// RTC_CLK/[(RTC_AsynchPrediv+1)*(RTC_SynchPrediv+1)] = 32768/[128*256] = 1s
		RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24; //RTC设置为24小时格式
		RTC_InitStructure.RTC_AsynchPrediv = 0x7F;            //RTC异步分频系数(1~0X7F)
		RTC_InitStructure.RTC_SynchPrediv  = 0xFF;            //RTC同步分频系数(0~7FFF)
		RTC_Init(&RTC_InitStructure);
		
		RTC_TimeTypeInitStructure.RTC_Hours = 18;
		RTC_TimeTypeInitStructure.RTC_Minutes = 59;
		RTC_TimeTypeInitStructure.RTC_Seconds = 23;
		RTC_TimeTypeInitStructure.RTC_H12 = RTC_H12_AM;
		RTC_SetTime(RTC_Format_BIN, &RTC_TimeTypeInitStructure);

		RTC_DateTypeInitStructure.RTC_WeekDay = 3;     //1-星期一 ... 6-星期六 7-星期日
		RTC_DateTypeInitStructure.RTC_Month = 12;      //1-12
		RTC_DateTypeInitStructure.RTC_Date = 2;        //1-31
		RTC_DateTypeInitStructure.RTC_Year = 20;       //0-99
		RTC_SetDate(RTC_Format_BIN, &RTC_DateTypeInitStructure);
	 
		RTC_WriteBackupRegister(RTC_BKP_DR0, BKP_VAL1);	//标记RTC已经初始化
	}

	return 0;
}

/* 至于闹钟与周期性唤醒等功能, 需要时再研究 */

//UTC时间
void set_time(int year, int month, int day, int hour, int minute, int second)
{
	RTC_TimeTypeDef RTC_TimeTypeStructure;
	RTC_DateTypeDef RTC_DateTypeStructure;
		
	RTC_TimeTypeStructure.RTC_Hours = hour;
	RTC_TimeTypeStructure.RTC_Minutes = minute;
	RTC_TimeTypeStructure.RTC_Seconds = second;
	RTC_TimeTypeStructure.RTC_H12 = RTC_H12_AM;
	RTC_SetTime(RTC_Format_BIN, &RTC_TimeTypeStructure);

	RTC_DateTypeStructure.RTC_WeekDay = get_week(year, month, day);  //1-7 7-星期日
	RTC_DateTypeStructure.RTC_Month = month;      //1-12
	RTC_DateTypeStructure.RTC_Date = day;         //1-31
	RTC_DateTypeStructure.RTC_Year = year-2000;   //0-99
	RTC_SetDate(RTC_Format_BIN, &RTC_DateTypeStructure);
}

//UTC时间
uint32_t get_time(int *t, int len)
{
	RTC_TimeTypeDef RTC_TimeTypeStructure;
	RTC_DateTypeDef RTC_DateTypeStructure;	
	
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeTypeStructure);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateTypeStructure);

	int year = RTC_DateTypeStructure.RTC_Year + 2000;
	int mon  = RTC_DateTypeStructure.RTC_Month;
	int day  = RTC_DateTypeStructure.RTC_Date;
	int hour = RTC_TimeTypeStructure.RTC_Hours;
	int min  = RTC_TimeTypeStructure.RTC_Minutes;
	int sec  = RTC_TimeTypeStructure.RTC_Seconds;
	
	if(len>=6*sizeof(int) || t!=0) {
		t[5] = year;
		t[4] = mon;
		t[3] = day;
		t[2] = hour;
		t[1] = min;
		t[0] = sec;
	}
	return my_mktime(year, mon, day, hour, min, sec);
}

void print_time(int flag)
{												
	int t[6];
	int week;
	
	get_time(t, sizeof(t));
	if(flag & 0x01)      //UTC时间
	{
		my_printf("UTC: %d-%02d-%02d %d:%02d:%02d\n\r", t[5], t[4], t[3], t[2], t[1], t[0]);
		week = get_week(t[5], t[4], t[3]);
		my_printf("week: %d\n\r", week);
	}
	
	if(flag & 0x02)     //北京时间
	{
		utc2bj(t, sizeof(t));
		my_printf("BJ:  %d-%02d-%02d %d:%02d:%02d\n\r", t[5], t[4], t[3], t[2], t[1], t[0]);
		week = get_week(t[5], t[4], t[3]);
		my_printf("week: %d\n\r", week);
	}
}








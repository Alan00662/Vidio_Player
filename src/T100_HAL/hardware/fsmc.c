#include "fsmc.h"
#include "stm32f4xx.h"

void fsmc_controller_init(void)
{
	#if 0
	GPIO_InitTypeDef  GPIO_InitStructure;
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef  writeTiming;
	FSMC_NORSRAMTimingInitTypeDef  readTiming;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOE, ENABLE);//使能PD,PE时钟  
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC,ENABLE);  //使能FSMC时钟 
	
	//PD0..PD1  -> D2..D3
	//PD4..PD5  -> NOE NWE
	//PD8..PD10 -> D13..D15 
	//PD14..PD15 -> D0..D1
	GPIO_InitStructure.GPIO_Pin = (uint16_t)0x03<<0 | (uint16_t)0x03<<4 | (uint16_t)0x07<<8 | (uint16_t)0x03<<14; //PD0..1 P4..5 PD8..10 PD14..15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//PE7..PE15 -> D4..D12
	GPIO_InitStructure.GPIO_Pin = (uint16_t)0x1FF<<7;  //PE7..15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	//PF12 -> A6(LCD_RS)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;  //F12
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	//PG12 -> NE4 (LCD_CS)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOG, &GPIO_InitStructure);	

	//Config AF
	//PD
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_FSMC);  //PD0-FSMC_D2
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_FSMC);  //PD1-FSMC_D3
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource4,GPIO_AF_FSMC);  //PD4-FSMC_NOE Read signal
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_FSMC);  //PD5-FSMC_NWE Write signal
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_FSMC);  //PD8-FSMC_D13
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_FSMC);  //PD9-FSMC_D14
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource10,GPIO_AF_FSMC); //PD10-FSMC_D15
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource14,GPIO_AF_FSMC); //PD14-FSMC_D0
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource15,GPIO_AF_FSMC); //PD15-FSMC_D1

	//PE
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource7,GPIO_AF_FSMC);  //PE7-FSMC_D4
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource8,GPIO_AF_FSMC);  //PE8-FSMC_D5
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource9,GPIO_AF_FSMC);  //PE9-FSMC_D6
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource10,GPIO_AF_FSMC); //PE10-FSMC_D7
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource11,GPIO_AF_FSMC); //PE11-FSMC_D8
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource12,GPIO_AF_FSMC); //PE12-FSMC_D9
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource13,GPIO_AF_FSMC); //PE13-FSMC_D10
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource14,GPIO_AF_FSMC); //PE14-FSMC_D11
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource15,GPIO_AF_FSMC); //PE15-FSMC_D12
	
	//PF
	GPIO_PinAFConfig(GPIOF,GPIO_PinSource12,GPIO_AF_FSMC);  //PF12-FSMC_A6 (LCD_RS)
	
	//PG
	GPIO_PinAFConfig(GPIOG,GPIO_PinSource12,GPIO_AF_FSMC); //PG12-FSMC_NE4 (LCD_CS)

	//=======================================================================
	//For LCD. ILI9486 ILI9341 R61581B0 SSD1289
	//1/168MHz = 5.95ns  1/200MHz = 5ns
	//LCD(ILI9486)读时序  ILI9486 Read cycle(min 450ns) 
	readTiming.FSMC_AccessMode = FSMC_AccessMode_A;  //模式A只需设置地址建立时间与数据保持时间, 其余参数均为0即可
	readTiming.FSMC_AddressSetupTime = 0x0F;  //NOE高电平持续时间(ILI9486 RD高电平持续时间min 90ns)=ADDST*HCLK = 15*5.95=89.28ns
	readTiming.FSMC_AddressHoldTime = 0;
//	readTiming.FSMC_DataSetupTime = 60;       //NOE低电平持续时间(ILI9486 RD低电平持续时间min 355ns,实际100ns即可)=DATAST*HCLK
	readTiming.FSMC_DataSetupTime = 18;       //ILI9486 RD低电平持续时间实际100ns即可 18*5.96=107.14ns      整个读周期:89.28+107.14=196.43ns
	readTiming.FSMC_BusTurnAroundDuration = 0;
	readTiming.FSMC_CLKDivision = 0;
	readTiming.FSMC_DataLatency = 0;
	
	//1/168MHz = 5.95ns  1/200MHz = 5ns
	//LCD(ILI9486)写时序  ILI9486 Write cycle(min 66ns) 
	writeTiming.FSMC_AccessMode = FSMC_AccessMode_A;
	writeTiming.FSMC_AddressSetupTime = 4;  //NWE高电平持续时间(ILI9486 WR高电平持续时间min 30ns)=(ADDST+1)*HCLK=(4+1)*5.95=29.76ns
	writeTiming.FSMC_AddressHoldTime = 0;
	writeTiming.FSMC_DataSetupTime = 6;     //NWE低电平持续时间(ILI9486 WR低电平持续时间min 30ns)=DATAST*HCLK=6*5.95=35.71ns 整个写周期:29.76+35.71=65.48ns
	writeTiming.FSMC_BusTurnAroundDuration = 0;
	writeTiming.FSMC_CLKDivision = 0;
	writeTiming.FSMC_DataLatency = 0;
	
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4; //NE4
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;  //地址线数据线不复用
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;  //存储器写使能
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;      //读写采用不同的时序
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readTiming;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &writeTiming;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  //初始化FSMC配置
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);  //使能BANK1区域4	
	
	#endif
}











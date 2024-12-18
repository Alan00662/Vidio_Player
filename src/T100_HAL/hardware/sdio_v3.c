#include "sdio_v3.h"


int sdio_controller_release(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	/*!< DeInitializes the SDIO peripheral */
	SDIO_DeInit();
	
	/*!< Set Power State to OFF */
	SDIO_SetPowerState(SDIO_PowerState_OFF);
	
	/*!< Disable SDIO Clock */
	SDIO_ClockCmd(DISABLE);

	/*!< Disable the SDIO AHB Clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, DISABLE);

	/*!< Configure PC.08, PC.09, PC.10, PC.11, PC.12 pin: D0, D1, D2, D3, CLK pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*!< Configure PD.02 CMD line */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	return 0;
}

void SDIO_Register_Deinit()
{
	SDIO->POWER=0x00000000;
	SDIO->CLKCR=0x00000000;
	SDIO->ARG=0x00000000;
	SDIO->CMD=0x00000000;
	SDIO->DTIMER=0x00000000;
	SDIO->DLEN=0x00000000;
	SDIO->DCTRL=0x00000000;
	SDIO->ICR=0x00C007FF;
	SDIO->MASK=0x00000000;	 
}

int sdio_controller_init(void)
{
	#if 0
	GPIO_InitTypeDef  GPIO_InitStructure;
	SDIO_InitTypeDef  SDIO_InitStructure;

	/*!< GPIOC and GPIOD Periph clock enable */
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);

	/*!< Enable the SDIO AHB Clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SDIO, ENABLE);
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, ENABLE);
	
	/*!< Configure PC.08, PC.09, PC.10, PC.11, PC.12 pin: D0, D1, D2, D3, CLK pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12; 	//PC8,9,10,11,12复用功能输出	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*!< Configure PD.02 CMD line */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	/* 引脚复用映射设置 */
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_SDIO);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_SDIO);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_SDIO);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_SDIO);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_SDIO);	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource2,GPIO_AF_SDIO);
	
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SDIO, DISABLE);
	SDIO_Register_Deinit();
	
	/* SDIO_CK = SDIOCLK/[clkdiv+2], 其中SDIOCLK固定为48Mhz */
	SDIO_InitStructure.SDIO_ClockDiv = 238;      //48MHz/(238+2)=200KHz 
	SDIO_InitStructure.SDIO_ClockEdge = SDIO_ClockEdge_Rising;
	SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
	SDIO_InitStructure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
	SDIO_InitStructure.SDIO_BusWide = SDIO_BusWide_1b;
	SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
	SDIO_Init(&SDIO_InitStructure);
	
	SDIO_SetPowerState(SDIO_PowerState_ON);
//	SDIO_ClockCmd(ENABLE);
	SDIO->CLKCR|=1<<8;       //SDIOCK使能
	#endif
	return 0;
}

void sdio_clk_open(void)
{
	SDIO_SetPowerState(SDIO_PowerState_ON);
//	SDIO_ClockCmd(ENABLE);
}

void sdio_clk_close(void)
{
	SDIO_SetPowerState(SDIO_PowerState_OFF);
//	SDIO_ClockCmd(DISABLE);	
}

//读取SDIO D0数据线电平
int sdio_dat0_read(void)
{
	return (GPIOC->IDR & GPIO_Pin_8)? 1 : 0;
}

void sdio_dat123_release(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC, GPIO_InitStructure.GPIO_Pin);
}

/** @brief 设置SDIO Controller的时钟与总线位宽
  * @param ClockDiv: 0-255
  *        ClockEdge: SDIO_ClockEdge_Rising, SDIO_ClockEdge_Falling
  *        BusWide:   SDIO_BusWide_1b, SDIO_BusWide_4b, SDIO_BusWide_8b
  *        ClockPowerSave: SDIO_ClockPowerSave_Disable, SDIO_ClockPowerSave_Enable
  */
void sdio_controller_config(uint8_t ClockDiv, uint32_t ClockEdge, uint32_t BusWide, uint32_t ClockPowerSave)
{
	SDIO_InitTypeDef  SDIO_InitStructure;

	SDIO_InitStructure.SDIO_ClockDiv = ClockDiv;
	SDIO_InitStructure.SDIO_ClockEdge = ClockEdge;
	SDIO_InitStructure.SDIO_ClockBypass = SDIO_ClockBypass_Disable;
	SDIO_InitStructure.SDIO_ClockPowerSave = ClockPowerSave;
	SDIO_InitStructure.SDIO_BusWide = BusWide;
	SDIO_InitStructure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
	SDIO_Init(&SDIO_InitStructure);	
}

/** @brief SDIO Controller省电配置
  *        为了省电, 当总线为空闲时, 设置PWRSAV位可以关闭SDIO_CK时钟输出
  * @param state: DISABLE--关闭省电模式, 始终输出SDIO_CK
  *               ENABLE --开启省电模式, 仅在有总线活动时才输出SDIO_CK
  */
void sdio_controller_power_save(FunctionalState state)
{
	SDIO->CLKCR &= ~((uint32_t)0x100);  //Clear Clock enable bit

	SDIO->CLKCR &= ~((uint32_t)0x200);  //Clear Power saving configuration bit
	if(state == ENABLE)
		SDIO->CLKCR |= ((uint32_t)0x200);

	SDIO->CLKCR |= ((uint32_t)0x100);   //Set Clock enable bit
}

/** @brief 设置SDIO Controller中断使能/失能
  */
void sdio_controller_irq_set(FunctionalState new_state)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;			   //SDIO中断配置
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //抢占优先级0 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		   //子优先级0 
	NVIC_InitStructure.NVIC_IRQChannelCmd = new_state;		   //使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);
}

/** @brief SDIO DMA配置
  * @param mem_buff -传输数据的内存地址
  *        trans_len-传输数据的长度(单位 字节)
  *        trans_dir-传输方向  DMA_DIR_MemoryToPeripheral, DMA_DIR_PeripheralToMemory
  */
void sdio_dma_config(void *mem_buff, uint32_t trans_len, uint32_t trans_dir)
{
	DMA_InitTypeDef DMA_InitStructure;
	  
	DMA_DeInit(DMA2_Stream6);                         //将DMA2的通道4寄存器重设为缺省值
//	DMA_Cmd(DMA2_Stream6, DISABLE);                   //关闭DMA
	while(DMA_GetCmdStatus(DMA2_Stream6) != DISABLE)  //Necessary
	{	}

	DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6|DMA_FLAG_HTIF6|DMA_FLAG_TEIF6);

	DMA_InitStructure.DMA_Channel = DMA_Channel_4;                       //DMA传输触发源 SDIO
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SDIO->FIFO;    //DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)mem_buff;          //DMA内存基地址
	DMA_InitStructure.DMA_DIR = trans_dir;                               //数据传输方向
	DMA_InitStructure.DMA_BufferSize = trans_len/4;                      //使用外设流控时硬件强制为0xFFFF, 这里可设置为任意值
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;     //外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;              //内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;//数据宽度为32位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;        //数据宽度为32位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                         // 使用普通模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                 //中等优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_INC4;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;  //外设突发单次传输
	DMA_Init(DMA2_Stream6, &DMA_InitStructure);     //初始化DMA Stream
	
	//STM32F4只有SDIO外设DMA传输才能使用外设流控, 其余外设DMA传输时都是DMA流控. 
	//当使用外设流控时DMAy_Streamx->NDTR被硬件强制为0xFFFF, 流控的外设来控制何时停止DMA传输
	DMA_FlowControllerConfig(DMA2_Stream6, DMA_FlowCtrl_Peripheral);

	DMA_Cmd(DMA2_Stream6, ENABLE);                 //使能DMA传输通道
	
//	my_printf("[%s] %u\n\r", __FUNCTION__, DMA2_Stream6->NDTR);
}











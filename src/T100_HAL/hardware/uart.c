#include "uart.h"

void uart1_init(uint32_t bps)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	USART_DeInit(USART1);
	
	//串口1对应引脚复用映射(必须再管脚配置之前就开启映射,否则会丢失第一字节数据)
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1); 
	
	//USART1端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9，PA10

   //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bps;                  //波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;     //一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;        //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART1, &USART_InitStructure);                       //初始化串口1

	//Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;             
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	/* Enable USART1 Receive and Transmit interrupts */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);   //初始化TX不使能TX中断
	
	/* Enable the USARTx */
    USART_Cmd(USART1, ENABLE);
	USART_ClearFlag(USART1, USART_FLAG_TC);
}


//===========================================================================================
void USART1_IRQHandler(void)
{
	volatile uint8_t ch;
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE))
	{
		ch = USART1->DR;

		//deal receive data
	}
}

void put_char(int c)
{
	USART1->DR = (c & (uint16_t)0x01FF);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
}

int get_char(void)
{
	return 0;
}

int  uart1_put_char(int ch)
{
	USART1->DR = (ch & (uint16_t)0x01FF);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);	
	return 1;
}

int  uart1_put_buff(void *buff, int len)
{
	char *ch = buff;
	int i = 0;
	for(i=0; i<len; i++)
	{
		USART1->DR = (ch[i] & (uint16_t)0x01FF);
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET)
		{	;	}
	}
	return i;
}

/*
 * @brief: UART1数据发送函数
 */
int uart1_write(void *buff, int len)
{
	char *ch = buff;
	int i = 0;

	for(i=0; i<len; i++)
	{
		USART1->DR = (ch[i] & (uint16_t)0x01FF);
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET)
		{	;	}
	}

	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET)     //等待数据从移位寄存器确实发送出去
	{	;	}

	return i;
}


//==============================================================================

void uart3_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	
	//串口3对应引脚复用映射(必须再管脚配置之前就开启映射,否则会丢失第一字节数据)
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3); 
	
	//USART1端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; //PB10与PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化PB10 PB11
	
   //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = 460800;   //波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;   //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART3, &USART_InitStructure); //初始化串口3
	
	//Enable the USARTx
    USART_Cmd(USART3, ENABLE);
	USART_ClearFlag(USART3, USART_FLAG_TC);
}


/*
 * @brief: UART3数据发送函数
 */
int uart3_write(void *buff, int len)
{
	char *ch = buff;
	int i = 0;

	for(i=0; i<len; i++)
	{
		USART3->DR = (ch[i] & (uint16_t)0x01FF);
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) != SET)
		{	;	}
	}

	return i;
}




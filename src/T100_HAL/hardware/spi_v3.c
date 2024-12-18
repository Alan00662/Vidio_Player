#include "spi_v3.h"
#include "share.h"
#include "systick.h"
#include <stddef.h>

/*
 * @brief: spi controller config as master
 */
void spi_controller_init(SPI_TypeDef* SPIx)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	
	if(SPIx == SPI1) {
//		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); //使能GPIOA时钟
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

		//PA5--SPI1_CLK  PA6-SPI1_MISO  PA7-SPI1_MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		
		//SPI1对应引脚复用映射
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1);
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1);
		GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1);
		
		/* 把挂接在此总线上的所有从设备的片选拉高(必须, 非常必要) */
//		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;         //LCD_CS
//		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//		GPIO_Init(GPIOA, &GPIO_InitStructure);
//		GPIO_SetBits(GPIOA, GPIO_Pin_2);
	}
	if(SPIx == SPI3) {
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

		//PB3--SPI3_CLK  PB4-SPI3_MISO  PB5-SPI3_MOSI
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOB, &GPIO_InitStructure);

		//SPI3对应引脚复用映射
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource3, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource4, GPIO_AF_SPI3);
		GPIO_PinAFConfig(GPIOB,GPIO_PinSource5, GPIO_AF_SPI3);

		/* 把挂接在此总线上的所有从设备的片选拉高(必须, 非常必要) */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;         //SPI_FLASH_CS
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
		GPIO_SetBits(GPIOB, GPIO_Pin_14);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;         //NRF24L01 CS
		GPIO_Init(GPIOG, &GPIO_InitStructure);
		GPIO_SetBits(GPIOG, GPIO_Pin_7);
	}
	
	SPI_I2S_DeInit(SPIx);  //复位SPIx Controller的所有寄存器
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPIx, &SPI_InitStructure);
	SPI_Cmd(SPIx, ENABLE);

	spi_write_read_byte(SPIx, 0xFF); //dummy send
	if(SPIx == SPI1) {
		SPI1_DMA_Init();
	}
	if(SPIx == SPI3) {
		SPI3_DMA_Init();
	}
}

/*
 * @brief: SPIx数据位宽切换为8bit
 * 使用 SPI_DataSizeConfig(SPI1, SPI_DataSize_8b)
 */
void spi_set_data_8bit(SPI_TypeDef* SPIx)
{
	SPIx->CR1 &= 0xF7BF;            //clear bit11 and bit6
	SPIx->CR1 |= SPI_DataSize_8b;
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPIx	
}

/*
 * @brief: SPIx数据位宽切换为16bit
 * 使用 SPI_DataSizeConfig(SPI1, SPI_DataSize_16b)
 */
void spi_set_data_16bit(SPI_TypeDef* SPIx)
{
	SPIx->CR1 &= 0xF7BF;            //clear bit11 and bit6
	SPIx->CR1 |= SPI_DataSize_16b;
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPIx
}

/* 工作频率不同、时钟极性不同的SPI设备挂接在同一总线上, 访问不同的设备前要
 * 先切换频率与时钟极性. 这样不仅很恶心, 而且导致效率低, 设计PCB时要尽量避免.
 */

//SPI频率设置, prescaler取值:
//SPI_BaudRatePrescaler_2    (SPI 42M @sys 84M)
//SPI_BaudRatePrescaler_4    (SPI 21M @sys 84M)
//SPI_BaudRatePrescaler_8    (SPI 10.5M @sys 84M)
//SPI_BaudRatePrescaler_16   (SPI 5.25M @sys 84M)
//SPI_BaudRatePrescaler_32   (SPI 2.625M @sys 84M)
//SPI_BaudRatePrescaler_64   (SPI 1.3125M @sys 84M)
//SPI_BaudRatePrescaler_128  (SPI 656.25K @sys 84M)
//SPI_BaudRatePrescaler_256  (SPI 328.125K @sys 84M)
void spi_set_clkpres(SPI_TypeDef* SPIx, uint16_t prescaler)
{
	SPIx->CR1 &= 0xFF87;            //clear bit3-bit6
	SPIx->CR1 |= (uint16_t)prescaler;
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPIx
}


//SPI时钟极性与相位设置
//SPI_CPOL: SPI_CPOL_Low, SPI_CPOL_High
//SPI_CPHA: SPI_CPHA_1Edge, SPI_CPHA_2Edge
void spi_set_cpol(SPI_TypeDef* SPIx, uint16_t SPI_CPOL, uint16_t SPI_CPHA)
{
	SPIx->CR1 &= (uint16_t)~(0x0003|0x0040);
	SPIx->CR1 |= (SPI_CPOL|SPI_CPHA);
	SPIx->CR1 |= (uint16_t)0x0040;  //Enable SPI
}

//SPI Controller上会挂接不同的设备, 有时在通信前需要配置Controller的
//频率 数据位宽 时钟极性与相位等信息, 通信结束后要把Controller恢复为
//之前的设置, 于是就设计了这2个获取与恢复Controller配置的函数
int spi_get_config(SPI_TypeDef* SPIx)
{
	return SPIx->CR1;
}

void spi_recover_config(SPI_TypeDef* SPIx, int cr)
{
	SPIx->CR1 = cr;
}

//SPI 收发原理:(对编程非常重要, XX应该是环形移位寄存器, 用户不可操作但Controller硬件可以操作)
//写数据寄存器DR, 会设置SPI_I2S_FLAG_TXE=0(非空), 待Controller硬件把DR中的数据转移到XX, 同时设置SPI_I2S_FLAG_TXE=1(空)
//特别特别特别需要注意的是, SPI_I2S_FLAG_TXE=1(空)并不等于Controller已经把数据已经发送出去了,
//更不等于Controller已经接受到了一个新的数据.
//待Controller硬件把XX中的数据真正发送出去后,也就完成了一个数据的接收, 接收到的数据应该也在XX,
//然后Controller硬件把XX中的数据放DR, 同时设置SPI_I2S_FLAG_RXNE=1(非空), 这时
//读数据寄存器DR, 会拿到接收到的数据, 同时会设置SPI_I2S_FLAG_RXNE=0(空).

//利用DMA发送数据后, 会造成SPI_I2S_FLAG_RXNE=1(非空), 必须通过读DR来清除此标志, 否则利用非DMA
//发送数据会丢失第1字节的数据.

/*
 * @brief: SPIx 收发函数
 *    ret: <0 收发失败;  >=0 收发成功
 */
int spi_write_read_byte(SPI_TypeDef* SPIx, int txdat)
{
	int tryCount = 0;

	SPIx->DR = (uint16_t)txdat;   //发送数据
	while((SPIx->SR & SPI_I2S_FLAG_TXE) == 0)               //等待发送完成
	{                             //0-发送缓冲区非空  1-发送缓冲区为空
		if(++tryCount > 0xFF){
			return -1;            //发送失败 
		}
	}

	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET) //等待接收完成
	{                           //0-接收缓冲区为空  1-接收缓冲区非空
		if(++tryCount > 0x1FF) {
			return -2;          //接收失败
		}
	}
	return((uint16_t)SPIx->DR);  //返回接收数据		
}

//把上面一个函数拆成如下2个函数
int spi_write_byte(SPI_TypeDef* SPIx, int txdat)
{
	int recv;

	SPIx->DR = (uint16_t)txdat;   //Write DR register
	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == 0); //不能是SPI_I2S_FLAG_TXE, Wait DR->XX->MOSI_Pin MISO_Pin->XX->DR
	recv = SPIx->DR;             //读数据寄存器, 清除SPI_I2S_FLAG_RXNE标志位(必须)
	return recv;
}

uint16_t spi_read_byte(SPI_TypeDef* SPIx)
{
	uint16_t dummy = 0x0000;
	if((SPIx->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;
	
	SPIx->DR = dummy;   //发送dummy数据
	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == 0);
	return (uint16_t)SPIx->DR;    //读数据寄存器, 清除SPI_I2S_FLAG_RXNE标志位(必须)
}

/*
 * @brief: SPI连续发送数据
 */
int spi_write_buff(SPI_TypeDef* SPIx, uint8_t *buff, int len)
{
	volatile int dummy;
	int i;
	
	for(i=0; i<len; i++)
	{
		SPIx->DR = buff[i];      //Write DR register
		while((SPIx->SR & SPI_I2S_FLAG_RXNE) == 0); //不能是SPI_I2S_FLAG_TXE
		dummy = SPIx->DR;        //读数据寄存器, 清除SPI_I2S_FLAG_RXNE标志位(必须)
	}
	return i;
}

/*
 * @brief: SPI连续读取数据
 */
int spi_read_buff(SPI_TypeDef* SPIx, uint8_t *buff, int len)
{
	int i;
	uint16_t dummy;
	
	if((SPIx->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;
	else
		dummy = 0x0000;
	for(i=0; i<len; i++)
	{
		SPIx->DR = dummy;    //发送dummy数据
		while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
		buff[i] = SPIx->DR;  //返回接收数据		
	}
	return i;
}


//=================================================================================
void SPI1_DMA_Init(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2时钟使能
	
	DMA_DeInit(DMA2_Stream3);
//	DMA_Cmd(DMA2_Stream3, DISABLE);                   //关闭DMA
	while(DMA_GetCmdStatus(DMA2_Stream3) != DISABLE)  //Necessary
	{	}

	DMA_InitStructure.DMA_Channel = DMA_Channel_3;                       //DMA传输触发源 SPI1_TX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;      //DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = 0x20000000;                  //DMA存储器0地址   dummy
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;              //memory-->外设
	DMA_InitStructure.DMA_BufferSize = 0;                                //数据传输量        dummy
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;     //外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;              //存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据长度:8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        //存储器数据长度:8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                          //使用普通模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;                     //低优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  //外设突发单次传输
	DMA_Init(DMA2_Stream3, &DMA_InitStructure);     //初始化DMA Stream

	DMA_Cmd(DMA2_Stream3, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);  //需要时再打开   

	/* DMA RX 配置 */
	DMA_DeInit(DMA2_Stream0);
	while(DMA_GetCmdStatus(DMA2_Stream0) != DISABLE)  //Necessary
	{	}
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;                       //DMA传输触发源 SPI1_RX
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;              //外设-->memory
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);

	DMA_Cmd(DMA2_Stream0, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);  //需要时再打开
}

/*
 * @brief 设置SPI1 DMA收发数据时的数据位宽
 * @param 8 -8bits
 *        16-16bits
 *        32-32bits
 */
void spi1_dma_data_size(int size)
{
	uint32_t memory_data_size;
	uint32_t peripheral_data_size;
	
	switch(size)
	{
	case 8:
		memory_data_size = DMA_MemoryDataSize_Byte;
		peripheral_data_size = DMA_PeripheralDataSize_Byte;
		break;
	
	case 16:
		memory_data_size = DMA_MemoryDataSize_HalfWord;
		peripheral_data_size = DMA_PeripheralDataSize_HalfWord;
		break;
	
	default:
		memory_data_size = DMA_MemoryDataSize_Word;
		peripheral_data_size = DMA_PeripheralDataSize_Word;
		break;
	}
	
	int flag;
	//DMA2_Stream3 发送通道
	flag = 0;
	if(DMA2_Stream3->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA2_Stream3->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA2_Stream3->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA2_Stream3->CR |= DMA_SxCR_EN;
	
	//DMA2_Stream0接收通道
	flag = 0;
	if(DMA2_Stream0->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA2_Stream0->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA2_Stream0->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA2_Stream0->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA2_Stream0->CR |= DMA_SxCR_EN;
}

//mem_inc 0-内存地址不递增  1-内存地址递增
int spi1_write_dma(void *buff, int len, int mem_inc)
{
	int tmp_len;
	int offset = 0;
	int ret = len;

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE); 

	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit 
	if( mem_inc )
		DMA2_Stream3->CR |= DMA_SxCR_MINC;
	
	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);

		DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;    //Stop DMA2_Stream7 transfer 
		DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
		
		if( mem_inc ) {
			DMA2_Stream3->M0AR = (uint32_t)buff + offset;
			offset += tmp_len;
		} else {
			DMA2_Stream3->M0AR = (uint32_t)buff;
		}
		DMA2_Stream3->NDTR = tmp_len;

		DMA2_Stream3->CR |= DMA_SxCR_EN;                //Start DMA2_Stream3 transfer

		while(DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET); //等待传输完成
		DMA_ClearFlag(DMA2_Stream3,DMA_FLAG_TCIF3);                      //清除DMA2_Steam3传输完成标志
		while((SPI1->SR & SPI_I2S_FLAG_BSY) != 0);                       //解决STM32DMA发送最后一字节丢失的问题

		len -= tmp_len;
	}

	DMA_Cmd(DMA2_Stream3, DISABLE);
	DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);

	volatile int dummy = SPI1->DR;      //Clear SPI_I2S_FLAG_RXNE Flag, 解决立即用非DMA方式发送数据会丢失1字节的问题

	return ret;
}

/*
 * @brief: 无法单纯的实现接收, 必须依靠发送来实现接收
 */
int spi1_read_dma(void *buff, int len)
{
	int tmp_len;
	int offset = 0;
	int ret = len; 
	int dummy = 0;

	if((SPI1->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);  //开启DMA传输触发源开关
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit
	DMA2_Stream0->CR &= ~(uint32_t)DMA_SxCR_EN;
	DMA2_Stream0->CR |= (uint32_t)DMA_SxCR_MINC;

	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);
 
		DMA2_Stream3->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA2_Stream0->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
		DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0|DMA_FLAG_HTIF0|DMA_FLAG_TEIF0);
		
		DMA2_Stream0->M0AR = (uint32_t)buff + offset;
		DMA2_Stream0->NDTR = tmp_len;
	
		DMA2_Stream3->M0AR = (uint32_t)&dummy;
		DMA2_Stream3->NDTR = tmp_len;

		DMA2_Stream0->CR |= DMA_SxCR_EN;
		DMA2_Stream3->CR |= DMA_SxCR_EN;
		
		while(DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET); //等待传输完成
		DMA_ClearFlag(DMA2_Stream3,DMA_FLAG_TCIF3);                      //清除DMA2_Steam3传输完成标志
		
		while(DMA_GetFlagStatus(DMA2_Stream0, DMA_FLAG_TCIF0) == RESET); //等待传输完成
		DMA_ClearFlag(DMA2_Stream0,DMA_FLAG_TCIF0);                      //清除DMA2_Steam0传输完成标志

		while((SPI1->SR & SPI_I2S_FLAG_BSY) != 0);        //必须

		len -= tmp_len;
		offset += tmp_len;
	}
	DMA_Cmd(DMA2_Stream3, DISABLE);
	DMA_Cmd(DMA2_Stream0, DISABLE);
	DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3);
	DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0|DMA_FLAG_HTIF0|DMA_FLAG_TEIF0);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);  //关闭DMA传输触发源开关
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);

	return ret;	
}

//=================================================================================
void SPI3_DMA_Init(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);//DMA1时钟使能
	
	DMA_DeInit(DMA1_Stream7);
//	DMA_Cmd(DMA1_Stream7, DISABLE);                   //关闭DMA
	while(DMA_GetCmdStatus(DMA1_Stream7) != DISABLE)  //Necessary
	{	}

	DMA_InitStructure.DMA_Channel = DMA_Channel_0;                       //DMA传输触发源 SPI3_TX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI3->DR;      //DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = 0x20000000;                  //DMA存储器0地址   dummy
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;              //memory-->外设
	DMA_InitStructure.DMA_BufferSize = 0;                                //数据传输量        dummy
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;     //外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;              //存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//外设数据长度:8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        //存储器数据长度:8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                         //使用普通模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;                    //低优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  //外设突发单次传输
	DMA_Init(DMA1_Stream7, &DMA_InitStructure);     //初始化DMA Stream

	DMA_Cmd(DMA1_Stream7, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, DISABLE);  //需要时再打开   

	/* DMA RX 配置 */
	DMA_DeInit(DMA1_Stream2);
	while(DMA_GetCmdStatus(DMA1_Stream2) != DISABLE)  //Necessary
	{	}
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;                       //DMA传输触发源 SPI3_RX
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;              //外设-->memory
	DMA_Init(DMA1_Stream2, &DMA_InitStructure);

	DMA_Cmd(DMA1_Stream2, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, DISABLE);  //需要时再打开
}

/*
 * @brief 设置SPI3 DMA收发数据时的数据位宽
 * @param 8 -8bits
 *        16-16bits
 *        32-32bits
 */
void spi3_dma_data_size(int size)
{
	uint32_t memory_data_size;
	uint32_t peripheral_data_size;
	
	switch(size)
	{
	case 8:
		memory_data_size = DMA_MemoryDataSize_Byte;
		peripheral_data_size = DMA_PeripheralDataSize_Byte;
		break;
	
	case 16:
		memory_data_size = DMA_MemoryDataSize_HalfWord;
		peripheral_data_size = DMA_PeripheralDataSize_HalfWord;
		break;
	
	default:
		memory_data_size = DMA_MemoryDataSize_Word;
		peripheral_data_size = DMA_PeripheralDataSize_Word;
		break;
	}

	int flag;
	//DMA1_Stream7 发送通道
	flag = 0;
	if(DMA1_Stream7->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA1_Stream7->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA1_Stream7->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA1_Stream7->CR |= DMA_SxCR_EN;
	
	//DMA1_Stream2接收通道
	flag = 0;
	if(DMA1_Stream2->CR & DMA_SxCR_EN)        //设置之前, 必须把EN位写0
	{
		flag = 1;
		DMA1_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;
	}
	DMA1_Stream2->CR &= 0xFFFF87FF;     //清除 11 12 13 14位
	DMA1_Stream2->CR |= memory_data_size|peripheral_data_size;
	if(	flag )
		DMA1_Stream2->CR |= DMA_SxCR_EN;
}

//mem_inc 0-内存地址不递增  1-内存地址递增
int spi3_write_dma(void *buff, int len, int mem_inc)
{
	int tmp_len;
	int offset = 0;
	int ret = len;

	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE); 

	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit 
	if( mem_inc )
		DMA1_Stream7->CR |= DMA_SxCR_MINC;
	
	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);

		DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;    //Stop DMA1_Stream7 transfer 
		DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
		
		if( mem_inc ) {
			DMA1_Stream7->M0AR = (uint32_t)buff + offset;
			offset += tmp_len;
		} else {
			DMA1_Stream7->M0AR = (uint32_t)buff;
		}
		DMA1_Stream7->NDTR = tmp_len;

		DMA1_Stream7->CR |= DMA_SxCR_EN;                //Start DMA1_Stream7 transfer

		while(DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TCIF7) == RESET); //等待传输完成
		DMA_ClearFlag(DMA1_Stream7,DMA_FLAG_TCIF7);                      //清除DMA1_Stream7传输完成标志
		while((SPI3->SR & SPI_I2S_FLAG_BSY) != 0);                       //解决STM32DMA发送最后一字节丢失的问题

		len -= tmp_len;
	}

	DMA_Cmd(DMA1_Stream7, DISABLE);
	DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, DISABLE);

	volatile int dummy = SPI3->DR;      //Clear SPI_I2S_FLAG_RXNE Flag, 解决立即用非DMA方式发送数据会丢失1字节的问题

	return ret;
}

/*
 * @brief: 无法单纯的实现接收, 必须依靠发送来实现接收
 */
int spi3_read_dma(void *buff, int len)
{
	int tmp_len;
	int offset = 0;
	int ret = len; 
	int dummy = 0;

	if((SPI3->CR1 & 0x0002) != 0)   //空闲时CLK为高电平
		dummy = 0xFFFF;

	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, ENABLE);  //开启DMA传输触发源开关
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);

	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;    //EN位为0,该寄存器的其他位才允许设置
	DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_MINC;  //Clear mem_inc Control bit
	DMA1_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;
	DMA1_Stream2->CR |= (uint32_t)DMA_SxCR_MINC;

	/* 数据量超过64K时要循环多次发送 */
	while(len > 0)
	{
		tmp_len = min(len, 0xFFFF);
 
		DMA1_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA1_Stream2->CR &= ~(uint32_t)DMA_SxCR_EN;
		DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
		DMA_ClearFlag(DMA1_Stream2, DMA_FLAG_TCIF2|DMA_FLAG_HTIF2|DMA_FLAG_TEIF2);

		DMA1_Stream2->M0AR = (uint32_t)buff + offset;
		DMA1_Stream2->NDTR = tmp_len;
	
		DMA1_Stream7->M0AR = (uint32_t)&dummy;
		DMA1_Stream7->NDTR = tmp_len;

		DMA1_Stream2->CR |= DMA_SxCR_EN;
		DMA1_Stream7->CR |= DMA_SxCR_EN;
		
		while(DMA_GetFlagStatus(DMA1_Stream7, DMA_FLAG_TCIF7) == RESET); //等待传输完成
		DMA_ClearFlag(DMA1_Stream7,DMA_FLAG_TCIF7);                      //清除DMA1_Stream7传输完成标志
		
		while(DMA_GetFlagStatus(DMA1_Stream2, DMA_FLAG_TCIF2) == RESET); //等待传输完成
		DMA_ClearFlag(DMA1_Stream2,DMA_FLAG_TCIF2);                      //清除DMA1_Steam3传输完成标志

		while((SPI3->SR & SPI_I2S_FLAG_BSY) != 0);        //必须

		len -= tmp_len;
		offset += tmp_len;
	}
	DMA_Cmd(DMA1_Stream7, DISABLE);
	DMA_Cmd(DMA1_Stream2, DISABLE);
	DMA_ClearFlag(DMA1_Stream7, DMA_FLAG_TCIF7|DMA_FLAG_HTIF7|DMA_FLAG_TEIF7);
	DMA_ClearFlag(DMA1_Stream2, DMA_FLAG_TCIF2|DMA_FLAG_HTIF2|DMA_FLAG_TEIF2);
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, DISABLE);  //关闭DMA传输触发源开关
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Rx, DISABLE);

	return ret;	
}


//==========================================================================================
//I2S Controller 

void i2s_controller_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	DMA_InitTypeDef   DMA_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	
	//=========================管脚配置===============================
	//PB12--I2S2_LRCK  PB13--I2S2_SCLK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
	//PC2--I2S2_SDIN  PC3--I2S2_SDOUT  PC6--I2S2_MCLK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
		
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource12,GPIO_AF_SPI2);  //PB12,AF5  I2S_LRCK
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource13,GPIO_AF_SPI2);	//PB13,AF5  I2S_SCLK 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource3,GPIO_AF_SPI2);	//PC3, AF5  I2S_DACDATA 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_SPI2);	//PC6, AF5  I2S_MCK
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource2,GPIO_AF6_SPI2);	//PC2, AF6  I2S_ADCDATA  I2S2ext_SD是AF6!!!

	//===========================================================
	//采用循环方式发送音频数据测试(要进行发送缓冲区非空检查), Not Necessary
//	i2s_controller_cfg(I2S_Mode_MasterTx, I2S_Standard_Phillips, I2S_DataFormat_16b, 8000);
//	SPI_I2S_SendData(SPI2, 0x1122);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);  //不进行发送缓冲区非空检查会发送失败
//	SPI_I2S_SendData(SPI2, 0x3344);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);	
//	SPI_I2S_SendData(SPI2, 0x5566);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);	
//	SPI_I2S_SendData(SPI2, 0x7788);
//	while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);

	//=========================DMA配置===============================
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);  //DMA1时钟使能
	
	//I2S发送通道
	DMA_DeInit(DMA1_Stream4);

	DMA_InitStructure.DMA_Channel = DMA_Channel_0;                       //DMA传输触发源 SPI2_TX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;      //DMA外设地址
	DMA_InitStructure.DMA_Memory0BaseAddr = 0x20000000;                  //DMA存储器0地址   dummy
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;              //memory-->外设
	DMA_InitStructure.DMA_BufferSize = 64;                               //数据传输量        dummy
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;    //外设非增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;             //存储器增量模式
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设数据长度:16位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;        //存储器数据长度:16位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;                       //DMA_Mode_Normal
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;               // 非常高优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  //外设突发单次传输
	DMA_Init(DMA1_Stream4, &DMA_InitStructure);     //初始化DMA Stream
	
	DMA_DoubleBufferModeConfig(DMA1_Stream4,0x20008000, DMA_Memory_0);   //双缓冲模式配置
	DMA_DoubleBufferModeCmd(DMA1_Stream4, ENABLE);                       //双缓冲模式开启

	DMA_Cmd(DMA1_Stream4, DISABLE);                    //需要时再使能DMA
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);  //需要时再打开
	
	//I2S接收通道
	DMA_DeInit(DMA1_Stream3);
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;                       //DMA传输触发源 I2S2_EXT_RX
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2S2ext->DR;   //DMA外设地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;              //外设-->memory
	DMA_Init(DMA1_Stream3, &DMA_InitStructure);     //初始化DMA Stream
	
	DMA_DoubleBufferModeConfig(DMA1_Stream3,0x2000C000, DMA_Memory_0);   //双缓冲模式配置
	DMA_DoubleBufferModeCmd(DMA1_Stream3, ENABLE);                       //双缓冲模式开启
	
	DMA_Cmd(DMA1_Stream3, DISABLE);                      //需要时再使能DMA
	SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, DISABLE); //需要时再打开 
	
	//=========================DMA中断配置===============================
	//DMA发送完成中断
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;        //响应优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);	

	DMA_ITConfig(DMA1_Stream4, DMA_IT_TC, ENABLE);    //开启DMA传输完成与传输错误中断
	DMA_ITConfig(DMA1_Stream4, DMA_IT_TE, ENABLE);
	
	//DMA接收完成中断
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream3_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //响应优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);	

	DMA_ITConfig(DMA1_Stream3, DMA_IT_TC, ENABLE);    //开启DMA传输完成与传输错误中断
	DMA_ITConfig(DMA1_Stream3, DMA_IT_TE, ENABLE);
}


//音频采样频率与I2S总线接口几个时钟频率的关系: 
//I2S_LRCK = 音频的采样频率
//I2S_MCK  = 256*I2S_LRCK (固定是这样, 有的硬件是384*I2S_LRCK)
//I2S_SCLK = 2*I2S_LRCK*采样分辨率 (音频数据无扩展时是这样)
//I2S_SCLK = 2*I2S_LRCK*采样分辨率*2 16位或者24位音频数据扩展成32位传输)

/** 采样率计算公式:Fs =I2SxCLK/[256*(2*I2SDIV+ODD)]
 ** I2SxCLK = (HSE/pllm)* PLLI2SN / PLLI2SR 
 ** HSE=8Mhz, pllm=8, 在system_stm32f4xx.c文件中配置系统时钟时确定
 ** PLLI2SN∈[192, 432]  PLLI2SR∈[2, 7]
 ** I2SDIV∈[2, 255]   ODD 0/1
 ** 表格式:采样率/10,PLLI2SN,PLLI2SR,I2SDIV,ODD
 **/
/*
static const uint16_t i2s_clk_tab[][5]=
{
	{800 ,256,5,12,1},		//8Khz采样率
	{1102,429,4,19,0},		//11.025Khz采样率 
	{1600,213,2,13,0},		//16Khz采样率
	{2205,429,4, 9,1},		//22.05Khz采样率
	{3200,213,2, 6,1},		//32Khz采样率
	{4410,271,2, 6,0},		//44.1Khz采样率
	{4800,258,3, 3,1},		//48Khz采样率
	{8820,316,2, 3,1},		//88.2Khz采样率
	{9600,344,2, 3,1},  	//96Khz采样率
	{17640,361,2,2,0},  	//176.4Khz采样率 
	{19200,393,2,2,0},  	//192Khz采样率
};
*/

//取代上面的表格, 根据目标频率自动计算4个参数
static int auto_cal_reg_param(uint32_t tar_freq, uint32_t *reg_param, uint32_t param_size)
{
	if(param_size < 4*sizeof(reg_param[0]))
		return -1;
	
	uint32_t record_deta = 0xFFFFFFFF;
	uint32_t pll_n, pll_r, div;
	uint32_t tmp_freq, tmp_deta;
	uint32_t cnt;

	//执行时间不超过800us
	for(pll_n=192; pll_n<=432; pll_n++)
	{
		for(pll_r=2; pll_r<=7; pll_r++)
		{
			div = pll_n*1000000/pll_r/256/tar_freq;
			cnt = 2;       //误差最小的值可能在div处, 也可能在div+1处, 所以此处要循环2次
			do
			{
				if(div>=4 && div<=511)
				{
					tmp_freq = pll_n*1000000/pll_r/256/div;
					if(tmp_freq > tar_freq)
						tmp_deta = tmp_freq - tar_freq;
					else
						tmp_deta = tar_freq - tmp_freq;
					
					if(tmp_deta < record_deta)
					{
						record_deta = tmp_deta;
						reg_param[0] = pll_n;
						reg_param[1] = pll_r;
						reg_param[2] = div/2;
						reg_param[3] = div%2;
						if(record_deta == 0)
							return 0;
					}
				}
				div++;
				cnt--;
			} while(cnt > 0);			
		}
	}

	if(record_deta == 0xFFFFFFFF)
		return -2;
	
	return record_deta;
}

/** @brief 配置I2S Controller的相关属性
 **
 **/
int i2s_controller_cfg(uint16_t I2S_Standard, uint16_t I2S_DataFormat, uint32_t sample_rate)
{
	I2S_InitTypeDef I2S_InitStructure;
	
	I2S_Cmd(SPI2, DISABLE);
	I2S_Cmd(I2S2ext, DISABLE);
	SPI_I2S_DeInit(SPI2);

	//====================配置Controller的属性====================
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
	I2S_InitStructure.I2S_Standard = I2S_Standard;
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat;
	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;  //在下面通过查表配置
	I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_Default;    //在下面通过查表配置
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;                  //SCLK在空闲时为低电平
	I2S_Init(SPI2, &I2S_InitStructure); 
	
	I2S_InitStructure.I2S_Mode = I2S_Mode_SlaveTx;
	I2S_FullDuplexConfig(I2S2ext, &I2S_InitStructure);
	
	//========配置PLL输出的I2SCLK & Controller的分频系数==========
	int ret = i2s_frequency_adjust(sample_rate);;
	if(ret < 0)
		return -1;
	
	/* 最后使能I2S2 COntroller */
	I2S_Cmd(SPI2, ENABLE);
	I2S_Cmd(I2S2ext, ENABLE);
	
	return 0;
}

/** @brief 改变I2S 音频的采样频率
  * @param new_freq 新的频率
  * @reval 0-成功 非零-失败
  */
int i2s_frequency_adjust(uint32_t new_freq)
{
	//====================配置PLL输出的I2SCLK====================
	uint32_t reg_param[4];
	int ret = auto_cal_reg_param(new_freq, reg_param, sizeof(reg_param));
	if(ret < 0)
		return -1;

	/* 先关闭PLLI2S */
	RCC->CR &= ~(uint32_t)RCC_CR_PLLI2SON;  //RCC_PLLI2SCmd(DISABLE);	

	/* 设置PLL的N R */
	//PLLI2SN∈[192, 432]  PLLI2SR∈[2, 7]
	RCC_PLLI2SConfig(reg_param[0], reg_param[1]);
	
	/* 开启PLLI2S */
	RCC->CR |= RCC_CR_PLLI2SON;  //RCC_PLLI2SCmd(ENABLE);
	
	/* Wait till the main PLL is ready */
    while((RCC->CR & RCC_CR_PLLI2SRDY) == 0)
    {
    }
	
	//====================配置Controller的分频系数====================
	SPI2->I2SPR = I2S_MCLKOutput_Enable| (reg_param[3]<<8) | reg_param[2];	
	
	return 0;
}


//DMA发送完成后的回调函数
static void (*i2s_tx_callback)(uint32_t) = NULL;

//DMA接收完成后的回调函数
static void (*i2s_rx_callback)(uint32_t) = NULL;


/**
 ** @brief: 注册获取DMA发送&接收完成后的回调函数
 */
void i2s_register_tx_rx_cb(void (*tx_cb_func)(uint32_t), void (*rx_cb_func)(uint32_t))
{
	i2s_tx_callback = tx_cb_func;
	i2s_rx_callback = rx_cb_func;
}


/** @brief 启动I2S数据发送
 ** @param buff--数据所在的内存地址
 **        len--以uint16_t为单位, 数据的大小
 ** @reval 0-启动成功  非0-启动失败
 */
int i2s_start_tx_rx(uint16_t *tx_buf, uint16_t *tx_buf2, uint16_t tx_len, uint16_t *rx_buf, uint16_t *rx_buf2, uint16_t rx_len)
{
	if(tx_buf==NULL || tx_buf2==NULL || tx_len==0)
	{
		return -1;
	}
	
	/* I2S接收配置 */
	DMA_Cmd(DMA1_Stream3, DISABLE);
	DMA_ClearFlag(DMA1_Stream3, DMA_IT_TCIF3|DMA_IT_TEIF3);
	SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, DISABLE);
	if(rx_buf!=NULL && rx_buf2!=NULL && rx_len!=0)  //对接收数据感兴趣时才进行接收配置
	{
		DMA1_Stream3->M0AR = (uint32_t)rx_buf;
		DMA1_Stream3->NDTR = rx_len;
		DMA_DoubleBufferModeConfig(DMA1_Stream3, (uint32_t)rx_buf2, DMA_Memory_0);	
		
		i2s_rx_dma_irq_enable();
		DMA_Cmd(DMA1_Stream3, ENABLE);
		SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, ENABLE);
	}
	
	/* I2S发送配置 */                       //一定要有
	DMA_Cmd(DMA1_Stream4, DISABLE);                      //先关闭
	DMA_ClearFlag(DMA1_Stream4, DMA_IT_TCIF4|DMA_IT_TEIF4);
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
	
	DMA1_Stream4->M0AR = (uint32_t)tx_buf;
	DMA1_Stream4->NDTR = tx_len;
	DMA_DoubleBufferModeConfig(DMA1_Stream4, (uint32_t)tx_buf2, DMA_Memory_0);
	
	i2s_tx_dma_irq_enable();
	DMA_Cmd(DMA1_Stream4, ENABLE);                      //这2个开关打开后将启动DMA传输
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	
	return 0;
}

/*
 * @brief: 停止音频播放, 调用此函数
 */
int i2s_tx_release(void)
{
	i2s_tx_dma_irq_disable();
	DMA_Cmd(DMA1_Stream4, DISABLE);
	DMA_ClearFlag(DMA1_Stream4, DMA_IT_TCIF4|DMA_IT_TEIF4);  //F407 DMA使能之后,单独运行关闭使能语句, 会改写中断传输标志位,
	                                                         //从而触发一次中断.所以此处有必要清除一次
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
	I2S_Cmd(SPI2, DISABLE);
	
	i2s_tx_callback = NULL;
	
	return 0;
}

/*
 * @brief: 停止录音, 调用此函数
 */
int i2s_rx_release(void)
{
	int ret = DMA_GetCurrDataCounter(DMA1_Stream3);
	
	i2s_tx_release();          //接收是靠发送驱动的, 有接收一定有发送
	
	i2s_rx_dma_irq_disable();
	DMA_Cmd(DMA1_Stream3, DISABLE);
	DMA_ClearFlag(DMA1_Stream3, DMA_IT_TCIF3|DMA_IT_TEIF3);  //F407 DMA使能之后,单独运行关闭使能语句, 会改写中断传输标志位,
	                                                         //从而触发一次中断.所以此处有必要清除一次
	SPI_I2S_DMACmd(I2S2ext, SPI_I2S_DMAReq_Rx, DISABLE);
	I2S_Cmd(I2S2ext, DISABLE);
	
	i2s_rx_callback = NULL;
	
	return ret;
}

//DMA发送完成中断服务函数
void DMA1_Stream4_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF4) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF4);

		if(i2s_tx_callback != NULL)
		{
			uint32_t cur_buff_idx = DMA_GetCurrentMemoryTarget(DMA1_Stream4);
			(*i2s_tx_callback)(cur_buff_idx);
		}
		else
		{	/* 没有注册回调函数, 中断没有意义,直接关闭中断 */
			i2s_tx_dma_irq_disable();
		}
	}
	else
	{
		DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TEIF4);
	}
}

//DMA接收完成中断服务函数
void DMA1_Stream3_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_Stream3, DMA_IT_TCIF3) == SET)
	{
		DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3);

		if(i2s_rx_callback != NULL)
		{
			uint32_t cur_buff_idx = DMA_GetCurrentMemoryTarget(DMA1_Stream3);
			(*i2s_rx_callback)(cur_buff_idx);
		}
		else
		{	/* 没有注册回调函数, 中断没有意义,直接关闭中断 */
			i2s_rx_dma_irq_disable();
		}
	}
	else
	{
		DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TEIF3);
	}	
}














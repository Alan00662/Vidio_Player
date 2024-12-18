#ifndef  __SPI_V3_H_
#define  __SPI_V3_H_


#include "stm32f4xx.h"

//SPI Controller上的设备片选信号
#define  FLASH_CS_L()  GPIOB->BSRRH = GPIO_Pin_14
#define  FLASH_CS_H()  GPIOB->BSRRL = GPIO_Pin_14
#define  NRF_CS_L()    GPIOG->BSRRH = GPIO_Pin_7
#define  NRF_CS_H()    GPIOG->BSRRL = GPIO_Pin_7

//#define  LCD_CS_L()    GPIOA->BSRRH = GPIO_Pin_2
//#define  LCD_CS_H()    GPIOA->BSRRL = GPIO_Pin_2

//SPI3 Controller上挂接的设备
#define  FLASH_MOUNT_SPI     SPI3
#define  NRF24L01_MOUNT_SPI  SPI3

//SPI1 Controller上挂接的设备
//#define  LCD_MOUNT_SPI     SPI1


void spi_controller_init(SPI_TypeDef* SPIx);
void spi_set_data_8bit(SPI_TypeDef* SPIx);
void spi_set_data_16bit(SPI_TypeDef* SPIx);
void spi_set_clkpres(SPI_TypeDef* SPIx, uint16_t prescaler);
void spi_set_cpol(SPI_TypeDef* SPIx, uint16_t SPI_CPOL, uint16_t SPI_CPHA);
int spi_get_config(SPI_TypeDef* SPIx);
void spi_recover_config(SPI_TypeDef* SPIx, int cr);

int spi_write_read_byte(SPI_TypeDef* SPIx, int txdat);
int spi_write_byte(SPI_TypeDef* SPIx, int txdat);
uint16_t spi_read_byte(SPI_TypeDef* SPIx);
int spi_write_buff(SPI_TypeDef* SPIx, uint8_t *buff, int len);
int spi_read_buff(SPI_TypeDef* SPIx, uint8_t *buff, int len);

//这里实现SPI1与SPI3的DMA收发
void SPI1_DMA_Init(void);
void spi1_dma_data_size(int size);
int spi1_write_dma(void *buff, int len, int mem_inc);
int spi1_read_dma(void *buff, int len);

void SPI3_DMA_Init(void);
void spi3_dma_data_size(int size);
int spi3_write_dma(void *buff, int len, int mem_inc);
int spi3_read_dma(void *buff, int len);

//============================================================
//I2S2 Controller
#define  i2s_tx_dma_irq_disable()  NVIC_DisableIRQ(DMA1_Stream4_IRQn)
#define  i2s_tx_dma_irq_enable()   NVIC_EnableIRQ(DMA1_Stream4_IRQn)

#define  i2s_rx_dma_irq_disable()  NVIC_DisableIRQ(DMA1_Stream3_IRQn)
#define  i2s_rx_dma_irq_enable()   NVIC_EnableIRQ(DMA1_Stream3_IRQn)

void i2s_controller_init(void);
int i2s_controller_cfg(uint16_t I2S_Standard, uint16_t I2S_DataFormat, uint32_t sample_rate);
void i2s_register_tx_rx_cb(void (*tx_cb_func)(uint32_t), void (*rx_cb_func)(uint32_t));
int i2s_start_tx_rx(uint16_t *tx_buf, uint16_t *tx_buf2, uint16_t tx_len, uint16_t *rx_buf, uint16_t *rx_buf2, uint16_t rx_len);
int i2s_frequency_adjust(uint32_t new_freq);
int i2s_tx_release(void);
int i2s_rx_release(void);


#endif




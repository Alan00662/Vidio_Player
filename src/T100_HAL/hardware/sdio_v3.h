#ifndef  __SDIO_V3_H_
#define  __SDIO_V3_H_

#include "stm32f4xx.h"


int sdio_controller_release(void);
int sdio_controller_init(void);

void sdio_clk_open(void);
void sdio_clk_close(void);
//void sdio_pin_init(void);
//void sdio_pin_toggle(void);
int sdio_dat0_read(void);
void sdio_dat123_release(void);

void sdio_controller_config(uint8_t ClockDiv, uint32_t ClockEdge, uint32_t BusWide, uint32_t ClockPowerSave);
void sdio_controller_power_save(FunctionalState state);
void sdio_controller_irq_set(FunctionalState new_state);

void sdio_dma_config(void *mem_buff, uint32_t trans_len, uint32_t trans_dir);

#endif





#include "led.h"

led_ctrl_t led_ctrl = {0, 0};

void led_init(void)
{
//	//PF9-LED0 PF10-LED1
//	GPIO_InitTypeDef  GPIO_InitStructure;
//	
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);  //使能GPIOF时钟

//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	GPIO_Init(GPIOF, &GPIO_InitStructure);
//	LED0Set();
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
//	GPIO_Init(GPIOF, &GPIO_InitStructure);
//	LED1Set();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOF_CLK_ENABLE();  
	  /*Configure GPIO pins : PFPin PFPin */
  GPIO_InitStruct.Pin = LED0_Pin|LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  LED0Set();
  LED1Set();
  
}


inline void led_tx_on(uint32_t ticks)
{
	led_ctrl.tx_tick = ticks;
	LED0Reset();
}

inline void led_rx_on(uint32_t ticks)
{
	led_ctrl.rx_tick = ticks;
	LED1Reset();	
}

inline void led_tx_off_check(void)
{
	if(led_ctrl.tx_tick > 0)
	{
		led_ctrl.tx_tick--;
		if(led_ctrl.tx_tick == 0)
		{	LED0Set();	}
	}
}

inline void led_rx_off_check(void)
{
	if(led_ctrl.rx_tick > 0)
	{
		led_ctrl.rx_tick--;
		if(led_ctrl.rx_tick == 0)
		{	LED1Set();	}
	}
}



#ifndef  __LED_H_
#define  __LED_H_

#ifdef  __cplusplus
    extern "C" {
#endif

#include "main.h" 

//PF9-LED0 PF10-LED1
//#define LED0Set()   GPIOF->BSRRL = GPIO_Pin_9     /*PF9 = 1 */
//#define LED0Reset() GPIOF->BSRRH = GPIO_Pin_9     /*PF9 = 0 */
//#define LED1Set()   GPIOF->BSRRL = GPIO_Pin_10    /*PF10 = 1 */
//#define LED1Reset() GPIOF->BSRRH = GPIO_Pin_10    /*PF10 = 0 */	

//#define LED0Toggle() GPIOF->ODR ^= GPIO_Pin_9
//#define LED1Toggle() GPIOF->ODR ^= GPIO_Pin_10
#define LED0Set()			HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET)    
#define LED0Reset()         HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET)
                                  
#define LED1Set()			HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET)    
#define LED1Reset()         HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET)

#define LED0Toggle()		HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin)
#define LED1Toggle()		HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin)

typedef struct led_ctrl_s
{
	uint32_t tx_tick;
	uint32_t rx_tick;
}led_ctrl_t;
		
extern led_ctrl_t led_ctrl;

//uart1 TXD/RXD indicate led
void led_init(void);
void led_tx_on(uint32_t ticks);  //called when uart1 recv data
void led_rx_on(uint32_t ticks);  //called when uart1 send data
void led_tx_off_check(void);
void led_rx_off_check(void);

#ifdef  __cplusplus
}  
#endif

#endif



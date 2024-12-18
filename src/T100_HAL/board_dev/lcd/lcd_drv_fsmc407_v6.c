#include "lcd_drv_fsmc407_v6.h"
//#include "systick.h"
#include "my_printf.h"
#include "libc.h"
#include "main.h"
//v6变化(2023-10-15):
//增加了HX8347D

#define delay_ms  HAL_Delay
//LCD基本属性
lcd_params_t lcd_dev = {
	.name = "STM32F407_2.8inch_FSMC_LCD240x320",
	.drv_type = 0,      //驱动IC(比如ILI9341 SSD1289等), 在lcd_init()中确定

	.dir_ctrl = {       //共8种组合, 常用的有4种:
		.swap_x_y = 1,  //000 011 -- 这2种是竖屏模式 水平分辨率(xres)=240, 垂直分辨率(yres)=320
		.rev_x = 1,     //101 110 -- 这2种是横屏模式 水平分辨率(xres)=320, 垂直分辨率(yres)=240
		.rev_y = 0,
	},
	.xres = 0,          //这2个参数在lcd_dir_ctrl()函数中会根据dir_ctrl属性设置
	.yres = 0,

	.wramcmd = 0,       //在lcd_init()中确定
	.setxcmd = 0,
	.setycmd = 0,

	.forecolor = LCD_MAGENTA,
	.backcolor = LCD_CYAN,    //LCD_WHITE  LCD_CYAN  LCD_RED
};


//===================================================================================
//这3个函数的使用情景: 一个寄存器要连续读写几个值
void LCD_WR_REG(uint16_t reg_val)
{
	lcd_opt->reg = reg_val;
}

void LCD_WR_DATA(uint16_t data)
{
	lcd_opt->ram = data;
}

uint16_t LCD_RD_DATA(void)
{
	volatile uint16_t dat;
	dat = lcd_opt->ram;
	return dat;
}

//此函数的使用场景: 一个寄存器写一个值
void LCD_WriteReg(uint16_t reg, uint16_t reg_val)
{
	lcd_opt->reg = reg;
	lcd_opt->ram = reg_val;
}

uint16_t LCD_ReadReg(uint16_t reg)
{
	lcd_opt->reg = reg;
//	delay_us(5);
	return lcd_opt->ram;
}

//===================================================================================
//发出往LCD显存中写数据的命令
void LCD_WriteRAM_Prepare(void)
{
	lcd_opt->reg = lcd_dev.wramcmd;
}

//往LCD显存中填充颜色值
void LCD_WriteRAM(uint16_t color)
{
	lcd_opt->ram = color;
}

/* 在改变LCD的扫描方向后(lcd_dir_ctrl()函数中根据lcd_dev.dir_ctrl设置LCD的扫描方向): 
 * (1) ILI9341等较新的显卡, GRAM开窗时会根据扫描方向自动切换坐标系.
 * (2) SSD1289/LGDP4531/ILI9325等较老的显卡, GRAM开窗时不会根据扫描方向自动切换坐标系,
 *     它们只有一种坐标系, 这时在开窗前就需要根据扫描方向进行坐标变换. 
 *  这是老显卡与新显卡的一个较大区别.
 */
//把虚拟(逻辑)坐标转换为绝对(物理)坐标
// *x 取值 [0, lcd_dev.xres-1]
// *y 取值 [0, lcd_dev.yres-1]
static void __lcd_xy_convert(uint16_t *x, uint16_t *y)
{
	uint16_t temp;

	if( lcd_dev.dir_ctrl.rev_x )
		*x = lcd_dev.xres - *x - 1;
	if( lcd_dev.dir_ctrl.rev_y )
		*y = lcd_dev.yres - *y - 1;
	if( lcd_dev.dir_ctrl.swap_x_y )
	{
		temp = *x;
		*x = *y;
		*y = temp;
	}
}


//设置LCD显存中光标的位置
//Xpos [0, lcddev.width-1]
//Ypos [0, lcddev.height-1]
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
	{	//其实就是Set_Window
		LCD_WR_REG(lcd_dev.setxcmd); 
		LCD_WR_DATA(Xpos>>8);
		LCD_WR_DATA(Xpos&0XFF);
		LCD_WR_DATA(Xpos>>8);           //Add 2022-10-27
		LCD_WR_DATA(Xpos&0XFF);

		LCD_WR_REG(lcd_dev.setycmd); 
		LCD_WR_DATA(Ypos>>8); 
		LCD_WR_DATA(Ypos&0XFF);
		LCD_WR_DATA(Ypos>>8); 
		LCD_WR_DATA(Ypos&0XFF);
	}
	else if(lcd_dev.drv_type == 0x8347)
	{
		LCD_WR_REG(0x02); 
		LCD_WR_DATA(Xpos>>8);
		LCD_WR_REG(0x03);
		LCD_WR_DATA(Xpos&0XFF);
		LCD_WR_REG(0x04);              //2023-10-15移植时加上end position, 同上面ILI9341.
		LCD_WR_DATA(Xpos>>8);          //测试发现, 不设置end position是大BUG, 比如
		LCD_WR_REG(0x05);              //可以注释掉end position然后进行lcd_draw_circle测试,
		LCD_WR_DATA(Xpos&0XFF);        //此时液晶显示异常

		LCD_WR_REG(0x06); 
		LCD_WR_DATA(Ypos>>8);
		LCD_WR_REG(0x07);
		LCD_WR_DATA(Ypos&0XFF);
		LCD_WR_REG(0x08);              //end position
		LCD_WR_DATA(Ypos>>8);
		LCD_WR_REG(0x09);
		LCD_WR_DATA(Ypos&0XFF);	
	}
	else
	{	//LGDP4531/LGDP4535/SPFD5408/SSD1289/ILI9325
		__lcd_xy_convert(&Xpos, &Ypos);
		LCD_WriteReg(lcd_dev.setxcmd, Xpos);
		LCD_WriteReg(lcd_dev.setycmd, Ypos);
	}
}

//设置一个窗口  (sx, sy)--窗口左上角坐标 (ex, ey)--窗口右下角坐标
//开辟窗口的优点: 往窗口内仍颜色数据后, 显存地址会自动递增
//LCD_Clear() LCD_Fill() LCD_Color_Fill() LCD_ReadWindow() 等函数都用到了设置窗口的功能
void LCD_Set_Window(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey)
{
	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
	{
		LCD_WR_REG(lcd_dev.setxcmd); 
		LCD_WR_DATA(sx>>8);
		LCD_WR_DATA(sx&0XFF);	 
		LCD_WR_DATA(ex>>8); 
		LCD_WR_DATA(ex&0XFF);
		
		LCD_WR_REG(lcd_dev.setycmd); 
		LCD_WR_DATA(sy>>8);
		LCD_WR_DATA(sy&0XFF); 
		LCD_WR_DATA(ey>>8); 
		LCD_WR_DATA(ey&0XFF); 
	}
	else if(lcd_dev.drv_type == 0x8347)
	{
		LCD_WR_REG(0x02); 
		LCD_WR_DATA(sx>>8);
		LCD_WR_REG(0x03);
		LCD_WR_DATA(sx&0XFF);
		LCD_WR_REG(0x04); 
		LCD_WR_DATA(ex>>8);
		LCD_WR_REG(0x05);
		LCD_WR_DATA(ex&0XFF);

		LCD_WR_REG(0x06); 
		LCD_WR_DATA(sy>>8);
		LCD_WR_REG(0x07);
		LCD_WR_DATA(sy&0XFF);
		LCD_WR_REG(0x08); 
		LCD_WR_DATA(ey>>8);
		LCD_WR_REG(0x09);
		LCD_WR_DATA(ey&0XFF); 	
	}
	else
	{
		//LGDP4531/LGDP4535/SPFD5408/SSD1289/ILI9325
		uint16_t x_min, x_max, y_min, y_max;

		__lcd_xy_convert(&sx, &sy);
		__lcd_xy_convert(&ex, &ey);
		if(sx > ex) {
			x_min = ex;
			x_max = sx;
		} else {
			x_min = sx;
			x_max = ex;	
		}

		if(sy > ey) {
			y_min = ey;
			y_max = sy;
		} else {
			y_min = sy;
			y_max = ey;
		}
		
		if(lcd_dev.drv_type == 0x1289)
		{
			//SetWindow
			LCD_WriteReg(0x44, (x_max<<8)|x_min);	//Set X Star and End
			LCD_WriteReg(0x45, y_min);	//Set Y Star
			LCD_WriteReg(0x46, y_max);	//Set Y End			
		}
		else	//LGDP4531/LGDP4535/SPFD5408/ILI9325
		{
			//SetWindow
			LCD_WriteReg(0x50, x_min);	//Set X Star
			LCD_WriteReg(0x51, x_max);	//Set X End
			LCD_WriteReg(0x52, y_min);	//Set Y Star
			LCD_WriteReg(0x53, y_max);	//Set Y End
		}

		//SetCursor
		LCD_WriteReg(lcd_dev.setxcmd, sx);
		LCD_WriteReg(lcd_dev.setycmd, sy);
	}
}


//=================================================================================
//#define  LCD_BL_ON()  GPIOB->BSRRL = GPIO_Pin_15     /*PB15 = 1 */
//#define  LCD_BL_OFF() GPIOB->BSRRH = GPIO_Pin_15     /*PB15 = 0 */

////pwm_duty = [0, 100]
//void lcd_backlight_pwm(uint16_t pwm_duty)
//{
//	GPIO_InitTypeDef  GPIO_InitStructure;
//	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
//	TIM_OCInitTypeDef TIM_OCInitStructure;
//	RCC_ClocksTypeDef RCC_ClocksStruct;
//	uint16_t prescaler, period;

//	if(pwm_duty>0 && pwm_duty<100)
//	{
//		GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM12);

//		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
//		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
//		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;		
//		GPIO_Init(GPIOB, &GPIO_InitStructure);
//		
//		TIM_Cmd(TIM12, DISABLE);
//		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE);
//		TIM_DeInit(TIM12);
//		TIM_InternalClockConfig(TIM12);
//		
//		//TIM12 Time Base config
//		RCC_GetClocksFreq(&RCC_ClocksStruct);

//		period = 100;
//		prescaler = (RCC_ClocksStruct.PCLK1_Frequency*2)/period/10000;   //分频后得到一个10KHz(100us)的时钟
//		
//		TIM_TimeBaseStructure.TIM_Prescaler = prescaler-1;      //预分频值
//		TIM_TimeBaseStructure.TIM_Period = period - 1;
//		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
//		TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
//		TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;            //重复溢出几次才来一次中断, 只有高级定时器TIM1与TIM8才需配置
//		TIM_TimeBaseInit(TIM12, &TIM_TimeBaseStructure);
//	
//		TIM_ARRPreloadConfig(TIM12, DISABLE);          //更新ARR后立即生效

//		//初始化TIM12_CH2为PWM输出模式
//		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               //PWM1占空比为正逻辑(高电平), PWM2占空比为反逻辑
//		TIM_OCInitStructure.TIM_Pulse  = pwm_duty;                      //该值决定占空比 TIM_Pulse/(TIM_Period+1)
//		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   //比较输出使能
//		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       //输出极性高
//		TIM_OC2Init(TIM12, &TIM_OCInitStructure);

//		TIM_OC2PreloadConfig(TIM12, TIM_OCPreload_Enable);               //使能预装载寄存器		

//		TIM_Cmd(TIM12, ENABLE); 	
//	}
//	else
//	{
//		//PB15--LCB_BL
//		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
//		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;		
//		GPIO_Init(GPIOB, &GPIO_InitStructure);
//		
//		if(pwm_duty == 0)
//			LCD_BL_OFF();
//		else
//			LCD_BL_ON();
//	}
//}
#define  LCD_BL_ON()  HAL_GPIO_WritePin(GPIOB, LCD_BL_Pin, GPIO_PIN_SET)         /*PB15 = 1 */
#define  LCD_BL_OFF() HAL_GPIO_WritePin(GPIOB, LCD_BL_Pin, GPIO_PIN_RESET)         /*PB15 = 0 */

/*
 * @brief: 初始化LCD控制器
 */
int lcd_init(void)
{
	int i;
	uint8_t buff[4] = {0};
	uint16_t drive_id = 0;
	
//	GPIO_InitTypeDef  GPIO_InitStructure;

//	//PB15--LCB_BL
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	LCD_BL_OFF();
	  GPIO_InitTypeDef GPIO_InitStruct = {0};
	  GPIO_InitStruct.Pin = LCD_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
//	delay_ms(50);    //After power on or reset, the delay is necessary. 2023-04-14

	drive_id = LCD_ReadReg(0x0000);  //LGDP4531/LGDP4535/SPFD5408/SSD1289/ILI9325 Device code read

	if(drive_id == 0x4531)           //LGDP4531
	{
		lcd_dev.drv_type = 0x4531;
		lcd_dev.wramcmd = 0x22;
		lcd_dev.setxcmd = 0x20;
		lcd_dev.setycmd = 0x21;

		LCD_WriteReg(0X00,0X0001);   
		delay_ms(10);   
		LCD_WriteReg(0X10,0X1628);   
		LCD_WriteReg(0X12,0X000e);//0x0006    
		LCD_WriteReg(0X13,0X0A39);   
		delay_ms(10);   
		LCD_WriteReg(0X11,0X0040);   
		LCD_WriteReg(0X15,0X0050);
		delay_ms(10);   
		LCD_WriteReg(0X12,0X001e);//16    
		delay_ms(10);   
		LCD_WriteReg(0X10,0X1620);   
		LCD_WriteReg(0X13,0X2A39);   
		delay_ms(10);   
		LCD_WriteReg(0X01,0X0100);   
		LCD_WriteReg(0X02,0X0300);   
		LCD_WriteReg(0X03,0X1038);//改变方向的   
		LCD_WriteReg(0X08,0X0202);   
		LCD_WriteReg(0X0A,0X0008);
		LCD_WriteReg(0X30,0X0000);    //LGDP4531 伽马校正
		LCD_WriteReg(0X31,0X0402);   
		LCD_WriteReg(0X32,0X0106);   
		LCD_WriteReg(0X33,0X0503);   
		LCD_WriteReg(0X34,0X0104);   
		LCD_WriteReg(0X35,0X0301);   
		LCD_WriteReg(0X36,0X0707);   
		LCD_WriteReg(0X37,0X0305);   
		LCD_WriteReg(0X38,0X0208);   
		LCD_WriteReg(0X39,0X0F0B);
		LCD_WriteReg(0X41,0X0002);   
		LCD_WriteReg(0X60,0X2700);   
		LCD_WriteReg(0X61,0X0001);   
		LCD_WriteReg(0X90,0X0210);   
		LCD_WriteReg(0X92,0X010A);   
		LCD_WriteReg(0X93,0X0004);   
		LCD_WriteReg(0XA0,0X0100);   
		LCD_WriteReg(0X07,0X0001);   
		LCD_WriteReg(0X07,0X0021);   
		LCD_WriteReg(0X07,0X0023);   
		LCD_WriteReg(0X07,0X0033);   
		LCD_WriteReg(0X07,0X0133);
		LCD_WriteReg(0XA0,0X0000);
	}
	else if(drive_id == 0x4535)           //LGDP4535
	{
		lcd_dev.drv_type = 0x4535;
		lcd_dev.wramcmd = 0x22;
		lcd_dev.setxcmd = 0x20;
		lcd_dev.setycmd = 0x21;

		LCD_WriteReg(0X15,0X0030);   
		LCD_WriteReg(0X9A,0X0010);   
 		LCD_WriteReg(0X11,0X0020);   
 		LCD_WriteReg(0X10,0X3428);   
		LCD_WriteReg(0X12,0X0002);//16    
 		LCD_WriteReg(0X13,0X1038);   
		delay_ms(40);   
		LCD_WriteReg(0X12,0X0012);//16    
		delay_ms(40);   
  		LCD_WriteReg(0X10,0X3420);   
 		LCD_WriteReg(0X13,0X3038);   
		delay_ms(70);   
		LCD_WriteReg(0X30,0X0000);   
		LCD_WriteReg(0X31,0X0402);   
		LCD_WriteReg(0X32,0X0307);   
		LCD_WriteReg(0X33,0X0304);   
		LCD_WriteReg(0X34,0X0004);   
		LCD_WriteReg(0X35,0X0401);   
		LCD_WriteReg(0X36,0X0707);   
		LCD_WriteReg(0X37,0X0305);   
		LCD_WriteReg(0X38,0X0610);   
		LCD_WriteReg(0X39,0X0610); 
		  
		LCD_WriteReg(0X01,0X0100);   
		LCD_WriteReg(0X02,0X0300);   
		LCD_WriteReg(0X03,0X1030);//改变方向的   
		LCD_WriteReg(0X08,0X0808);   
		LCD_WriteReg(0X0A,0X0008);   
 		LCD_WriteReg(0X60,0X2700);   
		LCD_WriteReg(0X61,0X0001);   
		LCD_WriteReg(0X90,0X013E);   
		LCD_WriteReg(0X92,0X0100);   
		LCD_WriteReg(0X93,0X0100);   
 		LCD_WriteReg(0XA0,0X3000);   
 		LCD_WriteReg(0XA3,0X0010);   
		LCD_WriteReg(0X07,0X0001);   
		LCD_WriteReg(0X07,0X0021);   
		LCD_WriteReg(0X07,0X0023);   
		LCD_WriteReg(0X07,0X0033);   
		LCD_WriteReg(0X07,0X0133);   		
	}
	else if(drive_id == 0x5408)       //SPFD5408
	{
		lcd_dev.drv_type = 0x5408;
		lcd_dev.wramcmd = 0x22;
		lcd_dev.setxcmd = 0x20;
		lcd_dev.setycmd = 0x21;
		
		//ILI9328
		LCD_WriteReg(0x00EC,0x108F);// internal timeing      
		LCD_WriteReg(0x00EF,0x1234);// ADD        
//		LCD_WriteReg(0x00e7,0x0010);      
//		LCD_WriteReg(0x0000,0x0001);//开启内部时钟
		LCD_WriteReg(0x0001,0x0100);     
		LCD_WriteReg(0x0002,0x0700);  //电源开启                    
		LCD_WriteReg(0x0003,0x1030);  //65K  RGB  
		LCD_WriteReg(0x0004,0x0000);                                   
		LCD_WriteReg(0x0008,0x0202);	           
		LCD_WriteReg(0x0009,0x0000);         
		LCD_WriteReg(0x000a,0x0000);//display setting         
		LCD_WriteReg(0x000c,0x0001);//display setting          
		LCD_WriteReg(0x000d,0x0000);//0f3c          
		LCD_WriteReg(0x000f,0x0000);
		//电源配置
		LCD_WriteReg(0x0010,0x0000);   
		LCD_WriteReg(0x0011,0x0007);
		LCD_WriteReg(0x0012,0x0000);                                                                 
		LCD_WriteReg(0x0013,0x0000);                 
		LCD_WriteReg(0x0007,0x0001);                 
		delay_ms(50); 
		LCD_WriteReg(0x0010,0x1490);   
		LCD_WriteReg(0x0011,0x0227);
		delay_ms(50); 
		LCD_WriteReg(0x0012,0x008A);                  
		delay_ms(50); 
		LCD_WriteReg(0x0013,0x1a00);   
		LCD_WriteReg(0x0029,0x0006);
		LCD_WriteReg(0x002b,0x000d);
		delay_ms(50); 
		LCD_WriteReg(0x0020,0x0000);                                                            
		LCD_WriteReg(0x0021,0x0000);           
		delay_ms(50); 
		//伽马校正
	//	LCD_WriteReg(0x0030,0x0000); 
	//	LCD_WriteReg(0x0031,0x0604);   
	//	LCD_WriteReg(0x0032,0x0305);
	//	LCD_WriteReg(0x0035,0x0000);
	//	LCD_WriteReg(0x0036,0x0C09); 
	//	LCD_WriteReg(0x0037,0x0204);
	//	LCD_WriteReg(0x0038,0x0301);        
	//	LCD_WriteReg(0x0039,0x0707);     
	//	LCD_WriteReg(0x003c,0x0000);
	//	LCD_WriteReg(0x003d,0x0a0a);

		LCD_WriteReg(0x0030,0x0707);    //SSD1289效果最好
		LCD_WriteReg(0x0031,0x0204);  
		LCD_WriteReg(0x0032,0x0204);  
		LCD_WriteReg(0x0033,0x0502);  
		LCD_WriteReg(0x0034,0x0507);  
		LCD_WriteReg(0x0035,0x0204);  
		LCD_WriteReg(0x0036,0x0204);  
		LCD_WriteReg(0x0037,0x0502);  
		LCD_WriteReg(0x003A,0x0302);  
		LCD_WriteReg(0x003B,0x0302);

		delay_ms(50); 
		LCD_WriteReg(0x0050,0x0000); //水平GRAM起始位置 
		LCD_WriteReg(0x0051,0x00ef); //水平GRAM终止位置 
		LCD_WriteReg(0x0052,0x0000); //垂直GRAM起始位置  
		LCD_WriteReg(0x0053,0x013f); //垂直GRAM终止位置  

		LCD_WriteReg(0x0060,0xa700);        
		LCD_WriteReg(0x0061,0x0001); 
		LCD_WriteReg(0x006a,0x0000);
		LCD_WriteReg(0x0080,0x0000);
		LCD_WriteReg(0x0081,0x0000);
		LCD_WriteReg(0x0082,0x0000);
		LCD_WriteReg(0x0083,0x0000);
		LCD_WriteReg(0x0084,0x0000);
		LCD_WriteReg(0x0085,0x0000);
	  
		LCD_WriteReg(0x0090,0x0010);     
		LCD_WriteReg(0x0092,0x0600);  
		//开启显示设置    
		LCD_WriteReg(0x0007,0x0133);	
	}
	else if(drive_id == 0x8989)          //SSD1289
	{
		lcd_dev.drv_type = 0x1289;
		lcd_dev.wramcmd = 0x22;
		lcd_dev.setxcmd = 0x4E;
		lcd_dev.setycmd = 0x4F;		
		
		LCD_WriteReg(0x0000,0x0001);//打开晶振
		LCD_WriteReg(0x0003,0xA8A4);//0xA8A4
		LCD_WriteReg(0x000C,0x0000);    
		LCD_WriteReg(0x000D,0x080C);   
		LCD_WriteReg(0x000E,0x2B00);    
		LCD_WriteReg(0x001E,0x00B0);    
		LCD_WriteReg(0x0001,0x2B3F);  //GRAM->LCD Panel 0x2B3F 0x6B3F
		LCD_WriteReg(0x0002,0x0600);
		LCD_WriteReg(0x0010,0x0000);  //the driver leaves the sleep mode 
		LCD_WriteReg(0x0011,0x6070);  //MCU->GRAM
		LCD_WriteReg(0x0005,0x0000);  
		LCD_WriteReg(0x0006,0x0000);  
		LCD_WriteReg(0x0016,0xEF1C);  
		LCD_WriteReg(0x0017,0x0003);  
		LCD_WriteReg(0x0007,0x0233); //0x0233       
		LCD_WriteReg(0x000B,0x0000);  
		LCD_WriteReg(0x000F,0x0000); //扫描开始地址
		LCD_WriteReg(0x0041,0x0000);  
		LCD_WriteReg(0x0042,0x0000);  
		LCD_WriteReg(0x0048,0x0000);  
		LCD_WriteReg(0x0049,0x013F);  
		LCD_WriteReg(0x004A,0x0000);  
		LCD_WriteReg(0x004B,0x0000);  
		LCD_WriteReg(0x0044,0xEF00);  
		LCD_WriteReg(0x0045,0x0000);  
		LCD_WriteReg(0x0046,0x013F);  
		LCD_WriteReg(0x0030,0x0707);  
		LCD_WriteReg(0x0031,0x0204);  
		LCD_WriteReg(0x0032,0x0204);  
		LCD_WriteReg(0x0033,0x0502);  
		LCD_WriteReg(0x0034,0x0507);  
		LCD_WriteReg(0x0035,0x0204);  
		LCD_WriteReg(0x0036,0x0204);  
		LCD_WriteReg(0x0037,0x0502);  
		LCD_WriteReg(0x003A,0x0302);  
		LCD_WriteReg(0x003B,0x0302);  
		LCD_WriteReg(0x0023,0x0000);  
		LCD_WriteReg(0x0024,0x0000);  
		LCD_WriteReg(0x0025,0x8000);  
		LCD_WriteReg(0x004f,0);        //行首址0
		LCD_WriteReg(0x004e,0);        //列首址0
	}
	else if(drive_id == 0x9325)           //ILI9325
	{
		lcd_dev.drv_type = 0x9325;
		lcd_dev.wramcmd = 0x22;
		lcd_dev.setxcmd = 0x20;
		lcd_dev.setycmd = 0x21;		
		
		LCD_WriteReg(0x00E5,0x78F0); 
		LCD_WriteReg(0x0001,0x0100); 
		LCD_WriteReg(0x0002,0x0700); 
		LCD_WriteReg(0x0003,0x1030); 
		LCD_WriteReg(0x0004,0x0000); 
		LCD_WriteReg(0x0008,0x0202);  
		LCD_WriteReg(0x0009,0x0000);
		LCD_WriteReg(0x000A,0x0000); 
		LCD_WriteReg(0x000C,0x0000); 
		LCD_WriteReg(0x000D,0x0000);
		LCD_WriteReg(0x000F,0x0000);
		//power on sequence VGHVGL
		LCD_WriteReg(0x0010,0x0000);   
		LCD_WriteReg(0x0011,0x0007);  
		LCD_WriteReg(0x0012,0x0000);  
		LCD_WriteReg(0x0013,0x0000); 
		LCD_WriteReg(0x0007,0x0000); 
		//vgh 
		LCD_WriteReg(0x0010,0x1690);   
		LCD_WriteReg(0x0011,0x0227);
		//delayms(100);
		//vregiout 
		LCD_WriteReg(0x0012,0x009D); //0x001b
		//delayms(100); 
		//vom amplitude
		LCD_WriteReg(0x0013,0x1900);
		//delayms(100); 
		//vom H
		LCD_WriteReg(0x0029,0x0025); 
		LCD_WriteReg(0x002B,0x000D); 
		//gamma
		LCD_WriteReg(0x0030,0x0007);
		LCD_WriteReg(0x0031,0x0303);
		LCD_WriteReg(0x0032,0x0003);// 0006
		LCD_WriteReg(0x0035,0x0206);
		LCD_WriteReg(0x0036,0x0008);
		LCD_WriteReg(0x0037,0x0406); 
		LCD_WriteReg(0x0038,0x0304);//0200
		LCD_WriteReg(0x0039,0x0007); 
		LCD_WriteReg(0x003C,0x0602);// 0504
		LCD_WriteReg(0x003D,0x0008); 
		//ram
		LCD_WriteReg(0x0050,0x0000); 
		LCD_WriteReg(0x0051,0x00EF);
		LCD_WriteReg(0x0052,0x0000); 
		LCD_WriteReg(0x0053,0x013F);  
		LCD_WriteReg(0x0060,0xA700); 
		LCD_WriteReg(0x0061,0x0001); 
		LCD_WriteReg(0x006A,0x0000); 
		//
		LCD_WriteReg(0x0080,0x0000); 
		LCD_WriteReg(0x0081,0x0000); 
		LCD_WriteReg(0x0082,0x0000); 
		LCD_WriteReg(0x0083,0x0000); 
		LCD_WriteReg(0x0084,0x0000); 
		LCD_WriteReg(0x0085,0x0000); 
		//
		LCD_WriteReg(0x0090,0x0010); 
		LCD_WriteReg(0x0092,0x0600); 
		
		LCD_WriteReg(0x0007,0x0133);
		LCD_WriteReg(0x00,0x0022);//		
	}
	else if(drive_id == 0x0047)
	{
		lcd_dev.drv_type = 0x8347;
		lcd_dev.wramcmd = 0x22;  //读与都是这个命令
		lcd_dev.setxcmd = 0x00;  //No use, HX8347D需要4个命令
		lcd_dev.setycmd = 0x00;  //No use, HX8347D需要4个命令
		
		//Driving ability Setting
		LCD_WriteReg(0xEA,0x00); //PTBA[15:8]
		LCD_WriteReg(0xEB,0x20); //PTBA[7:0]
		LCD_WriteReg(0xEC,0x08); //STBA[15:8]
		LCD_WriteReg(0xED,0xC4); //STBA[7:0]
		LCD_WriteReg(0xE8,0x40); //OPON[7:0]
		LCD_WriteReg(0xE9,0x38); //OPON1[7:0]
		LCD_WriteReg(0xF1,0x01); //OTPS1B
		LCD_WriteReg(0xF2,0x10); //GEN
	//	LCD_WriteReg(0x27,0xA3); //

		//Gamma 2.2 Setting
		LCD_WriteReg(0x40,0x00); //
		LCD_WriteReg(0x41,0x00); //
		LCD_WriteReg(0x42,0x01); //
		LCD_WriteReg(0x43,0x13); //
		LCD_WriteReg(0x44,0x10); //
		LCD_WriteReg(0x45,0x26); //
		LCD_WriteReg(0x46,0x08); //
		LCD_WriteReg(0x47,0x51); //
		LCD_WriteReg(0x48,0x02); //
		LCD_WriteReg(0x49,0x12); //
		LCD_WriteReg(0x4A,0x18); //
		LCD_WriteReg(0x4B,0x19); //
		LCD_WriteReg(0x4C,0x14); //

		LCD_WriteReg(0x50,0x19); //
		LCD_WriteReg(0x51,0x2F); //
		LCD_WriteReg(0x52,0x2C); //
		LCD_WriteReg(0x53,0x3E); //
		LCD_WriteReg(0x54,0x3F); //
		LCD_WriteReg(0x55,0x3F); //
		LCD_WriteReg(0x56,0x2E); //
		LCD_WriteReg(0x57,0x77); //
		LCD_WriteReg(0x58,0x0B); //
		LCD_WriteReg(0x59,0x06); //
		LCD_WriteReg(0x5A,0x07); //
		LCD_WriteReg(0x5B,0x0D); //
		LCD_WriteReg(0x5C,0x1D); //
		LCD_WriteReg(0x5D,0xCC); //

		//Power Voltage Setting
		LCD_WriteReg(0x1B,0x1B); //VRH=4.65V
		LCD_WriteReg(0x1A,0x01); //BT (VGH~15V,VGL~-10V,DDVDH~5V)
		LCD_WriteReg(0x24,0x2F); //VMH(VCOM High voltage ~3.2V)
		LCD_WriteReg(0x25,0x57); //VML(VCOM Low voltage -1.2V)

		//****VCOM offset**///
		LCD_WriteReg(0x23,0x86); //?? flick?for Flicker adjust //can reload from OTP

		//Power on Setting
		LCD_WriteReg(0x18,0x36); //I/P_RADJ,N/P_RADJ, Value equal Normal mode, 75Hz. 0x34-65Hz 0x36-75Hz  0x3A-100Hz
		LCD_WriteReg(0x19,0x01); //OSC_EN='1', start Osc
		LCD_WriteReg(0x01,0x00); //DP_STB='0', out deep sleep
		LCD_WriteReg(0x1F,0x88);// GAS=1, VOMG=00, PON=0, DK=1, XDK=0, DVDH_TRI=0, STB=0
		delay_ms(5);
		LCD_WriteReg(0x1F,0x80);// GAS=1, VOMG=00, PON=0, DK=0, XDK=0, DVDH_TRI=0, STB=0
		delay_ms(5);
		LCD_WriteReg(0x1F,0x90);// GAS=1, VOMG=00, PON=1, DK=0, XDK=0, DVDH_TRI=0, STB=0
		delay_ms(5);
		LCD_WriteReg(0x1F,0xD0);// GAS=1, VOMG=10, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
		delay_ms(5);

		//262k/65k color selection
		LCD_WriteReg(0x17,0x05); //default 0x06 262k color // 0x05 65k color

		//SET PANEL
		LCD_WriteReg(0x36,0x01); //SS_P, GS_P,REV_P,BGR_P

		//Display ON Setting
		LCD_WriteReg(0x28,0x38); //GON=1, DTE=1, D=1000
		delay_ms(40);
		LCD_WriteReg(0x28,0x3C); //GON=1, DTE=1, D=1100

		//Set GRAM Area
		LCD_WriteReg(0x02,0x00);
		LCD_WriteReg(0x03,0x00); //Column Start
		LCD_WriteReg(0x04,0x00);
		LCD_WriteReg(0x05,0xEF); //Column End
		LCD_WriteReg(0x06,0x00);
		LCD_WriteReg(0x07,0x00); //Row Start
		LCD_WriteReg(0x08,0x01);
		LCD_WriteReg(0x09,0x3F); //Row End 
		delay_ms(5);
		LCD_WriteReg(0x0022,0X00);		
	}
	else                                 //Probe ILI9341
	{
		//Probe ILI9341
		LCD_WR_REG(0xD3);  //Read ID4
		for(i=0; i<4; i++)
			buff[i] = LCD_RD_DATA();

		if(((uint32_t)buff[2]<<8|buff[3]) != 0x9341)
		{
			//Probe ST7789
			LCD_WR_REG(0x04);      //Read Display ID
			for(i=0; i<4; i++)
				buff[i] = LCD_RD_DATA();	
		}
	}

	if(((uint32_t)buff[2]<<8|buff[3]) == 0x9341)
	{
		lcd_dev.drv_type = 0x9341;
		lcd_dev.wramcmd = 0x2C;
		lcd_dev.setxcmd = 0x2A;
		lcd_dev.setycmd = 0x2B;			

		LCD_WR_REG(0xCF);  
		LCD_WR_DATA(0x00); 
		LCD_WR_DATA(0xC1); 
		LCD_WR_DATA(0X30); 
		LCD_WR_REG(0xED);  
		LCD_WR_DATA(0x64); 
		LCD_WR_DATA(0x03); 
		LCD_WR_DATA(0X12); 
		LCD_WR_DATA(0X81); 
		LCD_WR_REG(0xE8);  
		LCD_WR_DATA(0x85); 
		LCD_WR_DATA(0x10); 
		LCD_WR_DATA(0x7A); 
		LCD_WR_REG(0xCB);  
		LCD_WR_DATA(0x39); 
		LCD_WR_DATA(0x2C); 
		LCD_WR_DATA(0x00); 
		LCD_WR_DATA(0x34); 
		LCD_WR_DATA(0x02); 
		LCD_WR_REG(0xF7);  
		LCD_WR_DATA(0x20); 
		LCD_WR_REG(0xEA);  
		LCD_WR_DATA(0x00); 
		LCD_WR_DATA(0x00); 
		LCD_WR_REG(0xC0);    //Power control 
		LCD_WR_DATA(0x1B);   //VRH[5:0] 
		LCD_WR_REG(0xC1);    //Power control 
		LCD_WR_DATA(0x01);   //SAP[2:0];BT[3:0] 
		LCD_WR_REG(0xC5);    //VCM control 
		LCD_WR_DATA(0x30); 	 //3F
		LCD_WR_DATA(0x30); 	 //3C
		LCD_WR_REG(0xC7);    //VCM control2 
		LCD_WR_DATA(0XB7); 
		LCD_WR_REG(0x36);    // Memory Access Control 
		LCD_WR_DATA(0x48); 
		LCD_WR_REG(0x3A);   
		LCD_WR_DATA(0x55); 
		LCD_WR_REG(0xB1);   
		LCD_WR_DATA(0x00);   
		LCD_WR_DATA(0x1A); 
		LCD_WR_REG(0xB6);    // Display Function Control 
		LCD_WR_DATA(0x0A); 
		LCD_WR_DATA(0xA2); 
		LCD_WR_REG(0xF2);    // 3Gamma Function Disable 
		LCD_WR_DATA(0x00); 
		LCD_WR_REG(0x26);    //Gamma curve selected 
		LCD_WR_DATA(0x01); 
		LCD_WR_REG(0xE0);    //Set Gamma 
		LCD_WR_DATA(0x0F); 
		LCD_WR_DATA(0x2A); 
		LCD_WR_DATA(0x28); 
		LCD_WR_DATA(0x08); 
		LCD_WR_DATA(0x0E); 
		LCD_WR_DATA(0x08); 
		LCD_WR_DATA(0x54); 
		LCD_WR_DATA(0XA9); 
		LCD_WR_DATA(0x43); 
		LCD_WR_DATA(0x0A); 
		LCD_WR_DATA(0x0F); 
		LCD_WR_DATA(0x00); 
		LCD_WR_DATA(0x00); 
		LCD_WR_DATA(0x00); 
		LCD_WR_DATA(0x00); 		 
		LCD_WR_REG(0XE1);    //Set Gamma 
		LCD_WR_DATA(0x00); 
		LCD_WR_DATA(0x15); 
		LCD_WR_DATA(0x17); 
		LCD_WR_DATA(0x07); 
		LCD_WR_DATA(0x11); 
		LCD_WR_DATA(0x06); 
		LCD_WR_DATA(0x2B); 
		LCD_WR_DATA(0x56); 
		LCD_WR_DATA(0x3C); 
		LCD_WR_DATA(0x05); 
		LCD_WR_DATA(0x10); 
		LCD_WR_DATA(0x0F); 
		LCD_WR_DATA(0x3F); 
		LCD_WR_DATA(0x3F); 
		LCD_WR_DATA(0x0F); 
		LCD_WR_REG(0x2B); 
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x01);
		LCD_WR_DATA(0x3f);
		LCD_WR_REG(0x2A); 
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0x00);
		LCD_WR_DATA(0xef);	 
		LCD_WR_REG(0x11); //Exit Sleep
		delay_ms(120);
		LCD_WR_REG(0x29); //display on			
	}
	else if(((uint32_t)buff[2]<<8|buff[3]) == 0x8552)
	{
		lcd_dev.drv_type = 0x7789;
		lcd_dev.wramcmd = 0x2C;
		lcd_dev.setxcmd = 0x2A;
		lcd_dev.setycmd = 0x2B;
		
        LCD_WR_REG(0x11);
        delay_ms(120);

        LCD_WR_REG(0x36);
        LCD_WR_DATA(0x00);

        LCD_WR_REG(0x3A);
        LCD_WR_DATA(0X05);

        LCD_WR_REG(0xB2);
        LCD_WR_DATA(0x0C);
        LCD_WR_DATA(0x0C);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x33);
        LCD_WR_DATA(0x33);

        LCD_WR_REG(0xB7);
        LCD_WR_DATA(0x35);

        LCD_WR_REG(0xBB);       //vcom
        LCD_WR_DATA(0x32);      //30

        LCD_WR_REG(0xC0);
        LCD_WR_DATA(0x0C);

        LCD_WR_REG(0xC2);
        LCD_WR_DATA(0x01);

        LCD_WR_REG(0xC3);       //vrh
        LCD_WR_DATA(0x10);      //17 0D

        LCD_WR_REG(0xC4);       //vdv
        LCD_WR_DATA(0x20);      //20

        LCD_WR_REG(0xC6);
        LCD_WR_DATA(0x0f);

        LCD_WR_REG(0xD0);
        LCD_WR_DATA(0xA4);
        LCD_WR_DATA(0xA1);

        LCD_WR_REG(0xE0);       //Set Gamma
        LCD_WR_DATA(0xd0);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x02);
        LCD_WR_DATA(0x07);
        LCD_WR_DATA(0x0a);
        LCD_WR_DATA(0x28);
        LCD_WR_DATA(0x32);
        LCD_WR_DATA(0X44);
        LCD_WR_DATA(0x42);
        LCD_WR_DATA(0x06);
        LCD_WR_DATA(0x0e);
        LCD_WR_DATA(0x12);
        LCD_WR_DATA(0x14);
        LCD_WR_DATA(0x17);

        LCD_WR_REG(0XE1);       //Set Gamma
        LCD_WR_DATA(0xd0);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x02);
        LCD_WR_DATA(0x07);
        LCD_WR_DATA(0x0a);
        LCD_WR_DATA(0x28);
        LCD_WR_DATA(0x31);
        LCD_WR_DATA(0x54);
        LCD_WR_DATA(0x47);
        LCD_WR_DATA(0x0e);
        LCD_WR_DATA(0x1c);
        LCD_WR_DATA(0x17);
        LCD_WR_DATA(0x1b);
        LCD_WR_DATA(0x1e);

        LCD_WR_REG(0x2A);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0xef);

        LCD_WR_REG(0x2B);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x00);
        LCD_WR_DATA(0x01);
        LCD_WR_DATA(0x3f);

        LCD_WR_REG(0x29);       //display on
	}

	if(lcd_dev.drv_type == 0)
	{
		my_printf("[%s] LCD probe fail!\n\r", __FUNCTION__);
		my_printf("[%s] LCD_ReadReg(0x0000) ret val: 0x%04X\n\r", __FUNCTION__, drive_id);
		if(((uint32_t)buff[0]<<24|buff[1]<<16|buff[2]<<8|buff[3]) != 0x0)
		{
			my_printf("[%s] LCD_WR_REG(0xD3) and then read ret val: 0x%02X 0x%02X 0x%02X 0x%02X\n\r", \
						__FUNCTION__, buff[0], buff[1], buff[2], buff[3]);
		}
		return -1;
	}
	my_printf("[%s] LCD drive type: 0x%04X\n\r", __FUNCTION__, lcd_dev.drv_type);

	lcd_dir_ctrl(&lcd_dev);
	LCD_BL_ON();           //LCD背光开启
	lcd_clear(lcd_dev.backcolor);

	lcd_dma_init();        //LCD DMA传输初始化

	return 0;
}

void lcd_enable(void)
{
	LCD_BL_ON();

	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
		LCD_WR_REG(0X29);	//开启显示
	else if(lcd_dev.drv_type == 0x1289)
		LCD_WriteReg(0x0010,0x0000);  //the driver leaves the sleep mode 
	else if(lcd_dev.drv_type == 0x8347)
		asm("nop");
	else    //LGDP4531/LGDP4535/SPFD5408/ILI9325
		LCD_WriteReg(0x07,0x0133);    //开启显示
}

void lcd_disable(void)
{
	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
		LCD_WR_REG(0X28);	//关闭显示
	else if(lcd_dev.drv_type == 0x1289)
		LCD_WriteReg(0x0010,0x0001);  //the driver enters into the sleep mode 
	else if(lcd_dev.drv_type == 0x8347)
		asm("nop");
	else    //LGDP4531/LGDP4535/SPFD5408/ILI9325
		LCD_WriteReg(0x07,0x0);       //关闭显示

	LCD_BL_OFF();
}

static void __lcd_dir_ctrl_r36(lcd_params_t *lcd)
{
	/* ILI9341
	 * Command: 0x36
	 * Parameter: MY MX MV ML BGR MH 0 0
	 *            MY-Row Address Order
	 *            MX-Column Address Order
	 *            MV-Row/Column Exchange
	 *            ML-Vertical Refresh Order
	 *            BGR-Color selector switch control(GRAM-->LCD Panel)  
	 *            MH-Horizontal Refresh Order
	 */
	uint16_t reg = 0x36;
	uint16_t val = 0;

	if( lcd->dir_ctrl.swap_x_y ) {
		lcd->xres = LCD_YRES;
		lcd->yres = LCD_XRES;
		val |= 1<<5;

		if(lcd->dir_ctrl.rev_x) {
			val |= 1<<7;
		}
		if(lcd->dir_ctrl.rev_y) {
			val |= 1<<6;
		}
	}
	else {
		lcd->xres = LCD_XRES;
		lcd->yres = LCD_YRES;
//		val |= 0<<5;
		if(lcd->dir_ctrl.rev_x) {
			val |= 1<<6;
		}
		if(lcd->dir_ctrl.rev_y) {
			val |= 1<<7;
		}
	}

	val |= 0x08;     //GRAM-->LCD Panel: 0=RGB color filter panel, 1=BGR color filter panel
	LCD_WriteReg(reg, val);	
}

static void __lcd_dir_ctrl_r11(lcd_params_t *lcd)
{
	/* SSD1289 R11
	 */
	uint16_t reg = 0x11;
	uint16_t val = 0x6040;

	int scan_dir = lcd->dir_ctrl.swap_x_y<<2 | lcd->dir_ctrl.rev_y<<1 | lcd->dir_ctrl.rev_x;

	if( lcd->dir_ctrl.swap_x_y )
	{
		lcd->xres = LCD_YRES;
		lcd->yres = LCD_XRES;
	}
	else
	{
		lcd->xres = LCD_XRES;
		lcd->yres = LCD_YRES;	
	}

	switch(scan_dir)
	{
		case 0:
			val |= 0<<3 | 1<<4 | 1<<5;
			break;
		case 1:
			val |= 0<<3 | 0<<4 | 1<<5;
			break;
		case 2:
			val |= 0<<3 | 1<<4 | 0<<5;
			break;
		case 3:
			val |= 0<<3 | 0<<4 | 0<<5;
			break;
		case 4:
			val |= 1<<3 | 1<<4 | 1<<5;
			break;
		case 5:
			val |= 1<<3 | 1<<4 | 0<<5;
			break;
		case 6:
			val |= 1<<3 | 0<<4 | 1<<5;
			break;
		case 7:
			val |= 1<<3 | 0<<4 | 0<<5;
			break;
	}

	LCD_WriteReg(reg, val);	
}

static void __lcd_dir_ctrl_r03(lcd_params_t *lcd)
{
	/* LGDP4531/LGDP4535/SPFD5408/ILI9325 R3
	 */
	uint16_t reg = 0x03;
	uint16_t val = 0;

	int scan_dir = lcd->dir_ctrl.swap_x_y<<2 | lcd->dir_ctrl.rev_y<<1 | lcd->dir_ctrl.rev_x;

	if( lcd->dir_ctrl.swap_x_y )
	{
		lcd->xres = LCD_YRES;
		lcd->yres = LCD_XRES;
	}
	else
	{
		lcd->xres = LCD_XRES;
		lcd->yres = LCD_YRES;	
	}
	
	switch(scan_dir)
	{
		case 0:
			val |= 0<<3 | 1<<4 | 1<<5;
			break;
		case 1:
			val |= 0<<3 | 0<<4 | 1<<5;
			break;
		case 2:
			val |= 0<<3 | 1<<4 | 0<<5;
			break;
		case 3:
			val |= 0<<3 | 0<<4 | 0<<5;
			break;
		case 4:
			val |= 1<<3 | 1<<4 | 1<<5;
			break;
		case 5:
			val |= 1<<3 | 1<<4 | 0<<5;
			break;
		case 6:
			val |= 1<<3 | 0<<4 | 1<<5;
			break;
		case 7:
			val |= 1<<3 | 0<<4 | 0<<5;
			break;
	}
	
	val |= (uint16_t)1<<12;
	LCD_WriteReg(reg, val);
}

static void __lcd_dir_ctrl_r16(lcd_params_t *lcd)
{
	/* HX8347D
	 * Command: 0x16
	 * Parameter: MY MX MV ML BGR 0 0 0
	 *            MY-Row Address Order
	 *            MX-Column Address Order
	 *            MV-Row/Column Exchange
	 *            ML-Vertical Refresh Order, Maybe no use or no sense
	 *            BGR- 0-RGB 1-BGR 
	 *            MH-Horizontal Refresh Order, Maybe no use or no sense
	 */
	uint16_t reg = 0x16;
	uint16_t val = 0;

	if( lcd->dir_ctrl.swap_x_y )
	{
		lcd->xres = LCD_YRES;
		lcd->yres = LCD_XRES;
		val |= 1<<5;
		if(lcd->dir_ctrl.rev_x) {
			val |= 1<<7;
		}
		if(lcd->dir_ctrl.rev_y) {
			val |= 1<<6;
		}
	}
	else
	{
		lcd->xres = LCD_XRES;
		lcd->yres = LCD_YRES;	
//		val |= 0<<5;
		if(lcd->dir_ctrl.rev_x) {
			val |= 1<<6;
		}
		if(lcd->dir_ctrl.rev_y) {
			val |= 1<<7;
		}	
	}

	val |= 0<<3;     //RGB order
	LCD_WriteReg(reg, val);
}

void lcd_dir_ctrl(lcd_params_t *lcd)
{
	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
		__lcd_dir_ctrl_r36(lcd);
	else if(lcd_dev.drv_type == 0x1289)
		__lcd_dir_ctrl_r11(lcd);
	else if(lcd_dev.drv_type == 0x8347)
		__lcd_dir_ctrl_r16(lcd);
	else                          //LGDP4531/LGDP4535/SPFD5408/ILI9325
		__lcd_dir_ctrl_r03(lcd);
}

void lcd_write_reg(uint16_t reg, uint16_t val)
{
	LCD_WriteReg(reg, val);
}

//===========================================================================================
/*
 * @brief: lcd清屏函数
 * @param: color-24bit RGB
 */
void lcd_clear(uint32_t color)
{
	int i, blk, cnt;
	
	color = color24to16(color);
	cnt = LCD_XRES * LCD_YRES;
	
	LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1);
	lcd_opt->reg = lcd_dev.wramcmd;  //相当于LCD_WriteRAM_Prepare()

	blk = cnt/8;
	for(i=0; i<blk; i++)             //往开辟的窗口中仍颜色数据即可
	{
		lcd_opt->ram = color;        //相当于LCD_WriteRAM(color)
		lcd_opt->ram = color;        //拆成向量形式, 防止反复修改PC指针, 加快执行速度
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
	}
	blk = cnt%8;
	for(i=0; i<blk; i++)
	{
		lcd_opt->ram = color;
	}
}


/*
 * @brief: 画点函数
 * @param: x∈[0, lcd_cur_sel->xres-1]  y∈[0, lcd_cur_sel->yres-1]
 */
void lcd_draw_point(uint32_t x, uint32_t y, uint32_t color)
{
	LCD_SetCursor(x, y);
	LCD_WriteRAM_Prepare();
	LCD_WriteRAM(color24to16(color));
}

/*
 * @brief: 画点函数
 * @param: x∈[0, lcd_cur_sel->xres-1]  y∈[0, lcd_cur_sel->yres-1]
 */
void lcd_draw_point_16b(uint32_t x, uint32_t y, uint16_t color)
{
	LCD_SetCursor(x, y);
	LCD_WriteRAM_Prepare();
	LCD_WriteRAM(color);
}


/*
 * @brief: 读点函数, 只支持24bpp(16bpp 18bpp都涉及颜色的拼装)
 * @param: x∈[0, lcd_cur_sel->xres-1]  y∈[0, lcd_cur_sel->yres-1]
 */
int  lcd_read_point(uint32_t x, uint32_t y)
{
	int i, ret;
	uint32_t buff[3];
	uint16_t color;
	volatile uint16_t dummy;

	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
	{
		LCD_SetCursor(x, y);

		//0x2E--Memory Read
		lcd_opt->reg = 0x2E;
		for(i=0; i<3; i++)
			buff[i] = lcd_opt->ram;

		//buff[0]--dummy, buff[1]--Red[15..11] Green[7..2], buff[2]--Blue[15..11]
		ret = (buff[1]&0xF800)<<8 | (buff[1]&0x00FC)<<8 | (buff[2]&0xF800)>>8;	
	}
	else if(lcd_dev.drv_type == 0x8347)
	{
		LCD_SetCursor(x,y);
		
		lcd_opt->reg = lcd_dev.wramcmd;    //HX8347D读GRAM 与 写GRAM都是这个命令
		for(i=0; i<3; i++)
			buff[i] = lcd_opt->ram;

		//buff[0]--dummy, buff[1]--Red[15..11] Green[7..2], buff[2]--Blue[15..11]
		ret = (buff[1]&0xF800)<<8 | (buff[1]&0x00FC)<<8 | (buff[2]&0xF800)>>8;	
	}
	else    //LGDP4531/SPFD5408/SSD1289/ILI9325
	{
		LCD_SetCursor(x, y);

		//R34--Memory Read
		lcd_opt->reg = 0x22;
		dummy = lcd_opt->ram;    //dummy read
		color = lcd_opt->ram;

		if(lcd_dev.drv_type == 0x5408)
			color = (color&0x001F)<<11 | (color&0x07E0) | (color&0xF800)>>11;  //BGR-->RGB

		ret = color16to24(color);	
	}

	return ret;
}

/*
 * @brief: 读点函数, 只支持24bpp(16bpp 18bpp都涉及颜色的拼装)
 * @param: x∈[0, lcd_cur_sel->xres-1]  y∈[0, lcd_cur_sel->yres-1]
 */
int  lcd_read_point_16b(uint32_t x, uint32_t y)
{
	int i, ret;
	uint32_t buff[3];
	uint16_t color;
	volatile uint16_t dummy;

	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
	{
		LCD_SetCursor(x, y);

		//0x2E--Memory Read
		lcd_opt->reg = 0x2E;
		for(i=0; i<3; i++)
			buff[i] = lcd_opt->ram;

		//buff[0]--dummy, buff[1]--Red[15..11] Green[7..2], buff[2]--Blue[15..11]
		ret = (buff[1]&0xF800) | (buff[1]&0x00FC)<<3 | (buff[2]&0xF800)>>11;
	}
	else if(lcd_dev.drv_type == 0x8347)
	{
		LCD_SetCursor(x,y);

		lcd_opt->reg = lcd_dev.wramcmd;    //HX8347D读GRAM 与 写GRAM都是这个命令
		for(i=0; i<3; i++)
			buff[i] = lcd_opt->ram;

		//buff[0]--dummy, buff[1]--Red[15..11] Green[7..2], buff[2]--Blue[15..11]
		ret = (buff[1]&0xF800) | (buff[1]&0x00FC)<<3 | (buff[2]&0xF800)>>11;
	}
	else    //LGDP4531/LGDP4535/SPFD5408/SSD1289/ILI9325
	{
		LCD_SetCursor(x, y);

		//Memory Read
		lcd_opt->reg = 0x22;
		dummy = lcd_opt->ram;    //dummy read
		color = lcd_opt->ram;

		if(lcd_dev.drv_type == 0x5408)
			color = (color&0x001F)<<11 | (color&0x07E0) | (color&0xF800)>>11;  //BGR-->RGB
		
		ret = color;
	}

	return ret;
}

/*
 * @brief: 在指定区域内填充指定颜色
 * @param: 区域{[x, y]| sx<=x<=ex, sy<=y<=ey}  color-要填充的颜色
 * @ret:   填充的像素个数
 */
int  lcd_fill_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t color)
{
	if(sx>ex || sy>ey)
		return 0;

	int i;
	int blk, total = (ey-sy+1)*(ex-sx+1);
	
	color = color24to16(color);
	LCD_Set_Window(sx, sy, ex, ey);  //设置窗口
	LCD_WriteRAM_Prepare();     	 //开始写入GRAM
	blk = total/8;
	for(i=0; i<blk; i++)
	{
		lcd_opt->ram = color;  //LCD_WriteRAM(color);
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
	}
	blk = total%8;
	for(i=0; i<blk; i++)
	{
		lcd_opt->ram = color;
	}
	LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)

	return total;
}

/*
 * @brief: 在指定区域内填充指定颜色
 * @param: 区域{[x, y]| sx<=x<=ex, sy<=y<=ey}  color-要填充的颜色
 * @ret:   填充的像素个数
 */
int  lcd_fill_area_16b(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t color)
{
	if(sx>ex || sy>ey)
		return 0;

	int i;
	int blk, total = (ey-sy+1)*(ex-sx+1);

	LCD_Set_Window(sx, sy, ex, ey);  //设置窗口
	LCD_WriteRAM_Prepare();     	 //开始写入GRAM
	blk = total/8;
	for(i=0; i<blk; i++)
	{
		lcd_opt->ram = color;  //LCD_WriteRAM(color);
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
		lcd_opt->ram = color;
	}
	blk = total%8;
	for(i=0; i<blk; i++)
	{
		lcd_opt->ram = color;
	}
	LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)

	return total;
}


/*
 * @brief: 在指定区域内填充指定颜色
 * @param: 区域{[x, y]| sx<=x<=ex, sy<=y<=ey}  color-要填充的颜色数组
 * @ret:   填充的像素个数
 */
int  lcd_colorfill_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t *color)
{
	int i, blk;
	int total = (ey-sy+1)*(ex-sx+1);
	
	LCD_Set_Window(sx, sy, ex, ey);  //设置窗口
	LCD_WriteRAM_Prepare();     	 //开始写入GRAM
	blk = total/8;
	for(i=0; i<blk; i++)             //LCD_WriteRAM(color);
	{
		lcd_opt->ram = color24to16(color[0]);
		lcd_opt->ram = color24to16(color[1]);
		lcd_opt->ram = color24to16(color[2]);
		lcd_opt->ram = color24to16(color[3]);
		lcd_opt->ram = color24to16(color[4]);
		lcd_opt->ram = color24to16(color[5]);
		lcd_opt->ram = color24to16(color[6]);
		lcd_opt->ram = color24to16(color[7]);
		color += 8;
	}
	blk = total%8;
	for(i=0; i<blk; i++)
	{
		lcd_opt->ram = color24to16(color[0]);
		color += 1;
	}
	LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)

	return total;
}

/*
 * @brief: 在指定区域内填充指定颜色
 * @param: 区域{[x, y]| sx<=x<=ex, sy<=y<=ey}  color-要填充的颜色数组
 * @ret:   填充的像素个数
 */
int  lcd_colorfill_area_16b(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t *color)
{
	int i, blk;
	int total = (ey-sy+1)*(ex-sx+1);
	
	LCD_Set_Window(sx, sy, ex, ey);  //设置窗口
	LCD_WriteRAM_Prepare();     	 //开始写入GRAM
	blk = total/8;
	for(i=0; i<blk; i++)           //LCD_WriteRAM(color);
	{
		lcd_opt->ram = color[0];   //拆成向量,加快执行速度
		lcd_opt->ram = color[1];
		lcd_opt->ram = color[2];
		lcd_opt->ram = color[3];
		lcd_opt->ram = color[4];
		lcd_opt->ram = color[5];
		lcd_opt->ram = color[6];
		lcd_opt->ram = color[7];
		color += 8;
	}
	blk = total%8;
	for(i=0; i<blk; i++)
	{
		lcd_opt->ram = color[0];
		color += 1;
	}
	LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)

	return total;
}


/*
 * @brief: 读取指定区域的颜色
 * @param: 区域{[x, y]| sx<=x<=ex, sy<=y<=ey}  color-存放的颜色数组
 * @ret:   读取的像素个数
 */
int  lcd_read_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t *color, int size)
{
	int total = (ey-sy+1)*(ex-sx+1);
	
	if(size < total)
		return 0;
	
	int i, idx, db2;
	int height, width;
	uint16_t rd1, rd2, rd3;
	volatile uint16_t dummy;

	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
	{
		LCD_Set_Window(sx, sy, ex, ey);       //Set Window
//		LCD_SetCursor(sx, sy);                //Not necessary
		LCD_WR_REG(0x2E);                     //0x2E--Memory Read
		dummy = lcd_opt->ram;

		/* ILI9341(8080 mode 16bit interface I)边读取边组装数据 
		 * 每读次连续读出6字节, 可以拼装成2个16bit的颜色值数据
		 */
		idx = 0;
		db2 = total/2;
		for(i=0; i<db2; i++)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			rd3 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800)<<8 | (rd1&0x00FC)<<8 | (rd2&0xF800)>>8;
			color[idx++] = (rd2&0x00F8)<<16 | (rd3&0xFC00) | (rd3&0x00F8);
		}
		if(total % 2)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800)<<8 | (rd1&0x00FC)<<8 | (rd2&0xF800)>>8;
		}
		
		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)
	}
	else if(lcd_dev.drv_type == 0x1289)
	{
		LCD_Set_Window(sx, sy, ex, ey);  //设置窗口
		LCD_WR_REG(0x22);                //Memory Read
		dummy = lcd_opt->ram;
		
		idx = 0;
		db2 = total/2;
		for(i=0; i<db2; i++)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			color[idx++] = color16to24(rd1);
			color[idx++] = color16to24(rd2);
		}
		
		if(total % 2)
		{
			rd1 = lcd_opt->ram;
			color[idx++] = color16to24(rd1);
		}	

		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)
	}
	else if(lcd_dev.drv_type == 0x8347)
	{
		LCD_Set_Window(sx, sy, ex, ey);       //Set Window
		LCD_WR_REG(0x22);                     //0x22--HX8347D读/写GRAM都是这个命令
		dummy = lcd_opt->ram;
		
		/* HX8347D(8080 mode 16bit interface I)边读取边组装数据 
		 * 每读次连续读出6字节, 可以拼装成2个16bit的颜色值数据
		 */
		idx = 0;
		db2 = total/2;
		for(i=0; i<db2; i++)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			rd3 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800)<<8 | (rd1&0x00FC)<<8 | (rd2&0xF800)>>8;
			color[idx++] = (rd2&0x00F8)<<16 | (rd3&0xFC00) | (rd3&0x00F8);
		}
		if(total % 2)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800)<<8 | (rd1&0x00FC)<<8 | (rd2&0xF800)>>8;
		}

		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)
	}
	else if(lcd_dev.drv_type == 0x5408)
	{
		idx = 0;
		for(height=sy; height<=ey; height++)
		{
			for(width=sx; width<=ex; width++)
			{
				LCD_SetCursor(width, height);      //Set Cursor
				LCD_WR_REG(0x22);                  //R34--Memory Read
				dummy = lcd_opt->ram;
				rd1 = lcd_opt->ram;
				rd1 = (rd1&0x001F)<<11 | (rd1&0x07E0) | (rd1&0xF800)>>11;   //BGR-->RGB
				color[idx++] = color16to24(rd1);
			}
		}	
	}
	else    //LGDP4531/LGDP4535/ILI9325
	{
		idx = 0;
		for(height=sy; height<=ey; height++)
		{
			for(width=sx; width<=ex; width++)
			{
				LCD_SetCursor(width, height);      //Set Cursor
				LCD_WR_REG(0x22);                  //R34--Memory Read
				dummy = lcd_opt->ram;
				rd1 = lcd_opt->ram;
				color[idx++] = color16to24(rd1);
			}
		}		
	}

	return total;
}



/*
 * @brief: 读取指定区域的颜色
 * @param: 区域{[x, y]| sx<=x<=ex, sy<=y<=ey}  color-存放的颜色数组
 * @ret:   读取的像素个数
 */
int  lcd_read_area_16b(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t *color, int size)
{
	int total = (ey-sy+1)*(ex-sx+1);

	if(size < total)
		return 0;

	int i, idx, db2;
	int height, width;
	uint16_t rd1, rd2, rd3;
	volatile uint16_t dummy;
	
	if(lcd_dev.drv_type==0x9341 || lcd_dev.drv_type==0x7789)
	{
		LCD_Set_Window(sx, sy, ex, ey);       //Set Window
//		LCD_SetCursor(sx, sy);                //Not necessary
		LCD_WR_REG(0x2E);                     //0x2E--Memory Read
		dummy = lcd_opt->ram;
		
		/* ILI9341(8080 mode 16bit interface I)边读取边组装数据 
		 * 每读次连续读出6字节, 可以拼装成2个16bit的颜色值数据
		 */
		idx = 0;
		db2 = total/2;
		for(i=0; i<db2; i++)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			rd3 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800) | (rd1&0x00FC)<<3 | (rd2&0xF800)>>11;
			color[idx++] = (rd2&0x00F8)<<8 | (rd3&0xFC00)>>5 | (rd3&0x00F8)>>3;
		}
		if(total % 2)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800) | (rd1&0x00FC)<<3 | (rd2&0xF800)>>11;
		}
		
		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)
	}
	else if(lcd_dev.drv_type == 0x1289)
	{
		LCD_Set_Window(sx, sy, ex, ey);  //设置窗口
		LCD_WR_REG(0x22);                //Memory Read
		dummy = lcd_opt->ram;
		
		idx = 0;
		db2 = total/2;
		for(i=0; i<db2; i++)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			color[idx++] = rd1;
			color[idx++] = rd2;
		}
		
		if(total % 2)
		{
			rd1 = lcd_opt->ram;
			color[idx++] = rd1;
		}	

		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)
	}
	else if(lcd_dev.drv_type == 0x8347)
	{
		LCD_Set_Window(sx, sy, ex, ey);       //Set Window
		LCD_WR_REG(0x22);                     //0x22--HX8347D读/写GRAM都是这个命令
		dummy = lcd_opt->ram;
		
		/* HX8347D(8080 mode 16bit interface I)边读取边组装数据 
		 * 每读次连续读出6字节, 可以拼装成2个16bit的颜色值数据
		 */
		idx = 0;
		db2 = total/2;
		for(i=0; i<db2; i++)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			rd3 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800) | (rd1&0x00FC)<<3 | (rd2&0xF800)>>11;
			color[idx++] = (rd2&0x00F8)<<8 | (rd3&0xFC00)>>5 | (rd3&0x00F8)>>3;
		}
		if(total % 2)
		{
			rd1 = lcd_opt->ram;
			rd2 = lcd_opt->ram;
			color[idx++] = (rd1&0xF800) | (rd1&0x00FC)<<3 | (rd2&0xF800)>>11;
		}

		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1); //把窗口再设置成原样(整屏)	
	}
	else if(lcd_dev.drv_type == 0x5408)
	{
		idx = 0;
		for(height=sy; height<=ey; height++)
		{
			for(width=sx; width<=ex; width++)
			{
				LCD_SetCursor(width, height);      //Set Cursor
				LCD_WR_REG(0x22);                  //R34--Memory Read
				dummy = lcd_opt->ram;
				rd1 = lcd_opt->ram;
				rd1 = (rd1&0x001F)<<11 | (rd1&0x07E0) | (rd1&0xF800)>>11;   //BGR-->RGB
				color[idx++] = rd1;
			}
		}	
	}
	else    //LGDP4531/LGDP4535/ILI9325
	{
		idx = 0;
		for(height=sy; height<=ey; height++)
		{
			for(width=sx; width<=ex; width++)
			{
				LCD_SetCursor(width, height);      //Set Cursor
				LCD_WR_REG(0x22);                  //R34--Memory Read
				dummy = lcd_opt->ram;
				rd1 = lcd_opt->ram;
				color[idx++] = rd1;
			}
		}		
	}	

	return total;
}

//====================================================================
//DMA刷屏, 这样可以实现刷屏与CPU的并行工作

/** @brief LCD DMA传输初始化
  * @reval 0-成功   非0-失败
  */
int  lcd_dma_init(void)
{
	#if 0
	//只有DMA2能实现存储器到存储器搬运数据
	DMA_InitTypeDef DMA_InitStructure;
	DMA_DeInit(DMA2_Stream1);
//	DMA_Cmd(DMA2_Stream1, DISABLE);
	while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE)  //等待DMA可配置
	{	}

	DMA_InitStructure.DMA_Channel = DMA_Channel_7;                       //DMA传输触发源 dummy
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t )0x20000000;    //DMA外设地址 dummy
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&lcd_opt->ram;     //DMA存储器0地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToMemory;                  //memory-->memory
	DMA_InitStructure.DMA_BufferSize = 0;                                //数据传输量 dummy
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;      //外设增量模式
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;             //把LCD看做内存,此时内存地址没必要递增.
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //外设数据长度
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;         //存储器数据长度
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                        //使用正常模式 
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;                //中等优先级
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;      
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;          //存储器突发单次传输
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  //外设突发单次传输
	DMA_Init(DMA2_Stream1, &DMA_InitStructure);  	//初始化DMA Stream

	/* 使用STM32F407的DMA时, 要牢记:
	 * DMA使能后, 无论是使用软件DMA_Cmd(DMA2_Stream1, DISABLE)关闭DMA,
	 * 还是DMA硬件自动清除使能位(DMA传输完成), 都会触发相关中断标志位(DMA_FLAG_TCIF1等).
	 * 所以, 无论是软件主动关闭DMA, 还是硬件在传输完成后自动关闭DMA,
	 * 如果有必要, 都要清除相关标志位.
	 */
	DMA_Cmd(DMA2_Stream1, DISABLE);          //先关闭,使用时打开
	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1);
	#endif
	return 0;
}

/**
 ** @brief 清除LCD DMA传输通管道配置信息, 使该通道DeInit
 **/
void lcd_dma_deinit(void)
{
	if(DMA_GetCurrDataCounter(DMA2_Stream1) != 0)  //还未完成最后一次传输完成
	{
		while(DMA_GetFlagStatus(DMA2_Stream1, DMA_FLAG_TCIF1) == RESET)  //等待最后一次传输完成
		{	}
	}
	if((DMA2_Stream1->CR & DMA_SxCR_EN) != 0)
	{
		DMA2_Stream1->CR &= (uint32_t)~DMA_SxCR_EN;      //关闭DMA
		while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE)
		{	}
	}
	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1);
	DMA_DeInit(DMA2_Stream1);
//	DMA_Cmd(DMA2_Stream1, DISABLE);    //unnecessary

	LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1);
}

/**
 ** @brief 使用DMA往LCD发送pixel数据
 ** @tip   使用该函数前要先设置LCD窗口, 并发出写GRAM命令
 **/
void lcd_dma_trans_line(uint16_t *pixel, int pixel_size)
{
	if(DMA_GetCurrDataCounter(DMA2_Stream1) != 0)  //还未完成上次传输
	{
		while(DMA_GetFlagStatus(DMA2_Stream1, DMA_FLAG_TCIF1) == RESET)  //等待传输完成
		{
//			my_printf("DMA busying, waiting\n\r");
		}
	}
	if((DMA2_Stream1->CR & DMA_SxCR_EN) != 0)
	{
		DMA2_Stream1->CR &= (uint32_t)~DMA_SxCR_EN;      //关闭DMA
		while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE) //Necessary
		{	}
	}
	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1);

	DMA2_Stream1->PAR = (uint32_t)pixel;
//	DMA2_Stream1->M0AR = (uint32_t)&lcd_opt->ram;  //unnecessary
	DMA2_Stream1->NDTR = (uint16_t)pixel_size;
	DMA2_Stream1->CR |= (uint32_t)DMA_SxCR_EN;     //开启DMA
}

/**
 ** @brief 使用DMA往LCD发送pixel数据
 ** @tip   该函数会自动设置LCD窗口, 并发出写GRAM命令
 **/
void lcd_dma_trans_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t *pixel)
{
	//等待DMA释放LCD controller
	if(DMA_GetCurrDataCounter(DMA2_Stream1) != 0)  //还未完成上次传输
	{
		while(DMA_GetFlagStatus(DMA2_Stream1, DMA_FLAG_TCIF1) == RESET)  //等待传输完成
		{	__NOP();	}
	}
	if((DMA2_Stream1->CR & DMA_SxCR_EN) != 0)
	{
		DMA2_Stream1->CR &= (uint32_t)~DMA_SxCR_EN;      //关闭DMA
		while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE) //Necessary
		{	}
	}
	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1);

	//设置LCD窗口, 并发出写GRAM指令
	LCD_Set_Window(sx, sy, ex, ey);  //设置窗口
	LCD_WriteRAM_Prepare();     	 //开始写入GRAM

	//开启DMA传输
	int pixel_size = (ey-sy+1)*(ex-sx+1);
	DMA2_Stream1->PAR = (uint32_t)pixel;
//	DMA2_Stream1->M0AR = (uint32_t)&lcd_opt->ram;  //unnecessary
	DMA2_Stream1->NDTR = (uint16_t)pixel_size;
	DMA2_Stream1->CR |= (uint32_t)DMA_SxCR_EN;     //开启DMA
}

/**
 ** @brief 使DMA释放LCD controller
 ** @tip   DMA与CPU不可同时操控LCD controller, 否则会导致时序混乱, LCD像素显示异常
 **/
void lcd_dma_trans_release(void)
{
	//等待DMA释放LCD controller
	if(DMA_GetCurrDataCounter(DMA2_Stream1) != 0)  //还未完成上次传输
	{
		while(DMA_GetFlagStatus(DMA2_Stream1, DMA_FLAG_TCIF1) == RESET)  //等待传输完成
		{	__NOP();	}
	}
	if((DMA2_Stream1->CR & DMA_SxCR_EN) != 0)
	{
		DMA2_Stream1->CR &= (uint32_t)~DMA_SxCR_EN;      //关闭DMA
		while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE)
		{	}
	}
	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1);

	LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1);
}




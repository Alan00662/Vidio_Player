#ifndef  __LCD_DRV_FSMC407_V6_H_
#define  __LCD_DRV_FSMC407_V6_H_

#include <stdint.h>

//竖屏模式下(即swap_x_y,rev_x,rev_y三者都为0时)LCD的像素分辨率
#define  LCD_XRES  240
#define  LCD_YRES  320

//结合<<计算机组成原理>>理解FSMC驱动16bit的LCD
typedef struct lcd_opt_s
{
	volatile uint16_t reg;
	volatile uint16_t ram;
}lcd_opt_t;

//0x60000000-Bank1 region4  FSMC_A6--LCD_DC(0-Command 1-Data)
#define  lcd_base  (0x6C000000ul | 0x0000007Eul)  
#define  lcd_opt   ((lcd_opt_t *)lcd_base)

//屏幕方向控制
typedef struct dir_ctrl_s
{
	int swap_x_y; //x与y轴是否需要调换 0-不调换  1-调换
	int rev_x;    //x方向是否需要翻转  0-不翻转  1-翻转
	int rev_y;    //y方向是否需要翻转  0-不翻转  1-翻转
}dir_ctrl_t;

//一个LCD屏幕的参数
typedef struct lcd_params_s {
	char *name;
	uint16_t drv_type;  //显卡型号, 如9341

	dir_ctrl_t  dir_ctrl;
	int xres;           /*水平方向分辨率*/
	int yres;           /*竖直方向分辨率*/
	uint16_t wramcmd;   //开始写gram指令
	uint16_t setxcmd;	//设置x坐标指令
	uint16_t setycmd;	//设置y坐标指令

	uint32_t forecolor;
	uint32_t backcolor;
}lcd_params_t;

extern lcd_params_t lcd_dev;

/* 颜色 */
#define LCD_BLUE		  0x000000FF
#define LCD_GREEN		  0x0000FF00
#define LCD_RED 		  0x00FF0000
#define LCD_CYAN		  0x0000FFFF
#define LCD_MAGENTA 	  0x00FF00FF
#define LCD_YELLOW		  0x00FFFF00
#define LCD_LIGHTBLUE	  0x008080FF
#define LCD_LIGHTGREEN	  0x0080FF80
#define LCD_LIGHTRED	  0x00FF8080
#define LCD_LIGHTCYAN	  0x0080FFFF
#define LCD_LIGHTMAGENTA  0x00FF80FF
#define LCD_LIGHTYELLOW   0x00FFFF80
#define LCD_DARKBLUE	  0x00000080
#define LCD_DARKGREEN	  0x00008000
#define LCD_DARKRED 	  0x00800000
#define LCD_DARKCYAN	  0x00008080
#define LCD_DARKMAGENTA   0x00800080
#define LCD_DARKYELLOW	  0x00808000
#define LCD_WHITE		  0x00FFFFFF
#define LCD_LIGHTGRAY	  0x00D3D3D3
#define LCD_GRAY		  0x00808080
#define LCD_DARKGRAY	  0x00404040
#define LCD_BLACK		  0x00000000
#define LCD_BROWN		  0x00A52A2A
#define LCD_ORANGE		  0x00FFA500

// 把24bit的颜色转换为16bit
//__attribute__( ( always_inline ) ) static inline uint32_t color24to16(uint32_t color)
//{
//	uint32_t r = (color & 0x00FF0000) >> (16+3);
//	uint32_t g = (color & 0x0000FF00) >> (8+2);
//	uint32_t b = (color & 0x000000FF) >> (0+3);
//	color = (r << 11) | (g << 5) | (b << 0);  //RGB565 format
//	return color;
//}
#define  color24to16(color)  (uint16_t)(((color)&0xF80000)>>8 | ((color)&0x00FC00)>>5 | ((color)&0x0000F8)>>3)


//把16bit的颜色转换为24bit   使用时再放开注释
//__attribute__( ( always_inline ) ) static inline uint32_t color16to24(uint32_t color)
//{
//	uint32_t r = (color & 0xF800) >> 11;
//	uint32_t g = (color & 0x07E0) >> 5;
//	uint32_t b = (color & 0x001F) >> 0;
//	color = (r << (16+3)) | (g << (8+2)) | (b << (0+3));
//	return color;
//}
#define  color16to24(color)  (uint32_t)(((color)&0xF800)<<8 | ((color)&0x07E0)<<5 | ((color)&0x001F)<<3)


//================================================================
void LCD_Set_Window(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);
void LCD_WriteRAM_Prepare(void);

int  lcd_init(void);
void lcd_enable(void);
void lcd_disable(void);
void lcd_dir_ctrl(lcd_params_t *lcd);
void lcd_write_reg(uint16_t reg, uint16_t val);
void lcd_backlight_pwm(uint16_t pwm_duty);

void lcd_clear(uint32_t color);
void lcd_draw_point(uint32_t x, uint32_t y, uint32_t color);
void lcd_draw_point_16b(uint32_t x, uint32_t y, uint16_t color);
int  lcd_read_point(uint32_t x, uint32_t y);
int  lcd_read_point_16b(uint32_t x, uint32_t y);
int  lcd_fill_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t color);
int  lcd_fill_area_16b(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t color);
int  lcd_colorfill_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t *color);
int  lcd_colorfill_area_16b(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t *color);
int  lcd_read_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t *color, int size);
int  lcd_read_area_16b(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t *color, int size);

//LCD DMA传输控制
int  lcd_dma_init(void);
void lcd_dma_deinit(void);
void lcd_dma_trans_line(uint16_t *pixel, int pixel_size);
void lcd_dma_trans_area(uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint16_t *pixel);
void lcd_dma_trans_release(void);


#endif




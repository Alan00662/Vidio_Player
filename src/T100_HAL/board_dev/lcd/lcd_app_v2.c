#include "lcd_app_v2.h"
#include "font.h"

/*
 * @brief: 在指定位置画一个指定大小的圆
 * @param: (x0, y0)为圆心 r为半径  color为圆的颜色
 */
void lcd_draw_circle(uint32_t x0, uint32_t y0, uint32_t r, uint32_t color)
{
	int a,b;
	int di;
	a=0;b=r;
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		lcd_draw_point(x0-b,y0-a,color);             //3
		lcd_draw_point(x0+b,y0-a,color);             //0
		lcd_draw_point(x0-a,y0+b,color);             //1
		lcd_draw_point(x0-b,y0-a,color);             //7
		lcd_draw_point(x0-a,y0-b,color);             //2
		lcd_draw_point(x0+b,y0+a,color);             //4
		lcd_draw_point(x0+a,y0-b,color);             //5
		lcd_draw_point(x0+a,y0+b,color);             //6
		lcd_draw_point(x0-b,y0+a,color);
		a++;
		//使用Bresenham算法画圆
		if(di<0)di +=4*a+6;
		else
		{
			di+=10+4*(a-b);
			b--;
		}
		lcd_draw_point(x0+a,y0+b,color);
	}
}

/*
 * @brief: 画直线
 * @param: x1,y1:起点坐标  x2,y2:终点坐标  color:直线的颜色
 */
void lcd_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
{
	uint32_t t;
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;

	delta_x=x2-x1; //计算坐标增量
	delta_y=y2-y1;
	uRow=x1;
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向
	else if(delta_x==0)incx=0;//垂直线
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if(delta_y==0)incy=0;//水平线
	else{incy=-1;delta_y=-delta_y;}
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
	else distance=delta_y;
	for(t=0;t<=distance+1;t++ )//画线输出
	{
		lcd_draw_point(uRow,uCol,color);//画点
		xerr+=delta_x ;
		yerr+=delta_y ;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

/*
 * @brief: 画矩形
 * @param: (x1,y1),(x2,y2):矩形的对角坐标
 */
void lcd_draw_rectangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
{
	lcd_draw_line(x1,y1,x2,y1,color);
	lcd_draw_line(x1,y1,x1,y2,color);
	lcd_draw_line(x1,y2,x2,y2,color);
	lcd_draw_line(x2,y1,x2,y2,color);
}

//=====================================================================================
/*
 * @brief: 在指定位置显示一个字符    后续优化: 每次刷一行或者一列点,而不是一个一个的描点(已经完成优化)
 * @param: x,y: 起始坐标
 *         num: 要显示的字符 A B 1  ? : " "- > 等 从此函数可以推知字符的取模方式: 从上往下,从左到右
 *         size: 字体大小 12 or 16 or 24
 *         mode: 0-每次显示字体前都把字体区域的背景刷为屏幕背景(lcd_cur_sel->backcolor)
 *               1-刷为lcd_read_point(uint32_t x, uint32_t y)颜色值 (在图片上显示文字)
 *
 */
void lcd_show_char(uint32_t x, uint32_t y, int num, int size, int mode, uint32_t color)
{
    int temp, t1, t;
    uint16_t color_buff[24*12];           // 576 Byte
	int csize = (size/8+((size%8)?1:0))*(size/2);  //得到字体一个字符对应点阵集所占的字节数
	uint32_t y0 = y, x0=x;

	if(x >= lcd_dev.xres)	return;		                            //超区域了

	num = num-' ';	//得到偏移后的值
	if(size == 12) {
		uint16_t (*font12)[6] = (uint16_t (*)[])color_buff;
		for(t=0; t<csize; t++)
		{
			temp=asc2_1206[num][t]; 	 	//调用1206字体
			for(t1=0; t1<8; t1++)
			{
				if(temp&0x80)
					font12[y-y0][x-x0] = color24to16(color); //复杂的访存地址定位工作交给编译器
				else if(mode==0)
					font12[y-y0][x-x0] = color24to16(lcd_dev.backcolor);
				else if(mode==1)
					font12[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
				temp <<= 1;
				y++;
				if((y-y0) == size)
				{
					y=y0;
					x++;
					if(x>=lcd_dev.xres && t+1<csize)	return;	//超区域了
					break;
				}
			}
		}
		lcd_colorfill_area_16b(x0, y0, x0+12/2-1, y0+12-1, color_buff);
	}
	else if(size == 16) {
		uint16_t (*font16)[8] = (uint16_t (*)[])color_buff;
		for(t=0; t<csize; t++)
		{
			temp=asc2_1608[num][t]; 	 	//调用1608字体
			for(t1=0; t1<8; t1++)
			{
				if(temp&0x80)
					font16[y-y0][x-x0] = color24to16(color);
				else if(mode==0)
					font16[y-y0][x-x0] = color24to16(lcd_dev.backcolor);
				else if(mode==1)
					font16[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
				temp <<= 1;
				y++;
				if((y-y0) == size)
				{
					y=y0;
					x++;
					if(x>=lcd_dev.xres && t+1<csize)	return;	//超区域了
					break;
				}
			}
		}
		lcd_colorfill_area_16b(x0, y0, x0+16/2-1, y0+16-1, color_buff);
	}
	else {
		uint16_t (*font24)[12] = (uint16_t (*)[])color_buff;
		for(t=0; t<csize; t++)
		{
			temp=asc2_2412[num][t]; 	 	//调用2412字体
			for(t1=0; t1<8; t1++)
			{
				if(temp&0x80)
					font24[y-y0][x-x0] = color24to16(color);
				else if(mode==0)
					font24[y-y0][x-x0] = color24to16(lcd_dev.backcolor);
				else if(mode==1)
					font24[y-y0][x-x0] = color24to16(lcd_read_point(x, y));
				temp <<= 1;
				y++;
				if((y-y0) == size)
				{
					y=y0;
					x++;
					if(x>=lcd_dev.xres && t+1<csize)	return;	//超区域了
					break;
				}
			}
		}
		lcd_colorfill_area_16b(x0, y0, x0+24/2-1, y0+24-1, color_buff);
	}
}

/*
 * @brief: 显示字符串          显示区域 [x,x+width-1] [y,y+height-1]
 * @param: x,y:起点坐标
 *         width,height:区域大小
 *         size:字体大小
 *         p:字符串的存储地址
 */
void lcd_show_string(uint32_t x, uint32_t y, uint32_t width, uint32_t height, int size, char *p, int mode, uint32_t color)
{
	uint32_t x0 = x;
	width += x;
	height += y;
    while((*p<='~') && (*p>=' '))	//判断是不是非法字符!
    {
        if(x > width-size/2){x=x0;y+=size;}
        if(y>=height)break;//退出
        lcd_show_char(x, y, *p, size, mode, color);
        x+=size/2;
        p++;
    }
}


//校准时显示十字叉
void lcd_show_cross(uint32_t x, uint32_t y, uint32_t r, uint32_t color)
{
	lcd_draw_line(x-r, y, x+r, y, color);
	lcd_draw_line(x, y-r, x, y+r, color);
	lcd_draw_circle(x, y, r, color);
}




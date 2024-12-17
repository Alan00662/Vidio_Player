#include "ico_decode.h"
#include "lcd_app_v2.h"
#include "ff.h"
#include "my_printf.h"


/** @brief ����ALPHA BLENDING�㷨.
 ** @param src - lcd pixel data
 **        dst - image pixel data
 **        alpha - [0, 32]
 ** @reval ��Ϻ����ɫ
 **/
static uint16_t alpha_blend_16b(uint16_t src, uint16_t dst, uint8_t alpha)
{
	uint32_t src2;
	uint32_t dst2;	 
	//Convert to 32bit |-----GGGGGG-----RRRRR------BBBBB|
	src2=((src<<16)|src)&0x07E0F81F;
	dst2=((dst<<16)|dst)&0x07E0F81F;   
	//Perform blending R:G:B with alpha in range 0..32
	//Note that the reason that alpha may not exceed 32 is that there are only
	//5bits of space between each R:G:B value, any higher value will overflow
	//into the next component and deliver ugly result.
	dst2=((((dst2-src2)*alpha)>>5)+src2)&0x07E0F81F;
	return (dst2>>16)|dst2;  
}


/** @brief ����iconͼ��, ����LCD����ʾ
  * @param fname-iconͼ����
  *        lcd_x-LCD��ʾ�Ŀ�ʼx����
  *        lcd_y-LCD��ʾ�Ŀ�ʼy����
  * @reval 0-����ɹ�  ��0-����ʧ��
  * @tip   ͼƬ�����سߴ粻����32x32, ��ɫ�岻����16��
  */
int icon_decode(const char *fname, int lcd_x, int lcd_y)
{
	FIL fil;
	int ret;
	uint32_t rd_len;
	uint8_t  buff[32*4];
	uint32_t is_disp[32];
	uint16_t color565[256];   //���256����ɫ��
	uint16_t lcd_pixel[32];   //һ��LCD����
	
	ret = f_open(&fil, fname, FA_READ);
	if(ret != FR_OK)
	{
		return -1;
	}
	
	ret = f_read(&fil, buff, sizeof(icon_header_t), &rd_len);
	if(ret!=FR_OK || rd_len!=sizeof(icon_header_t))
	{
		f_close(&fil);
		return -2;
	}
	icon_header_t *icon_header_ptr = (icon_header_t *)buff;
	if(icon_header_ptr->reserved != 0x00 || icon_header_ptr->type != 0x01)
	{
		f_close(&fil);
		return -3;		
	}
	if(icon_header_ptr->count > 1)
	{	//ico�ļ��������ͼƬʱֻ�����һ��ͼƬ
		my_printf("Warning! %s contains %u image, only display the first one\n\r", fname, icon_header_ptr->count);
	}
	
	icon_pic_info_t icon_pic_info;
	ret = f_read(&fil, &icon_pic_info, sizeof(icon_pic_info_t), &rd_len);
	if(ret!=FR_OK || rd_len!=sizeof(icon_pic_info_t))
	{
		f_close(&fil);
		return -3;
	}
//	my_printf("icon_pic_info.width = %u\n\r", icon_pic_info.width);       //debug print
//	my_printf("icon_pic_info.height = %u\n\r", icon_pic_info.height);
//	my_printf("icon_pic_info.color_count = %u\n\n\r", icon_pic_info.color_count);
	if(icon_pic_info.width > 32 || icon_pic_info.height > 32)
	{	//ֻ�ܽ��벻���ز�����32x32��ͼƬ
		f_close(&fil);
		return -4;		
	}
	if(icon_pic_info.color_count > 16)
	{	//���õ�ɫ�岻�ܳ���16��
		f_close(&fil);
		return -5;		
	}
	
	f_lseek(&fil, icon_pic_info.image_offset);
	bmp_header_t bmp_header;
	ret = f_read(&fil, &bmp_header, sizeof(bmp_header_t), &rd_len);
	if(ret!=FR_OK || rd_len!=sizeof(bmp_header_t))
	{
		f_close(&fil);
		return -6;
	}
//	my_printf("bmp_header.bits_of_per_pix = %u\n\r", bmp_header.bits_of_per_pix);
//	my_printf("bmp_header.compress = 0x%08X\n\r", bmp_header.compress);
//	my_printf("bmp_header.pix_data_len = %u\n\n\r", bmp_header.pix_data_len);
	if(bmp_header.compress != 0x00)
	{	//��֧��ѹ��
		f_close(&fil);
		return -7;		
	}
	
	int i, t;
	uint8_t *pixel, alpha;
	uint32_t pixel_data_size = icon_pic_info.width*icon_pic_info.height * bmp_header.bits_of_per_pix/8;
	uint32_t pixel_line_size = (icon_pic_info.width * bmp_header.bits_of_per_pix/8 + 3) & ~0x03UL; //һ�����ص��ֽ��� 4�ֽڶ���
	uint32_t disp_info_size = 0;
	uint32_t pixel_data_start = 0;
//	my_printf("pixel_data_size = %u Bytes\n\r", pixel_data_size);
//	my_printf("pixel_line_size = %u Bytes\n\r", pixel_line_size);

	if(bmp_header.bits_of_per_pix==4 || bmp_header.bits_of_per_pix==8)   /* �е�ɫ��ʱ����icon */
	{	
		disp_info_size = bmp_header.pix_data_len - pixel_data_size;
//		my_printf("disp_info_size = %u Bytes\n\n\r", disp_info_size);
		
		//�ȶ�ȡ��ɫ����Ϣ
		uint32_t color_palette_num;
		color_palette_t *color_palette_ptr;
		color_palette_num = 1UL<<bmp_header.bits_of_per_pix;
		for(i=0; i<color_palette_num; i+=16)
		{
			ret = f_read(&fil, buff, sizeof(color_palette_t)*16, &rd_len);
			if(ret != FR_OK) {
				f_close(&fil);
				return -8;	
			}
			for(t=0; t<16; t++)
			{
				color_palette_ptr = (color_palette_t *)&buff[t*sizeof(color_palette_t)];
				color565[i+t] = (color_palette_ptr->r & 0xF8)<<8 | (color_palette_ptr->g & 0xFC)<<3 | (color_palette_ptr->b & 0xF8)>>3;
			}
		}
		
		//��ȡ͸������Ϣ
		pixel_data_start = fil.fptr;
		f_lseek(&fil, pixel_data_start+pixel_data_size);
		ret = f_read(&fil, is_disp, disp_info_size, &rd_len);
		if(ret!=FR_OK || rd_len!=disp_info_size) {
			f_close(&fil);
			return -9;		
		}
		for(i=0; i<icon_pic_info.height; i++)
		{
			is_disp[i] = (is_disp[i]&0x000000FF)<<24 | (is_disp[i]&0x0000FF00)<<8 | (is_disp[i]&0x00FF0000)>>8 | (is_disp[i]&0xFF000000)>>24;
		}

		//��ȡ����, ÿ�ν���һ��
		f_lseek(&fil, pixel_data_start);
		for(i=0; i<icon_pic_info.height; i++)
		{
			ret = f_read(&fil, buff, pixel_line_size, &rd_len);
			if(ret!=FR_OK || rd_len!=pixel_line_size) {
				f_close(&fil);
				return -10;	
			}
			
			if(bmp_header.bits_of_per_pix == 4) {
				for(t=0; t<pixel_line_size; t++)
				{
					lcd_pixel[2*t]   = color565[(buff[t]>>4)&0x0F];
					lcd_pixel[2*t+1] = color565[buff[t]&0x0F];
				}
			} else if(bmp_header.bits_of_per_pix == 8) {
				for(t=0; t<pixel_line_size; t++)
				{
					lcd_pixel[t] = color565[buff[t]];
				}				
			} else {
				f_close(&fil);
				return -11;				
			}

			for(t=0; t<icon_pic_info.width; t++)
			{	//����͸����
				if(is_disp[i] & 0x80000000) {
//					lcd_pixel[t] = lcd_read_point_16b(lcd_x+t, lcd_y+icon_pic_info.height-1-i);
					lcd_pixel[t] = color24to16(lcd_dev.backcolor);
				}
				is_disp[i] <<= 1;
			}
			
			lcd_colorfill_area_16b(lcd_x, lcd_y+icon_pic_info.height-1-i, lcd_x+icon_pic_info.width-1, lcd_y+icon_pic_info.height-1-i, lcd_pixel); //��LCD����ʾһ������
		}
		
//		my_printf("Decode palette icon success!\n\r");
	}
	else if(bmp_header.bits_of_per_pix == 24) /* �޵�ɫ��ʱ����RGB888 icon */
	{
		//��ȡ͸������Ϣ
		disp_info_size = icon_pic_info.height * 4;
//		my_printf("disp_info_size = %u Bytes\n\n\r", disp_info_size);

		pixel_data_start = fil.fptr;
		f_lseek(&fil, pixel_data_start+pixel_data_size);
		ret = f_read(&fil, is_disp, disp_info_size, &rd_len);
		if(ret!=FR_OK || rd_len!=disp_info_size) {
			f_close(&fil);
			return -9;		
		}
		for(i=0; i<icon_pic_info.height; i++)
		{
			is_disp[i] = (is_disp[i]&0x000000FF)<<24 | (is_disp[i]&0x0000FF00)<<8 | (is_disp[i]&0x00FF0000)>>8 | (is_disp[i]&0xFF000000)>>24;
		}
		
		//��ȡ����, ÿ�ν���һ��
		f_lseek(&fil, pixel_data_start);
		for(i=0; i<icon_pic_info.height; i++)
		{
			ret = f_read(&fil, buff, pixel_line_size, &rd_len);
			if(ret!=FR_OK || rd_len!=pixel_line_size) {
				f_close(&fil);
				return -10;	
			}
			
			pixel = buff;
			for(t=0; t<icon_pic_info.width; t++)
			{
				if(is_disp[i] & 0x80000000) {
//					lcd_pixel[t] = lcd_read_point_16b(lcd_x+t, lcd_y+icon_pic_info.height-1-i);
					lcd_pixel[t] = color24to16(lcd_dev.backcolor);
				}
				else
					lcd_pixel[t] = (pixel[2]&0xF8)<<8 | (pixel[1]&0xFC)<<3 | (pixel[0]&0xF8)>>3;
				pixel += 3;
				is_disp[i] <<= 1;
			}
			lcd_colorfill_area_16b(lcd_x, lcd_y+icon_pic_info.height-1-i, lcd_x+icon_pic_info.width-1, lcd_y+icon_pic_info.height-1-i, lcd_pixel); //��LCD����ʾһ������
		}
		
//		my_printf("Decode RGB888 icon success!\n\r");
	}
	else if(bmp_header.bits_of_per_pix == 32)   /* �޵�ɫ��ʱ����ARGB8888 icon */
	{
		uint16_t img_pixel_data;

		//��ȡ����, ÿ�ν���һ��(4�ֽڵ����ذ�����͸������Ϣ)
		pixel_data_start = fil.fptr;
//		f_lseek(&fil, pixel_data_start);
		for(i=0; i<icon_pic_info.height; i++)
		{
			ret = f_read(&fil, buff, pixel_line_size, &rd_len);
			if(ret!=FR_OK || rd_len!=pixel_line_size) {
				f_close(&fil);
				return -10;	
			}

			pixel = buff;
			for(t=0; t<icon_pic_info.width; t++)
			{
				alpha = pixel[3];                                                               //͸����
				img_pixel_data = (pixel[2]&0xF8)<<8 | (pixel[1]&0xFC)<<3 | (pixel[0]&0xF8)>>3;  //��ɫֵ
				if(alpha == 0xFF) {
					lcd_pixel[t] = img_pixel_data;
				}
				else {
//					lcd_pixel[t] = lcd_read_point_16b(lcd_x+t, lcd_y+icon_pic_info.height-1-i);
					lcd_pixel[t] = color24to16(lcd_dev.backcolor);
					if(alpha != 0x00) {
						lcd_pixel[t] = alpha_blend_16b(lcd_pixel[t], img_pixel_data, alpha>>3);
					}
				}
				pixel += 4;
			}
			lcd_colorfill_area_16b(lcd_x, lcd_y+icon_pic_info.height-1-i, lcd_x+icon_pic_info.width-1, lcd_y+icon_pic_info.height-1-i, lcd_pixel); //��LCD����ʾһ������
		}

//		my_printf("Decode ARGB8888 icon success!\n\r");
	}
	else
	{
		//2bpp 16bpp��ʱ��֧��
		my_printf("Error! not support %u bit image!!\n\r", bmp_header.bits_of_per_pix);
	}
	
	f_close(&fil);
	return 0;
}









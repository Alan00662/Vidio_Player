#ifndef  __ICO_DECODE_H_
#define  __ICO_DECODE_H_

#include <stdint.h>


//6 Bytes
typedef struct icon_header_s
{
	uint16_t reserved; //Reserved (must be 0)
	uint16_t type;     //Resource Type (1 for icons)
	uint16_t count;    //How many images?
}__attribute__((packed)) icon_header_t;

//16 Bytes
typedef struct icon_pic_info_s
{
	uint8_t width;  //Width, in pixels, of the image
	uint8_t height; //Height, in pixels, of the image
	uint8_t color_count; // Number of colors in image (0 if >=8bpp)
	uint8_t reserved1;
	uint32_t reserved2;
	uint32_t bytes_in_res;  //How many bytes in this resource?
	uint32_t image_offset;  //Where in the file is this image?
}__attribute__((packed)) icon_pic_info_t;

//40 Bytes
typedef struct bmp_header_s
{
	uint32_t this_struct_size;         //�ṹ�峤��
	uint32_t width;                    //ͼ����
	uint32_t height;                   //ͼ��߶�(XORͼ�߶�+ANDͼ�߶�)
	uint16_t not_clear;                //λ�����? �������2�ֽ��к�����
	uint16_t bits_of_per_pix;          //ÿ��������ռ��λ��
	uint32_t compress;                 //�������ݵ�ѹ������
	uint32_t pix_data_len;             //�������ݵĳ���
	uint8_t  reserved[16];             //16��0
}__attribute__((packed)) bmp_header_t;


//4 buyes
typedef struct color_palette_s
{
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t reserved;
}__attribute__((packed)) color_palette_t;



int icon_decode(const char *fname, int lcd_x, int lcd_y);


#endif




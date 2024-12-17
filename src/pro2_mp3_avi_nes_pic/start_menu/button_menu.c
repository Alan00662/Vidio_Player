#include "button_menu.h"
#include "lcd_button.h"
#include "dir_list.h"

#include "libc.h"
#include "share.h"
#include "key.h"
#include "lcd_app_v2.h"

static const button_t start_button[] = 
{
	{"���ֲ�����", "/music", "�����б�", 1, dir_list_form_load},
	{"��Ƶ������", "/video", "��Ƶ�б�", 1, dir_list_form_load},
	{"��Ϸ��",     "/nes",   "��Ϸ�б�", 1, dir_list_form_load},
	{"ͼƬ�����", "/picture", "ͼƬ�б�", 1, dir_list_form_load},
};

static const button_style_t button_style = 
{
	.form_image = "0:/sys/background_horizontal.bmp",
	.form_color = LCD_GREEN,

	.font_color = 0x000000,    //��ɫ
	.back_color = 0xE1E1E1,    //��ɫ
	.select_color = 0xFF00FF,  //��ɫ
	.select_mark = ">",

	.font_size = 24,        //֧��12/16/24�����������С
	.font_str_max_len = 10,
	.font_border_fill = 4,
	.obj_space_fill = 12,
};

static lcd_button_t button_menu = 
{
	.button = start_button,
	.style = &button_style,
	.button_cnt = ARRAY_SIZE(start_button),
	.focus_idx = 0,
};

//==========================================================
void work_dir_check_list(void)
{
	int lcd_x, lcd_y;
	char *const tip = "ϵͳ��������...";

	lcd_clear(lcd_dev.backcolor);
	lcd_x = (lcd_dev.xres - u_strlen(tip)*24/2)/2;
	lcd_y = lcd_dev.yres/2 - 24;
	lcd_show_string(lcd_x, lcd_y, lcd_dev.xres-lcd_x, 24, 24, tip, 0, lcd_dev.forecolor);

	lcd_button_work_dir_scan(button_menu.button, button_menu.button_cnt);
}


static void button_menu_click_action(void);
static void button_menu_move_prev(void);
static void button_menu_move_next(void);

void start_menu_load(void)
{
	//���ؽ���
	lcd_button_form_load(&button_menu);
	
	//ע�ᰴ��
	key_register_cb(KEY_SEL, button_menu_click_action, NULL, NULL, 0, 0);
	if(button_menu.button_cnt > 1)
	{
		key_register_cb(KEY_DEC, button_menu_move_prev, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, button_menu_move_next, NULL, NULL, 0, 0);		
	}
	else
	{
		key_unregister_cb(KEY_DEC);
		key_unregister_cb(KEY_INC);		
	}
}

//==========================================================
//button menu�����°����Ĺ���
static void button_menu_click_action(void)
{
	lcd_button_focus_click(&button_menu);
}

static void button_menu_move_prev(void)
{
	lcd_button_focus_prev(&button_menu);
}

static void button_menu_move_next(void)
{
	lcd_button_focus_next(&button_menu);
}



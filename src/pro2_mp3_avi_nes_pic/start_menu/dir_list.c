#include "dir_list.h"
#include "lcd_list.h"
#include "button_menu.h"
#include "run_app.h"

#include "lcd_app_v2.h"
#include "key.h"

#include "libc.h"
#include "my_printf.h"
#include "my_malloc.h"

//LCD list����
static const list_style_t style1 = 
{
	.title_font_size = 16, 
	.title_up_skip = 4,
	.title_down_skip = 4,
	.cnt_font_size = 12,
	.title_font_color = LCD_WHITE,
	.title_back_color = 0x0078D7,     //����

	.icon_size = 16,
	.icon_left_skip = 4,
	.icon_right_skip = 4,

	.item_font_size = 16,
	.item_up_skip = 4,
	.item_down_skip = 4,
	.item_font_color = LCD_BLACK,
	.item_back_color = LCD_WHITE,
	.item_sel_back_color = LCD_LIGHTGRAY,     //0xE5E5E5-ǳ��   LCD_LIGHTGRAY
};

static lcd_list_t lcd_list = 
{
	.sy = 0,           //LCD y����ʼ����
	.title = NULL,
	.item = NULL,      //ָ��item[], ��Ҫmalloc(sizeof(fil_obj_t)*total_item_num)
	.max_item_num = (240-(16+4+4))/(16+4+4), //���б���, ��item[]����Ԫ��������
	.real_item_num = 0,
	.sel_item_idx = 0, //ѡ�е��б���
	.style = &style1,
};

static dir_obj_t *dir_obj = NULL;

static int dir_item_idx = 0;             //��2�������ڲ��Ž�����ָ�dir�б�
static int lcd_sel_item_idx = 0;

static int dir_item_idx_sub = 0;
static int lcd_sel_item_idx_sub = 0;

//dir item�б�״̬��4�������Ļص�����
static void dir_item_close(void);
static void dir_item_select(void);
static void dir_item_slide_up(void);
static void dir_item_slide_down(void);

static void key_func_setting(int dir_item_num)
{
	if(dir_item_num == 0)
	{
		key_register_cb(KEY_SEL, NULL, dir_item_close, dir_item_close, 1000, 50);   //ע��3����Ϊ�˳�����
		key_register_cb(KEY_DEC, NULL, dir_item_close, dir_item_close, 1000, 50);
		key_register_cb(KEY_INC, NULL, dir_item_close, dir_item_close, 1000, 50);
	}
	else if(dir_item_num == 1)
	{
		key_register_cb(KEY_SEL, NULL, dir_item_select, dir_item_close, 1000, 50); //ֻ��ע��1������(��������)
		key_unregister_cb(KEY_DEC);
		key_unregister_cb(KEY_INC);
	}
	else
	{
		key_register_cb(KEY_SEL, NULL, dir_item_select, dir_item_close, 1000, 50);  //ע�����а���
		key_register_cb(KEY_DEC, dir_item_slide_up, NULL, dir_item_slide_up, 1000, 50);
		key_register_cb(KEY_INC, dir_item_slide_down, NULL, dir_item_slide_down, 1000, 50);
	}
}

/**
 ** @brief ���� button(��ť) ��Ķ�������--��LCD����ʾdirĿ¼�µ������ļ�
 ** @param dir  --Ŀ¼�ַ���(����/music /picture)     
 **        title--dir�б����
 ** @reval 0  --�ɹ�
 **        ��0--ʧ��
 **/
int dir_list_form_load(char *dir, char *title)
{
	if(lcd_list.item == NULL)
	{
		lcd_list.item = mem_malloc(sizeof(*lcd_list.item) * lcd_list.max_item_num);
		if(lcd_list.item == NULL) {
			return -1;
		}
	}
	if(title != NULL)
	{	lcd_list.title = title;	}

	if(dir_obj == NULL)
	{
		dir_obj = mem_malloc(sizeof(*dir_obj));
		if(dir_obj == NULL) {
			mem_free(lcd_list.item);
			lcd_list.item = NULL;
			return -2;
		}
		dir_obj->dir_list = NULL;
	}
	if(dir != NULL)
	{	u_strncpy(dir_obj->dir_name, dir, sizeof(dir_obj->dir_name));	}

	if(dir_obj->dir_list == NULL)
	{	dir_obj->dir_list = dir_list_create(dir_obj->dir_name);	}
	if(dir_obj->dir_list == NULL)
	{
		mem_free(dir_obj);
		dir_obj = NULL;
		mem_free(lcd_list.item);
		lcd_list.item = NULL;		
		return -3;
	}
	dir_obj->item_cnt = dir_get_obj_num(dir_obj->dir_list);
	dir_obj->dir_type = DIRECT_DIR;
	
	lcd_dev.backcolor = lcd_list.style->item_back_color;
	lcd_clear(lcd_dev.backcolor);
	lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx, dir_item_idx, 1);
	key_func_setting(dir_obj->item_cnt);

	return 0;
}

//dir����dirĿ¼���ļ��б���ʾ��LCD
static int dir_list_form_load_sub(void)
{
	if(dir_obj->dir_list == NULL)
	{
		dir_obj->dir_list = dir_list_create(dir_obj->dir_name);
		if(dir_obj->dir_list == NULL)
			return -1;
	}

	dir_obj->item_cnt = dir_get_obj_num(dir_obj->dir_list);
	dir_obj->dir_type = SUB_DIR;

	lcd_dev.backcolor = lcd_list.style->item_back_color;
	lcd_clear(lcd_dev.backcolor);
	lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx_sub, dir_item_idx_sub, 1);	
	key_func_setting(dir_obj->item_cnt);

	return 0;	
}

/**
 ** @brief ���¼���dir�б����, ����/��Ϸ��app�˳�����ô˺����������б����
 **/
void dir_list_form_reload(void)
{
	if(dir_obj->dir_type == DIRECT_DIR)
		dir_list_form_load(NULL, NULL);
	else
		dir_list_form_load_sub();
}

//====================================================================================
//dir�б�����°�������
//�ر�dir
static void dir_item_close(void)
{
	if(dir_obj->dir_list != NULL)
	{
		dir_list_destroy(dir_obj->dir_list);
		dir_obj->dir_list = NULL;
	}

	if(dir_obj->dir_type == DIRECT_DIR)
	{	//�Ӹ�Ŀ¼�˳�
		mem_free(dir_obj);
		dir_obj = NULL;
		mem_free(lcd_list.item);
		lcd_list.item = NULL;

		dir_item_idx = 0;
		lcd_sel_item_idx = 0;

		start_menu_load();    //��ת����ʼ�˵�
	}
	else
	{	//����Ŀ¼�˳�
		dir_item_idx_sub = 0;
		lcd_sel_item_idx_sub = 0;

		dir_back(dir_obj->dir_name);    //Ŀ¼����

		dir_list_form_load(NULL, NULL);
	}
}

//��ѡ�е�item
static void dir_item_select(void)
{
	char fname[128];
	int ret;
	fil_obj_t *fil_obj_ptr;

	fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	if(fil_obj_ptr->ftype != DIRECTORY)
	{	//��Ӧ�ó���ѡ�е��ļ�
		if(dir_obj->dir_type == DIRECT_DIR) {
			//�ڸ�Ŀ¼��ѡ���ļ�
			dir_item_idx = dir_obj->item_idx;
			lcd_sel_item_idx = lcd_list.sel_item_idx;
		}
		else {
			//����Ŀ¼��ѡ���ļ�
			dir_item_idx_sub = dir_obj->item_idx;
			lcd_sel_item_idx_sub = lcd_list.sel_item_idx;			
		}

		dir_list_destroy(dir_obj->dir_list);  //�����ļ�����, �ͷ��ڴ�, �������û���㹻���ڴ�������Ӧ�ó���
		dir_obj->dir_list = NULL;

		key_unregister_cb(KEY_SEL);           //Ӧ�ó��������ע�ᰴ���Ĺ���
		key_unregister_cb(KEY_DEC);
		key_unregister_cb(KEY_INC);

		u_snprintf(fname, sizeof(fname), "%s/%s", dir_obj->dir_name, fil_obj_ptr->fname);
		ret = run_file_with_app(fname, fil_obj_ptr->ftype);  //��Ӧ�ó�����ļ�
		if(ret != 0) {
			lcd_dev.backcolor = lcd_list.style->item_back_color;
			lcd_clear(lcd_dev.backcolor);
			u_snprintf(fname, sizeof(fname), "app execute error, ret=%d", ret);
			lcd_show_string(0, 0, lcd_dev.xres, 16, 16, fname, 0, lcd_list.style->item_font_color);		

			key_register_cb(KEY_SEL, dir_list_form_reload, NULL, NULL, 0, 0);  //ע�����а���
			key_register_cb(KEY_DEC, dir_list_form_reload, NULL, NULL, 0, 0);
			key_register_cb(KEY_INC, dir_list_form_reload, NULL, NULL, 0, 0);
		}
	}
	else
	{	//��ѡ�е���Ŀ¼
		dir_item_idx = dir_obj->item_idx;
		lcd_sel_item_idx = lcd_list.sel_item_idx;

		dir_increase(dir_obj->dir_name, fil_obj_ptr->fname);    //Ŀ¼����

		dir_list_destroy(dir_obj->dir_list);   //���ٸ�Ŀ¼list
		dir_obj->dir_list = NULL;

		ret = dir_list_form_load_sub();
		if(ret != 0)
		{
			my_printf("Open child directory fail, ret=%d\n\r", ret);
			dir_list_form_load(NULL, NULL);
		}
	}
}

//�ϻ����»�
static void dir_item_slide_up(void)
{
	lcd_list_slide_up(&lcd_list, dir_obj, 1);
}

static void dir_item_slide_down(void)
{
	lcd_list_slide_down(&lcd_list, dir_obj, 1);
}

//====================================================================================
/**
 ** @brief ����/ͼƬ��app������, ����mode�Զ�����list item
 ** @param mode  0--���е�ǰitem
 **              1--�»�һ��
 **             -1--�ϻ�һ��
 **/
int dir_list_item_auto_run(int mode, char **fname_out)
{
	if(dir_obj==NULL || lcd_list.item==NULL)
		return -1;

	if(dir_obj->dir_list == NULL)
	{
		dir_obj->dir_list = dir_list_create(dir_obj->dir_name);
		if(dir_obj->dir_list == NULL)
			return -2;
	}
	dir_obj->item_cnt = dir_get_obj_num(dir_obj->dir_list);

	if(dir_obj->dir_type == DIRECT_DIR)
	{	//�ڸ�Ŀ¼
		lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx, dir_item_idx, 0);
	}
	else
	{	//����Ŀ¼
		lcd_list_load(&lcd_list, dir_obj, lcd_sel_item_idx_sub, dir_item_idx_sub, 0);		
	}	

	fil_obj_t *fil_obj_ptr;
	if(mode > 0)
		lcd_list_slide_down(&lcd_list, dir_obj, 0);
	else if(mode < 0)
		lcd_list_slide_up(&lcd_list, dir_obj, 0);
	else
		asm("nop");
	fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	if(fil_obj_ptr->ftype == DIRECTORY)
	{	//Ŀ¼���⴦��, ���ָ�ԭ��
		if(mode > 0)
			lcd_list_slide_up(&lcd_list, dir_obj, 0);
		else if(mode < 0)
			lcd_list_slide_down(&lcd_list, dir_obj, 0);
		else
			asm("nop");
		fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	}

	if(dir_obj->dir_type == DIRECT_DIR)
	{	//�ڸ�Ŀ¼
		dir_item_idx = dir_obj->item_idx;
		lcd_sel_item_idx = lcd_list.sel_item_idx;
	}
	else
	{	//����Ŀ¼
		dir_item_idx_sub = dir_obj->item_idx;
		lcd_sel_item_idx_sub = lcd_list.sel_item_idx;			
	}

	char fname[128];	
	int ret;

	dir_list_destroy(dir_obj->dir_list);  //�����ļ�����, �ͷ��ڴ�, �������û���㹻���ڴ�������Ӧ�ó���
	dir_obj->dir_list = NULL;

	if(fname_out != NULL)
	{	*fname_out = fil_obj_ptr->fname;	}
	u_snprintf(fname, sizeof(fname), "%s/%s", dir_obj->dir_name, fil_obj_ptr->fname);
	ret = run_file_with_app(fname, fil_obj_ptr->ftype);  //��Ӧ�ó�����ļ�
	if(ret != 0)
	{
		lcd_dev.backcolor = lcd_list.style->item_back_color;
		lcd_clear(lcd_dev.backcolor);
		u_snprintf(fname, sizeof(fname), "app auto execute error, ret=%d", ret);
		lcd_show_string(0, 0, lcd_dev.xres, 16, 16, fname, 0, lcd_list.style->item_font_color);
		return 1;
	}

	return 0;	
}

/**
 ** @brief ��ȡ��ǰѡ�е�item
 ** @param fname_out  item�ļ������
 ** @reval 0   �ɹ�            
 **        ��0 ʧ��    
 **/
int dir_list_get_cur_item(char **fname_out)
{
	if(dir_obj==NULL || lcd_list.item==NULL)
		return -1;

	fil_obj_t *fil_obj_ptr = lcd_list_get_sel_item(&lcd_list);
	if(fname_out != NULL)
	{	*fname_out = fil_obj_ptr->fname;	}

	return 0;
}



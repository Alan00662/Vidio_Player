#include "fs_app_v2.h"
#include "ff.h"

#include "libc.h"
#include "ctype.h"
#include "share.h"
#include "my_printf.h"
#include "my_malloc.h"


//fs_app     ����
//fs_app_v2  20240512, fname_str_compare()�������ַ���ת��Ϊ��ֵǰ, ����ǰ
//           ����ַ�0�����˴���. ������0924�����ַ�������Ϊ��8����, �������ַ�
//           ��������7������, �ᵼ��ת�����Ϊ0. 

typedef struct ftype_tab_s
{
	int const ftype;
	char *const suffix[2];
}ftype_tab_t;

const ftype_tab_t ftype_tab[] =
{
	{MP3_FILE, {"mp3", "MP3"}},
	{WAV_FILE, {"wav", "WAV"}},
	{AAC_FILE, {"aac", "AAC"}},
	{AVI_FILE, {"avi", "AVI"}},
	{MP4_FILE, {"mp4", "MP4"}},
	{NES_FILE, {"nes", "NES"}},
	{JPG_FILE, {"jpg", "JPG"}},
	{BMP_FILE, {"bmp", "BMP"}},
};

/** @brief �����ļ�����ȡ�ļ�����
 */
int  get_file_obj_type(const char *fname)
{
	const char *suffix = NULL;
	
	int i = u_strlen(fname);
	
	while(i >= 0)
	{
		if(fname[i] == '.')
		{
			suffix = &fname[i+1];
			break;
		}
		i--;
	}
	
	if(suffix != NULL)
	{
		for(i=0; i<ARRAY_SIZE(ftype_tab); i++)
		{
			if(!u_strcmp(suffix, ftype_tab[i].suffix[0]) || !u_strcmp(suffix, ftype_tab[i].suffix[1]))
				return ftype_tab[i].ftype;
		}
	}

	return UNKNOW_FILE;
}

//�ַ���0947 0855�ڽ���strtoulת��ʱǰ���0������ֵ���8����,
//ת��ʱ������7����ַ�ֱ���˳�ת��, ���2���ַ���ת���Ľ������0.
//Ϊ�˱����������, ��ת��ǰ���ַ���ǰ���0�滻Ϊ�ո�.
static inline void str_prefix_zero_deal(char *s)
{
	while(*s == '0')
	{
		*s = ' ';
		s++;
	}
}

/** @brief ����Windows�ļ��������Ա�2���ļ����ַ���
 ** @param fname1-�ļ����ַ���1  fname2-�ļ����ַ���2
 ** @reval >0 fname1��ǰ fname2�ں�
 **        <0 fname2��ǰ fname1�ں�
 **        =0 �����ַ���һ��
 ** @tip   ����ļ����ַ�������.mp3 .avi .nes .txt�Ⱥ�׺, �Ա�ʱ�������Щ��׺
 **/
int fname_str_compare(const char *fname1, const char *fname2)
{
	int i;
	char ch1, ch2;
	char digit_buff1[32], digit_buff2[32];
	int  digit_idx;
	unsigned long digit1, digit2;


	int len1, suffix_len1;
	int len2, suffix_len2;
	int min_len;

	len1 = u_strlen(fname1);
	suffix_len1 = 0;
	i = len1 - 1;
	while(i >= 0)
	{
		suffix_len1++;
		if(fname1[i] == '.')
			break;
		i--;
	}
//	my_printf("len1=%u, suffix_len1=%u \n\r", len1, suffix_len1);
	if(suffix_len1 > 8)       //�ļ���׺���ĳ��Ȳ�����8���ַ�
	{	suffix_len1 = 8; }

	if(len1 > suffix_len1)    //�ļ����к�׺ʱȥ����׺���ĳ���
	{	len1 -= suffix_len1;	}
	
	len2 = u_strlen(fname2);	
	suffix_len2 = 0;
	i = len2 - 1;
	while(i >= 0)
	{
		suffix_len2++;
		if(fname2[i] == '.')
			break;
		i--;
	}
//	my_printf("len2=%u, suffix_len2=%u \n\r", len2, suffix_len2);
	if(suffix_len2 > 8)       //�ļ���׺���ĳ��Ȳ�����8���ַ�
	{	suffix_len2 = 8; }

	if(len2 > suffix_len2)    //�ļ����к�׺ʱȥ����׺���ĳ���
	{	len2 -= suffix_len2;	}
	
	
	min_len = len1 > len2 ? len2 : len1;
	digit_idx = 0;
	for(i=0; i<min_len; i++)
	{
		ch1 = fname1[i];
		ch2 = fname2[i];
		if(isdigit(ch1) && isdigit(ch2))
		{
			digit_buff1[digit_idx] = ch1;
			digit_buff2[digit_idx] = ch2;
			digit_idx++;
			if(digit_idx >= sizeof(digit_buff1)-1) {
				return 0;
			}
			continue;
		}
	
		if(digit_idx > 0)
		{
			if(isdigit(ch1)) {	         /* ch1�������ַ�, fname1�ں� */
				return -1;
			} else if(isdigit(ch2)) {    /* ch2�������ַ�, fname2�ں� */
				return 1;
			} else {                     /* ch1 ch2������ʮ�������� */
				digit_buff1[digit_idx] = '\0';
				digit_buff2[digit_idx] = '\0';
				digit_idx = 0;
				str_prefix_zero_deal(digit_buff1);
				str_prefix_zero_deal(digit_buff2);
				digit1 = u_strtoul(digit_buff1, NULL, 0);
				digit2 = u_strtoul(digit_buff2, NULL, 0);
				if(digit1 != digit2)
				{
					return (digit2 > digit1 ? (digit2 - digit1) : -(int)(digit1 - digit2));
				}
				else
				{	//����ֱ��ת�Ƶ����� if(ch1 != ch2)
				}
			}
		}
	
		if(ch1 != ch2)
		{
			if(ch1 == '_') {
				if(ch2!='(' && ch2!='[')
					return 1;
				else
					return -1;
			}
			else if(ch2 == '_') {
				if(ch1!='(' && ch1!='[')
					return -1;
				else
					return 1;
			}
			else if(isdigit(ch1)) {
				if(ch2=='(' || ch2=='[')
					return -1;
				else
					return 1;
			}
			else if(isdigit(ch2)) {
				if(ch1=='(' || ch1=='[')
					return 1;
				else
					return -1;
			}
			else if(isalnum(ch1) && isalnum(ch2))
				return ch2-ch1;
			else if(!isalnum(ch1) && !isalnum(ch2))
				return ch2-ch1;
			else if(isalnum(ch1))   /* ch1�������ַ�����ĸ, fname1��ǰ */
				return 1;
			else 
				return -1;
		}
	}

	//��һ����Ϊ�˷�ֹ�������: ��100 ��20 ��20ҳ ���������ֽ�β���ַ���
	if(digit_idx > 0)
	{		
		if(len1 == len2)
		{	//������ȵ��ַ���, �����ַ��� ��10  ��20
			digit_buff1[digit_idx] = '\0';
			digit_buff2[digit_idx] = '\0';
			digit_idx = 0;
			str_prefix_zero_deal(digit_buff1);
			str_prefix_zero_deal(digit_buff2);
			digit1 = u_strtoul(digit_buff1, NULL, 0);
			digit2 = u_strtoul(digit_buff2, NULL, 0);
			return (digit2 >= digit1 ? (digit2 - digit1) : -(digit1 - digit2));			
		}
		else
		{
			//���Ȳ���ȵ��ַ���, �����ַ��� ��50  ��100  ��50ҳ
			if(len1 > len2) {
				ch1 = fname1[i];
				if(isdigit(ch1)) {    /* ch1�������ַ�, fname1�ں� */
					return -1;
				}
			} else {
				ch2 = fname2[i];
				if(isdigit(ch2)) {    /* ch2�������ַ�, fname2�ں� */
					return 1;
				}
			}

			digit_buff1[digit_idx] = '\0';
			digit_buff2[digit_idx] = '\0';
			digit_idx = 0;
			str_prefix_zero_deal(digit_buff1);
			str_prefix_zero_deal(digit_buff2);
			digit1 = u_strtoul(digit_buff1, NULL, 0);
			digit2 = u_strtoul(digit_buff2, NULL, 0);
			if(digit1 != digit2)
			{
				return (digit2 > digit1 ? (digit2 - digit1) : -(int)(digit1 - digit2));
			}
			else
			{	//����ֱ��ת�Ƶ����� (len2-len1)
			}
		}
	}
	
	//��ǰ�涼���, ��ֱ�ӱȽ��ַ����ĳ���, �����ź���, �̵���ǰ��
	return (len2-len1);
}


//===================================================================================
//����Ĳ������
static int __list_insert(dir_list_t *dir_list, fil_obj_t *fil_obj, int order)
{
	/* �������  */
	dir_list_t *add_node = mem_malloc(sizeof(dir_list_t));
	if(add_node == NULL)
		return -1;
	
//	add_node->fil_obj = *fil_obj;    //�ṹ�����ֱ�Ӹ�ֵ, �����������ϵͳC���е�memcpy����ɽṹ���������帳ֵ
	u_memcpy(&add_node->fil_obj, fil_obj, sizeof(*fil_obj));
	add_node->next = NULL;
	
	/* Ѱ�ҽ��Ĳ���λ�� */
	dir_list_t *cur_node, *front_node;
	front_node = dir_list;
	cur_node = dir_list->next;
	
//	my_printf("\n\r");                 //debug print
//	my_printf("The front Node:\n\r");
	int ret;
	while(cur_node != NULL)
	{
//		my_printf("%s\n\r", cur_node->fil_obj.fname);
		ret = fname_str_compare(cur_node->fil_obj.fname, add_node->fil_obj.fname);

		if(order == ORDER_SEQ) {
			if(ret <= 0)
				break;
		}
		else {
			if(ret > 0)
				break;
		}
		front_node = cur_node;
		cur_node = cur_node->next;
	}
//	my_printf("Add node: %s\n\r", add_node->fil_obj.fname);
	
	/* �ѽ���������(��add_node���뵽front_node���� cur_nodeǰ��) */
	front_node->next = add_node;
	add_node->next = cur_node;
	
	return 0;
}

//��·���������������ǰ��
static void __list_alter(dir_list_t *dir_list)
{
	dir_list_t *dir_head, *dir_node;
	dir_list_t *cur_node, *front_node;
	
	dir_head = mem_malloc(sizeof(dir_list_t));
	if(dir_head == NULL)
		return;
	dir_head->next = NULL;
	u_memset(&dir_head->fil_obj, 0x00, sizeof(dir_head->fil_obj));
	dir_node = dir_head;
	
	front_node = dir_list;
	cur_node = dir_list->next;
	while(cur_node != NULL)
	{
		if(cur_node->fil_obj.ftype == DIRECTORY)
		{
			//ȡ��dir���
			dir_node->next = cur_node;
			dir_node = dir_node->next;
		
			front_node->next = cur_node->next;
			cur_node = cur_node->next;
		}
		else
		{
			front_node = cur_node;
			cur_node = cur_node->next;
		}
	}
	
	//��dir_node����dir_list�������ǰ��
	if(dir_head->next != NULL)
	{
		dir_node->next = dir_list->next;
		dir_list->next = dir_head->next;
	}
	mem_free(dir_head);
}

/** ����pathĿ¼�µ�list.txt�ļ�����������
 */
dir_list_t *dir_list_create(const char *path)
{
	dir_list_t *dir_head = NULL;
	
	/***** ��������ͷ *****/
	dir_head = mem_malloc(sizeof(dir_list_t));  //����ͷ���
	if(dir_head == NULL)
		return NULL;
	dir_head->next = NULL;
	u_memset(&dir_head->fil_obj, 0x00, sizeof(dir_head->fil_obj));
	
	/***** ɨ��·������������ *****/
	int i, cnt, ret;
	char path_buff[128];
	char line_buff[128];
	FIL fil;
	dir_list_t *cur_node, *add_node;
	
	u_snprintf(path_buff, sizeof(path_buff), "%s/%s", path, "list.txt");
	ret = f_open(&fil, path_buff, FA_READ);
	if(ret != FR_OK)    //˵������һ�����ļ���
	{
		my_printf("[%s] NULL\n\r", __FUNCTION__);
		f_close(&fil);
		return dir_head;
	}
	
	cur_node = dir_head;
	while(NULL != f_gets(line_buff, sizeof(line_buff), &fil))
	{
		add_node = mem_malloc(sizeof(*add_node));
		if(add_node == NULL)
			break;
//		u_sscanf(line_buff, "%s %d %d", add_node->fil_obj.fname, &add_node->fil_obj.ftype, &add_node->fil_obj.size);  //bug
		cnt = 0;
		i = u_strlen(line_buff)-1;
		while(i >= 0)
		{
			if(line_buff[i] == ' ')
			{
				cnt++;
				if(cnt == 1) {
					add_node->fil_obj.size = u_atoi(&line_buff[i+1]);
				}
				else
				{
					add_node->fil_obj.ftype = u_atoi(&line_buff[i+1]);
					break;
				}
			}
			i--;
		}
		line_buff[i] = '\0';   //�ַ�����β
		u_memcpy(add_node->fil_obj.fname, line_buff, i+1);
	
		add_node->next = NULL;

		//�ѽ���������
		cur_node->next = add_node;
		cur_node = cur_node->next;
	}
	
	f_close(&fil);
	return dir_head;
}

void dir_list_destroy(dir_list_t *dir_list)
{
	dir_list_t *cur_node, *behind_node;
	
	cur_node = dir_list;      //��ͷ��㿪ʼ�ͷ��ڴ�
	while(cur_node != NULL)
	{
		behind_node = cur_node->next;
		mem_free(cur_node);
		cur_node = behind_node;
	}
}

//����ı�������
int  dir_traverse_obj(dir_list_t *dir_list)
{
	int obj_cnt = 0;
	
	if(dir_list == NULL)
		return obj_cnt;

	dir_list_t *cur_node = dir_list->next;
	while(cur_node != NULL)
	{
		my_printf("%02X\t %d\t %s\n\r", cur_node->fil_obj.ftype, cur_node->fil_obj.size, cur_node->fil_obj.fname);
		cur_node = cur_node->next;
		obj_cnt++;
	}

	my_printf("obj_cnt = %d\n\r", obj_cnt);
	return obj_cnt;	
}

int  dir_get_obj_num(dir_list_t *dir_list)
{
	int obj_cnt = 0;
	
	if(dir_list == NULL)
		return obj_cnt;

	dir_list_t *cur_node = dir_list->next;
	while(cur_node != NULL)
	{
		cur_node = cur_node->next;
		obj_cnt++;
	}

	return obj_cnt;
}


int  dir_read_obj_info(dir_list_t *dir_list, int skip, fil_obj_t *out, int num)
{
	if(dir_list == NULL)
		return 0;
	
	int i = 0;
	dir_list_t *cur_node = dir_list->next;
	while(cur_node != NULL)
	{
		if(i >= skip)
			break;

		cur_node = cur_node->next;
		i++;
	}
	
	i = 0;
	while(cur_node != NULL)
	{
		if(i >= num)
			break;

//		out[i] = cur_node->fil_obj;       //�����������ϵͳC���е�memcpy����ɽṹ���������帳ֵ
		u_memcpy(&out[i], &cur_node->fil_obj, sizeof(cur_node->fil_obj));
		cur_node = cur_node->next;
		i++;
	}
	
	return i;
}

/** ���pathĿ¼�µ�list.txt�ļ�������
 */
void dir_list_output(const char *path, void (*out_func)(void *, int))
{
	if(out_func == NULL)
		return;

	int i, n, cnt, ret;
	char line_buff[128];
	char out_buff[128];
	char *tip;
	FIL fil;

	u_snprintf(line_buff, sizeof(line_buff), "%s/%s", path, "list.txt");
	ret = f_open(&fil, line_buff, FA_READ);
	if(ret != FR_OK)    //˵������һ�����ļ���
	{
		tip = "Empty folder!\n\r";
		out_func(tip, u_strlen(tip));
		f_close(&fil);
		return;
	}

	n = 0;
	tip = "\n\r";
	out_func(tip, u_strlen(tip));
	while(NULL != f_gets(line_buff, sizeof(line_buff), &fil))  //f_gets()Ч��̫��, �˴��д��Ż�
	{
//		u_sscanf(line_buff, "%s %d %d", add_node->fil_obj.fname, &add_node->fil_obj.ftype, &add_node->fil_obj.size);  //bug
		cnt = 0;
		i = u_strlen(line_buff)-1;
		while(i >= 0)
		{
			if(line_buff[i] == ' ')
			{
				cnt++;
				if(cnt == 2)
					break;
			}
			i--;
		}
		line_buff[i] = '\0';   //�ַ�����β
		n++;
		ret = u_snprintf(out_buff, sizeof(out_buff), "%d %s\n\r", n, line_buff);
		out_func(out_buff, ret);
	}

	f_close(&fil);	
}

/** ���pathĿ¼�µ�n��obj�Ƿ�Ϊ�ļ�(�����ļ���),
 ** �����򷵻�n, �������򷵻ؼ�⵽�ĵ�һ��Ϊ�ļ��ļ���m(mΪ0��ʾû���ļ�)
 */
int  dir_test_expect_obj(const char *path, int n)
{
	int i, cnt, ret;
	int m, obj_no;
	char path_buff[128];
	char line_buff[128];
	fil_obj_t fil_obj;
	FIL fil;

	u_snprintf(path_buff, sizeof(path_buff), "%s/%s", path, "list.txt");
	ret = f_open(&fil, path_buff, FA_READ);
	if(ret != FR_OK)    //˵������һ�����ļ���
	{
		return 0;
	}

	m = 0;
	obj_no = 0;
	while(NULL != f_gets(line_buff, sizeof(line_buff), &fil))  //f_gets()Ч��̫��, �˴��д��Ż�
	{
//		u_sscanf(line_buff, "%s %d %d", add_node->fil_obj.fname, &add_node->fil_obj.ftype, &add_node->fil_obj.size);  //bug
		cnt = 0;
		i = u_strlen(line_buff)-1;
		while(i >= 0)
		{
			if(line_buff[i] == ' ')
			{
				cnt++;
				if(cnt == 2) {
					fil_obj.ftype = u_atoi(&line_buff[i+1]);
					break;
				}
			}
			i--;
		}
		obj_no++;

		if(fil_obj.ftype!=DIRECTORY && m==0)
		{
			m = obj_no;  //��¼�׸���ΪDIRECTORY��obj���
		}
		if(obj_no == n)
		{
			if(fil_obj.ftype != DIRECTORY) {
				m = n;
				break;
			}
		}
	}

	f_close(&fil);
	return m;
}

/** @breif ���һ��Ŀ¼�µ��ļ��б��Ƿ���Ҫ����,
 **        �����Ҫ���»�˺������Զ���ɸ���
 ** @param mode 0-������Ŀ¼
 **             1-����Ŀ¼Ҳ����һ���ļ�
 **        order ORDER_SEQ-����˳��
 **              ORDER_REV-����
 */
int  dir_check_list(const char *path, int mode, int order)
{
	int ret;
	int child_dir_cnt = 0;
	dir_list_t *dir_head = NULL;
	
	/***** ��������ͷ *****/
	dir_head = mem_malloc(sizeof(dir_list_t));  //����ͷ���
	if(dir_head == NULL)
		return -1;
	dir_head->next = NULL;
	u_memset(&dir_head->fil_obj, 0x00, sizeof(dir_head->fil_obj));
	
	/***** ɨ��·������������ *****/
	DIR dir;
	FILINFO fno;
	fil_obj_t fil_obj;
	ret = f_opendir(&dir, path);  /* Open the directory */
	if(ret != FR_OK)
	{
		mem_free(dir_head);
		return -2;
	}
	
	for( ; ; )
	{
		ret = f_readdir(&dir, &fno);   /* Read a directory item */
		if(ret!=FR_OK || fno.fname[0]==0) /* Break on error or end of dir */
			break;
		
		if(fno.fattrib & AM_DIR)
		{
			child_dir_cnt++;
			if(mode == 0)      //skip child directory
				continue;
		}
		if(fno.fattrib & AM_HID)
		{
			continue;
		}
	
		u_strncpy(fil_obj.fname, fno.fname, sizeof(fil_obj.fname));
		fil_obj.fname[sizeof(fil_obj.fname)-1] = '\0';
		fil_obj.size = fno.fsize;
		if(fno.fattrib & AM_DIR)
			fil_obj.ftype = DIRECTORY;
		else
			fil_obj.ftype = get_file_obj_type(fno.fname);
		
		ret = __list_insert(dir_head, &fil_obj, order);
		if(ret != 0)
			break;
	}

	f_closedir(&dir);
	
	/******* ����������·������λ�� *******/
	if(mode!=0 && child_dir_cnt!=0)
	{
		__list_alter(dir_head);
	}
	
	//��ȡdirĿ¼��item������
	int  item_cnt;
	int  update_list_flag;
	char path_buff[128];
	char line_buff[128];
	char temp_buff[128];
	
	FIL  fil;
	dir_list_t *cur_node;

	item_cnt = dir_get_obj_num(dir_head);
	u_snprintf(path_buff, sizeof(path_buff), "%s/%s", path, "list.txt");
	my_printf("[%s] item_cnt = %d\n\r", __FUNCTION__, item_cnt);
	if(item_cnt == 0)	//Ŀ¼�����κ��ļ�
	{
		ret = f_stat(path_buff, &fno);
		if(ret == 0)   //��Ŀ¼Ϊ��ʱ, ɾ��list.txt�ļ�
		{
			f_unlink(path_buff);
		}
	}
	else				//Ŀ¼�����ļ�, ���list.txt�Ƿ���Ҫ����
	{
		ret = f_open(&fil, path_buff, FA_READ);
		if(ret == FR_OK)
		{
			//�򿪳ɹ�ʱ, ����Ƿ���Ҫ����
			update_list_flag = 0;
			cur_node = dir_head->next;

			while(NULL!=f_gets(line_buff, sizeof(line_buff), &fil) && NULL!=cur_node)
			{
				u_snprintf(temp_buff, sizeof(temp_buff), "%s %d %d\n", cur_node->fil_obj.fname, cur_node->fil_obj.ftype, cur_node->fil_obj.size);
				if(strcmp(line_buff, temp_buff))
				{
					update_list_flag = 1;
					break;
				}
				cur_node = cur_node->next;
			}
			
			//���list.txt�е�line������dir_head�еĽڵ����Ƿ�һ��
			if(update_list_flag == 0)
			{
				if(fil.fptr!=f_size(&fil) || cur_node!=NULL)
				{
					update_list_flag = 1;
				}
				my_printf("[%s] fil.fptr=%u f_size=%u\n\r", __FUNCTION__, (uint32_t)fil.fptr, (uint32_t)f_size(&fil));
				my_printf("[%s] cur_node=0x%08X\n\r", __FUNCTION__, (uint32_t)cur_node);
			}
			
			f_close(&fil);    //�����˹ر��ļ�
			
			if(update_list_flag == 0)
			{
				my_printf("[%s] No need to update list\n\r", __FUNCTION__);
				goto func_exit;
			}
			else
			{
				my_printf("[%s] Need to update list\n\r", __FUNCTION__);
			}
		}

		//�����ļ�
		my_printf("[%s] Updating list...\n\r", __FUNCTION__);
		ret = f_open(&fil, path_buff, FA_CREATE_ALWAYS|FA_WRITE);
		if(ret != FR_OK)
		{
			my_printf("[%s] Create fail error, ret=%d\n\r", ret);
			goto func_exit;
		}

		cur_node = dir_head->next;
		while(cur_node != NULL)
		{
			f_printf(&fil, "%s %d %d\n", cur_node->fil_obj.fname, cur_node->fil_obj.ftype, cur_node->fil_obj.size);
			cur_node = cur_node->next;
		}
		f_close(&fil);
		f_chmod(path_buff, AM_HID, AM_HID);    //��list.txt����Ϊ�����ļ�
	}
	
func_exit:
	dir_list_destroy(dir_head);
	return 0;
}

/** ��pathĿ¼�µ���Ŀ¼����dir_check_list
 */
int  dir_check_list_child(const char *path, int order)
{
	int ret;
	char path_buff[128];
	DIR dir;
	FILINFO fno;
	
	ret = f_opendir(&dir, path);  /* Open the directory */
	if(ret != FR_OK)
	{
		return -1;
	}
	
	for( ; ; )
	{
		ret = f_readdir(&dir, &fno);   /* Read a directory item */
		if(ret!=FR_OK || fno.fname[0]==0) /* Break on error or end of dir */
			break;
		
		if(fno.fattrib & AM_DIR)
		{
			u_snprintf(path_buff, sizeof(path_buff), "%s/%s", path, fno.fname);
			ret = dir_check_list(path_buff, 0, order);
			if(ret != 0) {
				my_printf("[%s] check list fail! ret=%d!!\n\r", __FUNCTION__, ret);
				break;
			}
		}
	}

	f_closedir(&dir);	
	
	return ret;
}


//==================================================================
dir_obj_t *dir_open(const char *dir_path)
{
	if(dir_path == NULL)
		return NULL;

	dir_obj_t *pdir = mem_malloc(sizeof(dir_obj_t));
	if(pdir == NULL)
		return NULL;

	pdir->dir_list = dir_list_create(dir_path);
	if(pdir->dir_list == NULL)
	{
		mem_free(pdir);
		return NULL;
	}

	pdir->item_idx = 0;
	pdir->item_cnt = dir_get_obj_num(pdir->dir_list);
	u_strncpy(pdir->dir_name, dir_path, sizeof(pdir->dir_name));

	return pdir;
}

void dir_close(dir_obj_t *pdir)
{
	if(pdir != NULL)
	{
		dir_list_destroy(pdir->dir_list);
		mem_free(pdir);
	}
}

//Ŀ¼����(/music/favorite --> /music)
void dir_back(char *dir_name)
{
	int i;

	i = u_strlen(dir_name);
	i--;
	while(i > 0)
	{
		if(dir_name[i]=='/' || dir_name[i]=='\\')
			break;
		else
			dir_name[i] = 0;
		i--;
	}

	if(i <= 0)    //����Ŀ¼ʱ���ٻ���(����/music��û���ٻ���)
		return;

	while(dir_name[i]=='/' || dir_name[i]=='\\')
	{
		dir_name[i] = 0;
		i--;
		if(i == 0)
			break;
	}
}

//Ŀ¼����
void dir_increase(char *dir_name, char *subdir)
{
	//����ʹ��strcat()����
	int i;

	i = u_strlen(dir_name);
	dir_name[i++] = '/';    //��������ӷָ���
	while(*subdir != '\0')
	{
		dir_name[i] = *subdir++;
		i++;
	}
	dir_name[i] = '\0';
}


int dir_direct_judge(char *dir_name)
{
	int cnt = 0;
	char *p = dir_name;

	while(*p != '\0')
	{
		if(*p=='/' || *p=='\\')
			cnt++;
		p++;
	}

	if(cnt == 1)
		return DIRECT_DIR;
	else
		return SUB_DIR;
}



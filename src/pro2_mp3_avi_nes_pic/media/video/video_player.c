#include "video_player.h"
#include "video_support.h"

#include "audio_card.h"
#include "lcd_app_v2.h"
#include "my_printf.h"
#include "my_malloc.h"
#include "user_task.h"
#include "libc.h"

#include "media_timer_v2.h"
#include "cpu_related_v3.h"
#include "dir_list.h"

video_player_obj_t  *video_player_obj = NULL;

//��Ƶ������Task Control Block
static task_control_block_t tcb_video_player;
static void video_timer_start(void);


int  video_player_start(char *fname)
{
	int ret;
	
	if(video_player_obj != NULL)   //�ȹرղ�����
	{
		video_player_stop();
	}
	
	video_player_obj = mem_malloc(sizeof(video_player_obj_t));
	if(video_player_obj == NULL)
	{
		my_printf("maloc video_player_obj fail\n\r");
		return -1;
	}

//	ret = lcd_dma_init();
//	if(ret != 0)
//	{
//		mem_free(video_player_obj);
//		my_printf("lcd_dma_init fail, ret=%d\n\r", ret);
//		return -2;
//	}

	ret = video_open_file(fname);
	if(ret != 0)
	{
		mem_free(video_player_obj);
//		lcd_dma_deinit();
		my_printf("video_open_file() fail, ret=%d\n\r", ret);
		return -3;
	}

	video_player_obj->state = VIDEO_PLAYER_PLAYING;
	
	//������ĻˢΪ��ɫ
	lcd_clear(LCD_BLACK);

	//======================
	tcb_video_player.name = "video_player";
	tcb_video_player.state = TS_STOPPED;
	tcb_video_player.is_always_alive = 0;
	tcb_video_player.action = video_player_cycle;
	tcb_video_player.mark = 0;
	task_create(&tcb_video_player);

	task_wakeup(&tcb_video_player);

	video_timer_start();

	return 0;
}

static void video_feeding_sound_card(void);

int  vid_open_sys_audio(int sample_freq, int channels, int sample_bits, void *pkg_buff, int pkg_size)
{
	int ret;

	video_player_obj->pcm_size = pkg_size;

	//������
	ret = audio_card_open(sample_freq, channels, sample_bits, pkg_buff, pkg_size);
	if(ret != 0)
	{
		my_printf("[%s] dac_sound_start fail, ret=%d\n\r", __FUNCTION__, ret);
		return -1;
	}

	//ע��ص�����
	audio_card_register_feeder(video_feeding_sound_card);
	
	//�������
//	ret = audio_card_get_volume();
//	if(ret < 28)
//	{
//		audio_card_chg_volume(28);
//	}

	//��ʼ��kfifo
	ret = kfifo_init(&video_player_obj->audio_fifo, video_player_obj->audio_buff, AUDIO_BUFF_SIZE);
	if(ret != 0)
	{
		audio_card_close();
		return -2;
	}
	return 0;
}


int vid_close_sys_audio(void)
{
	audio_card_close();    //ֱ�ӹر���������
	return 0;
}

//��fifo��ȡ��pcm�����Ӹ�����
static void video_feeding_sound_card(void)
{
	uint32_t ret = kfifo_len(&video_player_obj->audio_fifo);
	
	if(ret == 0)
	{
		u_memset(video_player_obj->temp_buff, 0x00, video_player_obj->pcm_size);
		my_printf("L\n\r");
	}
	else
	{
		ret = kfifo_out(&video_player_obj->audio_fifo, video_player_obj->temp_buff, video_player_obj->pcm_size);
		if(ret == 0)
		{
			u_memset(video_player_obj->temp_buff, 0x00, video_player_obj->pcm_size);
			my_printf("audio_fifo out error!!!\n\r");
		}
	}

	audio_card_write(video_player_obj->temp_buff, video_player_obj->pcm_size);

	tcb_video_player.mark |= 0x01;
	task_wakeup(&tcb_video_player);	
}


//��pcm����д��fifo
void  vid_write_pcm_data(void *pcm_ptr, int size)
{
	int ret;
	
	ret = kfifo_avail(&video_player_obj->audio_fifo);
	if(ret > size)
	{
		if(video_player_obj->state != VIDEO_PLAYER_PLAYING)
			u_memset(pcm_ptr, 0x00, size);
		ret = kfifo_in(&video_player_obj->audio_fifo, pcm_ptr, size);
		if(ret != size)
		{
			my_printf("ret = %d\n\r", ret);
			my_printf("audio_fifo write error!\n\r");
		}
	}
	else
	{
		my_printf("O\n\r");
	}
}

/** @brief ���һ������
 ** @param line_no �����к�, ȡֵ[1, 240]
 **        pixel   ����ֵ(RGB565)
 **        pixel_num���صĸ���, 320
 **/
void vid_write_bitmap_line(int line_no, uint16_t *pixel, int pixel_num)
{
//	lcd_colorfill_area_16b(0, line_no-1, pixel_num-1,line_no-1, pixel);

	if(line_no == 1)
	{
		LCD_Set_Window(0, 0, lcd_dev.xres-1, lcd_dev.yres-1);
		LCD_WriteRAM_Prepare();		
	}

	lcd_dma_trans_line(pixel, pixel_num);

	if(line_no == 240)
	{
		lcd_dma_trans_release();      //�ȴ����һ��������
	}
}

static void video_timer_isr(void);

static void video_timer_start(void)
{
	//����һ��1s�Ķ�ʱ��
	//10KHz��100us, 100us*10000=1000ms=1s
	media_timer_start(10000, 10000, video_timer_isr);
}

static void video_timer_stop(void)
{
	media_timer_stop();	
}

//Timer��ʱ���� 1000ms
static void video_timer_isr(void)
{
	tcb_video_player.mark |= 0x02;
	task_wakeup(&tcb_video_player);
}


void video_player_cycle(void)
{
	int ret, task_mark;
	int decode_cnt = 0;

	if(video_player_obj == NULL)
		return;	

	ALLOC_CPU_SR();
	ENTER_CRITICAL();
	task_mark = tcb_video_player.mark;
	tcb_video_player.mark = 0;
	EXIT_CRITICAL();

	if(likely(task_mark & 0x01))
	{
decode_loop:
		if(video_player_obj->state == VIDEO_PLAYER_PLAYING)
		{		
			ret = video_decode_stream();
			if(ret == 0)
			{	/* ����ɹ� */
				if(kfifo_len(&video_player_obj->audio_fifo) < video_player_obj->pcm_size*6)   //����4������
				{
					decode_cnt++;
					if(decode_cnt > 8) {   //ǿ���˳�, ��ֹ����������
	//					my_printf("Giving up CPU\n\r");
						tcb_video_player.mark |= 0x01;
						task_wakeup(&tcb_video_player);
					} else {
						goto decode_loop;
					}
				}
			}
			else if(ret < 0)
			{	/* ����ʧ�� */
				my_printf("decode stream error, ret=%d\n\r", ret);
				vid_ctrl_panel_hide();
				video_player_stop();
			}
			else
			{	/* ������� */
				vid_ctrl_panel_hide();
				video_player_stop();
//				task_mark = 0;
			}
		}
		else
		{
			u_memset(video_player_obj->temp_buff, 0x00, video_player_obj->pcm_size);
			vid_write_pcm_data(video_player_obj->temp_buff, video_player_obj->pcm_size);
		}
	}

	if(unlikely(task_mark & 0x02))
	{
		video_1s_hook();
	}
}

void video_player_stop(void)
{
	video_timer_stop();
	task_delete(&tcb_video_player);    //ɾ��audio����

	if(video_player_obj != NULL)
	{
		video_close();
//		lcd_dma_deinit();
		mem_free(video_player_obj);
		video_player_obj = NULL;
	}
}


//====================================================================
//����������

/** @brief �����λ��Ƶ�ļ�
 ** @param pos-λ��[0-99]
 */
int video_player_seek(int pos)
{
	int ret = 0;
	if(video_player_obj != NULL)
	{
		ret = video_seek(pos);
	}
	return ret;
}

/** @brief ���ò�����������
 ** @param volume-���� [0,100]
 */
int video_player_set_volume(int volume)
{
	int ret = 0;
	if(video_player_obj != NULL)
	{
		ret = video_set_volume(volume);
	}
	return ret;
}

/**  @brief ��ͣ����
 */
int video_player_pause(void)
{
	int ret = 0;
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PLAYING)
	{
		ret = video_pause();
		if(ret == 0)
			video_player_obj->state = VIDEO_PLAYER_PAUSE;
	}
	return ret;
}

/** @brief �ָ�����
 */
int video_player_resume(void)
{
	int ret = 0;
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PAUSE)
	{
		ret = video_resume();
		if(ret == 0)
			video_player_obj->state = VIDEO_PLAYER_PLAYING;
	}
	return 0;
}

//===========================================================================
//��Ƶ����״̬�°�������
#include "key.h"

//video play״̬��4�������Ļص�����
static void close_video(void);
static void set_state(void);
static void pause_resume_video(void);

int video_player_exec(char *fname)
{
	int ret;
	my_printf("video player execute: %s\n\r", fname);
	
	//ע�����а���
	key_register_cb(KEY_SEL, set_state, NULL, NULL, 0, 0);
	key_register_cb(KEY_DEC, pause_resume_video, NULL, NULL, 0, 0);
	key_register_cb(KEY_INC, close_video, NULL, NULL, 0, 0);

	ret = video_player_start(fname);
	return ret;
}

//��Ƶ����״̬�°����Ĺ���
static void close_video(void)
{
	my_printf("Close video\n\r");
	video_player_stop();
	
	dir_list_form_reload();
}

//�˰��������ı�����2�������Ĺ���
static int key_func_flag = 0;

static void seek_video_inc(void);
static void seek_video_dec(void);
static void seek_video_end_cb(void);
static void set_video_volume_inc(void);
static void set_video_volume_dec(void);

static void set_state(void)
{	
	//�ǲ���״̬������������LCD����ʾ������
	if(video_player_obj==NULL || video_player_obj->state!=VIDEO_PLAYER_PLAYING)
		return;

	key_func_flag++;
	
	if(key_func_flag == 1)
	{	//���������벥�Ŷ�λ״̬
		video_get_pos();
		key_register_cb(KEY_DEC, seek_video_dec, seek_video_end_cb, seek_video_dec, 600, 10);
		key_register_cb(KEY_INC, seek_video_inc, seek_video_end_cb, seek_video_inc, 600, 10);
	}
	else if(key_func_flag == 2)
	{	//������������������״̬
		video_get_volume();
		key_register_cb(KEY_DEC, set_video_volume_dec, NULL, set_video_volume_dec, 500, 10);
		key_register_cb(KEY_INC, set_video_volume_inc, NULL, set_video_volume_inc, 500, 10);
	}
	else
	{	//�����Բ������Ŀ���, ���ظ�2�������Ĺ���
		key_func_flag = 0;
		key_register_cb(KEY_DEC, pause_resume_video, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, close_video, NULL, NULL, 0, 0);
		video_resume();
	}
}

static void pause_resume_video(void)
{
	int ret = 0;
	
	if(video_player_obj != NULL)
	{
		if(video_player_obj->state == VIDEO_PLAYER_PLAYING)
		{
			ret = video_pause();
			if(ret == 0)
				video_player_obj->state = VIDEO_PLAYER_PAUSE;			
		}
		else
		{
			ret = video_resume();
			if(ret == 0)
				video_player_obj->state = VIDEO_PLAYER_PLAYING;		
		}
		
	}
}

//========================================================
static void seek_video_inc(void)
{
	int ret = 0;
	if(video_player_obj != NULL)
	{
		video_player_obj->state = VIDEO_PLAYER_PAUSE;
		int pos = video_get_pos();
		if(pos < 9800)
		{
			pos+=200;      //ǰ��2%
			ret = video_seek(pos);
			if(ret != 0)
			{
				video_player_stop();
			}
		}
	}
}

static void seek_video_dec(void)
{
	int ret = 0;
	if(video_player_obj!= NULL)
	{
		video_player_obj->state = VIDEO_PLAYER_PAUSE;
		int pos = video_get_pos();
		if(pos > 200)
		{
			pos-=200;       //����2%
			ret = video_seek(pos);
			if(ret != 0)
			{
				video_player_stop();
			}
		}
	}	
}

static void seek_video_end_cb(void)
{
	if(video_player_obj != NULL)
	{
		video_player_obj->state = VIDEO_PLAYER_PLAYING;
	}
}


static void set_video_volume_inc(void)
{
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PLAYING)
	{
		int volume = video_get_volume();
		if(volume < 100)
		{
			volume++;
			video_set_volume(volume);
		}
	}	
}

static void set_video_volume_dec(void)
{
	if(video_player_obj!=NULL && video_player_obj->state==VIDEO_PLAYER_PLAYING)
	{
		int volume = video_get_volume();
		if(volume > 0)
		{
			volume--;
			video_set_volume(volume);
		}
	}	
}


/** @breif ���Ŷ�λ ����������2������״̬������Ļص�����
 **/
void vid_ctrl_panel_hide(void)
{
	//�ǲ���״̬������������LCD����ʾ������
	if(video_player_obj==NULL || video_player_obj->state!=VIDEO_PLAYER_PLAYING)
		return;

	if(key_func_flag != 0)
	{
		key_func_flag = 0;
		key_register_cb(KEY_DEC, pause_resume_video, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, close_video, NULL, NULL, 0, 0);
	}
}






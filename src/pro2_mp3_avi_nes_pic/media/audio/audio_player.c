#include "audio_player.h"
#include "audio_player_ui.h"
#include "audio_decode.h"

#include "libc.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "user_task.h"
#include "myfat.h"

#include "cpu_related_v3.h"
#include "audio_card.h"
#include "media_timer_v2.h"
#include "dir_list.h"
#include "key.h"

static audio_player_obj_t *audio_player_obj = NULL;
static task_control_block_t tcb_audio_player;  //��Ƶ������Task Control Block
static int player_mode = SINGLE;               //Ĭ�ϵ�������

#define  KEY_SET_EXIT_TIME  8

//=============================================================================================
//ʱ����ͼ�������
#define  PICK_DOT_SKIP      4        //2.8����(xres=320)ÿ4����ȡһ����ʾ (1152/320)��ȡ��Ϊ4

static const wave_disp_t wave_disp = 
{
	.wave_center_y =  178,   //��������y����
	.wave_max_hight = 50,

	.pick_dot_skip = PICK_DOT_SKIP,  //ÿ���ٸ���ȡһ��
	.wave_draw_color = LCD_CYAN,
	.wave_border_color = LCD_LIGHTGRAY,
	.wave_center_color = LCD_LIGHTCYAN,
};

//���ֲ�����UI����
static const audio_player_page_t audio_player_page = 
{
	//page share
	.back_color = LCD_BLACK,
	.font_color = LCD_WHITE,
	.font_size = 16,

	//play song
	.song_y = 16,

	.progress_bar = 
	{
		.sx = 16,
		.sy = 40,        //song_y+font_size+8
		.ex = 320-16-1,  //lcd_dev.xres-sx-1
		.ey = 40+8-1,
		.pass_color = LCD_YELLOW,
		.undue_color = LCD_GRAY,
	},

	.time_x = 16,
	.time_y = 56,    //progress_bar.ey+1 + 8

	.volume_x = 16,
	.volume_y = 80, //time_y+font_size+8

	.mode_x = 16,
	.mode_y = 104,    //volume_y+font_size+8

	.pause_tip_color = LCD_RED,
	.pause_tip_x = 320/2,  //lcd_dev.xres/2
	.pause_tip_y = 104,    //mode_y

	.set_tip_color = LCD_MAGENTA,
	.set_tip_x = 320/2,    //lcd_dev.xres/2
	.set_tip_y = 80,       //volume_y
};

//=============================================================================================
static void audio_card_hungry_callback(void);
static void audio_timer_isr(void);
static void audio_player_1s_hook(void);

/** @breif ����������(�ò�������ʼ����fname�ļ�)
 ** @param fname-��������Ƶ�ļ���
 */
int  audio_player_start(char *fname)
{
	int ret;

	if(audio_player_obj != NULL)
	{
		audio_player_stop();
	}

	//������Ƶ
//	cpu_over_clock_run(RCC_CFGR_PLLMULL10);         //�������Ҫ������ʾ, ���Բ���Ƶ

	audio_player_obj = mem_malloc(sizeof(audio_player_obj_t));
	if(audio_player_obj == NULL)
	{
		return -1;
	}

	ret = audio_file_open(fname);
	if(ret != 0)
	{
		mem_free(audio_player_obj);
		audio_player_obj = NULL;
		return -2;
	}
	ret = audio_file_get_attr_info(&audio_player_obj->sample_freq, &audio_player_obj->channels, &audio_player_obj->sample_bits);
	if(ret != 0)
	{
		audio_file_close();
		mem_free(audio_player_obj);
		audio_player_obj = NULL;
		return -3;
	}

	//��ȡһ����Ƶ����
	ret = audio_file_decode_stream(audio_player_obj->pcm_buff, PCM_BUFF_SIZE);
	if(ret <= 0)
	{
		audio_file_close();
		mem_free(audio_player_obj);
		audio_player_obj = NULL;
		return -4;		
	}
	audio_player_obj->pcm_pkg_size = ret;

	//������
	ret = audio_card_open(audio_player_obj->sample_freq, audio_player_obj->channels, audio_player_obj->sample_bits, audio_player_obj->pcm_buff, audio_player_obj->pcm_pkg_size);
	if(ret != 0)
	{
		audio_file_close();
		mem_free(audio_player_obj);
		audio_player_obj = NULL;
		return -5;
	}

	//ע��ص�����,�������������
	audio_card_register_feeder(audio_card_hungry_callback);
//	if(audio_card_get_volume() < 26)	//�˴�����������С����
//		audio_card_chg_volume(26);

	//���ò�����״̬
	audio_player_obj->state = AUDIO_PLAYER_PLAYING;
	audio_player_obj->key_set_exit_time = 0;
	audio_player_obj->key_set_identify = 0;
	audio_player_obj->key_set_state_watch = AUDIO_PLAYER_IDLE;
	audio_player_obj->wave_frame_cnt = 0;
	
	//������Ƶ��������
	tcb_audio_player.name = "audio_player";
	tcb_audio_player.state = TS_STOPPED;
	tcb_audio_player.is_always_alive = 0;
	tcb_audio_player.action = audio_player_cycle;
	tcb_audio_player.mark = 0;
	task_create(&tcb_audio_player);

	//����һ��1s�Ķ�ʱ��
	//10KHz��100us, 100us*10000=1s
	media_timer_start(10000, 10000, audio_timer_isr);	//10KHz��100us, 100us*10000=1000ms=1s

	return 0;
}

//ֻ�輤����Ƶ��������, ��������ιʳ����������(����״̬���������뼰ʱιʳ,��������)
static void audio_card_hungry_callback(void)
{
	tcb_audio_player.mark |= 0x01;
	task_wakeup(&tcb_audio_player);
}

//Timer��ʱ���� 1000ms
static void audio_timer_isr(void)
{
	tcb_audio_player.mark |= 0x02;
	task_wakeup(&tcb_audio_player);
}

/** @brief ������Ƶ������(��Ƶ����)
 */
void audio_player_cycle(void)
{
	if(audio_player_obj == NULL)
		return;

	int ret, task_mark;

	ALLOC_CPU_SR();
	ENTER_CRITICAL();
	task_mark = tcb_audio_player.mark;
	tcb_audio_player.mark = 0;
	EXIT_CRITICAL();

	if(task_mark & 0x01)
	{	//���벢ιʳ����
		if(audio_player_obj->state == AUDIO_PLAYER_PLAYING)
		{
			ret = audio_file_decode_stream(audio_player_obj->pcm_buff, PCM_BUFF_SIZE);
			if(ret > 0)
			{
				audio_card_write(audio_player_obj->pcm_buff, ret);

				//��ʾ������ʱ����(ÿ2����Ƶ������ʾһ������; ���׷��ʵʱ��, ����ÿ�����ݶ���ʾ����)
				audio_player_obj->wave_frame_cnt++;
				if((audio_player_obj->wave_frame_cnt & 0x01) == 0)
					lcd_draw_sound_wave(&wave_disp, audio_player_obj->pcm_buff, audio_player_obj->pcm_pkg_size, audio_player_obj->channels, audio_player_obj->sample_bits);
			}
			else
			{	//�ļ���ȡ���� �� �ļ���ȡ���
				my_printf("Audio play finish, ret = %d\n\r", ret);
				audio_player_obj->key_set_exit_time = 1;
				audio_player_1s_hook();
				audio_player_stop();
				task_mark = 0;           //ʹ�����if((task_mark & 0x02))����ִ��

				//����ѭ����ѭ����Ŀ
				if(player_mode == SINGLE_CYCLE)
					ret = dir_list_item_auto_run(0, NULL);
				else if(player_mode == CYCLIC_SONGS)
					ret = dir_list_item_auto_run(1, NULL);

				return;
			}
		}
		else
		{
			u_memset(audio_player_obj->pcm_buff, 0x00, audio_player_obj->pcm_pkg_size);
			audio_card_write(audio_player_obj->pcm_buff, audio_player_obj->pcm_pkg_size);
//			my_printf("Audio player pause, Feed 0x00 to sound card!\n\r");
		}
	}

	if(task_mark & 0x02)
	{	//1s��ʱ����
		audio_player_1s_hook();
	}
}

static void close_audio(void);
/** @brief �ر���Ƶ������
 */
void audio_player_stop(void)
{
	if(audio_player_obj != NULL)
	{
		int total_time = audio_file_get_total_time();
		int played_time = audio_file_get_played_time();

		media_timer_stop();
		task_delete(&tcb_audio_player);    //ɾ��audio����

		audio_card_close();    //�ر�����

		audio_file_close();		

		mem_free(audio_player_obj);
		audio_player_obj = NULL;

//		cpu_normal_clock_run();   //������ĳ�Ƶ��Ӧ

		//��δ�������
		if(total_time != played_time) {
			lcd_draw_progress_bar(&audio_player_page.progress_bar, 0, 1);
			audio_player_disp_play_time(&audio_player_page, 0, 0);
			lcd_clear_sound_wave(&wave_disp);
		}

		key_unregister_cb(KEY_SEL);    //ֻ�����뿪����
		key_unregister_cb(KEY_DEC);
		key_register_cb(KEY_INC, close_audio,  NULL, NULL, 0, 0);
	}
}

//====================================================================================
static void close_audio(void);
static void set_state(void);
static void pause_resume_audio(void);

int audio_player_exec(char *fname)
{
	int i, ret;

	//ע�����а���
	key_register_cb(KEY_SEL, set_state, NULL, NULL, 0, 0);
	key_register_cb(KEY_DEC, pause_resume_audio, NULL, NULL, 0, 0);
	key_register_cb(KEY_INC, close_audio,  NULL, NULL, 0, 0);

//	ret = audio_player_start(fname);            //�Ƶ�����

	lcd_dev.backcolor = audio_player_page.back_color;
	lcd_clear(lcd_dev.backcolor);

	char *played_audio_name = fname;
	i = u_strlen(fname)-1;
	while(i > 0)
	{
		if(fname[i] == '/')
		{
			played_audio_name = &fname[i+1];
			break;
		}
		i--;
	}

	audio_player_disp_song_name(&audio_player_page, played_audio_name, lcd_dev.xres);
	ret = audio_player_start(fname);
	if(ret == 0)
	{
		lcd_draw_progress_bar(&audio_player_page.progress_bar, 0, 1);
		audio_player_disp_play_time(&audio_player_page, audio_file_get_total_time(), 0);
		audio_player_disp_volume(&audio_player_page, "����:", audio_card_get_volume());

		if(player_mode == SINGLE)
			audio_player_disp_play_mode(&audio_player_page, "��������");
		else if(player_mode == SINGLE_CYCLE)
			audio_player_disp_play_mode(&audio_player_page, "����ѭ��");
		else
			audio_player_disp_play_mode(&audio_player_page, "ѭ����Ŀ");
	}

	return ret;
}	

static void key_set_exit_check(void);

static void audio_player_1s_hook(void)
{
	int played_time, total_time;

	total_time = audio_file_get_total_time();
	played_time = audio_file_get_played_time();
	audio_player_disp_play_time(&audio_player_page, total_time, played_time);
	lcd_draw_progress_bar(&audio_player_page.progress_bar, 10000*played_time/total_time, 0);

	key_set_exit_check();
}

//==========================================================================
//��������1
static void close_audio(void)
{
	audio_player_stop();

	dir_list_form_reload();
}

static void seek_audio_dec(void);
static void seek_audio_inc(void);
static void seek_audio_end_cb(void);
static void set_audio_volume_dec(void);
static void set_audio_volume_inc(void);
static void set_player_mode_dec(void);
static void set_player_mode_inc(void);

static void key_set_exit_check(void)
{
	if(audio_player_obj->key_set_exit_time > 0)
	{
		audio_player_obj->key_set_exit_time--;
		if(audio_player_obj->key_set_exit_time == 0)
		{
			audio_player_obj->key_set_identify = 0;

			audio_player_disp_set_tip(&audio_player_page, NULL);

//			key_register_cb(KEY_SEL, set_state, NULL, NULL, 0, 0);
			key_register_cb(KEY_DEC, pause_resume_audio, NULL, NULL, 0, 0);
			key_register_cb(KEY_INC, close_audio, NULL, NULL, 0, 0);
		}
	}
}

static void set_state(void)
{
	if(audio_player_obj == NULL)
		return;

	audio_player_obj->key_set_identify++;
	
	if(audio_player_obj->key_set_identify == 1)
	{
		audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;
		audio_player_disp_set_tip(&audio_player_page, "��������");

		key_register_cb(KEY_DEC, set_audio_volume_dec, NULL, set_audio_volume_dec, 1000, 40);
		key_register_cb(KEY_INC, set_audio_volume_inc, NULL, set_audio_volume_inc, 1000, 40);
	}
	else if(audio_player_obj->key_set_identify == 2)
	{
		audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;
		audio_player_disp_set_tip(&audio_player_page, "���/����");

		key_register_cb(KEY_DEC, seek_audio_dec, seek_audio_end_cb, seek_audio_dec, 1000, 40);
		key_register_cb(KEY_INC, seek_audio_inc, seek_audio_end_cb, seek_audio_inc, 1000, 40);
	}
	else if(audio_player_obj->key_set_identify == 3)
	{
		audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;
		audio_player_disp_set_tip(&audio_player_page, "ģʽ����");

		key_register_cb(KEY_DEC, set_player_mode_dec, NULL, NULL, 0, 0);
		key_register_cb(KEY_INC, set_player_mode_inc, NULL, NULL, 0, 0);
	}
	else
	{
		//�˳�����seek ���� ��������״̬
		audio_player_obj->key_set_exit_time = 1;
		key_set_exit_check();
	}
}

static void pause_resume_audio(void)
{
	if(audio_player_obj != NULL)
	{
		if(audio_player_obj->state == AUDIO_PLAYER_PLAYING)
		{
			audio_player_obj->state = AUDIO_PLAYER_PAUSE;
			audio_player_disp_pause_tip(&audio_player_page, "��ͣ");
		}
		else
		{
			audio_player_obj->state = AUDIO_PLAYER_PLAYING;
			audio_player_disp_pause_tip(&audio_player_page, NULL);			
		}
	}
}

//=====================================================================================
//��������2
static void seek_audio_dec(void)
{
	if(audio_player_obj != NULL)
	{
		int total_time, played_time;
		int ret, percent;

		if(audio_player_obj->key_set_state_watch == AUDIO_PLAYER_IDLE)
		{
			audio_player_obj->key_set_state_watch = audio_player_obj->state;
			audio_player_obj->state = AUDIO_PLAYER_SEEK;
		}

		total_time = audio_file_get_total_time();
		played_time = audio_file_get_played_time();
		percent = 10000 * played_time / total_time;
		if(percent > 100)
		{
			if(total_time<100 && percent>300)
				percent -= 300;
			else
				percent -= 100;
			ret = audio_file_seek(percent);
			if(ret < 0) {
				audio_player_stop();
			}
			audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;

			total_time = audio_file_get_total_time();
			played_time = audio_file_get_played_time();
			audio_player_disp_play_time(&audio_player_page, total_time, played_time);
			lcd_draw_progress_bar(&audio_player_page.progress_bar, 10000*played_time/total_time, 1);
		}
	}
}

static void seek_audio_inc(void)
{
	if(audio_player_obj != NULL)
	{
		int total_time, played_time;
		int ret, percent;

		if(audio_player_obj->key_set_state_watch == AUDIO_PLAYER_IDLE)
		{
			audio_player_obj->key_set_state_watch = audio_player_obj->state;
			audio_player_obj->state = AUDIO_PLAYER_SEEK;
		}

		total_time = audio_file_get_total_time();
		played_time = audio_file_get_played_time();
		percent = 10000 * played_time / total_time;
		if(percent < 9900)
		{
			if(total_time<100 && percent<9700)
				percent += 300;
			else
				percent += 100;
			ret = audio_file_seek(percent);
			if(ret < 0) {
				audio_player_stop();
			}
			audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;

			total_time = audio_file_get_total_time();
			played_time = audio_file_get_played_time();
			audio_player_disp_play_time(&audio_player_page, total_time, played_time);
			lcd_draw_progress_bar(&audio_player_page.progress_bar, 10000*played_time/total_time, 1);
		}
	}
}

static void seek_audio_end_cb(void)
{
	if(audio_player_obj != NULL)
	{
		if(audio_player_obj->key_set_state_watch != AUDIO_PLAYER_IDLE)
		{
			audio_player_obj->state = audio_player_obj->key_set_state_watch;
			audio_player_obj->key_set_state_watch = AUDIO_PLAYER_IDLE;
		}

		int ret = audio_file_decode_stream(audio_player_obj->pcm_buff, PCM_BUFF_SIZE);   //seek����������һ��(�ӵ�����������)
		if(ret <= 0) {
			audio_player_stop();
		}
	}
}


static void set_audio_volume_dec(void)
{
	if(audio_player_obj != NULL)
	{
		int cur_volume = audio_card_get_volume();
		if(cur_volume > 0)
		{
			cur_volume--;
			audio_card_chg_volume(cur_volume);

			audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;
			audio_player_disp_volume(&audio_player_page, NULL, cur_volume);
		}
	}
}

static void set_audio_volume_inc(void)
{
	if(audio_player_obj != NULL)
	{
		int cur_volume = audio_card_get_volume();
		if(cur_volume < 100)
		{
			cur_volume++;
			audio_card_chg_volume(cur_volume);

			audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;
			audio_player_disp_volume(&audio_player_page, NULL, cur_volume);
		}
	}
}


static void set_player_mode_dec(void)
{
	if(audio_player_obj != NULL)
	{
		player_mode--;
		if(player_mode < 0)
			player_mode = CYCLIC_SONGS;
		if(player_mode == SINGLE)
			audio_player_disp_play_mode(&audio_player_page, "��������");
		else if(player_mode == SINGLE_CYCLE)
			audio_player_disp_play_mode(&audio_player_page, "����ѭ��");
		else
			audio_player_disp_play_mode(&audio_player_page, "ѭ����Ŀ");
		audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;
	}
}

static void set_player_mode_inc(void)
{
	if(audio_player_obj != NULL)
	{
		player_mode++;
		if(player_mode > CYCLIC_SONGS)
			player_mode = SINGLE;
		if(player_mode == SINGLE)
			audio_player_disp_play_mode(&audio_player_page, "��������");
		else if(player_mode == SINGLE_CYCLE)
			audio_player_disp_play_mode(&audio_player_page, "����ѭ��");
		else
			audio_player_disp_play_mode(&audio_player_page, "ѭ����Ŀ");
		audio_player_obj->key_set_exit_time = KEY_SET_EXIT_TIME;
	}
}




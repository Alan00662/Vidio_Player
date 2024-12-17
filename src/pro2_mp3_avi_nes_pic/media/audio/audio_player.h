#ifndef  __AUDIO_PLAYER_H_
#define  __AUDIO_PLAYER_H_

#include <stdint.h>
#include <stddef.h>
#include "ff.h"


enum audio_player_state_e
{
	AUDIO_PLAYER_IDLE = 0,  //������ǰû���ڲ�����Ƶ�ļ�(����˵��Ƶ�ļ��������)
	AUDIO_PLAYER_PLAYING,   //������ǰ������Ƶ����״̬
	AUDIO_PLAYER_PAUSE,     //������ǰ���ڲ�����ͣ״̬
	AUDIO_PLAYER_SEEK,      //������ǰ���ڿ��/����״̬
};

enum player_mode_e
{
	SINGLE,        //��������
	SINGLE_CYCLE,  //����ѭ��
	CYCLIC_SONGS,  //ѭ����Ŀ
};


//�����pcm���ݻ������Ĵ�С
#define  PCM_BUFF_SIZE  2304*2

typedef struct audio_player_obj_s
{
	int     state;           //��������״̬

	uint8_t __attribute__((aligned(4))) pcm_buff[PCM_BUFF_SIZE];
	int     pcm_pkg_size;    //�����һ��pcm���ݵ�ʵ�ʴ�С, pcm_pkg_size<=PCM_BUFF_SIZE

	int     sample_freq;     //��Ƶ�Ĳ���Ƶ��
	int     channels;        //��Ƶ��������
	int     sample_bits;     //��Ƶ������������(�����ֱ���) 8 16 ...

	int     key_set_exit_time;
	int     key_set_identify;
	int     key_set_state_watch;

	int     wave_frame_cnt;
}audio_player_obj_t;



int  audio_player_start(char *fname);
void audio_player_cycle(void);
void audio_player_stop(void);

int audio_player_exec(char *fname);


#endif



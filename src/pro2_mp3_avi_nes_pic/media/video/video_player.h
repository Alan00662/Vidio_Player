#ifndef  __VIDEO_PLAYER_H_
#define  __VIDEO_PLAYER_H_

#include <stdint.h>

#include "kfifo.h"

#include "video_app.h"

#define  AUDIO_PKG_SIZE   2304*2     //һ����Ƶ�Ĵ�С, ��һ֡MP3�����������Ĵ�С
#define  AUDIO_BUFF_SIZE  32*1024    //����7��, ��ʵ����4������, ����7��ֻ����Ϊ���������Ϊ2^n
#define  AUDIO_PKG_NUM    (AUDIO_BUFF_SIZE)/(AUDIO_PKG_SIZE)    //����7����Ƶ

enum video_player_state_e
{
	VIDEO_PLAYER_FREE = 0,  //������ǰû���ڲ�����Ƶ�ļ�(����˵��Ƶ�ļ��������)
	VIDEO_PLAYER_PLAYING,   //������ǰ������Ƶ����״̬
	VIDEO_PLAYER_PAUSE,     //������ǰ���ڲ�����ͣ״̬
	VIDEO_PLAYER_FINISH,    //�����ող��Ž���
};


typedef struct video_player_obj_s
{
	int      state;           //��������״̬

	uint8_t  audio_buff[AUDIO_BUFF_SIZE]; //��Ƶ������
	struct kfifo audio_fifo;
	uint32_t pcm_size;                    //һ����Ƶ�����Ĵ�С, ��λ:�ֽ�
	uint8_t  temp_buff[AUDIO_PKG_SIZE];
}video_player_obj_t;


int  video_player_start(char *fname);
void video_player_cycle(void);
void video_player_stop(void);

int video_player_seek(int pos);
int video_player_set_volume(int volume);
int video_player_pause(void);
int video_player_resume(void);

int video_player_exec(char *fname);


#endif



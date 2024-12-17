#ifndef  __AUDIO_DECODE_H_
#define  __AUDIO_DECODE_H_


//audio_decode������ṩ�ĺ���, �û�ͨ��������Щ��������ʵ����Ƶ�ļ��Ĳ���
int audio_file_open(char *fname);
int audio_file_get_attr_info(int *sample_freq, int *channels, int *sample_bits);
int audio_file_decode_stream(void *pcm_buff, int size);
int audio_file_close(void);

int audio_file_seek(int pos);
int audio_file_get_played_time(void);
int audio_file_get_total_time(void);




#endif




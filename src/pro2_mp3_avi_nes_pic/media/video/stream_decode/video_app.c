#include "video_app.h"

#include "parse_avi.h"
#include "dec_mjpeg.h"
#include "dec_mp3.h"

//�������ⲿ����
#include "video_support.h"
#include "ff.h"


#define  STREAM_SIZE  48*1024        //һ֡480x320��jpeg�����ᳬ��36KB
//#define  STREAM_SIZE  56*1024          //800*480

#define  AUDIO_PKG_SIZE   2304*2     //һ����Ƶ�Ĵ�С, ��һ֡MP3�����������Ĵ�С

#define  DIS_TIME         8          //������ �����ٶ� ��������ʾʱ�� 


typedef struct video_obj_s
{
	FIL      fil;             //�ļ����
	int      state;           //��������״̬

//	char     file_format[4];  //"AVI" "MP4"��, ���ھ�֧��һ�ָ�ʽ��û��Ҫ���ô˱���

	uint8_t  stream_buff[STREAM_SIZE];    //���ļ��ж�ȡ����Ƶ/��Ƶ��
	
	avi_info_t avi_info;      //�������mp4 flv��������ʽ, ��ʹ��union

	uint32_t block_end_ptr;   //һ���������Ƶ�����λ��
	uint32_t stream_size;	  //����С,������ż��, �����ȡ��Ϊ����,���1��Ϊż��.
	uint16_t stream_id;       //������ID,'dc'=0X6364 'wb'==0X6277

	int      pcm_size;                    //һ����Ƶ�����Ĵ�С, ��λ:�ֽ�
	uint8_t  pcm_buff[AUDIO_PKG_SIZE];
	
	int      time_cnt;        //�����
	player_dis_info_t player_dis_info;
}video_obj_t;

static video_obj_t  *video_obj = NULL;

static void print_avi_info(avi_info_t *avi_info_ptr)
{
	vid_printf("avi_info_ptr->sec_per_frame = %u\n\r", avi_info_ptr->sec_per_frame);
	vid_printf("avi_info_ptr->total_frame = %u\n\r", avi_info_ptr->total_frame);
	vid_printf("avi_info_ptr->width = %u\n\r", avi_info_ptr->width);
	vid_printf("avi_info_ptr->height = %u\n\r", avi_info_ptr->height);
	vid_printf("avi_info_ptr->compression = 0x%X\n\r", avi_info_ptr->compression);
	
	vid_printf("avi_info_ptr->sample_rate = %u\n\r", avi_info_ptr->sample_rate);
	vid_printf("avi_info_ptr->channels = %u\n\r", avi_info_ptr->channels);
	vid_printf("avi_info_ptr->format_tag = %u\n\r", avi_info_ptr->format_tag);
	vid_printf("avi_info_ptr->block_align = %u\n\r", avi_info_ptr->block_align);
	
	vid_printf("avi_info_ptr->stream_start = %u\n\r", avi_info_ptr->stream_start);
	vid_printf("avi_info_ptr->stream_end = %u\n\r", avi_info_ptr->stream_end);
	vid_printf("avi_info_ptr->video_flag = %s\n\r", avi_info_ptr->video_flag);
	vid_printf("avi_info_ptr->audio_flag = %s\n\r", avi_info_ptr->audio_flag);
}

static int _video_get_audio_pkg_size(void);


int video_open_file(char *fname)
{
	int ret, avi_idx_chk_size;
	uint32_t rd_len;

	if(video_obj != NULL)    //˵�����ڲ�����Ƶ, Ҫ�ȹر�
	{
		video_close();
	}

	video_obj = vid_malloc(sizeof(video_obj_t));  //������Ƶ����������
	if(video_obj == NULL)
	{
		return -1;
	}

	ret = f_open(&video_obj->fil, fname, FA_READ);
	if(ret != FR_OK)
	{
		vid_free(video_obj);
		video_obj = NULL;
		return -2;
	}

	ret = f_read(&video_obj->fil, video_obj->stream_buff, STREAM_SIZE, &rd_len);
	if(ret!=FR_OK || rd_len!=STREAM_SIZE)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -3;	
	}
	
	//��ȡavi�ļ������������Ϣ
	ret = avi_parse(video_obj->stream_buff, STREAM_SIZE, &video_obj->avi_info);
	if(ret < 0)
	{
		vid_printf("avi_parse fail, ret=%d\n\r", ret);
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -4;		
	}
	print_avi_info(&video_obj->avi_info);   //���Դ�ӡһ��video_obj->avi_info
//	if(video_obj->avi_info.compression!=HANDLER_MJPG || video_obj->avi_info.format_tag!=AUDIO_FORMAT_MP3)
//	{	//��֧��ͼ��JPEGѹ�� ��������PCM
//		f_close(&video_obj->fil);
//		vid_free(video_obj);
//		video_obj = NULL;
//		return -5;
//	}
	if(video_obj->avi_info.compression!=HANDLER_MJPG)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -5;		
	}

	/******** ����IndexChunk **********/
	f_lseek(&video_obj->fil, video_obj->avi_info.stream_end);
	ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
	if(ret!=FR_OK || rd_len!=8)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -6;	
	}
	avi_idx_chk_size = avi_idx_chunk_size(video_obj->stream_buff, 8);
	vid_printf("\nIndexChunk size = %d\n\r", avi_idx_chk_size);
//	my_printf("f_size(&video_obj->fil) = %d\n\r", (uint32_t)f_size(&video_obj->fil));
//	my_printf("video_obj->avi_info.stream_end+avi_idx_chk_size = %d\n\r", video_obj->avi_info.stream_end+avi_idx_chk_size);

	/***** ˵�����еڶ������ ******/
	if(f_size(&video_obj->fil) > video_obj->avi_info.stream_end+avi_idx_chk_size)
	{
		f_lseek(&video_obj->fil, video_obj->avi_info.stream_end+avi_idx_chk_size);
		ret = f_read(&video_obj->fil, video_obj->stream_buff, STREAM_SIZE, &rd_len);
		if(ret != FR_OK)
		{
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -7;
		}
		ret = avi_parse2(video_obj->stream_buff, rd_len, &video_obj->avi_info);   //�����buff��ƫ��
		video_obj->avi_info.stream_start2 += video_obj->avi_info.stream_end + avi_idx_chk_size;  //ת��Ϊ������ļ���ƫ��
		video_obj->avi_info.stream_end2 += video_obj->avi_info.stream_end + avi_idx_chk_size;
	}
	else
	{	//���Ϊ0, ˵��û�еڶ���movi
		video_obj->avi_info.stream_start2 = 0;
		video_obj->avi_info.stream_end2 = 0;
	}

	video_obj->time_cnt = DIS_TIME;
	video_obj->player_dis_info.adjust = 0;
	video_obj->player_dis_info.speed = 100;

	ret = jpeg_decode_init();          //jpeg�����ʼ��
	if(ret != 0)
	{
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -8;
	}

	if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3) {
		ret = mp3_decode_init();
		if(ret != 0)
		{
			jpeg_decode_finish();
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -9;
		}
	}

	/************  �Ȳ��ŵ�һ���� ***********/
	video_obj->block_end_ptr = video_obj->avi_info.stream_end;  

	f_lseek(&video_obj->fil, video_obj->avi_info.stream_start);
	ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
	if(ret != FR_OK) {
		if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3)
		{	mp3_decode_finish();	}
		jpeg_decode_finish();
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -10;		
	}
	ret = avi_get_stream_info(&video_obj->stream_buff[0], &video_obj->stream_id, &video_obj->stream_size);
	if(ret != 0)
	{
		if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3)
		{	mp3_decode_finish();	}
		jpeg_decode_finish();
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
		return -11;		
	}
	
	if(video_obj->avi_info.format_tag == AUDIO_FORMAT_MP3)
	{
		video_obj->pcm_size = _video_get_audio_pkg_size();
		vid_printf("video_obj->pcm_size = %d\n\r", video_obj->pcm_size);
		if(video_obj->pcm_size < 0)
		{
			mp3_decode_finish();
			jpeg_decode_finish();
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -12;			
		}

		//��������
		ret = vid_open_sys_audio(video_obj->avi_info.sample_rate, video_obj->avi_info.channels, 16, video_obj->pcm_buff, video_obj->pcm_size);
		if(ret < 0)
		{
			mp3_decode_finish();
			jpeg_decode_finish();
			f_close(&video_obj->fil);
			vid_free(video_obj);
			video_obj = NULL;
			return -13;			
		}
	}
	
	if(video_obj->avi_info.format_tag != AUDIO_FORMAT_MP3)
	{
		return video_obj->avi_info.sec_per_frame;
	}
	return 0;
}


static int _video_get_audio_pkg_size(void)
{
	int ret, pkg_size = 0;
	uint32_t rd_len;

decode_stream:
	while(video_obj->stream_size==0 || video_obj->stream_size+8>STREAM_SIZE)
	{	//�쳣֡(��֡�볬��֡)����, ������Ļ��϶�����ɲ�����ֹ
		if(video_obj->stream_size == 0)
		{
			vid_printf("Warning! stream size is 0!!\n\r");
			vid_printf("NULL stream attribute data:\n\r");
			vid_printf_arr(video_obj->stream_buff, 8, 0);		
		}
		else
		{
			vid_printf("Warning! Stream too large, abandon it!!\n\r");
			vid_printf("video_obj->stream_size = %uB %uKB\n\r", video_obj->stream_size, video_obj->stream_size>>10);
			f_lseek(&video_obj->fil, video_obj->fil.fptr + video_obj->stream_size);			
		}
		
		ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
		if(ret != FR_OK) {
			return -1;
		}
		if(video_obj->fil.fptr >= video_obj->block_end_ptr)
		{
			vid_printf("video play finish\n\r");		
			return -2;
		}

		ret = avi_get_stream_info(&video_obj->stream_buff[0], &video_obj->stream_id, &video_obj->stream_size);
		if(ret != 0)
		{
			vid_printf("Error2! can't find stream start flag!!\n\r");
			return -4;
		}
	}
	
	/***************** ����һ֡��Ƶ��������Ƶ�� ******************/
	if(video_obj->stream_id == STREAM_VIDS_FLAG)
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			vid_printf("Disk read error!\n\r");
			return -5;
		}

		//������Ƶ����  ��ʼ���׶�û��Ҫ���
//		ret = jpeg_decode_frame(video_obj->stream_buff, video_obj->stream_size, 0, 0);
//		if(ret != 0)
//		{
//			my_printf("jpeg decode error!\n\r");
//			return -3;
//		}
	}
	else
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			vid_printf("Disk read error!\n\r");
			return -6;
		}
		
		//������Ƶ����
		ret = mp3_decode_stream(video_obj->stream_buff, video_obj->stream_size, video_obj->pcm_buff, AUDIO_PKG_SIZE);
		if(ret < 0) {
			vid_printf("[%s] mp3 decode error! ignore!!\n\r", __FUNCTION__);
//			return -10;
		} else {
			pkg_size = ret;
		}
	}
	
	if(video_obj->fil.fptr >= video_obj->block_end_ptr)
	{
		vid_printf("video play finish\n\r");		
		return -7;		
	}
	
	//��ȡ��һ������������������Ϣ
	ret = avi_get_stream_info(&video_obj->stream_buff[video_obj->stream_size], &video_obj->stream_id, &video_obj->stream_size);
	if(ret != 0)
	{
		vid_printf("Get frame info error!\n\r");
		return -8;
	}
	if(pkg_size > 0)
	{
		return pkg_size;    //�Ѿ���ȡ����Ƶ���ݰ��Ĵ�С, ֱ�ӷ��ؼ���
	}

	goto decode_stream;	
}


//��ȡ��Ƶ����ʱ��  ��λ: s
static double __get_avi_total_time(void)
{
	double avi_total_time = (double)video_obj->avi_info.total_frame * video_obj->avi_info.sec_per_frame / 1000000;  //��λ: s
	return avi_total_time;
}

//��ȡ��Ƶ�Ѿ����ŵ�ʱ��  ��λ: s
static double __get_avi_played_time(void)
{
	double avi_total_time;
	double avi_palyed_time;
	uint32_t played_data_size;
	uint32_t total_data_size;

	avi_total_time = __get_avi_total_time();
	if(video_obj->avi_info.stream_start2 == 0)
	{
		played_data_size = video_obj->fil.fptr - video_obj->avi_info.stream_start;
		total_data_size = video_obj->avi_info.stream_end - video_obj->avi_info.stream_start;
	}
	else
	{
		if(video_obj->fil.fptr < video_obj->avi_info.stream_end)
			played_data_size = video_obj->fil.fptr - video_obj->avi_info.stream_start;
		else
			played_data_size = video_obj->avi_info.stream_end - video_obj->avi_info.stream_start + video_obj->fil.fptr - video_obj->avi_info.stream_start2;
		
		total_data_size = video_obj->avi_info.stream_end - video_obj->avi_info.stream_start + video_obj->avi_info.stream_end2 - video_obj->avi_info.stream_start2;	
	}
	avi_palyed_time = (double)played_data_size * avi_total_time / total_data_size;

	return avi_palyed_time;
}

int video_decode_stream(void)
{
	int ret, attr_oft;
	uint32_t rd_len;

	if(video_obj == NULL)  //�������
		return 1;
	
	while(video_obj->stream_size==0 || video_obj->stream_size+8>STREAM_SIZE)
	{	/************* �쳣֡(��֡�볬��֡)����, ������Ļ��϶�����ɲ�����ֹ ***************/
		if(video_obj->stream_size == 0)
		{
			vid_printf("Warning! stream size is 0!!\n\r");
		}
		else
		{
			vid_printf("Warning! Stream too large, abandon it!!\n\r");
			vid_printf("video_obj->stream_size = %uB %uKB\n\r", video_obj->stream_size, video_obj->stream_size>>10);		
			f_lseek(&video_obj->fil, video_obj->fil.fptr + video_obj->stream_size);
		}
		
		//��ȡ��һ֡(��֡)������(֡ͷ)��Ϣ
		ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
		if(ret != FR_OK) {
			return -2;
		}

get_stream_attr:
		//�ж��Ƿ񲥷Ž���
		if(video_obj->fil.fptr >= video_obj->block_end_ptr)
		{
			vid_printf("change block or avi finish\n\r");
			vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
			vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
			vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
			vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
			
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2) {
				vid_printf("video play finish\n\r");
				video_close();
				return 1;
			}
			else {
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
				if(ret!=FR_OK || rd_len!=STREAM_SIZE) {
					vid_printf("ret=%d\n\r", ret);
					return -3;
				}
				vid_printf("change to second block ^_^\n\r");
			}
		}

		//��ȡ��֡��������Ϣ
		video_obj->stream_size = 0;
		ret = avi_get_stream_info(&video_obj->stream_buff[0], &video_obj->stream_id, &video_obj->stream_size);
		if(ret != 0)
		{
			vid_printf("get stream info fail!\n\r");
			vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
			vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
			vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
			vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
			vid_printf("stream attribute data:\n\r");
			vid_printf_arr(&video_obj->stream_buff[0], 8, 0);

			if(0 == vid_strncmp((const char *)&video_obj->stream_buff[0], "ix", 2))
			{
				uint32_t ix00_size = (uint32_t)video_obj->stream_buff[7]<<24 | \
									(uint32_t)video_obj->stream_buff[6]<<16 | \
									(uint32_t)video_obj->stream_buff[5]<<8 | \
									(uint32_t)video_obj->stream_buff[4];
				vid_printf("ix00_size = 0x%08X, %u\n\r", ix00_size, ix00_size);

				f_lseek(&video_obj->fil, video_obj->fil.fptr+ix00_size);
				ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
				if(ret != FR_OK) {
					vid_printf("ret=%d\n\r", ret);
					return -4;
				}
				vid_printf("After skip ix00_size, the next 8 byte:");
				vid_printf_arr(video_obj->stream_buff, 8, 0);
			
				video_obj->stream_size = 0;
				goto get_stream_attr;
			}				
			
			vid_printf("Error! can't find stream start flag!!\n\r");
			return -5;
		}
	}
	
	//======================================================================================
	//������Ƶ������Ƶ��
	if(video_obj->stream_id == STREAM_VIDS_FLAG)
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			return -6;
		}

		//������Ƶ����
		if(video_obj->time_cnt > 0)
		{
			video_obj->player_dis_info.total_time = __get_avi_total_time();
			video_obj->player_dis_info.played_time = __get_avi_played_time();
			video_obj->player_dis_info.volume = vid_get_sys_volume();
			ret = jpeg_decode_frame(video_obj->stream_buff, video_obj->stream_size, 1, &video_obj->player_dis_info);
		}
		else
		{
			ret = jpeg_decode_frame(video_obj->stream_buff, video_obj->stream_size, 0, 0);
		}
		if(ret != 0)
		{
//			return -2;
			vid_printf("jpeg decode fail, ignore!!\n\r");
		}
	}
	else
	{
		ret = f_read(&video_obj->fil, video_obj->stream_buff, video_obj->stream_size+8, &rd_len);
		if(ret != FR_OK) {
			return -7;
		}
		
		//������Ƶ����
		ret = mp3_decode_stream(video_obj->stream_buff, video_obj->stream_size, video_obj->pcm_buff, AUDIO_PKG_SIZE);
		if(ret < 0)
		{
			vid_memset(video_obj->pcm_buff, 0x00, video_obj->pcm_size);
			vid_printf("mp3 decode fail, ignore!!\n\r");
		}
		
		vid_write_pcm_data(video_obj->pcm_buff, video_obj->pcm_size);      //�����Ƶ
	}	
	
get_stream_attr2:
	//�ж��Ƿ񲥷Ž���
	if(video_obj->fil.fptr >= video_obj->block_end_ptr)
	{
		vid_printf("change block or avi finish\n\r");
		vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
		vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
		vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
		vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
		
		if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2) {
			vid_printf("video play finish\n\r");
			video_close();
			return 1;
		}
		else {
			video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
			f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
			ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
			if(ret != FR_OK ) {
				vid_printf("ret=%d\n\r", ret);
				return -8;
			}
			video_obj->stream_size = 0;
			vid_printf("change to second block ^_^\n\r");
		}
	}

	//��ȡ��֡��������Ϣ
	attr_oft = video_obj->stream_size;
	ret = avi_get_stream_info(&video_obj->stream_buff[attr_oft], &video_obj->stream_id, &video_obj->stream_size);
	if(ret != 0)
	{
		vid_printf("get stream info fail!\n\r");
		vid_printf("fil.fptr = %u\n\r", (uint32_t)video_obj->fil.fptr);
		vid_printf("video_obj->block_end_ptr = %u\n\r", video_obj->block_end_ptr);
		vid_printf("video_obj->avi_info.stream_end = %u\n\r", video_obj->avi_info.stream_end);
		vid_printf("video_obj->avi_info.stream_start2 = %u\n\r", video_obj->avi_info.stream_start2);
		vid_printf("stream attribute data:\n\r");
		vid_printf_arr(&video_obj->stream_buff[attr_oft], 8, 0);

		if(0 == vid_strncmp((const char *)&video_obj->stream_buff[attr_oft], "ix", 2))
		{
			uint32_t ix00_size = (uint32_t)video_obj->stream_buff[attr_oft+7]<<24 | \
								(uint32_t)video_obj->stream_buff[attr_oft+6]<<16 | \
								(uint32_t)video_obj->stream_buff[attr_oft+5]<<8 | \
								(uint32_t)video_obj->stream_buff[attr_oft+4];
			vid_printf("ix00_size = 0x%08X, %u\n\r", ix00_size, ix00_size);

			f_lseek(&video_obj->fil, video_obj->fil.fptr+ix00_size);
			ret = f_read(&video_obj->fil, video_obj->stream_buff, 8, &rd_len);
			if(ret != FR_OK) {
				vid_printf("ret=%d\n\r", ret);
				return -9;
			}
			vid_printf("After skip ix00_size, the next 8 byte:");
			vid_printf_arr(video_obj->stream_buff, 8, 0);
		
			video_obj->stream_size = 0;
			goto get_stream_attr2;
		}
		
		vid_printf("Error! can't find stream start flag!!\n\r");
		return -10;
	}	
	
	//video_obj->stream_size�Ĵ�С�Ƿ�Ϊ0, �Ƿ񳬹���STREAM_SIZE, ��һ�ν���ʱ����ǰ���while�д���
	
	return 0;
}

int video_close(void)
{
	if(video_obj != NULL)
	{
		vid_close_sys_audio();

		mp3_decode_finish();
		jpeg_decode_finish();
		f_close(&video_obj->fil);
		vid_free(video_obj);
		video_obj = NULL;
	}
	return 0;
}


//===============================================================================
//video���ſ��ƺ���

/** @brief ������������
 **/
void video_1s_hook(void)
{
	if(video_obj!=NULL && video_obj->time_cnt>0)
	{
		video_obj->time_cnt--;
		if(video_obj->time_cnt == 0)
		{
			vid_ctrl_panel_hide();
		}
	}
}

/** @brief �����λ��Ƶ�ļ�
 ** @param pos-λ��[0-9999]
 */
int video_seek(int pos)
{
	if(video_obj == NULL)
		return -1;
	if(pos<0 || pos>9999)
		return -2;
	
	int ret, offset;
	uint32_t new_fil_pos, rd_len;

	new_fil_pos = (uint64_t)pos * (video_obj->avi_info.stream_end - video_obj->avi_info.stream_start + video_obj->avi_info.stream_end2 - video_obj->avi_info.stream_start2) / 10000;
	new_fil_pos += video_obj->avi_info.stream_start;
	if(new_fil_pos >= video_obj->avi_info.stream_end) {
		new_fil_pos += (video_obj->avi_info.stream_start2 - video_obj->avi_info.stream_end);
		video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
	}
	else {
		video_obj->block_end_ptr = video_obj->avi_info.stream_end;
	}
	ret = f_lseek(&video_obj->fil, new_fil_pos);
	if(ret != FR_OK)
		return -3;

	//seek��Ѱ����Ƶ��
find_stream_flag:
	ret = f_read(&video_obj->fil, video_obj->stream_buff, STREAM_SIZE, &rd_len);
	if(ret!=FR_OK || rd_len!=STREAM_SIZE) {
		vid_printf("[%s] Error1! Disk read fail\n\r", __FUNCTION__);
		return -4;
	}
	offset = avi_search_id(video_obj->stream_buff, STREAM_SIZE, video_obj->avi_info.video_flag);
	if(offset < 0)
	{
		vid_printf("[%s] Error2! Search stream ID fail, but seek continue\n\r", __FUNCTION__);
		if(video_obj->fil.fptr < video_obj->block_end_ptr)
		{
			goto find_stream_flag;
		}
		else
		{
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2)
				return 1;           //�Ѿ����ҵ��ļ���ĩβ
			else
			{
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				goto find_stream_flag;				
			}	
		}
	}

	video_obj->stream_size = 0;
	ret = avi_get_stream_info(&video_obj->stream_buff[offset], &video_obj->stream_id, &video_obj->stream_size);
	if(ret!=0 || video_obj->stream_size==0 || video_obj->stream_size+8>STREAM_SIZE)
	{
		vid_printf("[%s] Error3! Get stream attr fail, stream_size=%d, but seek continue\n\r", __FUNCTION__, video_obj->stream_size);
		if(video_obj->fil.fptr < video_obj->block_end_ptr)
		{
			goto find_stream_flag;
		}
		else
		{
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2)
				return 1;           //�Ѿ����ҵ��ļ���ĩβ
			else
			{
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				goto find_stream_flag;				
			}	
		}
	}
	f_lseek(&video_obj->fil, video_obj->fil.fptr-(STREAM_SIZE-(offset+8)));	

	//Ѱ�ҵ���Ƶ������������һ֡��Ƶ
	video_obj->time_cnt = DIS_TIME;
	video_obj->player_dis_info.adjust = 1;
	ret = video_decode_stream();
	if(ret != 0) {
		vid_printf("[%s] Error4! Decode fail, ret=%d, but seek continue\n\r", __FUNCTION__, ret);
		if(video_obj->fil.fptr < video_obj->block_end_ptr)
		{
			goto find_stream_flag;
		}
		else
		{
			if(video_obj->avi_info.stream_end2==0 || video_obj->block_end_ptr==video_obj->avi_info.stream_end2)
				return 1;           //�Ѿ����ҵ��ļ���ĩβ
			else
			{
				video_obj->block_end_ptr = video_obj->avi_info.stream_end2;
				f_lseek(&video_obj->fil, video_obj->avi_info.stream_start2);
				goto find_stream_flag;				
			}	
		}
	}

	return 0;
}

/** @brief ��ȡ��Ƶ�ļ��Ѳ��Űٷֱ�
 ** @reval 0-9999   5000-��ʾ50.00%
 */
int video_get_pos(void)
{
	if(video_obj == NULL)
		return -1;

	video_obj->time_cnt = DIS_TIME;             //������Ļ��ʾ
	video_obj->player_dis_info.adjust = 1;	
	return 10000*__get_avi_played_time()/__get_avi_total_time();
}


/**  @brief ���ò�����������
 */
int video_set_volume(int volume)
{
	if(video_obj != NULL)
	{
		video_obj->time_cnt = DIS_TIME;
		video_obj->player_dis_info.adjust = 2;
		return vid_set_sys_volume(volume);
	}
	return 0;
}

/**  @brief ��ȡ������������
 */
int video_get_volume(void)
{
	if(video_obj == NULL)
		return -1;
		
	video_obj->time_cnt = DIS_TIME;          //������Ļ��ʾ
	video_obj->player_dis_info.adjust = 2;	
	return vid_get_sys_volume();
}

/**  @brief ��ͣ����
 */
int video_pause(void)
{
	if(video_obj != NULL)
	{
		video_obj->time_cnt = DIS_TIME;
		video_obj->player_dis_info.adjust = 3;
		
		int i, ret, last_stream;
		for(i=0; i<5; i++)
		{
			last_stream = video_obj->stream_id;
			ret = video_decode_stream();
			if(ret != 0) {	/* ����ʧ�ܻ��߽��� */
				video_close();
				return -1;
			}
			if(last_stream == STREAM_VIDS_FLAG)
				break;
		}
	}
	
	return 0;
}

/** @brief �ָ�����
 */
int video_resume(void)
{
	if(video_obj != NULL)
	{
		video_obj->time_cnt = DIS_TIME;
		video_obj->player_dis_info.adjust = 0;
	}
	return 0;
}












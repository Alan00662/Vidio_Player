#ifndef  __PARSE_AVI_H_
#define  __PARSE_AVI_H_

#include <stdint.h>

#define AVI_RIFF_ID			0X46464952  //'RIFF'
#define AVI_AVI_ID			0X20495641  //'AVI '
#define AVI_LIST_ID			0X5453494C  //'LIST'

#define LIST_HDRL_ID			0X6C726468		//'hdrl', ��Ϣ���־
#define LIST_MOVI_ID			0X69766F6D 		//'movi', ���ݿ��־
#define LIST_HDRL_STRL_ID		0X6C727473		//'strl', LIST(����Ϊhdrl)�е�LIST(����Ϊstrl)�ı�־

#define BLOCK_AVIH_ID			0X68697661 		//'avih', LIST(����Ϊhdrl)���ӿ��־
#define BLOCK_STRH_ID           0X68727473      //'strh', LIST(����Ϊhdrl)�е�LIST(����Ϊstrl)��strh�ӿ��־
#define BLOCK_STRF_ID           0X66727473      //'strf', LIST(����Ϊhdrl)�е�LIST(����Ϊstrl)��strf�ӿ��־

#define INDEX_CHUNK_ID          0X31786469      //'idx1', IndexChunk��ı�־

//AVIStreamHeader�ṹ����س�Ա��ȡֵ
#define STREAM_TYPE_VIDS		0X73646976		//��Ƶ��
#define STREAM_TYPE_AUDS		0X73647561 		//��Ƶ��

#define HANDLER_MJPG		    0X47504A4D      //'MJPG', ��Ƶ���뷽ʽΪMJPG
#define HANDLER_H264            0x34363248      //'H264', ��Ƶ���뷽ʽΪH264
#define HANDLER_AUDIO           0x00000001

#define AUDIO_FORMAT_PCM        0x0001          //��Ƶ����PCM����
#define AUDIO_FORMAT_MP3        0x0055          //��Ƶ����MP3����


//����MovieListʱ����Ƶ������Ƶ����־
#define STREAM_VIDS_FLAG   0X6364		        //'dc', ��Ƶ����־
#define STREAM_AUDS_FLAG   0X6277               //'wb', ��Ƶ����־


//��������
typedef enum {
	AVI_OK = 0,
	AVI_RIFF_ERR,
	AVI_AVI_ERR,
	AVI_LIST_ERR,

	AVI_LIST_HDRL_ERR,
	AVI_LIST_MOVI_ERR,
	AVI_LIST_HDRL_STRL_ERR,

	AVI_BLOCK_AVIH_ERR,
	AVI_BLOCK_STRH_ERR,
	AVI_BLOCK_STRF_ERR,

	AVI_STREAM_ERR,
	AVI_HANDLER_ERR,            //��֧�ֵ�����Ƶ����
	AVI_STRAM_FLAG_ERR,
	AVI_STRAM_LARGE_ERR,        //stream��̫��, buff�Ų���

	AVI_BUFF_OVER_ERR,
}AVISTATUS;


//AVI�ļ�ͷ����Ϣ
typedef struct
{	
	uint32_t RiffID;			//����Ϊ'RIFF', ��0X46464952
	uint32_t FileSize;			//AVI�ļ���С(�����������8�ֽ�, Ҳ����RiffID��FileSize����������)
	uint32_t AviID;				//����Ϊ'AVI ', ��0X41564920 
}AVIHeader;


//LIST����Ϣ, ��Ҫ��������LIST��, hdrl(��Ϣ��)/movi(���ݿ�)/idxl(������, �Ǳ���, �ǿ�ѡ��)
typedef struct
{	
	uint32_t ListID;            //����Ϊ'LIST', ��0X5453494C
	uint32_t BlockSize;			//���С(�����������8�ֽ�,ҲListID��BlockSize����������)
	uint32_t ListType;			//LIST�ӿ�����:hdrl(��Ϣ��)/movi(���ݿ�)/idxl(������, �Ǳ���, �ǿ�ѡ��)
}ListHeader;


//LIST(����Ϊhdrl)��avih�ӿ���Ϣ
typedef struct
{	
	uint32_t BlockID;			//����Ϊ'avih', ��0X68697661
	uint32_t BlockSize;			//���С(�����������8�ֽ�,Ҳ����BlockID��BlockSize����������)
	uint32_t SecPerFrame;		//��Ƶ֡���ʱ��(��λΪus)
	uint32_t MaxByteSec; 		//������ݴ�����, �ֽ�/��
	uint32_t PaddingGranularity; //������������
	uint32_t Flags;				//AVI�ļ���ȫ�ֱ��, �����Ƿ����������
	uint32_t TotalFrame;		//�ļ���֡��
	uint32_t InitFrames;  		//Ϊ������ʽָ����ʼ֡��(�ǽ�����ʽӦ��ָ��Ϊ0)
	uint32_t Streams;			//�������������������, ͨ��Ϊ2
	uint32_t RefBufSize;		//�����ȡ���ļ��Ļ����С(Ӧ���������Ŀ�), Ĭ�Ͽ�����1M�ֽ�!
	uint32_t Width;				//ͼ���
	uint32_t Height;			//ͼ���
	uint32_t Reserved[4];		//����
}MainAVIHeader;


//LIST(����Ϊhdrl)�е�LIST(����Ϊstrl)��strh�ӿ���Ϣ
typedef struct
{	
	uint32_t BlockID;			//����Ϊ'strh', ��0X68727473
	uint32_t BlockSize;			//���С(�����������8�ֽ�, Ҳ����BlockID��BlockSize����������)
	uint32_t StreamType;		//����������, vids(0X73646976)-��Ƶ��, auds(0X73647561)-��Ƶ��
	uint32_t Handler;			//ָ�����Ĵ�����, ��������Ƶ��˵���ǽ�����, ��Ƶ:H264/MJPG/MPEG ��Ƶ:0x00000001
	uint32_t Flags;  			//���: �Ƿ�������������? ��ɫ���Ƿ�仯?
	uint16_t Priority;			//�������ȼ�(���ж����ͬ���͵���ʱ���ȼ���ߵ�ΪĬ����)
	uint16_t Language;			//��Ƶ�����Դ���
	uint32_t InitFrames;  		//Ϊ������ʽָ����ʼ֡��
	uint32_t Scale;				//������, ��Ƶÿ��Ĵ�С������Ƶ�Ĳ�����С
	uint32_t Rate; 				//Scale/Rate=ÿ�������
	uint32_t Start;				//��������ʼ���ŵ�λ��, ��λΪScale
	uint32_t Length;			//��������������, ��λΪScale
 	uint32_t SuggestBuffSize;   //����ʹ�õĻ�������С
    uint32_t Quality;			//��ѹ����������,ֵԽ��,����Խ��
	uint32_t SampleSize;		//��Ƶ��������С
	struct					    //��Ƶ֡��ռ�ľ��� 
	{				
	   	short Left;
		short Top;
		short Right;
		short Bottom;
	}Frame;				
}AVIStreamHeader;


//LIST(����Ϊhdrl)�е�LIST(����Ϊstrl)��strf�ӿ���Ϣ(ǰ��:strh�ӿ��Ѿ�������LISTΪVideo)
typedef struct 
{
	uint32_t BlockID;			//����Ϊ'strf', ��0X66727473
	uint32_t BlockSize;			//���С(�����������8�ֽ�,Ҳ����BlockID�ͱ�BlockSize����������)

	/* λͼ��Ϣͷ */
	struct {
		uint32_t BmpSize;			//bmp�ṹ���С,����(BmpSize����)
		int32_t Width;				//ͼ���, ������Ϊ��λ
		int32_t Height;				//ͼ���, ������Ϊ��λ. ����, ˵��ͼ���ǵ����; ����, ��˵��ͼ���������.
		uint16_t Planes;			//ƽ����, ����Ϊ1
		uint16_t BitCount;			//���ر�����, ��ֵΪ1 4 8 16 24��32
		uint32_t Compression;		//ѹ������, MJPG/H264��
		uint32_t SizeImage;			//ͼ���С, ���ֽ�Ϊ��λ
		int32_t XpixPerMeter;		//ˮƽ�ֱ���, ������/�ױ�ʾ
		int32_t YpixPerMeter;		//��ֱ�ֱ���, ������/�ױ�ʾ
		uint32_t ClrUsed;			//ʵ��ʹ���˵�ɫ���е���ɫ��, ѹ����ʽ�в�ʹ��
		uint32_t ClrImportant;		//��Ҫ����ɫ		
	} bmiHeader;
	
	/* ��ɫ��, ���п��� */
	struct {
		uint8_t  rgbBlue;			//��ɫ������(ֵ��ΧΪ0-255)
		uint8_t  rgbGreen; 			//��ɫ������(ֵ��ΧΪ0-255)
		uint8_t  rgbRed; 			//��ɫ������(ֵ��ΧΪ0-255)
		uint8_t  rgbReserved;		//����, ����Ϊ0		
	} bmColors[0];
}BITMAPINFO;

//LIST(����Ϊhdrl)�е�LIST(����Ϊstrl)��strf�ӿ���Ϣ(ǰ��:strh�ӿ��Ѿ�������LISTΪAudio)
typedef struct 
{
	uint32_t BlockID;			//����Ϊ'strf', ��0X66727473
	uint32_t BlockSize;			//���С(�����������8�ֽ�,Ҳ����BlockID�ͱ�BlockSize����������)
   	uint16_t FormatTag;			//��ʽ��־:0X0001=PCM, 0X0055=MP3...
	uint16_t Channels;	  		//������, һ��Ϊ2,��ʾ������
	uint32_t SampleRate; 		//��Ƶ������
	uint32_t BaudRate;   		//������ 
	uint16_t BlockAlign; 		//���ݿ�����־
	uint16_t Size;				//�ýṹ��С
}WAVEFORMAT; 

//Chunk����Ϣ
typedef struct
{	
	uint32_t ChunkID;         //����Ϊ'idx1', ��0X31786469
	uint32_t ChunkSize;		  //Chunk���С(�����������8�ֽ�,ҲChunkID��ChunkSize����������)
}IndexChunk;

//�û����ĵ�avi��Ϣ
typedef struct avi_info_s
{
	uint32_t sec_per_frame;    //��Ƶ֡���ʱ��(��λΪus)
	uint32_t total_frame;      //��Ƶ��֡��
	uint32_t width;            //ͼ��Ŀ��
	uint32_t height;           //ͼ��ĸ߶�
	uint32_t compression;      //ͼ��ѹ�����뷽ʽ
	
	uint32_t sample_rate;     //��Ƶ������
	uint16_t channels;        //������, һ��Ϊ2,��ʾ������
	uint16_t format_tag;      //��Ƶ��ʽ��־, 0X0001=PCM, 0X0055=MP3...
	uint16_t block_align;     //���ݿ�����־

	uint32_t stream_start;      //movi����ʼ��λ��
	uint32_t stream_end;        //movi��������λ��
	uint32_t stream_start2;     //movi����ʼ��λ�� ��2����Ϊ0ʱ˵��û�еڶ���movi
	uint32_t stream_end2;       //movi��������λ��
	const char *video_flag;     //��Ƶ֡���, VideoFLAG="00dc"/"01dc"
	const char *audio_flag;     //��Ƶ֡���, AudioFLAG="00wb"/"01wb"	
}avi_info_t;


int avi_parse(uint8_t *buff, int size, avi_info_t *avi_info_ptr);
int avi_idx_chunk_size(uint8_t *buff, int size);
int avi_parse2(uint8_t *buff, int size, avi_info_t *avi_info_ptr);
int avi_search_id(uint8_t *buff, int size, const char *id);
int avi_get_stream_info(uint8_t *buff, uint16_t *stream_id, uint32_t *stream_size);


#endif




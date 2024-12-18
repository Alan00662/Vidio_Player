#ifndef  __USB_APP_H_
#define  __USB_APP_H_

#include "usb_core.h"
#include "usbd_core.h"
#include "usb_conf.h"

#include <stdint.h>


#define  BULK_MAX_PACKET_SIZE  0x40	 //USB传输数据包的大小 64字节
#define  DISK_BLOCK_SIZE       512   //磁盘块大小 512字节 

#define  USB_MAS_DISK_READ     0x01
#define  USB_MAS_DISK_WRITE    0x02
#define  USB_MAS_COPY_DATA     0x04

#define  IN_BUFF_SIZE          (DISK_BLOCK_SIZE*4)     //IN_BUFF_SIZE <= OUT_BUFF_SIZE
#define  OUT_BUFF_SIZE         (DISK_BLOCK_SIZE*16)    //与MSC_MEDIA_PACKET保持一致

#define  USB_IN_BUFF_EMPTY    0
#define  USB_IN_BUFF_LOADED   1
#define  USB_OUT_BUFF_EMPTY   0
#define  USB_OUT_BUFF_FULL    1

#define  USB_IN_TRANS_IDLE     0
#define  USB_IN_TRANS_ONGOING  1
#define  USB_OUT_TRANS_IDLE    0
#define  USB_OUT_TRANS_ONGOING 1

#define  OPT_PREPARE  0
#define  OPT_ONGOING  1


typedef struct usb_disk_opt_s
{
	uint32_t opt_mark;   //读写标志
	uint32_t lun;        //磁盘id
	
	uint8_t  usb_out_buff[OUT_BUFF_SIZE];   //OUT事务传输完一个磁盘块就开始写磁盘,
	uint64_t write_addr;                     //且要等到buff数据写入磁盘后才Valid接收端点, 所以OUT事务采用一个buff就可 
	uint32_t write_len;
	uint32_t write_trans_cnt;                //Write10 Cmd写磁盘数据量计数器 write_trans_cnt==write_trans_size 说明完成了写命令
	uint32_t write_trans_size;               //Write10 Cmd要写磁盘的数据量

	uint8_t  usb_in_buff[IN_BUFF_SIZE];      //IN事务采用双缓存, 且缓存多个磁盘块. 这是为了提高读磁盘的速度
 	uint64_t read_addr;
	uint32_t read_len;
	uint32_t read_trans_cnt;               //Read10 Cmd读磁盘数据量计数器 read_trans_cnt==read_trans_size 说明完成了读命令
	uint32_t read_trans_size;              //Read10 Cmd要读磁盘的数据量

    uint8_t  usb_in_buff_load_state;  //USB device要上传的数据是否已经加载到缓存
	uint8_t  usb_out_buff_full_state; //USB device接收到的数据是否已经写满缓存

	uint8_t  usb_in_trans_state;  //数据从RAM->USB传输状态(USB IN事务传输状态) 
	uint8_t  usb_out_trans_state; //数据从USB->RAM传输状态(USB OUT事务传输状态)
	
	uint32_t usb_in_read_disk_speed;   //IN事务读磁盘速度
	uint32_t usb_out_write_disk_speed; //OUT事务写磁盘速度
}usb_disk_opt_t;


extern uint8_t Max_Lun;
extern USB_OTG_CORE_HANDLE USB_OTG_dev;

void USB_device_init(void);
void usb_read_write_disk(void);
void usb_mas_read_memory(int opt_flag, uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length);
void usb_mas_write_memory(int opt_flag, uint8_t lun, uint32_t Memory_Offset, uint32_t Transfer_Length);

void usb_1s_hook(void);


#endif



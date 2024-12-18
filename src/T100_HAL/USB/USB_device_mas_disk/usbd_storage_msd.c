/**
  ******************************************************************************
  * @file    usbd_storage_msd.c
  * @author  MCD application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file provides the disk operations functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "usbd_msc_mem.h"
#include "usb_conf.h"
#include "usb_app.h"

#include "spi_flash_v4.h"
#include "sdio_sdcard_v5.h"
#include "my_printf.h"


/** @defgroup STORAGE_Private_Defines
  * @{
  */


/* USB Mass storage Standard Inquiry Data */
const int8_t  STORAGE_Inquirydata[] = {//36
  
  /* LUN 0 */
  0x00,		
  0x80,		
  0x02,		
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 5),
  0x00,
  0x00,	
  0x00,
  'B', 'o', 'a', 'r', 'd', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
  'S', 'P', 'I', ' ', 'F', 'l', 'a', 's', /* Product      : 16 Bytes */
  'h', ' ', 'D', 'i', 's', 'k', ' ', ' ',
  '1', '.', '0' ,'0',                     /* Version      : 4 Bytes */

	/* LUN 1 */
	0x00,
	0x80,		
	0x02,		
	0x02,
	(USBD_STD_INQUIRY_LENGTH - 5),
	0x00,
	0x00,	
	0x00,
    /* Vendor Identification */
    'B', 'o', 'a', 'r', 'd', ' ', ' ', ' ',
    /* Product Identification */
    'S', 'D', ' ', 'D', 'i', 's', 'k', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    /* Product Revision Level */
    '1', '.', '0', ' '          
}; 

/**
  * @}
  */ 


/** @defgroup STORAGE_Private_FunctionPrototypes
  * @{
  */ 
int8_t STORAGE_Init (uint8_t lun);
int8_t STORAGE_GetCapacity (uint8_t lun, uint32_t *block_num, uint32_t *block_size);
int8_t STORAGE_IsReady (uint8_t lun);
int8_t STORAGE_IsWriteProtected (uint8_t lun);
int8_t STORAGE_Read (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_Write (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_GetMaxLun (void);


const USBD_STORAGE_cb_TypeDef USBD_MICRO_SDIO_fops =
{
	STORAGE_Init,             //MSC_BOT_Init() USBD_STORAGE_fops->Init
	STORAGE_GetCapacity,      //SCSI_ReadCapacity10() & SCSI_ReadFormatCapacity() USBD_STORAGE_fops->GetCapacity
	STORAGE_IsReady,          //SCSI_TestUnitReady() & SCSI_Read10()& SCSI_Write10() USBD_STORAGE_fops->IsReady
	STORAGE_IsWriteProtected, //SCSI_Write10()  USBD_STORAGE_fops->IsWriteProtected
	STORAGE_Read,             //SCSI_Read10()-->SCSI_ProcessRead()  USBD_STORAGE_fops->Read 
	STORAGE_Write,            //SCSI_Write10()-->SCSI_ProcessWrite() USBD_STORAGE_fops->Write
	STORAGE_GetMaxLun,        //USBD_MSC_Setup()  USBD_STORAGE_fops->GetMaxLun
	(int8_t *)STORAGE_Inquirydata, //SCSI_Inquiry() USBD_STORAGE_fops->pInquiry
};

const USBD_STORAGE_cb_TypeDef  *USBD_STORAGE_fops = &USBD_MICRO_SDIO_fops;



/**
  * @brief  Initialize the storage medium
  * @param  lun : logical unit number
  * @retval Status
  */

int8_t STORAGE_Init (uint8_t lun)
{
	switch (lun)
	{
		case 0:			     
			break;			   
		case 1:				 
			break;		  
		default:
			return -1;
	}

	return 0;
}

/**
  * @brief  return medium capacity and block size
  * @param  lun : logical unit number
  * @param  block_num :  number of physical block
  * @param  block_size : size of a physical block
  * @retval Status
  */
int8_t STORAGE_GetCapacity (uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
	switch (lun)
	{
		case 0:
			*block_size = 512;
			*block_num = sf_get_capacity()/512;
			break;			   
		case 1:
			*block_size = 512;
			*block_num = SDCardInfo.CardCapacity/512;
			break;		  
		default:
			*block_size = 512;
			*block_num = 2*1024*1024/512;
			my_printf("[%s] Error! LUN=%u!!\n\r", __FUNCTION__, lun);
			return -1;
	}

	return 0;
}

/**
  * @brief  check whether the medium is ready
  * @param  lun : logical unit number
  * @retval Status
  */
int8_t  STORAGE_IsReady (uint8_t lun)
{
	return 0;
}

/**
  * @brief  check whether the medium is write-protected
  * @param  lun : logical unit number
  * @retval Status
  */
int8_t  STORAGE_IsWriteProtected (uint8_t lun)
{
	return  0;
}

/**
  * @brief  Read data from the medium
  * @param  lun : logical unit number
  * @param  buf : Pointer to the buffer to save data
  * @param  blk_addr :  address of 1st block to be read
  * @param  blk_len : nmber of blocks to be read
  * @retval Status
  */
int8_t STORAGE_Read (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
/*
	int ret;
	switch (lun)		//这里,根据lun的值确定所要操作的磁盘
	{
		case 0:			//磁盘0为 SPI FLASH盘	 
			ret = sf_read_disk((uint32_t)blk_addr*512, buf, blk_len*512);   		  
			break;	  
		case 1:			//磁盘1为SD卡		    
			ret = sd_read_disk((uint64_t)blk_addr*512, buf, blk_len*512);	   
			break;			    
		default:
			ret = -1;
			break;
	}
	if(ret != blk_len*512)
	{
		my_printf("Disk%u read error! ret = %d, addr=%X len=%u!!\n\r", lun, ret, blk_addr, blk_len);
		return -1;
	}
*/
	my_printf("[%s] Error! calling abandon function\n\r", __FUNCTION__);
	return 0;
}
/**
  * @brief  Write data to the medium
  * @param  lun : logical unit number
  * @param  buf : Pointer to the buffer to write from
  * @param  blk_addr :  address of 1st block to be written
  * @param  blk_len : nmber of blocks to be read
  * @retval Status
  */
int8_t STORAGE_Write (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
/*
	int ret;
	switch (lun)		//这里,根据lun的值确定所要操作的磁盘
	{
		case 0:		 	//磁盘0为 SPI FLASH盘	
			ret = sf_write_disk((uint32_t)blk_addr*512, buf, blk_len*512);   		  
			break; 
		case 1:			//磁盘1为SD卡		  
			ret = sd_write_disk((uint64_t)blk_addr*512, buf, blk_len*512);   		  
			break;							  
		default:
			ret = -1;
			break;
	}
	if(ret != blk_len*512)
	{
		my_printf("Disk%u write error! ret = %d!!\n\r", lun, ret);
		return -1;
	}
*/
	my_printf("[%s] Error! calling abandon function\n\r", __FUNCTION__);
	return 0;
}

/**
  * @brief  Return number of supported logical unit
  * @param  None
  * @retval number of logical unit
  */

int8_t STORAGE_GetMaxLun (void)
{
	return (Max_Lun - 1);
}
/**
  * @}
  */ 


/**
  * @}
  */ 


/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


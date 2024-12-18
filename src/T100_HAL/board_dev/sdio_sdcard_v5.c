/* Includes ------------------------------------------------------------------*/
#include "sdio_sdcard_v5.h"
#include "sdio_v3.h"
#include "libc.h"
#include "cpu_related_v3.h"
#include "my_printf.h"
#include "systick.h"

//SD卡SDIO驱动存在的BUG: https://blog.csdn.net/cokewei/article/details/7343915

/** @addtogroup Utilities
  * @{
  */
  
/** @addtogroup STM32_EVAL
  * @{
  */ 

/** @addtogroup Common
  * @{
  */
  
/** @addtogroup STM32_EVAL_SDIO_SD
  * @brief      This file provides all the SD Card driver firmware functions.
  * @{
  */ 

/** @defgroup STM32_EVAL_SDIO_SD_Private_Types
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup STM32_EVAL_SDIO_SD_Private_Defines
  * @{
  */ 
/** 
  * @brief  SDIO Static flags, TimeOut, FIFO Address  
  */
#define NULL 0
#define SDIO_STATIC_FLAGS               ((uint32_t)0x000005FF)
#define SDIO_CMD0TIMEOUT                ((uint32_t)0x00010000)

/** 
  * @brief  Mask for errors Card Status R1 (OCR Register) 
  */
#define SD_OCR_ADDR_OUT_OF_RANGE        ((uint32_t)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((uint32_t)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((uint32_t)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((uint32_t)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((uint32_t)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((uint32_t)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((uint32_t)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((uint32_t)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((uint32_t)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((uint32_t)0x00200000)
#define SD_OCR_CC_ERROR                 ((uint32_t)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((uint32_t)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN     ((uint32_t)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN     ((uint32_t)0x00020000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((uint32_t)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((uint32_t)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((uint32_t)0x00004000)
#define SD_OCR_ERASE_RESET              ((uint32_t)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR            ((uint32_t)0x00000008)
#define SD_OCR_ERRORBITS                ((uint32_t)0xFDFFE008)

/** 
  * @brief  Masks for R6 Response 
  */
#define SD_R6_GENERAL_UNKNOWN_ERROR     ((uint32_t)0x00002000)
#define SD_R6_ILLEGAL_CMD               ((uint32_t)0x00004000)
#define SD_R6_COM_CRC_FAILED            ((uint32_t)0x00008000)

#define SD_VOLTAGE_WINDOW_SD            ((uint32_t)0x80100000)
#define SD_HIGH_CAPACITY                ((uint32_t)0x40000000)
#define SD_STD_CAPACITY                 ((uint32_t)0x00000000)
#define SD_CHECK_PATTERN                ((uint32_t)0x000001AA)
#define SD_VOLTAGE_WINDOW_MMC           ((uint32_t)0x80FF8000)

#define SD_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFF)
#define SD_ALLZERO                      ((uint32_t)0x00000000)

#define SD_WIDE_BUS_SUPPORT             ((uint32_t)0x00040000)
#define SD_SINGLE_BUS_SUPPORT           ((uint32_t)0x00010000)
#define SD_CARD_LOCKED                  ((uint32_t)0x02000000)

#define SD_DATATIMEOUT                  ((uint32_t)0xFFFFFFFF)
#define SD_0TO7BITS                     ((uint32_t)0x000000FF)
#define SD_8TO15BITS                    ((uint32_t)0x0000FF00)
#define SD_16TO23BITS                   ((uint32_t)0x00FF0000)
#define SD_24TO31BITS                   ((uint32_t)0xFF000000)
#define SD_MAX_DATA_LENGTH              ((uint32_t)0x01FFFFFF)

#define SD_HALFFIFO                     ((uint32_t)0x00000008)
#define SD_HALFFIFOBYTES                ((uint32_t)0x00000020)

/** 
  * @brief  Command Class Supported 
  */
#define SD_CCCC_LOCK_UNLOCK             ((uint32_t)0x00000080)
#define SD_CCCC_WRITE_PROT              ((uint32_t)0x00000040)
#define SD_CCCC_ERASE                   ((uint32_t)0x00000020)

/** 
  * @brief  Following commands are SD Card Specific commands.
  *         SDIO_APP_CMD should be sent before sending these commands. 
  */
#define SDIO_SEND_IF_COND               ((uint32_t)0x00000008)

/**
  * @}
  */ 


/** @defgroup STM32_EVAL_SDIO_SD_Private_Macros
  * @{
  */
/**
  * @}
  */ 
  

/** @defgroup STM32_EVAL_SDIO_SD_Private_Variables
  * @{
  */
//static uint32_t CardType =  SDIO_STD_CAPACITY_SD_CARD_V1_1;
//static uint32_t CSD_Tab[4], CID_Tab[4], RCA = 0;
//static uint8_t SDSTATUS_Tab[16];
//__IO uint32_t StopCondition = 0;
//__IO SD_Error TransferError = SD_OK;
//__IO uint32_t TransferEnd = 0;

//SDIO_InitTypeDef SDIO_InitStructure;
//SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
//SDIO_DataInitTypeDef SDIO_DataInitStructure;
SD_CardInfo SDCardInfo;
/**
  * @}			     
  */ 


/** @defgroup STM32_EVAL_SDIO_SD_Private_Function_Prototypes
  * @{
  */
static SD_Error CmdError(void);
static SD_Error CmdResp1Error(uint8_t cmd);
static SD_Error CmdResp7Error(void);
static SD_Error CmdResp3Error(void);
static SD_Error CmdResp2Error(void);
static SD_Error CmdResp6Error(uint8_t cmd, uint32_t *prca);
static SD_Error SDEnWideBus(FunctionalState NewState);
static SD_Error IsCardProgramming(uint8_t *pstatus);
static SD_Error FindSCR(uint32_t rca, uint32_t *pscr);
uint8_t convert_from_bytes_to_power_of_two(uint16_t NumberOfBytes);
  

/**
  * @brief  DeInitializes the SDIO interface.
  * @param  None
  * @retval None
  */
void SD_DeInit(void)
{ 
	sdio_controller_release();
}

/**
  * @brief  Initializes the SD Card and put it into StandBy State (Ready for data 
  *         transfer).
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_Init(void)    //abandon with sd_probe()
{
	SD_Error errorstatus = SD_OK;
	
	SD_DeInit();
	
	/* SDIO Peripheral Low Level Init */
	sdio_controller_init();
	sdio_controller_irq_set(ENABLE);
	delay_ms(10);      //使CLK维持一段时间,让SD卡稳定, 对于有些SD卡是必须的, 否则会初始化失败
	
	SDCardInfo.CardType = SDIO_STD_CAPACITY_SD_CARD_V1_1;
	errorstatus = SD_PowerON();  //获取CardType
	if (errorstatus != SD_OK)
	{
		/*!< CMD Response TimeOut (wait for CMDSENT flag) */
		return(errorstatus);
	}

	errorstatus = SD_InitializeCards(); //获取RCA CSD_Tab[4] CID_Tab[4]
	if (errorstatus != SD_OK)
	{
		/*!< CMD Response TimeOut (wait for CMDSENT flag) */
		return(errorstatus);
	}

	/*!< Configure the SDIO peripheral */
	/*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_TRANSFER_CLK_DIV) */
//	sdio_controller_config(SDIO_TRANSFER_CLK_DIV, SDIO_ClockEdge_Rising, SDIO_BusWide_1b);
  
	if (errorstatus == SD_OK)
	{
		/*----------------- Read CSD/CID MSD registers ------------------*/
//		SD_CSD SD_csd;
		SD_CID SD_cid;
		errorstatus = SD_GetCardInfo(&SDCardInfo, &SDCardInfo.SD_csd, &SD_cid);  //获取CardCapacity CardBlockSize 
	}

	if (errorstatus == SD_OK)
	{
		/*----------------- Select Card --------------------------------*/
		errorstatus = SD_SelectDeselect(SDCardInfo.RCA);
	}

  if (errorstatus == SD_OK)
  {
    errorstatus = SD_EnableWideBusOperation(SDIO_BusWide_4b);
  }

  return(errorstatus);
}


/**
  * @brief  Enquires cards about their operating voltage and configures 
  *   clock controls.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_PowerON(void)
{
	int i;
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;
	uint32_t response = 0, count = 0, validvoltage = 0;
//	uint32_t SDType = SD_STD_CAPACITY;

	/*!< Power ON Sequence -----------------------------------------------------*/
	/*!< Configure the SDIO peripheral */
	sdio_controller_config(SDIO_INIT_CLK_DIV, SDIO_ClockEdge_Rising, SDIO_BusWide_1b, SDIO_ClockPowerSave_Disable);

	/*!< CMD0: GO_IDLE_STATE ---------------------------------------------------*/
	/*!< No CMD response required */
	for(i=0; i<74; i++)
	{
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);

		errorstatus = CmdError();
		if(errorstatus == SD_OK)
			break;
	}
	if (errorstatus != SD_OK)
	{
		/*!< CMD Response TimeOut (wait for CMDSENT flag) */
		return(errorstatus);
	}
	delay_ms(2);

	/*!< CMD8: SEND_IF_COND ----------------------------------------------------*/
	/*!< Send CMD8 to verify SD card interface operating condition */
	/*!< Argument: - [31:12]: Reserved (shall be set to '0')
				   - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
                   - [7:0]: Check Pattern (recommended 0xAA) */
	/*!< CMD Response: R7 */
	SDIO_CmdInitStructure.SDIO_Argument = SD_CHECK_PATTERN;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SDIO_SEND_IF_COND;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp7Error();

	if (errorstatus == SD_OK)
	{
		SDCardInfo.CardType = SDIO_STD_CAPACITY_SD_CARD_V2_0; /*!< SD Card 2.0 */
//		SDType = SD_HIGH_CAPACITY;
	}

	/*!< CMD55 */
	SDIO_CmdInitStructure.SDIO_Argument = 0x00;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_APP_CMD);

	/*!< If errorstatus is Command TimeOut, it is a MMC card */
	/*!< If errorstatus is SD_OK it is a SD card: SD card 2.0 (voltage range mismatch)
		or SD card 1.x */
	if (errorstatus == SD_OK)
	{
		/*!< SD CARD */
		/*!< Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
		while ((!validvoltage) && (count < SD_MAX_VOLT_TRIAL))
		{
			delay_us(100);

			/*!< SEND CMD55 APP_CMD with RCA as 0 */
			SDIO_CmdInitStructure.SDIO_Argument = 0x00;
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);
			errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
			if (errorstatus != SD_OK)
			{
				return(errorstatus);
			}

			SDIO_CmdInitStructure.SDIO_Argument = 0x40FF0000;    //SD_VOLTAGE_WINDOW_SD | SDType;
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_OP_COND;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);
			errorstatus = CmdResp3Error();
			if (errorstatus != SD_OK)
			{
				return(errorstatus);
			}

			response = SDIO_GetResponse(SDIO_RESP1);
			validvoltage = (((response >> 31) == 1) ? 1 : 0);
			count++;
		}
		if (count >= SD_MAX_VOLT_TRIAL)
		{
			errorstatus = SD_INVALID_VOLTRANGE;
			return(errorstatus);
		}

		if (response &= SD_HIGH_CAPACITY)
		{
			SDCardInfo.CardType = SDIO_HIGH_CAPACITY_SD_CARD;
		}
	}/* MMC */
	else
	{
		sdio_clk_close();   //关闭时钟输出 (逻辑分析仪抓取读卡器通信波形发现是这么做的)
		delay_us(400);
		sdio_clk_open();    //延时一段时间后重新打开

		//MMC卡, 在识别出为MMC卡后, 最好再来个CMD0, 
		//否则以后的初始化在CMD线上可能会出现数据抖动, 造成初始化失败
		/*!< CMD0: GO_IDLE_STATE ---------------------------------------------------*/
		/*!< No CMD response required */
		for(i=0; i<74; i++)
		{
			SDIO_CmdInitStructure.SDIO_Argument = 0x0;
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_GO_IDLE_STATE;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_No;
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);

			errorstatus = CmdError();
			if(errorstatus == SD_OK)
				break;
		}
		if (errorstatus != SD_OK)
		{
			/*!< CMD Response TimeOut (wait for CMDSENT flag) */
			return(errorstatus);
		}

		//MMC卡,发送CMD1 SDIO_SEND_OP_COND,参数为:0x80FF8000 
		while((!validvoltage)&&(count<SD_MAX_VOLT_TRIAL))
		{
			delay_us(100);
			SDIO_CmdInitStructure.SDIO_Argument = 0x40FF0000;    //SD_VOLTAGE_WINDOW_MMC
			SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_OP_COND;
			SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;    //r3
			SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
			SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
			SDIO_SendCommand(&SDIO_CmdInitStructure);

			errorstatus=CmdResp3Error(); 					//等待R3响应   
 			if(errorstatus != SD_OK)
				return errorstatus;   	//响应错误  

			response= SDIO_GetResponse(SDIO_RESP1);  //SDIO->RESP1;  //得到响应
			validvoltage = (((response>>31)==1)?1:0);
			count++;
		}
		if(count >= SD_MAX_VOLT_TRIAL)
		{
			errorstatus=SD_INVALID_VOLTRANGE;
			return errorstatus;
		}	 			    
		SDCardInfo.CardType = SDIO_MULTIMEDIA_CARD;
		delay_ms(2);
  	}

	return(errorstatus);
}

/**
  * @brief  Turns the SDIO output signals off.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_PowerOFF(void)
{
  SD_Error errorstatus = SD_OK;

  /*!< Set Power State to OFF */
  SDIO_SetPowerState(SDIO_PowerState_OFF);

  return(errorstatus);
}

/**
  * @brief  Intialises all cards or single card as the case may be Card(s) come 
  *         into standby state.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_InitializeCards(void)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;
	uint32_t rca = 0x100;

	if (SDIO_GetPowerState() == SDIO_PowerState_OFF)
	{
		errorstatus = SD_REQUEST_NOT_APPLICABLE;
		return(errorstatus);
	}

	if (SDIO_SECURE_DIGITAL_IO_CARD != SDCardInfo.CardType)
	{
		/*!< Send CMD2 ALL_SEND_CID */
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ALL_SEND_CID;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp2Error();

		if (SD_OK != errorstatus)
		{
			return(errorstatus);
		}

		SDCardInfo.CID_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
		SDCardInfo.CID_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
		SDCardInfo.CID_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
		SDCardInfo.CID_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
	}

	if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == SDCardInfo.CardType) ||  (SDIO_STD_CAPACITY_SD_CARD_V2_0 == SDCardInfo.CardType) || \
		(SDIO_SECURE_DIGITAL_IO_COMBO_CARD == SDCardInfo.CardType) ||  (SDIO_HIGH_CAPACITY_SD_CARD == SDCardInfo.CardType))
	{
		/*!< Send CMD3 SET_REL_ADDR with argument 0 */
		/*!< SD Card publishes its RCA. */
		SDIO_CmdInitStructure.SDIO_Argument = 0x00;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp6Error(SD_CMD_SET_REL_ADDR, &rca);

		if (SD_OK != errorstatus)
		{
			return(errorstatus);
		}
	}
	if(SDIO_MULTIMEDIA_CARD == SDCardInfo.CardType)
	{
		SDIO_CmdInitStructure.SDIO_Argument = rca;                  //发送CMD3,短响应 
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_REL_ADDR;	//cmd3
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);	//发送CMD3,短响应 	
	
		errorstatus=CmdResp2Error(); 			    //等待R2响应   
		if(errorstatus != SD_OK)
		{
			return errorstatus;   	//响应错误
		}			
	}

	if (SDIO_SECURE_DIGITAL_IO_CARD != SDCardInfo.CardType)
	{
		SDCardInfo.RCA = rca&0xFFFF0000;

		/*!< Send CMD9 SEND_CSD with argument as card's RCA */
		SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_CSD;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Long;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp2Error();

		if (SD_OK != errorstatus)
		{
			return(errorstatus);
		}

		SDCardInfo.CSD_Tab[0] = SDIO_GetResponse(SDIO_RESP1);
		SDCardInfo.CSD_Tab[1] = SDIO_GetResponse(SDIO_RESP2);
		SDCardInfo.CSD_Tab[2] = SDIO_GetResponse(SDIO_RESP3);
		SDCardInfo.CSD_Tab[3] = SDIO_GetResponse(SDIO_RESP4);
	}

	errorstatus = SD_OK; /*!< All cards get intialized */
	return(errorstatus);
}


/**
  * @brief  Returns information about specific card.
  * @param  cardinfo: pointer to a SD_CardInfo structure that contains all SD card 
  *         information.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo, SD_CSD *SD_csd, SD_CID *SD_cid)
{
	SD_Error errorstatus = SD_OK;
	uint8_t tmp = 0;

	/*!< Byte 0 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[0] & 0xFF000000) >> 24);
	SD_csd->CSDStruct = (tmp & 0xC0) >> 6;
	SD_csd->SysSpecVersion = (tmp & 0x3C) >> 2;
	SD_csd->Reserved1 = tmp & 0x03;

	/*!< Byte 1 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[0] & 0x00FF0000) >> 16);
	SD_csd->TAAC = tmp;

	/*!< Byte 2 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[0] & 0x0000FF00) >> 8);
	SD_csd->NSAC = tmp;

	/*!< Byte 3 */
	tmp = (uint8_t)(cardinfo->CSD_Tab[0] & 0x000000FF);
	SD_csd->MaxBusClkFrec = tmp;

	/*!< Byte 4 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[1] & 0xFF000000) >> 24);
	SD_csd->CardComdClasses = tmp << 4;

	/*!< Byte 5 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[1] & 0x00FF0000) >> 16);
	SD_csd->CardComdClasses |= (tmp & 0xF0) >> 4;
	SD_csd->RdBlockLen = tmp & 0x0F;

	/*!< Byte 6 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[1] & 0x0000FF00) >> 8);
	SD_csd->PartBlockRead = (tmp & 0x80) >> 7;
	SD_csd->WrBlockMisalign = (tmp & 0x40) >> 6;
	SD_csd->RdBlockMisalign = (tmp & 0x20) >> 5;
	SD_csd->DSRImpl = (tmp & 0x10) >> 4;
	SD_csd->Reserved2 = 0; /*!< Reserved */

	if ((cardinfo->CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1) || (cardinfo->CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0) || (cardinfo->CardType == SDIO_MULTIMEDIA_CARD))
	{
		SD_csd->DeviceSize = (tmp & 0x03) << 10;

		/*!< Byte 7 */
		tmp = (uint8_t)(cardinfo->CSD_Tab[1] & 0x000000FF);
		SD_csd->DeviceSize |= (tmp) << 2;

		/*!< Byte 8 */
		tmp = (uint8_t)((cardinfo->CSD_Tab[2] & 0xFF000000) >> 24);
		SD_csd->DeviceSize |= (tmp & 0xC0) >> 6;

		SD_csd->MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
		SD_csd->MaxRdCurrentVDDMax = (tmp & 0x07);

		/*!< Byte 9 */
		tmp = (uint8_t)((cardinfo->CSD_Tab[2] & 0x00FF0000) >> 16);
		SD_csd->MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
		SD_csd->MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
		SD_csd->DeviceSizeMul = (tmp & 0x03) << 1;
		/*!< Byte 10 */
		tmp = (uint8_t)((cardinfo->CSD_Tab[2] & 0x0000FF00) >> 8);
		SD_csd->DeviceSizeMul |= (tmp & 0x80) >> 7;
    
		cardinfo->CardCapacity = (SD_csd->DeviceSize + 1) ;
		cardinfo->CardCapacity *= (1 << (SD_csd->DeviceSizeMul + 2));
		cardinfo->CardBlockSize = 1 << (SD_csd->RdBlockLen);
		cardinfo->CardCapacity *= cardinfo->CardBlockSize;
	}
	else if (cardinfo->CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		/*!< Byte 7 */
		tmp = (uint8_t)(cardinfo->CSD_Tab[1] & 0x000000FF);
		SD_csd->DeviceSize = (tmp & 0x3F) << 16;

		/*!< Byte 8 */
		tmp = (uint8_t)((cardinfo->CSD_Tab[2] & 0xFF000000) >> 24);

		SD_csd->DeviceSize |= (tmp << 8);

		/*!< Byte 9 */
		tmp = (uint8_t)((cardinfo->CSD_Tab[2] & 0x00FF0000) >> 16);

		SD_csd->DeviceSize |= (tmp);

		/*!< Byte 10 */
		tmp = (uint8_t)((cardinfo->CSD_Tab[2] & 0x0000FF00) >> 8);
    
		cardinfo->CardCapacity = (uint64_t)(SD_csd->DeviceSize + 1) * 512 * 1024;
		cardinfo->CardBlockSize = 512;    
	}


	SD_csd->EraseGrSize = (tmp & 0x40) >> 6;
	SD_csd->EraseGrMul = (tmp & 0x3F) << 1;

	/*!< Byte 11 */
	tmp = (uint8_t)(cardinfo->CSD_Tab[2] & 0x000000FF);
	SD_csd->EraseGrMul |= (tmp & 0x80) >> 7;
	SD_csd->WrProtectGrSize = (tmp & 0x7F);

	/*!< Byte 12 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[3] & 0xFF000000) >> 24);
	SD_csd->WrProtectGrEnable = (tmp & 0x80) >> 7;
	SD_csd->ManDeflECC = (tmp & 0x60) >> 5;
	SD_csd->WrSpeedFact = (tmp & 0x1C) >> 2;
	SD_csd->MaxWrBlockLen = (tmp & 0x03) << 2;

	/*!< Byte 13 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[3] & 0x00FF0000) >> 16);
	SD_csd->MaxWrBlockLen |= (tmp & 0xC0) >> 6;
	SD_csd->WriteBlockPaPartial = (tmp & 0x20) >> 5;
	SD_csd->Reserved3 = 0;
	SD_csd->ContentProtectAppli = (tmp & 0x01);

	/*!< Byte 14 */
	tmp = (uint8_t)((cardinfo->CSD_Tab[3] & 0x0000FF00) >> 8);
	SD_csd->FileFormatGrouop = (tmp & 0x80) >> 7;
	SD_csd->CopyFlag = (tmp & 0x40) >> 6;
	SD_csd->PermWrProtect = (tmp & 0x20) >> 5;
	SD_csd->TempWrProtect = (tmp & 0x10) >> 4;
	SD_csd->FileFormat = (tmp & 0x0C) >> 2;
	SD_csd->ECC = (tmp & 0x03);

	/*!< Byte 15 */
	tmp = (uint8_t)(cardinfo->CSD_Tab[3] & 0x000000FF);
	SD_csd->CSD_CRC = (tmp & 0xFE) >> 1;
	SD_csd->Reserved4 = 1;


	/*!< Byte 0 */
	tmp = (uint8_t)((cardinfo->CID_Tab[0] & 0xFF000000) >> 24);
	SD_cid->ManufacturerID = tmp;

	/*!< Byte 1 */
	tmp = (uint8_t)((cardinfo->CID_Tab[0] & 0x00FF0000) >> 16);
	SD_cid->OEM_AppliID = tmp << 8;

	/*!< Byte 2 */
	tmp = (uint8_t)((cardinfo->CID_Tab[0] & 0x000000FF00) >> 8);
	SD_cid->OEM_AppliID |= tmp;

	/*!< Byte 3 */
	tmp = (uint8_t)(cardinfo->CID_Tab[0] & 0x000000FF);
	SD_cid->ProdName1 = tmp << 24;

	/*!< Byte 4 */
	tmp = (uint8_t)((cardinfo->CID_Tab[1] & 0xFF000000) >> 24);
	SD_cid->ProdName1 |= tmp << 16;

	/*!< Byte 5 */
	tmp = (uint8_t)((cardinfo->CID_Tab[1] & 0x00FF0000) >> 16);
	SD_cid->ProdName1 |= tmp << 8;

	/*!< Byte 6 */
	tmp = (uint8_t)((cardinfo->CID_Tab[1] & 0x0000FF00) >> 8);
	SD_cid->ProdName1 |= tmp;

	/*!< Byte 7 */
	tmp = (uint8_t)(cardinfo->CID_Tab[1] & 0x000000FF);
	SD_cid->ProdName2 = tmp;

	/*!< Byte 8 */
	tmp = (uint8_t)((cardinfo->CID_Tab[2] & 0xFF000000) >> 24);
	SD_cid->ProdRev = tmp;

	/*!< Byte 9 */
	tmp = (uint8_t)((cardinfo->CID_Tab[2] & 0x00FF0000) >> 16);
	SD_cid->ProdSN = tmp << 24;

	/*!< Byte 10 */
	tmp = (uint8_t)((cardinfo->CID_Tab[2] & 0x0000FF00) >> 8);
	SD_cid->ProdSN |= tmp << 16;

	/*!< Byte 11 */
	tmp = (uint8_t)(cardinfo->CID_Tab[2] & 0x000000FF);
	SD_cid->ProdSN |= tmp << 8;

	/*!< Byte 12 */
	tmp = (uint8_t)((cardinfo->CID_Tab[3] & 0xFF000000) >> 24);
	SD_cid->ProdSN |= tmp;

	/*!< Byte 13 */
	tmp = (uint8_t)((cardinfo->CID_Tab[3] & 0x00FF0000) >> 16);
	SD_cid->Reserved1 |= (tmp & 0xF0) >> 4;
	SD_cid->ManufactDate = (tmp & 0x0F) << 8;

	/*!< Byte 14 */
	tmp = (uint8_t)((cardinfo->CID_Tab[3] & 0x0000FF00) >> 8);
	SD_cid->ManufactDate |= tmp;

	/*!< Byte 15 */
	tmp = (uint8_t)(cardinfo->CID_Tab[3] & 0x000000FF);
	SD_cid->CID_CRC = (tmp & 0xFE) >> 1;
	SD_cid->Reserved2 = 1;
  
  return(errorstatus);
}

void SD_GetSCRInfo(SD_CardInfo *cardinfo, SD_SCR *SD_scr)
{
	uint8_t tmp = 0;

	/*!< Byte 0 */
	tmp = (uint8_t)(cardinfo->SCR_Tab[0] & 0x000000FF);
	SD_scr->SCRStruct = (tmp & 0xF0)>>4;
	SD_scr->SDSpec = tmp & 0x0F;

	/*!< Byte 1 */
	tmp = (uint8_t)((cardinfo->SCR_Tab[0] & 0x0000FF00) >> 8);
	SD_scr->DataStaAfterErase  = (tmp & 0x80)>>7;
	SD_scr->SDSecurity = (tmp & 0x70)>>4;
	SD_scr->DataBusWidth = tmp & 0x0F;
}

/**
  * @brief  Returns the current SD card's status.
  * @param  psdstatus: pointer to the buffer that will contain the SD card status 
  *         (SD Status register).
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SendSDStatus(uint32_t *psdstatus)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
	SDIO_DataInitTypeDef SDIO_DataInitStructure;
	volatile int dummy; 
	int index = 0;
  SD_Error errorstatus = SD_OK;
//  uint32_t count = 0;
  
  if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
  {
    errorstatus = SD_LOCK_UNLOCK_FAILED;
    return(errorstatus);
  }

  /*!< Set block size for card if it is not equal to current block size for card. */
//  SDIO_CmdInitStructure.SDIO_Argument = 64;
//  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
//  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//  SDIO_SendCommand(&SDIO_CmdInitStructure);
//  
//  errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
//  if (errorstatus != SD_OK)
//  {
//    return(errorstatus);
//  }

  /*!< CMD55 */
  SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  errorstatus = CmdResp1Error(SD_CMD_APP_CMD);

  if (errorstatus != SD_OK)
  {
    return(errorstatus);
  }

  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = 64;
  SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_64b;
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);

  /*!< Send ACMD13 SD_APP_STAUS  with argument as card's RCA.*/
   SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;  
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_STAUS;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  errorstatus = CmdResp1Error(SD_CMD_SD_APP_STAUS);

  if (errorstatus != SD_OK)
  {
    return(errorstatus);
  }

  while (!(SDIO->STA &(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
  {
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
    {
	  if(index < 16)
      {
		*(psdstatus + index) = SDIO_ReadData();
		index++;
      }
	  else
	  {
		  dummy = SDIO_ReadData();
	  }
    }
  }

  if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
    errorstatus = SD_DATA_TIMEOUT;
    return(errorstatus);
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
    errorstatus = SD_DATA_CRC_FAIL;
    return(errorstatus);
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
    errorstatus = SD_RX_OVERRUN;
    return(errorstatus);
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_STBITERR);
    errorstatus = SD_START_BIT_ERR;
    return(errorstatus);
  }

  while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)
  {
	  if(index < 16)
      {
		*(psdstatus + index) = SDIO_ReadData();
		index++;
      }
	  else
	  {
		  dummy = SDIO_ReadData();
	  }
  }

  /*!< Clear all the static status flags*/
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  return(errorstatus);
}

//Read SSR
SD_Error SD_GetCardStatus(SD_SSR *cardstatus)
{
	uint32_t SSR[16];  //数组的开始地址4字节对齐
  SD_Error errorstatus = SD_OK;
  uint8_t tmp = 0;

  errorstatus = SD_SendSDStatus((uint32_t *)SSR);
  if (errorstatus  != SD_OK)
  {
    return(errorstatus);
  }

  uint8_t *SDSTATUS_Tab = (uint8_t *)SSR;
  
  /*!< Byte 0 */
  tmp = (uint8_t)((SDSTATUS_Tab[0] & 0xC0) >> 6);
  cardstatus->DAT_BUS_WIDTH = tmp;

  /*!< Byte 0 */
  tmp = (uint8_t)((SDSTATUS_Tab[0] & 0x20) >> 5);
  cardstatus->SECURED_MODE = tmp;

  /*!< Byte 2 */
  tmp = (uint8_t)((SDSTATUS_Tab[2] & 0xFF));
  cardstatus->SD_CARD_TYPE = tmp << 8;

  /*!< Byte 3 */
  tmp = (uint8_t)((SDSTATUS_Tab[3] & 0xFF));
  cardstatus->SD_CARD_TYPE |= tmp;

  /*!< Byte 4 */
  tmp = (uint8_t)(SDSTATUS_Tab[4] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA = tmp << 24;

  /*!< Byte 5 */
  tmp = (uint8_t)(SDSTATUS_Tab[5] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp << 16;

  /*!< Byte 6 */
  tmp = (uint8_t)(SDSTATUS_Tab[6] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp << 8;

  /*!< Byte 7 */
  tmp = (uint8_t)(SDSTATUS_Tab[7] & 0xFF);
  cardstatus->SIZE_OF_PROTECTED_AREA |= tmp;

  /*!< Byte 8 */
  tmp = (uint8_t)((SDSTATUS_Tab[8] & 0xFF));
  cardstatus->SPEED_CLASS = tmp;

  /*!< Byte 9 */
  tmp = (uint8_t)((SDSTATUS_Tab[9] & 0xFF));
  cardstatus->PERFORMANCE_MOVE = tmp;

  /*!< Byte 10 */
  tmp = (uint8_t)((SDSTATUS_Tab[10] & 0xF0) >> 4);
  cardstatus->AU_SIZE = tmp;

  /*!< Byte 11 */
  tmp = (uint8_t)(SDSTATUS_Tab[11] & 0xFF);
  cardstatus->ERASE_SIZE = tmp << 8;

  /*!< Byte 12 */
  tmp = (uint8_t)(SDSTATUS_Tab[12] & 0xFF);
  cardstatus->ERASE_SIZE |= tmp;

  /*!< Byte 13 */
  tmp = (uint8_t)((SDSTATUS_Tab[13] & 0xFC) >> 2);
  cardstatus->ERASE_TIMEOUT = tmp;

  /*!< Byte 13 */
  tmp = (uint8_t)((SDSTATUS_Tab[13] & 0x3));
  cardstatus->ERASE_OFFSET = tmp;
 
  return(errorstatus);
}


/**
  * @brief  Enables wide bus opeartion for the requeseted card if supported by 
  *         card.
  * @param  WideMode: Specifies the SD card wide bus mode. 
  *   This parameter can be one of the following values:
  *     @arg SDIO_BusWide_8b: 8-bit data transfer (Only for MMC)
  *     @arg SDIO_BusWide_4b: 4-bit data transfer
  *     @arg SDIO_BusWide_1b: 1-bit data transfer
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_EnableWideBusOperation(uint32_t WideMode)   //Abandon this function
{
  SD_Error errorstatus = SD_OK;

  /*!< MMC Card doesn't support this feature */
  if (SDIO_MULTIMEDIA_CARD == SDCardInfo.CardType)
  {
    errorstatus = SD_UNSUPPORTED_FEATURE;
    return(errorstatus);
  }
  else if ((SDIO_STD_CAPACITY_SD_CARD_V1_1 == SDCardInfo.CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == SDCardInfo.CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == SDCardInfo.CardType))
  {
    if (SDIO_BusWide_8b == WideMode)
    {
      errorstatus = SD_UNSUPPORTED_FEATURE;
      return(errorstatus);
    }
    else if (SDIO_BusWide_4b == WideMode)
    {
      errorstatus = SDEnWideBus(ENABLE);

      if (SD_OK == errorstatus)
      {
        /*!< Configure the SDIO peripheral */
		sdio_controller_config(SDIO_TRANSFER_CLK_DIV, SDIO_ClockEdge_Rising, SDIO_BusWide_4b, SDIO_ClockPowerSave_Disable);
      }
    }
    else
    {
      errorstatus = SDEnWideBus(DISABLE);

      if (SD_OK == errorstatus)
      {
        /*!< Configure the SDIO peripheral */
		sdio_controller_config(SDIO_TRANSFER_CLK_DIV, SDIO_ClockEdge_Rising, SDIO_BusWide_1b, SDIO_ClockPowerSave_Disable);
      }
    }
  }

  return(errorstatus);
}

/**
  * @brief  Selects od Deselects the corresponding card.
  * @param  addr: Address of the Card to be selected.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SelectDeselect(uint32_t addr)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SD_Error errorstatus = SD_OK;

  /*!< Send CMD7 SDIO_SEL_DESEL_CARD */
  SDIO_CmdInitStructure.SDIO_Argument =  addr;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEL_DESEL_CARD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);
  
  errorstatus = CmdResp1Error(SD_CMD_SEL_DESEL_CARD);

  return(errorstatus);
}


//===================================================================================
/**
  * @brief  Returns the current card's status.
  * @param  pcardstatus: pointer to the buffer that will contain the SD card 
  *         status (Card Status register).
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_SendStatus(uint32_t *pcardstatus)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;

	if (pcardstatus == NULL)
	{
		errorstatus = SD_INVALID_PARAMETER;
		return(errorstatus);
	}

	SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_SEND_STATUS);
	if (errorstatus != SD_OK)
	{
		return(errorstatus);
	}

	*pcardstatus = SDIO_GetResponse(SDIO_RESP1);
	return(errorstatus);
}


/**
  * @brief  Gets the cuurent sd card data transfer status.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be: 
  *        - SD_TRANSFER_OK: No data transfer is acting
  *        - SD_TRANSFER_BUSY: Data transfer is acting
  */
SDTransferState SD_GetStatus(void)
{
	uint32_t cardstate =  SD_CARD_TRANSFER;
	
	if (SD_SendStatus(&cardstate) != SD_OK)
    {
      return SD_TRANSFER_ERROR;
    }

	cardstate = (cardstate>>9) & 0x0F;
  
	if (cardstate == SD_CARD_TRANSFER)
	{
		return(SD_TRANSFER_OK);
	}
	else if(cardstate == SD_CARD_ERROR)
	{
		return (SD_TRANSFER_ERROR);
	}
	else
	{
		return(SD_TRANSFER_BUSY);
	}
}

/** SDIO多块传输, 传输结束或者传输出错要结束传输时一定要发送SD_CMD_STOP_TRANSMISSION命令,
 ** 否则会出现以下问题：
 ** (1)主机在读取指定块(包)后, 设备会继续向主机发送数据块, 直到达到一定的数据量, 这会占据总线;
 ** (2)再次操作设备时, 设备对命令无响应.
 **/
static SD_Error SD_StopMultiBlocksTrans(void)
{
	SDIO_CmdInitTypeDef  SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;
	
	/*!< In Case Of SD-CARD Send Command STOP_TRANSMISSION */
	if (((SDIO_MULTIMEDIA_CARD == SDCardInfo.CardType)) || (SDIO_STD_CAPACITY_SD_CARD_V1_1 == SDCardInfo.CardType) ||\
		(SDIO_HIGH_CAPACITY_SD_CARD == SDCardInfo.CardType) || (SDIO_STD_CAPACITY_SD_CARD_V2_0 == SDCardInfo.CardType))
	{
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);    //Clear all the static flags

		/*!< Send CMD12 STOP_TRANSMISSION */
		SDIO_CmdInitStructure.SDIO_Argument = 0x0;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);

		errorstatus = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);
		if (errorstatus != SD_OK)
		{
			return(errorstatus);
		}
	}
	
	return errorstatus;
}

//unit 100us
#define  TRY_COUNT  10000
#define  TRY_INTVL  50

#define  STOP_TRY   10

/**
  * @brief  Allows to read one block from a specified address in a card. The Data
  *         transfer can be managed by DMA mode or Polling mode. 
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the 
  *            data transfer and it is ready for data.            
  * @param  readbuff: pointer to the buffer that will contain the received data
  * @param  ReadAddr: Address from where data are to be read.  
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_ReadBlock(uint8_t *readbuff, uint64_t ReadAddr, uint16_t BlockSize)
{
	SDIO_DataInitTypeDef SDIO_DataInitStructure;
	SDIO_CmdInitTypeDef  SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;
	uint32_t power = 0;
	uint32_t status, try_cnt;
	int current_state;  //ready_for_data;

#if defined (SD_POLLING_MODE) 
//	uint32_t count = 0;
	uint32_t *tempbuff = (uint32_t *)readbuff;
#endif

	if (NULL == readbuff)
	{
		errorstatus = SD_INVALID_PARAMETER;
		return(errorstatus);
	}

	SDIO->DCTRL = 0x0;      //数据控制寄存器清零(关DMA)

	if (SDCardInfo.CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		ReadAddr /= 512;
	}

	/*!< Clear all DPSM configuration */
//	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
//	SDIO_DataInitStructure.SDIO_DataLength = 0;
//	SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
//	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
//	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
//	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;  //SDIO_DPSM_Enable
//	SDIO_DataConfig(&SDIO_DataInitStructure);
//  if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
//	{
//		errorstatus = SD_LOCK_UNLOCK_FAILED;
//		return(errorstatus);
//	}

	try_cnt = 0;
	while(try_cnt < TRY_COUNT)
	{
		errorstatus = SD_SendStatus(&status);
		if(errorstatus != SD_OK) {
			return errorstatus;
		}
		current_state = (status>>9)&0x0F;
		if(current_state!=SD_CARD_SENDING && current_state!=SD_CARD_RECEIVING && current_state!=SD_CARD_PROGRAMMING) {
			break;
		}

		if((try_cnt%TRY_INTVL) == 0) {
			if(current_state==SD_CARD_SENDING || current_state==SD_CARD_RECEIVING) {
				SD_StopMultiBlocksTrans();
				SDIO_ClearFlag(SDIO_STATIC_FLAGS);
			}
			my_printf("[%s] cur_state=%u\n\r", __FUNCTION__, current_state);
		}

		try_cnt++;
		delay_us(100);
	}
	if(try_cnt == TRY_COUNT)
	{
		errorstatus = SD_ERROR;  //SD Card not ready
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);
		return errorstatus;		
	}
	if(try_cnt >= TRY_INTVL)    //debug code
	{
		my_printf("[%s] try_cnt=%u\n\r", __FUNCTION__, try_cnt);
	}

	if ((BlockSize > 0) && (BlockSize <= 2048) && ((BlockSize & (BlockSize - 1)) == 0))
	{
		power = convert_from_bytes_to_power_of_two(BlockSize);

		/*!< Set Block Size for Card */
//		SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) BlockSize;
//		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
//		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//		SDIO_SendCommand(&SDIO_CmdInitStructure);
//		errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
//		if (SD_OK != errorstatus)
//		{
//			return(errorstatus);
//		}
	}
	else
	{
		errorstatus = SD_INVALID_PARAMETER;
		return(errorstatus);
	}

	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = BlockSize;
	SDIO_DataInitStructure.SDIO_DataBlockSize = (uint32_t)power << 4;
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);

#if defined (SD_DMA_MODE)	/* 提前初始化DMA */
	sdio_dma_config(readbuff, BlockSize, DMA_DIR_PeripheralToMemory);
    SDIO_DMACmd(ENABLE);  //开启器SDIO DMA请求
#endif

	/*!< Send CMD17 READ_SINGLE_BLOCK */
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)ReadAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_SINGLE_BLOCK;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_READ_SINGLE_BLOCK);
	if (errorstatus != SD_OK)
	{
		return(errorstatus);
	}

#if defined (SD_POLLING_MODE) 
	ALLOC_CPU_SR();             //不能被打断, 否则FIFO会溢出, 即出现SDIO_FLAG_RXOVERR
	ENTER_CRITICAL();

	/*!< In case of single block transfer, no need of stop transfer at all.*/
	/*!< Polling mode */
	while (!(SDIO->STA &(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DATAEND | SDIO_FLAG_STBITERR)))
	{
		if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
		{
//			for (count = 0; count < 8; count++)
//			{
//				*(tempbuff + count) = SDIO_ReadData();
//			}
			tempbuff[0] = SDIO->FIFO;  //向量加速, 运行速度远大于for循环
			tempbuff[1] = SDIO->FIFO;
			tempbuff[2] = SDIO->FIFO;
			tempbuff[3] = SDIO->FIFO;
			tempbuff[4] = SDIO->FIFO;
			tempbuff[5] = SDIO->FIFO;
			tempbuff[6] = SDIO->FIFO;
			tempbuff[7] = SDIO->FIFO;
			tempbuff += 8;
		}
	}
	EXIT_CRITICAL();

	if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
		errorstatus = SD_DATA_TIMEOUT;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
		errorstatus = SD_DATA_CRC_FAIL;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
		errorstatus = SD_RX_OVERRUN;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);
		errorstatus = SD_START_BIT_ERR;
		return(errorstatus);
	}
	while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)        //不会执行到, 上个while循环中就能拾取所有数据
	{
//		my_printf("[%s] SDIO FIFO leave data\n\r", __FUNCTION__);
		*tempbuff = SDIO_ReadData();
		tempbuff++;
	}

#elif defined (SD_DMA_MODE)

	while(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TCIF6) == 0)
//	while(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) == 0)           //while(SDIO->DCOUNT != 0)
	{	//等待传输完成
		if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != 0)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;			
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != 0)  //数据超时
			errorstatus = SD_DATA_TIMEOUT;
		else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != 0)   //接收FIFO上溢错误(FIFO写满,数据溢出)
			errorstatus = SD_RX_OVERRUN;
//		else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != 0)  //在宽总线模式, 在所有数据信号上都没有检测到起始位
//			errorstatus = SD_START_BIT_ERR;              //SDIO使用DMA传输时易出现SD_START_BIT_ERR, 但传输数据仍然正确

		else if(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TEIF6) != 0)
			errorstatus = SD_CC_ERROR;
		
		//SDIO DMA传输出错
		if(errorstatus != SD_OK)
		{
			DMA_DeInit(DMA2_Stream6);
			break;
		}
	}

	if(errorstatus == SD_OK)
	{	//接收时, DMA传输完成 != SDIO接收完成(SDIO_FLAG_DATAEND)[=8字节CRC校验接收完成(SDIO_FLAG_DBCKEND)+4bit高电平(包结束标志)]
		while(!(SDIO->STA &(SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DATAEND)))  //等待数据传输完成, 包括CRC校验字节
		{
			asm("nop");
		}

//		if(SDIO_GetFlagStatus(SDIO_FLAG_DBCKEND) == 0)
//		{	//不可能发生
//			my_printf("[%s] Error! SDIO_FLAG_DBCKEND miss!!\n\r", __FUNCTION__);		
//		}

		if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != 0)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;			
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != 0)  //数据超时
			errorstatus = SD_DATA_TIMEOUT;
	}

	DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6|DMA_FLAG_HTIF6|DMA_FLAG_TEIF6);

	SDIO_DMACmd(DISABLE);
//	SDIO_ITConfig(SDIO_IT_STBITERR, DISABLE);

//	if(errorstatus != SD_OK)
//		return errorstatus;
#endif
	
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);

	return(errorstatus);
}

/**
  * @brief  Allows to read blocks from a specified address  in a card.  The Data
  *         transfer can be managed by DMA mode or Polling mode. 
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the 
  *            data transfer and it is ready for data.   
  * @param  readbuff: pointer to the buffer that will contain the received data.
  * @param  ReadAddr: Address from where data are to be read.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be read.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_ReadMultiBlocks(uint8_t *readbuff, uint64_t ReadAddr, uint16_t BlockSize, uint32_t NumberOfBlocks)
{
	SDIO_DataInitTypeDef SDIO_DataInitStructure;
	SDIO_CmdInitTypeDef  SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;
	uint32_t status, try_cnt;
	int current_state;  //ready_for_data;

#if defined (SD_POLLING_MODE) 
//	uint32_t count = 0;
	uint32_t *tempbuff = (uint32_t *)readbuff;
#endif
	
	SDIO->DCTRL = 0x0;

	if (SDCardInfo.CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		ReadAddr /= 512;
	}

	/*!< Clear all DPSM configuration */
//	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
//	SDIO_DataInitStructure.SDIO_DataLength = 0;
//	SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
//	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
//	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
//	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
//	SDIO_DataConfig(&SDIO_DataInitStructure);
//  if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
//	{
//		errorstatus = SD_LOCK_UNLOCK_FAILED;
//		return(errorstatus);
//	}

	try_cnt = 0;
	while(try_cnt < TRY_COUNT)
	{
		errorstatus = SD_SendStatus(&status);
		if(errorstatus != SD_OK) {
			return errorstatus;
		}
		current_state = (status>>9)&0x0F;
		if(current_state!=SD_CARD_SENDING && current_state!=SD_CARD_RECEIVING && current_state!=SD_CARD_PROGRAMMING) {
			break;
		}

		if((try_cnt%TRY_INTVL) == 0) {
			if(current_state==SD_CARD_SENDING || current_state==SD_CARD_RECEIVING) {
				SD_StopMultiBlocksTrans();
				SDIO_ClearFlag(SDIO_STATIC_FLAGS);
			}
			my_printf("[%s] cur_state=%u\n\r", __FUNCTION__, current_state);
		}

		try_cnt++;
		delay_us(100);
	}
	if(try_cnt == TRY_COUNT)
	{
		errorstatus = SD_ERROR;  //SD Card not ready
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);
		return errorstatus;		
	}
	if(try_cnt >= TRY_INTVL)    //debug code
	{
		my_printf("[%s] try_cnt=%u\n\r", __FUNCTION__, try_cnt);
	}

	/*!< Set Block Size for Card */
//	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) BlockSize;   //若没有改变, 则不用每次都设置
//	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
//	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//	SDIO_SendCommand(&SDIO_CmdInitStructure);
//	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
//	if (SD_OK != errorstatus)
//	{
//		return(errorstatus);
//	}

	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = NumberOfBlocks * BlockSize;
	SDIO_DataInitStructure.SDIO_DataBlockSize = (uint32_t) 9 << 4;
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);

#if defined (SD_DMA_MODE)
	sdio_dma_config(readbuff, BlockSize*NumberOfBlocks, DMA_DIR_PeripheralToMemory);
	SDIO_DMACmd(ENABLE);  //开启器SDIO DMA请求
#endif

	/*!< Send CMD18 READ_MULT_BLOCK with argument data address */
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)ReadAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_READ_MULT_BLOCK;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_READ_MULT_BLOCK);
	if (errorstatus != SD_OK)
	{
		return(errorstatus);
	}

#if defined (SD_POLLING_MODE) 
	ALLOC_CPU_SR();             //不能被打断, 否则FIFO会溢出, 即出现SDIO_FLAG_RXOVERR
	ENTER_CRITICAL();

	/*!< Polling mode */
	while (!(SDIO->STA &(SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DATAEND | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_STBITERR)))
    {
        if (SDIO_GetFlagStatus(SDIO_FLAG_RXFIFOHF) != RESET)
        {
//          for (count = 0; count < SD_HALFFIFO; count++)
//          {
//            *(tempbuff + count) = SDIO_ReadData();
//          }
			tempbuff[0] = SDIO->FIFO;   //向量加速, 运行速度远大于for循环
			tempbuff[1] = SDIO->FIFO;
			tempbuff[2] = SDIO->FIFO;
			tempbuff[3] = SDIO->FIFO;
			tempbuff[4] = SDIO->FIFO;
			tempbuff[5] = SDIO->FIFO;
			tempbuff[6] = SDIO->FIFO;
			tempbuff[7] = SDIO->FIFO;
			tempbuff += SD_HALFFIFO;
        }
	}
	EXIT_CRITICAL();

	if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
		SD_StopMultiBlocksTrans();
        errorstatus = SD_DATA_TIMEOUT;
        return(errorstatus);
	}
    else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
		SD_StopMultiBlocksTrans();
        errorstatus = SD_DATA_CRC_FAIL;
        return(errorstatus);
    }
    else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
		SD_StopMultiBlocksTrans();
        errorstatus = SD_RX_OVERRUN;
        return(errorstatus);
    }
    else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
    {
        SDIO_ClearFlag(SDIO_FLAG_STBITERR);
		SD_StopMultiBlocksTrans();
        errorstatus = SD_START_BIT_ERR;
        return(errorstatus);
    }
    while (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)    //不会执行到, 上个while循环中就能拾取所有数据
    {
//		my_printf("[%s] SDIO FIFO leave data\n\r", __FUNCTION__);
        *tempbuff = SDIO_ReadData();
        tempbuff++;
    }

#elif defined (SD_DMA_MODE)

	while(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TCIF6) == 0)
	{	//等待传输完成
		if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != 0)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;			
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != 0)  //数据超时
			errorstatus = SD_DATA_TIMEOUT;
		else if(SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != 0)   //接收FIFO上溢错误(FIFO写满,数据溢出)
			errorstatus = SD_RX_OVERRUN;
//		else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != 0)  //在宽总线模式, 在所有数据信号上都没有检测到起始位
//			errorstatus = SD_START_BIT_ERR;              //SDIO使用DMA传输时易出现SD_START_BIT_ERR, 但传输数据仍然正确

		else if(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TEIF6) != 0)
			errorstatus = SD_CC_ERROR;
		
		//SDIO DMA传输出错
		if(errorstatus != SD_OK)
		{
			DMA_DeInit(DMA2_Stream6);
			break;
		}
	}
	
	if(errorstatus == SD_OK)
	{	//接收时, DMA传输完成 != SDIO接收完成(SDIO_FLAG_DATAEND)[=8字节CRC校验接收完成(SDIO_FLAG_DBCKEND)+4bit高电平(包结束标志)]
		while(!(SDIO->STA &(SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DATAEND)))  //等待数据传输完成, 包括CRC校验字节
		{
			asm("nop");
		}

//		if(SDIO_GetFlagStatus(SDIO_FLAG_DBCKEND) == 0)
//		{	//不可能发生
//			my_printf("[%s] Error! SDIO_FLAG_DBCKEND miss!!\n\r", __FUNCTION__);
//		}

		if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != 0)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;			
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != 0)  //数据超时
			errorstatus = SD_DATA_TIMEOUT;
	}

	DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6|DMA_FLAG_HTIF6|DMA_FLAG_TEIF6);

	SDIO_DMACmd(DISABLE);
//	SDIO_ITConfig(SDIO_IT_STBITERR, DISABLE);

	if(errorstatus != SD_OK)
	{
		SD_StopMultiBlocksTrans();
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);
		return errorstatus;
	}
#endif

	if (SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)
    {
		try_cnt = 0;
		do {
			errorstatus = SD_StopMultiBlocksTrans();
			if(errorstatus == SD_OK)
				break;
			try_cnt++;
		}while(try_cnt < STOP_TRY);
		if(try_cnt != 0) {
			my_printf("[%s] stop try_cnt=%u\n\r", __FUNCTION__, try_cnt);
		}

		if (errorstatus != SD_OK) {
			SDIO_ClearFlag(SDIO_STATIC_FLAGS);
			return(errorstatus);
		}
    }

    /*!< Clear all the static flags */
    SDIO_ClearFlag(SDIO_STATIC_FLAGS);

	return(errorstatus);
}

//unit 10us
#define  TIME_OUT  100000
/**
  * @brief  Allows to write one block starting from a specified address in a card.
  *         The Data transfer can be managed by DMA mode or Polling mode.
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the 
  *            data transfer and it is ready for data.      
  * @param  writebuff: pointer to the buffer that contain the data to be transferred.
  * @param  WriteAddr: Address from where data are to be read.   
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WriteBlock(uint8_t *writebuff, uint64_t WriteAddr, uint16_t BlockSize)
{
	SDIO_DataInitTypeDef SDIO_DataInitStructure;
	SDIO_CmdInitTypeDef  SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;
	uint32_t power = 0;
	uint32_t cardstate = 0;
	uint32_t status, try_cnt;
	int current_state;  //ready_for_data;

#if defined (SD_POLLING_MODE)
	uint32_t bytestransferred = 0, count = 0, restwords = 0;
	uint32_t *tempbuff = (uint32_t *)writebuff;
#endif
  
	SDIO->DCTRL = 0x0;

	if (SDCardInfo.CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		WriteAddr /= 512;
	}

	/*!< Clear all DPSM configuration */
//	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
//	SDIO_DataInitStructure.SDIO_DataLength = 0;
//	SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
//	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
//	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
//	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
//	SDIO_DataConfig(&SDIO_DataInitStructure);
//	if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
//	{
//		errorstatus = SD_LOCK_UNLOCK_FAILED;
//		return(errorstatus);
//	}

	try_cnt = 0;
	while(try_cnt < TRY_COUNT)
	{
		errorstatus = SD_SendStatus(&status);
		if(errorstatus != SD_OK) {
			return errorstatus;
		}
		current_state = (status>>9)&0x0F;
		if(current_state!=SD_CARD_SENDING && current_state!=SD_CARD_RECEIVING && current_state!=SD_CARD_PROGRAMMING) {
			break;
		}

		if((try_cnt%TRY_INTVL) == 0) {
			if(current_state==SD_CARD_SENDING || current_state==SD_CARD_RECEIVING) {
				SD_StopMultiBlocksTrans();
				SDIO_ClearFlag(SDIO_STATIC_FLAGS);
			}
			my_printf("[%s] cur_state=%u\n\r", __FUNCTION__, current_state);
		}

		try_cnt++;
		delay_us(100);
	}
	if(try_cnt == TRY_COUNT)
	{
		errorstatus = SD_ERROR;  //SD Card not ready
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);
		return errorstatus;		
	}
	if(try_cnt >= TRY_INTVL)    //debug code
	{
		my_printf("[%s] try_cnt=%u\n\r", __FUNCTION__, try_cnt);
	}

	/*!< Set the block size, both on controller and card */
	if ((BlockSize > 0) && (BlockSize <= 2048) && ((BlockSize & (BlockSize - 1)) == 0))
	{
		power = convert_from_bytes_to_power_of_two(BlockSize);

//		SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) BlockSize;
//		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
//		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//		SDIO_SendCommand(&SDIO_CmdInitStructure);
//		errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
//		if (errorstatus != SD_OK)
//		{
//		return(errorstatus);
//		}
	}
	else
	{
		errorstatus = SD_INVALID_PARAMETER;
		return(errorstatus);
	}
 
	/*!< Send CMD24 WRITE_SINGLE_BLOCK */
	SDIO_CmdInitStructure.SDIO_Argument = WriteAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_SINGLE_BLOCK;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_WRITE_SINGLE_BLOCK);
	if (errorstatus != SD_OK)
	{
		return(errorstatus);
	}

	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = BlockSize;
	SDIO_DataInitStructure.SDIO_DataBlockSize = (uint32_t)power << 4;
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);

	/*!< In case of single data block transfer no need of stop command at all */
#if defined (SD_POLLING_MODE)
	ALLOC_CPU_SR();             //不能被打断, 否则FIFO会空, 即出现SDIO_FLAG_TXUNDERR
	ENTER_CRITICAL();

	while (!(SDIO->STA & (SDIO_FLAG_DATAEND | SDIO_FLAG_TXUNDERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_STBITERR)))
	{
		if (SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)
		{
			if ((512 - bytestransferred) < 32)
			{
				restwords = ((512 - bytestransferred) % 4 == 0) ? ((512 - bytestransferred) / 4) : (( 512 -  bytestransferred) / 4 + 1);
				for (count = 0; count < restwords; count++, tempbuff++, bytestransferred += 4)
				{
					SDIO_WriteData(*tempbuff);
				}
			}
			else
			{
//				for (count = 0; count < 8; count++)
//				{
//					SDIO_WriteData(*(tempbuff + count));
//				}
				SDIO->FIFO = tempbuff[0];    //把for()拆成代码向量,加速运行
				SDIO->FIFO = tempbuff[1];
				SDIO->FIFO = tempbuff[2];
				SDIO->FIFO = tempbuff[3];
				SDIO->FIFO = tempbuff[4];
				SDIO->FIFO = tempbuff[5];
				SDIO->FIFO = tempbuff[6];
				SDIO->FIFO = tempbuff[7];
				tempbuff += 8;
				bytestransferred += 32;
			}
		}
	}
	EXIT_CRITICAL();
	
	if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
		errorstatus = SD_DATA_TIMEOUT;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
		errorstatus = SD_DATA_CRC_FAIL;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);
		errorstatus = SD_TX_UNDERRUN;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);
		errorstatus = SD_START_BIT_ERR;
		return(errorstatus);
	}
	
#elif defined (SD_DMA_MODE)
	sdio_dma_config(writebuff, BlockSize, DMA_DIR_MemoryToPeripheral);
    SDIO_DMACmd(ENABLE);  //开启器SDIO DMA请求

	while(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TCIF6) == 0)
	{	//等待传输完成(发送时, DMA传输完成 != SDIO发送完成)
		if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != 0)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;			
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != 0)  //数据超时
			errorstatus = SD_DATA_TIMEOUT;
		else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != 0)   //发送FIFO下溢错误(FIFO缺数据)
			errorstatus = SD_TX_UNDERRUN;
//		else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != 0)  //在宽总线模式, 在所有数据信号上都没有检测到起始位
//			errorstatus = SD_START_BIT_ERR;              //SDIO使用DMA传输时易出现SD_START_BIT_ERR, 但传输数据仍然正确

		else if(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TEIF6) != 0)
			errorstatus = SD_CC_ERROR;
		
		//SDIO DMA传输出错
		if(errorstatus != SD_OK)
		{
			DMA_DeInit(DMA2_Stream6);
			break;
		}
	}

	if(errorstatus == SD_OK)
	{	//发送时, DMA传输完成 != SDIO发送完成(SDIO_FLAG_DATAEND)[=8字节CRC校验接收完成(SDIO_FLAG_DBCKEND)+4bit高电平(包结束标志)]
		while(!(SDIO->STA & (SDIO_FLAG_DTIMEOUT|SDIO_FLAG_DCRCFAIL|SDIO_FLAG_DATAEND)))
		{
			asm("nop");
		}

//		if(SDIO_GetFlagStatus(SDIO_FLAG_DBCKEND) == 0)
//		{	//不可能发生
//			my_printf("[%s] Error! SDIO_FLAG_DBCKEND miss!!\n\r", __FUNCTION__);
//		}

		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) == SET)       //数据超时
			errorstatus = SD_DATA_TIMEOUT;
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) == SET)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;	
	}

	DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6|DMA_FLAG_HTIF6|DMA_FLAG_TEIF6);

	SDIO_DMACmd(DISABLE);
//	SDIO_ITConfig(SDIO_IT_STBITERR, DISABLE);

	if(errorstatus != SD_OK)
	{
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);
		return errorstatus;
	}
#endif

	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);
	
//	while(SD_GetStatus() != SD_TRANSFER_OK)
//	{
//		my_printf("SD_WR1_P\n\r");
//	}

	/*!< Wait till the card is in programming state */
	if(SDCardInfo.CardType == SDIO_MULTIMEDIA_CARD)
	{
		errorstatus = IsCardProgramming((uint8_t *)&cardstate);
		if((errorstatus==SD_OK) && ((cardstate==SD_CARD_PROGRAMMING) || (cardstate==SD_CARD_RECEIVING)))
		{
			try_cnt = 0;
			do {
				delay_us(10);
				cardstate =  sdio_dat0_read();
				try_cnt++;
			}while((cardstate==0) && (try_cnt<TIME_OUT));
		}
	}
	else  //SD_V1.1  SD_V2.0  SD_HC
	{
		try_cnt = 0;
		errorstatus = IsCardProgramming((uint8_t *)&cardstate);
		while((errorstatus==SD_OK) && ((cardstate==SD_CARD_PROGRAMMING)||(cardstate==SD_CARD_RECEIVING)) && (try_cnt<TIME_OUT))
		{
			try_cnt++;
			delay_us(10);
			errorstatus = IsCardProgramming((uint8_t *)&cardstate);
		}
	}

	return(errorstatus);
}

/**
  * @brief  Allows to write blocks starting from a specified address in a card.
  *         The Data transfer can be managed by DMA mode only. 
  * @note   This operation should be followed by two functions to check if the 
  *         DMA Controller and SD Card status.
  *          - SD_ReadWaitOperation(): this function insure that the DMA
  *            controller has finished all data transfer.
  *          - SD_GetStatus(): to check that the SD Card has finished the 
  *            data transfer and it is ready for data.     
  * @param  WriteAddr: Address from where data are to be read.
  * @param  writebuff: pointer to the buffer that contain the data to be transferred.
  * @param  BlockSize: the SD card Data block size. The Block size should be 512.
  * @param  NumberOfBlocks: number of blocks to be written.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_WriteMultiBlocks(uint8_t *writebuff, uint64_t WriteAddr, uint16_t BlockSize, uint32_t NumberOfBlocks)
{
	SDIO_DataInitTypeDef SDIO_DataInitStructure;
	SDIO_CmdInitTypeDef  SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;
	uint32_t power = 0;
	uint32_t cardstate = 0;
	uint32_t status, try_cnt;
	int current_state;  //ready_for_data;

#if defined (SD_POLLING_MODE)
	uint32_t *tempbuff = (uint32_t *)writebuff;
#endif

	if(writebuff==NULL || NumberOfBlocks<=1)
	{
		errorstatus = SD_INVALID_PARAMETER;
		return(errorstatus);
	}

    /*!< Common to all modes */
    if (NumberOfBlocks*BlockSize > SD_MAX_DATA_LENGTH)
    {
		errorstatus = SD_INVALID_PARAMETER;
		return(errorstatus);
    }	

	SDIO->DCTRL = 0x0;

	if (SDCardInfo.CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		BlockSize = 512;
		WriteAddr /= 512;
	}

	/*!< Clear all DPSM configuration */
//	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
//	SDIO_DataInitStructure.SDIO_DataLength = 0;
//	SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_1b;
//	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
//	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
//	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
//	SDIO_DataConfig(&SDIO_DataInitStructure);
//	if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
//	{
//		errorstatus = SD_LOCK_UNLOCK_FAILED;
//		return(errorstatus);
//	}

	try_cnt = 0;
	while(try_cnt < TRY_COUNT)
	{
		errorstatus = SD_SendStatus(&status);
		if(errorstatus != SD_OK) {
			return errorstatus;
		}
		current_state = (status>>9)&0x0F;
		if(current_state!=SD_CARD_SENDING && current_state!=SD_CARD_RECEIVING && current_state!=SD_CARD_PROGRAMMING) {
			break;
		}

		if((try_cnt%TRY_INTVL) == 0) {
			if(current_state==SD_CARD_SENDING || current_state==SD_CARD_RECEIVING) {
				SD_StopMultiBlocksTrans();
				SDIO_ClearFlag(SDIO_STATIC_FLAGS);
			}
			my_printf("[%s] cur_state=%u\n\r", __FUNCTION__, current_state);
		}

		try_cnt++;
		delay_us(100);
	}
	if(try_cnt == TRY_COUNT)
	{
		errorstatus = SD_ERROR;  //SD Card not ready
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);
		return errorstatus;		
	}
	if(try_cnt >= TRY_INTVL)    //debug code
	{
		my_printf("[%s] try_cnt=%u\n\r", __FUNCTION__, try_cnt);
	}

	/*!< Set the block size, both on controller and card */
	if ((BlockSize > 0) && (BlockSize <= 2048) && ((BlockSize & (BlockSize - 1)) == 0))
	{
		power = convert_from_bytes_to_power_of_two(BlockSize);

//		SDIO_CmdInitStructure.SDIO_Argument = (uint32_t) BlockSize;
//		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
//		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//		SDIO_SendCommand(&SDIO_CmdInitStructure);
//		errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
//		if (errorstatus != SD_OK)
//		{
//		return(errorstatus);
//		}
	}
	else
	{
		errorstatus = SD_INVALID_PARAMETER;
		return(errorstatus);
	}
	
//    if (SDIO_STD_CAPACITY_SD_CARD_V1_1==SDCardInfo.CardType || SDIO_STD_CAPACITY_SD_CARD_V2_0==SDCardInfo.CardType || SDIO_HIGH_CAPACITY_SD_CARD==SDCardInfo.CardType)
//    {
//		/*!< To improve performance */
//		SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;
//		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
//		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//		SDIO_SendCommand(&SDIO_CmdInitStructure);
//		errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
//		if (errorstatus != SD_OK)
//		{
//			return(errorstatus);
//		}

//		/*!< To improve performance */
//		SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)NumberOfBlocks;
//		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCK_COUNT;
//		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//		SDIO_SendCommand(&SDIO_CmdInitStructure);
//		errorstatus = CmdResp1Error(SD_CMD_SET_BLOCK_COUNT);
//		if (errorstatus != SD_OK)
//		{
//			return(errorstatus);
//		}
//    }

	/*!< Send CMD25 WRITE_MULT_BLOCK with argument data address */
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)WriteAddr;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_WRITE_MULT_BLOCK;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_WRITE_MULT_BLOCK);
	if (SD_OK != errorstatus)
	{
		return(errorstatus);
	}

	uint32_t TotalNumberOfBytes = NumberOfBlocks * BlockSize;
	SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
	SDIO_DataInitStructure.SDIO_DataLength = TotalNumberOfBytes;
	SDIO_DataInitStructure.SDIO_DataBlockSize = (uint32_t)power << 4;
	SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToCard;
	SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
	SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
	SDIO_DataConfig(&SDIO_DataInitStructure);

#if defined (SD_POLLING_MODE)
	ALLOC_CPU_SR();             //不能被打断, 否则FIFO会空, 即出现SDIO_FLAG_TXUNDERR
	ENTER_CRITICAL();

	while (!(SDIO->STA & (SDIO_FLAG_TXUNDERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DATAEND | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_STBITERR)))
	{
		if (SDIO_GetFlagStatus(SDIO_FLAG_TXFIFOHE) != RESET)
        {
			SDIO->FIFO = tempbuff[0];    //把for()拆成代码向量,加速运行
			SDIO->FIFO = tempbuff[1];
			SDIO->FIFO = tempbuff[2];
			SDIO->FIFO = tempbuff[3];
			SDIO->FIFO = tempbuff[4];
			SDIO->FIFO = tempbuff[5];
			SDIO->FIFO = tempbuff[6];
			SDIO->FIFO = tempbuff[7];
			tempbuff += SD_HALFFIFO;
        }
	}
	EXIT_CRITICAL();

	if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
		SD_StopMultiBlocksTrans();
		errorstatus = SD_DATA_TIMEOUT;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
		SD_StopMultiBlocksTrans();
		errorstatus = SD_DATA_CRC_FAIL;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_TXUNDERR);
		SD_StopMultiBlocksTrans();
		errorstatus = SD_TX_UNDERRUN;
		return(errorstatus);
	}
	else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);
		SD_StopMultiBlocksTrans();
		errorstatus = SD_START_BIT_ERR;
		return(errorstatus);
	}

#elif defined (SD_DMA_MODE)  
	sdio_dma_config(writebuff, BlockSize*NumberOfBlocks, DMA_DIR_MemoryToPeripheral);
    SDIO_DMACmd(ENABLE);  //开启器SDIO DMA请求

	while(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TCIF6) == 0)
	{	//等待传输完成(发送时, DMA传输完成 != SDIO发送完成)
		if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != 0)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;			
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != 0)  //数据超时
			errorstatus = SD_DATA_TIMEOUT;
		else if(SDIO_GetFlagStatus(SDIO_FLAG_TXUNDERR) != 0)   //发送FIFO下溢错误(FIFO缺数据)
			errorstatus = SD_TX_UNDERRUN;
//		else if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != 0)  //在宽总线模式, 在所有数据信号上都没有检测到起始位
//			errorstatus = SD_START_BIT_ERR;              //SDIO使用DMA传输时易出现SD_START_BIT_ERR, 但传输数据仍然正确

		else if(DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TEIF6) != 0)
			errorstatus = SD_CC_ERROR;
		
		//SDIO DMA传输出错
		if(errorstatus != SD_OK)
		{
			DMA_DeInit(DMA2_Stream6);
			break;
		}
	}
	
	if(errorstatus == SD_OK)
	{	//发送时, DMA传输完成 != SDIO发送完成(SDIO_FLAG_DATAEND)[=8字节CRC校验接收完成(SDIO_FLAG_DBCKEND)+4bit高电平(包结束标志)]
		while(!(SDIO->STA & (SDIO_FLAG_DTIMEOUT|SDIO_FLAG_DCRCFAIL|SDIO_FLAG_DATAEND)))
		{
			asm("nop");
		}

//		if(SDIO_GetFlagStatus(SDIO_FLAG_DBCKEND) == 0)
//		{	//不可能发生
//			my_printf("[%s] Error! SDIO_FLAG_DBCKEND miss!!\n\r", __FUNCTION__);
//		}

		if(SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) == SET)       //数据超时
			errorstatus = SD_DATA_TIMEOUT;
		else if(SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) == SET)  //已发送/接收数据块(CRC检测失败)
			errorstatus = SD_DATA_CRC_FAIL;	
		
	}

	DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6|DMA_FLAG_HTIF6|DMA_FLAG_TEIF6);

	SDIO_DMACmd(DISABLE);
//	SDIO_ITConfig(SDIO_IT_STBITERR, DISABLE);

	if(errorstatus != SD_OK)
	{
		SD_StopMultiBlocksTrans();
		SDIO_ClearFlag(SDIO_STATIC_FLAGS);
		return errorstatus;
	}
#endif

	if(SDIO_GetFlagStatus(SDIO_FLAG_DATAEND) != RESET)
	{
		try_cnt = 0;
		do {
			errorstatus = SD_StopMultiBlocksTrans();
			if(errorstatus == SD_OK)
				break;
			try_cnt++;
		}while(try_cnt < STOP_TRY);
		if(try_cnt != 0) {
			my_printf("[%s] stop try_cnt=%u\n\r", __FUNCTION__, try_cnt);
		}

		if (errorstatus != SD_OK) {
			SDIO_ClearFlag(SDIO_STATIC_FLAGS);
			return(errorstatus);
		}
	}

	SDIO_ClearFlag(SDIO_STATIC_FLAGS);//清除所有标记

//	while(SD_GetStatus() != SD_TRANSFER_OK)
//	{
//		my_printf("SD_WR2_P\n\r");
//	}

	/*!< Wait till the card is in programming state */
	if(SDCardInfo.CardType == SDIO_MULTIMEDIA_CARD)
	{
		errorstatus = IsCardProgramming((uint8_t *)&cardstate);
		if((errorstatus==SD_OK) && ((cardstate==SD_CARD_PROGRAMMING) || (cardstate==SD_CARD_RECEIVING)))
		{
			try_cnt = 0;
			do {
				delay_us(10);
				cardstate =  sdio_dat0_read();
				try_cnt++;
			}while((cardstate==0) && (try_cnt<TIME_OUT));
		}
	}
	else  //SD_V1.1  SD_V2.0  SD_HC
	{
		try_cnt = 0;
		errorstatus = IsCardProgramming((uint8_t *)&cardstate);
		while((errorstatus==SD_OK) && ((cardstate==SD_CARD_PROGRAMMING)||(cardstate==SD_CARD_RECEIVING)) && (try_cnt<TIME_OUT))
		{
			try_cnt++;
			delay_us(10);
			errorstatus = IsCardProgramming((uint8_t *)&cardstate);
		}
	}

	return(errorstatus);
}


/**
  * @brief  Allows to erase memory area specified for the given card.
  * @param  startaddr: the start address.
  * @param  endaddr: the end address.
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_Erase(uint64_t startaddr, uint64_t endaddr)
{
	SDIO_CmdInitTypeDef  SDIO_CmdInitStructure;
  
	SD_Error errorstatus = SD_OK;
//	uint8_t cardstate = 0;
//	uint32_t delay = 0;
//	__IO uint32_t maxdelay = 0;
	uint32_t CardType = SDCardInfo.CardType;
	
	if(CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1)
	{   //低版本的卡, 擦完之再写反而会出错
		return errorstatus;
	}

	/*!< Check if the card coomnd class supports erase command */
	if (((SDCardInfo.CSD_Tab[1] >> 20) & SD_CCCC_ERASE) == 0)
	{
		errorstatus = SD_REQUEST_NOT_APPLICABLE;
		return(errorstatus);
	}

	if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
	{
		errorstatus = SD_LOCK_UNLOCK_FAILED;
		return(errorstatus);
	}

	if (SDCardInfo.CardType == SDIO_HIGH_CAPACITY_SD_CARD)
	{
		startaddr /= 512;
		endaddr /= 512;
	}
  
	/*!< According to sd-card spec 1.0 ERASE_GROUP_START (CMD32) and erase_group_end(CMD33) */
	if ((SDIO_STD_CAPACITY_SD_CARD_V2_0 == CardType) || (SDIO_HIGH_CAPACITY_SD_CARD == CardType))
	{
		/*!< Send CMD32 SD_ERASE_GRP_START with argument as addr  */
		SDIO_CmdInitStructure.SDIO_Argument = startaddr;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_ERASE_GRP_START;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp1Error(SD_CMD_SD_ERASE_GRP_START);
		if (errorstatus != SD_OK)
		{
			return(errorstatus);
		}	

		/*!< Send CMD33 SD_ERASE_GRP_END with argument as addr  */
		SDIO_CmdInitStructure.SDIO_Argument = endaddr;
		SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_ERASE_GRP_END;
		SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
		SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
		SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
		SDIO_SendCommand(&SDIO_CmdInitStructure);
		errorstatus = CmdResp1Error(SD_CMD_SD_ERASE_GRP_END);
		if (errorstatus != SD_OK)
		{
			return(errorstatus);
		}
	}

	/*!< Send CMD38 ERASE */
	SDIO_CmdInitStructure.SDIO_Argument = 0;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_ERASE;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);
	errorstatus = CmdResp1Error(SD_CMD_ERASE);
	if (errorstatus != SD_OK)
	{
		return(errorstatus);
	}

//	maxdelay = 120000 / ((SDIO->CLKCR & 0xFF) + 2);
//	for (delay = 0; delay < maxdelay; delay++)
//	{}

	return(errorstatus);
	
//	/*!< Wait till the card is in programming state */
//	errorstatus = IsCardProgramming(&cardstate);
//	while ((errorstatus == SD_OK) && ((SD_CARD_PROGRAMMING == cardstate) || (SD_CARD_RECEIVING == cardstate)))
//	{
//		errorstatus = IsCardProgramming(&cardstate);
//	}
//	return(errorstatus);
}

//从SD_Erase中单独拿出擦写后的检查等待部分
void sd_wait_busy(void)
{
	SD_Error errorstatus = SD_OK;
	uint8_t cardstate = 0;
		
	/*!< Wait till the card is in programming state */
	errorstatus = IsCardProgramming(&cardstate);
	while ((errorstatus == SD_OK) && ((SD_CARD_PROGRAMMING == cardstate) || (SD_CARD_RECEIVING == cardstate)))
	{
		errorstatus = IsCardProgramming(&cardstate);
	}
}


/**
  * @brief  Gets the cuurent data transfer state.
  * @param  None
  * @retval SDTransferState: Data Transfer state.
  *   This value can be: 
  *        - SD_TRANSFER_OK: No data transfer is acting
  *        - SD_TRANSFER_BUSY: Data transfer is acting
  */
SDTransferState SD_GetTransferState(void)
{
  if (SDIO->STA & (SDIO_FLAG_TXACT | SDIO_FLAG_RXACT))
  {
    return(SD_TRANSFER_BUSY);
  }
  else
  {
    return(SD_TRANSFER_OK);
  }
}

/**
  * @brief  Aborts an ongoing data transfer.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
SD_Error SD_StopTransfer(void)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SD_Error errorstatus = SD_OK;

  /*!< Send CMD12 STOP_TRANSMISSION  */
  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_STOP_TRANSMISSION;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_STOP_TRANSMISSION);

  return(errorstatus);
}


















//============================ISR Function============================
void SDIO_IRQHandler(void)
{
	SD_ProcessIRQSrc();
}

SD_Error SD_ProcessIRQSrc(void)
{
	my_printf("SDIO irq\n\r");
	
	if(SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
	{
		SDIO_ClearFlag(SDIO_FLAG_STBITERR);
		my_printf("SDIO_FLAG_STBITERR\n\n\n\n\n\n\n\n\r");
	}	
	
	return SD_OK;
}






//========================================static functions=================================
/**
  * @brief  Checks for error conditions for CMD0.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdError(void)
{
  SD_Error errorstatus = SD_OK;
  uint32_t timeout;

  timeout = SDIO_CMD0TIMEOUT; /*!< 10000 */

  while ((timeout > 0) && (SDIO_GetFlagStatus(SDIO_FLAG_CMDSENT) == RESET))
  {
    timeout--;
  }

  if (timeout == 0)
  {
    errorstatus = SD_CMD_RSP_TIMEOUT;
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R7 response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp7Error(void)
{
  SD_Error errorstatus = SD_OK;
  uint32_t status;
  uint32_t timeout = SDIO_CMD0TIMEOUT;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)) && (timeout > 0))
  {
    timeout--;
    status = SDIO->STA;
  }

  if ((timeout == 0) || (status & SDIO_FLAG_CTIMEOUT))
  {
    /*!< Card is not V2.0 complient or card does not support the set voltage range */
    errorstatus = SD_CMD_RSP_TIMEOUT;
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return(errorstatus);
  }

  if (status & SDIO_FLAG_CMDREND)
  {
    /*!< Card is SD V2.0 compliant */
    errorstatus = SD_OK;
    SDIO_ClearFlag(SDIO_FLAG_CMDREND);
    return(errorstatus);
  }
  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R1 response.
  * @param  cmd: The sent command index.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp1Error(uint8_t cmd)
{
	while (!(SDIO->STA & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
	{
	}

  	if(SDIO_GetFlagStatus(SDIO_FLAG_CTIMEOUT) != RESET)	    //响应超时
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT); 				//清除命令响应超时标志
		return SD_CMD_RSP_TIMEOUT;
	}
	
 	if(SDIO_GetFlagStatus(SDIO_FLAG_CCRCFAIL) != RESET)	    //CRC错误
	{																				    
 		SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL); 				//清除标志
		return SD_CMD_CRC_FAIL;
	}
  
	/*!< Check response received is of desired command */
	if (SDIO_GetCommandResponse() != cmd)
	{
		return SD_ILLEGAL_CMD;    //命令不匹配
	}
		
	/*!< Clear all the static flags */
	SDIO_ClearFlag(SDIO_STATIC_FLAGS);

	return (SD_Error)(SDIO->RESP1 &  SD_OCR_ERRORBITS);
}

/**
  * @brief  Checks for error conditions for R3 (OCR) response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp3Error(void)
{
  SD_Error errorstatus = SD_OK;
  uint32_t status;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
  {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT)
  {
    errorstatus = SD_CMD_RSP_TIMEOUT;
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return(errorstatus);
  }
  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);
  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R2 (CID or CSD) response.
  * @param  None
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp2Error(void)
{
  SD_Error errorstatus = SD_OK;
  uint32_t status;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND)))
  {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT)
  {
    errorstatus = SD_CMD_RSP_TIMEOUT;
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return(errorstatus);
  }
  else if (status & SDIO_FLAG_CCRCFAIL)
  {
    errorstatus = SD_CMD_CRC_FAIL;
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  return(errorstatus);
}

/**
  * @brief  Checks for error conditions for R6 (RCA) response.
  * @param  cmd: The sent command index.
  * @param  prca: pointer to the variable that will contain the SD card relative 
  *         address RCA. 
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error CmdResp6Error(uint8_t cmd, uint32_t *prca)
{
  SD_Error errorstatus = SD_OK;
  uint32_t status;
  uint32_t response_r1;

  status = SDIO->STA;

  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CTIMEOUT | SDIO_FLAG_CMDREND)))
  {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT)
  {
    errorstatus = SD_CMD_RSP_TIMEOUT;
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return(errorstatus);
  }
  else if (status & SDIO_FLAG_CCRCFAIL)
  {
    errorstatus = SD_CMD_CRC_FAIL;
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return(errorstatus);
  }

  /*!< Check response received is of desired command */
  if (SDIO_GetCommandResponse() != cmd)
  {
    errorstatus = SD_ILLEGAL_CMD;
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  /*!< We have received response, retrieve it.  */
  response_r1 = SDIO_GetResponse(SDIO_RESP1);

  if (SD_ALLZERO == (response_r1 & (SD_R6_GENERAL_UNKNOWN_ERROR | SD_R6_ILLEGAL_CMD | SD_R6_COM_CRC_FAILED)))
  {
    *prca = response_r1;
    return(errorstatus);
  }

  if (response_r1 & SD_R6_GENERAL_UNKNOWN_ERROR)
  {
    return(SD_GENERAL_UNKNOWN_ERROR);
  }

  if (response_r1 & SD_R6_ILLEGAL_CMD)
  {
    return(SD_ILLEGAL_CMD);
  }

  if (response_r1 & SD_R6_COM_CRC_FAILED)
  {
    return(SD_COM_CRC_FAILED);
  }

  return(errorstatus);
}

/**
  * @brief  Enables or disables the SDIO 4bits bus mode.
  * @param  NewState: new state of the SDIO wide bus mode.
  *   This parameter can be: ENABLE or DISABLE.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error SDEnWideBus(FunctionalState NewState)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SD_Error errorstatus = SD_OK;

  uint32_t scr[2] = {0, 0};

  if (SDIO_GetResponse(SDIO_RESP1) & SD_CARD_LOCKED)
  {
    errorstatus = SD_LOCK_UNLOCK_FAILED;
    return(errorstatus);
  }

  /*!< Get SCR Register */
  errorstatus = FindSCR(SDCardInfo.RCA, scr);

  if (errorstatus != SD_OK)
  {
    return(errorstatus);
  }
//  my_printf("SCR[0]=%x\n\r", scr[0]);
//  my_printf("SCR[1]=%x\n\r", scr[1]);

  /*!< If wide bus operation to be enabled */
  if (NewState == ENABLE)
  {
    /*!< If requested card supports wide bus operation */
    if ((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO)
    {
      /*!< Send CMD55 APP_CMD with argument as card's RCA.*/
      SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
      if (errorstatus != SD_OK)
      {
        return(errorstatus);
      }

      /*!< Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
      SDIO_CmdInitStructure.SDIO_Argument = 0x2;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
      if (errorstatus != SD_OK)
      {
        return(errorstatus);
      }
      return(errorstatus);
    }
    else
    {
      errorstatus = SD_REQUEST_NOT_APPLICABLE;
      return(errorstatus);
    }
  }   /*!< If wide bus operation to be disabled */
  else
  {
    /*!< If requested card supports 1 bit mode operation */
    if ((scr[1] & SD_SINGLE_BUS_SUPPORT) != SD_ALLZERO)
    {
      /*!< Send CMD55 APP_CMD with argument as card's RCA.*/
      SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_APP_CMD);
      if (errorstatus != SD_OK)
      {
        return(errorstatus);
      }

      /*!< Send ACMD6 APP_CMD with argument as 0 for single bus mode */
      SDIO_CmdInitStructure.SDIO_Argument = 0x00;
      SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_SD_SET_BUSWIDTH;
      SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
      SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
      SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
      SDIO_SendCommand(&SDIO_CmdInitStructure);
      errorstatus = CmdResp1Error(SD_CMD_APP_SD_SET_BUSWIDTH);
      if (errorstatus != SD_OK)
      {
        return(errorstatus);
      }

      return(errorstatus);
    }
    else
    {
      errorstatus = SD_REQUEST_NOT_APPLICABLE;
      return(errorstatus);
    }
  }
}

/**
  * @brief  Checks if the SD card is in programming state.
  * @param  pstatus: pointer to the variable that will contain the SD card state.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error IsCardProgramming(uint8_t *pstatus)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
  SD_Error errorstatus = SD_OK;
  __IO uint32_t respR1 = 0, status = 0;

  SDIO_CmdInitStructure.SDIO_Argument = SDCardInfo.RCA;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SEND_STATUS;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  status = SDIO->STA;
  while (!(status & (SDIO_FLAG_CCRCFAIL | SDIO_FLAG_CMDREND | SDIO_FLAG_CTIMEOUT)))
  {
    status = SDIO->STA;
  }

  if (status & SDIO_FLAG_CTIMEOUT)
  {
    errorstatus = SD_CMD_RSP_TIMEOUT;
    SDIO_ClearFlag(SDIO_FLAG_CTIMEOUT);
    return(errorstatus);
  }
  else if (status & SDIO_FLAG_CCRCFAIL)
  {
    errorstatus = SD_CMD_CRC_FAIL;
    SDIO_ClearFlag(SDIO_FLAG_CCRCFAIL);
    return(errorstatus);
  }

  status = (uint32_t)SDIO_GetCommandResponse();

  /*!< Check response received is of desired command */
  if (status != SD_CMD_SEND_STATUS)
  {
    errorstatus = SD_ILLEGAL_CMD;
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);


  /*!< We have received response, retrieve it for analysis  */
  respR1 = SDIO_GetResponse(SDIO_RESP1);

  /*!< Find out card status */
  *pstatus = (uint8_t) ((respR1 >> 9) & 0x0000000F);

  if ((respR1 & SD_OCR_ERRORBITS) == SD_ALLZERO)
  {
    return(errorstatus);
  }

  if (respR1 & SD_OCR_ADDR_OUT_OF_RANGE)
  {
    return(SD_ADDR_OUT_OF_RANGE);
  }

  if (respR1 & SD_OCR_ADDR_MISALIGNED)
  {
    return(SD_ADDR_MISALIGNED);
  }

  if (respR1 & SD_OCR_BLOCK_LEN_ERR)
  {
    return(SD_BLOCK_LEN_ERR);
  }

  if (respR1 & SD_OCR_ERASE_SEQ_ERR)
  {
    return(SD_ERASE_SEQ_ERR);
  }

  if (respR1 & SD_OCR_BAD_ERASE_PARAM)
  {
    return(SD_BAD_ERASE_PARAM);
  }

  if (respR1 & SD_OCR_WRITE_PROT_VIOLATION)
  {
    return(SD_WRITE_PROT_VIOLATION);
  }

  if (respR1 & SD_OCR_LOCK_UNLOCK_FAILED)
  {
    return(SD_LOCK_UNLOCK_FAILED);
  }

  if (respR1 & SD_OCR_COM_CRC_FAILED)
  {
    return(SD_COM_CRC_FAILED);
  }

  if (respR1 & SD_OCR_ILLEGAL_CMD)
  {
    return(SD_ILLEGAL_CMD);
  }

  if (respR1 & SD_OCR_CARD_ECC_FAILED)
  {
    return(SD_CARD_ECC_FAILED);
  }

  if (respR1 & SD_OCR_CC_ERROR)
  {
    return(SD_CC_ERROR);
  }

  if (respR1 & SD_OCR_GENERAL_UNKNOWN_ERROR)
  {
    return(SD_GENERAL_UNKNOWN_ERROR);
  }

  if (respR1 & SD_OCR_STREAM_READ_UNDERRUN)
  {
    return(SD_STREAM_READ_UNDERRUN);
  }

  if (respR1 & SD_OCR_STREAM_WRITE_OVERRUN)
  {
    return(SD_STREAM_WRITE_OVERRUN);
  }

  if (respR1 & SD_OCR_CID_CSD_OVERWRIETE)
  {
    return(SD_CID_CSD_OVERWRITE);
  }

  if (respR1 & SD_OCR_WP_ERASE_SKIP)
  {
    return(SD_WP_ERASE_SKIP);
  }

  if (respR1 & SD_OCR_CARD_ECC_DISABLED)
  {
    return(SD_CARD_ECC_DISABLED);
  }

  if (respR1 & SD_OCR_ERASE_RESET)
  {
    return(SD_ERASE_RESET);
  }

  if (respR1 & SD_OCR_AKE_SEQ_ERROR)
  {
    return(SD_AKE_SEQ_ERROR);
  }

  return(errorstatus);
}


/**
  * @brief  Find the SD card SCR register value.
  * @param  rca: selected card address.
  * @param  pscr: pointer to the buffer that will contain the SCR value.
  * @retval SD_Error: SD Card Error code.
  */
static SD_Error FindSCR(uint32_t rca, uint32_t *pscr)
{
	SDIO_CmdInitTypeDef SDIO_CmdInitStructure;
	SDIO_DataInitTypeDef SDIO_DataInitStructure;
	volatile int dummy;
  uint32_t index = 0;
  SD_Error errorstatus = SD_OK;
  uint32_t tempscr[2] = {0, 0};

  /*!< Clear all DPSM configuration */
  SDIO->DCTRL = 0x0;

  /*!< Set Block Size To 8 Bytes */
  /*!< Send CMD55 APP_CMD with argument as card's RCA */
//  SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)8;
//  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
//  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
//  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
//  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
//  SDIO_SendCommand(&SDIO_CmdInitStructure);

//  errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);

//  if (errorstatus != SD_OK)
//  {
//    return(errorstatus);
//  }

  /*!< Send CMD55 APP_CMD with argument as card's RCA */
  SDIO_CmdInitStructure.SDIO_Argument = rca;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_APP_CMD;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_APP_CMD);

  if (errorstatus != SD_OK)
  {
    return(errorstatus);
  }
  SDIO_DataInitStructure.SDIO_DataTimeOut = SD_DATATIMEOUT;
  SDIO_DataInitStructure.SDIO_DataLength = 8;
  SDIO_DataInitStructure.SDIO_DataBlockSize = SDIO_DataBlockSize_8b;
  SDIO_DataInitStructure.SDIO_TransferDir = SDIO_TransferDir_ToSDIO;
  SDIO_DataInitStructure.SDIO_TransferMode = SDIO_TransferMode_Block;
  SDIO_DataInitStructure.SDIO_DPSM = SDIO_DPSM_Enable;
  SDIO_DataConfig(&SDIO_DataInitStructure);


  /*!< Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
  SDIO_CmdInitStructure.SDIO_Argument = 0x0;
  SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SD_APP_SEND_SCR;
  SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
  SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
  SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
  SDIO_SendCommand(&SDIO_CmdInitStructure);

  errorstatus = CmdResp1Error(SD_CMD_SD_APP_SEND_SCR);

  if (errorstatus != SD_OK)
  {
    return(errorstatus);
  }

  while (!(SDIO->STA & (SDIO_FLAG_RXOVERR | SDIO_FLAG_DCRCFAIL | SDIO_FLAG_DTIMEOUT | SDIO_FLAG_DBCKEND | SDIO_FLAG_STBITERR)))
  {
    if (SDIO_GetFlagStatus(SDIO_FLAG_RXDAVL) != RESET)
    {
      if(index < 2)
      {
		*(tempscr + index) = SDIO_ReadData();
		index++;
      }
	  else
	  {
		dummy = SDIO_ReadData();
	  }
    }
  }

  if (SDIO_GetFlagStatus(SDIO_FLAG_DTIMEOUT) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_DTIMEOUT);
    errorstatus = SD_DATA_TIMEOUT;
    return(errorstatus);
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_DCRCFAIL) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_DCRCFAIL);
    errorstatus = SD_DATA_CRC_FAIL;
    return(errorstatus);
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_RXOVERR) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_RXOVERR);
    errorstatus = SD_RX_OVERRUN;
    return(errorstatus);
  }
  else if (SDIO_GetFlagStatus(SDIO_FLAG_STBITERR) != RESET)
  {
    SDIO_ClearFlag(SDIO_FLAG_STBITERR);
    errorstatus = SD_START_BIT_ERR;
    return(errorstatus);
  }

  /*!< Clear all the static flags */
  SDIO_ClearFlag(SDIO_STATIC_FLAGS);

  *(pscr + 1) = ((tempscr[0] & SD_0TO7BITS) << 24) | ((tempscr[0] & SD_8TO15BITS) << 8) | ((tempscr[0] & SD_16TO23BITS) >> 8) | ((tempscr[0] & SD_24TO31BITS) >> 24);
  *(pscr) = ((tempscr[1] & SD_0TO7BITS) << 24) | ((tempscr[1] & SD_8TO15BITS) << 8) | ((tempscr[1] & SD_16TO23BITS) >> 8) | ((tempscr[1] & SD_24TO31BITS) >> 24);

  SDCardInfo.SCR_Tab[0] = tempscr[0];
  SDCardInfo.SCR_Tab[1] = tempscr[1];

  return(errorstatus);
}

/**
  * @brief  Converts the number of bytes in power of two and returns the power.
  * @param  NumberOfBytes: number of bytes.
  * @retval None
  */
uint8_t convert_from_bytes_to_power_of_two(uint16_t NumberOfBytes)
{
  uint8_t count = 0;

  while (NumberOfBytes != 1)
  {
    NumberOfBytes >>= 1;
    count++;
  }
  return(count);
}


//========================================easy use functions=================================
//SD数据块大小与缓存的数据块数
#define  SD_BLK_SIZE       512

/*
int  sd_probe(void)       //abandon this function
{
	int ret = SD_Init();
	if( ret != SD_OK ) {
		my_printf("SD init error, ret=%d!!!\n\r", ret);
		SD_PowerOFF();   //关闭SDIO的CLK输出, 否则SD_Init()后SDIO_CLK一直输出时钟
	}
	else
	{
		my_printf("SDCardInfo.CardType=%d\n\r", SDCardInfo.CardType);
		my_printf("SDCardInfo.RCA=%X\n\r", SDCardInfo.RCA);
		my_printf("SDCardInfo.CardBlockSize=%d\n\r", SDCardInfo.CardBlockSize);
		my_printf("SDCardInfo.CardCapacity=%dMB\n\r", (uint32_t)(SDCardInfo.CardCapacity>>20));
	}

	return ret;
}
*/

//打印相关寄存器信息
void sd_prt_reg(void)
{
//	int ret;
	
	//show SSR
//	SD_SSR card_status;
//	ret = SD_GetCardStatus(&card_status);
//	if (SD_OK == ret)
//	{
//		my_printf("card_status.DAT_BUS_WIDTH=%d\n\r", card_status.DAT_BUS_WIDTH);
//		my_printf("card_status.SD_CARD_TYPE=0x%X\n\r", card_status.SD_CARD_TYPE);
//		my_printf("card_status.SPEED_CLASS=%d\n\r", card_status.SPEED_CLASS);
//		my_printf("card_status.AU_SIZE=%d\n\r", card_status.AU_SIZE);
//		my_printf("card_status.ERASE_SIZE=%d\n\r", card_status.ERASE_SIZE);
//		my_printf("card_status.ERASE_TIMEOUT=%d\n\r", card_status.ERASE_TIMEOUT);
//	}
//	else
//	{
//		my_printf("Get card SSR error, ret=%d\n\r", ret);
//	}
	
	SD_CSD SD_csd;
	SD_CID SD_cid;
	SD_GetCardInfo(&SDCardInfo, &SD_csd, &SD_cid);  //获取CardCapacity CardBlockSize
	my_printf("CSD reg:\n\r");
	my_printf("SD_csd.CardComdClasses=0x%04X\n\r", SD_csd.CardComdClasses);
	my_printf("SD_csd.MaxBusClkFrec=0x%02X\n\r", SD_csd.MaxBusClkFrec);
}


/*
 * SD_ReadBlock() SD_ReadMultiBlocks() SD_WriteBlock() SD_WriteMultiBlocks()
 * 以上4个函数都要求访问地址512字节对齐, 数据缓存地址4字节对齐. 为了满足这个要求,
 * 对以上4个函数封装, 访问地址与数据缓存地址都任意字节对齐, 于是就有了如下函数.
 */
int sd_write_disk(uint64_t start_addr, void *buff, int len)
{
	int i, blk_cnt, ret = 0;
	uint8_t  *pdat = buff;
	uint8_t __attribute__((aligned(4))) temp[SD_BLK_SIZE];  //开辟512字节的栈

	//检查start_addr与len是否按SD_BLK_SIZE对齐
	if(len&(SD_BLK_SIZE-1) || (uint32_t)start_addr&(SD_BLK_SIZE-1))
		return -1;

	blk_cnt = len/SD_BLK_SIZE;
	if(((uint32_t)buff & 0x03) != 0)
	{	//buff地址未4字节对齐
		for(i=0; i<blk_cnt; i++) {   //一次写取一个块
			u_memcpy(temp, pdat, SD_BLK_SIZE);
			ret = SD_WriteBlock(temp, start_addr, SD_BLK_SIZE);
			if( ret )
				return -ret;
			start_addr += SD_BLK_SIZE;
			pdat += SD_BLK_SIZE;
		}
		ret = i * SD_BLK_SIZE;
		return ret;
	}
	else
	{	//buff地址4字节对齐
		if(blk_cnt == 1)
		{
			ret = SD_WriteBlock(buff, start_addr, SD_BLK_SIZE);
			if( ret )
				return -ret;
			return SD_BLK_SIZE;
		}
		else
		{
			ret = SD_WriteMultiBlocks(buff, start_addr, SD_BLK_SIZE, blk_cnt);
			if( ret )
				return -ret;
			return blk_cnt*SD_BLK_SIZE;				
		}
	}
}

//许多fatfs函数栈深近1KB, 这些函数再调用磁盘读写函数, 如果磁盘读写函数的栈也很深, 就需要开辟很大的栈.
//应用函数cp_file()的栈本身就很大, 再往下一层一层掉用, 会消耗大量的栈空间

/*
* @brief: fatfs专用, start_addr与len必须按SD_BLK_SIZE对齐, write()函数不必这样
 */
int sd_read_disk(uint64_t start_addr, void *buff, int len)
{
	int i, blk_cnt, ret = 0;
	uint8_t  *pdat = buff;
	uint8_t __attribute__((aligned(4))) temp[SD_BLK_SIZE];  //开辟512字节的栈
	
	//检查start_addr与len是否按SD_BLK_SIZE对齐
	if(len&(SD_BLK_SIZE-1) || (uint32_t)start_addr&(SD_BLK_SIZE-1))
		return -1;

	blk_cnt = len/SD_BLK_SIZE;
	if(((uint32_t)buff & 0x03) != 0)
	{	//buff地址未4字节对齐
		for(i=0; i<blk_cnt; i++) {   //一次读取一个块
			ret = SD_ReadBlock(temp, start_addr, SD_BLK_SIZE);
			if( ret )
				return -ret;
			u_memcpy(pdat, temp, SD_BLK_SIZE);
			start_addr += SD_BLK_SIZE;
			pdat += SD_BLK_SIZE;
		}
		ret = i * SD_BLK_SIZE;
		return ret;
	}
	else
	{	//buff地址4字节对齐
		if(blk_cnt == 1)
		{
			ret = SD_ReadBlock(buff, start_addr, SD_BLK_SIZE);
			if( ret )
				return -ret;
			return SD_BLK_SIZE;
		}
		else
		{
			ret = SD_ReadMultiBlocks(buff, start_addr, SD_BLK_SIZE, blk_cnt);
			if( ret )
				return -ret;
			return blk_cnt*SD_BLK_SIZE;				
		}
	}
}


//==============================================================================
/**
 ** @brief: 探测卡槽是否有SD卡插入, 若有则读取SD卡结构体信息.
 ** @retvl: 0--卡槽已插入SD卡
 **         非0--卡槽无SD卡
 **/
int sd_detect(void)
{
	SD_Error errorstatus = SD_OK;

	SD_DeInit();

	/* SDIO Peripheral Low Level Init */
	sdio_controller_init();
	sdio_controller_irq_set(ENABLE);
	delay_ms(10);      //使CLK维持一段时间,让SD卡稳定, 对于有些SD卡是必须的, 否则会初始化失败

	SDCardInfo.CardType = SDIO_STD_CAPACITY_SD_CARD_V1_1;
	errorstatus = SD_PowerON();  //获取CardType
	if (errorstatus != SD_OK)
	{
		SD_PowerOFF();   //关闭SDIO的CLK输出, 否则SD_Init()后SDIO_CLK一直输出时钟

		/*!< CMD Response TimeOut (wait for CMDSENT flag) */
		return(errorstatus);
	}

	errorstatus = SD_InitializeCards(); //获取RCA CSD_Tab[4] CID_Tab[4]
	if (errorstatus != SD_OK)
	{
		SD_PowerOFF();   //关闭SDIO的CLK输出, 否则SD_Init()后SDIO_CLK一直输出时钟

		/*!< CMD Response TimeOut (wait for CMDSENT flag) */
		return(errorstatus);
	}	

	if (errorstatus == SD_OK)
	{
		/*----------------- Read CSD/CID MSD registers ------------------*/
//		SD_CSD SD_csd;
		SD_CID SD_cid;
		errorstatus = SD_GetCardInfo(&SDCardInfo, &SDCardInfo.SD_csd, &SD_cid);  //获取CardCapacity CardBlockSize 
	}

	if (errorstatus == SD_OK)
	{
		/*----------------- Select Card --------------------------------*/
		errorstatus = SD_SelectDeselect(SDCardInfo.RCA);
	}
	
	if(errorstatus == SD_OK)
	{
		my_printf("SDCardInfo.CardType=%d\n\r", SDCardInfo.CardType);
		my_printf("SDCardInfo.RCA=%X\n\r", SDCardInfo.RCA);
		my_printf("SDCardInfo.CardBlockSize=%d\n\r", SDCardInfo.CardBlockSize);
		my_printf("SDCardInfo.CardCapacity=%dMB\n\r", (uint32_t)(SDCardInfo.CardCapacity>>20));

//		my_printf("CSD.CSDStruct=%d\n\r", SDCardInfo.SD_csd.CSDStruct);            //0--V1.01-V1.10 1--V2.00/Standard Capacity  2or3--V2.00/High Capacity
//		my_printf("CSD.SysSpecVersion=%d\n\r", SDCardInfo.SD_csd.SysSpecVersion);  //重点关注, MMC卡此值为4时可能为支持4线或8线的双排接口卡
//		my_printf("CSD.MaxBusClkFrec=%d\n\r", SDCardInfo.SD_csd.MaxBusClkFrec);
//		my_printf("CSD.CardComdClasses=%d\n\r", SDCardInfo.SD_csd.CardComdClasses);
//		my_printf("CSD.RdBlockLen=%d\n\r", SDCardInfo.SD_csd.RdBlockLen);
//		my_printf("CSD.PartBlockRead=%d\n\r", SDCardInfo.SD_csd.PartBlockRead);
//		my_printf("CSD.DeviceSizeMul=%d\n\r", SDCardInfo.SD_csd.DeviceSizeMul);
//		my_printf("CSD.MaxWrBlockLen=%d\n\r", SDCardInfo.SD_csd.MaxWrBlockLen);
//		my_printf("CSD.WriteBlockPaPartial=%d\n\r", SDCardInfo.SD_csd.WriteBlockPaPartial);
	}
	else
	{
		my_printf("SD detect error, errorstatus=%d!!!\n\r", errorstatus);
		SD_PowerOFF();   //关闭SDIO的CLK输出, 否则SD_Init()后SDIO_CLK一直输出时钟
	}

	return(errorstatus);
}

//SDIO_ClockEdge_Rising与SDIO_ClockEdge_Falling是什么鬼?
//SDIO_ClockEdge_Rising -在主时钟SDIOCLK的上升沿产生SDIO_CK
//SDIO_ClockEdge_Falling-在主时钟SDIOCLK的下降沿产生SDIO_CK

//SDIO Host and Device数据采样问题:
//Both host command and card responses are clocked out with the rising edge of the host clock.

//SDIO不管是从主机控制器向SD卡传输, 还是SD卡向主机控制器传输都只在主机clock的上升沿有效.
//这与SDIO_ClockEdge_Rising或SDIO_ClockEdge_Falling无关

//SDIO总线位宽与速度配置
int sd_bus_width_speed_config(void)
{
	SD_Error errorstatus = SD_OK;
	uint32_t status = 0;

	if(SDCardInfo.CardType == SDIO_MULTIMEDIA_CARD)
	{	//MMC卡, 使用单线模式即可
		/*!< Configure the SDIO peripheral */
//		sdio_dat123_release();

//		ret = SD_SendStatus(&status);
//		if(ret != SD_OK) {
//			return ret;
//		}
//		my_printf("MMC Card Status1: 0x%X\n\r", status);  //status is 0x900

		//初始态到高速传输模式的过渡态
		delay_us(300);
		sdio_controller_config(SDIO_TEMP_CLK_DIV_MMC, SDIO_ClockEdge_Rising, SDIO_BusWide_1b, SDIO_ClockPowerSave_Disable);
		delay_us(50);    //时钟维持一段时间, 使MMC卡适应
		errorstatus = SD_SendStatus(&status);
		if(errorstatus != SD_OK) {
			return errorstatus;
		}		
		my_printf("MMC Card Status2: 0x%X\n\r", status);  //status is 0x900

		//特别注意: MMC卡高速(20MHz max)
		if(SDCardInfo.SD_csd.SysSpecVersion > 3) {
			//可能为支持4线或8线的双排触点接口卡, 需要用CMD19与CMD14进行探测, 用CMD6切换. 
			//若不进行探测与切换, 直接使用默认的1线传输, 则传输可能会失败.
			//由于没有研究探测与切换方法, 故先不支持这种高版本的双排触点卡
			my_printf("MMC version %u, unsupport!\n\r", SDCardInfo.SD_csd.SysSpecVersion);
			while(1)  //die here
			{	asm("nop"); }
		}
		else {
			//7触点接口MMC卡, 时钟频率(20MHz max)
			delay_us(50);  
			sdio_controller_config(SDIO_TRANSFER_CLK_DIV_MMC, SDIO_ClockEdge_Rising, SDIO_BusWide_1b, SDIO_ClockPowerSave_Disable);  //SDIO_ClockEdge_Falling
		}
//		ret = SD_SendStatus(&status);
//		if(ret != SD_OK) {
//			return ret;
//		}
//		my_printf("MMC Card Status3: 0x%X\n\r", status);  //status is 0x900
		delay_us(10);  //高速时钟维持10us, 使MMC卡适应

		my_printf("MMC Card, Bus width 1b, Bus speed %uMHz\n\r", 48/(SDIO_TRANSFER_CLK_DIV_MMC+2));
	}
	else
	{	//SD_V1.1 SD_V2.0 or HIGH_CAPACITY_SD
		uint32_t cur_width;
		int sdio_clk_div;

		//初始态到高速传输模式的过渡态
		delay_us(300);
		sdio_controller_config(SDIO_TEMP_CLK_DIV, SDIO_ClockEdge_Rising, SDIO_BusWide_1b, SDIO_ClockPowerSave_Disable);
		delay_us(50);    //时钟维持一段时间, 使SD卡适应
		errorstatus = SD_SendStatus(&status);
		if(errorstatus != SD_OK) {
			return errorstatus;
		}		
		my_printf("SD Card Status2: 0x%X\n\r", status);  //status is 0x900		

		delay_us(50);
		errorstatus = SDEnWideBus(ENABLE);  //开启4线模式
		SD_GetSCRInfo(&SDCardInfo, &SDCardInfo.SD_scr);
		if (SD_OK == errorstatus) {
			/*!< Configure the SDIO peripheral */
//			if(SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1)  //SD卡从MMC卡发展而来, 早期的SD卡在时钟下降沿采集数据
//				sdio_controller_config(SDIO_TRANSFER_CLK_DIV, SDIO_ClockEdge_Falling, SDIO_BusWide_4b, SDIO_ClockPowerSave_Disable);
//			else
				sdio_controller_config(SDIO_TRANSFER_CLK_DIV, SDIO_ClockEdge_Rising, SDIO_BusWide_4b, SDIO_ClockPowerSave_Disable);
			cur_width = SDIO_BusWide_4b;
		}
		else {
//			if(SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1)
//				sdio_controller_config(SDIO_TRANSFER_CLK_DIV, SDIO_ClockEdge_Falling, SDIO_BusWide_1b, SDIO_ClockPowerSave_Disable);
//			else
				sdio_controller_config(SDIO_TRANSFER_CLK_DIV, SDIO_ClockEdge_Rising, SDIO_BusWide_1b, SDIO_ClockPowerSave_Disable);
			cur_width = SDIO_BusWide_1b;
		}
		delay_us(10);  //高速时钟维持10us, 使SD卡适应
		sdio_clk_div = SDIO_TRANSFER_CLK_DIV;

		//debug printf
		int bus_width = (cur_width==SDIO_BusWide_1b ? 1 : 4);
		if(SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V1_1) {
			my_printf("SD_V1.1, Bus width %db, Bus speed %uMHz\n\r", bus_width, 48/(sdio_clk_div+2));
		}
		else if(SDCardInfo.CardType == SDIO_STD_CAPACITY_SD_CARD_V2_0) {
			my_printf("SD_V2.0, Bus width %db, Bus speed %uMHz\n\r", bus_width, 48/(sdio_clk_div+2));
		}
		else if(SDCardInfo.CardType == SDIO_HIGH_CAPACITY_SD_CARD) {
			my_printf("SD_HighCapacity, Bus width %db, Bus speed %uMHz\n\r", bus_width, 48/(sdio_clk_div+2));
		}
		else {
			my_printf("Warning! SD type: %d\n\r", SDCardInfo.CardType);
		}

//		my_printf("SCR[0]=0x%08X\n\r", SDCardInfo.SCR_Tab[0]);
//		my_printf("SCR[1]=0x%08X\n\r", SDCardInfo.SCR_Tab[1]);
//		my_printf("SCR.SDSpec=%u\n\r", SDCardInfo.SD_scr.SDSpec);
//		my_printf("SCR.SDSecurity=%u\n\r", SDCardInfo.SD_scr.SDSecurity);
//		my_printf("SCR.DataBusWidth=%u\n\r", SDCardInfo.SD_scr.DataBusWidth);
	}

	return 0;
}

//SD卡数据块大小设置为512字节
//如果不改变数据块大小, 那么只需设置1次, 以后再读写SD卡数据时就不用再设置
int sd_block_512B_config(void)
{
	SDIO_CmdInitTypeDef  SDIO_CmdInitStructure;
	SD_Error errorstatus = SD_OK;

	//SD数据块大小调整为512字节
	/*!< Set Block Size To 512 Bytes */
	SDIO_CmdInitStructure.SDIO_Argument = (uint32_t)512;
	SDIO_CmdInitStructure.SDIO_CmdIndex = SD_CMD_SET_BLOCKLEN;
	SDIO_CmdInitStructure.SDIO_Response = SDIO_Response_Short;
	SDIO_CmdInitStructure.SDIO_Wait = SDIO_Wait_No;
	SDIO_CmdInitStructure.SDIO_CPSM = SDIO_CPSM_Enable;
	SDIO_SendCommand(&SDIO_CmdInitStructure);

	errorstatus = CmdResp1Error(SD_CMD_SET_BLOCKLEN);
	if (errorstatus != SD_OK) {
		return(errorstatus);
	}

	return 0;	
}

//检测sd卡插入后是否有接触不良等故障
int  sd_insert_trouble_test(void)
{
	int ret;
	uint8_t __attribute__((aligned(4))) temp[SD_BLK_SIZE];  //开辟512字节的栈

	//读一块数据看能否成功即可判断是否有SD插入后接触不良等故障
	ret = SD_ReadBlock(temp, 0x00, SD_BLK_SIZE);
	if( ret ) {
		return -ret;
	}
//	sdio_controller_power_save(ENABLE);  //开启省电模式, 仅在有总线活动时才输出SDIO_CK

	return 0;
}

//最新初始化函数, 替代sd_probe  2023-12-26
int sd_init(void)
{
	int ret;

	ret = sd_detect();
	if(ret != 0)
	{
		my_printf("Error! Can't detect SD Card! ret=%d!\n\r", ret);
		return -1;
	}

	ret = sd_bus_width_speed_config();
	if(ret != 0)
	{
		my_printf("Error! SD bus width speed config fail, ret=%d\n\r", ret);
		return -3;		
	}

	ret = sd_block_512B_config();
	if(ret != 0)
	{
		my_printf("Error! SD block set 512B fail, ret=%d\n\r", ret);
		return -4;
	}

	ret = sd_insert_trouble_test();
	if(ret != 0)
	{
		my_printf("Error! SD Insert Error, ret=%d\n\r", ret);
		return -5;
	}

	return 0;
}






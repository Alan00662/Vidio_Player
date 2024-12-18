/**
  ******************************************************************************
  * @file    usbd_usr.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    19-March-2012
  * @brief   This file includes the user application layer
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
#include "usbd_usr.h"

#include "my_printf.h"



/** @defgroup USBD_USR_Private_Variables
  * @{
  */ 
/*  Points to the DEVICE_PROP structure of current device */
/*  The purpose of this register is to speed up the execution */

const USBD_Usr_cb_TypeDef USR_FS_cb =
{
	USBD_USR_Init,              //USBD_Init()-->USBD_USR_Init()(pdev->dev.usr_cb->Init)  
	USBD_USR_DeviceReset,       //DCD_HandleUsbReset_ISR()-->USBD_Reset()-->USBD_USR_DeviceReset()
	USBD_USR_DeviceConfigured,  //xxxx->USBD_SetupStage()->USBD_StdDevReq()-->USBD_SetConfig()-->USBD_SetCfg()-->USBD_USR_DeviceConfigured()
	USBD_USR_DeviceSuspended,   //DCD_HandleUSBSuspend_ISR()-->USBD_Suspend()-->USBD_USR_DeviceSuspended()
	USBD_USR_DeviceResumed,     //DCD_HandleResume_ISR()-->USBD_Resume()-->USBD_USR_DeviceResumed()
	USBD_USR_DeviceConnected,     //DCD_SessionRequest_ISR()-->USBD_DevConnected()-->USBD_USR_DeviceConnected() (When defien VBUS_SENSING_ENABLED)
	USBD_USR_DeviceDisconnected,  //DCD_OTG_ISR()-->USBD_DevDisconnected()-->USBD_USR_DeviceDisconnected()      (When defien VBUS_SENSING_ENABLED)
};


void USBD_USR_Init(void)
{
	my_printf("[%s]\n\r", __FUNCTION__);
}

/**
* @brief  Displays the message on LCD on device reset event
* @param  speed : device speed
* @retval None
*/
void USBD_USR_DeviceReset (uint8_t speed)
{
	switch (speed)
	{
		case USB_OTG_SPEED_HIGH: 
			my_printf("[%s] HS\n\r", __FUNCTION__);
			break;

		case USB_OTG_SPEED_FULL: 
			my_printf("[%s] FS\n\r", __FUNCTION__);
			break;
		default:
			my_printf("[%s] ??\n\r", __FUNCTION__);
	}
}


/**
* @brief  Displays the message on LCD on device config event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConfigured (void)
{
	my_printf("[%s]\n\r", __FUNCTION__);
}

/**
* @brief  Displays the message on LCD on device suspend event 
* @param  None
* @retval None
*/
void USBD_USR_DeviceSuspended(void)
{
	my_printf("[%s]\n\r", __FUNCTION__);
}


/**
* @brief  Displays the message on LCD on device resume event
* @param  None
* @retval None
*/
void USBD_USR_DeviceResumed(void)
{
	my_printf("[%s]\n\r", __FUNCTION__);
}

/**
* @brief  USBD_USR_DeviceConnected
*         Displays the message on LCD on device connection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceConnected (void)
{
	my_printf("[%s]\n\r", __FUNCTION__);
}


/**
* @brief  USBD_USR_DeviceDisonnected
*         Displays the message on LCD on device disconnection Event
* @param  None
* @retval Staus
*/
void USBD_USR_DeviceDisconnected (void)
{
	my_printf("[%s]\n\r", __FUNCTION__);
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

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*
 * CAN_API.c
 *
 *  Created on: Sep 5, 2025
 *      Author: q
 */
#include "CAN_API.h"

extern CAN_HandleTypeDef hcan1;


CAN_GlobalState_t CAN_State = {0,0,0};

CAN_RxHeaderTypeDef g_RxHeader;
uint8_t g_RxData[8];


void CAN1_Tx(uint16_t ID, uint8_t DLC, uint8_t *Data, uint32_t CAN_RTR)
{

//	CAN_RTR_DATA                (0x00000000U)  /*!< Data frame   */
//	CAN_RTR_REMOTE              (0x00000002U)  /*!< Remote frame */

	CAN_TxHeaderTypeDef TxHeader;

	TxHeader.StdId = ID;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.DLC = DLC;
	TxHeader.RTR = CAN_RTR;

	uint32_t TxMailbox;

	if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) != 0)
	{
		HAL_CAN_AddTxMessage(&hcan1, &TxHeader, Data, &TxMailbox);
	}

}

void CAN_Init_FilterConfig(void)
{
	  CAN_FilterTypeDef canfilterconfig;

	  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE; // Включить фильтр
	  canfilterconfig.FilterBank = 0; // Используем 0-й банк фильтров
	  canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0; // Направлять в FIFO0
	  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK; // Режим маски (для точного совпадения маска будет из всех единиц)
	  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32-битный масштаб
	  canfilterconfig.SlaveStartFilterBank = 14;

	  // Чтобы поймать ТОЛЬКО ID 0x321, мы ставим его и в ID, и в маску.
	  // Но ID нужно сдвинуть влево на 5 бит, т.к. стандартный ID занимает старшие биты в 16-битном поле.
	  // Для 32-битного фильтра стандартный ID сдвигается на 21 бит.
//	  canfilterconfig.FilterIdHigh = 0x101 << 5;
//	  canfilterconfig.FilterIdLow = 0x0000;
//
//
//	  canfilterconfig.FilterMaskIdHigh = 0x101 << 5;
//	  canfilterconfig.FilterMaskIdLow = 0x0000;

//	   Альтернативный и более простой способ для точного совпадения - режим списка

	    uint32_t filter_id   = (0x100 << 21);
	    uint32_t filter_mask = (0x7F8 << 21);

	    canfilterconfig.FilterIdHigh =       (filter_id >> 16) & 0xFFFF;
	    canfilterconfig.FilterIdLow =         filter_id & 0xFFFF;
	    canfilterconfig.FilterMaskIdHigh =   (filter_mask >> 16) & 0xFFFF;
	    canfilterconfig.FilterMaskIdLow =     filter_mask & 0xFFFF;
	  // Применяем конфигурацию
	  if (HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig) != HAL_OK)
	  {
	      Error_Handler();
	  }

}

void CAN1_Rx(uint8_t *Data)
{
  CAN_RxHeaderTypeDef RxHeader;


  while(! HAL_CAN_GetRxFifoFillLevel(&hcan1,CAN_RX_FIFO0));

  if(HAL_CAN_GetRxMessage(&hcan1,CAN_RX_FIFO0,&RxHeader,Data) != HAL_OK)
  {
	  Error_Handler();
  }


}

/**
  * @brief  Callback-функция, вызываемая по прерыванию при поступлении нового сообщения в FIFO0.
  * @param  hcan: указатель на структуру CAN_HandleTypeDef.
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &g_RxHeader, g_RxData) == HAL_OK)
    {
    	if(CAN_State.CAN_NumberRX < 0xFF)
    	{
        	CAN_State.CAN_NumberRX++; // счётчик принятых сообщений
    	}

    }
}

/**
  * @brief  Callback-функция, вызываемая по прерыванию при возникновении ошибки CAN.
  * @param  hcan: указатель на структуру CAN_HandleTypeDef.
  * @retval None
  */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    // Включаем светодиод для индикации ошибки
    HAL_GPIO_WritePin(CAN_ERROR_LED_PORT, CAN_ERROR_LED_PIN, GPIO_PIN_SET);

    // Пример обработки ошибки Bus-Off: попытка перезапустить CAN контроллер
    if ((hcan->ErrorCode & HAL_CAN_ERROR_BOF) != 0)
    {
        // Здесь можно реализовать логику сброса CAN-периферии
        // Например, HAL_CAN_ResetError(hcan) или полная реинициализация.
//    	HAL_CAN_Stop(hcan);
//    	HAL_Delay(10);
//    	HAL_CAN_DeInit(&hcan1);
//    	HAL_Delay(10);
//    	HAL_CAN_Init(&hcan1);

    }
}




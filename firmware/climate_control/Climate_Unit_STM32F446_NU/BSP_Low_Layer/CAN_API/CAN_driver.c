/*
 * CAN_API.c
 *
 *  Created on: Sep 5, 2025
 *      Author: q
 */
#include "../../BSP_Low_Layer/CAN_API/CAN_driver.h"


static CAN_Config_t CAN_Config;
CAN_GlobalState_t CAN_State = {0,0,0};

CAN_RxHeaderTypeDef g_RxHeader;
uint8_t g_RxData[8];


void CAN_Driver_Init(CAN_HandleTypeDef *hcan, can_rx_callback_t rx_callback)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);

	CAN_Config.hcan = hcan;
	CAN_Config.rx_callback = rx_callback;

	if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_ERROR_PASSIVE | CAN_IT_ERROR_WARNING) != HAL_OK)
	{
		Error_Handler();
	}

	CAN_Init_FilterConfigList(hcan, 0x102, 1); // Фильтр по ID 0x102 в банк 1

	if(HAL_CAN_Start(hcan) == HAL_ERROR)
	{
		  Error_Handler();
	}
}

/*
 * Добавление фильтра по маске
 *
 * Активен только FilterBank 0
 */
void CAN_Driver_AddFilterMask(CAN_HandleTypeDef *hcan, uint16_t low_id, uint16_t high_id)
{
	  CAN_FilterTypeDef canfilterconfig;

	  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE; // Включить фильтр
	  canfilterconfig.FilterBank = 0; // Используем 0-й банк фильтров
	  canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0; // Направлять в FIFO0
	  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK; // Режим маски (для точного совпадения маска будет из всех единиц)
	  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32-битный масштаб
	  canfilterconfig.SlaveStartFilterBank = 14;

	  uint32_t filter_id   = ((low_id & 0x7FF) << 21); // Сдвигаем ID в позицию
	  uint32_t filter_mask = ~((high_id & 0x7FF) << 21); // Инвертируем маску для приёма в диапазоне от low_id до high_id

	  canfilterconfig.FilterIdHigh =       (filter_id >> 16) & 0xFFFF;
	  canfilterconfig.FilterIdLow =         filter_id & 0xFFFF;
	  canfilterconfig.FilterMaskIdHigh =   (filter_mask >> 16) & 0xFFFF;
	  canfilterconfig.FilterMaskIdLow =     filter_mask & 0xFFFF;


	  if (HAL_CAN_ConfigFilter(hcan, &canfilterconfig) != HAL_OK)
	  {
	      Error_Handler();
	  }

}
/*
 * Добавление фильтра по списку ID
 *
 * FilterNank 0 - зарезервирован для маски
 */

void CAN_Init_FilterConfigList(CAN_HandleTypeDef *hcan, uint16_t id, uint8_t FilterBank)
{
	  CAN_FilterTypeDef canfilterconfig;

	  if(!FilterBank)
	  {
		  Error_Handler();
	  }

	  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE; // Включить фильтр
	  canfilterconfig.FilterBank = FilterBank; // Используемый банк фильтров
	  canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0; // Направлять в FIFO0
	  canfilterconfig.FilterMode = CAN_FILTERMODE_IDLIST; // Режим маски (для точного совпадения маска будет из всех единиц)
	  canfilterconfig.FilterScale = CAN_FILTERSCALE_16BIT; // 16-битный масштаб
	  canfilterconfig.SlaveStartFilterBank = 14;

	  canfilterconfig.FilterIdHigh = id << 5;
	  canfilterconfig.FilterIdLow = 0x0000;

	  canfilterconfig.FilterMaskIdHigh = id << 5;
	  canfilterconfig.FilterMaskIdLow = 0x0000;

	  // Применяем конфигурацию
	  if (HAL_CAN_ConfigFilter(hcan, &canfilterconfig) != HAL_OK)
	  {
	      Error_Handler();
	  }

}

/** @defgroup CAN_remote_transmission_request CAN Remote Transmission Request
  * @{
  * CAN_RTR_DATA                (0x00000000U)  < Data frame
   *CAN_RTR_REMOTE              (0x00000002U)  < Remote frame
  */
HAL_StatusTypeDef CAN_Driver_Transmit(uint16_t id, uint8_t *data, uint8_t dlc, uint32_t FRAME_RTR)
{

	CAN_TxHeaderTypeDef TxHeader;

	TxHeader.StdId = id & 0x7FF;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.DLC = dlc;
	TxHeader.RTR = FRAME_RTR;

	uint32_t TxMailbox;

	if(HAL_CAN_GetTxMailboxesFreeLevel(CAN_Config.hcan) != 0)
	{
		return HAL_CAN_AddTxMessage(CAN_Config.hcan, &TxHeader, data, &TxMailbox);
	}

	return HAL_ERROR;

}

#ifdef USE_LOOP_CAN_RX

void CAN1_Rx(uint8_t *Data)
{
  CAN_RxHeaderTypeDef RxHeader;


  while(! HAL_CAN_GetRxFifoFillLevel(&hcan1,CAN_RX_FIFO0));

  if(HAL_CAN_GetRxMessage(&hcan1,CAN_RX_FIFO0,&RxHeader,Data) != HAL_OK)
  {
	  Error_Handler();
  }


}
#endif

/**
  * @brief  Callback-функция, вызываемая по прерыванию при поступлении нового сообщения в FIFO0.
  * @param  hcan: указатель на структуру CAN_HandleTypeDef.
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if (hcan->Instance == CAN_Config.hcan->Instance)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK)
        {
            CAN_State.CAN_NumberRX++;
            if (CAN_Config.rx_callback != NULL)
            {
                CAN_Config.rx_callback(rx_header.StdId, rx_data, rx_header.DLC);
            }
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




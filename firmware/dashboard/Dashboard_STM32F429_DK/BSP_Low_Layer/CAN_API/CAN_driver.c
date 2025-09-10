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

	if(HAL_CAN_Start(hcan) == HAL_ERROR)
	{
		  Error_Handler();
	}

	if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_ERROR_PASSIVE | CAN_IT_ERROR_WARNING) != HAL_OK)
	{
		Error_Handler();
	}
}

/*
 * Добавление фильтра по маске
 *
 * Активен только FilterBank 0
 * if ( (Входящий_ID & Маска) == (ID_Фильтра & Маска) ) then { Пропустить сообщение }
 * Бит в Маске, равный 1, означает: "Бит во входящем ID в этой позиции должен совпадать с битом в ID Фильтра".
 * Бит в Маске, равный 0, означает: "Мне все равно, какой бит во входящем ID в этой позиции. Я его игнорирую".
 *
 * Добавление фильтра по маске для приема диапазона ID [low_id, high_id]
 * ВАЖНО: Диапазон должен быть "выровнен" по степеням двойки для корректной работы.
 * Например, [0x100, 0x103] (4 ID) или [0x100, 0x107] (8 ID) будет работать идеально.
 * Диапазон [0x101, 0x106] будет пропускать лишние ID, т.к. маска не сможет его точно описать.
 */
void CAN_Driver_AddFilterMask(CAN_HandleTypeDef *hcan, uint16_t low_id, uint16_t high_id)
{
	  CAN_FilterTypeDef canfilterconfig;

	  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
	  canfilterconfig.FilterBank = 0; // Используем 0-й банк фильтров
	  canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0;
	  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
	  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
	  canfilterconfig.SlaveStartFilterBank = 14; // Для F4, указывает с какого банка начинаются фильтры для CAN2

      // --- ПРАВИЛЬНАЯ ЛОГИКА ---

      // 1. Убедимся, что работаем только с 11 битами стандартного ID
      uint32_t low_id_32 = low_id & 0x7FF;
      uint32_t high_id_32 = high_id & 0x7FF;

      // 2. Находим биты, которые РАЗЛИЧАЮТСЯ между low и high
      uint32_t diff_bits = low_id_32 ^ high_id_32;

      // 3. Инвертируем, чтобы получить маску, где '1' - это биты, которые должны СОВПАДАТЬ
      uint32_t filter_mask_32 = ~diff_bits;

      // 4. ID фильтра устанавливаем по нижней границе
      uint32_t filter_id_32 = low_id_32;

      // 5. Сдвигаем 11-битные ID и маску в 32-битные регистры фильтра.
      //    Для стандартного ID (STID) данные должны быть в старших битах.
      uint32_t final_filter_id = filter_id_32 << 21;
      uint32_t final_mask = (filter_mask_32 << 21) | 0x001FFFFF; // Маскируем также IDE, RTR и EXID биты, чтобы они не влияли

	  canfilterconfig.FilterIdHigh =       (0b100000000<< 5) & 0xFFFF;
	  canfilterconfig.FilterIdLow =        0; //final_filter_id & 0xFFFF;
	  canfilterconfig.FilterMaskIdHigh =   (0b111111 << 8) & 0xFFFF;
	  canfilterconfig.FilterMaskIdLow =   0; // final_mask << 0xFFFF;

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

void CAN_Driver_AddFilterList(CAN_HandleTypeDef *hcan, uint16_t id, uint8_t FilterBank)
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




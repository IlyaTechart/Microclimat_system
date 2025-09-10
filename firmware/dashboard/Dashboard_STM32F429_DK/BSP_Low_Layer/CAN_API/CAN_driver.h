/*
 * CAN_API.h
 *
 *  Created on: Sep 5, 2025
 *      Author: q
 */

#ifndef CAN_API_CAN_DRIVER_H_
#define CAN_API_CAN_DRIVER_H_

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"
#include "stm32f4xx_hal_gpio.h"

#define CAN_ERROR_LED_PORT GPIOA
#define CAN_ERROR_LED_PIN  GPIO_PIN_5


extern void Error_Handler(void);

// Указатель на функцию, которую вызовет драйвер при получении сообщения
typedef void (*can_rx_callback_t)(uint16_t id, uint8_t *data, uint8_t dlc);

typedef struct
{
	uint8_t CAN_NumberRX;
	uint8_t CAN_NumberTX;
	uint8_t CAN_ErrorFlag;

}CAN_GlobalState_t;

typedef struct
{
	CAN_HandleTypeDef *hcan;
	can_rx_callback_t rx_callback;

}CAN_Config_t;


void CAN_Driver_Init(CAN_HandleTypeDef *hcan, can_rx_callback_t rx_callback);
void CAN_Driver_AddFilterMask(CAN_HandleTypeDef *hcan, uint16_t low_id, uint16_t high_id);
void CAN_Driver_AddFilterList(CAN_HandleTypeDef *hcan, uint16_t id, uint8_t filter_bank);
HAL_StatusTypeDef CAN_Driver_Transmit(uint16_t id, uint8_t *data, uint8_t dlc, uint32_t FRAME_RTR);

#ifdef USE_LOOP_CAN_RX
void CAN1_Rx(uint8_t *Data);
#endif
// Вызываются из прерываний в stm32f4xx_it.c
void CAN_Driver_RxPendingCallback(void);
void CAN_Driver_ErrorCallback(void);



#endif /* CAN_API_CAN_DRIVER_H_ */

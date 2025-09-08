/*
 * CAN_API.h
 *
 *  Created on: Sep 5, 2025
 *      Author: q
 */

#ifndef CAN_API_CAN_API_H_
#define CAN_API_CAN_API_H_

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"

#define CAN_ERROR_LED_PORT GPIOA
#define CAN_ERROR_LED_PIN  GPIO_PIN_5

extern void Error_Handler(void);




typedef struct
{
	uint8_t CAN_NumberRX;
	uint8_t CAN_NumberTX;
	uint8_t CAN_ErrorFlag;

}CAN_GlobalState_t;





void CAN1_Tx(uint16_t ID, uint8_t *Data, uint8_t DLC, uint32_t CAN_RTR);
void CAN_Init_FilterConfig(void);
void CAN1_Rx(uint8_t *Data);



#endif /* CAN_API_CAN_API_H_ */

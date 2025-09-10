/*
 * CAN_protocol.c
 *
 *  Created on: Sep 10, 2025
 *      Author: q
 */

#include "CAN_protocol.h"
#include <string.h>


CAN_MessageQueue_t CAN_RxQueue = { .head = 0, .tail = 0, .count = 0 };


void CAN_Protocol_Init(CAN_HandleTypeDef *hcan)
{
	CAN_Driver_Init(hcan, CAN_Protocol_onMessage);
}


void CAN_Protocol_ProcessQueue(void)
{

	while(CAN_RxQueue.count > 0)
	{
		CAN_Message_t local_msg;

	    // Атомарно изменяем переменные, к которым есть доступ из прерывания
	    __disable_irq(); // Отключаем прерывания на пару тактов
		memcpy(&local_msg, &CAN_RxQueue.messages[CAN_RxQueue.tail], sizeof(CAN_Message_t));
	    CAN_RxQueue.tail = (CAN_RxQueue.tail + 1) % COUNT_QUEUE_MESSAGES;
	    CAN_RxQueue.count--;
	    __enable_irq(); // Включаем обратно

		switch (local_msg.id) {
			case 0x102:
				HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);
				break;
			case 0x106:
				HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
				break;

			default:
				break;
		}
	}

}

void CAN_Protocol_onMessage(uint16_t id, uint8_t *data, uint8_t dlc)
{
	if(CAN_RxQueue.count >= COUNT_QUEUE_MESSAGES)
	{
		return;
	}

	CAN_Message_t *msg = &CAN_RxQueue.messages[CAN_RxQueue.head];

	msg->id = id;
	msg->dlc = dlc;
	memcpy(msg->data, data, dlc);
	CAN_RxQueue.head = (CAN_RxQueue.head + 1) % COUNT_QUEUE_MESSAGES;

	CAN_RxQueue.count++;

}


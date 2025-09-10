/*
 * CAN_protocol.h
 *
 *  Created on: Sep 10, 2025
 *      Author: q
 */

#ifndef CAN_PROTOCOL_CAN_PROTOCOL_H_
#define CAN_PROTOCOL_CAN_PROTOCOL_H_

#include "../../BSP_Low_Layer/CAN_API/CAN_driver.h"


#define ID_SENSOR_CO2 		  0x101
#define ID_SENSOR_BME280_1    0x102
#define ID_SENSOR_BME280_2    0x103  // reserved
#define ID_PWM_FAN_1          0x104



#define COUNT_QUEUE_MESSAGES 10

typedef struct {
    uint16_t id;
    uint8_t  data[8];
    uint8_t  dlc;
} CAN_Message_t;

typedef struct {
	CAN_Message_t messages[COUNT_QUEUE_MESSAGES]; // Очередь на 10 сообщений
	uint8_t head;
	uint8_t tail;
	uint8_t count;
} CAN_MessageQueue_t;

// Функция, которую нужно периодически вызывать из main()
void CAN_Protocol_ProcessQueue(void);

// Callback для драйвера остается, но теперь он будет очень простым
void CAN_Protocol_onMessage(uint16_t id, uint8_t *data, uint8_t dlc);

#endif /* CAN_PROTOCOL_CAN_PROTOCOL_H_ */

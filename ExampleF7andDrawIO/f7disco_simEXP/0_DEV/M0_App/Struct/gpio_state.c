/*
 * gpio_state.c
 *
 *  Created on: Apr 18, 2025
 *      Author: CAO HIEU
 */

#include "main.h"
#include "Macro/macro.h"
#include "gpio_state.h"

void toOBC_Init(void) {
	GPIO_SetHigh(BUSY_GPIO_Port, BUSY_Pin);
	GPIO_SetHigh(READYSEND_GPIO_Port, READYSEND_Pin);
    toOBC_SetState(toOBC_IDLE);
}

void toOBC_SetState(toOBC_State_t state) {
    switch (state) {
        case toOBC_ERROR:
            GPIO_SetLow(BUSY_GPIO_Port, BUSY_Pin);
            GPIO_SetLow(READYSEND_GPIO_Port, READYSEND_Pin);
            break;
        case toOBC_READYSEND:
            GPIO_SetLow(BUSY_GPIO_Port, BUSY_Pin);
            GPIO_SetHigh(READYSEND_GPIO_Port, READYSEND_Pin);
            break;
        case toOBC_BUSY:
            GPIO_SetHigh(BUSY_GPIO_Port, BUSY_Pin);
            GPIO_SetLow(READYSEND_GPIO_Port, READYSEND_Pin);
            break;
        case toOBC_IDLE:
            GPIO_SetHigh(BUSY_GPIO_Port, BUSY_Pin);
            GPIO_SetHigh(READYSEND_GPIO_Port, READYSEND_Pin);
            break;
        default:
            GPIO_SetLow(BUSY_GPIO_Port, BUSY_Pin);
            GPIO_SetLow(READYSEND_GPIO_Port, READYSEND_Pin);
            break;
    }
}

toOBC_State_t toOBC_GetState(void) {
    uint8_t busy_state = GPIO_IsOutHigh(BUSY_GPIO_Port, BUSY_Pin);
    uint8_t readysend_state = GPIO_IsOutHigh(READYSEND_GPIO_Port, READYSEND_Pin);

    if (busy_state == 0 && readysend_state == 0) {
        return toOBC_ERROR;
    } else if (busy_state == 0 && readysend_state == 1) {
        return toOBC_READYSEND;
    } else if (busy_state == 1 && readysend_state == 0) {
        return toOBC_BUSY;
    } else { // busy_state == 1 && readysend_state == 1
        return toOBC_IDLE;
    }
}

void CPROCESS_Init(void) {
    GPIO_SetHigh(CPROCESS_GPIO_Port, CPROCESS_Pin);
}

void CPROCESS_SetState(CPROCESS_State_t state) {
    if (state == CPROCESS_FREE) {
        GPIO_SetHigh(CPROCESS_GPIO_Port, CPROCESS_Pin);
    } else { // CPROCESS_BUSY
        GPIO_SetLow(CPROCESS_GPIO_Port, CPROCESS_Pin);
    }
}

CPROCESS_State_t CPROCESS_GetState(void) {
    if (GPIO_IsOutHigh(CPROCESS_GPIO_Port, CPROCESS_Pin)) {
        return CPROCESS_FREE;
    } else {
        return CPROCESS_BUSY;
    }
}

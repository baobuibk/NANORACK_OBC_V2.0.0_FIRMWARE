/*
 * min_process.c
 *
 *  Created on: Apr 18, 2025
 *      Author: CAO HIEU
 */
#include "main.h"

#include "MIN_R01/min_proto.h"
#include "Logger/log.h"
#include "UART/uart_driver.h"
#include "Struct/gpio_state.h"

MIN_Context_t F7_MinCtx;

void MIN_Timeout_Handler(MIN_Context_t *ctx) {
    LOG("MIN-Timeout!");
}

void MIN_Process_Init(void){
	MIN_Context_Init(&F7_MinCtx, F7Disco_PORT);
	CPROCESS_Init();
	MIN_RegisterTimeoutCallback(&F7_MinCtx, MIN_Timeout_Handler);
	LOG("MIN Process Init!");
}

void MIN_Processing(void){
    while (UART_Driver_IsDataAvailable(USART6)) {
        int data = UART_Driver_Read(USART6);
        if (data >= 0) {
            uint8_t byte = (uint8_t)data;
            MIN_App_Poll(&F7_MinCtx, &byte, 1);
            CPROCESS_SetState(CPROCESS_BUSY);
        }
    }
	MIN_App_Poll(&F7_MinCtx, NULL, 0);
}

/*
 * min_process.c
 *
 *  Created on: Apr 18, 2025
 *      Author: CAO HIEU
 */
#include "main.h"
#include "board.h"
#include "uart_driver_dma.h"
#include "SysLog/syslog.h"
#include "min_proto.h"
#include "min_app/min_command.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "min_process.h"
#include "stdio.h"

MIN_Context_t OBC_MinCtx;

static SemaphoreHandle_t responseSemaphore;
static QueueHandle_t pendingCommandsQueue;

typedef struct {
    uint8_t cmdId;
    uint8_t expectedResponseId;
} CommandInfo_t;

void MIN_ResponseCallback(uint8_t min_id, const uint8_t *payload, uint8_t len) {
    CommandInfo_t cmdInfo;
    if (xQueuePeek(pendingCommandsQueue, &cmdInfo, 0) == pdTRUE) {
        if (min_id == cmdInfo.expectedResponseId) {
            xQueueReceive(pendingCommandsQueue, &cmdInfo, 0);
            xSemaphoreGive(responseSemaphore);
        }
    }
}

static void ClearPendingCommand(void) {
    CommandInfo_t cmdInfo;
    if (xQueueReceive(pendingCommandsQueue, &cmdInfo, 0) == pdTRUE) {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Cleared pending command: ID 0x%02X, Expected 0x%02X\r\n",
                 cmdInfo.cmdId, cmdInfo.expectedResponseId);
        UART_Driver_SendString(UART_DEBUG, buffer);
    }
}

void MIN_Timeout_Handler(MIN_Context_t *ctx) {
    SYSLOG_ERROR_POLL("MIN-Timeout!");
}

void MIN_Process_Init(void){
	MIN_Context_Init(&OBC_MinCtx, EXP_PORT);
	MIN_RegisterTimeoutCallback(&OBC_MinCtx, MIN_Timeout_Handler);

	responseSemaphore = xSemaphoreCreateBinary();
	pendingCommandsQueue = xQueueCreate(10, sizeof(CommandInfo_t));
	MIN_RegisterResponseHandler(MIN_ResponseCallback);

	Sys_Boardcast(E_OK, LOG_INFOR, "MIN Process Init!");
}

void MIN_Processing(void){
    while (UART_DMA_Driver_IsDataAvailable(UART_EXP)) {
        int data = UART_DMA_Driver_Read(UART_EXP);
        if (data >= 0) {
            uint8_t byte = (uint8_t)data;
            MIN_App_Poll(&OBC_MinCtx, &byte, 1);
        }
    }
	MIN_App_Poll(&OBC_MinCtx, NULL, 0);
}

// =================================================================
// Command Sending Functions
// =================================================================

void Min_Send_CONTROL_TEMP_CMD(uint8_t temperature) {
    uint8_t payload[1] = {temperature};
    MIN_Send(&OBC_MinCtx, CONTROL_TEMP_CMD, payload, 1);

    CommandInfo_t cmdInfo = {CONTROL_TEMP_CMD, CONTROL_TEMP_ACK};
    xQueueSend(pendingCommandsQueue, &cmdInfo, portMAX_DELAY);

    if (xSemaphoreTake(responseSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        SYSLOG_NOTICE("Response OK - CONTROL_TEMP_CMD");
    } else {
        SYSLOG_ERROR("Timeout CONTROL_TEMP_CMD");
        ClearPendingCommand();
    }
}

void Min_Send_COLLECT_DATA(uint32_t sample) {

    uint8_t payload[4] = {0};
    // Big-endian packing [3210]
    payload[3] = (uint8_t)((sample >> 24) & 0xFF);
    payload[2] = (uint8_t)((sample >> 16) & 0xFF);
    payload[1] = (uint8_t)((sample >> 8	) & 0xFF);
    payload[0] = (uint8_t)((sample & 0xFF));

    MIN_Send(&OBC_MinCtx, COLLECT_DATA_CMD, payload, sizeof(payload));

    CommandInfo_t cmdInfo = {COLLECT_DATA_CMD, DONE};
    xQueueSend(pendingCommandsQueue, &cmdInfo, portMAX_DELAY);

    if (xSemaphoreTake(responseSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        SYSLOG_NOTICE("Response OK - COLLECT_DATA");
    } else {
        SYSLOG_ERROR("Timeout COLLECT_DATA");
        ClearPendingCommand();
    }
}

void Min_Send_PRE_DATA(uint16_t chunk_size) {
    uint8_t payload[2] = {0};

    payload[1] = (uint8_t)((chunk_size >> 8) & 0xFF);
    payload[0] = (uint8_t)((chunk_size & 0xFF));

    MIN_Send(&OBC_MinCtx, PRE_DATA_CMD, payload, sizeof(payload));

    CommandInfo_t cmdInfo = {PRE_DATA_CMD, PRE_DATA_ACK};
    xQueueSend(pendingCommandsQueue, &cmdInfo, portMAX_DELAY);

    if (xSemaphoreTake(responseSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        SYSLOG_NOTICE("Response OK - PRE_DATA_CMD");
    } else {
        SYSLOG_ERROR("Timeout PRE_DATA_CMD");
        ClearPendingCommand();
    }
}

void Min_Send_PRE_CHUNK(uint8_t chunk) {
    uint8_t payload[1] = {chunk};
    MIN_Send(&OBC_MinCtx, PRE_CHUNK_CMD, payload, 1);

    CommandInfo_t cmdInfo = {PRE_CHUNK_CMD, PRE_CHUNK_ACK};
    xQueueSend(pendingCommandsQueue, &cmdInfo, portMAX_DELAY);

    if (xSemaphoreTake(responseSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        SYSLOG_NOTICE("Response OK - PRE_CHUNK_CMD");
    } else {
        SYSLOG_ERROR("Timeout PRE_CHUNK_CMD");
        ClearPendingCommand();
    }
}

void Min_Send_SAMPLERATE_SET(uint32_t sample_rate) {
    uint8_t payload[4] = {0};

    payload[3] = (uint8_t)((sample_rate >> 24) & 0xFF);
    payload[2] = (uint8_t)((sample_rate >> 16) & 0xFF);
    payload[1] = (uint8_t)((sample_rate >> 8) & 0xFF);
    payload[0] = (uint8_t)(sample_rate & 0xFF);

    MIN_Send(&OBC_MinCtx, SAMPLERATE_SET_CMD, payload, sizeof(payload));

    CommandInfo_t cmdInfo = {SAMPLERATE_SET_CMD, DONE};
    xQueueSend(pendingCommandsQueue, &cmdInfo, portMAX_DELAY);

    if (xSemaphoreTake(responseSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        SYSLOG_NOTICE("Response OK - SAMPLERATE_SET_CMD");
    } else {
        SYSLOG_ERROR("Timeout SAMPLERATE_SET_CMD");
        ClearPendingCommand();
    }
}

void Min_Send_SAMPLERATE_GET(void) {
    MIN_Send(&OBC_MinCtx, SAMPLERATE_GET_CMD, NULL, 0);

    CommandInfo_t cmdInfo = {SAMPLERATE_GET_CMD, SAMPLERATE_GET_ACK};
    xQueueSend(pendingCommandsQueue, &cmdInfo, portMAX_DELAY);

    if (xSemaphoreTake(responseSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        SYSLOG_NOTICE("Response OK - SAMPLERATE_GET_CMD");
    } else {
        SYSLOG_ERROR("Timeout SAMPLERATE_GET_CMD");
        ClearPendingCommand();
    }
}

void Min_Send_COLLECT_PACKAGE(void) {
    MIN_Send(&OBC_MinCtx, COLLECT_PACKAGE_CMD, NULL, 0);

    CommandInfo_t cmdInfo = {COLLECT_PACKAGE_CMD, COLLECT_PACKAGE_ACK};
    xQueueSend(pendingCommandsQueue, &cmdInfo, portMAX_DELAY);

    if (xSemaphoreTake(responseSemaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
        SYSLOG_NOTICE("Response OK - COLLECT_PACKAGE_CMD");
    } else {
        SYSLOG_ERROR("Timeout COLLECT_PACKAGE_CMD");
        ClearPendingCommand();
    }
}

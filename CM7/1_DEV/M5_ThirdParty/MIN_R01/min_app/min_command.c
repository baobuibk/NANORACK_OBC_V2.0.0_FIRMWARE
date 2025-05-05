/*
 * min_command.c
 *
 *  Created on: Apr 22, 2025
 *      Author: CAO HIEU
 */

#include "min_command.h"
#include <string.h>
#include "stdio.h"
#include "uart_driver_dma.h"
#include "board.h"

// =================================================================
// Command Handlers
// =================================================================

static void MIN_Handler_CONTROL_TEMP_ACK(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    char buffer[256];
    int offset = 0;

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Payload (%u bytes):", len);

    for (uint8_t i = 0; i < len && offset < sizeof(buffer) - 4; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %02X", payload[i]);
    }

    snprintf(buffer + offset, sizeof(buffer) - offset, "\r\n");
    UART_Driver_SendString(UART_DEBUG, buffer);

    snprintf(buffer, sizeof(buffer), "Message: \"%s\"\r\n", payload);

    UART_Driver_SendString(UART_DEBUG, buffer);

    MIN_Send(ctx, CONTROL_TEMP_ACK, NULL, 0);
}

static void MIN_Handler_PRE_DATA(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    char buffer[256];
    int offset = 0;

    if (len < 1) {
        snprintf(buffer, sizeof(buffer), "Invalid payload length.\r\n");
        UART_Driver_SendString(UART_DEBUG, buffer);
        return;
    }

    uint8_t num_chunks = payload[0];
    if (len != 1 + 2 * num_chunks) {
        snprintf(buffer, sizeof(buffer),
                 "Payload length mismatch. Expected %u bytes, got %u.\r\n",
                 1 + 2 * num_chunks, len);
        UART_Driver_SendString(UART_DEBUG, buffer);
        return;
    }

    snprintf(buffer, sizeof(buffer), "Pre Data Success: %u Chunks\r\n", num_chunks);
    UART_Driver_SendString(UART_DEBUG, buffer);

    offset = 0;
    for (uint8_t i = 0; i < num_chunks; i++) {
        uint16_t crc = (payload[1 + 2 * i] << 8) | payload[2 + 2 * i];
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                           "%u -> CRC16: 0x%04X\r\n", i, crc);
        if (offset >= sizeof(buffer) - 20) {
            UART_Driver_SendString(UART_DEBUG, buffer);
            offset = 0;
        }
    }

    if (offset > 0) {
        UART_Driver_SendString(UART_DEBUG, buffer);
    }
    MIN_Send(ctx, PRE_DATA_ACK, NULL, 0);
}

static void MIN_Handler_PRE_CHUNK(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    char buffer[256];
    int offset = 0;

    if (len != 2) {
        snprintf(buffer, sizeof(buffer), "Invalid payload length. Expected 2 bytes, got %u.\r\n", len);
        UART_Driver_SendString(UART_DEBUG, buffer);
        return;
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Payload (%u bytes):", len);
    for (uint8_t i = 0; i < len && offset < sizeof(buffer) - 4; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %02X", payload[i]);
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "\r\n");
    UART_Driver_SendString(UART_DEBUG, buffer);

    uint16_t crc = (payload[0] << 8) | payload[1];
    snprintf(buffer, sizeof(buffer), "Chunk CRC16: 0x%04X\r\n", crc);
    UART_Driver_SendString(UART_DEBUG, buffer);
    MIN_Send(ctx, PRE_CHUNK_ACK, NULL, 0);
}

static void MIN_Handler_COLLECT_PACKAGE(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    char buffer[256];
    int offset = 0;

    if (len != 2) {
        snprintf(buffer, sizeof(buffer), "Invalid payload length. Expected 2 bytes, got %u.\r\n", len);
        UART_Driver_SendString(UART_DEBUG, buffer);
        return;
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Payload (%u bytes):", len);
    for (uint8_t i = 0; i < len && offset < sizeof(buffer) - 4; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %02X", payload[i]);
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "\r\n");
    UART_Driver_SendString(UART_DEBUG, buffer);

    uint16_t crc = (payload[0] << 8) | payload[1];
    snprintf(buffer, sizeof(buffer), "Package CRC16: 0x%04X\r\n", crc);
    UART_Driver_SendString(UART_DEBUG, buffer);
    MIN_Send(ctx, COLLECT_PACKAGE_ACK, NULL, 0);
}

static void MIN_Handler_SAMPLERATE_GET(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    char buffer[256];
    int offset = 0;

    if (len != 4) {
        snprintf(buffer, sizeof(buffer), "Invalid payload length. Expected 4 bytes, got %u.\r\n", len);
        UART_Driver_SendString(UART_DEBUG, buffer);
        return;
    }

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "Payload (%u bytes):", len);
    for (uint8_t i = 0; i < len && offset < sizeof(buffer) - 4; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, " %02X", payload[i]);
    }
    snprintf(buffer + offset, sizeof(buffer) - offset, "\r\n");
    UART_Driver_SendString(UART_DEBUG, buffer);

    uint32_t sample_rate = (payload[3] << 24) | (payload[2] << 16) | (payload[1] << 8) | payload[0];
    snprintf(buffer, sizeof(buffer), "Sample rate: %lu Hz\r\n", (unsigned long)sample_rate);
    UART_Driver_SendString(UART_DEBUG, buffer);
    MIN_Send(ctx, SAMPLERATE_GET_ACK, NULL, 0);
}


static void MIN_Handler_HEARTBEAT_CMD(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    static const uint8_t response[] = "HB";
    MIN_Send(ctx, HEARTBEAT_ACK, response, sizeof(response) - 1);
}

static void MIN_Handler_GET_STATUS_CMD(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    static const uint8_t response[] = "OK";
    MIN_Send(ctx, STATUS_RESPONSE, response, sizeof(response) - 1);
}

static void MIN_Handler_RESET_CMD(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    MIN_ReInit(ctx);
}

static void MIN_Handler_PING_CMD(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    MIN_Send(ctx, PONG_CMD, NULL, 0);
}

static void MIN_Handler_DUMMY_CMD_1(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    static const uint8_t response[] = "D1";
    MIN_Send(ctx, DUMMY_CMD_1, response, sizeof(response) - 1);
}

static void MIN_Handler_DUMMY_CMD_2(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    static const uint8_t response[] = "D2";
    MIN_Send(ctx, DUMMY_CMD_2, response, sizeof(response) - 1);
}

static void MIN_Handler_CUSTOM_CMD_1(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    static const uint8_t response[] = "C1";
    MIN_Send(ctx, CUSTOM_CMD_1_ACK, response, sizeof(response) - 1);
}

static void MIN_Handler_CUSTOM_CMD_2(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    static const uint8_t response[] = "C2";
    MIN_Send(ctx, CUSTOM_CMD_2_ACK, response, sizeof(response) - 1);
}

// =================================================================
// Command Table
// =================================================================

static const MIN_Command_t command_table[] = {
    { HEARTBEAT_CMD,    	MIN_Handler_HEARTBEAT_CMD },
    { GET_STATUS_CMD,   	MIN_Handler_GET_STATUS_CMD },
    { RESET_CMD,        	MIN_Handler_RESET_CMD },
    { PING_CMD,         	MIN_Handler_PING_CMD },
    { DUMMY_CMD_1,      	MIN_Handler_DUMMY_CMD_1 },
    { DUMMY_CMD_2,      	MIN_Handler_DUMMY_CMD_2 },
    { CUSTOM_CMD_1,     	MIN_Handler_CUSTOM_CMD_1 },
    { CUSTOM_CMD_2,     	MIN_Handler_CUSTOM_CMD_2 },
	{ CONTROL_TEMP_ACK, 	MIN_Handler_CONTROL_TEMP_ACK },
	{ PRE_DATA_ACK, 		MIN_Handler_PRE_DATA},
	{ PRE_CHUNK_ACK, 		MIN_Handler_PRE_CHUNK},
	{ COLLECT_PACKAGE_ACK, 	MIN_Handler_COLLECT_PACKAGE},
	{ SAMPLERATE_GET_ACK, 	MIN_Handler_SAMPLERATE_GET},
};

static const int command_table_size = sizeof(command_table) / sizeof(command_table[0]);

// =================================================================
// Helper Functions
// =================================================================

const MIN_Command_t *MIN_GetCommandTable(void) {
    return command_table;
}

int MIN_GetCommandTableSize(void) {
    return command_table_size;
}

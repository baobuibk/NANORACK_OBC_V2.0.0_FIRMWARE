/*
 * min_command.c
 *
 *  Created on: Apr 22, 2025
 *      Author: CAO HIEU
 */

#include "min_command.h"
#include <string.h>
#include "stdio.h"

#include "QSPI/quadspi.h"
#include "Struct/gpio_state.h"
#include "SPI_SlaveOfOBC/spi_slave.h"

#define MAX_SIZE (100 * 1024) // 100KB
#define RAM_D2_200KB_START ((uint8_t*)&_scustom_data)
#define RAM_D2_200KB_SIZE  (200 * 1024) // 200KB

#define SPI_RAM_START ((uint8_t*)&_schunk_data)
#define SPI_RAM_SIZE  (10 * 1024)
#define MAX_CHUNKS 20

extern uint32_t _scustom_data;
extern uint32_t _ecustom_data;

extern uint8_t _schunk_data[];
extern uint8_t _echunk_data[];

uint32_t g_total_size;
uint16_t g_chunk_size;

uint32_t g_sample_rate;
uint16_t g_chunk_crcs[MAX_CHUNKS];
// =================================================================
// Command Handlers
// =================================================================

//static void MIN_Handler_COLLECT_DATA(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
//    printf("Payload (%u bytes):", len);
//    for (uint8_t i = 0; i < len; i++) {
//        printf(" %02X", payload[i]);
//    }
//    printf("\r\n");
//
//    if (len < 6) {
//        printf("Invalid payload length.\r\n");
//        return;
//    }
//    uint16_t chunk_size = (payload[1] << 8) | payload[0];
//    uint32_t sample = (payload[5] << 24) | (payload[4] << 16) | (payload[3] << 8) | payload[2];
//
//    printf("Message -Hex: chunk_size = %u (0x%04X), sample = %lu (0x%08lX)\r\n",
//           chunk_size, chunk_size, (unsigned long)sample, (unsigned long)sample);
//
//    MIN_Send(ctx, GOT_IT, NULL, 0);
//}

static uint16_t UpdateCRC16_XMODEM(uint16_t crc, uint8_t byte) {
    const uint16_t polynomial = 0x1021; // CRC16 XMODEM
    crc ^= (uint16_t)byte << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ polynomial;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

static void MIN_Handler_SAMPLERATE_SET(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    if (len < 4) {
        printf("Invalid payload length.\r\n");
        return;
    }

    g_sample_rate = (payload[3] << 24) | (payload[2] << 16) | (payload[1] << 8) | payload[0];

    printf("Sample rate set to %lu Hz\r\n", (unsigned long)g_sample_rate);
    MIN_Send(ctx, DONE, NULL, 0);
}

static void MIN_Handler_SAMPLERATE_GET(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    uint8_t response[4];

    response[0] = (uint8_t)(g_sample_rate & 0xFF);
    response[1] = (uint8_t)((g_sample_rate >> 8) & 0xFF);
    response[2] = (uint8_t)((g_sample_rate >> 16) & 0xFF);
    response[3] = (uint8_t)((g_sample_rate >> 24) & 0xFF);

    printf("Sample rate retrieved: %lu Hz\r\n", (unsigned long)g_sample_rate);
    MIN_Send(ctx, SAMPLERATE_GET_ACK, response, sizeof(response));
}

static void MIN_Handler_COLLECT_DATA(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {

	toOBC_SetState(toOBC_BUSY);

    if (len < 4) {
        printf("Invalid payload length.\r\n");
    	toOBC_SetState(toOBC_ERROR);
        return;
    }

    uint32_t sample = (payload[3] << 24) | (payload[2] << 16) | (payload[1] << 8) | payload[0];
    uint32_t total_size = sample * 2;

    if (total_size > RAM_D2_200KB_SIZE) {
        printf("Data size exceeds available RAM (%lu > %lu).\r\n",
               (unsigned long)total_size, (unsigned long)RAM_D2_200KB_SIZE);
    	toOBC_SetState(toOBC_ERROR);
        return;
    }

    if (CSP_QSPI_Read(RAM_D2_200KB_START, 0, total_size) != HAL_OK) {
        printf("Error reading QSPI Flash.\r\n");
    	toOBC_SetState(toOBC_ERROR);
        return;
    }

    g_total_size = total_size;

    printf("Collected %lu samples, total size: %lu bytes\r\n",
           (unsigned long)sample, (unsigned long)total_size);
    MIN_Send(ctx, DONE, NULL, 0);
	toOBC_SetState(toOBC_READYSEND);
}

static void MIN_Handler_PRE_DATA(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    if (len < 2) {
        printf("Invalid payload length.\r\n");
        return;
    }

    if (g_total_size == 0) {
        printf("No data collected yet.\r\n");
        return;
    }

    uint16_t chunk_size = (payload[1] << 8) | payload[0];
    g_chunk_size = chunk_size;

    uint32_t num_chunks = (g_total_size + g_chunk_size - 1) / g_chunk_size;
    if (num_chunks > MAX_CHUNKS) {
        printf("Too many chunks (%lu > %d).\r\n", (unsigned long)num_chunks, MAX_CHUNKS);
        return;
    }

    memset(g_chunk_crcs, 0, sizeof(g_chunk_crcs));

    uint8_t response[1 + 2 * num_chunks];
    response[0] = (uint8_t)num_chunks;

    for (uint32_t i = 0; i < num_chunks; i++) {
        uint32_t offset = i * g_chunk_size;
        uint32_t size = (i == num_chunks - 1) ? (g_total_size % g_chunk_size) : g_chunk_size;
        if (size == 0) size = g_chunk_size;
        uint16_t crc = 0x0000;
        for (uint32_t j = 0; j < size; j++) {
            crc = UpdateCRC16_XMODEM(crc, RAM_D2_200KB_START[offset + j]);
        }
        g_chunk_crcs[i] = crc;
        response[1 + 2 * i] = (uint8_t)(crc >> 8);
        response[2 + 2 * i] = (uint8_t)(crc & 0xFF);
    }

    MIN_Send(ctx, PRE_DATA_ACK, response, sizeof(response));
    printf("Pre Data Success: %lu Chunks\r\n", (unsigned long)num_chunks);
    for (uint32_t i = 0; i < num_chunks; i++) {
        printf("%lu -> CRC16: 0x%04X\r\n", (unsigned long)i, g_chunk_crcs[i]);
    }
}

static void MIN_Handler_PRE_CHUNK(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    if (len < 1) {
        printf("Invalid payload length.\r\n");
        return;
    }

    uint8_t chunk_num = payload[0];
    uint32_t num_chunks = (g_total_size + g_chunk_size - 1) / g_chunk_size;

    if (chunk_num >= num_chunks) {
        printf("Invalid chunk number (%u >= %lu).\r\n", chunk_num, (unsigned long)num_chunks);
        return;
    }

    uint32_t offset = chunk_num * g_chunk_size;
    uint32_t size = (chunk_num == num_chunks - 1) ? (g_total_size % g_chunk_size) : g_chunk_size;
    if (size == 0) size = g_chunk_size;

    memset(SPI_RAM_START, 0, SPI_RAM_SIZE);
    memcpy(SPI_RAM_START, RAM_D2_200KB_START + offset, size);

    SPI_SlaveDevice_ResetDMA((uint32_t)SPI_RAM_START, g_chunk_size);

    uint8_t response[2];
    response[0] = (uint8_t)(g_chunk_crcs[chunk_num] >> 8);
    response[1] = (uint8_t)(g_chunk_crcs[chunk_num] & 0xFF);

    MIN_Send(ctx, PRE_CHUNK_ACK, response, sizeof(response));
    printf("Prepared chunk %u for SPI read, size: %lu bytes, CRC16: 0x%04X\r\n",
           chunk_num, (unsigned long)size, g_chunk_crcs[chunk_num]);
}

static void MIN_Handler_COLLECT_PACKAGE(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    const uint32_t package_size = SPI_RAM_SIZE;
    const uint32_t sample_count = package_size / 2;

    memset(SPI_RAM_START, 0, SPI_RAM_SIZE);

    if (CSP_QSPI_Read(SPI_RAM_START, 0, SPI_RAM_SIZE) != HAL_OK) {
        printf("Error reading QSPI Flash.\r\n");
        return;
    }

    uint16_t crc = 0x0000;
    for (uint32_t i = 0; i < package_size; i++) {
        crc = UpdateCRC16_XMODEM(crc, SPI_RAM_START[i]);
    }

    uint8_t response[2];
    response[0] = (uint8_t)(crc >> 8);
    response[1] = (uint8_t)(crc & 0xFF);

    printf("Collected package: %lu samples, size: %lu bytes, CRC16: 0x%04X\r\n",
           (unsigned long)sample_count, (unsigned long)package_size, crc);
    MIN_Send(ctx, COLLECT_PACKAGE_ACK, response, sizeof(response));
}

static void MIN_Handler_READ_TEMP_CMD(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    (void)payload; (void)len;
    static const uint8_t response[] = "25.3";

    MIN_Send(ctx, TEMP_RESPONSE, response, sizeof(response) - 1);
}

static void MIN_Handler_CONTROL_TEMP_CMD(MIN_Context_t *ctx, const uint8_t *payload, uint8_t len) {
    printf("Payload (%u bytes):", len);
    for (uint8_t i = 0; i < len; i++) {
        printf(" %02X", payload[i]);
    }
    printf("\r\n");

    static const uint8_t response[] = "OK";
    MIN_Send(ctx, CONTROL_TEMP_ACK, response, sizeof(response) - 1);
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
    { READ_TEMP_CMD,    	MIN_Handler_READ_TEMP_CMD },
    { CONTROL_TEMP_CMD, 	MIN_Handler_CONTROL_TEMP_CMD },
    { HEARTBEAT_CMD,    	MIN_Handler_HEARTBEAT_CMD },
    { GET_STATUS_CMD,   	MIN_Handler_GET_STATUS_CMD },
    { RESET_CMD,        	MIN_Handler_RESET_CMD },
    { PING_CMD,         	MIN_Handler_PING_CMD },
    { DUMMY_CMD_1,      	MIN_Handler_DUMMY_CMD_1 },
    { DUMMY_CMD_2,      	MIN_Handler_DUMMY_CMD_2 },
    { CUSTOM_CMD_1,     	MIN_Handler_CUSTOM_CMD_1 },
    { CUSTOM_CMD_2,     	MIN_Handler_CUSTOM_CMD_2 },
    { COLLECT_DATA_CMD, 	MIN_Handler_COLLECT_DATA},
    { PRE_DATA_CMD, 		MIN_Handler_PRE_DATA},
    { PRE_CHUNK_CMD, 		MIN_Handler_PRE_CHUNK},
	{ SAMPLERATE_SET_CMD,  	MIN_Handler_SAMPLERATE_SET },
	{ SAMPLERATE_GET_CMD,  	MIN_Handler_SAMPLERATE_GET },
	{ COLLECT_PACKAGE_CMD, 	MIN_Handler_COLLECT_PACKAGE },
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

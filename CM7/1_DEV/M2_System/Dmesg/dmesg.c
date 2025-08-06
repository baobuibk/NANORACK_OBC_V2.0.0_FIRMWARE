/************************************************
 *  @file     : dmesg.c
 *  @date     : May 9, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    [-]
 ************************************************/

#include "dmesg.h"
#include "stdio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>
#include "main.h"
#include "uart_driver_dma.h"
#include "stdbool.h"

extern uint8_t _sdmesg_buffer;
extern uint8_t _edmesg_buffer;

#define DMESG_BUFFER_SIZE (32 * 1024) // 96KB
//#define DMESG_BUFFER_SIZE 256

static uint8_t *write_ptr = &_sdmesg_buffer;
static uint8_t *read_ptr = &_sdmesg_buffer;
static size_t log_count = 0;
static SemaphoreHandle_t dmesg_mutex = NULL;

void Dmesg_Init(void) {
    write_ptr = &_sdmesg_buffer;
    read_ptr = &_sdmesg_buffer;
    log_count = 0;
    dmesg_mutex = xSemaphoreCreateMutex();
    if (dmesg_mutex == NULL) {
        while (1);
    }
}

static void dmesg_write(const char *msg, uint32_t len) {
    if (len > 255) len = 255;

    uint32_t required_space = len + 1;

    uint32_t space_left;
    if (read_ptr <= write_ptr) {
        // Case 1: [____R###W____] - data  R  W
        space_left = DMESG_BUFFER_SIZE - (write_ptr - read_ptr) - 1; // -1 collision
    } else {
        // Case 2: [###W____R###] -
        space_left = read_ptr - write_ptr - 1; // -1 collision
    }

    while (space_left < required_space && log_count > 0) {
        uint8_t old_len = *read_ptr;
        read_ptr += old_len + 1;

        // Wrap around
        if (read_ptr >= &_edmesg_buffer) {
            read_ptr = &_sdmesg_buffer + (read_ptr - &_edmesg_buffer);
        }

        log_count--;

        // space_left
        if (read_ptr <= write_ptr) {
            space_left = DMESG_BUFFER_SIZE - (write_ptr - read_ptr) - 1;
        } else {
            space_left = read_ptr - write_ptr - 1;
        }
    }

    // write length
    *write_ptr = (uint8_t)len;
    write_ptr++;
    if (write_ptr >= &_edmesg_buffer) {
        write_ptr = &_sdmesg_buffer;
    }

    // write data
    for (uint32_t i = 0; i < len; i++) {
        *write_ptr = msg[i];
        write_ptr++;
        if (write_ptr >= &_edmesg_buffer) {
            write_ptr = &_sdmesg_buffer;
        }
    }
    log_count++;
}

void Dmesg_HardWrite(const char *msg) {
    uint32_t len = strlen(msg);
    dmesg_write(msg, len);
}

void Dmesg_SafeWrite(const char *msg) {
    if (dmesg_mutex != NULL && xSemaphoreTake(dmesg_mutex, portMAX_DELAY) == pdTRUE) {
        Dmesg_HardWrite(msg);
        xSemaphoreGive(dmesg_mutex);
    }
}

static void dmesg_print_entry(uint8_t **ptr, EmbeddedCli *cli) {
    uint8_t len = **ptr;
    (*ptr)++;
    if (*ptr >= &_edmesg_buffer) {
        *ptr = &_sdmesg_buffer;
    }
    char buffer[DMESG_MSG_MAX_LENGTH + 1];
    size_t copy_len = (len < sizeof(buffer) - 1) ? len : sizeof(buffer) - 1;
    for (size_t i = 0; i < copy_len; i++) {
        buffer[i] = **ptr;
        (*ptr)++;
        if (*ptr >= &_edmesg_buffer) {
            *ptr = &_sdmesg_buffer;
        }
    }
    buffer[copy_len] = '\0';
    embeddedCliPrint(cli, buffer);
}

void Dmesg_GetLogs(EmbeddedCli *cli) {
    if (dmesg_mutex == NULL || xSemaphoreTake(dmesg_mutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    uint8_t *ptr = read_ptr;
    uint8_t *end_ptr = write_ptr;
    size_t entries_to_read = log_count;

    for (size_t i = 0; i < entries_to_read && ptr != end_ptr; i++) {
        dmesg_print_entry(&ptr, cli);
    }

    xSemaphoreGive(dmesg_mutex);
}

void Dmesg_GetLatestN(size_t N, EmbeddedCli *cli) {
    if (dmesg_mutex == NULL || xSemaphoreTake(dmesg_mutex, portMAX_DELAY) != pdTRUE) {
        return;
    }

    if (N > log_count) N = log_count;
    if (N == 0) {
        xSemaphoreGive(dmesg_mutex);
        return;
    }

    size_t skip = log_count - N;
    uint8_t *ptr = read_ptr;

    for (size_t i = 0; i < skip; i++) {
        uint8_t len = *ptr;
        ptr += (len + 1);
        if (ptr >= &_edmesg_buffer) {
            ptr = &_sdmesg_buffer + (ptr - &_edmesg_buffer);
        }
    }

    for (size_t i = 0; i < N; i++) {
        dmesg_print_entry(&ptr, cli);
    }
    xSemaphoreGive(dmesg_mutex);
}

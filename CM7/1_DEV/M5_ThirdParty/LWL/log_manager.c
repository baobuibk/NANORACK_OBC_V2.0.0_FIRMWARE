#include "log_manager.h"
#include <stdio.h>

#include "logger/bscript_logger.h"
#include "DateTime/date_time.h"
#include "SimpleDataTransfer/simple_datatrans.h"

__attribute__((section(".log_data"))) __attribute__((aligned(4))) uint8_t obc_log_left[LOG_BUFFER_SIZE_OBC];
__attribute__((section(".log_data"))) __attribute__((aligned(4))) uint8_t obc_log_right[LOG_BUFFER_SIZE_OBC];

LogManager_TypeDef log_manager;

static bool LogManager_SendLogData(uint8_t *current_buffer_to_send, uint32_t data_length);

// --- Internal Helper Functions ---
void LogManager_Init(void) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;

    channel->buffer_left = obc_log_left;
    channel->buffer_right = obc_log_right;
    channel->buffer_size = LOG_BUFFER_SIZE_OBC;
    channel->trigger_threshold = LOG_TRIGGER_THRESHOLD_OBC;

    memset(channel->buffer_left, 0, channel->buffer_size);
    memset(channel->buffer_right, 0, channel->buffer_size);

    channel->current_index = 0;
    channel->active_buffer = LOG_BUFFER_LEFT;
    channel->transfer_ready_flag = false;

    BScript_Log("[LogManager] OBC Log channel initialized.");
}


void LogManager_Write(uint8_t *data, uint32_t length) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;

    if (channel->current_index + length > channel->buffer_size) {
        #ifdef LOG_MANAGER_DEBUG
        BScript_Log("[LogManager] Buffer full or data too large! Dropping data.");
        #endif
        return;
    }

    uint8_t *target_buffer = (channel->active_buffer == LOG_BUFFER_LEFT) ? channel->buffer_left : channel->buffer_right;

    memcpy(target_buffer + channel->current_index, data, length);
    channel->current_index += length;

    if (channel->current_index >= channel->trigger_threshold && !channel->transfer_ready_flag) {
        channel->transfer_ready_flag = true;
        channel->active_buffer = (channel->active_buffer == LOG_BUFFER_LEFT) ? LOG_BUFFER_RIGHT : LOG_BUFFER_LEFT;
        channel->current_index = 0;
    }
}

Std_ReturnType LogManager_Process(void) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;

    if (channel->transfer_ready_flag) {
        uint8_t *buffer_to_send = (channel->active_buffer == LOG_BUFFER_RIGHT) ?
                                 channel->buffer_left : channel->buffer_right;

        uint32_t length_to_send = channel->buffer_size;

        if (LogManager_SendLogData(buffer_to_send, length_to_send)) {
            memset(buffer_to_send, 0, channel->buffer_size);
            channel->transfer_ready_flag = false;
            return E_OK;
        } else {
            BScript_Log("[LogManager] Transfer failed. Will retry later.");
            return E_ERROR;
        }
    }

    return E_OK;
}

static bool LogManager_SendLogData(uint8_t *current_buffer_to_send, uint32_t data_length) {
    char base_filename[48];
    s_DateTime rtc;
    Utils_GetRTC(&rtc);

    snprintf(base_filename, sizeof(base_filename), "obc_log_20%02d%02d%02d_%02d%02d%02d",
             rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);

    BScript_Log("[LogManager] Initiating transfer for %s...", base_filename);

    SimpleTransferResult_t result = SimpleDataTransfer_ExecuteLogTransfer(
        base_filename,
        current_buffer_to_send,
        data_length,
        rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second
    );

    if (result == SIMPLE_TRANSFER_SUCCESS) {
        BScript_Log("[LogManager] Transfer for %s succeeded.", base_filename);
        return true;
    } else {
        BScript_Log("[LogManager] Transfer for %s failed. Result: %s",
                      base_filename, SimpleDataTransfer_GetResultString(result));
        return false;
    }
}


// --- Debugging API Implementation ---

#ifdef LOG_MANAGER_DEBUG

void LogManager_DebugInfo(void) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;

    BScript_Log("--- Log Manager Debug Info (OBC) ---");
    BScript_Log("Current Index: %lu bytes", channel->current_index);
    BScript_Log("Active Buffer: %s", (channel->active_buffer == LOG_BUFFER_LEFT) ? "LEFT" : "RIGHT");
    BScript_Log("Transfer Ready Flag: %s", channel->transfer_ready_flag ? "TRUE" : "FALSE");
    BScript_Log("------------------------------------");
}

void LogManager_DumpBuffer(LogBufferSide_TypeDef buffer_side) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;
    const char* buffer_name = (buffer_side == LOG_BUFFER_LEFT) ? "LEFT" : "RIGHT";
    uint8_t* buffer_ptr = (buffer_side == LOG_BUFFER_LEFT) ? channel->buffer_left : channel->buffer_right;
    uint32_t size = channel->buffer_size;

    BScript_Log("--- Dumping OBC %s Buffer (Size: %lu bytes) ---", buffer_name, size);

    char line_buf[128];
    for (uint32_t i = 0; i < size; i += 16) {
        int len = snprintf(line_buf, sizeof(line_buf), "%04lX: ", i);
        for (uint32_t j = 0; j < 16 && (i + j) < size; j++) {
            len += snprintf(line_buf + len, sizeof(line_buf) - len, "%02X ", buffer_ptr[i + j]);
        }
        BScript_Log("%s", line_buf);
    }
    BScript_Log("------------------------------------------------");
}

#endif // LOG_MANAGER_DEBUG

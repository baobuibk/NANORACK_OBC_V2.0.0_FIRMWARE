#include "log_manager.h"
#include <stdio.h>

#include "logger/bscript_logger.h"
#include "DateTime/date_time.h"
#include "SimpleDataTransfer/simple_datatrans.h"

__attribute__((section(".log_data"))) __attribute__((aligned(4))) uint8_t obc_log_left[LOG_BUFFER_SIZE_OBC];
__attribute__((section(".log_data"))) __attribute__((aligned(4))) uint8_t obc_log_right[LOG_BUFFER_SIZE_OBC];

extern uint8_t g_transfer_ram_d1_buffer[DATA_CHUNK_SIZE];

LogManager_TypeDef log_manager;

// --- Helper Functions ---
static LogBufferState_TypeDef* LogManager_GetBufferState(LogChannel_TypeDef *channel, LogBufferSide_TypeDef buffer_side) {
    return (buffer_side == LOG_BUFFER_LEFT) ? &channel->left_state : &channel->right_state;
}

static uint8_t* LogManager_GetBuffer(LogChannel_TypeDef *channel, LogBufferSide_TypeDef buffer_side) {
    return (buffer_side == LOG_BUFFER_LEFT) ? channel->buffer_left : channel->buffer_right;
}

static LogBufferSide_TypeDef LogManager_FindBestWriteBuffer(LogChannel_TypeDef *channel) {
//    LogBufferState_TypeDef *left_state = &channel->left_state;
//    LogBufferState_TypeDef *right_state = &channel->right_state;

    // Priority 1: Use current active buffer if it's available and not full
    LogBufferState_TypeDef *active_state = LogManager_GetBufferState(channel, channel->active_buffer);
    if (!active_state->is_transferring && !active_state->transfer_ready_flag) {
        return channel->active_buffer;
    }

    // Priority 2: Use the other buffer if available
    LogBufferSide_TypeDef other_buffer = (channel->active_buffer == LOG_BUFFER_LEFT) ? LOG_BUFFER_RIGHT : LOG_BUFFER_LEFT;
    LogBufferState_TypeDef *other_state = LogManager_GetBufferState(channel, other_buffer);
    if (!other_state->is_transferring && !other_state->transfer_ready_flag) {
        channel->active_buffer = other_buffer;  // Switch active buffer
        return other_buffer;
    }

    // No buffer available
    return LOG_BUFFER_LEFT;  // Return anything, caller should check availability
}

// --- Public API Implementation ---
void LogManager_Init(void) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;

    channel->buffer_left = obc_log_left;
    channel->buffer_right = obc_log_right;
    channel->buffer_size = LOG_BUFFER_SIZE_OBC;
    channel->trigger_threshold = LOG_TRIGGER_THRESHOLD_OBC;

    memset(channel->buffer_left, 0, channel->buffer_size);
    memset(channel->buffer_right, 0, channel->buffer_size);

    // Initialize left buffer state
    channel->left_state.current_index = 0;
    channel->left_state.transfer_ready_flag = false;
    channel->left_state.is_transferring = false;

    // Initialize right buffer state
    channel->right_state.current_index = 0;
    channel->right_state.transfer_ready_flag = false;
    channel->right_state.is_transferring = false;

    channel->active_buffer = LOG_BUFFER_LEFT;

    BScript_Log("[LogManager] OBC Log channel initialized with dual buffer state tracking.");
}

void LogManager_Write(uint8_t *data, uint32_t length) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;

    // Find the best buffer to write to
    LogBufferSide_TypeDef write_buffer = LogManager_FindBestWriteBuffer(channel);
    LogBufferState_TypeDef *buffer_state = LogManager_GetBufferState(channel, write_buffer);

    // Check if the selected buffer is actually available
    if (buffer_state->is_transferring || buffer_state->transfer_ready_flag) {
        #ifdef LOG_MANAGER_DEBUG
        BScript_Log("[LogManager] All buffers busy! Dropping %lu bytes of data.", length);
        #endif
        return;
    }

    // Check if data fits
    if (buffer_state->current_index + length > channel->buffer_size) {
        #ifdef LOG_MANAGER_DEBUG
        BScript_Log("[LogManager] Data too large for buffer! Marking buffer ready and dropping data.");
        #endif
        // Mark current buffer as ready and try to switch
        buffer_state->transfer_ready_flag = true;
        return;
    }

    // Write data to the selected buffer
    uint8_t *target_buffer = LogManager_GetBuffer(channel, write_buffer);
    memcpy(target_buffer + buffer_state->current_index, data, length);
    buffer_state->current_index += length;

    // Check if buffer should be marked as ready for transfer
    if (buffer_state->current_index >= channel->trigger_threshold) {
        buffer_state->transfer_ready_flag = true;
        #ifdef LOG_MANAGER_DEBUG
        BScript_Log("[LogManager] Buffer %s reached threshold, marked ready for transfer.",
                   (write_buffer == LOG_BUFFER_LEFT) ? "LEFT" : "RIGHT");
        #endif
    }
}

Std_ReturnType LogManager_Process(void) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;
    Std_ReturnType result = E_OK;

    // Check both buffers for transfer readiness
    for (int i = 0; i < 2; i++) {
        LogBufferSide_TypeDef buffer_side = (i == 0) ? LOG_BUFFER_LEFT : LOG_BUFFER_RIGHT;
        LogBufferState_TypeDef *buffer_state = LogManager_GetBufferState(channel, buffer_side);

        if (buffer_state->transfer_ready_flag && !buffer_state->is_transferring) {
            // Mark buffer as being transferred
            buffer_state->is_transferring = true;

            uint8_t *buffer_to_send = LogManager_GetBuffer(channel, buffer_side);
            uint32_t length_to_send = DATA_CHUNK_SIZE;  // Use actual data length, not full buffer

            if (length_to_send > DATA_CHUNK_SIZE) {
                length_to_send = DATA_CHUNK_SIZE;
            }

            char base_filename[48];
            s_DateTime rtc;
            Utils_GetRTC(&rtc);

            snprintf(base_filename, sizeof(base_filename), "obc_log_%s_20%02d%02d%02d_%02d%02d%02d",
                     (buffer_side == LOG_BUFFER_LEFT) ? "L" : "R",
                     rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);

            BScript_Log("[LogManager] Initiating transfer for %s (%lu bytes)...", base_filename, length_to_send);

            memcpy(g_transfer_ram_d1_buffer, buffer_to_send, length_to_send);

            SimpleTransferResult_t transfer_result = SimpleDataTransfer_ExecuteLogTransfer(
                base_filename,
                rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second
            );

            if (transfer_result == SIMPLE_TRANSFER_SUCCESS) {
                BScript_Log("[LogManager] Transfer for %s succeeded.", base_filename);

                // Reset buffer state
                memset(buffer_to_send, 0, channel->buffer_size);
                buffer_state->current_index = 0;
                buffer_state->transfer_ready_flag = false;
                buffer_state->is_transferring = false;
            } else {
                BScript_Log("[LogManager] Transfer for %s failed. Result: %s",
                           base_filename, SimpleDataTransfer_GetResultString(transfer_result));

                // Reset buffer state even on failure to prevent infinite retry
                memset(buffer_to_send, 0, channel->buffer_size);
                buffer_state->current_index = 0;
                buffer_state->transfer_ready_flag = false;
                buffer_state->is_transferring = false;
                result = E_ERROR;
            }

            // Only process one buffer per call to avoid blocking
            break;
        }
    }

    return result;
}

// --- Debugging API Implementation ---
#ifdef LOG_MANAGER_DEBUG

void LogManager_DebugInfo(void) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;

    BScript_Log("--- Log Manager Debug Info (OBC) ---");
    BScript_Log("Active Buffer: %s", (channel->active_buffer == LOG_BUFFER_LEFT) ? "LEFT" : "RIGHT");

    BScript_Log("LEFT Buffer:");
    BScript_Log("  Current Index: %lu bytes", channel->left_state.current_index);
    BScript_Log("  Transfer Ready: %s", channel->left_state.transfer_ready_flag ? "TRUE" : "FALSE");
    BScript_Log("  Is Transferring: %s", channel->left_state.is_transferring ? "TRUE" : "FALSE");

    BScript_Log("RIGHT Buffer:");
    BScript_Log("  Current Index: %lu bytes", channel->right_state.current_index);
    BScript_Log("  Transfer Ready: %s", channel->right_state.transfer_ready_flag ? "TRUE" : "FALSE");
    BScript_Log("  Is Transferring: %s", channel->right_state.is_transferring ? "TRUE" : "FALSE");

    BScript_Log("------------------------------------");
}

void LogManager_DumpBuffer(LogBufferSide_TypeDef buffer_side) {
    LogChannel_TypeDef *channel = &log_manager.obc_channel;
    const char* buffer_name = (buffer_side == LOG_BUFFER_LEFT) ? "LEFT" : "RIGHT";
    uint8_t* buffer_ptr = LogManager_GetBuffer(channel, buffer_side);
    LogBufferState_TypeDef* buffer_state = LogManager_GetBufferState(channel, buffer_side);

    BScript_Log("--- Dumping OBC %s Buffer (Data: %lu/%lu bytes) ---",
               buffer_name, buffer_state->current_index, channel->buffer_size);

    char line_buf[128];
    uint32_t dump_size = (buffer_state->current_index > 0) ? buffer_state->current_index : 256; // Show at least some data

    for (uint32_t i = 0; i < dump_size && i < channel->buffer_size; i += 16) {
        int len = snprintf(line_buf, sizeof(line_buf), "%04lX: ", i);
        for (uint32_t j = 0; j < 16 && (i + j) < dump_size; j++) {
            len += snprintf(line_buf + len, sizeof(line_buf) - len, "%02X ", buffer_ptr[i + j]);
        }
        BScript_Log("%s", line_buf);
    }
    BScript_Log("------------------------------------------------");
}

#endif // LOG_MANAGER_DEBUG

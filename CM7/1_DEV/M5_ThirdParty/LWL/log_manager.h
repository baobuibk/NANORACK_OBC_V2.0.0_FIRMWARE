#ifndef INC_LOG_MANAGER_H_
#define INC_LOG_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "Define/define.h"

#define LOG_MANAGER_DEBUG

// Configuration
#define LOG_BUFFER_SIZE_OBC         (32 * 1024)
#define LOG_TRIGGER_THRESHOLD_OBC   (31 * 1024)

typedef enum {
    LOG_BUFFER_LEFT,
    LOG_BUFFER_RIGHT
} LogBufferSide_TypeDef;

// Structure for individual buffer state
typedef struct {
    uint32_t current_index;
    volatile bool transfer_ready_flag;
    volatile bool is_transferring;  // Prevent writing during transfer
} LogBufferState_TypeDef;

// Structure to manage a single log channel (e.g., OBC or EXP)
typedef struct {
    uint8_t *buffer_left;
    uint8_t *buffer_right;
    uint32_t buffer_size;
    uint32_t trigger_threshold;

    // Separate state for each buffer
    LogBufferState_TypeDef left_state;
    LogBufferState_TypeDef right_state;

    LogBufferSide_TypeDef active_buffer;
} LogChannel_TypeDef;

// Main Log Manager structure holding all channels
typedef struct {
    LogChannel_TypeDef obc_channel;
} LogManager_TypeDef;

// --- Public API ---
void LogManager_Init(void);
void LogManager_Write(uint8_t *data, uint32_t length);
Std_ReturnType LogManager_Process(void);

// --- Debugging API (Optional) ---
#ifdef LOG_MANAGER_DEBUG
void LogManager_DebugInfo(void);
void LogManager_DumpBuffer(LogBufferSide_TypeDef buffer_side);
#endif

#endif /* INC_LOG_MANAGER_H_ */

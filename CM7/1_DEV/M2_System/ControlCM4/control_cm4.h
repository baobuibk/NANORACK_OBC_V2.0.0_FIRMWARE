/************************************************
 *  @file     : control_cm4.h
 *  @date     : Aug 25, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    [-]
 ************************************************/

#ifndef M2_SYSTEM_CONTROLCM4_CONTROL_CM4_H_
#define M2_SYSTEM_CONTROLCM4_CONTROL_CM4_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define CM4_CONTROL_CHECK_INTERVAL_MS      10000    // 30 seconds check interval
#define CM4_CONTROL_RESPONSE_TIMEOUT_MS    5000     // 5 seconds timeout for keep-alive
#define CM4_CONTROL_MAX_RETRY_COUNT        3        // Max missed keep-alive before reset
#define CM4_CONTROL_RESET_PULSE_MS         100      // Reset pulse duration
#define CM4_CONTROL_SHUTDOWN_TIMEOUT_MS    30000    // Wait time after shutdown command

#define CM4_CONTROL_DEFAULT_ON_TIME        (4 * 3600 + 55 * 60 + 0)   // 04:55:00 in seconds
#define CM4_CONTROL_DEFAULT_OFF_TIME       (7 * 3600 +  0 * 60 + 0)   // 07:00:00 in seconds

typedef enum {
	CM4_CONTROL_RUN,
	CM4_CONTROL_STOP
} CM4_Control_t;

/**
 * @brief CM4 Control States
 */
typedef enum {
    CM4_STATE_OFF = 0,
    CM4_STATE_BOOTING,
    CM4_STATE_ON,
    CM4_STATE_SHUTTING_DOWN,
    CM4_STATE_ERROR
} CM4_State_t;

/**
 * @brief CM4 Control Request Types
 */
typedef enum {
    CM4_REQUEST_NONE = 0,
    CM4_REQUEST_ON,
    CM4_REQUEST_OFF,
    CM4_REQUEST_RESET
} CM4_Request_t;

/**
 * @brief CM4 Control Results
 */
typedef enum {
    CM4_RESULT_OK = 0,
    CM4_RESULT_ERROR,
    CM4_RESULT_TIMEOUT,
    CM4_RESULT_ALREADY_ON,
    CM4_RESULT_ALREADY_OFF,
    CM4_RESULT_BUSY
} CM4_Result_t;

/**
 * @brief CM4 Auto Power Schedule
 */
typedef struct {
    uint32_t power_on_time_sec;     // Daily time to power on (0-86399 seconds)
    uint32_t power_off_time_sec;    // Daily time to power off (0-86399 seconds)
    bool auto_power_enabled;        // Enable/disable auto power management
    bool powered_on_today;          // Flag to track if powered on today
    bool powered_off_today;         // Flag to track if powered off today
} CM4_AutoPowerSchedule_t;

/**
 * @brief CM4 Control Statistics
 */
typedef struct {
    uint32_t total_resets;          // Total number of resets performed
    uint32_t keep_alive_misses;     // Current consecutive keep-alive misses
    uint32_t total_keep_alive_failures; // Total keep-alive failures
    uint32_t total_auto_power_cycles;   // Total auto power cycles
    uint32_t total_manual_requests;     // Total manual requests
    TickType_t last_reset_time;     // Last reset timestamp
    TickType_t last_response_time;  // Last successful keep-alive response
} CM4_Statistics_t;

/**
 * @brief CM4 Control Manager Structure
 */
typedef struct {
    CM4_State_t current_state;
    CM4_Request_t pending_request;
    CM4_AutoPowerSchedule_t auto_schedule;
    CM4_Statistics_t statistics;

    // Manual control tracking
    uint8_t manual_on_requests;     // Number of active manual ON requests
    uint8_t script_on_requests;     // Number of active script ON requests

    // Keep-alive mechanism
    bool keep_alive_enabled;
    uint32_t keep_alive_miss_count;
    TickType_t last_keep_alive_time;

    // Synchronization
    SemaphoreHandle_t control_mutex;

    // Configuration
    bool initialized;
} CM4_ControlManager_t;


CM4_Result_t CM4_Control_Init(void);
/**
 * @brief Configure auto power schedule
 * @param on_time_raw Time to power on in HH:MM:SS format (0x00HHMMSS)
 * @param off_time_raw Time to power off in HH:MM:SS format (0x00HHMMSS)
 * @param enable Enable/disable auto power management
 * @return CM4_RESULT_OK if successful, error code otherwise
 */
CM4_Result_t CM4_Control_ConfigureAutoPower(uint32_t on_time_raw, uint32_t off_time_raw, bool enable);
CM4_Result_t CM4_Control_RequestOn(void);
CM4_Result_t CM4_Control_RequestOff(void);
CM4_Result_t CM4_Control_ScriptRequestOn(void);
CM4_Result_t CM4_Control_ScriptRequestOff(void);
CM4_Result_t CM4_Control_ForceReset(void);
CM4_Result_t CM4_Control_ForceShutdown(void);
CM4_State_t CM4_Control_GetState(void);
bool CM4_Control_IsOn(void);
void CM4_Control_SetKeepAliveEnabled(bool enable);
bool CM4_Control_IsKeepAliveEnabled(void);
void CM4_Control_GetStatistics(CM4_Statistics_t* stats);
void CM4_Control_PrintStatus(void);
bool CM4_Control_SaveConfig(void);
bool CM4_Control_LoadConfig(void);
void CM4_Control_Task(void *pvParameters);

const char* CM4_Control_StateToString(CM4_State_t state);
const char* CM4_Control_ResultToString(CM4_Result_t result);
void CM4_Control_SecondsToTimeString(uint32_t seconds, char* buffer);


#endif /* M2_SYSTEM_CONTROLCM4_CONTROL_CM4_H_ */

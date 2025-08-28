/************************************************
 *  @file     : control_cm4.c
 *  @date     : Aug 25, 2025
 *  @author   : CAO HIEU
 *-----------------------------------------------
 *  Description :
 *    [-]
 ************************************************/
#include "main.h"
#include "control_cm4.h"
#include "logger/bscript_logger.h"
#include "DateTime/date_time.h"
#include "utils.h"
#include "modfsp.h"
#include "SPI_FRAM/fram_spi.h"
#include <string.h>
#include <stdio.h>

extern MODFSP_Data_t cm4_protocol;

static CM4_Control_t cm4controlmode = CM4_CONTROL_RUN;

#define CM4_RST_PORT          MCU_IO_RESET_CM4_GPIO_Port
#define CM4_RST_PIN           MCU_IO_RESET_CM4_Pin

#define FRAM_TOTAL_SIZE         (256 * 1024)    // 256KB total
#define FRAM_SPACE_SECTION_SIZE (18 * 1024)    // 18KB for scripts
#define FRAM_CM4_CONFIG_BASE_ADDR   (FRAM_TOTAL_SIZE - FRAM_SPACE_SECTION_SIZE)

#define FRAM_CM4_ON_TIME_ADDR           (FRAM_CM4_CONFIG_BASE_ADDR + 0)  // 4 bytes
#define FRAM_CM4_OFF_TIME_ADDR          (FRAM_CM4_CONFIG_BASE_ADDR + 4)  // 4 bytes
#define FRAM_CM4_AUTO_ENABLE_ADDR       (FRAM_CM4_CONFIG_BASE_ADDR + 8)  // 1 byte
#define FRAM_CM4_CONFIG_SIZE            9

#ifndef CMD_SUDO_SHUTDOWN_NOW
#define CMD_SUDO_SHUTDOWN_NOW           0xDA  // Replace with actual command ID
#endif

CM4_ControlManager_t g_cm4_manager = {0};

/*************************************************
 *              PRIVATE FUNCTIONS                *
 *************************************************/

static void CM4_Control_TriggerReset(void);
static void CM4_Control_SendShutdown(void);
static uint32_t CM4_Control_GetCurrentDailyTimeSeconds(void);
static void CM4_Control_UpdateState(CM4_State_t new_state);
static void CM4_Control_ProcessPendingRequest(void);
static void CM4_Control_HandleAutoPower(void);
static bool CM4_Control_ShouldPowerOff(void);
static void CM4_Control_ResetDailyFlags(void);

/*************************************************
 *              PRIVATE FUNCTIONS                *
 *************************************************/

const char* CM4_Control_StateToString(CM4_State_t state)
{
    switch (state) {
        case CM4_STATE_OFF:           return "OFF";
        case CM4_STATE_BOOTING:       return "BOOTING";
        case CM4_STATE_ON:            return "ON";
        case CM4_STATE_SHUTTING_DOWN: return "SHUTTING_DOWN";
        case CM4_STATE_ERROR:         return "ERROR";
        default:                      return "UNKNOWN";
    }
}

const char* CM4_Control_ResultToString(CM4_Result_t result)
{
    switch (result) {
        case CM4_RESULT_OK:           return "OK";
        case CM4_RESULT_ERROR:        return "ERROR";
        case CM4_RESULT_TIMEOUT:      return "TIMEOUT";
        case CM4_RESULT_ALREADY_ON:   return "ALREADY_ON";
        case CM4_RESULT_ALREADY_OFF:  return "ALREADY_OFF";
        case CM4_RESULT_BUSY:         return "BUSY";
        default:                      return "UNKNOWN";
    }
}

void CM4_Control_SecondsToTimeString(uint32_t seconds, char* buffer)
{
    if (!buffer) return;

    uint8_t hours = (seconds / 3600) % 24;
    uint8_t minutes = (seconds % 3600) / 60;
    uint8_t secs = seconds % 60;

    snprintf(buffer, 9, "%02u:%02u:%02u", hours, minutes, secs);
}

static uint32_t CM4_Control_GetCurrentDailyTimeSeconds(void)
{
    s_DateTime rtc;
    Utils_GetRTC(&rtc);
    return (rtc.hour * 3600) + (rtc.minute * 60) + rtc.second;
}

static void CM4_Control_UpdateState(CM4_State_t new_state)
{
    if (g_cm4_manager.current_state != new_state) {
        BScript_Log("[CM4_Control] State change: %s -> %s",
                   CM4_Control_StateToString(g_cm4_manager.current_state),
                   CM4_Control_StateToString(new_state));

        g_cm4_manager.current_state = new_state;
    }
}

static void CM4_Control_TriggerReset(void)
{
    BScript_Log("[CM4_Control] Triggering CM4 hardware reset");

    // Record reset statistics
    g_cm4_manager.statistics.total_resets++;
    g_cm4_manager.statistics.last_reset_time = xTaskGetTickCount();

    // Hardware reset sequence
    GPIO_SetLow(CM4_RST_PORT, CM4_RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(CM4_CONTROL_RESET_PULSE_MS));
    GPIO_SetHigh(CM4_RST_PORT, CM4_RST_PIN);

    // Update state
    CM4_Control_UpdateState(CM4_STATE_BOOTING);
    g_cm4_manager.keep_alive_miss_count = 0;
}

static void CM4_Control_SendShutdown(void)
{
    BScript_Log("[CM4_Control] Sending shutdown command to CM4");

    if (MODFSP_Send(&cm4_protocol, CMD_SUDO_SHUTDOWN_NOW, NULL, 0) == MODFSP_OK) {
        BScript_Log("[CM4_Control] Shutdown command sent successfully");
        CM4_Control_UpdateState(CM4_STATE_SHUTTING_DOWN);
    } else {
        BScript_Log("[CM4_Control] Failed to send shutdown command");
        CM4_Control_UpdateState(CM4_STATE_ERROR);
    }
}

static bool CM4_Control_ShouldPowerOff(void)
{
    // Don't power off if there are active manual or script requests
    if (g_cm4_manager.manual_on_requests > 0 || g_cm4_manager.script_on_requests > 0) {
        return false;
    }

    return true;
}

static void CM4_Control_ResetDailyFlags(void)
{
    uint32_t current_time = CM4_Control_GetCurrentDailyTimeSeconds();

    if (current_time < 60) {
        if (g_cm4_manager.auto_schedule.powered_on_today ||
            g_cm4_manager.auto_schedule.powered_off_today) {
            BScript_Log("[CM4_Control] Resetting daily flags at midnight");
            g_cm4_manager.auto_schedule.powered_on_today = false;
            g_cm4_manager.auto_schedule.powered_off_today = false;
        }
    }
}

static void CM4_Control_HandleAutoPower(void)
{
    uint32_t current_time = CM4_Control_GetCurrentDailyTimeSeconds();
    CM4_AutoPowerSchedule_t* schedule = &g_cm4_manager.auto_schedule;

    if (!schedule->powered_on_today &&
        current_time >= schedule->power_on_time_sec &&
        current_time < (schedule->power_on_time_sec + 300)) { // 5-minute window

//        if (g_cm4_manager.current_state == CM4_STATE_OFF) {
            BScript_Log("[CM4_Control] Auto power ON triggered");
//            CM4_Control_TriggerReset();
//            CM4_Control_RequestOn();

            g_cm4_manager.manual_on_requests++;
            if(g_cm4_manager.manual_on_requests > 1){
            	g_cm4_manager.manual_on_requests = 1;
            }
            g_cm4_manager.statistics.total_manual_requests++;

            if (g_cm4_manager.current_state == CM4_STATE_ON) {
                BScript_Log("[CM4_Control] Manual ON request - already on (requests: %u)",
                           g_cm4_manager.manual_on_requests);
            }

            g_cm4_manager.pending_request = CM4_REQUEST_ON;
            BScript_Log("[CM4_Control] Manual ON request queued (requests: %u)",
                       g_cm4_manager.manual_on_requests);


            schedule->powered_on_today = true;
            g_cm4_manager.statistics.total_auto_power_cycles++;
//        }
    }

    if (!schedule->powered_off_today &&
        current_time >= schedule->power_off_time_sec &&
        current_time < (schedule->power_off_time_sec + 300)) { // 5-minute window

//        if (g_cm4_manager.current_state == CM4_STATE_ON && CM4_Control_ShouldPowerOff()) {
            BScript_Log("[CM4_Control] Auto power OFF triggered");
//            CM4_Control_SendShutdown();
//            CM4_Control_RequestOff();
            if (g_cm4_manager.manual_on_requests > 0) {
                g_cm4_manager.manual_on_requests--;
            }

            g_cm4_manager.statistics.total_manual_requests++;

            // Only power off if no more requests are active
            if (g_cm4_manager.manual_on_requests == 0 && g_cm4_manager.script_on_requests == 0) {
                if (g_cm4_manager.current_state == CM4_STATE_OFF) {
                    BScript_Log("[CM4_Control] Manual OFF request - already off");
                }
                g_cm4_manager.pending_request = CM4_REQUEST_OFF;
                BScript_Log("[CM4_Control] Manual OFF request queued");
            } else {
                BScript_Log("[CM4_Control] Manual OFF request - keeping active req (manual: %u, script: %u)",
                           g_cm4_manager.manual_on_requests, g_cm4_manager.script_on_requests);
            }

            schedule->powered_off_today = true;
//        } else if (!CM4_Control_ShouldPowerOff()) {
//            BScript_Log("[CM4_Control] Auto power OFF delayed - active requests present");
//        }
    }
}

static void CM4_Control_ProcessPendingRequest(void)
{
    if (g_cm4_manager.pending_request == CM4_REQUEST_NONE) {
        return;
    }

    CM4_Request_t request = g_cm4_manager.pending_request;
    g_cm4_manager.pending_request = CM4_REQUEST_NONE;

    switch (request) {
        case CM4_REQUEST_ON:
            if (g_cm4_manager.current_state == CM4_STATE_OFF) {
                CM4_Control_TriggerReset();
            }
            break;

        case CM4_REQUEST_OFF:
            if (g_cm4_manager.current_state == CM4_STATE_ON && CM4_Control_ShouldPowerOff()) {
                CM4_Control_SendShutdown();
            }
            break;

        case CM4_REQUEST_RESET:
            CM4_Control_TriggerReset();
            break;

        default:
            break;
    }
}

/*************************************************
 *              PUBLIC FUNCTIONS                 *
 *************************************************/

CM4_Result_t CM4_Control_Init(void)
{
    if (g_cm4_manager.initialized) {
        return CM4_RESULT_OK;
    }

    memset(&g_cm4_manager, 0, sizeof(CM4_ControlManager_t));

    g_cm4_manager.control_mutex = xSemaphoreCreateMutex();
    if (!g_cm4_manager.control_mutex) {
        BScript_Log("[CM4_Control] Error: Failed to create control mutex");
        return CM4_RESULT_ERROR;
    }


    g_cm4_manager.current_state = CM4_STATE_ON;
    g_cm4_manager.keep_alive_enabled = true;
    g_cm4_manager.auto_schedule.auto_power_enabled = true;
    g_cm4_manager.auto_schedule.power_on_time_sec = CM4_CONTROL_DEFAULT_ON_TIME;
    g_cm4_manager.auto_schedule.power_off_time_sec = CM4_CONTROL_DEFAULT_OFF_TIME;

    cm4controlmode = CM4_CONTROL_RUN;

    CM4_Control_LoadConfig();

    g_cm4_manager.initialized = true;

    BScript_Log("[CM4_Control] Initialized successfully");
    char on_time_str[16], off_time_str[16];
    CM4_Control_SecondsToTimeString(g_cm4_manager.auto_schedule.power_on_time_sec, on_time_str);
    CM4_Control_SecondsToTimeString(g_cm4_manager.auto_schedule.power_off_time_sec, off_time_str);
    BScript_Log("[CM4_Control] Auto power schedule: ON at %s, OFF at %s", on_time_str, off_time_str);

    return CM4_RESULT_OK;
}

CM4_Result_t CM4_Control_ConfigureAutoPower(uint32_t on_time_raw, uint32_t off_time_raw, bool enable)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return CM4_RESULT_TIMEOUT;
    }

    // Parse time format (0x00HHMMSS)
    uint8_t on_hh = (on_time_raw >> 16) & 0xFF;
    uint8_t on_mm = (on_time_raw >> 8) & 0xFF;
    uint8_t on_ss = on_time_raw & 0xFF;

    uint8_t off_hh = (off_time_raw >> 16) & 0xFF;
    uint8_t off_mm = (off_time_raw >> 8) & 0xFF;
    uint8_t off_ss = off_time_raw & 0xFF;

    // Validate time components
    if (on_hh > 23 || on_mm > 59 || on_ss > 59 ||
        off_hh > 23 || off_mm > 59 || off_ss > 59) {
        xSemaphoreGive(g_cm4_manager.control_mutex);
        return CM4_RESULT_ERROR;
    }

    // Convert to seconds
    uint32_t on_time_sec = on_hh * 3600 + on_mm * 60 + on_ss;
    uint32_t off_time_sec = off_hh * 3600 + off_mm * 60 + off_ss;

    // Update configuration
    g_cm4_manager.auto_schedule.power_on_time_sec = on_time_sec;
    g_cm4_manager.auto_schedule.power_off_time_sec = off_time_sec;
    g_cm4_manager.auto_schedule.auto_power_enabled = enable;

    // Reset daily flags
    g_cm4_manager.auto_schedule.powered_on_today = false;
    g_cm4_manager.auto_schedule.powered_off_today = false;

    xSemaphoreGive(g_cm4_manager.control_mutex);

    // Save to FRAM
    CM4_Control_SaveConfig();

    BScript_Log("[CM4_Control] Auto power configured: ON at %02u:%02u:%02u, OFF at %02u:%02u:%02u, Enabled: %s",
               on_hh, on_mm, on_ss, off_hh, off_mm, off_ss, enable ? "YES" : "NO");

    return CM4_RESULT_OK;
}

CM4_Result_t CM4_Control_RequestOn(void)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    	BScript_Log("[CM4_Control] SOMEONE IS KEEPING MUTEX!!");
        return CM4_RESULT_TIMEOUT;
    }

    if (g_cm4_manager.manual_on_requests > 0) {
        BScript_Log("[CM4_Control] Manual ON request - already requested (requests: %u)",
                    g_cm4_manager.manual_on_requests);
        xSemaphoreGive(g_cm4_manager.control_mutex);
        return CM4_RESULT_ALREADY_ON;
    }

    g_cm4_manager.manual_on_requests++;
    g_cm4_manager.statistics.total_manual_requests++;

    if (g_cm4_manager.current_state == CM4_STATE_ON) {
        BScript_Log("[CM4_Control] Manual ON request - already on (requests: %u)",
                   g_cm4_manager.manual_on_requests);
        xSemaphoreGive(g_cm4_manager.control_mutex);
        return CM4_RESULT_ALREADY_ON;
    }

    g_cm4_manager.pending_request = CM4_REQUEST_ON;
    BScript_Log("[CM4_Control] Manual ON request queued (requests: %u)",
               g_cm4_manager.manual_on_requests);
    xSemaphoreGive(g_cm4_manager.control_mutex);

    return CM4_RESULT_OK;
}

CM4_Result_t CM4_Control_RequestOff(void)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return CM4_RESULT_TIMEOUT;
    }

    if (g_cm4_manager.manual_on_requests > 0) {
        g_cm4_manager.manual_on_requests--;
    }

    g_cm4_manager.statistics.total_manual_requests++;

    // Only power off if no more requests are active
    if (g_cm4_manager.manual_on_requests == 0 && g_cm4_manager.script_on_requests == 0) {
        if (g_cm4_manager.current_state == CM4_STATE_OFF) {
            xSemaphoreGive(g_cm4_manager.control_mutex);
            BScript_Log("[CM4_Control] Manual OFF request - already off");
            return CM4_RESULT_ALREADY_OFF;
        }

        g_cm4_manager.pending_request = CM4_REQUEST_OFF;
        BScript_Log("[CM4_Control] Manual OFF request queued");
    } else {
        BScript_Log("[CM4_Control] Manual OFF request - keeping active req (manual: %u, script: %u)",
                   g_cm4_manager.manual_on_requests, g_cm4_manager.script_on_requests);
    }

    xSemaphoreGive(g_cm4_manager.control_mutex);
    return CM4_RESULT_OK;
}

CM4_Result_t CM4_Control_ScriptRequestOn(void)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return CM4_RESULT_TIMEOUT;
    }

    if (g_cm4_manager.script_on_requests > 0) {
        xSemaphoreGive(g_cm4_manager.control_mutex);
        BScript_Log("[CM4_Control] Script ON request - already requested (requests: %u)",
                    g_cm4_manager.script_on_requests);
        return CM4_RESULT_ALREADY_ON;
    }

    g_cm4_manager.script_on_requests++;
    if(g_cm4_manager.script_on_requests > 1){
    	g_cm4_manager.script_on_requests = 1;
    }

    if (g_cm4_manager.current_state == CM4_STATE_ON) {
        xSemaphoreGive(g_cm4_manager.control_mutex);
        BScript_Log("[CM4_Control] Script ON request - already on (requests: %u)",
                   g_cm4_manager.script_on_requests);
        return CM4_RESULT_ALREADY_ON;
    }

    g_cm4_manager.pending_request = CM4_REQUEST_ON;
    xSemaphoreGive(g_cm4_manager.control_mutex);

    BScript_Log("[CM4_Control] Script ON request queued (requests: %u)",
               g_cm4_manager.script_on_requests);
    return CM4_RESULT_OK;
}

CM4_Result_t CM4_Control_ScriptRequestOff(void)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return CM4_RESULT_TIMEOUT;
    }

    if (g_cm4_manager.script_on_requests > 0) {
        g_cm4_manager.script_on_requests--;
    }

    // Only power off if no more requests are active
    if (g_cm4_manager.manual_on_requests == 0 && g_cm4_manager.script_on_requests == 0) {
        if (g_cm4_manager.current_state == CM4_STATE_OFF) {
            xSemaphoreGive(g_cm4_manager.control_mutex);
            BScript_Log("[CM4_Control] Script OFF request - already off");
            return CM4_RESULT_ALREADY_OFF;
        }

        g_cm4_manager.pending_request = CM4_REQUEST_OFF;
        BScript_Log("[CM4_Control] Script OFF request queued");
    } else {
        BScript_Log("[CM4_Control] Script OFF request - keeping active req (manual: %u, script: %u)",
                   g_cm4_manager.manual_on_requests, g_cm4_manager.script_on_requests);
    }

    xSemaphoreGive(g_cm4_manager.control_mutex);
    return CM4_RESULT_OK;
}

CM4_Result_t CM4_Control_ForceReset(void)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return CM4_RESULT_TIMEOUT;
    }

    BScript_Log("[CM4_Control] Force reset requested");
    CM4_Control_TriggerReset();

    xSemaphoreGive(g_cm4_manager.control_mutex);
    return CM4_RESULT_OK;
}

CM4_Result_t CM4_Control_ForceShutdown(void)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return CM4_RESULT_TIMEOUT;
    }

    BScript_Log("[CM4_Control] Force shutdown requested");
    CM4_Control_SendShutdown();

    xSemaphoreGive(g_cm4_manager.control_mutex);
    return CM4_RESULT_OK;
}

CM4_State_t CM4_Control_GetState(void)
{
    return g_cm4_manager.current_state;
}

bool CM4_Control_IsOn(void)
{
    return (g_cm4_manager.current_state == CM4_STATE_ON);
}

void CM4_Control_SetKeepAliveEnabled(bool enable)
{
    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        g_cm4_manager.keep_alive_enabled = enable;
        xSemaphoreGive(g_cm4_manager.control_mutex);

        BScript_Log("[CM4_Control] Keep-alive %s", enable ? "ENABLED" : "DISABLED");
    }
}

bool CM4_Control_IsKeepAliveEnabled(void)
{
    return g_cm4_manager.keep_alive_enabled;
}

void CM4_Control_GetStatistics(CM4_Statistics_t* stats)
{
    if (!stats) return;

    if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        *stats = g_cm4_manager.statistics;
        xSemaphoreGive(g_cm4_manager.control_mutex);
    }
}

void CM4_Control_PrintStatus(void)
{
    char on_time_str[16], off_time_str[16];
    CM4_Control_SecondsToTimeString(g_cm4_manager.auto_schedule.power_on_time_sec, on_time_str);
    CM4_Control_SecondsToTimeString(g_cm4_manager.auto_schedule.power_off_time_sec, off_time_str);

    s_DateTime current_rtc;
    Utils_GetRTC(&current_rtc);

    BScript_Log("[CM4_Control] === CM4 CONTROL STATUS ===");
    BScript_Log("[CM4_Control] Current RTC: 20%02d-%02d-%02d %02d:%02d:%02d",
               current_rtc.year, current_rtc.month, current_rtc.day,
               current_rtc.hour, current_rtc.minute, current_rtc.second);
    BScript_Log("[CM4_Control] Current State: %s", CM4_Control_StateToString(g_cm4_manager.current_state));
    BScript_Log("[CM4_Control] Pending Request: %d", g_cm4_manager.pending_request);
    BScript_Log("[CM4_Control] Manual ON Requests: %u", g_cm4_manager.manual_on_requests);
    BScript_Log("[CM4_Control] Script ON Requests: %u", g_cm4_manager.script_on_requests);

    BScript_Log("[CM4_Control] Auto Power Management:");
    BScript_Log("  - Enabled: %s", g_cm4_manager.auto_schedule.auto_power_enabled ? "YES" : "NO");
    BScript_Log("  - Power ON Time: %s", on_time_str);
    BScript_Log("  - Power OFF Time: %s", off_time_str);
    BScript_Log("  - Powered ON Today: %s", g_cm4_manager.auto_schedule.powered_on_today ? "YES" : "NO");
    BScript_Log("  - Powered OFF Today: %s", g_cm4_manager.auto_schedule.powered_off_today ? "YES" : "NO");

    BScript_Log("[CM4_Control] Statistics:");
    BScript_Log("  - Total Resets: %u", g_cm4_manager.statistics.total_resets);
    BScript_Log("  - Total Auto Power Cycles: %u", g_cm4_manager.statistics.total_auto_power_cycles);
    BScript_Log("  - Total Manual Requests: %u", g_cm4_manager.statistics.total_manual_requests);
    BScript_Log("  - Last Reset: %lu ticks ago",
               xTaskGetTickCount() - g_cm4_manager.statistics.last_reset_time);

    BScript_Log("[CM4_Control] === END STATUS ===");
}

bool CM4_Control_SaveConfig(void)
{
    FRAM_SPI_HandleTypeDef* fram_handle = FRAM_SPI_GetHandle();
    if (!fram_handle) {
        return false;
    }

    uint8_t config_data[FRAM_CM4_CONFIG_SIZE];

    // Pack configuration data
    config_data[0] = (uint8_t)(g_cm4_manager.auto_schedule.power_on_time_sec & 0xFF);
    config_data[1] = (uint8_t)((g_cm4_manager.auto_schedule.power_on_time_sec >> 8) & 0xFF);
    config_data[2] = (uint8_t)((g_cm4_manager.auto_schedule.power_on_time_sec >> 16) & 0xFF);
    config_data[3] = (uint8_t)((g_cm4_manager.auto_schedule.power_on_time_sec >> 24) & 0xFF);

    config_data[4] = (uint8_t)(g_cm4_manager.auto_schedule.power_off_time_sec & 0xFF);
    config_data[5] = (uint8_t)((g_cm4_manager.auto_schedule.power_off_time_sec >> 8) & 0xFF);
    config_data[6] = (uint8_t)((g_cm4_manager.auto_schedule.power_off_time_sec >> 16) & 0xFF);
    config_data[7] = (uint8_t)((g_cm4_manager.auto_schedule.power_off_time_sec >> 24) & 0xFF);

    config_data[8] = g_cm4_manager.auto_schedule.auto_power_enabled ? 1 : 0;

    Std_ReturnType result = FRAM_SPI_WriteMem(fram_handle, FRAM_CM4_ON_TIME_ADDR, config_data, FRAM_CM4_CONFIG_SIZE);

    if (result == E_OK) {
        BScript_Log("[CM4_Control] Configuration saved to FRAM successfully");
        return true;
    } else {
        BScript_Log("[CM4_Control] Failed to save configuration to FRAM (status: %d)", result);
        return false;
    }
}

bool CM4_Control_LoadConfig(void)
{
    FRAM_SPI_HandleTypeDef* fram_handle = FRAM_SPI_GetHandle();
    if (!fram_handle) {
        return false;
    }

    uint8_t config_data[FRAM_CM4_CONFIG_SIZE];
    Std_ReturnType result = FRAM_SPI_ReadMem(fram_handle, FRAM_CM4_ON_TIME_ADDR, config_data, FRAM_CM4_CONFIG_SIZE);

    if (result != E_OK) {
        BScript_Log("[CM4_Control] Failed to read configuration from FRAM (status: %d)", result);
        return false;
    }

    // Unpack configuration data
    uint32_t on_time = ((uint32_t)config_data[3] << 24) |
                      ((uint32_t)config_data[2] << 16) |
                      ((uint32_t)config_data[1] << 8) |
                      config_data[0];

    uint32_t off_time = ((uint32_t)config_data[7] << 24) |
                       ((uint32_t)config_data[6] << 16) |
                       ((uint32_t)config_data[5] << 8) |
                       config_data[4];

    bool auto_enable = (config_data[8] != 0);

    // Validate loaded data
    if (on_time == 0xFFFFFFFF || off_time == 0xFFFFFFFF || on_time >= 86400 || off_time >= 86400) {
        BScript_Log("[CM4_Control] Invalid configuration in FRAM, using defaults");
        return false;
    }

    if (on_time == 0x00000000 || off_time == 0x00000000 || on_time >= 86400 || off_time >= 86400) {
        BScript_Log("[CM4_Control] Invalid configuration in FRAM, using defaults");
        return false;
    }

    // Apply loaded configuration
    g_cm4_manager.auto_schedule.power_on_time_sec = on_time;
    g_cm4_manager.auto_schedule.power_off_time_sec = off_time;
    g_cm4_manager.auto_schedule.auto_power_enabled = auto_enable;

    char on_time_str[16], off_time_str[16];
    CM4_Control_SecondsToTimeString(on_time, on_time_str);
    CM4_Control_SecondsToTimeString(off_time, off_time_str);

    BScript_Log("[CM4_Control] Configuration loaded from FRAM: ON at %s, OFF at %s, Enabled: %s",
               on_time_str, off_time_str, auto_enable ? "YES" : "NO");

    return true;
}

void CM4_Control_Task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    BScript_Log("[CM4_Control] Control task started");

    while (1) {

        if (!g_cm4_manager.auto_schedule.auto_power_enabled) {
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
            continue;
        }

        if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {

            // Reset daily flags at midnight
            CM4_Control_ResetDailyFlags();

            // Handle automatic power management
            CM4_Control_HandleAutoPower();

            // Process pending requests
            CM4_Control_ProcessPendingRequest();

            // Handle state transitions
            switch (g_cm4_manager.current_state) {
                case CM4_STATE_BOOTING:
                    // Transition to ON after boot timeout (assume successful boot)
                    if ((xTaskGetTickCount() - g_cm4_manager.statistics.last_reset_time) >
                        pdMS_TO_TICKS(30000)) { // 30 second boot timeout
                        CM4_Control_UpdateState(CM4_STATE_ON);
                        g_cm4_manager.statistics.last_response_time = xTaskGetTickCount();
                    }
                    break;

                case CM4_STATE_SHUTTING_DOWN:
                    // Transition to OFF after shutdown timeout
                    if ((xTaskGetTickCount() - g_cm4_manager.statistics.last_response_time) >
                        pdMS_TO_TICKS(CM4_CONTROL_SHUTDOWN_TIMEOUT_MS)) {
                        CM4_Control_UpdateState(CM4_STATE_OFF);
                    }
                    break;

                default:
                    break;
            }

            xSemaphoreGive(g_cm4_manager.control_mutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

//#define KEEPALIVE_OUT_PORT    STMOUT_CM4IN_SDA_GPIO_Port
//#define KEEPALIVE_OUT_PIN     STMOUT_CM4IN_SDA_Pin
//
//#define KEEPALIVE_IN_PORT     CM4OUT_STMIN_D0_GPIO_Port
//#define KEEPALIVE_IN_PIN      CM4OUT_STMIN_D0_Pin
//
//#define CM4_RST_PORT          MCU_IO_RESET_CM4_GPIO_Port
//#define CM4_RST_PIN           MCU_IO_RESET_CM4_Pin
//
//void CM4_KeepAlive_Task(void *pvParameters)
//{
//    TickType_t xLastWakeTime = xTaskGetTickCount();
//
//    BScript_Log("[CM4_Control] Keep-alive task started");
//
//    while (1) {
//        // Early return if keep-alive is disabled or CM4 is off
//        if (!g_cm4_manager.keep_alive_enabled || g_cm4_manager.current_state == CM4_STATE_OFF) {
//            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CM4_CONTROL_CHECK_INTERVAL_MS));
//            continue;
//        }
//
//        // Skip keep-alive during boot and shutdown states
//        if (g_cm4_manager.current_state == CM4_STATE_BOOTING ||
//            g_cm4_manager.current_state == CM4_STATE_SHUTTING_DOWN) {
//            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CM4_CONTROL_CHECK_INTERVAL_MS));
//            continue;
//        }
//
//        // Perform keep-alive handshake
//        GPIO_SetLow(KEEPALIVE_OUT_PORT, KEEPALIVE_OUT_PIN);
//
//        uint8_t respondedLow = 0;
//        TickType_t start = xTaskGetTickCount();
//        while (xTaskGetTickCount() - start < pdMS_TO_TICKS(CM4_CONTROL_RESPONSE_TIMEOUT_MS)) {
//            if (GPIO_IsInLow(KEEPALIVE_IN_PORT, KEEPALIVE_IN_PIN)) {
//                respondedLow = 1;
//                break;
//            }
//            vTaskDelay(pdMS_TO_TICKS(50));
//        }
//
//        GPIO_SetHigh(KEEPALIVE_OUT_PORT, KEEPALIVE_OUT_PIN);
//
//        uint8_t respondedHigh = 0;
//        start = xTaskGetTickCount();
//        while (xTaskGetTickCount() - start < pdMS_TO_TICKS(CM4_CONTROL_RESPONSE_TIMEOUT_MS)) {
//            if (GPIO_IsInHigh(KEEPALIVE_IN_PORT, KEEPALIVE_IN_PIN)) {
//                respondedHigh = 1;
//                break;
//            }
//            vTaskDelay(pdMS_TO_TICKS(50));
//        }
//
//        // Process keep-alive result
//        if (xSemaphoreTake(g_cm4_manager.control_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
//            if (respondedLow && respondedHigh) {
//                g_cm4_manager.keep_alive_miss_count = 0;
//                g_cm4_manager.statistics.last_response_time = xTaskGetTickCount();
//            } else {
//                g_cm4_manager.keep_alive_miss_count++;
//                g_cm4_manager.statistics.total_keep_alive_failures++;
//
//                LWL_Log(OBC_STM32_CM4_MISS_COUNT, LWL_2(g_cm4_manager.keep_alive_miss_count));
//
//                BScript_Log("[CM4_Control] Keep-alive miss %u/%u (Low: %s, High: %s)",
//                           g_cm4_manager.keep_alive_miss_count, CM4_CONTROL_MAX_RETRY_COUNT,
//                           respondedLow ? "OK" : "FAIL", respondedHigh ? "OK" : "FAIL");
//
//                if (g_cm4_manager.keep_alive_miss_count >= CM4_CONTROL_MAX_RETRY_COUNT) {
//                    BScript_Log("[CM4_Control] Keep-alive failed %u times, triggering reset",
//                               CM4_CONTROL_MAX_RETRY_COUNT);
//                    CM4_Control_TriggerReset();
//                    g_cm4_manager.keep_alive_miss_count = 0;
//                }
//            }
//
//            xSemaphoreGive(g_cm4_manager.control_mutex);
//        }
//
//        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(CM4_CONTROL_CHECK_INTERVAL_MS));
//    }
//}

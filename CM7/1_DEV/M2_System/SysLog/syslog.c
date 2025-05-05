/*
 * syslog.c
 *
 *  Created on: Feb 27, 2025
 *      Author: CAO HIEU
 */

#include "syslog.h"
#include "stdio.h"
#include "uart_driver_dma.h"
#include "utils.h"
#include "DateTime/date_time.h"

static USART_TypeDef* syslog_uarts[SYSLOG_OUTPUT_UART_COUNT] = SYSLOG_OUTPUT_UARTS;

static const char* syslog_level_to_str(syslog_level_t level)
{
    switch(level) {
        case LOG_INFOR:  return "[INFO]  ";
        case LOG_DEBUG:  return "[DEBUG] ";
        case LOG_NOTICE: return "[NOTICE]";
        case LOG_WARN:   return "[WARN]  ";
        case LOG_ERROR:  return "[ERROR] ";
        case LOG_FATAL:  return "[FATAL] ";
        default:         return "[UNK]   ";
    }
}

/*
 * Function syslog_log:
 * - If SYSLOG_USE_RTC is enabled, retrieves RTC time and formats it as: "20YY-MM-DD HH:MM:SS "
 * - If SYSLOG_USE_WORKING_TIME is enabled, retrieves working time and formats it as:
 *   "HH:MM:SS " (or "days+HH:MM:SS " if days > 0)
 * - Then, it appends the log level in the format "[LEVEL] " and the source (if configured) "[SYSLOG_SOURCE] "
 * - Finally, it appends the message content enclosed in quotes, followed by a newline.
 * Example output:
 *    2024-02-22 23:40:03 0:05:03 [NOTICE] [OBC-STM32] "Start up"
 */
void syslog_log(syslog_level_t level, const char *msg, int use_polling)
{
    char log_buffer[128];
    int offset = 0;

#if SYSLOG_USE_RTC
    s_DateTime rtc;
    Utils_GetRTC(&rtc);
    offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                "20%02d-%02d-%02d %02d:%02d:%02d ",
                rtc.year, rtc.month, rtc.day, rtc.hour, rtc.minute, rtc.second);
#endif

#if SYSLOG_USE_WORKING_TIME
    uint8_t hours = 0, minutes = 0, seconds = 0;
    Utils_GetWorkingTime(NULL, &hours, &minutes, &seconds);
    offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                    "%02u:%02u:%02u ",
                    hours, minutes, seconds);
#endif

    offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                "%s ", syslog_level_to_str(level));

#ifdef SYSLOG_SOURCE
    offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                "[%s] ", SYSLOG_SOURCE);
#endif

    offset += snprintf(log_buffer + offset, sizeof(log_buffer) - offset,
                "\"%s\"\r\n", msg);

#ifdef DEBUG_USE_UART
    for (int i = 0; i < SYSLOG_OUTPUT_UART_COUNT; i++) {
        if (use_polling) {
            UART_Driver_Polling_SendString(syslog_uarts[i], log_buffer);
        } else {
            UART_Driver_SendString(syslog_uarts[i], log_buffer);
        }
    }
#endif
}

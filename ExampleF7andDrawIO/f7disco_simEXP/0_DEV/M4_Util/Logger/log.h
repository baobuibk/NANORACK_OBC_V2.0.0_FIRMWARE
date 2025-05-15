/*
 * log.h
 *
 *  Created on: Apr 17, 2025
 *      Author: CAO HIEU
 */

#ifndef M4_UTIL_LOGGER_LOG_H_
#define M4_UTIL_LOGGER_LOG_H_

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdint.h>
#include "DateTime/date_time.h"

#define LOG_PRINTF

#ifdef LOG_PRINTF

#ifndef INVITATION
#define INVITATION "F7_Disco"
#endif

void Log_Printf(const char *format, ...);

#define LOG(fmt, ...)                                                                                      \
    do {                                                                                                   \
        uint32_t _log_days;                                                                                \
        uint8_t _log_hours, _log_minutes, _log_seconds;                                                    \
        Utils_GetWorkingTime(&_log_days, &_log_hours, &_log_minutes, &_log_seconds);                       \
        printf("[%02d:%02d:%02d]-[%s]: " fmt "\r\n",                                                       \
               _log_hours, _log_minutes, _log_seconds, INVITATION, ##__VA_ARGS__);                         \
    } while (0)

#else

#define LOG(fmt, ...) do {;} while (0)
#endif // LOG_PRINTF

#endif // __LOG_H__


#endif /* M4_UTIL_LOGGER_LOG_H_ */

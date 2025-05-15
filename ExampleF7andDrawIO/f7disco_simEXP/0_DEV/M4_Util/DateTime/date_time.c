/*
 * date_time.c
 *
 *  Created on: Feb 24, 2025
 *      Author: CAO HIEU
 */
#include "date_time.h"

#define NULL ((void *)0)

static s_DateTime s_RealTimeClock_context = {1, 1, 0, 0, 0, 0};

static struct
{
    uint32_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} s_WorkingTimeClock_context = {0, 0, 0, 0};

static inline uint8_t isLeapYear(uint16_t fullYear)
{
    return ((fullYear % 4 == 0) && ((fullYear % 100 != 0) || (fullYear % 400 == 0))) ? 1 : 0;
}

static inline uint8_t getMaxDays(uint8_t month, uint16_t fullYear)
{
    static const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) {
        return 28 + isLeapYear(fullYear);
    } else {
        return daysInMonth[month - 1];
    }
}

static uint32_t DateTimeToEpoch(const s_DateTime *dt)
{
    uint32_t days = 0;
    uint16_t fullYear = 2000 + dt->year;

    for (uint16_t year = 2000; year < fullYear; year++)
    {
        days += 365 + isLeapYear(year);
    }
    for (uint8_t m = 1; m < dt->month; m++)
    {
        days += getMaxDays(m, fullYear);
    }
    days += dt->day - 1;

    return days * 86400UL + dt->hour * 3600UL + dt->minute * 60UL + dt->second;
}

void EpochToDateTime(uint32_t epoch, s_DateTime *dt)
{
    uint32_t days = epoch / 86400;
    uint32_t remSeconds = epoch % 86400;

    dt->hour   = remSeconds / 3600;
    remSeconds %= 3600;
    dt->minute = remSeconds / 60;
    dt->second = remSeconds % 60;

    uint16_t year = 2000;
    while (1)
    {
        uint16_t daysInYear = 365 + isLeapYear(year);
        if (days >= daysInYear)
        {
            days -= daysInYear;
            year++;
        } else
        {
            break;
        }
    }
    dt->year = year - 2000;

    uint8_t month = 1;
    while (1)
    {
        uint8_t dim = getMaxDays(month, year);
        if (days >= dim)
        {
            days -= dim;
            month++;
        }
        else
        {
            break;
        }
    }
    dt->month = month;
    dt->day = days + 1;
}

void Utils_SoftTime_Update(void)
{
    static uint16_t ms_counter = 0;

    if (++ms_counter >= 1000) // 1000 ms = 1 second
    {
        ms_counter = 0;

        // Update real-time clock
        if (++s_RealTimeClock_context.second >= 60)
        {
            s_RealTimeClock_context.second = 0;
            if (++s_RealTimeClock_context.minute >= 60)
            {
                s_RealTimeClock_context.minute = 0;
                if (++s_RealTimeClock_context.hour >= 24)
                {
                    s_RealTimeClock_context.hour = 0;
                    if (++s_RealTimeClock_context.day > getMaxDays(s_RealTimeClock_context.month, s_RealTimeClock_context.year))
                    {
                        s_RealTimeClock_context.day = 1;
                        if (++s_RealTimeClock_context.month > 12)
                        {
                            s_RealTimeClock_context.month = 1;
                            if (++s_RealTimeClock_context.year > 99)
                            {
                                s_RealTimeClock_context.year = 0;
                            }
                        }
                    }
                }
            }
        }

        // Update working time clock
        if (++s_WorkingTimeClock_context.seconds >= 60)
        {
            s_WorkingTimeClock_context.seconds = 0;
            if (++s_WorkingTimeClock_context.minutes >= 60)
            {
                s_WorkingTimeClock_context.minutes = 0;
                if (++s_WorkingTimeClock_context.hours >= 24)
                {
                    s_WorkingTimeClock_context.hours = 0;
                    s_WorkingTimeClock_context.days++;
                }
            }
        }
    }
}


void Utils_SoftTime_Init(void)
{
    s_RealTimeClock_context.year = 0;  // 2000
    s_RealTimeClock_context.month = 1; // January
    s_RealTimeClock_context.day = 1;
    s_RealTimeClock_context.hour = 0;
    s_RealTimeClock_context.minute = 0;
    s_RealTimeClock_context.second = 0;

    s_WorkingTimeClock_context.days = 0;
    s_WorkingTimeClock_context.hours = 0;
    s_WorkingTimeClock_context.minutes = 0;
    s_WorkingTimeClock_context.seconds = 0;
}

// ================= Helper Functions =================
void Utils_GetRTC(s_DateTime *dateTime)
{
    if (dateTime == NULL) return;
    *dateTime = s_RealTimeClock_context;
}

void Utils_SetRTC(const s_DateTime *dateTime)
{
    if (dateTime == NULL) return;
    s_RealTimeClock_context = *dateTime;
}
/*@usage:
 *  s_DateTime newTime = {15, 10, 23, 14, 30, 0}; // 15/10/2023 14:30:00
 *  DateTime_SetRTC(&newTime);
 */

void Utils_SetEpoch(uint32_t epoch)
{
    s_DateTime dt;
    if (epoch < EPOCH_OFFSET_UNIX)
    {
        return;
    }
    EpochToDateTime(epoch - EPOCH_OFFSET_UNIX, &dt);
    Utils_SetRTC(&dt);
}

uint32_t Utils_GetEpoch(void)
{
    return DateTimeToEpoch(&s_RealTimeClock_context) + EPOCH_OFFSET_UNIX;
}

void Utils_GetWorkingTime(uint32_t *days, uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    if (days) *days = s_WorkingTimeClock_context.days;
    if (hours) *hours = s_WorkingTimeClock_context.hours;
    if (minutes) *minutes = s_WorkingTimeClock_context.minutes;
    if (seconds) *seconds = s_WorkingTimeClock_context.seconds;
}

#if USE_EXTERNAL_RTC
Std_ReturnType Utils_SoftTime_Sync(void)
{
	s_DateTime currentTime;
	RV3129_HandleTypeDef *hrtc = RV3129_GetHandle();
	Std_ReturnType ret = E_ERROR;
	ret = RV3129_GetTime(hrtc, &currentTime);
    if(ret == E_OK)
    {
       Utils_SetRTC(&currentTime);
    }
    return ret;
}
#endif

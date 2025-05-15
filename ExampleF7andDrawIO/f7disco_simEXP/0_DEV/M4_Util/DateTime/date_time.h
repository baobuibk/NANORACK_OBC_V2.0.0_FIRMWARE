#ifndef M4_UTILS_DATETIME_DATE_TIME_H_
#define M4_UTILS_DATETIME_DATE_TIME_H_
#include "stdint.h"

#define USE_EXTERNAL_RTC 0
#define EPOCH_OFFSET_UNIX 946684800UL  //  01/01/1970 - 01/01/2000

typedef struct {
	uint8_t day;    //!< Day: Starting at 1 for the first day
	uint8_t month;  //!< Month: Starting at 1 for January
	uint8_t year;   //!< Year in format YY (2000 - 2099)
	uint8_t hour;   //!< Hour
	uint8_t minute; //!< Minute
	uint8_t second; //!< Second
} s_DateTime;

void Utils_SoftTime_Update(void);
void EpochToDateTime(uint32_t epoch, s_DateTime *dt);
void Utils_GetRTC(s_DateTime *dateTime);
void Utils_SetRTC(const s_DateTime *dateTime);
void Utils_SetEpoch(uint32_t epoch);
uint32_t Utils_GetEpoch(void);
void Utils_GetWorkingTime(uint32_t *days, uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
void Utils_SoftTime_Init(void);

#if USE_EXTERNAL_RTC
#include "devices.h"
Std_ReturnType Utils_SoftTime_Sync(void);
#endif

#endif /* M4_UTILS_DATETIME_DATE_TIME_H_ */

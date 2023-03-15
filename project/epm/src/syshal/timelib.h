/**************************************************************************//**
* @file     timelib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __TIME_LIB_H__
#define __TIME_LIB_H__
#include <time.h>
#include "nuc970.h"
#include "rtc.h"
#ifdef __cplusplus
extern "C"
{
#endif


/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//#include        "loc_time.h"
#define YEAR0               1900  /* the first year */
#define EPOCH_YR            1970  /* EPOCH = Jan 1 1970 00:00:00 */
#define SECS_DAY            (24L * 60L * 60L)
#define LEAPYEAR(year)      (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)      (LEAPYEAR(year) ? 366 : 365)
#define FIRSTSUNDAY(timp)    (((timp)->tm_yday - (timp)->tm_wday + 420) % 7)
#define FIRSTDAYOF(timp)    (((timp)->tm_wday - (timp)->tm_yday + 420) % 7)
#define TIME_MAX            ULONG_MAX
#define ABB_LEN 3

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
time_t RTC2Time(RTC_TIME_DATA_T* pt);
RTC_TIME_DATA_T* Time2RTC(time_t rawtime, RTC_TIME_DATA_T* ppt);
BOOL RTCAddTime(RTC_TIME_DATA_T* pt, time_t addTime);
time_t RTCAddTimeEx(time_t addTime);
BOOL SetOSTime(uint32_t u32Year, uint32_t u32cMonth, uint32_t u32cDay, uint32_t u32cHour, uint32_t u32cMinute, uint32_t u32cSecond, uint32_t u32cDayOfWeek);
BOOL SetOSTimeLite(uint32_t u32Year, uint32_t u32cMonth, uint32_t u32cDay, uint32_t u32cHour, uint32_t u32cMinute, uint32_t u32cSecond);
time_t GetCurrentUTCTime(void);
BOOL UTCTimeToString(time_t time, char* str);
BOOL UTCTimeToStringEx(time_t time, char* str);
void PrintRTC(void);
#ifdef __cplusplus
}
#endif

#endif//__TIME_LIB_H__

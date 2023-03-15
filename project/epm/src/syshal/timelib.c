/**************************************************************************//**
* @file     timelib.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "nuc970.h"
#include "sys.h"
#include "rtc.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "timelib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static struct tm timeinfo;
static struct tm* ptimeinfo;
static const int _ytab[2][12] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/



/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/


struct tm* fepgmtime(register const time_t time)
{
    //sysprintf("\r\n-- fepgmtime :%d !!! \r\n", time);
    static struct tm br_time;
    register struct tm *timep = &br_time;
    //time_t time = *timer;
    register unsigned long dayclock, dayno;
    int year = EPOCH_YR;

    dayclock = (unsigned long)time % SECS_DAY;
    dayno = (unsigned long)time / SECS_DAY;

    timep->tm_sec = dayclock % 60;
    timep->tm_min = (dayclock % 3600) / 60;
    timep->tm_hour = dayclock / 3600;
    timep->tm_wday = (dayno + 4) % 7;       /* day 0 was a thursday */
    while (dayno >= YEARSIZE(year)) 
    {
        dayno -= YEARSIZE(year);
        year++;
    }
    timep->tm_year = year - YEAR0;
    timep->tm_yday = dayno;
    timep->tm_mon = 0;
    while (dayno >= _ytab[LEAPYEAR(year)][timep->tm_mon]) 
    {
        dayno -= _ytab[LEAPYEAR(year)][timep->tm_mon];
        timep->tm_mon++;
    }
    timep->tm_mday = dayno + 1;
    timep->tm_isdst = 0;

    return timep;
}

time_t RTC2Time(RTC_TIME_DATA_T* pt)
{
    time_t rawtime;
    timeinfo.tm_year    = pt->u32Year - YEAR0;
    timeinfo.tm_mon     = pt->u32cMonth-1;
    timeinfo.tm_mday    = pt->u32cDay;
    timeinfo.tm_hour    = pt->u32cHour;
    timeinfo.tm_min     = pt->u32cMinute;
    timeinfo.tm_sec     = pt->u32cSecond;                
    timeinfo.tm_wday    = pt->u32cDayOfWeek;
    timeinfo.tm_yday    = 0;
    timeinfo.tm_isdst   = 0;
                
    rawtime = mktime(&timeinfo);
    return rawtime;
}

RTC_TIME_DATA_T* Time2RTC(time_t rawtime, RTC_TIME_DATA_T* ppt)
{
    //static RTC_TIME_DATA_T pt;
    //RTC_TIME_DATA_T* ppt = &pt;
    //sysprintf("\r\n-- Time2RTC :%d !!! \r\n", rawtime);
    ptimeinfo = fepgmtime(rawtime);
    ppt->u32Year = ptimeinfo->tm_year + YEAR0;
    ppt->u32cMonth = ptimeinfo->tm_mon + 1;
    ppt->u32cDay = ptimeinfo->tm_mday;
    ppt->u32cHour = ptimeinfo->tm_hour;
    ppt->u32cMinute = ptimeinfo->tm_min;
    ppt->u32cSecond = ptimeinfo->tm_sec;                
    ppt->u32cDayOfWeek = ptimeinfo->tm_wday;   
    return ppt;
}

BOOL RTCAddTime(RTC_TIME_DATA_T* pt, time_t addTime)
{
    //RTC_TIME_DATA_T* ppt;
    time_t rawtime = RTC2Time(pt);
    rawtime = rawtime + addTime;
    Time2RTC(rawtime, pt);
    //pt->u32Year = ppt->u32Year;
    //pt->u32cMonth = ppt->u32cMonth;
    //pt->u32cDay = ppt->u32cDay;
    //pt->u32cHour = ppt->u32cHour;
    //pt->u32cMinute = ppt->u32cMinute;
    //pt->u32cSecond = ppt->u32cSecond;                
    //pt->u32cDayOfWeek = ppt->u32cDayOfWeek;   
    return TRUE;
}

time_t RTCAddTimeEx(time_t addTime)
{
    RTC_TIME_DATA_T pt;
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        time_t rawtime = RTC2Time(&pt);   
        return (rawtime + addTime);        
    }
      
    return 0;
}

BOOL SetOSTime(uint32_t u32Year, uint32_t u32cMonth, uint32_t u32cDay, uint32_t u32cHour, uint32_t u32cMinute, uint32_t u32cSecond, uint32_t u32cDayOfWeek)
{
    UINT32 reval;
    RTC_TIME_DATA_T pt;
    RTC_TIME_DATA_T sInitTime;
    sInitTime.u32Year = u32Year;
    sInitTime.u32cMonth = u32cMonth;
    sInitTime.u32cDay = u32cDay;
    sInitTime.u32cHour = u32cHour;
    sInitTime.u32cMinute = u32cMinute;
    sInitTime.u32cSecond = u32cSecond;
    sInitTime.u32cDayOfWeek = u32cDayOfWeek;
    sInitTime.u8cClockDisplay = RTC_CLOCK_24;

    /* Initialization the RTC timer */
    reval = RTC_Open(&sInitTime);
    RTC_Close();
    if(reval !=E_RTC_SUCCESS)
    {
        sysprintf("SetOSTime RTC Open Fail!!(reval = %d)\n", reval);
        
        return FALSE;
    }   

    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {

        sysprintf("SetOSTime RTC_CURRENT_TIME: [%04d/%02d/%02d %02d:%02d:%02d (%d)  u8cClockDisplay = %d, u8cAmPm =%d]\r\n",
                                                pt.u32Year, pt.u32cMonth, pt.u32cDay, 
                                                pt.u32cHour, pt.u32cMinute, pt.u32cSecond, pt.u32cDayOfWeek, pt.u8cClockDisplay, pt.u8cAmPm); 
    }
    return TRUE;
}


BOOL SetOSTimeLite(uint32_t u32Year, uint32_t u32cMonth, uint32_t u32cDay, uint32_t u32cHour, uint32_t u32cMinute, uint32_t u32cSecond)
{
    uint32_t tempDayOfWeek = inp32(REG_RTC_WEEKDAY);
    //terninalPrintf("RTC:%d/%d/%d/ %d:%d:%d  Weekday = %d\r\n",u32Year, u32cMonth,u32cDay,u32cHour,u32cMinute,u32cSecond,tempDayOfWeek);
   return SetOSTime(u32Year,u32cMonth,u32cDay,u32cHour,u32cMinute,u32cSecond,tempDayOfWeek);
}

time_t GetCurrentUTCTime(void)
{
    RTC_TIME_DATA_T pt;
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        return RTC2Time(&pt);
    }
    return 0;
}

BOOL UTCTimeToString(time_t time, char* str)
{
    RTC_TIME_DATA_T pt;
    //sysprintf("\r\nUTCTimeToString[%d]\n", time);
    Time2RTC(time, &pt);
    sprintf(str,"%04d%02d%02d%02d%02d", pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute); 
    //sysprintf("\r\nUTCTimeToString[%s]\n", str);
    return TRUE;
}

BOOL UTCTimeToStringEx(time_t time, char* str)
{
    RTC_TIME_DATA_T pt;
    //sysprintf("\r\nUTCTimeToString[%d]\n", time);
    //terninalPrintf("GetCurrentUTCTime = %d\r\n",time);
    Time2RTC(time, &pt);
    //sprintf(str,"%02d%02d%02d%02d%02d%02d", pt.u32Year-2000, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond); 
    sprintf(str,"%02d%02d%02d%02d%02d",pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond); 
    //sysprintf("\r\nUTCTimeToString[%s]\n", str);
    return TRUE;
}


void PrintRTC(void)
{
    RTC_TIME_DATA_T pt;
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {

        sysprintf("RTC_CURRENT_TIME: [%04d/%02d/%02d %02d:%02d:%02d (%d)  u8cClockDisplay = %d, u8cAmPm =%d]\r\n",
                                                            pt.u32Year, pt.u32cMonth, pt.u32cDay, 
                                                            pt.u32cHour, pt.u32cMinute, pt.u32cSecond, pt.u32cDayOfWeek, pt.u8cClockDisplay, pt.u8cAmPm); 
    }
}

/*** (C) COPYRIGHT 2016 Far Easy Pass LTD. All rights reserved. ***/


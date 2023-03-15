/**************************************************************************//**
* @file     tarifflib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __TARIFF_LIB_H__
#define __TARIFF_LIB_H__
#include <time.h>
#include "nuc970.h"

#include "fileagent.h"
#include "rtc.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

    
#define TARIFF_FILE_NUM  3
    
    
#define TARIFF_WEEK_NUM  7
#define TARIFF_DAY_TYPE_NUM  7
#define TARIFF_DAY_TYPE_ITEM_NUM  6
#define TARIFF_TARIFF_SETTING_NUM  10
    
#define TARIFF_TARIFF_TYPE_FREE     0x0
#define TARIFF_TARIFF_TYPE_OFF      0x1
#define TARIFF_TARIFF_TYPE_LINEAR   0x2
    
typedef struct
{
    int type;
    int cost;
    int timeunit;
    int basecost;
    int costunit;
    int maxcost;
    int maxtime;
    int continuedtime;
}JsonTariffSetting; 

typedef struct
{    
    int     tariffsetting;
    int     hour;
    int     minute;   
    BOOL    continued;
}JsonTariffDayTypeItem; 

typedef struct
{
    JsonTariffDayTypeItem    dayTypeItem[TARIFF_DAY_TYPE_ITEM_NUM];
}JsonTariffDayType; 

typedef struct
{
    int                 jsonver;
    char                name[32];
    int                 effectivetime;
    char                createTime[32]; 
    char                modifyTime[32]; 
    int                 week[TARIFF_WEEK_NUM];
    JsonTariffDayType   dayType[TARIFF_DAY_TYPE_NUM];
    JsonTariffSetting   tariffSetting[TARIFF_TARIFF_SETTING_NUM];
}JsonTariff; 

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL TariffLibInit(void);
BOOL TariffUpdateCurrentTariffData(void);
JsonTariffSetting* TariffGetCurrentTariffType(void);
char* TariffGetFileName(void);
BOOL TariffLoadTariffFile(void);
BOOL TariffGetNextWakeupTime(UINT32* wakeUpTime, RTC_TIME_DATA_T* sCurTime);
#ifdef __cplusplus
}
#endif

#endif //__TARIFF_LIB_H__

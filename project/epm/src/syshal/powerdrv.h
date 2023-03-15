/**************************************************************************//**
* @file     powerdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __POWER_DRV_H__
#define __POWER_DRV_H__

#include "nuc970.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define WAKEUP_SOURCE_NONE      0x00
#define WAKEUP_SOURCE_RTC       0x01   
#define WAKEUP_SOURCE_KEYPAD    0x02    
#define WAKEUP_SOURCE_DIP       0x03   
#define WAKEUP_SOURCE_USER      0x04      
#define WAKEUP_SOURCE_OTHER     0x05     
#define WAKEUP_SOURCE_BATTERY   0x06       

#define MAX_POWER_REG_CALLBACK_NUM  10
typedef BOOL(*powerCallback)(int);
    
typedef struct
{
    char* drvName;
    powerCallback powerPreOffCallback;
    powerCallback powerOffCallback;
    powerCallback powerOnCallback;
    powerCallback powerStatusCallback;    
}powerCallbackFunc;

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL PowerDrvInit(BOOL testModeFlag);
BOOL PowerSuspend(UINT32 wakeUpSource);
void PowerClearISR(void);
void PowerSetWakeupTime(UINT32 time);
//BOOL GetPowerStableStatus(void);
int PowerRegCallback(powerCallbackFunc* callbackFunc);
void __wfi(void);
uint32_t PowerGetTotalWakeupTick(void);  
void PowerDrvSetEnable(BOOL flag);
void PowerDrvResetSystem(void);
#ifdef __cplusplus
}
#endif

#endif //__POWER_DRV_H__

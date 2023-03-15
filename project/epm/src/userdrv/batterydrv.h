/**************************************************************************//**
* @file     batterydrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __BATTERY_DRV_H__
#define __BATTERY_DRV_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define BATTERY_STATUS_IN_USE           0x01
#define BATTERY_STATUS_NEED_REPLACE     0x02
#define BATTERY_STATUS_IDLE             0x03
#define BATTERY_STATUS_EMPTY            0x04
	
#define BATTERY_1_LOW_DETECT_PORT       GPIOH
#define BATTERY_1_LOW_DETECT_PIN        BIT6
#define BATTERY_2_LOW_DETECT_PORT       GPIOH
#define BATTERY_2_LOW_DETECT_PIN        BIT5

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL BatteryDrvInit(BOOL testMode);
void BatteryGetValue(UINT32* solarBatVoltage, UINT32* leftVoltage, UINT32* rightVoltage);
void BatteryGetStatus(uint8_t* leftStatus, uint8_t* rightStatus);
void BatterySwitchStatusEx(BOOL leftSwitch);
BOOL BatteryGetVoltage(void);
void BatterySetEnableTestMode(BOOL mode);
void BatterySetSwitch1(BOOL flag);
void BatterySetSwitch2(BOOL flag);
BOOL BatteryCheckPowerDownCondition(void);

void setBatterySwitchStatus(BOOL battery1Switch, BOOL battery2Switch);

void ModifyAjustSolarBatFactor(uint32_t factorValue);

uint32_t ShowAjustSolarBatFactor(void);


void SetCADtenConnectFlagFunc(BOOL flag);

BOOL ReadCADtenConnectFlagFunc(void);

void SetBatINTtestFlagFunc(int index,BOOL flag);

BOOL ReadBatINTtestFlagFunc(int index);

#ifdef __cplusplus
}
#endif

#endif //__BATTERY_DRV_H__

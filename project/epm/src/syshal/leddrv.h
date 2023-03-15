/**************************************************************************//**
* @file     leddrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __LED_DRV_H__
#define __LED_DRV_H__

#include "nuc970.h"
#include "interface.h"
#include "fepconfig.h"
#include "ledcmdlib.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

//#define LED_MODE_NORMAL_INDEX       0x01
//#define LED_MODE_REPLACE_BP_INDEX   0x02
#if(BUILD_RELEASE_VERSION)
    #define LED_HEARTBEAT_SECONDS       (60*5)
#else    
    #if(BUILD_PRE_RELEASE_VERSION)
        #define LED_HEARTBEAT_SECONDS       (60*5)
    #else
        #define LED_HEARTBEAT_SECONDS       1677 //Max Sec
    #endif
    //#define LED_HEARTBEAT_SECONDS       60
#endif
/*
#define LED_RING_BUFFER_SIZE       1024
    
typedef struct LEDRINGFIFO{
    char Buff[LED_RING_BUFFER_SIZE];
    short SaveIndex;
    short ReadIndex;
} LEDRingfifoStruct;    
*/
    
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL LedDrvInit(BOOL testMode);
BOOL LedSetStatus(void);
BOOL LedSetColor(uint8_t* bayColor, uint8_t statusColor, BOOL checkFlag);
//void LedSetMode(uint8_t mode);
BOOL LedSendHeartbeat(uint8_t* ret);
BOOL LedSetAliveStatusLightFlush(uint8_t LED_freq,uint8_t LED_period);
BOOL LedSetStatusLightFlush(uint8_t LED_freq,uint8_t LED_period);
BOOL LedSetBayLightFlush(uint8_t LED_freq,uint8_t LED_period);


//BOOL MemsCalibrationSet(void);

BOOL MemsCalibrationSet(short* MEMSx,short* MEMSy,short* MEMSz);

BOOL MemsCollisionSet(uint8_t bias_degree,uint8_t strength_X,uint8_t strength_Y,uint8_t strength_Z);
BOOL MemsCollisionClean(void);

//BOOL MemsGetStatus(uint8_t* status);
BOOL QueryVersion(uint8_t* VerCode1,uint8_t* VerCode2,uint8_t* VerCode3,uint8_t* YearCode,
									uint8_t* MonthCode,uint8_t* DayCode,uint8_t* HourCode,uint8_t* MinuteCode);
							
BOOL ClrLedInitFlag(void);                            
BOOL LedSendFactoryTest(void);
void LedSetPower(BOOL flag);

BOOL QueryMEMSValue(short* MEMSx,short* MEMSy,short* MEMSz);
void QueryMEMSValueEx(short* MEMSx,short* MEMSy,short* MEMSz);

BOOL LedReadShake(uint8_t* ret);

#ifdef __cplusplus
}
#endif

#endif //__LED_DRV_H__

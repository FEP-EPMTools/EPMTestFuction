/**************************************************************************//**
* @file     meterdata.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __METER_DATA_H__
#define __METER_DATA_H__
#include <time.h>
#include "nuc970.h"

#include "paralib.h"
#include "tarifflib.h"
#include "fileagent.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define EPM_ID_FROM_JSON_FILE   0

#if(EPM_ID_FROM_JSON_FILE)

#endif
typedef enum{
    RUNNING_STATUS_INIT = 0x00, 
    RUNNING_STATUS_FILE_DOWNLOAD = 0x01, 
    RUNNING_STATUS_ERROR = 0x02,
    RUNNING_STATUS_NORMAL = 0x03, 
}epmRunningStatus;    

typedef enum{
    SPACE_DEPOSIT_STATUS_INIT = 0x00, 
    SPACE_DEPOSIT_STATUS_OK = 0x01, //有儲值(不管有沒有車)
    SPACE_DEPOSIT_STATUS_EXCEED_TIME,//有車 超時/沒儲值
    SPACE_DEPOSIT_STATUS_STANBY, //沒車 超時(設回沒儲值)/沒儲值
}spaceDepositStatus;
#if(0)
typedef enum{
    METER_ERROR_CODE_USER_DRV       = 0x0001,
    METER_ERROR_CODE_EPD_DRV        = 0x0002,
    METER_ERROR_CODE_FATFS_DRV      = 0x0004,
    METER_ERROR_CODE_GUI_DRV        = 0x0008,
    
    METER_ERROR_CODE_CARD_READER_DRV    = 0x0010,
    METER_ERROR_CODE_SPSCE_DRV          = 0x0020,
    METER_ERROR_CODE_POWER_DRV          = 0x0040,
    METER_ERROR_CODE_DATA_AGENT_DRV     = 0x0080,
    
    METER_ERROR_CODE_PCT08_DRV          = 0x0100,
    METER_ERROR_CODE_PHOTO_AGENT        = 0x0200,
    METER_ERROR_CODE_MODEM_AGENT_DRV    = 0x0400,
    METER_ERROR_CODE_TARIFF_FILE        = 0x0800,
    
    METER_ERROR_CODE_YAFFS2_DRV      = 0x1000,
    
    
    METER_ERROR_CODE_USER               = 0x8000  
}meterErrorCode;
#else
#define METER_ERROR_CODE_USER_DRV       0x0001
#define METER_ERROR_CODE_EPD_DRV        0x0002
#define METER_ERROR_CODE_FATFS_DRV      0x0004
#define METER_ERROR_CODE_GUI_DRV        0x0008
    
#define METER_ERROR_CODE_CARD_READER_DRV    0x0010
#define METER_ERROR_CODE_SPSCE_DRV          0x0020
#define METER_ERROR_CODE_POWER_DRV          0x0040
#define METER_ERROR_CODE_DATA_AGENT_DRV     0x0080
    
#define METER_ERROR_CODE_PCT08_DRV          0x0100
#define METER_ERROR_CODE_PHOTO_AGENT        0x0200
#define METER_ERROR_CODE_MODEM_AGENT_DRV    0x0400
#define METER_ERROR_CODE_TARIFF_FILE        0x0800
    
#define METER_ERROR_CODE_YAFFS2_DRV         0x1000   
    
#define METER_ERROR_CODE_USER               0x8000  

typedef uint16_t meterErrorCode;
#endif

//#pragma pack(1)
typedef struct
{
    uint32_t            buildVer;
    int                 epmid;
    char                buildStr[128];
    char                epmIdStr[128];
    
    uint8_t             currentSelSpace;  // 1-6
    uint16_t            currentSelTime;
    uint16_t            currentSelCost;
    
    meterErrorCode      meterErrorCode;
    
    epmRunningStatus    runningStatus;
    
    spaceDepositStatus  spaceSepositStatus[EPM_TOTAL_METER_SPACE_NUM]; 

    FileAgentTargetFileName   targetTariffFileName[TARIFF_FILE_NUM];
    FileAgentTargetFileName   targetParaFileName;
    char tariffFileFTPPath[_MAX_LFN];
    char paraFileFTPPath[_MAX_LFN];
    char jpgFileFTPPath[_MAX_LFN];
    char dsfFileFTPPath[_MAX_LFN];
    char dcfFileFTPPath[_MAX_LFN];
    char logFileFTPPath[_MAX_LFN];
}MeterData; //total max can`t exceed 256(ref flash define)
//#pragma pack()

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL MeterDataInit(void);
MeterData* GetMeterData(void);
BOOL UpdateMeterDepositTime(time_t time);
void UpdateMeterCurrentSelSpace(int value, BOOL setFlag);
void AutoUpdateMeterData(void);
void MeterSetErrorCode(meterErrorCode code);
void MeterUpdateExpiredTitle(uint8_t selectItemId);
void MeterSetBuildVer(uint32_t buildVer);
BOOL GetRealSpaceStatus(uint8_t index);
BOOL MeterUpdateLedHeartbeat(void);
void MeterUpdateModemAgentLastTime(void);
char*  MeterGetCurrentDCFFileName(RTC_TIME_DATA_T* pt);
#ifdef __cplusplus
}
#endif

#endif //__METER_DATA_H__

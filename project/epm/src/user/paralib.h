/**************************************************************************//**
* @file     paralib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __PARA_LIB_H__
#define __PARA_LIB_H__
#include <time.h>
#include "nuc970.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define  STORAGE_TYPE_FATFS    0x01   
#define  STORAGE_TYPE_FLASH    0x02  

#define  EPM_STORAGE_TYPE  STORAGE_TYPE_FATFS
    
#define  EPM_TOTAL_METER_SPACE_NUM           6 

//#define  EPM_DEFAULT_METER_POSITION      3  //0~6// meter0 [space1] 1 [2] 2 [3] 3 [4] 4 [5] 5 [6] 6
//#define  EPM_DEFAULT_SPACE_ENABLE_NUM    6  //1~6

#define  EPM_DEFAULT_METER_POSITION      1//3  // meter0 [space1] 1 [2] 2 [3] 3 [4] 4 [5] 5 [6] 6
#define  EPM_DEFAULT_SPACE_ENABLE_NUM    2//6  

    
#define EPM_STORAGE_VERSION 1

#if(EPM_STORAGE_TYPE == STORAGE_TYPE_FATFS)


    #define EPM_STORAGE_RESERVE_LEN         450
#elif(EPM_STORAGE_TYPE == STORAGE_TYPE_FLASH)
    #define EPM_STORAGE_RESERVE_LEN         189
#else
    #error
#endif

#pragma pack(1)
typedef struct
{
    uint8_t         version; //1 byte
    uint16_t        recordLen; //2 byte
    time_t          depositStartTime[EPM_TOTAL_METER_SPACE_NUM];   //8 byte
    time_t          depositEndTime[EPM_TOTAL_METER_SPACE_NUM];   //8 byte
    uint8_t         reserve[EPM_STORAGE_RESERVE_LEN];
    uint16_t        checksum; // 2 byte    
    uint8_t         octopusReceiptNumber[7];
}MeterStorageData; //total max can`t exceed 256(ref flash define)

typedef struct
{
    int             jsonver;
//    int             epmid;
    char            name[32];
    char            createTime[32]; 
    char            modifyTime[32]; 
    int             meterPosition;
    int             spaceEnableNum;
//#warning not implement yet
    time_t          bayLedOnInterval;
    time_t          bayLedFlashInterval;
    time_t          statusLedOnInterval;
    time_t          statusLedFlashInterval;
}MeterPara; 
#pragma pack()


/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL ParaLibInit(void);
void printParaValue(char* str);
MeterPara* GetMeterPara(void);
MeterStorageData* GetMeterStorageData(void);
void MeterStorageFlush(void);
//BOOL ParaLibSetMeterSpaceInfo(uint8_t meterPosition, uint8_t spaceEnableNum);
void ParaLibResetDepositEndTime(void);
BOOL MeterDataReloadParaFile(void);
#ifdef __cplusplus
}
#endif

#endif //__PARA_LIB_H__

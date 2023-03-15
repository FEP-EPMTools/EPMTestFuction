/**************************************************************************//**
* @file     dataprocesslib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __DATA_PROCESS_LIB_H__
#define __DATA_PROCESS_LIB_H__

#include "nuc970.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

    
    
typedef enum{
    DATA_PROCESS_ID_NONE     = 0x00,
    DATA_PROCESS_ID_LOG     = 0x01,
    DATA_PROCESS_ID_ESF     = 0x02,
    DATA_PROCESS_ID_PHOTO   = 0x03,
    DATA_PROCESS_ID_TRE     = 0x04,
    DATA_PROCESS_ID_DSF     = 0x05,
    DATA_PROCESS_ID_DCF     = 0x06,
    DATA_PROCESS_ID_PARA    = 0x07,
}DataProcessId;

typedef enum{
    DATA_PROCESS_EXECUTE_TYPE_NONE     = 0x00,
    DATA_PROCESS_EXECUTE_TYPE_ALWAYS   = 0x01,
    DATA_PROCESS_EXECUTE_TYPE_INTERVAL = 0x02,
    DATA_PROCESS_EXECUTE_TYPE_TIMER    = 0x03
}DataProcessExecuteType;

#if(0)
typedef enum{
    DATA_SEND_TYPE_NONE             = 0x00,
    DATA_SEND_TYPE_WEB_POST         = 0x01,
    DATA_SEND_TYPE_FTP              = 0x02,
    DATA_SEND_TYPE_FTP_GET          = 0x04
}DataSendType;
#else

#define DATA_SEND_TYPE_NONE         0x00
#define DATA_SEND_TYPE_WEB_POST     0x01
#define DATA_SEND_TYPE_FTP          0x02
#define DATA_SEND_TYPE_FTP_GET      0x04
typedef int DataSendType;
#endif


typedef enum{
    DATA_TYPE_ID_TRANSACTION     = 0x00,
    DATA_TYPE_ID_EXPIRED     = 0x01
}DataId;

typedef BOOL(*DataProcessPreprocessFunc)(uint32_t currentTime);
typedef BOOL(*DataProcessActionFunc)(uint32_t currentTime, DataSendType type);

typedef struct
{
    char*                       processName;
    DataProcessId               dataProcessId;
    //DataProcessPreprocessFunc   preprocessFunc;
    DataProcessActionFunc       executeFunc;
    BOOL                        executeFlag;
    //uint32_t                    lastExecuteTime;    //second
    //DataProcessExecuteType      executeType;    
    //uint32_t                    executeIntervalTime; //second
    //uint8_t                     executeTimerClock;
    DataSendType                sendType;
}DataProcessItem;


typedef BOOL(*DataProcessFtpGetReloadCallback)(void);

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL DataProcessLibInit(void);
BOOL DataProcessCheckExecute(uint32_t currentTime);
BOOL DataProcessExecute(uint32_t currentTime, DataSendType type);
BOOL DataProcessSendData(uint32_t currentTime, int bayid, int time, int cost, int balance, DataId dataType, char* flagStr);
BOOL DataProcessSetExecuteFlag(DataProcessId id);
BOOL DataProcessSendStatusData(uint32_t currentTime, char* flagStr);
BOOL DataParserWebPostReturnData(char* jsonStr);
#ifdef __cplusplus
}
#endif

#endif //__DATA_PROCESS_LIB_H__

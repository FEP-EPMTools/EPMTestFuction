/**************************************************************************//**
* @file     dataagent.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __DATA_AGENT_H__
#define __DATA_AGENT_H__
#include <time.h>
#include "nuc970.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define MAX_AGENT_DATA_SIZE     (1024+256)
#define MAX_AGENT_BUFF_NUMBER   10  
    
typedef BOOL(*dataAgentTxCallback)(uint16_t address);
    
typedef struct
{
    BOOL                    needAckFlag;
    BOOL                    useFlag;
    uint16_t                dataLen;
    dataAgentTxCallback     callback;
    uint8_t                 data[MAX_AGENT_DATA_SIZE];
    TickType_t              sendTimeTick;
}DataAgentTxBuff;

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL DataAgentInit(BOOL testModeFlag);
BOOL DataAgentAddData(uint8_t* pData, uint16_t dataLen, BOOL needAck, dataAgentTxCallback callback);
BOOL DataAgentRemoveDataByAck(uint16_t cmdIndex);
BOOL DataAgentSetDebugEnable(BOOL flag);
BOOL DataAgentStartFileTransfer(char* filename, char* targetFileName);
void DataAgentStopFileTransfer(void);
#ifdef __cplusplus
}
#endif

#endif //__DATA_AGENT_H__

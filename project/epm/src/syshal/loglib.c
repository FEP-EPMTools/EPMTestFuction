/**************************************************************************//**
* @file     loglib.c
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
#include "nuc970.h"
#include "sys.h"
#include "rtc.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "loglib.h"
#include "ff.h"
#include "meterdata.h"
#include "fileagent.h"
#include "batterydrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
static char currentLogFileName[_MAX_LFN];
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static SemaphoreHandle_t xSemaphore = NULL;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
char*  getCurrentLogFileName(RTC_TIME_DATA_T* pt)
{    
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, pt))
    {
        sprintf(currentLogFileName, "%08d_%04d%02d%02d%02d%02d.log", GetMeterData()->epmid, pt->u32Year, pt->u32cMonth, pt->u32cDay, pt->u32cHour, (pt->u32cMinute)/10);   
        //sprintf(currentLogFileName, "%08d_%04d%02d%02d%02d%02d.log", GetMeterPara()->epmid, pt->u32Year, pt->u32cMonth, pt->u32cDay, pt->u32cHour, (pt->u32cMinute));   
    }  
    return currentLogFileName;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL LoglibInit(void)
{
    sysprintf("LoglibInit!!\n");
    xSemaphore = xSemaphoreCreateMutex(); 
    return TRUE;
}
char*  LoglibGetCurrentLogFileName(void)
{       
    return currentLogFileName;
}
void  LoglibPrintfEx(LogType type, char* logData, BOOL flag)
{
    LoglibPrintf(type, logData);
}
void  LoglibPrintf(LogType type, char* logData)
{
    char* fileName;
    char* pTitle = NULL;
    RTC_TIME_DATA_T pt;
    char dateInfo[_MAX_LFN];
	char* pDateInfo;
    int strLen = 0;
    if(BatteryCheckPowerDownCondition())
    {
        sysprintf("\r\n ~~~~~ LoglibPrintf ignore (BatteryCheckPowerDownCondition) ~~~~~ \r\n");
        return ;
    }
    if(xSemaphore == NULL)
    {
        xSemaphore = xSemaphoreCreateMutex(); 
    }
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);    
    fileName = getCurrentLogFileName(&pt);
    sysprintf("\r\n ~~~~~ LoglibPrintf START ~~~~~ \r\n");
		   
    if(type == LOG_TYPE_INFO)
    {
        pTitle = "[INFO]";
    }
    else if(type == LOG_TYPE_WARNING)
    {
        pTitle = "[WARNING]";            
    }
    else if(type == LOG_TYPE_ERROR)
    {
        pTitle = "[ERROR]";
    }
    else
    {
        pTitle = "[OTHER]";
    }
		
    sprintf(dateInfo, "%04d/%02d/%02d_%02d:%02d:%02d[%d] %s:", pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond, pt.u32cDayOfWeek, pTitle);
    if(strlen(logData) > 1024)
        strLen = 1024;
    else
        strLen = strlen(logData);
	pDateInfo = pvPortMalloc(strlen(dateInfo) + strLen + 1); //§tµ²§À²Å¸¹
    if(pDateInfo != NULL)
    {
        memset(pDateInfo, 0x0, strlen(dateInfo) + strLen + 1);
        sprintf(pDateInfo, "%s%s", dateInfo, logData);
        sysprintf("[%s]", pDateInfo);
        FileAgentAddData(LOG_SAVE_POSITION, LOG_FILE_DIR, fileName, (uint8_t*)pDateInfo, strlen(pDateInfo), FILE_AGENT_ADD_DATA_TYPE_APPEND, TRUE, FALSE, FALSE);  
        //reVal = FileAgentAddData(LOG_SAVE_POSITION, LOG_FILE_DIR, fileName, (uint8_t*)pDateInfo, strlen(pDateInfo), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, FALSE);  
        
        sysprintf(" ~~~~~ LoglibPrintf END ~~~~~ \r\n");
    }
    else
    {
        sysprintf(" ~~~~~ LoglibPrintf pvPortMalloc ERROR ~~~~~ \r\n");
    }
    xSemaphoreGive(xSemaphore); 
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


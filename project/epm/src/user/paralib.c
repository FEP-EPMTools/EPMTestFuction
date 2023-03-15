/**************************************************************************//**
* @file     paralib.c
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

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "paralib.h"
#include "interface.h"
#include "epddrv.h"
#include "cJSON.h"
#include "osmisc.h"
#include "loglib.h"
#include "meterdata.h"
#if(EPM_STORAGE_TYPE == STORAGE_TYPE_FATFS)
#include "ff.h"
#include "fileagent.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
#if(EPM_STORAGE_TYPE == STORAGE_TYPE_FATFS)
static MeterPara meterPara;
static SemaphoreHandle_t xSemaphore;
#elif(EPM_STORAGE_TYPE == STORAGE_TYPE_FLASH)
static StorageInterface* pStorageInterface = NULL;
#endif
static MeterStorageData meterStorageData;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static uint16_t getStorageChecksum(uint8_t* pTarget, uint16_t len, char* str)
{
    int i;
    uint16_t checksum = 0;
    uint8_t* pr = (uint8_t*)pTarget;
    for(i = 0; i< len - sizeof(uint16_t); i++) //???checksum ?????
    {
        checksum = checksum + pr[i];
    }
    //sysprintf("  -- getStorageChecksum (%s) : checksum = 0x%x (%d)\r\n", str, checksum, checksum); 
    return checksum;
}

static void resetStorageValue(void)
{
    //sysprintf("     reset ParaValue >>\r\n"); 
    memset(&meterStorageData, 0x0, sizeof(MeterStorageData));
    
    meterStorageData.version = EPM_STORAGE_VERSION;
    meterStorageData.recordLen = sizeof(MeterStorageData);    
    
    //checksum    
    meterStorageData.checksum = getStorageChecksum((uint8_t*)&meterStorageData, sizeof(MeterStorageData), "Reset");   

    printParaValue("Reset");    
    //sysprintf("     <<  reset ParaValue\r\n"); 
}
static BOOL checkStorageValue(uint16_t versionSrc, uint16_t recordLenSrc, uint16_t checkSumSrc, uint16_t version, uint16_t recordLen, uint16_t checkSum, char* str)   
{
    if(versionSrc != version)
    {
        sysprintf("!!ERROR!! check %s ParaValue : version error  [%d:%d]\r\n", str, versionSrc, version); 
        //LoglibPrintf(LOG_TYPE_ERROR, "!!ERROR!! check %s ParaValue : version error  [%d:%d]\r\n", str, versionSrc, version); 
        return FALSE;
    }
    else if(recordLenSrc != recordLen)
    {
        sysprintf("!!ERROR!! check %s ParaValue : recordLen error  [%d:%d]\r\n", str, recordLenSrc, recordLen); 
        //LoglibPrintf(LOG_TYPE_ERROR, "!!ERROR!! check %s ParaValue : recordLen error  [%d:%d]\r\n", str, recordLenSrc, recordLen); 
        return FALSE;
    }
    else if(checkSumSrc != checkSum)
    {
        sysprintf("!!ERROR!! check %s ParaValue : checksum error  [%d:%d]\r\n", str, checkSumSrc, checkSum); 
        //LoglibPrintf(LOG_TYPE_ERROR, "!!ERROR!! check %s ParaValue : checksum error  [0x%02x:0x%02x]\r\n", str, checkSumSrc, checkSum); 
        return FALSE;
    }
    return TRUE;
}
static void loadStorageValue(uint8_t* src, uint16_t len, char* str)
{
    sysprintf("     load %s ParaValue >>\r\n", str); 
#if(EPM_STORAGE_TYPE == STORAGE_TYPE_FATFS)
    uint8_t* dataTmp;
    size_t dataTmpLen;
    BOOL needFree;
    FileAgentReturn reVal;     
    
    reVal = FileAgentGetData(EPM_STORAGE_SAVE_POSITION, EPM_STORAGE_FILE_DIR, EPM_STORAGE_FILE_NAME, &dataTmp, &dataTmpLen, &needFree, TRUE);
    if((reVal != FILE_AGENT_RETURN_ERROR) &&(dataTmpLen == len))
    {
        memcpy((void *)src, (const void *)dataTmp, (size_t)len);
        if(needFree)
        {
            vPortFree(dataTmp);
        }
    }
    else
    {
        memset(src, 0x0, len);   
    }
    
    //sysprintf("     << load %s ParaValue\r\n", str); 
#elif(EPM_STORAGE_TYPE == STORAGE_TYPE_FLASH)
    pStorageInterface->readFunc(src, len);
#endif
}
static void saveStorageValue(uint8_t* src, uint16_t len, char* str, BOOL blockFlag)
{
    sysprintf("     save \"%s\" ParaValue (len = %d) >>\r\n", str, len); 
#if(EPM_STORAGE_TYPE == STORAGE_TYPE_FATFS)
    if(xSemaphore == NULL)
    {
        xSemaphore = xSemaphoreCreateMutex(); 
    }    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);     
    FileAgentAddData(EPM_STORAGE_SAVE_POSITION, EPM_STORAGE_FILE_DIR, EPM_STORAGE_FILE_NAME, src, len, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, blockFlag/*TRUE*/, TRUE); 
    xSemaphoreGive(xSemaphore); 
#elif(EPM_STORAGE_TYPE == STORAGE_TYPE_FLASH)
    pStorageInterface->writeFunc(src, len);
#endif
}

static void resetJsonPara(void)
{
    meterPara.jsonver = -1;
    meterPara.spaceEnableNum = EPM_DEFAULT_SPACE_ENABLE_NUM;
    meterPara.meterPosition = EPM_DEFAULT_METER_POSITION;
}

static BOOL loadJsonParaFile(char* dir, char* name)
{
    sysprintf("     load loadJsonParaFile  >>\r\n"); 
#if(EPM_STORAGE_TYPE == STORAGE_TYPE_FATFS)
    uint8_t* dataTmp;
    size_t dataTmpLen;
    BOOL needFree;
    FileAgentReturn reVal; 
    
     //__SHOW_FREE_HEAP_SIZE__
    
    reVal = FileAgentGetData(EPM_PARA_SAVE_POSITION, dir, name, &dataTmp, &dataTmpLen, &needFree, TRUE);
    if(reVal != FILE_AGENT_RETURN_ERROR)
    {
        //為了在其他DISK備份
        FileAgentAddData(EPM_PARA_SAVE_POSITION, dir, name, dataTmp, dataTmpLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, TRUE); 
        
        cJSON *root_json = cJSON_Parse((char*)dataTmp);
        cJSON *tmp_json;
        
        if (NULL == root_json)
        {
            sysprintf("error((NULL == root_json)):%s\n", cJSON_GetErrorPtr());                        
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "jsonver");
        if (tmp_json != NULL)
        {
            meterPara.jsonver = tmp_json->valueint;
            sysprintf(" >> meterPara.jsonver:%d\n", meterPara.jsonver);
        }
        else
        {
            cJSON_Delete(root_json);
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "name");
        if (tmp_json != NULL)
        {
            strcpy((char*)meterPara.name, (const char*)tmp_json->valuestring);
            sysprintf(" >> meterPara.name:[%s]\n", meterPara.name);
        }
        else
        {
            cJSON_Delete(root_json);
            goto jsonExit;
        }
        /*
        tmp_json = cJSON_GetObjectItem(root_json, "epmid");
        if (tmp_json != NULL)
        {
            //sysprintf(" >> 0x%04x, 0x%04x\n", (tmp_json->valueint), (uint16_t)(tmp_json->valueint));
            meterPara.epmid = tmp_json->valueint;
            sysprintf(" >> meterPara.epmid:%d\n", meterPara.epmid);
            //cJSON_Delete(tmp_json);
        }
        else
        {
            cJSON_Delete(root_json);
            goto jsonExit;
        }
        */
        
        tmp_json = cJSON_GetObjectItem(root_json, "createtime");
        if (tmp_json != NULL)
        {
            strcpy(meterPara.createTime, tmp_json->valuestring);
            sysprintf(" >> meterPara.createTime:[%s]\n", meterPara.createTime);
        }
        else
        {
            cJSON_Delete(root_json);
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "modifytime");
        if (tmp_json != NULL)
        {
            strcpy(meterPara.modifyTime, tmp_json->valuestring);
            sysprintf(" >> meterPara.modifyTime:[%s]\n", meterPara.modifyTime);
        }
        else
        {
            cJSON_Delete(root_json);
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "baynum");
        if (tmp_json != NULL)
        {
            meterPara.spaceEnableNum = tmp_json->valueint;
            sysprintf(" >> meterPara.spaceEnableNum:%d\n", meterPara.spaceEnableNum);
        }
        else
        {
            cJSON_Delete(root_json);
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "meterposition");
        if (tmp_json != NULL)
        {
            meterPara.meterPosition = tmp_json->valueint;
            sysprintf(" >> meterPara.meterPosition:%d\n", meterPara.meterPosition);
        }
        else
        {
            cJSON_Delete(root_json);
            goto jsonExit;
        }
        cJSON_Delete(root_json);
        if(needFree)
        {
            vPortFree(dataTmp);
        }
        
         //__SHOW_FREE_HEAP_SIZE__ 
        return TRUE;
    }
    

jsonExit:    
    //__SHOW_FREE_HEAP_SIZE__ 

    //LoglibPrintf(LOG_TYPE_ERROR, "!!ERROR!! loadJsonParaFile ERROR\r\n"); 
    return FALSE;

#elif(EPM_STORAGE_TYPE == STORAGE_TYPE_FLASH)
    pStorageInterface->readFunc(src, len);
#endif
}
static BOOL paraFileLoadFlag = FALSE;
static BOOL paraCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4)
{    
    //sysprintf(" - tariffCallback copy [%d] [%s %s]!!\n", tariffIndex, dir, filename);
    //strcpy(tariffFileName[tariffIndex], filename);
    if(paraFileLoadFlag == FALSE)
    {        
        if(loadJsonParaFile(dir, filename))
        {
            paraFileLoadFlag = TRUE;        
        }
    }
    return TRUE;
}


static BOOL loadParaFileName ()
{
    paraFileLoadFlag = FALSE;
    resetJsonPara();
    FileAgentGetList(EPM_PARA_SAVE_POSITION, EPM_PARA_FILE_DIR, FILE_EXTENSION_EX(EPM_PRAR_FILE_EXTENSION), NULL, paraCallback, NULL, NULL, NULL, NULL);
    return TRUE;
}




static BOOL applyPara(void)
{ 
    return calculatePreparePositionInfo(meterPara.meterPosition, meterPara.spaceEnableNum);
}
static BOOL swInit(void)
{   
    BOOL reVal = FALSE;
    int retryTimes = 3;
    sysprintf("\r\nParaInit : sizeof(MeterStorageData) = %d...\r\n", sizeof(MeterStorageData)); 
    xSemaphore = xSemaphoreCreateMutex(); 
    
    if(MeterDataReloadParaFile() == FALSE)
    {
        return FALSE;
    }
   
    while(retryTimes != 0)
    {
        sysprintf("--INFO---  EPM Storage Init : %d  -----\r\n", retryTimes); 
        memset((uint8_t*)&meterStorageData, 0x0, sizeof(MeterStorageData));
        loadStorageValue((uint8_t*)&meterStorageData, sizeof(MeterStorageData), "ParaLib");
        
        //printParaValue("Load"); 
        
        if(checkStorageValue(meterStorageData.version, meterStorageData.recordLen, meterStorageData.checksum, 
                    EPM_STORAGE_VERSION, sizeof(MeterStorageData), getStorageChecksum((uint8_t*)&meterStorageData, sizeof(MeterStorageData), "ParaLib"), "ParaLib"))
        {
            //BOOL saveFlag = FALSE;
            printParaValue("Load OK");
            sysprintf("#####  checkStorageValue: OK...\r\n");   
            reVal = TRUE; 
            saveStorageValue((uint8_t*)&meterStorageData, sizeof(MeterStorageData), "Meter", FALSE);
            break;

        }
        else
        {            
            printParaValue("Load ERROR");
            sysprintf("#####  checkStorageValue: ERROR...\r\n"); 
            resetStorageValue();
            saveStorageValue((uint8_t*)&meterStorageData, sizeof(MeterStorageData), "Meter", TRUE);
        }
        retryTimes--;
    }
    return reVal;
}


    
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL ParaLibInit(void)
{
    sysprintf("ParaLibInit!!\n");
#if(EPM_STORAGE_TYPE == STORAGE_TYPE_FLASH)
    pStorageInterface = StorageGetInterface(STORAGE_FLASH_INTERFACE_INDEX);
    if(pStorageInterface == NULL)
    {
        sysprintf("ParaLibInit ERROR (pStorageInterface == NULL)!!\n");
        return FALSE;
    }
    if(pStorageInterface->initFunc() == FALSE)
    {
        sysprintf("ParaLibInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
#endif
    if(swInit() == FALSE)
    {
        sysprintf("ParaLibInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    
    
    sysprintf("ParaLibInit OK!!\n");
    return TRUE;
}

void printParaValue(char* str)
{
    int i;
    sysprintf("  \r\n"); 
    sysprintf("  - print Para Value (%s) : version = %d, recordLen = %d, sizeof(MeterStorageData) = %d \r\n", str, meterStorageData.version, meterStorageData.recordLen, sizeof(MeterStorageData));
    for(i = 0; i<EPM_TOTAL_METER_SPACE_NUM; i++)
    {
        sysprintf("  - METER_SPACE_%02d : StartTime = %d \r\n", i, meterStorageData.depositStartTime[i]);
        sysprintf("  - METER_SPACE_%02d : EndTime = %d \r\n", i, meterStorageData.depositEndTime[i]);
    }
    sysprintf("  - meterPosition  = %d, spaceEnableNum = %d \r\n", meterPara.meterPosition, meterPara.spaceEnableNum);    
    sysprintf("  \r\n"); 
}

MeterPara* GetMeterPara(void)
{
    return &meterPara;
}

MeterStorageData* GetMeterStorageData(void)
{
    return &meterStorageData;
}
void MeterStorageFlush(void)
{
    //checksum    
    meterStorageData.checksum = getStorageChecksum((uint8_t*)&meterStorageData, sizeof(MeterStorageData), "Reset");   
    saveStorageValue((uint8_t*)&meterStorageData, sizeof(MeterStorageData), "MeterStorageFlush", FALSE);;
}
/*
BOOL ParaLibSetMeterSpaceInfo(uint8_t meterPosition, uint8_t spaceEnableNum)
{
    sysprintf(" --> ParaLibSetMeterSpaceInfo meterPosition = %d, spaceEnableNum = %d  enter\r\n", meterPosition, spaceEnableNum); 
    if(calculatePreparePositionInfo(meterPosition, spaceEnableNum))
    {
        meterPara.meterPosition = meterPosition;
        meterPara.spaceEnableNum = spaceEnableNum;
        MeterStorageFlush();
        sysprintf("!!! ParaLibSetMeterSpaceInfo OK meterPosition  = %d, spaceEnableNum = %d \r\n", meterPosition, spaceEnableNum); 
        return TRUE;        
    }
    else
    {
        sysprintf("!!! ParaLibSetMeterSpaceInfo ERROR meterPosition  = %d, spaceEnableNum = %d \r\n", meterPosition, spaceEnableNum);   
        return FALSE;
    }
}
*/

void ParaLibResetDepositEndTime(void)
{
    int i;
    sysprintf(" -- !!! ParaLibResetDepositEndTime  --\r\n"); 
    for(i = 0; i<EPM_TOTAL_METER_SPACE_NUM; i++)
    {
        meterStorageData.depositStartTime[i] = 0;
        meterStorageData.depositEndTime[i] = 0;
    }
}

BOOL MeterDataReloadParaFile(void)
{
    sysprintf(" -- !!! MeterDataReloadParaFile  --\r\n"); 
    if(loadParaFileName() != TRUE)
    {
        //return FALSE;
       GetMeterData()->runningStatus = RUNNING_STATUS_FILE_DOWNLOAD;
    }
    else
    {
        
    }
    if(applyPara() == FALSE)
    {
        sysprintf("ParaLibInit ERROR (applyPara false)!!\n");
        return FALSE;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


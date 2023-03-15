/**************************************************************************//**
* @file     dataprocesslib.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
EPM Status File     : esf
Photo File          : jpg
Log File            : log
Data Single File    : dsf
Data Collection File: dcf


Tariff File         : tre   (tariff.tre)
Parameter File      : pre   (epm.pre)
[data={"jsonver":1,"epmver":"1.0.0(1703271007)","id":"05478786","time":1490580625,"flag":"routine","index":30,"bay":[false,false],"baydist":[0,0],"voltage":[0,0],"deposit":[1490580661,1490581498,0,0,0,0],"tariff":"Living3.0_001","setting":"TPE_001"}]
[data={"jsonver":1,"epmver":"1.0.0(1703271007)","id":"05478786","time":1490580612,"flag":"fataslibtest","index":26,"bay":[false,false],"baydist":[0,0],"voltage":[0,0],"deposit":[1490580661,1490581498,0,0,0,0],"tariff":"Living3.0_001","setting":"TPE_001","expired":{"bayid":1}}]
[data={"jsonver":1,"epmver":"1.0.0(1703271007)","id":"05478786","time":1490580623,"flag":"mainroutine","index":28,"bay":[false,false],"baydist":[0,0],"voltage":[0,0],"deposit":[1490580661,1490581498,0,0,0,0],"tariff":"Living3.0_001","setting":"TPE_001","transaction":{"bayid":0,"time":0,"cost":0,"balance":8888}}]


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
#include "dataprocesslib.h"
#include "jsoncmdlib.h"   
#include "meterdata.h"
#include "quentelmodemlib.h"
#include "batterydrv.h"
#include "tarifflib.h"
#include "spacedrv.h"
#include "ff.h"
#include "pct08cmdlib.h"
#include "pct08drv.h"
#include "loglib.h"
#include "timelib.h"
#include "photoagent.h"
#include "guimanager.h"
#include "osmisc.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
/*
{
  "ok": 0,
  "errCode": 404,
  "datetime": 1489742357697
}
*/
#define RE_EXECUTE_TIMES  3
#define LOG_SUCCESS_ENABLE  1

#define STATUS_DATA_QUEUE_SIZE  1024
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static QueueHandle_t xStatusDataQueue;


//-----  status -----
static BOOL statusExecuteFunc(uint32_t currentTime, DataSendType type);

//-----  dsf -----
static BOOL dsfExecuteFunc(uint32_t currentTime, DataSendType type);

//-----  dcf -----
static BOOL dcfExecuteFunc(uint32_t currentTime, DataSendType type);

//-----  photo -----
static BOOL photoExecuteFunc(uint32_t currentTime, DataSendType type);

//-----  log -----
static BOOL logExecuteFunc(uint32_t currentTime, DataSendType type);

//-----  tariff -----
static BOOL tariffExecuteFunc(uint32_t currentTime, DataSendType type);

//-----  para -----
static BOOL paraExecuteFunc(uint32_t currentTime, DataSendType type);

//-------------------
int static esfIndex = 0;
//static uint8_t  readDataTemp[1024*1024];
static DataProcessItem processItem[] = {{"[Status File]",       DATA_PROCESS_ID_ESF,       statusExecuteFunc,   TRUE,    DATA_SEND_TYPE_WEB_POST},
                                        {"[DSF File]",          DATA_PROCESS_ID_DSF,       dsfExecuteFunc,      TRUE,    DATA_SEND_TYPE_FTP},
                                        {"[DCF File]",          DATA_PROCESS_ID_DCF,       dcfExecuteFunc,      TRUE,    DATA_SEND_TYPE_FTP},
                                        {"[Photo File]",        DATA_PROCESS_ID_PHOTO,     photoExecuteFunc,    TRUE,    DATA_SEND_TYPE_FTP},
                                        {"[Log File]",          DATA_PROCESS_ID_LOG,       logExecuteFunc,      TRUE,    DATA_SEND_TYPE_FTP},
                                        {"[Para File]",         DATA_PROCESS_ID_PARA,      paraExecuteFunc,     TRUE,    DATA_SEND_TYPE_FTP_GET},
                                        {"[Tariff File]",       DATA_PROCESS_ID_TRE,       tariffExecuteFunc,   TRUE,    DATA_SEND_TYPE_FTP_GET},
                                        {NULL,                  DATA_PROCESS_ID_NONE,      NULL,                FALSE,   DATA_SEND_TYPE_NONE}
                                        };

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


static BOOL loadSimpleFile(BOOL needChdirFlag, StorageType storageType, char* dir, TCHAR* fileName, char* ftpdir, DataSendType type, BOOL checkMode)
{    
    uint8_t* dataTmp;
    size_t dataTmpLen;
    BOOL needFree;
    FileAgentReturn reVal;  
    BOOL returnVal = TRUE;
    sysprintf("\r\n  !!! --[INFO]--> loadSimpleFile[%s] start... !!!\r\n", fileName);

    reVal = FileAgentGetData(storageType, dir, fileName, &dataTmp, &dataTmpLen, &needFree, checkMode);
    if(reVal != FILE_AGENT_RETURN_ERROR)
    {
        if(dataTmpLen > 0)
        {
            if(type == DATA_SEND_TYPE_FTP)
            {
                char ftpPreTargetDir[_MAX_LFN];
                sprintf(ftpPreTargetDir, "%s%08d", FTP_PRE_PATH, GetMeterData()->epmid);
                char ftpTargetDir[_MAX_LFN];
                //sprintf(ftpTargetDir, "%08d/%s", GetMeterData()->epmid, ftpdir);                
                sprintf(ftpTargetDir, "%s%08d/%s", FTP_PRE_PATH, GetMeterData()->epmid, ftpdir);
                
                QModemFtpClientStart();
                if(QModemFtpClientProcess() == TRUE)
                {
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[512];
                        sprintf(str, " !!!!!! ~ FTP_CONNECT SUCCESS (loadSimpleFile) ~!!!!!!\r\n");
                        LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                    #endif
                    if(FtpClientSendFile(needChdirFlag, ftpPreTargetDir, ftpTargetDir, fileName, (uint8_t*)dataTmp, dataTmpLen) == FALSE)
                    {                    
                        #if(ENABLE_LOG_FUNCTION)
                        {
                            char str[256];
                            sprintf(str, "  -X-> FtpClientSendFile[%s] ERROR...\r\n", fileName);
                            LoglibPrintf(LOG_TYPE_ERROR, str);
                        }
                        #else
                        sysprintf("--[INFO]--> FtpClientSendFile[%s] ERROR...\r\n", fileName);
                        #endif
                        returnVal = FALSE;
                    }  
                    else
                    {
                        #if(ENABLE_LOG_FUNCTION)
                        char str[256];
                        sprintf(str, "  -O-> FtpClientSendFile[%s] OK...\r\n", fileName);
                        LoglibPrintf(LOG_TYPE_INFO, str);
                        #else
                        sysprintf("--[INFO]--> FtpClientSendFile[%s] OK...\r\n", fileName);
                        #endif
                    }   
                    #if(1)
                    if(FtpClientClose())
                    {
                        #if(ENABLE_LOG_FUNCTION)
                        {
                            char str[512];
                            sprintf(str, " !!!!!! ~ FTP_CLOSE SUCCESS (loadSimpleFile) ~!!!!!!\r\n");
                            LoglibPrintf(LOG_TYPE_INFO, str);
                        }
                        #endif
                    }
                    else
                    {
                        #if(ENABLE_LOG_FUNCTION)
                        {
                            char str[512];
                            sprintf(str, " !!!!!! ~ FTP_CLOSE ERROR (loadSimpleFile) ~!!!!!!\r\n");
                            LoglibPrintf(LOG_TYPE_INFO, str);
                        }
                        #endif
                    }
                    #endif
                }
                else
                {
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[512];
                        sprintf(str, " !!!!!! ~ FTP_CONNECT ERROR (loadSimpleFile) ~!!!!!!\r\n");
                        LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                    #endif
                    returnVal = FALSE;
                }
                //sysDelay(5000/portTICK_RATE_MS);//
            }
            /*
            else if(type == DATA_SEND_TYPE_WEB_POST)
            {
                if(WebPostMessage(WEB_POST_ADDRESS, (uint8_t*)dataTmp) == FALSE)
                {
                    sysprintf("--[INFO]--> WebPostMessage[%s] ERROR...\r\n", fileName);
                    //char str[256];
                    //sprintf(str, "  -X-> WebPostMessage[%s] ERROR...\r\n", fileName);
                    //LoglibPrintf(LOG_TYPE_ERROR, str); 
                    returnVal = FALSE;
                }  
                else
                {
                    sysprintf("--[INFO]--> WebPostMessage[%s] OK...\r\n", fileName);
                    #if(LOG_SUCCESS_ENABLE)
                    //char str[256];
                    //sprintf(str, "  -O-> WebPostMessage[%s] OK...\r\n", fileName);
                    //LoglibPrintf(LOG_TYPE_INFO, str); 
                    #endif
                } 
            }
            */
        }
    }
    if(returnVal == TRUE)
    {
        if(FileAgentDelFile(storageType, dir, fileName) != FILE_AGENT_RETURN_ERROR)
        {
            sysprintf("--[INFO]--> loadSimpleFile[%s%s], delete it OK...\r\n", dir, fileName);
        }
        else
        {
            sysprintf("--[INFO]--> loadSimpleFile[%s%s], delete it ERROR...\r\n", dir, fileName);
            //char str[1024];
            //sprintf(str, " !!! loadSimpleFile[%s%s], delete it ERROR...\r\n", dir, fileName);
            //LoglibPrintf(LOG_TYPE_ERROR, str);
        }
    }
    else
    {
        sysprintf("--[INFO]--> loadSimpleFile[%s%s], ignore delete it...\r\n", dir, fileName);
    }
    if(needFree)
    {
        vPortFree(dataTmp);
    }
    return returnVal;    
}
//static BOOL sendDataNeedChdirFlag = FALSE;
static BOOL sendDataCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4)
{    
    BOOL reVal = TRUE;
    DataSendType* type = (DataSendType*)para1;
    StorageType* storageType = (StorageType*)para2;
    BOOL* checkMode = (BOOL*)para3;
    char* ftpdir = (char*)para4;
    //sysprintf(" - sendDataCallback copy [%d] [%s %s]!!\n", tariffIndex, dir, filename);
    if(loadSimpleFile(TRUE, *storageType, dir, filename, ftpdir, *type, *checkMode) == FALSE)
    {
        //sysprintf("[sendDataCallback] LoadFile error\r\n");
        reVal = FALSE;
    }
    else
    {
        //sendDataNeedChdirFlag = FALSE;
    }
    //vTaskDelay(1000/portTICK_RATE_MS);
    return reVal;
}
static BOOL sendDataFile (StorageType storageType, char* dir, char* extensionName, char* ftpdir, DataSendType type, char* excludeFileName, BOOL checkMode)
{
    //sendDataNeedChdirFlag = TRUE;
    return FileAgentGetList(storageType, dir, extensionName, excludeFileName, sendDataCallback, &type, &storageType, &checkMode, ftpdir);
}
static BOOL needReloadFlag = FALSE;
static BOOL getDataFileCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4)
{
    int i;
    sysprintf(" - WARNING [getDataFileCallback]-> [%s %s] !!\n", dir, filename);
    FileAgentTargetFileName* targetFileName = (FileAgentTargetFileName*)para1;
    int targetFileNum = *(int*)para2;
    StorageType* storageType = (StorageType*)para3;
    for(i = 0; i<targetFileNum; i++)
    {   
        if(strlen(targetFileName[i].name) != 0)
        {
            //sysprintf(" -  WARNING [getDataFileCallback]->  compare [%s %s] !!\n", GetMeterData()->targetTariffFileName[i].name, filename);
            if(strcmp(targetFileName[i].name, filename) == 0)
            {
                sysprintf(" - WARNING [getDataFileCallback]->  get it [%s %s] !!\n", targetFileName[i].name, filename);
                targetFileName[i].existFlag = TRUE;
                break;
            }
        }
    }
    //sysprintf(" - WARNING [checkCallback] for exit -> [%d %d] !!\n", i, targetFileNum);
    if(i == targetFileNum)
    {
        #if(1)
        sysprintf(" - WARNING [getDataFileCallback]->  delete [%s %s] !!\n", dir, filename);
        FileAgentDelFile(*storageType, dir, filename);
        #else
        char fileName[_MAX_LFN];
        sysprintf(" - WARNING [getDataFileCallback]->  delete [%s %s] !!\n", dir, filename);
        sprintf(fileName, "%s%s", dir, filename);
        f_unlink(fileName);
        #endif
        needReloadFlag = TRUE;
        return TRUE;
    }   
    else
    {        
        //return FALSE;
        return TRUE;
    }
}
static BOOL getDataFile (StorageType storageType, char* localDir, char* ftpDir, char* extensionName, DataSendType type, char* excludeFileName, FileAgentTargetFileName* targetFileName, int targetFileNum, DataProcessFtpGetReloadCallback reloadFunc)
{
    sysprintf("--[WARNING]--> getDataFile[localDir = %s][ftpDir = %s][extensionName = %s] \r\n", localDir, ftpDir, extensionName);
    needReloadFlag = FALSE;
    
    for(int i = 0; i<targetFileNum; i++)
    {
        targetFileName[i].existFlag = FALSE;
        sysprintf("--[WARNING]--> targetFileName[%d]:[%s] \r\n", i, targetFileName[i].name);
    }

    FileAgentGetList(storageType, localDir, extensionName, excludeFileName, getDataFileCallback, targetFileName, &targetFileNum, &storageType, NULL);

    for(int i = 0; i<targetFileNum; i++)
    {
        if(strlen(targetFileName[i].name) != 0)
        {
            if(targetFileName[i].existFlag == FALSE)
            {
                char ftpTargetDir[_MAX_LFN];         
                sprintf(ftpTargetDir, "%s%s", FTP_PRE_PATH, ftpDir);
                
                QModemFtpClientStart();
                if(QModemFtpClientProcess() == TRUE)
                {
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[512];
                        sprintf(str, " !!!!!! ~ FTP_CONNECT SUCCESS (getDataFile) ~!!!!!!\r\n");
                        LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                    #endif
                    if(FtpClientGetFile(ftpTargetDir, targetFileName[i].name) == FALSE)
                    {
                       sysprintf("--[WARNING]--> FtpClientGetFile[%s, %s] ERROR...\r\n", ftpTargetDir, targetFileName[i].name);
                    }  
                    else
                    {
                        sysprintf("--[WARNING]--> FtpClientGetFile[%s, %s] OK...\r\n", ftpTargetDir, targetFileName[i].name);
                        needReloadFlag = TRUE;
                    }   
                    #if(1)
                    if(FtpClientClose())
                    {
                        #if(ENABLE_LOG_FUNCTION)
                        {
                            char str[512];
                            sprintf(str, " !!!!!! ~ FTP_CLOSE SUCCESS (getDataFile) ~!!!!!!\r\n");
                            LoglibPrintf(LOG_TYPE_INFO, str);
                        }
                        #endif
                    }
                    else
                    {
                        #if(ENABLE_LOG_FUNCTION)
                        {
                            char str[512];
                            sprintf(str, " !!!!!! ~ FTP_CLOSE ERROR (getDataFile) ~!!!!!!\r\n");
                            LoglibPrintf(LOG_TYPE_INFO, str);
                        }
                        #endif
                    }
                    #endif
                }
                else
                {
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[512];
                        sprintf(str, " !!!!!! ~ FTP_CONNECT ERROR (getDataFile) ~!!!!!!\r\n");
                        LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                    #endif
                }
            }
        }
    }
    if(needReloadFlag)
    {
        sysprintf("--[WARNING]--> needReloadFlag = TRUE...\r\n");
        if(reloadFunc != NULL)
        {
            reloadFunc();
        }
    }
    else
    {
        sysprintf("--[WARNING]--> needReloadFlag = FALSE...\r\n");
    }
    return TRUE;
}
                                        
static DataSendType checkProcessExecuteFlag(uint32_t currentTime)
{
    int i;
    DataSendType reval = DATA_SEND_TYPE_NONE;
    for (i = 0; ; i++)
    {
        if(processItem[i].processName == NULL)
            break;
        if(processItem[i].executeFlag == FALSE)
        {
          
        }
        else
        {
            //sysprintf("\r\n  -[info]-> proecess check Get it: [%s] !!\n", processItem[i].processName);
            reval = reval | (processItem[i].sendType);
        }
    }
    
    //sysprintf("\r\n  -[info]-> proecess check: reval = 0x%02x !!\n", reval);
    return reval;
}

static BOOL executeProcess(uint32_t currentTime, DataSendType type)
{
    int i;
    int reExecuteTimes = 1;//RE_EXECUTE_TIMES; 一次就好
    BOOL reval = FALSE;
    //sysprintf("\r\n  -[info]-> executeProcess: currentTime = %d, type = %d !!\n", currentTime, reval);
    while((reExecuteTimes > 0) && (reval == FALSE)) //重作3次
    {        
        reval = TRUE;
        for (i = 0; ; i++)
        {
            if(processItem[i].processName == NULL)
                break;
            if((processItem[i].executeFlag == TRUE) && (type == processItem[i].sendType))
            {
                #if(ENABLE_LOG_FUNCTION)
                {
                    char str[256];
                    sprintf(str, "<<<< [ INFO ] >>>> execute %s Process[%d] : Start(reExecuteTimes=%d) !!\r\n", processItem[i].processName, i, reExecuteTimes);
                    LoglibPrintf(LOG_TYPE_INFO, str);
                }
                #else
                sysprintf("\r\n  <<<< [ INFO ] >>>> execute %s Process[%d] : Start(reExecuteTimes=%d) !!\r\n", processItem[i].processName, i, reExecuteTimes);
                #endif
                if(processItem[i].executeFunc(currentTime, processItem[i].sendType))
                {
                    processItem[i].executeFlag = FALSE;
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[256];
                        sprintf(str, "<<<< [ SUCCESS ] >>>> execute %s Process[%d] : Execute OK !!\r\n", processItem[i].processName, i);
                        LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                    #else
                    sysprintf("\r\n  <<<< [ SUCCESS ] >>>> execute %s Process[%d] : Execute OK !!\r\n", processItem[i].processName, i);
                    #endif
                }
                else
                {
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[256];
                        sprintf(str, "<<<< [ ERROR ] >>>> execute %s Process[%d]: Execute ERROR !!\r\n", processItem[i].processName, i);
                        LoglibPrintf(LOG_TYPE_ERROR, str);
                    }
                    #else
                    sysprintf("\r\n  <<<< [ ERROR ] >>>> execute %s Process[%d]: Execute ERROR !!\r\n", processItem[i].processName, i);
                    #endif
                    reval = FALSE;
                }
            }
            else
            {
                //sysprintf("\r\n  ~[info]~> execute Process[%d] %s: Ignore !!\n", i, processItem[i].processName);               
            }
        }
        reExecuteTimes--;
    }
    //if(reval)
    //{
    //    char str[256];
    //    sprintf(str, " => executeProcess(type = %d): TRUE [reExecute left %d Times]...\r\n", type, reExecuteTimes);
    //    LoglibPrintf(LOG_TYPE_INFO, str);
    //}
    //else
    //{
    //    char str[256];
    //    sprintf(str, " => executeProcess(type = %d): FALSE [reExecute left %d Times]...\r\n", type, reExecuteTimes);
    //    LoglibPrintf(LOG_TYPE_ERROR, str);
    //}
    
    return reval;
}


//-----  status -----
static BOOL statusExecuteFunc(uint32_t currentTime, DataSendType type)
{
    //sysprintf("\r\n     -[info]-> status Execute Func[%d] !!\n", currentTime);
    //return sendEsfFile(FILE_EXTENSION_EX(STATUS_FILE_EXTENSION));  
    BOOL reVal = TRUE;
    char *statusData;    
    
    //sysprintf("\r\n     -[info]-> statusExecuteFunc [%d], uxQueueGetQueueNumber = %d !!\n", currentTime, uxQueueGetQueueNumber(xStatusDataQueue));
    while( xQueueReceive(xStatusDataQueue, &statusData, 0 ) == pdPASS )
    {
        BOOL postValue = FALSE;
        int reExecuteTimes = RE_EXECUTE_TIMES;
        while((reExecuteTimes > 0) && (postValue == FALSE)) //重作3次
        {
            postValue = TRUE;
            //sysprintf("\r\n     -[info]-> statusExecuteFunc statusData = [%s] !!\n", statusData);
            if(WebPostMessage(WEB_POST_ADDRESS, (uint8_t*)statusData) == FALSE)
            {                
                #if(ENABLE_LOG_FUNCTION)
                {
                    char str[1024];
                    sprintf(str, "  -X-> WebPostMessage[%s] ERROR...\r\n", statusData);
                    LoglibPrintf(LOG_TYPE_ERROR, str);
                }
                #else
                sysprintf("--[INFO]--> WebPostMessage[%s] ERROR...\r\n", statusData);
                #endif                
                postValue = FALSE;
            }  
            else
            {
                
                #if(ENABLE_LOG_FUNCTION)
                {
                    char str[1024];
                    sprintf(str, "  -O-> WebPostMessage[%s] OK...\r\n", statusData);
                    LoglibPrintf(LOG_TYPE_INFO, str); 
                }
                #else
                sysprintf("--[INFO]--> WebPostMessage[%s] OK...\r\n", statusData);
                #endif                
            } 
            reExecuteTimes--;
        }
        if(postValue == FALSE)
        {   
            if(pdPASS == xQueueSendToFront( xStatusDataQueue, ( void * ) &statusData, ( TickType_t ) 0 ))
            {
                #if(ENABLE_LOG_FUNCTION)
                {
                    char str[1024];
                    sprintf(str, "  -x-> WebPostMessage ERROR, insert back to queue OK, and break...\r\n");
                    LoglibPrintf(LOG_TYPE_ERROR, str); 
                }
                #endif           
            }
            else
            {
                #if(ENABLE_LOG_FUNCTION)
                {
                    char str[1024];
                    sprintf(str, "  -x-> WebPostMessage ERROR, insert back to queue error, and break...\r\n");
                    LoglibPrintf(LOG_TYPE_ERROR, str); 
                }
                #endif 
            }
            reVal = FALSE; 
            break;

        }
        else
        {
            #if(ENABLE_LOG_FUNCTION)
            {
                char str[1024];
                sprintf(str, "  -o-> WebPostMessage OK, vPortFree(statusData)...\r\n");
                LoglibPrintf(LOG_TYPE_INFO, str); 
            }
            #endif
            vPortFree(statusData); 
            reVal = TRUE; 
        }
        
    }
    return reVal;
}

//-----  dsf -----
static BOOL dsfExecuteFunc(uint32_t currentTime, DataSendType type)
{
    return sendDataFile(DSF_FILE_SAVE_POSITION, DSF_FILE_DIR, FILE_EXTENSION_EX(DSF_FILE_EXTENSION), GetMeterData()->dsfFileFTPPath, type, NULL, TRUE);    
}

//-----  dcf -----
static BOOL dcfExecuteFunc(uint32_t currentTime, DataSendType type)
{
    RTC_TIME_DATA_T pt;
    return sendDataFile(DCF_FILE_SAVE_POSITION, DCF_FILE_DIR, FILE_EXTENSION_EX(DCF_FILE_EXTENSION), GetMeterData()->dcfFileFTPPath, type, MeterGetCurrentDCFFileName(&pt), FALSE);    
}

//-----  photo -----
static BOOL photoExecuteFunc(uint32_t currentTime, DataSendType type)
{
    //sysprintf("\r\n     -[info]-> photo Execute Func[%d] !!\n", currentTime);
    return sendDataFile(PHOTO_SAVE_POSITION, PHOTO_FILE_DIR, FILE_EXTENSION_EX(PHOTO_FILE_EXTENSION), GetMeterData()->jpgFileFTPPath, type, NULL, TRUE);                
}

//-----  log -----
static BOOL logExecuteFunc(uint32_t currentTime, DataSendType type)
{
    //sysprintf("\r\n     -[info]-> log Execute Func[%d] !!\n", currentTime);
    return sendDataFile(LOG_SAVE_POSITION, LOG_FILE_DIR, FILE_EXTENSION_EX(LOG_FILE_EXTENSION), GetMeterData()->logFileFTPPath, type, LoglibGetCurrentLogFileName(), FALSE);                
}
//-----  tariff  -----
static BOOL tariffReloadCallback(void)
{
    TariffLoadTariffFile();
    TariffUpdateCurrentTariffData();
    return TRUE;
}

static BOOL tariffExecuteFunc(uint32_t currentTime, DataSendType type)
{
    sysprintf("--[WARNING]--> tariffExecuteFunc \r\n");
    return getDataFile(TARIFF_FILE_SAVE_POSITION, TARIFF_FILE_DIR, GetMeterData()->tariffFileFTPPath, FILE_EXTENSION_EX(TARIFF_FILE_EXTENSION), type, NULL, GetMeterData()->targetTariffFileName, TARIFF_FILE_NUM, tariffReloadCallback);
}
// ----- para  -----
static BOOL paraReloadCallback(void)
{
    MeterDataReloadParaFile();
    GuiManagerRefreshScreen();  
    return TRUE;
}
static BOOL paraExecuteFunc(uint32_t currentTime, DataSendType type)
{
    sysprintf("--[WARNING]--> paraExecuteFunc \r\n");
    return getDataFile(EPM_PARA_SAVE_POSITION, EPM_PARA_FILE_DIR, GetMeterData()->paraFileFTPPath, FILE_EXTENSION_EX(EPM_PRAR_FILE_EXTENSION), type, NULL, &GetMeterData()->targetParaFileName, 1, paraReloadCallback);

}

BOOL setExecuteFlag(DataProcessId id)
{
    int i;
    for (i = 0; ; i++)
    {
        if(processItem[i].processName == NULL)
            break;
        if(processItem[i].dataProcessId == id)
        {
            processItem[i].executeFlag = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL DataProcessLibInit(void)
{
    sysprintf("DataProcessLibInit!! \r\n");
 
    xStatusDataQueue = xQueueCreate( STATUS_DATA_QUEUE_SIZE, ( unsigned portBASE_TYPE ) sizeof(char*));
    if(xStatusDataQueue == NULL)
    {
        sysprintf("ERROR DataProcessLibInit(xStatusDataQueue == NULL)\n");
    }
 
    return TRUE;
}

BOOL DataProcessCheckExecute(uint32_t currentTime)
{
    return checkProcessExecuteFlag(currentTime);
}

BOOL DataProcessExecute(uint32_t currentTime, DataSendType type)
{
    return executeProcess(currentTime, type);
}

BOOL DataProcessSetExecuteFlag(DataProcessId id)
{
    if(id != DATA_PROCESS_ID_ESF)
        setExecuteFlag(DATA_PROCESS_ID_LOG);//有要傳檔案 順便傳就好
    return setExecuteFlag(id);
}

BOOL DataProcessSendStatusData(uint32_t currentTime, char* flagStr)
{
    BOOL reval = TRUE;
    char *out = NULL;   
    char *outurlencoded = NULL;
    UINT32 leftVoltage, rightVoltage;
    
    if(xStatusDataQueue == NULL)
    {
        sysprintf(" !! DataProcessSendStatusData ignore (xStatusDataQueue == NULL)\n");
        return FALSE;
    }
    sysprintf(" !! DataProcessSendStatusData Start currentTime...\r\n");
    if(currentTime == 0)
        currentTime = GetCurrentUTCTime();
    
    //{
    //char str[1024];
    //sprintf(str, " !! DataProcessSendStatusData[%d] !!\r\n", currentTime);
    //LoglibPrintf(LOG_TYPE_INFO, str); 
    //}
    sysprintf(" !! DataProcessSendStatusData Start get string:[currentTime = %d]\r\n", currentTime);
    
    BatteryGetValue(NULL,&rightVoltage, &leftVoltage);

    //char* JsonCmdCreateStatusData(int jsonver, char* epmver, char* flag, char* epmid, int time, int index, BOOL bay1, BOOL bay2, 
    //                            int baydist1, int baydist2, int voltage1, int voltage2, 
    //                            int deposit1, int deposit2, int deposit3, int deposit4, int deposit5, int deposit6,
    //                            char* tariff, char* setting);
    out = JsonCmdCreateStatusData(JSON_CMD_VER, GetMeterData()->buildStr, flagStr, GetMeterData()->epmIdStr, currentTime, esfIndex++, 
                                GetSpaceStatus(SPACE_INDEX_1), GetSpaceStatus(SPACE_INDEX_2), 
                                GetSpaceDist(SPACE_INDEX_1), GetSpaceDist(SPACE_INDEX_2), 
                                leftVoltage, rightVoltage, 
                                GetMeterStorageData()->depositEndTime[0], GetMeterStorageData()->depositEndTime[1], GetMeterStorageData()->depositEndTime[2], 
                                GetMeterStorageData()->depositEndTime[3], GetMeterStorageData()->depositEndTime[4], GetMeterStorageData()->depositEndTime[5],
                                TariffGetFileName(), GetMeterPara()->name); 
                                
    sysprintf(" !! DataProcessSendStatusData out string OK:[%s]\r\n", out);
    
    outurlencoded = pvPortMalloc(strlen(out) + strlen("data=") + 1);
    if(outurlencoded != NULL)
    {
        sprintf(outurlencoded, "%s%s", "data=", out);  
        sysprintf("\r\n !! DataProcessSendStatusData outurlencoded string OK:[%s]\r\n", out);        
        //sysprintf("\r\n     -[info 1]-> statusExecuteFunc [%d], result = %d, uxQueueGetQueueNumber = %d !!\n", currentTime, result, uxQueueGetQueueNumber(xStatusDataQueue));
        if(xQueueIsQueueFullFromISR(xStatusDataQueue))
        {
            char *statusData;
            if( xQueueReceive(xStatusDataQueue, &statusData, 0 ) == pdPASS)
            {
                //char str[1024];
                //sprintf(str, " !! DataProcessSendStatusData [%d] FULL:  xQueueReceive the oldest OK!!\r\n", currentTime);   
                //LoglibPrintf(LOG_TYPE_INFO, str);  
                //sysprintf("\r\n     -[info 1]-> DataProcessSendStatusData [%s] !!\n", statusData);
                vPortFree(statusData);                 
            }
            else
            {
                //char str[1024];
                //sprintf(str, " !! DataProcessSendStatusData [%d] FULL:  xQueueReceive the oldest ERROR!!\r\n", currentTime);   
                //LoglibPrintf(LOG_TYPE_ERROR, str);  
            }
        }
        if(pdPASS == xQueueSendToBack( xStatusDataQueue, ( void * ) &outurlencoded, ( TickType_t ) 0 ))
        {
            sysprintf("~~ DataProcessSendStatusData OK: \r\n"); 
            //char str[1024];
            //sprintf(str, " !! DataProcessSendStatusData[%d, esfIndex = %d] OK!!\r\n", currentTime, esfIndex); 
            //LoglibPrintf(LOG_TYPE_INFO, str);              
        }
        else
        {
            sysprintf("~~ DataProcessSendStatusData ERROR: \r\n");  
            //char str[1024];
            //sprintf(str, " !! DataProcessSendStatusData[%d, esfIndex = %d] ERROR!!\r\n", currentTime, esfIndex);    
            //LoglibPrintf(LOG_TYPE_ERROR, str);  
            reval = FALSE;
        }
        //sysprintf("\r\n     -[info 2]-> statusExecuteFunc [%d], result = %d, uxQueueGetQueueNumber = %d !!\n", currentTime, result, uxQueueGetQueueNumber(xStatusDataQueue));
    }
    else
    {
        reval = FALSE;
    }        
    vPortFree(out);
    return reval;
}


BOOL DataProcessSendData(uint32_t currentTime, int bayid, int time, int cost, int balance, DataId dataType, char* flagStr)
{
    BOOL reval = TRUE;
    char *out = NULL;   
    UINT32 leftVoltage, rightVoltage;
    cJSON *root_json;
    cJSON *extend_json;
    
     if(xStatusDataQueue == NULL)
    {
        sysprintf(" !! DataProcessSendData ignore (xStatusDataQueue == NULL)\n");
        return FALSE;
    }
    
    if(currentTime == 0)
        currentTime = GetCurrentUTCTime();
    if((dataType != DATA_TYPE_ID_TRANSACTION) && (dataType != DATA_TYPE_ID_EXPIRED))
    {
        sysprintf("\r\n !! DataProcessSendData[%d, dataType = %d]  ERROR!!\r\n", currentTime, dataType);
        return FALSE;
    }
    else
    {
        //char str[1024];
        //sprintf(str, " !! DataProcessSendData[%d, dataType = %d] !!\r\n", currentTime, dataType);
        //LoglibPrintf(LOG_TYPE_INFO, str);
    }
 
    BatteryGetValue(NULL,&rightVoltage, &leftVoltage);

    //char* JsonCmdCreateStatusData(int jsonver, char* epmver, char* flag, char* epmid, int time, int index, BOOL bay1, BOOL bay2, 
    //                            int baydist1, int baydist2, int voltage1, int voltage2, 
    //                            int deposit1, int deposit2, int deposit3, int deposit4, int deposit5, int deposit6,
    //                            char* tariff, char* setting);
    out = JsonCmdCreateStatusData(JSON_CMD_VER, GetMeterData()->buildStr, flagStr, GetMeterData()->epmIdStr, currentTime, esfIndex++, 
                                GetSpaceStatus(SPACE_INDEX_1), GetSpaceStatus(SPACE_INDEX_2), 
                                GetSpaceDist(SPACE_INDEX_1), GetSpaceDist(SPACE_INDEX_2), 
                                leftVoltage, rightVoltage, 
                                GetMeterStorageData()->depositEndTime[0], GetMeterStorageData()->depositEndTime[1], GetMeterStorageData()->depositEndTime[2], 
                                GetMeterStorageData()->depositEndTime[3], GetMeterStorageData()->depositEndTime[4], GetMeterStorageData()->depositEndTime[5],
                                TariffGetFileName(), GetMeterPara()->name);  

    root_json = cJSON_Parse((char*)out);
    if(root_json != NULL)
    {
        char* outUnformatted;
        char *outurlencoded = NULL;
        if(dataType == DATA_TYPE_ID_TRANSACTION)
        {
            extend_json = JsonCmdCreateTransactionStatusData(bayid, time, cost, balance);
        }
        else if(dataType == DATA_TYPE_ID_EXPIRED)
        {
            extend_json = JsonCmdCreateExpiredStatusData(bayid);
        }
        if(extend_json != NULL)
        {
            if(dataType == DATA_TYPE_ID_TRANSACTION)
            {
                cJSON_AddItemToObject(root_json, "transaction", extend_json);
            }
            else if(dataType == DATA_TYPE_ID_EXPIRED)
            {
                cJSON_AddItemToObject(root_json, "expired", extend_json);
            }
            outUnformatted = cJSON_PrintUnformatted(root_json);            
            cJSON_Delete(root_json);

            outurlencoded = pvPortMalloc(strlen(outUnformatted) + strlen("data=") + 1);
            if(outurlencoded != NULL)
            {
                sprintf(outurlencoded, "%s%s", "data=", outUnformatted);                              
                //sysprintf("\r\n     -[info 1]-> statusExecuteFunc [%d], result = %d, uxQueueGetQueueNumber = %d !!\n", currentTime, result, uxQueueGetQueueNumber(xStatusDataQueue));
                if(xQueueIsQueueFullFromISR(xStatusDataQueue))
                {
                    char *statusData;
                    if( xQueueReceive(xStatusDataQueue, &statusData, 0 ) == pdPASS)
                    {
                        //char str[1024];
                        //sprintf(str, " !! DataProcessSendData FULL:  xQueueReceive the oldest OK!![%d, dataType = %d] !!\r\n", currentTime, dataType);  
                        //LoglibPrintf(LOG_TYPE_INFO, str);  
                        sysprintf("\r\n     -[info 1]-> DataProcessSendData [%s] !!\n", statusData);
                        vPortFree(statusData);                            
                    }
                    else
                    {
                        //char str[1024];
                        //sprintf(str, " !! DataProcessSendData FULL:  xQueueReceive the oldest ERROR!![%d, dataType = %d] !!\r\n", currentTime, dataType);  
                        //LoglibPrintf(LOG_TYPE_ERROR, str);   
                    }
                }
                if(pdPASS == xQueueSendToBack( xStatusDataQueue, ( void * ) &outurlencoded, ( TickType_t ) 0 ))
                {
                    sysprintf("~~ DataProcessSendData OK: \r\n");  
                    //char str[1024];
                    //sprintf(str, " !! DataProcessSendData OK [%d, esfIndex = %d, dataType = %d] !!\r\n", currentTime, esfIndex, dataType);
                    //LoglibPrintf(LOG_TYPE_INFO, str);  
                }
                else
                {
                    sysprintf("~~ DataProcessSendData ERROR: \r\n"); 
                    //char str[1024];
                    //sprintf(str, " !! DataProcessSendData ERROR [%d, esfIndex = %d, dataType = %d] !!\r\n", currentTime, esfIndex, dataType);
                    //LoglibPrintf(LOG_TYPE_INFO, str);  
                    reval = FALSE;
                }
                //sysprintf("\r\n     -[info 2]-> statusExecuteFunc [%d], result = %d, uxQueueGetQueueNumber = %d !!\n", currentTime, result, uxQueueGetQueueNumber(xStatusDataQueue));
            }
            else
            {
                sysprintf("sendTransactionStatusData error: if(outurlencoded != NULL)!!\n");
                reval = FALSE;
            }   
            vPortFree(outUnformatted);   
        }
        else
        {
            sysprintf("sendTransactionStatusData error: if(extend_json != NULL)!!\n");
            reval = FALSE;
        }
    }
    else
    {
        sysprintf("sendTransactionStatusData error: if(root_json != NULL) [%s]!!\n", out);
        reval = FALSE;
    }    

    vPortFree(out); 

    return reval;
}

BOOL DataParserWebPostReturnData(char* jsonStr)
{
    cJSON *root_json;
    cJSON *tmp_json, *tmp_json2;
    
    sysprintf(" == DataParserWebPostReturnData [%s]\r\n", jsonStr);  
    //{"ok":1,"errorcode":0,"datetime":1488418637,"tarifffile":["tariff.tre","tariff1.tre",""],"parafile":"epm.pre"}
    
    //sysprintf("!!! parserString = [%s], destLen = %d, sizeof(double) = %d\r\n", str, destLen, sizeof(double));
     //__SHOW_FREE_HEAP_SIZE__
    root_json = cJSON_Parse((char*)jsonStr);     
    if (NULL != root_json)
    {
        tmp_json = cJSON_GetObjectItem(root_json, "ok");
        if(tmp_json != NULL)
        {
            sysprintf(" >> Web Get ok: %d\n", tmp_json->valueint);
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "errCode");
        if(tmp_json != NULL)
        {
            sysprintf(" >> Web Get errCode: %d\n", tmp_json->valueint);
        }
        //tariff file
        tmp_json = cJSON_GetObjectItem(root_json, "tarifffile");
        if (tmp_json != NULL)
        {
            int i;
            int arrarSize = cJSON_GetArraySize(tmp_json);
            if(arrarSize > TARIFF_FILE_NUM)
                arrarSize = TARIFF_FILE_NUM;
            for(i = 0; i < arrarSize; i++)
            {                       
                tmp_json2 = cJSON_GetArrayItem(tmp_json, i);  
                if (tmp_json2 != NULL)
                {
                    strcpy((char*)GetMeterData()->targetTariffFileName[i].name, (const char*)(tmp_json2->valuestring));
                    sysprintf(" >> Web Get target Tariff File Name[%d]:[%s]\n", i, GetMeterData()->targetTariffFileName[i].name);                    
                }
            }      
        }
        else
        {
            sysprintf(" >> Web Get targetTariffFileName error ((tmp_json != NULL))\n");
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "parafile");
        if (tmp_json != NULL)
        {
            strcpy((char*)GetMeterData()->targetParaFileName.name, (const char*)(tmp_json->valuestring));
            sysprintf(" >> Web Get targetParaFileName:[%s]\n", GetMeterData()->targetParaFileName.name);
        }
        else
        {
            sysprintf(" >> Web Get targetParaFileName error ((tmp_json != NULL))\n");
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "tariffpath");
        if (tmp_json != NULL)
        {
            strcpy((char*)GetMeterData()->tariffFileFTPPath, (const char*)(tmp_json->valuestring));
            sysprintf(" >> Web Get tariffFileFTPPath:[%s]\n", GetMeterData()->tariffFileFTPPath);
        }
        else
        {
            sysprintf(" >> Web Get tariffFileFTPPath error ((tmp_json != NULL))\n");
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "parapath");
        if (tmp_json != NULL)
        {
            strcpy((char*)GetMeterData()->paraFileFTPPath, (const char*)(tmp_json->valuestring));
            sysprintf(" >> Web Get paraFileFTPPath:[%s]\n", GetMeterData()->paraFileFTPPath);
        }
        else
        {
            sysprintf(" >> Web Get paraFileFTPPath error ((tmp_json != NULL))\n");
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "datetime");
        if(tmp_json != NULL)
        {
            RTC_TIME_DATA_T time;
            time_t rawtime = GetCurrentUTCTime();
            //time_t webPostRawtime = (int)(tmp_json->valuedouble/(double)1000)+1;
            #warning 
            //time_t webPostRawtime = tmp_json->valueint+1;
            time_t webPostRawtime = tmp_json->valueint+1 + 8*60*60;
            sysprintf(" >> Web Get datetime: %d (current time:%d)\n", webPostRawtime, rawtime); 
            ///*
            if(abs(webPostRawtime - rawtime) >= 3)
            {
                Time2RTC(webPostRawtime, &time);
                if(SetOSTime(time.u32Year, time.u32cMonth, time.u32cDay, time.u32cHour, time.u32cMinute, time.u32cSecond, time.u32cDayOfWeek) == TRUE)
                {
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[512];
                        sprintf(str, " --> CMD_TIME_CORRECTION_ID _!!!! : [%04d/%02d/%02d %02d:%02d:%02d (%d)]\r\n",
                                                            time.u32Year, time.u32cMonth, time.u32cDay, 
                                                            time.u32cHour, time.u32cMinute, time.u32cSecond, time.u32cDayOfWeek);
                        //LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                    #endif
                    TariffUpdateCurrentTariffData();
                 
                }
            }
            if(abs(webPostRawtime - rawtime) >= 20)
            {               
                GuiManagerRefreshScreen(); 
            }
        }
        
        
        
        
        //sysprintf(" >> cJSON_Print(root_json) = [%s]\n", cJSON_Print(root_json));//印出記得要FREE
        cJSON_Delete(root_json);
        //__SHOW_FREE_HEAP_SIZE__
    }    
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


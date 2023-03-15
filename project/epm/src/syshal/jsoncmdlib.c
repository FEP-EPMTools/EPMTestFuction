/**************************************************************************//**
* @file     jsoncmdlib.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifdef __WINDOWS__
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
#else
    #include <string.h>
    #include "nuc970.h"
    #include "sys.h"

    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "fepconfig.h"
    #include "powerdrv.h"
#endif
#include "jsoncmdlib.h"
#include "fatfslib.h"
#include "meterdata.h"
#include "timelib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static cJSON *statusRoot = NULL;
static cJSON *bayArray = NULL;
static cJSON *bayValueArray = NULL;
static cJSON *voltageArray = NULL;
static cJSON *depositArray = NULL;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
    
/*
{
   "ver" : 1,
   "epmver" : "1.0.1(1612091716)",
   "id" : "00000001",
   "time" : 1488418637,
   "flag" : "reboot",
   "index" : 1,
   "bay" : [ false, false ],
   "baydist" : [ 46, 149 ],
   "voltage" : [ 660, 654 ],
   "deposit" : [ 1488984523, 0, 1488418843, 0, 0, 1488418567 ],
   "tariff" : "TPE_001",
   "setting" : "TPE_001",
}

*/

BOOL JsonCmdLibInit(void)
{
    sysprintf("JsonCmdLibInit!!\n");
    return TRUE;
}
cJSON* JsonCmdCreateTransactionStatusData(int bayid, int time, int cost, int balance)
{
    cJSON *transaction_json;
        /*
            "transaction" : {
                "bayid" : 1,
                "time" : 9000,
                "cost" : 40,
                "balance" : 4958
                },
        */
    transaction_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(transaction_json, "bayid", bayid);
    cJSON_AddNumberToObject(transaction_json, "time", time);
    cJSON_AddNumberToObject(transaction_json, "cost", cost);
    cJSON_AddNumberToObject(transaction_json, "balance", balance);
    return transaction_json;
}

cJSON* JsonCmdCreateExpiredStatusData(int bayid)
{
    cJSON *expired_json;
    expired_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(expired_json, "bayid", bayid);
    return expired_json;
}

char* JsonCmdCreateStatusData(int jsonver, char* epmver, char* flag, char* epmid, int time, int index, BOOL bay1, BOOL bay2, 
                                int baydist1, int baydist2, int voltage1, int voltage2, 
                                int deposit1, int deposit2, int deposit3, int deposit4, int deposit5, int deposit6,
                                char* tariff, char* setting)
{
    char timeStr[_MAX_LFN];
    char *outUnformatted = NULL;
    sysprintf("~~ cJSON_PrintUnformatted: ENTER \r\n");    
    statusRoot = cJSON_CreateObject();
    if(statusRoot != NULL)
    {
        cJSON_AddNumberToObject(statusRoot, "jsonver", jsonver);
        cJSON_AddStringToObject(statusRoot, "epmver", epmver);
        cJSON_AddStringToObject(statusRoot, "id", epmid);
        cJSON_AddNumberToObject(statusRoot, "time", time);
        if(UTCTimeToString(time, timeStr))
        {
            cJSON_AddStringToObject(statusRoot, "timestr", timeStr);
        }
        cJSON_AddStringToObject(statusRoot, "flag", flag);
        cJSON_AddNumberToObject(statusRoot, "wakeuptick", PowerGetTotalWakeupTick());
        cJSON_AddNumberToObject(statusRoot, "index", index);
        cJSON_AddNumberToObject(statusRoot, "errorcode", GetMeterData()->meterErrorCode);
        
        
        
        cJSON_AddItemToObject(statusRoot, "bay", bayArray = cJSON_CreateArray());
        if(bay1)
        {
            cJSON_AddItemToArray(bayArray, cJSON_CreateTrue());
        }
        else
        {
            cJSON_AddItemToArray(bayArray, cJSON_CreateFalse());
        }
        if(bay2)
        {
            cJSON_AddItemToArray(bayArray, cJSON_CreateTrue());
        }
        else
        {
            cJSON_AddItemToArray(bayArray, cJSON_CreateFalse());
        }
        cJSON_AddNumberToObject(statusRoot, "heap", xPortGetFreeHeapSize());
        
        cJSON_AddItemToObject(statusRoot, "flash", bayValueArray = cJSON_CreateArray());
        cJSON_AddItemToArray(bayValueArray, cJSON_CreateNumber(FatfsGetDiskUseageEx("1:")));
        cJSON_AddItemToArray(bayValueArray, cJSON_CreateNumber(FatfsGetDiskUseageEx("2:")));
        
        cJSON_AddNumberToObject(statusRoot, "flasherror", FlashDrvExGetErrorTimes());
        cJSON_AddNumberToObject(statusRoot, "formatcounter", FileAgentGetFatFsAutoFormatCounter());
        
        cJSON_AddItemToObject(statusRoot, "baydist", bayValueArray = cJSON_CreateArray());
        cJSON_AddItemToArray(bayValueArray, cJSON_CreateNumber(baydist1));
        cJSON_AddItemToArray(bayValueArray, cJSON_CreateNumber(baydist2));
        
        cJSON_AddItemToObject(statusRoot, "voltage", voltageArray = cJSON_CreateArray());
        cJSON_AddItemToArray(voltageArray, cJSON_CreateNumber(voltage1));
        cJSON_AddItemToArray(voltageArray, cJSON_CreateNumber(voltage2));
        
        cJSON_AddItemToObject(statusRoot, "deposit", depositArray = cJSON_CreateArray());
        cJSON_AddItemToArray(depositArray, cJSON_CreateNumber(deposit1));
        cJSON_AddItemToArray(depositArray, cJSON_CreateNumber(deposit2));
        cJSON_AddItemToArray(depositArray, cJSON_CreateNumber(deposit3));
        cJSON_AddItemToArray(depositArray, cJSON_CreateNumber(deposit4));
        cJSON_AddItemToArray(depositArray, cJSON_CreateNumber(deposit5));
        cJSON_AddItemToArray(depositArray, cJSON_CreateNumber(deposit6));
        
        cJSON_AddStringToObject(statusRoot, "tariff", tariff);
        cJSON_AddStringToObject(statusRoot, "setting", setting);
        
        outUnformatted = cJSON_PrintUnformatted(statusRoot);
        sysprintf("~~ cJSON_PrintUnformatted: \r\n[%s]\r\n", outUnformatted);    
        cJSON_Delete(statusRoot);
        
    }
    else
    {
        sysprintf("~~ cJSON_PrintUnformatted: ERROR (statusRoot != NULL)\r\n");    
    }
    return outUnformatted;
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


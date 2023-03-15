/**************************************************************************//**
* @file     tarifflib.c
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
#include <time.h>
#include "nuc970.h"
#include "sys.h"
#include "rtc.h"
#include "gpio.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "tarifflib.h"
#include "cJSON.h"
#include "osmisc.h"
#include "fileagent.h"
#include "meterdata.h"
#include "loglib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static JsonTariff jsonTariff[TARIFF_FILE_NUM];
static JsonTariffSetting* currentJsonTariffType = NULL;
static JsonTariffDayType* currentJsonTariffDayType = NULL;
static int currentTariffFileIndex = 0xff;
static char tariffFileName[TARIFF_FILE_NUM][_MAX_LFN];
static int tariffEffectiveDate[TARIFF_FILE_NUM] = { 0, 0, 0};
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static int compare(const void *arg1, const void *arg2) 
{
    
    return  (*(int *)arg1 - *(int *)arg2);
}


static void sortTariffFile(void)
{
    //int i;
    //for(i = 0; i<TARIFF_FILE_NUM; i++)
    //{
    //    sysprintf(" > sortTariffFile before [%d]: [%d] \r\n", i, tariffEffectiveDate[i]);
    //}
    qsort((void *)tariffEffectiveDate, TARIFF_FILE_NUM, sizeof(int), compare);
    //for(i = 0; i<TARIFF_FILE_NUM; i++)
    //{
    //    sysprintf(" > sortTariffFile after [%d]: [%d] \r\n", i, tariffEffectiveDate[i]);
    //}
}

static BOOL selectTariffFile(void)
{
    int i;
    RTC_TIME_DATA_T pt;
    currentTariffFileIndex = 0xff;
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        int targetDate = pt.u32Year * 10000 + pt.u32cMonth * 100 + pt.u32cDay;
        int targetDateTmp = 0;;
        for(i = 0; i<TARIFF_FILE_NUM; i++)
        {
            sysprintf(" > selectTariffFile check [%d]: [%d:%d] \r\n", i, tariffEffectiveDate[i], targetDate);
            if(targetDate < tariffEffectiveDate[i])
            {
                if(i == 0)
                {
                    sysprintf(" > selectTariffFile ERROR [%d]: [%d:%d] \r\n", i, tariffEffectiveDate[i], targetDate);
                    return FALSE;
                }
                else
                {
                    targetDateTmp = tariffEffectiveDate[i-1];
                    sysprintf(" > selectTariffFile get date [%d]: [%d] \r\n", i, targetDateTmp);
                    break;
                }
            }
        }
        if((i == TARIFF_FILE_NUM) || (targetDateTmp == 0))
        {
            sysprintf(" > selectTariffFile get date_2 [%d]: [%d] \r\n", i, targetDateTmp);
            targetDateTmp = tariffEffectiveDate[i-1];
        }
        for(i = 0; i<TARIFF_FILE_NUM; i++)
        {
            //sysprintf(" > selectTariffFile compare [%d]: [%d:%d] \r\n", i, jsonTariff[i].effectivetime, targetDateTmp);
            if(targetDateTmp == jsonTariff[i].effectivetime)
            {
                sysprintf(" > selectTariffFile get it [%d][%s]: [%d:%d] \r\n", i, jsonTariff[i].name, jsonTariff[i].effectivetime, targetDateTmp);
                currentTariffFileIndex = i;
                return TRUE;
            }
        }
        
    }
    return FALSE;
}
static int tariffIndex = 0;
static BOOL checkTariffName(char* filename)
{
    for(int i = 0; i < tariffIndex; i++)
    {
        if(strcmp(tariffFileName[i], filename) == 0)
        {
            sysprintf(" - checkTariffName exist [%d] [%s]!!\n", i, filename);
            return TRUE;
        }
        else
        {
            //sysprintf(" - checkTariffName not exist [%d] [%s : %s]!!\n", i, tariffFileName[i], filename);
        }
    }
    return FALSE;
}
static BOOL tariffCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4)
{    
    if(tariffIndex < TARIFF_FILE_NUM)
    {        
        if(checkTariffName(filename) == FALSE)
        {
            //sysprintf(" - tariffCallback copy [%d] [%s %s]!!\n", tariffIndex, dir, filename);
            strcpy(tariffFileName[tariffIndex], filename);
            tariffIndex++;
        }
    }
    else
    {
        //sysprintf(" - tariffCallback ignore [%d] [%s %s]!!\n", tariffIndex, dir, filename);
    }
    return TRUE;
}


static BOOL loadTariffFileName ()
{
    tariffIndex = 0;
    memset(tariffFileName, 0x0, TARIFF_FILE_NUM*_MAX_LFN);

    FileAgentGetList(TARIFF_FILE_SAVE_POSITION, TARIFF_FILE_DIR, FILE_EXTENSION_EX(TARIFF_FILE_EXTENSION), NULL, tariffCallback, NULL, NULL, NULL, NULL);
    
    sysprintf(" -->> INFO >> Tariff File Name[%d] [%s] \r\n", 0, tariffFileName[0]);
    sysprintf(" -->> INFO >> Tariff File Name[%d] [%s] \r\n", 1, tariffFileName[1]);
    sysprintf(" -->> INFO >> Tariff File Name[%d] [%s] \r\n", 2, tariffFileName[2]);
    return TRUE;
}
static void resetJsonTariff(int index)
{
    memset(&jsonTariff[index], 0x0, sizeof(JsonTariff));

}
static void printJsonTariff(int index)
{
    sysprintf("\r\n ---------- jsonver: %d (%s, %s)(effective : %d) -----------\r\n", jsonTariff[index].jsonver, jsonTariff[index].createTime, jsonTariff[index].modifyTime, jsonTariff[index].effectivetime);
#if(0)
    sysprintf(" > week: ");     
    for(i = 0; i<TARIFF_WEEK_NUM; i++)
    {
        sysprintf("[%d]:%d, ", i, jsonTariff[index].week[i]); 
    }
    sysprintf("\r\n"); 
    sysprintf(" > JsonTariffDayType: \r\n");     
    for(i = 0; i<TARIFF_DAY_TYPE_NUM; i++)
    {        
        for(j = 0; j<TARIFF_DAY_TYPE_ITEM_NUM; j++)
        {
            sysprintf("   >> [%d, %d]:", i, j); 
            sysprintf(" tariffsetting: %d, hour / minute: %02d / %02d, continued: %d\r\n", 
                                jsonTariff[index].dayType[i].dayTypeItem[j].tariffsetting, 
                                jsonTariff[index].dayType[i].dayTypeItem[j].hour, jsonTariff[index].dayType[i].dayTypeItem[j].minute,
                                jsonTariff[index].dayType[i].dayTypeItem[j].continued); 
        }
    }
    sysprintf(" > JsonTariffSetting: \r\n");
    for(i = 0; i<TARIFF_TARIFF_SETTING_NUM; i++)
    {
        sysprintf("   >> [%d]: ", i); 
        sysprintf("type: %d, timeunit: %d, basecost: %d, costunit: %d, maxcost: %d, maxtime: %d, continuedtime: %d\r\n", 
                                        jsonTariff[index].tariffSetting[i].type, jsonTariff[index].tariffSetting[i].timeunit, jsonTariff[index].tariffSetting[i].basecost,  
                                        jsonTariff[index].tariffSetting[i].costunit, jsonTariff[index].tariffSetting[i].maxcost, jsonTariff[index].tariffSetting[i].maxtime,
                                        jsonTariff[index].tariffSetting[i].continuedtime); 
    }
    sysprintf(" ----------------------------------------\r\n");
#endif
}

static BOOL updateCurrentTariffData(int index)
{    
    RTC_TIME_DATA_T pt;
    int targetIndex = -1;
    BOOL getTargetFlag = FALSE;
    int j;
    if(index >= TARIFF_FILE_NUM)
    {
        sysprintf(" updateCurrentTariffData error: index = %d...\r\n", index);
        return FALSE;
    }        
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        /*
        typedef enum {
            RTC_SUNDAY         =   0,   //!< Sunday    
            RTC_MONDAY         =   1,   //!< Monday    
            RTC_TUESDAY        =   2,   //!< Tuesday   
            RTC_WEDNESDAY      =   3,   //!< Wednesday 
            RTC_THURSDAY       =   4,   //!< Thursday  
            RTC_FRIDAY         =   5,   //!< Friday    
            RTC_SATURDAY       =   6    //!< Saturday  
        } E_RTC_DWR_PARAMETER;
        */
        

        int targetHourMinute = pt.u32cHour * 100 + pt.u32cMinute;
        int tmpHourMinute = pt.u32cHour * 100 + pt.u32cMinute;
        int targetDay = pt.u32cDayOfWeek;

        sysprintf(" updateCurrentTariffData get dayofweek: %d...\r\n", targetDay);
        currentJsonTariffDayType = &jsonTariff[index].dayType[jsonTariff[index].week[targetDay]];
        for(j = 0; j<TARIFF_DAY_TYPE_ITEM_NUM; j++)
        {  
            tmpHourMinute = currentJsonTariffDayType->dayTypeItem[j].hour * 100 + currentJsonTariffDayType->dayTypeItem[j].minute;
            sysprintf("   >check> [%d]: %d vs %d \r\n ", j, targetHourMinute, tmpHourMinute); 
            
            if((tmpHourMinute == 0)&&(j!=0))
            {
                getTargetFlag = TRUE;
                break;
            }            
            else
            {
                if(targetHourMinute >= tmpHourMinute)
                {               
                    targetIndex = j;
                }
                else
                {
                    getTargetFlag = TRUE;
                    break;
                }
            }
            
            
        }        
    }
    
    if((getTargetFlag) || (j == TARIFF_DAY_TYPE_ITEM_NUM))
    {
        if(targetIndex >= 0)
        {
            sysprintf("   !!! Get it [%d] -->", targetIndex); 
            sysprintf(" tariffsetting: %d, hour / minute: %02d / %02d, continued: %d\r\n", 
                            currentJsonTariffDayType->dayTypeItem[targetIndex].tariffsetting, 
                            currentJsonTariffDayType->dayTypeItem[targetIndex].hour, currentJsonTariffDayType->dayTypeItem[targetIndex].minute,
                            currentJsonTariffDayType->dayTypeItem[targetIndex].continued); 
            if((currentJsonTariffDayType->dayTypeItem[targetIndex].tariffsetting >= 0) &&
                (currentJsonTariffDayType->dayTypeItem[targetIndex].tariffsetting < TARIFF_TARIFF_SETTING_NUM))
            {
                currentJsonTariffType = &jsonTariff[index].tariffSetting[currentJsonTariffDayType->dayTypeItem[targetIndex].tariffsetting];
                sysprintf("     Tariff Type--> type: %d, timeunit: %d, basecost: %d, costunit: %d, maxcost: %d, maxtime: %d, continuedtime: %d\r\n", 
                                        currentJsonTariffType->type, currentJsonTariffType->timeunit, currentJsonTariffType->basecost,  
                                        currentJsonTariffType->costunit, currentJsonTariffType->maxcost, currentJsonTariffType->maxtime,
                                        currentJsonTariffType->continuedtime); 
                return TRUE;
            }
        }
    }
    currentJsonTariffDayType = NULL;
    currentJsonTariffType = NULL;
    sysprintf(" updateCurrentTariffData error\r\n");
    MeterSetErrorCode(METER_ERROR_CODE_TARIFF_FILE);  
    return FALSE;

}

#define ENABLE_LOAD_JSON_DEBUG  0
static BOOL loadJsonTariff(int index, char* fileName)
{
    sysprintf("     load loadJsonTariff [%d] [%s]>>\r\n", index, fileName); 
    uint8_t* dataTmp;
    size_t dataTmpLen;
    BOOL needFree;
    FileAgentReturn reVal; 
    cJSON *root_json = NULL;
    cJSON *tmp_json = NULL;
    cJSON *tmp_json2 = NULL;
    cJSON *tmp_json3 = NULL;
    cJSON *tmp_json4 = NULL;
    int errorCode = 0;
    
     //__SHOW_FREE_HEAP_SIZE__
    reVal = FileAgentGetData(TARIFF_FILE_SAVE_POSITION, TARIFF_FILE_DIR, fileName, &dataTmp, &dataTmpLen, &needFree, TRUE);
    if(reVal != FILE_AGENT_RETURN_ERROR)
    {
         //為了在其他DISK備份
        FileAgentAddData(TARIFF_FILE_SAVE_POSITION, TARIFF_FILE_DIR, fileName, dataTmp, dataTmpLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, TRUE); 
        
        root_json = cJSON_Parse((char*)dataTmp);                    
        
        resetJsonTariff(index);
        
        if (NULL == root_json)
        {
            sysprintf("error():%s\n", cJSON_GetErrorPtr());   
            errorCode = 1;
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "jsonver");
        if (tmp_json != NULL)
        {
            jsonTariff[index].jsonver = tmp_json->valueint;
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].jsonver:%d\n", jsonTariff[index].jsonver);
            #endif
        }
        else
        {
            sysprintf("error(jsonver):%s\n", cJSON_GetErrorPtr()); 
            errorCode = 2;                        
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "name");
        if (tmp_json != NULL)
        {
            strcpy((char*)jsonTariff[index].name, (const char*)(tmp_json->valuestring));
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].name:[%s]\n", jsonTariff[index].name);
            #endif
        }
        else
        {
            sysprintf("error(name):%s\n", cJSON_GetErrorPtr());    
            errorCode = 3;                            
            goto jsonExit;
        }
        
        
        tmp_json = cJSON_GetObjectItem(root_json, "effectivetime");
        if (tmp_json != NULL)
        {
            jsonTariff[index].effectivetime = tmp_json->valueint;
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].effectivetime:%d\n", jsonTariff[index].effectivetime);
            #endif
        }
        else
        {
            sysprintf("error(effectivetime):%s\n", cJSON_GetErrorPtr()); 
            errorCode = 4;                            
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "createtime");
        if (tmp_json != NULL)
        {
            strcpy(jsonTariff[index].createTime, tmp_json->valuestring);
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].createTime:[%s]\n", jsonTariff[index].createTime);
            #endif
        }
        else
        {
            sysprintf("error(createtime):%s\n", cJSON_GetErrorPtr()); 
            errorCode = 4;                            
            goto jsonExit;
        }
        
        tmp_json = cJSON_GetObjectItem(root_json, "modifytime");
        if (tmp_json != NULL)
        {
            strcpy(jsonTariff[index].modifyTime, tmp_json->valuestring);
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].modifyTime:[%s]\n", jsonTariff[index].modifyTime);
            #endif
        }
        else
        {
            sysprintf("error(modifytime):%s\n", cJSON_GetErrorPtr());     
            errorCode = 5;                            
            goto jsonExit;
        }
        //week
        tmp_json = cJSON_GetObjectItem(root_json, "week");
        if (tmp_json != NULL)
        {
            int i;
            int arrarSize = cJSON_GetArraySize(tmp_json);
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].week: arrar size = %d\n", arrarSize);
            #endif
            if(arrarSize > TARIFF_WEEK_NUM)
                arrarSize = TARIFF_WEEK_NUM;
            for(i = 0; i < arrarSize; i++)
            {                       
                tmp_json2 = cJSON_GetArrayItem(tmp_json, i);    
                if(tmp_json2 != NULL)
                {                                
                    jsonTariff[index].week[i] = tmp_json2->valueint;
                    #if(ENABLE_LOAD_JSON_DEBUG)
                    sysprintf("     >> jsonTariff[index].week[%d]:%d\n", i, jsonTariff[index].week[i]);
                    #endif
                }
                else
                {
                    sysprintf("error(week array):%s\n", cJSON_GetErrorPtr());   
                    errorCode = 6;                                    
                    goto jsonExit;
                }
            }
        }
        else
        {
            sysprintf("error(week):%s\n", cJSON_GetErrorPtr()); 
            errorCode = 7;                            
            goto jsonExit;
        }
        
        //day type
        tmp_json = cJSON_GetObjectItem(root_json, "daytype");
        if (tmp_json != NULL)
        {
            int i, j;
            int arrarSize = cJSON_GetArraySize(tmp_json);
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].daytype: arrar size = %d\n", arrarSize);
            #endif
            if(arrarSize > TARIFF_DAY_TYPE_NUM)
                arrarSize = TARIFF_DAY_TYPE_NUM;
            for(i = 0; i < arrarSize; i++)
            {                       
                tmp_json2 = cJSON_GetArrayItem(tmp_json, i);   
                if (tmp_json2 != NULL)
                {
                    int arrarSize = cJSON_GetArraySize(tmp_json2);
                    #if(ENABLE_LOAD_JSON_DEBUG)
                    sysprintf("     >> jsonTariff[index].daytypeitem: arrar size = %d\n", arrarSize);
                    #endif
                    if(arrarSize > TARIFF_DAY_TYPE_ITEM_NUM)
                        arrarSize = TARIFF_DAY_TYPE_ITEM_NUM;
                    for(j = 0; j < arrarSize; j++)
                    {                       
                        tmp_json3 = cJSON_GetArrayItem(tmp_json2, j);    
                        if(tmp_json3 != NULL)
                        {
                            tmp_json4 = cJSON_GetObjectItem(tmp_json3, "tariffsetting");
                            if (tmp_json4 != NULL)
                            {
                                jsonTariff[index].dayType[i].dayTypeItem[j].tariffsetting = tmp_json4->valueint;
                                #if(ENABLE_LOAD_JSON_DEBUG)
                                sysprintf("         >> jsonTariff[index].dayType[%d].dayTypeItem[%d].tariffsetting: %d\n", i, j, jsonTariff[index].dayType[i].dayTypeItem[j].tariffsetting);
                                #endif
                            }
                            else
                            {
                                sysprintf("error(tariffsetting):%s\n", cJSON_GetErrorPtr());   
                                errorCode = 8;                                                
                                goto jsonExit;
                            }
                            
                            tmp_json4 = cJSON_GetObjectItem(tmp_json3, "hour");
                            if (tmp_json4 != NULL)
                            {
                                jsonTariff[index].dayType[i].dayTypeItem[j].hour = tmp_json4->valueint;
                                #if(ENABLE_LOAD_JSON_DEBUG)
                                sysprintf("         >> jsonTariff[index].dayType[%d].dayTypeItem[%d].hour: %d\n", i, j, jsonTariff[index].dayType[i].dayTypeItem[j].hour);
                                #endif
                            }
                            else
                            {
                                sysprintf("error(hour):%s\n", cJSON_GetErrorPtr());  
                                errorCode = 9;                                                
                                goto jsonExit;
                            }
                            
                            tmp_json4 = cJSON_GetObjectItem(tmp_json3, "minute");
                            if (tmp_json4 != NULL)
                            {
                                jsonTariff[index].dayType[i].dayTypeItem[j].minute = tmp_json4->valueint;
                                #if(ENABLE_LOAD_JSON_DEBUG)
                                sysprintf("         >> jsonTariff[index].dayType[%d].dayTypeItem[%d].minute: %d\n", i, j, jsonTariff[index].dayType[i].dayTypeItem[j].minute);
                                #endif
                            }   
                            else
                            {
                                sysprintf("error(minute):%s\n", cJSON_GetErrorPtr());  
                                errorCode = 10;                                             
                                goto jsonExit;
                            }
        
                            tmp_json4 = cJSON_GetObjectItem(tmp_json3, "continued");
                            if (tmp_json4 != NULL)
                            {
                                if(tmp_json4->type == cJSON_True)
                                {
                                    jsonTariff[index].dayType[i].dayTypeItem[j].continued = TRUE;
                                    #if(ENABLE_LOAD_JSON_DEBUG)
                                    sysprintf("         >> jsonTariff[index].dayType[%d].dayTypeItem[%d].continued: %d (TRUE)\n", i, j, jsonTariff[index].dayType[i].dayTypeItem[j].continued);
                                    #endif
                                }
                                else if(tmp_json4->type == cJSON_False)
                                {
                                    jsonTariff[index].dayType[i].dayTypeItem[j].continued = FALSE;
                                    #if(ENABLE_LOAD_JSON_DEBUG)
                                    sysprintf("         >> jsonTariff[index].dayType[%d].dayTypeItem[%d].continued: %d (FALSE)\n", i, j, jsonTariff[index].dayType[i].dayTypeItem[j].continued);
                                    #endif
                                }

                            }  
                            else
                            {
                                sysprintf("error(continued):%s\n", cJSON_GetErrorPtr());   
                                errorCode = 11;                                             
                                goto jsonExit;
                            }                                        
                            
                        }
                        else
                        {
                            sysprintf("error(tariffsetting array 2):%s\n", cJSON_GetErrorPtr());     
                            errorCode = 12;                                         
                            goto jsonExit;
                        }
                    }
                }  
                else
                {
                    sysprintf("error(tariffsetting array 1):%s\n", cJSON_GetErrorPtr());  
                    errorCode = 13;                                 
                    goto jsonExit;
                }                            
            }
            
        }
        else
        {
            sysprintf("error(daytype):%s\n", cJSON_GetErrorPtr());  
            errorCode = 14;                         
            goto jsonExit;
        }
        
        //tariff type
        tmp_json = cJSON_GetObjectItem(root_json, "tariffsetting");
        if (tmp_json != NULL)
        {
            int i;
            int arrarSize = cJSON_GetArraySize(tmp_json);
            #if(ENABLE_LOAD_JSON_DEBUG)
            sysprintf(" >> jsonTariff[index].tariffsetting: arrar size = %d\n", arrarSize);
            #endif
            if(arrarSize > TARIFF_TARIFF_SETTING_NUM)
                arrarSize = TARIFF_TARIFF_SETTING_NUM;
            for(i = 0; i < arrarSize; i++)
            {                       
                tmp_json2 = cJSON_GetArrayItem(tmp_json, i);   
                if (tmp_json2 != NULL)
                {
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "type");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].type = tmp_json3->valueint;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].type: %d\n", i, jsonTariff[index].tariffSetting[i].type);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(type):%s\n", cJSON_GetErrorPtr());  
                        errorCode = 15;                                    
                        goto jsonExit;
                    }
                    
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "cost");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].cost = tmp_json3->valueint;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].cost: %d\n", i, jsonTariff[index].tariffSetting[i].cost);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(cost):%s\n", cJSON_GetErrorPtr());  
                        errorCode = 16;   
                        goto jsonExit;
                    }
                    
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "timeunit");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].timeunit = tmp_json3->valueint * 60;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].timeunit: %d\n", i, jsonTariff[index].tariffSetting[i].timeunit);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(timeunit):%s\n", cJSON_GetErrorPtr());   
                        errorCode = 17;   
                        goto jsonExit;
                    }
                    
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "basecost");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].basecost = tmp_json3->valueint;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].basecost: %d\n", i, jsonTariff[index].tariffSetting[i].basecost);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(basecost):%s\n", cJSON_GetErrorPtr());   
                        errorCode = 18;   
                        goto jsonExit;
                    }
                    
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "costunit");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].costunit = tmp_json3->valueint;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].costunit: %d\n", i, jsonTariff[index].tariffSetting[i].costunit);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(costunit):%s\n", cJSON_GetErrorPtr());   
                        errorCode = 19;                                       
                        goto jsonExit;
                    }
                    
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "maxcost");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].maxcost = tmp_json3->valueint;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].maxcost: %d\n", i, jsonTariff[index].tariffSetting[i].maxcost);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(maxcost):%s\n", cJSON_GetErrorPtr());   
                        errorCode = 20;                                     
                        goto jsonExit;
                    }
                    
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "maxtime");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].maxtime = tmp_json3->valueint*60;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].maxtime: %d\n", i, jsonTariff[index].tariffSetting[i].maxtime);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(maxtime):%s\n", cJSON_GetErrorPtr());   
                        errorCode = 21;                                     
                        goto jsonExit;
                    }
                    
                    tmp_json3 = cJSON_GetObjectItem(tmp_json2, "continuedtime");
                    if (tmp_json3 != NULL)
                    {
                        jsonTariff[index].tariffSetting[i].continuedtime = tmp_json3->valueint*60;
                        #if(ENABLE_LOAD_JSON_DEBUG)
                        sysprintf("     >> jsonTariff[index].tariffSetting[%d].continuedtime: %d\n", i, jsonTariff[index].tariffSetting[i].continuedtime);
                        #endif
                    }
                    else
                    {
                        sysprintf("error(continuedtime):%s\n", cJSON_GetErrorPtr());   
                        errorCode = 22;                                     
                        goto jsonExit;
                    }                    
                }                            
            }
        }     
        else
        {
            sysprintf("error(tariffsetting):%s\n", cJSON_GetErrorPtr());   
            errorCode = 23;                         
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

    if(root_json != NULL)
        cJSON_Delete(root_json);
   
    sysprintf("     load loadJsonTariff ERROR (errorCode = %d)>>\r\n", errorCode); 
    //{
    //    char str[256];
    //    sprintf(str, "!!ERROR!! loadJsonTariff ERROR (errorCode = %d)\r\n", errorCode); 
    //    LoglibPrintf(LOG_TYPE_ERROR, str);
    //}
    return FALSE;

}

static BOOL swInit(void)
{   
    if(TariffLoadTariffFile() == FALSE)
        return FALSE; 
    return TariffUpdateCurrentTariffData();
}

static BOOL applyData(void)
{ 
    
    return TRUE;
}
    
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL TariffLibInit(void)
{
    sysprintf("TariffLibInit!!\n");
    
    if(swInit() == FALSE)
    {
        sysprintf("TariffLibInit ERROR (swInit false)!!\n");
        
        return FALSE;
    }
    if(applyData() == FALSE)
    {
        sysprintf("TariffLibInit ERROR (applyData false)!!\n");
        return FALSE;
    }
    
    sysprintf("TariffLibInit OK!!\n");
    return TRUE;
}
BOOL TariffLoadTariffFile(void)
{//有檔案改變時 重新載入到MEMORY
    int i;
    sysprintf("  !!! TariffLoadTariffFile !!!\n");
    if(loadTariffFileName() == TRUE)
    {
        for(i = 0; i<TARIFF_FILE_NUM; i++)
        {
            //sysprintf(" -->> INFO[%d] >> Tariff File Name[%d] [%s] \r\n", i, 0, tariffFileName[0]);
            //sysprintf(" -->> INFO >> Tariff File Name[%d] [%s] \r\n", 1, tariffFileName[1]);
            //sysprintf(" -->> INFO >> Tariff File Name[%d] [%s] \r\n", 2, tariffFileName[2]);
            tariffEffectiveDate[i] = 20991231;
            if(strlen(tariffFileName[i]) != 0)
            {
                if(loadJsonTariff(i, tariffFileName[i]) == FALSE)
                {
                    return FALSE;
                }
                else
                {
                    tariffEffectiveDate[i] = jsonTariff[i].effectivetime;
                    printJsonTariff(i);
                }
            }
            else
            {
                sysprintf("  !!! TariffLoadTariffFile [%d] ignore (%s)!!!\n", i, tariffFileName[i]);
            }
        }   
        sortTariffFile();
    }
    else
    {
        //GuiManagerShowScreen(GUI_FILE_DOWNLOAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
    }
    
    //if(TariffUpdateCurrentTariffData() == FALSE)
    //    return FALSE;  
    return TRUE;    
}

BOOL TariffUpdateCurrentTariffData(void)
{
    if(selectTariffFile() == FALSE)
        return FALSE;
    return updateCurrentTariffData(currentTariffFileIndex);
}

JsonTariffSetting* TariffGetCurrentTariffType(void)
{
    return currentJsonTariffType;
}
char* TariffGetFileName()
{
    return jsonTariff[currentTariffFileIndex].name;
}

BOOL TariffGetNextWakeupTime(UINT32* wakeUpTime, RTC_TIME_DATA_T* sCurTime)
{
    *wakeUpTime = 0;
    /* Get the currnet time */
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, sCurTime))
    {
        #if(BUILD_RELEASE_VERSION || BUILD_PRE_RELEASE_VERSION)
        *wakeUpTime = 60 - sCurTime->u32cSecond;
        if(*wakeUpTime < 5)
            *wakeUpTime = *wakeUpTime + 60;
        #else
        *wakeUpTime = 5;
        #endif
        return TRUE;
    }
    return FALSE;
}



/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


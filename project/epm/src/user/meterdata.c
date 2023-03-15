/**************************************************************************//**
* @file     meterdata.c
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
#include "meterdata.h"
#include "interface.h"
#include "paralib.h"
#include "epddrv.h"
#include "timelib.h"
#include "spacedrv.h"
#include "leddrv.h"
#include "buzzerdrv.h"
#include "batterydrv.h"
#include "cmdlib.h"
#include "guimanager.h"
#include "pct08cmdlib.h"
#include "pct08drv.h"
#include "photoagent.h"
#include "ff.h"
#include "osmisc.h"
#include "tarifflib.h"
#include "loglib.h"
#include "modemagent.h"
#include "cjson.h"
#include "guidrv.h"
#include "powerdrv.h"
#include "jsoncmdlib.h"
#include "fileagent.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_TARIFF_INTERVAL_SECONDS      59


#if(BUILD_RELEASE_VERSION)
    #define UPDATE_LED_HEARTBEAT_INTERVAL_SECONDS           (LED_HEARTBEAT_SECONDS - 90)
    #define MODEM_AGENT_ROUTINE_INTERVAL_SECONDS            (60*60*2)
    #define TAKE_PHOTO_INTERVAL_SECONDS                     (60*60*1)
#else    
    #if(BUILD_PRE_RELEASE_VERSION)
        #define UPDATE_LED_HEARTBEAT_INTERVAL_SECONDS   (LED_HEARTBEAT_SECONDS - 90)
        #define MODEM_AGENT_ROUTINE_INTERVAL_SECONDS            (60*30)
        #define TAKE_PHOTO_INTERVAL_SECONDS                     (60*15-5)
        #define TAKE_TRANSACTION_INTERVAL_SECONDS               (60*10-5)
    #else
        #define UPDATE_LED_HEARTBEAT_INTERVAL_SECONDS   (LED_HEARTBEAT_SECONDS - 15)
        #define MODEM_AGENT_ROUTINE_INTERVAL_SECONDS            (60*15)
        #define TAKE_PHOTO_INTERVAL_SECONDS                     (60*10-5)//(60*5-5)
        #define TAKE_TRANSACTION_INTERVAL_SECONDS               (60*5-5)
    #endif
#endif




/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static MeterData meterData;

static uint8_t bayColorLocal[6];

typedef BOOL(*MeterDataRoutineFunc)(time_t time);

typedef BOOL(*MeterDataRoutineConditionFunc)(time_t time);

typedef struct
{
    char*                       routineName;
    MeterDataRoutineFunc        routineFunc;
    MeterDataRoutineConditionFunc   conditionFunc;
    time_t                      processTimeInterval;
    time_t                      lastProcessTime;
}MeterDataRoutineItem;

static BOOL updateTariffDataRoutine(time_t currentTime);
static BOOL updateTariffDataRoutineCondition(time_t currentTime);
static BOOL sendLedHeartbeatRoutine(time_t currentTime);
static BOOL takePhotoRoutine(time_t currentTime);
static BOOL takeTransactionRoutine(time_t currentTime);

static BOOL modemAgentTransmitRoutine(time_t currentTime);
static MeterDataRoutineItem routineItem[] = {{" * update tariff data * ",   updateTariffDataRoutine,    updateTariffDataRoutineCondition,   UPDATE_TARIFF_INTERVAL_SECONDS,         0},
                                            {" * update led heartbeat * ",  sendLedHeartbeatRoutine,    NULL,                               UPDATE_LED_HEARTBEAT_INTERVAL_SECONDS,  0},
                                            //just test
                                            {" * take photo * ",            takePhotoRoutine,           NULL,                               TAKE_PHOTO_INTERVAL_SECONDS,            0},  
                                            #if(BUILD_RELEASE_VERSION)
                                            #else
                                            {" * transaction * ",           takeTransactionRoutine,     NULL,                               TAKE_TRANSACTION_INTERVAL_SECONDS,      0},
                                            #endif
                                            //
                                            #if(ENABLE_MODEM_AGENT_DRIVER)
                                            {" * modem agent * ",           modemAgentTransmitRoutine,  NULL,                               MODEM_AGENT_ROUTINE_INTERVAL_SECONDS,   0},
                                            #endif
                                            {NULL,                          NULL,                       NULL,                               0,                                      0}
                                            };

static SemaphoreHandle_t xMeterDataActionSemaphore;                                            
                                     
static BOOL meterDataPowerStatus = TRUE;

static BOOL meterDataIgnoreRun = FALSE;

static BOOL MeterDataCheckStatus(int flag);
static BOOL MeterDataPreOffCallback(int flag);
static BOOL MeterDataOffCallback(int flag);
static BOOL MeterDataOnCallback(int flag);
static powerCallbackFunc meterDataPowerCallabck = {" [MeterData] ", MeterDataPreOffCallback, MeterDataOffCallback, MeterDataOnCallback, MeterDataCheckStatus};

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL MeterDataPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    meterDataIgnoreRun = TRUE;
    //sysprintf("### MeterData OFF Callback [%s] ###\r\n", meterDataPowerCallabck.drvName);    
    return reVal;    
}
static BOOL MeterDataOffCallback(int flag)
{
    int timers = 2000/10;
    while(!meterDataPowerStatus)
    {
        sysprintf("[m]");
        if(timers-- == 0)
        {
            return FALSE;
        }
        vTaskDelay(10/portTICK_RATE_MS); 
    }
    return TRUE;    
}
static BOOL MeterDataOnCallback(int flag)
{
    BOOL reVal = TRUE;
    meterDataPowerStatus = FALSE;
    meterDataIgnoreRun = FALSE;
    xSemaphoreGive(xMeterDataActionSemaphore);
    //sysprintf("### MeterData ON Callback [%s] ###\r\n", meterDataPowerCallabck.drvName);    
    return reVal;    
}
static BOOL MeterDataCheckStatus(int flag)
{
    //BOOL reVal = TRUE;
    //sysprintf("### MeterData STATUS Callback [%s] ###\r\n", meterDataPowerCallabck.drvName); 
    return meterDataPowerStatus;    
}


static BOOL getSpaceStatus(uint8_t index)
{
    if(GetMeterPara()->meterPosition-1 == index)
    {//前一個
        return GetSpaceStatus(SPACE_INDEX_1);
    }
    else if(GetMeterPara()->meterPosition == index)
    {//後一個
        return GetSpaceStatus(SPACE_INDEX_2);
    }
    else
    {
        return FALSE;
    }
    
}

static BOOL updateTariffDataRoutine(time_t currentTime)
{
    TariffUpdateCurrentTariffData(); 
    return TRUE;
}
static BOOL updateTariffDataRoutineCondition(time_t currentTime)
{
    if(GuiManagerCompareCurrentScreenId(GUI_STANDBY_ID) || 
            GuiManagerCompareCurrentScreenId(GUI_FILE_DOWNLOAD_ID) || 
            GuiManagerCompareCurrentScreenId(GUI_FREE_ID) || 
            GuiManagerCompareCurrentScreenId(GUI_OFF_ID))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
static BOOL sendLedHeartbeatRoutine(time_t currentTime)
{
    LedSendHeartbeat(NULL); 
    return TRUE;
}

static BOOL takePhotoRoutine(time_t currentTime)
{
    #warning just test
    PhotoAgentStartTakePhoto(0);
    return TRUE;
}



static BOOL takeTransactionRoutine(time_t currentTime)
{
    #warning just test
    RTC_TIME_DATA_T pt;
    cJSON *extend_json;
    char* outUnformatted;
    time_t currentTimeTmp;
    DataProcessSendData(currentTime, GetMeterData()->currentSelSpace, GetMeterData()->currentSelTime,GetMeterData()->currentSelCost, currentTime, DATA_TYPE_ID_TRANSACTION, "transaction");

    Time2RTC(currentTime, &pt);    
    currentTimeTmp = pt.u32cDay*1000000 + pt.u32cHour*10000 + pt.u32cMinute*100 + pt.u32cSecond;
    
    extend_json = JsonCmdCreateTransactionStatusData(GetMeterData()->currentSelSpace, GetMeterData()->currentSelTime,GetMeterData()->currentSelCost, currentTimeTmp);
    if(extend_json != NULL)
    {
        RTC_TIME_DATA_T pt;
        char targetDSFFileName[_MAX_LFN];
        char* targetDCFFileName = MeterGetCurrentDCFFileName(&pt);
        //sprintf(targetDSFFileName,"%08d_%010d.%s", GetMeterData()->epmid, (int)currentTime, DSF_FILE_EXTENSION); 
        sprintf(targetDSFFileName,"%08d_%04d%02d%02d%02d%02d.%s", GetMeterData()->epmid, pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, DSF_FILE_EXTENSION); 
        outUnformatted = cJSON_PrintUnformatted(extend_json); 
        cJSON_Delete(extend_json);
        if(FileAgentAddData(DSF_FILE_SAVE_POSITION, DSF_FILE_DIR, targetDSFFileName, (uint8_t*)outUnformatted, strlen(outUnformatted), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, TRUE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
        {
            #if(ENABLE_MODEM_AGENT_DRIVER)
            #else
            FileAgentDelFile(DSF_FILE_SAVE_POSITION, DSF_FILE_DIR, targetDSFFileName);
            #endif
            //return TRUE;           
        }
        if(FileAgentAddData(DCF_FILE_SAVE_POSITION, DCF_FILE_DIR, targetDCFFileName, (uint8_t*)outUnformatted, strlen(outUnformatted), FILE_AGENT_ADD_DATA_TYPE_APPEND, TRUE, FALSE, FALSE) !=  FILE_AGENT_RETURN_ERROR )
        {
            #if(ENABLE_MODEM_AGENT_DRIVER)
            #else
            FileAgentDelFile(DCF_FILE_SAVE_POSITION, DCF_FILE_DIR, targetDCFFileName);
            #endif
            //return TRUE;           
        }
    }   
    PhotoAgentStartTakePhoto(currentTime); 
    return TRUE;
}

static BOOL modemAgentTransmitRoutine(time_t currentTime)
{
    DataProcessSendStatusData(currentTime, "Routine");
    
    ModemAgentStartSend(DATA_PROCESS_ID_ESF);
    ModemAgentStartSend(DATA_PROCESS_ID_LOG);//Log順便送上去 
    ModemAgentStartSend(DATA_PROCESS_ID_PHOTO);  

    ModemAgentStartSend(DATA_PROCESS_ID_DCF);  
    ModemAgentStartSend(DATA_PROCESS_ID_DSF);      

    ModemAgentStartSend(DATA_PROCESS_ID_TRE);  
    ModemAgentStartSend(DATA_PROCESS_ID_PARA);  
    return TRUE;
}

static void routineCounterTimer(void)
{
    time_t rawTime = GetCurrentUTCTime();
    if(rawTime != 0)
    {
        int i;
        BOOL runFlag;
        for (i = 0; ; i++)
        {
            if(routineItem[i].routineName == NULL)
                break;
            runFlag = FALSE;
            if(routineItem[i].conditionFunc != 0)
            {
                if(routineItem[i].conditionFunc(rawTime))
                {
                    runFlag = TRUE;
                }
            }
            else
            {
                runFlag = TRUE;
            }
            if(runFlag)
            {
                if(((rawTime - routineItem[i].lastProcessTime) > routineItem[i].processTimeInterval) || (rawTime < routineItem[i].lastProcessTime))
                {
                    sysprintf(" --- INFO ----> routineCounterTimer(%d) <%s> (lastProcessTime = %d) start ...\r\n", rawTime, routineItem[i].routineName, routineItem[i].lastProcessTime); 
                    routineItem[i].routineFunc(rawTime);  
                    sysprintf(" --- INFO ----> routineCounterTimer(%d) <%s> (lastProcessTime = %d) end ...\r\n", rawTime, routineItem[i].routineName, routineItem[i].lastProcessTime); 
                    routineItem[i].lastProcessTime = rawTime;
                }
            }
        }
    }
}


int FlashDrvExGetErrorTimes(void);

static void vMeterDataProcessTask( void *pvParameters )
{
    vTaskDelay(2000/portTICK_RATE_MS); 
    sysprintf("vMeterDataProcessTask Going...\r\n");     
    
    for(;;)
    {         
        meterDataPowerStatus = TRUE;
        BaseType_t reval = xSemaphoreTake(xMeterDataActionSemaphore, 1000/portTICK_RATE_MS); 
        if(meterDataIgnoreRun)
        {
            sysprintf("vMeterDataProcessTask ignore run...\r\n");
            continue;
        }
        meterDataPowerStatus = FALSE;

        routineCounterTimer();

        AutoUpdateMeterData(); 
        
        //sysprintf("    >> == (Tariff:[%s] (type = %d), Para:[%s], FormatCounter = %08d, FlashError = %08d) == <<\r\n", 
        //            TariffGetFileName(), TariffGetCurrentTariffType()->type, GetMeterPara()->name, FileAgentGetFatFsAutoFormatCounter(), FlashDrvExGetErrorTimes());  
#if(ENABLE_MODEM_AGENT_DRIVER)
        if((strlen(TariffGetFileName()) == 0) || (strlen(GetMeterPara()->name) == 0))
        {
            if(GuiManagerShowScreen(GUI_FILE_DOWNLOAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0) == GUI_SHOW_SCREEN_OK)
            {
                DataProcessSendStatusData(0, "FileDownload");
        
                ModemAgentStartSend(DATA_PROCESS_ID_ESF);
                ModemAgentStartSend(DATA_PROCESS_ID_TRE);  
                ModemAgentStartSend(DATA_PROCESS_ID_PARA); 
            }
        }
        else
#endif
        {
            //sysprintf("vMeterDataProcessTask Going (TariffGetFileName() = %s, type = %d)...\r\n", TariffGetFileName(), TariffGetCurrentTariffType()->type);  
            switch(TariffGetCurrentTariffType()->type)
            {
                case TARIFF_TARIFF_TYPE_FREE:              
                    if(GuiManagerCompareCurrentScreenId(GUI_FREE_ID) != TRUE)                    
                        GuiManagerShowScreen(GUI_FREE_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                    break;
                case TARIFF_TARIFF_TYPE_OFF:
                    if(GuiManagerCompareCurrentScreenId(GUI_OFF_ID) != TRUE)   
                        GuiManagerShowScreen(GUI_OFF_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                    break;
                case TARIFF_TARIFF_TYPE_LINEAR:
                    if(GuiManagerCompareCurrentScreenId(GUI_FREE_ID) || GuiManagerCompareCurrentScreenId(GUI_OFF_ID))
                    {
                        GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                    }
                    break;
            }
                
        }      
        
        
    }
   
}

static BOOL getEpmID(void)
{
    sysprintf("   -- getEpmID --\r\n"); 
#if(EPM_ID_FROM_JSON_FILE)
    uint8_t* dataTmp;
    size_t dataTmpLen;
    BOOL needFree;
    FileAgentReturn reVal; 
    
     //__SHOW_FREE_HEAP_SIZE__
    
    reVal = FileAgentGetData(EPM_ID_SAVE_POSITION, EPM_ID_FILE_DIR, EPM_ID_FILE_NAME, &dataTmp, &dataTmpLen, &needFree, FALSE);
    sysprintf("   -- getEpmID(len = %d) --[%s]\r\n", dataTmpLen, dataTmp); 
    if(reVal != FILE_AGENT_RETURN_ERROR)
    {
        cJSON *root_json = cJSON_Parse((char*)dataTmp);
        cJSON *tmp_json;
        if (NULL == root_json)
        {
            sysprintf("error((NULL == root_json)):%s\n", cJSON_GetErrorPtr());                        
            goto jsonExit;
        }
        
        
        tmp_json = cJSON_GetObjectItem(root_json, "epmid");
        if (tmp_json != NULL)
        {
            //sysprintf(" >> 0x%04x, 0x%04x\n", (tmp_json->valueint), (uint16_t)(tmp_json->valueint));
            meterData.epmid = tmp_json->valueint;
            sysprintf(" >> meterPara.epmid:%d\n", meterData.epmid);
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
    if(needFree)
    {
        vPortFree(dataTmp);
    }
    //__SHOW_FREE_HEAP_SIZE__ 
    sysprintf("   -- getEpmID ERROR--\r\n");
    return FALSE;

#else
    //GPJ 0 ~ 3
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xFFFF<<0)) | (0x0000u<<0));
    UINT32 portValue = GPIO_ReadPort(GPIOJ);
    meterData.epmid = portValue&0xF;
    sysprintf(" >> meterPara.epmid:%d (from DIP Setting)\n", meterData.epmid);
    return TRUE;
#endif
}



static BOOL swInit(void)
{   
    int i;
    PowerRegCallback(&meterDataPowerCallabck);
    
    meterData.currentSelSpace = 0;
    meterData.currentSelTime = 0;
    meterData.currentSelCost = 0;
    meterData.runningStatus = RUNNING_STATUS_INIT;
    //meterData.meterErrorCode = 0;///不能初始化 需靠原來的 0
    for(i = 0; i<EPM_TOTAL_METER_SPACE_NUM; i++)
    {
        meterData.spaceSepositStatus[i] = SPACE_DEPOSIT_STATUS_INIT; 
    }
    
    
    if(getEpmID() == FALSE)
    {
        return FALSE;
    }
    if(TariffLibInit() == FALSE)
    {
        GetMeterData()->runningStatus = RUNNING_STATUS_FILE_DOWNLOAD;
        sysprintf("MeterDataInit (swInit) set GetMeterData()->runningStatus = %d!!\n", GetMeterData()->runningStatus);
        //return FALSE;
    }
    
    sprintf(meterData.buildStr, "%d.%d.%d(%d)", MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, meterData.buildVer);
    sprintf(meterData.epmIdStr, "%08d", meterData.epmid);
    strcpy(meterData.tariffFileFTPPath, DEFAULT_TARIFF_FILE_FTP_PATH);
    strcpy(meterData.paraFileFTPPath, DEFAULT_PARA_FILE_FTP_PATH);
    strcpy(meterData.dcfFileFTPPath, DEFAULT_DCF_FILE_FTP_PATH);
    strcpy(meterData.dsfFileFTPPath, DEFAULT_DSF_FILE_FTP_PATH);
    strcpy(meterData.jpgFileFTPPath, DEFAULT_JPG_FILE_FTP_PATH);
    strcpy(meterData.logFileFTPPath, DEFAULT_LOG_FILE_FTP_PATH);

    
    calculateDeviceIDPositionInfo(meterData.epmid);
    
    AutoUpdateMeterData(); 
    
    xMeterDataActionSemaphore = xSemaphoreCreateBinary();       
    xTaskCreate( vMeterDataProcessTask, "vMeterDataProcessTask", 1024*10, NULL, METER_DATA_PROCESS_THREAD_PROI, NULL );
    
    return TRUE;
}

static BOOL applyData(void)
{ 
    
    return TRUE;
}
    
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL MeterDataInit(void)
{
    sysprintf("MeterDataInit!!\n");
    if(swInit() == FALSE)
    {
        sysprintf("MeterDataInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    if(applyData() == FALSE)
    {
        sysprintf("MeterDataInit ERROR (applyData false)!!\n");
        return FALSE;
    }
    
    sysprintf("MeterDataInit OK!!\n");
    return TRUE;
}

MeterData* GetMeterData(void)
{
    return &meterData;
}
BOOL UpdateMeterDepositTime(time_t time)
{
    uint16_t    currentSelCostTmp = TariffGetCurrentTariffType()->basecost +
                            ((time - TariffGetCurrentTariffType()->timeunit)/TariffGetCurrentTariffType()->timeunit/*SEL_TIME_INTERVAL_TIME*/) * TariffGetCurrentTariffType()->costunit; 
    sysprintf("UpdateMeterDepositTime (currentSelCostTmp = %d, maxcost = %d) !!\n", currentSelCostTmp, TariffGetCurrentTariffType()->maxcost);
    if(currentSelCostTmp <=  TariffGetCurrentTariffType()->maxcost)
    {        
        GetMeterData()->currentSelTime = time;
        GetMeterData()->currentSelCost = currentSelCostTmp; 
        return TRUE;
    }
    return FALSE;
}

void UpdateMeterCurrentSelSpace(int value, BOOL setFlag)
{
    if(setFlag)
    {
        GetMeterData()->currentSelSpace = value;
    }
    else
    {
        GetMeterData()->currentSelSpace = GetMeterData()->currentSelSpace + value;
    }
    if(GetMeterData()->currentSelSpace > GetMeterPara()->spaceEnableNum)
        GetMeterData()->currentSelSpace = 1;

    if(GetMeterData()->currentSelSpace == 0)
        GetMeterData()->currentSelSpace = GetMeterPara()->spaceEnableNum;
}


void AutoUpdateMeterData(void)
{
    int i;
    #if(0)
    sysprintf(" [INFO DATA] <AutoUpdateMeterData>  Start!!!.\n");    
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    BOOL beepFlag = FALSE;
    BOOL takePictFlag = FALSE;
    BOOL refreshFlag = FALSE;
    RTC_TIME_DATA_T pt;

    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        time_t rawTime = RTC2Time(&pt);
        time_t timeTmp;
        for(i = 0; i<GetMeterPara()->spaceEnableNum; i++)
        {
            uint8_t previousStatus = meterData.spaceSepositStatus[i];
            if(GetMeterStorageData()->depositEndTime[i] > rawTime)
            {
                timeTmp = GetMeterStorageData()->depositEndTime[i] - rawTime;
                SetDepositTimeBmpIdInfo(i, timeTmp + 59); //為了顯示 不要少一分鐘
                meterData.spaceSepositStatus[i] = SPACE_DEPOSIT_STATUS_OK;
                bayColorLocal[i] = LIGHT_COLOR_GREEN;
                if(timeTmp < 3)
                {
                    beepFlag = TRUE;
                }
            }
            else
            {
                GetMeterStorageData()->depositStartTime[i] = 0;
                GetMeterStorageData()->depositEndTime[i] = 0;
                SetDepositTimeBmpIdInfo(i, 0); //為了顯示 不要少一分鐘
                
                if(getSpaceStatus(i))
                {
                    
                    meterData.spaceSepositStatus[i] = SPACE_DEPOSIT_STATUS_EXCEED_TIME;
                    bayColorLocal[i] = LIGHT_COLOR_RED;
                    
                    if(previousStatus != SPACE_DEPOSIT_STATUS_EXCEED_TIME)//just send one time
                    {
                        //CmdSendExceedTime(i + 1, rawTime, GetMeterStorageData()->depositEndTime[i]); //spaceId 需要+1
                        DataProcessSendData(rawTime, i + 1, 0, 0, 0, DATA_TYPE_ID_EXPIRED, "expired");
                        ModemAgentStartSend(DATA_PROCESS_ID_ESF); 
                        takePictFlag = TRUE;
                        {
                        char str[256];
                        sprintf(str, "  - expired - %d: bayid= %d...\r\n", (int)rawTime, i + 1);
                        LoglibPrintf(LOG_TYPE_INFO, str);
                        }
                    }
                }
                else
                {
                    meterData.spaceSepositStatus[i] = SPACE_DEPOSIT_STATUS_STANBY;
                    bayColorLocal[i] = LIGHT_COLOR_RED;
                }
            }  
            if(previousStatus != SPACE_DEPOSIT_STATUS_INIT)
            {
                if(previousStatus != meterData.spaceSepositStatus[i])
                {
                    refreshFlag = TRUE;                    
                }
            }
            //sysprintf(" [INFO DATA] <spaceSepositStatus>  [%d] : %d.\n", i, meterData.spaceSepositStatus[i]);  
        }
        
        if(refreshFlag)
        {
            GuiManagerUpdateScreen();
            //CmdSendVersion(0xffff);
        }
        
        if(takePictFlag)
        {
            PhotoAgentStartTakePhoto(rawTime);
            ModemAgentStartSend(DATA_PROCESS_ID_PHOTO);
        }
    }
    
    if(beepFlag)
    {
        BuzzerPlay(30, 0, 1, TRUE);
    }
    
    LedSetColor(bayColorLocal, LIGHT_COLOR_IGNORE, TRUE);  
    //LedSetColor(bayColorLocal, LIGHT_COLOR_YELLOW, TRUE); 
   
    
    #if(0)
    sysprintf(" [INFO DATA] <AutoUpdateMeterData>  [%d].\n", xTaskGetTickCount() - tickLocalStart);    
    #endif
}



void MeterSetErrorCode(meterErrorCode code)
{
    sysprintf("\r\n *************    MeterSetErrorCode [code:0x%04x] !!!   *************\r\n", code);    
    meterData.meterErrorCode = meterData.meterErrorCode | code;
	//{
    //    char str[256];
    //    sprintf(str, "******* !!! MeterSetErrorCode [code:0x%04x, meterErrorCode:0x%04x] !!!   ******\r\n", code, meterData.meterErrorCode);
    //    LoglibPrintf(LOG_TYPE_ERROR, str);
    //}
}
void MeterUpdateExpiredTitle(uint8_t selectItemId)
{
    sysprintf("\r\n ****    MeterUpdateExpiredTitle selectItemId = %d [%d:%d, %d] !!!   ****\r\n", selectItemId, GetMeterData()->currentSelSpace, GetMeterPara()->meterPosition, GetMeterPara()->meterPosition+1);
    #warning 
    if(GetMeterPara()->spaceEnableNum != 2)
    {
        sysprintf("\r\n ****    MeterUpdateExpiredTitle ignore ****\r\n");
        return;
   
    }
    switch(GetMeterData()->spaceSepositStatus[GetMeterPara()->meterPosition - 1])
    {
        case SPACE_DEPOSIT_STATUS_INIT:

            break;
       case SPACE_DEPOSIT_STATUS_OK:
            if(selectItemId == 0)
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, TRUE, FALSE, FALSE);
            } 
            break;
        case SPACE_DEPOSIT_STATUS_EXCEED_TIME:
            if(selectItemId == 0)
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_W, TRUE, FALSE, FALSE);
            }            
            else if((GetMeterData()->currentSelSpace) == (GetMeterPara()->meterPosition))
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_B, TRUE, FALSE, FALSE);
            }
            else
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_W, TRUE, FALSE, FALSE);
            }
            break;
        case SPACE_DEPOSIT_STATUS_STANBY:
            if(selectItemId == 0)
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, TRUE, FALSE, FALSE);
            } 
            break;
    }
    switch(GetMeterData()->spaceSepositStatus[GetMeterPara()->meterPosition + 1 - 1])
    {
        case SPACE_DEPOSIT_STATUS_INIT:

            break;
         case SPACE_DEPOSIT_STATUS_OK:
            if(selectItemId == 0)
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, FALSE, TRUE, FALSE);
            } 
            break;
        case SPACE_DEPOSIT_STATUS_EXCEED_TIME:
            if(selectItemId == 0)
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_W, FALSE, TRUE, FALSE);
            } 
            else if((GetMeterData()->currentSelSpace) == (GetMeterPara()->meterPosition + 1))
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_B, FALSE, TRUE, FALSE);
            }
            else
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_W, FALSE, TRUE, FALSE);
            }
            break;
        case SPACE_DEPOSIT_STATUS_STANBY:
            if(selectItemId == 0)
            {
                ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, FALSE, TRUE, FALSE);
            } 
            break;
    }
}

void MeterSetBuildVer(uint32_t buildVer)
{
    GetMeterData()->buildVer = buildVer;
}
BOOL MeterUpdateLedHeartbeat(void)
{
    return sendLedHeartbeatRoutine(0);
}

void MeterUpdateModemAgentLastTime(void)
{
    time_t rawTime = GetCurrentUTCTime();
    if(rawTime != 0)
    {
        int i;
        for (i = 0; ; i++)
        {
            if(routineItem[i].routineName == NULL)
            {
                break;
            }
            if(routineItem[i].routineFunc == modemAgentTransmitRoutine)
            {
                routineItem[i].lastProcessTime = rawTime;
                break;
            }
        }
    }
}
static char currentDCFFileName[_MAX_LFN];
char*  MeterGetCurrentDCFFileName(RTC_TIME_DATA_T* pt)
{    
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, pt))
    {
        #if(BUILD_RELEASE_VERSION)
        sprintf(currentDCFFileName, "%08d_%04d%02d%02d%02d.%s", GetMeterData()->epmid, pt->u32Year, pt->u32cMonth, pt->u32cDay, pt->u32cHour, DCF_FILE_EXTENSION); 
        #else
        sprintf(currentDCFFileName, "%08d_%04d%02d%02d%02d%02d.%s", GetMeterData()->epmid, pt->u32Year, pt->u32cMonth, pt->u32cDay, pt->u32cHour, (pt->u32cMinute)/20, DCF_FILE_EXTENSION);  
        #endif
         
         
    }  
    return currentDCFFileName;
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


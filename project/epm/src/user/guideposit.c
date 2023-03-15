/**************************************************************************//**
* @file     guideposit.c
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
#include "guidrv.h"
#include "halinterface.h"
#include "guideposit.h"
#include "epddrv.h"
#include "guimanager.h"
#include "paralib.h"
#include "meterdata.h"
#include "cardreader.h"
#include "buzzerdrv.h"
#include "timelib.h"
#include "cardreader.h"
#include "cmdlib.h"
#include "photoagent.h"
#include "dataprocesslib.h"
#include "modemagent.h"
#include "loglib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER         GUI_TIME_0_INDEX
#define READER_PROCESS_TIMER      GUI_TIME_1_INDEX
#define EXIT_TIMER              GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL          portMAX_DELAY
#define READER_PROCESS_INTERVAL     (10/portTICK_RATE_MS)
#define EXIT_INTERVAL               (20000/portTICK_RATE_MS)
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <Deposit> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);   
    EPDDrawContainByID(TRUE, EPD_PICT_CONTAIN_DEPOSIT_INDEX); 
    sysprintf(" [INFO GUI] <Deposit> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);    
  
}
//static void setDepositResultStatusCallback(BOOL flag, uint16_t paraValue)
static void setDepositResultStatusCallback(BOOL flag, uint16_t returnInfo, uint16_t returnCode)
{    
    if(flag)
    {
        int targetIndex = GetMeterData()->currentSelSpace-1;
        time_t rawTimeTemp;
        sysprintf("\r\n  *** [INFO GUI] <Deposit> setDepositResultStatus: TRUE-> [Balance: %d]\r\n\r\n", paraValue);
        BuzzerPlay(100, 0, 1, TRUE);
        
        RTC_TIME_DATA_T pt;
        if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
        {
            rawTimeTemp = RTC2Time(&pt);
        }
        
        if(GetMeterStorageData()->depositEndTime[targetIndex] > rawTimeTemp)
        {
            //GetMeterStorageData()->depositStartTime[targetIndex]//依照原來start time
            //GetMeterStorageData()->depositEndTime[targetIndex] = GetMeterStorageData()->depositEndTime[targetIndex] + GetMeterData()->currentSelTime - 1/*預防server太快收到 顯示多一分鐘*/; 
            GetMeterStorageData()->depositEndTime[targetIndex] = GetMeterStorageData()->depositEndTime[targetIndex] + GetMeterData()->currentSelTime/*預防server太快收到 顯示多一分鐘*/; 
        }
        else
        {
            GetMeterStorageData()->depositStartTime[targetIndex] = rawTimeTemp;
            //GetMeterStorageData()->depositEndTime[targetIndex] = RTCAddTimeEx(GetMeterData()->currentSelTime  - 1/*預防server太快收到 顯示多一分鐘*/);
            GetMeterStorageData()->depositEndTime[targetIndex] = RTCAddTimeEx(GetMeterData()->currentSelTime/*預防server太快收到 顯示多一分鐘*/);
            
        }
        GuiManagerShowScreen(GUI_DEPOSIT_OK_ID, GUI_REDRAW_PARA_DONT_CARE, paraValue, 0);
        
        MeterStorageFlush();
        DataProcessSendData(rawTimeTemp, GetMeterData()->currentSelSpace, GetMeterData()->currentSelTime,GetMeterData()->currentSelCost, paraValue, DATA_TYPE_ID_TRANSACTION, "transaction");
        //CmdSendTransaction(GetMeterData()->currentSelSpace, rawTimeTemp, GetMeterStorageData()->depositEndTime[targetIndex],
        //                                                GetMeterData()->currentSelTime, GetMeterData()->currentSelCost, paraValue);
        ModemAgentStartSend(DATA_PROCESS_ID_ESF);
        
        PhotoAgentStartTakePhoto(rawTimeTemp); 
        ModemAgentStartSend(DATA_PROCESS_ID_PHOTO);
        {
        char str[512];
        sprintf(str, "  - transaction - %d: bayid= %d, time = %d, cost = %d, balance = %d...\r\n", (int)rawTimeTemp, GetMeterData()->currentSelSpace, GetMeterData()->currentSelTime, GetMeterData()->currentSelCost, paraValue);
        LoglibPrintf(LOG_TYPE_INFO, str);
        }
       
    }
    else
    {
        sysprintf("\r\n  *** [INFO GUI] <Deposit> setDepositResultStatus: FALSE-> [errorCode: %d]\r\n\r\n", paraValue);
        BuzzerPlay(50, 100, 2, TRUE);
        GuiManagerShowScreen(GUI_DEPOSIT_FAIL_ID, GUI_REDRAW_PARA_DONT_CARE, paraValue, 0);
    }
}

static void setCNResultCallback(BOOL flag, uint8_t* cn, int cnLen)
{
}
static void readerProcess(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();   
    sysprintf(".");
    //CardReaderProcess(GetMeterData()->currentSelCost/10, setDepositResultStatusCallback);
    CardReaderProcessCN(setCNResultCallback);
    //CardReaderProcess(GetMeterData()->currentSelCost, setDepositResultStatusCallback);
    //sysprintf(" [INFO GUI] <Deposit> updateData: [%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - keyStart);
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiDepositOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Deposit> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);  
    CardReaderStopInit();
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(READER_PROCESS_TIMER, READER_PROCESS_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL);     

    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    pGuiGetInterface->runTimeoutFunc(READER_PROCESS_TIMER);//更新畫面
    CardReaderSetPower(TRUE);
    //sysprintf(" [INFO GUI] <Deposit> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiDepositKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <Deposit> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <Deposit> Key:  ignore...\n"); 
        return reVal;
    }
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_LEFT_ID:
                UpdateMeterCurrentSelSpace(-1, FALSE);
                GuiManagerShowScreen(GUI_SEL_SPACE_ID, GUI_REDRAW_PARA_CONTAIN, 0, 0);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_RIGHT_ID:
                UpdateMeterCurrentSelSpace(1, FALSE);
                GuiManagerShowScreen(GUI_SEL_SPACE_ID, GUI_REDRAW_PARA_CONTAIN, 0, 0);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_ADD_ID:
                break;
            case GUI_KEYPAD_MINUS_ID:
                break;
            case GUI_KEYPAD_CONFIRM_ID:
                break;
        }
    }
    else
    {
        
    }     

    return reVal;
}
BOOL GuiDepositTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Deposit> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            keyIgnoreFlag = TRUE;
            updateBG();
            keyIgnoreFlag = FALSE;
            break;
        case READER_PROCESS_TIMER://不用ignore KEY, 因為太頻繁了
            readerProcess();
            break;
        case EXIT_TIMER:
            keyIgnoreFlag = TRUE;
            GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);  
            keyIgnoreFlag = FALSE;        
            break;

    }
    
    return TRUE;
}

BOOL GuiDepositPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <Deposit> power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:
            //pGuiGetInterface->runTimeoutFunc(READER_PROCESS_TIMER);
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


/**************************************************************************//**
* @file     guistandby.c
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
#include "guidrv.h"
#include "halinterface.h"

#include "epddrv.h"
#include "guimanager.h"
#include "powerdrv.h"
#include "paralib.h"
#include "meterdata.h"
#include "cardreader.h"
#include "spacedrv.h"
#include "tarifflib.h"

#include "guistandby.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define UPDATE_SPACE_DETECT_TIMER   GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   ((60*1000)/portTICK_RATE_MS)
#define UPDATE_SPACE_DETECT_INTERVAL     ((10*1000)/portTICK_RATE_MS)
/*-----------------------------------------*/
/* global file scope (static) variables */
/*-----------------------------------------*/
static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;

static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;
static uint8_t refreshType = GUI_REDRAW_PARA_NONE;
/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <Stand By> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart); 
    switch(refreshType)
    {
        case GUI_REDRAW_PARA_NORMAL:
            sysprintf(" [INFO GUI] <Stand By> updateBG refreshType <GUI_REDRAW_PARA_NORMAL>\n"); 
            EPDDrawItem(FALSE, GetMeterData()->currentSelSpace, 0);
            EPDDrawDepositTime(FALSE, GetMeterData()->currentSelSpace, 0);
            break;
        case GUI_REDRAW_PARA_REFRESH:
            sysprintf(" [INFO GUI] <Stand By> updateBG refreshType <GUI_REDRAW_PARA_REFRESH>\n"); 
            EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE); 
            EPDDrawBannerLine(FALSE);
            EPDDrawContainBG(FALSE);
            EPDDrawAllDepositTime(FALSE);
            break;
        case GUI_REDRAW_PARA_CONTAIN:
            sysprintf(" [INFO GUI] <Stand By> updateBG refreshType <GUI_REDRAW_PARA_CONTAIN>\n"); 
            EPDDrawItem(FALSE, GetMeterData()->currentSelSpace, 0);
            EPDDrawContainBG(FALSE);
            EPDDrawAllDepositTime(FALSE);
            break;
        default:
            sysprintf(" [INFO GUI] <Stand By> updateBG refreshType ERROR :refreshType = %d\n", refreshType); 
            break;
    }
    refreshType = GUI_REDRAW_PARA_NONE;
    
    UpdateClock(TRUE, FALSE);
    
    GetMeterData()->currentSelSpace = 0;
    
    sysprintf(" [INFO GUI] <Stand By> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);  
    //powerStatus = TRUE;    
}
static void updateData(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    /*
    #warning 
    switch(GetMeterData()->spaceSepositStatus[GetMeterPara()->meterPosition - 1])
    {
       case SPACE_DEPOSIT_STATUS_OK:
            ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, TRUE, FALSE, FALSE);
            break;
        case SPACE_DEPOSIT_STATUS_EXCEED_TIME:
            ShowExpired(EPD_PICT_INDEX_EXPIRED_W, TRUE, FALSE, FALSE);
            break;
        case SPACE_DEPOSIT_STATUS_STANBY:
            ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, TRUE, FALSE, FALSE);
            break;
    }
    switch(GetMeterData()->spaceSepositStatus[GetMeterPara()->meterPosition + 1 - 1])
    {
        
         case SPACE_DEPOSIT_STATUS_OK:
            ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, FALSE, TRUE, FALSE);
            break;
        case SPACE_DEPOSIT_STATUS_EXCEED_TIME:
            ShowExpired(EPD_PICT_INDEX_EXPIRED_W, FALSE, TRUE, FALSE);
            break;
        case SPACE_DEPOSIT_STATUS_STANBY:
            ShowExpired(EPD_PICT_INDEX_EXPIRED_BG, FALSE, TRUE, FALSE);
            break;
    }
    
    */
    EPDDrawAllDepositTime(FALSE);
    //sysprintf(" [INFO GUI] <Stand By> updateData enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart); 
    UpdateClock(TRUE, TRUE);
    sysprintf(" [INFO GUI] <Stand By> updateData: Local:[%d]\n", xTaskGetTickCount() - tickLocalStart); 
    
    //#warning just test
    //StartSpaceDrv();
    //pGuiGetInterface->runTimeoutFunc(UPDATE_SPACE_DETECT_TIMER);//更新畫面
    
    //TariffUpdateCurrentTariffData();
    //powerStatus = TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiStandbyOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{   terninalPrintf("StandbyOnDraw");
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> OnDraw (from GuiId = %d, para = %d)\n", oriGuiId, para); 
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);      terninalPrintf("4");
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);      
    PowerDrvSetEnable(TRUE);
    EPDSetSleepFunction(TRUE);
    refreshType = para;
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    CardReaderSetPower(FALSE);
    //
//    EPDReSetBacklightTimeout(5000/portTICK_RATE_MS);
    sysprintf(" [INFO GUI] <Stand By> OnDraw exit: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiStandbyUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n"); 
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
    return TRUE;
}
BOOL GuiStandbyKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <Stand By> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Stand By> Key:  ignore...\n"); 
        return reVal;
    }
    
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_LEFT_ID:
                EPDReSetBacklightTimeout(portMAX_DELAY);
                UpdateMeterCurrentSelSpace(GetMeterPara()->meterPosition, TRUE);
                UpdateMeterDepositTime(TariffGetCurrentTariffType()->timeunit/*SEL_TIME_INTERVAL_TIME*/);
                GuiManagerShowScreen(GUI_SEL_SPACE_ID, GUI_REDRAW_PARA_NORMAL, 0, 0);
                PowerDrvSetEnable(FALSE);
                EPDSetSleepFunction(FALSE);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_RIGHT_ID:
                EPDReSetBacklightTimeout(portMAX_DELAY);
                UpdateMeterCurrentSelSpace(GetMeterPara()->meterPosition+1, TRUE);
                UpdateMeterDepositTime(TariffGetCurrentTariffType()->timeunit);
                GuiManagerShowScreen(GUI_SEL_SPACE_ID, GUI_REDRAW_PARA_NORMAL, 0, 0);
                PowerDrvSetEnable(FALSE);
                EPDSetSleepFunction(FALSE);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_ADD_ID:
                break;
            case GUI_KEYPAD_MINUS_ID:
                break;
            case GUI_KEYPAD_CONFIRM_ID:
                break;
            
            case GUI_KEYPAD_NORMAL_ID:                
                //reVal = TRUE;
                break;
            
            case GUI_KEYPAD_REPLACE_BP_ID:
                EPDReSetBacklightTimeout(portMAX_DELAY);
                GuiManagerShowScreen(GUI_REPLACE_BP_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                reVal = TRUE;
                break;
            
            case GUI_KEYPAD_TESTER_ID:
                EPDReSetBacklightTimeout(portMAX_DELAY);
                GuiManagerShowScreen(GUI_TESTER_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                reVal = TRUE;
                break;
            
             case GUI_KEYPAD_TESTER_KEYPAD_ID:
                EPDReSetBacklightTimeout(portMAX_DELAY);
                GuiManagerShowScreen(GUI_TESTER_KEYPAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                reVal = TRUE;
                break;
            
            default:
                sysprintf(" [INFO GUI] <Stand By> Key:  not support keyId 0x%02x...\n", keyId); 
                break;
        }
    }
    else
    {
        
    }    
    //if(reVal)
    //    EPDReSetBacklightTimeout(5000);
    return reVal;
}
BOOL GuiStandbyTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Stand By> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    powerStatus = FALSE;  
    keyIgnoreFlag = TRUE;
    if(powerStatusFlag == FALSE)
    {
        switch(timerIndex)
        {
            case UPDATE_BG_TIMER:
                updateBG();
                break;
            case UPDATE_DATA_TIMER:
                updateData();
                break;
            case UPDATE_SPACE_DETECT_TIMER:
                StartSpaceDrv();
                break;
        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;  
    return TRUE;
}

BOOL GuiStandbyPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] Standby power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:      
            if(flag == WAKEUP_SOURCE_RTC)  
            {               
                sysprintf(" [INFO GUI] <Stand By> PowerCallbackFunc UPDATE_DATA_TIMER\n");
                powerStatus = FALSE;                 
                pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
            }
            else
            {
                sysprintf(" [INFO GUI] <Stand By> PowerCallbackFunc ignore\n");  
            }
            powerStatusFlag = FALSE;
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            powerStatusFlag = TRUE;
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


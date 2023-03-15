/**************************************************************************//**
* @file     guireplacebp.c
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
#include "guireplacebp.h"
#include "epddrv.h"
#include "guimanager.h"
#include "batterydrv.h"
#include "meterdata.h"
#include "leddrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER         GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER       GUI_TIME_1_INDEX
#define EXIT_TIMER              GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   (2000/portTICK_RATE_MS)
#define EXIT_INTERVAL          portMAX_DELAY
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;

static uint8_t bayColorLocal[6] = {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void batteryStatus2EpdId(uint8_t* leftStatus, uint8_t* rightStatus)
{
    switch(*leftStatus)
    {
    case BATTERY_STATUS_IN_USE:
        *leftStatus = EPD_PICT_INDEX_BATTERY_REPLACE_INUSE;
        break;
    case BATTERY_STATUS_NEED_REPLACE:
        *leftStatus = EPD_PICT_INDEX_BATTERY_REPLACE_NEED_REPLACE;
        break;
    case BATTERY_STATUS_IDLE:
        *leftStatus = EPD_PICT_INDEX_BATTERY_REPLACE_IDLE;
        break;
    case BATTERY_STATUS_EMPTY:
        *leftStatus = EPD_PICT_INDEX_BATTERY_REPLACE_EMPTY;
        break;
    }
    switch(*rightStatus)
    {
    case BATTERY_STATUS_IN_USE:
        *rightStatus = EPD_PICT_INDEX_BATTERY_REPLACE_INUSE;
        break;
    case BATTERY_STATUS_NEED_REPLACE:
        *rightStatus = EPD_PICT_INDEX_BATTERY_REPLACE_NEED_REPLACE;
        break;
    case BATTERY_STATUS_IDLE:
        *rightStatus = EPD_PICT_INDEX_BATTERY_REPLACE_IDLE;
        break;
    case BATTERY_STATUS_EMPTY:
        *rightStatus = EPD_PICT_INDEX_BATTERY_REPLACE_EMPTY;
        break;
    }
}
static void updateContain(void)
{
    UINT32 leftVoltage, rightVoltage;
    uint8_t leftStatus, rightStatus; 
    BatteryGetStatus(&leftStatus, &rightStatus);  
    batteryStatus2EpdId(&leftStatus, &rightStatus);        
    BatteryGetValue(&leftVoltage, &rightVoltage);

                    
    ShowBatteryStatus(rightStatus, TRUE, leftStatus, TRUE, FALSE);
    ShowVoltage(rightVoltage, TRUE, leftVoltage, TRUE, TRUE);
    
    switch(leftStatus)
    {
        case BATTERY_STATUS_IN_USE:
            bayColorLocal[0] = LIGHT_COLOR_GREEN;
            break;
        case BATTERY_STATUS_NEED_REPLACE:
            bayColorLocal[0] = LIGHT_COLOR_RED;
            break;
        case BATTERY_STATUS_IDLE:
            bayColorLocal[0] = LIGHT_COLOR_GREEN;
            break;
        case BATTERY_STATUS_EMPTY:
            bayColorLocal[0] = LIGHT_COLOR_OFF;
            break;
    }
    
    switch(rightStatus)
    {
        case BATTERY_STATUS_IN_USE:
            bayColorLocal[3] = LIGHT_COLOR_GREEN;
            break;
        case BATTERY_STATUS_NEED_REPLACE:
            bayColorLocal[3] = LIGHT_COLOR_RED;
            break;
        case BATTERY_STATUS_IDLE:
            bayColorLocal[3] = LIGHT_COLOR_GREEN;
            break;
        case BATTERY_STATUS_EMPTY:
            bayColorLocal[3] = LIGHT_COLOR_OFF;
            break;
    }
    LedSetColor(bayColorLocal, LIGHT_COLOR_IGNORE, TRUE); 
}
static void updateBG(void)
{

    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <ReplaceBP> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);   
//    EPDShowBGScreen(EPD_PICT_INDEX_BATTERY_REPLACE, FALSE); 
    updateContain();
    sysprintf(" [INFO GUI] <ReplaceBP> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);    
  
}

static void updateData(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    updateContain();
    sysprintf(" [INFO GUI] <ReplaceBP> updateData: [%d]. \n", xTaskGetTickCount() - tickLocalStart);
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiReplaceBPOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <ReplaceBP> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);  

    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL);     

    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //LedSetMode(LED_MODE_REPLACE_BP_INDEX);
    //sysprintf(" [INFO GUI] <ReplaceBP> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiReplaceBPKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <ReplaceBP> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <ReplaceBP> Key:  ignore...\n"); 
        return reVal;
    }
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_LEFT_ID:
                break;
            case GUI_KEYPAD_RIGHT_ID:
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_ADD_ID:
                break;
            case GUI_KEYPAD_MINUS_ID:
                break;
            case GUI_KEYPAD_CONFIRM_ID:
                break;
            case GUI_KEYPAD_NORMAL_ID:
                //MeterSetLedMode(LED_MODE_NORMAL_INDEX);
                //GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);  
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_REPLACE_BP_ID: 
                //LedSetMode(LED_MODE_NORMAL_INDEX);
                GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0); 
                reVal = TRUE;                 
                break;
            
            case GUI_KEYPAD_TESTER_KEYPAD_ID:  
                //LedSetMode(LED_MODE_NORMAL_INDEX);
                //GuiManagerShowScreen(GUI_TESTER_KEYPAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0); 
                //reVal = TRUE;            
                break;
            
            case GUI_KEYPAD_TESTER_ID:                
                //LedSetMode(LED_MODE_NORMAL_INDEX);
                //GuiManagerShowScreen(GUI_TESTER_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
             
        }
    }
    else
    {
        
    }     

    return reVal;
}
BOOL GuiReplaceBPTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <ReplaceBP> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            updateBG();
            break;
        case UPDATE_DATA_TIMER:
            updateData();
            break;
        case EXIT_TIMER:     
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiReplaceBPPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <ReplaceBP> power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:
            //pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


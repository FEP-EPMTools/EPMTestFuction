/**************************************************************************//**
* @file     guitester.c
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
#include "guitester.h"
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
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static void updateContain(void)
{

}
static void updateBG(void)
{

    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <Tester> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);   
//    EPDShowBGScreen(EPD_PICT_INDEX_TESTER, TRUE); 
    updateContain();
    sysprintf(" [INFO GUI] <Tester> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);    
  
}

static void updateData(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    updateContain();
    sysprintf(" [INFO GUI] <Tester> updateData: [%d]. \n", xTaskGetTickCount() - tickLocalStart);
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiTesterOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Tester> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);  

    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL);     

    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //LedSetMode(LED_MODE_AUTO_TEST_INDEX);
    //sysprintf(" [INFO GUI] <Tester> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiTesterKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <Tester> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Tester> Key:  ignore...\n"); 
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
                //LedSetMode(LED_MODE_NORMAL_INDEX);
                GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);  
                reVal = TRUE;
                break;
            
            case GUI_KEYPAD_REPLACE_BP_ID:
                //LedSetMode(LED_MODE_NORMAL_INDEX);
                //GuiManagerShowScreen(GUI_REPLACE_BP_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
            
            case GUI_KEYPAD_TESTER_KEYPAD_ID:
                //LedSetMode(LED_MODE_NORMAL_INDEX);
                GuiManagerShowScreen(GUI_TESTER_KEYPAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                reVal = TRUE;
                break;
            
            case GUI_KEYPAD_TESTER_ID: 
                //reVal = TRUE;                
                break;
             
        }
    }
    else
    {
        
    }     

    return reVal;
}
BOOL GuiTesterTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Tester> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
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

BOOL GuiTesterPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <Tester> power [%d] : flag = %d!!\n", type, flag);
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


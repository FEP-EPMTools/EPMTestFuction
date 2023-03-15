/**************************************************************************//**
* @file     guioff.c
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
#include "guifiledownload.h"
#include "epddrv.h"
#include "guimanager.h"
#include "tarifflib.h"
#include "meterdata.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define EXIT_TIMER          GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   (2000/portTICK_RATE_MS)
#define EXIT_INTERVAL          (6000/portTICK_RATE_MS)
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static void updateData(void);

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = TRUE;
static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <Off> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
//    EPDShowBGScreen(EPD_PICT_INDEX_OFF, TRUE); 
    sysprintf(" [INFO GUI] <Off> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);    
 
}
static void updateData(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <Off> updateData enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
    
    //sysprintf(" [INFO GUI] <Off> updateData: [%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - keyStart);
    //sysDelay(100);
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiOffOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Off> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);   
    powerStatus = TRUE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
    
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //sysprintf(" [INFO GUI] <Off> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiOffKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <Off> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <Off> Key:  ignore...\n"); 
        return reVal;
    }
    //pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_LEFT_ID:
               
                break;
            case GUI_KEYPAD_RIGHT_ID:
                
                break;
            case GUI_KEYPAD_ADD_ID:
               
                break;
            case GUI_KEYPAD_MINUS_ID:
                
                break;
        #if(SUPPORT_HK_10_HW)
            case GUI_KEYPAD_QRCODE_ID:
        #else
            case GUI_KEYPAD_CONFIRM_ID:
        #endif
                
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
        }
    }
    else
    {
        
    }
    return reVal;
}
BOOL GuiOffTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Off> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
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
            //GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);        
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiOffPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <Off> power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


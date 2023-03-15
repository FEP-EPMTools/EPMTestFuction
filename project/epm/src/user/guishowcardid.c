/**************************************************************************//**
* @file     guishowcardid.c
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
#include "guishowcardid.h"
#include "epddrv.h"
#include "guimanager.h"
#include "powerdrv.h"
#include "paralib.h"
#include "meterdata.h"
#include "cardreader.h"
#include "spacedrv.h"
#include "tarifflib.h"
#include "hwtester.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define UPDATE_SPACE_DETECT_TIMER   GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   (500/portTICK_RATE_MS)
#define UPDATE_SPACE_DETECT_INTERVAL     (50/portTICK_RATE_MS)

#define BTM_DISCRIPT_BAR_X 80
#define BTM_DISCRIPT_BAR_Y 600-44

#define X_HEAD_TITLE    90
#define Y_HEAD_TITLE    28
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

            break;
        case GUI_REDRAW_PARA_REFRESH:

            break;
        case GUI_REDRAW_PARA_CONTAIN:
            
            break;
        default:
            sysprintf(" [INFO GUI] <Stand By> updateBG refreshType ERROR :refreshType = %d\n", refreshType); 
            break;
                
    }
    refreshType = GUI_REDRAW_PARA_NONE;
    //Draw BG
    EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    EPDDrawString(FALSE,"Show Card ID",X_HEAD_TITLE,Y_HEAD_TITLE);
    EPDDrawString(FALSE,"CARD ID :",170,454);
    EPDDrawString(TRUE,"Press } To Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    EPDDrawString(TRUE,"Please wait   ",240,254);
//    epdShowID(cardID);
//    UpdateClock(TRUE, FALSE);
    
//    GetMeterData()->currentSelSpace = 0;
    
    sysprintf(" [INFO GUI] <Stand By> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);  
    //powerStatus = TRUE;    
}
static void updateData(void)
{
    
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiShowCardIdOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{   
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> OnDraw (from GuiId = %d, para = %d)\n", oriGuiId, para); 
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);      
    PowerDrvSetEnable(TRUE);
    EPDSetSleepFunction(FALSE);
    refreshType = para;
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    
    //CardReaderSetPower(EPM_READER_CTRL_ID_GUI, FALSE);
    //EPDReSetBacklightTimeout(5000/portTICK_RATE_MS);
    
    updateBG();
    return TRUE;
}
BOOL GuiShowCardIdUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n"); 
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
    return TRUE;
}
BOOL GuiShowCardIdKeyCallback(uint8_t keyId, uint8_t downUp)
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
                SetGuiResponseVal('q');
                reVal = TRUE;
                break;
            
            case GUI_KEYPAD_NORMAL_ID:                
                //reVal = TRUE;
                break;
            
            case GUI_KEYPAD_REPLACE_BP_ID:
//                EPDReSetBacklightTimeout(portMAX_DELAY);
                GuiManagerShowScreen(GUI_REPLACE_BP_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                reVal = TRUE;
                break;
            
            case GUI_KEYPAD_TESTER_ID:
//                EPDReSetBacklightTimeout(portMAX_DELAY);
                GuiManagerShowScreen(GUI_TESTER_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                reVal = TRUE;
                break;
            
             case GUI_KEYPAD_TESTER_KEYPAD_ID:
//                EPDReSetBacklightTimeout(portMAX_DELAY);
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
BOOL GuiShowCardIdTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Stand By> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    powerStatus = FALSE;  
    keyIgnoreFlag = TRUE;
    if(powerStatusFlag == FALSE)
    {
        switch(timerIndex)
        {
            case UPDATE_BG_TIMER:
                //updateBG();
                break;
            case UPDATE_DATA_TIMER:
//                updateData();
                break;
            case UPDATE_SPACE_DETECT_TIMER:
                updateData();
//                StartSpaceDrv();
                break;

        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;  
    return TRUE;
}

BOOL GuiShowCardIdPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] Standby power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:      
//            if(flag == WAKEUP_SOURCE_RTC)  
//            {               
//                sysprintf(" [INFO GUI] <Stand By> PowerCallbackFunc UPDATE_DATA_TIMER\n");
//                powerStatus = FALSE;                 
//                pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
//            }
//            else
//            {
//                sysprintf(" [INFO GUI] <Stand By> PowerCallbackFunc ignore\n");  
//            }
//            powerStatusFlag = FALSE;
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


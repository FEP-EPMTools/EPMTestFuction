/**************************************************************************//**
* @file     guifactory.c
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
#include "guifactory.h"
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
#define UPDATE_DATA_INTERVAL            ((500)/portTICK_RATE_MS)
#define UPDATE_SPACE_DETECT_INTERVAL    ((10*1000)/portTICK_RATE_MS)
#define TITLE_HIGH 100
#define EPD_WIDTH 1024
#define EPD_HEIGHT  758
#define MENU_WIDTH 920
#define MIDDLE          (MENU_WIDTH/2)+X_AFTER_SHIFT
#define X_AFTER_SHIFT   EPD_WIDTH-MENU_WIDTH
#define X_SINGLE_ITEM   X_AFTER_SHIFT
#define X_RESULT_SHIFT  1024-500
#define Y_RESULT_SHIFT  TITLE_HIGH
#define Y_RESULT_HIGH          35
#define Y_ITEM_HIGH     40
#define X_BTM_MSG_BAR   0
#define Y_BTM_MSG_BAR   EPD_HEIGHT-100

#define maxItem 13
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/


static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;

static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;
static uint8_t refreshType = GUI_REDRAW_PARA_NONE;

static int nowIndex=0,oldIndex=0;
static int statusStage=0;
static BOOL changeMsg=FALSE;
static BOOL testing=FALSE;
static BOOL keyTest=FALSE;
static int keyTestTime=0;

/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/
static void updateBG(void)
{
    EPDDrawString(TRUE,"please connect to computer and test with terminal",100,200);
}
static void updateData(void){
    
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiFactoryOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3){ 
    
    powerStatus = FALSE;     
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);      
    
    PowerDrvSetEnable(TRUE);
    EPDSetSleepFunction(FALSE);
    
    refreshType = para;
    
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//§óupdateScreen
    
    //CardReaderSetPower(EPM_READER_CTRL_ID_GUI, FALSE);
    updateBG();
    return TRUE;
}
BOOL GuiFactoryUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n");
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//§óupdateScreen
    return TRUE;
}


BOOL GuiFactoryKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <Stand By> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Stand By> Key:  ignore...\n"); 
        return reVal;
    }
    else if((GUI_KEY_DOWN_INDEX == downUp)&!keyIgnoreFlag)
    {
        switch(keyId)
        {
    #if(SUPPORT_HK_10_HW)
        case GUI_KEYPAD_QRCODE_ID:
    #else
        case GUI_KEYPAD_CONFIRM_ID:
    #endif
            reVal = TRUE;
            break;
        case GUI_KEYPAD_LEFT_ID:
            reVal = TRUE;
            break;
        case GUI_KEYPAD_RIGHT_ID:
            break;
        case GUI_KEYPAD_ADD_ID:
            reVal = TRUE;
            break;
        case GUI_KEYPAD_MINUS_ID:
            reVal = TRUE;
            break;
        case GUI_KEYPAD_NORMAL_ID:
            reVal = TRUE;
            break;
        case GUI_KEYPAD_REPLACE_BP_ID:
            reVal = TRUE;
            break;
        case GUI_KEYPAD_TESTER_ID:
            reVal = TRUE;
            break;
         case GUI_KEYPAD_TESTER_KEYPAD_ID:
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
BOOL GuiFactoryTimerCallback(uint8_t timerIndex)
{
    keyIgnoreFlag = TRUE;
    //sysprintf(" [INFO GUI] <Stand By> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    powerStatus = FALSE;  
    if(powerStatusFlag == FALSE)
    {
        switch(timerIndex)
        {
            case UPDATE_BG_TIMER:
                //updateBG();
                break;
            case UPDATE_DATA_TIMER:
                break;
            case UPDATE_SPACE_DETECT_TIMER:
//                StartSpaceDrv();
                break;
        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;  
    return TRUE;
}

BOOL GuiFactoryPowerCallbackFunc(uint8_t type, int flag)
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


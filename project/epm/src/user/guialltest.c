/**************************************************************************//**
* @file     guialltest.c
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
#include "guialltest.h"
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
#define UPDATE_DATA_INTERVAL            ((50)/portTICK_RATE_MS)
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

#define STRING_HEIGHT   44

#define KEYPAD_TEST_INDEX 6
#define MAX_SELECT_ITEM maxSelectItem
/*-----------------------------------------*/
/* global file scope (static) variables     */
/*-----------------------------------------*/
static int maxSelectItem = 0;

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
static BOOL keypadTestFlag=FALSE;

static HWTesterItem* item;
/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/
static void updateMsg(void){
    EPDDrawString(GetRefresh(),GetEPDString(),X_RESULT_SHIFT,Y_RESULT_SHIFT);
    SetIsEPDDone(TRUE);
}

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
    EPDDrawString(FALSE,"All Testing",80,4);
    EPDDrawString(FALSE,"Testing Item:",170,104);
    EPDDrawString(TRUE,"Message:",170,254);
    
//    UpdateClock(TRUE, FALSE);
    
//    GetMeterData()->currentSelSpace = 0;
    
    sysprintf(" [INFO GUI] <Stand By> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);  
    //powerStatus = TRUE;    
}
static void updateData(void){
    
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiAllTestOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3){ 
    if(para2==para3){

    }
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);      
    
    PowerDrvSetEnable(TRUE);
    EPDSetSleepFunction(FALSE);
    
    refreshType = para;
    
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//§óupdateScreen
    
    CardReaderSetPower(EPM_READER_CTRL_ID_GUI, FALSE);
    return TRUE;
}
BOOL GuiAllTestUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n");
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//§óupdateScreen
    return TRUE;
}


BOOL GuiAllTestKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <Stand By> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Stand By> Key:  ignore...\n"); 
        return reVal;
    }
    //Normal Key Function//
    else if((GUI_KEY_DOWN_INDEX == downUp)&!keyIgnoreFlag)
    {
        switch(keyId)
        {
        case GUI_KEYPAD_CONFIRM_ID:
            
            SetGuiResponseVal('q');
            reVal = TRUE;
            break;
        case GUI_KEYPAD_RIGHT_ID:
            
            break;
        case GUI_KEYPAD_LEFT_ID:
            
            break;
        case GUI_KEYPAD_ADD_ID:
            break;
        case GUI_KEYPAD_MINUS_ID:
            
            break;
        case GUI_KEYPAD_NORMAL_ID:
            break;
        case GUI_KEYPAD_REPLACE_BP_ID:
            break;
        case GUI_KEYPAD_TESTER_ID:
            break;
         case GUI_KEYPAD_TESTER_KEYPAD_ID:
            break;
        default:
            sysprintf(" [INFO GUI] <Stand By> Key:  not support keyId 0x%02x...\n", keyId); 
            break;
        }
    }
    //if(reVal)
    //    EPDReSetBacklightTimeout(5000);
    return reVal;
}
BOOL GuiAllTestTimerCallback(uint8_t timerIndex)
{
    keyIgnoreFlag = TRUE;
    //sysprintf(" [INFO GUI] <Stand By> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    powerStatus = FALSE;  
    if(powerStatusFlag == FALSE)
    {
        switch(timerIndex)
        {
            case UPDATE_BG_TIMER:
                updateBG();
                break;
            case UPDATE_DATA_TIMER:
//                updateMsg();
//                
                if(oldIndex!=nowIndex){
                    //deselect item change to black
                    EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,80,104+(oldIndex*STRING_HEIGHT));
                    //select item change to white
                    EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,80,104+(nowIndex*STRING_HEIGHT));
                    oldIndex=nowIndex;
                }
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

BOOL GuiAllTestPowerCallbackFunc(uint8_t type, int flag)
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


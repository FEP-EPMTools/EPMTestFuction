/**************************************************************************//**
* @file     guitool.c
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

#include "guibaselist.h"
#include "guidrv.h"
#include "epddrv.h"
#include "guimanager.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define UPDATE_SPACE_DETECT_TIMER   GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   (50/portTICK_RATE_MS)
#define UPDATE_SPACE_DETECT_INTERVAL     portMAX_DELAY

#define STRING_HEIGHT   50

#define X_HEAD_TITLE    90
#define Y_HEAD_TITLE    28

#define MAX_SELECT_ITEM maxSelectItem
/*-----------------------------------------*/
/* global file scope (static) variables */
/*-----------------------------------------*/
static int maxSelectItem = 0;

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;
static BOOL keyIgnoreFlag = FALSE;

static KeyinCallback keyinCallback = NULL;
static int nowIndex=0,oldIndex=0;
static char* title = "BaseList";
static ListItem* item;
/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/
static void updateBG(void)
{
    EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    EPDDrawStringMax(FALSE,title,X_HEAD_TITLE,Y_HEAD_TITLE,FALSE);
    //MENU
    for(int i = 0; ; i++)
    {
        //Dont Show Quit
        if(item[i].charItem == 'q')
        {
            maxSelectItem=i;
            break;
        }
        if(item[i].itemName == NULL)
        {
            maxSelectItem=i;
            break;
        }
        EPDDrawStringMax(FALSE,item[i].itemName,180,104+(i*STRING_HEIGHT),FALSE);
        EPDDrawStringMax(FALSE,"-",150,104+(i*STRING_HEIGHT),TRUE);
        //EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,150,104+(i*STRING_HEIGHT));
    }
    //Select Item//
    EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,150,104+(oldIndex*STRING_HEIGHT));
    //terninalPrintf("======== Tool ========[INFO GUI] <GuiOnDraw>  [%d0'ms].\n", xTaskGetTickCount() - tickLocalStart);
}

static void vDrawingTask( void *pvParameters )
{
    
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiBaseListOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{  
    item = ((GUIBaseListItem*)para2)->lisitem;
    title= ((GUIBaseListItem*) para2) -> title;
    keyinCallback = (KeyinCallback) para3;
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);      
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);
    
    updateBG();
    return TRUE;
}

BOOL GuiBaseListUpdateData(void)
{    
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n");
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
    return TRUE;
}

BOOL GuiBaseListKeyCallback(uint8_t keyId, uint8_t downUp)
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
            case GUI_KEYPAD_ONE:
                if((nowIndex>0)&&(nowIndex==oldIndex)){
                    oldIndex=nowIndex;
                    nowIndex--;
                    reVal = TRUE;
                }
                else if((nowIndex==oldIndex)&&(nowIndex==0)){
                    oldIndex=nowIndex;
                    nowIndex=MAX_SELECT_ITEM-1;
                    reVal = TRUE;
                }
                break;
                
            case GUI_KEYPAD_TWO:
                if((nowIndex<(MAX_SELECT_ITEM-1))&&(nowIndex==oldIndex)){
                    oldIndex=nowIndex;
                    nowIndex++;
                    reVal = TRUE;
                }
                else if((nowIndex==oldIndex)&&(nowIndex==MAX_SELECT_ITEM-1)){
                    oldIndex=nowIndex;
                    nowIndex=0;
                    reVal = TRUE;
                }
                break;
            
            case GUI_KEYPAD_THREE:
                reVal = TRUE;
                break;
            
            case GUI_KEYPAD_FOUR:
                reVal = TRUE;
                break;
            
            case GUI_KEYPAD_FIVE:
                if(nowIndex == oldIndex)
                {
                    keyinCallback(item[nowIndex].charItem);
                    reVal = TRUE;
                    reVal = TRUE;
                }
                break;
            case GUI_KEYPAD_SIX:
                keyinCallback('q');
                nowIndex = 0;
                oldIndex = 0;
                reVal = TRUE;
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
    return reVal;
}
BOOL GuiBaseListTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Stand By> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    powerStatus = FALSE;  
    keyIgnoreFlag = TRUE;
    if(powerStatusFlag == FALSE)
    {
        switch(timerIndex)
        {
            case UPDATE_BG_TIMER:
                break;
            case UPDATE_DATA_TIMER:
                if(oldIndex!=nowIndex)
                {
                    //deselect item change to black
                    EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,150,104+(oldIndex*50));
                    //select item change to white
                    EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,150,104+(nowIndex*50));
                    oldIndex=nowIndex;
                }
                break;
            case UPDATE_SPACE_DETECT_TIMER:
                break;
        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;  
    return TRUE;
}

BOOL GuiBaseListPowerCallbackFunc(uint8_t type, int flag)
{
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


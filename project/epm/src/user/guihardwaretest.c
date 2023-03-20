/**************************************************************************//**
* @file     guihardwaretest.c
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
#include "guihardwaretest.h"
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
#define UPDATE_DATA_INTERVAL   (50/portTICK_RATE_MS)
#define UPDATE_SPACE_DETECT_INTERVAL    ((10*1000)/portTICK_RATE_MS)

#define EPD_WIDTH 1024
#define EPD_HEIGHT 758
#define MENU_WIDTH 920
#define X_ORIGIN 1028-960
#define Y_ORIGIN 10
#define X_AFTER_SHIFT EPD_WIDTH-MENU_WIDTH
#define X_ITEM  112+X_AFTER_SHIFT
#define Y_ITEM_HIGH 75
#define X_BTM_MSG_BAR 414
#define Y_BTM_MSG_BAR 650

#define X_HEAD_TITLE    90
#define Y_HEAD_TITLE    28

#define STRING_HEIGHT   50

#define MAX_SELECT_ITEM maxSelectItem

#define BUILD_VERSION_HW   BUILD_VERSION

/*-----------------------------------------*/
/* global file scope (static) variables */
/*-----------------------------------------*/
static int maxSelectItem = 0;
static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;

static BOOL keyIgnoreFlag = FALSE;
static int nowIndex=0,oldIndex=0;

static int secretCounter = 0;

//TODO change to struct list match hwtest function
//static char response[]={'t'};

static HWTesterItem* item;
/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/
static void updateBG(void)
{
    //TickType_t tickLocalStart = xTaskGetTickCount();
    //Draw BG
    EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    EPDDrawStringMax(FALSE,"Main Menu",X_HEAD_TITLE,Y_HEAD_TITLE,FALSE);
    //MENU//
    for(int i = 0; ; i++)
    {
        //Dont Show Quit
        if(item[i].itemName == NULL)
        {
            maxSelectItem=i;
            break;
        }
        if(item[i].charItem == 'q')
        {
            maxSelectItem=i;
            break;
        }

        EPDDrawStringMax(FALSE,item[i].itemName,180,104+(i*STRING_HEIGHT),TRUE);
        EPDDrawStringMax(FALSE,"-",130,104+(i*STRING_HEIGHT),TRUE);
        //EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,130,104+(i*STRING_HEIGHT));
        
    }
    //EPDDrawStringMax(FALSE,"------- Version  ------",180,350,FALSE);
    //EPDDrawStringMax(FALSE,"AP\nReader\nEPD\nSIM\nMODEM\nRADAR\n",240,400,TRUE);
    //Select Item//
    //EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,130,104+(nowIndex*STRING_HEIGHT));
    EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_I_INDEX,130,104+(nowIndex*STRING_HEIGHT));
    char versionString[35]; 
    sprintf(versionString,"Ver. %d.%02d.%02d  build:%u",MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, getdataCode());
    EPDDrawString(TRUE,versionString,100,700);
    //terninalPrintf("========MAIN MENU========[INFO GUI] <GuiOnDraw>  [%d0'ms].\n", xTaskGetTickCount() - tickLocalStart); 
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/

BOOL GuiHWTestOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{     
    item=(HWTesterItem*) para2;
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);
    
    PowerDrvSetEnable(TRUE);
    EPDSetSleepFunction(TRUE);
    EPDSetSleepFunction(FALSE);
    
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//§óupdateScreen
    
    updateBG();
    return TRUE;
}

BOOL GuiHWTestUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n"); 
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//§óupdateScreen
    return TRUE;
}

BOOL GuiHWTestKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <Stand By> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Stand By> Key:  ignore...\n");
        return reVal;
    }
    if(nowIndex!=oldIndex)
    {
       return reVal;
    }
    if((GUI_KEY_DOWN_INDEX == downUp)&!keyIgnoreFlag)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_ONE:
                if((nowIndex>0)&&(nowIndex==oldIndex))
                {
                    oldIndex=nowIndex;
                    nowIndex--;
                }
                else if((nowIndex==oldIndex)&&(nowIndex==0))
                {
                    oldIndex=nowIndex;
                    nowIndex=MAX_SELECT_ITEM-1;
                }
                
                secretCounter = 0;
                
                reVal = TRUE;
                break;
            case GUI_KEYPAD_TWO:
                if((nowIndex<(MAX_SELECT_ITEM-1))&&(nowIndex==oldIndex))
                {
                    oldIndex=nowIndex;
                    nowIndex++;
                }
                else if((nowIndex==oldIndex)&&(nowIndex==MAX_SELECT_ITEM-1))
                {
                    oldIndex=nowIndex;
                    nowIndex=0;
                }
                
                secretCounter = 0;
                
                reVal = TRUE;
                break;
            case GUI_KEYPAD_THREE:
                if ((secretCounter == 0) ||(secretCounter == 1))
                 {
                     secretCounter++;
                 }
                else
                 {
                     secretCounter = 0;
                 }
                reVal = TRUE;
                break;
            case GUI_KEYPAD_FOUR:
                if ((secretCounter == 2) ||(secretCounter == 3))
                 {
                     secretCounter++;
                 }
                else
                 {
                     secretCounter = 0;
                 }
            
            
                reVal = TRUE;
                break;
            case GUI_KEYPAD_FIVE:
//                GuiManagerShowScreen((GUI_ALL_TEST_ID+nowIndex), GUI_REDRAW_PARA_REFRESH, 0, 0);
                SetGuiResponseVal(item[nowIndex].charItem);
            
                secretCounter = 0;
            
                reVal = TRUE;
                break;
            case GUI_KEYPAD_SIX:
                if (secretCounter == 4)
                 {
                     SetGuiResponseVal('b');
                 }
                else
                 {
                     secretCounter = 0;
                 }
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

BOOL GuiHWTestTimerCallback(uint8_t timerIndex)
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
                if(oldIndex!=nowIndex)
                {
                    //deselect item change to white
                    EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,130,104+(oldIndex*50));
                    //select item change to black
                    EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,130,104+(nowIndex*50));
                    oldIndex=nowIndex;
                }
                break;
            case UPDATE_SPACE_DETECT_TIMER:
                //StartSpaceDrv();
                break;
        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;
    return TRUE;
}

BOOL GuiHWTestPowerCallbackFunc(uint8_t type, int flag)
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


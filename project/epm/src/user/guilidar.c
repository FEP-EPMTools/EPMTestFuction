/**************************************************************************//**
* @file     guiradar.c
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

#include "buzzerdrv.h"
#include "hwtester.h"

#include "guilidar.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define UPDATE_RUNNING_DOT_TIMER   GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   ((60*1000)/portTICK_RATE_MS)
#define UPDATE_RUNNING_DOT_INTERVAL     (1000/portTICK_RATE_MS)

#define POWER_SET_MODE 1

/* XY PARA */
#define OFFSET_X 80
#define RADAR_0_MSG_X 180
#define RADAR_0_MSG_Y 150
#define RADAR_1_MSG_X 580
#define RADAR_1_MSG_Y 150

#define X_HEAD_TITLE    90
#define Y_HEAD_TITLE    28

#define SET_POWER_TITLE_X 180
#define SET_POWER_TITLE_Y 150
#define RADAR_0_TITLE_X 180
#define RADAR_0_TITLE_Y 250
#define RADAR_1_TITLE_X 180
#define RADAR_1_TITLE_Y 300
#define RADAR_0_ON_OFF_X 480
#define RADAR_0_ON_OFF_Y 250
#define RADAR_1_ON_OFF_X 480
#define RADAR_1_ON_OFF_Y 300
#define QUIT_X        480
#define QUIT_Y        350
#define BTM_DISCRIPT_BAR_X OFFSET_X
#define BTM_DISCRIPT_BAR_Y 700-STRING_HEIGHT

#define STRING_HEIGHT   44

#define RUNNING_COUNT_X 180
#define RUNNING_COUNT_Y 300

/* Cursor */
#define MAX_CURSOR        2

#define RADAR_0_ON_INDEX  0
#define RADAR_1_ON_INDEX  1
#define QUIT_INDEX        2
typedef struct{
    int index;
    int x;
    int y;
}CursorPos;
    
/*-----------------------------------------*/
/* global file scope (static) variables */
/*-----------------------------------------*/
static int param3=0;
static CursorPos cursorPos[]=
{
    {RADAR_0_ON_INDEX,RADAR_0_ON_OFF_X-44,RADAR_0_ON_OFF_Y},
    {RADAR_1_ON_INDEX,RADAR_1_ON_OFF_X-44,RADAR_1_ON_OFF_Y},
    //{QUIT_INDEX,QUIT_X-44,QUIT_Y}
};

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;
    
static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;

static int nowcursor=0;

/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();


    /*  DRAW Background */
    /*  para3           */
    /*  0 - show status */
    /*  1 - set power   */
    if(param3==0){
        EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        EPDDrawString(FALSE,"Lidar Tool",X_HEAD_TITLE,Y_HEAD_TITLE);
        
        EPDDrawString(FALSE,"Status:\nLidar 0",RADAR_0_MSG_X,RADAR_0_MSG_Y);
        EPDDrawString(FALSE,"\nLidar 1",RADAR_1_MSG_X,RADAR_1_MSG_Y);
        EPDDrawString(TRUE,"Press } To Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if(param3==POWER_SET_MODE){
        EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        EPDDrawString(FALSE,"Lidar Power Set",X_HEAD_TITLE,Y_HEAD_TITLE);

        EPDDrawString(FALSE,"Power:",SET_POWER_TITLE_X,SET_POWER_TITLE_Y);
        EPDDrawString(FALSE,"Lidar 0",RADAR_0_TITLE_X,RADAR_0_TITLE_Y);
        EPDDrawString(FALSE,"Lidar 1",RADAR_1_TITLE_X,RADAR_1_TITLE_Y);
        //EPDDrawString(FALSE,"QUIT",QUIT_X,QUIT_Y);
        EPDDrawString(TRUE," <>:Move   {:ON\\OFF   }:Exit ",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
        EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[0].x,cursorPos[0].y);
        EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[1].x,cursorPos[1].y);
        //EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[2].x,cursorPos[2].y);
        EPDDrawMulti(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
    }
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiLidarOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{   
    param3=para3;
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> OnDraw (from GuiId = %d, para = %d)\n", oriGuiId, para); 
    nowcursor=0;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_RUNNING_DOT_TIMER, UPDATE_RUNNING_DOT_INTERVAL);      

    EPDSetSleepFunction(FALSE);

    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //CardReaderSetPower(FALSE);
    //
    
    updateBG();
    sysprintf(" [INFO GUI] <Stand By> OnDraw exit: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiLidarUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n"); 
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
    return TRUE;
}
BOOL GuiLidarKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <Stand By> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Radar> Key:  ignore...\n");
        return reVal;
    }
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        keyIgnoreFlag=TRUE;
        switch(keyId)
        {
            case GUI_KEYPAD_ONE:
                if(nowcursor>0&&param3==POWER_SET_MODE)
                {
                    reVal = TRUE;
                    BuzzerPlay(100, 50, 1, FALSE);
                    EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                    nowcursor--;
                    EPDDrawMulti(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                }
                break;
                
            case GUI_KEYPAD_TWO:
                if(nowcursor<MAX_CURSOR-1&&param3==POWER_SET_MODE)
                {
                    reVal = TRUE;
                    BuzzerPlay(100, 50, 1, FALSE);
                    EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                    nowcursor++;
                    EPDDrawMulti(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                }
                break;
            
            case GUI_KEYPAD_THREE:
                break;
            
            case GUI_KEYPAD_FOUR:
                break;
            
            case GUI_KEYPAD_FIVE:
                //BuzzerPlay(100, 50, 1, FALSE);
                /*if(param3==0)
                {
                    SetGuiResponseVal('q');
                    break;
                }*/
                if(nowcursor==0&&param3==POWER_SET_MODE)
                {
                    SetGuiResponseVal('0');
                }
                else if(nowcursor==1&&param3==POWER_SET_MODE)
                {
                    SetGuiResponseVal('1');
                }
                /*else if(nowcursor==2)
                {
                    SetGuiResponseVal('q');
                }*/
                reVal = TRUE;
                break;
                
            case GUI_KEYPAD_SIX:
                reVal = TRUE;
                SetGuiResponseVal('q');
                break;
            
            case GUI_KEYPAD_NORMAL_ID:                
                //reVal = TRUE;
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
        keyIgnoreFlag=FALSE;
    }
    else
    {
        
    }
    //if(reVal)
    //    EPDReSetBacklightTimeout(5000);
    return reVal;
}
BOOL GuiLidarTimerCallback(uint8_t timerIndex)
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
            //updateData();
            break;
        case UPDATE_RUNNING_DOT_TIMER:
            break;
        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;  
    return TRUE;
}

BOOL GuiLidarPowerCallbackFunc(uint8_t type, int flag)
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


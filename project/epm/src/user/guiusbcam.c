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

#include "guiusbcam.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define UPDATE_RUNNING_DOT_TIMER   GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   ((50)/portTICK_RATE_MS) //((60*1000)/portTICK_RATE_MS)
#define UPDATE_RUNNING_DOT_INTERVAL     ((2500)/portTICK_RATE_MS)

/* XY PARA */
#define POWER_SET_MODE 1

#define OFFSET_X 80

#define CAM_0_MSG_X 180
#define CAM_0_MSG_Y 150
#define CAM_1_MSG_X 580
#define CAM_1_MSG_Y 150

#define X_HEAD_TITLE    90
#define Y_HEAD_TITLE    28

#define SET_POWER_TITLE_X 180
#define SET_POWER_TITLE_Y 150
#define CAM_0_TITLE_X 180
#define CAM_0_TITLE_Y 250
#define CAM_1_TITLE_X 180
#define CAM_1_TITLE_Y 300
#define CAM_0_ON_OFF_X 480
#define CAM_0_ON_OFF_Y 250
#define CAM_1_ON_OFF_X 480
#define CAM_1_ON_OFF_Y 300
#define QUIT_X        480
#define QUIT_Y        350

#define BTM_DISCRIPT_BAR_X OFFSET_X
#define BTM_DISCRIPT_BAR_Y 700-STRING_HEIGHT

#define STRING_HEIGHT   44

#define RUNNING_COUNT_X 180
#define RUNNING_COUNT_Y 300
/* Cursor */

#define CAM_0_ON_INDEX  0
#define CAM_1_ON_INDEX  1
//#define QUIT_INDEX        2
//#define MAX_CURSOR    3
#define MAX_CURSOR    2
typedef struct{
    int index;
    int x;
    int y;
}CursorPos;
    
/*-----------------------------------------*/
/* global file scope (static) variables */
/*-----------------------------------------*/
static int param3=0;

//選單的List
static CursorPos cursorPos[]={
    {CAM_0_ON_INDEX,CAM_0_ON_OFF_X-44,CAM_0_ON_OFF_Y},
    {CAM_1_ON_INDEX,CAM_1_ON_OFF_X-44,CAM_1_ON_OFF_Y},
    //{QUIT_INDEX,QUIT_X-44,QUIT_Y}
};

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;
    
static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;

static int nowcursor=0;


static int maxSelectItem = 2;
static int nowIndex=0,oldIndex=0;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();


    //  DRAW Background //
    //  para3           //
    //  0 - show status //
    //  1 - set power   //
    if(param3==0){
        EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        EPDDrawString(FALSE,"CAM Tool",X_HEAD_TITLE,Y_HEAD_TITLE);
        
        //EPDDrawString(FALSE,"Status:\nCAM 0",CAM_0_MSG_X,CAM_0_MSG_Y);
        //EPDDrawString(FALSE,"\nCAM 1",CAM_1_MSG_X,CAM_1_MSG_Y);
        EPDDrawString(FALSE,"Status:\nCAM 1",CAM_0_MSG_X,CAM_0_MSG_Y);
        EPDDrawString(FALSE,"\nCAM 2",CAM_1_MSG_X,CAM_1_MSG_Y);
        
        EPDDrawString(TRUE,"Press } To Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if(param3==POWER_SET_MODE){
        EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        EPDDrawString(FALSE,"CAM Power Set",X_HEAD_TITLE,Y_HEAD_TITLE);
        
        EPDDrawStringMax(FALSE,"Power:",SET_POWER_TITLE_X,SET_POWER_TITLE_Y,FALSE);
        //EPDDrawStringMax(FALSE,"CAM 0",CAM_0_TITLE_X,CAM_0_TITLE_Y,FALSE);
        //EPDDrawStringMax(FALSE,"CAM 1",CAM_1_TITLE_X,CAM_1_TITLE_Y,FALSE);
        EPDDrawStringMax(FALSE,"CAM 1",CAM_0_TITLE_X,CAM_0_TITLE_Y,FALSE);
        EPDDrawStringMax(FALSE,"CAM 2",CAM_1_TITLE_X,CAM_1_TITLE_Y,FALSE);
        //EPDDrawStringMax(FALSE," All OFF",QUIT_X,QUIT_Y,FALSE);
        EPDDrawStringMax(FALSE," <>:Move   {:Select   }:Exit ",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y,TRUE);
        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[0].x,cursorPos[0].y);
        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[1].x,cursorPos[1].y);
        //EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[2].x,cursorPos[2].y);
        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
    }
}

/*-----------------------------------------*/
/* Exported Functions                      */
/*-----------------------------------------*/
BOOL GuiUsbCamOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{   
    param3 = para3;
    nowcursor=0;
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <USBCam> OnDraw (from GuiId = %d, para = %d)\n", oriGuiId, para); 
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
    pGuiGetInterface->setTimeoutFunc(UPDATE_RUNNING_DOT_TIMER, UPDATE_RUNNING_DOT_INTERVAL);      

    EPDSetSleepFunction(FALSE);

    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //CardReaderSetPower(FALSE);
    //
    
    updateBG();
    sysprintf(" [INFO GUI] <USBCam> OnDraw exit: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiUsbCamUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <USBCam> UpdateData\n"); 
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
    return TRUE;
}
BOOL GuiUsbCamKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <USBCam> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <CAM> Key:  ignore...\n");
        return reVal;
    }
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        keyIgnoreFlag=TRUE;
        switch(keyId)
        {
            case GUI_KEYPAD_ONE:
                
                if((nowIndex>0)&&(nowIndex==oldIndex))
                {
                    oldIndex=nowIndex;
                    nowIndex--;
                    reVal = TRUE;
                }
                else if((nowIndex==oldIndex)&&(nowIndex==0))
                {
                    oldIndex=nowIndex;
                    nowIndex=maxSelectItem-1;
                    reVal = TRUE;
                }
                      
            
                /*
                if(nowcursor>0)
                {
                    reVal = TRUE;
                    EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                    nowcursor--;
                    EPDDrawMulti(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                }*/
                break;
                
            case GUI_KEYPAD_TWO:
                
                if((nowIndex<(maxSelectItem-1))&&(nowIndex==oldIndex))
                {
                    oldIndex=nowIndex;
                    nowIndex++;
                    reVal = TRUE;
                }
                else if((nowIndex==oldIndex)&&(nowIndex==maxSelectItem-1))
                {
                    oldIndex=nowIndex;
                    nowIndex=0;
                    reVal = TRUE;
                }
            
                
                /*
                if(nowcursor<MAX_CURSOR-1&&param3==POWER_SET_MODE)
                {
                    reVal = TRUE;
                    EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                    nowcursor++;
                    EPDDrawMulti(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                }*/
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
            
            
                if(nowIndex==0&&param3==POWER_SET_MODE)
                {
                    SetGuiResponseVal('1');
                }
                else if(nowIndex==1&&param3==POWER_SET_MODE)
                {
                    SetGuiResponseVal('2');
                }
            
            
            
                /*
                if(nowcursor==0&&param3==POWER_SET_MODE)
                {
                    //SetGuiResponseVal('1');
                    SetGuiResponseVal('0');
                }
                else if(nowcursor==1&&param3==POWER_SET_MODE)
                {
                    //SetGuiResponseVal('2');
                    SetGuiResponseVal('1');
                }*/
                reVal = TRUE;
                break;
                
            case GUI_KEYPAD_SIX:
                reVal = TRUE;
                SetGuiResponseVal('q');
                break;
            
            /*
        #if(SUPPORT_HK_10_HW)
            case GUI_KEYPAD_QRCODE_ID:
        #else
            case GUI_KEYPAD_CONFIRM_ID:
        #endif
               BuzzerPlay(100, 50, 1, FALSE);
                if(param3==0){
                    SetGuiResponseVal('q');
                    break;
                }
                if(nowcursor==0){
                    SetGuiResponseVal('0');
                }
                else if(nowcursor==1){
                    SetGuiResponseVal('1');
                }
                else if(nowcursor==2){
                    SetGuiResponseVal('q');
                }
                break;
            
            case GUI_KEYPAD_LEFT_ID:
                if(nowcursor>0)
                {
                    BuzzerPlay(100, 50, 1, FALSE);
                    EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                    nowcursor--;
                    EPDDrawMulti(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                }
                break;
            
            case GUI_KEYPAD_RIGHT_ID:
                if(nowcursor<MAX_CURSOR-1)
                {
                    BuzzerPlay(100, 50, 1, FALSE);
                    EPDDrawMulti(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                    nowcursor++;
                    EPDDrawMulti(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,cursorPos[nowcursor].x,cursorPos[nowcursor].y);
                }
                break;
            
            case GUI_KEYPAD_ADD_ID:
                break;
            
            case GUI_KEYPAD_MINUS_ID:
                break;
            */
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
                sysprintf(" [INFO GUI] <USBCam> Key:  not support keyId 0x%02x...\n", keyId); 
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
BOOL GuiUsbCamTimerCallback(uint8_t timerIndex)
{

    //sysprintf(" [INFO GUI] <USBCam> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
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
                EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX ,480-44,200+(50*(oldIndex+1)));
                //select item change to black
                EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,480-44,200+(50*(nowIndex+1)));
                oldIndex=nowIndex;
            }
//                updateData();
            break;
        case UPDATE_RUNNING_DOT_TIMER:
            break;
        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;  
    return TRUE;
}

BOOL GuiUsbCamPowerCallbackFunc(uint8_t type, int flag)
{

    //sysprintf(" [INFO GUI] Standby power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:      
            if(flag == WAKEUP_SOURCE_RTC)  
            {               
                sysprintf(" [INFO GUI] <USBCam> PowerCallbackFunc UPDATE_DATA_TIMER\n");
                powerStatus = FALSE;                 
                pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
            }
            else
            {
                sysprintf(" [INFO GUI] <USBCam> PowerCallbackFunc ignore\n");  
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


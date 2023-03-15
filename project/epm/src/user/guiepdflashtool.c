/**************************************************************************//**
* @file     guinull.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief For EPD Burning Test   
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
#include "epddrv.h"
#include "guimanager.h"
#include "guiepdflashtool.h"
#include "hwtester.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define EXIT_TIMER          GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     ((200)/portTICK_RATE_MS) //portMAX_DELAY
#define UPDATE_DATA_INTERVAL   ((50)/portTICK_RATE_MS)
#define EXIT_INTERVAL          portMAX_DELAY

#define BTM_DISCRIPT_BAR_X 90 //180
#define BTM_DISCRIPT_BAR_Y 700-44
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//static void updateData(void);
static KeyinCallback keyinCallback = NULL;
static ListItem* listItem;

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = TRUE;
static BOOL keyIgnoreFlag = FALSE;
static BOOL keyEnableFlag = TRUE;
static int maxSelectItem = 0;
static int nowIndex=0,oldIndex=0;

static BOOL cleanMsgAfterMove = FALSE;

static BOOL cleanMsgAfterMoveEx = FALSE;

static BOOL setVCOMFlag = FALSE;
static int nowcursor =3,oldcursor=3;
static char deviceID[4];
static BOOL refreshDeviceID=FALSE;

static BOOL setVCOMBGFlag = FALSE;
static BOOL clrVCOMBGFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    //TickType_t tickLocalStart = xTaskGetTickCount();
    //EPDDrawContainByIDPos(TRUE,EPD_PICT_LOADING,475,250);
    EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    EPDDrawStringMax(FALSE,"EPDflashTool",90,28,FALSE);
    //EPDDrawStringMax(FALSE,"xxxxxx Calibration xxxxxx",200,100,FALSE);
    //EPDDrawStringMax(FALSE,"\nx\nx\nx\nx\nx\nx\nx\nx",200,100,FALSE);
    //EPDDrawStringMax(FALSE,"\nx\nx\nx\nx\nx\nx\nx\nx",200+(24*28),100,FALSE);
    //EPDDrawStringMax(FALSE,"\n\n\n\n\n\n\n\n\nxxxxxxxxxxxxxxxxxxxxxxxxx",200,100,FALSE);
    EPDDrawStringMax(FALSE," <>:Move   {:Select    }:Exit ",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y,TRUE);
    
    for(int i = 0; ; i++)
    {//Dont Show Quit
        if(listItem[i].charItem == 0)
        {
            maxSelectItem = i-1;
            break;
        }
    }
    for(int i=0;i<maxSelectItem;i++)
    {
        //EPDDrawStringMax(FALSE,listItem[i].itemName,300+50,100+(44*(2+i)),FALSE);
        //EPDDrawStringMax(FALSE,"-",300,100+(44*(2+i)),TRUE);
        EPDDrawStringMax(FALSE,listItem[i].itemName,90+50,100+(44*(2+i)),FALSE);
        EPDDrawStringMax(FALSE,"-",90,100+(44*(2+i)),TRUE);
    }
    //EPDDrawString(FALSE,"ENABLE  ",300+450,100+(44*(7)));
    //EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,300,100+(44*(2+oldIndex)));
    EPDDrawString(FALSE,":ENABLE  ",90+300,100+(44*(7)));
    EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,90,100+(44*(2+oldIndex)));
}
//static void updateData(void)
//{
//    TickType_t tickLocalStart = xTaskGetTickCount();
//    //sysprintf(" [INFO GUI] <Free> updateData enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
//    
//    //sysprintf(" [INFO GUI] <Free> updateData: [%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - keyStart);
//    //sysDelay(100);
//}

/*-----------------------------------------*/
/* Exported Functions                      */
/*-----------------------------------------*/
BOOL GuiEpdflashToolOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int item, int keyinCallbackFun)
{
    if(reFreshPara == GUI_KEY_ENABLE)
        keyEnableFlag = TRUE;
    else if(reFreshPara == GUI_KEY_DISABLE)
        keyEnableFlag = FALSE;
    else if(reFreshPara == GUI_CLEAN_MESSAGE_ENABLE)
    {
        //cleanMsgAfterMove = TRUE;
        cleanMsgAfterMoveEx = TRUE;
    }
    else if(reFreshPara == GUI_REDRAW_PARA_CONTAIN)
    {
        updateBG();
    }
    else
    {
        //tickStart = xTaskGetTickCount();
        // sysprintf(" [INFO GUI] <Free> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);   
        listItem = (ListItem*)item;
        keyinCallback = (KeyinCallback)keyinCallbackFun;
        
        powerStatus = TRUE;
        pGuiGetInterface = GuiGetInterface();
        pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
        pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
        updateBG();
        pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
        //sysprintf(" [INFO GUI] <Free> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
        //oldIndex = 0;
        //nowIndex = 0;
    
    }
    return TRUE;
}
BOOL GuiEpdflashToolUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更updateScreen
    return TRUE;
}
BOOL GuiEpdflashToolKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <Free> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    //terninalPrintf("now[%d] old[%d]\n",nowIndex,oldIndex);
    BOOL reVal = FALSE; 
    if(!keyEnableFlag)
        return reVal;
    
    if(keyIgnoreFlag)
    {
        //terninalPrintf(" [INFO GUI] <EPDflashTool> Key:  ignore...\n"); 
        return reVal;
    }
    if(keyinCallback == NULL)
    {
        terninalPrintf(" [INFO GUI] <EPDflashTool> KeyCallback:  Function Pointer ERROR!!!\n"); 
        return reVal;
    }
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_ONE:
                //if(nowIndex == 0)
                //{
                    //EPDDrawString(TRUE,"            ",300+400,100+(44*(2)));
                //}
                if(setVCOMFlag)
                {
                    if((nowcursor>0))
                    {
                        nowcursor--;
                        reVal = TRUE;
                    }
                    
                }
                else
                {
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
                }
                break;
            case GUI_KEYPAD_TWO:
                //if(nowIndex == 0)
                //{
                    //EPDDrawString(TRUE,"            ",300+400,100+(44*(2)));
                //}
                if(setVCOMFlag)
                {
                    if((nowcursor<3))
                    {
                        nowcursor++;
                        reVal = TRUE;
                    }
 
                }
                else
                {
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
                }
                break;
            case GUI_KEYPAD_THREE:
                //reVal = TRUE;
                if(setVCOMFlag)
                {   
                    if(deviceID[nowcursor]<'9')
                        deviceID[nowcursor]++;
                    else 
                        deviceID[nowcursor]='0';
                    refreshDeviceID=TRUE;
                    reVal = TRUE; 
                }
                break;
            case GUI_KEYPAD_FOUR:
                //reVal = TRUE;
                if(setVCOMFlag)
                {
                    if(deviceID[nowcursor]>'0')
                    {
                        deviceID[nowcursor]--;
                    }
                    else 
                    {
                        deviceID[nowcursor]='9';
                    }
                    refreshDeviceID=TRUE;
                    reVal = TRUE;
                }
                break;
            case GUI_KEYPAD_FIVE:
                if(setVCOMFlag)
                {
                    setVCOMFlag = FALSE;
                    IT8951SetVCOM(atoi(deviceID));
                    terninalPrintf("%d ",atoi(deviceID));
                    //terninalPrintf("atoi(deviceID) = %d \r\n",atoi(deviceID));
                    SetGuiResponseVal('y');
                    clrVCOMBGFlag = TRUE;

                    reVal = TRUE;
                }
                else if(oldIndex==nowIndex)
                {
                    if((oldIndex == 0) || (oldIndex == 2))
                    {
                        setVCOMFlag = TRUE;
                        sprintf(deviceID,"%04d",IT8951GetVCOM());
                        setVCOMBGFlag = TRUE;
                    }
                    cleanMsgAfterMove = TRUE;
                    keyinCallback(listItem[nowIndex].charItem);
                    reVal = TRUE;
                }
                break;
            case GUI_KEYPAD_SIX:
                if((oldIndex == 0) || (oldIndex == 2))
                {
                    setVCOMFlag = FALSE;
                    clrVCOMBGFlag = TRUE;
                }
                keyinCallback('q');
                //oldIndex = 0;
                //nowIndex = 0;
                reVal = TRUE;
                break;
            case GUI_KEYPAD_REPLACE_BP_ID:
                //EPDReSetBacklightTimeout(portMAX_DELAY);
                //GuiManagerShowScreen(GUI_REPLACE_BP_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
            
            case GUI_KEYPAD_TESTER_ID:
                //EPDReSetBacklightTimeout(portMAX_DELAY);
                //GuiManagerShowScreen(GUI_TESTER_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
            
             case GUI_KEYPAD_TESTER_KEYPAD_ID:
                //EPDReSetBacklightTimeout(portMAX_DELAY);
                //GuiManagerShowScreen(GUI_TESTER_KEYPAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
        }
    }
    else
    {
        
    }
   // setPrintfFlag(FALSE);
    return reVal;
}
BOOL GuiEpdflashToolTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Free> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            if(setVCOMBGFlag)
            {
                setVCOMBGFlag = FALSE;
                //EPDDrawStringMax(FALSE,"                              ",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y,FALSE);
                EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_BLACK_INDEX,80,600-44);
                EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,80,600-44);
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
                EPDDrawStringMax(TRUE,"<\\> Move\n+\\= Change\n{  Save\n}  Exit",80,600-44,TRUE);
            }
            if(clrVCOMBGFlag)
            {
                clrVCOMBGFlag = FALSE;
                //EPDDrawStringMax(FALSE,"         \n           \n        \n        ",80,600-44,FALSE);
                //EPDDrawStringMax(FALSE,"         ",80,600-44,FALSE);
                //EPDDrawStringMax(FALSE,"           ",80,600,FALSE);
                //EPDDrawStringMax(FALSE,"        ",80,600+44,FALSE);
                //EPDDrawStringMax(FALSE,"        ",80,600+44*2,FALSE);
                
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_BLACK_INDEX,80,600-44);
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,80,600-44);
                //EPDDrawStringMax(TRUE," <>:Move   {:Select    }:Exit ",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y,TRUE);
                
            }
            break;
        case UPDATE_DATA_TIMER:
            if(setVCOMFlag){
                //TickType_t tickLocalStart = xTaskGetTickCount();
                //move corsor
                if(nowcursor!=oldcursor){
                    if(oldIndex == 0)
                    {
                        //EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldcursor]-'0'),390+(28*(oldcursor+1)),100+(44*(2)));
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldcursor]-'0'),556+175-25+(28*(oldcursor+1)),100+(44*(2)));

                        //EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),390+(28*(nowcursor+1)),100+(44*(2)));
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),556+175-25+(28*(nowcursor+1)),100+(44*(2)));
                    }
                    else if(oldIndex == 2)
                    {
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldcursor]-'0'),390+(28*(oldcursor+1)),100+(44*(4)));
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),390+(28*(nowcursor+1)),100+(44*(4)));
                        
                    }
                    oldcursor=nowcursor;
                }
                //add or minus number
                if(refreshDeviceID){
                    if(oldIndex == 0)
                    {
                        //EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),390+(28*(nowcursor+1)),100+(44*(2)));
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),556+175-25+(28*(nowcursor+1)),100+(44*(2)));
                    }
                    else if(oldIndex == 2)
                    {   
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),390+(28*(nowcursor+1)),100+(44*(4)));
                    }
                    refreshDeviceID=FALSE;
                }
            }
            else if(oldIndex!=nowIndex)
            {
                
                if(((cleanMsgAfterMove == TRUE)&&((oldIndex == 0) || (oldIndex == 1))) || (cleanMsgAfterMoveEx))
                {
                    cleanMsgAfterMoveEx = FALSE;
                    cleanMsgAfterMove = FALSE;
                    //EPDDrawString(FALSE,"            ",300+400,100+(44*(2)));
                    //EPDDrawString(TRUE,"            ",300+400,100+(44*(3)));
                    //EPDDrawString(FALSE,"            ",90+400,100+(44*(2)));
                    EPDDrawString(FALSE,"               ",550,100+(44*(2)));
                    EPDDrawString(FALSE,"               ",400,100+(44*(3)));
                    EPDDrawString(TRUE,"            ",90+300,100+(44*(4)));
                }
                //deselect item change to white
                //EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX ,300,100+(44*(oldIndex+2)));
                EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX ,90,100+(44*(oldIndex+2)));
                //select item change to black
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,300,100+(44*(nowIndex+2)));
                EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,90,100+(44*(nowIndex+2)));
                oldIndex=nowIndex;
            }
            break;
        case EXIT_TIMER:
            //GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);        
            break;
    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiEpdflashToolPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <Free> power [%d] : flag = %d!!\n", type, flag);
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


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
#include "guiradartool.h"
#include "hwtester.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define EXIT_TIMER          GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   ((50)/portTICK_RATE_MS)
#define EXIT_INTERVAL          portMAX_DELAY

#define BTM_DISCRIPT_BAR_X 180
#define BTM_DISCRIPT_BAR_Y 700-44
#define EPD_HEIGHT  758
#define X_MID_LINE  515 //535
#define Y_MID_LINE  104

#define X_SHIFT  250
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//static void updateData(void);
static KeyinCallback keyinCallback = NULL;
static ListItem* listItem;

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = TRUE;
static BOOL keyIgnoreFlag = FALSE;

static int maxSelectItem = 0;
static int nowIndex=0,oldIndex=0;

static BOOL cleanMsgAfterMove = FALSE;

static uint8_t tempFreshPara;

static int progress;
static char progressStr[50];
static  int VerData;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    if( ( tempFreshPara == GUI_SET_RADARA_PARAMETER ) || ( tempFreshPara == GUI_SET_RADARA_PARAMETER ) )
    {
        EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        if( tempFreshPara == GUI_SET_RADARA_PARAMETER )
            EPDDrawStringMax(FALSE,"Set RadarA Parameter",90,28,FALSE);
        else if( tempFreshPara == GUI_SET_RADARA_PARAMETER )
            EPDDrawStringMax(FALSE,"Set RadarB Parameter",90,28,FALSE);
        
        EPDDrawStringMax(FALSE,"   X :",250,100+(44*1),FALSE);
        EPDDrawStringMax(FALSE,"   Y :",250,100+(44*2),FALSE);
        EPDDrawStringMax(FALSE,"   L :",250,100+(44*3),FALSE);
        EPDDrawStringMax(FALSE,"   W :",250,100+(44*4),FALSE);
        EPDDrawStringMax(FALSE,"   H :",250,100+(44*5),FALSE);
        EPDDrawStringMax(FALSE,"theta:",250,100+(44*6),FALSE);
        EPDDrawStringMax(FALSE,"  phi:",250,100+(44*7),FALSE);
        
        
        
    }
    else if(tempFreshPara == GUI_NEW_RADAR_TEST)
    {
        EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        EPDDrawStringMax(FALSE,"New Radar Test",90,28,FALSE);
        //EPDDrawStringMax(FALSE,"RadarA",190,100,FALSE);
        EPDDrawStringMax(FALSE,"RadarA:",15,100,FALSE);
        EPDDrawStringMax(FALSE,"Material:",15,100+(44*1),FALSE);
        EPDDrawStringMax(FALSE,"Type:",15,100+(44*2),FALSE);
        EPDDrawStringMax(FALSE,"Status:",15,100+(44*3),FALSE);
        EPDDrawStringMax(FALSE,"Result:",15,100+(44*4),FALSE);
        EPDDrawStringMax(FALSE,"Pos x:     y:",15,100+(44*5),FALSE);
        EPDDrawStringMax(FALSE,"Pos z:     R:",15,100+(44*6),FALSE);
        //EPDDrawStringMax(FALSE,"Para X:    Y:",15,100+(44*7),FALSE);
        EPDDrawStringMax(FALSE,"Para X:",15,100+(44*7),FALSE);
        EPDDrawStringMax(FALSE,"Para Y:",15,100+(44*8),FALSE);
        EPDDrawStringMax(FALSE,"Para L:",15,100+(44*9),FALSE);
        EPDDrawStringMax(FALSE,"Para W:",15,100+(44*10),FALSE);
        EPDDrawStringMax(FALSE,"Para H:",15,100+(44*11),FALSE);
        EPDDrawStringMax(FALSE,"Para theta:",15,100+(44*12),FALSE);
        EPDDrawStringMax(FALSE,"Para phi:",15,100+(44*13),FALSE);
        EPDDrawStringMax(FALSE,"Para L/R:",15,100+(44*14),FALSE);
        
        //EPDDrawStringMax(FALSE,"RadarB",190+X_MID_LINE,100,FALSE);
        EPDDrawStringMax(FALSE,"RadarB:",15+X_MID_LINE,100,FALSE);
        EPDDrawStringMax(FALSE,"Material:",15+X_MID_LINE,100+(44*1),FALSE);
        EPDDrawStringMax(FALSE,"Type:",15+X_MID_LINE,100+(44*2),FALSE);
        EPDDrawStringMax(FALSE,"Status:",15+X_MID_LINE,100+(44*3),FALSE);
        EPDDrawStringMax(FALSE,"Result:",15+X_MID_LINE,100+(44*4),FALSE);
        EPDDrawStringMax(FALSE,"Pos x:     y:",15+X_MID_LINE,100+(44*5),FALSE);
        EPDDrawStringMax(FALSE,"Pos z:     R:",15+X_MID_LINE,100+(44*6),FALSE);
        //EPDDrawStringMax(FALSE,"Para X:    Y:",15+X_MID_LINE,100+(44*7),FALSE);
        EPDDrawStringMax(FALSE,"Para X:",15+X_MID_LINE,100+(44*7),FALSE);
        EPDDrawStringMax(FALSE,"Para Y:",15+X_MID_LINE,100+(44*8),FALSE);
        EPDDrawStringMax(FALSE,"Para L:",15+X_MID_LINE,100+(44*9),FALSE);
        EPDDrawStringMax(FALSE,"Para W:",15+X_MID_LINE,100+(44*10),FALSE);
        EPDDrawStringMax(FALSE,"Para H:",15+X_MID_LINE,100+(44*11),FALSE);
        EPDDrawStringMax(FALSE,"Para theta:",15+X_MID_LINE,100+(44*12),FALSE);
        EPDDrawStringMax(FALSE,"Para phi:",15+X_MID_LINE,100+(44*13),FALSE);
        EPDDrawStringMax(FALSE,"Para L/R:",15+X_MID_LINE,100+(44*14),FALSE);
        
        /*  Draw Mid-Line  */
        for(int i=0;i<(EPD_HEIGHT-100);i+=100)
        {
            EPDDrawContainByIDPos(FALSE,EDP_PICT_LINE_V,X_MID_LINE,Y_MID_LINE+i);
        }
        //EPDDrawStringMax(TRUE," }:Exit ",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y+44,TRUE);
        //EPDDrawStringMax(TRUE," }:Exit ",250+X_MID_LINE,BTM_DISCRIPT_BAR_Y+65,TRUE);
        EPDDrawStringMax(TRUE," }:Exit ",300+X_MID_LINE,BTM_DISCRIPT_BAR_Y+65,TRUE);
    }
    else if(tempFreshPara == GUI_NEW_RADARA_OTA)
    {
        //sprintf(progressStr,"%d  percent    ",progress);
        if(progress == 0xFF)
            sprintf(progressStr,"Ver.%d.%d.%d.%d",(VerData & 0xff000000)>>24,(VerData & 0xff0000)>>16,(VerData & 0xff00)>>8,VerData & 0xff); 
        else
        {
            sprintf(progressStr,"%d",progress);
            EPDDrawStringMax(FALSE,"           ",BTM_DISCRIPT_BAR_X + 400,100+(44*(2+5)),FALSE);
            EPDDrawStringMax(FALSE,"percent    ",BTM_DISCRIPT_BAR_X + 500,100+(44*(2+5)),FALSE);
        }
        EPDDrawStringMax(TRUE,progressStr,BTM_DISCRIPT_BAR_X + 400,100+(44*(2+5)),TRUE);
    }
    else if(tempFreshPara == GUI_NEW_RADARB_OTA)
    {
        //sprintf(progressStr,"%d  percent    ",progress);
        if(progress == 0xFF)
            sprintf(progressStr,"Ver.%d.%d.%d.%d",(VerData & 0xff000000)>>24,(VerData & 0xff0000)>>16,(VerData & 0xff00)>>8,VerData & 0xff); 
        else
        {
            sprintf(progressStr,"%d",progress);
            EPDDrawStringMax(FALSE,"           ",BTM_DISCRIPT_BAR_X + 400,100+(44*(2+6)),FALSE);
            EPDDrawStringMax(FALSE,"percent    ",BTM_DISCRIPT_BAR_X + 500,100+(44*(2+6)),FALSE);
        }
        EPDDrawStringMax(TRUE,progressStr,BTM_DISCRIPT_BAR_X + 400,100+(44*(2+6)),TRUE);
    }
    else
    {
        //TickType_t tickLocalStart = xTaskGetTickCount();
        //EPDDrawContainByIDPos(TRUE,EPD_PICT_LOADING,475,250);
        EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        EPDDrawStringMax(FALSE,"RadarTool",90,28,FALSE);
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
            EPDDrawStringMax(FALSE,listItem[i].itemName,BTM_DISCRIPT_BAR_X+50,100+(44*(2+i)),FALSE);
            EPDDrawStringMax(FALSE,"-",BTM_DISCRIPT_BAR_X,100+(44*(2+i)),TRUE);
           // EPDDrawStringMax(FALSE,listItem[i].itemName,300+50,100+(44*(2+i)),FALSE);
           // EPDDrawStringMax(FALSE,"-",300,100+(44*(2+i)),TRUE);
        }
        //EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,300,100+(44*(2+oldIndex)));
        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,BTM_DISCRIPT_BAR_X,100+(44*(2+oldIndex)));
    }
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
BOOL GuiRadarToolOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int item, int keyinCallbackFun)
{
    //if(reFreshPara == GUI_CLEAN_MESSAGE_ENABLE)
    //    cleanMsgAfterMove = TRUE;
    if(reFreshPara == GUI_SET_RADARA_PARAMETER)
    {
        tempFreshPara = GUI_SET_RADARA_PARAMETER;
        updateBG();
    }
    else if(reFreshPara == GUI_SET_RADARB_PARAMETER)
    {
        tempFreshPara = GUI_SET_RADARB_PARAMETER;
        updateBG();
    }
    else if(reFreshPara == GUI_NEW_RADAR_TEST)
    {
        tempFreshPara = GUI_NEW_RADAR_TEST;
        updateBG();
    }
    else if(reFreshPara == GUI_NEW_RADARA_OTA)
    {
        tempFreshPara = GUI_NEW_RADARA_OTA;
        progress = item;
        if(progress == 0xFF)
            VerData = keyinCallbackFun;
        updateBG();
    }
    else if(reFreshPara == GUI_NEW_RADARB_OTA)
    {
        tempFreshPara = GUI_NEW_RADARB_OTA;
        progress = item;
        if(progress == 0xFF)
            VerData = keyinCallbackFun;
        updateBG();
    }
    else
    {
        tempFreshPara = GUI_REDRAW_PARA_REFRESH;
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
    }
    return TRUE;
}
BOOL GuiRadarToolUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更updateScreen
    return TRUE;
}
BOOL GuiRadarToolKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <Free> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    //terninalPrintf("now[%d] old[%d]\n",nowIndex,oldIndex);
    BOOL reVal = FALSE; 
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
                if(!GetTesterFlag())
                {
                    /*
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(8)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(9)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(10)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(11)));
                    EPDDrawString(TRUE,"            ",700,100+(44*(nowIndex+2)));
                    */
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
                if(!GetTesterFlag())
                {
                    /*
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(8)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(9)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(10)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(11)));
                    EPDDrawString(TRUE,"            ",700,100+(44*(nowIndex+2)));
                    */
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
                break;
            case GUI_KEYPAD_FOUR:
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_FIVE:
                if(oldIndex==nowIndex)
                {
                    cleanMsgAfterMove = TRUE;
                    keyinCallback(listItem[nowIndex].charItem);
                    reVal = TRUE;
                }
                break;
            case GUI_KEYPAD_SIX:
                keyinCallback('q');
                oldIndex = 0;
                nowIndex = 0;
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
BOOL GuiRadarToolTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Free> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            break;
        case UPDATE_DATA_TIMER:
            if(oldIndex!=nowIndex)
            {
                /*
                if(cleanMsgAfterMove)
                {
                    cleanMsgAfterMove = FALSE;
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(8)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(9)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(10)));
                    EPDDrawString(FALSE,"                                 ",50,100+(44*(11)));
                    EPDDrawString(FALSE,"            ",700,100+(44*(2)));
                    EPDDrawString(TRUE,"            ",700,100+(44*(3)));
                    //EPDDrawString(TRUE,"            ",700,100+(44*(oldIndex+2)));
                }
                */
                //deselect item change to white
                //EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX ,300,100+(44*(oldIndex+2)));
                //select item change to black
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,300,100+(44*(nowIndex+2)));
                
                
                //deselect item change to white
                EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX ,BTM_DISCRIPT_BAR_X,100+(44*(oldIndex+2)));
                //select item change to black
                EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,BTM_DISCRIPT_BAR_X,100+(44*(nowIndex+2)));
                
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

BOOL GuiRadarToolPowerCallbackFunc(uint8_t type, int flag)
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


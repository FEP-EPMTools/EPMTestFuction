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
#include "halinterface.h"
#include "guifiledownload.h"
#include "epddrv.h"
#include "guimanager.h"
#include "tarifflib.h"
#include "meterdata.h"
#include "hwtester.h"
#include "guiversion.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define EXIT_TIMER          GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   portMAX_DELAY
#define EXIT_INTERVAL          1000/portTICK_RATE_MS //portMAX_DELAY
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//static void updateData(void);

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = TRUE;
static BOOL keyIgnoreFlag = FALSE;


static int waitCounter = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
//static void updateBG(void)
//{
//    TickType_t tickLocalStart = xTaskGetTickCount();
//    //sysprintf(" [INFO GUI] <Free> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
//    // sysprintf(" [INFO GUI] <Free> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);    
// 
//}
//static void updateData(void)
//{
//    TickType_t tickLocalStart = xTaskGetTickCount();
//    //sysprintf(" [INFO GUI] <Free> updateData enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
//    
//    //sysprintf(" [INFO GUI] <Free> updateData: [%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - keyStart);
//    //sysDelay(100);

//}

static void updateBG(void)
{
    //TickType_t tickLocalStart = xTaskGetTickCount();
    //show BG and item
    //Draw BG
    //EPDDrawMulti(FALSE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    
    EPDDrawContainByIDPos(TRUE,EPD_PICT_LOADING,500,250);
    EPDDrawContainByIDPos(FALSE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    //TITLE
 /*   EPDDrawStringMax(FALSE,"Tool",X_HEAD_TITLE,Y_HEAD_TITLE,FALSE);
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
    EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,150,104+(nowIndex*STRING_HEIGHT)); */
    //terninalPrintf("======== Tool ========[INFO GUI] <GuiOnDraw>  [%d0'ms].\n", xTaskGetTickCount() - tickLocalStart);
}





/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiVersionOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    if((reFreshPara == GUI_TIMER_ENABLE) || (reFreshPara == GUI_TIMER_DISABLE))
    {
        
        if(reFreshPara == GUI_TIMER_ENABLE)
            pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
        else
            pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, portMAX_DELAY);
        
    }
    else
    {
        //    tickStart = xTaskGetTickCount();
        // sysprintf(" [INFO GUI] <Free> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);   
        powerStatus = TRUE;
        pGuiGetInterface = GuiGetInterface();
        pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
        pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
        
        pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
        
        
        pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
        
        //pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
        
        
        EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
        
        
        //EPDDrawString(FALSE,"Version Tool",0,0);

        
        //sysprintf(" [INFO GUI] <Free> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    }
    return TRUE;
}
BOOL GuiVersionUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更updateScreen
    return TRUE;
}
BOOL GuiVersionKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <Free> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <Free> Key:  ignore...\n"); 
        return reVal;
    }
    //pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_LEFT_ID:
                SetGuiResponseVal('p');
                reVal = TRUE;
                break;
            case GUI_KEYPAD_RIGHT_ID:
                SetGuiResponseVal('n');
                reVal = TRUE;
                break;
            case GUI_KEYPAD_ADD_ID:
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_MINUS_ID:
                //SetGuiResponseVal('q');
                //reVal = TRUE;
                break;
        #if(SUPPORT_HK_10_HW)
            case GUI_KEYPAD_QRCODE_ID:
                SetGuiResponseVal('q');
                reVal = TRUE;
                break;
        #else
            case GUI_KEYPAD_CONFIRM_ID:
        #endif
                //reVal = TRUE;
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
BOOL GuiVersionTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Free> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    keyIgnoreFlag = TRUE;

    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:

            break;
        case UPDATE_DATA_TIMER:
            


        
        
            
        
            break;
        case EXIT_TIMER:
            //GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0); 


            switch(waitCounter)
            {
            case 3:
                waitCounter=-1;
                EPDDrawString(TRUE,"        ",500,50);
                break;
            case 0:
                EPDDrawString(TRUE,".       ",500,50);
                break;
            case 1:
                EPDDrawString(TRUE,"..      ",500,50);
                break;
            case 2:
                EPDDrawString(TRUE,"...     ",500,50);
                break;
            }
            waitCounter++;
            
        
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiVersionPowerCallbackFunc(uint8_t type, int flag)
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


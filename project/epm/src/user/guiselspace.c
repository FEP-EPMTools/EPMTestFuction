/**************************************************************************//**
* @file     guiselspace.c
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
#include "guiselspace.h"
#include "epddrv.h"
#include "guimanager.h"
#include "paralib.h"
#include "meterdata.h"
#include "cardreader.h"
#include "tarifflib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define EXIT_TIMER          GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   (2000/portTICK_RATE_MS)
#define EXIT_INTERVAL          (6000/portTICK_RATE_MS)
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static void updateData(void);
static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static TickType_t tickStart = 0, keyStart;
static uint8_t selOriIndex = 3;

static BOOL keyIgnoreFlag = FALSE;
static uint8_t refreshType = GUI_REDRAW_PARA_NONE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <SelSpace> updateBG enter: cost ticks = [%d], refreshType = %d\n", xTaskGetTickCount() - tickStart, refreshType);   
    
    switch(refreshType)
    {
        case GUI_REDRAW_PARA_NORMAL:
            sysprintf(" [INFO GUI] <SelSpace> updateBG refreshType <GUI_REDRAW_PARA_NORMAL>\n");  
            EPDDrawItem(FALSE, selOriIndex, GetMeterData()->currentSelSpace); 
            EPDDrawDepositTime(FALSE, selOriIndex, GetMeterData()->currentSelSpace); 
            break;
        //case GUI_REDRAW_PARA_REFRESH:
       //     sysprintf(" [INFO GUI] <SelSpace> updateBG refreshTypeERROR :refreshType = %d\n", refreshType);   
        //    break;
        case GUI_REDRAW_PARA_CONTAIN:
            sysprintf(" [INFO GUI] <SelSpace> updateBG refreshType <GUI_REDRAW_PARA_CONTAIN>\n");  
            EPDDrawContainBG(FALSE);
            EPDDrawAllDepositTime(FALSE);
            EPDDrawItem(FALSE, selOriIndex, GetMeterData()->currentSelSpace); 
            EPDDrawDepositTime(FALSE, selOriIndex, GetMeterData()->currentSelSpace); 
            break;
        default:
            sysprintf(" [INFO GUI] <SelSpace> updateBG refreshType ERROR :refreshType = %d\n", refreshType);   
            break;
                
    }
    refreshType = GUI_REDRAW_PARA_NONE;    
    selOriIndex = GetMeterData()->currentSelSpace;
    //updateData();
    UpdateClock(TRUE, TRUE);
    sysprintf(" [INFO GUI] <SelSpace> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);     
}
static void updateData(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <SelSpace> updateData enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart); 
    if(selOriIndex != GetMeterData()->currentSelSpace)
    {
        EPDDrawItem(FALSE, selOriIndex, GetMeterData()->currentSelSpace);
        EPDDrawDepositTime(TRUE, selOriIndex, GetMeterData()->currentSelSpace);
        //sysDelay(100);
        selOriIndex = GetMeterData()->currentSelSpace;
        
    }
    else
    {
         UpdateClock(FALSE, TRUE);
    }
    
    sysprintf(" [INFO GUI] <SelSpace> updateData: Local:[%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - keyStart);  
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiSelSpaceOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <SelSpace> OnDraw (from GuiId = %d, para = %d)\n", oriGuiId, para);   
    powerStatus = FALSE;
    if(oriGuiId == GUI_STANDBY_ID)
    {
        selOriIndex = 0;//GetMeterData()->currentSelSpace;
    }
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL);     
    refreshType = para;
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
    CardReaderSetPower(TRUE);
    TariffUpdateCurrentTariffData();  
    sysprintf(" [INFO GUI] <SelSpace> OnDraw exit: cost ticks = %d(para = %d, refreshType = %d)\n", xTaskGetTickCount() - tickStart, para, refreshType);
    
    return TRUE;
}
BOOL GuiSelSpaceKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <SelSpace> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <SelSpace> Key:  ignore...\n"); 
        return reVal;
    }
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_LEFT_ID:                
                if(selOriIndex != GetMeterData()->currentSelSpace)
                    break;
                keyStart = xTaskGetTickCount();
                selOriIndex = GetMeterData()->currentSelSpace;
                UpdateMeterCurrentSelSpace(-1, FALSE);
                pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_RIGHT_ID:                
                if(selOriIndex != GetMeterData()->currentSelSpace)
                    break;
                keyStart = xTaskGetTickCount();
                selOriIndex = GetMeterData()->currentSelSpace;
                UpdateMeterCurrentSelSpace(1, FALSE);
                pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_ADD_ID:
            case GUI_KEYPAD_MINUS_ID:
                GuiManagerShowScreen(GUI_SEL_TIME_ID, GUI_REDRAW_PARA_DONT_CARE, 0, 0);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_CONFIRM_ID:
                break;
        }
    }
    else
    {
        
    }    
    return reVal;
}
BOOL GuiSelSpaceTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <SelSpace> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            updateBG();
            break;
        case UPDATE_DATA_TIMER:
            updateData();
            break;
        case EXIT_TIMER:
            GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_NORMAL, 0, 0);
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiSelSpacePowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <SelSpace> power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:
            //pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


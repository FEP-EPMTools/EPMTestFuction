/**************************************************************************//**
* @file     guiseltime.c
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
#include "guiseltime.h"
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
static BOOL keyIgnoreFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <SelTime> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);   
    EPDDrawContainSelTime(FALSE);
    
    EPDDrawTime(FALSE, GetMeterData()->currentSelTime);  
    EPDDrawCost(TRUE, GetMeterData()->currentSelCost);   
    //UpdateClock(FALSE, TRUE);   
    //updateData();    
    sysprintf(" [INFO GUI] <SelTime> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);    
 
}
static void updateData(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <SelTime> updateData enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
    
    EPDDrawTime(FALSE, GetMeterData()->currentSelTime);  
    EPDDrawCost (TRUE, GetMeterData()->currentSelCost);   
    UpdateClock(FALSE, TRUE);    
    sysprintf(" [INFO GUI] <SelTime> updateData: [%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - keyStart);
    //sysDelay(100);
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiSelTimeOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <SelTime> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);   
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    
    //UpdateMeterDepositTime(SEL_TIME_INTERVAL_TIME);
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更載畫面
    CardReaderSetPower(TRUE);
    //sysprintf(" [INFO GUI] <SelTime> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiSelTimeKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <SelTime> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <SelTime> Key:  ignore...\n"); 
        return reVal;
    }
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_LEFT_ID:
                UpdateMeterCurrentSelSpace(-1, FALSE);
                GuiManagerShowScreen(GUI_SEL_SPACE_ID, GUI_REDRAW_PARA_CONTAIN, 0, 0);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_RIGHT_ID:
                UpdateMeterCurrentSelSpace(1, FALSE);
                GuiManagerShowScreen(GUI_SEL_SPACE_ID, GUI_REDRAW_PARA_CONTAIN, 0, 0);
                reVal = TRUE;
                break;
            case GUI_KEYPAD_ADD_ID:
                keyStart = xTaskGetTickCount();
                //sysprintf(" [INFO GUI] <SelTime> GUI_KEYPAD_ADD_ID:  %d vs %d...\n", GetMeterData()->currentSelTime, (TariffGetCurrentTariffType()->maxtime - TariffGetCurrentTariffType()->timeunit)); 
                if(GetMeterData()->currentSelTime < (TariffGetCurrentTariffType()->maxtime - TariffGetCurrentTariffType()->timeunit))
                {
                    reVal = UpdateMeterDepositTime(GetMeterData()->currentSelTime + TariffGetCurrentTariffType()->timeunit);
                    if(reVal)
                        pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
                }
                break;
            case GUI_KEYPAD_MINUS_ID:
                keyStart = xTaskGetTickCount();
                //sysprintf(" [INFO GUI] <SelTime> GUI_KEYPAD_MINUS_ID:  %d vs %d...\n", GetMeterData()->currentSelTime, TariffGetCurrentTariffType()->timeunit); 
                if(GetMeterData()->currentSelTime > TariffGetCurrentTariffType()->timeunit)
                {
                    reVal = UpdateMeterDepositTime(GetMeterData()->currentSelTime - TariffGetCurrentTariffType()->timeunit);
                    if(reVal)
                        pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
                }
                break;
            case GUI_KEYPAD_CONFIRM_ID:
                keyStart = xTaskGetTickCount();
                if(GetMeterData()->currentSelCost != 0)
                {
                    if(CardReaderGetBootedStatus())
                    {                        
                        GuiManagerShowScreen(GUI_DEPOSIT_ID, GUI_REDRAW_PARA_DONT_CARE, 0, 0);
                    }
                    else
                    {
                        GuiManagerShowScreen(GUI_READER_INIT_ID, GUI_REDRAW_PARA_DONT_CARE, 0, 0); 
                    }
                    reVal = TRUE;
                }
                else
                {
                    
                }
                break;
        }
    }
    else
    {
        
    }
    return reVal;
}
BOOL GuiSelTimeTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <SelTime> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
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
            GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);        
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiSelTimePowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <SelTime> power [%d] : flag = %d!!\n", type, flag);
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


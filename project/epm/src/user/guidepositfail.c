/**************************************************************************//**
* @file     guidepositfail.c
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
#include "guidepositfail.h"
#include "epddrv.h"
#include "guimanager.h"
#include "paralib.h"
#include "meterdata.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER         GUI_TIME_0_INDEX
//#define CHECK_READER_TIMER      GUI_TIME_1_INDEX
#define EXIT_TIMER              GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
//#define UPDATE_DATA_INTERVAL (500/portTICK_RATE_MS)
#define EXIT_INTERVAL          (3000/portTICK_RATE_MS)
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;

//static uint16_t errorCode = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <DepositFail> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);   
    EPDDrawContainByID(TRUE, EPD_PICT_CONTAIN_DEPOSIT_FAIL_INDEX); 
    sysprintf(" [INFO GUI] <DepositFail> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);  
}


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiDepositFailOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <DepositFail> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3); 
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    //pGuiGetInterface->setTimeoutFunc(CHECK_READER_TIMER, UPDATE_DATA_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL);     
    
    //errorCode = para2;
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //sysprintf(" [INFO GUI] <DepositFail> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiDepositFailKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <DepositFail> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <DepositFail> Key:  ignore...\n"); 
        return reVal;
    }
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

                break;
            case GUI_KEYPAD_MINUS_ID:
                break;
            case GUI_KEYPAD_CONFIRM_ID:
                break;
        }
    }
    else
    {
        
    }
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 

    return reVal;
}
BOOL GuiDepositFailTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <DepositFail> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            updateBG();
            break;
        //case CHECK_READER_TIMER:
            //checkReader();
            //break;
        case EXIT_TIMER:
            GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);        
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiDepositFailPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <DepositFail> power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:
            //pGuiGetInterface->runTimeoutFunc(CHECK_READER_TIMER);
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


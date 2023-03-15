/**************************************************************************//**
* @file     guireaderinit.c
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
#include "guireaderinit.h"
#include "epddrv.h"
#include "guimanager.h"
#include "paralib.h"
#include "meterdata.h"
#include "cardreader.h"
#include "buzzerdrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER         GUI_TIME_0_INDEX
#define CHECK_READER_TIMER      GUI_TIME_1_INDEX
#define EXIT_TIMER              GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define CHECK_READER_INTERVAL  (500/portTICK_RATE_MS)
#define EXIT_INTERVAL          (25000/portTICK_RATE_MS)
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static void checkReader(void);

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateBG(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    //sysprintf(" [INFO GUI] <ReaderInit> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);   
    EPDDrawContainByID(TRUE, EPD_PICT_CONTAIN_READER_INIT_INDEX); 
    sysprintf(" [INFO GUI] <ReaderInit> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);  
 
}

static void checkReader(void)
{
    TickType_t tickLocalStart = xTaskGetTickCount();   
    //sysprintf("!");
    if(CardReaderGetBootedStatus())
    {                        
        BuzzerPlay(50, 0, 1, TRUE);
        GuiManagerShowScreen(GUI_DEPOSIT_ID, GUI_REDRAW_PARA_DONT_CARE, 0, 0);
    }
    //sysprintf(" [INFO GUI] <ReaderInit> updateData: [%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiReaderInitOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <ReaderInit> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3); 
    powerStatus = FALSE;
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(CHECK_READER_TIMER, CHECK_READER_INTERVAL);
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL);     
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    //CardReaderSetPower(TRUE);
    //sysprintf(" [INFO GUI] <ReaderInit> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiReaderInitKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <ReaderInit> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <ReaderInit> Key:  ignore...\n"); 
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
    //if(reVal)
    //    keyIgnoreFlag = TRUE;
    return reVal;
}
BOOL GuiReaderInitTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <ReaderInit> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    //keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            keyIgnoreFlag = TRUE;
            updateBG();
            break;
        case CHECK_READER_TIMER:
            checkReader();
            break;
        case EXIT_TIMER:
            keyIgnoreFlag = TRUE;
            EPDDrawContainByID(TRUE, EPD_PICT_CONTAIN_READER_INIT_FAIL_INDEX); 
            vTaskDelay(5000/portTICK_RATE_MS);
            GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);        
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiReaderInitPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <ReaderInit> power [%d] : flag = %d!!\n", type, flag);
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


/**************************************************************************//**
* @file     timerdrv.c
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
#include "timerdrv.h"
#include "powerdrv.h"
#include "epddrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static TimerInterface* pTimerGetInterface = NULL;
static KeyInterface* pKeyGetInterface = NULL;

static guiTimerCallbackFunc pGuiTimerCallbackFunc = NULL;
static guiKeyCallbackFunc pGuiKeyCallbackFunc = NULL;

static UserGuiInstance* pUserGuiInstance = NULL;

static BOOL guiDrvIgnoreRun = FALSE;

static BOOL guiDrvCheckStatus(int flag);
static BOOL guiDrvPreOffCallback(int flag);
static BOOL guiDrvOffCallback(int flag);
static BOOL guiDrvOnCallback(int flag);
static powerCallbackFunc guiDrvPowerCallabck = {" [GUIDrv] ", guiDrvPreOffCallback, guiDrvOffCallback, guiDrvOnCallback, guiDrvCheckStatus};

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL guiDrvPreOffCallback(int flag)
{
    guiDrvIgnoreRun = TRUE;
    if(pUserGuiInstance != NULL)
        return pUserGuiInstance->guiInstance->powerCallback(GUI_POWER_PREV_OFF_INDEX, flag);
    else
        return FALSE;
}
static BOOL guiDrvOffCallback(int flag)
{
    if(pKeyGetInterface != NULL)
    {
        pKeyGetInterface->setPowerFunc(FALSE);
    }
    if(pUserGuiInstance != NULL)
        return pUserGuiInstance->guiInstance->powerCallback(GUI_POWER_OFF_INDEX, flag);
    else
        return FALSE;    
}
static BOOL guiDrvOnCallback(int flag)
{
    guiDrvIgnoreRun = FALSE;
    if(pKeyGetInterface != NULL)
    {
        pKeyGetInterface->setPowerFunc(TRUE);
    }
    
    if(pUserGuiInstance != NULL)
        return pUserGuiInstance->guiInstance->powerCallback(GUI_POWER_ON_INDEX, flag);
    else
        return FALSE; 
}
static BOOL guiDrvCheckStatus(int flag)
{
    if(pUserGuiInstance != NULL)
        return pUserGuiInstance->guiInstance->powerCallback(GUI_POWER_STATUS_INDEX, flag);
    else
        return FALSE;  
}
static BOOL GUITimerCallbackFunc(uint8_t timerIndex) 
{
    //sysprintf("\r\n [ INFO GUI Timer] Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    if(guiDrvIgnoreRun)
    {
        sysprintf("GUITimerCallbackFunc : ignore!!\n");
        return FALSE;
    }
    
    if(pGuiTimerCallbackFunc != NULL)
        return pGuiTimerCallbackFunc(timerIndex);
    else
        return FALSE;
}
static BOOL GUIKeyCallbackFunc(uint8_t keyId, uint8_t downUp) 
{
    //sysprintf(" [ INFO GUI Key] :  keyId = %d, downUp = %d\n", keyId, downUp);
    if(guiDrvIgnoreRun)
    {
        sysprintf("GUITimerCallbackFunc : ignore!!\n");
        return FALSE;
    }
    if(pGuiKeyCallbackFunc != NULL)
    {
        //BOOL reVal = ;        
        return pGuiKeyCallbackFunc(keyId, downUp);//reVal;
    }
    else
    {
        return FALSE;
    }
}
/*------------------------------------------*/
/* Exported Functions                       */
/*------------------------------------------*/
BOOL GUIDrvInit(BOOL testModeFlag)
{
    sysprintf("GUIDrvInit!!\n");
    
    PowerRegCallback(&guiDrvPowerCallabck);
    
    pKeyGetInterface = KeyGetInterface();
    if(pKeyGetInterface == NULL)
    {
        sysprintf("GUIDrvInit ERROR (pKeyGetInterface == NULL)!!\n");
        return FALSE;
    }
    if(pKeyGetInterface->initFunc() == FALSE)
    {
        sysprintf("GUIDrvInit ERROR (pKeyGetInterface initFunc false)!!\n");
        return FALSE;
    }
    pKeyGetInterface->setCallbackFunc(GUIKeyCallbackFunc);
    
    pTimerGetInterface = TimerGetInterface();
    if(pTimerGetInterface == NULL)
    {
        sysprintf("GUIDrvInit ERROR (pTimerGetInterface == NULL)!!\n");
        return FALSE;
    }
    if(pTimerGetInterface->initFunc() == FALSE)
    {
        sysprintf("GUIDrvInit ERROR (pTimerGetInterface initFunc false)!!\n");
        return FALSE;
    }
    pTimerGetInterface->setCallbackFunc(GUITimerCallbackFunc);
    return TRUE;
}

void GuiSetKeyCallbackFunc(guiKeyCallbackFunc callback)
{
    pGuiKeyCallbackFunc = callback;
}
void GuiSetTimerCallbackFunc(guiTimerCallbackFunc callback)
{
    pGuiTimerCallbackFunc = callback;
}
void GuiSetTimeout(uint8_t timerIndex, TickType_t time)
{
    pTimerGetInterface->setTimeoutFunc(timerIndex, time);
}
void GuiRunTimeoutFunc(uint8_t timerIndex)
{
    pTimerGetInterface->runFunc(timerIndex);
}


BOOL GuiSetInstance(UserGuiInstance* instance, uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    sysprintf("  ==> GuiSetInstance\r\n");
    TimerAllStop(); 
    pGuiKeyCallbackFunc = NULL;
    pGuiTimerCallbackFunc = NULL;    
    
    pUserGuiInstance = instance;   
    GuiSetKeyCallbackFunc(pUserGuiInstance->guiInstance->keyCallback);
    GuiSetTimerCallbackFunc(pUserGuiInstance->guiInstance->timerCallback);    
    pUserGuiInstance->guiInstance->onDraw(oriGuiId, reFreshPara, para2, para3);
    
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


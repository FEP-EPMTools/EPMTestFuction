/**************************************************************************//**
* @file     halinterface.c
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
#include "gpio.h"
#include "uart.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "halinterface.h"
#include "keydrv.h"
#include "timerdrv.h"
#include "guidrv.h"
#include "epmreader.h"
#include "uvcdrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//************************  KEY  ************************//
static KeyInterface mKeyInterface[] = {{KeyDrvInit, KeyDrvSetCallbackFunc, KeyDrvSetPowerFunc}};
//************************  TIMER  ************************//
static TimerInterface mTimerInterface[] = {{TimerDrvInit, TimerSetTimeout, TimerRun, TimerSetCallback}};

//************************  GUI  ************************//
static GuiInterface mGuiInterface[] = {{GUIDrvInit,
                                        GuiSetKeyCallbackFunc,
                                        GuiSetTimerCallbackFunc, 
                                        GuiSetTimeout,
                                        GuiRunTimeoutFunc}};

//************************  TSReader  ************************//
static TSReaderInterface mTSReaderInterface[] = {{EPMReaderInit, EPMReaderSetPower, EPMReaderBreakCheckReader, EPMReaderCheckReader, EPMReaderProcess, EPMReaderProcessCN, EPMReaderGetBootedStatus, EPMReaderSignOnProcess, EPMReaderSaveFile, EPMReaderSaveFilePure}};

//************************  Camera  ************************//
static CameraInterface mCameraInterface[] = {//{PCT08DrvInit, PCT08TakePhoto, PCT08SetPower},
                                             //   {ZM460DrvInit, ZM460TakePhoto, ZM460SetPower},
                                                {UVCDrvInit, UVCTakePhoto, UVCSetPower, UVCDrvInitBurning}
                                                };


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
//************************  KEY  ************************//
KeyInterface* KeyGetInterface(void)
{    
    return mKeyInterface;
}
//************************  TIMER  ************************//
TimerInterface* TimerGetInterface(void)
{    
    return mTimerInterface;
}
//************************  GUI  ************************//
GuiInterface* GuiGetInterface(void)
{
    return mGuiInterface;
}

//************************  TSReader  ************************//
TSReaderInterface* TSReaderGetInterface(uint8_t index)
{
     if(index < TSREADER_HALINTERFACE_NUM)
    {        
        return &mTSReaderInterface[index];
    }
    else
    {
        return NULL;
    }
}
//************************  Camera  ************************//
CameraInterface* CameraGetInterface(uint8_t index)
{
     if(index < CAMERA_HALINTERFACE_NUM)
    {        
        return &mCameraInterface[index];
    }
    else
    {
        return NULL;
    }
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


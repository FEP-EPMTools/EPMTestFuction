/**************************************************************************//**
* @file     keydrv.c
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
#include "keydrv.h"
#include "interface.h"
#include "buzzerdrv.h"
#include "halinterface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define KEY_DEVICE_NUM  2
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint8_t deviceInterfaceIndex[KEY_DEVICE_NUM] = {KEY_HARDWARE_PCF8885_INTERFACE_INDEX, KEY_HARDWARE_DIP_INTERFACE_INDEX};
static KeyHardwareInterface* pKeyHardwareInterface[KEY_DEVICE_NUM] = {NULL, NULL};
static keyCallbackFunc pKeyCallbackFunc = NULL;
static uint8_t keyDrvMode = KEY_DRV_MODE_NORMAL_INDEX;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL KeyCallbackFunc(uint8_t keyId, uint8_t downUp)
{
    BOOL reVal = FALSE;
    //sysprintf("\r\n *** KeyCallbackFunc: keyId = %d, downUp = %d ***\n", keyId, downUp);
    //BuzzerPlay((keyId+1)*100, (keyId+1)*10, downUp, TRUE);
    //BuzzerPlay(100*downUp, 50, keyId+1, FALSE); 
    switch(keyDrvMode)
    {
        case KEY_DRV_MODE_NORMAL_INDEX:
            if(pKeyCallbackFunc != NULL)
            {
                if(pKeyCallbackFunc(keyId, downUp))
                {
                   BuzzerPlay(100, 50, 1, FALSE);
                    reVal = TRUE;
                }
            }
            break;
        case KEY_DRV_MODE_TEST_INDEX:
            if(pKeyCallbackFunc != NULL)
            {
                if(pKeyCallbackFunc(keyId, downUp))
                {										
                    //BuzzerPlay(100, 50, 1, FALSE);
                    reVal = TRUE;
                }
            }
        
						/*
            if(downUp == KEY_HARDWARE_DOWN_EVENT)
            {
               BuzzerPlay(100, 50, 1, FALSE);
            }
            else
            {
               BuzzerPlay(50, 50, 1, FALSE);
            }
						*/
            break;
    }
        
    

    return reVal;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL KeyDrvInit(void)
{
    int i; 
    for(i = 0; i<KEY_DEVICE_NUM; i++)
    { 
        pKeyHardwareInterface[i] = KeyHardwareGetInterface(deviceInterfaceIndex[i]);
        if(pKeyHardwareInterface[i] == NULL)
        {
            sysprintf("KeyDrvInit ERROR (pKeyHardwareInterface == NULL)!!\n");
            return FALSE;
        }
        if(pKeyHardwareInterface[i]->initFunc(FALSE) == FALSE)
        {
            sysprintf("KeyDrvInit ERROR (initFunc false)!!\n");
            return FALSE;
        }
        pKeyHardwareInterface[i]->setCallbackFunc(KeyCallbackFunc);
    }
    return TRUE;
}

void KeyDrvSetCallbackFunc(keyCallbackFunc func)
{
    pKeyCallbackFunc = func;
}

void KeyDrvSetPowerFunc(BOOL powerFlag)
{
    int i;
    for(i = 0; i<KEY_DEVICE_NUM; i++)
    { 
        if(pKeyHardwareInterface[i]->setPowerFunc != NULL)
            pKeyHardwareInterface[i]->setPowerFunc(powerFlag);
    }
}

void KeyDrvSetMode(uint8_t mode)
{
    keyDrvMode = mode;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


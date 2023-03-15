/**************************************************************************//**
* @file     loradrv.c
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
#include "adc.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "loradrv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define LORA_UART   UART_8_INTERFACE_INDEX

#define LORA_BUSY_PORT  GPIOH
#define LORA_BUSY_PIN   BIT7
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL LoraWaitForReady(void)
{
    int timers = 2000;
    TickType_t mTick = xTaskGetTickCount();
    while(GPIO_ReadBit(LORA_BUSY_PORT,LORA_BUSY_PIN)==0)
    {
        //sysprintf("{%d}", 500-timers);
        if(timers-- == 0)
        {
            sysprintf("\r\n ##  LoraWaitForReady timeout  ##\n");            
            return FALSE;
        }
        vTaskDelay(1/portTICK_RATE_MS); 
        //sysDelay(1);
    };
    //if(xTaskGetTickCount() != mTick)
    //    sysprintf("\r\nLoraWaitForReady {%d}\r\n", xTaskGetTickCount() - mTick);
    return TRUE;
}
static BOOL hwInit(void)
{
    //busy pin  
    /* Set PH7 to GPIO */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xFu<<28)) | (0x0u<<28));    
    /* Configure PH7 to input mode */
    GPIO_OpenBit(LORA_BUSY_PORT, LORA_BUSY_PIN, DIR_INPUT, PULL_UP);
    return TRUE;
}
static BOOL swInit(void)
{   
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL LoraDrvInit(void)
{
    sysprintf("LoraDrvInit!!\n");
    pUartInterface = UartGetInterface(LORA_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("LoraDrvInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    //if(pUartInterface->initFunc(9600) == FALSE)
    if(pUartInterface->initFunc(115200) == FALSE)
    {
        sysprintf("LoraDrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(hwInit() == FALSE)
    {
        sysprintf("LoraDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("LoraDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}


INT32 LoraRead(PUINT8 pucBuf, UINT32 uLen)
{
    return pUartInterface->readFunc(pucBuf, uLen);
}

INT32 LoraWrite(PUINT8 pucBuf, UINT32 uLen)
{
    if(LoraWaitForReady())
    {
        return pUartInterface->writeFunc(pucBuf, uLen);
    }
    return 0;
}
BaseType_t LoraReadWait(TickType_t time)
{
    return pUartInterface->readWaitFunc(time);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


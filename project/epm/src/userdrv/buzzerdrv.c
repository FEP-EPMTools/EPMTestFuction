/**************************************************************************//**
* @file     buzzerdrv.c
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
#include "pwm.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "buzzerdrv.h"
#include "interface.h"

#include "timelib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define BUZZER_PORT  GPIOD
#define BUZZER_PIN   BIT15
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static SemaphoreHandle_t xSemaphore;
static TickType_t threadWaitTime        = portMAX_DELAY;

static uint32_t playTime, playDelayTime;
static uint8_t playCounter;
static BOOL initFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void buzzerPlay(TickType_t Time)
{  
    GPIO_SetBit(BUZZER_PORT, BUZZER_PIN);  
    vTaskDelay(Time/portTICK_RATE_MS);
    GPIO_ClrBit(BUZZER_PORT, BUZZER_PIN);
}
static void vBuzzerDrvTask( void *pvparamOuteters )
{
    sysprintf("vBuzzerDrvTask Going...\r\n");  
    for(;;)
    { 
        BaseType_t reval = xSemaphoreTake(xSemaphore, threadWaitTime); 
        if(reval == pdTRUE)
        {
            //sysprintf("\r\nvBuzzerDrvTask show...\r\n"); 
        }
        else
        {//timeout
            //sysprintf("\r\n[ENTER:%d, %d]\n", threadWaitTime, playCounter);
            buzzerPlay(playTime);
            if(playCounter == 1)
            {
                threadWaitTime = portMAX_DELAY;
                playCounter = 0;
            }
            else
            {
                playCounter--;
                threadWaitTime = playDelayTime/portTICK_RATE_MS;
            }  
            //sysprintf("\r\n[EXIT:%d, %d]\n", threadWaitTime, playCounter);            
        }   
    }
}

static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    //outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | (1 << 27)); // Enable PWM engine clock
    
    //PWM PIN
    outpw(REG_SYS_GPD_MFPH,(inpw(REG_SYS_GPD_MFPH) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(BUZZER_PORT, BUZZER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(BUZZER_PORT, BUZZER_PIN);
    
    buzzerPlay(200);
    return TRUE;
}
static BOOL swInit(void)
{
    xSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vBuzzerDrvTask, "vBuzzerDrvTask", 1024, NULL, BUZZER_DRV_THREAD_PROI, NULL );
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL BuzzerDrvInit(BOOL testMode)
{
    if(initFlag)
        return TRUE;
    sysprintf("BuzzerDrvInit!!\n");
    if(hwInit() == FALSE)
    {
        sysprintf("BuzzerDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("BuzzerDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    initFlag = TRUE;
    return TRUE;
}


void BuzzerPlay(uint32_t time, uint32_t delayTime, uint8_t counter, BOOL blocking)
{
    //sysprintf("BuzzerPlay enter (%d, %d, %d, %d)!!\n",  time, delayTime, counter, blocking);
    if(xSemaphore == NULL)
        return;
    if(time == 0)
        return;
    
    if(counter >1)
    {
        if(delayTime == 0)
            return;
    }
    
    if(counter == 0)
        return;
    
    playTime = time;
    playDelayTime = delayTime;
    playCounter = counter;
    
    threadWaitTime = 0;
    xSemaphoreGive(xSemaphore);  
    if(blocking)
    {    
        while(playCounter != 0)
        {
            vTaskDelay(10/portTICK_RATE_MS);
        }
    }
    //sysprintf("BuzzerPlay exit (%d, %d, %d, %d)!!\n",  time, delayTime, counter, blocking);
}
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


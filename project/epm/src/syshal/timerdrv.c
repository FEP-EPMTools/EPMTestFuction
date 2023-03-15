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
#include "timerdrv.h"
#include "halinterface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ENABLE_TIMER_DRV_DEBUG     0
#define TIME_THREAD_NUM   3
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static SemaphoreHandle_t xSemaphore[TIME_THREAD_NUM];
static TickType_t threadWaitTime[TIME_THREAD_NUM] = {portMAX_DELAY, portMAX_DELAY, portMAX_DELAY};
static TickType_t threadWaitTimeBackup[TIME_THREAD_NUM] = {portMAX_DELAY, portMAX_DELAY, portMAX_DELAY};
static timerCallbackFunc mTimerCallbackFunc = NULL;
static SemaphoreHandle_t xRunSemaphore;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static void vTimerDrvTask( void *pvparamOuteters )
{
    int taskIndex = (int)pvparamOuteters;
    vTaskDelay(taskIndex*50/portTICK_RATE_MS);
    //sysprintf("vTimerDrvTask[%d] Going...\r\n", taskIndex);   
    for(;;)
    {       
        BaseType_t reval = xSemaphoreTake(xSemaphore[taskIndex], threadWaitTime[taskIndex]); 
        
        if(reval == pdTRUE)
        {
            #if(ENABLE_TIMER_DRV_DEBUG)
            sysprintf("\r\n ##>> vTimerDrvTask go [%d], threadWaitTime[%d] = %d...\r\n", taskIndex, taskIndex, threadWaitTime[taskIndex]); 
            #endif
        }
        else
        {//timeout
            #if(ENABLE_TIMER_DRV_DEBUG)
            sysprintf("\r\n ##>> vTimerDrvTask timeout [%d], threadWaitTime[%d] = %d, threadWaitTimeBackup[%d] = %d..\r\n", taskIndex, taskIndex, threadWaitTime[taskIndex], taskIndex, threadWaitTimeBackup[taskIndex]); 
            #endif
            threadWaitTime[taskIndex] = threadWaitTimeBackup[taskIndex];   //for BOOL TimerRun(uint8_t timerIndex)  
            if(mTimerCallbackFunc != NULL)
            {
                #if(ENABLE_TIMER_DRV_DEBUG)
                sysprintf("\r\n ##>> ~~wait~~ [%d]!!\n", taskIndex); 
                #endif                
                xSemaphoreTake(xRunSemaphore, portMAX_DELAY);
                #if(ENABLE_TIMER_DRV_DEBUG)
                sysprintf("\r\n ##>> ~~go~~ [%d]!!\n", taskIndex); 
                #endif
                mTimerCallbackFunc(taskIndex);
                xSemaphoreGive(xRunSemaphore);
            }
  
        }   
        
        
    }
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL TimerDrvInit(void)
{
    int i;
    sysprintf("TimerDrvInit!!\n"); 
    xRunSemaphore = xSemaphoreCreateMutex();     
    for(i = 0; i<TIME_THREAD_NUM; i++)
    {
        xSemaphore[i] = xSemaphoreCreateCounting(5, 0);//xSemaphoreCreateBinary();
        //xSemaphore[i] = xSemaphoreCreateBinary();
    }
    
    xTaskCreate( vTimerDrvTask, "vTimerDrvTask0", 1024*10, (void*)0, TIMER_DRV_0_THREAD_PROI, NULL );
    xTaskCreate( vTimerDrvTask, "vTimerDrvTask1", 1024*10, (void*)1, TIMER_DRV_1_THREAD_PROI, NULL );
    xTaskCreate( vTimerDrvTask, "vTimerDrvTask2", 1024*10, (void*)2, TIMER_DRV_2_THREAD_PROI, NULL );
    return TRUE;
}
BOOL TimerAllStop(void)
{
    #if(ENABLE_TIMER_DRV_DEBUG)
    sysprintf("\r\n ##>> TimerAllStop ...\r\n");
    #endif
    int i;
    for(i=0; i< TIME_THREAD_NUM; i++)
    {
        TimerSetTimeout(i, portMAX_DELAY);
    }
    return TRUE;
}
BOOL TimerSetTimeout(uint8_t timerIndex, TickType_t time)
{
    #if(ENABLE_TIMER_DRV_DEBUG)
    sysprintf("\r\n ##>> TimerSetTimeout[%d] tick:%d (ori:%d) Going...\r\n", timerIndex, time, threadWaitTime[timerIndex]);
    #endif
    threadWaitTime[timerIndex] = time;
    threadWaitTimeBackup[timerIndex] = time;
    xSemaphoreGive(xSemaphore[timerIndex]);
    #if(ENABLE_TIMER_DRV_DEBUG)
    sysprintf("\r\n ##>> TimerSetTimeout[%d] tick:%d (ori:%d) OK...\r\n", timerIndex, time, threadWaitTime[timerIndex]);
    #endif
    return TRUE;
}
BOOL TimerRun(uint8_t timerIndex)
{
    threadWaitTime[timerIndex] = 0;
    xSemaphoreGive(xSemaphore[timerIndex]);
    #if(ENABLE_TIMER_DRV_DEBUG)
    sysprintf("\r\n ##>> TimerRun[%d] tick:%d Going...\r\n", timerIndex, threadWaitTime[timerIndex]);   
    #endif
    return TRUE;
}
void TimerSetCallback(timerCallbackFunc callback)
{
    //sysprintf("\r\n !!!! TimerSetCallback ...  !!!!\r\n"); 
    mTimerCallbackFunc = callback;
} 


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


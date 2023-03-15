/**************************************************************************//**
* @file     spacedrv.c
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
#include "spacedrv.h"
#include "interface.h"
#include "powerdrv.h"
#include "meterdata.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ENABLE_SPACE_DRV_DEBUG_MESSAGE   1
 
#define SPACE_DEV_INTERFACE_INDEX   DIST_SR04T_INTERFACE_INDEX//DIST_VL53L0X_INTERFACE_INDEX
//#define SPACE_DEV_INTERFACE_INDEX   DIST_VL53L0X_INTERFACE_INDEX
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static DistInterface* pDistInterface;
static BOOL mSpaceStatus[2] = {FALSE};
static int mSpaceDist[2];
static uint8_t mSpaceCounter[2] ;
static SemaphoreHandle_t xSemaphore;
#if(JUST_DISTANCE_FUNCTION)
static TickType_t threadWaitTime = 2000/portTICK_RATE_MS;
#else
static TickType_t threadWaitTime = portMAX_DELAY;
#endif

static BOOL spaceDrvPowerStatus = TRUE;
static BOOL spaceDrvIgnoreRun = FALSE;

static BOOL SpaceDrvCheckStatus(int flag);
static BOOL SpaceDrvPreOffCallback(int flag);
static BOOL SpaceDrvOffCallback(int flag);
static BOOL SpaceDrvOnCallback(int flag);
static powerCallbackFunc SpaceDrvPowerCallabck = {" [SpaceDrv] ", SpaceDrvPreOffCallback, SpaceDrvOffCallback, SpaceDrvOnCallback, SpaceDrvCheckStatus};
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL SpaceDrvPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    spaceDrvIgnoreRun = TRUE;
    //sysprintf("### SpaceDrv OFF Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName);    
    return reVal;    
}
static BOOL SpaceDrvOffCallback(int flag)
{
    int timers = 2000/10;
    while(!spaceDrvPowerStatus)
    {
        sysprintf("[s]");
        if(timers-- == 0)
        {
            return FALSE;
        }
        vTaskDelay(10/portTICK_RATE_MS); 
    }
    return TRUE;    
}
static BOOL SpaceDrvOnCallback(int flag)
{
    BOOL reVal = TRUE;
    //StartSpaceDrv();
    //sysprintf("### SpaceDrv ON Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName); 
    spaceDrvIgnoreRun = FALSE;
    spaceDrvPowerStatus = FALSE;  
    xSemaphoreGive(xSemaphore);    
    return reVal;    
}
static BOOL SpaceDrvCheckStatus(int flag)
{
    //BOOL reVal = TRUE;
    //sysprintf("### SpaceDrv STATUS Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName); 
    return spaceDrvPowerStatus;    
}


static void vSpaceDrvTask( void *pvParameters )
{
    SpaceDrvOnCallback(0);
    for(;;)
    {  
        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
        //sysprintf(" -wait- vSpaceDrvTask %d---\r\n", threadWaitTime); 
        #endif
        spaceDrvPowerStatus = TRUE;
        xSemaphoreTake(xSemaphore, threadWaitTime/*portMAX_DELAY*/);
        if(spaceDrvIgnoreRun)
        {
            sysprintf("vSpaceDrvTask ignore---\r\n"); 
            continue;
        }
        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
        //sysprintf(" -start- vSpaceDrvTask ---\r\n"); 
        #endif
        //if(GetMainSpaceDrvFlag())
        {
            int detectResult;
            BOOL mSpaceStatusTmp[2] = {FALSE};//為了能一次更新狀態 不然拍照會拍兩張
            spaceDrvPowerStatus = FALSE;
            if(pDistInterface->measureDistFunc(DIST_DEVICE_1, &detectResult))
            {
                mSpaceDist[DIST_DEVICE_1] = detectResult;
                #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                sysprintf(" -start- SPACE_DRV_1 detectResult = %d cm  ---\r\n", detectResult); 
                #endif
                if(detectResult < 80)
                {
                    
                    mSpaceCounter[SPACE_INDEX_1]++;
                    
                    if(mSpaceCounter[SPACE_INDEX_1] >= 3)
                    {
                        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                        sysprintf(" -start- SPACE_DRV_1 get car: counter = %d, set TRUE  ---\r\n", mSpaceCounter[SPACE_INDEX_1]); 
                        #endif
                        mSpaceStatusTmp[SPACE_INDEX_1] = TRUE;
                        mSpaceCounter[SPACE_INDEX_1] = 3;
                    }
                    else
                    {
                        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                        sysprintf(" -start- SPACE_DRV_1 get car: counter = %d, Set FALSE ---\r\n", mSpaceCounter[SPACE_INDEX_1]); 
                        #endif
                        mSpaceStatusTmp[SPACE_INDEX_1] = FALSE;
                    }
                }
                else
                {
                    #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                    sysprintf(" -start- SPACE_DRV_1 no car: Set FALSE ---\r\n"); 
                    #endif
                    mSpaceStatusTmp[SPACE_INDEX_1] = FALSE;
                    mSpaceCounter[SPACE_INDEX_1] = 0;
                }
            }
            else
            {
                mSpaceDist[DIST_DEVICE_1] = 0;
                MeterSetErrorCode(METER_ERROR_CODE_SPSCE_DRV);                
                #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                sysprintf(" -error- SPACE_DRV_1   ---\r\n"); 
                #endif
                #warning need check here
            }
            if(pDistInterface->measureDistFunc(DIST_DEVICE_2, &detectResult))
            {
                mSpaceDist[DIST_DEVICE_2] = detectResult;
                #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                sysprintf(" -start- SPACE_DRV_2 detectResult = %d cm  ---\r\n", detectResult); 
                #endif
                if(detectResult < 80)
                {
                    
                    mSpaceCounter[SPACE_INDEX_2]++;
                    
                    if(mSpaceCounter[SPACE_INDEX_2] >= 3)
                    {
                        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                        sysprintf(" -start- SPACE_DRV_2 get car: counter = %d, set TRUE  ---\r\n", mSpaceCounter[SPACE_INDEX_2]); 
                        #endif
                        mSpaceStatusTmp[SPACE_INDEX_2] = TRUE;
                        mSpaceCounter[SPACE_INDEX_2] = 3;
                    }
                    else
                    {
                        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                        sysprintf(" -start- SPACE_DRV_2 get car: counter = %d, Set FALSE ---\r\n", mSpaceCounter[SPACE_INDEX_2]); 
                        #endif
                        mSpaceStatusTmp[SPACE_INDEX_2] = FALSE;
                    }
                }
                else
                {
                    #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                    sysprintf(" -start- SPACE_DRV_2 no car: Set FALSE ---\r\n"); 
                    #endif
                    mSpaceStatusTmp[SPACE_INDEX_2] = FALSE;
                    mSpaceCounter[SPACE_INDEX_2] = 0;
                }
            }
            else
            {
                mSpaceDist[DIST_DEVICE_2] = 0;
                MeterSetErrorCode(METER_ERROR_CODE_SPSCE_DRV);   
                #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
                sysprintf(" -error- SPACE_DRV_1   ---\r\n"); 
                #endif
                #warning need check here
            }
            mSpaceStatus[SPACE_INDEX_1] = mSpaceStatusTmp[SPACE_INDEX_1];
            mSpaceStatus[SPACE_INDEX_2] = mSpaceStatusTmp[SPACE_INDEX_2];
            //RefreshSpaceStatusScreen();            
        }
        //else
        //{
        //    mSpaceCounter[SPACE_INDEX_1] = 0;
        //    mSpaceCounter[SPACE_INDEX_2] = 0;
        //}
        

        
        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
        //sysprintf(" -end- vSpaceDrvTask ---\r\n"); 
        #endif
    }
    
}

static BOOL swInit(void)
{   
    PowerRegCallback(&SpaceDrvPowerCallabck);
    xSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vSpaceDrvTask, "vSpaceDrvTask", 2048, NULL, SPACE_DRV_THREAD_PROI, NULL );
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL SpaceDrvInit(BOOL testModeFlag)
{
    sysprintf("SpaceDrvInit!!\n");
    pDistInterface = DistGetInterface(SPACE_DEV_INTERFACE_INDEX);
    if(pDistInterface == NULL)
    {
        sysprintf("SpaceDrvInit ERROR (pDistInterface == NULL)!!\n");
        return FALSE;
    }
    if(pDistInterface->initFunc() == FALSE)
    {
        sysprintf("SpaceDrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("SpaceDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    
    sysprintf("SpaceDrvInit OK!!\n");
    return TRUE;
}

BOOL GetSpaceStatus(uint8_t index)
{
    return mSpaceStatus[index];
}

int GetSpaceDist(uint8_t index)
{
    return mSpaceDist[index];
}
void SetSpaceStatus(uint8_t index, BOOL status)
{
    if(status)
    {
        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
        sysprintf(" -- SetSpaceStatus [%] to be TRUE ---\r\n", index); 
        #endif
        mSpaceCounter[index] = 3;
    }
    else
    {
        #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
        sysprintf(" -- SetSpaceStatus [%] to be FALSE ---\r\n", index); 
        #endif
        mSpaceCounter[index] = 0;
    }
    mSpaceStatus[index] = status;
    
}
void StartSpaceDrv(void)
{
    #if(ENABLE_SPACE_DRV_DEBUG_MESSAGE)
    sysprintf(" --StartSpaceDrv ---\r\n"); 
    #endif
    spaceDrvPowerStatus = FALSE;
    xSemaphoreGive(xSemaphore);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


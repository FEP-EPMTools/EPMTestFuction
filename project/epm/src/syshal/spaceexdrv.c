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
#include "spaceexdrv.h"
#include "interface.h"

#include "powerdrv.h"

#include "halinterface.h"
#include "meterdata.h"
#include "guimanager.h"
//#include "radardrv.h"
#include "radardrv.h"
#include "sflashrecord.h"
#include "loglib.h"
#include "dataprocesslib.h"
#include "epddrv.h"
#if(USE_SPACE_EX_DRIVER)
#else
int SpaceExGetCurrentCarIdFake(uint8_t index);
#endif

#if (ENABLE_BURNIN_TESTER)
#include "timelib.h"
#include "burnintester.h"
#include "modemagent.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define SPACE_EX_DEV_INTERFACE_INDEX   RADAR_INTERFACE_INDEX
#define ENABLE_SPACE_EX_DRV_DEBUG_MESSAGE  0

#define SPACE_EX_NUM    2

#define SPACE_DRV_EX_INTERVAL  3000
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static RadarInterface* pRadarInterface;
//static int mCarId[SPACE_EX_NUM] = {0, 0};
static SemaphoreHandle_t xSemaphore[SPACE_EX_NUM];
static TickType_t threadWaitTime[SPACE_EX_NUM] = {portMAX_DELAY, portMAX_DELAY};

static int sampleIntervalTime = SPACE_DRV_EX_INTERVAL;
static int unstableIntervalTime = SPACE_DRV_EX_INTERVAL;

static SemaphoreHandle_t xRunningSemaphore[SPACE_EX_NUM] = {NULL, NULL};
static BOOL testModeFlag = FALSE;

static BOOL SpaceExDrvCheckStatus(int flag);
static BOOL SpaceExDrvPreOffCallback(int flag);
static BOOL SpaceExDrvOffCallback(int flag);
//static BOOL SpaceExDrvOnCallback(int flag, int wakeupSource);
static BOOL SpaceExDrvOnCallback(int flag);

static BOOL spaceExDrvPowerStatus[SPACE_EX_NUM] = {TRUE, TRUE};
static BOOL spaceExDrvIgnoreRun = FALSE;
static BOOL spaceExDrvWorkStatus[SPACE_EX_NUM] = {TRUE, TRUE};
static powerCallbackFunc SpaceExDrvPowerCallabck = {" [SpaceExDrv] ", SpaceExDrvPreOffCallback, SpaceExDrvOffCallback, SpaceExDrvOnCallback, SpaceExDrvCheckStatus};

#if (ENABLE_BURNIN_TESTER)
static int spaceExMaterial[SPACE_EX_NUM] = {0, 0};
static int spaceExObject[SPACE_EX_NUM] = {0, 0};
static int spaceExObjectStatus[SPACE_EX_NUM] = {0, 0};
static uint32_t radarBurninCounter[SPACE_EX_NUM] = {0, 0};
static uint32_t radarBurninErrorCounter[SPACE_EX_NUM] = {0, 0};
static uint32_t lidarBurninCounter[SPACE_EX_NUM] = {0, 0};
static uint32_t lidarBurninErrorCounter[SPACE_EX_NUM] = {0, 0};

static SemaphoreHandle_t xRadarSemaphore , xRadar0Semaphore , xRadar1Semaphore;

static char errorMsgBuffer[256];
#endif

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static BOOL SpaceExDrvPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    spaceExDrvIgnoreRun = TRUE;
    //sysprintf("### SpaceExDrv OFF Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName);    
    return reVal;    
}
static BOOL SpaceExDrvOffCallback(int flag)
{
    
    switch(flag)
    {
        //case SUSPEND_MODE_BATTERY_REMOVE:
        //    if(pRadarInterface->setPowerFunc != NULL)            
        //        pRadarInterface->setPowerFunc(FALSE);            
        //    return TRUE;
        //case SUSPEND_MODE_TARIFF:
        default:
        #if(1)
        if(pRadarInterface->setPowerFunc != NULL)            
            pRadarInterface->setPowerFunc(FALSE);
        #else
        {
            int timers = 2000/10;
            //while(!spaceExDrvPowerStatus)
            while((!spaceExDrvPowerStatus[SPACE_EX_INDEX_1]) || (!spaceExDrvPowerStatus[SPACE_EX_INDEX_2]))
            {
                sysprintf("[s]");
                if(timers-- == 0)
                {
                    return FALSE;
                }
                vTaskDelay(10/portTICK_RATE_MS); 
            }
        }
        #endif
        return TRUE;  
                    
    }    
}
//static BOOL SpaceExDrvOnCallback(int flag, int wakeupSource)
static BOOL SpaceExDrvOnCallback(int flag)
{
    BOOL reVal = TRUE;
    //StartSpaceExDrv();
    //sysprintf("### SpaceExDrv ON Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName); 
    spaceExDrvIgnoreRun = FALSE;
    //spaceExDrvPowerStatus = FALSE;  
    //xSemaphoreGive(xSemaphore);    
    return reVal;    
}
static BOOL SpaceExDrvCheckStatus(int flag)
{
    //BOOL reVal = TRUE;
    //sysprintf("### SpaceExDrv STATUS Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName); 
    switch(flag)
    {
        //case SUSPEND_MODE_BATTERY_REMOVE:            
        //    return TRUE;
        //case SUSPEND_MODE_TARIFF:
        default:
            #if(1)
            return TRUE;
            #else
            if(spaceExDrvPowerStatus[SPACE_EX_INDEX_1] && spaceExDrvPowerStatus[SPACE_EX_INDEX_2])
                return TRUE; 
            else
                return FALSE;
            #endif
    }        
}
extern BOOL SysGetBooted(void);
static void vSpaceExDrvTask( void *pvParameters )
{
    //SpaceExDrvOnCallback(0, WAKEUP_SOURCE_NONE);
    SpaceExDrvOnCallback(0);
    int spaceIndex = (int)pvParameters;
    BOOL changeFlag;
    sysprintf("vSpaceDrvTask[%d] WAIT---\r\n", spaceIndex);  
    if(testModeFlag)
    {
    }
    else
    {    
        while(SysGetBooted() == FALSE)
        {
            vTaskDelay(2000/portTICK_RATE_MS);
        }
        threadWaitTime[spaceIndex] = (2000/portTICK_RATE_MS);
    }
    sysprintf("vSpaceDrvTask[%d] GO---\r\n", spaceIndex);    
    //threadWaitTime[spaceIndex] = portMAX_DELAY;
    
    for(;;)
    {  
        #if(ENABLE_SPACE_EX_DRV_DEBUG_MESSAGE)
        sysprintf(" -wait- vSpaceExDrvTask(spaceIndex = %d) %d[%d]---\r\n", spaceIndex, threadWaitTime[spaceIndex], portMAX_DELAY); 
        #endif
        spaceExDrvPowerStatus[spaceIndex]  = TRUE;
        #if(ENABLE_THREAD_RUNNING_DEBUG)
        sysprintf("\r\n ! (-WARNING-) %s Waiting (%d)... ) !\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()), threadWaitTime[spaceIndex]); 
        #endif
        xSemaphoreTake(xSemaphore[spaceIndex], threadWaitTime[spaceIndex]/*portMAX_DELAY*/);
        #if(ENABLE_THREAD_RUNNING_DEBUG)
        sysprintf("\r\n ! (-WARNING-) %s Go (%d)... ) !\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()), threadWaitTime[spaceIndex]); 
        #endif        
        
        #if(ENABLE_SPACE_EX_DRV_DEBUG_MESSAGE)
        sysprintf(" -start- vSpaceExDrvTask (spaceIndex = %d) ---\r\n", spaceIndex); 
        #endif
        if(testModeFlag)
        {
            //xSemaphoreTake(xRunningSemaphore, portMAX_DELAY);
            SpaceExSemaphoreTake(spaceIndex);
            threadWaitTime[spaceIndex] = SpaceExGetFeature(spaceIndex, &changeFlag);
            //xSemaphoreGive(xRunningSemaphore);
            SpaceExSemaphoreGive(spaceIndex);
        }
        else
        {
            #if(0)
            if(GetMeterPara()->bayenable[spaceIndex] == 0)
            {
                sysprintf(" WARNING--->vSpaceExDrvTask [%d] disable: IGNORE...\r\n", spaceIndex);
                threadWaitTime[spaceIndex] = (60*1000/portTICK_RATE_MS);
                continue;
            }
            
            spaceExDrvPowerStatus[spaceIndex]  = FALSE;
            //----------
            
            //xSemaphoreTake(xRunningSemaphore, portMAX_DELAY);
            SpaceExSemaphoreTake(spaceIndex);
            if(spaceExDrvIgnoreRun)
            {
                sysprintf("vSpaceDrvTask ignore---\r\n");             
            }
            else
            {
                if(GuiManagerCompareCurrentScreenId(GUI_STANDBY_ID))
                {
                    threadWaitTime[spaceIndex] = SpaceExGetFeature(spaceIndex, &changeFlag);
                }
                else
                {
                    threadWaitTime[spaceIndex] = 5000/portTICK_RATE_MS;
                }
            }
            //xSemaphoreGive(xRunningSemaphore);
            SpaceExSemaphoreGive(spaceIndex);
            //----------   
            
            
            
            if(MeterGetEnableDistanceInfoFlag())
            {
                if(GuiManagerCompareCurrentScreenId(GUI_STANDBY_ID))
                {
                    //#warning need use timer driver to update screen
                    //EPDDrawDistanceNumber(FALSE, mSpaceDist[SPACE_INDEX_1][0] , 250, 220);
                    //EPDDrawDistanceNumber(TRUE, mSpaceDist[SPACE_INDEX_2][0] , 720, 220);
                    if(changeFlag)
                    {
                        MeterDataThreadStart();  
                        //GuiManagerUpdateScreenEx(GUI_STANDBY_ID); 
                        if(spaceIndex == 0)
                        {
                            #if(USE_SPACE_EX_DRIVER)
                            EPDDrawDistanceNumber(TRUE, SpaceExGetCurrentCarId(SPACE_EX_INDEX_1), 250, 535/*230*/);
       
                            #else
                            EPDDrawDistanceNumber(TRUE, SpaceExGetCurrentCarIdFake(SPACE_INDEX_1), 250, 535/*230*/);
       
                            #endif
                        }
                        else if(spaceIndex == 1)
                        {
                            #if(USE_SPACE_EX_DRIVER)                
                            EPDDrawDistanceNumber(TRUE, SpaceExGetCurrentCarId(SPACE_EX_INDEX_2), 720, 535/*230*/);        
                            #else
                            EPDDrawDistanceNumber(TRUE, SpaceExGetCurrentCarIdFake(SPACE_INDEX_2), 720, 535/*230*/);
                            #endif
                        }
                                              
                    }                        
                }
            }
            #endif
           
        }

        //threadWaitTime = (3000/portTICK_RATE_MS);
        #if(ENABLE_SPACE_EX_DRV_DEBUG_MESSAGE)
        sysprintf(" -end- vSpaceExDrvTask (spaceIndex = %d) ---\r\n", spaceIndex); 
        #endif
    }
    
}

#if (ENABLE_BURNIN_TESTER)
static void vSpaceExDrvTestTask(void *pvParameters)
{
    int spaceIndex = (int)pvParameters;
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    BOOL changeFlag;
    int featureValue;
    int VerfeatureValue;
    int RetryCount = 0;
    int VerRetryCount = 0;
    char errorMsgBuffer[256];
    
    uint8_t RadarData[22];
    uint8_t VersionCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D};
    uint8_t ResultCmd[24] = {0x7A, 0xA7, 0x00, 0x18, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x1A, 0xD3, 0x3D};
    
    //TickType_t tickLocalStart;
    terninalPrintf("vSpaceExDrvTestTask[%d] Going...\r\n", spaceIndex);
    
    for(;;)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vSpaceExDrvTestTask[%d] Terminated !!\r\n", spaceIndex);
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_SPACE_EX_INTERVAL)
        {
            //terninalPrintf("vSpaceExDrvTestTask heartbeat.\r\n");
            //lastTime = currentTime;
            //testLoop = FALSE;
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        
        testLoop = FALSE;
        //if (GetModemFTPRunningStatus())
        //{
        //    lastTime = GetCurrentUTCTime();
        //    continue;
        //}
        /*
        if (pRadarInterface->setPowerStatusFunc != NULL)
        {
            pRadarInterface->setPowerStatusFunc(spaceIndex, TRUE);
            //vTaskDelay(1000 / portTICK_RATE_MS);
            
            //ReadRadarVersionString(spaceIndex, errorMsgBuffer);
            //After power on, send serveral commands to active radar & lidar, and wait to become stable!!
            while (1)
            {
                //featureValue = pRadarInterface->checkFeaturnFunc(spaceIndex, &changeFlag, &spaceExMaterial[spaceIndex], &spaceExObject[spaceIndex], &spaceExObjectStatus[spaceIndex]);
                //if (featureValue != VOS_FEATURE_IGNORE) {
                if (ReadRadarVersionString(spaceIndex, errorMsgBuffer)) {
                    break;
                }
                vTaskDelay(200 / portTICK_RATE_MS);
            }
        }
        
        */
        
        if (pRadarInterface->setPowerStatusFunc != NULL)
        {
            pRadarInterface->setPowerStatusFunc(spaceIndex, TRUE);
            pRadarInterface->RadarFlushFunc(spaceIndex);
            VerRetryCount = 0;
            while (1)
            {

                //if (ReadRadarVersionString(spaceIndex, errorMsgBuffer)) {
                 //   break;
                //}
                VerfeatureValue = pRadarInterface->RadarResultFunc(spaceIndex, 0x00,VersionCmd,RadarData);
                if( VerfeatureValue == TRUE)
                {
                    //terninalPrintf("Radar%d Version : Ver.%d.%d.%d.%d\r\n",spaceIndex+1,RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
                    break;
                }
                else if((VerfeatureValue == RADAR_RETURN_BLANK) || (VerfeatureValue == RADAR_WAIT_NEXTDATA))
                {
                    VerRetryCount++;
                    if(VerRetryCount >= 10)
                        break;
                }
                else if( VerfeatureValue == FALSE)
                {
                    #if (ENABLE_BURNIN_TESTER)
                    if (EnabledBurninTestMode())
                    {
                        sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    VerfeatureValue == FALSE --\r\n", spaceIndex+1);
                        AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                    }
                    #endif
                    
                    
                    VerRetryCount++;
                    if(VerRetryCount >= 10)
                        break;
                    pRadarInterface->setPowerStatusFunc(spaceIndex, FALSE);
                    vTaskDelay(500 / portTICK_RATE_MS);
                    pRadarInterface->setPowerStatusFunc(spaceIndex, TRUE);
                    vTaskDelay(500 / portTICK_RATE_MS);
                }
                else if( VerfeatureValue == UART_FORMAT_ERR)
                {
                    VerRetryCount++;
                    if(VerRetryCount >= 10)
                        break;
                    if(pRadarInterface->setPowerStatusFunc != NULL) {
                        pRadarInterface->RadarinitBurningFunc(spaceIndex);
                        pRadarInterface->setPowerStatusFunc(spaceIndex, FALSE);
                        vTaskDelay(500 / portTICK_RATE_MS);
                        pRadarInterface->setPowerStatusFunc(spaceIndex, TRUE);
                        vTaskDelay(500 / portTICK_RATE_MS);
                    }
                }
                //vTaskDelay(200 / portTICK_RATE_MS);
            }
        }
        
        //xSemaphoreGive(xRadar0Semaphore);
        for (int i = 0 ; i < BURNIN_SPACE_EX_COUNT_PER_LOOP ; i++)
        {
            
            /*
            terninalPrintf("spaceIndex = %d\r\n",spaceIndex);
            if(spaceIndex == 0)
                xSemaphoreTake(xRadar0Semaphore, portMAX_DELAY);
            else if(spaceIndex == 1)
                xSemaphoreTake(xRadar1Semaphore, portMAX_DELAY);
            */
            //vTaskDelay(500 / portTICK_RATE_MS);
            
            //tickLocalStart = xTaskGetTickCount();
            
            
            //featureValue = pRadarInterface->checkFeaturnFunc(spaceIndex, &changeFlag, &spaceExMaterial[spaceIndex], &spaceExObject[spaceIndex], &spaceExObjectStatus[spaceIndex]);
            featureValue = pRadarInterface->RadarResultFunc(spaceIndex, 0x02,ResultCmd,RadarData);
            
            
            //terninalPrintf("vSpaceExDrvTestTask[%d], featureValue=%d, spaceExObjectStatus=%d\r\n", spaceIndex, featureValue, spaceExObjectStatus[spaceIndex]);
            /*
            switch (featureValue)
            {
                case VOS_FEATURE_OCCUPIED:
                case VOS_FEATURE_OCCUPIED_UN_STABLED:
                case VOS_FEATURE_VACUUM:
                case VOS_FEATURE_VACUUM_UN_STABLED:
                case VOS_FEATURE_IGNORE:
                    if (spaceExObjectStatus[spaceIndex] == VOS_OBJECT_MOVING_ERROR) {
                        lidarBurninErrorCounter[spaceIndex]++;
                    }
                    break;
                case RADAR_FEATURE_IGNORE_LIDAR_FAIL:
                    lidarBurninErrorCounter[spaceIndex]++;
                    break;
                default:
                    sprintf(errorMsgBuffer, "vSpaceExDrvTestTask[%d] ==> featureValue[%d]=0x%x\r\n", spaceIndex, radarBurninCounter[spaceIndex], featureValue);
                    AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                    terninalPrintf(errorMsgBuffer);
                    radarBurninErrorCounter[spaceIndex]++;
                    lidarBurninErrorCounter[spaceIndex]++;
                    break;
            }
            
            */
            //terninalPrintf("checkFeaturnFunc[%d] ==> featureValue=0x%x, counter=%d, SpentTime=%d\r\n", spaceIndex, featureValue, radarBurninCounter[spaceIndex], (xTaskGetTickCount() - tickLocalStart));
            if(featureValue == UART_FORMAT_ERR)
            {
                if(pRadarInterface->setPowerStatusFunc != NULL) 
                {
                    pRadarInterface->RadarinitBurningFunc(spaceIndex);
                    pRadarInterface->setPowerStatusFunc(spaceIndex, FALSE);
                    vTaskDelay(500 / portTICK_RATE_MS);
                    pRadarInterface->setPowerStatusFunc(spaceIndex, TRUE);
                    vTaskDelay(500 / portTICK_RATE_MS);
                }                
            }
            
            if( (featureValue != RADAR_RETURN_BLANK) && (featureValue != RADAR_WAIT_NEXTDATA) && (featureValue != UART_FORMAT_ERR)) 
            {
                radarBurninCounter[spaceIndex]++;
                if(featureValue == FALSE)
                    radarBurninErrorCounter[spaceIndex]++;
            }
            
            
            if( (featureValue == TRUE) || (featureValue == FALSE) )
            {
                RetryCount = 0;
            }
            else if((featureValue == RADAR_RETURN_BLANK) || (featureValue == RADAR_WAIT_NEXTDATA))
            {
                RetryCount++;
                if(RetryCount >= 10)
                {
                    terninalPrintf("Radar%d reset power.\r\n", spaceIndex);
                    pRadarInterface->setPowerStatusFunc(spaceIndex, FALSE);
                    vTaskDelay(500 / portTICK_RATE_MS);
                    pRadarInterface->setPowerStatusFunc(spaceIndex, TRUE);
                }
            }
                
            
            
            
                
            //radarBurninCounter[spaceIndex]++;
            //lidarBurninCounter[spaceIndex]++;
            /*
            if(spaceIndex == 0)
                xSemaphoreGive(xRadar1Semaphore);
            else if(spaceIndex == 1)
                xSemaphoreGive(xRadar0Semaphore);
            */

            
            //vTaskDelay(BURNIN_SPACE_EX_GAP_TIME / portTICK_RATE_MS);
            
            
            if (GetPrepareStopBurninFlag()) {
                break;
            }
        }
        if(pRadarInterface->setPowerStatusFunc != NULL) {
            pRadarInterface->RadarinitBurningFunc(spaceIndex);
            pRadarInterface->setPowerStatusFunc(spaceIndex, FALSE);
        }
        lastTime = GetCurrentUTCTime();
    }
}
#endif

#if (ENABLE_BURNIN_TESTER)
static BOOL swInit(void)
{
    PowerRegCallback(&SpaceExDrvPowerCallabck);
    
    //xRadar0Semaphore = xSemaphoreCreateBinary(); 
    //xRadar1Semaphore = xSemaphoreCreateBinary(); 
    xTaskCreate(vSpaceExDrvTestTask, "vSpaceExDrvTestTask1", 1024*10, (void*)0, SPACE_DRV_THREAD_PROI, NULL);
    vTaskDelay(50 / portTICK_RATE_MS);
    xTaskCreate(vSpaceExDrvTestTask, "vSpaceExDrvTestTask2", 1024*10, (void*)1, SPACE_DRV_THREAD_PROI, NULL);
    //vTaskStartScheduler();
    return TRUE;
}
#else
static BOOL swInit(void)
{
    PowerRegCallback(&SpaceExDrvPowerCallabck);
    for(int i = 0; i<SPACE_EX_NUM; i++)
    {
        xSemaphore[i] = xSemaphoreCreateBinary();
        xRunningSemaphore[i]  = xSemaphoreCreateMutex(); 
    }
    
    xTaskCreate( vSpaceExDrvTask, "vSpaceExDrvTask1", 4048, (void*)0, SPACE_DRV_THREAD_PROI, NULL );
    xTaskCreate( vSpaceExDrvTask, "vSpaceExDrvTask2", 4048, (void*)1, SPACE_DRV_THREAD_PROI, NULL );
    return TRUE;
}
#endif

static void printfBuffData(char* str, uint8_t* data, int len)
{
    int i;
    sysprintf("\r\n %s: len = %d...\r\n", str, len);
    
    for(i = 0; i<len; i++)
    { 
        sysprintf("0x%02x, ",(unsigned char)data[i]);
        if((i%16) == 15)
            sysprintf("\r\n");
    }
    sysprintf("\r\n");
    
}

static uint8_t* hexStringToByteArray(char* hexStr, int* returnLen) 
{
    int len = strlen(hexStr);
    if(len == 0)
    {
        return NULL;
    }
    uint8_t* data = pvPortMalloc(len/2);
    *returnLen = len/2;
    for (int i = 0; i < len; i += 2)    
    {
        //unsigned long strtoul (const char* str, char** endptr, int base);
        char hexStrTmp[3];
        memcpy(hexStrTmp, hexStr+i, sizeof(char)*2);
        hexStrTmp[2] = 0x0;
        data[i/2] = strtoul (hexStrTmp, NULL, 16);;
    }
    return data;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL SpaceExDrvInit(BOOL flag)
{
    sysprintf("SpaceExDrvInit!!\n");
    testModeFlag = flag;
    //pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);
    pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);
    
    if(pRadarInterface == NULL)
    {
        sysprintf("SpaceExDrvInit ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        sysprintf("SpaceExDrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("SpaceExDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    
    sysprintf("SpaceExDrvInit OK!!\n");
    return TRUE;
}

TickType_t SpaceExGetFeature(uint8_t index, BOOL* changeFlag)
{    
    TickType_t waitTime = (0/portTICK_RATE_MS);
    /*
    #define RADAR_FEATURE_OCCUPIED      0x01
    #define RADAR_FEATURE_VACUUM        0x02
    #define RADAR_FEATURE_UN_STABLED    0x03
    #define RADAR_FEATURE_INIT          0x04
    */
    vTaskDelay(200/portTICK_RATE_MS);
    int featureValue = pRadarInterface->checkFeaturnFunc(index, changeFlag, NULL, NULL, NULL);
    switch(featureValue)
    {
         case RADAR_FEATURE_OCCUPIED:
             /*
            if(GetMeterStorageData()->carId[index] == 0)
            {
                GetMeterStorageData()->carId[index] = 1;
                MeterStorageFlush(FALSE);
            }
            */
            if(testModeFlag)
            {
                waitTime = (sampleIntervalTime/portTICK_RATE_MS);
            }
            else
            {
                waitTime = (sampleIntervalTime/portTICK_RATE_MS);
                //waitTime = portMAX_DELAY;//(5000/portTICK_RATE_MS);
            }
            //#if(ENABLE_LOG_FUNCTION)
            //{
            //    char printBuffer[512];
            //    sprintf(printBuffer,  "[RADAR SpaceDev] [%d] (RADAR_FEATURE_OCCUPIED), mCarId = %d, waitTime = %d!!! \r\n", index, mCarId[index], (int)(waitTime*portTICK_RATE_MS)); 
            //    LoglibPrintf(LOG_TYPE_INFO, printBuffer, FALSE);
            //}
            //#endif 
            spaceExDrvWorkStatus[index] = TRUE;
            break;
        case RADAR_FEATURE_VACUUM:
            /*
            if(GetMeterStorageData()->carId[index] != 0)
            {
                GetMeterStorageData()->carId[index] = 0;   
                MeterStorageFlush(FALSE);
            }    
            */        
            if(testModeFlag)
            {
                waitTime = (sampleIntervalTime/portTICK_RATE_MS);
            }
            else
            {
                waitTime = (sampleIntervalTime/portTICK_RATE_MS);
            }
            spaceExDrvWorkStatus[index] = TRUE;
            break;
        case RADAR_FEATURE_VACUUM_UN_STABLED:
            waitTime = (unstableIntervalTime/portTICK_RATE_MS);
            spaceExDrvWorkStatus[index] = TRUE;
            break;
        case RADAR_FEATURE_OCCUPIED_UN_STABLED:
            waitTime = (unstableIntervalTime/portTICK_RATE_MS);
            spaceExDrvWorkStatus[index] = TRUE;
            break;
        default:
            {
                //sysprintf("SpaceExGetFeature %d TIMEOUT!!\n", index);
                #if(0)
                if(spaceExDrvWorkStatus[index] == TRUE)
                {
                    char str[512];
                    #if(ENABLE_LOG_FUNCTION)
                    {                    
                        sprintf(str, "[RADAR] -> SpaceExGetFeature %d TIMEOUT!!\r\n", index);
                        LoglibPrintf(LOG_TYPE_ERROR, str, FALSE);
                    }
                    #else
                    sysprintf("[RADAR] -> SpaceExGetFeature %d TIMEOUT!!\n", index);
                    #endif   
                    sprintf(str, "RadarErr<%d>", index);
                    DataProcessSendStatusData(0, str, WEB_POST_EVENT_ALERT);
                    spaceExDrvWorkStatus[index] = FALSE;
                }
                else
                {
                    sysprintf("[RADAR] -> SpaceExGetFeature %d TIMEOUT!!\n", index);
                }
                #endif
                //waitTime = (sampleIntervalTime/portTICK_RATE_MS);
                //waitTime = (60*1000/portTICK_RATE_MS);
                waitTime = (15*1000/portTICK_RATE_MS);
               // waitTime = portMAX_DELAY;
            }
            break;
    }
    //just for test
    //waitTime = 0;
    //waitTime = (1000/portTICK_RATE_MS);
    //if(testModeFlag)
    //{
    //    waitTime = (3000/portTICK_RATE_MS);
    //}
    //sysprintf(" !!! SpaceExGetFeature !!!  [%d]: mCarId = %d, waitTime = %d--  \r\n\r\n", index, GetMeterStorageData()->carId[index], waitTime*portTICK_RATE_MS);
    
    
    return waitTime;
}
int SpaceExGetCurrentCarId(uint8_t index)
{
    //sysprintf("\r\n ->SpaceExGetCurrentCarId [%d]: mCarId = %d ...\r\n", index, GetMeterStorageData()->carId[index]);
    //return GetMeterStorageData()->carId[index];
    return 0;
}

void SpaceExSetCurrentCarId(uint8_t index, int value)
{
    //sysprintf("\r\n ->SpaceExGetCurrentCarId [%d]: mCarId = %d ...\r\n", index, mCarId[index]);
    #if(0)
    if(value == 0)
    {
        GetMeterStorageData()->carId[index] = value;
    }
    else
    {
        GetMeterStorageData()->carId[index] = GetMeterStorageData()->carId[index] + value;
    }
    MeterStorageFlush(FALSE);
    #if(ENABLE_LOG_FUNCTION)
    {
        char str[512];
        sprintf(str, " !! RESET CAR ID !!!! ->SpaceExSetCurrentCarId [%d, %d]: mCarId = %d ...\r\n", index, value, GetMeterStorageData()->carId[index]);
        LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
    }
    #else
    sysprintf("\r\n ->SpaceExSetCurrentCarId [%d, %d]: mCarId = %d ...\r\n", index, value, mCarId[index]);
    #endif
    #endif
}



BOOL SpaceExStart(int spaceIndex)
{
    //threadWaitTime[spaceIndex] = (0/portTICK_RATE_MS);
    sysprintf(" !!! SpaceExStart [%d]!!!\r\n", spaceIndex);
    xSemaphoreGive(xSemaphore[spaceIndex]);
    return TRUE;
}

void SpaceExSemaphoreTake(int index)
{
    xSemaphoreTake(xRunningSemaphore[index], portMAX_DELAY);
    //vTaskDelay(500/portTICK_RATE_MS);xSemaphore
}

void SpaceExSemaphoreGive(int index)
{
    xSemaphoreGive(xRunningSemaphore[index]);
    
}

void SpaceExSemaphoreTotalTake(void)
{
    xSemaphoreTake(xRunningSemaphore[0], portMAX_DELAY);
    xSemaphoreTake(xRunningSemaphore[1], portMAX_DELAY);
}

void SpaceExSemaphoreTotalGive(void)
{
    xSemaphoreGive(xRunningSemaphore[0]);
    xSemaphoreGive(xRunningSemaphore[1]);
}

void SpaceExSetSampleInterval(int time)
{
    sampleIntervalTime = time;
}

void SpaceExSetUnstableInterval(int time)
{
    unstableIntervalTime = time;
}

#if (ENABLE_BURNIN_TESTER)
uint32_t GetRadarBurninTestCounter(int spaceIndex)
{
    return radarBurninCounter[spaceIndex];
}

uint32_t GetRadarBurninTestErrorCounter(int spaceIndex)
{
    return radarBurninErrorCounter[spaceIndex];
}

uint32_t GetLidarBurninTestCounter(int spaceIndex)
{
    return lidarBurninCounter[spaceIndex];
}

uint32_t GetLidarBurninTestErrorCounter(int spaceIndex)
{
    return lidarBurninErrorCounter[spaceIndex];
}
#endif

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


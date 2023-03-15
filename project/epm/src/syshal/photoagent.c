/**************************************************************************//**
* @file     photoagent.c
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
#include <time.h>
#include "nuc970.h"
#include "sys.h"
#include "rtc.h"
#include "gpio.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "photoagent.h"
#include "interface.h"
#include "cmdlib.h"
#include "meterdata.h"
#include "dataagent.h"
#include "timelib.h"
#include "pct08cmdlib.h"
#include "pct08drv.h"
#include "powerdrv.h"
#include "modemagent.h"
#include "dataprocesslib.h"
#include "ff.h"
#include "loglib.h"
#if (ENABLE_BURNIN_TESTER)
#include "dipdrv.h"
#include "burnintester.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

#if (ENABLE_BURNIN_TESTER)
static CameraInterface* pCameraInterface = NULL;
static uint32_t cameraBurninCounter[UVCAMERA_NUM] = {0};
static uint32_t cameraBurninPhotoErrorCounter[UVCAMERA_NUM] = {0};
static uint32_t cameraBurninFileErrorCounter[UVCAMERA_NUM] = {0};
#endif

static SemaphoreHandle_t xTakePhotoSemaphore;
static TickType_t threadTakePhotoWaitTime        = portMAX_DELAY;

static uint32_t photoCurrentTime;

static BOOL photoAgentPowerStatus = TRUE;
static BOOL photoAgentIgnoreRun = FALSE;
static BOOL PhotoAgentCheckStatus(int flag);
static BOOL PhotoAgentPreOffCallback(int flag);
static BOOL PhotoAgentOffCallback(int flag);
static BOOL PhotoAgentOnCallback(int flag);
static powerCallbackFunc photoAgentPowerCallabck = {" [PhotoAgent] ", PhotoAgentPreOffCallback, PhotoAgentOffCallback, PhotoAgentOnCallback, PhotoAgentCheckStatus};

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL PhotoAgentPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    photoAgentIgnoreRun = TRUE;
    //sysprintf("### PhotoAgent OFF Callback [%s] ###\r\n", photoAgentPowerCallabck.drvName);    
    return reVal;    
}
static BOOL PhotoAgentOffCallback(int flag)
{
    int timers = 2000/10;
    while(!photoAgentPowerStatus)
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
static BOOL PhotoAgentOnCallback(int flag)
{
    BOOL reVal = TRUE;
    //sysprintf("### PhotoAgent ON Callback [%s] ###\r\n", photoAgentPowerCallabck.drvName);    
    return reVal;    
}
static BOOL PhotoAgentCheckStatus(int flag)
{
    //BOOL reVal = TRUE;
    //sysprintf("### PhotoAgent STATUS Callback [%s] ###\r\n", photoAgentPowerCallabck.drvName); 
    photoAgentIgnoreRun = FALSE;
    return photoAgentPowerStatus;    
}

static BOOL swTestInitPure(void)
{
    pCameraInterface = CameraGetInterface(CAMERA_UVC_INTERFACE_INDEX);
    if (pCameraInterface == NULL)
    {
        sysprintf("usbCamTest ERROR (pCameraInterface == NULL)!!\n");
        return FALSE;
    }
    if (pCameraInterface->initBurningFunc(FALSE) == FALSE)  
    {
        sysprintf("usbCamTest ERROR (pCameraInterface->initBurningFunc(FALSE) == FALSE)!!\n");
        return FALSE;
    }
    return TRUE;
}


static void vPhotoAgentTakePhotoTask( void *pvParameters )
{
    //vTaskDelay(2000/portTICK_RATE_MS); 
    sysprintf("vPhotoAgentTakePhotoTask Going...\r\n");     
    for(;;)
    {     
        photoAgentPowerStatus = TRUE;
        BaseType_t reval = xSemaphoreTake(xTakePhotoSemaphore, threadTakePhotoWaitTime);  
        if(photoAgentIgnoreRun)
        {
            sysprintf("vPhotoAgentTakePhotoTask ignore...\r\n"); 
            continue;
        }
        if(reval != pdTRUE)
        {//timeout

        }
        else
        {
            #if(0)
            {
                uint8_t* photoPr;
                int photoLen = 0;;
                
                char targetFileNameTmp[_MAX_LFN];
                memset(targetFileNameTmp, 0x0, sizeof(targetFileNameTmp));
                if(photoCurrentTime != 0)
                {
                    char timeStr[_MAX_LFN];
                    //sprintf(targetFileNameTmp,"%08d_%010d.%s", GetMeterData()->epmid, photoCurrentTime, PHOTO_FILE_EXTENSION); 
                    if(UTCTimeToString(photoCurrentTime, timeStr))
                    {
                        sprintf(targetFileNameTmp,"%08d_%s.%s", GetMeterData()->epmid, timeStr, PHOTO_FILE_EXTENSION); 
                    }
                }
                else
                {
                    RTC_TIME_DATA_T pt;
                    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
                    {
                        sprintf(targetFileNameTmp,"%08d_%04d%02d%02d%02d%02d_.%s", GetMeterData()->epmid, pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, PHOTO_FILE_EXTENSION);                         
                    }
                    else
                    {
                        sprintf(targetFileNameTmp,"%08d_%010d_.%s", GetMeterData()->epmid, 999999, PHOTO_FILE_EXTENSION); 
                    }
                }
                
                sysprintf("\r\n !! Start PCT08TakePhoto (%s)!!\r\n", targetFileNameTmp); 
                //ModemAgentStartSend(DATA_PROCESS_ID_PHOTO);                
                if(PCT08TakePhoto(&photoPr, &photoLen, PHOTO_SAVE_POSITION, PHOTO_FILE_DIR, targetFileNameTmp))
                {
                    //sysprintf("\r\n !! PCT08TakePhoto OK:  photoLen = %d!!...\r\n", photoLen);
                    //sysprintf("!! PCT08TakePhoto OK:  photoLen = %d!!, start send file...\r\n", photoLen);
                    //#if(BUILD_RELEASE_VERSION)
                    //    ModemAgentStartSend(DATA_PROCESS_ID_PHOTO);
                    //#endif
                    {
                    char str[256];
                        sprintf(str, " ![PHOTO]! PCT08TakePhoto OK [%s] : photoLen = %d !!...\r\n", targetFileNameTmp, photoLen);
                    LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                }
                else
                {
                    //sysprintf("\r\n !! PCT08TakePhoto error:  photoLen = %d!!\r\n");
                    {
                        char str[256];
                        sprintf(str, " ![PHOTO]! PCT08TakePhoto ERROR [%s]...\r\n", targetFileNameTmp);
                        LoglibPrintf(LOG_TYPE_ERROR, str);
                    }
                }
            }
            #endif
        }  

    }
   
}

#if (ENABLE_BURNIN_TESTER)
static void vUVCameraTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    RTC_TIME_DATA_T pt;
    BOOL testLoop = FALSE;
    char targetFileNameTmp[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char targetFilePathTmp[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    uint8_t* ptrPhoto;
    int photoLen = 0;
    int uvcIndex;
    int CamInitCounter = 0;
    terninalPrintf("vUVCameraTestTask Going...\r\n");
    
    while (TRUE)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vUVCameraTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_CAMERA_INTERVAL)
        {
            //terninalPrintf("vUVCameraTestTask heartbeat.\r\n");
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        
        testLoop = FALSE;
        for (uvcIndex = UVCAMERA_INDEX_0 ; uvcIndex <= UVCAMERA_INDEX_1; uvcIndex++)
        {
            RTC_Read(RTC_CURRENT_TIME, &pt);
            sprintf(targetFileNameTmp, "uvc_%d_%08d_%04d%02d%02d%02d%02d%02d.%s", uvcIndex, GetDeviceID(), pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond, PHOTO_FILE_EXTENSION);
            sprintf(targetFilePathTmp, "0:\\%08d\\uvc%d", GetDeviceID(), uvcIndex);
            if (pCameraInterface->takePhotoFunc(uvcIndex, &ptrPhoto, &photoLen, FILE_AGENT_STORAGE_TYPE_FATFS, targetFilePathTmp, targetFileNameTmp, FALSE, 1, 0) == FALSE)
            {
                cameraBurninPhotoErrorCounter[uvcIndex]++;
            }
            else
            {
                //terninalPrintf("vUVCameraTestTask[0], PhotoLength = %d\r\n", photoLen);
                if (FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, targetFilePathTmp, targetFileNameTmp, ptrPhoto, photoLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE) == FILE_AGENT_RETURN_ERROR)
                {
                    cameraBurninFileErrorCounter[uvcIndex]++;
                }
            }
            cameraBurninCounter[uvcIndex]++;
        }
        lastTime = GetCurrentUTCTime();
        
        //CamInitCounter++;
        
        //if(CamInitCounter >= 100)
        //{
            //CamInitCounter = 0;
            vTaskDelay(500 / portTICK_RATE_MS);
            swTestInitPure();
            vTaskDelay(500 / portTICK_RATE_MS);
        //}
    }
}
#endif

static BOOL swInit(void)
{   
    PowerRegCallback(&photoAgentPowerCallabck);
    xTakePhotoSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vPhotoAgentTakePhotoTask, "vPhotoAgentTakePhotoTask", 1024*10, NULL, METER_TAKE_PHOTO_PROCESS_THREAD_PROI, NULL );
    return TRUE;
}

#if (ENABLE_BURNIN_TESTER)
static BOOL swTestInit(void)
{
    //PowerRegCallback(&photoAgentPowerCallabck);
    //if (GPIO_ReadBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN))
    //{
        pCameraInterface = CameraGetInterface(CAMERA_UVC_INTERFACE_INDEX);
        if (pCameraInterface == NULL)
        {
            sysprintf("usbCamTest ERROR (pCameraInterface == NULL)!!\n");
            return FALSE;
        }
        if (pCameraInterface->initFunc(FALSE) == FALSE)
        {
            sysprintf("usbCamTest ERROR (pCameraInterface->initFunc(FALSE) == FALSE)!!\n");
            return FALSE;
        }
        xTaskCreate(vUVCameraTestTask, "vUVCameraTestTask", 1024*10, NULL, CAMERA_TEST_THREAD_PROI, NULL);
    //}
    return TRUE;
}
#endif

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL PhotoAgentInit(BOOL testModeFlag)
{
    sysprintf("PhotoAgentInit!!\n");
   
    if(swInit() == FALSE)
    {
        sysprintf("PhotoAgentInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    
    sysprintf("PhotoAgentInit OK!!\n");
    return TRUE;
}

void PhotoAgentStartTakePhoto(uint32_t currentTime)
{
    //sysprintf(">> PhotoAgentStartTakePhoto!!\n");
    if(xTakePhotoSemaphore == NULL)
        return;
    #if(ENABLE_MODEM_AGENT_DRIVER)
    #else
    FileAgentFatfsDeleteFile("1:", FILE_EXTENSION_EX(PHOTO_FILE_EXTENSION));
    FileAgentFatfsDeleteFile("2:", FILE_EXTENSION_EX(PHOTO_FILE_EXTENSION));
    #endif
    photoAgentPowerStatus = FALSE;
    //if(currentTime == 0)
    //{
    //    currentTime = GetCurrentUTCTime();
    //}
    photoCurrentTime = currentTime;
    xSemaphoreGive(xTakePhotoSemaphore);
}

#if (ENABLE_BURNIN_TESTER)
BOOL UVCameraTestInit(BOOL testModeFlag)
{
    sysprintf("UVCameraTestInit!!\n");
    
    if (swTestInit() == FALSE)
    {
        sysprintf("UVCameraTestInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    
    sysprintf("UVCameraTestInit OK!!\n");
    return TRUE;
}

uint32_t GetCameraBurninTestCounter(int cameraIndex)
{
    return cameraBurninCounter[cameraIndex];
}

uint32_t GetCameraBurninPhotoErrorCounter(int cameraIndex)
{
    return cameraBurninPhotoErrorCounter[cameraIndex];
}

uint32_t GetCameraBurninFileErrorCounter(int cameraIndex)
{
    return cameraBurninFileErrorCounter[cameraIndex];
}
#endif

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


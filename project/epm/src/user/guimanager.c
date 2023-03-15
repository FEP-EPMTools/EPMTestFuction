/**************************************************************************//**
* @file     guimanager.c
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
#include "epddrv.h"
#include "fepconfig.h"
#include "user.h"
#include "guidrv.h"
#include "guimanager.h"
#include "meterdata.h"

/*  GUI Includes    */
//#include "guistandby.h"
//#include "guiselspace.h"
//#include "guiseltime.h"
//#include "guireaderinit.h"
//#include "guideposit.h"
//#include "guidepositok.h"
//#include "guidepositfail.h"
//#include "guireplacebp.h"
//#include "guitester.h"
//#include "guitesterkeypad.h"
//#include "guifiledownload.h"
//#include "guifree.h"
//#include "guioff.h"
#include "guihardwaretest.h"
#include "guisingletest.h"
#include "guitool.h"

#include "guisettingid.h"
#include "guishowcardid.h"
#include "guiradar.h"
#include "guiusbcam.h"
#include "guilidar.h"
#include "guinull.h"
#include "guicalibration.h"
#include "guiversion.h"
#include "guiblank.h"
#include "guiepdflashtool.h"
#include "guiradarotatool.h"
#include "guirtctool.h"
#include "guiradartool.h"
#if (ENABLE_BURNIN_TESTER)
#include "timelib.h"
#include "burnintester.h"
#include "guiburnintester.h"
#endif


/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static SemaphoreHandle_t xSemaphore;

//static GuiInstance guiInstanceStandby[] ={GuiStandbyOnDraw, GuiStandbyUpdateData, GuiStandbyKeyCallback, GuiStandbyTimerCallback, GuiStandbyPowerCallbackFunc}; 
//static GuiInstance guiInstanceSelSpace[] ={GuiSelSpaceOnDraw, NULL, GuiSelSpaceKeyCallback, GuiSelSpaceTimerCallback, GuiSelSpacePowerCallbackFunc}; 
//static GuiInstance guiInstanceSelTime[] ={GuiSelTimeOnDraw, NULL, GuiSelTimeKeyCallback, GuiSelTimeTimerCallback, GuiSelTimePowerCallbackFunc}; 
//static GuiInstance guiInstanceReaderInit[] ={GuiReaderInitOnDraw, NULL, GuiReaderInitKeyCallback, GuiReaderInitTimerCallback, GuiReaderInitPowerCallbackFunc}; 
//static GuiInstance guiInstanceDeposit[] ={GuiDepositOnDraw, NULL, GuiDepositKeyCallback, GuiDepositTimerCallback, GuiDepositPowerCallbackFunc}; 
//static GuiInstance guiInstanceDepositOK[] ={GuiDepositOKOnDraw, NULL, GuiDepositOKKeyCallback, GuiDepositOKTimerCallback, GuiDepositOKPowerCallbackFunc}; 
//static GuiInstance guiInstanceDepositFail[] ={GuiDepositFailOnDraw, NULL, GuiDepositFailKeyCallback, GuiDepositFailTimerCallback, GuiDepositFailPowerCallbackFunc}; 
//static GuiInstance guiInstanceReplaceBP[] ={GuiReplaceBPOnDraw, NULL, GuiReplaceBPKeyCallback, GuiReplaceBPTimerCallback, GuiReplaceBPPowerCallbackFunc}; 
//static GuiInstance guiInstanceTester[] ={GuiTesterOnDraw, NULL, GuiTesterKeyCallback, GuiTesterTimerCallback, GuiTesterPowerCallbackFunc}; 
//static GuiInstance guiInstanceTesterKeypad[] ={GuiTesterKeypadOnDraw, NULL, GuiTesterKeypadKeyCallback, GuiTesterKeypadTimerCallback, GuiTesterKeypadPowerCallbackFunc}; 
//static GuiInstance guiInstanceFileDownload[] ={GuiFileDownloadOnDraw, NULL, GuiFileDownloadKeyCallback, GuiFileDownloadTimerCallback, GuiFileDownloadPowerCallbackFunc}; 
//static GuiInstance guiInstanceFree[] ={GuiFreeOnDraw, NULL, GuiFreeKeyCallback, GuiFreeTimerCallback, GuiFreePowerCallbackFunc}; 
//static GuiInstance guiInstanceOff[] ={GuiOffOnDraw, NULL, GuiOffKeyCallback, GuiOffTimerCallback, GuiOffPowerCallbackFunc}; 
//TODO input function
static GuiInstance guiInstanceHardwareTest[]={GuiHWTestOnDraw, GuiHWTestUpdateData, GuiHWTestKeyCallback, GuiHWTestTimerCallback, GuiHWTestPowerCallbackFunc};
static GuiInstance guiInstanceSingleTest[]  ={GuiSingleTestOnDraw, GuiSingleTestUpdateData, GuiSingleTestKeyCallback, GuiSingleTestTimerCallback, GuiSingleTestPowerCallbackFunc};
static GuiInstance guiInstanceTool[]={GuiToolOnDraw, GuiToolUpdateData, GuiToolKeyCallback, GuiToolTimerCallback, GuiToolPowerCallbackFunc};
static GuiInstance guiInstanceSettingId[] ={GuiSettingIdOnDraw, GuiSettingIdUpdateData, GuiSettingIdKeyCallback, GuiSettingIdTimerCallback, GuiSettingIdPowerCallbackFunc};
static GuiInstance guiInstanceShowCardId[]={GuiShowCardIdOnDraw, GuiShowCardIdUpdateData, GuiShowCardIdKeyCallback, GuiShowCardIdTimerCallback, GuiShowCardIdPowerCallbackFunc};
static GuiInstance guiInstanceRadar[] ={GuiRadarOnDraw, GuiRadarUpdateData, GuiRadarKeyCallback, GuiRadarTimerCallback, GuiRadarPowerCallbackFunc};
static GuiInstance guiInstanceUsbCam[]={GuiUsbCamOnDraw, GuiUsbCamUpdateData, GuiUsbCamKeyCallback, GuiUsbCamTimerCallback, GuiUsbCamPowerCallbackFunc};
static GuiInstance guiInstanceLidar[] ={GuiLidarOnDraw, GuiLidarUpdateData, GuiLidarKeyCallback, GuiLidarTimerCallback, GuiLidarPowerCallbackFunc};
static GuiInstance guiInstanceNull[]={GuiNullOnDraw, GuiNullUpdateData, GuiNullKeyCallback, GuiNullTimerCallback, GuiNullPowerCallbackFunc};
static GuiInstance guiInstanceCalibration[]={GuiCalibrationOnDraw, GuiCalibrationUpdateData, GuiCalibrationKeyCallback, GuiCalibrationTimerCallback, GuiCalibrationPowerCallbackFunc};
static GuiInstance guiInstanceVersion[]={GuiVersionOnDraw, GuiVersionUpdateData, GuiVersionKeyCallback, GuiVersionTimerCallback, GuiVersionPowerCallbackFunc};
static GuiInstance guiInstanceBlank[]={GuiBlankOnDraw, GuiBlankUpdateData, GuiBlankKeyCallback, GuiBlankTimerCallback, GuiBlankPowerCallbackFunc};
static GuiInstance guiInstanceEpdflashTool[]={GuiEpdflashToolOnDraw, GuiEpdflashToolUpdateData, GuiEpdflashToolKeyCallback, GuiEpdflashToolTimerCallback, GuiEpdflashToolPowerCallbackFunc};
static GuiInstance guiInstanceRadarOTATool[]={GuiRadarOTAToolOnDraw, GuiRadarOTAToolUpdateData, GuiRadarOTAToolKeyCallback, GuiRadarOTAToolTimerCallback, GuiRadarOTAToolPowerCallbackFunc};
static GuiInstance guiInstanceBurninTester[]={GuiBurninTesterOnDraw, GuiBurninTesterUpdateData, GuiBurninTesterKeyCallback, GuiBurninTesterTimerCallback, GuiBurninTesterPowerCallbackFunc};
static GuiInstance guiInstanceRTCTool[]={GuiRTCToolOnDraw, GuiRTCToolUpdateData, GuiRTCToolKeyCallback, GuiRTCToolTimerCallback, GuiRTCToolPowerCallbackFunc};
static GuiInstance guiInstanceRadarTool[]={GuiRadarToolOnDraw, GuiRadarToolUpdateData, GuiRadarToolKeyCallback, GuiRadarToolTimerCallback, GuiRadarToolPowerCallbackFunc};


static UserGuiInstance userGuiInstance[] ={ //{GUI_STANDBY_ID, guiInstanceStandby}, 
                                            //{GUI_SEL_SPACE_ID, guiInstanceSelSpace}, 
                                            //{GUI_SEL_TIME_ID, guiInstanceSelTime}, 
                                            //{GUI_READER_INIT_ID, guiInstanceReaderInit}, 
                                            //{GUI_DEPOSIT_ID, guiInstanceDeposit}, 
                                            //{GUI_DEPOSIT_OK_ID, guiInstanceDepositOK}, 
                                            //{GUI_DEPOSIT_FAIL_ID, guiInstanceDepositFail}, 
                                            //{GUI_REPLACE_BP_ID, guiInstanceReplaceBP}, 
                                            //{GUI_TESTER_ID, guiInstanceTester},
                                            //{GUI_TESTER_KEYPAD_ID, guiInstanceTesterKeypad},
                                            //{GUI_FILE_DOWNLOAD_ID, guiInstanceFileDownload},
                                            //{GUI_FREE_ID, guiInstanceFree},
                                            //{GUI_OFF_ID, guiInstanceOff},
                                            {GUI_HW_TEST_ID,guiInstanceHardwareTest},
                                            {GUI_SINGLE_TEST_ID,guiInstanceSingleTest},
                                            {GUI_TOOL_TEST_ID,guiInstanceTool},
                                            {GUI_SETTING_ID,guiInstanceSettingId},
                                            {GUI_SHOW_CARD_ID,guiInstanceShowCardId},
                                            {GUI_RADAR_ID,guiInstanceRadar},
                                            {GUI_USB_CAM_ID,guiInstanceUsbCam},
                                            {GUI_LIDAR_ID,guiInstanceLidar},
                                            {GUI_CALIBRATION_ID,guiInstanceCalibration},
                                            {GUI_NULL_ID,guiInstanceNull},
											{GUI_VERSION_ID,guiInstanceVersion},
                                            {GUI_BLANK_ID,guiInstanceBlank},
                                            {GUI_EPDFLASH_TOOL_ID,guiInstanceEpdflashTool},
                                            {GUI_RADAROTA_TOOL_ID,guiInstanceRadarOTATool},
                                            {GUI_BURNIN_TESTER_ID,guiInstanceBurninTester},
                                            {GUI_RTC_TOOL_ID,guiInstanceRTCTool},
                                            {GUI_RADAR_TOOL_ID,guiInstanceRadarTool},
                                            {0xff, NULL}}; 

static UserGuiInstance* pCurrentGuiInstance = NULL;
static int currentPara;
static int currentPara2;
static int currentPara3;
static uint8_t currentGuiId;
                                            

#if (ENABLE_BURNIN_TESTER)
static uint32_t epdBurninCounter = 0;
static uint32_t epdBurninErrorCounter = 0;
static BOOL epdErrorFlag = FALSE;
#endif

                                            
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void redrawscreen(void)
{
    GuiManagerRefreshScreen();
}

#if (ENABLE_BURNIN_TESTER)
static void vGuiManagerTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    terninalPrintf("vGuiManagerTestTask Going...\r\n");
    
    for(;;)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vGuiManagerTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_EPD_REFRESH_INTERVAL)
        {
            //terninalPrintf("vGuiManagerTestTask heartbeat.\r\n");
            //lastTime = currentTime;
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        testLoop = FALSE;
        if (epdErrorFlag == FALSE)
        {
            
            EPDSetBacklight(TRUE);
            GuiManagerShowScreen(GUI_NULL_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
            EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE);
            vTaskDelay(1000 / portTICK_RATE_MS);
                        //EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
            epdBurninCounter++;
                        //terninalPrintf("EpdBurninCounter = %d\r\n", GetEpdBurninTestCounter());
            GuiManagerShowScreen(GUI_BURNIN_TESTER_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
            EPDSetBacklight(FALSE);
            
        }
        else
        {
            epdBurninCounter++;
            epdBurninErrorCounter++;
        }
        lastTime = GetCurrentUTCTime();
    }
}
#endif


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/

#if (ENABLE_BURNIN_TESTER)
uint32_t GetEpdBurninTestCounter(void)
{
    return epdBurninCounter;
}

uint32_t GetEpdBurninTestErrorCounter(void)
{
    return epdBurninErrorCounter;
}

void SetEpdErrorFlag(BOOL flag)
{
    epdErrorFlag = flag;
}
#endif


BOOL GuiManagerInit(void)
{   
    //EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, FALSE); 
    //EPDDrawAllScreen(FALSE);
    //EPDDrawAllDepositTime(TRUE);
    EPDSetReinitCallbackFunc(redrawscreen);
    xSemaphore = xSemaphoreCreateMutex();
    switch(GetMeterData()->runningStatus)
    {
        case RUNNING_STATUS_INIT:
//            GuiManagerShowScreen(GUI_HW_TEST_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
            //GuiManagerShowScreen(GUI_FILE_DOWNLOAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);  
            GetMeterData()->runningStatus = RUNNING_STATUS_NORMAL;
            break;
        case RUNNING_STATUS_FILE_DOWNLOAD:
            #if(ENABLE_MODEM_AGENT_DRIVER)
            GuiManagerShowScreen(GUI_FILE_DOWNLOAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
            #else
            GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
            //GuiManagerShowScreen(GUI_FILE_DOWNLOAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
            GetMeterData()->runningStatus = RUNNING_STATUS_NORMAL;
            #endif
            break;
        case RUNNING_STATUS_ERROR:
            break;
        case RUNNING_STATUS_NORMAL:
            break;
    }
     
#if (ENABLE_BURNIN_TESTER)
    if (EnabledBurninTestMode())
    {
        xTaskCreate(vGuiManagerTestTask, "vGuiManagerTestTask", 1024*20, NULL, GUI_MANAGER_THREAD_PROI, NULL);
    }
#endif
    
    return TRUE;
}

uint8_t GuiManagerShowScreen(uint8_t guiId, uint8_t para, int para2, int para3)
{
    int i;
    uint8_t oriGuid;
    uint8_t reVal = GUI_SHOW_SCREEN_ERROR;
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    
    //sysprintf("  ==> GUIManagerShowScreen %d start...\r\n", guiId);
    if(pCurrentGuiInstance != NULL)
    {
        oriGuid = pCurrentGuiInstance->guiId;
    }
    else
    {
        oriGuid = NULL;
    }
    for(i = 0; ; i++)
    {
        //sysprintf("  ==> check %d start...\r\n", guiId);
        if(userGuiInstance[i].guiId == 0xff)
        {
            break;
        }
        else 
        {
            if(userGuiInstance[i].guiId == guiId)
            {
                if(userGuiInstance[i].guiId == pCurrentGuiInstance->guiId)
                {
                    //GuiManagerRefreshScreen();
                    sysprintf("  ==> GUIShowScreen Get it, but the same --> ignore...\r\n");
                    reVal = GUI_SHOW_SCREEN_IGNORE;
                }
                else
                {
                    //sysprintf("  ==> GUIShowScreen Get it...\r\n");
                    pCurrentGuiInstance = &userGuiInstance[i];
                    GuiSetInstance(&userGuiInstance[i], oriGuid, para, para2, para3);
                    currentPara =para;
                    currentPara2=para2;
                    currentPara3=para3;
                    currentGuiId=pCurrentGuiInstance->guiId;
                    reVal = GUI_SHOW_SCREEN_OK;
                }
                break;
            }
        }
    }
    xSemaphoreGive(xSemaphore);
    return reVal;
}

BOOL GuiManagerRefreshScreen(void)
{
    if(pCurrentGuiInstance != NULL)
    {
        sysprintf("\r\n  = ############ => GuiManagerRefreshScreen start...\r\n");
        pCurrentGuiInstance->guiInstance->onDraw(currentGuiId, GUI_REDRAW_PARA_REFRESH, currentPara2, currentPara3);
        sysprintf("  = ############ => GuiManagerRefreshScreen END...\r\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL GuiManagerTimerSet(uint8_t reFreshPara)
{
    if(pCurrentGuiInstance != NULL)
    {
        sysprintf("\r\n  = ############ => GuiManagerTimerSet start...\r\n");
        pCurrentGuiInstance->guiInstance->onDraw(currentGuiId, reFreshPara, currentPara2, currentPara3);
        sysprintf("  = ############ => GuiManagerTimerSet END...\r\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL GuiManagerCleanMessage(uint8_t reFreshPara)
{
    if(pCurrentGuiInstance != NULL)
    {
        sysprintf("\r\n  = ############ => GuiManagerCleanMessage start...\r\n");
        pCurrentGuiInstance->guiInstance->onDraw(currentGuiId, reFreshPara, currentPara2, currentPara3);
        sysprintf("  = ############ => GuiManagerCleanMessage END...\r\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL GuiManagerUpdateMessage(uint8_t reFreshPara,int UpdatePara2,int UpdatePara3)
{
    if(pCurrentGuiInstance != NULL)
    {
        sysprintf("\r\n  = ############ => GuiManagerCleanMessage start...\r\n");
        pCurrentGuiInstance->guiInstance->onDraw(currentGuiId, reFreshPara, UpdatePara2, UpdatePara3);
        sysprintf("  = ############ => GuiManagerCleanMessage END...\r\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL GuiManagerUpdateScreen(void)
{
    if(pCurrentGuiInstance != NULL)
    {
        sysprintf("\r\n  = ############ => GuiManagerUpdateScreen start...\r\n");
        if(pCurrentGuiInstance->guiInstance->updateData != NULL)
            pCurrentGuiInstance->guiInstance->updateData();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL GuiManagerCompareCurrentScreenId(uint8_t guiId)
{
    if(guiId == pCurrentGuiInstance->guiId)
    {        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL GuiManagerResetInstance(void)
{
#if (ENABLE_BURNIN_TESTER)
    if (EnabledBurninTestMode() == FALSE)
#endif
    {
        GUIDrvInit(FALSE);
    }
    if(pCurrentGuiInstance == NULL)
    {
        return FALSE;
    }
    else
    {
        for(int i = 0; ; i++)
        {
            if(userGuiInstance[i].guiId == 0xff)
                return FALSE;
            else 
            {
                if(userGuiInstance[i].guiId == currentGuiId)
                    return GuiSetInstance(&userGuiInstance[i],currentGuiId, currentPara, currentPara2, currentPara3);
            }
        }
    }
}

BOOL GuiManagerOnclickListener(guiKeyCallbackFunc listener){
    if(pCurrentGuiInstance == NULL)
    {
        return FALSE;
    }
    if(listener == NULL)
    {
        return FALSE;
    }
    pCurrentGuiInstance->guiInstance->keyCallback = listener;
    return TRUE;
}

BOOL GuiManagerResetKeyCallbackFunc(void)
{
    if(pCurrentGuiInstance == NULL)
    {
        terninalPrintf("[ERROR]<GUIManager>ResetKeyCallbackFunc Fail [pCurrentGuiInstance is NULL]\r\n");
        return FALSE;
    }
    else 
    {
        //terninalPrintf( "pCurrentGuiInstance->guiId [%d]\r\n" , pCurrentGuiInstance->guiId );
        //terninalPrintf( "currentGuiId               [%d]\r\n" , currentGuiId );
        for(int i = 0; ; i++)
        {
            if(userGuiInstance[i].guiId == 0xff)
                return FALSE;
            else 
            {
                if(userGuiInstance[i].guiId == currentGuiId)
                    GuiSetKeyCallbackFunc(userGuiInstance[i].guiInstance->keyCallback);
                    GUIDrvInit(FALSE);
                return TRUE;
            }
        }
    }
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


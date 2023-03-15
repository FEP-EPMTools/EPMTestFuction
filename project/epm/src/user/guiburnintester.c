/**************************************************************************//**
* @file     guiburnintester.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief For EPD Burning Test   
*
* @note
* Copyright (C) 2020 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"
#include "gpio.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "guidrv.h"
#include "halinterface.h"
#include "epddrv.h"
#include "guimanager.h"
#include "spaceexdrv.h"
#include "nt066edrv.h"
#include "modemagent.h"
#include "smartcarddrv.h"
#include "cardreader.h"
#include "burnintester.h"
#include "guiburnintester.h"
#include "photoagent.h"
#include "dipdrv.h"
#include "creditReaderDrv.h"

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define EXIT_TIMER          GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   (BURNIN_RTC_INTERVAL/portTICK_RATE_MS)
#define EXIT_INTERVAL          portMAX_DELAY    //5000/portTICK_RATE_MS  // 
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = TRUE;
static BOOL keyIgnoreFlag = FALSE;
static uint8_t pageIndex = 1;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void updateData(void)
{
    if (GetPrepareStopBurninFlag() && GetBurninTerminatedFlag()) {
        return;
    }
    char stringBuffer[32];
    RTC_TIME_DATA_T pt;
    uint32_t hours, minutes, seconds;
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        sprintf(stringBuffer, "%04d\\%02d\\%02d %02d:%02d:%02d", pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond);
        EPDDrawString(FALSE, stringBuffer, 440, 60);
        GetBurninTestRuntime(&hours, &minutes, &seconds);
        sprintf(stringBuffer, "%02d:%02d:%02d", hours, minutes, seconds);
        EPDDrawString(FALSE, stringBuffer, 640, 110);
    }
    
    if (pageIndex == 0)
    {
        sprintf(stringBuffer, "%d:%d", GetLedBurninTestCounter(), GetLedBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 160);
        sprintf(stringBuffer, "%d:%d", GetEpdBurninTestCounter(), GetEpdBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 210);
        sprintf(stringBuffer, "%d:%d", GetRadarBurninTestCounter(VOS_INDEX_0), GetRadarBurninTestErrorCounter(VOS_INDEX_0));
        EPDDrawString(FALSE, stringBuffer, 640, 260);
        sprintf(stringBuffer, "%d:%d", GetRadarBurninTestCounter(VOS_INDEX_1), GetRadarBurninTestErrorCounter(VOS_INDEX_1));
        EPDDrawString(FALSE, stringBuffer, 640, 310);
        sprintf(stringBuffer, "%d:%d", GetLidarBurninTestCounter(VOS_INDEX_0), GetLidarBurninTestErrorCounter(VOS_INDEX_0));
        EPDDrawString(FALSE, stringBuffer, 640, 360);
        sprintf(stringBuffer, "%d:%d", GetLidarBurninTestCounter(VOS_INDEX_1), GetLidarBurninTestErrorCounter(VOS_INDEX_1));
        EPDDrawString(FALSE, stringBuffer, 640, 410);
        sprintf(stringBuffer, "%d:%d", GetBatteryBurninTestCounter(BATTERY_INDEX_0), GetBatteryBurninTestErrorCounter(BATTERY_INDEX_0));
        EPDDrawString(FALSE, stringBuffer, 640, 460);
        sprintf(stringBuffer, "%d:%d", GetBatteryBurninTestCounter(BATTERY_INDEX_1), GetBatteryBurninTestErrorCounter(BATTERY_INDEX_1));
        EPDDrawString(FALSE, stringBuffer, 640, 510);
        sprintf(stringBuffer, "%d", GetBatteryLastADCValue(BATTERY_INDEX_0));
        EPDDrawString(FALSE, stringBuffer, 500, 460);
        sprintf(stringBuffer, "%d", GetBatteryLastADCValue(BATTERY_INDEX_1));
        EPDDrawString(FALSE, stringBuffer, 500, 510);
        sprintf(stringBuffer, "%d:%d", GetBatteryBurninTestCounter(BATTERY_INDEX_SOLAR), GetBatteryBurninTestErrorCounter(BATTERY_INDEX_SOLAR));
        EPDDrawString(FALSE, stringBuffer, 640, 560);
        sprintf(stringBuffer, "%d:%d", GetNT066EBurninTestCounter(), GetNT066EBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 610);
        sprintf(stringBuffer, "%d:%d", GetModemATBurninTestCounter(), GetModemATBurninTestErrorCounter());
        EPDDrawString(TRUE, stringBuffer, 640, 660);
    }
    else
    {
        sprintf(stringBuffer, "%d:%d:%d", GetModemFTPBurninTestCounter(), GetModemDialupBurninTestErrorCounter(), GetModemFTPBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 160);
        sprintf(stringBuffer, "%d:%d", GetSmartCardBurninTestCounter(), GetSmartCardBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 210);
        sprintf(stringBuffer, "%d:%d", GetCardReaderBurninTestCounter(), GetCardReaderBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 260);
        sprintf(stringBuffer, "%d:%d", GetCreditReaderBurninTestCounter(), GetCreditReaderBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 310);
        sprintf(stringBuffer, "%d:%d", GetSDCardBurninTestCounter(), GetSDCardBurninTestErrorCounter());
        EPDDrawString(FALSE, stringBuffer, 640, 360);
        sprintf(stringBuffer, "%d:%d", GetNandFlashBurninTestCounter(FLASH_INDEX_0), GetNandFlashBurninTestErrorCounter(FLASH_INDEX_0));
        EPDDrawString(FALSE, stringBuffer, 640, 410);
        sprintf(stringBuffer, "%d:%d", GetNandFlashBurninTestCounter(FLASH_INDEX_1), GetNandFlashBurninTestErrorCounter(FLASH_INDEX_1));
        EPDDrawString(FALSE, stringBuffer, 640, 460);
        sprintf(stringBuffer, "%d:%d", GetNandFlashBurninTestCounter(FLASH_INDEX_2), GetNandFlashBurninTestErrorCounter(FLASH_INDEX_2));
        EPDDrawString(TRUE, stringBuffer, 640, 510);
        //if (GPIO_ReadBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN))
        //{
            sprintf(stringBuffer, "%d:%d:%d", GetCameraBurninTestCounter(UVCAMERA_INDEX_0), GetCameraBurninPhotoErrorCounter(UVCAMERA_INDEX_0), GetCameraBurninFileErrorCounter(UVCAMERA_INDEX_0));
            EPDDrawString(FALSE, stringBuffer, 640, 560);
            sprintf(stringBuffer, "%d:%d:%d", GetCameraBurninTestCounter(UVCAMERA_INDEX_1), GetCameraBurninPhotoErrorCounter(UVCAMERA_INDEX_1), GetCameraBurninFileErrorCounter(UVCAMERA_INDEX_1));
            EPDDrawString(TRUE, stringBuffer, 640, 610);
        //}
    }
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiBurninTesterOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    char stringBuffer[64];
    powerStatus = TRUE;
    if (pageIndex == 1) {
        pageIndex = 0;
    }
    else {
        pageIndex = 1;
    }
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    
    EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
    EPDDrawString(FALSE, "EPM Burn-In Test Program", 100, 10);
    EPDDrawString(FALSE, "RTC:", 160, 60);
    EPDDrawString(FALSE, "Runtime:", 160, 110);
    if (pageIndex == 0)
    {
        EPDDrawString(FALSE, "LED-Buzzer:", 160, 160);
        EPDDrawString(FALSE, "EPD-Backlight:", 160, 210);
        EPDDrawString(FALSE, "Radar 1:", 160, 260);
        EPDDrawString(FALSE, "Radar 2:", 160, 310);
        EPDDrawString(FALSE, "Lidar 1:", 160, 360);
        EPDDrawString(FALSE, "Lidar 2:", 160, 410);
        EPDDrawString(FALSE, "Battery 1:", 160, 460);
        EPDDrawString(FALSE, "Battery 2:", 160, 510);
        EPDDrawString(FALSE, "Solar Battery:", 160, 560);
        EPDDrawString(FALSE, "Touch IC:", 160, 610);
        EPDDrawString(FALSE, "Modem AT:", 160, 660);
    }
    else
    {
        EPDDrawString(FALSE, "Modem FTP:", 160, 160);
        EPDDrawString(FALSE, "SmartCard Slot:", 160, 210);
        EPDDrawString(FALSE, "Card Reader:", 160, 260);
        EPDDrawString(FALSE, "Credit Card:", 160, 310);
        EPDDrawString(FALSE, "SD Card Reader:", 160, 360);
        EPDDrawString(FALSE, "SPI Flash 1:", 160, 410);
        EPDDrawString(FALSE, "SPI Flash 2:", 160, 460);
        EPDDrawString(FALSE, "SPI Flash 3:", 160, 510);
        //if (GPIO_ReadBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN))
        //{
            EPDDrawString(FALSE, "Camera 1:", 160, 560);
            EPDDrawString(FALSE, "Camera 2:", 160, 610);
        //}
        //else
        //{
          // EPDDrawString(FALSE, "Camera 1: Not Available", 160, 560);
          //  EPDDrawString(FALSE, "Camera 2: Not Available", 160, 610);
        //}
    }
    
    //sprintf(stringBuffer, "Program Version: %s  Page %d", BURNIN_TEST_VERSION, (pageIndex+1));
    sprintf(stringBuffer, "Program Version: %d.%02d.%02d  Page %d", MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, (pageIndex+1));
    EPDDrawString(TRUE, stringBuffer, 90, 710);
    
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    return TRUE;
}

void GuiBurninTesterStop(void)
{
    EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
    EPDDrawString(TRUE, "EPM Burn-In Test Stopped ...", 100, 100);
    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, portMAX_DELAY);
}

BOOL GuiBurninTesterUpdateData(void)
{
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
    return TRUE;
}

BOOL GuiBurninTesterKeyCallback(uint8_t keyId, uint8_t downUp)
{
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <BurninTester> Key:  ignore...\n"); 
        return reVal;
    }
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        
    }
    else
    {
        
    }
    return reVal;
}

BOOL GuiBurninTesterTimerCallback(uint8_t timerIndex)
{
    keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:
            break;
        case UPDATE_DATA_TIMER:
            updateData();
            break;
        case EXIT_TIMER:
            break;
    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiBurninTesterPowerCallbackFunc(uint8_t type, int flag)
{
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2020 Far Easy Pass LTD. All rights reserved. ***/

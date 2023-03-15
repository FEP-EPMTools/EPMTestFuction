/**************************************************************************//**
* @file     guinull.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief For EPD Burning Test   
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
#include "guidrv.h"
#include "halinterface.h"
#include "guifiledownload.h"
#include "epddrv.h"
#include "guimanager.h"
#include "tarifflib.h"
#include "meterdata.h"
#include "hwtester.h"
#include "guiblank.h"
#include "gpio.h"
#include "leddrv.h"
#include "i2c1drv.h"
#include "MtpProcedure.h"

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define EXIT_TIMER          GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   portMAX_DELAY
#define EXIT_INTERVAL         portMAX_DELAY    //5000/portTICK_RATE_MS  // 
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//static void updateData(void);

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = TRUE;
static BOOL keyIgnoreFlag = FALSE;
static char chrtemp;
static BOOL touchflag = FALSE;

static BOOL key1flag = FALSE;
static BOOL key2flag = FALSE;
static BOOL key3flag = FALSE;
static BOOL key4flag = FALSE;
static BOOL key5flag = FALSE;
static BOOL key6flag = FALSE;
static uint8_t tempColorAllGreen[8]= {LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};
static uint8_t tempColorOff[8] =     {LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};

static uint8_t TempreFreshPara;

static uint8_t MTPstatus;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
//static void updateBG(void)
//{
//    TickType_t tickLocalStart = xTaskGetTickCount();
//    //sysprintf(" [INFO GUI] <Free> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
//    // sysprintf(" [INFO GUI] <Free> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);    
// 
//}
//static void updateData(void)
//{
//    TickType_t tickLocalStart = xTaskGetTickCount();
//    //sysprintf(" [INFO GUI] <Free> updateData enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart);  
//    
//    //sysprintf(" [INFO GUI] <Free> updateData: [%d]. Key tick = ![%d]!\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - keyStart);
//    //sysDelay(100);

//}

static void updateData(void)
{
    MTP_WaitingStartMessage();
}


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiBlankOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3)
{
    TempreFreshPara = reFreshPara;
    touchflag = FALSE;
    int counter=0;
//    tickStart = xTaskGetTickCount();
    // sysprintf(" [INFO GUI] <Free> OnDraw (from GuiId = %d, reFreshPara = %d, para2 = %d, para3 = %d)\n", oriGuiId, reFreshPara, para2, para3);   

    powerStatus = TRUE;
    
    if(reFreshPara == GUI_MTP_SCREEN)
    {
        MTPstatus = GUI_MTP_SCREEN;
        return TRUE;
    }
    else if(reFreshPara == GUI_MTP_START)
    {
        terninalPrintf("GUI_MTP_START\r\n");
        MTPstatus = GUI_MTP_START;
        return TRUE;
    }
    
    pGuiGetInterface = GuiGetInterface();
    pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);  
    //if((reFreshPara == GUI_MTP_SCREEN) || (reFreshPara == GUI_MTP_START))
    if(reFreshPara == GUI_MTP_INI)
    {
        pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, 1000/portTICK_RATE_MS);
    }
    else
    {
        pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
    }        
    pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    
    pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
    EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    
    if(reFreshPara == GUI_KEYPAD_TEST)
    {
        EPDDrawString(TRUE,"KEYPAD TEST \nPress SW4 to quit.\nPress a to set input pin.\nPress b to set I2C pin.",100,100);
    }
    else if(reFreshPara == GUI_MTP_INI)
    {
        MTPstatus = GUI_MTP_SCREEN;
        return TRUE;
    }
    else
        EPDDrawString(TRUE,"Hello World",100,100);
    
    
    
    
    
    

/*while(1)
{
    while(sysIsKbHit())
    {//empty registor buffer
    #include <TMPA900.H>
        sysGetChar();
    }
    while(1)
    {//wait user respone
        vTaskDelay(100/portTICK_RATE_MS);
        if(sysIsKbHit())
        {
           chrtemp = sysGetChar();
           char* stringtmp = malloc(2);
           *stringtmp = chrtemp;
           *(stringtmp+1) = '\0';
           terninalPrintf("ishit=%s \n",stringtmp);
           EPDDrawString(TRUE,stringtmp,100,150);
           break;
        }


    }
    if(chrtemp=='q')
      {
            break;
      }
  }*/
    
    
    


    if(reFreshPara == GUI_KEYPAD_TEST)
    {
        key1flag = FALSE;
        key2flag = FALSE;
        key3flag = FALSE;
        key4flag = FALSE;
        key5flag = FALSE;
        key6flag = FALSE;
        
        if(GPIO_ReadBit(GPIOI,BIT3))
        {
            while(1)
            {
                if(sysIsKbHit())
                {
                    if(sysGetChar() == 'a')
                    {
                        terninalPrintf("Change I2C to input pin.\r\n");
                        BOOL pin2changeflag = TRUE;
                        BOOL pin3changeflag = TRUE;
                        I2c1ResetInputPin();
                        while(1)
                        {
                            if(!(GPIO_ReadBit(GPIOG,BIT2)) && (pin2changeflag == TRUE))
                            {   
                                pin2changeflag = FALSE;
                                terninalPrintf("GPIOG PIN2 LOW.\r\n");
                            }
                            else if((GPIO_ReadBit(GPIOG,BIT2)) && (pin2changeflag == FALSE))
                            {
                                pin2changeflag = TRUE;
                                terninalPrintf("GPIOG PIN2 HIGH.\r\n");
                            }
                                
                            
                            if(!(GPIO_ReadBit(GPIOG,BIT3)) && (pin3changeflag == TRUE))
                            {   
                                pin3changeflag = FALSE;
                                terninalPrintf("GPIOG PIN3 LOW.\r\n");
                            }
                            else if((GPIO_ReadBit(GPIOG,BIT3)) && (pin3changeflag == FALSE))
                            {
                                pin3changeflag = TRUE;
                                terninalPrintf("GPIOG PIN3 HIGH.\r\n");
                            }
                            
                            
                            if(sysIsKbHit())
                            {
                                if(sysGetChar() == 'b')
                                {
                                    terninalPrintf("Set I2C pin.\r\n");
                                    I2c1SetPin();
                                    break;
                                }
                                vTaskDelay(100/portTICK_RATE_MS);
                            }
                        }
                    }
                }
                
                if((key1flag == TRUE) && (key2flag == TRUE) && (key3flag == TRUE) && (key4flag == TRUE) && (key5flag == TRUE) && (key6flag == TRUE))
                {
                    key1flag = FALSE;
                    key2flag = FALSE;
                    key3flag = FALSE;
                    key4flag = FALSE;
                    key5flag = FALSE;
                    key6flag = FALSE;
                    LedSetColor(tempColorAllGreen, LIGHT_COLOR_OFF, TRUE);
                    vTaskDelay(1000/portTICK_RATE_MS);
                    LedSetColor(tempColorOff, LIGHT_COLOR_OFF, TRUE);
                }
                if(!GPIO_ReadBit(GPIOI,BIT3))
                    break;
                vTaskDelay(100/portTICK_RATE_MS);
            }
        }
        else
        {
            while(1)
            {
                if((key1flag == TRUE) && (key2flag == TRUE) && (key3flag == TRUE) && (key4flag == TRUE) && (key5flag == TRUE) && (key6flag == TRUE))
                {
                    key1flag = FALSE;
                    key2flag = FALSE;
                    key3flag = FALSE;
                    key4flag = FALSE;
                    key5flag = FALSE;
                    key6flag = FALSE;
                    LedSetColor(tempColorAllGreen, LIGHT_COLOR_OFF, TRUE);
                    vTaskDelay(1000/portTICK_RATE_MS);
                    LedSetColor(tempColorOff, LIGHT_COLOR_OFF, TRUE);
                }
                if(GPIO_ReadBit(GPIOI,BIT3))
                    break;
                vTaskDelay(100/portTICK_RATE_MS);
            }
        }
        
        
    }
    else
    {

    
        while(1){
            vTaskDelay(100/portTICK_RATE_MS);
            BOOL flag = sysIsKbHit(); 
            //terninalPrintf("ishit=%d \n",flag);
            if(flag  || touchflag)
             {
                 counter++;
                 if (!touchflag)
                 {
                    chrtemp=sysGetChar();
                 }
                 //terninalPrintf("ishit=%c \n",chrtemp);
               if(chrtemp==0x1b)
               {
                break;
               }
               char* stringtmp = malloc(2);
               *stringtmp = chrtemp;
               *(stringtmp+1) = '\0';
               terninalPrintf("ishit=%s \n",stringtmp);
               EPDDrawString(TRUE,stringtmp,100+counter*50,150);
               vTaskDelay(100/portTICK_RATE_MS);
               free(stringtmp);
             }
        }
    }
    //sysprintf(" [INFO GUI] <Free> OnDraw exit: cost ticks = %d\n", xTaskGetTickCount() - tickStart);
    return TRUE;
}
BOOL GuiBlankUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更updateScreen
    return TRUE;
}
BOOL GuiBlankKeyCallback(uint8_t keyId, uint8_t downUp)
{
    //sysprintf(" [INFO GUI] <Free> Key:  keyId = %d, downUp = %d\n", keyId, downUp);   
    BOOL reVal = FALSE; 
    if(keyIgnoreFlag)
    {
        sysprintf(" [INFO GUI] <Free> Key:  ignore...\n"); 
        return reVal;
    }
    //pGuiGetInterface->setTimeoutFunc(EXIT_TIMER, EXIT_INTERVAL); 
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_ONE:
                if(TempreFreshPara == GUI_KEYPAD_TEST)
                {
                    key1flag = TRUE;
                    reVal = TRUE;
                }
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_TWO:
                if(TempreFreshPara == GUI_KEYPAD_TEST)
                {
                    key2flag = TRUE;
                    reVal = TRUE;
                }
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_THREE:
                if(TempreFreshPara == GUI_KEYPAD_TEST)
                {
                    key3flag = TRUE;
                    reVal = TRUE;
                }
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_FOUR:
                if(TempreFreshPara == GUI_KEYPAD_TEST)
                {
                    key4flag = TRUE;
                    reVal = TRUE;
                }
                //SetGuiResponseVal('q');
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_FIVE:
                if(TempreFreshPara == GUI_KEYPAD_TEST)
                {
                    key5flag = TRUE;
                    reVal = TRUE;
                }
                //reVal = TRUE;
                break;
            case GUI_KEYPAD_SIX:
                if(TempreFreshPara == GUI_KEYPAD_TEST)
                {
                    key6flag = TRUE;
                    reVal = TRUE;
                }
                else
                {  
                    SetGuiResponseVal('q');
                
                    //chrtemp='q';
                
                    chrtemp=0x1b;
                    touchflag = TRUE;
                    reVal = TRUE;
                }
                break;
            case GUI_KEYPAD_REPLACE_BP_ID:
                //EPDReSetBacklightTimeout(portMAX_DELAY);
                //GuiManagerShowScreen(GUI_REPLACE_BP_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
            
            case GUI_KEYPAD_TESTER_ID:
                //EPDReSetBacklightTimeout(portMAX_DELAY);
                //GuiManagerShowScreen(GUI_TESTER_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
            
             case GUI_KEYPAD_TESTER_KEYPAD_ID:
                //EPDReSetBacklightTimeout(portMAX_DELAY);
                //GuiManagerShowScreen(GUI_TESTER_KEYPAD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
                //reVal = TRUE;
                break;
        }
    }
    else
    {
        
    }
   // setPrintfFlag(FALSE);
    return reVal;
}
BOOL GuiBlankTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Free> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    keyIgnoreFlag = TRUE;
    switch(timerIndex)
    {
        case UPDATE_BG_TIMER:

            break;
        case UPDATE_DATA_TIMER:
            if(MTPstatus == GUI_MTP_SCREEN)
            {
                terninalPrintf("ReadyGoMTPScreen\r\n");
            }
            else if(MTPstatus == GUI_MTP_START)
            {
                
                //terninalPrintf("MTPstatus == GUI_MTP_START\r\n");
                
                if (MTP_GetProcedureFlag() == FALSE) 
                {
                    updateData();
                }
                else
                {
                    pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, portMAX_DELAY);
                    //if (MTP_GetSwitchRTCFlag()) {
                    //    EPDDrawString(TRUE, "Start RTC Procedure ...", 100, 100);
                    //}
                    //else {
                        //EPDDrawString(TRUE, "Start MTP Procedure ...", 100, 100);
                    //}
                }
            }

            break;
        case EXIT_TIMER:
            //GuiManagerShowScreen(GUI_STANDBY_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);  
            SetGuiResponseVal('q');
            break;

    }
    keyIgnoreFlag = FALSE;
    return TRUE;
}

BOOL GuiBlankPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] <Free> power [%d] : flag = %d!!\n", type, flag);
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

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


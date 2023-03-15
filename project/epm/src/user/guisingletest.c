
/**************************************************************************//**
* @file     guisingletest.c
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
#include "guidrv.h"
#include "halinterface.h"
#include "guisingletest.h"
#include "epddrv.h"
#include "powerdrv.h"
#include "paralib.h"
#include "meterdata.h"
#include "cardreader.h"
#include "spacedrv.h"
#include "tarifflib.h"
#include "hwtester.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define UPDATE_SPACE_DETECT_TIMER   GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL            ((50)/portTICK_RATE_MS)
#define UPDATE_DATA_INTERVAL_SMODE      ((100)/portTICK_RATE_MS)
#define UPDATE_SPACE_DETECT_INTERVAL    ((10*1000)/portTICK_RATE_MS)

#define ALL_TEST_MODE 1

#define TITLE_HIGH 100
#define EPD_WIDTH 1024
#define EPD_HEIGHT  758
#define MENU_WIDTH 920
#define X_SINGLE_ITEM   X_AFTER_SHIFT
#define X_RESULT_SHIFT  1024-500
#define Y_RESULT_SHIFT  TITLE_HIGH
#define Y_RESULT_HIGH          35
#define Y_ITEM_HIGH     40
#define X_BTM_MSG_BAR   550
#define Y_BTM_MSG_BAR   EPD_HEIGHT-100
#define X_SELECT_ITEM   90
#define Y_SELECT_ITEM   104
#define X_HEAD_TITLE    90
#define Y_HEAD_TITLE    28

#define SECOND_SECTION_INDEX 5 //!!!change hw actionTestItem too!!!
#define SECOND_SECTION_Y_OFFSET 0//28

#define STRING_HEIGHT   44

#define X_MID_LINE  535
#define Y_MID_LINE  104

#define MAX_SELECT_ITEM maxSelectItem

#define CAD_TIMEOUT_SECOND 10

/*-----------------------------------------*/
/* global file scope (static) variables     */
/*-----------------------------------------*/
static int maxSelectItem = 0;

static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;
static BOOL cleanMsgAfterMove = FALSE;

static BOOL keyIgnoreFlag = FALSE;


static int nowIndex=0,oldIndex=0;


static BOOL CADtimerEnFlag = FALSE;
static BOOL CADtimeoutFlag = FALSE;
static int CADmcounter = 0;
static int CADcounter = 0;

static BOOL OctopusSelectTypeEnFlag = FALSE;


static HWTesterItem* item;
static int testMode;
/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/
static void updateBG(void)
{
    //TickType_t tickLocalStart = xTaskGetTickCount();
    keyIgnoreFlag=TRUE;
    //EPDDrawContainByIDPos(FALSE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    //EPDDrawContainByIDPos(TRUE,EPD_PICT_LOADING,550,250);
    EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    //EPDDrawStringMax(FALSE," Menual----------",X_SELECT_ITEM-50,Y_SELECT_ITEM-36,FALSE);
    //EPDDrawStringMax(FALSE," Auto------------",X_SELECT_ITEM-50,Y_SELECT_ITEM+(SECOND_SECTION_INDEX*STRING_HEIGHT)-9,FALSE);
    
    //show BG and item
    /*  MENU  */
    for(int i = 0; ; i++)
    {
        //Dont Show Quit
        if(item[i].charItem == 'q')
        {
            maxSelectItem=i;
            break;
        }
        if(item[i].itemName == NULL)
        {
            maxSelectItem=i;
            break;
        }
        if(i<SECOND_SECTION_INDEX)
        {
            EPDDrawStringMax(FALSE,item[i].itemName,130,104+(i*STRING_HEIGHT),FALSE);
            EPDDrawStringMax(FALSE,"-",X_SELECT_ITEM,Y_SELECT_ITEM-1+(i*STRING_HEIGHT),FALSE);
            //EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM-1+(i*STRING_HEIGHT));
        }
        else
        {
            
            EPDDrawStringMax(FALSE,item[i].itemName,130,104+(i*STRING_HEIGHT),FALSE);
            EPDDrawStringMax(FALSE,"-",X_SELECT_ITEM,Y_SELECT_ITEM-1+(i*STRING_HEIGHT),FALSE);
            
            //EPDDrawStringMax(FALSE,item[i].itemName,130,104+(i*STRING_HEIGHT)+SECOND_SECTION_Y_OFFSET,FALSE);
            //EPDDrawStringMax(FALSE,"-",X_SELECT_ITEM,Y_SELECT_ITEM+(i*STRING_HEIGHT)+SECOND_SECTION_Y_OFFSET,FALSE);
            //EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(i*STRING_HEIGHT)+SECOND_SECTION_Y_OFFSET);
        }
    }
    
    if(testMode==0)
    {/*  TITLE  */
        EPDDrawStringMax(FALSE,"Single Test",X_HEAD_TITLE,Y_HEAD_TITLE,FALSE);
        //Select Item//
        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT));
    }
    else
    {
        EPDDrawStringMax(FALSE,"All Test",X_HEAD_TITLE,Y_HEAD_TITLE,FALSE);
    }
    
    /*  Draw Mid-Line  */
    for(int i=0;i<(EPD_HEIGHT-100);i+=100)
    {
        EPDDrawContainByIDPos(FALSE,EDP_PICT_LINE_V,X_MID_LINE,Y_MID_LINE+i);
    }
    /*  Msg Bar  */
    EPDDrawStringMax(FALSE,"Result:",550,104,FALSE);
    //EPDDrawString(FALSE,"+:Retry -:No\n^:Yes\\Continue",550,656);
    //EPDDrawStringMax(TRUE,"Message:",550,254,TRUE);
    EPDDrawStringMax(TRUE,"Message:",550,304,TRUE);
    
    keyIgnoreFlag=FALSE;
    //terninalPrintf("========SINGLE========[INFO GUI] <GuiOnDraw>  [%d0'ms].\n", xTaskGetTickCount() - tickLocalStart); 
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiSingleTestOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{ 
    if(para == GUI_CLEAN_MESSAGE_ENABLE)
        cleanMsgAfterMove = TRUE;
    else if(para == GUI_KEY_ENABLE)
        keyIgnoreFlag=FALSE;
    else if(para == GUI_KEY_DISABLE)
        keyIgnoreFlag=TRUE;
    else if(para == GUI_CAD_TIMER_ENABLE)
    {
        CADtimerEnFlag = TRUE;
        CADtimeoutFlag = FALSE;
        CADmcounter = 0;
        CADcounter = 0;
        terninalPrintf("%d \r",CAD_TIMEOUT_SECOND - CADcounter);
    }
    else if(para == GUI_CAD_TIMEROUT_FLAG)
    {
        QueryCADtimeoutFunc(CADtimeoutFlag);
    }
    else if(para == GUI_CAD_TIMER_DISABLE)
    {
        CADtimerEnFlag = FALSE;
    }
    else if(para == GUI_OCTOPUS_SELECTTYPE_EN)
        OctopusSelectTypeEnFlag = TRUE;
    else if(para == GUI_OCTOPUS_SELECTTYPE_DE)
        OctopusSelectTypeEnFlag = FALSE;
    else
    {
        if(para2!=0){
            item=(HWTesterItem*) para2;
        }
        nowIndex = 0;
        oldIndex = 0;
        testMode = para3;
        powerStatus = FALSE;
        pGuiGetInterface = GuiGetInterface();
        pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);
        if(testMode==0)
            pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL_SMODE);
        else
            pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL);
        pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);      
        
        PowerDrvSetEnable(TRUE);
        EPDSetSleepFunction(FALSE);
        
        pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更updateScreen
        
        
        updateBG();
    }
    return TRUE;
}
BOOL GuiSingleTestUpdateData(void)
{   
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n");
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更updateScreen
    return TRUE;
}

BOOL GuiSingleTestKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <Stand By> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Stand By> Key:  ignore...\n");
        return reVal;
    }
    //Normal Key Function//
    else if((GUI_KEY_DOWN_INDEX == downUp)&!keyIgnoreFlag)
    {
        switch(keyId)
        {
        /*  <-  */
        case GUI_KEYPAD_ONE:
            /*
            if(OctopusSelectTypeEnFlag)
            {
                SetGuiResponseVal('1');
                reVal = TRUE;
            }
            */
            if(testMode==ALL_TEST_MODE)
                break;
            if((nowIndex>0)&&(nowIndex==oldIndex)&&!GetTesterFlag())
            {
                oldIndex=nowIndex;
                nowIndex--;
            }
            else if((nowIndex==oldIndex)&&(nowIndex==0)&&!GetTesterFlag())
            {
                oldIndex=nowIndex;
                nowIndex=MAX_SELECT_ITEM-1;
            }
            reVal = TRUE;
            break;
        /*  ->  */
        case GUI_KEYPAD_TWO:
            /*
            if(OctopusSelectTypeEnFlag)
            {
                SetGuiResponseVal('2');
                reVal = TRUE;
            }
            */
            if(testMode==ALL_TEST_MODE)
                break;
            if((nowIndex<(MAX_SELECT_ITEM-1))&&(nowIndex==oldIndex)&&!GetTesterFlag())
            {
                oldIndex=nowIndex;
                nowIndex++;
            }
            else if((nowIndex==oldIndex)&&(nowIndex==MAX_SELECT_ITEM-1)&&!GetTesterFlag())
            {
                oldIndex=nowIndex;
                nowIndex=0;
            }
            reVal = TRUE;
            break;
        /*  +   */
        case GUI_KEYPAD_THREE:
            if(OctopusSelectTypeEnFlag)
            {
                SetGuiResponseVal('3');
                reVal = TRUE;
                //OctopusSelectTypeEnFlag = FALSE;
            }
            else
            {
                SetGuiResponseVal('y');
                reVal = TRUE;
            }
            break;
        /*  -   */
        case GUI_KEYPAD_FOUR:
            if(OctopusSelectTypeEnFlag)
            {
                SetGuiResponseVal('4');
                reVal = TRUE;
                //OctopusSelectTypeEnFlag = FALSE;
            }
            else
            {
                SetGuiResponseVal('n');
                reVal = TRUE;
            }
            break;
        /*  v   */    
        case GUI_KEYPAD_FIVE:
            if(testMode!=ALL_TEST_MODE && nowIndex == oldIndex)
            {
                //removeStrAfterTest = TRUE;
                cleanMsgAfterMove = TRUE;
                terninalPrintf("cleanMsgAfterMove:%d\n",cleanMsgAfterMove);
                SetGuiResponseVal(item[nowIndex].charItem);
                reVal = TRUE;
            }
            reVal = TRUE;
            break;
        /*  x   */    
        case GUI_KEYPAD_SIX:
            SetGuiResponseVal('q');
            reVal = TRUE;
            break;
        case GUI_KEYPAD_NORMAL_ID:
            reVal = TRUE;
            break;
        case GUI_KEYPAD_REPLACE_BP_ID:
            reVal = TRUE;
            break;
        case GUI_KEYPAD_TESTER_ID:
            reVal = TRUE;
            break;
         case GUI_KEYPAD_TESTER_KEYPAD_ID:
            reVal = TRUE;
            break;
        default:
            sysprintf(" [INFO GUI] <Stand By> Key:  not support keyId 0x%02x...\n", keyId); 
            break;
        }
    }

    //if(reVal)
    //    EPDReSetBacklightTimeout(5000);
    return reVal;
}
BOOL GuiSingleTestTimerCallback(uint8_t timerIndex)
{
    keyIgnoreFlag = TRUE;
    //sysprintf(" [INFO GUI] <Stand By> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    powerStatus = FALSE;  
    if(powerStatusFlag == FALSE)
    {
        switch(timerIndex)
        {
            case UPDATE_BG_TIMER:
                //updateBG();
                break;
            case UPDATE_DATA_TIMER:
                //updateMsg();
            /*
                if(removeStrAfterTest)
                {
                    EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CLEAN_BAR,550-2,304-2);
                    EPDDrawString(TRUE,"                 \n                \n",550,154);
                    removeStrAfterTest=FALSE;
                }
            */
                if(oldIndex!=nowIndex && testMode!=ALL_TEST_MODE)
                {
                    if(cleanMsgAfterMove)
                    {
                        cleanMsgAfterMove = FALSE;
                        //EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CLEAN_BAR,550-2,304-2);
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CLEAN_BAR,550-2,348-2);
                        //EPDDrawStringMax(TRUE,"                 \n                \n",550,154,TRUE);
                        EPDDrawStringMax(FALSE,"                 \n                \n",550,154,TRUE);
                    }
                    if((nowIndex<SECOND_SECTION_INDEX-1)&&(oldIndex!=maxSelectItem-1))
                    {//在第一區 (now <6)1,2,3,4,5 but not([0] <- last)不包括最後一個跳去第一個
                        //deselect
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT));
                    }
                    else if(oldIndex==(SECOND_SECTION_INDEX-2))
                    {//在交界處 (5->[6])
                        //deselect
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT));
                    }
                    else if(oldIndex==(SECOND_SECTION_INDEX-1) && nowIndex==SECOND_SECTION_INDEX)
                    {//S1->S2 光標往下走(6 ->[7])
                        //deselect
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                    }
                    else if(oldIndex==SECOND_SECTION_INDEX && nowIndex==(SECOND_SECTION_INDEX-1))
                    {//S1->S2 光標往上走([6] <-7)
                        //deselect
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT));
                    }
                    else if((oldIndex==(SECOND_SECTION_INDEX+1) && nowIndex==SECOND_SECTION_INDEX))
                    {//在交界處 ([7] <-8) 
                        //deselect
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                    }
                    else if((nowIndex>SECOND_SECTION_INDEX)&&(oldIndex!=0))
                    {//在第二區 (7 < [now])8,9,10,11.... but not([last]<-0)不包括第一個跳去最後一個
                        //deselect
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                    }
                    else if((nowIndex==0) && (oldIndex==maxSelectItem-1))
                    {//最後一個光標回到第一個(last->first)
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT));
                    }
                    else if((nowIndex==maxSelectItem-1) && (oldIndex==0))
                    {//第一個回到最後一個光標(last<-first)
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(oldIndex*STRING_HEIGHT));
                        //select
                        EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,X_SELECT_ITEM,Y_SELECT_ITEM+(nowIndex*STRING_HEIGHT+SECOND_SECTION_Y_OFFSET));
                    }
                    oldIndex=nowIndex;
                }
                
                if(CADtimerEnFlag)
                {
                    CADmcounter++;
                    if(CADmcounter >= 40)
                    {
                        CADmcounter = 0;
                        CADcounter++;
                        terninalPrintf("%d \r",CAD_TIMEOUT_SECOND - CADcounter);
                    }
                    
                    if(CADcounter >= CAD_TIMEOUT_SECOND)
                    {
                        CADtimerEnFlag = FALSE;
                        CADtimeoutFlag = TRUE;
                    }
                    
                }
                
                
                break;
            case UPDATE_SPACE_DETECT_TIMER:
//                StartSpaceDrv();
                break;
        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;
    return TRUE;
}

BOOL GuiSingleTestPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] Standby power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:      
            if(flag == WAKEUP_SOURCE_RTC)  
            {               
                sysprintf(" [INFO GUI] <Stand By> PowerCallbackFunc UPDATE_DATA_TIMER\n");
                powerStatus = FALSE;                 
                pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);
            }
            else
            {
                sysprintf(" [INFO GUI] <Stand By> PowerCallbackFunc ignore\n");  
            }
            powerStatusFlag = FALSE;
            break;
        case GUI_POWER_OFF_INDEX:
            break;
        case GUI_POWER_PREV_OFF_INDEX:
            powerStatusFlag = TRUE;
            break;
    }
    return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


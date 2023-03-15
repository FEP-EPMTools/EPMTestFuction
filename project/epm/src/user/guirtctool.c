/**************************************************************************//**
* @file     guiRTCTool.c
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
#include "guirtctool.h"
#include "epddrv.h"
#include "guimanager.h"
#include "powerdrv.h"
#include "paralib.h"
#include "meterdata.h"
#include "cardreader.h"
#include "spacedrv.h"
#include "tarifflib.h"
#include "hwtester.h"
#include "../octopus/octopusreader.h"

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UPDATE_BG_TIMER     GUI_TIME_0_INDEX
#define UPDATE_DATA_TIMER   GUI_TIME_1_INDEX
#define UPDATE_SPACE_DETECT_TIMER   GUI_TIME_2_INDEX

#define UPDATE_BG_INTERVAL     portMAX_DELAY
#define UPDATE_DATA_INTERVAL   (500/portTICK_RATE_MS)
#define UPDATE_SPACE_DETECT_INTERVAL     (50/portTICK_RATE_MS)
#define STRING_HEIGH    44
#define BTM_DISCRIPT_BAR_X 80
#define BTM_DISCRIPT_BAR_Y 600-STRING_HEIGH

#define X_HEAD_TITLE    90
#define Y_HEAD_TITLE    28

#define GET_ID_MODE 1
#define SET_ID_MODE 0
/*-----------------------------------------*/
/* global file scope (static) variables */
/*-----------------------------------------*/
static GuiInterface* pGuiGetInterface = NULL;
static BOOL powerStatus = FALSE;
static BOOL powerStatusFlag = FALSE;
static int inputFlag;//0-set ID 1-show ID

static TickType_t tickStart = 0;
static BOOL keyIgnoreFlag = FALSE;
static uint8_t refreshType = GUI_REDRAW_PARA_NONE;

static int nowcursor =4,oldcursor=4;

static int nowYearcursor =3,oldYearcursor=3;
static int nowMonthcursor =1,oldMonthcursor=1;
static int nowDaycursor =1,oldDaycursor=1;
static int nowHourcursor =1,oldHourcursor=1;
static int nowMinutecursor =1,oldMinutecursor=1;

static int  nowTimecursor[] = {3,1,1,1,1,0};
static int  oldTimecursor[] = {3,1,1,1,1,0};

static char deviceID[5];


static int TimeNumSize[] = {4,2,2,2,2,1};
static char NumArray[4];


static char YearNum[4];
static char MonthNum[2];
static char DayNum[2];
static char HourNum[2];
static char MinuteNum[2];
static char WeekdayNum[1];

static char YearStr[10];
static char MonthStr[10];
static char DayStr[10];
static char HourStr[10];
static char MinuteStr[10];
static char WeekdayStr[10];


static char* NumArrayPtr[] = {(char*)YearStr,(char*)MonthStr,(char*)DayStr,(char*)HourStr,(char*)MinuteStr,(char*)WeekdayStr};

static BOOL refreshTimeFlag[] = {FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};

static BOOL refreshYearFlag = FALSE;
static BOOL refreshMonthFlag = FALSE;
static BOOL refreshDayFlag = FALSE;
static BOOL refreshHourFlag = FALSE;
static BOOL refreshMinuteFlag = FALSE;
static BOOL refreshWeekdayFlag = FALSE;


static BOOL refreshDeviceID=FALSE;

static BOOL SetYearFlag = FALSE;
static BOOL SetMonthFlag = FALSE;
static BOOL SetDayFlag = FALSE;
static BOOL SetHourFlag = FALSE;
static BOOL SetMinuteFlag = FALSE;
static BOOL SetWeekdayFlag = FALSE;

static UINT tempYear,tempMonth,tempDay,tempHour,tempMinute,tempSecond;
/*-----------------------------------------*/
/* prototypes of static functions       */
/*-----------------------------------------*/

static void updateBG(void)
{
    EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    EPDDrawString(FALSE,"Set RTC    ",X_HEAD_TITLE,Y_HEAD_TITLE);
    if(inputFlag == GUI_SETRTC_YEAR)  
    {        
        EPDDrawString(FALSE,"Please enter year value in",170,204);
        EPDDrawString(FALSE,"decimal.(ex:2020)",90,254);
        EPDDrawString(FALSE,"Enter number is ",90,354);
        //char YearStr[10];
        sprintf(YearStr,"20%d", tempYear);
        EPDDrawString(FALSE,YearStr,17*32,354);
        EPDDrawString(TRUE,"<\\> Move\n+\\= Change\n{  Set\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if(inputFlag == GUI_SETRTC_MONTH)
    {        
        EPDDrawString(FALSE,"Please enter Month value in",170,204);
        EPDDrawString(FALSE,"decimal.ex:(01-12)",90,254);
        EPDDrawString(FALSE,"Enter number is ",90,354);
        //char MonthStr[10];
        sprintf(MonthStr,"%02d", tempMonth);
        EPDDrawString(FALSE,MonthStr,17*32,354);
        EPDDrawString(TRUE,"<\\> Move\n+\\= Change\n{  Set\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if(inputFlag == GUI_SETRTC_DAY)
    {        
        EPDDrawString(FALSE,"Please enter Day value in",170,204);
        EPDDrawString(FALSE,"decimal.ex:(01-31)",90,254);
        EPDDrawString(FALSE,"Enter number is ",90,354);
        //char DayStr[10];
        sprintf(DayStr,"%02d", tempDay);
        EPDDrawString(FALSE,DayStr,17*32,354);
        EPDDrawString(TRUE,"<\\> Move\n+\\= Change\n{  Set\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if(inputFlag == GUI_SETRTC_HOUR)
    {        
        EPDDrawString(FALSE,"Please enter Hour value in",170,204);
        EPDDrawString(FALSE,"decimal.ex:(0-23)",90,254);
        EPDDrawString(FALSE,"Enter number is ",90,354);
        //char HourStr[10];
        sprintf(HourStr,"%02d", tempHour);
        EPDDrawString(FALSE,HourStr,17*32,354);
        EPDDrawString(TRUE,"<\\> Move\n+\\= Change\n{  Set\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if(inputFlag == GUI_SETRTC_MINUTE)
    {        
        EPDDrawString(FALSE,"Please enter Minute value in",170,204);
        EPDDrawString(FALSE,"decimal.ex:(0-59)",90,254);
        EPDDrawString(FALSE,"Enter number is ",90,354);
        //char MinuteStr[10];
        sprintf(MinuteStr,"%02d", tempMinute);
        EPDDrawString(FALSE,MinuteStr,17*32,354);
        EPDDrawString(TRUE,"<\\> Move\n+\\= Change\n{  Set\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if(inputFlag == GUI_SETRTC_WEEKDAY)
    {        
        EPDDrawString(FALSE,"Please enter Weekday value in",170,204);
        EPDDrawString(FALSE,"decimal.ex:(0:Sunday, 1:Monday,...,\n6:Saturday)",90,254);
        EPDDrawString(FALSE,"Enter number is ",90,404);
        //char WeekdayStr[10];
        sprintf(WeekdayStr,"%d", 1);
        EPDDrawString(FALSE,WeekdayStr,17*32,404);
        EPDDrawString(TRUE,"\n+\\= Change\n{  Save\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
    }
    else if((inputFlag == 0) || (inputFlag == GUI_SETRTC_MODIFY) )
    {
        /*
        TickType_t tickLocalStart = xTaskGetTickCount();
        //sysprintf(" [INFO GUI] <Stand By> updateBG enter: cost ticks = [%d]\n", xTaskGetTickCount() - tickStart); 
        switch(refreshType)
        {
            case GUI_REDRAW_PARA_NORMAL:

                break;
            case GUI_REDRAW_PARA_REFRESH:

                break;
            case GUI_REDRAW_PARA_CONTAIN:
                
                break;
            default:
                sysprintf(" [INFO GUI] <Stand By> updateBG refreshType ERROR :refreshType = %d\n", refreshType); 
                break;
                    
        }
        */
        //refreshType = GUI_REDRAW_PARA_NONE;
        //Draw BG

        
        char RTCstr[30];
        sprintf(RTCstr,"20%d\\%d\\%d  %d:%d:%d", tempYear,tempMonth,tempDay,tempHour,tempMinute,tempSecond);
        if(inputFlag == 0)
            EPDDrawString(FALSE,"Recent time:",170,204);
        else if(inputFlag == GUI_SETRTC_MODIFY)
            EPDDrawString(FALSE,"Modify time:",170,204);
        EPDDrawString(FALSE,RTCstr,200,254);
        EPDDrawString(FALSE,"Is RTC time correct?",170,454);
        //EPDDrawString(FALSE,"Device ID :",170,254);
        //EPDDrawString(FALSE,deviceID,560,254);
        
        //if(inputFlag==0){
            //EPDDrawString(FALSE,"Set RTC    ",X_HEAD_TITLE,Y_HEAD_TITLE);
            //EPDDrawString(TRUE,"<\\> Move\n+\\= Change\n{  Save\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
            EPDDrawString(TRUE,"+\\= Yes\\No\n}  Exit",BTM_DISCRIPT_BAR_X,BTM_DISCRIPT_BAR_Y);
        //}

        
        //sysprintf(" [INFO GUI] <Stand By> updateBG: **Local:[%d]**, **[%d]**\n", xTaskGetTickCount() - tickLocalStart, xTaskGetTickCount() - tickStart);  
    }
}
static void updateData(void)
{
    if( (inputFlag >= GUI_SETRTC_YEAR) && (inputFlag <= GUI_SETRTC_WEEKDAY) )
    {
        int yPosition;
        
        if(inputFlag == GUI_SETRTC_WEEKDAY)
            yPosition = 404;
        else
            yPosition = 354;
        
        //move corsor
        if(nowTimecursor[inputFlag & 0x0F] != oldTimecursor[inputFlag & 0x0F]){
            //deselecet  
            EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+((*(NumArrayPtr[inputFlag & 0x0F] + oldTimecursor[inputFlag & 0x0F]))-'0')
            //EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX
                                    ,17*32+(28*oldTimecursor[inputFlag & 0x0F]),yPosition);
            //select
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+((*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F]))-'0')
            //EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX
                                    ,17*32+(28*nowTimecursor[inputFlag & 0x0F]),yPosition);
            
            //terninalPrintf("nowcursor = %d\r\n",*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F]));
            //terninalPrintf("oldcursor = %d\r\n",*(NumArrayPtr[inputFlag & 0x0F] + oldTimecursor[inputFlag & 0x0F]));
            
            oldTimecursor[inputFlag & 0x0F] = nowTimecursor[inputFlag & 0x0F];
        }
        //add or minus number
        if(refreshTimeFlag[inputFlag & 0x0F]){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+((*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F]))-'0')
                                    ,17*32+(28*nowTimecursor[inputFlag & 0x0F]),yPosition);
            refreshTimeFlag[inputFlag & 0x0F]=FALSE;
        }
        
    }
    /*
    if(inputFlag == GUI_SETRTC_YEAR)        
    {
        //move corsor
        if(nowYearcursor!=oldYearcursor){
            //deselecet
            EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldYearcursor]-'0'),560+(28*oldYearcursor),254);
            //select
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowYearcursor]-'0'),560+(28*nowYearcursor),254);
            oldcursor=nowcursor;
        }
        //add or minus number
        if(refreshDeviceID){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowYearcursor]-'0'),560+(28*nowYearcursor),254);
            refreshDeviceID=FALSE;
        }
    }
    else if(inputFlag == GUI_SETRTC_MONTH)
    {
        //move corsor
        if(nowMonthcursor!=oldMonthcursor){
            //deselecet
            EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldMonthcursor]-'0'),560+(28*oldMonthcursor),254);
            //select
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowMonthcursor]-'0'),560+(28*nowMonthcursor),254);
            oldMonthcursor=nowMonthcursor;
        }
        //add or minus number
        if(refreshDeviceID){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowMonthcursor]-'0'),560+(28*nowMonthcursor),254);
            refreshDeviceID=FALSE;
        }
    }
    else if(inputFlag == GUI_SETRTC_DAY)
    {
        //move corsor
        if(nowDaycursor!=oldDaycursor){
            //deselecet
            EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldDaycursor]-'0'),560+(28*oldDaycursor),254);
            //select
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowDaycursor]-'0'),560+(28*nowDaycursor),254);
            oldcursor=nowDaycursor;
        }
        //add or minus number
        if(refreshDeviceID){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowDaycursor]-'0'),560+(28*nowDaycursor),254);
            refreshDeviceID=FALSE;
        }
    }
    else if(inputFlag == GUI_SETRTC_HOUR)
    {
        //move corsor
        if(nowHourcursor!=oldHourcursor){
            //deselecet
            EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldHourcursor]-'0'),560+(28*oldHourcursor),254);
            //select
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowHourcursor]-'0'),560+(28*nowHourcursor),254);
            oldHourcursor=nowHourcursor;
        }
        //add or minus number
        if(refreshDeviceID){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowHourcursor]-'0'),560+(28*nowHourcursor),254);
            refreshDeviceID=FALSE;
        }
    }
    else if(inputFlag == GUI_SETRTC_MINUTE)
    {
        //move corsor
        if(nowMinutecursor!=oldMinutecursor){
            //deselecet
            EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldMinutecursor]-'0'),560+(28*oldMinutecursor),254);
            //select
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowMinutecursor]-'0'),560+(28*nowMinutecursor),254);
            oldMinutecursor=nowMinutecursor;
        }
        //add or minus number
        if(refreshDeviceID){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowMinutecursor]-'0'),560+(28*nowMinutecursor),254);
            refreshDeviceID=FALSE;
        }
    }
    else if(inputFlag == GUI_SETRTC_WEEKDAY)
    {
        //move corsor

        //add or minus number
        if(refreshDeviceID){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),560+(28*nowcursor),254);
            refreshDeviceID=FALSE;
        }
    }
    
    */
    /*
    if(inputFlag==0){
        TickType_t tickLocalStart = xTaskGetTickCount();
        //move corsor
        if(nowcursor!=oldcursor){
            //deselecet
            EPDDrawContainByIDPos(FALSE,EPD_PICT_NUM_SMALL_2_INDEX+(deviceID[oldcursor]-'0'),560+(28*oldcursor),254);
            //select
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),560+(28*nowcursor),254);
            oldcursor=nowcursor;
        }
        //add or minus number
        if(refreshDeviceID){
            EPDDrawContainByIDPos(TRUE,EPD_PICT_NUM_SMALL_2_I_INDEX+(deviceID[nowcursor]-'0'),560+(28*nowcursor),254);
            refreshDeviceID=FALSE;
        }
    }
    */
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL GuiRTCToolOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3)
{   
    //inputFlag=para3;
    inputFlag = para;
    if(inputFlag == GUI_REDRAW_PARA_REFRESH)
        inputFlag = 0;
    
    if(inputFlag == GUI_SETRTC_YEAR)        
        SetYearFlag = TRUE;
    else if(inputFlag == GUI_SETRTC_MONTH)
        SetMonthFlag = TRUE;
    else if(inputFlag == GUI_SETRTC_DAY)
        SetDayFlag = TRUE;
    else if(inputFlag == GUI_SETRTC_HOUR)
        SetHourFlag = TRUE;
    else if(inputFlag == GUI_SETRTC_MINUTE)
        SetMinuteFlag = TRUE;
    else if(inputFlag == GUI_SETRTC_WEEKDAY)
        SetWeekdayFlag = TRUE;
    else if(inputFlag == GUI_SETRTC_MODIFY)
    {
        terninalPrintf("\r\nModify time: ");
        OctopusReaderQueryTimeEx(&tempYear,&tempMonth,&tempDay,&tempHour,&tempMinute,&tempSecond);
        //OctopusReaderQueryTime();
    }
    else if(inputFlag == 0)
    {
        //tickStart = xTaskGetTickCount();
        sysprintf(" [INFO GUI] <Stand By> OnDraw (from GuiId = %d, para = %d)\n", oriGuiId, para); 
        powerStatus = FALSE;
        pGuiGetInterface = GuiGetInterface();
        pGuiGetInterface->setTimeoutFunc(UPDATE_BG_TIMER, UPDATE_BG_INTERVAL);
        pGuiGetInterface->setTimeoutFunc(UPDATE_DATA_TIMER, UPDATE_DATA_INTERVAL); 
        pGuiGetInterface->setTimeoutFunc(UPDATE_SPACE_DETECT_TIMER, UPDATE_SPACE_DETECT_INTERVAL);      
        PowerDrvSetEnable(TRUE);
        EPDSetSleepFunction(FALSE);

        pGuiGetInterface->runTimeoutFunc(UPDATE_BG_TIMER);//更新畫面
        
        //CardReaderSetPower(EPM_READER_CTRL_ID_GUI, FALSE);
        //EPDReSetBacklightTimeout(5000/portTICK_RATE_MS);
        
        //sprintf(deviceID,"%05d",GetDeviceIDString());
        
        /*
        UINT* tempYear;
        UINT* tempMonth;
        UINT* tempDay;
        UINT* tempHour;
        UINT* tempMinute;
        UINT* tempSecond;
        */

        terninalPrintf("Recent time: ");
        OctopusReaderQueryTimeEx(&tempYear,&tempMonth,&tempDay,&tempHour,&tempMinute,&tempSecond);
        //OctopusReaderQueryTimeEx(tempYear,tempMonth,tempDay,tempHour,tempMinute,tempSecond);
        //OctopusReaderQueryTime();
        //updateBG();
        //sprintf(deviceID,"%05d",GetDeviceIDString());
        
       
    }   
    updateBG();
    return TRUE;
}
BOOL GuiRTCToolUpdateData(void)
{    
    //tickStart = xTaskGetTickCount();
    sysprintf(" [INFO GUI] <Stand By> UpdateData\n"); 
    pGuiGetInterface->runTimeoutFunc(UPDATE_DATA_TIMER);//更新畫面
    return TRUE;
}
BOOL GuiRTCToolKeyCallback(uint8_t keyId, uint8_t downUp)
{   
    BOOL reVal = FALSE; 
    //sysprintf(" [INFO GUI] <Stand By> Key:  keyId = %d, downUp = %d\n", keyId, downUp);
    //if(keyIgnoreFlag)
    if(keyIgnoreFlag && (keyId < GUI_KEYPAD_NORMAL_ID))
    {
        sysprintf(" [INFO GUI] <Stand By> Key:  ignore...\n"); 
        return reVal;
    }
    if(GUI_KEY_DOWN_INDEX == downUp)
    {
        switch(keyId)
        {
            case GUI_KEYPAD_ONE:
                
               if((nowTimecursor[inputFlag & 0x0F] >0) && (inputFlag >= GUI_SETRTC_YEAR) && (inputFlag <= GUI_SETRTC_WEEKDAY)){
                    nowTimecursor[inputFlag & 0x0F]--;
                    reVal = TRUE;
                }
                /*
                if((nowcursor>0)){
                    nowcursor--;
                    reVal = TRUE;
                }
                */
                break;
                
            case GUI_KEYPAD_TWO:
                if((nowTimecursor[inputFlag & 0x0F] < (TimeNumSize[inputFlag & 0x0F]-1)) && (inputFlag >= GUI_SETRTC_YEAR) && (inputFlag <= GUI_SETRTC_WEEKDAY) ){
                    nowTimecursor[inputFlag & 0x0F]++;
                    reVal = TRUE;
                }
                
            
                /*
                if((nowcursor<4)){
                    nowcursor++;
                    reVal = TRUE;
                }
                */
                break;
                
            case GUI_KEYPAD_THREE:
                if((inputFlag >= GUI_SETRTC_YEAR) && (inputFlag <= GUI_SETRTC_WEEKDAY))
                {
                    if( ((*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F])) <'9') )
                        //NumArray[nowTimecursor[inputFlag & 0x0F]]++;                
                        (*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F])) ++;
                    else 
                        //NumArray[nowTimecursor[inputFlag & 0x0F]]='0';
                        (*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F])) ='0';
                    refreshTimeFlag[inputFlag & 0x0F] = TRUE;
                    reVal = TRUE;
                }
                //else if(inputFlag == 0)
                else if((inputFlag == 0) || (inputFlag == GUI_SETRTC_MODIFY))
                {
                    SetGuiResponseVal('y');
                    reVal = TRUE;
                }
                /*
                if(deviceID[nowcursor]<'9')
                    deviceID[nowcursor]++;
                else 
                    deviceID[nowcursor]='0';
                refreshDeviceID=TRUE;
                reVal = TRUE;
                */
                break;
                
            case GUI_KEYPAD_FOUR:
                if((inputFlag >= GUI_SETRTC_YEAR) && (inputFlag <= GUI_SETRTC_WEEKDAY))
                {
                    if( ((*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F]))>'0') )
                    {
                        //NumArray[nowTimecursor[inputFlag & 0x0F]]--;
                        (*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F]))--;
                    }
                    else 
                    {
                        (*(NumArrayPtr[inputFlag & 0x0F] + nowTimecursor[inputFlag & 0x0F])) = '9';
                    }
                    refreshTimeFlag[inputFlag & 0x0F] = TRUE;
                    reVal = TRUE;
                }
                //else if(inputFlag == 0)
                else if((inputFlag == 0) || (inputFlag == GUI_SETRTC_MODIFY))
                {
                    SetGuiResponseVal('n');
                    reVal = TRUE;
                }
            
                /*
                if(deviceID[nowcursor]>'0')
                {
                    deviceID[nowcursor]--;
                }
                else 
                {
                    deviceID[nowcursor]='9';
                }
                refreshDeviceID=TRUE;
//                SetDeviceIDString(deviceID,NULL);
                reVal = TRUE;
                
                */
                break;
            case GUI_KEYPAD_FIVE:
                if( (inputFlag >= GUI_SETRTC_YEAR) && (inputFlag <= GUI_SETRTC_WEEKDAY) )
                {
                    
                    HwTestReceieveU32Func(atoi(NumArrayPtr[inputFlag & 0x0F]));
                    //terninalPrintf("%d",atoi(NumArrayPtr[inputFlag & 0x0F]));
                    SetGuiResponseVal(0x0D);
                    reVal = TRUE;
                    
                }
                /*
                if(inputFlag==0)
                {
                    SetDeviceIDString(deviceID,NULL);
                    SetGuiResponseVal('y');
                }
                reVal = TRUE;
                */
                break;
            case GUI_KEYPAD_SIX:
                SetGuiResponseVal('q');
                reVal = TRUE;
                break;
            default:
                sysprintf(" [INFO GUI] <Stand By> Key:  not support keyId 0x%02x...\n", keyId); 
                break;
        }
    }
    else
    {
        
    }    
    //if(reVal)
    //    EPDReSetBacklightTimeout(5000);
//    terninalPrintf("[ID]:%s\ncursor:%d",deviceID,nowcursor);
    return reVal;
}
BOOL GuiRTCToolTimerCallback(uint8_t timerIndex)
{
    //sysprintf(" [INFO GUI] <Stand By> Timer [%d] : tick = %d!!\n", timerIndex, xTaskGetTickCount());
    powerStatus = FALSE;  
    keyIgnoreFlag = TRUE;
    if(powerStatusFlag == FALSE)
    {
        switch(timerIndex)
        {
            case UPDATE_BG_TIMER:
                //updateBG();
                break;
            case UPDATE_DATA_TIMER:
//                updateData();
                break;
            case UPDATE_SPACE_DETECT_TIMER:
                updateData();
//                StartSpaceDrv();
                break;

        }
    }
    keyIgnoreFlag = FALSE;
    powerStatus = TRUE;  
    return TRUE;
}

BOOL GuiRTCToolPowerCallbackFunc(uint8_t type, int flag)
{
    //sysprintf(" [INFO GUI] Standby power [%d] : flag = %d!!\n", type, flag);
    switch(type)
    {
        case GUI_POWER_STATUS_INDEX:
            return powerStatus;
        case GUI_POWER_ON_INDEX:      
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


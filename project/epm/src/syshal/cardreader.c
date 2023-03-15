/**************************************************************************//**
* @file     cardreader.c
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
#include "cardreader.h"
#include "halinterface.h"
#include "powerdrv.h"
#include "buzzerdrv.h"
//#include "ipassdfti.h"
#include "ipassdpti.h"
#include "loglib.h"
#include "../octopus/octopusreader.h"
#include "dipdrv.h"

#if (ENABLE_BURNIN_TESTER)
#include "timelib.h"
#include "gpio.h"
#include "burnintester.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//#define RX_BUFF_LEN   64 
#define CARD_READER_TSREADER_INDEX    TSREADER_EPM_READER_INTERFACE_INDEX
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static TSReaderInterface* pTSReaderInterface;

static SemaphoreHandle_t xSemaphore, xCommandSemaphore;
static TickType_t threadWaitTime = portMAX_DELAY;

static uint8_t cardReaderBootedStatus = FALSE;
static BOOL initFlag = FALSE;

static BOOL ReaderInterconnectFlag = FALSE;
static BOOL InterconnectResultFlag = FALSE;

static BOOL octopusModeFlag = FALSE;
static BOOL CheckReaderHangUpEn = FALSE;

/*
static BOOL CardReaderCheckStatus(int flag);
static BOOL CardReaderPreOffCallback(int flag);
static BOOL CardReaderOffCallback(int flag);
static BOOL CardReaderOnCallback(int flag, int wakeupSource);
static powerCallbackFunc cardDrvPowerCallabck = {" [CardReader] ", CardReaderPreOffCallback, CardReaderOffCallback, CardReaderOnCallback, CardReaderCheckStatus};
*/

#if (ENABLE_BURNIN_TESTER)
static uint32_t cardReaderBurninCounter = 0;
static uint32_t cardReaderBurninErrorCounter = 0;
#endif

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static void vCardReaderRxTask( void *pvParameters )
{
    //vTaskDelay(2000/portTICK_RATE_MS); 
    sysprintf("vCardReaderRxTask Going...\r\n");  
    //pTSReaderInterface->setPowerFunc(FALSE);       

    //vTaskDelete(NULL); 
    for(;;)
    {     
        #if(ENABLE_THREAD_RUNNING_DEBUG)
        sysprintf("\r\n ! (-WARNING-) %s Waiting (%d)... ) !\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()), threadWaitTime); 
        #endif
        BaseType_t reval = xSemaphoreTake(xSemaphore, threadWaitTime); 
        #if(ENABLE_THREAD_RUNNING_DEBUG)
        sysprintf("\r\n ! (-WARNING-) %s Go (%d)... ) !\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()), threadWaitTime); 
        #endif
        if(reval != pdTRUE)
        {//timeout
            threadWaitTime = portMAX_DELAY;
            sysprintf("\r\n ! (-vCardReaderRxTask TIMEOUT-) %s ... ) !\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
            pTSReaderInterface->setPowerFunc(EPM_READER_CTRL_ID_GUI, FALSE); 
        }
        else
        {
            if(TSREADER_CHECK_READER_INIT == cardReaderBootedStatus)
            {
                cardReaderBootedStatus = pTSReaderInterface->checkReaderFunc();
                if((cardReaderBootedStatus == TSREADER_CHECK_READER_BREAK) || (cardReaderBootedStatus == TSREADER_CHECK_READER_ERROR))
                {
                    sysprintf("\r\n ! (-vCardReaderRxTask TSREADER_CHECK_READER_BREAK or TSREADER_CHECK_READER_ERROR-) %s ... ) !\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
                    pTSReaderInterface->setPowerFunc(EPM_READER_CTRL_ID_GUI, FALSE); 
                }
            }
            vTaskDelay(2000/portTICK_RATE_MS); 
        }  
    }
}

#if (ENABLE_BURNIN_TESTER)
static void vCardReaderTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    int waitCounter;
    terninalPrintf("vCardReaderTestTask Going...\r\n");
    
    while (TRUE)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vCardReaderTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_CARD_READER_INTERVAL)
        {
            //terninalPrintf("vCardReaderTestTask heartbeat.\r\n");
            //lastTime = currentTime;
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        testLoop = FALSE;
        if (GPIO_ReadBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN)) {
            waitCounter = 22;
        }
        else {
            waitCounter = 10;
        }
        
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
        while (CardReaderGetBootedStatus() != TSREADER_CHECK_READER_OK)
        {
            vTaskDelay(1000 / portTICK_RATE_MS);
            waitCounter--;
            if (waitCounter == 0) {
                break;
            }
        }
        if (waitCounter == 0) {
            cardReaderBurninErrorCounter++;
        }
        cardReaderBurninCounter++;
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI, FALSE);
        lastTime = GetCurrentUTCTime();
    }
}
#endif



static BOOL swInit(void)
{   
    //PowerRegCallback(&cardDrvPowerCallabck);   
    xCommandSemaphore = xSemaphoreCreateMutex();//xSemaphoreCreateRecursiveMutex();    
    xSemaphore = xSemaphoreCreateBinary();    
    xTaskCreate( vCardReaderRxTask, "vCardReaderRxTask", 1024, NULL,CARD_READER_THREAD_PROI, NULL ); //CARD_READER_THREAD_PROI
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL CardReaderInit(BOOL testModeFlag)
{
    //if(initFlag)
    //    return TRUE;
    //sysprintf("CardReaderInit!!\n");
    
    //sysprintf("CardReaderInit: sizeof(IPassDFTIBody) = %d : TOTAL_IPASS_DFTI_BODY_SIZE = %d \r\n", sizeof(IPassDFTIBody), TOTAL_IPASS_DFTI_BODY_SIZE);
    sysprintf("CardReaderInit: sizeof(IPassDPTIBody) = %d : TOTAL_IPASS_DPTI_BODY_SIZE = %d \r\n", sizeof(IPassDPTIBody), TOTAL_IPASS_DPTI_BODY_SIZE);
    
#if (ENABLE_BURNIN_TESTER)
    //GPIO J4 (High=TSReader, Low=OctopusReader)
    if (GPIO_ReadBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN) || (!GPIO_ReadBit(GPIOJ, BIT1)) )
#else
    //if(testModeFlag || (!GPIO_ReadBit(GPIOJ, BIT1)) )
    if(testModeFlag)
#endif
    {
        //terninalPrintf("TSReaderGetInterface\r\n");
        pTSReaderInterface = TSReaderGetInterface(CARD_READER_TSREADER_INDEX);
    }
    else
    {
        //terninalPrintf("OctopusReaderGetInterface\r\n");
        pTSReaderInterface = OctopusReaderGetInterface();
        octopusModeFlag = TRUE;
    }
    
    if(pTSReaderInterface == NULL)
    {
        sysprintf("CardReaderInit ERROR (pTSReaderInterface == NULL)!!\n");
        return FALSE;
    }
    if(pTSReaderInterface->initFunc() == FALSE)
    {
        sysprintf("CardReaderInit ERROR (pTSReaderInterface false)!!\n");
        return FALSE;
    }
    //pTSReaderInterface->setPowerFunc(EPM_READER_CTRL_ID_ALL, FALSE);  
    if(ReaderInterconnectFlag)
    {
        //terninalPrintf("ReaderInterconnectFlag = FALSE\r\n");
        ReaderInterconnectFlag = FALSE;
        //pTSReaderInterface->getBootedStatusFunc();
        if(EPMReaderGetBootedStatusEx())
        {
            //terninalPrintf("InterconnectResultFlag = TRUE\r\n");
            InterconnectResultFlag = TRUE;
        }
        else
            InterconnectResultFlag = FALSE;
    }
    else
    {
        //terninalPrintf("ReaderInterconnectFlag = TRUE\r\n");
        if(swInit() == FALSE)
        {
            sysprintf("CardReaderInit ERROR (swInit false)!!\n");
            return FALSE;
        }
    }
    sysprintf("CardReaderInit OK!!\n");
    initFlag = TRUE;
    
    //CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
    
#if (ENABLE_BURNIN_TESTER)
if (EnabledBurninTestMode())
{
    xTaskCreate(vCardReaderTestTask, "vCardReaderTestTask", 1024*5, NULL, CARD_READER_TEST_THREAD_PROI, NULL);
}
#endif

    return TRUE;
}
/*
static void CardReaderStartInit(void)
{
    if(xSemaphore == NULL)
    {
        return;
    }
    sysprintf(" --> CardReaderStartInit --\n");
    cardReaderBootedStatus = TSREADER_CHECK_READER_ERROR;
    xSemaphoreGive(xSemaphore);  
}
static void CardReaderStopInit(void)
{
    if(pTSReaderInterface == NULL)
    {
        return;
    }
    sysprintf(" --> CardReaderStopInit --\n");
    pTSReaderInterface->breakCheckReaderFunc();
}
*/

BOOL CardReaderCheckHangUpEnable(void)
{
    return CheckReaderHangUpEn;
}


void InitCardReaderHangUpStatus(void)
{
    if(octopusModeFlag)
    {
        InitOctopusReaderHangUpStatus();
        CheckReaderHangUpEn = TRUE;
    }
}



int CardReaderGetBootedStatus(void)
{
    //return pTSReaderInterface->getBootedStatusFunc();
    if(octopusModeFlag && OctopusReaderHangUpStatus() && CheckReaderHangUpEn)
    {
        return TSREADER_CHECK_READER_ERROR;
    }
       
    return cardReaderBootedStatus;
}
void CardReaderSetPower(uint8_t id, BOOL flag)
{
    if(pTSReaderInterface == NULL)
    {
        return;
    }
    //sysprintf("CardReaderSetPower [%d:%d]!!\n", id, flag);
    
    {
        char str[512];
        sprintf(str, "   CardReaderSetPower [%d:%d]!!\r\n", id, flag);
        LoglibPrintf(LOG_TYPE_INFO, str);
    }
 
    if(flag)
    {
        cardReaderBootedStatus = TSREADER_CHECK_READER_INIT;        
        threadWaitTime = portMAX_DELAY;
        
        pTSReaderInterface->setPowerFunc(id, flag); 
        xSemaphoreGive(xSemaphore);
    }
    else
    {
//        threadWaitTime = (30000/portTICK_RATE_MS);         
//        xSemaphoreGive(xSemaphore);
        //terninalPrintf("Close Reader Power\n");
        pTSReaderInterface->setPowerFunc(id, flag); 
        if(octopusModeFlag)
        {
            CheckReaderHangUpEn = FALSE;
        }
    }
    
}

BOOL CardReaderProcess(uint16_t targetDeduct, tsreaderDepositResultCallback callback)
{
    if(pTSReaderInterface == NULL)
    {
        return FALSE;
    }
    return pTSReaderInterface->processFunc(targetDeduct, callback);
}

BOOL CardReaderProcessCN(tsreaderCNResultCallback callback)
{
    if(pTSReaderInterface == NULL)
    {
        return FALSE;
    }
    return pTSReaderInterface->processCNFunc(callback);
}

BOOL CardReaderSignOnProcess(void)
{
    if(pTSReaderInterface == NULL)
    {
        return FALSE;
    }
    return pTSReaderInterface->signOnProcessFunc();
}

void CardReaderSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue)
{
    if(pTSReaderInterface == NULL)
    {
        return;
    }
    sysprintf("CardReaderSaveFile [%d]!!\n", paraValue);    
    pTSReaderInterface->saveFileFunc(pt, paraValue); 
}
void CardReaderEnterCS(void)
{
    if(xCommandSemaphore != NULL)
    {
        //sysprintf("\r\n  -ReaderCS-> WAIT TAKE !!\n"); 
        //xSemaphoreTakeRecursive(xCommandSemaphore, portMAX_DELAY); 
        xSemaphoreTake(xCommandSemaphore, portMAX_DELAY); 
        //sysprintf("\r\n  -ReaderCS-> TAKE  OK !!\n");
    }
}
void CardReaderExitCS(void)
{
    if(xCommandSemaphore != NULL)
    {
        //sysprintf("\r\n  -ReaderCS-> WAIT GIVE !!\n");
        //xSemaphoreGiveRecursive(xCommandSemaphore); 
        xSemaphoreGive(xCommandSemaphore); 
        //sysprintf("\r\n  -ReaderCS-> GIVE OK!!\n");
    }
}


void SetReaderInterconnectFlag(BOOL Flag)
{    
    ReaderInterconnectFlag = Flag;
}

BOOL ReaderInterconnectResult(void)
{
    BOOL flag;
    flag = InterconnectResultFlag;
    InterconnectResultFlag = FALSE;    
    return flag;
    
}

#if (ENABLE_BURNIN_TESTER)
uint32_t GetCardReaderBurninTestCounter(void)
{
    return cardReaderBurninCounter;
}

uint32_t GetCardReaderBurninTestErrorCounter(void)
{
    return cardReaderBurninErrorCounter;
}
#endif

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


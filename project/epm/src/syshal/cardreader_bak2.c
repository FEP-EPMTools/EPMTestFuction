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

/*
static BOOL CardReaderCheckStatus(int flag);
static BOOL CardReaderPreOffCallback(int flag);
static BOOL CardReaderOffCallback(int flag);
static BOOL CardReaderOnCallback(int flag, int wakeupSource);
static powerCallbackFunc cardDrvPowerCallabck = {" [CardReader] ", CardReaderPreOffCallback, CardReaderOffCallback, CardReaderOnCallback, CardReaderCheckStatus};
*/
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
                //terninalPrintf("cardReaderBootedStatus = %d \r\n", cardReaderBootedStatus );
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
    if(initFlag)
        return TRUE;
    //sysprintf("CardReaderInit!!\n");
    
    //sysprintf("CardReaderInit: sizeof(IPassDFTIBody) = %d : TOTAL_IPASS_DFTI_BODY_SIZE = %d \r\n", sizeof(IPassDFTIBody), TOTAL_IPASS_DFTI_BODY_SIZE);
    sysprintf("CardReaderInit: sizeof(IPassDPTIBody) = %d : TOTAL_IPASS_DPTI_BODY_SIZE = %d \r\n", sizeof(IPassDPTIBody), TOTAL_IPASS_DPTI_BODY_SIZE);
    
    pTSReaderInterface = TSReaderGetInterface(CARD_READER_TSREADER_INDEX);
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
    if(swInit() == FALSE)
    {
        sysprintf("CardReaderInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    sysprintf("CardReaderInit OK!!\n");
    initFlag = TRUE;
    
    //CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
    
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
int CardReaderGetBootedStatus(void)
{
    //return pTSReaderInterface->getBootedStatusFunc();
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
        //terninalPrintf("cardReaderBootedStatus = TSREADER_CHECK_READER_INIT\n");
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
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


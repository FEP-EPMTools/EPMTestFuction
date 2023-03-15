/**************************************************************************//**
* @file     lidardrv.c
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

#include "osmisc.h"

#include "fepconfig.h"
#include "interface.h"

#include "lidardrv.h"

#include "buzzerdrv.h"
#include "loglib.h"
#include "dataagent.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define LIDAR_1_UART   UART_7_INTERFACE_INDEX
#define LIDAR_2_UART   UART_3_INTERFACE_INDEX

#define LIDAR_NUM       2

#define LIDAR_BUFFER_SIZE  (1024) 
#define LIDAR_TIMEOUT_TIME  (5000/portTICK_RATE_MS)//(2000/portTICK_RATE_MS)

#define CHANGE_COUNTER_CONFIRM_MAX_MUN      15

#define CHECK_VALUE_EXCEED_DISTANCE         400
#define CHECK_VALUE_MAX_DISTANCE            300
#define CHECK_VALUE_MIN_DISTANCE            1//30
#define CHECK_VALUE_MIN_POWER               32
#define CHECK_STABLE_DISTANCE_RANGE         5
#define CHANGE_COUNTER_CONFIRM_DEFAULT_MUN  1//5
#define CHECK_VACUUM_COUNTER                1//3

//-----
#define LIDAR_SAMPLE_TIMES      1//5
#define ENABLE_MAX_MIN_FILTER   0//1

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//static SemaphoreHandle_t xActionSemaphore[LIDAR_NUM] ;
//static TickType_t threadWaitTime[LIDAR_NUM]  = {portMAX_DELAY};
//static SemaphoreHandle_t xRunningSemaphore = NULL;

static uint8_t lidarBuffer[LIDAR_NUM][LIDAR_BUFFER_SIZE];
static uint32_t lidarBufferIndex[LIDAR_NUM];

static lidarPacket targetPacket[LIDAR_NUM];
static uint32_t targetPacketIndex[LIDAR_NUM];

static int checkStableDistData[LIDAR_NUM][CHANGE_COUNTER_CONFIRM_MAX_MUN];
static int checkStableDistIndex[LIDAR_NUM];

static int checkStablePowerData[LIDAR_NUM][CHANGE_COUNTER_CONFIRM_MAX_MUN];
//static int checkStablePowerIndex[LIDAR_NUM];

static UartInterface* pUartInterface[LIDAR_NUM] = {NULL, NULL};
static int uartInterface[LIDAR_NUM] = {LIDAR_1_UART, LIDAR_2_UART};
static int prevFeature[LIDAR_NUM] = {LIDAR_FEATURE_INIT, LIDAR_FEATURE_INIT};


static int counterNumber[LIDAR_NUM] = {0, 0};
static char logStr[1024];
static char checkStr[1024];

static int checkExceedDist[LIDAR_NUM] = {CHECK_VALUE_EXCEED_DISTANCE, CHECK_VALUE_EXCEED_DISTANCE};
static int checkMaxDist[LIDAR_NUM] = {CHECK_VALUE_MAX_DISTANCE, CHECK_VALUE_MAX_DISTANCE};
static int checkMinDist[LIDAR_NUM] = {CHECK_VALUE_MIN_DISTANCE, CHECK_VALUE_MIN_DISTANCE};
static int checkMinPower[LIDAR_NUM] = {CHECK_VALUE_MIN_POWER, CHECK_VALUE_MIN_POWER};
static int checkStableDistRange[LIDAR_NUM] = {CHECK_STABLE_DISTANCE_RANGE, CHECK_STABLE_DISTANCE_RANGE};
static int checkStableCounter[LIDAR_NUM] = {CHANGE_COUNTER_CONFIRM_DEFAULT_MUN, CHANGE_COUNTER_CONFIRM_DEFAULT_MUN};
static int checkVacuumCounter[LIDAR_NUM] = {CHECK_VACUUM_COUNTER, CHECK_VACUUM_COUNTER};

static int lastDist[LIDAR_NUM] = {0, 0};
static int lastPower[LIDAR_NUM] = {0, 0};
static int timeoutCounter[LIDAR_NUM] = {0, 0};
static int timeoutCounter2[LIDAR_NUM] = {0, 0};
static int zeroCounter[LIDAR_NUM] = {0, 0};
static int vacuumCounter[LIDAR_NUM] = {0, 0};

//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x01, 0x00, 0x64};//1hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x02, 0x00, 0x65};//2hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x03, 0x00, 0x66};//3hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x0a, 0x00, 0x6d};//10hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x14, 0x00, 0x77};//20hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x64, 0x00, 0xC7};//100hz


//static uint8_t outputOffCmd[5] = {0x5A, 0x05, 0x07, 0x00, 0x66};//5A 05 07 00 66
//static uint8_t outputOnCmd[5] = {0x5A, 0x05, 0x07, 0x01, 0x67};//5A 05 07 01 67

//static uint8_t powerCmd[5] = {0x5A, 0x05, 0x22, 0x00, 0x81};//5A 05 22 00 81  //0
//static uint8_t powerCmd[5] = {0x5A, 0x05, 0x22, 0x05, 0x86};//5A 05 22 0x05 86  //5*10=50
static uint8_t powerCmd[5] = {0x5A, 0x05, 0x22, 0x01, 0x82};//5A 05 22 0x01 82  //1*10=10

static uint8_t powerSaveCmd[4] = {0x5A, 0x04, 0x11, 0x6F};//5A 04 11 6F


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


static void resetCheckStableStatus(int lidarIndex, int* dataBuffer, int* bufferIndex)
{
    //sysprintf("\r\n = [WARNING RESET [%d]] ==   reset CheckStableStatus ====\r\n\r\n", lidarIndex);
    for(int i = 0; i<checkStableCounter[lidarIndex]; i++)
    {
        dataBuffer[i] = 0;
    }
    *bufferIndex = 0;
}

static BOOL checkStableValue(int lidarIndex, int compareDist, int* dataBuffer, int* bufferIndex, int valueRange)
{
    BOOL reVal = TRUE;
    sysprintf("\r\n");
    for(int i = 0; i<checkStableCounter[lidarIndex]; i++)
    {
        int currentDist = abs(dataBuffer[i] - compareDist);
        if(dataBuffer[i] !=0)
        {
            if(currentDist > valueRange)
            {
                sysprintf(" = [WARNING CHECK [%d]] ==   check StableValue[%d]:[%d-%d => %d vs %d] FALSE ====\r\n", lidarIndex, i, compareDist, dataBuffer[i], currentDist, valueRange);
                reVal = FALSE;
                break;
            }
            else
            {
                sysprintf(" = [WARNING CHECK [%d]] ==   check StableValue[%d]:[%d-%d => %d vs %d] TRUE ====\r\n", lidarIndex, i, compareDist, dataBuffer[i], currentDist, valueRange);
            }
        }
        else
        {
            sysprintf(" = [WARNING CHECK [%d]] ==   check StableValue[%d]:[%d-%d => %d vs %d] FALSE (IGNORE) ====\r\n", lidarIndex, i, compareDist, dataBuffer[i], currentDist, valueRange);
            reVal = FALSE;
            break;
        }
    }
    dataBuffer[*bufferIndex] = compareDist;  
    sysprintf(" = [WARNING SET [%d]] check StableValue:add %d in index %d ====\r\n\r\n", lidarIndex, compareDist, *bufferIndex);    
    *bufferIndex = (*bufferIndex+1)%checkStableCounter[lidarIndex];
    if(reVal)
    {
        sprintf(checkStr, "-- {INFORMATION _2}   ~~~~~ < %d cm, %d cm, %d cm, %d cm, %d cm> < %d, %d, %d, %d, %d>\r\n", 
                                                                    checkStableDistData[lidarIndex][0], checkStableDistData[lidarIndex][1], checkStableDistData[lidarIndex][2], checkStableDistData[lidarIndex][3], checkStableDistData[lidarIndex][4],
                                                                    checkStablePowerData[lidarIndex][0], checkStablePowerData[lidarIndex][1], checkStablePowerData[lidarIndex][2], checkStablePowerData[lidarIndex][3], checkStablePowerData[lidarIndex][4]); 
        //LoglibPrintfEx(LOG_TYPE_WARNING, checkStr, FALSE);
    }
    return reVal;
}
static void lidarFlushRxBuffer(int lidarIndex)
{
    //sysprintf("lidarFlushRxBuffer [%d]!!\n", lidarIndex);
    if (pUartInterface[lidarIndex]->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
static void lidarSetPower(int lidarIndex, BOOL flag)
{
    //sysprintf("~~ lidarSetPower [%d]:%d enter!!\n", lidarIndex, flag);    
    if(flag)
    {
        pUartInterface[lidarIndex]->setPowerFunc(TRUE);
        pUartInterface[lidarIndex]->setRS232PowerFunc(TRUE);
    }
    else
    {    
        pUartInterface[lidarIndex]->setRS232PowerFunc(FALSE);
        pUartInterface[lidarIndex]->setPowerFunc(FALSE);
    }
    //sysprintf("~~ lidarSetPower [%d]:%d exit!!\n", lidarIndex, flag);
}

static INT32 lidarWrite(int lidarIndex, PUINT8 pucBuf, UINT32 uLen)
{
    //sysprintf("--> lidarWrite len=%d:[%s] \r\n", uLen, pucBuf); 
    if(pUartInterface[lidarIndex] == NULL)
    {
        //sysprintf("lidarWrite %d...\r\n", uLen); 
        return 0 ;   
    }
    return pUartInterface[lidarIndex]->writeFunc(pucBuf, uLen);
}


static INT32 lidarRead(int lidarIndex, PUINT8 pucBuf, UINT32 uLen)
{
    INT32 reVal = 0;
    if(pUartInterface == NULL)
        return 0 ;
    reVal = pUartInterface[lidarIndex]->readFunc(pucBuf, uLen);
    //sysprintf("[%d}", reVal); 
    return reVal;
}
static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.   
    return TRUE;
}
static BOOL swInit(void)
{   
    //xRunningSemaphore  = xSemaphoreCreateMutex(); 
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL LidarDrvInit(void)
{
    sysprintf("LidarDrvInit!!\n");
    for(int i = 0; i<LIDAR_NUM; i++)
    {
        pUartInterface[i] = UartGetInterface(uartInterface[i]);
        if(pUartInterface[i] == NULL)
        {
            sysprintf("LidarDrvInit ERROR (pUartInterface[%d] == NULL)!!\n", i);
            return FALSE;
        }
        if(pUartInterface[i]->initFunc(115200) == FALSE)
        {
            sysprintf("LidarDrvInit ERROR (initFunc[%d] false)!!\n", i);
            return FALSE;
        }
        pUartInterface[i]->setRS232PowerFunc(FALSE);
        pUartInterface[i]->setPowerFunc(FALSE);
        
        resetCheckStableStatus(i, checkStableDistData[i], &checkStableDistIndex[i]);
    }
    
    if(hwInit() == FALSE)
    {
        sysprintf("LidarDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("LidarDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}
static int getHeader(uint8_t* data, int dataLen)
{
    for(int i = 0; i<(dataLen-1); i++)
    {
        if((data[i] == LIDAR_HEADER) && 
            (data[i+1] == LIDAR_HEADER))
        {
            //sysprintf("--- getHeader return %d ----\r\n", i);
            return i;
        }
    }
    return -1;
}
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
/*
static void logBuffData(char* titleStr, pm_feature* feature, int len)
{
    
    char str[1024]; 
    uint8_t* dataPr = (uint8_t*)feature;
    memset(str, 0x0, sizeof(str));                                        
    for(int i = 0; i<len; i++)
    { 
        char tmp[3];
        sprintf(tmp, "%02X", dataPr[i]);
        memcpy(str + 2*i, tmp, 2);
    }                                            

    //sprintf(emptyFeatureStr, "   ~~~~~ (%d)[%s:%s] ~~~\r\n",len, titleStr, str); 
    sprintf(logStr, "   ~~~~~ [%s:%s] ~~~\r\n", titleStr,str); 
    LoglibPrintfEx(LOG_TYPE_WARNING, logStr, FALSE); 
}
*/
static char* getTitleStr(int index)
{
    switch(index)
    {
        case LIDAR_FEATURE_OCCUPIED:
            return "OCCUPIED";
        case LIDAR_FEATURE_VACUUM:
            return "VACUUM";
        case LIDAR_FEATURE_UN_STABLED:
            return "UN_STABLED";
        case LIDAR_FEATURE_INIT:
            return "INIT";
        case LIDAR_FEATURE_FAIL:
            return "FAIL";
        default:
            return "UNKNOWN";

    }
    
}
#if(0)
    if(counterNumber[lidarIndex] < 20+lidarIndex*10)
    {
        if(prevFeature[lidarIndex] != LIDAR_FEATURE_OCCUPIED)
        {
            *changeFlag = TRUE;
        }
        returnVal = LIDAR_FEATURE_OCCUPIED;
    }
    else if(counterNumber[lidarIndex] < 30+lidarIndex*10)
    {
        if(prevFeature[lidarIndex] != LIDAR_FEATURE_VACUUM)
        {
            *changeFlag = TRUE;
        }
        returnVal = LIDAR_FEATURE_VACUUM;
    }
    else
    {
        counterNumber[lidarIndex] = 0;
    }
    prevFeature[lidarIndex] = returnVal;
    counterNumber[lidarIndex]++;
#endif

int LidarCheckFeature(int lidarIndex, BOOL* changeFlag, void* para1, void* para2, void* para3)
{
    int returnVal = LIDAR_FEATURE_FAIL;//LIDAR_FEATURE_UN_STABLED;
    #if(1)
    sysprintf("\r\n --- [information:LIDAR] LidarCheckFeature!![%d] (CheckDist:(%d, %d, %d), MinPower:%d, StableRange:%d ,StableCounter:%d, VacuumCounter:%d, counter:%d) ENTER  ----\r\n", 
                            lidarIndex, checkExceedDist[lidarIndex], checkMaxDist[lidarIndex], checkMinDist[lidarIndex], checkMinPower[lidarIndex],
                            checkStableDistRange[lidarIndex], checkStableCounter[lidarIndex], checkVacuumCounter[lidarIndex], counterNumber[lidarIndex]);
    #endif
    //int counter = 0;
    int* reDist = para1;
    int* rePower = para2;
    char* titleStr;
    uint8_t* pTargetPacket;
    uint8_t* pLidarBuffer;// = (uint8_t*)&lidarBuffer[lidarIndex][lidarIndex];
//    char debugStr[64];
    int getTimes = 0;
    int totalDist = 0;
    int totalPower = 0;
    #if(ENABLE_MAX_MIN_FILTER)
    int maxDist = 0, minDist = 0;
    int maxPower = 0, minPower = 0;
    #endif
    TickType_t tickLocalStart = xTaskGetTickCount();
    
    pTargetPacket = (uint8_t*)&targetPacket[lidarIndex];
    pLidarBuffer = (uint8_t*)&lidarBuffer[lidarIndex];
    
    *changeFlag = FALSE;
    lidarBufferIndex[lidarIndex] = 0;
    targetPacketIndex[lidarIndex] = 0;
    //*reDist = 0;
    if(lidarIndex == 0)
    {
        titleStr = " [L] ";
    }
    else
    {
        titleStr = " [R] ";
    }
    
    //xSemaphoreTake(xRunningSemaphore, portMAX_DELAY);
    
    //lidarFlushRxBuffer(lidarIndex);  
    //LidarSetPowerStatus(lidarIndex, TRUE);    
    
    //vTaskDelay(500/portTICK_RATE_MS);
    
    tickLocalStart = xTaskGetTickCount();
    lidarFlushRxBuffer(lidarIndex); 
    
    //lidarWrite(lidarIndex, outputOnCmd, sizeof(outputOnCmd));
    
    while(1)
    {
        int headerIndex = 0;
        int leftPacketLen = 0;
        if((xTaskGetTickCount() - tickLocalStart) > LIDAR_TIMEOUT_TIME)
        {
            sysprintf("\n\r [ERROR:LIDAR]-%s-    TIMEOUT(%d, %d) --\r\n", titleStr, targetPacketIndex[lidarIndex], leftPacketLen);
            BuzzerPlay(50, 100, lidarIndex+1, TRUE);
            vTaskDelay(500/portTICK_RATE_MS);
            BuzzerPlay(300, 50, 1, FALSE);
            timeoutCounter[lidarIndex]++;
            goto LidarCheckFeatureExit;
        }
      
        //sysprintf(" ~~~ Wait LidarCheckFeature read -> packet: index = %d, left = %d ; buffer: index = %d ...\r\n", targetPacketIndex[lidarIndex], leftPacketLen, lidarBufferIndex[lidarIndex]);
        
        //vTaskDelay(200/portTICK_RATE_MS);
        int lidarBufferLeft = lidarRead(lidarIndex, pLidarBuffer, LIDAR_BUFFER_SIZE);
        //sysprintf(" ~~~ LidarCheckFeature read (%d bytes) -> packet: index = %d, left = %d ; buffer: index = %d, left = %d ...\r\n", lidarBufferLeft, targetPacketIndex[lidarIndex], leftPacketLen, lidarBufferIndex[lidarIndex], lidarBufferLeft);
        if(lidarBufferLeft == 0)
        {
            //sysprintf("^");
            vTaskDelay(100/portTICK_RATE_MS);
            continue;
        }
        if(targetPacketIndex[lidarIndex] == 0)
        {
            headerIndex = getHeader(pLidarBuffer, lidarBufferLeft);
            if(headerIndex == -1)
            {
                sysprintf("cant find header!!!\r\n");
                //if(lidarBufferLeft > 64)
                //{
                //    lidarBufferLeft = 64;
                //}
                //logBuffData("LIDAR CANT FIND HEADER", (pm_feature*)pLidarBuffer, lidarBufferLeft);
                printfBuffData("LIDAR CANT FIND HEADER", pLidarBuffer, lidarBufferLeft);
                continue;
            }
            //sysprintf("find header!!!\r\n");
            lidarBufferLeft = lidarBufferLeft - headerIndex;
            lidarBufferIndex[lidarIndex] = headerIndex;
        }
        else
        {        
            lidarBufferIndex[lidarIndex] = 0;
        }
        leftPacketLen = sizeof(lidarPacket) - targetPacketIndex[lidarIndex];
        //sysprintf("\r\nLidarCheckFeature read (%d bytes) -> packet: index = %d, left = %d ; buffer: index = %d, left = %d...\r\n", lidarBufferLeft, targetPacketIndex[lidarIndex], leftPacketLen, lidarBufferIndex[lidarIndex], lidarBufferLeft);
        while(lidarBufferLeft > leftPacketLen)
        {
            uint8_t checkSum;
            int currentDist = 0;
            int currentPower = 0;
            //sysprintf(" - LidarCheckFeature move full data -> packet(%d) from buffer(%d) : leftPacketLen = %d...\r\n", targetPacketIndex[lidarIndex], lidarBufferIndex[lidarIndex], leftPacketLen);
            //sysprintf(" - %d(%d):%d(%d)-\r\n", targetPacketIndex[lidarIndex], leftPacketLen, lidarBufferIndex[lidarIndex], lidarBufferLeft);
            //if(((xTaskGetTickCount() - tickLocalStart) > LIDAR_TIMEOUT_TIME) && (ignoreDataTimes == 0))
            if((xTaskGetTickCount() - tickLocalStart) > LIDAR_TIMEOUT_TIME)
            {
                sysprintf("\n\r [ERROR:LIDAR]-%s-    TIMEOUT 2(%d, %d) --\r\n", titleStr, lidarBufferLeft, leftPacketLen);
                //////BuzzerPlay(50, 100, lidarIndex+1, TRUE);
                //vTaskDelay(500/portTICK_RATE_MS);
               // BuzzerPlay(500, 50, 1, FALSE);
                timeoutCounter2[lidarIndex]++;
                goto LidarCheckFeatureExit;
            }            
            memcpy(pTargetPacket + targetPacketIndex[lidarIndex], pLidarBuffer + lidarBufferIndex[lidarIndex], leftPacketLen); 
            //---------
            #if(0)
            sprintf(debugStr, "DUMP LIDAR DATA(%s_%d):", titleStr, getTimes);
            printfBuffData(debugStr, pTargetPacket, sizeof(lidarPacket));
            #endif
            checkSum = 0;
            for(int i = 0; i<sizeof(lidarPacket)-1; i++)
            {
                checkSum = checkSum + pTargetPacket[i];
            }
            if(checkSum == pTargetPacket[sizeof(lidarPacket)-1])
            {     
                
                currentDist = targetPacket[lidarIndex].dist[0] + targetPacket[lidarIndex].dist[1]*256;
                if(currentDist == 0)
                {
                    sysprintf("!");
                    //sysprintf(" =IGNORE= [%d:LIDAR (0x%02x, 0x%02x)]-%s->> (dist:%d cm, power:%d) --\r\n", getTimes, checkSum, pTargetPacket[sizeof(lidarPacket)-1], titleStr, currentDist, currentPower);
                    //sysprintf("  =IGNORE= [%d]-%s->> (dist:%d cm, power:%d) --\r\n", getTimes, titleStr, currentDist, currentPower);
                    zeroCounter[lidarIndex]++;
                }
                else
                {

                    getTimes++;
                    currentPower = targetPacket[lidarIndex].strength[0] + targetPacket[lidarIndex].strength[1]*256;
                    //sysprintf("[%d:LIDAR (0x%02x, 0x%02x)]-%s->> (dist:%d cm, power:%d) --\r\n", getTimes, checkSum, pTargetPacket[sizeof(lidarPacket)-1], titleStr, currentDist, currentPower);
                    //sysprintf("  =GET= [LIDAR]-%s->[%d]> (dist:%d cm, power:%d) --\r\n", titleStr, getTimes, currentDist, currentPower);
                    totalDist = totalDist + currentDist;
                    totalPower = totalPower + currentPower;
                    #if(ENABLE_MAX_MIN_FILTER)
                    if(maxDist == 0)
                    {
                        maxDist = currentDist;
                        maxPower = currentPower;
                    }
                    else
                    {
                        if(currentDist>maxDist)
                        {
                            maxDist = currentDist;
                            maxPower = currentPower;
                        }
                    }
                    
                    if(minDist == 0)
                    {
                        minDist = currentDist;
                        minPower = currentPower;
                    }
                    else
                    {
                        if(currentDist<minDist)
                        {
                            minDist = currentDist;
                            minPower = currentPower;
                        }
                    }
                    #endif
                    if(getTimes >= LIDAR_SAMPLE_TIMES)
                    {            
                        #if(ENABLE_MAX_MIN_FILTER)                        
                        totalDist =  (totalDist - minDist - maxDist)/(getTimes-2);
                        totalPower =  (totalPower - minPower - maxPower)/(getTimes-2);
                        sysprintf(" [INFO] ---> [AVERAGE:LIDAR -%s->> Dist: < %d > cm (max:%d cm, min:%d cm), Power: < %d > (max:%d, min:%d)--\r\n", 
                                                                     titleStr, totalDist, maxDist, minDist, totalPower, maxPower, minPower);
                        #else
                        totalDist =  totalDist/getTimes;
                        sysprintf(" [INFO] ---> [AVERAGE:LIDAR -%s->> Dist: < %d cm >, Power: < %d >--\r\n", titleStr, totalDist, totalPower);
                        #endif
                        //
                        lastDist[lidarIndex] =  totalDist;
                        lastPower[lidarIndex] =  totalPower;
                        //
                        //if((totalDist < checkMaxDist[lidarIndex]) && (totalDist > checkMinDist[lidarIndex]))
                        if(totalDist < checkMinDist[lidarIndex])
                        {
                            returnVal = LIDAR_FEATURE_IGNORE;
                            vacuumCounter[lidarIndex] = 0;
                        }
                        else if(totalDist < checkMaxDist[lidarIndex])
                        {
                            if(checkStableValue(lidarIndex, totalDist, checkStableDistData[lidarIndex], &checkStableDistIndex[lidarIndex], checkStableDistRange[lidarIndex]))
                            {
                                returnVal = LIDAR_FEATURE_OCCUPIED;
                            }
                            else
                            {
                                returnVal = LIDAR_FEATURE_UN_STABLED;
                            }
                            vacuumCounter[lidarIndex] = 0;
                        }
                        //
                        //else if((totalPower <= checkMinPower[lidarIndex]) && (totalPower > 10))
                        else if( ((totalPower <= checkMinPower[lidarIndex]) && (totalPower > 10)) 
                                                                    && (totalDist < checkExceedDist[lidarIndex]) )
                        {
                            //if(checkStableValue(lidarIndex, totalDist, checkStablePowerData[lidarIndex], &checkStablePowerIndex[lidarIndex]))
                            {
                                returnVal = LIDAR_FEATURE_OCCUPIED;
                            }
                            //else
                            //{
                            //    returnVal = LIDAR_FEATURE_UN_STABLED;
                            //}
                            vacuumCounter[lidarIndex] = 0;
                        }
                        //
                        else
                        {                            
                            if(vacuumCounter[lidarIndex] >= (checkVacuumCounter[lidarIndex] -1))
                            {
                                returnVal = LIDAR_FEATURE_VACUUM;  
                                resetCheckStableStatus(lidarIndex, checkStableDistData[lidarIndex], &checkStableDistIndex[lidarIndex]);                              
                            }
                            else
                            {
                                vacuumCounter[lidarIndex]++;
                                returnVal = LIDAR_FEATURE_UN_STABLED;
                                sysprintf(" [INFO] ---> [VACUUM COUNTER:LIDAR -%s->> < %d >  --\r\n", titleStr, vacuumCounter[lidarIndex]);                                
                            }
                            
                        }
                        if((returnVal == LIDAR_FEATURE_OCCUPIED) || (returnVal == LIDAR_FEATURE_VACUUM))
                        {
                            if(prevFeature[lidarIndex] == LIDAR_FEATURE_INIT)
                            {
                                *changeFlag = TRUE;
                                sprintf(logStr, "-- {INFORMATION}   ~~~~~ [WARNING:LIDAR]O%sO[%d] < %d cm, %d > Change to %s (init)  OO\r\n", titleStr, counterNumber[lidarIndex], totalDist, totalPower, getTitleStr(returnVal)); 
                                LoglibPrintfEx(LOG_TYPE_WARNING, logStr, FALSE);   
                            }
                            else
                            {
                                if(prevFeature[lidarIndex] != returnVal)
                                {
                                    
                                    *changeFlag = TRUE;
                                    sprintf(logStr, "-- {INFORMATION}   ~~~~~ [WARNING:LIDAR]O%sO[%d] < %d cm, %d > Change to %s  OO\r\n", titleStr, counterNumber[lidarIndex], totalDist, totalPower, getTitleStr(returnVal)); 
                                    LoglibPrintfEx(LOG_TYPE_WARNING, logStr, FALSE); 
                                    if(returnVal == LIDAR_FEATURE_OCCUPIED)
                                    {
                                        BuzzerPlay(1000, 100, 1, FALSE);
                                        LoglibPrintfEx(LOG_TYPE_WARNING, checkStr, FALSE); 
                                        
                                    }
                                    else
                                    {
                                        BuzzerPlay(500, 100, 1, FALSE);
                                    }
                                                                      
                                }
                            }
                            prevFeature[lidarIndex] = returnVal;                            
                        }
                        counterNumber[lidarIndex]++;
                        *reDist = totalDist;
                        *rePower = totalPower;
                        goto LidarCheckFeatureExit;
                    }
                }
            }
            //----------
            lidarBufferLeft = lidarBufferLeft - leftPacketLen; //buff 還剩多少沒處理
            lidarBufferIndex[lidarIndex] = lidarBufferIndex[lidarIndex] + leftPacketLen; //buff 沒處理的起始INDEX
            
            targetPacketIndex[lidarIndex] = 0;
            leftPacketLen = sizeof(lidarPacket) - targetPacketIndex[lidarIndex];  
            //vTaskDelay(50/portTICK_RATE_MS);  
        }
        if(lidarBufferLeft > 0)
        {
            //sysprintf(" - LIDARCheckFeature move left data -> packet(%d) from buffer(%d) : len = %d...\r\n", targetPacketIndex[lidarIndex], lidarBufferIndex[lidarIndex], lidarBufferLeft);
            //sysprintf(" = %d(%d):%d(%d)=\r\n", targetPacketIndex[lidarIndex], leftPacketLen, lidarBufferIndex[lidarIndex], lidarBufferLeft);
            memcpy(pTargetPacket + targetPacketIndex[lidarIndex], pLidarBuffer + lidarBufferIndex[lidarIndex], lidarBufferLeft);
            lidarBufferIndex[lidarIndex] = 0;
            targetPacketIndex[lidarIndex] = targetPacketIndex[lidarIndex] + lidarBufferLeft;
            //vTaskDelay(200/portTICK_RATE_MS); 
        }        
    }  
LidarCheckFeatureExit:  
    //lidarWrite(lidarIndex, outputOffCmd, sizeof(outputOffCmd));
    //lidarSetPower(lidarIndex, FALSE); 
    //xSemaphoreGive(xRunningSemaphore);
    return returnVal;
}


void LidarSetPowerStatus(int lidarIndex, BOOL flag)
{
    lidarSetPower(lidarIndex, flag);
    vTaskDelay(1000/portTICK_RATE_MS);
    if(flag)
    {
        //lidarWrite(lidarIndex, freqCmd, sizeof(freqCmd));
        //vTaskDelay(3000/portTICK_RATE_MS);
        
        lidarWrite(lidarIndex, powerCmd, sizeof(powerCmd));
        vTaskDelay(1000/portTICK_RATE_MS);
        
        lidarWrite(lidarIndex, powerSaveCmd, sizeof(powerSaveCmd));
        //vTaskDelay(3000/portTICK_RATE_MS);
    }
    
}

void LidarSetStartCommand(int lidarindex, char* command)
{
    
}



void LidarSetPower(BOOL flag)
{
    lidarSetPower(0, flag);
    lidarSetPower(1, flag);
}

void LidarLogStatus(void)
{
    sprintf(logStr, "-- {LIDAR counterNumber: %d, %d, last dist:(%d cm, %d cm), last power:(%d, %d), timeoutCounter:([%d, %d] [%d, %d]), zeroCounter:[%d, %d], CheckDist:(%d, %d, %d), (%d, %d, %d), MinPower:%d, %d, StableRange:%d, %d ,ConfirmNum:%d, %d} --\r\n", 
                                    counterNumber[0], counterNumber[1], lastDist[0], lastDist[1], lastPower[0], lastPower[1],
                                    timeoutCounter[0], timeoutCounter[1], timeoutCounter2[0], timeoutCounter2[1], 
                                    zeroCounter[0], zeroCounter[1], 
                                    checkExceedDist[0], checkMaxDist[0], checkMinDist[0], checkExceedDist[1], checkMaxDist[1], checkMinDist[1],
                                    checkMinPower[0], checkMinPower[1],
                                    checkStableDistRange[0], checkStableDistRange[1], checkStableCounter[0], checkStableCounter[1]);
    LoglibPrintfEx(LOG_TYPE_INFO, logStr, FALSE);
}

void LidarSetLogFlag(int lidarindex)
{
    //logStatus[lidarindex] = TRUE;
}

void LidarSetDumpRawData(int lidarindex, BOOL flag)
{
    //dumpStatus[lidarindex] = flag;
}

void LidarSetCheckDistExceedMaxMin(int index, int exceed, int max, int min)
{    
    checkExceedDist[index] = exceed;
    checkMaxDist[index] = max;
    checkMinDist[index] = min;
    sysprintf("  =SET= [LidarSetCheckDistExceedMaxMin[%d]]> (exceed:%d, max:%d, min:%d) --\r\n", index, checkExceedDist[index], checkMaxDist[index], checkMinDist[index]);
}

void LidarSetCheckPowerMin(int index, int min)
{    
    checkMinPower[index] = min;
    sysprintf("  =SET= [LidarSetCheckPowerMin[%d]]> (min:%d) --\r\n", index, checkMinPower[index]);
}
void LidarSetStableCounter(int lidarIndex, int checkCounter)
{
    checkStableCounter[lidarIndex] = checkCounter;
    sysprintf("  =SET= [LidarSetStableCounter[%d]]> (checkCounter:%d) --\r\n", lidarIndex, checkStableCounter[lidarIndex]);
}
void LidarSetStableDistRange(int lidarIndex, int checkCounter)
{
    checkStableDistRange[lidarIndex] = checkCounter;
    sysprintf("  =SET= [LidarSetStableCounter[%d]]> (checkCounter:%d) --\r\n", lidarIndex, checkStableDistRange[lidarIndex]);
}
void LidarSetVacuumCounter(int lidarIndex, int checkCounter)
{
    checkVacuumCounter[lidarIndex] = checkCounter;
    sysprintf("  =SET= [LidarSetVacuumCounter[%d]]> (checkCounter:%d) --\r\n", lidarIndex, checkVacuumCounter[lidarIndex]);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


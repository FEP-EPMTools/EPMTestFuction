/**************************************************************************//**
* @file     radardrv.c
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

#include "radardrv.h"

#include "buzzerdrv.h"
#include "loglib.h"
#include "dataagent.h"
#include "gpio.h"
#include "hwtester.h"
#if (ENABLE_BURNIN_TESTER)
#include "burnintester.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define RADAR_1_UART   UART_3_INTERFACE_INDEX //UART_7_INTERFACE_INDEX
#define RADAR_2_UART   UART_7_INTERFACE_INDEX //UART_3_INTERFACE_INDEX

#define RADAR_NUM       2

#define RADAR_BUFFER_SIZE  (1024) 
#define RADAR_TIMEOUT_TIME  waitTimeoutTick[radarIndex]//(5000/portTICK_RATE_MS)//(2000/portTICK_RATE_MS)

#define RADAR_TIMEOUT_TIME_NORMAL  (5000/portTICK_RATE_MS) //(10000/portTICK_RATE_MS)//(5000/portTICK_RATE_MS)//(2000/portTICK_RATE_MS)
#define RADAR_TIMEOUT_TIME_CALIBRATION  ((15*1000)/portTICK_RATE_MS)//((75*1000)/portTICK_RATE_MS)//(2000/portTICK_RATE_MS)

#define CHANGE_COUNTER_CONFIRM_MAX_MUN      15

#define CHECK_VALUE_EXCEED_DISTANCE         400
#define CHECK_VALUE_MAX_DISTANCE            300
#define CHECK_VALUE_MIN_DISTANCE            30
#define CHECK_VALUE_MIN_POWER               45
#define CHECK_STABLE_DISTANCE_RANGE         5
#define CHECK_STABLE_POWER_RANGE         5
//#define CHECK_CHANGE_COUNTER_CONFIRM_DEFAULT_MUN  5
#define CHECK_CHANGE_COUNTER_CONFIRM_DEFAULT_MUN  3//by Jer
#define CHECK_LOWPOWER_CHANGE_COUNTER_CONFIRM_DEFAULT_MUN  2
#define CHECK_VACUUM_COUNTER                2
#define CHECK_OCCUPIED_COUNTER              3

#define CHECK_MIN_POWER_FILTER      5
//-----
#define RADAR_SAMPLE_TIMES      1//5
#define ENABLE_MAX_MIN_FILTER   0//1

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//static SemaphoreHandle_t xActionSemaphore[RADAR_NUM] ;
//static TickType_t threadWaitTime[RADAR_NUM]  = {portMAX_DELAY};
//static SemaphoreHandle_t xRunningSemaphore = NULL;

static void initialRingBufFun(void);
static int SaveRingBufFun(int bufIndex,uint8_t* Buff,int size);
static int ReadRingBufFun(int bufIndex,uint8_t* Buff);
static void ShiftReadIndex(int bufIndex, int size);

static uint8_t radarBuffer[RADAR_NUM][RADAR_BUFFER_SIZE];
static uint32_t radarBufferIndex[RADAR_NUM];

static RingfifoStruct RingFifo[RING_FIFO_DEVICE];
static RingfifoStruct* RingFifoPtr[RING_FIFO_DEVICE];

static radarPacket targetPacket[RADAR_NUM];
static radarCalibrate targetCalibratePacket[RADAR_NUM];
static radarVersion targetVersionPacket[RADAR_NUM];

static uint32_t targetPacketIndex[RADAR_NUM];

static int checkStableDistData[RADAR_NUM][CHANGE_COUNTER_CONFIRM_MAX_MUN];
static int checkStableDistIndex[RADAR_NUM];

static int checkStablePowerData[RADAR_NUM][CHANGE_COUNTER_CONFIRM_MAX_MUN];
static int checkStablePowerIndex[RADAR_NUM];

//static int checkStablePowerData[RADAR_NUM][CHANGE_COUNTER_CONFIRM_MAX_MUN];
//static int checkStablePowerIndex[RADAR_NUM];

static UartInterface* pUartInterface[RADAR_NUM] = {NULL, NULL};
static int uartInterface[RADAR_NUM] = {RADAR_1_UART, RADAR_2_UART};
static int prevFeature[RADAR_NUM]   = {RADAR_FEATURE_INIT, RADAR_FEATURE_INIT};


static int counterNumber[RADAR_NUM] = {0, 0};
static char logStr[1024];
static char checkStr[2][1024];

static int checkExceedDist[RADAR_NUM] = {CHECK_VALUE_EXCEED_DISTANCE, CHECK_VALUE_EXCEED_DISTANCE};
static int checkMaxDist[RADAR_NUM] = {CHECK_VALUE_MAX_DISTANCE, CHECK_VALUE_MAX_DISTANCE};
static int checkMinDist[RADAR_NUM] = {CHECK_VALUE_MIN_DISTANCE, CHECK_VALUE_MIN_DISTANCE};
static int checkMinPower[RADAR_NUM] = {CHECK_VALUE_MIN_POWER, CHECK_VALUE_MIN_POWER};
static int checkStableDistRange[RADAR_NUM] = {CHECK_STABLE_DISTANCE_RANGE, CHECK_STABLE_DISTANCE_RANGE};
static int checkStablePowerRange[RADAR_NUM] = {CHECK_STABLE_POWER_RANGE, CHECK_STABLE_POWER_RANGE};
static int checkStableCounter[RADAR_NUM] = {CHECK_CHANGE_COUNTER_CONFIRM_DEFAULT_MUN, CHECK_CHANGE_COUNTER_CONFIRM_DEFAULT_MUN};
static int checkLowPowerStableCounter[RADAR_NUM] = {CHECK_LOWPOWER_CHANGE_COUNTER_CONFIRM_DEFAULT_MUN, CHECK_LOWPOWER_CHANGE_COUNTER_CONFIRM_DEFAULT_MUN};
static int checkVacuumCounter[RADAR_NUM] = {CHECK_VACUUM_COUNTER, CHECK_VACUUM_COUNTER};
//static int checkOccupiedCounter[RADAR_NUM] = {CHECK_OCCUPIED_COUNTER, CHECK_OCCUPIED_COUNTER};

static int lastDist [RADAR_NUM] = {0, 0};
static int lastPower[RADAR_NUM] = {0, 0};
static int timeoutCounter[RADAR_NUM]  = {0, 0};
static int timeoutCounter2[RADAR_NUM] = {0, 0};
static int timeoutCounter3[RADAR_NUM] = {0, 0};
static int zeroCounter[RADAR_NUM] = {0, 0};
static int vacuumCounter[RADAR_NUM] = {0, 0};
static int occupiedCounter[RADAR_NUM] = {0, 0};
static int lowPowerCounter[RADAR_NUM] = {0, 0};

static BOOL stableFlag[RADAR_NUM] = {FALSE, FALSE};


static BOOL powerStatus[RADAR_NUM] = {FALSE, FALSE};
static BOOL calibrationFlag[RADAR_NUM] = {FALSE, FALSE};
static BOOL queryversionFlag[RADAR_NUM] = {FALSE, FALSE};


static uint16_t lidarRecentDist[RADAR_NUM] = {0, 0};
static uint16_t ridarRecentDist[RADAR_NUM] = {0, 0};
static BOOL NewValueFlag[RADAR_NUM] = {FALSE, FALSE};

static int  lidarCalibrateStatusFlag[RADAR_NUM] = {FALSE, FALSE};                           
static int  lidarCalibrateDistValue[RADAR_NUM] = {0, 0};

static TickType_t waitTimeoutTick[RADAR_NUM] = {RADAR_TIMEOUT_TIME_NORMAL, RADAR_TIMEOUT_TIME_NORMAL};

//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x01, 0x00, 0x64};//1hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x02, 0x00, 0x65};//2hz
static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x03, 0x00, 0x66};//3hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x0a, 0x00, 0x6d};//10hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x14, 0x00, 0x77};//20hz
//static uint8_t freqCmd[6] = {0x5A, 0x06, 0x03, 0x64, 0x00, 0xC7};//100hz


//static uint8_t outputOffCmd[5] = {0x5A, 0x05, 0x07, 0x00, 0x66};//5A 05 07 00 66
static uint8_t outputOnCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x02, 0x00, 0x0B, 0xD3, 0x3D};
//static uint8_t calibrationCmd[11] = {0x7A, 0xA7, 0x00, 0x0B, 0x07, 0x00, 0x3C, 0x00, 0x4E, 0xD3, 0x3D}; //60 secs
static uint8_t calibrationCmd[11] = {0x7A, 0xA7, 0x00, 0x0B, 0x07, 0x00, 0x46, 0x00, 0x58, 0xD3, 0x3D}; //70 secs

static uint8_t versionCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D}; //70 secs

//static uint8_t versionCmd[11] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D}; //70 secs

static uint8_t powerCmd[5] = {0x5A, 0x05, 0x22, 0x00, 0x81};//5A 05 22 00 81      //0
//static uint8_t powerCmd[5] = {0x5A, 0x05, 0x22, 0x05, 0x86};//5A 05 22 0x05 86  //5*10=50
//static uint8_t powerCmd[5] = {0x5A, 0x05, 0x22, 0x01, 0x82};//5A 05 22 0x01 82  //1*10=10

static uint8_t powerSaveCmd[4] = {0x5A, 0x04, 0x11, 0x6F};//5A 04 11 6F

static uint8_t output0Cmd[10] = {0x7A, 0xA7, 0x00, 0x0A, 0x30, 0x01, 0x00, 0x3B, 0xD3, 0x3D};
static uint8_t output1Cmd[10] = {0x7A, 0xA7, 0x00, 0x0A, 0x30, 0x02, 0x00, 0x3C, 0xD3, 0x3D};


#define RECEIVE_TEMP_BUFF_LEN  1024
#define RECEIVE_DATA_BUFF_LEN  18//16
#define OTA_DATA_ENVELOPE_SIZE  256//10 //5
#define MAX_WAIT_ACK_TIME   2000//200




static uint8_t firstcmdTakeAck[] = {0x7A,0xA7,0x00,0x09,0x15,0x00,0x1E,0xD3,0x3D};

static uint8_t cmdTakeAck[] = {0x7A,0xA7,0x00,0x0B,0x16,0x00,0x00,0x00,0x00,0xD3,0x3D};
static uint8_t receiveData[RECEIVE_DATA_BUFF_LEN];
static uint8_t receiveDataTmp[RECEIVE_TEMP_BUFF_LEN];
static int receiveDataIndex = 0;

static uint8_t firstCmd[] = {0x7A,0xA7,0x00,0x0D,0x05,0x00,0x00,0x00,0x01,0x00,0x13,0xD3,0x3D};

//static uint8_t firstCmd[] = {0x7A,0xA7,0x00,0x09,0x00,0x00,0x09,0xD3,0x3D};

//static uint8_t firstCmd[] = {0x7A,0xA7,0x00,0x09,0x02,0x00,0x0B,0xD3,0x3D};
 
static uint8_t *Cmd;

static SemaphoreHandle_t xSemaphore;
static TickType_t threadWaitTime = portMAX_DELAY;
static TaskHandle_t xHandle = NULL;
static BOOL RadarPoolingFlag = FALSE;

#if (ENABLE_BURNIN_TESTER)
//static char errorMsgBuffer[256];
static char errorMsgBuffer[4096];
#endif

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


//static void resetCheckStableStatus(int radarIndex, int* dataBuffer, int* bufferIndex)
//{
//    //sysprintf("\r\n = [WARNING RESET [%d]] ==   reset CheckStableStatus ====\r\n\r\n", radarIndex);
//    for(int i = 0; i<checkStableCounter[radarIndex]; i++)
//    {
//        dataBuffer[i] = 0;
//    }
//    *bufferIndex = 0;
//    stableFlag[radarIndex] = FALSE;
//}

//static BOOL unstableLogFlag[RADAR_NUM] = {TRUE, TRUE};
//static BOOL checkStableValue(int radarIndex, int compareData, int* dataBuffer, int* bufferIndex, int valueRange, char* typeStr)
//{
//    BOOL reVal = TRUE;
//    BOOL addDataFlag = TRUE;
//    BOOL fullFlag = stableFlag[radarIndex];
//    sysprintf("\r\n");
//    for(int i = 0; i<checkStableCounter[radarIndex]; i++)
//    {
//        int currentDist = abs(dataBuffer[i] - compareData);
//        if(dataBuffer[i] !=0)
//        {
//            if(currentDist > valueRange)
//            {
//                sysprintf(" = [WARNING CHECK [%d]] ==   check StableValue[%d]:[%d-%d => %d vs %d] FALSE ====\r\n", radarIndex, i, compareData, dataBuffer[i], currentDist, valueRange);
//                reVal = FALSE;
//                //尚未積滿五個
//                if(fullFlag == TRUE)
//                {                    
//                    addDataFlag = FALSE;
//                }
//                break;
//            }
//            else
//            {
//                sysprintf(" = [WARNING CHECK [%d]] ==   check StableValue[%d]:[%d-%d => %d vs %d] TRUE ====\r\n", radarIndex, i, compareData, dataBuffer[i], currentDist, valueRange);
//            }
//        }
//        else
//        {
//            sysprintf(" = [WARNING CHECK [%d]] ==   check StableValue[%d]:[%d-%d => %d vs %d] FALSE (IGNORE) ====\r\n", radarIndex, i, compareData, dataBuffer[i], currentDist, valueRange);
//            reVal = FALSE;
//            break;
//        }
//    }
//    
//      
//    if(addDataFlag)
//    {
//        dataBuffer[*bufferIndex] = compareData;  
//        sysprintf(" = [WARNING SET [%d]] check StableValue:add %d in index %d ====\r\n\r\n", radarIndex, compareData, *bufferIndex);
//        *bufferIndex = (*bufferIndex+1)%checkStableCounter[radarIndex];
//    }
// 
//    
//    if(reVal)
//    {
//        stableFlag[radarIndex] = TRUE;
//        //if(unstableLogFlag[radarIndex] == TRUE)
//        //{
//            sprintf(checkStr[radarIndex], "-- {INFORMATION _2 %s}[%d][%d] TRUE(V) ~~ < %d : %d, %d, %d, %d, %d> \r\n", 
//                                                                    typeStr, radarIndex, counterNumber[radarIndex], compareData, dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3], dataBuffer[4]); 
//       
//            //LoglibPrintf(LOG_TYPE_WARNING, checkStr, FALSE);
//        //    unstableLogFlag[radarIndex] = FALSE;
//        //}
//    }
//    else
//    {
//        //if(stableFlag[radarIndex])
//        //{
//            sprintf(checkStr[radarIndex], "-- {INFORMATION _2 %s}[%d][%d] FALSE(X) ~~ < %d : %d, %d, %d, %d, %d> \r\n", 
//                                                                    typeStr, radarIndex, counterNumber[radarIndex], compareData, dataBuffer[0], dataBuffer[1], dataBuffer[2], dataBuffer[3], dataBuffer[4]); 
//            //LoglibPrintf(LOG_TYPE_WARNING, checkStr, FALSE);
//        //    unstableLogFlag[radarIndex] = TRUE;
//        //}
//    }
//    return reVal;
//}
static void radarFlushRxBuffer(int radarIndex)
{
    //sysprintf("radarFlushRxBuffer [%d]!!\n", radarIndex);
    if (pUartInterface[radarIndex]->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}

static void radarSetPower(int radarIndex, BOOL flag)
{
    //terninalPrintf("~[Warning]~ radarSetPower [%d]:%d enter!!\n", radarIndex, flag);    
    if(flag)
    {
        pUartInterface[radarIndex]->setPowerFunc(TRUE);
        pUartInterface[radarIndex]->setRS232PowerFunc(TRUE);
        powerStatus[radarIndex] = TRUE;
        //vTaskDelay(2000/portTICK_RATE_MS);
    }
    else
    {    
        pUartInterface[radarIndex]->setRS232PowerFunc(FALSE);
        pUartInterface[radarIndex]->setPowerFunc(FALSE);
        powerStatus[radarIndex] = FALSE;
    }
    //terninalPrintf("~~ radarSetPower [%d]:%d exit!!\n", radarIndex, flag);
}

static INT32 radarWrite(int radarIndex, PUINT8 pucBuf, UINT32 uLen)
{
    //sysprintf("--> radarWrite len=%d:[%s] \r\n", uLen, pucBuf); 
    /* DEBUG RADAR *///terninalPrintf("=>");
    /* DEBUG RADAR *///for(int i=0;i<uLen;i++)
    /* DEBUG RADAR *///    terninalPrintf("%02x ",pucBuf[i]);
    /* DEBUG RADAR *///terninalPrintf("\n");
    if(pUartInterface[radarIndex] == NULL)
    {
        //sysprintf("radarWrite %d...\r\n", uLen); 
        return 0 ;   
    } 
    return pUartInterface[radarIndex]->writeFunc(pucBuf, uLen);
}

static int getHeader(uint8_t* data, int dataLen)
{
    for(int i = 0; i<(dataLen-1); i++)
    {
        if((data[i] == RADAR_HEADER) && 
            (data[i+1] == RADAR_HEADER_2))
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
    terninalPrintf("\r\n %s: len = %d...\r\n", str, len);
    
    for(i = 0; i<len; i++)
    { 
        terninalPrintf("[%02d:0x%02x], ",i, (unsigned char)data[i]);
        if((i%16) == 15)
            terninalPrintf("\r\n");
    }
    terninalPrintf("\r\n");
    
}

static char* getTitleStr(int index)
{
    switch(index)
    {
        case RADAR_FEATURE_OCCUPIED:
            return "OCCUPIED";
        case RADAR_FEATURE_VACUUM:
            return "VACUUM";
        case RADAR_FEATURE_VACUUM_UN_STABLED:
            return "RADAR_FEATURE_VACUUM_UN_STABLED";
        case RADAR_FEATURE_OCCUPIED_UN_STABLED:
            return "RADAR_FEATURE_OCCUPIED_UN_STABLED";
        case RADAR_FEATURE_INIT:
            return "INIT";
        case RADAR_FEATURE_FAIL:
            return "FAIL";
        default:
            return "UNKNOWN";

    }
}

static char* getMaterialStr(int index)
{
    /*
    Material :
        0x00: Empty
        0x01: Metal
        0x02: Wood
        0x03: Water
        0x04: Paper Cartoon
    */
    switch(index)
    {
        case 0:
            return " [Empty] ";
        case 1:
            return " [Metal] ";
        case 2:
            return " [Wood] ";
        case 3:
            return " [Water] ";
        case 4:
            return " [Paper Cartoon] ";
        default:
            return " [UNKNOWN] ";

    }
    
}

static char* getObjectStr(int index)
{
    /*
    2. Object
        0x00: Empty
        0x01: Car
        0x02: Motor Bike
        0x03: Motor Cycle
        0x04: Human Beings
    */
    switch(index)
    {
        case 0:
            return " [Empty] ";
        case 1:
            return " [Car] ";
        case 2:
            return " [Motor Bike] ";
        case 3:
            return " [Motor Cycle] ";
        case 4:
            return " [Human Beings] ";
        default:
            return " [UNKNOWN] ";

    }
    
}

#if(0)
    if(counterNumber[radarIndex] < 20+radarIndex*10)
    {
        if(prevFeature[radarIndex] != RADAR_FEATURE_OCCUPIED)
        {
            *changeFlag = TRUE;
        }
        returnVal = RADAR_FEATURE_OCCUPIED;
    }
    else if(counterNumber[radarIndex] < 30+radarIndex*10)
    {
        if(prevFeature[radarIndex] != RADAR_FEATURE_VACUUM)
        {
            *changeFlag = TRUE;
        }
        returnVal = RADAR_FEATURE_VACUUM;
    }
    else
    {
        counterNumber[radarIndex] = 0;
    }
    prevFeature[radarIndex] = returnVal;
    counterNumber[radarIndex]++;
#endif
static int checkLowPowerCounterFun(int radarIndex)
{
    int returnVal;
    if(lowPowerCounter[radarIndex] >= (checkLowPowerStableCounter[radarIndex] -1))
    {
        returnVal = RADAR_FEATURE_OCCUPIED;
    }
    else
    {
        lowPowerCounter[radarIndex]++;
        returnVal = RADAR_FEATURE_OCCUPIED_UN_STABLED;
        sysprintf(" [INFO] -- checkLowPowerCounterFun -> [OCCUPIED COUNTER:RADAR -%d->> < %d vs %d>  --\r\n", radarIndex, lowPowerCounter[radarIndex], checkLowPowerStableCounter[radarIndex]); 
    }
    return returnVal;
}

static INT32 radarRead(int radarIndex, PUINT8 pucBuf, UINT32 uLen)
{
    INT32 reVal = 0;
    if(pUartInterface == NULL)
        return 0 ;
    reVal = pUartInterface[radarIndex]->readFunc(pucBuf, uLen);
    /*
    if(calibrationFlag[radarIndex] == TRUE)
    {
        //sysprintf("[%d}", reVal); 
        char debugStr[64];
        sprintf(debugStr, "radarRead[%d]", radarIndex);
        if(reVal != 0)
        {
            printfBuffData(debugStr, pucBuf, reVal);
        }
        else
        {
            terninalPrintf("|");
        }
    }*/
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

static void initialRingBufFun(void){
    int i;
    for(i=0;i<RING_FIFO_DEVICE;i++)
        memset(RingFifoPtr[i],0x00,sizeof(RingfifoStruct));
}

static int SaveRingBufFun(int bufIndex,uint8_t* Buff,int size){
   
    int i;
    for(i=0;i<size;i++)
        RingFifoPtr[bufIndex]->Buff[ (RingFifoPtr[bufIndex]->SaveIndex + i) % RING_BUFFER_SIZE ] = Buff[i];
        
    RingFifoPtr[bufIndex]->SaveIndex = ( RingFifoPtr[bufIndex]->SaveIndex + size ) % RING_BUFFER_SIZE;
    
    return TRUE;
}


static int ReadRingBufFun(int bufIndex,uint8_t* Buff){
    

    int i;
    short size;
    if( RingFifoPtr[bufIndex]->SaveIndex == RingFifoPtr[bufIndex]->ReadIndex )
        return -1;
    else{
        size = RingFifoPtr[bufIndex]->SaveIndex - RingFifoPtr[bufIndex]->ReadIndex;
        if(size < 0)
            size += RING_BUFFER_SIZE;
        for(i=0;i<size;i++)
        {
            Buff[i] = RingFifoPtr[bufIndex]->Buff[ (RingFifoPtr[bufIndex]->ReadIndex + i) % RING_BUFFER_SIZE ];
            /*
            if(size < 5)
            {
            terninalPrintf("size = ");
            for(int i=0;i<size;i++)
                terninalPrintf("%02x ",RingFifoPtr[bufIndex]->Buff[ (RingFifoPtr[bufIndex]->ReadIndex + i) % RING_BUFFER_SIZE ]);    
            terninalPrintf("\r\n");
            }
            */
        }

        return size;
    }
}

static void ShiftReadIndex(int bufIndex, int size)
{
    RingFifoPtr[bufIndex]->ReadIndex = ( RingFifoPtr[bufIndex]->ReadIndex + size ) % RING_BUFFER_SIZE;
}


static void vRadarTask(void *pvParameters)
{
    INT32 reVal1,reVal2;
    
    for(;;)
    {
        //BaseType_t reval = xSemaphoreTake(xSemaphore, threadWaitTime);
        if(RadarPoolingFlag)
        {
            RadarPoolingFlag = FALSE;
            reVal1 = radarRead(0, (uint8_t*)&radarBuffer[0], RADAR_BUFFER_SIZE);
            reVal2 = radarRead(1, (uint8_t*)&radarBuffer[1], RADAR_BUFFER_SIZE);
            //vTaskDelay(10/portTICK_RATE_MS);
            if(reVal1 > 0)
            {
                SaveRingBufFun(0,(uint8_t*)&radarBuffer[0],reVal1);
            }
            if(reVal2 > 0)
            {
                SaveRingBufFun(1,(uint8_t*)&radarBuffer[1],reVal2);
            }
        }
    }
}

void RadarTaskCreate(void)
{
    xTaskCreate(vRadarTask, "vRadarTask", 1024*10, NULL, RADAR_THREAD_PROI, xHandle);
    //xSemaphore = xSemaphoreCreateBinary();
    
}

void RadarTaskDelete(void)
{
    if( xHandle != NULL )
    {
        vTaskDelete( xHandle );
    }
}


BOOL RadarDrvInit(void)
{
    sysprintf("RadarDrvInit!!\n");
    for(int i = 0; i<RADAR_NUM; i++)
    {
        pUartInterface[i] = UartGetInterface(uartInterface[i]);
        if(pUartInterface[i] == NULL)
        {
            sysprintf("RadarDrvInit ERROR (pUartInterface[%d] == NULL)!!\n", i);
            return FALSE;
        }
        if(pUartInterface[i]->initFunc(115200) == FALSE)
        {
            sysprintf("RadarDrvInit ERROR (initFunc[%d] false)!!\n", i);
            return FALSE;
        }
        pUartInterface[i]->setRS232PowerFunc(FALSE);
        pUartInterface[i]->setPowerFunc(FALSE);
        
//        resetCheckStableStatus(i, checkStableDistData[i], &checkStableDistIndex[i]);
//        resetCheckStableStatus(i, checkStablePowerData[i], &checkStablePowerIndex[i]);
    }
    radarRead(0, (uint8_t*)&radarBuffer[0], RADAR_BUFFER_SIZE);
    radarRead(1, (uint8_t*)&radarBuffer[1], RADAR_BUFFER_SIZE);
    
    for(int j=0;j<RING_FIFO_DEVICE;j++)
        RingFifoPtr[j] = &RingFifo[j];
    
    initialRingBufFun();
    
    if(hwInit() == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}


BOOL RadarDrvInitEx(int radarIndex ,int BaudRate)
{
    sysprintf("RadarDrvInit!!\n");
    pUartInterface[radarIndex] = UartGetInterface(uartInterface[radarIndex]);
    if(pUartInterface[radarIndex] == NULL)
    {
        sysprintf("RadarDrvInit ERROR (pUartInterface[%d] == NULL)!!\n", radarIndex);
        return FALSE;
    }
    
    pUartInterface[radarIndex]->ioctlFunc(80,1,0);
    if(pUartInterface[radarIndex]->initFunc(BaudRate) == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (initFunc[%d] false)!!\n", radarIndex);
        pUartInterface[radarIndex]->ioctlFunc(80,0,0);
        return FALSE;
    }
    pUartInterface[radarIndex]->ioctlFunc(80,0,0);
    
    
    for(int j=0;j<RING_FIFO_DEVICE;j++)
        RingFifoPtr[j] = &RingFifo[j];
    
    initialRingBufFun();
    
    if(hwInit() == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}

BOOL RadarDrvInitBurning(int radarIndex)
{
    sysprintf("RadarDrvInit!!\n");
    pUartInterface[radarIndex] = UartGetInterface(uartInterface[radarIndex]);
    if(pUartInterface[radarIndex] == NULL)
    {
        sysprintf("RadarDrvInit ERROR (pUartInterface[%d] == NULL)!!\n", radarIndex);
        return FALSE;
    }
    if(pUartInterface[radarIndex]->initFunc(115200) == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (initFunc[%d] false)!!\n", radarIndex);
        return FALSE;
    }
    pUartInterface[radarIndex]->setRS232PowerFunc(FALSE);
    pUartInterface[radarIndex]->setPowerFunc(FALSE);
        

    radarRead(radarIndex, (uint8_t*)&radarBuffer[radarIndex], RADAR_BUFFER_SIZE);
    
    
    if(hwInit() == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("RadarDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}


BOOL Decode_StateMachine(int radarIndex ,uint8_t radarCmd,int readBuffSize, uint8_t* radarBuff, uint8_t* radarData)
{
    uint8_t status = S1_DECODE_HEADER;  
    int headerindex = 0;
    uint16_t datalen = 0;
    uint16_t CRCcheck = 0;
    int i;
    
    while(1)
    {   
        switch(status) 
        {
            case S1_DECODE_HEADER:
                while(1)
                {
                    if( (radarBuff[headerindex + 0] == 0x7A) && (radarBuff[headerindex + 1] == 0xA7) )
                    {
                        status = S2_DECODE_DATALEN;
                        break;
                    }
                    else if( (radarBuff[headerindex + 0] == 0x7A) && (radarBuff[headerindex + 1] == 0x00) &&
                             (radarBuff[headerindex + 2] == 0xA7) && (radarBuff[headerindex + 3] == 0x00)  )
                    {
                        #if (ENABLE_BURNIN_TESTER)
                        if (EnabledBurninTestMode())
                        {
                            sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    UART_FORMAT_ERR, headerindex = %d --\r\n", radarIndex+1, headerindex);
                            sprintf(errorMsgBuffer,"%sdata = ",errorMsgBuffer);
                            for(int k=0;k<readBuffSize;k++)
                                sprintf(errorMsgBuffer,"%s%02x ",errorMsgBuffer,radarBuff[k]);
                            sprintf(errorMsgBuffer,"%s\r\n",errorMsgBuffer);
                            AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                            
                            terninalPrintf("[ERROR:RADAR] [%d]    UART_FORMAT_ERR, headerindex = %d --\r\n", radarIndex+1, headerindex);
                            terninalPrintf("data = ");
                            for(int m=0;m<readBuffSize;m++)
                                terninalPrintf("%02x ",radarBuff[m]);
                            terninalPrintf("\r\n");
                        }
                        #endif
                        
                        return UART_FORMAT_ERR;
                    }
                    else
                    {
                        headerindex++;
                        if(( readBuffSize - headerindex) < 9)
                        {
                            #if (ENABLE_BURNIN_TESTER)
                            if (EnabledBurninTestMode() == FALSE)
                            #endif
                                terninalPrintf("[ERROR:RADAR] [%d]    No data, headerindex = %d --\r\n", radarIndex+1, headerindex);
                            
                            #if (ENABLE_BURNIN_TESTER)
                            if (EnabledBurninTestMode())
                            {
                                sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    No data, headerindex = %d --\r\n", radarIndex+1, headerindex);
                                sprintf(errorMsgBuffer,"%sdata = ",errorMsgBuffer);
                                for(int k=0;k<readBuffSize;k++)
                                    sprintf(errorMsgBuffer,"%s%02x ",errorMsgBuffer,radarBuff[k]);
                                sprintf(errorMsgBuffer,"%s\r\n",errorMsgBuffer);
                                AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                                
                                terninalPrintf("[ERROR:RADAR] [%d]    No data, headerindex = %d --\r\n", radarIndex+1, headerindex);
                                terninalPrintf("data = ");
                                for(int m=0;m<readBuffSize;m++)
                                    terninalPrintf("%02x ",radarBuff[m]);
                                terninalPrintf("\r\n");
                            }
                            #endif
                            return UART_FORMAT_ERR;
                        }
                    }
                }
                break;
            case S2_DECODE_DATALEN:
                datalen = ( radarBuff[headerindex + 2] << 8 ) | radarBuff[headerindex + 3] ;
                //if( (datalen == (index - headerindex) ) && ( pRadarBuffer[ (index - headerindex) - 2] == 0xD3 ) && 
                //                                           ( pRadarBuffer[ (index - headerindex) - 1] == 0x3D ) )
                
                if(  datalen <= (readBuffSize - headerindex) )
                {
                    if( ( radarBuff[ (datalen + headerindex) - 2] == 0xD3 ) && 
                        ( radarBuff[ (datalen + headerindex) - 1] == 0x3D ) )
                
                        status = S3_DECODE_CMD;
                    else
                    {
                        #if (ENABLE_BURNIN_TESTER)
                        if (EnabledBurninTestMode() == FALSE)
                        #endif
                            terninalPrintf("[ERROR:RADAR] [%d]    datalen not match, datalen = %d --\r\n", radarIndex+1, datalen);
                        
                        
                        #if (ENABLE_BURNIN_TESTER)
                        if (EnabledBurninTestMode())
                        {
                            sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    datalen not match, datalen = %d --\r\n", radarIndex+1, datalen);
                            sprintf(errorMsgBuffer,"%sdata = ",errorMsgBuffer);
                            for(int k=0;k<readBuffSize;k++)
                                sprintf(errorMsgBuffer,"%s%02x ",errorMsgBuffer,radarBuff[k]);
                            sprintf(errorMsgBuffer,"%s\r\n",errorMsgBuffer);
                            AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                        }
                        #endif
                        
                        return UART_FORMAT_ERR;
                    } 
                    
                }
                else
                    return  RADAR_WAIT_NEXTDATA;
                /*
                if( ( radarBuff[ (datalen + headerindex) - 2] == 0xD3 ) && 
                    ( radarBuff[ (datalen + headerindex) - 1] == 0x3D ) )
                
                    status = S3_DECODE_CMD;
                
                else
                {
                    terninalPrintf("[ERROR:RADAR] [%d]    datalen not match, datalen = %d --\r\n", radarIndex+1, datalen);
                    //terninalPrintf("ERROR3: datalen = %d\r\n",datalen);
                    return FALSE;
                }
                */
                break;
            case S3_DECODE_CMD:
                //if( pRadarBuffer[headerindex + 4] == 0x12)
                if( radarBuff[headerindex + 4] == (radarCmd | 0x10))
                    status = S4_DECODE_CHECKSUM;
                else
                {
                    #if (ENABLE_BURNIN_TESTER)
                    if (EnabledBurninTestMode() == FALSE)
                    #endif
                        terninalPrintf("[ERROR:RADAR] [%d]    cmd not match, headerindex = %d,pRadarBuffer[headerindex + 4] =%d --\r\n", radarIndex+1, headerindex,radarBuff[headerindex + 4]);
                    
                    #if (ENABLE_BURNIN_TESTER)
                    if (EnabledBurninTestMode())
                    {
                        sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    cmd not match, headerindex = %d,pRadarBuffer[headerindex + 4] =%d --\r\n", radarIndex+1, headerindex,radarBuff[headerindex + 4]);
                        sprintf(errorMsgBuffer,"%sdata = ",errorMsgBuffer);
                        for(int k=0;k<readBuffSize;k++)
                            sprintf(errorMsgBuffer,"%s%02x ",errorMsgBuffer,radarBuff[k]);
                        sprintf(errorMsgBuffer,"%s\r\n",errorMsgBuffer);
                        AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                    }
                    #endif
                    
                    
                    
                    return UART_FORMAT_ERR;
                }                    
                break;
            case S4_DECODE_CHECKSUM:
                //for(i=2 ; i<(datalen-4) ; i++)
                for(i = headerindex + 2 ; i<((datalen + headerindex)-4) ; i++)
                    CRCcheck += radarBuff[i];
                //if( CRCcheck == ( ( pRadarBuffer[datalen-4]<<8 | pRadarBuffer[datalen-3] ) ) )
                if( CRCcheck == ( ( radarBuff[(datalen + headerindex)-4]<<8 | radarBuff[(datalen + headerindex)-3] ) ) )
                    status = S5_DECODE_DATABODY;
                else
                {
                    #if (ENABLE_BURNIN_TESTER)
                    if (EnabledBurninTestMode() == FALSE)
                    #endif
                        terninalPrintf("[ERROR:RADAR] [%d]    CRCcheck not match, CRCcheck = %d --\r\n", radarIndex+1, CRCcheck);
                    
                    
                    #if (ENABLE_BURNIN_TESTER)
                    if (EnabledBurninTestMode())
                    {
                        sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    CRCcheck not match, CRCcheck = %d --\r\n", radarIndex+1, CRCcheck);
                        sprintf(errorMsgBuffer,"%sdata = ",errorMsgBuffer);
                        for(int k=0;k<readBuffSize;k++)
                            sprintf(errorMsgBuffer,"%s%02x ",errorMsgBuffer,radarBuff[k]);
                        sprintf(errorMsgBuffer,"%s\r\n",errorMsgBuffer);
                        AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                    }
                    #endif
                    
                    
                    return UART_FORMAT_ERR;
                }
                break;
            case S5_DECODE_DATABODY:
                if(radarData != NULL)
                {
                    if(radarCmd == 0x00)
                        memcpy(radarData,radarBuff + headerindex + 5,4);
                    else if(radarCmd == 0x02)
                        memcpy(radarData,radarBuff + headerindex + 5,22);
                }
                return TRUE;
                //return pRadarBuffer[headerindex + 5]<<24 | pRadarBuffer[headerindex + 6]<<16 | 
                //       pRadarBuffer[headerindex + 7]<<8 | pRadarBuffer[headerindex + 8] ;
                break;

            default:
                #if (ENABLE_BURNIN_TESTER)
                if (EnabledBurninTestMode() == FALSE)
                #endif
                    terninalPrintf("[ERROR:RADAR] [%d]    Unknown --\r\n", radarIndex+1);
                //terninalPrintf("ERROR6\r\n");
                
                
                #if (ENABLE_BURNIN_TESTER)
                if (EnabledBurninTestMode())
                {
                    sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    Unknown --\r\n", radarIndex+1);
                    sprintf(errorMsgBuffer,"%sdata = ",errorMsgBuffer);
                    for(int k=0;k<readBuffSize;k++)
                        sprintf(errorMsgBuffer,"%s%02x ",errorMsgBuffer,radarBuff[k]);
                    sprintf(errorMsgBuffer,"%s\r\n",errorMsgBuffer);
                    AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                }
                #endif
                
                
                
                return FALSE;
                break;
        }
    }
    
}



//int newRadarResult(int radarIndex, BOOL* changeFlag, void* para1, void* para2, void* para3)
int newRadarResult(int radarIndex, uint8_t radarCmd, uint8_t* CmdBuff, uint8_t* radarData)
{
    uint8_t* pRadarBuffer;
    int counter = 0;
    int index = 0;
    int result;

    INT32 reVal;
    uint8_t tempBuff[RING_BUFFER_SIZE];
    int tempsize;
    
    pRadarBuffer = (uint8_t*)&radarBuffer[radarIndex];
    /*
    if(radarIndex == 0)
    {        
        outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<<20)) | (0xa<<20));
        //GPIO_CloseBit(GPIOE,BIT13);
    }
    else if(radarIndex == 1)
    {
        outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<20)) | (0x9<<20));
        //GPIO_CloseBit(GPIOG,BIT5);
    }
    */
    radarSetPower(radarIndex, TRUE);  
    
                             
    if(radarCmd == 0x00)
        radarWrite(radarIndex, CmdBuff, 9);
    else if(radarCmd == 0x02)
        radarWrite(radarIndex, CmdBuff, 24);
    
    
    //while(counter < 30)
    //while(counter < 300)
    while(counter < 290)
    {
        reVal = radarRead(radarIndex, pRadarBuffer + index, RADAR_BUFFER_SIZE - index);
        vTaskDelay(10/portTICK_RATE_MS);
        if(reVal > 0)
        {
            index = index + reVal; 

        }   
        counter++;
    }
    //pUartInterface[radarIndex] -> readWaitFunc(portMAX_DELAY);
    
    if(index > 0)
    {
        SaveRingBufFun(radarIndex,pRadarBuffer,index);
    }
    
    if(index < 9)
    {
        if((index == 1) && (pRadarBuffer[0] == 0x00) )
            return RADAR_RETURN_BLANK;
        else
        {
            #if (ENABLE_BURNIN_TESTER)
            if (EnabledBurninTestMode() == FALSE)
            #endif
                terninalPrintf("[ERROR:RADAR] [%d]    No data, BuffIndex = %d --\r\n", radarIndex+1, index);
            
            #if (ENABLE_BURNIN_TESTER)
            if (EnabledBurninTestMode())
            {
                sprintf(errorMsgBuffer,"[ERROR:RADAR] [%d]    No data, BuffIndex = %d --\r\n", radarIndex+1, index);
                //sprintf(errorMsgBuffer,"%sdata123456 = \r\n",errorMsgBuffer);
                AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
            }
            #endif
            
            
            return FALSE;
        }
    }

        
    
    //terninalPrintf("radar%d <= ",radarIndex+1);
    tempsize = ReadRingBufFun(radarIndex,tempBuff);
    /*
    for(int i=0;i<tempsize;i++)
    {   
        if(radarIndex == 0)
            terninalPrintf("%02x. ",tempBuff[i]);
        else if(radarIndex == 1)
            terninalPrintf("%02x_ ",tempBuff[i]);
    }        
    terninalPrintf("\r\n");
    */
    vTaskDelay(100/portTICK_RATE_MS);

    
    result = Decode_StateMachine( radarIndex,radarCmd,tempsize, tempBuff, radarData);
    //if(Decode_StateMachine( radarIndex,radarCmd,index, pRadarBuffer, radarData) == TRUE)
    
    if(result != RADAR_WAIT_NEXTDATA)
    {
        ShiftReadIndex(radarIndex, tempsize);
        /*
        if(radarIndex == 0)
        {        
            outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<<20)) | (0x0<<20));
            GPIO_OpenBit(GPIOE, BIT13, DIR_INPUT, NO_PULL_UP);         
            //GPIO_CloseBit(GPIOE,BIT13);
        }
        else if(radarIndex == 1)
        {
            outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<20)) | (0x0<<20));
            GPIO_OpenBit(GPIOG, BIT5, DIR_INPUT, NO_PULL_UP); 
            //GPIO_CloseBit(GPIOG,BIT5);
        }
        */
    }
    
    return result;
    
}



int newRadarResultElite(int radarIndex, uint8_t radarCmd, uint8_t* CmdBuff, uint8_t* radarData)
{
    uint8_t* pRadarBuffer;
    int counter = 0;
    int index = 0;
    int result;

    INT32 reVal;
    uint8_t tempBuff[RING_BUFFER_SIZE];
    int tempsize;
    
    pRadarBuffer = (uint8_t*)&radarBuffer[radarIndex];

    radarSetPower(radarIndex, TRUE);  
    
                             
    if(radarCmd == 0x00)
        radarWrite(radarIndex, CmdBuff, 9);
    else if(radarCmd == 0x02)
        radarWrite(radarIndex, CmdBuff, 24);
     //xSemaphoreGive(xSemaphore);
    RadarPoolingFlag = TRUE;
    vTaskDelay(10/portTICK_RATE_MS);
    //terninalPrintf("radar%d <= ",radarIndex+1);
    tempsize = ReadRingBufFun(radarIndex,tempBuff);
    /*
    for(int i=0;i<tempsize;i++)
        terninalPrintf("%02x ",tempBuff[i]);    
    terninalPrintf("\r\n");
    */
    if(tempsize < 9)
    {
        #if (ENABLE_BURNIN_TESTER)
        if (EnabledBurninTestMode() == FALSE)
        #endif
            terninalPrintf("[ERROR:RADAR] [%d]    No data, BuffSize = %d --\r\n", radarIndex+1, tempsize);
        return FALSE;
    }

    result = Decode_StateMachine( radarIndex,radarCmd,tempsize, tempBuff, radarData);
    
    if(result != RADAR_WAIT_NEXTDATA)
    {
        ShiftReadIndex(radarIndex, tempsize);

    }
    
    return result;
    
}

int newRadarResultPure(int radarIndex, uint8_t* CmdBuff, int CmdBuffLen, uint8_t* radarData, int* radarDataLen,int WaitTime)
{
    uint8_t* pRadarBuffer;
    int counter = 0;
    int index = 0;
    int result;

    INT32 reVal;
    uint8_t tempBuff[RING_BUFFER_SIZE];
    int tempsize;
    
    pRadarBuffer = (uint8_t*)&radarBuffer[radarIndex];

    radarSetPower(radarIndex, TRUE);  
    
    if(CmdBuff != NULL)
        radarWrite(radarIndex, CmdBuff, CmdBuffLen);
    /*
    if(radarCmd == 0x00)
        radarWrite(radarIndex, CmdBuff, 9);
    else if(radarCmd == 0x02)
        radarWrite(radarIndex, CmdBuff, 24);
    else if(radarCmd == 0x05)
        radarWrite(radarIndex, CmdBuff, 9);
    */
    
    if(WaitTime >= 10)
    {
        while(counter < (WaitTime / 10))
        {
            vTaskDelay(10/portTICK_RATE_MS);
            reVal = radarRead(radarIndex, pRadarBuffer + index, RADAR_BUFFER_SIZE - index);
            if(reVal > 0)
            {
                index = index + reVal; 

            }   
            counter++;
        }
    }
    else
    {
        while(counter < WaitTime)
        {
            vTaskDelay(1/portTICK_RATE_MS);
            reVal = radarRead(radarIndex, pRadarBuffer + index, RADAR_BUFFER_SIZE - index);
            if(reVal > 0)
            {
                index = index + reVal; 

            }   
            counter++;
        }
    }
    /*
    if(index > 0)
    {
        SaveRingBufFun(radarIndex,pRadarBuffer,index);
    }
    */
    if(index < 1)
    {
        terninalPrintf("[ERROR:RADAR] [%d]    No data, BuffIndex = %d --\r\n", radarIndex+1, index);
        return FALSE;        
    }
    /*
    if(index == 1)
    {
        terninalPrintf("pRadarBuffer = ");
        for(int i=0;i<20;i++)
            terninalPrintf("%02x ",pRadarBuffer[i]);    
        terninalPrintf("\r\n");
    }
    */
    //terninalPrintf("radar%d <= ",radarIndex+1);
    //tempsize = ReadRingBufFun(radarIndex,tempBuff);
    /*
    if(tempsize <= 5)
    {
        terninalPrintf("tempBuff = ");
        for(int i=0;i<tempsize;i++)
            terninalPrintf("%02x ",tempBuff[i]);    
        terninalPrintf("\r\n");
    }
    */
    /*
    for(int i=0;i<tempsize;i++)
        terninalPrintf("%02x ",tempBuff[i]);    
    terninalPrintf("\r\n");
    */
   // vTaskDelay(100/portTICK_RATE_MS);
    /*
    *radarDataLen = tempsize;
    memcpy(radarData,tempBuff,tempsize);
    */
    
    *radarDataLen = index;
    if(CmdBuff != NULL)
        memcpy(radarData,pRadarBuffer,index);
    
    //result = Decode_StateMachine( radarIndex,radarCmd,tempsize, tempBuff, radarData);

    //ShiftReadIndex(radarIndex, tempsize);

    
    return TRUE;
    
}



void newRadarFlush(int radarIndex)
{
    radarRead(radarIndex, (uint8_t*)&radarBuffer[radarIndex], RADAR_BUFFER_SIZE);
    RingFifoPtr[radarIndex]->ReadIndex = RingFifoPtr[radarIndex]->SaveIndex;
    //memset(RingFifoPtr[radarIndex],0x00,sizeof(RingfifoStruct));
}


int RadarCheckFeature(int radarIndex, BOOL* changeFlag, void* para1, void* para2, void* para3)
{
    int returnVal = RADAR_FEATURE_FAIL;  //RADAR_FEATURE_IGNORE;
    #if(1)
    sysprintf("\r\n --- [information:RADAR] RadarCheckFeature!![%d] (CheckDist:(%d, %d, %d), MinPower:%d, StableRange:%d ,StableCounter:%d, VacuumCounter:%d, counter:%d) ENTER  ----\r\n", 
                            radarIndex, checkExceedDist[radarIndex], checkMaxDist[radarIndex], checkMinDist[radarIndex], checkMinPower[radarIndex],
                            checkStableDistRange[radarIndex], checkStableCounter[radarIndex], checkVacuumCounter[radarIndex], counterNumber[radarIndex]);
    #endif
    //int counter = 0;
    int* reDist = para1;
    int* rePower = para2;
    int* occupiedType = para3;
    char* titleStr;
    uint8_t* pTargetPacket;
    uint8_t* pRadarBuffer;// = (uint8_t*)&radarBuffer[radarIndex][radarIndex];
    uint8_t packageTotalSize;
    char debugStr[64];
    int getTimes = 0;
    int totalDist = 0;
    int totalPower = 0;
    
    BOOL timeoutFlag = FALSE;
    
    #if(ENABLE_MAX_MIN_FILTER)
    int maxDist = 0, minDist = 0;
    int maxPower = 0, minPower = 0;
    #endif
    TickType_t tickLocalStart = xTaskGetTickCount();
    
    pTargetPacket = (uint8_t*)&targetPacket[radarIndex];
    pRadarBuffer = (uint8_t*)&radarBuffer[radarIndex];
    
    *changeFlag = FALSE;
    radarBufferIndex[radarIndex] = 0;
    targetPacketIndex[radarIndex] = 0;
    //*reDist = 0;
    *occupiedType = RADAR_VACUUM_TYPE;
    if(radarIndex == 0)
    {
        //titleStr = " [L] ";
        titleStr = " [1] ";
    }
    else
    {
        //titleStr = " [R] ";
        titleStr = " [2] ";
    }
    
    //xSemaphoreTake(xRunningSemaphore, portMAX_DELAY);
    
    //radarFlushRxBuffer(radarIndex);  
#if (ENABLE_BURNIN_TESTER)
    if ((EnabledBurninTestMode() == FALSE) && (powerStatus[radarIndex] == FALSE))
    {
        radarSetPower(radarIndex, TRUE);
        //vTaskDelay(300/portTICK_RATE_MS);
    }
#else
    
    if(powerStatus[radarIndex] == FALSE)
    {
        radarSetPower(radarIndex, TRUE);  
    }        
#endif    
    //vTaskDelay(500/portTICK_RATE_MS);
    
    tickLocalStart = xTaskGetTickCount();
    radarFlushRxBuffer(radarIndex); 
    if(calibrationFlag[radarIndex] == TRUE)
    {
        BuzzerPlay(100, 100, 3, TRUE);
        //calibrationFlag[radarIndex] = FALSE;
        waitTimeoutTick[radarIndex] = RADAR_TIMEOUT_TIME_CALIBRATION;
        radarWrite(radarIndex, calibrationCmd, sizeof(calibrationCmd));
        terninalPrintf("\n\r ---------- >[INFO:RADAR]-%s- WAIT... RADAR_TIMEOUT_TIME = %d --\r\n", titleStr, RADAR_TIMEOUT_TIME*portTICK_RATE_MS);
        /*
        waitTimeoutTick[1] = RADAR_TIMEOUT_TIME_CALIBRATION;
        radarWrite(1, calibrationCmd, sizeof(calibrationCmd));
        terninalPrintf("\n\r ---------- >[INFO:RADAR]- [R] - WAIT... RADAR_TIMEOUT_TIME = %d --\r\n", RADAR_TIMEOUT_TIME*portTICK_RATE_MS);*/
    }
    else if(queryversionFlag[radarIndex] == TRUE)
    {
        waitTimeoutTick[radarIndex] = RADAR_TIMEOUT_TIME_NORMAL;
        radarWrite(radarIndex, versionCmd, sizeof(versionCmd));
    }
    else
    {
        if(readMBtestFunc())
        {
            if(radarIndex == 0)
            {
                //terninalPrintf("radarIndex = 0 \r\n");
                waitTimeoutTick[radarIndex] = (500/portTICK_RATE_MS); //RADAR_TIMEOUT_TIME_NORMAL;
                radarWrite(1, output0Cmd, sizeof(output0Cmd));
            }
            else if(radarIndex == 1)
            {
                //terninalPrintf("radarIndex = 1 \r\n");
                waitTimeoutTick[radarIndex] = (500/portTICK_RATE_MS); //RADAR_TIMEOUT_TIME_NORMAL;
                radarWrite(0, output1Cmd, sizeof(output1Cmd));
            }
        }
        else
        {
            waitTimeoutTick[radarIndex] = RADAR_TIMEOUT_TIME_NORMAL;
            radarWrite(radarIndex, outputOnCmd, sizeof(outputOnCmd));
        }
    }
    
    while(1)
    {
        int headerIndex = 0;
        int leftPacketLen = 0;
        if((xTaskGetTickCount() - tickLocalStart) > RADAR_TIMEOUT_TIME)
        {
            #if (ENABLE_BURNIN_TESTER)
            if (EnabledBurninTestMode() == FALSE)
            #endif
            {
                terninalPrintf("\n\r [ERROR:RADAR]-%s-    TIMEOUT(%d, %d), RADAR_TIMEOUT_TIME = %d --\r\n", titleStr, targetPacketIndex[radarIndex], leftPacketLen, RADAR_TIMEOUT_TIME*portTICK_RATE_MS);
                BuzzerPlay(50, 100, radarIndex+1, TRUE);
                //vTaskDelay(500/portTICK_RATE_MS);
                //BuzzerPlay(300, 50, 1, FALSE);
            }
            timeoutCounter[radarIndex]++;
            
            //RadarSetPowerStatus(radarIndex, FALSE);  

            timeoutFlag = TRUE;
            
            returnVal = RADAR_FEATURE_FAIL;  
            goto RadarCheckFeatureExit;
        }
      
        //sysprintf(" ~~~ Wait RadarCheckFeature read -> packet: index = %d, left = %d ; buffer: index = %d ...\r\n", targetPacketIndex[radarIndex], leftPacketLen, radarBufferIndex[radarIndex]);
        
        //vTaskDelay(200/portTICK_RATE_MS);
        
        int radarBufferLeft;
        
        if(readMBtestFunc())
        {
            if(radarIndex == 1)
            {
                radarBufferLeft = radarRead(1, pRadarBuffer, RADAR_BUFFER_SIZE);
                //terninalPrintf("radarIndex = %d ,  radarBufferLeft = %d ",radarIndex,radarBufferLeft);
            }
            else
            {
                radarBufferLeft = radarRead(0, pRadarBuffer, RADAR_BUFFER_SIZE);
                //terninalPrintf("radarIndex = %d ,  radarBufferLeft = %d ",radarIndex,radarBufferLeft);
            }
            
        }
        else
        {        
            radarBufferLeft = radarRead(radarIndex, pRadarBuffer, RADAR_BUFFER_SIZE);
            //terninalPrintf(" ~~~ RadarCheckFeature read (%d bytes) -> packet: index = %d, left = %d ; buffer: index = %d, left = %d ...\r\n", radarBufferLeft, targetPacketIndex[radarIndex], leftPacketLen, radarBufferIndex[radarIndex], radarBufferLeft);
        }    
            
            
        if(radarBufferLeft == 0)
        {   
            //sysprintf("^");
            vTaskDelay(100/portTICK_RATE_MS);
            continue;
        }
        #if (ENABLE_BURNIN_TESTER)
        if (EnabledBurninTestMode() == FALSE)
        #endif
        {
            if(readMBtestFunc())
            {}
            else
            {
                terninalPrintf("pRadarBuffer%d = ",radarIndex+1);
                for(int i = 0; i<radarBufferLeft; i++)
                 {
                     terninalPrintf("%02x ",*(pRadarBuffer+i));
                 }
                terninalPrintf("\n\r");
            }
        }
        if(targetPacketIndex[radarIndex] == 0)
        {
            headerIndex = getHeader(pRadarBuffer, radarBufferLeft);
            /*terninalPrintf("pRadarBuffer%d = ",radarIndex+1);
            for(int i = 0; i<radarBufferLeft; i++)
             {
                 terninalPrintf("%02x ",*(pRadarBuffer+i));
             }
            terninalPrintf("\n\r");
            terninalPrintf("headerIndex = %08x\r\n", headerIndex);
             */
            if(headerIndex == -1)
            {
                returnVal = RADAR_FEATURE_FAIL;  
                goto RadarCheckFeatureExit;
                
                
                sysprintf("cant find header!!!\r\n");
                //if(radarBufferLeft > 64)
                //{
                //    radarBufferLeft = 64;
                //}
                //logBuffData("RADAR CANT FIND HEADER", (pm_feature*)pRadarBuffer, radarBufferLeft);
                printfBuffData("RADAR CANT FIND HEADER", pRadarBuffer, radarBufferLeft);
                timeoutCounter3[radarIndex]++;
                continue;
            }
            
            //sysprintf("find header!!!\r\n");
            radarBufferLeft = radarBufferLeft - headerIndex;
            radarBufferIndex[radarIndex] = headerIndex;
        }
        else
        {
            radarBufferIndex[radarIndex] = 0;
        }
        if(calibrationFlag[radarIndex] == TRUE)
        {
            packageTotalSize  = sizeof(radarCalibrate);
            pTargetPacket = (uint8_t*)&targetCalibratePacket[radarIndex];
        }
        else if(queryversionFlag[radarIndex] == TRUE)
        {
            packageTotalSize  = sizeof(radarVersion);
            pTargetPacket = (uint8_t*)&targetVersionPacket[radarIndex]; 
        }
        else
        {
            if(readMBtestFunc())
                packageTotalSize  = 10;
            else
                packageTotalSize  = sizeof(radarPacket);   
            pTargetPacket = (uint8_t*)&targetPacket[radarIndex];            
        }
        leftPacketLen = packageTotalSize - targetPacketIndex[radarIndex];
        //leftPacketLen = sizeof(radarPacket) - targetPacketIndex[radarIndex];
        //sysprintf("\r\nRadarCheckFeature read (%d bytes) -> packet: index = %d, left = %d ; buffer: index = %d, left = %d...\r\n", radarBufferLeft, targetPacketIndex[radarIndex], leftPacketLen, radarBufferIndex[radarIndex], radarBufferLeft);
        while(radarBufferLeft >= leftPacketLen)
        {
            uint16_t checkSum, checkSumTarget;
            int currentDist = 0;
            int currentPower = 0;
            //sysprintf(" - RadarCheckFeature move full data -> packet(%d) from buffer(%d) : leftPacketLen = %d...\r\n", targetPacketIndex[radarIndex], radarBufferIndex[radarIndex], leftPacketLen);
            //sysprintf(" - %d(%d):%d(%d)-\r\n", targetPacketIndex[radarIndex], leftPacketLen, radarBufferIndex[radarIndex], radarBufferLeft);
            //if(((xTaskGetTickCount() - tickLocalStart) > RADAR_TIMEOUT_TIME) && (ignoreDataTimes == 0))
            if((xTaskGetTickCount() - tickLocalStart) > RADAR_TIMEOUT_TIME)
            {
                terninalPrintf("\n\r [ERROR:RADAR]-%s-    TIMEOUT 2(%d, %d), RADAR_TIMEOUT_TIME = %d --\r\n", titleStr, radarBufferLeft, leftPacketLen, RADAR_TIMEOUT_TIME*portTICK_RATE_MS);
                //////BuzzerPlay(50, 100, radarIndex+1, TRUE);
                //vTaskDelay(500/portTICK_RATE_MS);
                //BuzzerPlay(500, 50, 1, FALSE);
                timeoutCounter2[radarIndex]++;
                
                //RadarSetPowerStatus(radarIndex, FALSE); 
                
                timeoutFlag = TRUE;
                
                returnVal = RADAR_FEATURE_FAIL;                
                goto RadarCheckFeatureExit;
            }            
            memcpy(pTargetPacket + targetPacketIndex[radarIndex], pRadarBuffer + radarBufferIndex[radarIndex], leftPacketLen); 
            //---------
            
            if(calibrationFlag[radarIndex] == TRUE)
            {
                #if(1)
                sprintf(debugStr, "DUMP RADAR CALLIBRATE DATA(%s_%d):", titleStr, getTimes);
                printfBuffData(debugStr, pTargetPacket, sizeof(radarCalibrate));
                #endif
                terninalPrintf("\r\n\r\n     [!!! INFOMATION !!!] ---> [RADAR CALIBRATION -%s->> < value = %d >  -- [!!! INFOMATION !!!]\r\n\r\n", 
                                    titleStr, targetCalibratePacket[radarIndex].value);   
                if(targetCalibratePacket[radarIndex].value != 0)
                {
                    returnVal = RADAR_FEATURE_FAIL;
                }
                else
                {
                    returnVal = RADAR_CALIBRATION_OK;
                }
                *reDist = targetCalibratePacket[radarIndex].value;
                *rePower = targetCalibratePacket[radarIndex].value;
                goto RadarCheckFeatureExit; 
            }
            else if(queryversionFlag[radarIndex] == TRUE)
            {
               /* terninalPrintf("RADAR%d FW VERSION: VER %d.%d.%d.%s \r\n",radarIndex
                                                                         ,targetVersionPacket[radarIndex].code1
                                                                         ,targetVersionPacket[radarIndex].code2
                                                                         ,targetVersionPacket[radarIndex].code3
                                                              ,(targetVersionPacket[radarIndex].code4 == 1)?"Beta":"");   */
                goto RadarCheckFeatureExit;
            }
            else //if(calibrationFlag[radarIndex] == TRUE)
            {
                if(readMBtestFunc())
                {
                    if (targetPacket[radarIndex].cmdid == 0x30)
                    {
                        if(((radarIndex == 0)&&(targetPacket[radarIndex].material == 0x01)) || ((radarIndex == 1)&&(targetPacket[radarIndex].material == 0x02)) )    
                            returnVal = RADAR_FEATURE_IGNORE;
                        else
                            returnVal = RADAR_FEATURE_FAIL;
                        goto RadarCheckFeatureExit;
                    
                    }
                    else
                        returnVal = RADAR_FEATURE_FAIL;
                    goto RadarCheckFeatureExit;
                }

                #if(0)
                sprintf(debugStr, "DUMP RADAR DATA(%s_%d):", titleStr, getTimes);
                printfBuffData(debugStr, pTargetPacket, sizeof(radarPacket));
                #endif
                if((targetPacket[radarIndex].end1 == RADAR_END) && 
                (targetPacket[radarIndex].end2 == RADAR_END_2))
                {
                    checkSum = 0;
                    for(int i = 2; i<sizeof(radarPacket)-4; i++)
                    {
                        //sysprintf("     [INFO] ---> [CAL_1 : 0x%04x v.s. 0x%02x>  --\r\n", checkSum, pTargetPacket[i]);
                        checkSum = checkSum + pTargetPacket[i];
                        //sysprintf("     [INFO] ---> [CAL_2 : 0x%04x v.s. 0x%02x>  --\r\n", checkSum, pTargetPacket[i]);
                    }
                    checkSumTarget = ((targetPacket[radarIndex].checkSum>>8) & 0xff) | ((targetPacket[radarIndex].checkSum&0xff) << 8);
                    //sysprintf(" [INFO] ---> [COMPARE : 0x%04x v.s. 0x%04x>  --\r\n", checkSum, checkSumTarget);  
                    if(checkSum == checkSumTarget)
                    {     
                    
                        //terninalPrintf("\r\n     [!!! INFO !!!] ---> [RADAR -%s->> < material = %d %s, object = %d %s >  --\r\n", 
                        //            titleStr, targetPacket[radarIndex].material, getMaterialStr(targetPacket[radarIndex].material),
                        //                        targetPacket[radarIndex].object, getObjectStr(targetPacket[radarIndex].object));   
                        counterNumber[radarIndex]++;
                        
                        
                        
                        lidarRecentDist[radarIndex] = targetPacket[radarIndex].lidarRecentSenseDist[0]<<8 | targetPacket[radarIndex].lidarRecentSenseDist[1] ;
                        ridarRecentDist[radarIndex] = targetPacket[radarIndex].radarRecentSenseDist[0]<<8 | targetPacket[radarIndex].radarRecentSenseDist[1] ;
                        NewValueFlag[radarIndex] = TRUE;
                        
                        if(targetPacket[radarIndex].object > 0x0 && targetPacket[radarIndex].object <= 0x3)
                        {
                            //returnVal = RADAR_FEATURE_OCCUPIED;  
                            //*** case 3 *** 
    //                        if(occupiedCounter[radarIndex] >= checkOccupiedCounter[radarIndex])
                                if(occupiedCounter[radarIndex] >= checkStableCounter[radarIndex])
                            {
                                returnVal = RADAR_FEATURE_OCCUPIED;
                                *occupiedType = RADAR_OCCUPIED_TYPE_DISTANCE;
                            }
                            else
                            {
                                occupiedCounter[radarIndex]++;
                                returnVal = RADAR_FEATURE_OCCUPIED_UN_STABLED;
                            }
                            vacuumCounter[radarIndex] = 0;
                        }
                        else if(targetPacket[radarIndex].object == 0x0)
                        {
                            //returnVal = RADAR_FEATURE_VACUUM;  
                            //*** case 5 (vacuum) *** 
                            if( vacuumCounter[radarIndex] >= (checkVacuumCounter[radarIndex]) )
                            {
                                returnVal = RADAR_FEATURE_VACUUM;
    //                            resetCheckStableStatus(radarIndex, checkStableDistData[radarIndex], &checkStableDistIndex[radarIndex]);
    //                            resetCheckStableStatus(radarIndex, checkStablePowerData[radarIndex], &checkStablePowerIndex[radarIndex]);
                            }
                            else
                            {
                                vacuumCounter[radarIndex]++;
                                returnVal = RADAR_FEATURE_VACUUM_UN_STABLED;
                                sysprintf(" [INFO] ---> [VACUUM COUNTER:RADAR -%s->> < %d >  --\r\n", titleStr, vacuumCounter[radarIndex]);                                
                            }
                            occupiedCounter[radarIndex] = 0;
                            //vacuumCounter[radarIndex] = 0;                            
                        }
                        else if(targetPacket[radarIndex].object == 0x4 ||targetPacket[radarIndex].object == 0xF)
                        {
                            returnVal = RADAR_FEATURE_IGNORE;
                        }
                        
                        //---
                        if((returnVal == RADAR_FEATURE_OCCUPIED) || (returnVal == RADAR_FEATURE_VACUUM))
                        {
                            if(prevFeature[radarIndex] == RADAR_FEATURE_INIT)
                            {
                                *changeFlag = TRUE;
                                sprintf(logStr, "-- {INFORMATION}   ~~~~~ [WARNING:RADAR]O%sO[%d]  Change to %s  (init) OO\r\n", titleStr, counterNumber[radarIndex], getTitleStr(returnVal)); 
                                //sprintf(logStr, "-- {INFORMATION}   ~~~~~ [WARNING:RADAR]O%sO[%d] < %d cm, %d > Change to %s (init)  OO\r\n", titleStr, counterNumber[radarIndex], totalDist, totalPower, getTitleStr(returnVal)); 
                                //terninalPrintf(logStr);
                                LoglibPrintf(LOG_TYPE_WARNING, logStr);  
                                if(returnVal == RADAR_FEATURE_OCCUPIED)
                                {
                                    #if (ENABLE_BURNIN_TESTER)
                                    if (EnabledBurninTestMode() == FALSE)
                                    {
                                        BuzzerPlay(1000, 100, 1, FALSE);
                                    }
                                    #endif
                                    //BuzzerPlay(1000, 100, 1, FALSE);
                                    RadarLogCheckStr(radarIndex);
                                    //LoglibPrintf(LOG_TYPE_WARNING, checkStr, FALSE);  
                                }
                                #if (ENABLE_BURNIN_TESTER)
                                else
                                {
                                    if (EnabledBurninTestMode() == FALSE)
                                    {
                                        BuzzerPlay(500, 100, 1, FALSE);
                                    }
                                }
                                #endif
                            }
                            else
                            {
                                if(prevFeature[radarIndex] != returnVal)
                                {
                                    
                                    *changeFlag = TRUE;
                                    sprintf(logStr, "-- {INFORMATION}   ~~~~~ [WARNING:RADAR]O%sO[%d]  Change to %s  OO\r\n", titleStr, counterNumber[radarIndex], getTitleStr(returnVal));
                                    //terninalPrintf(logStr);                                
                                    LoglibPrintf(LOG_TYPE_WARNING, logStr); 
                                    if(returnVal == RADAR_FEATURE_OCCUPIED)
                                    {
                                        BuzzerPlay(1000, 100, 1, FALSE);
                                        RadarLogCheckStr(radarIndex);
                                        //LoglibPrintf(LOG_TYPE_WARNING, checkStr, FALSE);  
                                    }
                                    else
                                    {
                                        BuzzerPlay(500, 100, 1, FALSE);
                                    }
                                                                      
                                }
                            }
                            prevFeature[radarIndex] = returnVal;   
                        }   
                        *reDist = targetPacket[radarIndex].material;
                        *rePower = targetPacket[radarIndex].object;
                    
                        if(targetPacket[radarIndex].lidarCalibrateStatus == 0x0)
                        {
                            lidarCalibrateStatusFlag[radarIndex] = TRUE;                           
                            lidarCalibrateDistValue[radarIndex] = (targetPacket[radarIndex].lidarDistValue[0]<<8) | (targetPacket[radarIndex].lidarDistValue[1]);
                        }
                        else
                        {
                            lidarCalibrateStatusFlag[radarIndex] = FALSE;
                            lidarCalibrateDistValue[radarIndex] = 0; 
                        }
                        
                        
                        
                        
                        
                                           
                        goto RadarCheckFeatureExit;                    
                    }
                    else
                    {
                         terninalPrintf(" [INFO] ---> [CHECKSUM ERR : 0x%02x, 0x%02x>  --\r\n", checkSum, checkSumTarget);  
                    }
                }
                else
                {
                    terninalPrintf(" [INFO] ---> [END ERR : 0x%02x, 0x%02x>  --\r\n", targetPacket[radarIndex].end1, targetPacket[radarIndex].end2);  
                }
            }//if(calibrationFlag[radarIndex] == TRUE)
            
            //----------
            radarBufferLeft = radarBufferLeft - leftPacketLen; //buff 還剩多少沒處理
            radarBufferIndex[radarIndex] = radarBufferIndex[radarIndex] + leftPacketLen; //buff 沒處理的起始INDEX
            
            targetPacketIndex[radarIndex] = 0;
            
            leftPacketLen = packageTotalSize - targetPacketIndex[radarIndex];            
            //leftPacketLen = sizeof(radarPacket) - targetPacketIndex[radarIndex];
            //vTaskDelay(50/portTICK_RATE_MS);  
        }
        if(radarBufferLeft > 0)
        {
            //sysprintf(" - RadarCheckFeature move left data -> packet(%d) from buffer(%d) : len = %d...\r\n", targetPacketIndex[radarIndex], radarBufferIndex[radarIndex], radarBufferLeft);
            //sysprintf(" = %d(%d):%d(%d)=\r\n", targetPacketIndex[radarIndex], leftPacketLen, radarBufferIndex[radarIndex], radarBufferLeft);
            memcpy(pTargetPacket + targetPacketIndex[radarIndex], pRadarBuffer + radarBufferIndex[radarIndex], radarBufferLeft);
            radarBufferIndex[radarIndex] = 0;
            targetPacketIndex[radarIndex] = targetPacketIndex[radarIndex] + radarBufferLeft;
            //vTaskDelay(200/portTICK_RATE_MS); 
        }        
    }  
RadarCheckFeatureExit:  
    //radarWrite(radarIndex, outputOffCmd, sizeof(outputOffCmd));
    
    if(calibrationFlag[radarIndex] == TRUE)
    {
        if(returnVal == RADAR_FEATURE_IGNORE)
        {
            #if (ENABLE_BURNIN_TESTER)
            if (EnabledBurninTestMode() == FALSE)
            #endif
            {
                BuzzerPlay(3000, 100, 1, TRUE);
            }
        }
        else if(returnVal == RADAR_FEATURE_FAIL)
        {
            //BuzzerPlay(2000, 1000, 2, TRUE);
            //BuzzerPlay(80, 80, 3, TRUE);
            
            //BuzzerPlay(80, 8, 3, TRUE);
        }
        else if(returnVal == RADAR_CALIBRATION_OK)
        {
            BuzzerPlay(300, 0, 1, TRUE);
        }
        
        calibrationFlag[radarIndex] = FALSE;
    }
    else if(queryversionFlag[radarIndex] == TRUE)
    {
        queryversionFlag[radarIndex] = FALSE;
        //if(returnVal != RADAR_FEATURE_FAIL)
        terninalPrintf("targetVersionPacket[radarIndex].cmdid = 0x%02x \n",targetVersionPacket[radarIndex].cmdid);
        if((targetVersionPacket[radarIndex].cmdid == 0x10) && (timeoutFlag == FALSE))
        {
            //returnVal = RADAR_QUERY_VERSION_OK;
            return RADAR_QUERY_VERSION_OK;
        }
    }
    
    #if (ENABLE_BURNIN_TESTER)
    if ((EnabledBurninTestMode() == FALSE) && (returnVal == RADAR_FEATURE_FAIL))
    {
        radarSetPower(radarIndex, FALSE); 
    }
    #else
    
    if(returnVal == RADAR_FEATURE_FAIL)
    {
        //radarSetPower(radarIndex, FALSE); 
    }
    #endif
    //xSemaphoreGive(xRunningSemaphore);
    
    
   if(targetPacket[radarIndex].lidarstatus == 0xFF)
   {
        switch(returnVal)
        {
        case RADAR_FEATURE_OCCUPIED:
            returnVal = RADAR_FEATURE_OCCUPIED_LIDAR_FAIL;
            break;
        case RADAR_FEATURE_VACUUM:
            returnVal = RADAR_FEATURE_VACUUM_LIDAR_FAIL;
            break;
        case RADAR_FEATURE_OCCUPIED_UN_STABLED:
            returnVal = RADAR_FEATURE_OCCUPIED_UN_STABLED_LIDAR_FAIL;
            break; 
        case RADAR_FEATURE_VACUUM_UN_STABLED:
            returnVal = RADAR_FEATURE_VACUUM_UN_STABLED_LIDAR_FAIL;
            break; 
        case RADAR_FEATURE_IGNORE:
            returnVal = RADAR_FEATURE_IGNORE_LIDAR_FAIL;
            break; 
        }                              
   }
    
    
    return returnVal;
}


int RadarCalibrate(int radarIndex, BOOL* changeFlag,int* dist1 ,int* dist2,void* para1, void* para2, void* para3)
{
    int returnVal = RADAR_FEATURE_FAIL;  //RADAR_FEATURE_IGNORE;

    //int counter = 0;
    int* reDist = para1;
    int* rePower = para2;
    int* occupiedType = para3;
    char* titleStr[2] = {" [L] "," [R] "};
    uint8_t* pTargetPacket[2];
    //uint8_t  pRadarBuffer[2][30];// = (uint8_t*)&radarBuffer[radarIndex][radarIndex];
    
    uint8_t*  pRadarBuffer1;


    int headerIndex[2];
    int radarBufferLeft[2];
    uint8_t packageTotalSize;
    char debugStr[64];
    int getTimes = 0;
    int totalDist = 0;
    int totalPower = 0;
    #if(ENABLE_MAX_MIN_FILTER)
    int maxDist = 0, minDist = 0;
    int maxPower = 0, minPower = 0;
    #endif
    TickType_t tickLocalStart = xTaskGetTickCount();
    

    *changeFlag = FALSE;
    radarBufferIndex[radarIndex] = 0;
    targetPacketIndex[radarIndex] = 0;
    //*reDist = 0;
    *occupiedType = RADAR_VACUUM_TYPE;
    for(int i=0;i<1;i++)
    {   
        pTargetPacket[i] = (uint8_t*)&targetPacket[i];
        pRadarBuffer1 = (uint8_t*)&radarBuffer[i];
        waitTimeoutTick[i] = RADAR_TIMEOUT_TIME_CALIBRATION;
        
    if(powerStatus[i] == FALSE)
    {
        radarSetPower(i, TRUE);  
    }    
        
        
        radarFlushRxBuffer(i);
        
        //radarWrite(i, calibrationCmd, sizeof(calibrationCmd));
        radarWrite(i, versionCmd, sizeof(versionCmd));

        terninalPrintf("\n\r ---------- >[INFO:RADAR]-%s- WAIT... RADAR_TIMEOUT_TIME = %d --\r\n", titleStr[i], RADAR_TIMEOUT_TIME*portTICK_RATE_MS);
    }


    while(1)
    {   
        int headerIndex = 0;
        int leftPacketLen = 0;
        if((xTaskGetTickCount() - tickLocalStart) > RADAR_TIMEOUT_TIME)
        {
            #if (ENABLE_BURNIN_TESTER)
            if (EnabledBurninTestMode() == FALSE)
            #endif
            {
                terninalPrintf("\n\r [ERROR:RADAR]-%s-    TIMEOUT(%d, %d), RADAR_TIMEOUT_TIME = %d --\r\n", titleStr, targetPacketIndex[radarIndex], leftPacketLen, RADAR_TIMEOUT_TIME*portTICK_RATE_MS);
                BuzzerPlay(50, 100, radarIndex+1, TRUE);
                vTaskDelay(500/portTICK_RATE_MS);
                BuzzerPlay(300, 50, 1, FALSE);
            }
            timeoutCounter[radarIndex]++;
            
            //RadarSetPowerStatus(radarIndex, FALSE);   
            returnVal = RADAR_FEATURE_FAIL;  
            //break;
            goto ErrorExit;
        }
        
        if(radarBufferLeft[0] == 0)
        {   
            radarBufferLeft[0] = radarRead(0, pRadarBuffer1, RADAR_BUFFER_SIZE);
        }
        //if(radarBufferLeft[1] == 0)
        //{
         //   radarBufferLeft[1] = radarRead(1, pRadarBuffer[1], RADAR_BUFFER_SIZE);
       // }
        //terninalPrintf(" ~~~ RadarCheckFeature read (%d bytes) -> packet: index = %d, left = %d ; buffer: index = %d, left = %d ...\r\n", radarBufferLeft, targetPacketIndex[radarIndex], leftPacketLen, radarBufferIndex[radarIndex], radarBufferLeft);
        if(radarBufferLeft[0] == 0 )//|| (radarBufferLeft[1] == 0))
        {   
            //sysprintf("^");
            vTaskDelay(100/portTICK_RATE_MS);
            continue;
        }
        else
        {
            terninalPrintf("finish. radarBufferLeft[0] = %d ,radarBufferLeft[1] = %d\n\r",radarBufferLeft[0],radarBufferLeft[1]);
            //return TRUE;
            break;
        }
        
    }
    

    for(int i = 0; i<1; i++)
    {   
        terninalPrintf("pRadarBuffer[%d] = ",i);
        for(int j = 0; j<radarBufferLeft[i]; j++)
        {   
            terninalPrintf("%02x ",*(pRadarBuffer1+j));
        }
        terninalPrintf("\n\r");
        

        /*
        headerIndex[i] = getHeader(pRadarBuffer[i], radarBufferLeft[i]);
        if(headerIndex[i] == -1)
        {  
            sysprintf("cant find Radar%d header!!!\r\n",i);

            printfBuffData("RADAR CANT FIND HEADER", pRadarBuffer[i], radarBufferLeft[i]);

            continue;
        } */
            
     /*   if((*(pRadarBuffer[i]  )==0x7a) &&
           (*(pRadarBuffer[i]+1)==0xa7) &&              
           (*(pRadarBuffer[i]+2)==0x00) &&
           (*(pRadarBuffer[i]+3)==0x0a) &&
           (*(pRadarBuffer[i]+4)==0x17) &&
           (*(pRadarBuffer[i]+6)==0x00) &&
            //(*(pRadarBuffer[i]+7)==0x21) &&
           (*(pRadarBuffer[i]+8)==0xd3) &&
           (*(pRadarBuffer[i]+9)==0x3d) )
        {
            #if(1)
            sprintf(debugStr, "DUMP RADAR CALLIBRATE DATA(%s_%d):", titleStr[i], getTimes);
            printfBuffData(debugStr, pTargetPacket[i], sizeof(radarCalibrate));
            #endif
            terninalPrintf("\r\n\r\n     [!!! INFOMATION !!!] ---> [RADAR CALIBRATION -%s->> < value = %d >  -- [!!! INFOMATION !!!]\r\n\r\n", 
                                 titleStr[i], *(pRadarBuffer[i]+5));   
            if(*(pRadarBuffer[i]+5) != 0)
            {    
                returnVal = RADAR_FEATURE_FAIL;
            }
        }*/
   
   } 
        
 

ErrorExit:      

return returnVal;

}

void RadarSetPowerStatus(int radarIndex, BOOL flag)
{
    radarSetPower(radarIndex, flag);
    //vTaskDelay(2000/portTICK_RATE_MS);
    //會耗電
    //radarWrite(radarIndex, freqCmd, sizeof(freqCmd));
    //vTaskDelay(3000/portTICK_RATE_MS);
    #if(0)
    radarWrite(radarIndex, powerCmd, sizeof(powerCmd));
    vTaskDelay(3000/portTICK_RATE_MS);
    
    radarWrite(radarIndex, powerSaveCmd, sizeof(powerSaveCmd));
    vTaskDelay(3000/portTICK_RATE_MS);
    #endif    
}
/*
void RadarSetStartCommand(int radarindex, char* command)
{
    
}
*/


void RadarSetPower(BOOL flag)
{
    radarSetPower(0, flag);
    radarSetPower(1, flag);
}

void RadarLogStatus(void)
{
    sprintf(logStr, "-- {RADAR counterNumber: %d, %d, last dist:(%d cm, %d cm), last power:(%d, %d), timeoutCounter:([%d, %d] [%d, %d] [%d, %d]), zeroCounter:[%d, %d], CheckDist:(%d, %d, %d), (%d, %d, %d), MinPower:%d, %d, StableRange:%d, %d ,ConfirmNum:(%d, %d), (%d, %d)} --\r\n", 
                                    counterNumber[0], counterNumber[1], lastDist[0], lastDist[1], lastPower[0], lastPower[1],
                                    timeoutCounter[0], timeoutCounter[1], timeoutCounter2[0], timeoutCounter2[1], timeoutCounter3[0], timeoutCounter3[1], 
                                    zeroCounter[0], zeroCounter[1], 
                                    checkExceedDist[0], checkMaxDist[0], checkMinDist[0], checkExceedDist[1], checkMaxDist[1], checkMinDist[1],
                                    checkMinPower[0], checkMinPower[1],
                                    checkStableDistRange[0], checkStableDistRange[1], 
                                    checkStableCounter[0], checkStableCounter[1], checkLowPowerStableCounter[0], checkLowPowerStableCounter[1]);
    LoglibPrintf(LOG_TYPE_INFO, logStr);
}
void RadarLogCheckStr(int index)
{
    LoglibPrintf(LOG_TYPE_INFO, checkStr[index]);
    
}
void RadarSetLogFlag(int radarindex)
{
    //logStatus[radarindex] = TRUE;
}

void RadarSetDumpRawData(int radarindex, BOOL flag)
{
    //dumpStatus[radarindex] = flag;
}

void RadarSetCheckDistExceedMaxMin(int index, int exceed, int max, int min)
{    
    checkExceedDist[index] = exceed;
    checkMaxDist[index] = max;
    checkMinDist[index] = min;
    sysprintf("  =SET= [RadarSetCheckDistExceedMaxMin[%d]]> (exceed:%d, max:%d, min:%d) --\r\n", index, checkExceedDist[index], checkMaxDist[index], checkMinDist[index]);
}

void RadarSetCheckPowerMin(int index, int min)
{    
    checkMinPower[index] = min;
    sysprintf("  =SET= [RadarSetCheckPowerMin[%d]]> (min:%d) --\r\n", index, checkMinPower[index]);
}
void RadarSetStableCounter(int radarIndex, int checkCounter)
{
    checkStableCounter[radarIndex] = checkCounter;
    sysprintf("  =SET= [RadarSetStableCounter[%d]]> (checkCounter:%d) --\r\n", radarIndex, checkStableCounter[radarIndex]);
}
void RadarSetStableDistRange(int radarIndex, int checkCounter)
{
    checkStableDistRange[radarIndex] = checkCounter;
    sysprintf("  =SET= [RadarSetStableCounter[%d]]> (checkCounter:%d) --\r\n", radarIndex, checkStableDistRange[radarIndex]);
}
void RadarSetVacuumCounter(int radarIndex, int checkCounter)
{
    checkVacuumCounter[radarIndex] = checkCounter;
    sysprintf("  =SET= [RadarSetVacuumCounter[%d]]> (checkCounter:%d) --\r\n", radarIndex, checkVacuumCounter[radarIndex]);
}
void RadarSetLowPowerStableCounter(int radarIndex, int checkCounter)
{
    checkVacuumCounter[radarIndex] = checkCounter;
    sysprintf("  =SET= [RadarSetLowPowerStableCounter[%d]]> (checkCounter:%d) --\r\n", radarIndex, checkLowPowerStableCounter[radarIndex]);
}

void RadarSetStartCalibrate(int radarIndex, BOOL flag)
{
    calibrationFlag[radarIndex] = flag;
    //terninalPrintf("  =SET= [RadarSetStartCalibrate[%d]]> (flag:%d) --\r\n", radarIndex, calibrationFlag[radarIndex]);
}

void RadarSetQueryVersion(int radarIndex, BOOL flag)
{
    queryversionFlag[radarIndex] = flag;
    
}

void ReadRadarVersion(int radarIndex, uint8_t* code1,uint8_t* code2,uint8_t* code3,uint8_t* code4)
{
     //BOOL changeFlag;
     //uint8_t temp1,temp2,temp3,temp4;
     //RadarSetQueryVersion(radarIndex, TRUE);
     //RadarCheckFeature(radarIndex, &changeFlag, NULL, NULL, NULL);
     *code1 = targetVersionPacket[radarIndex].code1;
     *code2 = targetVersionPacket[radarIndex].code2;
     *code3 = targetVersionPacket[radarIndex].code3;
     *code4 = targetVersionPacket[radarIndex].code4;
    
    /* temp1 = targetVersionPacket[radarIndex].code1;
     temp2 = targetVersionPacket[radarIndex].code2;
     temp3 = targetVersionPacket[radarIndex].code3;
     temp4 = targetVersionPacket[radarIndex].code4;
    
     *code1 = temp1;
     *code2 = temp2;
     *code3 = temp3;
     *code4 = temp4; 
      terninalPrintf("RADAR%d FW VERSION: VER %d.%d.%d.%s \r\n"          ,radarIndex            
                                                                         ,temp1
                                                                         ,temp2
                                                                         ,temp3
                                                              ,(temp4 == 1)?"Beta":""); */
    
    
}
int ReadRadarVersionString(int radarIndex, char* radarVerStr)
{
    
     BOOL changeFlag;
     //RadarDrvInit();
     RadarSetQueryVersion(radarIndex, TRUE);
     if(RadarCheckFeature(radarIndex, &changeFlag, NULL, NULL, NULL) == RADAR_QUERY_VERSION_OK )
     {    
         
         sprintf(radarVerStr,"VER %d.%d.%d.%s"
                                                                             ,targetVersionPacket[radarIndex].code1
                                                                             ,targetVersionPacket[radarIndex].code2
                                                                             ,targetVersionPacket[radarIndex].code3
                                                                  ,(targetVersionPacket[radarIndex].code4 == 1)?"Beta":""); 
         return TRUE;
     }
     
     return FALSE;
}

int RadarReadDistValue(int index,int* dist)
{
    if(lidarCalibrateStatusFlag[index] == TRUE)
    {
        *dist = lidarCalibrateDistValue[index];
        lidarCalibrateStatusFlag[index] = FALSE ;
        return TRUE;
    }
        
    return FALSE;
}






static void newprintfBuffData(char* str, uint8_t* data, int len)
{
    int i;
    //terninalPrintf("\r\n %s: len = %d...\r\n   -%02d:-> \r\n", str, len, len);
    
    //terninalPrintf("%s[%d] = ", str, len);
    terninalPrintf("\r");
    //terninalPrintf("\r\r\r\r\r");
    //sysprintf("%s[%d] = ", str, len);
    
    //terninalPrintf("buff = ");
    for(i = 0; i<len; i++)
    { 
        if((data[i]>=0x20)&&(data[i]<=0x7e))
            //terninalPrintf("[%02d]:0x%02x(%c)\r\n", i, (unsigned char)data[i], (unsigned char)data[i]);
        
            //terninalPrintf("%02x ",(unsigned char)data[i]);
            terninalPrintf("\r");
            //terninalPrintf("\r\r\r\r\r");
            //sysprintf("%02x ",(unsigned char)data[i]);
        else
            //terninalPrintf("[%02d]:0x%02x\r\n", i, (unsigned char)data[i]);
        
            //terninalPrintf("%02x ",(unsigned char)data[i]);
            terninalPrintf("\r");
            //terninalPrintf("\r\r\r\r\r");
            //sysprintf("%02x ",(unsigned char)data[i]);
    }
    //terninalPrintf("\r\n");
    
}


static BOOL actionCmd(int radarIndex ,uint8_t* buff, int buffLen, int* receiveLen, int waitTime, uint8_t* cmd, 
                      int cmdLen, uint8_t* cmdAck, int cmdAckRealLen, int cmdAckCompareLen)
{
    int receiveIndex = 0;
    BOOL sendResult = FALSE;
    
    //uint8_t* pRadarBuffer;
    uint8_t* pRadarBuffer  = (uint8_t*)&radarBuffer[0];
    
    receiveIndex = 0;
    *receiveLen = 0;
    newprintfBuffData("actionCmd Send", cmd, cmdLen);
    terninalPrintf("\r");
    //terninalPrintf("\r\n");
    //sysprintf("\r\n");
    
    
    //vTaskDelay(1000/portTICK_RATE_MS);
    //terninalPrintf("cmdLen = %d\r\n", cmdLen);
    if(cmdLen > 0)
    {
        //flushTxRx();
        radarFlushRxBuffer(radarIndex);
        //if(sendData(cmd, cmdLen) == cmdLen) 
        int tempLen = radarWrite(radarIndex, cmd, cmdLen);
        //terninalPrintf("tempLen = %d\r\n", tempLen);        
        if( tempLen == cmdLen)
        {
            sendResult = TRUE;
        }
    }
    else
    {
        sendResult = TRUE;
    }
    //sendResult = TRUE;
    //terninalPrintf("sendResult = %d\r\n", sendResult);
    if(sendResult)
    {
        int counter = waitTime/100;
        while(counter != 0)
        {
            int reval = radarRead(radarIndex, pRadarBuffer, RADAR_BUFFER_SIZE); //readData(receiveDataTmp,  cmdAckRealLen);  //sizeof(receiveDataTmp)
            if(reval > 0)
            {
                if(buffLen < receiveIndex + reval)
                {
                    terninalPrintf("Buffer exceed:%d, %d\r\n", buffLen, receiveIndex + reval);
                    return FALSE;
                }
                //printf("reval=%d\r\n",reval);
                //memcpy(buff + receiveIndex, receiveDataTmp, reval);
                memcpy(buff + receiveIndex, pRadarBuffer, reval);
                receiveIndex = receiveIndex + reval;
                //terninalPrintf("Get data %d\r\n", receiveIndex);
                if(receiveIndex >= cmdAckRealLen)
                {
                    if(cmdAckCompareLen>0)
                    {
                        if(memcmp(buff, cmdAck, cmdAckCompareLen) == 0)
                        {
                            *receiveLen = receiveIndex;
                            //terninalPrintf("Get Ack %d\r\n", *receiveLen);
                            
                            newprintfBuffData("Receieve", pRadarBuffer, receiveIndex);
                            terninalPrintf("\r");
                            //terninalPrintf("\r\n");
                            //sysprintf("\r\n");
                            
                            //vTaskDelay(1000/portTICK_RATE_MS);
                            
                            return TRUE;
                        }
                        else
                        {
                            terninalPrintf("Get Ack cmp error\r\n");
                            //printfBuffData("Get Ack cmp error", receiveDataTmp, receiveIndex);
                            newprintfBuffData("Get Ack cmp error", pRadarBuffer, receiveIndex);
                            
                            terninalPrintf("\r\n");
                            //printfBuffData("Get Ack cmp error", buff, receiveIndex);
                            //printfBuffData("Get Ack cmp error", cmdAck, receiveIndex);
                            return FALSE;
                        }
                    }
                    else
                    {
                        
                        *receiveLen = receiveIndex;
                        terninalPrintf("Get Data %d bytes\r\n", *receiveLen);
                        return TRUE;
                    }
                }
            }  
            //printf("counter = %d\r\n", counter);
            vTaskDelay(100/portTICK_RATE_MS);
            //Sleep(100);
            counter--;
        }
    }  
    terninalPrintf("Get Ack ERROR: receiveDataIndex = %d, cmdAckRealLen = %d\r\n", receiveIndex, cmdAckRealLen);
    //printfBuffData("Get Ack ERROR", receiveDataTmp, receiveIndex);
    newprintfBuffData("Get Ack ERROR", pRadarBuffer, receiveIndex);
    terninalPrintf("\r\n");
    return FALSE;
}









BOOL RadarFirstOTA(int radarIndex, int BufferLen)
{
    int checksum=0;
    
    firstCmd[5] = (BufferLen & 0xFF000000)>>24;
    firstCmd[6] = (BufferLen & 0x00FF0000)>>16;
    firstCmd[7] = (BufferLen & 0x0000FF00)>>8;
    firstCmd[8] = (BufferLen & 0x000000FF);
    
    checksum=0;
    for(int k = 2; k<9; k++)
     {
        checksum += firstCmd[k];
     }
    
    firstCmd[9] = (checksum >> 8);
    firstCmd[10] = (checksum & 0x00FF);
    
    if(actionCmd(radarIndex,receiveData, sizeof(receiveData), &receiveDataIndex, MAX_WAIT_ACK_TIME, firstCmd, 
                    sizeof(firstCmd), firstcmdTakeAck, sizeof(firstcmdTakeAck), sizeof(firstcmdTakeAck)) == FALSE)
    {
        terninalPrintf("FirstOTA Transmission failed.\r\n");
        return FALSE ;
    }
    

    
    return TRUE;
}
BOOL RadarOTA(int radarIndex, uint8_t* ReadBuffer, int BufferIndex, int ReadBufferLen)
{   
    int CmdLen = 0;
    int checksum = 0;
    int TakeAckchecksum = 0;
    
    int ReadBufferIndex = 0 ;
    
    
    if(ReadBufferLen == OTA_DATA_ENVELOPE_SIZE)
    {   
                  
        CmdLen = 11 + OTA_DATA_ENVELOPE_SIZE;
        
        Cmd = malloc((CmdLen)* sizeof(uint8_t));
            
        Cmd[0]=0x7A;
        Cmd[1]=0xA7;
        Cmd[2]=(CmdLen >> 8);
        Cmd[3]=(CmdLen & 0x00FF);
        Cmd[4]=0x06;
        Cmd[5]=(BufferIndex >> 8);
        Cmd[6]=(BufferIndex & 0x00FF);
        
        cmdTakeAck[5] = Cmd[5];
        cmdTakeAck[6] = Cmd[6];
        
        TakeAckchecksum = cmdTakeAck[2] +  cmdTakeAck[3] + cmdTakeAck[4] + cmdTakeAck[5] + cmdTakeAck[6] ;
        
        cmdTakeAck[7] = (TakeAckchecksum >> 8);
        cmdTakeAck[8] = (TakeAckchecksum & 0x00FF);
        
        TakeAckchecksum = 0 ;
        //fread( Readbuffer, OTA_DATA_ENVELOPE_SIZE, 1, pFile );

        for(int i = 0; i<OTA_DATA_ENVELOPE_SIZE; i++)
           { 
 
           
            //terninalPrintf("[%02d]:0x%02x\r\n", i,  ReadBuffer[ReadBufferIndex]);
            
            Cmd[7+i] = ReadBuffer[ReadBufferIndex];
            ReadBufferIndex++;
            
           }

        checksum=0;
        for(int k = 2; k< (7+OTA_DATA_ENVELOPE_SIZE); k++)
         {
            checksum += Cmd[k];
         }
        
        Cmd[CmdLen-4] = (checksum >> 8);
        Cmd[CmdLen-3] = (checksum & 0x00FF);
        
        Cmd[CmdLen-2] = 0xD3;
        Cmd[CmdLen-1] = 0x3D;
        
        
        
        
       if( actionCmd(radarIndex,receiveData, sizeof(receiveData), &receiveDataIndex, MAX_WAIT_ACK_TIME, Cmd, CmdLen, cmdTakeAck, sizeof(cmdTakeAck), sizeof(cmdTakeAck)) == FALSE )
         {
            free(Cmd);
            terninalPrintf("Transmission failed.\r\n");
            //if (j == 3)
            return  FALSE ; 
         }

        free(Cmd);  
       
     }  
     else
     {  
    
        
    
        CmdLen = 11 + ReadBufferLen;  //RemainDataSize;
        
        Cmd = malloc((CmdLen)* sizeof(uint8_t));
            
        Cmd[0]=0x7A;
        Cmd[1]=0xA7;
        Cmd[2]=(CmdLen >> 8);
        Cmd[3]=(CmdLen & 0x00FF);
        Cmd[4]=0x06;
        Cmd[5]=(BufferIndex >> 8);
        Cmd[6]=(BufferIndex & 0x00FF);
    
    
        cmdTakeAck[5] = Cmd[5];
        cmdTakeAck[6] = Cmd[6];
        
        TakeAckchecksum = cmdTakeAck[2] +  cmdTakeAck[3] + cmdTakeAck[4] + cmdTakeAck[5] + cmdTakeAck[6] ;
        
        cmdTakeAck[7] = (TakeAckchecksum >> 8);
        cmdTakeAck[8] = (TakeAckchecksum & 0x00FF);
        
        TakeAckchecksum = 0 ;
        
    
    
        //fread( Readbuffer, RemainDataSize, 1, pFile );

    for(int i = 0; i<ReadBufferLen; i++)
     { 
 
   
        //terninalPrintf("[%02d]:0x%02x\r\n", i,  ReadBuffer[ReadBufferIndex]);
        Cmd[7+i] = ReadBuffer[ReadBufferIndex];
        ReadBufferIndex++;
     }
        
    checksum=0;
    for(int k = 2; k< (7+ReadBufferLen); k++)
     {
        checksum += Cmd[k];
     }
        
    Cmd[CmdLen-4] = (checksum >> 8);
    Cmd[CmdLen-3] = (checksum & 0x00FF);
        
    Cmd[CmdLen-2] = 0xD3;
    Cmd[CmdLen-1] = 0x3D;
    
    if( actionCmd(radarIndex,receiveData, sizeof(receiveData), &receiveDataIndex, MAX_WAIT_ACK_TIME, Cmd, CmdLen, cmdTakeAck, sizeof(cmdTakeAck), sizeof(cmdTakeAck)) ==  FALSE )
      {
         free(Cmd);
         terninalPrintf("Transmission failed.\r\n");
         return  FALSE ;  
      }
        
    free(Cmd);  


    //printf("FW Update Complete.\r\n");
    
  }
    
    
    
    
    return TRUE;
}


BOOL RadarRecentDistValue(int radarIndex, uint16_t* lidarDist,  uint16_t* radarDist)
{
    
    if(NewValueFlag[radarIndex] == TRUE)
    {
        *lidarDist = lidarRecentDist[radarIndex] ;
        *radarDist = ridarRecentDist[radarIndex] ;
        NewValueFlag[radarIndex] = FALSE;
        return TRUE;
    }
    return FALSE;
    
}




/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


/**************************************************************************//**
* @file     leddrv.c
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
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "leddrv.h"
#include "ledcmdlib.h"
#include "hwtester.h"
#if (ENABLE_BURNIN_TESTER)
#include "burnintester.h"
#include "fileagent.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define LED_DRV_UART    UART_8_INTERFACE_INDEX

#define LED_FREQ        0xff //10**10ms = 100sec
//#define LED_PERIOD    30 //30*100ms = 3sec 
#define LED_PERIOD      30 //10*100ms = 3sec 

#define STATUS_LED_FREQ     0 //10**10ms = 0.1sec
#define STATUS_LED_PERIOD   10 //10*100ms = 1sec 

#if(SUPPORT_HK_10_HW)
    //GPH14 wakeup pin
    #define WAKEUP_PORT GPIOH
    #define WAKEUP_PIN  BIT14

    //GPH15 sensor pin
    #define SENSOR_PORT GPIOH
    #define SENSOR_PIN  BIT15
#else
    //GPH15 wakeup pin
    #define WAKEUP_PORT GPIOH
    #define WAKEUP_PIN  BIT15

    //GPH14 sensor pin
    #define SENSOR_PORT GPIOH
    #define SENSOR_PIN  BIT14
#endif

#define RESET_PORT  GPIOF
#define RESET_PIN   BIT9

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;
static SemaphoreHandle_t xSemaphore;
static SemaphoreHandle_t xSemaphoreResponse;
static TickType_t threadWaitTime = portMAX_DELAY;//5000;

//static uint8_t ledMode = LED_MODE_NORMAL_INDEX;
static BOOL factoryTestSendFlag = FALSE;
static BOOL heartSendFlag = FALSE;
static BOOL ledSetFlag = FALSE;
static BOOL paraSetFlag = FALSE;
int whatever;

static BOOL calibrationSetFlag = FALSE;
static BOOL collisionSetFlag = FALSE;
static BOOL collisionCleanFlag = FALSE;

static BOOL initFlag = FALSE;

static BOOL firstinitFlag = TRUE;


static uint8_t bayColorTmp[TOTAL_BAY_LIGHT_NUM] = {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF}, statusColorTmp = LIGHT_COLOR_OFF;
static uint8_t local_bias_degree= 30;
static uint8_t local_strength_X = 30;
static uint8_t local_strength_Y = 30;
static uint8_t local_strength_Z = 30;
static uint8_t local_DeathMin = LED_HEARTBEAT_SECONDS/60; 
static int local_DeathSec = LED_HEARTBEAT_SECONDS%60;

static char status=0;
static BOOL fReadStatus = FALSE;


static short xMEMSVal,yMEMSVal,zMEMSVal;


static BOOL QueryMEMSFlag = FALSE;

static BOOL ledBuzyflag = FALSE;

static BOOL reinitFlag  = TRUE;

static int queryMEMSWaitCounter = 0;

static BOOL shakeflag = FALSE;
/*
static LEDRingfifoStruct RingFifo;
static LEDRingfifoStruct* RingFifoPtr;
*/
//static uint8_t bayColorAllOn[TOTAL_BAY_LIGHT_NUM] = {LIGHT_COLOR_RED, LIGHT_COLOR_GREEN, LIGHT_COLOR_RED, LIGHT_COLOR_GREEN, LIGHT_COLOR_RED, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN}, statusColorAllOn = LIGHT_COLOR_YELLOW;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
#define COUNTER_TIMES  30
/**
 *  @brief  read Command Ack
 *
 *  @param  None
 *
 *  @return if Command Response
 */
 
 
 /*
static int SaveRingBufFun(int bufIndex,uint8_t* Buff,int size){
   
    int i;
    for(i=0;i<size;i++)
        RingFifoPtr->Buff[ (RingFifoPtr->SaveIndex + i) % LED_RING_BUFFER_SIZE ] = Buff[i];
        
    RingFifoPtr->SaveIndex = ( RingFifoPtr->SaveIndex + size ) % LED_RING_BUFFER_SIZE;
    
    return TRUE;
}


static int ReadRingBufFun(int bufIndex,uint8_t* Buff){
    

    int i;
    short size;
    if( RingFifoPtr->SaveIndex == RingFifoPtr->ReadIndex )
        return -1;
    else{
        size = RingFifoPtr->SaveIndex - RingFifoPtr->ReadIndex;
        if(size < 0)
            size += LED_RING_BUFFER_SIZE;
        for(i=0;i<size;i++)
        {
            Buff[i] = RingFifoPtr->Buff[ (RingFifoPtr->ReadIndex + i) % LED_RING_BUFFER_SIZE ];
        }

        return size;
    }
}

static void ShiftReadIndex(int bufIndex, int size)
{
    RingFifoPtr->ReadIndex = ( RingFifoPtr->ReadIndex + size ) % LED_RING_BUFFER_SIZE;
}
 */
 
 
static BOOL readCmdAck()
{
    uint8_t buff[512]; //buff[18];
    int index = 0;
    int headindex = 0;
    int counter = 0;
    INT32 reVal;
    vTaskDelay(10/portTICK_RATE_MS);
    memset(buff, 0x0, sizeof(buff));
    while(counter < COUNTER_TIMES)
    {
        short Command_ID;
        vTaskDelay(10/portTICK_RATE_MS);
        reVal = pUartInterface->readFunc(buff + index, sizeof(buff)-index);
        if(reVal > 0)
        {
            index = index + reVal;
            /* DEBUG LED *///terninalPrintf("readCmdAck<=");
            /* DEBUG LED *///for(int i=0;i<index;i++)
            /* DEBUG LED *///terninalPrintf("%02x ",buff[i]);
            /* DEBUG LED *///terninalPrintf("\n");
            if(RUN_Results((char*)buff, index, &Command_ID, &headindex) == COMMAND_SUCCESSFUL)
            {
                
                /*
                terninalPrintf("buff= ");
                for(int i=0;i<18;i++)
                {
                    terninalPrintf("%02x ",buff[i]);
                }
                terninalPrintf(" ");  
                */
                
                if(0x07 == (uint8_t)Command_ID )
                {
                    if(QueryMEMSFlag)
                    {
                        QueryMEMSFlag = FALSE;
                        xMEMSVal = (buff[6]<<8)  | buff[7];
                        yMEMSVal = (buff[8]<<8)  | buff[9];
                        zMEMSVal = (buff[10]<<8) | buff[11];

                    }
                }
                
                
                
                
                return TRUE;
            }
        }
        sysprintf(".");        
        counter++;
    }
    return FALSE;
}

static BOOL readCmdStatus()
{
    int index = 0;
    int headindex = 0;
    int counter = 0;
    INT32 reVal;
    uint8_t buff[512];
    vTaskDelay(10/portTICK_RATE_MS);
    memset(buff, 0x0, sizeof(buff));
    while(counter < COUNTER_TIMES)
    {
        short Command_ID;
        vTaskDelay(10/portTICK_RATE_MS);
        reVal = pUartInterface->readFunc(buff + index, sizeof(buff)-index);
        if(reVal > 0)
        {
            index = index + reVal;
            /* DEBUG LED *///terninalPrintf("readCmdStatus<=");
            /* DEBUG LED *///for(int i=0;i<index;i++)
            /* DEBUG LED *///   terninalPrintf("%02x ",buff[i]);
            /* DEBUG LED *///terninalPrintf("\n");
            if(RUN_Results((char*)buff, index, &Command_ID, &headindex) == COMMAND_SUCCESSFUL)
            {
                /*
                terninalPrintf("buff= ");
                for(int i=0;i<18;i++)
                {
                    terninalPrintf("%02x ",buff[i]);
                }
                terninalPrintf(" ");  
                */
                if(0x00 == (uint8_t)Command_ID)
                    shakeflag = TRUE;
                if((0x00 == (uint8_t)Command_ID) || (0x09 == (uint8_t)Command_ID) 
                    || (0x05 == (uint8_t)Command_ID) || (0x30 == (uint8_t)Command_ID))
                {   
                    index = 0;
                    counter = 0;
                    
                }
                
                if(0x10 == (uint8_t)Command_ID )
                {
                    //7a a7 (len) 11 10 ff (MEMS_XYZ Temprature 8byte) (status) (check) d3 3d 
                    //status = buff[6];
                    status = buff[14];
                    reVal = TRUE;
                    fReadStatus = TRUE;
                    //terninalPrintf(" t1 ");
                    
                    if(QueryMEMSFlag)
                    {
                        QueryMEMSFlag = FALSE;
                        xMEMSVal = (buff[6]<<8)  | buff[7];
                        yMEMSVal = (buff[8]<<8)  | buff[9];
                        zMEMSVal = (buff[10]<<8) | buff[11];

                    }
                    
                    return TRUE;
                }
            }
        }
        sysprintf(".");        
        counter++;
    }
    return FALSE;
}

static BOOL readCmdVersion(uint8_t* VerCode1,uint8_t* VerCode2,uint8_t* VerCode3,uint8_t* YearCode,
														uint8_t* MonthCode,uint8_t* DayCode,uint8_t* HourCode,uint8_t* MinuteCode)
{
    int index = 0;
    int counter = 0;
    int headindex = 0;
    INT32 reVal;
    uint8_t buff[50]; //buff[17];
    uint8_t cmd[7];
    vTaskDelay(10/portTICK_RATE_MS);
    memset(buff, 0x0, sizeof(buff));
    while(counter < COUNTER_TIMES)
    {
        short Command_ID;
        vTaskDelay(10/portTICK_RATE_MS);
        reVal = pUartInterface->readFunc(buff + index, sizeof(buff)-index);
        if(reVal > 0)
        {
            index = index + reVal;
            /* DEBUG LED *///terninalPrintf("<=");
            /* DEBUG LED *///for(int i=0;i<index;i++)
            /* DEBUG LED *///   terninalPrintf("%02x ",buff[i]);
            /* DEBUG LED *///terninalPrintf("\n");
            if(RUN_Results((char*)buff, index, &Command_ID, &headindex) == COMMAND_SUCCESSFUL)
            {
                if(0x08 == (uint8_t)Command_ID)
                {
                    //7a a7 (len) 11 08 ff (VersionCode1) (VersionCode2) (VersionCode3) (YearCode) (MonthCode) (DayCode)
									  //(HourCode) (MinuteCode) (check) d3 3d 
                    
									  *VerCode1 	= buff[6];
										*VerCode2 	= buff[7];
										*VerCode3 	= buff[8];
										*YearCode   = buff[9];
										*MonthCode  = buff[10];
										*DayCode    = buff[11];
										*HourCode   = buff[12];
										*MinuteCode = buff[13];
									
                    reVal = TRUE;
                    fReadStatus = TRUE;
                    return TRUE;
                }
            }
        }
        sysprintf(".");        
        counter++;
        
        
        
        


       // pUartInterface->writeFunc(cmd, VersionQuery((char*)cmd,sizeof(cmd)));
        
        
        
        
        
        
        
        
        
        
    }
    return FALSE;
}



#if(0)
static void printfBuffData(char* str, uint8_t* data, int len)
{
    int i;
    sysprintf("\r\n %s: len = %d...\r\n   -%02d:-> \r\n", str, len, len);
    
    for(i = 0; i<len; i++)
    { 
        if((data[i]>=0x20)&&(data[i]<=0x7e))
            sysprintf("[%02d]:0x%02x(%c)\r\n", i, (unsigned char)data[i], (unsigned char)data[i]);
        else
            sysprintf("[%02d]:0x%02x\r\n", i, (unsigned char)data[i]);
    }
    sysprintf("\r\n");
}

static void flushBuffer(void)
{
    if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0)
    {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
#endif
static BOOL ledBoardWakeuped(void)
{
    /* DEBUG LED *///terninalPrintf("%d",GPIO_ReadBit(SENSOR_PORT, SENSOR_PIN));
    if(GPIO_ReadBit(SENSOR_PORT, SENSOR_PIN))   //READ pin PortH.14
    {
//        sysprintf("ledBoardWakeuped TRUE\r\n"); 
        return TRUE;
    }
    else
    {
//        sysprintf("ledBoardWakeuped FALSE\r\n"); 
        return FALSE;
    }
}

static void setLedBoardWakeup(BOOL flag)
{
    if(flag)
    {
//        sysprintf("wakeup ledBoard\r\n");
        GPIO_SetBit(WAKEUP_PORT, WAKEUP_PIN);   //wakeup ledBoard PortH.15
    }
    else
    {
//        sysprintf("shutdown ledBoard\r\n");
        GPIO_ClrBit(WAKEUP_PORT, WAKEUP_PIN);   //shutdown ledBoard PortH.15
    }
    vTaskDelay(10/portTICK_RATE_MS); 
}

static BOOL wakeupLedBoard()
{
    /* DEBUG LED *///terninalPrintf("\n -> wakeupLedBoard [");
    //terninalPrintf("wakeupLedBoard.\r\n");
    int counter = 0;
    if(ledBoardWakeuped()) 
    {
        /* DEBUG LED *///terninalPrintf("high] (already wakeup)\r\n");
    }
    else
    {
        /* DEBUG LED *///terninalPrintf("low] (not wakeup)\r\n");
    }
    //Waking up ledBroad
    setLedBoardWakeup(TRUE);
    while(counter < COUNTER_TIMES)
    {
        vTaskDelay(10/portTICK_RATE_MS);
        if(ledBoardWakeuped())
        {
            /* DEBUG LED *///terninalPrintf("\n");
            //terninalPrintf("wakeupLedBoard TRUE.\r\n");
            //reinitFlag = TRUE;
            ledBuzyflag = FALSE;
            return TRUE;
        }
        /* DEBUG LED *///terninalPrintf(".");
        counter++;
    }
    /* DEBUG LED *///terninalPrintf("\r\n");
    setLedBoardWakeup(FALSE);
    //terninalPrintf(" -> wakeupLedBoard error\r\n");
    //terninalPrintf("wakeupLedBoard fail.\r\n");
    
    
    //ClrLedInitFlag();
    
    //vSemaphoreDelete(xSemaphoreResponse);
    //vSemaphoreDelete(xSemaphore);
    /*
    if(reinitFlag)
    {
        reinitFlag = FALSE;
        LedDrvInit(TRUE);
        vTaskDelay(6000/portTICK_RATE_MS);
    }
    */
    
    //LedDrvInit(TRUE);
    
    
    
    GPIO_SetBit(RESET_PORT, RESET_PIN);
    vTaskDelay(500/portTICK_RATE_MS);
    //vTaskDelay(1000/portTICK_RATE_MS);
    GPIO_ClrBit(RESET_PORT, RESET_PIN);
    vTaskDelay(500/portTICK_RATE_MS);
    //vTaskDelay(1000/portTICK_RATE_MS);
    GPIO_SetBit(RESET_PORT, RESET_PIN);
    
    
    //vTaskDelay(6000/portTICK_RATE_MS);
    vTaskDelay(10000/portTICK_RATE_MS);
    
    //setLedBoardWakeup(TRUE);
    
    //ledBuzyflag = TRUE;
    ledBuzyflag = FALSE;
    return FALSE;
}
static BOOL shutdownLedBoard()
{
    /* DEBUG LED *///terninalPrintf("\n -> shutdownLedBoard [");
    int counter = 0;
//    sysprintf(" -> shutdownLedBoard\r\n");
        if(ledBoardWakeuped()) 
    {
        /* DEBUG LED *///terninalPrintf("] (not shutdown)\r\n");
    }
    else
    {
        /* DEBUG LED *///terninalPrintf("] (already shutdown)\r\n");
    }
    setLedBoardWakeup(FALSE);   //shutting down LED
    while(counter < COUNTER_TIMES)
    {
        vTaskDelay(100/portTICK_RATE_MS);
        if(!ledBoardWakeuped())          
        {
            /* DEBUG LED *///terninalPrintf("\n");
            return TRUE;
        }
        //terninalPrintf(".");
        counter++;
    }
    if(readMBtestFunc())
    {}
    else
    {
        terninalPrintf(" -> shutdownLedBoard error\r\n");
    }
    return FALSE;
        
}
static BOOL setLedPara(void)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        shutdownLedBoard();
        return FALSE;
    }
    
    cmdRetuemLen = Bay_light_Command((char*)cmd, sizeof(cmd), LED_FREQ, LED_PERIOD);
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
            sysprintf("setLedPara (bay): OK...\r\n");  
            cmdRetuemLen = State_light_Command((char*)cmd, sizeof(cmd), STATUS_LED_FREQ, STATUS_LED_PERIOD);
            
            if(cmdRetuemLen != COMMAND_ERROR)
            {
                //printfBuffData("@@ setLedPara @@", cmd, cmdRetuemLen);
                pUartInterface->writeFunc(cmd, cmdRetuemLen);
                if(readCmdAck())
                {
                    sysprintf("setLedPara (status): OK...\r\n");  
                    reVal = TRUE;
                }
                else
                {
                    sysprintf("setLedPara (status): ERROR...\r\n");  
                }
            }
            reVal = TRUE;
        }
        else
        {
            sysprintf("setLedPara (bay): ERROR...\r\n");  
        }
    }
    shutdownLedBoard();
    return reVal;
    
}
#if (ENABLE_BURNIN_TESTER)
static char errorMsgBuffer[256];
#endif
static BOOL setColor(uint8_t* bayColor, uint8_t statusColor)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        terninalPrintf("setColor wakeupLedBoard error\r\n");
#if (ENABLE_BURNIN_TESTER)
        if (EnabledBurninTestMode())
        {
            sprintf(errorMsgBuffer, "setColor ==> wakeupLedBoard error !!\r\n");
            AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
        }
#endif
        return FALSE;
    }    
    #if(0)
    cmdRetuemLen = Bay_light_Command((char*)cmd, sizeof(cmd), LED_FREQ, LED_PERIOD);
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
            sysprintf("setLedPara (bay): OK...\r\n");  
            cmdRetuemLen = State_light_Command((char*)cmd, sizeof(cmd), STATUS_LED_FREQ, STATUS_LED_PERIOD);
            if(cmdRetuemLen != COMMAND_ERROR)
            {
                //printfBuffData("@@ setColor @@", cmd, cmdRetuemLen);
                pUartInterface->writeFunc(cmd, cmdRetuemLen);
                if(readCmdAck())
                {
                    sysprintf("setLedPara (status): OK...\r\n");  
                    reVal = TRUE;
                }
                else
                {
                    sysprintf("setLedPara (status): ERROR...\r\n");  
                }
            }
            reVal = TRUE;
        }
        else
        {
            sysprintf("setLedPara (bay): ERROR...\r\n");  
        }
    }
    #endif
    cmdRetuemLen = Light_Color_Command((char*)cmd, sizeof(cmd), bayColor, statusColor);
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
            //sysprintf("setColor: OK...\r\n");  
            //sysprintf("Led Color:[%d, %d, %d, %d, %d, %d, %d, %d: %d]\n\r", bayColor[0], bayColor[1], bayColor[2], bayColor[3], bayColor[4], bayColor[5], bayColor[6], bayColor[7], statusColor);
            reVal = TRUE;
        }
        else
        {
            sysprintf("setColor: ERROR...\r\n");
        }
    }
    shutdownLedBoard();
    return reVal;
}
#warning need check this
#if(0)
static void processLed(void)
{
    switch(ledMode)
    {
        case LED_MODE_NORMAL_INDEX: 
        {
            
        }
            break;
        case LED_MODE_REPLACE_BP_INDEX: 
        {
           
        }
            break;
        case LED_MODE_AUTO_TEST_INDEX: 
        {
           
        }
            break;
    }
    
}
#endif



static BOOL sendHeartbeat(uint8_t DeathMin , int DeathSec)
{
    local_DeathMin = DeathMin;
    local_DeathSec = DeathSec;
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        terninalPrintf("sendHeartbeat wakeupLedBoard FALSE.\r\n");
        return FALSE;
    }    
  
    cmdRetuemLen = HeartBeatTimeSet((char*)cmd, sizeof(cmd), DeathMin, DeathSec);
//    sysprintf("HeartBeatTimeSet: %d[0x%02x, 0x%02x]...\r\n", cmdRetuemLen, cmd[4] , cmd[5]); 
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdStatus())
        {           
            reVal = TRUE;
        }
        else
        {
            sysprintf("LedSendHeartbeat: ERROR...\r\n");  
        }
        
    }
    shutdownLedBoard();
    return reVal;
}
static BOOL memsCalibrationSet(void)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        return FALSE;
    }    
  
    cmdRetuemLen = CalibrationSet((char*)cmd,sizeof(cmd));
//    sysprintf("HeartBeatTimeSet: %d[0x%02x, 0x%02x]...\r\n", cmdRetuemLen, cmd[4] , cmd[5]); 
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
//            sysprintf("LedSendHeartbeat: OK...\r\n");              
            reVal = TRUE;
        }
        else
        {
            sysprintf("LedSendHeartbeat: ERROR...\r\n");  
        }
    }
    shutdownLedBoard();
    return reVal;
}

static BOOL memsCollisionSet(void)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        return FALSE;
    }
    
    cmdRetuemLen = CollisionSet((char*)cmd,sizeof(cmd),local_bias_degree,local_strength_X,local_strength_Y,local_strength_Z);
//    sysprintf("HeartBeatTimeSet: %d[0x%02x, 0x%02x]...\r\n", cmdRetuemLen, cmd[4] , cmd[5]); 
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
//            sysprintf("LedSendHeartbeat: OK...\r\n");
            reVal = TRUE;
        }
        else
        {
            sysprintf("LedSendHeartbeat: ERROR...\r\n");
        }
    }
    shutdownLedBoard();
    return reVal;
}

static BOOL memsCollisionClean(void)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    //uint8_t cmd[7];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        return FALSE;
    }    
  
    cmdRetuemLen = CollisionClean((char*)cmd,sizeof(cmd));
//    sysprintf("HeartBeatTimeSet: %d[0x%02x, 0x%02x]...\r\n", cmdRetuemLen, cmd[4] , cmd[5]); 
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        vTaskDelay(10/portTICK_RATE_MS);
        if(readCmdAck())
        {
//            sysprintf("LedSendHeartbeat: OK...\r\n");              
            reVal = TRUE;
        }
        else
        {
            if(readMBtestFunc())
            {}
            else
            {
                terninalPrintf("memsCollisionClean Error...\r\n");
            }
            sysprintf("LedSendHeartbeat: ERROR...\r\n");  
        }
    }
    shutdownLedBoard();
    return reVal;
}

static BOOL sendFactoryTest()
{
    BOOL reVal = FALSE;
    //uint8_t cmd[16];
    uint8_t cmd[7];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        return FALSE;
    }    
    cmdRetuemLen = FactoryTest((char*)cmd, sizeof(cmd));    /*get cmd*/
    sysprintf("LedSendFactoryTest: %d[0x%02x, 0x%02x]...\r\n", cmdRetuemLen, cmd[4] , cmd[5]); 
    if(cmdRetuemLen != COMMAND_ERROR)
    {
            /* DEBUG LED *///terninalPrintf("cmd=>");
    /* DEBUG LED *///for(int i=0;i<cmdRetuemLen;i++)
    /* DEBUG LED */    //terninalPrintf("%02x ",cmd[i]);
    /* DEBUG LED *///terninalPrintf("\n");
        //vTaskDelay(1000/portTICK_RATE_MS);
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
            sysprintf("LedSendFactoryTest: OK...\r\n");              
            reVal = TRUE;
        }
        else
        {
            sysprintf("LedSendFactoryTest: ERROR...\r\n");  
        }
    }
    shutdownLedBoard();
    return reVal;
}

static void vLedTask( void *pvParameters )
{
    BOOL reTryFlag = FALSE;
    sysprintf("vLedTask Going...\r\n");
    /* DEBUG LED *///terninalPrintf("vLedTask Going...\r\n");
    //先設Low,如果沒接板子因為pull-up使所以會讀到 high 等到板子反應為 low 時為8051進入主程式
    setLedBoardWakeup(FALSE);
    while(ledBoardWakeuped())
    {
        vTaskDelay(10/portTICK_RATE_MS);
    }
    int i=0;
    while(i<200)
    {
        vTaskDelay(10/portTICK_RATE_MS);
        if(!ledBoardWakeuped())
        {
            i++;
        }
        else
        {
            i=0;
        }
    }
    if(sendHeartbeat(LED_HEARTBEAT_SECONDS/60, LED_HEARTBEAT_SECONDS%60)== FALSE)
    {
        terninalPrintf("Task initial sendHeartbeat error\r\n");
        heartSendFlag = TRUE;
    }
    //vTaskDelay(1000/portTICK_RATE_MS);
    
    if(setLedPara() == FALSE)
    {
        terninalPrintf("LED Task initial (setLedPara) error\r\n");
        paraSetFlag=TRUE;
    }
    
    if(setColor(bayColorTmp, statusColorTmp) == FALSE)
    {
        terninalPrintf("LED Task (setColor) error\r\n");
        ledSetFlag=TRUE;
    }
    
    if(memsCollisionSet() == FALSE)
    {
        terninalPrintf("LED Task (setCollision) error\r\n");
        collisionSetFlag=TRUE;
    }
    if(memsCollisionClean() == FALSE)
    {
        terninalPrintf("LED Task(cleanCollision)error\r\n");
        collisionCleanFlag=TRUE;
    }
    
    for(;;)
    {
        BaseType_t reval = xSemaphoreTake(xSemaphore, threadWaitTime);
        reTryFlag = FALSE; 
        if(reval == pdTRUE)
        {
            
        }
        else
        {//heartbeat timeout
            //heartSendFlag = TRUE;
        }
        if(ledSetFlag)
        {
            if(setColor(bayColorTmp, statusColorTmp) == FALSE)
                reTryFlag = TRUE;
            else
                ledSetFlag = FALSE;
        }
        if(paraSetFlag)
        {
            if(setLedPara() == FALSE)
                reTryFlag = TRUE;
            else
                paraSetFlag = FALSE;
        }
        if(heartSendFlag)
        {
            if(sendHeartbeat(LED_HEARTBEAT_SECONDS/60, LED_HEARTBEAT_SECONDS%60)== FALSE) 
            {
                //reTryFlag = TRUE;
                reTryFlag = FALSE;
                heartSendFlag = FALSE;
            }
            else
                heartSendFlag = FALSE;
            //terninalPrintf(" t4 \r\n");
            xSemaphoreGive(xSemaphoreResponse);
        }
        if(factoryTestSendFlag)
        {
            if(sendFactoryTest()== FALSE)
                reTryFlag = TRUE;
            else
                factoryTestSendFlag = FALSE;
        }
        if(calibrationSetFlag)
        {
            if(memsCalibrationSet()== FALSE)
                reTryFlag = TRUE;
            else
                calibrationSetFlag = FALSE;
            xSemaphoreGive(xSemaphoreResponse);
        }
        if(collisionSetFlag)
        {
            if(memsCollisionSet()== FALSE)
                reTryFlag = TRUE;
            else
                collisionSetFlag = FALSE;
        }
        if(collisionCleanFlag)
        {
            if(memsCollisionClean()== FALSE)
                reTryFlag = TRUE;
            else
                collisionCleanFlag = FALSE;
        }
        if(reTryFlag)
        {
            threadWaitTime = 2000/portTICK_RATE_MS;
            //threadWaitTime = 4000/portTICK_RATE_MS;
        }
        else
        {
            threadWaitTime = portMAX_DELAY;
        }
    }
}
static BOOL hwInit(void)
{
    //wakeup pin H15
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(WAKEUP_PORT, WAKEUP_PIN, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_SetBit(WAKEUP_PORT, WAKEUP_PIN);
    
    //sensor pin H14
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xFu<<24)) | (0x0u<<24));
    GPIO_OpenBit(SENSOR_PORT, SENSOR_PIN, DIR_INPUT, PULL_UP);  
    
    //Reset pin F9
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(RESET_PORT, RESET_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(RESET_PORT, RESET_PIN);
    return TRUE;
}

static BOOL swInit(void)
{
    xSemaphore = xSemaphoreCreateBinary(); 
    xSemaphoreResponse = xSemaphoreCreateBinary(); 
    if(xSemaphore == NULL) 
        return FALSE;
    //#warning just temp
    xTaskCreate( vLedTask, "vLedTask", 1024, NULL, LED_THREAD_PROI, NULL );
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/


BOOL ClrLedInitFlag(void)
{
    initFlag = FALSE;
    return TRUE;
}



BOOL LedDrvInit(BOOL testMode)
{
    if(testMode == SPECIAL)
    {
        sendHeartbeat(LED_HEARTBEAT_SECONDS/60, LED_HEARTBEAT_SECONDS%60);
        setLedPara();
        setColor(bayColorTmp, statusColorTmp);
        memsCollisionSet();
        memsCollisionClean();
    }
    if(initFlag)
    {
        return TRUE;
    }
    //terninalPrintf("LedDrvInit!!\n");
    sysprintf("LedDrvInit!!\n");
    pUartInterface = UartGetInterface(LED_DRV_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("LedDrvInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    if(pUartInterface->initFunc(9600) == FALSE)
    {
        sysprintf("LedDrvInit ERROR (pUartInterface->initFunc false)!!\n");
        return FALSE;
    }    
    pUartInterface->setPowerFunc(TRUE);
    if(hwInit() == FALSE)
    {
        sysprintf("LedDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    /*  reset   slave   */
    GPIO_SetBit(RESET_PORT, RESET_PIN);
    vTaskDelay(500/portTICK_RATE_MS);
    //vTaskDelay(1000/portTICK_RATE_MS);
    GPIO_ClrBit(RESET_PORT, RESET_PIN);
    vTaskDelay(500/portTICK_RATE_MS);
    //vTaskDelay(1000/portTICK_RATE_MS);
    GPIO_SetBit(RESET_PORT, RESET_PIN);
    
    
    if(firstinitFlag)
    {
        if(swInit() == FALSE)
        {
            sysprintf("LedDrvInit ERROR (swInit false)!!\n");
            return FALSE;
        }
        firstinitFlag = FALSE;
    }
    //RingFifoPtr = &RingFifo;
    sysprintf("LedDrvInit OK!!\n");
    //terninalPrintf("LedDrvInit OK!!\r\n");
    initFlag = TRUE;
    return TRUE;
}

BOOL LedSetStatus(void)
{
    return setLedPara();
}

BOOL LedSetColor(uint8_t* bayColor, uint8_t statusColor, BOOL checkFlag)
{
    BOOL reVal = FALSE;
    if(checkFlag)
    {
        if((bayColor != NULL) && (memcmp(bayColorTmp, bayColor, sizeof(bayColorTmp)) != 0))
        {
            memcpy(bayColorTmp, bayColor, sizeof(bayColorTmp));
            reVal = TRUE;
        }
        if( (statusColor != LIGHT_COLOR_IGNORE) && (statusColorTmp != statusColor))
        {
            statusColorTmp = statusColor;
            reVal = TRUE;
        }
    }
    if(reVal)
    {
        ledSetFlag = TRUE;
        xSemaphoreGive(xSemaphore);
    }
    return reVal;
}

void LedSetMode(uint8_t mode)
{
    //ledMode = mode;
}
/*
BOOL LedSendHeartbeat(void)
{
    sysprintf("===> LedSendHeartbeat ...\r\n");
    heartSendFlag = TRUE;
    xSemaphoreGive(xSemaphore);
    return TRUE;  
}
*/

BOOL LedSetAliveStatusLightFlush(uint8_t LED_freq,uint8_t LED_period)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        shutdownLedBoard();
        return FALSE;
    }
    
    cmdRetuemLen = Alive_State_light_Command((char*)cmd, sizeof(cmd), LED_freq, LED_period);
    
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
            sysprintf("setLedPara (status): OK...\r\n");  
            reVal = TRUE;
        }
        else
        {
            sysprintf("setLedPara (status): ERROR...\r\n");  
            reVal = FALSE;
        }
    }

    
    shutdownLedBoard();
    return reVal;

}

BOOL LedSetStatusLightFlush(uint8_t LED_freq,uint8_t LED_period)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        shutdownLedBoard();
        return FALSE;
    }
    
    cmdRetuemLen = State_light_Command((char*)cmd, sizeof(cmd), LED_freq, LED_period);
    
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {
            sysprintf("setLedPara (status): OK...\r\n");  
            reVal = TRUE;
        }
        else
        {
            sysprintf("setLedPara (status): ERROR...\r\n");  
            reVal = FALSE;
        }
    }

    
    shutdownLedBoard();
    return reVal;
}


BOOL LedSetBayLightFlush(uint8_t LED_freq,uint8_t LED_period)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        shutdownLedBoard();
        return FALSE;
    }
    
    cmdRetuemLen = Bay_light_Command((char*)cmd, sizeof(cmd), LED_freq, LED_period);
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdAck())
        {

            reVal = TRUE;
        }
        else
        {
            reVal = FALSE;  
        }
    }
    shutdownLedBoard();
    return reVal;
    
    
}
BOOL LedReadShake(uint8_t* ret)
{
    //heartSendFlag = TRUE;
    //xSemaphoreGive(xSemaphore);
    readCmdStatus();
    *ret = status;
    //terninalPrintf("status = %02x\r\n",status);
    //terninalPrintf("*ret = %02x\r\n",*ret);
    if(shakeflag)
    {
        *ret = *ret | 0x01;
        shakeflag = FALSE;
    }
    return TRUE;
}


BOOL LedSendHeartbeat(uint8_t* ret)
{
    BOOL retVal = FALSE;
    sysprintf("===> LedSendHeartbeat ...\r\n");
    heartSendFlag = TRUE;
    fReadStatus = FALSE;
    xSemaphoreGive(xSemaphore);
    BaseType_t reval = xSemaphoreTake(xSemaphoreResponse, 5000/portTICK_RATE_MS);
    //BaseType_t reval = xSemaphoreTake(xSemaphoreResponse, 10000/portTICK_RATE_MS);
    if(reval == pdTRUE)
    {
        //terninalPrintf(" t3 ");
        if(fReadStatus)
        {
            //if(shakeflag)
            //    status = status | 0x01;
            *ret = status;
            //shakeflag = FALSE;
            retVal = TRUE;
            //terninalPrintf(" t2 ");
        }
    }
    else
    {//timeout
        //terninalPrintf("===> LedSendHeartbeat Timeout error\r\n");
        retVal = FALSE;  
    }
    return retVal;  
}

BOOL LedSendFactoryTest()
{
    sysprintf("===> LedSendFactoryTest ...\r\n"); 
//sendFactoryTest();    
    factoryTestSendFlag = TRUE;
    xSemaphoreGive(xSemaphore);   
    BaseType_t reval = xSemaphoreTake(xSemaphoreResponse, 1000/portTICK_RATE_MS);
    //if(reval == pdTRUE)
    //{
        return TRUE;
    //}
    //return FALSE;
    
}

void LedSetPower(BOOL flag)
{
    if(pUartInterface != 0)
    {
        pUartInterface->setPowerFunc(flag);
    }
}

//BOOL MemsCalibrationSet(void)

BOOL MemsCalibrationSet(short* MEMSx,short* MEMSy,short* MEMSz)
{
    sysprintf("===> SendCalibration ...\r\n");

    QueryMEMSFlag = TRUE;
    calibrationSetFlag = TRUE;
    xSemaphoreGive(xSemaphore);
    BaseType_t reval = xSemaphoreTake(xSemaphoreResponse, 1000/portTICK_RATE_MS);
    if(reval == pdTRUE)
    {
        
        *MEMSx = xMEMSVal;
        *MEMSy = yMEMSVal;
        *MEMSz = zMEMSVal;
        
        return TRUE;
    }
    return FALSE;
    
    
    
        
    //if((LedSendHeartbeat(NULL) == TRUE))
    //{
        
    //}
    
    
}

BOOL MemsCollisionSet(uint8_t bias_degree,uint8_t strength_X,uint8_t strength_Y,uint8_t strength_Z)
{
    local_bias_degree= bias_degree;
    local_strength_X = strength_X;
    local_strength_Y = strength_Y;
    local_strength_Z = strength_Z;
    sysprintf("===> SendCollision set...\r\n");
    collisionSetFlag = TRUE;
    xSemaphoreGive(xSemaphore);
    return TRUE;
}

BOOL MemsCollisionClean(void)
{
    sysprintf("===> SendCollision clean...\r\n");   
    collisionCleanFlag = TRUE;
    xSemaphoreGive(xSemaphore);   
    return TRUE;
}


BOOL QueryVersion(uint8_t* VerCode1,uint8_t* VerCode2,uint8_t* VerCode3,uint8_t* YearCode,
									uint8_t* MonthCode,uint8_t* DayCode,uint8_t* HourCode,uint8_t* MinuteCode)
{
	  BOOL reVal = FALSE;
    uint8_t cmd[7];
    uint8_t cmdRetuemLen;
    if(wakeupLedBoard() == FALSE)
    {
        return FALSE;
    }    
  
    cmdRetuemLen = VersionQuery((char*)cmd,sizeof(cmd));

    pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0);
    
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        if(readCmdVersion(VerCode1,VerCode2,VerCode3,YearCode,MonthCode,DayCode,HourCode,MinuteCode))
        {
          
            reVal = TRUE;
            //flushBuffer();
            
            //pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0);
            
        }
        else
        {
            sysprintf("LedVersionQuery: ERROR...\r\n");  
        }
    }
    shutdownLedBoard();
    return reVal;
	
}


/*
BOOL MemsGetStatus(uint8_t* status)
{
    BOOL reVal = FALSE;
    uint8_t cmd[16];
    uint8_t buff[18];
    uint8_t cmdRetuemLen;
    int index = 0;
    int counter = 0;
    if(wakeupLedBoard() == FALSE)
    {
        return FALSE;
    }
    cmdRetuemLen = HeartBeatTimeSet((char*)cmd, sizeof(cmd), local_DeathMin, local_DeathSec);
    if(cmdRetuemLen != COMMAND_ERROR)
    {
        pUartInterface->writeFunc(cmd, cmdRetuemLen);
        INT32 recVal;
        vTaskDelay(10/portTICK_RATE_MS);
        memset(buff, 0x0, sizeof(buff));
        while(counter < COUNTER_TIMES)
        {
            short Command_ID;
            vTaskDelay(10/portTICK_RATE_MS);
            recVal = pUartInterface->readFunc(buff + index, sizeof(buff)-index);
            if(recVal > 0)
            {
                index = index + recVal;
                if(RUN_Results((char*)buff, index, &Command_ID) == COMMAND_SUCCESSFUL)
                {
                    if(0x10 == (uint8_t)Command_ID)
                    {
                        //7a a7 (len) 11 10 ff (status) (check) d3 3d 
                        *status = buff[6];
                        reVal = TRUE;
                        break;
                    }
                }
            }
            counter++;
        }
    }
    shutdownLedBoard();
    return reVal;
}
*/


BOOL QueryMEMSValue(short* MEMSx,short* MEMSy,short* MEMSz)
{
    QueryMEMSFlag = TRUE;
    
    if(!ledBuzyflag)
    {
        queryMEMSWaitCounter = 0;
        ledBuzyflag = TRUE;
        if((LedSendHeartbeat(NULL) == TRUE))
        {
            *MEMSx = xMEMSVal;
            *MEMSy = yMEMSVal;
            *MEMSz = zMEMSVal;    
            return TRUE;
        }
    }
    else
    {
        queryMEMSWaitCounter++;
        if(queryMEMSWaitCounter >=3)
            ledBuzyflag = FALSE;
    }
    return FALSE;
}

void QueryMEMSValueEx(short* MEMSx,short* MEMSy,short* MEMSz)
{
    QueryMEMSFlag = TRUE;
    //if((LedSendHeartbeat(NULL) == TRUE))
    //{
        *MEMSx = xMEMSVal;
        *MEMSy = yMEMSVal;
        *MEMSz = zMEMSVal;        
    //}
  
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/

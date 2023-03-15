/**************************************************************************//**
* @file     creditReaderDrv.c
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
#include "rtc.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "creditReaderDrv.h"
#include "interface.h"
#include "gpio.h"
#include "buzzerdrv.h"
#if (ENABLE_BURNIN_TESTER)
#include "timelib.h"
#include "burnintester.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define CREDIT_READER_UART   UART_10_INTERFACE_INDEX

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;
static uint8_t tempbuff[300];
static uint8_t No3Data;
#if (ENABLE_BURNIN_TESTER)
static uint32_t creditReaderBurninCounter = 0;
static uint32_t creditReaderBurninErrorCounter = 0;
#endif
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static void flushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}

#if (ENABLE_BURNIN_TESTER)
static void vCreditReaderTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    uint8_t sndBuffer[128];
    uint8_t rcvBuffer[128];
    uint32_t sndLength = 0;
    uint32_t rcvLength = 0;
    terninalPrintf("vCreditReaderTestTask Going...\r\n");
    
    while (TRUE)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vCreditReaderTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_CREDIT_READER_INTERVAL)
        {
            //terninalPrintf("vCreditReaderTestTask heartbeat.\r\n");
            //lastTime = currentTime;
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        testLoop = FALSE; 
        CreditCardSetPower(TRUE);
        vTaskDelay(3000/portTICK_RATE_MS);
        CreditCardFlushTxRx(); 
        //Unknown Test
        sndLength = 0;
        sndBuffer[sndLength++] = 0x02;
        sndBuffer[sndLength++] = 0x00;
        sndBuffer[sndLength++] = 0x10;
        sndBuffer[sndLength++] = 0x01;
        sndBuffer[sndLength++] = 0x00;
        sndBuffer[sndLength++] = 0xFF;
        sndBuffer[sndLength++] = 0x03;
        sndBuffer[sndLength++] = 0xA0;
        sndBuffer[sndLength++] = 0x65;
        CreditCardWrite(sndBuffer, sndLength);
        
        vTaskDelay(500/portTICK_RATE_MS);
        rcvLength = CreditCardRead(rcvBuffer, sizeof(rcvBuffer));  
        //BaseType_t reval = pUartInterface->readWaitFunc(portMAX_DELAY);
        //while(1)
        //{
        //    rcvLength = CreditCardRead(rcvBuffer, sizeof(rcvBuffer));
        //    if (rcvLength > 0) {
        //        break;
        //    }
        //}
        //terninalPrintf("vCreditReaderTestTask ==> rcvLength=%d\r\n", rcvLength);
        if (rcvLength == 0) {
            creditReaderBurninErrorCounter++;
        }
        creditReaderBurninCounter++;
        CreditCardSetPower(FALSE);
        lastTime = GetCurrentUTCTime(); 
    }
}
#endif

static BOOL swInit(void)
{
#if (ENABLE_BURNIN_TESTER)
    if (EnabledBurninTestMode())
    {
        xTaskCreate(vCreditReaderTestTask, "vCreditReaderTestTask", 1024*5, NULL, CARD_READER_TEST_THREAD_PROI, NULL);
    }
#endif
    return TRUE;
}

void bv1000_testing(void)
{
    uint8_t sndBuffer[128];
    uint8_t rcvBuffer[128];
    uint32_t sndLength = 0;
    uint32_t rcvLength = 0;
    
    CreditCardSetPower(TRUE);
    vTaskDelay(3000/portTICK_RATE_MS);
    
    CreditCardFlushTxRx();
    sndBuffer[sndLength++] = 0x02;
    sndBuffer[sndLength++] = 0x01;
    sndBuffer[sndLength++] = 0x01;
    sndBuffer[sndLength++] = 0x00;
    sndBuffer[sndLength++] = 0x20;
    sndBuffer[sndLength++] = 0x03;
    sndBuffer[sndLength++] = 0x13;
    sndBuffer[sndLength++] = 0x0D;
    CreditCardWrite(sndBuffer, sndLength);
    vTaskDelay(500/portTICK_RATE_MS);
    rcvLength = CreditCardRead(rcvBuffer, sizeof(rcvBuffer));
    terninalPrintf("bv1000_testing ==> rcvLength=%d", rcvLength);
    if (rcvLength > 0)
    {
        terninalPrintf(", data = ");
        for (int i = 0 ; i < rcvLength ; i++)
        {
            terninalPrintf("%02x, ", rcvBuffer[i]);
        }
    }
    terninalPrintf("\r\n");
    
    CreditCardSetPower(FALSE);
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL CreditReaderDrvInit(BOOL testModeFlag)
{
    sysprintf("CreditReaderDrvInit!!\n");
    pUartInterface = UartGetInterface(CREDIT_READER_UART);
    if (pUartInterface == NULL)
    {
        sysprintf("CreditReaderDrvInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    if (pUartInterface->initFunc(115200) == FALSE)
    {
        sysprintf("CreditReaderDrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if (swInit() == FALSE)
    {
        sysprintf("CreditReaderDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    sysprintf("CreditReaderDrvInit OK!!\n");
    CreditCardSetPower(FALSE);//bv1000_testing();
    return TRUE;
}

void CreditCardSetPower(BOOL flag)
{
    pUartInterface->setRS232PowerFunc(flag);
    pUartInterface->setPowerFunc(flag);
}

void CreditCardFlushTxRx(void)
{
    flushBuffer();
}

INT32 CreditCardRead(PUINT8 pucBuf, UINT32 uLen)
{
    return pUartInterface->readFunc(pucBuf, uLen);
}

INT32 CreditCardWrite(PUINT8 pucBuf, UINT32 uLen)
{
    return pUartInterface->writeFunc(pucBuf, uLen);
}

#if (ENABLE_BURNIN_TESTER)
uint32_t GetCreditReaderBurninTestCounter(void)
{
    return creditReaderBurninCounter;
}

uint32_t GetCreditReaderBurninTestErrorCounter(void)
{
    return creditReaderBurninErrorCounter;
}
#endif

static uint16_t crc16_ccitt(uint8_t *data, UINT length)
{
    uint8_t i;
    uint16_t crc = 0;        // Initial value
    while(length--)
    {
        crc ^= *data++;        // crc ^= *data; data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x8408;        // 0x8408 = reverse 0x1021
            else
                crc = (crc >> 1);
        }
    }
    
    crc = ( (crc>>8) & 0x00FF ) | ( (crc<<8) & 0xFF00 );
    return crc;
}

static void readCreditCard(uint8_t* buff,int buffsize,int* feedbackBuffSize ,BOOL para1)
{
    int index = 0;
    int counter = 0;
    BOOL breakFlag = FALSE;
    BOOL noAPDUmsgFlag = FALSE;
    BOOL quickleaveFlag = FALSE;
    INT32 reVal;
    uint8_t IndexShift = 0;

    CommunicationInterface* CreditCardInterface;
    CreditCardInterface = CommunicationGetInterface(0);
    //CreditCardInterface->initFunc();
    //uartIoctl(10, 25, 0, 0);
    while(counter < 300)
    {
        //short Command_ID;
        vTaskDelay(10/portTICK_RATE_MS);
        counter++;
        reVal = CreditCardInterface->readFunc(buff + index, buffsize-index);
        if(reVal > 0)
        {
            index = index + reVal;
            //terninalPrintf("<=");            
            switch(buff[0]) 
            {
                case 0x01: //SOH
                    noAPDUmsgFlag = TRUE;
                    if(index == 4)
                        quickleaveFlag = TRUE;
                    break;
                //case 0x02: //STX
                //    break;
                case 0x04: //EOT
                    noAPDUmsgFlag = TRUE;
                    if(index == 2)
                        quickleaveFlag = TRUE;
                    break;
                case 0x06: //ACK
                    if(index > 2)
                    {
                        //if((buff[2] == 0x01) || (buff[2] == 0x02))
                        //if(buff[2] == 0x01)
                          //  noAPDUmsgFlag = TRUE;
                        IndexShift = 2;                        
                    }
                    else if((index == 2) && (para1 == TRUE))    
                    {                        
                        noAPDUmsgFlag = TRUE;
                        quickleaveFlag = TRUE;
                    }
                    break;
                case 0x15: //NAK
                    noAPDUmsgFlag = TRUE;
                    if(index == 2)
                        quickleaveFlag = TRUE;
                    break;
                default:
                    break;
            }
            
            
            for(int i=0;i<index;i++)
            {

                //terninalPrintf("%02x ",buff[i]);
                if(noAPDUmsgFlag)
                {

                    if(quickleaveFlag)
                        breakFlag = TRUE;
                }                    
                else if((buff[i] == 0x03) && (buff[i-1] != 0x10) && ((i + IndexShift) > 1) && (i < (index-2) ))
                {
                    /*
                   for(int j=0;j<=i;j++)
                        terninalPrintf("%02x ",buff[j]);
                   terninalPrintf("%02x ",buff[i+1]);
                   terninalPrintf("%02x index = %d",buff[i+2],i+2);
                    
                   terninalPrintf("\n   ");
                   for(int k=0;k<i;k++)
                   {
                       if((buff[k] >= 0x20) && (buff[k] <= 0x7E))
                            terninalPrintf("%c",buff[k]);
                   } 
                    */
                   breakFlag = TRUE; 
                   break;
                }                               
               }
               //terninalPrintf("\n");    
              
               if(breakFlag)
                  break;
        }
    }
    
    *feedbackBuffSize = index;
}

static uint8_t Decoder_S0_Parse(uint8_t ControlChr)
{
    switch(ControlChr) 
    {
        case 0x01: //SOH
            return DECODER_S1_SOH;
            break;
        case 0x02: //STX
            return DECODER_S2_STX;
            break;
        case 0x04: //EOT
            return DECODER_S3_EOT;
            break;
        case 0x06: //ACK
            return DECODER_S21_ACKin;
            break;
        case 0x15: //NAK
            return DECODER_S24_NACKin;
            break;
        default:
            return DECODER_S15_NACKout;
            break;
    }
}
static void DecodeStateMachine(uint8_t* buff,int feedbackBuffSize,
                        uint8_t currentState,uint8_t* nextState,
                        uint8_t* currentSequenceNum,
                        BOOL ACKmessageFlag
                        )
{
    BOOL pollingFlag = TRUE;
    BOOL errorFlag = FALSE;
    BOOL ACKmessageParseFlag = FALSE;
    BOOL STXACKmessage = FALSE;
    uint8_t originalSequenceNum = *currentSequenceNum;
    uint8_t respons[] = {0x06,0x00};
    uint16_t FramesNum;
    uint8_t IndexShift = 0;
    CommunicationInterface* CreditCardInterface;
    CreditCardInterface = CommunicationGetInterface(0);
    while(pollingFlag)
    {
        switch(currentState) 
        {
            case DECODER_S0_Start:
                if(ACKmessageParseFlag)
                    currentState = Decoder_S0_Parse(buff[2]);
                else
                    currentState = Decoder_S0_Parse(buff[0]);
                break;
            case DECODER_S1_SOH:     
                //terninalPrintf("DECODER_S1_SOH , *currentSequenceNum = %d\r\n",*currentSequenceNum);
                if(buff[1+IndexShift] == *currentSequenceNum)
                    currentState = DECODER_S4_SOHsequenceNumber;
                else
                    currentState = DECODER_S15_NACKout;
                break;    
            case DECODER_S2_STX:   
                //terninalPrintf("DECODER_S2_STX\r\n");
                if(buff[1+IndexShift] == *currentSequenceNum)
                    currentState = DECODER_S6_STXsequenceNumber;
                else
                    currentState = DECODER_S15_NACKout;
                break;            
            case DECODER_S3_EOT:
                if(buff[1+IndexShift] == *currentSequenceNum)
                    currentState = DECODER_S16_EOTsequenceNumber;
                else
                    currentState = DECODER_S15_NACKout;
                break;            
                break;            
            case DECODER_S4_SOHsequenceNumber:
                //break;            
            case DECODER_S5_NumberOfFrames1:
                currentState = DECODER_S27_NumberOfFrames2;
                break;            
            case DECODER_S6_STXsequenceNumber:
                //terninalPrintf("DECODER_S6_STXsequenceNumber\r\n");
                currentState = DECODER_S8_IsDLE;
                break;            
            case DECODER_S7_APDU:
                //break;            
            case DECODER_S8_IsDLE:
                //break;            
            case DECODER_S9_IsETX:
                currentState = DECODER_S11_EndOfFrame;
                break;            
            case DECODER_S10_AddToBuffer:
                break;            
            case DECODER_S11_EndOfFrame:
                //break;            
            case DECODER_S12_CRC1:
                //break;            
            case DECODER_S13_CRC2:
                //break;            
            case DECODER_S14_IsCrcOK:
                currentState = DECODER_S17_ACKout;
                break;            
            case DECODER_S15_NACKout:
               // terninalPrintf("DECODER_S15_NACKout\r\n");
                currentState = DECODER_S28_Finished;
                errorFlag = TRUE;
                break;            
            case DECODER_S16_EOTsequenceNumber:
                //break;            
            case DECODER_S17_ACKout:
                respons[1] = *currentSequenceNum;
                //terninalPrintf("currentSequenceNum = %d\r\n", currentSequenceNum);
                CreditCardInterface->writeFunc(respons,sizeof(respons));
                //break;            
            case DECODER_S18_IsComplete:
                currentState = DECODER_S28_Finished;
                break;            
            case DECODER_S19_APDUmessage:
                break;            
            case DECODER_S20_IsSTXorSOH:
                break;            
            case DECODER_S21_ACKin:
                currentState = DECODER_S22_ACKsequenceNumber;
               // if(buff[1+IndexShift] == currentSequenceNum)
               //     currentState = DECODER_S22_ACKsequenceNumber;
               // else
               //     currentState = DECODER_S15_NACKout;
                break;            
            case DECODER_S22_ACKsequenceNumber:
                if(ACKmessageFlag == TRUE)
                    currentState = DECODER_S23_ACKmessage;
                else
                    currentState = DECODER_S28_Finished;
                break;            
            case DECODER_S23_ACKmessage:
                if( (buff[2] == 0x01) || (buff[2] == 0x02) )
                {
                    if(buff[2] == 0x02)
                        STXACKmessage = TRUE;
                    ACKmessageParseFlag = TRUE;
                    IndexShift = 2;
                    currentState = DECODER_S0_Start;
                }
                else
                    currentState = DECODER_S28_Finished;
                //DecodeStateMachine(buff,feedbackBuffSize,DECODER_S0_Start,&nextState,0,FALSE);
                break;            
            case DECODER_S24_NACKin:
                break;            
            case DECODER_S25_NACKsequenceNumber:
                break;            
            case DECODER_S26_NACKmessage:
                break;            
            case DECODER_S27_NumberOfFrames2:
                FramesNum = ( buff[3+IndexShift]<<8 ) | (buff[2+IndexShift]) ;
                currentState = DECODER_S17_ACKout;
                break;            
            case DECODER_S28_Finished: 
                //terninalPrintf("DECODER_S28_Finished , *currentSequenceNum = %d\r\n",*currentSequenceNum);
                if(( *currentSequenceNum >= (FramesNum + originalSequenceNum + 1)) || 
                    (STXACKmessage == TRUE)||
                    (errorFlag == TRUE) )
                {
                    (*currentSequenceNum)++;
                    pollingFlag = FALSE;
                }
                else
                {

                    //if( *currentSequenceNum >= ( FramesNum + originalSequenceNum ))
                     //   readCreditCard(buff,300,&feedbackBuffSize, TRUE);
                   // else
                        readCreditCard(buff,300,&feedbackBuffSize, FALSE);

                    //*currentSequenceNum = *currentSequenceNum + 1;
                    (*currentSequenceNum)++;
                    currentState = DECODER_S0_Start;
                    ACKmessageParseFlag = FALSE;
                    IndexShift = 0;
                }
                break;        
            default:
                pollingFlag = FALSE;
                break;
        }
    }
    
    
}


void CADReadCardinit(uint8_t* pucBuf)
{
    uint8_t APDUBuf[] = {0x20,0x07,0x00,0x1C};
    
    memset(tempbuff,0x00,sizeof(tempbuff));
    //uint8_t pucBuf[300];
    uint8_t buff[300]; //buff[30];

    int feedbackBuffSize;
    uint8_t currentState = 0;
    uint8_t nextState;
    uint8_t currentSequenceNum = 0;
    uint16_t tempcrc;
    INT32 reVal;
    BOOL breakFlag = FALSE;


    
    

    pUartInterface = UartGetInterface(CREDIT_READER_UART);
    CommunicationInterface* CreditCardInterface;
    CreditCardInterface = CommunicationGetInterface(0);
    CreditCardInterface->initFunc();
    //CreditCardSetPower(TRUE);
    
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOB, BIT4, DIR_OUTPUT, NO_PULL_UP);
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOB, BIT3, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOA, BIT14, DIR_OUTPUT, NO_PULL_UP);

    //GPIO_ClrBit(GPIOG, BIT6);
    //GPIO_SetBit(GPIOG, BIT6);
    GPIO_ClrBit(GPIOB, BIT3);
    GPIO_SetBit(GPIOB, BIT4);
    GPIO_SetBit(GPIOA, BIT14);

    //GPIO_ClrBit(GPIOB, BIT3);
    vTaskDelay(3000/portTICK_RATE_MS);
    //vTaskDelay(5000/portTICK_RATE_MS);

    uartIoctl(10, 25, 0, 0);  //UART10FlushBuffer , UARTA=10 , UART_IOC_FLUSH_RX_BUFFER=25 

    
    pucBuf[0] = 0x02;
    pucBuf[1] = 0x00;
    memcpy(pucBuf+2,APDUBuf,sizeof(APDUBuf));
    pucBuf[sizeof(APDUBuf)+2] = 0x03;
    
    // crc compute from Sequence Number to ETX(0x03) 
    // see PknPaymentKitInterface_PhysicalAndDataLinkLayers.pdf p.9
    tempcrc = crc16_ccitt(pucBuf+1,sizeof(APDUBuf)+2);  
    
    pucBuf[sizeof(APDUBuf)+3] = tempcrc >>8;
    pucBuf[sizeof(APDUBuf)+4] = tempcrc & 0x00FF;
    
}

BOOL CADReadCard(uint8_t* pucBuf,uint8_t* currentSequenceNum)
{
    /*
    uint8_t APDUBuf[] = {0x20,0x07,0x00,0x1C};
    
 
    uint8_t pucBuf[300];
    uint8_t buff[300]; //buff[30];

    int feedbackBuffSize;
    uint8_t currentState = 0;
    uint8_t nextState;
    uint8_t currentSequenceNum = 0;
    uint16_t tempcrc;
    INT32 reVal;
    BOOL breakFlag = FALSE;


    
    

    
    CommunicationInterface* CreditCardInterface;
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOB, BIT4, DIR_OUTPUT, NO_PULL_UP);
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOB, BIT3, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOA, BIT14, DIR_OUTPUT, NO_PULL_UP);


    GPIO_ClrBit(GPIOB, BIT3);
    GPIO_SetBit(GPIOB, BIT4);
    GPIO_SetBit(GPIOA, BIT14);
    //GPIO_ClrBit(GPIOB, BIT3);
    vTaskDelay(3000/portTICK_RATE_MS);
    //vTaskDelay(5000/portTICK_RATE_MS);
    CreditCardInterface = CommunicationGetInterface(0);
    CreditCardInterface->initFunc();
    uartIoctl(10, 25, 0, 0);  //UART10FlushBuffer , UARTA=10 , UART_IOC_FLUSH_RX_BUFFER=25 

    
    pucBuf[0] = 0x02;
    pucBuf[1] = 0x00;
    memcpy(pucBuf+2,APDUBuf,sizeof(APDUBuf));
    pucBuf[sizeof(APDUBuf)+2] = 0x03;
    
    // crc compute from Sequence Number to ETX(0x03) 
    // see PknPaymentKitInterface_PhysicalAndDataLinkLayers.pdf p.9
    tempcrc = crc16_ccitt(pucBuf+1,sizeof(APDUBuf)+2);  
    
    pucBuf[sizeof(APDUBuf)+3] = tempcrc >>8;
    pucBuf[sizeof(APDUBuf)+4] = tempcrc & 0x00FF;
        
    vTaskDelay(2000/portTICK_RATE_MS);
    */
    uint8_t APDUBuf[] = {0x20,0x07,0x00,0x1C};
        //uint8_t pucBuf[300];
    //uint8_t buff[300]; //buff[30];

    No3Data = tempbuff[3];
    /*
    terninalPrintf("before read = ");
    for(int i=0;i<30;i++)
        terninalPrintf("%02x ",tempbuff[i]);
    terninalPrintf("\r\n");
    */
    int feedbackBuffSize;
    uint8_t currentState = 0;
    uint8_t nextState;
    //uint8_t currentSequenceNum = 0;
    uint16_t tempcrc;
    INT32 reVal;
    BOOL breakFlag = FALSE;
    CommunicationInterface* CreditCardInterface;
    CreditCardInterface = CommunicationGetInterface(0);
    //while(1)
    //{
        CreditCardInterface->writeFunc(pucBuf,sizeof(APDUBuf)+5);
        readCreditCard(tempbuff,sizeof(tempbuff),&feedbackBuffSize, FALSE);
        DecodeStateMachine(tempbuff,feedbackBuffSize,DECODER_S0_Start,&nextState,currentSequenceNum,TRUE);
        
        //if(userResponse()=='q')
        //    break;
        
       // if(buff[7] == 0x05)
       // {
        //    BuzzerPlay(200, 500, 1, TRUE);
        //    break;
        //}
    //}
    
   // GPIO_SetBit(GPIOB, BIT3);
   // GPIO_ClrBit(GPIOB, BIT4);
   // GPIO_ClrBit(GPIOA, BIT14);
    /*
    terninalPrintf("after read =  ");
    for(int i=0;i<30;i++)
        terninalPrintf("%02x ",tempbuff[i]);
    terninalPrintf("\r\n");
    */

    
    if(tempbuff[7] == 0x05)
        return TRUE;
    else
        return FALSE;
}

BOOL DetectCADConnect(void)
{
    if(No3Data == tempbuff[3])
        return FALSE;
    else
        return TRUE;    
}

void CADReadCardpoweron(void)
{
    GPIO_ClrBit(GPIOB, BIT3);
    GPIO_SetBit(GPIOB, BIT4);
    GPIO_SetBit(GPIOA, BIT14);   
}


void CADReadCardpoweroff(void)
{
   // GPIO_SetBit(GPIOG, BIT6);
    //GPIO_ClrBit(GPIOG, BIT6);
    GPIO_SetBit(GPIOB, BIT3);
    GPIO_ClrBit(GPIOB, BIT4);
    GPIO_ClrBit(GPIOA, BIT14);    
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


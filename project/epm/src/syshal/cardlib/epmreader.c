/**************************************************************************//**
* @file     epmreader.c
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
#ifdef _PC_ENV_
    #include "misc.h"
    #include "interface.h"
    #include "halinterface.h"
    #include "epmreader.h"
    #include "blkcommon.h"
	#include "ipasslib.h"
    #include "ecclib.h"
    
    #define sysprintf       miscPrintf//printf
    #define pvPortMalloc    malloc
    #define vPortFree       free
    #define EPM_READER_UART				UART_INTERFACE_INDEX
#else
    #include "nuc970.h"
    #include "sys.h"
    #include "rtc.h"
    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "fepconfig.h"
    #include "epmreader.h"
    #include "blkcommon.h"
    #include "ipasslib.h"
    #include "ecclib.h"
    #include "interface.h"
    #include "halinterface.h"    
    #include "cardlogcommon.h"
    #include "fileagent.h"
    #include "meterdata.h"
    #include "sflashrecord.h"
    #include "dataprocesslib.h"
    #include "loglib.h"
    #define EPM_READER_UART             UART_2_INTERFACE_INDEX
    extern UINT32 sysDOS_Time_To_UTC(DateTime_T ltime);
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UART_BARDRATE				115200 //ts2000

#define CMD_UART_RX_BUFF_LEN		512 //need check uart.c UARTRXBUFSIZE[UART_NUM] 

#define ENABLE_SHOW_RETURN_DATA     1
#define ENABLE_TX_PRINT				0//0
//#define ENABLE_TX_PRINT				1//0
#define ENABLE_RX_PRINT				0//
#define ENABLE_RX_TIME_PRINT		0

#define CARD_INIT_TIMEOUT_TIME          (25000/portTICK_RATE_MS)
#define CARD_INIT_TIMEOUT_INTERVAL      (1000/portTICK_RATE_MS) 

#define SUPPORT_ECC_CARD_READER     0//1

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint8_t	readVNCmd[] =    {0xEA, 0x01, 0x00, 0x00, 0x01, 0x00, 0x90, 0x00};//api ver
//static uint8_t read_ivn[] =   {0xEA, 0x05, 0x00, 0x00, 0x01, 0x00, 0x90, 0x00};//ipass library ver
static uint8_t	readCNCmd[] =    {0xEA, 0x02, 0x01, 0x00, 0x01, 0x00, 0x90, 0x00};
//static uint8_t	readTimeCmd[] =  {0xEA, 0x01, 0x02, 0x00, 0x01, 0x00, 0x90, 0x00};
//static uint8_t	readIdCmd[] =    {0xEA, 0x04, 0x01, 0x00, 0x06, 0x84, 0x0A, 0x00, 0x00, 0x04, 0x8A, 0x90, 0x00 };//{0xEA,0x01,0x22,0x00,0x01,0x00,0x90,0x00};

static UartInterface* pUartInterface = NULL;

static uint8_t	rxBuffer[500];

static uint8_t	cnData[8]= {0};
static uint8_t	cnLen = 0;

static uint8_t	dataTime[4]= {0};
static char		dataStr[9]= {0};
static char		timeStr[9]= {0};

static uint8_t	machineNo[4]= {0, 1, 2, 3};

static char		verStr[64]= {0};
static int	verStrLen = 0;

static uint8_t readerId[7] = {0};

static uint16_t currentTargetDeduct; 

static uint8_t rxTemp[CMD_UART_RX_BUFF_LEN];

static DateTime_T unixTime;

static uint32_t utcTime;

static TickType_t powerUpTick = 0;
static BOOL needInitReaderFlag = TRUE;
static tsreaderDepositResultCallback  ptsreaderDepositResultCallback;

static uint8_t currentCardType = CARD_TYPE_ID_NONE;
static time_t epmCurrentUTCTime = 0;

//static EpmReaderCtrl   epmReaderCtrl[EPM_READER_CTRL_NUMBER];

#if(SUPPORT_ECC_CARD_READER)
static BOOL eccNeedRestFlag = TRUE;
#endif

static BOOL disableReaderFlag = FALSE;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
/*
static void resetEpmReaderCtrl(void)
{
    int i;
    for(i = 0; i<EPM_READER_CTRL_NUMBER; i++)
    {
        epmReaderCtrl[i].onOffFlag = FALSE;
        EPMReaderSetPower(i, epmReaderCtrl[i].onOffFlag);
    }
}

static BOOL setEpmReaderCtrl(uint8_t id, BOOL flag)
{
    int i;
    BOOL reVal = FALSE;
    epmReaderCtrl[id].onOffFlag = flag;
    for(i = 0; i<EPM_READER_CTRL_NUMBER; i++)
    {
        if(epmReaderCtrl[i].onOffFlag == TRUE)
        {
            reVal = TRUE; 
            break;
        }
    }
    return reVal;
}
*/
#if(0)
static uint16_t readTime(void)
{
    /*
    Run Card readTime()
    Data[0] we read is 0xEA!
    Data[1] we read is 0x 1!
    Data[2] we read is 0x 2!
    Data[3] we read is 0x 0!
    Data[4] we read is 0x 7!
    Data[5] we read is 0x 2!
    Data[6] we read is 0x10!
    Data[7] we read is 0x 8!
    Data[8] we read is 0x16!
    Data[9] we read is 0x11!
    Data[10] we read is 0x 8!
    Data[11] we read is 0x1F!
    Data[12] we read is 0x90!
    Data[13] we read is 0x 0!

    Ready to Read sec 2 data!
    Len we read is 14!,the data len we get is 14
    The Read is 14! len is 14!

    read data over!
    dataTime: 0x10(8) 0x16(17) 0x1(75) 0xA918(1)
    
    0xEA 0x01 0x02 0x00 0x07 X1 X2 X3 X4 X5 X6 X7 0x90 0x00
    X1:Day of Week
    X2:Year
    X3:Month
    X4:Day
    X5:Hour
    X6:Minute
    X7:Second

    */
    uint16_t returnCode;
    uint16_t returnInfo;
	uint8_t* receiveData;
	uint16_t receiveDataLen;	
    //sysprintf("\r\nRun Card readTime()\n");
    
    EPMReaderFlushBuffer();
    
    int nret = EPMReaderSendCmd(readTimeCmd,sizeof(readTimeCmd));
    if(nret != sizeof(readTimeCmd))
    {
        sysprintf("readTime() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(readTimeCmd));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        //vTaskDelay(10/portTICK_RATE_MS); 
        int count=EPMReaderReceiveCmd(50, &receiveData, &receiveDataLen);
        
        if(count == 0)
        {
            sysprintf("readTime() receiveReaderCmd error\n"); 
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;
        }
        else
        {
           returnInfo = EPMReaderParserMessage(CARD_MESSAGE_TYPE_TIME, receiveData, receiveDataLen, &returnCode);
        }
    }

    return returnInfo;    

}
#endif
static uint16_t readCN(uint16_t* returnCode)
{ 
    uint16_t returnInfo;
	uint8_t* receiveData;
	uint16_t receiveDataLen;
    //sysprintf("\r\nRun Card readCN(), errorTimes = %d\n", errorTimes);
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
	currentCardType = CARD_TYPE_ID_NONE;
    EPMReaderFlushBuffer();    
    
    int nret = EPMReaderSendCmd(readCNCmd,sizeof(readCNCmd));
    if(nret != sizeof(readCNCmd))
    {
        sysprintf("readCN() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(readCNCmd));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        //vTaskDelay(10/portTICK_RATE_MS);     
        int count=EPMReaderReceiveCmd(300, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char! @ readCN\n",count);
        if(count == 0)
        {     
            sysprintf("readCN() receiveReaderCmd error\n"); 
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;            
        }
        else
        {
            returnInfo = EPMReaderParserMessage(CARD_MESSAGE_TYPE_CN, receiveData, receiveDataLen, returnCode);            
        }
    }    
    return returnInfo;

}


static uint16_t readVN(void)
{
    uint16_t returnCode;
    uint16_t returnInfo;
	uint8_t* receiveData;
	uint16_t receiveDataLen;
    //sysprintf("\r\nRun Card readVN()\n");
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd(readVNCmd,sizeof(readVNCmd));
#if(1)
    if(nret != sizeof(readVNCmd))
    {
        sysprintf("readVN() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(readVNCmd));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(1000, &receiveData, &receiveDataLen);
        if(count == 0)
        {        
            sysprintf("readVN() receiveReaderCmd error\n"); 
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;        
        }
        else
        {
            if(disableReaderFlag)
            {
                sysprintf(" !!!!!!!readVN() CARD_MESSAGE_RETURN_TIMEOUT JUST FOR DEBUG !!!!!!!\n"); 
                returnInfo = CARD_MESSAGE_RETURN_TIMEOUT; 
            }
            else
            {
                returnInfo = EPMReaderParserMessage(CARD_MESSAGE_TYPE_API_VER_NO, receiveData, receiveDataLen, &returnCode);  
            }                
        }
    }
#endif
    return returnInfo;
}
/*
static uint16_t Read_ReaderId(void)
{
    uint16_t returnCode;
    uint16_t returnInfo;
	uint8_t* receiveData;
	uint16_t receiveDataLen;
    //sysprintf("\r\nRun Card Read_ReaderId()\n");
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd(readIdCmd,sizeof(readIdCmd));
    if(nret != sizeof(readIdCmd))
    {
        sysprintf("Read_ReaderId() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(readVNCmd));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(200, &receiveData, &receiveDataLen);
        if(count == 0)
        {        
            sysprintf("Read_ReaderId() receiveReaderCmd error\n"); 
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;    
        }
        else
        {
           returnInfo = EPMReaderParserMessage(CARD_MESSAGE_TYPE_READER_ID, receiveData, receiveDataLen, &returnCode);
        }
    }
    return returnInfo;
}
*/


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL EPMReaderInit(void)
{
    sysprintf("EPMReaderInit!!\n");
    CardLogUint32ToString((char*)"epmid", GetMeterData()->epmid, machineNo, 4);
    sysprintf("machineNo : 0x%02x(%c) 0x%02x(%c) 0x%02x(%c) 0x%02x(%c)\r\n", machineNo[3], machineNo[3], machineNo[2], machineNo[2], machineNo[1], machineNo[1], machineNo[0], machineNo[0]);
   
    pUartInterface = UartGetInterface(EPM_READER_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("EPMReaderInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    if(pUartInterface->initFunc(UART_BARDRATE) == FALSE)
    {
        sysprintf("EPMReaderInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    //resetEpmReaderCtrl();
#ifdef _PC_ENV_
    BlkCommonInit();
    ECCLibInit();
#endif
    sysprintf("EPMReaderInit OK!!\n");
    return TRUE;
}
BOOL EPMReaderSetPower(uint8_t id, BOOL flag)
{
    if(flag)
    {
        powerUpTick = xTaskGetTickCount();
    }
    else
    {
        powerUpTick = 0; 
    }
    //flag = setEpmReaderCtrl(id, flag);
    pUartInterface->setRS232PowerFunc(flag);
    pUartInterface->setPowerFunc(flag);
    
    if(flag == FALSE)
    {
        needInitReaderFlag = TRUE;
        disableReaderFlag = FALSE;
    }
    #if(SUPPORT_ECC_CARD_READER)
    if(flag == FALSE)
    {
        eccNeedRestFlag = TRUE;
    }
    sysprintf(" == INFORMATION == !!! EPMReaderSetPower flag = %d, eccNeedRestFlag = %d, needInitReaderFlag = %d !!!...\r\n", flag, eccNeedRestFlag, needInitReaderFlag);
    #endif

    return flag;
}

uint8_t EPMReaderCheckReader(void)
{
    if(needInitReaderFlag == FALSE)
    {
        //sysprintf(" !!! EPMReaderCheckReader IGNORE  !!!...\r\n");
        
        LoglibPrintf(LOG_TYPE_INFO, " !!! EPMReaderCheckReader IGNORE  !!!...\r\n");
        
        return  TSREADER_CHECK_READER_OK;
    }
    else
    {
        uint8_t reVal = TSREADER_CHECK_READER_OK;
        LoglibPrintf(LOG_TYPE_INFO, " !!! EPMReaderCheckReader enter  !!!...\r\n");
    
        while(readVN() != CARD_MESSAGE_RETURN_SUCCESS)
        {
            sysprintf("readVN retry(%d:%d)...\r\n", xTaskGetTickCount() - powerUpTick, CARD_INIT_TIMEOUT_TIME); 
            sysprintf(":");
            vTaskDelay(CARD_INIT_TIMEOUT_INTERVAL);
            if((powerUpTick != 0) && ((xTaskGetTickCount() - powerUpTick) > CARD_INIT_TIMEOUT_TIME) )
            {
                //sysprintf("readVN break (time:%d)...\r\n", (int)(xTaskGetTickCount() - powerUpTick));
                reVal = TSREADER_CHECK_READER_ERROR;
                {
                    char str[512];
                    sprintf(str, "   Card Reader --> readVN break (time:%d)...\r\n", (int)(xTaskGetTickCount() - powerUpTick));
                    LoglibPrintf(LOG_TYPE_ERROR, str);
                }
                return reVal;  
            }         
        }       

        #if(SUPPORT_ECC_CARD_READER)
        if(eccNeedRestFlag)
        {
            uint16_t returnInfo, returnCode;
            vTaskDelay(5000/portTICK_RATE_MS);  
            time_t epmUTCTime = GetCurrentUTCTime();
            uint16_t reValTmp = ECCPPRReset(&returnInfo, &returnCode, epmUTCTime, epmUTCTime, FALSE);
            if(reValTmp == CARD_MESSAGE_RETURN_SUCCESS)
            {
                eccNeedRestFlag = FALSE;
                sysprintf("== INFORMATION == ECCPPRReset OK... (eccNeedRestFlag = %d)\r\n", eccNeedRestFlag); 
            }
            else
            {
                //sysprintf("== INFORMATION == ECCPPRReset ERROR...(eccNeedRestFlag = %d)\r\n", eccNeedRestFlag); 
                {
                    char str[512];
                    sprintf(str, "== INFORMATION == ECCPPRReset ERROR...(eccNeedRestFlag = %d)\r\n", eccNeedRestFlag); 
                    LoglibPrintf(LOG_TYPE_ERROR, str);
                }
                reVal = TSREADER_CHECK_READER_ERROR;                    
                //DataProcessSendStatusData(0, "pprresetErr", WEB_POST_EVENT_ALERT);
                return reVal;
            }             
        }    
        else
        {
            sysprintf("== INFORMATION == ECCPPRReset IGNORE...(eccNeedRestFlag = %d)\r\n", eccNeedRestFlag); 
        }    
        #endif
       
        //needInitReaderFlag = FALSE;
        return reVal;
    }
}

#define RECEIVE_WAIT_INTERVAL   10
int EPMReaderReceiveCmd(uint32_t waitTime, uint8_t** receiveData, uint16_t* dataLen)
{
    int timeoutCounter;    
    int ret,rxCount,rxBcount,len;
    ret = 0;
    rxBcount = 0;
    rxCount = 0;  
    //sysprintf("xTaskGetTickCount\n");
    TickType_t mTick = xTaskGetTickCount(); 
    timeoutCounter = waitTime/RECEIVE_WAIT_INTERVAL;
    //sysprintf("xTaskGetTickCount : %d\n", mTick);
    while(1)
    {
        ret = pUartInterface->readFunc(rxTemp, CMD_UART_RX_BUFF_LEN);
        if(ret!=0)
        {
            rxCount+=ret;            
            memcpy(rxBuffer + rxBcount, rxTemp, ret);
            rxBcount = rxBcount + ret; 
            //#if(ENABLE_RX_PRINT)
            #if(0)
            {
                int i;
				sysprintf("The read is %d bytes!\n",ret);
                for(i=0; i<ret; i++)
                {
                    sysprintf("Data[%d] we read is 0x%02x!\n", i, rxTemp[i]);
                }   
            } 
            #endif  
        }
        else
        {
            if(rxCount>=5)
            {
                //sysprintf("\r\n !!!  receiveReaderCmd break section 1: %d ticks, timeoutCounter = %d\n", xTaskGetTickCount() - mTick, timeoutCounter);
                break;
            }
            
            if(timeoutCounter-- == 0)
            {              
                rxBcount = 0;
                //sysprintf("receiveReaderCmd timeout 1, break (%d ticks)....\n", xTaskGetTickCount() - mTick);
                goto exitHandle;
            }
            sysprintf(".");
            vTaskDelay(RECEIVE_WAIT_INTERVAL/portTICK_RATE_MS); 
        }
    }
    #warning need check here
    if(rxCount < 5)
    {
        sysprintf("Total Read %d data!, break!!! (rxCount<5)\n", rxCount);
        rxBcount = 0;
        goto exitHandle;
    }
    if(rxBuffer[0] != 0xEA)
    {
        sysprintf("carddrv: receiveReaderCmd header err [0x%02x: 0xEA]\n", rxBuffer[0]);
        rxBcount = 0;
        goto exitHandle;
    }
    len=(rxBuffer[3]<<8)+rxBuffer[4]+7;
    
    #if(ENABLE_RX_PRINT)
    timeoutCounter = 200;
    #else
    timeoutCounter = 10;
    #endif
    
    //if(len>rxBcount) 
    //    sysprintf("Len we want read is %d!,the data len we get is %d. continue reading...\n", len, rxBcount);
    //mTick = xTaskGetTickCount(); 
    while(len>rxBcount)
    {
        ret = pUartInterface->readFunc(rxTemp, CMD_UART_RX_BUFF_LEN);
        if(ret!=0)
        {
            memcpy(rxBuffer + rxBcount, rxTemp, ret);
            rxBcount = rxBcount + ret;   
            //#if(ENABLE_RX_PRINT)
            #if(0)
            {
                int i;
                for(i=0; i<ret; i++)
                {
                    sysprintf("Data 2 [%d] we read is 0x%02x!\n", i, rxTemp[i]);
                }   
            } 
            #endif  
            /*
            if((rxBuffer[rxBcount-2] == 0x90) && (rxBuffer[rxBcount-1] == 0x0))
            {
                sysprintf(" -->Receive Data Finish!!! don`t break (%d ticks)\r\n", xTaskGetTickCount() - mTick);
                break;
            }
            */
        }
        else
        {
            if(timeoutCounter-- == 0)
            {
                
                sysprintf("receiveReaderCmd timeout 2, break (%d ticks)....\n", (int)(xTaskGetTickCount() - mTick));
                sysprintf("Len we want read is %d!,the data len we get is %d\n", len, rxBcount);
                {
                    int i;
                    sysprintf("--- dump rxBuffer ---\n");
                    for(i=0; i<rxBcount; i++)
                    {
                        sysprintf(" [%d]: 0x%02x\n", i, rxBuffer[i]);
                    }
                    sysprintf("--------------------\n");
                }
                rxBcount = 0;
                goto exitHandle;                
            }
            vTaskDelay(RECEIVE_WAIT_INTERVAL/portTICK_RATE_MS); 
        }
    }
exitHandle:
    #if(ENABLE_RX_TIME_PRINT)
    if(rxBcount!=0)
        sysprintf("receiveReaderCmd OK case %d ticks....\n", xTaskGetTickCount() - mTick);
    #endif
	*receiveData = rxBuffer;
	*dataLen = rxBcount;
    return rxBcount;
}

uint32_t EPMReaderSendCmd(uint8_t buff[], int len)
{
    uint32_t ret;
    #if(ENABLE_TX_PRINT)
    {
        int i;
        sysprintf("\r\n =====  EPMReaderSendCmd (len = %d)====\r\n", len);
        for(i = 0; i < len; i++)
        {
            sysprintf("0x%02x, ", buff[i]);
            if(i%10 == 9)
                sysprintf("\r\n");

        }
        sysprintf("\r\n =====  EPMReaderSendCmd ====\r\n");
    }
    #endif
    //sysprintf("\r\n =====  EPMReaderSendCmd ==ENTER writeFunc==\r\n");
    ret = pUartInterface->writeFunc(buff, len);
    //sysprintf("\r\n =====  EPMReaderSendCmd ==EXIT writeFunc==\r\n");
    //sysprintf("The write is %d! len is %d!\n",ret,len);
    return ret;
}

uint16_t EPMReaderParserMessage(uint8_t msgType, uint8_t* msgBuff, uint16_t msgLen, uint16_t* returnCode)
{    
    uint16_t dataLen;
    //uint8_t lrcValue;
    uint8_t targetType1, targetType2;
    if(returnCode == NULL)
    {
        sysprintf("\r\n~~~ EPMReaderParserMessage msgType = %d[%d] error (returnCode == NULL)~~~>\r\n", msgType, msgLen);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    *returnCode = CARD_MESSAGE_CODE_NO_USE; //錯誤的總稱

    #if(ENABLE_RX_PRINT)
    int i;    
    sysprintf("\r\n~~~ EPMReaderParserMessage msgType = %d[%d] ~~~>\r\n", msgType, msgLen);
    for(i = 0; i<msgLen; i++)
    {
        sysprintf("0x%02x, ", msgBuff[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<~~~ EPMReaderParserMessage ~~~\r\n");
    #endif
    
    if(msgBuff[0] != 0xEA)
    {
        sysprintf("EPMReaderParserMessage(parser type:%d): header err [0x%02x: 0xEA]\n", msgType, msgBuff[0]);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    if((msgBuff[msgLen-2] != 0x90) || (msgBuff[msgLen-1] != 0x0))
    {
        sysprintf("EPMReaderParserMessage(parser type:%d): end flag err [0x%02x: 0x%02x]\n", msgType, msgBuff[msgLen-2], msgBuff[msgLen-1]);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    
    switch(msgType)
    {
        case CARD_MESSAGE_TYPE_CN:
            targetType1 = 0x02;
            targetType2 = 0x01;
            break;
        case CARD_MESSAGE_TYPE_TIME:
            targetType1 = 0x01;
            targetType2 = 0x02;
            break;        
        case CARD_MESSAGE_TYPE_API_VER_NO:
            targetType1 = 0x01;
            targetType2 = 0x00;
            break;
        case CARD_MESSAGE_TYPE_READER_ID:
            targetType1 = 0x04;
            targetType2 = 0x01;
            break;
        
        default:
            return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    
    
    if((msgBuff[1] != targetType1) || (msgBuff[2] != targetType2))
    {
        sysprintf("EPMReaderParserMessage(parser type:%d): type err [0x%02x, 0x%02x : 0x%02x, 0x%02x]\n", msgType, msgBuff[1], msgBuff[2], targetType1, targetType2);
        
        #if(ENABLE_SHOW_RETURN_DATA)
        {
            int i;
            sysprintf("\r\n--- Raw Data [%d] --->\r\n", msgLen);
            for(i = 0; i<msgLen; i++)
            {
                 sysprintf("0x%02x, ", msgBuff[i]);
                if(i%10 == 9)
                    sysprintf("\r\n");

            }
            sysprintf("\r\n<--- Raw Data ---\r\n");   
        }   
        #endif
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    dataLen = rxBuffer[4]|(rxBuffer[3]<<8);    
    //lrcValue = EPMReaderLRC(rxBuffer, 5, dataLen-1);  
    
    switch(msgType)
    {
        case CARD_MESSAGE_TYPE_API_VER_NO:   
            //--- IVN [13] --->
            //0x46, 0x52, 0x44, 0x20, 0x76, 0x31, 0x35, 0x31, 0x31, 0x30,
            //0x32, 0x30, 0x31, [FRD v15110201]
        
            //--- IVN [13] --->
            //0x50, 0x31, 0x4D, 0x20, 0x76, 0x31, 0x37, 0x30, 0x36, 0x31,
            //0x36, 0x30, 0x31,
            //IVN: [P1M v17061601]
            //<--- IVN ---

            //<--- IVN ---

            memset(verStr, 0x0, sizeof(verStr)); 
            if(dataLen == 1)
            {
                sysprintf("readVN error: 0x%02x !!! \r\n", rxBuffer[5]);              
                return CARD_MESSAGE_RETURN_PARSER_ERROR;
            }            
            else
            {
                verStrLen = dataLen;
                memcpy(verStr, rxBuffer+5, verStrLen);
                #if(ENABLE_SHOW_RETURN_DATA)
                sysprintf("\r\n--- VN [%d] --->\r\n", verStrLen);
                for(int i = 0; i<verStrLen; i++)
                {
                    //sysprintf(" [%03d] : 0x%02x \r\n", i, rxBuffer[5+i]);
                    sysprintf("0x%02x, ", verStr[i]);
                    if(i%10 == 9)
                        sysprintf("\r\n");

                }
                
                sysprintf("\r\n<--- VN ---\r\n");
                #endif
               /* sysprintf("IVN: [");
                terninalPrintf("IVN: [");
                for(int i = 0; i<verStrLen; i++)
                {
                    sysprintf("%c", verStr[i]);
                    terninalPrintf("%c", verStr[i]);
                }
                sysprintf("]");
                terninalPrintf("]"); */
            }

            break;
        case CARD_MESSAGE_TYPE_READER_ID:   
            /*
            0xEA, 0x04, 0x01, 0x00, 0x07, 0x42, 0x1F, 0x7B, 0x5E, 0x90,
            0x00, 0xE8, 0x90, 0x00,
            
            --- Raw Data [14] --->
            0xEA, 0x04, 0x01, 0x00, 0x07, 0xDA, 0x0E, 0x7B, 0x5E, 0x90,
            0x00, 0x61, 0x90, 0x00,
            <--- Raw Data ---

            */
            memset(readerId, 0x0, sizeof(readerId));             
            if(dataLen == sizeof(readerId))
            {
                memcpy(readerId, rxBuffer+5, dataLen);
                #if(ENABLE_SHOW_RETURN_DATA)
                sysprintf("\r\n--- READER ID [%d] --->\r\n", dataLen);
                for(int i = 0; i<dataLen; i++)
                {
                    //sysprintf(" [%03d] : 0x%02x \r\n", i, rxBuffer[5+i]);
                    sysprintf("0x%02x, ", readerId[i]);
                    if(i%10 == 9)
                        sysprintf("\r\n");

                }
                sysprintf("\r\n<--- READER ID ---\r\n");
                #endif
            }
            else
            {
                sysprintf("Read_ReaderId error: len = %d error !!! \r\n", dataLen);
                return CARD_MESSAGE_RETURN_PARSER_ERROR;
            } 
            break;
            
        case CARD_MESSAGE_TYPE_CN:
            memset(cnData, 0x0, sizeof(cnData)); 
            cnLen = 0;
            if(dataLen == 1)
            {
                switch(rxBuffer[5])
                {         
                case 1://Get fail 
                    break;
                case 2://Multicard
                    break;
                default://Other Fail
                    break;
                }   
                
                *returnCode = rxBuffer[5];               
                return CARD_MESSAGE_TYPE_CN_RETURN_LEN_1;
            }
            #warning need check here =7???
            else// if(dataLen == 7) 
            {
                switch(rxBuffer[5])
                {
                    case 1:
                        #if(ENABLE_CARD_READER_DRIVER) 
                        sysprintf("readCN: Card Type 107 [0x%02x]\r\n", rxBuffer[5]);
                        #endif
                        return CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE;
                    case 2:
                        #if(ENABLE_CARD_READER_DRIVER) 
                        sysprintf("readCN: Card Type 200 [0x%02x]\r\n", rxBuffer[5]);
                        #endif
                        return CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE;
                    case 3:
                        #if(ENABLE_CARD_READER_DRIVER) 
                        sysprintf("readCN: Card Type Far Eastern [0x%02x]\r\n", rxBuffer[5]);
                        #endif
                        return CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE;;
                    case 4:
                        #if(ENABLE_CARD_READER_DRIVER) 
                        sysprintf("readCN: Card Type Easy Card [0x%02x]\r\n", rxBuffer[5]);
                        #endif
						currentCardType = CARD_TYPE_ID_ECC;
                        sysprintf("\r\n => readCN (ECC) : [0x%02x : 0x%02x : 0x%02x : 0x%02x] \r\n", rxBuffer[6], rxBuffer[7], rxBuffer[8], rxBuffer[9]);
                        memcpy(cnData, rxBuffer+6, rxBuffer[(msgLen-3)]);
                        cnLen = rxBuffer[(msgLen-3)];
                        #if(SUPPORT_ECC_CARD_READER)
                        break;
                        #else
                        return CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE;
                        #endif
                    case 5:
                        #if(ENABLE_CARD_READER_DRIVER) 
                        sysprintf("readCN: Card Type iPASS [0x%02x]\r\n", rxBuffer[5]);
                        #endif
						currentCardType = CARD_TYPE_ID_IPASS;
                        //沒有LRC
                        sysprintf("\r\n => readCN (IPASS) : [0x%02x : 0x%02x : 0x%02x : 0x%02x] \r\n", rxBuffer[6], rxBuffer[7], rxBuffer[8], rxBuffer[9]);
                        memcpy(cnData, rxBuffer+6, rxBuffer[(msgLen-3)]); 
                        cnLen = rxBuffer[(msgLen-3)];
                        //return CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE;                
                        break;
                    default:
                        sysprintf("\r\n => readCN warning(Card Type Other !!!! [0x%02x]) : ", rxBuffer[5]);
                        {
                            int i, len;
                            len = rxBuffer[(msgLen-3)];
                            memcpy(cnData, rxBuffer+6, len); 
                            cnLen = rxBuffer[(msgLen-3)];
                            if(len != 0)
                            {
                                sysprintf("\r\n [ ");
                                for(i = 0; i<len; i++)
                                {
                                    cnData[i]=rxBuffer[6+i];
                                    sysprintf("0x%02x ",cnData[i]);
                                } 
                                sysprintf("]\r\n");
                            }
                            
                        }
                        return CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE;//CARD_MESSAGE_TYPE_CN_RETURN_UNKNOWN_CARDTYPE;
                }               
            }             
            break;
        case CARD_MESSAGE_TYPE_TIME:  
             
            if(dataLen != 7)
            {
                sysprintf("Read_TIME: dataLen %d error \r\n", dataLen);
                return CARD_MESSAGE_RETURN_PARSER_ERROR;
            }
            //沒有LRC
            //sysprintf("=> dataTime(%d): year %d, Month %d, Day %d, Hour %d, Minute %d, Second %d\n", 
            //                rxBuffer[5], rxBuffer[6], rxBuffer[7], rxBuffer[8], rxBuffer[9], rxBuffer[10], rxBuffer[11]);
           
            unixTime.year  = rxBuffer[6] + 2000;
            unixTime.mon   = rxBuffer[7];
            unixTime.day   = rxBuffer[8];
            unixTime.hour  = rxBuffer[9];
            unixTime.min   = rxBuffer[10];
            unixTime.sec   = rxBuffer[11];//sCurTime.u32cSecond;
            
            sprintf(dataStr, "%04d%02d%02d", unixTime.year, unixTime.mon, unixTime.day);
            sprintf(timeStr, "%02d%02d%02d", unixTime.hour, unixTime.min, unixTime.sec);

            utcTime = sysDOS_Time_To_UTC(unixTime);

            dataTime[0]=(char)utcTime;
            dataTime[1]=(char)(utcTime>>8);
            dataTime[2]=(char)(utcTime>>16);
            dataTime[3]=(char)(utcTime>>24);
            sysprintf("\r\n => readTime (%d)[%s][%s] : [0x%02x : 0x%02x : 0x%02x : 0x%02x] \r\n", utcTime, dataStr, timeStr, dataTime[0], dataTime[1], dataTime[2], dataTime[3]);
            break;     
       
        default:
            break;
    }    

    return CARD_MESSAGE_RETURN_SUCCESS;
}
BOOL EPMReaderBreakCheckReader(void)
{
    BOOL reVal = TRUE;
    //checkReaderFlag = FALSE;
    return reVal;
}
BOOL EPMReaderProcessCN(tsreaderCNResultCallback callback)
{
    uint16_t returnInfo;
    uint16_t returnCode;
    returnInfo = readCN(&returnCode);
    if(cnLen > 0)
    {    
        if(callback != NULL)
        {
            callback(TRUE, cnData, cnLen);
            return TRUE;
        }
    }
    callback(FALSE, NULL, 0);
    return TRUE;
}

BOOL EPMReaderProcess(uint16_t targetDeduct, tsreaderDepositResultCallback callback)
{   
    BOOL retval = FALSE;
    uint16_t returnCode;
    uint16_t returnInfo;
    TickType_t tickLocalStart;    
    char title[64];
    //sysprintf(" ==> CardReadProcess\n");   
    ptsreaderDepositResultCallback =  callback;  
    
    tickLocalStart = xTaskGetTickCount();
    currentTargetDeduct = 0;
#if(0) 
    currentTargetDeduct = targetDeduct;
	retval = ECCLibProcess(&returnInfo, &returnCode, targetDeduct, callback, cnData, dataTime, machineNo);
#else
    returnInfo = readCN(&returnCode);
    if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
    {         
        returnInfo = CARD_MESSAGE_RETURN_SUCCESS;//readTime();
        if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
        {      
            #if(0)
                currentTargetDeduct = targetDeduct;
                retval = ECCLibProcess(&returnInfo, &returnCode, targetDeduct, callback, cnData, utcTime, machineNo);
            #else
			switch(currentCardType)
			{
                
				case CARD_TYPE_ID_IPASS:
					//currentTargetDeduct = targetDeduct;
					//retval = IPASSLibProcess(&returnInfo, &returnCode, targetDeduct, callback, cnData, dataTime, machineNo);
                    retval = TRUE;
					break;
				case CARD_TYPE_ID_ECC:
                    //epmCurrentUTCTime = GetCurrentUTCTime();
                    //currentTargetDeduct = targetDeduct;
					//retval = ECCLibProcess(&returnInfo, &returnCode, targetDeduct, callback, cnData, epmCurrentUTCTime, machineNo);
                    retval = TRUE;
					break;
                default:
                    retval = TRUE;
                    //break;

			}
            #endif
            
            //if(retval)
            //    break;
        } 
        else //returnInfo = readTime();
        {
        } //returnInfo = readTime();   
             
    } 
    else
    {        
        //sysprintf("ZZ(0x%02x:%d)", returnInfo, returnCode);
        static int checkCounter = 0;
        switch(returnInfo)
        {                         
            case CARD_MESSAGE_TYPE_CN_RETURN_LEN_1:
                checkCounter = 0;
                //sysprintf(":"); 
                break;
            case CARD_MESSAGE_RETURN_PARSER_ERROR:
            case CARD_MESSAGE_RETURN_TIMEOUT:
            case CARD_MESSAGE_RETURN_SEND_ERROR:  
                sysprintf(" ==> readCN return FLASE: errorCode = 0x%04x(%d),  returnInfo = 0x%02x !!!\n", returnCode, returnCode, returnInfo);                
                break;
            case CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE:
                sysprintf(" ==> readCN return FLASE: errorCode = 0x%04x(%d),  returnInfo = 0x%02x !!!\n", returnCode, returnCode, returnInfo);
                checkCounter++;
                if(checkCounter > 3)
                {
                    if(ptsreaderDepositResultCallback != NULL)
                    {
                        ptsreaderDepositResultCallback(FALSE, returnInfo, returnCode);
                    }
                    
                    //sprintf(title, "CN(0x%02x:%d)", returnInfo, returnCode);
                    //DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);

                    retval = TRUE; //完成 離開刷卡 loop
                }
                break;
            case CARD_MESSAGE_TYPE_CN_RETURN_UNKNOWN_CARDTYPE:
                sysprintf(" ==> readCN return FLASE: errorCode = 0x%04x(%d),  returnInfo = 0x%02x !!!\n", returnCode, returnCode, returnInfo);
                #if(0)//繼續循卡                
                if(ptsreaderDepositResultCallback != NULL)
                {
                    ptsreaderDepositResultCallback(FALSE, returnInfo, returnCode);
                }
                
                sprintf(title, "CN(0x%02x:%d)", returnInfo, returnCode);
                DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);
                
                retval = TRUE; //完成 離開刷卡 loop
                #endif
                break;
				/*
            case CARD_MESSAGE_TYPE_CN_RETURN_IN_BLK:
                sprintf(title, "CN(0x%02x:%d)", returnInfo, returnCode);
                DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);
                
                #warning lock card   
                //returnInfo = iPass_Execute('L', targetDeduct, &returnCode);
                // // 票值回復後要再繼續執行 Read Card Basic，但卡號為 4 Bytes 的卡號。?????
                //sysprintf("****  [INFO Card Reader] Lock Card : errorCode = 0x%04x(%d),  returnInfo = 0x%02x !!!\n", returnCode, returnCode, returnInfo);  
                
                if(ptsreaderDepositResultCallback != NULL)
                {
                    ptsreaderDepositResultCallback(FALSE, returnInfo, returnCode);
                }
                
                sprintf(title, "LOCK(0x%02x:%d)", returnInfo, returnCode);
                DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);                

                retval = TRUE; //完成 離開刷卡 loop    
                
                break;
				*/
        }
    }
#endif
    if(retval)
	{
        sysprintf(" [INFO Card Reader] <EPMReaderProcess>  : [%d] ticks\n", (int)(xTaskGetTickCount() - tickLocalStart));  
	}
	else
	{// 清成沒有卡片
		currentCardType = CARD_TYPE_ID_NONE;
	}
    
    return retval;
}
BOOL EPMReaderSignOnProcess(void)
{
    #if(1)
    return TRUE;
    #else
    uint16_t returnInfo, returnCode;
    time_t epmUTCTime = GetCurrentUTCTime();
    returnInfo = ECCPPRReset(&returnInfo, &returnCode, epmUTCTime, epmUTCTime, TRUE);

    if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
    { 
        sysprintf("\r\n     -[info]-> EPMReaderSignOnProcess ECCPPRReset SUCCESS!!\n");
        return TRUE;
    }
    else
    {
        sysprintf("\r\n     -[info]-> EPMReaderSignOnProcess ECCPPRReset ERROR!!\n");
        //先只做一次
        return TRUE;
        //return FALSE;
    }
    #endif
}

BOOL EPMReaderGetBootedStatus(void)
{
    if(readVN() == CARD_MESSAGE_RETURN_SUCCESS)
        return TRUE;
    else
        return FALSE;
}


BOOL EPMReaderGetBootedStatusEx(void)
{
    BOOL resultFlag1 = FALSE;
    BOOL resultFlag2 = FALSE;
    uint32_t CTSValue;
    uint8_t* receiveData;
	uint16_t receiveDataLen;
    EPMReaderFlushBuffer();
    EPMReaderSendCmd(readVNCmd,sizeof(readVNCmd));
    memset(receiveData,0x00,sizeof(readVNCmd));
    memset(rxBuffer,0x00,sizeof(readVNCmd));
    EPMReaderReceiveCmd(1000, &receiveData, &receiveDataLen);
    /*
    if(receiveDataLen > 0)
    {
        terninalPrintf("receiveDataLen=");
        int counter = 0; 
        //for(int i=0;i<receiveDataLen;i++)
        for(int i=0;i<sizeof(readVNCmd);i++)
        {

            terninalPrintf("%02x ",receiveData[i]);
        }
        terninalPrintf("\n  ");
    
    }
    */
    //pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    
    
    //UINT32 CTSFlag,uArg0;
    //pUartInterface->ioctlFunc(UART_IOC_GETCTSSTATE, (UINT32)&uArg0, (UINT32)&CTSFlag);
    
    //terninalPrintf("CTSFlag=%d\n",CTSFlag);
    //terninalPrintf("inpw(REG_UART0_MSR+uOffset)=%32b\n",inpw(REG_UART0_MSR+UART2*UARTOFFSET));
    
    if(memcmp(readVNCmd,receiveData,sizeof(readVNCmd)) == 0)
    {
        //pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
        terninalPrintf("RX & TX interconnect success.\r\n");
        resultFlag1 = TRUE;
        //return TRUE;
    }
    else
    {
        terninalPrintf("RX & TX interconnect error.\r\n");
        //pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
        resultFlag1 = FALSE;
        //return FALSE;
    }
    pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    CTSValue =  inpw(REG_UART0_MSR+UART2*UARTOFFSET) ;
    //terninalPrintf("inpw(REG_UART0_MSR+uOffset)=%32b\n",CTSValue);
    if(CTSValue & 0x01 )
    {
        terninalPrintf("RTS & CTS interconnect success.\r\n");
        resultFlag2 = TRUE;
    }
    else
    {
        terninalPrintf("RTS & CTS interconnect error.\r\n");
        resultFlag2 = FALSE;
    }
    
    return resultFlag1 & resultFlag2;
}

void EPMReaderSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue)
{  
    #if(1)
    
    #else
    switch(currentCardType)
    {
    	case CARD_TYPE_ID_IPASS:
        	IPASSSaveFile(pt, paraValue, currentTargetDeduct, readerId, dataStr, timeStr);
			break;
		case CARD_TYPE_ID_ECC:
            ECCSaveFile(currentTargetDeduct, epmCurrentUTCTime);
			break;
	}
    #endif
	
}

void EPMReaderSaveFilePure(void)
{    
    #if(1)

    #else
    switch(currentCardType)
    {
    	case CARD_TYPE_ID_IPASS:
        	IPASSSaveFilePure(currentTargetDeduct, readerId, dataStr, timeStr);
			break;
		case CARD_TYPE_ID_ECC:
            ECCSaveFilePure(currentTargetDeduct, epmCurrentUTCTime);
			break;
	}
	#endif
}

char EPMReaderLRC(uint8_t *array,int start,int len)
{
    int i;
    char temp;
    temp = array[start];
    for(i=(start+1); i<(start+len); i++)
    {
        temp ^= array[i];
        //sysprintf("we got num %02d cnData = 0x%x\n",i,array[i]);
    }
    //sysprintf("Get EPMReaderLRC 0x%02x!\n",temp);
    return temp;
}
void EPMReaderFlushBuffer(void)
{
    //sysprintf(" --> EPMReaderFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}

void EPMReaderSetDisableReaderFlag(BOOL flag)
{
    disableReaderFlag = flag;
}

void EPMReaderGetVersion(char* ReaderVerBuf)
{      
    for(int i = 0; i<verStrLen; i++)
    {     
        ReaderVerBuf[i] = verStr[i];
        
        //sysprintf("%c", verStr[i]);
        //terninalPrintf("%c", verStr[i]);
    }
 
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


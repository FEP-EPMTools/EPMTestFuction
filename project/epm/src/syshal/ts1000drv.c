/**************************************************************************//**
* @file     ts1000drv.c
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
#include "ts1000drv.h"
#include "interface.h"
#include "halinterface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define TS1000_DRV_UART   UART_2_INTERFACE_INDEX
#define CAD_UART_RX_BUFF_LEN   64 //need check uart.c UARTRXBUFSIZE[UART_NUM] 

#define ENABLE_TX_PRINT 1
#define ENABLE_RX_PRINT 1

#define CARD_MESSAGE_TYPE_CN                0x01  
#define CARD_MESSAGE_TYPE_TIME              0x02
#define CARD_MESSAGE_TYPE_ICD               0x03
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE     0x04

#define CARD_INIT_TIMEOUT_TIME   25000 
#define CARD_INIT_TIMEOUT_INTERVAL   1000 
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;

static UINT8 rxTemp[CAD_UART_RX_BUFF_LEN];
static UINT8 rx_buff[500];

static UINT8 read_ivn[] =  {0xEA,0x05,0x00,0x00,0x01,0x00,0x90,0x00};
static UINT8 read_cn[] =    {0xEA,0x02,0x01,0x00,0x01,0x00,0x90,0x00};
static UINT8 read_time[] =  {0xEA,0x01,0x02,0x00,0x01,0x00,0x90,0x00};


static UINT8 data_cn[8]= {0};
static UINT8 data_icd[512]= {0};
static UINT8 data_time[4]= {0};
static char data_mn[16]= {0};

static DateTime_T unix_Time;
static UINT32 utcTime;
static RTC_TIME_DATA_T sCurTime;

static TickType_t powerUpTick = 0;
static BOOL checkReaderFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void CardFlushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
        sysprintf("Set TX Flush fail!\n");
        return;
    }
    if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}

static char LRC(UINT8 *array,int start,int len)
{
    int i;
    char temp;
    temp = array[start];
    for(i=(start+1); i<(start+len); i++)
    {
        temp ^= array[i];
        //sysprintf("we got num %02d data_cn = 0x%x\n",i,array[i]);
    }
    //sysprintf("Get LRC 0x%02x!\n",temp);
    return temp;
}

static uint32_t uart_w(UINT8 buff[], int len)
{
    uint32_t ret;
    #if(ENABLE_TX_PRINT)
    {
        int i;
        sysprintf(" =====  uart_w (len = %d)====\r\n", len);
        for(i = 0; i < len; i++)
        {
            sysprintf("0x%02x, ", buff[i]);
            if(i%10 == 9)
                sysprintf("\r\n");

        }
        sysprintf("\r\n =====  uart_w ====\r\n", len);
    }
    #endif
    ret = pUartInterface->writeFunc(buff, len);
    //sysprintf("The write is %d! len is %d!\n",ret,len);
    return ret;
}

static int uart_r(void)
{
    int timeoutCounter = 20;//by sam
    int ret,rx_count,rx_bcount,len;
    ret = 0;
    rx_bcount = 0;
    rx_count = 0;//by sam
    while(1)
    {
        ret = pUartInterface->readFunc(rxTemp, CAD_UART_RX_BUFF_LEN);
        if(ret!=0)
        {
            rx_count+=ret;
            //sysprintf("The read is %d bytes!\n",ret);
            memcpy(rx_buff + rx_bcount, rxTemp, ret);
            rx_bcount = rx_bcount + ret; 
            //#if(ENABLE_RX_PRINT)
            #if(0)
            {
                int i;
                for(i=0; i<ret; i++)
                {
                    sysprintf("Data[%d] we read is 0x%02x!\n", i, rxTemp[i]);
                }   
            } 
            #endif  
        }
        else
        {
            if(rx_count>=5)
                break;
            
            if(timeoutCounter-- == 0)
            {              
                rx_bcount = 0;
                goto exitHandle;
            }
            //sysprintf(".");
            #ifdef SUPPORT_FREERTOS
            vTaskDelay(10/portTICK_RATE_MS); 
            #else
            sysDelay(1);
            #endif
        }
    }
    #warning need check here
    if(rx_count < 5) //by sam
    {
        sysprintf("Total Read %d data!, break!!! (rx_count<5)\n", rx_count);
        rx_bcount = 0;
        goto exitHandle;
    }
    if(rx_buff[0] != 0xEA)
    {
        sysprintf("carddrv: uart_r header err [0x%02x: 0xEA]\n", rx_buff[0]);
        rx_bcount = 0;
        goto exitHandle;
    }
    len=(rx_buff[3]<<8)+rx_buff[4]+7;
    
    #if(ENABLE_RX_PRINT)
    timeoutCounter = 100;
    #else
    timeoutCounter = 10;
    #endif
    
    //if(len>rx_bcount) 
    //    sysprintf("Len we want read is %d!,the data len we get is %d. continue reading...\n", len, rx_bcount);
    
    while(len>rx_bcount)
    {
        ret = pUartInterface->readFunc(rxTemp, CAD_UART_RX_BUFF_LEN);
        if(ret!=0)
        {
            memcpy(rx_buff + rx_bcount, rxTemp, ret);
            rx_bcount = rx_bcount + ret;   
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
            if((rx_buff[rx_bcount-2] == 0x90) && (rx_buff[rx_bcount-1] == 0x0))
            {
                sysprintf(" -->Receive Data Finish!!!break\r\n");
                break;
            }
        }
        else
        {
            if(timeoutCounter-- == 0)
            {
                
                sysprintf("uart_r timeout 2, break....\n");
                sysprintf("Len we want read is %d!,the data len we get is %d\n", len, rx_bcount);
                {
                    int i;
                    sysprintf("--- dump rx_buff ---\n");
                    for(i=0; i<rx_bcount; i++)
                    {
                        sysprintf(" [%d]: 0x%02x\n", i, rx_buff[i]);
                    }
                    sysprintf("--------------------\n");
                }
                rx_bcount = 0;
                goto exitHandle;                
            }
            vTaskDelay(10/portTICK_RATE_MS); 
        }
    }
exitHandle:
   
    return rx_bcount;
}


static BOOL parserMessage(uint8_t msgType, uint8_t* msgBuff, uint16_t msgLen)
{    
    BOOL reVal = FALSE;
    
    uint16_t dataLen;
    uint8_t lrcValue;
    uint8_t targetType1, targetType2;
    #if(ENABLE_RX_PRINT)
    int i;    
    sysprintf("\r\n~~~ parserMessage msgType = %d[%d] ~~~>\r\n", msgType, msgLen);
    for(i = 0; i<msgLen; i++)
    {
        sysprintf("0x%02x, ", msgBuff[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<~~~ parserMessage ~~~\r\n");
    #endif
    
    if(msgBuff[0] != 0xEA)
    {
        sysprintf("parserMessage(parser type:%d): header err [0x%02x: 0xEA]\n", msgType, msgBuff[0]);
        goto exitFuncion; 
    }
    if((msgBuff[msgLen-2] != 0x90) || (msgBuff[msgLen-1] != 0x0))
    {
        sysprintf("parserMessage(parser type:%d): end flag err [0x%02x: 0x%02x]\n", msgType, msgBuff[msgLen-2], msgBuff[msgLen-1]);
        goto exitFuncion; 
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
        case CARD_MESSAGE_TYPE_ICD:
            targetType1 = 0x05;
            targetType2 = 0x01;
            break;
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE:
            targetType1 = 0x05;
            targetType2 = 0x02;
            break;
        default:
            return reVal;
    }
    
    
    if((msgBuff[1] != targetType1) || (msgBuff[2] != targetType2))
    {
        sysprintf("parserMessage(parser type:%d): type err [0x%02x, 0x%02x : 0x02, 0x01]\n", msgType, msgBuff[1], msgBuff[2], targetType1, targetType2);
        goto exitFuncion; 
    }
    dataLen = rx_buff[4]|(rx_buff[3]<<8);    
    lrcValue = LRC(rx_buff, 5, dataLen-1);  
    
    switch(msgType)
    {
        case CARD_MESSAGE_TYPE_CN:
            
            if(dataLen == 1)
            {
                switch(rx_buff[5])
                {         
                case 1:
                    //sysprintf("Read_CN error: Get fail \r\n");
                    //sysprintf("~");
                    break;
                case 2:
                    sysprintf("Read_CN error: Multicard !!! \r\n");
                    //sysprintf("=");
                    break;
                default:
                    sysprintf("Read_CN error: Other Fail !!! \r\n");
                    //sysprintf("?");
                    break;
                }
                memset(data_cn, 0x0, sizeof(data_cn)); 
                goto exitFuncion;
            }
            #warning need check here =7???
            else// if(dataLen == 7) 
            {
                char* pStr;
                switch(rx_buff[5])
                {
                    /*
                    01 : 107?
                    02 : 200?
                    03 : ???????????
                    04 : ???
                    05 : ?????
                    */
                    
                    case 1:
                        //sysprintf("Read_CN: Card Type 107 [0x%02x]\r\n", rx_buff[5]);
                        pStr = "Card Type 107 [0x01]";
                        //CardSetPollingTime(portMAX_DELAY);
                        //SetDepositResultStatus(FALSE, 0x01);
                        goto exitFuncion;
                        //break;
                    case 2:
                        //sysprintf("Read_CN: Card Type 200 [0x%02x]\r\n", rx_buff[5]);
                        pStr = "Card Type 200 [0x02]";
                        //CardSetPollingTime(portMAX_DELAY);
                        //SetDepositResultStatus(FALSE, 0x01);
                        goto exitFuncion;
                        //break;
                    case 3:
                        //sysprintf("Read_CN: Card Type Far Eastern [0x%02x]\r\n", rx_buff[5]);
                        pStr = "Far Eastern [0x03]";
                        //CardSetPollingTime(portMAX_DELAY);
                        //SetDepositResultStatus(FALSE, 0x01);
                        goto exitFuncion;
                        //break;
                    case 4:
                        //sysprintf("Read_CN: Card Type Easy Card [0x%02x]\r\n", rx_buff[5]);
                        pStr = "Easy Card [0x04]";
                        //CardSetPollingTime(portMAX_DELAY);
                        //SetDepositResultStatus(FALSE, 0x01);
                        goto exitFuncion;
                        //break;
                    case 5:
                        //sysprintf("Read_CN: Card Type iPASS [0x%02x]\r\n", rx_buff[5]);
                        pStr = "iPASS [0x05]";
                        break;
                    default:
                        sysprintf("\r\n => Read_CN warning(Card Type Other !!!! [0x%02x]) : ", rx_buff[5]);
                        {
                            int i, len;
                            len = rx_buff[(msgLen-3)];
                            memcpy(data_cn, rx_buff+6, len); 
                            
                            if(len != 0)
                            {
                                sysprintf("\r\n [ ");
                                for(i = 0; i<len; i++)
                                {
                                    data_cn[i]=rx_buff[6+i];
                                    sysprintf("0x%02x ",data_cn[i]);
                                } 
                                sysprintf("]\r\n");
                            }
                            
                        }
                        //CardSetPollingTime(portMAX_DELAY);
                        //SetDepositResultStatus(FALSE, 0x06);
                        goto exitFuncion;
                }
                lrcValue = LRC(rx_buff, 5, msgLen-1);
                //??? need check??
                //sysprintf("Read_CN: LRC [0x%02x]  compare 0x%02x \r\n", lrcValue, rx_buff[(msgLen-3)]); ???
                sysprintf("\r\n => Read_CN (%s) : [0x%02x : 0x%02x : 0x%02x : 0x%02x] \r\n", pStr, rx_buff[6], rx_buff[7], rx_buff[8], rx_buff[9]);
                memcpy(data_cn, rx_buff+6, 4); 
                reVal = TRUE;
            }
             
            break;
        case CARD_MESSAGE_TYPE_TIME:  
             
            if(dataLen != 7)
            {
                sysprintf("Read_TIME: dataLen %d error \r\n", dataLen);
                goto exitFuncion;
            }
            //??LRC
            sysprintf("   => data_time: year %d, Month %d, Day %d, Hour %d, Minute %d, Second %d\n", 
                            rxTemp[6], rxTemp[7], rxTemp[8], rxTemp[9], rxTemp[10], rxTemp[11]);
            
            extern UINT32 sysDOS_Time_To_UTC(DateTime_T ltime);//sys_timer.c
            unix_Time.year  = rxTemp[6] + 2000;
            unix_Time.mon   = rxTemp[7];
            unix_Time.day   = rxTemp[8];
            unix_Time.hour  = rxTemp[9];
            unix_Time.min   = rxTemp[10];
            unix_Time.sec   = sCurTime.u32cSecond;

            utcTime = sysDOS_Time_To_UTC(unix_Time);

            data_time[0]=(char)utcTime;
            data_time[1]=(char)(utcTime>>8);
            data_time[2]=(char)(utcTime>>16);
            data_time[3]=(char)(utcTime>>24);            
            reVal = TRUE;

            break;
            
            
        case CARD_MESSAGE_TYPE_ICD:  
            if(msgLen != dataLen + 7)
            {
                sysprintf("Read_ICD: len error \n");
                //CardSetPollingTime(0);
                goto exitFuncion;
            }
            else
            {                
                lrcValue = LRC(rx_buff, 5, dataLen-1);
                //sysprintf("Read_ICD: LRC [0x%02x]  compare 0x%02x \r\n", lrcValue, rx_buff[(msgLen-3)]);
                if(dataLen == 1)
                {
                    sysprintf("Read_ICD (1): return error 0x%02x\r\n", rx_buff[5]);
                    //CardSetPollingTime(portMAX_DELAY);
                    //SetDepositResultStatus(FALSE, rx_buff[5]);
                    goto exitFuncion;
                }
                else if(dataLen == 2)
                {
                    //uint16_t errorcode;
                    //errorcode = rx_buff[5]|(rx_buff[6]<<8);
                    sysprintf("Read_ICD (2): return error 0x%04x [0x%02x, 0x%02x]\r\n", rx_buff[5]|(rx_buff[6]<<8), rx_buff[5], rx_buff[6]);
                    //CardSetPollingTime(portMAX_DELAY);
                    //SetDepositResultStatus(FALSE, errorcode);
                    goto exitFuncion;
                }
                else if(lrcValue != rx_buff[(msgLen-3)])
                {
                    sysprintf("Read_ICD: LRC error [0x%02x]  compare 0x%02x \r\n", lrcValue, rx_buff[(msgLen-3)]);
                    //CardSetPollingTime(0);
                    goto exitFuncion;
                }
                else
                {                    
                    memset(data_icd, 0x0, sizeof(data_icd));
                    if(dataLen < sizeof(data_icd))
                        memcpy(data_icd, rx_buff+5, dataLen);
                    else
                        memcpy(data_icd, rx_buff+5, sizeof(data_icd));
                    
                    sysprintf("ICD money:[%d, %d] --->\r\n", 
                            data_icd[18] | (data_icd[19]<<8)  | (data_icd[20]<<16)  | (data_icd[21]<<24),
                            data_icd[22] | (data_icd[23]<<8)  | (data_icd[24]<<16)  | (data_icd[25]<<24));
                    
                    #if(0)
                    sysprintf("\r\n--- ICD [%d] --->\r\n", dataLen);
                    for(i = 0; i<dataLen; i++)
                    {
                        //sysprintf(" [%03d] : 0x%02x \r\n", i, rx_buff[5+i]);
                        sysprintf("0x%02x, ", rx_buff[5+i]);
                        if(i%10 == 9)
                            sysprintf("\r\n");

                    }
                    sysprintf("\r\n<--- ICD ---\r\n");
                    #endif
                    reVal = TRUE;
                }
            }
        break;
            
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE:
            if(msgLen != dataLen + 7)
            {
                sysprintf("iPass_Execute: len error \n");
                //CardSetPollingTime(0);
                goto exitFuncion;
            }
            else
            {                
                lrcValue = LRC(rx_buff, 5, dataLen-1);
                //sysprintf("iPass_Execute: LRC [0x%02x]  compare 0x%02x \r\n", lrcValue, rx_buff[(msgLen-3)]);
                if(dataLen == 1)
                {
                    sysprintf("iPass_Execute (1): return error 0x%02x\r\n", rx_buff[5]);
                    //CardSetPollingTime(portMAX_DELAY);
                    //SetDepositResultStatus(FALSE, rx_buff[5]);
                    goto exitFuncion;
                }
                else if(dataLen == 2)
                {
                    //uint16_t errorcode;
                    //errorcode = rx_buff[5]|(rx_buff[6]<<8);
                    sysprintf("iPass_Execute (2): return error 0x%04x [0x%02x, 0x%02x]\r\n", rx_buff[5]|(rx_buff[6]<<8), rx_buff[5], rx_buff[6]);
                    //CardSetPollingTime(portMAX_DELAY);
                   // SetDepositResultStatus(FALSE, errorcode);
                    goto exitFuncion;
                }
                else if(lrcValue != rx_buff[(msgLen-3)])
                {
                    int i;
                    sysprintf("iPass_Execute: LRC error [0x%02x]  compare 0x%02x \r\n", lrcValue, rx_buff[(msgLen-3)]);
                    sysprintf("\r\n--- iPass_Execute [%d] Dump--->\r\n", dataLen);
                    for(i = 0; i<dataLen; i++)
                    {
                        //sysprintf(" [%03d] : 0x%02x \r\n", i, rx_buff[5+i]);
                        sysprintf("0x%02x, ", rx_buff[5+i]);
                        if(i%10 == 9)
                            sysprintf("\r\n");

                    }
                    sysprintf("\r\n<--- iPass_Execute ---\r\n");
                    //CardSetPollingTime(0);
                    goto exitFuncion;
                }
                else
                {                    
                    int money;                    
                    #if(0)
                    {
                        int i;
                        sysprintf("\r\n--- iPass_Execute [%d] --->\r\n", dataLen);
                        for(i = 0; i<dataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, rx_buff[5+i]);
                            sysprintf("0x%02x, ", rx_buff[5+i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- iPass_Execute ---\r\n");   
                    }   
                    #endif
                    money=rx_buff[5]+(rx_buff[6]<<8)+(rx_buff[7]<<16)+(rx_buff[8]<<24);
                    sysprintf("iPass_Execute: Balance Money = %d\n",  money);
                    //CardSetPollingTime(portMAX_DELAY);
                    //SetDepositResultStatus(TRUE, money);
                    reVal = TRUE;
                }
            }
            break;
        default:
            break;
    }
    
exitFuncion:
    return reVal;
}
static BOOL Read_Time(void)
{
    /*
    Run Card Read_Time()
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
    data_time: 0x10(8) 0x16(17) 0x1(75) 0xA918(1)
    
    0xEA 0x01 0x02 0x00 0x07 X1 X2 X3 X4 X5 X6 X7 0x90 0x00
    X1:Day of Week
    X2:Year
    X3:Month
    X4:Day
    X5:Hour
    X6:Minute
    X7:Second

    */
    int nret;
    BOOL reVal = FALSE;
    int count=0;
    //sysprintf("\r\nRun Card Read_Time()\n");
    
    CardFlushBuffer();
    
    nret = uart_w(read_time,sizeof(read_time));
    if(nret != sizeof(read_time))
    {
        sysprintf("Read_Time() uart_w size error [%d: %d]\n", nret, sizeof(read_time));
    }
    else
    {
        //vTaskDelay(10/portTICK_RATE_MS); 
        count=uart_r();
        
        if(count == 0)
        {
            
        }
        else
        {
           reVal = parserMessage(CARD_MESSAGE_TYPE_TIME, rx_buff, count);
        }
    }

    return reVal;    

}



static BOOL Read_CN(void)
{ 
    int nret;
    BOOL reVal = FALSE;
    int count=0;
    //sysprintf("\r\nRun Card Read_CN(), errorTimes = %d\n", errorTimes);
    
    CardFlushBuffer();    
    
    nret = uart_w(read_cn,sizeof(read_cn));
    if(nret != sizeof(read_cn))
    {
        sysprintf("Read_CN() uart_w size error [%d: %d]\n", nret, sizeof(read_cn));
    }
    else
    {
        //vTaskDelay(10/portTICK_RATE_MS);     
        count=uart_r();
        //sysprintf("we get %d char! @ Read_CN\n",count);
        if(count == 0)
        {        
        }
        else
        {
           reVal = parserMessage(CARD_MESSAGE_TYPE_CN, rx_buff, count);
        }
    }
    
    return reVal;

}


static BOOL Read_IVN(void)
{
    int nret;
    int count=0;
    //sysprintf("\r\nRun Card Read_IVN()\n");
    CardFlushBuffer();
    nret = uart_w(read_ivn,sizeof(read_ivn));
    if(nret != sizeof(read_ivn))
    {
        sysprintf("Read_IVN() uart_w size error [%d: %d]\n", nret, sizeof(read_ivn));
    }
    else
    {
        count=uart_r();
        if(count != 0)
        {
            sysprintf(" => IVN [0x%02x, 0x%02x]:%d.%d.%d.%d\n", rx_buff[5], rx_buff[6], (rx_buff[5]>>4)&0xf, rx_buff[5]&0xf, (rx_buff[6]>>4)&0xf, rx_buff[6]&0xf);
            return TRUE;
        }
    }
    return FALSE;
}



static BOOL Read_ICD(void)
{
    /*
Data[0] we read is 0xEA!
Data[1] we read is 0x05!
Data[2] we read is 0x01!
Data[3] we read is 0x00!
Data[4] we read is 0x87!
Data[5] we read is 0x25!
Data[6] we read is 0x9B!
Data[7] we read is 0xDB!
Data[8] we read is 0x9B!
Data[9] we read is 0xFE!
Data[10] we read is 0x88!
Data[11] we read is 0x04!
Data[12] we read is 0x00!
Data[13] we read is 0x43!
Data[14] we read is 0x27!

--- ICD [135] --->
0x25, 0x9B, 0xDB, 0x9B, 0xFE, 0x88, 0x04, 0x00, 0x43, 0x27, 
0xBB, 0xD2, 0x00, 0x15, 0x0B, 0x07, 0x0D, 0x02, 0x63, 0x23, 
0x00, 0x00, 0x63, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x52, 0xDD, 0x56, 
0x01, 0x23, 0x00, 0x63, 0x23, 0x07, 0x00, 0x01, 0x00, 0x00, 
0x00, 0x00, 0xD3, 0x06, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x52, 
0xDD, 0x56, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xD1, 0x7C, 0xA8, 0x66, 0x4D, 
<--- ICD ---

    
    */
    int nret;
    BOOL reVal = FALSE;
    int count=0;
    UINT8 read_idata[16]= {0};
    
    CardFlushBuffer();
    
    //Get_Time();
    read_idata[0]=0xea;
    read_idata[1]=0x05;
    read_idata[2]=0x01;
    read_idata[3]=0x00;
    read_idata[4]=0x09;
    read_idata[5]=data_cn[0];
    read_idata[6]=data_cn[1];
    read_idata[7]=data_cn[2];
    read_idata[8]=data_cn[3];
    read_idata[9]=data_time[0];
    read_idata[10]=data_time[1];
    read_idata[11]=data_time[2];
    read_idata[12]=data_time[3];
    read_idata[13]=LRC(read_idata,5,8);
    read_idata[14]=0x90;
    read_idata[15]=0x00;
    nret = uart_w(read_idata,sizeof(read_idata)); 
    if(nret != sizeof(read_idata))
    {
        sysprintf("Read_ICD() uart_w size error [%d: %d]\n", nret, sizeof(read_idata));
    }
    else
    {
        //vTaskDelay(10/portTICK_RATE_MS); 
        count=uart_r();
        //sysprintf("we get %d char!@ Read_ICD\n",count);
        if(count == 0)
        {
        }
        else
        {
           reVal = parserMessage(CARD_MESSAGE_TYPE_ICD, rx_buff, count);
        }    
    }

    return reVal;
}


static BOOL iPass_Execute(char CmdType, uint16_t deduct)
{
    //0xEA, 0x05, 0x02, 0x00, 0x02, 
    //0x5A, 0x02, 
    //0x90, 0x00,
    int nret;
    BOOL reVal = FALSE;
    //int money;
    //int deduct_value;
    int count;
    UINT8 deduct_idata[47]= {0};
    //Get_Time();
    CardFlushBuffer();

    deduct_idata[0] =0xEA;
    deduct_idata[1] =0x05;
    deduct_idata[2] =0x02;
    deduct_idata[3] =0x00;
    deduct_idata[4] =0x28;
    deduct_idata[5] =data_icd[0];    //x16 ok
    deduct_idata[6] =data_icd[1];
    deduct_idata[7] =data_icd[2];
    deduct_idata[8] =data_icd[3];
    deduct_idata[9] =data_icd[4];
    deduct_idata[10]=data_icd[5];
    deduct_idata[11]=data_icd[6];
    deduct_idata[12]=data_icd[7];
    deduct_idata[13]=data_icd[8];
    deduct_idata[14]=data_icd[9];
    deduct_idata[15]=data_icd[10];
    deduct_idata[16]=data_icd[11];
    deduct_idata[17]=data_icd[12];
    deduct_idata[18]=data_icd[13];
    deduct_idata[19]=data_icd[14];
    deduct_idata[20]=data_icd[15];
    deduct_idata[21]=deduct&0xff;//0x01;      //x2
    deduct_idata[22]=(deduct>>8)&0xff;
    deduct_idata[23]=data_time[0];      //x4 Little Endian
    deduct_idata[24]=data_time[1];
    deduct_idata[25]=data_time[2];
    deduct_idata[26]=data_time[3];
    deduct_idata[27]=0x20;      //x1 

    if(CmdType =='D')
        sysprintf("we get the char!\n");

    if (CmdType == 'R')
    {
        deduct_idata[27] = 0x90;
    }
    else if (CmdType == 'L')
    {
        deduct_idata[27] = 0x91;
    }

    deduct_idata[28]=0xa1;
    deduct_idata[29]=0x2d;
    deduct_idata[30]=0x00;
    deduct_idata[31]=0x00;
    deduct_idata[32]=data_mn[0];
    deduct_idata[33]=data_mn[1];
    deduct_idata[34]=data_mn[2];
    deduct_idata[35]=data_mn[3];
    deduct_idata[36]=0x00;
    deduct_idata[37]=0x00;
    deduct_idata[38]=0x00;
    deduct_idata[39]=0x00;
    deduct_idata[40]=0x00;
    deduct_idata[41]=0x00;
    deduct_idata[42]=0x00;
    deduct_idata[43]=0x00;
    deduct_idata[44]=LRC(deduct_idata,5,39); //LRC(0-38)
    deduct_idata[45]=0x90;
    deduct_idata[46]=0x00;

    sysprintf("iPass_Execute() deduct_value is %d (%d) \n", deduct_idata[21] + (deduct_idata[22]<<8), deduct);
    
    nret = uart_w(deduct_idata,sizeof(deduct_idata));
    if(nret != sizeof(deduct_idata))
    {
        sysprintf("iPass_Execute() uart_w size error [%d: %d]\n", nret, sizeof(deduct_idata));
    }
    else
    {
        //vTaskDelay(10/portTICK_RATE_MS); 
        count=uart_r();
        //sysprintf("we get %d char!@ iPass_Execute\n",count);
        if(count == 0)
        {
        }
        else
        {
           reVal = parserMessage(CARD_MESSAGE_TYPE_IPASS_EXECUTE, rx_buff, count);
        }    
    }
    return reVal;


}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL TS1000DrvInit(void)
{
    sysprintf("TS1000DrvInit!!\n");
    pUartInterface = UartGetInterface(TS1000_DRV_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("TS1000DrvInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    if(pUartInterface->initFunc(57600) == FALSE)
    {
        sysprintf("TS1000DrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }

    sysprintf("TS1000DrvInit OK!!\n");
    return TRUE;
}
BOOL TS1000SetPower(BOOL flag)
{
    if(flag)
    {
        powerUpTick = xTaskGetTickCount();
    }
    else
    {
        powerUpTick = 0; 
    }
    return pUartInterface->setPowerFunc(flag);
}
uint8_t TS1000CheckReader(void)
{
    uint8_t reVal = TSREADER_CHECK_READER_OK;
    int times = CARD_INIT_TIMEOUT_TIME / CARD_INIT_TIMEOUT_INTERVAL;
    sysprintf(" !!! TS1000CheckReader enter  !!!...\r\n");
    checkReaderFlag = TRUE;
    while((Read_IVN() == FALSE)&&(checkReaderFlag))
    {
        //sysprintf("Read_IVN retry...\r\n"); 
        sysprintf(":");
        vTaskDelay(CARD_INIT_TIMEOUT_INTERVAL/portTICK_RATE_MS);   
        if(times-- < 0)
        {
            sysprintf("Read_IVN break (counter)...\r\n");
            reVal = TSREADER_CHECK_READER_ERROR;
            break;  
        }  
        if((powerUpTick != 0) && ((xTaskGetTickCount() - powerUpTick) > CARD_INIT_TIMEOUT_TIME) )
        {
            sysprintf("Read_IVN break (time:%d)...\r\n", xTaskGetTickCount() - powerUpTick);
            reVal = TSREADER_CHECK_READER_ERROR;
            break;  
        }            
    }

    if(checkReaderFlag == FALSE)
    {//因為break而退出
        reVal = TSREADER_CHECK_READER_BREAK;
        sysprintf("\r\n !!! TS1000CheckReader break [%d] ticeks!!!...\r\n",  xTaskGetTickCount() - powerUpTick);        
    }
    else
    {
        if(reVal == TSREADER_CHECK_READER_ERROR)
        {
            sysprintf("\r\n !!! TS1000CheckReader ERROR [%d] ticeks!!!...\r\n",  xTaskGetTickCount() - powerUpTick);

        }
        else
        {
            sysprintf("\r\n !!! TS1000CheckReader OK [%d] ticeks, times = %d!!!...\r\n",  xTaskGetTickCount() - powerUpTick, times);
        }
    }
    return reVal;
}

BOOL TS1000BreakCheckReader(void)
{
    BOOL reVal = TRUE;
    checkReaderFlag = FALSE;
    return reVal;
}


BOOL TS1000Process(uint16_t targetDeduct)
{   
    BOOL retval = FALSE;
    //sysprintf(" ==> CardReadProcess\n");
    if(Read_CN())
    {
        retval = TRUE;
        if(Read_Time())
        {
            if(Read_ICD())
            {        
                //if(targetDeduct == 0)
                //{
                //    sysprintf(" ==> CardReadProcess ignore, targetDeduct = 0 !!!\n");
                //}     
                //else  
                //{                    
                //    if(iPass_Execute('R', targetDeduct))
                //    {
                //        CardSetPollingTime(portMAX_DELAY);
                        
                //    }
                //}
            }
        }        
    }
    return retval;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


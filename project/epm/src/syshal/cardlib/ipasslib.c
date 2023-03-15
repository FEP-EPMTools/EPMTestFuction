/**************************************************************************//**
* @file     ipasslib.c
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
    
    #include "ipasslib.h"
    #include "misc.h"
    #include "blkcommon.h"
    
    #define sysprintf       miscPrintf//printf
    #define pvPortMalloc    malloc
    #define vPortFree       free
    #define ENABLE_CARD_LOG_SAVE    0
#else
    #include "nuc970.h"
    #include "sys.h"
    #include "rtc.h"
    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "fileagent.h"
    #include "meterdata.h"
    #include "sflashrecord.h"

    #include "fepconfig.h"
    #include "ipasslib.h"
    #include "dataprocesslib.h"
    #include "blkcommon.h"
    #define ENABLE_CARD_LOG_SAVE    1
#endif

#define ENABLE_SHOW_RETURN_DATA     1
#define ENABLE_TX_PRINT             0
#define ENABLE_RX_PRINT             0
#define ENABLE_RX_TIME_PRINT        0

#define CARD_MESSAGE_TYPE_IPASS_ICD                 0x01
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE             0x02
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE_LOCK        0x03
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE_RECOVERY    0x04
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE_ERROR       0x05


/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
static uint8_t icdData[512]= {0};
static int icdDataLen = 0;

static uint8_t executeData[512]= {0};
static int executeDataLen = 0;

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint16_t parserMessage(uint8_t msgType, uint8_t* receiveData, uint16_t receiveDataLen, uint16_t* returnCode)
{    
    uint16_t returnInfo = CARD_MESSAGE_RETURN_SUCCESS;
    
    uint16_t dataLen;
    uint8_t lrcValue;
    uint8_t targetType1, targetType2;
    if(returnCode == NULL)
    {
        sysprintf("\r\n~~~ parserMessage(ECC) msgType = %d[%d] error (returnCode == NULL)~~~>\r\n", msgType, receiveDataLen);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    *returnCode = CARD_MESSAGE_CODE_NO_USE; //錯誤的總稱
    #if(ENABLE_RX_PRINT)
    int i;    
    sysprintf("\r\n~~~ parserMessage(ECC) msgType = %d[%d] ~~~>\r\n", msgType, receiveDataLen);
    for(i = 0; i<receiveDataLen; i++)
    {
        sysprintf("0x%02x, ", receiveData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<~~~ parserMessage(ECC) ~~~\r\n");
    #endif
    
    if(receiveData[0] != 0xEA)
    {
        sysprintf("parserMessage(parser type:%d): header err [0x%02x: 0xEA]\n", msgType, receiveData[0]);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    if((receiveData[receiveDataLen-2] != 0x90) || (receiveData[receiveDataLen-1] != 0x0))
    {
        sysprintf("parserMessage(parser type:%d): end flag err [0x%02x: 0x%02x]\n", msgType, receiveData[receiveDataLen-2], receiveData[receiveDataLen-1]);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    
    switch(msgType)
    {        
        case CARD_MESSAGE_TYPE_IPASS_ICD:
            targetType1 = 0x05;
            targetType2 = 0x01;
            break;
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE:
            targetType1 = 0x05;
            targetType2 = 0x06;
            break;
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE_LOCK:
            targetType1 = 0x05;
            targetType2 = 0x04;
            break;
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE_RECOVERY:
            targetType1 = 0x05;
            targetType2 = 0x08;
            break;        
        default:
            return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    
    
    if((receiveData[1] != targetType1) || (receiveData[2] != targetType2))
    {
        sysprintf("parserMessage(parser type:%d): type err [0x%02x, 0x%02x : 0x%02x, 0x%02x]\n", msgType, receiveData[1], receiveData[2], targetType1, targetType2);
        
        #if(ENABLE_SHOW_RETURN_DATA)
        {
            int i;
            sysprintf("\r\n--- Raw Data [%d] --->\r\n", receiveDataLen);
            for(i = 0; i<receiveDataLen; i++)
            {
                 sysprintf("0x%02x, ", receiveData[i]);
                if(i%10 == 9)
                    sysprintf("\r\n");

            }
            sysprintf("\r\n<--- Raw Data ---\r\n");   
        }   
        #endif
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    dataLen = receiveData[4]|(receiveData[3]<<8);    
    lrcValue = EPMReaderLRC(receiveData, 5, dataLen-1);  
    
    switch(msgType)
    {      
        case CARD_MESSAGE_TYPE_IPASS_ICD:  
            memset(icdData, 0x0, sizeof(icdData));
            if(receiveDataLen != dataLen + 7)
            {
                sysprintf("IPASSLibReadICD: len error \n");
                return CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_ERROR;
            }
            else
            {                
                lrcValue = EPMReaderLRC(receiveData, 5, dataLen-1);
                //sysprintf("IPASSLibReadICD: EPMReaderLRC [0x%02x]  compare 0x%02x \r\n", lrcValue, receiveData[(receiveDataLen-3)]);
                if(dataLen == 1)
                {
                    sysprintf("IPASSLibReadICD (1): return error 0x%02x\r\n", receiveData[5]);

                    *returnCode = receiveData[5];
                    return CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_1;
                }
                else if(dataLen == 2)
                {
                    uint16_t errorcode;
                    errorcode = receiveData[5]|(receiveData[6]<<8);
                    sysprintf("IPASSLibReadICD (2): return error 0x%04x [0x%02x, 0x%02x]\r\n", receiveData[5]|(receiveData[6]<<8), receiveData[5], receiveData[6]);
                    
                    *returnCode = errorcode;
                    return CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_2;
                }
                else if(lrcValue != receiveData[(receiveDataLen-3)])
                {
                    sysprintf("IPASSLibReadICD: EPMReaderLRC error [0x%02x]  compare 0x%02x \r\n", lrcValue, receiveData[(receiveDataLen-3)]);

                    *returnCode = 0x0;
                    return CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LRC_ERROR;
                }
                else
                {                    
                    
                    icdDataLen = dataLen;
                    memcpy(icdData, receiveData+5, icdDataLen);
                   
                    #if(ENABLE_SHOW_RETURN_DATA)
                    sysprintf("\r\n--- ICD [%d] --->\r\n", icdDataLen);
                    for(int i = 0; i<icdDataLen; i++)
                    {
                        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                        sysprintf("0x%02x, ", icdData[i]);
                        if(i%10 == 9)
                            sysprintf("\r\n");

                    }
                    sysprintf("\r\n<--- ICD ---\r\n");
                    #endif
                    
                }
            }
        break;
            
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE:
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE_LOCK:
        case CARD_MESSAGE_TYPE_IPASS_EXECUTE_RECOVERY:
            if(receiveDataLen != dataLen + 7)
            {
                sysprintf("iPass_Execute: len error \n");
                return CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_ERROR;
            }
            else
            {                
                lrcValue = EPMReaderLRC(receiveData, 5, dataLen-1);
                //sysprintf("iPass_Execute: EPMReaderLRC [0x%02x]  compare 0x%02x \r\n", lrcValue, receiveData[(receiveDataLen-3)]);
                if(dataLen == 1)
                {
                    sysprintf("iPass_Execute (1): return error 0x%02x\r\n", receiveData[5]);

                    *returnCode = receiveData[5];
                    return CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_1;
                }
                else if(dataLen == 2)
                {
                    uint16_t errorcode;
                    errorcode = receiveData[5]|(receiveData[6]<<8);
                    sysprintf("iPass_Execute (2): return error 0x%04x [0x%02x, 0x%02x]\r\n", receiveData[5]|(receiveData[6]<<8), receiveData[5], receiveData[6]);

                    *returnCode = errorcode;
                    return CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_2;
                }
                else if(lrcValue != receiveData[(receiveDataLen-3)])
                {
                    int i;
                    sysprintf("iPass_Execute: EPMReaderLRC error [0x%02x]  compare 0x%02x \r\n", lrcValue, receiveData[(receiveDataLen-3)]);
                    sysprintf("\r\n--- iPass_Execute [%d] Dump--->\r\n", dataLen);
                    for(i = 0; i<dataLen; i++)
                    {
                        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                        sysprintf("0x%02x, ", receiveData[5+i]);
                        if(i%10 == 9)
                            sysprintf("\r\n");

                    }
                    sysprintf("\r\n<--- iPass_Execute ---\r\n");
                    
                    *returnCode = 0x0;
                    return CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LRC_ERROR;
                }
                else
                {                    
                    int money;     
                    memset(executeData, 0x0, sizeof(executeData));
                    executeDataLen = dataLen;
                    memcpy(executeData, receiveData+5, executeDataLen);
                   
                    #if(ENABLE_SHOW_RETURN_DATA)
                    {
                        int i;
                        sysprintf("\r\n--- iPass_Execute [%d] --->\r\n", executeDataLen);
                        for(i = 0; i<executeDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", executeData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- iPass_Execute ---\r\n");   
                    }   
                    #endif
                    money=receiveData[5]+(receiveData[6]<<8)+(receiveData[7]<<16)+(receiveData[8]<<24);
                    //sysprintf("\r\n=>  [[[   iPass_Execute: Balance Money = %d   ]]] \n",  money);
                    //CardSetPollingTime(portMAX_DELAY);
                    //setcallBackValue(TRUE, money);
                    *returnCode = money;
                }
            }
            break;
        default:
            break;
    }    

    return returnInfo;
}


static uint16_t readICD(uint16_t* returnCode, uint8_t* cnData, uint8_t* dataTime)
{
    uint16_t returnInfo;
    uint8_t* receiveData;
    uint16_t receiveDataLen;

    UINT8 icdCmd[16]= {0};    
    
    //Get_Time();
    icdCmd[0]=0xea;
    icdCmd[1]=0x05;
    icdCmd[2]=0x01;
    icdCmd[3]=0x00;
    icdCmd[4]=0x09;
    icdCmd[5]=cnData[0];
    icdCmd[6]=cnData[1];
    icdCmd[7]=cnData[2];
    icdCmd[8]=cnData[3];
    icdCmd[9]=dataTime[0];
    icdCmd[10]=dataTime[1];
    icdCmd[11]=dataTime[2];
    icdCmd[12]=dataTime[3];
    icdCmd[13]=EPMReaderLRC(icdCmd,5,8);
    icdCmd[14]=0x90;
    icdCmd[15]=0x00;
    
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd(icdCmd,sizeof(icdCmd));
    if(nret != sizeof(icdCmd))
    {
        sysprintf("readICD() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(icdCmd));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        //vTaskDelay(10/portTICK_RATE_MS); 
        int count=EPMReaderReceiveCmd(300, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char!@ readICD\n",count);
        if(count == 0)
        {
            sysprintf("readICD() receiveReaderCmd error\n");
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;              
        }
        else
        {            
            returnInfo = parserMessage(CARD_MESSAGE_TYPE_IPASS_ICD, receiveData, receiveDataLen, returnCode);
        }    
    }

    return returnInfo;
}




static uint16_t executeDeduct(char CmdType, uint16_t deduct, uint16_t* returnCode, uint8_t* icdData, uint8_t* dataTime, uint8_t* machineNo)
{
    uint16_t returnInfo = CARD_MESSAGE_RETURN_OTHER_ERROR;
    uint8_t* receiveData;
    uint16_t receiveDataLen;
    
    uint8_t executeCmd[47]= {0};

    executeCmd[0] =0xEA;
    executeCmd[1] =0x05;

    if(CmdType =='D')
    {
        executeCmd[2] =0x06;
    }
    else if (CmdType == 'R')
    {
        executeCmd[2] = 0x08;
    }
    else if (CmdType == 'L')
    {
        executeCmd[2] = 0x04;
    }
    executeCmd[3] =0x00;
    executeCmd[4] =0x28;
    executeCmd[5] =icdData[0];    //x16 ok
    executeCmd[6] =icdData[1];
    executeCmd[7] =icdData[2];
    executeCmd[8] =icdData[3];
    executeCmd[9] =icdData[4];
    executeCmd[10]=icdData[5];
    executeCmd[11]=icdData[6];
    executeCmd[12]=icdData[7];
    executeCmd[13]=icdData[8];
    executeCmd[14]=icdData[9];
    executeCmd[15]=icdData[10];
    executeCmd[16]=icdData[11];
    executeCmd[17]=icdData[12];
    executeCmd[18]=icdData[13];
    executeCmd[19]=icdData[14];
    executeCmd[20]=icdData[15];
    executeCmd[21]=deduct&0xff;//0x01;      //x2
    executeCmd[22]=(deduct>>8)&0xff;
    executeCmd[23]=dataTime[0];      //x4 Little Endian
    executeCmd[24]=dataTime[1];
    executeCmd[25]=dataTime[2];
    executeCmd[26]=dataTime[3];
    

    if(CmdType =='D')
    {
        executeCmd[27]=0x24;
    }
    else if (CmdType == 'R')
    {
        executeCmd[27] = 0x90;
    }
    else if (CmdType == 'L')
    {
        executeCmd[27] = 0x91;
    }
    #define SYSTEM_ID       0x41
    #define LOCATION_ID     0x2d
    executeCmd[28]=SYSTEM_ID;    
    executeCmd[29]=LOCATION_ID; 
    executeCmd[30]=0x00;         
    executeCmd[31]=0x00;
    executeCmd[32]=machineNo[0];   
    executeCmd[33]=machineNo[1];
    executeCmd[34]=machineNo[2];
    executeCmd[35]=machineNo[3];
    executeCmd[36]=0x01;           
    executeCmd[37]=0x01;           
    executeCmd[38]=0x00;          
    executeCmd[39]=0x00;          
    executeCmd[40]=0x00;
    executeCmd[41]=0x00;
    executeCmd[42]=0x00;
    executeCmd[43]=0x00;
    executeCmd[44]=EPMReaderLRC(executeCmd,5,39); //EPMReaderLRC(0-38)
    executeCmd[45]=0x90;
    executeCmd[46]=0x00;

    sysprintf("executeDeduct() deduct_value is %d (%d) \n", executeCmd[21] + (executeCmd[22]<<8), deduct);
    
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd(executeCmd,sizeof(executeCmd));
    if(nret != sizeof(executeCmd))
    {
        sysprintf("executeDeduct() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(executeCmd));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        uint8_t exeType = CARD_MESSAGE_TYPE_IPASS_EXECUTE_ERROR;
        if(CmdType =='D')
        {
            exeType = CARD_MESSAGE_TYPE_IPASS_EXECUTE;
        }
        else if (CmdType == 'R')
        {
            exeType = CARD_MESSAGE_TYPE_IPASS_EXECUTE_RECOVERY;
        }
        else if (CmdType == 'L')
        {
            exeType = CARD_MESSAGE_TYPE_IPASS_EXECUTE_LOCK;
        }
        
        if(exeType != CARD_MESSAGE_TYPE_IPASS_EXECUTE_ERROR)
        {
            //vTaskDelay(10/portTICK_RATE_MS); 
            int count=EPMReaderReceiveCmd(200, &receiveData, &receiveDataLen);
            //sysprintf("we get %d char!@ executeDeduct\n",count);
            if(count == 0)
            {
                sysprintf("executeDeduct() receiveReaderCmd error\n"); 
                returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;
            }
            else
            {
               returnInfo = parserMessage(exeType, receiveData, receiveDataLen, returnCode);
            } 
        }        
    }
    return returnInfo;
}


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL IPASSLibProcess(uint16_t* returnInfo, uint16_t* returnCode, uint16_t targetDeduct, tsreaderDepositResultCallback callback, uint8_t* cnData, uint8_t* dataTime, uint8_t* machineNo)
{
    BOOL retval = FALSE;
    //uint16_t returnCode;
    //uint16_t returnInfo;
    char title[64];
    uint32_t cnValue = 0;
    tsreaderDepositResultCallback  ptsreaderDepositResultCallback = callback;
    cnValue = cnData[0]<<24 | cnData[1]<<16 | cnData[2]<<8 | cnData[3]<<0;

    if(IPASSBLKSearchTargetID(cnValue) == -1)
    {
        //sysprintf("============= OK : NOT IN BLK  ============== \r\n");
        *returnInfo = readICD(returnCode, cnData, dataTime);
        if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
        {        
            //icd[51] : BYTE syncStatus;    //15同步狀態
            //0表無須同步票值，1表主蓋備，2表備蓋主 52
            if (icdData[51] == 0x00)
            {
                if(targetDeduct == 0)
                {
                    sysprintf(" ==> CardReadProcess ignore, targetDeduct = 0 !!!\n");
                }     
                else  
                {         

                    *returnInfo = executeDeduct('D', targetDeduct, returnCode, icdData, dataTime, machineNo);
                    if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
                    {
                        if(ptsreaderDepositResultCallback != NULL)
                            ptsreaderDepositResultCallback(TRUE, *returnInfo, *returnCode);
                        retval = TRUE;                            
                    }
                    else //*returnInfo = executeDeduct('D', targetDeduct, returnCode);
                    {
                        sysprintf(" ==> executeDeduct return FLASE: errorCode = 0x%04x(%d),  *returnInfo = 0x%02x !!!\n", *returnCode, *returnCode, *returnInfo);
                        switch(*returnInfo)
                        {
                            case CARD_MESSAGE_RETURN_PARSER_ERROR:
                            case CARD_MESSAGE_RETURN_TIMEOUT:
                            case CARD_MESSAGE_RETURN_SEND_ERROR:
                            case CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LRC_ERROR:
                            case CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_ERROR:
                                break;

                            case CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_1:
                                if(ptsreaderDepositResultCallback != NULL)
                                {
                                    ptsreaderDepositResultCallback(FALSE, *returnInfo, *returnCode);
                                }

                                sprintf(title, "EXE(0x%02x:%d)", *returnInfo, *returnCode);
                                DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);

                                retval = TRUE; //完成 離開刷卡 loop
                                break;
                            case CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_2:
                                if(ptsreaderDepositResultCallback != NULL)
                                {
                                    ptsreaderDepositResultCallback(FALSE, *returnInfo, *returnCode);
                                }

                                sprintf(title, "EXE(0x%02x:%d)", *returnInfo, *returnCode);
                                DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);

                                retval = TRUE; //完成 離開刷卡 loop
                                break;


                        }   

                    } //*returnInfo = executeDeduct('D', targetDeduct, returnCode);
                }
            }
            else //if (icdData[51] == 0x00)
            {
                #warning recovery card  
                *returnInfo = executeDeduct('R', targetDeduct, returnCode, icdData, dataTime, machineNo); 
                sysprintf("****  [INFO Card Reader] Recovery : errorCode = 0x%04x(%d),  *returnInfo = 0x%02x !!!\n", *returnCode, *returnCode, *returnInfo);

                sprintf(title, "RECOVERY(0x%02x:%d)", *returnInfo, *returnCode);
                DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);                    

            } //if (icdData[51] == 0x00)
        }   
        else //*returnInfo = readICD(returnCode);
        {
            sysprintf(" ==> readICD return FLASE: errorCode = 0x%04x(%d),  *returnInfo = 0x%02x !!!\n", *returnCode, *returnCode, *returnInfo);
            switch(*returnInfo)
            {
                case CARD_MESSAGE_RETURN_PARSER_ERROR:
                case CARD_MESSAGE_RETURN_TIMEOUT:
                case CARD_MESSAGE_RETURN_SEND_ERROR:
                case CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LRC_ERROR:
                case CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_ERROR:
                    sysprintf(":"); 
                    break;   

                case CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_1:
                    if(ptsreaderDepositResultCallback != NULL)
                    {
                        ptsreaderDepositResultCallback(FALSE, *returnInfo, *returnCode);
                    }

                    sprintf(title, "ICD(0x%02x:%d)", *returnInfo, *returnCode);
                    DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);

                    retval = TRUE; //完成 離開刷卡 loop
                    break;
                case CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_2:
                    if(ptsreaderDepositResultCallback != NULL)
                    {
                        ptsreaderDepositResultCallback(FALSE, *returnInfo, *returnCode);
                    }

                    sprintf(title, "ICD(0x%02x:%d)", *returnInfo, *returnCode);
                    DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);

                    retval = TRUE; //完成 離開刷卡 loop
                    break;
            }

        } //*returnInfo = readICD(returnCode);
    }
    else
    {
        //sysprintf("============= ERROR : IN BLK  ============== \r\n");
        *returnInfo = CARD_MESSAGE_TYPE_IPASS_RETURN_IN_BLACK;
        *returnCode = CARD_MESSAGE_CODE_NO_USE;
        
        sprintf(title, "CN(0x%02x:%d)", *returnInfo, *returnCode);
        DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);
                
        #warning lock card   
        //returnInfo = iPass_Execute('L', targetDeduct, &returnCode);
        // // 票值回復後要再繼續執行 Read Card Basic，但卡號為 4 Bytes 的卡號。?????
        //sysprintf("****  [INFO Card Reader] Lock Card : errorCode = 0x%04x(%d),  returnInfo = 0x%02x !!!\n", *returnInfo, *returnCode, *returnInfo);  
                
        if(ptsreaderDepositResultCallback != NULL)
        {
            ptsreaderDepositResultCallback(FALSE, *returnInfo, *returnCode);
        }
                
        sprintf(title, "LOCK(0x%02x:%d)", *returnInfo, *returnCode);
        DataProcessSendStatusData(0, title, WEB_POST_EVENT_ALERT);                

        retval = TRUE; //完成 離開刷卡 loop 
        
    }

    
    return retval;
}

void IPASSSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue, uint16_t currentTargetDeduct, uint8_t* readerId, char* dataStr, char* timeStr)
{  
#if(1)
    
    uint8_t* pLogBody  = (uint8_t*)pvPortMalloc(sizeof(IPassDPTIBody));
    IPASSLogDPTIContainInit(pLogBody, 'D', icdData, icdDataLen, executeData, executeDataLen, currentTargetDeduct, readerId, sizeof(readerId), dataStr, timeStr, (char*)"0024");

#if(ENABLE_CARD_LOG_SAVE)
    char targetLogFileName[_MAX_LFN];
    sprintf(targetLogFileName,"%08d_%04d%02d%02d%02d%02d%02d.%s", GetMeterData()->epmid, pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond, IPASS_DPTI_FILE_EXTENSION_SFLASH); 

    SFlashAppendRecord(targetLogFileName, SFLASH_RECORD_TYPE_IPASS, (uint8_t*)pLogBody, sizeof(IPassDPTIBody));
    
    sprintf(targetLogFileName,"%08d_%04d%02d%02d%02d%02d%02d.%s", GetMeterData()->epmid, pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond, IPASS_DPTI_FILE_EXTENSION); 
    //FileAgentReturn FileAgentAddData(StorageType storageType, char* dir, char* name, uint8_t* data, int dataLen, FileAgentAddType addType, BOOL dataNeedFreeFlag, BOOL blockFlag, BOOL checkMode)
    if(FileAgentAddData(IPASS_DPTI_FILE_SAVE_POSITION, IPASS_DPTI_FILE_DIR, targetLogFileName, (uint8_t*)pLogBody, sizeof(IPassDPTIBody), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
    {
        //sysprintf("\r\n=>  [DFCI(%d):%s] \n",  sizeof(IPassDPTIBody), &dftiBody);
    }    
    char targetDSFFileName[_MAX_LFN];
    sprintf(targetDSFFileName,"%08d_%04d%02d%02d%02d%02d%02d.%s", GetMeterData()->epmid, pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond, DSF_FILE_EXTENSION); 
    uint8_t* picdData  = pvPortMalloc(icdDataLen);
    if(picdData != NULL)
    {
        memcpy(picdData, icdData, icdDataLen);
        if(FileAgentAddData(DSF_FILE_SAVE_POSITION, DSF_FILE_DIR, targetDSFFileName, (uint8_t*)picdData, icdDataLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
        {

        }
    }    
    char targetDCFFileName[_MAX_LFN];
    sprintf(targetDCFFileName,"%08d_%04d%02d%02d%02d%02d%02d.%s", GetMeterData()->epmid, pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond, DCF_FILE_EXTENSION); 
    uint8_t* pexecuteData  = pvPortMalloc(executeDataLen);
    if(pexecuteData != NULL)
    {
        memcpy(pexecuteData, executeData, executeDataLen);
        if(FileAgentAddData(DCF_FILE_SAVE_POSITION, DCF_FILE_DIR, targetDCFFileName, (uint8_t*)pexecuteData, executeDataLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
        {

        }
    } 
#endif //#if(ENABLE_CARD_LOG_SAVE)
     #endif
}

void IPASSSaveFilePure(uint16_t currentTargetDeduct, uint8_t* readerId, char* dataStr, char* timeStr)
{    
//#if(ENABLE_CARD_LOG_SAVE)
    uint8_t* pLogBody  = (uint8_t*)pvPortMalloc(sizeof(IPassDPTIBody));
    IPASSLogDPTIContainInit(pLogBody, 'D', icdData, icdDataLen, executeData, executeDataLen, currentTargetDeduct, readerId, sizeof(readerId), dataStr, timeStr, (char*)"0024");
   
    
    vPortFree(pLogBody);   
//#endif
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


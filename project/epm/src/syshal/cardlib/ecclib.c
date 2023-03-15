/**************************************************************************//**
* @file     ecclib.c
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
    #include "ecclib.h"
    #include "ecccmd.h"
    #include "ecclog.h"
    #include "eccblk.h"
    
    #define sysprintf       miscPrintf//printf
    #define pvPortMalloc    malloc
    #define vPortFree       free
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
    #include "ecclib.h"
    #include "ecccmd.h"
    #include "ecclog.h"
    #include "eccblk.h"
    #include "quentelmodemlib.h"
    #include "loglib.h"
    #include "dataprocesslib.h"
    //#include "scdrv.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

#define ENABLE_ECC_LOCK_CARD    0//1

#define ENABLE_SHOW_RETURN_DATA     0
#define ENABLE_TX_PRINT             0
#define ENABLE_RX_PRINT             0//0
#define ENABLE_RX_TIME_PRINT        0

#define CARD_MESSAGE_TYPE_ECC_PPR_RESET             0x01
#define CARD_MESSAGE_TYPE_ECC_DCA_READ              0x02
#define CARD_MESSAGE_TYPE_ECC_EDCA_DEDUCT           0x03
#define CARD_MESSAGE_TYPE_ECC_LOCK_CARD             0x04
#define CARD_MESSAGE_TYPE_ECC_SIGN_ON_QUERY         0x05
#define CARD_MESSAGE_TYPE_ECC_SIGN_ON               0x06
#define ENABLE_ECC_LOG_MESSAGE      0//1
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint32_t serialNumber = 1;

static ECCCmdPPRResetResponseData               eccCmdPPRResetResponseData;

static ECCCmdDCAReadResponseSuccessData         eccCmdDCAReadResponseSuccessData;
static ECCCmdDCAReadResponseError1Data          eccCmdDCAReadResponseError1Data;
static ECCCmdDCAReadResponseError2Data          eccCmdDCAReadResponseError2Data;

static ECCCmdEDCADeductResponseData             eccCmdEDCADeductResponseData;
static ECCCmdLockCardResponseData               eccCmdLockCardResponseData;

static ECCCmdPPRSignOnQueryResponseData         eccCmdPPRSignOnQueryResponseData;
static ECCCmdPPRSignOnResponseData              eccCmdPPRSignOnResponseData;

#if(ENABLE_ECC_AUTO_LOAD)
//socket
static ECCCmdSignOnResponseSocketData           eccCmdSignOnResponseSocketData;
static ECCCmdSignOnConfirmResponseSocketData    eccCmdSignOnConfirmResponseSocketData;
static BOOL eccRemaiderAmt = FALSE;
#endif



//2018.08.14  --> A. 【SEQ_NO_BEF_TXN】交易前序號：扣款兼自動加值，應填自動加值後的序號，卻填自動加值前序號。
//2018.08.14  --> B. 【EV_BEF_TXN】交易前卡片金額：扣款兼自動加值，應填自動加值後的餘額，卻填自動加值前餘額。 
static BOOL cardAutoloadAvailable = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static uint16_t parserMessage(uint8_t msgType, uint8_t* receiveData, uint16_t receiveDataLen, uint16_t* returnCode, uint16_t* resultStatus)
{    
    uint16_t returnInfo = CARD_MESSAGE_RETURN_SUCCESS;
    
    uint16_t dataLen;
    uint8_t lrcValue;
    uint8_t targetType1, targetType2;
    if(returnCode == NULL)
    {
        sysprintf("\r\n~~~ parserMessage(ECC) receiveDataLen = %d error (returnCode == NULL)~~~>\r\n", receiveDataLen);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    *returnCode = CARD_MESSAGE_CODE_NO_USE; 
    #if(ENABLE_RX_PRINT)
    int i;    
    sysprintf("\r\n~~~ parserMessage(ECC) [Len: %d] ~~~>\r\n", receiveDataLen);
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
        sysprintf("parserMessage(ECC): header err [0x%02x: 0xEA]\n", receiveData[0]);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    if((receiveData[receiveDataLen-2] != 0x90) || (receiveData[receiveDataLen-1] != 0x0))
    {
        sysprintf("parserMessage(ECC): end flag err [0x%02x: 0x%02x]\n", receiveData[receiveDataLen-2], receiveData[receiveDataLen-1]);
        return CARD_MESSAGE_RETURN_PARSER_ERROR;
    }
    targetType1 = 0x04;
    targetType2 = 0x01;    
    
    
    if((receiveData[1] != targetType1) || (receiveData[2] != targetType2))
    {
        sysprintf("parserMessage(ECC): type err [0x%02x, 0x%02x : 0x%02x, 0x%02x]\n", receiveData[1], receiveData[2], targetType1, targetType2);
        
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
    //lrcValue = EPMReaderLRC(receiveData, 5, dataLen-1);  
    
    if(receiveDataLen != dataLen + 7)//扣掉 FEP 封包
    {
        sysprintf("parserMessage(ECC): len error \n");
        return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
    }
    else
    {                
        lrcValue = EPMReaderLRC(receiveData, 5, dataLen-1);//Body 最後是LRC, 所以要扣掉
        // ...      -5  -4  -3  -2  -1
        // ...    | status|LRC|0x90|0x00
        if(lrcValue != receiveData[(receiveDataLen-3)])
        {
            sysprintf("parserMessage(ECC): EPMReaderLRC error [0x%02x]  compare 0x%02x \r\n", lrcValue, receiveData[(receiveDataLen-3)]);
    
            *returnCode = 0x0;
            return CARD_MESSAGE_TYPE_ECC_RETURN_LRC_ERROR;
        }
        else
        {                 
            //#if(ENABLE_SHOW_RETURN_DATA)
            #if(0)
            sysprintf("\r\n--- receiveData [%d] --->\r\n", dataLen);
            for(int i = 0; i<dataLen; i++)
            {
                //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                sysprintf("0x%02x, ", receiveData[i+5]);
                if(i%10 == 9)
                    sysprintf("\r\n");
    
            }
            sysprintf("\r\n<--- receiveData ---\r\n");
            #endif  
            *resultStatus = receiveData[receiveDataLen-4]|(receiveData[receiveDataLen-5]<<8);  
            switch(msgType)
            {
                case CARD_MESSAGE_TYPE_ECC_PPR_RESET:
                    #if(ENABLE_ECC_LOG_MESSAGE)
                    //#if(0)
                    {
                        char str[512];
                        sprintf(str, "\r\n<--- CARD_MESSAGE_TYPE_ECC_PPR_RESET:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                        LoglibPrintf(LOG_TYPE_INFO, str, FALSE);                        
                        sprintf(str, "pprreset(0x%04X)", *resultStatus);   
                        DataProcessSendStatusData(0, str, WEB_POST_EVENT_NORMAL);
                    }
                    #else
                    sysprintf("\r\n<--- CARD_MESSAGE_TYPE_ECC_PPR_RESET:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                    #endif
                    //just for test
                    //*resultStatus = 0x6303;

                    switch(*resultStatus)
                    {
                        case ECC_CMD_RESET_SUCCESS_ID:
                            //20180801 for socket signon  
                            serialNumber = 1;
                        
                            //serialNumber = 0;

                            if(sizeof(eccCmdPPRResetResponseData) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy((uint8_t*)&eccCmdPPRResetResponseData, receiveData+5, sizeof(eccCmdPPRResetResponseData));
                            }
                            break;
                        
                        default: 
                            return CARD_MESSAGE_TYPE_ECC_RETURN_PPRRESET_ERROR;
                    }
                    break;
                case CARD_MESSAGE_TYPE_ECC_DCA_READ:
                    #if(ENABLE_ECC_LOG_MESSAGE)
                    //#if(0)
                    {
                        char str[512];
                        sprintf(str, "\r\n<--- CARD_MESSAGE_TYPE_ECC_DCA_READ:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1 /*扣掉LRC, 有包含status */);
                        LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
                    }
                    #else
                    sysprintf("\r\n<--- CARD_MESSAGE_TYPE_ECC_DCA_READ:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1 /*扣掉LRC, 有包含status */);
                    #endif
                    
                    switch(*resultStatus)
                    {
                        case ECC_CMD_READ_SUCCESS_ID:
                            if(sizeof(eccCmdDCAReadResponseSuccessData) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                 return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy(&eccCmdDCAReadResponseSuccessData, receiveData + 5 , sizeof(eccCmdDCAReadResponseSuccessData));
                            }
                            break;
                        //0x640E(餘額異常) or 0x6418(通路限制)
                        case ECC_CMD_READ_ERROR_1_ID_1:// 0x640E(餘額異常)   
                        case ECC_CMD_READ_ERROR_1_ID_2://0x6418(通路限制)
                            if(sizeof(eccCmdDCAReadResponseError1Data) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                 return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy(&eccCmdDCAReadResponseError1Data, receiveData + 5 , sizeof(eccCmdDCAReadResponseError1Data));
                            }
                            break;


                        case ECC_CMD_READ_ERROR_2_ID_1://0x6103(CPD檢查異常)
                            if(sizeof(eccCmdDCAReadResponseError2Data) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                 return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy(&eccCmdDCAReadResponseError2Data, receiveData + 5 , sizeof(eccCmdDCAReadResponseError2Data));
                            }
                            break;
                    }
                    break;
                case CARD_MESSAGE_TYPE_ECC_EDCA_DEDUCT:
                    #if(ENABLE_ECC_LOG_MESSAGE)
                    //#if(0)
                    {
                        char str[512];
                        sprintf(str, "\r\n<--- CARD_MESSAGE_TYPE_ECC_EDCA_DEDUCT:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                        LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
                    }
                    #else
                    sysprintf("\r\n<--- CARD_MESSAGE_TYPE_ECC_EDCA_DEDUCT:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                    #endif
                    
                    switch(*resultStatus)
                    {
                        case ECC_CMD_EDCA_DEDUCT_SUCCESS_ID:
                            if(sizeof(eccCmdEDCADeductResponseData) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                 return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy(&eccCmdEDCADeductResponseData, receiveData + 5, sizeof(eccCmdEDCADeductResponseData));
                            }
                            break;
                    }
                    break;

                case CARD_MESSAGE_TYPE_ECC_LOCK_CARD:
                    #if(ENABLE_ECC_LOG_MESSAGE)
                    //#if(0)
                    {
                        char str[512];
                        sprintf(str, "\r\n<--- CARD_MESSAGE_TYPE_ECC_LOCK_CARD:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                        LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
                    }
                    #else
                    sysprintf("\r\n<--- CARD_MESSAGE_TYPE_ECC_LOCK_CARD:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                    #endif
                    switch(*resultStatus)
                    {
                        case ECC_CMD_LOCK_CARD_SUCCESS_ID:
                            if(sizeof(eccCmdLockCardResponseData) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                 return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy(&eccCmdLockCardResponseData, receiveData + 5, sizeof(eccCmdLockCardResponseData));
                            }
                            break;
                    }
                    break;
                case CARD_MESSAGE_TYPE_ECC_SIGN_ON_QUERY:
                    #if(ENABLE_ECC_LOG_MESSAGE)
                    //#if(0)
                    {
                        char str[512];
                        sprintf(str, "\r\n<--- CARD_MESSAGE_TYPE_ECC_SIGN_ON_QUERY:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                        LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
                    }
                    #else
                    sysprintf("\r\n<--- CARD_MESSAGE_TYPE_ECC_SIGN_ON_QUERY:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                    #endif
                    switch(*resultStatus)
                    {
                        case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_1:
                        case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_2:
                        case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_3:
                            if(sizeof(eccCmdPPRSignOnQueryResponseData) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                 return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy(&eccCmdPPRSignOnQueryResponseData, receiveData + 5, sizeof(eccCmdPPRSignOnQueryResponseData));
                            }
                            break;
                    }
                    break;
                    
                case CARD_MESSAGE_TYPE_ECC_SIGN_ON:
                    #if(ENABLE_ECC_LOG_MESSAGE)
                    //#if(0)
                    {
                        char str[512];
                        sprintf(str, "\r\n<--- CARD_MESSAGE_TYPE_ECC_SIGN_ON:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                        LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
                    }
                    #else
                    sysprintf("\r\n<--- CARD_MESSAGE_TYPE_ECC_SIGN_ON:  resultStatus = 0x%04x, real ecc data len = %d---\r\n", *resultStatus, dataLen - 1/*扣掉LRC, 有包含status */);
                    #endif
                    switch(*resultStatus)
                    {
                        case ECC_CMD_SIGN_ON_SUCCESS_ID:
                        case ECC_CMD_SIGN_ON_NEED_UPDATE_ID:
                            if(sizeof(eccCmdPPRSignOnResponseData) != (dataLen - 3)) //扣掉 LRC, status
                            {
                                 return CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR;
                            }
                            else
                            {
                                memcpy(&eccCmdPPRSignOnResponseData, receiveData + 5, sizeof(eccCmdPPRSignOnResponseData));
                            }
                            break;
                    }
                    break;


                    
                default:
                    return CARD_MESSAGE_RETURN_PARSER_ERROR;
                    //break;
            }
                      
        }
    } 

    return returnInfo;
}

static uint16_t getLenData(uint16_t data)
{
    uint16_t reval = ((data>>8)&0xff) |((data&0xff)<<8);
    return reval;
}

#define MAX_ASCII_STR_LEN 16
static void getECCCmdUint32ToASCIIStr(uint32_t number, uint8_t* data, uint8_t dataLen)
{
    char tmp[MAX_ASCII_STR_LEN+1];  
    if(dataLen > MAX_ASCII_STR_LEN)
        return;

    sprintf(tmp, "%016d", number);  
    //sysprintf("getECCCmdUint32ToASCIIStr :[%d, %X][%s]\r\n", number, number, tmp);  
    memcpy(data, &tmp[MAX_ASCII_STR_LEN-dataLen], dataLen*sizeof(char));//只有cpy 最後的幾格字元
}

static void getECCCmdDataTimeData(uint32_t time, ECCCmdDataTime* eccCmdDataTime)
{
    //sysprintf("getECCCmdDataTimeData :[%d, %X]\r\n", time, time);  
    for(int i = 0; i < sizeof(ECCCmdDataTime); i++)
    {
        eccCmdDataTime->value[i] = (time>>(8*i))&0xff;
    }
}

static void getECCCmdSerialNumber(uint32_t number, ECCCmdSerialNumber* eccCmdSerialNumber)
{
    char tmp[6+1];     
    sprintf(tmp, "%06d", number);  
    //sysprintf("getECCCmdSerialNumber :[%d, %X][%s]\r\n", number, number, tmp);  
    memcpy(eccCmdSerialNumber->value, tmp, 6);
}

static void getECCCmdUTCTimeStr(uint32_t utcTime, uint8_t* data, uint8_t dataLen)
{
    char tmp[15];   
    if(dataLen != 14)
        return;
    RTC_TIME_DATA_T time;
    Time2RTC(utcTime, &time);  
    sprintf(tmp, "%04d%02d%02d%02d%02d%02d", time.u32Year, time.u32cMonth, time.u32cDay, time.u32cHour, time.u32cMinute, time.u32cSecond);
    //sysprintf("getECCCmdUTCTimeStr :[%d, %X][%s]\r\n", utcTime, utcTime, tmp);  
    memcpy(data, tmp, 14);
}
static int32_t uint8ToInt32(uint8_t* data, uint8_t dataLen)
{
    int32_t reVal = 0;
    if(dataLen > sizeof(int32_t))
        return 0;
    for(int i = 0; i < sizeof(int32_t); i++)
    {
        if(i<dataLen)
        {
            reVal = reVal | ((int32_t)(data[i])<<(8*i));
        }
        else
        {
            reVal = reVal | ((int32_t)(0xff)<<(8*i));
        }
    }
    return reVal;
}
static uint32_t uint8ToUint32(uint8_t* data, uint8_t dataLen)
{
    uint32_t reVal = 0;
    if(dataLen > sizeof(uint32_t))
        return 0;
    for(int i = 0; i<dataLen; i++)
    {
        reVal = reVal | ((uint32_t)(data[i])<<(8*i));
    }
    return reVal;
}

static uint8_t uint32ToUint8(uint32_t src, uint8_t* data, uint8_t dataLen)
{
    if(dataLen > sizeof(uint32_t))
        return 0;
    for(int i = 0; i < dataLen; i++)
    {
        data[i] = (src>>(8*i))&0xff;
    }
    return dataLen;
}
static uint8_t getMoneyToUint8(uint32_t src, uint8_t* data, uint8_t dataLen)
{
    return uint32ToUint8(src, data, dataLen);
}


static uint16_t pprReset(uint16_t* returnCode, uint16_t* resultStatus, uint32_t utcTime, uint32_t epmUTCTime, BOOL SignOnFlag)
{
    static ECCCmdPPRResetRequestPack eccCmdPPRResetRequestPack;
    uint16_t returnInfo;
    uint8_t* receiveData;
    uint16_t receiveDataLen;
    serialNumber = 1;
    sysprintf("ECCLibInit(pprReset): utcTime = %d, epmUTCTime = %d, serialNumber = %d...\r\n", utcTime, epmUTCTime, serialNumber);  
    eccCmdPPRResetRequestPack.header = 0xEA;
    eccCmdPPRResetRequestPack.cmd1   = 0x04;
    eccCmdPPRResetRequestPack.cmd2   = 0x01;
    eccCmdPPRResetRequestPack.len    = getLenData(sizeof(ECCCmdPPRResetRequest) + 1); // add LRC byte

    eccCmdPPRResetRequestPack.body.CLA = 0x80;
    eccCmdPPRResetRequestPack.body.INS = 0x01;
    eccCmdPPRResetRequestPack.body.P1 = 0x00;
    #if(ENABLE_ECC_AUTO_LOAD)
    if(SignOnFlag)
    {
        eccCmdPPRResetRequestPack.body.P2 = 0x00;//※ P2=0x00：PPR_Reset 用於授權取額度設備
    }
    else
    {
        eccCmdPPRResetRequestPack.body.P2 = 0x01;//※ P2=0x01：PPR_Reset_Offline
    }
    #else
    eccCmdPPRResetRequestPack.body.P2 = 0x01;//※ P2=0x01：PPR_Reset_Offline
    #endif
    eccCmdPPRResetRequestPack.body.Lc = 0x40;

    //data
    getECCCmdUint32ToASCIIStr(TM_LOCATION_ID, eccCmdPPRResetRequestPack.body.data.TMLocationID, sizeof(eccCmdPPRResetRequestPack.body.data.TMLocationID));  //TM Location ID 10 TM 終端機(TM)店號，ASCII(右靠左補 0)
    getECCCmdUint32ToASCIIStr(TM_ID, eccCmdPPRResetRequestPack.body.data.TMID, sizeof(eccCmdPPRResetRequestPack.body.data.TMID));  //TM Location ID 10 TM 終端機(TM)店號，ASCII(右靠左補 0)

    getECCCmdUTCTimeStr(epmUTCTime, eccCmdPPRResetRequestPack.body.data.TMTXNDateTime, sizeof(eccCmdPPRResetRequestPack.body.data.TMTXNDateTime));  //TM TXN Date Time 14 TM 終端機(TM)交易日期時間，ASCII (YYYYMMDDhhmmss)
    getECCCmdSerialNumber(serialNumber, &(eccCmdPPRResetRequestPack.body.data.TMSerialNumber)); //TM Serial Number 6 TM 終端機(TM)交易序號，ASCII (右靠左補 0，值須為 0~9) 交易成功時進號，失敗時不進號
    getECCCmdUint32ToASCIIStr(TM_AGENT_NUMBER, eccCmdPPRResetRequestPack.body.data.TMAgentNumber, sizeof(eccCmdPPRResetRequestPack.body.data.TMAgentNumber)); //TM Agent Number 4 TM 終端機(TM)收銀員代號，ASCII (右靠左補 0，值須為 0~9)
    sysprintf("ECCLibInit(pprReset): TMAgentNumber [0x%02x, 0x%02x, 0x%02x, 0x%02x] ...\r\n", 
                                                    eccCmdPPRResetRequestPack.body.data.TMAgentNumber[0], 
                                                    eccCmdPPRResetRequestPack.body.data.TMAgentNumber[1], 
                                                    eccCmdPPRResetRequestPack.body.data.TMAgentNumber[2], 
                                                    eccCmdPPRResetRequestPack.body.data.TMAgentNumber[3] );  
    getECCCmdDataTimeData(utcTime, &(eccCmdPPRResetRequestPack.body.data.TXNDateTime)); // //TXN Date Time 4 TM 交易日期時間 Unsigned and LSB First (UnixDateTime)
                
    eccCmdPPRResetRequestPack.body.data.LocationID = 0x32;                     //Location ID 1 定值 舊場站代碼 (由悠遊卡公司指定)
    eccCmdPPRResetRequestPack.body.data.NewLocationID[0] = 0x32;               //New Location ID 2 定值 新場站代碼 Unsigned and LSB First (由悠遊卡公司指定)
    eccCmdPPRResetRequestPack.body.data.NewLocationID[1] = 0x00;
    eccCmdPPRResetRequestPack.body.data.ServiceProviderID = 0x00;              //Service Provider ID 1 定值  服務業者代碼，補 0x00，1bytes 若設定 0x00 後續 Response 會帶出正確值
    memset(eccCmdPPRResetRequestPack.body.data.NewServiceProviderID, 0x00, sizeof(eccCmdPPRResetRequestPack.body.data.NewServiceProviderID));        //New Service Provider ID 3 定值 新服務業者代碼，補 0x00，3bytes Unsigned and LSB First 若設定 0x00 後續 Response 會帶出正確值
    eccCmdPPRResetRequestPack.body.data.PaymentType = 0x08;//0x73;                    //小額設定參數 1 TM 小額消費設定旗標  使用於第一類交易
                                                                                        //固定填 0x08
                                                                                        //使用於第二類交易
                                                                                        //固定填 0x73
    eccCmdPPRResetRequestPack.body.data.OneDayQuota[0] = 0x00;//0xB8;                 //One Day Quota For Micro Payment 2 TM 小額消費日限額額度 Unsigned and LSB First 
    eccCmdPPRResetRequestPack.body.data.OneDayQuota[1] = 0x00;//0x0B;                                 //使用於第一類交易
                                                                                             //固定填 0x00 0x00
                                                                                             //使用於第二類交易
                                                                                             //固定填 0xB8 x0B 即為 3000
    eccCmdPPRResetRequestPack.body.data.OnceQuota[0] = 0x00;//0xE8;                   //Once Quota For Micro Payment 2 TM小額消費次限額額度 Unsigned and LSB First
    eccCmdPPRResetRequestPack.body.data.OnceQuota[1] = 0x00;//0x03;                                //使用於第一類交易
                                                                                           //固定填 0x00 0x00
                                                                                           //使用於第二類交易
                                                                                           //固定填 0xE8 x03 即為 1000
    eccCmdPPRResetRequestPack.body.data.SAMSlotControlFlag = 0x11;             //SAM Slot Control Flag 1 TM SAM 卡位置控制旗標
                                                                                     //Bit 4 ~7：一代 SAM Slot 位置
                                                                                     //0000 xxxxb ：預設值，SAM Slot 1
                                                                                     //0001 xxxx b：SAM Slot 1
                                                                                     //0010 xxxxb ：SAM Slot 2
                                                                                     //……
                                                                                     //1111 xxxxb ：SAM Slot 15
                                                                                     //Bit 0 ~3：二代 SAM Slot 位置
                                                                                     //xxxx 0000 b：預設值，SAM Slot 2
                                                                                     //xxxx 0001 b：SAM Slot 1
                                                                                     //xxxx 0010 b：SAM Slot 2
                                                                                     //……
                                                                                     //xxxx 1111 b：SAM Slot 15
                                                                                     //使用二合一 SAM 固定填 0x11，其餘請用預設值 0x00
    memset(eccCmdPPRResetRequestPack.body.data.RFU, 0x00, sizeof(eccCmdPPRResetRequestPack.body.data.RFU));     //RFU(Reserved For Use) 11 TM 保留，補 0x00，11bytes
    //data

    eccCmdPPRResetRequestPack.body.Le = 0xFA;

    eccCmdPPRResetRequestPack.LRC = EPMReaderLRC((uint8_t*)&eccCmdPPRResetRequestPack, 5, sizeof(eccCmdPPRResetRequestPack.body));
    eccCmdPPRResetRequestPack.tail1 = 0x90;  
    eccCmdPPRResetRequestPack.tail2 = 0x00;  

    
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd((uint8_t*)&eccCmdPPRResetRequestPack,sizeof(eccCmdPPRResetRequestPack));
    if(nret != sizeof(eccCmdPPRResetRequestPack))
    {
        sysprintf("pprReset() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(eccCmdPPRResetRequestPack));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(3000, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char!@ count\n",count);
        if(count == 0)
        {
            sysprintf("pprReset() receiveReaderCmd error\n");
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;              
        }
        else
        {            
            returnInfo = parserMessage(CARD_MESSAGE_TYPE_ECC_PPR_RESET, receiveData, receiveDataLen, returnCode, resultStatus);
            
            #if(0)
            //just for test
            returnInfo = CARD_MESSAGE_RETURN_SUCCESS;
            #endif
            
            #if(1)
            if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
            {
                sysprintf("\r\n--- pprReset : status 0x%04x,", *resultStatus);
                uint8_t* prData; 
                uint32_t prDataLen;
                switch(*resultStatus)
                {
                    case ECC_CMD_RESET_SUCCESS_ID:
                        prData = (uint8_t*)&eccCmdPPRResetResponseData;
                        prDataLen = sizeof(eccCmdPPRResetResponseData);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- pprReset ---\r\n");
                        break;
                }     
            }
            #endif  
        }    
    }

    return returnInfo;
}


static uint16_t edcaRead(uint16_t* returnCode, uint16_t* resultStatus, uint32_t utcTime)
{
    static ECCCmdDCAReadRequestPack eccCmdDCAReadRequestPack;
    uint16_t returnInfo;
    uint8_t* receiveData;
    uint16_t receiveDataLen;
     
    eccCmdDCAReadRequestPack.header = 0xEA;
    eccCmdDCAReadRequestPack.cmd1   = 0x04;
    eccCmdDCAReadRequestPack.cmd2   = 0x01;
    eccCmdDCAReadRequestPack.len    = getLenData(sizeof(ECCCmdDCAReadRequest) + 1); // add LRC byte

    eccCmdDCAReadRequestPack.body.CLA = 0x80;
    eccCmdDCAReadRequestPack.body.INS = 0x01;
    eccCmdDCAReadRequestPack.body.P1 = 0x01;
    eccCmdDCAReadRequestPack.body.P2 = 0x00;
    eccCmdDCAReadRequestPack.body.Lc = 0x10;

    //data
    eccCmdDCAReadRequestPack.body.data.lcdControlFlag = 0x00;
    getECCCmdSerialNumber(serialNumber, &(eccCmdDCAReadRequestPack.body.data.tmSerialNumber));
    getECCCmdDataTimeData(utcTime, &(eccCmdDCAReadRequestPack.body.data.txnDateTime));
    memset(eccCmdDCAReadRequestPack.body.data.RFU, 0x20, sizeof(eccCmdDCAReadRequestPack.body.data.RFU));
    //data

    eccCmdDCAReadRequestPack.body.Le = 0xA0;

    eccCmdDCAReadRequestPack.LRC = EPMReaderLRC((uint8_t*)&eccCmdDCAReadRequestPack, 5, sizeof(eccCmdDCAReadRequestPack.body));
    eccCmdDCAReadRequestPack.tail1 = 0x90;  
    eccCmdDCAReadRequestPack.tail2 = 0x00;  

    
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd((uint8_t*)&eccCmdDCAReadRequestPack,sizeof(eccCmdDCAReadRequestPack));
    if(nret != sizeof(eccCmdDCAReadRequestPack))
    {
        sysprintf("edcaRead() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(eccCmdDCAReadRequestPack));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(600, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char!@ readICD\n",count);
        if(count == 0)
        {
            sysprintf("edcaRead() receiveReaderCmd error\n");
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;              
        }
        else
        {            
            returnInfo = parserMessage(CARD_MESSAGE_TYPE_ECC_DCA_READ, receiveData, receiveDataLen, returnCode, resultStatus);
            #if(0)
            if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
            {
                uint8_t* prData;
                uint32_t prDataLen;
                sysprintf("\r\n--- edcaRead : status 0x%04x,", *resultStatus);
                switch(*resultStatus)
                {
                    case ECC_CMD_READ_SUCCESS_ID:
                        prData = (uint8_t*)&eccCmdDCAReadResponseSuccessData;
                        prDataLen = sizeof(eccCmdDCAReadResponseSuccessData);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- edcaRead ---\r\n");
                        break;

                        //0x640E(餘額異常) or 0x6418(通路限制)
                    case ECC_CMD_READ_ERROR_1_ID_1:// 0x640E(餘額異常)   
                    case ECC_CMD_READ_ERROR_1_ID_2://0x6418(通路限制)
                        prData = (uint8_t*)&eccCmdDCAReadResponseError1Data;
                        prDataLen = sizeof(eccCmdDCAReadResponseError1Data);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- edcaRead ---\r\n");
                        break;


                    case ECC_CMD_READ_ERROR_2_ID_1://0x6103(CPD檢查異常)
                        prData = (uint8_t*)&eccCmdDCAReadResponseError2Data;
                        prDataLen = sizeof(eccCmdDCAReadResponseError2Data);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- edcaRead ---\r\n");
                        break;


                }                
                
            }
            #endif
        }    
    }

    return returnInfo;
}

static uint16_t edcaDeduct(uint16_t* returnCode, uint16_t* resultStatus, uint32_t deductValue, uint32_t utcTime, uint32_t epmUTCTime, BOOL AutoLoadFlag)
{
    static ECCCmdEDCADeductPack eccCmdEDCADeductPack;
    uint16_t returnInfo;
    uint8_t* receiveData;
    uint16_t receiveDataLen;
    
    sysprintf("\r\n --> edcaDeduct() AutoLoadFlag = %d...\n", AutoLoadFlag);
     
    eccCmdEDCADeductPack.header = 0xEA;
    eccCmdEDCADeductPack.cmd1   = 0x04;
    eccCmdEDCADeductPack.cmd2   = 0x01;
    eccCmdEDCADeductPack.len    = getLenData(sizeof(ECCCmdEDCADeductRequest) + 1); // add LRC byte

    eccCmdEDCADeductPack.body.CLA = 0x80;
    eccCmdEDCADeductPack.body.INS = 0x01;
    eccCmdEDCADeductPack.body.P1 = 0x02;
    eccCmdEDCADeductPack.body.P2 = 0x00;
    eccCmdEDCADeductPack.body.Lc = 0x40;

    //data
    eccCmdEDCADeductPack.body.data.MsgType = 0x01;                                //Msg Type 1 TM 0x01 扣款
    eccCmdEDCADeductPack.body.data.NewSubtype = 0x00;                             //New_Subtype 1 TM 0x00 default
    getECCCmdUint32ToASCIIStr(TM_LOCATION_ID, eccCmdEDCADeductPack.body.data.TMLocationID, sizeof(eccCmdEDCADeductPack.body.data.TMLocationID));  //TM Location ID 10 TM 終端機(TM)店號，ASCII (右靠左補 0)
    getECCCmdUint32ToASCIIStr(TM_ID, eccCmdEDCADeductPack.body.data.TMID, sizeof(eccCmdEDCADeductPack.body.data.TMID));  //TM ID 2 TM 終端機(TM)機號，ASCII (右靠左補 0)
    
    getECCCmdUTCTimeStr(epmUTCTime, eccCmdEDCADeductPack.body.data.TMTXNDateTime, sizeof(eccCmdEDCADeductPack.body.data.TMTXNDateTime));  //TM TXN Date Time 14 TM 終端機(TM)交易日期時間 (YYYYMMDDhhmmss)
    getECCCmdSerialNumber(serialNumber, &(eccCmdEDCADeductPack.body.data.TMSerialNumber));     //TM Serial Number 6 TM 終端機(TM)交易序號，ASCII (右靠左補 0，值須為 0~9)交易成功時進號，失敗時不進號
    getECCCmdUint32ToASCIIStr(TM_AGENT_NUMBER, eccCmdEDCADeductPack.body.data.TMAgentNumber, sizeof(eccCmdEDCADeductPack.body.data.TMAgentNumber));                       //TM Agent Number 4 TM 終端機(TM)收銀員代號，ASCII (右靠左補 0，值須為 0~9)
    getECCCmdDataTimeData(utcTime, &(eccCmdEDCADeductPack.body.data.TXNDateTime));     //TXN Date Time 4 TM 交易日期時間 Unsigned and LSB First (UnixDateTime)
    
    getMoneyToUint8(deductValue, eccCmdEDCADeductPack.body.data.TXNAMT, sizeof(eccCmdEDCADeductPack.body.data.TXNAMT));  //TXN AMT 3 TM 交易金額 Signed and LSB First (使用特種票時補 0x00)
    #if(ENABLE_ECC_AUTO_LOAD)
    if(AutoLoadFlag)
        eccCmdEDCADeductPack.body.data.AutoLoad = 0x01;              //Auto-Load 1 TM 是否進行自動加值
    else                                                                    //- 0x00：否
        eccCmdEDCADeductPack.body.data.AutoLoad = 0x00;                     //- 0x01：是
    #else
        eccCmdEDCADeductPack.body.data.AutoLoad = 0x00;
    #endif

    eccCmdEDCADeductPack.body.data.TXNType = 0x20;                  //TXN Type 1 TM 交易方式 
                                                                        //- 0x20：小額扣款
    //2018.07.09 modify in ECC
    eccCmdEDCADeductPack.body.data.TransferGroupCode = 0x06;//0x00;         //Transfer Group Code 1 TM 本運具之轉乘群組代碼
                                                                        //0x06：小額扣款型進出站停車場
                                                                        //0x00：其它無需寫入轉乘資訊
    memset(eccCmdEDCADeductPack.body.data.RFU, 0x00, sizeof(eccCmdEDCADeductPack.body.data.RFU));   //RFU(Reserved For Use) 15 TM 保留，補 0x00，15 bytes
    eccCmdEDCADeductPack.body.data.LCDControlFlag = 0x00;          //LCD Control Flag 1 TM 用於控制交易完成後之 LCD 顯示
                                                                        //0x00：顯示【交易完成 請取卡】(default)
                                                                        //0x01：顯示【（請勿移動票卡）】
    //data

    eccCmdEDCADeductPack.body.Le = 0x7A;

    eccCmdEDCADeductPack.LRC = EPMReaderLRC((uint8_t*)&eccCmdEDCADeductPack, 5, sizeof(eccCmdEDCADeductPack.body));
    eccCmdEDCADeductPack.tail1 = 0x90;  
    eccCmdEDCADeductPack.tail2 = 0x00;  

    
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd((uint8_t*)&eccCmdEDCADeductPack,sizeof(eccCmdEDCADeductPack));
    if(nret != sizeof(eccCmdEDCADeductPack))
    {
        sysprintf("edcaDeduct() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(eccCmdEDCADeductPack));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(500, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char!@ readICD\n",count);
        if(count == 0)
        {
            sysprintf("edcaDeduct() receiveReaderCmd error\n");
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;              
        }
        else
        {            
            returnInfo = parserMessage(CARD_MESSAGE_TYPE_ECC_EDCA_DEDUCT, receiveData, receiveDataLen, returnCode, resultStatus);
            #if(0)
            if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
            {
                uint8_t* prData;
                uint32_t prDataLen;
                sysprintf("\r\n--- edcaDeduct : status 0x%04x,", *resultStatus);
                switch(*resultStatus)
                {
                    case ECC_CMD_EDCA_DEDUCT_SUCCESS_ID:
                        prData = (uint8_t*)&eccCmdEDCADeductResponseData;
                        prDataLen = sizeof(eccCmdEDCADeductResponseData);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- edcaDeduct ---\r\n");
                        break;
                }                
                
            }
            #endif
        }    
    }

    return returnInfo;
}
static uint16_t eccLockCard(uint16_t* returnCode, uint16_t* resultStatus, uint8_t* cardId, uint32_t utcTime)
{
    static ECCCmdLockCardPack eccCmdLockCardPack;
    uint16_t returnInfo;
#if(ENABLE_ECC_LOCK_CARD)
    uint8_t* receiveData;
    uint16_t receiveDataLen;
#endif
    eccCmdLockCardPack.header = 0xEA;
    eccCmdLockCardPack.cmd1   = 0x04;
    eccCmdLockCardPack.cmd2   = 0x01;
    eccCmdLockCardPack.len    = getLenData(sizeof(ECCCmdLockCardRequest) + 1); // add LRC byte

    eccCmdLockCardPack.body.CLA = 0x80;
    eccCmdLockCardPack.body.INS = 0x41;
    eccCmdLockCardPack.body.P1 = 0x01;
    eccCmdLockCardPack.body.P2 = 0x00;
    eccCmdLockCardPack.body.Lc = 0x0E;

    //data
    eccCmdLockCardPack.body.data.MsgType = 0x22;                                //Msg Type 1 API 0x22 鎖卡
    eccCmdLockCardPack.body.data.Subtype = 0x00;                                //Subtype 1 API 0x00 default
    memcpy(eccCmdLockCardPack.body.data.CardPhysicalID, cardId, sizeof(eccCmdLockCardPack.body.data.CardPhysicalID));    //Card Physical ID 7 API Mifare 卡號
    getECCCmdDataTimeData(utcTime, &(eccCmdLockCardPack.body.data.TXNDateTime)); //TXN Date Time 4 API 交易日期時間 Unsigned and LSB First (UnixDateTime)
    eccCmdLockCardPack.body.data.BlockingReason = 0x01;                         //Blocking Reason 1 API 鎖卡原因 定值 0x01
    //data

    eccCmdLockCardPack.body.Le = 0x28;

    eccCmdLockCardPack.LRC = EPMReaderLRC((uint8_t*)&eccCmdLockCardPack, 5, sizeof(eccCmdLockCardPack.body));
    eccCmdLockCardPack.tail1 = 0x90;  
    eccCmdLockCardPack.tail2 = 0x00;  


    *returnCode = CARD_MESSAGE_CODE_NO_USE;
#if(!ENABLE_ECC_LOCK_CARD)
    uint8_t* prData;
    uint32_t prDataLen;
    prData = (uint8_t*)&eccCmdLockCardPack;
    prDataLen = sizeof(eccCmdLockCardPack);
    sysprintf("\r\n~~~ eccCmdLockCardPack : status 0x%04x,", *resultStatus);
    sysprintf(" len = %d ~~~>\r\n", prDataLen);
    for(int i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<~~~ eccCmdLockCardPack ~~~\r\n");
    returnInfo = CARD_MESSAGE_RETURN_SUCCESS;
    *resultStatus = ECC_CMD_LOCK_CARD_SUCCESS_ID;
#else
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd((uint8_t*)&eccCmdLockCardPack,sizeof(eccCmdLockCardPack));
    if(nret != sizeof(eccCmdLockCardPack))
    {
        sysprintf("eccLockCard() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(eccCmdLockCardPack));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(500, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char!@ readICD\n",count);
        if(count == 0)
        {
            sysprintf("eccLockCard() receiveReaderCmd error\n");
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;              
        }
        else
        {            
            returnInfo = parserMessage(CARD_MESSAGE_TYPE_ECC_LOCK_CARD, receiveData, receiveDataLen, returnCode, resultStatus);
            #if(0)
            if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
            {
                uint8_t* prData;
                uint32_t prDataLen;
                sysprintf("\r\n--- eccLockCard : status 0x%04x,", *resultStatus);
                switch(*resultStatus)
                {
                    case ECC_CMD_LOCK_CARD_SUCCESS_ID:
                        prData = (uint8_t*)&eccCmdLockCardResponseData;
                        prDataLen = sizeof(eccCmdLockCardResponseData);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- eccLockCard ---\r\n");
                        break;
                }                
                
            }
            #endif
        }    
    }
#endif
    return returnInfo;
}
#if(ENABLE_ECC_AUTO_LOAD)
static uint16_t eccSignOnQuery(uint16_t* returnCode, uint16_t* resultStatus, uint32_t utcTime)
{
    static ECCCmdPPRSignOnQueryRequestPack eccCmdSignOnQueryRequestPack;
    uint16_t returnInfo;
    uint8_t* receiveData;
    uint16_t receiveDataLen;
     
    eccCmdSignOnQueryRequestPack.header = 0xEA;
    eccCmdSignOnQueryRequestPack.cmd1   = 0x04;
    eccCmdSignOnQueryRequestPack.cmd2   = 0x01;
    eccCmdSignOnQueryRequestPack.len    = getLenData(sizeof(ECCCmdPPRSignOnQueryRequest) + 1); // add LRC byte

    eccCmdSignOnQueryRequestPack.body.CLA = 0x80;
    eccCmdSignOnQueryRequestPack.body.INS = 0x03;
    eccCmdSignOnQueryRequestPack.body.P1 = 0x00;
    eccCmdSignOnQueryRequestPack.body.P2 = 0x00;
    //沒有 lc
    eccCmdSignOnQueryRequestPack.body.Le = 0x28;

    eccCmdSignOnQueryRequestPack.LRC = EPMReaderLRC((uint8_t*)&eccCmdSignOnQueryRequestPack, 5, sizeof(eccCmdSignOnQueryRequestPack.body));
    eccCmdSignOnQueryRequestPack.tail1 = 0x90;  
    eccCmdSignOnQueryRequestPack.tail2 = 0x00;  

    
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd((uint8_t*)&eccCmdSignOnQueryRequestPack,sizeof(eccCmdSignOnQueryRequestPack));
    if(nret != sizeof(eccCmdSignOnQueryRequestPack))
    {
        sysprintf("eccSignOnQuery() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(eccCmdSignOnQueryRequestPack));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(500, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char!@ readICD\n",count);
        if(count == 0)
        {
            sysprintf("eccSignOnQuery() receiveReaderCmd error\n");
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;              
        }
        else
        {            
            returnInfo = parserMessage(CARD_MESSAGE_TYPE_ECC_SIGN_ON_QUERY, receiveData, receiveDataLen, returnCode, resultStatus);
            #if(1)
            if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
            {
                uint8_t* prData;
                uint32_t prDataLen;
                sysprintf("\r\n--- eccSignOnQuery : status 0x%04x,", *resultStatus);
                switch(*resultStatus)
                {
                    case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_1:
                    case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_2:
                    case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_3:                            
                        prData = (uint8_t*)&eccCmdPPRSignOnQueryResponseData;
                        prDataLen = sizeof(eccCmdPPRSignOnQueryResponseData);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- eccSignOnQuery ---\r\n");
                        break;
                }                
                
            }
            #endif
        }    
    }

    return returnInfo;
}

static void convertSpecialUint8To2Bit(uint8_t srcByte, uint8_t* destByte, int bitShift)
{
    switch(srcByte)
    {
        case 0x00:
            *destByte =  (*destByte & ~(0x3<<bitShift)) | (0x0<<bitShift);
            break;
        case 0x01:
            *destByte =  (*destByte & ~(0x3<<bitShift)) | (0x1<<bitShift);
            break;
        case 0x10:
            *destByte =  (*destByte & ~(0x3<<bitShift)) | (0x2<<bitShift);
            break;
        case 0x11:
            *destByte =  (*destByte & ~(0x3<<bitShift)) | (0x3<<bitShift);
            break;
    }
     
}

static void convertSpecialUint8To1Bit(uint8_t srcByte, uint8_t* destByte, int bitShift)
{
    switch(srcByte)
    {
        case 0x0:
            *destByte =  (*destByte & ~(0x1<<bitShift)) | (0x0<<bitShift);
            break;
        case 0x1:
            *destByte =  (*destByte & ~(0x1<<bitShift)) | (0x1<<bitShift);
            break;
    }
    
}

static uint16_t eccSignOn(uint16_t* returnCode, uint16_t* resultStatus, ECCCmdSignOnResponseSocketData* eccCmdSignOnResponseSocketData, uint32_t utcTime)
{
    static ECCCmdPPRSignOnRequestPack eccCmdSignOnRequestPack;
    uint8_t tmpByte;
    uint16_t returnInfo;
    uint8_t* receiveData;
    uint16_t receiveDataLen;
     
    eccCmdSignOnRequestPack.header = 0xEA;
    eccCmdSignOnRequestPack.cmd1   = 0x04;
    eccCmdSignOnRequestPack.cmd2   = 0x01;
    eccCmdSignOnRequestPack.len    = getLenData(sizeof(ECCCmdPPRSignOnRequest) + 1); // add LRC byte

    eccCmdSignOnRequestPack.body.CLA = 0x80;
    eccCmdSignOnRequestPack.body.INS = 0x02;
    eccCmdSignOnRequestPack.body.P1 = 0x00;
    eccCmdSignOnRequestPack.body.P2 = 0x00;
    eccCmdSignOnRequestPack.body.Lc = 0x80;
    
    //data
    ////---(以下 1 個欄位用於舊 SAM Card)
    memcpy(eccCmdSignOnRequestPack.body.data.HTAC, eccCmdSignOnResponseSocketData->HTAC, sizeof(eccCmdSignOnRequestPack.body.data.HTAC));                           //H-TAC 8 Host Host 認證碼 Response 電文：H-TAC
    ////---(以下 5 個欄位用於新 SAM Card)
    memcpy(eccCmdSignOnRequestPack.body.data.HAToken, eccCmdSignOnResponseSocketData->CPUHOSTToken, sizeof(eccCmdSignOnRequestPack.body.data.HAToken));             //HAToken 16 Host Response 電文：CPU HOST Token
    eccCmdSignOnRequestPack.body.data.SAMUpdateOption = eccCmdSignOnResponseSocketData->SAMUpdateOption;                                                            //SAM Update Option 1 Host Response 電文：SAM Update Option
    memcpy(eccCmdSignOnRequestPack.body.data.NewSAMValue, eccCmdSignOnResponseSocketData->NewSAMValue, sizeof(eccCmdSignOnRequestPack.body.data.NewSAMValue));      //New SAM Value 40 Host Response 電文：New SAM Value
    memcpy(eccCmdSignOnRequestPack.body.data.UpdateSAMValueMAC, eccCmdSignOnResponseSocketData->UpdateSAMValueMAC, sizeof(eccCmdSignOnRequestPack.body.data.UpdateSAMValueMAC));   //Update SAM Value MAC 16 Host Response 電文：Update SAM Value MAC
    ////---(以下 5 個欄位為 PPR_SignOn 設定參數之值，適用於有 SignOn 之設備)
    //CPD Read Flag／One Day Quota Write For Micro Payment／SAM SignOnControl Flag／Check EV Flag For Mifare Only／ Merchant Limit Use For Micro Payment  1 Host
    tmpByte = 0x00;    
    convertSpecialUint8To2Bit(eccCmdSignOnResponseSocketData->CPUCPDReadFlag, &tmpByte, 0); //index 199             //Response 電文：CPU CPD Read Flag 例: 0x00 則填入 xxxxxx00b  
    convertSpecialUint8To2Bit(eccCmdSignOnResponseSocketData->CPUOneDayQuotaWriteFlag, &tmpByte, 2); //index  198   //Response 電文：CPU One Day Quota Write Flag 例: 0x11 則填入 xxxx11xxb
    convertSpecialUint8To2Bit(eccCmdSignOnResponseSocketData->CPUSAMSignOnControlFlag, &tmpByte, 4); //index  197   //Response 電文：CPU SAM SignOnControl Flag 例: 0x11 則填入 xx11xxxxb
    convertSpecialUint8To1Bit(eccCmdSignOnResponseSocketData->CheckEVFlag, &tmpByte, 6);   //index  78              //Response 電文：Check EV Flag 例: 0x00 則填入 x0xxxxxxb 
    convertSpecialUint8To1Bit(eccCmdSignOnResponseSocketData->DeductLimitFlag, &tmpByte, 7);   //index  86          //Response 電文：Deduct Limit Flag 例: 0x00 則填入 0xxxxxxxb
    eccCmdSignOnRequestPack.body.data.Flag01 = tmpByte;  
    
    //One Day Quota Flag For Micro Payment／Once Quota Flag For Micro Payment／Check Debit Flag／RFU(Reserved For Use) 1 Host
    tmpByte = 0x00; 
//    uint8_t         Flag02;                     
    convertSpecialUint8To2Bit(eccCmdSignOnResponseSocketData->OneDayQuotaFlag, &tmpByte, 0);    //index 72      //Response 電文：One Day Quota Flag 例: 0x11 則填入 xxxxxx11b
    convertSpecialUint8To1Bit(eccCmdSignOnResponseSocketData->OnceQuotaFlag, &tmpByte, 2);      //index 75      //Response 電文：Once Quota Flag 例: 0x01 則填入 xxxxx1xxb 
    convertSpecialUint8To1Bit(eccCmdSignOnResponseSocketData->CheckDeductFlag, &tmpByte, 3);    //index 83      //Response 電文：Check Deduct Flag 例: 0x01 則填入 xxxx1xxxb
                                                                                                                //固定填入 0000xxxxb
    eccCmdSignOnRequestPack.body.data.Flag02 = tmpByte;   
    
    memcpy(eccCmdSignOnRequestPack.body.data.OneDayQuota, eccCmdSignOnResponseSocketData->OneDayQuota, sizeof(eccCmdSignOnRequestPack.body.data.OneDayQuota));     //One Day Quota 2 Host Response 電文：One Day Quota For Micro Payment 
    memcpy(eccCmdSignOnRequestPack.body.data.OnceQuotaForMicroPayment, eccCmdSignOnResponseSocketData->OnceQuota, sizeof(eccCmdSignOnRequestPack.body.data.OnceQuotaForMicroPayment));    //Once Quota For Micro Payment 2 Host Response 電文：Once Quota
    memcpy(eccCmdSignOnRequestPack.body.data.CheckDebitValue, eccCmdSignOnResponseSocketData->CheckDeductValue, sizeof(eccCmdSignOnRequestPack.body.data.CheckDebitValue));      //Check Debit Value 2 Host Response 電文：Check Deduct Value
    ////---(以下 2 個欄位用於舊 SAM Card 的額度控管)
    eccCmdSignOnRequestPack.body.data.AddQuotaFlag  = eccCmdSignOnResponseSocketData->AddQuotaFlag;     //Add Quota Flag 1 Host Response 電文：Add Quota Flag
    memcpy(eccCmdSignOnRequestPack.body.data.AddQuota, eccCmdSignOnResponseSocketData->AddQuota, sizeof(eccCmdSignOnRequestPack.body.data.AddQuota));   //Add Quota 3 Host Response 電文：Add Quota
    memset(eccCmdSignOnRequestPack.body.data.RFU, 0x00, sizeof(eccCmdSignOnRequestPack.body.data.RFU)); //RFU(Reserved For Use) 31 Host 保留，補 0x00，31bytes
    eccCmdSignOnRequestPack.body.data.EDC_1 = eccCmdSignOnResponseSocketData->CPUHashType;                             //EDC 4 Host  Response 電文：CPU Hash Type + Response 電文：CPU EDC
    memcpy(&eccCmdSignOnRequestPack.body.data.EDC_2, eccCmdSignOnResponseSocketData->CPUEDC, sizeof(eccCmdSignOnRequestPack.body.data.EDC_2));
   //data
    
    eccCmdSignOnRequestPack.body.Le = 0x1D;

    eccCmdSignOnRequestPack.LRC = EPMReaderLRC((uint8_t*)&eccCmdSignOnRequestPack, 5, sizeof(eccCmdSignOnRequestPack.body));
    eccCmdSignOnRequestPack.tail1 = 0x90;  
    eccCmdSignOnRequestPack.tail2 = 0x00;  

    
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    EPMReaderFlushBuffer();
    int nret = EPMReaderSendCmd((uint8_t*)&eccCmdSignOnRequestPack,sizeof(eccCmdSignOnRequestPack));
    if(nret != sizeof(eccCmdSignOnRequestPack))
    {
        sysprintf("eccSignOn() EPMReaderSendCmd size error [%d: %d]\n", nret, sizeof(eccCmdSignOnRequestPack));
        returnInfo = CARD_MESSAGE_RETURN_SEND_ERROR;
    }
    else
    {
        int count=EPMReaderReceiveCmd(500, &receiveData, &receiveDataLen);
        //sysprintf("we get %d char!@ readICD\n",count);
        if(count == 0)
        {
            sysprintf("eccSignOn() receiveReaderCmd error\n");
            returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;              
        }
        else
        {            
            returnInfo = parserMessage(CARD_MESSAGE_TYPE_ECC_SIGN_ON, receiveData, receiveDataLen, returnCode, resultStatus);
            #if(1)
            if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
            {
                uint8_t* prData;
                uint32_t prDataLen;
                sysprintf("\r\n--- eccSignOn : status 0x%04x,", *resultStatus);
                switch(*resultStatus)
                {
                        case ECC_CMD_SIGN_ON_SUCCESS_ID:
                        case ECC_CMD_SIGN_ON_NEED_UPDATE_ID:                            
                        prData = (uint8_t*)&eccCmdPPRSignOnResponseData;
                        prDataLen = sizeof(eccCmdPPRSignOnResponseData);
                        sysprintf(" len = %d --->\r\n", prDataLen);
                        for(int i = 0; i<prDataLen; i++)
                        {
                            //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                            sysprintf("0x%02x, ", prData[i]);
                            if(i%10 == 9)
                                sysprintf("\r\n");

                        }
                        sysprintf("\r\n<--- eccSignOn ---\r\n");
                        break;
                }                
                
            }
            #endif
        }    
    }

    return returnInfo;
}

static void convertSpecial2BitToUint8(uint8_t srcByte, uint8_t* destByte, int bitShift)
{
    uint8_t targetValue = (srcByte>>bitShift) & 0x3;
    switch(targetValue)
    {
        case 0:
            *destByte =  0x00;
            break;
        case 1:
            *destByte =  0x01;
            break;
        case 2:
            *destByte =  0x10;
            break;
        case 3:
            *destByte =  0x11;
            break;
    }
    
}

static void convertSpecial1BitToUint8(uint8_t srcByte, uint8_t* destByte, int bitShift)
{
    uint8_t targetValue = (srcByte>>bitShift) & 0x1;
    switch(targetValue)
    {
        case 0:
            *destByte =  0x00;
            break;
        case 1:
            *destByte =  0x01;
            break;
    }
    
}

static uint16_t eccSocketSignOnRequest(uint16_t* returnCode, uint16_t* resultStatus, ECCCmdPPRResetResponseData* eccCmdPPRResetResponseData, uint32_t utcTime, uint32_t epmUTCTime)
{
    static ECCCmdSignOnRequestSocketData eccCmdSignOnRequestSocketData;
    uint16_t returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_SIGN_ON_REQUEST_ERROR;
    //index 0~1
    eccCmdSignOnRequestSocketData.TotalLen[0] = 0x01;            //Length 2 Unsigned 總 長 度 360byte( 不 含 本 欄位)固定填 0x01 68
    eccCmdSignOnRequestSocketData.TotalLen[1] = 0x68; 
    //index 2~9
    eccCmdSignOnRequestSocketData.Header[0] = 0x39;              //Header 8 ASCII 固定 99903480 0x39 39 39 30 33 34 38 30
    eccCmdSignOnRequestSocketData.Header[1] = 0x39;
    eccCmdSignOnRequestSocketData.Header[2] = 0x39;
    eccCmdSignOnRequestSocketData.Header[3] = 0x30;
    eccCmdSignOnRequestSocketData.Header[4] = 0x33;
    eccCmdSignOnRequestSocketData.Header[5] = 0x34;
    eccCmdSignOnRequestSocketData.Header[6] = 0x38;
    eccCmdSignOnRequestSocketData.Header[7] = 0x30;
    //index 10~13
    eccCmdSignOnRequestSocketData.MessageTypeID[0] = 0x30;       //Message Type ID 4 ASCII 固定 0800 0x30 38 30 30
    eccCmdSignOnRequestSocketData.MessageTypeID[1] = 0x38;
    eccCmdSignOnRequestSocketData.MessageTypeID[2] = 0x30;
    eccCmdSignOnRequestSocketData.MessageTypeID[3] = 0x30;
    //index 14~16
    eccCmdSignOnRequestSocketData.DataFieldLen[0] = 0x33;        //1 Data Field Length 3 ASCII 總長度345byte 0x33 34 35
    eccCmdSignOnRequestSocketData.DataFieldLen[1] = 0x34; 
    eccCmdSignOnRequestSocketData.DataFieldLen[2] = 0x35; 
    //index 17~22
    eccCmdSignOnRequestSocketData.ProcessingCode[0] = 0x38;      //2 Processing Code 6 ASCII 固定 881999 (ASCII) 0x38 38 31 39 39 39
    eccCmdSignOnRequestSocketData.ProcessingCode[1] = 0x38; 
    eccCmdSignOnRequestSocketData.ProcessingCode[2] = 0x31; 
    eccCmdSignOnRequestSocketData.ProcessingCode[3] = 0x39; 
    eccCmdSignOnRequestSocketData.ProcessingCode[4] = 0x39; 
    eccCmdSignOnRequestSocketData.ProcessingCode[5] = 0x39; 
    //index 23~24
    eccCmdSignOnRequestSocketData.BlackListVersion[0]  = 0x00;    //6 BlackListVersion 2 Unsigned 固定 0x00 
    eccCmdSignOnRequestSocketData.BlackListVersion[1]  = 0x00;
    
    eccCmdSignOnRequestSocketData.MsgType  = 0x00;            //index 25    //7 Msg Type 1 Unsigned 固定 0x00
    eccCmdSignOnRequestSocketData.Subtype  = 0x00;            //index 26     //8 Subtype 1 Unsigned 固定 0x00
    memcpy(eccCmdSignOnRequestSocketData.DeviceID, eccCmdPPRResetResponseData->DeviceID, sizeof(eccCmdSignOnRequestSocketData.DeviceID));   //index 27~30 //9 Device ID 4 Unsigned Device ID
    eccCmdSignOnRequestSocketData.ServiceProvider = eccCmdPPRResetResponseData->DeviceID[2];                                                //index 31 //10 Service Provider 1 Unsigned Device ID 的第三個 byte 例:Device ID=0x3F B0 07 00 Service Provider=0x07
    getECCCmdDataTimeData(utcTime, &(eccCmdSignOnRequestSocketData.TXNDateTime));                                                           //index 32~35 //11 TXN Date Time 4 Unsigned 現在時間(Unix Date Time) Request,Response,Confirm 皆須一致
    getECCCmdUint32ToASCIIStr(TM_ID, eccCmdSignOnRequestSocketData.TMID, sizeof(eccCmdSignOnRequestSocketData.TMID));                       //index 36~37 //28 TM ID 2 ASCII TM ID
    getECCCmdUTCTimeStr(epmUTCTime, eccCmdSignOnRequestSocketData.TMTXNDateTime, sizeof(eccCmdSignOnRequestSocketData.TMTXNDateTime));      //index 38~51 //29 TM TXN Date Time 14 ASCII TM TXN Date Time 例:2012/10/23 18:50:35 0x32 30 31 32 31 30 32 33 3138 35 30 33 35
    getECCCmdSerialNumber(serialNumber, &(eccCmdSignOnRequestSocketData.TMSerialNumber));                                                   //index 52~57 //30 TM Serial Number 6 ASCII TM Serial Number 右靠左補 0;值須為 0~9
    getECCCmdUint32ToASCIIStr(TM_AGENT_NUMBER, eccCmdSignOnRequestSocketData.TMAgentNumber, sizeof(eccCmdSignOnRequestSocketData.TMAgentNumber)); //index 58~61 //31 TM Agent Number 4 ASCII TM Agent Number 右靠左補 0;值須為 0~9
    memcpy(eccCmdSignOnRequestSocketData.STAC, eccCmdPPRResetResponseData->STAC, sizeof(eccCmdSignOnRequestSocketData.STAC));           //index 62~69 //33 S-TAC 8 Unsigned S-TAC
    eccCmdSignOnRequestSocketData.KeyVersion = eccCmdPPRResetResponseData->SAMKeyVersion;             //index 70 //35 Key Version 1 Unsigned SAM Key Version
    memcpy(eccCmdSignOnRequestSocketData.SAMID, eccCmdPPRResetResponseData->SAMID, sizeof(eccCmdSignOnRequestSocketData.SAMID));         //index 71~78 //36 SAM ID 8 ASCII SAM ID
    memcpy(eccCmdSignOnRequestSocketData.SAMSN, eccCmdPPRResetResponseData->SAMSN, sizeof(eccCmdSignOnRequestSocketData.SAMSN));         //index 79~82 //37 SAM SN 4 Unsigned SAM SN
    memcpy(eccCmdSignOnRequestSocketData.SACRN, eccCmdPPRResetResponseData->SAMCRN, sizeof(eccCmdSignOnRequestSocketData.SACRN));        //index 83~90 //38 SAM CRN 8 Unsigned SAM CRN
    memcpy(eccCmdSignOnRequestSocketData.ReaderFirmwareVersion, eccCmdPPRResetResponseData->ReaderFWVersion, sizeof(eccCmdSignOnRequestSocketData.ReaderFirmwareVersion)); //index 91~96 //39 Reader Firmware Version 6 Unsigned Reader FW Version
    
    eccCmdSignOnRequestSocketData.NetworkManagement[0] = 0x30;   //index 97~99 //46 Network Management Code 3 ASCII 固定 079:Device Control 0x30 37 39
    eccCmdSignOnRequestSocketData.NetworkManagement[1] = 0x37; 
    eccCmdSignOnRequestSocketData.NetworkManagement[2] = 0x39; 
    
    //100
    convertSpecial2BitToUint8(eccCmdPPRResetResponseData->Flag02, &(eccCmdSignOnRequestSocketData.OneDayQuotaFlag), 0); //One Day Quota Flag 1 Unsigned One Day Quota Flag For Micro Payment  例: xxxxxx11b 則填入0x11
    memcpy(eccCmdSignOnRequestSocketData.OneDayQuota, eccCmdPPRResetResponseData->OneDayQuotaForMicroPayment, sizeof(eccCmdSignOnRequestSocketData.OneDayQuota)); //One Day Quota 2 Unsigned One Day Quota For Micro Payment
    
    //103
    convertSpecial1BitToUint8(eccCmdPPRResetResponseData->Flag02, &(eccCmdSignOnRequestSocketData.OnceQuotaFlag), 2);  //Once Quota Flag 1 Unsigned Once Quota Flag For Micro Payment  //例: xxxxx1xxb //則填入0x01
    memcpy(eccCmdSignOnRequestSocketData.OnceQuota, eccCmdPPRResetResponseData->OnceQuotaForMicroPayment, sizeof(eccCmdSignOnRequestSocketData.OnceQuota)); //Once Quota 2 Unsigned Once Quota For Micro Payment
    
    //108
    convertSpecial1BitToUint8(eccCmdPPRResetResponseData->Flag01, &(eccCmdSignOnRequestSocketData.CheckEVFlag), 6);   //Check EV Flag 1 Unsigned Check EV Flag For Mifare Only  //例: x0xxxxxxb //則填入0x00
    eccCmdSignOnRequestSocketData.AddQuotaFlag = eccCmdPPRResetResponseData->AddQuotaFlag;                       //Add Quota Flag 1 Unsigned Add Quota Flag
    memcpy(eccCmdSignOnRequestSocketData.AddQuota, eccCmdPPRResetResponseData->AddQuota, sizeof(eccCmdSignOnRequestSocketData.AddQuota));  //Add Quota 3 Unsigned Add Quota
    
    //111
    convertSpecial1BitToUint8(eccCmdPPRResetResponseData->Flag02, &(eccCmdSignOnRequestSocketData.CheckDeductFlag), 3); //Check Deduct Flag 1 Unsigned Check Debit Flag //例: xxxx1xxxb //則填入0x01
    memcpy(eccCmdSignOnRequestSocketData.CheckDeductValue, eccCmdPPRResetResponseData->CheckDebitValue, sizeof(eccCmdSignOnRequestSocketData.CheckDeductValue));   //Check Deduct Value 2 Unsigned Check Debit Value
    
    //114
    convertSpecial1BitToUint8(eccCmdPPRResetResponseData->Flag01, &(eccCmdSignOnRequestSocketData.DeductLimitFlag), 7);  //Deduct Limit Flag 1 Unsigned Merchant Limit Use For Micro Payment //例: 0xxxxxxxb  //則填入0x00
    
    //115
    eccCmdSignOnRequestSocketData.APIVersion[0] = 0x00;                      //API Version 4 Unsigned API Version
    eccCmdSignOnRequestSocketData.APIVersion[1] = 0x00;
    eccCmdSignOnRequestSocketData.APIVersion[2] = 0x02;
    eccCmdSignOnRequestSocketData.APIVersion[3] = 0x16;
    
    //119
    memset(eccCmdSignOnRequestSocketData.RFU, 0x00, sizeof(eccCmdSignOnRequestSocketData.RFU));         //RFU 5 Unsigned RFU
    memcpy(eccCmdSignOnRequestSocketData.TheRemainderOfAdd, eccCmdPPRResetResponseData->TheRemainderOfAddQuota, sizeof(eccCmdSignOnRequestSocketData.TheRemainderOfAdd));   //The Remainder of Add 3 Unsigned The Remainder of Add Quota Quota
    memcpy(eccCmdSignOnRequestSocketData.deMACParameter, eccCmdPPRResetResponseData->deMACParameter, sizeof(eccCmdSignOnRequestSocketData.deMACParameter));                 //deMAC Parameter 8 Unsigned deMAC Parameter
    
    //135
    //#warning changeHLByte on leon code, 沒差 看來是00
    memcpy(eccCmdSignOnRequestSocketData.CancelCreditQuota, eccCmdPPRResetResponseData->CancelCreditQuota, sizeof(eccCmdSignOnRequestSocketData.CancelCreditQuota));           //Cancel Credit Quota 3 Unsigned Cancel Credit Quota
    
    //138
    memset(eccCmdSignOnRequestSocketData.RFU2, 0x00, sizeof(eccCmdSignOnRequestSocketData.RFU2));    //RFU 18 Unsigned 固定填0x00

    //156
    memset(eccCmdSignOnRequestSocketData.CPUCardPhysicalID, 0x00, sizeof(eccCmdSignOnRequestSocketData.CPUCardPhysicalID)); //58 CPU Card Physical ID 7 Unsigned 固定填 0x00
    memset(eccCmdSignOnRequestSocketData.CPUTXNAMT, 0x00, sizeof(eccCmdSignOnRequestSocketData.CPUTXNAMT));    //59 CPU TXN AMT 3 Unsigned 固定填 0x00
    
    //166
    memcpy(eccCmdSignOnRequestSocketData.CPUDeviceID, eccCmdPPRResetResponseData->NewDeviceID, sizeof(eccCmdSignOnRequestSocketData.CPUDeviceID));    //62 CPU Device ID 6 Unsigned New Device ID
    memcpy(eccCmdSignOnRequestSocketData.CPUServiceProvider, &(eccCmdPPRResetResponseData->NewDeviceID[3]), sizeof(eccCmdSignOnRequestSocketData.CPUServiceProvider));    //63 CPU Service Provider ID 3 Unsigned New Device ID 的 4~6byte 例:New Device ID=0xD2 64 03 7B 00 00 CPU Service Provider=0x7B 00 00
    
    //175
    memset(eccCmdSignOnRequestSocketData.CPUEVBeforeTXN, 0x00, sizeof(eccCmdSignOnRequestSocketData.CPUEVBeforeTXN));  //71 CPU EV Before TXN 3 Unsigned 固定填 0x00
   
    //178
    memcpy(eccCmdSignOnRequestSocketData.CPUSAMID, eccCmdPPRResetResponseData->SID, sizeof(eccCmdSignOnRequestSocketData.CPUSAMID));  //79 CPU SAM ID 8 ASCII SID
    //186
    memcpy(eccCmdSignOnRequestSocketData.CPUSAMTXNCnt, eccCmdPPRResetResponseData->STC, sizeof(eccCmdSignOnRequestSocketData.CPUSAMTXNCnt));   //80 CPU SAM TXN CNT 4 Unsigned STC 
    //190
    eccCmdSignOnRequestSocketData.SAMVersionNumber = eccCmdPPRResetResponseData->SAMVersionNumber;             //SAM Version Number 1 Unsigned SAM Version Number
    //191
    memcpy(eccCmdSignOnRequestSocketData.SAMUsageControl, eccCmdPPRResetResponseData->SAMUsageControl, sizeof(eccCmdSignOnRequestSocketData.SAMUsageControl));    //SAM Usage Control 3 Unsigned SAM Usage Control
    //194
    eccCmdSignOnRequestSocketData.SAMAdminKVN = eccCmdPPRResetResponseData->SAMAdminKVN;                            //SAM Admin KVN 1 Unsigned SAM Admin KVN
    //195
    eccCmdSignOnRequestSocketData.SAMIssuerKVN = eccCmdPPRResetResponseData->SAMIssuerKVN;                           //SAM Issuer KVN 1 Unsigned SAM Issuer KVN
    //196
    memcpy(eccCmdSignOnRequestSocketData.TagListTable, eccCmdPPRResetResponseData->TagListTable, sizeof(eccCmdSignOnRequestSocketData.TagListTable));   //Tag List Table 40 Unsigned Tag List Table
    //236
    memcpy(eccCmdSignOnRequestSocketData.SAMIssuerSpecificData, eccCmdPPRResetResponseData->SAMIssuerSpecificData, sizeof(eccCmdSignOnRequestSocketData.SAMIssuerSpecificData));    //SAM Issuer Specific Data 32 Unsigned SAM Issuer Specific Data
    //268
    memcpy(eccCmdSignOnRequestSocketData.CPURSAM, eccCmdPPRResetResponseData->RSAM, sizeof(eccCmdSignOnRequestSocketData.CPURSAM));    //82 CPU RSAM 8 Unsigned RSAM
    //276
    memcpy(eccCmdSignOnRequestSocketData.CPURHOST, eccCmdPPRResetResponseData->RHOST, sizeof(eccCmdSignOnRequestSocketData.CPURHOST)); //83 CPU RHOST 8 Unsigned RHOST
    
    //284
    memcpy(eccCmdSignOnRequestSocketData.AuthorizedCreditLimit, eccCmdPPRResetResponseData->AuthorizedCreditLimit, sizeof(eccCmdSignOnRequestSocketData.AuthorizedCreditLimit));   //Authorized Credit Limit 3 Unsigned Authorized Credit Limit
    //287
    memcpy(eccCmdSignOnRequestSocketData.AuthorizedCreditBalance, eccCmdPPRResetResponseData->AuthorizedCreditBalance, sizeof(eccCmdSignOnRequestSocketData.AuthorizedCreditBalance));    //Authorized Credit Balance 3 Unsigned Authorized Credit Balance
    //290
    memcpy(eccCmdSignOnRequestSocketData.AuthorizedCreditCumulative, eccCmdPPRResetResponseData->AuthorizedCreditCumulative, sizeof(eccCmdSignOnRequestSocketData.AuthorizedCreditCumulative));    //Authorized Credit Cumulative 3 Unsigned Authorized Credit Cumulative
    //293
    memcpy(eccCmdSignOnRequestSocketData.AuthorizedCancelCreditCumulative, eccCmdPPRResetResponseData->AuthorizedCancelCreditCumulative, sizeof(eccCmdSignOnRequestSocketData.AuthorizedCancelCreditCumulative));    //Authorized Cancel Credit Cumulative 3 Unsigned Authorized Cancel Credit Cumulative

    //296
    memcpy(eccCmdSignOnRequestSocketData.CPUSAMSingleCreditTXNAMTLimit, eccCmdPPRResetResponseData->SingleCreditTXNAMTLimit, sizeof(eccCmdSignOnRequestSocketData.CPUSAMSingleCreditTXNAMTLimit));   //85 CPU SAM Single Credit TXN AMT Limit 3 Unsigned Single Credit TXN AMT Limit
    //299
    getECCCmdUint32ToASCIIStr(TM_LOCATION_ID, eccCmdSignOnRequestSocketData.CPUTMLocationID, sizeof(eccCmdSignOnRequestSocketData.CPUTMLocationID));       //90 CPU TM Location ID 10 ASCII TM Location ID
    //309
    memcpy(eccCmdSignOnRequestSocketData.CPUTERMToken, eccCmdPPRResetResponseData->SATOKEN, sizeof(eccCmdSignOnRequestSocketData.CPUTERMToken));      //91 CPU TERM Token 16 Unsigned SATOKEN
    //325
    memcpy(eccCmdSignOnRequestSocketData.CPULastDeviceID, eccCmdPPRResetResponseData->PreviousNewDeviceID, sizeof(eccCmdSignOnRequestSocketData.CPULastDeviceID));         //CPU Last Device ID 6 Unsigned Previous New Device ID
    //331
    memcpy(eccCmdSignOnRequestSocketData.CPULastSAMTXNCNT, eccCmdPPRResetResponseData->PreviousSTC, sizeof(eccCmdSignOnRequestSocketData.CPULastSAMTXNCNT));       //CPU Last SAM TXN CNT 4 Unsigned Previous STC
    //335
    memcpy(&(eccCmdSignOnRequestSocketData.LastTXNDateTime), &(eccCmdPPRResetResponseData->PreviousTXNDateTime), sizeof(eccCmdSignOnRequestSocketData.LastTXNDateTime));    //Last TXN Date Time 4 Unsigned Previous TXN Date Time
    //339
    eccCmdSignOnRequestSocketData.CPULastCreditBalanceChangeFlag = eccCmdPPRResetResponseData->PreviousCreditBalanceChangeFlag;         //CPU Last Credit Balance Change Flag 1 Unsigned Previous Credit Balance Change Flag
    //340
    memcpy(eccCmdSignOnRequestSocketData.ConfirmCode, eccCmdPPRResetResponseData->PreviousConfirmCode, sizeof(eccCmdSignOnRequestSocketData.ConfirmCode));        //Confirm Code 2 Unsigned Previous Confirm Code
    //342
    memcpy(eccCmdSignOnRequestSocketData.CPUTERMCryptogram, eccCmdPPRResetResponseData->PreviousCACrypto, sizeof(eccCmdSignOnRequestSocketData.CPUTERMCryptogram));      //CPU TERM Cryptogram 16 Unsigned Previous CACrypto
    //358
    convertSpecial2BitToUint8(eccCmdPPRResetResponseData->Flag01, &(eccCmdSignOnRequestSocketData.CPUSAMSignOnControlFlag), 4); //95 CPU SAM SignOnControl Flag 1 Unsigned SAM SignOnControl Flag //例: xx11xxxxb  //則填入 0x11
    //359
    eccCmdSignOnRequestSocketData.CPUSpecVersionNumber = eccCmdPPRResetResponseData->SpecVersionNumber;               //96 CPU Spec. Version Number 1 Unsigned Spec. Version Number
    //360
    convertSpecial2BitToUint8(eccCmdPPRResetResponseData->Flag01, &(eccCmdSignOnRequestSocketData.CPUOneDayQuotaWriteFlag), 2);     //97 CPU One Day Quota Write Flag 1 Unsigned One Day Quota Write For Micro Payment  //例: xxxx11xxb //則填入 0x11
    //361
    convertSpecial2BitToUint8(eccCmdPPRResetResponseData->Flag01, &(eccCmdSignOnRequestSocketData.CPUCPDReadFlag), 0);   //98 CPU CPD Read Flag 1 Unsigned CPD Read Flag //例: xxxxxx00b //則填入 0x00
    
    #if(1)
    {
    uint8_t* prData= (uint8_t*)&eccCmdSignOnRequestSocketData;
    uint32_t prDataLen = sizeof(eccCmdSignOnRequestSocketData);;

    sysprintf("r\n--- eccCmdSignOnRequestSocketData len = %d (serialNumber = %d) --->\r\n", prDataLen, serialNumber);
    for(int i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- eccCmdSignOnRequestSocketData ---\r\n");   
    }
    #endif
    
    uint8_t* receiveData;
    uint16_t receiveDataLen = 1; //表示需接收結果
    if(SocketSend(ECC_SIGN_ON_SERVER_IP, ECC_SIGN_ON_SERVER_PORT, (uint8_t*)&eccCmdSignOnRequestSocketData, sizeof(eccCmdSignOnRequestSocketData), &receiveData, &receiveDataLen))
    {
        sysprintf("\r\n     -[info]-> eccSocketSignOnRequest SocketSend OK: receiveData = [%s],  receiveDataLen = %d!!\n", receiveData, receiveDataLen);
        if(receiveDataLen == sizeof(ECCCmdSignOnResponseSocketData))
        {
            memcpy(&eccCmdSignOnResponseSocketData, receiveData, sizeof(ECCCmdSignOnResponseSocketData));
            uint8_t* prData= (uint8_t*)&eccCmdSignOnResponseSocketData;
            uint32_t prDataLen = sizeof(eccCmdSignOnResponseSocketData);
            *resultStatus = eccCmdSignOnResponseSocketData.GatewayRespond[1]|(eccCmdSignOnResponseSocketData.GatewayRespond[0]<<8);
            sysprintf("\r\n--- eccSocketSignOnRequest  len = %d : status 0x%04x, --> \r\n", prDataLen, *resultStatus);

            for(int i = 0; i<prDataLen; i++)
            {
                //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                sysprintf("0x%02x, ", prData[i]);
                if(i%10 == 9)
                    sysprintf("\r\n");

            }
            sysprintf("\r\n<--- eccSocketSignOnRequest ---\r\n");    
            returnInfo = CARD_MESSAGE_RETURN_SUCCESS;            
        }
        else
        {
        }
    }
    else
    {
        sysprintf("\r\n     -[info]-> eccSocketSignOnRequest SocketSend ERROR!!\n");
    }
    return returnInfo;
}

static uint16_t eccSocketSignOnConfirm(uint16_t* returnCode, uint16_t* resultStatus, ECCCmdPPRResetResponseData* eccCmdPPRResetResponseData, 
                                        ECCCmdPPRSignOnResponseData* eccCmdPPRSignOnResponseData, uint8_t* signOnStatus, uint32_t utcTime, uint32_t epmUTCTime)
{
    static ECCCmdSignOnConfirmRequestSocketData eccCmdSignOnConfirmRequestSocketData;
    uint16_t returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_SIGN_ON_CONFIRM_ERROR;
    
    eccCmdSignOnConfirmRequestSocketData.Length[0] = 0x00;              //Length 2 Unsigned 總長度67 byte(不含本欄位) 固定填 0x00 43
    eccCmdSignOnConfirmRequestSocketData.Length[1] = 0x43;
    
    eccCmdSignOnConfirmRequestSocketData.Header[0] = 0x39;              //Header 8 ASCII 固定 99903480  0x39 39 39 30 33 34 38 30
    eccCmdSignOnConfirmRequestSocketData.Header[1] = 0x39; 
    eccCmdSignOnConfirmRequestSocketData.Header[2] = 0x39; 
    eccCmdSignOnConfirmRequestSocketData.Header[3] = 0x30; 
    eccCmdSignOnConfirmRequestSocketData.Header[4] = 0x33; 
    eccCmdSignOnConfirmRequestSocketData.Header[5] = 0x34; 
    eccCmdSignOnConfirmRequestSocketData.Header[6] = 0x38; 
    eccCmdSignOnConfirmRequestSocketData.Header[7] = 0x30;   
    
    eccCmdSignOnConfirmRequestSocketData.MessageTypeID[0] = 0x30;       //Message Type ID 4 ASCII 固定 0801 0x30 38 30 31
    eccCmdSignOnConfirmRequestSocketData.MessageTypeID[1] = 0x38;
    eccCmdSignOnConfirmRequestSocketData.MessageTypeID[2] = 0x30;
    eccCmdSignOnConfirmRequestSocketData.MessageTypeID[3] = 0x31;    
    
    
    eccCmdSignOnConfirmRequestSocketData.DataFieldLength[0] = 0x30;     //1 Data Field Length 3 ASCII 總長度52 byte 0x30 35 32
    eccCmdSignOnConfirmRequestSocketData.DataFieldLength[1] = 0x35;
    eccCmdSignOnConfirmRequestSocketData.DataFieldLength[2] = 0x32;
    
    eccCmdSignOnConfirmRequestSocketData.ProcessingCode[0] = 0x38;      //2 Processing Code 6 ASCII 固定 881999 (ASCII) 0x38 38 31 39 39 39
    eccCmdSignOnConfirmRequestSocketData.ProcessingCode[1] = 0x38; 
    eccCmdSignOnConfirmRequestSocketData.ProcessingCode[2] = 0x31; 
    eccCmdSignOnConfirmRequestSocketData.ProcessingCode[3] = 0x39; 
    eccCmdSignOnConfirmRequestSocketData.ProcessingCode[4] = 0x39; 
    eccCmdSignOnConfirmRequestSocketData.ProcessingCode[5] = 0x39; 
    
    eccCmdSignOnConfirmRequestSocketData.MsgType = 0x00;                //7 Msg Type 1 Unsigned 固定 0x00
    eccCmdSignOnConfirmRequestSocketData.Subtype = 0x00;;                //8 Subtype 1 Unsigned 固定 0x00
    
    memcpy(eccCmdSignOnConfirmRequestSocketData.DeviceID, eccCmdPPRResetResponseData->DeviceID, sizeof(eccCmdSignOnConfirmRequestSocketData.DeviceID));  //9 Device ID 4 Unsigned Device ID
    getECCCmdDataTimeData(utcTime, &(eccCmdSignOnConfirmRequestSocketData.TXNDateTime));   //11 TXN Date Time 4 Unsigned 現在時間(Unix Date Time) Request,Response,Confirm 皆須一致
    
    memcpy(eccCmdSignOnConfirmRequestSocketData.ConfirmCode, signOnStatus, sizeof(eccCmdSignOnConfirmRequestSocketData.ConfirmCode));      //47 Confirm Code 2 Unsigned PPR_SignON 的回應碼  例:0x63 08
    memset(eccCmdSignOnConfirmRequestSocketData.CPUCardPhysicalID, 0x00, sizeof(eccCmdSignOnConfirmRequestSocketData.CPUCardPhysicalID));   //58 CPU Card Physical ID 7 Unsigned 固定 0x00
    memcpy(eccCmdSignOnConfirmRequestSocketData.CPUDeviceID, eccCmdPPRResetResponseData->NewDeviceID, sizeof(eccCmdSignOnConfirmRequestSocketData.CPUDeviceID));       //62 CPU Device ID 6 Unsigned New Device ID
    memcpy(eccCmdSignOnConfirmRequestSocketData.CPUSAMTXNCnt, eccCmdPPRResetResponseData->STC, sizeof(eccCmdSignOnConfirmRequestSocketData.CPUSAMTXNCnt));       //80 CPU SAM TXN CNT 4 Unsigned STC
    
    eccCmdSignOnConfirmRequestSocketData.CPUCreditBalanceChangeFlag = eccCmdPPRSignOnResponseData->CreditBalanceChangeFlag;     //100 CPU Credit Balance Change Flag 1 Unsigned Credit Balance Change Flag
    memcpy(eccCmdSignOnConfirmRequestSocketData.CPUTERMCryptogram, eccCmdPPRSignOnResponseData->CACrypto, sizeof(eccCmdSignOnConfirmRequestSocketData.CPUTERMCryptogram));  //101 CPU TERM Cryptogram 16 Unsigned CACrypto
    
    #if(1)
    {
    uint8_t* prData= (uint8_t*)&eccCmdSignOnConfirmRequestSocketData;
    uint32_t prDataLen = sizeof(eccCmdSignOnConfirmRequestSocketData);;

    sysprintf("r\n--- eccCmdSignOnConfirmRequestSocketData len = %d --->\r\n", prDataLen);
    for(int i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- eccCmdSignOnConfirmRequestSocketData ---\r\n");   
    }
    #endif
    
    uint8_t* receiveData;
    uint16_t receiveDataLen = 0;//表示不接收
    if(SocketSend(ECC_SIGN_ON_SERVER_IP, ECC_SIGN_ON_SERVER_PORT, (uint8_t*)&eccCmdSignOnConfirmRequestSocketData, sizeof(eccCmdSignOnConfirmRequestSocketData), &receiveData, &receiveDataLen))
    {
        sysprintf("\r\n     -[info]-> eccSocketSignOnConfirm SocketSend OK: receiveData = [%s],  receiveDataLen = %d!!\n", receiveData, receiveDataLen);
        #if(1)//just send, need not receive
        returnInfo = CARD_MESSAGE_RETURN_SUCCESS; 
        *resultStatus = ECC_CMD_SOCKET_SIGN_ON_CONFIRM_SUCCESS_ID;
        #else
        if(receiveDataLen == sizeof(ECCCmdSignOnConfirmResponseSocketData))
        {
            memcpy(&eccCmdSignOnConfirmResponseSocketData, receiveData, sizeof(ECCCmdSignOnConfirmResponseSocketData));
            uint8_t* prData= (uint8_t*)&eccCmdSignOnConfirmResponseSocketData;
            uint32_t prDataLen = sizeof(eccCmdSignOnConfirmResponseSocketData);;

            *resultStatus = eccCmdSignOnConfirmResponseSocketData.value[1]|(eccCmdSignOnConfirmResponseSocketData.value[0]<<8);
            sysprintf("\r\n--- eccSocketSignOnConfirm  len = %d : status 0x%04x,", prDataLen, *resultStatus);
            for(int i = 0; i<prDataLen; i++)
            {
                //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
                sysprintf("0x%02x, ", prData[i]);
                if(i%10 == 9)
                    sysprintf("\r\n");

            }
            sysprintf("\r\n<--- eccSocketSignOnConfirm ---\r\n");    
            returnInfo = CARD_MESSAGE_RETURN_SUCCESS;            
        }
        else
        {
        }
        #endif
    }
    else
    {
        sysprintf("\r\n     -[info]-> eccSocketSignOnConfirm SocketSend ERROR!!\n");
    }
    return returnInfo;
}

static uint16_t eccSignOnProcess(uint16_t* returnCode, uint16_t* resultStatus, uint32_t utcTime, uint32_t epmUTCTime)
//static uint16_t eccSignOnProcess(uint16_t* returnInfo, uint16_t* returnCode, uint32_t utcTime, uint32_t epmUTCTime)
{
#if(ENABLE_ECC_AUTO_LOAD)
    uint32_t authCreditBalance;
    uint32_t remainderAddQuota;
    uint16_t returnInfo;
    returnInfo =  pprReset(returnCode, resultStatus, utcTime, epmUTCTime, TRUE);
    sysprintf("eccSignOnProcess [STEP 01]:  pprReset (SIGN ON) retun --> returnInfo = 0x%04X, resultStatus = 0x%04X!!\n", returnInfo, *resultStatus);
    if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
    {  
        switch(*resultStatus)
        {
            case ECC_CMD_RESET_SUCCESS_ID:
                returnInfo =  eccSocketSignOnRequest(returnCode, resultStatus, &eccCmdPPRResetResponseData, utcTime, epmUTCTime);
                sysprintf("eccSignOnProcess [STEP 02]:  eccSocketSignOnRequest retun --> returnInfo = 0x%04X, *resultStatus = 0x%04X!!\n", returnInfo, *resultStatus);
                if((returnInfo == CARD_MESSAGE_RETURN_SUCCESS) && (*resultStatus == ECC_CMD_SOCKET_SIGN_ON_REQUEST_SUCCESS_ID))
                {  
                    #if(0)//just for test
                    sysprintf("eccSignOnProcess [STEP 02]:  break...!!\n");
                    break;
                    #endif
                    returnInfo =  eccSignOn(returnCode, resultStatus, &eccCmdSignOnResponseSocketData, utcTime);
                    sysprintf("eccSignOnProcess [STEP 03]:  eccSignOn retun --> returnInfo = 0x%04X, *resultStatus = 0x%04X!!\n", returnInfo, *resultStatus);
                    if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
                    {  
                        switch(*resultStatus)
                        {
                            case ECC_CMD_SIGN_ON_SUCCESS_ID:
                            case ECC_CMD_SIGN_ON_NEED_UPDATE_ID:
                                returnInfo =  eccSocketSignOnConfirm(returnCode, resultStatus, &eccCmdPPRResetResponseData, &eccCmdPPRSignOnResponseData, (uint8_t*)&*resultStatus, utcTime, epmUTCTime);
                                sysprintf("eccSignOnProcess [STEP 04]:  eccSocketSignOnConfirm retun --> returnInfo = 0x%04X, *resultStatus = 0x%04X!!\n", returnInfo, *resultStatus);
                                if((returnInfo == CARD_MESSAGE_RETURN_SUCCESS) && (*resultStatus == ECC_CMD_SOCKET_SIGN_ON_CONFIRM_SUCCESS_ID))
                                {  
                                    returnInfo =  pprReset(returnCode, resultStatus, utcTime, epmUTCTime, FALSE);
                                    sysprintf("eccSignOnProcess [STEP 05]:  pprReset(OFF LINE) retun --> returnInfo = 0x%04X, *resultStatus = 0x%04X!!\n", returnInfo, *resultStatus);
                                    if(returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
                                    {  
                                        switch(*resultStatus)
                                        {
                                            case ECC_CMD_RESET_SUCCESS_ID:                                                
                                                sysprintf("\r\n !!!!! eccSignOnProcess  [STEP FINAL]: SUCCESS !!!!!\n");
                                                break;
                                        }
                                    }
                                }  
                                
                                break;
                            }
                        }

                }
                break;
        }
       
    }
    return returnInfo;
#else
    return CARD_MESSAGE_RETURN_OTHER_ERROR;
#endif
}
#endif
static void eccSaveLockCardLog(uint32_t lockType, uint8_t newSpID, uint8_t svceLocID, char* blkFileName, uint32_t utcTime, uint8_t* DCAReadData, ECCCmdLockCardResponseData* LockCardResponseData)
{
    uint8_t* eccLockCardLogBody = (uint8_t*)pvPortMalloc(sizeof(ECCLockCardLogBody));;    
    ECCLockCardLogContainInit((ECCLockCardLogBody*)eccLockCardLogBody, lockType, newSpID, svceLocID, blkFileName, utcTime, DCAReadData, LockCardResponseData);
#ifdef _PC_ENV_    
    MiscSaveToFile(ECCLockCardLogGetFileName(), (uint8_t*)eccLockCardLogBody, sizeof(ECCLockCardLogBody));
#else
    char targetLogFileName[_MAX_LFN];
    sprintf(targetLogFileName,"%ss", ECCLockCardLogGetFileName()); 

    SFlashAppendRecord(targetLogFileName, SFLASH_RECORD_TYPE_ECC, (uint8_t*)eccLockCardLogBody, sizeof(ECCLockCardLogBody));

    if(FileAgentAddData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCLockCardLogGetFileName(), (uint8_t*)eccLockCardLogBody, sizeof(ECCLockCardLogBody), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
    {

    }   
    #if(USE_SAM_ENCRYPT)
    if(SCEncryptSAMData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCLockCardLogGetFileName(), (uint8_t*)eccLockCardLogBody, sizeof(ECCLockCardLogBody)))
    {
    }
    #endif
#endif
}

static void eccSaveAutoLoadLog(uint8_t newSpID, uint32_t utcTime, uint8_t* DCAReadData, ECCCmdLockCardResponseData* LockCardResponseData)
{
    uint8_t* eccAutoLoadLogBody = (uint8_t*)pvPortMalloc(sizeof(ECCAutoLoadLogBody));
    
    ECCAutoLoadLogContainInit((ECCAutoLoadLogBody*)eccAutoLoadLogBody, TM_AGENT_NUMBER, &eccCmdDCAReadResponseSuccessData, sizeof(eccCmdDCAReadResponseSuccessData), &eccCmdEDCADeductResponseData, sizeof(eccCmdEDCADeductResponseData), newSpID, utcTime);

#ifdef _PC_ENV_    
    MiscSaveToFile(ECCAutoLoadLogGetFileName(), (uint8_t*)eccAutoLoadLogBody, sizeof(ECCAutoLoadLogBody));
#else
    char targetLogFileName[_MAX_LFN];
    sprintf(targetLogFileName,"%ss", ECCAutoLoadLogGetFileName()); 

    SFlashAppendRecord(targetLogFileName, SFLASH_RECORD_TYPE_ECC, (uint8_t*)eccAutoLoadLogBody, sizeof(ECCAutoLoadLogBody));

    if(FileAgentAddData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCAutoLoadLogGetFileName(), (uint8_t*)eccAutoLoadLogBody, sizeof(ECCAutoLoadLogBody), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
    {

    }   
    #if(USE_SAM_ENCRYPT)
    if(SCEncryptSAMData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCAutoLoadLogGetFileName(), (uint8_t*)eccAutoLoadLogBody, sizeof(ECCAutoLoadLogBody)))
    {
    }
    #endif
#endif
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
void ECCLibInit(void)
{
    sysprintf("ECCLibInit(Command):     sizeof(ECCCmdPPRResetRequestData)            = %d (%d)!!\n", sizeof(ECCCmdPPRResetRequestData), ECC_CMD_PPR_RESET_REQUEST_DATA_LEN);
    sysprintf("                         sizeof(ECCCmdPPRResetResponseData)           = %d (%d)!!\n", sizeof(ECCCmdPPRResetResponseData), ECC_CMD_PPR_RESET_RESPONSE_DATA_LEN);
    
    sysprintf("                         sizeof(ECCCmdDCAReadRequestData)            = %d (%d)!!\n", sizeof(ECCCmdDCAReadRequestData), ECC_CMD_DCAR_READ_REQUEST_DATA_LEN);
    sysprintf("                         sizeof(ECCCmdDCAReadResponseSuccessData)    = %d (%d)!!\n", sizeof(ECCCmdDCAReadResponseSuccessData), ECC_CMD_DCAR_READ_RESPONSE_SUCCESS_DATA_LEN );
    sysprintf("                         sizeof(ECCCmdDCAReadResponseError1Data)     = %d (%d)!!\n", sizeof(ECCCmdDCAReadResponseError1Data), ECC_CMD_DCAR_READ_RESPONSE_ERROR_1_DATA_LEN );
    sysprintf("                         sizeof(ECCCmdDCAReadResponseError2Data)     = %d (%d)!!\n", sizeof(ECCCmdDCAReadResponseError2Data), ECC_CMD_DCAR_READ_RESPONSE_ERROR_2_DATA_LEN );
    
    sysprintf("                         sizeof(ECCCmdEDCADeductRequestData)         = %d (%d)!!\n", sizeof(ECCCmdEDCADeductRequestData), ECC_CMD_EDCA_DEDUCT_REQUEST_DATA_LEN );
    sysprintf("                         sizeof(ECCCmdEDCADeductResponseData)        = %d (%d)!!\n", sizeof(ECCCmdEDCADeductResponseData), ECC_CMD_EDCA_DEDUCT_RESPONSE_DATA_LEN );
    
    sysprintf("                         sizeof(ECCCmdLockCardRequestData)           = %d (%d)!!\n", sizeof(ECCCmdLockCardRequestData), ECC_CMD_LOCK_CARD_REQUEST_DATA_LEN );
    sysprintf("                         sizeof(ECCCmdLockCardResponseData)          = %d (%d)!!\n", sizeof(ECCCmdLockCardResponseData), ECC_CMD_LOCK_CARD_RESPONSE_DATA_LEN );
    
    sysprintf("                         sizeof(ECCCmdPPRSignOnRequestData)           = %d (%d)!!\n", sizeof(ECCCmdPPRSignOnRequestData), ECC_CMD_PPR_SIGN_ON_REQUEST_DATA_LEN );
    sysprintf("                         sizeof(ECCCmdPPRSignOnResponseData)          = %d (%d)!!\n", sizeof(ECCCmdPPRSignOnResponseData), ECC_CMD_PPR_SIGN_ON_RESPONSE_DATA_LEN );
    
    
    
    sysprintf("ECCLibInit(Log):         sizeof(ECCLogHeader)                        = %d (%d)!!\n", sizeof(ECCLogHeader), TOTAL_ECC_LOG_HEADER_SIZE);
    sysprintf("                         sizeof(ECCLogTail)                          = %d (%d)!!\n", sizeof(ECCLogTail), TOTAL_ECC_LOG_TAIL_SIZE);
    sysprintf("                         sizeof(ECCDeductLogBody)                    = %d (%d)!!\n", sizeof(ECCDeductLogBody), TOTAL_ECC_DEDUCT_LOG_BODY_SIZE);
    sysprintf("                         sizeof(ECCReSendLogBody)                    = %d (%d)!!\n", sizeof(ECCReSendLogBody), TOTAL_ECC_RESEND_LOG_BODY_SIZE);
    sysprintf("                         sizeof(ECCLockCardLogBody)                  = %d (%d)!!\n", sizeof(ECCLockCardLogBody), TOTAL_ECC_LOCK_CARD_LOG_BODY_SIZE);
    sysprintf("                         sizeof(ECCBlkFeedbackLogBody)               = %d (%d)!!\n", sizeof(ECCBlkFeedbackLogBody), TOTAL_ECC_BLK_FEEDBACK_LOG_BODY_SIZE);
    sysprintf("                         sizeof(ECCAutoLoadLogBody)                  = %d (%d)!!\n", sizeof(ECCAutoLoadLogBody), TOTAL_ECC_AUTO_LOAD_LOG_BODY_SIZE);
    
    sysprintf("ECCLibInit(Sign On Socket):  sizeof(ECCCmdSignOnRequestSocketData)           = %d (%d)!!\n", sizeof(ECCCmdSignOnRequestSocketData), ECC_CMD_SIGN_ON_REQUEST_SOCKET_LEN);
    sysprintf("                             sizeof(ECCCmdSignOnResponseSocketData)          = %d (%d)!!\n", sizeof(ECCCmdSignOnResponseSocketData), ECC_CMD_SIGN_ON_RESPONSE_SOCKET_LEN);
    sysprintf("                             sizeof(ECCCmdSignOnConfirmRequestSocketData)    = %d (%d)!!\n", sizeof(ECCCmdSignOnConfirmRequestSocketData), ECC_CMD_SIGN_ON_CONFIRM_REQUEST_SOCKET_LEN);
    sysprintf("                             sizeof(ECCCmdSignOnConfirmResponseSocketData)   = %d (%d)!!\n", sizeof(ECCCmdSignOnConfirmResponseSocketData), ECC_CMD_SIGN_ON_CONFIRM_RESPONSE_SOCKET_LEN);

}


uint16_t ECCPPRReset(uint16_t* returnInfo, uint16_t* returnCode, uint32_t utcTime, uint32_t epmUTCTime, BOOL SignOnMode)
{
#if(ENABLE_ECC_AUTO_LOAD)
    uint32_t authCreditBalance;
    uint32_t remainderAddQuota;
    uint16_t resultStatus;
    *returnInfo =  pprReset(returnCode, &resultStatus, utcTime, epmUTCTime, FALSE);
 
    sysprintf("ECCPPRReset:  pprReset retun --> *returnInfo = 0x%04X, resultStatus = 0x%04X!!\n", *returnInfo, resultStatus);
    if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
    {  
        switch(resultStatus)
        {
            case ECC_CMD_RESET_SUCCESS_ID:
reCheck:
                sysprintf("ECCPPRReset:  RUN eccSignOnQuery --> *returnInfo = 0x%04X, resultStatus = 0x%04X!!\n", *returnInfo, resultStatus);
                *returnInfo =  eccSignOnQuery(returnCode, &resultStatus, utcTime);
                if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
                {
                    switch(resultStatus)
                    {
                        case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_1:    //0x9000
                        case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_2:    //0x6304
                            break;
                        case ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_3:    //0x6305
                            authCreditBalance = uint8ToUint32(eccCmdPPRSignOnQueryResponseData.authCreditBalance, sizeof(eccCmdPPRSignOnQueryResponseData.authCreditBalance));
                            remainderAddQuota = uint8ToUint32(eccCmdPPRSignOnQueryResponseData.remainderAddQuota, sizeof(eccCmdPPRSignOnQueryResponseData.remainderAddQuota));
                            #if(ENABLE_ECC_LOG_MESSAGE)
                            //#if(0)
                            {
                                char str[512];
                                sprintf(str, "ECCPPRReset:  authCreditBalance= %d, remainderAddQuota = %d!!\n", authCreditBalance, remainderAddQuota);
                                LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
                            }
                            #else                                                        
                            sysprintf("ECCPPRReset:  authCreditBalance= %d, remainderAddQuota = %d!!\n", authCreditBalance, remainderAddQuota);
                            #endif
                            if(remainderAddQuota > (authCreditBalance/2))
                            //if(remainderAddQuota != authCreditBalance)  //just for test  
                            {//額度足夠
                                sysprintf("ECCPPRReset:  quota available!!\n");
                                #if(ENABLE_ECC_AUTO_LOAD)
                                eccRemaiderAmt = TRUE;
                                #endif
                            }
                            else
                            {
                                
                                #if(ENABLE_ECC_AUTO_LOAD)
                                if(SignOnMode)
                                {
                                    sysprintf("ECCPPRReset:  need signon quota, go go go... !!\n");
                                    *returnInfo =  eccSignOnProcess(returnCode, &resultStatus, utcTime, epmUTCTime);
                                    if((*returnInfo == CARD_MESSAGE_RETURN_SUCCESS) && (resultStatus == ECC_CMD_RESET_SUCCESS_ID))
                                    {// 取額度授權成功                                        
                                        sysprintf("ECCPPRReset:  eccSignOnProcess OK, goto reCheck;... !!\n");
                                        DataProcessSetEccNeedSignOnFlag(FALSE);
                                        goto reCheck;
                                    }
                                    else
                                    {// 取額度授權失敗
                                        sysprintf("ECCPPRReset:  eccSignOnProcess ERROR (*returnInfo = 0x%04x, resultStatus = 0x%04x)... !!\n", *returnInfo, resultStatus);
                                        *returnInfo =  pprReset(returnCode, &resultStatus, utcTime, epmUTCTime, FALSE);
                                    }
                                }
                                else
                                {
                                    //#warning add flag (for dataprocesslib)
                                    sysprintf("ECCPPRReset:  need signon quota, but ignore (call DataProcessSetEccNeedSignOnFlag)... !!\n");
                                    DataProcessSetEccNeedSignOnFlag(TRUE);
                                }
                                #else
                                    sysprintf("ECCPPRReset:  need signon quota, but ignore(not support sign on mode)... !!\n");
                                #endif
                            }
                            break;               
                    }
                    
                }
                break;
        }
       
    }
    return *returnInfo;
#else
    uint16_t resultStatus;
    return pprReset(returnCode, &resultStatus, utcTime, epmUTCTime, FALSE);
#endif
    //return edcaDeduct(returnCode, resultStatus, 1,  utcTime, epmUTCTime);
}
BOOL ECCLibProcess(uint16_t* returnInfo, uint16_t* returnCode, uint16_t targetDeduct, tsreaderDepositResultCallback callback, uint8_t* cnData, uint32_t utcTime, uint8_t* machineNo)
{
    uint16_t resultStatus;
    
    //BOOL retval = FALSE;
    // for temp 測試時只做一次
    BOOL retval = TRUE;

    char* blkFileName = "";
    
    //2018.08.14  --> A. 【SEQ_NO_BEF_TXN】交易前序號：扣款兼自動加值，應填自動加值後的序號，卻填自動加值前序號。
    //2018.08.14  --> B. 【EV_BEF_TXN】交易前卡片金額：扣款兼自動加值，應填自動加值後的餘額，卻填自動加值前餘額。     
    cardAutoloadAvailable = FALSE;
    
    *returnInfo = CARD_MESSAGE_RETURN_TIMEOUT;
    *returnCode = CARD_MESSAGE_CODE_NO_USE;
    //*returnInfo = pprReset(returnCode, &resultStatus, utcTime, utcTime);
    //if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
    {  
        *returnInfo = edcaRead(returnCode, &resultStatus, utcTime);
        //just for test
        //resultStatus = ECC_CMD_READ_ERROR_1_ID_1;
        if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
        {  
            switch(resultStatus)
            {
                case 0://???
                    *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR;  
                    *returnCode = resultStatus;
                    if(callback != NULL)
                    {
                        callback(FALSE, *returnInfo, *returnCode);
                    }
                    retval = TRUE;
                    break;

                //0x640E(餘額異常) or 0x6418(通路限制)
                case ECC_CMD_READ_ERROR_1_ID_1:// 0x640E(餘額異常) 
                    *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR;  
                    *returnCode = resultStatus;
                    //寫入鎖卡檔
                    //#warning need check this, there is no "blkFileName" & eccCmdLockCardResponseData in this situation
                    //eccSaveLockCardLog(resultStatus, NEW_SP_ID, SVCE_LOC_ID, NULL, utcTime, (uint8_t*)&eccCmdDCAReadResponseError1Data, NULL);
                    eccSaveLockCardLog(resultStatus, NEW_SP_ID, SVCE_LOC_ID, blkFileName, utcTime, (uint8_t*)&eccCmdDCAReadResponseError1Data, &eccCmdLockCardResponseData);
                    if(callback != NULL)
                    {
                        callback(FALSE, *returnInfo, *returnCode);
                    }
                    retval = TRUE;
                    break;

                case ECC_CMD_READ_ERROR_2_ID_1://0x6103(CPD檢查異常)
                    *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR;  
                    *returnCode = resultStatus;
                    //寫入鎖卡檔
                    //#warning need check this, there is no "blkFileName" & eccCmdLockCardResponseData in this situation
                    //eccSaveLockCardLog(resultStatus, NEW_SP_ID, SVCE_LOC_ID, NULL, utcTime, (uint8_t*)&eccCmdDCAReadResponseError2Data, NULL);
                    eccSaveLockCardLog(resultStatus, NEW_SP_ID, SVCE_LOC_ID, blkFileName, utcTime, (uint8_t*)&eccCmdDCAReadResponseError2Data, &eccCmdLockCardResponseData);
                    if(callback != NULL)
                    {
                        callback(FALSE, *returnInfo, *returnCode);
                    }
                    retval = TRUE;
                    break;

                //0x6418(通路限制)
                case ECC_CMD_READ_ERROR_1_ID_2://0x6418(通路限制)
                    *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR;  
                    *returnCode = resultStatus;
                    //#warning need check here
                    //??
                    if(callback != NULL)
                    {
                        callback(FALSE, *returnInfo, *returnCode);
                    }
                    retval = TRUE;
                    break;

                case ECC_CMD_READ_SUCCESS_ID:
                {
                    /*
                    0x00, 
                    0x19, --> (1) purseUsageControl 
                    0xf4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x8c, 0x53, 0x35, 0x5e, 
                    0xbf, 0x12, 0x00, --> (19) CardMoney
                    0xc3, 0x05, 0x00, 
                    0x08, --> (25) cardType
                    0x00, 0x80, 0xbf, 0x34, 
                    0x5e, 0x01, 
                    0xf0, 0xde, 0x7a, 0x57, --> (32) cardID  0x00, 0x00, 0x00, 
                    0x04, --> (39) cardIDBytes
                    0xc1, 0x30, 0x7c, 0x00, 0x8d, 0x10, 0x03, 0x7c, 0x00, 0x00, 
                    0x7c, 0x7c, 0x00, 0x00, 0x32, 0x32, 0x00, 0x02, 0x32, 0x00, 
                    0x00, 0x00, 0xb4, 0x00, 0x00, 0x4b, 0x61, 0x80, 0x59, 0x30, 
                    0x88, 0x13, 0x00, 0x88, 0x13, 0x00, 0x62, 0x00, 0x00, 0x01, 
                    0x00, 0x4c, 0x3d, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
                    0x23, 0x4c, 0xc2, 0x00, 0x00, 0x8a, 0xc3, 0x32, 0x5a, 0x02, 
                    0x00, 0x00, 0x00, 0xc0, 0x12, 0x00, 0x25, 0x25, 0x02, 0x00, 
                    0x6c, 0x30, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                    0x00, 0x00, 0xfc, 0x7f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                     */
                    uint8_t cardIDBytes;
                    uint8_t CardNo[7];
                    char    CardNoStr[7*2+1];
                    
                    //2018.08.14 處理負值
                    //uint32_t CardMoney;
                    int32_t CardMoney;
                    
                    uint8_t purseUsageControl;
                    uint8_t cardType;
                    #if(ENABLE_ECC_AUTO_LOAD)
                    BOOL condition1 = FALSE;
                    BOOL condition2 = FALSE;
                    #endif
                    //prData = (uint8_t*)&eccCmdDCAReadResponseSuccessData;
                    //prDataLen = sizeof(eccCmdDCAReadResponseSuccessData);
                    memset(CardNo, 0x00, sizeof(CardNo));
                    if(eccCmdDCAReadResponseSuccessData.CardPhysicalIDLength == 0x04)
                    {                        
                        cardIDBytes = 4;
                        sprintf(CardNoStr, "%02X%02X%02X%02X", eccCmdDCAReadResponseSuccessData.CardPhysicalID[0],
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[1],
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[2], 
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[3]);
                    }
                    else
                    {
                        cardIDBytes = 7;
                        sprintf(CardNoStr, "%02X%02X%02X%02X%02X%02X%02X", eccCmdDCAReadResponseSuccessData.CardPhysicalID[0],
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[1],
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[2], 
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[3],
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[4],
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[5], 
                                                    eccCmdDCAReadResponseSuccessData.CardPhysicalID[6]);
                    }
                    memcpy(CardNo, eccCmdDCAReadResponseSuccessData.CardPhysicalID, cardIDBytes);
                    
                    //### edcaRead: cardIDBytes = 4 (F0DE7A57) [0xF0, 0xDE, 0x7A, 0x57, 0x00, 0x00, 0x00]
                    sysprintf(" ### ECCLibProcess [edcaRead]: cardIDBytes = %d (%s) [0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X]\n", cardIDBytes, CardNoStr, CardNo[0], CardNo[1], CardNo[2], CardNo[3], CardNo[4], CardNo[5], CardNo[6]);
 
                    //CardMoney = uint8ToInt32(eccCmdDCAReadResponseSuccessData.PurseBalanceBeforeTXN, sizeof(eccCmdDCAReadResponseSuccessData.PurseBalanceBeforeTXN));
                    CardMoney = uint8ToUint32(eccCmdDCAReadResponseSuccessData.PurseBalanceBeforeTXN, sizeof(eccCmdDCAReadResponseSuccessData.PurseBalanceBeforeTXN));
                    sysprintf(" ### ECCLibProcess [edcaRead]: CardMoney = %d [0x%02X, 0x%02X, 0x%02X]\n", CardMoney, 
                                                                eccCmdDCAReadResponseSuccessData.PurseBalanceBeforeTXN[0], 
                                                                eccCmdDCAReadResponseSuccessData.PurseBalanceBeforeTXN[1], 
                                                                eccCmdDCAReadResponseSuccessData.PurseBalanceBeforeTXN[2]);                    

                    #if(ENABLE_ECC_LOG_MESSAGE)                    
                    {
                    char str[128];
                    sprintf(str, " ### ECCLibProcess [edcaRead]: CardMoney = %d  ###\r\n", CardMoney);
                    LoglibPrintf(LOG_TYPE_INFO, str, FALSE);
                    }
                    #endif
                        
                    //2018.08.14  --> (5) 負值票卡應拒絕交易，畫面無法顯示，設備 Timeout。
                    if(CardMoney < 0)
                    {
                        sysprintf(" ###  ECCLibProcess (CardMoney < 0), Break... !!!\n"); 
                        *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_INSUFFICIENT_MONEY_ERROR;
                        if(callback != NULL)
                        {
                            callback(FALSE, *returnInfo, *returnCode);
                        }
                        retval = TRUE;
                        break;
                    }
                    
#if(ENABLE_ECC_LOCK_CARD)
                    //if (TSCC_IS_BlackCard_2G(tempBlackCard))
                    if(ECCBLKSearchTargetIDByArray(CardNo, cardIDBytes, &blkFileName) != -1)
                    {
                        *returnInfo = eccLockCard(returnCode, &resultStatus, eccCmdDCAReadResponseSuccessData.CardPhysicalID, utcTime);
                        sysprintf(" ### ECCLibProcess [eccLockCard] [%s]: *returnInfo = 0x%02X, resultStatus = 0x%04X\n", blkFileName, *returnInfo, resultStatus);
                        if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
                        {  
                            switch(resultStatus)
                            {
                                case ECC_CMD_LOCK_CARD_SUCCESS_ID:
                                    eccSaveLockCardLog(resultStatus, NEW_SP_ID, SVCE_LOC_ID, blkFileName, utcTime, (uint8_t*)&eccCmdDCAReadResponseSuccessData, &eccCmdLockCardResponseData);
                                    *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_FAILURE_ERROR; ////卡片失效   
                                    if(callback != NULL)
                                    {
                                        callback(FALSE, *returnInfo, *returnCode);
                                    }
                                    retval = TRUE;
                                    return TRUE;
                                    //break;
                            }
                        }
                        else
                        {
                            //#warning NEED CHECK HERE
                            *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_FAILURE_ERROR; ////卡片失效   
                            if(callback != NULL)
                            {
                                callback(FALSE, *returnInfo, *returnCode);
                            }
                            retval = TRUE;
                            return TRUE;
                        }
                    }                    
                    //retval = TRUE;
                    //return TRUE;
                                                               
#endif
                    
                    purseUsageControl = eccCmdDCAReadResponseSuccessData.PurseUsageControl;
                    cardType = eccCmdDCAReadResponseSuccessData.CardType;
                    sysprintf(" ### ECCLibProcess [edcaRead]: purseUsageControl = 0x%02X, cardType = 0x%02X\n", purseUsageControl, cardType);
                    //0x19  1 1001
                    //bit0：是否 Activated；
                    if(purseUsageControl & (0x01<<0)) 
                    {//票卡不適用
                         sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit0): Activated\n");
                    }
                    else
                    {
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit0): not Activated CANT_USE_ERROR\n"); 
                        *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_CANT_USE_ERROR;
                        if(callback != NULL)
                        {                            
                            callback(FALSE, *returnInfo, *returnCode);
                        }
                        retval = TRUE;
                        break;
                    }

                    //bit1：是否 Lock
                    if(purseUsageControl & (0x01<<1)) 
                    {//票卡失效
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit1): Locked FAILURE_ERROR\n");
                        *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_FAILURE_ERROR;
                        if(callback != NULL)
                        {
                            callback(FALSE, *returnInfo, *returnCode);
                        }
                        retval = TRUE;
                        break;
                    }
                    else
                    {
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit1): not Lock\n");
                    }

                    //bit2：是否 Refunded(退卡)；
                    if(purseUsageControl & (0x01<<2)) 
                    {//票卡不適用
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit2): Refunded CANT_USE_ERROR\n"); 
                        *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_CANT_USE_ERROR;
                        if(callback != NULL)
                        {
                            callback(FALSE, *returnInfo, *returnCode);
                        }
                        retval = TRUE;
                        break;
                    }
                    else
                    {
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit2): not Refunded\n");
                    }
                    
                    //bit3：是否允許 Autoload；
                    if(purseUsageControl & (0x01<<3)) 
                    {
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit3): Autoload\n");                        
                    }
                    else
                    {
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit3): can`t Autoload\n");
                    }

                    //bit4：是否允許 Credit；
                    if(purseUsageControl & (0x01<<4)) 
                    {
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit4): Credit\n");
                    }
                    else
                    {//票卡不適用
                        sysprintf(" ### ECCLibProcess [edcaRead] (purseUsageControl : bit4): not Credit CANT_USE_ERROR\n");  
                        *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_CANT_USE_ERROR;
                        if(callback != NULL)
                        {
                            callback(FALSE, *returnInfo, *returnCode);
                        }
                        retval = TRUE;
                        break;
                    }
                    //bit5~bit7：保留
                    
                    if (cardType == 0x09 || cardType == 0x0B || cardType == 0x0C)
                    {// 卡別檢查要不為 09, 0B, 0C
                        sysprintf(" ### ECCLibProcess [edcaRead] (cardType : 0x%02x):  CANT_USE_ERROR !!!\n"); 
                        *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_CANT_USE_ERROR;
                        if(callback != NULL)
                        {
                            callback(FALSE, *returnInfo, *returnCode);
                        }
                        retval = TRUE;
                        break;
                    }
                    else
                    {
                    
                    }
                    #if(ENABLE_ECC_AUTO_LOAD)
                    /**************************************************/
                    /****************  自動加值部分  ******************/
                    // 如果開啟自動加值功能，要檢查卡片是否可加值
                    // === Condition1 票卡的自動加值日限次未超出 (Autoload Date 在當日不可超過日限次數，1次) ===
                    int autoLoadCount = eccCmdDCAReadResponseSuccessData.AutoloadCounter;
                    uint16_t autoloadTimeTemp = ((uint16_t)eccCmdDCAReadResponseSuccessData.AutoloadDate[1] << 8) | eccCmdDCAReadResponseSuccessData.AutoloadDate[0];
                    uint32_t day = (autoloadTimeTemp>>0)&0x1F; //bit 0 - 4 (5 bits)
                    uint32_t month = (autoloadTimeTemp>>5)&0xF; //bit 5 - 8 (4 bits)
                    uint32_t year = ((autoloadTimeTemp>>9)&0x7F)  + 1980;   //bit 9 - 15 (7 bits)                  
                    RTC_TIME_DATA_T time;
                    Time2RTC(utcTime, &time);
                    sysprintf(" ### ECCLibProcess : autoLoadCount = %d [0x%02x, 0x%02x: 0x%04x] (AutoloadDate: %d_%d_%d <-> utcTime: %d_%d_%d) !!!\n", 
                                    autoLoadCount, 
                                    eccCmdDCAReadResponseSuccessData.AutoloadDate[0], eccCmdDCAReadResponseSuccessData.AutoloadDate[1], autoloadTimeTemp, 
                                    year, month, day, time.u32Year, time.u32cMonth, time.u32cDay); 
                    if((year == time.u32Year) && (month == time.u32cMonth) && (day == time.u32cDay))
                    {//上次autoload是今天
                        //2018.08.10 add , autoload 次數 只能一次
                        //if(autoLoadCount <= 0X01)
                        if(autoLoadCount < 0X01)
                        {
                            condition1 = TRUE;
                            sysprintf(" ### ECCLibProcess [condition1 = TRUE] !!!\n");
                        }
                    }
                    else
                    {//上次autoload不是今天
                        #if(1)
                        //2018.08.14  --> 3) 跨日離線自動加值應要能交易。
                        condition1 = TRUE;
                        #else
                        //if(autoLoadCount <= 0X01)
                        if(autoLoadCount < 0X01)
                        {
                            condition1 = TRUE;
                            sysprintf(" ### ECCLibProcess [condition1 = TRUE] !!!\n");
                        }
                        #endif
                    }
                    
                    
                    
                    // === Condition2 票卡最後一次加值紀錄未非當日捷運設備的自動加值交易 ===
                    condition2 = TRUE; // 2017/05/22 ECC 說不用判斷
                    
                    // 票卡可否加值判斷
                    if( (purseUsageControl & (0x01<<3)) && (cardType == 0x08) && eccRemaiderAmt && condition1 && condition2)
                    {
                        sysprintf(" ### ECCLibProcess [cardAutoloadAvailable = TRUE] !!!\n");
                        cardAutoloadAvailable = TRUE;
                    }
                    else
                    {
                        sysprintf(" ### ECCLibProcess [cardAutoloadAvailable = FALSE] !!!\n");
                        cardAutoloadAvailable = FALSE;
                    }
                    
                    /**************************************************/
                    #endif
                    /** 
                     * 說明 : 檢查扣款餘額是否大於扣款金額
                     * 先前檢查的是卡片有沒有自動加值的能力以 card_autoload_available 參數
                     * 以下邏輯是卡片有自動加值功能但是是否真的需要自動加值
                     */
                    if(cardAutoloadAvailable)
                    {
                        if (targetDeduct > CardMoney)
                        {
                            sysprintf(" ###  ECCLibProcess INSUFFICIENT MONEY (AutoLoad) !!!\n"); 
                            cardAutoloadAvailable = TRUE;
                        }
                        else
                        {
                            sysprintf(" ###  ECCLibProcess SUFFICIENT MONEY (AutoLoad) !!!\n"); 
                            cardAutoloadAvailable = FALSE;
                        }
                    }
                    else
                    {
                        //檢查扣款餘額是否大於扣款金額
                        if (targetDeduct > CardMoney)
                        {
                            sysprintf(" ###  ECCLibProcess INSUFFICIENT_MONEY_ERROR (NON AutoLoad) !!!\n"); 
                            *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_INSUFFICIENT_MONEY_ERROR;
                            if(callback != NULL)
                            {
                                callback(FALSE, *returnInfo, *returnCode);
                            }
                            retval = TRUE;
                            break;
                        }
                        sysprintf(" ###  ECCLibProcess SUFFICIENT MONEY (NON AutoLoad) !!!\n"); 
                    }
                    if(cardAutoloadAvailable)
                    {
                        sysprintf(" ### ECCLibProcess FINALLY [cardAutoloadAvailable = TRUE] !!!\n");
                    }
                    else
                    {
                        sysprintf(" ### ECCLibProcess FINALLY [cardAutoloadAvailable = FALSE] !!!\n");
                    }
                    
                    #if(0)//不正式扣款
                    if(callback != NULL)
                    {
                        *returnCode = 5478;
                        callback(TRUE, *returnInfo, *returnCode);
                    }
                    retval = TRUE;
                    return TRUE;
                    #endif


                    *returnInfo = edcaDeduct(returnCode, &resultStatus, targetDeduct, utcTime, utcTime, cardAutoloadAvailable);
                    if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS)
                    {  
                        BOOL write02Log_OK = FALSE;
                        if((resultStatus == ECC_CMD_EDCA_DEDUCT_ERROR_ID_1) || //0x6088
                            (resultStatus == ECC_CMD_EDCA_DEDUCT_ERROR_ID_2))  //0x6424
                        {
                            if(resultStatus == ECC_CMD_EDCA_DEDUCT_ERROR_ID_2) //0x6424
                            {
                                //寫交易 LOG 02-加值交易
                                write02Log_OK = TRUE;
                                eccSaveAutoLoadLog(NEW_SP_ID, utcTime, (uint8_t*)&eccCmdDCAReadResponseError1Data, &eccCmdLockCardResponseData);
                            }
                            // 重做 edcaDeduct
                            utcTime = GetCurrentUTCTime();
                            *returnInfo = edcaDeduct(returnCode, &resultStatus, targetDeduct, utcTime, utcTime, cardAutoloadAvailable);
                        }


                        if(resultStatus == ECC_CMD_EDCA_DEDUCT_SUCCESS_ID)//0x9000
                        {
                            serialNumber++;
                            //CardMoney = uint8ToUint32(eccCmdEDCADeductResponseData.PurseBalance, sizeof(eccCmdEDCADeductResponseData.PurseBalance));
                            CardMoney = uint8ToUint32(eccCmdEDCADeductResponseData.PurseBalance, sizeof(eccCmdEDCADeductResponseData.PurseBalance));
                            sysprintf(" ### ECCLibProcess [edcaDeduct]: targetDeduct = %d, CardMoney = %d, serialNumber = %d [0x%02X, 0x%02X, 0x%02X]\n", 
                                                                        targetDeduct, CardMoney, serialNumber, 
                                                                        eccCmdEDCADeductResponseData.PurseBalance[0], 
                                                                        eccCmdEDCADeductResponseData.PurseBalance[1], 
                                                                        eccCmdEDCADeductResponseData.PurseBalance[2]);
                            
                            
                            //寫交易 LOG 01-扣款  (callback 呼叫 ECCSaveFilePure)          
                            //寫交易 LOG 12-加值重送  (callback 呼叫 ECCSaveFilePure) 
                            if (cardAutoloadAvailable && write02Log_OK == FALSE)
                            {
                                //寫交易 LOG 02-加值交易
                                eccSaveAutoLoadLog(NEW_SP_ID, utcTime, (uint8_t*)&eccCmdDCAReadResponseError1Data, &eccCmdLockCardResponseData);
                            }
                            if(callback != NULL)
                            {
                                *returnCode = CardMoney;
                                callback(TRUE, *returnInfo, *returnCode);
                            }
                            retval = TRUE;
                        }
                        //2018.08.10 add , autoload後無法再autoload
                        else if(resultStatus == ECC_CMD_EDCA_DEDUCT_ERROR_ID_3)
                        {
                            *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR;  
                            *returnCode = resultStatus;
                            if(callback != NULL)
                            {
                                callback(FALSE, *returnInfo, *returnCode);
                            }
                            retval = TRUE;
                        }
                        //2018.10.17 餘額不足
                        else if(resultStatus == 0x6403)
                        {                            
                            *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_INSUFFICIENT_MONEY_ERROR;  
                            *returnCode = resultStatus;
                            if(callback != NULL)
                            {
                                callback(FALSE, *returnInfo, *returnCode);
                            }
                            retval = TRUE;
                        }
                        
                        else //2018.08.14 其他錯誤都直接回應錯誤值
                        {
                            *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR;  
                            *returnCode = resultStatus;
                            if(callback != NULL)
                            {
                                callback(FALSE, *returnInfo, *returnCode);
                            }
                            retval = TRUE;
                        }
                    }
                    
                }
                break; //case ECC_CMD_READ_SUCCESS_ID:
            
            default:
                *returnInfo = CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR;  
                *returnCode = resultStatus;
                //#warning need check here
                //??
                if(callback != NULL)
                {
                    callback(FALSE, *returnInfo, *returnCode);
                }
                retval = TRUE;
                break;

            }  
        }// READ // if(*returnInfo == CARD_MESSAGE_RETURN_SUCCESS) 
    
    }    
    return retval;
}
void ECCSaveFile(uint16_t currentTargetDeduct, time_t epmUTCTime)
{     
    uint8_t* eccDeductLogBody = (uint8_t*)pvPortMalloc(sizeof(ECCDeductLogBody));  
    //2018.08.14  --> A. 【SEQ_NO_BEF_TXN】交易前序號：扣款兼自動加值，應填自動加值後的序號，卻填自動加值前序號。
    //2018.08.14  --> B. 【EV_BEF_TXN】交易前卡片金額：扣款兼自動加值，應填自動加值後的餘額，卻填自動加值前餘額。 
    //ECCDeductLogContainInit((ECCDeductLogBody*)eccDeductLogBody, currentTargetDeduct, 1, epmUTCTime, FALSE, &eccCmdDCAReadResponseSuccessData, sizeof(eccCmdDCAReadResponseSuccessData), &eccCmdEDCADeductResponseData, sizeof(eccCmdEDCADeductResponseData));
    ECCDeductLogContainInit((ECCDeductLogBody*)eccDeductLogBody, currentTargetDeduct, TM_AGENT_NUMBER, epmUTCTime, cardAutoloadAvailable, &eccCmdDCAReadResponseSuccessData, sizeof(eccCmdDCAReadResponseSuccessData), &eccCmdEDCADeductResponseData, sizeof(eccCmdEDCADeductResponseData));
    
    uint8_t* eccReSendLogBody = (uint8_t*)pvPortMalloc(sizeof(ECCReSendLogBody));
    //2018.08.14  --> A. 【SEQ_NO_BEF_TXN】交易前序號：扣款兼自動加值，應填自動加值後的序號，卻填自動加值前序號。
    //2018.08.14  --> B. 【EV_BEF_TXN】交易前卡片金額：扣款兼自動加值，應填自動加值後的餘額，卻填自動加值前餘額。 
    //ECCReSendLogContainInit((ECCReSendLogBody*)eccReSendLogBody, currentTargetDeduct, 1, epmUTCTime, FALSE, &eccCmdDCAReadResponseSuccessData, sizeof(eccCmdDCAReadResponseSuccessData), &eccCmdEDCADeductResponseData, sizeof(eccCmdEDCADeductResponseData));
    ECCReSendLogContainInit((ECCReSendLogBody*)eccReSendLogBody, currentTargetDeduct, TM_AGENT_NUMBER, epmUTCTime, cardAutoloadAvailable, &eccCmdDCAReadResponseSuccessData, sizeof(eccCmdDCAReadResponseSuccessData), &eccCmdEDCADeductResponseData, sizeof(eccCmdEDCADeductResponseData));
    
#ifdef _PC_ENV_
    MiscSaveToFile(ECCDeductLogGetFileName(), (uint8_t*)eccDeductLogBody, sizeof(ECCDeductLogBody));
    MiscSaveToFile(ECCReSendLogGetFileName(), (uint8_t*)eccReSendLogBody, sizeof(ECCReSendLogBody));
#else
    uint8_t* pData;
    char targetLogFileName[_MAX_LFN];
    
    sprintf(targetLogFileName,"%ss", ECCDeductLogGetFileName()); 
    SFlashAppendRecord(targetLogFileName, SFLASH_RECORD_TYPE_ECC, (uint8_t*)eccDeductLogBody, sizeof(ECCDeductLogBody));
    if(FileAgentAddData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCDeductLogGetFileName(), (uint8_t*)eccDeductLogBody, sizeof(ECCDeductLogBody), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
    {

    }    
    #if(USE_SAM_ENCRYPT)
    if(SCEncryptSAMData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCDeductLogGetFileName(), (uint8_t*)eccDeductLogBody, sizeof(ECCDeductLogBody)))
    {
    }
    #endif
    
    sprintf(targetLogFileName,"%ss", ECCReSendLogGetFileName()); 
    SFlashAppendRecord(targetLogFileName, SFLASH_RECORD_TYPE_ECC, (uint8_t*)eccReSendLogBody, sizeof(ECCReSendLogBody));
    
    if(FileAgentAddData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCReSendLogGetFileName(), (uint8_t*)eccReSendLogBody, sizeof(ECCReSendLogBody), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
    {

    }   
    #if(USE_SAM_ENCRYPT)
    if(SCEncryptSAMData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCReSendLogGetFileName(), (uint8_t*)eccReSendLogBody, sizeof(ECCReSendLogBody)))
    {
    }
    #endif
#if(0)
    //ReadResponse
    pData = (uint8_t*)pvPortMalloc(sizeof(ECCCmdDCAReadResponseSuccessData));
    memcpy(pData, &eccCmdDCAReadResponseSuccessData, sizeof(ECCCmdDCAReadResponseSuccessData));
    sprintf(targetLogFileName,"%s.%s", ECCDeductLogGetFileName(), DSF_FILE_EXTENSION);     
    if(FileAgentAddData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, targetLogFileName, (uint8_t*)pData, sizeof(ECCCmdDCAReadResponseSuccessData), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
    {

    }  
    
    //DeductResponse
    pData = (uint8_t*)pvPortMalloc(sizeof(ECCCmdEDCADeductResponseData));
    memcpy(pData, &eccCmdEDCADeductResponseData, sizeof(ECCCmdEDCADeductResponseData));
    sprintf(targetLogFileName,"%s.%s", ECCDeductLogGetFileName(), DCF_FILE_EXTENSION);     
    if(FileAgentAddData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, targetLogFileName, (uint8_t*)pData, sizeof(ECCCmdEDCADeductResponseData), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
    {

    } 
#endif   
#endif //#if(ENABLE_CARD_LOG_SAVE)
}
void ECCSaveFilePure(uint16_t currentTargetDeduct, time_t epmUTCTime)
{
    uint8_t* eccDeductLogBody = (uint8_t*)pvPortMalloc(sizeof(ECCDeductLogBody));  
    ECCDeductLogContainInit((ECCDeductLogBody*)eccDeductLogBody, currentTargetDeduct, TM_AGENT_NUMBER, epmUTCTime, cardAutoloadAvailable, &eccCmdDCAReadResponseSuccessData, sizeof(eccCmdDCAReadResponseSuccessData), &eccCmdEDCADeductResponseData, sizeof(eccCmdEDCADeductResponseData));
    #ifdef _PC_ENV_
    MiscSaveToFile(ECCDeductLogGetFileName(), (uint8_t*)eccDeductLogBody, sizeof(ECCDeductLogBody));
    #else
    vPortFree(eccDeductLogBody);  
    #endif
    
    uint8_t* eccReSendLogBody = (uint8_t*)pvPortMalloc(sizeof(ECCReSendLogBody));
    ECCReSendLogContainInit((ECCReSendLogBody*)eccReSendLogBody, currentTargetDeduct, TM_AGENT_NUMBER, epmUTCTime, cardAutoloadAvailable, &eccCmdDCAReadResponseSuccessData, sizeof(eccCmdDCAReadResponseSuccessData), &eccCmdEDCADeductResponseData, sizeof(eccCmdEDCADeductResponseData));
    #ifdef _PC_ENV_
    MiscSaveToFile(ECCReSendLogGetFileName(), (uint8_t*)eccReSendLogBody, sizeof(ECCReSendLogBody));
    #else
    vPortFree(eccReSendLogBody);  
    #endif
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


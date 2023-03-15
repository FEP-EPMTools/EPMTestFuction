/**************************************************************************//**
* @file     ecclog.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
/*
001:Log
012:Resend
034:LockCard
052:BlkFeedback
002:AutoLoad
*/

#define ECC_LOG_USE_NEW_FILE_NAME 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _PC_ENV_
    #include "misc.h"
    #include "ecccmd.h"
    #include "ecclog.h"
    #include "cardlogcommon.h"
    #define  sysprintf       miscPrintf
#else
    #include "nuc970.h"
    #include "sys.h"

    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "ff.h"

    #include "fepconfig.h"
    #include "ecccmd.h"
    #include "ecclog.h"
    #include "cardlogcommon.h"
    #include "meterdata.h"
    #include "timelib.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
//static ECCDeductLogBody eccLog;
static char currentECCDeductLogFileName[_MAX_LFN];
static char currentECCReSendLogFileName[_MAX_LFN];
static char currentECCLockCardLogFileName[_MAX_LFN];
static char currentECCFeedbackLogFileName[_MAX_LFN];
static char currentECCAutoLoadLogFileName[_MAX_LFN];
static char currentDateTimeStr[32];
//static uint32_t DPTITraceNo = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static uint32_t Spid, NewSpID, NewLocationID; 
static uint32_t uint8ToUint32(uint8_t* data, uint8_t dataLen)
{
    uint32_t reVal = 0;
    if(dataLen > sizeof(uint32_t))
        return 0;
    for(int i = 0; i < dataLen; i++)
    {
        reVal = reVal | ((uint32_t)(data[i])<<(8*i));
    }
    return reVal;
}
#if(0)
static uint8_t uint32ToUint8Inverse(uint32_t src, uint8_t* data, uint8_t dataLen)
{
    if(dataLen > sizeof(uint32_t))
        return 0;
    //sysprintf(" --- ecclog (uint32ToUint8): %d (len = %d)\r\n", src, dataLen);
    for(int i = 0; i < dataLen; i++)
    {
        data[dataLen - 1 - i] = (src>>(8*i))&0xff;
    }
    return dataLen;
}
#endif
static uint8_t uint32ToUint8(uint32_t src, uint8_t* data, uint8_t dataLen)
{
    if(dataLen > sizeof(uint32_t))
        return 0;
    //sysprintf(" --- ecclog (uint32ToUint8): %d (len = %d)\r\n", src, dataLen);
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

static char* getECCLogUTCTimeStr(uint32_t utcTime)
{
    RTC_TIME_DATA_T time;
    Time2RTC(utcTime, &time);  
    sprintf(currentDateTimeStr, "%04d%02d%02d%02d%02d%02d", time.u32Year, time.u32cMonth, time.u32cDay, time.u32cHour, time.u32cMinute, time.u32cSecond);
    //sysprintf("getECCCmdUTCTimeStr :[%d, %X][%s]\r\n", utcTime, utcTime, tmp);  
    return currentDateTimeStr;
}

static void getECCLogTimeData(uint32_t time, ECCLogDataTime* eccLogDataTime)
{
    //sysprintf("getECCLogTimeData :[%d, %X]\r\n", time, time);  
    for(int i = 0; i < sizeof(ECCLogDataTime); i++)
    {
        eccLogDataTime->value[i] = (time>>(8*i))&0xff;
    }
}

char *getExtension(char *fileName)
{
    int len = strlen(fileName);
    int i = len;
    while( fileName[i]!='.' && i>0 )
    { 
        i--; 
    }

    if(fileName[i]=='.')
    {
        return &fileName[i+1];
    }
    else
    {
        return &fileName[len];
    }
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL ECCDeductLogContainInit(ECCDeductLogBody* body, uint32_t deductValue, uint32_t AgentNo, uint32_t utcTime, BOOL cardAutoloadAvailable, ECCCmdDCAReadResponseSuccessData* DCAReadData, int DCAReadDataLen, ECCCmdEDCADeductResponseData* EDCADeductData, int EDCADeductDataLen)
{
    #if(1)
    return TRUE;
    #else
    uint32_t i;
    uint8_t* prData; 
    uint32_t prDataLen;
    ECCDeductLogBody* pECCDeductLogBody = body;
    #if(!ECC_LOG_USE_NEW_FILE_NAME)
    uint32_t SP_ID_INT, SP_ID_INT_NEW, LOCATION_ID_INT_NEW;
    #endif
    uint8_t PVN = DCAReadData->PurseVersionNumber;
    sysprintf("\r\n====  ECCDeductLogContainInit(%s), cardAutoloadAvailable:%d ====\r\n", GetMeterData()->epmIdStr, cardAutoloadAvailable);
    sysprintf("sizeof(ECCDeductLogBody) = %d\r\n", sizeof(ECCDeductLogBody));

    if(sizeof(ECCDeductLogBody) != TOTAL_ECC_DEDUCT_LOG_BODY_SIZE)
    {
        sysprintf("ECCDeductLogContainInit ERROR: sizeof(ECCDeductLogBody) != TOTAL_ECC_DEDUCT_LOG_BODY_SIZE [%d, %d]\r\n", sizeof(ECCDeductLogBody), TOTAL_ECC_DEDUCT_LOG_BODY_SIZE);
        return FALSE;
    }
    memset(pECCDeductLogBody, 0x00, sizeof(ECCDeductLogBody));
    //DPTITraceNo++;
    // ********   Header   ********
    pECCDeductLogBody->Header.PurseVerNo = DCAReadData->PurseVersionNumber;         //1 PURSE_VER_NO 卡片版本 Unsigned 1 代碼參照 8.1 來源: 設備
    pECCDeductLogBody->Header.CardTxnTypeID = EDCADeductData->MsgType;              //2 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    
    pECCDeductLogBody->Header.CardTxnSubTypeID = DCAReadData->PersonalProfile;      //3 CARD_TXN_SUBTYPE_ID 交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3 //交易次類別 (MsgType = 0x01 時，SubType = Personal Profile)
    pECCDeductLogBody->Header.DevID[0] = DCAReadData->DeviceID[2];                  //4 DEV_ID 設備編號 Unsigned 3 SP_ID(1 Byte):如 122 即為 0x7A
    pECCDeductLogBody->Header.DevID[1] = DCAReadData->DeviceID[0];                                                    //DEV_TYPE(4 Bits): 3：BV 如 BV 即為 0x3□
    pECCDeductLogBody->Header.DevID[2] = DCAReadData->DeviceID[1];                                                    //DEV_ID_NO(12 Bites): 如 1234 即為 0x□4 0xD2
                                                                                                                    //組合 HEX: 0x7A 0x34 0xD2
                                                                                                                    //倒數2 Bytes LSB FIRST: 0x7A 0xD2 0x34
                                                                                                                    //來源: Mifare Sam 卡
    pECCDeductLogBody->Header.SpID = DCAReadData->ServiceProviderID;                //5 SP_ID 業者代碼 Unsigned 1  如 122 即為 0x7A 來源: 設備
    Spid = uint8ToUint32((uint8_t*)&(pECCDeductLogBody->Header.SpID), sizeof(pECCDeductLogBody->Header.SpID));
    sysprintf("ECCDeductLogContainInit Spid = %d\r\n", Spid);

    memcpy(&(pECCDeductLogBody->Header.TxnTimeStamp), &(EDCADeductData->TXNDateTime), sizeof(pECCDeductLogBody->Header.TxnTimeStamp));  //[以 EDCADeductBytes 的為準] //6 TXN_TIMESTAMP 交易時間 UnixTime 4 來源: 設備
    memcpy(pECCDeductLogBody->Header.CardPhysicalID, DCAReadData->CardPhysicalID, sizeof(pECCDeductLogBody->Header.CardPhysicalID));    //7 CARD_PHYSICAL_ID 卡片晶片號碼 Unsigned 7 來源: 設備
    pECCDeductLogBody->Header.IssuerID = DCAReadData->IssuerCode;                   //8 ISSUER_ID 發卡單位 Unsigned 1   PVN <> 0 時: 來源: Cpu 卡
                                                                                                                      //PVN = 0 時: 2 (0x02)：悠遊卡公司
                                                                                                                                    //66 (0x42)：基隆交通卡
                                                                                                                                    //來源: 設備
    pECCDeductLogBody->Header.CardTxnSeqNo = EDCADeductData->TSQN[0];               //只有一個byte   //9 CARD_TXN_SEQ_NO 交易後序號 Unsigned 1 來源: 卡片
    getMoneyToUint8(deductValue, pECCDeductLogBody->Header.TxnAMT, sizeof(pECCDeductLogBody->Header.TxnAMT));   //10 TXN_AMT 交易金額 Signed 3 來源: 設備
    memcpy(pECCDeductLogBody->Header.ElectronicValue, EDCADeductData->PurseBalance, sizeof(pECCDeductLogBody->Header.ElectronicValue)); //11 ELECTRONIC_VALUE 交易後卡片金額 Signed 3 來源: 卡片
    pECCDeductLogBody->Header.SVCELocID = DCAReadData->LocationID;                      //12 SVCE_LOC_ID 場站代碼 Unsigned 1 來源: 設備
    pECCDeductLogBody->Header.CardPhysicalIDLen = DCAReadData->CardPhysicalIDLength;    //13 CARD_PHYSICAL_ID_LEN 晶片號碼長度 Unsigned 1 如 4 bytes 即為 0x04
                                                                                                    //如 7 bytes 即為 0x07
                                                                                                    //來源: 卡片
    memcpy(pECCDeductLogBody->Header.NewCardTxnSeqNo, EDCADeductData->TSQN, sizeof(pECCDeductLogBody->Header.NewCardTxnSeqNo));     //14 NEW_CARD_TXN_SEQ_NO 新交易後序號 Unsigned 3 來源: 卡片
    memcpy(pECCDeductLogBody->Header.NewDevID, DCAReadData->NewDeviceID, sizeof(pECCDeductLogBody->Header.NewDevID));               //15 NEW_DEV_ID 新設備編號 Unsigned 6           NEW_SP_ID(3 Bytes): 如 122 即為0x00 0x00 0x7A
                                                                                                //NEW_DEV_TYPE(1 Byte): 3：BV 如 3 即為 0x03
                                                                                                //4096(含)之後開始編碼
                                                                                                //NEW_DEV_ID_NO (2 Bytes): 如 4096 即為 0x10 0x00
                                                                                                //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
                                                                                                //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                                                                //來源: Cpu Sam 卡
                                                                                                //二代卡設備未修改，以 SIS2 傳送時: 固定填 6 Bytes 0x00
    memcpy(pECCDeductLogBody->Header.NewSpID, DCAReadData->NewServiceProviderID, sizeof(pECCDeductLogBody->Header.NewSpID));    //16 NEW_SP_ID 新業者代碼 Unsigned 3 NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
    NewSpID = uint8ToUint32((uint8_t*)&(pECCDeductLogBody->Header.NewSpID), sizeof(pECCDeductLogBody->Header.NewSpID));                              //以 LSB FIRST 傳送: 0x7A 0x00 0x00
    sysprintf("ECCDeductLogContainInit NewSpID = %d\r\n", NewSpID);                                                                         //來源: 設備
                                                                                                                                    
    memcpy(pECCDeductLogBody->Header.NewSVCELocID, DCAReadData->NewLocationID, sizeof(pECCDeductLogBody->Header.NewSVCELocID));     //17 NEW_SVCE_LOC_ID 新場站代碼 Unsigned 2 NEW_SVCE_LOC_ID(2 Bytes): 如 1 即為 0x00 0x01
    NewLocationID = uint8ToUint32((uint8_t*)&(pECCDeductLogBody->Header.NewSVCELocID), sizeof(pECCDeductLogBody->Header.NewSVCELocID));                  //以 LSB FIRST 傳送: 0x01 0x00
    sysprintf("ECCDeductLogContainInit NewLocationID = %d\r\n", NewLocationID);                                                             //來源: 設備                         
                                                                                    
    pECCDeductLogBody->Header.NewCardTxnSubTypeID = 0x00;        //18 NEW_CARD_TXN_SUBTYPE_ID 新交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    pECCDeductLogBody->Header.NewPersonalProfile = DCAReadData->PersonalProfile;         //19 NEW_PERSONAL_PROFILE 新身份別 Unsigned 1 0x00: 一般
                                                                                            //0x01: 敬老 1
                                                                                            //0x02: 敬老 2
                                                                                            //0x03: 愛心
                                                                                            //0x04: 陪伴
                                                                                            //0x05: 學生
                                                                                            //0x08: 優待
                                                                                            //來源: 卡片
    pECCDeductLogBody->Header.TxnPersonalProfile = 0x00;         //20 TXN_PERSONAL_PROFILE 交易身份別 Unsigned 1 實際交易的身份別，如卡片身份別為學生，但部份業者未給予優待，以一般卡扣款，則此欄位填入一般
                                                                                            //0x00: 一般
                                                                                            //0x01: 敬老 1
                                                                                            //0x02: 敬老 2
                                                                                            //0x03: 愛心
                                                                                            //0x04: 陪伴
                                                                                            //0x05: 學生
                                                                                            //0x08: 優待
                                                                                            //來源: 設備
    pECCDeductLogBody->Header.ACQID = 0x02;                      //21 ACQ_ID 收單單位 Unsigned 1   2 (0x02)：悠遊卡公司
                                                                                //66 (0x42)：基隆交通卡
                                                                                //來源: 設備
    memcpy(pECCDeductLogBody->Header.CardPurseID, &(DCAReadData->PID), sizeof(pECCDeductLogBody->Header.CardPurseID)); //22 CARD_PURSE_ID 錢包編號(外觀號碼) Pack 8      PVN <> 0 時:
                                                                                                                                                                            //如卡號 1234567890123456
                                                                                                                                                                            //即為 0x12 0x34 0x56 0x78 0x90 0x12 0x34 0x56
                                                                                                                                                                            //編碼參照 8.5
                                                                                                                                                                            //來源: Cpu 卡
                                                                                                                                                                        //PVN = 0 時:
                                                                                                                                                                            //固定填 8 Bytes 0x00

    if (PVN == 0x00)                                                 //23 CARD_CTC_SEQ_NO 卡片處理序號 Unsigned Msb 3   PVN <> 0 時: 如 1 即為 0x00 0x00 0x01  
    {                                                                                                                   //以 MSB FIRST 傳送:  0x00 0x00 0x01                
        memset(pECCDeductLogBody->Header.CardCTCSeqNo, 0x00, sizeof(pECCDeductLogBody->Header.CardCTCSeqNo));           //來源: Cpu 卡                                      
    }                                                                                                                   //PVN = 0 時: 固定填 3 Bytes 0x00                          
    else
    {
        memcpy(pECCDeductLogBody->Header.CardCTCSeqNo, EDCADeductData->CTC, sizeof(pECCDeductLogBody->Header.CardCTCSeqNo));
    }
    
              
    pECCDeductLogBody->Header.AreaCode = DCAReadData->AreaCode;          //24 AREA_CODE 區碼 Unsigned 1 來源: 卡片
    
    if (PVN == 0x00)                                                     //25 SUB_AREA_CODE 附屬區碼 Unsigned 2    PVN <> 0 時: 依 3 碼郵遞區號  來源: Cpu 卡     
    {                                                                                                              //PVN = 0 時: 固定填 2 Bytes 0x00              
        memset(pECCDeductLogBody->Header.SubAreaCode, 0x00, sizeof(pECCDeductLogBody->Header.SubAreaCode)); 
    }                                                                                                       
    else
    {
        memcpy(pECCDeductLogBody->Header.SubAreaCode, DCAReadData->SubAreaCode, sizeof(pECCDeductLogBody->Header.SubAreaCode));
    }

    //26 SEQ_NO_BEF_TXN 交易前序號 Unsigned 3  來源: 卡片  二代卡設備未修改，以 SIS2 傳送時: 固定填 3 Bytes 0x00    
    //27 EV_BEF_TXN 交易前卡片金額 Signed 3 來源: 卡片                                                               
    if (cardAutoloadAvailable)
    {//抓扣款後自動加值的
         //交易前序號
         memcpy(pECCDeductLogBody->Header.SeqNoBefTxn, EDCADeductData->AutoLoadTSQN, sizeof(pECCDeductLogBody->Header.SeqNoBefTxn));

        //交易前卡片金額
        memcpy(pECCDeductLogBody->Header.EVBdfTxn, EDCADeductData->AutoLoadPurseBalance, sizeof(pECCDeductLogBody->Header.EVBdfTxn));
    }
    else
    {//抓讀卡的
        //交易前序號
        memcpy(pECCDeductLogBody->Header.SeqNoBefTxn, DCAReadData->TXNSNBeforeTXN, sizeof(pECCDeductLogBody->Header.SeqNoBefTxn));

        //交易前卡片金額
        memcpy(pECCDeductLogBody->Header.EVBdfTxn, DCAReadData->PurseBalanceBeforeTXN, sizeof(pECCDeductLogBody->Header.EVBdfTxn));
    }
    #if(1)
    prData = (uint8_t*)&(pECCDeductLogBody->Header);
    prDataLen = sizeof(pECCDeductLogBody->Header);
    sysprintf("--- pECCDeductLogBody->Header:  len = %d --->\r\n", prDataLen);
    for(i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- pECCDeductLogBody->Header ---\r\n");
    #endif
    // ********   Data   ********
    //2018.08.14  --> C. 【XFER_CODE】轉乘代碼：CPU_L2卡應填 06，卻填 00。    
    sysprintf("\r\n====  ECCDeductLogContainInit: TransferGroupCode = 0x%02x, NewTransferGroupCode = 0x%02x, 0x%02x ====\r\n", 
                        EDCADeductData->TransferGroupCode, EDCADeductData->NewTransferGroupCode[0], EDCADeductData->NewTransferGroupCode[1]);    
    if (PVN != 0x00) //CPU_L2卡應填 06, 根據 NewTransferGroupCode[2] 來判斷 因為這時候都是 00
    {
        uint8_t prevTransferGroupCode = 0x00;
        if(EDCADeductData->NewTransferGroupCode[0] > 0xF)
        {
            prevTransferGroupCode = prevTransferGroupCode | 0xF0;
        }
        else
        {
            prevTransferGroupCode = prevTransferGroupCode | ((EDCADeductData->NewTransferGroupCode[0] & 0x0F) << 4);
        }   

        if(EDCADeductData->NewTransferGroupCode[1] > 0xF)
        {
            prevTransferGroupCode = prevTransferGroupCode | 0x0F;
        }
        else
        {
            prevTransferGroupCode = prevTransferGroupCode | (EDCADeductData->NewTransferGroupCode[1] & 0x0F);
        }

        pECCDeductLogBody->XferCODE = prevTransferGroupCode;            //4 XFER_CODE 轉乘代碼 Byte 1 bit7..4: 上筆轉乘代碼
                                                                            //bit3..0: 本次轉乘代碼
                                                                            //如捷運轉公車: 0x12
                                                                            //來源: 設備
                                                                            //預設填 0x00
           

    }
    else
    {
        pECCDeductLogBody->XferCODE = EDCADeductData->TransferGroupCode; //4 XFER_CODE 轉乘代碼 Byte 1 bit7..4: 上筆轉乘代碼
                                                                            //bit3..0: 本次轉乘代碼
                                                                            //如捷運轉公車: 0x12
                                                                            //來源: 設備
                                                                            //預設填 0x00
    }
    sysprintf("\r\n====  ECCDeductLogContainInit: XferCODE = 0x%02x ====\r\n", pECCDeductLogBody->XferCODE);
    memcpy(pECCDeductLogBody->NewXferCODE, EDCADeductData->NewTransferGroupCode, sizeof(pECCDeductLogBody->NewXferCODE));//5 NEW_XFER_CODE 新轉乘代碼 Byte 2
                                                                                                                            //Byte1: 上筆轉乘代碼
                                                                                                                            //Byte2: 本次轉乘代碼
                                                                                                                            //如捷運轉公車: 0x01 0x02
                                                                                                                            //來源: 設備
                                                                                                                            //預設填 2 Bytes 0x00
    getMoneyToUint8(deductValue, pECCDeductLogBody->OriAMT, sizeof(pECCDeductLogBody->OriAMT));   //6 ORI_AMT 原價(未打折) Unsigned 3
                                                                                                                    //計算參照 8.8
                                                                                                                    //來源: 設備
    
    pECCDeductLogBody->DisRate = 0x64;          //7 DIS_RATE 費率 Unsigned 1  如 85 折，DIS_RATE=85 即為 0x55
                                                    //如未打折，DIS_RATE=100 即為 0x64
                                                    //計算參照 8.8
                                                    //來源: 設備
    getMoneyToUint8(deductValue, pECCDeductLogBody->TicketAMT, sizeof(pECCDeductLogBody->TicketAMT)); //index 14   //8 TICKET_AMT 票價(打折後) Unsigned 3
                                                                                                            //計算參照 8.8
                                                                                                            //來源: 設備
    memcpy(pECCDeductLogBody->LoyaltyCounter, DCAReadData->LoyaltyCounter, sizeof(pECCDeductLogBody->LoyaltyCounter));//17 LOYALTY_COUNTER 累積忠誠點 Unsigned 2
                                                                                                                            //來源: 設備
                                                                                                                            //預設填 2 Bytes 0x00
//#warning different from leon sample
    //2018.08.14  --> D. 【AGENT_NO】操作員代碼：應與加值LOG欄位相同。
    uint32ToUint8(AgentNo, pECCDeductLogBody->AgentNo, sizeof(pECCDeductLogBody->AgentNo));//18 AGENT_NO 操作員代碼 Unsigned 2
                                                                                                        //如 Ascii 字元'1234'即為 0x31 0x32 0x33 0x34
                                                                                                        //轉 10 進制數值 1234 即為 0x04 0xD2
                                                                                                        //以 LSB FIRST 傳送: 0xD2 0x04
                                                                                                        //來源: 設備
                                                                                                        //此欄位 10 進制數值應與 PPR_Reset/PPR_Reset_Online 一致
                                                                                                        //預設填 2 Bytes 0x00
    if (PVN != 0x00) 
    {
        memset(pECCDeductLogBody->CarPlateNo, 0x20, sizeof(pECCDeductLogBody->CarPlateNo));     //20 CAR_PLATE_NO 車牌號碼 Ascii 10 來源: 設備
        
        //2018.08.14 -->  F. 【RFU1-XFER_PRE_SEQ_NO】轉乘上筆交易序號：應填轉乘上筆交易序號，卻填 0。
        pECCDeductLogBody->RFU1XferPreSeqNo[0] = DCAReadData->TSQNofURT[2];
        pECCDeductLogBody->RFU1XferPreSeqNo[1] = DCAReadData->TSQNofURT[1];
        pECCDeductLogBody->RFU1XferPreSeqNo[2] = DCAReadData->TSQNofURT[0];
        //2018.08.14 --> G.【RFU1-CARD_TYPE】票別：應填票卡卡種，卻填 0。
        pECCDeductLogBody->RFU1CardType = DCAReadData->CardType;
        //2018.08.14 --> H.【RFU1-XFER_PRE_TIMESTAMP】上筆交易時間：應填轉乘上筆交易時間，卻都填 1970/1/1 00:00:00。
        //memcpy(pECCDeductLogBody->RFU1XferPreTimeStamp, DCAReadData->TXNDateTimeofURT, sizeof(pECCDeductLogBody->RFU1XferPreTimeStamp));
        pECCDeductLogBody->RFU1XferPreTimeStamp[0] = DCAReadData->TXNDateTimeofURT[3];
        pECCDeductLogBody->RFU1XferPreTimeStamp[1] = DCAReadData->TXNDateTimeofURT[2];
        pECCDeductLogBody->RFU1XferPreTimeStamp[2] = DCAReadData->TXNDateTimeofURT[1];
        pECCDeductLogBody->RFU1XferPreTimeStamp[3] = DCAReadData->TXNDateTimeofURT[0];

        
        
        //2018.08.14  --> I.【RFU1_VER】欄位版本：應填 01，卻填 00。
        pECCDeductLogBody->RFU1Ver = 0x01;                //23 RFU1_VER 欄位版本(共用) Unsigned 1
                                                                //PVN <> 0 時:
                                                                    //固定填 0x00
                                                                //PVN = 0 時:
                                                                    //固定填 0x00
        
        //2018.08.14 --> J.【RFU2-SUB_LOC_ID】附屬場站代碼：應填 50，卻填 00
        //memcpy(pECCDeductLogBody->RFU2SubLocId, DCAReadData->NewLocationID, sizeof(pECCDeductLogBody->RFU2SubLocId));
        sysprintf("\r\n====  ECCDeductLogContainInit: (1) RFU2SubLocId = [0x%02x, 0x%02x], LocationID = 0x%02x, NewLocationID = [0x%02x, 0x%02x] ====\r\n", 
                    pECCDeductLogBody->RFU2SubLocId[0], pECCDeductLogBody->RFU2SubLocId[1], DCAReadData->LocationID, DCAReadData->NewLocationID[0], DCAReadData->NewLocationID[1]);
        //pECCDeductLogBody->RFU2SubLocId[0] = DCAReadData->NewLocationID[1];
        //pECCDeductLogBody->RFU2SubLocId[1] = DCAReadData->NewLocationID[0];
        pECCDeductLogBody->RFU2SubLocId[0] = 0x0;
        pECCDeductLogBody->RFU2SubLocId[1] = DCAReadData->LocationID;
        
        sysprintf("\r\n====  ECCDeductLogContainInit: (2) RFU2SubLocId = [0x%02x, 0x%02x], LocationID = 0x%02x, NewLocationID = [0x%02x, 0x%02x] ====\r\n", 
                    pECCDeductLogBody->RFU2SubLocId[0], pECCDeductLogBody->RFU2SubLocId[1], DCAReadData->LocationID, DCAReadData->NewLocationID[0], DCAReadData->NewLocationID[1]);
      
        //2018.08.14  --> K.【RFU2_VER】欄位版本：應填 01，卻填 00。
        pECCDeductLogBody->RFU2Ver = 0x01;                 //25 RFU2_VER 欄位版本(業者) Unsigned 1
                                                                //PVN <> 0 時:
                                                                    //定義參照 8.11
                                                                    //未定義: 固定填 0x00
                                                                //PVN = 0 時:
                                                                    //固定填 0x00
        
        
        pECCDeductLogBody->TM = EDCADeductData->TM;     //26 TM(TXN Mode) 交易模式 Byte 1
                                                            //PVN <> 0 時: 來源: 設備
                                                            //PVN = 0 時: 固定填 0x00
        pECCDeductLogBody->TQ = EDCADeductData->TQ;     //27 TQ(TXN Qualifier) 交易屬性 Byte 1
                                                            //PVN <> 0 時: 來源: 設備
                                                            //PVN = 0 時: 固定填 0x00
        pECCDeductLogBody->SignKeyVer = EDCADeductData->SignatureKeyKVN;             //28 SIGN_KEYVER SIGNATURE KEY 版本 Byte 1
                                                                                            //PVN <> 0 時:
                                                                                                //來源: Cpu 卡
                                                                                            //PVN = 0 時:
                                                                                                //固定填 0x00
        memcpy(pECCDeductLogBody->SignValue, EDCADeductData->SIGN, sizeof(pECCDeductLogBody->SignValue)); //29 SIGN_VALUE SIGNATURE 值 Byte 16
                                                                                                                //PVN <> 0 時:
                                                                                                                    //來源: Cpu 卡
                                                                                                                //PVN = 0 時:
                                                                                                                    //固定填 16 Bytes 0x00
    }
    else//if (PVN != 0x00) 
    {//0x00：Mifare／Level1；
        //2018.08.14  --> E. 【CAR_PLATE_NO】汽車車牌號碼：應填20202020202020202020，M卡與 L1卡卻填00000000000000000000。
        memset(pECCDeductLogBody->CarPlateNo, 0x20, sizeof(pECCDeductLogBody->CarPlateNo));     //20 CAR_PLATE_NO 車牌號碼 Ascii 10 來源: 設備
        
        //2018.08.14 -->  F. 【RFU1-XFER_PRE_SEQ_NO】轉乘上筆交易序號：應填轉乘上筆交易序號，卻填 0。
        pECCDeductLogBody->RFU1XferPreSeqNo[0] = DCAReadData->TSQNofURT[2];
        pECCDeductLogBody->RFU1XferPreSeqNo[1] = DCAReadData->TSQNofURT[1];
        pECCDeductLogBody->RFU1XferPreSeqNo[2] = DCAReadData->TSQNofURT[0];
        //2018.08.14 --> G.【RFU1-CARD_TYPE】票別：應填票卡卡種，卻填 0。
        pECCDeductLogBody->RFU1CardType = DCAReadData->CardType;
        //2018.08.14 --> H.【RFU1-XFER_PRE_TIMESTAMP】上筆交易時間：應填轉乘上筆交易時間，卻都填 1970/1/1 00:00:00。
        //memcpy(pECCDeductLogBody->RFU1XferPreTimeStamp, DCAReadData->TXNDateTimeofURT, sizeof(pECCDeductLogBody->RFU1XferPreTimeStamp));
        pECCDeductLogBody->RFU1XferPreTimeStamp[0] = DCAReadData->TXNDateTimeofURT[3];
        pECCDeductLogBody->RFU1XferPreTimeStamp[1] = DCAReadData->TXNDateTimeofURT[2];
        pECCDeductLogBody->RFU1XferPreTimeStamp[2] = DCAReadData->TXNDateTimeofURT[1];
        pECCDeductLogBody->RFU1XferPreTimeStamp[3] = DCAReadData->TXNDateTimeofURT[0];
        
        
        //2018.08.14  --> I.【RFU1_VER】欄位版本：應填 01，卻填 00。
        pECCDeductLogBody->RFU1Ver = 0x01;                //23 RFU1_VER 欄位版本(共用) Unsigned 1
                                                                //PVN <> 0 時:
                                                                    //固定填 0x00
                                                                //PVN = 0 時:
                                                                    //固定填 0x00
        
        //2018.08.14 --> J.【RFU2-SUB_LOC_ID】附屬場站代碼：應填 50，卻填 00
        //memcpy(pECCDeductLogBody->RFU2SubLocId, DCAReadData->NewLocationID, sizeof(pECCDeductLogBody->RFU2SubLocId));
        sysprintf("\r\n====  ECCDeductLogContainInit: (1) RFU2SubLocId = [0x%02x, 0x%02x], LocationID = 0x%02x, NewLocationID = [0x%02x, 0x%02x] ====\r\n", 
                    pECCDeductLogBody->RFU2SubLocId[0], pECCDeductLogBody->RFU2SubLocId[1], DCAReadData->LocationID, DCAReadData->NewLocationID[0], DCAReadData->NewLocationID[1]);
        //pECCDeductLogBody->RFU2SubLocId[0] = DCAReadData->NewLocationID[1];
        //pECCDeductLogBody->RFU2SubLocId[1] = DCAReadData->NewLocationID[0];
        pECCDeductLogBody->RFU2SubLocId[0] = 0x0;
        pECCDeductLogBody->RFU2SubLocId[1] = DCAReadData->LocationID;
        
        sysprintf("\r\n====  ECCDeductLogContainInit: (2) RFU2SubLocId = [0x%02x, 0x%02x], LocationID = 0x%02x, NewLocationID = [0x%02x, 0x%02x] ====\r\n", 
                    pECCDeductLogBody->RFU2SubLocId[0], pECCDeductLogBody->RFU2SubLocId[1], DCAReadData->LocationID, DCAReadData->NewLocationID[0], DCAReadData->NewLocationID[1]);
      
        //2018.08.14  --> K.【RFU2_VER】欄位版本：應填 01，卻填 00。
        pECCDeductLogBody->RFU2Ver = 0x01;                 //25 RFU2_VER 欄位版本(業者) Unsigned 1
                                                                //PVN <> 0 時:
                                                                    //定義參照 8.11
                                                                    //未定義: 固定填 0x00
                                                                //PVN = 0 時:
                                                                    //固定填 0x00
    }

    #if(1)
    prData = (uint8_t*)pECCDeductLogBody; 
    prData = prData + sizeof(ECCLogHeader);
    prDataLen = sizeof(ECCDeductLogBody) - sizeof(ECCLogHeader) - sizeof(ECCLogTail);
    sysprintf("--- pECCDeductLogBody:  len = %d --->\r\n", prDataLen);
    for(i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- pECCDeductLogBody ---\r\n");
    #endif
    // ********   Tail   ********
    #warning need check here
    if (PVN == 0x00) 
    {
         memcpy(&(pECCDeductLogBody->Tail.MACKeySet), EDCADeductData->MAC, 10); //MAC 10 bytes        
    }
    else
    {
        //pECCDeductLogBody->Tail.MACKeySet = 0x00;          //1 MACKeySet 0x00 Byte 1 固定填 0x00
        //pECCDeductLogBody->Tail.MAC3DESKey = 0x01;         //2 MAC3DESKey 0x01 Byte 1 固定填 0x01
        //uint8_t         MACValue[4];                //3 MACValue 交易驗証碼 Unsigned 4
                                                            //PVN <> 0 時: 固定填 4 Bytes 0x00
                                                            //PVN = 0 時: 來源:設備
        //uint8_t         MACMFRC[4];                 //4 MACMFRC 讀卡機編號 Unsigned 4
                                                            //PVN <> 0 時: 固定填 4 Bytes 0x00
                                                            //PVN = 0 時: 來源:設備
    }
    pECCDeductLogBody->Tail.MAC3DESKey = 0x01;         //2 MAC3DESKey 0x01 Byte 1 固定填 0x01

    if (PVN != 0x00) 
    {
        memcpy(pECCDeductLogBody->Tail.SAMID, EDCADeductData->SID, sizeof(pECCDeductLogBody->Tail.SAMID)); //5 SAM_ID SAM ID Pack 8
                                                        //PVN <> 0 時: 來源: Cpu Sam 卡
                                                        //PVN = 0 時: 來源:設備
    }
    if (PVN != 0x00) 
    {
        pECCDeductLogBody->Tail.HashType = EDCADeductData->HashType;  //6 HASH_TYPE HASH_TYPE Unsigned 1
                                                        //PVN <> 0 時: 代碼參照 8.4 來源: 設備
                                                        //PVN = 0 時: 固定填 0x00
    }
    else
    {
        pECCDeductLogBody->Tail.HashType = 0x00;
    }

    if (PVN != 0x00) 
    {
        pECCDeductLogBody->Tail.HostAdminKeyVer = EDCADeductData->HostAdminKeyKVN;            //7 HOST_ADMIN_KEYVER HOST ADMIN KEY 版本 Byte 1
                                                        //PVN <> 0 時: MAC 用哪個版本 KEY 去押的  來源: Cpu Sam 卡
                                                        //PVN = 0 時: 固定填 0x00
    }
    else
    {
        pECCDeductLogBody->Tail.HostAdminKeyVer = 0x00;
    }   
    if (PVN != 0x00) 
    {
        memcpy(pECCDeductLogBody->Tail.CpuMACValue, EDCADeductData->MAC, sizeof(pECCDeductLogBody->Tail.CpuMACValue)); //8 CPU_MAC_VALUE MAC 值 Byte 16                        
                                                        //PVN <> 0 時: 來源: Cpu Sam 卡計算
                                                        //PVN = 0 時: 固定填 16 Bytes 0x00
    }
    else
    {
        memset(pECCDeductLogBody->Tail.CpuMACValue, 0x00, sizeof(pECCDeductLogBody->Tail.CpuMACValue));
    }
    #if(1)
    prData = (uint8_t*)&(pECCDeductLogBody->Tail);
    prDataLen = sizeof(pECCDeductLogBody->Tail);
    sysprintf("--- pECCDeductLogBody->Tail:  len = %d --->\r\n", prDataLen);
    for(i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- pECCDeductLogBody->Tail ---\r\n");
    #endif
    
#if(ECC_LOG_USE_NEW_FILE_NAME)
    //咪表ID_訂單號_時間(yyyyMMddHHmmss)_001U.ECC
    sprintf(currentECCDeductLogFileName, "%s_%s_%s_001U.ECC", GetMeterData()->epmIdStr, GetMeterData()->bookingId, getECCLogUTCTimeStr(utcTime));
#else
    SP_ID_INT = DCAReadData->ServiceProviderID;
    SP_ID_INT_NEW = uint8ToUint32(DCAReadData->NewServiceProviderID, sizeof(DCAReadData->NewServiceProviderID));
    LOCATION_ID_INT_NEW = uint8ToUint32(DCAReadData->NewLocationID, sizeof(DCAReadData->NewLocationID));
    sprintf(currentECCDeductLogFileName, "00220.%03d_%08d_%05d_00000.%s_001U.ECC", SP_ID_INT, SP_ID_INT_NEW, LOCATION_ID_INT_NEW, getECCLogUTCTimeStr(utcTime));
#endif

    //sysprintf("\r\n====  ECCLogInit Body:[%s]\r\n", pECCDeductLogBody);    
    return TRUE;
    #endif
}
/*
uint8_t* ECCDeductLogGetContain(int* dataLen)
{
    * dataLen = sizeof(ECCDeductLogBody);
    return (uint8_t*)&eccLog;
}
*/
char* ECCDeductLogGetFileName(void)
{
    return currentECCDeductLogFileName;
}

BOOL ECCReSendLogContainInit(ECCReSendLogBody* body, uint32_t deductValue, uint32_t AgentNo, uint32_t utcTime, BOOL cardAutoloadAvailable, ECCCmdDCAReadResponseSuccessData* DCAReadData, int DCAReadDataLen, ECCCmdEDCADeductResponseData* EDCADeductData, int EDCADeductDataLen)
{
    #if(1)
    return TRUE;
    #else
    uint32_t i;
    uint8_t* prData; 
    uint32_t prDataLen;
    ECCReSendLogBody* pECCReSendLogBody = body;
    uint8_t PVN;// = DCAReadData->PurseVersionNumber;
    #if(!ECC_LOG_USE_NEW_FILE_NAME)
    uint32_t SP_ID_INT, SP_ID_INT_NEW, LOCATION_ID_INT_NEW;
    #endif

    LastCreditTXNLogData* pLastCreditTXNLogData = &(DCAReadData->LastCreditTXNLog);
    sysprintf("\r\n====  ECCReSendLogContainInit(%s) ====\r\n", GetMeterData()->epmIdStr);
    sysprintf("sizeof(ECCReSendLogBody) = %d\r\n", sizeof(ECCReSendLogBody));

    if(sizeof(ECCReSendLogBody) != TOTAL_ECC_RESEND_LOG_BODY_SIZE)
    {
        sysprintf("ECCReSendLogContainInit ERROR: sizeof(ECCReSendLogBody) != TOTAL_ECC_RESEND_LOG_BODY_SIZE [%d, %d]\r\n", sizeof(ECCReSendLogBody), TOTAL_ECC_RESEND_LOG_BODY_SIZE);
        return FALSE;
    }
    memset(pECCReSendLogBody, 0x00, sizeof(ECCReSendLogBody));
    PVN = pLastCreditTXNLogData->PurseVersionNumber;
    pECCReSendLogBody->PurseVerNo = pLastCreditTXNLogData->PurseVersionNumber;      //1 PURSE_VER_NO 卡片版本 Unsigned 1 代碼參照 8.1 來源: 設備
    pECCReSendLogBody->CardTxnTypeID = 0x0C;                    //(固定填 0x0C)     //2 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    pECCReSendLogBody->CardTxnSubTypeID = pLastCreditTXNLogData->SubType;           //3 CARD_TXN_SUBTYPE_ID 交易次類別(卡片) Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    if (PVN == 0x02)
    {
        memcpy(pECCReSendLogBody->CardDevID, pLastCreditTXNLogData->DeviceID, sizeof(pECCReSendLogBody->CardDevID));        //4 CARD_DEV_ID 設備編號(卡片) Unsigned 6   PVN=2，同 NEW_DEV_ID 如:
    }                                                                                           //NEW_SP_ID=49(0x31)
    else                                                                                            //NEW_DEV_TYPE=12(0x0C)
    {                                                                                            //NEW_DEV_ID_NO=99(0x63)
        pECCReSendLogBody->CardDevID[0] = pLastCreditTXNLogData->DeviceID[2];                        //組合 HEX: 00-00-31-0C-00-63
        pECCReSendLogBody->CardDevID[1] = pLastCreditTXNLogData->DeviceID[0];                        //以 LSB FIRST 傳送: 63-00-0C-31-00-00
        pECCReSendLogBody->CardDevID[2] = pLastCreditTXNLogData->DeviceID[1];                    //PVN<>2，同 DEV_ID，右補 3 Bytes 0x00
        pECCReSendLogBody->CardDevID[3] = pLastCreditTXNLogData->DeviceID[3];                        //同上例，組合 HEX: 31-C0-63
        pECCReSendLogBody->CardDevID[4] = pLastCreditTXNLogData->DeviceID[4];                        //倒數2 Bytes LSB FIRST: 31-63-C0
        pECCReSendLogBody->CardDevID[5] = pLastCreditTXNLogData->DeviceID[5];                        //右補 3 Bytes 0x00: 31-63-C0-00-00-00
    }                                                                                          //範例參照 8.6
                                                                                                //來源: 卡片
    memcpy(pECCReSendLogBody->CardSpID, pLastCreditTXNLogData->ServiceProviderID, sizeof(pECCReSendLogBody->CardSpID));  //5 CARD_SP_ID 業者代碼(卡片) Unsigned 3 PVN=2，同 NEW_SP_ID
                                                                                                                        //PVN<>2，同 SP_ID
                                                                                                                        //來源: 卡片
    memcpy(&(pECCReSendLogBody->TxnTimeStamp), &(pLastCreditTXNLogData->TXNDateTime), sizeof(pECCReSendLogBody->TxnTimeStamp));  //6 TXN_TIMESTAMP 交易時間(卡片) UnixTime 4 來源: 卡片
    memcpy(pECCReSendLogBody->CardPhysicalID, DCAReadData->CardPhysicalID, sizeof(pECCReSendLogBody->CardPhysicalID));  //7 CARD_PHYSICAL_ID 卡片晶片號碼 Unsigned 7 來源: 卡片
    pECCReSendLogBody->IssuerID = DCAReadData->IssuerCode;               //8 ISSUER_ID 發卡單位 Unsigned 1     PVN <> 0 時: 來源: Cpu 卡
                                                                                    //PVN = 0 時:
                                                                                        //2 (0x02)：悠遊卡公司
                                                                                        //66 (0x42)：基隆交通卡
                                                                                        //來源: 設備
    memcpy(pECCReSendLogBody->CardTxnSwqNo, pLastCreditTXNLogData->TSQN, sizeof(pECCReSendLogBody->CardTxnSwqNo));//9 CARD_TXN_SEQ_NO 交易後序號(卡片) Unsigned 3 來源: 卡片
    memcpy(pECCReSendLogBody->TxnAMT, pLastCreditTXNLogData->TXNAMT, sizeof(pECCReSendLogBody->TxnAMT));           //10 TXN_AMT 交易金額(卡片) Signed 3 來源: 卡片
    memcpy(pECCReSendLogBody->ElectronicValue, pLastCreditTXNLogData->EV, sizeof(pECCReSendLogBody->ElectronicValue));    //11 ELECTRONIC_VALUE 交易後卡片金額(卡片) Signed 3 來源: 卡片
    memcpy(pECCReSendLogBody->CardSvceLocID, pLastCreditTXNLogData->LocationID, sizeof(pECCReSendLogBody->CardSvceLocID));       //12 CARD_SVCE_LOC_ID 場站代碼(卡片) Unsigned 2
                                                                                                                                        //PVN=2，NEW_SVCE_LOC_ID
                                                                                                                                        //PVN<>2，SVCE_LOC_ID
                                                                                                                                        //來源: 卡片
    pECCReSendLogBody->CardPhysicalIDLen = DCAReadData->CardPhysicalIDLength;      //13 CARD_PHYSICAL_ID_LEN 晶片號碼長度 Unsigned 1
                                                                                                    //如 4 bytes 即為 0x04
                                                                                                    //如 7 bytes 即為 0x07
                                                                                                    //來源: 卡片
    pECCReSendLogBody->NewPersonalProfile = DCAReadData->PersonalProfile;     //14 NEW_PERSONAL_PROFILE 新身份別 Unsigned 1
                                                                                                    //0x00: 一般
                                                                                                    //0x01: 敬老 1
                                                                                                    //0x02: 敬老 2
                                                                                                    //0x03: 愛心
                                                                                                    //0x04: 陪伴
                                                                                                    //0x05: 學生
                                                                                                    //0x08: 優待
                                                                                                    //來源: 卡片
    memcpy(pECCReSendLogBody->CardPurseID, &(DCAReadData->PID), sizeof(pECCReSendLogBody->CardPurseID)); //15 CARD_PURSE_ID 錢包編號(外觀號碼) Pack 8
                                                                                    //PVN <> 0 時:
                                                                                        //如卡號 1234567890123456
                                                                                        //即為 0x12 0x34 0x56 0x78 0x90 0x12 0x34 0x56
                                                                                        //代碼參照 8.5
                                                                                        //來源: Cpu 卡
                                                                                    //PVN = 0 時:
                                                                                        //固定填 8 Bytes 0x00
    pECCReSendLogBody->BankCode = DCAReadData->BankCode; //16 BANK_CODE 銀行代碼 Unsigned 1    PVN <> 0 時:
                                                            //固定填 0x00
                                                            //PVN = 0 時:
                                                            //來源: 卡片
    memcpy(pECCReSendLogBody->LoyaltyCounter, DCAReadData->LoyaltyCounter, sizeof(pECCReSendLogBody->LoyaltyCounter));      //17 LOYALTY_COUNTER 累積忠誠點 Unsigned 2 來源: 設備
    pECCReSendLogBody->ResendDevID[0] = DCAReadData->DeviceID[2];         //18 RESEND_DEV_ID 重送設備編號 Unsigned 3 
    pECCReSendLogBody->ResendDevID[1] = DCAReadData->DeviceID[0];           //SP_ID(1 Byte): 如 122 即為 0x7A
    pECCReSendLogBody->ResendDevID[2] = DCAReadData->DeviceID[1];           //DEV_TYPE(4 Bits):  3：BV 如 BV 即為 0x30
                                                                            //DEV_ID_NO(12 Bites): 如 1234 即為 0x04 0xD2
                                                                            //組合 HEX: 0x7A 0x34 0xD2
                                                                            //倒數2 Bytes LSB FIRST:  0x7A 0xD2 0x34
                                                                            //範例參照 8.6
                                                                            //來源: Mifare Sam 卡
    pECCReSendLogBody->ResendSpID = DCAReadData->ServiceProviderID;             //19 RESEND_SP_ID 重送業者代碼 Unsigned 1  如 122 即為 0x7A  來源: 設備
    pECCReSendLogBody->ResendLocID = DCAReadData->LocationID;            //20 RESEND_LOC_ID 重送場站代碼 Unsigned 1 來源: 設備
    memcpy(pECCReSendLogBody->ResendNewDevID, DCAReadData->NewDeviceID, sizeof(pECCReSendLogBody->ResendNewDevID));         //21 RESEND_NEW_DEV_ID 重送新設備編號 Unsigned 6 
                                                            //NEW_SP_ID(3 Bytes): 122 即為 0x00 0x00 0x7A
                                                            //NEW_DEV_TYPE(1 Byte): 3：BV  如 3 即為 0x03
                                                                //4096(含)之後開始編碼
                                                            //NEW_DEV_ID_NO (2 Bytes): 如 4096 即為 0x10 0x00
                                                            //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
                                                            //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                            //範例參照 8.6
                                                            //來源: Cpu Sam 卡
    memcpy(pECCReSendLogBody->ResendNewSpID, DCAReadData->NewServiceProviderID, sizeof(pECCReSendLogBody->ResendNewSpID));  //22 RESEND_NEW_SP_ID 重送新業者代碼 Unsigned 3
                                                            //NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
                                                            //以 LSB FIRST 傳送: 0x7A 0x00 0x00
                                                            //來源: 設備
    memcpy(pECCReSendLogBody->ResendNewLocID, DCAReadData->NewLocationID, sizeof(pECCReSendLogBody->ResendNewLocID));       //23 RESEND_NEW_LOC_ID 重送新場站代碼 Unsigned 2
                                                            //NEW_SVCE_LOC_ID (2 Bytes): 如 1 即為 0x00 0x01
                                                            //以 LSB FIRST 傳送: 0x01 0x00
                                                            //來源: 設備
    
   
    
#if(ECC_LOG_USE_NEW_FILE_NAME)
    //咪表ID_訂單號_時間(yyyyMMddHHmmss)_001U.ECC
    sprintf(currentECCReSendLogFileName, "%s_%s_%s_012U.ECC", GetMeterData()->epmIdStr, GetMeterData()->bookingId, getECCLogUTCTimeStr(utcTime));
#else
    SP_ID_INT = DCAReadData->ServiceProviderID;
    SP_ID_INT_NEW = uint8ToUint32(DCAReadData->NewServiceProviderID, sizeof(DCAReadData->NewServiceProviderID));
    LOCATION_ID_INT_NEW = uint8ToUint32(DCAReadData->NewLocationID, sizeof(DCAReadData->NewLocationID));
    sprintf(currentECCReSendLogFileName, "00220.%03d_%08d_%05d_00000.%s_012U.ECC", SP_ID_INT, SP_ID_INT_NEW, LOCATION_ID_INT_NEW, getECCLogUTCTimeStr(utcTime));
#endif    
    #if(1)
    prData = (uint8_t*)&(pECCReSendLogBody);
    prDataLen = sizeof(ECCReSendLogBody);
    sysprintf("--- pECCReSendLogBody:  len = %d --->\r\n", prDataLen);
    for(i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- pECCReSendLogBody ---\r\n");
    #endif
    return TRUE;
    #endif
}
char* ECCReSendLogGetFileName(void)
{
    return currentECCReSendLogFileName;
}
BOOL ECCLockCardLogContainInit(ECCLockCardLogBody* body, uint32_t lockType, uint8_t newSpID, uint8_t svceLocID, char* blkFileName, 
                                    uint32_t utcTime, uint8_t* DCAReadData, ECCCmdLockCardResponseData* LockCardResponseData)
{
        #if(1)
    return TRUE;
    #else
    uint32_t i;
    uint8_t* prData; 
    uint32_t prDataLen;
    int targetblkFileNameLen;
    ECCCmdLockCardResponseData* TargetLockCardResponseData;
    
    char* blkFileNameExtension = getExtension(blkFileName);
    uint8_t PVN;
    LastCreditTXNLogData AddValueBytes;

    // --> 只有以下狀況 <--
    //0x9000                                >>> "鎖卡成功" 
    //==> USE readSuccessData:              [the sam as DCAReadData]
    //==> USE TargetLockCardResponseData:   [the sam as LockCardResponseData]
    ECCCmdDCAReadResponseSuccessData*    readSuccessData = (ECCCmdDCAReadResponseSuccessData*)DCAReadData;

    //0x640E(餘額異常) or 0x6418(通路限制)  >>> "讀卡失敗"      ... 0x6418(通路限制) 目前沒用到
    //==> USE readError1Data:                   [the sam as DCAReadData]
    //=??=> USE TargetLockCardResponseData:     [the sam as LockCardResponseData]
    ECCCmdDCAReadResponseError1Data*    readError1Data = (ECCCmdDCAReadResponseError1Data*)DCAReadData; 

    //0x6103(CPD檢查異常)                   >>> "讀卡失敗"  
    //==> USE TargetLockCardResponseData:   [cthe sam as DCAReadData]  --> 跟 LockCard response (0x9000) structure 相同 


    ECCLockCardLogBody* pECCLockCardLogBody = body;
    sysprintf("\r\n====  ECCLockCardLogContainInit (blkFileName = [%s](%d), Extension = [%s])====\r\n", 
                                                                            blkFileName, strlen(blkFileName), blkFileNameExtension);
    sysprintf("sizeof(ECCLockCardLogBody) = %d\r\n", sizeof(ECCLockCardLogBody));

    if(sizeof(ECCLockCardLogBody) != TOTAL_ECC_LOCK_CARD_LOG_BODY_SIZE)
    {
        sysprintf("ECCLockCardLogContainInit ERROR: sizeof(ECCLockCardLogBody) != TOTAL_ECC_BLK_FEEDBACK_LOG_BODY_SIZE [%d, %d]\r\n", sizeof(ECCLockCardLogBody), TOTAL_ECC_BLK_FEEDBACK_LOG_BODY_SIZE);
        return FALSE;
    }
    memset(pECCLockCardLogBody, 0x00, sizeof(ECCLockCardLogBody));
    
    //*** body ***//
    if(lockType == ECC_CMD_READ_ERROR_2_ID_1)// 0x6103 (CPD檢查異常) --> (//跟 LockCard response (0x9000) structure 相同)
    {
        //memcpy(DCAReadData, &TargetLockCardResponseData, sizeof(ECCCmdLockCardResponseData));
        TargetLockCardResponseData = (ECCCmdLockCardResponseData*)DCAReadData;
    }
    else
    {
        //memcpy(LockCardResponseData, &TargetLockCardResponseData, sizeof(ECCCmdLockCardResponseData));
        TargetLockCardResponseData = (ECCCmdLockCardResponseData*)LockCardResponseData;
    }

    PVN = readSuccessData->PurseVersionNumber;

    //2018.08.14  --> //A.【PURSE_VER_NO】卡片版本：CPU_L1卡應填 01，卻填 00。
    //1 PURSE_VER_NO 卡片版本 Unsigned 1 代碼參照 8.1 來源: 設備
    //票卡版號    0x00：Mifare／Level1；
    //            0x02：Level2
    //EDCA READ 無法分辨
    //pECCLockCardLogBody->PurseVerNo = readSuccessData->PurseVersionNumber; 
    pECCLockCardLogBody->PurseVerNo = LockCardResponseData->PurseVersionNumber;     

    //2 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    pECCLockCardLogBody->CardTxnTypeID = 0x22;  //(固定填0x22)          
    
    //3 CARD_TXN_SUBTYPE_ID 交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照8.3
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)      
    {
        pECCLockCardLogBody->CardTxnSubTypeID = readError1Data->PersonalProfile;       
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        pECCLockCardLogBody->CardTxnSubTypeID = TargetLockCardResponseData->PersonalProfile; 
    }
    
    //4 DEV_ID 設備編號 Unsigned 3
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)  
    {                                                                   
        pECCLockCardLogBody->DevID[0] = readError1Data->DeviceID[2];    
        pECCLockCardLogBody->DevID[1] = readError1Data->DeviceID[0];     
        pECCLockCardLogBody->DevID[2] = readError1Data->DeviceID[1];    
    }                                                                   
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗"   
    {                                                                                                                                     
        pECCLockCardLogBody->DevID[0] = TargetLockCardResponseData->DeviceID[2];;
        pECCLockCardLogBody->DevID[1] = TargetLockCardResponseData->DeviceID[0];;
        pECCLockCardLogBody->DevID[2] = TargetLockCardResponseData->DeviceID[1];;
    }   
    
    //5 SP_ID 業者代碼 Unsigned 1 如 122 即為 0x7A 來源: 設備
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)   
    {
        pECCLockCardLogBody->SpID = readError1Data->ServiceProviderID;
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        pECCLockCardLogBody->SpID = TargetLockCardResponseData->ServiceProviderID;
    }

    //6 TXN_TIMESTAMP 交易時間 UnixTime 4 來源: 設備
    getECCLogTimeData(utcTime, &(pECCLockCardLogBody->TxnTimeStamp));  

    //7 CARD_PHYSICAL_ID 卡片晶片號碼 Unsigned 7 來源: 設備
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)       
    {
        memcpy(pECCLockCardLogBody->CardPhysicalID, readError1Data->CardPhysicalID, sizeof(pECCLockCardLogBody->CardPhysicalID)); 
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        memcpy(pECCLockCardLogBody->CardPhysicalID, TargetLockCardResponseData->CardPhysicalID, sizeof(pECCLockCardLogBody->CardPhysicalID));
    }   
    
    //8 ISSUER_ID 發卡單位 Unsigned 1 PVN <> 0 時:
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)   
    {                                                                                                
        pECCLockCardLogBody->IIssuerID = readError1Data->IssuerCode;                                 
    }                                                                                                
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) >>> "讀卡失敗"                                   
    {                                                                                                
        pECCLockCardLogBody->IIssuerID = TargetLockCardResponseData->IssuerCode;
    }

    //9 SVCE_LOC_ID 場站代碼 Unsigned 1 來源: 設備
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)                   
    {
        pECCLockCardLogBody->SvceLocID = readError1Data->LocationID;  
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        pECCLockCardLogBody->SvceLocID = TargetLockCardResponseData->LocationID;
    }

    //10 CARD_PHYSICAL_ID_LEN 晶片號碼長度 Unsigned 1
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)     
    {                                                                              
        pECCLockCardLogBody->CardPhysicalIDLen = readError1Data->CardPhysicalIDLength;           
    }                                                                           
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) >>> "讀卡失敗"              
    {                                                                           
        pECCLockCardLogBody->CardPhysicalIDLen = TargetLockCardResponseData->CardPhysicalIDLength;
    }                                                                           

    //11 NEW_DEV_ID 新設備編號 Unsigned 6 
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)       
    {
        memcpy(pECCLockCardLogBody->NewDevID, readError1Data->NewDeviceID, sizeof(pECCLockCardLogBody->NewDevID)); 
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        memcpy(pECCLockCardLogBody->NewDevID, TargetLockCardResponseData->NewDeviceID, sizeof(pECCLockCardLogBody->NewDevID));
    }  
                                                               
    //12 NEW_SP_ID 新業者代碼 Unsigned 3 
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)       
    {
        memcpy(pECCLockCardLogBody->NewSpID, readError1Data->NewServiceProviderID, sizeof(pECCLockCardLogBody->NewSpID)); 
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        memcpy(pECCLockCardLogBody->NewSpID, TargetLockCardResponseData->NewServiceProviderID, sizeof(pECCLockCardLogBody->NewSpID));
    }  
                                                               
    //13 NEW_SVCE_LOC_ID 新場站代碼 Unsigned 2 
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)       
    {
        memcpy(pECCLockCardLogBody->NewSvceLocID, readError1Data->NewLocationID, sizeof(pECCLockCardLogBody->NewSvceLocID)); 
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        memcpy(pECCLockCardLogBody->NewSvceLocID, TargetLockCardResponseData->NewLocationID, sizeof(pECCLockCardLogBody->NewSvceLocID));
    }  
                                                                   
    //14 NEW_CARD_TXN_SUBTYPE_ID 新交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    pECCLockCardLogBody->NewCardTxnSubTypeID = 0x00;  

    //15 NEW_PERSONAL_PROFILE 新身份別 Unsigned 1
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)     
    {                                                                              
        pECCLockCardLogBody->NewPersonalProfile = readError1Data->PersonalProfile;           
    }                                                                           
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗"               
    {                                                                           
        pECCLockCardLogBody->NewPersonalProfile = TargetLockCardResponseData->PersonalProfile;
    }   
                                                                   
    //16 CARD_PURSE_ID 錢包編號(外觀號碼) Pack 8
    if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E (餘額異常)       
    {
        memcpy(pECCLockCardLogBody->CardPurseID, &(readError1Data->PID), sizeof(pECCLockCardLogBody->CardPurseID)); 
    }
    else //0x9000 >>> "鎖卡成功" or 0x6103(CPD檢查異常) 0x6418(通路限制) >>> "讀卡失敗" 
    {
        memcpy(pECCLockCardLogBody->CardPurseID, &(TargetLockCardResponseData->PID), sizeof(pECCLockCardLogBody->CardPurseID));
    }  
                                                     
    #warning  0x640E (餘額異常) 沒有  pECCLockCardLogBody                                                              
    //17 CARD_CTC_SEQ_NO 卡片處理序號 Unsigned Msb 3
    if (lockType == ECC_CMD_READ_ERROR_2_ID_1)//0x6103(CPD檢查異常)       
    {
        memcpy(pECCLockCardLogBody->CardCTCSeqNo, TargetLockCardResponseData->CTC, sizeof(pECCLockCardLogBody->CardCTCSeqNo)); 
    }
    else //0x9000 >>> "鎖卡成功" or  ????0x640E (餘額異常)???  0x6418(通路限制) >>> "讀卡失敗"
    {
        memset(pECCLockCardLogBody->CardCTCSeqNo, 0x00, sizeof(pECCLockCardLogBody->CardCTCSeqNo));
    }  
                                                                    
    //18 CARD_BLOCKING_REASON 卡片鎖卡原因 Unsigned 1 代碼參照 8.9 來源: 設備
    if (lockType == ECC_CMD_READ_SUCCESS_ID)//0x9000 鎖卡成功
    {
        pECCLockCardLogBody->CardBlockingReason = 0x01;
    }
    else if (lockType == ECC_CMD_READ_ERROR_2_ID_1)//0x6103(CPD檢查異常)
    {
        pECCLockCardLogBody->CardBlockingReason = 0x02;
    }
    else if (lockType == ECC_CMD_READ_ERROR_1_ID_1)//0x640E(餘額異常)
    {
        pECCLockCardLogBody->CardBlockingReason = 0x0E;
    }
    else if (lockType == 0x610F)
    {
        pECCLockCardLogBody->CardBlockingReason = 0x0F;
    }
    else if (lockType == ECC_CMD_READ_ERROR_1_ID_2)//0x6418(通路限制)
    {
        pECCLockCardLogBody->CardBlockingReason = 0x18;
    }

    //2018.08.14  --> B.【BLOCKING_FILE】鎖卡名單檔名：離線黑名單鎖卡應填 黑名單檔名，卻空白。
    //19 BLOCKING_FILE 鎖卡名單檔名 Ascii 20 
    //#if(1)//CardBlockingReason = 0x01 改成跟 CardBlockingReason = 0x02 相同 (ie 後面都是空白)
    if (lockType == ECC_CMD_READ_SUCCESS_ID)//0x9000 鎖卡成功
    {
        memset(pECCLockCardLogBody->BlockingFile, ' ', sizeof(pECCLockCardLogBody->BlockingFile));
        if(strlen(blkFileName) > sizeof(pECCLockCardLogBody->BlockingFile))
        {
            targetblkFileNameLen = sizeof(pECCLockCardLogBody->BlockingFile);
        }
        else
        {
            targetblkFileNameLen = strlen(blkFileName);
        }
        memcpy(pECCLockCardLogBody->BlockingFile, blkFileName, targetblkFileNameLen);
    }
    else 
    //#endif
    {
        memset(pECCLockCardLogBody->BlockingFile, 0x20, sizeof(pECCLockCardLogBody->BlockingFile));
    }  
    //2018.08.14  --> C.【BLOCKING_ID_FLAG】鎖卡卡號ID旗標：離線黑名單鎖卡應填 M，卻空白。                                                                    
    //20 BLOCKING_ID_FLAG 鎖卡卡號 ID 旗標 Ascii 1
    //#if(1)//CardBlockingReason = 0x01 改成跟 CardBlockingReason = 0x02 相同 (ie 後面都是空白)
    if (lockType == ECC_CMD_READ_SUCCESS_ID)//0x9000 鎖卡成功
    {
        pECCLockCardLogBody->BlockingIDFlag = 0x00;
        if ( (strcmp(blkFileNameExtension, "BIG") == 0) || (strcmp(blkFileNameExtension, "SML") == 0) )
        {
            if (PVN == 0x00)
            {
                pECCLockCardLogBody->BlockingIDFlag = 0x4D;
            }
            else
            {
                pECCLockCardLogBody->BlockingIDFlag = 0x50;
            }
        }
        else
        {
            pECCLockCardLogBody->BlockingIDFlag = 0x4D;
        }
    }
    else
    //#endif
    {
        pECCLockCardLogBody->BlockingIDFlag = 0x20;
    }
                                                                   
    //21 DEV_INFO 設備資訊 Ascii 20 BV、Tobu ：車號
    memset(pECCLockCardLogBody->DevInfo, 0x20, sizeof(pECCLockCardLogBody->DevInfo)); 
                                                                
    //22 ELECTRONIC_VALUE 交易後卡片金額 Signed 3 來源: 卡片 --> (Purse Balance - 640E 與 6418 才有的)
    if ( (lockType == ECC_CMD_READ_ERROR_1_ID_1 /*0x640E(餘額異常)*/) || 
            (lockType == ECC_CMD_READ_ERROR_1_ID_2 /*0x6418(通路限制)*/) )
    {
        memcpy(pECCLockCardLogBody->ElectronicValue, readError1Data->PurseBalance, sizeof(pECCLockCardLogBody->ElectronicValue)); 
    }

    //23 NEW_CARD_TXN_SEQ_NO 新交易後序號 Unsigned 3 來源: 卡片 --> (TXN SN - 640E 與 6418 才有的)
    if ( (lockType == ECC_CMD_READ_ERROR_1_ID_1 /*0x640E(餘額異常)*/) || 
                (lockType == ECC_CMD_READ_ERROR_1_ID_2 /*0x6418(通路限制)*/) )
    {
        memcpy(pECCLockCardLogBody->NewCardTxnSeqNo, readError1Data->TTXNSN, sizeof(pECCLockCardLogBody->NewCardTxnSeqNo)); 
    }
    //加值紀錄 (PPR_EDCARead 9000 與 640E 與 6418 才有的)
    // === 2017/06/01 START ===
    #if(0)//CardBlockingReason = 0x01 改成跟 CardBlockingReason = 0x02 相同 (ie 後面都是空白)
    if (lockType == ECC_CMD_READ_SUCCESS_ID)//0x9000 鎖卡成功
    {
        memcpy(&AddValueBytes, &(readSuccessData->LastCreditTXNLog), sizeof(AddValueBytes));
    }
    else 
    #endif
    if ( (lockType == ECC_CMD_READ_ERROR_1_ID_1 /*0x640E(餘額異常)*/) || 
                        (lockType == ECC_CMD_READ_ERROR_1_ID_2 /*0x6418(通路限制)*/) )
    {
        //Array.Copy(EDCAReadBytes, 65, AddValueBytes, 0, 33);
        memcpy(&AddValueBytes, &(readError1Data->LastCreditTXNLog), sizeof(AddValueBytes));
    }
    // === 2017/06/01 E N D ===
    #if(0)//CardBlockingReason = 0x01 改成跟 CardBlockingReason = 0x02 相同 (ie 後面都是空白)
    if ( (lockType == ECC_CMD_READ_SUCCESS_ID /*0x9000 鎖卡成功*/) ||
                        (lockType == ECC_CMD_READ_ERROR_1_ID_1 /*0x640E(餘額異常)*/) || 
                        (lockType == ECC_CMD_READ_ERROR_1_ID_2 /*0x6418(通路限制)*/) )
    #else
    if ( /*(lockType == ECC_CMD_READ_SUCCESS_ID 0x9000 鎖卡成功) ||*/
                        (lockType == ECC_CMD_READ_ERROR_1_ID_1 /*0x640E(餘額異常)*/) || 
                        (lockType == ECC_CMD_READ_ERROR_1_ID_2 /*0x6418(通路限制)*/) )
    #endif
    {
        uint8_t ADD_PVN = AddValueBytes.PurseVersionNumber;

        //24 CRELOG_TXN_SEQ_NO 票卡最末筆加值記錄  1.交易後序號 Unsigned 3 來源: 卡片
        memcpy(pECCLockCardLogBody->CreLogTxnSeqNo, AddValueBytes.TSQN, sizeof(pECCLockCardLogBody->CreLogTxnSeqNo)); 
     

        //25 CRELOG_TXN_TIMESTAMP 票卡最末筆加值記錄  2.交易時間 UnixTime 4 來源: 卡片
        memcpy(&(pECCLockCardLogBody->CreLogTxnTimeStamp), &(AddValueBytes.TXNDateTime), sizeof(pECCLockCardLogBody->CreLogTxnTimeStamp)); 

        //26 CRELOG_TXN_SUBTYPE_ID 票卡最末筆加值記錄  3.交易次類別 Unsigned 1 來源: 卡片
        pECCLockCardLogBody->CreLogTxnSubTypeID = AddValueBytes.SubType; 

        //27 CRELOG_TXN_AMT 票卡最末筆加 Signed 3 來源: 卡片值記錄  4.交易金額
        memcpy(pECCLockCardLogBody->CreLogTxnAMT, AddValueBytes.TXNAMT, sizeof(pECCLockCardLogBody->CreLogTxnAMT)); 

        //28 CRELOG_EV 票卡最末筆加值記錄  5.交易後卡片金額   Signed 3 來源: 卡片
        memcpy(pECCLockCardLogBody->CreLogEV, AddValueBytes.EV, sizeof(pECCLockCardLogBody->CreLogEV)); 

        //29 CRELOG_DEV_ID 票卡最末筆加值記錄  6.設備編號 Unsigned 6 
        if(ADD_PVN == 0x02)
        {
            memcpy(pECCLockCardLogBody->CreLogDevID, AddValueBytes.DeviceID, sizeof(pECCLockCardLogBody->CreLogDevID)); 
        }
        else
        {
            pECCLockCardLogBody->CreLogDevID[0] = AddValueBytes.DeviceID[2];
            pECCLockCardLogBody->CreLogDevID[1] = AddValueBytes.DeviceID[0];
            pECCLockCardLogBody->CreLogDevID[2] = AddValueBytes.DeviceID[1];
            pECCLockCardLogBody->CreLogDevID[3] = AddValueBytes.DeviceID[3];
            pECCLockCardLogBody->CreLogDevID[4] = AddValueBytes.DeviceID[4];
            pECCLockCardLogBody->CreLogDevID[5] = AddValueBytes.DeviceID[5];
        }
    }

    if ( (lockType == ECC_CMD_READ_ERROR_1_ID_1 /*0x640E(餘額異常)*/) || 
                        (lockType == ECC_CMD_READ_ERROR_1_ID_2 /*0x6418(通路限制)*/) )
    { 
        //30 ANOTHER_EV 票卡另一個錢包餘額 Signed 3 來源: 卡片
        memcpy(pECCLockCardLogBody->AnotherEV, readError1Data->AnotherEV, sizeof(pECCLockCardLogBody->AnotherEV));                                                         

        //31 MIFARE_SET_PARA 票卡地區認證 旗標 Byte 1 後台 UNPACK 文字格式 CHAR(2) 來源: 卡片
        pECCLockCardLogBody->MifareSetPara =  readError1Data->MifareSettingParameter;

        //32 CPU_SET_PARA 票卡是否限制 通路使用旗標 Byte 1 後台 UNPACK 文字格式 CHAR(2) 來源: 卡片
        pECCLockCardLogBody->CpuSetPara =  readError1Data->CPUSettingParameter;
    }

    //33 RFU 保留欄位 Byte 16 後台 UNPACK 文字格式 CHAR(32) 固定填 16 Bytes 0x00
    memset(pECCLockCardLogBody->RFU, 0x00, sizeof(pECCLockCardLogBody->RFU));     //RFU(Reserved For Use) 11 TM 保留，補 0x00，11bytes
    
    ////////////////
#if(ECC_LOG_USE_NEW_FILE_NAME)
    //咪表ID_訂單號_時間(yyyyMMddHHmmss)_001U.ECC
    sprintf(currentECCLockCardLogFileName, "%s_%s_%s_034U.ECC", GetMeterData()->epmIdStr, GetMeterData()->bookingId, getECCLogUTCTimeStr(utcTime));
#else    
    sprintf(currentECCLockCardLogFileName, "00220.%03d_%08d_%05d_00000.%s_034U.ECC", newSpID, newSpID, svceLocID, getECCLogUTCTimeStr(utcTime));
#endif
    
    #if(1)
    prData = (uint8_t*)&(pECCLockCardLogBody);
    prDataLen = sizeof(ECCLockCardLogBody);
    sysprintf("--- pECCLockCardLogBody:  len = %d --->\r\n", prDataLen);
    for(i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- pECCLockCardLogBody ---\r\n");
    #endif
    return TRUE;
    #endif
}
char* ECCLockCardLogGetFileName(void)
{
    return currentECCLockCardLogFileName;
}
BOOL ECCBlkFeedbackLogContainInit(ECCBlkFeedbackLogBody* body, uint8_t newSpID, uint8_t svceLocID, char* blkFileName, uint32_t utcTime)
{
    #if(1)
    return TRUE;
    #else
    uint32_t i;
    uint8_t* prData; 
    uint32_t prDataLen;
    int targetblkFileNameLen;
    ECCBlkFeedbackLogBody* pECCBlkFeedbackLogBody = body;
    sysprintf("\r\n====  ECCBlkFeedbackLogContainInit (blkFileName = [%s](%d))====\r\n", blkFileName, strlen(blkFileName));
    sysprintf("sizeof(ECCBlkFeedbackLogBody) = %d\r\n", sizeof(ECCBlkFeedbackLogBody));

    if(sizeof(ECCBlkFeedbackLogBody) != TOTAL_ECC_BLK_FEEDBACK_LOG_BODY_SIZE)
    {
        sysprintf("ECCDeductLogContainInit ERROR: sizeof(ECCBlkFeedbackLogBody) != TOTAL_ECC_BLK_FEEDBACK_LOG_BODY_SIZE [%d, %d]\r\n", sizeof(ECCBlkFeedbackLogBody), TOTAL_ECC_BLK_FEEDBACK_LOG_BODY_SIZE);
        return FALSE;
    }
    memset(pECCBlkFeedbackLogBody, 0x00, sizeof(ECCBlkFeedbackLogBody));
    //*** header tail ***//
    uint8_t fileHead_01[] = { 0x48, 0x45, 0x41, 0x44, 0x30, 0x30, 0x36, 0x31, 0x44, 0x41, 0x54, 0x41 };
    uint8_t fileTail_01[] = { 0x54, 0x41, 0x49, 0x4C,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31,
                              0x45, 0x4E, 0x44 };
    memcpy(pECCBlkFeedbackLogBody->header, fileHead_01, sizeof(pECCBlkFeedbackLogBody->header));
    memcpy(pECCBlkFeedbackLogBody->tail, fileTail_01, sizeof(pECCBlkFeedbackLogBody->tail));
    //*** body ***//
    pECCBlkFeedbackLogBody->CardTxnTypeId = 0x34;          //1 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    pECCBlkFeedbackLogBody->DevID[0] = newSpID; //2 DEV_ID 設備編號 Unsigned 3 SP_ID(1 Byte): 如 122 即為 0x7A
    pECCBlkFeedbackLogBody->DevID[1] = 0x6B;                                //DEV_TYPE(4 Bits): 如 3 即為 0x3□
    pECCBlkFeedbackLogBody->DevID[2] = 0x30;                                //DEV_ID_NO(12 Bites): 如 100 即為 0x□0 0x64
                                                                                //組合 HEX: 0x7A 0x30 0x64
                                                                                //倒數2 Bytes LSB FIRST: 0x7A 0x64 0x30
                                                                                //來源: Mifare Sam 卡
    pECCBlkFeedbackLogBody->SpID = newSpID;               //3 SP_ID 業者代碼 Unsigned 1 SP_ID(1 Byte): 如 122 即為 0x7A 來源: 設備
    pECCBlkFeedbackLogBody->SvceLocID = svceLocID;              //4 SVCE_LOC_ID 場站代碼 Unsigned 1 來源: 設備
    pECCBlkFeedbackLogBody->NewDevID[0] = pECCBlkFeedbackLogBody->DevID[0];            //5 NEW_DEV_ID 新設備編號 新增 Unsigned 6 NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
    pECCBlkFeedbackLogBody->NewDevID[1] = 0x6B;                                                //NEW_DEV_TYPE(1 Byte): 3：BV 如 3 即為 0x03
    pECCBlkFeedbackLogBody->NewDevID[2] = 0x30;                                                //4096(含)之後開始編碼
    pECCBlkFeedbackLogBody->NewDevID[3] = 0x00;                                                //NEW_DEV_ID_NO(2 Bytes): 如 4096 即為 0x10 0x00
    pECCBlkFeedbackLogBody->NewDevID[4] = 0x00;                                                //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
    pECCBlkFeedbackLogBody->NewDevID[5] = 0x00;                                                //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                                                        //來源: Cpu Sam 卡
                                                                                        //二代卡設備未修改，以 SIS2 傳送時: 固定填 6 Bytes 0x00
    pECCBlkFeedbackLogBody->NewSpID[0] = pECCBlkFeedbackLogBody->DevID[0];   //6 NEW_SP_ID 新業者代碼 新增 Unsigned 3 NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
    pECCBlkFeedbackLogBody->NewSpID[1] = 0x00;                                       //以 LSB FIRST 傳送: 0x7A 0x00 0x00
    pECCBlkFeedbackLogBody->NewSpID[2] = 0x00;                                       //來源: 設備
    pECCBlkFeedbackLogBody->NewSvceLocID[0] = pECCBlkFeedbackLogBody->SvceLocID;        //7 NEW_SVCE_LOC_ID 新場站代碼 新增 Unsigned 2 NEW_SVCE_LOC_ID(2 Bytes): 如 1 即為 0x00 0x01
    pECCBlkFeedbackLogBody->NewSvceLocID[1] = 0x00;                                                                    //以 LSB FIRST 傳送: 0x01 0x00
                                                                                        //來源: 設備
    memset(pECCBlkFeedbackLogBody->BlockingFile, ' ', sizeof(pECCBlkFeedbackLogBody->BlockingFile)); //8 BLOCKING_FILE 鎖卡名單檔名 新增 Ascii 20 左靠右補空白，如'BLC00001.BIGΔΔΔΔΔΔΔΔ'   來源: 設備
    if(strlen(blkFileName) > sizeof(pECCBlkFeedbackLogBody->BlockingFile))
    {
        targetblkFileNameLen = sizeof(pECCBlkFeedbackLogBody->BlockingFile);
    }
    else
    {
        targetblkFileNameLen = strlen(blkFileName);
    }
    memcpy(pECCBlkFeedbackLogBody->BlockingFile, blkFileName, targetblkFileNameLen);

    getECCLogTimeData(utcTime, &(pECCBlkFeedbackLogBody->ReceiveDateTime));     //9 RECEIVE_DATETIME  收到鎖卡檔時間 新增 UnixTime 4 來源: 設備

    memset(pECCBlkFeedbackLogBody->DevInfo, ' ', sizeof(pECCBlkFeedbackLogBody->DevInfo));   //10 DEV_INFO 設備資訊 新增 Ascii 20  BV、Tobu ：車號
                                                                                    //路邊計時器：車位號碼
                                                                                    //xAVM ：機號
                                                                                    //Dongle ：店號
                                                                                    //其他設備：無
                                                                                    //左靠右補空白，如
                                                                                    //'123-ABΔΔΔΔΔΔΔΔΔΔΔΔΔΔ'
                                                                                    //來源: 設備
#if(ECC_LOG_USE_NEW_FILE_NAME)
    //咪表ID_訂單號_時間(yyyyMMddHHmmss)_001U.ECC
    sprintf(currentECCFeedbackLogFileName, "%s_%s_%s_052U.ECC", GetMeterData()->epmIdStr, GetMeterData()->bookingId, getECCLogUTCTimeStr(utcTime));
#else 
    sprintf(currentECCFeedbackLogFileName, "00220.%03d_%08d_%05d_00000.%s_052U.ECC", newSpID, newSpID, svceLocID, getECCLogUTCTimeStr(utcTime));
#endif
    
    #if(1)
    prData = (uint8_t*)&(pECCBlkFeedbackLogBody);
    prDataLen = sizeof(ECCBlkFeedbackLogBody);
    sysprintf("--- pECCBlkFeedbackLogBody:  len = %d --->\r\n", prDataLen);
    for(i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- pECCBlkFeedbackLogBody ---\r\n");
    #endif
    return TRUE;
    #endif
}


char* ECCBlkFeedbackLogGetFileName(void)
{
    return currentECCFeedbackLogFileName;
}

BOOL ECCAutoLoadLogContainInit(ECCAutoLoadLogBody* body, uint32_t AgentNo, ECCCmdDCAReadResponseSuccessData* DCAReadData, int DCAReadDataLen, ECCCmdEDCADeductResponseData* EDCADeductData, int EDCADeductDataLen, uint8_t newSpID, uint32_t utcTime)
{
    #if(1)
    return TRUE;
    #else
    uint32_t i;
    uint8_t* prData; 
    uint32_t prDataLen;
    #if(!ECC_LOG_USE_NEW_FILE_NAME)
    uint32_t SP_ID_INT, SP_ID_INT_NEW, LOCATION_ID_INT_NEW;
    #endif
    ECCAutoLoadLogBody* pECCAutoLoadLogBody = body;
    uint8_t PVN = DCAReadData->PurseVersionNumber;
    sysprintf("\r\n====  ECCAutoLoadLogContainInit ====\r\n");
    sysprintf("sizeof(ECCAutoLoadLogBody) = %d\r\n", sizeof(ECCAutoLoadLogBody));

    if(sizeof(ECCAutoLoadLogBody) != TOTAL_ECC_AUTO_LOAD_LOG_BODY_SIZE)
    {
        sysprintf("ECCAutoLoadLogContainInit ERROR: sizeof(ECCAutoLoadLogBody) != TOTAL_ECC_AUTO_LOAD_LOG_BODY_SIZE [%d, %d]\r\n", sizeof(ECCBlkFeedbackLogBody), TOTAL_ECC_AUTO_LOAD_LOG_BODY_SIZE);
        return FALSE;
    }
    memset(pECCAutoLoadLogBody, 0x00, sizeof(ECCAutoLoadLogBody));
    
    // ********   Header   ********
    //pECCAutoLoadLogBody->Header;
    
    pECCAutoLoadLogBody->Header.PurseVerNo = DCAReadData->PurseVersionNumber;                 //1 PURSE_VER_NO 卡片版本 Unsigned 1 代碼參照 8.1 來源: 設備
    pECCAutoLoadLogBody->Header.CardTxnTypeID = 0x02;//EDCADeductData->MsgType;              //2 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    pECCAutoLoadLogBody->Header.CardTxnSubTypeID = 0x40;//DCAReadData->PersonalProfile;      ////交易次類別 (MsgType = 0x01 時，SubType = Personal Profile)     //3 CARD_TXN_SUBTYPE_ID 交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    pECCAutoLoadLogBody->Header.DevID[0] = DCAReadData->DeviceID[2];                  //4 DEV_ID 設備編號 Unsigned 3 SP_ID(1 Byte):如 122 即為 0x7A
    pECCAutoLoadLogBody->Header.DevID[1] = DCAReadData->DeviceID[0];                                                    //DEV_TYPE(4 Bits): 3：BV 如 BV 即為 0x3□
    pECCAutoLoadLogBody->Header.DevID[2] = DCAReadData->DeviceID[1];                                                    //DEV_ID_NO(12 Bites): 如 1234 即為 0x□4 0xD2
                                                                                                                        //組合 HEX: 0x7A 0x34 0xD2
                                                                                                                        //倒數2 Bytes LSB FIRST: 0x7A 0xD2 0x34
                                                                                                                        //來源: Mifare Sam 卡
    pECCAutoLoadLogBody->Header.SpID = DCAReadData->ServiceProviderID;                       //5 SP_ID 業者代碼 Unsigned 1  如 122 即為 0x7A 來源: 設備
    memcpy(&(pECCAutoLoadLogBody->Header.TxnTimeStamp), &(EDCADeductData->AutoLoadTXNDateTime), sizeof(pECCAutoLoadLogBody->Header.TxnTimeStamp));     //6 TXN_TIMESTAMP 交易時間 UnixTime 4 來源: 設備
    memcpy(pECCAutoLoadLogBody->Header.CardPhysicalID, DCAReadData->CardPhysicalID, sizeof(pECCAutoLoadLogBody->Header.CardPhysicalID));   //7 CARD_PHYSICAL_ID 卡片晶片號碼 Unsigned 7 來源: 設備
    pECCAutoLoadLogBody->Header.IssuerID = DCAReadData->IssuerCode;              //8 ISSUER_ID 發卡單位 Unsigned 1   PVN <> 0 時: 來源: Cpu 卡
                                                                                  //PVN = 0 時: 2 (0x02)：悠遊卡公司
                                                                                                //66 (0x42)：基隆交通卡
                                                                                                //來源: 設備
    pECCAutoLoadLogBody->Header.CardTxnSeqNo = EDCADeductData->AutoLoadTSQN[0];          //9 CARD_TXN_SEQ_NO 交易後序號 Unsigned 1 來源: 卡片
    memcpy(pECCAutoLoadLogBody->Header.TxnAMT, EDCADeductData->AutoLoadTXNAMT, sizeof(pECCAutoLoadLogBody->Header.TxnAMT));    //10 TXN_AMT 交易金額 Signed 3 來源: 設備
    memcpy(pECCAutoLoadLogBody->Header.ElectronicValue, EDCADeductData->AutoLoadPurseBalance, sizeof(pECCAutoLoadLogBody->Header.ElectronicValue));    //11 ELECTRONIC_VALUE 交易後卡片金額 Signed 3 來源: 卡片
    pECCAutoLoadLogBody->Header.SVCELocID = DCAReadData->LocationID;                  //12 SVCE_LOC_ID 場站代碼 Unsigned 1 來源: 設備
    pECCAutoLoadLogBody->Header.CardPhysicalIDLen = DCAReadData->CardPhysicalIDLength;          //13 CARD_PHYSICAL_ID_LEN 晶片號碼長度 Unsigned 1 如 4 bytes 即為 0x04
                                                                                                    //如 7 bytes 即為 0x07
                                                                                                    //來源: 卡片
    memcpy(pECCAutoLoadLogBody->Header.NewCardTxnSeqNo, EDCADeductData->AutoLoadTSQN, sizeof(pECCAutoLoadLogBody->Header.NewCardTxnSeqNo));   //14 NEW_CARD_TXN_SEQ_NO 新交易後序號 Unsigned 3 來源: 卡片
    memcpy(pECCAutoLoadLogBody->Header.NewDevID, DCAReadData->NewDeviceID, sizeof(pECCAutoLoadLogBody->Header.NewDevID));               //15 NEW_DEV_ID 新設備編號 Unsigned 6           NEW_SP_ID(3 Bytes): 如 122 即為0x00 0x00 0x7A
                                                                                                //NEW_DEV_TYPE(1 Byte): 3：BV 如 3 即為 0x03
                                                                                                //4096(含)之後開始編碼
                                                                                                //NEW_DEV_ID_NO (2 Bytes): 如 4096 即為 0x10 0x00
                                                                                                //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
                                                                                                //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                                                                //來源: Cpu Sam 卡
                                                                                                //二代卡設備未修改，以 SIS2 傳送時: 固定填 6 Bytes 0x00
    memcpy(pECCAutoLoadLogBody->Header.NewSpID, DCAReadData->NewServiceProviderID, sizeof(pECCAutoLoadLogBody->Header.NewSpID));   //16 NEW_SP_ID 新業者代碼 Unsigned 3 NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
                                                                                                                                        //以 LSB FIRST 傳送: 0x7A 0x00 0x00
                                                                                                                                        //來源: 設備
    memcpy(pECCAutoLoadLogBody->Header.NewSVCELocID, DCAReadData->NewLocationID, sizeof(pECCAutoLoadLogBody->Header.NewSVCELocID));   //17 NEW_SVCE_LOC_ID 新場站代碼 Unsigned 2 NEW_SVCE_LOC_ID(2 Bytes): 如 1 即為 0x00 0x01
                                                                                                                                            //以 LSB FIRST 傳送: 0x01 0x00
                                                                                                                                            //來源: 設備
    pECCAutoLoadLogBody->Header.NewCardTxnSubTypeID = 0x40;//0x00;  ////扣款填=0x00, AutoLoad=0x40(自動加值)      //18 NEW_CARD_TXN_SUBTYPE_ID 新交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    pECCAutoLoadLogBody->Header.NewPersonalProfile = DCAReadData->PersonalProfile;         //19 NEW_PERSONAL_PROFILE 新身份別 Unsigned 1 0x00: 一般
                                                                                                    //0x01: 敬老 1
                                                                                                    //0x02: 敬老 2
                                                                                                    //0x03: 愛心
                                                                                                    //0x04: 陪伴
                                                                                                    //0x05: 學生
                                                                                                    //0x08: 優待
                                                                                                    //來源: 卡片
    pECCAutoLoadLogBody->Header.TxnPersonalProfile = 0x00;         //20 TXN_PERSONAL_PROFILE 交易身份別 Unsigned 1 實際交易的身份別，如卡片身份別為學生，但部份業者未給予優待，以一般卡扣款，則此欄位填入一般
                                                                    //0x00: 一般
                                                                    //0x01: 敬老 1
                                                                    //0x02: 敬老 2
                                                                    //0x03: 愛心
                                                                    //0x04: 陪伴
                                                                    //0x05: 學生
                                                                    //0x08: 優待
                                                                    //來源: 設備
    pECCAutoLoadLogBody->Header.ACQID = 0x02;                      //21 ACQ_ID 收單單位 Unsigned 1   2 (0x02)：悠遊卡公司
                                                                                //66 (0x42)：基隆交通卡
                                                                                //來源: 設備
    memcpy(pECCAutoLoadLogBody->Header.CardPurseID, &(DCAReadData->PID), sizeof(pECCAutoLoadLogBody->Header.CardPurseID));    //22 CARD_PURSE_ID 錢包編號(外觀號碼) Pack 8      PVN <> 0 時:
                                                                                                    //如卡號 1234567890123456
                                                                                                    //即為 0x12 0x34 0x56 0x78 0x90 0x12 0x34 0x56
                                                                                                    //編碼參照 8.5
                                                                                                    //來源: Cpu 卡
                                                                                                //PVN = 0 時:
                                                                                                    //固定填 8 Bytes 0x00
    if (PVN == 0x00)                                                //23 CARD_CTC_SEQ_NO 卡片處理序號 Unsigned Msb 3   PVN <> 0 時: 如 1 即為 0x00 0x00 0x01
    {                           
        memset(pECCAutoLoadLogBody->Header.CardCTCSeqNo, 0x00, sizeof(pECCAutoLoadLogBody->Header.CardCTCSeqNo)); //以 MSB FIRST 傳送:  0x00 0x00 0x01
    }                                                                                                                       //來源: Cpu 卡
    else                                                                                                                    //PVN = 0 時: 固定填 3 Bytes 0x00
    {
        memcpy(pECCAutoLoadLogBody->Header.CardCTCSeqNo, EDCADeductData->AutoLoadCTC, sizeof(pECCAutoLoadLogBody->Header.CardCTCSeqNo));
    }
    pECCAutoLoadLogBody->Header.AreaCode = DCAReadData->AreaCode;                   //24 AREA_CODE 區碼 Unsigned 1 來源: 卡片
    if (PVN == 0x00)                                                                //25 SUB_AREA_CODE 附屬區碼 Unsigned 2    PVN <> 0 時: 依 3 碼郵遞區號  來源: Cpu 卡
    {                                                                                    //PVN = 0 時: 固定填 2 Bytes 0x00
        memset(pECCAutoLoadLogBody->Header.SubAreaCode, 0x00, sizeof(pECCAutoLoadLogBody->Header.SubAreaCode));
    }
    else
    {
        memcpy(pECCAutoLoadLogBody->Header.SubAreaCode, DCAReadData->SubAreaCode, sizeof(pECCAutoLoadLogBody->Header.SubAreaCode));
    }
    memcpy(pECCAutoLoadLogBody->Header.SeqNoBefTxn, DCAReadData->TXNSNBeforeTXN, sizeof(pECCAutoLoadLogBody->Header.SeqNoBefTxn));   //26 SEQ_NO_BEF_TXN 交易前序號 Unsigned 3  來源: 卡片  二代卡設備未修改，以 SIS2 傳送時: 固定填 3 Bytes 0x00
    memcpy(pECCAutoLoadLogBody->Header.EVBdfTxn, DCAReadData->PurseBalanceBeforeTXN, sizeof(pECCAutoLoadLogBody->Header.EVBdfTxn));         //27 EV_BEF_TXN 交易前卡片金額 Signed 3 來源: 卡片
    #if(1)
    prData = (uint8_t*)&(pECCAutoLoadLogBody->Header);
    prDataLen = sizeof(pECCAutoLoadLogBody->Header);
    sysprintf("--- pECCAutoLoadLogBody->Header:  len = %d --->\r\n", prDataLen);
    for(i = 0; i<prDataLen; i++)
    {
        //sysprintf(" [%03d] : 0x%02x \r\n", i, receiveData[5+i]);
        sysprintf("0x%02x, ", prData[i]);
        if(i%10 == 9)
            sysprintf("\r\n");

    }
    sysprintf("\r\n<--- pECCAutoLoadLogBody->Header ---\r\n");
    #endif
    
    // ********   Data   ********

    if (PVN == 0x00)     //1 LOYALTY_COUNTER 累積忠誠點 Unsigned 2 PVN <> 0 時: 固定填 0x00
    {                                                                                    //PVN = 0 時:
        memcpy(pECCAutoLoadLogBody->LoyaltyCounter, DCAReadData->LoyaltyCounter, sizeof(pECCAutoLoadLogBody->LoyaltyCounter));                                                     //來源: 卡片
    }
    //2018.08.14  -->A.【AGENT_NO】操作員代碼：應與扣款LOG欄位相同。    
    //pECCAutoLoadLogBody->AgentNo[0] = 0x00;             //2 AGENT_NO 操作員代碼 Unsigned 2 如 Ascii 字元'1234'即為 0x31 0x32 0x33 0x34 轉 10 進制數值 1234 即為 0x04 0xD2
    //pECCAutoLoadLogBody->AgentNo[1] = 0x01;                                                        //以 LSB FIRST 傳送:0xD2 0x04
    uint32ToUint8(AgentNo, pECCAutoLoadLogBody->AgentNo, sizeof(pECCAutoLoadLogBody->AgentNo));                                                        //來源: 設備
                                                            //此欄位 10 進制數值應與PPR_Reset/PPR_Reset_Online 一致
    pECCAutoLoadLogBody->BankCode = DCAReadData->BankCode;               //3 BANK_CODE 銀行代碼 Unsigned 1 PVN <> 0 時: 固定填 0x00
                                                            //PVN = 0 時:
                                                            //來源: 卡片
    if (PVN != 0x00)     //4 LOC_PROVIDER 設備服務提供者 Unsigned 3 PVN <> 0 時: 同 NEW_SP_ID(3 Bytes): 如 131071 即為 0x01 0xFF 0xFF
    {                                                        //以 LSB FIRST 傳送: 0xFF 0xFF 0x01
        memcpy(pECCAutoLoadLogBody->LocProvider, DCAReadData->NewServiceProviderID, sizeof(pECCAutoLoadLogBody->LocProvider));                                                     //來源: 設備
    }                                                        //PVN = 0 時: 同 SP_ID(1 Bytes): 如 2 即為 0x02
    else                                                        //補足 3 Bytes 為 0x00 0x00 0x02
    {                                                        //以 LSB FIRST 傳送:0x02 0x00 0x00
        pECCAutoLoadLogBody->LocProvider[0] = newSpID;                                                    //來源: 設備
        pECCAutoLoadLogBody->LocProvider[1] = 0x00;  
        pECCAutoLoadLogBody->LocProvider[2] = 0x00;  
    }
    
    memset(pECCAutoLoadLogBody->MachineID, 0x20, sizeof(pECCAutoLoadLogBody->MachineID));  //5 MACHINE_ID 機器編號 Ascii 20 SP_ID(2 Bytes): '49' AVM='0',VAVM='1'(1 Byte) 流水編號(3 Bytes): '050’ 左靠右補空白，如 '490050ΔΔΔΔΔΔΔΔΔΔΔΔΔΔ' 來源: 設備
    //2018.08.14  -->B.【MACHINE_ID】機器編號：應填  2020202020202020202020202020202020202020 ，卻填 7C20202020202020202020202020202020202020。
    //diff with leon sample code
    //pECCAutoLoadLogBody->MachineID[0] = newSpID;

    #warning need check here
    memset(pECCAutoLoadLogBody->CMID, ' ', sizeof(pECCAutoLoadLogBody->CMID));             //6 CM_ID 鈔箱編號 Ascii 20 MACHINE_ID(6 Bytes) '-' 卸鈔日 yyyymmdd(8 Bytes)'-' 卸鈔序號(4 Bytes) 左靠右補空白，如'490050-20100101-1ΔΔΔ' 來源: 設備
    memset(pECCAutoLoadLogBody->RFU1, 0x0, sizeof(pECCAutoLoadLogBody->RFU1));             //7 RFU1 保留欄位(共用) Byte 16 PVN <> 0 時: 固定填 16 Bytes 0x00
                                                                                                    //PVN = 0 時: 固定填 16 Bytes 0x00
    pECCAutoLoadLogBody->RFU1Ver = 0x00;                //8 RFU1_VER 欄位版本(共用) Unsigned 1 PVN <> 0 時:固定填 0x00
                                                                                    //PVN = 0 時: 固定填 0x00
    //2018.08.14  -->C.【RFU2】保留欄位：前面2個Byte應填 0050，卻填0000。
    memset(pECCAutoLoadLogBody->RFU2, 0x0, sizeof(pECCAutoLoadLogBody->RFU2));     //9 RFU2 保留欄位(業者) Byte 16 PVN <> 0 時: 固定填 16 Bytes 0x00
                                                                                //PVN = 0 時: 固定填 16 Bytes 0x00
    pECCAutoLoadLogBody->RFU2[0] = 0x00;//SVCE_LOC_ID;
    pECCAutoLoadLogBody->RFU2[1] = SVCE_LOC_ID ;
    
    
    //2018.08.14  -->D.【RFU2_VER】欄位版本：應填 01，卻填 00。
    pECCAutoLoadLogBody->RFU2Ver = 0x01;                  //10 RFU2_VER 欄位版本(業者) Unsigned 1 PVN <> 0 時: 固定填 0x00
                                                                                        //PVN = 0 時: 固定填 0x00
    if (PVN != 0x00)
    {
        pECCAutoLoadLogBody->TXNMode = EDCADeductData->AutoLoadTM;      //11 TM(TXN Mode) 交易模式 Byte 1 PVN <> 0 時:來源: 設備
                                                                                                    //PVN = 0 時: 固定填 0x00
        pECCAutoLoadLogBody->TXNQualifier = EDCADeductData->AutoLoadTQ; //12 TQ(TXN Qualifier) 交易屬性 Byte 1 PVN <> 0 時: 來源: 設備
                                                                                            //PVN = 0 時: 固定填 0x00
        pECCAutoLoadLogBody->SignKeyVer = EDCADeductData->SignatureKeyKVN;;             //13 SIGN_KEYVER SIGNATURE KEY 版本 Byte 1 PVN <> 0 時: 來源: Cpu 卡
                                                                                        //PVN = 0 時: 固定填 0x00
        memcpy(pECCAutoLoadLogBody->SignValue, EDCADeductData->AutoLoadSIGN, sizeof(pECCAutoLoadLogBody->SignValue));   //14 SIGN_VALUE SIGNATURE 值 Byte 16 PVN <> 0 時: 來源: Cpu 卡
                                                                        //PVN = 0 時: 固定填 16 Bytes 0x00
    }
    // ********   Tail   ********
    if (PVN == 0x00)
    {
        memcpy(&(pECCAutoLoadLogBody->Tail.MACKeySet), EDCADeductData->AutoLoadMAC, 10); //MAC 10 bytes   //1 MACKeySet 0x00 Byte 1 固定填 0x00
    }
    else
    {
        //uint8_t         MACKeySet;                  //1 MACKeySet 0x00 Byte 1 固定填 0x00
        //uint8_t         MAC3DESKey;                 //2 MAC3DESKey 0x01 Byte 1 固定填 0x01
        //uint8_t         MACValue[4];                //3 MACValue 交易驗証碼 Unsigned 4
                                                            //PVN <> 0 時: 固定填 4 Bytes 0x00
                                                            //PVN = 0 時: 來源:設備
        //uint8_t         MACMFRC[4];                 //4 MACMFRC 讀卡機編號 Unsigned 4
                                                            //PVN <> 0 時: 固定填 4 Bytes 0x00
                                                            //PVN = 0 時: 來源:設備
    }
    pECCAutoLoadLogBody->Tail.MAC3DESKey = 0x01;         //2 MAC3DESKey 0x01 Byte 1 固定填 0x01
    
    #warning two case are the same
    if (PVN == 0x00)
    {// PPR_RESET 舊SAM Info，PPR_RESET_OFFLINE 的舊SAM ID 為 0x00， 所以用新SAM ID
        memcpy(pECCAutoLoadLogBody->Tail.SAMID, EDCADeductData->SID, sizeof(pECCAutoLoadLogBody->Tail.SAMID));  //5 SAM_ID SAM ID Pack 8
                                                        //PVN <> 0 時: 來源: Cpu Sam 卡
                                                        //PVN = 0 時: 來源:設備
    }
    else
    {// PPR_RESET 新SAM Info
        memcpy(pECCAutoLoadLogBody->Tail.SAMID, EDCADeductData->SID, sizeof(pECCAutoLoadLogBody->Tail.SAMID)); 
    }
    
    if (PVN != 0x00)
    {
        pECCAutoLoadLogBody->Tail.HashType = EDCADeductData->HashType;                   //6 HASH_TYPE HASH_TYPE Unsigned 1
                                                        //PVN <> 0 時: 代碼參照 8.4 來源: 設備
                                                        //PVN = 0 時: 固定填 0x00
    }
    if (PVN != 0x00)
    {
        pECCAutoLoadLogBody->Tail.HostAdminKeyVer = EDCADeductData->HostAdminKeyKVN;            //7 HOST_ADMIN_KEYVER HOST ADMIN KEY 版本 Byte 1
                                                        //PVN <> 0 時: MAC 用哪個版本 KEY 去押的  來源: Cpu Sam 卡
                                                        //PVN = 0 時: 固定填 0x00
    }
    if (PVN != 0x00)
    {
        memcpy(pECCAutoLoadLogBody->Tail.CpuMACValue, EDCADeductData->AutoLoadMAC, sizeof(pECCAutoLoadLogBody->Tail.CpuMACValue));    //8 CPU_MAC_VALUE MAC 值 Byte 16
                                                        //PVN <> 0 時: 來源: Cpu Sam 卡計算
                                                        //PVN = 0 時: 固定填 16 Bytes 0x00
    }
    
    
    
#if(ECC_LOG_USE_NEW_FILE_NAME)
    //咪表ID_訂單號_時間(yyyyMMddHHmmss)_001U.ECC
    sprintf(currentECCAutoLoadLogFileName, "%s_%s_%s_002U.ECC", GetMeterData()->epmIdStr, GetMeterData()->bookingId, getECCLogUTCTimeStr(utcTime));
#else 
    SP_ID_INT = DCAReadData->ServiceProviderID;
    SP_ID_INT_NEW = uint8ToUint32(DCAReadData->NewServiceProviderID, sizeof(DCAReadData->NewServiceProviderID));
    LOCATION_ID_INT_NEW = uint8ToUint32(DCAReadData->NewLocationID, sizeof(DCAReadData->NewLocationID));
    sprintf(currentECCAutoLoadLogFileName, "00220.%03d_%08d_%05d_00000.%s_002U.ECC", SP_ID_INT, SP_ID_INT_NEW, LOCATION_ID_INT_NEW, getECCLogUTCTimeStr(utcTime));
#endif
    return TRUE;
    #endif
}
char* ECCAutoLoadLogGetFileName(void)
{
    return currentECCAutoLoadLogFileName;
}
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


/**************************************************************************//**
* @file     ipassdpti.c
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
    #include "ipassdpti.h"
    #include "cardlogcommon.h"
    #define  sysprintf       printf
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
    #include "ipassdpti.h"
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
static IPassDPTIBody iPassDPTI;
static char currentIPASSDTFIFileName[_MAX_LFN];
static uint32_t DPTITraceNo = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
#define DATA_SHIFT_BYTE     0
BOOL IPASSLogDPTIContainInit(uint8_t* body, char CmdType, uint8_t* icdData, int icdLen, uint8_t* executeData, int executeLen, uint32_t txnAmount, uint8_t* readerId, int readerIdLen, char* dateStr, char* timeStr, char* APIVersion)
{
    IPassDPTIBody* pIPassDPTIBody = (IPassDPTIBody*)body;
    
    sysprintf("\r\n====  IPASSLogDPTIContainInit(%s) ====\r\n", GetMeterData()->epmIdStr);
    sysprintf("sizeof(IPassDPTIBody) = %d\r\n", sizeof(IPassDPTIBody));

    if(sizeof(IPassDPTIBody) != TOTAL_IPASS_DPTI_BODY_SIZE)
    {
        sysprintf("IPASSLogInit ERROR: sizeof(IPassDPTIBody) != TOTAL_IPASS_DPTI_BODY_SIZE [%d, %d]\r\n", sizeof(IPassDPTIBody), TOTAL_IPASS_DPTI_BODY_SIZE);
        return FALSE;
    }
    memset(pIPassDPTIBody, ' ', sizeof(IPassDPTIBody));
    DPTITraceNo++;
    //--- body ---
    /*
--- ICD [150] --->
0xAA, 0xD5, 0x17, 0x7C, 0x14, 0x88, 0x04, 0x00, 0x47, 0xC1,
0x26, 0x35, 0xB9, 0x00, 0x05, 0x07, 0x04, 0x00, 0xF8, 0x11,
0x00, 0x00, 0xF8, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x6F, 0xA9, 0x59,
0x20, 0x01, 0x00, 0xF8, 0x11, 0xA1, 0x2D, 0x30, 0x30, 0x30,
0x32, 0x00, 0x65, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x6F,
0xA9, 0x59, 0x00, 0x00, 0x20, 0xA1, 0x2D, 0x30, 0x30, 0x30,
0x32, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2B, 0xB1, 0x3A,
0x57, 0x19, 0x88, 0x13, 0x88, 0x13, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32,

<--- ICD ---
iPass_Execute() deduct_value is 1 (1)

--- iPass_Execute [75] --->
0xF7, 0x11, 0x00, 0x00, 0x66, 0x00, 0x50, 0x36, 0x01, 0x49,
0x83, 0x97, 0x74, 0x49, 0x40, 0x47, 0xD8, 0x6E, 0x00, 0x00,
0x30, 0x30, 0x30, 0x32, 0x20, 0x17, 0x09, 0x01, 0x22, 0x39,
0x50, 0x24, 0x00, 0x00, 0x01, 0x00, 0x46, 0x00, 0xAA, 0xD5,
0x17, 0x7C, 0x14, 0x88, 0x04, 0x00, 0x47, 0xC1, 0x26, 0x35,
0xB9, 0x00, 0x05, 0x07, 0x00, 0x01, 0x02, 0x19, 0x70, 0x01,
0x01, 0x08, 0x00, 0x00, 0x50, 0x36, 0x01, 0x49, 0x83, 0x97,
0x74, 0x49, 0x02, 0x02, 0xE4,
<--- iPass_Execute ---
    */
    
    pIPassDPTIBody->RecordType = 'D';                                                                                       //1     Record Type 
    CardLogUint32ToString("DPTITraceNo", DPTITraceNo, pIPassDPTIBody->DPTITraceNo, sizeof(pIPassDPTIBody->DPTITraceNo));    //2     DPTI 資料編號 
    memcpy(pIPassDPTIBody->EquipmentType, "33", sizeof(pIPassDPTIBody->EquipmentType));   //33 自動繳費機                   //3     交易設備種類 
    memcpy(pIPassDPTIBody->EquipmentId, GetMeterData()->epmIdStr, sizeof(pIPassDPTIBody->EquipmentId));                     //4     交易設備編號 
    memcpy(pIPassDPTIBody->CarType, "01", sizeof(pIPassDPTIBody->CarType));  //01 小型車                                    //5     車種 
    memcpy(pIPassDPTIBody->txnDate.value, dateStr, sizeof(pIPassDPTIBody->txnDate.value));                                  //6     交易日期 
    memcpy(pIPassDPTIBody->txnTime.value, timeStr, sizeof(pIPassDPTIBody->txnTime.value));                                  //7     交易時間 
    if(CmdType =='D')
    {
        memcpy(pIPassDPTIBody->TxnType, "24", sizeof(pIPassDPTIBody->TxnType));                                             //8     交易類型 
    }
    if (CmdType == 'R')
    {
        memcpy(pIPassDPTIBody->TxnType, "90", sizeof(pIPassDPTIBody->TxnType));                                             //8     交易類型 
    }
    else if (CmdType == 'L')
    {
        memcpy(pIPassDPTIBody->TxnType, "91", sizeof(pIPassDPTIBody->TxnType));                                             //8     交易類型 
    }    
    
    uint32_t RWid = CardLogHexBuffToUint32(&readerId[0], 4);
    CardLogUint32ToHexString("RWid", RWid, pIPassDPTIBody->RWid, sizeof(pIPassDPTIBody->RWid));                             //9     讀卡器編號 
    
    memcpy(pIPassDPTIBody->CarNo, "        ", sizeof(pIPassDPTIBody->CarNo));                                               //10    車號 
    
    if(CmdType =='D')
    {
        CardLogUint32ToString("txnAmount", txnAmount, pIPassDPTIBody->TxnAmount, sizeof(pIPassDPTIBody->TxnAmount));        //11    交易金額 
    }
    else if (CmdType == 'R')
    {
        CardLogUint32ToString("txnAmount", 0, pIPassDPTIBody->TxnAmount, sizeof(pIPassDPTIBody->TxnAmount));                //11    交易金額 
    }
    else if (CmdType == 'L')
    {
        CardLogUint32ToString("txnAmount", 0, pIPassDPTIBody->TxnAmount, sizeof(pIPassDPTIBody->TxnAmount));                //11    交易金額 
    }   
    
    if(CmdType =='D')
    {
        uint32_t BalanceBeforeTxn = CardLogBuffToUint32(&icdData[18], 4);
        CardLogUint32ToString("BalanceBeforeTxn", BalanceBeforeTxn, pIPassDPTIBody->BalanceBeforeTxn, sizeof(pIPassDPTIBody->BalanceBeforeTxn));    //12    交易前卡片餘額 
    }
    else if (CmdType == 'R')
    {
        uint32_t BalanceBeforeTxn = CardLogBuffToUint32(&icdData[22], 4);
        CardLogUint32ToString("BalanceBeforeTxn", BalanceBeforeTxn, pIPassDPTIBody->BalanceBeforeTxn, sizeof(pIPassDPTIBody->BalanceBeforeTxn));    //12    交易前卡片餘額 
    }
    else if (CmdType == 'L')
    {
        uint32_t BalanceBeforeTxn = CardLogBuffToUint32(&icdData[18], 4);
        CardLogUint32ToString("BalanceBeforeTxn", BalanceBeforeTxn, pIPassDPTIBody->BalanceBeforeTxn, sizeof(pIPassDPTIBody->BalanceBeforeTxn));    //12    交易前卡片餘額 
    }   
   
    if(CmdType =='D')
    {
        uint32_t BalanceAfterTxn = CardLogBuffToUint32(&executeData[0], 4);
        CardLogUint32ToString("BalanceAfterTxn", BalanceAfterTxn, pIPassDPTIBody->BalanceAfterTxn, sizeof(pIPassDPTIBody->BalanceAfterTxn));        //13    交易後卡片餘額
    }
    else if (CmdType == 'R')
    {
        uint32_t BalanceAfterTxn = CardLogBuffToUint32(&icdData[18], 4);
        CardLogUint32ToString("BalanceAfterTxn", BalanceAfterTxn, pIPassDPTIBody->BalanceAfterTxn, sizeof(pIPassDPTIBody->BalanceAfterTxn));        //13    交易後卡片餘額
    }
    else if (CmdType == 'L')
    {
        uint32_t BalanceAfterTxn = CardLogBuffToUint32(&executeData[0], 4);
        CardLogUint32ToString("BalanceAfterTxn", BalanceAfterTxn, pIPassDPTIBody->BalanceAfterTxn, sizeof(pIPassDPTIBody->BalanceAfterTxn));        //13    交易後卡片餘額
    }   
    
    
    CardLogCSNToHexString("CardSerialNo", &icdData[0], pIPassDPTIBody->CardSerialNo);       //14    卡號(CSN) 
    
    if(CmdType =='D')
    {
        uint32_t TxnSeqNo = CardLogBuffToUint32(&executeData[4], 2);
        CardLogUint32ToString("KRTCTxnSeqNo", TxnSeqNo, pIPassDPTIBody->KRTCTxnSeqNo, sizeof(pIPassDPTIBody->KRTCTxnSeqNo));        //15    卡片交易序號 
    }
    else if (CmdType == 'R')
    {
        uint32_t TxnSeqNo = CardLogBuffToUint32(&icdData[52], 2);
        CardLogUint32ToString("KRTCTxnSeqNo", TxnSeqNo, pIPassDPTIBody->KRTCTxnSeqNo, sizeof(pIPassDPTIBody->KRTCTxnSeqNo));        //15    卡片交易序號 
    }
    else if (CmdType == 'L')
    {
        uint32_t TxnSeqNo = CardLogBuffToUint32(&executeData[4], 2);
        CardLogUint32ToString("KRTCTxnSeqNo", TxnSeqNo, pIPassDPTIBody->KRTCTxnSeqNo, sizeof(pIPassDPTIBody->KRTCTxnSeqNo));        //15    卡片交易序號 
    } 
    
    
    uint32_t IdentityType = CardLogBuffToUint32(&icdData[17], 1);
    CardLogUint32ToHexString("IdentityType", IdentityType, pIPassDPTIBody->IdentityType, sizeof(pIPassDPTIBody->IdentityType));     //16    個人身份別 
    
    uint32_t CardType = CardLogBuffToUint32(&icdData[16], 1);
    CardLogUint32ToHexString("CardType", CardType, pIPassDPTIBody->CardType, sizeof(pIPassDPTIBody->CardType));                     //17    卡片種類 
    
    uint32_t IssueCode = CardLogBuffToUint32(&icdData[54], 1);
    CardLogUint32ToHexString("IssueCode", IssueCode, pIPassDPTIBody->IssueCode, sizeof(pIPassDPTIBody->IssueCode));                 //18    發卡單位   diff DFTI
    
    
    memcpy(&pIPassDPTIBody->TicketStatus, "1", sizeof(pIPassDPTIBody->TicketStatus));                                               //19    繳費/定期票設定狀態  // 1 已繳費
    memcpy(pIPassDPTIBody->PaymentDate.value, dateStr, sizeof(pIPassDPTIBody->PaymentDate.value));                                  //20    繳費日期 
    memcpy(pIPassDPTIBody->PaymentTime.value, timeStr, sizeof(pIPassDPTIBody->PaymentTime.value));                                  //21    繳費時間 
    memcpy(&pIPassDPTIBody->PaymentMethod, "A", sizeof(pIPassDPTIBody->PaymentMethod));                                             //22    "繳費方式與繳費後進出場狀態"
    memcpy(pIPassDPTIBody->InDate.value, dateStr, sizeof(pIPassDPTIBody->InDate.value));                                            //23    進場日期 
    memcpy(pIPassDPTIBody->InTime.value, timeStr, sizeof(pIPassDPTIBody->InTime.value));                                            //24    進場時間 
    memcpy(pIPassDPTIBody->SocialWelfareOffer, "000000", sizeof(pIPassDPTIBody->SocialWelfareOffer));                               //25    社福優惠金額 
    memcpy(pIPassDPTIBody->TransferStopFlag, "0000", sizeof(pIPassDPTIBody->TransferStopFlag));                                     //26    轉停優惠旗標 
    memcpy(pIPassDPTIBody->TransferStopOffer, "000000", sizeof(pIPassDPTIBody->TransferStopOffer));                                 //27    轉停優惠金額 
    memcpy(pIPassDPTIBody->ProjectFlag, "00", sizeof(pIPassDPTIBody->ProjectFlag));                                                 //28    專案優惠旗標 
    memcpy(pIPassDPTIBody->ProjectOffer, "000000", sizeof(pIPassDPTIBody->ProjectOffer));                                           //29    專案優惠金額 
    
    uint32_t Ticket1IssueCode = CardLogBuffToUint32(&icdData[55], 2);
    CardLogUint32ToHexString("Ticket1IssueCode", Ticket1IssueCode, pIPassDPTIBody->Ticket1IssueCode, sizeof(pIPassDPTIBody->Ticket1IssueCode));     //30    定期票 1 發行單位 
    
    uint32_t Ticket1Type = CardLogBuffToUint32(&icdData[58], 1);
    CardLogUint32ToHexString("Ticket1Type", Ticket1Type, pIPassDPTIBody->Ticket1Type, sizeof(pIPassDPTIBody->Ticket1Type));                         //31    定期票 1 種類 
    
    uint32_t Ticket1StartDate = CardLogBuffToUint32(&icdData[59], 2);
    CardLogUint32ToDosTime("Ticket1StartDate", Ticket1StartDate, pIPassDPTIBody->Ticket1StartDate.value, sizeof(pIPassDPTIBody->Ticket1StartDate.value));   //32    定期票 1 起始日期 
         
    uint32_t Ticket1EndDate = CardLogBuffToUint32(&icdData[61], 2);
    CardLogUint32ToDosTime("Ticket1EndDate", Ticket1EndDate, pIPassDPTIBody->Ticket1EndDate.value, sizeof(pIPassDPTIBody->Ticket1EndDate.value));           //33    定期票 1 結束日期 
    
    uint32_t Ticket2IssueCode = CardLogBuffToUint32(&icdData[64], 2);
    CardLogUint32ToHexString("Ticket2IssueCode", Ticket2IssueCode, pIPassDPTIBody->Ticket2IssueCode, sizeof(pIPassDPTIBody->Ticket2IssueCode));             //34    定期票 2 發行單位 
    
    uint32_t Ticket2Type = CardLogBuffToUint32(&icdData[66], 1);
    CardLogUint32ToHexString("Ticket2Type", Ticket2Type, pIPassDPTIBody->Ticket2Type, sizeof(pIPassDPTIBody->Ticket2Type));                                 //35    定期票 2 種類 
    
    uint32_t Ticket2StartDate = CardLogBuffToUint32(&icdData[67], 2);
    CardLogUint32ToDosTime("Ticket2StartDate", Ticket2StartDate, pIPassDPTIBody->Ticket2StartDate.value, sizeof(pIPassDPTIBody->Ticket2StartDate.value));   //36    定期票 2 起始日期 
    
    uint32_t Ticket2EndDate = CardLogBuffToUint32(&icdData[69], 2);
    CardLogUint32ToDosTime("Ticket2EndDate", Ticket2EndDate, pIPassDPTIBody->Ticket2EndDate.value, sizeof(pIPassDPTIBody->Ticket2EndDate.value));           //37    定期票 2 結束日期 
    
    RTC_TIME_DATA_T time;
    char PreviousTxnDateStr[10];
    char PreviousTxnTimeStr[8];     
    time_t   PreviousTxnDateUTC = CardLogBuffToUint32(&icdData[36], 4);
    Time2RTC(PreviousTxnDateUTC, &time);    
    sprintf(PreviousTxnDateStr, "%04d%02d%02d", time.u32Year, time.u32cMonth, time.u32cDay);
    sprintf(PreviousTxnTimeStr, "%02d%02d%02d", time.u32cHour, time.u32cMinute, time.u32cSecond);
    memcpy(pIPassDPTIBody->PreviousTxnDate.value, PreviousTxnDateStr, sizeof(pIPassDPTIBody->PreviousTxnDate.value));   //38    前筆交易日期 
    memcpy(pIPassDPTIBody->PreviousTxnTime.value, PreviousTxnTimeStr, sizeof(pIPassDPTIBody->PreviousTxnTime.value));   //39    前筆交易時間 

    uint32_t PreviousTxnType = CardLogBuffToUint32(&icdData[40], 1);
    CardLogUint32ToHexString("PreviousTxnType", PreviousTxnType, pIPassDPTIBody->PreviousTxnType, sizeof(pIPassDPTIBody->PreviousTxnType));             //40    前筆交易類別 
    
    uint32_t PreviousTxnAmount = CardLogBuffToUint32(&icdData[41], 2);
    CardLogUint32ToString("PreviousTxnAmount", PreviousTxnAmount, pIPassDPTIBody->PreviousTxnAmount, sizeof(pIPassDPTIBody->PreviousTxnAmount));        //41    前筆交易票值/票點 
    
    uint32_t PreviousBalanceAfterTxn = CardLogBuffToUint32(&icdData[43], 2);
    CardLogUint32ToString("PreviousBalanceAfterTxn", PreviousBalanceAfterTxn, pIPassDPTIBody->PreviousBalanceAfterTxn, sizeof(pIPassDPTIBody->PreviousBalanceAfterTxn)); //42    前筆交易後票值/點 
    
    uint32_t PreviousTxnSysID = CardLogBuffToUint32(&icdData[45], 1);
    CardLogUint32ToHexString("PreviousTxnSysID", PreviousTxnSysID, pIPassDPTIBody->PreviousTxnSysID, sizeof(pIPassDPTIBody->PreviousTxnSysID));         //43    前筆交易系統編號 
    
    uint32_t PreviousTxnLocation = CardLogBuffToUint32(&icdData[46], 1);
    CardLogUint32ToHexString("PreviousTxnLocation", PreviousTxnLocation, pIPassDPTIBody->PreviousTxnLocation, sizeof(pIPassDPTIBody->PreviousTxnLocation));     //44    前筆交易地點/業者 
  
    uint32_t PreviousTxnequipmentID = CardLogBuffToUint32(&icdData[47], 4);
    CardLogUint32ToHexString("PreviousTxnequipmentID", PreviousTxnequipmentID, pIPassDPTIBody->PreviousTxnequipmentID, sizeof(pIPassDPTIBody->PreviousTxnequipmentID));  //45    前筆交易設備編號 
    
    memcpy(pIPassDPTIBody->APIVersion, APIVersion, sizeof(pIPassDPTIBody->APIVersion));                             //46    API Ver. 
    memcpy(pIPassDPTIBody->BusinessDate.value, dateStr, sizeof(pIPassDPTIBody->BusinessDate.value));                //47    Business Date 
    CardLogFillSpace(pIPassDPTIBody->Reserved, sizeof(pIPassDPTIBody->Reserved));                                   //48    Reserved 
    
    uint32_t SAMID_1 = CardLogHexBuffToUint32(&executeData[6], 4);
    uint32_t SAMID_2 = CardLogHexBuffToUint32(&executeData[10], 4);
    CardLogUint32ToHexString("SAMID_1", SAMID_1, &(pIPassDPTIBody->SAMID[0]),                               sizeof(pIPassDPTIBody->SAMID)/2);                           //49    SAM id 
    CardLogUint32ToHexString("SAMID_2", SAMID_2, &(pIPassDPTIBody->SAMID[sizeof(pIPassDPTIBody->SAMID)/2]), sizeof(pIPassDPTIBody->SAMID)/2);
              
    uint32_t TxnAuthenticationCode = CardLogHexBuffToUint32(&executeData[14], 4);
    CardLogUint32ToHexString("TxnAuthenticationCode", TxnAuthenticationCode, pIPassDPTIBody->TxnAuthenticationCode, sizeof(pIPassDPTIBody->TxnAuthenticationCode));    //50    交易驗證碼(TAC) 
    memcpy(&pIPassDPTIBody->TACVerifyResult, "0", sizeof(pIPassDPTIBody->TACVerifyResult));             //51    TAC Varify 
    
    CardLogFillSeparator(&(pIPassDPTIBody->Separator));         //52    Record Separator 
    pIPassDPTIBody->endChar = 0x0;; 
    sysprintf("\r\n====  IPASSLogInit Body:[%s]\r\n", pIPassDPTIBody);
    
    
    return TRUE;
}
uint8_t* IPASSLogDPTIGetContain(int* dataLen)
{
    * dataLen = sizeof(IPassDPTIBody);
    return (uint8_t*)&iPassDPTI;
}
char* IPASSLogDPTIGetFileName(void)
{
    return currentIPASSDTFIFileName;
}
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


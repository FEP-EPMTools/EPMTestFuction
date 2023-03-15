/**************************************************************************//**
* @file     ipassdpti.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __IPASS_DPTI_H__
#define __IPASS_DPTI_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _PC_ENV_
#else
    #include "nuc970.h"
    #include "sys.h"
#endif

#include "ipasscommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define IPASS_DPTI_FILE_SAVE_POSITION          FILE_AGENT_STORAGE_TYPE_AUTO
#define IPASS_DPTI_FILE_EXTENSION              "dpt"
#define IPASS_DPTI_FILE_EXTENSION_SFLASH       "dpts"
#define IPASS_DPTI_FILE_DIR                    "1:"  //沒用到 任意值
    
    
#pragma pack(1)
typedef struct
{
    uint8_t         RecordType;                 //1	    Record Type 
    uint8_t         DPTITraceNo[6];             //2	    DPTI 資料編號 
    uint8_t         EquipmentType[2];           //3	    交易設備種類 
    uint8_t         EquipmentId[8];             //4	    交易設備編號 
    uint8_t         CarType[2];                 //5	    車種 
    IPassDate       txnDate;                    //6	    交易日期 
    IPassTime       txnTime;                    //7	    交易時間 
    uint8_t         TxnType[2];                 //8	    交易類型 
    uint8_t         RWid[8];                    //9	    讀卡器編號 
    uint8_t         CarNo[8];                   //10	車號 
    uint8_t         TxnAmount[6];               //11	交易金額 
    uint8_t         BalanceBeforeTxn[6];        //12	交易前卡片餘額 
    uint8_t         BalanceAfterTxn[6];         //13	交易後卡片餘額 
    uint8_t         CardSerialNo[32];           //14	卡號(CSN) 
    uint8_t         KRTCTxnSeqNo[6];            //15	卡片交易序號 
    uint8_t         IdentityType[2];            //16	個人身份別 
    uint8_t         CardType[2];                //17	卡片種類 
    uint8_t         IssueCode[3];               //18	發卡單位   diff DFTI
    uint8_t         TicketStatus;               //19	繳費/定期票設定狀態 
    IPassDate       PaymentDate;                //20	繳費日期 
    IPassTime       PaymentTime;                //21	繳費時間 
    uint8_t         PaymentMethod;              //22	"繳費方式與繳費後進出場狀態"
    IPassDate       InDate;                     //23	進場日期 
    IPassTime       InTime;                     //24	進場時間 
    uint8_t         SocialWelfareOffer[6];      //25	社福優惠金額 
    uint8_t         TransferStopFlag[4];        //26	轉停優惠旗標 
    uint8_t         TransferStopOffer[6];       //27	轉停優惠金額 
    uint8_t         ProjectFlag[2];             //28	專案優惠旗標 
    uint8_t         ProjectOffer[6];            //29	專案優惠金額 
    uint8_t         Ticket1IssueCode[4];        //30	定期票 1 發行單位 
    uint8_t         Ticket1Type[2];             //31	定期票 1 種類 
    IPassDate       Ticket1StartDate;           //32	定期票 1 起始日期 
    IPassDate       Ticket1EndDate;             //33	定期票 1 結束日期 
    uint8_t         Ticket2IssueCode[4];        //34	定期票 2 發行單位 
    uint8_t         Ticket2Type[2];             //35	定期票 2 種類 
    IPassDate       Ticket2StartDate;           //36	定期票 2 起始日期 
    IPassDate       Ticket2EndDate;             //37	定期票 2 結束日期 
    IPassDate       PreviousTxnDate;            //38	前筆交易日期 
    IPassTime       PreviousTxnTime;            //39	前筆交易時間 
    uint8_t         PreviousTxnType[2];         //40	前筆交易類別 
    uint8_t         PreviousTxnAmount[6];       //41	前筆交易票值/票點 
    uint8_t         PreviousBalanceAfterTxn[6]; //42	前筆交易後票值/點 
    uint8_t         PreviousTxnSysID[2];        //43	前筆交易系統編號 
    uint8_t         PreviousTxnLocation[2];     //44	前筆交易地點/業者 
    uint8_t         PreviousTxnequipmentID[8];  //45	前筆交易設備編號 
    uint8_t         APIVersion[4];              //46	API Ver. 
    IPassDate       BusinessDate;               //47	Business Date 
    uint8_t         Reserved[14];               //48	Reserved 
    uint8_t         SAMID[16];                  //49	SAM id 
    uint8_t         TxnAuthenticationCode[8];   //50	交易驗證碼(TAC) 
    uint8_t         TACVerifyResult;            //51	TAC Varify 
    IPassSeparator  Separator;                  //52	Record Separator 
    char            endChar;  
}IPassDPTIBody;
#define TOTAL_IPASS_DPTI_BODY_SIZE   (305 + 1)

#pragma pack()
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL IPASSLogDPTIContainInit(uint8_t* body, char CmdType, uint8_t* icdData, int icdLen, uint8_t* executeData, int executeLen, uint32_t txnAmount, uint8_t* readerId, int readerIdLen, char* dateStr, char* timeStr, char* APIVersion);
char* IPASSLogDPTIGetFileName(void);
#ifdef __cplusplus
}
#endif

#endif //__IPASS_DPTI_H__

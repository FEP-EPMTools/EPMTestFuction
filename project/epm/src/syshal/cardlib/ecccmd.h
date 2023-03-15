/**************************************************************************//**
* @file     ecccmd.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef _ECC_CMD_
#define _ECC_CMD_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _PC_ENV_
    #include "basetype.h"
#else
    #include "nuc970.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ENABLE_ECC_AUTO_LOAD     0//1
#if(0)
    #define ECC_SIGN_ON_SERVER_IP     "52.196.223.34"
    #define ECC_SIGN_ON_SERVER_PORT     7878
#else    
//16:37 Leon 麒鈞 取水位看一下是否可以用 IP:118.163.153.125
//6:37 Leon 麒鈞 PORT:4007
    #define ECC_SIGN_ON_SERVER_IP     "118.163.153.125"//"13.115.255.101"
    #define ECC_SIGN_ON_SERVER_PORT     4007
#endif
//#define ECC_SIGN_ON_SERVER_IP     "172.16.11.20"
//#define ECC_SIGN_ON_SERVER_PORT     8348    
    
    
#warning need check here

#define TM_LOCATION_ID      1
#define TM_ID               1
#define TM_AGENT_NUMBER     1

//綠創新業者代碼 130， LOCALTION 50
#define NEW_SP_ID           0x82//綠創新業者代碼 130   //0x7C
#define SVCE_LOC_ID         0x32//LOCALTION 50


#pragma pack(1)
// ----- Common -----
typedef struct
{
    uint8_t  value[6];
}ECCCmdSerialNumber; 

typedef struct
{
    uint8_t  value[4];
}ECCCmdDataTime; 

//typedef struct
//{
//    union 
//    {
//        uint8_t     u8Value[2];
//        uint16_t    u16Value;
//    }value;
//}ECCCmdResponseStatus; 

typedef struct
{
    uint8_t     value[8];
}ECCCmdPID; 

/*
Header	Command	Len	        Body	        Tail
EA	    04	01	LL+1 (2B)	Data	LRC	    90	00

Data:依照悠遊卡公司提供相關文件帶入

Response:
Header	Command	Len	        Body	Tail
EA	    04	01	Len (MSB)	Data	90	00
*/

//********************  5.1 PPR_Reset/PPR_Reset_Offline：重置 Reader，將初始化參數傳給 Reader  ********************
// ----- PPR_Reset Request-----
#define ECC_CMD_PPR_RESET_REQUEST_DATA_LEN  64//Total Length 64 總長度
typedef struct
{
    uint8_t             TMLocationID[10];              //TM Location ID 10 TM 終端機(TM)店號，ASCII(右靠左補 0)
    uint8_t             TMID[2];                        //TM ID 2 TM 終端機(TM)機號，ASCII (右靠左補 0)
    uint8_t             TMTXNDateTime[14];              //TM TXN Date Time 14 TM 終端機(TM)交易日期時間，ASCII (YYYYMMDDhhmmss)
    ECCCmdSerialNumber  TMSerialNumber;                 //TM Serial Number 6 TM 終端機(TM)交易序號，ASCII (右靠左補 0，值須為 0~9) 交易成功時進號，失敗時不進號
    uint8_t             TMAgentNumber[4];               //TM Agent Number 4 TM 終端機(TM)收銀員代號，ASCII (右靠左補 0，值須為 0~9)
    ECCCmdDataTime      TXNDateTime;                    //TXN Date Time 4 TM 交易日期時間 Unsigned and LSB First (UnixDateTime)
    uint8_t             LocationID;                     //Location ID 1 定值 舊場站代碼 (由悠遊卡公司指定)
    uint8_t             NewLocationID[2];               //New Location ID 2 定值 新場站代碼 Unsigned and LSB First (由悠遊卡公司指定)
    uint8_t             ServiceProviderID;              //Service Provider ID 1 定值  服務業者代碼，補 0x00，1bytes 若設定 0x00 後續 Response 會帶出正確值
    uint8_t             NewServiceProviderID[3];        //New Service Provider ID 3 定值 新服務業者代碼，補 0x00，3bytes Unsigned and LSB First 若設定 0x00 後續 Response 會帶出正確值
    uint8_t             PaymentType;                    //小額設定參數 1 TM 小額消費設定旗標  使用於第一類交易
                                                            //固定填 0x08
                                                            //使用於第二類交易
                                                            //固定填 0x73
    uint8_t             OneDayQuota[2];                 //One Day Quota For Micro Payment 2 TM 小額消費日限額額度 Unsigned and LSB First 
                                                         //使用於第一類交易
                                                         //固定填 0x00 0x00
                                                         //使用於第二類交易
                                                         //固定填 0xB8 x0B 即為 3000
    uint8_t             OnceQuota[2];                   //Once Quota For Micro Payment 2 TM小額消費次限額額度 Unsigned and LSB First
                                                           //使用於第一類交易
                                                           //固定填 0x00 0x00
                                                           //使用於第二類交易
                                                           //固定填 0xE8 x03 即為 1000
    uint8_t             SAMSlotControlFlag;             //SAM Slot Control Flag 1 TM SAM 卡位置控制旗標
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
    uint8_t             RFU[11];                        //RFU(Reserved For Use) 11 TM 保留，補 0x00，11bytes
    
}ECCCmdPPRResetRequestData; 



typedef struct
{
    uint8_t CLA;
    uint8_t INS;
    uint8_t P1;
    uint8_t P2;
    uint8_t Lc;
    ECCCmdPPRResetRequestData data;//16 bytes
    uint8_t Le;    
}ECCCmdPPRResetRequest;  

typedef struct
{
    uint8_t                 header;
    uint8_t                 cmd1;
    uint8_t                 cmd2;
    uint16_t                len;
    ECCCmdPPRResetRequest    body;
    uint8_t                 LRC;
    uint8_t                 tail1;  
    uint8_t                 tail2;  
}ECCCmdPPRResetRequestPack;

// ----- PPR_Reset Response-----
#define ECC_CMD_PPR_RESET_RESPONSE_DATA_LEN 250// Total Length 250 總長度
typedef struct
{
    #if(0)
    uint8_t contain[ECC_CMD_PPR_RESET_RESPONSE_DATA_LEN];
    #else

    uint8_t         SpecVersionNumber;                  //Spec. Version Number 1 Reader Host 識別版號 定值 0x01
    uint8_t         ReaderID[4];                        //Reader ID 4 Reader 讀卡機編號
    uint8_t         ReaderFWVersion[6];  //index 5      //Reader FW Version 6 Reader 讀卡機韌體版本 
    ////--(以下 6 個欄位來自舊 SAM Card)
    uint8_t         SAMID[8];         //index 11        //SAM ID 8 SAM SAM 編號 (P2=0x01 時，補 0x00) Unsigned and MSB First
    uint8_t         SAMSN[4];         //index 19        //SAM SN 4 SAM SAM 使用次數 (P2=0x01 時，補 0x00) Unsigned and MSB First
    uint8_t         SAMCRN[8];        //index 23        //SAM CRN 8 SAM SAM 產生之 Random Number (P2=0x01 時，補 0x00)
    uint8_t         DeviceID[4];       //index 31       //Device ID 4 SAM 舊設備編號 Unsigned and LSB First
    uint8_t         SAMKeyVersion;     //index 35       //SAM Key Version 1 SAM SAM 金鑰版本  
    uint8_t         STAC[8];           //index 36       //S-TAC 8 SAM SAM 認證碼 (P2=0x01 時，補 0x00)
    ////--(以下 17 個欄位來自新 SAM Card)
    uint8_t         SAMVersionNumber;    //44               //SAM Version Number 1 SAM SAM 版本
    uint8_t         SID[8];              //45               //SID 8 SAM SAM ID Unsigned and MSB First
    uint8_t         SAMUsageControl[3];   //53              //SAM Usage Control 3 SAM SAM 控制參數
    uint8_t         SAMAdminKVN;          //56              //SAM Admin KVN 1 SAM SAM Admin Key Version Number
    uint8_t         SAMIssuerKVN;         //57              //SAM Issuer KVN 1 SAM SAM Issuer Key Version Number
    uint8_t         AuthorizedCreditLimit[3]; //58          //Authorized Credit Limit 3 SAM 加值額度預設值(ACL) Unsigned and LSB First
    uint8_t         SingleCreditTXNAMTLimit[3];  //61       //Single Credit TXN AMT Limit 3 SAM 加值交易金額限額 Unsigned and LSB First
    uint8_t         AuthorizedCreditBalance[3];  //64       //Authorized Credit Balance 3 SAM 加值授權額度(ACB) Unsigned and LSB First
    uint8_t         AuthorizedCreditCumulative[3];//67      //Authorized Credit Cumulative 3 SAM 加值累積已用額度(ACC) Unsigned and LSB First    
    uint8_t         AuthorizedCancelCreditCumulative[3];//70//Authorized Cancel Credit Cumulative 3 SAM 取消加值累積已用額度(ACCC) Unsigned and LSB First
    uint8_t         NewDeviceID[6];       //73              //New Device ID 6 SAM 新設備編號 Unsigned and LSB First
    uint8_t         TagListTable[40];     //79              //Tag List Table 40 SAM Tag 控制參數 
    uint8_t         SAMIssuerSpecificData[32]; //119         //SAM Issuer Specific Data 32 SAM SAM 自訂資料
    uint8_t         STC[4];              //151               //STC 4 SAM SAM TXN Counter (P2=0x01 時，補 0x00) Unsigned and MSB First
    uint8_t         RSAM[8];             //155               //RSAM 8 SAM SAM 產生之 Random Number (P2=0x01 時，補 0x00)
    uint8_t         RHOST[8];            //163               //RHOST 8 Reader Host 之 Random Number (Reader 產生) (P2=0x01 時，補 0x00)
    uint8_t         SATOKEN[16];       //171                 //SATOKEN 16 SAM SAM Authentication Token (P2=0x01 時，補 0x00)
    ////--(以下 5 個欄位為 PPR_SignOn 設定參數之值，適用於有 SignOn 之設備)
    uint8_t         Flag01;            //187                 //CPD Read Flag／One Day Quota Write For Micro Payment／SAM SignOnControl Flag／Check EV Flag For Mifare Only／Merchant Limit Use For Micro Payment／ 1 Reader
                                                                    //Bit 0~1：二代 CPD 讀取與驗證設定
                                                                        //xxxxxx00b：不讀取回 Host 且 Reader 不驗證
                                                                        //xxxxxx01b：讀取回 Host 且 Reader 不驗證
                                                                        //xxxxxx10b：不讀取回 Host 且 Reader 要驗證
                                                                        //xxxxxx11b：讀取回 Host 且 Reader 要驗證        
                                                                    //Bit 2~3：小額消費日限額寫入
                                                                        //xxxx00xxb：T=Mifare 與 T=CPU 都不寫入
                                                                        //xxxx01xxb：T=Mifare 寫入、T=CPU 不寫入
                                                                        //xxxx10xxb：T=Mifare 不寫入、T=CPU 寫入
                                                                        //xxxx11xxb：T=Mifare 與 T=CPU 都寫入
                                                                    //Bit 4~5：SAM 卡 SignOn 控制旗標
                                                                        //xx00xxxxb：兩張 SAM 卡都不需 SignOn(離線)
                                                                        //xx01xxxxb：只 SignOn 新 SAM 卡(淘汰 Mifare時)
                                                                        //xx10xxxxb：只 SignOn 舊 SAM 卡
                                                                        //xx11xxxxb：兩張 SAM 卡都要 SignOn
                                                                    //Bit 6：檢查餘額旗標
                                                                        //x0xxxxxxb：檢查餘額
                                                                        //x1xxxxxxb：不檢查餘額
                                                                    //Bit 7：小額消費通路限制使用旗標
                                                                        //0xxxxxxxb：限制使用
                                                                        //1xxxxxxxb：不限制使用
    uint8_t         Flag02;           //188                  //One Day Quota Flag For Micro Payment／Once Quota Flag For Micro Payment／Check Debit Flag／RFU(Reserved For Use) 1 Reader
                                                                    //Bit 0~1：小額消費日限額旗標
                                                                        //xxxxxx00b：不檢查、不累計日限額
                                                                        //xxxxxx01b：不檢查、累計日限額
                                                                        //xxxxxx10b：檢查、不累計日限額
                                                                        //xxxxxx11b：檢查、累計日限額
                                                                    //Bit 2：小額消費次限額旗標
                                                                        //xxxxx0xxb：不限制次限額
                                                                        //xxxxx1xxb：限制次限額
                                                                    //Bit 3：扣值交易合法驗證旗標
                                                                        //xxxx0xxxb：不限制扣值交易合法驗證金額
                                                                        //xxxx1xxxb：限制扣值交易合法驗證金額
                                                                    //Bit 4~7：RFU(Reserved For Use)
    uint8_t         OneDayQuotaForMicroPayment[2]; //189     //One Day Quota For Micro Payment 2 Reader 小額消費日限額額度 Unsigned and LSB First 註：退貨交易會歸還日限額
    uint8_t         OnceQuotaForMicroPayment[2];   //191     //Once Quota For Micro Payment 2 Reader 小額消費次限額額度 Unsigned and LSB First 
    uint8_t         CheckDebitValue[2];    //193             //Check Debit Value 2 Reader 扣值交易合法驗證金額 Unsigned and LSB First
    ////---(以下 3 個欄位用於舊的額度控管)
    uint8_t         AddQuotaFlag;          //195             //Add Quota Flag 1 Reader 加值額度控管旗標  0x00 不檢查額度  0x01 檢查額度
    uint8_t         AddQuota[3];                        //Add Quota 3 Reader 加值額度 Unsigned and LSB First
    uint8_t         TheRemainderOfAddQuota[3]; //199         //The Remainder of Add Quota 3 Reader 剩餘加值額度 Unsigned and LSB First
    uint8_t         CancelCreditQuota[3];  //202             //Cancel Credit Quota 3 Reader 取消加值累計額度 Unsigned and LSB First
    ////---(以下 1 個欄位用於舊 SAM Card)
    uint8_t         deMACParameter[8];    //205              //deMAC Parameter 8 Reader 某些超商 Dongle deMAC 需用到此參數  (P2=0x01 時，補 0x00)
    ECCCmdDataTime  LastTXNDateTime;      //213              //Last TXN Date Time 4 Reader 最後一次 Mutual Authentication 成功時間 (UnixDateTime)
    ////---(以下 6 個欄位用於新 SAM Card SignOn 補 confirm 之用，含成功及失敗) (P2=0x01 時，補 0x00)
    uint8_t         PreviousNewDeviceID[6];  //217           //Previous New Device ID 6 Reader 新設備編號 Unsigned and LSB First
    uint8_t         PreviousSTC[4];     //223                //Previous STC 4 Reader SAM TXN Counter Unsigned and MSB First
    ECCCmdDataTime  PreviousTXNDateTime;  //227              //Previous TXN Date Time 4 Reader 交易日期時間 Unsigned and LSB First (UnixDateTime)
    uint8_t         PreviousCreditBalanceChangeFlag; //231   //Previous Credit Balance Change Flag 1 Reader 加值授權額度(ACB)變更旗標  0x00 未變更  0x01 額度有變更
    uint8_t         PreviousConfirmCode[2];  //232           //Previous Confirm Code 2 Reader Status Code 1+ Status Code 2
    uint8_t         PreviousCACrypto[16];   //234            //Previous CACrypto 16 Reader Credit Authorization Cryptogram (額度變更失敗或未變更時，補 0x00)
    #endif
}ECCCmdPPRResetResponseData;

#define ECC_CMD_RESET_SUCCESS_ID         0x9000
//typedef struct
//{
//    ECCCmdPPRResetResponseData          data;
//    ECCCmdResponseStatus                status;
//}ECCCmdPPRResetResponse;


//********************  5.4 PPR_EDCARead：讀取小額扣款相關資訊  ********************
// ----- DCAR_Read Request-----
#define ECC_CMD_DCAR_READ_REQUEST_DATA_LEN  16//Total Length 16     總長度
typedef struct
{
    uint8_t             lcdControlFlag;             //LCD Control Flag 1 TM 用於控制交易完成後之 LCD 顯示
                                                    //0x00：顯示【票卡餘額 XXXX】 (default)
                                                    //x01：顯示【（請勿移動票卡）】
    ECCCmdSerialNumber  tmSerialNumber;             //TM Serial Number 6 TM 終端機(TM)交易序號，ASCII
                                                            //(右靠左補 0，值須為 0~9)
                                                            //交易成功時進號，失敗時不進號
    ECCCmdDataTime      txnDateTime;                //TXN Date Time 4 TM 交易日期時間
                                                            //Unsigned and LSB First (UnixDateTime)
    uint8_t             RFU[5];                     //RFU(Reserved For Use) 5 TM 保留，補 0x00，5bytes  
}ECCCmdDCAReadRequestData; 




typedef struct
{
    uint8_t CLA;
    uint8_t INS;
    uint8_t P1;
    uint8_t P2;
    uint8_t Lc;
    ECCCmdDCAReadRequestData data;//16 bytes
    uint8_t Le;    
}ECCCmdDCAReadRequest;  
typedef struct
{
    uint8_t                 header;
    uint8_t                 cmd1;
    uint8_t                 cmd2;
    uint16_t                len;
    ECCCmdDCAReadRequest    body;
    uint8_t                 LRC;
    uint8_t                 tail1;  
    uint8_t                 tail2;  
}ECCCmdDCAReadRequestPack;

// ----- DCAR_Read Response -----
typedef struct //Total Length 33 總長度
{
    uint8_t             PurseVersionNumber;         //index 61  //Purse Version Number 1 Card 票卡版號
                                                                        //0x00：Mifare／Level1；
                                                                        //(含 PVN=0x02 and RFU<>0x02 時)
                                                                        //0x02：Level2
    uint8_t             TSQN[3];                                //TSQN 3 Card 交易序號 Unsigned and LSB First
    ECCCmdDataTime      TXNDateTime;                //index 65(4)  //TXN Date Time 4 Card 交易日期時間 Unsigned and LSB First (UnixDateTime)
    uint8_t             SubType;                    //index 69(8)  //Sub Type 1 Card 加/扣值方式代碼
    uint8_t             TXNAMT[3];                  //index??(9)   //TXN AMT 3 Card 交易金額 Signed and LSB First
    uint8_t             EV[3];                      //index??(12)  //EV 3 Card 交易後餘額 Signed and LSB First
    uint8_t             ServiceProviderID[3];      //index 76(15)//Service Provider ID 3 Card 服務業者代碼 Unsigned and LSB First 
    uint8_t             LocationID[2];             //index??(18)  //Location ID 2 Card 場站代碼 Unsigned and LSB First
    uint8_t             DeviceID[6];               //index ??(20) //Device ID 6 Card 設備編號 Unsigned and LSB First
    uint8_t             RFU;                                 //RFU 1 Card RFU
    uint8_t             EVValue[3];                                  //EV 3 Card 交易後餘額(Calculate by CPU Card) Signed and LSB First
                                                                        //(Purse Version Number=0x02 時才會有
                                                                        //值，其他補 0x00)
    uint8_t             TSQNValue[3];                                //TSQN 3 Card 交易序號(Calculate by CPU Card) Unsigned and LSB First
                                                                        //(Purse Version Number=0x02 時才會有
                                                                        //值，其他補 0x00)
    
}LastCreditTXNLogData;


#define ECC_CMD_DCAR_READ_RESPONSE_SUCCESS_DATA_LEN 160// Total Length 160 總長度
typedef struct
{
    uint8_t             PurseVersionNumber;                     //Purse Version Number 1 Card 票卡版號
                                                                    //0x00：Mifare／Level1；
                                                                    //0x02：Level2
    uint8_t             PurseUsageControl;          // index 1      //Purse Usage Control 1 Card 票卡功能設定 (0：否；1：是)
                                                                    //bit0：是否 Activated；
                                                                    //bit1：是否 Blocked；
                                                                    //bit2：是否 Refunded；
                                                                    //bit3：是否允許 Autoload；
                                                                    //bit4：是否允許 Credit；
                                                                    //bit5~bit7：保留
    uint8_t             SingleAutoLoadTransactionAmount[3]; //index 2   //Single Auto-Load Transaction Amount 3 Card 自動加值金額預設值
                                                                //Unsigned and LSB First
//--(以下 2 欄位用於 CPU Level2 Card)：Purse Version Number=0x00 時，以下欄位補 0x00--
    ECCCmdPID           PID;                        //index 5            //PID(Purse ID) 8 Card 外觀卡號
    uint8_t             SubAreaCode[2];             //index 13             //Sub Area Code 2 Card 子區碼(郵遞區號)Unsigned and LSB First
    ECCCmdDataTime      PurseExpDate;               //index 15            //Purse Exp. Date 4 Card 票卡到期日期 Unsigned and LSB First(UnixDateTime)
    uint8_t             PurseBalanceBeforeTXN[3];   // index 19 //Purse Balance Before TXN 3 Card 交易前票卡錢包餘額 Signed and LSB First
    uint8_t             TXNSNBeforeTXN[3];          //index 22            //TXN SN. Before TXN 3 Card 交易前票卡交易序號 Unsigned and LSB First
    uint8_t             CardType;                   // index 25 //Card Type 1 Card 卡別
    uint8_t             PersonalProfile;            // index 26 //Personal Profile 1 Card 身份別
    ECCCmdDataTime      ProfileExpDate;                         //Profile Exp. Date 4 Card 身份到期日 Unsigned and LSB First(UnixDateTime)
    uint8_t             AreaCode;                               //Area Code 1 Card 區碼
    uint8_t             CardPhysicalID[7];          // index 32 //Card Physical ID 7 Card Mifare 卡號
    uint8_t             CardPhysicalIDLength;       // index 39 //Card Physical ID Length 1 Reader Mifare 卡號長度
                                                                    //0x04：Mifare 卡號為 4 bytes
                                                                    //0x07：Mifare 卡號為 7 bytes
    uint8_t             DeviceID[4];                // index 40 //Device ID 4 Reader 舊設備編號 Unsigned and LSB First
    uint8_t             NewDeviceID[6];             // index 44 //New Device ID 6 Reader 新設備編號 Unsigned and LSB First
    uint8_t             ServiceProviderID;          // index 50 //Service Provider ID 1 Reader 舊服務業者代碼
    uint8_t             NewServiceProviderID[3];    // index 51 //New Service Provider ID 3 Reader 新服務業者代碼 Unsigned and LSB First
    uint8_t             LocationID;                 // index 54 //Location ID 1 Reader 舊場站代碼
    uint8_t             NewLocationID[2];           // index 55 //New Location ID 2 Reader 新場站代碼 Unsigned and LSB First
    uint8_t             IssuerCode;                 // index 57 //Issuer Code 1 Card 發卡公司
    uint8_t             BankCode;                   //index 58  //Bank Code 1 Card 銀行代碼
    uint8_t             LoyaltyCounter[2];          //index 59  //Loyalty Counter 2 Card 忠誠點(2bytes) Unsigned and LSB First
    LastCreditTXNLogData LastCreditTXNLog;           //index 61(a)//LastCreditTXNLog[33]; //Last Credit TXN Log 33 Card 票卡最後一筆加值記錄(註 1)
    uint8_t             AutoloadCounter;            //index 94(a)//Autoload Counter 1 Card 離線 Autoload 次數
    uint8_t             AutoloadDate[2];            //index 95(a)//Autoload Date 2 Card 離線 Autoload 日期
//--(以下 2 欄位保留予小額消費－票卡累計日限額使用) 不需累計日限額時，補 0x00 (是否需累計日限額由 PPR_Reset_Offline 參數決定)
    uint8_t             CardOneDayQuota[3];                     //Card One Day Quota 3 Card 交易前票卡累計日限額額度
    uint8_t             CardOneDayQuotaDate[2];                 //Card One Day Quota Date 2 Card 交易前票卡累計日限額日期
//--Usage TXN Record for Transfer(URT)-轉乘交易紀錄
    uint8_t             TSQNofURT[3];                           //TSQN of URT 3 Card 轉乘交易序號 Unsigned and LSB First
    uint8_t             TXNDateTimeofURT[4];                    //TXN Date Time of URT 4 Card 轉乘交易日期時間 Unsigned and LSB First (UnixDateTime)
    uint8_t             TXNTypeofURT;                           //TXN Type of URT 1 Card 轉乘交易方式
                                                                    //- 0x00: 刷進，無優惠值 (原價)
                                                                    //- 0x01: 刷進，有優惠值 (有打折)
                                                                    //- 0x02: 刷進，有點數或搭乘優惠(有點數抵減或轉乘優惠)
                                                                    //- 0x03: 刷進，定期票 (使用旅遊票或月票)
                                                                    //- 0x10: 刷出，無優惠值 (原價)
                                                                    //- 0x11: 刷出，有優惠值 (有打折)
                                                                    //- 0x12: 刷出，有點數或搭乘優惠(有點數抵減或轉乘優惠)
                                                                    //- 0x13: 刷出，定期票 (使用旅遊票或月票)
    uint8_t             TXNAMTofURT[3];                         //TXN AMT of URT 3 Card 轉乘交易金額 Signed and LSB First
    uint8_t             EVofURT[3];                             //EV of URT 3 Card 轉乘交易後餘額 Signed and LSB First
    uint8_t             TransferGroupCode[2];                   //Transfer Group Code 2 Card 轉乘群組代碼
                                                                    //MSB byte：最後搭乘運具之轉乘群組
                                                                    //LSB byte：最後搭乘運具之前的轉乘群組
    uint8_t             LocationIDofURT[2];                     //Location ID of URT 2 Card 場站代碼(2bytes) Unsigned and LSB First
    uint8_t             DeviceIDofURT[6];                       //Device ID of URT 6 Card 設備編號(6bytes) Unsigned and LSB First
    uint8_t             PersonID[16];                           //身分證號碼 16 Card 身分證號碼 Unsigned and LSB First
    uint8_t             TheRemainderofAddQuota[3];              //The Remainder of Add Quota 3 Reader 剩餘加值額度 Unsigned and LSB First
    uint8_t             RFU[15];                                //RFU 15 Card 預留
}ECCCmdDCAReadResponseSuccessData; //0x9000

//0x640E(餘額異常) or 0x6418(通路限制)
#define ECC_CMD_DCAR_READ_RESPONSE_ERROR_1_DATA_LEN  120//Total Length 120 總長度
typedef struct
{
    uint8_t             PurseVersionNumber;                     //Purse Version Number 1 Card 票卡版號
                                                                    //0x00：Mifare／Level1；
                                                                    //0x02：Level2
    uint8_t             PurseUsageControl;                      //Purse Usage Control 1 Card 票卡功能設定 (0：否；1：是)
                                                                    //bit0：是否 Activated；
                                                                    //bit1：是否 Blocked；
                                                                    //bit2：是否 Refunded；
                                                                    //bit3：是否允許 Autoload；
                                                                    //bit4：是否允許 Credit；
                                                                    //bit5~bit7：保留
    uint8_t             SingleAutoLoadTransactionAmount[3];     //Single Auto-Load Transaction Amount 3 Card 自動加值金額預設值 Unsigned and LSB First  
//--(以下 3 欄位用於 CPU Level2 Card)：Purse Version Number=0x00 時，以下欄位補 0x00
    ECCCmdPID           PID;              // index 5            //PID(Purse ID) 8 Card 外觀卡號
    uint8_t             CPUIssuerkeyKVN;                        //CPU Issuer key KVN 1 Card 票卡變更purse 資料所需 key 的 Key Version Number
    uint8_t             SubAreaCode[2];                         //Sub Area Code 2 Card 子區碼(郵遞區號) Unsigned and LSB First
    ECCCmdDataTime      PurseExpDate;                           //Purse Exp. Date 4 Card 票卡到期日期 Unsigned and LSB First (UnixDateTime)
    uint8_t             PurseBalance[3];  //index 20            //Purse Balance 3 Card 票卡錢包餘額 Signed and LSB First
    uint8_t             TTXNSN[3];                              //XN SN. 3 Card 票卡交易序號 Unsigned and LSB First
    uint8_t             CardType;                               //Card Type 1 Card 卡別 
    uint8_t             PersonalProfile;   // index 27          //Personal Profile 1 Card 身份別
    ECCCmdDataTime      ProfileExpDate;                         //Profile Exp. Date 4 Card 身份到期日 Unsigned and LSB First(UnixDateTime)
    uint8_t             AreaCode;                               //Area Code 1 Card 區碼
    uint8_t             CardPhysicalID[7];   //index 33         //Card Physical ID 7 Card Mifare 卡號
    uint8_t             CardPhysicalIDLength; //index 40        //Card Physical ID Length 1 Reader Mifare 卡號長度
                                                                    //0x04 Mifare 卡號為 4 bytes
                                                                    //0x07 Mifare 卡號為 7 bytes
    uint8_t             DeviceID[4];        //index 41          //Device ID 4 Reader 舊設備編號 Unsigned and LSB First
    uint8_t             NewDeviceID[6];     //index 45          //New Device ID 6 Reader 新設備編號 Unsigned and LSB First
    uint8_t             ServiceProviderID;  //index 51          //Service Provider ID 1 Reader 舊服務業者代碼
    uint8_t             NewServiceProviderID[3];//index 52      //New Service Provider ID 3 Reader 新服務業者代碼 Unsigned and LSB First
    uint8_t             LocationID;         //index 55          //Location ID 1 Reader 舊場站代碼
    uint8_t             NewLocationID[2];   //index 56          //New Location ID 2 Reader 新場站代碼 Unsigned and LSB First
    uint8_t             Deposit[3];                             //Deposit 3 Card 押金 Unsigned and LSB First
    uint8_t             IssuerCode;         //index 61          //Issuer Code 1 Card 發卡公司
    uint8_t             BankCode;                               //Bank Code 1 Card 銀行代碼
    uint8_t             LoyaltyCounter[2];                      //Loyalty Counter 2 Card 忠誠點(2bytes) Unsigned and LSB First
    LastCreditTXNLogData   LastCreditTXNLog;                    //Last Credit TXN Log 33 Card 票卡最後一筆加值記錄(註 1)
    uint8_t             MsgType;                                //Msg Type 1 Reader/TM 0x00 default
    uint8_t             Subtype;                                //Subtype 1 Reader/TM 0x00 default
    uint8_t             AnotherEV[3];    //index 100            //Another EV 3 Card 票卡另一個錢包餘額 Signed and LSB First
    uint8_t             MifareSettingParameter;    //index 103  //Mifare Setting Parameter 1 Card 票卡地區認證旗標
    uint8_t             CPUSettingParameter;  //index 104       //CPU Setting Parameter 1 Card 票卡是否限制通路使用旗標
    uint8_t             RFU[15];                                //RFU(Reserved For Use) 15 Card 保留，補 0x00，15bytes    
}ECCCmdDCAReadResponseError1Data; 

//0x6103(CPD檢查異常) //跟 LockCard response 相同
#define ECC_CMD_DCAR_READ_RESPONSE_ERROR_2_DATA_LEN  40//Total Length 40 總長度
typedef struct
{
    uint8_t             PurseVersionNumber;                     //Purse Version Number 1 Card 票卡版號
                                                                        //0x00：Mifare／Level1；
                                                                        //0x02：Level2  
//--(以下 2 欄位用於 CPU Level2 Card)：Purse Version Number=0x00 時，以下欄位補 0x00
    ECCCmdPID           PID;                                    //PID(Purse ID) 8 Card 外觀卡號
    uint8_t             CTC[3];                                 //CTC 3 Card Card TXN Counter Unsigned and MSB First
    uint8_t             CardType;                               //Card Type 1 Card 卡別
    uint8_t             PersonalProfile;      //index 13        //Personal Profile 1 Card 身份別
    uint8_t             CardPhysicalID[7];                      //Card Physical ID 7 Card Mifare 卡號
    uint8_t             CardPhysicalIDLength;                   //Card Physical ID Length 1 Reader Mifare 卡號長度
                                                                        //0x04：Mifare 卡號為 4 bytes
                                                                        //0x07：Mifare 卡號為 7 bytes
    uint8_t             DeviceID[4];        //index 22          //Device ID 4 Reader 舊設備編號 Unsigned and LSB First
    uint8_t             NewDeviceID[6];                         //New Device ID 6 Reader 新設備編號 Unsigned and LSB First
    uint8_t             ServiceProviderID;  //index 32          //Service Provider ID 1 Reader 舊服務業者代碼 
    uint8_t             NewServiceProviderID[3];                //New Service Provider ID 3 Reader 新服務業者代碼 Unsigned and LSB First
    uint8_t             LocationID;                             //Location ID 1 Reader 舊場站代碼
    uint8_t             NewLocationID[2];                       //New Location ID 2 Reader 新場站代碼 Unsigned and LSB First
    uint8_t             IssuerCode;                             //Issuer Code 1 Card 發卡公司
}ECCCmdDCAReadResponseError2Data; 

#define ECC_CMD_READ_SUCCESS_ID         0x9000
//typedef struct
//{
//    ECCCmdDCAReadResponseSuccessData    data;
//    ECCCmdResponseStatus                status;
//}ECCCmdDCAReadResponseSuccess; //0x9000


//0x640E(餘額異常) or 0x6418(通路限制)
#define ECC_CMD_READ_ERROR_1_ID_1       0x640E
#define ECC_CMD_READ_ERROR_1_ID_2       0x6418
//typedef struct
//{
//    ECCCmdDCAReadResponseError1Data     data;
//    ECCCmdResponseStatus                status;
//}ECCCmdDCAReadResponseError1;

//0x6103(CPD檢查異常)
#define ECC_CMD_READ_ERROR_2_ID_1       0x6103
//typedef struct
//{
//    ECCCmdDCAReadResponseError2Data     data;
//    ECCCmdResponseStatus                status;  
//}ECCCmdDCAReadResponseError2;

//********************  5.5 PPR_EDCADeduct：小額扣款交易(含選擇是否自動加值)  ********************
// ----- EDCA_Deduct Request-----
#define ECC_CMD_EDCA_DEDUCT_REQUEST_DATA_LEN  64//Total Length 64     總長度
typedef struct
{
    uint8_t             MsgType;                                //Msg Type 1 TM 0x01 扣款
    uint8_t             NewSubtype;                             //New_Subtype 1 TM 0x00 default
    uint8_t             TMLocationID[10];                       //TM Location ID 10 TM 終端機(TM)店號，ASCII (右靠左補 0)
    uint8_t             TMID[2];                                //TM ID 2 TM 終端機(TM)機號，ASCII (右靠左補 0)
    uint8_t             TMTXNDateTime[14];                      //TM TXN Date Time 14 TM 終端機(TM)交易日期時間 (YYYYMMDDhhmmss)
    ECCCmdSerialNumber  TMSerialNumber;                         //TM Serial Number 6 TM 終端機(TM)交易序號，ASCII (右靠左補 0，值須為 0~9)交易成功時進號，失敗時不進號
    uint8_t             TMAgentNumber[4];                       //TM Agent Number 4 TM 終端機(TM)收銀員代號，ASCII (右靠左補 0，值須為 0~9)
    ECCCmdDataTime      TXNDateTime;                            //TXN Date Time 4 TM 交易日期時間 Unsigned and LSB First (UnixDateTime)
    uint8_t             TXNAMT[3];                              //TXN AMT 3 TM 交易金額 Signed and LSB First (使用特種票時補 0x00)
    uint8_t             AutoLoad;                               //Auto-Load 1 TM 是否進行自動加值
                                                                        //- 0x00：否
                                                                        //- 0x01：是
    uint8_t             TXNType;                                //TXN Type 1 TM 交易方式 
                                                                        //- 0x20：小額扣款
    uint8_t             TransferGroupCode;                      //Transfer Group Code 1 TM 本運具之轉乘群組代碼
                                                                        //0x06：小額扣款型進出站停車場
                                                                        //0x00：其它無需寫入轉乘資訊
    uint8_t             RFU[15];                                //RFU(Reserved For Use) 15 TM 保留，補 0x00，15 bytes
    uint8_t             LCDControlFlag;                         //LCD Control Flag 1 TM 用於控制交易完成後之 LCD 顯示
                                                                        //0x00：顯示【交易完成 請取卡】(default)
                                                                        //0x01：顯示【（請勿移動票卡）】
}ECCCmdEDCADeductRequestData; 



typedef struct
{
    uint8_t CLA;
    uint8_t INS;
    uint8_t P1;
    uint8_t P2;
    uint8_t Lc;
    ECCCmdEDCADeductRequestData data;
    uint8_t Le;    
}ECCCmdEDCADeductRequest; 

typedef struct
{
    uint8_t                 header;
    uint8_t                 cmd1;
    uint8_t                 cmd2;
    uint16_t                len;
    ECCCmdEDCADeductRequest    body;
    uint8_t                 LRC;
    uint8_t                 tail1;  
    uint8_t                 tail2;  
}ECCCmdEDCADeductPack;

// ----- EDCA_Deduct response-----
#define ECC_CMD_EDCA_DEDUCT_RESPONSE_DATA_LEN  122//Total Length 122 總長度
typedef struct
{
    //--(以下 4 欄位用於 CPU Level2 Card)：Purse Version Number=0x00 時，以下欄位補 0x00
    uint8_t             SignatureKeyKVN ;                       //Signature key KVN 1 Card 票卡進行簽章所需key的 Key Version Number
    uint8_t             SID[8];                                 //SID 8 SAM SAM ID(用於計算 HCrypto)
    uint8_t             HashType;            //index 9                   //Hash Type 1 Reader Hash 方式(用於計算 HCrypto)
    uint8_t             HostAdminKeyKVN;                        //Host Admin Key KVN 1 Reader Host Admin Key KVN (用於計算HCrypto)
    // *** 自動加值成功後回傳以下內容 ***
    uint8_t             AutoLoadMsgType;                        //Msg Type 1 Reader 0x02 加值
    uint8_t             AutoLoadSubType;                        //Sub Type 1 Reader 0x40 自動加值
    //--(以下 4 欄位用於 CPU Level2 Card)：Purse Version Number=0x00 時，以下欄位補 0x00
    uint8_t             AutoLoadCTC[3];                         //CTC 3 Card Card TXN Counter Unsigned and MSB First
    uint8_t             AutoLoadTM;          //index 16                   //TM(TXN Mode) 1 Reader 交易模式種類，定值 0x00
    uint8_t             AutoLoadTQ;                             //TQ(TXN Qualifier) 1 Reader 交易屬性設定值 自動加值=13h
    uint8_t             AutoLoadSIGN[16];    //index 18         //SIGN 16 SAM Signature
    uint8_t             AutoLoadTSQN[3];     //index 34         //TSQN 3 Reader 交易序號 Unsigned and LSB First
    uint8_t             AutoLoadTXNAMT[3];                      //TXN AMT 3 TM 交易金額 Signed and LSB First
    uint8_t             AutoLoadPurseBalance[3];  //index 40    //Purse Balance 3 Reader 票卡餘額 Signed and LSB First
    uint8_t             AutoLoadMAC[16];        //index 43      //MAC／HCrypto 16 Reader  Purse Version Number<>0x02 時， 交易押碼值(MAC 10bytes)＋RFU( 6bytes)
                                                                                            //Purse Version Number=0x02 時，Host Cryptogram(16bytes)
    ECCCmdDataTime      AutoLoadTXNDateTime;    //index 59      //TXN Date Time 4 Reader 交易日期時間(交易時間以本欄位為準) Unsigned and LSB First (UnixDateTime)(Retry 時本欄位值會與 input 不同)
    uint8_t             AutoLoadConfirmCode[2];                 //Confirm Code 2 Reader Status Code 1+ Status Code 2
    // *** 扣款成功後回傳以下內容 ***
    uint8_t             MsgType;             //index 65         //Msg Type 1 Reader 0x01 扣款
    uint8_t             SubType;                                //Sub Type 1 Reader Msg Type=0x01 時，Sub Type=Personal Profile
    uint8_t             TransferGroupCode;                      //Transfer Group Code 1 Reader 轉乘群組代碼
    uint8_t             NewTransferGroupCode[2];                //New Transfer Group Code 2 Reader 新轉乘群組代碼
    //--(以下 4 欄位用於 CPU Level2 Card)：Purse Version Number=0x00 時，以下欄位補 0x00
    uint8_t             CTC[3];             //index 71                    //CTC 3 Card Card TXN Counter Unsigned and MSB First
    uint8_t             TM;                 //index 73          //TM(TXN Mode) 1 Reader 交易模式種類，定值 0x00
    uint8_t             TQ;                 //index 74          //TQ(TXN Qualifier) 1 Reader 交易屬性設定值
                                                                    // 進站扣款(不代墊)=08h
                                                                    // 進站扣款(代墊)=0Ah
                                                                    // 出站扣款(不代墊)=09h
                                                                    // 出站扣款(代墊)=0Bh
                                                                    // 特種票出站(代墊)=23h
    uint8_t             SIGN[16];           //index 75          //SIGN 16 SAM Signature
    uint8_t             TSQN[3];            //index 91          //TSQN 3 Reader 交易序號 Unsigned and LSB First
    uint8_t             TXNAMT[3];          //index 94          //TXN AMT 3 TM 交易金額 Signed and LSB First
    uint8_t             PurseBalance[3];    //index 97          //Purse Balance 3 Reader 票卡餘額 Signed and LSB First
    uint8_t             MAC[16];            //index 100         //MAC／HCrypto 16 Reader  Purse Version Number<>0x02 時， 交易押碼值(MAC 10bytes)＋ RFU( 6bytes)
                                                                    //Purse Version Number=0x02 時， Host Cryptogram(16bytes)
    ECCCmdDataTime      TXNDateTime;        //index 116                    //XN Date Time 4 Reader 交易日期時間(交易時間以本欄位為準) Unsigned and LSB First (UnixDateTime) (Retry 時本欄位值會與 input 不同)
    uint8_t             ConfirmCode[2];                            //Confirm Code 2 Reader Status Code 1+ Status Code 2
}ECCCmdEDCADeductResponseData; 

#define ECC_CMD_EDCA_DEDUCT_SUCCESS_ID         0x9000
#define ECC_CMD_EDCA_DEDUCT_ERROR_ID_1         0x6088
#define ECC_CMD_EDCA_DEDUCT_ERROR_ID_2         0x6424
//2018.08.10 add 
#define ECC_CMD_EDCA_DEDUCT_ERROR_ID_3         0x6402
//typedef struct
//{
//    ECCCmdEDCADeductResponseData        data;
//    ECCCmdResponseStatus                status;
//}ECCCmdEDCADeductResponse; //0x9000


//********************  5.6 PPR_LockCard：當發現為禁用名單票卡，需將票卡鎖卡  ********************
// ----- LockCard Request-----
#define ECC_CMD_LOCK_CARD_REQUEST_DATA_LEN  14//Total Length 14     總長度
typedef struct
{
    uint8_t             MsgType;                                //Msg Type 1 API 0x22 鎖卡
    uint8_t             Subtype;                                //Subtype 1 API 0x00 default
    uint8_t             CardPhysicalID[7];                      //Card Physical ID 7 API Mifare 卡號
    ECCCmdDataTime      TXNDateTime;                            //TXN Date Time 4 API 交易日期時間 Unsigned and LSB First (UnixDateTime)
    uint8_t             BlockingReason;                         //Blocking Reason 1 API 鎖卡原因 定值 0x01
}ECCCmdLockCardRequestData; 



typedef struct
{
    uint8_t CLA;
    uint8_t INS;
    uint8_t P1;
    uint8_t P2;
    uint8_t Lc;
    ECCCmdLockCardRequestData data;//16 bytes
    uint8_t Le;    
}ECCCmdLockCardRequest;  

typedef struct
{
    uint8_t                 header;
    uint8_t                 cmd1;
    uint8_t                 cmd2;
    uint16_t                len;
    ECCCmdLockCardRequest    body;
    uint8_t                 LRC;
    uint8_t                 tail1;  
    uint8_t                 tail2;  
}ECCCmdLockCardPack;

// ----- LockCard response-----
#define ECC_CMD_LOCK_CARD_RESPONSE_DATA_LEN  40//Total Length 40 總長度
typedef struct
{
    uint8_t             PurseVersionNumber;                     //Purse Version Number 1 Card 票卡版號
                                                                        //0x00：Mifare；
                                                                        //0x01：Level1；
                                                                        //0x02：Level2
//--(以下 2 欄位用於 CPU Card)：Purse Version Number=0x00 時，以下欄位補 0x00
    ECCCmdPID           PID;                   //index 1        //PID(Purse ID) 8 Card 外觀卡號
    uint8_t             CTC[3];                //index 9        //CTC 3 Card Card TXN Counter Unsigned and MSB First
    uint8_t             CardType;                               //Card Type 1 Card 卡別
    uint8_t             PersonalProfile;       //index 13       //Personal Profile 1 Card 身份別
    uint8_t             CardPhysicalID[7];     //index 14       //Card Physical ID 7 Card Mifare 卡號
    uint8_t             CardPhysicalIDLength;  //index 21       //Card Physical ID Length 1 Reader Mifare 卡號長度
                                                                        //0x04 Mifare 卡號為 4 bytes
                                                                        //0x07 Mifare 卡號為 7 bytes
    uint8_t             DeviceID[4];           //index 22       //Device ID 4 Reader 舊設備編號 Unsigned and LSB First
    uint8_t             NewDeviceID[6];        //index 26       //New Device ID 6 Reader 新設備編號 Unsigned and LSB First
    uint8_t             ServiceProviderID ;     //index 32      //Service Provider ID 1 Reader 服務業者代碼 
    uint8_t             NewServiceProviderID[3]; //index 33     //New Service Provider ID 3 Reader 服務業者代碼 Unsigned and LSB First 
    uint8_t             LocationID;             //index 36      //Location ID 1 Reader 舊場站代碼
    uint8_t             NewLocationID[2];        //index 37               //New Location ID 2 Reader 新場站代碼 Unsigned and LSB First
    uint8_t             IssuerCode;             //index 39      //Issuer Code 1 Card 發卡公司
}ECCCmdLockCardResponseData; 

#define ECC_CMD_LOCK_CARD_SUCCESS_ID         0x9000
//typedef struct
//{
//    ECCCmdLockCardResponseData          data;
//    ECCCmdResponseStatus                status;
//}ECCCmdLockCardResponse; //0x9000

//********************  5.3 PPR_SignOnQuery：查詢端末開機是否已認證成功/查詢端末開機參數設定  ********************
// ----- SignOnQuery Request-----
typedef struct
{
    uint8_t CLA;
    uint8_t INS;
    uint8_t P1;
    uint8_t P2;
    //uint8_t Lc;  沒有lc
    uint8_t Le;    
}ECCCmdPPRSignOnQueryRequest;  

typedef struct
{
    uint8_t                 header;
    uint8_t                 cmd1;
    uint8_t                 cmd2;
    uint16_t                len;
    ECCCmdPPRSignOnQueryRequest    body;
    uint8_t                 LRC;
    uint8_t                 tail1;  
    uint8_t                 tail2;  
}ECCCmdPPRSignOnQueryRequestPack;

// ----- SignOnQuery response-----
#define ECC_CMD_SIGN_ON_QUERY_RESPONSE_DATA_LEN  40//Total Length 40 總長度
typedef struct
{
    uint8_t     pack1[3];
    uint8_t     authCreditBalance[3];
    uint8_t     pack2[28];
    uint8_t     remainderAddQuota[3];
    uint8_t     pack3[3];
}ECCCmdPPRSignOnQueryResponseData; 

#define ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_1         0x9000
#define ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_2         0x6304
#define ECC_CMD_SIGN_ON_QUERY_SUCCESS_ID_3         0x6305


//********************  5.2 PPR_SignOn：將 0810 端末開機訊息透過 Reader 傳入 SAM 卡中作認證  ********************
// ----- PPR_SignOn：將 Request-----
#define ECC_CMD_PPR_SIGN_ON_REQUEST_DATA_LEN  128//Total length 128 總長度
typedef struct
{ 
    ////---(以下 1 個欄位用於舊 SAM Card)
    uint8_t         HTAC[8];                    //H-TAC 8 Host Host 認證碼 Response 電文：H-TAC
    ////---(以下 5 個欄位用於新 SAM Card)
    uint8_t         HAToken[16];                //HAToken 16 Host Response 電文：CPU HOST Token
    uint8_t         SAMUpdateOption;            //SAM Update Option 1 Host Response 電文：SAM Update Option
    uint8_t         NewSAMValue[40];            //New SAM Value 40 Host Response 電文：New SAM Value
    uint8_t         UpdateSAMValueMAC[16];      //Update SAM Value MAC 16 Host Response 電文：Update SAM Value MAC
    ////---(以下 5 個欄位為 PPR_SignOn 設定參數之值，適用於有 SignOn 之設備)
    uint8_t         Flag01;                     //CPD Read Flag／One Day Quota Write For Micro Payment／SAM SignOnControl Flag／Check EV Flag For Mifare Only／ Merchant Limit Use For Micro Payment  1 Host
                                                        //Response 電文：CPU CPD Read Flag
                                                        //例: 0x00
                                                        //則填入 xxxxxx00b
                                                        //Response 電文：CPU One Day Quota Write Flag
                                                        //例: 0x11
                                                        //則填入 xxxx11xxb
                                                        //Response 電文：CPU SAM SignOnControl Flag
                                                        //例: 0x11
                                                        //則填入 xx11xxxxb
                                                        //Response 電文：Check EV Flag
                                                        //例: 0x00
                                                        //則填入 x0xxxxxxb
                                                        //Response 電文：Deduct Limit Flag
                                                        //例: 0x00
                                                        //則填入 0xxxxxxxb
    uint8_t         Flag02;                     //One Day Quota Flag For Micro Payment／Once Quota Flag For Micro Payment／Check Debit Flag／RFU(Reserved For Use) 1 Host
                                                        //Response 電文：One Day Quota Flag
                                                        //例: 0x11
                                                        //則填入 xxxxxx11b
                                                        //Response 電文：Once Quota Flag
                                                        //例: 0x01
                                                        //則填入 xxxxx1xxb
                                                        //Response 電文：Check Deduct Flag
                                                        //例: 0x01
                                                        //則填入 xxxx1xxxb
                                                        //固定填入 0000xxxxb
    uint8_t         OneDayQuota[2];                     //One Day Quota 2 Host Response 電文：One Day Quota For Micro Payment 
    uint8_t         OnceQuotaForMicroPayment[2];        //Once Quota For Micro Payment 2 Host Response 電文：Once Quota
    uint8_t         CheckDebitValue[2];                 //Check Debit Value 2 Host Response 電文：Check Deduct Value
    ////---(以下 2 個欄位用於舊 SAM Card 的額度控管)
    uint8_t         AddQuotaFlag;                       //Add Quota Flag 1 Host Response 電文：Add Quota Flag
    uint8_t         AddQuota[3];                        //Add Quota 3 Host Response 電文：Add Quota
    uint8_t         RFU[31];                            //RFU(Reserved For Use) 31 Host 保留，補 0x00，31bytes
    uint8_t         EDC_1;                             //EDC 4 Host  Response 電文：CPU Hash Type +
    uint8_t         EDC_2[3];                                                            //Response 電文：CPU EDC
}ECCCmdPPRSignOnRequestData; 

typedef struct
{
    uint8_t CLA;
    uint8_t INS;
    uint8_t P1;
    uint8_t P2;
    uint8_t Lc;
    ECCCmdPPRSignOnRequestData data;
    uint8_t Le;    
}ECCCmdPPRSignOnRequest;  

typedef struct
{
    uint8_t                 header;
    uint8_t                 cmd1;
    uint8_t                 cmd2;
    uint16_t                len;
    ECCCmdPPRSignOnRequest    body;
    uint8_t                 LRC;
    uint8_t                 tail1;  
    uint8_t                 tail2;  
}ECCCmdPPRSignOnRequestPack;

// ----- PPR_SignOn：將 Response-----
#define ECC_CMD_PPR_SIGN_ON_RESPONSE_DATA_LEN 29// Total Length 29 總長度
typedef struct
{
    uint8_t             CreditBalanceChangeFlag;                //Credit Balance Change Flag 1 SAM 加值授權額度(ACB)變更旗標
                                                                    //0x00 未變更
                                                                    //0x01 額度有變更
    ////--- (以下 5 欄位用於新 SAM Card)：Credit Balance Change Flag=0x00 時，以下欄位補 0x00
    uint8_t             OriginalAuthorizedCreditLimit[3];       //Original Authorized Credit Limit 3 SAM 原加值額度預設值(ACL) Unsigned and LSB First
    uint8_t             OriginalAuthorizedCreditBalance[3];     //Original Authorized Credit Balance 3 SAM 原加值授權額度(ACB) Unsigned and LSB First
    uint8_t             OriginalAuthorizedCreditCumulative[3];  //Original Authorized Credit Cumulative 3 SAM 原加值累積已用額度(ACC) Unsigned and LSB First
    uint8_t             OriginalAuthorizedCancelCreditCumulative[3];    //Original Authorized Cancel Credit Cumulative 3 SAM 原取消加值累積已用額度(ACCC) Unsigned and LSB First
    uint8_t             CACrypto[16];  //index13                         //CACrypto 16 SAM Credit Authorization Cryptogram
}ECCCmdPPRSignOnResponseData;

#define ECC_CMD_SIGN_ON_SUCCESS_ID         0x9000
#define ECC_CMD_SIGN_ON_NEED_UPDATE_ID     0x6308



/************************************************************/
/*                      授權電文說明                        */
/************************************************************/
//********************  6.1 Request 電文 0800  ********************
#define ECC_CMD_SIGN_ON_REQUEST_SOCKET_LEN  362//總長度 362
typedef struct
{
    uint8_t     TotalLen[2];            //Length 2 Unsigned 總 長 度 360byte( 不 含 本 欄位)固定填 0x01 68
    uint8_t     Header[8];     //index2         //Header 8 ASCII 固定 99903480 0x39 39 39 30 33 34 38 30
    uint8_t     MessageTypeID[4];       //Message Type ID 4 ASCII 固定 0800 0x30 38 30 30
    uint8_t     DataFieldLen[3];        //1 Data Field Length 3 ASCII 總長度345byte 0x33 34 35
    uint8_t     ProcessingCode[6];      //2 Processing Code 6 ASCII 固定 881999 (ASCII) 0x38 38 31 39 39 39
    uint8_t     BlackListVersion[2];    //6 BlackListVersion 2 Unsigned 固定 0x00 
    uint8_t     MsgType;                //7 Msg Type 1 Unsigned 固定 0x00
    uint8_t     Subtype;                //8 Subtype 1 Unsigned 固定 0x00
    uint8_t     DeviceID[4];            //9 Device ID 4 Unsigned Device ID
    uint8_t     ServiceProvider;        //10 Service Provider 1 Unsigned Device ID 的第三個 byte 例:Device ID=0x3F B0 07 00 Service Provider=0x07
    ECCCmdDataTime  TXNDateTime;        //11 TXN Date Time 4 Unsigned 現在時間(Unix Date Time) Request,Response,Confirm 皆須一致
    uint8_t     TMID[2];                //28 TM ID 2 ASCII TM ID
    uint8_t     TMTXNDateTime[14]; //index38     //29 TM TXN Date Time 14 ASCII TM TXN Date Time 例:2012/10/23 18:50:35 0x32 30 31 32 31 30 32 33 3138 35 30 33 35
    ECCCmdSerialNumber     TMSerialNumber;      //30 TM Serial Number 6 ASCII TM Serial Number 右靠左補 0;值須為 0~9
    uint8_t     TMAgentNumber[4];       //31 TM Agent Number 4 ASCII TM Agent Number 右靠左補 0;值須為 0~9
    uint8_t     STAC[8];                //33 S-TAC 8 Unsigned S-TAC
    uint8_t     KeyVersion;             //35 Key Version 1 Unsigned SAM Key Version
    uint8_t     SAMID[8];               //36 SAM ID 8 ASCII SAM ID
    uint8_t     SAMSN[4];               //37 SAM SN 4 Unsigned SAM SN
    uint8_t     SACRN[8];               //38 SAM CRN 8 Unsigned SAM CRN
    uint8_t     ReaderFirmwareVersion[6];   //39 Reader Firmware Version 6 Unsigned Reader FW Version
    uint8_t     NetworkManagement[3];   //46 Network Management Code 3 ASCII 固定 079:Device Control 0x30 37 39
    uint8_t     OneDayQuotaFlag;         //One Day Quota Flag 1 Unsigned One Day Quota Flag For Micro Payment
                                                        //例: xxxxxx11b
                                                        //則填入0x11
    uint8_t     OneDayQuota[2];                     //One Day Quota 2 Unsigned One Day Quota For Micro Payment
    uint8_t     OnceQuotaFlag;                      //Once Quota Flag 1 Unsigned Once Quota Flag For Micro Payment
                                                        //例: xxxxx1xxb
                                                        //則填入0x01
    uint8_t     OnceQuota[2];                       //Once Quota 2 Unsigned Once Quota For Micro Payment
    uint8_t     CheckEVFlag;                        //Check EV Flag 1 Unsigned Check EV Flag For Mifare Only
                                                        //例: x0xxxxxxb
                                                        //則填入0x00
    uint8_t     AddQuotaFlag;                       //Add Quota Flag 1 Unsigned Add Quota Flag
    uint8_t     AddQuota[3];                        //Add Quota 3 Unsigned Add Quota
    uint8_t     CheckDeductFlag;                    //Check Deduct Flag 1 Unsigned Check Debit Flag
                                                        //例: xxxx1xxxb
                                                        //則填入0x01
    uint8_t     CheckDeductValue[2];                //Check Deduct Value 2 Unsigned Check Debit Value
    uint8_t     DeductLimitFlag;                    //Deduct Limit Flag 1 Unsigned Merchant Limit Use For Micro Payment
                                                        //例: 0xxxxxxxb
                                                        //則填入0x00
    uint8_t     APIVersion[4];                      //API Version 4 Unsigned API Version
    uint8_t     RFU[5];                             //RFU 5 Unsigned RFU
    uint8_t     TheRemainderOfAdd[3];               //The Remainder of Add 3 Unsigned The Remainder of Add Quota Quota
    uint8_t     deMACParameter[8];                  //deMAC Parameter 8 Unsigned deMAC Parameter
    uint8_t     CancelCreditQuota[3];               //Cancel Credit Quota 3 Unsigned Cancel Credit Quota
    uint8_t     RFU2[18];                           //RFU 18 Unsigned 固定填0x00

    uint8_t     CPUCardPhysicalID[7];               //58 CPU Card Physical ID 7 Unsigned 固定填 0x00
    uint8_t     CPUTXNAMT[3];                       //59 CPU TXN AMT 3 Unsigned 固定填 0x00
    uint8_t     CPUDeviceID[6];                     //62 CPU Device ID 6 Unsigned New Device ID
    uint8_t     CPUServiceProvider[3];              //63 CPU Service Provider ID 3 Unsigned New Device ID 的 4~6byte 例:New Device ID=0xD2 64 03 7B 00 00 CPU Service Provider=0x7B 00 00
    uint8_t     CPUEVBeforeTXN[3];                  //71 CPU EV Before TXN 3 Unsigned 固定填 0x00
    uint8_t     CPUSAMID[8];                        //79 CPU SAM ID 8 ASCII SID
    uint8_t     CPUSAMTXNCnt[4];                    //80 CPU SAM TXN CNT 4 Unsigned STC 
    uint8_t     SAMVersionNumber;                       //SAM Version Number 1 Unsigned SAM Version Number
    uint8_t     SAMUsageControl[3];                     //SAM Usage Control 3 Unsigned SAM Usage Control
    uint8_t     SAMAdminKVN;                            //SAM Admin KVN 1 Unsigned SAM Admin KVN
    uint8_t     SAMIssuerKVN;                           //SAM Issuer KVN 1 Unsigned SAM Issuer KVN
    uint8_t     TagListTable[40];                       //Tag List Table 40 Unsigned Tag List Table
    uint8_t     SAMIssuerSpecificData[32];              //SAM Issuer Specific Data 32 Unsigned SAM Issuer Specific Data

    uint8_t     CPURSAM[8];                         //82 CPU RSAM 8 Unsigned RSAM
    uint8_t     CPURHOST[8];                        //83 CPU RHOST 8 Unsigned RHOST
    uint8_t     AuthorizedCreditLimit[3];               //Authorized Credit Limit 3 Unsigned Authorized Credit Limit
    uint8_t     AuthorizedCreditBalance[3];             //Authorized Credit Balance 3 Unsigned Authorized Credit Balance
    uint8_t     AuthorizedCreditCumulative[3];          //Authorized Credit Cumulative 3 Unsigned Authorized Credit Cumulative
    uint8_t     AuthorizedCancelCreditCumulative[3];    //Authorized Cancel Credit Cumulative 3 Unsigned Authorized Cancel Credit Cumulative

    uint8_t     CPUSAMSingleCreditTXNAMTLimit[3];        //85 CPU SAM Single Credit TXN AMT Limit 3 Unsigned Single Credit TXN AMT Limit
    uint8_t     CPUTMLocationID[10];                //90 CPU TM Location ID 10 ASCII TM Location ID
    uint8_t     CPUTERMToken[16];                   //91 CPU TERM Token 16 Unsigned SATOKEN
    uint8_t     CPULastDeviceID[6];                     //CPU Last Device ID 6 Unsigned Previous New Device ID
    uint8_t     CPULastSAMTXNCNT[4];                    //CPU Last SAM TXN CNT 4 Unsigned Previous STC
    ECCCmdDataTime  LastTXNDateTime;                    //Last TXN Date Time 4 Unsigned Previous TXN Date Time
    uint8_t     CPULastCreditBalanceChangeFlag;         //CPU Last Credit Balance Change Flag 1 Unsigned Previous Credit Balance Change Flag
    uint8_t     ConfirmCode[2];                         //Confirm Code 2 Unsigned Previous Confirm Code
    uint8_t     CPUTERMCryptogram[16];                  //CPU TERM Cryptogram 16 Unsigned Previous CACrypto

    uint8_t     CPUSAMSignOnControlFlag;            //95 CPU SAM SignOnControl Flag 1 Unsigned SAM SignOnControl Flag
                                                            //例: xx11xxxxb
                                                            //則填入 0x11
    uint8_t     CPUSpecVersionNumber;               //96 CPU Spec. Version Number 1 Unsigned Spec. Version Number
    uint8_t     CPUOneDayQuotaWriteFlag;            //97 CPU One Day Quota Write Flag 1 Unsigned One Day Quota Write For Micro Payment
                                                            //例: xxxx11xxb
                                                            //則填入 0x11
    uint8_t     CPUCPDReadFlag;                     //98 CPU CPD Read Flag 1 Unsigned CPD Read Flag
                                                            //例: xxxxxx00b
                                                            //則填入 0x00
}ECCCmdSignOnRequestSocketData; 
#define ECC_CMD_SOCKET_SIGN_ON_REQUEST_SUCCESS_ID         0x3030

//********************  6.2 Response 電文 0810  ********************
#define ECC_CMD_SIGN_ON_RESPONSE_SOCKET_LEN  203//總長度 203
typedef struct
{
    uint8_t     Length[2];              //Length 2 Unsigned
    uint8_t     Header[8];              //Header 8 ASCII
    uint8_t     MessageTypeID[4];       //Message Type ID 4 ASCII
    uint8_t     DataFieldLen[3];        //1 Data Field Length 3 ASCII
    uint8_t     ProcessingCode[6];      //2 Processing Code 6 ASCII
    uint8_t     MsgType;                //7 Msg Type 1 Unsigned
    uint8_t     SubType;                //8 Subtype 1 Unsigned
    uint8_t     DeviceID[4];            //9 Device ID 4 Unsigned
    ECCCmdDataTime     TXNDateTime;     //11 TXN Date Time 4 Unsigned
    uint8_t     TMID[2];     //index35  //28 TM ID 2 ASCII
    uint8_t     TMTXNDateTime[14];      //29 TM TXN Date Time 14 ASCII
    ECCCmdSerialNumber     TMSerialNumber;      //30 TM Serial Number 6 ASCII
    uint8_t     TMAgentNumber[4]; //index55 //31 TM Agent Number 4 ASCII
    uint8_t     HTAC[8];      //index59 //34 H-TAC 8 Unsigne
    uint8_t     GatewayRespond[2];  //index67    //43 Getway Response 2 Unsigned
    uint8_t     NetworkManagement[3];   //46 Network Management Code 3 ASCII
    uint8_t     OneDayQuotaFlag;//index 72  //One Day Quota Flag 1 Unsigned
    uint8_t     OneDayQuota[2]; //index 73  //One Day Quota 2 Unsigned
    uint8_t     OnceQuotaFlag;  //index 75  //Once Quota Flag 1 Unsigned
    uint8_t     OnceQuota[2];   //index 76  //Once Quota 2 Unsigned
    uint8_t     CheckEVFlag;    //index 78  //Check EV Flag 1 Unsigned
    uint8_t     AddQuotaFlag;   //79            //Add Quota Flag 1 Unsigned
    uint8_t     AddQuota[3];    //80            //Add Quota 3 Unsigned
    uint8_t     CheckDeductFlag;  //index 83 //Check Deduct Flag 1 Unsigned
    uint8_t     CheckDeductValue[2];//index 84        //Check Deduct Value 2 Unsigned
    uint8_t     DeductLimitFlag;   //index 86 //Deduct Limit Flag 1 Unsigned
    uint8_t     APIVersion[4];              //API Version 4 Unsigned
    uint8_t     RFU[5];                     //RFU 5 Unsigned

    uint8_t     CPUCardPhysicalID[7];   //58 CPU Card Physical ID 7 Unsigned
    uint8_t     CPUDeviceID[6];         //62 CPU Device ID 6 Unsigned
    uint8_t     CPUSAMTXNCnt[4];        //80 CPU SAM TXN CNT 4 Unsigned
    uint8_t     CPUHashType;            //86 CPU Hash Type 1 Unsigned
    uint8_t     CPUTMLocationID[10]; //index114   //90 CPU TM Location ID 10 ASCII
    uint8_t     CPUHOSTToken[16]; //index124      //92 CPU HOST Token 16 Unsigned
    uint8_t     SAMUpdateOption;  //index140      //94 SAM Update Option 1 Unsigned
    uint8_t     NewSAMValue[40];  //index141          //New SAM Value 40 Unsigned
    uint8_t     UpdateSAMValueMAC[16]; //index181     //Update SAM Value MAC 16 Unsigned

    uint8_t     CPUSAMSignOnControlFlag; //index197   //95 CPU SAM SignOnControl Flag 1 Unsigned
    uint8_t     CPUOneDayQuotaWriteFlag; //index198   //97 CPU One Day Quota Write Flag 1 Unsigned
    uint8_t     CPUCPDReadFlag;  //index199           //98 CPU CPD Read Flag 1 Unsigned
    uint8_t     CPUEDC[3];                  //99 CPU EDC 3 Unsigned
}ECCCmdSignOnResponseSocketData; 
//********************  6.3 Confirm 電文 0801  ********************
#define ECC_CMD_SIGN_ON_CONFIRM_REQUEST_SOCKET_LEN  69//總長度 69
typedef struct
{
    uint8_t     Length[2];              //Length 2 Unsigned 總長度67 byte(不含本欄位) 固定填 0x00 43
    uint8_t     Header[8];              //Header 8 ASCII 固定 99903480  0x39 39 39 30 33 34 38 30
    uint8_t     MessageTypeID[4];       //Message Type ID 4 ASCII 固定 0801 0x30 38 30 31
    uint8_t     DataFieldLength[3];     //1 Data Field Length 3 ASCII 總長度52 byte 0x30 35 32
    uint8_t     ProcessingCode[6];      //2 Processing Code 6 ASCII 固定 881999 (ASCII) 0x38 38 31 39 39 39
    uint8_t     MsgType;                //7 Msg Type 1 Unsigned 固定 0x00
    uint8_t     Subtype;                //8 Subtype 1 Unsigned 固定 0x00
    uint8_t     DeviceID[4];            //9 Device ID 4 Unsigned Device ID
    ECCCmdDataTime  TXNDateTime;        //11 TXN Date Time 4 Unsigned 現在時間(Unix Date Time) Request,Response,Confirm 皆須一致
    uint8_t     ConfirmCode[2];         //47 Confirm Code 2 Unsigned PPR_SignON 的回應碼  例:0x63 08
    uint8_t     CPUCardPhysicalID[7];   //58 CPU Card Physical ID 7 Unsigned 固定 0x00
    uint8_t     CPUDeviceID[6];         //62 CPU Device ID 6 Unsigned New Device ID
    uint8_t     CPUSAMTXNCnt[4];        //80 CPU SAM TXN CNT 4 Unsigned STC
    uint8_t     CPUCreditBalanceChangeFlag;     //100 CPU Credit Balance Change Flag 1 Unsigned Credit Balance Change Flag
    uint8_t     CPUTERMCryptogram[16];  //101 CPU TERM Cryptogram 16 Unsigned CACrypto
}ECCCmdSignOnConfirmRequestSocketData; 
//********************  6.4 Response Code  ********************
//Response code 為 2 bytes ASCII code(ex. 00 = 0x3030)
#define ECC_CMD_SIGN_ON_CONFIRM_RESPONSE_SOCKET_LEN  2//總長度
typedef struct
{
    uint8_t value[2];
}ECCCmdSignOnConfirmResponseSocketData; 

#define ECC_CMD_SOCKET_SIGN_ON_CONFIRM_SUCCESS_ID         0x3030//0x9000
#pragma pack()
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/

    
    
#ifdef __cplusplus
}
#endif

#endif //_IPASS_LIB_

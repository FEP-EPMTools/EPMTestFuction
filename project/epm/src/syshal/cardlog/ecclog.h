/**************************************************************************//**
* @file     ecclog.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __ECC_LOG_H__
#define __ECC_LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _PC_ENV_
#else
    #include "nuc970.h"
    #include "sys.h"
#endif
#include "ecccmd.h"
#include "ecclogcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ECC_LOG_FILE_SAVE_POSITION          FILE_AGENT_STORAGE_TYPE_AUTO
#define ECC_LOG_FILE_SAVE_POSITION_SFLASH   FILE_AGENT_STORAGE_TYPE_SFLASH_RECORD
#define ECC_LOG_FILE_EXTENSION              "ECC"
//#define ECC_LOG_FILE_EXTENSION_SFLASH       "dpts"
#define ECC_LOG_FILE_DIR                    "1:"  
    
    
#pragma pack(1)
typedef struct
{
    uint8_t  value[4];
}ECCLogDataTime;

#define TOTAL_ECC_LOG_HEADER_SIZE   66  //0x42
typedef struct 
{
    //uint8_t data[TOTAL_ECC_LOG_HEADER_SIZE];
    uint8_t         PurseVerNo;                 //1 PURSE_VER_NO 卡片版本 Unsigned 1 代碼參照 8.1 來源: 設備
    uint8_t         CardTxnTypeID;              //2 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    uint8_t         CardTxnSubTypeID;           //3 CARD_TXN_SUBTYPE_ID 交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    uint8_t         DevID[3];                   //4 DEV_ID 設備編號 Unsigned 3 SP_ID(1 Byte):如 122 即為 0x7A
                                                        //DEV_TYPE(4 Bits): 3：BV 如 BV 即為 0x3□
                                                        //DEV_ID_NO(12 Bites): 如 1234 即為 0x□4 0xD2
                                                        //組合 HEX: 0x7A 0x34 0xD2
                                                        //倒數2 Bytes LSB FIRST: 0x7A 0xD2 0x34
                                                        //來源: Mifare Sam 卡
    uint8_t         SpID;                       //5 SP_ID 業者代碼 Unsigned 1  如 122 即為 0x7A 來源: 設備
    ECCLogDataTime  TxnTimeStamp;               //6 TXN_TIMESTAMP 交易時間 UnixTime 4 來源: 設備
    uint8_t         CardPhysicalID[7];          //7 CARD_PHYSICAL_ID 卡片晶片號碼 Unsigned 7 來源: 設備
    uint8_t         IssuerID;                   //8 ISSUER_ID 發卡單位 Unsigned 1   PVN <> 0 時: 來源: Cpu 卡
                                                                                  //PVN = 0 時: 2 (0x02)：悠遊卡公司
                                                                                                //66 (0x42)：基隆交通卡
                                                                                                //來源: 設備
    uint8_t         CardTxnSeqNo;               //9 CARD_TXN_SEQ_NO 交易後序號 Unsigned 1 來源: 卡片
    uint8_t         TxnAMT[3];                  //10 TXN_AMT 交易金額 Signed 3 來源: 設備
    uint8_t         ElectronicValue[3];         //11 ELECTRONIC_VALUE 交易後卡片金額 Signed 3 來源: 卡片
    uint8_t         SVCELocID;                  //12 SVCE_LOC_ID 場站代碼 Unsigned 1 來源: 設備
    uint8_t         CardPhysicalIDLen;          //13 CARD_PHYSICAL_ID_LEN 晶片號碼長度 Unsigned 1 如 4 bytes 即為 0x04
                                                                                                    //如 7 bytes 即為 0x07
                                                                                                    //來源: 卡片
    uint8_t         NewCardTxnSeqNo[3];         //14 NEW_CARD_TXN_SEQ_NO 新交易後序號 Unsigned 3 來源: 卡片
    uint8_t         NewDevID[6];                //15 NEW_DEV_ID 新設備編號 Unsigned 6           NEW_SP_ID(3 Bytes): 如 122 即為0x00 0x00 0x7A
                                                                                                //NEW_DEV_TYPE(1 Byte): 3：BV 如 3 即為 0x03
                                                                                                //4096(含)之後開始編碼
                                                                                                //NEW_DEV_ID_NO (2 Bytes): 如 4096 即為 0x10 0x00
                                                                                                //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
                                                                                                //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                                                                //來源: Cpu Sam 卡
                                                                                                //二代卡設備未修改，以 SIS2 傳送時: 固定填 6 Bytes 0x00
    uint8_t         NewSpID[3];                 //16 NEW_SP_ID 新業者代碼 Unsigned 3 NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
                                                                                    //以 LSB FIRST 傳送: 0x7A 0x00 0x00
                                                                                    //來源: 設備
    uint8_t         NewSVCELocID[2];            //17 NEW_SVCE_LOC_ID 新場站代碼 Unsigned 2 NEW_SVCE_LOC_ID(2 Bytes): 如 1 即為 0x00 0x01
                                                                                    //以 LSB FIRST 傳送: 0x01 0x00
                                                                                    //來源: 設備
    uint8_t         NewCardTxnSubTypeID;        //18 NEW_CARD_TXN_SUBTYPE_ID 新交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    uint8_t         NewPersonalProfile;         //19 NEW_PERSONAL_PROFILE 新身份別 Unsigned 1 0x00: 一般
                                                                                            //0x01: 敬老 1
                                                                                            //0x02: 敬老 2
                                                                                            //0x03: 愛心
                                                                                            //0x04: 陪伴
                                                                                            //0x05: 學生
                                                                                            //0x08: 優待
                                                                                            //來源: 卡片
    uint8_t         TxnPersonalProfile;         //20 TXN_PERSONAL_PROFILE 交易身份別 Unsigned 1 實際交易的身份別，如卡片身份別為學生，但部份業者未給予優待，以一般卡扣款，則此欄位填入一般
                                                                    //0x00: 一般
                                                                    //0x01: 敬老 1
                                                                    //0x02: 敬老 2
                                                                    //0x03: 愛心
                                                                    //0x04: 陪伴
                                                                    //0x05: 學生
                                                                    //0x08: 優待
                                                                    //來源: 設備
    uint8_t         ACQID;                      //21 ACQ_ID 收單單位 Unsigned 1   2 (0x02)：悠遊卡公司
                                                                                //66 (0x42)：基隆交通卡
                                                                                //來源: 設備
    uint8_t         CardPurseID[8];             //22 CARD_PURSE_ID 錢包編號(外觀號碼) Pack 8      PVN <> 0 時:
                                                                                                    //如卡號 1234567890123456
                                                                                                    //即為 0x12 0x34 0x56 0x78 0x90 0x12 0x34 0x56
                                                                                                    //編碼參照 8.5
                                                                                                    //來源: Cpu 卡
                                                                                                //PVN = 0 時:
                                                                                                    //固定填 8 Bytes 0x00
    uint8_t         CardCTCSeqNo[3];            //23 CARD_CTC_SEQ_NO 卡片處理序號 Unsigned Msb 3   PVN <> 0 時: 如 1 即為 0x00 0x00 0x01
                                                                                                        //以 MSB FIRST 傳送:  0x00 0x00 0x01
                                                                                                        //來源: Cpu 卡
                                                                                                 //PVN = 0 時: 固定填 3 Bytes 0x00
    uint8_t         AreaCode;                   //24 AREA_CODE 區碼 Unsigned 1 來源: 卡片
    uint8_t         SubAreaCode[2];             //25 SUB_AREA_CODE 附屬區碼 Unsigned 2    PVN <> 0 時: 依 3 碼郵遞區號  來源: Cpu 卡
                                                                                        //PVN = 0 時: 固定填 2 Bytes 0x00
    uint8_t         SeqNoBefTxn[3];             //26 SEQ_NO_BEF_TXN 交易前序號 Unsigned 3  來源: 卡片  二代卡設備未修改，以 SIS2 傳送時: 固定填 3 Bytes 0x00
    uint8_t         EVBdfTxn[3];                //27 EV_BEF_TXN 交易前卡片金額 Signed 3 來源: 卡片
}ECCLogHeader;

#define TOTAL_ECC_LOG_TAIL_SIZE     36  //0x24
typedef struct 
{
    //uint8_t data[TOTAL_ECC_LOG_TAIL_SIZE];
    uint8_t         MACKeySet;                  //1 MACKeySet 0x00 Byte 1 固定填 0x00
    uint8_t         MAC3DESKey;                 //2 MAC3DESKey 0x01 Byte 1 固定填 0x01
    uint8_t         MACValue[4];                //3 MACValue 交易驗証碼 Unsigned 4
                                                        //PVN <> 0 時: 固定填 4 Bytes 0x00
                                                        //PVN = 0 時: 來源:設備
    uint8_t         MACMFRC[4];                 //4 MACMFRC 讀卡機編號 Unsigned 4
                                                        //PVN <> 0 時: 固定填 4 Bytes 0x00
                                                        //PVN = 0 時: 來源:設備
    uint8_t         SAMID[8];                   //5 SAM_ID SAM ID Pack 8
                                                        //PVN <> 0 時: 來源: Cpu Sam 卡
                                                        //PVN = 0 時: 來源:設備
    uint8_t         HashType;                   //6 HASH_TYPE HASH_TYPE Unsigned 1
                                                        //PVN <> 0 時: 代碼參照 8.4 來源: 設備
                                                        //PVN = 0 時: 固定填 0x00
    uint8_t         HostAdminKeyVer;            //7 HOST_ADMIN_KEYVER HOST ADMIN KEY 版本 Byte 1
                                                        //PVN <> 0 時: MAC 用哪個版本 KEY 去押的  來源: Cpu Sam 卡
                                                        //PVN = 0 時: 固定填 0x00
    uint8_t         CpuMACValue[16];            //8 CPU_MAC_VALUE MAC 值 Byte 16
                                                        //PVN <> 0 時: 來源: Cpu Sam 卡計算
                                                        //PVN = 0 時: 固定填 16 Bytes 0x00
}ECCLogTail;




typedef struct
{
    ECCLogHeader        Header;
    uint8_t             EntryExitInd;           //1 ENTRY_EXIT_IND 進出(上下車)旗標 Unsigned 1 代碼參照 8.7 
                                                    //來源: 設備  
                                                    //預設填 0x00
    uint8_t             EntryLocID[2];          //2 ENTRY_LOC_ID 進站代碼 Unsigned 2 
                                                    //來源: 卡片
    ECCLogDataTime      EntryDateTime;          //3 ENTRY_DATETIME 進站時間 UnixTime 4 捷運交易加入進站時間 
                                                    //來源: 卡片
    uint8_t             XferCODE;  //index 7    //4 XFER_CODE 轉乘代碼 Byte 1 bit7..4: 上筆轉乘代碼
                                                    //bit3..0: 本次轉乘代碼
                                                    //如捷運轉公車: 0x12
                                                    //來源: 設備
                                                    //預設填 0x00
    uint8_t             NewXferCODE[2];         //5 NEW_XFER_CODE 新轉乘代碼 Byte 2
                                                    //Byte1: 上筆轉乘代碼
                                                    //Byte2: 本次轉乘代碼
                                                    //如捷運轉公車: 0x01 0x02
                                                    //來源: 設備
                                                    //預設填 2 Bytes 0x00
    uint8_t             OriAMT[3];  //index 10  //6 ORI_AMT 原價(未打折) Unsigned 3
                                                    //計算參照 8.8
                                                    //來源: 設備
    uint8_t             DisRate;    //index 13  //7 DIS_RATE 費率 Unsigned 1  如 85 折，DIS_RATE=85 即為 0x55
                                                    //如未打折，DIS_RATE=100 即為 0x64
                                                    //計算參照 8.8
                                                    //來源: 設備
    uint8_t             TicketAMT[3];  //index 14   //8 TICKET_AMT 票價(打折後) Unsigned 3
                                                    //計算參照 8.8
                                                    //來源: 設備
    uint8_t             XferDisc[3];            //9 XFER_DISC 轉乘優恵 Unsigned 3
                                                    //計算參照 8.8
                                                    //來源: 設備
                                                    //預設填 3 Bytes 0x00
    uint8_t             PersonDisc[3];          //10 PERSONAL_DISC 個人優恵 Unsigned 3
                                                    //計算參照 8.8
                                                    //來源: 設備
                                                    //預設填 3 Bytes 0x00
    uint8_t             PeakDisc[3];            //11 PEAK_DISC 尖峰優恵 Unsigned 3
                                                    //計算參照 8.8
                                                    //來源: 設備
                                                    //預設填 3 Bytes 0x00
    uint8_t             Penalty[3];             //12 PENALTY 罰款 Unsigned 3  計算參照 8.8
                                                    //來源: 設備
                                                    //預設填 3 Bytes 0x00
    uint8_t             OtherDisc[3];           //13 OTHER_DISC 其他補助金額 Unsigned 3
                                                    //計算參照 8.8
                                                    //來源: 設備
                                                    //預設填 3 Bytes 0x00
    uint8_t             AdvanceAMT[3];          //14 ADVANCE_AMT 預扣款(進站/上車) Signed 3
                                                    //PVN <> 0 時:
                                                        //出站(下車)填入進站(上車)預扣款
                                                        //計算參照 8.8
                                                        //來源: Cpu 卡
                                                    //PVN = 0 時:
                                                        //固定填 3 Bytes 0x00
    uint8_t             PersonalUsePoints[2];   //15 PERSONAL_USE_POINTS 補助款使用點數 Unsigned 2
                                                    //來源: 設備
                                                    //預設填 2 Bytes 0x00
    uint8_t             PersonalCounter[2];     //16 PERSONAL_COUNTER 補助款累積點數 Unsigned 2
                                                    //PVN <> 0 時:  來源: Cpu 卡
                                                    //PVN = 0 時: 固定填 2 Bytes 0x00
    uint8_t             LoyaltyCounter[2];  //index 39    //17 LOYALTY_COUNTER 累積忠誠點 Unsigned 2
                                                    //來源: 設備
                                                    //預設填 2 Bytes 0x00
    uint8_t             AgentNo[2];         //index 41    //18 AGENT_NO 操作員代碼 Unsigned 2
                                                    //如 Ascii 字元'1234'即為 0x31 0x32 0x33 0x34
                                                    //轉 10 進制數值 1234 即為 0x04 0xD2
                                                    //以 LSB FIRST 傳送: 0xD2 0x04
                                                    //來源: 設備
                                                    //此欄位 10 進制數值應與 PPR_Reset/PPR_Reset_Online 一致
                                                    //預設填 2 Bytes 0x00
    uint8_t             BusTypeID;              //19 BUS_TYPE_ID 車種類別 Unsigned 1
                                                    //增加停車場區分汽機車
                                                    //代碼參照 8.10
                                                    //來源: 設備
                                                    //預設填 0x00
    uint8_t             CarPlateNo[10];         //20 CAR_PLATE_NO 車牌號碼 Ascii 10 來源: 設備
    uint8_t             VenderID[3];            //21 VENDER_ID 售卡服務提供者 (各業者自行規畫紅利積點)Unsigned 3 固定填 3 Bytes 0x00
    
    //2018.08.14 -->  F. 【RFU1-XFER_PRE_SEQ_NO】轉乘上筆交易序號：應填轉乘上筆交易序號，卻填 0。
    uint8_t             RFU1XferPreSeqNo[3];
    //2018.08.14 --> G.【RFU1-CARD_TYPE】票別：應填票卡卡種，卻填 0。
    uint8_t             RFU1CardType;
    //2018.08.14 --> H.【RFU1-XFER_PRE_TIMESTAMP】上筆交易時間：應填轉乘上筆交易時間，卻都填 1970/1/1 00:00:00。
    uint8_t             RFU1XferPreTimeStamp[4];
    
    uint8_t             RFU[8];                //22 RFU1 保留欄位(共用) Byte 16
                                                //PVN <> 0 時:
                                                    //固定填 16 Bytes 0x00
                                                //PVN = 0 時:
                                                    //固定填 16 Bytes 0x00
    uint8_t             RFU1Ver;                //23 RFU1_VER 欄位版本(共用) Unsigned 1
                                                    //PVN <> 0 時:
                                                        //固定填 0x00
                                                    //PVN = 0 時:
                                                        //固定填 0x00
    //2018.08.14 --> J.【RFU2-SUB_LOC_ID】附屬場站代碼：應填 50，卻填 00
    uint8_t             RFU2SubLocId[2];
    uint8_t             RFU2[14];               //24 RFU2 保留欄位(業者) Byte 16
                                                    //PVN <> 0 時:
                                                        //定義參照 8.11
                                                        //未定義: 固定填 16 Bytes 0x00
                                                    //PVN = 0 時:
                                                        //固定填 16 Bytes 0x00
    uint8_t             RFU2Ver;                 //25 RFU2_VER 欄位版本(業者) Unsigned 1
                                                    //PVN <> 0 時:
                                                        //定義參照 8.11
                                                        //未定義: 固定填 0x00
                                                    //PVN = 0 時:
                                                        //固定填 0x00
    uint8_t             TM;       //index 73    //26 TM(TXN Mode) 交易模式 Byte 1
                                                    //PVN <> 0 時: 來源: 設備
                                                    //PVN = 0 時: 固定填 0x00
    uint8_t             TQ;                     //27 TQ(TXN Qualifier) 交易屬性 Byte 1
                                                    //PVN <> 0 時: 來源: 設備
                                                    //PVN = 0 時: 固定填 0x00
    uint8_t             SignKeyVer;             //28 SIGN_KEYVER SIGNATURE KEY 版本 Byte 1
                                                    //PVN <> 0 時:
                                                        //來源: Cpu 卡
                                                    //PVN = 0 時:
                                                        //固定填 0x00
    uint8_t             SignValue[16];          //29 SIGN_VALUE SIGNATURE 值 Byte 16
                                                //PVN <> 0 時:
                                                    //來源: Cpu 卡
                                                //PVN = 0 時:
                                                    //固定填 16 Bytes 0x00
    ECCLogTail          Tail;
}ECCDeductLogBody; //len 0x6E
#define TOTAL_ECC_DEDUCT_LOG_BODY_SIZE   212  //0xd4

typedef struct
{
    uint8_t             PurseVerNo;             //1 PURSE_VER_NO 卡片版本 Unsigned 1 代碼參照 8.1 來源: 設備
    uint8_t             CardTxnTypeID;          //2 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    uint8_t             CardTxnSubTypeID;       //3 CARD_TXN_SUBTYPE_ID 交易次類別(卡片) Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    uint8_t             CardDevID[6];           //4 CARD_DEV_ID 設備編號(卡片) Unsigned 6   PVN=2，同 NEW_DEV_ID 如:
                                                                                                //NEW_SP_ID=49(0x31)
                                                                                                //NEW_DEV_TYPE=12(0x0C)
                                                                                                //NEW_DEV_ID_NO=99(0x63)
                                                                                                //組合 HEX: 00-00-31-0C-00-63
                                                                                                //以 LSB FIRST 傳送: 63-00-0C-31-00-00
                                                                                            //PVN<>2，同 DEV_ID，右補 3 Bytes 0x00
                                                                                                //同上例，組合 HEX: 31-C0-63
                                                                                                //倒數2 Bytes LSB FIRST: 31-63-C0
                                                                                                //右補 3 Bytes 0x00: 31-63-C0-00-00-00
                                                                                                //範例參照 8.6
                                                                                                //來源: 卡片
    uint8_t             CardSpID[3];            //5 CARD_SP_ID 業者代碼(卡片) Unsigned 3 PVN=2，同 NEW_SP_ID
                                                                        //PVN<>2，同 SP_ID
                                                                        //來源: 卡片
    ECCLogDataTime      TxnTimeStamp;           //6 TXN_TIMESTAMP 交易時間(卡片) UnixTime 4 來源: 卡片
    uint8_t             CardPhysicalID[7];      //7 CARD_PHYSICAL_ID 卡片晶片號碼 Unsigned 7 來源: 卡片
    uint8_t             IssuerID;               //8 ISSUER_ID 發卡單位 Unsigned 1     PVN <> 0 時: 來源: Cpu 卡
                                                                                    //PVN = 0 時:
                                                                                        //2 (0x02)：悠遊卡公司
                                                                                        //66 (0x42)：基隆交通卡
                                                                                        //來源: 設備
    uint8_t             CardTxnSwqNo[3];        //9 CARD_TXN_SEQ_NO 交易後序號(卡片) Unsigned 3 來源: 卡片
    uint8_t             TxnAMT[3];              //10 TXN_AMT 交易金額(卡片) Signed 3 來源: 卡片
    uint8_t             ElectronicValue[3];     //11 ELECTRONIC_VALUE 交易後卡片金額(卡片) Signed 3 來源: 卡片
    uint8_t             CardSvceLocID[2];       //12 CARD_SVCE_LOC_ID 場站代碼(卡片) Unsigned 2
                                                            //PVN=2，NEW_SVCE_LOC_ID
                                                            //PVN<>2，SVCE_LOC_ID
                                                            //來源: 卡片
    uint8_t             CardPhysicalIDLen;      //13 CARD_PHYSICAL_ID_LEN 晶片號碼長度 Unsigned 1
                                                            //如 4 bytes 即為 0x04
                                                            //如 7 bytes 即為 0x07
                                                            //來源: 卡片
    uint8_t             NewPersonalProfile;     //14 NEW_PERSONAL_PROFILE 新身份別 Unsigned 1
                                                            //0x00: 一般
                                                            //0x01: 敬老 1
                                                            //0x02: 敬老 2
                                                            //0x03: 愛心
                                                            //0x04: 陪伴
                                                            //0x05: 學生
                                                            //0x08: 優待
                                                            //來源: 卡片
    uint8_t             CardPurseID[8];         //15 CARD_PURSE_ID 錢包編號(外觀號碼) Pack 8
                                                            //PVN <> 0 時:
                                                                //如卡號 1234567890123456
                                                                //即為 0x12 0x34 0x56 0x78 0x90 0x12 0x34 0x56
                                                                //代碼參照 8.5
                                                                //來源: Cpu 卡
                                                            //PVN = 0 時:
                                                                //固定填 8 Bytes 0x00
    uint8_t             BankCode;               //16 BANK_CODE 銀行代碼 Unsigned 1    PVN <> 0 時:
                                                            //固定填 0x00
                                                            //PVN = 0 時:
                                                            //來源: 卡片
    uint8_t             LoyaltyCounter[2];      //17 LOYALTY_COUNTER 累積忠誠點 Unsigned 2 來源: 設備
    uint8_t             ResendDevID[3];         //18 RESEND_DEV_ID 重送設備編號 Unsigned 3 
                                                            //SP_ID(1 Byte): 如 122 即為 0x7A
                                                            //DEV_TYPE(4 Bits):  3：BV 如 BV 即為 0x30
                                                            //DEV_ID_NO(12 Bites): 如 1234 即為 0x04 0xD2
                                                            //組合 HEX: 0x7A 0x34 0xD2
                                                            //倒數2 Bytes LSB FIRST:  0x7A 0xD2 0x34
                                                            //範例參照 8.6
                                                            //來源: Mifare Sam 卡
    uint8_t             ResendSpID;             //19 RESEND_SP_ID 重送業者代碼 Unsigned 1  如 122 即為 0x7A  來源: 設備
    uint8_t             ResendLocID;            //20 RESEND_LOC_ID 重送場站代碼 Unsigned 1 來源: 設備
    uint8_t             ResendNewDevID[6];      //21 RESEND_NEW_DEV_ID 重送新設備編號 Unsigned 6 
                                                            //NEW_SP_ID(3 Bytes): 122 即為 0x00 0x00 0x7A
                                                            //NEW_DEV_TYPE(1 Byte): 3：BV  如 3 即為 0x03
                                                                //4096(含)之後開始編碼
                                                            //NEW_DEV_ID_NO (2 Bytes): 如 4096 即為 0x10 0x00
                                                            //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
                                                            //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                            //範例參照 8.6
                                                            //來源: Cpu Sam 卡
    uint8_t             ResendNewSpID[3];       //22 RESEND_NEW_SP_ID 重送新業者代碼 Unsigned 3
                                                            //NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
                                                            //以 LSB FIRST 傳送: 0x7A 0x00 0x00
                                                            //來源: 設備
    uint8_t             ResendNewLocID[2];      //23 RESEND_NEW_LOC_ID 重送新場站代碼 Unsigned 2
                                                            //NEW_SVCE_LOC_ID (2 Bytes): 如 1 即為 0x00 0x01
                                                            //以 LSB FIRST 傳送: 0x01 0x00
                                                            //來源: 設備
}ECCReSendLogBody;
#define TOTAL_ECC_RESEND_LOG_BODY_SIZE  64

typedef struct
{
    uint8_t             PurseVerNo;             //1 PURSE_VER_NO 卡片版本 Unsigned 1 代碼參照 8.1 來源: 設備
    uint8_t             CardTxnTypeID;          //2 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    uint8_t             CardTxnSubTypeID;       //3 CARD_TXN_SUBTYPE_ID 交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    uint8_t             DevID[3];               //4 DEV_ID 設備編號 Unsigned 3
                                                                //SP_ID(1 Byte): 如 122 即為 0x7A
                                                                //DEV_TYPE(4 Bits): 3：BV  如 BV 即為 0x3□
                                                                //DEV_ID_NO(12 Bites): 如 1234 即為 0x□4 0xD2  
                                                                //組合 HEX: 0x7A 0x34 0xD2
                                                                //倒數2 Bytes LSB FIRST: 0x7A 0xD2 0x34
                                                                //來源: Mifare Sam 卡
    uint8_t             SpID;                   //5 SP_ID 業者代碼 Unsigned 1 如 122 即為 0x7A 來源: 設備
    ECCLogDataTime      TxnTimeStamp;           //6 TXN_TIMESTAMP 交易時間 UnixTime 4 來源: 設備
    uint8_t             CardPhysicalID[7];      //7 CARD_PHYSICAL_ID 卡片晶片號碼 Unsigned 7 來源: 設備
    uint8_t             IIssuerID;              //8 ISSUER_ID 發卡單位 Unsigned 1 PVN <> 0 時:
                                                                    //來源: Cpu 卡
                                                                //PVN = 0 時:
                                                                    //2 (0x02)：悠遊卡公司
                                                                    //66 (0x42)：基隆交通卡
                                                                //來源: 設備
    uint8_t             SvceLocID;              //9 SVCE_LOC_ID 場站代碼 Unsigned 1 來源: 設備
    uint8_t             CardPhysicalIDLen;         //10 CARD_PHYSICAL_ID_LEN 晶片號碼長度 Unsigned 1
                                                                //如 4 bytes 即為 0x04
                                                                //如 7 bytes 即為 0x07
                                                                //來源: 卡片
    uint8_t             NewDevID[6];            //11 NEW_DEV_ID 新設備編號 Unsigned 6
                                                                //NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
                                                                //NEW_DEV_TYPE(1 Byte): 3：BV 如 3 即為 0x03
                                                                //4096(含)之後開始編碼
                                                                //NEW_DEV_ID_NO(2 Bytes): 如 4096 即為 0x10 0x00
                                                                //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
                                                                //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                                //來源: Cpu Sam 卡
    uint8_t             NewSpID[3];             //12 NEW_SP_ID 新業者代碼 Unsigned 3 
                                                                //NEW_SP_ID(3 Bytes): 
                                                                    //如 122 即為 0x00 0x00 0x7A
                                                                    //以 LSB FIRST 傳送: 0x7A 0x00 0x00
                                                                    //來源: 設備
    uint8_t             NewSvceLocID[2];        //13 NEW_SVCE_LOC_ID 新場站代碼 Unsigned 2 
                                                                    //NEW_SVCE_LOC_ID  (2 Bytes): 
                                                                        //如 1 即為 0x00 0x01
                                                                        //以 LSB FIRST 傳送: 0x01 0x00
                                                                        //來源: 設備
    uint8_t             NewCardTxnSubTypeID;    //14 NEW_CARD_TXN_SUBTYPE_ID 新交易次類別 Unsigned 1 依現況設備邏輯填入 代碼參照 8.3
    uint8_t             NewPersonalProfile;     //15 NEW_PERSONAL_PROFILE 新身份別 Unsigned 1
                                                                    //0x00: 一般
                                                                    //0x01: 敬老 1
                                                                    //0x02: 敬老 2
                                                                    //0x03: 愛心
                                                                    //0x04: 陪伴
                                                                    //0x05: 學生
                                                                    //0x08: 優待
                                                                    //來源: 卡片
    uint8_t             CardPurseID[8];         //16 CARD_PURSE_ID 錢包編號(外觀號碼) Pack 8
                                                                    //PVN <> 0 時: 如卡號 1234567890123456
                                                                                //即為 0x12 0x34 0x56 0x78 0x90 0x12 0x34 0x56
                                                                    //編碼參照 8.5
                                                                    //來源: Cpu 卡
                                                                    //PVN = 0 時:
                                                                    //固定填 8 Bytes 0x00
    uint8_t             CardCTCSeqNo[3];        //17 CARD_CTC_SEQ_NO 卡片處理序號 Unsigned Msb 3
                                                                    //PVN <> 0 時: 如 1 即為 0x00 0x00 0x01
                                                                        //以 MSB FIRST 傳送: 0x00 0x00 0x01
                                                                    //來源: Cpu 卡
                                                                    //PVN = 0 時: 固定填 3 Bytes 0x00
    uint8_t             CardBlockingReason;     //18 CARD_BLOCKING_REASON 卡片鎖卡原因 Unsigned 1 代碼參照 8.9 來源: 設備
    uint8_t             BlockingFile[20];       //19 BLOCKING_FILE 鎖卡名單檔名 Ascii 20 CARD_BLOCKING_REASON =1 時，
                                                                        //左靠右補空白，如'BLC00001.BIGΔΔΔΔΔΔΔΔ'
                                                                        //來源: 設備
                                                                    //CARD_BLOCKING_REASON<>1 時，
                                                                        //固定填 20 Bytes 0x20
    uint8_t             BlockingIDFlag;         //20 BLOCKING_ID_FLAG 鎖卡卡號 ID 旗標 Ascii 1
                                                                    //當 CARD_BLOCKING_REASON<>1 時，固定填 0x20
                                                                    //當 CARD_BLOCKING_REASON =1 且鎖卡名單檔為'.BIG'、'.SML'時，填BLOCKING_ID_FLAG 值
                                                                            //'M': MifareID 晶片號碼
                                                                            //'P': PurseID 錢包編號
                                                                    //當 CARD_BLOCKING_REASON =1 且鎖卡名單檔不為'.BIG'、'.SML'時，固定填'M'
    uint8_t             DevInfo[20];             //21 DEV_INFO 設備資訊 Ascii 20 BV、Tobu ：車號
                                                                //路邊計時器：車位號碼
                                                                //xAVM ：機號
                                                                //Dongle ：店號+機號
                                                                //其他設備：固定填 0x20
                                                                //左靠右補空白，如'123-ABΔΔΔΔΔΔΔΔΔΔΔΔΔΔ'
                                                                //來源: 設備
    uint8_t             ElectronicValue[3];     //22 ELECTRONIC_VALUE 交易後卡片金額 Signed 3 來源: 卡片
    uint8_t             NewCardTxnSeqNo[3];     //23 NEW_CARD_TXN_SEQ_NO 新交易後序號 Unsigned 3 來源: 卡片
    uint8_t             CreLogTxnSeqNo[3];      //24 CRELOG_TXN_SEQ_NO 票卡最末筆加值記錄  1.交易後序號 Unsigned 3 來源: 卡片
    ECCLogDataTime      CreLogTxnTimeStamp;     //25 CRELOG_TXN_TIMESTAMP 票卡最末筆加值記錄  2.交易時間 UnixTime 4 來源: 卡片
    uint8_t             CreLogTxnSubTypeID;     //26 CRELOG_TXN_SUBTYPE_ID 票卡最末筆加值記錄  3.交易次類別 Unsigned 1 來源: 卡片
    uint8_t             CreLogTxnAMT[3];        //27 CRELOG_TXN_AMT 票卡最末筆加 Signed 3 來源: 卡片值記錄  4.交易金額
    uint8_t             CreLogEV[3];            //28 CRELOG_EV 票卡最末筆加值記錄  5.交易後卡片金額   Signed 3 來源: 卡片
    uint8_t             CreLogDevID[6];         //29 CRELOG_DEV_ID 票卡最末筆加值記錄  6.設備編號 Unsigned 6 
                                                                //PVN=2，同 NEW_DEV_ID 如:
                                                                    //NEW_SP_ID=49(0x31)
                                                                    //NEW_DEV_TYPE=12(0x0C)
                                                                    //NEW_DEV_ID_NO=99(0x63)
                                                                    //組合 HEX: 00-00-31-0C-00-63
                                                                    //以 LSB FIRST 傳送: 63-00-0C-31-00-00
                                                                //PVN<>2，同 DEV_ID，右補 3 Bytes 0x00
                                                                    //同上例，組合 HEX: 31-C0-63
                                                                    //倒數2 Bytes LSB FIRST: 31-63-C0
                                                                    //右補 3 Bytes 0x00: 31-63-C0-00-00-00
                                                                //範例參照 8.6
                                                                //來源: 卡片
    uint8_t             AnotherEV[3];           //30 ANOTHER_EV 票卡另一個錢包餘額 Signed 3 來源: 卡片
    uint8_t             MifareSetPara;          //31 MIFARE_SET_PARA 票卡地區認證 旗標 Byte 1 後台 UNPACK 文字格式 CHAR(2) 來源: 卡片
    uint8_t             CpuSetPara;             //32 CPU_SET_PARA 票卡是否限制 通路使用旗標 Byte 1 後台 UNPACK 文字格式 CHAR(2) 來源: 卡片
    uint8_t             RFU[16];                //33 RFU 保留欄位 Byte 16 後台 UNPACK 文字格式 CHAR(32) 固定填 16 Bytes 0x00
}ECCLockCardLogBody;
#define TOTAL_ECC_LOCK_CARD_LOG_BODY_SIZE  134

typedef struct
{
    uint8_t             header[12];
    uint8_t             CardTxnTypeId;          //1 CARD_TXN_TYPE_ID 交易類別 Unsigned 1 代碼參照 8.2 來源: 設備
    uint8_t             DevID[3];               //2 DEV_ID 設備編號 Unsigned 3 SP_ID(1 Byte): 如 122 即為 0x7A
                                                                                //DEV_TYPE(4 Bits): 如 3 即為 0x3□
                                                                                //DEV_ID_NO(12 Bites): 如 100 即為 0x□0 0x64
                                                                                //組合 HEX: 0x7A 0x30 0x64
                                                                                //倒數2 Bytes LSB FIRST: 0x7A 0x64 0x30
                                                                                //來源: Mifare Sam 卡
    uint8_t             SpID;                   //3 SP_ID 業者代碼 Unsigned 1 SP_ID(1 Byte): 如 122 即為 0x7A 來源: 設備
    uint8_t             SvceLocID;              //4 SVCE_LOC_ID 場站代碼 Unsigned 1 來源: 設備
    uint8_t             NewDevID[6];            //5 NEW_DEV_ID 新設備編號 新增 Unsigned 6 NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
                                                                                        //NEW_DEV_TYPE(1 Byte): 3：BV 如 3 即為 0x03
                                                                                        //4096(含)之後開始編碼
                                                                                        //NEW_DEV_ID_NO(2 Bytes): 如 4096 即為 0x10 0x00
                                                                                        //組合 HEX: 0x00 0x00 0x7A 0x03 0x10 0x00
                                                                                        //以 LSB FIRST 傳送: 0x00 0x10 0x03 0x7A 0x00 0x00
                                                                                        //來源: Cpu Sam 卡
                                                                                        //二代卡設備未修改，以 SIS2 傳送時: 固定填 6 Bytes 0x00
    uint8_t             NewSpID[3];             //6 NEW_SP_ID 新業者代碼 新增 Unsigned 3 NEW_SP_ID(3 Bytes): 如 122 即為 0x00 0x00 0x7A
                                                            //以 LSB FIRST 傳送: 0x7A 0x00 0x00
                                                            //來源: 設備
    uint8_t             NewSvceLocID[2];        //7 NEW_SVCE_LOC_ID 新場站代碼 新增 Unsigned 2 NEW_SVCE_LOC_ID(2 Bytes): 如 1 即為 0x00 0x01
                                                                        //以 LSB FIRST 傳送: 0x01 0x00
                                                                        //來源: 設備
    uint8_t             BlockingFile[20];       //8 BLOCKING_FILE 鎖卡名單檔名 新增 Ascii 20 左靠右補空白，如'BLC00001.BIGΔΔΔΔΔΔΔΔ'   來源: 設備
    ECCLogDataTime      ReceiveDateTime;        //9 RECEIVE_DATETIME  收到鎖卡檔時間 新增 UnixTime 4 來源: 設備
    uint8_t             DevInfo[20];            //10 DEV_INFO 設備資訊 新增 Ascii 20  BV、Tobu ：車號
                                                                                    //路邊計時器：車位號碼
                                                                                    //xAVM ：機號
                                                                                    //Dongle ：店號
                                                                                    //其他設備：無
                                                                                    //左靠右補空白，如
                                                                                    //'123-ABΔΔΔΔΔΔΔΔΔΔΔΔΔΔ'
                                                                                    //來源: 設備
    uint8_t             tail[31];
}ECCBlkFeedbackLogBody;
#define TOTAL_ECC_BLK_FEEDBACK_LOG_BODY_SIZE  (12 + 61 + 31)

typedef struct
{
    ECCLogHeader        Header;

    uint8_t             LoyaltyCounter[2];      //1 LOYALTY_COUNTER 累積忠誠點 Unsigned 2 PVN <> 0 時: 固定填 0x00
                                                                                        //PVN = 0 時:
                                                                                        //來源: 卡片
    uint8_t             AgentNo[2];             //2 AGENT_NO 操作員代碼 Unsigned 2 如 Ascii 字元'1234'即為 0x31 0x32 0x33 0x34 轉 10 進制數值 1234 即為 0x04 0xD2
                                                            //以 LSB FIRST 傳送:0xD2 0x04
                                                            //來源: 設備
                                                            //此欄位 10 進制數值應與PPR_Reset/PPR_Reset_Online 一致
    uint8_t             BankCode;               //3 BANK_CODE 銀行代碼 Unsigned 1 PVN <> 0 時: 固定填 0x00
                                                            //PVN = 0 時:
                                                            //來源: 卡片
    uint8_t             LocProvider[3];         //4 LOC_PROVIDER 設備服務提供者 Unsigned 3 PVN <> 0 時: 同 NEW_SP_ID(3 Bytes): 如 131071 即為 0x01 0xFF 0xFF
                                                            //以 LSB FIRST 傳送: 0xFF 0xFF 0x01
                                                            //來源: 設備
                                                            //PVN = 0 時: 同 SP_ID(1 Bytes): 如 2 即為 0x02
                                                            //補足 3 Bytes 為 0x00 0x00 0x02
                                                            //以 LSB FIRST 傳送:0x02 0x00 0x00
                                                            //來源: 設備
    uint8_t             MachineID[20];//index 8 //5 MACHINE_ID 機器編號 Ascii 20 SP_ID(2 Bytes): '49' AVM='0',VAVM='1'(1 Byte) 流水編號(3 Bytes): '050’ 左靠右補空白，如 '490050ΔΔΔΔΔΔΔΔΔΔΔΔΔΔ' 來源: 設備
    uint8_t             CMID[20];               //6 CM_ID 鈔箱編號 Ascii 20 MACHINE_ID(6 Bytes) '-' 卸鈔日 yyyymmdd(8 Bytes)'-' 卸鈔序號(4 Bytes) 左靠右補空白，如'490050-20100101-1ΔΔΔ' 來源: 設備
    uint8_t             RFU1[16];               //7 RFU1 保留欄位(共用) Byte 16 PVN <> 0 時: 固定填 16 Bytes 0x00
                                                                                //PVN = 0 時: 固定填 16 Bytes 0x00
    uint8_t             RFU1Ver;                //8 RFU1_VER 欄位版本(共用) Unsigned 1 PVN <> 0 時:固定填 0x00
                                                                                    //PVN = 0 時: 固定填 0x00
    uint8_t             RFU2[16];               //9 RFU2 保留欄位(業者) Byte 16 PVN <> 0 時: 固定填 16 Bytes 0x00
                                                                                //PVN = 0 時: 固定填 16 Bytes 0x00
    uint8_t             RFU2Ver;                //10 RFU2_VER 欄位版本(業者) Unsigned 1 PVN <> 0 時: 固定填 0x00
                                                                                        //PVN = 0 時: 固定填 0x00
    uint8_t             TXNMode;                //11 TM(TXN Mode) 交易模式 Byte 1 PVN <> 0 時:來源: 設備
                                                                                //PVN = 0 時: 固定填 0x00
    uint8_t             TXNQualifier;           //12 TQ(TXN Qualifier) 交易屬性 Byte 1 PVN <> 0 時: 來源: 設備
                                                                                        //PVN = 0 時: 固定填 0x00
    uint8_t             SignKeyVer;             //13 SIGN_KEYVER SIGNATURE KEY 版本 Byte 1 PVN <> 0 時: 來源: Cpu 卡
                                                                                        //PVN = 0 時: 固定填 0x00
    uint8_t             SignValue[16];          //14 SIGN_VALUE SIGNATURE 值 Byte 16 PVN <> 0 時: 來源: Cpu 卡
                                                            //PVN = 0 時: 固定填 16 Bytes 0x00

    ECCLogTail          Tail;
}ECCAutoLoadLogBody;
#define TOTAL_ECC_AUTO_LOAD_LOG_BODY_SIZE  203

#pragma pack()
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
//001U
BOOL ECCDeductLogContainInit(ECCDeductLogBody* body, uint32_t deductValue, uint32_t AgentNo, uint32_t utcTime, BOOL cardAutoloadAvailable, ECCCmdDCAReadResponseSuccessData* DCAReadData, int DCAReadDataLen, ECCCmdEDCADeductResponseData* EDCADeductData, int EDCADeductDataLen);
char* ECCDeductLogGetFileName(void);

//012U
BOOL ECCReSendLogContainInit(ECCReSendLogBody* body, uint32_t deductValue, uint32_t AgentNo, uint32_t utcTime, BOOL cardAutoloadAvailable, ECCCmdDCAReadResponseSuccessData* DCAReadData, int DCAReadDataLen, ECCCmdEDCADeductResponseData* EDCADeductData, int EDCADeductDataLen);
char* ECCReSendLogGetFileName(void);

//034U
BOOL ECCLockCardLogContainInit(ECCLockCardLogBody* body, uint32_t lockType, uint8_t newSpID, uint8_t svceLocID, char* blkFileName, uint32_t utcTime, uint8_t* DCAReadData, ECCCmdLockCardResponseData* LockCardResponseData);
char* ECCLockCardLogGetFileName(void);

//052U
BOOL ECCBlkFeedbackLogContainInit(ECCBlkFeedbackLogBody* body, uint8_t newSpID, uint8_t svceLocID, char* blkFileName, uint32_t utcTime);
char* ECCBlkFeedbackLogGetFileName(void);

//002U
BOOL ECCAutoLoadLogContainInit(ECCAutoLoadLogBody* body, uint32_t AgentNo, ECCCmdDCAReadResponseSuccessData* DCAReadData, int DCAReadDataLen, ECCCmdEDCADeductResponseData* EDCADeductData, int EDCADeductDataLen, uint8_t newSpID, uint32_t utcTime);
char* ECCAutoLoadLogGetFileName(void);

#ifdef __cplusplus
}
#endif

#endif //__ECC_LOG_H__


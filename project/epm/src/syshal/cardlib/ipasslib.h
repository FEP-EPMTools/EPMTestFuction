/**************************************************************************//**
* @file     ipasslib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef _IPASS_LIB_
#define _IPASS_LIB_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _PC_ENV_
    #include "basetype.h"
#else
    #include "nuc970.h" 
#endif

#include "epmreader.h"
#include "halinterface.h"
#include "cardlogcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

    
//(returnInfo) ipass return value 
//#define CARD_MESSAGE_TYPE_IPASS_RETURN_BASE				 0x20    
#define CARD_MESSAGE_TYPE_IPASS_RETURN_IN_BLACK               (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 0)

#define CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_1               (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 1)
#define CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_2               (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 2)
#define CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LRC_ERROR           (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 3)
#define CARD_MESSAGE_TYPE_IPASS_ICD_RETURN_LEN_ERROR           (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 4)

#define CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_1         (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 7)
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_2         (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 8)
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LRC_ERROR     (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 9)
#define CARD_MESSAGE_TYPE_IPASS_EXECUTE_RETURN_LEN_ERROR     (CARD_MESSAGE_TYPE_IPASS_RETURN_BASE + 10)


//---- iPass ----
#define NO_ERR                      0
#define CMD_FORMAT_ERR              1        //命令格式錯誤
#define CMD_TYPE_ERR                2        //命令類別錯誤
#define CMD_BCC_ERR                 3        //BCC錯誤
#define CMD_TIMEOUT_ERR             4        //命令接收逾時
#define CMD_LENGTH_ERR              5        //資料長度錯誤
#define CMD_READERNOTAUTH_ERR       6        //讀卡機尚未認證
#define CMD_READERLOCK_ERR          7        //讀卡機已鎖
#define CMD_SAMNOTAUTH_ERR          8        //SAM卡尚未認證成功

#define CMD_BUFLENOVERFLOW_ERR      9        //資料接收長度溢位  (上面空行不可少, 會編譯失敗)

#define READER_STATE_ERR            51       //讀卡機狀態錯誤
#define READER_SN_ERR               52       //無法取得讀卡機編號

#define READER_AUTH_ERR             101      //讀卡機認證碼錯誤
#define READER_R1LEN_ERR            102      //R1長度錯誤
#define READER_AUTHCODELEN_ERR      103      //Auth Code長度錯誤

#define SAM_POWERON_ERR             151      //4.1 電源重置失敗
#define SAM_SELECTAID_ERR           152      //4.2 Select AID失敗
#define SAM_GETDATA_ERR             153      //4.3 GetData失敗
#define SAM_GETRANDOM_ERR           154      //4.4 GetRandom失敗
#define SAM_AUTH_ERR                155      //4.5 Authenticate失敗
#define SAM_GETSESSIONKEY_ERR       156      //4.6 Get SessionKey失敗
#define SAM_NOCARD_ERR              157      //4.7 無SAM卡
#define SAM_GETTAC_ERR              158
#define SAM_GETMIFAREKEY_ERR        159
#define CARD_POLLINGNOCARD_ERR      401    
#define CARD_POLLINGMULTICARD_ERR   402 
//---- iPass ----

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL IPASSLibProcess(uint16_t* returnInfo, uint16_t* returnCode, uint16_t targetDeduct, tsreaderDepositResultCallback callback, uint8_t* cnData, uint8_t* dataTime, uint8_t* machineNo);
void IPASSSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue, uint16_t currentTargetDeduct, uint8_t* readerId, char* dataStr, char* timeStr);
void IPASSSaveFilePure(uint16_t currentTargetDeduct, uint8_t* readerId, char* dataStr, char* timeStr);
#ifdef __cplusplus
}
#endif

#endif //_IPASS_LIB_

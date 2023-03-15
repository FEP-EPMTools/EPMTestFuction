/**************************************************************************//**
* @file     ecclib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef _ECC_LIB_
#define _ECC_LIB_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _PC_ENV_
    #include "basetype.h"
#else
    #include <time.h>
    #include "nuc970.h"
    #include "sys.h"
    #include "rtc.h"
    
    #include "fepconfig.h"
    #include "sflashrecord.h"
    #include "fileagent.h"
    #include "timelib.h"
    #include "meterdata.h"

    #define ECC_LOG_FILE_SAVE_POSITION          FILE_AGENT_STORAGE_TYPE_AUTO
    #define ECC_LOG_FILE_DIR                    "1:"  //??? ???   

#endif

#include "epmreader.h"
#include "halinterface.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

//(returnInfo) ecc return value
//#define CARD_MESSAGE_TYPE_ECC_RETURN_BASE				 0x30   
#define CARD_MESSAGE_TYPE_ECC_RETURN_LEN_ERROR               (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 0)
#define CARD_MESSAGE_TYPE_ECC_RETURN_LRC_ERROR               (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 1)
#define CARD_MESSAGE_TYPE_ECC_RETURN_CANT_USE_ERROR          (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 2) //卡片不適用
#define CARD_MESSAGE_TYPE_ECC_RETURN_FAILURE_ERROR           (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 3) //卡片失效  
#define CARD_MESSAGE_TYPE_ECC_RETURN_READ_ERROR               (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 4) // need check 0x640E(餘額異常) 0x6103(CPD檢查異常) or other
#define CARD_MESSAGE_TYPE_ECC_RETURN_INSUFFICIENT_MONEY_ERROR (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 5) // 餘額不足
#define CARD_MESSAGE_TYPE_ECC_RETURN_PPRRESET_ERROR             (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 6) // pprreset error

#define CARD_MESSAGE_TYPE_ECC_RETURN_SIGN_ON_REQUEST_ERROR              (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 8) // Sign On Request error
#define CARD_MESSAGE_TYPE_ECC_RETURN_SIGN_ON_CONFIRM_ERROR              (CARD_MESSAGE_TYPE_ECC_RETURN_BASE + 9) // Sign On Confirm error
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
void ECCLibInit(void);
uint16_t ECCPPRReset(uint16_t* returnInfo, uint16_t* returnCode, uint32_t utcTime, uint32_t epmUTCTime, BOOL SignOnMode);
BOOL ECCLibProcess(uint16_t* returnInfo, uint16_t* returnCode, uint16_t targetDeduct, tsreaderDepositResultCallback callback, uint8_t* cnData, uint32_t utcTime, uint8_t* machineNo);
void ECCSaveFile(uint16_t currentTargetDeduct, time_t epmUTCTime);
void ECCSaveFilePure(uint16_t currentTargetDeduct, time_t epmUTCTime);
    
    
#ifdef __cplusplus
}
#endif

#endif //_IPASS_LIB_

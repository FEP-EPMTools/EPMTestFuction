/**************************************************************************//**
* @file     epmreader.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef _EPM_READER
#define _EPM_READER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _PC_ENV_
    #include "basetype.h"
#else
    #include "nuc970.h"
#endif

#include "halinterface.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define CARD_TYPE_ID_NONE                           0x00
#define CARD_TYPE_ID_IPASS                          0x01
#define CARD_TYPE_ID_ECC                            0x02

#define CARD_MESSAGE_TYPE_CN                        0x01  
#define CARD_MESSAGE_TYPE_TIME                      0x02
#define CARD_MESSAGE_TYPE_API_VER_NO                0x03
#define CARD_MESSAGE_TYPE_READER_ID                 0x04

//(returnCode)    
#define CARD_MESSAGE_CODE_NO_USE                    0xff

//(returnInfo) common return value 
#define CARD_MESSAGE_RETURN_TIMEOUT                     0x01
#define CARD_MESSAGE_RETURN_SUCCESS                     0x02
#define CARD_MESSAGE_RETURN_PARSER_ERROR                0x03
#define CARD_MESSAGE_RETURN_SEND_ERROR                  0x04
#define CARD_MESSAGE_RETURN_OTHER_ERROR                 0x05


#define CARD_MESSAGE_TYPE_CN_RETURN_LEN_1                0x11
#define CARD_MESSAGE_TYPE_CN_RETURN_NOT_SUPPORT_CARDTYPE 0x12
#define CARD_MESSAGE_TYPE_CN_RETURN_UNKNOWN_CARDTYPE     0x13

#define CARD_MESSAGE_TYPE_IPASS_RETURN_BASE				 0x20
#define CARD_MESSAGE_TYPE_ECC_RETURN_BASE				 0x30  


#define EPM_READER_CTRL_ID_GUI      0 
//#define EPM_READER_CTRL_ID_MODEM    1 
//#define EPM_READER_CTRL_NUMBER      (EPM_READER_CTRL_ID_MODEM + 1)

//typedef struct
//{
//   BOOL         onOffFlag;
//}EpmReaderCtrl;



/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL EPMReaderInit(void);
BOOL EPMReaderSetPower(uint8_t id, BOOL flag);
uint8_t EPMReaderCheckReader(void);
BOOL EPMReaderBreakCheckReader(void);

uint32_t EPMReaderSendCmd(uint8_t buff[], int len);
int EPMReaderReceiveCmd(uint32_t waitTime, uint8_t** receiveData, uint16_t* dataLen);
uint16_t EPMReaderParserMessage(uint8_t msgType, uint8_t* msgBuff, uint16_t msgLen, uint16_t* returnCode);

BOOL EPMReaderProcess(uint16_t targetDeduct, tsreaderDepositResultCallback callback);
BOOL EPMReaderProcessCN(tsreaderCNResultCallback callback);
BOOL EPMReaderGetBootedStatus(void);
BOOL EPMReaderGetBootedStatusEx(void);

BOOL EPMReaderSignOnProcess(void);
void EPMReaderSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue);
void EPMReaderSaveFilePure(void);
void EPMReaderFlushBuffer(void);
char EPMReaderLRC(uint8_t *array,int start,int len);
void EPMReaderGetVersion(char* ReaderVerBuf);


#ifdef __cplusplus
}
#endif

#endif //_EPM_READER

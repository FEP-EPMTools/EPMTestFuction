/**************************************************************************//**
* @file     quentelmodemlib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __QUENTEL_MODEM_LIB_H__
#define __QUENTEL_MODEM_LIB_H__

#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "stdint.h"   
#include "nuc970.h"

#include "atcmdparser.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
    
    

//------------
#define MODEM_STAGE_CMD_REG_ACTION_NUM 4
typedef enum{
    DIALUP_IDLE_STAGE_INDEX            = 0x00,
    DIALUP_FLOW_CTL_ON_STAGE_INDEX     = 0x01,
    DIALUP_ECHO_OFF_STAGE_INDEX        = 0x02,
    DIALUP_EXIST_STAGE_INDEX           = 0x03,
    DIALUP_REGISTER_STAGE_INDEX        = 0x04,
    DIALUP_GATT_STAGE_INDEX            = 0x05,
    DIALUP_GREG_STAGE_INDEX            = 0x06,
    DIALUP_CONTEXT_SETTING_STAGE_INDEX = 0x07,
    DIALUP_CONNECTING_STAGE_INDEX      = 0x08,
    DIALUP_DISCONNECT_STAGE_INDEX      = 0x09,
    DIALUP_QUERY_CONNECTED_STAGE_INDEX = 0x0a,
    DIALUP_PIN_STAGE_INDEX             = 0x0b,
    DIALUP_CONNECTED_STAGE_INDEX       = 0x0f,
    
    FTP_IDLE_STAGE_INDEX                = 0x10,
    FTP_SET_PDP_STAGE_INDEX             = 0x11,
    FTP_SET_USER_INFO_STAGE_INDEX       = 0x12,
    FTP_SET_FILE_TYPE_STAGE_INDEX       = 0x13,
    FTP_SET_TRANSFER_MODE_STAGE_INDEX   = 0x14,
    FTP_SET_TIMEOUT_STAGE_INDEX         = 0x15,
    FTP_CONNECTING_STAGE_INDEX          = 0x16,
    FTP_DISCONNECT_STAGE_INDEX          = 0x17,
    FTP_QUERY_CONNECTED_STAGE_INDEX     = 0x18,
    FTP_CONNECTED_STAGE_INDEX           = 0x1f,
            
    MODEM_NULL_STAGE_INDEX            = 0xff,
}ModemStageIndex;
//-------------------------
typedef struct
{
    CmdReq      cmdReg;
    ModemStageIndex	nextStage;    
}ModemCmdRegAction;
    
typedef struct
{
    ModemStageIndex         stageIndex;
    uint8_t*                cmd;
    ModemCmdRegAction*      cmdRegAction;
    uint32_t                checkIntervalTime;
    uint32_t                waitTime;
    uint8_t                 retryTimes;
}ModemStageItem;


//------------
typedef struct
{
    CmdReq      cmdReg;
    BOOL        returnValue;    
}CmdRegAction;

typedef struct
{
    uint8_t*                cmd;
    CmdRegAction*           cmdRegAction;
    uint32_t                checkIntervalTime;
    uint32_t                waitTime;
}ModemCmdItem;



/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/   
BOOL QModemLibInit(UINT32 baudRate);
BOOL QModemDialupProcess(void);
void QModemDialupStart(void);


BOOL QModemFtpClientProcess(void);
void QModemFtpClientStart(void);

void QModemTotalStop(void);

BOOL FtpClientSendFile(BOOL needChdirFlag, char* preDirName, char* dirName, char* fileName, uint8_t* buff, int len);
BOOL FtpClientGetFile(char* dirName, char* fileName);
BOOL FtpClientGetFileEx(char* dirName, char* fileName);
BOOL FtpClientGetFileOTA(char* dirName, char* fileName);
BOOL WebPostMessage(char* url, uint8_t* buff);
BOOL FtpClientClose(void);
char* FtpQueryCsq(void);
BOOL QModemATCmdTest(void);

ModemStageIndex QModemDialupStageIndex(void);
ModemStageIndex QModemFtpStageIndex(void);
BOOL QModemSetHighSpeed(void);
BOOL QModemQueryIPAddress(char* ipBuffer);
BOOL QModemGetVer(char* reStr);
BOOL QModemGetSIMNumber(char* SIMStr);

INT QModemIoctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1);

BOOL QModemSetSleep(void);
void QModemSetQurccfg(void);
void QModemSetTestCmd(void);

BOOL QModemQuerySIMInitStatus(int* para);
BOOL QModemQueryNTP(UINT32* rYear,UINT32* rMonth,UINT32* rDay,UINT32* rHour,UINT32* rMinute,UINT32* rSecond);
BOOL QModemTerminal(char* CmdString,char* FBCmdStr,int FBCmdSize,int* retlen,int waitime);


BOOL QModemSetPower(BOOL flag);
#ifdef __cplusplus
}
#endif

#endif /* __QUENTEL_MODEM_LIB_H__ */


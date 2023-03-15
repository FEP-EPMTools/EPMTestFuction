/**************************************************************************//**
* @file     quentelmodemlib.c
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
#include "nuc970.h"
#include "sys.h"

#include "atcmdparser.h"
#include "quentelmodemlib.h"
#include "interface.h"
#include "loglib.h"
#include "cJSON.h"
#include "timelib.h"
#include "tarifflib.h"
#include "dataprocesslib.h"
#include "fileagent.h"
#include "tarifflib.h"
#include "nuc970.h"
#include "batterydrv.h"
#include "epddrv.h"
#include "leddrv.h"
#if (ENABLE_BURNIN_TESTER)
#include "burnintester.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define QUENTEL_MODEM_UART      UART_4_INTERFACE_INDEX
#define FTP_UART_WRITE_SIZE     (1*1024)
#define MODEM_CMD_RETRY_TIMES   5

#define X_POS_MSG 550
#define Y_POS_MSG 354

typedef enum{
    MODEM_RETURN_OK = 0x01,
    MODEM_RETURN_ERROR = 0x02,
    MODEM_RETURN_BREAK = 0x03
}ModemReturnValue;

typedef enum{
    CMD_RETURN_OK = 0x01,
    CMD_RETURN_PROCESSING = 0x02,
    CMD_RETURN_BREAK = 0x03,
    CMD_RETURN_ERROR = 0x04,
}CmdReturnValue;

typedef enum{
    CMD_ACTION_OK = 0x01,
    CMD_ACTION_IGNORE = 0x02,
    CMD_ACTION_ERROR = 0x03,
}CmdActionValue;
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;
static BOOL cmdActionBreakFlag = FALSE;
static BOOL firstTimeCmd = TRUE;
static BOOL firstTimeFtpCmd = TRUE;

//------------
static ModemCmdRegAction nullModemCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}, 
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

//-----------------------------------Dialup-----------------------------------------------
static ModemCmdRegAction flowCtlOnCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] = {{CMD_REQ_OK, DIALUP_ECHO_OFF_STAGE_INDEX}, 
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};                                                                    
                                                                    
static ModemCmdRegAction echooffCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] = {{CMD_REQ_OK, DIALUP_EXIST_STAGE_INDEX}, 
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction existCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_EXIST_OK, DIALUP_REGISTER_STAGE_INDEX}, 
                                                                    {CMD_REQ_ERROR, DIALUP_EXIST_STAGE_INDEX},
                                                                    {CMD_REQ_CME_ERROR, DIALUP_EXIST_STAGE_INDEX},
                                                                    {CMD_REQ_NEED_PIN_OK, DIALUP_PIN_STAGE_INDEX}};

static ModemCmdRegAction pinCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, DIALUP_EXIST_STAGE_INDEX}, 
                                                                    {CMD_REQ_CME_ERROR, DIALUP_EXIST_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction regCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_REG_OK, DIALUP_GATT_STAGE_INDEX}, 
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction gattCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_GATT_OK, DIALUP_GREG_STAGE_INDEX}, 
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction gregCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_GREG_OK, DIALUP_CONTEXT_SETTING_STAGE_INDEX}, 
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction contextSettingCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, DIALUP_CONNECTING_STAGE_INDEX}, 
                                                                            {CMD_REQ_ERROR, DIALUP_DISCONNECT_STAGE_INDEX},
                                                                            {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                            {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction connectingCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, DIALUP_QUERY_CONNECTED_STAGE_INDEX}, 
                                                                        {CMD_REQ_ERROR, DIALUP_DISCONNECT_STAGE_INDEX},
                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction disconnectCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, DIALUP_CONTEXT_SETTING_STAGE_INDEX}, 
                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction queryConnectCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_QUERY_CONNECTED_OK, DIALUP_CONNECTED_STAGE_INDEX}, 
                                                                        {CMD_REQ_ERROR, DIALUP_EXIST_STAGE_INDEX},
                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

//static ModemCmdRegAction connectedCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_ERROR, /*DIALUP_EXIST_STAGE_INDEX*/DIALUP_CONNECTED_STAGE_INDEX}, 
//                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
//                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
//                                                                        {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemStageItem dialupStageItem[] = {{DIALUP_IDLE_STAGE_INDEX,           (uint8_t*)NULL,                             nullModemCmdRegAction,           500, 2000, 30},
                                            {DIALUP_FLOW_CTL_ON_STAGE_INDEX,    (uint8_t*)CHECK_FLOW_CTL_ON_CMD,           flowCtlOnCmdRegAction,        500, 10*1000, 30},
                                            {DIALUP_ECHO_OFF_STAGE_INDEX,       (uint8_t*)CHECK_ECHO_OFF_CMD,               echooffCmdRegAction,        500, 2000, 30},
                                            {DIALUP_EXIST_STAGE_INDEX,          (uint8_t*)CHECK_EXIST_CMD,                  existCmdRegAction,          1000, 2000, 30},
                                            {DIALUP_PIN_STAGE_INDEX,            (uint8_t*)CHECK_PIN_CMD,                    pinCmdRegAction,            2000, 2000,  2},
                                            {DIALUP_REGISTER_STAGE_INDEX,       (uint8_t*)CHECK_REG_CMD,                    regCmdRegAction,            500, 10*1000, 30},
                                            {DIALUP_GATT_STAGE_INDEX,           (uint8_t*)CHECK_GATT_CMD,                   gattCmdRegAction,           500, 2000, 30},
                                            {DIALUP_GREG_STAGE_INDEX,           (uint8_t*)CHECK_GREG_CMD,                   gregCmdRegAction,           500, 5000, 30},
                                            {DIALUP_CONTEXT_SETTING_STAGE_INDEX, (uint8_t*)CHECK_CONTEXT_SETTING_CMD,       contextSettingCmdRegAction, 500, 2000, 30},
                                            {DIALUP_CONNECTING_STAGE_INDEX,     (uint8_t*)CHECK_CONNECTING_CMD,             connectingCmdRegAction,     500, 15*1000, 30},
                                            {DIALUP_DISCONNECT_STAGE_INDEX,     (uint8_t*)CHECK_DISCONNECT_CMD,             disconnectCmdRegAction,     500, 5000, 30},
                                            {DIALUP_QUERY_CONNECTED_STAGE_INDEX,(uint8_t*)CHECK_QUERY_CONNECTED_CMD,        queryConnectCmdRegAction,   500, 10*1000, 30},
                                            
                                            
                                            //{DIALUP_CONNECTED_STAGE_INDEX,      (uint8_t*)NULL,                             connectedCmdRegAction,3000, 3000},
                                            {DIALUP_CONNECTED_STAGE_INDEX,      (uint8_t*)NULL,                             nullModemCmdRegAction,3000, 3000, 30},
                                            {MODEM_NULL_STAGE_INDEX,           (uint8_t*)NULL,                             nullModemCmdRegAction,           100, 0, 30}
                                            }; 

//--------------------------------------------   FTP  -----------------------------------------
static ModemCmdRegAction ftpSetPDPCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, FTP_SET_USER_INFO_STAGE_INDEX}, 
                                                                                    {CMD_REQ_CME_ERROR, FTP_DISCONNECT_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction ftpSetUserInfoCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, FTP_SET_FILE_TYPE_STAGE_INDEX}, 
                                                                                    {CMD_REQ_CME_ERROR, FTP_DISCONNECT_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction ftpSetFileTypeCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, FTP_SET_TRANSFER_MODE_STAGE_INDEX}, 
                                                                                    {CMD_REQ_CME_ERROR, FTP_DISCONNECT_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction ftpSetTransferModeCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, FTP_SET_TIMEOUT_STAGE_INDEX}, 
                                                                                    {CMD_REQ_CME_ERROR, FTP_DISCONNECT_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction ftpSetTimeoutCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, FTP_CONNECTING_STAGE_INDEX}, 
                                                                                    {CMD_REQ_CME_ERROR, FTP_DISCONNECT_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction ftpConnectingCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {//{CMD_REQ_OK, FTP_QUERY_CONNECTED_STAGE_INDEX},
                                                                                    {CMD_REQ_QUERY_FTP_CONNECTED_OK, FTP_CONNECTED_STAGE_INDEX},     
                                                                                    {CMD_REQ_ERROR, FTP_SET_PDP_STAGE_INDEX},
                                                                                    {CMD_REQ_CME_ERROR, FTP_DISCONNECT_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemCmdRegAction ftpDisconnectCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_OK, FTP_SET_PDP_STAGE_INDEX}, 
                                                                                    {CMD_REQ_ERROR, FTP_SET_PDP_STAGE_INDEX},
                                                                                    {CMD_REQ_CME_ERROR, FTP_SET_PDP_STAGE_INDEX},
                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

//static ModemCmdRegAction ftpQueryConnectedPDPCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_QUERY_FTP_CONNECTED_OK, FTP_CONNECTED_STAGE_INDEX}, 
//                                                                                    {CMD_REQ_ERROR, FTP_SET_PDP_STAGE_INDEX},
//                                                                                    {CMD_REQ_CME_ERROR, FTP_SET_PDP_STAGE_INDEX},
//                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

//static ModemCmdRegAction ftpConnectedPDPCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =   {{CMD_REQ_FTP_DISCONNECT, FTP_SET_PDP_STAGE_INDEX}, 
//                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
//                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX},
//                                                                                    {CMD_REQ_NULL, MODEM_NULL_STAGE_INDEX}};

static ModemStageItem ftpClientStageItem[] = {{FTP_IDLE_STAGE_INDEX,                (uint8_t*)NULL,                                     nullModemCmdRegAction,          500, 5000, 30},
                                                {FTP_SET_PDP_STAGE_INDEX,           (uint8_t*)FTP_SET_PDP_CMD,                          ftpSetPDPCmdRegAction,          500, 5000, 30},
                                                {FTP_SET_USER_INFO_STAGE_INDEX,     (uint8_t*)FTP_SET_USER_INFO_CMD(FTP_ID, FTP_PASSWD),ftpSetUserInfoCmdRegAction,     500, 5000, 30},
                                                {FTP_SET_FILE_TYPE_STAGE_INDEX,     (uint8_t*)FTP_SET_FILE_TYPE_CMD,                    ftpSetFileTypeCmdRegAction,     500, 5000, 30},
                                                {FTP_SET_TRANSFER_MODE_STAGE_INDEX, (uint8_t*)FTP_SET_TRANSFER_MODE_CMD,                ftpSetTransferModeCmdRegAction, 500, 5000, 30},
                                                {FTP_SET_TIMEOUT_STAGE_INDEX,       (uint8_t*)FTP_SET_TIMEOUT_CMD,                      ftpSetTimeoutCmdRegAction,      500, 5000, 30},
                                                {FTP_CONNECTING_STAGE_INDEX,        (uint8_t*)FTP_CONNECTING_CMD(FTP_ADDRESS, FTP_PORT),  ftpConnectingCmdRegAction,      500, 5000, 30},
                                                {FTP_DISCONNECT_STAGE_INDEX,        (uint8_t*)FTP_DISCONNECT_CMD,                       ftpDisconnectCmdRegAction,      500, 5000, 30},
                                                //{FTP_QUERY_CONNECTED_STAGE_INDEX,   (uint8_t*)NULL,                                     ftpQueryConnectedPDPCmdRegAction, 500, 10000, 30},
                                                
                                                //{FTP_CONNECTED_STAGE_INDEX,         (uint8_t*)NULL,                                   ftpConnectedPDPCmdRegAction,           3000, 3000},
                                                {FTP_CONNECTED_STAGE_INDEX,         (uint8_t*)NULL,                                     nullModemCmdRegAction,           3000, 5000, 30},
                                                {MODEM_NULL_STAGE_INDEX,            (uint8_t*)NULL,                                     nullModemCmdRegAction,           400, 0, 30}
                                            }; 
//=================================================================================================
//static CmdRegAction nullCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_NULL, FALSE}};
//--------------------------------------------   WEB Post  -----------------------------------------      
static CmdRegAction webSetUrlInfoCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_CONNECT_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem webSetUrlInfoCmdItem = {(uint8_t*)NULL,    webSetUrlInfoCmdRegAction, 500, 5000};    

static CmdRegAction webSetUrlContainCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem webSetUrlContainCmdItem = {(uint8_t*)NULL,    webSetUrlContainCmdRegAction, 500, 5000};  

static CmdRegAction webSetPostInfoCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_CONNECT_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem webSetPostInfoCmdItem = {(uint8_t*)NULL,    webSetPostInfoCmdRegAction, 500, 10000};  

static CmdRegAction webSetPostContainCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_WEB_POST_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem webSetPostContainCmdItem = {(uint8_t*)NULL,    webSetPostContainCmdRegAction, 500, 10000};  

static CmdRegAction webGetPostContainCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_WEB_POST_GET_DATA, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem webGetPostContainCmdItem = {(uint8_t*)NULL,    webGetPostContainCmdRegAction, 500, 5000}; 

//--------------------------------------------   FTP Send  --------------------------------------
static CmdRegAction ftpSendInitCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_CONNECT_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem ftpSendInitCmdItem = {(uint8_t*)NULL,    ftpSendInitCmdRegAction, 1000, 10000};

static CmdRegAction ftpSendDataCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_FTP_SEND_DATA_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_FTP_SEND_DATA_TIMEOUT, FALSE}, 
                                                                                    {CMD_REQ_OK, TRUE}};
static ModemCmdItem ftpSendDataCmdItem = {(uint8_t*)NULL,    ftpSendDataCmdRegAction, 1000, 60000};

static CmdRegAction ftpChDirCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_FTP_CHANGE_DIR_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_FTP_CHANGE_DIR_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem ftpChDirCmdItem = {(uint8_t*)NULL,    ftpChDirCmdRegAction, 500, 10000};

static CmdRegAction ftpMkDirCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_FTP_MAKE_DIR_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    //{CMD_REQ_FTP_MAKE_DIR_ERROR, FALSE},//不判斷  因為已經有目錄也是會RETURN這個 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem ftpMkDirCmdItem = {(uint8_t*)NULL,    ftpMkDirCmdRegAction, 500, 10000};

//static CmdRegAction ftpGetDirCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_FTP_GET_DIR_OK, TRUE}, 
//                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
//                                                                                    {CMD_REQ_FTP_GET_DIR_ERROR, FALSE}, 
//                                                                                    {CMD_REQ_NULL, FALSE}};
//static ModemCmdItem ftpGetDirCmdItem = {(uint8_t*)NULL,    ftpGetDirCmdRegAction, 500, 10000};

//+QFTPCWD: 627,550
static CmdRegAction ftpGetDataCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_FTP_GET_DATA_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem ftpGetDataCmdItem = {(uint8_t*)NULL,    ftpGetDataCmdRegAction, 1000, 15000};
//+CME ERROR: 627

static CmdRegAction ftpCloseCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_FTP_DISCONNECT, TRUE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem ftpCloseCmdItem = {(uint8_t*)FTP_DISCONNECT_CMD,    ftpCloseCmdRegAction, 500, 5000};

//--------------------------------------------   Network  --------------------------------------
static CmdRegAction networkQueryCsqCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_QUERY_CSQ_OK, TRUE}, 
                                                                                    {CMD_REQ_CME_ERROR, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem networkQueryCsqCmdItem = {(uint8_t*)FTP_QUERY_CSQ_CMD,    networkQueryCsqCmdRegAction, 500, 5000};

static CmdRegAction networkAtTestCmdRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] =    {{CMD_REQ_OK, TRUE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}, 
                                                                                    {CMD_REQ_NULL, FALSE}};
static ModemCmdItem networkAtTestCmdItem = {(uint8_t*)CHECK_AT_TEST_CMD,    networkAtTestCmdRegAction, 500, 1000};
static ModemCmdItem networkSetHighSpeedCmdItem = {(uint8_t*)SET_IPR_CMD,    networkAtTestCmdRegAction, 500, 2000};
static ModemCmdItem readVersionCmdItem = {(uint8_t*)CHECK_AT_TEST_VER_CMD,    networkAtTestCmdRegAction, 500, 2000}; //CHECK_AT_TEST_VER_CMD
static ModemCmdItem readSIMNumberCmdItem = {(uint8_t*)CHECK_SIM_NUM_CMD,    networkAtTestCmdRegAction, 500, 2000}; //CHECK_AT_TEST_VER_CMD
static ModemCmdItem sleepCmdItem = {(uint8_t*)SLEEP_CMD,    networkAtTestCmdRegAction, 500, 2000}; 
static ModemCmdItem qurccfgCmdItem = {(uint8_t*)QURCCFG_CMD,    networkAtTestCmdRegAction, 500, 2000}; 


static CmdRegAction networkQueryIPAddressRegAction[MODEM_STAGE_CMD_REG_ACTION_NUM] = {{CMD_REQ_OK, TRUE}, 
                                                                                      {CMD_REQ_NULL, FALSE}, 
                                                                                      {CMD_REQ_NULL, FALSE}, 
                                                                                      {CMD_REQ_NULL, FALSE}};
static ModemCmdItem networkQueryIPAddressItem = {(uint8_t*)CHECK_QUERY_CONNECTED_CMD, networkQueryIPAddressRegAction, 500, 5000};

static ModemCmdItem networkQuerySIMInitStatusItem = {(uint8_t*)QUERY_SIM_INIT_STATUS_CMD, networkQueryIPAddressRegAction, 300, 1500};



static ModemCmdItem networkNTPEnableWriteItem = {(uint8_t*)AUTOMATIC_TIME_ZONE_UPDATE_W_CMD, networkQueryIPAddressRegAction, 300, 1500};

static ModemCmdItem networkNTPEnableReadItem = {(uint8_t*)AUTOMATIC_TIME_ZONE_UPDATE_R_CMD, networkQueryIPAddressRegAction, 300, 1500};


static ModemCmdItem networkQueryNTPWriteItem = {(uint8_t*)QUERY_TIME_ZONE_W_CMD, networkQueryIPAddressRegAction, 300, 1500};

static ModemCmdItem networkQueryNTPReadItem = {(uint8_t*)QUERY_TIME_ZONE_R_CMD, networkQueryIPAddressRegAction, 300, 1500};

static ModemCmdItem networkQueryNTPItem = {(uint8_t*)QUERY_NETWORK_TIME_CMD, networkQueryIPAddressRegAction, 300, 1500};

unsigned char readBuff[4096];

static ModemStageIndex currentDialupStageIndex = DIALUP_IDLE_STAGE_INDEX;
static ModemStageIndex currentFtpStageIndex = FTP_IDLE_STAGE_INDEX;

static uint8_t modemColorAllGreen[8]= {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN};
static uint8_t modemColorAllRed[8] = {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_RED, LIGHT_COLOR_RED};
//static uint8_t modemColorAllGreen[8]= {LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN};
//static uint8_t modemColorAllRed[8] = {LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED};
static uint8_t modemColorAllOff[8] = {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};
static BOOL changeLEDColorFlag = TRUE;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
//#if(ENABLE_MODEM_CMD_DEBUG)
static void printfBuffData(char* str, uint8_t* data, int len)
{
    int i;
    sysprintf("\r\n %s: len = %d...\r\n   -%02d:-> ", str, len, len);
    if(len>72)
        len = 72;
    
    for(i = 0; i<len; i++)
    { 
        if((data[i]>=0x20)&&(data[i]<=0x7e))
            sysprintf("[%02d]:0x%02x(%c)\r\n", i, (unsigned char)data[i], (unsigned char)data[i]);
        else
            sysprintf("[%02d]:0x%02x\r\n", i, (unsigned char)data[i]);
    }
    sysprintf("\r\n");
    
}
//#endif
static char* parserString(char* str, char* startStr, char* endStr, int* destLen)
{
    int i;
    char* startIndex = NULL;
    *destLen = 0;
    for (i = 0; i < (strlen(str) - strlen(endStr))+1; i++)
    {
        if(startIndex == 0)
        {
            if(memcmp(str+i, startStr, strlen(startStr)) == 0)
            {
                startIndex = str+i + strlen(startStr);
            }
        }
        else
        {
            if(memcmp(str+i, endStr, strlen(endStr)) == 0)
            {
                *destLen = str+i - startIndex;
            }
        }
    }
    return startIndex;
}

static INT32 QModemWrite(PUINT8 pucBuf, UINT32 uLen)
{
    //sysprintf("QModemWrite %d...\r\n", uLen); 
    if(pUartInterface == NULL)
        return 0 ;   
    //#if(ENABLE_MODEM_CMD_DEBUG)
    if(ATCmdGetReceiveDebugFlag())
    {
        printfBuffData("QModemWrite", pucBuf, uLen);
    }
    //#endif
    return pUartInterface->writeFunc(pucBuf, uLen);
}
static INT32 QModemRead(PUINT8 pucBuf, UINT32 uLen)
{
    if(pUartInterface == NULL)
        return 0 ;
    return pUartInterface->readFunc(pucBuf, uLen);
}

static INT32 QModemReadEx(PUINT8 pucBuf, UINT32 uLen,UINT32 readtime)
{
    int index = 0;
    int counter = 0;
    INT32 reVal;
    if(pUartInterface == NULL)
        return 0 ;
    vTaskDelay(10/portTICK_RATE_MS);
    memset(pucBuf, 0x0, uLen);
    while(counter < (readtime/10))
    {
        vTaskDelay(10/portTICK_RATE_MS);
        reVal = pUartInterface->readFunc(pucBuf + index, uLen-index);
        if(reVal > 0)
            index = index + reVal;
        counter++;
    }
    return index;
}
#warning nee implement QModem power
#if(1)
//static BOOL QModemSetPower(BOOL flag)
BOOL QModemSetPower(BOOL flag)
{
    if(pUartInterface == NULL)
        return FALSE ;
    return pUartInterface->setPowerFunc(flag);
}
#endif
//static INT QModemIoctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
INT QModemIoctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    if(pUartInterface == NULL)
        return 0 ;
    return pUartInterface->ioctlFunc(uCmd, uArg0, uArg1);
}

void QModemFlushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
    //    sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (QModemIoctl(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
void QModemAbortDataMode(void)
{
    sysprintf(" --> QModemAbortDataMode\n");
    QModemWrite((uint8_t*)"+++\r\n", strlen((char*)"+++\r\n"));
    vTaskDelay(1000/portTICK_RATE_MS); 
}

static void resetRetryTimes(ModemStageItem* pModemStageItem)
{
    int i;
    for(i = 0;;i++)
    {
        if(pModemStageItem[i].stageIndex == MODEM_NULL_STAGE_INDEX)
        {
            break;
        }
        else
        {
            pModemStageItem[i].retryTimes = 0; 
        }
    }

}
static ModemStageItem* fineModemStageItem(ModemStageIndex index, ModemStageItem* pModemStageItem)
{
    int i;
    ModemStageItem* reVal = NULL;
    for(i = 0;;i++)
    {
        if(pModemStageItem[i].stageIndex == MODEM_NULL_STAGE_INDEX)
        {
            break;
        }
        else
        {
            if(pModemStageItem[i].stageIndex == index)
            {
                reVal = &pModemStageItem[i];
                //sysprintf("fineModemStageItem get it:%d\r\n", i);
                break;
            }
                
            
        }
    }
    //sysprintf("fineModemStageItem %d:%d\r\n", index, reVal->stageIndex);
    return reVal;
}

static ModemStageIndex processCmdReqAction(ModemStageItem* pStageItem, CmdReq cmdReq)
{
    int i;
    ModemStageIndex reVal = MODEM_NULL_STAGE_INDEX;
    for(i = 0;i<MODEM_STAGE_CMD_REG_ACTION_NUM;i++)
    {
        if(pStageItem->cmdRegAction[i].cmdReg == CMD_REQ_NULL)
        {
            break;
        }
        else
        {
            if(pStageItem->cmdRegAction[i].cmdReg == cmdReq)
            {
                reVal = pStageItem->cmdRegAction[i].nextStage;
                //sysprintf("processCmdReqAction get it:%d\r\n", i);
                break;
            }
                
            
        }
    }
    //sysprintf("processCmdReqAction %d \r\n", reVal);
    return reVal;
}

static CmdActionValue processCmdReqActionPure(ModemCmdItem* pModemCmdItem, CmdReq cmdReq)
{
    int i;
    CmdActionValue reVal = CMD_ACTION_IGNORE;
    for(i = 0;i<MODEM_STAGE_CMD_REG_ACTION_NUM;i++)
    {
        if(pModemCmdItem->cmdRegAction[i].cmdReg == CMD_REQ_NULL)
        {
            break;
        }
        else
        {
            //sysprintf("processCmdReqActionPure compare:%d : %d\r\n", pModemCmdItem->cmdRegAction[i].cmdReg, cmdReq);
            if(pModemCmdItem->cmdRegAction[i].cmdReg == cmdReq)
            {
                //sysprintf("processCmdReqActionPure get it:%d\r\n", i);
                if(pModemCmdItem->cmdRegAction[i].returnValue)
                    reVal = CMD_ACTION_OK;
                else
                    reVal = CMD_ACTION_ERROR;
                
                break;
            }
                
            
        }
    }
    //sysprintf("processCmdReqActionPure %d \r\n", reVal);
    return reVal;
}
#if(0)
static void saveToFile(char* fileName, uint8_t* buff, int buffLen)
{
   sysprintf("saveToFile %d\r\n", buffLen);
   FILE *fptr;

   fptr = fopen(fileName, "wb");
   if(fptr == NULL)
   {
      sysprintf("Error!");
      return;
   }
   
   fwrite(buff,1,buffLen,fptr);
   fclose(fptr);

   //return 0;
}
#endif

static ModemReturnValue actionCmd(ModemStageItem* pModemStageItemTmp, ModemStageIndex* stageIndex, ModemStageIndex exitStageIndex, ParserType parserType) 
{
    CmdReq cmdReq = CMD_REQ_NULL;
    ModemStageIndex stageIndexTmp = MODEM_NULL_STAGE_INDEX;
    int waitTimes = (pModemStageItemTmp->waitTime) / (pModemStageItemTmp->checkIntervalTime);
    if (pModemStageItemTmp->cmd != NULL)
    {
        QModemFlushBuffer();
        QModemWrite(pModemStageItemTmp->cmd, strlen((char*) pModemStageItemTmp->cmd));
    }
    while (waitTimes > 0) 
    {
        if((*stageIndex == exitStageIndex) || cmdActionBreakFlag)
        {
            sysprintf("\r\n !!! actionCmd MODEM_RETURN_BREAK !!!\n");
            return MODEM_RETURN_BREAK;
        }
        vTaskDelay((pModemStageItemTmp->checkIntervalTime)/portTICK_RATE_MS);

        int n = QModemRead(readBuff, sizeof (readBuff));

        if (n > 0) 
        {
            cmdReq = atCmdProcessReadData(readBuff, n, parserType);
            if (cmdReq != CMD_REQ_NULL)
            {
                //sysprintf("\r\n !!! get cmeReq = %d !!!\n", cmdReq);
                stageIndexTmp = processCmdReqAction(pModemStageItemTmp, cmdReq);
                if (stageIndexTmp != MODEM_NULL_STAGE_INDEX) 
                {
                    *stageIndex = stageIndexTmp;
                    break;
                }
            }
        }
        else
        {
            sysprintf("-");
        }
        waitTimes--;
    }
    if(waitTimes == 0)
        return MODEM_RETURN_ERROR;
    else
        return MODEM_RETURN_OK;
}


static ModemReturnValue actionCmdPureEx(ModemCmdItem* pModemCmdItemTmp, ParserType parserType) 
{
    BOOL reVal =FALSE;
    CmdActionValue actionValue;
    CmdReq cmdReq = CMD_REQ_NULL;
    TickType_t tickLocalStart = xTaskGetTickCount();
    int waitTimes = (pModemCmdItemTmp->waitTime) / (pModemCmdItemTmp->checkIntervalTime);
    if (pModemCmdItemTmp->cmd != NULL) 
    {
        QModemFlushBuffer();
        
        tickLocalStart = xTaskGetTickCount();
        //sysprintf("\r\n !!! [WARNING]  actionCmdPure SEND %d (waitTimes = %d, waitTime = %d, checkIntervalTime = %d )!!!\n", tickLocalStart, waitTimes, pModemCmdItemTmp->waitTime, pModemCmdItemTmp->checkIntervalTime);
        
        //ATCmdSetReceiveDebugFlag(TRUE);
        QModemWrite(pModemCmdItemTmp->cmd, strlen((char*) pModemCmdItemTmp->cmd));
        //QModemFlushBuffer();
        //ATCmdSetReceiveDebugFlag(FALSE);
        
    }
    while (waitTimes > 0) 
    {
        if(cmdActionBreakFlag)
        {
            sysprintf("\r\n !!! actionCmdPure MODEM_RETURN_BREAK !!!\n");
            return MODEM_RETURN_BREAK;
        }
        int n = QModemRead(readBuff, sizeof (readBuff));      
        if (n > 0) {
            cmdReq = atCmdProcessReadData(readBuff, n, parserType);

            /* DEBUG LED *///terninalPrintf("n >=");
            /* DEBUG LED *///for(int i=0;i<n;i++)
            /* DEBUG LED *///terninalPrintf("%02x ",readBuff[i]);
            /* DEBUG LED *///terninalPrintf("\n  ");
            
            if((readBuff[n-4] == 'O') && (readBuff[n-3] == 'K') && (readBuff[n-2] == 0x0D) && (readBuff[n-1] == 0x0A))
                reVal = TRUE;
            else
                reVal = FALSE;
            break;
            if (cmdReq != CMD_REQ_NULL) 
            {

                //sysprintf("\r\n !!! get cmeReq = %d !!!\n", cmdReq);
                actionValue = processCmdReqActionPure(pModemCmdItemTmp, cmdReq);
                if(actionValue == CMD_ACTION_OK)
                {
                    reVal = TRUE;
                    break;
                }
                else if(actionValue == CMD_ACTION_ERROR)
                {
                    break;
                }
                else
                {
                    //sysprintf("\r\n !!! ++> re get cmeReq !!!\n");
                }
                
            }
        }
        else {
            //sysprintf("\r\n~[%d]~\r\n", xTaskGetTickCount() - tickLocalStart);
            sysprintf("~");
        }
        vTaskDelay((pModemCmdItemTmp->checkIntervalTime)/portTICK_RATE_MS);
        waitTimes--;
    }
    if(waitTimes == 0)
    {
        sysprintf("\r\n [---- ERROR ----] - actionCmdPure ERROR- timeout (%d)\r\n", xTaskGetTickCount() - tickLocalStart);
        {
            char str[512];
            sprintf(str, "\r\n [---- ERROR ----] - actionCmdPure ERROR- timeout (%s)\r\n", pModemCmdItemTmp->cmd);
            LoglibPrintf(LOG_TYPE_ERROR, str);
        }
        QModemAbortDataMode();
        return MODEM_RETURN_ERROR;
    }
    else
    {
        if(reVal)
            return MODEM_RETURN_OK;
        else
            return MODEM_RETURN_ERROR;
    }
}

static ModemReturnValue actionCmdPurePro(ModemCmdItem* pModemCmdItemTmp, ParserType parserType,char* rbuff,int rlen,int* retunlen,UINT32 readtime) 
{
    BOOL reVal =FALSE;
    CmdActionValue actionValue;
    CmdReq cmdReq = CMD_REQ_NULL;
    TickType_t tickLocalStart = xTaskGetTickCount();
    int waitTimes = (pModemCmdItemTmp->waitTime) / (pModemCmdItemTmp->checkIntervalTime);
    if (pModemCmdItemTmp->cmd != NULL) 
    {
        QModemFlushBuffer();
        
        tickLocalStart = xTaskGetTickCount();

        QModemWrite(pModemCmdItemTmp->cmd, strlen((char*) pModemCmdItemTmp->cmd));
        
    }
    while (waitTimes > 0) 
    {
        if(cmdActionBreakFlag)
        {
            sysprintf("\r\n !!! actionCmdPure MODEM_RETURN_BREAK !!!\n");
            return MODEM_RETURN_BREAK;
        }
        //terninalPrintf("sizeof (rbuff) = %d",sizeof (rbuff));
        int n = QModemReadEx((PUINT8)rbuff, rlen,readtime);
        if (n > 0) {
            *retunlen = n;
            //cmdReq = atCmdProcessReadData(rbuff, n, parserType);
            /*
            terninalPrintf("rbuff=%s\r\n",rbuff);
            terninalPrintf("rbuff=");
            for(int i=0;i<50;i++)
            {
              terninalPrintf("%02x ",rbuff[i]);
            }
            terninalPrintf("\r\n");  
            */
            //ATCmdSetReceiveDebugFlag(FALSE);
            //sysprintf("\r\n --TEST---  !!! get cmeReq = %d !!!\n", cmdReq);
            reVal = TRUE;
            break;
            if (cmdReq != CMD_REQ_NULL) 
            {

                //sysprintf("\r\n !!! get cmeReq = %d !!!\n", cmdReq);
                actionValue = processCmdReqActionPure(pModemCmdItemTmp, cmdReq);
                if(actionValue == CMD_ACTION_OK)
                {
                    reVal = TRUE;
                    break;
                }
                else if(actionValue == CMD_ACTION_ERROR)
                {
                    break;
                }
                else
                {
                    //sysprintf("\r\n !!! ++> re get cmeReq !!!\n");
                }
                
            }
        }
        else {
            //sysprintf("\r\n~[%d]~\r\n", xTaskGetTickCount() - tickLocalStart);
            sysprintf("~");
        }
        vTaskDelay((pModemCmdItemTmp->checkIntervalTime)/portTICK_RATE_MS);
        waitTimes--;
    }
    if(waitTimes == 0)
    {
        sysprintf("\r\n [---- ERROR ----] - actionCmdPure ERROR- timeout (%d)\r\n", xTaskGetTickCount() - tickLocalStart);
        {
            char str[512];
            sprintf(str, "\r\n [---- ERROR ----] - actionCmdPure ERROR- timeout (%s)\r\n", pModemCmdItemTmp->cmd);
            LoglibPrintf(LOG_TYPE_ERROR, str);
        }
        QModemAbortDataMode();
        return MODEM_RETURN_ERROR;
    }
    else
    {
        if(reVal)
            return MODEM_RETURN_OK;
        else
            return MODEM_RETURN_ERROR;
    }
}


static ModemReturnValue actionCmdPure(ModemCmdItem* pModemCmdItemTmp, ParserType parserType) 
{
    BOOL reVal =FALSE;
    CmdActionValue actionValue;
    CmdReq cmdReq = CMD_REQ_NULL;
    TickType_t tickLocalStart = xTaskGetTickCount();
    int waitTimes = (pModemCmdItemTmp->waitTime) / (pModemCmdItemTmp->checkIntervalTime);
    if (pModemCmdItemTmp->cmd != NULL) 
    {
        QModemFlushBuffer();
        
        tickLocalStart = xTaskGetTickCount();
        //sysprintf("\r\n !!! [WARNING]  actionCmdPure SEND %d (waitTimes = %d, waitTime = %d, checkIntervalTime = %d )!!!\n", tickLocalStart, waitTimes, pModemCmdItemTmp->waitTime, pModemCmdItemTmp->checkIntervalTime);
        
        //ATCmdSetReceiveDebugFlag(TRUE);
        QModemWrite(pModemCmdItemTmp->cmd, strlen((char*) pModemCmdItemTmp->cmd));
        //QModemFlushBuffer();
        //ATCmdSetReceiveDebugFlag(FALSE);
        
    }
    while (waitTimes > 0) 
    {
        if(cmdActionBreakFlag)
        {
            sysprintf("\r\n !!! actionCmdPure MODEM_RETURN_BREAK !!!\n");
            return MODEM_RETURN_BREAK;
        }
        int n = QModemRead(readBuff, sizeof (readBuff));      
        if (n > 0) {
            //ATCmdSetReceiveDebugFlag(TRUE);
            cmdReq = atCmdProcessReadData(readBuff, n, parserType);
            /*
            terninalPrintf("readBuff=%s\r\n",readBuff);
            terninalPrintf("readBuff=");
            for(int i=0;i<50;i++)
            {
              terninalPrintf("%02x ",readBuff[i]);
            }
            terninalPrintf("\r\n");  */
            //ATCmdSetReceiveDebugFlag(FALSE);
            //sysprintf("\r\n --TEST---  !!! get cmeReq = %d !!!\n", cmdReq);
            reVal = TRUE;
            break;
            if (cmdReq != CMD_REQ_NULL) 
            {

                //sysprintf("\r\n !!! get cmeReq = %d !!!\n", cmdReq);
                actionValue = processCmdReqActionPure(pModemCmdItemTmp, cmdReq);
                if(actionValue == CMD_ACTION_OK)
                {
                    reVal = TRUE;
                    break;
                }
                else if(actionValue == CMD_ACTION_ERROR)
                {
                    break;
                }
                else
                {
                    //sysprintf("\r\n !!! ++> re get cmeReq !!!\n");
                }
                
            }
        }
        else {
            //sysprintf("\r\n~[%d]~\r\n", xTaskGetTickCount() - tickLocalStart);
            sysprintf("~");
        }
        vTaskDelay((pModemCmdItemTmp->checkIntervalTime)/portTICK_RATE_MS);
        waitTimes--;
    }
    if(waitTimes == 0)
    {
        sysprintf("\r\n [---- ERROR ----] - actionCmdPure ERROR- timeout (%d)\r\n", xTaskGetTickCount() - tickLocalStart);
        {
            char str[512];
            sprintf(str, "\r\n [---- ERROR ----] - actionCmdPure ERROR- timeout (%s)\r\n", pModemCmdItemTmp->cmd);
            LoglibPrintf(LOG_TYPE_ERROR, str);
        }
        QModemAbortDataMode();
        return MODEM_RETURN_ERROR;
    }
    else
    {
        if(reVal)
            return MODEM_RETURN_OK;
        else
            return MODEM_RETURN_ERROR;
    }
}

static CmdReturnValue cmdProcess(char* processName, ModemStageIndex* stageIndex, ModemStageIndex targetStageIndex, ModemStageIndex exitStageIndex, ModemStageItem* pModemStageItem)
{
    ModemStageItem* pModemStageItemTmp = fineModemStageItem(*stageIndex, pModemStageItem);
//    CmdReq cmdReq = CMD_REQ_NULL;
    ModemStageIndex stageIndexTmp = MODEM_NULL_STAGE_INDEX;
    //sysprintf("\r\n  !! %s !!  <<< CURRENT STAGE :[0x%02x] >>>\r\n", processName, *stageIndex);
    if(targetStageIndex == *stageIndex)
    {
        sysprintf("\r\n   ======= [%s] SUCCESS!!!  =======\r\n ", processName);
        return CMD_RETURN_OK;
    }
    if((exitStageIndex == *stageIndex) || cmdActionBreakFlag)
    {
        sysprintf("\r\n   ======= [%s] Break!!!  =======\r\n ", processName);
        return CMD_RETURN_BREAK;
    }
    if(pModemStageItemTmp == NULL)
    {
        sysprintf("\r\n === [%s] pModemStageItemTmp == NULL!!!  ====\r\n ", processName);
        return CMD_RETURN_ERROR;
    }
    else
    {
        
        if((pModemStageItemTmp->cmdRegAction) == nullModemCmdRegAction)
        {
            vTaskDelay((pModemStageItemTmp->waitTime)/portTICK_RATE_MS);
        }
        else
        {
            vTaskDelay(1000/portTICK_RATE_MS);
            ModemReturnValue modemReval = actionCmd(pModemStageItemTmp, stageIndex, exitStageIndex, PARSER_TYPE_NORMAL);
            if(modemReval == MODEM_RETURN_ERROR)
            {
                sysprintf("\r\n [---- ERROR ----] timeout (waittime:%d, retryTimes:%d) -- *stageIndex = %d [%s] \r\n", pModemStageItemTmp->waitTime, pModemStageItemTmp->retryTimes, *stageIndex,  pModemStageItemTmp->cmd);
                //sysprintf("*");
                if((pModemStageItemTmp->retryTimes++) > MODEM_CMD_RETRY_TIMES)
                {
                    //{
                    //    char str[1024];
                    //    sprintf(str, "!!! modem cmdProcess retry error: stageIndex = %d...\r\n", pModemStageItemTmp->stageIndex);
                    //    LoglibPrintf(LOG_TYPE_ERROR, str);
                    //}
                    return CMD_RETURN_ERROR;
                }
                stageIndexTmp = processCmdReqAction(pModemStageItemTmp, CMD_REQ_ERROR);
                if(stageIndexTmp != MODEM_NULL_STAGE_INDEX)
                {
                    *stageIndex = stageIndexTmp;                
                }
            }
            else if(modemReval == MODEM_RETURN_BREAK)
            {
                sysprintf("\r\n   ======= [%s] Break _2!!!  =======\r\n ", processName);
                return CMD_RETURN_BREAK;
            }
        }
    }
    return CMD_RETURN_PROCESSING;

}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL QModemLibInit(UINT32 baudRate)
{
    sysprintf("QModemLibInit!!\n");
    pUartInterface = UartGetInterface(QUENTEL_MODEM_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("QModemLibInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    if(pUartInterface->initFunc(baudRate) == FALSE)
    {
        sysprintf("QModemLibInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(pUartInterface->setPowerFunc(TRUE) == FALSE)
    {
        sysprintf("QModemLibInit ERROR (setPowerFunc false)!!\n");
        return FALSE;
    }
    #if(ENABLE_MODEM_FLOW_CONTROL)
    if(pUartInterface->ioctlFunc(UART_IOC_ENABLEHWFLOWCONTROL, 0, 0))
    {
        sysprintf("QModemLibInit ERROR (ioctlFunc UART_IOC_ENABLEHWFLOWCONTROL false)!!\n");
        return FALSE;
    }
    #endif
    //QModemTotalStop();
    return TRUE;
}

void QModemDialupStart(void)
{
    sysprintf(" --> QModemDialupStart!!\n\r");
    if(pUartInterface == NULL)
        return ;
    if(BatteryCheckPowerDownCondition())
    {
        sysprintf(" --> QModemDialupStart!! (BatteryCheckPowerDownCondition)\n\r");
        QModemTotalStop();
        return ;
    }
    pUartInterface->setPowerFunc(TRUE);
    resetRetryTimes(dialupStageItem);
    
    //firstTimeCmd = TRUE;
    
    if(firstTimeCmd)
    {
        currentDialupStageIndex = DIALUP_FLOW_CTL_ON_STAGE_INDEX;//DIALUP_ECHO_OFF_STAGE_INDEX;
        firstTimeCmd = FALSE;
    }
    else
    {
        currentDialupStageIndex = DIALUP_ECHO_OFF_STAGE_INDEX;
    }

    cmdActionBreakFlag = FALSE;
    
    firstTimeFtpCmd = TRUE;
    
}
void QModemFtpClientStart(void)
{
    if(BatteryCheckPowerDownCondition())
    {
        sysprintf(" --> QModemDialupStart!! (BatteryCheckPowerDownCondition)\n\r");
        QModemTotalStop();
        return ;
    }
    resetRetryTimes(ftpClientStageItem);
    cmdActionBreakFlag = FALSE;
    
    if(firstTimeFtpCmd)
    {
        currentFtpStageIndex = FTP_SET_PDP_STAGE_INDEX;
        firstTimeFtpCmd = FALSE;
        sysprintf("\r\n [INFORMATION] --> QModemFtpClientStart [FTP_SET_PDP_STAGE_INDEX]!!\n\r");
    }
    else
    {
        currentFtpStageIndex = FTP_CONNECTING_STAGE_INDEX;
        sysprintf("\r\n [INFORMATION] --> QModemFtpClientStart [FTP_CONNECTING_STAGE_INDEX]!!\n\r");
    }
}
void QModemTotalStop(void)
{
    sysprintf(" --> QModemTotalStop!!\n\r");
    if(pUartInterface == NULL)
        return ;
    pUartInterface->setPowerFunc(FALSE);
    resetRetryTimes(dialupStageItem);
    resetRetryTimes(ftpClientStageItem);
    
    currentDialupStageIndex = DIALUP_IDLE_STAGE_INDEX;
    currentFtpStageIndex = FTP_IDLE_STAGE_INDEX; 
    cmdActionBreakFlag = TRUE;    		
    sysprintf(" --> QModemTotalStop OK!!\n\r");
}

BOOL QModemDialupProcess(void)
{
    int waitCounter = 0;
    sysprintf(" --> QModemDialupProcess!!\n\r");
        
    //LedSetColor(modemColorAllGreen, 0x04, TRUE);
    
            //LedSetAliveStatusLightFlush(40,8);
            //LedSetAliveStatusLightFlush(0xff,8);
            //vTaskDelay(1000/portTICK_RATE_MS);
            //LedSetColor(modemColorAllGreen, 0x04, TRUE);
    
            LedSetStatusLightFlush(50,10);  // solve LED board bug
    LedSetColor(modemColorAllOff, 0x04, TRUE);
    //vTaskDelay(100/portTICK_RATE_MS);
    LedSetBayLightFlush(0xff,8);
    vTaskDelay(100/portTICK_RATE_MS);
    LedSetColor(modemColorAllGreen, 0x04, TRUE);    
    LedSetAliveStatusLightFlush(0xff,8);
    vTaskDelay(2000/portTICK_RATE_MS);
   
    LedSetColor(modemColorAllRed, 0x08, TRUE);
    vTaskDelay(2000/portTICK_RATE_MS);
    LedSetColor(modemColorAllOff, 0x08, TRUE);
    

    LedSetAliveStatusLightFlush(40,8);
        LedSetStatusLightFlush(0,10);  // solve LED board bug
    
    //terninalPrintf("Stop.\r\n");
    //LedSendFactoryTest();
    //while(1);
    /*
        if(flag)
        {
            pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0); // wale
            
        }
        else
        {
            pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0); // sleep
        }
    */
    
    while(1)
    {
        terninalPrintf(".");
        /*
        if(changeLEDColorFlag)
        {
            changeLEDColorFlag = FALSE;
            //LedSetColor(modemColorAllRed, 0x04, TRUE);
        }
        else
        {
            changeLEDColorFlag = TRUE;
            //LedSetColor(modemColorAllGreen, 0x08, TRUE);
        }
        */
        CmdReturnValue reval= cmdProcess(" = DIAL_UP PROCESS = ", &currentDialupStageIndex, DIALUP_CONNECTED_STAGE_INDEX, DIALUP_IDLE_STAGE_INDEX, dialupStageItem);
        if(reval == CMD_RETURN_OK)
        {
            return TRUE;
        }
        else if(reval == CMD_RETURN_PROCESSING)
        {
            waitCounter++;
            if(waitCounter > 30)
                return FALSE;
        }
        else if(reval == CMD_RETURN_BREAK)
        {
            return FALSE;
        }
        else if(reval == CMD_RETURN_ERROR)
        {
            return FALSE;
        }
        else
        {
            return FALSE;
        }
    }   
}

BOOL QModemFtpClientProcess(void)
{
    
    sysprintf(" --> QModemFtpClientProcess!!\n\r");
    while(1)
    {
        CmdReturnValue reval= cmdProcess(" ~ FTP_CONNECT PROCESS ~ ", &currentFtpStageIndex, FTP_CONNECTED_STAGE_INDEX, FTP_IDLE_STAGE_INDEX, ftpClientStageItem);
        if(reval == CMD_RETURN_OK)
        {
            return TRUE;
        }
        else if(reval == CMD_RETURN_PROCESSING)
        {
        }
        else if(reval == CMD_RETURN_BREAK)
        {
            return FALSE;
        }
        else if(reval == CMD_RETURN_ERROR)
        {
            return FALSE;
        }
        else
        {
            return FALSE;
        }
    } 
}
ModemStageIndex QModemDialupStageIndex(void)
{
    return currentDialupStageIndex;
}
ModemStageIndex QModemFtpStageIndex(void)
{
    return currentFtpStageIndex;
}

BOOL FtpClientSendFile(BOOL needChdirFlag, char* preDirName, char* dirName, char* fileName, uint8_t* buff, int len)
{
    BOOL needMkdirFlag = FALSE;
    //BOOL needChdirFlag = TRUE;
    uint8_t cmdFtpSendInit[256];
    ModemReturnValue modemReval;
    if(len == 0)
    {
        sysprintf(" == FtpClientSendFile return false len = %d\r\n", len);
        return FALSE;
    }
    else
    {
        sysprintf(" == FtpClientSendFile [%s][%s][%s] len = %d (needChdirFlag = %d)\r\n", preDirName, dirName, fileName, len, needChdirFlag);
    }
    #if(0)
    sprintf((char*)cmdFtpSendInit, "AT+QFTPPWD\r\n");  
    ftpGetDirCmdItem.cmd = cmdFtpSendInit;
    modemReval = actionCmdPure(&ftpGetDirCmdItem, PARSER_TYPE_NORMAL);
    if(modemReval == MODEM_RETURN_ERROR)
    {
        sysprintf("\r\n -- FtpClientSendFile MODEM_RETURN_ERROR(1)-- cmdFtpSendInit[%s]\r\n", ftpGetDirCmdItem.cmd);
    }
    else if(modemReval == MODEM_RETURN_BREAK)
    {

       
    }
    else
    {
        int destLen;
        //sysprintf(" == Web Get Current dir OK [%s]\r\n", ATCmdDataTempBuffer(NULL));                         
        char* str = parserString((char*)ATCmdDataTempBuffer(NULL), "0,/", "\r\n", &destLen);                        
        if(str != NULL)
        {
            str[destLen] = 0x0;
             
            sysprintf(" == compare [%s:%d] [%s:%d]\r\n", str, strlen(str), dirName, strlen(dirName));  
            if(strcmp(str, dirName) == 0)
            {
                needChdirFlag = FALSE;
                sysprintf("  == Web Get Current Real dir OK [%s], ignore need change directory\r\n", str);   
            }    
            else
            {
                sysprintf("  == Web Get Current Real dir OK [%s], need change directory\r\n", str);  
            }                
        }
    }
    #else
    //needChdirFlag = TRUE;
    #endif
    
    if(needChdirFlag)
    {
        sprintf((char*)cmdFtpSendInit, "AT+QFTPCWD=\"/%s\"\r\n", dirName);  
        //sprintf((char*)cmdFtpSendInit, "AT+QFTPCWD=\"/\"\r\n"); 
        ftpChDirCmdItem.cmd = cmdFtpSendInit;
        modemReval = actionCmdPure(&ftpChDirCmdItem, PARSER_TYPE_NORMAL);
        if(modemReval == MODEM_RETURN_ERROR)
        {
            sysprintf("\r\n -- FtpClientSendFile MODEM_RETURN_ERROR(2)-- cmdFtpSendInit[%s]\r\n", ftpChDirCmdItem.cmd);
            needMkdirFlag = TRUE;
        }
        else if(modemReval == MODEM_RETURN_BREAK)
        {

           return FALSE;
        }
        
        if(needMkdirFlag)
        {
            sysprintf("\r\n == FtpClientSendFile Create Directory [%s][%s][%s]\r\n", preDirName, dirName, fileName);
            
            sprintf((char*)cmdFtpSendInit, "AT+QFTPMKDIR=\"/%s\"\r\n", preDirName);  
            ftpMkDirCmdItem.cmd = cmdFtpSendInit;
            ModemReturnValue modemReval = actionCmdPure(&ftpMkDirCmdItem, PARSER_TYPE_NORMAL);
            if(modemReval == MODEM_RETURN_ERROR)
            {
                sysprintf("\r\n -- FtpClientSendFile MODEM_RETURN_ERROR(3)-- cmdFtpSendInit[%s]\r\n", ftpMkDirCmdItem.cmd);
                return FALSE;

            }
            else if(modemReval == MODEM_RETURN_BREAK)
            {

                return FALSE;
            }
            else
            {
                sprintf((char*)cmdFtpSendInit, "AT+QFTPMKDIR=\"/%s\"\r\n", dirName);  
                ftpMkDirCmdItem.cmd = cmdFtpSendInit;
                ModemReturnValue modemReval = actionCmdPure(&ftpMkDirCmdItem, PARSER_TYPE_NORMAL);
                if(modemReval == MODEM_RETURN_ERROR)
                {
                    sysprintf("\r\n -- FtpClientSendFile MODEM_RETURN_ERROR(4)-- cmdFtpSendInit[%s]\r\n", ftpMkDirCmdItem.cmd);
                    return FALSE;

                }
                else if(modemReval == MODEM_RETURN_BREAK)
                {

                    return FALSE;
                }
                else
                {
                    sprintf((char*)cmdFtpSendInit, "AT+QFTPCWD=\"/%s\"\r\n", dirName);  
                    //sprintf((char*)cmdFtpSendInit, "AT+QFTPCWD=\"/\"\r\n"); 
                    ftpChDirCmdItem.cmd = cmdFtpSendInit;
                    ModemReturnValue modemReval = actionCmdPure(&ftpChDirCmdItem, PARSER_TYPE_NORMAL);
                    if(modemReval == MODEM_RETURN_ERROR)
                    {
                        sysprintf("\r\n -- FtpClientSendFile MODEM_RETURN_ERROR(5)-- cmdFtpSendInit[%s]\r\n", ftpChDirCmdItem.cmd);
                        return FALSE;

                    }
                    else if(modemReval == MODEM_RETURN_BREAK)
                    {

                        return FALSE;
                    }
                }
            }
        }
        else
        {
            //sysprintf("\r\n == FtpClientSendFile Ignore Create Directory [%s][%s][%s]\r\n", preDirName, dirName, fileName);
        }
    }
            
    sprintf((char*)cmdFtpSendInit, "AT+QFTPPUT=\"%s\", \"COM:\",0, %d, 1\r\n", fileName, len); //last data package , auto close ftp socket 
    //sprintf((char*)cmdFtpSendInit, "AT+QFTPPUT=\"%s\", \"COM:\",0, %d, 0\r\n", fileName, len);   
    //sprintf((char*)cmdFtpSendInit, "AT+QFTPPUT=\"%s\",\"COM:\"\r\n", fileName);       
    ftpSendInitCmdItem.cmd = cmdFtpSendInit;
    modemReval = actionCmdPure(&ftpSendInitCmdItem, PARSER_TYPE_NORMAL);
    if(modemReval == MODEM_RETURN_ERROR)
    {
        sysprintf("\r\n -- FtpClientSendFile MODEM_RETURN_ERROR(6)-- cmdFtpSendInit[%s]\r\n", ftpSendInitCmdItem.cmd);
        //QModemAbortDataMode();
        return FALSE;
    
    }
    else if(modemReval == MODEM_RETURN_BREAK)
    {
    
        return FALSE;
    }
    else
    {
        //sysprintf(" == init Ftp Client init OK\r\n");
        int i = 0;
        int total = 0;
        int leftLen = 0;
        int n;
        for(i = 0; i< len/FTP_UART_WRITE_SIZE; i++)
        {
            if(cmdActionBreakFlag)
            {
                sysprintf("\r\n !!! FtpClientSendFile MODEM_RETURN_BREAK !!!\n");
                return FALSE;
            }
            n = QModemWrite(buff + i*FTP_UART_WRITE_SIZE, FTP_UART_WRITE_SIZE);
            if(n != FTP_UART_WRITE_SIZE)
            {
                sysprintf(" == Ftp Client write error 1\r\n");
                return FALSE;
            }
            else
            {
                total = total + n;
                sysprintf("%08d (%d:%d)\r", total, n, i*FTP_UART_WRITE_SIZE);
            }
        }
        if(cmdActionBreakFlag)
        {
            sysprintf("\r\n !!! FtpClientSendFile MODEM_RETURN_BREAK_2 !!!\n");
            return FALSE;
        }
        //sysprintf("\r\n== [%d:%d] ==\r\n", len/FTP_UART_WRITE_SIZE*FTP_UART_WRITE_SIZE, len-(len/FTP_UART_WRITE_SIZE*FTP_UART_WRITE_SIZE));
        leftLen = len-(len/FTP_UART_WRITE_SIZE*FTP_UART_WRITE_SIZE);
        if(leftLen > 0)
        {
            n = QModemWrite(buff + len/FTP_UART_WRITE_SIZE*FTP_UART_WRITE_SIZE, leftLen);
            if(n != leftLen)
            {
                sysprintf(" == Ftp Client write error 2\r\n");
                return 0;
            }
            else
            {
                total = total + n;
                //sysprintf("\r\n%08d _2 (%d:%d)\r", total, n, leftLen);
            }
        }
        //QModemAbortDataMode();
        //sysprintf("\r\n == QModemWrite %d/%d\r\n", total, len);
        if (len > 0)
        {
            modemReval = actionCmdPure(&ftpSendDataCmdItem, PARSER_TYPE_NORMAL);
            if(modemReval == MODEM_RETURN_ERROR)
            {
                sysprintf(" == Send Ftp Client Data Error\r\n");
                //QModemAbortDataMode();
                return FALSE;
            }
            else if(modemReval == MODEM_RETURN_BREAK)
            {
                sysprintf(" == Send Ftp Client Data Break\r\n");
                return FALSE;
            }
            else
            {
                sysprintf(" == Send Ftp Client Data OK[%s]\r\n", ATCmdDataTempBuffer(NULL));
                return TRUE;
            }
        }        
    }  
    return FALSE;
}

BOOL FtpClientGetFile(char* dirName, char* fileName)
{
    uint8_t cmdFtpGetInit[256];
    
    sysprintf(" == FtpClientGetFile [%s][%s]\r\n", dirName, fileName);

    sprintf((char*)cmdFtpGetInit, "AT+QFTPCWD=\"/%s\"\r\n", dirName);  
    //sprintf((char*)cmdFtpGetInit, "AT+QFTPCWD=\"/\"\r\n"); 
    
    //sysprintf(" == init Ftp change dir..., we send [%s]\r\n", cmdFtpGetInit);     
    ftpChDirCmdItem.cmd = cmdFtpGetInit;
    ModemReturnValue modemReval = actionCmdPure(&ftpChDirCmdItem, PARSER_TYPE_NORMAL);
    if(modemReval == MODEM_RETURN_ERROR)
    {
        //sysprintf("\r\n -- timeout (%d) -- *stageIndex = %d \r\n", pModemStageItemTmp->waitTime, *stageIndex);
        return FALSE;

    }
    else if(modemReval == MODEM_RETURN_BREAK)
    {

        return FALSE;
    }
    else
    {
        
        //AT+QFTPGET=\"%s\", "COM:"        
        sprintf((char*)cmdFtpGetInit, "AT+QFTPGET=\"%s\", \"COM:\"\r\n", fileName);
        //sysprintf(" == init Ftp change dir OK, we send [%s]\r\n", cmdFtpGetInit);  
        ftpGetDataCmdItem.cmd = cmdFtpGetInit;
        ModemReturnValue modemReval = actionCmdPure(&ftpGetDataCmdItem, PARSER_TYPE_FTP);
        if(modemReval == MODEM_RETURN_ERROR)
        {
            //sysprintf("\r\n -- timeout (%d) -- *stageIndex = %d \r\n", pModemStageItemTmp->waitTime, *stageIndex);
            return FALSE;

        }
        else if(modemReval == MODEM_RETURN_BREAK)
        {

            return FALSE;
        }
        else
        {
            //sysprintf(" == Web Get FTP contain OK [%s]\r\n", ATCmdDataTempBuffer(NULL));
            int destLen;
            char* str = parserString((char*)ATCmdDataTempBuffer(NULL), "\r\nCONNECT\r\n", "\r\nOK\r\n", &destLen);
            if(FileAgentAddData(TARIFF_FILE_SAVE_POSITION, TARIFF_FILE_DIR, fileName, (uint8_t*)str, destLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
            {
                 return TRUE;           
            }
            return FALSE;
        }
        
    }
}

BOOL FtpClientGetFileEx(char* dirName, char* fileName)
{
    uint8_t cmdFtpGetInit[256];
    
    sysprintf(" == FtpClientGetFile [%s][%s]\r\n", dirName, fileName);

    sprintf((char*)cmdFtpGetInit, "AT+QFTPCWD=\"/%s\"\r\n", dirName);  
    //sprintf((char*)cmdFtpGetInit, "AT+QFTPCWD=\"/\"\r\n"); 
    
    //sysprintf(" == init Ftp change dir..., we send [%s]\r\n", cmdFtpGetInit);     
    ftpChDirCmdItem.cmd = cmdFtpGetInit;
    ModemReturnValue modemReval = actionCmdPure(&ftpChDirCmdItem, PARSER_TYPE_NORMAL);
    if(modemReval == MODEM_RETURN_ERROR)
    {
        //sysprintf("\r\n -- timeout (%d) -- *stageIndex = %d \r\n", pModemStageItemTmp->waitTime, *stageIndex);
        return FALSE;

    }
    else if(modemReval == MODEM_RETURN_BREAK)
    {

        return FALSE;
    }
    else
    {
        
        //AT+QFTPGET=\"%s\", "COM:"        
        sprintf((char*)cmdFtpGetInit, "AT+QFTPGET=\"%s\", \"COM:\"\r\n", fileName);
        //sysprintf(" == init Ftp change dir OK, we send [%s]\r\n", cmdFtpGetInit);  
        ftpGetDataCmdItem.cmd = cmdFtpGetInit;
        ModemReturnValue modemReval = actionCmdPure(&ftpGetDataCmdItem, PARSER_TYPE_FTP);
        if(modemReval == MODEM_RETURN_ERROR)
        {
            //sysprintf("\r\n -- timeout (%d) -- *stageIndex = %d \r\n", pModemStageItemTmp->waitTime, *stageIndex);
            return FALSE;

        }
        else if(modemReval == MODEM_RETURN_BREAK)
        {

            return FALSE;
        }
        else
        {
            //sysprintf(" == Web Get FTP contain OK [%s]\r\n", ATCmdDataTempBuffer(NULL));
            int destLen;
            char* str = parserString((char*)ATCmdDataTempBuffer(NULL), "\r\nCONNECT\r\n", "\r\nOK\r\n", &destLen);
            terninalPrintf("*str = ");
            for(int i = 0;i<destLen;i++)
                terninalPrintf("%02x ",str[i]);
            terninalPrintf("\r\n");
            return TRUE; 
            //if(FileAgentAddData(TARIFF_FILE_SAVE_POSITION, TARIFF_FILE_DIR, fileName, (uint8_t*)str, destLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
            //{
            //     return TRUE;           
            //}
            //return FALSE;
        }
        
    }
}

BOOL FtpClientGetFileOTA(char* dirName, char* fileName)
{
    uint8_t cmdFtpGetInit[256];
    
    sysprintf(" == FtpClientGetFile [%s][%s]\r\n", dirName, fileName);

    sprintf((char*)cmdFtpGetInit, "AT+QFTPCWD=\"/%s\"\r\n", dirName);  
    //sprintf((char*)cmdFtpGetInit, "AT+QFTPCWD=\"/\"\r\n"); 
    
    //sysprintf(" == init Ftp change dir..., we send [%s]\r\n", cmdFtpGetInit);     
    ftpChDirCmdItem.cmd = cmdFtpGetInit;
    ModemReturnValue modemReval = actionCmdPure(&ftpChDirCmdItem, PARSER_TYPE_NORMAL);
    if(modemReval == MODEM_RETURN_ERROR)
    {
        //sysprintf("\r\n -- timeout (%d) -- *stageIndex = %d \r\n", pModemStageItemTmp->waitTime, *stageIndex);
        return FALSE;

    }
    else if(modemReval == MODEM_RETURN_BREAK)
    {

        return FALSE;
    }
    else
    {
        
        //AT+QFTPGET=\"%s\", "COM:"        
        sprintf((char*)cmdFtpGetInit, "AT+QFTPGET=\"%s\", \"COM:\"\r\n", fileName);
        //sysprintf(" == init Ftp change dir OK, we send [%s]\r\n", cmdFtpGetInit);  
        ftpGetDataCmdItem.cmd = cmdFtpGetInit;
        ModemReturnValue modemReval = actionCmdPure(&ftpGetDataCmdItem, PARSER_TYPE_FTP);
        if(modemReval == MODEM_RETURN_ERROR)
        {
            //sysprintf("\r\n -- timeout (%d) -- *stageIndex = %d \r\n", pModemStageItemTmp->waitTime, *stageIndex);
            return FALSE;

        }
        else if(modemReval == MODEM_RETURN_BREAK)
        {

            return FALSE;
        }
        else
        {
            //sysprintf(" == Web Get FTP contain OK [%s]\r\n", ATCmdDataTempBuffer(NULL));
            int destLen;
            char* str = parserString((char*)ATCmdDataTempBuffer(NULL), "\r\nCONNECT\r\n", "\r\nOK\r\n", &destLen);
            //if(FileAgentAddData(TARIFF_FILE_SAVE_POSITION, TARIFF_FILE_DIR, fileName, (uint8_t*)str, destLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
            if(FileAgentAddData(FILE_AGENT_STORAGE_TYPE_YAFFS2, "/", "epmtest.bin", (uint8_t*)str, destLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE) !=  FILE_AGENT_RETURN_ERROR )
            {
                 return TRUE;           
            }
            return FALSE;
        }
        
    }
}


BOOL WebPostMessage(char* url, uint8_t* buff)
{
    char cmdWebUrlBuff[64];
    uint8_t cmdWebPostBuff[64];
    cmdActionBreakFlag = FALSE;
    sprintf((char*)cmdWebUrlBuff, "%sjsonlen=%d", url, strlen((char*)buff));
    sprintf((char*)cmdWebPostBuff, "AT+QHTTPURL=%d,10\r\n", strlen(cmdWebUrlBuff));    
    //sysprintf(" == WebPostMessage url = %s(%s): [%s], send [%s]\r\n", url, cmdWebUrlBuff, buff, cmdWebPostBuff);
    
    webSetUrlInfoCmdItem.cmd = cmdWebPostBuff;
    ModemReturnValue modemReval = actionCmdPure(&webSetUrlInfoCmdItem, PARSER_TYPE_NORMAL);
    if(modemReval == MODEM_RETURN_ERROR)
    {
        return FALSE;
    }
    else if(modemReval == MODEM_RETURN_BREAK)
    {
        return FALSE;
    }
    else
    {
        //sysprintf(" == Web set url info OK\r\n");
        webSetUrlContainCmdItem.cmd = (uint8_t*)cmdWebUrlBuff;
        ModemReturnValue modemReval = actionCmdPure(&webSetUrlContainCmdItem, PARSER_TYPE_NORMAL);
        if(modemReval == MODEM_RETURN_ERROR)
        {            
            return FALSE;
        }
        else if(modemReval == MODEM_RETURN_BREAK)
        {
            return FALSE;
        }
        else
        {
            sprintf((char*)cmdWebPostBuff, "AT+QHTTPPOST=%d,10,10\r\n", strlen((char*)buff));   
            //sysprintf(" == Web set url contain OK, send [%s]\r\n", cmdWebPostBuff);            
            webSetPostInfoCmdItem.cmd = cmdWebPostBuff;
            ModemReturnValue modemReval = actionCmdPure(&webSetPostInfoCmdItem, PARSER_TYPE_NORMAL);
            if(modemReval == MODEM_RETURN_ERROR)
            {
                //QModemAbortDataMode();
                return FALSE;

            }
            else if(modemReval == MODEM_RETURN_BREAK)
            {
                return FALSE;
            }
            else
            {
               // sysprintf(" == Web set Post Info OK\r\n");
                webSetPostContainCmdItem.cmd = buff;
                ModemReturnValue modemReval = actionCmdPure(&webSetPostContainCmdItem, PARSER_TYPE_NORMAL);
                if(modemReval == MODEM_RETURN_ERROR)
                {
                    //QModemAbortDataMode();
                    return FALSE;

                }
                else if(modemReval == MODEM_RETURN_BREAK)
                {
                    return FALSE;
                }
                else
                {
                    //sysprintf(" == Web set Post contain OK [%s]\r\n", ATCmdDataTempBuffer(NULL)); 
                    sprintf((char*)cmdWebPostBuff, "AT+QHTTPREAD=%d\r\n", 100); 
                    webGetPostContainCmdItem.cmd = cmdWebPostBuff;
                    ModemReturnValue modemReval = actionCmdPure(&webGetPostContainCmdItem, PARSER_TYPE_WEB_POST);
                    if(modemReval == MODEM_RETURN_ERROR)
                    {
                        //QModemAbortDataMode();
                        return FALSE;
                    }
                    else if(modemReval == MODEM_RETURN_BREAK)
                    {

                        return FALSE;
                    }
                    else
                    {
                        int destLen;
                        //sysprintf(" == Web Get Post contain OK [%s]\r\n", ATCmdDataTempBuffer(NULL));                         
                        char* str = parserString((char*)ATCmdDataTempBuffer(NULL), "\r\nCONNECT\r\n", "\r\nOK\r\n", &destLen);
                        
                        if(str != NULL)
                        {
                            str[destLen] = 0x0;
                            if(DataParserWebPostReturnData(str))
                            {
                            }
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}

BOOL FtpClientClose(void)
{
    if(actionCmdPure(&ftpCloseCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
    {
        sysprintf(" == FtpClientClose OK\r\n"); 
        return TRUE;
    }
    else
    {
        sysprintf(" == FtpClientClose ERROR\r\n"); 
        return FALSE;
    }
}

char* FtpQueryCsq(void)
{
    if(actionCmdPure(&networkQueryCsqCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
    {
        sysprintf(" == FtpQueryCsq OK [%s]\r\n", ATCmdDataTempBuffer2); 
        int destLen;
        char* str = parserString((char*)ATCmdDataTempBuffer2(NULL), "+CSQ: ", ",", &destLen);
        if(str != NULL)
        {
            str[destLen] = 0x0;
            sysprintf(" == FtpQueryCsq RSSI [%s]\r\n", str); 
            return str;                            
        }        
        sysprintf(" == FtpQueryCsq RSSI parser error\r\n"); 
        return NULL;
    }
    else
    {
        sysprintf(" == FtpQueryCsq ERROR\r\n"); 
        return NULL;
    }
}


BOOL QModemQueryIPAddress(char* ipBuffer)
{
    if (actionCmdPure(&networkQueryIPAddressItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
    {
        sysprintf(" == QModemQueryIPAddress OK [%s]\r\n", ATCmdDataTempBuffer2(NULL));
        int destLen;
        char* str = parserString((char*)ATCmdDataTempBuffer2(NULL), "\"", "\"", &destLen);
        sysprintf(" == IP Address = [%s], destLen=%d\r\n", str, destLen);
        if (destLen > 0)
        {
            memcpy(ipBuffer, str, destLen);
            return TRUE;
        }
    }
    return FALSE;
}




BOOL QModemATCmdTest(void)
{
    BOOL timeOut = 10;//per
    int index = 0;
    INT32 reVal;
    uint8_t buff[54];
    cmdActionBreakFlag = FALSE;
    //pUartInterface->setPowerFunc(TRUE);
    //vTaskDelay(1000/portTICK_RATE_MS);
    sysprintf(" Finish power on\r\n");
    //char retval[100];
    while(timeOut > 0)
    {
        sysprintf(" timeOut == %d\r\n",timeOut); 
        if(actionCmdPure(&networkAtTestCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
        {
            return TRUE;
        }
        else
        {
            timeOut--;
        }
#if (ENABLE_BURNIN_TESTER)
        if (!EnabledBurninTestMode())
#endif
        {
            EPDDrawString(TRUE,".",X_POS_MSG+150+(9-timeOut)*25,Y_POS_MSG);
        }
    }
    sysprintf(" == QModemATCmdTest ERROR\r\n"); 
    //pUartInterface->setPowerFunc(FALSE);
    cmdActionBreakFlag = TRUE;
    return FALSE;
}

BOOL QModemGetVer(char* reStr)
{
    BOOL time = 2;//10;
    char* pch1= malloc(100);
    char* pch2;
    char tempchr[100] ;  //= malloc(100);
    int charCounter;
    int tryCounter = 0;
    //int pchindex;
    cmdActionBreakFlag = FALSE;
        while(time > 0)
    {
        //terninalPrintf(" timeOut == %d\r\n",time); 
        if(actionCmdPure(&readVersionCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
        {   
            /*
            memcpy(tempchr,readBuff,100);
            //terninalPrintf("tempchr=%s\r\n",tempchr); 
            pch1 = (char*) memchr(tempchr,'\n',100);
            //terninalPrintf("pch1=%s\r\npch1AD=%d\r\n",pch1,pch1);
            memcpy(tempchr,pch1+1,99);
            //terninalPrintf("tempchr=%s\r\ntempchrAD=%d\r\n",tempchr,tempchr);
            pch2 = (char*) memchr(tempchr,'\n',100);
            //terninalPrintf("pch2=%s\r\npch2AD=%d\r\n",pch2,pch2);
            memset (pch2,'\0',1);
            memcpy(reStr,tempchr,pch2-tempchr);
            //terninalPrintf("reStr=%s\r\n",reStr);
            free(pch1);*/
            int j = 0;
            charCounter=0;
            for(int i=0;i<50;i++)
            {
                
                if(readBuff[j++] == 0x0A)
                {   
                    charCounter++;
                    if(charCounter == 2)
                        break;
                }
                   
            }
            

            //terninalPrintf("readBuff[%d]=%02x=%c\r\n",j+2,readBuff[j+2],readBuff[j+2]);
            //terninalPrintf("readBuff[%d]=%02x=%c\r\n",j+3,readBuff[j+3],readBuff[j+3]);
            tryCounter++;
            if((readBuff[j+2] == 'O') && (readBuff[j+3] == 'K'))
            {
                
                memcpy(reStr,readBuff,100);
                return TRUE;
            }
            else if(tryCounter == 5)
            {
               return FALSE; 
            }
        }
        else
        {
            time--;
        }
    }
    cmdActionBreakFlag = TRUE;
    return FALSE;
    
    
    
    
    
    
    
    uint8_t buff[100];
    int index = 0;
    QModemWrite((uint8_t*)"AT+GMR\r\n",strlen((char*)"AT+GMR\r\n"));
    int timeOut = 3000;
    int reVal = 0;
    while(timeOut > 0)
    {
        //pUartInterface->setPowerFunc(FALSE);
        //memcpy(retval,buff,16);
        reVal = pUartInterface->readFunc((uint8_t*)buff + index, sizeof(buff)-index);
        index = index + reVal;
        if(index >= 33)
        {
            /*
            for(int i=0;i<index;i++)
                terninalPrintf("%c",buff[i]);
            terninalPrintf("\n");*/
            int destLen = 0;
            char* tmp = parserString((char*)buff, "AT+GMR\r\r\n", "\r\n\r\nOK\r\n", &destLen);
            tmp[destLen] = 0x0;
            //memcpy(reStr,tmp,34);
            for(int i=0;i<=destLen;i++)
            {
                *(reStr+i) = *(tmp + i);
            }
            sysprintf(">%s< %p %p %p\n",reStr,reStr,tmp,buff);
            return TRUE;
            /*
            sysprintf(" == QModemATCmdTest OK\r\n"); 
            
            if(index>=32)
            {
                memcpy(reStr,buff,32);
                terninalPrintf("[%d]\n",(reVal));
                for(int i=0;i<sizeof(reStr);i++)
                    terninalPrintf("%c",reStr[i]);
                terninalPrintf("\n");
                return TRUE;
            }
            */
            vTaskDelay(2000/portTICK_RATE_MS);
        }
        vTaskDelay(10/portTICK_RATE_MS);
        timeOut--;
    }
    sysprintf("timeout\n");
    return FALSE;
}


BOOL QModemGetSIMNumber(char* SIMStr)
{
    BOOL time = 2;//10;
    //char* pch1= malloc(100);
    //char* pch2;
    char tempchr[100] ;  //= malloc(100);
    int charCounter;
    int tryCounter = 0;
    //int pchindex;
    cmdActionBreakFlag = FALSE;
    while(time > 0)
    {
        //terninalPrintf(" timeOut == %d\r\n",time); 
       // if(actionCmdPure(&readVersionCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
        if(actionCmdPure(&readSIMNumberCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
        
        {

            int j = 0;
            charCounter=0;
            for(int i=0;i<50;i++)
            {
                
                if(readBuff[i] == 0x2B)  // Real Plus
                    readBuff[i] = 0xD3;
                
                //if(readBuff[j++] == 0x0A)
                if(readBuff[j++] == 0x00)
                {   
                    //charCounter++;
                    //if(charCounter == 2)
                        break;
                }
                   
            }
            

            //terninalPrintf("readBuff[%d]=%02x=%c\r\n",j+2,readBuff[j+2],readBuff[j+2]);
            //terninalPrintf("readBuff[%d]=%02x=%c\r\n",j+3,readBuff[j+3],readBuff[j+3]);
            tryCounter++;
            for(int k=6;k>0;k--)
            {
                //if((readBuff[j+2] == 'O') && (readBuff[j+3] == 'K'))
                    //terninalPrintf("readBuff[%d]=%02x=%c\r\n",j-k,readBuff[j-k],readBuff[j-k]);
                    //terninalPrintf("readBuff[%d]=%02x=%c\r\n",j-k+1,readBuff[j-k+1],readBuff[j-k+1]);
                //if((readBuff[j-k] == 'O') && (readBuff[j-k+1] == 'K'))
                if((readBuff[j-k] == 0x0D) && (readBuff[j-k+1] == 0x0A))
                {

 
                    memcpy(SIMStr,readBuff,100);
                    return TRUE;
                }
                else if(tryCounter == 5)
                {
                   return FALSE; 
                }
            }
        }
        else
        {
            time--;
        }
    }
    cmdActionBreakFlag = TRUE;
    return FALSE;
}

BOOL QModemSetHighSpeed(void)
{
    cmdActionBreakFlag = FALSE;
    pUartInterface->setPowerFunc(TRUE);
    vTaskDelay(10000/portTICK_RATE_MS);
    if(actionCmdPure(&networkSetHighSpeedCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
    {
        sysprintf(" == QModemSetHighSpeed OK\r\n"); 
        //QModemSetPower(FALSE);
        cmdActionBreakFlag = TRUE;
        return TRUE;
    }
    else
    {
        sysprintf(" == QModemSetHighSpeed ERROR\r\n"); 
        //QModemSetPower(FALSE);
        cmdActionBreakFlag = TRUE;
        return FALSE;
    }
}

BOOL QModemSetSleep(void)
{
    if(actionCmdPureEx(&sleepCmdItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
        return TRUE;
    else
        return FALSE;
}

void QModemSetQurccfg(void)
{
    actionCmdPure(&qurccfgCmdItem, PARSER_TYPE_NORMAL);
}
void QModemSetTestCmd(void)
{
    ModemCmdItem CmdItem1 = {(uint8_t*)CHECK_FLOW_CTL_ON_CMD,    networkAtTestCmdRegAction, 500, 2000}; 
    ModemCmdItem CmdItem2 = {(uint8_t*)CHECK_ECHO_OFF_CMD,    networkAtTestCmdRegAction, 500, 2000};
    ModemCmdItem CmdItem3 = {(uint8_t*)CHECK_EXIST_CMD,    networkAtTestCmdRegAction, 500, 2000};
    ModemCmdItem CmdItem4 = {(uint8_t*)CHECK_PIN_CMD,    networkAtTestCmdRegAction, 500, 2000};
    ModemCmdItem CmdItem5 = {(uint8_t*)CFUN_CMD,    networkAtTestCmdRegAction, 500, 2000};
/*
                                            {DIALUP_FLOW_CTL_ON_STAGE_INDEX,    (uint8_t*)CHECK_FLOW_CTL_ON_CMD,           flowCtlOnCmdRegAction,        500, 10*1000, 30},
                                            {DIALUP_ECHO_OFF_STAGE_INDEX,       (uint8_t*)CHECK_ECHO_OFF_CMD,               echooffCmdRegAction,        500, 2000, 30},
                                            {DIALUP_EXIST_STAGE_INDEX,          (uint8_t*)CHECK_EXIST_CMD,                  existCmdRegAction,          1000, 2000, 30},
                                            {DIALUP_PIN_STAGE_INDEX,            (uint8_t*)CHECK_PIN_CMD,                    pinCmdRegAction,            2000, 2000,  2},
    */
    
    //actionCmdPure(&CmdItem1 , PARSER_TYPE_NORMAL);
    //actionCmdPure(&CmdItem2 , PARSER_TYPE_NORMAL);
    //actionCmdPure(&CmdItem3 , PARSER_TYPE_NORMAL);
    //actionCmdPure(&CmdItem4 , PARSER_TYPE_NORMAL);
    actionCmdPure(&CmdItem5 , PARSER_TYPE_NORMAL);
}


BOOL QModemQuerySIMInitStatus(int* para)
{
    char rbuff[50];
    int retnlen;
    
    //if (actionCmdPure(&networkQuerySIMInitStatusItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
    if (actionCmdPurePro(&networkQuerySIMInitStatusItem, PARSER_TYPE_NORMAL,rbuff,sizeof(rbuff),&retnlen,10) == MODEM_RETURN_OK) 
    {    
        if(retnlen>10)
        {
            if((rbuff[retnlen-3] == 'K') && (rbuff[retnlen-4] == 'O') )
            {
                *para = rbuff[retnlen-9] & 0x07;
                return TRUE;
            }
        }

    }
    return FALSE;
}


BOOL QModemQueryNTP(UINT32* rYear,UINT32* rMonth,UINT32* rDay,UINT32* rHour,UINT32* rMinute,UINT32* rSecond)
{
    char rbuff[100];
    int retnlen;
    
    //actionCmdPure(&networkNTPEnableWriteItem, PARSER_TYPE_NORMAL);
    //actionCmdPure(&networkNTPEnableReadItem, PARSER_TYPE_NORMAL);
    //if (actionCmdPure(&networkQueryNTPWriteItem, PARSER_TYPE_NORMAL) == MODEM_RETURN_OK)
    //{
        //if (actionCmdPurePro(&networkQueryNTPReadItem, PARSER_TYPE_NORMAL,rbuff,sizeof(rbuff),&retnlen, 100) == MODEM_RETURN_OK)
        if (actionCmdPurePro(&networkQueryNTPItem, PARSER_TYPE_NORMAL,rbuff,sizeof(rbuff),&retnlen, 100) == MODEM_RETURN_OK)    
        { 
            //actionCmdPurePro(&networkQueryNTPReadItem, PARSER_TYPE_NORMAL,rbuff,sizeof(rbuff),&retnlen, 50);
            if(retnlen>32)
            {
                if((rbuff[retnlen-3] == 'K') && (rbuff[retnlen-4] == 'O') )
                {
                    *rYear      = (rbuff[retnlen-31] & 0x0F)*10 + (rbuff[retnlen-30] & 0x0F);
                    *rMonth     = (rbuff[retnlen-28] & 0x0F)*10 + (rbuff[retnlen-27] & 0x0F);
                    *rDay       = (rbuff[retnlen-25] & 0x0F)*10 + (rbuff[retnlen-24] & 0x0F);
                    *rHour      = (rbuff[retnlen-22] & 0x0F)*10 + (rbuff[retnlen-21] & 0x0F);
                    *rMinute    = (rbuff[retnlen-19] & 0x0F)*10 + (rbuff[retnlen-18] & 0x0F);
                    *rSecond    = (rbuff[retnlen-16] & 0x0F)*10 + (rbuff[retnlen-15] & 0x0F);
                    return TRUE;
                }
            }
        }
    //}
    return FALSE;
}


BOOL QModemTerminal(char* CmdString,char* FBCmdStr,int FBCmdSize,int* retlen,int waitime)
{
    
    ModemCmdItem tempCmdItem = {(uint8_t*)CmdString, networkQueryIPAddressRegAction, 300, 1500};
    actionCmdPurePro(&tempCmdItem, PARSER_TYPE_NORMAL,FBCmdStr,FBCmdSize,retlen, waitime);
    
}
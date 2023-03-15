/**************************************************************************//**
* @file     atcmdparser.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __AT_CMD_PARSER_H__
#define __AT_CMD_PARSER_H__
#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include <nuc970.h>

#ifdef __cplusplus
extern "C" {
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define CHECK_AT_TEST_CMD          "AT\r\n"
#define CHECK_AT_TEST_VER_CMD      "AT+GMR\r\n"
#define SLEEP_CMD                  "AT+QSCLK=1\r\n"
#define QURCCFG_CMD                  "AT+QURCCFG=\"urcport\",\"uart1\"\r\n" 
#define CFUN_CMD                   "AT+CFUN?\r\n"
    
#define QUERY_SIM_INIT_STATUS_CMD  "AT+QINISTAT\r\n" 
    
#define AUTOMATIC_TIME_ZONE_UPDATE_W_CMD    "AT+CTZR=1\r\n" 
#define AUTOMATIC_TIME_ZONE_UPDATE_R_CMD    "AT+CTZU?\r\n"  //"AT+CCLK?\r\n"
#define QUERY_TIME_ZONE_W_CMD               "AT+CTZR=2\r\n" 
#define QUERY_TIME_ZONE_R_CMD               "AT+CTZR?\r\n" 
    
#define QUERY_NETWORK_TIME_CMD              "AT+QLTS=2\r\n" 
    
    
#if(ENABLE_MODEM_FLOW_CONTROL)
    #define CHECK_FLOW_CTL_ON_CMD       "AT+IFC=2,2\r\n"
#else
    #define CHECK_FLOW_CTL_ON_CMD       "AT+IFC=0,0\r\n"
#endif
#define CHECK_ECHO_OFF_CMD          "ATE0\r\n"
    
#define CHECK_EXIST_CMD             "AT+CPIN?\r\n"
#define CHECK_PIN_CMD              "AT+CPIN=0000\r\n"  //"AT+CPIN=1234\r\n"
#define CHECK_REG_CMD               "AT+CREG?\r\n"
    
#define CHECK_SIM_NUM_CMD               "AT+CIMI\r\n"    
    
//#define CHECK_OPS_CMD               "AT+COPS?\r\n"   
#define CHECK_GATT_CMD              "AT+CGATT?\r\n"    
#define CHECK_GREG_CMD              "AT+CGREG?\r\n" 
#define CHECK_CONTEXT_SETTING_CMD   "AT+QICSGP=1,1,\"internet\",\"\",\"\",0\r\n"
#define CHECK_CONNECTING_CMD        "AT+QIACT=1\r\n"
#define CHECK_DISCONNECT_CMD        "AT+QIDEACT=1\r\n"
#define CHECK_QUERY_CONNECTED_CMD   "AT+QIACT?\r\n"
    
#define FTP_SET_PDP_CMD             "AT+QFTPCFG=\"contextid\",1\r\n"
#define FTP_SET_USER_INFO_CMD(id, passwd)       "AT+QFTPCFG=\"account\",\""id"\",\""passwd"\"\r\n"
#define FTP_SET_FILE_TYPE_CMD       "AT+QFTPCFG=\"filetype\",0\r\n"
#define FTP_SET_TRANSFER_MODE_CMD   "AT+QFTPCFG=\"transmode\",1\r\n"   //passive
//#define FTP_SET_TRANSFER_MODE_CMD   "AT+QFTPCFG=\"transmode\",0\r\n"    //active
#define FTP_SET_TIMEOUT_CMD         "AT+QFTPCFG=\"rsptimeout\",90\r\n"
#define FTP_CONNECTING_CMD(address, port) "AT+QFTPOPEN=\""address"\","port"\r\n"
#define FTP_DISCONNECT_CMD          "AT+QFTPCLOSE\r\n"
#define FTP_QUERY_CSQ_CMD          "AT+CSQ\r\n"

#define SET_IPR_CMD              "AT+IPR=921600;&W\r\n" 
//#define FTP_GET_DATA_CMD          "AT+QHTTPREAD=80\r\n"
    
//#define FTP_CONNECT_CMD             "AT+QFTPOPEN=\"54.249.1.95\",21\r\n"


typedef enum{
	CMD_REQ_NULL            = 0,
	CMD_REQ_OK              = 100,
	CMD_REQ_ERROR           = 200, //include timeout
    CMD_REQ_CME_ERROR       = 300,
	CMD_REQ_EXIST_OK        = 101,
	CMD_REQ_REG_OK          = 102,
    CMD_REQ_GATT_OK         = 103,
    CMD_REQ_GREG_OK         = 104,
    CMD_REQ_QUERY_CONNECTED_OK = 105,
    CMD_REQ_QUERY_FTP_CONNECTED_OK = 106,
    CMD_REQ_CONNECT_OK      = 107,
    CMD_REQ_FTP_SEND_DATA_OK = 108,
    CMD_REQ_FTP_SEND_DATA_TIMEOUT = 109,
    CMD_REQ_FTP_DISCONNECT = 110,
    CMD_REQ_WEB_POST_OK = 111,
    CMD_REQ_WEB_POST_GET_DATA = 112,
    CMD_REQ_FTP_CHANGE_DIR_OK = 113,
    CMD_REQ_FTP_CHANGE_DIR_ERROR = 114,
    CMD_REQ_FTP_GET_DATA_OK = 115,
    CMD_REQ_FTP_MAKE_DIR_OK = 116,
    CMD_REQ_FTP_MAKE_DIR_ERROR = 117,
    CMD_REQ_FTP_GET_DIR_OK = 118,
    CMD_REQ_FTP_GET_DIR_ERROR = 119,
    CMD_REQ_QUERY_CSQ_OK = 120,
    CMD_REQ_NEED_PIN_OK = 121,
    CMD_REQ_REBOOT_OK       = 1100
}CmdReq;

typedef enum{
    PARSER_TYPE_NORMAL = 0x01,
    PARSER_TYPE_WEB_POST = 0x02,
    PARSER_TYPE_FTP = 0x03
}ParserType;

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/    
  

CmdReq atCmdProcessReadData(uint8_t* data, int len, ParserType parserType);
void ATCmdSetReceiveDebugFlag(int flag);
uint8_t* ATCmdDataTempBuffer(int* len);
BOOL ATCmdGetReceiveDebugFlag(void);
uint8_t* ATCmdDataTempBuffer2(int* len);
#ifdef __cplusplus
}
#endif

#endif /* __AT_CMD_PARSER_H__ */


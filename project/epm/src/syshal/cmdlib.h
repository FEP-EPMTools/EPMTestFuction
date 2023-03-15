/**************************************************************************//**
* @file     cmddrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __CMD_DRV_H__
#define __CMD_DRV_H__

#include "nuc970.h"
#include "dataagent.h"
#ifdef __cplusplus
extern "C"
{
#endif


/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define CMD_VERSION_ID              0x01   
#define CMD_TIME_CORRECTION_ID		0x02  
#define CMD_TRANSACTION_DATA_ID		0x03
#define CMD_ACK_ID                  0x04
#define CMD_EXCEED_TIME_ID		    0x05
#define CMD_BATTERY_FAIL_ID		    0x06
#define CMD_FILE_FRANSFER_ID		0x07
#define CMD_FILE_FRANSFER_DATA_ID   0x08
#define CMD_RESET_DEVICE_ID         0x09
    
#define CMD_HEADER_VALUE            0xda   
#define CMD_HEADER_VALUE2		    0xad  
    
#define CMD_END_VALUE               0x1f   
#define CMD_END_VALUE2		        0xf1  

#pragma pack(1)
typedef struct
{
    uint8_t         value[2];    //2 byte
    uint16_t        cmdLen;
    uint32_t        deviceId;
    uint16_t        cmdIndex;    
    uint8_t         cmdId;
    uint16_t        paraLen;
}CmdHeader; //9 bytes


typedef struct
{
    uint16_t    checksum;
    uint8_t     value[2];    //2 byte
}CmdEnd; //4 bytes

typedef struct
{    
    uint8_t     majorVer;
    uint8_t     minorVer;
    uint8_t     revisionVer;
    uint32_t    buildVer;
    uint32_t    currentTime;
    uint32_t    leftStartTime; 
    uint32_t    rightStartTime;
    uint32_t    leftEndTime; 
    uint32_t    rightEndTime;
    uint16_t    voltageValueL;
    uint16_t    voltageValueR;
    uint8_t     spaceStatus;
    uint8_t     depositStatus;
    uint32_t    wakeupTime;
}versionPara; 

typedef struct
{
    CmdHeader   header;
    versionPara para;    
    CmdEnd      end;  
}CmdVersion; 


//---
typedef struct
{    
    uint32_t    u32Year;
    uint8_t     u8cMonth;
    uint8_t     u8cDay;
    uint8_t     u8cHour;
    uint8_t     u8cMinute;
    uint8_t     u8cSecond;
    uint8_t     u8cDayOfWeek;
}timePara; 

typedef struct
{
    CmdHeader   header;
    timePara    para;
    CmdEnd      end;  
}CmdTimeCorrection;
//---
typedef struct
{    
    uint32_t    deviceId;
    uint16_t    cmdIndex;
}ackPara; 

typedef struct
{
    CmdHeader   header;
    ackPara     para;
    CmdEnd      end;  
}CmdAck;
//--
typedef struct
{
    uint8_t     spaceId;
    uint32_t    currentTime;
    uint32_t    endTime; 
    uint32_t    depositTime;    
    uint16_t    cost;
    uint16_t    balance;  
}transactionPara;

typedef struct
{
    CmdHeader           header;
    transactionPara     para;
    CmdEnd              end;  
}CmdTransaction;

//--
typedef struct
{
    uint8_t     spaceId;
    uint32_t    currentTime;
    uint32_t    endTime;    
}exceedTimePara;

typedef struct
{
    CmdHeader           header;
    exceedTimePara      para;
    CmdEnd              end;  
}CmdExceedTime;
//--
typedef struct
{
    uint8_t     batteryId;
    uint16_t    voltage;  
}batteryFailPara;

typedef struct
{
    CmdHeader           header;
    batteryFailPara     para;
    CmdEnd              end;  
}CmdBatteryFail;

//-file transfer -
#define FILE_TRANSFER_NAME_LEN  32
#define FILE_TRANSFER_CMD_STATUS_START  0x01
#define FILE_TRANSFER_CMD_STATUS_END    0x02
typedef struct
{
    char        fileName[FILE_TRANSFER_NAME_LEN];
    uint16_t    fileLen; 
    uint8_t     statusId; //start, end 
}fileTransferPara;

typedef struct
{
    CmdHeader               header;
    fileTransferPara        para;
    CmdEnd                  end;  
}CmdFileTransfer;

//-file transfer data-
#define FILE_TRANSFER_DATA_LEN  1024
typedef struct
{
    uint16_t        startAddress;
    uint16_t        dataLen; 
    uint8_t         data[FILE_TRANSFER_DATA_LEN];
}fileTransferDataPara;

typedef struct
{
    CmdHeader               header;
    fileTransferDataPara    para;
    CmdEnd                  end;  
}CmdFileTransferData;

//-reset device-
typedef struct
{    
    uint32_t    deviceId;
    uint8_t     spaceId;
}resetDevicePara; 

typedef struct
{
    CmdHeader   header;
    resetDevicePara    para;
    CmdEnd      end;  
}CmdResetDevice;
//---

#pragma pack()

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/

BOOL CmdSendVersion(uint32_t wakeupTime);
BOOL CmdSendTransaction(uint8_t spaceId, uint32_t currentTime, uint32_t endTime, uint32_t depositTime, uint16_t cost, uint16_t balance);
void CmdProcessReadData(uint8_t* data, uint16_t len);
BOOL CmdSendExceedTime(uint8_t spaceId, uint32_t currentTime, uint32_t depositTime);
BOOL CmdSendBatteryFail(uint8_t batteryId, uint16_t voltage);
BOOL CmdSendFileTransfer(char* fileName, uint16_t fileLen, uint8_t statusId, dataAgentTxCallback callback);
BOOL CmdSendFileTransferData(uint16_t startAddress, uint16_t dataLen, uint8_t* data, dataAgentTxCallback callback);
#ifdef __cplusplus
}
#endif

#endif//__CMD_DRV_H__

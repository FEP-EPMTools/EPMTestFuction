/**************************************************************************//**
* @file     cmddrv.c
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
#include "gpio.h"
#include "rtc.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "dataagent.h"
#include "interface.h"
#include "powerdrv.h"
#include "cmdlib.h"
#include "spacedrv.h"
#include "paralib.h"
#include "batterydrv.h"
#include "timelib.h"
#include "guimanager.h"
#include "meterdata.h"
#include "tarifflib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ENABLE_CMD_LIB_DEBUG_MESSAGE 1
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

//static CmdHeader    CmdHeaderDefault    = {0xda, 0xad, 0,  0, 0,};
//static CmdEnd       CmdEndDefault       = {0x1f, 0xf1};

static uint16_t mCommandIndex = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static uint16_t getChecksum(uint8_t* pTarget, uint16_t len, char* str)
{
    int i;
    uint16_t checksum = 0;
    uint8_t* pr = (uint8_t*)pTarget;
    for(i = 2; i< len - sizeof(uint16_t) - sizeof(CmdEnd); i++) //???checksum ?????
    {
        checksum = checksum + pr[i];
    }
    //sysprintf("  -- getChecksum (%s) : checksum = 0x%x (%d)\r\n", str, checksum, checksum); 
    return checksum;
}

static void initCmd(uint8_t cmdId, void* data, void* para)
{
    uint16_t cmdLen, paraLen;
    CmdHeader* pCmdHeader;
    CmdEnd* pCmdEnd;
    uint8_t* pPara;
    char* pStr;
    
    switch(cmdId)
    {
        case CMD_VERSION_ID: 
        {
            CmdVersion* pCmd = (CmdVersion*)data;
            cmdLen = sizeof(CmdVersion);
            paraLen = sizeof(versionPara);
            
            pCmdHeader = &(pCmd->header);
            pCmdEnd = &(pCmd->end);
            pPara = (uint8_t*)&(pCmd->para);
            pStr = "CMD_VERSION_ID";
        }
            break;
        case CMD_TRANSACTION_DATA_ID: 
        {
            CmdTransaction* pCmd = (CmdTransaction*)data;
            cmdLen = sizeof(CmdTransaction);
            paraLen = sizeof(transactionPara);
            
            pCmdHeader = &(pCmd->header);
            pCmdEnd = &(pCmd->end);
            pPara = (uint8_t*)&(pCmd->para);
            pStr = "CMD_TRANSACTION_DATA_ID";
        }
            break;
        case CMD_EXCEED_TIME_ID: 
        {
            CmdExceedTime* pCmd = (CmdExceedTime*)data;
            cmdLen = sizeof(CmdExceedTime);
            paraLen = sizeof(exceedTimePara);
            
            pCmdHeader = &(pCmd->header);
            pCmdEnd = &(pCmd->end);
            pPara = (uint8_t*)&(pCmd->para);
            pStr = "CMD_EXCEED_TIME_ID";
        }        
            break;   
        case CMD_BATTERY_FAIL_ID: 
        {
            CmdBatteryFail* pCmd = (CmdBatteryFail*)data;
            cmdLen = sizeof(CmdBatteryFail);
            paraLen = sizeof(batteryFailPara);
            
            pCmdHeader = &(pCmd->header);
            pCmdEnd = &(pCmd->end);
            pPara = (uint8_t*)&(pCmd->para);
            pStr = "CMD_BATTERY_FAIL_ID";
        }   
            break;  
        case CMD_FILE_FRANSFER_ID: 
        {
            CmdFileTransfer* pCmd = (CmdFileTransfer*)data;
            cmdLen = sizeof(CmdFileTransfer);
            paraLen = sizeof(fileTransferPara);
            
            pCmdHeader = &(pCmd->header);
            pCmdEnd = &(pCmd->end);
            pPara = (uint8_t*)&(pCmd->para);
            pStr = "CMD_FILE_FRANSFER_ID";
        }   
            break;  
        case CMD_FILE_FRANSFER_DATA_ID: 
        {
            CmdFileTransferData* pCmd = (CmdFileTransferData*)data;
            cmdLen = sizeof(CmdFileTransferData);
            paraLen = sizeof(fileTransferDataPara);
            
            pCmdHeader = &(pCmd->header);
            pCmdEnd = &(pCmd->end);
            pPara = (uint8_t*)&(pCmd->para);
            pStr = "CMD_FILE_FRANSFER_DATA_ID";
        }   
            break;  
        
        
        default:
            return;
    }
    memset(data, 0x0, cmdLen);
    //memcpy(&(data->header), &CmdHeaderDefault, sizeof(CmdHeader));  
    pCmdHeader->value[0] = CMD_HEADER_VALUE;
    pCmdHeader->value[1] = CMD_HEADER_VALUE2;
    
    pCmdHeader->cmdLen = cmdLen;  
    pCmdHeader->deviceId = GetMeterPara()->epmid;//  GPIO_ReadBit(GPIOH, BIT11)+1;////DeviceID;
    pCmdHeader->cmdIndex = mCommandIndex++;    
    pCmdHeader->cmdId = cmdId;    
    pCmdHeader->paraLen = paraLen;
    
    memcpy(pPara, para, paraLen);
    
    pCmdEnd->checksum = getChecksum((uint8_t*)data, cmdLen, pStr); 
    
    pCmdEnd->value[0] = CMD_END_VALUE;
    pCmdEnd->value[1] = CMD_END_VALUE2;
}
/*
static BOOL sendCmd(uint8_t* data, uint8_t len)
{
    INT32 sendLen = LoraWrite(data, len);
    if(sendLen != len)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
*/
//extern CmdHeader    CmdHeaderDefault;
//extern CmdEnd       CmdEndDefault;
static uint8_t cmdDataTmp[256];
static int cmdDataIndex = -1;
static int cmdDataNeedLen = -1;

static void checkcmd(uint8_t* data, uint16_t len)
{
	CmdHeader headerTmp;
	CmdEnd endTmp;
    uint16_t checksumTmp;
	memcpy(&headerTmp, data, sizeof(CmdHeader));
	memcpy(&endTmp, data + len - sizeof(CmdEnd), sizeof(CmdEnd));
	if ((headerTmp.value[0] != CMD_HEADER_VALUE) || (headerTmp.value[1] != CMD_HEADER_VALUE2))
	{
		sysprintf("CmdHeader error !!\r\n");
		return;
	}

	if ((endTmp.value[0] != CMD_END_VALUE) || (endTmp.value[1] != CMD_END_VALUE2))
	{
		sysprintf("CmdEnd error !!\r\n");
		return;
	}


	checksumTmp = getChecksum((uint8_t*)data, len, "checkcmd");
	if (checksumTmp != endTmp.checksum)
	{
		sysprintf("checksum error [0x%04x: 0x%04x]!!\r\n", checksumTmp, endTmp.checksum);
		return;
	}
#if(BUILD_DEBUG_VERSION)
	sysprintf("[CMD INFO] Command ID: 0x%02x (index: %d), from device: %d\r\n", headerTmp.cmdId, headerTmp.cmdIndex, headerTmp.deviceId);
#endif
	switch (headerTmp.cmdId)
	{
	case CMD_VERSION_ID:
		{//DA AD 0C 01 03 14 16 18 52 00 1F F1
            CmdVersion* pCmdVersion = (CmdVersion*)data;
            
            if (pCmdVersion->header.paraLen != sizeof(versionPara))
            { 
                sysprintf(" --> CMD_VERSION_ID len error: %d:%d\r\n", pCmdVersion->header.paraLen, sizeof(versionPara));
            }
            else
            {
                sysprintf(" --> CMD_VERSION_ID: %03d, %03d, %03d\r\n", pCmdVersion->para.majorVer, pCmdVersion->para.minorVer, pCmdVersion->para.revisionVer);
            }
		}
		break;
    
    case CMD_TIME_CORRECTION_ID:
		{//DA AD 13 02 0A E0 07 00 00 0A 1B 0D 18 27 04 7B 01 1F F1
            

            CmdTimeCorrection* pCmdTimeCorrection = (CmdTimeCorrection*)data; 
            if (pCmdTimeCorrection->header.paraLen != sizeof(timePara))
            { 
                sysprintf(" --> CMD_TIME_CORRECTION_ID len error: %d:%d\r\n", pCmdTimeCorrection->header.paraLen, sizeof(timePara));
                break;
            }            
            sysprintf(" --> CMD_TIME_CORRECTION_ID : [%04d/%02d/%02d %02d:%02d:%02d (%d)]\r\n",
                                                pCmdTimeCorrection->para.u32Year, pCmdTimeCorrection->para.u8cMonth, pCmdTimeCorrection->para.u8cDay, 
                                                pCmdTimeCorrection->para.u8cHour, pCmdTimeCorrection->para.u8cMinute, pCmdTimeCorrection->para.u8cSecond, pCmdTimeCorrection->para.u8cDayOfWeek);
            #if(JUST_TEST_LORA_CMD)
            #else
            #if(1)
            {
                RTC_TIME_DATA_T time;
                time_t      rawtime;
                time.u32Year = pCmdTimeCorrection->para.u32Year;
                time.u32cMonth = pCmdTimeCorrection->para.u8cMonth;
                time.u32cDay = pCmdTimeCorrection->para.u8cDay;
                time.u32cHour = pCmdTimeCorrection->para.u8cHour;
                time.u32cMinute = pCmdTimeCorrection->para.u8cMinute;
                time.u32cSecond = pCmdTimeCorrection->para.u8cSecond;
                time.u32cDayOfWeek = pCmdTimeCorrection->para.u8cDayOfWeek;//no care
                
                rawtime =  RTC2Time(&time);
                
                Time2RTC(rawtime, &time);
                
      
                sysprintf(" --> CMD_TIME_CORRECTION_ID _!!!! : [%04d/%02d/%02d %02d:%02d:%02d (%d)]\r\n",
                                                    time.u32Year, time.u32cMonth, time.u32cDay, 
                                                    time.u32cHour, time.u32cMinute, time.u32cSecond, time.u32cDayOfWeek);
                //void SetOSTime(uint32_t u32Year, uint32_t u32cMonth, uint32_t u32cDay, uint32_t u32cHour, uint32_t u32cMinute, uint32_t u32cSecond, uint32_t u32cDayOfWeek);
                if(SetOSTime(time.u32Year, time.u32cMonth, time.u32cDay, time.u32cHour, time.u32cMinute, time.u32cSecond, time.u32cDayOfWeek) == TRUE)
                {
                    //RefreshMainScreen();
                    //RefreshSpaceStatusScreen();
                    TariffUpdateCurrentTariffData();
                    GuiManagerRefreshScreen();
                }
            }
            #endif
            #endif
		}
		break;
    case CMD_ACK_ID:
		{
            CmdAck* pCmdAck = (CmdAck*)data;  
            if (pCmdAck->header.paraLen != sizeof(ackPara))
            { 
                sysprintf(" --> CMD_ACK_ID len error: %d:%d\r\n", pCmdAck->header.paraLen, sizeof(ackPara));
                break;
            } 
            #if(BUILD_DEBUG_VERSION)            
            sysprintf(" --> CMD_ACK_ID : cmdIndex = %d...\r\n", pCmdAck->para.cmdIndex);
            #endif
            if(pCmdAck->para.deviceId == GetMeterPara()->epmid/*(GPIO_ReadBit(GPIOH, BIT11)+1)*/)
            {
                DataAgentRemoveDataByAck(pCmdAck->para.cmdIndex);
            }
            else
            {
                #if(BUILD_DEBUG_VERSION)            
                sysprintf(" --> CMD_ACK_ID : deviceId = %d (local : %d)...\r\n", pCmdAck->para.deviceId, GetMeterData()->deviceId/*(GPIO_ReadBit(GPIOH, BIT11)+1)*/);
                #endif
            }
		}
		break;    
    case CMD_RESET_DEVICE_ID:
		{
            CmdResetDevice* pCmdResetDevice = (CmdResetDevice*)data;  
            if (pCmdResetDevice->header.paraLen != sizeof(resetDevicePara))
            { 
                sysprintf(" --> CMD_RESET_DEVICE_ID len error: %d:%d\r\n", pCmdResetDevice->header.paraLen, sizeof(resetDevicePara));
                break;
            } 
            #if(BUILD_DEBUG_VERSION)            
            sysprintf(" --> CMD_RESET_DEVICE_ID : deviceId = %d...\r\n", pCmdResetDevice->para.deviceId);
            #endif
            if(pCmdResetDevice->para.deviceId == GetMeterPara()->epmid/*(GPIO_ReadBit(GPIOH, BIT11)+1)*/)
            {
                ParaLibResetDepositEndTime();
                MeterStorageFlush();
                AutoUpdateMeterData();
                GuiManagerRefreshScreen();
            }
            else
            {
                #if(BUILD_DEBUG_VERSION)            
                sysprintf(" --> CMD_ACK_ID : deviceId = %d (local : %d)...\r\n", pCmdAck->para.deviceId, GetMeterData()->deviceId/*(GPIO_ReadBit(GPIOH, BIT11)+1)*/);
                #endif
            }
		}
		break;           
	default:
		sysprintf("[ERROR]cmd id error [0x%02x]!!", data[3]);
		break;
	}
}



void CmdProcessReadData(uint8_t* data, uint16_t len)
{
	int i;
	int cpyLen;
	BOOL dumpFlag = TRUE;
	for (i = 0; i < len;)
	{
		//if (memcmp(data + i, &CmdHeaderDefault, sizeof(CmdHeader)) == 0)
        if ((data[i] == CMD_HEADER_VALUE) && (data[i+1] == CMD_HEADER_VALUE2))
		{
			memset(cmdDataTmp, 0x0, sizeof(cmdDataTmp));
			cmdDataIndex = 0;
			cpyLen = 2;//sizeof(CmdHeader);
			memcpy(cmdDataTmp, data + i, cpyLen);

			i = i + cpyLen;
			cmdDataIndex = cmdDataIndex + cpyLen;

			cmdDataTmp[cmdDataIndex] = data[i];
			cmdDataNeedLen = data[i];
			i++;
			cmdDataIndex++;

			cmdDataNeedLen = cmdDataNeedLen - 3;
            //sysprintf("[CmdProcessReadData] get headerCmd!! (cmdDataNeedLen = %d)\r\n", cmdDataNeedLen); 
			dumpFlag = FALSE;
		}
		/*
		else if (memcmp(data + i, &CmdEndDefault, sizeof(CmdEnd)) == 0)
		{
			if (ileft >= sizeof(CmdEnd))
			{
				cpyLen = sizeof(CmdEnd);
			}
			else
			{
				cpyLen = ileft;
			}
			memcpy(cmdDataTmp + cmdDataIndex, data + i, cpyLen);
			ileft = ileft - cpyLen;
			i = i + cpyLen;
			cmdDataIndex = cmdDataIndex + cpyLen;
			checkcmd(cmdDataTmp, cmdDataIndex);
		}
		*/
		else
		{
			if (cmdDataIndex > 0)
			{
				cmdDataTmp[cmdDataIndex] = data[i];
				cmdDataIndex++;
				i++;
				cmdDataNeedLen--;
				if (cmdDataNeedLen == 0)
				{
                    //sysprintf("[CmdProcessReadData] receive end\r\n"); 
					checkcmd(cmdDataTmp, cmdDataIndex);
					cmdDataIndex = -1;
					cmdDataNeedLen = -1;
				}
				dumpFlag = FALSE;
			}
			else
			{
				i++;
			}
		}
	}
	if (dumpFlag)
	{
        sysprintf("[CmdProcessReadData] data ignore !! dump data:\r\n   ==> ");
		for (int i = 0; i < len; i++)
		{
			sysprintf("[0x%02x], ", data[i]);
		}
        sysprintf("\r\n");
	}
	
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL CmdSendVersion(uint32_t wakeupTime)
{

    RTC_TIME_DATA_T pt;
    UINT32 leftVoltage, rightVoltage;
    //static int timers = 0;
    //DA AD 01 03 01 03 05 0D 00 1F F1
    CmdVersion cmd;
    versionPara pPara;
    pPara.majorVer = MAJOR_VERSION;
    pPara.minorVer = MINOR_VERSION;
    pPara.revisionVer = REVISION_VERSION;
    pPara.buildVer = GetMeterData()->buildVer;//BUILD_VERSION;
    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        pPara.currentTime = RTC2Time(&pt);
    }
    else
    {
        pPara.currentTime = 0;
    }
    pPara.leftStartTime = GetMeterStorageData()->depositStartTime[GetMeterPara()->meterPosition - 1];
    pPara.rightStartTime = GetMeterStorageData()->depositStartTime[GetMeterPara()->meterPosition];
    
    pPara.leftEndTime = GetMeterStorageData()->depositEndTime[GetMeterPara()->meterPosition - 1];
    pPara.rightEndTime = GetMeterStorageData()->depositEndTime[GetMeterPara()->meterPosition];
    
    BatteryGetValue(&rightVoltage, &leftVoltage);

    pPara.voltageValueL = leftVoltage;
    pPara.voltageValueR = rightVoltage;
    
    pPara.spaceStatus = 0;
    if(GetSpaceStatus(SPACE_INDEX_1))
    {
        pPara.spaceStatus = pPara.spaceStatus | (0x1<<0);
    }
    if(GetSpaceStatus(SPACE_INDEX_2))
    {
        pPara.spaceStatus = pPara.spaceStatus | (0x1<<1);
    }
    
    pPara.depositStatus = ((GetMeterData()->spaceSepositStatus[GetMeterPara()->meterPosition - 1]&0xf) << 0) | 
                                ((GetMeterData()->spaceSepositStatus[GetMeterPara()->meterPosition]&0xf) << 4);
    pPara.wakeupTime = wakeupTime;
    
    //timers++;
    initCmd(CMD_VERSION_ID, &cmd, &pPara);
    #if(ENABLE_CMD_LIB_DEBUG_MESSAGE)
    sysprintf("  -- CmdSendVersion ...\r\n"); 
    #endif
    return DataAgentAddData((uint8_t*)&cmd, sizeof(CmdVersion), TRUE, NULL);

}

BOOL CmdSendTransaction(uint8_t spaceId, uint32_t currentTime, uint32_t endTime, uint32_t depositTime, uint16_t cost, uint16_t balance)
{
    //static int timers = 0;
    CmdTransaction cmd;
    transactionPara pPara;
    //timers++;
    /*
    typedef struct
    {
        uint8_t     spaceId;
        uint32_t    currentTime;
        uint32_t    depositTime;    
        uint16_t    cost;
        uint16_t    balance;  
    }transactionPara;
    */
    pPara.spaceId = spaceId;
    pPara.currentTime = currentTime;
    pPara.endTime = endTime;   
    pPara.depositTime = depositTime;    
    pPara.cost = cost;
    pPara.balance = balance; 
   
    initCmd(CMD_TRANSACTION_DATA_ID, &cmd, &pPara);
    #if(ENABLE_CMD_LIB_DEBUG_MESSAGE)
    sysprintf("  -- CmdSendTransaction ...\r\n"); 
    #endif
    //return sendCmd((uint8_t*)&cmd, sizeof(CmdVersion));
    return DataAgentAddData((uint8_t*)&cmd, sizeof(CmdTransaction), TRUE, NULL);
}

BOOL CmdSendExceedTime(uint8_t spaceId, uint32_t currentTime, uint32_t endTime)
{
    //static int timers = 0;
    CmdExceedTime cmd;
    exceedTimePara pPara;
    //timers++;
    /*
    typedef struct
    {
        uint8_t     spaceId;
        uint32_t    currentTime;
        uint32_t    depositTime;    
        uint16_t    cost;
        uint16_t    balance;  
    }transactionPara;
    */
    pPara.spaceId = spaceId;
    pPara.currentTime = currentTime;
    pPara.endTime = endTime;    
   
    initCmd(CMD_EXCEED_TIME_ID, &cmd, &pPara);
    #if(ENABLE_CMD_LIB_DEBUG_MESSAGE)
    sysprintf("  -- CmdSendExceedTime ...\r\n"); 
    #endif
    //return sendCmd((uint8_t*)&cmd, sizeof(CmdVersion));
    return DataAgentAddData((uint8_t*)&cmd, sizeof(CmdExceedTime), TRUE, NULL);
}

BOOL CmdSendBatteryFail(uint8_t batteryId, uint16_t voltage)
{
    //static int timers = 0;
    CmdBatteryFail cmd;
    batteryFailPara pPara;
    pPara.batteryId = batteryId;
    pPara.voltage = voltage;   
   
    initCmd(CMD_BATTERY_FAIL_ID, &cmd, &pPara);
    #if(ENABLE_CMD_LIB_DEBUG_MESSAGE)
    sysprintf("  -- CmdSendBatteryFail (%d, %d) ...\r\n", batteryId, voltage); 
    #endif
    //return sendCmd((uint8_t*)&cmd, sizeof(CmdVersion));
    return DataAgentAddData((uint8_t*)&cmd, sizeof(CmdBatteryFail), FALSE, NULL);
}

BOOL CmdSendFileTransfer(char* fileName, uint16_t fileLen, uint8_t statusId, dataAgentTxCallback callback)
{
    //static int timers = 0;
    CmdFileTransfer cmd;
    fileTransferPara pPara;
    strcpy(pPara.fileName, fileName);
    pPara.fileLen = fileLen;  
    pPara.statusId = statusId;      
   
    initCmd(CMD_FILE_FRANSFER_ID, &cmd, &pPara);
    #if(ENABLE_CMD_LIB_DEBUG_MESSAGE)
    sysprintf("  -- CmdSendFileTransfer (%s, %d, %d) ...\r\n", fileName, fileLen, statusId); 
    #endif
    //return sendCmd((uint8_t*)&cmd, sizeof(CmdVersion));
    return DataAgentAddData((uint8_t*)&cmd, sizeof(CmdFileTransfer), TRUE, callback);
}

BOOL CmdSendFileTransferData(uint16_t startAddress, uint16_t dataLen, uint8_t* data, dataAgentTxCallback callback)
{
    //static int timers = 0;
    CmdFileTransferData cmd;
    fileTransferDataPara pPara;
    pPara.startAddress = startAddress;
    pPara.dataLen = dataLen;
    memcpy(pPara.data, data, dataLen);
   
    initCmd(CMD_FILE_FRANSFER_DATA_ID, &cmd, &pPara);
    #if(ENABLE_CMD_LIB_DEBUG_MESSAGE)
    sysprintf("  -- CmdSendFileTransferData (0x%04x, %d) ...\r\n", startAddress, dataLen); 
    #endif
    //return sendCmd((uint8_t*)&cmd, sizeof(CmdVersion));
    return DataAgentAddData((uint8_t*)&cmd, sizeof(CmdFileTransferData), TRUE, callback);
}

/*** (C) COPYRIGHT 2016 Far Easy Pass LTD. All rights reserved. ***/


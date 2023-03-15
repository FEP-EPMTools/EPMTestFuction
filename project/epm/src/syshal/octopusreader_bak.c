/**************************************************************************//**
* @file     octopusreader.c
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
#ifdef _PC_ENV_
    #include "misc.h"
    #include "interface.h"
    #include "halinterface.h"
    #include "octopusreader.h"
    #include "blkcommon.h"
	#include "ipasslib.h"
    #include "ecclib.h"
    
    #define sysprintf       miscPrintf//printf
    #define pvPortMalloc    malloc
    #define vPortFree       free
    #define EPM_READER_UART				UART_INTERFACE_INDEX
#else
    #include "nuc970.h"
    #include "sys.h"
    #include "rtc.h"
    #include "gpio.h"
    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "fepconfig.h"
    #include "octopusreader.h"
    #include "interface.h"    
    #include "fileagent.h"
    #include "loglib.h"
    #include "rwl.h"
    #include "csrw.h"
    #include "octopusreader.h"
    #include "timelib.h"
    #include "paralib.h"
//    #include "projectconfig.h"
    #define OCTOPUS_READER_UART             UART_2_INTERFACE_INDEX
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define UART_BARDRATE				921600
#define CARD_INIT_TIMEOUT_TIME          (10000/portTICK_RATE_MS)
#define CARD_INIT_TIMEOUT_INTERVAL      (1000/portTICK_RATE_MS) 
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;
static TSReaderInterface mReaderInterface = {OctopusReaderInit, OctopusReaderSetPower, OctopusReaderBreakCheckReader, OctopusReaderCheckReader, OctopusReaderProcess, OctopusReaderGetBootedStatus, OctopusReaderSignOnProcess, OctopusReaderSaveFile, OctopusReaderSaveFilePure};
static TickType_t powerUpTick = 0;
static uint16_t currentTargetDeduct; 
static tsreaderDepositResultCallback  ptsreaderDepositResultCallback = NULL;
static BOOL needInitReaderFlag = TRUE;
static BYTE AI[7] = {0x00, 0x00, 0x01, 0x00, 0xA3, 0x00, 0x00}; //<2-bytes receipt number> <2-bytes bay number> <1-byte optional info>
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static BOOL readVN(void)
{  
    INT s32_ret;
    stDevVer DeviceV;
    char timeStr[32];
    char* timeStrStart;
    //time_t u32_time = GetCurrentUTCTime() - GMT_TIME_ZONE_DIFF_SECONDS;
    time_t u32_time = GetCurrentUTCTime() - 0;
    UTCTimeToString(u32_time, timeStr);  //%04d%02d%02d%02d%02d%02d
    sysprintf(" timeStr = [%s]\r\n", timeStr);
    //timeStr = [20191018143648]
    //u32_time = [19/10/18 14:36:48]

    //CMD Header(5) >>>
    //00 00 FF 00 00
    //CMD (15) >>>
    //46 00 08 04 06 03 04 01 08 01 00 01 09 01 00

    UINT    u32_tempstr[30];
    CHAR	tmpstr[3];
    memset(u32_tempstr, 0, sizeof(u32_tempstr));
    
    timeStrStart = timeStr+2;
    //memset(temp, 0, sizeof(temp));
    //fgets(temp, sizeof(temp), stdin);
    if (timeStrStart[0] != '\n')
    {
        for (int i = 0; i < 6; i++)
        {
            strncpy((CHAR *)&tmpstr, (const CHAR *)&timeStrStart[i*2], 2);
            u32_tempstr[5-i] = atoi(tmpstr);
        }
    }  
    //CmdTimeVer(&u32_tempstr[0]);
    /*
    RSP (80) <<<
    47 00 00 5A FF E2 01 02 03 04 00 00 00 04 03 20 00 00 00 00 00 00 00 02 00 00 00 02 00 9D
    00 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 00 00 00 02 00 00 00 FB 00 00 00 02
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
    TimeVer Success
    Device ID  : 5963746
    Oper ID    : 16909060
    Dev Time   : 4
    Comp ID    : 800
    Key Ver    : 0
    EOD Ver    : 2
    BL Ver     : 2
    FIRM Ver   : 10289154
    CCHS Ver   : 0
    Location ID: 0
    IntBL Ver  : 0
    FuncBL Ver : 2
    AlertMsgVer: 2
    RWKey Ver  : 251 (0x00FB)
    OTP Ver    : 2
    Reserved   :
     DBG (20)
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

    */
    s32_ret = TimeVer((BYTE *) &DeviceV, &u32_tempstr[0]);
    if (s32_ret == ERR_NOERR) {
        terninalPrintf("TimeVer Success\n");
        sysprintf("Device ID  : %u\n", DeviceV.DevID);
        sysprintf("Oper ID    : %u\n", DeviceV.OperID);
        sysprintf("Dev Time   : %u\n", DeviceV.DevTime);					//Reader time is base on 1/1/2000
        sysprintf("Comp ID    : %u\n", DeviceV.CompID);
        sysprintf("Key Ver    : %u\n", DeviceV.KeyVer);
        sysprintf("EOD Ver    : %u\n", DeviceV.EODVer);
        sysprintf("BL Ver	   : %u\n", DeviceV.BLVer);
        sysprintf("FIRM Ver   : %u\n", DeviceV.FIRMVer);
        sysprintf("CCHS Ver   : %u\n", DeviceV.CCHSVer);
        sysprintf("Location ID: %u\n", DeviceV.CSSer);
        sysprintf("IntBL Ver  : %u\n", DeviceV.IntBLVer);
        sysprintf("FuncBL Ver : %u\n", DeviceV.FuncBLVer);
        sysprintf("AlertMsgVer: %u\n", DeviceV.AlertMsgVer);
        sysprintf("RWKey Ver  : %u (0x%04X)\n", DeviceV.RWKeyVer, DeviceV.RWKeyVer);
        sysprintf("OTP Ver    : %u\n", DeviceV.OTPVer);
        sysprintf("Reserved   : \n");
        DbgDump((BYTE *)DeviceV.Reserved, sizeof(DeviceV.Reserved));
        return TRUE;
    } 
    else
    {
        terninalPrintf("TimeVer fail!\n");
        return FALSE;
    }

}


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
TSReaderInterface* OctopusReaderGetInterface(void)
{
    return &mReaderInterface;
}
BOOL OctopusReaderSetAIValue(uint8_t AI1, uint8_t AI2)
{   
    AI[0] = AI1;
    AI[1] = AI2;
    terninalPrintf("OctopusReaderSetAIValue OK!! octopusReceiptNumber = [0x%02x, 0x%02x] !!\r\n", AI[0], AI[1]);
    return TRUE;
}
BOOL OctopusReaderInit(void)
{
    sysprintf("OctopusReaderInit!!\n");
   
    pUartInterface = UartGetInterface(OCTOPUS_READER_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("OctopusReaderInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    //pUartInterface->ioctlFunc(UART_IOC_SET_OCTOPUS_MODE, 0, 0);
    //pUartInterface->ioctlFunc(UART_IOC_SET_RS485_MODE, 0, 0);
    
    if(pUartInterface->initFunc(UART_BARDRATE) == FALSE)
    {
        sysprintf("OctopusReaderInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    OctopusReaderSetPower(0, FALSE);
    
    AI[0] = GetMeterStorageData()->octopusReceiptNumber[0];
    AI[1] = GetMeterStorageData()->octopusReceiptNumber[1];
    sysprintf("OctopusReaderInit OK!!!\r\n");
    return TRUE;
}
BOOL OctopusReaderSetPower(uint8_t id, BOOL flag)
{    
    if(flag)
    {
        powerUpTick = xTaskGetTickCount();
    }
    else
    {
        if(needInitReaderFlag == FALSE)
        {
            INT s32_ret = EndSession();
            if (s32_ret == ERR_NOERR)
                sysprintf("End Session success\n");
            else
                sysprintf("End Session fail!\n");
            vTaskDelay(CARD_INIT_TIMEOUT_INTERVAL);
        }
        powerUpTick = 0; 
    }
    pUartInterface->setPowerFunc(flag);     
    pUartInterface->setRS232PowerFunc(flag);
    
    if(flag == FALSE)
    {
        needInitReaderFlag = TRUE;
    }
    return flag;
}

uint8_t OctopusReaderCheckReader(void)
{
    uint8_t reVal = TSREADER_CHECK_READER_OK;
    //LoglibPrintf(LOG_TYPE_INFO, " !!! OctopusReaderCheckReader enter  !!!...\r\n", FALSE);
    LoglibPrintf(LOG_TYPE_INFO, " !!! OctopusReaderCheckReader enter  !!!...\r\n");
    
    while(readVN() != TRUE)
    {
        terninalPrintf("readVN retry(%d:%d)...\r\n", xTaskGetTickCount() - powerUpTick, CARD_INIT_TIMEOUT_TIME); 
        terninalPrintf(":");
        vTaskDelay(CARD_INIT_TIMEOUT_INTERVAL);
        if((powerUpTick != 0) && ((xTaskGetTickCount() - powerUpTick) > CARD_INIT_TIMEOUT_TIME) )
        {
            //sysprintf("readVN break (time:%d)...\r\n", (int)(xTaskGetTickCount() - powerUpTick));
            reVal = TSREADER_CHECK_READER_ERROR;                
            {
                char str[512];
                sprintf(str, "   Card Reader --> readVN break (time:%d)...\r\n", (int)(xTaskGetTickCount() - powerUpTick));
                //LoglibPrintf(LOG_TYPE_ERROR, str, FALSE);
                LoglibPrintf(LOG_TYPE_ERROR, str);
            }
            return reVal;  
        }            
    }    
    if(needInitReaderFlag)
    {
        INT s32_ret = Authenticate();
        if	(s32_ret == ERR_NOERR)
        {
            terninalPrintf("Authenticate success\n");
            needInitReaderFlag = FALSE;
        }
        else
        {
            terninalPrintf("Authenticate fail!\n"); 
            reVal = TSREADER_CHECK_READER_ERROR; 
        }
    }                        
    return reVal;
}

BOOL OctopusReaderBreakCheckReader(void)
{
    BOOL reVal = TRUE;
    //checkReaderFlag = FALSE;
    return reVal;
}

#define MONEY_BASE_VALUE_IN_CENTS   10
BOOL OctopusReaderProcess(uint16_t targetDeduct, tsreaderDepositResultCallback callback)
{   
    terninalPrintf("---- OctopusReaderProcess (targetDeduct:%d cents, total time:%d, AI:[0x%02x, 0x%02x])...\r\n", 
                        targetDeduct, (xTaskGetTickCount() - powerUpTick)/portTICK_RATE_MS,  AI[0],  AI[1]);   
    BOOL retval = FALSE;  
    INT s32_ret;    
      
    TickType_t tickLocalStart;  
    ptsreaderDepositResultCallback =  callback;  
    tickLocalStart = xTaskGetTickCount();
    currentTargetDeduct = targetDeduct;     
    
    UINT 	in1, in2, in3, in4;
    CHAR	temp[20];
    CHAR	tmpstr[3];
    Rsp_PollDeductStl pollDeductInfo;
    
    in1 = 30;   //timeout (default = 30 (3000ms)
    in2 = currentTargetDeduct/MONEY_BASE_VALUE_IN_CENTS;    //deduct amount in cents (default = 1 = 10 cents)
    in3 = 0;    //Alert Msg format (default = 0

    //INT PollDeduct (BYTE bTimeout, INT TxnAmt, const BYTE *AddInfo, BYTE bAlertMsgFmt, Rsp_PollDeductStl *cardInfo)
    memset((CHAR*)&pollDeductInfo, 0, sizeof(pollDeductInfo));
    sysprintf("Calling Deduct(%d, 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X, %d)...\n", in1, AI[0], AI[1], AI[2], AI[3], AI[4], in2);
    #if(0)
    /*
    case 0x10:
            EPDDrawContainByID(FALSE, EPD_PICT_CONTAIN_DEPOSIT_FAIL_INDEX);
            EPDDrawContainByIDEx(190, 390, TRUE, EPD_PICT_DEPOSIT_FAILE_RETRY_INDEX);
            break;
        case 0x13:
            reval = TRUE;
            break;
        case 0x15:
            reval = TRUE;
            break;
        case 0x19:
            EPDDrawContainByID(FALSE, EPD_PICT_CONTAIN_DEPOSIT_FAIL_INDEX);
            EPDDrawContainByIDEx(200, 390, TRUE, EPD_PICT_DEPOSIT_FAILE_SAME_CARD_INDEX);
            break;
        case 0x20:
        case 0x38:
            EPDDrawContainByID(FALSE, EPD_PICT_CONTAIN_DEPOSIT_FAIL_INDEX);
            EPDDrawContainByIDEx(275, 400, TRUE, EPD_PICT_DEPOSIT_FAILE_AGAIN_INDEX);
            break;
        case 0x30:
            reval = TRUE;
            break;
        default:
            reval = TRUE;
            break;
    */
    s32_ret = ERR_ERRRESP;
    pollDeductInfo.f_err = 0x10;
    //pollDeductInfo.f_err = 0x13;
    //pollDeductInfo.f_err = 0x15;
    //pollDeductInfo.f_err = 0x19;
    //pollDeductInfo.f_err = 0x20;//ignore
    //pollDeductInfo.f_err = 0x38;
    //pollDeductInfo.f_err = 0x30;
    //pollDeductInfo.f_err = 0xdc;  

    //pollDeductInfo.pollStat = 1;
    pollDeductInfo.pollStat = 0;
    #else
    s32_ret = PollDeduct(in1, in2, AI, in3, &pollDeductInfo);
    terninalPrintf(" -- PollDeduct return  [0x%02x] --  \n", s32_ret);  
    #endif
    if (s32_ret == ERR_NOERR) 
    {
        terninalPrintf("Deduction success \n");        
        terninalPrintf("Poll Status = %d\n",pollDeductInfo.pollStat);
        strncpy(temp, (CHAR *)pollDeductInfo.CardNo, sizeof(pollDeductInfo.CardNo));
        //by sam
        //temp[sizeof(temp)]='\0';
        temp[sizeof(pollDeductInfo.CardNo)]='\0';
            
        terninalPrintf("Card ID = %s\n",temp);
        sysprintf("Language = %d\n", pollDeductInfo.language);
        terninalPrintf("Remaining Value before = %d (%d.%d)\n", End4(pollDeductInfo.beforeRV), End4(pollDeductInfo.beforeRV)*MONEY_BASE_VALUE_IN_CENTS/100, End4(pollDeductInfo.beforeRV)*MONEY_BASE_VALUE_IN_CENTS%100);
        terninalPrintf("Remaining Value after = %d (%d.%d)\n", End4(pollDeductInfo.afterRV), End4(pollDeductInfo.afterRV)*MONEY_BASE_VALUE_IN_CENTS/100, End4(pollDeductInfo.afterRV)*MONEY_BASE_VALUE_IN_CENTS%100);
        sysprintf("Alert Tone = %d\n", pollDeductInfo.alertTone);
        sysprintf("Alert Msg = %d\n", pollDeductInfo.alertMsg);
        sysprintf("Eng Alert Msg =\n");
        DbgDump((BYTE *)pollDeductInfo.engAlertMsg, sizeof(pollDeductInfo.engAlertMsg));
        sysprintf("Chi Alert Msg =\n");
        DbgDump((BYTE *)pollDeductInfo.chiAlertMsg, sizeof(pollDeductInfo.chiAlertMsg));
        terninalPrintf("Octopus Type = %d\n", pollDeductInfo.octopusType);
        sysprintf("Reserved = \n");
        DbgDump((BYTE *)pollDeductInfo.Reserved, sizeof(pollDeductInfo.Reserved));
        
        if(ptsreaderDepositResultCallback != NULL)
        {
            ptsreaderDepositResultCallback(TRUE, pollDeductInfo.octopusType, End4(pollDeductInfo.afterRV)*MONEY_BASE_VALUE_IN_CENTS);
        }
        AI[1]++;		//increment receipt no.
    
        if(AI[1] == 0)
            AI[0]++;
        
        GetMeterStorageData()->octopusReceiptNumber[0] = AI[0]; 
        GetMeterStorageData()->octopusReceiptNumber[1] = AI[1];     
    } 
    else //if (s32_ret == ERR_NOERR) 
    {    
        #if(0)
        if(pollDeductInfo.f_err == 0x20)
        {
            //pollDeductInfo.f_err = 0x10; //- retry
            //pollDeductInfo.f_err = 0x13; //fail
            //pollDeductInfo.f_err = 0x15; //fail
            //pollDeductInfo.f_err = 0x19; //- retry
            //pollDeductInfo.f_err = 0x20;//ignore
            //pollDeductInfo.f_err = 0x38; //- retry
            //pollDeductInfo.f_err = 0x30; //fail
            pollDeductInfo.f_err = 0xdc; //fail 
        }
        #endif
        //if ((pollDeductInfo.pollStat) || (pollDeductInfo.f_err == 0x20) || (pollDeductInfo.f_err == 0x03))        
        if ((pollDeductInfo.f_err == 0x20) || (pollDeductInfo.f_err == 0x03)) //0x03 : Invalid parameters
        {
            terninalPrintf("Deduction fail (pollStat = %d, f_err - 0x%02x)!\n", pollDeductInfo.pollStat, pollDeductInfo.f_err);
            //if(pollDeductInfo.f_err == 0x20) 
            if( (pollDeductInfo.f_err == 0x20) || (pollDeductInfo.f_err == 0x00) )
            {
            }
            else
            {
                if(ptsreaderDepositResultCallback != NULL)
                {
                    ptsreaderDepositResultCallback(FALSE, 0, pollDeductInfo.f_err);
                }
            }
        }
        else 
        {
            if (pollDeductInfo.pollStat) 
            {
                terninalPrintf("Poll success but deduct fail (pollStat = %d, f_err - 0x%02x)!\n", pollDeductInfo.pollStat, pollDeductInfo.f_err);
                strncpy(temp, (CHAR *)pollDeductInfo.CardNo, sizeof(pollDeductInfo.CardNo));
                //by sam
                //temp[sizeof(temp)]='\0';
                temp[sizeof(pollDeductInfo.CardNo)]='\0';
                    
                terninalPrintf("Card ID = %s\n",temp);
                sysprintf("Language = %d\n", pollDeductInfo.language);
                terninalPrintf("Remaining Value before = %d\n",End4(pollDeductInfo.beforeRV));
                if(ptsreaderDepositResultCallback != NULL)
                {
                    ptsreaderDepositResultCallback(FALSE, End4(pollDeductInfo.beforeRV), pollDeductInfo.f_err);
                }
            }
            else
            {
                terninalPrintf("Deduction fail 2 (pollStat = %d, f_err - 0x%02x)!\n", pollDeductInfo.pollStat, pollDeductInfo.f_err);
                if(ptsreaderDepositResultCallback != NULL)
                {
                    ptsreaderDepositResultCallback(FALSE, 0, pollDeductInfo.f_err);
                }
            }
            
            
        }       
    } //if (s32_ret == ERR_NOERR) 
    return retval;
}
BOOL OctopusReaderSignOnProcess(void)
{   
    return FALSE;
}

BOOL OctopusReaderGetBootedStatus(void)
{
    return FALSE;
}

void OctopusReaderSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue)
{      
	terninalPrintf("OctopusReaderSaveFile octopusReceiptNumber = [0x%02x, 0x%02x]\r\n",GetMeterStorageData()->octopusReceiptNumber[0], GetMeterStorageData()->octopusReceiptNumber[1]);
    //MeterStorageFlush(FALSE);
    MeterStorageFlush();
}

void OctopusReaderSaveFilePure(void)
{       
	
}

//============
INT32 OctopusReaderWrite(PUINT8 pucBuf, UINT32 uLen)
{
    if(pUartInterface != NULL)
    {
        return  pUartInterface->writeFunc(pucBuf, uLen);
    }
    return 0;
}
INT32 OctopusReaderRead(PUINT8 pucBuf, UINT32 uLen)
{
    if(pUartInterface != NULL)
    {
        return  pUartInterface->readFunc(pucBuf, uLen);
    }
    return 0;
}

void OctopusReaderFlushBuffer(void)
{
    //sysprintf(" --> OctopusReaderFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
BOOL OctopusExportXFile(void)
{
    terninalPrintf("\r\n**XFile** (init receive + content receive)\n");                  
    UINT in1 = MAX_SEGMENT_LEN;
    CHAR XFileName[128];
    INT s32_ret;
    s32_ret = XFileInitRecv(XFileName, in1);

    if (s32_ret != ERR_NOERR) 
    {
        terninalPrintf("XFile Init Fail\n");
        return FALSE;
    }
    else
    {
        terninalPrintf("XFile Init success\n");
    }
    vTaskDelay(2000/portTICK_RATE_MS);
    s32_ret = XFileContRecv(XFileName, in1);

    if (s32_ret == ERR_NOERR) 
    {
        terninalPrintf("XFile Receive success\n");
        terninalPrintf("\n*****************************************************************\n");
        terninalPrintf("Exchange File Name = %s\n",XFileName);
        terninalPrintf("*****************************************************************\n\n");
        return TRUE;
    } 
    else
    {
        terninalPrintf("XFile Receive fail!\n");
        return FALSE;
    }
}



/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


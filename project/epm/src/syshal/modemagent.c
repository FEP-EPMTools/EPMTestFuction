/**************************************************************************//**
* @file     modemagent.c
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
#include "rtc.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "powerdrv.h"
#include "fepconfig.h"
#include "timelib.h"
#include "atcmdparser.h"
#include "quentelmodemlib.h"
#include "modemagent.h"
#include "ff.h"
#include "fatfslib.h"
#include "jsoncmdlib.h"    
#include "pct08cmdlib.h"
#include "pct08drv.h"
#include "meterdata.h"
#include "dataprocesslib.h"
#include "loglib.h"
#include "photoagent.h"
#include "batterydrv.h"
#include "hwtester.h"
#include "osmisc.h"
#include "gpio.h"
#if (ENABLE_BURNIN_TESTER)
#include "burnintester.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define RE_EXECUTE_TIMES  3
//#define BASE_DATA_DIR  "1:"
#define MODEM_CHECK_INTERVAL    (10*1000)
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static SemaphoreHandle_t xModemAgentSemaphore;
static TickType_t threadModemAgentWaitTime        = portMAX_DELAY;

static SemaphoreHandle_t xModemRoutineSemaphore;

static int errortimes = 0;

static BOOL modemAgentPowerStatus = TRUE;
static BOOL modemAgentPowerStatusFlag = FALSE;

static BOOL ModemAgentCheckStatus(int flag);
static BOOL ModemAgentPreOffCallback(int flag);
static BOOL ModemAgentOffCallback(int flag);
static BOOL ModemAgentOnCallback(int flag);
static powerCallbackFunc modemAgentPowerCallabck = {" [ModemAgent] ", ModemAgentPreOffCallback, ModemAgentOffCallback, ModemAgentOnCallback, ModemAgentCheckStatus};

#if (ENABLE_BURNIN_TESTER)
static uint32_t modemATBurninCounter = 0;
static uint32_t modemATBurninErrorCounter = 0;
static uint32_t modemFTPBurninCounter = 0;
static uint32_t modemFTPBurninErrorCounter_FTP = 0;
static uint32_t modemFTPBurninErrorCounter_Dialup = 0;
static uint32_t modemFTPBurninErrorCounter_GetFile = 0;
static uint32_t sdCardBurninCounter = 0;
static uint32_t sdCardBurninErrorCounter = 0;
static BOOL modemGoingFTP = FALSE;
#endif

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static BOOL ModemAgentPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    //sysprintf("### ModemAgent OFF Callback [%s] ###\r\n", modemAgentPowerCallabck.drvName);   
    modemAgentPowerStatusFlag = TRUE;
    return reVal;    
}
static BOOL ModemAgentOffCallback(int flag)
{
    if(flag)
    {
    }
    else
    {
        int timers = 2000/10;
        while(!modemAgentPowerStatus)
        {
            sysprintf("[mo]");
            if(timers-- == 0)
            {
                sysprintf("\r\n ####  [ModemAgentOffCallback FALSE]  ####  \r\n");
                return FALSE;
            }
            vTaskDelay(10/portTICK_RATE_MS); 
        }
    }
    return TRUE;    
}
static BOOL ModemAgentOnCallback(int flag)
{
    BOOL reVal = TRUE;
    xSemaphoreGive(xModemRoutineSemaphore); 
    modemAgentPowerStatusFlag = FALSE;
    return reVal;    
}
static BOOL ModemAgentCheckStatus(int flag)
{
    //BOOL reVal = TRUE;
    //sysprintf("### modemAgent STATUS Callback [%s] ###\r\n", modemAgentPowerCallabck.drvName); 
    if(flag)
    {        
        return TRUE;
    }
    else
    {
        return modemAgentPowerStatus;  
    }        
}

static void vModemAgentTask( void *pvParameters )
{
    RTC_TIME_DATA_T pt;
    uint32_t currentTime;
    BOOL retryFlag = FALSE;
    int reRetryTimes = RE_EXECUTE_TIMES;
    sysprintf("vModemAgentTask Going...\r\n");      
    //threadModemAgentWaitTime = 0;  
    vTaskDelay(2000/portTICK_RATE_MS);
    for(;;)
    {        
        //sysprintf("\r\n**! INFORMATION !** vModemAgentTask Waiting ...\r\n");
        modemAgentPowerStatus = TRUE;
        BaseType_t reval = xSemaphoreTake(xModemAgentSemaphore, threadModemAgentWaitTime/*portMAX_DELAY*/);
        
        if(modemAgentPowerStatusFlag == FALSE)
        {
            modemAgentPowerStatus = FALSE;
            if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
            {
                DataSendType sendType;
                currentTime = RTC2Time(&pt);
                sendType = DataProcessCheckExecute(currentTime);
                if(sendType != DATA_SEND_TYPE_NONE)
                {
                    #if(ENABLE_LOG_FUNCTION)
                    {
                        char str[256];
                        sprintf(str, "**! INFORMATION !** vModemAgentTask GO  ...\r\n");
                        LoglibPrintf(LOG_TYPE_INFO, str);
                    }
                    #else
                    sysprintf("\r\n**! INFORMATION !** vModemAgentTask GO ...\r\n");
                    #endif
                    
                    reRetryTimes = 0;     
                    retryFlag = FALSE;   
                    MeterUpdateModemAgentLastTime();
                    while((reRetryTimes < RE_EXECUTE_TIMES) && (retryFlag == FALSE)) //­«§@3¦¸
                    {
                        retryFlag = TRUE;
  
                        QModemDialupStart();
                        if(QModemDialupProcess() == TRUE)
                        {
                            #if(ENABLE_LOG_FUNCTION)
                            {
                                char str[256];
                                sprintf(str, " !!!!!! ~~~~~~ Dialup SUCCESS [RSSI:%s] ...\r\n", FtpQueryCsq());
                                LoglibPrintf(LOG_TYPE_INFO, str);
                            }
                            #else
                            sysprintf(" !!!!!! ~~~~~~ Dialup SUCCESS ~~~~~~ [RSSI:%s] ...\r\n", FtpQueryCsq());
                            #endif
                            if(DataProcessExecute(currentTime, DATA_SEND_TYPE_WEB_POST))
                            {
                                
                            }
                            else
                            {
                                errortimes++;
                                retryFlag = FALSE;
                            }
                            if((sendType & DATA_SEND_TYPE_FTP)||(sendType & DATA_SEND_TYPE_FTP_GET))
                            {
                                //QModemFtpClientStart();
                                //if(QModemFtpClientProcess() == TRUE)
                                {
                                    //#if(ENABLE_LOG_FUNCTION)
                                    //{
                                    //    char str[512];
                                    //    sprintf(str, " !!!!!! ~ FTP_CONNECT SUCCESS ~!!!!!!\r\n");
                                    //    LoglibPrintf(LOG_TYPE_INFO, str);
                                    //}
                                    //#endif
                                    if(DataProcessExecute(currentTime, DATA_SEND_TYPE_FTP_GET))
                                    {

                                    }
                                    else
                                    {
                                        errortimes++;
                                        retryFlag = FALSE;
                                    }
                                    if(DataProcessExecute(currentTime, DATA_SEND_TYPE_FTP))
                                    {

                                    }
                                    else
                                    {
                                        errortimes++;
                                        retryFlag = FALSE;
                                    }
                                    #if(0)
                                    if(FtpClientClose())
                                    {
                                        #if(ENABLE_LOG_FUNCTION)
                                        {
                                            char str[512];
                                            sprintf(str, " !!!!!! ~ FTP_CLOSE SUCCESS ~!!!!!!\r\n");
                                            LoglibPrintf(LOG_TYPE_INFO, str);
                                        }
                                        #endif
                                    }
                                    else
                                    {
                                        #if(ENABLE_LOG_FUNCTION)
                                        {
                                            char str[512];
                                            sprintf(str, " !!!!!! ~ FTP_CLOSE ERROR ~!!!!!!\r\n");
                                            LoglibPrintf(LOG_TYPE_INFO, str);
                                        }
                                        #endif
                                    }
                                    #endif
                                }
                                //else
                                //{
                                //    errortimes++;
                                //    retryFlag = FALSE;

                                //}
                            }                        
                        }
                        else
                        {
                            #if(ENABLE_LOG_FUNCTION)
                            {
                                char str[256];
                                sprintf(str, " !!!!!! ~~~~~~ Dialup ERROR ~~~~~~ !!!!!!\r\n");
                                LoglibPrintf(LOG_TYPE_ERROR, str);
                            }
                            #else
                            sysprintf(" !!!!!! ~~~~~~ Dialup ERROR ~~~~~~ !!!!!!\r\n");
                            #endif
                            retryFlag = FALSE;
                        }
                        QModemTotalStop(); 
                        #if(ENABLE_LOG_FUNCTION)                        
                        if(retryFlag)
                        {
                            char str[256];
                            sprintf(str, "*** vModemAgentTask [%d times] Success...\r\n", reRetryTimes+1);
                            LoglibPrintf(LOG_TYPE_INFO, str);
                        }
                        else
                        {
                            char str[256];
                            sprintf(str, "*** vModemAgentTask [%d times] Error...\r\n", reRetryTimes+1);
                            LoglibPrintf(LOG_TYPE_ERROR, str);
                        }
                        #else
                        if(retryFlag)
                        {
                            sysprintf("*** vModemAgentTask [%d times] Success...\r\n", reRetryTimes+1);
                        }
                        else
                        {
                            sysprintf("*** vModemAgentTask [%d times] Error...\r\n", reRetryTimes+1);
                        }
                        #endif
                        if(!BatteryCheckPowerDownCondition())
                        {
                            FileAgentFatfsListFile("1:", "*.*");
                            FileAgentFatfsListFile("2:", "*.*");
                        }
                        reRetryTimes++;
                    }
                        
                }
                else
                {
                    //sysprintf("\r\n**! INFORMATION !** vModemAgentTask Ignore Send...\r\n");
                    sysprintf("&");
                }
               
            }
        }
        
       
    }
}


#if (ENABLE_BURNIN_TESTER)
#define FTP_BURNIN_TEST_LOG_PATH    "test/burntestlog/"  //"test2/burntestlog/"
static void vModemTestTask(void *pvParameters)
{
    time_t lastATTime = GetCurrentUTCTime();
    time_t lastFTPTime = lastATTime;
    time_t currentTime;
    BOOL testLoop = FALSE;
    BOOL ftpLoop = FALSE;
    BOOL status;
    BOOL lasterrorflag = FALSE;
    BOOL FTPdeleteflag = FALSE;
    //BOOL needFree = FALSE;
    int reRetryTimes;
    //char *filename;
    char filenameBuffer[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char logpathBuffer[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    
    char lastfilenameBuffer[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char lastlogpathBuffer[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    
    char paraFilePathTmp[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char targetFilePathTmp[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char ipAddress[20];
    //FileAgentReturn reVal;
    //uint8_t* fileDataBuffer;
    char *reportBuffer;
    //size_t fileDataLength;
    RTC_TIME_DATA_T pt;
    char errorMsgBuffer[256];
    int FTPErrorCode1 = 0; 
    int FTPErrorCode2 = 0; 
    int FTPretryTimes = 3;
    
    //size_t readSDtempSize;
    //BOOL needFreeFlag;
    char ErrorFlagStr[5];
    char FTPOccupiedFlagStr[5];
    char RecentFilenameStr[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char RecentLogpathStr[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char LastFilenameStr[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    char LastLogpathStr[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
    
    char ErrorFlagPath[BURNIN_LOG_FILENAME_BUFFER_LENGTH*2];
    char FTPOccupiedPath[BURNIN_LOG_FILENAME_BUFFER_LENGTH*2];
    char RecentFilenamePath[BURNIN_LOG_FILENAME_BUFFER_LENGTH*2];
    char RecentLogPath[BURNIN_LOG_FILENAME_BUFFER_LENGTH*2];
    char LastFilenamePath[BURNIN_LOG_FILENAME_BUFFER_LENGTH*2];
    char LastLogPath[BURNIN_LOG_FILENAME_BUFFER_LENGTH*2];
    FIL file;
    UINT br;
    /*
    FTPErrorCode1
    1. FtpClientSendFile return false len = 0
    2. AT+QFTPCWD return MODEM_RETURN_BREAK
    3. AT+QFTPMKDIR return MODEM_RETURN_ERROR
    4. AT+QFTPMKDIR return MODEM_RETURN_BREAK
    5. AT+QFTPMKDIR again return MODEM_RETURN_ERROR
    6. AT+QFTPMKDIR again return MODEM_RETURN_BREAK
    7. AT+QFTPCWD return MODEM_RETURN_ERROR
    8. AT+QFTPCWD return MODEM_RETURN_BREAK
    9. AT+QFTPPUT return MODEM_RETURN_ERROR
    10.AT+QFTPPUT return MODEM_RETURN_BREAK
    11.AT+QFTPPUT return cmdActionBreakFlag
    12.AT+QFTPPUT return Ftp Client write error
    13.AT+QFTPPUT return cmdActionBreakFlag2
    14.AT+QFTPPUT return Ftp Client write error2
    15.NULL return MODEM_RETURN_ERROR
    16.NULL return MODEM_RETURN_BREAK

    FTPErrorCode2
    1. FTP feedback datasize ERROR
    2. FTP feedback cmd ERROR
    */

    terninalPrintf("vModemTestTask Going...\r\n");
    //QueryNTPfun();
    //ResetRuntimefun();
    /*
    setPrintfFlag(FALSE);
    __SHOW_FREE_HEAP_SIZE__
    setPrintfFlag(TRUE);
    */
    while (TRUE)
    {
        if (GetDeviceID() != 0)
        {
            terninalPrintf("Already GetDeviceID.\r\n");
            sprintf(targetFilePathTmp, "0:\\%08d\\TestReport", GetDeviceID());
            sprintf(paraFilePathTmp, "0:\\%08d\\Parameter", GetDeviceID());
            break;
        }
        vTaskDelay(200 / portTICK_RATE_MS);
    }
    
    while (TRUE)
    {
        /*
        if(FTPretryTimes <= 0)
        {
            //Reset pin GPG8
            outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xFu<<0)) | (0x0u<<0));
            GPIO_OpenBit(GPIOG, BIT8, DIR_OUTPUT, NO_PULL_UP);
            GPIO_ClrBit(GPIOG, BIT8);
            vTaskDelay(1000/portTICK_RATE_MS);
            GPIO_SetBit(GPIOG, BIT8);
        }
        */
        //if( (FTPretryTimes < 3) && (FTPretryTimes > 0) )
        
        if(((FTPretryTimes < 3) && (FTPretryTimes > 0)) || FTPdeleteflag )
        {
            
            /*
            QModemTotalStop();
            vTaskDelay(1000/portTICK_RATE_MS);
            QModemLibInit(921600);
            */
            ftpLoop = TRUE;
        }
        else
        {
            currentTime = GetCurrentUTCTime();
            if ((currentTime - lastFTPTime) > BURNIN_MODEM_FTP_INTERVAL)
            {
                //terninalPrintf("vModemTestTaskFTP %d.\r\n",currentTime);
                lastFTPTime = currentTime;
                ftpLoop = TRUE;     //Enabled Dialup & FTP Testing
                //ftpLoop = FALSE;    //Disabled Dialup & FTP Testing
                
                FTPretryTimes = 3;
            }
            else if ((currentTime - lastATTime) > BURNIN_MODEM_AT_INTERVAL)
            {
                //terninalPrintf("vModemTestTaskAT %d.\r\n",currentTime);
                lastATTime = currentTime;
                testLoop = TRUE;    //Enabled AT Command Testing
                //testLoop = FALSE;   //Disabled AT Command Testing
            }
        }
        
        if (ftpLoop)
        {
            ftpLoop = FALSE;
            
            //if( (FTPretryTimes < 3) && (FTPretryTimes > 0) )
            if(((FTPretryTimes < 3) && (FTPretryTimes > 0)) || FTPdeleteflag )
            {}
            else
            {
            
                //Write Test Report to File (SD Card)
                RTC_Read(RTC_CURRENT_TIME, &pt);
                sprintf(filenameBuffer, "Burnin_%08d_%04d%02d%02d%02d%02d%02d.log", GetDeviceID(), pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond);
                reportBuffer = BuildBurninTestReport(&pt);
                if (FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, targetFilePathTmp, filenameBuffer, (uint8_t *)reportBuffer, strlen(reportBuffer), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE) == FILE_AGENT_RETURN_ERROR)
                {
                    sdCardBurninErrorCounter++;
                }
                sdCardBurninCounter++;
            
            }
            
            if (GetPrepareStopBurninFlag())
            {
                NoticeSDReportDone();
            }
            
            modemFTPBurninCounter++;
            //Get Filename of Last Test Report
            //filename = GetLastBurninLogFilename();
            //if (filename == NULL)
            //{
            //    modemFTPBurninErrorCounter_GetFile++;
            //    lastFTPTime = GetCurrentUTCTime();
            //    continue;
            //}
            //memcpy(filenameBuffer, filename, BURNIN_LOG_FILENAME_BUFFER_LENGTH);
            
            //Read File of Last Test Report
            //reVal = FileAgentGetData(FILE_AGENT_STORAGE_TYPE_FATFS, "0:", filenameBuffer, &fileDataBuffer, &fileDataLength, &needFree, FALSE);
            //if ((reVal == FILE_AGENT_RETURN_ERROR) || (fileDataLength == 0))
            //{
            //    modemFTPBurninErrorCounter_GetFile++;
            //    continue;
            //}
            //terninalPrintf("QModemFTPCmdTest ==> FileAgentGetData OK [%s]!!!\r\n", filenameBuffer);
            
            //Modem Dialup Process
            modemGoingFTP = TRUE;
            reRetryTimes = 1;//RE_EXECUTE_TIMES;
            while (reRetryTimes > 0)
            {
                QModemDialupStart();
                if (QModemDialupProcess() == TRUE)
                {
                    memset(ipAddress, 0x00, sizeof(ipAddress));
                    QModemQueryIPAddress(ipAddress);
                    //terninalPrintf("QModemFTPCmdTest ==> IP Address [%s]!!!\r\n", ipAddress);
                    break;
                }
                vTaskDelay(500 / portTICK_RATE_MS);
                reRetryTimes--;
            }
            if (reRetryTimes <= 0)
            {
                modemFTPBurninErrorCounter_Dialup++;
                sprintf(errorMsgBuffer, "QModemFTPCmdTest ==> Dialup Error !!\r\n");
                AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                terninalPrintf(errorMsgBuffer);
                
                FTPretryTimes = 0;
            }
            //Modem FTP Send File Process
            else
            {
                reRetryTimes = 1;
                while (reRetryTimes > 0)
                {
                    QModemFtpClientStart();
                    status = QModemFtpClientProcess();
                    //terninalPrintf("QModemFTPCmdTest ==> Connect Status=%d\r\n", status);
                    if (status)
                    {
                        FTPErrorCode1 = 0; 
                        FTPErrorCode2 = 0; 
                        
                        
                        memset(ErrorFlagStr,0x00,sizeof(ErrorFlagStr));
                        sprintf(ErrorFlagPath, "%s\\ErrorFlag.txt", paraFilePathTmp);
                        f_open(&file, ErrorFlagPath, FA_OPEN_EXISTING |FA_READ);
                        f_read(&file, ErrorFlagStr, 1, &br);
                        //terninalPrintf("ErrorFlagStr = %s\r\n", ErrorFlagStr);
                        //terninalPrintf("atoi(ErrorFlagStr) = %d\r\n", atoi(ErrorFlagStr));
                        //if(lasterrorflag)
                        if(atoi(ErrorFlagStr) == 1)
                        {
                            //lasterrorflag = FALSE;

                            
                            memset(LastFilenameStr,0x00,sizeof(LastFilenameStr));
                            sprintf(LastFilenamePath, "%s\\LastErrFileName.txt", paraFilePathTmp);
                            f_open(&file, LastFilenamePath, FA_OPEN_EXISTING |FA_READ);
                            f_read(&file, LastFilenameStr, sizeof(LastFilenameStr), &br);
                            //terninalPrintf("LastFilenameStr = %s\r\n", LastFilenameStr);
                            
                            memset(LastLogpathStr,0x00,sizeof(LastLogpathStr));
                            sprintf(LastLogPath, "%s\\LastErrLogpathBuffer.txt", paraFilePathTmp);
                            f_open(&file, LastLogPath, FA_OPEN_EXISTING |FA_READ);
                            f_read(&file, LastLogpathStr, sizeof(LastLogpathStr), &br);
                            //terninalPrintf("LastLogpathStr = %s\r\n", LastLogpathStr);
                            
                            //FtpClientDeleteFile(TRUE, FTP_PRE_PATH, lastlogpathBuffer, lastfilenameBuffer);
                            
                            FtpClientDeleteFile(TRUE, FTP_PRE_PATH, LastLogpathStr, LastFilenameStr);
                            
                            FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "ErrorFlag.txt", (uint8_t *)"0", strlen("0"), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                            
                            
                            FTPdeleteflag = FALSE;
                            
                            FtpClientClose();
                            
                            break;
                        }
                        
                        memset(FTPOccupiedFlagStr,0x00,sizeof(FTPOccupiedFlagStr));
                        sprintf(FTPOccupiedPath, "%s\\FTPOccupiedFlag.txt", paraFilePathTmp);
                        f_open(&file, FTPOccupiedPath, FA_OPEN_EXISTING |FA_READ);
                        f_read(&file, FTPOccupiedFlagStr, 1, &br);
                        //terninalPrintf("FTPOccupiedFlagStr = %s\r\n", FTPOccupiedFlagStr);
                        //terninalPrintf("atoi(FTPOccupiedFlagStr) = %d\r\n", atoi(FTPOccupiedFlagStr));
                        
                        
                        if(atoi(FTPOccupiedFlagStr) == 1)
                        {
                            memset(RecentFilenameStr,0x00,sizeof(RecentFilenameStr));
                            sprintf(RecentFilenamePath, "%s\\RecentFileName.txt", paraFilePathTmp);
                            f_open(&file, RecentFilenamePath, FA_OPEN_EXISTING |FA_READ);
                            f_read(&file, RecentFilenameStr, sizeof(RecentFilenameStr), &br);
                            //terninalPrintf("RecentFilenameStr = %s\r\n", RecentFilenameStr);
                            
                            memset(RecentLogpathStr,0x00,sizeof(RecentLogpathStr));
                            sprintf(RecentLogPath, "%s\\RecentLogpathBuffer.txt", paraFilePathTmp);
                            f_open(&file, RecentLogPath, FA_OPEN_EXISTING |FA_READ);
                            f_read(&file, RecentLogpathStr, sizeof(RecentLogpathStr), &br);
                            //terninalPrintf("RecentLogpathStr = %s\r\n", RecentLogpathStr);
                            
                            //FtpClientDeleteFile(TRUE, FTP_PRE_PATH, lastlogpathBuffer, lastfilenameBuffer);
                            
                            FtpClientDeleteFile(TRUE, FTP_PRE_PATH, RecentLogpathStr, RecentFilenameStr);
                            
                            FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "FTPOccupiedFlag.txt", (uint8_t *)"0", strlen("0"), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                            
                            FTPdeleteflag = FALSE;
                            
                            FtpClientClose();
                            
                            break;
                            
                            
                        }
                        
                        
                        //status = FtpClientSendFile(TRUE, FTP_PRE_PATH, FTP_BURNIN_TEST_LOG_PATH, filenameBuffer, (uint8_t *)reportBuffer, strlen(reportBuffer));
                        //if(reRetryTimes == 3)
                        if(FTPretryTimes == 3)
                        {
                            sprintf(logpathBuffer,FTP_BURNIN_TEST_LOG_PATH);
                            //status = FtpClientSendFileEx(TRUE, FTP_PRE_PATH, FTP_BURNIN_TEST_LOG_PATH, filenameBuffer, (uint8_t *)reportBuffer, strlen(reportBuffer),&FTPErrorCode1,&FTPErrorCode2);
                        }
                        else
                        {
                            sprintf(logpathBuffer,"test/errorlog/");
                            //status = FtpClientSendFileEx(TRUE, FTP_PRE_PATH, "test/errorlog/", filenameBuffer, (uint8_t *)reportBuffer, strlen(reportBuffer),&FTPErrorCode1,&FTPErrorCode2);
                        }
                        
                        FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "FTPOccupiedFlag.txt", (uint8_t *)"1", strlen("1"), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                        FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "RecentFileName.txt", (uint8_t *)filenameBuffer, strlen(filenameBuffer), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                        FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "RecentLogpathBuffer.txt", (uint8_t *)logpathBuffer, strlen(logpathBuffer), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                        
                        
                        status = FtpClientSendFileEx(TRUE, FTP_PRE_PATH, logpathBuffer, filenameBuffer, (uint8_t *)reportBuffer, strlen(reportBuffer),&FTPErrorCode1,&FTPErrorCode2);
                        //status = FtpClientSendFile(TRUE, FTP_PRE_PATH, FTP_BURNIN_TEST_LOG_PATH, filenameBuffer, fileDataBuffer, fileDataLength);
                        //terninalPrintf("QModemFTPCmdTest ==> Send File Status=%d\r\n", status);

                        FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "FTPOccupiedFlag.txt", (uint8_t *)"0", strlen("0"), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                        
                        if (status)
                        {
                            FTPretryTimes = 3;
                            if (FtpClientClose() == FALSE)
                            {
                                //FTP Close Failed!!
                            }
                            break;
                        }
                        else
                        {
                            
                            FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "ErrorFlag.txt", (uint8_t *)"1", strlen("1"), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                            FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "LastErrFileName.txt", (uint8_t *)filenameBuffer, strlen(filenameBuffer), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                            FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, paraFilePathTmp, "LastErrLogpathBuffer.txt", (uint8_t *)logpathBuffer, strlen(logpathBuffer), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
                            FTPdeleteflag = TRUE;
                            //lasterrorflag = TRUE;
                            memcpy(lastfilenameBuffer,filenameBuffer,sizeof(lastfilenameBuffer));
                            memcpy(lastlogpathBuffer,logpathBuffer,sizeof(lastlogpathBuffer));
                            
                            //sprintf(filenameBuffer, "Burnin_%08d_%04d%02d%02d%02d%02d%02d_Resend.log", GetDeviceID(), pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond);
                            sprintf(filenameBuffer, "Burnin_%08d_%04d%02d%02d%02d%02d%02d_Resend%02d_%02d_%02d.log", GetDeviceID(), pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond,4-FTPretryTimes, FTPErrorCode1,FTPErrorCode2);
                            sprintf(errorMsgBuffer, "QModemFTPCmdTest ==> Upload File Error !!\r\n");
                            AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                            terninalPrintf(errorMsgBuffer);
                            
                            FTPretryTimes--;
                            
                            //terninalPrintf("MODEM reset.\r\n");
                            //Reset pin GPG8
                            outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xFu<<0)) | (0x0u<<0));
                            GPIO_OpenBit(GPIOG, BIT8, DIR_OUTPUT, NO_PULL_UP);
                            GPIO_ClrBit(GPIOG, BIT8);
                            vTaskDelay(1000/portTICK_RATE_MS);
                            GPIO_SetBit(GPIOG, BIT8);
                            

                            
                        }
                    }
                    else
                    {
                        sprintf(errorMsgBuffer, "QModemFTPCmdTest ==> Connect Server Error !!\r\n");
                        AppendBurninErrorLog(errorMsgBuffer, strlen(errorMsgBuffer));
                        terninalPrintf(errorMsgBuffer);
                        
                        FTPretryTimes = 0;
                        
                    }
                    vTaskDelay(500 / portTICK_RATE_MS);
                    reRetryTimes--;
                }
                if ((reRetryTimes <= 0) || (status == FALSE))
                {
                    //FTP Connect Fail or Upload File Failed!!
                    modemFTPBurninErrorCounter_FTP++;
                }
            }
            
            //if (needFree)
            //{
            //    vPortFree(fileDataBuffer);
            //}
            //lastFTPTime = GetCurrentUTCTime();
            modemGoingFTP = FALSE;
            if (GetPrepareStopBurninFlag())
            {
                NoticeFTPReportDone();
                terninalPrintf("vModemTestTask Terminated !!\r\n");
                vTaskDelete(NULL);
            }
        }
        else if (testLoop)
        {
            testLoop = FALSE;
            status = QModemATCmdTest();
            modemATBurninCounter++;
            if (!status) {
                modemATBurninErrorCounter++;
            }
            //terninalPrintf("QModemATCmdTest status=%d\r\n", status);
        }
        else
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
    }
}
#endif




static BOOL hwInit(void)
{   
    return TRUE;
}

static BOOL swInit(void)
{   
    if(DataProcessLibInit() == FALSE) 
    {
        return FALSE;
    }
    PowerRegCallback(&modemAgentPowerCallabck);
    xModemAgentSemaphore = xSemaphoreCreateBinary();
    xModemRoutineSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vModemAgentTask, "vModemAgentTask", 1024*20, NULL, MODEM_AGENT_TX_THREAD_PROI, NULL ); 
    //xTaskCreate( vModemAgentRoutineTask, "vModemAgentRoutineTask", 1024 * 10, NULL, MODEM_AGENT_ROUTINE_THREAD_PROI, NULL ); 
    
    return QModemLibInit(921600);
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/

BOOL ModemAgentInit(BOOL testModeFlag)
{
    sysprintf("ModemAgentInit (%s) (%s) \r\n", WEB_POST_ADDRESS, FTP_CONNECTING_CMD(FTP_ADDRESS, FTP_PORT));
    
    if(hwInit() == FALSE)
    {
        sysprintf("ModemAgentInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("ModemAgentInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    
#if (ENABLE_BURNIN_TESTER)
    if (EnabledBurninTestMode())
    {
        xTaskCreate(vModemTestTask, "vModemTestTask", 1024*10, NULL, MODEM_TEST_THREAD_PROI, NULL); 
    }
#endif
    
    sysprintf("ModemAgentInit OK!!\n");
    return TRUE;    
}


BOOL ModemAgentStartSend(DataProcessId id)
{
    if(xModemAgentSemaphore == NULL)
    {
        sysprintf(" -- [Modem Agent] --  ModemAgentStartSend [DataProcessId = %d], IGNORE...\r\n", id);  
        return FALSE;
    }
    sysprintf(" -- [Modem Agent] --  ModemAgentStartSend [DataProcessId = %d]...\r\n", id);  
    if(DataProcessSetExecuteFlag(id))
    {
        xSemaphoreGive(xModemAgentSemaphore); 
    }
    return TRUE;
}
/*
BOOL ModemAgentStopSend(void)
{
    sysprintf(" -- [Modem Agent] --  ModemAgentStopSend...\r\n");  
    threadModemAgentWaitTime = portMAX_DELAY; 
    QModemTotalStop();

    return TRUE;
}
*/

#if (ENABLE_BURNIN_TESTER)
uint32_t GetModemATBurninTestCounter(void)
{
    return modemATBurninCounter;
}

uint32_t GetModemATBurninTestErrorCounter(void)
{
    return modemATBurninErrorCounter;
}

uint32_t GetModemFTPBurninTestCounter(void)
{
    return modemFTPBurninCounter;
}

uint32_t GetModemFTPBurninTestErrorCounter(void)
{
    return modemFTPBurninErrorCounter_FTP;
}

uint32_t GetModemDialupBurninTestErrorCounter(void)
{
    return modemFTPBurninErrorCounter_Dialup;
}

uint32_t GetModemFileBurninTestErrorCounter(void)
{
    return modemFTPBurninErrorCounter_GetFile;
}

BOOL GetModemFTPRunningStatus(void)
{
    return modemGoingFTP;
}

uint32_t GetSDCardBurninTestCounter(void)
{
    return sdCardBurninCounter;
}

uint32_t GetSDCardBurninTestErrorCounter(void)
{
    return sdCardBurninErrorCounter;
}
#endif




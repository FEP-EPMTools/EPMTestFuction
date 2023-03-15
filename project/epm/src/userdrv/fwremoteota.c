
/**************************************************************************//**
* @file     fwremoteota.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2020 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"
#include "gpio.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "timelib.h"
#include "buzzerdrv.h"
#include "leddrv.h"
#include "flashdrvex.h"
#include "epddrv.h"
#include "cardreader.h"
#include "nt066edrv.h"
#include "modemagent.h"
#include "photoagent.h"
#include "powerdrv.h"
#include "batterydrv.h"
#include "spaceexdrv.h"
#include "guimanager.h"
#include "guidrv.h"
#include "smartcarddrv.h"
#include "cardreader.h"
#include "fatfslib.h"
#include "fileagent.h"
#include "quentelmodemlib.h"
#include "hwtester.h"
#include "burnintester.h"
#include "sflashrecord.h"
#include "dipdrv.h"
#include "photoagent.h"
#include "yaffs2drv.h"
#include "fwremoteota.h"

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

typedef BOOL(*initFunction)(BOOL testModeFlag);
typedef struct
{
    char*               drvName;
    initFunction        func;
    BOOL                runTestMode;
} initFunctionList;

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

static char FWname[50];
static uint32_t FWnamesize;

static initFunctionList mInitFunctionList[] =  {{"EpdDrv", EpdDrvInit, FALSE},
                                                {"LEDDrv", LedDrvInit, FALSE},
                                                {"BuzzerDrv",BuzzerDrvInit,FALSE},
                                                {"GUIDrv", GUIDrvInit, FALSE},
                                                {"PowerDrv", PowerDrvInit, FALSE},
                                                {"FlashDrv", FlashDrvExInit, FALSE},
                                                {"ModemAgentDrv", ModemAgentInit, FALSE},
                                                //{"FATFS", FatfsInit, FALSE},
                                                {"", NULL,FALSE}};
                                                
                                                
 
static void hwInit(void)
{
    for (int i = 0 ; ; i++)
    {
        if (mInitFunctionList[i].func == NULL)
        {
            break;
        }
        if (mInitFunctionList[i].func(mInitFunctionList[i].runTestMode))
        {
            //mInitFunctionList[i].result =TRUE;
            terninalPrintf(" * [%02d]: Initial %s OK...    *\r\n", i, mInitFunctionList[i].drvName);
        }
        else
        {
            terninalPrintf(" * [%02d]: Initial %s ERROR... *\r\n", i, mInitFunctionList[i].drvName);
            //if(memcmp("FATFS",mInitFunctionList[i].drvName,sizeof(mInitFunctionList[i].drvName)))
            //    ErrorDiableFlag = TRUE;
        }
    }
    
    if (GuiManagerInit())
    {
        terninalPrintf(" * Initial GuiManager OK...    *\r\n");
    }
    else
    {
        terninalPrintf(" * Initial GuiManager ERROR... *\r\n");
    }
    
}

void callFileContent(char* Str,uint32_t Len)
{
    memcpy(FWname,Str,Len);
    FWnamesize = Len;
}


BOOL FWremoteOTAFunc(void)
{
    int reRetryTimes;
    BOOL status;
    uint8_t* dataBuff;
    uint32_t dataLen;
    

    
    setPrintfFlag(TRUE);
    hwInit();
    FileAgentInit();
    
    EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
    EPDDrawString(TRUE,"FW remote OTA",100,100);
    
    vTaskDelay(100 / portTICK_RATE_MS);
    EPDDrawString(TRUE,"1.Start Modem Dialup",150,200);
    terninalPrintf("Modem Dialup Start");
    reRetryTimes = 1;
    while (reRetryTimes > 0)
    {
        QModemDialupStart();
        if (QModemDialupProcess() == TRUE)

            break;
        vTaskDelay(500 / portTICK_RATE_MS);
        reRetryTimes--;
    }
    terninalPrintf("\r\n");
    if (reRetryTimes <= 0)
    {
        terninalPrintf("QModemFTPCmdTest ==> Dialup Error !!\r\n");
        BuzzerPlay(80, 80, 3, TRUE);
        vTaskDelay(100 / portTICK_RATE_MS);
        EPDDrawString(FALSE,"ERROR",800,200);
        vTaskDelay(15 / portTICK_RATE_MS);
        EPDDrawString(TRUE,"  Please Check Following\n  Parts Setup:\n   a.MODEM\n   b.SIM\n   c.Antenna\n   d.Battery",150,250);
    }
    else
    {
        EPDDrawString(FALSE,"OK",800,200);
        vTaskDelay(15 / portTICK_RATE_MS);
        EPDDrawString(TRUE,"2.Start FTP Process",150,250);
        reRetryTimes = 3;
        while (reRetryTimes > 0)
        {
            QModemFtpClientStart();
            status = QModemFtpClientProcess();
            terninalPrintf("QModemFTPCmdTest ==> Connect Status=%d\r\n", status);
            if (status)
            {
                //status = FtpClientSendFile(TRUE, FTP_PRE_PATH, FTP_BURNIN_TEST_LOG_PATH, filenameBuffer, (uint8_t *)reportBuffer, strlen(reportBuffer));
                //status = FtpClientGetFileEx(FTP_FW_REMOTE_OTA_PATH, (char*)"hello.txt");
                
                //status = FtpClientGetFileOTA(FTP_FW_REMOTE_OTA_PATH, (char*)"epmtest.bin");
                //status = FtpClientGetFile(FTP_FW_REMOTE_OTA_PATH, (char*)"epmtest.bin");
                
                //status = FtpClientGetFileEx(FTP_FW_REMOTE_OTA_PATH, (char*)"epmtest.bin");
                FtpClientGetFileLite(FTP_FW_REMOTE_OTA_PATH, (char*)"version.txt");
                terninalPrintf("Update FWname = %s\r\n", FWname);
                /*
                terninalPrintf("Update FWname = ");
                for(int i=0;i<FWnamesize;i++)
                    terninalPrintf("%c", FWname[i]);
                terninalPrintf("\r\n");
                */
                //if(FtpClientGetFilePure(FTP_FW_REMOTE_OTA_PATH, (char*)"epmtest.bin",  &dataBuff, &dataLen, 30*1000) == TRUE)
                if(FtpClientGetFilePure(FTP_FW_REMOTE_OTA_PATH, (char*)FWname,  &dataBuff, &dataLen, 30*1000) == TRUE)
                {
                    terninalPrintf("QModemFTPCmdTest ==> FtpClientGetFilePure OK (dataLen = %d)\r\n", dataLen);
                    EPDDrawString(FALSE,"OK",800,250);
                    vTaskDelay(15 / portTICK_RATE_MS);
                    EPDDrawString(TRUE,"3.Check Yaffs2 flash",150,300);
                    if(Yaffs2DrvInit())
                    {
                        if(FileAgentAddData(FILE_AGENT_STORAGE_TYPE_YAFFS2, "/", "epmtest.bin", (uint8_t*)dataBuff, dataLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE) !=  FILE_AGENT_RETURN_ERROR )
                        //if(FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, "0:", "epmtest.bin", (uint8_t*)dataBuff, dataLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE) !=  FILE_AGENT_RETURN_ERROR )
                        {
                            BuzzerPlay(300, 0, 1, TRUE);
                            vTaskDelay(100 / portTICK_RATE_MS);
                            EPDDrawString(FALSE,"OK",800,300);
                            vTaskDelay(15 / portTICK_RATE_MS);
                            EPDDrawString(TRUE,"  Update FW Complete\n  Prepare Reset...",150,350);
                            terninalPrintf(" >> CopyFirmwareToYaffs2 FileAgentAddData OK... \n");                        
                        }
                        else
                        {
                            terninalPrintf(" >> CopyFirmwareToYaffs2 FileAgentAddData ERROR... \n");
                            BuzzerPlay(80, 80, 3, TRUE);
                            vTaskDelay(100 / portTICK_RATE_MS);
                            EPDDrawString(TRUE,"ERROR",800,300);
                        }
                    }
                    else
                    {
                        terninalPrintf(" >> CopyFirmwareToYaffs2 Yaffs2DrvInit ERROR... \n");
                        BuzzerPlay(80, 80, 3, TRUE);
                        vTaskDelay(100 / portTICK_RATE_MS);
                        EPDDrawString(TRUE,"ERROR",800,300);
                    }
                }
                else
                {
                    terninalPrintf("QModemFTPCmdTest ==> FtpClientGetFilePure ERROR\r\n");
                    BuzzerPlay(80, 80, 3, TRUE);
                    vTaskDelay(100 / portTICK_RATE_MS);
                    EPDDrawString(TRUE,"ERROR",800,250);
                }
                if (FtpClientClose() == FALSE)
                {
                    //FTP Close Failed!!
                }
                if (status)
                {
                    break;
                }
                else
                {
                    terninalPrintf("QModemFTPCmdTest ==> Download File Error !!\r\n");
                    BuzzerPlay(80, 80, 3, TRUE);
                    vTaskDelay(100 / portTICK_RATE_MS);
                    EPDDrawString(TRUE,"ERROR",800,250);

                }
            }
            else
            {
                terninalPrintf("QModemFTPCmdTest ==> Connect Server Error !!\r\n");
            }
            vTaskDelay(500 / portTICK_RATE_MS);
            reRetryTimes--;
        }
        if ((reRetryTimes <= 0) || (status == FALSE))
        {
            //FTP Connect Fail or Upload File Failed!!
            terninalPrintf("FTP Connect Fail or Upload File Failed !!\r\n");
            BuzzerPlay(80, 80, 3, TRUE);
            vTaskDelay(100 / portTICK_RATE_MS);
            EPDDrawString(TRUE,"ERROR",800,200);
        }
    }
    
    //terninalPrintf("inpw(REG_SYS_RSTSTS) = 0x%08x\r\n",inpw(REG_SYS_RSTSTS));
    //terninalPrintf("inpw(REG_WWDT_STATUS) = 0x%08x\r\n",inpw(REG_WWDT_STATUS));
    //outps(REG_WWDT_STATUS,inpw(REG_WWDT_STATUS));
    PowerDrvResetSystem();
    
    while(1);
    return TRUE;
}
                                                
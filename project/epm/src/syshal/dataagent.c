/**************************************************************************//**
* @file     dataagent.c
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
#include "meterdata.h"
#include "photoagent.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define DATA_AGENT_TX_INTERVAL 3000
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static CommunicationInterface* pCommunicationInterface = NULL; 

static SemaphoreHandle_t xRxTxSemaphore = NULL;
static SemaphoreHandle_t xDataAgentRoutineSemaphore;
static SemaphoreHandle_t xDataAgentFileTransferSemaphore;
static SemaphoreHandle_t xDataAgentTxSemaphore;
static DataAgentTxBuff  dataAgentTxBuff[MAX_AGENT_BUFF_NUMBER];

static TickType_t threadFileTransferWaitTime   = portMAX_DELAY;
static TickType_t threadRoutineWaitTime   = (60*1000/portTICK_RATE_MS);
static TickType_t threadTxWaitTime        = portMAX_DELAY;

static uint8_t RXBuff[1024];

static BOOL dataAgentPowerStatus = TRUE;

static BOOL DataAgentCheckStatus(int flag);
static BOOL DataAgentPreOffCallback(int flag);
static BOOL DataAgentOffCallback(int flag);
static BOOL DataAgentOnCallback(int flag);
static powerCallbackFunc dataAgentPowerCallabck = {" [DataAgent] ", DataAgentPreOffCallback, DataAgentOffCallback, DataAgentOnCallback, DataAgentCheckStatus};

static void vDataAgentFileTransferTask( void *pvParameters );

static BOOL dataAgentDebugEnable = FALSE;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static BOOL DataAgentPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    //sysprintf("### DataAgent OFF Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName);    
    return reVal;    
}
static BOOL DataAgentOffCallback(int flag)
{
     int timers = 2000/10;
    while(!batteryDrvPowerStatus)
    {
        sysprintf("[d]");
        if(timers-- == 0)
        {
            return FALSE;
        }
        vTaskDelay(10/portTICK_RATE_MS); 
    }
    return TRUE;   
}
static BOOL DataAgentOnCallback(int flag)
{
    BOOL reVal = TRUE;
    dataAgentPowerStatus = FALSE;
    xSemaphoreGive(xDataAgentTxSemaphore);
    #warning check this
    xSemaphoreGive(xDataAgentRoutineSemaphore);
    //sysprintf("### DataAgent ON Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName);    
    return reVal;    
}
static BOOL DataAgentCheckStatus(int flag)
{
    //BOOL reVal = TRUE;
    //sysprintf("### DataAgent STATUS Callback [%s] ###\r\n", dataAgentPowerCallabck.drvName); 
    return dataAgentPowerStatus;    
}



static uint8_t getData(uint8_t* pbuff, uint16_t buffLen)
{
    uint8_t reVal = 0;
    int i;
    xSemaphoreTake(xRxTxSemaphore, portMAX_DELAY);
    for(i = 0; i<MAX_AGENT_BUFF_NUMBER; i++)
    {
        if(dataAgentTxBuff[i].useFlag == TRUE)
        {
            CmdHeader* pCmdHeader = (CmdHeader*)dataAgentTxBuff[i].data;
            if((xTaskGetTickCount() - dataAgentTxBuff[i].sendTimeTick) > DATA_AGENT_TX_INTERVAL)
            {                
                uint16_t sendLen = 0;
                dataAgentTxBuff[i].useFlag = TRUE;            
                if(buffLen<dataAgentTxBuff[i].dataLen)
                {
                    memcpy(pbuff, dataAgentTxBuff[i].data, buffLen);
                    sendLen = buffLen;
                }
                else
                {
                    memcpy(pbuff, dataAgentTxBuff[i].data, dataAgentTxBuff[i].dataLen);
                    sendLen = dataAgentTxBuff[i].dataLen;
                }
                
                if(dataAgentTxBuff[i].needAckFlag == FALSE)
                {
                    dataAgentTxBuff[i].useFlag = FALSE;
                    memset(dataAgentTxBuff[i].data, 0x0, MAX_AGENT_DATA_SIZE);
                    //#if(BUILD_DEBUG_VERSION)
                    if(dataAgentDebugEnable)
                        sysprintf(" ==> getData[%d]: OK [cmd id : 0x%02x, cmd index : %d, len = %d ], remove from queue...\r\n", i, pCmdHeader->cmdId, pCmdHeader->cmdIndex, sendLen); 
                    //#endif
                }
                else
                {
                    //#if(BUILD_DEBUG_VERSION)
                    if(dataAgentDebugEnable)
                        sysprintf(" ==> getData[%d]: OK [cmd id : 0x%02x, cmd index : %d, len = %d ], need ACK...\r\n", i, pCmdHeader->cmdId, pCmdHeader->cmdIndex, sendLen); 
                    //#endif
                }
                dataAgentTxBuff[i].sendTimeTick = xTaskGetTickCount();
                pCommunicationInterface->writeFunc(pbuff, sendLen); 
                
                //if(dataAgentDebugEnable)
                //    vTaskDelay(1500/portTICK_RATE_MS);
                //return reVal;
            }
            else
            {
                if(dataAgentDebugEnable)
                    sysprintf(" ==> getData[%d]: WARNING ignore [cmd id : 0x%02x, cmd index : %d]\r\n", i, pCmdHeader->cmdId, pCmdHeader->cmdIndex); 
            }
            reVal++;
        }
    }    
    //sysprintf("getData end: sendLen = %d...\r\n", reVal); 
    xSemaphoreGive(xRxTxSemaphore);    
    return reVal;
}

static void vDataAgentRxTask( void *pvParameters )
{
    vTaskDelay(2000/portTICK_RATE_MS); 
    int totalRead = 0;;
    sysprintf("vDataAgentRxTask Going...\r\n");        
    for(;;)
    {
        BaseType_t reval = pCommunicationInterface->readWaitFunc(portMAX_DELAY);//xSemaphoreTake(xDataAgentRxSemaphore, portMAX_DELAY);
        vTaskDelay(10/portTICK_RATE_MS); 
        //sysprintf("Lora Read Going...\r\n");
        while(1)
        {
            int retval = pCommunicationInterface->readFunc(RXBuff, sizeof(RXBuff));
            if( retval > 0)
            {
                totalRead = totalRead + retval;
                
                //sysprintf("vDataAgentRxTask %d Data(Total:%d) ...\r\n", retval, totalRead);
                #if(0)
                {
                    int i;
                    char cChar;
                    for(i = 0; i< retval; i++)
                    {
                        cChar = RX_Test[i] & 0xFF;
                        //sysprintf("Lora Read [%02d]: 0x%02x (%c)...\r\n", i, cChar, cChar); 
                        sysprintf("[%02d]: 0x%02x (%c)\r\n", i, cChar, cChar);
                    }
                }
                #endif
                CmdProcessReadData(RXBuff, retval);
                
            }
            else
            {
                //sysprintf("!!vDataAgentRxTask Break(Total:%d) ...\r\n", totalRead);
                break;
            }
        }
    }
}
static void vDataAgentTxTask( void *pvParameters )
{
    vTaskDelay(1000/portTICK_RATE_MS); 
    uint8_t buff[MAX_AGENT_DATA_SIZE];
//    int totalRead = 0;;
    sysprintf("vDataAgentTxTask Going...\r\n");       
    threadTxWaitTime = 0; //需要先執行一次     
    for(;;)
    {
        #if(BUILD_DEBUG_VERSION)
        sysprintf("vDataAgentTxTask Waiting (%d)...\r\n", threadTxWaitTime); 
        #endif
        BaseType_t reval = xSemaphoreTake(xDataAgentTxSemaphore, threadTxWaitTime/*portMAX_DELAY*/);
        //sysprintf("vDataAgentTxTask Going...\r\n"); 
        #warning just test
        dataAgentPowerStatus = FALSE;
        
        uint8_t reVal = getData(buff, MAX_AGENT_DATA_SIZE);
        if(reVal == 0)
        {
            //sysprintf("vDataAgentTxTask break...\r\n"); 
            threadTxWaitTime = portMAX_DELAY;
            dataAgentPowerStatus = TRUE;
        }
        else
        {
            //sysprintf("vDataAgentTxTask sending (len = %d)...\r\n", reVal); 
            //if(dataAgentDebugEnable)
            //{
            //    threadTxWaitTime = 5000;
            //}
            //else
            {
                threadTxWaitTime = DATA_AGENT_TX_INTERVAL/portTICK_RATE_MS;
            }
            //#warning just test
            //dataAgentPowerStatus = TRUE;
        }  
        if(reval == pdTRUE)
        {
            
        }
        else
        {//timeout
            
        }        
    }
}

static void vDataAgentRoutineTask( void *pvParameters )
{
    vTaskDelay(3000/portTICK_RATE_MS);     
    sysprintf("vDataAgentRoutineTask Going...\r\n");  

    xSemaphoreGive(xDataAgentRoutineSemaphore);
    for(;;)
    {
        BaseType_t reval = xSemaphoreTake(xDataAgentRoutineSemaphore, threadRoutineWaitTime/*portMAX_DELAY*/);  
        #warning check this
        //CmdSendVersion(PowerGetTotalWakeupTick());  
          
        #warning just test 
#if(0)        
        {
            static int times = 0;
            if(times%5 == 0)
                PhotoAgentStartTakePhoto(0);
            times++;
        }
#endif
    }
}

static BOOL DataAgentRemoveDataByCmdId(uint16_t cmdId)
{
    BOOL reVal = FALSE;
    int i;
    xSemaphoreTake(xRxTxSemaphore, portMAX_DELAY);
    for(i = 0; i<MAX_AGENT_BUFF_NUMBER; i++)
    {
        if((dataAgentTxBuff[i].useFlag == TRUE) && (dataAgentTxBuff[i].needAckFlag == TRUE))
        {
            CmdHeader* pCmdHeader = (CmdHeader*)dataAgentTxBuff[i].data;
            if(pCmdHeader->cmdId == cmdId)
            {
                dataAgentTxBuff[i].useFlag = FALSE;
                dataAgentTxBuff[i].sendTimeTick = 0;
                dataAgentTxBuff[i].needAckFlag = FALSE;
                memset(dataAgentTxBuff[i].data, 0x0, MAX_AGENT_DATA_SIZE);
                
                //#if(BUILD_DEBUG_VERSION)
                if(dataAgentDebugEnable)
                    sysprintf(" ~> DataAgentRemoveDataByCmdId[%d]: OK [cmdId : %d ]...\r\n", i, cmdId);
                //#endif

                reVal = TRUE;
                break;
            }
            else
            {
                //if(dataAgentDebugEnable)
                //    sysprintf("DataAgentRemoveDataByAck[%d]: cmdIndex error [%d : %d] ...\r\n", i, pCmdHeader->cmdIndex, cmdIndex);
            }
            //return TRUE;
        }
    }  
    
    if(reVal == FALSE)    
        sysprintf(" ~> DataAgentRemoveDataByCmdId: ERROR [cmdId : %d ]...\r\n", cmdId); 
    
    xSemaphoreGive(xRxTxSemaphore);
    return reVal;
}

static void initBuffData(void)
{
    int i;
    for(i = 0; i<MAX_AGENT_BUFF_NUMBER; i++)
    {
        dataAgentTxBuff[i].needAckFlag = FALSE;
        dataAgentTxBuff[i].useFlag = FALSE;
        dataAgentTxBuff[i].callback = NULL;
        dataAgentTxBuff[i].sendTimeTick = 0;
        memset(dataAgentTxBuff[i].data, 0x0, MAX_AGENT_DATA_SIZE);
    }    
}
static BOOL hwInit(void)
{   
    return TRUE;
}

static BOOL swInit(void)
{   
    PowerRegCallback(&dataAgentPowerCallabck);
    initBuffData();
    xRxTxSemaphore = xSemaphoreCreateMutex();
    xDataAgentRoutineSemaphore = xSemaphoreCreateBinary();
    xDataAgentFileTransferSemaphore = xSemaphoreCreateBinary();
    xDataAgentTxSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vDataAgentRxTask, "vDataAgentRxTask", 1024*10, NULL, DATA_AGENT_RX_THREAD_PROI, NULL ); 
    xTaskCreate( vDataAgentTxTask, "vDataAgentTxTask", 1024*10, NULL, DATA_AGENT_TX_THREAD_PROI, NULL ); 
    xTaskCreate( vDataAgentRoutineTask, "vDataAgentRoutineTask", 1024*10, NULL, DATA_AGENT_ROUTINE_THREAD_PROI, NULL ); 
    xTaskCreate( vDataAgentFileTransferTask, "vDataAgentFileTransferTask", 1024*10, NULL, DATA_AGENT_ROUTINE_THREAD_PROI, NULL ); 
    return TRUE;
}
  
    
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL DataAgentInit(BOOL testModeFlag)
{
    sysprintf("DataAgentInit!!\n");
    //pCommunicationInterface = CommunicationGetInterface(COMMUNICATION_RS232_INTERFACE_INDEX/*COMMUNICATION_LORA_INTERFACE_INDEX*/);
    pCommunicationInterface = CommunicationGetInterface(COMMUNICATION_LORA_INTERFACE_INDEX);
    if(pCommunicationInterface == NULL)
    {
        sysprintf("DataAgentInit ERROR (pStorageInterface == NULL)!!\n");
        return FALSE;
    }
    if(pCommunicationInterface->initFunc() == FALSE)
    {
        sysprintf("DataAgentInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(hwInit() == FALSE)
    {
        sysprintf("DataAgentInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("DataAgentInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    sysprintf("DataAgentInit OK!!\n");
    return TRUE;
}
BOOL DataAgentAddData(uint8_t* pData, uint16_t dataLen, BOOL needAck, dataAgentTxCallback callback)
{
    BOOL reVal = FALSE;
    int i;
    if(xRxTxSemaphore == NULL)
    {
        sysprintf("DataAgentAddData: ignore, (xRxTxSemaphore == NULL)...\r\n"); 
        return reVal;
    }
    if(dataAgentDebugEnable)
        sysprintf("DataAgentAddData: Waitinging ...\r\n");
    xSemaphoreTake(xRxTxSemaphore, portMAX_DELAY);
    if(dataAgentDebugEnable)
        sysprintf("DataAgentAddData: Going ...\r\n");
    for(i = 0; i<MAX_AGENT_BUFF_NUMBER; i++)
    {
        if(dataAgentTxBuff[i].useFlag == FALSE)
        {
            dataAgentTxBuff[i].useFlag = TRUE;
            dataAgentTxBuff[i].sendTimeTick = 0;
            dataAgentTxBuff[i].needAckFlag = needAck;
            dataAgentTxBuff[i].callback = callback;
            if(dataLen<MAX_AGENT_DATA_SIZE)
            {
                memcpy(dataAgentTxBuff[i].data, pData, dataLen);
                dataAgentTxBuff[i].dataLen = dataLen; 
            }
            else
            {
                memcpy(dataAgentTxBuff[i].data, pData, MAX_AGENT_DATA_SIZE);
                dataAgentTxBuff[i].dataLen = MAX_AGENT_DATA_SIZE;
            }
            xSemaphoreGive( xDataAgentTxSemaphore); 
            //#if(BUILD_DEBUG_VERSION)
            if(dataAgentDebugEnable)
            {
                CmdHeader* cmdHeader = (CmdHeader*)pData;
                sysprintf("DataAgentAddData[%d]: OK [cmdId = %d, cmdIndex = %d ]...\r\n", i, cmdHeader->cmdId, cmdHeader->cmdIndex);
            }
           // #endif
            reVal = TRUE;
            break;
        }
    }    
    if(reVal == FALSE)
        sysprintf("DataAgentAddData: error...\r\n"); 
    else
        dataAgentPowerStatus = FALSE;
    
    xSemaphoreGive(xRxTxSemaphore);
    return reVal;
}
BOOL DataAgentRemoveDataByAck(uint16_t cmdIndex)
{
    BOOL reVal = FALSE;
    int i;
    xSemaphoreTake(xRxTxSemaphore, portMAX_DELAY);
    for(i = 0; i<MAX_AGENT_BUFF_NUMBER; i++)
    {
        if((dataAgentTxBuff[i].useFlag == TRUE) && (dataAgentTxBuff[i].needAckFlag == TRUE))
        {
            CmdHeader* pCmdHeader = (CmdHeader*)dataAgentTxBuff[i].data;
            if(pCmdHeader->cmdIndex == cmdIndex)
            {
                dataAgentTxBuff[i].useFlag = FALSE;
                dataAgentTxBuff[i].needAckFlag = FALSE;
                dataAgentTxBuff[i].sendTimeTick = 0;
                memset(dataAgentTxBuff[i].data, 0x0, MAX_AGENT_DATA_SIZE);
                
                //#if(BUILD_DEBUG_VERSION)
                if(dataAgentDebugEnable)
                    sysprintf(" -> DataAgentRemoveDataByAck[%d]: OK [index : %d ]...\r\n", i, cmdIndex);
                //#endif
                
                if(dataAgentTxBuff[i].callback != NULL)
                {
                    dataAgentTxBuff[i].callback(0);
                }
                reVal = TRUE;
                break;
            }
            else
            {
                //if(dataAgentDebugEnable)
                //    sysprintf("DataAgentRemoveDataByAck[%d]: cmdIndex error [%d : %d] ...\r\n", i, pCmdHeader->cmdIndex, cmdIndex);
            }
            //return TRUE;
        }
    }  
    
    if(reVal == FALSE)    
        sysprintf(" -> DataAgentRemoveDataByAck: ERROR [index : %d ]...\r\n", cmdIndex); 
    
    xSemaphoreGive(xRxTxSemaphore);
    return reVal;
}




BOOL DataAgentSetDebugEnable(BOOL flag)
{
    dataAgentDebugEnable = flag;
    return dataAgentDebugEnable;
}
#if(1)
#include "ff.h"
#define FILE_TRANSFER_STATUS_IDLE  0x01
#define FILE_TRANSFER_STATUS_INIT  0x02
#define FILE_TRANSFER_STATUS_SEND  0x03
#define FILE_TRANSFER_STATUS_END  0x04

static uint8_t fileTransferStatus = FILE_TRANSFER_STATUS_IDLE;
static FIL MyFile;
static BOOL MyFileOpenFlag = FALSE;
static fileTransferDataPara  mFileTransferDataPara;
static char targetFileNameTmp[FILE_TRANSFER_NAME_LEN];
static uint16_t  mFileTransferCurrentAddress = 0;
static uint16_t  mFileTransferCurrentDataLen = 0;
static uint16_t  mFileTransferCurrentFileLen = 0;
static uint16_t loadFileData(uint8_t* buff, uint16_t len)
{
    if(MyFileOpenFlag)
    {
        FRESULT reval;
        uint16_t NumByteToRead;
        reval = f_read(&MyFile, buff, len, (void *)&NumByteToRead);
        //  
        if(reval != FR_OK)
        {
            sysprintf("loadFileData:Cannot Read (reval != FR_OK)... \r\n"); 
            return 0;
        }            
        return NumByteToRead;;
    }
    else
    {
        sysprintf("loadFileData:not open yet... \r\n"); 
        return 0;
    }
}

static BOOL DataAgentTxCallback(uint16_t address)
{   
    if(FILE_TRANSFER_STATUS_IDLE == fileTransferStatus)
    {
        sysprintf("DataAgentTxCallback ignore[FILE_TRANSFER_STATUS_IDLE]...\r\n"); 
        return FALSE;
    }
    else
    {
        if(MyFileOpenFlag)
        {
            uint16_t readLen;
            //sysprintf("DataAgentTxCallback Going...\r\n"); 
            sysprintf("+");
            mFileTransferCurrentAddress = mFileTransferCurrentAddress +  mFileTransferCurrentDataLen;  
            readLen = loadFileData(mFileTransferDataPara.data, FILE_TRANSFER_DATA_LEN); 
            if( readLen == 0)
            {
                f_close(&MyFile);
                MyFileOpenFlag = FALSE;
                fileTransferStatus = FILE_TRANSFER_STATUS_END;
                xSemaphoreGive(xDataAgentFileTransferSemaphore);             
                return FALSE;
            }    
            mFileTransferCurrentDataLen = readLen;
            fileTransferStatus = FILE_TRANSFER_STATUS_SEND;
            xSemaphoreGive(xDataAgentFileTransferSemaphore);  
        }
        else
        {
             sysprintf("DataAgentTxCallback End...\r\n");  
             fileTransferStatus = FILE_TRANSFER_STATUS_IDLE;
             threadFileTransferWaitTime = portMAX_DELAY;
        }

        return TRUE;
    }
}


BOOL DataAgentStartFileTransfer(char* filename, char* targetFileName)
{      
//    FILINFO fno;
    if(xRxTxSemaphore == NULL)
    {
        sysprintf("DataAgentStartFileTransfer: [%s][%s]: ignore, (xRxTxSemaphore == NULL)...\r\n", filename, targetFileName); 
        return FALSE;
    }
    sysprintf("DataAgentStartFileTransfer: [%s][%s]..\r\n", filename, targetFileName); 
    
    DataAgentStopFileTransfer();
    
    FRESULT reval = f_open(&MyFile, filename, FA_READ);
    if(reval != FR_OK) 
    {
        sysprintf("Cannot Open %s file for reading...\r\n", filename); 
        return FALSE;
    }
    else
    {
        //if(f_stat(filename, &fno) == FR_OK)
        //{
            mFileTransferCurrentFileLen = MyFile.fsize;
        //}
        MyFileOpenFlag = TRUE;
        mFileTransferCurrentAddress = 0;
        mFileTransferCurrentDataLen = 0;
        
        fileTransferStatus = FILE_TRANSFER_STATUS_INIT;
        strcpy(targetFileNameTmp, targetFileName);
        xSemaphoreGive(xDataAgentFileTransferSemaphore);
        return TRUE;
    }
}

void DataAgentStopFileTransfer(void)
{      
//    FILINFO fno;
    sysprintf("DataAgentStopFileTransfer..\r\n"); 
    if(xRxTxSemaphore == NULL)
    {
        sysprintf("DataAgentStopFileTransfer: ignore, (xRxTxSemaphore == NULL)...\r\n"); 
        return;
    }
    
    DataAgentRemoveDataByCmdId(CMD_FILE_FRANSFER_ID);
    DataAgentRemoveDataByCmdId(CMD_FILE_FRANSFER_DATA_ID);
    
    if(MyFileOpenFlag)
    {
        f_close(&MyFile);
        MyFileOpenFlag = FALSE;
        fileTransferStatus = FILE_TRANSFER_STATUS_IDLE;
        threadFileTransferWaitTime = portMAX_DELAY;
        xSemaphoreGive(xDataAgentFileTransferSemaphore);
    }   
}


static void vDataAgentFileTransferTask( void *pvParameters )
{
    sysprintf("vDataAgentFileTransferTask Going...\r\n");       
    for(;;)
    {
        BaseType_t reval = xSemaphoreTake(xDataAgentFileTransferSemaphore, threadFileTransferWaitTime/*portMAX_DELAY*/); 
        switch(fileTransferStatus)
        {
        case FILE_TRANSFER_STATUS_IDLE:
            break;
        case FILE_TRANSFER_STATUS_INIT:
            CmdSendFileTransfer(targetFileNameTmp, mFileTransferCurrentFileLen/*MyFile.fsize*/, FILE_TRANSFER_CMD_STATUS_START, DataAgentTxCallback) ; 
            break;
        case FILE_TRANSFER_STATUS_SEND:
            CmdSendFileTransferData(mFileTransferCurrentAddress, mFileTransferCurrentDataLen, mFileTransferDataPara.data, DataAgentTxCallback);
            break;  
        case FILE_TRANSFER_STATUS_END:
            CmdSendFileTransfer(targetFileNameTmp, mFileTransferCurrentFileLen/*MyFile.fsize*/, FILE_TRANSFER_CMD_STATUS_END, DataAgentTxCallback) ;
            break;
        }
    }
}
#endif
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


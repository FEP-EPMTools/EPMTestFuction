/**************************************************************************//**
* @file     atcmdparser.c
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

#include "fepconfig.h"
#include "atcmdparser.h"
//#include "uart10drv.h"
//#include "dataagent.h"

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/



#define ENABLE_AT_CMD_DEBUG  0
#define ENABLE_AT_CMD_DEBUG_2  0

#define ENABLE_AT_CMD_DEBUG_RECEIVE_RAW 0

#define ENABLE_AT_CMD_DEBUG_FINAL  ENABLE_MODEM_CMD_DEBUG

 
#define TARGET_CMD_ITME_NUM  5

typedef enum{
    DATA_TYPE_OK = 0x01,
    DATA_TYPE_ERROR = 0x02,
    DATA_TYPE_CME_ERROR = 0x03,
    DATA_TYPE_AT_CMD = 0x04,
    DATA_TYPE_CONNECT = 0x05,
    DATA_TYPE_WEB_POST_GET_DATA = 0x06,
    DATA_TYPE_FTP_GET_DATA = 0x07
}DataType;

typedef enum{
    WAIT_OK_FLAG_TRUE = 0x01,
    WAIT_OK_FLAG_FALSE = 0x02
}WaitOKFlag;


typedef struct
{
    uint8_t*	headerStr;
    uint8_t*	endStr;
    DataType	dataType;    
}TargetCmdItem;

typedef struct
{
    TargetCmdItem* item[TARGET_CMD_ITME_NUM];
}TargetCmdList;

typedef struct
{
	CmdReq     cmdId;
    uint8_t*    checkcmd;
    WaitOKFlag        waitOKFlag;
}CmdItem;
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint8_t dataTempBuffer[1024*1024];
static int dataTempBufferLen;

static uint8_t dataTempBuffer2[1024*1024];
static int dataTempBufferLen2;
//#if(ENABLE_MODEM_CMD_DEBUG)
//int enableReceiveDebug = 1;
//#else
int enableReceiveDebug = 0;
//#endif
static CmdItem cmdList[] = { { CMD_REQ_EXIST_OK, (uint8_t*)"\r\n+CPIN: READY\r\n", WAIT_OK_FLAG_TRUE},                               
                                {CMD_REQ_REG_OK, (uint8_t*)"\r\n+CREG: 0,1", WAIT_OK_FLAG_TRUE},	
                                {CMD_REQ_GATT_OK, (uint8_t*)"\r\n+CGATT: 1", WAIT_OK_FLAG_TRUE},	
                                {CMD_REQ_GREG_OK, (uint8_t*)"\r\n+CGREG: 0,1", WAIT_OK_FLAG_TRUE},
                                {CMD_REQ_QUERY_CONNECTED_OK, (uint8_t*)"\r\n+QIACT: 1,1,1,", WAIT_OK_FLAG_TRUE},
                                {CMD_REQ_QUERY_FTP_CONNECTED_OK, (uint8_t*)"\r\n+QFTPOPEN: 0,0\r\n", WAIT_OK_FLAG_FALSE},
                                {CMD_REQ_FTP_SEND_DATA_OK, (uint8_t*)"\r\n+QFTPPUT: 0,", WAIT_OK_FLAG_FALSE},
                                {CMD_REQ_FTP_SEND_DATA_TIMEOUT, (uint8_t*)"\r\n+QFTPPUT: 609,0\r\n", WAIT_OK_FLAG_FALSE}, 
                                {CMD_REQ_FTP_DISCONNECT, (uint8_t*)"\r\n+QFTPCLOSE:", WAIT_OK_FLAG_FALSE}, 
                                {CMD_REQ_WEB_POST_OK, (uint8_t*)"\r\n+QHTTPPOST: 0,200", WAIT_OK_FLAG_FALSE}, 
                                {CMD_REQ_FTP_CHANGE_DIR_OK, (uint8_t*)"\r\n+QFTPCWD: 0,0\r\n", WAIT_OK_FLAG_FALSE},
                                {CMD_REQ_FTP_CHANGE_DIR_ERROR, (uint8_t*)"\r\n+QFTPCWD: 627", WAIT_OK_FLAG_FALSE},                               
                                {CMD_REQ_FTP_MAKE_DIR_OK, (uint8_t*)"\r\n+QFTPMKDIR:", WAIT_OK_FLAG_FALSE},//不用 \r\n+QFTPMKDIR: 0,0
                                {CMD_REQ_FTP_MAKE_DIR_ERROR, (uint8_t*)"\r\n+QFTPMKDIR: 627", WAIT_OK_FLAG_FALSE}, 
                                {CMD_REQ_FTP_GET_DIR_OK, (uint8_t*)"\r\n+QFTPPWD: 0,", WAIT_OK_FLAG_FALSE},//不用 \r\n+QFTPMKDIR: 0,0
                                {CMD_REQ_FTP_GET_DIR_ERROR, (uint8_t*)"\r\n+QFTPPWD: 627", WAIT_OK_FLAG_FALSE}, 
                                {CMD_REQ_QUERY_CSQ_OK, (uint8_t*)"\r\n+CSQ: ", WAIT_OK_FLAG_TRUE}, 
                                

                                {CMD_REQ_NULL, (uint8_t*) "" , WAIT_OK_FLAG_TRUE}};


static TargetCmdItem targetCmdItemOK = {(uint8_t*)"\r\nOK", (uint8_t*)"\r\n", DATA_TYPE_OK};
static TargetCmdItem targetCmdItemError = {(uint8_t*)"\r\nERROR", (uint8_t*)"\r\n", DATA_TYPE_ERROR};
static TargetCmdItem targetCmdItemCMEError = {(uint8_t*)"\r\n+CME ERROR:", (uint8_t*)"\r\n", DATA_TYPE_CME_ERROR};
static TargetCmdItem targetCmdItemATCmd = {(uint8_t*)"\r\n+", (uint8_t*)"\r\n", DATA_TYPE_AT_CMD};
static TargetCmdItem targetCmdItemConnect = {(uint8_t*)"\r\nCONNECT", (uint8_t*)"\r\n", DATA_TYPE_CONNECT}; //none "+"

static TargetCmdItem targetCmdItemWebPostGetData = {(uint8_t*)"\r\nCONNECT", (uint8_t*)"+QHTTPREAD: 0\r\n", DATA_TYPE_WEB_POST_GET_DATA}; //none "+"

static TargetCmdItem targetCmdItemFtpGetData = {(uint8_t*)"\r\nCONNECT", (uint8_t*)"+QFTPGET: 0", DATA_TYPE_FTP_GET_DATA};

static TargetCmdList targetCmdList = {{ &targetCmdItemOK, &targetCmdItemError, &targetCmdItemCMEError, &targetCmdItemATCmd, &targetCmdItemConnect}};

static TargetCmdList webPostTargetCmdList = {{ &targetCmdItemOK, &targetCmdItemError, &targetCmdItemCMEError, &targetCmdItemATCmd, &targetCmdItemWebPostGetData}};

static TargetCmdList ftpCmdList = {{ &targetCmdItemOK, &targetCmdItemError, &targetCmdItemCMEError, &targetCmdItemATCmd, &targetCmdItemFtpGetData}};

static CmdReq currentCmdType = CMD_REQ_NULL;

static uint8_t cmdDataTmp[4*1024];
static int cmdDataTmpIndex = 0;;
static uint8_t cmdData[4*1024];


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
//#if(ENABLE_AT_CMD_DEBUG | ENABLE_AT_CMD_DEBUG_2 | ENABLE_AT_CMD_DEBUG_RECEIVE_RAW)
//#if(0)
static void printfBuffData(char* str, uint8_t* data, int len)
{
    int i;
    sysprintf("\r\n %s: len = %d...\r\n   -%02d:-> ", str, len, len);
    
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

static CmdReq fineCurrentCmdRegType(uint8_t* data, uint8_t len, WaitOKFlag* waitOKFlag)
{
    int i;
    #if(ENABLE_AT_CMD_DEBUG_2)
            sysprintf("   ~enter~ fineCurrentCmdRegType\r\n");
    #endif
    *waitOKFlag = WAIT_OK_FLAG_FALSE;
    for(i = 0; ; i++)
    {
		if (cmdList[i].cmdId == CMD_REQ_NULL)
            break;
        if(memcmp(data, cmdList[i].checkcmd, strlen((char*)cmdList[i].checkcmd)) == 0)
        {
            #if(ENABLE_AT_CMD_DEBUG_2)
            sysprintf("   ~~ fineCurrentCmdRegType return: %d (len = %d) \r\n", cmdList[i].cmdId, (unsigned char)len);
            #endif
            *waitOKFlag = cmdList[i].waitOKFlag;
            return cmdList[i].cmdId;
        }
    }
    return CMD_REQ_NULL;
}

static CmdReq preProcessCmdData(DataType targetHeaderIndex, uint8_t* data, int len, ParserType parserType)
{
	CmdReq cmdReq = CMD_REQ_NULL;
    WaitOKFlag waitOKFlag;
    //sysprintf("   ~ AT Receive [type %d] : (%d)~ [%s] \r\n", targetHeaderIndex, len, data);
    switch(targetHeaderIndex)
    {
    case DATA_TYPE_OK:
        if(currentCmdType != CMD_REQ_NULL)
        {
            cmdReq = currentCmdType;
            currentCmdType = CMD_REQ_NULL;
        }
        else
        {
            cmdReq = CMD_REQ_OK;
        }
        break;
        
    case DATA_TYPE_ERROR:
        cmdReq = CMD_REQ_ERROR;
        currentCmdType = CMD_REQ_NULL;
        //writeFlag = 0;
        break;
    case DATA_TYPE_AT_CMD:
        currentCmdType = fineCurrentCmdRegType(data, len, &waitOKFlag);
        if(currentCmdType != CMD_REQ_NULL)
        {
            if(waitOKFlag == WAIT_OK_FLAG_FALSE)
            {
                cmdReq = currentCmdType;
                currentCmdType = CMD_REQ_NULL;
            }
            else
            {
                memcpy(dataTempBuffer2, data, len);
                dataTempBuffer2[len] = 0x0;
                dataTempBufferLen2 = len;
            }
        }
        break;
        
    case DATA_TYPE_CME_ERROR:
        cmdReq = CMD_REQ_CME_ERROR;
        currentCmdType = CMD_REQ_NULL;
    #if(ENABLE_AT_CMD_DEBUG_FINAL)
        sysprintf("\r\n   ERROR ~ DATA_TYPE_CME_ERROR(%d) ~ [%s]\r\n", len, data);
    #endif
        //writeFlag = 0;
        break;
    
    case DATA_TYPE_CONNECT:
        cmdReq = CMD_REQ_CONNECT_OK;
        currentCmdType = CMD_REQ_NULL;
        //writeFlag = 0;
        break;

    case DATA_TYPE_WEB_POST_GET_DATA:
        cmdReq = CMD_REQ_WEB_POST_GET_DATA;
        currentCmdType = CMD_REQ_NULL;
        //writeFlag = 0;
        break;
    
    case DATA_TYPE_FTP_GET_DATA:
        cmdReq = CMD_REQ_FTP_GET_DATA_OK;
        currentCmdType = CMD_REQ_NULL;
        //writeFlag = 0;
        break;
    }
    
    if(cmdReq != CMD_REQ_NULL)
    {
        //BaseType_t DataAgentSignalReqCmd(void *pvBuffer);
        //DataAgentSignalReqCmd(&cmdReq);
        //if((cmdReq != CMD_REQ_OK) && (cmdReq != CMD_REQ_ERROR))
        {
            memcpy(dataTempBuffer, data, len);
            dataTempBuffer[len] = 0x0;
            dataTempBufferLen = len;
        }
        #if(ENABLE_AT_CMD_DEBUG_FINAL)
        //sysprintf("\r\n  ~ CMD TYPE ~  *** [%03d] ***\r\n", cmdReq);
        sysprintf("\r\n  ~ CMD TYPE ~  *** [%03d:%s] ***\r\n", cmdReq, dataTempBuffer);
        #endif
        
    }
    else
    {
        dataTempBufferLen = 0;
    }
   
    return cmdReq;
}

static uint8_t* findStartAddress(uint8_t* srcPr, int srcLen, uint8_t* destPr, int destLen)
{
    int i;
    for (i = 0; i < (srcLen - destLen)+1; i++)
    {
        if(memcmp(srcPr+i, destPr, destLen) == 0)
        {
            return srcPr + i;
        }
#if(0)
        for (j = 0; j < destLen; j++)
        {
            if (srcPr[i+j] != destPr[j])
                break;
        }
        if (j == destLen)
            return srcPr + i;
#endif
    }
    return NULL;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/

CmdReq atCmdProcessReadData(uint8_t* data, int len, ParserType parserType)
{
    //printfBuffData((char*)"DATA TEMP", cmdDataTmp, cmdDataTmpIndex);
    CmdReq reVal = CMD_REQ_NULL;
    int i;
    uint8_t* startAddress;
    int     targetHeaderIndex;
    uint8_t* targetStartTempPr = NULL;
    uint8_t* targetEndTempPr = NULL;
    int targetLen = 0;

    int dataCurrentIndex = 0;
    int dataLeftLen = 0;
    uint8_t* dataCurrentPr = 0;
	int totalCmdDataLen;
    
    TargetCmdList* pTargetCmdList = NULL;
    
    switch(parserType)
    {
        case PARSER_TYPE_NORMAL:
            pTargetCmdList = &targetCmdList;
            break;
        case PARSER_TYPE_WEB_POST:
            pTargetCmdList = &webPostTargetCmdList;
            break;
        case PARSER_TYPE_FTP:
            pTargetCmdList = &ftpCmdList;
            break;
    }

    
//#if(ENABLE_AT_CMD_DEBUG)
#if(0)
    printfBuffData((char*)" [START]", data, len);
#endif
	totalCmdDataLen = cmdDataTmpIndex + len;
	//{
	//	char str[512];
	//	sprintf(str, " @@  totalCmdDataLen = %d, cmdDataTmpIndex = %d, len = %d\r\n", totalCmdDataLen, cmdDataTmpIndex, len);
	//	updateMainMessagechar(str);
	//}
    memcpy(cmdData, cmdDataTmp, cmdDataTmpIndex);
    memcpy(cmdData + cmdDataTmpIndex, data, len);
    data = cmdData;
	
//#if(ENABLE_AT_CMD_DEBUG)
//#if(ENABLE_AT_CMD_DEBUG_RECEIVE_RAW)
    if(enableReceiveDebug)
        printfBuffData((char*)"START_2", data, totalCmdDataLen);
//#endif
    cmdDataTmpIndex = 0;
    while (dataCurrentIndex < totalCmdDataLen)
    {   
 redo:
        //sysprintf("  \r\n *[INFO]** Start check 1(%d): dataCurrentIndex = %d, dataLeftLen = %d\r\n", totalCmdDataLen, dataCurrentIndex, dataLeftLen);
        dataLeftLen = totalCmdDataLen - dataCurrentIndex;
        dataCurrentPr = data + dataCurrentIndex;
        
        if(dataLeftLen <= 0)
        {
            //sysprintf("  \r\n *[INFO]** atCmdProcessReadData: break\r\n");
            break;
        }
        #if(ENABLE_AT_CMD_DEBUG_2)
        //#if(1)
        sysprintf("  \r\n *[INFO]** Start check 2: dataCurrentIndex = %d, dataLeftLen = %d\r\n", dataCurrentIndex, dataLeftLen);
        #endif
        targetStartTempPr = 0;
        targetHeaderIndex = -1;
        for(i = 0; i<TARGET_CMD_ITME_NUM; i++)    
        {
            startAddress  = findStartAddress(dataCurrentPr, dataLeftLen, pTargetCmdList->item[i]->headerStr, strlen((char*)pTargetCmdList->item[i]->headerStr));
            if(startAddress)
            {
                if(targetStartTempPr == 0)
                {
                    targetStartTempPr = startAddress;
                    targetHeaderIndex = i;
                }
                else
                {
                    if(targetStartTempPr > startAddress)
                    {
                        targetStartTempPr = startAddress;
                        targetHeaderIndex = i;
                    }
                }
            }           
        }
        #if(ENABLE_AT_CMD_DEBUG_2)
        sysprintf("   ==> get  targetHeaderIndex  %d\r\n", targetHeaderIndex);
        #endif
        if(targetHeaderIndex != -1)
        {
            i = targetHeaderIndex;
            //sysprintf("   ==> check [%d]: %d, %d\r\n", i, strlen((char*)pTargetCmdList->item[i]->headerStr), strlen((char*)pTargetCmdList->item[i]->endStr));
            targetLen = 0;
            //targetStartTempPr = findStartAddress(dataCurrentPr, dataLeftLen, pTargetCmdList->item[i]->headerStr, strlen((char*)pTargetCmdList->item[i]->headerStr));
            #if(ENABLE_AT_CMD_DEBUG_2)
            sysprintf("   =find header=> %d\r\n", targetStartTempPr - dataCurrentPr);
            #endif
            if(targetStartTempPr)
            {
                targetEndTempPr = findStartAddress(targetStartTempPr+1, dataLeftLen - (targetStartTempPr - dataCurrentPr), pTargetCmdList->item[i]->endStr, strlen((char*)pTargetCmdList->item[i]->endStr));
                #if(ENABLE_AT_CMD_DEBUG_2)
                sysprintf("   =find end=> %d\r\n", targetEndTempPr - dataCurrentPr);
                #endif
                if(targetEndTempPr)
                {
                    targetLen = targetEndTempPr - targetStartTempPr + strlen((char*)pTargetCmdList->item[i]->endStr);
                    dataCurrentPr = targetStartTempPr; 
                }
                else
                {
                    #if(ENABLE_AT_CMD_DEBUG_2)
                    sysprintf("   =Cant find end=>\r\n");
                    #endif
                }

                if (targetLen)
                {//頭尾都有找到
                    #if(ENABLE_AT_CMD_DEBUG_2)
                    sysprintf("   =targetHeaderIndex = %d, dataType = %d=>\r\n", targetHeaderIndex, pTargetCmdList->item[targetHeaderIndex]->dataType);
                    #endif 
					memset(cmdDataTmp, 0x0, sizeof(cmdDataTmp));
                    memcpy(cmdDataTmp, dataCurrentPr, targetLen);
                    dataCurrentIndex = targetEndTempPr - data + strlen((char*)(pTargetCmdList->item[i]->endStr));
                    cmdDataTmpIndex = targetLen;
                    #if(ENABLE_AT_CMD_DEBUG)
                    printfBuffData((char*) "[DATA]", cmdDataTmp, cmdDataTmpIndex);
                    #endif
                    //processCmdData(preProcessCmdData(pTargetCmdList->item[targetHeaderIndex]->dataType, cmdDataTmp, cmdDataTmpIndex));
                    reVal = preProcessCmdData(pTargetCmdList->item[targetHeaderIndex]->dataType, cmdDataTmp, cmdDataTmpIndex, parserType);
                    cmdDataTmpIndex = 0;
                    goto redo;
                }
                else
                {//只有找到頭  沒有尾 還沒有收完 

                }
            }
            else
            {//沒找到這次頭
                #if(ENABLE_AT_CMD_DEBUG_2)
                sysprintf("   =Cant find header=>\r\n");
                #endif
            }
        }
        #if(ENABLE_AT_CMD_DEBUG_2)
        sysprintf("   =copy to temp=>\r\n");
        printfBuffData((char*)"Copy to TEMP", dataCurrentPr, dataLeftLen);
        #endif
        memcpy(cmdDataTmp, dataCurrentPr, dataLeftLen);
        dataCurrentIndex = dataCurrentIndex + dataLeftLen;
        cmdDataTmpIndex = dataLeftLen;
    }
    return reVal;
}

void ATCmdSetReceiveDebugFlag(int flag)
{
    enableReceiveDebug = flag;
}

BOOL ATCmdGetReceiveDebugFlag(void)
{
    return enableReceiveDebug;
}

uint8_t* ATCmdDataTempBuffer(int* len)
{
    if(len != NULL)
    {
        *len = dataTempBufferLen;
    }
    //printfBuffData((char*)"Get Temp buffer", dataTempBuffer, dataTempBufferLen);
    return dataTempBuffer;
}

uint8_t* ATCmdDataTempBuffer2(int* len)
{
    if(len != NULL)
    {
        *len = dataTempBufferLen2;
    }
    //printfBuffData((char*)"Get Temp buffer", dataTempBuffer, dataTempBufferLen);
    return dataTempBuffer2;
}

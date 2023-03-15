/**************************************************************************//**
* @file     fileagent.c
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

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "fileagent.h"
#include "fatfslib.h"
#include "ff.h"
#include "yaffs2drv.h"
#include "yaffsfs.h"
#include "powerdrv.h"
#include "dataprocesslib.h"

#if (ENABLE_BURNIN_TESTER)
#include "rtc.h"
#include "burnintester.h"
#include "sflashrecord.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define FILE_DATA_QUEUE_SIZE        128

#define FA_HEADER_VALUE             0xda   
#define FA_HEADER_VALUE2		    0xad  
    
#define FA_END_VALUE                0x1f   
#define FA_END_VALUE2		        0xf1  

#if (ENABLE_BURNIN_TESTER)
#define BURNIN_ERRMSG_BUFFER_MAX    10240
#endif

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static BOOL initFlag = FALSE;
static FileAgentDataItem fileAgentDataItem[FILE_DATA_QUEUE_SIZE];
static int headerIndex = 0;
static int endIndex = 0;
static SemaphoreHandle_t xSemaphore;
static SemaphoreHandle_t xAddDataMutex;
static SemaphoreHandle_t xFatfsReadWriteMutex;
static SemaphoreHandle_t xYaffs2ReadWriteMutex;

static TickType_t threadWaitTime        = portMAX_DELAY;

static int error = 0;

#pragma pack(1)
typedef struct
{
    uint8_t         value[2];    //2 byte
    uint16_t        Len;
}FAHeader; //9 bytes


typedef struct
{
    uint16_t    checksum;
    uint8_t     value[2];     //2 byte
}FAEnd; //4 bytes
#pragma pack()

static int autoFormatCounter1 = 0;
static int autoFormatCounter2 = 0;

static FAHeader faHeader = {{FA_HEADER_VALUE, FA_HEADER_VALUE2}, 0};
static FAEnd faEnd = {0,  {FA_END_VALUE, FA_END_VALUE2}};


static BOOL fileAgentPowerStatus = TRUE;
static BOOL fileAgentPowerStatusFlag = FALSE;
static BOOL FileAgentCheckStatus(int flag);
static BOOL FileAgentPreOffCallback(int flag);
static BOOL FileAgentOffCallback(int flag);
static BOOL FileAgentOnCallback(int flag);
static powerCallbackFunc fileAgentPowerCallabck = {" [FileAgent] ", FileAgentPreOffCallback, FileAgentOffCallback, FileAgentOnCallback, FileAgentCheckStatus};

#if (ENABLE_BURNIN_TESTER)
static SemaphoreHandle_t xBurninErrorMessageMutex;
static char burninErrLogFilename[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
static char burninErrLogFilepath[BURNIN_LOG_FILENAME_BUFFER_LENGTH];
static char burninErrMsgBuffer[BURNIN_ERRMSG_BUFFER_MAX];
static uint32_t burninErrMsgLength = 0;
#endif


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


static BOOL FileAgentPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    fileAgentPowerStatusFlag = TRUE;
    //sysprintf("### FileAgent OFF Callback [%s] ###\r\n", fileAgentPowerCallabck.drvName);    
    return reVal;    
}
static BOOL FileAgentOffCallback(int flag)
{
    int timers;
    if(flag)
    {
        timers = 10000/10;
    }
    else
    {
        timers = 2000/10;
    }
    
    while(!fileAgentPowerStatus)
    {
        sysprintf("[f]");
        if(timers-- == 0)
        {
            sysprintf("\r\n ####  [FileAgentOffCallback FALSE]  ####  \r\n");
            return FALSE;
        }
        vTaskDelay(10/portTICK_RATE_MS); 
    }
    return TRUE;   
}
static BOOL FileAgentOnCallback(int flag)
{
    BOOL reVal = TRUE;
    fileAgentPowerStatusFlag = FALSE;
    return reVal;    
}
static BOOL FileAgentCheckStatus(int flag)
{
    //BOOL reVal = TRUE;
    //sysprintf("### fileAgent STATUS Callback [%s] ###\r\n", fileAgentPowerCallabck.drvName); 
    return fileAgentPowerStatus;    
}


/*
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
*/
static uint16_t getChecksum(uint8_t* pTarget, uint16_t len, char* str)
{
    int i;
    uint16_t checksum = 0;
    uint8_t* pr = (uint8_t*)pTarget;
    for(i = 0; i< len; i++) //???checksum ?????
    {
        checksum = checksum + pr[i];
    }
    //sysprintf("  -- getChecksum (%s) : checksum = 0x%x (%d)\r\n", str, checksum, checksum); 
    return checksum;
}

static FRESULT fatfsMkdir(char* dir)
{
    //reval = f_mkdir("0:\\Test1");
    //reval = f_mkdir("0:\\Test1\\Test2");
    //reval = f_mkdir("0:\\Test1\\Test2\\Test3");
    int dirLength = strlen(dir);
    if (dir[dirLength-1] == ':') {
        return FR_OK;
    }
    //If last character is slash symbol, cut it !!
    if ((dir[dirLength-1] == '\\') || (dir[dirLength-1] == '/'))
    {
        dir[dirLength-1] = 0x00;
        dirLength--;
    }
    
    int pathLevel = 0;
    int i;
    //Calculate the number of Levels of sub-directory
    for (i = 0 ; i < dirLength ; i++)
    {
        if ((dir[i] == '\\') || (dir[i] == '/')) {
            pathLevel++;
        }
    }
    if (pathLevel == 0) {
        return FR_OK;
    }
    
    char pathName[256];
    int buffIdx = 0;
    memset(pathName, 0x00, sizeof(pathName));
    //Check and mkdir ==> level by level
    for (i = 1 ; i <= pathLevel ; i++)
    {
        int hits = 0;
        for (buffIdx = 0 ; buffIdx < dirLength ; buffIdx++)
        {
            if ((dir[buffIdx] == '\\') || (dir[buffIdx] == '/'))
            {
                hits++;
                if (hits == (i + 1)) {
                    break;
                }
            }
            pathName[buffIdx] = dir[buffIdx];
        }
        if (f_chdir(pathName) != FR_OK)
        {
            FRESULT reval = f_mkdir(pathName);
            if (reval != FR_OK) {
                return reval;
            }
        }
    }
    return FR_OK;
}

static BOOL fatfsSaveToFile(char* dir,char* name, FileAgentAddType addType, uint8_t* buff, size_t buffLen, BOOL checkMode)
{    
    FIL MyFile;
    uint32_t bytesWritten = 0;
    FRESULT reval;
    char fileName[_MAX_LFN];
    xSemaphoreTake(xFatfsReadWriteMutex, portMAX_DELAY); 
    //sysprintf("fatfsSaveToFile %s%s (addType:%d, buffLen:%d): [%s]\r\n", dir, name, addType, buffLen, buff);
    //sysprintf("fatfsSaveToFile %s%s (addType:%d, buffLen:%d)\r\n", dir, name, addType, buffLen);
    
    // 20200601 reference original burning program and modify by Steven-------------------- 
    //sprintf(fileName, "%s%s", dir, name);
    
    fatfsMkdir(dir);
    int dirLength = strlen(dir);
    if ((dir[dirLength-1] == ':') || (dir[dirLength-1] == '\\') || (dir[dirLength-1] == '/')) {
        sprintf(fileName, "%s%s", dir, name);
    }
    else {
        sprintf(fileName, "%s\\%s", dir, name);
    }
    //-------------------------------------------------------------------------------------
    switch(addType)
    {
        case FILE_AGENT_ADD_DATA_TYPE_OVERWRITE:
            f_unlink(fileName);
            reval = f_open(&MyFile, fileName, FA_CREATE_NEW|FA_WRITE);
            //reval = f_open(&MyFile, fileName, FA_CREATE_ALWAYS|FA_WRITE);
            break;
        case FILE_AGENT_ADD_DATA_TYPE_APPEND:
            reval = f_open(&MyFile, fileName, FA_OPEN_ALWAYS|FA_WRITE);
            break;    
    }    
    //sysprintf("fatfsSaveToFile %s%s OPEN OK\r\n", dir, name);
    if(reval != FR_OK) 
    {
        sysprintf("fatfsSaveToFile %s : open error[%d]\r\n", fileName, reval);
        xSemaphoreGive(xFatfsReadWriteMutex); 
        return FALSE;
    }
    
    switch(addType)
    {
        case FILE_AGENT_ADD_DATA_TYPE_OVERWRITE:
            break;
        case FILE_AGENT_ADD_DATA_TYPE_APPEND:
            //sysprintf("fatfsSaveToFile %s : f_lseek %d \r\n", fileName, f_size(&MyFile));
            f_lseek(&MyFile, f_size(&MyFile));
            break;    
    }
    if(checkMode)
    {
        faHeader.Len = buffLen;
        reval= f_write (&MyFile, &faHeader, sizeof(faHeader), (void *)&bytesWritten);
        if((bytesWritten != sizeof(faHeader)) || (reval != FR_OK)) /*EOF or Error*/
        {
            sysprintf("fatfsSaveToFile %s : write faHeader error (%d)\r\n", fileName, reval);
            f_close(&MyFile); 
            reval = f_unlink(fileName);
            sysprintf("fatfsSaveToFile %s : delete it (errorcode: %d)\r\n", fileName, reval);
            xSemaphoreGive(xFatfsReadWriteMutex); 
            return FALSE;
        }
    }
    //sysprintf("fatfsSaveToFile %s%s START WRITE\r\n", dir, name);
    reval= f_write (&MyFile, buff, buffLen, (void *)&bytesWritten);    
    if((bytesWritten != buffLen) || (reval != FR_OK)) /*EOF or Error*/
    {
        sysprintf("fatfsSaveToFile %s : write error\r\n", fileName);
        f_close(&MyFile); 
        reval = f_unlink(fileName);
        sysprintf("fatfsSaveToFile %s : delete it (errorcode: %d)\r\n", fileName, reval);
        xSemaphoreGive(xFatfsReadWriteMutex); 
        return FALSE;
    }
    //sysprintf("fatfsSaveToFile %s%s WRITE OK\r\n", dir, name);
    if(checkMode)
    {
        faEnd.checksum = getChecksum(buff, buffLen, "fatfsSaveToFile");
        reval= f_write (&MyFile, &faEnd, sizeof(faEnd), (void *)&bytesWritten);
        if((bytesWritten != sizeof(faEnd)) || (reval != FR_OK)) /*EOF or Error*/
        {
            sysprintf("fatfsSaveToFile %s : write faEnd error (%d)\r\n", fileName, reval);
            f_close(&MyFile); 
            reval = f_unlink(fileName);
            sysprintf("fatfsSaveToFile %s : delete it (errorcode: %d)\r\n", fileName, reval);
            xSemaphoreGive(xFatfsReadWriteMutex); 
            return FALSE;
        }
    }
    
    reval = f_close(&MyFile);
    if(reval != FR_OK)
    {
        sysprintf("fatfsSaveToFile %s : close error (%d)\r\n", fileName, reval);
        reval = f_unlink(fileName);
        sysprintf("fatfsSaveToFile %s : delete it (errorcode: %d)\r\n", fileName, reval);
        xSemaphoreGive(xFatfsReadWriteMutex); 
        return FALSE;
    }
    //sysprintf("\r\n !!! fatfsSaveToFile %s : Success[%s]\r\n", fileName, buff);
    sysprintf("\r\n !![Information FileAgent]!! fatfsSaveToFile %s : Success\r\n", fileName);
    //FatfsListFileEx(dir);
    //sysprintf("[OK]");
    xSemaphoreGive(xFatfsReadWriteMutex); 
    return TRUE;
}     

  

static FileAgentReturn fatfsGetReturnValueByDir(char* dir)
{
    int i;
    for(i = 0; i < FatFsGetCounter(); i++)
    {
        if(FatFsGetExistFlag((FatfsIndex)i))
        {
            if(strcmp(FatFsGetRootStr((FatfsIndex)i), dir) == 0)
            {
                switch(i)
                {
                    case 0:
                        return FILE_AGENT_RETURN_OK_FATFS_0;
                    case 1:
                        return FILE_AGENT_RETURN_OK_FATFS_1;
                    case 2:
                        return FILE_AGENT_RETURN_OK_FATFS_2;
                    default:
                        return FILE_AGENT_RETURN_ERROR;
                }                
            }
        }
    }
    return FILE_AGENT_RETURN_ERROR;
}   


static BOOL fatfsGetFromFile(char* dir, char* name, uint8_t** buff, size_t* buffLen, BOOL checkMode)
{
    FIL MyFile;
    FRESULT res;
    uint16_t NumByteToRead;
    char fileName[_MAX_LFN];
    xSemaphoreTake(xFatfsReadWriteMutex, portMAX_DELAY); 
    sprintf(fileName, "%s%s", dir, name);  
    FRESULT reval = f_open(&MyFile, fileName, FA_READ);  
    //sysprintf("fatfsGetFromFile %s enter \r\n", fileName);
    if(reval != FR_OK) 
    {
        sysprintf("fatfsGetFromFile %s : open error[%d]\r\n", fileName, reval);
        xSemaphoreGive(xFatfsReadWriteMutex); 
        return FALSE;
    }
    else
    {     
        *buffLen = f_size(&MyFile);//MyFile.fsize;
        if(*buffLen > 0)
        {
            *buff =  pvPortMalloc(*buffLen);   
            if(buff != NULL)
            {
                res = f_read(&MyFile, *buff, *buffLen, (void *)&NumByteToRead);
                f_close(&MyFile);        
                if((NumByteToRead == 0) || (NumByteToRead != *buffLen) || (res != FR_OK)) /*EOF, len error or Error*/
                {
                    sysprintf("fatfsGetFromFile %s : read error\r\n", fileName);
                    xSemaphoreGive(xFatfsReadWriteMutex); 
                    return FALSE;
                }
            }
            else
            {
                sysprintf("fatfsGetFromFile %s : pvPortMalloc error\r\n", fileName);
                f_close(&MyFile);
                xSemaphoreGive(xFatfsReadWriteMutex);                 
                return FALSE;
            }
        }
        else
        {
            sysprintf("fatfsGetFromFile %s : f_size(&MyFile) error\r\n", fileName);
            f_close(&MyFile);
            xSemaphoreGive(xFatfsReadWriteMutex); 
            return FALSE;
        }
        f_close(&MyFile);
              
    }
    
    if(checkMode)
    {
        uint8_t* buffTmp = *buff;
        size_t buffLenTmp = *buffLen;
        uint8_t* targetData;
        size_t targetDataLen;
        
        //printfBuffData("-- FileAgentGetData --", buffTmp, buffLenTmp);
        if(FileAgentParserAutoData(buffTmp, buffLenTmp, &targetData, &targetDataLen))
        {
            //sysprintf("[ INFO ]processRead parser (len = %d):[%s]...\r\n", targetDataLen, targetData); 
            *buff =  pvPortMalloc(targetDataLen);   
            if(buff != NULL)
            {
                *buffLen = targetDataLen;
                memcpy(*buff, targetData, targetDataLen);     
                vPortFree(buffTmp);                
            }
            else
            {
                sysprintf("fatfsGetFromFile %s : pvPortMalloc error\r\n", fileName);
                vPortFree(buffTmp);
                xSemaphoreGive(xFatfsReadWriteMutex); 
                return FALSE;
            }
            //sysprintf("processRead Free data...\r\n"); 
            
        }
        else
        {
            sysprintf("\r\n !![Information FileAgent]!! fatfsGetFromFile %s : parser error delete it \r\n", fileName);
            f_unlink(fileName);
            xSemaphoreGive(xFatfsReadWriteMutex); 
            return FALSE;
        }
    }
    sysprintf("\r\n !![Information FileAgent]!! fatfsGetFromFile %s : Success\r\n", fileName);
    //FatfsListFileEx(dir);
    //sysprintf("[OK]\r\n");
    xSemaphoreGive(xFatfsReadWriteMutex); 
    return TRUE;

}
//--------------
#define AUTO_SAVE_RETRY_TIMES    10
static FileAgentReturn fatfsAutoSaveToFile(char* name, FileAgentAddType addType, uint8_t* buff, size_t buffLen, BOOL checkMode)
{
    //sysprintf(" -> fatfsAutoSaveToFile %s (addType:%d, buffLen:%d): [%s]\r\n", name, addType, buffLen, buff);
    int i;
    char fileNameTmp[_MAX_LFN];
    FileAgentReturn reVal = FILE_AGENT_RETURN_ERROR;
    int reTryCounter = 0;
    char* targetFile = name;
    for(i = 0; i < FatFsGetCounter(); i++)
    {
        if(FatFsGetExistFlag((FatfsIndex)i))
        {
            reTryCounter = 0;
            while(reTryCounter < AUTO_SAVE_RETRY_TIMES)
            {
                if(fatfsSaveToFile(FatFsGetRootStr((FatfsIndex)i), targetFile, addType, buff, buffLen, checkMode))
                {
                    reVal = reVal|fatfsGetReturnValueByDir(FatFsGetRootStr((FatfsIndex)i));
                    break;
                }
                else
                {
                    char purefileName[_MAX_LFN];                    
                    char *pureExtendName = strchr(name, '.');
                    memset(purefileName, 0x0, _MAX_LFN);
                    strncpy(purefileName, name, strlen(name) - strlen(pureExtendName));
                    reTryCounter++;
                    sprintf(fileNameTmp, "%s_%02d%s", purefileName, reTryCounter, pureExtendName);                    
                    sysprintf(" -> fatfsAutoSaveToFile %s%s ERROR: retry %d\r\n", FatFsGetRootStr((FatfsIndex)i), fileNameTmp, reTryCounter); 
                    targetFile = fileNameTmp;
                }
            }
            if(reTryCounter == AUTO_SAVE_RETRY_TIMES)
            {
                autoFormatCounter1++;
                if((autoFormatCounter1%3) == 2)
                {
                    sysprintf("[ERROR] fatfsAutoSaveToFile [%s]  Retry (%d) ERROR!!, go format\n", FatFsGetRootStr((FatfsIndex)i), autoFormatCounter1);   
                    if(FileAgentFatFsFormat(FatFsGetRootStr((FatfsIndex)i)))
                    {     
                        sysprintf("[ERROR] fatfsAutoSaveToFile [%s]  f_mkfs OK!!\n", FatFsGetRootStr((FatfsIndex)i));           
                    }
                    else
                    {
                        sysprintf("[ERROR] fatfsAutoSaveToFile [%s]  f_mkfs error!!\n", FatFsGetRootStr((FatfsIndex)i));
                    }
                }
                else
                {
                    sysprintf("[ERROR] fatfsAutoSaveToFile [%s]  Retry (%d) ERROR, ignore format!!\n", FatFsGetRootStr((FatfsIndex)i), autoFormatCounter1);  
                }
            }
        }
    }
    return reVal;
}  


static FileAgentReturn fatfsAutoGetFromFile(char* name, uint8_t** buff, size_t* buffLen, BOOL checkMode)
{
    //sysprintf(" -> fatfsAutoGetFromFile %s (addType:%d, buffLen:%d): [%s]\r\n", name, addType, buffLen, buff);
    int i;
    FileAgentReturn reVal = FILE_AGENT_RETURN_ERROR;
    for(i = 0; i < FatFsGetCounter(); i++)
    {
        if(FatFsGetExistFlag((FatfsIndex)i))
        {
            if(fatfsGetFromFile(FatFsGetRootStr((FatfsIndex)i), name, buff, buffLen, checkMode))
            {
                reVal = reVal|fatfsGetReturnValueByDir(FatFsGetRootStr((FatfsIndex)i));;
                sysprintf(" -> fatfsAutoGetFromFile %s Get it , break...\r\n", name);
                break;
            }
        }
    }
    return reVal;
} 

static FileAgentReturn fatfsAutoDelFile(char* name)
{
    //sysprintf(" -> fatfsAutoDelFile %s (addType:%d, buffLen:%d): [%s]\r\n", name, addType, buffLen, buff);
    int i;
    FileAgentReturn reVal = FILE_AGENT_RETURN_ERROR;
    for(i = 0; i < FatFsGetCounter(); i++)
    {
        if(FatFsGetExistFlag((FatfsIndex)i))
        {
            char fileName[_MAX_LFN];
            FRESULT reval;
            sprintf(fileName, "%s%s", FatFsGetRootStr((FatfsIndex)i), name); 
            reval = f_unlink(fileName);
            if(reval == FR_OK) 
            {
                reVal = reVal|fatfsGetReturnValueByDir(FatFsGetRootStr((FatfsIndex)i));
                sysprintf("\r\n !![Information FileAgent]!! fatfsAutoDelFile %s : Success\r\n", fileName);
                //break;
            }
            else
            {
                sysprintf("\r\n !![Information FileAgent]!! fatfsAutoDelFile %s : Error (errorcode: %d)\r\n", fileName, reval);
            }
        }
    }
    return reVal;
} 

static FileAgentReturn fatfsAutoGetList(char* extensionName, char* excludeFileName, fileAgentFatfsListCallback callback, void* para1, void* para2, void* para3, void* para4)
{
    //sysprintf(" -> fatfsAutoGetList %s (addType:%d, buffLen:%d): [%s]\r\n", name, addType, buffLen, buff);
    int i;
    FileAgentReturn reVal = FILE_AGENT_RETURN_ERROR;
    for(i = 0; i < FatFsGetCounter(); i++)
    {
        if(FatFsGetExistFlag((FatfsIndex)i))
        {
            if(FileAgentGetFatfsList(FatFsGetRootStr((FatfsIndex)i), extensionName, excludeFileName, callback, para1, para2, para3, para4) )
            {
                reVal = reVal|fatfsGetReturnValueByDir(FatFsGetRootStr((FatfsIndex)i));;
                //break;
            }
        }
    }
    return reVal;
} 

static BOOL yaffs2SaveToFile(char* fileName, FileAgentAddType addType, uint8_t* buff, size_t buffLen, BOOL checkMode)
{    
    uint32_t bytesWritten = 0;
    int outh;
    xSemaphoreTake(xYaffs2ReadWriteMutex, portMAX_DELAY); 
    //sysprintf("yaffs2SaveToFile %s (addType:%d, buffLen:%d): [%s]\r\n", fileName, addType, buffLen, buff);
    switch(addType)
    {
        case FILE_AGENT_ADD_DATA_TYPE_OVERWRITE:
            outh = yaffs_open(fileName, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);            
            break;
        case FILE_AGENT_ADD_DATA_TYPE_APPEND:
            outh = yaffs_open(fileName, O_CREAT | O_RDWR | O_APPEND, S_IREAD | S_IWRITE);  
            break;    
    }    

    if (outh < 0) 
    {
        sysprintf("yaffs2SaveToFile %s : open error %d, %s\n", fileName, outh, Yaffs2ErrorStr());
        xSemaphoreGive(xYaffs2ReadWriteMutex); 
        return FILE_AGENT_RETURN_ERROR;
    }
    
    switch(addType)
    {
        case FILE_AGENT_ADD_DATA_TYPE_OVERWRITE:                   
            break;
        case FILE_AGENT_ADD_DATA_TYPE_APPEND:
            //yaffs_lseek(outh, 0, SEEK_END);         
            break;    
    }
    
    if(checkMode)
    {
        faHeader.Len = buffLen;    
        bytesWritten = yaffs_write(outh, &faHeader, sizeof(faHeader)); 
        if(bytesWritten != sizeof(faHeader)) /*EOF or Error*/
        {
            sysprintf("yaffs2SaveToFile %s : write faHeader error (%d)\r\n", fileName, bytesWritten);
            yaffs_close(outh); 
            xSemaphoreGive(xYaffs2ReadWriteMutex); 
            return FILE_AGENT_RETURN_ERROR;
        }
    }
    
    bytesWritten = yaffs_write(outh, buff, buffLen);  
    if(bytesWritten != buffLen) /*EOF or Error*/
    {
        sysprintf("yaffs2SaveToFile %s : write error\r\n", fileName);
        yaffs_close(outh);
        xSemaphoreGive(xYaffs2ReadWriteMutex); 
        return FILE_AGENT_RETURN_ERROR;
    }
    if(checkMode)
    {
        faEnd.checksum = getChecksum(buff, buffLen, "fatfsSaveToFile");  
        bytesWritten = yaffs_write(outh, &faEnd, sizeof(faEnd)); 
        if(bytesWritten != sizeof(faEnd)) /*EOF or Error*/
        {
            sysprintf("yaffs2SaveToFile %s : write faEnd error (%d)\r\n", fileName, bytesWritten);
            yaffs_close(outh); 
            xSemaphoreGive(xYaffs2ReadWriteMutex); 
            return FILE_AGENT_RETURN_ERROR;
        }
    }
    //sysprintf("yaffs2SaveToFile %s : Success[%s]\r\n", fileName, buff);
    //Yaffs2ListFileEx("/");
    yaffs_close(outh);
    sysprintf("[ok]");
    xSemaphoreGive(xYaffs2ReadWriteMutex); 
    return FILE_AGENT_RETURN_OK_YAFFS2;
}   


static FileAgentReturn yaffs2GetFromFile(char* fileName, uint8_t** buff, size_t* buffLen, BOOL checkMode)
{
    int outh;
    struct yaffs_stat stat;
    xSemaphoreTake(xYaffs2ReadWriteMutex, portMAX_DELAY); 
    outh= yaffs_open(fileName, O_RDWR, 0);
    if (outh < 0) 
    {
        sysprintf("yaffs2GetFromFile %s : open error %d, %s\n", fileName, outh, Yaffs2ErrorStr());
        xSemaphoreGive(xYaffs2ReadWriteMutex); 
        return FILE_AGENT_RETURN_ERROR;
    }
    if(yaffs_fstat(outh, &stat) == 0)
    {
        *buffLen = stat.st_size;
        if(*buffLen > 0)
        {
            *buff =  pvPortMalloc(*buffLen);   
            if(buff != NULL)
            {
                int readLen = yaffs_read(outh, *buff, *buffLen);
                yaffs_close(outh);
                if(*buffLen == readLen)
                {
                }
                else
                {
                    sysprintf("yaffs2GetFromFile %s : yaffs_read error (%d, %d)\r\n", fileName, *buffLen, readLen);
                    xSemaphoreGive(xYaffs2ReadWriteMutex); 
                    return FILE_AGENT_RETURN_ERROR;
                }
            }
            else
            {
                sysprintf("yaffs2GetFromFile %s : pvPortMalloc error\r\n", fileName);
                xSemaphoreGive(xYaffs2ReadWriteMutex); 
                return FILE_AGENT_RETURN_ERROR;
            }
        }
        else
        {
            sysprintf("yaffs2GetFromFile %s : stat.st_size error\r\n", fileName);
            xSemaphoreGive(xYaffs2ReadWriteMutex); 
            return FILE_AGENT_RETURN_ERROR;
        }
    }
    else
    {
        sysprintf("yaffs2GetFromFile %s : yaffs_fstat(outh, &stat) error\r\n", fileName);
        xSemaphoreGive(xYaffs2ReadWriteMutex); 
        return FILE_AGENT_RETURN_ERROR;
    }
    
    if(checkMode)
    {
        uint8_t* buffTmp = *buff;
        size_t buffLenTmp = *buffLen;
        uint8_t* targetData;
        size_t targetDataLen;
        if(FileAgentParserAutoData(buffTmp, buffLenTmp, &targetData, &targetDataLen))
        {
            //sysprintf("[ INFO ]processRead parser (len = %d):[%s]...\r\n", targetDataLen, targetData); 
            *buff =  pvPortMalloc(targetDataLen);   
            if(buff != NULL)
            {
                *buffLen = targetDataLen;
                memcpy(*buff, targetData, targetDataLen);
                vPortFree(buffTmp);   
            }
            else
            {
                sysprintf("fatfsGetFromFile %s : pvPortMalloc error\r\n", fileName);
                vPortFree(buffTmp);
                xSemaphoreGive(xYaffs2ReadWriteMutex); 
                return FILE_AGENT_RETURN_ERROR;
            }
           
        }
        else
        {
            sysprintf("yaffs2GetFromFile %s : parser error\r\n", fileName);
            xSemaphoreGive(xYaffs2ReadWriteMutex); 
            return FILE_AGENT_RETURN_ERROR;
        }
    }        
    //sysprintf("yaffs2GetFromFile %s : Success\r\n", fileName);
    sysprintf("[ok]");
    xSemaphoreGive(xYaffs2ReadWriteMutex); 
    return FILE_AGENT_RETURN_OK_YAFFS2;
}

static FileAgentReturn processWrite(FileAgentDataItem item)
{
    FileAgentReturn saveReval = FILE_AGENT_RETURN_ERROR;
    //sysprintf("[1:%s]", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
    //xSemaphoreTake(xReadWriteMutex, portMAX_DELAY); 
    switch(item.storageType)
    {
        case FILE_AGENT_STORAGE_TYPE_AUTO:
            //if(yaffs2SaveToFile(item.name, item.addType, item.data, item.dataLen, TRUE))
            //    saveReval = TRUE;
            saveReval = fatfsAutoSaveToFile(item.name, item.addType, item.data, item.dataLen, item.checkMode);
            break;
            
        case FILE_AGENT_STORAGE_TYPE_YAFFS2:
            saveReval = yaffs2SaveToFile(item.name, item.addType, item.data, item.dataLen, FALSE);
            break;
        
        case FILE_AGENT_STORAGE_TYPE_FATFS:            
            saveReval = fatfsSaveToFile(item.dir, item.name, item.addType, item.data, item.dataLen, FALSE);
            
            break;
    }    
    //xSemaphoreGive(xReadWriteMutex); 
    if(saveReval != FILE_AGENT_RETURN_ERROR)
    {
        error++;
    }
    if(item.dataNeedFreeFlag)
        vPortFree(item.data);
    
    return saveReval;
}

static FileAgentReturn processRead(StorageType storageType, char* dir, char* name, uint8_t** buff, size_t* buffLen, BOOL checkMode)
{
    FileAgentReturn saveReval = FILE_AGENT_RETURN_ERROR;
    //xSemaphoreTake(xReadWriteMutex, portMAX_DELAY); 
    switch(storageType)
    {
        case FILE_AGENT_STORAGE_TYPE_AUTO:    
            //saveReval = yaffs2GetFromFile(name, buff, buffLen, TRUE);
            //if(saveReval == FILE_AGENT_RETURN_ERROR)
            {
                saveReval = fatfsAutoGetFromFile(name, buff, buffLen, checkMode);
            }
            break;
        case FILE_AGENT_STORAGE_TYPE_YAFFS2:
            saveReval = yaffs2GetFromFile(name, buff, buffLen, FALSE); 
            break;
        case FILE_AGENT_STORAGE_TYPE_FATFS:
            saveReval = fatfsGetFromFile(dir, name, buff, buffLen, FALSE);            
            break;
    }    
    //xSemaphoreGive(xReadWriteMutex); 
    if(saveReval == FILE_AGENT_RETURN_ERROR)
    {
        error++;
    }
    return saveReval;
}

static void vFileAgentTask( void *pvParameters )
{
    
    vTaskDelay(1000/portTICK_RATE_MS); 
    sysprintf("vFileAgentTask Going...\r\n"); 

    for(;;)
    {        
        fileAgentPowerStatus = TRUE;
        BaseType_t reval = xSemaphoreTake(xSemaphore, threadWaitTime); 
        //sysprintf("\r\n [INFO]vFileAgentTask Go [%d:%d]...\r\n", endIndex, headerIndex);
        
        if(fileAgentPowerStatusFlag == FALSE)
        {
            fileAgentPowerStatus = FALSE;
            while(headerIndex != endIndex)   
            {            
                sysprintf("\r\n [INFO]vFileAgentTask Process Start [%d:%d]...\r\n", endIndex, headerIndex);
                if(processWrite(fileAgentDataItem[endIndex]) != FILE_AGENT_RETURN_ERROR)
                {
                    //xSemaphoreTake(xReadWriteMutex, portMAX_DELAY);
                    //vTaskDelay(200/portTICK_RATE_MS);             
                    //xSemaphoreGive(xReadWriteMutex);             
                    endIndex++;
                    if(endIndex == FILE_DATA_QUEUE_SIZE)
                    {
                        endIndex = 0;
                    }
                    sysprintf("\r\n [INFO]vFileAgentTask Process OK [%d:%d]...\r\n", endIndex, headerIndex);
                }
                else
                {
                    sysprintf("\r\n [INFO]vFileAgentTask Process ERROR [%d:%d]...\r\n", endIndex, headerIndex);
                }
            }   
        }
        
        //sysprintf("\r\n [INFO]vFileAgentTask End [%d:%d]...\r\n", endIndex, headerIndex);        
    }
}

#if (ENABLE_BURNIN_TESTER)
time_t GetCurrentUTCTime(void);
static void vSDCardTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL writeLogLoop = FALSE;
    terninalPrintf("vSDCardTestTask Going...\r\n");
    memset(burninErrMsgBuffer, 0x00, BURNIN_ERRMSG_BUFFER_MAX);
    
    while (TRUE)
    {
        if (GetDeviceID() != 0)
        {
            sprintf(burninErrLogFilename, "Burnin_%08d_Error.log", GetDeviceID());
            sprintf(burninErrLogFilepath, "0:\\%08d", GetDeviceID());
            break;
        }
        vTaskDelay(200 / portTICK_RATE_MS);
    }
    
    //Test for Multi-Level Path
    //RTC_TIME_DATA_T pt;
    //RTC_Read(RTC_CURRENT_TIME, &pt);
    //char *reportBuffer = BuildBurninTestReport(&pt);
    //FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, "0:\\Test1\\Test2\\Test3", "Test4.log", (uint8_t *)reportBuffer, strlen(reportBuffer), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE);
    
    while (TRUE)
    {
        currentTime = GetCurrentUTCTime();
        if (((currentTime - lastTime) > BURNIN_ERROR_LOG_INTERVAL) || (GetPrepareStopBurninFlag() && GetBurninTerminatedFlag()))
        {
            lastTime = currentTime;
            writeLogLoop = TRUE;
        }
        if (!writeLogLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        
        writeLogLoop = FALSE;
        if (burninErrMsgLength > 0)
        {
            xSemaphoreTake(xBurninErrorMessageMutex, portMAX_DELAY);
            FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, burninErrLogFilepath, burninErrLogFilename, (uint8_t *)burninErrMsgBuffer, burninErrMsgLength, FILE_AGENT_ADD_DATA_TYPE_APPEND, FALSE, TRUE, FALSE);
            memset(burninErrMsgBuffer, 0x00, burninErrMsgLength);
            burninErrMsgLength = 0;
            xSemaphoreGive(xBurninErrorMessageMutex);
        }
        if (GetPrepareStopBurninFlag() && GetBurninTerminatedFlag())
        {
            terninalPrintf("vSDCardTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
    }
}
#endif


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL FileAgentInit(void)
{
    sysprintf("FileAgentInit [0x%02x, 0x%02x] [0x%02x, 0x%02x]!! \n", faHeader.value[0], faHeader.value[1], faEnd.value[0], faEnd.value[1] );
    PowerRegCallback(&fileAgentPowerCallabck);
    
    xSemaphore = xSemaphoreCreateBinary();  
    xAddDataMutex = xSemaphoreCreateMutex(); 
    xFatfsReadWriteMutex = xSemaphoreCreateMutex(); 
    xYaffs2ReadWriteMutex = xSemaphoreCreateMutex(); 
    //xTaskCreate( vFileAgentTask, "vFileAgentTask", 1024*10, NULL, FILE_AGENT_THREAD_PROI, NULL ); 
    
  
    initFlag = TRUE;
    //xTaskCreate( vFatFsDrvAgentTestTask, "A", 1024*10, NULL, FILE_AGENT_THREAD_PROI+1, NULL ); 
    //xTaskCreate( vFatFsDrvAgentTestTask, "B", 1024*10, NULL, FILE_AGENT_THREAD_PROI+1, NULL ); 
    //xTaskCreate( vFatFsDrvAgentTestTask, "C", 1024*10, NULL, FILE_AGENT_THREAD_PROI+1, NULL ); 
    //xTaskCreate( vFatFsDrvAgentTestTask, "D", 1024*10, NULL, FILE_AGENT_THREAD_PROI+1, NULL ); 
    //xTaskCreate( vFatFsDrvAgentTestTask, "E", 1024*10, NULL, FILE_AGENT_THREAD_PROI+1, NULL );
    //xTaskCreate( vFatFsDrvAgentTestTask, "F", 1024*10, NULL, FILE_AGENT_THREAD_PROI+1, NULL );
#if (ENABLE_BURNIN_TESTER)
    if (EnabledBurninTestMode())
    {
        //memset(lastBurninLogFilename, 0x00, sizeof(lastBurninLogFilename));
        xBurninErrorMessageMutex = xSemaphoreCreateMutex();
        xTaskCreate(vSDCardTestTask, "vSDCardTestTask", 1024*5, NULL, SD_CARD_TEST_THREAD_PROI, NULL);
    }
#endif
    
    return TRUE;
}

BOOL FileAgentAddData(StorageType storageType, char* dir, char* name, uint8_t* data, int dataLen, FileAgentAddType addType, BOOL dataNeedFreeFlag, BOOL blockFlag, BOOL checkMode)
{    
    if(initFlag == FALSE)
    {
        sysprintf("\r\nFileAgentAddData [%s] : ERROR (initFlag == FALSE)...\r\n", name);
        return FALSE;
    }
    if((storageType == FILE_AGENT_STORAGE_TYPE_FATFS) && (dir == NULL))
    {
        sysprintf("\r\nFileAgentAddData [%s] : ERROR ((addType == FILE_AGENT_STORAGE_TYPE_FATFS) && (dir == NULL))...\r\n", name);
        return FALSE;
    }
    
    if((checkMode == TRUE) && (addType == FILE_AGENT_ADD_DATA_TYPE_APPEND))
    {
        sysprintf("\r\nFileAgentAddData [%s] : ERROR (checkMode == TRUE) && (addType == FILE_AGENT_ADD_DATA_TYPE_APPEND)...\r\n", name);
        return FALSE;
    }
    
    if(blockFlag)
    {
        FileAgentDataItem tmpItem;
        sysprintf("FileAgentAddData [%s%s] : BLOCK !! \n", dir, name);        
        tmpItem.storageType = storageType;
        strcpy(tmpItem.dir, (const char*)dir);
        strcpy(tmpItem.name, (const char*)name);
        tmpItem.data = data;
        tmpItem.dataLen = dataLen;
        tmpItem.addType= addType;
        tmpItem.dataNeedFreeFlag = dataNeedFreeFlag;
        tmpItem.checkMode = checkMode;
        
        processWrite(tmpItem);  
    }
    else
    {        
        
        xSemaphoreTake(xAddDataMutex, portMAX_DELAY); 
        fileAgentPowerStatus = FALSE;
        sysprintf("\r\nFileAgentAddData [%s] : non BLOCK !! start [%d:%d]...\r\n", name, endIndex, headerIndex);
        fileAgentDataItem[headerIndex].storageType = storageType;
        strcpy(fileAgentDataItem[headerIndex].dir, (const char*)dir);
        strcpy(fileAgentDataItem[headerIndex].name, (const char*)name);
        fileAgentDataItem[headerIndex].data = data;
        fileAgentDataItem[headerIndex].dataLen = dataLen;
        fileAgentDataItem[headerIndex].addType= addType;
        fileAgentDataItem[headerIndex].dataNeedFreeFlag = dataNeedFreeFlag;
        fileAgentDataItem[headerIndex].checkMode = checkMode;
        
        headerIndex++;
        if(headerIndex == FILE_DATA_QUEUE_SIZE)
        {
            headerIndex = 0;
        }

        if(headerIndex == endIndex)
        {
            endIndex++;
        }
        //sysprintf("\r\nFileAgentAddData [%s] : non BLOCK !! end [%d:%d]...\r\n", name, endIndex, headerIndex);
        xSemaphoreGive(xAddDataMutex); 
        
        
        xSemaphoreGive(xSemaphore); 
        
    }
    return TRUE;
}
FileAgentReturn FileAgentGetData(StorageType storageType, char* dir, char* name, uint8_t** data, size_t* buffLen, BOOL* needFree, BOOL checkMode)
{
    FileAgentReturn reVal = processRead(storageType, dir, name, data, buffLen, checkMode);
    if(reVal != FILE_AGENT_RETURN_ERROR)
    {
         *needFree = TRUE;
        
    }
    else
    {
        *needFree = FALSE;
    }
    return reVal;
}

BOOL FileAgentParserAutoData(uint8_t* srcData, size_t srcDataLen, uint8_t** targetData, size_t* targetDataLen)
{
    FAHeader* pFAHeader = (FAHeader*)srcData;  
    
    //sysprintf("\r\FileAgentParserAutoData[0x%02x, 0x%02x] len = %d:...\r\n", pFAHeader->value[0], pFAHeader->value[1], pFAHeader->Len);
    if((pFAHeader->value[0] == FA_HEADER_VALUE) && 
        (pFAHeader->value[1] == FA_HEADER_VALUE2) && 
        (pFAHeader->Len == (srcDataLen-sizeof(FAHeader) - sizeof(FAEnd))) 
       )
    {
            uint16_t checkSum = getChecksum(srcData + sizeof(FAHeader), pFAHeader->Len, "<parser>") ; 
            FAEnd* pFAEnd = (FAEnd*)(srcData + srcDataLen - sizeof(FAEnd));
            if((checkSum == pFAEnd->checksum)  &&
                (pFAEnd->value[0] == FA_END_VALUE) && 
                (pFAEnd->value[1] == FA_END_VALUE2)  )
            {
                *targetData = srcData + sizeof(FAHeader);
                *targetDataLen = pFAHeader->Len;
                srcData[pFAHeader->Len + sizeof(FAHeader)] = 0x0;
                //sysprintf("\r\nFileAgentParserAutoData OK [%s].\r\n", *targetData);
                sysprintf("\r\nFileAgentParserAutoData OK .\r\n");
                return TRUE;
            }
            else
            {
                sysprintf("\r\nFileAgentParserAutoData End ERROR [0x%02x, 0x%02x] checkSum = %d (%d)...\r\n", pFAEnd->value[0], pFAEnd->value[1], pFAEnd->checksum, checkSum);
            }
    }
    else
    {
        sysprintf("\r\nFileAgentParserAutoData Header ERROR [0x%02x, 0x%02x] len = %d (%d)...\r\n", pFAHeader->value[0], pFAHeader->value[1], pFAHeader->Len, (srcDataLen-sizeof(FAHeader) - sizeof(FAEnd)));
    }
    return FALSE;
}
FileAgentReturn FileAgentDelFile(StorageType storageType, char* dir, char* name)
{
    FileAgentReturn delReval = FILE_AGENT_RETURN_ERROR;
    char fileName[_MAX_LFN];
    
    sprintf(fileName, "%s%s", dir, name);
    switch(storageType)
    {
        case FILE_AGENT_STORAGE_TYPE_AUTO:  
            xSemaphoreTake(xFatfsReadWriteMutex, portMAX_DELAY);
            delReval = fatfsAutoDelFile(name);        
            xSemaphoreGive(xFatfsReadWriteMutex);  
            break;
        case FILE_AGENT_STORAGE_TYPE_YAFFS2:
            xSemaphoreTake(xYaffs2ReadWriteMutex, portMAX_DELAY); 
            if(yaffs_unlink(name) < 0)
            {
            }
            else
            {
                delReval = FILE_AGENT_RETURN_OK_YAFFS2; 
            }
            xSemaphoreGive(xYaffs2ReadWriteMutex); 
            break;
        case FILE_AGENT_STORAGE_TYPE_FATFS:
        {
            FRESULT res;
            xSemaphoreTake(xFatfsReadWriteMutex, portMAX_DELAY);
            res = f_unlink(fileName);    
            if(res == FR_OK)   
            {
                delReval = fatfsGetReturnValueByDir(dir);
            }
            else
            {
                sysprintf("\r\n FileAgentDelFile f_unlink (%s) ERROR ...\r\n", fileName);
            }
            xSemaphoreGive(xFatfsReadWriteMutex);    
        }            
            break;
    }    
    //xSemaphoreGive(xReadWriteMutex); 
    if(delReval == FILE_AGENT_RETURN_ERROR)
    {
        error++;
    }
    return delReval;
}
BOOL FileAgentGetList(StorageType storageType, char* dir, char* extensionName, char* excludeFileName, fileAgentFatfsListCallback callback, void* para1, void* para2, void* para3, void* para4)
{
    BOOL getReval = TRUE;
    switch(storageType)
    {
        case FILE_AGENT_STORAGE_TYPE_AUTO:   
            //xSemaphoreTake(xFatfsReadWriteMutex, portMAX_DELAY);          
            if(fatfsAutoGetList( extensionName, excludeFileName, callback, para1, para2, para3, para4) == FILE_AGENT_RETURN_ERROR)
            {
                getReval = FALSE;
            }
            //xSemaphoreGive(xFatfsReadWriteMutex);  
            break;
        case FILE_AGENT_STORAGE_TYPE_YAFFS2:
            //xSemaphoreTake(xYaffs2ReadWriteMutex, portMAX_DELAY); 
            //xSemaphoreGive(xYaffs2ReadWriteMutex); 
            break;
        case FILE_AGENT_STORAGE_TYPE_FATFS:
        {
            //xSemaphoreTake(xFatfsReadWriteMutex, portMAX_DELAY);     
       
            if(FALSE == FileAgentGetFatfsList(dir, extensionName, excludeFileName, callback, para1, para2, para3, para4) )   
            {
                getReval = FALSE;
            }
            //xSemaphoreGive(xFatfsReadWriteMutex);    
        }            
            break;
    }    
    //xSemaphoreGive(xReadWriteMutex); 
    if(getReval == FALSE)
    {
        error++;
    }
    return getReval;
}
BOOL FileAgentGetFatfsList(char* dir, char* extensionName, char* excludeFileName, fileAgentFatfsListCallback callback, void* para1, void* para2, void* para3, void* para4)
{
    //BOOL reVal = FALSE;
    FRESULT fr;     /* Return value */
    DIR dj;         /* Directory search object */
    FILINFO fno;    /* File information */
#if(USER_NEW_FATFS)
#else
    TCHAR lfname[_MAX_LFN]; 
#endif    
    TCHAR nameTmp[_MAX_LFN]; 
    int warningFileNum = 0;
    BOOL reVal = TRUE;    
    sysprintf("\r\n >==>  FileAgentGetFatfsList start [%s] [%s] <==<\r\n", dir, extensionName);
    if(dir == NULL)
    {
        return reVal;
    }    
#if(USER_NEW_FATFS)
#else    
    fno.lfname = lfname; 
    fno.lfsize = _MAX_LFN - 1;
#endif    
    fr = f_findfirst(&dj, &fno, dir, extensionName);  /* Start to search for photo files */
    while (fr == FR_OK && fno.fname[0]) 
    {   /* Repeat while an item is found */
        #if(USER_NEW_FATFS)
        strcpy(nameTmp, fno.fname);
        //sysprintf(" FatfsList > get File Name(long)[%d]:[%s] [%s] \r\n", index, nameTmp, fno.lfname);            
        
        #else
        if(strlen(fno.lfname) != 0)
        {            
            strcpy(nameTmp, fno.lfname);
            //sysprintf(" FatfsList > get File Name(long)[%d]:[%s] [%s] \r\n", index, nameTmp, fno.lfname);            
        }
        else
        {
            strcpy(nameTmp, fno.fname);
            //sysprintf(" FatfsList > get File Name(short)[%d]:[%s] [%s] \r\n", index, nameTmp, fno.fname);
        }
        #endif
        if((fno.fsize == 0xFFFFFFFF) || (fno.fsize == 0))
        {
            FRESULT reval;
            char fileName[_MAX_LFN];        
            sprintf(fileName, "%s%s", dir, nameTmp);
            reval = f_unlink(fileName);
            warningFileNum++;
            sysprintf("    >-del->  [%s] [%s] : [%s] return: %d <--<\r\n", dir, nameTmp, fileName, reval);
            
        }
        else
        {
            if(callback != NULL)
            {
                if(excludeFileName != NULL)
                {
                    if(strcmp(excludeFileName, nameTmp) != 0)
                    {
                        sysprintf("    >-->  [%s] [%s] (len = %d) <--<\r\n", dir, nameTmp, fno.fsize);
                        if(callback(dir, nameTmp, fno.fsize, para1, para2, para3, para4) == FALSE)
                        {
                            reVal = FALSE;
                        }
                    }
                }
                else
                {
                    sysprintf("    >-->  [%s] [%s] (len = %d) <--<\r\n", dir, nameTmp, fno.fsize);
                    if(callback(dir, nameTmp, fno.fsize, para1, para2, para3, para4) == FALSE)
                    {
                        reVal = FALSE;
                    }
                }
               
            }
        }
        if(reVal == FALSE)
        {
            sysprintf("    FileAgentGetFatfsList ERROR Break <--<\r\n", dir, nameTmp, fno.fsize);
            break;
        }
        fr = f_findnext(&dj, &fno);               /* Search for next item */
    }
    sysprintf(" >==>  FileAgentGetFatfsList exit  [%s] [%s] (warningFileNum = %d) <==<\r\n", dir, extensionName, warningFileNum);
    f_closedir(&dj);
    
    if(warningFileNum >= 128)
    {        
        autoFormatCounter2++;
        sysprintf("[ERROR] FileAgentGetFatfsList [%s]  Start f_mkfs!!\n", dir);
        if(FileAgentFatFsFormat(dir))
        {     
            sysprintf("[ERROR] FileAgentGetFatfsList [%s]  f_mkfs OK!!\n", dir);           
        }
        else
        {
            sysprintf("[ERROR] FileAgentGetFatfsList [%s]  f_mkfs error!!\n", dir);
        }
    }
    return reVal;
}
static BOOL listCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4)
{
    //sysprintf(" listCallback > [%s][%s], Len = %d (0x%x)\r\n", dir, filename, fileLen, fileLen);
    return TRUE;
}
static BOOL delCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4)
{
    char fileName[_MAX_LFN];
    sysprintf(" delCallback > [%s][%s], Len = %d\r\n", dir, filename, fileLen);
    sprintf(fileName, "%s%s", dir, filename);
    f_unlink(fileName);
    return TRUE;
}
BOOL FileAgentFatfsListFile (char* dir, char* extensionName)
{
    //sysprintf("\r\n ===[ INFO ]=== FileAgentFatfsListFile [%s][%s] Start ===[ INFO ]=== \r\n", dir, extensionName);    
    FileAgentGetFatfsList(dir, extensionName, NULL, listCallback, NULL, NULL, NULL, NULL);
    FatfsGetDiskUseage(dir);
    //sysprintf("\r\n ===[ INFO ]=== FileAgentFatfsListFile [%s][%s] End ===[ INFO ]=== \r\n", dir, extensionName);
    return TRUE;
}
BOOL FileAgentFatfsDeleteFile (char* dir, char* extensionName)
{
    //sysprintf("\r\n ===[ INFO ]=== FileAgentFatfsDeleteFile [%s][%s] Start ===[ INFO ]=== \r\n", dir, extensionName);
    FileAgentGetFatfsList(dir, extensionName, NULL, delCallback, NULL, NULL, NULL, NULL);
   // sysprintf("\r\n ===[ INFO ]=== FileAgentFatfsDeleteFile [%s][%s] End ===[ INFO ]=== \r\n", dir, extensionName);
    return TRUE;
}

BOOL FileAgentFatFsFormat(char* dir)
{
    BOOL reval;
    char message[128]; 
    xSemaphoreTake(xFatfsReadWriteMutex, portMAX_DELAY); 
    if(FatFsFormat(dir))
    {     
        sysprintf("\r\n [OK] FileAgentFatFsFormat [%s] !!\n", dir);
        sprintf(message, "f_mkfs_%s_OK", dir);
        DataProcessSendStatusData(0, message);
        reval = TRUE; 
    }
    else
    {
        sysprintf("\r\n [ERROR] FileAgentFatFsFormat [%s] !!\n", dir);
        sprintf(message, "f_mkfs_%s_ERROR", dir);
        DataProcessSendStatusData(0, message);
        reval = FALSE;
    }
    xSemaphoreGive(xFatfsReadWriteMutex); 
    return reval;
}

int FileAgentGetFatFsAutoFormatCounter(void)
{
    return (autoFormatCounter2 * 1000) + autoFormatCounter1;
}

#if (ENABLE_BURNIN_TESTER)
#define TIMESTAMP_STRING_LENGTH     22
BOOL AppendBurninErrorLog(char *msgBuffer, uint32_t msgLength)
{
    if ((burninErrMsgLength + msgLength + TIMESTAMP_STRING_LENGTH) > BURNIN_ERRMSG_BUFFER_MAX) {
        return FALSE;
    }
    RTC_TIME_DATA_T pt;
    char timeStamp[32];
    if (E_RTC_SUCCESS != RTC_Read(RTC_CURRENT_TIME, &pt)) {
        return FALSE;
    }
    
    //terninalPrintf("ABurninErrorLog Take before\r\n");
    
    xSemaphoreTake(xBurninErrorMessageMutex, portMAX_DELAY);
    
    //terninalPrintf("ABurninErrorLog Take after\r\n");
    
    sprintf(timeStamp, "%04d/%02d/%02d% 02d:%02d:%02d - ", pt.u32Year, pt.u32cMonth, pt.u32cDay, pt.u32cHour, pt.u32cMinute, pt.u32cSecond);
    memcpy(&burninErrMsgBuffer[burninErrMsgLength], timeStamp, TIMESTAMP_STRING_LENGTH);
    burninErrMsgLength += TIMESTAMP_STRING_LENGTH;
    memcpy(&burninErrMsgBuffer[burninErrMsgLength], msgBuffer, msgLength);
    burninErrMsgLength += msgLength;
    xSemaphoreGive(xBurninErrorMessageMutex);
    return TRUE;
}
#endif


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


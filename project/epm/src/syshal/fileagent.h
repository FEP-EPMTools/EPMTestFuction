/**************************************************************************//**
* @file     fileagent.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __FILE_AGENT_H__
#define __FILE_AGENT_H__

#include "nuc970.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "ff.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
 

typedef enum{
    FILE_AGENT_ADD_DATA_TYPE_OVERWRITE = 0x00,
    FILE_AGENT_ADD_DATA_TYPE_APPEND    = 0x01
}FileAgentAddType;


typedef enum{
    FILE_AGENT_STORAGE_TYPE_AUTO = 0x00,
    FILE_AGENT_STORAGE_TYPE_YAFFS2 = 0x01,
    FILE_AGENT_STORAGE_TYPE_FATFS = 0x02
}StorageType;  

#if(0)
typedef enum{
    FILE_AGENT_RETURN_ERROR = 0x00,
    FILE_AGENT_RETURN_OK_YAFFS2 = 0x01,
    FILE_AGENT_RETURN_OK_FATFS_0 = 0x02,
    FILE_AGENT_RETURN_OK_FATFS_1 = 0x04,
    FILE_AGENT_RETURN_OK_FATFS_2 = 0x08
}FileAgentReturn;
#else
#define FILE_AGENT_RETURN_ERROR         0x00
#define FILE_AGENT_RETURN_OK_YAFFS2     0x01
#define FILE_AGENT_RETURN_OK_FATFS_0    0x02
#define FILE_AGENT_RETURN_OK_FATFS_1    0x04
#define FILE_AGENT_RETURN_OK_FATFS_2    0x08
typedef int FileAgentReturn;
#endif


typedef struct
{
    StorageType         storageType; 
    char                dir[_MAX_LFN]; 
    char                name[_MAX_LFN];
    uint8_t*            data;
    int                 dataLen;
    BOOL                dataNeedFreeFlag;
    FileAgentAddType    addType;
    BOOL                checkMode;
}FileAgentDataItem;

typedef struct
{
    char    name[_MAX_LFN];
    BOOL    existFlag;
}FileAgentTargetFileName; 

typedef BOOL(*fileAgentFatfsListCallback)(char* dir, char* extensionName, int fileLen, void* para1, void* para2, void* para3, void* para4);
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL FileAgentInit(void);
BOOL FileAgentAddData(StorageType storageType,char* dir, char* name, uint8_t* data, int dataLen, FileAgentAddType addType, BOOL dataNeedFreeFlag, BOOL blockFlag, BOOL checkMode);
FileAgentReturn FileAgentGetData(StorageType storageType,char* dir, char* name, uint8_t** data, size_t* buffLen, BOOL* needFree, BOOL checkMode);
BOOL FileAgentParserAutoData(uint8_t* srcData, size_t srcDataLen, uint8_t** targetData, size_t* targetDataLen);
FileAgentReturn FileAgentDelFile(StorageType storageType,char* dir, char* name);
BOOL FileAgentGetFatfsList (char* dir, char* extensionName, char* excludeFileName, fileAgentFatfsListCallback callback, void* para1, void* para2, void* para3, void* para4);
BOOL FileAgentFatfsListFile (char* dir, char* extensionName);
BOOL FileAgentFatfsDeleteFile (char* dir, char* extensionName);
BOOL FileAgentGetList(StorageType storageType, char* dir, char* extensionName, char* excludeFileName, fileAgentFatfsListCallback callback, void* para1, void* para2, void* para3, void* para4);
BOOL FileAgentFatFsFormat(char* dir);
int FileAgentGetFatFsAutoFormatCounter(void);

#if (ENABLE_BURNIN_TESTER)
BOOL AppendBurninErrorLog(char *msgBuffer, uint32_t msgLength);
#endif


#ifdef __cplusplus
}
#endif

#endif //__DATA_PROCESS_LIB_H__

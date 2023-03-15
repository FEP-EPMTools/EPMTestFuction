/**************************************************************************//**
* @file     eccblk.c
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
#include <math.h>
#ifdef _PC_ENV_
    #include "misc.h"
    #include "interface.h"
    #include "halinterface.h"
    #include "eccblk.h"
    #include "ecclog.h"
    #include "blkcommon.h"
    #define sysprintf       miscPrintf//printf
    #define pvPortMalloc    malloc
    #define vPortFree       free
#else
    #include <time.h>
    #include "nuc970.h"
    #include "sys.h"

    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "osmisc.h"
    #include "fepconfig.h"
    #include "eccblk.h"
    #include "fileagent.h"
    #include "fatfslib.h"
    #include "ecclib.h"
    #include "eccblk.h"
    #include "ecclog.h"
    #include "blkcommon.h"
    #include "timelib.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
#if(0)
static char ECCBLKFileName[ECC_BLK_MAX_FILE_NUM][_MAX_LFN];
static int ECCBLKItemNumber[ECC_BLK_MAX_FILE_NUM] = {0};
static EccBLKItem* pECCKBLNData[ECC_BLK_MAX_FILE_NUM] = {NULL};
#endif
static int currentDataIndex = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static uint64_t getUint64IDFromArray(uint8_t* idArray, int idLen)
{
    uint64_t targetID = 0;
    for(int i = 0; i < idLen; i++)
    {            
        targetID = targetID | ((uint64_t)(idArray[i])<<(8*i));
    }
    return targetID;
}

static int getItemNumFromString(uint8_t* strArray, int strLen)
{
    int targetItemNum = 0;
    for(int i = 0; i < strLen; i++)
    {            
        targetItemNum = targetItemNum + (strArray[i]-'0') * pow(10, strLen - 1 - i);
        //sysprintf(" getItemNumFromString[%d:%02x]: %08d\r\n", i, strArray[i], targetItemNum); 
    }
    return targetItemNum;
}
static int searchTargetID(uint64_t target, EccBLKItem* pEccKBItem, int itemSize)
{
    int first = 0;
    int last = itemSize - 1;
    int middle = (first+last)/2;
    #ifdef _PC_ENV_
    sysprintf(" searchTargetID: [0x%016llX]\r\n", target); 
    #else
    uint8_t* pUint8 = (uint8_t*)&target;
    sysprintf(" searchTargetID: [0x%02X %02X %02X %02X %02X %02X %02X %02X]\r\n", pUint8[7], pUint8[6], pUint8[5], pUint8[4], pUint8[3], pUint8[2], pUint8[1], pUint8[0]);
    #endif
    while (first <= last) 
    {
        if (pEccKBItem[middle].value < target)
        {
            first = middle + 1;    
        }
        else if (pEccKBItem[middle].value == target) 
        {
            
            break;
        }
        else
        {
            last = middle - 1;
        }
 
        middle = (first + last)/2;
    }
    if (first > last)
    {
        #ifdef _PC_ENV_
        sysprintf("Not found! [0x%016llX] is not present in the list.\n", target);
        #else
        sysprintf("Not found! [0x%02X %02X %02X %02X %02X %02X %02X %02X] is not present in the list.\n", pUint8[7], pUint8[6], pUint8[5], pUint8[4], pUint8[3], pUint8[2], pUint8[1], pUint8[0]);
        #endif
        return -1;
    }
    else
    {
        #ifdef _PC_ENV_
        sysprintf("found! [0x%016llX] at location %d.: [%s]\n", target, middle+1, pEccKBItem[middle].str);
        #else
        sysprintf("found! [0x%02X %02X %02X %02X %02X %02X %02X %02X] at location %d.: [%s]\n", pUint8[7], pUint8[6], pUint8[5], pUint8[4], pUint8[3], pUint8[2], pUint8[1], pUint8[0], middle+1, pEccKBItem[middle].str);
        #endif
        return middle+1;
    }
}

static BOOL ECCBLKLoad(uint8_t* data, int size, EccBLKItem** pItem, int* itemNum)
{
    uint8_t* pStart = data + 25;
    int      TargetItemNum = 0;
    uint8_t  itemDataLen = ECC_BLK_ITEM_BYTE_LEN;
    int oriSize = size;
    size = size - (25 + 32);
    int index = 0;
    //__SHOW_FREE_HEAP_SIZE__
    sysprintf("\r\n====  ECCKBLNLoad (size = %d, itemDataLen = %d) ====\r\n", size, itemDataLen);
    if((size%itemDataLen) != 0)
    {
        sysprintf("ECCKBLNLoad ERROR(maybe empty record):  [%d]\r\n", ECC_BLK_ITEM_BYTE_LEN);
        return FALSE;
    }
    *itemNum = size/itemDataLen;
    TargetItemNum = getItemNumFromString(data + oriSize - 11, 8);
    sysprintf("ECCKBLNLoad *itemNum = [%d : %d]\r\n", *itemNum, TargetItemNum);
    if(TargetItemNum != *itemNum)
    {
        sysprintf("ECCKBLNLoad TargetItemNum ERROR\r\n");
        return FALSE;
    }
    if(memcmp(data + oriSize - 3, "END", 3) == 0)
    {
        sysprintf("ECCKBLNLoad tail END OK\r\n");
    }
    else
    {
        sysprintf("ECCKBLNLoad tail END[%d] ERROR [%s]\r\n", sizeof("END"), data + oriSize - 3);
        return FALSE;
    }
    //__SHOW_FREE_HEAP_SIZE__
    if(*pItem != NULL)
    {
        sysprintf("ECCKBLNLoad FREE memory\r\n");
        //__SHOW_FREE_HEAP_SIZE__
        vPortFree(*pItem);
    }
    else
    {
        sysprintf("ECCKBLNLoad ignore FREE memory\r\n");
    }
    //__SHOW_FREE_HEAP_SIZE__
    *pItem  = (EccBLKItem*)pvPortMalloc(*itemNum * sizeof(EccBLKItem));
    if(*pItem == NULL)
    {
        sysprintf("ECCKBLNLoad ERROR: *pItem alloc error[%d]\r\n", *itemNum * sizeof(EccBLKItem));
        *itemNum = 0;
        return FALSE;
    }
    sysprintf("ECCKBLNLoad alloc size: %d bytes \r\n", *itemNum * sizeof(EccBLKItem));
    //__SHOW_FREE_HEAP_SIZE__
    for(index = 0; index < *itemNum; index++)
    {
        uint8_t* pTarget = pStart + index*itemDataLen;
        sprintf((char*)((*pItem)[index].str), "%02x%02x%02x%02x%02x%02x%02x%02x", pTarget[7], pTarget[6], pTarget[5], pTarget[4], pTarget[3], pTarget[2], pTarget[1], pTarget[0]);
#if(1)
        (*pItem)[index].value = getUint64IDFromArray(pTarget, 8);
#else
        (*pItem)[index].value = 0;        
        for(int i = 0; i<8; i++)
        {            
            (*pItem)[index].value = (*pItem)[index].value | ((uint64_t)(pTarget[i])<<(8*i));
            //sysprintf("  --> <%08d>: %d [%d, 0x%llx ]\r\n", index, i, (*pItem)[index].value, (*pItem)[index].value);  
        }
#endif
        (*pItem)[index].lockFlag = pTarget[8];
        
#if(1)
        if((pTarget[3] == 0x57) && (pTarget[2] == 0x7a)&& (pTarget[1] == 0xde))
        {
            #ifdef _PC_ENV_
            sysprintf("\r\n!!!!!!!!! <%08d>:[%s (%d) : %lld, 0x%016llX]!!!!!!!!! \r\n", index, (char*)((*pItem)[index].str), (*pItem)[index].lockFlag, (*pItem)[index].value,  (*pItem)[index].value);  
            #else
            uint8_t* pUint8 = (uint8_t*)&(*pItem)[index].value;
            sysprintf("\r\n!!!!!!!!! <%08d>:[%s (%d) : [0x%02X %02X %02X %02X %02X %02X %02X %02X]]!!!!!!!!! \r\n", index, (char*)((*pItem)[index].str), (*pItem)[index].lockFlag, pUint8[7], pUint8[6], pUint8[5], pUint8[4], pUint8[3], pUint8[2], pUint8[1], pUint8[0]);  
            #endif            
        }
#else
        /*
         *  <00000000>:[000000000009e5c5 (1) : 648645, 0x000000000009E5C5]
            <00000001>:[00000000000a8645 (2) : 689733, 0x00000000000A8645]
            <00000002>:[00000000000a93d5 (2) : 693205, 0x00000000000A93D5]
            <00000003>:[00000000000a9555 (1) : 693589, 0x00000000000A9555]
            <00000004>:[00000000000ab2e5 (1) : 701157, 0x00000000000AB2E5]
            <00000005>:[00000000000af095 (1) : 716949, 0x00000000000AF095]
         */
        if(index > 4)
            break;
        sysprintf(" <%08d>:[%s (%d) : %lld, 0x%016llX]\r\n", index, (char*)((*pItem)[index].str), (*pItem)[index].lockFlag, (*pItem)[index].value,  (*pItem)[index].value);   
        
#endif
   
    }  
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
#if(0)
BOOL ECCBLKLoadFileFromSD(char* fileName)
{
#ifdef _PC_ENV_
    return FALSE;
#else
    uint8_t* dataTmp;
    size_t dataTmpLen;
    BOOL needFree;
    FileAgentReturn reVal;  
    FRESULT result; 
//    BOOL Reval = FALSE;   
    FATFS  _FatfsVol ;   
    sysprintf(" >> ECCBLKLoadFileFromSD ... \n");
    FatfsHardwareInterface* pFatfsHardwareInterface = FatfsHardwareGetInterface(FATFS_HARDWARE_SD_INTERFACE_INDEX);
    FatfsSetCallback(0, pFatfsHardwareInterface);
    if(pFatfsHardwareInterface->initFunc())
    {
        sysprintf(" >> ECCBLKLoadFileFromSD initFunc OK... \n");
      
        result = f_mount(&_FatfsVol,"0:", 1);
        if(result != FR_NO_FILESYSTEM)
        {
            FileAgentFatfsListFile("0:", "*.*");
            reVal = FileAgentGetData(FILE_AGENT_STORAGE_TYPE_FATFS, "0:", fileName, &dataTmp, &dataTmpLen, &needFree, FALSE);
            if(reVal != FILE_AGENT_RETURN_ERROR)
            {
                sysprintf(" >> ECCBLKLoadFileFromSD FileAgentGetData OK... \n");
                ECCBLKLoad(dataTmp, dataTmpLen, &pECCKBLNData[0], &ECCBLKItemNumber[0]);
                if(needFree)
                {
                    vPortFree(dataTmp);                            
                }  
                
            }
            else
            {
                 sysprintf(" >> ECCBLKLoadFileFromSD FileAgentGetData return %d... \n", reVal);
            }
        }
        else
        {
            sysprintf(" >> ECCBLKLoadFileFromSD f_mount ERROR, return %d... \n", result);
        }

    }
    else
    {
        sysprintf(" >> ECCBLKLoadFileFromSD SdDrvInit ERROR... \n");
    }
    return FALSE;
#endif
}

static BOOL ECCBLKLoadFile(int* index, char* fileName)
{
////#ifdef _PC_ENV_
//    return FALSE;
//#else
    uint8_t* dataTmp;
    size_t dataTmpLen;
    BOOL needFree;
    FileAgentReturn reVal;  
  
    if(strlen(fileName) == 0)
    {
        sysprintf(" >> ECCBLKLoadFile error: strlen(fileName) == 0 ... \n");
        return FALSE;
    }
    sysprintf(" >> ECCBLKLoadFile %d:[%s] ... \n", * index, fileName);
    //__SHOW_FREE_HEAP_SIZE__
    reVal = FileAgentGetData(ECC_BLK_FILE_SAVE_POSITION, ECC_BLK_FILE_DIR, fileName, &dataTmp, &dataTmpLen, &needFree, TRUE);
    if(reVal != FILE_AGENT_RETURN_ERROR)
    { 
        sysprintf(" >> ECCBLKLoadFile FileAgentGetData OK (dataTmpLen = %d)... \n", dataTmpLen);
        //__SHOW_FREE_HEAP_SIZE__
        if(ECCBLKLoad(dataTmp, dataTmpLen, &pECCKBLNData[*index], &ECCBLKItemNumber[*index]))
        {
            //__SHOW_FREE_HEAP_SIZE__
            strcpy(ECCBLKFileName[*index], fileName);
            sysprintf(" >> ECCBLKLoadFile copy file name to ECCBLKFileName [%s]... \n", ECCBLKFileName[*index]);
#if(0)
            uint8_t* eccBlkFeedbackLogBody = (uint8_t*)pvPortMalloc(sizeof(ECCBlkFeedbackLogBody));
            time_t epmUTCTime = GetCurrentUTCTime();
            ECCBlkFeedbackLogContainInit((ECCBlkFeedbackLogBody*)eccBlkFeedbackLogBody, NEW_SP_ID, SVCE_LOC_ID, fileName, epmUTCTime);
            
            #ifdef _PC_ENV_
            MiscSaveToFile(ECCBlkFeedbackLogGetFileName(), (uint8_t*)eccBlkFeedbackLogBody, sizeof(ECCBlkFeedbackLogBody));
            #else            
            char targetLogFileName[_MAX_LFN];
            sprintf(targetLogFileName,"%ss", ECCBlkFeedbackLogGetFileName()); 

            SFlashAppendRecord(targetLogFileName, SFLASH_RECORD_TYPE_ECC, (uint8_t*)eccBlkFeedbackLogBody, sizeof(ECCBlkFeedbackLogBody));

            if(FileAgentAddData(ECC_LOG_FILE_SAVE_POSITION, ECC_LOG_FILE_DIR, ECCBlkFeedbackLogGetFileName(), (uint8_t*)eccBlkFeedbackLogBody, sizeof(ECCBlkFeedbackLogBody), FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
            {

            }    
            #endif
#endif
        }
        else
        {
            sysprintf(" >> ECCBLKLoadFile delete [%s] ... \n", fileName);
            FileAgentDelFile(ECC_BLK_FILE_SAVE_POSITION, ECC_BLK_FILE_DIR, fileName);
            memset(ECCBLKFileName[*index], 0x0, sizeof(ECCBLKFileName[*index]));
        }
        if(needFree)
        {
            sysprintf(" >> ECCBLKLoadFile vPortFree ... \n");
            //__SHOW_FREE_HEAP_SIZE__
            vPortFree(dataTmp);   
            //__SHOW_FREE_HEAP_SIZE__            
        }  
        
                
    }
    else
    {
         sysprintf(" >> ECCBLKLoadFile FileAgentGetData return %d... \n", reVal);
    }
    {
        char* fileName;
        ECCBLKSearchTargetID(0x1234567890, &fileName);
    }
    return FALSE;
//#endif
}
#ifdef _PC_ENV_
#else
static BOOL blkCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4, void* para5)
{   
    int* index = (int*)para1;
    if(*index >= ECC_BLK_MAX_FILE_NUM)
    {
        sysprintf(" >> [ECC] blkCallback [%s] ignore: index = %d(%d)... \n", filename, currentDataIndex, *index);
    }
    else
    {
        sysprintf(" >> [ECC] blkCallback [%s]: index = %d(%d)... \n", filename, currentDataIndex, *index);
        ECCBLKLoadFile(index, filename);
        *index = *index + 1;
    }
    
    return TRUE;
}
#endif

BOOL ECCBLKLoadAllFile(void)
{
    sysprintf(" >> [ECC] ECCBLKLoadAllFile... \n");

    currentDataIndex = 0;
    #if(0)
    for(int i = 0; i<ECC_BLK_MAX_FILE_NUM; i++)
    {
        if(pECCKBLNData[i] != NULL)
        {
            sysprintf("ECCBLKLoadAllFile FREE memory\r\n");
            //__SHOW_FREE_HEAP_SIZE__
            vPortFree(pECCKBLNData[i]);
            //__SHOW_FREE_HEAP_SIZE__
        }
        pECCKBLNData[i] = NULL;
        ECCBLKItemNumber[i] = 0;
    }
    #endif
#ifdef _PC_ENV_
    int index = 0;
    //ECCBLKLoadFile(&index, "BLC03331A_161221.BA2");   
    ECCBLKLoadFile(&index, "BLC03331A_16122X.BA2");   
#else
    FileAgentGetList(ECC_BLK_FILE_SAVE_POSITION, ECC_BLK_FILE_DIR, FILE_EXTENSION_EX(ECC_BLK_FILE_EXTENSION), NULL, blkCallback, &currentDataIndex, NULL, NULL, NULL, NULL);
#endif
    return TRUE;

}

int ECCBLKSearchTargetID(uint64_t target, char** blkFileName)
{
    int i = 0;
    int reVal = -1;
    for(i = 0; i<ECC_BLK_MAX_FILE_NUM; i++)
    {
        if((pECCKBLNData[i] == NULL) || (ECCBLKItemNumber[i] == 0))
        {
          
        }
        else
        {  
            reVal = searchTargetID(target, pECCKBLNData[i], ECCBLKItemNumber[i]);
            if(reVal != -1)
            {
                *blkFileName = ECCBLKFileName[i];
                break;
            }
        }
    }
    return reVal;
}

int ECCBLKSearchTargetIDByArray(uint8_t* idArray, int idsize, char** blkFileName)
{
    //### edcaRead: cardIDBytes = 4 (F0DE7A57) [0xF0, 0xDE, 0x7A, 0x57, 0x00, 0x00, 0x00]
    uint64_t targetID = getUint64IDFromArray(idArray, idsize);
    return ECCBLKSearchTargetID(targetID, blkFileName);
}
#endif
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


/**************************************************************************//**
* @file     ipasskb.c
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
    #include "ipassblk.h"
    #include "blkcommon.h"
    #define sysprintf       printf
    #define pvPortMalloc    malloc
    #define vPortFree       free
#else
    #include "nuc970.h"
    #include "sys.h"

    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "fepconfig.h"
    #include "ipassblk.h"
    #include "fileagent.h"
    #include "fatfslib.h"
    #include "blkcommon.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define IPASS_BLK_MAX_FILE_NUM    2
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static int IPASSBLKItemNumber[IPASS_BLK_MAX_FILE_NUM] = {0, 0};
static IPassBLKItem* pIPASSKBLNData[IPASS_BLK_MAX_FILE_NUM] = {NULL, NULL};
static int currentDataIndex = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


static int searchTargetID(uint32_t target, IPassBLKItem* pIPassKBItem, int itemSize)
{
    int first = 0;
    int last = itemSize - 1;
    int middle = (first+last)/2;
 
    while (first <= last) 
    {
        if (pIPassKBItem[middle].value < target)
        {
            first = middle + 1;    
        }
        else if (pIPassKBItem[middle].value == target) 
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
        sysprintf("Not found! [0x%08x] is not present in the list.\n", target);
        return -1;
    }
    else
    {
        sysprintf("found! [0x%08x] at location %d.: [%s]\n", target, middle+1, pIPassKBItem[middle].str);
        return middle+1;
    }
}
static BOOL IPASSBLKLoad(uint8_t* data, int size, IPassBLKItem** pItem, int* itemNum)
{
    uint8_t* pStart = data;
    uint8_t* pEndStr = (uint8_t*)"\r\n";
    uint8_t  itemDataLen = IPASS_BLK_ITEM_BYTE_LEN + strlen((char*)pEndStr);
    int index = 0;
    sysprintf("\r\n====  IPASSKBLNLoad (size = %d, itemDataLen = %d) ====\r\n", size, itemDataLen);
    if((size%itemDataLen) != 0)
    {
        sysprintf("IPASSKBLNLoad ERROR(maybe empty record):  [%d]\r\n", IPASS_BLK_ITEM_BYTE_LEN + strlen((char*)pEndStr));
        //return FALSE;
        return TRUE;//為了不被刪除, 回應TRUE
    }
    *itemNum = size/itemDataLen;
    sysprintf("IPASSKBLNLoad *itemNum = [%d]\r\n", *itemNum);
    
    if(*pItem != NULL)
        vPortFree(*pItem);
    
    *pItem  = (IPassBLKItem*)pvPortMalloc(*itemNum * sizeof(IPassBLKItem));
    if(*pItem == NULL)
    {
        sysprintf("IPASSKBLNLoad ERROR: *pItem alloc error[%d]\r\n", *itemNum * sizeof(IPassBLKItem));
        *itemNum = 0;
        return FALSE;
    }
    for(index = 0; index < *itemNum; index++)
    {
        uint8_t* pTarget = pStart + index*itemDataLen;
        //IPassBLKItem item;
        if(memcmp((const void*)(pTarget + IPASS_BLK_ITEM_BYTE_LEN), (const void*)pEndStr, strlen((char*)pEndStr)) != 0)
        {
            sysprintf("IPASSKBLNLoad ERROR: Separator error\r\n");
            if(*pItem != NULL)
                vPortFree(*pItem);
            *itemNum = 0;
            return FALSE;
        }
        memcpy((*pItem)[index].str, pTarget, IPASS_BLK_ITEM_BYTE_LEN);
        (*pItem)[index].str[IPASS_BLK_ITEM_BYTE_LEN] = 0x0;
        (*pItem)[index].value = BlkHexStr2Dec(((*pItem)[index].str), strlen((char*)(*pItem)[index].str));//strtol((char*)*pItem[index].str,NULL,16);
        //sysprintf(" <%08d>:[%s: 0x%08x]\r\n", index, (char*)*pItem[index].str, *pItem[index].value);        
    }  
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
#if(0)
BOOL IPASSBLKLoadFileFromSD(char* fileName)
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
    sysprintf(" >> IPASSBLKLoadFileFromSD ... \n");
    FatfsHardwareInterface* pFatfsHardwareInterface = FatfsHardwareGetInterface(FATFS_HARDWARE_SD_INTERFACE_INDEX);
    FatfsSetCallback(0, pFatfsHardwareInterface);
    if(pFatfsHardwareInterface->initFunc())
    {
        sysprintf(" >> IPASSBLKLoadFileFromSD initFunc OK... \n");
      
        result = f_mount(&_FatfsVol,"0:", 1);
        if(result != FR_NO_FILESYSTEM)
        {
            FileAgentFatfsListFile("0:", "*.*");
            reVal = FileAgentGetData(FILE_AGENT_STORAGE_TYPE_FATFS, "0:", fileName, &dataTmp, &dataTmpLen, &needFree, FALSE);
            if(reVal != FILE_AGENT_RETURN_ERROR)
            {
                sysprintf(" >> IPASSBLKLoadFileFromSD FileAgentGetData OK... \n");
                IPASSBLKLoad(dataTmp, dataTmpLen, &pIPASSKBLNData[0], &IPASSBLKItemNumber[0]);
                if(needFree)
                {
                    vPortFree(dataTmp);                            
                }  
                
            }
            else
            {
                 sysprintf(" >> IPASSBLKLoadFileFromSD FileAgentGetData return %d... \n", reVal);
            }
        }
        else
        {
            sysprintf(" >> IPASSBLKLoadFileFromSD f_mount ERROR, return %d... \n", result);
        }

    }
    else
    {
        sysprintf(" >> IPASSBLKLoadFileFromSD SdDrvInit ERROR... \n");
    }
    return FALSE;
#endif
}
#endif
static BOOL IPASSBLKLoadFile(int* index, char* fileName)
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
        sysprintf(" >> IPASSBLKLoadFile error: strlen(fileName) == 0 ... \n");
        return FALSE;
    }
    sysprintf(" >> IPASSBLKLoadFile[%s] ... \n", fileName);

    reVal = FileAgentGetData(IPASS_BLK_FILE_SAVE_POSITION, IPASS_BLK_FILE_DIR, fileName, &dataTmp, &dataTmpLen, &needFree, TRUE);
    if(reVal != FILE_AGENT_RETURN_ERROR)
    { 
        sysprintf(" >> IPASSBLKLoadFile FileAgentGetData OK (dataTmpLen = %d)... \n", dataTmpLen);
        if(IPASSBLKLoad(dataTmp, dataTmpLen, &pIPASSKBLNData[*index], &IPASSBLKItemNumber[*index]))
        {
        }
        else
        {
            sysprintf(" >> IPASSBLKLoadFile delete [%s] ... \n", fileName);
            FileAgentDelFile(ECC_BLK_FILE_SAVE_POSITION, ECC_BLK_FILE_DIR, fileName);
        }
        if(needFree)
        {
            vPortFree(dataTmp);                            
        }  
                
    }
    else
    {
         sysprintf(" >> IPASSBLKLoadFile FileAgentGetData return %d... \n", reVal);
    }

    return FALSE;
//#endif
}
#ifdef _PC_ENV_
#else
static BOOL blkCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4, void* para5)
{   
    int* index = (int*)para1;
    if(*index >= IPASS_BLK_MAX_FILE_NUM)
    {
        sysprintf(" >> [IPASS] blkCallback [%s] ignore: index = %d(%d)... \n", filename, currentDataIndex, *index);
    }
    else
    {
        sysprintf(" >> [IPASS] blkCallback [%s]: index = %d(%d)... \n", filename, currentDataIndex, *index);
        IPASSBLKLoadFile(index, filename);
        *index = *index + 1;
    }
    
    return TRUE;
}
#endif

BOOL IPASSBLKLoadAllFile(void)
{
    sysprintf(" >> [IPASS] IPASSBLKLoadAllFile... \n");

    currentDataIndex = 0;
    for(int i = 0; i<IPASS_BLK_MAX_FILE_NUM; i++)
    {
        //pIPASSKBLNData[i] = NULL;
        IPASSBLKItemNumber[i] = 0;
    }
#ifdef _PC_ENV_
    int index = 0;
    IPASSBLKLoadFile(&index, ".//KBLN_2017061801.DAT");   
#else
//    FileAgentGetList(IPASS_BLK_FILE_SAVE_POSITION, IPASS_BLK_FILE_DIR, FILE_EXTENSION_EX(IPASS_BLK_FILE_EXTENSION), NULL, blkCallback, &currentDataIndex, NULL, NULL, NULL, NULL);
#endif
    return TRUE;

}

int IPASSBLKSearchTargetID(uint32_t target)
{
    int i = 0;
    int reVal = -1;
    for(i = 0; i<IPASS_BLK_MAX_FILE_NUM; i++)
    {
        if((pIPASSKBLNData[i] == NULL) || (IPASSBLKItemNumber[i] == 0))
        {
          
        }
        else
        {  
            reVal = searchTargetID(target, pIPASSKBLNData[i], IPASSBLKItemNumber[i]);
            if(reVal != -1)
                break;
        }
    }
    return reVal;
}

int IPASSBLKSearchTargetIDByString(uint8_t* str, int size)
{
    return IPASSBLKSearchTargetID(BlkHexStr2Dec(str, size));
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


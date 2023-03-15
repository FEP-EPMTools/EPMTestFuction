/**************************************************************************//**
* @file     sflashrecord.c
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
#include "sflashrecord.h"
#include "fileagent.h"
#include "loglib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define SFLASH_DEBUG    0
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint32_t     currentMaxIndex = 0;
static int          headerRecordIndex = 0;
static int          endRecordIndex = -1;
static uint8_t      currentStatus[SFLASH_RECORD_TOTAL_NUM];
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void printfBuffData(char* str, uint8_t* data, int len)
{
    int i;
    sysprintf("\r\n %s: len = %d...\r\n  ", str, len);
    
    for(i = 0; i<len; i++)
    { 
        #if(1)
        sysprintf("0x%02x, ", data[i]);
        if(i%0x10 == 0xf)
            sysprintf("\r\n  ");
        #else
        if((data[i]>=0x20)&&(data[i]<=0x7e))
            sysprintf("[%02d]:0x%02x(%c)\r\n", i, (unsigned char)data[i], (unsigned char)data[i]);
        else
            sysprintf("[%02d]:0x%02x\r\n", i, (unsigned char)data[i]);
        #endif
        
    }
    sysprintf("\r\n");
    
}


/*
static int pageToRecordIndex(int pageIndex)
{
    if((pageIndex%SFLASH_RECORD_PAGE_SIZE_PER_RECORD) != 0)
    {
        return -1;
    }
    else
    {
        return pageIndex/SFLASH_RECORD_PAGE_SIZE_PER_RECORD;
    }
}
static int recordToPageIndex(int recordIndex)
{
    return recordIndex*SFLASH_RECORD_PAGE_SIZE_PER_RECORD;
}
*/
static uint32_t recordIndexToStartAddress(int recordIndex)
{
    return SFLASH_RECORD_START_ADDRESS + recordIndex*SFLASH_RECORD_SIZE;
}

static uint32_t recordIndexToStatusAddress(int recordIndex)
{
    sysprintf(" >> recordIndexToStatusAddress : recordIndex = %d, -> 0x%08x \r\n", recordIndex, recordIndexToStartAddress(recordIndex+1) - sizeof(SFlashRecordEnd) - sizeof(uint8_t));
    return recordIndexToStartAddress(recordIndex+1) - sizeof(SFlashRecordEnd) - sizeof(uint8_t);
}

//is first page/record in sector
static BOOL isFirstRecordInSector(int recordIndex)
{
    if((recordIndex%SFLASH_RECORD_NUM_PER_SECTOR) == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
static BOOL getEntireRecord(uint8_t flashIndex, int recordIndex, uint8_t* pData, int dataLen)
{
    if(dataLen != SFLASH_RECORD_SIZE)
        return FALSE;
    FlashDrvNormalRead(flashIndex, recordIndexToStartAddress(recordIndex), pData, SFLASH_RECORD_SIZE);
    return TRUE;
}

static BOOL getEntireRecordEx(uint8_t flashIndex, int recordIndex, int recordNum, uint8_t* pData, int dataLen)
{
    if(dataLen != SFLASH_RECORD_SIZE*recordNum)
    {
        sysprintf(" >> getEntireRecordEx Len error : dataLen = %d, SFLASH_RECORD_SIZE*recordNum = %d \r\n", dataLen, SFLASH_RECORD_SIZE*recordNum);
        return FALSE;
    }
    FlashDrvNormalRead(flashIndex, recordIndexToStartAddress(recordIndex), pData, SFLASH_RECORD_SIZE*recordNum);
    return TRUE;
}

//mark record status
static BOOL makeRecordStatus(uint8_t flashIndex, int recordIndex, uint8_t status)
{
    sysprintf(" >> makeRecordStatus : recordIndex = %d, status = 0x%02x \r\n", recordIndex, status);
    FlashDrvPageProgram(flashIndex, recordIndexToStatusAddress(recordIndex), &status, 1);
    return TRUE;
}
static uint16_t getChecksum(uint8_t* pTarget, uint16_t len, char* str)
{
    int i;
    uint16_t checksum = 0;
    uint8_t* pr = (uint8_t*)pTarget;
    //sysprintf("  -- getChecksum (%s) : len = %d\r\n", str, len); 
    for(i = 0; i< len; i++) 
    {
        checksum = checksum + pr[i];
    }
    //sysprintf("  -- getChecksum (%s) : checksum = 0x%x (%d)\r\n", str, checksum, checksum); 
    return checksum;
}
//check data to mark bad or get currentMaxIndex / currentRecordIndex
static BOOL checkEntireRecord(uint8_t* srcData, uint8_t** targetData, size_t* targetDataLen)
{    
    //SFlashRecordHeader* pSFlashRecordHeader = (SFlashRecordHeader*)srcData;  
    SFlashRecord*       pSFlashRecord       = (SFlashRecord*)srcData;  
    SFlashRecordHeader* pSFlashRecordHeader = &(pSFlashRecord->header);  
    SFlashRecordEnd*    pSFlashRecordEnd    = &(pSFlashRecord->end); 
    //sysprintf("\r\checkEntireRecord[0x%02x, 0x%02x] len = %d: [srcDataLen = %d]...\r\n", pSFlashRecordHeader->value[0], pSFlashRecordHeader->value[1], pSFlashRecordHeader->Len, srcDataLen);

    if((pSFlashRecordHeader->value[0] == SFLASH_HEADER_VALUE) && 
        (pSFlashRecordHeader->value[1] == SFLASH_HEADER_VALUE2))
            //&& pSFlashRecordHeader->Len == (srcDataLen-sizeof(SFlashRecordHeader) - sizeof(SFlashRecordEnd))) )       
    {
            //uint16_t checkSum = getChecksum(srcData + sizeof(SFlashRecordHeader), pSFlashRecordHeader->Len, "<parser>") ; 
            uint16_t checkSum = getChecksum(srcData + sizeof(SFlashRecordHeader), SFLASH_RECORD_SIZE - sizeof(SFlashRecordHeader) - sizeof(SFlashRecordEnd) - sizeof(uint8_t), "<parser>") ; 
            
            if((checkSum == pSFlashRecordEnd->checksum)  &&
                (pSFlashRecordEnd->value[0] == SFLASH_END_VALUE) && 
                (pSFlashRecordEnd->value[1] == SFLASH_END_VALUE2))
            {
                *targetData = pSFlashRecord->data;// srcData + sizeof(SFlashRecordHeader);
                *targetDataLen = pSFlashRecordHeader->Len;
                srcData[pSFlashRecordHeader->Len + sizeof(SFlashRecordHeader)] = 0x0;
                //sysprintf("\r\n checkEntireRecord OK .\r\n");
                //printfBuffData("checkEntireRecord", *targetData, 32);
                return TRUE;
            }
            else
            {
                terninalPrintf("\r\n checkEntireRecord End ERROR [0x%02x, 0x%02x] checkSum = %d (%d)...\r\n", pSFlashRecordEnd->value[0], pSFlashRecordEnd->value[1], pSFlashRecordEnd->checksum, checkSum);
                #if(0)
                printfBuffData("checkEntireRecord", srcData, SFLASH_RECORD_SIZE);
                #endif
            }
    }
    else
    {
        terninalPrintf("\r\n checkEntireRecord Header ERROR [0x%02x, 0x%02x] ...\r\n", pSFlashRecordHeader->value[0], pSFlashRecordHeader->value[1]);
        printfBuffData("checkEntireRecord", srcData, SFLASH_RECORD_SIZE);
    }
    return FALSE;
}
#define SFLASH_BATCH_CHECK_NUM   SFLASH_RECORD_TOTAL_NUM
static void scanSFlash(void)
{
    int i, j;   
    int targetIndex;    
    //SFlashRecord data[SFLASH_BATCH_CHECK_NUM];
    SFlashRecord* data = pvPortMalloc(sizeof(SFlashRecord) * SFLASH_BATCH_CHECK_NUM);
    if(data == NULL)
    {
        sysprintf("S-Flash pvPortMalloc Fail .........\r\n");
        return;
    }
    SFlashRecord* pSFlashRecord;// = (SFlashRecord*)data;
    uint8_t* targetData;
    size_t targetDataLen;
    for(i = 0; i < SFLASH_RECORD_TOTAL_NUM/SFLASH_BATCH_CHECK_NUM ; i++)
    {
        //getEntireRecord(SPI_FLASH_EX_0_INDEX, i, data, sizeof(data));  
        getEntireRecordEx(SPI_FLASH_EX_0_INDEX, i * SFLASH_BATCH_CHECK_NUM, SFLASH_BATCH_CHECK_NUM, (uint8_t*)data, sizeof(SFlashRecord) * SFLASH_BATCH_CHECK_NUM/*sizeof(data)*/);
        //sprintf(title, "scanSFlash_%02d [Before]", i);
        #if(SFLASH_DEBUG)
        sysprintf("S-Flash Batch Check <%04d ~ %04d>\r\n", SFLASH_BATCH_CHECK_NUM * i + 0, SFLASH_BATCH_CHECK_NUM * i + SFLASH_BATCH_CHECK_NUM - 1);
        #endif
        for(j = 0; j < SFLASH_BATCH_CHECK_NUM ; j++)
        {
            //printfBuffData("S-Flash Check", data, SFLASH_RECORD_SIZE); 
            targetIndex = SFLASH_BATCH_CHECK_NUM * i + j;
            #if(!SFLASH_DEBUG)
            sysprintf("S-Flash Check <%04d/%04d>\r", targetIndex, SFLASH_RECORD_TOTAL_NUM);
            #endif
            pSFlashRecord = &data[j];
            currentStatus[targetIndex] = pSFlashRecord->status;        
            switch(pSFlashRecord->status)
            {
                case SFLASH_RECORD_STATUS_EMPTY:
                    #if(SFLASH_DEBUG)
                    sysprintf(" <%08d> -> [EMPTY]\r\n", targetIndex);
                    #endif
                    break;
                
                case SFLASH_RECORD_STATUS_DATA:                
                    if(checkEntireRecord((uint8_t*)pSFlashRecord, &targetData, &targetDataLen))
                    {
                        #if(SFLASH_DEBUG)
                        sysprintf(" <%08d> -> [DATA] : index = %d, name = [%s] \r\n      data = [%s] \r\n", targetIndex, pSFlashRecord->index, pSFlashRecord->name, targetData);
                        #endif
                        if(currentMaxIndex <  pSFlashRecord->index)
                        {
                            currentMaxIndex = pSFlashRecord->index;
                            headerRecordIndex = targetIndex + 1; 
                            headerRecordIndex = headerRecordIndex % SFLASH_RECORD_TOTAL_NUM;
                        }  
                        if(endRecordIndex == -1)
                        {
                            endRecordIndex = targetIndex;
                        }                        
                    } 
                    else
                    {
                        #if(SFLASH_DEBUG)
                        sysprintf(" <%08d> -> [DATA] ERROR: Mark BAD Record \r\n", targetIndex);
                        #endif
                        makeRecordStatus(SPI_FLASH_EX_0_INDEX, targetIndex, SFLASH_RECORD_STATUS_BAD);
                        currentStatus[targetIndex] = SFLASH_RECORD_STATUS_BAD;
                    }                    
                    break;
                    
                case SFLASH_RECORD_STATUS_BACKUP:                
                    if(checkEntireRecord((uint8_t*)pSFlashRecord, &targetData, &targetDataLen))
                    {
                        #if(SFLASH_DEBUG)
                        sysprintf(" <%08d> -> [BACKUP] : index = %d, name = [%s] \r\n      data = [%s] \r\n", targetIndex, pSFlashRecord->index, pSFlashRecord->name, targetData);
                        #endif
                        if(currentMaxIndex <  pSFlashRecord->index)
                        {
                            currentMaxIndex = pSFlashRecord->index;
                            headerRecordIndex = targetIndex +1; 
                            headerRecordIndex = headerRecordIndex % SFLASH_RECORD_TOTAL_NUM;
                        }   
                    } 
                    else
                    {
                        #if(SFLASH_DEBUG)
                        sysprintf(" <%08d> -> [BACKUP] ERROR: Mark BAD Record \r\n", targetIndex);
                        #endif
                        makeRecordStatus(SPI_FLASH_EX_0_INDEX, targetIndex, SFLASH_RECORD_STATUS_BAD);
                        currentStatus[targetIndex] = SFLASH_RECORD_STATUS_BAD;
                    }                 
                    break;
                    
                case SFLASH_RECORD_STATUS_BAD:
                    #if(SFLASH_DEBUG)
                    sysprintf(" <%08d> -> [BAD] \r\n", targetIndex);
                    #endif
                    break;    
                    
                default:
                    #if(SFLASH_DEBUG)
                    sysprintf(" <%08d> -> [OTHER] \r\n", targetIndex);
                    #endif
                    makeRecordStatus(SPI_FLASH_EX_0_INDEX, targetIndex, SFLASH_RECORD_STATUS_BAD);
                    currentStatus[targetIndex] = SFLASH_RECORD_STATUS_BAD;
                    break;
            }
            //getEntireRecord(SPI_FLASH_EX_0_INDEX, targetIndex, data, sizeof(data));
            //sprintf(title, "scanSFlash_%02d [End]", targetIndex);
            //printfBuffData(title, data, SFLASH_RECORD_SIZE);
            //sysprintf(" -- currentStatus[%d] = %d (0X%02X), currentMaxIndex = %d -- \r\n", targetIndex, currentStatus[targetIndex], currentStatus[targetIndex], currentMaxIndex);
        }

    }
    //printfBuffData(" [ Current Status ] ", currentStatus, SFLASH_RECORD_TOTAL_NUM);
    if(endRecordIndex == -1)
    {
        //endRecordIndex = 0;
        endRecordIndex = headerRecordIndex;
    }  
    sysprintf("\r\n #!!!# scanSFlash END: total record number = %d, currentMaxIndex = %d, headerRecordIndex = %d, endRecordIndex = %d -- \r\n", SFLASH_RECORD_TOTAL_NUM, currentMaxIndex, headerRecordIndex, endRecordIndex);
    vPortFree(data); 
}

static void listSFlash(uint8_t flashIndex)
{
    int i;    
    uint8_t data[SFLASH_RECORD_SIZE];
    SFlashRecord* pSFlashRecord = (SFlashRecord*)data;
    uint8_t* targetData;
    size_t targetDataLen;
    for(i = 0; i < SFLASH_RECORD_TOTAL_NUM ; i++)
    {
        getEntireRecord(flashIndex, i, data, sizeof(data));        
        //sprintf(title, "listSFlash%02d [Before]", i);
        //printfBuffData(title, data, SFLASH_RECORD_SIZE);
        
        //if(checkEntireRecord(data, &targetData, &targetDataLen) == FALSE)
        //{
        //    #if(SFLASH_DEBUG)
        //    terninalPrintf(" <%08d> -> [checkEntireRecord] ERROR\r\n", i);
        //    #endif
        //    continue;
        //}
        
        currentStatus[i] = pSFlashRecord->status;        
        switch(pSFlashRecord->status)
        {
            case SFLASH_RECORD_STATUS_EMPTY:
                terninalPrintf(" <%08d> -> [EMPTY]\r\n", i);
                break;
            
            case SFLASH_RECORD_STATUS_DATA:                
                if(checkEntireRecord(data, &targetData, &targetDataLen))
                {
                    terninalPrintf(" <%08d> -> [DATA] : index = %d, len = %d, name = [%s] \r\n      data = [%s] \r\n", i, pSFlashRecord->index, targetDataLen, pSFlashRecord->name, targetData);                    
                } 
                else
                {
                    terninalPrintf(" <%08d> -> [DATA] ERROR\r\n", i);
                }                    
                break;
                
            case SFLASH_RECORD_STATUS_BACKUP:                
                if(checkEntireRecord(data, &targetData, &targetDataLen))
                {
                    terninalPrintf(" <%08d> -> [BACKUP] : index = %d, len = %d, name = [%s] \r\n      data = [%s] \r\n", i, pSFlashRecord->index, targetDataLen, pSFlashRecord->name, targetData);      
                } 
                else
                {
                    terninalPrintf(" <%08d> -> [BACKUP] ERROR\r\n", i);
                }                 
                break;
                
            case SFLASH_RECORD_STATUS_BAD:
                terninalPrintf(" <%08d> -> [BAD] \r\n", i);            
                break;    
                
            default:
                terninalPrintf(" <%08d> -> [OTHER] \r\n", i);
                break;
        }
    }
    terninalPrintf(" ##  total record number = %d, currentMaxIndex = %d, headerRecordIndex = %d, endRecordIndex = %d -- \r\n", SFLASH_RECORD_TOTAL_NUM, currentMaxIndex, headerRecordIndex, endRecordIndex);
}
static uint32_t getNextUseableRecordAddress(void)
{    
    uint32_t addressTmp;
    int headerRecordIndexTmp;
    
    if(isFirstRecordInSector(headerRecordIndex))  
    {
        sysprintf(" -[ INFORMATION ]- getNextUseableRecordAddress [%d] need erase \r\n", headerRecordIndex);
        FlashDrvSectorErase(SPI_FLASH_EX_0_INDEX, recordIndexToStartAddress(headerRecordIndex));
        for(int i = 0; i < SFLASH_RECORD_NUM_PER_SECTOR ; i++)
        {
            int targetRecordIndex = i + headerRecordIndex;
            if(targetRecordIndex < SFLASH_RECORD_TOTAL_NUM)
            {
                currentStatus[targetRecordIndex] = SFLASH_RECORD_STATUS_EMPTY;
                sysprintf(" -[ INFORMATION ]- getNextUseableRecordAddress Set currentStatus[%d] = SFLASH_RECORD_STATUS_EMPTY  \r\n", targetRecordIndex);
            }  
        }
    } 
    headerRecordIndexTmp = headerRecordIndex;
    while(currentStatus[headerRecordIndex] != SFLASH_RECORD_STATUS_EMPTY)
    {
        ++headerRecordIndex;
        headerRecordIndex = headerRecordIndex % SFLASH_RECORD_TOTAL_NUM;
        if(headerRecordIndexTmp == headerRecordIndex)
        {
            #if(ENABLE_LOG_FUNCTION)
            {
                char str[512];
                sprintf(str, "**! INFORMATION !** S-Flash getNextUseableRecordAddress ERROR (headerRecordIndexTmp = %d)...\r\n", headerRecordIndexTmp);
                LoglibPrintf(LOG_TYPE_ERROR, str);
            }
            #else
            sysprintf("**! INFORMATION !** S-Flash getNextUseableRecordAddress ERROR (headerRecordIndexTmp = %d)...\r\n", headerRecordIndexTmp);
            #endif
            break;
        }
    }
    addressTmp = recordIndexToStartAddress(headerRecordIndex);
    sysprintf(" -[ INFORMATION ]- getNextUseableRecordAddress Get Target Record Index [%d][0x%04x] \r\n", headerRecordIndex, addressTmp);    
    ++headerRecordIndex;
    headerRecordIndex = headerRecordIndex % SFLASH_RECORD_TOTAL_NUM;
    return addressTmp;  
}

static uint32_t getNextDataRecordAddress(void)
{
    if(headerRecordIndex == -1)
    {
        //sysprintf(" -- getNextDataRecordAddress empty [%d]  \r\n", endRecordIndex);
        return 0xffffffff;
    }
        
    for(;;)
    {              
        //sysprintf(" -- getNextDataRecordAddress Check [%d: %d]:  0X%02X \r\n", endRecordIndex, headerRecordIndex, currentStatus[endRecordIndex]);
        switch(currentStatus[endRecordIndex]) 
        {
            case SFLASH_RECORD_STATUS_EMPTY:
                break;
            
            case SFLASH_RECORD_STATUS_DATA:  
                return recordIndexToStartAddress(endRecordIndex);
                
            case SFLASH_RECORD_STATUS_BACKUP:
                break;
                
            case SFLASH_RECORD_STATUS_BAD:
                break;
                
            default:
                //因為 scan過 不太可能發生
                break;
        }
        if(endRecordIndex == headerRecordIndex)
        {
            //sysprintf(" -- getNextDataRecordAddress break [%d]  \r\n", endRecordIndex);
            return 0xffffffff;
        }
        ++endRecordIndex;
        endRecordIndex = endRecordIndex % SFLASH_RECORD_TOTAL_NUM;  
    }
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL SFlashRecordInit(void)
{
    sysprintf("SFlashRecordInit!! ( start page = %d, RECORD_SIZE size = %d, sizeof(SFlashRecord) == %d) \r\n", SFLASH_RECORD_START_PAGE, SFLASH_RECORD_SIZE, sizeof(SFlashRecord));
    if(SFLASH_RECORD_SIZE != sizeof(SFlashRecord))
    {
        sysprintf("SFlashRecordInit ERROR !! (SFLASH_RECORD_SIZE != sizeof(SFlashRecord)) \r\n");
        return FALSE;
    }
    scanSFlash();
    return TRUE;
}
void SFlashPrintfRecord(uint8_t* srcData)
{
    SFlashRecord* pSFlashRecord = (SFlashRecord*)srcData;
    SFlashRecordHeader* pSFlashRecordHeader = (SFlashRecordHeader*)&(pSFlashRecord->header); 
    SFlashRecordEnd* pSFlashRecordEnd = (SFlashRecordEnd*)(SFlashRecordHeader*)&(pSFlashRecord->end); 
    
    sysprintf("SFlashPrintfRecord (%s) Header [0x%02x, 0x%02x] len = %d...\r\n", pSFlashRecord->name,  pSFlashRecordHeader->value[0], pSFlashRecordHeader->value[1], pSFlashRecordHeader->Len);
    sysprintf("SFlashPrintfRecord (%s) End [0x%02x, 0x%02x] checksum = 0x%04x...\r\n", pSFlashRecord->name, pSFlashRecordEnd->value[0], pSFlashRecordEnd->value[1], pSFlashRecordEnd->checksum);
    //printfBuffData(" [ SFlashPrintfRecord ] ", srcData + sizeof(SFlashRecordHeader), pSFlashRecordHeader->Len);
}
void SFlashListRecord(uint8_t flashIndex)
{
    setPrintfFlag(TRUE);
    listSFlash(flashIndex);
    setPrintfFlag(FALSE);
}
BOOL SFlashAppendRecord(char* name, uint8_t type, uint8_t* dataSrc, int dataSrcLen)
{
    uint32_t targetAddress;
    uint32_t targetRecordIndex;
    uint8_t data[SFLASH_RECORD_SIZE];
    SFlashRecord* pSFlashRecord = (SFlashRecord*)data;
    SFlashRecordHeader* pSFlashRecordHeader = (SFlashRecordHeader*)&(pSFlashRecord->header); 
    SFlashRecordEnd* pSFlashRecordEnd = (SFlashRecordEnd*)&(pSFlashRecord->end); 

    memset(data, 0xff, SFLASH_RECORD_SIZE);
    
    pSFlashRecord->type = type;

    pSFlashRecordHeader->value[0] = SFLASH_HEADER_VALUE;
    pSFlashRecordHeader->value[1] = SFLASH_HEADER_VALUE2;
    pSFlashRecordHeader->Len = dataSrcLen;  

    //memcpy(pSFlashRecord->data + sizeof(SFlashRecordHeader), dataSrc, dataSrcLen);
    memcpy(pSFlashRecord->data, dataSrc, dataSrcLen);

    
    pSFlashRecordEnd->value[0] = SFLASH_END_VALUE;
    pSFlashRecordEnd->value[1] = SFLASH_END_VALUE2;
    
    pSFlashRecord->index = ++currentMaxIndex;
    
    if(strlen(name) > (SFLASH_RECORD_NAME_LEN-1))
        name[SFLASH_RECORD_NAME_LEN-1] = 0x0;
    
    strcpy((char*)(pSFlashRecord->name), name);

    pSFlashRecord->status = SFLASH_RECORD_STATUS_DATA;
    
    pSFlashRecordEnd->checksum = getChecksum(data + sizeof(SFlashRecordHeader), SFLASH_RECORD_SIZE - sizeof(SFlashRecordHeader) - sizeof(SFlashRecordEnd) - sizeof(uint8_t), "<append>") ;
  
    
    
    
#if(SFLASH_DEBUG)
    SFlashPrintfRecord(data);
    printfBuffData("SFlashAppendRecord", data, SFLASH_RECORD_SIZE);
#endif
    
    targetAddress = getNextUseableRecordAddress();
    targetRecordIndex = targetAddress/SFLASH_RECORD_SIZE;
    //sysprintf(" [!! INFORMATION !!] SFlashAppendRecord targetAddress = 0x%04x, targetRecordIndex = %d \r\n", targetAddress, targetRecordIndex);

    for(int i = 0; i<SFLASH_RECORD_PAGE_SIZE_PER_RECORD; i++)
    {
        FlashDrvPageProgram(SPI_FLASH_EX_0_INDEX, targetAddress + SPI_FLASH_EX_PAGE_SIZE*i, data + SPI_FLASH_EX_PAGE_SIZE*i, SPI_FLASH_EX_PAGE_SIZE);
    }
    currentStatus[targetRecordIndex] = SFLASH_RECORD_STATUS_DATA;
    //sysprintf(" [!! INFORMATION !!]  SFlashAppendRecord [%s](type:%d): record index %d (Data Index = %d) OK [headerRecordIndex = %d : endRecordIndex = %d]\r\n", name, type, targetRecordIndex, pSFlashRecord->index, headerRecordIndex, endRecordIndex);
    return TRUE;
}
int SFlashGetRecord(uint8_t** targetData, size_t* targetDataLen, char* fileName, uint8_t* type)
{
    uint32_t targetAddress;
    uint8_t data[SFLASH_RECORD_SIZE];
    uint8_t* targetDataTmp;
    size_t targetDataLenTmp;
    SFlashRecord* pSFlashRecord = (SFlashRecord*)data;
    while(1)
    {
        targetAddress = getNextDataRecordAddress();
        //sysprintf("SFlashGetRecord targetAddress = 0x%04x \r\n", targetAddress);
        if(targetAddress != 0xffffffff)
        {
            getEntireRecord(SPI_FLASH_EX_0_INDEX, targetAddress/SFLASH_RECORD_SIZE, data, sizeof(data));  
            //process
            SFlashPrintfRecord(data);
            if(checkEntireRecord(data, &targetDataTmp, &targetDataLenTmp))
            {
                strcpy(fileName, (char*)pSFlashRecord->name);
                *type = pSFlashRecord->type;
                *targetData =  pvPortMalloc(targetDataLenTmp);
                memcpy(*targetData, targetDataTmp, targetDataLenTmp);
                *targetDataLen = targetDataLenTmp;
                //sysprintf(" [!! INFORMATION !!]  SFlashGetRecord : record index [ %d ] OK\r\n", targetAddress/SFLASH_RECORD_SIZE);
                return targetAddress/SFLASH_RECORD_SIZE;
            }  
            else
            {
                //sysprintf("SFlashGetRecord checkEntireRecord error [%d:%d]: Mark it to BAD \r\n", headerRecordIndex, endRecordIndex);
                makeRecordStatus(SPI_FLASH_EX_0_INDEX, targetAddress/SFLASH_RECORD_SIZE, SFLASH_RECORD_STATUS_BAD);
                currentStatus[targetAddress/SFLASH_RECORD_SIZE] = SFLASH_RECORD_STATUS_BAD;
            }
        }
        else
        {
            //sysprintf("SFlashGetRecord empty, ignore\r\n");  
            break;        
        }
    }
    return -1;
}

void SFlashMarkRecordStatus(int recordIndex, uint8_t status)
{
    //sysprintf(" [!! INFORMATION !!]  SFlashMarkRecordStatus : Mark record index [ %d ], status = 0x%02x \r\n", recordIndex, status);
    endRecordIndex = recordIndex + 1;
    endRecordIndex = endRecordIndex % SFLASH_RECORD_TOTAL_NUM;
    
    currentStatus[recordIndex] = status;
    makeRecordStatus(SPI_FLASH_EX_0_INDEX, recordIndex, status);
}


BOOL SFlashSaveStorage(int storageId, uint8_t* dataSrc, int dataSrcLen)
{
    uint8_t data[SFLASH_STORAGE_SIZE];
    uint32_t targetAddress = (storageId * SFLASH_STORAGE_SIZE) + SFLASH_STORAGE_START_ADDRESS;
    
    SFlashStorage* pSFlashStorage = (SFlashStorage*)data;
    SFlashStorageHeader* pSFlashStorageHeader = (SFlashStorageHeader*)&(pSFlashStorage->header); 
    SFlashStorageEnd* pSFlashStorageEnd = (SFlashStorageEnd*)&(pSFlashStorage->end); 
    
    memset(data, 0x00, SFLASH_STORAGE_SIZE);
    if(storageId >= SFLASH_STORAGE_TOTAL_NUM)
    {
        sysprintf(" [!! INFORMATION !!] SFlashSaveStorage exceed storageId (%d >= %d) = 0x%04x \r\n", storageId, SFLASH_STORAGE_TOTAL_NUM);
        return FALSE;
    }
    if(dataSrcLen > SFLASH_STORAGE_DATA_LEN)
    {
        dataSrcLen = SFLASH_STORAGE_DATA_LEN;
    }
    
    pSFlashStorageHeader->value[0] = SFLASH_STORAGE_HEADER_VALUE;
    pSFlashStorageHeader->value[1] = SFLASH_STORAGE_HEADER_VALUE2;
    pSFlashStorageHeader->Len = dataSrcLen;  

    //memcpy(pSFlashRecord->data + sizeof(SFlashRecordHeader), dataSrc, dataSrcLen);
    memcpy(pSFlashStorage->data, dataSrc, dataSrcLen);

    
    pSFlashStorageEnd->value[0] = SFLASH_STORAGE_END_VALUE;
    pSFlashStorageEnd->value[1] = SFLASH_STORAGE_END_VALUE2;    
    pSFlashStorageEnd->checksum = getChecksum(data + sizeof(SFlashStorageHeader), SFLASH_STORAGE_SIZE - sizeof(SFlashStorageHeader) - sizeof(SFlashStorageEnd) - sizeof(uint8_t), "<SaveStorage>") ;
    
    
    //sysprintf(" [!! INFORMATION !!] SFlashSaveStorage(storageId = %d, dataSrcLen = %d) targetAddress = 0x%04x \r\n", storageId, dataSrcLen, targetAddress);
    FlashDrvSectorErase(SPI_FLASH_EX_0_INDEX, targetAddress);
    for(int i = 0; i<SFLASH_STORAGE_PAGE_SIZE_PER_RECORD; i++)
    {
        FlashDrvPageProgram(SPI_FLASH_EX_0_INDEX, targetAddress + SPI_FLASH_EX_PAGE_SIZE*i, data + SPI_FLASH_EX_PAGE_SIZE*i, SPI_FLASH_EX_PAGE_SIZE);
    }
    memset(data, 0x00, SFLASH_STORAGE_SIZE);
    FlashDrvNormalRead(SPI_FLASH_EX_0_INDEX, targetAddress, data, SFLASH_STORAGE_SIZE);
    if(memcmp(pSFlashStorage->data, dataSrc, dataSrcLen) == 0)
    {
        sysprintf(" [!! INFORMATION !!] SFlashSaveStorage(storageId = %d)  targetAddress = 0x%04x SUCCESS \r\n", storageId, targetAddress);
        return TRUE;
    }
    else
    {
        sysprintf(" [!! INFORMATION !!] SFlashSaveStorage(storageId = %d)  targetAddress = 0x%04x ERROR \r\n", storageId, targetAddress);
        return FALSE;
    }
    
    
}

BOOL SFlashLoadStorage(int storageId, uint8_t* dataSrc, int dataSrcLen)
{
    uint8_t data[SFLASH_STORAGE_SIZE];
    uint32_t targetAddress = (storageId * SFLASH_STORAGE_SIZE) + SFLASH_STORAGE_START_ADDRESS;
    
    SFlashStorage* pSFlashStorage = (SFlashStorage*)data;
    SFlashStorageHeader* pSFlashStorageHeader = (SFlashStorageHeader*)&(pSFlashStorage->header); 
    SFlashStorageEnd* pSFlashStorageEnd = (SFlashStorageEnd*)&(pSFlashStorage->end); 
    
    memset(data, 0x00, SFLASH_STORAGE_SIZE);
        
    //sysprintf(" [!! INFORMATION !!] SFlashLoadStorage(storageId = %d, dataSrcLen = %d) targetAddress = 0x%04x \r\n", storageId, dataSrcLen, targetAddress);
    
    FlashDrvNormalRead(SPI_FLASH_EX_0_INDEX, targetAddress, data, SFLASH_STORAGE_SIZE);
    
    if(dataSrcLen > SFLASH_STORAGE_DATA_LEN)
    {
        dataSrcLen = SFLASH_STORAGE_DATA_LEN;
    }
    
    if((pSFlashStorageHeader->value[0] == SFLASH_STORAGE_HEADER_VALUE) && 
        (pSFlashStorageHeader->value[1] == SFLASH_STORAGE_HEADER_VALUE2))
            //&& pSFlashStorageHeader->Len == (srcDataLen-sizeof(SFlashStorageHeader) - sizeof(SFlashStorageEnd))) )       
    {
            //uint16_t checkSum = getChecksum(srcData + sizeof(SFlashStorageHeader), pSFlashStorageHeader->Len, "<parser>") ; 
            uint16_t checkSum = getChecksum(data + sizeof(SFlashStorageHeader), SFLASH_STORAGE_SIZE - sizeof(SFlashStorageHeader) - sizeof(SFlashStorageEnd) - sizeof(uint8_t), "<LoadStorage>") ; 
            
            if((checkSum == pSFlashStorageEnd->checksum)  &&
                (pSFlashStorageEnd->value[0] == SFLASH_STORAGE_END_VALUE) && 
                (pSFlashStorageEnd->value[1] == SFLASH_STORAGE_END_VALUE2))
            {
                memcpy(dataSrc, pSFlashStorage->data, dataSrcLen);
                return TRUE;
            }
            else
            {
                terninalPrintf("\r\n checkEntireStorage End ERROR [0x%02x, 0x%02x] checkSum = %d (%d)...\r\n", pSFlashStorageEnd->value[0], pSFlashStorageEnd->value[1], pSFlashStorageEnd->checksum, checkSum);
                #if(0)
                printfBuffData("checkEntireStorage", dataSrc, SFLASH_STORAGE_SIZE);
                #endif
            }
    }
    else
    {
        terninalPrintf("\r\n checkEntireStorage Header ERROR [0x%02x, 0x%02x] ...\r\n", pSFlashStorageHeader->value[0], pSFlashStorageHeader->value[1]);
        printfBuffData("checkEntireStorage", dataSrc, SFLASH_STORAGE_SIZE);
    }
    
    
    
    
    
    return FALSE;
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


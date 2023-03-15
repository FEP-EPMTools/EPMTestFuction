/**************************************************************************//**
* @file     sflashrecord.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SFLASH_RECORD_H__
#define __SFLASH_RECORD_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"

#include "flashdrvex.h"

#ifdef __cplusplus
extern "C"
{
#endif
   
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//|-------------  RAW:2MB  ------------------|//
//|-- RECORD:1MB --|-- STORAGE: 16k--|--??---|//
    
//RECORD DATA
#define SFLASH_RECORD_PAGE_SIZE_PER_RECORD  4 //4 page per record
#define SFLASH_RECORD_NUM_PER_SECTOR        (SPI_FLASH_EX_PAGE_NUM_PER_SECTOR / SFLASH_RECORD_PAGE_SIZE_PER_RECORD) //4 record per sector   

#define SFLASH_RECORD_START_PAGE            SPI_FLASH_EX_RAW_START_PAGE   //起始 page
#define SFLASH_RECORD_START_ADDRESS         (SFLASH_RECORD_START_PAGE * SPI_FLASH_EX_PAGE_SIZE)   //起始位址 
#define SFLASH_RECORD_TOTAL_PAGE_SIZE       4096  //用掉多少 page, 4096*256 = 1024k = 1MB, max value: SPI_FLASH_EX_RAW_PAGE_SIZE
#define SFLASH_RECORD_LAST_PAGE             (SFLASH_RECORD_START_PAGE + SFLASH_RECORD_TOTAL_PAGE_SIZE - 1)   //最後 page

#define SFLASH_RECORD_TOTAL_NUM             (SFLASH_RECORD_TOTAL_PAGE_SIZE / SFLASH_RECORD_PAGE_SIZE_PER_RECORD)  //可以有幾筆 record: 4096/4 = 1000
#define SFLASH_RECORD_SIZE                  (SPI_FLASH_EX_PAGE_SIZE * SFLASH_RECORD_PAGE_SIZE_PER_RECORD) //每筆record大小: 256*4 = 1024byte

//storage data
#define SFLASH_STORAGE_PAGE_SIZE_PER_RECORD (SPI_FLASH_EX_SECTOR_SIZE / SPI_FLASH_EX_PAGE_SIZE) //1 sector = 16 pages   per record (4K)
    
#define SFLASH_STORAGE_START_PAGE           (SFLASH_RECORD_LAST_PAGE+1)   //起始 page
#define SFLASH_STORAGE_START_ADDRESS        (SFLASH_STORAGE_START_PAGE * SPI_FLASH_EX_PAGE_SIZE)   //起始位址 
#define SFLASH_STORAGE_TOTAL_PAGE_SIZE      (16*4)  //用掉多少 page, 16*4*256 = 16k
#define SFLASH_STORAGE_LAST_PAGE            (SFLASH_STORAGE_START_PAGE + SFLASH_STORAGE_TOTAL_PAGE_SIZE)   //最後 page

#define SFLASH_STORAGE_TOTAL_NUM             (SFLASH_STORAGE_TOTAL_PAGE_SIZE / SFLASH_STORAGE_PAGE_SIZE_PER_RECORD)  //可以有幾筆 record: (16*4)/16 = 4
#define SFLASH_STORAGE_SIZE                  (SPI_FLASH_EX_PAGE_SIZE * SFLASH_STORAGE_PAGE_SIZE_PER_RECORD) //每筆record大小: 256*16 = 4096byte

//---
#if(SPI_FLASH_EX_RAW_PAGE_SIZE < (SFLASH_RECORD_TOTAL_PAGE_SIZE + SFLASH_STORAGE_TOTAL_PAGE_SIZE))
    #error   (SPI_FLASH_EX_RAW_PAGE_SIZE < (SFLASH_RECORD_TOTAL_PAGE_SIZE + SFLASH_STORAGE_TOTAL_PAGE_SIZE))
#endif    
   
#define SFLASH_STORAGE_EPM_SERIAL_ID_INDEX_BASE  0

#define SFLASH_STORAGE_SPACE_EX_DATA_INDEX_BASE  1
    


//#if (ENABLE_MTP_FUNCTION)
#define SFLASH_STORAGE_EPM_SERIAL_ID_INDEX  0
#define SFLASH_STORAGE_EPM_GUID_INDEX  1
//#endif    
    
//--record flag----    
#define SFLASH_RECORD_STATUS_EMPTY          0xFF    // 0b 1111 1111 //no data
#define SFLASH_RECORD_STATUS_DATA           0xFE    // 0b 1111 1110 //data, but not send yet
#define SFLASH_RECORD_STATUS_BACKUP         0xFC    // 0b 1111 1100 //data, already send to server
#define SFLASH_RECORD_STATUS_BAD            0x00    // 0b 0000 0000 //bad data
    
#define SFLASH_RECORD_NAME_LEN              64//32  

#define SFLASH_RECORD_TYPE_ECC              0x01  
#define SFLASH_RECORD_TYPE_IPASS            0x02
#define SFLASH_RECORD_TYPE_TRANSACTION      0x03

#define SFLASH_HEADER_VALUE             0xda   
#define SFLASH_HEADER_VALUE2		    0xad  
    
#define SFLASH_END_VALUE                0x1f   
#define SFLASH_END_VALUE2		        0xf1  

#define SFLASH_RECORD_DATA_LEN              (SFLASH_RECORD_SIZE - SFLASH_RECORD_NAME_LEN*sizeof(uint8_t) - sizeof(uint8_t) - sizeof(uint32_t) - sizeof(uint8_t) - sizeof(SFlashRecordHeader) - sizeof(SFlashRecordEnd))
    
#pragma pack(1)
typedef struct
{
    uint8_t         value[2];    //2 byte
    size_t          Len;
}SFlashRecordHeader; //9 bytes

typedef struct
{
    uint16_t    checksum; 
    uint8_t     value[2];     //2 byte
}SFlashRecordEnd; //4 bytes

typedef struct
{
    SFlashRecordHeader      header;
    uint8_t                 data[SFLASH_RECORD_DATA_LEN];
    uint8_t                 name[SFLASH_RECORD_NAME_LEN];
    uint8_t                 type;  
    int32_t                 index;
    uint8_t                 status;
    SFlashRecordEnd         end;
}SFlashRecord;  //SFLASH_RECORD_SIZE  //256*4 = 1024byte

//--record flag----  
#define SFLASH_STORAGE_HEADER_VALUE            0xdb   
#define SFLASH_STORAGE_HEADER_VALUE2		    0xbd  
    
#define SFLASH_STORAGE_END_VALUE        0x2f   
#define SFLASH_STORAGE_END_VALUE2		0xf2  

#define SFLASH_STORAGE_DATA_LEN              (SFLASH_STORAGE_SIZE - sizeof(SFlashStorageHeader) - sizeof(SFlashStorageEnd))

typedef struct
{
    uint8_t         value[2];    //2 byte
    size_t          Len;
}SFlashStorageHeader; //9 bytes

typedef struct
{
    uint16_t    checksum; 
    uint8_t     value[2];     //2 byte
}SFlashStorageEnd; //4 bytes

typedef struct
{
    SFlashStorageHeader     header;
    uint8_t                 data[SFLASH_STORAGE_DATA_LEN];
    SFlashStorageEnd        end;
}SFlashStorage;    //4096 byte

//checksum: data[SFLASH_RECORD_DATA_LEN] ~ index
#pragma pack()

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL SFlashRecordInit(void);
void SFlashListRecord(uint8_t flashIndex);
BOOL SFlashAppendRecord(char* name, uint8_t type, uint8_t* dataSrc, int dataSrcLen);
int  SFlashGetRecord(uint8_t** targetData, size_t* targetDataLen, char* fileName, uint8_t* type);
void SFlashMarkRecordStatus(int recordIndex, uint8_t status);

BOOL SFlashSaveStorage(int storageId, uint8_t* dataSrc, int dataSrcLen);
BOOL SFlashLoadStorage(int storageId, uint8_t* dataSrc, int dataSrcLen);

#ifdef __cplusplus
}
#endif

#endif //__SFLASH_RECORD_H__

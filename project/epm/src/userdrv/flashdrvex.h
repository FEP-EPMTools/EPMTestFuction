/**************************************************************************//**
* @file     flashdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __FLASH_DRV_EX_H__
#define __FLASH_DRV_EX_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//page 256 bytes    (program/write uint)
//sector 4K bytes = 16 pages (erase unit)
//block 32k/64k = 8/16 sectors
//64Mb = 8MB = 2048 sectors = 32768 pages // half: 16384 pages = 8192 record

#define SPI_FLASH_EX_TOTAL_SIZE            (8*1024*1024)
#define SPI_FLASH_EX_PAGE_SIZE             256
#define SPI_FLASH_EX_SECTOR_SIZE           (4*1024)
#define SPI_FLASH_EX_PAGE_NUM_PER_SECTOR   (SPI_FLASH_EX_SECTOR_SIZE / SPI_FLASH_EX_PAGE_SIZE)//16
// |----------------------- 8MB -------------------|
// |---  RAW:3MB  --|----------  FS:5MB   ---------|

//=============
//use it to set serial-record size.
#define SPI_FLASH_EX_RAW_START_ADDRESS      0
//#define SPI_FLASH_EX_RAW_START_ADDRESS      SPI_FLASH_EX_SECTOR_SIZE//0
#define SPI_FLASH_EX_RAW_START_PAGE         (SPI_FLASH_EX_RAW_START_ADDRESS/SPI_FLASH_EX_PAGE_SIZE)  // 0
#define SPI_FLASH_EX_RAW_START_SECTOR       (SPI_FLASH_EX_RAW_START_ADDRESS/SPI_FLASH_EX_SECTOR_SIZE)  //0
#define SPI_FLASH_EX_RAW_END_PAGE           (SPI_FLASH_EX_FS_START_PAGE - 1)
#define SPI_FLASH_EX_RAW_END_SECTOR         (SPI_FLASH_EX_FS_START_SECTOR - 1)
#define SPI_FLASH_EX_RAW_PAGE_SIZE          (SPI_FLASH_EX_RAW_END_PAGE - SPI_FLASH_EX_RAW_START_PAGE + 1)
#define SPI_FLASH_EX_RAW_SECTOR_SIZE        (SPI_FLASH_EX_RAW_END_SECTOR - SPI_FLASH_EX_RAW_START_SECTOR + 1)
    
//=============
//use it to set file system size.
//#define SPI_FLASH_EX_FS_TOTAL_SIZE         (4*1024*1024)   //4MB
#define SPI_FLASH_EX_FS_TOTAL_SIZE         (5*1024*1024)   //5MB
//#define SPI_FLASH_EX_FS_TOTAL_SIZE         (8*1024*1024)  //8MB
//------------
#define SPI_FLASH_EX_FS_START_ADDRESS       (SPI_FLASH_EX_TOTAL_SIZE - SPI_FLASH_EX_FS_TOTAL_SIZE)
//#define SPI_FLASH_EX_FS_START_ADDRESS       0
#define SPI_FLASH_EX_FS_START_PAGE          (SPI_FLASH_EX_FS_START_ADDRESS/SPI_FLASH_EX_PAGE_SIZE)
#define SPI_FLASH_EX_FS_START_SECTOR        (SPI_FLASH_EX_FS_START_ADDRESS/SPI_FLASH_EX_SECTOR_SIZE)
#define SPI_FLASH_EX_FS_END_PAGE            (SPI_FLASH_EX_FS_START_PAGE + SPI_FLASH_EX_FS_TOTAL_SIZE/SPI_FLASH_EX_PAGE_SIZE)
#define SPI_FLASH_EX_FS_END_SECTOR          (SPI_FLASH_EX_FS_START_SECTOR + SPI_FLASH_EX_FS_TOTAL_SIZE/SPI_FLASH_EX_SECTOR_SIZE)


    

#define SPI_FLASH_EX_0_INDEX  1
#define SPI_FLASH_EX_1_INDEX  2

#define SPI_FLASH_EX_2_INDEX  3
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL FlashDrvExInit(BOOL testModeFlag);

BOOL FlashDrvExInitialize(uint8_t pdrv);
BOOL FlashDrvExStatus(uint8_t pdrv);
BOOL FlashDrvExRead(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count);
BOOL FlashDrvExWrite(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count);
BOOL FlashDrvExIoctl(uint8_t pdrv, uint8_t cmd, void *buff );
void FlashDrvExChipEraseFs(uint8_t flashIndex);
void FlashDrvExChipEraseFull(uint8_t flashIndex);
void FlashDrvExChipErasePure(uint8_t flashIndex, int startSector, int endSector);
uint16_t FlashDrvExGetChipID(uint8_t flashIndex);

void FlashDrvPageProgram(uint8_t flashIndex, uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen);
void FlashDrvSectorErase(uint8_t flashIndex, uint32_t StartAddress);
void FlashDrvNormalRead(uint8_t flashIndex, uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen);

#ifdef __cplusplus
}
#endif

#endif //__FLASH_DRV_EX_H__

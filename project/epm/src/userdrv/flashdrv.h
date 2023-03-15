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

#ifndef __FLASH_DRV_H__
#define __FLASH_DRV_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define SPI_FLASH_START_ADDRESS         (15*1024*1024) //15MB

#define SPI_FLASH_PAGE_SIZE             256
#define SPI_FLASH_SECTOR_SIZE           (4*1024)
#define SPI_FLASH_PAGE_NUM_PER_SECTOR   (SPI_FLASH_SECTOR_SIZE / SPI_FLASH_PAGE_SIZE)

#define SPI_FLASH_DATA_HEADER           0x2a
#define SPI_FLASH_DATA_HEADER_2         0xa2
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL FlashDrvInit(void);
BOOL FlashRead(uint8_t *u8DataBuffer, int BuffLen);    
BOOL FlashWrite(uint8_t *u8DataBuffer, int BuffLen);
#ifdef __cplusplus
}
#endif

#endif //__FLASH_DRV_H__

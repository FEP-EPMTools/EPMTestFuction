/**************************************************************************//**
* @file     flashdrv.c
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
#include "adc.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "flashdrv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define FLASH_SPI  SPI_0_INTERFACE_INDEX
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static SpiInterface* pSpiInterface;

//page 256 bytes
//sector 4K bytes = 16 pages
//block 32k/64k = 8/16 sectors
static uint8_t bufftemp[SPI_FLASH_SECTOR_SIZE];
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
/*
static void SpiFlash_ChipErase(void)
{
    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x06, Write enable
    pSpiInterface->writeFunc(0, 0x06);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);

    //////////////////////////////////////////

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0xC7, Chip Erase
    pSpiInterface->writeFunc(0, 0xC7);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);
}
*/
static void SpiFlash_SectorErase(uint32_t StartAddress)
{
    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x06, Write enable
    pSpiInterface->writeFunc(0, 0x06);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);

    //////////////////////////////////////////

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x20, Sector Erase
    pSpiInterface->writeFunc(0, 0x20);
    
    
    // send 24-bit start address
    pSpiInterface->writeFunc(0, (StartAddress>>16) & 0xFF);

    pSpiInterface->writeFunc(0, (StartAddress>>8) & 0xFF);

    pSpiInterface->writeFunc(0, StartAddress & 0xFF);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);
}

static uint8_t SpiFlash_ReadStatusReg(void)
{
    uint8_t u8Status;

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x05, Read status register
    pSpiInterface->writeFunc(0, 0x05);

    // read status
    pSpiInterface->writeFunc(0, 0x00);

    u8Status = pSpiInterface->readFunc(0);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);

    return u8Status;
}

static void SpiFlash_WaitReady(void)
{
    uint8_t ReturnValue;

    do {
        ReturnValue = SpiFlash_ReadStatusReg();
        ReturnValue = ReturnValue & 1;
    } while(ReturnValue!=0); // check the BUSY bit
}

static void SpiFlash_NormalPageProgram(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i = 0;

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x06, Write enable
    pSpiInterface->writeFunc(0, 0x06);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);


    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x02, Page program
    pSpiInterface->writeFunc(0, 0x02);

    // send 24-bit start address
    pSpiInterface->writeFunc(0, (StartAddress>>16) & 0xFF);

    pSpiInterface->writeFunc(0, (StartAddress>>8) & 0xFF);

    pSpiInterface->writeFunc(0, StartAddress & 0xFF);

    // write data
    for(i=0; i<BuffLen; i++) {
        pSpiInterface->writeFunc(0, u8DataBuffer[i]);
    }

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);
}

static void SpiFlash_NormalRead(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i;

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x03, Read data
    pSpiInterface->writeFunc(0, 0x03);

    // send 24-bit start address
    pSpiInterface->writeFunc(0, (StartAddress>>16) & 0xFF);

    pSpiInterface->writeFunc(0, (StartAddress>>8) & 0xFF);

    pSpiInterface->writeFunc(0, StartAddress & 0xFF);

    // read data
    for(i=0; i<BuffLen; i++) {
        pSpiInterface->writeFunc(0, 0x00);
        u8DataBuffer[i] = pSpiInterface->readFunc(0);
    }
    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);
}

static uint16_t SpiFlash_ReadMidDid(void)
{
    uint8_t u8RxData[2];

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0x90, Read Manufacturer/Device ID
    pSpiInterface->writeFunc(0, 0x90);

    // send 24-bit '0', dummy
    pSpiInterface->writeFunc(0, 0x00);

    pSpiInterface->writeFunc(0, 0x00);

    pSpiInterface->writeFunc(0, 0x00);

    // receive 16-bit
    pSpiInterface->writeFunc(0, 0x00);
    u8RxData[0] = pSpiInterface->readFunc(0);

    pSpiInterface->writeFunc(0, 0x00);
    u8RxData[1] = pSpiInterface->readFunc(0);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);

    return ( (u8RxData[0]<<8) | u8RxData[1] );
}

static BOOL hwInit(void)
{

    return TRUE;
}
static BOOL swInit(void)
{   
    uint16_t u16ID;
    // check flash id
    if((u16ID = SpiFlash_ReadMidDid()) != 0xEF17) 
    {
        sysprintf("Flash Wrong ID, 0x%x\n", u16ID);
        return FALSE;
    } 
    else
    {
        sysprintf("Flash found: W25Q128BV ...\n");
        return TRUE;
    }
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL FlashDrvInit(void)
{
    sysprintf("FlashDrvInit!!\n");
    pSpiInterface = SpiGetInterface(FLASH_SPI);
    if(pSpiInterface == NULL)
    {
        sysprintf("EpdInit ERROR (pSpiInterface == NULL)!!\n");
        return FALSE;
    }
    if(pSpiInterface->initFunc() == FALSE)
    {
        sysprintf("EpdInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(hwInit() == FALSE)
    {
        sysprintf("FlashDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("FlashDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}

BOOL FlashRead(uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i;
    memset(bufftemp, 0x0, sizeof(bufftemp));
    SpiFlash_NormalRead(SPI_FLASH_START_ADDRESS, bufftemp, sizeof(bufftemp));
    for(i = 0; i< SPI_FLASH_SECTOR_SIZE; i = i + SPI_FLASH_PAGE_SIZE)
    {
        sysprintf("FlashRead: check address 0x%08x [0x%02x, 0x%02x]... \r\n", i, bufftemp[i], bufftemp[i+1]); 
        if((bufftemp[i] == SPI_FLASH_DATA_HEADER) && (bufftemp[i+1] == SPI_FLASH_DATA_HEADER_2))
        {
            if(BuffLen > (SPI_FLASH_PAGE_SIZE - 2))
            {
                sysprintf("FlashRead: get data at address 0x%08x [len = %d]... \r\n", i, SPI_FLASH_PAGE_SIZE - 2); 
                memcpy(u8DataBuffer, (bufftemp+i+2), SPI_FLASH_PAGE_SIZE - 2);
            }
            else
            {
                sysprintf("FlashRead: get data at address 0x%08x [len = %d]... \r\n", i, BuffLen); 
                memcpy(u8DataBuffer, (bufftemp+i+2), BuffLen);
            }
            return TRUE;
        }
    }
    sysprintf("FlashRead: cant get data, erase sector...\r\n"); 
    SpiFlash_SectorErase(SPI_FLASH_START_ADDRESS);
    //SpiFlash_ChipErase();
     /* Wait ready */
    SpiFlash_WaitReady();
    return FALSE;
}
BOOL FlashWrite(uint8_t *u8DataBuffer, int BuffLen)
{
    BOOL reval = FALSE;
    uint32_t i;
    int targetLen;
    uint8_t pageTemp[SPI_FLASH_PAGE_SIZE], targetPageTemp[SPI_FLASH_PAGE_SIZE];
    memset(bufftemp, 0x0, sizeof(bufftemp));
    SpiFlash_NormalRead(SPI_FLASH_START_ADDRESS, bufftemp, sizeof(bufftemp));
    for(i = 0; i< SPI_FLASH_SECTOR_SIZE; i = i + SPI_FLASH_PAGE_SIZE)
    {
        sysprintf("FlashWrite: check address 0x%08x [0x%02x, 0x%02x]... \r\n", i, bufftemp[i], bufftemp[i+1]); 
        if((bufftemp[i] == SPI_FLASH_DATA_HEADER) && (bufftemp[i+1] == SPI_FLASH_DATA_HEADER_2))
        {            
            sysprintf("FlashWrite: get data at address 0x%08x, break... \r\n", i);             
            break;
        }
    }
    if(i >= SPI_FLASH_SECTOR_SIZE)
    {//§ä¤£¨ìheader
        i = 0;//SPI_FLASH_PAGE_SIZE;
        memset(targetPageTemp, 0x0, sizeof(targetPageTemp));
    }
    else
    {
        memcpy(targetPageTemp, bufftemp+i, sizeof(targetPageTemp));
    }
    
    if(BuffLen > (SPI_FLASH_PAGE_SIZE - 2))
    {
        targetLen = SPI_FLASH_PAGE_SIZE - 2;
    }
    else
    {
        targetLen = BuffLen;
    }
    targetPageTemp[0] = SPI_FLASH_DATA_HEADER;
    targetPageTemp[1] = SPI_FLASH_DATA_HEADER_2;
    memcpy((targetPageTemp+2), u8DataBuffer, targetLen);
    /*
    {
        int j;
        for(j = 0; j<10; j++)
        {
            sysprintf("FlashWrite:dump page [%02d]: 0x%02x...\r\n", j, u8DataBuffer[j]); 
        }
    }
    */
    while( i < SPI_FLASH_SECTOR_SIZE)
    {
        sysprintf("FlashWrite: write data at address 0x%08x [len = %d]... \r\n", SPI_FLASH_START_ADDRESS + i, targetLen);     
        
        sysprintf("FlashWrite:erase sector...\r\n"); 
        SpiFlash_SectorErase(SPI_FLASH_START_ADDRESS);
        //SpiFlash_ChipErase();
         /* Wait ready */
        SpiFlash_WaitReady();
        sysprintf("FlashWrite:write page...\r\n"); 
        /*
        {
            int j;
            for(j = 0; j<10; j++)
            {
                sysprintf("FlashWrite:dump2 page [%02d]: 0x%02x...\r\n", j, targetPageTemp[j]); 
            }
        }
        */
        SpiFlash_NormalPageProgram(SPI_FLASH_START_ADDRESS + i, targetPageTemp, SPI_FLASH_PAGE_SIZE);  
         /* Wait ready */
        SpiFlash_WaitReady();
        SpiFlash_NormalRead(SPI_FLASH_START_ADDRESS + i, pageTemp, SPI_FLASH_PAGE_SIZE);
        if(memcmp(targetPageTemp, pageTemp, SPI_FLASH_PAGE_SIZE) == 0)
        {
            sysprintf("FlashWrite: compare OK...break...\r\n"); 
            reval = TRUE;
            break;
            //i = i + SPI_FLASH_PAGE_SIZE;
        }
        else
        {
            sysprintf("FlashWrite: compare error...\r\n"); 
            i = i + SPI_FLASH_PAGE_SIZE;
        }
    }
    
    return reval;
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


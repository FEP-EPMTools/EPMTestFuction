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
#include "flashdrvex.h"
#include "interface.h"

#include "ff.h"
#include "diskio.h"
#include "loglib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

#define SHIFT_SECTOR_INDEX      SPI_FLASH_EX_FS_START_SECTOR
#define FLASH_SPI               SPI_0_INTERFACE_INDEX

//SPI_FLASH_EX_2_INDEX GPI14 CS pin
#define SPI_FLASH_EX_2_CS_PORT  GPIOI
#define SPI_FLASH_EX_2_CS_PIN   BIT14



//SPI_FLASH_EX_1_INDEX GPI13 CS pin
#define SPI_FLASH_EX_1_CS_PORT  GPIOI
#define SPI_FLASH_EX_1_CS_PIN   BIT13


//SPI_FLASH_EX_0_INDEX GPI11 WP pin
#define SPI_FLASH_EX_0_WP_PORT  GPIOI
#define SPI_FLASH_EX_0_WP_PIN   BIT11
//SPI_FLASH_EX_0_INDEX GPI12 HD pin
#define SPI_FLASH_EX_0_HD_PORT  GPIOI
#define SPI_FLASH_EX_0_HD_PIN   BIT12

#if(SUPPORT_HK_10_HW)
    #warning for temp old reader board
    //SPI_FLASH_EX_1_INDEX GPI4 WP pin
    #define SPI_FLASH_EX_1_WP_PORT  GPIOI
    #define SPI_FLASH_EX_1_WP_PIN   BIT4
#else
    //SPI_FLASH_EX_1_INDEX GPI14 WP pin
    #define SPI_FLASH_EX_1_WP_PORT  GPIOI
    #define SPI_FLASH_EX_1_WP_PIN   BIT14
#endif
////SPI_FLASH_EX_1_INDEX GPI14 WP pin
//#define SPI_FLASH_EX_1_WP_PORT  GPIOI
//#define SPI_FLASH_EX_1_WP_PIN   BIT14
    
//SPI_FLASH_EX_1_INDEX GPI15 HD pin
#define SPI_FLASH_EX_1_HD_PORT  GPIOI
#define SPI_FLASH_EX_1_HD_PIN   BIT15


/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static BOOL initFlag = FALSE;
static SpiInterface* pSpiInterface = NULL;

static SemaphoreHandle_t xSemaphore;

static BOOL existFlag[2];
static int errorTimes[2];



FATFS  _FatfsVolSFlash;

//static int blockSize[2];

uint8_t serialFlashBuff[5*SPI_FLASH_EX_SECTOR_SIZE];

//static TCHAR  _Path[3] = { '1', ':', 0 };
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void setFlashCS(uint8_t flashIndex, BOOL flag)
{
    if(flashIndex == SPI_FLASH_EX_0_INDEX)
    {
        if(flag)
        {
            //GPIO_SetBit(SPI_FLASH_EX_0_HD_PORT, SPI_FLASH_EX_0_HD_PIN);
            pSpiInterface->activeCSFunc(flag);
        }
        else
        {
            pSpiInterface->activeCSFunc(flag);
            //GPIO_ClrBit(SPI_FLASH_EX_0_HD_PORT, SPI_FLASH_EX_0_HD_PIN);
        }
        
    }
    else if(flashIndex == SPI_FLASH_EX_1_INDEX)
    {
        if(flag)
        {
            //GPIO_SetBit(SPI_FLASH_EX_1_HD_PORT, SPI_FLASH_EX_1_HD_PIN);
            GPIO_ClrBit(SPI_FLASH_EX_1_CS_PORT, SPI_FLASH_EX_1_CS_PIN);
        }
        else
        {
            GPIO_SetBit(SPI_FLASH_EX_1_CS_PORT, SPI_FLASH_EX_1_CS_PIN);
            //GPIO_ClrBit(SPI_FLASH_EX_1_HD_PORT, SPI_FLASH_EX_1_HD_PIN);
        }
    }
    else if(flashIndex == SPI_FLASH_EX_2_INDEX)
    {
        if(flag)
        {
            GPIO_ClrBit(SPI_FLASH_EX_2_CS_PORT, SPI_FLASH_EX_2_CS_PIN);
        }
        else
        {
            GPIO_SetBit(SPI_FLASH_EX_2_CS_PORT, SPI_FLASH_EX_2_CS_PIN);
        }
    }
    
    
}

static void SpiFlash_SectorErase(uint8_t flashIndex, uint32_t StartAddress)
{
    // /CS: active
    setFlashCS(flashIndex, TRUE);

    // send Command: 0x06, Write enable
    pSpiInterface->writeFunc(0, 0x06);

    // /CS: de-active
    setFlashCS(flashIndex, FALSE);

    //////////////////////////////////////////

    // /CS: active
    setFlashCS(flashIndex, TRUE);

    // send Command: 0x20, Sector Erase
    pSpiInterface->writeFunc(0, 0x20);
    
    
    // send 24-bit start address
    pSpiInterface->writeFunc(0, (StartAddress>>16) & 0xFF);

    pSpiInterface->writeFunc(0, (StartAddress>>8) & 0xFF);

    pSpiInterface->writeFunc(0, StartAddress & 0xFF);

    // /CS: de-active
    setFlashCS(flashIndex, FALSE);
    //sysprintf(" !!!!! ERROR : SpiFlash_SectorErase %d [StartAddress: 0x%08X, setor: %d]...\r\n", flashIndex, StartAddress , StartAddress/SPI_FLASH_EX_SECTOR_SIZE); 
}

static uint8_t SpiFlash_ReadStatusReg(uint8_t flashIndex)
{
    uint8_t u8Status;

    // /CS: active
    setFlashCS(flashIndex, TRUE);

    // send Command: 0x05, Read status register
    pSpiInterface->writeFunc(0, 0x05);

    // read status
    pSpiInterface->writeFunc(0, 0x00);

    u8Status = pSpiInterface->readFunc(0);

    // /CS: de-active
    setFlashCS(flashIndex, FALSE);

    return u8Status;
}
static void SpiFlash_WaitReady(uint8_t flashIndex)
{
    int counter = 5000/10;
    uint8_t ReturnValue;

    do {
        //vTaskDelay(1/portTICK_RATE_MS); //add by sam
        #if(FREERTOS_USE_1000MHZ)
        vTaskDelay(1/portTICK_RATE_MS); 
        #error
        #else
        //terninalPrintf("_"); 
        
        vTaskDelay(10/portTICK_RATE_MS); 
        if(counter-- == 0)
        {
            terninalPrintf("\r\n #################################\r\n"); 
            terninalPrintf(    " ### SpiFlash_WaitReady  break ### \r\n"); 
            terninalPrintf(    " ################################# \r\n"); 
            break;  
        }            
        #endif
        ReturnValue = SpiFlash_ReadStatusReg(flashIndex);
        ReturnValue = ReturnValue & 1;
        
    } while(ReturnValue!=0); // check the BUSY bit
}

static void SpiFlash_NormalPageProgram(uint8_t flashIndex, uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i = 0;

    // /CS: active
    setFlashCS(flashIndex, TRUE);

    // send Command: 0x06, Write enable
    pSpiInterface->writeFunc(0, 0x06);

    // /CS: de-active
    setFlashCS(flashIndex, FALSE);


    // /CS: active
    setFlashCS(flashIndex, TRUE);

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
    setFlashCS(flashIndex, FALSE);
}
static void SpiFlash_NormalSectorProgram(uint8_t flashIndex, uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i = 0;
    for(i = 0; i < BuffLen; i = i + SPI_FLASH_EX_PAGE_SIZE)
    {
        SpiFlash_NormalPageProgram(flashIndex, StartAddress + i, u8DataBuffer + i, SPI_FLASH_EX_PAGE_SIZE);
        SpiFlash_WaitReady(flashIndex);
    }   
}
static void SpiFlash_NormalRead(uint8_t flashIndex, uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i;

    // /CS: active
    setFlashCS(flashIndex, TRUE);

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
    setFlashCS(flashIndex, FALSE);
}

static uint16_t SpiFlash_ReadMidDid(uint8_t flashIndex)
{
    uint8_t u8RxData[2];

    // /CS: active
    setFlashCS(flashIndex, TRUE);

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
    setFlashCS(flashIndex, FALSE);
    
    sysprintf("SpiFlash_ReadMidDid[0x%02X, 0x%02X]...\r\n", u8RxData[0] , u8RxData[1] ); 

    return ( (u8RxData[0]<<8) | u8RxData[1] );
}

static BOOL hwInit(void)
{
    
    //GPI14 FLASH1 CS pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(SPI_FLASH_EX_2_CS_PORT, SPI_FLASH_EX_2_CS_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(SPI_FLASH_EX_2_CS_PORT, SPI_FLASH_EX_2_CS_PIN);
    
    //GPI13 FLASH1 CS pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(SPI_FLASH_EX_1_CS_PORT, SPI_FLASH_EX_1_CS_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(SPI_FLASH_EX_1_CS_PORT, SPI_FLASH_EX_1_CS_PIN);
    
    //GPI11 FLASH0 WP pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(SPI_FLASH_EX_0_WP_PORT, SPI_FLASH_EX_0_WP_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(SPI_FLASH_EX_0_WP_PORT, SPI_FLASH_EX_0_WP_PIN);
    //GPI12 FLASH0 HD pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(SPI_FLASH_EX_0_HD_PORT, SPI_FLASH_EX_0_HD_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(SPI_FLASH_EX_0_HD_PORT, SPI_FLASH_EX_0_HD_PIN);
    
#if(SUPPORT_HK_10_HW)
    //GPI4 FLASH1 WP pin
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(SPI_FLASH_EX_1_WP_PORT, SPI_FLASH_EX_1_WP_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(SPI_FLASH_EX_1_WP_PORT, SPI_FLASH_EX_1_WP_PIN);
#else
    //GPI14 FLASH1 WP pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(SPI_FLASH_EX_1_WP_PORT, SPI_FLASH_EX_1_WP_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(SPI_FLASH_EX_1_WP_PORT, SPI_FLASH_EX_1_WP_PIN);
#endif
//    //GPI14 FLASH1 WP pin
//    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<24)) | (0x0<<24));
//    GPIO_OpenBit(SPI_FLASH_EX_1_WP_PORT, SPI_FLASH_EX_1_WP_PIN, DIR_OUTPUT, NO_PULL_UP);
//    GPIO_SetBit(SPI_FLASH_EX_1_WP_PORT, SPI_FLASH_EX_1_WP_PIN);
    
    //GPI15 FLASH1 HD pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(SPI_FLASH_EX_1_HD_PORT, SPI_FLASH_EX_1_HD_PIN, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(SPI_FLASH_EX_1_HD_PORT, SPI_FLASH_EX_1_HD_PIN);
    
    return TRUE;
}
static BOOL swInitPure(void)
{     
    if(xSemaphore == NULL)
    {
        xSemaphore = xSemaphoreCreateMutex(); 
    }  
    return TRUE;
}
static BOOL swInit(uint8_t pdrv)
{   
    BOOL reval = TRUE;
    uint16_t u16ID;
    uint8_t targetIndex;
    
    //if(xSemaphore == NULL)
    //{
    //    xSemaphore = xSemaphoreCreateMutex(); 
    //}
    SpiFlash_WaitReady(pdrv);  
    // check flash id
    u16ID = SpiFlash_ReadMidDid(pdrv/*SPI_FLASH_EX_0_INDEX*/);
    targetIndex = pdrv -1;
    switch(u16ID)
    {
        case 0xEF16:
            sysprintf("Flash[%d] found: W25Q64BV ...\n", targetIndex); 
            existFlag[targetIndex] = TRUE;
            //blockSize[targetIndex] = 8;
            break;        
        case 0xEF17:
            sysprintf("Flash[[%d] found: W25Q128BV ...\n", targetIndex);
            existFlag[targetIndex] = TRUE;  
            //blockSize[targetIndex] = 16;       
            break;  
        case 0x17EF:
            sysprintf("Flash[[%d] found: W25Q128BV ...\n", targetIndex);
            existFlag[targetIndex] = TRUE;  
            //blockSize[targetIndex] = 16;       
            break;         
        default:
            sysprintf("Flash[[%d] Wrong ID, 0x%x\n", targetIndex, u16ID);
            reval = FALSE;  
            existFlag[targetIndex] = FALSE;        
            break;         
    }
   
    return reval;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL FlashDrvExInit(BOOL testModeFlag)
{    
    if(initFlag)
        return TRUE;
    if(pSpiInterface == NULL)
    {
        sysprintf("FlashDrvExInit!!(FS:0x%08x (page:%d~%d, sector:%d~%d), RAW:0x%08x (page:%d~%d, sector:%d~%d))\n",  
                    SPI_FLASH_EX_FS_START_ADDRESS, SPI_FLASH_EX_FS_START_PAGE, SPI_FLASH_EX_FS_END_PAGE, SPI_FLASH_EX_FS_START_SECTOR, SPI_FLASH_EX_FS_END_SECTOR,
                    SPI_FLASH_EX_RAW_START_ADDRESS, SPI_FLASH_EX_RAW_START_PAGE, SPI_FLASH_EX_RAW_END_PAGE, SPI_FLASH_EX_RAW_START_SECTOR, SPI_FLASH_EX_RAW_END_SECTOR);
        pSpiInterface = SpiGetInterface(FLASH_SPI);
        if(pSpiInterface == NULL)
        {
            sysprintf("FlashDrvExInit ERROR (pSpiInterface == NULL)!!\n");
            return FALSE;
        }
        if(pSpiInterface->initFunc() == FALSE)
        {
            sysprintf("FlashDrvExInit ERROR (initFunc false)!!\n");
            return FALSE;
        }
        if(hwInit() == FALSE)
        {
            sysprintf("FlashDrvExInit ERROR (hwInit false)!!\n");
            return FALSE;
        }
        if(swInitPure() == FALSE)
        {
            sysprintf("FlashDrvExInit ERROR (swInitPure false)!!\n");
            return FALSE;
        }        
       
    }
    else
    {
        sysprintf("FlashDrvExInit!! ignore\n");
    }
    sysprintf("FlashDrvExInit!! OK\n");
    initFlag = TRUE;
    return TRUE;
}


BOOL FlashDrvExInitialize(uint8_t pdrv)
{
    //if(SPI_FLASH_EX_0_INDEX == pdrv)
    //    return FALSE;
    //return TRUE;
    if(swInit(pdrv) == FALSE)
    {
        sysprintf("FlashDrvExInitialize ERROR (swInit false)!!\n");
        return FALSE;
    }
    //sysprintf("FlashDrvExInitialize existFlag[pdrv - 1] = %d!!\n", existFlag[pdrv - 1]);
    return existFlag[pdrv - 1];
}
BOOL FlashDrvExStatus(uint8_t pdrv)
{
    return TRUE;    
}
BOOL FlashDrvExRead(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    //sysprintf(".");
    sector = sector + SHIFT_SECTOR_INDEX;
    SpiFlash_WaitReady(pdrv);
    SpiFlash_NormalRead(pdrv, sector*SPI_FLASH_EX_SECTOR_SIZE, buff, count*SPI_FLASH_EX_SECTOR_SIZE);
    SpiFlash_WaitReady(pdrv);
    //sysprintf("\r\n >> SpiFlash_NormalRead <%d> [sector = %d(0x%x), count = %d <<\n", pdrv, sector, sector, count);
    xSemaphoreGive(xSemaphore);
    
    return TRUE;
}
BOOL FlashDrvExWrite(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count)
{    
    BOOL reVal = TRUE;
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    //sysprintf("\r\n >> SpiFlash_NormalPageProgram [pdrv = %d, sector = %d(0x%x), count = %d <<\n", pdrv, sector, sector, count);
    //sysprintf("^");
    sector = sector + SHIFT_SECTOR_INDEX;
    SpiFlash_WaitReady(pdrv);
    SpiFlash_SectorErase(pdrv, sector*SPI_FLASH_EX_SECTOR_SIZE);
    SpiFlash_WaitReady(pdrv);
    SpiFlash_NormalSectorProgram(pdrv, sector*SPI_FLASH_EX_SECTOR_SIZE, buff, SPI_FLASH_EX_SECTOR_SIZE*count);
    SpiFlash_WaitReady(pdrv);
    #if(1)
    {
        //uint8_t* serialFlashBuff = pvPortMalloc(count*SPI_FLASH_EX_SECTOR_SIZE);
        //if(serialFlashBuff!=NULL)
        if(count*SPI_FLASH_EX_SECTOR_SIZE <= sizeof(serialFlashBuff))
        {
            SpiFlash_WaitReady(pdrv);
            SpiFlash_NormalRead(pdrv, sector*SPI_FLASH_EX_SECTOR_SIZE, serialFlashBuff, count*SPI_FLASH_EX_SECTOR_SIZE);
            SpiFlash_WaitReady(pdrv);
            if(memcmp(buff, serialFlashBuff, count*SPI_FLASH_EX_SECTOR_SIZE) != 0)
            {
                //#if(ENABLE_LOG_FUNCTION)
                //{
                //     char str[512];
                //     sprintf(str, " SpiFlash_NormalPageProgram <%d> ERROR (sector = %d(0x%x), size = %d) <<\n", pdrv, sector, sector, count*SPI_FLASH_EX_SECTOR_SIZE );
                //     LoglibPrintf(LOG_TYPE_ERROR, str, FALSE);
                //}
                //#else
                sysprintf("\r\n >> SpiFlash_NormalPageProgram <%d> ERROR (sector = %d(0x%x), size = %d) <<\n", pdrv, sector, sector, count*SPI_FLASH_EX_SECTOR_SIZE );
                //#endif
                errorTimes[pdrv-1]++;
                reVal = FALSE;
            }
            else
            {
                //#warning just for test 
                #if(0)
                if( ((sector>32) && (sector<96) && (pdrv == 2))  || ((sector>96) && (sector<128) && (pdrv == 1)) )
                {
                    sysprintf("\r\n >> SpiFlash_NormalPageProgram <%d> ERROR FACK (sector = %d(0x%x), size = %d, errorTimes[%d:%d] ) <<\n", 
                                                                pdrv, sector, sector, count*SPI_FLASH_EX_SECTOR_SIZE, errorTimes[0], errorTimes[1]);
                    SpiFlash_WaitReady(pdrv);
                    SpiFlash_SectorErase(pdrv, sector*SPI_FLASH_EX_SECTOR_SIZE);
                    SpiFlash_WaitReady(pdrv);
                    errorTimes[pdrv-1]++;
                    reVal = FALSE;
                }
                else
                #endif
                {
                    
                    //sysprintf("\r\n >> SpiFlash_NormalPageProgram <%d> SUCCESS (sector = %d(0x%x), size = %d, errorTimes[%d:%d] ) <<\n", 
                    //                                            pdrv, sector, sector, count*SPI_FLASH_EX_SECTOR_SIZE, errorTimes[0], errorTimes[1]);
                }
            }
            //vPortFree(serialFlashBuff);
        }
        
    }
    #endif
    xSemaphoreGive(xSemaphore);
    return reVal;
}
BOOL FlashDrvExIoctl(uint8_t pdrv, uint8_t cmd, void *buff )
{
    BOOL res = TRUE;
    switch(cmd) 
    {
        case CTRL_SYNC:
            SpiFlash_WaitReady(pdrv);
            break;
        case GET_SECTOR_COUNT:
            *(DWORD*)buff = SPI_FLASH_EX_FS_TOTAL_SIZE/SPI_FLASH_EX_SECTOR_SIZE;
            break;
        case GET_SECTOR_SIZE:
            *(WORD*)buff = SPI_FLASH_EX_SECTOR_SIZE;
            break;
        
        #if(USER_NEW_FATFS)
        case GET_BLOCK_SIZE:
            *(WORD*)buff = SPI_FLASH_EX_FS_TOTAL_SIZE/SPI_FLASH_EX_SECTOR_SIZE;
            //sysprintf(" >> FlashDrvExIoctl GET_BLOCK_SIZE (%d): %d<<\n", pdrv, *(WORD*)buff);
            break;
        #endif

        default:
            //sysprintf(" >> FlashDrvExIoctl cmd: %d  <<\n", cmd);
            res = FALSE;
            break;
    }
    //if(res)
    //    sysprintf(" >> FlashDrvExIoctl cmd: %d (%d)  <<\n", cmd, *(WORD*)buff);
    //else
    //    sysprintf(" >> FlashDrvExIoctl cmd: %d false  <<\n", cmd);
    return res;
}

void FlashDrvExChipEraseFs(uint8_t flashIndex)
{
    FlashDrvExChipErasePure(flashIndex, SPI_FLASH_EX_FS_START_SECTOR, SPI_FLASH_EX_FS_END_SECTOR);    
    sysprintf(" >> FlashDrvExChipErase_FS %d OK <<\n", flashIndex);
}
void FlashDrvExChipErasePure(uint8_t flashIndex, int startSector, int endSector)
{
    int sector;
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    sysprintf(" >> FlashDrvExChipErase_Pure %d go(sector:%d~%d) <<\n", flashIndex, startSector, endSector);
    
    SpiFlash_WaitReady(flashIndex);
    //for(sector = startSector; sector <= endSector; sector++)
    for(sector = startSector; sector < endSector; sector++) //¥u¨ì 2047
    {    
        SpiFlash_SectorErase(flashIndex, sector*SPI_FLASH_EX_SECTOR_SIZE);
        SpiFlash_WaitReady(flashIndex);
        sysprintf(" >> Erase %04d <<\r", sector);
    }
    sysprintf(" >> FlashDrvExChipErase_Pure %d OK <<\n", flashIndex);
    xSemaphoreGive(xSemaphore);    
}

void FlashDrvExChipEraseFull(uint8_t flashIndex)
{
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    SpiFlash_WaitReady(flashIndex);  
    sysprintf(" >> FlashDrvExChipErase_Full %d go <<\n", flashIndex);
    // /CS: active
    setFlashCS(flashIndex, TRUE);

    // send Command: 0x06, Write enable
    pSpiInterface->writeFunc(0, 0x06);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);

    //////////////////////////////////////////

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE);

    // send Command: 0xC7, Chip Erase
    pSpiInterface->writeFunc(0, 0xC7);

    // /CS: active
    setFlashCS(flashIndex, FALSE);

    SpiFlash_WaitReady(flashIndex);
    sysprintf(" >> FlashDrvExChipErase_Full %d OK <<\n", flashIndex);
    xSemaphoreGive(xSemaphore);
   
}

int FlashDrvExGetErrorTimes(void)
{
    return errorTimes[0] + errorTimes[1]*10000;
}
uint16_t FlashDrvExGetChipID(uint8_t flashIndex)
{
    uint16_t u16ID;
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    SpiFlash_WaitReady(flashIndex);  
    // check flash id
    u16ID = SpiFlash_ReadMidDid(flashIndex/*SPI_FLASH_EX_0_INDEX*/);
    xSemaphoreGive(xSemaphore);
    return u16ID;
}

void FlashDrvPageProgram(uint8_t flashIndex, uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    SpiFlash_WaitReady(flashIndex);
    SpiFlash_NormalPageProgram(flashIndex, StartAddress, u8DataBuffer, BuffLen);
    SpiFlash_WaitReady(flashIndex);
    xSemaphoreGive(xSemaphore);
}

void FlashDrvSectorErase(uint8_t flashIndex, uint32_t StartAddress)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    SpiFlash_WaitReady(flashIndex);
    SpiFlash_SectorErase(flashIndex, StartAddress);
    SpiFlash_WaitReady(flashIndex);
    xSemaphoreGive(xSemaphore);
}

void FlashDrvNormalRead(uint8_t flashIndex, uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    SpiFlash_WaitReady(flashIndex);
    SpiFlash_NormalRead(flashIndex, StartAddress, u8DataBuffer, BuffLen);
    SpiFlash_WaitReady(flashIndex);
    xSemaphoreGive(xSemaphore);
}
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


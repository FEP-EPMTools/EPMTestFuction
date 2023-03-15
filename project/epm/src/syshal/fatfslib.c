/**************************************************************************//**
* @file     fatfslib.c
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
#include "interface.h"
#include "fatfslib.h"
#include "ff.h"
#include "modemagent.h"
#include "paralib.h"
#include "fileagent.h"
#include "epddrv.h"
#include "flashdrvex.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define FATFS_HARDWARE_NUM  3
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static uint8_t deviceInterfaceIndex[FATFS_HARDWARE_NUM] = {FATFS_HARDWARE_SD_INTERFACE_INDEX, FATFS_HARDWARE_SFLASH_0_INTERFACE_INDEX, FATFS_HARDWARE_SFLASH_1_INTERFACE_INDEX};
static FatfsHardwareInterface* pFatfsHardwareInterface[FATFS_HARDWARE_NUM] = {NULL, NULL, NULL};

static FATFS  _FatfsVol[FATFS_HARDWARE_NUM];
static char*  _Path[FATFS_HARDWARE_NUM] = {"0:", "1:", "2:"};

static BOOL  fatfsExistFlag[FATFS_HARDWARE_NUM] = {FALSE, FALSE, FALSE};
//static SemaphoreHandle_t xSemaphore;

//static TickType_t threadWaitTime   = portMAX_DELAY;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


static BOOL hwInit(void)
{
    return TRUE;
}
static BOOL swInit(void)
{    
    //xSemaphore = xSemaphoreCreateBinary();
    /* xSemaphoreGive(xSemaphore); */
    return FileAgentInit();
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL FatfsInit(BOOL testModeFlag)
{
    int i; 
    FRESULT result; 
    BOOL Reval = FALSE;
    sysprintf("FatfsInit!!\n");    
    
    //for(i = 0; i<FATFS_HARDWARE_NUM; i++)
    for(i = 0; i<1; i++)
    //for(i = 1; i<FATFS_HARDWARE_NUM; i++)
    { 
        pFatfsHardwareInterface[i] = FatfsHardwareGetInterface(deviceInterfaceIndex[i]);
        if(pFatfsHardwareInterface[i] == NULL)
        {
            sysprintf("FatfsInit ERROR (pKeyHardwareInterface == NULL)!!\n");
            //return FALSE;
        }
        else
        {
            if(pFatfsHardwareInterface[i]->initFunc() == FALSE)
            {
                sysprintf("FatfsInit ERROR (initFunc false)!!\n");
                //return FALSE;
            }  
            else
            {
                BOOL mountFlag = FALSE;
                result = f_mount(&_FatfsVol[i], (const TCHAR*)_Path[i], 1);
                //if(result != FR_OK)
                sysprintf("[INFO] FatfsInit [%d] (f_mount: %d)!!\n", i, result);
                if(result == FR_NO_FILESYSTEM)
                {
                    sysprintf("[INFO] FatfsInit [%d] FR_NO_FILESYSTEM  (try  f_mkfs)!!\n", i);
                    #if(ENABLE_EPD_DRIVER)
//                    EPDShowBGScreen(EPD_PICT_INDEX_DEFRAG, TRUE); 
                    #endif
                    if(FatFsFormat(_Path[i]))
                    {
                        //f_mkfs((const TCHAR*)_Path[i], 0, 0);            
                        sysprintf("[INFO] FatfsInit [%d]  f_mkfs OK!!\n", i);
                        ///*
                        result = f_mount(&_FatfsVol[i], (const TCHAR*)_Path[i], 1); 
                        
                        if(result == FR_OK)
                        {
                            sysprintf("[INFO] FatfsInit [%d]  re f_mount OK!!\n", i);
                            mountFlag = TRUE; 
                        }
                        else
                        {
                            sysprintf("[INFO] FatfsInit [%d]  re f_mount ERROR (%d)!!\n", i, result);
                            //return FALSE;
                        }
                        //*/
                    }
                    else
                    {
                        sysprintf("[INFO] FatfsInit [%d]  f_mkfs error!!\n", i);
                        //return FALSE;
                    }
                    
                }
                else
                {
                    mountFlag = TRUE; 
                }
                if(mountFlag)
                {
                    sysprintf("FatfsInit f_mount [%d]:[%s]  OK !!\n", i, _Path[i]);
                    FatfsGetDiskUseage(_Path[i]);
                    //FatfsListFileEx(_Path[i]);
                    if(i != 0) //ignore sd card
                       fatfsExistFlag[i] = TRUE;  
                    Reval = TRUE;    
                }                
            }                
        }        
       
    }
    
    if(Reval == FALSE)
    {
        //terninalPrintf("FatfsInit Reval == FALSE!!\n");
        return FALSE;//只要有一個成功就可以
    }
    if(hwInit() == FALSE)
    {
        //terninalPrintf("FatfsInit ERROR (hwInit false)!!\n");
        sysprintf("FatfsInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        //terninalPrintf("FatfsInit ERROR (swInit false)!!\n");
        sysprintf("FatfsInit ERROR (swInit false)!!\n");
        return FALSE;
    }    
    
    //f_mount(&_FatfsVol[0], (const TCHAR*)_Path[0], 1);
    return TRUE;
}
FatfsHardwareInterface* FatfsGetCallback(uint8_t index)
{
    if(index < FATFS_HARDWARE_NUM) 
        return pFatfsHardwareInterface[index];
    else
        return NULL;
}

void FatfsGetDiskUseage(char* dir)
{
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;
    /* Get drive information and free clusters */
    FRESULT res = f_getfree(dir, &fre_clust, &fs);
    if (res) 
    {
        sysprintf("FatfsGetDiskUseage f_getfree ERROR (%d)\n", res);
        return;
    }
    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;
    //sysprintf("fre_clust = %d, (fs->n_fatent - 2) = %d, fs->csize = %d, fs->ssize = %d\n", fre_clust, (fs->n_fatent - 2), fs->csize, fs->ssize);
    sysprintf("FatfsGetDiskUseage [%s]%d KB total drive space. %d KB available.\n", dir, tot_sect*fs->ssize/1024, fre_sect*fs->ssize/1024);
    if((fre_sect*fs->ssize/1024) == 0)
    {
        sysprintf("--[ERROR]--> DISK error 2, execute f_mkfs(%s, 0, 0)...\r\n", dir);
        //f_mkfs(dir, 0, 0); 
        FatFsFormat(dir);
        sysprintf("--[ERROR]--> DISK error 2, execute f_mkfs(%s, 0, 0)  OK,  re do!!!...\r\n", dir);
    }
}

int FatfsGetDiskUseageEx(char* dir)
{
    FATFS *fs;
    DWORD fre_clust, fre_sect;//, tot_sect;
    /* Get drive information and free clusters */
    FRESULT res = f_getfree(dir, &fre_clust, &fs);
    if (res) 
    {
        //sysprintf("FatfsGetDiskUseage f_getfree ERROR (%d)\n", res);
        return 0;
    }
    /* Get total sectors and free sectors */
    //tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;
    //sysprintf("fre_clust = %d, (fs->n_fatent - 2) = %d, fs->csize = %d, fs->ssize = %d\n", fre_clust, (fs->n_fatent - 2), fs->csize, fs->ssize);
    //sysprintf("FatfsGetDiskUseage [%s]%d KB total drive space. %d KB available.\n", dir, tot_sect*fs->ssize/1024, fre_sect*fs->ssize/1024);
    //if((fre_sect*fs->ssize/1024) == 0)
    //{
    //    sysprintf("--[ERROR]--> DISK error 2, execute f_mkfs(%s, 0, 0)...\r\n", dir);
        //f_mkfs(dir, 0, 0); 
    //    FatFsFormat(dir);
    //    sysprintf("--[ERROR]--> DISK error 2, execute f_mkfs(%s, 0, 0)  OK,  re do!!!...\r\n", dir);
    //}
    return fre_sect*fs->ssize;
}

BOOL FatFsFormat(char* path)
{
    FRESULT res;
    /* Create FAT volume */
    if(strcmp(path, "0:") == 0)
    {
        sysprintf("\r\n --[WARNING] FatFsFormat FM_FAT32 [%s]--\r\n", path);
        //res = f_mkfs(path, 0, 0);
        return FALSE;
    }
    else
    {
        sysprintf("\r\n --[WARNING] FatFsFormat FM_ANY [%s]--\r\n", path);
        #if(USER_NEW_FATFS)
        BYTE work[_MAX_SS]; /* Work area (larger is better for processing time) */
        res = f_mkfs(path, FM_SFD |FM_ANY, 0, work, sizeof(work));
        #else
        if(strcmp(path, "1:") == 0)
            FlashDrvExChipEraseFs(SPI_FLASH_EX_0_INDEX);
        else
            FlashDrvExChipEraseFs(SPI_FLASH_EX_1_INDEX);
        res = f_mkfs(path, 1, 0);
        #endif
    }
    if (res == FR_OK) 
    {        
        //FatfsGetDiskUseage(path);
        sysprintf("\r\n --[WARNING] FatFsFormat [%s]  OK --\r\n", path);
        return TRUE;
    }
    else
    {
        sysprintf("\r\n --[WARNING] FatFsFormat [%s]  ERROR(%d) --\r\n", path, res);
        return FALSE;
    }

}
int FatFsGetCounter(void)
{
    return FATFS_HARDWARE_NUM;
}
BOOL FatFsGetExistFlag(FatfsIndex index)
{
    return fatfsExistFlag[index];
}
char* FatFsGetRootStr(FatfsIndex index)
{
    return _Path[index];
}



/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


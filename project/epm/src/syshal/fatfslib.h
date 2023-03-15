/**************************************************************************//**
* @file     fatfslib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __FATFS_LIB_H__
#define __FATFS_LIB_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
typedef enum{
    FATFS_INDEX_SD = 0,
    FATFS_INDEX_SFLASH_0 = 1,
    FATFS_INDEX_SFLASH_1 = 2
}FatfsIndex;      

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL FatfsInit(BOOL testModeFlag);
FatfsHardwareInterface* FatfsGetCallback(uint8_t index);
void FatfsListFile(char* dir);
void FatfsListFileEx (char* dir);
void FatfsGetDiskUseage(char* dir);
int  FatfsGetDiskUseageEx(char* dir);
BOOL FatFsFormat(char* path);
int FatFsGetCounter(void);
BOOL FatFsGetExistFlag(FatfsIndex index);
char* FatFsGetRootStr(FatfsIndex index);
int FlashDrvExGetErrorTimes(void);
#ifdef __cplusplus
}
#endif

#endif //__FATFS_LIB_H__

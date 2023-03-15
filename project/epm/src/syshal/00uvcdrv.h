/**************************************************************************//**
* @file     uvcdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __UVC_DRV_H__
#define __UVC_DRV_H__

#include "nuc970.h"
#include "halinterface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL UVCDrvInit(BOOL testModeFlag);
void UVCSetPower(int index, BOOL flag);
BOOL UVCTakePhoto(int index, uint8_t** photoPr, int* photoLen, StorageType type, char* dir, char* fileName, BOOL smallSizeFlag);
#ifdef __cplusplus
}
#endif

#endif //__UVC_DRV_H__

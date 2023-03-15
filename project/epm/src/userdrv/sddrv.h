/**************************************************************************//**
* @file  sddrv.c
* @version  V1.00
* $Revision:
* $Date:
* @brief
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SD_DRV_H__
#define __SD_DRV_H__

#include "nuc970.h"

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
BOOL SdDrvInit(void);
    
BOOL SDDrvInitialize(uint8_t pdrv);
BOOL SDDrvStatus(uint8_t pdrv);
BOOL SDDrvRead(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count);
BOOL SDDrvWrite(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count);
BOOL SDDrvIoctl(uint8_t pdrv, uint8_t cmd, void *buff);
    
#ifdef __cplusplus
}
#endif

#endif//__SD_DRV_H__

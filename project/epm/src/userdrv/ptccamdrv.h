/**************************************************************************//**
* @file     ptccamdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __PTC_CAM_DRV_H__
#define __PTC_CAM_DRV_H__

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
BOOL PTCCamDrvInit(void);
INT32 PTCCamWrite(PUINT8 pucBuf, UINT32 uLen);
INT32 PTCCamRead(PUINT8 pucBuf, UINT32 uLen);
BOOL PTCCamSetPower(BOOL flag);

#ifdef __cplusplus
}
#endif

#endif //__PTC_CAM_DRV_H__

/**************************************************************************//**
* @file     pct08drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __PCT08_DRV_H__
#define __PCT08_DRV_H__

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
BOOL PCT08DrvInit(BOOL testModeFlag);
void PCT08FlushTxRx(void);
void PCT08SetPower(BOOL flag);
INT32 PCT08Read(PUINT8 pucBuf, UINT32 uLen);
INT32 PCT08Write(PUINT8 pucBuf, UINT32 uLen);
#ifdef __cplusplus
}
#endif

#endif //__PCT08_DRV_H__

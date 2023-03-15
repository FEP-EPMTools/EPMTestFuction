/**************************************************************************//**
* @file     ts2000drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __TS2000_DRV_H__
#define __TS2000_DRV_H__

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
BOOL TS2000DrvInit(void);
BOOL TS2000SetPower(BOOL flag);
uint8_t TS2000CheckReader(void);
BOOL TS2000BreakCheckReader(void);
BOOL TS2000Process(uint16_t targetDeduct, tsreaderDepositResultCallback callback);
BOOL TS2000GetBootedStatus(void);
#ifdef __cplusplus
}
#endif

#endif //__TS2000_DRV_H__

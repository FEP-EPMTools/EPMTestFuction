/**************************************************************************//**
* @file     ts1000drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __TS1000_DRV_H__
#define __TS1000_DRV_H__

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
BOOL TS1000DrvInit(void);
BOOL TS1000SetPower(BOOL flag);
uint8_t TS1000CheckReader(void);
BOOL TS1000BreakCheckReader(void);
BOOL TS1000Process(uint16_t targetDeduct);
#ifdef __cplusplus
}
#endif

#endif //__TS1000_DRV_H__

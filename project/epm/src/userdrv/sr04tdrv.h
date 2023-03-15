/**************************************************************************//**
* @file     sr04tdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SR04T_DRV_H__
#define __SR04T_DRV_H__

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
BOOL SR04TDrvInit(void);
BOOL SR04TMeasureDist(uint8_t id, int* detectResult);
#ifdef __cplusplus
}
#endif

#endif //__SR04T_DRV_H__

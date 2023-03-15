/**************************************************************************//**
* @file     vl53l0xdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __VL53L0X_DRV_H__
#define __VL53L0X_DRV_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define VL53L0X_DEVICE_1        0x0
#define VL53L0X_DEVICE_2        0x1
#define VL53L0X_DEVICE_ALL      0xf
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/



BOOL Vl53l0xDrvInit(void);
BOOL Vl53l0xMeasureDist(uint8_t id, int* detectResult);
#ifdef __cplusplus
}
#endif

#endif //__VL53L0X_DRV_H__

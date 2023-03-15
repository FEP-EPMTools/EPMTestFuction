/**************************************************************************//**
* @file     i2c0drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __I2C0_DRV_H__
#define __I2C0_DRV_H__

#include "nuc970.h"
#include "interface.h"
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
BOOL I2c0DrvInit(void);
int32_t I2c0Ioctl(uint32_t cmd, uint32_t arg0, uint32_t arg1);
int32_t I2c0Read(uint8_t* buf, uint32_t len);
int32_t I2c0Write(uint8_t* buf, uint32_t len);
void I2c0enableCriticalSectionFunc(BOOL flag);
#ifdef __cplusplus
}
#endif

#endif //__I2C0_DRV_H__

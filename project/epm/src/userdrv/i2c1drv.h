/**************************************************************************//**
* @file     i2c1drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __I2C1_DRV_H__
#define __I2C1_DRV_H__

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
BOOL I2c1DrvInit(void);
int32_t I2c1Ioctl(uint32_t cmd, uint32_t arg0, uint32_t arg1);
int32_t I2c1Read(uint8_t* buf, uint32_t len);
int32_t I2c1Write(uint8_t* buf, uint32_t len);
void I2c1enableCriticalSectionFunc(BOOL flag);
void I2c1SetPin(void);
void I2c1ResetPin(void);
void I2c1ResetInputPin(void);
#ifdef __cplusplus
}
#endif

#endif //__I2C1_DRV_H__

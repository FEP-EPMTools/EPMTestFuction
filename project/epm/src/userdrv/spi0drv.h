/**************************************************************************//**
* @file     spi0drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SPI0_DRV_H__
#define __SPI0_DRV_H__

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
BOOL Spi0DrvInit(void);
void Spi0Write(uint8_t buff_id, uint32_t data);
uint32_t Spi0Read(uint8_t buff_id);
void Spi0ActiveCS(BOOL active);
void Spi0SetPin(void);
void Spi0ResetPin(void);
#ifdef __cplusplus
}
#endif

#endif //__SPI0_DRV_H__

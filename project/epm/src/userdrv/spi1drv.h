/**************************************************************************//**
* @file     spi1drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SPI1_DRV_H__
#define __SPI1_DRV_H__

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
BOOL Spi1DrvInit(void);
void Spi1Write(uint8_t buff_id, uint32_t data);
uint32_t Spi1Read(uint8_t buff_id);
void Spi1ActiveCS(BOOL active);
void Spi1SetPin(void);
void Spi1ResetPin(void);
#ifdef __cplusplus
}
#endif

#endif //__SPI1_DRV_H__

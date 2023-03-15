/**************************************************************************//**
* @file     loradrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __LORA_DRV_H__
#define __LORA_DRV_H__

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
BOOL LoraDrvInit(void);
INT32 LoraRead(PUINT8 pucBuf, UINT32 uLen);
INT32 LoraWrite(PUINT8 pucBuf, UINT32 uLen);
BaseType_t LoraReadWait(TickType_t time);
#ifdef __cplusplus
}
#endif

#endif //__LORA_DRV_H__

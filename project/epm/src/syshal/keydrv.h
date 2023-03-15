/**************************************************************************//**
* @file     keydrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __KEY_DRV_H__
#define __KEY_DRV_H__

#include "nuc970.h"
#include "halinterface.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

#define KEY_DRV_MODE_NORMAL_INDEX  0x01
#define KEY_DRV_MODE_TEST_INDEX  0x02
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL KeyDrvInit(void);
void KeyDrvSetCallbackFunc(keyCallbackFunc func);
void KeyDrvSetPowerFunc(BOOL powerFlag);
void KeyDrvSetMode(uint8_t mode);
    
#ifdef __cplusplus
}
#endif

#endif //__KEY_DRV_H__

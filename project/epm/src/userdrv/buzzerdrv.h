/**************************************************************************//**
* @file     buzzerdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __BUZZER_DRV_H__
#define __BUZZER_DRV_H__

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
BOOL BuzzerDrvInit(BOOL testMode);
void BuzzerPlay(uint32_t time, uint32_t delayTime, uint8_t counter, BOOL blocking);
#ifdef __cplusplus
}
#endif

#endif //__BUZZER_DRV_H__

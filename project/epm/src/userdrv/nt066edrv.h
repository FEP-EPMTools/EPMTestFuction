/**************************************************************************//**
* @file     nt066edrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __NT066E_DRV_H__
#define __NT066E_DRV_H__

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
BOOL NT066EDrvInit(BOOL testModeFlag);
void NT066ESetCallbackFunc(keyHardwareCallbackFunc func);
BOOL NT066EResetChip(void);  
BOOL NT066ESetPower(BOOL flag);   
BOOL NT066ESetTriggerLevel(int level);    
    
#if (ENABLE_BURNIN_TESTER)
uint32_t GetNT066EBurninTestCounter(void);
uint32_t GetNT066EBurninTestErrorCounter(void);
#endif    
    
#ifdef __cplusplus
}
#endif

#endif //__NT066E_DRV_H__

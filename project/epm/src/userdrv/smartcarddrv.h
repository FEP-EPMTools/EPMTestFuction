/**************************************************************************//**
* @file     smartcarddrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SMART_CARD_DRV_H__
#define __SMART_CARD_DRV_H__

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
BOOL SmartCardDrvInit(BOOL testModeFlag);

BOOL SmartCardTestInit(BOOL testModeFlag);
uint32_t GetSmartCardBurninTestCounter(void);
uint32_t GetSmartCardBurninTestErrorCounter(void);    
    
#ifdef __cplusplus
}
#endif

#endif //__SMART_CARD_DRV_H__

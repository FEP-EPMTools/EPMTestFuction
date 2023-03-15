/**************************************************************************//**
* @file     guiburnintester.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2020 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __GUI_BURNIN_TESTER_H__
#define __GUI_BURNIN_TESTER_H__

#include "nuc970.h"
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

BOOL GuiBurninTesterOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3);
BOOL GuiBurninTesterUpdateData(void);
BOOL GuiBurninTesterKeyCallback(uint8_t keyId, uint8_t downUp);
BOOL GuiBurninTesterTimerCallback(uint8_t timerIndex);
BOOL GuiBurninTesterPowerCallbackFunc(uint8_t type, int flag);
void GuiBurninTesterStop(void);

#ifdef __cplusplus
}
#endif

#endif //__GUI_BURNIN_TESTER_H__

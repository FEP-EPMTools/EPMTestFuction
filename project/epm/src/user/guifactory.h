/**************************************************************************//**
* @file     guifactory.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __GUI_FACTORY_H__
#define __GUI_FACTORY_H__

#include "nuc970.h"
#include "halinterface.h"
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

BOOL GuiFactoryOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3);
BOOL GuiFactoryUpdateData(void);
BOOL GuiFactoryKeyCallback(uint8_t keyId, uint8_t downUp);
BOOL GuiFactoryTimerCallback(uint8_t timerIndex);
BOOL GuiFactoryPowerCallbackFunc(uint8_t type, int flag);
    
void ShowMsgBar(int id,int xShift);
#ifdef __cplusplus
}
#endif

#endif //__GUI_FACTORY_H__

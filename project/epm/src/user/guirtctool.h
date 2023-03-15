/**************************************************************************//**
* @file     guisettingid.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __GUI_RTCTOOL_H__
#define __GUI_RTCTOOL_H__

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

BOOL GuiRTCToolOnDraw(uint8_t oriGuiId, uint8_t para, int para2, int para3);
BOOL GuiRTCToolUpdateData(void);
BOOL GuiRTCToolKeyCallback(uint8_t keyId, uint8_t downUp);
BOOL GuiRTCToolTimerCallback(uint8_t timerIndex);
BOOL GuiRTCToolPowerCallbackFunc(uint8_t type, int flag);
#ifdef __cplusplus
}
#endif

#endif //__GUI_SETTING_H__

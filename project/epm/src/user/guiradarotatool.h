/**************************************************************************//**
* @file     guinull.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __GUI_RADAROTATOOL_H__
#define __GUI_RADAROTATOOL_H__

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

BOOL GuiRadarOTAToolOnDraw(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3);
BOOL GuiRadarOTAToolUpdateData(void);
BOOL GuiRadarOTAToolKeyCallback(uint8_t keyId, uint8_t downUp);
BOOL GuiRadarOTAToolTimerCallback(uint8_t timerIndex);
BOOL GuiRadarOTAToolPowerCallbackFunc(uint8_t type, int flag);
#ifdef __cplusplus
}
#endif

#endif //__GUI_NULL_H__

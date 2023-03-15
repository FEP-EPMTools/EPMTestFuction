/**************************************************************************//**
* @file     timerdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __TIMER_DRV_H__
#define __TIMER_DRV_H__

#include "nuc970.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "halinterface.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define TIMER_INTERFACE_NUM          3

#define TIMER_0_INTERFACE_INDEX      0
#define TIMER_1_INTERFACE_INDEX      1    
#define TIMER_2_INTERFACE_INDEX      2 

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL TimerDrvInit(void);
BOOL TimerSetTimeout(uint8_t timerIndex, TickType_t time);
BOOL TimerRun(uint8_t timerIndex);
void TimerSetCallback(timerCallbackFunc callback);
BOOL TimerAllStop(void);    
    

#ifdef __cplusplus
}
#endif

#endif //__TIMER_DRV_H__

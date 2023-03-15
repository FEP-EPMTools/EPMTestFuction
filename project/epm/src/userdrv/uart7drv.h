/**************************************************************************//**
* @file     uart7drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __UART7_DRV_H__
#define __UART7_DRV_H__

#include "nuc970.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

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
BOOL UART7DrvInit(UINT32 baudRate);
INT32 UART7Write(PUINT8 pucBuf, UINT32 uLen);
BaseType_t UART7ReadWait(TickType_t time);
INT32 UART7Read(PUINT8 pucBuf, UINT32 uLen);
BOOL UART7SetPower(BOOL flag);
BOOL UART7SetRS232Power(BOOL flag);
INT UART7Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1);
#ifdef __cplusplus
}
#endif

#endif //__UART7_DRV_H__

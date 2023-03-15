/**************************************************************************//**
* @file     uart0drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __UART0_DRV_H__
#define __UART0_DRV_H__

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
BOOL UART0DrvInit(UINT32 baudRate);
INT32 UART0Write(PUINT8 pucBuf, UINT32 uLen);
BaseType_t UART0ReadWait(TickType_t time);
INT32 UART0Read(PUINT8 pucBuf, UINT32 uLen);
BOOL UART0SetPower(BOOL flag);
BOOL UART0SetRS232Power(BOOL flag);
INT UART0Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1);
#ifdef __cplusplus
}
#endif

#endif //__UART0_DRV_H__

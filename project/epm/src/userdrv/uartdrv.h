/**************************************************************************//**
* @file     uartdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __UART_DRV_H__
#define __UART_DRV_H__

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
#define UART_DRV_NUMBER     1

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL UARTDrvInit(UINT32 baudRate);
INT32 UARTWrite(PUINT8 pucBuf, UINT32 uLen);
BaseType_t UARTReadWait(TickType_t time);
INT32 UARTRead(PUINT8 pucBuf, UINT32 uLen);
BOOL UARTSetPower(BOOL flag);
BOOL UARTSetRS232Power(BOOL flag);
INT UARTIoctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1);
#ifdef __cplusplus
}
#endif

#endif //__UART_DRV_H__

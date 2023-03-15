/**************************************************************************//**
* @file     uart1drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __UART1_DRV_H__
#define __UART1_DRV_H__

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
BOOL UART1DrvInit(UINT32 baudRate);
INT32 UART1Write(PUINT8 pucBuf, UINT32 uLen);
BaseType_t UART1ReadWait(TickType_t time);
INT32 UART1Read(PUINT8 pucBuf, UINT32 uLen);
BOOL UART1SetPower(BOOL flag);
BOOL UART1SetRS232Power(BOOL flag);
INT UART1Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1);
#ifdef __cplusplus
}
#endif

#endif //__UART1_DRV_H__

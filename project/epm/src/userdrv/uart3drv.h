/**************************************************************************//**
* @file     uart3drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __UART3_DRV_H__
#define __UART3_DRV_H__

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
BOOL UART3DrvInit(UINT32 baudRate);
INT32 UART3Write(PUINT8 pucBuf, UINT32 uLen);
BaseType_t UART3ReadWait(TickType_t time);
INT32 UART3Read(PUINT8 pucBuf, UINT32 uLen);
BOOL UART3SetPower(BOOL flag);
BOOL UART3SetRS232Power(BOOL flag);
INT UART3Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1);
#ifdef __cplusplus
}
#endif

#endif //__UART3_DRV_H__

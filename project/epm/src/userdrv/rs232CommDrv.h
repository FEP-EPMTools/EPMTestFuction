/**************************************************************************//**
* @file     rs232CommDrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __RS232_COMM_DRV_H__
#define __RS232_COMM_DRV_H__

#include "nuc970.h"
#include "interface.h"
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
BOOL RS232CommDrvInit(void);
INT32 RS232CommDrvRead(PUINT8 pucBuf, UINT32 uLen);
INT32 RS232CommDrvWrite(PUINT8 pucBuf, UINT32 uLen);
BaseType_t RS232CommDrvReadWait(TickType_t time);
#ifdef __cplusplus
}
#endif

#endif //__RS232_COMM_DRV_H__

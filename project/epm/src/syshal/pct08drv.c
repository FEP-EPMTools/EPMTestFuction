/**************************************************************************//**
* @file     pct08drv.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"
#include "rtc.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "pct08drv.h"
#include "interface.h"
#include "halinterface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define CPT08_DRV_UART   UART_10_INTERFACE_INDEX

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static void flushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL PCT08DrvInit(BOOL testModeFlag)
{
    sysprintf("PCT08DrvInit!!\n");
    pUartInterface = UartGetInterface(CPT08_DRV_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("PCT08DrvInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    //if(pUartInterface->initFunc(57600) == FALSE)
    //if(pUartInterface->initFunc(38400) == FALSE)
    // by sam 2018.04.03
    if(pUartInterface->initFunc(115200) == FALSE)
    {
        sysprintf("PCT08DrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }

    sysprintf("PCT08DrvInit OK!!\n");
    PCT08SetPower(FALSE);
    return TRUE;
}
void PCT08SetPower(BOOL flag)
{
    pUartInterface->setRS232PowerFunc(flag);
    pUartInterface->setPowerFunc(flag);
}
void PCT08FlushTxRx(void)
{
    flushBuffer();
}
INT32 PCT08Read(PUINT8 pucBuf, UINT32 uLen)
{
    return pUartInterface->readFunc(pucBuf, uLen);
}
INT32 PCT08Write(PUINT8 pucBuf, UINT32 uLen)
{
    return pUartInterface->writeFunc(pucBuf, uLen);
}




/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


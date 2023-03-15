/**************************************************************************//**
* @file     rs232CommDrv.c
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
#include "adc.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "rs232CommDrv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define RS232_COMM_UART   UART_10_INTERFACE_INDEX

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UartInterface* pUartInterface = NULL;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(void)
{
    return TRUE;
}
static BOOL swInit(void)
{   
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL RS232CommDrvInit(void)
{
    sysprintf("LoraDrvInit!!\n");
    pUartInterface = UartGetInterface(RS232_COMM_UART);
    if(pUartInterface == NULL)
    {
        sysprintf("LoraDrvInit ERROR (pUartInterface == NULL)!!\n");
        return FALSE;
    }
    //if(pUartInterface->initFunc(9600) == FALSE)
    if(pUartInterface->initFunc(115200) == FALSE)
    {
        sysprintf("LoraDrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(hwInit() == FALSE)
    {
        sysprintf("LoraDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("LoraDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}


INT32 RS232CommDrvRead(PUINT8 pucBuf, UINT32 uLen)
{
    return pUartInterface->readFunc(pucBuf, uLen);
}

INT32 RS232CommDrvWrite(PUINT8 pucBuf, UINT32 uLen)
{
    return pUartInterface->writeFunc(pucBuf, uLen);
}
BaseType_t RS232CommDrvReadWait(TickType_t time)
{
    return pUartInterface->readWaitFunc(time);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


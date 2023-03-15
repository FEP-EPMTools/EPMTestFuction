/**************************************************************************//**
* @file     uart8drv.c
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
#include "gpio.h"
#include "uart.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "uart8drv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//GPF8 Power pin
#define POWER_PORT  GPIOF
#define POWER_PIN   BIT8
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UART_T param;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(UINT32 baudRate)
{
    int retval;
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    //uart8 GPH 12 13 14 15
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<<16)) | (0x9<<16));
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<<20)) | (0x9<<20));
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<<24)) | (0x9<<24));
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xFu<<28)) | (0x9u<<28));
    
    //Power pin GPF8
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xFu<<0)) | (0x0u<<0));
    GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    UART8SetPower(FALSE);

    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = baudRate;
    param.ucUartNo = UART8;
    param.ucDataBits = DATA_BITS_8;
    param.ucStopBits = STOP_BITS_1;
    param.ucParity = PARITY_NONE;
    param.ucRxTriggerLevel = UART_FCR_RFITL_1BYTE;
    retval = uartOpen(&param);
    if(retval != 0) 
    {
        sysprintf("hwInit Open UART error!\n");
        return FALSE;
    }

    /* set TX interrupt mode */
    retval = uartIoctl(param.ucUartNo, UART_IOC_SETTXMODE, UARTPOLLMODE /*UARTINTMODE*/ , 0);
    if (retval != 0) 
    {
        sysprintf("hwInit Set TX interrupt mode fail!\n");
        return FALSE;
    }

    /* set RX interrupt mode */
    retval = uartIoctl(param.ucUartNo, UART_IOC_SETRXMODE, UARTINTMODE, 0);
    if (retval != 0) 
    {
        sysprintf("hwInit Set RX interrupt mode fail!\n");
        return FALSE;
    }
    
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL UART8DrvInit(UINT32 baudRate)
{
    int retval;
    sysprintf("UART8DrvInit!!\n");
    retval = hwInit(baudRate);
    return retval;
}
INT32 UART8Write(PUINT8 pucBuf, UINT32 uLen)
{
    /* DEBUG LED *///terninalPrintf("=>");
    /* DEBUG LED *///for(int i=0;i<uLen;i++)
    /* DEBUG LED *///    terninalPrintf("%02x ",pucBuf[i]);
    /* DEBUG LED *///terninalPrintf("\n");
    return uartWrite(param.ucUartNo, pucBuf, uLen);
}
INT32 UART8Read(PUINT8 pucBuf, UINT32 uLen)
{
    /*
    int temp = uartRead(param.ucUartNo, pucBuf, uLen);  
    terninalPrintf("<=");
    for(int i=0 ; i<temp ; i++)
        terninalPrintf("%02x ",pucBuf[i]);
    terninalPrintf("\n");
    return temp;
    */
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART8ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART8SetPower(BOOL flag)
{
    if(flag)
    {         
        GPIO_SetBit(POWER_PORT, POWER_PIN);
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<24)) | (0x1<<24));        
    }
    else
    {
        GPIO_ClrBit(POWER_PORT, POWER_PIN);  
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<24)) | (0x0<<24));         
    }
    return TRUE;
}
BOOL UART8SetRS232Power(BOOL flag)
{
    return FALSE;
}
INT UART8Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


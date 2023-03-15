/**************************************************************************//**
* @file     uart0drv.c
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
#include "uart0drv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

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
    
    // GPE 0, 1 //TX, RX
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<0)) | (0x9<<0));
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<4)) | (0x9<<4));
    
    
    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = baudRate;
    param.ucUartNo = UART0;
    param.ucDataBits = DATA_BITS_8;
    param.ucStopBits = STOP_BITS_1;
    param.ucParity = PARITY_NONE;
    param.ucRxTriggerLevel = UART_FCR_RFITL_1BYTE;
    retval = uartOpen(&param);
    if(retval != 0) 
    {
        terninalPrintf("hwInit Open UART error!\r\n");
        sysprintf("hwInit Open UART error!\n");
        return FALSE;
    }

    /* set TX interrupt mode */
    retval = uartIoctl(param.ucUartNo, UART_IOC_SETTXMODE, UARTPOLLMODE /*UARTINTMODE*/ , 0);
    //retval = uartIoctl(param.ucUartNo, UART_IOC_SETTXMODE, UARTINTMODE , 0);
    if (retval != 0) 
    {
        terninalPrintf("hwInit Set TX interrupt mode fail!\r\n");
        sysprintf("hwInit Set TX interrupt mode fail!\n");
        return FALSE;
    }

    /* set RX interrupt mode */
    retval = uartIoctl(param.ucUartNo, UART_IOC_SETRXMODE, UARTINTMODE, 0);
    if (retval != 0) 
    {
        terninalPrintf("hwInit Set RX interrupt mode fail!\r\n");
        sysprintf("hwInit Set RX interrupt mode fail!\n");
        return FALSE;
    }

    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL UART0DrvInit(UINT32 baudRate)
{
    int retval;
    //terninalPrintf("UART0DrvInit!!\r\n");
    sysprintf("UART0DrvInit!!\n");
    retval = hwInit(baudRate);
    //terninalPrintf("UART0DrvInit!!\r\n");
    return retval;
}
INT32 UART0Write(PUINT8 pucBuf, UINT32 uLen)
{
    INT32  reVal;
    reVal = uartWrite(param.ucUartNo, pucBuf, uLen);
    return reVal;
}
INT32 UART0Read(PUINT8 pucBuf, UINT32 uLen)
{
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART0ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART0SetPower(BOOL flag)
{
    if(flag)
    {         
        
    }
    else
    {
       
            
    }
    
    return FALSE;
}
BOOL UART0SetRS232Power(BOOL flag)
{
    if(flag)
    {         
        // GPE 0, 1 //TX, RX
        outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<0)) | (0x9<<0));
        outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<4)) | (0x9<<4));
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<16)) | (0x1<<16)); 
    }
    else
    {
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<16)) | (0x0<<16));   
        // GPE 0, 1 //TX, RX
        outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<0)) | (0x0<<0));
        outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<4)) | (0x0<<4)); 
    }
    return TRUE;
}
INT UART0Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

void UART0SetRTS(BOOL flag)
{
    if(flag)
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    else
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
}

void UART0FlushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (UART0Ioctl(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
    


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


/**************************************************************************//**
* @file     uart1drv.c
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
#include "uart1drv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//GPE6 Power pin
#define POWER_PORT  GPIOE
#define POWER_PIN   BIT6

//GPE7 RS232 Power pin
#define RS232_POWER_PORT  GPIOE
#define RS232_POWER_PIN   BIT7

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
    
    // GPE 2, 3, 4, 5 //TX, RX, RTS, CTS
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<8)) | (0x9<<8));
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<12)) | (0x9<<12));
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<16)) | (0x9<<16));
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<20)) | (0x9<<20));     
    
    //Power pin GPE6
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    UART1SetPower(FALSE);
    
    //RS232 Power pin GPE7
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(RS232_POWER_PORT, RS232_POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    UART1SetRS232Power(FALSE);
    
    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = baudRate;
    param.ucUartNo = UART1;
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
    //retval = uartIoctl(param.ucUartNo, UART_IOC_SETTXMODE, UARTINTMODE , 0);
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
BOOL UART1DrvInit(UINT32 baudRate)
{
    int retval;
    sysprintf("UART1DrvInit!!\n");
    retval = hwInit(baudRate);
    return retval;
}
INT32 UART1Write(PUINT8 pucBuf, UINT32 uLen)
{
    INT32  reVal;
    
    /* DEBUG LED *///terninalPrintf("DebugSend>=");
    /* DEBUG LED *///for(int i=0;i<uLen;i++)
    /* DEBUG LED */// terninalPrintf("%02x ",pucBuf[i]);
    /* DEBUG LED *///terninalPrintf("\n  ");
                   /*
                   for(int k=0;k<uLen;k++)
                   {
                        if((pucBuf[k] >= 0x20) && (pucBuf[k] <= 0x7E))
                            terninalPrintf("%c",pucBuf[k]);
                   } 
                   terninalPrintf("\n"); 
                   */    
    
    /* DEBUG LED */terninalPrintf("\r");
    /* DEBUG LED */for(int i=0;i<uLen;i++)
    /* DEBUG LED */ terninalPrintf("\r");
    /* DEBUG LED */terninalPrintf("\r");
                  
                   for(int k=0;k<uLen;k++)
                   {
                        if((pucBuf[k] >= 0x20) && (pucBuf[k] <= 0x7E))
                            terninalPrintf("\r");
                   } 
                   terninalPrintf("\r"); 
    
    
    reVal = uartWrite(param.ucUartNo, pucBuf, uLen);
    return reVal;
}
INT32 UART1Read(PUINT8 pucBuf, UINT32 uLen)
{
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART1ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART1SetPower(BOOL flag)
{
    return FALSE;
}

BOOL UART1SetRS232Power(BOOL flag)
{
    if(flag)
    {         
        GPIO_SetBit(RS232_POWER_PORT, RS232_POWER_PIN);    
    }
    else
    {
        GPIO_ClrBit(RS232_POWER_PORT, RS232_POWER_PIN);         
    }
    return TRUE;
}

INT UART1Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

void UART1SetRTS(BOOL flag)
{
    if(flag)
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    else
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
}

void UART1FlushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (UART1Ioctl(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
    


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


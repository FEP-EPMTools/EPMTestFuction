/**************************************************************************//**
* @file     uart2drv.c
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
#include "uart2drv.h"
#include "ptccamdrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//GPF10 Power pin
#define POWER_PORT  GPIOF
#define POWER_PIN   BIT10

//GPG9 RS232 Power pin
#define RS232_POWER_PORT  GPIOG
#define RS232_POWER_PIN   BIT9
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
    
    // GPF11, 12, 13, 14 //TX, RX, RTS, CTS
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0x9<<12));
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0x9<<16));
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0x9<<20)); 
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0x9<<24)); 
    
    //GPF10 Power pin
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    UART2SetPower(FALSE);
    
    //RS232 Power pin GPG9
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xFu<<4)) | (0x0u<<4));
    GPIO_OpenBit(RS232_POWER_PORT, RS232_POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    UART2SetRS232Power(FALSE);
    
    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = baudRate;
    param.ucUartNo = UART2;
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
BOOL UART2DrvInit(UINT32 baudRate)
{
    int retval = TRUE;;
    sysprintf("UART2DrvInit!!\n");
    retval = hwInit(baudRate);
    return retval;
}
INT32 UART2Write(PUINT8 pucBuf, UINT32 uLen)
{
    return uartWrite(param.ucUartNo, pucBuf, uLen);
}
INT32 UART2Read(PUINT8 pucBuf, UINT32 uLen)
{
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART2ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART2SetPower(BOOL flag)
{
    
    if(GPIO_ReadBit(GPIOJ, BIT4))
    {
        if(flag)
        {         
            GPIO_SetBit(POWER_PORT, POWER_PIN);   
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x1<<18));           
        }
        else
        {
            GPIO_ClrBit(POWER_PORT, POWER_PIN);
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x0<<18));             
        }
    }
    else
    {
        if(flag)
        {         
            GPIO_SetBit(GPIOI, BIT15);   
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x1<<18));           
        }
        else
        {
            GPIO_ClrBit(GPIOI, BIT15);
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x0<<18));             
        }
    }
    return TRUE;
}
BOOL UART2SetRS232Power(BOOL flag)
{
    if(flag)
    {         
        GPIO_ClrBit(RS232_POWER_PORT, RS232_POWER_PIN);     
    }
    else
    {
        GPIO_SetBit(RS232_POWER_PORT, RS232_POWER_PIN);    
            
    }
    return TRUE;
}
INT UART2Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


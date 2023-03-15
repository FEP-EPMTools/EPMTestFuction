/**************************************************************************//**
* @file     uart4drv.c
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
#include "uart4drv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//GPG7 Power pin
#define POWER_PORT  GPIOG
#define POWER_PIN   BIT7
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
    
    // GPH 8, 9, 10, 11 //TX, RX, RTS, CTS
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<<0)) | (0x9<<0));
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<<4)) | (0x9<<4));
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<<8)) | (0x9<<8));
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<<12)) | (0x9<<12));     
    
    //Power pin GPG7
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP);
    //GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, PULL_UP);    
    UART4SetPower(FALSE);
    
    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = baudRate;
    param.ucUartNo = UART4;
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
BOOL UART4DrvInit(UINT32 baudRate)
{
    int retval;
    sysprintf("UART4DrvInit!!\n");
    retval = hwInit(baudRate);
    return retval;
}
INT32 UART4Write(PUINT8 pucBuf, UINT32 uLen)
{
    INT32  reVal;
    
    /*
    terninalPrintf("Modem>=");
    for(int i=0;i<uLen;i++)
    terninalPrintf("%02x ",pucBuf[i]);
    terninalPrintf("\n  ");
                           
    for(int k=0;k<uLen;k++)
    {
        if((pucBuf[k] >= 0x20) && (pucBuf[k] <= 0x7E))
            terninalPrintf("%c",pucBuf[k]);
    } 
    terninalPrintf("\n"); 
    */
    
                           
    reVal = uartWrite(param.ucUartNo, pucBuf, uLen);
    return reVal;
}
INT32 UART4Read(PUINT8 pucBuf, UINT32 uLen)
{
    
    /*
    INT32 temp;
    memset(pucBuf,0x00,uLen);
    temp = uartRead(param.ucUartNo, pucBuf, uLen);
    
    terninalPrintf("<=");
    int counter = 0; 
    for(int i=0;i<uLen;i++)
    {
        if(i>2)
        {                                    
            if((pucBuf[i]==0x00) && (pucBuf[i-1]==0x0A) && (pucBuf[i-2]==0x0D))
            {
                counter = i;
                break;
            }
            if((pucBuf[i]==0x00) && (pucBuf[i-1]==0x00) && (pucBuf[i-2]==0x00))
            {
                //counter = i;
                break;
            }
        }
        terninalPrintf("%02x ",pucBuf[i]);
    }
    terninalPrintf("\n  ");
    for(int k=0;k<counter;k++)
    {
        if((pucBuf[k] >= 0x20) && (pucBuf[k] <= 0x7E))
            terninalPrintf("%c",pucBuf[k]);
    } 
    terninalPrintf("\n"); 
    
    return temp;  
    */
    
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART4ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART4SetPower(BOOL flag)
{
    if(flag)
    {         
        GPIO_SetBit(POWER_PORT, POWER_PIN);   
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<20)) | (0x1<<20));         
    }
    else
    {
        GPIO_ClrBit(POWER_PORT, POWER_PIN);  
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<20)) | (0x0<<20));        
    }
    return TRUE;
}
BOOL UART4SetRS232Power(BOOL flag)
{
    return FALSE;
}
INT UART4Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

void UART4SetRTS(BOOL flag)
{
    if(flag)
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    else
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
}

void UART4FlushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (UART4Ioctl(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
    


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


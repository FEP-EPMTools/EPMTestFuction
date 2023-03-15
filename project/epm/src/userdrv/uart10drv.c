/**************************************************************************//**
* @file     uart10drv.c
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
#include "uart10drv.h"
#include "hwtester.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//GPB10 Power pin
#define POWER_PORT  GPIOB
#define POWER_PIN   BIT10

//GPB11 RS232 Power pin
#define RS232_POWER_PORT  GPIOB
#define RS232_POWER_PIN   BIT11

//GPB3 3.3V Power Pin (for TRS3232EI)
#define POWER_PORT_33V  GPIOB
#define POWER_PIN_33V   BIT3

//GPB4 5.0V Power Pin (for RS232)
#define POWER_PORT_50V  GPIOB
#define POWER_PIN_50V   BIT4

//GPG6 12.0V Power Pin (for Credit Card Reader)
#define POWER_PORT_12V  GPIOG
#define POWER_PIN_12V   BIT6

//NCP(No-wired), TEN(Terminal Enable Input, GPIO PA14), NPU(No-wired)
#define BV1000_TEN_PORT     GPIOA
#define BV1000_TEN_PIN      BIT14
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UART_T param;
//static UINT8 RxBuffer[RX_BUFF_LEN];
//static SemaphoreHandle_t xSemaphore;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(UINT32 baudRate)
{
    int retval;
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    // GPB12, 13, 14, 15 //TX, RX, RTS, CTS
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<16)) | (0x9<<16));
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<20)) | (0x9<<20));
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<24)) | (0x9<<24));
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xFu<<28)) | (0x9u<<28));     
    
    //if (GPIO_ReadBit(GPIOJ, BIT0)) //if SW6 F1 high, test funtion mode
    if (readMBtestFunc())
    {
        //Power pin GPB10
        outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<8)) | (0x0<<8));
        GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
        UART10SetPower(FALSE);
        
        //RS232 Power pin GPB11
        outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xFu<<12)) | (0x0u<<12));
        GPIO_OpenBit(RS232_POWER_PORT, RS232_POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
        UART10SetRS232Power(FALSE);
    }
    else
    {
        //5.0V Power pin GPB4
        outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xFu<<16)) | (0x0u<<16));
        GPIO_OpenBit(POWER_PORT_50V, POWER_PIN_50V, DIR_OUTPUT, NO_PULL_UP); 
        UART10SetRS232Power(FALSE);

        //3.3V Power pin GPB3
        outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xFu<<12)) | (0x0u<<12));
        GPIO_OpenBit(POWER_PORT_33V, POWER_PIN_33V, DIR_OUTPUT, NO_PULL_UP); 
        /*
        //12.0V Power pin GPG6
        outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xFu<<24)) | (0x0u<<24));
        GPIO_OpenBit(POWER_PORT_12V, POWER_PIN_12V, DIR_OUTPUT, NO_PULL_UP);
        UART10SetPower(FALSE);
        */ //already setting 12V in main function modify by Steven 20200508
        //TEN Pin GPA14
        outpw(REG_SYS_GPA_MFPH,(inpw(REG_SYS_GPA_MFPH) & ~(0xFu<<24)) | (0x0u<<24));
        GPIO_OpenBit(BV1000_TEN_PORT, BV1000_TEN_PIN, DIR_OUTPUT, NO_PULL_UP);
    }
    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = baudRate;
    param.ucUartNo = UARTA;
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
BOOL UART10DrvInit(UINT32 baudRate)
{
    int retval;
    sysprintf("UART10DrvInit!!\n");
    retval = hwInit(baudRate);
    return retval;
}
INT32 UART10Write(PUINT8 pucBuf, UINT32 uLen)
{
    INT32  reVal;
    
    /* DEBUG LED *///terninalPrintf("CADSend>=");
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
    //terninalPrintf("\r");
    
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
INT32 UART10Read(PUINT8 pucBuf, UINT32 uLen)
{
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART10ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART10SetPower(BOOL flag)
{
    //if (GPIO_ReadBit(GPIOJ, BIT0)) //if SW6 F1 high, test funtion mode
    if(readMBtestFunc())
    {
        if(flag)
        {         
            //GPIO_SetBit(POWER_PORT, POWER_PIN); 
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<26)) | (0x1<<26));    
            GPIO_ClrBit(POWER_PORT_33V, POWER_PIN_33V);            
        }
        else
        {
            //GPIO_ClrBit(POWER_PORT, POWER_PIN);    
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<26)) | (0x0<<26)); 
            GPIO_SetBit(POWER_PORT_33V, POWER_PIN_33V);            
        }
    }
    else
    {
        if(flag)
        {
            // GPB12, 13, 14, 15 //TX, RX, RTS, CTS
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<16)) | (0x9<<16));
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<20)) | (0x9<<20));
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<24)) | (0x9<<24));
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xFu<<28)) | (0x9u<<28));
            GPIO_ClrBit(POWER_PORT_33V, POWER_PIN_33V);
            GPIO_SetBit(POWER_PORT_12V, POWER_PIN_12V);
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<26)) | (0x1<<26));
            GPIO_SetBit(BV1000_TEN_PORT, BV1000_TEN_PIN);
        }
        else
        {
            GPIO_ClrBit(BV1000_TEN_PORT, BV1000_TEN_PIN);
           // GPIO_ClrBit(POWER_PORT_12V, POWER_PIN_12V);   not need close 12V modify by Steven 20200508
            GPIO_SetBit(POWER_PORT_33V, POWER_PIN_33V);
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<26)) | (0x0<<26));
            // GPB12, 13, 14, 15 //TX, RX, RTS, CTS
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<16)) | (0<<16));
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<20)) | (0<<20));
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<24)) | (0<<24));
            outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xFu<<28)) | (0<<28));
        }                
    }
    return TRUE;
}
BOOL UART10SetRS232Power(BOOL flag)
{
    //if (GPIO_ReadBit(GPIOJ, BIT0)) //if SW6 F1 high, test funtion mode
    if(readMBtestFunc())
    {
        if(flag)
        {         
            GPIO_ClrBit(RS232_POWER_PORT, RS232_POWER_PIN);    
               
        }
        else
        {
            GPIO_SetBit(RS232_POWER_PORT, RS232_POWER_PIN);      
        }
    }
    else
    {
        if(flag)
        {
            GPIO_SetBit(POWER_PORT_50V, POWER_PIN_50V);    
        }
        else
        {
            GPIO_ClrBit(POWER_PORT_50V, POWER_PIN_50V);      
        }
    }
    return TRUE;
}
INT UART10Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

void UART10SetRTS(BOOL flag)
{
    if(flag)
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    else
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
}

void UART10FlushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (UART10Ioctl(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
    


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


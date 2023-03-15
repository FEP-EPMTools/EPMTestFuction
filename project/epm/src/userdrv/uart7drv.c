/**************************************************************************//**
* @file     uart7drv.c
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
#include "uart7drv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//GPB10 5V Power pin
#define POWER_PORT  GPIOB
#define POWER_PIN   BIT10
 
 #if(SUPPORT_HK_10_HW)
    //GPB1 extern select pin
    #define EXT_SEL_PORT  GPIOB
    #define EXT_SEL_PIN   BIT1
#endif
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UART_T param;
static UINT32 baudRateTemp = 115200;
static BOOL OTAinitFlag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(UINT32 baudRate)
{
    int retval;
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    // GPG 4, 5 //TX, RX
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<16)) | (0x9<<16));
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<20)) | (0x9<<20));
    
    
    //5V Power pin GPB10
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xFu<<8)) | (0x0u<<8));
    GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    if(OTAinitFlag == FALSE)
        UART7SetPower(FALSE);
    
    #if(SUPPORT_HK_10_HW)
    //GPB0 extern select pin
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(EXT_SEL_PORT, EXT_SEL_PIN, DIR_OUTPUT, NO_PULL_UP); 
    //GPIO_ClrBit(EXT_SEL_PORT, EXT_SEL_PIN); 
    GPIO_SetBit(EXT_SEL_PORT, EXT_SEL_PIN); 
    #endif 
    
    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = baudRate;
    param.ucUartNo = UART7;
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
BOOL UART7DrvInit(UINT32 baudRate)
{
    int retval;
    sysprintf("UART7DrvInit!!\n");
    baudRateTemp = baudRate;
    retval = hwInit(baudRate);
    return retval;
}
INT32 UART7Write(PUINT8 pucBuf, UINT32 uLen)
{
    INT32  reVal;
    reVal = uartWrite(param.ucUartNo, pucBuf, uLen);
    /*
    terninalPrintf("UART7Write >= ");
    for(int i=0;i<uLen;i++)
        terninalPrintf("%02x ",*(pucBuf+i));
    terninalPrintf("\r\n");
    */
    return reVal;
}
INT32 UART7Read(PUINT8 pucBuf, UINT32 uLen)
{
    /*
    INT32 temp;
    memset(pucBuf,0x00,uLen);
    temp = uartRead(param.ucUartNo, pucBuf, uLen);
    if(temp > 0)
    {        
        terninalPrintf("UART7Read <= ");
        int counter = 0; 
        for(int i=0;i<temp;i++)
            terninalPrintf("%02x ",pucBuf[i]);    
        terninalPrintf("\r\n");
    }        
    return temp;  
    */
    
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART7ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART7SetPower(BOOL flag)
{
    //sysprintf("UART7SetPower [ %d ]!! enter\n", flag);
    if(flag)
    {     
        //hwInit(baudRateTemp); 
        //GPIO_SetBit(POWER_PORT, POWER_PIN); 
        GPIO_ClrBit(POWER_PORT, POWER_PIN);    
        vTaskDelay(100/portTICK_RATE_MS);          
        //vTaskDelay(1000/portTICK_RATE_MS);      
        // GPG 4, 5 //TX, RX
        outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<16)) | (0x9<<16));
        outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<20)) | (0x9<<20));
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<23)) | (0x1<<23));   
            
    }
    else
    {
        GPIO_SetBit(POWER_PORT, POWER_PIN); 
        //GPIO_ClrBit(POWER_PORT, POWER_PIN);
        vTaskDelay(200/portTICK_RATE_MS);
        // GPG 4, 5 //TX, RX
        outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<23)) | (0x0<<23));  
        
        outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<16)) | (0<<16));
        outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<20)) | (0<<20)); 
        
        
               
            
    }
    //sysprintf("UART7SetPower [ %d ]!! exit\n", flag);
    return TRUE;
}

BOOL UART7SetRS232Power(BOOL flag)
{
    return FALSE;
}
INT UART7Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    if(uCmd == 80)
    {
        if(uArg0 == 1)
            OTAinitFlag = TRUE;
        else 
            OTAinitFlag = FALSE;
        return 0;
    }
    return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

void UART7SetRTS(BOOL flag)
{
    if(flag)
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    else
        uartIoctl(param.ucUartNo, UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
}

void UART7FlushBuffer(void)
{
    //sysprintf(" --> CardFlushBuffer\n");
    //if (pUartInterface->ioctlFunc(UART_IOC_FLUSH_TX_BUFFER, 0, 0) != 0) {
   //     sysprintf("Set TX Flush fail!\n");
    //    return;
    //}
    if (UART7Ioctl(UART_IOC_FLUSH_RX_BUFFER, 0, 0) != 0) {
        sysprintf("Set RX Flush fail!\n");
        return;
    }
}
    


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


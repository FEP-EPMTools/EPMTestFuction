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
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/


//(octopus mode: connect octopus reader pwrctrl[Power Source: J17] pin, high:shut down)
//GPI4 Ooctopus Power Ctrl pin //from flash1 wp(PI4)
#define OCTOPUS_POWER_CTRL_PORT  GPIOI
#define OCTOPUS_POWER_CTRL_PIN   BIT4

//(normal  mode: connect 5v power pin)
//GPF10 5V Power pin 
#define POWER_PORT  GPIOF
#define POWER_PIN   BIT10


//GPG9 RS232/rs485 Power pin 
//(normal  mode: connect rs232 chip power pin)
//(octopus mode: connect rs485 chip power pin)
#define CONVERT_CHIP_PORT  GPIOG
#define CONVERT_CHIP_PIN   BIT9

//PG6 12V power
//(only use in octopus mode: connect 12v power pin)
#define POWER_12V_PORT  GPIOG
#define POWER_12V_PIN   BIT6

//PI15 12V power
//(only use in octopus mode: connect 12v power pin)
#define POWER_12V_2_PORT  GPIOI
#define POWER_12V_2_PIN   BIT15

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UART_T param;
static BOOL octopusModeFlag = FALSE;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(UINT32 baudRate)
{
    int retval;
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    // GPF11, 12 //TX, RX
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0x9<<12));
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0x9<<16));
    
    
    if(octopusModeFlag)
    //if((octopusModeFlag) && GPIO_ReadBit(GPIOJ, BIT1))
    {//GPF13 rts(DE active high), 14 cts (RE active low) //output low
        outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0<<20));
        outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0<<24));
        
        GPIO_OpenBit(GPIOF, BIT13, DIR_OUTPUT, NO_PULL_UP); 
        GPIO_OpenBit(GPIOF, BIT14, DIR_OUTPUT, NO_PULL_UP); 
        
        //disable
        GPIO_ClrBit(GPIOF, BIT13); //RTS low disable TX
        GPIO_SetBit(GPIOF, BIT14); //CTS high disable RX   

        
    }
    else
    {//GPF13, 14 //RTS, CTS
        outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0x9<<20)); 
        outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0x9<<24)); 
    }
    
    //GPI4 Ooctopus Power Ctrl pin
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(OCTOPUS_POWER_CTRL_PORT, OCTOPUS_POWER_CTRL_PIN, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_SetBit(OCTOPUS_POWER_CTRL_PORT, OCTOPUS_POWER_CTRL_PIN);//high:shut down
    
    //GPF10 5V Power pin
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(POWER_PORT, POWER_PIN);
    
    
    //RS232/rs485 Power pin GPG9
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xFu<<4)) | (0x0u<<4));
    GPIO_OpenBit(CONVERT_CHIP_PORT, CONVERT_CHIP_PIN, DIR_OUTPUT, NO_PULL_UP); 
    UART2SetRS232Power(FALSE);
    /*
    //12V enable pin PG6
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<24)) | (0x0u<<24));  
    GPIO_OpenBit(POWER_12V_PORT, POWER_12V_PIN, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(POWER_12V_PORT, POWER_12V_PIN);
    */
    //12V enable pin PI15
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<28)) | (0x0<<28));  
    GPIO_OpenBit(POWER_12V_2_PORT, POWER_12V_2_PIN, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(POWER_12V_2_PORT, POWER_12V_2_PIN);
    
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
BOOL UART2DrvInit(UINT32 baudRate)
{
    int retval = TRUE;;
    sysprintf("UART2DrvInit!!\n");
    retval = hwInit(baudRate);
    return retval;
}
#define UART_WRITE_SIZE 256
INT32 UART2Write(PUINT8 pucBuf, UINT32 uLen)
{
    /*               
    terninalPrintf("Reader>=");
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
    
    if(octopusModeFlag)
    //if((octopusModeFlag) && GPIO_ReadBit(GPIOJ, BIT1))
    {
        int i = 0;
        int total = 0;
        int leftLen = 0;
        int n;
        //terninalPrintf("octopusModeFlag = TRUE\n");
        for(i = 0; i< uLen/UART_WRITE_SIZE; i++)
        {            
            n = uartWrite(param.ucUartNo, pucBuf + i*UART_WRITE_SIZE, UART_WRITE_SIZE); ;//QModemWrite(buff + i*FTP_UART_WRITE_SIZE, FTP_UART_WRITE_SIZE);
            if(n != UART_WRITE_SIZE)
            {
                sysprintf(" == UART2Write write error\r\n");
                return 0;
            }
            else
            {
                total = total + n;
                sysprintf("%08d (%d:%d)\r", total, n, i*UART_WRITE_SIZE);
                
                vTaskDelay(100/portTICK_RATE_MS);
            }
        }
        leftLen = uLen-(uLen/UART_WRITE_SIZE*UART_WRITE_SIZE);
        if(leftLen > 0)
        {
            n = uartWrite(param.ucUartNo, pucBuf + uLen/UART_WRITE_SIZE*UART_WRITE_SIZE, leftLen);
            if(n != leftLen)
            {
                sysprintf(" == UART2Write write error\r\n");
                return 0;
            }
            else
            {
                total = total + n;
                sysprintf("\r\n%08d _2 (%d:%d)\r", total, n, leftLen);
            }
        }
        if(total == uLen)
        {
            sysprintf(" == UART2Write write OK\r\n");
            return uLen;
        }
        else
        {
            sysprintf(" == UART2Write write len error [%s:%s]\r\n", total == uLen);
            return uLen;
        }
            
    }
    else
    {
        
        //terninalPrintf("octopusModeFlag = FALSE\n");
                           
            /* DEBUG LED *///terninalPrintf(">=");
            /* DEBUG LED *///for(int i=0;i<uLen;i++)
            /* DEBUG LED *///terninalPrintf("%02x ",pucBuf[i]);
            /* DEBUG LED *///terninalPrintf("\n  ");
                            /*                 
                           for(int k=0;k<uLen;k++)
                           {
                                if((pucBuf[k] >= 0x20) && (pucBuf[k] <= 0x7E))
                                    terninalPrintf("%c",pucBuf[k]);
                           } 
                           terninalPrintf("\n"); 
                            */
        return uartWrite(param.ucUartNo, pucBuf, uLen);
    }
}
INT32 UART2Read(PUINT8 pucBuf, UINT32 uLen)
{
    
    /* 
    INT32 temp;
    memset(pucBuf,0x00,uLen);
    temp = uartRead(param.ucUartNo, pucBuf, uLen);
    if(temp > 0)
    {
        terninalPrintf("<=");
        int counter = 0; 
        for(int i=0;i<temp;i++)
        {

            terninalPrintf("%02x ",pucBuf[i]);
        }
        terninalPrintf("\n  ");
        for(int k=0;k<temp;k++)
        {
            if((pucBuf[k] >= 0x20) && (pucBuf[k] <= 0x7E))
                terninalPrintf("%c",pucBuf[k]);
        } 
        terninalPrintf("\n");
    }    
    return temp;  
    */
    
    
    
    
    
    
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BaseType_t UART2ReadWait(TickType_t time)
{
    return  uartWaitReadEvent(param.ucUartNo, time);
}

BOOL UART2SetPower(BOOL flag)
{
    if(octopusModeFlag)
    //if((octopusModeFlag) && GPIO_ReadBit(GPIOJ, BIT1))
    {          
        if(flag)
        {         
            GPIO_SetBit(POWER_12V_PORT, POWER_12V_PIN);
            GPIO_SetBit(POWER_12V_2_PORT, POWER_12V_2_PIN);
            
            // GPF11, 12, 13, 14 //TX, RX, RTS, CTS
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0x9<<12));
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0x9<<16));

            GPIO_SetBit(GPIOF, BIT13); //RTS high enable TX
            GPIO_ClrBit(GPIOF, BIT14); //CTS low enable RX
            
            //outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x1<<18)); 
            
            GPIO_ClrBit(OCTOPUS_POWER_CTRL_PORT, OCTOPUS_POWER_CTRL_PIN);              
        }
        else
        {
            GPIO_SetBit(OCTOPUS_POWER_CTRL_PORT, OCTOPUS_POWER_CTRL_PIN);//high:shut down
            
            //outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x0<<18)); 
            
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0<<12));
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0<<16));

            GPIO_ClrBit(GPIOF, BIT13); //RTS low disable TX
            GPIO_SetBit(GPIOF, BIT14); //CTS high disable RX

            //GPIO_ClrBit(POWER_12V_PORT, POWER_12V_PIN);      not need close 12V power 20200507 by Steven 
            GPIO_ClrBit(POWER_12V_2_PORT, POWER_12V_2_PIN);
        }
    }
    else
    {
        if(flag)
        {         
            // GPF11, 12, 13, 14 //TX, RX, RTS, CTS
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0x9<<12));
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0x9<<16));
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0x9<<20)); 
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0x9<<24)); 
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x1<<18)); 
            GPIO_SetBit(POWER_PORT, POWER_PIN);   
                      
        }
        else
        {
            GPIO_ClrBit(POWER_PORT, POWER_PIN);
            outpw(REG_CLK_PCLKEN0,(inpw(REG_CLK_PCLKEN0) & ~(0x1<<18)) | (0x0<<18)); 
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0<<12));
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0<<16));
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0<<20)); 
            outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0<<24));         
        }
    }
    return TRUE;
}
BOOL UART2SetRS232Power(BOOL flag)
{
    if(flag)
    {         
        GPIO_ClrBit(CONVERT_CHIP_PORT, CONVERT_CHIP_PIN);     
    }
    else
    {
        GPIO_SetBit(CONVERT_CHIP_PORT, CONVERT_CHIP_PIN);    
            
    }
    return TRUE;
}
INT UART2Ioctl(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1)
{
    if(UART_IOC_SET_OCTOPUS_MODE == uCmd)
    {
        //terninalPrintf("UART2Ioctl [UART_IOC_SET_OCTOPUS_MODE]\r\n");
        octopusModeFlag = TRUE;
        return Successful;
    }
    else
        return uartIoctl(param.ucUartNo, uCmd, uArg0, uArg1);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


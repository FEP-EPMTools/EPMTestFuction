/**************************************************************************//**
* @file     ptccamdrv.c
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
#include "ptccamdrv.h"
#include "uart10drv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define RX_BUFF_LEN   64 
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static UART_T param;
static UINT8 RxBuffer[RX_BUFF_LEN];
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(void)
{
    int retval;
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0x9<<12));
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0x9<<16));
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0x9<<20)); 
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0x9<<24));    
     
    //GPB4 Power pin
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOB, BIT4, DIR_OUTPUT, NO_PULL_UP); 
    PTCCamSetPower(FALSE);
    
    /* configure UART */
    param.uFreq = 12000000;
    param.uBaudRate = 38400;
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

static void vRxTask( void *pvparamOuteters )
{
    sysprintf("vRxTask(PTCCam) Going...\r\n");   
    PTCCamSetPower(TRUE);
    for(;;)
    {
        BaseType_t reval = uartWaitReadEvent(param.ucUartNo, portMAX_DELAY);
        //vTaskDelay(10); 
        while(1)
        {
            int retval = uartRead(param.ucUartNo, RxBuffer, RX_BUFF_LEN);
            if( retval > 0)
            {
                //sysprintf("vRxTask(PTCCam) Read %d Data ...\r\n", retval);                
                retval = UART10Write(RxBuffer, retval);                
                //sysprintf("vRxTask(PTCCam) write %d Data ...\r\n", retval);
                //}
                #if(0)
                {
                    int i;
                    char cChar;
                    for(i = 0; i< retval; i++)
                    {
                        cChar = RxBuffer[i] & 0xFF;
                        sysprintf("[%02d]: 0x%02x\r\n", i, cChar);
                    }
                }
                #endif                
            }
            else
            {
                //sysprintf("!! vRxTask Read Break ...\r\n");
                sysprintf(".");
                break;
            }
        }       
    }
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL PTCCamDrvInit(void)
{
    int retval;
    sysprintf("PTCCamDrvInit!!\n");
    retval = hwInit();
    xTaskCreate( vRxTask, "vRxTask", 1024, NULL, PTC_CAM_THREAD_PROI, NULL );
    return retval;
}
INT32 PTCCamWrite(PUINT8 pucBuf, UINT32 uLen)
{
    return uartWrite(param.ucUartNo, pucBuf, uLen);
}
INT32 PTCCamRead(PUINT8 pucBuf, UINT32 uLen)
{
    return uartRead(param.ucUartNo, pucBuf, uLen);
}
BOOL PTCCamSetPower(BOOL flag)
{
    if(flag)
    {         
        GPIO_SetBit(GPIOB, BIT4);    
    }
    else
    {
        GPIO_ClrBit(GPIOB, BIT4);         
    }
    return TRUE;
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


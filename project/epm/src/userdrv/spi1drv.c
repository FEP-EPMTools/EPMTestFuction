/**************************************************************************//**
* @file     spi1drv.c
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
#include "spi.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "spi1drv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define SPI_PORT    1
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    Spi1SetPin();
    
    spiInit(SPI_PORT);
    spiOpen(SPI_PORT);

    // set spi interface speed to 1.2MHz
    //spiIoctl(SPI_PORT, SPI_IOC_SET_SPEED, 4800000, 0);
    spiIoctl(SPI_PORT, SPI_IOC_SET_SPEED, 9600000, 0);
    //spiIoctl(SPI_PORT, SPI_IOC_SET_SPEED, 15000000, 0);
    // set spi interface speed to 2MHz
    //spiIoctl(SPI_PORT, SPI_IOC_SET_SPEED, 2000000, 0);

    // set transfer length to 8-bit
    spiIoctl(SPI_PORT, SPI_IOC_SET_TX_BITLEN, 8, 0);
    // set transfer mode
    spiIoctl(SPI_PORT, SPI_IOC_SET_MODE, SPI_MODE_0, 0);

    spiIoctl(SPI_PORT, SPI_IOC_SET_SS_ACTIVE_LEVEL, SPI_SS_ACTIVE_LOW, 0);

    spiIoctl(SPI_PORT, SPI_IOC_SET_TX_NUM, 0, 0);

    spiIoctl(SPI_PORT, SPI_IOC_SET_LSB_MSB, SPI_MSB, 0);
    
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL Spi1DrvInit(void)
{
    BOOL retval;
    sysprintf("SPI1DrvInit!!\n");
    retval = hwInit();
    return retval;
}
void Spi1Write(uint8_t buff_id, uint32_t data)
{
    //sysprintf("=>%02x ");
    //terninalPrintf("t%04X ",data);
    spiWrite(SPI_PORT, buff_id, data);
    spiIoctl(SPI_PORT, SPI_IOC_TRIGGER, 0, 0);
    while(spiGetBusyStatus(SPI_PORT));
}
uint32_t Spi1Read(uint8_t buff_id)
{
    /*
    uint32_t temp;
    temp = spiRead(SPI_PORT, buff_id);
    terninalPrintf("r%04X ",temp);
    return temp;
    */
    
    return spiRead(SPI_PORT, buff_id);
}
void Spi1ActiveCS(BOOL active)
{
    if(active)
    {// /CS: active
        //terninalPrintf("\r\n");
        spiIoctl(SPI_PORT, SPI_IOC_ENABLE_SS, SPI_SS_SS0, 0);
    }
    else
    {
        //terninalPrintf("\r\n");
        spiIoctl(SPI_PORT, SPI_IOC_DISABLE_SS, SPI_SS_SS0, 0);
    }
}
void Spi1SetPin(void)
{
    /* Configure multi function pins to SPI1 */
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<20)) | (0xB<<20));//GPI5 SS0
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<24)) | (0xB<<24));//GPI6 CLK

    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xFu<<28)) | (0xBu<<28));//GPI7 DATAO
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<0)) | (0xB<<0));//GPI8 DATAI  
}

void Spi1ResetPin(void)
{
    /* Configure multi function pins to SPI1 */
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<20)) | (0x0<<20));//GPI5 input
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<24)) | (0x0<<24));//GPI6 input

    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xFu<<28)) | (0x0<<28));//GPI7 input
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<0)) | (0x0<<0));//GPI8 input
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


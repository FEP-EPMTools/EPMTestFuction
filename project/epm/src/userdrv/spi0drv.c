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
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "spi0drv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define SPI_PORT    0

//GPI10 cs pin
#define CS_PORT  GPIOI
#define CS_PIN   BIT10
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    Spi0SetPin();
    
    spiInit(SPI_PORT);
    spiOpen(SPI_PORT);

    // set spi interface speed to 1.2MHz
    spiIoctl(SPI_PORT, SPI_IOC_SET_SPEED, 4800000, 0);
    // set spi interface speed to 2MHz
    //spiIoctl(SPI_PORT, SPI_IOC_SET_SPEED, 2000000, 0);
    //spiIoctl(SPI_PORT, SPI_IOC_SET_SPEED, 1000000, 0);
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
BOOL Spi0DrvInit(void)
{
    BOOL retval;
    sysprintf("SPI0DrvInit!!\n");
    retval = hwInit();
    return retval;
}
void Spi0Write(uint8_t buff_id, uint32_t data)
{
    spiWrite(SPI_PORT, buff_id, data);
    spiIoctl(SPI_PORT, SPI_IOC_TRIGGER, 0, 0);
    while(spiGetBusyStatus(SPI_PORT));
}
uint32_t Spi0Read(uint8_t buff_id)
{
    return spiRead(SPI_PORT, buff_id);
}
void Spi0ActiveCS(BOOL active)
{
    if(active)
    {// /CS: active
        GPIO_ClrBit(CS_PORT, CS_PIN);
    }
    else
    {
        GPIO_SetBit(CS_PORT, CS_PIN);
    }
}
void Spi0SetPin(void)
{
    /* Configure multi function pins to SPI0 */
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<8)) | (0x0<<8));//GPI10 SS0
    GPIO_OpenBit(CS_PORT, CS_PIN, DIR_OUTPUT, NO_PULL_UP); 
    
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xFu<<28)) | (0xBu<<28));//GPB7 CLK
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<0)) | (0xB<<0));//GPB8 DATAO
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<4)) | (0xB<<4));//GPB9 DATAI 
}

void Spi0ResetPin(void)
{
    /* Configure multi function pins to SPI0 */
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<8)) | (0x0<<8));//GPI10 SS0
    GPIO_OpenBit(CS_PORT, CS_PIN, DIR_INPUT, NO_PULL_UP); 
    
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xFu<<28)) | (0x0<<28));//GPB7 input
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<0)) | (0x0<<0));//GPB8 input
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<4)) | (0x0<<4));//GPB9 input 
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


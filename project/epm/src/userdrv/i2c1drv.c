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
#include "i2c.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "i2c1drv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define I2C_PORT    1
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    I2c1SetPin();
    i2cInit(I2C_PORT);
    int32_t rtval = i2cOpen((PVOID)I2C_PORT);
    if(rtval < 0) 
    {
        sysprintf("Open I2C1 error!\n");
        return FALSE;
    }
    i2cIoctl(I2C_PORT, I2C_IOC_SET_SPEED, 100, 0);
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL I2c1DrvInit(void)
{
    BOOL retval;
    sysprintf("I2c1DrvInit!!\n");
    retval = hwInit();
    return retval;
}

int32_t I2c1Ioctl(uint32_t cmd, uint32_t arg0, uint32_t arg1)
{
    return i2cIoctl(I2C_PORT, cmd, arg0, arg1);
}
int32_t I2c1Read(uint8_t* buf, uint32_t len)
{
    return i2cRead(I2C_PORT, buf, len);
}
int32_t I2c1Write(uint8_t* buf, uint32_t len)
{
    return i2cWrite(I2C_PORT, buf, len);
}

void I2c1enableCriticalSectionFunc(BOOL flag)
{
    if(flag)
        i2cEnterCriticalSection(I2C_PORT);
    else
        i2cExitCriticalSection(I2C_PORT);
}

void I2c1SetPin(void)
{
    sysprintf("I2c1SetPin!!\n");
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<8)) | (0x8<<8));//GPG2 I2C1_SCL
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<12)) | (0x8<<12));//GPG3 I2C1_SDA
}

void I2c1ResetPin(void)
{
    sysprintf("I2c1ResetPin!!\n");
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<8)) | (0x0<<8));//GPG2 I2C1_SCL
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<12)) | (0x0<<12));//GPG3 I2C1_SDA
    
    GPIO_OpenBit(GPIOG, BIT2, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(GPIOG, BIT2); 
    GPIO_OpenBit(GPIOG, BIT3, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(GPIOG, BIT3); 
}

void I2c1ResetInputPin(void)
{
    sysprintf("I2c1ResetInputPin!!\n");
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<8)) | (0x0<<8));//GPG2 I2C1_SCL
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<12)) | (0x0<<12));//GPG3 I2C1_SDA
    
    GPIO_OpenBit(GPIOG, BIT2, DIR_INPUT, NO_PULL_UP); 
    //GPIO_ClrBit(GPIOG, BIT2); 
    GPIO_OpenBit(GPIOG, BIT3, DIR_INPUT, NO_PULL_UP); 
    //GPIO_ClrBit(GPIOG, BIT3); 
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


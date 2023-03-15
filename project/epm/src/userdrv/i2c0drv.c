/**************************************************************************//**
* @file     spi0drv.c
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
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "i2c0drv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define I2C_PORT    0
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    //init i2c0
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<0)) | (0x8<<0));//GPG0 I2C0_SCL
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<4)) | (0x8<<4));//GPG1 I2C0_SDA
    i2cInit(I2C_PORT);
    int32_t rtval = i2cOpen((PVOID)I2C_PORT);
    if(rtval < 0) 
    {
        sysprintf("Open I2C0 error!\n");
    }
    i2cIoctl(I2C_PORT, I2C_IOC_SET_SPEED, 100, 0);
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL I2c0DrvInit(void)
{
    BOOL retval;
    sysprintf("I2c0DrvInit!!\n");
    retval = hwInit();
    return retval;
}

int32_t I2c0Ioctl(uint32_t cmd, uint32_t arg0, uint32_t arg1)
{
    return i2cIoctl(I2C_PORT, cmd, arg0, arg1);
}
int32_t I2c0Read(uint8_t* buf, uint32_t len)
{
    return i2cRead(I2C_PORT, buf, len);
}
int32_t I2c0Write(uint8_t* buf, uint32_t len)
{
    return i2cWrite(I2C_PORT, buf, len);
}
void I2c0enableCriticalSectionFunc(BOOL flag)
{
    if(flag)
        i2cEnterCriticalSection(I2C_PORT);
    else
        i2cExitCriticalSection(I2C_PORT);
}
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


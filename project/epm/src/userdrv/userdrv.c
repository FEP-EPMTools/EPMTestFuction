/**************************************************************************//**
* @file     userdrv.c
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

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "userdrv.h"
#include "ptccamdrv.h"
#include "uart10drv.h"
#include "buzzerdrv.h"
#include "leddrv.h"
#include "batterydrv.h"
#include "dipdrv.h"
#include "flashdrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static BOOL FakeDrvInit(BOOL testMode);
static userDrvInitFunctionList mUserDrvInitFunctionList[] = {
                                                            {"FakeDrvInit", FakeDrvInit},
                                                            #if(ENABLE_BUZZER_DRIVER)
                                                            {"BuzzerDrv", BuzzerDrvInit},
                                                            #endif
                                                            #if(ENABLE_LED_DRIVER)
                                                            {"LedDrv", LedDrvInit},
                                                            #endif
                                                            #if(ENABLE_BATTERY_DRIVER)
                                                            {"BatteryDrv", BatteryDrvInit},
                                                            #endif         
                                                            {"", NULL}};
static UartInterface* pUartInterface = NULL;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL FakeDrvInit(BOOL testMode)      
{
    pUartInterface = UartGetInterface(UART_1_INTERFACE_INDEX);
    if(pUartInterface != NULL)
    {
        pUartInterface->setPowerFunc(FALSE);
        pUartInterface->setRS232PowerFunc(FALSE);        
    }
    
    pUartInterface = UartGetInterface(UART_3_INTERFACE_INDEX);
    if(pUartInterface != NULL)
    {
        pUartInterface->setPowerFunc(FALSE);
        pUartInterface->setRS232PowerFunc(FALSE);        
    }
    
    pUartInterface = UartGetInterface(UART_7_INTERFACE_INDEX);
    if(pUartInterface != NULL)
    {
        pUartInterface->setPowerFunc(FALSE);
        pUartInterface->setRS232PowerFunc(FALSE);        
    }
    #if(ENABLE_MODEM_AGENT_DRIVER)
    
    #else
    pUartInterface = UartGetInterface(UART_4_INTERFACE_INDEX);
    if(pUartInterface != NULL)
    {
        pUartInterface->setPowerFunc(FALSE);
        pUartInterface->setRS232PowerFunc(FALSE);        
    }
    #endif
    /*
    //12V enable pin
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<24)) | (0x0u<<24));  
    GPIO_OpenBit(GPIOG, BIT6, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(GPIOG, BIT6);
    */  // not need close 12V power 20200507 by Steven 
    #if(SUPPORT_HK_10_HW)  
    //Sensor board DC5V GPB6
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOB, BIT6, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(GPIOB, BIT6);  
    #endif
    
    return TRUE;
}    
                                                            
static BOOL userDrvInit(BOOL testModeFlag)
{
    int i;
    sysprintf("  ==> userDrvInit start...\r\n");

    for(i = 0; ; i++)
    {
        if(mUserDrvInitFunctionList[i].func != NULL)
        {
            if(mUserDrvInitFunctionList[i].func(testModeFlag))
            {
                sysprintf("  = [%02d]: %s OK... =\r\n", i, mUserDrvInitFunctionList[i].drvName);
            }
            else
            {
                sysprintf("  = [%02d]: %s ERROR... =\r\n", i, mUserDrvInitFunctionList[i].drvName);
                return FALSE;
            }
        }
        else
        {
            
            break;
        }
    }    
    sysprintf("  ==> userDrvInit end...\r\n");
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL UserDrvInit(BOOL testModeFlag)
{
    sysprintf("UserDrvInit!!\n");
    return userDrvInit(testModeFlag);
}



/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


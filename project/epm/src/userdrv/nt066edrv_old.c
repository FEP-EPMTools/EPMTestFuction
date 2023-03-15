/**************************************************************************//**
* @file     nt066edrv.c
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
#include "interface.h"
#include "nt066edrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define NT066E_ID          0x30//0x20

#define RETRY       5000  /* Programming cycle may be in progress. Please refer to 24LC64 datasheet */

#if(SUPPORT_HK_10_HW)   
    #define NT066E_RESET_PIN_PORT  GPIOE
    #define NT066E_RESET_PORT_BIT  BIT9

    #define NT066E_POWER_PIN_PORT  GPIOE
    #define NT066E_POWER_PORT_BIT  BIT8

#else
    #define NT066E_RESET_PIN_PORT  GPIOG
    #define NT066E_RESET_PORT_BIT  BIT1

    #define NT066E_POWER_PIN_PORT  GPIOG
    #define NT066E_POWER_PORT_BIT  BIT0
#endif
//#define NT066E_RESET_PIN_PORT  GPIOG
//#define NT066E_RESET_PORT_BIT  BIT1

//#define NT066E_POWER_PIN_PORT  GPIOG
//#define NT066E_POWER_PORT_BIT  BIT0

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static I2cInterface* pI2cInterface;
static keyHardwareCallbackFunc pKeyHardwareCallbackFunc;
static SemaphoreHandle_t xNT066ESemaphore;
static TickType_t threadWaitTime = portMAX_DELAY;
static int intTimes;
static int rErrTimes, wErrTimes;
static BOOL enableTestNT066E = FALSE;
static BOOL keyDownFlag[8] = {0};
static BOOL initFlag = FALSE;

#warning
BOOL NT066EResetChip(void);
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL sendNT066EWriteCmd(uint8_t cmd, uint8_t* paraData, uint8_t dataLen)
{
    int j;
    pI2cInterface->enableCriticalSectionFunc(TRUE);
    pI2cInterface->ioctlFunc(I2C_IOC_SET_DEV_ADDRESS, NT066E_ID, 0);
    if(cmd != 0)
    {
        pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, cmd, 1);
    }
    else
    {
        pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, 0, 0);
    }
    j = RETRY;
    while(j-- > 0) 
    {
        if(pI2cInterface->writeFunc(paraData, dataLen) == dataLen)
            break;
    }
    pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, 0, 0);
    if(j <= 0)
    {
        sysprintf("\n !!! NT066E Write ERROR !!!\n");
        wErrTimes++;
        pI2cInterface->enableCriticalSectionFunc(FALSE);
        return FALSE;
    }
    else
    {
        pI2cInterface->enableCriticalSectionFunc(FALSE);
        return TRUE;
    }
}

static BOOL sendNT066EReadCmd(uint8_t cmd, uint8_t* paraData, uint8_t dataLen)
{
    int j;
    pI2cInterface->enableCriticalSectionFunc(TRUE);
    pI2cInterface->ioctlFunc(I2C_IOC_SET_DEV_ADDRESS, NT066E_ID, 0);
    if(cmd != 0)
    {
        pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, cmd, 1);
    }
    else
    {
        //sysprintf("\n !!! @@@@@  NT066E Read I2C_IOC_SET_SUB_ADDRESS   @@@@ !!!\n");
        pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, 0, 0);
    }
    j = RETRY;
    while(j-- > 0) 
    {
        if(pI2cInterface->readFunc(paraData, dataLen) == dataLen)
            break;
    }
    pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, 0, 0);
    if(j <= 0)
    {
        sysprintf("\n !!! NT066E Read ERROR !!!\n");
        rErrTimes++;
        pI2cInterface->enableCriticalSectionFunc(FALSE);
        return FALSE;
    }
    else
    {
        //sysprintf("\n ~~~ NT066E Read 0x%02x command: 0x%02x ~~~\n", cmd, paraData[0]);
        pI2cInterface->enableCriticalSectionFunc(FALSE);
        return TRUE;
    }
    
}

INT32 EINT1Callback(UINT32 status, UINT32 userData)
{
    //#if(BUILD_DEBUG_VERSION || JUST_TEST_NT066E)
    //sysprintf("\r\n  -#- EINT7 0x%08x [%04d] -#- \r\n", status, intTimes++);
    //sysprintf("\r\n  -#- EINT7 [%04d] -#- \r\n", intTimes++);
    
    //setPrintfFlag(FALSE);
    //sysprintf("\r\n  -#- EINT1 [%02x] -#- \r\n", GPIO_ReadPort(GPIOF)&0x3f);
    //setPrintfFlag(TRUE);
    
    if(xNT066ESemaphore != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken;        
        
        xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR( xNT066ESemaphore, &xHigherPriorityTaskWoken );
        portEXIT_SWITCHING_ISR (xHigherPriorityTaskWoken); 
    }        

    GPIO_ClrISRBit(GPIOH, BIT1);    
    return 0;
}

static int processKeyEvent(uint8_t value)
{
    int i;
    for(i = 0; i<8; i++)
    {
        if((value>>i)&0x1)
        {
            //sysprintf("\r\n~~~~Key %d DOWN Event ~~~~~\r\n", i);  
            if(keyDownFlag[i] == FALSE)
            {
                //sysprintf("  -- v -- Key [%d] Down Event  ---\r\n", i);
                keyDownFlag[i] = TRUE;
                //processKeyDownEvent(i);
                if(pKeyHardwareCallbackFunc != NULL)
                    pKeyHardwareCallbackFunc(i, KEY_HARDWARE_DOWN_EVENT);
                return KEY_HARDWARE_DOWN_EVENT;//just one key
            }
            else
            {
                //sysprintf("  -- v -- Key [%d] Down Event error, ignore ---\r\n", i);
            }
        }
        else
        {
            //sysprintf("\r\n~~~~Key %d UP Event ~~~~~\r\n", i);  
            if(keyDownFlag[i] == TRUE)
            {
                //sysprintf("  -- ^ -- Key [%d] Up Event  ---\r\n", i);
                keyDownFlag[i] = FALSE;
                //processKeyUpEvent(i);
                if(pKeyHardwareCallbackFunc != NULL)
                    pKeyHardwareCallbackFunc(i, KEY_HARDWARE_UP_EVENT);
                return KEY_HARDWARE_UP_EVENT;//just one key
            }
            else
            {
                //sysprintf("  -- ^ -- Key [%d] Up Event error, irnore ---\r\n", i);
            }
        }
    }
    return KEY_HARDWARE_ERROR_EVENT;
}
static void vNT066EDrvTask( void *pvparamOuteters )
{
    sysprintf("vNT066EDrvTask Going...\r\n");   
    uint8_t data[8];
    for(;;)
    {       
        BOOL reVal;
        BaseType_t reval = xSemaphoreTake(xNT066ESemaphore, threadWaitTime);         
        //#if(!SUPPORT_NT066E) 
        //if(GPIO_ReadBit(GPIOI, BIT2) == 0)
        //#endif
        {        
            if(enableTestNT066E) 
            { 
                sysprintf("\r\n **vNT066EDrvTask  GO **\r\n");
            }                

            //data[0] = 0x11;
            //sendNT066EWriteCmd(NULL, data, 1);  
            reVal = sendNT066EReadCmd(0x11, data, 1);

            if(reVal)
            {
                if(enableTestNT066E)
                {
                    sysprintf("~~~ NT066E Read SensReg[%d] 0x%02x ~~~\n", intTimes, data[0]);
                }
                if(KEY_HARDWARE_DOWN_EVENT == processKeyEvent(data[0]))
                {

                }
            }
            else
            {
                sysprintf("!!! NT066E Read SensReg ERROR !!!\n");
            } 
    
        } 
    }
}

void NT066ESetClkReg(uint8_t frqc, uint8_t frqf)
{
    uint8_t dataTmp[8];
    uint8_t dataTmp1[8];
    sysprintf("Start Tx ClkReg [frqc: 0x%x (b%02b), frqf: 0x%x (b%03b)]--> \n", frqc, frqc, frqf, frqf);
    if(sendNT066EReadCmd(0x36, dataTmp, 1))
    {
        sysprintf(" ~~~ NT066E Read ClkReg(1) 0x%02x ~~~\n", dataTmp[0]);
    }
    else
    {
        sysprintf(" !!! NT066E Read ERROR !!!\n");
        return;
    }   
    if(frqc != 0xff)
    {
        dataTmp[0] = (dataTmp[0] & ~(0x3<<3)) | (frqc&0x3)<<3; 
    }
    if(frqf != 0xff)
    {
        dataTmp[0] = (dataTmp[0] & ~(0x7<<0)) | (frqf&0x7)<<0;
    }
    sysprintf(" ~~~ NT066E write ClkReg 0x%02x ~~~\n", dataTmp[0]);
    sendNT066EWriteCmd(0x35, dataTmp, 1);   

    if(sendNT066EReadCmd(0x36, dataTmp1, 1))
    {
        
        if(dataTmp1[0] != dataTmp[0])
        {
            sysprintf(" ~~~ NT066E Tx ClkReg error 0x%02x : 0x%02x ~~~\n", dataTmp[0], dataTmp1[0]);
            return;
        }
        else
        {
            sysprintf(" ~~~ NT066E Read ClkReg(2) 0x%02x (b%08b)~~~\n", dataTmp1[0], dataTmp1[0]);
        }
    }
    else
    {
        sysprintf(" !!! NT066E Read ERROR !!!\n");
        return;
    }   
}
static BOOL hwInit(void)
{
    //uint8_t dataTmp[8];
    //uint8_t dataTmp1[8];
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    /* Set PH1 to EINT7 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<4)) | (0xF<<4));    
    /* Configure PH1 to input mode */
    GPIO_OpenBit(GPIOH, BIT1, DIR_INPUT, PULL_UP);
    /* Confingure PH1 to falling-edge trigger */
    GPIO_EnableTriggerType(GPIOH, BIT1, BOTH_EDGE/*FALLING*/);
    //EINT1
    GPIO_EnableEINT(NIRQ1, (GPIO_CALLBACK)EINT1Callback, 0);
    
    GPIO_ClrISRBit(GPIOI, BIT2);    
    
    #if(SUPPORT_HK_10_HW)
    //GPE8 Power pin
    outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<<0)) | (0x0<<0));
    #else
    //GPG0 Power pin
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<0)) | (0x0<<0));
    #endif    
//    //GPG0 Power pin
//    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<0)) | (0x0<<0));
    
    GPIO_OpenBit(NT066E_POWER_PIN_PORT, NT066E_POWER_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);  
    GPIO_SetBit(NT066E_POWER_PIN_PORT, NT066E_POWER_PORT_BIT); 
    sysDelay(200/portTICK_RATE_MS);
    GPIO_ClrBit(NT066E_POWER_PIN_PORT, NT066E_POWER_PORT_BIT); 
    
   #if(SUPPORT_HK_10_HW)
    //GPE9 reset pin
    outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<<4)) | (0x0<<4));
    #else
    //GPG1 reset pin
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<4)) | (0x0<<4));
    #endif
//    //GPG1 reset pin
//    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<4)) | (0x0<<4));
    
    GPIO_OpenBit(NT066E_RESET_PIN_PORT, NT066E_RESET_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(NT066E_RESET_PIN_PORT, NT066E_RESET_PORT_BIT); 
    sysDelay(200/portTICK_RATE_MS);
    GPIO_ClrBit(NT066E_RESET_PIN_PORT, NT066E_RESET_PORT_BIT); 
    sysDelay(200/portTICK_RATE_MS);
    GPIO_SetBit(NT066E_RESET_PIN_PORT, NT066E_RESET_PORT_BIT); 
    
    //GPF 0 ~ 5 trig pin set to input
    outpw(REG_SYS_GPF_MFPL,(inpw(REG_SYS_GPF_MFPL) & ~(0xFFFFFF<<0)) | (0x000000u<<0));
    GPIO_OpenBit(GPIOF, BIT0, DIR_INPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOF, BIT1, DIR_INPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOF, BIT2, DIR_INPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOF, BIT3, DIR_INPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOF, BIT4, DIR_INPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOF, BIT5, DIR_INPUT, NO_PULL_UP);
    
    
    
    initFlag = TRUE;
    
    if(NT066ESetTriggerLevel(7) == FALSE)
    {
        return FALSE;
    } 
    
    if(NT066EResetChip() == FALSE)
    {
        return FALSE;
    }    
    

    pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, 0, 0);
    return TRUE;
}

static BOOL swInit(void)
{
    xNT066ESemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vNT066EDrvTask, "vNT066EDrvTask", 1024, NULL, NT066E_DRV_THREAD_PROI, NULL );
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL NT066EDrvInit(BOOL testModeFlag)
{
    sysprintf("NT066EDrvInit!!\n");
    if(initFlag)
        return TRUE;
    pI2cInterface = I2cGetInterface(I2C_1_INTERFACE_INDEX);
    if(pI2cInterface == NULL)
    {
        sysprintf("NT066EDrvInit ERROR (pI2cInterface == NULL)!!\n");
        return FALSE;
    }
    
    if(pI2cInterface->initFunc() == FALSE)
    {
        sysprintf("NT066EDrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    
    if(hwInit() == FALSE)
    {
        sysprintf("NT066EDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    
    if(testModeFlag == FALSE)
    {
        if(swInit() == FALSE)
        {
            sysprintf("NT066EDrvInit ERROR (swInit false)!!\n");
            return FALSE;
        }
    }
    
    
    return TRUE;
}
void NT066ESetCallbackFunc(keyHardwareCallbackFunc func)
{
    pKeyHardwareCallbackFunc = func;
}

BOOL NT066EResetChip(void)
{
    //#if(SUPPORT_NT066E)
    uint8_t dataTmp[8];
    uint8_t dataTmp1[8];
    if(initFlag == FALSE)
        return FALSE;
    sysprintf("NT066EResetChip --> \n");
    if(sendNT066EReadCmd(0x04, dataTmp, 1))
    {
        sysprintf(" ~~~ NT066E Read SysCtrl(1) 0x%02x ~~~\n", dataTmp[0]);
    }
    else
    {
        sysprintf(" !!! NT066E Read ERROR !!!\n");
        return FALSE;
    }   
    
    dataTmp[0] = dataTmp[0] | 0x80;
    sendNT066EWriteCmd(0x04, dataTmp, 1);   

    if(sendNT066EReadCmd(0x04, dataTmp1, 1))
    {
        sysprintf(" ~~~ NT066E Read SysCtrl(2) 0x%02x ~~~\n", dataTmp1[0]);
        if(dataTmp1[0] != dataTmp[0])
        {
            sysprintf(" ~~~ NT066E Tx SysCtrl error 0x%02x : 0x%02x ~~~\n", dataTmp[0], dataTmp1[0]);
            return FALSE;
        }
    }
    else
    {
        sysprintf(" !!! NT066E Read ERROR !!!\n");
        return FALSE;
    }   
    //#endif    
    return TRUE;
}

BOOL NT066EResetBuildBit(BOOL flag)
{
    //#if(SUPPORT_NT066E)
    uint8_t dataTmp[8];
    uint8_t dataTmp1[8];
    if(initFlag == FALSE)
        return FALSE;
    sysprintf("NT066EResetBuildBit --> \n");
    if(sendNT066EReadCmd(0x04, dataTmp, 1))
    {
        sysprintf(" ~~~ NT066E Read NT066EResetBuildBit(1) 0x%02x ~~~\n", dataTmp[0]);
    }
    else
    {
        sysprintf(" !!! NT066E Read ERROR !!!\n");
        return FALSE;
    }   
    if(flag)
        dataTmp[0] = dataTmp[0] | 0x1;
    else
        dataTmp[0] = dataTmp[0] & (~0x1);
    
    sendNT066EWriteCmd(0x04, dataTmp, 1);   

    if(sendNT066EReadCmd(0x04, dataTmp1, 1))
    {
        sysprintf(" ~~~ NT066E Read NT066EResetBuildBit(2) 0x%02x ~~~\n", dataTmp1[0]);
        if(dataTmp1[0] != dataTmp[0])
        {
            sysprintf(" ~~~ NT066E Tx NT066EResetBuildBit error 0x%02x : 0x%02x ~~~\n", dataTmp[0], dataTmp1[0]);
            return FALSE;
        }
    }
    else
    {
        sysprintf(" !!! NT066E Read ERROR !!!\n");
        return FALSE;
    }   
    //#endif    
    return TRUE;
}
#define SEN_TRIGGER_LEVEL_START_ADDRESS  0x20
BOOL NT066ESetTriggerLevel(int level)
{
    //#if(SUPPORT_NT066E)
    uint8_t dataTmp = level;
    uint8_t dataTmp1;
    if(initFlag == FALSE)
        return FALSE;
    sysprintf("NT066ESetTriggerLevel --> \n");
    NT066EResetBuildBit(TRUE);
    for(uint8_t i = 0; i<6; i++)
    {
        sendNT066EWriteCmd(i+SEN_TRIGGER_LEVEL_START_ADDRESS, &dataTmp, 1);   

        if(sendNT066EReadCmd(i+SEN_TRIGGER_LEVEL_START_ADDRESS, &dataTmp1, 1))
        {
            sysprintf(" ~~~ NT066E Read TriggerLevel 0x%02x ~~~\n", dataTmp1);
            if(dataTmp1 != dataTmp)
            {
                sysprintf(" ~~~ NT066E Tx TriggerLevel error 0x%02x : 0x%02x ~~~\n", dataTmp, dataTmp1);
                return FALSE;
            }
        }
        else
        {
            sysprintf(" !!! NT066E Read TriggerLevel ERROR !!!\n");
            return FALSE;
        }   
    }
    NT066EResetBuildBit(FALSE);
    return TRUE;
}

BOOL NT066ESetPower(BOOL flag)
{
    #if(0)
    return TRUE;
    #else
    if(flag)
    {
        sysprintf(" !!! NT066E Set Power ON !!!\n");
        pI2cInterface->setPin();
        GPIO_SetBit(NT066E_RESET_PIN_PORT, NT066E_RESET_PORT_BIT); 
        
        GPIO_ClrBit(NT066E_POWER_PIN_PORT, NT066E_POWER_PORT_BIT); 
        
    }
    else
    {
        sysprintf(" !!! NT066E Set Power OFF !!!\n");        
        GPIO_SetBit(NT066E_POWER_PIN_PORT, NT066E_POWER_PORT_BIT); 
        
        GPIO_ClrBit(NT066E_RESET_PIN_PORT, NT066E_RESET_PORT_BIT); 
        pI2cInterface->resetPin();
    }
    return TRUE;
    #endif
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


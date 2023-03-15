/**************************************************************************//**
* @file     dipdrv.c
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
#include "adc.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "dipdrv.h"
#include "interface.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#if(SUPPORT_HK_10_HW)//by Jer
    #define DIP_1_PORT  GPIOI
    #define DIP_1_PIN   BIT3
#else
    #define DIP_1_PORT  GPIOH
    #define DIP_1_PIN   BIT4
#endif

//#define DIP_1_PORT  GPIOH
//#define DIP_1_PIN   BIT4
#define DIP_2_PORT  GPIOH
#define DIP_2_PIN   BIT2
#define DIP_3_PORT  GPIOH
#define DIP_3_PIN   BIT3
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static SemaphoreHandle_t xActionSemaphore;
static TickType_t threadWaitTime = portMAX_DELAY;
static keyHardwareCallbackFunc pKeyHardwareCallbackFunc;
static int prevScreenId = -1;
static TickType_t batteryButtonTick = 0;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
extern BOOL SysGetBooted(void);

static void vDipDrvTask( void *pvParameters )
{
    sysprintf("!!! vDipDrvTask Waiting... !!!\r\n"); 
    while(SysGetBooted() == FALSE)
    {
        vTaskDelay(500/portTICK_RATE_MS);
    }
    vTaskDelay(1000/portTICK_RATE_MS);
    sysprintf("!!! vDipDrvTask Going... !!!\r\n"); 
    xSemaphoreGive( xActionSemaphore);
    for(;;)
    {       
        //BOOL actionReval = TRUE;
        BaseType_t reval = xSemaphoreTake(xActionSemaphore, threadWaitTime);         
        vTaskDelay(50/portTICK_RATE_MS); 
        if(!GPIO_ReadBit(DIP_1_PORT, DIP_1_PIN))
        {            
            if((xTaskGetTickCount() - batteryButtonTick) > (1000/portTICK_RATE_MS))
            {
                sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_BATTERY_REPLACE ...\r\n");
                pKeyHardwareCallbackFunc(DIP_REPLACE_BP_ID, KEY_HARDWARE_DOWN_EVENT);
                prevScreenId = DIP_REPLACE_BP_ID;
                batteryButtonTick = xTaskGetTickCount();
            }
            //else
            //{
            //    sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_BATTERY_REPLACE ignore...\r\n");
            //}
           
        }
        else
        {
            UINT32 portValue = GPIO_ReadPort(GPIOH);
            sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_STANDBY(0x%02x, %d) ...\r\n", (portValue>>2)&0x3, (portValue>>2)&0x3); 
            portValue = (portValue>>2)&0x3;
            switch(portValue)
            {
                case 3:
                    
                    if(prevScreenId != DIP_NORMAL_ID)
                    {
                        sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_STANDBY(11) ...\r\n"); 
                        pKeyHardwareCallbackFunc(DIP_NORMAL_ID, KEY_HARDWARE_DOWN_EVENT);
                        prevScreenId = DIP_NORMAL_ID;
                    }
                    else
                    {
                        sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_STANDBY(11) ignore...\r\n"); 
                    }
                    break;
                case 2:
                    //sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_SETTING_SPACE(10) ...\r\n");  
                    //actionReval = pKeyHardwareCallbackFunc(DIP_SETTING_SPACE_ID, KEY_HARDWARE_DOWN_EVENT);
                    break;
                case 1:
                    
                    if(prevScreenId != DIP_TESTER_KEYPAD_ID)
                    {
                        sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_TESTER_KEYPAD(01) ...\r\n");  
                        pKeyHardwareCallbackFunc(DIP_TESTER_KEYPAD_ID, KEY_HARDWARE_DOWN_EVENT);
                        prevScreenId = DIP_TESTER_KEYPAD_ID;
                    }
                    else
                    {
                        sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_TESTER_KEYPAD(01) ignore...\r\n"); 
                    }
                    break;
                case 0:
                    
                    if(prevScreenId != DIP_TESTER_ID)
                    {
                        sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_TESTER(00) ...\r\n");  
                        pKeyHardwareCallbackFunc(DIP_TESTER_ID, KEY_HARDWARE_DOWN_EVENT);
                        prevScreenId = DIP_TESTER_ID;
                    }
                    else
                    {
                        sysprintf("vDipDrvTask MAIN_SERVICE_STATUS_TESTER(00) ignore...\r\n");  
                    }
                    break;
            }
            
        }
        //if(!actionReval)
        //{
       //     sysprintf("BATTERY_REPLACE fail...\r\n");
        //    vTaskDelay(3000/portTICK_RATE_MS);
        //    sysprintf("BATTERY_REPLACE retry...\r\n");
        //    xSemaphoreGive(xActionSemaphore);
        //}
    }
}
static void processAction(void)
{
    BaseType_t xHigherPriorityTaskWoken;  
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR( xActionSemaphore, &xHigherPriorityTaskWoken );
    portEXIT_SWITCHING_ISR (xHigherPriorityTaskWoken);   
}
#if(SUPPORT_HK_10_HW)
#else
INT32 EINT4Callback(UINT32 status, UINT32 userData)
{
    sysprintf("\r\n - EINT4 0x%08x [%04d] - \r\n", status, GPIO_ReadBit(GPIOH, DIP_1_PIN));
    processAction();
    GPIO_ClrISRBit(DIP_1_PORT, DIP_1_PIN);
    return 0;
}
#endif
INT32 EINT2Callback(UINT32 status, UINT32 userData)
{
    sysprintf("\r\n - EINT2 0x%08x [%04d] - \r\n", status, GPIO_ReadBit(GPIOH, DIP_2_PIN));
    processAction();
    GPIO_ClrISRBit(DIP_2_PORT, DIP_2_PIN);
    return 0;
}
INT32 EINT3Callback(UINT32 status, UINT32 userData)
{
    sysprintf("\r\n - EINT3 0x%08x [%04d] - \r\n", status, GPIO_ReadBit(GPIOH, DIP_3_PIN));
    processAction();
    GPIO_ClrISRBit(DIP_3_PORT, DIP_3_PIN);
    return 0;
}
static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
#if(SUPPORT_HK_10_HW)
        /* Set PI3 to input */
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xFu<<12)) | (0x0u<<12));
    GPIO_OpenBit(DIP_1_PORT, DIP_1_PIN, DIR_INPUT, PULL_UP);
#else
    /* Set PH4 to EINT4 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<16)) | (0xF<<16));    
    /* Configure PH4 to input mode */
    GPIO_OpenBit(DIP_1_PORT, DIP_1_PIN, DIR_INPUT, PULL_UP);
    /* Confingure PH4 to both-edge trigger */
    GPIO_EnableTriggerType(GPIOH, DIP_1_PIN, FALLING);
    //EINT4
    GPIO_EnableEINT(NIRQ4, (GPIO_CALLBACK)EINT4Callback, 0);    
    GPIO_ClrISRBit(DIP_1_PORT, DIP_1_PIN);
#endif
    
    /* Set PH2 to EINT2 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<8)) | (0xF<<8));    
    /* Configure PH2 to input mode */
    GPIO_OpenBit(DIP_2_PORT, DIP_2_PIN, DIR_INPUT, NO_PULL_UP);
    /* Confingure PH2 to both-edge trigger */
    GPIO_EnableTriggerType(GPIOH, DIP_2_PIN, BOTH_EDGE);
    //EINT2
    GPIO_EnableEINT(NIRQ2, (GPIO_CALLBACK)EINT2Callback, 0);    
    GPIO_ClrISRBit(DIP_2_PORT, DIP_2_PIN);
    
    /* Set PH3 to EINT3 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<12)) | (0xF<<12));    
    /* Configure PH3 to input mode */
    GPIO_OpenBit(DIP_3_PORT, DIP_3_PIN, DIR_INPUT, NO_PULL_UP);
    /* Confingure PH3 to both-edge trigger */
    GPIO_EnableTriggerType(GPIOH, DIP_3_PIN, BOTH_EDGE);
    //EINT3
    GPIO_EnableEINT(NIRQ3, (GPIO_CALLBACK)EINT3Callback, 0);    
    GPIO_ClrISRBit(DIP_3_PORT, DIP_3_PIN);  
    

    return TRUE;
}
static BOOL swInit(void)
{   
    xActionSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vDipDrvTask, "vDipDrvTask", 512, NULL, DIP_THREAD_PROI, NULL);
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL DipDrvInit(BOOL testModeFlag)
{
//    sysprintf("DipDrvInit!!\n");
    if(hwInit() == FALSE)
    {
        terninalPrintf("DipDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(testModeFlag == FALSE)
    {
        if(swInit() == FALSE)
        {
            terninalPrintf("DipDrvInit ERROR (swInit false)!!\n");
            return FALSE;
        }
    }
    return TRUE;
}

void DipSetCallbackFunc(keyHardwareCallbackFunc func)
{
    pKeyHardwareCallbackFunc = func;
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


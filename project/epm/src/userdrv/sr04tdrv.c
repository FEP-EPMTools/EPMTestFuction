/**************************************************************************//**
* @file     sr04tdrv.c
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
#include "etimer.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "sr04tdrv.h"
#include "buzzerdrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define OUTPUT_PORT_1   GPIOB
#define OUTPUT_PIN_1    BIT4

#define OUTPUT_PORT_2   GPIOB
#define OUTPUT_PIN_2    BIT0

#define POWER_PORT_1    GPIOB
#define POWER_PIN_1     BIT5

#define POWER_PORT_2    GPIOB
#define POWER_PIN_2     BIT2

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static int distValue[2];
static BOOL initFlag = FALSE;

#define TOGGLE_HZ_VALUE (100*1000) //10ns
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void setPower(uint8_t id, BOOL flag)
{
    #if(0)
    if(flag)
        sysprintf("-> setPower (%d) enter: true\r\n", id);
    else
        sysprintf("-> setPower (%d) enter: false\r\n", id);
    #endif
    //ETIMER_Stop(0);
    //ETIMER_DisableInt(0);
    //ETIMER_ClearIntFlag(0);
    //ETIMER_ClearCaptureIntFlag(0);
    //ETIMER_Close(0);
    if(DIST_DEVICE_1 == id)
    {
        if(flag)
            GPIO_SetBit(POWER_PORT_1, POWER_PIN_1); 
        else
            GPIO_ClrBit(POWER_PORT_1, POWER_PIN_1); 
    }
    
    if(DIST_DEVICE_2 == id)
    {
        if(flag)
            GPIO_SetBit(POWER_PORT_2, POWER_PIN_2); 
        else
            GPIO_ClrBit(POWER_PORT_2, POWER_PIN_2); 
    } 
    #if(0)
    if(flag)
        sysprintf("-> setPower (%d) exit: true\r\n", id);
    else
        sysprintf("-> setPower (%d) exit: false\r\n", id);
    #endif
}

static void settingToggle(uint16_t ns, uint8_t id)
{
    //sysprintf("-> settingToggle [%d]\r\n", ns);
    //toggleTimes = 0;
    //targetToggleTimes = ns/(1000000/TOGGLE_HZ_VALUE);
    //sysprintf("-> settingToggle [%d], targetToggleTimes = %d\r\n", ns, targetToggleTimes);
    //ETIMER_Open(0, ETIMER_TOGGLE_MODE, TOGGLE_HZ_VALUE);  
    //ETIMER_Stop(0);
    //ETIMER_DisableInt(0);
    //ETIMER_ClearIntFlag(0);
    //ETIMER_ClearCaptureIntFlag(0);  
    if(DIST_DEVICE_1 == id)
    {
        GPIO_ClrBit(OUTPUT_PORT_1, OUTPUT_PIN_1); 
    }
    
    if(DIST_DEVICE_2 == id)
    {
        GPIO_ClrBit(OUTPUT_PORT_2, OUTPUT_PIN_2); 
    }    
    
}

static void startToggle(uint8_t id)
{
    //sysprintf("-> startToggle\r\n");
    //ETIMER_EnableInt(0);    
    //ETIMER_Start(0);
    if(DIST_DEVICE_1 == id)
    {
        GPIO_SetBit(OUTPUT_PORT_1, OUTPUT_PIN_1); 
        #if(FREERTOS_USE_1000MHZ)
        vTaskDelay(1/portTICK_RATE_MS); 
        #else
        //#error
        vTaskDelay(10/portTICK_RATE_MS); 
        #endif
        GPIO_ClrBit(OUTPUT_PORT_1, OUTPUT_PIN_1); 
    }
    
    if(DIST_DEVICE_2 == id)
    {
        GPIO_SetBit(OUTPUT_PORT_2, OUTPUT_PIN_2); 
        #if(FREERTOS_USE_1000MHZ)
        vTaskDelay(1/portTICK_RATE_MS); 
        #else
        //#error
        vTaskDelay(10/portTICK_RATE_MS); 
        #endif
        GPIO_ClrBit(OUTPUT_PORT_2, OUTPUT_PIN_2); 
    }    
    
}
static void stopToggle(uint8_t id)
{
    //sysprintf("-> stopToggle\r\n");
    //ETIMER_Stop(0);
    //ETIMER_DisableInt(0);
    //ETIMER_ClearIntFlag(0);
    //ETIMER_ClearCaptureIntFlag(0);
    //ETIMER_Close(0);
    if(DIST_DEVICE_1 == id)
    {
        GPIO_ClrBit(OUTPUT_PORT_1, OUTPUT_PIN_1); 
    }
    
    if(DIST_DEVICE_2 == id)
    {
        GPIO_ClrBit(OUTPUT_PORT_2, OUTPUT_PIN_2); 
    }    
}

static void startCapture(uint8_t id)
{
    //sysprintf("-> startCapture\r\n");
    if(DIST_DEVICE_1 == id)
    {
        ETIMER_Open(0, ETIMER_ONESHOT_MODE, 10); //100ms
        ETIMER_EnableCaptureInt(0);
        ETIMER_EnableCapture(0, ETIMER_CAPTURE_TRIGGER_COUNTING_MODE, ETIMER_CAPTURE_RISING_THEN_FALLING_EDGE);
        ETIMER_Start(0);
    }
    
    if(DIST_DEVICE_2 == id)
    {
        ETIMER_Open(1, ETIMER_ONESHOT_MODE, 10); //100ms
        ETIMER_EnableCaptureInt(1);
        ETIMER_EnableCapture(1, ETIMER_CAPTURE_TRIGGER_COUNTING_MODE, ETIMER_CAPTURE_RISING_THEN_FALLING_EDGE);
        ETIMER_Start(1);
    }
    
    
}
static void stopCapture(uint8_t id)
{
    //sysprintf("-> stopCapture\r\n");
    if(DIST_DEVICE_1 == id)
    {    
        ETIMER_Stop(0);
        ETIMER_DisableCapture(0);        
        ETIMER_DisableCaptureInt(0);        
        ETIMER_Close(0);
    }
    if(DIST_DEVICE_2 == id)
    {
        ETIMER_Stop(1);
        ETIMER_DisableCapture(1);        
        ETIMER_DisableCaptureInt(1);        
        ETIMER_Close(1);
    }
}
static int checkActive(uint8_t id)
{
    if(DIST_DEVICE_1 == id)
    {    
        return ETIMER_Is_Active(0);
    }
    else
    {
        return ETIMER_Is_Active(1);
    }
}
static BOOL runCapture(uint8_t id, int* detectResult)
{
    sysprintf("-> runCapture\r\n");
    BOOL reVal = TRUE;
    int totalTimes = 4;
    int getTimes = 0;
    int totalDist = 0;
    uint16_t counter = 0;
    distValue[id] = 0;
    //distValue[DIST_DEVICE_2] = 0;
    setPower(id, TRUE);
    vTaskDelay(300/portTICK_RATE_MS); 
    while(totalTimes>0)
    {
        reVal = TRUE;
        settingToggle(10, id);
        startCapture(id);
        startToggle(id);
        counter = 10;
        while(checkActive(id))
        {
            vTaskDelay(10/portTICK_RATE_MS);
            sysprintf(".");
            counter--;
            if(counter == 0)
            {
                //sysprintf("runCapture[%d]: timeout\r\n", id);            
                //xSemaphoreGive(xSemaphore);
                reVal = FALSE;
                break;
            }
        }
        stopToggle(id); 
        stopCapture(id); 
        totalTimes--;
        if(reVal)
        {
            getTimes++;
            terninalPrintf("getTimes = %d \r\n", getTimes);
            totalDist = totalDist + distValue[id];
        }
    }
    if(getTimes>=4)
    {        
        *detectResult = totalDist/getTimes;
        reVal = TRUE;
        sysprintf("runCapture TRUE [%d]: %d cm\r\n", getTimes, *detectResult); 
    }
    else
    {
        *detectResult = 0;
        reVal = FALSE;
        sysprintf("runCapture FALSE [%d]: %d cm\r\n", getTimes, *detectResult); 
    }   
    setPower(id, FALSE);
    return reVal;
}
/*
static void vSR04TTestTask( void *pvParameters )
{
    sysprintf("vSR04TTestTask Going...\r\n"); 
    vTaskDelay(1000/portTICK_RATE_MS); 
    uint8_t times = 0;
    uint16_t counter = 0;
    for(;;)
    {   
        //sysprintf("\r\nvSR04TTestTask Action...\r\n");
        tickStart = xTaskGetTickCount(); 

        settingToggle(10);
        startCapture();

        startToggle();

        counter = 10;
        while(ETIMER_Is_Active(0) || ETIMER_Is_Active(1))
        {
            vTaskDelay(10/portTICK_RATE_MS);
            //sysprintf(".");
            counter--;
            if(counter == 0)
            {
                sysprintf("timeout\r\n");
                distValue = 0;
                xSemaphoreGive(xSemaphore);
                break;
            }
        }
        stopToggle(); 
        stopCapture();        
       
        vTaskDelay(200/portTICK_RATE_MS);    
        times++;  
    }
}
*/
static BOOL swInit(void)
{
    //xSemaphore = xSemaphoreCreateBinary();
    //xBuzzerSemaphore = xSemaphoreCreateBinary(); 
    //xTaskCreate( vSR04TTestTask, "vSR04TTestTask", 1024, NULL, 5, NULL );    
    //xTaskCreate( vSR04TReadTask, "vSR04TReadTask", 1024, NULL, 4, NULL );
    //xTaskCreate( vBuzzerTask, "vBuzzerTask", 1024, NULL, 6, NULL );
    return TRUE;
}

static void ETMR0_IRQHandler(void)
{
    // clear timer interrupt flag
    if(ETIMER_GetIntFlag(0))
    {
        ETIMER_ClearIntFlag(0);
    }
    if(ETIMER_GetCaptureIntFlag(0))
    {
        uint16_t distance;//, distance2;
        uint32_t data5 = (uint32_t)((uint64_t)ETIMER_GetCaptureData(0) * (uint64_t)100*(uint64_t)1000 /(uint64_t)inpw(REG_ETMR0_CMPR)); 

        distance = data5*34/100/2;
        //distance2 = data5*10/58;
        //sysprintf(" **> Get Capture (%d/%d): [%d] us, %d cm (%d mm, %d mm)\n", ETIMER_GetCaptureData(0), inpw(REG_ETMR0_CMPR), data5, distance/10, distance, distance2);
        //sysprintf(" **> [<1>: %03d cm]\n", distance/10);
        distValue[DIST_DEVICE_1] = distance/10;
        ETIMER_ClearCaptureIntFlag(0);
        stopCapture(DIST_DEVICE_1);
    }   

}


static void ETMR1_IRQHandler(void)
{
    // clear timer interrupt flag
    if(ETIMER_GetIntFlag(1))
    {
        ETIMER_ClearIntFlag(1);
    }
    if(ETIMER_GetCaptureIntFlag(1))
    {
        uint16_t distance;//, distance2;
        uint32_t data5 = (uint32_t)((uint64_t)ETIMER_GetCaptureData(1) * (uint64_t)100*(uint64_t)1000 /(uint64_t)inpw(REG_ETMR1_CMPR)); 

        distance = data5*34/100/2;
        //distance2 = data5*10/58;
        //sysprintf(" **> Get Capture (%d/%d): [%d] us, %d cm (%d mm, %d mm)\n", ETIMER_GetCaptureData(0), inpw(REG_ETMR0_CMPR), data5, distance/10, distance, distance2);
        //sysprintf(" **> [<2>: %03d cm]\n", distance/10);
        distValue[DIST_DEVICE_2] = distance/10;
        ETIMER_ClearCaptureIntFlag(1);
        stopCapture(DIST_DEVICE_2);
    }   

}
static BOOL hwInit(void)
{       
    //outpw(REG_CLK_DIVCTL8,inpw(REG_CLK_DIVCTL8) | 0x550000); //Enable GPIO engin clock.
    //outpw(REG_CLK_DIVCTL8,(inpw(REG_CLK_DIVCTL8) & ~(0x550000)) | (0x550000));
    //outpw(REG_CLK_DIVCTL8,(inpw(REG_CLK_DIVCTL8) & ~(0x550000)));
    
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    /* Configure PB4 PB0 to output mode */
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<16)) | (0x0<<16));
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(OUTPUT_PORT_1, OUTPUT_PIN_1, DIR_OUTPUT, NO_PULL_UP);  
    GPIO_OpenBit(OUTPUT_PORT_2, OUTPUT_PIN_2, DIR_OUTPUT, NO_PULL_UP);  
    stopToggle(DIST_DEVICE_1);
    stopToggle(DIST_DEVICE_2);
    
    /* Configure PB5 PB2 to output mode */
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<20)) | (0x0<<20));
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(POWER_PORT_1, POWER_PIN_1, DIR_OUTPUT, NO_PULL_UP);     
    GPIO_OpenBit(POWER_PORT_2, POWER_PIN_2, DIR_OUTPUT, NO_PULL_UP);  
    setPower(DIST_DEVICE_1, FALSE);
    setPower(DIST_DEVICE_2, FALSE);


    //capture ETIMER0 GPB3
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 4)); // Enable ETIMER0 engine clock
    outpw(REG_SYS_GPB_MFPL, (inpw(REG_SYS_GPB_MFPL) & ~(0xF << 12)) | (0xf << 12)); // Enable ETIMER0 capture out pin @ GPB3
    ETIMER_Open(0, ETIMER_ONESHOT_MODE, 1); 
    ETIMER_Stop(0);
    
    ETIMER_EnableInt(0);
    ETIMER_EnableCaptureInt(0);
    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, ETMR0_IRQn, (PVOID)ETMR0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(ETMR0_IRQn);


    //capture ETIMER1 GPB1
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 5)); // Enable ETIMER1 engine clock
    outpw(REG_SYS_GPB_MFPL, (inpw(REG_SYS_GPB_MFPL) & ~(0xF << 4)) | (0xd << 4)); // Enable ETIMER1 capture out pin @ GPB1
    ETIMER_Open(1, ETIMER_ONESHOT_MODE, 1); 
    ETIMER_Stop(1);
    
    ETIMER_EnableInt(1);
    ETIMER_EnableCaptureInt(1);
    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, ETMR1_IRQn, (PVOID)ETMR1_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(ETMR1_IRQn);  

    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL SR04TDrvInit(void)
{
    if(initFlag)
        return TRUE;
    sysprintf("SR04TDrvInit!!\n");
    
    if(hwInit() == FALSE)
    {
        sysprintf("SR04TDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("SR04TDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }    
    
    initFlag = TRUE;
    return TRUE;
}

BOOL SR04TMeasureDist(uint8_t id, int* detectResult)
{
    //return rangingTest(&MyDevice[id], detectResult); 
    return runCapture(id, detectResult);
    //return TRUE;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


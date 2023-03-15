/**************************************************************************//**
* @file     batterydrv.c
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
#include "batterydrv.h"
#include "interface.h"
#include "powerdrv.h"
#include "epddrv.h"
#include "quentelmodemlib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
static SemaphoreHandle_t xActionSemaphore;
static TickType_t threadWaitTime = portMAX_DELAY;//3000/portTICK_RATE_MS;//portMAX_DELAY;

#define ENABLE_BATTERY_DRV_DEBUG_MESSAGE    0

#define BATTERY_1_ENABLE_PORT       GPIOF
#define BATTERY_1_ENABLE_PIN        BIT6
#define BATTERY_2_ENABLE_PORT       GPIOF
#define BATTERY_2_ENABLE_PIN        BIT7


#define BATTERY_1_LOW_DETECT_PORT       GPIOH
#define BATTERY_1_LOW_DETECT_PIN        BIT6
#define BATTERY_2_LOW_DETECT_PORT       GPIOH
#define BATTERY_2_LOW_DETECT_PIN        BIT5


#define BATTERY_0_BATTERY_INDEX     0
#define BATTERY_1_BATTERY_INDEX     1 //0
#define BATTERY_2_BATTERY_INDEX     2 //1

#if(SUPPORT_HK_10_HW)
    #define BATTERY_SOLAR_ENABLE_PORT       GPIOE
    #define BATTERY_SOLAR_ENABLE_PIN        BIT14
    
    #define BATTERY_SOLAR_LOW_DETECT_PORT       GPIOH
    #define BATTERY_SOLAR_LOW_DETECT_PIN        BIT7
    
    #define BATTERY_SOLAR_BATTERY_INDEX         2
#endif

#define ADC_MEASURE_TIMES           7
#define AIN_START_INDEX             0 //1
#define AIN_NUM                     3 //2

#define VOLTAGE_DIVIDER_1             510
#define VOLTAGE_DIVIDER_2             180
#define REAL_VOLTAGE(voltage)             (voltage*(VOLTAGE_DIVIDER_1+VOLTAGE_DIVIDER_2)/VOLTAGE_DIVIDER_2)//3
#define REAL_VOLTAGE(voltage)             (voltage*(VOLTAGE_DIVIDER_1+VOLTAGE_DIVIDER_2)/VOLTAGE_DIVIDER_2)//

/*
#define SVOLTAGE_DIVIDER_1             600
#define SVOLTAGE_DIVIDER_2             180
#define SREAL_VOLTAGE(voltage)             (voltage*(SVOLTAGE_DIVIDER_1+SVOLTAGE_DIVIDER_2)/SVOLTAGE_DIVIDER_2)//3
*/

#define SVOLTAGE_DIVIDER_1              750     // CPU internal divider resistor (7.5K)
#define SVOLTAGE_DIVIDER_2              250     // CPU internal divider resistor (2.5K)
#define SVOLTAGE_MULTIPLIER             2       // The voltage of solar battery is 8.4V, but maximum input of ADC0 is 5.5V, so we use an additional divider resistor at main board.
#define SREAL_VOLTAGE(voltage)          (voltage*SVOLTAGE_MULTIPLIER*(SVOLTAGE_DIVIDER_1+SVOLTAGE_DIVIDER_2)/SVOLTAGE_DIVIDER_2)


#define DIVIDER_VOLTAGE(realVoltage)    (realVoltage*VOLTAGE_DIVIDER_2/(VOLTAGE_DIVIDER_1+VOLTAGE_DIVIDER_2))
#define BATTERY_EMPTY_VOLTAGE           DIVIDER_VOLTAGE(500)
#define BATTERY_FAIL_VOLTAGE            DIVIDER_VOLTAGE(700)
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static BOOL testLocalMode = FALSE;
static uint8_t saveIndex = 0;
static UINT32 adcValue[ADC_MEASURE_TIMES*AIN_NUM];
static UINT32 voltageValue[AIN_NUM];
static uint8_t batteryStatus[AIN_NUM];
static uint8_t batteryStatusTmp[AIN_NUM];
static BOOL enableTestBattery = FALSE;

static BOOL batteryDrvCheckStatus(int flag);
static BOOL batteryDrvPreOffCallback(int flag);
static BOOL batteryDrvOffCallback(int flag);
static BOOL batteryDrvOnCallback(int flag);
static powerCallbackFunc batteryDrvPowerCallabck = {" [BatteryDrv] ", batteryDrvPreOffCallback, batteryDrvOffCallback, batteryDrvOnCallback, batteryDrvCheckStatus};
static BOOL batteryDrvPowerStatus = TRUE;
static BOOL batteryDrvIgnoreRun = FALSE;

static BOOL batteryConnectMeasureflag = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL batteryDrvPreOffCallback(int flag)
{
    batteryDrvIgnoreRun = TRUE;
    return TRUE;
}
static BOOL batteryDrvOffCallback(int flag)
{
    int timers = 2000/10;
    while(!batteryDrvPowerStatus)
    {
        sysprintf("[b]");
        if(timers-- == 0)
        {
            return FALSE;
        }
        vTaskDelay(10/portTICK_RATE_MS); 
    }
    return TRUE;     
}
static BOOL batteryDrvOnCallback(int flag)
{
    batteryDrvIgnoreRun = FALSE;
    xSemaphoreGive(xActionSemaphore);
    return TRUE;
}
static BOOL batteryDrvCheckStatus(int flag)
{    
    return batteryDrvPowerStatus;  
}

static INT32 NormalConvCallback(UINT32 status, UINT32 userData)
{
    /*  The status content that contains normal data.
     */
     
    adcValue[saveIndex] = status;
    saveIndex++;
    #if(0)
    voltage=(refVoltage*status*10)>>12;
    if(userData == 0)
    {
        sysprintf("BAT [%d]: adc=0x%03x (%d),voltage=%d\r\n", userData, status, status, voltage*4);
    }
    else
    {
        sysprintf("BAT [%d]: adc=0x%03x (%d),voltage=%d\r\n", userData, status, status, voltage);
    }
    #endif
    if(saveIndex == ADC_MEASURE_TIMES*AIN_NUM)
    {       
        {
            BaseType_t xHigherPriorityTaskWoken;  
            xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR( xActionSemaphore, &xHigherPriorityTaskWoken );
            portEXIT_SWITCHING_ISR (xHigherPriorityTaskWoken);  
        }
        
    }
    return 0;
}
static void measureBattery()
{
    int i, ainIndex;
    saveIndex = 0;

    for(ainIndex = AIN_START_INDEX; ainIndex < AIN_START_INDEX + AIN_NUM; ainIndex++)
    {
        adcIoctl(NAC_ON,(UINT32)NormalConvCallback,ainIndex); //Enable Normal AD Conversion

        
        switch(ainIndex)
        {
            case 0:
                //adcIoctl(VBAT_ON,(UINT32)NormalConvCallback,ainIndex);
                adcChangeChannel(AIN0);
                //terninalPrintf(" AIN0= %d\r\n",adcChangeChannel(AIN0));
                break;
            case 1:
                //adcIoctl(NAC_ON,(UINT32)NormalConvCallback,ainIndex);
                adcChangeChannel(AIN1);
                //terninalPrintf(" AIN1= %d\r\n",adcChangeChannel(AIN1));
                break;
            case 2:
                //adcIoctl(NAC_ON,(UINT32)NormalConvCallback,ainIndex);
                adcChangeChannel(AIN2);
                //terninalPrintf(" AIN2= %d\r\n",adcChangeChannel(AIN2));
                break;
        }      
        vTaskDelay(200/portTICK_RATE_MS);        
        for(i = 0; i < ADC_MEASURE_TIMES; i++)
        {
            adcIoctl(START_MST,0,0);
            //vTaskDelay(100/portTICK_RATE_MS);
        }
    }
    adcIoctl(NAC_OFF,0,0); //Disable Normal AD Conversion
    
    
    //adcIoctl(VBAT_OFF,0,0); 
}

BOOL NT066EResetChip(void);
static void vBatteryDrvTask( void *pvParameters )
{
    sysprintf("vBatteryDrvTask Going ...\r\n");  
    for(;;)
    {       
        BaseType_t reval = xSemaphoreTake(xActionSemaphore, threadWaitTime); 
        /*
				if(reval != pdTRUE)  
        {  //timeout      
            batteryDrvPowerStatus = FALSE;            
            measureBattery();
        }
        else
        {
					*/
            int refVoltage = 330; /* 3.3v */  
            UINT32 maxAdc, minAdc;
            UINT32 adcSum;
            UINT32 averageAdc;
            UINT32 voltage;
            int i, ainIndex;
                  
            measureBattery();
            for(ainIndex = 0; ainIndex < AIN_NUM; ainIndex++)
            {
                adcSum = 0;
                maxAdc = minAdc = adcValue[ainIndex*ADC_MEASURE_TIMES];
                for(i = 0; i<ADC_MEASURE_TIMES; i++)
                {
                    int targetI = i + ainIndex*ADC_MEASURE_TIMES;
                    if(adcValue[targetI] > maxAdc)
                    {
                        maxAdc = adcValue[targetI];
                    }
                    if(adcValue[targetI] < minAdc)
                    {
                        minAdc = adcValue[targetI];
                    }
                    adcSum = adcSum + adcValue[targetI];
                }
                averageAdc = (adcSum - maxAdc - minAdc)/(ADC_MEASURE_TIMES-2);   


                if(ainIndex == 0)
                    //voltage=(refVoltage*averageAdc)>>8;
                    voltage=(refVoltage*averageAdc)>>12;
                else
                    voltage=(refVoltage*averageAdc)>>12;
                
                if(ainIndex == 0)
                    voltage = voltage * 3;
                #if(ENABLE_BATTERY_DRV_DEBUG_MESSAGE)
                sysprintf("Battery_%d : ADC = 0x%03x (%d), voltage = %dv (%dv), fail voltage = (%dv, %dv)\r\n", ainIndex, averageAdc, averageAdc, voltage, REAL_VOLTAGE(voltage), BATTERY_FAIL_VOLTAGE, REAL_VOLTAGE(BATTERY_FAIL_VOLTAGE));
                #endif
                if(voltage < 100)
                {
                    voltageValue[ainIndex] = 0;
                    //voltageValue[ainIndex] = voltage;
                }
                else
                {
                    voltageValue[ainIndex] = voltage;
                }           
            }

           //terninalPrintf("Solar cell voltage: [%d], [%d], [%d]\r\n",SREAL_VOLTAGE(voltageValue[BATTERY_0_BATTERY_INDEX]) ,REAL_VOLTAGE(voltageValue[BATTERY_1_BATTERY_INDEX]),REAL_VOLTAGE(voltageValue[BATTERY_2_BATTERY_INDEX]));
           //terninalPrintf("threadWaitTime: %d \r\n",threadWaitTime);
            
           /*
           if(GPIO_ReadBit(GPIOJ, BIT4))
               terninalPrintf("Hi\r\n");
           else
               terninalPrintf("Lo\r\n");
           */
            
            
            if(batteryConnectMeasureflag == TRUE)
            {   
                batteryConnectMeasureflag = FALSE;
                UINT32 leftVoltage,  rightVoltage;
                if((voltageValue[0]==0) || (voltageValue[1]==0))
                {
                    vTaskDelay(500/portTICK_RATE_MS);
                    measureBattery();
                    for(ainIndex = 0; ainIndex < AIN_NUM; ainIndex++)
                    {
                        adcSum = 0;
                        maxAdc = minAdc = adcValue[ainIndex*ADC_MEASURE_TIMES];
                        for(i = 0; i<ADC_MEASURE_TIMES; i++)
                        {
                        int targetI = i + ainIndex*ADC_MEASURE_TIMES;
                        if(adcValue[targetI] > maxAdc)
                        {
                            maxAdc = adcValue[targetI];
                        }
                        if(adcValue[targetI] < minAdc)
                        {
                            minAdc = adcValue[targetI];
                        }
                        adcSum = adcSum + adcValue[targetI];
                        }
                        averageAdc = (adcSum - maxAdc - minAdc)/(ADC_MEASURE_TIMES-2);        
                        voltage=(refVoltage*averageAdc)>>12;
                        if(voltage < 100)
                        {
                            voltageValue[ainIndex] = 0;
                        }
                        else
                        {
                            voltageValue[ainIndex] = voltage;
                        }           
                    }

                }
                batteryConnectMeasureflag = FALSE;
                BatteryGetValue(NULL,&leftVoltage, &rightVoltage);
                //terninalPrintf(" Voltage: [%d], [%d]\r\n", leftVoltage, rightVoltage);
                if((leftVoltage>0) && (rightVoltage>0))
                {
                    if(leftVoltage>rightVoltage)
                    {   
                        BatterySetSwitch1(FALSE);
                        //terninalPrintf("\r\nLeft power close.\r\n");
                    }
                    else
                    {
                        BatterySetSwitch2(FALSE);
                        //terninalPrintf("\r\nRight power close.\r\n");
                    }
                }
                
                
            }
            /*
            if(voltageValue[BATTERY_1_BATTERY_INDEX] < BATTERY_FAIL_VOLTAGE)
            {
                //CmdSendBatteryFail(0, voltageValue[BATTERY_1_BATTERY_INDEX]);
            }
            if(voltageValue[BATTERY_2_BATTERY_INDEX] < BATTERY_FAIL_VOLTAGE)
            {
                //CmdSendBatteryFail(1, voltageValue[BATTERY_2_BATTERY_INDEX]);
            }
           
            //switch
            batteryStatusTmp[BATTERY_1_BATTERY_INDEX] = batteryStatus[BATTERY_1_BATTERY_INDEX];
            batteryStatusTmp[BATTERY_2_BATTERY_INDEX] = batteryStatus[BATTERY_2_BATTERY_INDEX];
            if(voltageValue[BATTERY_1_BATTERY_INDEX] < BATTERY_FAIL_VOLTAGE)
            {
                if(voltageValue[BATTERY_2_BATTERY_INDEX] < BATTERY_FAIL_VOLTAGE)
                {                    
                    #warning temp
                    //setBatterySwitchStatus(FALSE, FALSE);
                    //
                    if(voltageValue[BATTERY_1_BATTERY_INDEX] == 0)
                    {
                        batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_EMPTY;
                    }
                    else
                    {
                        batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_NEED_REPLACE;
                    }
                    
                    if(voltageValue[BATTERY_2_BATTERY_INDEX] == 0)
                    {
                        batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_EMPTY;
                    }
                    else
                    {
                        batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_NEED_REPLACE;
                    }
                    //
                }
                else
                {
                    setBatterySwitchStatus(FALSE, TRUE);
                    //
                    if(voltageValue[BATTERY_1_BATTERY_INDEX] == 0)
                    {
                        batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_EMPTY;
                    }
                    else
                    {
                        batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_NEED_REPLACE;
                    }
                    
                    batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_IN_USE;
                    //
                }
            }
            else
            {
                if(voltageValue[BATTERY_2_BATTERY_INDEX] < BATTERY_FAIL_VOLTAGE)
                {
                    setBatterySwitchStatus(TRUE, FALSE);
                    //
                    batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_IN_USE;
                    if(voltageValue[BATTERY_2_BATTERY_INDEX] == 0)
                    {
                        batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_EMPTY;
                    }
                    else
                    {
                        batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_NEED_REPLACE;
                    }
                    //
                }
                else
                {
                    if(voltageValue[BATTERY_1_BATTERY_INDEX] > voltageValue[BATTERY_2_BATTERY_INDEX])
                    {
                        if(batteryStatus[BATTERY_2_BATTERY_INDEX] == BATTERY_STATUS_NEED_REPLACE)
                        {
                            //重複設定
                            setBatterySwitchStatus(TRUE, FALSE);
                            //
                            batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_IN_USE;
                            batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_NEED_REPLACE;
                            //
                        }
                        else
                        {
                            setBatterySwitchStatus(FALSE, TRUE);
                            //
                            batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_IDLE;
                            batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_IN_USE;
                            //
                        }
                    }
                    else
                    {
                        if(batteryStatus[BATTERY_1_BATTERY_INDEX] == BATTERY_STATUS_NEED_REPLACE)
                        {
                            //重複設定
                            setBatterySwitchStatus(FALSE, TRUE);
                            //
                            batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_NEED_REPLACE;
                            batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_IN_USE;
                            //
                        }
                        else
                        {
                            setBatterySwitchStatus(TRUE, FALSE);
                            //
                            batteryStatus[BATTERY_1_BATTERY_INDEX] = BATTERY_STATUS_IN_USE;
                            batteryStatus[BATTERY_2_BATTERY_INDEX] = BATTERY_STATUS_IDLE;
                            //
                        }
                    }
                }
            } 
            if((batteryStatusTmp[BATTERY_1_BATTERY_INDEX] == BATTERY_STATUS_EMPTY) && (batteryStatus[BATTERY_1_BATTERY_INDEX] != BATTERY_STATUS_EMPTY))
            {
                NT066EResetChip();
            }
            if((batteryStatusTmp[BATTERY_2_BATTERY_INDEX] == BATTERY_STATUS_EMPTY) && (batteryStatus[BATTERY_2_BATTERY_INDEX] != BATTERY_STATUS_EMPTY))
            {
                NT066EResetChip();
            }    
            batteryDrvPowerStatus = TRUE;    
						*/
        //}        
    }
}



void setBatterySwitchStatus(BOOL battery1Switch, BOOL battery2Switch)
{
    //battery1Switch = TRUE;
    //battery2Switch = TRUE;
    BatterySetSwitch1(battery1Switch);
    BatterySetSwitch2(battery2Switch);
    #if(ENABLE_BATTERY_DRV_DEBUG_MESSAGE)
    sysprintf(" --> setBatterySwitchStatus [%d:%d] (enable:%d, %d low:%d, %d)\r\n", battery1Switch, battery2Switch, 
                GPIO_ReadBit(BATTERY_1_ENABLE_PORT, BATTERY_1_ENABLE_PIN), GPIO_ReadBit(BATTERY_2_ENABLE_PORT, BATTERY_2_ENABLE_PIN),
                GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN), GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN));
    #endif
    
}
INT32 EINT5Callback(UINT32 status, UINT32 userData)
{
    //terninalPrintf("Right battery \r\n");
    sysprintf("\r\n - EINT5 0x%08x [%d] - \r\n", status, GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN));
    if(GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN) == 1)
    {
        //terninalPrintf("\r\nRight battery connect.\r\n"); 
        
        //BatteryGetVoltage();				
        //vTaskDelay(500/portTICK_RATE_MS);
        
        batteryConnectMeasureflag = TRUE;
        
        BaseType_t xHigherPriorityTaskWoken;  
        xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR( xActionSemaphore, &xHigherPriorityTaskWoken );
        portEXIT_SWITCHING_ISR (xHigherPriorityTaskWoken);  

        
    }
    else
    {
       // terninalPrintf("\r\nRight battery unplug.\r\n");
    }
    
    if(BatteryCheckPowerDownCondition())
    {
        
        BatterySetSwitch1(FALSE);
        BatterySetSwitch2(FALSE);
        //terninalPrintf("\r\nLeft power close.\r\n");
        //terninalPrintf("\r\nRight power close.\r\n");
				//PCT08SetPower(FALSE);
        QModemTotalStop();
    }
    else if(GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN) == 0)
    {
        
        if(!testLocalMode)
        {
            BatterySetSwitch1(TRUE);
            //terninalPrintf("\r\nLeft power open.\r\n");
        }
    }
    //else
    //{   

    //}

    
    GPIO_ClrISRBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN);
    
    return 0;
}
INT32 EINT6Callback(UINT32 status, UINT32 userData)
{
    //terninalPrintf("Left battery \r\n");
    sysprintf("\r\n - EINT6 0x%08x [%d] - \r\n", status, GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN));
    
    if(GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN) == 1)
    {
        //terninalPrintf("\r\nLeft battery connect.\r\n");        
        //UINT32 leftVoltage,  rightVoltage;
        //BatteryGetVoltage();				
        //vTaskDelay(500/portTICK_RATE_MS);
        
        batteryConnectMeasureflag = TRUE;
        
        BaseType_t xHigherPriorityTaskWoken;  
        xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR( xActionSemaphore, &xHigherPriorityTaskWoken );
        portEXIT_SWITCHING_ISR (xHigherPriorityTaskWoken);  

        
        //BatteryGetValue(&leftVoltage, &rightVoltage);
        //terninalPrintf(" Voltage: [%d], [%d]\r\n", leftVoltage, rightVoltage);
    }
    else
    {
        //terninalPrintf("\r\nLeft battery unplug.\r\n");        
    }        
    
    
    
    if(BatteryCheckPowerDownCondition())
    {

        BatterySetSwitch1(FALSE);
        BatterySetSwitch2(FALSE);
       // terninalPrintf("\r\nLeft power close.\r\n");
       // terninalPrintf("\r\nRight power close.\r\n");
				//PCT08SetPower(FALSE);
        QModemTotalStop();
    }
    else if(GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN) == 0)
    {        
        if(!testLocalMode)
        {
            BatterySetSwitch2(TRUE);
           // terninalPrintf("\r\nRight power open.\r\n");
        }
    }
    //else
   // {

    //}

    
    GPIO_ClrISRBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN);
    
    return 0;
}

#if(SUPPORT_HK_10_HW)  
INT32 EINT7Callback(UINT32 status, UINT32 userData)
{
    
    
    
    sysprintf("\r\n - EINT7(Battery_SOLAR) 0x%08x [%d] - \r\n", status, GPIO_ReadBit(BATTERY_SOLAR_LOW_DETECT_PORT, BATTERY_SOLAR_LOW_DETECT_PIN));
    
    if(GPIO_ReadBit(BATTERY_SOLAR_LOW_DETECT_PORT, BATTERY_SOLAR_LOW_DETECT_PIN) == 0)
    {
        //terninalPrintf("\r\nsolar cell connect.\r\n");
        
        BaseType_t xHigherPriorityTaskWoken;  
        xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR( xActionSemaphore, &xHigherPriorityTaskWoken );
        portEXIT_SWITCHING_ISR (xHigherPriorityTaskWoken);  

        
        //BatteryGetValue(&leftVoltage, &rightVoltage);
        //terninalPrintf("Solar cell voltage: [%d], [%d], [%d]\r\n",REAL_VOLTAGE(voltageValue[BATTERY_0_BATTERY_INDEX]) ,REAL_VOLTAGE(voltageValue[BATTERY_1_BATTERY_INDEX]),REAL_VOLTAGE(voltageValue[BATTERY_2_BATTERY_INDEX]));
        
        
        
    }
    else
    {
        //terninalPrintf("\r\nsolar cell unplug.\r\n");
    }
    #if(0)
    if(BatteryCheckPowerDownCondition() == POWER_CONDITION_BATTERY_REMOVE)
    {
        BatterySetSwitch1(FALSE);
        BatterySetSwitch2(FALSE);
        EPDSetBacklight(FALSE); //
        //ModemAgentStopSend();
        //NT066ESetPower(FALSE); //????? ????
        //LedSetPower(FALSE);
    }
    else if(GPIO_ReadBit(BATTERY_SOLAR_LOW_DETECT_PORT, BATTERY_SOLAR_LOW_DETECT_PIN) == 0)
    {
        if(!testLocalMode)
        {
            batteryFailFlag[BATTERY_SOLAR_BATTERY_INDEX] = 0;
            BatterySetSwitch2(TRUE);   
        }
    }
    #endif
    GPIO_ClrISRBit(BATTERY_SOLAR_LOW_DETECT_PORT, BATTERY_SOLAR_LOW_DETECT_PIN);
    
    return 0;
}
#endif

static BOOL hwInit(void)
{   
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    
    /* Set MFP_GPF6 MFP_GPF7 to output */
    outpw(REG_SYS_GPF_MFPL,(inpw(REG_SYS_GPF_MFPL) & ~(0xF<<24)));
    outpw(REG_SYS_GPF_MFPL,(inpw(REG_SYS_GPF_MFPL) & ~(0xFu<<28)));
    //outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xFu<<24)));
    GPIO_OpenBit(BATTERY_1_ENABLE_PORT, BATTERY_1_ENABLE_PIN, DIR_OUTPUT, NO_PULL_UP);  
    GPIO_OpenBit(BATTERY_2_ENABLE_PORT, BATTERY_2_ENABLE_PIN, DIR_OUTPUT, NO_PULL_UP);
    //GPIO_OpenBit(BATTERY_SOLAR_ENABLE_PORT, BATTERY_SOLAR_ENABLE_PIN, DIR_OUTPUT, NO_PULL_UP);
    

    #if(SUPPORT_HK_10_HW)     
    /* Set MFP_GPE14 to output */
    outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<<24)));
    GPIO_OpenBit(BATTERY_SOLAR_ENABLE_PORT, BATTERY_SOLAR_ENABLE_PIN, DIR_OUTPUT, NO_PULL_UP); 
    //off the power   
    GPIO_SetBit(BATTERY_SOLAR_ENABLE_PORT, BATTERY_SOLAR_ENABLE_PIN);
    #endif   
    
    /* Set PH5 to EINT5 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<20)) | (0xF<<20));    
    /* Configure PH5 to input mode */
    GPIO_OpenBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN, DIR_INPUT, NO_PULL_UP);
    /* Confingure PH5 to both-edge trigger */
    GPIO_EnableTriggerType(GPIOH, BATTERY_2_LOW_DETECT_PIN, BOTH_EDGE);
    //EINT5
    GPIO_EnableEINT(NIRQ5, (GPIO_CALLBACK)EINT5Callback, 0);    
    GPIO_ClrISRBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN);
    
    
    /* Set PH6 to EINT6 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<24)) | (0xF<<24));    
    /* Configure PH6 to input mode */
    GPIO_OpenBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN, DIR_INPUT, NO_PULL_UP);
    /* Confingure PH5 to both-edge trigger */
    GPIO_EnableTriggerType(GPIOH, BATTERY_1_LOW_DETECT_PIN, BOTH_EDGE);
    //EINT6
    GPIO_EnableEINT(NIRQ6, (GPIO_CALLBACK)EINT6Callback, 0);    
    GPIO_ClrISRBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN);
    
    
    
    
    
    #if(SUPPORT_HK_10_HW)    
    /* Set PH7 to EINT7 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<28)) | (0xF<<28));    
    /* Configure PH6 to input mode */
    GPIO_OpenBit(BATTERY_SOLAR_LOW_DETECT_PORT, BATTERY_SOLAR_LOW_DETECT_PIN, DIR_INPUT, NO_PULL_UP);
    /* Confingure PH5 to both-edge trigger */
    GPIO_EnableTriggerType(GPIOH, BATTERY_SOLAR_LOW_DETECT_PIN, BOTH_EDGE);
    //EINT6
    GPIO_EnableEINT(NIRQ7, (GPIO_CALLBACK)EINT7Callback, 0);    
    GPIO_ClrISRBit(BATTERY_SOLAR_LOW_DETECT_PORT, BATTERY_SOLAR_LOW_DETECT_PIN);  
    #endif   

    setBatterySwitchStatus(TRUE, TRUE);    
    //adcOpen(); // move to main.c
    adcIoctl(VBAT_OFF,0,0); 
    adcIoctl(NAC_OFF,0,0); //Disable Normal AD Conversion
    return TRUE;
}
static BOOL swInit(void)
{
    PowerRegCallback(&batteryDrvPowerCallabck);
    xActionSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vBatteryDrvTask, "vBatteryDrvTask", 512, NULL, BATTERY_THREAD_PROI, NULL);
    
    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL BatteryDrvInit(BOOL testMode)
{
		static BOOL initFlag = FALSE;
		if(initFlag == TRUE)
				return TRUE;
    sysprintf("BatteryDrvInit!! [testMode = %d]\n", testMode);
    testLocalMode = testMode;
    if(hwInit() == FALSE)
    {
        sysprintf("BatteryDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(testMode)
    {
    }
    else
    {
        if(swInit() == FALSE)
        {
            sysprintf("BatteryDrvInit ERROR (swInit false)!!\n");
            return FALSE;
        }
    }
		initFlag = TRUE;
    
    
    
    batteryConnectMeasureflag = TRUE;
        
    BaseType_t xHigherPriorityTaskWoken;  
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR( xActionSemaphore, &xHigherPriorityTaskWoken );
    portEXIT_SWITCHING_ISR (xHigherPriorityTaskWoken); 
    
    
    return TRUE;
}

void BatteryGetValue(UINT32* solarBatVoltage, UINT32* battery1Voltage, UINT32* battery2Voltage)
{
    *solarBatVoltage = SREAL_VOLTAGE(voltageValue[BATTERY_0_BATTERY_INDEX]);
    *battery1Voltage = REAL_VOLTAGE(voltageValue[BATTERY_1_BATTERY_INDEX]);
    *battery2Voltage = REAL_VOLTAGE(voltageValue[BATTERY_2_BATTERY_INDEX]);
}

void BatteryGetStatus(uint8_t* battery1Status, uint8_t* battery2Status)
{
    *battery1Status = batteryStatus[BATTERY_1_BATTERY_INDEX];
    *battery2Status = batteryStatus[BATTERY_2_BATTERY_INDEX];
}

void BatterySwitchStatusEx(BOOL battery1Switch)
{
    if(battery1Switch)
    {
        setBatterySwitchStatus(TRUE, TRUE);
        vTaskDelay(200/portTICK_RATE_MS);
        setBatterySwitchStatus(TRUE, FALSE);
    }
    else
    {
        setBatterySwitchStatus(TRUE, TRUE);
        vTaskDelay(200/portTICK_RATE_MS);
        setBatterySwitchStatus(FALSE, TRUE);
    }
}

void BatterySetSwitch1(BOOL flag)
{
    if(flag)
    {        
        GPIO_ClrBit(BATTERY_1_ENABLE_PORT, BATTERY_1_ENABLE_PIN);        
    }
    else
    {
        GPIO_SetBit(BATTERY_1_ENABLE_PORT, BATTERY_1_ENABLE_PIN);  
    }
    #if(ENABLE_BATTERY_DRV_DEBUG_MESSAGE)
    sysprintf(" --> BatterySetSwitch1 [%d] (enable:%d, %d low:%d, %d)\r\n", flag, 
                GPIO_ReadBit(BATTERY_1_ENABLE_PORT, BATTERY_1_ENABLE_PIN), GPIO_ReadBit(BATTERY_2_ENABLE_PORT, BATTERY_2_ENABLE_PIN),
                GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN), GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN));
    #endif
}

void BatterySetSwitch2(BOOL flag)
{
    if(flag)
    {
        GPIO_ClrBit(BATTERY_2_ENABLE_PORT, BATTERY_2_ENABLE_PIN);        
    }
    else
    {
       GPIO_SetBit(BATTERY_2_ENABLE_PORT, BATTERY_2_ENABLE_PIN);
    } 
    #if(ENABLE_BATTERY_DRV_DEBUG_MESSAGE)
    sysprintf(" --> BatterySetSwitch2 [%d] (enable:%d, %d low:%d, %d)\r\n", flag, 
                GPIO_ReadBit(BATTERY_1_ENABLE_PORT, BATTERY_1_ENABLE_PIN), GPIO_ReadBit(BATTERY_2_ENABLE_PORT, BATTERY_2_ENABLE_PIN),
                GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN), GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN));
    #endif
}

BOOL BatteryGetVoltage(void)
{
    sysprintf("===> BatteryGetVoltage ...\r\n");   
    xSemaphoreGive(xActionSemaphore);   
    return TRUE;
    
}

void BatterySetEnableTestMode(BOOL mode)
{
    enableTestBattery = mode;
    if(enableTestBattery)
    {
        threadWaitTime = 1000/portTICK_RATE_MS;//portMAX_DELAY;
    }
    else
    {
        threadWaitTime = 3000/portTICK_RATE_MS;
    }
    xSemaphoreGive(xActionSemaphore);
}

BOOL BatteryCheckPowerDownCondition(void)
{
    //sysprintf(" --> BatteryCheckPowerDownCondition (enable:%d, %d  low:%d, %d)\r\n", GPIO_ReadBit(BATTERY_1_ENABLE_PORT, BATTERY_1_ENABLE_PIN), GPIO_ReadBit(BATTERY_2_ENABLE_PORT, BATTERY_2_ENABLE_PIN), GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN), GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN));
    #if(ENABLE_POWER_DOWN)
    if((GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN) == 0) &&
        (GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN) == 0))
    {
        sysprintf("\r\n - INFO -> BatteryCheckPowerDownCondition \r\n");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    #else
    return FALSE;
    #endif
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


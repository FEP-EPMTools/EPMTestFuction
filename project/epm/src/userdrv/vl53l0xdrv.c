/**************************************************************************//**
* @file     vl53l0xdrv.c
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
#include "vl53l0xdrv.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ENABLE_VL53L0X_DRV_DEBUG_MESSAGE   0

#define I2C_PORT    0

#define DEVICE_1_ENABLE_PIN_PORT  GPIOI
#define DEVICE_1_ENABLE_PORT_BIT  BIT14

#define DEVICE_2_ENABLE_PIN_PORT  GPIOI
#define DEVICE_2_ENABLE_PORT_BIT  BIT15


#define VL53L0X_DEFAULT_I2C_ID          0x29 //(0x52>>1)

#define VL53L0X_BASE_I2C_ID                0x21

#define VL53L0X_DEVICE_NUM          2    


static VL53L0X_Dev_t MyDevice[VL53L0X_DEVICE_NUM];

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static I2cInterface* pI2cInterface;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void print_pal_error(VL53L0X_Error Status)
{
    char buf[VL53L0X_MAX_STRING_LENGTH];
    VL53L0X_GetPalErrorString(Status, buf);
    sysprintf("API Status: %i : %s\n", Status, buf);    
}
static void enableDevice(int index)
{
    switch(index)
    {
        case VL53L0X_DEVICE_1:
            GPIO_SetBit(DEVICE_1_ENABLE_PIN_PORT, DEVICE_1_ENABLE_PORT_BIT);
            GPIO_ClrBit(DEVICE_2_ENABLE_PIN_PORT, DEVICE_2_ENABLE_PORT_BIT);
            break;
        case VL53L0X_DEVICE_2:
            //GPIO_ClrBit(GPIOI, BIT14);
            //GPIO_SetBit(GPIOI, BIT15);
            GPIO_SetBit(DEVICE_1_ENABLE_PIN_PORT, DEVICE_1_ENABLE_PORT_BIT);
            GPIO_SetBit(DEVICE_2_ENABLE_PIN_PORT, DEVICE_2_ENABLE_PORT_BIT);
            break;
        case VL53L0X_DEVICE_ALL:
            GPIO_SetBit(DEVICE_1_ENABLE_PIN_PORT, DEVICE_1_ENABLE_PORT_BIT);
            GPIO_SetBit(DEVICE_2_ENABLE_PIN_PORT, DEVICE_2_ENABLE_PORT_BIT);
            break;
    }
    
    
    
}
static BOOL rangingTest(VL53L0X_Dev_t *pMyDevice, int* currentMeasureDist)
{   
    static int times = 0;
    int i;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    VL53L0X_RangingMeasurementData_t    RangingMeasurementData;
    int distSum;
    int distNum;
    
    distSum = 0;
    distNum = 0;

    for(i=0;i<5;i++)
    {
        TickType_t mTick = xTaskGetTickCount();
            
        Status = VL53L0X_PerformSingleRangingMeasurement(pMyDevice, &RangingMeasurementData);
        if (Status != VL53L0X_ERROR_NONE) 
        {
            return FALSE;
        }
        else
        {
            if(RangingMeasurementData.RangeStatus == 0)   
            {                    
                   
                distNum++;
                distSum = distSum + RangingMeasurementData.RangeMilliMeter;
            }
            else
            {
                   
            }

        }
            
        times++;
        vTaskDelay(20/portTICK_RATE_MS);
    }
    if(distNum >= 3)
    {
        #if(ENABLE_VL53L0X_DRV_DEBUG_MESSAGE)
        sysprintf(" -- [INFO] %s Measured distance OK [%d]: %icm --\n", pMyDevice->deviceName, distNum, distSum/distNum/10);
        #endif
        *currentMeasureDist = distSum/distNum/10;
    }
    else
    {
        #if(ENABLE_VL53L0X_DRV_DEBUG_MESSAGE)
        sysprintf(" -- [INFO] %s Measured distance ERROR [%d]--\n", pMyDevice->deviceName, distNum);
        #endif
        *currentMeasureDist = 0xffff;
    }
    return TRUE;
}
static VL53L0X_Error rangingInit(VL53L0X_Dev_t *pMyDevice)
{
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    uint8_t VhvSettings;
    uint8_t PhaseCal;

    if(Status == VL53L0X_ERROR_NONE)
    {
        sysprintf ("VL53L0XInit: VL53L0X_StaticInit\n");
        Status = VL53L0X_StaticInit(pMyDevice); // Device Initialization
        print_pal_error(Status);
    }
    
    if(Status == VL53L0X_ERROR_NONE)
    {
        sysprintf ("VL53L0XInit: VL53L0X_PerformRefCalibration\n");
        Status = VL53L0X_PerformRefCalibration(pMyDevice,
        		&VhvSettings, &PhaseCal); // Device Initialization
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {
        sysprintf ("VL53L0XInit: VL53L0X_PerformRefSpadManagement\n");
        Status = VL53L0X_PerformRefSpadManagement(pMyDevice,
        		&refSpadCount, &isApertureSpads); // Device Initialization
        sysprintf ("refSpadCount = %d, isApertureSpads = %d\n", refSpadCount, isApertureSpads);
        print_pal_error(Status);
    }

    if(Status == VL53L0X_ERROR_NONE)
    {

        // no need to do this when we use VL53L0X_PerformSingleRangingMeasurement
        sysprintf ("VL53L0XInit: VL53L0X_SetDeviceMode\n");
        Status = VL53L0X_SetDeviceMode(pMyDevice, VL53L0X_DEVICEMODE_SINGLE_RANGING); // Setup in single ranging mode
        print_pal_error(Status);
    }

    // Enable/Disable Sigma and Signal check
	
    if (Status == VL53L0X_ERROR_NONE) {
        Status = VL53L0X_SetLimitCheckEnable(pMyDevice,
        		VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
    }
    if (Status == VL53L0X_ERROR_NONE) {
        Status = VL53L0X_SetLimitCheckEnable(pMyDevice,
        		VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
    }
  
    if (Status == VL53L0X_ERROR_NONE) {
        Status = VL53L0X_SetLimitCheckValue(pMyDevice,
        		VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
        		(FixPoint1616_t)(0.1*65536));
	}			
    if (Status == VL53L0X_ERROR_NONE) {
        Status = VL53L0X_SetLimitCheckValue(pMyDevice,
        		VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
        		(FixPoint1616_t)(60*65536));			
    }
    if (Status == VL53L0X_ERROR_NONE) {
        Status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(pMyDevice,
        		33000);
	}
	
    if (Status == VL53L0X_ERROR_NONE) {
        Status = VL53L0X_SetVcselPulsePeriod(pMyDevice, 
		        VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
    }
    if (Status == VL53L0X_ERROR_NONE) {
        Status = VL53L0X_SetVcselPulsePeriod(pMyDevice, 
		        VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);
    }
    return Status;
}
static VL53L0X_Error vVL53L0XLibraryInit(VL53L0X_Dev_t *pMyDevice)
{
    TickType_t mTick = xTaskGetTickCount();
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    Status = VL53L0X_DataInit(pMyDevice); // Data initialization
    
    if(Status == VL53L0X_ERROR_NONE)
    {
        sysprintf ("VL53L0XInit: VL53L0X_GetDeviceInfo OK !!! call rangingInit...\n");
        Status = rangingInit(pMyDevice); 
    }
    else
    {
        sysprintf ("VL53L0XInit: VL53L0X_GetDeviceInfo error (%d) !!!\n", Status);
    }
    sysprintf (" [INFO] --> vVL53L0XLibraryInit cost %d ms !!!\n", xTaskGetTickCount() - mTick);
    return Status;

}

static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    /* Set MFP_GPI14 MFP_GPI15 to output */
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<24)));
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xFu<<28)));
    GPIO_OpenBit(DEVICE_1_ENABLE_PIN_PORT, DEVICE_1_ENABLE_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);  
    GPIO_OpenBit(DEVICE_2_ENABLE_PIN_PORT, DEVICE_2_ENABLE_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);
    GPIO_ClrBit(DEVICE_1_ENABLE_PIN_PORT, DEVICE_1_ENABLE_PORT_BIT);
    GPIO_ClrBit(DEVICE_2_ENABLE_PIN_PORT, DEVICE_2_ENABLE_PORT_BIT);
    return TRUE;
}

static BOOL swInit(void)
{
    int i;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    VL53L0X_DeviceInfo_t                DeviceInfo;
    int32_t status_int;
    uint8_t ProductRevisionMajor, ProductRevisionMinor;
    
    Status = VL53L0X_i2c_init(pI2cInterface);
    if (Status != VL53L0X_ERROR_NONE) 
    {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        sysprintf ("VL53L0XInit:  VL53L0X_ERROR_CONTROL_INTERFACE\n");
        return FALSE;
    } 
    else
    {
        sysprintf ("VL53L0XInit: Init Comms\n");
    }
    for(i = 0; i<VL53L0X_DEVICE_NUM; i++)
    {
        enableDevice(i);
        vTaskDelay(200/portTICK_RATE_MS);
        switch(i)
        {
            case VL53L0X_DEVICE_1:
                MyDevice[i].deviceName = "[ VL53L0X_1 ]";
                break;
            case VL53L0X_DEVICE_2:
                MyDevice[i].deviceName = "[ VL53L0X_2 ]";
                break;
        }
        
        MyDevice[i].I2cDevAddr = VL53L0X_DEFAULT_I2C_ID;
        
        if(Status == VL53L0X_ERROR_NONE)
        {
            status_int = VL53L0X_GetProductRevision(&MyDevice[i], &ProductRevisionMajor, &ProductRevisionMinor);
            if (status_int != 0)
                Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        }
         if(Status == VL53L0X_ERROR_NONE)
        {
            sysprintf("VL53L0XInit[%d]: VL53L0X GetProductRevision %d %d !!, OK !!! call VL53L0X_DataInit...\n", i, ProductRevisionMajor, ProductRevisionMinor);
            Status = vVL53L0XLibraryInit(&MyDevice[i]); // Data initialization
            print_pal_error(Status);
        }
        else
        {
            sysprintf ("VL53L0XInit[%d]: VL53L0X_GetProductRevision error (%d) !!!\n", i, Status);
        }
         
        if(Status == VL53L0X_ERROR_NONE)
        {
            /*
            VL53L0X_GetDeviceInfo:
            Device Name : VL53L0X ES1 or later
            Device Type : VL53L0X
            Device ID : VL53L0CBV0DH/1$1
            ProductRevisionMajor : 1
            ProductRevisionMinor : 1
            API Status: 0 : No Error
            */
            sysprintf ("VL53L0XInit[%d]: VL53L0X_DataInit OK !!! call VL53L0X_GetDeviceInfo...\n", i);
            Status = VL53L0X_GetDeviceInfo(&MyDevice[i], &DeviceInfo);
            if(Status == VL53L0X_ERROR_NONE)
            {
                sysprintf("VL53L0X_GetDeviceInfo:\n", i);
                sysprintf("Device Name : %s\n", DeviceInfo.Name);
                sysprintf("Device Type : %s\n", DeviceInfo.Type);
                sysprintf("Device ID : %s\n", DeviceInfo.ProductId);
                sysprintf("ProductRevisionMajor : %d\n", DeviceInfo.ProductRevisionMajor);
                sysprintf("ProductRevisionMinor : %d\n", DeviceInfo.ProductRevisionMinor);

                if ((DeviceInfo.ProductRevisionMinor != 1) && (DeviceInfo.ProductRevisionMinor != 1)) 
                {
                    sysprintf("Error expected cut 1.1 but found cut %d.%d\n", DeviceInfo.ProductRevisionMajor, DeviceInfo.ProductRevisionMinor);
                    Status = VL53L0X_ERROR_NOT_SUPPORTED;
                }
            }
            print_pal_error(Status);
        }
        else
        {
            sysprintf ("VL53L0XInit[%d]: VL53L0X_DataInit error (%d) !!!\n", i, Status);
        }
        #if(1)
        
        
        if(VL53L0X_SetDeviceAddress(&MyDevice[i], VL53L0X_BASE_I2C_ID + i) == VL53L0X_ERROR_NONE)
        {
            sysprintf ("VL53L0XInit[%d]: VL53L0X_SetDeviceAddress 0x%02x OK (%d) !!!\n", i, VL53L0X_BASE_I2C_ID + i, Status);
        }
        else
        {
            sysprintf ("VL53L0XInit[%d]: VL53L0X_SetDeviceAddress 0x%02x error (%d) !!!\n", i, VL53L0X_BASE_I2C_ID + i, Status);
        }
        MyDevice[i].I2cDevAddr = VL53L0X_BASE_I2C_ID + i;
        #endif
    }
    if(Status == VL53L0X_ERROR_NONE)
        return TRUE;
    else
        return FALSE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL Vl53l0xDrvInit(void)
{
    sysprintf("Vl53l0xDrvInit!!\n");
    pI2cInterface = I2cGetInterface(I2C_0_INTERFACE_INDEX);
    if(pI2cInterface == NULL)
    {
        sysprintf("Vl53l0xDrvInit ERROR (pI2cInterface == NULL)!!\n");
        return FALSE;
    }
    
    if(pI2cInterface->initFunc() == FALSE)
    {
        sysprintf("Vl53l0xDrvInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    
    if(hwInit() == FALSE)
    {
        sysprintf("Vl53l0xDrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("Vl53l0xDrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }
    return TRUE;
}

BOOL Vl53l0xMeasureDist(uint8_t id, int* detectResult)
{
    return rangingTest(&MyDevice[id], detectResult); 
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


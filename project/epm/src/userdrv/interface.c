/**************************************************************************//**
* @file     interface.c
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
#include "uart.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "interface.h"
#include "uartdrv.h"
#include "uart0drv.h"
#include "uart1drv.h"
#include "uart2drv.h"
#include "uart3drv.h"
#include "uart4drv.h"
#include "uart7drv.h"
#include "uart8drv.h"
#include "uart10drv.h"
#include "spi0drv.h"
#include "spi1drv.h"
#include "i2c0drv.h"
#include "i2c1drv.h"
#include "vl53l0xdrv.h"
#include "nt066edrv.h"
#include "flashdrv.h"
#include "loradrv.h"
#include "dipdrv.h"
#include "rs232CommDrv.h"
#include "mtp232CommDrv.h"
#include "sddrv.h"
#include "flashdrvex.h"
#include "sr04tdrv.h"
#include "radardrv.h"
#include "lidardrv.h"
#include "rb60pOTA.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

//************************  UART  ************************//
static UartInterface mUartInterface[UART_INTERFACE_NUM] = {{UART1DrvInit, UART1Write, UART1Read, UART1ReadWait, UART1SetPower, UART1SetRS232Power, UART1Ioctl},
                                                            {UART2DrvInit, UART2Write, UART2Read, UART2ReadWait, UART2SetPower, UART2SetRS232Power, UART2Ioctl},
                                                            {UART3DrvInit, UART3Write, UART3Read, UART3ReadWait, UART3SetPower, UART3SetRS232Power, UART3Ioctl},
                                                            {UART4DrvInit, UART4Write, UART4Read, UART4ReadWait, UART4SetPower, UART4SetRS232Power, UART4Ioctl},
                                                            {UART7DrvInit, UART7Write, UART7Read, UART7ReadWait, UART7SetPower, UART7SetRS232Power, UART7Ioctl},
                                                            {UART8DrvInit, UART8Write, UART8Read, UART8ReadWait, UART8SetPower, UART8SetRS232Power, UART8Ioctl},
                                                            {UART10DrvInit, UART10Write, UART10Read, UART10ReadWait, UART10SetPower, UART10SetRS232Power, UART10Ioctl}
                                                            
//#if (ENABLE_MTP_FUNCTION)
                                                            ,{UART0DrvInit, UART0Write, UART0Read, UART0ReadWait, UART0SetPower, UART0SetRS232Power, UART0Ioctl}
//#endif
                                                            };
//************************  SPI  ************************//
static SpiInterface mSpiInterface[SPI_INTERFACE_NUM] = {{Spi0DrvInit, Spi0Write, Spi0Read, Spi0ActiveCS, Spi0SetPin, Spi0ResetPin},
                                                        {Spi1DrvInit, Spi1Write, Spi1Read, Spi1ActiveCS, Spi1SetPin, Spi1ResetPin}};

//************************  I2C  ************************//
static I2cInterface mI2cInterface[I2C_INTERFACE_NUM] = {{I2c1DrvInit, I2c1Ioctl, I2c1Write, I2c1Read, I2c1enableCriticalSectionFunc, I2c1SetPin, I2c1ResetPin}};
//************************  KEY  ************************//
static KeyHardwareInterface mKeyHardwareInterface[KEY_HARDWARE_INTERFACE_NUM] = {{NT066EDrvInit, NT066ESetCallbackFunc, NULL/*NT066ESetPower*/}, {DipDrvInit, DipSetCallbackFunc, NULL}};

//************************  Distance  ************************//
static DistInterface mDistInterface[DIST_INTERFACE_NUM] = {{SR04TDrvInit, SR04TMeasureDist}};

//************************  Storage  ************************//
//static StorageInterface mStorageInterface[STORAGE_INTERFACE_NUM] = {{FlashDrvInit, FlashWrite, FlashRead}};

//************************  Communication  ************************//
static CommunicationInterface mCommunicationInterface[COMMUNICATION_INTERFACE_NUM] = {{RS232CommDrvInit, RS232CommDrvWrite, RS232CommDrvRead, RS232CommDrvReadWait},
                                                                                      {MTP232CommDrvInit, MTP232CommDrvWrite, MTP232CommDrvRead, MTP232CommDrvReadWait}
                                                                                     };

//************************  FatFs Hardware  ************************//
static FatfsHardwareInterface mFatfsHardwareInterface[FATFS_HARDWARE_INTERFACE_NUM] = {{SdDrvInit, SDDrvInitialize, SDDrvStatus, SDDrvRead, SDDrvWrite, SDDrvIoctl, 512}, 
                                                                                        {FlashDrvExInit, FlashDrvExInitialize, FlashDrvExStatus, FlashDrvExRead, FlashDrvExWrite, FlashDrvExIoctl, SPI_FLASH_EX_SECTOR_SIZE},
                                                                                        {FlashDrvExInit, FlashDrvExInitialize, FlashDrvExStatus, FlashDrvExRead, FlashDrvExWrite, FlashDrvExIoctl, SPI_FLASH_EX_SECTOR_SIZE}
                                                                                       };

//************************  RADAR  ************************//
static RadarInterface mRadarInterface[RADAR_INTERFACE_NUM] ={{RadarDrvInit, RadarCheckFeature, RadarSetPower, RadarSetPowerStatus,RadarSetStartCalibrate,RadarCalibrate,RadarReadDistValue,RadarSetQueryVersion,
                                                              ReadRadarVersion,ReadRadarVersionString,RadarFirstOTA,RadarOTA,RadarRecentDistValue,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
                                                                {LidarDrvInit, LidarCheckFeature, LidarSetPower, LidarSetPowerStatus,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
                                                             //{RadarDrvInit, newRadarResult, RadarSetPower,RadarSetPowerStatus,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
                                                             {RadarDrvInit, NULL, RadarSetPower,RadarSetPowerStatus,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,newRadarResult,newRadarResultElite,RadarTaskCreate,
                                                              RadarTaskDelete,RB60POTAFunc,newRadarFlush,RadarDrvInitBurning}
                                                            };                                                                                    
//static RadarInterface mRadarInterface[RADAR_INTERFACE_NUM] ={{RadarDrvInit, RadarCheckFeature, RadarSetPowerStatus, RadarSetPower,RadarSetStartCalibrate}};      

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
//************************  UART  ************************//
UartInterface* UartGetInterface(uint8_t index)
{
    if(index < UART_INTERFACE_NUM)
    {        
        return &mUartInterface[index];
    }
    else
    {
        return NULL;
    }
}
//************************  SPI  ************************//
SpiInterface* SpiGetInterface(uint8_t index)
{
    if(index < SPI_INTERFACE_NUM)
    {        
        return &mSpiInterface[index];
    }
    else
    {
        return NULL;
    }
}

//************************  I2C  ************************//
I2cInterface* I2cGetInterface(uint8_t index)
{
    if(index < I2C_INTERFACE_NUM)
    {        
        return &mI2cInterface[index];
    }
    else
    {
        return NULL;
    }
}

//************************  KEY Hardware ************************//
KeyHardwareInterface* KeyHardwareGetInterface(uint8_t index)
{
    if(index < KEY_HARDWARE_INTERFACE_NUM)
    {        
        return &mKeyHardwareInterface[index];
    }
    else
    {
        return NULL;
    }
}

//************************  Distance  ************************//
DistInterface* DistGetInterface(uint8_t index)
{
    if(index < DIST_INTERFACE_NUM)
    {        
        return &mDistInterface[index];
    }
    else
    {
        return NULL;
    }
}
//************************  Storage  ************************//
/*
StorageInterface* StorageGetInterface(uint8_t index)
{
    if(index < STORAGE_INTERFACE_NUM)
    {        
        return &mStorageInterface[index];
    }
    else
    {
        return NULL;
    }
}
*/
//************************  Communication  ************************//
CommunicationInterface* CommunicationGetInterface(uint8_t index)
{
    if(index < COMMUNICATION_INTERFACE_NUM)
    {        
        return &mCommunicationInterface[index];
    }
    else
    {
        return NULL;
    }
}
//************************  FatFs Hardware  ************************//
FatfsHardwareInterface* FatfsHardwareGetInterface(uint8_t index)
{
    if(index < FATFS_HARDWARE_INTERFACE_NUM)
    {        
        return &mFatfsHardwareInterface[index];
    }
    else
    {
        return NULL;
    }
}

//************************  RADAR  ************************//
RadarInterface* RadarGetInterface(uint8_t index)
{
    if(index < RADAR_INTERFACE_NUM)
    {        
        return &mRadarInterface[index];
    }
    else
    {
        return NULL;
    }
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


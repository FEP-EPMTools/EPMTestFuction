/**************************************************************************//**
* @file     interface.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include "nuc970.h"
#include "uart.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "vl53l0xdrv.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//************************  UART  ************************//
#define UART_INTERFACE_NUM          8 //7

#define UART_1_INTERFACE_INDEX      0
#define UART_2_INTERFACE_INDEX      1
#define UART_3_INTERFACE_INDEX      2    
#define UART_4_INTERFACE_INDEX      3 
#define UART_7_INTERFACE_INDEX      4
#define UART_8_INTERFACE_INDEX      5    
#define UART_10_INTERFACE_INDEX     6 
#define UART_0_INTERFACE_INDEX      7    
    
#define UART_IOC_SET_OCTOPUS_MODE   40  //must exceed UART_IOC_SET_LIN_MODE        	 31     /*!< Select LIN Mode */

typedef BOOL (*uartInitFunc)(UINT32 baudRate);
typedef INT32(*uartWriteFunc)(PUINT8 pucBuf, UINT32 uLen);
typedef INT32(*uartReadFunc)(PUINT8 pucBuf, UINT32 uLen);
typedef BaseType_t(*uartReadWaitFunc)(TickType_t time);
typedef BOOL(*uartSetPowerFunc)(BOOL flag);
typedef INT (*uartIoctlFunc)(UINT32 uCmd, UINT32 uArg0, UINT32 uArg1);

//************************  SPI  ************************//
#define SPI_INTERFACE_NUM           2

#define SPI_0_INTERFACE_INDEX       0
#define SPI_1_INTERFACE_INDEX       1

    
typedef BOOL(*spiInitFunc)(void);
typedef void(*spiWriteFunc)(uint8_t buff_id, uint32_t data);
typedef uint32_t(*spiReadFunc)(uint8_t buff_id);
typedef void(*spiActiveCS)(BOOL active);
typedef void(*spiSetPin)(void);
typedef void(*spiResetPin)(void);

//************************  I2C  ************************//
#define I2C_INTERFACE_NUM           1

#define I2C_1_INTERFACE_INDEX       0
    
typedef BOOL(*i2cInitFunc)(void);
typedef int32_t(*i2cIoctlFunc)(uint32_t cmd, uint32_t arg0, uint32_t arg1);
typedef int32_t(*i2cReadFunc)(uint8_t* buf, uint32_t len);
typedef int32_t(*i2cWriteFunc)(uint8_t* buf, uint32_t len);
typedef void(*i2cenableCriticalSectionFunc)(BOOL flag);
typedef void(*i2cSetPin)(void);
typedef void(*i2cResetPin)(void);

//************************  KEY  ************************//
#define KEY_HARDWARE_INTERFACE_NUM               2

#define KEY_HARDWARE_PCF8885_INTERFACE_INDEX     0
#define KEY_HARDWARE_DIP_INTERFACE_INDEX         1
    
#define KEY_HARDWARE_DOWN_EVENT                  0x1
#define KEY_HARDWARE_UP_EVENT                    0x2
#define KEY_HARDWARE_ERROR_EVENT                 0xf

typedef BOOL(*keyHardwareCallbackFunc)(uint8_t keyId, uint8_t downUp);
    
typedef BOOL(*keyHardwareInitFunc)(BOOL testModeFlag);
typedef void(*keyHardwareSetCallbackFunc)(keyHardwareCallbackFunc func);
typedef BOOL(*keyHardwareSetPowerFunc)(BOOL powerFlag);

//************************  Distance  ************************//
#define DIST_INTERFACE_NUM              1

#define DIST_SR04T_INTERFACE_INDEX      0

#define DIST_DEVICE_1  0
#define DIST_DEVICE_2  1
    
typedef BOOL(*distInitFunc)(void);
typedef BOOL(*distMeasureDist)(uint8_t id, int* detectResult);

//************************  Storage  ************************//
#define STORAGE_INTERFACE_NUM              1

#define STORAGE_FLASH_INTERFACE_INDEX       0
    
typedef BOOL(*storageInitFunc)(void);
typedef BOOL(*storageWrite)(uint8_t *u8DataBuffer, int BuffLen);
typedef BOOL(*storageRead)(uint8_t *u8DataBuffer, int BuffLen);

//************************  Communication  ************************//
#define COMMUNICATION_INTERFACE_NUM                 2 //1

#define COMMUNICATION_RS232_INTERFACE_INDEX         0
#define MTP_RS232_INTERFACE_INDEX                   1

typedef BOOL(*commInitFunc)(void);
typedef INT32(*commWrite)(PUINT8 pucBuf, UINT32 uLen);
typedef INT32(*commRead)(PUINT8 pucBuf, UINT32 uLen);
typedef BaseType_t(*commReadWaitFunc)(TickType_t time);

//************************  FatFs Hardware  ************************//
#define FATFS_HARDWARE_INTERFACE_NUM                     3

#define FATFS_HARDWARE_SD_INTERFACE_INDEX                0
#define FATFS_HARDWARE_SFLASH_0_INTERFACE_INDEX          1
#define FATFS_HARDWARE_SFLASH_1_INTERFACE_INDEX          2

typedef BOOL(*fatfsHwInitFunc)(void);
typedef BOOL(*diskInitialize)(uint8_t pdrv);
typedef BOOL(*diskStatus)(uint8_t pdrv);
typedef BOOL(*diskRead)(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count);
typedef BOOL(*diskWrite)(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count);
typedef BOOL(*diskIoctl)(uint8_t pdrv, uint8_t cmd, void *buff );

//************************  Radar  ************************//
//下面三行為舊版(光達跟雷達是分開的)
//#define RADAR_INTERFACE_NUM                     2//雷達介面數量
//#define RADAR_AV_DESIGN_INTERFACE_INDEX         0//雷達引數(index)
//#define LIDAR_AV_DESIGN_INTERFACE_INDEX         1//光達引數(index)

#define RADAR_INTERFACE_NUM                     3 //2
#define RADAR_AV_DESIGN_INTERFACE_INDEX         0
#define LIDAR_AV_DESIGN_INTERFACE_INDEX         1
#define NEWRADAR_INTERFACE_INDEX                2

#define LIDAR_FEATURE_OCCUPIED      0x01
#define LIDAR_FEATURE_VACUUM        0x02
#define LIDAR_FEATURE_UN_STABLED    0x03
#define LIDAR_FEATURE_INIT          0x04
#define LIDAR_FEATURE_IGNORE        0x05
#define LIDAR_FEATURE_FAIL          0xFF

#define RADAR_FEATURE_OCCUPIED                  0x01
#define RADAR_FEATURE_OCCUPIED_UN_STABLED       0x02
#define RADAR_FEATURE_VACUUM                    0x03
#define RADAR_FEATURE_VACUUM_UN_STABLED         0x04
#define RADAR_FEATURE_INIT                      0x05
#define RADAR_FEATURE_IGNORE                    0x06


#define RADAR_FEATURE_OCCUPIED_LIDAR_FAIL                  0x31
#define RADAR_FEATURE_OCCUPIED_UN_STABLED_LIDAR_FAIL       0x32
#define RADAR_FEATURE_VACUUM_LIDAR_FAIL                    0x33
#define RADAR_FEATURE_VACUUM_UN_STABLED_LIDAR_FAIL         0x34
#define RADAR_FEATURE_INIT_LIDAR_FAIL                      0x35
#define RADAR_FEATURE_IGNORE_LIDAR_FAIL                    0x36

#define RADAR_FEATURE_FAIL                      0xFF

#define RADAR_OCCUPIED_TYPE_POWER_1             0x10
#define RADAR_OCCUPIED_TYPE_POWER_2             0x11
#define RADAR_OCCUPIED_TYPE_DISTANCE            0x20
#define RADAR_VACUUM_TYPE                       0x00

#define RADAR_QUERY_VERSION_OK                  0x50
#define RADAR_CALIBRATION_OK                    0x51


#if (ENABLE_BURNIN_TESTER)
#define VOS_INDEX_0                           0x0
#define VOS_INDEX_1                           0x1

//新版雷達API定義
#define VOS_FEATURE_OCCUPIED                  0x01
#define VOS_FEATURE_OCCUPIED_UN_STABLED       0x02
#define VOS_FEATURE_VACUUM                    0x03
#define VOS_FEATURE_VACUUM_UN_STABLED         0x04
#define VOS_FEATURE_INIT                      0x05
#define VOS_FEATURE_IGNORE                    0x06
#define VOS_FEATURE_FAIL                      0xFF

#define VOS_OCCUPIED_TYPE_POWER_1             0x10
#define VOS_OCCUPIED_TYPE_POWER_2             0x11
#define VOS_OCCUPIED_TYPE_DISTANCE            0x20
#define VOS_VACUUM_TYPE                       0x00

//0 (from 0xff):不確定狀態  1:存在物體  2:不存在物體  3:物體正在入庫  4:物體正在出庫
#define VOS_OBJECT_MOVING_UNSTABLE            0x00
#define VOS_OBJECT_MOVING_EXIST               0x01
#define VOS_OBJECT_MOVING_NON_EXIST           0x02
#define VOS_OBJECT_MOVING_ENTERING            0x03
#define VOS_OBJECT_MOVING_LEAVING             0x04
#define VOS_OBJECT_MOVING_ERROR               0x05
#endif




typedef BOOL(*radarInitialize)(void);
//typedef int (*radarCheckFeature)(int index, BOOL* changeFlag, void* para1, void* para2);
typedef int (*radarCheckFeature)(int index, BOOL* changeFlag, void* para1, void* para2, void* para3);
typedef void(*radarSetPowerStatusFunc)(int index, BOOL flag);
typedef void(*radarSetPowerFunc)(BOOL flag);
//練習
typedef void(*radarSetStartCalibrate)(int lidarIndex, BOOL flag);
typedef void(*radarSetQueryVersion)(int lidarIndex, BOOL flag);


typedef void(*radarReadQueryVersion)(int lidarIndex, uint8_t* code1 ,uint8_t* code2, uint8_t* code3, uint8_t* code4);
typedef int(*radarReadQueryVersionString)(int lidarIndex, char* VersionStr);


typedef int (*radarCalibrateFunc)(int index, BOOL* changeFlag,int* dist1 ,int* dist2,void* para1, void* para2, void* para3);

typedef int (*radarReadDistValueFunc)(int index,int* dist);


typedef int (*radarCaptureEmptyFeature)(int radarIndex);
//typedef BOOL (*radarSetEmptyFeature)(int radarIndex, pm_feature* feature);
//typedef BOOL (*radarGetEmptyFeature)(int radarIndex, pm_feature* feature);
typedef BOOL (*radarCheckEmptyFeaturnFunc)(int radarIndex);

typedef void(*radarSetPowerExFunc)(int radarIndex, BOOL flag);


typedef BOOL (*radarFirstOTA)(int radarIndex, int BufferLen);
typedef BOOL (*radarOTA)(int radarIndex, uint8_t* ReadBuffer, int BufferIndex, int ReadBufferLen);

typedef BOOL (*radarRecentDistValue)(int radarIndex, uint16_t* lidarDist,  uint16_t* radarDist);

typedef int (*radarResult)(int radarIndex, uint8_t radarCmd, uint8_t* CmdBuff, uint8_t* radarData);

typedef void(*radarCreateTask)(void);
typedef void(*radarDeleteTask)(void);

typedef BOOL (*radarRB60POTA)(int radarIndex);
typedef void (*radarFlush)(int radarIndex);
typedef BOOL (*radarInitBurning)(int radarIndex);


/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
//************************  UART  ************************//
typedef struct
{
    uartInitFunc        initFunc;
    uartWriteFunc       writeFunc;
    uartReadFunc        readFunc;
    uartReadWaitFunc    readWaitFunc;
    uartSetPowerFunc    setPowerFunc;
    uartSetPowerFunc    setRS232PowerFunc;
    uartIoctlFunc       ioctlFunc;
}UartInterface;

UartInterface* UartGetInterface(uint8_t index);

//************************  SPI  ************************//
typedef struct
{
    spiInitFunc         initFunc;
    spiWriteFunc        writeFunc;
    spiReadFunc         readFunc;
    spiActiveCS         activeCSFunc;
    spiSetPin           setPin;
    spiResetPin         resetPin;
}SpiInterface;

SpiInterface* SpiGetInterface(uint8_t index);

//************************  I2C  ************************//
typedef struct
{
    i2cInitFunc         initFunc;
    i2cIoctlFunc        ioctlFunc;
    i2cWriteFunc        writeFunc;
    i2cReadFunc         readFunc;
    i2cenableCriticalSectionFunc enableCriticalSectionFunc;
    i2cSetPin           setPin;
    i2cResetPin         resetPin;    
}I2cInterface;

I2cInterface* I2cGetInterface(uint8_t index);

//************************  KEY  Hardware************************//
typedef struct
{
    keyHardwareInitFunc             initFunc;
    keyHardwareSetCallbackFunc      setCallbackFunc;
    keyHardwareSetPowerFunc         setPowerFunc;
}KeyHardwareInterface;

KeyHardwareInterface* KeyHardwareGetInterface(uint8_t index);

//************************  Distance  ************************//
typedef struct
{
    distInitFunc         initFunc;
    distMeasureDist      measureDistFunc;
}DistInterface;

DistInterface* DistGetInterface(uint8_t index);

//************************  Storage  ************************//
typedef struct
{
    storageInitFunc         initFunc;
    storageWrite            writeFunc;
    storageRead             readFunc;
}StorageInterface;

StorageInterface* StorageGetInterface(uint8_t index);


//************************  Communication  ************************//
typedef struct
{
    commInitFunc            initFunc;
    commWrite               writeFunc;
    commRead                readFunc;
    commReadWaitFunc        readWaitFunc;
}CommunicationInterface;

CommunicationInterface* CommunicationGetInterface(uint8_t index);

//************************  FatfsDrv  ************************//

typedef struct
{
    fatfsHwInitFunc     initFunc;
    diskInitialize      diskInitFunc;
    diskStatus          diskStatusFunc;
    diskRead            diskReadFunc;
    diskWrite           diskWriteFunc;
    diskIoctl           diskIoctlFunc;
    uint16_t            diskSectorSize;
}FatfsHardwareInterface;

FatfsHardwareInterface* FatfsHardwareGetInterface(uint8_t index);

//************************  Radar  ************************//
//typedef struct
//{
//    radarInitialize             initFunc;
//    radarCheckFeature           checkFeaturnFunc;
//    radarSetPowerFunc           setPowerFunc;
//    radarSetPowerExFunc         setPowerExFunc;
//}RadarInterface;

//RadarInterface* RadarGetInterface(uint8_t index);
typedef struct
{
    radarInitialize             initFunc;
    radarCheckFeature           checkFeaturnFunc;
    radarSetPowerFunc           setPowerFunc;
    radarSetPowerStatusFunc     setPowerStatusFunc;
    //練習
    radarSetStartCalibrate      setStartCalibrate;
    radarCalibrateFunc          calibrateFunc; 
    radarReadDistValueFunc      readDistValueFunc;
    radarSetQueryVersion        setQueryVersion;
    radarReadQueryVersion       readQueryVersion;
    radarReadQueryVersionString readQueryVersionString;
    radarFirstOTA               FirstOTAFunc;
    radarOTA                    OTAFunc;
    radarRecentDistValue        RecentDistValueFunc;
    radarResult                 RadarResultFunc;
    radarResult                 RadarResultElite;
    radarCreateTask             RadarCreateTaskFunc;
    radarDeleteTask             RadarDeleteTaskFunc;
    radarRB60POTA               RadarRB60POTAFunc;
    radarFlush                  RadarFlushFunc;
    radarInitBurning            RadarinitBurningFunc;
}RadarInterface;

RadarInterface* RadarGetInterface(uint8_t index);


#ifdef __cplusplus
}
#endif

#endif //__INTERFACE_H__

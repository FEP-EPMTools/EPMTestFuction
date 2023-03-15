/**************************************************************************//**
* @file     uartinterface.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __HAL_INTERFACE_H__
#define __HAL_INTERFACE_H__

#include "nuc970.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "rtc.h"
#include "fileagent.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//************************  KEY  ************************//  
typedef BOOL(*keyCallbackFunc)(uint8_t keyId, uint8_t downUp);
    
typedef BOOL(*keyInitFunc)(void);
typedef void(*keySetCallbackFunc)(keyCallbackFunc func);
typedef void(*keySetPowerFunc)(BOOL powerFlag);
//************************  TIMER  ************************//    
typedef BOOL(*timerCallbackFunc)(uint8_t timerIndex);  


typedef BOOL(*timerInitFunc)(void);
typedef BOOL(*timerSetTimeoutFunc)(uint8_t timerIndex, TickType_t time);
typedef BOOL(*timerRunFunc)(uint8_t timerIndex);
typedef void(*timerSetCallbackFunc)(timerCallbackFunc callback);  
    
//************************  GUI  ************************//  
typedef BOOL(*guiOnDrawFunc)(uint8_t oriGuiId, uint8_t reFreshPara, int para2, int para3);
typedef BOOL(*guiUpdateDataFunc)(void);
typedef BOOL(*guiKeyCallbackFunc)(uint8_t keyId, uint8_t downUp); 
typedef BOOL(*guiTimerCallbackFunc)(uint8_t timerIndex); 
typedef BOOL(*guiPowerCallbackFunc)(uint8_t type, int flag); 
    
typedef BOOL(*guiInitFunc)(BOOL testModeFlag);
typedef void(*guiSetKeyCallbackFunc)(guiKeyCallbackFunc callback);  
typedef void(*guiSetTimerCallbackFunc)(guiTimerCallbackFunc callback); 
typedef void(*guiSetTimeoutFunc)(uint8_t timerIndex, TickType_t time);
typedef void(*guiRunTimeoutFunc)(uint8_t timerIndex);

//************************  TSReader  ************************// 
typedef void(*tsreaderDepositResultCallback)(BOOL flag, uint16_t paraValue, uint16_t infoValue);
typedef void(*tsreaderCNResultCallback)(BOOL flag, uint8_t* cn, int cnLen);

typedef BOOL(*tsreaderInitFunc)(void);
typedef BOOL(*tsreaderSetPowerFunc)(uint8_t id, BOOL flag);
typedef uint8_t(*tsreaderCheckReaderFunc)(void);
typedef BOOL(*tsreaderBreakCheckReaderFunc)(void);
typedef BOOL(*tsreaderProcessFunc)(uint16_t targetDeduct, tsreaderDepositResultCallback callback);
typedef BOOL(*tsreaderProcessCNFunc)(tsreaderCNResultCallback callback);
typedef BOOL(*tsreaderGetBootedStatus)(void);
typedef BOOL(*tsreaderSignOnProcess)(void);
typedef void(*tsreaderSaveFileFunc)(RTC_TIME_DATA_T pt, uint16_t paraValue);
typedef void(*tsreaderSaveFilePureFunc)(void);

//************************  Camera  ************************//
typedef BOOL(*cameraInitFunc)(BOOL testModeFlag);
#if(SUPPORT_HK_10_HW)
typedef BOOL(*cameraTakePhoto)(int index, uint8_t** photoPr, int* photoLen, StorageType type, char* dir, char* fileName, BOOL smallSizeFlag,int photoNum, int takeInterval);
#else
typedef BOOL(*cameraTakePhoto)(int index, uint8_t** photoPr, int* photoLen, StorageType type, char* dir, char* fileName, BOOL smallSizeFlag);
#endif
typedef void(*cameraSetPower)(int index,BOOL flag);

typedef BOOL(*cameraInitBurningFunc)(BOOL testModeFlag);



/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
//************************  KEY ************************//
typedef struct
{
    keyInitFunc             initFunc;
    keySetCallbackFunc      setCallbackFunc;
    keySetPowerFunc      setPowerFunc;
}KeyInterface;

KeyInterface* KeyGetInterface(void);
//************************  TIMER  ************************//
typedef struct
{
    timerInitFunc               initFunc;
    timerSetTimeoutFunc         setTimeoutFunc;
    timerRunFunc                runFunc;
    timerSetCallbackFunc        setCallbackFunc;
}TimerInterface;

TimerInterface* TimerGetInterface(void);

//************************  GUI  ************************//
typedef struct
{
    guiInitFunc                     initFunc;
    guiSetKeyCallbackFunc           setKeyCallbackFunc;
    guiSetTimerCallbackFunc         setTimerCallbackFunc;
    guiSetTimeoutFunc               setTimeoutFunc;
    guiRunTimeoutFunc               runTimeoutFunc;
}GuiInterface;

GuiInterface* GuiGetInterface(void);

//************************  TSReader  ************************//
#define TSREADER_HALINTERFACE_NUM          1//2

#define TSREADER_CHECK_READER_INIT              1
#define TSREADER_CHECK_READER_OK                2
#define TSREADER_CHECK_READER_ERROR             3 
#define TSREADER_CHECK_READER_BREAK             4


//#define TSREADER_TS1000_INTERFACE_INDEX     0
#define TSREADER_EPM_READER_INTERFACE_INDEX     0//1    

typedef struct
{
    tsreaderInitFunc                initFunc;
    tsreaderSetPowerFunc            setPowerFunc;
    tsreaderBreakCheckReaderFunc    breakCheckReaderFunc;
    tsreaderCheckReaderFunc         checkReaderFunc;
    tsreaderProcessFunc             processFunc;
    tsreaderProcessCNFunc           processCNFunc;
    tsreaderGetBootedStatus         getBootedStatusFunc;
    tsreaderSignOnProcess           signOnProcessFunc;
    tsreaderSaveFileFunc            saveFileFunc;
    tsreaderSaveFilePureFunc        saveFilePureFunc;
}TSReaderInterface;

TSReaderInterface* TSReaderGetInterface(uint8_t index);

//************************  Camera  ************************//
#define CAMERA_HALINTERFACE_NUM          1

//#define CAMERA_PCT08_INTERFACE_INDEX     0  
//#define CAMERA_ZM460_INTERFACE_INDEX     1  
#define CAMERA_UVC_INTERFACE_INDEX     0//2  
typedef struct
{
    cameraInitFunc                  initFunc;
    cameraTakePhoto                 takePhotoFunc; 
    cameraSetPower                  setPowerFunc;
    cameraInitBurningFunc           initBurningFunc;
}CameraInterface;

CameraInterface* CameraGetInterface(uint8_t index);


#ifdef __cplusplus
}
#endif

#endif //__HAL_INTERFACE_H__

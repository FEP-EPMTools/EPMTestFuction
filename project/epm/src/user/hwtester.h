/**************************************************************************//**
* @file     hwtester.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __HW_TESTER_H__
#define __HW_TESTER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"
#include "keydrv.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ACTION_TESTER_ITEM_NONE     0x00
#define ACTION_TESTER_ITEM_TRUE     0x01
#define ACTION_TESTER_ITEM_FALSE    0x02
	

	
#define TOUCH_UP		1
#define TOUCH_DOWN		2

	

	
/*
#define MATRIX_NUMBER_BUZZER			0
#define MATRIX_NUMBER_LED				1
#define MATRIX_NUMBER_SWITCH			2
#define MATRIX_NUMBER_BOTTON			3
#define MATRIX_NUMBER_EPD				4
#define MATRIX_NUMBER_TOUCH				5
#define MATRIX_NUMBER_MODEM				6
#define MATRIX_NUMBER_RTC				7
#define MATRIX_NUMBER_READER			8
#define MATRIX_NUMBER_FLASH				9
#define MATRIX_NUMBER_SMARTCARD		    10
#define MATRIX_NUMBER_CAMERA			11
#define MATRIX_NUMBER_PROWAVE			12
#define MATRIX_NUMBER_BATTERY			13
*/
typedef BOOL(*HWTesterFunc)(void* para1, void* para2);

typedef struct
{
    char                        charItem;
    char*                       itemName;
    HWTesterFunc                testerFunc;
}HWTesterItem;
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL HWTesterInit(void);
BOOL MTPTesterInit(void);

BOOL CallBackReturnValue(uint8_t keyId, uint8_t downUp);
void SetGuiResponseVal(char guiresponse);
BOOL GetTesterFlag(void);
/*
BOOL GetTesterResult(void);
BOOL GetEPDPrintFlag(void);
char* GetEPDString(void);
uint32_t GetCardID(void);
BOOL GetRefresh(void);*/

int GetDeviceIDString(void);
BOOL SetDeviceIDString(void* para1,void* para2);

void hwtestEDPflashBurn(void);

void hwtestEDPflashBurnLite(void);

BOOL readMBtestFunc(void);
BOOL readAssemblyTestFunc(void);

void HwTestReceieveU32Func(UINT32 tempVal);

#if (ENABLE_BURNIN_TESTER)
BOOL LEDColorBuffSet(UINT16 GreenPinBite, UINT16 RedPinBite);
BOOL LEDBoardLightSet(void);
#endif

void QueryCADtimeoutFunc(BOOL timeoutFlag);

BOOL QueryNTPfun(void);

#ifdef __cplusplus
}
#endif

#endif //__HW_TESTER_H__

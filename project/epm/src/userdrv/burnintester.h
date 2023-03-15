/**************************************************************************//**
* @file     burnintester.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2020 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __BURNIN_TESTER_H__
#define __BURNIN_TESTER_H__

#include "nuc970.h"
#include "rtc.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

#define BURNIN_TEST_VERSION                 "1.00.41"
#define BURNIN_TEST_REPORT_BUFFER_LENGTH    4096
#define BURNIN_LOG_FILENAME_BUFFER_LENGTH   64

#define BURNIN_RTC_INTERVAL                 2000    //mili-second

#define BURNIN_LED_BUZZER_INTERVAL          12      //second
#define BURNIN_LED_GAP_TIME                 500     //mili-second
#define BURNIN_BUZZER_GAP_TIME              400     //mili-second
#define BURNIN_BUZZER_PLAY                  3
#define BURNIN_BUZZER_PERIOD                100     //mili-second

#define BURNIN_EPD_BACKLIGHT_INTERVAL       45      //second
#define BURNIN_EPD_REFRESH_INTERVAL         45      //second

#define BURNIN_SPACE_EX_INTERVAL            30      //second
#define BURNIN_SPACE_EX_GAP_TIME            800     //mili-second
#define BURNIN_SPACE_EX_COUNT_PER_LOOP      100

#define BURNIN_CAMERA_INTERVAL              20      //second
#define UVCAMERA_NUM                        2
#define UVCAMERA_INDEX_0                    0
#define UVCAMERA_INDEX_1                    1

#define BURNIN_NAND_FLASH_INTERVAL          15      //second
#define NAND_FLASH_NUM                      3
#define FLASH_INDEX_0                       0
#define FLASH_INDEX_1                       1
#define FLASH_INDEX_2                       2

#define BURNIN_BATTERY_INTERVAL             5       //second
#define BURNIN_BATTERY_BOUNDARY             660     //voltage(scale 1/100)
#define BATTERY_NUM                         3
#define BATTERY_INDEX_0                     0
#define BATTERY_INDEX_1                     1
#define BATTERY_INDEX_SOLAR                 2

#define BURNIN_NT066E_INTERVAL              15      //second
#define BURNIN_SMARTCARD_INTERVAL           10      //second
#define BURNIN_CARD_READER_INTERVAL         30      //second
#define BURNIN_CREDIT_READER_INTERVAL       20      //second

#define BURNIN_MODEM_AT_INTERVAL            5       //second
#define BURNIN_MODEM_FTP_INTERVAL           300     //second
//#define BURNIN_SD_CARD_INTERVAL             297     //second
#define BURNIN_ERROR_LOG_INTERVAL           180     //second

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/

BOOL BurninTesterInit(void);
BOOL EnabledBurninTestMode(void);
char* BuildBurninTestReport(RTC_TIME_DATA_T *pt);
void GetBurninTestRuntime(uint32_t *hours, uint32_t *minutes, uint32_t *seconds);
uint32_t GetLedBurninTestCounter(void);
uint32_t GetLedBurninTestErrorCounter(void);
uint32_t GetBatteryBurninTestCounter(int index);
uint32_t GetBatteryBurninTestErrorCounter(int index);
uint32_t GetNandFlashBurninTestCounter(int index);
uint32_t GetNandFlashBurninTestErrorCounter(int index);
uint32_t GetBatteryLastADCValue(int index);
uint32_t GetDeviceID(void);

void BurninTestingMonitor(void);
BOOL GetPrepareStopBurninFlag(void);
void NoticeFTPReportDone(void);
void NoticeSDReportDone(void);
BOOL GetBurninTerminatedFlag(void);
void ResetRuntimefun(void);
BOOL CalibRTCviaNTP(void);

#ifdef __cplusplus
}
#endif

#endif //__BURNIN_TESTER_H__

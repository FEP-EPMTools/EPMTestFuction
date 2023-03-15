/**************************************************************************//**
* @file     cardreader.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __CARD_READER_H__
#define __CARD_READER_H__

#include "nuc970.h"
#include "rtc.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "halinterface.h"
#include "epmreader.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL CardReaderInit(BOOL testModeFlag);
int CardReaderGetBootedStatus(void);
//void CardReaderStartInit(void);
//void CardReaderStopInit(void);
void CardReaderSetPower(uint8_t id, BOOL flag);
BOOL CardReaderProcess(uint16_t targetDeduct, tsreaderDepositResultCallback callback);
BOOL CardReaderProcessCN(tsreaderCNResultCallback callback);
BOOL CardReaderSignOnProcess(void);
void CardReaderSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue);
void CardReaderEnterCS(void);
void CardReaderExitCS(void);
#ifdef __cplusplus
}
#endif

#endif //__CARD_READER_H__

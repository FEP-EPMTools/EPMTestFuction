/**************************************************************************//**
* @file     octopusreader.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef _OCTOPUS_READER
#define _OCTOPUS_READER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _PC_ENV_
    #include "basetype.h"
#else
    #include "nuc970.h"
#endif

#include "halinterface.h"

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
TSReaderInterface* OctopusReaderGetInterface(void);
BOOL OctopusReaderSetAIValue(uint8_t AI1, uint8_t AI2);
BOOL OctopusReaderInit(void);
BOOL OctopusReaderSetPower(uint8_t id, BOOL flag);
uint8_t OctopusReaderCheckReader(void);
BOOL OctopusReaderBreakCheckReader(void);

BOOL OctopusReaderProcess(uint16_t targetDeduct, tsreaderDepositResultCallback callback);
BOOL OctopusReaderGetBootedStatus(void);
BOOL OctopusReaderSignOnProcess(void);
void OctopusReaderSaveFile(RTC_TIME_DATA_T pt, uint16_t paraValue);
void OctopusReaderSaveFilePure(void);
    
INT32 OctopusReaderWrite(PUINT8 pucBuf, UINT32 uLen);
INT32 OctopusReaderRead(PUINT8 pucBuf, UINT32 uLen);
void OctopusReaderFlushBuffer(void);
BOOL OctopusExportXFile(void);
#ifdef __cplusplus
}
#endif

#endif //_OCTOPUS_READER

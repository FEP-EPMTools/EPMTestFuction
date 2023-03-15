/**************************************************************************//**
* @file     spaceexdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SPACE_EX_DRV_H__
#define __SPACE_EX_DRV_H__

#include "nuc970.h"
//#include "radardrv.h"
#include "radardrv.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define SPACE_EX_INDEX_1        0x0
#define SPACE_EX_INDEX_2        0x1
    
#ifndef SPACE_INDEX_1
    #define SPACE_INDEX_1 SPACE_EX_INDEX_1
#endif
#ifndef SPACE_INDEX_2
    #define SPACE_INDEX_2 SPACE_EX_INDEX_2
#endif

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL SpaceExDrvInit(BOOL testModeFlag);
TickType_t SpaceExGetFeature(uint8_t index, BOOL* changeFlag);
int SpaceExGetCurrentCarId(uint8_t index);    
void SpaceExSetCurrentCarId(uint8_t index, int value);


BOOL SpaceExStart(int spaceIndex);
void SpaceExSemaphoreTake(int index);

void SpaceExSemaphoreGive(int index);
void SpaceExSetSampleInterval(int time);
void SpaceExSetUnstableInterval(int time);

void SpaceExSemaphoreTotalTake(void);
void SpaceExSemaphoreTotalGive(void);

#ifdef __cplusplus
}
#endif

#endif //__SPACE_EX_DRV_H__

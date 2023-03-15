/**************************************************************************//**
* @file     spacedrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __SPACE_DRV_H__
#define __SPACE_DRV_H__

#include "nuc970.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define SPACE_INDEX_1        0x0
#define SPACE_INDEX_2       0x1

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL SpaceDrvInit(BOOL testModeFlag);
BOOL GetSpaceStatus(uint8_t index);
void SetSpaceStatus(uint8_t index, BOOL status);
void StartSpaceDrv(void);
int GetSpaceDist(uint8_t index);
#ifdef __cplusplus
}
#endif

#endif //__SPACE_DRV_H__

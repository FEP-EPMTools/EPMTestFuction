/**************************************************************************//**
* @file     photoagent.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __PHOTO_AGENT_H__
#define __PHOTO_AGENT_H__
#include <time.h>
#include "nuc970.h"

#include "paralib.h"

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
BOOL PhotoAgentInit(BOOL testModeFlag);
void PhotoAgentStartTakePhoto(uint32_t currentTime);
#if (ENABLE_BURNIN_TESTER)
BOOL UVCameraTestInit(BOOL testModeFlag);
uint32_t GetCameraBurninTestCounter(int cameraIndex);
uint32_t GetCameraBurninPhotoErrorCounter(int cameraIndex);
uint32_t GetCameraBurninFileErrorCounter(int cameraIndex);
#endif
#ifdef __cplusplus
}
#endif

#endif //__PHOTO_AGENT_H__

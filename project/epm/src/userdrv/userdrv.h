/**************************************************************************//**
* @file     userdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __USER_DRV_H__
#define __USER_DRV_H__

#include "nuc970.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
typedef BOOL(*userDrvInitFunction)(BOOL testModeFlag);

typedef struct
{
    char*                   drvName;
    userDrvInitFunction     func;    
}userDrvInitFunctionList;

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL UserDrvInit(BOOL testModeFlag);

#ifdef __cplusplus
}
#endif

#endif //__USER_DRV_H__

/**************************************************************************//**
* @file     user.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __USER_H__
#define __USER_H__

#include "nuc970.h"
#include "guidrv.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
typedef BOOL(*userInitFunction)(void);

typedef struct
{
    char*                drvName;
    userInitFunction     func;    
}userInitFunctionList;

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL UserInit(BOOL testModeFlag);

#ifdef __cplusplus
}
#endif

#endif //__USER_H__

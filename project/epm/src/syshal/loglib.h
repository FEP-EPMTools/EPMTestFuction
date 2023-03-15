/**************************************************************************//**
* @file     loglib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __LOG_LIB_H__
#define __LOG_LIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"


#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
    


    
typedef enum{
    LOG_TYPE_INFO               = 0x00,
    LOG_TYPE_WARNING            = 0x01,
    LOG_TYPE_ERROR              = 0x02
}LogType;

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL LoglibInit(void);
void  LoglibPrintf(LogType type, char* logData);
void  LoglibPrintfEx(LogType type, char* logData, BOOL flag);
char*  LoglibGetCurrentLogFileName(void);
#ifdef __cplusplus
}
#endif

#endif //__LOG_LIB_H__

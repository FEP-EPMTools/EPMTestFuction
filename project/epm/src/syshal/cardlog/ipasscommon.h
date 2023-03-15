/**************************************************************************//**
* @file     ipassdpti.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __IPASS_COMMON_H__
#define __IPASS_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _PC_ENV_
    #include "misc.h"
#else
    #include "nuc970.h"
    #include "sys.h"
#endif
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/    
#pragma pack(1)
typedef struct
{
    uint8_t  value[8];
}IPassDate; 

typedef struct
{
    uint8_t  value[6];
}IPassTime; 

typedef struct
{
    uint8_t  value[2];
}IPassSeparator; 
#pragma pack()
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif //__IPASS_COMMON_H__

/**************************************************************************//**
* @file     blkcommon.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __BLK_COMMON_H__
#define __BLK_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _PC_ENV_
    #include "misc.h"
#else
    #include "nuc970.h"
    #include "sys.h"
#endif

#include "ipassblk.h"
#include "eccblk.h"

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
BOOL BlkCommonInit(void);
uint32_t BlkHexStr2Dec(uint8_t* str, int size);    
    
#ifdef __cplusplus
}
#endif

#endif //__BLK_COMMON_H__

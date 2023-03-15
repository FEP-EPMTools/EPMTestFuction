/**************************************************************************//**
* @file     yaffs2drv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __YAFFS2_DRV_H__
#define __YAFFS2_DRV_H__

#include "nuc970.h"

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
BOOL Yaffs2DrvInit(void);
void Yaffs2ListFileEx (char* dir);
const char *Yaffs2ErrorStr(void);
#ifdef __cplusplus
}
#endif

#endif //__YAFFS2_DRV_H__

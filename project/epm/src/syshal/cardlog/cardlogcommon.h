/**************************************************************************//**
* @file     cardlogcommon.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __CARD_LOG_COMMON_H__
#define __CARD_LOG_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _PC_ENV_
#else
    #include "nuc970.h"
    #include "sys.h"
#endif

//#include "ipassdfti.h"
#include "ipassdpti.h"
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
void CardLogUint32ToString(char* debugStr, uint32_t value, uint8_t* str, int strSize);
void CardLogUint32ToHexString(char* debugStr, uint32_t value, uint8_t* str, int strSize);  
void CardLogCSNToHexString(char* debugStr, uint8_t* value, uint8_t* str);  
void CardLogUint32ToDosTime(char* debugStr, uint32_t value, uint8_t* str, int strSize);    
uint32_t CardLogBuffToUint32(uint8_t* buff, int buffSize);  
uint32_t CardLogHexBuffToUint32(uint8_t* buff, int buffSize);  
void CardLogFillSeparator(IPassSeparator* separator);
void CardLogFillSpace(uint8_t* data, int size);   
#ifdef __cplusplus
}
#endif

#endif //__CARD_LOG_COMMON_H__

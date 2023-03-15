/**************************************************************************//**
* @file     ipassblk.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __IPASS_BLK_H__
#define __IPASS_BLK_H__

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
#define IPASS_BLK_ITEM_BYTE_LEN    8
    
#define IPASS_BLK_FILE_SAVE_POSITION          FILE_AGENT_STORAGE_TYPE_AUTO
#define IPASS_BLK_FILE_EXTENSION              "dat"
#define IPASS_BLK_FILE_DIR                    "1:"

#pragma pack(1)
typedef struct
{
    //ex: 00000016\r\n
    uint8_t     str[9];
    uint32_t    value;
}IPassBLKItem; 
#pragma pack()

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
//BOOL IPASSBLKLoad(uint8_t* data, int size, IPassBLKItem** pItem, int* itemNum);
BOOL IPASSBLKLoadFileFromSD(char* fileName);
int IPASSBLKSearchTargetID(uint32_t target);
BOOL IPASSBLKLoadAllFile(void);
int IPASSBLKSearchTargetIDByString(uint8_t* str, int size);
#ifdef __cplusplus
}
#endif

#endif //__IPASS_BLK_H__

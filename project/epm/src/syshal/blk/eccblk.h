/**************************************************************************//**
* @file     eccblk.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __ECC_BLK_H__
#define __ECC_BLK_H__

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
#define ECC_BLK_ITEM_BYTE_LEN    9//8
#define ECC_BLK_ITEM_ID_BYTE_LEN   8
    
#define ECC_BLK_FILE_SAVE_POSITION          FILE_AGENT_STORAGE_TYPE_AUTO
#define ECC_BLK_FILE_EXTENSION              "BA2"
#define ECC_BLK_FILE_DIR                    "1:"

#pragma pack(1)
typedef struct
{
    //ex: 00000016\r\n
    uint8_t     str[16+1];
    uint64_t    value;
    uint8_t    lockFlag;
}EccBLKItem; 
#pragma pack()

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
//BOOL ECCBLKLoad(uint8_t* data, int size, IPassBLKItem** pItem, int* itemNum);
BOOL ECCBLKLoadFileFromSD(char* fileName);
int ECCBLKSearchTargetID(uint64_t target, char** blkFileName);
BOOL ECCBLKLoadAllFile(void);
int ECCBLKSearchTargetIDByArray(uint8_t* idArray, int idsize, char** blkFileName);
#ifdef __cplusplus
}
#endif

#endif //__ECC_BLK_H__

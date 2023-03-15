/**************************************************************************//**
* @file     blkcommon.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _PC_ENV_
    #include "misc.h"
    #include "interface.h"
    #include "halinterface.h"
    #include "blkcommon.h"
    #define sysprintf       printf
#else
    #include "nuc970.h"
    #include "sys.h"

    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "fepconfig.h"
    #include "blkcommon.h"
    #include "fileagent.h"
    #include "fatfslib.h"
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL BlkCommonInit(void)
{
    sysprintf("BlkCommonInit!!\n");
//    IPASSBLKLoadAllFile();
//    ECCBLKLoadAllFile();
    return TRUE;
}

#define TO_DEX_LEN   4
uint32_t BlkHexStr2Dec(uint8_t* str, int size)
{
    int decValue, decValue2;
    uint8_t strTmp[TO_DEX_LEN + 1];
    uint8_t strTmp2[TO_DEX_LEN + 1];
    uint32_t  reVal = 0;
    memcpy(strTmp, str, TO_DEX_LEN);
    strTmp[TO_DEX_LEN] = 0x0;
    decValue = strtol((char*)strTmp,NULL,16);
    memcpy(strTmp2, str + TO_DEX_LEN, TO_DEX_LEN);
    strTmp2[TO_DEX_LEN] = 0x0;
    decValue2 = strtol((char*)strTmp2,NULL,16);    
    reVal = decValue<<(TO_DEX_LEN*4) | decValue2;     
    //sysprintf("HexStr2Dec:[%s:%s], [%d, %d] ==> %d, 0x%08x\r\n", strTmp, strTmp2, decValue, decValue2, reVal, reVal);
    return reVal;
}

uint32_t BlkHex2HexStr(uint8_t* str, int size)
{
    int decValue, decValue2;
    uint8_t strTmp[TO_DEX_LEN + 1];
    uint8_t strTmp2[TO_DEX_LEN + 1];
    uint32_t  reVal = 0;
    memcpy(strTmp, str, TO_DEX_LEN);
    strTmp[TO_DEX_LEN] = 0x0;
    decValue = strtol((char*)strTmp,NULL,16);
    memcpy(strTmp2, str + TO_DEX_LEN, TO_DEX_LEN);
    strTmp2[TO_DEX_LEN] = 0x0;
    decValue2 = strtol((char*)strTmp2,NULL,16);    
    reVal = decValue<<(TO_DEX_LEN*4) | decValue2;     
    //sysprintf("HexStr2Dec:[%s:%s], [%d, %d] ==> %d, 0x%08x\r\n", strTmp, strTmp2, decValue, decValue2, reVal, reVal);
    return reVal;
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


/**************************************************************************//**
* @file     cardlogcommon.c
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
    #include "cardlogcommon.h"
    #define     sysprintf       printf
#else
    #include "nuc970.h"
    #include "sys.h"

    /* Scheduler includes. */
    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    #include "semphr.h"

    #include "fepconfig.h"
    #include "cardlogcommon.h"
    #include "fileagent.h"
    #include "fatfslib.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define ENABLE_CARDLOG_DEBUG   0
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static IPassSeparator separatorData = {0x0d, 0x0a};
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
void CardLogCSNToHexString(char* debugStr, uint8_t* value, uint8_t* str)
{    
    //最多32個字元
    int i;
    #if(ENABLE_CARDLOG_DEBUG)
    sysprintf("CardLogUint32ToHexString [%s](strSize = %d):[%d, %X]\r\n", debugStr, strSize, value, value);  
    #endif    
    for(i = 0; i<4; i++)
    {
        uint32_t uint32Value = CardLogHexBuffToUint32(&value[i*4], 4);
        CardLogUint32ToHexString("uint32Value", uint32Value, str + i*8, 8);
    }
}

uint32_t CardLogBuffToUint32(uint8_t* buff, int buffSize)
{    
    uint32_t value = 0;
    for(int i = 0; i<buffSize; i++)
    {
        value = value + (buff[i]<<(8*i));
    }
    return value;
}

#define TRANSFER_MAX_LEN  8
void CardLogUint32ToString(char* debugStr, uint32_t value, uint8_t* str, int strSize)
{    
    //最多8個字元
    char tmp[TRANSFER_MAX_LEN+1];
    #if(ENABLE_CARDLOG_DEBUG)
    sysprintf("CardLogUint32ToString [%s](strSize = %d):[%d, %X]\r\n", debugStr, strSize, value, value);  
    #endif    
    sprintf(tmp, "%08d", value);   
    //sysprintf("[%d:%c, %c (%s)]\r\n", TRANSFER_MAX_LEN-strSize, tmp[TRANSFER_MAX_LEN-strSize], tmp[TRANSFER_MAX_LEN-strSize + 1], tmp); 
    memcpy(str, &tmp[TRANSFER_MAX_LEN-strSize], strSize*sizeof(char));//只有cpy 最後的幾格字元
    
}

void CardLogUint32ToHexString(char* debugStr, uint32_t value, uint8_t* str, int strSize)
{    
    //最多8個字元
    char tmp[TRANSFER_MAX_LEN+1];
    #if(ENABLE_CARDLOG_DEBUG)
    sysprintf("CardLogUint32ToHexString [%s](strSize = %d):[%d, %X]\r\n", debugStr, strSize, value, value);  
    #endif    
    sprintf(tmp, "%08X", value);  
    //sysprintf("[%d:%c, %c(%s)]\r\n", TRANSFER_MAX_LEN-strSize, tmp[TRANSFER_MAX_LEN-strSize], tmp[TRANSFER_MAX_LEN-strSize + 1], tmp);    
    memcpy(str, &tmp[TRANSFER_MAX_LEN-strSize], strSize*sizeof(char));//只有cpy 最後的幾格字元
}
void CardLogUint32ToDosTime(char* debugStr, uint32_t value, uint8_t* str, int strSize)
{    
    //最多8個字元
    char tmp[TRANSFER_MAX_LEN+1];
    #if(ENABLE_CARDLOG_DEBUG)
    sysprintf("CardLogUint32ToDosTime [%s](strSize = %d):[%d, %X]\r\n", debugStr, strSize, value, value);  
    #endif    
    sprintf(tmp, "%04d%02d%02d", (value>>9)&0x7f + 1980, (value>>5)&0xf, value&0x1f);  
    //sysprintf("[%d:%c, %c(%s)]\r\n", TRANSFER_MAX_LEN-strSize, tmp[TRANSFER_MAX_LEN-strSize], tmp[TRANSFER_MAX_LEN-strSize + 1], tmp);    
    memcpy(str, &tmp[TRANSFER_MAX_LEN-strSize], strSize*sizeof(char));//只有cpy 最後的幾格字元
}


uint32_t CardLogHexBuffToUint32(uint8_t* buff, int buffSize)
{    
    uint32_t value = 0;
    for(int i = 0; i<buffSize; i++)
    {
        value = value | (buff[(buffSize - 1)- i]<<(8*i));
    }
    return value;
}

void CardLogFillSeparator(IPassSeparator* separator)
{
    memcpy(separator, &separatorData, sizeof(IPassSeparator));
}
void CardLogFillSpace(uint8_t* data, int size)
{
    sysprintf("fillSpace(%d)\r\n", size);
    memset(data, ' ', size);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


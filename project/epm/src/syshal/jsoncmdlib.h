/**************************************************************************//**
* @file     jsoncmdlib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __JSON_CMD_LIB_H__
#define __JSON_CMD_LIB_H__


#ifdef __WINDOWS__
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "minwindef.h"
    #define sysprintf printf
#else
    #include "nuc970.h"
    #define  printf sysprintf
#endif

#include "cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define JSON_CMD_VER 1

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL JsonCmdLibInit(void);
cJSON* JsonCmdCreateTransactionStatusData(int bayid, int time, int cost, int balance);
cJSON* JsonCmdCreateExpiredStatusData(int bayid);
char* JsonCmdCreateStatusData(int jsonver, char* epmver, char* flag, char* epmid, int time, int index, BOOL bay1, BOOL bay2, 
                                int baydist1, int baydist2, int voltage1, int voltage2, 
                                int deposit1, int deposit2, int deposit3, int deposit4, int deposit5, int deposit6,
                                char* tariff, char* setting);

#ifdef __cplusplus
}
#endif

#endif //__JSON_CMD_LIB_H__

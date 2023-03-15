/**************************************************************************//**
* @file     fwremoteota.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2020 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __FWREMOTE_OTA_H__
#define __FWREMOTE_OTA_H__

#include "nuc970.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
    
#define FTP_FW_REMOTE_OTA_PATH    "firmware/testOTA/" //"test/Steven/"     
    
/*
typedef BOOL(*userDrvInitFunction)(BOOL testModeFlag);

typedef struct
{
    char*                   drvName;
    userDrvInitFunction     func;    
}userDrvInitFunctionList;
*/
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
//BOOL UserDrvInit(BOOL testModeFlag);
    
void callFileContent(char* Str,uint32_t Len);
BOOL FWremoteOTAFunc(void);

#ifdef __cplusplus
}
#endif

#endif //__FWREMOTE_OTA_H__

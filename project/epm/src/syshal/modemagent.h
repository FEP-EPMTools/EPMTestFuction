/**************************************************************************//**
* @file     modemagent.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __MODEM_AGENT_H__
#define __MODEM_AGENT_H__
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "basetype.h"
#include "dataprocesslib.h"
#ifdef __cplusplus
extern "C" { 
#endif
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/    
    

BOOL ModemAgentInit(BOOL testModeFlag);
BOOL ModemAgentStartSend(DataProcessId id);
//BOOL ModemAgentStopSend(void);
    
#if (ENABLE_BURNIN_TESTER)
uint32_t GetModemATBurninTestCounter(void);
uint32_t GetModemATBurninTestErrorCounter(void);
uint32_t GetModemFTPBurninTestCounter(void);
uint32_t GetModemFTPBurninTestErrorCounter(void);
uint32_t GetModemDialupBurninTestErrorCounter(void);
uint32_t GetModemFileBurninTestErrorCounter(void);
uint32_t GetSDCardBurninTestCounter(void);
uint32_t GetSDCardBurninTestErrorCounter(void);
BOOL GetModemFTPRunningStatus(void);
#endif    
    
    
    
#ifdef __cplusplus
}
#endif

#endif /* __MODEM_AGENT_H__ */


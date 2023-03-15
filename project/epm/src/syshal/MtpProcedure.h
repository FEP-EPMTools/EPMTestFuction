/**************************************************************************//**
* @file     MtpProcedure.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2019 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __MTP_PROCEDURE_H__
#define __MTP_PROCEDURE_H__

#include "nuc970.h"
#include "halinterface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

#define MTP_LIBRARY_VERSION         "1.00.02"

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/

BOOL MTP_ProcedureInit(void);
BOOL MTP_IsExitProcedure(void);
void MTP_ProcedureDeInit(void);
void MTP_WaitingStartMessage(void);
BOOL MTP_GetProcedureFlag(void);
BOOL MTP_GetSwitchRTCFlag(void);

//===================== for CRCTool =====================
void CRCTool_Init(int CodingType);
uint32_t CRCTool_TableFast(uint8_t *p, int p_len);
uint32_t CRCTool_Table(uint8_t *p, int p_len);
//===================== for CRCTool =====================

#ifdef __cplusplus
}
#endif

#endif //__MTP_PROCEDURE_H__

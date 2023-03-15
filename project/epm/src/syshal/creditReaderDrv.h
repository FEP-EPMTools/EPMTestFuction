/**************************************************************************//**
* @file     creditReaderDrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __CREDIT_CARD_READER_DRV_H__
#define __CREDIT_CARD_READER_DRV_H__

#include "nuc970.h"
#include "halinterface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define DECODER_S0_Start                        0
#define DECODER_S1_SOH                          1
#define DECODER_S2_STX                          2
#define DECODER_S3_EOT                          3
#define DECODER_S4_SOHsequenceNumber            4
#define DECODER_S5_NumberOfFrames1              5
#define DECODER_S6_STXsequenceNumber            6
#define DECODER_S7_APDU                         7
#define DECODER_S8_IsDLE                        8
#define DECODER_S9_IsETX                        9
#define DECODER_S10_AddToBuffer                 10
#define DECODER_S11_EndOfFrame                  11
#define DECODER_S12_CRC1                        12
#define DECODER_S13_CRC2                        13
#define DECODER_S14_IsCrcOK                     14
#define DECODER_S15_NACKout                     15
#define DECODER_S16_EOTsequenceNumber           16
#define DECODER_S17_ACKout                      17
#define DECODER_S18_IsComplete                  18
#define DECODER_S19_APDUmessage                 19
#define DECODER_S20_IsSTXorSOH                  20
#define DECODER_S21_ACKin                       21
#define DECODER_S22_ACKsequenceNumber           22
#define DECODER_S23_ACKmessage                  23
#define DECODER_S24_NACKin                      24
#define DECODER_S25_NACKsequenceNumber          25
#define DECODER_S26_NACKmessage                 26
#define DECODER_S27_NumberOfFrames2             27
#define DECODER_S28_Finished                    28

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL CreditReaderDrvInit(BOOL testModeFlag);
void CreditCardFlushTxRx(void);
void CreditCardSetPower(BOOL flag);
INT32 CreditCardRead(PUINT8 pucBuf, UINT32 uLen);
INT32 CreditCardWrite(PUINT8 pucBuf, UINT32 uLen);

#if (ENABLE_BURNIN_TESTER)
uint32_t GetCreditReaderBurninTestCounter(void);
uint32_t GetCreditReaderBurninTestErrorCounter(void);
#endif

void CADReadCardinit(uint8_t* pucBuf);
BOOL CADReadCard(uint8_t* pucBuf,uint8_t* currentSequenceNum);
BOOL DetectCADConnect(void);
void CADReadCardpoweron(void);
void CADReadCardpoweroff(void);


#ifdef __cplusplus
}
#endif

#endif //__CREDIT_CARD_READER_DRV_H__

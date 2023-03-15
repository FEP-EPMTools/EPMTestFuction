/*****************************************************************************/
/* File Name   : rwl.h                                                       */
/* Author      : Copyright OCL                                        			 */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : header file for rwl.c																			 */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/
#ifndef RWL_H
#define RWL_H

#include <stdio.h>
#include "rwltype.h"
#include "csrw.h"

typedef struct {
	UINT  DevID;      	/* Device ID */
	UINT  OperID;   	  /* Operator ID */
	UINT  DevTime;      /* Device Time */
	UINT  CompID;     	/* Company ID */
	UINT  KeyVer;     	/* Key Version */
	UINT  EODVer;   	  /* EOD Version */
	UINT  BLVer;  	    /* Blacklist Version */
	UINT  FIRMVer;      /* Firmware Version */
	UINT  CCHSVer;      /* CCHS MSG ID */
	UINT  CSSer;        /* CS Serial #, Loc ID */
	UINT  IntBLVer;     /* Interim Blacklist Version */
	UINT  FuncBLVer;  	/* Functional Blacklist Version */
	UINT  AlertMsgVer;	/* alert msg version */
	UINT	RWKeyVer;			/* rw key version */
	UINT  OTPVer;				/* OTP version */
	BYTE	Reserved[20];	/* reserved */
} stDevVer;

extern INT InitComm(BYTE sPort, INT sBaud);
extern INT Authenticate(void);
extern INT EndSession(void);
extern INT AntennaOff(void);
extern INT XFileInitRecv(CHAR *, UINT);
extern INT XFileContRecv(CHAR *, UINT);
extern INT TimeVer(BYTE *, UINT *);
extern INT PollDeduct(BYTE bTimeout, INT TxnAmt, const BYTE *AddInfo, BYTE bAlertMsgFmt, Rsp_PollDeductStl *cardInfo);
extern INT HouseKeepingInitTran(void);
extern INT HouseKeepingContTran(void);
extern INT WriteID(UINT CSID);
#endif // RWL_H



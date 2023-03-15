/*****************************************************************************/
/* File Name   : csrw.h			                                                 */
/* Author      : Copyright OCL                                        			 */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : header file for csrwtest.c																	 */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/

#ifndef __CSRW_H__
#define __CSRW_H__

#ifndef __CSRWTEST__
#include <stdio.h>
#include <string.h>
#include "rwltype.h"
#include "rwl_common.h"
#endif

#include "nuc970.h"

//#ifdef __cplusplus
//extern "C"
//{
//#endif

//----------------------------------------------------------------------
// Generic defines used by application
//----------------------------------------------------------------------
void CmdPollDeduct(BYTE bTimeout, INT TxnAmt, const BYTE *ai, BYTE bAlertMsgFmt);
void CmdAntennaOff(void);
void CmdEndSession(void);
void CmdTimeVer(UINT * u32_time);
void CmdMetaFileInitSend(CHAR* inFile, CHAR* inDir, int inFileLen);
void CmdMetaFileContSend(void);
void CmdXFileInitRecv(CHAR* outFile, UINT u32_SegmentSize);
void CmdXFileContRecv(CHAR* outFile, UINT u32_SegmentSize);
void CmdWriteID(UINT ID);
INT CmdAuthenticate(BYTE *u8_stage);
ULONG GetRTC2000(void);

/*=========================================================================*/
/*                      FEATURES COMPILATION SWITCHES                      */
/*=========================================================================*/
/* list of available commands */
#define CSCMD_AUTH1						0x04	/* authentication 1 */
#define CSCMD_AUTH2						0x06	/* authentication 2 */
#define	CSCMD_WRITEID					0x36	/* Write ID */
#define CSCMD_TIMEVER					0x46	/* Time sync and version */
#define	CSCMD_INITTRAN				0x5A	/* Metafile file Init transfer */
#define	CSCMD_CONTTRAN				0x5C	/* Metafile file Content transfer */
#define CSCMD_END_SESSION			0x5E	/* End Session */
#define CSCMD_ANTENNA_CTRL		0x6E	/* Antenna Off*/
#define CSCMD_POLLDEDUCT_STL	0x70	/* Polldeduct*/
#define CSCMD_INITRECV				0x48	/* XFile Init Receive */
#define CSCMD_CONTRECV				0x7C	/* XFile Content receive */

/* error codes */
#define ERR_NOERR							0x00	/* no error */
#define ERR_ERRRESP						0x01	/* error */
#define ERR_RESP							0x7f	/* response code when invalid command */
#define ERR_INVPARAM					0x03	/* invalid parameters */
#define ERR_CSCREAD						0x10	/* fail to read blocks from CSC */
#define ERR_CSCBLOCK					0x13	/* CSC is blocked */
#define ERR_CSCREJECT					0x15	/* CSC cannot add value */
#define ERR_UNCONF						0x19	/* CSC unconfirm transaction */
#define ERR_POLLTIME					0x20	/* time out without finding a CSC */
#define ERR_NOFUND						0x30	/* insufficient fund in CSC */
#define ERR_UNCONF_DIFFCARD		0x38	/* different card in unconfirm retry */

extern void sendNAK(void);

#pragma pack (push, 1)
typedef struct {
  BYTE 	f_rspCode;
  BYTE 	f_err;
  BYTE 	pollStat; /* Poll result or status, this byte will set to 1 to indicate Card ID and language data is valid */
  BYTE 	CardNo[10]; /* card no. of the Octopus card/product in ASCII (padded with leading zeros to 8 bytes)*/
  BYTE 	language; /* The preferred language of the customer set in the Octopus card/product */
  INT 	beforeRV;/* remaining value before deduction in 10 cents */
  INT 	afterRV; /* remaining value after deduction in 10 cents */
  BYTE 	alertTone; /* Flag for generating alert tone */
  BYTE 	alertMsg; /* Flag for displaying alert message */
  CHAR engAlertMsg[100]; /* English alert message */
  CHAR chiAlertMsg[200]; /* Chinese alert message */
    //add by sam
  BYTE 	octopusType; /* octopus Type */
  BYTE 	Reserved[19]; /* Reserved for future use */
} Rsp_PollDeductStl;

typedef struct {
	BYTE f_cmdCode;
	BYTE f_res;
	BYTE f_fileType;
	BYTE f_verCtrl;
	BYTE f_fileAct;
	BYTE f_name[32];
	ULONG f_len;
	USHORT f_segSize;
	USHORT f_segRemain;
} Cmd_InitTran;

typedef struct {
	BYTE f_rspCode;
	BYTE f_err;
	USHORT f_fileID;
	USHORT f_segSize;
} Rsp_InitTran;

typedef struct {
	BYTE f_cmdCode;
	BYTE f_res;
	USHORT f_fileID;
	ULONG f_offset;
	USHORT f_len;
	BYTE f_data[MAX_SEGMENT_LEN];
} Cmd_ContentTran;

typedef struct {
	BYTE f_rspCode;
	BYTE f_err;
	USHORT f_byteWrite;
	ULONG f_nextOff;
} Rsp_ContentTran;

typedef struct {
	BYTE	f_cmdCode;
	BYTE	f_res;
	BYTE	f_fileType;
	USHORT	f_segSize;
} Cmd_InitReceive;

typedef struct {
	BYTE	f_rspCode;
	BYTE	f_err;
	BYTE	f_name[32];
	ULONG	f_len;
	USHORT	f_fileID;
} Rsp_InitReceive;

typedef struct {
	BYTE	f_cmdCode;
	BYTE	f_res;
	USHORT	f_fileID;
	ULONG	f_offset;
	USHORT	f_len;
} Cmd_ContentReceive;

typedef struct {
	BYTE	f_rspCode;
	BYTE	f_err;
	USHORT	f_byteRead;
	ULONG	f_nextOff;
	BYTE	f_data[MAX_SEGMENT_LEN];
} Rsp_ContentReceive;
#pragma pack (pop)

//#ifdef __cplusplus
//}
//#endif

#endif // __CSRW_H__



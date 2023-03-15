/*****************************************************************************/
/* File Name   : rwl_common.h                                                */
/* Author      : Copyright OCL                                        			 */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : common header file																					 */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/
#ifndef RWL_COMMON_H
#define RWL_COMMON_H

#include <stdio.h>
#include "rwltype.h"

#define CLAMP(a, b) (((a) > (b))? (b): (a))

#define KEYLEN			16
#define FILELEN			128
#define	COM_PORT_TYPE_LEN	10
#define ININAME			"RWL.INI"
#define	ERR_FILE_COM			1
#define	ERR_FILE_OPEN			2

#define TEST_NAK				1

//#define TRUE	1
//#define FALSE	0

#define MSGOFF_HDRTYPE  	2   // message byte to determine extend command
#define MSGOFF_LEN      	3   // message length
#define MSGOFF_LCS      	4   // length checksum
#define MSGOFF_LCS_EXT		5	// length checksum for extend command
#define MSGOFF_DATA     	5   // start of data
#define MSGOFF_DATA_EXT 	6   // start of data
#define MSGOFFEND_DCS   	2   // offset from message end of data check sum
#define MSGSIZ_HEADER   	5   // size of message header
#define MSGSIZ_FOOTER   	2   // size of message footer
#define MSGSIZ_HEADER_EXT	6	// header for extend command
#define MSGSIZ_FOOTER_EXT 3   // size of message footer for extend command

#define ACK_RECV "ACK RECV"
#define CMD_START "CMD START"
#define CMD_SENT "CMD SENT"
#define ACK_SEND "ACK SEND"
#define NAK_SEND "NAK SEND"
#define DATA_START "DATA START"
#define CMD_DATA_TYPE	1
#define ACK_DATA_TYPE	2
#define NAK_DATA_TYPE	3
#define RWACKLMT 100000  //reader must return ack within 100ms after command sent
#define RWDATALMT 3500000  //reader must return data within 3.5s after ack received
#define MAXSTR          5000	//extend message size
#define MAXMSG          4956	//extend message size
#define MAX_SEGMENT_LEN	 4000

#pragma pack (push, 1)
typedef struct
{
	CHAR				OTPFILEDIR[FILELEN];
	CHAR				UPLOADDIR[FILELEN];
	BYTE				CommPort;
	CHAR				CommPortType[FILELEN];
	INT					CommBaud;
	INT					Time1;
	BYTE				DTK1[KEYLEN];
	BYTE				DTK2[KEYLEN];
	UINT				CompID;
} stRWLRec;
#pragma pack (pop)

#define GETLONG(p)      ( ( ((ULONG)*((p)  )) << 24 ) + \
                          ( ((ULONG)*((p)+1)) << 16 ) + \
                          ( ((ULONG)*((p)+2)) << 8  ) + \
                          ( ((ULONG)*((p)+3))       ) )

#define GETWORD(p)      ( ( ((ULONG)*((p)  )) << 8  ) + \
                          ( ((ULONG)*((p)+1))       ) )


void com_out(BYTE *msg, INT len, BYTE u8_DataType);
USHORT End2(USHORT);
UINT End4(UINT);
USHORT LongCS(BYTE *addr, UINT count);
USHORT GetINT2(BYTE *p);
UINT GetINT4(BYTE *p);

typedef struct {
	CHAR FN[1024];
	UINT FileOffset;
	UINT FileLen;
} FileRec;

extern INT		g_len;              /* length of inbuff */
extern BYTE   g_inbuff[MAXSTR];      /* input buffer from RWer */
extern stRWLRec 	theRWL;
extern BYTE g_AckExpired;
extern BYTE g_RespExpired;
extern INT ReadBlock (BYTE *);
void ComFlush(void);
extern void GetTime(CHAR *s);

void DbgPrint(const CHAR *fmt, ...);
void DbgDump(BYTE *bBuf, INT iLen);
void DbgCmdDump(BYTE *bBuf, INT iLen);
void DbgRspDump(BYTE *bBuf, INT iLen);

#endif // RWL_COMMON_H



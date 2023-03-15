/*****************************************************************************/
/* File Name   : csrwtest.c                                                  */
/* Author      : Copyright OCL                                                   */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : Command construction                                                                                */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include <sys/time.h>
#include <ctype.h>
//#include <fcntl.h>
//#include <unistd.h>
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


#include "csdes.h"
#include "rwl.h"
#include "rwl_common.h"
#include "csrw.h"
#include "octopusreader.h"

void    terninalPrintf (char* pcStr, ...);
#define printf terninalPrintf

#define __CSRWTEST__

/* variables */
BYTE ka[8], kb[8];     // Octopus Reader keys

/*====================[ Command functions ]=================================*/
/* global buffers and length to simplify programming */
BYTE   g_msg[MAXSTR];      /* message buffer to send to RWer */
BYTE   *g_pmsg;            /* pointer to message */

#define INITMSG()       g_pmsg=g_msg;*g_pmsg++=0x0;*g_pmsg++=0;*g_pmsg++=0xff;\
                        *g_pmsg++=0;*g_pmsg++=0;
#define INITEXTMSG()    g_pmsg=g_msg;*g_pmsg++=0x0;*g_pmsg++=0;*g_pmsg++=0xcc;\
                        *g_pmsg++=0;*g_pmsg++=0;*g_pmsg++=0;
#define PUTLONG(l)      *g_pmsg++=(BYTE) ((l)>>24);*g_pmsg++=(BYTE) ((l)>>16);\
                        *g_pmsg++=(BYTE) ((l)>>8);*g_pmsg++=(BYTE) (l)
#define PUTWORD(w)      *g_pmsg++=(BYTE) ((w)>>8);*g_pmsg++=(BYTE) (w)
#define PUTBYTE(b)      *g_pmsg++=(BYTE) (b)
#define SENDBLOCK(e)     PUTWORD(0);sendblock((g_pmsg-g_msg), g_msg, e && f_encrypt)

BYTE szOutMsg[MAXSTR];
BYTE msgkey[8];
INT f_encrypt=0;

Cmd_InitReceive initRecvCmd;
Cmd_ContentReceive contRecvCmd;
Rsp_InitReceive initRecvRsp;
Rsp_ContentReceive contRecvRsp;
Cmd_InitTran initTranCmd;
Cmd_ContentTran contTranCmd;
Rsp_InitTran initTranRsp;
Rsp_ContentTran contTranRsp;
//FILE *SendOTPfp;
uint8_t* OTPData = NULL;;
size_t OTPDataLen;
size_t OTPDataIndex;
//For time performance measurement
//struct timeval g_gettime;
//struct timeval g_lastgettime;

TickType_t g_gettime;
TickType_t g_lastgettime;

ULONG g_timeelapse;
BYTE g_AckExpired=0;
BYTE g_RespExpired=0;

/*============================================================================*/
/* Utility function                                                                       */
/*============================================================================*/

void GetTime (CHAR *s)
{
    TickType_t usecdiff=0;
    INT ret=0;

    //gettimeofday(&g_gettime, NULL);
    //usecdiff = (g_gettime.tv_sec - g_lastgettime.tv_sec)*1000000 + g_gettime.tv_usec - g_lastgettime.tv_usec;
    TickType_t g_gettime=xTaskGetTickCount ();
    usecdiff=g_gettime-g_lastgettime;
    //terninalPrintf(" GetTime [%s] enter : usecdiff = %d..\n", s, usecdiff);
    ret=strncmp (s, ACK_RECV, strlen (ACK_RECV));
    if(ret==0)
    {
        //terninalPrintf(" GetTime [ACK_RECV]: %d compare %d..\n", usecdiff, ((RWACKLMT/1000)/portTICK_RATE_MS/portTICK_RATE_MS));
        if(usecdiff>((RWACKLMT/1000)/portTICK_RATE_MS/portTICK_RATE_MS))
        {
            g_AckExpired=1;
        }
    }

    ret=strncmp (s, DATA_START, strlen (DATA_START));
    if(ret==0)
    {
        //terninalPrintf(" GetTime [DATA_START]: %d compare %d..\n", usecdiff, ((RWDATALMT/1000)/portTICK_RATE_MS/portTICK_RATE_MS));
        if(usecdiff>((RWDATALMT/1000)/portTICK_RATE_MS))
            g_RespExpired=1;
    }

    memmove (&g_lastgettime, &g_gettime, sizeof (g_gettime));
    /*
    ULONG usecdiff=0;
    INT ret = 0;

    gettimeofday(&g_gettime, NULL);
    usecdiff = (g_gettime.tv_sec - g_lastgettime.tv_sec)*1000000 + g_gettime.tv_usec - g_lastgettime.tv_usec;

    ret = strncmp(s, ACK_RECV, strlen(ACK_RECV));
    if (ret == 0) {
        if (usecdiff > RWACKLMT)
            g_AckExpired = 1;
    }

    ret = strncmp(s, DATA_START, strlen(DATA_START));
    if (ret == 0) {
        if (usecdiff > RWDATALMT)
            g_RespExpired = 1;
    }

    memmove(&g_lastgettime, &g_gettime, sizeof(g_gettime));
    */
}

void READBLOCK (BYTE b_decrypt)
{
    INT i=0;

    memset (g_inbuff, 0, MAXSTR);
    g_len=ReadBlock (g_inbuff);

    //retry no data receive or file com error
    for(i=0; i<theRWL.Time1; i++)
    {
        if(g_len==0)
        {
            printf ("No Data, send NAK\n");
            sendNAK ();
            g_len=ReadBlock (g_inbuff);
        }
        else
        {
            break;
        }
    }

    //not decrypt data for 0x7F error or file com error
    if((g_len>0)&&b_decrypt && f_encrypt&&(g_inbuff[0]!=ERR_RESP))
    {
        CscryptEncryptBlock (msgkey, &g_inbuff[1], (g_len-1), 0);
    }
}
#include "timelib.h"
/* return current time as seconds from 1/1/2000 GMT */
ULONG GetRTC2000 ()
{
    time_t t, t1;
    struct tm *gmt, tm2000={0, 0, 0, 1, 0, 100, 0, 0, 0, 0, 0};     //base on 1/1/1900

    #if(1)
    t = GetCurrentUTCTime();
    #else
    time (&t);               /* current time */
    gmt=gmtime (&t);       /* seconds since 1/1/70 */
    t=mktime (gmt);
    #endif
    t1=mktime (&tm2000);

    //printf ("GetRTC2000 (t = %d, t1 = %d)\n", t, t1);
    return (ULONG)t-(ULONG)t1;
}

INT MakeMessageHeader (BYTE *dest, INT cnt)
{
    INT i;
    unsigned chkSumExt;
    BYTE cs;

    if(cnt<=(256-MSGSIZ_HEADER-MSGSIZ_FOOTER))
    {
        // short packet can be sent by old message format
        dest[MSGOFF_HDRTYPE]=0xFF;            // sync byte for extend command
        dest[MSGOFF_LEN]=cnt;             // set up message length
        dest[MSGOFF_LCS]=(256-cnt);     // length check sum
        for(cs=i=0; i<cnt; i++)      // prepare data checksum
            cs+=dest[MSGOFF_DATA+i];
        dest[MSGOFF_DATA+cnt]=(0x100-(cs&0xff));
        dest[MSGOFF_DATA+cnt+1]=0;    // post amble
        return(cnt+MSGSIZ_HEADER+MSGSIZ_FOOTER);
    }
    else
    {
        // long packet need to sent by extend message format
        dest[MSGOFF_HDRTYPE]=0xCC;                                    // sync byte for extend command
        dest[MSGOFF_LEN]=((cnt>>8)&0xFF);             // set up message length
        dest[MSGOFF_LEN+1]=((cnt)&0xFF);              // set up message length
        dest[MSGOFF_LCS_EXT]=(0x100-((dest[MSGOFF_LEN]+dest[MSGOFF_LEN+1])&0xFF));
        chkSumExt=LongCS (&szOutMsg[MSGSIZ_HEADER_EXT], cnt);
        dest[MSGSIZ_HEADER_EXT+cnt]=(chkSumExt&0xFF);
        dest[MSGSIZ_HEADER_EXT+cnt+1]=((chkSumExt>>8)&0xFF);
        dest[MSGSIZ_HEADER_EXT+cnt+2]=0;                   // post amble
        return(cnt+MSGSIZ_HEADER_EXT+MSGSIZ_FOOTER_EXT);
    }
}

// prepare message to send to CSRW
// return: length of whole message
INT PrepMessage (BYTE *dest, CHAR *src)
{
#define ishex(c)    (( ((c)>='0')&&((c)<='9')) || \
                      (((c)>='A')&&((c)<='F')) || \
                      (((c)>='a')&&((c)<='f')) )
#define tohex(c)    ((toupper(c)>='A') ? (toupper(c)+10-'A') : (c-'0'))

    BYTE *p;
    INT c1, c2;
    INT cnt;

    dest[0]=0;        // preamble
    dest[1]=0;        // sync

    p=&dest[MSGOFF_DATA];     // start of data
    cnt=0;                    // data count
    c1=c2='\0';             // first and second characters of a hex number

    for(; *src!='\0'; src++)
    {
        if(ishex (*src))
        {
            if(c1==0)
                c1=*src;
            else
            {
                c2=*src;
                *p++=(tohex (c1)<<4)+tohex (c2);
                cnt++;
                if(cnt>=MAXMSG)
                    break;
                c1=c2='\0';
            }
        }
    }

    return MakeMessageHeader (dest, cnt);
}

INT MakeMessage (BYTE *dest, INT cnt)
{
    return MakeMessageHeader (dest, cnt);
}

/* - Encrypt the RW key to protect sensitive information
 * Description: Get the key for use
 * Output:      outkey - store the resultant key
 * Return:      0
 * Remarks: The length of outkey should be at least 16-byte (KEYLEN)
 */

static BYTE KaKey[] = {0x16, 0xA3, 0x29, 0xC7, 0x42, 0xF1, 0x06, 0x39};
static BYTE KbKey[] = {0xB7, 0x7D, 0x72, 0x19, 0x04, 0xB5, 0x7D, 0x91};
void ChangeKey(BYTE KeyType)
{    
    BYTE ProKaKey[] = {0x18, 0x22, 0xe0, 0xdc, 0xb7, 0x52, 0x94, 0xad};
    BYTE ProKbKey[] = {0x85, 0x3e, 0x69, 0x15, 0xf8, 0x46, 0x6a, 0x04};
    
    BYTE TestKaKey[] = {0x16, 0xA3, 0x29, 0xC7, 0x42, 0xF1, 0x06, 0x39};
    BYTE TestKbKey[] = {0xB7, 0x7D, 0x72, 0x19, 0x04, 0xB5, 0x7D, 0x91};
    
    if(KeyType == OCTOPUS_USE_PRODUCTION_KEY)
    {
        memcpy(KaKey,ProKaKey,sizeof(ProKaKey));
        memcpy(KbKey,ProKbKey,sizeof(ProKbKey));
    }
    else if(KeyType == OCTOPUS_USE_TEST_KEY)
    {
        memcpy(KaKey,TestKaKey,sizeof(TestKaKey));
        memcpy(KbKey,TestKbKey,sizeof(TestKbKey));
    }
}

INT GetKey (BYTE* outkey)
{
    /*
    The Ka and Kb value here are only for demonstration purpose
    key value will be delivered in seperate channel on demand
    key storage should be encrypted and should not disclose to third parties
    Please refer to section 2.11.2 and 2.13.1.2 in New Parking Meter - Integration Specification to SIs
    for key storage requirement and encryption/decryption algorithm
    */
#if(1)
    //?- Version = 0xFB
    //?- Ka = 0x16, 0xA3, 0x29, 0xC7, 0x42, 0xF1, 0x06, 0x39
    //?- Kb = 0xB7, 0x7D, 0x72, 0x19, 0x04, 0xB5, 0x7D, 0x91
    //Ka
    memcpy (outkey, KaKey, sizeof(KaKey));
    //Kb
    memcpy (outkey+8, KbKey, sizeof(KbKey));
#else
    //Ka
    memcpy (outkey, 0, 8);
    //Kb
    memcpy (&outkey[8], 0xFF, 8);
#endif
    return 0;
}
//====================================================================
// functions
//====================================================================
// send block to CSRW
void sendACK ()
{
    memset (szOutMsg, 0, 6);
    szOutMsg[2]=szOutMsg[4]=0xFF;
    com_out (szOutMsg, 6, ACK_DATA_TYPE);
}

// send block to CSRW
void sendNAK ()
{
    /* NAK = \x00\x00\xFF\xFF\x00\x00 */
    GetTime (NAK_SEND);
    memset (szOutMsg, 0, 6);
    szOutMsg[2]=szOutMsg[3]=0xFF;
    com_out (szOutMsg, 6, NAK_DATA_TYPE);
}

// send block to CSRW
void sendblock (INT count, BYTE *block, INT encrypt)
{
    INT i, cs=0;
    INT en_cnt;             // count those for encryption
    USHORT dataCS=0;

    memset (szOutMsg, 0, sizeof (szOutMsg));
    memcpy (szOutMsg, block, count);
    block=szOutMsg;

    /* if encryption, start encrypting after command code */
    if(encrypt && f_encrypt)
    {
        /* length of data to encrypt */
        // handle extend command message format
        if(szOutMsg[MSGOFF_HDRTYPE]==0xFF)
        {
            en_cnt=count-MSGSIZ_HEADER-MSGSIZ_FOOTER-1;
            en_cnt=((en_cnt+7)/8)*8;

            CscryptEncryptBlock (msgkey, &block[MSGOFF_DATA+1], en_cnt, 1);
            count=en_cnt+MSGSIZ_HEADER+MSGSIZ_FOOTER+1;
        }
        else
        {
            en_cnt=count-MSGSIZ_HEADER_EXT-MSGSIZ_FOOTER_EXT-1;
            en_cnt=((en_cnt+7)/8)*8;

            CscryptEncryptBlock (msgkey, &block[MSGOFF_DATA_EXT+1], en_cnt, 1);
            count=en_cnt+MSGSIZ_HEADER_EXT+MSGSIZ_FOOTER_EXT+1;
        }
    }

    /* calculate length checksum in szOutMsg[3] */
    // handle extend command message format
    if(szOutMsg[MSGOFF_HDRTYPE]==0xFF)
    {
        // normal message format
        szOutMsg[MSGOFF_LEN]=count-MSGSIZ_HEADER-MSGSIZ_FOOTER;
        szOutMsg[MSGOFF_LCS]=0x100-szOutMsg[MSGOFF_LEN];
    }
    else
    {
        // new extend message format
        szOutMsg[MSGOFF_LEN]=(((count-MSGSIZ_HEADER_EXT-MSGSIZ_FOOTER_EXT)>>8)&0xFF);
        szOutMsg[MSGOFF_LEN+1]=((count-MSGSIZ_HEADER_EXT-MSGSIZ_FOOTER_EXT)&0xFF);
        szOutMsg[MSGOFF_LCS_EXT]=(0x100-((szOutMsg[MSGOFF_LEN]+szOutMsg[MSGOFF_LEN+1])&0xFF));
    }

    // handle extend command message format
    if(szOutMsg[MSGOFF_HDRTYPE]==0xFF)
    {
        // normal message format
        for(i=MSGOFF_DATA; i<count-MSGSIZ_FOOTER; i++)
            cs+=szOutMsg[i];

        szOutMsg[count-MSGOFFEND_DCS]=0x100-(cs&0xff);
    }
    else
    {
        // new extend message format
        dataCS=LongCS (&szOutMsg[MSGSIZ_HEADER_EXT], (count-MSGSIZ_HEADER_EXT-MSGSIZ_FOOTER_EXT));
        szOutMsg[count-3]=((dataCS>>8)&0xFF);
        szOutMsg[count-2]=((dataCS)&0xFF);
    }

    szOutMsg[count-1]=0;             // post-amble

    //do not encrypt command if session has not established
    /*
    if(encrypt && f_encrypt)
        printf ("Enc Cmd Len = %d bytes\n", en_cnt+1);  //command code will not encrypt
    else
        printf ("No encryption\n");
*/
    com_out (szOutMsg, count, CMD_DATA_TYPE);
}

/*============================================================================*/
/* Function call to construt command message                                  */
/*============================================================================*/
void CmdPollDeduct (BYTE bTimeout, INT TxnAmt, const BYTE *ai, BYTE bAlertMsgFmt)
{
    CHAR szCmdData[MAXSTR];                         // detect message
    BYTE szCmdMsg[MAXSTR];          // actual detect message to send
    INT     iCmdMsgLen;
    ULONG TxnTime;

    strcpy (szCmdData, "70 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"); //17 bytes

    iCmdMsgLen=PrepMessage (szCmdMsg, szCmdData);
    TxnTime=GetRTC2000 ();
    szCmdMsg[7]=bTimeout;
    szCmdMsg[8]=(TxnAmt>>8)&0xff;
    szCmdMsg[9]=(TxnAmt)&0xff;
    szCmdMsg[10]=(TxnTime>>24)&0xff;
    szCmdMsg[11]=(TxnTime>>16)&0xff;
    szCmdMsg[12]=(TxnTime>>8)&0xff;
    szCmdMsg[13]=TxnTime&0xff;
    memcpy (&szCmdMsg[14], ai, 7);       /* Block Info, Additional Info */
    szCmdMsg[21]=bAlertMsgFmt;

    DbgCmdDump (szCmdMsg, iCmdMsgLen-MSGSIZ_FOOTER);//not print the message footer
    sendblock (iCmdMsgLen, szCmdMsg, 1);

    READBLOCK (1);
    DbgRspDump (g_inbuff, g_len);
}

void CmdWriteID (UINT ID)
{
    INITMSG ();
    PUTBYTE (CSCMD_WRITEID);
    PUTBYTE (0);
    PUTLONG (ID);
    DbgCmdDump (g_msg, (g_pmsg-g_msg));
    SENDBLOCK (1);
    READBLOCK (1);
    DbgRspDump (g_inbuff, g_len);
}

// Antenna OFF
void CmdAntennaOff (void)
{
    INITMSG ();
    PUTBYTE (CSCMD_ANTENNA_CTRL);
    PUTBYTE (0);
    DbgCmdDump (g_msg, (g_pmsg-g_msg));

    SENDBLOCK (1);
    READBLOCK (1);
    DbgRspDump (g_inbuff, g_len);
}

// End Session
void CmdEndSession (void)
{
    INITMSG ();
    PUTBYTE (CSCMD_END_SESSION);
    PUTBYTE (0);
    DbgCmdDump (g_msg, (g_pmsg-g_msg));
    SENDBLOCK (1);

    READBLOCK (1);
    DbgRspDump (g_inbuff, g_len);
    if((g_len>0)&&(g_inbuff[0]==CSCMD_END_SESSION+1)&&(g_inbuff[1]==0))
        f_encrypt=0;
}
#include "fileagent.h"
void CmdMetaFileInitSend (CHAR* inFile, CHAR* inDir, int inFileLen)
{
    UINT msgLen, fileLen;
    ULONG before, after;
    CHAR *fname;

    //DbgPrint ("(%s %s %d) Start\n", __FILE__, __func__, __LINE__);

    memset (&initTranCmd, 0, sizeof (Cmd_InitTran));
    memset (&initTranRsp, 0, sizeof (Rsp_InitTran));
    memset (&contTranCmd, 0, sizeof (Cmd_ContentTran));
    memset (&contTranRsp, 0, sizeof (Rsp_ContentTran));
#if(1)    
    BOOL needFree;
    FileAgentReturn reVal;  
    
    
    reVal = FileAgentGetData(FILE_AGENT_STORAGE_TYPE_FATFS, inDir, inFile, &OTPData, &OTPDataLen, &needFree, FALSE);
    if((reVal != FILE_AGENT_RETURN_ERROR) &&(OTPDataLen == inFileLen))
    {
        if(needFree)
        {
            vPortFree(OTPData);
        }
    }
    else
    {
        printf ("[WARNING!!!] FILE OPEN ERROR\n");
        return;
    }
#else
    if((SendOTPfp=fopen (inFile, "rb"))==NULL)
    {
        printf ("[WARNING!!!] FILE OPEN ERROR\n");
        return; // file open error
    }
#endif
    // send init packet first
    initTranCmd.f_cmdCode=CSCMD_INITTRAN; //0x5A	/* Metafile file Init transfer */
    initTranCmd.f_res=0;
    initTranCmd.f_fileType=10;//0x0A
    initTranCmd.f_verCtrl=0;
    initTranCmd.f_fileAct=0;
    fname=strrchr (inFile, 'O');
    printf ("fname:%s\n", fname);
    strncpy ((CHAR *)initTranCmd.f_name, (const CHAR *)fname, 32);

    //fseek (SendOTPfp, 0, SEEK_END);
    //fileLen=ftell (SendOTPfp);
    fileLen = inFileLen;
    
    initTranCmd.f_len=End4 (fileLen);
    printf ("f_len:%d\n", fileLen);
    initTranCmd.f_segSize=0;
    initTranCmd.f_segRemain=0;

    memcpy (&g_msg[MSGOFF_DATA], &initTranCmd, sizeof (Cmd_InitTran));
    msgLen=MakeMessage (g_msg, sizeof (Cmd_InitTran));
    DbgCmdDump (g_msg, msgLen-MSGSIZ_FOOTER); //not display footer
    sendblock (msgLen, g_msg, 1);

    before=GetRTC2000 ();
    after=before+10;    //just in case no repsonse from reader, set the time limit to wait for 10secs

    do
    {
        READBLOCK (1);
    }
    while((g_len==0)&&(GetRTC2000 ()<after));
    DbgRspDump (g_inbuff, g_len);

    if((g_len>0)&&(g_inbuff[0]==(CSCMD_INITTRAN+1))&&(g_inbuff[1]==0))
    {
        initTranRsp.f_rspCode=g_inbuff[0];
        initTranRsp.f_err=g_inbuff[1];
        initTranRsp.f_fileID=GetINT2 (&g_inbuff[2]);
        initTranRsp.f_segSize=GetINT2 (&g_inbuff[4]);

        printf ("InitTran success, fileID:%x segSize:%d\n", initTranRsp.f_fileID, initTranRsp.f_segSize);
    }
}

void CmdMetaFileContSend (void)
{
    
    INT nRec=0;
    UINT msgLen;
    UINT fileLen=0;
    ULONG before, after;
    USHORT fileID, rByte;
    
    OTPDataIndex = 0;
    //fseek (SendOTPfp, 0, SEEK_SET);
    
    fileID=initTranRsp.f_fileID;
    fileLen=End4 (initTranCmd.f_len);

    while(fileLen>contTranRsp.f_nextOff)
    {
        //int targetCopyLen;
        
        contTranCmd.f_cmdCode=CSCMD_CONTTRAN;
        contTranCmd.f_res=0;

        contTranCmd.f_fileID=End2 (fileID);
        contTranCmd.f_offset=End4(OTPDataIndex);//End4 (ftell (SendOTPfp));
        
        //rByte=fread (contTranCmd.f_data, sizeof (CHAR), initTranRsp.f_segSize, SendOTPfp);        
  
        if((OTPDataIndex + initTranRsp.f_segSize) > OTPDataLen)
        {
            rByte = OTPDataLen - OTPDataIndex;
        }
        else
        {
            rByte = initTranRsp.f_segSize;
        }
        memcpy(contTranCmd.f_data, OTPData + OTPDataIndex, rByte);        
        
        printf ("\r\n--> f_fileID 0x%04x (fileLen = %d): contTranCmd->f_offset:%d (rByte = %d)\n", End2 (contTranCmd.f_fileID), fileLen, End4 (contTranCmd.f_offset), rByte);
        contTranCmd.f_len=End2 (rByte);

        if(sizeof (Cmd_ContentTran)-sizeof (contTranCmd.f_data)+rByte>255)
            memcpy (&g_msg[MSGOFF_DATA_EXT], &contTranCmd, sizeof (Cmd_ContentTran)-sizeof (contTranCmd.f_data)+rByte);
        else
            memcpy (&g_msg[MSGOFF_DATA], &contTranCmd, sizeof (Cmd_ContentTran)-sizeof (contTranCmd.f_data)+rByte);
        
        msgLen=MakeMessage (g_msg, sizeof (Cmd_ContentTran)-sizeof (contTranCmd.f_data)+rByte);
        
        if(msgLen>255)
        {
            DbgCmdDump (g_msg, msgLen-MSGSIZ_FOOTER_EXT);      //limit the debug dump size
        }
        else
        {
            DbgCmdDump (g_msg, msgLen-MSGSIZ_FOOTER);      //limit the debug dump size
        }
        sendblock (msgLen, g_msg, 1);
        before=GetRTC2000 ();
        after=before+10;    //just in case no repsonse from reader, set the time limit to wait for 10secs

        do
        {
            READBLOCK (1);
        }
        while((g_len==0)&&(GetRTC2000 ()<after));

        DbgRspDump (g_inbuff, g_len);
        if((g_len>0)&&(g_inbuff[0]==(CSCMD_CONTTRAN+1))&&(g_inbuff[1]==0))
        {
            nRec++;
            contTranRsp.f_rspCode=g_inbuff[0];
            contTranRsp.f_err=g_inbuff[1];
            contTranRsp.f_byteWrite=GetINT2 (&g_inbuff[2]);
            contTranRsp.f_nextOff=GetINT4 (&g_inbuff[4]);

            printf ("ContTran success, wByte:0x%04x(%d) nextOff:%d\n", contTranRsp.f_byteWrite, contTranRsp.f_byteWrite, contTranRsp.f_nextOff);
            //fseek (SendOTPfp, contTranRsp.f_nextOff, SEEK_SET);
            OTPDataIndex = contTranRsp.f_nextOff;
        }
        else
        {
            printf ("Fail in getting cont tran response 0x%x %d\n", contTranRsp.f_rspCode, contTranRsp.f_err);
            //fclose (SendOTPfp);
            if(OTPData != NULL)
            {
                vPortFree(OTPData);
                OTPData = NULL;
            }
            return;
        }
        //printf ("fileLen:%d, rByte%d\n", fileLen, rByte);
    }

    //fclose (SendOTPfp);
    if(OTPData != NULL)
    {
        vPortFree(OTPData);
        OTPData = NULL;
    }
    
    if(fileLen<=contTranRsp.f_nextOff)
        f_encrypt=0;
    printf (" f_encrypt %d\n", f_encrypt);
    
}

void CmdXFileInitRecv (CHAR* outFile, UINT u32_SegmentSize)
{
    UINT msgLen;

    memset (&initRecvCmd, 0, sizeof (Cmd_InitReceive));
    memset (&initRecvRsp, 0, sizeof (Rsp_InitReceive));
    memset (&contRecvCmd, 0, sizeof (Cmd_ContentReceive));
    memset (&contRecvRsp, 0, sizeof (Rsp_ContentReceive));

    // send init packet first
    initRecvCmd.f_cmdCode=CSCMD_INITRECV;
    initRecvCmd.f_res=0;
    initRecvCmd.f_fileType=1;
    initRecvCmd.f_segSize=End2 (u32_SegmentSize);

    memcpy (&g_msg[MSGOFF_DATA], &initRecvCmd, sizeof (Cmd_InitReceive));
    msgLen=MakeMessage (g_msg, sizeof (Cmd_InitReceive));
    DbgCmdDump (g_msg, msgLen-MSGSIZ_FOOTER); //not display footer
    sendblock (msgLen, g_msg, 1);
    READBLOCK (1);
    DbgRspDump (g_inbuff, g_len);

    if((g_len>0)&&(g_inbuff[0]==(CSCMD_INITRECV+1))&&(g_inbuff[1]==0))
    {
        initRecvRsp.f_rspCode=g_inbuff[0];
        initRecvRsp.f_err=g_inbuff[1];
        memcpy (&initRecvRsp.f_name[0], &g_inbuff[2], 32);
        initRecvRsp.f_len=GetINT4 (&g_inbuff[34]);
        initRecvRsp.f_fileID=GetINT2 (&g_inbuff[38]);
        strcpy ((CHAR *)outFile, (const CHAR *)initRecvRsp.f_name);
    }

    printf ("InitRecv success, fileID:%x fileLen:%lu fname:%s\n", initRecvRsp.f_fileID, initRecvRsp.f_len, initRecvRsp.f_name);
}

void CmdXFileContRecv (CHAR* outFile, UINT u32_SegmentSize)
{
#if(1)
    UINT msgLen, fileLen;
    //FILE *fp;
    ULONG before, after;
    BYTE u8_LastDummyPacket=0;
    USHORT fileID;
    //CHAR fname[256];
    
    uint8_t* xFileDataPr;
    int xFileDataIndex = 0;
    int xFileDataLen = 0;

    //sprintf (fname, "%s/%s", theRWL.UPLOADDIR, initRecvRsp.f_name);
    
    //if((fp=fopen (fname, "wb+"))==NULL)
    //{
    //    printf ("[WARNING!!!] FILE OPEN ERROR\n");
    //    return; // file open error
    //}
    fileLen=End4 (GetINT4 ((BYTE *)&initRecvRsp.f_len));
    xFileDataLen = fileLen;
    fileID=End2 (initRecvRsp.f_fileID);
    
    printf ("CmdXFileContRecv: target f_name:[%s](xFileDataLen = %d), u32_SegmentSize = %d\n", initRecvRsp.f_name, fileLen, u32_SegmentSize);
    
    xFileDataPr = pvPortMalloc(fileLen);
    if(xFileDataPr == NULL)
    {
        printf ("[WARNING!!!] CmdXFileContRecv pvPortMalloc ERROR\n");
        return; // file open error
    }
    

    while((fileLen>0)||u8_LastDummyPacket)
    {
        contRecvCmd.f_cmdCode=CSCMD_CONTRECV;
        contRecvCmd.f_res=0;
        contRecvCmd.f_fileID=fileID;
        
        //contRecvCmd.f_offset=End4 (ftell (fp));
        contRecvCmd.f_offset=End4 (xFileDataIndex);
        
        
        if(fileLen>u32_SegmentSize)
            contRecvCmd.f_len=End2 (u32_SegmentSize);
        else
            contRecvCmd.f_len=End2 (fileLen);

        if(u8_LastDummyPacket)
            contRecvCmd.f_len=0;

        memcpy (&g_msg[MSGOFF_DATA], &contRecvCmd, sizeof (Cmd_ContentReceive));

        printf ("f_fileID 0x%x contRecvCmd->f_offset:0x%x(%d), left fileLen:%d\n", End2 (contRecvCmd.f_fileID), End4 (contRecvCmd.f_offset), End4 (contRecvCmd.f_offset), fileLen);
        msgLen=MakeMessage (g_msg, sizeof (Cmd_ContentReceive));
        DbgCmdDump (g_msg, msgLen-MSGSIZ_FOOTER); //not display footer
        sendblock (msgLen, g_msg, 1);
        before=GetRTC2000 ();
        after = before + 10;    //just in case no repsonse from reader, set the time limit to wait for 10secs

        while(1)
        {
            do
            {
                READBLOCK (1);
            }
            while((g_len==0)&&(GetRTC2000 ()<after));

            DbgRspDump (g_inbuff, g_len);

            printf ("g_len:%d g_inbuff[0] :%x g_inbuff[1] :%x\n", g_len, g_inbuff[0], g_inbuff[1]);

            if((g_len>0)&&(g_inbuff[0]==(CSCMD_CONTRECV+1))&&(g_inbuff[1]==0))
            {
                contRecvRsp.f_rspCode=g_inbuff[0];
                contRecvRsp.f_err=g_inbuff[1];
                contRecvRsp.f_byteRead=GetINT2 (&g_inbuff[2]);
                contRecvRsp.f_nextOff=GetINT4 (&g_inbuff[4]);
                memcpy (&contRecvRsp.f_data[0], &g_inbuff[8], contRecvRsp.f_byteRead);

                if(!u8_LastDummyPacket)
                {
                    //this is resend of last packet, discard this
                    if(End4 (contRecvCmd.f_offset)!=contRecvRsp.f_nextOff)
                    {
                        //fwrite (&contRecvRsp.f_data[0], 1, contRecvRsp.f_byteRead, fp);
                        memcpy(xFileDataPr+xFileDataIndex, &contRecvRsp.f_data[0], contRecvRsp.f_byteRead);
                        
                        printf ("ContRecv success, wByte:%x nextOff:%lu\n", contRecvRsp.f_byteRead, contRecvRsp.f_nextOff);
                        fileLen-=contRecvRsp.f_byteRead;
                        //fseek (fp, contRecvRsp.f_nextOff, SEEK_SET);
                        xFileDataIndex = contRecvRsp.f_nextOff;
                        
                    }
                }
                else
                {
                    //fwrite (&contRecvRsp.f_data[0], 1, contRecvRsp.f_byteRead, fp);
                    memcpy(xFileDataPr+xFileDataIndex, &contRecvRsp.f_data[0], contRecvRsp.f_byteRead);
                    
                    printf ("ContRecv success (u8_LastDummyPacket), wByte:%x nextOff:%lu\n", contRecvRsp.f_byteRead, contRecvRsp.f_nextOff);
                    fileLen-=contRecvRsp.f_byteRead;

                    //fseek (fp, contRecvRsp.f_nextOff, SEEK_SET);
                    xFileDataIndex = contRecvRsp.f_nextOff;
                }

                if(u8_LastDummyPacket)
                {
                    u8_LastDummyPacket=0;
                }
                else if(fileLen==0)
                {//last dummy packet
//send last dummy packet to trigger RW purge UD
                    u8_LastDummyPacket=1;
                }
                break;
            }
            else
            {
                printf ("Fail in getting init recv response rspCode:%x, rspErr:%x\n", contRecvRsp.f_rspCode, contRecvRsp.f_err);
                //fclose (fp);
                //unlink (fname);
                if(xFileDataPr != NULL)
                {
                    vPortFree(OTPData);
                }
                return;
            }
        }

        printf ("fileLen:%d\n", fileLen);
    }

    //fclose (fp);
    sysprintf("\r\n--- xFileDataPr [%d] --->\r\n", xFileDataLen);
    for(int i = 0; i < xFileDataLen; i++)
    {
        sysprintf("0x%02x, ", xFileDataPr[i]);
        if(i%10 == 9)
            sysprintf("\r\n");
    
    }
    sysprintf("\r\n<--- xFileDataPr ---\r\n");
    
    strcpy ((CHAR *)outFile, (const CHAR *)initRecvRsp.f_name);
    //reVal = FileAgentGetData(FILE_AGENT_STORAGE_TYPE_FATFS, inDir, inFile, &OTPData, &OTPDataLen, &needFree, FALSE);
    //FileAgentReturn FileAgentAddData(StorageType storageType,char* dir, char* name, uint8_t* data, int dataLen, FileAgentAddType addType, BOOL dataNeedFreeFlag, BOOL blockFlag, BOOL checkMode);
    if(FileAgentAddData(FILE_AGENT_STORAGE_TYPE_FATFS, "0:", outFile, xFileDataPr, xFileDataLen, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, FALSE) ==  FILE_AGENT_RETURN_ERROR ) 
    {
        printf ("CmdXFileContRecv: Save FAIL, outFile fname:%s\n", outFile);
    }
    else
    {
        printf ("CmdXFileContRecv: Save SUCCESS, outFile fname:%s\n", outFile);
    }
    if(xFileDataPr != NULL)
    {
        vPortFree(OTPData);
    }
    
#else
    UINT msgLen, fileLen;
    FILE *fp;
    ULONG before, after;
    BYTE u8_LastDummyPacket=0;
    USHORT fileID;
    CHAR fname[256];

    sprintf (fname, "%s/%s", theRWL.UPLOADDIR, initRecvRsp.f_name);
    printf ("target fname:%s\n", fname);
    if((fp=fopen (fname, "wb+"))==NULL)
    {
        printf ("[WARNING!!!] FILE OPEN ERROR\n");
        return; // file open error
    }
    fileLen=End4 (GetINT4 ((BYTE *)&initRecvRsp.f_len));
    fileID=End2 (initRecvRsp.f_fileID);

    while((fileLen>0)||u8_LastDummyPacket)
    {
        contRecvCmd.f_cmdCode=CSCMD_CONTRECV;
        contRecvCmd.f_res=0;
        contRecvCmd.f_fileID=fileID;
        contRecvCmd.f_offset=End4 (ftell (fp));
        if(fileLen>u32_SegmentSize)
            contRecvCmd.f_len=End2 (u32_SegmentSize);
        else
            contRecvCmd.f_len=End2 (fileLen);

        if(u8_LastDummyPacket)
            contRecvCmd.f_len=0;

        memcpy (&g_msg[MSGOFF_DATA], &contRecvCmd, sizeof (Cmd_ContentReceive));

        printf ("f_fileID %x contRecvCmd->f_offset:%x fileLen:%d\n", End2 (contRecvCmd.f_fileID), End4 (contRecvCmd.f_offset), fileLen);
        msgLen=MakeMessage (g_msg, sizeof (Cmd_ContentReceive));
        DbgCmdDump (g_msg, msgLen-MSGSIZ_FOOTER); //not display footer
        sendblock (msgLen, g_msg, 1);
        before=GetRTC2000 ();
        after=before+10;    //just in case no repsonse from reader, set the time limit to wait for 10secs

        while(1)
        {
            do
            {
                READBLOCK (1);
            }
            while((g_len==0)&&(GetRTC2000 ()<after));

            DbgRspDump (g_inbuff, g_len);

            printf ("g_len:%d g_inbuff[0] :%x g_inbuff[1] :%x\n", g_len, g_inbuff[0], g_inbuff[1]);

            if((g_len>0)&&(g_inbuff[0]==(CSCMD_CONTRECV+1))&&(g_inbuff[1]==0))
            {
                contRecvRsp.f_rspCode=g_inbuff[0];
                contRecvRsp.f_err=g_inbuff[1];
                contRecvRsp.f_byteRead=GetINT2 (&g_inbuff[2]);
                contRecvRsp.f_nextOff=GetINT4 (&g_inbuff[4]);
                memcpy (&contRecvRsp.f_data[0], &g_inbuff[8], contRecvRsp.f_byteRead);

                if(!u8_LastDummyPacket)
                {
                    //this is resend of last packet, discard this
                    if(End4 (contRecvCmd.f_offset)!=contRecvRsp.f_nextOff)
                    {
                        fwrite (&contRecvRsp.f_data[0], 1, contRecvRsp.f_byteRead, fp);
                        printf ("ContRecv success, wByte:%x nextOff:%lu\n", contRecvRsp.f_byteRead, contRecvRsp.f_nextOff);
                        fileLen-=contRecvRsp.f_byteRead;
                        fseek (fp, contRecvRsp.f_nextOff, SEEK_SET);
                    }
                }
                else
                {
                    fwrite (&contRecvRsp.f_data[0], 1, contRecvRsp.f_byteRead, fp);
                    printf ("ContRecv success, wByte:%x nextOff:%lu\n", contRecvRsp.f_byteRead, contRecvRsp.f_nextOff);
                    fileLen-=contRecvRsp.f_byteRead;

                    fseek (fp, contRecvRsp.f_nextOff, SEEK_SET);
                }

                if(u8_LastDummyPacket)
                    u8_LastDummyPacket=0;
                else if(fileLen==0)
                {//last dummy packet
//send last dummy packet to trigger RW purge UD
                    u8_LastDummyPacket=1;
                }
                break;
            }
            else
            {
                printf ("Fail in getting init recv response rspCode:%x, rspErr:%x\n", contRecvRsp.f_rspCode, contRecvRsp.f_err);
                fclose (fp);
                unlink (fname);
                return;
            }
        }

        printf ("fileLen:%d\n", fileLen);
    }

    fclose (fp);

    strcpy ((CHAR *)outFile, (const CHAR *)initRecvRsp.f_name);
#endif
    return;
}

INT CmdAuthenticate (BYTE *u8_stage)
{
    INT i;
    static BYTE rc[8]={1,3,5,7,9,11,13,15};
    BYTE enrc[8], enrr[8], rr[8];
    BYTE u8_key[16];

    f_encrypt=0;  //reset encryption flag every time try to auth

    GetKey (u8_key);
    memcpy (ka, u8_key, 8);
    memcpy (kb, u8_key+8, 8);

    for(i=0; i<8; i++)
        rc[i]=rand ();

    CscryptEncrypt (ka, rc, enrc, 1);        /* Encrypt Rc */

    *u8_stage=CSCMD_AUTH1;
    INITMSG ();
    PUTBYTE (CSCMD_AUTH1);
    PUTBYTE (0);
    PUTBYTE (enrc[0]);
    PUTBYTE (enrc[1]);
    PUTBYTE (enrc[2]);
    PUTBYTE (enrc[3]);
    PUTBYTE (enrc[4]);
    PUTBYTE (enrc[5]);
    PUTBYTE (enrc[6]);
    PUTBYTE (enrc[7]);
    DbgCmdDump (g_msg, (g_pmsg-g_msg));
    SENDBLOCK (0);
    READBLOCK (0);
    DbgRspDump (g_inbuff, g_len);

    // verify response
    if((g_inbuff[0]!=CSCMD_AUTH1+1)||(g_inbuff[1]!=0))
    {
        printf ("Auth 1 failed\n");
        return 1;
    }

    if(g_len<18)
    {
        printf ("Auth 1 response too short\n");
        return 1;
    }

    memcpy (enrc, &g_inbuff[2], 8);
    memcpy (enrr, &g_inbuff[10], 8);

    CscryptEncrypt (kb, enrc, enrc, 0);  /* Decrypt Rc */
    if(memcmp (rc, enrc, 8))
    {
        printf ("Auth 1 response failed\n");
        return 1;
    }

    // send auth 2 to reader
    CscryptEncrypt (kb, enrr, rr, 0);    /* Decrypt Rr */
    CscryptEncrypt (ka, rr, enrr, 1);    /* Encrypt Rr */

    *u8_stage=CSCMD_AUTH2;
    INITMSG ();
    PUTBYTE (CSCMD_AUTH2);
    PUTBYTE (0);
    PUTBYTE (enrr[0]);
    PUTBYTE (enrr[1]);
    PUTBYTE (enrr[2]);
    PUTBYTE (enrr[3]);
    PUTBYTE (enrr[4]);
    PUTBYTE (enrr[5]);
    PUTBYTE (enrr[6]);
    PUTBYTE (enrr[7]);

    DbgCmdDump (g_msg, (g_pmsg-g_msg));
    SENDBLOCK (0);
    READBLOCK (0);
    DbgRspDump (g_inbuff, g_len);

    if((g_inbuff[0]==CSCMD_AUTH2+1)&&(g_inbuff[1]==0))
    {
        memcpy (msgkey, rr, 8);
        f_encrypt=1;
        return 0;
    }
    else
    {
        printf ("Auth 2 failed\n");
        return 1;
    }
}

void CmdTimeVer (UINT * u32_time)
{
    //46 00 07 7F 02 09 02 0B 06 13 00 00 00 00 00

    time_t sec;
    struct tm *tms;
    UINT u32_temp[6];

    memset (u32_temp, 0, sizeof (u32_temp));

    INITMSG ();
    /*
    g_pmsg=g_msg;
    *g_pmsg++=0x0;
    *g_pmsg++=0;
    *g_pmsg++=0xff;
    *g_pmsg++=0;
    *g_pmsg++=0;
    */
    PUTBYTE (CSCMD_TIMEVER);
    //*g_pmsg++=(BYTE) (b) 

    PUTBYTE (0);
#if(1)
    if(memcmp (u32_temp, u32_time, sizeof (u32_temp))==0)
    {
        time (&sec);
        tms=gmtime (&sec);
        PUTBYTE (tms->tm_sec%10);
        PUTBYTE (tms->tm_sec/10);
        PUTBYTE (tms->tm_min%10);
        PUTBYTE (tms->tm_min/10);
        PUTBYTE (tms->tm_hour%10);
        PUTBYTE (tms->tm_hour/10);
        PUTBYTE (tms->tm_mday%10);
        PUTBYTE (tms->tm_mday/10);
        PUTBYTE ((tms->tm_mon+1)%10);
        PUTBYTE ((tms->tm_mon+1)/10);
        PUTBYTE ((tms->tm_year+1900)%10);
        PUTBYTE (((tms->tm_year+1900)%100)/10);
        PUTBYTE (0);     //day of the week, set to 0 as not interpret by R/W
    }
    else
    {
       // terninalPrintf (" u32_time = [%d/%d/%d %d:%d:%d]\r\n", u32_time[5], u32_time[4], u32_time[3], u32_time[2], u32_time[1], u32_time[0]);
        PUTBYTE (u32_time[0]%10);  //second
        PUTBYTE (u32_time[0]/10);  //second
        PUTBYTE (u32_time[1]%10);  //min
        PUTBYTE (u32_time[1]/10);  //min
        PUTBYTE (u32_time[2]%10);  //hour
        PUTBYTE (u32_time[2]/10);  //hour
        PUTBYTE (u32_time[3]%10);  //day
        PUTBYTE (u32_time[3]/10);  //day
        PUTBYTE (u32_time[4]%10);  //month
        PUTBYTE (u32_time[4]/10);  //month
        PUTBYTE (u32_time[5]%10);  //year 20XX
        PUTBYTE (u32_time[5]/10);  //year 20XX
        PUTBYTE (0);     //day of the week, set to 0 as not interpret by R/W
    }
#else
    PUTBYTE (9);
    PUTBYTE (0);
    PUTBYTE (8);
    PUTBYTE (4);
    PUTBYTE (0);
    PUTBYTE (1);
    PUTBYTE (6);
    PUTBYTE (1);
    PUTBYTE (7);
    PUTBYTE (0);
    PUTBYTE (5);
    PUTBYTE (1);
    PUTBYTE (0);
#endif

    DbgCmdDump (g_msg, (g_pmsg-g_msg));

    SENDBLOCK (0);
    READBLOCK (0);

    DbgRspDump (g_inbuff, g_len);
}



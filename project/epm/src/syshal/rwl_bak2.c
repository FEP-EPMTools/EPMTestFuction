/*****************************************************************************/
/* File Name   : rwl.c			                                                 */
/* Author      : Copyright OCL                                        			 */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : serial port related API    																 */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/
#include <stdarg.h>
#include <limits.h>
#include "rwl.h"
#include "rwl_common.h"
#include "csrw.h"
#include "csdes.h"
#include <stdlib.h>
#include <string.h>
//#include <termios.h>
//#include <fcntl.h>
#include <stdio.h>
//#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "octopusreader.h"


void    terninalPrintf (char* pcStr, ...);
#define printf terninalPrintf

/*
 * Pre-defined Constants
 */
#define BAUDRATE        B921600
#define COMMPORT        0

 /*
  * Macros
  */
#define GetBaudRate(b)  ( ( b == 9600 ) ? B9600 : \
                          ( b == 19200 ) ? B19200 : \
                          ( b == 38400 ) ? B38400 : \
                          ( b == 57600 ) ? B57600 : \
                          ( b == 115200 ) ? B115200 : \
                          ( b == 230400 ) ? B230400 : \
                          ( b == 460800 ) ? B460800 : \
                          ( b == 500000 ) ? B500000 : \
                          ( b == 576000 ) ? B576000 : \
                          ( b == 921600 ) ? B921600 : BAUDRATE)

  //define state machine for message format reception
#define ST_SENDNAK			0
#define ST_PREAMBLE			1
#define ST_START1				2
#define ST_START2				3
#define ST_LEN					4
#define ST_LCS					5
#define ST_DATA					6
#define ST_DCS					7
#define ST_POSTAMBLE		8
#define ST_ACKPOSTAMBLE	9
#define ST_NAKPOSTAMBLE	10
#define	ST_NAK					11
#define ST_COMPLETE			12
#define ST_LEN_EX1			13
#define ST_LEN_EX2			14
#define ST_DCS1					15
#define	ST_DCS2					16

/*
 * Local variables
 */
 //static INT fOpened = 0;
 //static struct termios oldtio;
 //static INT hComPort = -1;

INT g_len;              /* length of inbuff */
BYTE   g_inbuff[MAXSTR];      /* input buffer from RWer */
stRWLRec theRWL;
//////////////////////////////////////////////////////////////////////////////
// Internal API prototype
//////////////////////////////////////////////////////////////////////////////
//static INT InitCommunication (void);
//static ULONG GetPrivateProfileStringX(CHAR * lpAppName, CHAR * lpKeyName, CHAR * lpDefault, CHAR * lpReturnedString, ULONG nSize, CHAR * lpFileName);
///////////////////////////////////////////////////////////////////////////////
// API
///////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------
// InitCommunication - Open and initialize selected COM port.

static void printfBuffData(char* str, uint8_t* data, int len)
{
    int i;
    terninalPrintf("\r\n %s: len = %d start -->\r\n", str, len);
    
    for(i = 0; i<len; i++)
    { 
        #if(0)
        terninalPrintf("[%02d:0x%02x], ",i, (unsigned char)data[i]);
        if((i%16) == 15)
            terninalPrintf("\r\n");
        #else
        terninalPrintf("  0x%02x, ", (unsigned char)data[i]);
        #endif
    }
    terninalPrintf("<-- %s end\r\n\r\n", str);
    
}
#if(0)
static INT InitCommunication ()
{
#if(1)
    return 0;
#else
    CHAR  szPort[15];
    struct termios newtio;

    if(fOpened==1)
        close (hComPort);

    sprintf (szPort, "/dev/tty%s%d", theRWL.CommPortType, ((theRWL.CommPort)?theRWL.CommPort-1:COMMPORT));
    hComPort=open (szPort, O_RDWR|O_NOCTTY);
    if(hComPort<0)
        return hComPort;

    // save old config 
    tcgetattr (hComPort, &oldtio);

    // setup baudrate and other communication port configuration 
    newtio.c_cflag=GetBaudRate (theRWL.CommBaud)|CS8|CLOCAL|CREAD;

    newtio.c_cflag&=~PARENB;
    // Ignore bytes with parity erros and make terminal raw and dumb 
    newtio.c_iflag=IGNPAR;

    // Raw output 
    newtio.c_oflag=0;

    // Don't echo characters and don't generate signals 
    newtio.c_lflag=0;

    // blocking read until timer alarms
    newtio.c_cc[VMIN]=0;
    newtio.c_cc[VTIME]=5;

    // clean the line and activate the settings 
    tcflush (hComPort, TCIOFLUSH);
    tcsetattr (hComPort, TCSANOW, &newtio);
    fOpened=1;

    return theRWL.CommPort;
#endif
}
#endif



/* put a character to COM port, wait till all sent */
void com_out (BYTE *msg, INT len, BYTE u8_DataType)
{   
    //old//terninalPrintf(" com_out  %d bytes..\n", len);
    //old//printfBuffData(" -- [com_out] --", msg, len);
    
    ComFlush();	//flush any date receive but not read    
    GetTime(CMD_START);
    //old//OctopusReaderWrite(msg, len); 
    
   //old// write(hComPort, msg, len);
    //old//tcdrain(hComPort);

    switch (u8_DataType)
    {
    case CMD_DATA_TYPE:
        GetTime(CMD_SENT);
        break;
    case ACK_DATA_TYPE:
        GetTime(ACK_SEND);
        break;
    case NAK_DATA_TYPE:
        GetTime(NAK_SEND);
        break;
    default:
        GetTime("UNKNOWN DATA");
        break;
    }
    
}

void ComFlush (void)
{
    ////OctopusReaderFlushBuffer();
#if(0)
    if(hComPort&&(fOpened==1))
        // clean the line and activate the settings 
        tcflush (hComPort, TCIFLUSH);
#endif
}

//INT ReadData(BYTE *Msg, INT iExpect, struct timeval *st_TimeOut)
INT ReadData (BYTE *Msg, INT iExpect, TickType_t* st_TimeOut)
{
#if(1)
    INT i=0;
    int m_nBytes = 0;
    TickType_t tickLocalStart = xTaskGetTickCount();
    //vTaskDelay(2000/portTICK_RATE_MS);
    //terninalPrintf(" ReadData [iExpect = %d, st_TimeOut = %d] enter ..\n", iExpect, *st_TimeOut);
    while(1)
    {
        ////m_nBytes = OctopusReaderRead(Msg+i, iExpect);        
        //if(m_nBytes>=0)
        if(m_nBytes > 0)
        {   
            i+=(USHORT)m_nBytes;
            
            //terninalPrintf(" ReadData [iExpect = %d, st_TimeOut = %d] Read %d [%d] bytes..\n", iExpect, *st_TimeOut, m_nBytes, i);
            //printfBuffData(" -- [ReadData] --", Msg, i);
            
            if(i==iExpect)
                return i;
            //else
            //    return 0;
        }
        //terninalPrintf(" ReadData [iExpect = %d, st_TimeOut = %d] compare %d vs %d ..\n", iExpect, *st_TimeOut, (xTaskGetTickCount() - tickLocalStart), *st_TimeOut);
        terninalPrintf(".");
        if((xTaskGetTickCount() - tickLocalStart) > *st_TimeOut)
        {
            terninalPrintf(" ReadData [iExpect = %d, st_TimeOut = %d] Break ..\n", iExpect, *st_TimeOut);
            break;
        }
        vTaskDelay(10/portTICK_RATE_MS);

    }
    return 0;
#else
    INT i=0;
    ssize_t m_nBytes;
    fd_set ReadCom;
    FD_ZERO (&ReadCom);
    FD_SET (hComPort, &ReadCom);

    if(!fOpened)
        return -1;

    if(Msg==NULL)
        return -1;

    if(select (hComPort+1, &ReadCom, NULL, NULL, st_TimeOut)>0)
    {
        if(FD_ISSET (hComPort, &ReadCom))
        {
            m_nBytes=read (hComPort, Msg+i, iExpect);
            if(m_nBytes>=0)
            {
                i+=(USHORT)m_nBytes;
                if(i==iExpect)
                    return i;
                else
                    return 0;
            }
        }
    }

    return 0; //no data return within timeout period
#endif
}

//======================================================================
// ReadThread - Receives characters from the serial port
//======================================================================
static char* getStateStr(int index)
{
    switch(index)
    {
        case 0:
            return "ST_SENDNAK";
        case 1:
            return "ST_PREAMBLE";
        case 2:
            return "ST_START1";
        case 3:
            return "ST_START2";
        case 4:
            return "ST_LEN";
        case 5:
            return "ST_LCS";
        case 6:
            return "ST_DATA";
        case 7:
            return "ST_DCS";
        case 8:
            return "ST_POSTAMBLE";
        case 9:
            return "ST_ACKPOSTAMBLE";
        case 10:
            return "ST_NAKPOSTAMBLE";
        
        case 11:
            return "ST_NAK";
        case 12:
            return "ST_COMPLETE";
        case 13:
            return "ST_LEN_EX1";
        case 14:
            return "ST_LEN_EX2";
        case 15:
            return "ST_DCS1";
        case 16:
            return "ST_DCS2";
        default:
            return "OTHER";
        
    }
}



//[00:0x00], [01:0x00], [02:0x00], [03:0x00], [04:0x00], [05:0x80], [06:0x07], [07:0xFB], [08:0x03], [09:0x01], [10:0x00], [11:0x07], [12:0x03],
//[00:0x00], [01:0x00], [02:0xFF], [03:0xFF], [04:0x00], [05:0x00],
/*

  00:[0x00]
  01:[0x00]
  02:[0x00]
  03:[0x00]
  04:[0x00]
  05:[0x07]
  06:[0xFB]
  07:[0x03]
  08:[0x01]
  09:[0x00]
  10:[0x30]
  11:[0x00]
  12:[0x08]
  13:[0x00]
  14:[0x3F]
  15:[0x00]


  00:[0x00]
  01:[0x00]
  02:[0xFF]
  03:[0xFF]
  04:[0x00]
  05:[0x00]

*/



INT ReadBlock (BYTE *inbuff)
{
#if(0)
    return 0;
#else

    INT _maxMsgRetries=1;
    INT	retries;
    INT state, result, cs, i;
    INT len=0;
    BYTE extend=0;
    BYTE response;
//    BYTE responseTmp[128];
    //struct timeval st_AckTO = {0, 100000};
    //struct timeval st_DataTO={3, 500000};
    //struct timeval st_TimeOut;
    TickType_t st_AckTO = (100000/1000/portTICK_RATE_MS);//0.1sec (100ms)
    //TickType_t st_AckTO = (2000000/1000/portTICK_RATE_MS);//2sec (2000ms)
    //TickType_t st_AckTO = (3000000/1000/portTICK_RATE_MS);
    TickType_t st_DataTO = (3500000/1000/portTICK_RATE_MS);//3.5secs (3500ms)
    //TickType_t st_DataTO = (10000000/1000/portTICK_RATE_MS);
    TickType_t st_TimeOut;

    retries=_maxMsgRetries;
    state=ST_PREAMBLE;
    g_AckExpired=g_RespExpired=0;
    memcpy (&st_TimeOut, &st_AckTO, sizeof (st_TimeOut));
    
    while((state!=ST_NAK)&&(state!=ST_COMPLETE))
    {
        //terninalPrintf(" ReadBlock start   :  [state = %s]\n", getStateStr(state));
        if(state!=ST_SENDNAK)
        {
            result=ReadData (&response, 1, &st_TimeOut);		//read 1 byte data
            //result=ReadData(responseTmp, sizeof(responseTmp), &st_TimeOut);		//read 1 byte data

            if(result==0)
                return 0;
        }
        //response = responseTmp[0];
        //terninalPrintf(" ReadBlock after I  :  [state = %s, data = 0x%02x]\n", getStateStr(state), response);
        switch(state)
        {
            case ST_SENDNAK:
                if((retries-1)<0)
                    return 0;
                retries--;
                state=ST_PREAMBLE;
                sendNAK();
                break;

            case ST_PREAMBLE:
                if(response==0)		//ACK or NAK first preamble
                    state=ST_START1;
                else
                    state=ST_SENDNAK;
                break;

            case ST_START1:
                if(response==0)		//ACK or NAK first byte of packet start indicator
                    state=ST_START2;
                else
                    state=ST_SENDNAK;
                break;

            case ST_START2:
                if(response==0xff)	//ACK or NAK second byte of packet start indicator
                    state=ST_LEN;
                else if(response==0xcc)	//extend msg second byte of packet start indicator
                    state=ST_LEN_EX1;
                else if(response!=0)	//error happen
                    state=ST_SENDNAK;
                break;

            case ST_LEN:
                extend=0;
                len=response;		//Len = 0 for ACk, Len = 0xFF for NAK
                state=ST_LCS;
                break;

            case ST_LEN_EX1:
                extend=1;
                len=(response<<8)&0xFF00;		//extend msg len consist of two bytes, first Len byte
                state=ST_LEN_EX2;
                break;

            case ST_LEN_EX2:
                len|=response&0xFF;		//extend msg len consist of two bytes, second Len byte
                state=ST_LCS;
                break;

            case ST_LCS:
                if((len==0)&&(response==0xff)) // ACK 
                    state=ST_ACKPOSTAMBLE;		//wait for postamble of the ACK
                else if((len==0xff)&&(response==0)) // NAK 
                    state=ST_NAKPOSTAMBLE;		//wait for postamble of the NAK
                else if((len==0)&&(response==0))
                    state=ST_START2;
                else if((extend==0)&&((len+response)&0xff)==0)
                { // data 
                    cs=0;
                    GetTime (DATA_START);
                    memcpy (&st_TimeOut, &st_DataTO, sizeof (st_TimeOut));
                    state=ST_DATA;
                }
                else if((extend==1)&&((((len>>8)&0xFF)+(len&0xFF)+response)&0xFF)==0)
                {
                    cs=0;
                    GetTime (DATA_START);
                    memcpy (&st_TimeOut, &st_DataTO, sizeof (st_TimeOut));
                    state=ST_DATA;
                }
                else
                    state=ST_SENDNAK;
                break;

            case ST_ACKPOSTAMBLE:
                if(response==0)
                {
                    GetTime (ACK_RECV);
                    memcpy (&st_TimeOut, &st_DataTO, sizeof (st_TimeOut));
                    state=ST_PREAMBLE; // wait for data again 
                }
                else
                    state=ST_SENDNAK;
                break;

            case ST_NAKPOSTAMBLE:
                if(response==0)
                {
                    GetTime ("NAK RECV");
                    state=ST_NAK;
                }
                else
                    state=ST_SENDNAK;
                break;

            case ST_DATA:
                memcpy (&st_TimeOut, &st_DataTO, sizeof (st_TimeOut));
                inbuff[cs++]=response;
                if(cs>=len)
                {
                    if(extend==0)
                        state=ST_DCS;
                    else
                        state=ST_DCS1;
                }
                break;

            case ST_DCS:
                for(i=cs=0; i<len; i++)
                    cs+=inbuff[i];
                if(((cs+response)&0xff)!=0)
                    state=ST_SENDNAK;
                else
                    state=ST_POSTAMBLE;
                break;

            case ST_DCS1:
                cs=response&0xFF;
                state=ST_DCS2;
                break;

            case ST_DCS2:
                cs|=(response<<8)&0xFF00;
                if(LongCS (inbuff, len)!=End2 (cs))
                    state=ST_SENDNAK;
                else
                    state=ST_POSTAMBLE;
                break;

            case ST_POSTAMBLE:
                state=ST_COMPLETE;
                GetTime ("DATA END");
                break;
        }
        //terninalPrintf(" ReadBlock after II :  [state = %s, data = 0x%02x]\n\r\n", getStateStr(state), response);
    }

    if(state==ST_NAK)
        return 0;

    return (len);
#endif
}

INT InitComm (BYTE sPort, INT sBaud)
{
#if(1)
    sprintf(theRWL.OTPFILEDIR, "0:");
    return 0;
#else
    CHAR tmp[FILELEN*2];

    theRWL.CommPort=sPort;
    theRWL.CommBaud=sBaud;
    GetPrivateProfileStringX ("COMM", "PORTTYPE", "S", theRWL.CommPortType, FILELEN, ININAME);
    if(InitCommunication ()>=0)
    {
        //Maintenance file path
        GetPrivateProfileStringX ("METAFILE", "FILEDIR", "/SAMPLE/DOWNLOAD", theRWL.OTPFILEDIR, FILELEN, ININAME);
        GetPrivateProfileStringX ("EXCHANGE", "FILEDIR", "/SAMPLE/UPLOAD", theRWL.UPLOADDIR, FILELEN, ININAME);

        //Keyname under COMM
        GetPrivateProfileStringX ("COMM", "TIME1", "1", tmp, sizeof (tmp), ININAME);
        theRWL.Time1=atoi (tmp);	//number of read retry

        return ERR_NOERR;
    }
    return ERR_FILE_COM;
#endif
}

USHORT End2 (USHORT s)
{
    return ((s>>8)|(s<<8));
}

UINT End4 (UINT i)
{
    return (i<<24)|((i<<8)&0x00FF0000)|((i>>8)&0x0000FF00)|(i>>24);
}

/***********************************************************/
/* Function: GetINT2(BYTE *)                      */
/* Description: determine the value from the input with    */
/*              Big-Endian/Little-Endian conversion        */
/* input: p - pointer to 2 bytes                           */
/* output: 2 byte value read using B-E format if input is  */
/*         in L-E format and vice versa                    */
/***********************************************************/

USHORT GetINT2 (BYTE *p)
{
    // Returns a 2 byte Big-Endian number 
    return p[0]*256+p[1];
}

/***********************************************************/
/* Function: GetINT4(BYTE *)                      */
/* Description: determine the value from the input with    */
/*              Big-Endian/Little-Endian conversion        */
/* input: p - pointer to 4 bytes                           */
/* output: 4 byte value read using B-E format if input is  */
/*         in L-E format and vice versa                    */
/***********************************************************/

UINT GetINT4 (BYTE *p)
{
    // Returns a 4 bytes Big-Endian number 
    return (((p[0]*256)+p[1])*256+p[2])*256+p[3];
}

// extend command message format
USHORT LongCS (BYTE *addr, UINT count)
{
    // Compute Internet Checksum for "count" bytes
     //         beginning at location "addr".  
   
    ULONG sum=0;

    while(count>1)
    {
        //  This is the inner loop 
        sum+=End2 (GetINT2 (addr));
        addr+=2;
        count-=2;
    }
 
    //  Add left-over byte, if any 
    if(count>0)
        sum+=*(BYTE *)addr;

    //  Fold 32-bit sum to 16 bits 
    while(sum>>16)
        sum=(sum&0xffff)+(sum>>16);

    sum=End2 (sum);
    
    
    return (USHORT)~sum;
}



///////////////////////////////////////////////////////////////////////////////
// No need to export this API
///////////////////////////////////////////////////////////////////////////////
/*
static ULONG GetPrivateProfileStringX(
    CHAR * lpAppName,        // section name
    CHAR * lpKeyName,        // key name
    CHAR * lpDefault,        // default string
    CHAR * lpReturnedString,  // destination buffer
    ULONG nSize,              // size of destination buffer
    CHAR * lpFileName        // initialization file name
)
{
#define MAX_INI_SIZE 4096

    FILE  *iniFile;
    CHAR aline[MAX_INI_SIZE];
    ULONG nBytes, fsize;
    CHAR *token;
    CHAR AppName[64], KeyName[64];

    // set default value first
    strcpy(lpReturnedString, lpDefault);
    iniFile = fopen(lpFileName, "r");
    if (iniFile == NULL) {
        CHAR twDir[128];
        strcpy(twDir, "/etc/");
        strcat(twDir, lpFileName);
        iniFile = fopen(twDir, "r");
        if (iniFile == NULL)
            return 0;
    }

    fseek(iniFile, 0, SEEK_END);
    fsize = ftell(iniFile);
    rewind(iniFile);
    if (fsize > MAX_INI_SIZE) {
        fclose(iniFile);
        return 0;
    }
    else {
        nBytes = fread(aline, 1, fsize, iniFile);
        if (nBytes != fsize) {
            fclose(iniFile);
            return 0;
        }
        fclose(iniFile);
    }

    sprintf(AppName, "[%s]", lpAppName);
    sprintf(KeyName, "%s=", lpKeyName);
    token = strtok(aline, "\n");
    while (token != NULL) {
        if (strncmp(token, AppName, strlen(AppName)) == 0) {
            // find app, then find key
            token = strtok(NULL, "\n");
            while (token != NULL) {
                if (strncmp(token, KeyName, strlen(KeyName)) == 0) {
                    // Find key
                    if (strlen(token) - 1 - strlen(KeyName) > nSize)
                        nBytes = nSize;
                    else
                        nBytes = strlen(token) - 1 - strlen(KeyName);
                    strncpy(lpReturnedString, token + strlen(KeyName), nBytes + 1);
                    lpReturnedString[nBytes + 1] = 0;
                    return 0;
                }
                token = strtok(NULL, "\n");
            }
        }
        token = strtok(NULL, "\n");
    }

    return 0;

}
*/




/*
void DbgPrint (const CHAR *fmt, ...)
{

    printf (" DBG ");
    va_list ap;
    va_start (ap, fmt);
    vprintf (fmt, ap);
    va_end (ap);
}
*/
void DbgDump (BYTE *bBuf, INT iLen)
//void DbgDump (uint8_t *bBuf, INT iLen)
{
    INT i;
    BYTE u8_LineFeed=20;
    //uint8_t u8_LineFeed=20;
    

    sysprintf (" DBG (%d) ", iLen);

    //fflush(stdout);

    if(iLen<1)
        return;

    if(iLen>50)
        u8_LineFeed=30;

    if(iLen>100)
        u8_LineFeed=50;

    for(i=0; i<iLen; i++)
    {
        if(i%u8_LineFeed==0)
            sysprintf ("\n");
        sysprintf ("%02X ", bBuf[i]);
    }
    sysprintf ("\n");
}

void DbgCmdDump (BYTE *bBuf, INT iLen)
{
    UINT i;
    BYTE u8_LineFeed=20;
    UINT u32_CmdHdr;
    UINT u32_DumpLen;

    if(iLen>255)
        u32_CmdHdr=6;
    else
        u32_CmdHdr=5;

    iLen-=u32_CmdHdr;

    //limit the data dump length
    if(iLen>200)
        u32_DumpLen=200;
    else
        u32_DumpLen=iLen;

    //by sam
    printf ("\nCMD Header(%d) >>> ", u32_CmdHdr);
    for(i=0; i<u32_CmdHdr; i++)
    {
        if(i%u8_LineFeed==0)
            printf ("\n");
        printf ("%02X ", bBuf[i]);
    }
    //by sam
    
    printf ("\nCMD (%d) >>> ", iLen);

    //fflush(stdout);

    if(iLen<1)
        return;

    if(iLen>50)
        u8_LineFeed=30;

    if(iLen>100)
        u8_LineFeed=50;

    for(i=0; i<u32_DumpLen; i++)
    {
        if(i%u8_LineFeed==0)
            printf ("\n");
        printf ("%02X ", bBuf[i+u32_CmdHdr]);
    }
    printf ("\n");
}

void DbgRspDump (BYTE *bBuf, INT iLen)
{
    UINT i;
    BYTE u8_LineFeed=20;
    UINT u32_DumpLen=0;

    //limit the data dump length
    if(iLen>400)
        u32_DumpLen=400;
    else
        u32_DumpLen=iLen;

    printf ("\nRSP (%d) <<< ", iLen);

    //fflush(stdout);

    if(iLen<1)
        return;

    if(iLen>50)
        u8_LineFeed=30;

    if(iLen>100)
        u8_LineFeed=50;

    for(i=0; i<u32_DumpLen; i++)
    {
        if(i%u8_LineFeed==0)
            printf ("\n");
        printf ("%02X ", bBuf[i]);
    }
    printf ("\n");
}

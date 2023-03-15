/*****************************************************************************/
/* File Name   : rwlfunc.c                                                   */
/* Author      : Copyright OCL                                        			 */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : To Demonstrate the use of the Octopus Reader-writer Library */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/
#include <stdio.h>
//#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
//#include <unistd.h>
#include <limits.h>
#include "rwl.h"
#include "rwl_common.h"
#include "csdes.h"
#include "csrw.h"
#include "fileagent.h"

void    terninalPrintf (char* pcStr, ...);
#define printf terninalPrintf


#define RW_RESULT	(g_inbuff[1])

static BOOL getDataFileCallback(char* dir, char* filename, int fileLen, void* para1, void* para2, void* para3, void* para4, void* para5)
{
    FileRec* CurrentFI = para1;
    CHAR* FN = para2;
    UINT ver = *(UINT*)para3;
    CHAR* FILEDIR = para4;
    
    printf(" - WARNING [getDataFileCallback in Octopus]-> [%s %s (len=%d)] : FN = [%s], ver = %d, FILEDIR = [%s]!!\n", dir, filename, fileLen, FN, ver, FILEDIR);  
    
    UINT R=0;
    UINT NewVer=0;
    UINT FNlen;
    CHAR cVer[10];
    
    NewVer = ver;
    FNlen = strlen(FN);    
    
    if (strncmp(filename, FN, FNlen)==0) 
    {
        strcpy(cVer, strchr(filename,'-')+1);
        R = (UINT) atoi (cVer);
        if (R > NewVer)
        {            
            NewVer = R;
            printf("   - NewVer = %d \n", NewVer);
            
            memset(CurrentFI->FN,0x00,sizeof(CurrentFI->FN));
            //strcpy(CurrentFI->FN, FILEDIR);
            //strcat(CurrentFI->FN, "/");
            strcat(CurrentFI->FN, filename);
            CurrentFI->FileOffset = 0;
            CurrentFI->FileLen = fileLen;
            
            printf("   - CurrentFI->FN = [%s], CurrentFI->FileLen = %d \n", CurrentFI->FN, CurrentFI->FileLen);
        }
    }
    
    if (NewVer > ver)
    {
        printf("   - Return value = %d \n", NewVer);
        //return NewVer;
    }
    else
    {
        printf("   - Return value = %d \n", -1);
        //return -1;
    }
    
    return TRUE;    
}


static INT FindOpFile (FileRec *CurrentFI, CHAR *FN, UINT ver, CHAR *FILEDIR)       /* kelvinli */
{    
    //struct dirent **FindFileData;
    #if(0)
    #if(0)
    UINT R=0;
    UINT NewVer=0;
    UINT FNlen;
    CHAR cVer[10];
    INT  n;
    INT  cnt;

    NewVer = ver;
    FNlen = strlen(FN);
    #endif
    //BOOL FileAgentGetList(StorageType storageType, char* dir, char* extensionName, char* excludeFileName, fileAgentFatfsListCallback callback, void* para1, void* para2, void* para3, void* para4, void* para5);
    if(FileAgentGetList(FILE_AGENT_STORAGE_TYPE_FATFS, FILEDIR, "*.*", NULL, getDataFileCallback, CurrentFI, FN, &ver, FILEDIR))
    {
        printf(" \r\n--- FindOpFile : CurrentFI->FN = [%s], CurrentFI->FileLen = %d \n", CurrentFI->FN, CurrentFI->FileLen);
        return 1;
    }
    else
    {
        return -1;
    }
    #if(0)
    n = scandir(FILEDIR, &FindFileData, 0, alphasort);
    if (n>=0) {
        INT index_max = -1;
        for(cnt=n-1; cnt >= 0; --cnt) {
            if (strncmp(FindFileData[cnt]->d_name, FN, FNlen)==0) {
                strcpy(cVer, strchr(FindFileData[cnt]->d_name,'-')+1);
                R = (UINT) atoi (cVer);
                if (R > NewVer){
                    NewVer = R;
                    index_max = cnt;
                }
            }
        }

        if (index_max > -1) {
            FILE *fp = NULL;
            memset(CurrentFI->FN,0x00,sizeof(CurrentFI->FN));
            strcpy(CurrentFI->FN, FILEDIR);
            strcat(CurrentFI->FN, "/");
            strcat(CurrentFI->FN, FindFileData[index_max]->d_name);
            CurrentFI->FileOffset = 0;
            fp = fopen(CurrentFI->FN, "rb");
            fseek(fp, 0, SEEK_END);
            CurrentFI->FileLen = ftell(fp);
            fclose(fp);
        }
        // free memory allocated via malloc() in scandir()
        while (n--)
            free(FindFileData[n]);
        free(FindFileData);
    }

    if (NewVer > ver){
        return NewVer;
    }else{
        return -1;
    }
    
    return -1;
    #endif
    #else
    return -1;
    #endif
}

/*************************************
        XFile
 *************************************/
INT XFileInitRecv (CHAR *XFileName, UINT u32_SegmentSize)
{
    CmdXFileInitRecv (XFileName, u32_SegmentSize);
    terninalPrintf ("CmdXFileInitRecv g_len %d, g_inbuff[0] %d,  RW_RESULT %d\n", g_len, g_inbuff[0], RW_RESULT);
    if((g_len>0)&&(g_inbuff[0]==CSCMD_INITRECV+1)&&(RW_RESULT==0))
        return ERR_NOERR;
    else
        return ERR_ERRRESP;
}

INT XFileContRecv (CHAR *XFileName, UINT u32_SegmentSize)
{
    CmdXFileContRecv (XFileName, u32_SegmentSize);
    terninalPrintf ("CmdXFileContRecv g_len %d, g_inbuff[0] %d,  RW_RESULT %d\n", g_len, g_inbuff[0], RW_RESULT);
    if((g_len>0)&&(g_inbuff[0]==CSCMD_CONTRECV+1)&&(RW_RESULT==0))
        return ERR_NOERR;
    else
        return ERR_ERRRESP;
}

/*************************************
        TimeVer
 *************************************/
INT TimeVer (BYTE *Ver, UINT * u32_time)
{
    stDevVer *V=(stDevVer *)Ver;

    CmdTimeVer (u32_time);

    if((g_len>0)&&(g_inbuff[0]==CSCMD_TIMEVER+1)&&(RW_RESULT==0))
    {
        V->DevID=GETLONG (g_inbuff+2); 					/* Device ID */
        V->OperID=GETLONG (g_inbuff+6);					/* Operator ID */
        V->DevTime=GETLONG (g_inbuff+10);				/* Device Time */
        V->CompID=GETWORD (g_inbuff+14);					/* Company ID */
        V->KeyVer=GETLONG (g_inbuff+16);					/* Key Version */
        V->EODVer=GETLONG (g_inbuff+20);					/* EOD Version */
        V->BLVer=GETLONG (g_inbuff+24);					/* Blacklist Version */
        V->FIRMVer=GETLONG (g_inbuff+28);				/* Firmware Version */
        V->CCHSVer=GETLONG (g_inbuff+32);				/* CCHS Version */
        V->CSSer=GETLONG (g_inbuff+36);					/* CS Serial No, Loc ID*/
        V->IntBLVer=GETLONG (g_inbuff+40);				//Interim Blacklist Version
        V->FuncBLVer=GETLONG (g_inbuff+44);			//functional blacklist version
        V->AlertMsgVer=GETLONG (g_inbuff+48);	//Alert msg file version
        V->RWKeyVer=GETLONG (g_inbuff+52);			//rwkey version
        V->OTPVer=GETLONG (g_inbuff+56);				//Last receive OTP file version
        memcpy (V->Reserved, g_inbuff+60, sizeof (V->Reserved));//Last receive OTP file version

        theRWL.CompID=V->CompID;		//save for OTP file transfer
        return ERR_NOERR;
    }
    else
        return ERR_ERRRESP;
}

/*************************************
        Housekeep
 *************************************/
INT HouseKeepingInitTran (void)
{
    CHAR 	FindFN[1024];
    FileRec FI;

    sprintf (FindFN, "OTP.%04d-", theRWL.CompID);
    terninalPrintf ("Finding OTP file %s\n", FindFN);

    if(FindOpFile (&FI, FindFN, 0, theRWL.OTPFILEDIR)!=-1)
    {
        sprintf (FindFN, "%s", FI.FN);
        terninalPrintf ("Sending OTP file, filename:%s%s\n", theRWL.OTPFILEDIR, FindFN);

        CmdMetaFileInitSend (FindFN, theRWL.OTPFILEDIR, FI.FileLen);
        if((g_len>0)&&(g_inbuff[0]==CSCMD_INITTRAN+1)&&(RW_RESULT==0))
            return ERR_NOERR;
        else
            return ERR_ERRRESP;
    }
    else
        return ERR_FILE_OPEN; // file open error
}

INT HouseKeepingContTran (void)
{
    CmdMetaFileContSend ();
    if((g_len>0)&&(g_inbuff[0]==CSCMD_CONTTRAN+1)&&(RW_RESULT==0))
        return ERR_NOERR;
    else
        return ERR_ERRRESP;
}

/*************************************
        Write Location ID
 *************************************/
INT WriteID (UINT CSID)
{
    CmdWriteID (CSID);

    if((g_len>0)&&(g_inbuff[0]==CSCMD_WRITEID+1)&&(RW_RESULT==0))
        return ERR_NOERR;
    else
        return ERR_ERRRESP;
}

/*************************************
        Antenna Off
 *************************************/
INT AntennaOff (void)
{
    CmdAntennaOff ();
    if((g_len>0)&&(g_inbuff[0]==CSCMD_ANTENNA_CTRL+1)&&(RW_RESULT==0))
        return ERR_NOERR;
    else
        return ERR_ERRRESP;
}

/*************************************
        End Session
 *************************************/
INT EndSession (void)
{
    CmdEndSession ();
    if((g_len>0)&&(g_inbuff[0]==CSCMD_END_SESSION+1)&&(RW_RESULT==0))
        return ERR_NOERR;
    else
        return ERR_ERRRESP;
}

/*************************************
        Auth 1 + Auth 2
 *************************************/
INT Authenticate (void)
{
    BYTE u8_stage;
    UINT	u32_ret;
    u32_ret=CmdAuthenticate (&u8_stage);
    if(u32_ret)
        return ERR_ERRRESP;

    if(u8_stage==CSCMD_AUTH1)
    {
        if((g_len>0)&&(g_inbuff[0]==CSCMD_AUTH1+1)&&(RW_RESULT==0))
            return ERR_NOERR;
    }
    if(u8_stage==CSCMD_AUTH2)
    {
        if((g_len>0)&&(g_inbuff[0]==CSCMD_AUTH2+1)&&(RW_RESULT==0))
            return ERR_NOERR;
    }
    return ERR_ERRRESP;
}

/*************************************
        POLL+Deduct
 *************************************/
INT PollDeduct (BYTE bTimeout, INT TxnAmt, const BYTE *AddInfo, BYTE bAlertMsgFmt, Rsp_PollDeductStl *cardInfo)
{
    Rsp_PollDeductStl pollDeductStlResp;

    CmdPollDeduct (bTimeout, TxnAmt, AddInfo, bAlertMsgFmt);
    if(g_len>0)
        memcpy (&pollDeductStlResp, g_inbuff, CLAMP (g_len, (INT)sizeof (Rsp_PollDeductStl)));

    memcpy (cardInfo, &pollDeductStlResp, sizeof (Rsp_PollDeductStl));

    if((g_len>0)&&(g_inbuff[0]==CSCMD_POLLDEDUCT_STL+1)&&(RW_RESULT==0))
    {
        return ERR_NOERR;
    }
    else
    {
        //for power up time measurement
        if((g_len>0)&&(g_inbuff[0]==CSCMD_POLLDEDUCT_STL+1)&&(RW_RESULT==ERR_NOFUND))
            return ERR_NOFUND;
    }
    return ERR_ERRRESP;
}

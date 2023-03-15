/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"
#include "pct08cmdlib.h"
#include "pct08drv.h"
#include "ff.h"
#include "fileagent.h"
/*
Send:   56 00 36 01 00
ACK:    76 00 36 00 00

Send:   56 00 34 01 00
ACK:    76 00 34 00 04 00 00 0E 4C
Send:   56 00 32 0C 00 0A 00 00 00 00 00 00 0E 4C 00 FF
 */
#define RECEIVE_PHOTO_BUFF_LEN  1024 * 1024
#define RECEIVE_TEMP_BUFF_LEN  1024
#define RECEIVE_DATA_BUFF_LEN  16

static uint8_t cmdReadVerInfo[] = {0x56, 0x00, 0x26, 0x00};
static uint8_t cmdReadVerInfoAck[] = {0x76, 0x00, 0x26, 0x00};

static uint8_t cmdTake[] = {0x56, 0x00, 0x36, 0x01, 0x00};
static uint8_t cmdTakeAck[] = {0x76, 0x00, 0x36, 0x00, 0x00};

static uint8_t cmdPhotoInfo[] = {0x56, 0x00, 0x34, 0x01, 0x00};
static uint8_t cmdPhotoInfoAck[] = {0x76, 0x00, 0x34, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
static uint8_t cmdGetPhoto[] = {0x56, 0x00, 0x32, 0x0c, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x4C, 0x00, 0xFF};

static uint8_t cmdGetPhotoAck[] = {0x76, 0x00, 0x32, 0x00, 0x00};

//static uint8_t cmdCleanBuff[] = {0x56, 0x00, 0x36, 0x01, 0x02};
static uint8_t cmdCleanBuff[] = {0x56, 0x00, 0x36, 0x01, 0x03};
static uint8_t cmdCleanBuffAck[] = {0x76, 0x00, 0x36, 0x00, 0x00};

static uint8_t receiveData[RECEIVE_DATA_BUFF_LEN];
static uint8_t receivePhoto[RECEIVE_PHOTO_BUFF_LEN];
static int receiveDataIndex = 0;
static uint8_t receiveDataTmp[RECEIVE_TEMP_BUFF_LEN];
static int photoSize = 0;
static BOOL pct08ActionCmd(uint8_t* buff, int buffLen, int* receiveLen, uint32_t waitTime, uint8_t* cmd, int cmdLen, uint8_t* cmdAck, int cmdAckRealLen, int cmdAckCompareLen)
{
    int receiveIndex = 0;
    PCT08FlushTxRx();
    receiveIndex = 0;
    *receiveLen = 0;
    if(PCT08Write(cmd, cmdLen) == cmdLen)
    {
        int counter = waitTime/(100/portTICK_RATE_MS);
        while(counter != 0)
        {
            int reval = PCT08Read(receiveDataTmp, sizeof(receiveDataTmp));
            if(reval > 0)
            {
                if(buffLen < receiveIndex + reval)
                {
                    sysprintf("Buffer exceed:%d, %d\r\n", buffLen, receiveIndex + reval);
                    return FALSE;
                }
                memcpy(buff + receiveIndex, receiveDataTmp, reval);
                receiveIndex = receiveIndex + reval;
                //sysprintf("Get data %d (%d) \r\n", reval, receiveIndex);
                sysprintf("-");
                if(receiveIndex >= cmdAckRealLen)
                {
                    if(cmdAckCompareLen>0)
                    {
                        if(memcmp(buff, cmdAck, cmdAckCompareLen) == 0)
                        {
                            *receiveLen = receiveIndex;
                            sysprintf("Get Ack %d\r\n", *receiveLen);
                            
                            return TRUE;
                        }
                        else
                        {
                            sysprintf("Get Ack cmp error\r\n");
                            return FALSE;
                        }
                    }
                    else
                    {
                        
                        *receiveLen = receiveIndex;
                        sysprintf("Get Data %d bytes\r\n", *receiveLen);
                        return TRUE;
                    }
                }
            }
            //else
            {
                //sysprintf("counter = %d\r\n", counter);
                vTaskDelay(100/portTICK_RATE_MS);
                counter--;
            }
            
        }
    }
    sysprintf("Get Ack ERROR: receiveDataIndex = %d, cmdAckRealLen = %d\r\n", receiveIndex, cmdAckRealLen);
    return FALSE;
}

BOOL PCT08TakePhoto(uint8_t** photoPr, int* photoLen, StorageType type, char* dir, char* fileName)
{
    BOOL reVal = FALSE;
    *photoLen = 0;
    PCT08SetPower(TRUE);
    vTaskDelay(1000/portTICK_RATE_MS);
    if(pct08ActionCmd(receiveData, sizeof(receiveData), &receiveDataIndex, 500, cmdTake, sizeof(cmdTake), cmdTakeAck, sizeof(cmdTakeAck), sizeof(cmdTakeAck)))
    {
        if(pct08ActionCmd(receiveData, sizeof(receiveData), &receiveDataIndex, 500, cmdPhotoInfo, sizeof(cmdPhotoInfo), cmdPhotoInfoAck, sizeof(cmdPhotoInfoAck), sizeof(cmdPhotoInfoAck)-2))
        {
            photoSize = (receiveData[sizeof(cmdPhotoInfoAck) - 1]&0xff) | ((receiveData[sizeof(cmdPhotoInfoAck) - 2] << 8)&0xff00);
            sysprintf("PCT08TakePhoto : photoSize = %d (0x%02x, 0x%02x)\r\n", photoSize, receiveData[sizeof(cmdPhotoInfoAck) - 2], receiveData[sizeof(cmdPhotoInfoAck) - 1]);
            cmdGetPhoto[12] = receiveData[sizeof(cmdPhotoInfoAck) - 2];
            cmdGetPhoto[13] = receiveData[sizeof(cmdPhotoInfoAck) - 1];
            
            if(pct08ActionCmd(receivePhoto, sizeof(receivePhoto), &receiveDataIndex, 4000, cmdGetPhoto, sizeof(cmdGetPhoto), NULL, photoSize + 10, 0))
            {                
                if(memcmp(receivePhoto, cmdGetPhotoAck, sizeof(cmdGetPhotoAck)) || memcmp(receivePhoto + receiveDataIndex - 5, cmdGetPhotoAck, sizeof(cmdGetPhotoAck)))
                {
                    sysprintf("PCT08TakePhoto ERROR: Header error\r\n");
                }
                else
                {
                    sysprintf(" ~~~ PCT08TakePhoto OK (photoSize = %d): receiveDataIndex = %d (0x%02x, 0x%02x : 0x%02x, 0x%02x)\r\n", 
                            photoSize, receiveDataIndex, receivePhoto[5], receivePhoto[6], receivePhoto[receiveDataIndex-7], receivePhoto[receiveDataIndex-6]);
                    *photoLen = photoSize;
                    *photoPr = (receivePhoto + 5);
                    #if(0)
                    {
                        char name[_MAX_LFN];
                        sprintf(name, "%s%s", dir, fileName);
                        saveToFile(receivePhoto + 5, receiveDataIndex - 10, name);
                    }
                    reVal = TRUE;
                    #else
                    if(FileAgentAddData(type, dir, fileName, receivePhoto + 5, receiveDataIndex - 10, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, FALSE, TRUE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
                    {
                        reVal = TRUE;
                    }
                    #endif
                    if(pct08ActionCmd(receiveData, sizeof(receiveData), &receiveDataIndex, 500, cmdCleanBuff, sizeof(cmdCleanBuff), cmdCleanBuffAck, sizeof(cmdCleanBuffAck), sizeof(cmdCleanBuffAck)))
                    {
                        sysprintf("PCT08TakePhoto Clean OK\r\n");
                    } 
                    else
                    {
                        sysprintf("PCT08TakePhoto Clean ERROR\r\n");
                    }
                }
            }
            else
            {
                sysprintf("cmdGetPhoto ERROR:\r\n");
            }
        }
        else
        {
            sysprintf("cmdPhotoInfo ERROR:\r\n");
        }
    }
    else
    {
        sysprintf("cmdTake ERROR:\r\n");
    }
    PCT08SetPower(FALSE);
    return reVal;
}

BOOL PCT08ReadVerInfo(void)
{
    BOOL reVal = FALSE;
    PCT08SetPower(TRUE);
    vTaskDelay(1000/portTICK_RATE_MS);
    if(pct08ActionCmd(receiveDataTmp, sizeof(receiveDataTmp), &receiveDataIndex, 500, cmdReadVerInfo, sizeof(cmdReadVerInfo), cmdReadVerInfoAck, sizeof(cmdReadVerInfoAck), sizeof(cmdReadVerInfoAck)))
    {
        reVal = TRUE;
    }
    else
    {
        //sysprintf("cmdReadVerInfo ERROR:\r\n");
    }
    PCT08SetPower(FALSE);
    return reVal;
}


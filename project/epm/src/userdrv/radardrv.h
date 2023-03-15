/**************************************************************************//**
* @file     radardrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __RADAR_DRV_H__
#define __RADAR_DRV_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//59 59 10 00 D0 0F 00 09 AA
//const int HEADER=0x59;//frame header of data packag
//uart[0]=HEADER;
//uart[1]=HEADER;
//check=uart[0]+uart[1]+  uart[2]+uart[3]  +  uart[4]+uart[5]  +uart[6]+uart[7];
//uart[8]==(check&0xff))//verify the received data as per protocol
//dist=uart[2]+uart[3]*256;//calculate distance value
//strength=uart[4]+uart[5]*256;//calculate signal strength value
#define RADAR_HEADER    0x7A
#define RADAR_HEADER_2    0xA7
    
#define RADAR_END    0xD3
#define RADAR_END_2    0x3D    

#define S1_DECODE_HEADER            1
#define S2_DECODE_DATALEN           2
#define S3_DECODE_CMD               3
#define S4_DECODE_CHECKSUM          4
#define S5_DECODE_DATABODY          5

#define RADAR_RETURN_BLANK          0x50505050
#define RADAR_WAIT_NEXTDATA         0x30
#define RADAR_RETURN_FAIL           0xFFFFFFFF  
#define UART_FORMAT_ERR             0x10
    
#define RING_BUFFER_SIZE            1024
#define RING_FIFO_DEVICE            2
    
#pragma pack(1)
typedef struct radarPacket_t {
    uint8_t header1;
    uint8_t header2;
    uint8_t len[2];
    uint8_t cmdid;
    uint8_t material;
    uint8_t object;
    uint8_t lidarstatus;
    uint8_t lidarCalibrateStatus;
    uint8_t lidarDistValue[2];
    uint8_t senseResult;
    uint8_t lidarRecentSenseDist[2];
    uint8_t radarRecentSenseDist[2];
    uint8_t reserve[4];
    uint16_t checkSum;
    uint8_t end1;
    uint8_t end2;
} radarPacket;

/*
typedef struct radarPacket_t {
    uint8_t header1;
    uint8_t header2;
    uint8_t len[2];
    uint8_t cmdid;
    uint8_t material;
    uint8_t object;
    uint8_t lidarstatus;
    uint8_t lidarCalibrateStatus;
    uint8_t lidarDistValue[2];
    uint8_t reserve[2];
    uint16_t checkSum;
    uint8_t end1;
    uint8_t end2;
} radarPacket; */

typedef struct radarCalibrate_t {
    uint8_t header1;
    uint8_t header2;
    uint8_t len[2];
    uint8_t cmdid;
    uint8_t value;
    uint16_t checkSum;
    uint8_t end1;
    uint8_t end2;
} radarCalibrate;



typedef struct radarVersion_t {
    uint8_t header1;
    uint8_t header2;
    uint8_t len[2];
    uint8_t cmdid;
    uint8_t code1;
    uint8_t code2;
    uint8_t code3;
    uint8_t code4;
    uint16_t checkSum;
    uint8_t end1;
    uint8_t end2;
} radarVersion;

typedef struct RINGFIFO{
    char Buff[RING_BUFFER_SIZE];
    short SaveIndex;
    short ReadIndex;
} RingfifoStruct;


#pragma pack()
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL RadarDrvInit(void);    
BOOL RadarDrvInitEx(int radarIndex ,int BaudRate);
BOOL RadarDrvInitBurning(int radarIndex);
int RadarCheckFeature(int radarIndex, BOOL* changeFlag, void* para1, void* para2, void* para3);
void RadarSetPowerStatus(int radarIndex, BOOL flag);
void RadarSetPower(BOOL flag);
void RadarLogCheckStr(int index);
void RadarSetCheckDistExceedMaxMin(int index, int exceed, int max, int min);
void RadarSetCheckPowerMin(int index, int min);
void RadarSetStableCounter(int radarIndex, int checkCounter);
void RadarSetStableDistRange(int radarIndex, int checkCounter);
void RadarSetVacuumCounter(int radarIndex, int checkCounter);
void RadarSetLowPowerStableCounter(int radarIndex, int checkCounter);
void RadarSetStartCalibrate(int radarIndex, BOOL flag);
int RadarCalibrate(int radarIndex, BOOL* changeFlag,int* dist1,int* dist2,void* para1, void* para2, void* para3);
int RadarReadDistValue(int index,int* dist);
void RadarSetQueryVersion(int radarIndex, BOOL flag);
void ReadRadarVersion(int radarIndex, uint8_t* code1,uint8_t* code2,uint8_t* code3,uint8_t* code4);
int ReadRadarVersionString(int radarIndex, char* radarVerStr);

BOOL RadarFirstOTA(int radarIndex, int BufferLen);
BOOL RadarOTA(int radarIndex, uint8_t* ReadBuffer, int BufferIndex, int ReadBufferLen);

BOOL RadarRecentDistValue(int radarIndex, uint16_t* lidarDist,  uint16_t* radarDist);


BOOL Decode_StateMachine(int radarIndex ,uint8_t radarCmd,int readBuffSize, uint8_t* radarBuff, uint8_t* radarData);

//int newRadarResult(int radarIndex, BOOL* changeFlag, void* para1, void* para2, void* para3);
int newRadarResult(int radarIndex, uint8_t radarCmd, uint8_t* CmdBuff, uint8_t* radarData);

int newRadarResultElite(int radarIndex, uint8_t radarCmd, uint8_t* CmdBuff, uint8_t* radarData);

int newRadarResultPure(int radarIndex, uint8_t* CmdBuff, int CmdBuffLen, uint8_t* radarData, int* radarDataLen,int WaitTime);
void newRadarFlush(int radarIndex);

void RadarTaskCreate(void);
void RadarTaskDelete(void);

#ifdef __cplusplus
}
#endif

#endif //__RADAR_DRV_H__

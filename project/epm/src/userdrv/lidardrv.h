/**************************************************************************//**
* @file     lidardrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __LIDAR_DRV_H__
#define __LIDAR_DRV_H__

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
#define LIDAR_HEADER    0x59
#pragma pack(1)
typedef struct lidarPacket_t {
    uint8_t header1;
    uint8_t header2;
    uint8_t dist[2];
    uint8_t strength[2];
    uint8_t reserve[2];
    uint8_t checkSum;
} lidarPacket;
#pragma pack()
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL LidarDrvInit(void);    
int LidarCheckFeature(int radarIndex, BOOL* changeFlag, void* para1, void* para2, void* para3);
void LidarSetPowerStatus(int lidarIndex, BOOL flag);
void LidarSetPower(BOOL flag);

void LidarSetCheckDistExceedMaxMin(int index, int exceed, int max, int min);
void LidarSetCheckPowerMin(int index, int min);
void LidarSetStableCounter(int lidarIndex, int checkCounter);
void LidarSetStableDistRange(int lidarIndex, int checkCounter);
void LidarSetVacuumCounter(int lidarIndex, int checkCounter);
#ifdef __cplusplus
}
#endif

#endif //__LIDAR_DRV_H__

/**************************************************************************//**
* @file     ledcmdlib.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __LED_CMD_LIB_H__
#define __LED_CMD_LIB_H__




#ifdef EPM_PROJECT
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>
    #include "nuc970.h"
#else
    #include <stdio.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define TOTAL_BAY_LIGHT_NUM     8
    
#define NOT_FLASHING_OFF        0x00
#define NOT_FLASHING_ON         0xff
#define LED_ON_TIME_10MS        0x01
#define LED_PERIOD_01MS         0x01
    
#define LIGHT_COLOR_OFF         0
#define LIGHT_COLOR_GREEN       1
#define LIGHT_COLOR_RED         2
#define LIGHT_COLOR_YELLOW      3
#define LIGHT_COLOR_IGNORE	    0xff

#define COMMAND_ERROR            0
#define COMMAND_LENGTH_ERROR     (-10)
#define COMMAND_FORMAT_ERROR     (-11)

#define COMMAND_SUCCESSFUL      1
#define COMMAND_FIAL            (-1)

uint8_t Bay_light_Command(char* Out_command, uint8_t Buff_LEN,uint8_t Frequency, uint8_t Period);
uint8_t Alive_State_light_Command(char* Out_command, uint8_t Buff_LEN, uint8_t Frequency, uint8_t Period);
uint8_t State_light_Command(char* Out_command, uint8_t Buff_LEN, uint8_t Frequency, uint8_t Period);
uint8_t Light_Color_Command(char* Out_command, uint8_t Buff_LEN, uint8_t Bay[], uint8_t state);
uint8_t HeartBeatTimeSet(char* Out_command, uint8_t Buff_LEN, uint8_t DeathMin, uint8_t DeathSec);
uint8_t CalibrationSet(char* Out_command, uint8_t Buff_LEN);
uint8_t CollisionSet(char* Out_command, uint8_t Buff_LEN,uint8_t bias_degree,uint8_t strength_X,uint8_t strength_Y,uint8_t strength_Z);
uint8_t VersionQuery(char* Out_command, uint8_t Buff_LEN);
uint8_t CollisionClean(char* Out_command, uint8_t Buff_LEN);

int8_t FactoryTest(char* Results_Command , uint8_t Command_LEN);
//int8_t RUN_Results(char* Results_Command , uint8_t Len, short* Command_ID);
int8_t RUN_Results(char* Results_Command , uint8_t Command_LEN, short* Command_ID, int* head);
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
//BOOL BaseDrvInit(void);

#ifdef __cplusplus
}
#endif

#endif //__LED_CMD_LIB_H__

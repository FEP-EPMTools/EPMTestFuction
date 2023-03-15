/**************************************************************************//**
* @file     ledcmdlib.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef EPM_PROJECT
#else
    #include "M051Series.h"
#endif

/* Scheduler includes. */
#include "ledcmdlib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* Exported Functions                                */
/*-----------------------------------------*/

uint8_t Bay_light_Command(char* Out_command, uint8_t Buff_LEN,uint8_t Frequency, uint8_t Period)
{
    uint8_t TX_Check_Sum;
    if((Buff_LEN < 9) | (Period == 0))
            return COMMAND_ERROR; 
    if((Frequency > Period*10) && (Frequency != 0xFF))
            return COMMAND_ERROR; 
        
    TX_Check_Sum = 0x09+0x01+Frequency+Period;

    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x09;
    Out_command[3] = 0x01;
    Out_command[4] = Frequency;
    Out_command[5] = Period;
    Out_command[6] = TX_Check_Sum;
    Out_command[7] = 0xd3;
    Out_command[8] = 0x3d;
    return 9;
    
}

uint8_t Alive_State_light_Command(char* Out_command, uint8_t Buff_LEN, uint8_t Frequency, uint8_t Period)
{   
    uint8_t TX_Check_Sum;
    if((Buff_LEN < 9) | (Period == 0))
            return COMMAND_ERROR; 
    if((Frequency > Period*10) && (Frequency != 0xFF))
            return COMMAND_ERROR; 
        
    TX_Check_Sum = 0x09+0x12+Frequency+Period;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x09;
    Out_command[3] = 0x12;
    Out_command[4] = Frequency;
    Out_command[5] = Period;
    Out_command[6] = TX_Check_Sum;
    Out_command[7] = 0xd3;
    Out_command[8] = 0x3d;
    return 9;
}

uint8_t State_light_Command(char* Out_command, uint8_t Buff_LEN, uint8_t Frequency, uint8_t Period)
{   
    uint8_t TX_Check_Sum;
    if((Buff_LEN < 9) | (Period == 0))
            return COMMAND_ERROR; 
    if((Frequency > Period*10) && (Frequency != 0xFF))
            return COMMAND_ERROR; 
        
    TX_Check_Sum = 0x09+0x02+Frequency+Period;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x09;
    Out_command[3] = 0x02;
    Out_command[4] = Frequency;
    Out_command[5] = Period;
    Out_command[6] = TX_Check_Sum;
    Out_command[7] = 0xd3;
    Out_command[8] = 0x3d;
    return 9;
}
uint8_t Light_Color_Command(char* Out_command, uint8_t Buff_LEN, uint8_t Bay[], uint8_t state)
{       
    uint8_t Bay1_4, Bay5_8, BayState ,TX_Check_Sum;
    uint8_t data_check;
    for(data_check=0;data_check<=7;data_check++)
    {
        if(!((Bay[data_check] == 0) | (Bay[data_check] == 1) | (Bay[data_check] == 2)))
            return COMMAND_ERROR; //Light color erro
    }
    //if(!((state == 0x00) | (state == 0x3)))
     //       return COMMAND_ERROR; //Light color erro
                
    if(Buff_LEN < 10)
            return COMMAND_ERROR;
    Bay1_4 = Bay[0]+ (Bay[1] << 2) + (Bay[2] << 4) + (Bay[3] << 6) ;
    Bay5_8 = Bay[4]+ (Bay[5] << 2) + (Bay[6] << 4) + (Bay[7] << 6) ;
    BayState = state;
    TX_Check_Sum = 0x0a + 0x05 + Bay1_4 + Bay5_8 + BayState;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x0a;
    Out_command[3] = 0x05;
    Out_command[4] = Bay1_4;
    Out_command[5] = Bay5_8;
    Out_command[6] = BayState;  
    Out_command[7] = TX_Check_Sum;
    Out_command[8] = 0xd3;
    Out_command[9] = 0x3d;
    return 10;
}
uint8_t HeartBeatTimeSet(char* Out_command, uint8_t Buff_LEN, uint8_t DeathMin, uint8_t DeathSec)
{
    uint8_t TimeBig,TimeLittle,TX_Check_Sum;
    uint16_t TotalSec;
    if(Buff_LEN < 9)
        return COMMAND_ERROR;
    if((DeathMin >= 27) && (DeathSec > 57))
        return COMMAND_ERROR;
    if(DeathSec > 59)
        return COMMAND_ERROR;
    if(DeathMin > 27)
        return COMMAND_ERROR;
    TotalSec = DeathMin*60 + DeathSec;
    TimeBig = (TotalSec >> 8) & 0xFF;
    TimeLittle = TotalSec & 0xFF;
    TX_Check_Sum = 0x09 + 0x10 + TimeBig + TimeLittle;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x09;
    Out_command[3] = 0x10;
    Out_command[4] = TimeBig;
    Out_command[5] = TimeLittle;
    Out_command[6] = TX_Check_Sum;
    Out_command[7] = 0xd3;
    Out_command[8] = 0x3d;
    return 9;
}

uint8_t CalibrationSet(char* Out_command, uint8_t Buff_LEN)
{
    uint8_t TX_Check_Sum;
    if(Buff_LEN < 7)
        return COMMAND_ERROR;
    TX_Check_Sum = 0x07 + 0x07;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x07;
    Out_command[3] = 0x07;
    Out_command[4] = TX_Check_Sum;
    Out_command[5] = 0xd3;
    Out_command[6] = 0x3d;
    return 7;
}

uint8_t CollisionSet(char* Out_command, uint8_t Buff_LEN,uint8_t bias_degree,uint8_t strength_X,uint8_t strength_Y,uint8_t strength_Z)
{
    uint8_t TX_Check_Sum;
    if(Buff_LEN < 11)
        return COMMAND_ERROR;
    TX_Check_Sum = 0x0B + 0x06 + bias_degree + strength_X + strength_Y + strength_Z;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x0B;
    Out_command[3] = 0x06;
    Out_command[4] = bias_degree;
    Out_command[5] = strength_X;
    Out_command[6] = strength_Y;
    Out_command[7] = strength_Z;
    Out_command[8] = TX_Check_Sum;
    Out_command[9] = 0xd3;
    Out_command[10]= 0x3d;
    return 11;
}

uint8_t VersionQuery(char* Out_command, uint8_t Buff_LEN)
{
    uint8_t TX_Check_Sum;
    if(Buff_LEN < 7)
        return COMMAND_ERROR;
    TX_Check_Sum = 0x07 + 0x08;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x07;
    Out_command[3] = 0x08;
    Out_command[4] = TX_Check_Sum;
    Out_command[5] = 0xd3;
    Out_command[6] = 0x3d;
    return 7;
}


uint8_t CollisionClean(char* Out_command, uint8_t Buff_LEN)
{
    uint8_t TX_Check_Sum;
    if(Buff_LEN < 7)
        return COMMAND_ERROR;
    TX_Check_Sum = 0x07 + 0x09;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x07;
    Out_command[3] = 0x09;
    Out_command[4] = TX_Check_Sum;
    Out_command[5] = 0xd3;
    Out_command[6] = 0x3d;
    return 7;
}

int8_t FactoryTest(char* Out_command , uint8_t Command_LEN)
{
    if(Command_LEN < 7)
        return COMMAND_ERROR;
    Out_command[0] = 0x7a;
    Out_command[1] = 0xa7;
    Out_command[2] = 0x07;
    Out_command[3] = 0x30;
    Out_command[4] = 0x37;
    Out_command[5] = 0xd3;
    Out_command[6] = 0x3d;  
    return 7;

}

int8_t RUN_Results(char* Results_Command , uint8_t Command_LEN, short* Command_ID, int* head)
{
    uint8_t Len;
    int temphead;
    while(Results_Command[*head] != 0x7A)
    {
        temphead = *head;
        temphead++;
        *head = temphead;
        if(*head >= Command_LEN)
           return COMMAND_LENGTH_ERROR;
    }
    
    Len = Results_Command[2 + *head];
    //if(!((Command_LEN != 18)|(Command_LEN != 10)) | (Command_LEN != Len))
    //    return COMMAND_LENGTH_ERROR;
    if(((Results_Command[0 + *head] & 0xff) != 0x7A) | ((Results_Command[1 + *head] & 0xff) != 0xA7) | ((Results_Command[Len-2 + *head] & 0xff) != 0xD3) | ((Results_Command[Len-1 + *head] & 0xff) != 0x3D))
        return COMMAND_FORMAT_ERROR;
    if((Results_Command[5 + *head] & 0xff) == 0xFF)
    {
        *Command_ID = Results_Command[4 + *head];
        return COMMAND_SUCCESSFUL;
    }
    if((Results_Command[5 + *head] & 0xff) == 0x00)
    {
        *Command_ID = Results_Command[4 + *head];
        return COMMAND_FIAL;
    }
    return -5;
}


/**************************************************************************//**
* @file     dipdrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __DIP_DRV_H__
#define __DIP_DRV_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define DIP_NORMAL_ID               0x10
#define DIP_REPLACE_BP_ID           0x11 
#define DIP_SETTING_SPACE_ID        0x12 
#define DIP_TESTER_KEYPAD_ID        0x13 
#define DIP_TESTER_ID               0x14    
    
    
#define DIP_SW6_GPIO_PORT           GPIOJ
#define DIP_SW6_GPIO_F1             BIT0
#define DIP_SW6_GPIO_F2             BIT1
#define DIP_SW6_GPIO_F3             BIT2
#define DIP_SW6_GPIO_F4             BIT3
#define DIP_SW6_GPIO_F5             BIT4    
    

//Contactless Card Reader Selector (High=TSReader, Low=OctopusReader)
#define DIP_CARD_READER_SELECT_PORT     DIP_SW6_GPIO_PORT
#define DIP_CARD_READER_SELECT_PIN      DIP_SW6_GPIO_F5
//Testing Program Selector (High=Normal Tester, Low=Burn/In Tester)
#define DIP_BURNIN_TEST_PORT            DIP_SW6_GPIO_PORT
#define DIP_BURNIN_TEST_PIN             DIP_SW6_GPIO_F1

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL DipDrvInit(BOOL testModeFlag);
void DipSetCallbackFunc(keyHardwareCallbackFunc func);

#ifdef __cplusplus
}
#endif

#endif //__DIP_DRV_H__

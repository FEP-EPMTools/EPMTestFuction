/**************************************************************************//**
* @file     guidrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __GUI_DRV_H__
#define __GUI_DRV_H__

#include "nuc970.h"
#include "interface.h"
#include "halinterface.h"
#include "timerdrv.h"
#include "dipdrv.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define GUI_TIME_0_INDEX        TIMER_0_INTERFACE_INDEX
#define GUI_TIME_1_INDEX        TIMER_1_INTERFACE_INDEX
#define GUI_TIME_2_INDEX        TIMER_2_INTERFACE_INDEX    

#define GUI_KEY_UP_INDEX        KEY_HARDWARE_UP_EVENT
#define GUI_KEY_DOWN_INDEX      KEY_HARDWARE_DOWN_EVENT
#define GUI_KEY_ERROR_INDEX     KEY_HARDWARE_ERROR_EVENT   
    
//#define GUI_KEYPAD_LEFT_ID      0x02
//#define GUI_KEYPAD_RIGHT_ID     0x01
//#define GUI_KEYPAD_ADD_ID       0x03
//#define GUI_KEYPAD_CONFIRM_ID   0x00   
#define GUI_KEYPAD_LEFT_ID      0x03
#define GUI_KEYPAD_RIGHT_ID     0x02
#define GUI_KEYPAD_ADD_ID       0x04
#define GUI_KEYPAD_MINUS_ID     0x01
#if(SUPPORT_HK_10_HW)
    #define GUI_KEYPAD_QRCODE_ID   0x00
    #define GUI_KEYPAD_CARD_ID     0x05
#else
    #define GUI_KEYPAD_CONFIRM_ID   0x00  
#endif

#define GUI_KEYPAD_ONE      0x03
#define GUI_KEYPAD_TWO      0x02
#define GUI_KEYPAD_THREE    0x04
#define GUI_KEYPAD_FOUR     0x01
#define GUI_KEYPAD_FIVE     0x05
#define GUI_KEYPAD_SIX      0x00

#define GUI_KEYPAD_NORMAL_ID            DIP_NORMAL_ID
#define GUI_KEYPAD_REPLACE_BP_ID        DIP_REPLACE_BP_ID
#define GUI_KEYPAD_SETTING_SPACE_ID     DIP_SETTING_SPACE_ID
#define GUI_KEYPAD_TESTER_KEYPAD_ID     DIP_TESTER_KEYPAD_ID
#define GUI_KEYPAD_TESTER_ID            DIP_TESTER_ID     

#define GUI_POWER_STATUS_INDEX      0x01
#define GUI_POWER_ON_INDEX          0x02
#define GUI_POWER_OFF_INDEX         0x03
#define GUI_POWER_PREV_OFF_INDEX    0x04
   
#define GUI_REDRAW_PARA_DONT_CARE   0x00   
#define GUI_REDRAW_PARA_NONE        0x01   
#define GUI_REDRAW_PARA_NORMAL      0x02
#define GUI_REDRAW_PARA_REFRESH     0x03
#define GUI_REDRAW_PARA_CONTAIN     0x04
    
#define GUI_TIMER_ENABLE            0x05
#define GUI_TIMER_DISABLE           0x06

#define GUI_CLEAN_MESSAGE_ENABLE    0x07

#define GUI_KEY_ENABLE              0x08
#define GUI_KEY_DISABLE             0x09

#define GUI_KEYPAD_TEST             0x0A
#define GUI_MTP_INI                 0x0B
#define GUI_MTP_SCREEN              0x0C
#define GUI_MTP_START               0x0D

#define GUI_CAD_TIMER_ENABLE        0x10
#define GUI_CAD_TIMER_DISABLE       0x11
#define GUI_CAD_TIMEROUT_FLAG       0x12

#define GUI_OCTOPUS_SELECTTYPE_EN   0x15 
#define GUI_OCTOPUS_SELECTTYPE_DE   0x16

#define GUI_SETRTC_YEAR             0x20
#define GUI_SETRTC_MONTH            0x21
#define GUI_SETRTC_DAY              0x22
#define GUI_SETRTC_HOUR             0x23
#define GUI_SETRTC_MINUTE           0x24
#define GUI_SETRTC_WEEKDAY          0x25
#define GUI_SETRTC_MODIFY           0x26

#define GUI_SET_RADARA_PARAMETER    0x30
#define GUI_SET_RADARB_PARAMETER    0x31
#define GUI_NEW_RADAR_TEST          0x32
#define GUI_NEW_RADARA_OTA          0x33
#define GUI_NEW_RADARB_OTA          0x34

typedef struct
{
    guiOnDrawFunc           onDraw;
    guiUpdateDataFunc       updateData;
    guiKeyCallbackFunc      keyCallback;
    guiTimerCallbackFunc    timerCallback;
    guiPowerCallbackFunc    powerCallback;
}GuiInstance;

typedef struct
{
    uint8_t           guiId;
    GuiInstance*      guiInstance;
}UserGuiInstance;

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL GUIDrvInit(BOOL testModeFlag);
void GuiSetKeyCallbackFunc(guiKeyCallbackFunc callback);
void GuiSetTimerCallbackFunc(guiTimerCallbackFunc callback);
BOOL GuiSetInstance(UserGuiInstance* instance, uint8_t oriGuiId, uint8_t para, int para2, int para3);
void GuiSetTimeout(uint8_t timerIndex, TickType_t time);
void GuiRunTimeoutFunc(uint8_t timerIndex);
#ifdef __cplusplus
}
#endif

#endif //__GUI_DRV_H__

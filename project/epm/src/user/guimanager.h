/**************************************************************************//**
* @file     guimanager.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __GUI_MANAGER_H__
#define __GUI_MANAGER_H__

#include "nuc970.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/    
#define GUI_STANDBY_ID          1
#define GUI_SEL_SPACE_ID        2
#define GUI_SEL_TIME_ID         3
#define GUI_READER_INIT_ID      4
#define GUI_DEPOSIT_ID          5
#define GUI_DEPOSIT_OK_ID       6
#define GUI_DEPOSIT_FAIL_ID     7
#define GUI_REPLACE_BP_ID       8
#define GUI_TESTER_ID           9
#define GUI_TESTER_KEYPAD_ID    10
#define GUI_FILE_DOWNLOAD_ID    11
#define GUI_FREE_ID             12
#define GUI_OFF_ID              13
#define GUI_HW_TEST_ID          14
#define GUI_ALL_TEST_ID         15
#define GUI_SINGLE_TEST_ID      16
#define GUI_TOOL_TEST_ID        17
#define GUI_FACTORY_TEST_ID     18
#define GUI_SETTING_ID          19
#define GUI_SHOW_CARD_ID        20
#define GUI_RADAR_ID            21
#define GUI_USB_CAM_ID          22
#define GUI_LIDAR_ID            23
#define GUI_NULL_ID             24
#define GUI_CALIBRATION_ID      25
#define GUI_VERSION_ID          26
#define GUI_BLANK_ID            27
#define GUI_EPDFLASH_TOOL_ID    28
#define GUI_RADAROTA_TOOL_ID    29
#define GUI_BURNIN_TESTER_ID    30
#define GUI_RTC_TOOL_ID         31
#define GUI_RADAR_TOOL_ID       32


#define GUI_SHOW_SCREEN_OK          0x01
#define GUI_SHOW_SCREEN_ERROR       0x02
#define GUI_SHOW_SCREEN_IGNORE      0x03

#define GUI_MSG_SHOW    1
#define GUI_MSG_REFLASH 2    
#define GUI_MSG_CLEAN   3

typedef struct
{
    char        charItem;
    char*       itemName;
}ListItem;

/*定義 GUI 鍵盤回調*/
typedef void (*KeyinCallback)(char);

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL GuiManagerInit(void);
uint8_t GuiManagerShowScreen(uint8_t guiId, uint8_t para, int para2, int para3);
BOOL GuiManagerRefreshScreen(void);
BOOL GuiManagerUpdateScreen(void);
BOOL GuiManagerCompareCurrentScreenId(uint8_t guiId);
//new add 2019/01/15 by Jermey
BOOL GuiManagerResetInstance(void);
BOOL GuiManagerResetKeyCallbackFunc(void);

BOOL GuiManagerTimerSet(uint8_t reFreshPara);

BOOL GuiManagerCleanMessage(uint8_t reFreshPara);

BOOL GuiManagerUpdateMessage(uint8_t reFreshPara,int UpdatePara2,int UpdatePara3);

#if (ENABLE_BURNIN_TESTER)
uint32_t GetEpdBurninTestCounter(void);
uint32_t GetEpdBurninTestErrorCounter(void);
void SetEpdErrorFlag(BOOL flag);
#endif



#ifdef __cplusplus
}
#endif

#endif //__GUI_MANAGER_H__

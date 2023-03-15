/**************************************************************************//**
* @file     epddrv.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __EPD_DRV_H__
#define __EPD_DRV_H__
#include <time.h>
#include "nuc970.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
typedef unsigned char TByte; //1 byte
typedef unsigned short TWord; //2 bytes
typedef unsigned long TDWord; //4 bytes
//structure prototype 3
//See user defined command ?Get Device information (0x0302)
typedef struct
{
    TWord usPanelW;
    TWord usPanelH;
    TWord usImgBufAddrL;
    TWord usImgBufAddrH;
    TWord usFWVersion[8]; //16 Bytes String
    TWord usLUTVersion[8]; //16 Bytes String
    
}I80IT8951DevInfo;
typedef void(*guiUpdateMsgFunc)(void);
typedef struct
{
    char responeChar;
    int whiteItemName;
    int blackItemName;
}ItemIndex;

typedef enum{
    EPD_PICT_ALL_WHITE_INDEX = 3,
    EPD_PICT_ALL_BLACK_INDEX = 4,      
    
    EPD_PICT_PIXEL_4_INDEX = 5, 
    
    EPD_PICT_INDEX_INIT = 6,
    EPD_PICT_INDEX_FILE_DOWNLOADING = 7,
    EPD_PICT_INDEX_INIT_FAIL = 8,  

    EPD_PICT_NUM_SMALL_INDEX = 9,
    EPD_PICT_X_SMALL_INDEX = 25,
    EPD_PICT_COLON_SMALL_INDEX = 26,
    
    EPD_PICT_DEPOSIT_FAILE_ERROR_CODE_INDEX = 27,
    EPD_PICT_DEPOSIT_FAILE_ECC_INDEX = 28,
    EPD_PICT_DEPOSIT_FAILE_FAILURE_INDEX = 29,
    EPD_PICT_DEPOSIT_FAILE_CANT_USE_INDEX = 30,
    EPD_PICT_DEPOSIT_FAILE_INSUFFOCOENT_MONEY_INDEX = 31,
    EPD_PICT_PAYMENT_QRCODE_UNCHECK_INDEX = 32,
    EPD_PICT_PAYMENT_QRCODE_CHECKED_INDEX = 33,
    EPD_PICT_PAYMENT_CARD_UNCHECK_INDEX = 34,
    EPD_PICT_PAYMENT_CARD_CHECKED_INDEX = 35,
   
    EPD_PICT_NUM_BIG_INDEX = 42,
    EPD_PICT_COLON_BIG_INDEX = 52,
    EPD_PICT_LINE_BIG_INDEX = 53,  
    EPD_PICT_POINT_BIG_INDEX = 54,    
    
    EPD_PICT_NUM_BIG_I_INDEX = 55,
    EPD_PICT_COLON_BIG_I_INDEX = 65,
    EPD_PICT_LINE_BIG_I_INDEX = 66, 
    EPD_PICT_POINT_BIG_I_INDEX = 67,  
    EPD_PICT_POINT_TRAILVER_INDEX = 68, 
    
    EPD_PICT_NUM_SMALL_2_INDEX = 81,
    EPD_PICT_COLON_SMALL_2_INDEX = 91,
    EPD_PICT_LINE_SMALL_2_INDEX = 92, 
    EPD_PICT_SLASH_SMALL_2_INDEX = 93,
    EPD_PICT_POINT_SMALL_2_INDEX = 94, 
    EPD_PICT_V_SMALL_2_INDEX = 95,
    
    EPD_PICT_NUM_SMALL_2_I_INDEX = 96,
    EPD_PICT_COLON_SMALL_2_I_INDEX = 106,
    EPD_PICT_LINE_SMALL_2_I_INDEX = 107, 
    EPD_PICT_SLASH_SMALL_2_I_INDEX = 108,
    EPD_PICT_POINT_SMALL_2_I_INDEX = 109, 
    EPD_PICT_V_SMALL_2_I_INDEX = 110,

    EPD_PICT_CAR1_INDEX = 120, 
    
    EPD_PICT_INDEX_ISSUED_W = 122,
    EPD_PICT_INDEX_ISSUED_B = 123,
    EPD_PICT_BIG_CAST_BG_INDEX = 124,
    
    EPD_PICT_CAR1_I_INDEX = 126,
    EPD_PICT_METER_INDEX = 132,
    EPD_PICT_UPPER_LINE_INDEX = 133,
    EPD_PICT_SIGNAL_INDEX = 134,
    EPD_PICT_SIGNAL_BG_INDEX = 135,
    
    EPD_PICT_DISABLE_PARKING_INDEX = 136,
    EPD_PICT_DISABLE_PARKING_E_INDEX = 137,
    
    EPD_PICT_CONTAIN_CLEAR_INDEX = 138,    
    EPD_PICT_CONTAIN_SELECT_TIME_INDEX = 139,    
    EPD_PICT_SMALL_CAST_BG_INDEX = 140,
    EPD_PICT_CONTAIN_DEPOSIT_INDEX = 141,
    EPD_PICT_EPD_VERSION_INDEX = 142,
    EPD_PICT_CONTAIN_LINEPAY_INDEX = 143,
    EPD_PICT_CONTAIN_LINEPAY_IN_PROGRESS_INDEX = 144,
    EPD_PICT_CONTAIN_SELECT_MOBILE_INDEX = 145,
    EPD_PICT_CONTAIN_READER_INIT_INDEX = 146,    
    EPD_PICT_CONTAIN_READER_INIT_FAIL_INDEX = 147,    
    EPD_PICT_CONTAIN_SEL_PAYMENT_INDEX = 148,    
    EPD_PICT_CONTAIN_DEPOSIT_OK_INDEX = 149,    
    EPD_PICT_CONTAIN_DEPOSIT_FAIL_INDEX = 150,   
    EPD_PICT_CONTAIN_SCAN2PAY_INDEX = 151, 
    EPD_PICT_CONTAIN_MOBILE_PAY_OK_INDEX = 152, 

    EPD_PICT_CONTAIN_INDEX_BATTERY_REPLACE = 153,  
    EPD_PICT_INDEX_BATTERY_REPLACE_INUSE = 154,  
    EPD_PICT_INDEX_BATTERY_REPLACE_NEED_REPLACE = 155,  
    EPD_PICT_INDEX_BATTERY_REPLACE_IDLE = 156,  
    EPD_PICT_INDEX_BATTERY_REPLACE_EMPTY = 157,  
    
    EPD_PICT_CONTAIN_INDEX_TESTER_KEYPAD = 158, 
    EPD_PICT_CONTAIN_INDEX_TESTER = 159,  
    
    //EPD_PICT_CONTAIN_BG_6_5_INDEX = 160,
    //EPD_PICT_CONTAIN_BG_4_INDEX = 161,
    //EPD_PICT_CONTAIN_BG_4_3_INDEX = 162,
    EPD_PICT_CONTAIN_BG_2_INDEX = 163,
//    EPD_PICT_CONTAIN_BG_2_1_INDEX = 164,
    //EPD_PICT_BOARD_4_L_INDEX = 165,
    //EPD_PICT_BOARD_4_R_INDEX = 166,
    //EPD_PICT_BOARD_4_I_L_INDEX = 167,
    //EPD_PICT_BOARD_4_I_R_INDEX = 168,
    EPD_PICT_BOARD_2_L_INDEX = 169,
    EPD_PICT_BOARD_2_R_INDEX = 170,
    EPD_PICT_BOARD_2_I_L_INDEX = 171,
    EPD_PICT_BOARD_2_I_R_INDEX = 172,
    
    EPD_PICT_INDEX_EXPIRED_BG = 173,
    EPD_PICT_INDEX_EXPIRED_W = 174,
    EPD_PICT_INDEX_EXPIRED_B = 175,
    EPD_PICT_INDEX_FREE     = 177,
    EPD_PICT_INDEX_OFF_2      = 178,
    EPD_PICT_CONTAIN_DEPOSIT_2_INDEX = 179,      

    EDP_PICT_LINE_H     = 180,
    EDP_PICT_LINE_V     = 181,
    /////UPPER//////
    EPD_PICT_UPPER_A        = 182,
    
    EPD_PICT_0X21_24        = 208,
    EPD_PICT_0X22_24        = 209,
    EPD_PICT_0X23_24        = 210,
    EPD_PICT_EMPTY          = 211,
    EPD_PICT_LOADING        = 212,
    /////LOWER//////
    EPD_PICT_LOWER_A        = 213,
    /////KEY////////
    EPD_PICT_KEY_CLEAN_BAR  = 239,
    EPD_PICT_KEY_CONFIRM    = 240,
    EPD_PICT_KEY_LEFT       = 241,
    EPD_PICT_KEY_MINUS      = 242,
    EPD_PICT_KEY_PLUS       = 243,
    EPD_PICT_KEY_RIGHT      = 244,
    EPD_PICT_KEY_CROSS      = 245,
    EPD_PICT_KEY_CARD       = 246,
    EPD_PICT_KEY_CARD2      = 247,
    EPD_PICT_KEY_QRCODE     = 248,
    EPD_PICT_KEY_ONE        = 249,
    EPD_PICT_KEY_TWO        = 250,
    EPD_PICT_KEY_THREE      = 251,
    EPD_PICT_KEY_FOUR       = 252,
    EPD_PICT_KEY_FIVE       = 253,
    EPD_PICT_KEY_SIX        = 254,
    
    EPD_PICT_SMALL2_UPPER_A_INDEX = 255,
    EPD_PICT_SMALL2_UPPER_0_INDEX = 281,
    
    EPD_PICT_INDEX_NULL      = 0xffff,
    
}epdPictIndex; 
/*  Keypad Icon Char Defination */
#define KEYPAD_ONE_CHAR '<'
#define KEYPAD_TWO_CHAR '>'
#define KEYPAD_THREE_CHAR '+'
#define KEYPAD_FOUR_CHAR  '='
#define KEYPAD_FIVE_CHAR  '{'
#define KEYPAD_SIX_CHAR   '}'


#define KEYPAD_REAL_PLUS_CHAR   0xD3

#define EPD_FONT_TYPE_SMALL     0x01
#define EPD_FONT_TYPE_SMALL_2   0x02
#define EPD_FONT_TYPE_MEDIUM    0x03
#define EPD_FONT_TYPE_BIG       0x04
#define EPD_FONT_TYPE_DEBUG     0x05

#define CAR_ITEM_MAX_NUM    6
#define BANNER_ITEM_NUM    (CAR_ITEM_MAX_NUM + 1)

#define DEPOSIT_TIME_DIGITAL_NUM          4//5

#define DEVICE_ID_DIGITAL_NUM          8

#define MAX_LOAD_IMG    32

#define SPI_FLASH_EX_PAGE_SIZE   256

#pragma pack(1)
typedef struct
{
    unsigned int        bmpId;
    unsigned int        xPos;
    unsigned int        yPos;
    
}positionInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[DEVICE_ID_DIGITAL_NUM]; 
}positionDeviceIDInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info;   
}positionSingleInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[32];   
}positionStringInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[MAX_LOAD_IMG];   
}positionMultiInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[6]; //(car board, number)*2  
}positionItemInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[BANNER_ITEM_NUM + CAR_ITEM_MAX_NUM*2 + 1];
}positionAllItemInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[BANNER_ITEM_NUM + 1];   
}bannerLinePositionInfo;

typedef struct
{
    unsigned int carEnableNum;
    positionInfo info[BANNER_ITEM_NUM];   
}bannerPositionInfo;

typedef struct
{
    unsigned int boardEnableNum;
    positionInfo info[CAR_ITEM_MAX_NUM];   
}containPositionInfo;

typedef struct
{
    unsigned int depositTimeEnableNum;
    positionInfo info[2*DEPOSIT_TIME_DIGITAL_NUM];   
}depositTimePositionInfo;

typedef struct
{
    unsigned int depositTimeEnableNum;
    positionInfo info[CAR_ITEM_MAX_NUM*DEPOSIT_TIME_DIGITAL_NUM];   
}depositTimeAllPositionInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[20];   
}positionTmpInfo;

typedef struct
{
    uint8_t id[5];   
}timeIdInfo;
#define MAX_COST_POSITION  4
typedef struct
{
    uint8_t id[MAX_COST_POSITION];   //9999
}costIdInfo;

#pragma pack()

typedef void(*epdReinitCallbackFunc)(void);
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
BOOL EpdDrvInit(BOOL testModeFlag);
void EPDSetBacklight(BOOL flag);
BOOL EPDGetBacklight(void);
void EPDReSetBacklightTimeout(time_t time);
void EPDSetReinitCallbackFunc(epdReinitCallbackFunc callback);
void EPDSetSleepFunction(BOOL flag);
void EPDDrawDeviceId(BOOL reFreshFlag);
void EPDShowBGScreen(int id, BOOL reFreshFlag);
void EPDShowErrorID(uint16_t id);
void ShowExpired(uint8_t id, BOOL leftShow, BOOL rightShow, BOOL reFreshFlag);

void EPDDrawBanner(BOOL reFreshFlag);
void EPDDrawContain(BOOL reFreshFlag);
void EPDDrawUpperLine(BOOL reFreshFlag);
void EPDDrawBannerLine(BOOL reFreshFlag);

void UpdateClock(BOOL reFreshFlag, BOOL checkFlag);
BOOL calculatePreparePositionInfo(uint8_t meterPosition, uint8_t carEnableNumber);
void calculateDeviceIDPositionInfo(int32_t id);

BOOL calculateAllItemInfo(void);
void EPDDrawContainBG(BOOL reFreshFlag);
void EPDDrawCar(BOOL reFreshFlag, uint8_t selectCarId, BOOL inverseFlag);
void EPDDrawBoard(BOOL reFreshFlag, uint8_t selectCarId, BOOL inverseFlag);
void EPDDrawItem(BOOL reFreshFlag, uint8_t oriSelectCarId, uint8_t selectCarId);
void EPDDrawAllItem(BOOL reFreshFlag);
void EPDDrawAllDepositTime(BOOL reFreshFlag);
void EPDDrawDepositTime(BOOL reFreshFlag, uint8_t oriSelectItemId, uint8_t selectItemId);
void EPDDrawAllScreen(BOOL reFreshFlag);
void EPDDrawPleaseWait(BOOL reFreshFlag, BOOL clearFlag);
void EPDDrawContainSelTime(BOOL reFreshFlag);
void EPDDrawContainClear(BOOL reFreshFlag);
void EPDDrawTime(BOOL reFreshFlag, time_t time);
void EPDDrawCost(BOOL reFreshFlag, uint32_t cost);
void EPDDrawContainByID(BOOL reFreshFlag, uint8_t id);
BOOL getIDFromTime(time_t time, timeIdInfo* info);
void SetDepositTimeBmpIdInfo(uint8_t index, time_t time);

void ShowBatteryStatus(uint8_t leftId, BOOL leftShow, uint8_t rightId, BOOL rightShow, BOOL reFreshFlag);
void ShowVoltage(time_t leftVoltage, BOOL leftShow, time_t rightVoltage, BOOL rightShow, BOOL reFreshFlag);

void setTotalPower(BOOL flag);
//////////custom function//////////
void EPDDrawContainByIDPos(BOOL reFreshFlag, uint8_t id,unsigned int x_Pos,unsigned int y_Pos);
int  EPDDrawString(BOOL rowrefresh,char* string,unsigned int x_init_Pos,unsigned int y_init_Pos);
void EPDDrawStringMax(BOOL refresh,char* string,unsigned int x_Pos,unsigned int y_Pos,BOOL load);
void EPDDrawMulti(BOOL reFreshFlag, uint8_t id,unsigned int x_Pos,unsigned int y_Pos);
BOOL ReadIT8951SystemInfoLite(char* readFWVersion,char* readLUTVersion);
BOOL ReadIT8951SystemInfo(char* readFWVersion,char* readLUTVersion);

BOOL EPDIT8951Test(void);

void W25Q64BVquery(int temposition);
void W25Q64BVqueryEx(int temposition,uint8_t *u8DataBuffer,int size);
void W25Q64BVErase(void);
void W25Q64BVBurn(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen);

BOOL SpiFlash_WaitEraseReady(void);

BOOL readEPDswitch(void);
void toggleEPDswitch(BOOL toggleEPDflag);



void IT8951WriteReg(uint16_t usRegAddr,uint16_t usValue);

uint16_t IT8951GetVCOM(void);
void IT8951SetVCOM(uint16_t vcom);


void setSpecialCleanFlag(BOOL flag);


void setdataCode(uint32_t buildDataCode);
uint32_t getdataCode(void);

uint8_t getflashvalue(int position);

void W25Q64BVdeviceID(void);

void setW25Q64BVspecialburn(void);

void clrW25Q64BVspecialburn(void);

void EPDShowBGScreenEx(int id, BOOL reFreshFlag,TWord usW, TWord usH);

#ifdef __cplusplus
}
#endif

#endif //__EPD_DRV_H__

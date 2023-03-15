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

#define EPD_PICT_FAKE_INDEX     EPD_PICT_SIGNAL_BG_INDEX

#define EPD_FONT_TYPE_SMALL_1           0x01
#define EPD_FONT_TYPE_SMALL_2           0x02
#define EPD_FONT_TYPE_MEDIUM            0x03
#define EPD_FONT_TYPE_BIG               0x04
#define EPD_FONT_TYPE_SMALL_2_INVERSE   0x05
#define EPD_FONT_TYPE_BIG_INVERSE       0x06
//#define EPD_FONT_TYPE_DEBUG     0x05

#define CAR_ITEM_MAX_NUM    2//6
#define BANNER_ITEM_NUM    (CAR_ITEM_MAX_NUM + 1)

#define DEPOSIT_TIME_DIGITAL_NUM        4//5

#define DEVICE_ID_DIGITAL_NUM           8

#define ALPHABET_NUMBER_DIGITS_MAX      12

#define VERSION_DIGITAL_NUM             8     

//#define TIME_START_X_POSITION     345
//#define TIME_START_Y_POSITION     370
//#define COST_START_X_POSITION     500
//#define COST_START_Y_POSITION     (TIME_START_Y_POSITION + 130)
//#define COST_2_START_Y_POSITION   330
#define TIME_START_X_POSITION     260
#define TIME_START_Y_POSITION     300
#define COST_1_START_X_POSITION     690
#define COST_1_START_Y_POSITION     300
#define COST_2_START_X_POSITION     500
#define COST_2_START_Y_POSITION     500

#define CURRENT_VERSION_X                      105//750
#define CURRENT_VERSION_Y                          665//40


#define RADAR_INFO_X_POSITION       375
#define RADAR_INFO_Y_POSITION       15

#define RADAR_INFO_X_POSITION_2     570
#define RADAR_INFO_Y_POSITION_2     RADAR_INFO_Y_POSITION

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
    positionInfo info[VERSION_DIGITAL_NUM]; 
}positionVerInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[ALPHABET_NUMBER_DIGITS_MAX]; 
}positionDigitsInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info;   
}positionSingleInfo;

typedef struct
{
    unsigned int itemNum;
    positionInfo info[2];   
}positionDoubleInfo;

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
    #if(ENABLE_GUI_CAST_HONG_KONG_MODE)
    uint8_t id[MAX_COST_POSITION + 3];   //9999.00
    #else
    uint8_t id[MAX_COST_POSITION];   //9999
    #endif
    
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
//void EPDSetReinitCallbackFunc(epdReinitCallbackFunc callback);
void EPDSetSleepFunction(BOOL flag);
void EPDSetFakeTimeout(BOOL flag);
void EPDSetPower(BOOL flag);

void EPDDrawDeviceId(BOOL reFreshFlag);
void EPDDrawIntergerNumber(BOOL reFreshFlag, int32_t id, int xPosition, int yPosition, int digits);
void EPDShowBGScreen(int id, BOOL reFreshFlag);
void EPDShowBGScreenPosition(int id, unsigned int positionX, unsigned int positionY, BOOL reFreshFlag);
void EPDShowErrorID(BOOL reFreshFlag, int positionX, int positionY, uint16_t id);
void EPDShowErrorIDHex(BOOL reFreshFlag, int positionX, int positionY, uint16_t id);
void ShowExpired(uint8_t id, BOOL leftShow, BOOL rightShow, BOOL reFreshFlag);
void EPDShowQRCode(char* data, int y, BOOL reFreshFlag);
void EPDShowBMPFile(char* fileName, int scaleValue, int x, int y, BOOL reFreshFlag);
void EPDShowBMPData(uint8_t* data, int dataLen, int scaleValue, int x, int y, BOOL reFreshFlag);
void EPDShowBMPFromBase64(char* base64Data, int scaleValue, int x, int y, BOOL reFreshFlag);
void EPDDrawVersion(BOOL reFreshFlag);

void EPDDrawBanner(BOOL reFreshFlag);
void EPDDrawContain(BOOL reFreshFlag);
void EPDDrawUpperLine(BOOL reFreshFlag);
void EPDDrawBannerLine(BOOL reFreshFlag);


BOOL UpdateClock(BOOL reFreshFlag, BOOL checkFlag);
BOOL UpdateOffInfo(BOOL reFreshFlag);
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
void EPDDrawContainSelPayment(BOOL reFreshFlag);
void EPDDrawContainClear(BOOL reFreshFlag);
void EPDDrawTime(BOOL reFreshFlag, time_t time);
void EPDDrawCost(int positionX, int positionY, BOOL reFreshFlag, uint32_t cost, uint8_t fontType);
void EPDDrawContainByID(BOOL reFreshFlag, uint8_t id);
void EPDDrawContainByIDEx(int xPos, int yPos, BOOL reFreshFlag, uint8_t id);
BOOL getIDFromTime(time_t time, timeIdInfo* info, uint8_t fontType);
void SetDepositTimeBmpIdInfo(uint8_t index, time_t time);
void EPDShowVersionNumber(BOOL reFreshFlag, int positionX, int positionY, uint32_t id);
void EPDShowBookingId(BOOL reFreshFlag, int positionX, int positionY, char* bookid);

void ShowBatteryStatus(uint8_t leftId, BOOL leftShow, uint8_t rightId, BOOL rightShow, BOOL reFreshFlag);
void ShowVoltage(time_t leftVoltage, BOOL leftShow, time_t rightVoltage, BOOL rightShow, BOOL reFreshFlag);
void EPDSetWDTMode(void);
void EPDDrawSignalIcon(BOOL reFreshFlag, BOOL showFlag);
void EPDDrawPaymentMethod(BOOL reFreshFlag, BOOL cardCheckFlag, BOOL qrcodeCheckFlag);
void EPDDrawCarID(BOOL reFreshFlag, int positionX, int positionY, char *carID);

TWord EPDGetVCOM(void);

void EPDSetVCOM(TWord vcom);

void EPDShowDisableParkingIcon(int spaceid, BOOL reFreshFlag);

void calculateVersionPositionInfo(int xPosition, int yPosition, uint8_t majorVer, uint8_t minorVer, uint8_t revisionVer);

#ifdef __cplusplus
}
#endif

#endif //__EPD_DRV_H__

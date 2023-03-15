/**************************************************************************//**
* @file     hwtester.c
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
#include "nuc970.h"
#include "sys.h"
#include "gpio.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "hwtester.h"
#include "buzzerdrv.h"
#include "timelib.h"
#include "leddrv.h"
#include "flashdrvex.h"
#include "epddrv.h"
#include "cardreader.h"
#include "nt066edrv.h"
#include "pct08drv.h"
#include "pct08cmdlib.h"
#include "quentelmodemlib.h"
#include "dipdrv.h"
#include "batterydrv.h"
#include "sr04tdrv.h"
#include "smartcarddrv.h"
#include "powerdrv.h"

#include "guimanager.h"
#include "guidrv.h"

#include "sflashrecord.h"

#include "ff.h"
#include "fatfslib.h"

#include "epddrv.h"
#include "creditReaderDrv.h"
#include "../octopus/octopusreader.h"
#include "MtpProcedure.h"
//#include "modemagent.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define TEST_FALSE						0x10
#define TEST_TRUE                       0x11    
#define TEST_SUCCESSFUL_LIGHT_OFF		0x12
#define TEST_SUCCESSFUL_LIGHT_ON		0x13


#if(SUPPORT_HK_10_HW)
    #define IOPIN_BOTH_ON		0
    #define IOPIN_ONLY1_ON		2
    #define IOPIN_ONLY2_ON		1
    #define IOPIN_BOTH_OFF		3
    #define SOLAR_ON            4
    #define SOLAR_OFF           5
#else
    #define IOPIN_BOTH_ON		0
    #define IOPIN_ONLY1_ON		1
    #define IOPIN_ONLY2_ON		2
    #define IOPIN_BOTH_OFF		3
    #define SOLAR_ON            4
    #define SOLAR_OFF           5
#endif

#define BATTERY_L_MAX_VOLTAGE 770 //1200	//7.7v
#define BATTERY_L_MIN_VOLTAGE 730 //800	//7.3v
#define BATTERY_R_MAX_VOLTAGE 800 //1200	//7.7v
#define BATTERY_R_MIN_VOLTAGE 760 //800	//7.3v
#define SOLAR_BAT_MAX_VOLTAGE 850 //860 20200706 considerd loader voltage and modify by Steven 
#define SOLAR_BAT_MIN_VOLTAGE 770 //790 20200706 considerd loader voltage and modify by Steven
#define MB_BAT_MAX_VOLTAGE    880 //850 
#define MB_BAT_MIN_VOLTAGE    770 //780 

#define MODEM_BAUDRATE 921600

#define ALL_TEST_MODE   1
#define GET_ID_MODE     1
#define POWER_SET_MODE  1
/*   ERROR CODE  */
#define GUI_MANAGER_INIT_ERROR              0x0100

#define LED_CONNECT_ERRO					0x0201
#define LED_HARDWARE_ERRO                   0x0200//no light

#define RED_SWITCH_INITIAL_ERRO             0x0300//initial error
#define RED_SWITCH_1_ERRO                   0x0301//switch 1 no detect
#define RED_SWITCH_2_ERRO                   0x0302//switch 2 no detect
#define RED_SWITCH_BOTH_ERRO                0x0303//switch both no detect
#define RED_SWITCH_BOTH_ON_ERRO				0x0310
#define RED_SWITCH_SEQUENCE_ERRO			0x0320

#define POWER_BOTTON_ON_BEFOR_START_ERRO	0x0401
#define POWER_BOTTON_WAIT_ON_ERRO			0x0402
#define POWER_BOTTON_WAIT_OFF_ERRO			0x0403

#define EPD_INIT_ERROR                      0x0500
#define EPD_CONNECT_ERRO					0x0501

#define TOUCH_CONNECT_ERRO					0x0601
#define TOUCH_SEQUENCE_ERRO					0x0610
#define MODEM_CONNECT_ERRO					0x0701
#define RTC_TIME_ERRO						0x0801
#define READER_INIT_ERRO                    0x0900
#define READER_CONNECT_ERRO					0x0901
#define FLASH1_SPACE_ERRO					0x0A01
#define FLASH2_SPACE_ERRO					0x0A02
#define SMART_CARD_CONNECT_ERRO				0x0B01
#define CAMERA_CONNECT_ERRO					0x0C01
#define PROWAVE1_DISTANCE_ERRO				0x0D01
#define PROWAVE2_DISTANCE_ERRO				0x0D02
#define BATTERY1_VALUE_ERRO					0x0E01
#define BATTERY2_VALUE_ERRO					0x0E02
#define BATTERY1_TIME_OUT_ERRO				0x0E10
#define BATTERY2_TIME_OUT_ERRO				0x0E20
#define BATTERY_SEQUENCE_ERRO				0x0E30

#define SOFTWARE_ERRO 0x0010

#define MODEM_BAUDRATE          921600

#define MENU_STRING_MAIN    "\r\n  ************** Main Menu *************\r\nPlease Select Test Type:\r\n"
#define MENU_STRING_SINGLE  "\r\n  ********** Single Test Menu **********\r\nPlease Select: \r\n"
#define MENU_STRING_TOOL    "\r\n  ************* Tools Menu *************\r\nPlease Select: \r\n"

typedef BOOL(*initFunction)(BOOL testModeFlag);
typedef struct
{
    char*               drvName;
    initFunction        func;    
    BOOL                result;    
}initFunctionList;

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
///////////////////MENU/////////////////////
static UINT8 getTerminalChar(void);
static int actionTestItem(char targetChar, HWTesterItem* item, void* para1, void* para2, BOOL ignoreChar);
static BOOL enterMunu(char* title, HWTesterItem* item, void* para1, void* para2);
static BOOL totalTest(void* para1, void* para2);
static BOOL singleTest(void* para1, void* para2);
static BOOL singleTestB(void* para1, void* para2);
static BOOL toolsFunction(void* para1, void* para2);
static BOOL blankFunction(void* para1, void* para2);
static BOOL KeyPadTool(void* para1, void* para2);
static BOOL ModemHSpdTool(void* para1, void* para2);
static BOOL RTCTool(void* para1, void* para2);
static BOOL ModemTool(void* para1, void* para2);
static BOOL NTPTool(void* para1, void* para2);
static BOOL exitTest(void* para1, void* para2);
static BOOL testSuccessful(int i,BOOL lightOff);
static BOOL testFailure(int i);

#if (!ENABLE_BURNIN_TESTER)
////////////////////LED TOOL///////////////
static BOOL LEDColorBuffSet(UINT16 GreenPinBite, UINT16 RedPinBite);
static BOOL LEDBoardLightSet(void);
#endif

//////////////////MAIN FUNCTION///////////
static BOOL buzzerTest(void* para1, void* para2);
static BOOL LEDTest(void* para1, void* para2);
static BOOL LEDTestLite(void* para1, void* para2);
static BOOL MemsTest(void* para1, void* para2);
static BOOL redSwitchAllTest(void* para1, void* para2);
static BOOL electricitySingleTest(void* para1, void* para2);
static BOOL SW6SingleTest(void* para1, void* para2);
static BOOL EPDSingleTest(void* para1, void* para2);
static BOOL CADSingleTest(void* para1, void* para2);
static BOOL CADReaderTest(void* para1, void* para2);
static BOOL keyPadAllTest(void* para1, void* para2);
static BOOL modeminit(BOOL);
static BOOL modemTest(void* para1, void* para2);
static BOOL RTCTest(void* para1, void* para2);
static BOOL readerTest(void* para1, void* para2);;
static BOOL sFlashTest(void* para1, void* para2);
static BOOL smartCardTest(void* para1, void* para2);
static BOOL versionQuery(void* para1, void* para2);

static BOOL cameraTest(void* para1, void* para2);
static BOOL radarSingleTest(void* para1, void* para2);
static BOOL NewRadarSingleTest(void* para1, void* para2);
static BOOL sensorSingleTest(void* para1, void* para2);
static BOOL lidarSingleTest(void* para1, void* para2);
static BOOL batteryTest(void* para1, void* para2);
static BOOL SuperCapTest(void* para1, void* para2);
//static BOOL epdTest(void* para1, void* para2);
//static BOOL toolsAdjustKeypad(void* para1, void* para2);
//static BOOL idConfig(void* para1, void* para2);
//static BOOL enable12vPower(void* para1, void* para2);
static BOOL suspendSystem(void* para1, void* para2);
//static BOOL batterySelect(void* para1, void* para2);
//static BOOL buzzerLoop(void* para1, void* para2);
//static BOOL modemSetHighSpeed(void* para1, void* para2);
static BOOL readerGetCN(void* para1, void* para2);
static BOOL radarTest(void* para1, void* para2);
static BOOL NEWradarTest(void* para1, void* para2);
static BOOL NEWradarSet(void* para1, void* para2);
static BOOL radarPowerSet(void* para1, void* para2);
static BOOL usbCamTest(void* para1, void* para2);
static BOOL usbCamPowerSet(void* para1, void* para2);
static BOOL lidarTest(void* para1, void* para2);
static BOOL lidarPowerSet(void* para1, void* para2);
static BOOL epdBurningTest(void* para1, void* para2);
static BOOL calibrationConfig(void* para1, void* para2);
static BOOL versionQueryTool(void* para1, void* para2);
static BOOL EPDflashTool(void* para1, void* para2);
static BOOL NewRadarTool(void* para1, void* para2);
static BOOL NEWradarSetDefault(void* para1, void* para2);
static BOOL RadarOTATool(void* para1, void* para2);
static BOOL CADPowerTest(void* para1, void* para2);
static BOOL ModemAndReaderTest(void* para1, void* para2);
static BOOL otherIOTest(void* para1, void* para2);



static BOOL setDeviceID(void* para1, void* para2);
static int getDeviceID(void* para1, void* para2);
static BOOL setDeviceIDTool(void* para1, void* para2);
static BOOL getDeviceIDTool(void* para1, void* para2);

static void cleanRst(void);
static void cleanMsg(void);

//void setTotalPower(BOOL flag);
//Getter Setter
static void setTesterFlag(BOOL testerflag);
static char getGuiResponse(void);

static void InitMTPvalue(uint8_t index,uint8_t* MTPBuf);
static void SetMTPCRC(uint8_t index,uint8_t* MTPBuf);
static void MTPCmdprint(uint8_t index,uint8_t* MTPBuf);
static void MTPCmdprintEx(uint8_t index,uint8_t* MTPBuf);
//PRIVATE VARIABLE
static char guiResponseChar=0;
static BOOL guiResponseFlag=FALSE;
static BOOL TouchPadResponseFlag = FALSE;
static BOOL testerFlag=FALSE;
static BOOL testerResult=FALSE;
static char* epdString;
static BOOL epdPrintFlag=FALSE;
static BOOL refreshMsgBar=FALSE;
static BOOL isEPDInit=FALSE;
static int passItem=0;
static int failItem=0;
static int testedItem=0;
static int allTestItem=0;
static int MBallTestItem=0;
//LED PATTERN
static uint8_t bayColorOff[8] =     {LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF,   LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};
static uint8_t bayColorAllGreen[8]= {LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};
static uint8_t bayColorAllRed[8] = {LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_RED, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};

static uint8_t modemColorAllGreen[8]= {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_GREEN, LIGHT_COLOR_GREEN};
static uint8_t modemColorAllRed[8] = {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_RED, LIGHT_COLOR_RED};

static uint8_t bayColorChange[8] = {LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF, LIGHT_COLOR_OFF};
static uint8_t callBackKeyId,callBackDownUp;
static UINT16 testErroCode = 0x0000;



static unsigned int *setVCOMpoint;
static unsigned int setVCOMEx;

static int epmIDSet=0 ;
static uint32_t cardID=0;

static BOOL firstInitFlag = TRUE;

static BOOL ModemGPIOH4INTFlag = FALSE;
static BOOL ModemResultLEDFlag = FALSE;
static BOOL EPDtestFlag = FALSE;

//static int  SDbufferSize = 128 ;
FIL filephoto;
UINT brPHOTO;

//static char * PhotoFileNameStr;
//static char  PhotoFileNameStr[50];

RTC_TIME_DATA_T* pptPHOTO;

#define PHOTOBUFFERSIZE 256
static uint8_t tempPr[PHOTOBUFFERSIZE];
//static uint8_t tempPr[100000];
//static char * PhotoFileNameStr;
//static char PhotoFileNameStr[50];


static BOOL IOtestResultFlag[20];

static BOOL targetkeyStage;

static BOOL MBtestFlag = FALSE;

static BOOL AssemblyTestFlag = FALSE;

static BOOL SleeptestFlag = FALSE;

static BOOL gCADtimeoutFlag = FALSE;

static UINT32 ReceieveTouchPadVal;

static uint8_t MTPString[60][18] = {{0x90,0x90,0x09,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00}, //[0]battery test
                                    {0x90,0x91,0x00,0x00,0x00}, //[1]ask bat1 signal
                                    {0x90,0x92,0x00,0x00,0x00}, //[2]ask bat2 signal
                                    {0x90,0x93,0x00,0x00,0x00}, //[3]ask solar bat signal
                                    {0x90,0x94,0x00,0x00,0x00}, //[4]switch bat1
                                    {0x90,0x95,0x00,0x00,0x00}, //[5]switch bat2
                                    
                                    {0x90,0x9A,0x01,0x82,0x00,0x00}, //[6]buzzer test
                                    {0x90,0x9B,0x00,0x00,0x00}, //[7]ask buzzer test
                                    
                                    {0x90,0x9C,0x04,0x82,0x82,0x82,0x82,0x00,0x00}, //[8]LED test
                                    {0x90,0x9D,0x00,0x00,0x00}, //[9]ask LED signal
                                    
                                    {0x90,0x9E,0x01,0x82,0x00,0x00}, //[10]Credit Card Power
                                    {0x90,0x9F,0x00,0x00,0x00}, //[11]ask 12V test result
                                    
                                    {0x90,0xA0,0x07,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00}, //[12]Keypad test
                                    {0x90,0xA1,0x00,0x00,0x00}, //[13]press key1
                                    {0x90,0xA2,0x00,0x00,0x00}, //[14]press key2
                                    {0x90,0xA3,0x00,0x00,0x00}, //[15]press key3
                                    {0x90,0xA4,0x00,0x00,0x00}, //[16]press key4
                                    {0x90,0xA5,0x00,0x00,0x00}, //[17]press key5
                                    {0x90,0xA6,0x00,0x00,0x00}, //[18]press key6
                                    
                                    {0x90,0xA7,0x07,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00}, //[19]switch test
                                    {0x90,0xA8,0x00,0x00,0x00}, //[20]switch SW4on
                                    {0x90,0xA9,0x00,0x00,0x00}, //[21]switch SW4off
                                    {0x90,0xAA,0x00,0x00,0x00}, //[22]switch SW5on
                                    {0x90,0xAB,0x00,0x00,0x00}, //[23]switch SW5off
                                    {0x90,0xAC,0x00,0x00,0x00}, //[24]switch SW6on
                                    {0x90,0xAD,0x00,0x00,0x00}, //[25]switch SW6off

                                    {0x90,0xB0,0x06,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00}, //[26]Credit Card test
                                    
                                    {0x90,0xB1,0x0D,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00},
                                     //[27]Modem & Reader test
                                    {0x90,0xB2,0x00,0x00,0x00}, //[28]ask SysPwr D5
                                    
                                    {0x90,0xB3,0x09,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00}, //[29]sensor test
                                    {0x90,0xB4,0x00,0x00,0x00}, //[30]ask SysPwr D2
                                    
                                    {0x90,0xB5,0x01,0x82,0x00,0x00}, //[31]RTC test
                                    
                                    {0x90,0xB6,0x01,0x82,0x00,0x00}, //[32]EPD test
                                    
                                    {0x90,0xB7,0x01,0x82,0x00,0x00}, //[33]sleep test
                                    {0x90,0xB8,0x00,0x00,0x00}, //[34]key any touchpad
                                    {0x90,0xB9,0x00,0x00,0x00}, //[35]ask sleep current
                                    
                                    {0x90,0xBA,0x01,0x82,0x00,0x00}, //[36]SuperCap test
                                    {0x90,0xBB,0x00,0x00,0x00}, //[37]ask test result
                                    
                                    {0x90,0xC0,0x00,0x00,0x00}, //[38]End condition
                                    
                                    {0x90,0xC1,0x00,0x00,0x00},  //[39]show"Start test battery"
                                    {0x90,0xC2,0x00,0x00,0x00},  //[40]show"Start test buzzer"
                                    {0x90,0xC3,0x00,0x00,0x00},  //[41]show"Start test LED"
                                    {0x90,0xC4,0x00,0x00,0x00},  //[42]show"Start test Credit Card Power"
                                    {0x90,0xC5,0x00,0x00,0x00},  //[43]show"Start test Keypad"
                                    {0x90,0xC6,0x00,0x00,0x00},  //[44]show"Start test switch"
                                    {0x90,0xC7,0x00,0x00,0x00},  //[45]show"Start test Credit Card"
                                    {0x90,0xC8,0x00,0x00,0x00},  //[46]show"Start test Modem & Reader"
                                    {0x90,0xC9,0x00,0x00,0x00},  //[47]show"Start test sensor"
                                    {0x90,0xCA,0x00,0x00,0x00},  //[48]show"Start test RTC"
                                    {0x90,0xCB,0x00,0x00,0x00},  //[49]show"Start test EPD"
                                    {0x90,0xCC,0x00,0x00,0x00},  //[50]show"Start test sleep mode"
                                    
                                    //{0x90,0xCD,0x00,0x00,0x00},  //[51]show"Start test SuperCap" X
                                    {0x90,0xC0,0x00,0x00,0x00},  //[51]End condition
                                    
                                    {0x90,0xD1,0x00,0x00,0x00},  //[52]show"Start test battery1 switch"
                                    {0x90,0xD2,0x00,0x00,0x00},  //[53]show"Start test battery2 switch"
                                    {0x90,0xD3,0x00,0x00,0x00},  //[54]show"Start test solar battery switch"
                                    
                                    {0x90,0xD4,0x00,0x00,0x00},  //[55]show"Start test LED reset"
                                    {0x90,0xD5,0x00,0x00,0x00},  //[56]show"Start test LED version"
                                    
                                    }; 
                                  
static uint8_t MTPReportindex[15]={0,6,8,10,12,19,26,27,29,31,32,33,36};                    
                                    
//static uint16_t crctemp[4];


    
static initFunctionList mInitFunctionList[] =  {{"EpdDrv", EpdDrvInit, FALSE},
                                                {"LEDDrv", LedDrvInit, FALSE},
                                                {"BuzzerDrv",BuzzerDrvInit,FALSE},
                                                {"GUIDrv", GUIDrvInit, FALSE},
                                                {"CardReader", CardReaderInit, FALSE},
                                                {"PowerDrv", PowerDrvInit, FALSE},
                                                {"FlashDrv", FlashDrvExInit, FALSE},
                                                {"QModem", modeminit, FALSE},
                                                {"", NULL,FALSE}};	

static HWTesterItem   mainTestItem[] = {
{'v',   "Version Check", versionQueryTool},
{'a',   "All Test",     totalTest},
{'s',   "Single Test",  singleTest},
{'t',   "Tools",        toolsFunction},
{'b',   NULL,          blankFunction},
//{'d',   "Auto Test",autoTest},
//{'e',   "Exit Test",   exitTest},
{0,     NULL,               NULL}};

static HWTesterItem   MB_mainTestItem[] = {
{'a',   "All Test",     totalTest},
{'s',   "Single Test",  singleTest},
{'t',   "Tools",        toolsFunction},
{'b',   NULL,          blankFunction},
{0,     NULL,               NULL}};


static char ReaderitemName[20] = "Reader" ;
//static char* ReaderitemName = "Reader" ; Octopus

static HWTesterItem   singleTestItem[] = { 
{'1',   "Buzzer",          buzzerTest},
{'2',   "Key Pad",         keyPadAllTest},
{'3',   "LED",             LEDTestLite},
{'4',   "Switch",      redSwitchAllTest},																					
{'5',   "Battery",         batteryTest},
{'6',   "Credit Card",      CADReaderTest},
{'7',   "Mems",          MemsTest},
{'8',   "Modem\\RTC",      modemTest},
{'9',   "Flash",    sFlashTest},
//{'a',   "Radar\\Lidar",      radarSingleTest},
{'a',   "Radar",        NewRadarSingleTest},
{'b',   "Smart Card",      smartCardTest},
{'c',   "Camera",     cameraTest},
{'d',   ReaderitemName,          readerTest},
{'q',   "Quit", exitTest},
{0,     NULL,               NULL}};


static HWTesterItem   AllTestItem[] = { 
{'1',   "Buzzer",          buzzerTest},
{'2',   "Key Pad",         keyPadAllTest},
{'3',   "LED",             LEDTestLite},
{'4',   "Switch",      redSwitchAllTest},																					
{'5',   "Battery",         batteryTest},
{'6',   "Credit Card",      CADReaderTest},
{'7',   "Mems",          MemsTest},
{'8',   "Modem\\RTC",      modemTest},
{'9',   "Flash",    sFlashTest},
//{'a',   "Radar\\Lidar",      radarSingleTest},
{'a',   "Radar",        NewRadarSingleTest},
{'b',   "Smart Card",      smartCardTest},
{'c',   "Camera",     cameraTest},
{'q',   "Quit", exitTest},
{0,     NULL,               NULL}};

static HWTesterItem   MB_singleTestItem[] = { 
{'1',   "Battery",          batteryTest},
{'2',   "Buzzer",           buzzerTest},
{'3',   "LED",              LEDTest},
{'4',   "Credit Card Power",CADPowerTest},
{'5',   "Key Pad",          keyPadAllTest},
{'6',   "Switch",           redSwitchAllTest},
{'7',   "Credit Card",      CADSingleTest},
{'8',   "Reader & Modem",   otherIOTest},
{'9',   "Sensor",           sensorSingleTest},
{'a',   "RTC",              RTCTest},
{'b',   "EPD",              EPDSingleTest},
{'c',   "CPU Sleep",        suspendSystem},
//{'d',   "SuperCap",         SuperCapTest},
{'q',   "Quit",             exitTest},
{0,     NULL,               NULL}};

//{'a',   "Reader Flash",     sFlashTest},
//{'7',   "Credit Card",      CADSingleTest},
//{'8',   "Modem & Reader",   ModemAndReaderTest},
//{'9',   "sensor",           radarSingleTest},


//{'2',   "Smart Card",      smartCardTest},
//{'3',   "Flash",    sFlashTest},
//{'2',   "Modem",            modemTest},


static HWTesterItem   toolsFunctionItem[] = {   
//{'1',   "Adjust Key Pad",     toolsAdjustKeypad},  
//{'2',   "ID Config",          idConfig}, 
//{'3',   "12V Power",          enable12vPower}, 
//{'4',   "Suspend System",     suspendSystem},
//{'5',   "Battery Select",     batterySelect},
//{'6',   "Buzzer Loop",        buzzerLoop},
//{'7',   "Modem Set to High Speed",    modemSetHighSpeed},
{'1',   "Set Device ID",    setDeviceIDTool},
{'2',   "Get Device ID",    getDeviceIDTool},
{'3',   "CardRead Show ID", readerGetCN},
{'4',   "New Radar Tool",   NewRadarTool},
//{'4',   "New Radar Test",   NEWradarTest},
//{'4',   "Radar Test",       radarTest},
{'5',   "Radar Power Set",  radarPowerSet},
{'6',   "USB CAM Test",     usbCamTest},
{'7',   "USB CAM Power Set", usbCamPowerSet},
//{'8',   "Lidar Test",       lidarTest},
//{'9',   "Lidar Power Set",  lidarPowerSet},
//{'8',   "EPD Burning Test", epdBurningTest},
{'8',   "Calibration",      calibrationConfig},
//{'c',   "versionQueryTool", versionQueryTool},
{'9',   "EPD Tool", EPDflashTool},
{'a',   "Radar OTA", RadarOTATool},
{'b',   "KeyPad Tool", KeyPadTool},
{'c',   "Set Modem to High Speed", ModemHSpdTool},
{'d',   "Set RTC", RTCTool},
//{'e',   "New Radar Set", NEWradarSet},
//{'e',   "Modem Tool", ModemTool},
//{'f',   "NTP Tool", NTPTool},
{'q',   "Quit",            exitTest },//exitTest
//{'d',   NULL,               blankFunction},
{0,     NULL,               NULL}};

static ListItem calibrationListItem[] = {
//{'0',"Gyroscope"},
{'0',"MEMS"},
{'1',"Lidar1"},
{'2',"Lidar2"},
{'q',"Quit"},
//{'2',"Radar"},
{0,     NULL}
};


static ListItem EPDflashToolListItem[] = {
//{'0',"query"},
{'0',"FACTORY TEST"},
//{'1',"SD BURN"},
{'1',"EPD VCOM"},
{'2',"EPD BURNING TEST"},
{'3',"EPD All BLACK"},
{'4',"EPD All WHITE"},
{'5',"EPD DISP"},
{'q',"Quit"},
{0,     NULL}
};

static ListItem EPDEmergentToolListItem[] = {
{'0',"EPD EMERGENT BURN"},
{'q',"Quit"},
{0,     NULL}
};

static ListItem RadarOTAToolListItem[] = {
{'0',"Radar1 OTA"},
{'1',"Radar2 OTA"},
{'q',"Quit"},
{0,     NULL}
};

static ListItem NewRadarToolListItem[] = {
{'0',"Set Default RadarA"},
{'1',"Set Default RadarB"},
{'2',"Set RadarA Parameter"},
{'3',"Set RadarB Parameter"},
{'4',"New Radar Test"},
{'5',"RadarA OTA"},
{'6',"RadarB OTA"},
{'q',"Quit"},
{0,     NULL}
};
/*
static ListItem NewRadarToolListItem[] = {
{'0',"Set RadarA Parameter"},
{'1',"Set RadarB Parameter"},
{'2',"New Radar Test"},
{'q',"Quit"},
{0,     NULL}
};
*/

typedef struct
{
    char    charItem;
    char*   itemName;
    BOOL    result;
}TestResult;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
/////////////////////Tool function/////////////////////////
static void SetMTPCRC(uint8_t index,uint8_t* MTPBuf)
{
    uint16_t crctemp;
    CRCTool_Init(2);
    //crctemp = CRCTool_Table(MTPBuf, MTPBuf[2]+3); 
    crctemp = CRCTool_Table(MTPBuf+18*index, *(MTPBuf+18*index+2)+3); 
    //MTPBuf[MTPBuf[2]+3] = crctemp >> 8;
    //MTPBuf[MTPBuf[2]+4] = crctemp & 0x00FF;
    *(MTPBuf + 18*index + *(MTPBuf+18*index+2)+3) = crctemp >> 8;
    *(MTPBuf + 18*index + *(MTPBuf+18*index+2)+4) = crctemp & 0x00FF;
}

static void InitMTPvalue(uint8_t index,uint8_t* MTPBuf)
{
    for(int k=0;k<(*(MTPBuf+18*index+2));k++)
        *(MTPBuf + 18*index + 3 + k) = 0x82;
}

static void MTPCmdprint(uint8_t index,uint8_t* MTPBuf)
{
    
    vTaskDelay(300/portTICK_RATE_MS);
    for(int k=0;k<(*(MTPBuf+18*index+2)+5);k++)
    {
        while ((inpw(REG_UART0_FSR) & (1<<23))); //waits for TX_FULL bit is clear
            outpw(REG_UART0_THR, *(MTPBuf+18*index+k));
    }
    vTaskDelay(100/portTICK_RATE_MS);
    /*    
    for(int k=0;k<(*(MTPBuf+2)+5);k++)
    {
        while ((inpw(REG_UART0_FSR) & (1<<23))); //waits for TX_FULL bit is clear
            outpw(REG_UART0_THR, *(MTPBuf+k));
    }
    */
}
static void MTPCmdprintEx(uint8_t index,uint8_t* MTPBuf)
{
    
    //vTaskDelay(300/portTICK_RATE_MS);
    for(int k=0;k<(*(MTPBuf+18*index+2)+5);k++)
    {
        while ((inpw(REG_UART0_FSR) & (1<<23))); //waits for TX_FULL bit is clear
            outpw(REG_UART0_THR, *(MTPBuf+18*index+k));
    }
    //vTaskDelay(100/portTICK_RATE_MS);

}

static UINT8 getTerminalChar(void)
{
    UINT8 charTmp;
    charTmp = sysGetChar();
//    terninalPrintf("getTerminalChar: %c ", charTmp);
    if((charTmp >= 'A') && (charTmp <= 'Z'))
    {
        return charTmp + 'a' - 'A';
    }
    else if(((charTmp >= 'a') && (charTmp <= 'z')) || ((charTmp >= '0') && (charTmp <= '9')) ||  (charTmp == ' '))
    {
        return charTmp;
    }
    else
    {
        return 0;
    }
}

static char userResponseLoop(void)
{
    while(sysIsKbHit())
    {//empty registor buffer
#include <TMPA900.H>
        getTerminalChar();
    }
    while(guiResponseFlag)
    {//empty registor buffer
        getGuiResponse();
    }
    while(1)
    {//wait user respone
        vTaskDelay(100/portTICK_RATE_MS);
        if(sysIsKbHit())
        {
            return getTerminalChar();
        }
        if(guiResponseFlag)
        {
            return getGuiResponse();
        }
    }
}

static char userResponse(void)
{
    char reVal = FALSE;
    if(sysIsKbHit())
    {
        reVal = getTerminalChar();
    }
    if(guiResponseFlag)
    {
        reVal = getGuiResponse();
    }
    while(sysIsKbHit())
    {//empty registor buffer
        getTerminalChar();
    }
    while(guiResponseFlag)
    {//empty registor buffer
        getGuiResponse();
    }
    return reVal;
}


static char superResponseLoop(void)
{
    while(sysIsKbHit())
    {//empty registor buffer
        sysGetChar();
    }
    while(guiResponseFlag)
    {//empty registor buffer
        getGuiResponse();
    }
    while(1)
    {//wait user respone
        vTaskDelay(100/portTICK_RATE_MS);
        if(sysIsKbHit())
        {
            return sysGetChar();
        }
        if(guiResponseFlag)
        {
            return getGuiResponse();
        }
    }
}

static char superResponseLoopEx(void)
{
    while(sysIsKbHit())
    {//empty registor buffer
        sysGetChar();
    }
    while(guiResponseFlag)
    {//empty registor buffer
        getGuiResponse();
    }
    while(1)
    {//wait user respone
        vTaskDelay(100/portTICK_RATE_MS);
        if(sysIsKbHit())
        {
            TouchPadResponseFlag = FALSE;
            return sysGetChar();
        }
        if(guiResponseFlag)
        {
            TouchPadResponseFlag = TRUE;
            return getGuiResponse();
        }
    }
}

#if (!ENABLE_BURNIN_TESTER)
static BOOL LEDColorBuffSet(UINT16 GreenPinBite, UINT16 RedPinBite)
#else
BOOL LEDColorBuffSet(UINT16 GreenPinBite, UINT16 RedPinBite)
#endif
{
    UINT8 i;
    for(i=0;i<=5;i++)
    {
        if(((RedPinBite >> i)& 0x1) == 1)
        {
            bayColorChange[i] = LIGHT_COLOR_RED;
        }
        else if(((GreenPinBite >> i)& 0x1) == 1)
        {
            bayColorChange[i] = LIGHT_COLOR_GREEN;
        }
        else
        {
            bayColorChange[i] = LIGHT_COLOR_OFF;
        }
    }
    return TRUE;
}

#if (!ENABLE_BURNIN_TESTER)
static BOOL LEDBoardLightSet(void)
#else
BOOL LEDBoardLightSet(void)
{
    //setPrintfFlag(FALSE);    
    LedSetColor(bayColorChange, LIGHT_COLOR_OFF, TRUE); 
    //setPrintfFlag(TRUE);
    return TRUE;
}
#endif
static uint8_t guiManagerShowScreen(uint8_t guiId, uint8_t para, int para2, int para3)
{
    if(isEPDInit)
    {
        return GuiManagerShowScreen(guiId,para,para2,para3);
    }
    return 0;
}
static BOOL guiManagerRefreshScreen(void)
{
    if(isEPDInit)
    {
        return GuiManagerRefreshScreen();
    }
    return FALSE;
}
static BOOL guiManagerResetKeyCallbackFunc(void)
{
    if(isEPDInit)
    {
        return GuiManagerResetKeyCallbackFunc();
    }
    return FALSE;
}
static BOOL guiManagerCompareCurrentScreenId(uint8_t guiId)
{
    if(isEPDInit)
    {
        return GuiManagerCompareCurrentScreenId(guiId);
    }
    return FALSE;
}
//static BOOL guiManagerResetInstance(void){
//    if(isEPDInit)
//        return GuiManagerResetInstance();
//    return FALSE;
//}
#define X_POS_RST 550
#define Y_POS_RST 154
#define X_POS_MSG 550
#define Y_POS_MSG 354 //304

#define X_RETRY_BAR_MSG 550
#define Y_RETRY_BAR_MSG 556

static void guiPrintResult(char* str)
{
    if(isEPDInit)
    {
        //vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(FALSE,"                 \n                \n",X_POS_RST,Y_POS_RST);
        EPDDrawString(TRUE ,str,X_POS_RST,Y_POS_RST);
    }
}

static void guiPrintMessage(char* str)
{
    if(isEPDInit)
    {
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CLEAN_BAR,X_POS_MSG-2,Y_POS_MSG-2);
        EPDDrawString(TRUE,str,X_POS_MSG,Y_POS_MSG);
    }
}
static void guiPrintMessageNoClean(char* str)
{
    if(isEPDInit)
    {
        //EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CLEAN_BAR,X_POS_MSG-2,Y_POS_MSG-2);
        EPDDrawString(TRUE,str,X_POS_MSG,Y_POS_MSG);
    }
}
static void cleanMsg(void){
    if(isEPDInit)
    {
        //CLEAR MSG BAR//
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CLEAN_BAR,X_POS_MSG-2,Y_POS_MSG-2);
    }
}

static void cleanRst(void){
    if(isEPDInit)
    {
        //CLEAR MSG BAR//
        EPDDrawString(FALSE,"         ",X_POS_RST+200,Y_POS_RST-50);
        EPDDrawString(TRUE,"                 \n                \n",X_POS_RST,Y_POS_RST);
    }
}

static void SolarValtageCalibration(void)
{
    char tempchar;
    char* numberString = NULL;
    UINT8 firststringflag = 1;
    UINT8 changenumberflag = 0;

    char str[20];
   

    int number = 0;
    //terninalPrintf("\r\nLast VCOM value = %d \r\n",IT8951GetVCOM());
    terninalPrintf("Please enter AjustSolarBatFactor Value in decimal, now value = %d\r\n",ShowAjustSolarBatFactor());
    terninalPrintf("Enter number is ");
    

    
    char charTmp;
    unsigned int idTmp=0,i=0;

    while(1){
        //charTmp=userResponseLoop();
        
        charTmp = superResponseLoop();
        
        if(charTmp>='0'&&charTmp<='9')
        {
            idTmp=idTmp*10;
            idTmp+=(charTmp-'0');
            i++;
            terninalPrintf("%c",charTmp);
        }
        if(charTmp == 0x0D)
        {
            ModifyAjustSolarBatFactor(idTmp);
            break;
        }
        if((charTmp=='y') || (charTmp=='q'))
        {
            break;
        }
        if(i==5)
        {
            ModifyAjustSolarBatFactor(idTmp);           
            break;
        }
    }
    terninalPrintf("\r\n");
    
    vTaskDelay(100/portTICK_RATE_MS);
    
    
}

static BOOL SetEPDVCOM(void)
{
    char tempchar;
    char* numberString = NULL;
    UINT8 firststringflag = 1;
    UINT8 changenumberflag = 0;

    char str[20];
   

    int number = 0;
    //terninalPrintf("\r\nLast VCOM value = %d \r\n",IT8951GetVCOM());
    terninalPrintf("Please enter EPD VCOM Value in decimal.\r\n");
    terninalPrintf("Enter number is ");
    
    /*
    do
    {   
        tempchar = superResponseLoop();  

        terninalPrintf("%c",tempchar);
        if((tempchar >= 48) && (tempchar <= 57))   
        { 
            changenumberflag = 1;
            if(firststringflag == 0)
                sprintf(numberString,"%s%c",numberString,tempchar);
            else
            {
                sprintf(numberString,"%c",tempchar);
                firststringflag = 0;
            }
                
        }

    }while(tempchar != 0x0D);

    if (changenumberflag)
        number = atoi(numberString);

    terninalPrintf("\r\n");
    if (changenumberflag)
        IT8951SetVCOM(number);
    
    */
    
    //terninalPrintf("New VCOM value = %d \r\n",IT8951GetVCOM());
    
    char charTmp;
    unsigned int idTmp=0,i=0;
    //terninalPrintf("Current Device ID: %05d\nPlease Enter 5 Digit Number To Set Device ID\n",getDeviceID(0,0));
    //terninalPrintf("Press 'q' to quit without save...\n");
    while(1){
        //charTmp=userResponseLoop();
        
        charTmp = superResponseLoop();
        
        if(charTmp>='0'&&charTmp<='9')
        {
            idTmp=idTmp*10;
            idTmp+=(charTmp-'0');
            i++;
            terninalPrintf("%c",charTmp);
        }
        if(charTmp == 0x0D)
        {
            //setDeviceID(&epmIDSet,NULL);//empIDSet is set by guisettingid
            *setVCOMpoint = idTmp;
            IT8951SetVCOM(idTmp);
            break;
        }
        if((charTmp=='y') || (charTmp=='q'))
        {
            break;
        }
        if(i==5)
        {
            //setDeviceID(&idTmp,NULL);
            *setVCOMpoint = idTmp;
            IT8951SetVCOM(idTmp);
            break;
        }
    }
    terninalPrintf("\r\n");
    
    vTaskDelay(100/portTICK_RATE_MS);
    
    return TRUE;


}


static BOOL SetEPDVCOMEx(void)
{
    char tempchar;
    char* numberString = NULL;
    UINT8 firststringflag = 1;
    UINT8 changenumberflag = 0;

    char str[20];
   

    int number = 0;
    //terninalPrintf("\r\nLast VCOM value = %d \r\n",IT8951GetVCOM());
    terninalPrintf("Please enter EPD VCOM Value in decimal.\r\n");
    terninalPrintf("Enter number is ");
    

    
    char charTmp;
    unsigned int idTmp=0,i=0;

    while(1){
        //charTmp=userResponseLoop();
        
        charTmp = superResponseLoop();
        
        if(charTmp>='0'&&charTmp<='9')
        {
            idTmp=idTmp*10;
            idTmp+=(charTmp-'0');
            i++;
            terninalPrintf("%c",charTmp);
        }
        if(charTmp == 0x0D)
        {
            //setDeviceID(&epmIDSet,NULL);//empIDSet is set by guisettingid
            //*setVCOMpoint = idTmp;
            setVCOMEx = idTmp;
            
            //IT8951SetVCOM(idTmp);
            break;
        }
        if((charTmp=='y') || (charTmp=='q'))
        {
            break;
        }
        if(i==5)
        {
            //setDeviceID(&idTmp,NULL);
            //*setVCOMpoint = idTmp;
            setVCOMEx = idTmp;
            //IT8951SetVCOM(idTmp);
            break;
        }
    }
    terninalPrintf("\r\n");
    
    vTaskDelay(100/portTICK_RATE_MS);
    
    return TRUE;


}


static BOOL W25Q64BVQuery(void)
{
    char tempchar;
    char* numberString = NULL;
    UINT8 firststringflag = 1;
    UINT8 changenumberflag = 0;

    char str[20];
   

    int number = 0;
    
    terninalPrintf("Please enter flash memory address in decimal.\r\n");
    terninalPrintf("Enter number is ");
    do
    {   
        tempchar = superResponseLoop();  

        terninalPrintf("%c",tempchar);
        if((tempchar >= 48) && (tempchar <= 57))   
        { 
            changenumberflag = 1;
            if(firststringflag == 0)
                sprintf(numberString,"%s%c",numberString,tempchar);
            else
            {
                sprintf(numberString,"%c",tempchar);
                firststringflag = 0;
            }
                
        }

    }while(tempchar != 0x0D);

    if (changenumberflag)
        number = atoi(numberString);

    terninalPrintf("\r\n");
    if (changenumberflag)
        W25Q64BVquery(number);
    
    vTaskDelay(100/portTICK_RATE_MS);
    
    return TRUE;


}


static BOOL W25Q64BVerase(void)
{
    int waitime = 20;
    int waitCounter = 30;
    int retryTime = 0;
    W25Q64BVErase();

    GPIO_ClrBit(GPIOI, BIT0); 
    vTaskDelay(100/portTICK_RATE_MS);
    do{
        waitime = 20 - retryTime;
        if(waitime <= 0)
            waitime = 0;
        terninalPrintf("W25Q64BV erasing,please wait %d second...\r",waitime);
        if(SpiFlash_WaitEraseReady())
            break;
        retryTime++;
        waitCounter--;
    }
    while(waitCounter > 0);

    GPIO_SetBit(GPIOI, BIT0);
    if(waitCounter <= 0)
        return FALSE;
    else
        return TRUE;
    
    
    
    /*
    int waitime = 15;
    W25Q64BVErase();

    for(int i=0;i<waitime;i++)
    {
        terninalPrintf("W25Q64BV erasing,please wait %d second...\r",waitime-i);
        vTaskDelay(1000/portTICK_RATE_MS);
    }  
    terninalPrintf("W25Q64BV erase complete.                      \r\n");


    return TRUE;

    */
}

static BOOL W25Q64BVburn(void)
{

    
    
        char tempchar;
        char* numberString = NULL;
        UINT8 firststringflag = 1;
        UINT8 changenumberflag = 0;
    
        int binselect;
        //int count = 2560;
        //uint8_t u8DataBuffer[count];
        int  SDbufferSize = 2560 ;
        uint8_t SDbuffer[SDbufferSize];
        FIL file;
        UINT br;
        char * binString;
    
        
        terninalPrintf("Open SD card bin file DefaultTest.bin .\r\n");
        /*
        //terninalPrintf("Please select bin file. 0:DefaultTest_1.01.06.bin  1:DefaultTest.bin\r\n");
        terninalPrintf("Please select bin file. 1:DefaultTest.bin\r\n");
        terninalPrintf("Enter number is ");
        do
        {   
            tempchar = superResponseLoop();  

            terninalPrintf("%c",tempchar);
            if((tempchar >= 48) && (tempchar <= 57))   
            { 
                changenumberflag = 1;
                if(firststringflag == 0)
                    sprintf(numberString,"%s%c",numberString,tempchar);
                else
                {
                    sprintf(numberString,"%c",tempchar);
                    firststringflag = 0;
                }
                    
            }

        }while(tempchar != 0x0D);
        
        terninalPrintf("\r\n");
    
        if (changenumberflag)
        binselect = atoi(numberString);
        
    
        if(binselect == 1)
        {
            binString = "0:DefaultTest.bin"; //"0:DefaultTest_1.01.06.bin" ;
        }
        else
        {
            binString = "0:DefaultTest.bin" ;
        }
        */
        
        binString = "0:DefaultTest.bin";
        

        
        
        
        
        if(!UserDrvInit(FALSE))
        {
            terninalPrintf("UserDrvInit fail.\r\n");
            return FALSE;
        }
        if(!FatfsInit(TRUE))
        {
            terninalPrintf("FatfsInit fail.\r\n");
            return FALSE;
        }
        //if(f_open(&file, "0:DefaultTest_1.01.06.bin", FA_OPEN_EXISTING |FA_READ))
        if(f_open(&file, binString, FA_OPEN_EXISTING |FA_READ))
        {
            terninalPrintf("SD card file open fail.\r\n");
            return FALSE;
        }
        
        if(W25Q64BVerase() == FALSE)
        {
            terninalPrintf("Erase fail.\r\n");
            return FALSE;
        }

        int count = file.fsize / SDbufferSize;
        int remain = file.fsize % SDbufferSize;
        int progress = count / 10 ;
        
        //terninalPrintf("file.fsize = %d\r\n",file.fsize);
        //terninalPrintf("count = %d\r\n",count);
        //terninalPrintf("remain = %d\r\n",remain);
        //terninalPrintf("progress = %d\r\n",progress);
        terninalPrintf("W25Q64BV burning...\r\n");
        vTaskDelay(100/portTICK_RATE_MS);
        GPIO_ClrBit(GPIOI, BIT0); 
        //vTaskDelay(10/portTICK_RATE_MS);
        vTaskDelay(100/portTICK_RATE_MS);
        
        for(int i=0;i<count;i++)
        {
            if(i%progress == 0)
            {
                terninalPrintf("%d%% complete...\r",(i/progress)*10);
            }
            
            f_read(&file, SDbuffer, 2560, &br);
            W25Q64BVBurn(i*SDbufferSize,(uint8_t *) SDbuffer,sizeof(SDbuffer) );
        }
        
        if(remain != 0)
        {
            f_read(&file, SDbuffer, remain, &br);
            W25Q64BVBurn(count*SDbufferSize,(uint8_t *) SDbuffer,remain );
        }
        
        GPIO_SetBit(GPIOI, BIT0);
        
        
        
        /*
        for(int i=0;i<count;i++)
        {
            SDbuffer[i] = i%256;   
        }  */


        /*for(int k=0;k<10;k++)
        {
            terninalPrintf("%d%% complete...\r",(k+1)*10);
            for(int j=0;j<325;j++)
                {
                    W25Q64BVBurn(k*325*SDbufferSize+j*SDbufferSize,(uint8_t *) SDbuffer,sizeof(SDbuffer) );
                }
        }  */
        terninalPrintf("\r\n");

        vTaskDelay(100/portTICK_RATE_MS);
        f_close(&file);
        
        return TRUE;

}


static BOOL W25Q64BVFetch(void)
{

    

        char tempchar;
        char* numberString = NULL;
        UINT8 firststringflag = 1;
        UINT8 changenumberflag = 0;
    
        int binselect;
        //int count = 2560;
        //uint8_t u8DataBuffer[count];
        //int  SDbufferSize = 2560 ;
        int totalSize;
        int  SDbufferSize = 2000 ;
        uint8_t SDbuffer[SDbufferSize];
        FIL file,fileread;
        //FIL file;
        UINT br;
        char * binString;
    
        
        terninalPrintf("\r\n");
        
        binString = "0:DefaultTest.bin";
        
        
        
        
        
        if(!UserDrvInit(FALSE))
        {
            terninalPrintf("UserDrvInit fail.\r\n");
            return FALSE;
        }
        if(!FatfsInit(TRUE))
        {
            terninalPrintf("FatfsInit fail.\r\n");
            return FALSE;
        }
        
        //if(f_open(&file, "0:DefaultTest_1.01.06.bin", FA_OPEN_EXISTING |FA_READ))
        if(f_open(&file, binString, FA_OPEN_EXISTING |FA_READ))
        {
            terninalPrintf("SD card file open %s fail.\r\n",binString);
            return FALSE;
        }
        
        
        totalSize = file.fsize;
        int count = file.fsize / SDbufferSize;
        int remain = file.fsize % SDbufferSize;
        int progress = count / 10 ;

        
        
        
        
        if(f_open(&fileread,"0:EPDdebug.bin", FA_CREATE_ALWAYS | FA_WRITE))
        {
            terninalPrintf("SD card file open EPDdebug.bin fail.\r\n");
            return FALSE;
        }
       
        terninalPrintf("file.fsize = %d\r\n",totalSize);
        terninalPrintf("count = %d\r\n",count);
        terninalPrintf("remain = %d\r\n",remain);
        terninalPrintf("progress = %d\r\n",progress);
        //terninalPrintf("W25Q64BV burning...\r\n");
         
         
         //int count = 50;
         //int progress = 5;
         //int remain  = 3;
        
        GPIO_ClrBit(GPIOI, BIT0); 
        vTaskDelay(10/portTICK_RATE_MS);
         
        for(int i=0;i<count;i++)
        {
            if(i%progress == 0)
            {
                terninalPrintf("%d%% complete...\r",(i/progress)*10);
            }
            
            //f_read(&file, SDbuffer, 2560, &br);
            //W25Q64BVBurn(i*SDbufferSize,(uint8_t *) SDbuffer,sizeof(SDbuffer) );
            //terninalPrintf("i = %d\r\n",i);
            W25Q64BVqueryEx(i*SDbufferSize,SDbuffer,sizeof(SDbuffer));
            //W25Q64BVquery(i);
            //W25Q64BVqueryEx(i*SDbufferSize,NULL,sizeof(SDbuffer));
            f_write(&fileread, SDbuffer, SDbufferSize, &br);
            
        }
        
        if(remain != 0)
        {   
            //f_read(&file, SDbuffer, remain, &br);
            //W25Q64BVBurn(count*SDbufferSize,(uint8_t *) SDbuffer,remain );
            
            W25Q64BVqueryEx(count*SDbufferSize,SDbuffer,remain);
            f_write(&fileread, SDbuffer, remain, &br);
            
        }
        
        
        GPIO_SetBit(GPIOI, BIT0);
        
        

        terninalPrintf("\r\n");

        vTaskDelay(100/portTICK_RATE_MS);
        f_close(&file);
        f_close(&fileread);
        return TRUE;

}



static BOOL RadarOTAprogramTool(int radarIndex)
{
    char tempchar;
    char* numberString = NULL;
    UINT8 firststringflag = 1;
    UINT8 changenumberflag = 0;

    int binselect;
    int  SDbufferSize = 256 ;
    uint8_t SDbuffer[SDbufferSize];
    FIL file;
    UINT br;
    char * binString;
    if(radarIndex)
    {
        EPDDrawString(FALSE,"            ",700,100+(44*(2)));
        EPDDrawString(FALSE,"Wait.       ",700,100+(44*(3)));
    }
    else
    {
        EPDDrawString(FALSE,"Wait.       ",700,100+(44*(2)));
        EPDDrawString(FALSE,"            ",700,100+(44*(3)));
    }
    EPDDrawString(FALSE,"                                 ",50,100+(44*(8)));
    EPDDrawString(FALSE,"                                 ",50,100+(44*(9)));
    EPDDrawString(FALSE,"                                 ",50,100+(44*(10)));
    EPDDrawString(TRUE,"                                 ",50,100+(44*(11)));
    
    static RadarInterface* pRadarInterface;

    pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);

    if(pRadarInterface == NULL)
    {
        terninalPrintf("radarTest ERROR (pRadarInterface == NULL)!!\n");
    }

    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("radarTest ERROR (initFunc false)!!\n");
    }
    
    
    
    
    
    
    /*
    terninalPrintf("Please select bin file. 0:OTA_Test.bin  1:RadarLidar_V1.0.46.Beta.bin  2:RadarLidar_V1.0.44.Beta.bin  ");
    terninalPrintf("3:RadarLidar_1.0.42.Beta.bin  4:RadarLidar_1.0.41.Beta.bin  5:RadarLidar_1.0.40.Beta.bin\r\n");
    terninalPrintf("Enter number is ");
    do
    {   
        tempchar = superResponseLoop();  

        terninalPrintf("%c",tempchar);
        if((tempchar >= 48) && (tempchar <= 57))   
        { 
            changenumberflag = 1;
            if(firststringflag == 0)
                sprintf(numberString,"%s%c",numberString,tempchar);
            else
            {
                sprintf(numberString,"%c",tempchar);
                firststringflag = 0;
            }
                
        }

    }while(tempchar != 0x0D);
    
    terninalPrintf("\r\n");

    if (changenumberflag)
    binselect = atoi(numberString);  */
   /* 
    for(int s=0;s<100;s++)
    {
    
    terninalPrintf("s = %d \r\n",s);
    binselect = (s%5) + 1 ; 
        
    terninalPrintf("Enter number = %d \r\n",binselect);  */
/*
    switch(binselect) { 
        case 0: 
            binString = "0:RadarLidar_V1.0.46.Beta.bin"; //"0:OTA_Test.bin";
            break;
        case 1: 
            binString = "0:RadarLidar_V1.0.46.Beta.bin"; 
            break; 
        case 2: 
            binString = "0:RadarLidar_V1.0.44.Beta.bin"; 
            break; 
        case 3: 
            binString = "0:RadarLidar_1.0.42.Beta.bin"; 
            break; 
        case 4: 
            binString = "0:RadarLidar_1.0.41.Beta.bin"; 
            break; 
        case 5: 
            binString = "0:RadarLidar_1.0.40.Beta.bin"; 
            break; 

        default: 
            binString = "0:RadarLidar_V1.0.46.Beta.bin"; 
    } */
    
    //terninalPrintf("binString = %s \r\n",binString);
    vTaskDelay(5000/portTICK_RATE_MS);
    
    binString = "0:OTA.bin";
    
    
    
    pRadarInterface->setPowerStatusFunc(radarIndex,FALSE);
    vTaskDelay(500/portTICK_RATE_MS);
    pRadarInterface->setPowerStatusFunc(radarIndex,TRUE);
    vTaskDelay(3000/portTICK_RATE_MS);
    
    char templastString[50] ;
    char tempRadarlastVersionString[50];
    int lastretryCounter = 3;
    
    
    while(lastretryCounter>0)
    {
        if(pRadarInterface -> readQueryVersionString(radarIndex, tempRadarlastVersionString))
            break;
        lastretryCounter--;
    }

    if(lastretryCounter>0)
    {
        //terninalPrintf("\r");
        terninalPrintf("Last Radar%d Version = %s\r\n",radarIndex+1,tempRadarlastVersionString);
        sprintf(templastString,"Radar%d last Ver.:%s",radarIndex+1,tempRadarlastVersionString);
    }
    else
    {
        terninalPrintf("Last Radar%d Version = Error\r\n",radarIndex+1);
        sprintf(templastString,"Radar%d last Ver.:Error",radarIndex+1);
    }
    
        EPDDrawString(FALSE,templastString,50,100+(44*(radarIndex*2+8)));
        EPDDrawString(TRUE,"                                 ",50,100+(44*(radarIndex*2+9)));
    
   // vTaskDelay(3000/portTICK_RATE_MS);
    
    
    
    
    terninalPrintf("Open file \"%s\" \r\n",binString);
    
    
    
    
    /*if(!UserDrvInit(FALSE))
    {
        terninalPrintf("UserDrvInit fail.\r\n");
        EPDDrawString(TRUE,"Fail.       ",700,100+(44*(radarIndex+2)));
        return FALSE;
    }*/
    
    //terninalPrintf("-\r\n");
    //setPrintfFlag(FALSE);
    /*//if(firstInitFlag)
    //{
    //    firstInitFlag = FALSE;
        if(!FatfsInit(TRUE))
        {
            terninalPrintf("FatfsInit fail.\r\n");
            EPDDrawString(TRUE,"Fail.       ",700,100+(44*(radarIndex+2)));
            return FALSE;
        }
    //} */
    //terninalPrintf("--\r\n");
    //if(f_open(&file, "0:DefaultTest_1.01.06.bin", FA_OPEN_EXISTING |FA_READ))
    if(f_open(&file, binString, FA_OPEN_EXISTING |FA_READ))
    {
        terninalPrintf("SD card file open fail.\r\n");
        f_close(&file);
        EPDDrawString(TRUE,"Fail.       ",700,100+(44*(radarIndex+2)));
        return FALSE;
    }
    
    //terninalPrintf("---\r\n");
    int count = file.fsize / SDbufferSize;
    int remain = file.fsize % SDbufferSize;
    int progress = count / 10 ;
    
    terninalPrintf("file.fsize = %d\r\n",file.fsize);
    terninalPrintf("count = %d\r\n",count);
    terninalPrintf("remain = %d\r\n",remain);
    terninalPrintf("progress = %d\r\n",progress);
    
    terninalPrintf("\r\r\r");

    
    

    
    
    
    
    
    
    
    
    

    //pRadarInterface->setPowerStatusFunc(radarIndex,TRUE);

    //vTaskDelay(1000/portTICK_RATE_MS);
    //terninalPrintf("-----\r\n");
    if(pRadarInterface->FirstOTAFunc(radarIndex,file.fsize) == FALSE)
    {
        terninalPrintf("Radar%d first OTA fail.\r\n",radarIndex+1);
        EPDDrawString(TRUE,"Fail.       ",700,100+(44*(radarIndex+2)));
        return FALSE;
    }   
    
    terninalPrintf("0%% complete...\r");
    EPDDrawString(TRUE,"0 percent   ",700,100+(44*(radarIndex+2)));
    char tempProgressString[13];
    for(int i=1;i<=count;i++)
    {
        //terninalPrintf("i=%d\r",i);
        //terninalPrintf(".");
        if(i%progress == 0)
        {
            terninalPrintf("%d%% complete...\r",(i/progress)*10);
            sprintf(tempProgressString,"%d percent ",(i/progress)*10);
            EPDDrawString(TRUE,tempProgressString,700,100+(44*(radarIndex+2)));
        } 
        
        f_read(&file, SDbuffer, SDbufferSize, &br);
       
        if(pRadarInterface->OTAFunc(radarIndex,SDbuffer, i, SDbufferSize) == FALSE)
        {
            f_close(&file);
            terninalPrintf("Radar%d OTA fail@%d.\r\n",radarIndex+1,i);
            EPDDrawString(TRUE,"Fail.       ",700,100+(44*(radarIndex+2)));
            return FALSE;
        }
       // W25Q64BVBurn(i*SDbufferSize,(uint8_t *) SDbuffer,sizeof(SDbuffer) );
    }
    
    if(remain != 0)
    {
        f_read(&file, SDbuffer, remain, &br);
        if(pRadarInterface->OTAFunc(radarIndex,SDbuffer, count + 1, remain) == FALSE)
        {
            f_close(&file);
            terninalPrintf("Radar%d OTA fail@%d.\r\n",radarIndex+1,count + 1);
            EPDDrawString(TRUE,"Fail.       ",700,100+(44*(radarIndex+2)));
            return FALSE;
        }
            
        //W25Q64BVBurn(count*SDbufferSize,(uint8_t *) SDbuffer,remain );
    }
    
    
    
    
    //vTaskDelay(1000/portTICK_RATE_MS);
    //pRadarInterface->setPowerStatusFunc(radarIndex,FALSE);
    
    f_close(&file);
    
    
    
    
    
    //vTaskDelay(5000/portTICK_RATE_MS);
    terninalPrintf("Radar%d OTA success.\r\n",radarIndex+1);
    EPDDrawString(TRUE,"Success.    ",700,100+(44*(radarIndex+2)));
    //pRadarInterface->setPowerStatusFunc(radarIndex,FALSE);
    
    vTaskDelay(7000/portTICK_RATE_MS);
    pRadarInterface->setPowerStatusFunc(radarIndex,FALSE);
    
    
    /*pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);

    if(pRadarInterface == NULL)
    {
        terninalPrintf("radarTest ERROR (pRadarInterface == NULL)!!\n");
    }

    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("radarTest ERROR (initFunc false)!!\n");
    }
    
    
    pRadarInterface->setPowerStatusFunc(radarIndex,TRUE);
    //vTaskDelay(1000/portTICK_RATE_MS);
    */
    vTaskDelay(3000/portTICK_RATE_MS);
    pRadarInterface->setPowerStatusFunc(radarIndex,TRUE);
    vTaskDelay(2000/portTICK_RATE_MS);
    //vTaskDelay(4000/portTICK_RATE_MS);

    char tempnewString[50] ;
    char tempRadarnewVersionString[50];
    int newretryCounter = 3;
    
    while(newretryCounter>0)
    {
        if(pRadarInterface -> readQueryVersionString(radarIndex, tempRadarnewVersionString))
            break;
        newretryCounter--;
    }

    if(newretryCounter>0)
    {
        //terninalPrintf("\r");
        terninalPrintf("New Radar%d Version = %s\r\n",radarIndex+1,tempRadarnewVersionString);
        sprintf(tempnewString,"Radar%d  new Ver.:%s",radarIndex+1,tempRadarnewVersionString);
    }
    else
    {
        terninalPrintf("New Radar%d Version = Error\r\n",radarIndex+1);
        sprintf(tempnewString,"Radar%d  new Ver.:Error",radarIndex+1);
    }
    
    pRadarInterface->setPowerStatusFunc(radarIndex,FALSE);
    EPDDrawString(TRUE,tempnewString,50,100+(44*(radarIndex*2+9)));
   /* if(radarIndex)      
        EPDDrawString(TRUE,tempString,100,100+(44*(11)));
    else
        EPDDrawString(TRUE,tempString,100,100+(44*(10)));
     */   
    
    //setPrintfFlag(TRUE);
    
    
    //terninalPrintf("New Radar%d Version = %s\r\n",radarIndex+1,tempRadarnewVersionString);
    
    /*
    char SucCoutString[50] ;
    
    sprintf(SucCoutString,"Success Count : %d   ",s+1);
    
    
    EPDDrawString(TRUE,SucCoutString,50,100+(44*(6)));
    
    
    }   */
    return TRUE;
}

static BOOL OpenCamPhoto(int index)
{
    
    //FIL filephoto;
    char * PhotoFileNameStr;
    if(!UserDrvInit(FALSE))
    {
        terninalPrintf("UserDrvInit fail.\r\n");
        return FALSE;
    }
    if(!FatfsInit(TRUE))
    {
        terninalPrintf("FatfsInit fail.\r\n");
        return FALSE;
    }
  
    
    //Time2RTC(GetCurrentUTCTime(), pptPHOTO);
    //terninalPrintf("Year/Month/Day/Hour/Minute/Second  %04d/%02d/%02d/%02d/%02d/%02d \r\n",ppt->u32Year,ppt->u32cMonth,ppt->u32cDay,ppt->u32cHour,ppt->u32cMinute,ppt->u32cSecond);
    
    //sprintf(PhotoFileNameStr,"0:photo%d_%s.jpg",index+1,timeStr);
    
    
    //sprintf(PhotoFileNameStr,"0:photo%d_%04d%02d%02d%02d%02d%02d.jpg",index+1,pptPHOTO->u32Year ,pptPHOTO->u32cMonth,pptPHOTO->u32cDay,pptPHOTO->u32cHour,pptPHOTO->u32cMinute,pptPHOTO->u32cSecond);
    
    //sprintf(PhotoFileNameStr,"0:photo%d_%02d%02d%02d%02d%02d.jpg",index+1,pptPHOTO->u32cMonth,pptPHOTO->u32cDay,pptPHOTO->u32cHour,pptPHOTO->u32cMinute,pptPHOTO->u32cSecond);
    
    sprintf(PhotoFileNameStr,"0:photo%d_%d.jpg",index+1,GetCurrentUTCTime());
    
    
    //terninalPrintf("PhotoFileNameStr = %s\r\n",PhotoFileNameStr);
    //sprintf(PhotoFileNameStr,"0:photo%d.jpg",index+1);
                    
    if(f_open(&filephoto,PhotoFileNameStr, FA_CREATE_ALWAYS | FA_WRITE))
    //if(f_open(&filephoto, PhotoFileNameStr, FA_OPEN_EXISTING |FA_READ))
    {
        terninalPrintf("SD card file open %s fail.\r\n",PhotoFileNameStr);
        return FALSE;
    }
    
    
}

static void CloseCamPhoto(void)
{
    f_close(&filephoto);

}

static BOOL SaveCamPhoto(int index,uint8_t* photoPr,int photoLen)
{
   // int  SDbufferSize = 256 ;
   // uint8_t SDbuffer[SDbufferSize];
    //FIL filephoto;
    //char * PhotoFileNameStr;
    //char  PhotoFileNameStr[50];
    //UINT br;
    
    //RTC_TIME_DATA_T* ppt;
    
    int count ;
    int remain ;
    int progress ;

    char * timeStr;
    /*
    if(!UserDrvInit(FALSE))
    {
        terninalPrintf("UserDrvInit fail.\r\n");
        return FALSE;
    }
    if(!FatfsInit(TRUE))
    {
        terninalPrintf("FatfsInit fail.\r\n");
        return FALSE;
    }
    */
    
    
    
    //UTCTimeToStringEx(GetCurrentUTCTime(), timeStr);
    //Time2RTC(GetCurrentUTCTime(), ppt);
    //terninalPrintf("timeStr = %s\r\n",timeStr);
    //terninalPrintf("Year/Month/Day/Hour/Minute/Second  %04d/%02d/%02d/%02d/%02d/%02d \r\n",ppt->u32Year,ppt->u32cMonth,ppt->u32cDay,ppt->u32cHour,ppt->u32cMinute,ppt->u32cSecond);
    
    //sprintf(PhotoFileNameStr,"0:photo%d_%s.jpg",index+1,timeStr);
    
    /*
    //sprintf(PhotoFileNameStr,"0:photo%d_%02d%02d%02d%02d%02d.jpg",index+1,ppt->u32cMonth,ppt->u32cDay,ppt->u32cHour,ppt->u32cMinute,ppt->u32cSecond);
    sprintf(PhotoFileNameStr,"0:photo%d_%d.jpg",index+1,GetCurrentUTCTime());
    //terninalPrintf("PhotoFileNameStr = %s\r\n",PhotoFileNameStr);
    //sprintf(PhotoFileNameStr,"0:photo%d.jpg",index+1);
                    
    if(f_open(&filephoto,PhotoFileNameStr, FA_CREATE_ALWAYS | FA_WRITE))
    //if(f_open(&filephoto, PhotoFileNameStr, FA_OPEN_EXISTING |FA_READ))
    {
        terninalPrintf("SD card file open %s fail.\r\n",PhotoFileNameStr);
        return FALSE;
    }
    */
    
    f_write(&filephoto, tempPr, photoLen, &brPHOTO);
    //f_close(&filephoto);
    
    
    //count = photoLen / SDbufferSize;
    //remain = photoLen % SDbufferSize;
    //progress = count / 10 ;
    /*
    terninalPrintf("count = %d\r\n",count);
    terninalPrintf("remain = %d\r\n",remain);
    terninalPrintf("progress = %d\r\n",progress);
    */
    //uint8_t tempBuffer[] = {0x7A,0xA7,index};
    /*
    if(index == 1)
        tempBuffer[2] = 0x02;
    else
        tempBuffer[2] = 0x01;
    */
    //f_write(&filephoto, tempBuffer, sizeof(tempBuffer), &br);

    
    //terninalPrintf("photoPrAddr = %08x \r\n",photoPr);
    /*
                    terninalPrintf("nphotoPr = ");
                for(int j=0;j<10;j++)
                    terninalPrintf("%02x ",*(photoPr+j));
        
                terninalPrintf("\r\n");
    */
    
    /*
    for(int i=0;i<count;i++)
    {
        if(i%progress == 0)
        {
            terninalPrintf("%d%% complete...\r",(i/progress)*10);
        }
        
        //W25Q64BVqueryEx(i*SDbufferSize,SDbuffer,sizeof(SDbuffer));
        //memcpy(SDbuffer,*(&photoPr+i*SDbufferSize) , SDbufferSize);
        
        //memcpy(SDbuffer,photoPr+i*SDbufferSize , SDbufferSize);
        for(int j=0;j<SDbufferSize;j++)
            SDbuffer[j] = *(photoPr+i*SDbufferSize+j);
        f_write(&filephoto, SDbuffer, SDbufferSize, &br);
        
    }
    
    if(remain != 0)
    {   
        
        //W25Q64BVqueryEx(count*SDbufferSize,SDbuffer,remain);
        //memcpy(SDbuffer,*(&photoPr+count*SDbufferSize) , remain);
        
        
        //memcpy(SDbuffer,photoPr+count*SDbufferSize , remain);
        
        for(int k=0;k<remain;k++)
            SDbuffer[k] = *(photoPr+count*SDbufferSize+k);
        
        f_write(&filephoto, SDbuffer, remain, &br);
        
    }
        
    */


    return TRUE;
    
}


//====================================================/=/=======================================================//

static BOOL versionQuery(void* para1, void* para2)
{
	  //uint8_t *VerCode1,*VerCode2,*VerCode3,*YearCode,*MonthCode,*DayCode,*HourCode,*MinuteCode;
	
        uint8_t VerCode1,VerCode2,VerCode3,YearCode,MonthCode,DayCode,HourCode,MinuteCode;
		char* string = malloc(29);
		//char* string2 = malloc(17);
		//terninalPrintf("%d",string);
		//char string[28];

		QueryVersion(&VerCode1,&VerCode2,&VerCode3,&YearCode,&MonthCode,&DayCode,&HourCode,&MinuteCode);
		/*terninalPrintf("VerCode1=%d\r\n",VerCode1);
		terninalPrintf("VerCode2=%d\r\n",VerCode2);
		terninalPrintf("VerCode3=%d\r\n",VerCode3);
		terninalPrintf("YearCode=%d\r\n",YearCode);
		terninalPrintf("MonthCode=%d\r\n",MonthCode);
		terninalPrintf("DayCode=%d\r\n",DayCode);
		terninalPrintf("HourCode=%d\r\n",HourCode);
		terninalPrintf("MinuteCode=%d\r\n",MinuteCode);*/
	
		*string      = 'V';
		*(string+1)  = 'e';
	    *(string+2)  = 'r';
	    *(string+3)  = ' ';
	    *(string+4)  = 48+VerCode1;
		*(string+5)  = '.';
	    *(string+6)  = 48+(VerCode2/10);
		*(string+7)  = 48+(VerCode2%10);
		*(string+8)  = '.';
	    *(string+9)  = 48+(VerCode3/10);
	    *(string+10) = 48+(VerCode3%10);		
		*(string+11) = '\n';
		*(string+12) = 'b';
		*(string+13) = 'u';
		*(string+14) = 'i';
		*(string+15) = 'l';
		*(string+16) = 'd';
		*(string+17) = ' ';
		*(string+18) = 48+(YearCode/10);
		*(string+19) = 48+(YearCode%10);
		*(string+20) = 48+(MonthCode/10);
		*(string+21) = 48+(MonthCode%10);
		*(string+22) = 48+(DayCode/10);
		*(string+23) = 48+(DayCode%10);
		*(string+24) = 48+(HourCode/10);
		*(string+25) = 48+(HourCode%10);
		*(string+26) = 48+(MinuteCode/10);
		*(string+27) = 48+(MinuteCode%10);
		*(string+28) = '\0';		
	
		//EPDDrawString(TRUE,"Version Query",X_POS_MSG,Y_POS_MSG+50);
		//terninalPrintf("%s\r\n",string1);
		terninalPrintf("%s\r\n",string);
		terninalPrintf("Is the version number correct?(y/n)\r\n");
		guiPrintMessage(string);
		//EPDDrawString(TRUE,string1,X_POS_MSG,Y_POS_MSG+50);
		//EPDDrawString(TRUE,string2,X_POS_MSG,Y_POS_MSG+100);
		
		
		//vTaskDelay(2000/portTICK_RATE_MS);
		//EPDDrawContainByIDPos(TRUE,EPD_PICT_KEY_CLEAN_BAR,X_POS_MSG-2,Y_POS_MSG-2); 
		
		free(string);
		//free(string2);
		
		if(userResponseLoop()=='n')
          {
			guiPrintMessage("");
            EPDSetBacklight(FALSE);
            return TEST_FALSE;
          }
		return TEST_SUCCESSFUL_LIGHT_OFF;
}






//////////////////////buzzer test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL buzzerTest(void* para1, void* para2)
{
    char tempchar;
    terninalPrintf("!!! buzzerTest !!!\r\n");
    if(MBtestFlag )
    {}
    else
    {
        guiPrintResult("Buzzer Testing");
        guiPrintMessage("Speaker play \n2 Time");
        //EPDDrawString(TRUE,"Buzzer Will  \nBeep 2 Time",X_POS_MSG,Y_POS_MSG);
    }
    terninalPrintf("Speaker play 2 Time\n");
    //vTaskDelay(1300/portTICK_RATE_MS);
    BuzzerPlay(400,400, 2, TRUE);
    if(MBtestFlag )
    {
        SetMTPCRC(7,(uint8_t*)MTPString);
        MTPCmdprint(7,(uint8_t*)MTPString); 
    }
    else
    {
        cleanMsg();
        guiPrintMessage("\n+:Have Sound\n=:No Sound");
    }
    terninalPrintf("Do you Heard Buzzer playing two time?(y/n)\n");

    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar=='y')
        {
            //terninalPrintf(" Buzzer Test..... [OK]\r\n");
            MTPString[6][3] = 0x81;
            return TEST_SUCCESSFUL_LIGHT_OFF;
        }
        else if(tempchar=='n')
        {
            if(MBtestFlag )
            {
                MTPString[6][3] = 0x80;
            }
            else
            {
                //EPDDrawString(TRUE,"FAIL             ",X_POS_RST,Y_POS_RST);
                //terninalPrintf(" Buzzer Test..... [ERROR]\r\n");
                cleanMsg();
            }
            
            if(MBtestFlag )
                return FALSE;
            else
                return TEST_FALSE;
        }
    }
}

//////////////////////KeyPad test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF

static BOOL keyPadAllTest(void* para1, void* para2)
{
    if(MBtestFlag )
    {
        /////guiManagerShowScreen(GUI_BLANK_ID, GUI_REDRAW_PARA_REFRESH,0, 0);
        //guiManagerShowScreen(GUI_SINGLE_TEST_ID, GUI_REDRAW_PARA_REFRESH,(int)MB_singleTestItem, 0);
        SetMTPCRC(18,(uint8_t*)MTPString);
        MTPCmdprint(18,(uint8_t*)MTPString);
        
        NT066ESetPower(TRUE);
    }
    terninalPrintf("!!! keypadTest !!!\r\n");
    if(MBtestFlag )
    {}
    else
        guiPrintResult("Keypad Testing");
    terninalPrintf("Start Test Keypad...\r\n");
#if(SUPPORT_HK_10_HW)
    BOOL lastKey[] = {GUI_KEYPAD_LEFT_ID,GUI_KEYPAD_RIGHT_ID,GUI_KEYPAD_ADD_ID,GUI_KEYPAD_MINUS_ID,GUI_KEYPAD_CARD_ID,GUI_KEYPAD_QRCODE_ID};
    if(MBtestFlag )
    {
        lastKey[0] = 0x05; //0x00;
        lastKey[1] = 0x04; //0x01;
        lastKey[2] = 0x03; //0x02;
        lastKey[3] = 0x02; //0x03;
        lastKey[4] = 0x01; //0x04;
        lastKey[5] = 0x00; //0x05;
    }
#else
    BOOL lastKey[] = {GUI_KEYPAD_LEFT_ID,GUI_KEYPAD_RIGHT_ID,GUI_KEYPAD_ADD_ID,GUI_KEYPAD_MINUS_ID,GUI_KEYPAD_CONFIRM_ID}; 
#endif
    BOOL lastStage[] = {TOUCH_UP, TOUCH_DOWN};
    BOOL keyStage = 0;
    BOOL tempkeyStage = 0;
    BOOL statusStage = 0;

    callBackKeyId = 0;
    callBackDownUp = 0;
    
    int counter1s = 0;
    int counterms = 0;
    int MTPcounterms = 0;
    
    KeyDrvSetMode(KEY_DRV_MODE_NORMAL_INDEX);
    KeyDrvSetCallbackFunc(CallBackReturnValue);

    if(MBtestFlag )
    {
        targetkeyStage = lastKey[keyStage];
        //terninalPrintf("Click Key 1\n");
        //SetMTPCRC(18,(uint8_t*)MTPString);
        //NT066ESetPower(FALSE);
        //MTPCmdprintEx(18,(uint8_t*)MTPString);
        //NT066ESetPower(TRUE);
        terninalPrintf("Click Key 6\n");
        while(1)
        {
            if(callBackDownUp != 0)
            {
                sysprintf("keyId = 0x%x,downUp = 0x%x\r\n ", callBackKeyId, callBackDownUp);
                if((callBackDownUp == lastStage[statusStage]) && (callBackKeyId == lastKey[keyStage]))
                {
                    if(statusStage == 1)
                    {	
                        keyStage++;
                        
                        statusStage = 0;
                        switch(keyStage){
                        case 1:
                            //terninalPrintf("Click Key 2\n");
                            MTPString[12][8] = 0x81;
                            //SetMTPCRC(17,(uint8_t*)MTPString);
                            //NT066ESetPower(FALSE);
                            //MTPCmdprint(17,(uint8_t*)MTPString);
                            //NT066ESetPower(TRUE);
                            terninalPrintf("Click Key 5\n");
                            counter1s = 0;
                            counterms = 0;
                            break;
                        case 2:
                            MTPString[12][7] = 0x81;
                            //terninalPrintf("Click Key 3\n");
                            //SetMTPCRC(16,(uint8_t*)MTPString);
                            //NT066ESetPower(FALSE);
                            //MTPCmdprint(16,(uint8_t*)MTPString);
                            //NT066ESetPower(TRUE);
                            terninalPrintf("Click Key 4\n");
                            counter1s = 0;
                            counterms = 0;
                            break;
                        case 3:
                            MTPString[12][6] = 0x81;
                            //terninalPrintf("Click Key 4\n");
                            //SetMTPCRC(15,(uint8_t*)MTPString);
                            //NT066ESetPower(FALSE);
                            //MTPCmdprint(15,(uint8_t*)MTPString);
                            //NT066ESetPower(TRUE);
                            terninalPrintf("Click Key 3\n");
                            counter1s = 0;
                            counterms = 0;
                            break;
                    #if(SUPPORT_HK_10_HW)
                        case 4:
                            MTPString[12][5] = 0x81;
                            //terninalPrintf("Click Key 5\n");
                            //SetMTPCRC(14,(uint8_t*)MTPString);
                            //NT066ESetPower(FALSE);
                            //MTPCmdprint(14,(uint8_t*)MTPString);
                            //NT066ESetPower(TRUE);
                            terninalPrintf("Click Key 2\n");
                            counter1s = 0;
                            counterms = 0;
                            break;
                        case 5:
                            MTPString[12][4] = 0x81;
                            //terninalPrintf("Click Key 6\n");
                            //SetMTPCRC(13,(uint8_t*)MTPString);
                            //NT066ESetPower(FALSE);
                            //MTPCmdprint(13,(uint8_t*)MTPString);
                            //NT066ESetPower(TRUE);
                            terninalPrintf("Click Key 1\n");
                            counter1s = 0;
                            counterms = 0;
                            break;
                    #else
                        case 4:
                            guiPrintMessage("Click { Key");
                            terninalPrintf("Click Key 5\n");
                            break;
                    #endif
                        }
                    }
                    else
                    {	
                        //BuzzerPlay(200, 0, 1, FALSE);
                        statusStage++;
                    }
                    
                }
                else
                {
                    testErroCode = TOUCH_SEQUENCE_ERRO;
    //                NT066ESetPower(FALSE);
                    setPrintfFlag(TRUE);
                    //terninalPrintf("KeyPad Test [ERROR]\n");
                    //guiManagerResetKeyCallbackFunc();

                    if(MBtestFlag )
                         KeyDrvSetCallbackFunc(NULL);
                     //   NT066ESetPower(FALSE);
                    if(MBtestFlag )
                        return FALSE;
                    //return TEST_FALSE;
                }
                
            #if(SUPPORT_HK_10_HW)
                if(keyStage == 6)
            #else
                if(keyStage == 5)
            #endif
                {
                    break;
                }
                callBackDownUp = 0;
            }
            targetkeyStage = lastKey[keyStage];
            vTaskDelay(10/portTICK_RATE_MS);
            
            counterms++;
            MTPcounterms++;
            
            if(MTPcounterms >= 40)
            {
                MTPcounterms = 0;
                if(tempkeyStage < keyStage)
                {
                    tempkeyStage++;
                    switch(keyStage)
                        {
                            case 1:
                                SetMTPCRC(17,(uint8_t*)MTPString);
                                MTPCmdprintEx(17,(uint8_t*)MTPString);
                                break;
                            case 2:
                                SetMTPCRC(16,(uint8_t*)MTPString);
                                MTPCmdprintEx(16,(uint8_t*)MTPString);
                                break;
                            case 3:
                                SetMTPCRC(15,(uint8_t*)MTPString);
                                MTPCmdprintEx(15,(uint8_t*)MTPString);
                                break;
                            case 4:
                                SetMTPCRC(14,(uint8_t*)MTPString);
                                MTPCmdprintEx(14,(uint8_t*)MTPString);
                                break;
                            case 5:
                                SetMTPCRC(13,(uint8_t*)MTPString);
                                MTPCmdprintEx(13,(uint8_t*)MTPString);
                                break;    
                        }
                }

            }
            
            if(counterms >= 100)
            {
                counterms = 0;
                terninalPrintf("%d \r",15-counter1s);
                counter1s++;
                if(counter1s >= 15)
                {
                    testErroCode = TOUCH_SEQUENCE_ERRO;
                    setPrintfFlag(TRUE);
                    MTPString[12][8 - keyStage] = 0x80;
                    MTPString[12][9] = 0x80;
                    if(MBtestFlag )
                         KeyDrvSetCallbackFunc(NULL);
                    if(MBtestFlag )
                        return FALSE;
                }
            }
            
            
        }
        //terninalPrintf("KeyPad Test..... [OK]\n");
        //guiManagerResetKeyCallbackFunc();
        
        
        if(MBtestFlag )
        {
             KeyDrvSetCallbackFunc(NULL);
            //NT066ESetPower(FALSE);
        }
        MTPString[12][3] = 0x81;
        MTPString[12][9] = 0x81;
        return TEST_SUCCESSFUL_LIGHT_OFF;
 
    }
    else
    {
        targetkeyStage = lastKey[keyStage];
        // KeyDrvInit();
        //TODO FIND BUG cannot run 2nd time
        guiPrintMessage("Click < Key");
        terninalPrintf("Click Key 1\n");
        while(1)
        {
            if(callBackDownUp != 0)
            {
                sysprintf("keyId = 0x%x,downUp = 0x%x\r\n ", callBackKeyId, callBackDownUp);
                if((callBackDownUp == lastStage[statusStage]) && (callBackKeyId == lastKey[keyStage]))
                {
                    if(statusStage == 1)
                    {	
                        keyStage++;
                        statusStage = 0;
                        
                        counter1s = 0;
                        counterms = 0;
                        switch(keyStage){
                        case 1:
                            guiPrintMessage("Click > Key");
                            terninalPrintf("Click Key 2\n");
                            break;
                        case 2:
                            guiPrintMessage("Click + Key");
                            terninalPrintf("Click Key 3\n");
                            break;
                        case 3:
                            guiPrintMessage("Click = Key");
                            terninalPrintf("Click Key 4\n");
                            break;
                    #if(SUPPORT_HK_10_HW)
                        case 4:
                            guiPrintMessage("Click { Key");
                            terninalPrintf("Click Key 5\n");
                            break;
                        case 5:
                            guiPrintMessage("Click } Key");
                            terninalPrintf("Click Key 6\n");
                            break;
                    #else
                        case 4:
                            guiPrintMessage("Click { Key");
                            terninalPrintf("Click Key 5\n");
                            break;
                    #endif
                        }
                    }
                    else
                    {	
                        //BuzzerPlay(200, 0, 1, FALSE);
                        statusStage++;
                    }
                    
                }
                else
                {
                    testErroCode = TOUCH_SEQUENCE_ERRO;
    //                NT066ESetPower(FALSE);
                    setPrintfFlag(TRUE);
                    //terninalPrintf("KeyPad Test [ERROR]\n");
                    guiManagerResetKeyCallbackFunc();

                    if(MBtestFlag )
                        NT066ESetPower(FALSE);
                    return TEST_FALSE;
                }
                
            #if(SUPPORT_HK_10_HW)
                if(keyStage == 6)
            #else
                if(keyStage == 5)
            #endif
                {
                    break;
                }
                callBackDownUp = 0;
            }
            targetkeyStage = lastKey[keyStage];
            vTaskDelay(100/portTICK_RATE_MS);
            
            counterms++;
            
            if(counterms >= 10)
            {
                counterms = 0;
                terninalPrintf("%d \r",15-counter1s);
                counter1s++;
                if(counter1s >= 15)
                {
                    testErroCode = TOUCH_SEQUENCE_ERRO;
                    setPrintfFlag(TRUE);
                    guiManagerResetKeyCallbackFunc();
                    return TEST_FALSE;
                }
            }
            
            
        }
        //terninalPrintf("KeyPad Test..... [OK]\n");
        guiManagerResetKeyCallbackFunc();
        
        
        if(MBtestFlag )
        {
            NT066ESetPower(FALSE);
        }
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
}

//////////////////////LED    test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL LEDTest(void* para1, void* para2)
{
    char tempchar;
    BOOL LEDresultFlag1 = FALSE;        //  LED flash result
    BOOL LEDresultFlag2 = FALSE;        //  LED reset result
    BOOL LEDresultFlag3 = FALSE;        //  LED version result
    terninalPrintf("!!! ledTest !!!\r\n");
    if(MBtestFlag )
    {
        //SetMTPCRC(40,(uint8_t*)MTPString);
        //MTPCmdprint(40,(uint8_t*)MTPString);
    }
    else
        guiPrintResult("LED Testing");
    EPDSetBacklight(FALSE);
    vTaskDelay(1500/portTICK_RATE_MS);
    if(LedDrvInit(TRUE) == TRUE)
    {
        //MemsCalibrationSet();
        //1.White LED Test
        //terninalPrintf("5 White LED will lightup\n");
        //guiPrintMessage("Background Light\nwill ON");
        //vTaskDelay(2000/portTICK_RATE_MS);
        //EPDSetBacklight(TRUE);
        
        //guiPrintMessage("Background Light\nwill ON\n\n+:Yes =:No");
        //terninalPrintf("Does 5 Background White LED lightup?(y/n)\n");
       /* if(userResponseLoop()=='n')
        {
            EPDSetBacklight(FALSE);
            //terninalPrintf(" LED Test [ERROR]\r\n");
            return TEST_FALSE;
        }
        EPDSetBacklight(FALSE); */
        if(MBtestFlag )
        {}
        else
            guiPrintResult("LED Testing");
        //2.Red Green LED test
        //terninalPrintf("Front 7 LED and Back 3 LED will lightup one by one.\r\n");
        //guiPrintMessage("Front 7 LED\nBack  3 LED\nTurn ON\nOne By One");
        terninalPrintf("Front 8 LED and Back 3 LED will lightup one by one.\r\n");
        if(MBtestFlag )
        {}
        else
            guiPrintMessage("Front 8 LED\nBack  3 LED\nTurn ON\nOne By One");
        vTaskDelay(1500/portTICK_RATE_MS);
        EPDSetBacklight(TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_SetBit(GPIOG, BIT7);
        EPDSetBacklight(FALSE);
        LedSendFactoryTest();
        //vTaskDelay(1500/portTICK_RATE_MS);

        vTaskDelay(5000/portTICK_RATE_MS);
        //guiPrintMessage("Front 7 LED\nBack  3 LED\nTurn ON\nOne By One\n\n+:Yes =:No");
        if(MBtestFlag )
        {
            SetMTPCRC(9,(uint8_t*)MTPString);
            MTPCmdprint(9,(uint8_t*)MTPString);
        }
        else
            guiPrintMessage("Front 8 LED\nBack  3 LED\nTurn ON\nOne By One\n\n+:Yes =:No");
        terninalPrintf("Does all LED lightup?(y/n)\n");
        GPIO_ClrBit(GPIOG, BIT7);
        while(1)
        {
            tempchar = userResponseLoop();
            if(tempchar =='y')
            {
                //cleanMsg();
                //EPDDrawString(TRUE,"PASS              ",X_POS_RST,Y_POS_RST);
                //terninalPrintf(" LED Test [OK]\r\n");
                //////////////////////MEMS   test/////////////////////////
                //return MemsTest(para1,para2);
                EPDSetBacklight(FALSE);
                LEDresultFlag1 = TRUE;
                MTPString[8][3] = 0x81;
                break;
                //return TEST_SUCCESSFUL_LIGHT_OFF;
            }
            else if(tempchar =='n')
            {
                EPDSetBacklight(FALSE);
                LEDresultFlag1 = FALSE;
                MTPString[8][3] = 0x80;
                if(MBtestFlag )
                    return FALSE;
                break;
                //return TEST_FALSE;
            }
        }
         
    }
    else
    {    //LED hardware initial fail
        if(MBtestFlag )
        {}
        else
            guiPrintMessage("INIT ERROR");
        terninalPrintf("LED Initial [ERROR]\r\n");
        testErroCode = LED_CONNECT_ERRO;
        LEDresultFlag1 = FALSE;
        EPDSetBacklight(FALSE);
        if(MBtestFlag )
            return FALSE;
    }
    
    
    //terninalPrintf(" LED Test [ERROR]\r\n");  
    //EPDSetBacklight(FALSE);
    //return TEST_FALSE;
    
    //--------------LED reset----------------------------------
    uint8_t VerCode1,VerCode2,VerCode3,YearCode,MonthCode,DayCode,HourCode,MinuteCode;
    char LEDVerStr[50];
    //char* string = malloc(29);
    GPIO_ClrBit(GPIOF, BIT9);
    terninalPrintf("Test LED reset pin.\r\n");
    if(MBtestFlag )
    {
        SetMTPCRC(55,(uint8_t*)MTPString);
        MTPCmdprint(55,(uint8_t*)MTPString);
    }
    else
    {
        guiPrintResult("Test LED reset");
        vTaskDelay(15/portTICK_RATE_MS);
        guiPrintMessage("");
    }
    vTaskDelay(100/portTICK_RATE_MS);
    if(QueryVersion(&VerCode1,&VerCode2,&VerCode3,&YearCode,&MonthCode,&DayCode,&HourCode,&MinuteCode))
    {
 
        // sprintf(LEDVerStr,"Ver %d.%d.%d Build %02d%02d%02d%02d%02d",VerCode1,VerCode2,VerCode3,
         //        YearCode,MonthCode,DayCode,HourCode,MinuteCode);
        //terninalPrintf("LED Ver: %s\r\n",LEDVerStr); 
        terninalPrintf("LED reset error.\r\n");
        LEDresultFlag2 = FALSE;
        MTPString[8][4] = 0x80;
        GPIO_SetBit(GPIOF, BIT9);
        if(MBtestFlag )
            return FALSE;
        else
            guiPrintMessage("reset error.");
    }
    else
    {
        //terninalPrintf("LED Ver: Error\r\n");
        terninalPrintf("LED reset success.\r\n");
        if(MBtestFlag )
        {}
        else
            guiPrintMessage("reset success.");
        LEDresultFlag2 = TRUE;
        MTPString[8][4] = 0x81;
        GPIO_SetBit(GPIOF, BIT9);
        

    }
    
    //--------------LED reset----------------------------------
    
    
    
    //--------------LED version----------------------------------
    
    
    terninalPrintf("Test LED version.\r\n");
    if(MBtestFlag )
    {
        SetMTPCRC(56,(uint8_t*)MTPString);
        MTPCmdprint(56,(uint8_t*)MTPString);
    }
    else
    {
        guiPrintResult("Test LED version.");
        vTaskDelay(15/portTICK_RATE_MS);
        guiPrintMessage("");
    }
    vTaskDelay(1000/portTICK_RATE_MS);
    LedDrvInit(SPECIAL);
    vTaskDelay(3000/portTICK_RATE_MS);
    if(QueryVersion(&VerCode1,&VerCode2,&VerCode3,&YearCode,&MonthCode,&DayCode,&HourCode,&MinuteCode))
    {
 
        sprintf(LEDVerStr,"Ver %d.%d.%d Build %02d%02d%02d%02d%02d",VerCode1,VerCode2,VerCode3,
                 YearCode,MonthCode,DayCode,HourCode,MinuteCode);
        terninalPrintf("LED Ver: %s\r\n",LEDVerStr); 
        if(MBtestFlag )
        {}
        else
        {
            char ShowLEDVerStr[50];
            sprintf(ShowLEDVerStr,"LED Ver: \nVer %d.%d.%d\nBuild %02d%02d%02d%02d%02d",
                    VerCode1,VerCode2,VerCode3,YearCode,MonthCode,DayCode,HourCode,MinuteCode);
            guiPrintMessage(ShowLEDVerStr);
        }

        LEDresultFlag3 = TRUE;
        MTPString[8][5] = 0x81;
    }
    else
    {
        terninalPrintf("LED Ver: Error\r\n");
        //terninalPrintf("LED version success.\r\n");
        MTPString[8][5] = 0x80;        
        LEDresultFlag3 = FALSE;
        
        if(MBtestFlag )
            return FALSE;
        else
            guiPrintMessage("LED Ver: Error");

    }
    
    //--------------LED version----------------------------------
    
    
    if(LEDresultFlag1 && LEDresultFlag2 && LEDresultFlag3)
    {
        MTPString[8][6] = 0x81;
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
    else
    {
        MTPString[8][6] = 0x80;
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
}

static BOOL LEDTestLite(void* para1, void* para2)
{
    char tempchar;
    BOOL LEDresultFlag1 = FALSE;        //  LED flash result
    BOOL LEDresultFlag2 = FALSE;        //  LED reset result
    BOOL LEDresultFlag3 = FALSE;        //  LED version result
    terninalPrintf("!!! ledTest !!!\r\n");

    guiPrintResult("LED Testing");
    
    
    //--------------LED reset----------------------------------
    uint8_t VerCode1,VerCode2,VerCode3,YearCode,MonthCode,DayCode,HourCode,MinuteCode;
    char LEDVerStr[50];
    //char* string = malloc(29);
    GPIO_ClrBit(GPIOF, BIT9);
    terninalPrintf("Test LED reset pin.\r\n");
    if(MBtestFlag )
    {
        SetMTPCRC(55,(uint8_t*)MTPString);
        MTPCmdprint(55,(uint8_t*)MTPString);
    }
    else
    {
        guiPrintResult("Test LED reset");
        vTaskDelay(15/portTICK_RATE_MS);
        guiPrintMessage("");
    }
    vTaskDelay(100/portTICK_RATE_MS);
    if(QueryVersion(&VerCode1,&VerCode2,&VerCode3,&YearCode,&MonthCode,&DayCode,&HourCode,&MinuteCode))
    {
 

        terninalPrintf("LED reset error.\r\n");
        LEDresultFlag2 = FALSE;
        MTPString[8][4] = 0x80;
        GPIO_SetBit(GPIOF, BIT9);
        if(MBtestFlag )
            return FALSE;
        else
            guiPrintMessage("reset error.");
    }
    else
    {
        //terninalPrintf("LED Ver: Error\r\n");
        terninalPrintf("LED reset success.\r\n");
        if(MBtestFlag )
        {}
        else
            guiPrintMessage("reset success.");
        LEDresultFlag2 = TRUE;
        MTPString[8][4] = 0x81;
        GPIO_SetBit(GPIOF, BIT9);
        

    }
    
    //--------------LED reset----------------------------------
    
    
    
    
    EPDSetBacklight(FALSE);
    //vTaskDelay(1500/portTICK_RATE_MS);
    vTaskDelay(1000/portTICK_RATE_MS);
    LedDrvInit(SPECIAL);
    vTaskDelay(500/portTICK_RATE_MS);
    
    if(LedDrvInit(TRUE) == TRUE)
    {


        guiPrintResult("LED Testing");
        terninalPrintf("Front 8 LED and Back 3 LED will lightup one by one.\r\n");
        guiPrintMessage("Front 8 LED\nBack  3 LED\nTurn ON\nOne By One");
        vTaskDelay(1500/portTICK_RATE_MS);
        EPDSetBacklight(TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_SetBit(GPIOG, BIT7);
        EPDSetBacklight(FALSE);
        LedSendFactoryTest();

        vTaskDelay(5000/portTICK_RATE_MS);
        guiPrintMessage("Front 8 LED\nBack  3 LED\nTurn ON\nOne By One\n\n+:Yes =:No");
        terninalPrintf("Does all LED lightup?(y/n)\n");
        GPIO_ClrBit(GPIOG, BIT7);
        while(1)
        {
            tempchar = userResponseLoop();
            if(tempchar =='y')
            {
                EPDSetBacklight(FALSE);
                LEDresultFlag1 = TRUE;
                MTPString[8][3] = 0x81;
                break;
            }
            else if(tempchar =='n')
            {
                EPDSetBacklight(FALSE);
                LEDresultFlag1 = FALSE;
                MTPString[8][3] = 0x80;
                if(MBtestFlag )
                    return FALSE;
                break;
            }
        }
         
    }
    else
    {   
        guiPrintMessage("INIT ERROR");
        terninalPrintf("LED Initial [ERROR]\r\n");
        testErroCode = LED_CONNECT_ERRO;
        LEDresultFlag1 = FALSE;
        EPDSetBacklight(FALSE);

    }
    
    
    
    if(LEDresultFlag1 && LEDresultFlag2)
        return TEST_SUCCESSFUL_LIGHT_OFF;
    else
        return TEST_FALSE;
    
}

//////////////////////MEMS   test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL MemsTest(void* para1, void* para2)
{
    //guiPrintMessage("Upright EPM");
    //terninalPrintf("\n1.stand upright EPM\n");
    uint8_t ret;
    int timeout = 10;
    //x y z axis test
    


  

    /*
    for(int i=0;i<timeout;i++)
    {
        if(i == (timeout-1))
        {
            return TEST_FALSE;
        }
        if(LedSendHeartbeat(&ret) == TRUE)
        {
            char str[40];
            terninalPrintf("result [%08b (%d)] collision(%b) x(%b) y(%b) z(%b)  %d  \r\n",ret,ret,ret&0x1,(ret>>3)&0x1,(ret>>2)&0x1,(ret>>1)&0x1,timeout - i);
            sprintf(str,"\nx %d y %d z %d",(ret>>3)&0x1,(ret>>2)&0x1,(ret>>1)&0x1);
            guiPrintMessageNoClean(str);
            if(((ret>>1)&0x3)==0)
                break;
            else
                vTaskDelay(3000/portTICK_RATE_MS);
        }
    }
    guiPrintMessage("LayDown EPM");
    terninalPrintf("\n2.lay down EPM\n");
    timeout = 10;
    //x y z axis test
    for(int i=0;i<timeout;i++)
    {
        if(i == (timeout-1))
        {
            return TEST_FALSE;
        }
        if(LedSendHeartbeat(&ret) == TRUE)
        {
            char str[40];
            terninalPrintf("result [%08b (%d)] collision(%b) x(%b) y(%b) z(%b)  %d  \r\n",ret,ret,ret&0x1,(ret>>3)&0x1,(ret>>2)&0x1,(ret>>1)&0x1,timeout - i);
            sprintf(str,"\nx %d y %d z %d",(ret>>3)&0x1,(ret>>2)&0x1,(ret>>1)&0x1);
            guiPrintMessageNoClean(str);
            if(!((ret>>3)&0x1)&&((ret>>2)&0x1)&&((ret>>1)&0x1))
                break;
            else
                vTaskDelay(3000/portTICK_RATE_MS);
        }
    }
    timeout = 10;
    */
    //collision test

    int count=0;
    /*
    do{
        MemsCollisionClean();
        //vTaskDelay(500/portTICK_RATE_MS);
        //LedSendHeartbeat(&ret);
        //vTaskDelay(1500/portTICK_RATE_MS);
        //LedSendHeartbeat(&ret);
        count++;
        terninalPrintf("MemsCollisionClean count=%d ret=%08b\r\n",count,ret);
        //vTaskDelay(3000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
    }while(ret&0x1);    
    */
    
    
    MemsCollisionClean();
    vTaskDelay(1500/portTICK_RATE_MS);
    /*
    GPIO_ClrBit(GPIOF, BIT9);
    vTaskDelay(100/portTICK_RATE_MS);
    GPIO_SetBit(GPIOF, BIT9);
    vTaskDelay(1000/portTICK_RATE_MS);
    LedDrvInit(SPECIAL);
    vTaskDelay(500/portTICK_RATE_MS);
    */
    
    
    guiPrintMessage("Collision EPM");
    terninalPrintf("\nCollision EPM\n");
    //terninalPrintf("\n3.Collision EPM\n");
 

    /*
    //LedSetPower(0);
    //GPIO_SetBit(GPIOF, BIT8);
    GPIO_ClrBit(GPIOF, BIT9);
    vTaskDelay(500/portTICK_RATE_MS);
    //GPIO_ClrBit(GPIOF, BIT8);
    GPIO_SetBit(GPIOF, BIT9);
    //LedSetPower(1);  */
    for(int i=0;i<timeout;i++)
    {
        if(i == (timeout-1))
        {
            return TEST_FALSE;
        }
        //if(LedSendHeartbeat(&ret) == TRUE)
        if(LedReadShake(&ret) == TRUE)
        {
            char str[40];
            //terninalPrintf("ret = %02x\r\n",ret);
            terninalPrintf("result [%08b (%d)] collision(%b) x(%b) y(%b) z(%b)  %d  \r\n",ret,ret,ret&0x1,(ret>>3)&0x1,(ret>>2)&0x1,(ret>>1)&0x1,timeout - i);
            sprintf(str,"\nCollision %d",ret&0x1);
            guiPrintMessageNoClean(str);
            if((ret&0x1)>0)
                return TEST_SUCCESSFUL_LIGHT_OFF;
            else
                vTaskDelay(1000/portTICK_RATE_MS);
        }
    }
    return TEST_FALSE;
}

/////////////////////Red Switch test//////////////////////////////TEST_SUCCESSFUL_LIGHT_ON;
static BOOL redSwitchAllTest(void* para1, void* para2)
{
    terninalPrintf("!!! switch Test !!!\r\n");
    if(MBtestFlag )
    {}
    else
        //guiPrintResult("Blue Switch \nTesting");
        guiPrintResult("SW4 Switch \nTesting");
        //guiPrintResult("Red Switch \nTesting");
    UINT8 lastStatus[] = {IOPIN_BOTH_OFF, IOPIN_ONLY1_ON, IOPIN_BOTH_OFF, IOPIN_ONLY2_ON, IOPIN_BOTH_OFF}; 
    UINT8 statusStage = 0;
    UINT32 portValue = GPIO_ReadPort(GPIOH);
    UINT8 pinStage = (portValue>>2)&0x3;
    if(!DipDrvInit(TRUE))
    {
        terninalPrintf(" DIP Driver Init [ERROR]\r\n");
        setPrintfFlag(TRUE);
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    //terninalPrintf("Please Switch SW1 and SW2  to off position\r\n");
    terninalPrintf("ok\nPlease Switch SW1 and SW2  to on position\r\n");
    if(MBtestFlag )
    {        
        SetMTPCRC(20,(uint8_t*)MTPString);
        MTPCmdprint(20,(uint8_t*)MTPString);
    }
    else
        guiPrintMessage("Switch 1\\2\nON");
    int timeOut = 15; //10;
    //int mscounter = 0;
    while((timeOut--)>0)
    {
        terninalPrintf("\r%d ",timeOut);
        /*
        vTaskDelay(1000/portTICK_RATE_MS);
        portValue = GPIO_ReadPort(GPIOH);
        pinStage = (portValue>>2)&0x3;
        */
        portValue = GPIO_ReadPort(GPIOH);
        pinStage = (portValue>>2)&0x3;
        for(int i=0;i<10;i++)
        {
            vTaskDelay(100/portTICK_RATE_MS);

            if(pinStage != ((GPIO_ReadPort(GPIOH)>>2)&0x3))                            
                BuzzerPlay(200, 500, 1, TRUE);
            portValue = GPIO_ReadPort(GPIOH);
            pinStage = (portValue>>2)&0x3;
        }
        //if(pinStage == IOPIN_BOTH_OFF)
        if(pinStage == IOPIN_BOTH_ON)
        {
            MTPString[19][3] = 0x81;
            //terninalPrintf("ok\nPlease Switch SW1 and SW2  to on position\r\n");
            terninalPrintf("Please Switch SW1 and SW2  to off position\r\n");
            if(MBtestFlag )
            {
                SetMTPCRC(21,(uint8_t*)MTPString);
                MTPCmdprint(21,(uint8_t*)MTPString);
            }
            else
                guiPrintMessage("Switch 1\\2\nOFF");
            timeOut = 15; //10;
            while((timeOut)-->0)
            {
                terninalPrintf("\r%d",timeOut);
                /*
                vTaskDelay(1000/portTICK_RATE_MS);
                portValue = GPIO_ReadPort(GPIOH);
                pinStage = (portValue>>2)&0x3;
                */
                portValue = GPIO_ReadPort(GPIOH);
                pinStage = (portValue>>2)&0x3;
                for(int i=0;i<10;i++)
                {
                    vTaskDelay(100/portTICK_RATE_MS);

                    if(pinStage != ((GPIO_ReadPort(GPIOH)>>2)&0x3))                            
                        BuzzerPlay(200, 500, 1, TRUE);
                    portValue = GPIO_ReadPort(GPIOH);
                    pinStage = (portValue>>2)&0x3;
                }
                
                //if(pinStage == IOPIN_BOTH_ON)
                if(pinStage == IOPIN_BOTH_OFF)
                {
                    //terninalPrintf("\nRedSwitch Test [OK]\r\n");
                    MTPString[19][4] = 0x81;
                    return electricitySingleTest(para1,para2);
                }
            }
            
            if(MBtestFlag )
            {
                MTPString[19][4] = 0x80;
            }
            else
                //guiPrintMessage("BlueSwitch FAIL");
            guiPrintMessage("SW4 test FAIL");
                //guiPrintMessage("RedSwitch FAIL");
            
            if(MBtestFlag )
                return FALSE;
            else
                return TEST_FALSE;
            
            
        }
    }
    //terninalPrintf("RedSwitch Test [ERROR]\r\n");
    if(MBtestFlag )
    {
        MTPString[19][3] = 0x80;
    }
    else
        //guiPrintMessage("BlueSwitch FAIL");
        guiPrintMessage("SW4 test FAIL");
        //guiPrintMessage("RedSwitch FAIL");
    
    if(MBtestFlag )
        return FALSE;
    else
        return TEST_FALSE;
    
}
static BOOL redSwitchAllTest_old(void* para1, void* para2)
{
    terninalPrintf("!!! switch Test !!!\r\n");
    guiPrintResult("Red Switch \nTesting");
    UINT8 lastStatus[] = {IOPIN_BOTH_OFF, IOPIN_ONLY1_ON, IOPIN_BOTH_OFF, IOPIN_ONLY2_ON, IOPIN_BOTH_OFF}; 
    UINT8 statusStage = 0;
    UINT32 portValue = GPIO_ReadPort(GPIOH);
    UINT8 pinStage = (portValue>>2)&0x3;
    if(!DipDrvInit(TRUE))
    {
        terninalPrintf(" DIP Driver Init [ERROR]\r\n");
        setPrintfFlag(TRUE);
        return TEST_FALSE;
    }
    terninalPrintf("Please Switch SW1 and SW2  to off position\r\n");
    guiPrintMessage("Switch SW1\\SW2\nOff");
    while(1)
    {
        portValue = GPIO_ReadPort(GPIOH);
        pinStage = (portValue>>2)&0x3;
        if(pinStage == IOPIN_BOTH_OFF)
        {
            break;
        }
    }
    portValue = GPIO_ReadPort(GPIOH);
    pinStage = (portValue>>2)&0x3;
    //step1 both off
    if((pinStage) != lastStatus[statusStage])
    {
        return TEST_FALSE;
    }
    else
    {
        statusStage++;
        guiPrintMessage("Switch SW1 ON");
        terninalPrintf("Switch 1 on\r\n");
        while(1)
        {
            portValue = GPIO_ReadPort(GPIOH);
            pinStage = (portValue>>2)&0x3;
            if((pinStage != lastStatus[statusStage-1]) && (pinStage != lastStatus[statusStage]))
            {
                if(pinStage == IOPIN_BOTH_ON)
                {
                    testErroCode = RED_SWITCH_BOTH_ON_ERRO;
                }
                else
                {
                    testErroCode = RED_SWITCH_SEQUENCE_ERRO;
                }
                guiPrintResult("Operating ERRO");
                //terninalPrintf("RedSwitch Test [ERROR]\r\n");
                return TEST_FALSE;
            }
            else
            {
                if(pinStage == lastStatus[statusStage])
                {
                    switch(pinStage)
                    {
                    case IOPIN_BOTH_ON:
                        break;
                    case IOPIN_ONLY1_ON:
                        guiPrintMessage("Switch SW1 OFF");
                        terninalPrintf("Switch 1 off\r\n");
                        break;
                    case IOPIN_ONLY2_ON:
                        guiPrintMessage("Switch SW2 OFF");
                        terninalPrintf("Switch 2 off !!!\r\n");
                        break;
                    case IOPIN_BOTH_OFF:
                        if(statusStage==2)
                        {
                            guiPrintMessage("Switch SW2 ON");
                            terninalPrintf("Switch 2 on \r\n");
                        }
                        break;
                    default:
                        terninalPrintf("Switch = 0x%02x !!!\r\n", (portValue>>2)&0x3);
                        break;
                    }
                    statusStage++;
                }
            }
            if(statusStage == 5)
            {
                //terninalPrintf("RedSwitch Test [OK]\r\n");
                return electricitySingleTest(para1,para2);
            }
        }
    }
    //terninalPrintf("RedSwitch Test [ERROR]\r\n");
    return FALSE;
}

//////////////////////Power Button Test///////////////////TEST_SUCCESSFUL_LIGHT_ON
static BOOL electricitySingleTest(void* para1, void* para2)
{
    //terninalPrintf("!!! Button Switch Test !!!\r\n");
    if(MBtestFlag )
    {}
    else
        guiPrintResult("Button Switch");
    if(DipDrvInit(TRUE))
    {
        terninalPrintf("Press Power Button\r\n");
        if(MBtestFlag )
        {
            SetMTPCRC(22,(uint8_t*)MTPString);
            MTPCmdprint(22,(uint8_t*)MTPString);
        }
        else
        {
            cleanMsg();
            guiPrintMessage("Press Button");
        }
        int timeOut = 15; //10;
        while((timeOut--)>0)
        {
            //vTaskDelay(1000/portTICK_RATE_MS);
            terninalPrintf("\r%d",timeOut);
        #if(SUPPORT_HK_10_HW)
            UINT32 powerBitValue = GPIO_ReadBit(GPIOI, 0x08);
        #else
            UINT32 powerBitValue = GPIO_ReadBit(GPIOH, 0x10);
        #endif
            
            for(int i=0;i<10;i++)
            {
                vTaskDelay(100/portTICK_RATE_MS);

                if(powerBitValue != GPIO_ReadBit(GPIOI, 0x08) )                            
                    BuzzerPlay(200, 500, 1, TRUE);
                powerBitValue = GPIO_ReadBit(GPIOI, 0x08);
            }
            
            if(!powerBitValue)
            {
                MTPString[19][5] = 0x81;
                terninalPrintf("\nPower on !!!\r\n");
                if(MBtestFlag )
                {}
                else
                {
                    LEDColorBuffSet(0x3F, 0x00);
                    LEDBoardLightSet();
                }
                terninalPrintf("Release Power Button\r\n");
                if(MBtestFlag )
                {
                    SetMTPCRC(23,(uint8_t*)MTPString);
                    MTPCmdprint(23,(uint8_t*)MTPString);
                }
                else
                    guiPrintMessage("Release Button");
                timeOut = 15; //10;
                while((timeOut--)>0)
                {
                    terninalPrintf("\r%d",timeOut);
                    //vTaskDelay(1000/portTICK_RATE_MS);
                #if(SUPPORT_HK_10_HW)
                    powerBitValue = GPIO_ReadBit(GPIOI, 0x08);
                #else
                    powerBitValue = GPIO_ReadBit(GPIOH, 0x10);
                #endif
                    
                for(int i=0;i<10;i++)
                {
                    vTaskDelay(100/portTICK_RATE_MS);

                    if(powerBitValue != GPIO_ReadBit(GPIOI, 0x08) )                            
                        BuzzerPlay(200, 500, 1, TRUE);
                    powerBitValue = GPIO_ReadBit(GPIOI, 0x08);
                }
                    
                    
                    if(powerBitValue)
                    {
                        MTPString[19][6] = 0x81;
                        terninalPrintf("\nPower off !!!\r\n");
                        if(MBtestFlag )
                        {}
                        else
                        {
                            LEDColorBuffSet(0x00, 0x00);
                            LEDBoardLightSet();
                        }
                        //terninalPrintf("Power Button Test [OK]\r\n");
                        if(MBtestFlag )
                        {
                            return SW6SingleTest(para1, para2);
                        }
                        else
                        {
                            return TEST_SUCCESSFUL_LIGHT_ON;
                        }
                    }                   
                }
                if(MBtestFlag )
                {
                    MTPString[19][6] = 0x80;
                }
                else
                    guiPrintMessage("Power Button\nFAIL");
                if(MBtestFlag )
                    return FALSE;
                else
                    return TEST_FALSE;
                
                break;
            }
        }
        vTaskDelay(300/portTICK_RATE_MS);
    }
    else
    {
        //terninalPrintf(" DIP Driver Init [ERROR]\r\n");
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    //terninalPrintf("Power Button Test [ERROR]\r\n");
    if(MBtestFlag )
    {
        MTPString[19][5] = 0x80;
    }
    else
        guiPrintMessage("Power Button\nFAIL");
    if(MBtestFlag )
        return FALSE;
    else
        return TEST_FALSE;
}

static BOOL SW6SingleTest(void* para1, void* para2)
{
    uint8_t timeOut1 = 0;
    uint8_t timeOut2 = 0;
    uint8_t maxtime = 15;
    uint32_t tempPort;
    //UINT32 portValue = (GPIO_ReadPort(GPIOJ) & 0x1F);
    SetMTPCRC(24,(uint8_t*)MTPString);
    MTPCmdprint(24,(uint8_t*)MTPString);
    terninalPrintf("Please Switch SW6 No.1~5 to on position\r\n");
    while(1)
    {
        if((GPIO_ReadPort(GPIOJ) & 0x1F) == 0)
            break;
        terninalPrintf("\r%d ",maxtime-timeOut1);
        timeOut1++;
        if(timeOut1 >= maxtime)
        {
            if(MBtestFlag )
            {            
                MTPString[19][7] = 0x80;
                return FALSE;
            }
            else
                return TEST_FALSE;
        }
        //vTaskDelay(1000/portTICK_RATE_MS);
        tempPort = GPIO_ReadPort(GPIOJ) & 0x1F ;
        for(int i=0;i<10;i++)
        {
            vTaskDelay(100/portTICK_RATE_MS);

            if(tempPort != (GPIO_ReadPort(GPIOJ) & 0x1F) )                            
                BuzzerPlay(200, 500, 1, TRUE);
            tempPort = GPIO_ReadPort(GPIOJ) & 0x1F ;
        }
        
    }
    MTPString[19][7] = 0x81;
    SetMTPCRC(25,(uint8_t*)MTPString);
    MTPCmdprint(25,(uint8_t*)MTPString);
    terninalPrintf("Please Switch SW6 No.1~5 to off position\r\n");
    while(1)
    {
        if((GPIO_ReadPort(GPIOJ) & 0x1F) == 0x1F)
            break;
        terninalPrintf("\r%d ",maxtime-timeOut2);
        timeOut2++;
        if(timeOut2 >= maxtime)
        {
            if(MBtestFlag )
            {
                MTPString[19][8] = 0x80;
                return FALSE;
            }
            else
                return TEST_FALSE;
        }
        //vTaskDelay(1000/portTICK_RATE_MS);
        
        tempPort = GPIO_ReadPort(GPIOJ) & 0x1F ;
        for(int i=0;i<10;i++)
        {
            vTaskDelay(100/portTICK_RATE_MS);

            if(tempPort != (GPIO_ReadPort(GPIOJ) & 0x1F) )                            
                BuzzerPlay(200, 500, 1, TRUE);
            tempPort = GPIO_ReadPort(GPIOJ) & 0x1F ;
        }
        
    }
    MTPString[19][8] = 0x81;
    MTPString[19][9] = 0x81;
    return TEST_SUCCESSFUL_LIGHT_ON;
}


static BOOL EPDSingleTest(void* para1, void* para2)
{    
    char  preadFWVersion[16];
    char  CmpStr[16];
    BOOL EPDtestFlag = TRUE;
    memset(preadFWVersion,0x00,sizeof(preadFWVersion));
    memset(CmpStr,0x00,sizeof(CmpStr));
    
    terninalPrintf("Please wait about 10 seconds.\r\n");

    ReadIT8951SystemInfo( preadFWVersion, NULL);
    
    //EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE); 
    //EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
    /*
    EPDtestFlag = TRUE;
    
    terninalPrintf("Please wait about 10 seconds.\r\n");
    if(readEPDswitch() == FALSE)
    {
        vTaskDelay(500/portTICK_RATE_MS);
        toggleEPDswitch(TRUE);
        //if(!ReadIT8951SystemInfo( preadFWVersion, NULL))
        //    EPDtestFlag = FALSE;
        //vTaskDelay(1000/portTICK_RATE_MS);
        //EPDDrawString(TRUE,"ENABLE  ",90+325,100+(44*(8)));
        //terninalPrintf("ENABLE EPD.\r\n");

        vTaskDelay(1000/portTICK_RATE_MS);
    }
    GPIO_ClrBit(GPIOI, BIT0); 
    vTaskDelay(100/portTICK_RATE_MS);
    GPIO_SetBit(GPIOI, BIT0);
    //vTaskDelay(5000/portTICK_RATE_MS);
    if(!ReadIT8951SystemInfo( preadFWVersion, NULL))
        EPDtestFlag = FALSE;
    
    //GPIO_ClrBit(GPIOI, BIT0);
    if(EPDtestFlag)
    {
        //EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE); 
        //EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
    }
    //vTaskDelay(1000/portTICK_RATE_MS);
    if(readEPDswitch() == TRUE)
    {
        //if(EPDtestFlag)
        //    EPDDrawString(TRUE,"DISABLE",90+325,100+(44*(8)));

        toggleEPDswitch(FALSE);
    }
    if(EPDtestFlag)
    {
        //EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE); 
        //EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
    }
    vTaskDelay(500/portTICK_RATE_MS);
    //vTaskDelay(1000/portTICK_RATE_MS);
    
    */

    
    if(memcmp(preadFWVersion,CmpStr,sizeof(CmpStr)) == 0)
    {
        MTPString[32][3] = 0x80;
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    else
    {
        terninalPrintf("readFWVersion=%s\r\n",preadFWVersion);
        MTPString[32][3] = 0x81;
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }

    
/*
    if(EPDIT8951Test())
        return TEST_SUCCESSFUL_LIGHT_OFF;
    else
        return TEST_FALSE;
    */
}

static BOOL CADPowerTest(void* para1, void* para2)
{
    
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOB, BIT4, DIR_OUTPUT, NO_PULL_UP);
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOB, BIT3, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOA, BIT14, DIR_OUTPUT, PULL_UP);

    //GPIO_SetBit(GPIOG, BIT6);
    //GPIO_SetBit(GPIOB, BIT3);
    GPIO_ClrBit(GPIOB, BIT3);
    GPIO_SetBit(GPIOB, BIT4);
    GPIO_SetBit(GPIOA, BIT14);
    
    char tempchar;
    SetMTPCRC(11,(uint8_t*)MTPString);
    MTPCmdprint(11,(uint8_t*)MTPString);
    terninalPrintf("IS CAD power 12V?(y/n)\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            terninalPrintf("CAD power OK.\r\n");
            
            //close CAD power
            GPIO_SetBit(GPIOB, BIT3);
            GPIO_ClrBit(GPIOB, BIT4);
            GPIO_ClrBit(GPIOA, BIT14);
            MTPString[10][3] = 0x81;
            return TEST_SUCCESSFUL_LIGHT_ON;
        }                
        else if(tempchar =='n')
        {
            terninalPrintf("CAD power ERROR.\r\n");
            
            //close CAD power
            GPIO_SetBit(GPIOB, BIT3);
            GPIO_ClrBit(GPIOB, BIT4);
            GPIO_ClrBit(GPIOA, BIT14);
            MTPString[10][3] = 0x80;
            if(MBtestFlag )
                return FALSE;
            else
                return TEST_FALSE;
        }                
    }
    
    
    
}

static BOOL CADSingleTest(void* para1, void* para2)
{
    BOOL resultFlag1 = FALSE;   //test CAD 12v power flag
    BOOL resultFlag2 = FALSE;   //test CAD Rx & Tx interconnect flag
    BOOL resultFlag3 = FALSE;   //test RTS & CTS interconnect flag
    BOOL resultFlag4 = FALSE;   //test CAD TEN & Solar I/O connect flag
    char SendCADStr[] = "Hello CAD";
    char SendDebugStr[] = "Cmd OK";
    char CADReadBuf[15];
    char DebugReadBuf[10];
    int ReadBufLen;
    // open CAD power
    UartInterface* CADUartInterface;
    CADUartInterface = UartGetInterface(UART_10_INTERFACE_INDEX);
    
    CommunicationInterface* CreditCardInterface;
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOB, BIT4, DIR_OUTPUT, NO_PULL_UP);
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOB, BIT3, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOA, BIT14, DIR_OUTPUT, PULL_UP);

    //GPIO_SetBit(GPIOG, BIT6);
    GPIO_SetBit(GPIOB, BIT3);
    GPIO_SetBit(GPIOB, BIT4);
    GPIO_SetBit(GPIOA, BIT14);
    
    // Initial debug UART and flush buffer      
    UartInterface* pUartInterface;
    pUartInterface = UartGetInterface(UART_1_INTERFACE_INDEX);
    if(pUartInterface == NULL)
    {   
        terninalPrintf("Debug UART GetInterface ERROR.\r\n");
    }        
    
    if(pUartInterface->initFunc(115200) == FALSE)
    {
        terninalPrintf("Debug UART GetInterface ERROR.\r\n");
    }
    uartIoctl(1, 25, 0, 0);  //UART1FlushBuffer , UARTA=1 , UART_IOC_FLUSH_RX_BUFFER=25 
    //pUartInterface->setPowerFunc(FALSE);
    //pUartInterface->setRS232PowerFunc(FALSE); 
    
    
    //vTaskDelay(3000/portTICK_RATE_MS);
    
    
    // Initial CAD and flush buffer
    CreditCardInterface = CommunicationGetInterface(0);
    CreditCardInterface->initFunc();
    
    CADUartInterface->setPowerFunc(TRUE);
    uartIoctl(10, 25, 0, 0);  //UART10FlushBuffer , UARTA=10 , UART_IOC_FLUSH_RX_BUFFER=25 
    //CreditCardInterface->readFunc((PUINT8)CADReadBuf, sizeof(CADReadBuf));
    //while(1);
    /*
    char tempchar;
    
    terninalPrintf("IS CAD power 12V?(y/n)\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            terninalPrintf("CAD power OK.\r\n");
            resultFlag1 = TRUE;
            break;
        }                
        else if(tempchar =='n')
        {
            terninalPrintf("CAD power ERROR.\r\n");
            resultFlag1 = FALSE;
            break;
        }                
    }
    */
    
    
    
    
    
    int index = 0;
    int counter = 0;
    INT32 reVal;
    int offsetindex = 0;

    // Debug send message to CAD
    pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    pUartInterface->writeFunc((PUINT8)SendCADStr,sizeof(SendCADStr));
    //vTaskDelay(10/portTICK_RATE_MS);
    memset(CADReadBuf, 0x0, sizeof(CADReadBuf));
    while(counter < 30)
    {

        reVal = CreditCardInterface->readFunc((PUINT8)CADReadBuf + index, sizeof(CADReadBuf)-index);
        vTaskDelay(10/portTICK_RATE_MS);
        if(reVal > 0)
        {
            index = index + reVal;
            
            
            /*
            terninalPrintf("CADReceive<=");
            for(int i=0;i<index;i++)
                terninalPrintf("%02x ",CADReadBuf[i]);
            terninalPrintf("\r\n");
            
            for(int k=0;k<index;k++)
            {
                if((CADReadBuf[k] >= 0x20) && (CADReadBuf[k] <= 0x7E))
                    terninalPrintf("%c",CADReadBuf[k]);
            } 
            terninalPrintf("\r\n"); 
            */
            
            while((CADReadBuf[offsetindex] != SendCADStr[0] ) && (offsetindex < (sizeof(CADReadBuf)-sizeof(SendCADStr))))
            {
                offsetindex++;
            }
            
            if(offsetindex>0)
            {
                memcpy(CADReadBuf,(char*)CADReadBuf+offsetindex,sizeof(SendCADStr));
                offsetindex = 0;
            }
            
            
            /*
            terninalPrintf("offsetindex = %d",offsetindex);
            terninalPrintf("CADReceive(mod)<=");
            for(int i=0;i<index;i++)
                terninalPrintf("%02x ",CADReadBuf[i]);
            terninalPrintf("\r\n");
            
            for(int k=0;k<index;k++)
            {
                if((CADReadBuf[k] >= 0x20) && (CADReadBuf[k] <= 0x7E))
                    terninalPrintf("%c",CADReadBuf[k]);
            } 
            terninalPrintf("\r\n"); 
            */
        }
   
        counter++;
    }
    
    if( memcmp(CADReadBuf,SendCADStr,sizeof(SendCADStr)) == 0 )
    {
        terninalPrintf("U14 Rx & U15 Tx connect success.\r\n");
        MTPString[26][3] = 0x81;
    }
    else
    {
        terninalPrintf("U14 Rx & U15 Tx connect error.\r\n");
        MTPString[26][3] = 0x80;
        return FALSE;
    }
    

    //vTaskDelay(1000/portTICK_RATE_MS);
        
            // CAD send message to Debug
    CADUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);    
    //terninalPrintf("1. DebudCTSValue=%32b\n",inpw(REG_UART0_MSR+UART1*UARTOFFSET));
    CreditCardInterface->writeFunc((PUINT8)SendDebugStr,sizeof(SendDebugStr));
    //terninalPrintf("2. DebudCTSValue=%32b\n",inpw(REG_UART0_MSR+UART1*UARTOFFSET));
    index = 0;
    counter = 0;

    memset(DebugReadBuf, 0x0, sizeof(DebugReadBuf));
    while(counter < 30)
    {

        reVal = pUartInterface->readFunc((PUINT8)DebugReadBuf + index, sizeof(DebugReadBuf)-index);
        vTaskDelay(10/portTICK_RATE_MS);
        if(reVal > 0)
        {
            index = index + reVal;
            /*
            terninalPrintf("DebugReceive<=");
            for(int i=0;i<index;i++)
                terninalPrintf("%02x ",DebugReadBuf[i]);
            terninalPrintf("\r\n");
            
            for(int k=0;k<index;k++)
            {
                if((DebugReadBuf[k] >= 0x20) && (DebugReadBuf[k] <= 0x7E))
                    terninalPrintf("%c",DebugReadBuf[k]);
            } 
            terninalPrintf("\r\n"); 
            */
        }
   
        counter++;
    }
    
       // terninalPrintf("3. DebudCTSValue=%32b\n",inpw(REG_UART0_MSR+UART1*UARTOFFSET));
    
    
    if( memcmp(DebugReadBuf,SendDebugStr,sizeof(SendDebugStr)) == 0 )
    {
        terninalPrintf("U14 Tx & U15 Rx connect success.\r\n");
        MTPString[26][4] = 0x81;
    }
    else
    {
        terninalPrintf("U14 Tx & U15 Rx connect error.\r\n");
        MTPString[26][4] = 0x80;
        return FALSE;
    }
    
    /*
    //vTaskDelay(1000/portTICK_RATE_MS);
    vTaskDelay(100/portTICK_RATE_MS);
    uint32_t CADCTSValue,DebudCTSValue;

    
    // Debug send RTS signal to CAD
    //pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
    vTaskDelay(100/portTICK_RATE_MS);
    CADCTSValue =  inpw(REG_UART0_MSR+UARTA*UARTOFFSET) ;
    //terninalPrintf("CADCTSValue=%32b\n",CADCTSValue);
    */
    
    
    BOOL DebugUR1RTSLowCADUR10CTSInFlag  = FALSE;
    BOOL DebugUR1RTSHighCADUR10CTSInFlag = FALSE;

    
    //Set DebugUR1RTS(PE4) and CADUR10CTS(PB15) GPIO

    GPIO_CloseBit(GPIOB, BIT15);
    GPIO_CloseBit(GPIOE, BIT4);
    
    //Set CADUR10CTS(PB15) input
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<< 28)) | (0<< 28));
    GPIO_OpenBit(GPIOB, BIT15, DIR_INPUT, PULL_UP);

    //Set DebugUR1RTS(PE4) output
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<< 16)) | (0<< 16));
    GPIO_OpenBit(GPIOE, BIT4, DIR_OUTPUT, PULL_UP);

    
    //Set DebugUR1RTS(PE4) LOW
    GPIO_ClrBit(GPIOE, BIT4);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT15))
        DebugUR1RTSLowCADUR10CTSInFlag  = TRUE;
        //terninalPrintf("CADUR10CTS receieve DebugUR1RTSLow success\r\n");
    else
        DebugUR1RTSLowCADUR10CTSInFlag  = FALSE;
        //terninalPrintf("CADUR10CTS receieve DebugUR1RTSLow error.\r\n");
    
    //Set DebugUR1RTS(PE4) HIGH
    GPIO_SetBit(GPIOE, BIT4);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT15))
        DebugUR1RTSHighCADUR10CTSInFlag = TRUE;
        //terninalPrintf("CADUR10CTS receieve DebugUR1RTSHigh success\r\n");
    else
        DebugUR1RTSHighCADUR10CTSInFlag = FALSE;
        //terninalPrintf("CADUR10CTS receieve DebugUR1RTSHigh error.\r\n");
    GPIO_CloseBit(GPIOB, BIT15);
    GPIO_CloseBit(GPIOE, BIT4);
    
    
   // if(CADCTSValue & 0x01 )
    if(DebugUR1RTSLowCADUR10CTSInFlag & DebugUR1RTSHighCADUR10CTSInFlag )
    {
        terninalPrintf("U14 CTS & U15 RTS connect success.\r\n");
        MTPString[26][5] = 0x81;
    }
    else
    {        
        terninalPrintf("U14 CTS & U15 RTS connect error.\r\n");
        MTPString[26][5] = 0x80;
        return FALSE;
     
    }
    
    /*
    // CAD send RTS signal to Debug   
    //CADUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    CADUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
    vTaskDelay(100/portTICK_RATE_MS);
    DebudCTSValue =  inpw(REG_UART0_MSR+UART1*UARTOFFSET) ;
    //terninalPrintf("DebudCTSValue=%32b\n",DebudCTSValue);
    */
    
    
    BOOL CADUR10RTSLowDebugUR1CTSInFlag  = FALSE;
    BOOL CADUR10RTSHighDebugUR1CTSInFlag = FALSE;

    
    //Set CADUR10RTS(PB14) and DebugUR1CTS(PE5) GPIO

    GPIO_CloseBit(GPIOE, BIT5);
    GPIO_CloseBit(GPIOB, BIT14);
    
    //Set DebugUR1CTS(PE5) input
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<< 20)) | (0<< 20));
    GPIO_OpenBit(GPIOE, BIT5, DIR_INPUT, PULL_UP);

    //Set CADUR10RTS(PB14) output
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<< 24)) | (0<< 24));
    GPIO_OpenBit(GPIOB, BIT14, DIR_OUTPUT, PULL_UP);

    
    //Set CADUR10RTS(PB14) LOW
    GPIO_ClrBit(GPIOB, BIT14);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOE, BIT5))
        CADUR10RTSLowDebugUR1CTSInFlag  = TRUE;
        //terninalPrintf("DebugUR1CTS receieve CADUR10RTSLow success\r\n");
    else
        CADUR10RTSLowDebugUR1CTSInFlag  = FALSE;
        //terninalPrintf("DebugUR1CTS receieve CADUR10RTSLow error.\r\n");
    
    //Set CADUR10RTS(PB14) HIGH
    GPIO_SetBit(GPIOB, BIT14);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOE, BIT5))
        CADUR10RTSHighDebugUR1CTSInFlag = TRUE;
        //terninalPrintf("DebugUR1CTS receieve CADUR10RTSHigh success\r\n");
    else
        CADUR10RTSHighDebugUR1CTSInFlag = FALSE;
        //terninalPrintf("DebugUR1CTS receieve CADUR10RTSHigh error.\r\n");
    GPIO_CloseBit(GPIOE, BIT5);
    GPIO_CloseBit(GPIOB, BIT14);
    
    
    
    //if(DebudCTSValue & 0x01 )
    if(CADUR10RTSLowDebugUR1CTSInFlag & CADUR10RTSHighDebugUR1CTSInFlag )
    {
        terninalPrintf("U14 RTS & U15 CTS connect success.\r\n");
        MTPString[26][6] = 0x81;
    }
    else
    {        
        terninalPrintf("U14 RTS & U15 CTS connect error.\r\n");
        MTPString[26][6] = 0x80;  
        return FALSE;
    }
    /*
    if(resultFlag2 && resultFlag3)
    {
        terninalPrintf("U14 & U15 RX,TX,CTS,RTS connect success.\r\n");
        IOtestResultFlag[0] = TRUE;
        MTPString[26][3] = 0x81;
    }
    else
    {
        terninalPrintf("U14 & U15 RX,TX,CTS,RTS connect error.\r\n");
        IOtestResultFlag[0] = FALSE;
        return FALSE;
    }
    */
    SetCADtenConnectFlagFunc(FALSE);
    //vTaskDelay(1000/portTICK_RATE_MS);
    vTaskDelay(100/portTICK_RATE_MS);
    GPIO_ClrBit(GPIOA, BIT14); // TEN I/O clear LOW
    
    if(ReadCADtenConnectFlagFunc())
    {
        terninalPrintf("CAD TEN & Solar I/O connect success.\r\n");
        resultFlag4 = TRUE;
        IOtestResultFlag[1] = TRUE;
        MTPString[26][7] = 0x81;
    }
    else
    {
        terninalPrintf("CAD TEN & Solar I/O connect error.\r\n");
        resultFlag4 = FALSE;
        IOtestResultFlag[1] = FALSE;
        MTPString[26][7] = 0x80;
        return FALSE;
    }
    
    
    //vTaskDelay(3000/portTICK_RATE_MS);
    
    //close debug UART RS232 power
    pUartInterface->setRS232PowerFunc(TRUE);
    
    //close CAD RS232 power
    CADUartInterface->setPowerFunc(FALSE);
    
    //close CAD UART IO
    
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<16)) | (0x0<<16));
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<20)) | (0x0<<20));
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<24)) | (0x0<<24));
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xFu<<28))| (0x0<<28)); 
    
    GPIO_OpenBit(GPIOB, BIT12, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_OpenBit(GPIOB, BIT13, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOB, BIT14, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOB, BIT15, DIR_OUTPUT, NO_PULL_UP);
    
    GPIO_ClrBit(GPIOB, BIT12);
    GPIO_ClrBit(GPIOB, BIT13);
    GPIO_ClrBit(GPIOB, BIT14);
    GPIO_ClrBit(GPIOB, BIT15);
    
    //close debug UART IO
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<8))  | (0x0<<8));
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<12)) | (0x0<<12));
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<16)) | (0x0<<16));
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<20)) | (0x0<<20));   
    
    GPIO_OpenBit(GPIOE, BIT2, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOE, BIT3, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOE, BIT4, DIR_OUTPUT, NO_PULL_UP);
    GPIO_OpenBit(GPIOE, BIT5, DIR_OUTPUT, NO_PULL_UP);
    
    GPIO_ClrBit(GPIOE, BIT2);
    GPIO_ClrBit(GPIOE, BIT3);
    GPIO_ClrBit(GPIOE, BIT4);
    GPIO_ClrBit(GPIOE, BIT5);
    
    //close CAD power

    GPIO_SetBit(GPIOB, BIT3);
    GPIO_ClrBit(GPIOB, BIT4);
    GPIO_ClrBit(GPIOA, BIT14);
    

    if( (MTPString[26][3] == 0x81)  &&  
        (MTPString[26][4] == 0x81)  &&
        (MTPString[26][5] == 0x81)  &&
        (MTPString[26][6] == 0x81)  &&
        (MTPString[26][7] == 0x81)  
      )
    {
        MTPString[26][8] = 0x81;
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
    else
    {
        MTPString[26][8] = 0x80;
        if(MBtestFlag)
            return FALSE;
        else
            return TEST_FALSE; 
    }
    
   /* 
    if( IOtestResultFlag[0]  &&  
        IOtestResultFlag[1]
      )
    {
        MTPString[26][5] = 0x81;
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
    else
    {
        if(MBtestFlag)
            return FALSE;
        else
            return TEST_FALSE; 
    }
    */
    /*
    //if( (resultFlag1 == TRUE) && (resultFlag2 == TRUE) && (resultFlag3 == TRUE) && (resultFlag4 == TRUE) )
    if( (resultFlag2 == TRUE) && (resultFlag3 == TRUE) && (resultFlag4 == TRUE) )
        return TEST_SUCCESSFUL_LIGHT_ON;
    else
        return TEST_FALSE;
    
    */
}

void QueryCADtimeoutFunc(BOOL timeoutFlag)
{
    gCADtimeoutFlag = timeoutFlag;
}

static BOOL CADReaderTest(void* para1, void* para2)
{
    uint8_t pucBuf[300];
    int waitcounter = 0;
    int mwaitcounter = 0;
    uint8_t currentSequenceNum = 0;
    gCADtimeoutFlag = FALSE;
    
    guiPrintResult("credit card\nTesting");
    //guiPrintResult("credit card Init");
    
    CADReadCardinit((uint8_t*)pucBuf);
    vTaskDelay(2000/portTICK_RATE_MS);
    terninalPrintf("Please insert credit card in 10 seconds.\r\n");
    guiPrintMessage("Please Tag\nor key = skip");
    GuiManagerCleanMessage(GUI_CAD_TIMER_ENABLE);
    
    char tempchar;
    
    CADReadCard((uint8_t*)pucBuf ,&currentSequenceNum);
    
    while(1)
    {
        //terninalPrintf("%d \r",10-waitcounter);
        if(CADReadCard((uint8_t*)pucBuf ,&currentSequenceNum))
        {
            //BuzzerPlay(200, 500, 1, TRUE);
            GuiManagerCleanMessage(GUI_CAD_TIMER_DISABLE);
            break;
        }
        
        
        GuiManagerCleanMessage(GUI_CAD_TIMEROUT_FLAG);
        if(gCADtimeoutFlag)
            break;
        
        tempchar = userResponse();
        if( (tempchar =='n') || !DetectCADConnect())
        {
            GuiManagerCleanMessage(GUI_CAD_TIMER_DISABLE);
            CADReadCardpoweroff();
            return TEST_FALSE;
        }

        /*
        else
            mwaitcounter++;
        if(mwaitcounter >= 2)
        {
            mwaitcounter = 0;
            waitcounter++; 
        }
        if(waitcounter >= 10)
            break;
        vTaskDelay(500/portTICK_RATE_MS);
        */
    }
    
    
    CADReadCardpoweroff();
    //if(waitcounter >= 10)
    if(gCADtimeoutFlag)
        return TEST_FALSE;
    else
        return TEST_SUCCESSFUL_LIGHT_OFF;

    
    
    
}

//////////////////////Battery test////////////////////////TEST_SUCCESSFUL_LIGHT_ON
static BOOL batteryTest(void* para1, void* para2)
{    
    char string1[50];
    terninalPrintf("!!! batteryTest !!!\r\n");
    if(MBtestFlag )
    {}
    else
        guiPrintResult("Battery Testing");
//    uint16_t ledData = 0x00;
    //BOOL lastStatus[] = {IOPIN_BOTH_OFF, IOPIN_ONLY1_ON, IOPIN_BOTH_OFF, IOPIN_ONLY2_ON, IOPIN_BOTH_OFF, SOLAR_OFF, SOLAR_ON}; 
    BOOL lastStatus[] = {IOPIN_BOTH_OFF, IOPIN_ONLY1_ON, IOPIN_BOTH_OFF, IOPIN_ONLY2_ON, IOPIN_BOTH_OFF};
    BOOL statusStage = 0;
    //BOOL pinStage = 0;
    UINT32 solarVoltage ,leftVoltage,  rightVoltage;

    if(MBtestFlag )
    {
        char tempchar;
        BOOL batResultFlag1 = FALSE;   // Test battery1 
        BOOL batResultFlag2 = FALSE;   // Test battery2
        BOOL batResultFlag3 = FALSE;   // Test solar battery
        BOOL batResultFlag4 = FALSE;   // ADC0
        BOOL batResultFlag5 = FALSE;   // ADC1
        BOOL batResultFlag6 = FALSE;   // ADC2
        BOOL batResultFlag7 = FALSE;   //Test SW1
        BOOL batResultFlag8 = FALSE;   //Test SW2
        
        int waitCount; 
        
        BatteryDrvInit(FALSE);
/*
        terninalPrintf("Test solar battery.\r\n");
        BatterySetSwitch1(FALSE);
        BatterySetSwitch2(FALSE);
        
        outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<<24)));
        GPIO_OpenBit(GPIOE,BIT14, DIR_OUTPUT, NO_PULL_UP); 
        
        GPIO_ClrBit(GPIOE,BIT14);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_SetBit(GPIOE,BIT14);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_ClrBit(GPIOE,BIT14);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_SetBit(GPIOE,BIT14);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_ClrBit(GPIOE,BIT14);
        
        terninalPrintf("Is the D7 light signal correct?(y/n)\r\n");
        while(1)
        {
            tempchar = userResponseLoop();
            if(tempchar =='y')
            {
                terninalPrintf("solar battery control success.\r\n");
                batResultFlag1 =TRUE;
                break;
            }
            else if(tempchar =='n')
            {
                terninalPrintf("solar battery control error.\r\n");
                batResultFlag1 = FALSE;
                return TEST_FALSE;
            }
        }
        */
        
        terninalPrintf("Test battery voltage.\r\n");
        //GPIO_ClrBit(GPIOE,BIT14);
        //BatterySetSwitch2(FALSE);
        
        BatteryGetVoltage();				
        vTaskDelay(500/portTICK_RATE_MS);
        BatteryGetValue(&solarVoltage, &leftVoltage, &rightVoltage);
        terninalPrintf(" Voltage: [%d], [%d] , SolarVol: [%d]\r\n", leftVoltage, rightVoltage, solarVoltage);
        
        if((leftVoltage < MB_BAT_MIN_VOLTAGE) || (leftVoltage > MB_BAT_MAX_VOLTAGE))
        {
            terninalPrintf("Battery 1 Voltage: [%d][ERROR]\r\n", leftVoltage);
            batResultFlag5= FALSE;
            MTPString[0][3] = 0x80;
        }
        else
        {
            batResultFlag5= TRUE;
            MTPString[0][3] = 0x81;
        }
        if((rightVoltage < MB_BAT_MIN_VOLTAGE) || (rightVoltage > MB_BAT_MAX_VOLTAGE))
        {   
            terninalPrintf("Battery 2 Voltage: [%d][ERROR]\r\n", rightVoltage);
            batResultFlag6= FALSE;
            MTPString[0][4] = 0x80;
        }
        else
        {
            batResultFlag6= TRUE;
            MTPString[0][4] = 0x81;
        }
        if((solarVoltage < MB_BAT_MIN_VOLTAGE) || (solarVoltage > MB_BAT_MAX_VOLTAGE))
        {   
            terninalPrintf("Solar Bat Voltage: [%d][ERROR]\r\n", solarVoltage);
            batResultFlag4= FALSE;
            MTPString[0][5] = 0x80;
        }
        else
        {
            batResultFlag4= TRUE;
            MTPString[0][5] = 0x81;
        }
        if(!(batResultFlag4 && batResultFlag5 && batResultFlag6))  
        {    
            //SetMTPCRC(0,(uint8_t*)MTPString);
            //MTPCmdprint(0,(uint8_t*)MTPString);            
            return FALSE;
        }
        
        SetMTPCRC(52,(uint8_t*)MTPString);
        MTPCmdprint(52,(uint8_t*)MTPString); 
        terninalPrintf("Test battery1.\r\n");
        vTaskDelay(1000/portTICK_RATE_MS);
        BatterySetSwitch1(TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        BatterySetSwitch1(FALSE);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        BatterySetSwitch1(TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        //BatterySetSwitch1(FALSE);
        //vTaskDelay(1000/portTICK_RATE_MS);
        //BatterySetSwitch1(TRUE);
        //vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_SetBit(GPIOE,BIT14);
        
        SetMTPCRC(1,(uint8_t*)MTPString);
        MTPCmdprint(1,(uint8_t*)MTPString); 
        terninalPrintf("Is the D7 & D8 light signal correct?(y/n)\r\n");
        while(1)
        {
            tempchar = userResponseLoop();
            if(tempchar =='y')
            {
                terninalPrintf("battery1 control success.\r\n");
                batResultFlag1 =TRUE;
                MTPString[0][6] = 0x81;
                break;
            }
            else if(tempchar =='n')
            {
                terninalPrintf("battery1 control error.\r\n");
                batResultFlag1 = FALSE;
                
                GPIO_ClrBit(GPIOE,BIT14);
                vTaskDelay(1000/portTICK_RATE_MS);
                BatterySetSwitch1(FALSE);
                BatterySetSwitch2(FALSE);
                MTPString[0][6] = 0x80;                           
                return FALSE;
                break;
            }
        }
        
        SetMTPCRC(53,(uint8_t*)MTPString);
        MTPCmdprint(53,(uint8_t*)MTPString); 
        terninalPrintf("Test battery2.\r\n");
        vTaskDelay(1000/portTICK_RATE_MS);
        //GPIO_SetBit(GPIOE,BIT14);
        BatterySetSwitch2(TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        BatterySetSwitch2(FALSE);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        BatterySetSwitch2(TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        //BatterySetSwitch2(FALSE);
        //vTaskDelay(1000/portTICK_RATE_MS);
        //BatterySetSwitch2(TRUE);
        //vTaskDelay(1000/portTICK_RATE_MS);
        BatterySetSwitch1(FALSE);
        
        SetMTPCRC(2,(uint8_t*)MTPString);
        MTPCmdprint(2,(uint8_t*)MTPString);
        terninalPrintf("Is the D8 & D9 light signal correct?(y/n)\r\n");
        while(1)
        {
            tempchar = userResponseLoop();
            if(tempchar =='y')
            {
                terninalPrintf("battery2 control success.\r\n");
                batResultFlag2 =TRUE;
                MTPString[0][7] = 0x81;
                break;
            }
            else if(tempchar =='n')
            {
                terninalPrintf("battery2 control error.\r\n");
                batResultFlag2= FALSE;
                
                
                GPIO_ClrBit(GPIOE,BIT14); 
                vTaskDelay(1000/portTICK_RATE_MS);
                BatterySetSwitch1(FALSE);
                BatterySetSwitch2(FALSE);
                MTPString[0][7] = 0x80;
                return FALSE;
                break;
            }
        }
        
        SetMTPCRC(54,(uint8_t*)MTPString);
        MTPCmdprint(54,(uint8_t*)MTPString);
        terninalPrintf("Test solar battery.\r\n");
        vTaskDelay(1000/portTICK_RATE_MS);
        //BatterySetSwitch1(FALSE);
        //BatterySetSwitch2(FALSE); 
        
        GPIO_ClrBit(GPIOE,BIT14);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        GPIO_SetBit(GPIOE,BIT14);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        GPIO_ClrBit(GPIOE,BIT14);
        vTaskDelay(1000/portTICK_RATE_MS);
        //vTaskDelay(500/portTICK_RATE_MS);
        //GPIO_SetBit(GPIOE,BIT14);
        //vTaskDelay(1000/portTICK_RATE_MS);
        //GPIO_ClrBit(GPIOE,BIT14);
        //vTaskDelay(1000/portTICK_RATE_MS);
        BatterySetSwitch2(FALSE); 
        
        SetMTPCRC(3,(uint8_t*)MTPString);
        MTPCmdprint(3,(uint8_t*)MTPString);
        terninalPrintf("Is the D7 & D9 light signal correct?(y/n)\r\n");
        while(1)
        {
            tempchar = userResponseLoop();
            if(tempchar =='y')
            {
                terninalPrintf("solar battery control success.\r\n");
                batResultFlag3 =TRUE;
                MTPString[0][8] = 0x81;
                break;
            }
            else if(tempchar =='n')
            {
                terninalPrintf("solar battery control error.\r\n");
                batResultFlag3 = FALSE;
                
                GPIO_ClrBit(GPIOE,BIT14); 
                vTaskDelay(1000/portTICK_RATE_MS);
                BatterySetSwitch1(FALSE);
                BatterySetSwitch2(FALSE);
                MTPString[0][8] = 0x80;
                return FALSE;
                //return TEST_FALSE;
                break;
            }
        }
        
        
        
        
        SetMTPCRC(4,(uint8_t*)MTPString);
        MTPCmdprint(4,(uint8_t*)MTPString);
        terninalPrintf("Test SW1.\r\n");
        SetBatINTtestFlagFunc(1,FALSE);
        waitCount = 10;
        
        while(1)
        {
            if(ReadBatINTtestFlagFunc(1))
            {
                terninalPrintf("SW1 test success.\r\n");
                batResultFlag7= TRUE;
                BuzzerPlay(200, 500, 1, TRUE);
                LEDColorBuffSet(0x00,0x04 );
                LEDBoardLightSet();
                MTPString[0][9] = 0x81;
                break;
            }
            
            if(waitCount <= 0)
            {
                terninalPrintf("SW1 test error.\r\n");
                batResultFlag7= FALSE;
                MTPString[0][9] = 0x80;
                return FALSE;
                break;
            }
            waitCount--;
            vTaskDelay(1000/portTICK_RATE_MS);
        }
        
        SetMTPCRC(5,(uint8_t*)MTPString);
        MTPCmdprint(5,(uint8_t*)MTPString);
        terninalPrintf("Test SW2.\r\n");
        SetBatINTtestFlagFunc(2,FALSE);
        waitCount = 10;
        
        while(1)
        {
            if(ReadBatINTtestFlagFunc(2))
            {
                terninalPrintf("SW2 test success.\r\n");
                //LEDColorBuffSet(0x00,0x20
                BuzzerPlay(200, 500, 1, TRUE);
                LEDColorBuffSet(0x00,0x08);
                LEDBoardLightSet();
                batResultFlag8= TRUE;
                MTPString[0][10] = 0x81;
                break;
            }
            
            if(waitCount <= 0)
            {
                terninalPrintf("SW2 test error.\r\n");
                batResultFlag8= FALSE;
                //SetMTPCRC(0,(uint8_t*)MTPString);
                //MTPCmdprint(0,(uint8_t*)MTPString);
                MTPString[0][10] = 0x80;
                return FALSE;
                break;
            }
            waitCount--;
            vTaskDelay(1000/portTICK_RATE_MS);
        }
        vTaskDelay(3000/portTICK_RATE_MS);
        LEDColorBuffSet(0x00, 0x00);
        LEDBoardLightSet();
        
        if( batResultFlag1  &&
            batResultFlag2  &&
            batResultFlag3  &&
            batResultFlag4  &&
            batResultFlag5  &&
            batResultFlag6  &&
            batResultFlag7  && 
            batResultFlag8 )
        {
            MTPString[0][11] = 0x81;
            //SetMTPCRC(0,(uint8_t*)MTPString);
            //MTPCmdprint(0,(uint8_t*)MTPString);
            return TEST_SUCCESSFUL_LIGHT_ON;
        }
        else
        {
            //SetMTPCRC(0,(uint8_t*)MTPString);
            //MTPCmdprint(0,(uint8_t*)MTPString);
            MTPString[0][11] = 0x80;
            return FALSE;
        }
            //return TEST_FALSE;
        
    }
    else
    {
        int mtimeoutCounter1 = 0;  // Bat1 & Bat2 mtimeoutCounter
        int timeoutCounter1 = 0;  // Bat1 & Bat2 timeoutCounter
        int mtimeoutCounter2 = 0;  // SolarBat mtimeoutCounter
        int timeoutCounter2 = 0;  // SolarBat timeoutCounter
        if(BatteryDrvInit(FALSE))
        {
            //terninalPrintf(" Battery Driver Init [OK]...(press 'n' to exit!!!) \r\n");
            //vTaskDelay(15/portTICK_RATE_MS);
            guiPrintMessage("Battery Testing");
            //UINT32 solarVoltage ,leftVoltage,  rightVoltage;
            //BatterySetEnableTestMode(TRUE);
            BatteryGetVoltage();				
            vTaskDelay(500/portTICK_RATE_MS);
            BatteryGetValue(&solarVoltage, &leftVoltage, &rightVoltage);
            terninalPrintf(" Voltage: [%d], [%d] , SolarVol: [%d]\r\n", leftVoltage, rightVoltage, solarVoltage);
            sprintf(string1,"Battery\nLeft :%d.%dV\nRight:%d.%dV\nSolar:%d.%dV",
                    leftVoltage/100,(leftVoltage/10)%10,
                    rightVoltage/100,(rightVoltage/10)%10,
                    solarVoltage/100,(solarVoltage/10)%10);
            //vTaskDelay(15/portTICK_RATE_MS);
            guiPrintMessage(string1);
            

            if(MBtestFlag )
            {  
                if((leftVoltage < MB_BAT_MIN_VOLTAGE) || (leftVoltage > MB_BAT_MAX_VOLTAGE))
                {
                    terninalPrintf("Battery 1 Voltage: [%d][ERROR]\r\n", leftVoltage);
                    return TEST_FALSE;
                }
                if((rightVoltage < MB_BAT_MIN_VOLTAGE) || (rightVoltage > MB_BAT_MAX_VOLTAGE))
                {   
                    terninalPrintf("Battery 2 Voltage: [%d][ERROR]\r\n", rightVoltage);
                    return TEST_FALSE;
                }
                if((solarVoltage < MB_BAT_MIN_VOLTAGE) || (solarVoltage > MB_BAT_MAX_VOLTAGE))
                {   
                    terninalPrintf("Solar Bat Voltage: [%d][ERROR]\r\n", solarVoltage);
                    return TEST_FALSE;
                }
            }
            else
            {
                if((leftVoltage < BATTERY_L_MIN_VOLTAGE) || (leftVoltage > BATTERY_L_MAX_VOLTAGE))
                {
                    terninalPrintf("Battery 1 Voltage: [%d][ERROR]\r\n", leftVoltage);
                    return TEST_FALSE;
                }
                if((rightVoltage < BATTERY_R_MIN_VOLTAGE) || (rightVoltage > BATTERY_R_MAX_VOLTAGE))
                {   
                    terninalPrintf("Battery 2 Voltage: [%d][ERROR]\r\n", rightVoltage);
                    return TEST_FALSE;
                }
                if((solarVoltage < SOLAR_BAT_MIN_VOLTAGE) || (solarVoltage > SOLAR_BAT_MAX_VOLTAGE))
                {   
                    terninalPrintf("Solar Bat Voltage: [%d][ERROR]\r\n", solarVoltage);
                    return TEST_FALSE;
                }
            }
            terninalPrintf("  Start check battery \r\n");
            terninalPrintf("Please Remove Battery 1\r\n");
            //vTaskDelay(15/portTICK_RATE_MS);
            guiPrintMessage("Remove Bat 1");
            //LEDColorBuffSet(0x38, 0x00);
            LEDColorBuffSet(0x01, 0x00);
            LEDBoardLightSet();
            //BOOL portValue = (GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN) << 0) 
            //    | (GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN) << 1) ;
            BOOL portValue = (GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN) << 1)
                        | (GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN) << 0);

            if((portValue) != lastStatus[statusStage])
            {
                terninalPrintf("Battery test operation[ERROR]\r\n");
                return TEST_FALSE;
            }
            else
            {
                //terninalPrintf("Please Remove Battery 1\r\n");
                statusStage++;
                while(1)
                {
                    //portValue = (GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN) << 1)
                    //    | (GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN) << 0);
                    portValue = (GPIO_ReadBit(BATTERY_1_LOW_DETECT_PORT, BATTERY_1_LOW_DETECT_PIN) << 0) 
                                | (GPIO_ReadBit(BATTERY_2_LOW_DETECT_PORT, BATTERY_2_LOW_DETECT_PIN) << 1) ;
                    if((portValue != lastStatus[statusStage-1]) && (portValue != lastStatus[statusStage]))
                    {
                        testErroCode = BATTERY_SEQUENCE_ERRO;
                        //terninalPrintf("Battery Test [ERROR]");
                        return TEST_FALSE;
                    }
                    else if(portValue == lastStatus[statusStage])
                        {
                        switch(portValue)
                        {
                        case IOPIN_BOTH_ON:
                            terninalPrintf("IOPIN_BOTH_ON");
                            break;
                        case IOPIN_ONLY1_ON:
                            cleanMsg();
                            //vTaskDelay(15/portTICK_RATE_MS);
                            guiPrintMessage("Connect Bat 1");
                            terninalPrintf("Connect Battery 1\n");
                        
                            //LEDColorBuffSet(0x00,0x04 );
                            //LEDBoardLightSet();
                            /*
                            LEDColorBuffSet(0x38, 0x00);
                            LEDBoardLightSet();
                            */
                            break;
                        case IOPIN_ONLY2_ON:
                            terninalPrintf("Please connect Battery 2\r\n");
                            vTaskDelay(15/portTICK_RATE_MS);
                            guiPrintMessage("Connect Bat 2");
                            /*
                            LEDColorBuffSet(0x03, 0x00);
                            LEDBoardLightSet();
                            */
                            break;
                        case IOPIN_BOTH_OFF:
                            if(statusStage==4) break;
                            terninalPrintf("Power both neither connect !!!\r\n");
                            //vTaskDelay(15/portTICK_RATE_MS);
                            guiPrintMessage("Remove Bat 2");
                            terninalPrintf("Please Remove Battery 2\r\n");
                            //LEDColorBuffSet(0x03, 0x00);
                            LEDColorBuffSet(0x20, 0x00);
                            LEDBoardLightSet();
                            break;
                        /*
                        case SOLAR_OFF:

                            guiPrintMessage("Remove Solar Bat ");
                            terninalPrintf("Please Remove Solar Bat\r\n");
                            LEDColorBuffSet(0x20, 0x00);
                            LEDBoardLightSet();
                            break;
                        case SOLAR_ON:
                            terninalPrintf("Please connect Solar Bat\r\n");
                            guiPrintMessage("Connect Solar Bat");

                            break;
                        */
                        default:
                            terninalPrintf("Power = 0x%02x !!!\r\n", portValue);
                            break;
                        }
                        statusStage++;
                        timeoutCounter1 = 0;
                    }

                    if(statusStage == 5)
                    {
                        statusStage = 0;
                        break;
                    }      
                    //vTaskDelay(300/portTICK_RATE_MS);
                    vTaskDelay(200/portTICK_RATE_MS);
                    mtimeoutCounter1++;
                    if(mtimeoutCounter1 >= 5)
                    {
                        mtimeoutCounter1 = 0 ;
                        terninalPrintf(" %d \r", 15 - timeoutCounter1);
                        timeoutCounter1++ ;
                        if(timeoutCounter1 >= 15)
                        {
                            return TEST_FALSE;
                        }
                    }
                    
                    
                }
            }
            /*
            guiPrintMessage("Remove Solar Bat ");
            terninalPrintf("Please Remove Solar Bat\r\n");

            
                LedSetStatusLightFlush(50,10);  // solve LED board bug
            LedSetColor(bayColorOff, 0x08, TRUE);
            LedSetBayLightFlush(0xff,8);
            vTaskDelay(100/portTICK_RATE_MS);
            //LedSetColor(modemColorAllGreen, 0x04, TRUE);    
            LedSetAliveStatusLightFlush(0xff,8);
 
            while(!GPIO_ReadBit(GPIOH,BIT7))
            {
                mtimeoutCounter2++;
                if(mtimeoutCounter2 >= 5)
                {
                    mtimeoutCounter2 = 0 ;
                    terninalPrintf(" %d \r", 15 - timeoutCounter2);
                    timeoutCounter2++ ;
                    if(timeoutCounter2 >= 15)
                    {
                        LedSetStatusLightFlush(0,10);  // solve LED board bug
                        return TEST_FALSE;
                    }
                }
                vTaskDelay(200/portTICK_RATE_MS);
            }

            terninalPrintf("Please connect Solar Bat\r\n");
            //vTaskDelay(15/portTICK_RATE_MS);
            guiPrintMessage("Connect Solar Bat");
            //while(GPIO_ReadBit(GPIOH,BIT7));
            mtimeoutCounter2 = 0;
            timeoutCounter2 = 0;
            while(GPIO_ReadBit(GPIOH,BIT7))
            {
                mtimeoutCounter2++;
                if(mtimeoutCounter2 >= 5)
                {
                    mtimeoutCounter2 = 0 ;
                    terninalPrintf(" %d \r", 15 - timeoutCounter2);
                    timeoutCounter2++ ;
                    if(timeoutCounter2 >= 15)
                    {
                        LedSetStatusLightFlush(0,10);  // solve LED board bug
                        return TEST_FALSE;
                    }
                }
                vTaskDelay(200/portTICK_RATE_MS);
            }
            
            setBatterySwitchStatus(TRUE, FALSE);
            LedSetStatusLightFlush(0,10);  // solve LED board bug
            //LEDColorBuffSet(0x1F, 0x00);		
            LEDColorBuffSet(0x00, 0x00);	        
            LEDBoardLightSet();
            vTaskDelay(2000/portTICK_RATE_MS);
            setBatterySwitchStatus(FALSE, TRUE);
            LEDColorBuffSet(0x00, 0x00);							
            LEDBoardLightSet();
            vTaskDelay(2000/portTICK_RATE_MS);
            setBatterySwitchStatus(TRUE, TRUE);
            //terninalPrintf("Battery Test [OK]");
            
            */
            return TEST_SUCCESSFUL_LIGHT_ON;
        }
        
        setPrintfFlag(TRUE);
        //return TRUE;
        terninalPrintf(" Battery Driver Init [ERROR]\r\n");
        return TEST_FALSE;
    }
}

static BOOL SuperCapTest(void* para1, void* para2)
{
    char tempchar;
    BOOL SuperCapResultFlag = FALSE;
    
    SetMTPCRC(39,(uint8_t*)MTPString);
    MTPCmdprint(39,(uint8_t*)MTPString);
    
    BatterySetSwitch1(FALSE);
    BatterySetSwitch2(FALSE); 
    GPIO_SetBit(GPIOE,BIT14);
    
    vTaskDelay(500/portTICK_RATE_MS);
    
    LEDColorBuffSet(0x0C, 0x00);
    LEDBoardLightSet();

    SetMTPCRC(37,(uint8_t*)MTPString);
    MTPCmdprint(37,(uint8_t*)MTPString);
    terninalPrintf("Is the D3 & D4 light signal correct?(y/n)\r\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            terninalPrintf("SuperCap test success.\r\n");
            SuperCapResultFlag = TRUE;
             MTPString[36][3] = 0x81;
            break;
        }
        else if(tempchar =='n')
        {
            terninalPrintf("SuperCap test error.\r\n");
            SuperCapResultFlag = FALSE;
            break;
        }
    }
    
    BatterySetSwitch1(FALSE);
    BatterySetSwitch2(FALSE); 
    GPIO_ClrBit(GPIOE,BIT14);
    
    LEDColorBuffSet(0x00, 0x00);
    LEDBoardLightSet();
    
    if(SuperCapResultFlag)
        return TEST_SUCCESSFUL_LIGHT_OFF;
    else
    {
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    
}

//////////////////////Reader   test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL fReaderCn=FALSE;
static void readerCNCallback(BOOL flag, uint8_t* cn, int cnLen)/* CALLBACK FUNCTION */
{
    static BOOL prevFlag = FALSE;
    char stringID[80];
    if(prevFlag != flag)
    {
        if(flag)
        {
            BuzzerPlay(200, 2, 1, FALSE);
            uint32_t tmpCardID=0;
            cleanMsg();
            terninalPrintf("Detect Card!\nCard ID:%02X%02X%02X%02X\n\n",cn[0],cn[1],cn[2],cn[3]);
            sprintf(stringID,"Detect Card!\nCard ID:\n%02X%02X%02X%02X\n\n",cn[0],cn[1],cn[2],cn[3]);
            for(int i = 0; i<cnLen; i++)
            {
                uint32_t tmpByte = cn[i];
                tmpCardID = tmpCardID | tmpByte << 8*(cnLen-i-1);
            }
            cardID=tmpCardID;
            //terninalPrintf("\r\n");
            //show id at epd
            EPDDrawString(TRUE,stringID,X_POS_MSG,Y_POS_MSG);
            fReaderCn=TRUE;
        }
    }
//    terninalPrintf(".");
    prevFlag = flag;
}


void octopusDepositResultCallback(BOOL flag, uint16_t paraValue, uint16_t infoValue)
{    
    char stringID[80];
    if(flag)
    {
        BuzzerPlay(200, 2, 1, FALSE);
        cleanMsg();
        terninalPrintf("Deduct Success\n\n Balance: %d\r\n\r\n", infoValue);
        sprintf(stringID,"Deduct Success\n\n Balance: %d\r\n\r\n", infoValue);

        EPDDrawString(TRUE,stringID,X_POS_MSG,Y_POS_MSG);
        
    }
    else
    {
        BuzzerPlay(200, 2, 1, FALSE);
        cleanMsg();
        //terninalPrintf("Deduct Fail\n\n");
        sprintf(stringID,"Deduct Fail\n\n");

        EPDDrawString(TRUE,stringID,X_POS_MSG,Y_POS_MSG);
    }
    fReaderCn=TRUE;
}


static UINT32 SetValue(uint8_t DecimalNumber)
{
    
    char charTmp;
    UINT32 idTmp=0,i=0;
    
    while(1){

    charTmp = superResponseLoopEx();

    if(charTmp>='0'&&charTmp<='9')
    {
        idTmp=idTmp*10;
        idTmp+=(charTmp-'0');
        i++;
        terninalPrintf("%c",charTmp);
    }
    if(charTmp == 0x0D)
    {
        if(TouchPadResponseFlag)
        {
            TouchPadResponseFlag = FALSE;
            terninalPrintf("%d",ReceieveTouchPadVal);
            return ReceieveTouchPadVal;
        }
        else
            return idTmp;
        break;
    }
    //if((charTmp=='y') || (charTmp=='q'))
    if(charTmp=='q')
    {
        TouchPadResponseFlag = FALSE;
        return 0xffff;
        break;
    }
    if(i==DecimalNumber)
    {
        return idTmp;
        break;
    }
    }
    terninalPrintf("\r\n");
    
    
}

//static void SetOctopusTime(void)
static BOOL SetOctopusTime(void)
{
    uint16_t idTmpYear,idTmpMonth,idTmpDay,idTmpHour,idTmpMinute,idTmpSecond,idTmpWeekday;
    RTC_TIME_DATA_T id;
    GuiManagerCleanMessage(GUI_SETRTC_YEAR);
    terninalPrintf("Please enter year value in decimal.(ex:2020)\r\n");
    terninalPrintf("Enter number is ");
    //idTmpYear = SetValue(4);
    id.u32Year = SetValue(4);
    if(id.u32Year == 0xFFFF)
        return FALSE;
    GuiManagerCleanMessage(GUI_SETRTC_MONTH);
    terninalPrintf("\r\nPlease enter Month value in decimal.(01~12)\r\n");
    terninalPrintf("Enter number is ");
    do
    {
        //idTmpMonth = SetValue(2);
        id.u32cMonth = SetValue(2);
        if(id.u32cMonth == 0xFFFF)
            return FALSE;
        //if ((idTmpMonth == 0) || (idTmpMonth > 12))
        if ((id.u32cMonth == 0) || (id.u32cMonth > 12))
            terninalPrintf("Input error \r\nRe-enter number is ");
        else
            break;
        
    }while(1);
    GuiManagerCleanMessage(GUI_SETRTC_DAY);
    terninalPrintf("\r\nPlease enter Day value in decimal.(01~31)\r\n");
    terninalPrintf("Enter number is ");
    do
    {
        //idTmpDay = SetValue(2);
        id.u32cDay = SetValue(2);
        if(id.u32cDay == 0xFFFF)
            return FALSE;
        //if ((idTmpDay == 0) || (idTmpDay > 31))
        if ((id.u32cDay == 0) || (id.u32cDay > 31))
            terninalPrintf("Input error \r\nRe-enter number is ");
        else
            break;
        
    }while(1);
    GuiManagerCleanMessage(GUI_SETRTC_HOUR);
    terninalPrintf("\r\nPlease enter Hour value in decimal.(0~23)\r\n");
    terninalPrintf("Enter number is ");
    do
    {
        //idTmpHour = SetValue(2);
        id.u32cHour = SetValue(2);
        if(id.u32cHour == 0xFFFF)
            return FALSE;
        //if (idTmpHour > 23)
        if (id.u32cHour > 23)
            terninalPrintf("Input error \r\nRe-enter number is ");
        else
            break;
        
    }while(1);
    GuiManagerCleanMessage(GUI_SETRTC_MINUTE);
    terninalPrintf("\r\nPlease enter Minute value in decimal.(0~59)\r\n");
    terninalPrintf("Enter number is ");
    do
    {
        //idTmpMinute = SetValue(2);
        id.u32cMinute = SetValue(2);
       if(id.u32cMinute == 0xFFFF)
            return FALSE;
        //if (idTmpMinute > 59)
        if (id.u32cMinute > 59)
            terninalPrintf("Input error \r\nRe-enter number is ");
        else
            break;
        
    }while(1);
    /*
    terninalPrintf("\r\nPlease enter Second value in decimal.(0~59)\r\n");
    terninalPrintf("Enter number is ");
    do
    {
        //idTmpSecond = SetValue(2);
        id.u32cSecond = SetValue(2);
        
        
        //if (idTmpSecond > 59)
        if (id.u32cSecond > 59)
            terninalPrintf("Input error \r\nRe-enter number is ");
        else
            break;
        
    }while(1);
    */
    GuiManagerCleanMessage(GUI_SETRTC_WEEKDAY);
    terninalPrintf("\r\nPlease enter Weekday value in decimal.(0:Sunday,1:Monday,...,6:Saturday)\r\n");
    terninalPrintf("Enter number is ");
    do
    {
        //idTmpWeekday = SetValue(1);
        id.u32cDayOfWeek = SetValue(1);
        if(id.u32cDayOfWeek == 0xFFFF)
            return FALSE;
        //if (idTmpWeekday > 6)
        if (id.u32cDayOfWeek > 6)
            terninalPrintf("Input error \r\nRe-enter number is ");
        else
            break;
        
    }while(1);
    
    //terninalPrintf("Year/Month/Day  Hour:Minute:Second = %d/%d/%d  %d:%d:%d\r\n",pt.u32Year,pt.u32cMonth,pt.u32cDay,pt.u32cHour,pt.u32cMinute,pt.u32cSecond);
    

    
    //SetOSTime(idTmpYear,idTmpMonth,idTmpDay,idTmpHour,idTmpMinute,idTmpSecond,idTmpWeekday);
    //SetOSTime(id.u32Year,id.u32cMonth,id.u32cDay,id.u32cHour,id.u32cMinute,id.u32cSecond,id.u32cDayOfWeek);
    SetOSTime(id.u32Year,id.u32cMonth,id.u32cDay,id.u32cHour,id.u32cMinute,0,id.u32cDayOfWeek);
    //vTaskDelay(100/portTICK_RATE_MS);
    
    //terninalPrintf("\r\nModify time: ");
    //QueryOStime(id);
    
    GuiManagerCleanMessage(GUI_SETRTC_MODIFY);
    
    return TRUE;
}

static BOOL readerTest(void* para1, void* para2)
{
    int waitCounter = 10;
    int i=0;
    char tempchar;
    RTC_TIME_DATA_T ot,pt;
    time_t time, time2, time3;
    if(MBtestFlag )
    {
        
        BOOL resultFlag1 = FALSE;       //Reader Interconnect Result Flag
        BOOL resultFlag2 = FALSE;       //SAM test Result Flag
        BOOL resultFlag3 = FALSE;       //Flash test Result Flag
        BOOL resultFlag4 = FALSE;       //Reader power control test Result Flag
        char tempchar;
        
        terninalPrintf("!!! readerTest !!!\r\n");
        guiPrintResult("Reader Testing");
        guiPrintResult("Reader Init");
        SetReaderInterconnectFlag(TRUE);
        if(CardReaderInit(TRUE) == FALSE)
        //if(CardReaderInit(FALSE) == FALSE)
        {
            terninalPrintf("CardReaderInit FAIL\r\n");
            return TEST_FALSE;
        }

       // CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
        
        if(ReaderInterconnectResult())            
            resultFlag1 = TRUE;
        else
            resultFlag1 = FALSE;
        
        
        if(smartCardTest(para1,para2) == TEST_SUCCESSFUL_LIGHT_OFF)
        {
            terninalPrintf("smartCardTest success.\r\n");
            resultFlag2 = TRUE;
        }
        else
        {
            terninalPrintf("smartCardTest error.\r\n");
            resultFlag2 = FALSE;
        }
        
        
        if(sFlashTest(para1,para2) == TEST_SUCCESSFUL_LIGHT_OFF)
        {
            terninalPrintf("FlashTest success.\r\n");
            resultFlag3 = TRUE;
        }
        else
        {
            terninalPrintf("FlashTest error.\r\n");
            resultFlag3 = FALSE;
        }
        
               
        outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<16)) | (0x0<<16));
        GPIO_OpenBit(GPIOI, BIT4, DIR_OUTPUT, NO_PULL_UP);
        GPIO_ClrBit(GPIOI, BIT4);
        
        outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<28)) | (0x0<<28));
        GPIO_OpenBit(GPIOI, BIT15, DIR_OUTPUT, NO_PULL_UP);
        GPIO_ClrBit(GPIOI, BIT15);
        
        outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<4)) | (0x0<<4));
        GPIO_OpenBit(GPIOG, BIT9, DIR_OUTPUT, NO_PULL_UP);
        GPIO_ClrBit(GPIOG, BIT9);
        
        terninalPrintf("Close Reader power.\r\n");
        vTaskDelay(2000/portTICK_RATE_MS);
        terninalPrintf("Open ReaderBoardSpareIO1\r\n");
        GPIO_SetBit(GPIOI, BIT4);
        vTaskDelay(2000/portTICK_RATE_MS);
        
        terninalPrintf("Open ReaderBoardSpareIO1\r\n");        
        GPIO_SetBit(GPIOI, BIT15);
        vTaskDelay(2000/portTICK_RATE_MS);
        
        terninalPrintf("Open VD33Ctr\r\n");        
        GPIO_SetBit(GPIOG, BIT9);


        terninalPrintf("Does all LED lightup?(y/n)\n");
        while(1)
        {
            tempchar = userResponseLoop();
            if(tempchar =='y')
            {
                resultFlag4 = TRUE;
                break;
            }                
            else if(tempchar =='n')
            {
                resultFlag4 = FALSE;
                break;
            }                
        }
        
        
        if( resultFlag1 & resultFlag2 & resultFlag3 & resultFlag4 )
            return TEST_SUCCESSFUL_LIGHT_OFF;
        else
            return TEST_FALSE;
        
        
        
        /*
        if(ReaderInterconnectResult())            
            return TEST_SUCCESSFUL_LIGHT_OFF;
        else
            return TEST_FALSE;
        */
    }
    else
    {
        
    
        if(!GPIO_ReadBit(GPIOJ, BIT4))
        {
            
            terninalPrintf("!!! readerTest !!!\r\n");
            guiPrintResult("Reader Testing");
            GuiManagerCleanMessage(GUI_OCTOPUS_SELECTTYPE_EN);
            terninalPrintf("Please Select card type 3:test card  4:formal card\r\n");
            vTaskDelay(100/portTICK_RATE_MS);
            guiPrintMessage("Please Select\ncard type\n+:test   card\n=:formal card");
            vTaskDelay(100/portTICK_RATE_MS);
            while(1)
            {
                tempchar = userResponseLoop();
                //if(tempchar =='1')
                if(tempchar =='3')
                {
                    terninalPrintf("3\r\n");
                    GuiManagerCleanMessage(GUI_OCTOPUS_SELECTTYPE_DE);
                    ChangeOctopusKey(OCTOPUS_USE_TEST_KEY);
                    break;
                }
                //else if(tempchar =='2')
                else if(tempchar =='4')
                {
                    terninalPrintf("4\r\n");
                    GuiManagerCleanMessage(GUI_OCTOPUS_SELECTTYPE_DE);
                    ChangeOctopusKey(OCTOPUS_USE_PRODUCTION_KEY);
                    break;
                    //return TEST_FALSE;
                }
            }
            
            

            if(CardReaderInit(FALSE) == FALSE)
            {
                terninalPrintf("CardReaderInit FAIL\r\n");
            }
            else
            {
                terninalPrintf("CardReaderInit success\r\n");
            }
            CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
            
            RTC_Read(RTC_CURRENT_TIME, &ot);
            //time = GetCurrentUTCTime();
            time = RTC2Time(&ot);
            SetOSTime(2021,1,13,15,0,0,3);
            time2 = GetCurrentUTCTime();
            guiPrintResult("Reader Init");
            vTaskDelay(100/portTICK_RATE_MS);
            while(CardReaderGetBootedStatus() != TSREADER_CHECK_READER_OK)
            {
                terninalPrintf(".");
                switch(i)
                {
                case 3:
                    i=0;
                    guiPrintMessage(" ");
                    break;
                case 0:
                    guiPrintMessage(".");
                    break;
                case 1:
                    guiPrintMessage("..");
                    break;
                case 2:
                    guiPrintMessage("...");
                    break;
                }
                i++;
                vTaskDelay(1000/portTICK_RATE_MS);
                waitCounter--;
                if(waitCounter == 0)
                    break;
            }
            if(waitCounter == 0)
            {
                testErroCode = READER_CONNECT_ERRO;
                //terninalPrintf("\r\n  readerTest [ERROR]\r\n");
                //EPDDrawString(TRUE,"FAIL                ",X_POS_RST,Y_POS_RST);
                
                time3 = GetCurrentUTCTime();
                time += time3 - time2;
                Time2RTC(time, &ot);
                SetOSTime(ot.u32Year,ot.u32cMonth,ot.u32cDay,ot.u32cHour,ot.u32cMinute,0,ot.u32cDayOfWeek);
                
                return TEST_FALSE;
            }
            cleanMsg();
            

            
            
            //guiPrintMessage("Please Tag Octopus Card\n");
            guiPrintMessage("Please Tag\nor key = skip");
            terninalPrintf("\nPlease tag Octopus Card\n");
            
            InitCardReaderHangUpStatus();
            
            fReaderCn = FALSE;
            int tempstatus;
            tempstatus = CardReaderGetBootedStatus();
            //while(CardReaderGetBootedStatus() == TSREADER_CHECK_READER_OK)
            while(tempstatus == TSREADER_CHECK_READER_OK)
            {
                tempstatus = CardReaderGetBootedStatus();
                CardReaderProcess(1, octopusDepositResultCallback);
                if(fReaderCn)
                    break;
                tempchar = userResponse();
                if(tempchar =='n')
                {
                    CardReaderSetPower(EPM_READER_CTRL_ID_GUI,FALSE);
                    
                    time3 = GetCurrentUTCTime();
                    time += time3 - time2;
                    Time2RTC(time, &ot);
                    SetOSTime(ot.u32Year,ot.u32cMonth,ot.u32cDay,ot.u32cHour,ot.u32cMinute,0,ot.u32cDayOfWeek);
                    
                    return TEST_FALSE;
                }
            }

            //terninalPrintf("CardReaderGetBootedStatus() = %d \n",tempstatus);
            CardReaderSetPower(EPM_READER_CTRL_ID_GUI,FALSE);
            
            time3 = GetCurrentUTCTime();
            time += time3 - time2;
            Time2RTC(time, &ot);
            SetOSTime(ot.u32Year,ot.u32cMonth,ot.u32cDay,ot.u32cHour,ot.u32cMinute,0,ot.u32cDayOfWeek);
            
            if(tempstatus == TSREADER_CHECK_READER_ERROR)
                return TEST_FALSE;
            else
                return TEST_SUCCESSFUL_LIGHT_OFF;
            
            
        }
        else
        {
        
       
            if(CardReaderInit(TRUE) == FALSE)
            //if(CardReaderInit(FALSE) == FALSE)
            {
                terninalPrintf("CardReaderInit FAIL\r\n");
            }
            else
            {
                terninalPrintf("CardReaderInit success\r\n");
            }
            CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
            terninalPrintf("!!! readerTest !!!\r\n");
            guiPrintResult("Reader Testing");
            guiPrintResult("Reader Init");
            while(CardReaderGetBootedStatus() != TSREADER_CHECK_READER_OK)
            {
                terninalPrintf(".");
                switch(i)
                {
                case 3:
                    i=0;
                    guiPrintMessage(" ");
                    break;
                case 0:
                    guiPrintMessage(".");
                    break;
                case 1:
                    guiPrintMessage("..");
                    break;
                case 2:
                    guiPrintMessage("...");
                    break;
                }
                i++;
                vTaskDelay(1000/portTICK_RATE_MS);
                waitCounter--;
                if(waitCounter == 0)
                    break;
            }
            if(waitCounter == 0)
            {
                testErroCode = READER_CONNECT_ERRO;
                //terninalPrintf("\r\n  readerTest [ERROR]\r\n");
                //EPDDrawString(TRUE,"FAIL                ",X_POS_RST,Y_POS_RST);
                return TEST_FALSE;
            }
            cleanMsg();
    #if(SUPPORT_HK_10_HW)
    #else
            guiPrintMessage("Please Tag Card\nor key = skip");
            terninalPrintf("\nPlease tag card\n");
            while(CardReaderGetBootedStatus() == TSREADER_CHECK_READER_OK)
            {
                CardReaderProcessCN(readerCNCallback);
                if(fReaderCn)
                    break;
                tempchar = userResponseLoop();
                if(tempchar =='n')
                {
                    return TEST_FALSE;
                }
            }
    #endif
            //CardReaderSetPower(EPM_READER_CTRL_ID_GUI,FALSE);
            

            return TEST_SUCCESSFUL_LIGHT_OFF;
        }
    }
        
}

static BOOL ModemAndReaderTest(void* para1, void* para2)
{
    char SendModemStr[] = "Hello Modem";
    char SendReaderStr[] = "Hi Reader";
    char ModemReadBuf[20];
    char ReaderReadBuf[20];
    UartInterface* ModemUartInterface;
    UartInterface* ReaderUartInterface;
    ModemUartInterface  = UartGetInterface(UART_4_INTERFACE_INDEX);
    ReaderUartInterface = UartGetInterface(UART_2_INTERFACE_INDEX);
    ModemUartInterface->initFunc(115200);
    ReaderUartInterface->initFunc(115200);
    //ModemUartInterface->setPowerFunc(TRUE);
    
    uartIoctl(4, 25, 0, 0);  //UART1FlushBuffer , UARTA=4 , UART_IOC_FLUSH_RX_BUFFER=25 
    uartIoctl(2, 25, 0, 0);  //UART1FlushBuffer , UARTA=2 , UART_IOC_FLUSH_RX_BUFFER=25 
    
    int index = 0;
    int counter = 0;
    INT32 reVal;

    // Debug send message to CAD
    //pUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    ReaderUartInterface->writeFunc((PUINT8)SendModemStr,sizeof(SendModemStr));
    memset(ModemReadBuf, 0x0, sizeof(ModemReadBuf));
    while(counter < 30)
    {
        /*
        reVal = ReaderUartInterface->readFunc((PUINT8)ReaderReadBuf + index, sizeof(SendModemStr)-index);
        vTaskDelay(10/portTICK_RATE_MS);
        if(reVal > 0)
        {
            index = index + reVal;
            
            terninalPrintf("ReaderReceive<=");
            for(int i=0;i<index;i++)
                terninalPrintf("%02x ",ReaderReadBuf[i]);
            terninalPrintf("\r\n");
            
            for(int k=0;k<index;k++)
            {
                if((ReaderReadBuf[k] >= 0x20) && (ReaderReadBuf[k] <= 0x7E))
                    terninalPrintf("%c",ReaderReadBuf[k]);
            } 
            terninalPrintf("\r\n"); 
            
        }
        */

        reVal = ModemUartInterface->readFunc((PUINT8)ModemReadBuf + index, sizeof(SendModemStr)-index);
        vTaskDelay(10/portTICK_RATE_MS);
        if(reVal > 0)
        {
            index = index + reVal;
            /*
            terninalPrintf("ModemReceive<=");
            for(int i=0;i<index;i++)
                terninalPrintf("%02x ",ModemReadBuf[i]);
            terninalPrintf("\r\n");
            
            for(int k=0;k<index;k++)
            {
                if((ModemReadBuf[k] >= 0x20) && (ModemReadBuf[k] <= 0x7E))
                    terninalPrintf("%c",ModemReadBuf[k]);
            } 
            terninalPrintf("\r\n"); 
            */
        }
   
        counter++;
    }
    
    
    index = 0;
    counter = 0;

    //ModemUartInterface->ioctlFunc(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
    ModemUartInterface->writeFunc((PUINT8)SendReaderStr,sizeof(SendReaderStr));
    memset(ReaderReadBuf, 0x0, sizeof(ReaderReadBuf));
    while(counter < 30)
    {
        /*
        reVal = ModemUartInterface->readFunc((PUINT8)ModemReadBuf + index, sizeof(SendReaderStr)-index);
        vTaskDelay(10/portTICK_RATE_MS);
        if(reVal > 0)
        {
            index = index + reVal;
            
            terninalPrintf("ModemReceive<=");
            for(int i=0;i<index;i++)
                terninalPrintf("%02x ",ModemReadBuf[i]);
            terninalPrintf("\r\n");
            
            for(int k=0;k<index;k++)
            {
                if((ModemReadBuf[k] >= 0x20) && (ModemReadBuf[k] <= 0x7E))
                    terninalPrintf("%c",ModemReadBuf[k]);
            } 
            terninalPrintf("\r\n"); 
            
        }
        */
        
        reVal = ReaderUartInterface->readFunc((PUINT8)ReaderReadBuf + index, sizeof(SendReaderStr)-index);
        vTaskDelay(10/portTICK_RATE_MS);
        if(reVal > 0)
        {
            index = index + reVal;
            /*
            terninalPrintf("ReaderReceive<=");
            for(int i=0;i<index;i++)
                terninalPrintf("%02x ",ReaderReadBuf[i]);
            terninalPrintf("\r\n");
            
            for(int k=0;k<index;k++)
            {
                if((ReaderReadBuf[k] >= 0x20) && (ReaderReadBuf[k] <= 0x7E))
                    terninalPrintf("%c",ReaderReadBuf[k]);
            } 
            terninalPrintf("\r\n"); 
            */
        }
        
        counter++;
    }
    
    
    if( memcmp(ReaderReadBuf,SendReaderStr,sizeof(SendReaderStr)) == 0 )
    {
        terninalPrintf("Modem Tx & Reader Rx connect success.\r\n");
        IOtestResultFlag[2] = TRUE;
        MTPString[27][7] = 0x81;
    }
    else
    {
        terninalPrintf("Modem Tx & Reader Rx connect error.\r\n");
        IOtestResultFlag[2] = FALSE;
        MTPString[27][7] = 0x80;
    }
    
    if( memcmp(ModemReadBuf,SendModemStr,sizeof(SendModemStr)) == 0 )
    {
        terninalPrintf("Modem Rx & Reader Tx connect success.\r\n");
        IOtestResultFlag[3] = TRUE;
        MTPString[27][8] = 0x81;
    }
    else
    {
        terninalPrintf("Modem Rx & Reader Tx connect error.\r\n");
        IOtestResultFlag[3] = FALSE;
        MTPString[27][8] = 0x80;
    }
    

    
    
    
    BOOL ReaderUR2VDPwCTLowModemRTSInFlag  = FALSE;
    BOOL ReaderUR2VDPwCTHighModemRTSInFlag = FALSE;
    BOOL ReaderUR2VDPwCTInModemRTSLowFlag  = FALSE;
    BOOL ReaderUR2VDPwCTInModemRTSHighFlag = FALSE;

    GPIO_CloseBit(GPIOF, BIT10);
    GPIO_CloseBit(GPIOH, BIT10);
   //Set ReaderUR2VDPwCT(PF10) and ModemRTS(PH10) GPIO
    //Set ReaderUR2VDPwCT(PF10) output
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOF, BIT10, DIR_OUTPUT, PULL_UP);
    //Set ModemRTS(PH10) input
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOH, BIT10, DIR_INPUT, PULL_UP);
    
    //Set ReaderUR2VDPwCT(PF10) LOW
    GPIO_ClrBit(GPIOF, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOH, BIT10))
        ReaderUR2VDPwCTLowModemRTSInFlag  = TRUE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTLowModemRTSInFlag success\r\n");
    else
        ReaderUR2VDPwCTLowModemRTSInFlag  = FALSE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTLowModemRTSInFlag error.\r\n");
    
    //Set ReaderUR2VDPwCT(PF10) HIGH
    GPIO_SetBit(GPIOF, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOH, BIT10))
        ReaderUR2VDPwCTHighModemRTSInFlag = TRUE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTHighModemRTSInFlag success\r\n");
    else
        ReaderUR2VDPwCTHighModemRTSInFlag = FALSE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTHighModemRTSInFlag error.\r\n");
    //GPIO_ClrBit(GPIOF, BIT14);
    GPIO_CloseBit(GPIOF, BIT10);
    GPIO_CloseBit(GPIOH, BIT10);
    
    
    
    //Set ReaderUR2VDPwCT(PF10) input
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOF, BIT10, DIR_INPUT, PULL_UP);
    //Set ModemRTS(PH10) output
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOH, BIT10, DIR_OUTPUT, PULL_UP);
        
    //Set ModemRTS(PH10) LOW
    GPIO_ClrBit(GPIOH, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOF, BIT10))
        ReaderUR2VDPwCTInModemRTSLowFlag  = TRUE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTInModemRTSLowFlag success\r\n");
    else
        ReaderUR2VDPwCTInModemRTSLowFlag  = FALSE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTInModemRTSLowFlag error.\r\n");
    
    
    //Set ModemRTS(PH10) HIGH
    GPIO_SetBit(GPIOH, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOF, BIT10))
        ReaderUR2VDPwCTInModemRTSHighFlag = TRUE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTInModemRTSHighFlag success\r\n");
    else
        ReaderUR2VDPwCTInModemRTSHighFlag = FALSE;
        //terninalPrintf("Modem RTS & Reader UR2VDPwCT connect ReaderUR2VDPwCTInModemRTSHighFlag error.\r\n");
    //GPIO_ClrBit(GPIOH, BIT10);
    GPIO_CloseBit(GPIOF, BIT10);
    GPIO_CloseBit(GPIOH, BIT10);

    
    if(ReaderUR2VDPwCTLowModemRTSInFlag && ReaderUR2VDPwCTHighModemRTSInFlag &&
       ReaderUR2VDPwCTInModemRTSLowFlag && ReaderUR2VDPwCTInModemRTSHighFlag)
    {
        terninalPrintf("Modem RTS & Reader UR2VDPwCT connect success\r\n");
        IOtestResultFlag[4] = TRUE;
        MTPString[27][9] = 0x81;
    }
    else
    {
        terninalPrintf("Modem RTS & Reader UR2VDPwCT connect error.\r\n");
        IOtestResultFlag[4] = FALSE;
        MTPString[27][9] = 0x80;
    }
    
    
    return TEST_SUCCESSFUL_LIGHT_OFF;
}

INT32 EINT4Callback(UINT32 status, UINT32 userData)
{
    
    //terninalPrintf("\r\nMODEM GPIOH4 interrupt.\r\n"); 
    ModemGPIOH4INTFlag = TRUE;
    GPIO_ClrISRBit(GPIOH, BIT4);
    return 0;
}

static BOOL otherIOTest(void* para1, void* para2)
{
    BOOL rightResultFlag[sizeof(IOtestResultFlag)];
    
    
    memset(rightResultFlag,TRUE,sizeof(rightResultFlag));
    
    memset(IOtestResultFlag,FALSE,sizeof(IOtestResultFlag));
    
    sFlashTest(para1, para2);
    
    smartCardTest(para1, para2);
    
    //CADSingleTest(para1, para2);
    
    ModemAndReaderTest(para1, para2);
    
    //radarSingleTest(para1, para2);
    
    //-------------Modem INT & RST connect----------------------------
    //Set ReaderIO1(PG8) output
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(GPIOG, BIT8, DIR_OUTPUT, PULL_UP); 
    GPIO_ClrBit(GPIOG, BIT8);
    //userResponseLoop();
    
    ModemGPIOH4INTFlag = FALSE;
    // Set PH4 to EINT4 
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<16)) | (0xF<<16));    
    // Configure PH4 to input mode 
    GPIO_OpenBit(GPIOH, BIT4, DIR_INPUT, NO_PULL_UP);
    // Confingure PH4 to both-edge trigger 
    GPIO_ClrISRBit(GPIOH, BIT4);
    GPIO_EnableTriggerType(GPIOH, BIT4, RISING);
    //EINT5
    GPIO_EnableEINT(NIRQ4, (GPIO_CALLBACK)EINT4Callback, 0); 
    vTaskDelay(100/portTICK_RATE_MS);
    GPIO_SetBit(GPIOG, BIT8);
    //userResponseLoop();
    vTaskDelay(100/portTICK_RATE_MS);
    GPIO_DisableEINT(NIRQ4);
    
    if(ModemGPIOH4INTFlag)
    {
        terninalPrintf("Modem INT & RST connect success\r\n");
        IOtestResultFlag[9] = TRUE;
        MTPString[27][10] = 0x81;
    }
    else
    {
        terninalPrintf("Modem INT & RST connect error.\r\n");
        IOtestResultFlag[9] = FALSE;
        MTPString[27][10] = 0x80;
        return FALSE;
    }
    //---------------------------end------------------------------
    
    
    //-------------Reader IO1 & Reader RTS connect----------------------------
    
    
    BOOL ReaderRTSLowReaderIO1InFlag  = FALSE;
    BOOL ReaderRTSHighReaderIO1InFlag = FALSE;
    BOOL ReaderRTSInReaderIO1LowFlag  = FALSE;
    BOOL ReaderRTSInReaderIO1HighFlag = FALSE;

    
    //Set ReaderRTS(PF13) and ReaderIO1(PI4) GPIO
    //Set ReaderRTS(PF13) output
    GPIO_CloseBit(GPIOF, BIT13);
    GPIO_CloseBit(GPIOI, BIT4);
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0<<20));
    GPIO_OpenBit(GPIOF, BIT13, DIR_OUTPUT, PULL_UP);
    //Set ReaderIO1(PI4) input
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<16)) | (0<<16));
    GPIO_OpenBit(GPIOI, BIT4, DIR_INPUT, PULL_UP);
    
    //Set ReaderRTS(PF13) LOW
    GPIO_ClrBit(GPIOF, BIT13);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOI, BIT4))
        ReaderRTSLowReaderIO1InFlag  = TRUE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSLowReaderIO1InFlag success\r\n");
    else
        ReaderRTSLowReaderIO1InFlag  = FALSE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSLowReaderIO1InFlag error.\r\n");
    
    //Set ReaderRTS(PF13) HIGH
    GPIO_SetBit(GPIOF, BIT13);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOI, BIT4))
        ReaderRTSHighReaderIO1InFlag = TRUE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSHighReaderIO1InFlag success\r\n");
    else
        ReaderRTSHighReaderIO1InFlag = FALSE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSHighReaderIO1InFlag error.\r\n");
    GPIO_CloseBit(GPIOF, BIT13);
    GPIO_CloseBit(GPIOI, BIT4);
    
    
    
    //Set ReaderRTS(PF13) input
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0<<20));
    GPIO_OpenBit(GPIOF, BIT13, DIR_INPUT, PULL_UP);
    //Set ReaderIO1(PI4) output
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<16)) | (0<<16));
    GPIO_OpenBit(GPIOI, BIT4, DIR_OUTPUT, PULL_UP);
        
    //Set ReaderIO1(PI4) LOW
    GPIO_ClrBit(GPIOI, BIT4);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOF, BIT13))
        ReaderRTSInReaderIO1LowFlag  = TRUE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSInReaderIO1LowFlag success\r\n");
    else
        ReaderRTSInReaderIO1LowFlag  = FALSE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSInReaderIO1LowFlag error.\r\n");
    
    
    //Set ReaderIO1(PI4) HIGH
    GPIO_SetBit(GPIOI, BIT4);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOF, BIT13))
        ReaderRTSInReaderIO1HighFlag = TRUE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSInReaderIO1HighFlag success\r\n");
    else
        ReaderRTSInReaderIO1HighFlag = FALSE;
        //terninalPrintf("Reader IO1 & Reader RTS connect ReaderRTSInReaderIO1HighFlag error.\r\n");
    GPIO_CloseBit(GPIOF, BIT13);
    GPIO_CloseBit(GPIOI, BIT4);

    
    if(ReaderRTSLowReaderIO1InFlag && ReaderRTSHighReaderIO1InFlag &&
       ReaderRTSInReaderIO1LowFlag && ReaderRTSInReaderIO1HighFlag)
    {
        terninalPrintf("Reader IO1 & Reader RTS connect success\r\n");
        IOtestResultFlag[10] = TRUE;
        MTPString[27][11] = 0x81;
    }
    else
    {
        terninalPrintf("Reader IO1 & Reader RTS connect error.\r\n");
        IOtestResultFlag[10] = FALSE;
        MTPString[27][11] = 0x80;
        return FALSE;
    }
    
    
    
    //---------------------------end------------------------------
    
    
    
    //-------------Reader VD33PwCT & Reader IO2 connect----------------------------
    
    
    BOOL ReaderIO2LowReaderVD33PwCTInFlag  = FALSE;
    BOOL ReaderIO2HighReaderVD33PwCTInFlag = FALSE;
    BOOL ReaderIO2InReaderVD33PwCTLowFlag  = FALSE;
    BOOL ReaderIO2InReaderVD33PwCTHighFlag = FALSE;

    
    GPIO_CloseBit(GPIOI, BIT15);
    GPIO_CloseBit(GPIOG, BIT9);
    //Set ReaderIO2(PI15) and ReaderVD33PwCT(PG9) GPIO
    //Set ReaderIO2(PI15) output
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<28)) | (0<<28));
    GPIO_OpenBit(GPIOI, BIT15, DIR_OUTPUT, PULL_UP);
    //Set ReaderVD33PwCT(PG9) input
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<< 4)) | (0<< 4));
    GPIO_OpenBit(GPIOG, BIT9, DIR_INPUT, PULL_UP);
    
    //Set ReaderIO2(PI15) LOW
    GPIO_ClrBit(GPIOI, BIT15);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOG, BIT9))
        ReaderIO2LowReaderVD33PwCTInFlag  = TRUE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2LowReaderVD33PwCTInFlag success\r\n");
    else
        ReaderIO2LowReaderVD33PwCTInFlag  = FALSE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2LowReaderVD33PwCTInFlag error.\r\n");
    
    //Set ReaderIO2(PI15) HIGH
    GPIO_SetBit(GPIOI, BIT15);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOG, BIT9))
        ReaderIO2HighReaderVD33PwCTInFlag = TRUE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2HighReaderVD33PwCTInFlag success\r\n");
    else
        ReaderIO2HighReaderVD33PwCTInFlag = FALSE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2HighReaderVD33PwCTInFlag error.\r\n");
    GPIO_CloseBit(GPIOI, BIT15);
    GPIO_CloseBit(GPIOG, BIT9);
    
    
    
    //Set ReaderIO2(PI15) input
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<28)) | (0<<28));
    GPIO_OpenBit(GPIOI, BIT15, DIR_INPUT, PULL_UP);
    //Set ReaderVD33PwCT(PG9) output
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<< 4)) | (0<< 4));
    GPIO_OpenBit(GPIOG, BIT9, DIR_OUTPUT, PULL_UP);
        
    //Set ReaderVD33PwCT(PG9) LOW
    GPIO_ClrBit(GPIOG, BIT9);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOI, BIT15))
        ReaderIO2InReaderVD33PwCTLowFlag  = TRUE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2InReaderVD33PwCTLowFlag success\r\n");
    else
        ReaderIO2InReaderVD33PwCTLowFlag  = FALSE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2InReaderVD33PwCTLowFlag error.\r\n");
    
    
    //Set ReaderVD33PwCT(PG9) HIGH
    GPIO_SetBit(GPIOG, BIT9);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOI, BIT15))
        ReaderIO2InReaderVD33PwCTHighFlag = TRUE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2InReaderVD33PwCTHighFlag success\r\n");
    else
        ReaderIO2InReaderVD33PwCTHighFlag = FALSE;
        //terninalPrintf("Reader VD33PwCT & Reader IO2 connect ReaderIO2InReaderVD33PwCTHighFlag error.\r\n");
    GPIO_CloseBit(GPIOI, BIT15);
    GPIO_CloseBit(GPIOG, BIT9);

    
    if(ReaderIO2LowReaderVD33PwCTInFlag && ReaderIO2HighReaderVD33PwCTInFlag &&
       ReaderIO2InReaderVD33PwCTLowFlag && ReaderIO2InReaderVD33PwCTHighFlag)
    {
        terninalPrintf("Reader VD33PwCT & Reader IO2 connect success\r\n");
        IOtestResultFlag[11] = TRUE;
        MTPString[27][12] = 0x81;
    }
    else
    {
        terninalPrintf("Reader VD33PwCT & Reader IO2 connect error.\r\n");
        IOtestResultFlag[11] = FALSE;
        MTPString[27][12] = 0x80;
        return FALSE;
    }
    
    
    
    //---------------------------end------------------------------
    
    
    //-------------Sensor SnrSwVD33 & Reader CTS connect----------------------------
    
    BOOL ReaderCTSLowSensorSnrSwVD33InFlag  = FALSE;
    BOOL ReaderCTSHighSensorSnrSwVD33InFlag = FALSE;
    BOOL ReaderCTSInSensorSnrSwVD33LowFlag  = FALSE;
    BOOL ReaderCTSInSensorSnrSwVD33HighFlag = FALSE;

    /*
    GPIO_CloseBit(GPIOF, BIT14);
    GPIO_CloseBit(GPIOB, BIT6);
   //Set ReaderCTS(PF14) and SensorSnrSwVD33(PB6) GPIO
    //Set ReaderCTS(PF14) output
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0<<24));
    GPIO_OpenBit(GPIOF, BIT14, DIR_OUTPUT, PULL_UP);
    //Set SensorSnrSwVD33(PB6) input
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<24)) | (0<<24));
    GPIO_OpenBit(GPIOB, BIT6, DIR_INPUT, PULL_UP);
    
    //Set ReaderCTS(PF14) LOW
    GPIO_ClrBit(GPIOF, BIT14);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT6))
        ReaderCTSLowSensorSnrSwVD33InFlag  = TRUE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSLowSensorSnrSwVD33InFlag success\r\n");
    else
        ReaderCTSLowSensorSnrSwVD33InFlag  = FALSE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSLowSensorSnrSwVD33InFlag error.\r\n");
    
    //Set ReaderCTS(PF14) HIGH
    GPIO_SetBit(GPIOF, BIT14);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT6))
        ReaderCTSHighSensorSnrSwVD33InFlag = TRUE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSHighSensorSnrSwVD33InFlag success\r\n");
    else
        ReaderCTSHighSensorSnrSwVD33InFlag = FALSE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSHighSensorSnrSwVD33InFlag error.\r\n");
    //GPIO_ClrBit(GPIOF, BIT14);
    */
    
    GPIO_CloseBit(GPIOF, BIT14);
    GPIO_CloseBit(GPIOB, BIT6);
    
    
    
    //Set ReaderCTS(PF14) input
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0<<24));
    GPIO_OpenBit(GPIOF, BIT14, DIR_INPUT, PULL_UP);
    //Set SensorSnrSwVD33(PB6) output
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<24)) | (0<<24));
    GPIO_OpenBit(GPIOB, BIT6, DIR_OUTPUT, PULL_UP);
        
    //Set SensorSnrSwVD33(PB6) LOW
    GPIO_ClrBit(GPIOB, BIT6);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    //if(!GPIO_ReadBit(GPIOF, BIT14))
    if(GPIO_ReadBit(GPIOF, BIT14))
    {
        ReaderCTSInSensorSnrSwVD33LowFlag  = TRUE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSInSensorSnrSwVD33LowFlag success\r\n");
    }
    else
    {
        ReaderCTSInSensorSnrSwVD33LowFlag  = FALSE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSInSensorSnrSwVD33LowFlag error.\r\n");
    }
    
    
    //Set SensorSnrSwVD33(PB6) HIGH
    GPIO_SetBit(GPIOB, BIT6);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    //if(GPIO_ReadBit(GPIOF, BIT14))
    if(!GPIO_ReadBit(GPIOF, BIT14))
    {
        ReaderCTSInSensorSnrSwVD33HighFlag = TRUE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSInSensorSnrSwVD33HighFlag success\r\n");
    }
    else
    {
        ReaderCTSInSensorSnrSwVD33HighFlag = FALSE;
        //terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect ReaderCTSInSensorSnrSwVD33HighFlag error.\r\n");
    }
    //GPIO_ClrBit(GPIOH, BIT10);
    GPIO_CloseBit(GPIOF, BIT14);
    GPIO_CloseBit(GPIOB, BIT6);

    //Sensor board DC5V GPB6
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOB, BIT6, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOB, BIT6);  
    
    vTaskDelay(500/portTICK_RATE_MS);
    if(//ReaderCTSLowSensorSnrSwVD33InFlag && ReaderCTSHighSensorSnrSwVD33InFlag &&
       ReaderCTSInSensorSnrSwVD33LowFlag && ReaderCTSInSensorSnrSwVD33HighFlag)
    {
        terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect success\r\n");
        IOtestResultFlag[12] = TRUE;
        MTPString[27][13] = 0x81;
    }
    else
    {
        terninalPrintf("Sensor SnrSwVD33 & Reader CTS connect error.\r\n");
        IOtestResultFlag[12] = FALSE;
        MTPString[27][13] = 0x80;
        return FALSE;
    }
    
    //---------------------------end------------------------------

    
    //-----------------------LED D5 test--------------------------
    
    BOOL LEDtestFlag = FALSE;
    char tempchar;
    LEDColorBuffSet(0x10, 0x00);
    LEDBoardLightSet();
    SetMTPCRC(28,(uint8_t*)MTPString);
    MTPCmdprint(28,(uint8_t*)MTPString);
    terninalPrintf("Is the D5 light signal correct?(y/n)\r\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            terninalPrintf("Reader SysPwr test success.\r\n");
            LEDtestFlag = TRUE;
            MTPString[27][14] = 0x81;
            break;
        }
        else if(tempchar =='n')
        {
            terninalPrintf("Reader SysPwr test error.\r\n");
            LEDtestFlag = FALSE;
            MTPString[27][14] = 0x80;
            return FALSE;
            break;
        }
    }
    LEDColorBuffSet(0x00, 0x00);
    LEDBoardLightSet();

    //---------------------------end------------------------------
    
    
    
    if( IOtestResultFlag[2]  &&     //  Modem Tx & Reader Rx connect test result
        IOtestResultFlag[3]  &&     //  Modem Rx & Reader Tx connect test result
        IOtestResultFlag[4]  &&     //  Modem RTS & Reader UR2VDPwCT connect test result
        IOtestResultFlag[9]  &&     //  Modem INT & RST connect test result
        IOtestResultFlag[10] &&     //  Reader IO1 & Reader RTS connect test result
        IOtestResultFlag[11] &&     //  Reader VD33PwCT & Reader IO2 connect test result
        IOtestResultFlag[12] &&     //  Sensor SnrSwVD33 & Reader CTS connect test result
        IOtestResultFlag[13] &&     //  Flash1 test result
        IOtestResultFlag[14] &&     //  Flash2 test result
        IOtestResultFlag[15] &&     //  Flash3 test result
        IOtestResultFlag[16] &&     //  smartCard test result 
        LEDtestFlag             )   //  SysPwr test result
    {
        MTPString[27][15] = 0x81;
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
    else
    {
        MTPString[27][15] = 0x80;
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE; 
    }
    
    
    //-------------Sensor UR3PwrCT & Reader UR33PwrCT connect----------------------------
    /*
    
    BOOL ReaderUR33PwrCTLowSensorUR3PwrCTInFlag  = FALSE;
    BOOL ReaderUR33PwrCTHighSensorUR3PwrCTInFlag = FALSE;
    BOOL ReaderUR33PwrCTInSensorUR3PwrCTLowFlag  = FALSE;
    BOOL ReaderUR33PwrCTInSensorUR3PwrCTHighFlag = FALSE;

    
    //Set ReaderUR33PwrCT(PG9) and SensorUR3PwrCT(PE11) GPIO
    //Set ReaderUR33PwrCT(PG9) output
    GPIO_CloseBit(GPIOG, BIT9);
    GPIO_CloseBit(GPIOE, BIT11);
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<4)) | (0<<4));
    GPIO_OpenBit(GPIOG, BIT9, DIR_OUTPUT, PULL_UP);
    //Set SensorUR3PwrCT(PE11) input
    outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<< 12)) | (0<< 12));
    GPIO_OpenBit(GPIOE, BIT11, DIR_INPUT, PULL_UP);
    
    //Set ReaderUR33PwrCT(PG9) LOW
    GPIO_ClrBit(GPIOG, BIT9);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOE, BIT11))
        ReaderUR33PwrCTLowSensorUR3PwrCTInFlag  = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTLowSensorUR3PwrCTInFlag success\r\n");
    else
        ReaderUR33PwrCTLowSensorUR3PwrCTInFlag  = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTLowSensorUR3PwrCTInFlag error.\r\n");
    
    //Set ReaderUR33PwrCT(PG9) HIGH
    GPIO_SetBit(GPIOG, BIT9);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOE, BIT11))
        ReaderUR33PwrCTHighSensorUR3PwrCTInFlag = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTHighSensorUR3PwrCTInFlag success\r\n");
    else
        ReaderUR33PwrCTHighSensorUR3PwrCTInFlag = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTHighSensorUR3PwrCTInFlag error.\r\n");
    GPIO_CloseBit(GPIOE, BIT11);
    GPIO_CloseBit(GPIOG, BIT9);
    
    
    
    //Set ReaderUR33PwrCT(PG9) input
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<4)) | (0<<4));
    GPIO_OpenBit(GPIOG, BIT9, DIR_INPUT, PULL_UP);
    //Set SensorUR3PwrCT(PE11) output
    outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<< 12)) | (0<< 12));
    GPIO_OpenBit(GPIOE, BIT11, DIR_OUTPUT, PULL_UP);
        
    //Set SensorUR3PwrCT(PE11) LOW
    GPIO_ClrBit(GPIOE, BIT11);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOG, BIT9))
        ReaderUR33PwrCTInSensorUR3PwrCTLowFlag  = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTInSensorUR3PwrCTLowFlag success\r\n");
    else
        ReaderUR33PwrCTInSensorUR3PwrCTLowFlag  = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTInSensorUR3PwrCTLowFlag error.\r\n");
    
    
    //Set SensorUR3PwrCT(PE11) HIGH
    GPIO_SetBit(GPIOE, BIT11);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOG, BIT9))
        ReaderUR33PwrCTInSensorUR3PwrCTHighFlag = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTInSensorUR3PwrCTHighFlag success\r\n");
    else
        ReaderUR33PwrCTInSensorUR3PwrCTHighFlag = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect ReaderUR33PwrCTInSensorUR3PwrCTHighFlag error.\r\n");
    GPIO_CloseBit(GPIOE, BIT11);
    GPIO_CloseBit(GPIOG, BIT9);

    
    if(ReaderUR33PwrCTLowSensorUR3PwrCTInFlag && ReaderUR33PwrCTHighSensorUR3PwrCTInFlag &&
       ReaderUR33PwrCTInSensorUR3PwrCTLowFlag && ReaderUR33PwrCTInSensorUR3PwrCTHighFlag)
    {
        terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect success\r\n");
        IOtestResultFlag[11] = TRUE;
    }
    else
    {
        terninalPrintf("Sensor UR3PwrCT & Reader UR33PwrCT connect error.\r\n");
        IOtestResultFlag[11] = FALSE;
    }
    
    
    */
    //---------------------------end------------------------------
    
    
    //-------------Sensor UR7PwrCT & Reader RTS connect----------------------------
    /*
    
    BOOL ReaderRTSLowSensorUR7PwrCTInFlag  = FALSE;
    BOOL ReaderRTSHighSensorUR7PwrCTInFlag = FALSE;
    BOOL ReaderRTSInSensorUR7PwrCTLowFlag  = FALSE;
    BOOL ReaderRTSInSensorUR7PwrCTHighFlag = FALSE;

    
    //Set ReaderRTS(PF13) and SensorUR7PwrCT(PB10) GPIO
    //Set ReaderRTS(PF13) output
    GPIO_CloseBit(GPIOF, BIT13);
    GPIO_CloseBit(GPIOB, BIT10);
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0<<20));
    GPIO_OpenBit(GPIOF, BIT13, DIR_OUTPUT, PULL_UP);
    //Set SensorUR7PwrCT(PB10) input
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOB, BIT10, DIR_INPUT, PULL_UP);
    
    //Set ReaderRTS(PF13) LOW
    GPIO_ClrBit(GPIOF, BIT13);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT10))
        ReaderRTSLowSensorUR7PwrCTInFlag  = TRUE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSLowSensorUR7PwrCTInFlag success\r\n");
    else
        ReaderRTSLowSensorUR7PwrCTInFlag  = FALSE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSLowSensorUR7PwrCTInFlag error.\r\n");
    
    //Set ReaderRTS(PF13) HIGH
    GPIO_SetBit(GPIOF, BIT13);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT10))
        ReaderRTSHighSensorUR7PwrCTInFlag = TRUE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSHighSensorUR7PwrCTInFlag success\r\n");
    else
        ReaderRTSHighSensorUR7PwrCTInFlag = FALSE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSHighSensorUR7PwrCTInFlag error.\r\n");
    GPIO_CloseBit(GPIOF, BIT13);
    GPIO_CloseBit(GPIOB, BIT10);
    
    
    
    //Set ReaderRTS(PF13) input
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0<<20));
    GPIO_OpenBit(GPIOF, BIT13, DIR_INPUT, PULL_UP);
    //Set SensorUR7PwrCT(PB10) output
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOB, BIT10, DIR_OUTPUT, PULL_UP);
        
    //Set SensorUR7PwrCT(PB10) LOW
    GPIO_ClrBit(GPIOB, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOF, BIT13))
        ReaderRTSInSensorUR7PwrCTLowFlag  = TRUE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSInSensorUR7PwrCTLowFlag success\r\n");
    else
        ReaderRTSInSensorUR7PwrCTLowFlag  = FALSE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSInSensorUR7PwrCTLowFlag error.\r\n");
    
    
    //Set SensorUR7PwrCT(PB10) HIGH
    GPIO_SetBit(GPIOB, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOF, BIT13))
        ReaderRTSInSensorUR7PwrCTHighFlag = TRUE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSInSensorUR7PwrCTHighFlag success\r\n");
    else
        ReaderRTSInSensorUR7PwrCTHighFlag = FALSE;
        //terninalPrintf("Sensor UR7PwrCT & Reader RTS connect ReaderRTSInSensorUR7PwrCTHighFlag error.\r\n");
    GPIO_CloseBit(GPIOF, BIT13);
    GPIO_CloseBit(GPIOB, BIT10);

    
    if(ReaderRTSLowSensorUR7PwrCTInFlag && ReaderRTSHighSensorUR7PwrCTInFlag &&
       ReaderRTSInSensorUR7PwrCTLowFlag && ReaderRTSInSensorUR7PwrCTHighFlag)
    {
        terninalPrintf("Sensor UR7PwrCT & Reader RTS connect success\r\n");
        IOtestResultFlag[12] = TRUE;
    }
    else
    {
        terninalPrintf("Sensor UR7PwrCT & Reader RTS connect error.\r\n");
        IOtestResultFlag[12] = FALSE;
    }
    
    
    */
    //---------------------------end------------------------------
    /*
    if(memcmp(IOtestResultFlag,rightResultFlag,sizeof(IOtestResultFlag)) == 0 )    
        return TEST_SUCCESSFUL_LIGHT_OFF;
    else
        return TEST_FALSE;
    */
}
//////////////////////EPD    test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
/*
static BOOL epdTest(void* para1, void* para2)
{
    if(!isEPDInit){
        terninalPrintf("EPD No Connection [ERROR]\n");
        return TEST_FALSE;
    }
    EPDDrawString(TRUE,"EPD Testing   ",X_POS_RST,Y_POS_RST);
    EPDSetBacklight(FALSE);
    vTaskDelay(500/portTICK_RATE_MS);
    EPDSetBacklight(TRUE);

    EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_BLACK_INDEX,0,0);
    vTaskDelay(1500/portTICK_RATE_MS);
    EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    vTaskDelay(1500/portTICK_RATE_MS);
    guiManagerRefreshScreen();
    vTaskDelay(7000/portTICK_RATE_MS);
    EPDDrawString(TRUE,"EPD Testing   ",X_POS_RST,Y_POS_RST);
    EPDDrawString(TRUE,"Is Screen\nAll Black Then\nAll White\n\n+:Yes  =:No",X_POS_MSG,Y_POS_MSG);
    terninalPrintf("Does EPD Screen turn to all black then turn to all white?(y/n)\n");
    if(userResponseLoop()=='y'){
        EPDDrawString(TRUE,"PASS            ",X_POS_RST,Y_POS_RST);
        cleanMsg();
        //terninalPrintf("EPD Test [OK]\r\n");
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
    EPDDrawString(TRUE,"ERROR              ",X_POS_RST,Y_POS_RST);
    cleanMsg();
    //terninalPrintf("EPD Test [ERROR]\r\n");
    return TEST_FALSE;
}
*/

//////////////////////Modem  test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL modeminit(BOOL testflag)
{
    return QModemLibInit(MODEM_BAUDRATE);
}




static BOOL modemTest(void* para1, void* para2)
{
 
    QModemLibInit(MODEM_BAUDRATE);
    if(MBtestFlag )
    {     
        BOOL resultFlag1 = FALSE;      // MODEM Dialup test resultFlag
        BOOL resultFlag2 = FALSE;      // MODEM interrupt test resultFlag
        BOOL resultFlag3 = FALSE;      // MODEM RTS test resultFlag
        char tempchar;
        
        terninalPrintf("MODEM Dialup test,please wait about 1 minute.\r\n");
        //GPIO_SetBit(GPIOG, BIT7);
        //LedSetColor(modemColorAllRed, 0x04, TRUE);
        
        vTaskDelay(1000/portTICK_RATE_MS);
        //Reset pin GPG8
        outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xFu<<0)) | (0x0u<<0));
        GPIO_OpenBit(GPIOG, BIT8, DIR_OUTPUT, NO_PULL_UP);
        GPIO_ClrBit(GPIOG, BIT8);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_SetBit(GPIOG, BIT8);
        
        QModemDialupStart();
        if(QModemDialupProcess() == TRUE)
        {
            terninalPrintf("\r\nMODEM Dialup SUCCESS\r\n");
            LedSetBayLightFlush(0,8);
            LedSetAliveStatusLightFlush(0,8);
            
            
            resultFlag1 = TRUE;
            //return RTCTest(para1,para2);
        }
        else
        {
            //terninalPrintf("\r\n");
            terninalPrintf("\r\nMODEM Dialup ERROR\r\n");
            LedSetAliveStatusLightFlush(0,8);
            resultFlag1 = FALSE;
            //RTCTest(para1,para2);
            //return TEST_FALSE;
        } 
        
        
            QModemTotalStop();
            vTaskDelay(1000/portTICK_RATE_MS);

            QModemLibInit(921600);
            
        ModemGPIOH4INTFlag = FALSE;
        // Set PH4 to EINT4 
        outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<16)) | (0xF<<16));    
        // Configure PH4 to input mode 
        GPIO_OpenBit(GPIOH, BIT4, DIR_INPUT, NO_PULL_UP);
        // Confingure PH4 to both-edge trigger 
        GPIO_ClrISRBit(GPIOH, BIT4);
        GPIO_EnableTriggerType(GPIOH, BIT4, FALLING);
        //EINT5
        GPIO_EnableEINT(NIRQ4, (GPIO_CALLBACK)EINT4Callback, 0); 
        
        
            //terninalPrintf("Check MODEM AT Cmd again,please wait about 5 seconds.\n");
            terninalPrintf("Check MODEM AT Cmd,please wait about 10 seconds.\n");
            vTaskDelay(1000/portTICK_RATE_MS);
            if(!QModemATCmdTest())
            {   
                terninalPrintf("MODEM send ATCmd [ERROR]\r\n");
                return TEST_FALSE;
            }
            terninalPrintf("MODEM send sleep Cmd,please wait 10 seconds.\n");
            //QModemIoctl(UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);

            //vTaskDelay(30000/portTICK_RATE_MS);
            vTaskDelay(10000/portTICK_RATE_MS);
    
    
        
        int sentSleepCmdTimes = 3;
        while(sentSleepCmdTimes > 0)
        {
            if(QModemSetSleep())
                break;
            sentSleepCmdTimes--;

        }
        if(sentSleepCmdTimes <= 0)
        {
            terninalPrintf("MODEM send SleepCmd [ERROR]\r\n");
            resultFlag2 = FALSE;
            //return TEST_FALSE;
        }

        //ModemGPIOH4INTFlag = FALSE;
        QModemSetQurccfg();
        /*
        // Set PH4 to EINT4 
        outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<16)) | (0xF<<16));    
        // Configure PH4 to input mode 
        GPIO_OpenBit(GPIOH, BIT4, DIR_INPUT, NO_PULL_UP);
        // Confingure PH4 to both-edge trigger 
        GPIO_ClrISRBit(GPIOH, BIT4);
        GPIO_EnableTriggerType(GPIOH, BIT4, FALLING);
        //EINT5
        GPIO_EnableEINT(NIRQ4, (GPIO_CALLBACK)EINT4Callback, 0);    
        */
        
        //ModemGPIOH4INTFlag = FALSE;
        //vTaskDelay(100/portTICK_RATE_MS);
        //QModemSetTestCmd();
        vTaskDelay(1000/portTICK_RATE_MS);
        if(!ModemGPIOH4INTFlag)
        {
            terninalPrintf("MODEM interrupt test error.\n");
            GPIO_DisableEINT(NIRQ4);
            QModemTotalStop();
            resultFlag2 = FALSE;
            //return TEST_FALSE;
        }
        else
        {
            terninalPrintf("MODEM interrupt test success.\n");

            resultFlag2 = TRUE;
        }
        terninalPrintf("Please check RTS signal.\n");
        QModemIoctl(UART_IOC_SETRTSSIGNAL, UART_RTS_LOW, 0);
        
        
        terninalPrintf("MODEM RTS signal OK?(y/n)\n");

        while(1)
        {
            tempchar = userResponseLoop();
            QModemIoctl(UART_IOC_SETRTSSIGNAL, UART_RTS_HIGH, 0);
            GPIO_DisableEINT(NIRQ4);
            QModemTotalStop();
            if(tempchar =='y')
            {
                resultFlag3 = TRUE;
                break;
            }
            else if(tempchar =='n')
            {
                resultFlag3 = FALSE;
                break;
            }
        }
        ModemResultLEDFlag = TRUE;

        if(resultFlag1 & resultFlag2 & resultFlag3)
        {
            return TEST_SUCCESSFUL_LIGHT_OFF;
        }
        else
        {
            return TEST_FALSE;
        }
        /*
        terninalPrintf("MODEM Dialup test,please wait about 1 minute.\r\n");
        QModemDialupStart();
        if(QModemDialupProcess() == TRUE)
        {
            terninalPrintf("\r\nMODEM Dialup SUCCESS\r\n");
            return RTCTest(para1,para2);
        }
        else
        {
            terninalPrintf("\r\n");
            RTCTest(para1,para2);
            return TEST_FALSE;
        } 

        */
    }
    else
    {    
        terninalPrintf("!!! modemTest !!!\r\n");
        guiPrintResult("Modem Testing");
        //guiPrintMessage("Start...");
        guiPrintMessage("Start");
        //QModemLibInit(MODEM_BAUDRATE);
        if(QModemATCmdTest())
        {
            //guiPrintMessage((char*)retVer);
            guiPrintMessage("Modem OK");
            terninalPrintf("  modemTest [OK]\r\n");
            return RTCTest(para1,para2);
        }
        testErroCode = MODEM_CONNECT_ERRO;
        //terninalPrintf("  modemTest [ERROR]\r\n");
        guiPrintMessage("Modem Fail");
        terninalPrintf("  modemTest [Error]\r\n");
        RTCTest(para1,para2);
        return TEST_FALSE; 
    }

    
    /*guiPrintMessage("Get Version");
    char retVer[100];
    if(QModemGetVer(retVer))
    {   
        //sprintf(retVer,"%s\n%s","Version",retVer);
        terninalPrintf("=>%s  %p",retVer,retVer);
        guiPrintMessage(retVer);
    }
    else
    {
        guiPrintMessage("Modem Fail\nATCmd Error");
        return TEST_FALSE;
    }
    return TEST_SUCCESSFUL_LIGHT_OFF; */
    
}

//////////////////////RTC   test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL RTCTest(void* para1, void* para2)
{
    terninalPrintf("!!! rtcTest !!!\r\n");
    if(MBtestFlag )
    {}
    else
    {
        guiPrintResult("RTC Testing");
        //guiPrintMessage("Wait 2s");
        //EPDDrawString(TRUE,"Wait 2s",X_POS_RST,Y_POS_RST+200);
        EPDDrawString(TRUE,"Wait 2s",X_POS_RST,Y_POS_RST+250);
    }
    RTC_TIME_DATA_T pt;
    time_t time, time2;
    //terninalPrintf("!!! RTCTest !!!\r\n");

    if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
    {
        char* tString;
        //sprintf(tString,"CURRENT TIME:\n%04d-%02d-%02d\n%02d %02d %02d",
        //    pt.u32Year, pt.u32cMonth, pt.u32cDay, 
         //   pt.u32cHour, pt.u32cMinute, pt.u32cSecond);  
        terninalPrintf("RTC_CURRENT_TIME: [%04d/%02d/%02d %02d:%02d:%02d (%d)  u8cClockDisplay = %d, u8cAmPm =%d]\r\n",
            pt.u32Year, pt.u32cMonth, pt.u32cDay, 
            pt.u32cHour, pt.u32cMinute, pt.u32cSecond, pt.u32cDayOfWeek, pt.u8cClockDisplay, pt.u8cAmPm); 
        //guiPrintMessage(tString);
    }
    vTaskDelay(2000/portTICK_RATE_MS);//wait other task finsh
    time = GetCurrentUTCTime();
    terninalPrintf(" RTCTest Get time: %d, Please Wait...\r\n", time);
    vTaskDelay(2000/portTICK_RATE_MS );
    time2 = GetCurrentUTCTime();
    terninalPrintf(" RTCTest Get time again: %d\r\n", time2);

    if((time2 - time) != 2)
    {
        testErroCode = RTC_TIME_ERRO;
        //terninalPrintf(" RTCTest [ERROR]\r\n");
        //EPDDrawString(TRUE,"FAIL                ",X_POS_RST,Y_POS_RST);
        //guiPrintMessage("RTC Fail");
        //EPDDrawString(TRUE,"RTC   Fail   ",X_POS_RST,Y_POS_RST+200);
        if(MBtestFlag )
        {
            MTPString[31][3] = 0x80;
        }
        else
            EPDDrawString(TRUE,"RTC   Fail   ",X_POS_RST,Y_POS_RST+250);
        terninalPrintf("  RTCTest [ERROR]\r\n");
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    //terninalPrintf(" RTCTest [PASS]\r\n");
    //EPDDrawString(TRUE,"RTC   OK     ",X_POS_RST,Y_POS_RST+200);
    if(MBtestFlag )
    {
        MTPString[31][3] = 0x81;
    }
    else
        EPDDrawString(TRUE,"RTC   OK     ",X_POS_RST,Y_POS_RST+250);
    terninalPrintf("  RTCTest [OK]\r\n");
    return TEST_SUCCESSFUL_LIGHT_OFF;
}

//////////////////////sFlash test/////////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL sFlashTest(void* para1, void* para2)
{    
    uint16_t uID;
    terninalPrintf("!!! flashTest !!!\r\n");
    if(MBtestFlag )
    {}
    else
        guiPrintResult("Flash Testing");
    FlashDrvExInit(FALSE);
    FlashDrvExInitialize(SPI_FLASH_EX_0_INDEX);
    FlashDrvExInitialize(SPI_FLASH_EX_1_INDEX);
    FlashDrvExInitialize(SPI_FLASH_EX_2_INDEX);

    
    terninalPrintf("Query Chip 1\r\n");
    uID = FlashDrvExGetChipID(SPI_FLASH_EX_0_INDEX);
    if(uID == 0xEF16){
        terninalPrintf(" Chip 1: 0x%04x [OK] \r\n", uID);
        
        if(MBtestFlag )
        {}
        else
        {
            //vTaskDelay(15/portTICK_RATE_MS);
            guiPrintMessage("Chip 1 OK");
        }
        
        IOtestResultFlag[13] = TRUE;
        MTPString[27][3] = 0x81;
    }
    else
    {
        testErroCode = FLASH1_SPACE_ERRO;
        terninalPrintf(" Chip 1: 0x%04x [ERROR] \r\n", uID);
        if(MBtestFlag )
        {}
        else
        {
            //vTaskDelay(15/portTICK_RATE_MS);
            guiPrintMessage("Chip 1 FAIL ");
        }
        IOtestResultFlag[13] = FALSE;
        MTPString[27][3] = 0x80;
        //return TEST_FALSE;
    }
    terninalPrintf("Query Chip 2\r\n");
    uID = FlashDrvExGetChipID(SPI_FLASH_EX_1_INDEX);
    if(uID == 0xEF16){
        terninalPrintf(" Chip 2: 0x%04x [OK] \r\n", uID);
        //guiPrintMessage("Chip 2 OK");
        
        if(MBtestFlag )
        {}
        else
        {
            vTaskDelay(15/portTICK_RATE_MS);
            EPDDrawString(TRUE,"Chip 2 OK    ",X_POS_RST,Y_POS_RST+250);
        }
        IOtestResultFlag[14] = TRUE;
        MTPString[27][4] = 0x81;
    }
    else
    {
        testErroCode = FLASH2_SPACE_ERRO;
        terninalPrintf(" Chip 2: 0x%04x [ERROR] \r\n", uID);
        //guiPrintMessage("\nChip 2 FAIL ");
        if(MBtestFlag )
        {}
        else
        {
            vTaskDelay(15/portTICK_RATE_MS);
            EPDDrawString(TRUE,"Chip 2 FAIL  ",X_POS_RST,Y_POS_RST+250);
        }
        IOtestResultFlag[14] = FALSE;
        MTPString[27][4] = 0x80;
        //return TEST_FALSE;
    }
    
    
    
    
    terninalPrintf("Query Chip 3\r\n");
    uID = FlashDrvExGetChipID(SPI_FLASH_EX_2_INDEX);
    if(uID == 0xEF16){
        terninalPrintf(" Chip 3: 0x%04x [OK] \r\n", uID);
        //guiPrintMessage("Chip 2 OK");
        
        if(MBtestFlag )
        {}
        else
        {
            vTaskDelay(15/portTICK_RATE_MS);
            EPDDrawString(TRUE,"Chip 3 OK    ",X_POS_RST,Y_POS_RST+300);
        }
        IOtestResultFlag[15] = TRUE;
        MTPString[27][5] = 0x81;
    }
    else
    {
        testErroCode = FLASH2_SPACE_ERRO;
        terninalPrintf(" Chip 3: 0x%04x [ERROR] \r\n", uID);
        //guiPrintMessage("\nChip 2 FAIL ");
        if(MBtestFlag )
        {}
        else
        {
            vTaskDelay(15/portTICK_RATE_MS);
            EPDDrawString(TRUE,"Chip 3 FAIL  ",X_POS_RST,Y_POS_RST+300);
        }
        IOtestResultFlag[15] = FALSE;
        MTPString[27][5] = 0x80;
        return TEST_FALSE;
    }
    
    
    return TEST_SUCCESSFUL_LIGHT_OFF;
}

//////////////////////SmartCard test//////////////////////TEST_SUCCESSFUL_LIGHT_OFF
static BOOL smartCardTest(void* para1, void* para2)
{
    terninalPrintf("!!! smartCardTest !!!\r\n");
    if(MBtestFlag )
    {}
    else
        guiPrintResult("Smart Card\nTesting");
    if(SmartCardDrvInit(TRUE))
    {				
        //terninalPrintf("  smartCardTest [OK]\r\n");
        terninalPrintf("smartCardTest success.\r\n");
    }
    else
    {
        testErroCode = SMART_CARD_CONNECT_ERRO;
        //terninalPrintf("  smartCardTest [ERROR]\r\n");
        terninalPrintf("smartCardTest error.\r\n");
        IOtestResultFlag[16] = FALSE;
        MTPString[27][6] = 0x80;
        return TEST_FALSE;
    }
    if(MBtestFlag )
    {}
    else
        guiPrintMessage("OK");
    IOtestResultFlag[16] = TRUE;
    MTPString[27][6] = 0x81;
    return TEST_SUCCESSFUL_LIGHT_OFF;
}

//////////////////////Camera test/////////////////////////

static BOOL cameraTest(void* para1, void* para2)
{    
    static CameraInterface*     pCameraInterface = NULL;
    uint8_t reValueFailFlag = FALSE;
    terninalPrintf("!!! usbCamTest !!!\r\n");
    if(MBtestFlag )
    {}
    else
    {
        GPIO_ClrBit(GPIOB, BIT6);
        guiPrintResult("Camera");
    }
    setPrintfFlag(TRUE);    
    pCameraInterface = CameraGetInterface(CAMERA_UVC_INTERFACE_INDEX);
    if(pCameraInterface == NULL)
    {
        sysprintf("usbCamTest ERROR (pCameraInterface == NULL)!!\n");
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    if(pCameraInterface->initFunc(FALSE) == FALSE)
    {
        sysprintf("usbCamTest ERROR (pCameraInterface->initFunc(FALSE) == FALSE)!!\n");
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    
    //userResponseLoop();
    uint8_t* photoPr;
    int photoLen = 0;
    char targetFileNameTmp[_MAX_LFN];
    memset(targetFileNameTmp, 0x0, sizeof(targetFileNameTmp));
    sprintf(targetFileNameTmp,"uvcphoto.%s", PHOTO_FILE_EXTENSION);    
    //Cam0
    //guiPrintMessage("Cam0 Testing..");
    //terninalPrintf("  camera[0] -- Taking Photo\r\n");
    if(MBtestFlag )
    {}
    else
        guiPrintMessage("Cam1 Testing..");
    terninalPrintf("  camera[1] -- Taking Photo\r\n");
    //if(pCameraInterface->takePhotoFunc(0, &photoPr, &photoLen, FILE_AGENT_STORAGE_TYPE_FATFS, "0:", targetFileNameTmp, FALSE))
    //if(pCameraInterface->takePhotoFunc(0, &photoPr, &photoLen, FILE_AGENT_STORAGE_TYPE_FATFS, "0:", targetFileNameTmp, FALSE, 1, 0))
    if(pCameraInterface->takePhotoFunc(1, &photoPr, &photoLen, FILE_AGENT_STORAGE_TYPE_FATFS, "0:", targetFileNameTmp, FALSE, 1, 0))
    {
        if(MBtestFlag )
        {}
        else
            guiPrintMessage("Cam1 OK");
        terninalPrintf("  camera[1] (%s) [OK]\r\n",targetFileNameTmp);
        
        IOtestResultFlag[7] = TRUE;
    }
    else
    {
        if(MBtestFlag )
        {}
        else
            guiPrintMessage("Cam1 ERROR");
        //terninalPrintf("  cameraTest[1] [ERROR]\r\n");
        terninalPrintf("  camera[1] [ERROR]\r\n");
        reValueFailFlag = TRUE;
        
        IOtestResultFlag[7] = FALSE;
        if(MBtestFlag )
            return FALSE;
        //return TEST_FALSE;
    }
    //Cam1
    //guiPrintMessage("Cam1 Testing..");
    //EPDDrawString(TRUE,"Cam1 Testing..",X_POS_RST,Y_POS_RST+200);
    
    if(MBtestFlag )
    {
    }
    else
    {
        EPDDrawString(TRUE,"Cam2 Testing..",X_POS_RST,Y_POS_RST+250);
        terninalPrintf("  camera[2] -- Taking Photo\r\n");
        //if(pCameraInterface->takePhotoFunc(1, &photoPr, &photoLen, FILE_AGENT_STORAGE_TYPE_FATFS, "1:", targetFileNameTmp, FALSE, 1, 0))
        if(pCameraInterface->takePhotoFunc(0, &photoPr, &photoLen, FILE_AGENT_STORAGE_TYPE_FATFS, "1:", targetFileNameTmp, FALSE, 1, 0))
        {
            //guiPrintMessage("Cam1 OK");
            //EPDDrawString(TRUE,"Cam1 OK       ",X_POS_RST,Y_POS_RST+200);
            EPDDrawString(TRUE,"Cam2 OK       ",X_POS_RST,Y_POS_RST+250);
            //terninalPrintf("  cameraTest[2] (%s) [OK]\r\n",targetFileNameTmp);
            terninalPrintf("  camera[2] (%s) [OK]\r\n",targetFileNameTmp);
            
            IOtestResultFlag[8] = TRUE;
        }
        else
        {
            //guiPrintMessage("Cam1 ERROR");
            //EPDDrawString(TRUE,"Cam1 ERROR    ",X_POS_RST,Y_POS_RST+200);
            EPDDrawString(TRUE,"Cam2 ERROR    ",X_POS_RST,Y_POS_RST+250);
            //terninalPrintf("  cameraTest[2] [ERROR]\r\n");
            terninalPrintf("  camera[2] [ERROR]\r\n");
            
            IOtestResultFlag[8] = FALSE;
            
            return TEST_FALSE;
        }
    }
    vTaskDelay(2000/portTICK_RATE_MS);
    if(MBtestFlag )
    {}
    else                
        GPIO_SetBit(GPIOB, BIT6);
    if(reValueFailFlag == FALSE)    
        return TEST_SUCCESSFUL_LIGHT_OFF;
    else
    {
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
}

//////////////////////Radar test//////////////////////// single
static BOOL NewRadarSingleTest(void* para1, void* para2)
{
    static RadarInterface* pRadarInterface;
    
    int featureValue[2];
    uint8_t RadarData[22];
    
    //uint8_t VersionCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D};
    uint8_t ResultCmd[24] = {0x7A, 0xA7, 0x00, 0x18, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x1A, 0xD3, 0x3D};
    char NumStr[11][10];
    
    terninalPrintf("!!! New Radar Test !!!\r\n");
    guiPrintResult("Radar");
    pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);
    terninalPrintf("Radar Initial...");
    
    
    if(pRadarInterface == NULL)
    {
        terninalPrintf("radarTest ERROR (pRadarInterface == NULL)!!\n");
        return TEST_FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("radarTest ERROR (initFunc false)!!\n");
        return TEST_FALSE;
    }
    
    //pRadarInterface->setPowerStatusFunc(0,TRUE);
    //pRadarInterface->setPowerStatusFunc(1,TRUE);

    
    for(int i = 0; i<2; i++)
    {
        if(i == 0)
            guiPrintMessage("Radar1 Test...");
        else if(i == 1)
            EPDDrawString(TRUE,"Radar2 Test... ",X_POS_MSG,Y_POS_MSG);
        pRadarInterface->setPowerStatusFunc(i,TRUE);
        vTaskDelay(500/portTICK_RATE_MS);
        featureValue[i] = pRadarInterface->RadarResultFunc(i, 0x02,ResultCmd,RadarData);
        pRadarInterface->setPowerStatusFunc(i,FALSE);
        /*
        terninalPrintf("featureValue = %08x \r\n",featureValue);
        terninalPrintf("RadarData =              ");
        for(int j = 0; j<sizeof(RadarData); j++)
            terninalPrintf("%02x ",RadarData[j]);
        terninalPrintf("\r\n");
        */
        if(featureValue[i] == TRUE)
        {
            
            terninalPrintf("Radar%d OK\r\n",i+1);
            vTaskDelay(100/portTICK_RATE_MS);
            EPDDrawString(FALSE,"                ",X_POS_MSG,Y_POS_MSG);
            if(i == 0)
                EPDDrawString(TRUE,"Radar1 OK",X_POS_MSG,Y_POS_MSG+50);
            else if(i == 1)
                EPDDrawString(TRUE,"Radar2 OK",X_POS_MSG,Y_POS_MSG+200);

            /*
            terninalPrintf("Radar%d Object Distance (R): %d\r\n",i+1,RadarData[7]*10);           
            
            memset(NumStr,0x00,sizeof(NumStr));
            
            sprintf(NumStr[3],"%d",RadarData[7]*10);
            
 
            EPDDrawString(FALSE,"    ",15+7*25+515*i,100+(44*5));
            EPDDrawString(FALSE,"    ",15+15*25+515*i,100+(44*5));
            EPDDrawString(FALSE,"    ",15+7*25+515*i,100+(44*6));
            EPDDrawString(FALSE,"    ",15+15*25+515*i,100+(44*6));
            EPDDrawString(FALSE,"     ",15+8*25+515*i,100+(44*7));
            EPDDrawString(FALSE,"     ",15+8*25+515*i,100+(44*8));
            EPDDrawString(FALSE,"     ",15+8*25+515*i,100+(44*9));
            EPDDrawString(FALSE,"     ",15+8*25+515*i,100+(44*10));
            EPDDrawString(FALSE,"     ",15+8*25+515*i,100+(44*11));
            EPDDrawString(FALSE,"     ",25+12*25+515*i,100+(44*12));
            EPDDrawString(FALSE,"     ",25+10*25+515*i,100+(44*13));
                      
            
            EPDDrawString(FALSE,NumStr[0],15+7*25+515*i,100+(44*5));
            EPDDrawString(FALSE,NumStr[1],15+15*25+515*i,100+(44*5));
            EPDDrawString(FALSE,NumStr[2],15+7*25+515*i,100+(44*6));
            EPDDrawString(FALSE,NumStr[3],15+15*25+515*i,100+(44*6));
            EPDDrawString(FALSE,NumStr[4],15+8*25+515*i,100+(44*7));
            EPDDrawString(FALSE,NumStr[5],15+8*25+515*i,100+(44*8));
            EPDDrawString(FALSE,NumStr[6],15+8*25+515*i,100+(44*9));
            EPDDrawString(FALSE,NumStr[7],15+8*25+515*i,100+(44*10));
            EPDDrawString(FALSE,NumStr[8],15+8*25+515*i,100+(44*11));
            EPDDrawString(FALSE,NumStr[9],25+12*25+515*i,100+(44*12));
            EPDDrawString(TRUE,NumStr[10],25+10*25+515*i,100+(44*13));
            */
        }
        else
        {
            /*
            if(i == 0)
                guiPrintMessage("Radar1 FAIL");
            else if(i == 1)
                EPDDrawString(FALSE,"Radar2 FAIL",X_POS_MSG,Y_POS_MSG);
            */
            terninalPrintf("Radar%d FAIL\r\n",i+1);
            vTaskDelay(100/portTICK_RATE_MS);
            EPDDrawString(FALSE,"                ",X_POS_MSG,Y_POS_MSG);
            if(i == 0)
                EPDDrawString(TRUE,"Radar1 FAIL",X_POS_MSG,Y_POS_MSG+50);
            else if(i == 1)
                EPDDrawString(TRUE,"Radar2 FAIL",X_POS_MSG,Y_POS_MSG+200);                
            

        }
    }

    //pRadarInterface->setPowerStatusFunc(0,FALSE);
    //pRadarInterface->setPowerStatusFunc(1,FALSE);
    if((featureValue[0] == TRUE) && (featureValue[1] == TRUE) )
        return TEST_SUCCESSFUL_LIGHT_OFF;
    else
        return TEST_FALSE;
}


static BOOL radarSingleTest(void* para1, void* para2)
{
    static RadarInterface* pRadarInterface;
    int dist0;
    int dist1;
    char distStringBuff0[20];
    char distStringBuff1[20];
    uint8_t reValueFailFlag = FALSE;
    BOOL changeFlag;
    BOOL LidarfailFlag = 0;
    terninalPrintf("!!! radar\\lidar Test !!!\r\n");
    if(MBtestFlag )
    {}
    else
        guiPrintResult("Radar");
    pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);
    terninalPrintf("Radar Initial...");
    if(MBtestFlag )
    {}
    else
        guiPrintMessage("Radar Initial...");
    
    if(pRadarInterface == NULL)
    {
        terninalPrintf("radarTest ERROR (pRadarInterface == NULL)!!\n");
        if(MBtestFlag )
            return FALSE;
        else
        return TEST_FALSE;
    }
    
    if(pRadarInterface->initFunc() == FALSE)
    {
        //EPDDrawString(TRUE,"FAIL            \n                  ",X_POS_RST,Y_POS_RST);
        terninalPrintf("radarTest ERROR (initFunc false)!!\n");
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    
    if(MBtestFlag )
    { 
        pRadarInterface->setPowerStatusFunc(0,TRUE);
        pRadarInterface->setPowerStatusFunc(1,TRUE);
        //guiPrintMessage("Radar1 Test...");
        vTaskDelay(1000/portTICK_RATE_MS);
        if( pRadarInterface->checkFeaturnFunc(0, &changeFlag, NULL, NULL, NULL) != RADAR_FEATURE_IGNORE)
        {
            reValueFailFlag = TRUE;
            terninalPrintf("radar UR3TX & radar UR7RX connect error.\r\n");
            IOtestResultFlag[5] = FALSE;
            MTPString[29][4] = 0x80;
            return FALSE;
        }
        else
        {
            terninalPrintf("radar UR3TX & radar UR7RX connect success.\r\n");
            IOtestResultFlag[5] = TRUE;
            MTPString[29][4] = 0x81;
        //guiPrintMessage("Radar2 Test...");
        }
        if( pRadarInterface->checkFeaturnFunc(1, &changeFlag, NULL, NULL, NULL) != RADAR_FEATURE_IGNORE )
        {
            reValueFailFlag = TRUE;
            terninalPrintf("radar UR7TX & radar UR3RX connect error.\r\n");
            IOtestResultFlag[6] = FALSE;
            MTPString[29][5] = 0x80;
            return FALSE;
        }
        else
        {
            terninalPrintf("radar UR7TX & radar UR3RX connect success.\r\n");
            IOtestResultFlag[6] = TRUE;
            MTPString[29][5] = 0x81;
        }
        
        pRadarInterface->setPowerStatusFunc(0,FALSE);
        pRadarInterface->setPowerStatusFunc(1,FALSE);
        
        if(reValueFailFlag)
        {
            if(MBtestFlag )
                return FALSE;
            else
                return TEST_FALSE;
        }
        else
            return TEST_SUCCESSFUL_LIGHT_OFF;
        /*
        if(reValueFailFlag)
        {
            cameraTest(para1, para2);
            return TEST_FALSE;
        }
        else
            return cameraTest(para1, para2);
        */
        
        /*
        if(GPIO_ReadBit(GPIOJ, BIT4))
        {
        
            if(reValueFailFlag)
            {
                cameraTest(para1, para2);
                return TEST_FALSE;
            }
            else
                return cameraTest(para1, para2);
                //return TEST_SUCCESSFUL_LIGHT_OFF;
        }
        else
        {
            if(reValueFailFlag)
                return TEST_FALSE;
            else
                return TEST_SUCCESSFUL_LIGHT_OFF;
        }
        */
    }
    else
    {
        pRadarInterface->setPowerStatusFunc(0,TRUE);
        pRadarInterface->setPowerStatusFunc(1,TRUE);
        guiPrintMessage("Radar1 Test...");
        vTaskDelay(4000/portTICK_RATE_MS);
        int featureValue = pRadarInterface->checkFeaturnFunc(0, &changeFlag, NULL, NULL, NULL);
        if(featureValue==RADAR_FEATURE_OCCUPIED
            || featureValue==RADAR_FEATURE_VACUUM 
            || featureValue==RADAR_FEATURE_OCCUPIED_UN_STABLED
            || featureValue==RADAR_FEATURE_VACUUM_UN_STABLED
            || featureValue==RADAR_FEATURE_IGNORE
            || featureValue==RADAR_FEATURE_OCCUPIED_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_VACUUM_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_OCCUPIED_UN_STABLED_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_VACUUM_UN_STABLED_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_IGNORE_LIDAR_FAIL)
        {
            terninalPrintf("Radar1 OK\n");
            //guiPrintMessage("Radar0 OK");
            vTaskDelay(100/portTICK_RATE_MS);
            EPDDrawString(FALSE,"                ",X_POS_MSG,Y_POS_MSG);
            EPDDrawString(FALSE,"Radar1 OK",X_POS_MSG,Y_POS_MSG+50);
            if((featureValue & 0xf0) == 0x30)
            {
                EPDDrawString(TRUE," Lidar1 FAIL",X_POS_MSG,Y_POS_MSG+100);
                LidarfailFlag = 1;
            }
            else
            {
                EPDDrawString(TRUE," Lidar1 OK",X_POS_MSG,Y_POS_MSG+100);
            }
            switch(featureValue)
            {
            case RADAR_FEATURE_OCCUPIED:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_OCCUPIED!!\n" );
                break;
            case RADAR_FEATURE_VACUUM:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_VACUUM!!\n" );
                break;
            case RADAR_FEATURE_OCCUPIED_UN_STABLED:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_UN_STABLED!!\n" );
                break; 
            case RADAR_FEATURE_VACUUM_UN_STABLED:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_UN_STABLED!!\n" );
                break; 
            case RADAR_FEATURE_IGNORE:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_IGNORE!!\n" );
                break; 
            
            
            case RADAR_FEATURE_OCCUPIED_LIDAR_FAIL:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_OCCUPIED_LIDAR_FAIL!!\n" );
                break;
            case RADAR_FEATURE_VACUUM_LIDAR_FAIL:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_VACUUM_LIDAR_FAIL!!\n" );
                break;
            case RADAR_FEATURE_OCCUPIED_UN_STABLED_LIDAR_FAIL:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_UN_STABLED_LIDAR_FAIL!!\n" );
                break; 
            case RADAR_FEATURE_VACUUM_UN_STABLED_LIDAR_FAIL:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_UN_STABLED_LIDAR_FAIL!!\n" );
                break; 
            case RADAR_FEATURE_IGNORE_LIDAR_FAIL:
                terninalPrintf("[RADAR]1 ->  RADAR_FEATURE_IGNORE_LIDAR_FAIL!!\n" );
                break; 
            }
            vTaskDelay(100/portTICK_RATE_MS);
            if(pRadarInterface->readDistValueFunc(0,&dist0) == TRUE)
            {
                sprintf(distStringBuff0," Dist:%d",dist0);
                EPDDrawString(TRUE,distStringBuff0,X_POS_MSG,Y_POS_MSG+150);
            }
            else
            {
                EPDDrawString(TRUE," L1 not calib.",X_POS_MSG,Y_POS_MSG+150);  //"L1 not calibrated"
            }
        }
        else
        {
            guiPrintMessage("Radar1 TIMEOUT");
            terninalPrintf("[RADAR]1 ->  TIMEOUT!!\n"); 
            //terninalPrintf("RadarTest [ERROR]\n");
            EPDDrawString(FALSE,"Radar1 FAIL",X_POS_MSG,Y_POS_MSG+50);
            EPDDrawString(TRUE," Lidar1 FAIL",X_POS_MSG,Y_POS_MSG+100);
            reValueFailFlag = TRUE;
            //return TEST_FALSE;
        }
        //guiPrintMessage("Radar1 Test...");
        vTaskDelay(100/portTICK_RATE_MS);
        EPDDrawString(TRUE,"Radar2 Test... ",X_POS_MSG,Y_POS_MSG);
        featureValue = pRadarInterface->checkFeaturnFunc(1, &changeFlag, NULL, NULL, NULL);
        if(featureValue==RADAR_FEATURE_OCCUPIED 
            || featureValue==RADAR_FEATURE_VACUUM
            || featureValue==RADAR_FEATURE_OCCUPIED_UN_STABLED
            || featureValue==RADAR_FEATURE_VACUUM_UN_STABLED
            || featureValue==RADAR_FEATURE_IGNORE
            || featureValue==RADAR_FEATURE_OCCUPIED_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_VACUUM_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_OCCUPIED_UN_STABLED_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_VACUUM_UN_STABLED_LIDAR_FAIL
            || featureValue==RADAR_FEATURE_IGNORE_LIDAR_FAIL)
        {
            terninalPrintf("Radar2 OK\n");
            //guiPrintMessage("Radar1 OK");
            vTaskDelay(100/portTICK_RATE_MS);
            EPDDrawString(FALSE,"                ",X_POS_MSG,Y_POS_MSG);
            EPDDrawString(FALSE,"Radar2 OK",X_POS_MSG,Y_POS_MSG+200);
            if((featureValue & 0xf0) == 0x30)
            {
                EPDDrawString(TRUE," Lidar2 FAIL",X_POS_MSG,Y_POS_MSG+250);
                LidarfailFlag = 1;
            }
            else
            {
                EPDDrawString(TRUE," Lidar2 OK",X_POS_MSG,Y_POS_MSG+250);
            }
            switch(featureValue)
            {
            case RADAR_FEATURE_OCCUPIED:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_OCCUPIED!!\n" );
                break;
            case RADAR_FEATURE_VACUUM:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_VACUUM!!\n" );
                break;
            case RADAR_FEATURE_OCCUPIED_UN_STABLED:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_UN_STABLED!!\n" );
                break; 
            case RADAR_FEATURE_VACUUM_UN_STABLED:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_UN_STABLED!!\n" );
                break; 
            case RADAR_FEATURE_IGNORE:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_IGNORE!!\n" );
                break; 
            
            case RADAR_FEATURE_OCCUPIED_LIDAR_FAIL:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_OCCUPIED_LIDAR_FAIL!!\n" );
                break;
            case RADAR_FEATURE_VACUUM_LIDAR_FAIL:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_VACUUM_LIDAR_FAIL!!\n" );
                break;
            case RADAR_FEATURE_OCCUPIED_UN_STABLED_LIDAR_FAIL:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_UN_STABLED_LIDAR_FAIL!!\n" );
                break; 
            case RADAR_FEATURE_VACUUM_UN_STABLED_LIDAR_FAIL:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_UN_STABLED_LIDAR_FAIL!!\n" );
                break; 
            case RADAR_FEATURE_IGNORE_LIDAR_FAIL:
                terninalPrintf("[RADAR]2 ->  RADAR_FEATURE_IGNORE_LIDAR_FAIL!!\n" );
                break; 
            }
            vTaskDelay(100/portTICK_RATE_MS);
            if(pRadarInterface->readDistValueFunc(1,&dist1) == TRUE)
            {
                sprintf(distStringBuff1," Dist:%d",dist1);
                EPDDrawString(TRUE,distStringBuff1,X_POS_MSG,Y_POS_MSG+300);
            }
            else
            {
                EPDDrawString(TRUE," L2 not calib.",X_POS_MSG,Y_POS_MSG+300);  //"L2 not calibrated"
            }
        }
        else
        {
            //guiPrintMessage("Radar1 TIMEOUT");
            EPDDrawString(FALSE,"Radar2 TIMEOUT",X_POS_MSG,Y_POS_MSG);
            terninalPrintf("[RADAR]2 ->  TIMEOUT!!\n"); 
            //terninalPrintf("RadarTest [ERROR]\n");
            EPDDrawString(FALSE,"Radar2 FAIL",X_POS_MSG,Y_POS_MSG+200);
            EPDDrawString(TRUE," Lidar2 FAIL",X_POS_MSG,Y_POS_MSG+250);
            
            pRadarInterface->setPowerStatusFunc(0,FALSE);
            pRadarInterface->setPowerStatusFunc(1,FALSE);
            
            return TEST_FALSE;
        }
        
        if(LidarfailFlag || reValueFailFlag)
        {
            pRadarInterface->setPowerStatusFunc(0,FALSE);
            pRadarInterface->setPowerStatusFunc(1,FALSE);
            
            return TEST_FALSE;
        }
        
        pRadarInterface->setPowerStatusFunc(0,FALSE);
        pRadarInterface->setPowerStatusFunc(1,FALSE);
        
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
}


static BOOL sensorSingleTest(void* para1, void* para2)
{
    GPIO_SetBit(GPIOG, BIT7);
    if(cameraTest(para1, para2) == FALSE)
    {
        GPIO_ClrBit(GPIOG, BIT7);
        MTPString[29][3] = 0x80;
        return FALSE;
    }
    GPIO_ClrBit(GPIOG, BIT7);
    MTPString[29][3] = 0x81;
    
    if(radarSingleTest(para1, para2) == FALSE)
        return FALSE;
    
    //-------------Sensor UR3PwrCT & Sensor UR7PwrCT connect----------------------------
    
    
    BOOL SensorUR7PwrCTLowSensorUR3PwrCTInFlag  = FALSE;
    BOOL SensorUR7PwrCTHighSensorUR3PwrCTInFlag = FALSE;
    BOOL SensorUR7PwrCTInSensorUR3PwrCTLowFlag  = FALSE;
    BOOL SensorUR7PwrCTInSensorUR3PwrCTHighFlag = FALSE;

    
    //Set SensorUR7PwrCT(PB10) and SensorUR3PwrCT(PE11) GPIO
    //Set SensorUR7PwrCT(PB10) output
    GPIO_CloseBit(GPIOB, BIT10);
    GPIO_CloseBit(GPIOE, BIT11);
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOB, BIT10, DIR_OUTPUT, PULL_UP);
    //Set SensorUR3PwrCT(PE11) input
    outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<< 12)) | (0<< 12));
    GPIO_OpenBit(GPIOE, BIT11, DIR_INPUT, PULL_UP);
    
    //Set SensorUR7PwrCT(PB10) LOW
    GPIO_ClrBit(GPIOB, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOE, BIT11))
        SensorUR7PwrCTLowSensorUR3PwrCTInFlag  = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTLowSensorUR3PwrCTInFlag success\r\n");
    else
        SensorUR7PwrCTLowSensorUR3PwrCTInFlag  = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTLowSensorUR3PwrCTInFlag error.\r\n");
    
    //Set SensorUR7PwrCT(PB10) HIGH
    GPIO_SetBit(GPIOB, BIT10);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOE, BIT11))
        SensorUR7PwrCTHighSensorUR3PwrCTInFlag = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTHighSensorUR3PwrCTInFlag success\r\n");
    else
        SensorUR7PwrCTHighSensorUR3PwrCTInFlag = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTHighSensorUR3PwrCTInFlag error.\r\n");
    GPIO_CloseBit(GPIOE, BIT11);
    GPIO_CloseBit(GPIOB, BIT10);
    
    
    
    //Set SensorUR7PwrCT(PB10) input
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOB, BIT10, DIR_INPUT, PULL_UP);
    //Set SensorUR3PwrCT(PE11) output
    outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<< 12)) | (0<< 12));
    GPIO_OpenBit(GPIOE, BIT11, DIR_OUTPUT, PULL_UP);
        
    //Set SensorUR3PwrCT(PE11) LOW
    GPIO_ClrBit(GPIOE, BIT11);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT10))
        SensorUR7PwrCTInSensorUR3PwrCTLowFlag  = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTInSensorUR3PwrCTLowFlag success\r\n");
    else
        SensorUR7PwrCTInSensorUR3PwrCTLowFlag  = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTInSensorUR3PwrCTLowFlag error.\r\n");
    
    
    //Set SensorUR3PwrCT(PE11) HIGH
    GPIO_SetBit(GPIOE, BIT11);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT10))
        SensorUR7PwrCTInSensorUR3PwrCTHighFlag = TRUE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTInSensorUR3PwrCTHighFlag success\r\n");
    else
        SensorUR7PwrCTInSensorUR3PwrCTHighFlag = FALSE;
        //terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect SensorUR7PwrCTInSensorUR3PwrCTHighFlag error.\r\n");
    GPIO_CloseBit(GPIOE, BIT11);
    GPIO_CloseBit(GPIOB, BIT10);

    
    if(SensorUR7PwrCTLowSensorUR3PwrCTInFlag && SensorUR7PwrCTHighSensorUR3PwrCTInFlag &&
       SensorUR7PwrCTInSensorUR3PwrCTLowFlag && SensorUR7PwrCTInSensorUR3PwrCTHighFlag)
    {
        terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect success\r\n");
        IOtestResultFlag[17] = TRUE;
        MTPString[29][6] = 0x81;
    }
    else
    {
        terninalPrintf("Sensor UR3PwrCT & Sensor UR7PwrCT connect error.\r\n");
        IOtestResultFlag[17] = FALSE;
        MTPString[29][6] = 0x80;
        return FALSE;
    }
    
    
    
    //---------------------------end------------------------------
    
    
    //-------------Sensor USB1ExSel & Sensor UR7ExSel connect----------------------------
    
    BOOL SensorUR7ExSelLowSensorUSB1ExSelInFlag  = FALSE;
    BOOL SensorUR7ExSelHighSensorUSB1ExSelInFlag = FALSE;
    BOOL SensorUR7ExSelInSensorUSB1ExSelLowFlag  = FALSE;
    BOOL SensorUR7ExSelInSensorUSB1ExSelHighFlag = FALSE;

    
    GPIO_CloseBit(GPIOB, BIT1);
    GPIO_CloseBit(GPIOB, BIT2);
   //Set SensorUR7ExSel(PB1) and SensorUSB1ExSel(PB2) GPIO
    //Set SensorUR7ExSel(PB1) output
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<< 4)) | (0<< 4));
    GPIO_OpenBit(GPIOB, BIT1, DIR_OUTPUT, PULL_UP);
    //Set SensorUSB1ExSel(PB2) input
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOB, BIT2, DIR_INPUT, PULL_UP);
    
    //Set SensorUR7ExSel(PB1) LOW
    GPIO_ClrBit(GPIOB, BIT1);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT2))
    {
        SensorUR7ExSelLowSensorUSB1ExSelInFlag  = TRUE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelLowSensorUSB1ExSelInFlag success\r\n");
    }
    else
    {
        SensorUR7ExSelLowSensorUSB1ExSelInFlag  = FALSE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelLowSensorUSB1ExSelInFlag error.\r\n");
    }
    //Set SensorUR7ExSel(PB1) HIGH
    GPIO_SetBit(GPIOB, BIT1);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT2))
    {
        SensorUR7ExSelHighSensorUSB1ExSelInFlag = TRUE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelHighSensorUSB1ExSelInFlag success\r\n");
    }
    else
    {
        SensorUR7ExSelHighSensorUSB1ExSelInFlag = FALSE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelHighSensorUSB1ExSelInFlag error.\r\n");
    }
    GPIO_CloseBit(GPIOB, BIT1);
    GPIO_CloseBit(GPIOB, BIT2);
    
    
    
    //Set SensorUR7ExSel(PB1) input
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<< 4)) | (0<< 4));
    GPIO_OpenBit(GPIOB, BIT1, DIR_INPUT, PULL_UP);
    //Set SensorUSB1ExSel(PB2) output
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOB, BIT2, DIR_OUTPUT, PULL_UP);
        
    //Set SensorUSB1ExSel(PB2) LOW
    GPIO_ClrBit(GPIOB, BIT2);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT1))
    {
        SensorUR7ExSelInSensorUSB1ExSelLowFlag  = TRUE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelInSensorUSB1ExSelLowFlag success\r\n");
    }
    else
    {
        SensorUR7ExSelInSensorUSB1ExSelLowFlag  = FALSE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelInSensorUSB1ExSelLowFlag error.\r\n");
    }
    
    //Set SensorUSB1ExSel(PB2) HIGH
    GPIO_SetBit(GPIOB, BIT2);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT1))
    {
        SensorUR7ExSelInSensorUSB1ExSelHighFlag = TRUE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelInSensorUSB1ExSelHighFlag success\r\n");
    }
    else
    {
        SensorUR7ExSelInSensorUSB1ExSelHighFlag = FALSE;
        //terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect SensorUR7ExSelInSensorUSB1ExSelHighFlag error.\r\n");
    }
    GPIO_CloseBit(GPIOB, BIT1);
    GPIO_CloseBit(GPIOB, BIT2);

    
    if(SensorUR7ExSelLowSensorUSB1ExSelInFlag && SensorUR7ExSelHighSensorUSB1ExSelInFlag &&
       SensorUR7ExSelInSensorUSB1ExSelLowFlag && SensorUR7ExSelInSensorUSB1ExSelHighFlag)
    {
        terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect success\r\n");
        IOtestResultFlag[18] = TRUE;
        MTPString[29][7] = 0x81;
    }
    else
    {
        terninalPrintf("Sensor USB1ExSel & Sensor UR7ExSel connect error.\r\n");
        IOtestResultFlag[18] = FALSE;
        MTPString[29][7] = 0x80;
        return FALSE;
    }
    
    //---------------------------end------------------------------
    
    
    
    //-------------Sensor USB1PwCT & Sensor UR3ExSel connect----------------------------
    
    BOOL SensorUR3ExSelLowSensorUSB1PwCTInFlag  = FALSE;
    BOOL SensorUR3ExSelHighSensorUSB1PwCTInFlag = FALSE;
    BOOL SensorUR3ExSelInSensorUSB1PwCTLowFlag  = FALSE;
    BOOL SensorUR3ExSelInSensorUSB1PwCTHighFlag = FALSE;

    
    GPIO_CloseBit(GPIOB, BIT5);
    GPIO_CloseBit(GPIOB, BIT0);
   //Set SensorUR3ExSel(PB0) and SensorUSB1PwCT(PB5) GPIO
    //Set SensorUR3ExSel(PB0) output
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<< 0)) | (0<< 0));
    GPIO_OpenBit(GPIOB, BIT0, DIR_OUTPUT, PULL_UP);
    //Set SensorUSB1PwCT(PB5) input
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<20)) | (0<<20));
    GPIO_OpenBit(GPIOB, BIT5, DIR_INPUT, PULL_UP);
    
    //Set SensorUR3ExSel(PB0) LOW
    GPIO_ClrBit(GPIOB, BIT0);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT5))
    {
        SensorUR3ExSelLowSensorUSB1PwCTInFlag  = TRUE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelLowSensorUSB1PwCTInFlag success\r\n");
    }
    else
    {
        SensorUR3ExSelLowSensorUSB1PwCTInFlag  = FALSE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelLowSensorUSB1PwCTInFlag error.\r\n");
    }
    //Set SensorUR3ExSel(PB0) HIGH
    GPIO_SetBit(GPIOB, BIT0);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT5))
    {
        SensorUR3ExSelHighSensorUSB1PwCTInFlag = TRUE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelHighSensorUSB1PwCTInFlag success\r\n");
    }
    else
    {
        SensorUR3ExSelHighSensorUSB1PwCTInFlag = FALSE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelHighSensorUSB1PwCTInFlag error.\r\n");
    }
    GPIO_CloseBit(GPIOB, BIT5);
    GPIO_CloseBit(GPIOB, BIT0);
    
    
    
    //Set SensorUR3ExSel(PB0) input
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<< 0)) | (0<< 0));
    GPIO_OpenBit(GPIOB, BIT0, DIR_INPUT, PULL_UP);
    //Set SensorUSB1PwCT(PB5) output
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<20)) | (0<<20));
    GPIO_OpenBit(GPIOB, BIT5, DIR_OUTPUT, PULL_UP);
        
    //Set SensorUSB1PwCT(PB5) LOW
    GPIO_ClrBit(GPIOB, BIT5);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(!GPIO_ReadBit(GPIOB, BIT0))
    {
        SensorUR3ExSelInSensorUSB1PwCTLowFlag  = TRUE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelInSensorUSB1PwCTLowFlag success\r\n");
    }
    else
    {
        SensorUR3ExSelInSensorUSB1PwCTLowFlag  = FALSE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelInSensorUSB1PwCTLowFlag error.\r\n");
    }
    
    //Set SensorUSB1PwCT(PB5) HIGH
    GPIO_SetBit(GPIOB, BIT5);
    vTaskDelay(100/portTICK_RATE_MS);
    //userResponseLoop();
    if(GPIO_ReadBit(GPIOB, BIT0))
    {
        SensorUR3ExSelInSensorUSB1PwCTHighFlag = TRUE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelInSensorUSB1PwCTHighFlag success\r\n");
    }
    else
    {
        SensorUR3ExSelInSensorUSB1PwCTHighFlag = FALSE;
        //terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect SensorUR3ExSelInSensorUSB1PwCTHighFlag error.\r\n");
    }
    GPIO_CloseBit(GPIOB, BIT5);
    GPIO_CloseBit(GPIOB, BIT0);
    //Set SensorUR3ExSel(PB0) output
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<< 0)) | (0<< 0));
    GPIO_OpenBit(GPIOB, BIT0, DIR_OUTPUT, PULL_UP);
    //Set SensorUSB1PwCT(PB5) output
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<20)) | (0<<20));
    GPIO_OpenBit(GPIOB, BIT5, DIR_OUTPUT, PULL_UP);

    
    if(SensorUR3ExSelLowSensorUSB1PwCTInFlag && SensorUR3ExSelHighSensorUSB1PwCTInFlag &&
       SensorUR3ExSelInSensorUSB1PwCTLowFlag && SensorUR3ExSelInSensorUSB1PwCTHighFlag)
    {
        terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect success\r\n");
        IOtestResultFlag[19] = TRUE;
        MTPString[29][8] = 0x81;
    }
    else
    {
        terninalPrintf("Sensor USB1PwCT & Sensor UR3ExSel connect error.\r\n");
        IOtestResultFlag[19] = FALSE;
        MTPString[29][8] = 0x80;
        return FALSE;
    }
    
    //---------------------------end------------------------------
    
    //-----------------------LED D2 test--------------------------
    
    BOOL LEDtestFlag = FALSE;
    char tempchar;
    LEDColorBuffSet(0x02, 0x00);
    LEDBoardLightSet();
    SetMTPCRC(30,(uint8_t*)MTPString);
    MTPCmdprint(30,(uint8_t*)MTPString);
    terninalPrintf("Is the D2 light signal correct?(y/n)\r\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            terninalPrintf("Sensor SysPwr test success.\r\n");
            LEDtestFlag = TRUE;
            MTPString[29][9] = 0x81;
            break;
        }
        else if(tempchar =='n')
        {
            terninalPrintf("Sensor SysPwr test error.\r\n");
            LEDtestFlag = FALSE;
            
            LEDColorBuffSet(0x00, 0x00);
            LEDBoardLightSet();
            MTPString[29][9] = 0x80;
            return FALSE;
            break;
        }
    }
    LEDColorBuffSet(0x00, 0x00);
    LEDBoardLightSet();

    

    //---------------------------end------------------------------
    

    
    if( IOtestResultFlag[5]  &&     //  radar UR3TX & radar UR7RX connect test result
        IOtestResultFlag[6]  &&     //  radar UR7TX & radar UR3RX connect test result
        IOtestResultFlag[7]  &&     //  camera1 test result
        //IOtestResultFlag[8]  &&   //  camera2 test result
        IOtestResultFlag[17] &&     //  Sensor UR3PwrCT & Sensor UR7PwrCT connect test result
        IOtestResultFlag[18] &&     //  Sensor USB1ExSel & Sensor UR7ExSel connect test result
        IOtestResultFlag[19] &&     //  Sensor USB1PwCT & Sensor UR3ExSel connect test result
        LEDtestFlag             )   //  SysPwr test result
    {
        MTPString[29][10] = 0x81;
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
    else
    {
        MTPString[29][10] = 0x80;
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;    
    }
}
//////////////////////  Lidar test//////////////////////// single
static BOOL lidarSingleTest(void* para1, void* para2)
{    
    static RadarInterface* pRadarInterface;
    BOOL changeFlag;
    terninalPrintf("!!! lidarTest !!!\r\n");
    guiPrintResult("Lidar");
    pRadarInterface = RadarGetInterface(LIDAR_AV_DESIGN_INTERFACE_INDEX);
    terninalPrintf("Lidar Initial...");
    guiPrintMessage("Lidar Initial...");
    
    if(pRadarInterface == NULL)
    {
        terninalPrintf("lidarTest ERROR (pRadarInterface == NULL)!!\n");
        return TEST_FALSE;
    }
    
    if(pRadarInterface->initFunc() == FALSE)
    {
        //EPDDrawString(TRUE,"FAIL            \n                  ",X_POS_RST,Y_POS_RST);
        terninalPrintf("lidarTest ERROR (initFunc false)!!\n");
        return TEST_FALSE;
    }
    pRadarInterface->setPowerStatusFunc(0,TRUE);
    pRadarInterface->setPowerStatusFunc(1,TRUE);
    guiPrintMessage("Lidar0 Test...");
    int featureValue = pRadarInterface->checkFeaturnFunc(0, &changeFlag, NULL, NULL, NULL);
    if(featureValue==LIDAR_FEATURE_OCCUPIED || featureValue==LIDAR_FEATURE_VACUUM || featureValue==LIDAR_FEATURE_UN_STABLED)
    {
        guiPrintMessage("Lidar0 OK");
        switch(featureValue)
        {
        case LIDAR_FEATURE_OCCUPIED:
            terninalPrintf("[LIDAR]0 ->  LIDAR_FEATURE_OCCUPIED!!\n" );
            break;
        case LIDAR_FEATURE_VACUUM:
            terninalPrintf("[LIDAR]0 ->  LIDAR_FEATURE_VACUUM!!\n" );
            break;
        case LIDAR_FEATURE_UN_STABLED:
            terninalPrintf("[LIDAR]0 ->  LIDAR_FEATURE_UN_STABLED!!\n" );
            break; 
        }
    }
    else
    {
        guiPrintMessage("Lidar0 TIMEOUT");
        terninalPrintf("[LIDAR]0 ->  TIMEOUT!!\n" ); 
        terninalPrintf("LidarTest [ERROR]\n");
        return TEST_FALSE;
    }
    featureValue = pRadarInterface->checkFeaturnFunc(1, &changeFlag, NULL, NULL, NULL);
    if(featureValue==LIDAR_FEATURE_OCCUPIED || featureValue==LIDAR_FEATURE_VACUUM || featureValue==LIDAR_FEATURE_UN_STABLED)
    {
        guiPrintMessage("Lidar1 OK");
        switch(featureValue)
        {
        case LIDAR_FEATURE_OCCUPIED:
            terninalPrintf("[LIDAR]1 ->  LIDAR_FEATURE_OCCUPIED!!\n" );
            break;
        case LIDAR_FEATURE_VACUUM:
            terninalPrintf("[LIDAR]1 ->  LIDAR_FEATURE_VACUUM!!\n" );
            break;
        case LIDAR_FEATURE_UN_STABLED:
            terninalPrintf("[LIDAR]1 ->  LIDAR_FEATURE_UN_STABLED!!\n" );
            break; 
        }
    }
    else
    {
        guiPrintMessage("Lidar1 TIMEOUT");
        terninalPrintf("[LIDAR]1 ->  TIMEOUT!!\n" ); 
        terninalPrintf("LidarTest [ERROR]\n");
        return TEST_FALSE;
    }
    return TEST_SUCCESSFUL_LIGHT_OFF;
}


//////////////////toolsAdjustKeypad///////////////////////
/*
static BOOL toolsAdjustKeypad(void* para1, void* para2)
{    
    setPrintfFlag(FALSE);
    if(NT066EDrvInit(TRUE) == FALSE)
    {
        terninalPrintf("  AdjustKeypad [ERROR]\r\n");
    }
    else
    {

        //if(NT066EResetChip())
        {
            terninalPrintf("  AdjustKeypad [OK](set I2C1 to input)...(press 'q' to exit!!!)\r\n");
            outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<8)) | (0x0<<8));//GPG2 input
            outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<12)) | (0x0<<12));//GPG3 input
            GPIO_OpenBit(GPIOG, BIT2, DIR_INPUT, NO_PULL_UP);
            GPIO_OpenBit(GPIOG, BIT3, DIR_INPUT, NO_PULL_UP);
            while(1)
            {
                if(sysIsKbHit())
                {
                    if(getTerminalChar() == 'q')
                        break;
                }
                vTaskDelay(100/portTICK_RATE_MS);
            }
            terninalPrintf("  AdjustKeypad Exit(set I2C1 to function)\r\n");
            outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<8)) | (0x8<<8));//GPG2 I2C1_SCL
            outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<12)) | (0x8<<12));//GPG3 I2C1_SDA
        }
        //else
        //{
        //    terninalPrintf("  AdjustKeypad [ERROR]\r\n");
        //}
    }


    setPrintfFlag(TRUE);
    return TRUE;
}
//////////////////idConfig//////////////////////////////////////
static BOOL idConfig(void* para1, void* para2)
{        
    terninalPrintf("  idConfig...(press 'q' to exit!!!)\r\n");
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xFFFFF<<0)) | (0x00000u<<0));    
    while(1)
    {
        UINT32 portValue = GPIO_ReadPort(GPIOJ);
        terninalPrintf(" >> epmid:%d (0x%02x) (portValue = 0x%08x)\n",portValue&0x1F, portValue&0x1F, portValue);
        if(userResponse()=='q'){
            break;
        }
        vTaskDelay(100/portTICK_RATE_MS);
    }
    return TRUE;
}
//////////////////enable12vPower///////////////////////////////
static BOOL enable12vPower(void* para1, void* para2)
{        
    terninalPrintf("  12V power...(press 'q' to exit!!!)\r\n");
    terninalPrintf("    1) turn ON\r\n");
    terninalPrintf("    2) turn OFF\r\n");
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<24)) | (0x0u<<24));  
    GPIO_OpenBit(GPIOG, BIT6, DIR_OUTPUT, NO_PULL_UP); 
    GPIO_ClrBit(GPIOG, BIT6);    

    while(1)
    {
        if(sysIsKbHit())
        {
            char cmd = getTerminalChar();
            switch(cmd)
            {
            case 'q':
                return TRUE;
            case '1':
                terninalPrintf("--> turn ON (GPG6 high)\r\n");
                GPIO_SetBit(GPIOG, BIT6); 
                break;
            case '2':
                terninalPrintf("--> turn OFF (GPG6 low)\r\n");
                GPIO_ClrBit(GPIOG, BIT6);    
                break;
            }
        }
        vTaskDelay(100/portTICK_RATE_MS);
    }
    return TRUE;
}
*/
static BOOL suspendSystem(void* para1, void* para2)
{ 
    
    UINT32 reg;
    char tempchar;
    BOOL testFlag = FALSE;
    
    //LEDColorBuffSet(0x00,0x01 );
    //LEDBoardLightSet();
    //vTaskDelay(100/portTICK_RATE_MS);
    terninalPrintf("CPU enter sleep... \r\n");
    //terninalPrintf("Please touch keypad to continue. \r\n");
    terninalPrintf("Please wait 5 seconds to continue. \r\n");
    SetMTPCRC(34,(uint8_t*)MTPString);
    MTPCmdprint(34,(uint8_t*)MTPString);
    vTaskDelay(500/portTICK_RATE_MS);
    //EPDSetBacklight(TRUE);

    //userResponseLoop();
    /*
    GPIO_CloseBit(GPIOF, BIT14);
    GPIO_CloseBit(GPIOI, BIT15);
    GPIO_CloseBit(GPIOG, BIT9);
    GPIO_CloseBit(GPIOF, BIT13);
    GPIO_CloseBit(GPIOI, BIT4);
    */
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0x9<<24));
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<28)) | (0x0<<28));
    GPIO_OpenBit(GPIOI, BIT15, DIR_OUTPUT, NO_PULL_UP);
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(GPIOG, BIT9, DIR_OUTPUT, NO_PULL_UP);
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0x9<<20)); 
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOI, BIT4, DIR_OUTPUT, NO_PULL_UP);
    
    
    //GPIO_SetBit(GPIOE, BIT7);   // close UART1 RS232 (debug) power
    //GPIO_ClrBit(GPIOI, BIT15);  // close ReaderBoardSpareIO2
    //GPIO_SetBit(GPIOG, BIT9);     // close UART2 RS232 & RS485 (Reader)power
    //GPIO_ClrBit(GPIOF, BIT8);   // close LED power
    GPIO_ClrBit(GPIOI, BIT1);   // close EPD power
    GPIO_ClrBit(GPIOG, BIT7);
    //GPIO_SetBit(GPIOB, BIT6);   // sensor board power enable
    //GPIO_SetBit(GPIOB, BIT3);  // close UART10 RS232 (CAD) power

    //GPIO_SetBit(GPIOE, BIT8);   // close TouchKeyPwrCT
    GPIO_ClrBit(GPIOF, BIT10);
    
    //GPIO_SetBit(GPIOE,BIT14);
    //vTaskDelay(100/portTICK_RATE_MS);
    /*
    GPIO_CloseBit(GPIOI, BIT0);
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(GPIOI, BIT0, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT5);
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(GPIOI, BIT5, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT6);
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOI, BIT6, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT7);
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<28)) | (0x0<<28));
    GPIO_OpenBit(GPIOI, BIT7, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT8);
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(GPIOI, BIT8, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT9);
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(GPIOI, BIT9, DIR_INPUT, NO_PULL_UP);
    */
     /*   
    GPIO_CloseBit(GPIOE, BIT0);
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(GPIOE, BIT0, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOE, BIT1);
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(GPIOE, BIT1, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOE, BIT2);
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(GPIOE, BIT2, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOE, BIT3);
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOE, BIT3, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOE, BIT4);
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOE, BIT4, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOE, BIT5);
    outpw(REG_SYS_GPE_MFPL,(inpw(REG_SYS_GPE_MFPL) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(GPIOE, BIT5, DIR_INPUT, NO_PULL_UP);
    */
    /*
    GPIO_CloseBit(GPIOF, BIT11);
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOF, BIT11, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOF, BIT12);
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOF, BIT12, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOF, BIT13);
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(GPIOF, BIT13, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOF, BIT14);
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOF, BIT14, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOB, BIT7);
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<28)) | (0x0<<28));
    GPIO_OpenBit(GPIOB, BIT7, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOB, BIT8);
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(GPIOB, BIT8, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOB, BIT9);
    outpw(REG_SYS_GPB_MFPH,(inpw(REG_SYS_GPB_MFPH) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(GPIOB, BIT9, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOG, BIT10);
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(GPIOG, BIT10, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOG, BIT11);
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOG, BIT11, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOG, BIT12);
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOG, BIT12, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOG, BIT13);
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(GPIOG, BIT13, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOG, BIT14);
    outpw(REG_SYS_GPG_MFPH,(inpw(REG_SYS_GPG_MFPH) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOG, BIT14, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT10);
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(GPIOI, BIT10, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT11);
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOI, BIT11, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT12);
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOI, BIT12, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT13);
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(GPIOI, BIT13, DIR_INPUT, NO_PULL_UP);
    GPIO_CloseBit(GPIOI, BIT14);
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOI, BIT14, DIR_INPUT, NO_PULL_UP);
    */
    
    
    
    GPIO_ClrBit(GPIOG, BIT6);
   // SleeptestFlag = TRUE;
    //NT066ESetPower(TRUE);
    
    
    RTC_Ioctl(0, RTC_IOC_SET_RELEATIVE_ALARM, 5, NULL);
    
    
    //RTC_Ioctl(0, RTC_IOC_SET_RELEATIVE_ALARM, 0xFFFFFFFF, NULL);
    sysDelay(10/portTICK_RATE_MS);
    outpw(REG_SYS_WKUPSSR , inpw(REG_SYS_WKUPSSR)); // clean wakeup status

    outpw(0xB00001FC, 0x59);
    outpw(0xB00001FC, 0x16);
    outpw(0xB00001FC, 0x88);
    while(!(inpw(0xB00001FC) & 0x1));
    
    
    //outpw(REG_SYS_WKUPSER , (1 << 2)| (1 << 3)| (1 << 4)| (1 << 1)|(1 << 24)); // wakeup source select DIP(EINT2, EINT3, EINT4) Keypad(EINT1) RTC
    // outpw(REG_SYS_WKUPSER ,  (1 << 1)); // wakeup source select DIP(EINT2, EINT3, EINT4) Keypad(EINT1) RTC
     outpw(REG_SYS_WKUPSER ,  (1 << 24)); // wakeup source select RTC
    
    reg=inpw(REG_CLK_PMCON);   //Enable NUC970 to enter power down mode
    reg = reg & (0xFF00FFFE);
    outpw(REG_CLK_PMCON,reg);
    
    
    /*
    terninalPrintf("Is CPU sleep?(y/n)\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            //outpw(REG_CLK_PMCON,reg | 0x01);
            RTC_Ioctl(0, RTC_IOC_SET_RELEATIVE_ALARM, 1, NULL);
            break;
        }
        else if(tempchar =='n')
        {
            //outpw(REG_CLK_PMCON,reg | 0x01);
            RTC_Ioctl(0, RTC_IOC_SET_RELEATIVE_ALARM, 1, NULL);
            break;
        }
    }
    */
    
      
    __wfi();    

    
    outpw(REG_SYS_WKUPSER , 0); // wakeup source select NONE  

    PowerClearISR();
    
    //NT066ESetPower(FALSE);
    
    //GPIO_SetBit(GPIOF, BIT8);   // open LED power
    //GPIO_ClrBit(GPIOE,BIT14);
    GPIO_SetBit(GPIOG, BIT6);
    //EPDSetBacklight(FALSE);
    //LEDColorBuffSet(0x00,0x00 );
    //LEDBoardLightSet();
    //----recovery reader flash setting-----------
    
    GPIO_CloseBit(GPIOB, BIT7);
    GPIO_CloseBit(GPIOB, BIT8);
    GPIO_CloseBit(GPIOB, BIT9);
    
    GPIO_CloseBit(GPIOI, BIT10);
    GPIO_CloseBit(GPIOI, BIT11);
    GPIO_CloseBit(GPIOI, BIT12);
    GPIO_CloseBit(GPIOI, BIT13);
    GPIO_CloseBit(GPIOI, BIT14);

    
    SpiInterface* pSpiInterface;
    pSpiInterface = SpiGetInterface(0);
    pSpiInterface->initFunc();
    
    //GPI14 FLASH1 CS pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOI, BIT14, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOI, BIT14);
    
    //GPI13 FLASH1 CS pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(GPIOI, BIT13, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOI, BIT13);
    
    //GPI11 FLASH0 WP pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOI, BIT11, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOI, BIT11);
    //GPI12 FLASH0 HD pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOI, BIT12, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOI, BIT12);
    
    /*
    //GPI4 FLASH1 WP pin
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOI, BIT4, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOI, BIT4);


    
    //GPI15 FLASH1 HD pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(GPIOI, BIT15, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOI, BIT15);
    */
    //---------------------------------------------------------------
    
    
    

    //BuzzerPlay(200, 500, 1, TRUE);
    
    //----recovery EPD setting-----------
    
    GPIO_CloseBit(GPIOI, BIT0);
    GPIO_CloseBit(GPIOI, BIT5);
    GPIO_CloseBit(GPIOI, BIT6);
    GPIO_CloseBit(GPIOI, BIT7);
    GPIO_CloseBit(GPIOI, BIT8);
    GPIO_CloseBit(GPIOI, BIT9);

    SpiInterface* EPDpSpiInterface;
    EPDpSpiInterface = SpiGetInterface(1);
    EPDpSpiInterface->initFunc();
    

    
    //RDY Pin
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(GPIOI, BIT9, DIR_INPUT, PULL_UP);  
    
    //GPI0 reset pin
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(GPIOI, BIT0, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(GPIOI, BIT0); 
    
    EPDpSpiInterface->setPin();


    GPIO_SetBit(GPIOI, BIT1);   // open EPD power
    //ReadIT8951SystemInfo( NULL, NULL);
    
    GPIO_ClrBit(GPIOI, BIT0); 
   
    sysDelay(1000/portTICK_RATE_MS);
    //ReloadWWDT();
    sysDelay(1000/portTICK_RATE_MS);
    //ReloadWWDT();
    sysDelay(1000/portTICK_RATE_MS);

    GPIO_SetBit(GPIOI, BIT0); 
    
    
    
    //----------------------------------

    //SleeptestFlag = FALSE;
    /*
    if(tempchar =='y')
    {
        //outpw(REG_CLK_PMCON,reg | 0x01);
        return TEST_SUCCESSFUL_LIGHT_OFF;
    }
    else if(tempchar =='n')
    {
        //outpw(REG_CLK_PMCON,reg | 0x01);
        return TEST_FALSE;
    }
    */
    SetMTPCRC(35,(uint8_t*)MTPString);
    MTPCmdprint(35,(uint8_t*)MTPString);
    terninalPrintf("Is CPU sleep?(y/n)\r\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            //outpw(REG_CLK_PMCON,reg | 0x01);
            MTPString[33][3] = 0x81;
            testFlag = TRUE;
            break;
            //return TEST_SUCCESSFUL_LIGHT_OFF;
        }
        else if(tempchar =='n')
        {
            //outpw(REG_CLK_PMCON,reg | 0x01);
            MTPString[33][3] = 0x80;
            testFlag = FALSE;
            break;
            //if(MBtestFlag )
            //    return FALSE;
            //else
            //    return TEST_FALSE;
            
        }
    }
    

    
    
    if(testFlag)
        return TEST_SUCCESSFUL_LIGHT_OFF;
    else
    {
        if(MBtestFlag )
            return FALSE;
        else
            return TEST_FALSE;
    }
    
    /*
    UINT32 reg;
    char tempchar;
    
    terninalPrintf("  suspendSystem\r\n");
    setPrintfFlag(FALSE);
    PowerSuspend(0xffffffff); 
    //PowerClearISR(); 
    setPrintfFlag(TRUE);
    terninalPrintf("Is CPU sleep?(y/n)\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='y')
        {
            //outpw(REG_CLK_PMCON,reg | 0x01);
            return TEST_SUCCESSFUL_LIGHT_OFF;
        }
        else if(tempchar =='n')
        {
            //outpw(REG_CLK_PMCON,reg | 0x01);
            return TEST_FALSE;
        }
    }
    */
    
    /*
    terninalPrintf("  suspendSystem\r\n");
    //setPrintfFlag(FALSE);
    PowerSuspend(0xffffffff); 
    PowerClearISR(); 
    //setPrintfFlag(TRUE);
    return TRUE;
    */
    
}

/*
static BOOL batterySelect(void* para1, void* para2)
{        
    terninalPrintf("  batterySelect...(press 'q' to exit!!!)\r\n");
    terninalPrintf("    1) Battery 1 ON\r\n");
    terninalPrintf("    2) Battery 1 OFF\r\n");
    terninalPrintf("    3) Battery 2 ON\r\n");
    terninalPrintf("    4) Battery 2 OFF\r\n");
    setPrintfFlag(FALSE);  
    if(BatteryDrvInit(TRUE))
    {    
        while(1)
        {
            if(sysIsKbHit())
            {
                char cmd = getTerminalChar();
                switch(cmd)
                {
                case 'q':
                    return TRUE;
                case '1':
                    terninalPrintf("--> Battery 1 ON\r\n");
                    BatterySetSwitch1(TRUE); 
                    break;
                case '2':
                    terninalPrintf("--> Battery 1 OFF\r\n");
                    BatterySetSwitch1(FALSE);     
                    break;
                case '3':
                    terninalPrintf("--> Battery 2 ON\r\n");
                    BatterySetSwitch2(TRUE); 
                    break;
                case '4':
                    terninalPrintf("--> Battery 2 OFF\r\n");
                    BatterySetSwitch2(FALSE);     
                    break;
                }
            }
            vTaskDelay(100/portTICK_RATE_MS);
        }
    }
    setPrintfFlag(TRUE);
    return TRUE;
}

static BOOL buzzerLoop(void* para1, void* para2)
{        
    terninalPrintf("  buzzerLoop...(press 'q' to exit!!!)\r\n");
    if(BuzzerDrvInit(TRUE))
    {    
        while(1)
        {
            if(sysIsKbHit())
            {
                char cmd = getTerminalChar();
                switch(cmd)
                {
                case 'q':
                    return TRUE;                    
                }
            }
            BuzzerPlay(500, 0, 1, TRUE);
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    }
    return TRUE;
}
*/
/********************************************************************************
 *                                    TOOL                                      *
 ********************************************************************************/
/*  1   */
/////////////////////////////////DEVICE ID////////////////////////////////
static BOOL setDeviceIDTool(void* para1, void* para2){

    guiManagerShowScreen(GUI_SETTING_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
    if(!FlashDrvExInit(FALSE)){
        terninalPrintf("No Flash [Error]!!!\r\n");
        BuzzerPlay(200,50,4,FALSE);
        LEDColorBuffSet(0x00,0xff);
        LEDBoardLightSet();
        vTaskDelay(700/portTICK_RATE_MS);
        return FALSE;
    }
    char charTmp;
    unsigned int idTmp=0,i=0;
    terninalPrintf("Current Device ID: %05d\nPlease Enter 5 Digit Number To Set Device ID\n",getDeviceID(0,0));
    terninalPrintf("Press 'q' to quit without save...\n");
    while(1){
        charTmp=userResponseLoop();
        if(charTmp>='0'&&charTmp<='9')
        {
            idTmp=idTmp*10;
            idTmp+=(charTmp-'0');
            i++;
            terninalPrintf("%c",charTmp);
        }
        if(charTmp=='y')
        {
            setDeviceID(&epmIDSet,NULL);//empIDSet is set by guisettingid
            break;
        }
        if(charTmp=='q')
        {
            break;
        }
        if(i==5)
        {
            setDeviceID(&idTmp,NULL);
            break;
        }
    }
    return TRUE;
}

static BOOL setDeviceID(void* para1, void* para2)
{    
    FlashDrvExInit(FALSE);
    
    int epmID = *(int*)para1 ;

    if(SFlashSaveStorage(SFLASH_STORAGE_EPM_SERIAL_ID_INDEX_BASE, (uint8_t*)&epmID, sizeof(epmID)))
    {
        terninalPrintf(" >> setDeviceID:%05d (to SFlash Record) write OK\n", epmID);
    }
    return TRUE;
}
/*  2   */
static BOOL getDeviceIDTool(void* para1, void* para2)
{
    guiManagerShowScreen(GUI_SETTING_ID, GUI_REDRAW_PARA_REFRESH, 0,GET_ID_MODE);
    terninalPrintf(" >> Getting DeviceID...\n");
    int epmid=getDeviceID(0,0);
    terninalPrintf(" >> getDeviceIDTool:%d (from SFlash Record)\n", epmid);
    terninalPrintf("Press 'q' to exit\r\n");
    while(1)
    {
        if(userResponse()=='q')
            break;
        vTaskDelay(300/portTICK_RATE_MS);
    }
    return TRUE;
}
static int getDeviceID(void* para1, void* para2)
{
    int epmid;
    FlashDrvExInit(FALSE);
    if(SFlashLoadStorage(SFLASH_STORAGE_EPM_SERIAL_ID_INDEX_BASE, (uint8_t*)&epmid, sizeof(epmid)))
    {  
        terninalPrintf(" >> getDeviceID:%05d (from SFlash Record) read OK\n", epmid);
    }
    return epmid;
}
/*  3   */
///////////////////////////CARD READER////////////////////////////////
#define X_MARGIN_LODING 312
#define Y_MARGIN_LODING 298

static void setCNResultCallback(BOOL flag, uint8_t* cn, int cnLen)
{
    static BOOL prevFlag = FALSE; 
    if(prevFlag != flag)
    {
        if(flag)
        {
            BuzzerPlay(200, 2, 1, FALSE);
            uint32_t tmpCardID=0;
            terninalPrintf("Detect Card!\nCard ID:%02X%02X%02X%02X\n\n",cn[0],cn[1],cn[2],cn[3]);
            for(int i = 0; i<cnLen; i++)
            {
                uint32_t tmpByte = cn[i];
                tmpCardID = tmpCardID | tmpByte << 8*(cnLen-i-1);
            }
            cardID=tmpCardID;
            //terninalPrintf("\r\n");
            //show id at epd
            for(int i=0;i<8;i++)
            {
                if(i<7)
                    EPDDrawMulti(FALSE,((cardID>>(28-(i*4)))&0xf) + EPD_PICT_NUM_SMALL_INDEX,500+(i*50),454);
                else if(i==7)
                    EPDDrawMulti(TRUE,((cardID>>(28-(i*4)))&0xf) + EPD_PICT_NUM_SMALL_INDEX,500+(i*50),454);
            }
        }
        else
        {//clean screen
            terninalPrintf("Please tag your card again\n");
            cardID=0; 
            for(int i=0;i<8;i++)
            {
                EPDDrawString(FALSE,"  ",500+(i*50),454);
                EPDDrawString(FALSE,"  ",500+(i*50),498);
            }
            EPDDrawString(TRUE,"  ",500,454);
        }
    }
//    terninalPrintf(".");
    prevFlag = flag;
}

static BOOL readerGetCN(void* para1, void* para2)
{    
    guiManagerShowScreen(GUI_SHOW_CARD_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
    int waitCounter = 30;
    int iCount=0;
//    uint16_t uID;
    terninalPrintf("!!! readerGetCN !!!\r\n");
    CardReaderInit(TRUE);
    CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
    terninalPrintf("READER CHECK ");
    while(CardReaderGetBootedStatus() != TSREADER_CHECK_READER_OK)
    {
        terninalPrintf(".");
        if(iCount==4)
        {
            iCount=0;
            EPDDrawString(TRUE,"          ",X_MARGIN_LODING,Y_MARGIN_LODING);
        }
        EPDDrawString(TRUE,".",X_MARGIN_LODING+(iCount*28),Y_MARGIN_LODING);
        vTaskDelay(1000/portTICK_RATE_MS);
        waitCounter--;
        iCount++;
        if(waitCounter == 0)
        {
            terninalPrintf("\nCHECK READER [Time Out]\n");
            break;
        }
    }
    EPDDrawString(TRUE,"          ",X_MARGIN_LODING,Y_MARGIN_LODING);
    vTaskDelay(1000/portTICK_RATE_MS);
    BuzzerPlay(500, 2, 2, TRUE);
    EPDDrawString(TRUE,"Please tag your card        ",240,254);
    terninalPrintf("\nPlease tag your card\nPress 'q' To Quit...\n");
    while(CardReaderGetBootedStatus() == TSREADER_CHECK_READER_OK)
    {
        CardReaderProcessCN(setCNResultCallback);
        if(userResponse()=='q')
        {
            break;
        }
    }
    return TRUE;
}

///////////////////////////CAM   Test/////////////////////////////////
/*Globa Val*/
#define CAM_MSG_X 180
#define CAM_MSG_Y 250

static BOOL usbCamTest(void* para1, void* para2)
{
/*
    int  SDbufferSize = 128 ;
    uint8_t SDbuffer[SDbufferSize];
    FIL filephoto;
    char * PhotoFileNameStr;
    UINT br;
    
    int count ;
    int remain ;
    int progress ;

    
    if(!UserDrvInit(FALSE))
    {
        terninalPrintf("UserDrvInit fail.\r\n");
        return FALSE;
    }
    if(!FatfsInit(TRUE))
    {
        terninalPrintf("FatfsInit fail.\r\n");
        return FALSE;
    }
    */
    int count ;
    int remain ;
    int progress ;
    
    guiManagerShowScreen(GUI_USB_CAM_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
    static CameraInterface*     pCameraInterface = NULL;
    terninalPrintf("!!! usbCamTest !!!\r\n");
//    setPrintfFlag(FALSE)
    pCameraInterface = CameraGetInterface(CAMERA_UVC_INTERFACE_INDEX);
    if(pCameraInterface == NULL)
    {
        sysprintf("usbCamTest ERROR (pCameraInterface == NULL)!!\n");
        return FALSE;
    }
    if(pCameraInterface->initFunc(FALSE) == FALSE)
    {
        sysprintf("usbCamTest ERROR (pCameraInterface->initFunc(FALSE) == FALSE)!!\n");
        return FALSE;
    }
    int okCounter[2] = {0};
    int errorCounter[2] = {0};
    uint8_t* photoPr ;
    //uint8_t tempPr[3];
    //uint8_t photoPr[100000];
    int photoLen = 0;
    char targetFileNameTmp[_MAX_LFN];
    memset(targetFileNameTmp, 0x0, sizeof(targetFileNameTmp));
    
    sprintf(targetFileNameTmp,"uvcphoto.%s", PHOTO_FILE_EXTENSION); 
    vTaskDelay(3000/portTICK_RATE_MS);
    terninalPrintf("Press 'q' to Exit\r\n");
    EPDDrawString(TRUE,"Testing   ",CAM_MSG_X,CAM_MSG_Y);
    while(1)
    {   
        for(int i = 0; i<2; i++)
        {
//            setPrintfFlag(FALSE); 
            EPDDrawString(TRUE,"Testing   ",CAM_MSG_X+(i*400),CAM_MSG_Y);
            if(pCameraInterface->takePhotoFunc((1-i), &photoPr, &photoLen, FILE_AGENT_STORAGE_TYPE_FATFS, "0:", targetFileNameTmp, FALSE,1,0))
            {
                EPDDrawString(TRUE,"OK          ",CAM_MSG_X+(i*400),CAM_MSG_Y);
                terninalPrintf("  cameraTest _%d (%s) [OK] photoLen = %d\r\n", i+1, targetFileNameTmp,photoLen);
                
                if(!GPIO_ReadBit(GPIOJ, BIT3))
                {
                    count = photoLen / PHOTOBUFFERSIZE;
                    remain = photoLen % PHOTOBUFFERSIZE;
                    progress = count / 10 ;
                    terninalPrintf("count = %d\r\n",count);
                    terninalPrintf("remain = %d\r\n",remain);
                    terninalPrintf("progress = %d\r\n",progress);
                    
                    OpenCamPhoto(i);
                    
                    for(int m=0;m<count;m++)
                    {
                        if(m%progress == 0)
                        {
                            terninalPrintf("%d%% complete...\r",(m/progress)*10);
                        }
                        
                        for(int j=0;j<PHOTOBUFFERSIZE;j++)
                        //for(int j=0;j<256;j++)
                        {
                            //terninalPrintf("%02x ",*(photoPr+j));
                            tempPr[j] = *(photoPr+m*PHOTOBUFFERSIZE+j);
                        }
                        
                        SaveCamPhoto(i,NULL,PHOTOBUFFERSIZE);

                        
                    }
                    
                    if(remain != 0)
                    {   
                        
                        for(int j=0;j<remain;j++)
                        //for(int j=0;j<256;j++)
                        {
                            //terninalPrintf("%02x ",*(photoPr+j));
                            tempPr[j] = *(photoPr+count*PHOTOBUFFERSIZE+j);
                        }
                        
                        SaveCamPhoto(i,NULL,remain);
     
                    }
                    
                    CloseCamPhoto();
                    
                    /*
                    //terninalPrintf("photoPr = ");
                    for(int j=0;j<photoLen;j++)
                    //for(int j=0;j<256;j++)
                    {
                        //terninalPrintf("%02x ",*(photoPr+j));
                        tempPr[j] = *(photoPr+j);
                    }
                    
                    */
                    //terninalPrintf("\r\n");
                    
                    //terninalPrintf("photoPrAdd = %08x\r\n",photoPr);
                    
                    //SaveCamPhoto(i,photoPr,photoLen);

                        //tempPr[0] = i+1;
                        //tempPr[1] = i+1;
                        //tempPr[2] = i+1;
                    //PhotoBuffFunc(tempPr,3);
                    
                    
                    
                    
                    //SaveCamPhoto(i,NULL,photoLen);
                    //SaveCamPhoto(i,NULL,256);
                }

                
                
                
                
                
                
                okCounter[i]++;
            }
            else
            {
                EPDDrawString(TRUE,"FAIL       ",CAM_MSG_X+(i*400),CAM_MSG_Y);
                terninalPrintf("  cameraTest _%d [ERROR]\r\n", i+1);
                errorCounter[i]++;
            }
            terninalPrintf("**************************************************\r\n\r\n", i);
            if(userResponse()=='q')
            {
                return TRUE;
            }
        }
        terninalPrintf("==== Cam_1-> OK:%d, ERROR:%d ====\r\n", okCounter[0], errorCounter[0]);  
        terninalPrintf("==== Cam_2-> OK:%d, ERROR:%d ====\r\n", okCounter[1], errorCounter[1]);  
        //vTaskDelay(20000/portTICK_RATE_MS);
        vTaskDelay(2000/portTICK_RATE_MS);
    }
}
#define CAM_0_ON_OFF_X 480
#define CAM_0_ON_OFF_Y 250
#define CAM_1_ON_OFF_X 480
#define CAM_1_ON_OFF_Y 300
static BOOL usbCamPowerSet(void* para1, void* para2)
{   //init//
    BOOL camStatus[2]={TRUE,FALSE};
    BOOL changeFlag;
    char* boolConvString[2]={" OFF "," ON  "};
    guiManagerShowScreen(GUI_USB_CAM_ID, GUI_REDRAW_PARA_REFRESH, 0, POWER_SET_MODE);
    static CameraInterface* pCameraInterface;
    char tmp;
    terninalPrintf("!!! usbCamPowerSet !!!\r\n");
    pCameraInterface = CameraGetInterface(CAMERA_UVC_INTERFACE_INDEX);
    if(pCameraInterface == NULL)
    {
        terninalPrintf("CamTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pCameraInterface->initFunc(FALSE) == FALSE)
    {
        terninalPrintf("CamTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    vTaskDelay(2000/portTICK_RATE_MS);
    //pCameraInterface->setPowerFunc(0,camStatus[0]);
    pCameraInterface->setPowerFunc(1,camStatus[0]);
    //end init//
    EPDDrawStringMax(FALSE,boolConvString[camStatus[0]],CAM_0_ON_OFF_X,CAM_0_ON_OFF_Y,FALSE);
    EPDDrawStringMax(TRUE ,boolConvString[camStatus[1]],CAM_1_ON_OFF_X,CAM_1_ON_OFF_Y,TRUE);
    //terninalPrintf("Now Power Status:('q' to exit)\n0->set Cam0\n1->set Cam1\n");
    terninalPrintf("Now Power Status:('q' to exit)\n1->set Cam1\n2->set Cam2\n");
    //terninalPrintf("Cam0[%s]  Cam1[%s]",boolConvString[camStatus[0]],boolConvString[camStatus[1]]);
    terninalPrintf("Cam1[%s]  Cam2[%s]",boolConvString[camStatus[0]],boolConvString[camStatus[1]]);
    while(1)
    {
        tmp=userResponseLoop();
        //setPrintfFlag(FALSE);
        if(tmp=='q')
        {
            break;
        }
        //else if(tmp=='0')
        else if(tmp=='1')
        {
            camStatus[0] =!camStatus[0];
            camStatus[1] = FALSE;
            //pCameraInterface->setPowerFunc(0,camStatus[0]);
            pCameraInterface->setPowerFunc(1,camStatus[0]);
        }
        else if(tmp=='2')
        //else if(tmp=='2')
        {
            camStatus[1] =!camStatus[1];
            camStatus[0] = FALSE;
            //pCameraInterface->setPowerFunc(1,camStatus[1]);
            pCameraInterface->setPowerFunc(0,camStatus[1]);
        }
        //terninalPrintf("\rCam0[%s]  Cam1[%s]",boolConvString[camStatus[0]],boolConvString[camStatus[1]]);
        terninalPrintf("\rCam1[%s]  Cam2[%s]",boolConvString[camStatus[0]],boolConvString[camStatus[1]]);
        EPDDrawString(FALSE,boolConvString[camStatus[0]],CAM_0_ON_OFF_X,CAM_0_ON_OFF_Y);
        EPDDrawString(TRUE ,boolConvString[camStatus[1]],CAM_1_ON_OFF_X,CAM_1_ON_OFF_Y);
        //setPrintfFlag(TRUE); 
    }
    return TRUE;
}

///////////////////////////RADAR Test/////////////////////////////////
/*Globa Val*/
#define RADAR_MSG_X 50  //80 //180
#define RADAR_MSG_Y 250
#define RADAR_INTERVAL 500  //400
static BOOL NEWradarSet(void* para1, void* para2)
{
    int featureValue;
    uint8_t RadarData[22];
    uint8_t ReadCmd[24]   = {0x7A, 0xA7, 0x00, 0x18, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x1A, 0xD3, 0x3D};
                             
    uint8_t ResultCmd[24] = {0x7A, 0xA7, 0x00, 0x18, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x1B, 0xD3, 0x3D};
                             
    uint16_t   setIndex,setXval,setYval,setLval,setWval,setHval,setThetaval,setPhival,setLR,setFR;                              
    uint16_t   *setIndexPtr = para1;          
    //uint16_t   *setFRtypePtr = para2;                             
    static RadarInterface* pRadarInterface;
    pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);
    if(pRadarInterface == NULL)
    {
        terninalPrintf("NEWradarTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("NEWradarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    
    setIndex = *setIndexPtr;
    
    /*
    terninalPrintf("Please enter radar index in decimal.(1~2)\r\n");
    terninalPrintf("Enter number is ");
    setIndex = SetValue(1);
    setIndex--;
    if((setIndex == 0xffff) || ((setIndex != 0)&&(setIndex != 1)))
        return TEST_FALSE;
    */
    terninalPrintf("\r\nPlease enter X parameters in decimal.(unit:cm)\r\n");
    terninalPrintf("Enter number is ");
    setXval = SetValue(5);
    if(setXval == 0xffff)
        return TEST_FALSE;
    ResultCmd[6] = setXval >> 8;
    ResultCmd[7] = setXval & 0x00FF;
    
    terninalPrintf("\r\nPlease enter Y parameters in decimal.(unit:cm)\r\n");
    terninalPrintf("Enter number is ");
    setYval = SetValue(5);
    if(setYval == 0xffff)
        return TEST_FALSE;
    ResultCmd[8] = setYval >> 8;
    ResultCmd[9] = setYval & 0x00FF;   
    
    
    terninalPrintf("\r\nPlease enter L parameters in decimal.(unit:cm)\r\n");
    terninalPrintf("Enter number is ");
    setLval = SetValue(5);
    if(setLval == 0xffff)
        return TEST_FALSE;
    ResultCmd[10] = setLval >> 8;
    ResultCmd[11] = setLval & 0x00FF;  
    
    terninalPrintf("\r\nPlease enter W parameters in decimal.(unit:cm)\r\n");
    terninalPrintf("Enter number is ");
    setWval = SetValue(5);
    if(setWval == 0xffff)
        return TEST_FALSE;
    ResultCmd[12] = setWval >> 8;
    ResultCmd[13] = setWval & 0x00FF;  
    
    terninalPrintf("\r\nPlease enter H parameters in decimal.(unit:cm)\r\n");
    terninalPrintf("Enter number is ");
    setHval = SetValue(5);
    if(setHval == 0xffff)
        return TEST_FALSE;
    ResultCmd[14] = setHval >> 8;
    ResultCmd[15] = setHval & 0x00FF;  
    
    terninalPrintf("\r\nPlease enter Theta parameters in decimal.(unit:degree)\r\n");
    terninalPrintf("Enter number is ");
    setThetaval = SetValue(2);
    if(setThetaval == 0xffff)
        return TEST_FALSE;
    ResultCmd[16] = setThetaval;
    //ResultCmd[16] = setThetaval >> 8;
    //ResultCmd[17] = setThetaval & 0x00FF;  
    
    terninalPrintf("\r\nPlease enter Phi parameters in decimal.(unit:degree)\r\n");
    terninalPrintf("Enter number is ");
    setPhival = SetValue(2);
    //terninalPrintf("\r\n");
    if(setPhival == 0xffff)
        return TEST_FALSE;
    ResultCmd[17] = setPhival;
    //ResultCmd[18] = setPhival >> 8;
    //ResultCmd[19] = setPhival & 0x00FF;
    
    terninalPrintf("\r\nPlease enter L/R parameters in decimal.(0:right  1:left)\r\n");
    terninalPrintf("Enter number is ");
    setLR = SetValue(1);
    terninalPrintf("\r\n");
    if( (setLR == 0xffff) || (setLR > 1) )
        return TEST_FALSE;
    ResultCmd[18] = setLR;
    /*
    terninalPrintf("\r\nPlease enter F/R parameters in decimal.(0:F type  1:R type)\r\n");
    terninalPrintf("Enter number is ");
    setFR = SetValue(1);
    terninalPrintf("\r\n");
    if( (setFR == 0xffff) || (setFR > 1))
        return TEST_FALSE;
    //ResultCmd[18] = setFR;
    */
    //*setFRtypePtr = setFR;
    
    
    pRadarInterface->setPowerStatusFunc(setIndex,TRUE);
    
    featureValue = pRadarInterface->RadarResultFunc(setIndex, 0x02,ResultCmd,RadarData);
    
    pRadarInterface->setPowerStatusFunc(setIndex,FALSE);
    
    if((featureValue == TRUE) && (memcmp(RadarData + 8,ResultCmd + 6,13) == 0) )
    {
        terninalPrintf("Set radar%d success.\r\n",setIndex + 1);
        return TRUE;
    }
    else
    {
        return TEST_FALSE; 
    }
}

static BOOL NEWradarSetDefault(void* para1, void* para2)
{
    int featureValue;
    uint8_t RadarData[22];
    uint8_t ReadCmd[24]   = {0x7A, 0xA7, 0x00, 0x18, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x1A, 0xD3, 0x3D};
                             
    uint8_t ResultCmd[24] = {0x7A, 0xA7, 0x00, 0x18, 0x02, 0x01, 0x00, 0x1E, 0x00, 0x1E, 
                             0x00, 0xC8, 0x00, 0xC8, 0x00, 0x73, 0x00, 0x2D, 0x00, 0x05,
                             0x02, 0x8C, 0xD3, 0x3D};
                             
    uint16_t   setIndex,setXval,setYval,setLval,setWval,setHval,setFR,rdIndex;    
    uint8_t    setThetaval,setPhival; //,setLR;
    uint16_t   *setIndexPtr = para1;         
    //uint16_t   *setFRtypePtr = para2;                     
    static RadarInterface* pRadarInterface;
    pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);
    if(pRadarInterface == NULL)
    {
        terninalPrintf("NEWradarTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("NEWradarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    
    setIndex = *setIndexPtr;
    
    /*
    terninalPrintf("Please enter radar index in decimal.(1~2)\r\n");
    terninalPrintf("Enter number is ");
    setIndex = SetValue(1);
    setIndex--;
    if((setIndex == 0xffff) || ((setIndex != 0)&&(setIndex != 1)))
        return TEST_FALSE;
    */

    setXval = 30;
    ResultCmd[6] = setXval >> 8;
    ResultCmd[7] = setXval & 0x00FF;
    
    setYval = 30;
    ResultCmd[8] = setYval >> 8;
    ResultCmd[9] = setYval & 0x00FF;   
    
    setLval = 200;
    ResultCmd[10] = setLval >> 8;
    ResultCmd[11] = setLval & 0x00FF;  
    
    setWval = 200;
    ResultCmd[12] = setWval >> 8;
    ResultCmd[13] = setWval & 0x00FF;  
    
    setHval = 115;
    ResultCmd[14] = setHval >> 8;
    ResultCmd[15] = setHval & 0x00FF;  
    
    setThetaval = 45;
    ResultCmd[16] = setThetaval;
    //ResultCmd[16] = setThetaval >> 8;
    //ResultCmd[17] = setThetaval & 0x00FF;  
    
    setPhival = 5;
    ResultCmd[17] = setPhival;
    //ResultCmd[18] = setPhival >> 8;
    //ResultCmd[19] = setPhival & 0x00FF;
    
    
    terninalPrintf("\r\nPlease enter F/R parameters in decimal.(0:F type  1:R type)\r\n");
    terninalPrintf("Enter number is ");
    setFR = SetValue(1);
    terninalPrintf("\r\n");
    if(setFR == 0xffff)
        return TEST_FALSE;
    //ResultCmd[18] = setFR;
    
    if(setFR > 1)
    {
        terninalPrintf("input error.\r\n");
        return TEST_FALSE; 
    }
    
    //*setFRtypePtr = setFR;
    
    
    if(setIndex == 0) // EPM right(A) side radar
    {
        if(setFR == 0)  // F type
            ResultCmd[18] = 1;  // left radar
        else if(setFR == 1)  // R type
            ResultCmd[18] = 0;  // right radar
    }
    else if(setIndex == 1) // EPM left(B) side radar
    {
        if(setFR == 0)  // F type
            ResultCmd[18] = 0;  // right radar
        else if(setFR == 1)  // R type
            ResultCmd[18] = 1;  // left radar
    }
    /*
    if(ResultCmd[18] == 1)   // left radar
        rdIndex = 0;
    else if(ResultCmd[18] == 0) // right radar
        rdIndex = 1;
    */
    pRadarInterface->setPowerStatusFunc(setIndex,TRUE);
    
    featureValue = pRadarInterface->RadarResultFunc(setIndex, 0x02,ResultCmd,RadarData);
    
    pRadarInterface->setPowerStatusFunc(setIndex,FALSE);
    
    if((featureValue == TRUE) && (memcmp(RadarData + 8,ResultCmd + 6,13) == 0) )
    {
        terninalPrintf("Set radar%d success.\r\n",setIndex + 1);
        return TRUE;
    }
    else
    {
        return TEST_FALSE; 
    }
}
/*
void UART3Callback(void)
{
    terninalPrintf("radar33 <= ");
    while(!(inpw(REG_UART3_FSR) & (1 << 14)))
        terninalPrintf("%02x ",inpw(REG_UART3_RBR));    
    terninalPrintf("\r\n");
}

void UART7Callback(void)
{
    terninalPrintf("radar77 <= ");
    while(!(inpw(REG_UART7_FSR) & (1 << 14)))
        terninalPrintf("%02x ",inpw(REG_UART7_RBR));    
    terninalPrintf("\r\n");
}
*/
static BOOL NEWradarTest(void* para1, void* para2)
{
    int featureValue;
    uint8_t RadarData[22];
    
    uint8_t VersionCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D};
    uint8_t ResultCmd[24] = {0x7A, 0xA7, 0x00, 0x18, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x1A, 0xD3, 0x3D};
    
    
    char VerStr[2][20];
    char NumStr[15][10]; //NumStr[11][10];
    //uint16_t *FRtypePtr = para2;              
    int k;
    //uint8_t FRtype = 0;  // default F type     
    //uint8_t tempLRvalue[2];                   
    static RadarInterface* pRadarInterface;
    pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);
    if(pRadarInterface == NULL)
    {
        terninalPrintf("NEWradarTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("NEWradarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    pRadarInterface->setPowerStatusFunc(0,TRUE);
    pRadarInterface->setPowerStatusFunc(1,TRUE);
    
    //UART3_EnableINT(UART3Callback);
    //UART7_EnableINT(UART7Callback);
    
    //vTaskDelay(10000/portTICK_RATE_MS);
    /*
    EPDDrawString(FALSE,"Empty",15+10*25,100+(44*1));
    EPDDrawString(FALSE,"Empty",15+6*25,100+(44*2));
    EPDDrawString(FALSE,"Active",15+8*25,100+(44*3));
    EPDDrawString(FALSE,"Empty",15+8*25,100+(44*4));
    EPDDrawString(FALSE,"1000",15+7*25,100+(44*5));
    EPDDrawString(FALSE,"1000",15+15*25,100+(44*5));
    EPDDrawString(FALSE,"1000",15+7*25,100+(44*6));
    EPDDrawString(FALSE,"1000",15+15*25,100+(44*6));
    EPDDrawString(FALSE,"1000",15+8*25,100+(44*7));
    EPDDrawString(FALSE,"10000",15+8*25,100+(44*8));
    EPDDrawString(FALSE,"10000",15+8*25,100+(44*9));
    EPDDrawString(FALSE,"10000",15+8*25,100+(44*10));
    EPDDrawString(FALSE,"10000",15+8*25,100+(44*11));
    EPDDrawString(FALSE,"10000",25+12*25,100+(44*12));
    EPDDrawString(TRUE,"10000",25+10*25,100+(44*13));
    */
    /*
    //terninalPrintf("*FRtypePtr = %d\r\n",*FRtypePtr);
    if(*FRtypePtr == 0) // F type
        EPDDrawString(FALSE,"F-type",90+25*20,28);
    else if(*FRtypePtr == 1) // R type
        EPDDrawString(FALSE,"R-type",90+25*20,28);
    */
    for(int j = 0; j<2; j++)
    {
        if(pRadarInterface->RadarResultFunc(j, 0x00,VersionCmd,RadarData) == TRUE)
        {
            terninalPrintf("Radar%d Version : Ver.%d.%d.%d.%d\r\n",j+1,RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
            sprintf(VerStr[j],"%d.%d.%d.%d",RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
            EPDDrawString(FALSE,VerStr[j],15+8*25+515*j,100+(44*0));
        }
    }
    pRadarInterface->RadarCreateTaskFunc(); 
    //vTaskDelay(5000/portTICK_RATE_MS);
    pRadarInterface->RadarResultFunc(0, 0x02,ResultCmd,RadarData);
    pRadarInterface->RadarResultFunc(1, 0x02,ResultCmd,RadarData);
    //pRadarInterface->RadarResultFunc(0, 0x02,ResultCmd,RadarData);
    //pRadarInterface->RadarResultFunc(1, 0x02,ResultCmd,RadarData);
    //vTaskDelay(1000/portTICK_RATE_MS);
    while(1)
    {
        
        for(int i = 0; i<2; i++)
        {
            //featureValue = pRadarInterface->checkFeaturnFunc(i, NULL, NULL, NULL, NULL);
            //featureValue = pRadarInterface->RadarResultFunc(i, 0x02,ResultCmd,RadarData);
            featureValue = pRadarInterface->RadarResultElite(i, 0x02,ResultCmd,RadarData);
            /*
            terninalPrintf("featureValue = %08x \r\n",featureValue);
            terninalPrintf("RadarData =              ");
            for(int j = 0; j<sizeof(RadarData); j++)
                terninalPrintf("%02x ",RadarData[j]);
            terninalPrintf("\r\n");
            */
            if(featureValue == TRUE)
            {
                terninalPrintf("----------Radar%dResult----------\r\n",i+1);
                
                //k = (i + *FRtypePtr)%2;
                k = i;
                //terninalPrintf("Object Material: ");
                terninalPrintf("Object Material: %d\r\n",RadarData[0]);
                EPDDrawString(FALSE,"        ",15+10*25+515*k,100+(44*1));
                sprintf(NumStr[11],"%d",RadarData[0]);
                EPDDrawString(FALSE,NumStr[11],15+10*25+515*k,100+(44*1));
                /*
                switch(RadarData[0]) 
                {
                    case 0x00:
                        terninalPrintf("Empty\r\n");
                        EPDDrawString(FALSE,"Empty",15+10*25+515*i,100+(44*1));
                        break;
                    case 0x01:
                        terninalPrintf("Metal\r\n");
                        EPDDrawString(FALSE,"Metal",15+10*25+515*i,100+(44*1));
                        break;
                    case 0x02:
                        terninalPrintf("Wood\r\n");
                        EPDDrawString(FALSE,"Wood",15+10*25+515*i,100+(44*1));
                        break;
                    case 0x03:
                        terninalPrintf("Water\r\n");
                        EPDDrawString(FALSE,"Water",15+10*25+515*i,100+(44*1));
                        break;
                    case 0x04:
                        terninalPrintf("Carton\r\n");
                        EPDDrawString(FALSE,"Carton",15+10*25+515*i,100+(44*1));
                        break;
                    case 0x05:
                        terninalPrintf("Others\r\n");
                        EPDDrawString(FALSE,"Others",15+10*25+515*i,100+(44*1));
                        break;
                    default:
                        terninalPrintf("Unknown\r\n");
                        EPDDrawString(FALSE,"Unknown",15+10*25+515*i,100+(44*1));
                        break;
                }
                */
                //terninalPrintf("Object type: ");
                terninalPrintf("Object type: %d\r\n",RadarData[1]);
                EPDDrawString(FALSE,"            ",15+6*25+515*k,100+(44*2));
                sprintf(NumStr[12],"%d",RadarData[1]);
                EPDDrawString(FALSE,NumStr[12],15+6*25+515*k,100+(44*2));
                
                /*
                switch(RadarData[1]) 
                {
                    case 0x00:
                        terninalPrintf("Empty\r\n");
                        EPDDrawString(FALSE,"Empty",15+6*25+515*i,100+(44*2));
                        break;
                    case 0x01:
                        terninalPrintf("Car\r\n");
                        EPDDrawString(FALSE,"Car",15+6*25+515*i,100+(44*2));
                        break;
                    case 0x02:
                        terninalPrintf("Motocycle\r\n");
                        EPDDrawString(FALSE,"Motocycle",15+6*25+515*i,100+(44*2));
                        break;
                    case 0x03:
                        terninalPrintf("Scooter\r\n");
                        EPDDrawString(FALSE,"Scooter",15+6*25+515*i,100+(44*2));
                        break;
                    case 0x04:
                        terninalPrintf("Bike\r\n");
                        EPDDrawString(FALSE,"Bike",15+6*25+515*i,100+(44*2));
                        break;
                    case 0x05:
                        terninalPrintf("Human\r\n");
                        EPDDrawString(FALSE,"Human",15+6*25+515*i,100+(44*2));
                        break;
                    case 0x06:
                        terninalPrintf("Obstacle\r\n");
                        EPDDrawString(FALSE,"Obstacle",15+6*25+515*i,100+(44*2));
                        break;
                    case 0x0F:
                        terninalPrintf("Radar obscured\r\n");
                        EPDDrawString(FALSE,"Radar obscured",15+6*25+515*i,100+(44*2));
                        break;
                    default:
                        terninalPrintf("Unknown\r\n");
                        EPDDrawString(FALSE,"Unknown",15+6*25+515*i,100+(44*2));
                        break;
                }
                */
                //terninalPrintf("Radar Status: ");
                terninalPrintf("Radar Status: %d\r\n",RadarData[2]);
                EPDDrawString(FALSE,"         ",15+8*25+515*k,100+(44*3));
                sprintf(NumStr[13],"%d",RadarData[2]);
                EPDDrawString(FALSE,NumStr[13],15+8*25+515*k,100+(44*3));
                /*
                switch(RadarData[2]) 
                {
                    case 0x00:
                        terninalPrintf("Active\r\n");
                        EPDDrawString(FALSE,"Active",15+8*25+515*i,100+(44*3));
                        break;
                    case 0xFF:
                        terninalPrintf("Fail\r\n");
                        EPDDrawString(FALSE,"Fail",15+8*25+515*i,100+(44*3));
                        break;
                    default:
                        terninalPrintf("Unknown\r\n");
                        EPDDrawString(FALSE,"Unknown",15+8*25+515*i,100+(44*3));
                        break;
                }
                */
                
                //terninalPrintf("Identification result: ");
                terninalPrintf("Identification result: %d\r\n",RadarData[3]);
                EPDDrawString(FALSE,"          ",15+8*25+515*k,100+(44*4));
                sprintf(NumStr[14],"%d",RadarData[3]);
                EPDDrawString(FALSE,NumStr[14],15+8*25+515*k,100+(44*4));
                /*
                switch(RadarData[3]) 
                {
                    case 0x00:
                        terninalPrintf("Empty\r\n");
                        EPDDrawString(FALSE,"Empty",15+8*25+515*i,100+(44*4));
                        break;
                    case 0x01:
                        terninalPrintf("Parking (near)\r\n");
                        EPDDrawString(FALSE,"Parking (near)",15+8*25+515*i,100+(44*4));
                        break;
                    case 0x02:
                        terninalPrintf("Parking (away)\r\n");
                        EPDDrawString(FALSE,"Parking (away)",15+8*25+515*i,100+(44*4));
                        break;
                    case 0x03:
                        terninalPrintf("Parking (idling)\r\n");
                        EPDDrawString(FALSE,"Parking (idling)",15+8*25+515*i,100+(44*4));
                        break;
                    case 0x04:
                        terninalPrintf("Parking (static)\r\n");
                        EPDDrawString(FALSE,"Parking (static)",15+8*25+515*i,100+(44*4));
                        break;
                    case 0x05:
                        terninalPrintf("Occupying parking lot\r\n");
                        EPDDrawString(FALSE,"Occupying parking lot",15+8*25+515*i,100+(44*4));
                        break;
                    case 0x06:
                        terninalPrintf("Reserved\r\n");
                        EPDDrawString(FALSE,"Reserved",15+8*25+515*i,100+(44*4));
                        break;
                    case 0x0F:
                        terninalPrintf("Radar obscured\r\n");
                        EPDDrawString(FALSE,"Radar obscured",15+8*25+515*i,100+(44*4));
                        break;
                    default:
                        terninalPrintf("Unknown\r\n");
                        EPDDrawString(FALSE,"Unknown",15+8*25+515*i,100+(44*4));
                        break;
                }
                */

                terninalPrintf("Object Position (x): %d\r\n",RadarData[4]*10);
                terninalPrintf("Object Position (y): %d\r\n",RadarData[5]*10);
                terninalPrintf("Object Position (z): %d\r\n",RadarData[6]*10);
                terninalPrintf("Object Distance (R): %d\r\n",RadarData[7]*10);
                
                terninalPrintf("X: %d\r\n",     RadarData[8]<<8 | RadarData[9]);
                terninalPrintf("Y: %d\r\n",     RadarData[10]<<8 | RadarData[11]);
                terninalPrintf("L: %d\r\n",     RadarData[12]<<8 | RadarData[13]);
                terninalPrintf("W: %d\r\n",     RadarData[14]<<8 | RadarData[15]);
                terninalPrintf("H: %d\r\n",     RadarData[16]<<8 | RadarData[17]);
                terninalPrintf("theta: %d\r\n", RadarData[18]);
                terninalPrintf("phi: %d\r\n",   RadarData[19]);
                terninalPrintf("L/R: %d\r\n",   RadarData[20]);
                //terninalPrintf("theta: %d\r\n", RadarData[18]<<8 | RadarData[19]);
                //terninalPrintf("phi: %d\r\n",   RadarData[20]<<8 | RadarData[21]);
                terninalPrintf("--------------------------------\r\n");
                
                
                
                
                memset(NumStr,0x00,sizeof(NumStr));
                
                sprintf(NumStr[0],"%d",RadarData[4]*10);
                sprintf(NumStr[1],"%d",RadarData[5]*10);
                sprintf(NumStr[2],"%d",RadarData[6]*10);
                sprintf(NumStr[3],"%d",RadarData[7]*10);
                
                sprintf(NumStr[4], "%d",RadarData[8]<<8  | RadarData[9]);
                sprintf(NumStr[5], "%d",RadarData[10]<<8 | RadarData[11]);
                sprintf(NumStr[6], "%d",RadarData[12]<<8 | RadarData[13]);
                sprintf(NumStr[7], "%d",RadarData[14]<<8 | RadarData[15]);
                sprintf(NumStr[8], "%d",RadarData[16]<<8 | RadarData[17]);
                sprintf(NumStr[9], "%d",RadarData[18]);
                sprintf(NumStr[10],"%d",RadarData[19]);
                sprintf(NumStr[11],"%d",RadarData[20]);
                //sprintf(NumStr[9], "%d",RadarData[18]<<8 | RadarData[19]);
                //sprintf(NumStr[10],"%d",RadarData[20]<<8 | RadarData[21]);
                
                
                EPDDrawString(FALSE,"    ",15+7*25+515*k,100+(44*5));
                EPDDrawString(FALSE,"    ",15+15*25+515*k,100+(44*5));
                EPDDrawString(FALSE,"    ",15+7*25+515*k,100+(44*6));
                EPDDrawString(FALSE,"    ",15+15*25+515*k,100+(44*6));
                EPDDrawString(FALSE,"     ",15+8*25+515*k,100+(44*7));
                EPDDrawString(FALSE,"     ",15+8*25+515*k,100+(44*8));
                EPDDrawString(FALSE,"     ",15+8*25+515*k,100+(44*9));
                EPDDrawString(FALSE,"     ",15+8*25+515*k,100+(44*10));
                EPDDrawString(FALSE,"     ",15+8*25+515*k,100+(44*11));
                EPDDrawString(FALSE,"     ",25+12*25+515*k,100+(44*12));
                EPDDrawString(FALSE,"     ",25+10*25+515*k,100+(44*13));
                EPDDrawString(FALSE," ",10*25+515*k,100+(44*14));
                          
                
                EPDDrawString(FALSE,NumStr[0],15+7*25+515*k,100+(44*5));
                EPDDrawString(FALSE,NumStr[1],15+15*25+515*k,100+(44*5));
                EPDDrawString(FALSE,NumStr[2],15+7*25+515*k,100+(44*6));
                EPDDrawString(FALSE,NumStr[3],15+15*25+515*k,100+(44*6));
                EPDDrawString(FALSE,NumStr[4],15+8*25+515*k,100+(44*7));
                EPDDrawString(FALSE,NumStr[5],15+8*25+515*k,100+(44*8));
                EPDDrawString(FALSE,NumStr[6],15+8*25+515*k,100+(44*9));
                EPDDrawString(FALSE,NumStr[7],15+8*25+515*k,100+(44*10));
                EPDDrawString(FALSE,NumStr[8],15+8*25+515*k,100+(44*11));
                EPDDrawString(FALSE,NumStr[9],25+12*25+515*k,100+(44*12));
                EPDDrawString(FALSE,NumStr[10],25+10*25+515*k,100+(44*13));
                EPDDrawString(TRUE,NumStr[11],10*25+515*k,100+(44*14));
                
            }
            else if(featureValue == FALSE)
                vTaskDelay(2000/portTICK_RATE_MS);
            else if( featureValue == 0x10)  // UART_FORMAT_ERR
            {
                if(pRadarInterface->setPowerStatusFunc != NULL) {
                    pRadarInterface->RadarinitBurningFunc(i);
                    pRadarInterface->setPowerStatusFunc(i, FALSE);
                    vTaskDelay(500 / portTICK_RATE_MS);
                    pRadarInterface->setPowerStatusFunc(i, TRUE);
                    vTaskDelay(500 / portTICK_RATE_MS);
                }
            }
            
            
            if(userResponse()=='q')
            {
                //UART3_DisableINT();
                //UART7_DisableINT();
                
                pRadarInterface->RadarDeleteTaskFunc();
                pRadarInterface->setPowerStatusFunc(0,FALSE);
                pRadarInterface->setPowerStatusFunc(1,FALSE);
                return TRUE;
            }
        }
    }
    
}

static BOOL radarTest(void* para1, void* para2)
{
    guiManagerShowScreen(GUI_RADAR_ID,GUI_REDRAW_PARA_REFRESH,0,0);
    char epdMessage[64];
    static RadarInterface* pRadarInterface;
    BOOL changeFlag;
    int dist;
    int power;
    int occupiedType;
    int distValue[2];
    
    uint16_t lidarDistValue[2];
    uint16_t radarDistValue[2];
    
    char lidarDistString[2][20];
    char radarDistString[2][20];
    
    char distStringBuff[2][20];
    terninalPrintf("!!! lidarTest !!!\r\n");
    //setPrintfFlag(FALSE);
    pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);
    if(pRadarInterface == NULL)
    {
        terninalPrintf("lidarTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("lidarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    terninalPrintf("Press 'q' to Exit\r\n");
    EPDDrawString(TRUE,"Testing   ",RADAR_MSG_X,RADAR_MSG_Y);
    pRadarInterface->setPowerStatusFunc(0,TRUE);
    pRadarInterface->setPowerStatusFunc(1,TRUE);
    vTaskDelay(4000/portTICK_RATE_MS);
    
    /*
    char temp1String[50] ;
    char tempRadarVersionString[50];
    
    for(int i = 0; i<2; i++)
    {
        pRadarInterface -> readQueryVersionString(i, tempRadarVersionString);
        terninalPrintf("\r");
        terninalPrintf("Radar%d Version = %s\r\n",i+1,tempRadarVersionString);
        sprintf(templastString,"Radar%d Version:%s",i+1,tempRadarVersionString);       
        EPDDrawString(TRUE,templastString,50,100+(44*(i+10)));        
        vTaskDelay(1000/portTICK_RATE_MS);
    }  */
    
    char temp1String[50] ;
    char temp2String[50] ;
    char tempRadar1VersionString[50] ;
    char tempRadar2VersionString[50] ;
    int retry1 = 2; 
    int retry2 = 2;
    
    
    while(retry1 > 0)
    {
        if(pRadarInterface -> readQueryVersionString(0, tempRadar1VersionString))
            break;
        retry1--;
    }
    
    if(retry1 > 0)
    {
        
        terninalPrintf("Radar1 Version = %s\r\n",tempRadar1VersionString);
        sprintf(temp1String,"Radar1 Version:%s",tempRadar1VersionString);       
        EPDDrawString(TRUE,temp1String,50,100+(44*(10)));        
        
    }
    else
    {
        sprintf(temp1String,"Radar1 Version:Error");       
        EPDDrawString(TRUE,temp1String,50,100+(44*(10)));   
    }
    
    
    
    
    while(retry2 > 0)
    {
        if(pRadarInterface -> readQueryVersionString(1, tempRadar2VersionString))
            break;
        retry2--;
    }
    
    if(retry2 > 0)
    {
        terninalPrintf("Radar2 Version = %s\r\n",tempRadar2VersionString);
        sprintf(temp2String,"Radar2 Version:%s",tempRadar2VersionString);       
        EPDDrawString(TRUE,temp2String,50,100+(44*(11))); 
    }
    else
    {
        sprintf(temp2String,"Radar2 Version:Error");       
        EPDDrawString(TRUE,temp2String,50,100+(44*(11)));   
    }
    
    
    
    while(1)
    {
        for(int i = 0; i<2; i++)
        {
            EPDDrawString(TRUE,"Testing   ",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
            int featureValue = pRadarInterface->checkFeaturnFunc(i, &changeFlag, &dist, &power, &occupiedType);
            switch(featureValue)
            {
                case RADAR_FEATURE_OCCUPIED:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_OCCUPIED (%d)!!\n", i+1, dist);
                    break;
                case RADAR_FEATURE_VACUUM:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_VACUUM (%d)!!\n", i+1, dist);
                    break;
                case RADAR_FEATURE_OCCUPIED_UN_STABLED:
                    sprintf(epdMessage, "%d %d        "        , dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_UN_STABLED (%d )!!\n", i+1, dist);
                    break; 
                case RADAR_FEATURE_VACUUM_UN_STABLED:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_UN_STABLED (%d )!!\n", i+1, dist);
                    break; 
                case RADAR_FEATURE_IGNORE:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_IGNORE (%d )!!\n", i+1, dist);
                    break; 
                
                case RADAR_FEATURE_OCCUPIED_LIDAR_FAIL:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_OCCUPIED_LIDAR_FAIL (%d)!!\n", i+1, dist);
                    break;
                case RADAR_FEATURE_VACUUM_LIDAR_FAIL:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_VACUUM_LIDAR_FAIL (%d)!!\n", i+1, dist);
                    break;
                case RADAR_FEATURE_OCCUPIED_UN_STABLED_LIDAR_FAIL:
                    sprintf(epdMessage, "%d %d        "        , dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_UN_STABLED_LIDAR_FAIL (%d )!!\n", i+1, dist);
                    break; 
                case RADAR_FEATURE_VACUUM_UN_STABLED_LIDAR_FAIL:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_UN_STABLED_LIDAR_FAIL (%d )!!\n", i+1, dist);
                    break; 
                case RADAR_FEATURE_IGNORE_LIDAR_FAIL:
                    sprintf(epdMessage, "%d %d        ", dist, power);
                    EPDDrawString(TRUE,epdMessage,RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  RADAR_FEATURE_IGNORE_LIDAR_FAIL (%d )!!\n", i+1, dist);
                    break; 
                
                default:
                    EPDDrawString(TRUE,"FAIL        ",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y);
                    terninalPrintf("[RADAR]%d ->  TIMEOUT!! featureValue=%d\n", i+1,featureValue);
                    break;
            }
            
            EPDDrawString(FALSE,"                  ",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+50);
            EPDDrawString(FALSE,"                  ",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+100);
            if(pRadarInterface->RecentDistValueFunc(i,&lidarDistValue[i],&radarDistValue[i]) == TRUE)
            {

                sprintf(lidarDistString[i],"lidarDist:%d",lidarDistValue[i]);
                EPDDrawString(FALSE,lidarDistString[i],RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+50);
                sprintf(radarDistString[i],"radarDist:%d",radarDistValue[i]);
                EPDDrawString(TRUE,radarDistString[i],RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+100);
                
            }
            else
            {
                sprintf(lidarDistString[i],"lidarDist:error");
                EPDDrawString(FALSE,lidarDistString[i],RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+50);
                sprintf(radarDistString[i],"radarDist:error");
                EPDDrawString(TRUE,radarDistString[i],RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+100);
            }
            
            EPDDrawString(FALSE,"                  ",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+150);
            EPDDrawString(FALSE,"                  ",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+200); 
            
            if(pRadarInterface->readDistValueFunc(i,&distValue[i]) == TRUE)
            {
                sprintf(distStringBuff[i],"CaliDist:%d",distValue[i]);
                //EPDDrawString(TRUE,distStringBuff[i],RADAR_MSG_X+(i*400),RADAR_MSG_Y+50);
                EPDDrawString(TRUE,distStringBuff[i],RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+150);
            }
            else
            {
                if(i == 0)
                    //EPDDrawString(FALSE,"Lidar1 not",RADAR_MSG_X+(i*400),RADAR_MSG_Y+50);
                    EPDDrawString(FALSE,"Lidar1 not",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+150);
                else if(i == 1)
                    //EPDDrawString(FALSE,"Lidar2 not",RADAR_MSG_X+(i*400),RADAR_MSG_Y+50);
                    EPDDrawString(FALSE,"Lidar2 not",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+150);
                //EPDDrawString(TRUE,"calibrated",RADAR_MSG_X+(i*400),RADAR_MSG_Y+100);
                EPDDrawString(TRUE,"calibrated",RADAR_MSG_X+(i*RADAR_INTERVAL),RADAR_MSG_Y+200);
            }
        
            
            
            if(userResponse()=='q')
            {
                pRadarInterface->setPowerStatusFunc(0,FALSE);
                pRadarInterface->setPowerStatusFunc(1,FALSE);
                return TRUE;
            }
        }
    }
}
#define RADAR_0_ON_OFF_X 480
#define RADAR_0_ON_OFF_Y 250
#define RADAR_1_ON_OFF_X 480
#define RADAR_1_ON_OFF_Y 300
static BOOL radarPowerSet(void* para1, void* para2)
{
static BOOL radarStatus[2]={TRUE,TRUE};
    BOOL changeFlag;
    char* boolConvString[2] = {" OFF "," ON  "};
    guiManagerShowScreen(GUI_RADAR_ID, GUI_REDRAW_PARA_REFRESH, 0, POWER_SET_MODE);
    static RadarInterface* pRadarInterface;
    char tmp;
    terninalPrintf("!!! radarPowerSet !!!\r\n");
    pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);
    if(pRadarInterface == NULL)
    {
        //terninalPrintf("radarTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        //terninalPrintf("radarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    vTaskDelay(2000/portTICK_RATE_MS);
    pRadarInterface->setPowerStatusFunc(0,radarStatus[0]);
    pRadarInterface->setPowerStatusFunc(1,radarStatus[1]);
    
    //terninalPrintf("Press...\r\n'0' set Radar0 to [%s]\r\n'1' set Radar1 to [%s]\r\n",boolConvString[!radarStatus[0]],boolConvString[!radarStatus[1]]);

//    while(1)
    {
        EPDDrawString(TRUE,"Checking",RADAR_0_ON_OFF_X,RADAR_0_ON_OFF_Y);
        if(pRadarInterface->checkFeaturnFunc(0, &changeFlag, NULL, NULL, NULL)==RADAR_FEATURE_FAIL)
        {
            EPDDrawString(TRUE,"      Error",RADAR_0_ON_OFF_X,RADAR_0_ON_OFF_Y);
            radarStatus[0] = FALSE;
        }
        else
        {
            EPDDrawString(TRUE,"           ",RADAR_0_ON_OFF_X,RADAR_0_ON_OFF_Y);
        }
        EPDDrawString(TRUE,"Checking",RADAR_1_ON_OFF_X,RADAR_1_ON_OFF_Y);
        if(pRadarInterface->checkFeaturnFunc(1, &changeFlag, NULL, NULL, NULL)==RADAR_FEATURE_FAIL)
        {
            EPDDrawString(TRUE,"      Error",RADAR_1_ON_OFF_X,RADAR_1_ON_OFF_Y);
            radarStatus[1] = FALSE;
        }
        else 
        {
            EPDDrawString(TRUE,"           ",RADAR_1_ON_OFF_X,RADAR_1_ON_OFF_Y);
        }
        //break;
        if(userResponse()=='q')
            return TRUE;
    }
    //terninalPrintf("Check:\r\nRadar0 [%s]\r\nRadar1 [%s]\r\n",radarStatus[0]?"OK":"ERROR",radarStatus[1]?"OK":"ERROR");
    terninalPrintf("Check:\r\nRadar1 [%s]\r\nRadar2 [%s]\r\n",radarStatus[0]?"OK":"ERROR",radarStatus[1]?"OK":"ERROR");
    EPDDrawString(FALSE,boolConvString[radarStatus[0]],RADAR_0_ON_OFF_X,RADAR_0_ON_OFF_Y);
    EPDDrawString(TRUE ,boolConvString[radarStatus[1]],RADAR_1_ON_OFF_X,RADAR_1_ON_OFF_Y);
    terninalPrintf("Now Power Status:('q' to exit)\r\n");
    //terninalPrintf("Radar0[%s]  Radar1[%s] ('0' set Radar0)('1' set Radar1)",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
    terninalPrintf("Radar1[%s]  Radar2[%s] ('1' set Radar1)('2' set Radar2)",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
    while(1)
    {
        tmp=userResponseLoop();
        //setPrintfFlag(FALSE);
        if(tmp=='q')
        {
            break;
        }
        //else if(tmp=='0')
        else if(tmp=='1')
        {
            radarStatus[0]=!(radarStatus[0]);
            //terninalPrintf("\rRadar0[%s]  Radar1[%s] ('0' set Radar0)('1' set Radar1)",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
            terninalPrintf("\rRadar1[%s]  Radar2[%s] ('1' set Radar1)('2' set Radar2)",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
            pRadarInterface->setPowerStatusFunc(0,radarStatus[0]);
        }
        //else if(tmp=='1')
        else if(tmp=='2')
        {
            radarStatus[1]=!(radarStatus[1]);
            //terninalPrintf("\rRadar0[%s]  Radar1[%s] ('0' set Radar0)('1' set Radar1)",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
            terninalPrintf("\rRadar1[%s]  Radar2[%s] ('1' set Radar1)('2' set Radar2)",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
            pRadarInterface->setPowerStatusFunc(1,radarStatus[1]);
        }
        EPDDrawString(FALSE,boolConvString[radarStatus[0]],RADAR_0_ON_OFF_X,RADAR_0_ON_OFF_Y);
        EPDDrawString(TRUE ,boolConvString[radarStatus[1]],RADAR_1_ON_OFF_X,RADAR_1_ON_OFF_Y);
        //setPrintfFlag(TRUE); 
    }
    return TRUE;
}

///////////////////////////LIDAR Test/////////////////////////////////
#define LIDAR_MSG_X 180
#define LIDAR_MSG_Y 250
static BOOL lidarTest(void* para1, void* para2)
{
    guiManagerShowScreen(GUI_LIDAR_ID,GUI_REDRAW_PARA_REFRESH,0,0);
    char epdMessage[64];
    static RadarInterface* pRadarInterface;
    BOOL changeFlag;
    int dist;
    int power;
    int occupiedType;
    terninalPrintf("!!! lidarTest !!!\r\n");
    //setPrintfFlag(FALSE);
    pRadarInterface = RadarGetInterface(LIDAR_AV_DESIGN_INTERFACE_INDEX);
    if(pRadarInterface == NULL)
    {
        terninalPrintf("lidarTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("lidarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    terninalPrintf("Press 'q' to Exit\r\n");
    EPDDrawString(TRUE,"Testing   ",LIDAR_MSG_X,LIDAR_MSG_Y);
    pRadarInterface->setPowerStatusFunc(0,TRUE);
    pRadarInterface->setPowerStatusFunc(1,TRUE);
    while(1)
    {
        for(int i = 0; i<2; i++)
        {
            EPDDrawString(TRUE,"Testing   ",LIDAR_MSG_X+(i*400),LIDAR_MSG_Y);
            int featureValue = pRadarInterface->checkFeaturnFunc(i, &changeFlag, &dist, &power, &occupiedType);
            switch(featureValue)
            {
                 case LIDAR_FEATURE_OCCUPIED:
                    sprintf(epdMessage, "%dcm, %d", dist, power);
                    EPDDrawString(TRUE,epdMessage,LIDAR_MSG_X+(i*400),LIDAR_MSG_Y);
                    terninalPrintf("[LIDAR]%d ->  RADAR_FEATURE_OCCUPIED (%d cm)!!\n", i, dist);
                    break;
                 
                case LIDAR_FEATURE_VACUUM:
                    sprintf(epdMessage, "%dcm, %d", dist, power);
                    EPDDrawString(TRUE,epdMessage,LIDAR_MSG_X+(i*400),LIDAR_MSG_Y);
                    terninalPrintf("[LIDAR]%d ->  RADAR_FEATURE_VACUUM (%d cm)!!\n", i, dist);
                    break;
                
                case LIDAR_FEATURE_UN_STABLED:
                    sprintf(epdMessage, "%dcm, %d", dist, power);
                    EPDDrawString(TRUE,epdMessage,LIDAR_MSG_X+(i*400),LIDAR_MSG_Y);
                    terninalPrintf("[LIDAR]%d ->  RADAR_FEATURE_UN_STABLED (%d cm)!!\n", i, dist);
                    break;
                
                default:
                    //EPDDrawString(TRUE,"FAIL        ",LIDAR_MSG_X+(i*400),LIDAR_MSG_Y);
                    terninalPrintf("[LIDAR]%d ->  TIMEOUT!!\n", i);
                    break;
            }
            if(userResponse()=='q'){
                pRadarInterface->setPowerStatusFunc(0,FALSE);
                pRadarInterface->setPowerStatusFunc(1,FALSE);
                return TRUE;
            }
        }
    }
}
#define LIDAR_0_ON_OFF_X 480
#define LIDAR_0_ON_OFF_Y 250
#define LIDAR_1_ON_OFF_X 480
#define LIDAR_1_ON_OFF_Y 300
static BOOL lidarPowerSet(void* para1, void* para2)
{
    static BOOL radarStatus[2]={FALSE,FALSE};
    BOOL changeFlag;
    char* boolConvString[2]={"OFF   ","ON    "};
    guiManagerShowScreen(GUI_LIDAR_ID, GUI_REDRAW_PARA_REFRESH, 0, POWER_SET_MODE);
    static RadarInterface* pRadarInterface;
    char tmp;
    terninalPrintf("!!! lidarPowerSet !!!\r\n");
    pRadarInterface = RadarGetInterface(LIDAR_AV_DESIGN_INTERFACE_INDEX);
    if(pRadarInterface == NULL)
    {
        terninalPrintf("lidarTest ERROR (pRadarInterface == NULL)!!\n");
        return FALSE;
    }
    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("lidarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    vTaskDelay(2000/portTICK_RATE_MS);
    pRadarInterface->setPowerStatusFunc(0,radarStatus[0]);
    pRadarInterface->setPowerStatusFunc(1,radarStatus[1]);
    terninalPrintf("Now Status:\r\nLidar0 [%s]\r\nLidar1 [%s]\r\n",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
    terninalPrintf("Press...\r\n'0' set Lidar0 to [%s]\r\n'1' set Lidar1 to [%s]\r\n",boolConvString[!radarStatus[0]],boolConvString[!radarStatus[1]]);
    #if(SUPPORT_HK_10_HW)
    #else
    while(1)
    {
        EPDDrawString(TRUE,"Wait    ",LIDAR_0_ON_OFF_X,LIDAR_0_ON_OFF_Y);
        if(pRadarInterface->checkFeaturnFunc(0, &changeFlag, NULL, NULL)==LIDAR_FEATURE_FAIL)
        {
            EPDDrawString(TRUE,"FAIL    ",LIDAR_0_ON_OFF_X,LIDAR_0_ON_OFF_Y);
        }
        else
        {
            EPDDrawString(TRUE,"        ",LIDAR_0_ON_OFF_X,LIDAR_0_ON_OFF_Y);
        }
        EPDDrawString(TRUE,"Wait    ",LIDAR_1_ON_OFF_X,LIDAR_1_ON_OFF_Y);
        if(pRadarInterface->checkFeaturnFunc(1, &changeFlag, NULL, NULL)==LIDAR_FEATURE_FAIL)
        {
            EPDDrawString(TRUE,"FAIL    ",LIDAR_1_ON_OFF_X,LIDAR_1_ON_OFF_Y);
        }
        else
        {
            EPDDrawString(TRUE,"        ",LIDAR_1_ON_OFF_X,LIDAR_1_ON_OFF_Y);
            break;
        }
        if(userResponse()=='q')
        {
            return TRUE;
        }
    }
    #endif
    EPDDrawString(FALSE,boolConvString[radarStatus[0]],LIDAR_0_ON_OFF_X,LIDAR_0_ON_OFF_Y);
    EPDDrawString(TRUE ,boolConvString[radarStatus[1]],LIDAR_1_ON_OFF_X,LIDAR_1_ON_OFF_Y);
    while(1)
    {
        tmp=userResponseLoop();
        setPrintfFlag(FALSE);
        if(tmp=='q')
        {
            break;
        }
        else if(tmp=='0')
        {
            radarStatus[0]=!(radarStatus[0]);
            pRadarInterface->setPowerStatusFunc(0,radarStatus[0]);
        }
        else if(tmp=='1')
        {
            radarStatus[1]=!(radarStatus[1]);
            pRadarInterface->setPowerStatusFunc(1,radarStatus[1]);
        }
        terninalPrintf("Now Status:\r\nLidar0 [%s]\r\nLidar1 [%s]\r\n",boolConvString[radarStatus[0]],boolConvString[radarStatus[1]]);
        EPDDrawString(FALSE,boolConvString[radarStatus[0]],LIDAR_0_ON_OFF_X,LIDAR_0_ON_OFF_Y);
        EPDDrawString(TRUE ,boolConvString[radarStatus[1]],LIDAR_1_ON_OFF_X,LIDAR_1_ON_OFF_Y);
        terninalPrintf("Press...\r\n'0' set Lidar0 to [%s]\r\n'1' set Lidar1 to [%s]\r\n",boolConvString[!radarStatus[0]],boolConvString[!radarStatus[1]]);
        setPrintfFlag(TRUE); 
    }
    return TRUE;
}

///////////////////////////Burn  Test/////////////////////////////////

//static int epdImageLoopId[5]={EPD_PICT_ALL_WHITE_INDEX, EPD_PICT_ALL_BLACK_INDEX, EPD_PICT_INDEX_INIT, EPD_PICT_INDEX_INIT_FAIL, EPD_PICT_INDEX_FILE_DOWNLOADING};

static int epdImageLoopId[5]={EPD_PICT_ALL_BLACK_INDEX, EPD_PICT_ALL_WHITE_INDEX, EPD_PICT_INDEX_INIT, EPD_PICT_INDEX_INIT_FAIL, EPD_PICT_INDEX_FILE_DOWNLOADING};
static BOOL epdBurningTest(void* para1, void* para2)
{    
    //setPrintfFlag(FALSE);
    //terninalPrintf("!!! epdImageLoopId !!!\r\n");
    //terninalPrintf("Press 'q' to exit\r\n");
    terninalPrintf("Press 'q' to exit\r\n");
    //guiManagerShowScreen(GUI_NULL_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
    //setPrintfFlag(FALSE);
    //EpdDrvInit(TRUE);
    //EPDSetBacklight(TRUE);
    //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_BLACK_INDEX,0,0);
    EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE);
    for(int i = 0; i<3;)
    {    
        //EPDShowBGScreen(epdImageLoopId[i], TRUE); 
        EPDShowBGScreenEx(epdImageLoopId[i],TRUE,500,500);
        //vTaskDelay(2000/portTICK_RATE_MS);
        vTaskDelay(1000/portTICK_RATE_MS);
        //if(i == 2)
        if(i == 1)
        {
            i = 0;
        }
        else
        {
            i++;
        }
        if(userResponse()=='q'){
            break;
        }
    }
    //guiManagerRefreshScreen();
    
    GuiManagerCleanMessage(GUI_REDRAW_PARA_CONTAIN);
    
    //EPDSetBacklight(FALSE);
    //setPrintfFlag(TRUE);
    return TRUE;
}
static BOOL calibrationConfig(void* para1, void* para2)
{
    
    char tempchar;
    static RadarInterface* pRadarInterface;
    BOOL changeFlag;
    int distValue[2];
    char distStringBuff[2][20];
    
    short m_MEMSx,m_MEMSy,m_MEMSz;
    char responsechar;
    
    for(int i = 0; ; i++)
    {//Dont Show Quit
        if(calibrationListItem[i].charItem == 0)
        {
            break;
        }
        terninalPrintf("    %c) [%s]\r\n", calibrationListItem[i].charItem, calibrationListItem[i].itemName);
        
    }
    guiManagerShowScreen(GUI_CALIBRATION_ID, GUI_REDRAW_PARA_REFRESH, (int)calibrationListItem ,(int)SetGuiResponseVal);
    BOOL fExit = FALSE;
    while(1)
    {
        if(fExit)
            break;
        terninalPrintf("--> ");
        setTesterFlag(FALSE);
        tempchar = userResponseLoop();
        setTesterFlag(TRUE);
        terninalPrintf("%c",tempchar);
        GuiManagerCleanMessage(GUI_CLEAN_MESSAGE_ENABLE);
        switch(tempchar)
        {
            case 'a':
                terninalPrintf("\r\n");
                while(1)
                {   
                    responsechar = userResponse();
                    if(responsechar == 'q')
                        break;
                    else if(responsechar == 'c')
                    {   
                        if(MemsCalibrationSet(&m_MEMSx,&m_MEMSy,&m_MEMSz))
                        {
                            terninalPrintf("After MEMS calibration X=%d Y=%d Z=%d\r\n",m_MEMSx,m_MEMSy,m_MEMSz);
                        }
                    }
                        
                    if(QueryMEMSValue(&m_MEMSx,&m_MEMSy,&m_MEMSz))                       
                        terninalPrintf("MEMS measurement: X=%d Y=%d Z=%d\r\n",m_MEMSx,m_MEMSy,m_MEMSz);
                    else
                        terninalPrintf("MEMS measurement: No data.\r\n");
                    vTaskDelay(1000/portTICK_RATE_MS);
                }
                break;
            case 'b':    
                SolarValtageCalibration();
                break;
            case 'm':
                //short m_MEMSx,m_MEMSy,m_MEMSz;
                QueryMEMSValue(&m_MEMSx,&m_MEMSy,&m_MEMSz);
                terninalPrintf("MEMS measurement: X=%d Y=%d Z=%d\r\n",m_MEMSx,m_MEMSy,m_MEMSz);
                break;
            case '0':
                terninalPrintf("\r\n =====  [0)MEMSCalibration Start] =====\r\n");
                EPDDrawString(FALSE,"                              ",100,100+(44*(10)));
                EPDDrawString(FALSE,"                              ",100,100+(44*(11)));
                //EPDDrawString(FALSE,"           ",300+400,100+(44*(2)));
                EPDDrawString(FALSE,"           ",300+400,100+(44*(3)));
                EPDDrawString(FALSE,"           ",300+400,100+(44*(4)));
                EPDDrawString(TRUE,"Wait",300+400,100+(44*(2)));
                short MEMSx,MEMSy,MEMSz;
                QueryMEMSValue(&MEMSx,&MEMSy,&MEMSz);
                terninalPrintf("Before MEMS calibration X=%d Y=%d Z=%d\r\n",MEMSx,MEMSy,MEMSz);
                if(MemsCalibrationSet(&MEMSx,&MEMSy,&MEMSz))
                {
                    terninalPrintf("After MEMS calibration X=%d Y=%d Z=%d\r\n",MEMSx,MEMSy,MEMSz);
                    QueryMEMSValueEx(&MEMSx,&MEMSy,&MEMSz);
                    terninalPrintf("After MEMS calibration X=%d Y=%d Z=%d\r\n",MEMSx,MEMSy,MEMSz);
                    EPDDrawString(TRUE,"OK   ",300+400,100+(44*(2)));
                }
                else
                {
                    EPDDrawString(TRUE,"FAIL ",300+400,100+(44*(2)));
                }
                break;
            case '1':
                //EPDDrawString(TRUE,"no Function",300+400,100+(44*(3)));
                EPDDrawString(FALSE,"                              ",100,100+(44*(10)));
                EPDDrawString(FALSE,"                              ",100,100+(44*(11)));
                EPDDrawString(FALSE,"           ",300+400,100+(44*(2)));
                //EPDDrawString(FALSE,"           ",300+400,100+(44*(3)));
                EPDDrawString(FALSE,"           ",300+400,100+(44*(4)));
                EPDDrawString(TRUE,"Wait       ",300+400,100+(44*(3)));

                //terninalPrintf("\r\n!!! lidar1Test !!!\r\n");
                terninalPrintf("\r\n =====  [1)lidar1Calibration Start] =====\r\n");
            
                //setPrintfFlag(FALSE);
                //pRadarInterface = RadarGetInterface(LIDAR_AV_DESIGN_INTERFACE_INDEX);
                pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);
                if(pRadarInterface == NULL)
                {
                    terninalPrintf("lidarTest ERROR (pRadarInterface == NULL)!!\n");
                    return FALSE;
                }
                if(pRadarInterface->initFunc() == FALSE)
                {
                    terninalPrintf("lidarTest ERROR (initFunc false)!!\n");
                    return FALSE;
                }
                terninalPrintf("Press 'q' to Exit\r\n");
                EPDDrawString(TRUE,"Start Li1  ",300+400,100+(44*(3)));
                
                int retry1 = 3;
                //int tempvalue;
                while(retry1 >0)
                {
                    pRadarInterface->setStartCalibrate(0, TRUE);
                    //tempvalue = pRadarInterface->checkFeaturnFunc( 0, &changeFlag, NULL, NULL, NULL);
                    //terninalPrintf("tempvalue = %d \r\n",tempvalue);
                    if(pRadarInterface->checkFeaturnFunc( 0, &changeFlag, NULL, NULL, NULL) == RADAR_CALIBRATION_OK)  
                        break;
                    retry1--;
                }
                
                if(retry1 >0)
                {

                    pRadarInterface->setPowerStatusFunc(0,FALSE);
                    vTaskDelay(1000/portTICK_RATE_MS);
                    pRadarInterface->setPowerStatusFunc(0,TRUE);
                    vTaskDelay(4000/portTICK_RATE_MS);

                    
                    
                    pRadarInterface->checkFeaturnFunc( 0, &changeFlag, NULL, NULL, NULL);

                    EPDDrawString(TRUE,"           ",300+400,100+(44*(3)));
                    
                    if(pRadarInterface->readDistValueFunc(0,&distValue[0]) == TRUE)
                    {
                        terninalPrintf("\r\nLidar1 Distance:%d  \r\n",distValue[0]);
                        sprintf(distStringBuff[0],"Lidar1 Dist:%d        ",distValue[0]);
                        EPDDrawString(TRUE,distStringBuff[0],100,100+(44*(10)));
                    }
                    else
                    {
                        terninalPrintf("\r\nLidar1 calibration fail.\r\n");
                        EPDDrawString(TRUE,"Lidar1 not calibrated",100,100+(44*(10)));
                    }
                }
                else
                {
                    terninalPrintf("\r\nLidar1 calibration fail.\r\n");
                    EPDDrawString(FALSE,"           ",300+400,100+(44*(3)));
                    EPDDrawString(TRUE,"Lidar1 not calibrated",100,100+(44*(10)));
                }
                pRadarInterface->setPowerStatusFunc(0,FALSE);
                
                break;
            case '2':
                EPDDrawString(FALSE,"                              ",100,100+(44*(10)));
                EPDDrawString(FALSE,"                              ",100,100+(44*(11)));
                EPDDrawString(FALSE,"           ",300+400,100+(44*(2)));
                EPDDrawString(FALSE,"           ",300+400,100+(44*(3)));
                //EPDDrawString(TRUE,"           ",300+400,100+(44*(4)));
                EPDDrawString(TRUE,"Wait       ",300+400,100+(44*(4)));
                //static RadarInterface* pRadarInterface;
                //BOOL changeFlag;
                //terninalPrintf("\r\n!!! lidar2Test !!!\r\n");
                terninalPrintf("\r\n =====  [2)lidar2Calibration Start] =====\r\n");
                //setPrintfFlag(FALSE);
                //pRadarInterface = RadarGetInterface(LIDAR_AV_DESIGN_INTERFACE_INDEX);
                pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);
                if(pRadarInterface == NULL)
                {
                    terninalPrintf("lidarTest ERROR (pRadarInterface == NULL)!!\n");
                    return FALSE;
                }
                if(pRadarInterface->initFunc() == FALSE)
                {
                    terninalPrintf("lidarTest ERROR (initFunc false)!!\n");
                    return FALSE;
                }
                terninalPrintf("Press 'q' to Exit\r\n");
                EPDDrawString(TRUE,"Start Li2  ",300+400,100+(44*(4)));

                int retry2 = 3;
                while(retry2 >0)
                {
                    pRadarInterface->setStartCalibrate(1, TRUE);
                    if(pRadarInterface->checkFeaturnFunc( 1, &changeFlag, NULL, NULL, NULL) == RADAR_CALIBRATION_OK)  
                        break;
                    retry2--;
                }
                
                if(retry2 >0)
                {

                    pRadarInterface->setPowerStatusFunc(1,FALSE);
                    vTaskDelay(1000/portTICK_RATE_MS);
                    pRadarInterface->setPowerStatusFunc(1,TRUE);
                    vTaskDelay(4000/portTICK_RATE_MS);
                    
                    
                    pRadarInterface->checkFeaturnFunc( 1, &changeFlag, NULL, NULL, NULL);              

                    EPDDrawString(TRUE,"           ",300+400,100+(44*(4)));
                    
                    if(pRadarInterface->readDistValueFunc(1,&distValue[1]) == TRUE)
                    {
                        terninalPrintf("\r\nLidar2 Distance:%d  \r\n",distValue[1]);
                        sprintf(distStringBuff[1],"Lidar2 Dist:%d        ",distValue[1]);
                        EPDDrawString(TRUE,distStringBuff[1],100,100+(44*(11)));
                    }
                    else
                    {
                        terninalPrintf("\r\nLidar2 calibration fail.\r\n");
                        EPDDrawString(TRUE,"Lidar2 not calibrated",100,100+(44*(11)));
                    }
                }
                else
                {
                    terninalPrintf("\r\nLidar2 calibration fail.\r\n");
                    EPDDrawString(FALSE,"           ",300+400,100+(44*(4)));
                    EPDDrawString(TRUE,"Lidar2 not calibrated",100,100+(44*(11)));
                }
                pRadarInterface->setPowerStatusFunc(1,FALSE);
                
                
                break;
            case '3':
                EPDDrawString(TRUE,"no Function",300+400,100+(44*(4)));
                break;
            case 'q':
                terninalPrintf("\r\n =====  [q)Quit Start] =====\r\n");
                fExit = TRUE;
                break;
            default:
                terninalPrintf("error input");
                break;
        }

        if(fExit)
            break;
       terninalPrintf(" =====  [ End  ] =====\r\n");
        
        
        terninalPrintf("\r\n =====  [9)Calibration Start] =====\r\n");
        for(int i = 0; ; i++)
        {//Dont Show Quit
            if(calibrationListItem[i].charItem == 0)
            {
                break;
            }
            terninalPrintf("    %c) [%s]\r\n", calibrationListItem[i].charItem, calibrationListItem[i].itemName);       
        }
    }
    return TRUE;
}



static BOOL EPDflashTool(void* para1, void* para2)
{
    
    char tempchar;
    
    int lastvcomValue;
    int modifyvcomValue;
    int newvcomValue; 
    int recoveryvcomValue;
    char lastVCOMString[9];
    char modifyVCOMString[10];
    char recoveryVCOMString[12];
    
    
    for(int i = 0; ; i++)
    {//Dont Show Quit
        if(EPDflashToolListItem[i].charItem == 0)
        {
            break;
        }
        terninalPrintf("    %c) [%s]\r\n", EPDflashToolListItem[i].charItem, EPDflashToolListItem[i].itemName);
        
    }
    guiManagerShowScreen(GUI_EPDFLASH_TOOL_ID, GUI_REDRAW_PARA_REFRESH, (int)EPDflashToolListItem ,(int)SetGuiResponseVal);
    BOOL fExit = FALSE;
    while(1)
    {
        if(fExit)
            break;
        terninalPrintf("--> ");
        //setTesterFlag(FALSE);
        tempchar = userResponseLoop();
        //setTesterFlag(TRUE);
        terninalPrintf("%c",tempchar);
        GuiManagerCleanMessage(GUI_CLEAN_MESSAGE_ENABLE);
        switch(tempchar)
        {
            case 's':
                setW25Q64BVspecialburn();
                W25Q64BVburn();
                clrW25Q64BVspecialburn();
                break;
            case 'd':
                W25Q64BVdeviceID();
                break;
            case 'p':
                W25Q64BVFetch();
                break;
            
            case 'v':
                terninalPrintf("\r\nVCOM value = %d \r\n",IT8951GetVCOM());
                break;

            case 'u':
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(3)));
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(4)));
                //EPDDrawString(TRUE,"Wait",300+400,100+(44*(2)));
                if(W25Q64BVQuery())
                {
                    //EPDDrawString(TRUE,"OK   ",300+400,100+(44*(2)));
                }
                else
                {
                    //EPDDrawString(TRUE,"FAIL ",300+400,100+(44*(2)));
                }
                break;
            case 'e':
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(2)));
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(4)));
                //EPDDrawString(TRUE,"Wait       ",300+400,100+(44*(3)));
                W25Q64BVerase();
                
                break;
            case '0':

                //EPDDrawString(TRUE,"               ",575,100+(44*(2)));
                terninalPrintf("\r\n =====  [0)FACTORY TEST Start] =====\r\n");
                vTaskDelay(500/portTICK_RATE_MS);
                lastvcomValue = IT8951GetVCOM();

                terninalPrintf("VCOM value before burn = %d \r\n",lastvcomValue);
                //sprintf(lastVCOMString,"VCOM:%d   ",lastvcomValue );
                //EPDDrawString(TRUE,lastVCOMString,575,100+(44*(2)));
                if(lastvcomValue == 0)
                {
                    //if(SetEPDVCOM())
                    if(SetEPDVCOMEx())           
                    {
                        //modifyvcomValue = IT8951GetVCOM();
                        //terninalPrintf("New VCOM value = %d \r\n",modifyvcomValue);
                        //sprintf(modifyVCOMString,":%d",modifyvcomValue );
                        //EPDDrawString(TRUE,modifyVCOMString,550+150+8,100+(44*(2)));
                        //vTaskDelay(1500/portTICK_RATE_MS);
                        
                        //terninalPrintf("New VCOM value = %d \r\n",*setVCOMpoint);
                        terninalPrintf("New VCOM value = %d \r\n",setVCOMEx);
                        vTaskDelay(1500/portTICK_RATE_MS);
                        
                    }
                    else
                    {
                       // EPDDrawString(TRUE,"FAIL   ",90+575,100+(44*(2)));
                        break;
                    }
                }
                
                GuiManagerCleanMessage(GUI_KEY_DISABLE);
                if(W25Q64BVburn())
                {
                    vTaskDelay(100/portTICK_RATE_MS);
                    newvcomValue = IT8951GetVCOM();
                    terninalPrintf("VCOM value after burn = %d \r\n",newvcomValue);
                    //IT8951SetVCOM(modifyvcomValue);
                    vTaskDelay(100/portTICK_RATE_MS);
                    if(lastvcomValue == 0)
                    {
                        //IT8951SetVCOM(*setVCOMpoint);
                        IT8951SetVCOM(setVCOMEx);
                    }
                    else
                        IT8951SetVCOM(lastvcomValue);
                    vTaskDelay(100/portTICK_RATE_MS);
                    recoveryvcomValue = IT8951GetVCOM();
                    terninalPrintf("Recovery VCOM value = %d \r\n",recoveryvcomValue);

                    toggleEPDswitch(TRUE);
                    vTaskDelay(100/portTICK_RATE_MS);
                    EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE);
                    terninalPrintf("Enter EPD all black,press \"space\" to continue.\r\n");
                    while(userResponseLoop() != 0x20);
                    EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
                    terninalPrintf("Enter EPD all white,press \"space\" to continue.\r\n");
                    while(userResponseLoop() != 0x20);
                    EPDSetSleepFunction(TRUE);
                    terninalPrintf("Enter EPD sleep mode,press \"space\" to continue.\r\n");
                    while(userResponseLoop() != 0x20);
                    EPDSetSleepFunction(FALSE);
                    
                    GuiManagerCleanMessage(GUI_REDRAW_PARA_CONTAIN);
                    

                    sprintf(recoveryVCOMString,"OK VCOM:%d   ",recoveryvcomValue );
                    EPDDrawString(TRUE,recoveryVCOMString,575,100+(44*(2)));

     
                    
                }
                else
                {
                    terninalPrintf("BURN FAIL\r\n");
                    EPDDrawString(TRUE,"FAIL        ",90+575,100+(44*(2)));
                }
                GuiManagerCleanMessage(GUI_KEY_ENABLE);

                break;
            /*
            case '1':

                EPDDrawString(TRUE,"         ",90+400,100+(44*(3)));
                terninalPrintf("\r\n =====  [1)SD Burn Start] =====\r\n");
                lastvcomValue = IT8951GetVCOM();

                terninalPrintf("VCOM value before burn = %d \r\n",lastvcomValue);
                sprintf(lastVCOMString,"VCOM:%d   ",lastvcomValue );
                EPDDrawString(TRUE,lastVCOMString,400,100+(44*(3)));

            
                if(W25Q64BVburn())
                {
                    newvcomValue = IT8951GetVCOM();
                    terninalPrintf("VCOM value after burn = %d \r\n",newvcomValue);
                    IT8951SetVCOM(modifyvcomValue);
                    recoveryvcomValue = IT8951GetVCOM();
                    terninalPrintf("Recovery VCOM value = %d \r\n",recoveryvcomValue);
                 
                    EPDDrawString(TRUE,"$$$$$$$$$$$$    ",400,100+(44*(3)));
                    EPDDrawString(TRUE,"------------    ",400,100+(44*(3)));
                    sprintf(recoveryVCOMString,"OK VCOM:%d   ",recoveryvcomValue );
                    EPDDrawString(TRUE,recoveryVCOMString,400,100+(44*(3)));
     
                    
                }
                else
                {
                    terninalPrintf("BURN FAIL\r\n");
                    EPDDrawString(TRUE,"FAIL        ",90+400,100+(44*(3)));
                }
                

                break;
            */
            case '1':
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(3)));
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(4)));
                //EPDDrawString(TRUE,"Wait",300+400,100+(44*(2)));
                terninalPrintf("\r\n =====  [1)EPD VCOM Start] =====\r\n");
                int tempVCOM;
                char VCOMString[5];
                tempVCOM = IT8951GetVCOM();
                terninalPrintf("\r\nLast VCOM value = %d \r\n",tempVCOM );
                sprintf(VCOMString,":%d   ",tempVCOM );
                //EPDDrawString(TRUE,VCOMString,300+400,100+(44*(3)));
                EPDDrawString(TRUE,VCOMString,90+300,100+(44*(4)));
                if(SetEPDVCOM())
                {
                    vTaskDelay(500/portTICK_RATE_MS);
                    tempVCOM = IT8951GetVCOM();
                    vTaskDelay(100/portTICK_RATE_MS);
                    terninalPrintf("New VCOM value = %d \r\n",tempVCOM);
                    sprintf(VCOMString,":%d",tempVCOM );
                    //EPDDrawString(TRUE,VCOMString,300+400,100+(44*(3)));
                    EPDDrawString(TRUE,VCOMString,90+300,100+(44*(4)));
                    //EPDDrawString(TRUE,"OK   ",300+400,100+(44*(2)));
                }
                else
                {
                    EPDDrawString(TRUE,"FAIL   ",90+400,100+(44*(4)));
                }
                
                
                EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_BLACK_INDEX,80,600-44);
                vTaskDelay(100/portTICK_RATE_MS);
                EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,80,600-44);
                vTaskDelay(100/portTICK_RATE_MS);
                EPDDrawStringMax(TRUE," <>:Move   {:Select    }:Exit ",90,700-44,TRUE);
                
                break;
            
            case '2':
                terninalPrintf("\r\n =====  [2)EPD Burning Test Start] =====\r\n");
                epdBurningTest(NULL,NULL);
                break;
            case '3':  
                terninalPrintf("\r\n =====  [3)EPD All Black Start] =====\r\n");
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_BLACK_INDEX,0,0);
                EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE);            
                terninalPrintf("\r\nPress 'q' to Exit Test\n");
                while(userResponseLoop()!='q');            
                GuiManagerCleanMessage(GUI_REDRAW_PARA_CONTAIN);
                
                /*
                EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
                EPDDrawStringMax(FALSE,"EPDflashTool",90,28,FALSE);
                EPDDrawStringMax(FALSE," <>:Move   {:Select    }:Exit ",180,700-44,TRUE);
    
                for(int i=0;i<6;i++)
                {
                    EPDDrawStringMax(FALSE,EPDflashToolListItem[i].itemName,300+50,100+(44*(2+i)),FALSE);
                    EPDDrawStringMax(FALSE,"-",300,100+(44*(2+i)),TRUE);
                }
                EPDDrawString(FALSE,"ENABLE  ",300+450,100+(44*(7)));
                EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,300,100+(44*(5)));
                */

                break;    
            case '4':
                terninalPrintf("\r\n =====  [4)EPD All White Start] =====\r\n");
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
                EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);              
                terninalPrintf("\r\nPress 'q' to Exit Test\n");
                while(userResponseLoop()!='q');            
                GuiManagerCleanMessage(GUI_REDRAW_PARA_CONTAIN);
                /*
                EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
                EPDDrawStringMax(FALSE,"EPDflashTool",90,28,FALSE);
                EPDDrawStringMax(FALSE," <>:Move   {:Select    }:Exit ",180,700-44,TRUE);
    
                for(int i=0;i<6;i++)
                {
                    EPDDrawStringMax(FALSE,EPDflashToolListItem[i].itemName,300+50,100+(44*(2+i)),FALSE);
                    EPDDrawStringMax(FALSE,"-",300,100+(44*(2+i)),TRUE);
                }
                EPDDrawString(FALSE,"ENABLE  ",300+450,100+(44*(7)));
                EPDDrawContainByIDPos(TRUE,EPD_PICT_LINE_SMALL_2_I_INDEX,300,100+(44*(6)));
                */
                
                break;              
            case '5':
                //setPrintfFlag(FALSE);
                if(readEPDswitch() == TRUE)
                {
                    //EPDDrawString(TRUE,"DISABLE",300+450,100+(44*(7)));
                    EPDDrawString(TRUE,"DISABLE",90+325,100+(44*(7)));
                    //terninalPrintf("\r\nEPD display disable.\r\n");
                    //terninalPrintf("\r\n =====  [1)EPD display disable] =====\r\n");
                    terninalPrintf("\r\n =====  [5)EPD DISP Start] =====\r\n");
                    terninalPrintf("EPD display disable.\r\n");
                    toggleEPDswitch(FALSE);

                }
                else
                {
                    terninalPrintf("\r\n =====  [5)EPD DISP Start] =====\r\n");
                    vTaskDelay(500/portTICK_RATE_MS);
                    toggleEPDswitch(TRUE);
                    //GuiManagerInit();
                    EPDSetBacklight(FALSE);
                    //terninalPrintf("\r\nEPD display enable.\r\n");
                    //terninalPrintf("\r\n =====  [1)EPD display enable] =====\r\n");
                    terninalPrintf("EPD display enable.\r\n");
                    //setSpecialCleanFlag(TRUE);
                    //EPDDrawString(TRUE,"$$$$$$$",300+450,100+(44*(7)));
                    EPDDrawString(TRUE,"$$$$$$$",90+325,100+(44*(7)));
                    //EPDDrawString(TRUE,"------  ",300+450,100+(44*(3)));
                   // EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX ,300,100+(44*(oldIndex+2)));
                    //EPDDrawString(TRUE,"ENABLE  ",300+450,100+(44*(7)));
                    EPDDrawString(TRUE,"ENABLE  ",90+325,100+(44*(7)));
                    vTaskDelay(1000/portTICK_RATE_MS);
                }
            
            
                //EPDDrawString(TRUE,"no Function",300+400,100+(44*(4)));
                break;

            case 'q':
                terninalPrintf("\r\n =====  [q)Quit Start] =====\r\n");
                fExit = TRUE;
                break;
            default:
                terninalPrintf("\r\nerror input");
                break;
        }

        if(fExit)
            break;
        terninalPrintf(" =====  [ End  ] =====\r\n");
        
        
        terninalPrintf("\r\n =====  [a)EPD Tool Start] =====\r\n");
        for(int i = 0; ; i++)
        {//Dont Show Quit
            if(EPDflashToolListItem[i].charItem == 0)
            {
                break;
            }
            terninalPrintf("    %c) [%s]\r\n", EPDflashToolListItem[i].charItem, EPDflashToolListItem[i].itemName);
        
        }
    }
    return TRUE;
}

static BOOL NewRadarTool(void* para1, void* para2)
{

    char tempchar;
    uint16_t mindex,rindex;
    uint16_t FRtype = 0;  // 0:F type  1:R type
    BOOL TempResult;
    RadarInterface* pRadarInterface;
    
    for(int i = 0; ; i++)
    {//Dont Show Quit
        if(NewRadarToolListItem[i].charItem == 0)
        {
            break;
        }
        terninalPrintf("    %c) [%s]\r\n", NewRadarToolListItem[i].charItem, NewRadarToolListItem[i].itemName);
        
    }
         
    guiManagerShowScreen(GUI_RADAR_TOOL_ID, GUI_REDRAW_PARA_REFRESH, (int)NewRadarToolListItem ,(int)SetGuiResponseVal);
    //setPrintfFlag(TRUE);
    
    BOOL fExit = FALSE;
    

    
    
    while(1)
    {
        if(fExit)
            break;
        terninalPrintf("--> ");        
        setTesterFlag(FALSE);
        tempchar = userResponseLoop();
        setTesterFlag(TRUE);
        terninalPrintf("%c",tempchar);
        //GuiManagerCleanMessage(GUI_CLEAN_MESSAGE_ENABLE);
        //setPrintfFlag(TRUE);
        switch(tempchar)
        {
            case '0':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [0)Set RadarA Parameter Start] =====\r\n");
                //GuiManagerCleanMessage(GUI_SET_RADARA_PARAMETER);
                mindex = 0;   // EPM right side               
                TempResult = NEWradarSetDefault(&mindex,NULL);
                if( (TempResult == FALSE) || (TempResult == TEST_FALSE) )
                    break;
                vTaskDelay(1000/portTICK_RATE_MS);
                GuiManagerCleanMessage(GUI_NEW_RADAR_TEST);
                NEWradarTest(NULL,NULL);
                break;
                
            case '1':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [1)Set RadarB Parameter Start] =====\r\n");
                //GuiManagerCleanMessage(GUI_SET_RADARB_PARAMETER);
                mindex = 1;   // EPM left side 
                TempResult = NEWradarSetDefault(&mindex,NULL);
                if( (TempResult == FALSE) || (TempResult == TEST_FALSE) )
                    break;
                vTaskDelay(1000/portTICK_RATE_MS);
                GuiManagerCleanMessage(GUI_NEW_RADAR_TEST);
                NEWradarTest(NULL,NULL);
                break;
            case '2':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [2)Set RadarA Parameter Start] =====\r\n");
                //GuiManagerCleanMessage(GUI_SET_RADARA_PARAMETER);
                rindex = 0;                
                TempResult = NEWradarSet(&rindex,NULL);
                if( (TempResult == FALSE) || (TempResult == TEST_FALSE) )
                    break;
                vTaskDelay(1000/portTICK_RATE_MS);
                GuiManagerCleanMessage(GUI_NEW_RADAR_TEST);
                NEWradarTest(NULL,NULL);
                break;
            case '3':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [3)Set RadarB Parameter Start] =====\r\n");
                //GuiManagerCleanMessage(GUI_SET_RADARB_PARAMETER);
                rindex = 1;
                TempResult = NEWradarSet(&rindex,NULL);
                if( (TempResult == FALSE) || (TempResult == TEST_FALSE) )
                    break;
                vTaskDelay(1000/portTICK_RATE_MS);
                GuiManagerCleanMessage(GUI_NEW_RADAR_TEST);
                NEWradarTest(NULL,NULL);
                break;
            case '4':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [4)New Radar Test Start] =====\r\n");
                /*
                terninalPrintf("\r\nPlease enter F/R parameters in decimal.(0:F type  1:R type)\r\n");
                terninalPrintf("Enter number is ");
                FRtype = SetValue(1);
                terninalPrintf("\r\n");
                if(FRtype == 0xffff)
                {
                    FRtype = 0;
                    break;
                }

                if(FRtype > 1)
                {
                    terninalPrintf("input error.\r\n");
                    FRtype = 0;
                    break;
                }
                */
                GuiManagerCleanMessage(GUI_NEW_RADAR_TEST);
                NEWradarTest(NULL,NULL);
                break;     
            case '5':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [5)RadarA OTA] =====\r\n");
            
                pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);
                if(pRadarInterface == NULL)
                {
                    terninalPrintf("NEWradarTest ERROR (pRadarInterface == NULL)!!\n");
                    break;
                }
                if(pRadarInterface->RadarRB60POTAFunc(0) == FALSE)
                {
                    terninalPrintf("RB60POTAFunc ERROR!!\n");
                    EPDDrawString(TRUE,"ERROR            ",580,100+(44*(2+5)));  
                }
                pRadarInterface->initFunc();                    
                break;  
            case '6':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [6)RadarB OTA] =====\r\n");
            
                pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);
                if(pRadarInterface == NULL)
                {
                    terninalPrintf("NEWradarTest ERROR (pRadarInterface == NULL)!!\n");
                    break;
                }
                if(pRadarInterface->RadarRB60POTAFunc(1) == FALSE)
                {
                    terninalPrintf("RB60POTAFunc ERROR!!\n");
                    EPDDrawString(TRUE,"ERROR            ",580,100+(44*(2+6)));  
                }
                pRadarInterface->initFunc(); 
                break;                  
            case 'q':
                terninalPrintf("\r\n =====  [q)Quit Start] =====\r\n");
                //terninalPrintf("\r\n");
                fExit = TRUE;
                break;
            default:
                terninalPrintf("\r\n");
                terninalPrintf("error input");
                break;
        }

        if(fExit)
            break;
        terninalPrintf(" =====  [ End  ] =====\r\n");
        
        
        terninalPrintf("\r\n =====  [4)New Radar Tool Start] =====\r\n");
        for(int i = 0; ; i++)
        {//Dont Show Quit
            if(NewRadarToolListItem[i].charItem == 0)
            {
                break;
            }
            terninalPrintf("    %c) [%s]\r\n", NewRadarToolListItem[i].charItem, NewRadarToolListItem[i].itemName);
        
        }
        if( (tempchar != '5') && (tempchar != '6') )
            GuiManagerCleanMessage(GUI_REDRAW_PARA_REFRESH);
    }
    
    
        return TRUE;
}


static BOOL RadarOTATool(void* para1, void* para2)
{
    
    char tempchar;
    
    
    for(int i = 0; ; i++)
    {//Dont Show Quit
        if(RadarOTAToolListItem[i].charItem == 0)
        {
            break;
        }
        terninalPrintf("    %c) [%s]\r\n", RadarOTAToolListItem[i].charItem, RadarOTAToolListItem[i].itemName);
        
    }
         
    guiManagerShowScreen(GUI_RADAR_TOOL_ID, GUI_REDRAW_PARA_REFRESH, (int)RadarOTAToolListItem ,(int)SetGuiResponseVal);
    //setPrintfFlag(TRUE);
    
    BOOL fExit = FALSE;
    
      
    while(1)
    {
        if(fExit)
            break;
        terninalPrintf("--> ");        
        setTesterFlag(FALSE);
        tempchar = userResponseLoop();
        setTesterFlag(TRUE);
        terninalPrintf("%c",tempchar);
        GuiManagerCleanMessage(GUI_CLEAN_MESSAGE_ENABLE);
        //setPrintfFlag(TRUE);
        switch(tempchar)
        {

            case '0':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [0)Radar1 OTA Start] =====\r\n");
            
                //pRadarInterface->setPowerStatusFunc(0,TRUE);
            
                //pRadarInterface->FirstOTAFunc(0,0);
            
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(2)));
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(3)));
                //EPDDrawString(TRUE,"Wait     ",300+400,100+(44*(2)));

                RadarOTAprogramTool(0);
                

                    

                
                /*if(W25Q64BVburn())
                {
                    //EPDDrawString(FALSE,"        ",300+400,100+(44*(2)));
                    //EPDDrawString(FALSE,"        ",300+400,100+(44*(3)));
                    EPDDrawString(TRUE,"OK          ",300+400,100+(44*(2)));
                }
                else
                {
                    terninalPrintf("BURN FAIL\r\n");
                    //EPDDrawString(FALSE,"        ",300+400,100+(44*(2)));
                    //EPDDrawString(FALSE,"        ",300+400,100+(44*(3)));
                    EPDDrawString(TRUE,"FAIL        ",300+400,100+(44*(2)));
                }  */
                
                //pRadarInterface->setPowerStatusFunc(0,FALSE);

                break;
            case '1':
                terninalPrintf("\r\n");
                terninalPrintf("\r\n =====  [1)Radar2 OTA Start] =====\r\n");
                //EPDDrawString(TRUE,"Wait     ",300+400,100+(44*(3)));

                RadarOTAprogramTool(1);
                
            
            
            

                //setPrintfFlag(FALSE);
                /*if(readEPDswitch() == TRUE)
                {
                    EPDDrawString(TRUE,"DISABLE",300+450,100+(44*(3)));
                    toggleEPDswitch(FALSE);
                    terninalPrintf("EPD display disable.\r\n");
                }
                else
                {
                    toggleEPDswitch(TRUE);
                    terninalPrintf("EPD display enable.\r\n");
                    EPDDrawString(TRUE,"ENABLE  ",300+450,100+(44*(3)));
                    vTaskDelay(1000/portTICK_RATE_MS);
                }  */
                //vTaskDelay(1000/portTICK_RATE_MS);
                
            
                break;
     

            case 'q':
                terninalPrintf("\r\n =====  [q)Quit Start] =====\r\n");
                //terninalPrintf("\r\n");
                fExit = TRUE;
                break;
            default:
                terninalPrintf("\r\n");
                terninalPrintf("error input");
                break;
        }

        if(fExit)
            break;
        terninalPrintf(" =====  [ End  ] =====\r\n");
        
        
        terninalPrintf("\r\n =====  [b)Radar OTA Start] =====\r\n");
        for(int i = 0; ; i++)
        {//Dont Show Quit
            if(RadarOTAToolListItem[i].charItem == 0)
            {
                break;
            }
            terninalPrintf("    %c) [%s]\r\n", RadarOTAToolListItem[i].charItem, RadarOTAToolListItem[i].itemName);
        
        }
    }
    
    
        return TRUE;
}



static BOOL versionQueryTool(void* para1, void* para2)
{
	  //uint8_t *VerCode1,*VerCode2,*VerCode3,*YearCode,*MonthCode,*DayCode,*HourCode,*MinuteCode;
    uint8_t *code11,*code12,*code13,*code14,*code21,*code22,*code23,*code24;
    int pageCounter = 0;
    int pagechangeFlag = TRUE;
    char tempchar;
    char  preadFWVersion[16] , preadLUTVersion[16] , tempreadLUTVersion[17];
    static char retVer[100],SIMStr[100];
    
    char* pch1= malloc(100);
    char* SIMpch1= malloc(100);
    char* pch2;
    char* SIMpch2;
    static char tempchr[100],SIMtempchr[100] ;  //= malloc(100);
    char tempRadar1VersionString[50];
    char tempRadar2VersionString[50];
    static RadarInterface* pRadarInterface;
    //RadarInterface* pRadarInterface;
    
    
    int waitCounter = 15;//30;
    //int iCount=0;
    int ReaderStatus;
    char ReaderVerBuf[64];
    
    EPDDrawContainByIDPos(TRUE,EPD_PICT_LOADING,500,250);
   
    //EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);   
    guiManagerShowScreen(GUI_VERSION_ID, GUI_REDRAW_PARA_REFRESH, 0 ,0);
    
    
    EPDDrawString(FALSE,"------- Version  ------",180,0);
    //EPDDrawContainByIDPos(TRUE,EPD_PICT_LOADING,500,50);
    EPDDrawString(FALSE,"Checking",275,50);
    //EPDDrawString(TRUE,"LED:\nAP:\nReader:\nEPD:\nSIM:\nMODEM:\nRADAR1:\nRADAR2:\n\n\nPress \'q\' to continue...",100,150);
    EPDDrawString(TRUE,"LED:\nAP:\nReader:\nEPD:\nSIM:\nMODEM:\nRADAR1:\nRADAR2:",100,150);   
       
    //---LED---
    uint8_t VerCode1,VerCode2,VerCode3,YearCode,MonthCode,DayCode,HourCode,MinuteCode;
    char* string = malloc(29);
                        //char* string2 = malloc(17);
                        //terninalPrintf("%d",string);
                        //char string[28];

    if(QueryVersion(&VerCode1,&VerCode2,&VerCode3,&YearCode,&MonthCode,&DayCode,&HourCode,&MinuteCode))
    {
        /*terninalPrintf("VerCode1=%d\r\n",VerCode1);
        terninalPrintf("VerCode2=%d\r\n",VerCode2);
        terninalPrintf("VerCode3=%d\r\n",VerCode3);
        terninalPrintf("YearCode=%d\r\n",YearCode);
        terninalPrintf("MonthCode=%d\r\n",MonthCode);
        terninalPrintf("DayCode=%d\r\n",DayCode);
        terninalPrintf("HourCode=%d\r\n",HourCode);
        terninalPrintf("MinuteCode=%d\r\n",MinuteCode);*/

        *string      = 'V';
        *(string+1)  = 'e';
        *(string+2)  = 'r';
        *(string+3)  = ' ';
        *(string+4)  = 48+VerCode1;
        *(string+5)  = '.';
        *(string+6)  = 48+(VerCode2/10);
        *(string+7)  = 48+(VerCode2%10);
        *(string+8)  = '.';
        *(string+9)  = 48+(VerCode3/10);
        *(string+10) = 48+(VerCode3%10);		
        *(string+11) = '\0';
        *(string+12)  = 'b';
        *(string+13)  = 'u';
        *(string+14)  = 'i';
        *(string+15)  = 'l';
        *(string+16)  = 'd';
        *(string+17)  = ' ';
        *(string+18)  = 48+(YearCode/10);
        *(string+19)  = 48+(YearCode%10);
        *(string+20)  = 48+(MonthCode/10);
        *(string+21)  = 48+(MonthCode%10);
        *(string+22) = 48+(DayCode/10);
        *(string+23) = 48+(DayCode%10);
        *(string+24) = 48+(HourCode/10);
        *(string+25) = 48+(HourCode%10);
        *(string+26) = 48+(MinuteCode/10);
        *(string+27) = 48+(MinuteCode%10);
        *(string+28) = '\0';		
         
        terninalPrintf("LED : %s\r\n",string); 
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,string,350,150);
    }
    else
    {
        terninalPrintf("LED : Error\r\n"); 
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,"Error",350,150);
    }

    //---AP---
    vTaskDelay(15/portTICK_RATE_MS);
    EPDDrawString(TRUE,"Error",350,200);
    
    //---Reader---
    
    
    if(!GPIO_ReadBit(GPIOJ, BIT4))
    {
    
        CardReaderInit(FALSE);
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
        terninalPrintf("READER CHECK ");

        do{
            ReaderStatus = CardReaderGetBootedStatus();

            vTaskDelay(1000/portTICK_RATE_MS);
            waitCounter--;
            if(ReaderStatus == TSREADER_CHECK_READER_OK)
            {   
                OctopusReaderGetVersion(ReaderVerBuf);
                break;
            }
            if(waitCounter == 0)
            {
                terninalPrintf("\nCHECK READER [Time Out]\n");
                break;
            }
        }while(ReaderStatus != TSREADER_CHECK_READER_OK);

        terninalPrintf("ReaderVerBuf = %s\r\n",ReaderVerBuf);
        vTaskDelay(15/portTICK_RATE_MS);
        if (waitCounter <= 0)
        {
            EPDDrawString(TRUE,"Error",350,250);
        }
        else
        {
            EPDDrawString(TRUE,ReaderVerBuf,350,250);
        }
        
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI,FALSE);
    }
    else
    {
       
        CardReaderInit(TRUE);
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
        terninalPrintf("READER CHECK ");

        do{
            ReaderStatus = CardReaderGetBootedStatus();

            vTaskDelay(1000/portTICK_RATE_MS);
            waitCounter--;
            if(ReaderStatus == TSREADER_CHECK_READER_OK)
            {   
                EPMReaderGetVersion(ReaderVerBuf);
                break;
            }
            if(waitCounter == 0)
            {
                terninalPrintf("\nCHECK READER [Time Out]\n");
                break;
            }
        }while(ReaderStatus != TSREADER_CHECK_READER_OK);

        terninalPrintf("ReaderVerBuf = %s\r\n",ReaderVerBuf);
        vTaskDelay(15/portTICK_RATE_MS);
        if (waitCounter <= 0)
        {
            EPDDrawString(TRUE,"Error",350,250);
        }
        else
        {
            EPDDrawString(TRUE,ReaderVerBuf,350,250);
        }
    
    }  
    
    //---EPD---
    
    GuiManagerTimerSet(GUI_TIMER_DISABLE);
    vTaskDelay(1000/portTICK_RATE_MS);
    vTaskDelay(1000/portTICK_RATE_MS);
    ReadIT8951SystemInfo( preadFWVersion, preadLUTVersion);
    //memcpy(tempreadLUTVersion,preadLUTVersion,16);
    //tempreadLUTVersion[16] = '\0';//0x00;
    /*
    terninalPrintf("readFWVersion=");
    
    for (int p=0;p<16;p++)
    {
        terninalPrintf(" %02x",preadFWVersion[p]);
    }
    
    terninalPrintf("\r\n");
    
    
    terninalPrintf("readLUTVersion=");
    
    for (int p=0;p<16;p++)
    {
        terninalPrintf(" %02x",preadLUTVersion[p]);
    }
    
    terninalPrintf("\r\n");
    */
    terninalPrintf("readFWVersion=%s\r\n",preadFWVersion);
    terninalPrintf("readLUTVersion=%s\r\n",preadLUTVersion);
    //terninalPrintf("IT8951GetVCOM = %d \r\n",IT8951GetVCOM());
    vTaskDelay(15/portTICK_RATE_MS);
    if (preadFWVersion == NULL)
    {
        EPDDrawString(TRUE,"Error",350,300); 
    }
    else
    {
        EPDDrawString(TRUE,preadFWVersion,350,300); 
    }
    
    
    GuiManagerTimerSet(GUI_TIMER_ENABLE);
    
    //---SIM---
    BOOL QModemGetSIMNumberFlag ;
    QModemGetSIMNumberFlag = QModemGetSIMNumber(SIMStr);

    if(QModemGetSIMNumberFlag)
    {           
        memcpy(SIMtempchr,SIMStr,100);
        SIMpch1 = (char*) memchr(SIMtempchr,'\n',100); 
        memcpy(SIMtempchr,SIMpch1+1,99);    
        SIMpch2 = (char*) memchr(SIMtempchr,'\n',100);
        memset (SIMpch2,'\0',1);
        //terninalPrintf("tempchr=%s\r\ntempchrAD=%d\r\n",SIMtempchr,SIMtempchr); 
        terninalPrintf("SIM : %s\r\n",SIMtempchr);
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,SIMtempchr,350,350);        
    }
    else
    {
        terninalPrintf("SIM : Error\r\n"); 
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,"Error",350,350);
    }
    
    

    
    //---MODEM---
    BOOL QModemGetVerFlag;
    QModemGetVerFlag = QModemGetVer(retVer);
    
    if(QModemGetVerFlag)
    {            
        memcpy(tempchr,retVer,100);
        pch1 = (char*) memchr(tempchr,'\n',100);
        memcpy(tempchr,pch1+1,99);
        pch2 = (char*) memchr(tempchr,'\n',100);
        memset (pch2,'\0',1);
        //terninalPrintf("tempchr=%s\r\ntempchrAD=%d\r\n",tempchr,tempchr);
        terninalPrintf("MODEM : %s\r\n",tempchr);
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,tempchr,350,400); 
    }
    else
    {
        terninalPrintf("MODEM : Error\r\n");
        vTaskDelay(15/portTICK_RATE_MS);        
        EPDDrawString(TRUE,"Error",350,400); 
    }

    
    
    
    //---RADAR---
    
    
    BOOL changeFlag;
    
    uint8_t RadarData[22];
    
    uint8_t VersionCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D};
    //pRadarInterface = RadarGetInterface(RADAR_AV_DESIGN_INTERFACE_INDEX);
    pRadarInterface = RadarGetInterface(NEWRADAR_INTERFACE_INDEX);  

    if(pRadarInterface == NULL)
    {
        terninalPrintf("radarTest ERROR (pRadarInterface == NULL)!!\n");
    }

    if(pRadarInterface->initFunc() == FALSE)
    {
        terninalPrintf("radarTest ERROR (initFunc false)!!\n");
    }
    pRadarInterface->setPowerStatusFunc(0,TRUE);
    pRadarInterface->setPowerStatusFunc(1,TRUE);

    for(int j = 0; j<2; j++)
    {
        if(pRadarInterface->RadarResultFunc(j, 0x00,VersionCmd,RadarData) == TRUE)
        {   

            terninalPrintf("Radar%d Version : Ver.%d.%d.%d.%d\r\n",j+1,RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
            
            if(j == 0)
            {
                sprintf(tempRadar1VersionString,"VER %d.%d.%d.%d",RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
                vTaskDelay(15/portTICK_RATE_MS);
                EPDDrawString(TRUE,tempRadar1VersionString,350,450);
            }
            else if(j == 1)
            {
                sprintf(tempRadar2VersionString,"VER %d.%d.%d.%d",RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
                vTaskDelay(15/portTICK_RATE_MS);
                EPDDrawString(FALSE,tempRadar2VersionString,350,500);
            }
        }
        else
        {
            terninalPrintf("Radar%d Version : Error\r\n",j+1);
            vTaskDelay(15/portTICK_RATE_MS);
            if(j == 0)
                EPDDrawString(TRUE,"Error",350,450);
            else if(j == 1)
                EPDDrawString(TRUE,"Error",350,500);                
        } 
    }
    /*
    if(pRadarInterface -> readQueryVersionString(0, tempRadar1VersionString))
    {
        terninalPrintf("%s\r\n",tempRadar1VersionString);
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,tempRadar1VersionString,350,450);
    }
    else
    {
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,"Error",350,450);
    }
    if(pRadarInterface -> readQueryVersionString(1, tempRadar2VersionString))
    {
        terninalPrintf("%s\r\n",tempRadar2VersionString);
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(FALSE,tempRadar2VersionString,350,500);
    }
    else
    {
        vTaskDelay(15/portTICK_RATE_MS);
        EPDDrawString(TRUE,"Error",350,500);
    }
    */
    pRadarInterface->setPowerStatusFunc(0,FALSE);
    pRadarInterface->setPowerStatusFunc(1,FALSE);

    
    GuiManagerTimerSet(GUI_TIMER_DISABLE);
    //EPDDrawString(TRUE,"Press \'q\' to continue...",100,650);
    vTaskDelay(15/portTICK_RATE_MS);
    EPDDrawString(TRUE,"Press } to quit...",100,650);
    terninalPrintf("\r\n\r\nPress 'q' to quit... \r\n");
    
    vTaskDelay(1500/portTICK_RATE_MS);
    EPDDrawString(FALSE,"              ",275,50);
    EPDDrawString(FALSE,"              ",275,56);
    EPDDrawString(FALSE,"              ",275,100);
    EPDDrawString(TRUE,"              ",275,106);
      /*  while(1)
        {

            if(sysIsKbHit())
            {

                tempchar = getTerminalChar();
                if(tempchar=='q')
                    break;
                if((tempchar=='n')&&(pageCounter == 0))
                {
                    pageCounter = 1;
                    pagechangeFlag = TRUE;
                }
                if((tempchar=='p')&&(pageCounter == 1))
                {
                    pageCounter = 0;
                    pagechangeFlag = TRUE; 
                }
            } 
            if(guiResponseFlag)
            {
                tempchar = getGuiResponse();
                if(tempchar=='q')
                    break;
                if((tempchar=='n')&&(pageCounter == 0))
                {
                    pageCounter = 1;
                    pagechangeFlag = TRUE;
                }
                if((tempchar=='p')&&(pageCounter == 1))
                {
                    pageCounter = 0;
                    pagechangeFlag = TRUE;
                }
            }
            if(pagechangeFlag)
            {
                pagechangeFlag = FALSE;
                if(pageCounter == 0)
                {  
                    EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,50);
                    EPDDrawString(FALSE,"LED:\n\n\nAP:\n\n\nReader:\n\n\nEPD:\n\n\n            Page1      >Next",100,50);
                    EPDDrawString(FALSE,string,350,50);
                    EPDDrawString(FALSE,preadFWVersion,350,500); 
                    EPDDrawString(TRUE,preadLUTVersion,350,550);
                }
                else if(pageCounter == 1)
                {
                    EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,50);
                    EPDDrawString(FALSE,"SIM:\n\n\nMODEM:\n\n\nRADAR:\n\n\n\n\n\n<Previous   Page2      ",100,50);
                    EPDDrawString(TRUE,tempchr,350,200);
                    terninalPrintf("tempchrPrintAgain=%s\r\n",tempchr);
                    
                }
            }
            vTaskDelay(100/portTICK_RATE_MS);
        } */
        
		//terninalPrintf("Is the version number correct?(y/n)\r\n");
		//guiPrintMessage(string);
                            //EPDDrawString(TRUE,string1,X_POS_MSG,Y_POS_MSG+50);
                            //EPDDrawString(TRUE,string2,X_POS_MSG,Y_POS_MSG+100);
		
		
                            //vTaskDelay(2000/portTICK_RATE_MS);
                            //EPDDrawContainByIDPos(TRUE,EPD_PICT_KEY_CLEAN_BAR,X_POS_MSG-2,Y_POS_MSG-2); 
		
       while(1)
       {
            if(sysIsKbHit())
            {

                tempchar = getTerminalChar();
                if(tempchar=='q')
                    break;
             } 
            if(guiResponseFlag)
            {
                tempchar = getGuiResponse();
                if(tempchar=='q')
                    break;
            }
            vTaskDelay(100/portTICK_RATE_MS);
       }
        
		//free(string);
        //free(pch1);
                            //free(string2);
		

                  
          
	/* if(userResponseLoop()=='n')
      {
				guiPrintMessage("");
        EPDSetBacklight(FALSE);
        return TRUE;
      }
      */
		return TRUE;
}

//====================================================/=/=======================================================//

//////////////////////////////////////////User Interface-Cmd///////////////////////////////////////////
static BOOL exitTest(void* para1, void* para2)
{   
    return FALSE;
}
static BOOL totalTest(void* para1, void* para2)
{
    LEDColorBuffSet(0x00, 0x00);
    LEDBoardLightSet();
    char string1[80];
    testedItem=0;
    passItem=0;
    failItem=0;
    terninalPrintf("!!! Total Test Item !!!\r\n");
    int result;
    int tempItem;
    time_t  testBegintime;
    time_t  testEndtime;
    
    if(MBtestFlag )
    {
        //guiManagerShowScreen(GUI_SINGLE_TEST_ID, GUI_REDRAW_PARA_REFRESH,(int)MB_singleTestItem, ALL_TEST_MODE);
        //vTaskDelay(300/portTICK_RATE_MS);
        testBegintime = GetCurrentUTCTime();
        result=actionTestItem(0, MB_singleTestItem, para1, para2, TRUE);
    }
    else
    {
        //guiManagerShowScreen(GUI_SINGLE_TEST_ID, GUI_REDRAW_PARA_REFRESH,(int)singleTestItem, ALL_TEST_MODE);
        guiManagerShowScreen(GUI_SINGLE_TEST_ID, GUI_REDRAW_PARA_REFRESH,(int)AllTestItem, ALL_TEST_MODE);       
        vTaskDelay(300/portTICK_RATE_MS);
        //result=actionTestItem(0, singleTestItem, para1, para2, TRUE);
        result=actionTestItem(0, AllTestItem, para1, para2, TRUE);
    }
    if(MBtestFlag )
    {}
    else
    {
        cleanRst();
        cleanMsg();
    }
    if(MBtestFlag )                       
        tempItem = MBallTestItem;//allTestItem;
    else
        tempItem = allTestItem - 1;
    //if(passItem == allTestItem)
    if(passItem == tempItem)
    {
        terninalPrintf("<ALL TEST>Result:[PASS]\r\n");
        if(MBtestFlag )
        {}
        else
            EPDDrawString(TRUE,"PASS            ",X_POS_RST,Y_POS_RST);
        
    }
    else
    {
        terninalPrintf("<ALL TEST>Result:[FAIL]\r\n");
        if(MBtestFlag )
        {}
        else
        {
            EPDDrawString(TRUE,"FAIL            ",X_POS_RST,Y_POS_RST);
            LEDColorBuffSet(0x00, 0xff);
            LEDBoardLightSet();
        }
    }
    
    if(MBtestFlag )
    {
        sprintf(string1,"Total:%02d\\%02d\nPass :%02d\\%02d\nFail :%02d\\%02d\n\nPress } to Exit",testedItem,MBallTestItem,passItem,testedItem,failItem,testedItem);
        terninalPrintf("Total Test Item:%d/%d\r\nPass Item:%d/%d\r\nFail Item:%d/%d\r\n",testedItem,MBallTestItem,passItem,testedItem,failItem,testedItem);
    }
    else
    {
        sprintf(string1,"Total:%02d\\%02d\nPass :%02d\\%02d\nFail :%02d\\%02d\n\nPress } to Exit",testedItem,allTestItem-1,passItem,testedItem,failItem,testedItem);
        terninalPrintf("Total Test Item:%d/%d\r\nPass Item:%d/%d\r\nFail Item:%d/%d\r\n",testedItem,allTestItem-1,passItem,testedItem,failItem,testedItem);
    }
    if(MBtestFlag )
    {
        testEndtime = GetCurrentUTCTime();
        terninalPrintf("Test takes time : %d seconds\r\n",testEndtime - testBegintime);
        return TRUE;
    }
    else
    {
        terninalPrintf("Press 'q' to Exit Test\n");
        /* Show Result */
        guiPrintMessage(string1);
        while(userResponseLoop()!='q');
        if(result!=ACTION_TESTER_ITEM_FALSE)
        {
            return TRUE;
        }
        return FALSE;
    }
}

static BOOL singleTest(void* para1, void* para2)
{
    int guiID = GUI_SINGLE_TEST_ID;
    if(MBtestFlag )
        enterMunu(MENU_STRING_SINGLE, MB_singleTestItem, (void*)&guiID, NULL);
    else
        enterMunu(MENU_STRING_SINGLE, singleTestItem, (void*)&guiID, NULL);
    LEDColorBuffSet(0x00, 0x00);
    LEDBoardLightSet();
    setTesterFlag(FALSE);
    return TRUE;
}

static BOOL blankFunction(void* para1, void* para2)
{
    //int guiID=GUI_TOOL_TEST_ID;
    //enterMunu(MENU_STRING_TOOL, toolsFunctionItem, (void*)&guiID, NULL);
    
    char chrtemp;
    //guiManagerShowScreen(GUI_BLANK_ID, GUI_REDRAW_PARA_REFRESH,(int)singleTestItem, ALL_TEST_MODE);
    guiManagerShowScreen(GUI_BLANK_ID, GUI_REDRAW_PARA_REFRESH,0, 0);
    
   //EPDDrawMulti(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
    
    
   //EPDDrawString(TRUE,"Hello World",100,100);
    
   /* while(1)
    {
        //sysInitializeUART();
        //terninalPrintf("--> ");
        chrtemp=userResponseLoop();
        
        char* stringtmp = malloc(2);
        *stringtmp = chrtemp;
        *(stringtmp+1) = '\0';
        terninalPrintf("ishit=%s \n",stringtmp);
        EPDDrawString(TRUE,stringtmp,100,150);
        free(stringtmp); 
        if(chrtemp=='q')
        {
          break;
        }
        vTaskDelay(1000/portTICK_RATE_MS);
    }*/
    terninalPrintf("confirm to quit?\n");
    while(1)
    {
        if(userResponseLoop()=='q')
        {
            
           return TRUE; 
        }
        else
        {
            terninalPrintf("try again!\n");
        }
    } 
    return TRUE;
}

static BOOL KeyPadTool(void* para1, void* para2)
{
    //GuiManagerCleanMessage(GUI_CLEAN_MESSAGE_ENABLE);
    terninalPrintf("Press SW4 to quit.\n");
    guiManagerShowScreen(GUI_BLANK_ID, GUI_KEYPAD_TEST,0, 0);
    return TRUE;
}
static BOOL ModemHSpdTool(void* para1, void* para2)
{
    QModemLibInit(921600);
    if(!QModemATCmdTest())
    {
        QModemLibInit(115200);
        if(QModemSetHighSpeed())
            terninalPrintf("QModemSetHighSpeed successful\r\n");
        else
            terninalPrintf("QModemSetHighSpeed ERROR\r\n");   
        //QModemLibInit(921600);
    }
    else
    {
        terninalPrintf("QModem already HighSpeed. \r\n");
    }

    
    /*
    if(QModemSetHighSpeed())
        terninalPrintf("QModemSetHighSpeed successful\r\n");
    else
    {
        QModemLibInit(115200);
        if(QModemSetHighSpeed())
            terninalPrintf("QModemSetHighSpeed successful\r\n");
        else
            terninalPrintf("QModemSetHighSpeed ERROR\r\n");
    }
    */
    return TRUE;
}

static BOOL RTCTool(void* para1, void* para2)
{
    char tempchar;
    //guiManagerShowScreen(GUI_RTC_TOOL_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);  
    guiManagerShowScreen(GUI_RTC_TOOL_ID, GUI_REDRAW_PARA_REFRESH, 0 ,(int)SetGuiResponseVal); 
    //terninalPrintf("Recent time: ");
    //OctopusReaderQueryTime();
    
    
    //SetOctopusTime();
    //terninalPrintf("\r\nModify time: ");
    //OctopusReaderQueryTime();

    terninalPrintf("Is RTC time correct?(y/n)\r\n");
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar == 'n')
        {
            if(!SetOctopusTime())
                return TRUE;
            //terninalPrintf("\r\nModify time: ");
            //OctopusReaderQueryTime();

            terninalPrintf("Is reader time correct?(y/n)\r\n");
        }
        else if((tempchar == 'y') || (tempchar == 'q'))
            break;
        else
            terninalPrintf("Error input\r\n");
    }
    
    return TRUE;

}

static BOOL ModemTool(void* para1, void* para2)
{
    char tempChr;
    char CmdString[50];
    char FBCmdStr[100];
    int retlen;
    memset(CmdString,0x00,sizeof(CmdString));
    /*
    QModemLibInit(MODEM_BAUDRATE);
    
    int SIMStatus;
    if(QModemATCmdTest())
    {
        QModemQuerySIMInitStatus(&SIMStatus);
        terninalPrintf("SIMStatus = %d\r\n",SIMStatus);
        if((SIMStatus & 0x01) || (SIMStatus & 0x02) || (SIMStatus & 0x04))
        {
            QModemDialupStart();
            if(QModemDialupProcess() == TRUE)
            {
                terninalPrintf("\r\nMODEM Dialup SUCCESS\r\n");
                //vTaskDelay(5000/portTICK_RATE_MS);
                //if(QModemQueryNTP())
                //    terninalPrintf("\r\nQuery NTP SUCCESS\r\n");
                //else
                //    terninalPrintf("\r\nQuery NTP ERROR\r\n");
            }
            else
            {
                terninalPrintf("\r\nMODEM Dialup ERROR\r\n");
                
            }
            
        }
        
    }
    */
    
    //terninalPrintf("WEEKDAY = %d",inp32(REG_RTC_WEEKDAY));
    terninalPrintf("send >> ");
    while(1)
    {
        tempChr = superResponseLoop();
        if(tempChr == 0x1B)
            break;
        if((tempChr >= 0x20) &&(tempChr <= 0x7E))
            sprintf(CmdString,"%s%c",CmdString,tempChr);
            terninalPrintf("%c",tempChr);
        if(tempChr == 0x0D)
        {
            terninalPrintf("\r\n",CmdString);
            sprintf(CmdString,"%s\r\n",CmdString);
            QModemTerminal(CmdString,FBCmdStr,sizeof(FBCmdStr),&retlen,20);
            terninalPrintf("retn >> %s",FBCmdStr);
            //for(int i=0;i<retlen;i++)
            //    FBCmdStr
            
            
            memset(CmdString,0x00,sizeof(CmdString));
            terninalPrintf("send >> ");
        }
    }
    return TRUE;
}

static BOOL NTPTool(void* para1, void* para2)
{
    return QueryNTPfun();
}

BOOL QueryNTPfun(void)
{
    int SIMStatus;
    int QueryNTPRtyTimes = 0;
    int QuerySIMRtyTimes = 0;
    int QueryMODEMRtyTimes = 0;
    char waitStr[100]="Query NTP,please wait";
    RTC_TIME_DATA_T id;
    //QModemLibInit(MODEM_BAUDRATE);
    //if(QModemATCmdTest())
    //{
        //QModemATCmdTest();
        //QModemQuerySIMInitStatus(&SIMStatus);
        //vTaskDelay(5000/portTICK_RATE_MS);
    //terninalPrintf("Query NTP,please wait");
    while(QuerySIMRtyTimes < 20)
    {
        if(QModemQuerySIMInitStatus(&SIMStatus))
        {}
        else
            QueryMODEMRtyTimes++;
        
        if(QueryMODEMRtyTimes >= 8)
        {
            terninalPrintf("\r\nMODEM SETUP ERROR\r\n");
            return FALSE;
        }
        
        if(SIMStatus == 7)
            break;
        QuerySIMRtyTimes++;
        vTaskDelay(500/portTICK_RATE_MS);
        sprintf(waitStr,"%s.",waitStr);
        terninalPrintf("%s\r",waitStr);
        //terninalPrintf(".");
        //terninalPrintf("SIMStatus = %d\r\n",SIMStatus);
    }
    if(QuerySIMRtyTimes >= 20)
    {
        terninalPrintf("\r\nSIM SETUP ERROR\r\n");
        return FALSE;
    }
    terninalPrintf("\r\n");
        //if((SIMStatus & 0x01) || (SIMStatus & 0x02) || (SIMStatus & 0x04))
        //{
    while(QueryNTPRtyTimes < 3)
    {
        if(QModemQueryNTP(&id.u32Year,&id.u32cMonth,&id.u32cDay,&id.u32cHour,&id.u32cMinute,&id.u32cSecond))
        {
            terninalPrintf("Query NTP SUCCESS\r\n");
            id.u32Year += 2000;
            terninalPrintf("NTP:%d/%d/%d/ %d:%d:%d\r\n",id.u32Year, id.u32cMonth,id.u32cDay,id.u32cHour,id.u32cMinute,id.u32cSecond);
            
            if(SetOSTimeLite(id.u32Year, id.u32cMonth,id.u32cDay,id.u32cHour,id.u32cMinute,id.u32cSecond))
            {
                terninalPrintf("SET RTC SUCCESS\r\n");
                return TRUE;
            }
            else
            {
                terninalPrintf("SET RTC ERROR\r\n");
                return FALSE;
            }
        }
        else
        {
            QueryNTPRtyTimes++;
            //terninalPrintf("Query NTP ERROR\r\n");
        }
    }
    if(QueryNTPRtyTimes >= 3)
    {
        terninalPrintf("Query NTP ERROR\r\n");
        return FALSE;
    }

        //}
        //else
        //{
            //terninalPrintf("SIM SETUP ERROR\r\n");
        //}
    //}
    

}

static BOOL toolsFunction(void* para1, void* para2)
{
    int guiID=GUI_TOOL_TEST_ID;
    enterMunu(MENU_STRING_TOOL, toolsFunctionItem, (void*)&guiID, NULL);
    return TRUE;
}
 
static BOOL enterMunu(char* title, HWTesterItem* item, void* para1, void* para2)
{
    UINT8 charTmp;
    int reVal;
    while(1)
    {
        LEDColorBuffSet(0x00, 0x00);
        LEDBoardLightSet();
        EPDSetBacklight(FALSE);
        //Print Menu
        terninalPrintf("%s",title);
        for(int i = 0; ; i++)
        {
            if(item[i].itemName == NULL)
                break;
            terninalPrintf("    %c) [%s]\r\n", item[i].charItem, item[i].itemName);
        }
        if(para1!=NULL)
        {
            if(MBtestFlag )
            {
                //guiManagerShowScreen(*(int*)para1, GUI_REDRAW_PARA_REFRESH ,(int)item , 0 );
            }
            else
                guiManagerShowScreen(*(int*)para1, GUI_REDRAW_PARA_REFRESH ,(int)item , 0 );
            
        }
        terninalPrintf("--> ");
        setTesterFlag(FALSE);
        //Wait user Key-in
        charTmp = userResponseLoop();
        terninalPrintf("%c\r\n", charTmp);
        reVal = actionTestItem(charTmp, item, para1, para2, FALSE);
        //terninalPrintf("reVal = 0x%02x\r\n", reVal);
        if(reVal == ACTION_TESTER_ITEM_FALSE||reVal == TEST_FALSE)
        {
            return FALSE;
        }
        if((*(int*)para1 == GUI_SINGLE_TEST_ID) && (para2 == NULL))
            GuiManagerCleanMessage(GUI_CLEAN_MESSAGE_ENABLE);
        
        vTaskDelay(100/portTICK_RATE_MS);
    }
}

static int actionTestItem(char targetChar, HWTesterItem* item, void* para1, void* para2, BOOL ignoreChar)//ignoreChar TRUE->AllTest FALSE->Menu/Single/Tool
{
    int reVal = ACTION_TESTER_ITEM_NONE;
    uint16_t testResult;
    for(int i=0;i<allTestItem+(!ignoreChar);i++)//全測試忽略 quit();
    {
        //terninalPrintf("i = %d\r\n",i);
        //terninalPrintf("allTestItem = %d\r\n",allTestItem);
       // terninalPrintf("allTestItem+(!ignoreChar) = %d\r\n",allTestItem+(!ignoreChar));
       // terninalPrintf("allTestItem = %d\r\n",allTestItem);
       // terninalPrintf("(!ignoreChar) = %d\r\n",(!ignoreChar));
       // terninalPrintf("targetChar = %c\r\n",targetChar);
        if(((item[i].charItem == 'q') && ignoreChar) || (item[i].charItem == 0))
            break;
        if(((item[i].charItem == targetChar)||ignoreChar)&&(item[i].testerFunc != NULL))
        {
            int y_offset=0;
            if(item[i].charItem != 'q')
                testedItem++;
            //Errow point to now test item
            //if((ignoreChar) && (i <allTestItem+(!ignoreChar)-1 ))
            if((ignoreChar) && (i <allTestItem+(!ignoreChar) ))
            {
                if(i<5)
                {
                    y_offset = 110+(i*44);
                }
                else
                {
                    y_offset = 110+(i*44);//+ 28;
                }
                if(MBtestFlag )
                {}
                else
                    EPDDrawContainByIDPos(TRUE,EPD_PICT_KEY_RIGHT,90,y_offset);
            }
            LEDColorBuffSet(0x00, 0x00);
            LEDBoardLightSet();
            if(item[i].itemName != NULL)
            {
            terninalPrintf("\r\n =====  [%c)%s Start] =====\r\n", item[i].charItem, item[i].itemName);
            }
            else
            {
                terninalPrintf("\r\n =====  [%c) BLANK Start] =====\r\n", item[i].charItem);
            }
            if(!ignoreChar)
            {
                setTesterFlag(TRUE);
            }
            // entry tester function point //
            if(guiManagerCompareCurrentScreenId(GUI_SINGLE_TEST_ID))//這邊是要執行每一項測試時先清除殘餘的訊息
            {
                if(MBtestFlag )
                {}
                else
                {
                    cleanMsg();
                    cleanRst();
                }
            }
            if((MBtestFlag) && (item == MB_singleTestItem))
            {
                //terninalPrintf("i = %d \r\n", i);
                InitMTPvalue(MTPReportindex[i],(uint8_t*)MTPString);
                SetMTPCRC(39+i,(uint8_t*)MTPString);
                MTPCmdprint(39+i,(uint8_t*)MTPString);
            }
            testResult = item[i].testerFunc(para1, (void*)&ignoreChar);
            if(!ignoreChar)
            {
                setTesterFlag(FALSE);
            }
            //terninalPrintf("testResult = 0x%02x\r\n", testResult);
            switch(testResult)
            {
            case TRUE:
                reVal = ACTION_TESTER_ITEM_TRUE;
                break;
            case FALSE://exit
                if(MBtestFlag)
                {
                    if(item[i].charItem != 'q')
                    {
                        MTPString[MTPReportindex[i]][MTPString[MTPReportindex[i]][2]+2] = 0x80;
                        SetMTPCRC(MTPReportindex[i],(uint8_t*)MTPString);
                        MTPCmdprint(MTPReportindex[i],(uint8_t*)MTPString);
                    }
                    SetMTPCRC(38,(uint8_t*)MTPString);
                    MTPCmdprint(38,(uint8_t*)MTPString);
                }                    
                reVal = ACTION_TESTER_ITEM_FALSE;
                return reVal;
            case TEST_SUCCESSFUL_LIGHT_OFF:
                if(ignoreChar)
                {
                    if(MBtestFlag )
                    {}
                    else
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CONFIRM,90,y_offset);
                        //EPDDrawContainByIDPos(TRUE,EPD_PICT_KEY_CONFIRM,90,y_offset);
                    passItem++;
                }
                reVal = ACTION_TESTER_ITEM_TRUE;
                if(MBtestFlag )
                {
                    terninalPrintf("Test [successful] !!\r\n");
                    MTPString[MTPReportindex[i]][MTPString[MTPReportindex[i]][2]+2] = 0x81;
                    SetMTPCRC(MTPReportindex[i],(uint8_t*)MTPString);
                    MTPCmdprint(MTPReportindex[i],(uint8_t*)MTPString);    
                    //if(i == (allTestItem-1))
                    if(i == (MBallTestItem-1))
                    {
                        SetMTPCRC(38,(uint8_t*)MTPString);
                        MTPCmdprint(38,(uint8_t*)MTPString);
                    }
                    BuzzerPlay(300, 0, 1, TRUE);
                    vTaskDelay(1000/portTICK_RATE_MS);
                }
                else
                    testSuccessful(i,FALSE);
                break;
            case TEST_SUCCESSFUL_LIGHT_ON:
                if(ignoreChar)
                {
                    if(MBtestFlag )
                    {}
                    else
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_KEY_CONFIRM,90,y_offset);
                        //EPDDrawContainByIDPos(TRUE,EPD_PICT_KEY_CONFIRM,90,y_offset);
                    passItem++;
                }
                reVal = ACTION_TESTER_ITEM_TRUE;
                if(MBtestFlag )
                {
                    terninalPrintf("Test [successful] !!\r\n");
                    MTPString[MTPReportindex[i]][MTPString[MTPReportindex[i]][2]+2] = 0x81;
                    SetMTPCRC(MTPReportindex[i],(uint8_t*)MTPString);
                    MTPCmdprint(MTPReportindex[i],(uint8_t*)MTPString);  
                    //if(i == (allTestItem-1))
                    if(i == (MBallTestItem-1)) 
                    {
                        SetMTPCRC(38,(uint8_t*)MTPString);
                        MTPCmdprint(38,(uint8_t*)MTPString);
                    }
                    BuzzerPlay(300, 0, 1, TRUE);
                    vTaskDelay(1000/portTICK_RATE_MS);
                }
                else
                    testSuccessful(i,TRUE);
                break;
            case TEST_FALSE:	
                if(MBtestFlag )
                {
                    terninalPrintf("Test [Error] !!\r\n");
                    BuzzerPlay(80, 80, 3, TRUE);
                    testErroCode = 0x0000;          //cleaning
                }
                else                
                    testFailure(i);
                if(ignoreChar)
                {
                    setTesterFlag(FALSE);
                    if(MBtestFlag )
                    {}
                    else
                        EPDDrawContainByIDPos(FALSE,EPD_PICT_LOWER_A+23,90,y_offset-9);
                        //EPDDrawContainByIDPos(TRUE,EPD_PICT_LOWER_A+23,90,y_offset-9);
                    failItem++;
                    
                    if(MBtestFlag )
                    {
                    }
                    else
                    {
                        setTesterFlag(TRUE);
                        /*
                        //guiPrintMessageNoClean("\n\n\n\n\n+:Retry \n=:Next\n}:Exit\n");
                        EPDDrawString(TRUE,"+:Retry \n=:Next\n}:Exit\n",X_POS_MSG+250,Y_POS_MSG-250);
                        terninalPrintf("\r\n'y' to retry,\r\n'n' to continue,\r\n'q' to Exit\n");
                        while(1)
                        {
                            char tmp=userResponseLoop();
                            if(tmp=='y')
                            {
                                i--;
                                failItem--;
                                testedItem--;
                                setTesterFlag(TRUE);
                                break;
                            }
                            else if(tmp=='n')
                            {
                                setTesterFlag(TRUE);
                                break;
                            }
                            else if(tmp=='q')
                            {
                                cleanRst();
                                return reVal;
                            }
                            else
                            {
                                terninalPrintf("Wrong input!Please enter again!\r\n");
                            }
                        }
                        */
                    }
                }
                else
                {
                    reVal = ACTION_TESTER_ITEM_TRUE;
                }
                break;
            case TEST_TRUE:
                break;
            }
            terninalPrintf(" =====  [ End  ] =====\r\n\r\n");
            if(ignoreChar==FALSE)
            {
                break;
            }
            else
            {
                vTaskDelay(500/portTICK_RATE_MS);
            }
            //select item change to white
            //cleanMsg();
        }
    }
    return reVal;
}
 
static BOOL testSuccessful(int i,BOOL lightOff)
{
    if(EPDtestFlag)
        EPDtestFlag = FALSE;
    else
    {
        if(MBtestFlag )
        {}
        else
            guiPrintResult("PASS");
    }
    //guiPrintMessage(" ");
    terninalPrintf("Test [successful] !!\r\n");
    /*
    if(lightOff == TRUE)
    {
        LedSetColor(bayColorOff, LIGHT_COLOR_OFF, TRUE);	
        vTaskDelay(500/portTICK_RATE_MS);
    }
    */
    if(ModemResultLEDFlag)
    {
        ModemResultLEDFlag = FALSE;
        GPIO_SetBit(GPIOG, BIT7);
        
            //LedSetStatusLightFlush(50,10);  // solve LED board bug
        //LedSetColor(bayColorOff, 0x04, TRUE);
        LedSetBayLightFlush(0xff,8);
        vTaskDelay(100/portTICK_RATE_MS);
        LedSetColor(modemColorAllRed, 0x00, TRUE);
        
        BuzzerPlay(300, 0, 1, TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        LEDColorBuffSet(0x00, 0x00);
        LEDBoardLightSet();
        GPIO_ClrBit(GPIOG, BIT7);

    }
    else
    {
        LEDColorBuffSet(0xff, 0x00);
        LEDBoardLightSet();
        BuzzerPlay(300, 0, 1, TRUE);
        vTaskDelay(1000/portTICK_RATE_MS);
        LEDColorBuffSet(0x00, 0x00);
        LEDBoardLightSet();
    }
    return TRUE;
}

static BOOL testFailure(int i)
{	
    if(EPDtestFlag)
        EPDtestFlag = FALSE;
    else
    {
        if(MBtestFlag )
        {}
        else
            guiPrintResult("FAIL");
    }
    terninalPrintf("Test [Error] !!\r\n");
    //guiPrintMessageNoClean("");
    sysprintf("\r\n/*********  Erro Code : 0x%04x   *********/\r\n", testErroCode);
    
    if(ModemResultLEDFlag)
    {
        ModemResultLEDFlag = FALSE;  
        GPIO_SetBit(GPIOG, BIT7);        
           // LedSetStatusLightFlush(50,10);  // solve LED board bug   
        //LedSetColor(bayColorOff, 0x04, TRUE);        
        LedSetBayLightFlush(0xff,8);
        vTaskDelay(100/portTICK_RATE_MS);
        LedSetColor(modemColorAllGreen, 0x00, TRUE);
        
        BuzzerPlay(80, 80, 3, TRUE);
        testErroCode = 0x0000;          //cleaning
        
        GPIO_ClrBit(GPIOG, BIT7);
    }
    else
    {
        LEDColorBuffSet(0x00,0xff );
        LEDBoardLightSet();

        BuzzerPlay(80, 80, 3, TRUE);

        testErroCode = 0x0000;          //cleaning
    }
    return FALSE;
}

//static BOOL primaryTest()
//{
//    BuzzerPlay(100, 800, 5, TRUE);
//    vTaskDelay(4500/portTICK_RATE_MS);
//    while(1);
//}
static void hwInit(void){
    setPrintfFlag(TRUE);
    
    
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOJ, BIT4, DIR_INPUT, PULL_UP);
      
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOJ, BIT3, DIR_INPUT, PULL_UP);
    
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(GPIOJ, BIT2, DIR_INPUT, PULL_UP);
    
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(GPIOJ, BIT1, DIR_INPUT, PULL_UP);
    
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(GPIOJ, BIT0, DIR_INPUT, PULL_UP);
    
    
    if((GPIO_ReadBit(GPIOJ, BIT0)) && (!GPIO_ReadBit(GPIOJ, BIT1)) )
        MBtestFlag = TRUE;
    else if((GPIO_ReadBit(GPIOJ, BIT0)) && (GPIO_ReadBit(GPIOJ, BIT1)) )
        AssemblyTestFlag = TRUE;
    
    if(!GPIO_ReadBit(GPIOJ, BIT4))
        memcpy(ReaderitemName,"Octopus Reader",sizeof("Octopus Reader"));

    
    
    for(int i = 0; ; i++)
    {
        if(mInitFunctionList[i].func == NULL)
        {
            break;
        }
        if(mInitFunctionList[i].func(TRUE))
        {
            mInitFunctionList[i].result =TRUE;
            terninalPrintf(" * [%02d]: Initial %s OK...    *\r\n", i, mInitFunctionList[i].drvName);
        }
        else
        {
            terninalPrintf(" * [%02d]: Initial %s ERROR... *\r\n", i, mInitFunctionList[i].drvName);
        }
    }
    //if(mInitFunctionList[0].result==TRUE)
    //{
        if(GuiManagerInit()){
            isEPDInit = TRUE;
            terninalPrintf(" * Initial GuiManager  OK...    *\r\n");
        }
        else
        {
            terninalPrintf(" * Initial GuiManager  ERROR... *\r\n");
        }
    //}
        
    if(MBtestFlag)
    {
        //MBtestFlag = TRUE;
        for(int i=0;;i++)
        {
            if(MB_singleTestItem[i].itemName == NULL)
            {
                break;
            }
            allTestItem++;
            MBallTestItem++;
        }
        allTestItem-=1;
        MBallTestItem-=1;
    }
    else
    {
        for(int i=0;;i++)
        {
            if(singleTestItem[i].itemName == NULL)
            {
                break;
            }
            allTestItem++;
        }
        allTestItem-=1;
    }
    
    int toolsallTestItem = 0;
    for(int j=0;;j++)
    {
        if(toolsFunctionItem[j].itemName == NULL)
        {
            break;
        }
        toolsallTestItem++;
    }
    toolsallTestItem-=1;
    
    if(toolsallTestItem > allTestItem)  
        allTestItem = toolsallTestItem;
    
    KeyDrvInit();
    BatteryDrvInit(FALSE);
    
    

    
    /*
    // open 12V power
    
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOG, BIT6, DIR_OUTPUT, PULL_UP);
    GPIO_SetBit(GPIOG, BIT6);
    */
    
    /*
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOJ, BIT4, DIR_INPUT, PULL_UP);
    
    
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<12)) | (0x0<<12));
    GPIO_OpenBit(GPIOJ, BIT3, DIR_INPUT, PULL_UP);
    */
    /*
    //Set ModemRTS(PH10) output
    outpw(REG_SYS_GPH_MFPH,(inpw(REG_SYS_GPH_MFPH) & ~(0xF<< 8)) | (0<< 8));
    GPIO_OpenBit(GPIOH, BIT10, DIR_OUTPUT, PULL_UP);
    GPIO_SetBit(GPIOH, BIT10);
    */
    
    // battery control
    if(MBtestFlag)
    {
        BatterySetSwitch1(FALSE);
        BatterySetSwitch2(FALSE);
        
        //BatterySetSwitch1(TRUE);
        //BatterySetSwitch2(TRUE);
        
        outpw(REG_SYS_GPE_MFPH,(inpw(REG_SYS_GPE_MFPH) & ~(0xF<<24)));
        GPIO_OpenBit(GPIOE,BIT14, DIR_OUTPUT, NO_PULL_UP); 
        
        GPIO_ClrBit(GPIOE,BIT14);
    }
 
    
    #if(SUPPORT_HK_10_HW)
    //Sensor board DC5V GPB6
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOB, BIT6, DIR_OUTPUT, NO_PULL_UP);
    //GPIO_ClrBit(GPIOB, BIT6);
    GPIO_SetBit(GPIOB, BIT6);   
    
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xF<<28)) | (0x0<<28));
    GPIO_OpenBit(GPIOI, BIT15, DIR_OUTPUT, NO_PULL_UP);
    //GPIO_ClrBit(GPIOI, BIT15);
    
    
    #endif
}
/*-----------------------------------------*/
/* Exported Functions                      */
/*-----------------------------------------*/
BOOL HWTesterInit(void)
{   
    hwInit();
    setPrintfFlag(TRUE);
    vTaskDelay(1500/portTICK_RATE_MS);
    BuzzerPlay(100, 800, 1, TRUE);
    //LedSendHeartbeat();
    //LedSetColor(bayColorAllGreen, LIGHT_COLOR_OFF, TRUE);
    NT066EResetChip();
    
   //BuzzerPlay(500, 500, 1, TRUE);
   //BuzzerPlay(100, 100, 1, TRUE);
    //BuzzerPlay(100, 500, 5, TRUE);
   // BuzzerPlay(500, 200, 1, TRUE);
   // vTaskDelay(10000/portTICK_RATE_MS);
    
    //IT8951SetVCOM(100);
    
    
    //IT8951WriteReg(0x0004, 0x0001);
    
    //terninalPrintf("IT8951GetVCOM = %d \r\n",IT8951GetVCOM());
    

    //QModemSetPower(TRUE);
    //QueryNTPfun();

    
    int timeOut = 0;
    while(1)
    {
        int guiID = GUI_HW_TEST_ID;
        
        if(MBtestFlag )
            enterMunu(MENU_STRING_MAIN, MB_mainTestItem, (void*)&guiID, NULL);
        else
            enterMunu(MENU_STRING_MAIN, mainTestItem, (void*)&guiID, NULL);
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

BOOL MTPTesterInit(void)
{   
    hwInit();
    setPrintfFlag(TRUE);
    vTaskDelay(1500/portTICK_RATE_MS);
    BuzzerPlay(100, 800, 1, TRUE);
    char tempchar;
    guiManagerShowScreen(GUI_BLANK_ID, GUI_MTP_INI,0, 0);
    //vTaskDelay(5000/portTICK_RATE_MS);
    //GuiManagerCleanMessage(GUI_MTP_START);
    while(1)
    {
        tempchar = userResponseLoop();
        if(tempchar =='b')
        {
            GuiManagerCleanMessage(GUI_MTP_START);
            
            if (MTP_ProcedureInit() == FALSE)
            {
                terninalPrintf("blankFunction ==> MTP_ProcedureInit Failed !\r\n");
                continue;
            }
            
            //while(userResponseLoop() != 'b');
            
            while (1)
            {
                
                if (MTP_IsExitProcedure() == TRUE) {
                    break;
                }
                
            }
            
            MTP_ProcedureDeInit();
            terninalPrintf("Exit MTP Procedure!\r\n");
            GuiManagerCleanMessage(GUI_MTP_SCREEN);
            continue;

        }                          
    }
    
    
}

/*-----------------------------------------*/
/* KeyPad Listener                        */
/*-----------------------------------------*/
BOOL CallBackReturnValue(uint8_t keyId, uint8_t downUp)
{
    //terninalPrintf("targetkeyStage = %d\n",targetkeyStage);
    //terninalPrintf("keyId = %d\n",keyId);
    if(targetkeyStage == keyId)
    {
        callBackKeyId = keyId;
        callBackDownUp = downUp;
        if(GUI_KEY_DOWN_INDEX == downUp)
        {
            BuzzerPlay(200, 500, 1, TRUE);
        }
    }
    return 0;
}
/*-----------------------------------------*/
/* getter and setter                       */
/*-----------------------------------------*/
static char getGuiResponse(void){
    guiResponseFlag=FALSE;
    return guiResponseChar;
}

static void setTesterFlag(BOOL testerflag){
    testerFlag=testerflag;
}

void SetGuiResponseVal(char guiresponsechar){
    guiResponseFlag = TRUE;
    guiResponseChar = guiresponsechar;
}

BOOL GetTesterFlag(void){
    return testerFlag;
}

int GetDeviceIDString(void){
    return getDeviceID(0, 0);
}

BOOL SetDeviceIDString(void* para1,void* para2){
    int deviceID=0;
    char* deviceIDString=(char*)para1;
    for(int i=0;i<4;i++)
    {
        deviceID += (deviceIDString[i]-'0');
        deviceID *= 10;
    }
    deviceID += deviceIDString[4]-'0';
    epmIDSet = deviceID;
    return TRUE;
}


void hwtestEDPflashBurn(void)
{
    char tempchar;    
    int lastvcomValue;
    int modifyvcomValue;
    int newvcomValue; 
    int recoveryvcomValue;
    char lastVCOMString[9];
    char modifyVCOMString[10];
    char recoveryVCOMString[12];
    
    
    for(int i = 0; ; i++)
    {//Dont Show Quit
        if(EPDflashToolListItem[i].charItem == 0)
        {
            break;
        }
        terninalPrintf("    %c) [%s]\r\n", EPDflashToolListItem[i].charItem, EPDflashToolListItem[i].itemName);
        
    }   
    BOOL fExit = FALSE;
    while(1)
    {
        if(fExit)
            break;
        terninalPrintf("--> ");
        tempchar = userResponseLoop();
        terninalPrintf("%c",tempchar);
        switch(tempchar)
        {
            case 's':
                setW25Q64BVspecialburn();
                W25Q64BVburn();
                clrW25Q64BVspecialburn();
                break;
            case 'd':
                W25Q64BVdeviceID();
                break;
            case 'p':
                W25Q64BVFetch();
                break;
            
            case 'v':
                terninalPrintf("\r\nVCOM value = %d \r\n",IT8951GetVCOM());
                break;

            case 'u':

                if(W25Q64BVQuery())
                {
                    //EPDDrawString(TRUE,"OK   ",300+400,100+(44*(2)));
                }
                else
                {
                    //EPDDrawString(TRUE,"FAIL ",300+400,100+(44*(2)));
                }
                break;
            case 'e':
                W25Q64BVerase();
                
                break;
            case '0':

                terninalPrintf("\r\n =====  [0)FACTORY TEST Start] =====\r\n");
                vTaskDelay(500/portTICK_RATE_MS);
                //lastvcomValue = IT8951GetVCOM();

                //terninalPrintf("VCOM value before burn = %d \r\n",lastvcomValue);
                //sprintf(lastVCOMString,"VCOM:%d   ",lastvcomValue );
            
                if(SetEPDVCOMEx())
                {
                    //modifyvcomValue = IT8951GetVCOM();
                    //terninalPrintf("New VCOM value = %d \r\n",*setVCOMpoint);
                    terninalPrintf("New VCOM value = %d \r\n",setVCOMEx);
                    vTaskDelay(1500/portTICK_RATE_MS);
                }

                if(W25Q64BVburn())
                {
                    newvcomValue = IT8951GetVCOM();
                    terninalPrintf("VCOM value after burn = %d \r\n",newvcomValue);
                    IT8951SetVCOM(*setVCOMpoint);
                    recoveryvcomValue = IT8951GetVCOM();
                    terninalPrintf("Recovery VCOM value = %d \r\n",recoveryvcomValue);

                    toggleEPDswitch(TRUE);
                    //vTaskDelay(5000/portTICK_RATE_MS);
                    EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE);
                    terninalPrintf("Enter EPD all black,press \"space\" to continue.\r\n");
                    while(userResponseLoop() != 0x20);
                    EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
                    terninalPrintf("Enter EPD all white,press \"space\" to continue.\r\n");
                    while(userResponseLoop() != 0x20);
                    EPDSetSleepFunction(TRUE);
                    terninalPrintf("Enter EPD sleep mode,press \"space\" to continue.\r\n");
                    while(userResponseLoop() != 0x20);
                    EPDSetSleepFunction(FALSE);
                    
                    //fExit = TRUE;
     
                    
                }
                else
                {
                    terninalPrintf("BURN FAIL\r\n");
                }

                break;
                /*
            case '1':


                terninalPrintf("\r\n =====  [1)SD Burn Start] =====\r\n");
                lastvcomValue = IT8951GetVCOM();

                terninalPrintf("VCOM value before burn = %d \r\n",lastvcomValue);

            
                if(W25Q64BVburn())
                {
                    newvcomValue = IT8951GetVCOM();
                    terninalPrintf("VCOM value after burn = %d \r\n",newvcomValue);
                    IT8951SetVCOM(modifyvcomValue);
                    recoveryvcomValue = IT8951GetVCOM();
                    terninalPrintf("Recovery VCOM value = %d \r\n",recoveryvcomValue);
                 
     
                    
                }
                else
                {
                    terninalPrintf("BURN FAIL\r\n");
                }
                

                break;
                */
            case '1':
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(3)));
                //EPDDrawString(FALSE,"        ",300+400,100+(44*(4)));
                //EPDDrawString(TRUE,"Wait",300+400,100+(44*(2)));
                terninalPrintf("\r\n =====  [2)EPD VCOM Start] =====\r\n");
                int tempVCOM;
                char VCOMString[5];
                tempVCOM = IT8951GetVCOM();
                terninalPrintf("\r\nLast VCOM value = %d \r\n",tempVCOM );

                if(SetEPDVCOM())
                {
                    vTaskDelay(500/portTICK_RATE_MS);
                    tempVCOM = IT8951GetVCOM();
                    terninalPrintf("New VCOM value = %d \r\n",tempVCOM);

                }
                else
                {
                    //EPDDrawString(TRUE,"FAIL   ",90+400,100+(44*(4)));
                }
                
                             
                break;
            
            case '2':
                terninalPrintf("\r\n =====  [3)EPD Burning Test Start] =====\r\n");
                epdBurningTest(NULL,NULL);
                break;
            case '3':  
                terninalPrintf("\r\n =====  [4)EPD All Black Start] =====\r\n");
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_BLACK_INDEX,0,0);
                EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE);            
                terninalPrintf("\r\nPress 'q' to Exit Test\n");
                while(userResponseLoop()!='q');            
                GuiManagerCleanMessage(GUI_REDRAW_PARA_CONTAIN);
                

                break;    
            case '4':
                terninalPrintf("\r\n =====  [5)EPD All White Start] =====\r\n");
                //EPDDrawContainByIDPos(TRUE,EPD_PICT_ALL_WHITE_INDEX,0,0);
                EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);              
                terninalPrintf("\r\nPress 'q' to Exit Test\n");
                while(userResponseLoop()!='q');            
                GuiManagerCleanMessage(GUI_REDRAW_PARA_CONTAIN);
                
                break;              
            case '5':
                //setPrintfFlag(FALSE);
                if(readEPDswitch() == TRUE)
                {
                    //EPDDrawString(TRUE,"DISABLE",300+450,100+(44*(7)));
                    EPDDrawString(TRUE,"DISABLE",90+325,100+(44*(8)));
                    //terninalPrintf("\r\nEPD display disable.\r\n");
                    //terninalPrintf("\r\n =====  [1)EPD display disable] =====\r\n");
                    terninalPrintf("\r\n =====  [6)EPD DISP Start] =====\r\n");
                    terninalPrintf("EPD display disable.\r\n");
                    toggleEPDswitch(FALSE);

                }
                else
                {
                    terninalPrintf("\r\n =====  [6)EPD DISP Start] =====\r\n");
                    vTaskDelay(500/portTICK_RATE_MS);
                    toggleEPDswitch(TRUE);
                    //GuiManagerInit();
                    EPDSetBacklight(FALSE);
                    //terninalPrintf("\r\nEPD display enable.\r\n");
                    //terninalPrintf("\r\n =====  [1)EPD display enable] =====\r\n");
                    terninalPrintf("EPD display enable.\r\n");
                    //setSpecialCleanFlag(TRUE);
                    //EPDDrawString(TRUE,"$$$$$$$",300+450,100+(44*(7)));
                    EPDDrawString(TRUE,"$$$$$$$",90+325,100+(44*(8)));
                    //EPDDrawString(TRUE,"------  ",300+450,100+(44*(3)));
                   // EPDDrawContainByIDPos(FALSE,EPD_PICT_LINE_SMALL_2_INDEX ,300,100+(44*(oldIndex+2)));
                    //EPDDrawString(TRUE,"ENABLE  ",300+450,100+(44*(7)));
                    EPDDrawString(TRUE,"ENABLE  ",90+325,100+(44*(8)));
                    vTaskDelay(1000/portTICK_RATE_MS);
                }
            
            
                //EPDDrawString(TRUE,"no Function",300+400,100+(44*(4)));
                break;

            case 'q':
                terninalPrintf("\r\n =====  [q)Quit Start] =====\r\n");
                fExit = TRUE;
                break;
            default:
                terninalPrintf("\r\nerror input");
                break;
        }

        if(fExit)
            break;
        terninalPrintf(" =====  [ End  ] =====\r\n");
        
        
        terninalPrintf("\r\n =====  [a)EPD Tool Start] =====\r\n");
        for(int i = 0; ; i++)
        {//Dont Show Quit
            if(EPDflashToolListItem[i].charItem == 0)
            {
                break;
            }
            terninalPrintf("    %c) [%s]\r\n", EPDflashToolListItem[i].charItem, EPDflashToolListItem[i].itemName);
        
        }
    }
    //return TRUE;
    
    
    
    
    
    /*
    char tempchar;
    terninalPrintf("Press \"Enter\" to Burn EPD or \'s\' to special burning or \'q\' to quit.\r\n");
    tempchar = userResponseLoop();
    //setTesterFlag(TRUE);
    //terninalPrintf("%c   %02x \r\n",tempchar ,tempchar );
    terninalPrintf("%c\r\n",tempchar);
    //GuiManagerCleanMessage(GUI_CLEAN_MESSAGE_ENABLE);
    switch(tempchar)
    {
        case 0x0:
            W25Q64BVburn();
            break;
        case 'u':
            W25Q64BVQuery();
            break;
        case 'e':
            W25Q64BVerase();
            break;
        case 's':
            setW25Q64BVspecialburn();
            W25Q64BVburn();
            clrW25Q64BVspecialburn();
            break;
        case 'd':
            W25Q64BVdeviceID();
            break;
        case 'q':
            //terninalPrintf("\r\n =====  [q)Quit Start] =====\r\n");
            //fExit = TRUE;
            break;
        
        default:
            terninalPrintf("\r\nerror input\r\n");
            break;
    }
            
            */
            
    
    
    
}


void hwtestEDPflashBurnLite(void)
{
    char tempchar;    
    int lastvcomValue;
    int modifyvcomValue;
    int newvcomValue; 
    int recoveryvcomValue;
    char lastVCOMString[9];
    char modifyVCOMString[10];
    char recoveryVCOMString[12];
    
    terninalPrintf("\r\n =====  [EPD Emergent Tool Start] =====\r\n");
    for(int i = 0; ; i++)
    {//Dont Show Quit
        if(EPDEmergentToolListItem[i].charItem == 0)
        {
            break;
        }
        terninalPrintf("    %c) [%s]\r\n", EPDEmergentToolListItem[i].charItem, EPDEmergentToolListItem[i].itemName);
        
    }   
    BOOL fExit = FALSE;
    while(1)
    {
        if(fExit)
            break;
        terninalPrintf("--> ");
        tempchar = userResponseLoop();
        terninalPrintf("%c",tempchar);
        switch(tempchar)
        {
            case '0':
                setW25Q64BVspecialburn();
                W25Q64BVburn();
                clrW25Q64BVspecialburn();
                break;
            case 'q':
                terninalPrintf("\r\n =====  [q)Quit Start] =====\r\n");
                fExit = TRUE;
                break;
            default:
                terninalPrintf("\r\nerror input");
                break;
        }

        if(fExit)
            break;
        terninalPrintf(" =====  [ End  ] =====\r\n");
        
        terninalPrintf("\r\n =====  [EPD Emergent Tool Start] =====\r\n");
        for(int i = 0; ; i++)
        {//Dont Show Quit
            if(EPDEmergentToolListItem[i].charItem == 0)
            {
                break;
            }
            terninalPrintf("    %c) [%s]\r\n", EPDEmergentToolListItem[i].charItem, EPDEmergentToolListItem[i].itemName);
        
        }
    }
    
}
/*
uint32_t GetCardID(void){
    return cardID;
}*/

BOOL readMBtestFunc(void)
{
    return MBtestFlag;
}

BOOL readAssemblyTestFunc(void)
{
    return AssemblyTestFlag;
}

void HwTestReceieveU32Func(UINT32 tempVal)
{
    ReceieveTouchPadVal = tempVal;
}
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


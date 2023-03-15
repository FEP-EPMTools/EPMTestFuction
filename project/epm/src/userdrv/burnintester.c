/**************************************************************************//**
* @file     burnintester.c
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2020 Far Easy Pass LTD. All rights reserved.
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
#include "timelib.h"
#include "buzzerdrv.h"
#include "leddrv.h"
#include "flashdrvex.h"
#include "epddrv.h"
#include "cardreader.h"
#include "nt066edrv.h"
#include "modemagent.h"
#include "photoagent.h"
#include "powerdrv.h"
#include "batterydrv.h"
#include "spaceexdrv.h"
#include "guimanager.h"
#include "guidrv.h"
#include "smartcarddrv.h"
#include "cardreader.h"
#include "fatfslib.h"
#include "fileagent.h"
#include "hwtester.h"
#include "burnintester.h"
#include "sflashrecord.h"
#include "dipdrv.h"
#include "photoagent.h"
#include "guiburnintester.h"
#include "creditReaderDrv.h"
#include "../octopus/octopusreader.h"
#include "quentelmodemlib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

typedef BOOL(*initFunction)(BOOL testModeFlag);
typedef struct
{
    char*               drvName;
    initFunction        func;
    BOOL                runTestMode;
} initFunctionList;

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

static void versionQueryFunc(void);

static initFunctionList mInitFunctionList[] =  {{"EpdDrv", EpdDrvInit, FALSE},
                                                {"LEDDrv", LedDrvInit, FALSE},
                                                {"BuzzerDrv", BuzzerDrvInit, FALSE},
                                                {"GUIDrv", GUIDrvInit, FALSE},
                                                {"TouchDrv(NT066E)", NT066EDrvInit, TRUE},
                                                {"CardReaderDrv", CardReaderInit, FALSE},
                                                {"PowerDrv", PowerDrvInit, FALSE},
                                                {"FlashExDrv", FlashDrvExInit, FALSE},
                                                {"ModemAgentDrv", ModemAgentInit, FALSE},
                                                {"BatteryDrv", BatteryDrvInit, FALSE},
                                                {"SpaceExDrv", SpaceExDrvInit, FALSE},
                                                {"SmartCardDrv", SmartCardTestInit, FALSE},
                                                {"FATFS", FatfsInit, FALSE},
                                                {"UVCameraDrv", UVCameraTestInit, FALSE},
                                                {"CreditReaderDrv", CreditReaderDrvInit, FALSE},
                                                {"", NULL, FALSE}};

static time_t startUTCTime;
static RTC_TIME_DATA_T startRTCTime;
static BOOL isEnabledBurninTestMode = FALSE;
static uint32_t deviceID;
static uint32_t specificBurninTime = 0;
static uint32_t ledBurninCounter = 0;
static uint32_t ledBurninErrorCounter = 0;
static uint32_t batteryBurninCounter[BATTERY_NUM] = {0};
static uint32_t batteryBurninErrorCounter[BATTERY_NUM] = {0};
static uint32_t batteryLastADCValue[BATTERY_NUM] = {0};
static uint32_t nandFlashBurninCounter[NAND_FLASH_NUM] = {0};
static uint32_t nandFlashBurninErrorCounter[NAND_FLASH_NUM] = {0};
static char burninTestReportBuffer[BURNIN_TEST_REPORT_BUFFER_LENGTH];
static BOOL prepareStopBurninTest = FALSE;
static BOOL lastTestReportDone_SD = FALSE;
static BOOL lastTestReportDone_FTP = FALSE;
static BOOL ErrorDiableFlag = FALSE;


static char LEDVerStr[50];
static char ReaderVerBuf[64];
static char  preadFWVersion[17] , preadLUTVersion[17];
static char SIMtempchr[100] ;
static char tempchr[100];
//static char MODEMchr[100];
static char tempRadar1VersionString[50];
static char tempRadar2VersionString[50];
static BOOL FirstRun = TRUE;

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static void hwInit(void)
{
    for (int i = 0 ; ; i++)
    {
        if (mInitFunctionList[i].func == NULL)
        {
            break;
        }
        if (mInitFunctionList[i].func(mInitFunctionList[i].runTestMode))
        {
            //mInitFunctionList[i].result =TRUE;
            terninalPrintf(" * [%02d]: Initial %s OK...    *\r\n", i, mInitFunctionList[i].drvName);
        }
        else
        {
            terninalPrintf(" * [%02d]: Initial %s ERROR... *\r\n", i, mInitFunctionList[i].drvName);
            //if(memcmp("FATFS",mInitFunctionList[i].drvName,sizeof(mInitFunctionList[i].drvName)))
            //    ErrorDiableFlag = TRUE;
        }
    }
    if (GuiManagerInit())
    {
        terninalPrintf(" * Initial GuiManager OK...    *\r\n");
    }
    else
    {
        terninalPrintf(" * Initial GuiManager ERROR... *\r\n");
    }
    
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xFu<<24)) | (0x0u<<24));
    //GPIO_OpenBit(GPIOJ, BIT4, DIR_INPUT, PULL_UP);
    GPIO_OpenBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN, DIR_INPUT, PULL_UP);
    GPIO_OpenBit(DIP_BURNIN_TEST_PORT, DIP_BURNIN_TEST_PIN, DIR_INPUT, PULL_UP);
    
    #if(SUPPORT_HK_10_HW)
    //Sensor board DC5V GPB6
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xFu<<24)) | (0x0u<<24));
    GPIO_OpenBit(GPIOB, BIT6, DIR_OUTPUT, NO_PULL_UP);
    GPIO_ClrBit(GPIOB, BIT6);
    
    outpw(REG_SYS_GPI_MFPH,(inpw(REG_SYS_GPI_MFPH) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(GPIOI, BIT15, DIR_OUTPUT, NO_PULL_UP);
    //GPIO_ClrBit(GPIOI, BIT15);
    #endif
}

static float calcSuccessRate(uint32_t totalCount, uint32_t errorCount)
{
    float rate;
    if (totalCount == 0) {
        rate = 0.0f;
    }
    else {
        rate = (float)(totalCount - errorCount) / (float)totalCount;
    }
    return (rate * 100.0);
}

static BOOL burninLedTest(void)
{
    int ledIndex = 0;
    BOOL status1, status2 = TRUE;
    for (int i = 0 ; i < 2 ; i++)
    {
        LEDColorBuffSet((1 << ledIndex), LIGHT_COLOR_OFF);
        status1 = LEDBoardLightSet();
        if (!status1) {status2 = FALSE;}
        vTaskDelay(BURNIN_LED_GAP_TIME / portTICK_RATE_MS);
        LEDColorBuffSet(LIGHT_COLOR_OFF, (1 << ledIndex));
        status1 = LEDBoardLightSet();
        if (!status1) {status2 = FALSE;}
        vTaskDelay(BURNIN_LED_GAP_TIME / portTICK_RATE_MS);
        ledIndex = 5;
    }
    LEDColorBuffSet(LIGHT_COLOR_OFF, LIGHT_COLOR_OFF);
    status1 = LEDBoardLightSet();
    if (!status1) {status2 = FALSE;}
    vTaskDelay(BURNIN_LED_GAP_TIME / portTICK_RATE_MS);
    //LedSetColor(NULL, 0x01, TRUE);
    //vTaskDelay(BURNIN_LED_GAP_TIME / portTICK_RATE_MS);
    //LedSetColor(NULL, 0x02, TRUE);
    //vTaskDelay(BURNIN_LED_GAP_TIME / portTICK_RATE_MS);
    //LedSetColor(NULL, LIGHT_COLOR_OFF, TRUE);
    return status2;
}

static void vLedBuzzerTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    terninalPrintf("vLedBuzzerTestTask Going...\r\n");
    while (TRUE)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vLedBuzzerTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_LED_BUZZER_INTERVAL)
        {
            //terninalPrintf("vLedBuzzerTestTask heartbeat.\r\n");
            lastTime = currentTime;
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        testLoop = FALSE;
        if (burninLedTest() == FALSE) {
            ledBurninErrorCounter++;
        }
        BuzzerPlay(BURNIN_BUZZER_PERIOD, BURNIN_BUZZER_GAP_TIME, BURNIN_BUZZER_PLAY, TRUE);
        ledBurninCounter++;
        //terninalPrintf("LedBurninCounter = %d\r\n", GetLedBurninTestCounter());
        //lastTime = GetCurrentUTCTime();
    }
}

static void vBatteryTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    uint32_t leftVoltage, rightVoltage;
    terninalPrintf("vBatteryTestTask Going...\r\n");
    
    BatterySetEnableTestMode(TRUE);
    while (TRUE)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vBatteryTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_BATTERY_INTERVAL)
        {
            //terninalPrintf("vBatteryTestTask heartbeat.\r\n");
            lastTime = currentTime;
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        testLoop = FALSE;
        BatteryGetVoltage();
        vTaskDelay(500 / portTICK_RATE_MS);
        BatteryGetValue(NULL,&leftVoltage, &rightVoltage);
        batteryLastADCValue[BATTERY_INDEX_0] = leftVoltage;
        batteryLastADCValue[BATTERY_INDEX_1] = rightVoltage;
        batteryLastADCValue[BATTERY_INDEX_SOLAR] = SolarBatteryGetValue();
        batteryBurninCounter[BATTERY_INDEX_0]++;
        batteryBurninCounter[BATTERY_INDEX_1]++;
        batteryBurninCounter[BATTERY_INDEX_SOLAR]++;
        
        if (leftVoltage < BURNIN_BATTERY_BOUNDARY) {
            batteryBurninErrorCounter[BATTERY_INDEX_0]++;
        }
        if (rightVoltage < BURNIN_BATTERY_BOUNDARY) {
            batteryBurninErrorCounter[BATTERY_INDEX_1]++;
        }
        if (batteryLastADCValue[BATTERY_INDEX_SOLAR] < BURNIN_BATTERY_BOUNDARY) {
            batteryBurninErrorCounter[BATTERY_INDEX_SOLAR]++;
        }
        //terninalPrintf("leftVoltage = %d, rightVoltage = %d\r\n", leftVoltage, rightVoltage);
    }
}

static void vNandFlashTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    BOOL flashStatus[NAND_FLASH_NUM] = {FALSE};
    uint16_t uID;
    int index;
    terninalPrintf("vNandFlashTestTask Going...\r\n");
    
    flashStatus[FLASH_INDEX_0] = FlashDrvExInitialize(SPI_FLASH_EX_0_INDEX);
    flashStatus[FLASH_INDEX_1] = FlashDrvExInitialize(SPI_FLASH_EX_1_INDEX);
    flashStatus[FLASH_INDEX_2] = FlashDrvExInitialize(SPI_FLASH_EX_2_INDEX);
    
    while (TRUE)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vNandFlashTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_NAND_FLASH_INTERVAL)
        {
            //terninalPrintf("vNandFlashTestTask heartbeat.\r\n");
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        testLoop = FALSE;
        for (index = FLASH_INDEX_0 ; index <= FLASH_INDEX_2 ; index++)
        {
            if (flashStatus[index] == FALSE) {
                nandFlashBurninErrorCounter[index]++;
            }
            else
            {
                uID = FlashDrvExGetChipID(FLASH_INDEX_0 + 1);
                //terninalPrintf("vNandFlashTestTask ==> Flash[%d] uID=0x%x\r\n", index, uID);
                if (uID != 0xEF16) {
                    nandFlashBurninErrorCounter[index]++;
                }
            }
            nandFlashBurninCounter[index]++;
        }
        lastTime = GetCurrentUTCTime();
    }
}

static void versionQueryFunc(void)
{
    terninalPrintf("------- Version -------\r\n"); 
    //---LED---
    uint8_t VerCode1,VerCode2,VerCode3,YearCode,MonthCode,DayCode,HourCode,MinuteCode;
    //char LEDVerStr[50];
    vTaskDelay(1000/portTICK_RATE_MS);
    if(QueryVersion(&VerCode1,&VerCode2,&VerCode3,&YearCode,&MonthCode,&DayCode,&HourCode,&MinuteCode))
    {
        //sprintf(LEDVerStr,"Ver %d.%02d.%02d build %d%d%d%d%d",VerCode1,VerCode2,VerCode3,YearCode,MonthCode,
         //                                                     DayCode,HourCode,MinuteCode);
        sprintf(LEDVerStr,"Ver %d.%02d.%02d ",VerCode1,VerCode2,VerCode3);
    }
    else
        sprintf(LEDVerStr,"Error");
    terninalPrintf("1.LED:%s\r\n",LEDVerStr); 
    
    //---Reader---
    int waitCounter = 15;
    int ReaderStatus;
    //char ReaderVerBuf[64];
    if(!GPIO_ReadBit(GPIOJ, BIT4))
    {
    
        CardReaderInit(FALSE);
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
        //terninalPrintf("READER CHECK ");

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
                //terninalPrintf("\nCHECK READER [Time Out]\n");
                sprintf(ReaderVerBuf,"Error");
                break;
            }
        }while(ReaderStatus != TSREADER_CHECK_READER_OK);

        //terninalPrintf("ReaderVerBuf = %s\r\n",ReaderVerBuf);
        
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI,FALSE);
    }
    else
    {
       
        CardReaderInit(TRUE);
        CardReaderSetPower(EPM_READER_CTRL_ID_GUI, TRUE);
        //terninalPrintf("READER CHECK ");

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
                sprintf(ReaderVerBuf,"Error");
                break;
            }
        }while(ReaderStatus != TSREADER_CHECK_READER_OK);

        //terninalPrintf("ReaderVerBuf = %s\r\n",ReaderVerBuf);

    }  
    terninalPrintf("2.READER:%s\r\n",ReaderVerBuf); 
    
    //---EPD---
    
    //char  preadFWVersion[17] , preadLUTVersion[17];
    preadFWVersion[16]  = 0x00;
    preadLUTVersion[16] = 0x00;
    ReadIT8951SystemInfoLite( preadFWVersion, preadLUTVersion);
    if (preadFWVersion == NULL)
        sprintf(preadFWVersion,"Error");
    
    terninalPrintf("3.EPD:%s\r\n",preadFWVersion); 
    //terninalPrintf("readFWVersion=%s\r\n",preadFWVersion);
    //terninalPrintf("readLUTVersion=%s\r\n",preadLUTVersion);
    
    //---SIM---
    char retVer[100],SIMStr[100];
    //char tempchr[100];
    //char SIMtempchr[100] ;
    char* pch1= malloc(100);
    char* SIMpch1= malloc(100);
    char* pch2;
    char* SIMpch2;
    BOOL QModemGetSIMNumberFlag ;
    QModemGetSIMNumberFlag = QModemGetSIMNumber(SIMStr);

    if(QModemGetSIMNumberFlag)
    {           
        memcpy(SIMtempchr,SIMStr,100);
        SIMpch1 = (char*) memchr(SIMtempchr,'\n',100); 
        memcpy(SIMtempchr,SIMpch1+1,99);    
        SIMpch2 = (char*) memchr(SIMtempchr,'\n',100);
        memset (SIMpch2,'\0',1);
    
    }
    else
    {
        sprintf(SIMtempchr,"Error");
    }
    
    for(int g=0;g<sizeof(SIMtempchr);g++)
    {
        if((SIMtempchr[g] < 0x20) || (SIMtempchr[g] > 0x7E) )
            SIMtempchr[g] = 0x00;
    }

    terninalPrintf("4.SIM:%s\r\n",SIMtempchr); 
    
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
    }
    else
    {
        sprintf(tempchr,"Error");
    }
    //sysprintf(MODEMchr,"%s",tempchr);
    
    for(int m=0;m<sizeof(tempchr);m++)
    {
        if((tempchr[m] < 0x20) || (tempchr[m] > 0x7E) )
            tempchr[m] = 0x00;
    }
    
    terninalPrintf("5.MODEM:%s\r\n",tempchr);
    
    
    //---RADAR---

    BOOL changeFlag;
    RadarInterface* pRadarInterface;
    //char tempRadar1VersionString[50];
    //char tempRadar2VersionString[50];
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

            //terninalPrintf("Radar%d Version : Ver.%d.%d.%d.%d\r\n",j+1,RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
            
            if(j == 0)
            {
                sprintf(tempRadar1VersionString,"Ver %d.%d.%d.%d",RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
            }
            else if(j == 1)
            {
                sprintf(tempRadar2VersionString,"Ver %d.%d.%d.%d",RadarData[0],RadarData[1],RadarData[2],RadarData[3]);
            }
        }
        else
        {
            //terninalPrintf("Radar%d Version : Error\r\n",j+1);

            if(j == 0)
                sprintf(tempRadar1VersionString,"Error");
            else if(j == 1)
                sprintf(tempRadar2VersionString,"Error");                
        }
        
        if(j == 0)
            terninalPrintf("6.Radar1:%s\r\n",tempRadar1VersionString);
        else if(j == 1)
            terninalPrintf("7.Radar2:%s\r\n",tempRadar2VersionString);  
        
    }

    pRadarInterface->setPowerStatusFunc(0,FALSE);
    pRadarInterface->setPowerStatusFunc(1,FALSE);
    
    terninalPrintf("-----------------------\r\n"); 
}


/*-----------------------------------------*/
/* Exported Functions                      */
/*-----------------------------------------*/

BOOL CalibRTCviaNTP()
{
    //GUIDrvInit(FALSE);
    //PowerDrvInit(FALSE);
    //PowerDrvInit(FALSE);
    //ModemAgentInit(FALSE);
    setPrintfFlag(TRUE);
    QModemLibInit(921600);
    QueryNTPfun();
    return TRUE;
}


//copy from MtpProcedure.c
#define SET_DEVICE_PARAMETER_MESSAGE_LENGTH     32
#define MANUFACTURE_DEVICE_ID_LENGTH            (SET_DEVICE_PARAMETER_MESSAGE_LENGTH - sizeof(uint32_t) - sizeof(uint32_t))
static uint8_t manufactureDeviceID[MANUFACTURE_DEVICE_ID_LENGTH];

BOOL BurninTesterInit(void)
{
    isEnabledBurninTestMode = TRUE;
    setPrintfFlag(TRUE);
    hwInit();
    BuzzerPlay(100, 800, 1, TRUE);
    NT066EResetChip();
    LEDColorBuffSet(0x00, 0x00);
    LEDBoardLightSet();
    versionQueryFunc();
    GuiManagerShowScreen(GUI_BURNIN_TESTER_ID, GUI_REDRAW_PARA_REFRESH, 0, 0);
    EPDSetBacklight(FALSE);
    //terninalPrintf("FTP_ADDRESS = %s \r\n",FTP_ADDRESS);
    //terninalPrintf("FTP_ID = %s \r\n",FTP_ID);
    //terninalPrintf("FTP_PASSWD = %s \r\n",FTP_PASSWD);
    //terninalPrintf("FTP_PORT = %s \r\n",FTP_PORT);
    uint8_t readBuffer[SET_DEVICE_PARAMETER_MESSAGE_LENGTH];
    
    if (SFlashLoadStorage(SFLASH_STORAGE_EPM_SERIAL_ID_INDEX_BASE, readBuffer, sizeof(readBuffer)))
    {
        deviceID = *((uint32_t *)(&readBuffer[0]));
        specificBurninTime = *((uint32_t *)(&readBuffer[4]));
        //terninalPrintf("specificBurninTime = %08x\r\n",specificBurninTime);
        if (specificBurninTime == 0xFFFFFFFF) {
            specificBurninTime = 0;
        }
        if (specificBurninTime > 0) {
            specificBurninTime = specificBurninTime * 60 - 10;  //Minutes to seconds
        }
        memcpy(manufactureDeviceID, &readBuffer[8], MANUFACTURE_DEVICE_ID_LENGTH);
        //specificBurninTime = 890;   //Timer Test
        terninalPrintf(" >> DeviceID = %08d, manufactureDeviceID = %s, specificBurninTime:%d (from SFlash Record) read OK\n", deviceID, manufactureDeviceID, specificBurninTime);
    }
    xTaskCreate(vLedBuzzerTestTask, "vLedBuzzerTestTask", 1024*5, NULL, LED_BUZZER_TEST_THREAD_PROI, NULL);
    xTaskCreate(vBatteryTestTask, "vBatteryTestTask", 1024*5, NULL, BATTERY_TEST_THREAD_PROI, NULL);
    xTaskCreate(vNandFlashTestTask, "vNandFlashTestTask", 1024*5, NULL, NAND_FLASH_TEST_THREAD_PROI, NULL);
    RTC_Read(RTC_CURRENT_TIME, &startRTCTime);
    startUTCTime = GetCurrentUTCTime();
    //versionQueryFunc();
    return TRUE;
}

void NoticeFTPReportDone(void)
{
    if (prepareStopBurninTest) {
        lastTestReportDone_FTP = TRUE;
    }
}

void NoticeSDReportDone(void)
{
    if (prepareStopBurninTest) {
        lastTestReportDone_SD = TRUE;
    }
}

BOOL GetBurninTerminatedFlag(void)
{
    return (lastTestReportDone_FTP & lastTestReportDone_SD);
}

void BurninTestingMonitor(void)
{
    if (specificBurninTime == 0) {
        return;
    }
    
    terninalPrintf("BurninTestingMonitor Going...\r\n");
    uint32_t currentRuntime;
    while (TRUE)
    {
        currentRuntime = GetCurrentUTCTime() - startUTCTime;
        if (currentRuntime >= specificBurninTime) {
            prepareStopBurninTest = TRUE;
        }
        if ((prepareStopBurninTest) && (lastTestReportDone_FTP) && (lastTestReportDone_SD))
        {
            GuiBurninTesterStop();
            break;
        }
        vTaskDelay(500 / portTICK_RATE_MS);
    }
    terninalPrintf("BurninTestingMonitor Terminated !!\r\n");
}

char* BuildBurninTestReport(RTC_TIME_DATA_T *pt)
{
    char stringBuffer[256];
    uint32_t totalCount, errorCount;
    uint32_t hours, minutes, seconds;
    
    //Report Title, Program Version, Time Parameters
    memset(burninTestReportBuffer, 0x00, BURNIN_TEST_REPORT_BUFFER_LENGTH);
    strcat(burninTestReportBuffer, "EPM Burn/In Test Report\r\n");
    //sprintf(stringBuffer, "Program Version: %s\r\n", BURNIN_TEST_VERSION);
    sprintf(stringBuffer, "Program Version: %d.%02d.%02d\r\n", MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION);
    strcat(burninTestReportBuffer, stringBuffer);
    sprintf(stringBuffer, "Manufacture DeviceID: %s\r\n", manufactureDeviceID);
    strcat(burninTestReportBuffer, stringBuffer);
    strcat(burninTestReportBuffer, "Testing Start Time: ");
    sprintf(stringBuffer, "%04d/%02d/%02d %02d:%02d:%02d\r\n", startRTCTime.u32Year, startRTCTime.u32cMonth, startRTCTime.u32cDay, startRTCTime.u32cHour, startRTCTime.u32cMinute, startRTCTime.u32cSecond);
    strcat(burninTestReportBuffer, stringBuffer);
    strcat(burninTestReportBuffer, "Report Create Time: ");
    sprintf(stringBuffer, "%04d/%02d/%02d %02d:%02d:%02d\r\n", pt->u32Year, pt->u32cMonth, pt->u32cDay, pt->u32cHour, pt->u32cMinute, pt->u32cSecond);
    strcat(burninTestReportBuffer, stringBuffer);
    GetBurninTestRuntime(&hours, &minutes, &seconds);
    strcat(burninTestReportBuffer, "Program Runtime: ");
    sprintf(stringBuffer, "%02dh:%02dm:%02ds\r\n\r\n", hours, minutes, seconds);
    strcat(burninTestReportBuffer, stringBuffer);
    
    if(FirstRun)
    {
        FirstRun = FALSE;
        strcat(burninTestReportBuffer, "------- Version -------\r\n");
        sprintf(stringBuffer, "1.LED: %s\r\n", LEDVerStr);
        strcat(burninTestReportBuffer, stringBuffer);
        sprintf(stringBuffer, "2.READER: %s\r\n", ReaderVerBuf);
        strcat(burninTestReportBuffer, stringBuffer);
        sprintf(stringBuffer, "3.EPD: %s\r\n", preadFWVersion);
        strcat(burninTestReportBuffer, stringBuffer);
        sprintf(stringBuffer, "4.SIM: %s\r\n", SIMtempchr);
        strcat(burninTestReportBuffer, stringBuffer);
        sprintf(stringBuffer, "5.MODEM: %s\r\n", tempchr);
        strcat(burninTestReportBuffer, stringBuffer);
        sprintf(stringBuffer, "6.Radar1: %s\r\n", tempRadar1VersionString);
        strcat(burninTestReportBuffer, stringBuffer);
        sprintf(stringBuffer, "7.Radar2: %s\r\n", tempRadar2VersionString);
        strcat(burninTestReportBuffer, stringBuffer);
        strcat(burninTestReportBuffer, "-----------------------\r\n\r\n");
    }
    //LED & Buzzer
    totalCount = GetLedBurninTestCounter();
    errorCount = GetLedBurninTestErrorCounter();
    sprintf(stringBuffer, "01.LED/Buzzer: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //EPD & EPD Backlight
    totalCount = GetEpdBurninTestCounter();
    errorCount = GetEpdBurninTestErrorCounter();
    sprintf(stringBuffer, "02.EPD/Backlight: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Radar 1
    totalCount = GetRadarBurninTestCounter(VOS_INDEX_0);
    errorCount = GetRadarBurninTestErrorCounter(VOS_INDEX_0);
    sprintf(stringBuffer, "03.Radar 1: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Radar 2
    totalCount = GetRadarBurninTestCounter(VOS_INDEX_1);
    errorCount = GetRadarBurninTestErrorCounter(VOS_INDEX_1);
    sprintf(stringBuffer, "04.Radar 2: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Lidar 1
    totalCount = GetLidarBurninTestCounter(VOS_INDEX_0);
    errorCount = GetLidarBurninTestErrorCounter(VOS_INDEX_0);
    sprintf(stringBuffer, "05.Lidar 1: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Lidar 2
    totalCount = GetLidarBurninTestCounter(VOS_INDEX_1);
    errorCount = GetLidarBurninTestErrorCounter(VOS_INDEX_1);
    sprintf(stringBuffer, "06.Lidar 2: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Battery 1
    totalCount = GetBatteryBurninTestCounter(BATTERY_INDEX_0);
    errorCount = GetBatteryBurninTestErrorCounter(BATTERY_INDEX_0);
    sprintf(stringBuffer, "07.Battery 1: Total=%d, Error=%d, SuccessRate=%.2f, LastADCValue=%d\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount), GetBatteryLastADCValue(BATTERY_INDEX_0));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Battery 2
    totalCount = GetBatteryBurninTestCounter(BATTERY_INDEX_1);
    errorCount = GetBatteryBurninTestErrorCounter(BATTERY_INDEX_1);
    sprintf(stringBuffer, "08.Battery 2: Total=%d, Error=%d, SuccessRate=%.2f, LastADCValue=%d\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount), GetBatteryLastADCValue(BATTERY_INDEX_1));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Solar Battery
    totalCount = GetBatteryBurninTestCounter(BATTERY_INDEX_SOLAR);
    errorCount = GetBatteryBurninTestErrorCounter(BATTERY_INDEX_SOLAR);
    sprintf(stringBuffer, "09.Solar Battery: Total=%d, Error=%d, SuccessRate=%.2f, LastADCValue=%d\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount), GetBatteryLastADCValue(BATTERY_INDEX_SOLAR));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Touch IC (NT066E)
    totalCount = GetNT066EBurninTestCounter();
    errorCount = GetNT066EBurninTestErrorCounter();
    sprintf(stringBuffer, "10.Touch IC(NT066E): Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Modem AT Command
    totalCount = GetModemATBurninTestCounter();
    errorCount = GetModemATBurninTestErrorCounter();
    sprintf(stringBuffer, "11.Modem AT Command: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Modem FTP Command (Upload Test Report)
    totalCount = GetModemFTPBurninTestCounter();
    errorCount = GetModemFTPBurninTestErrorCounter() + GetModemFileBurninTestErrorCounter() + GetModemDialupBurninTestErrorCounter();
    sprintf(stringBuffer, "12.Modem FTP Command: Total=%d, GetFileError=%d, DialupError=%d, FtpError=%d, SuccessRate=%.2f\r\n", totalCount,
            GetModemFileBurninTestErrorCounter(), GetModemDialupBurninTestErrorCounter(), GetModemFTPBurninTestErrorCounter(), calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //SmartCard (SAM) Slot
    totalCount = GetSmartCardBurninTestCounter();
    errorCount = GetSmartCardBurninTestErrorCounter();
    sprintf(stringBuffer, "13.SmartCard (SAM) Slot: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Contactless Card Reader
    totalCount = GetCardReaderBurninTestCounter();
    errorCount = GetCardReaderBurninTestErrorCounter();
    sprintf(stringBuffer, "14.Contactless Card Reader: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Credit Card Reader
    totalCount = GetCreditReaderBurninTestCounter();
    errorCount = GetCreditReaderBurninTestErrorCounter();
    sprintf(stringBuffer, "15.Credit Card Reader: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //SD Card Reader (Save Test Report)
    totalCount = GetSDCardBurninTestCounter();
    errorCount = GetSDCardBurninTestErrorCounter();
    sprintf(stringBuffer, "16.SD Card Reader: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //SPI NAND Flash 1
    totalCount = GetNandFlashBurninTestCounter(FLASH_INDEX_0);
    errorCount = GetNandFlashBurninTestErrorCounter(FLASH_INDEX_0);
    sprintf(stringBuffer, "17.SPI Flash 1: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //SPI NAND Flash 2
    totalCount = GetNandFlashBurninTestCounter(FLASH_INDEX_1);
    errorCount = GetNandFlashBurninTestErrorCounter(FLASH_INDEX_1);
    sprintf(stringBuffer, "18.SPI Flash 2: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //SPI NAND Flash 3
    totalCount = GetNandFlashBurninTestCounter(FLASH_INDEX_2);
    errorCount = GetNandFlashBurninTestErrorCounter(FLASH_INDEX_2);
    sprintf(stringBuffer, "19.SPI Flash 3: Total=%d, Error=%d, SuccessRate=%.2f\r\n", totalCount, errorCount, calcSuccessRate(totalCount, errorCount));
    strcat(burninTestReportBuffer, stringBuffer);
    
    //Camera 1
    //if (GPIO_ReadBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN))
    //{
        totalCount = GetCameraBurninTestCounter(UVCAMERA_INDEX_0);
        errorCount = GetCameraBurninPhotoErrorCounter(UVCAMERA_INDEX_0) + GetCameraBurninFileErrorCounter(UVCAMERA_INDEX_0);
        sprintf(stringBuffer, "20.Camera 1: Total=%d, PhotoError=%d, FileError=%d, SuccessRate=%.2f\r\n", totalCount,
                GetCameraBurninPhotoErrorCounter(UVCAMERA_INDEX_0), GetCameraBurninFileErrorCounter(UVCAMERA_INDEX_0), calcSuccessRate(totalCount, errorCount));
        strcat(burninTestReportBuffer, stringBuffer);
    //}
    
    //Camera 2
    //if (GPIO_ReadBit(DIP_CARD_READER_SELECT_PORT, DIP_CARD_READER_SELECT_PIN))
    //{
        totalCount = GetCameraBurninTestCounter(UVCAMERA_INDEX_1);
        errorCount = GetCameraBurninPhotoErrorCounter(UVCAMERA_INDEX_1) + GetCameraBurninFileErrorCounter(UVCAMERA_INDEX_1);
        sprintf(stringBuffer, "21.Camera 2: Total=%d, PhotoError=%d, FileError=%d, SuccessRate=%.2f\r\n", totalCount,
                GetCameraBurninPhotoErrorCounter(UVCAMERA_INDEX_1), GetCameraBurninFileErrorCounter(UVCAMERA_INDEX_1), calcSuccessRate(totalCount, errorCount));
        strcat(burninTestReportBuffer, stringBuffer);
    //}
    
    //terninalPrintf(burninTestReportBuffer);
    return burninTestReportBuffer;
}

void GetBurninTestRuntime(uint32_t *hours, uint32_t *minutes, uint32_t *seconds)
{
    uint32_t currentRuntime = GetCurrentUTCTime() - startUTCTime;
    *hours = currentRuntime / 3600;
    *minutes = (currentRuntime % 3600) / 60;
    *seconds = currentRuntime % 60;
}

BOOL EnabledBurninTestMode(void)
{
    return isEnabledBurninTestMode;
}

uint32_t GetLedBurninTestCounter(void)
{
    return ledBurninCounter;
}

uint32_t GetLedBurninTestErrorCounter(void)
{
    return ledBurninErrorCounter;
}

uint32_t GetBatteryBurninTestCounter(int index)
{
    return batteryBurninCounter[index];
}

uint32_t GetBatteryBurninTestErrorCounter(int index)
{
    return batteryBurninErrorCounter[index];
}

uint32_t GetNandFlashBurninTestCounter(int index)
{
    return nandFlashBurninCounter[index];
}

uint32_t GetNandFlashBurninTestErrorCounter(int index)
{
    return nandFlashBurninErrorCounter[index];
}

uint32_t GetBatteryLastADCValue(int index)
{
    return batteryLastADCValue[index];
}

uint32_t GetDeviceID(void)
{
    return deviceID;
}

BOOL GetPrepareStopBurninFlag(void)
{
    return prepareStopBurninTest;
}

void ResetRuntimefun(void)
{
    startUTCTime = GetCurrentUTCTime();
}

/*** * Copyright (C) 2020 Far Easy Pass LTD. All rights reserved. ***/

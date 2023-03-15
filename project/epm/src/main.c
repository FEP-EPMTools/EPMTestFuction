/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/05/07 5:38p $
 * @brief  
 *
 * @note
 total = 1024 Blocks = 1024*64 pages
 1 nand     = 1028bits / 8 = 128MB
 1 block    = 128MB / 1024MB = 128K
 1 page     = 128k / 64 = 2K
 uboot-spl  0x00000000 ~ 0x0007ffff     4 blocks (0  ~ 3     block) = 512k = 0.5MB
 env        0x00080000 ~ 0x000fffff     4 blocks (4  ~ 7     block) = 512k = 0.5MB
 uboot      0x00100000 ~ 0x001fffff     8 blocks (8  ~ 15    block) = 1024k = 1MB
 ap1        0x00200000 ~ 0x002fffff     8 blocks (16 ~ 23    block) = 1024k = 1MB
 ap2        0x00300000 ~ 0x003fffff     8 blocks (24 ~ 31    block) = 1024k = 1MB
 ap3        0x00400000 ~ 0x004fffff     8 blocks (32 ~ 39    block) = 1024k = 1MB
 ---        0x00500000 ~ 0x005fffff     8 blocks (40 ~ 47    block) = 1024k = 1MB
 ---        0x00600000 ~ 0x006fffff     8 blocks (48 ~ 55    block) = 1024k = 1MB
 ---        0x00700000 ~ 0x007fffff     8 blocks (56 ~ 63    block) = 1024k = 1MB
 yaffs2     0x00800000 ~ 0x07ffffff     960 blocks (64 ~ 1023    block) = 1024k *  = 120MB

 * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
 ******************************************************************************/
#include "nuc970.h"
#include "sys.h"
#include "rtc.h"
#include "adc.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "osmisc.h"
#include "powerdrv.h"
#include "keydrv.h"
#include "timerdrv.h"
#include "guidrv.h"
#include "user.h"
#include "userdrv.h"
#include "cardreader.h"
#include "epddrv.h"
#include "spacedrv.h"
#include "paralib.h"
#include "dataagent.h"
#include "guimanager.h"
#include "buzzerdrv.h"
#include "pct08cmdlib.h"
#include "pct08drv.h"
#include "fatfslib.h"
#include "cmdlib.h"
#include "meterdata.h"
#include "photoagent.h"
#include "leddrv.h"
#include "modemagent.h"
#include "flashdrvex.h"
#include "yaffs2drv.h"
#include "quentelmodemlib.h"
#include "loglib.h"
#include "dataprocesslib.h"
#include "leddrv.h"
#include "tarifflib.h"
#include "timelib.h"
#include "meterdata.h"
#include "hwtester.h"
#include "batterydrv.h"
#include "fwremoteota.h"

#if (ENABLE_BURNIN_TESTER)
#include "burnintester.h"
#endif


static BOOL sysBooted = FALSE;

BOOL sysRunDebug = TRUE;

typedef BOOL(*initFunction)(BOOL testModeFlag);

typedef struct
{
    char*                   drvName;
    initFunction            func;  
    meterErrorCode          errorId;     
}initFunctionList;

static initFunctionList mInitFunctionList[] = {{"UserDrv", UserDrvInit, METER_ERROR_CODE_USER_DRV},                                                
                                                #if(ENABLE_EPD_DRIVER)
                                                {"EpdDrv", EpdDrvInit, METER_ERROR_CODE_EPD_DRV}, 
                                                #endif
                                                #if(ENABLE_YAFFS2_DRIVER)
                                                {"Yaffs2DrvInit", Yaffs2DrvInit, METER_ERROR_CODE_YAFFS2_DRV},
                                                #endif
                                                #if(ENABLE_FATFS_DRIVER)
                                                {"FatfsInit", FatfsInit, METER_ERROR_CODE_FATFS_DRV},
                                                #endif
                                                #if(ENABLE_GUI_DRIVER)
                                                {"GUIDrv", GUIDrvInit, METER_ERROR_CODE_GUI_DRV}, 
                                                #endif
                                                #if(ENABLE_CARD_READER_DRIVER)
                                                {"CardReader", CardReaderInit, METER_ERROR_CODE_CARD_READER_DRV},
                                                #endif
                                                #if(ENABLE_SPACE_DRIVER)
                                                {"SpaceDrv", SpaceDrvInit, METER_ERROR_CODE_SPSCE_DRV},
                                                #endif
                                                #if(ENABLE_POWER_DRIVER)
                                                {"PowerDrv", PowerDrvInit, METER_ERROR_CODE_POWER_DRV}, 
                                                #endif
                                                #if(ENABLE_DATA_AGENT_DRIVER)
                                                {"DataAgentDrv", DataAgentInit, METER_ERROR_CODE_DATA_AGENT_DRV}, 
                                                #endif
                                                #if(ENABLE_MODEM_AGENT_DRIVER)
                                                {"ModemAgentDrv", ModemAgentInit, METER_ERROR_CODE_MODEM_AGENT_DRV}, 
                                                #endif
                                                #if(ENABLE_TAKE_PHOTO_DRIVER)
                                                {"PCT08DrvInit", PCT08DrvInit, METER_ERROR_CODE_PCT08_DRV}, 
                                                #endif
                                                #if(ENABLE_PHOTO_AGENT_DRIVER)
                                                {"PhotoAgentInit", PhotoAgentInit, METER_ERROR_CODE_PHOTO_AGENT}, 
                                                #endif
                                                #if(ENABLE_USER_DRIVER)
                                                {"User", UserInit, METER_ERROR_CODE_USER}, //必須是最後一個
                                                #endif
                                                {"", NULL}};
/*-----------------------------------------------------------------------------*/
                                                

static void printSysInfo(void)
{
    sysprintf("\n\n**** Hello EPM Project %s %s [Ver. %d.%02d.%02d  build:%u]!!! ****\n", __DATE__, __TIME__, MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, BUILD_VERSION);
    setdataCode(BUILD_VERSION);
    sysprintf("APLL    clock %d MHz\n", sysGetClock(SYS_APLL));
    sysprintf("UPLL    clock %d MHz\n", sysGetClock(SYS_UPLL));
    sysprintf("CPU     clock %d MHz\n", sysGetClock(SYS_CPU));
    sysprintf("System  clock %d MHz\n", sysGetClock(SYS_SYSTEM));
    sysprintf("HCLK1   clock %d MHz\n", sysGetClock(SYS_HCLK1));
    sysprintf("HCLK234 clock %d MHz\n", sysGetClock(SYS_HCLK234));
    sysprintf("PCLK    clock %d MHz\n", sysGetClock(SYS_PCLK));
    sysprintf("REG_SYS_RSTSTS 0x%08x\n", inpw(REG_SYS_RSTSTS));
    
}
void PrintRTC(void);
static void debugProcess(void)
{
    UINT8 charTmp;
    //static uint8_t meterPosition = 0xff; 
    //static uint8_t carEnableNumber = 0xff;; 
    if(sysIsKbHit())
    {        
        charTmp = sysGetChar();
        sysprintf("\r\n keyin[!0x%02x: %c!]\r\n", charTmp, charTmp); 
        switch(charTmp) 
        {
            
            case 'd':
            case 'D':
                printSysInfo();
                {
                UINT32 leftVoltage,  rightVoltage;
                BatteryGetValue(NULL,&leftVoltage, &rightVoltage);
                sysprintf(" Voltage: [%d], [%d]\r\n", leftVoltage, rightVoltage);
                }
                sysprintf("[ QModemDialupStageIndex = %d,  QModemFtpStageIndex = %d] \r\n", QModemDialupStageIndex(), QModemFtpStageIndex()); 
                sysprintf("[ Tariff:[%s] (type = %d), Para:[%s], FormatCounter = %08d, FlashError = %08d ]\r\n", 
                                            TariffGetFileName(), TariffGetCurrentTariffType()->type, GetMeterPara()->name, FileAgentGetFatFsAutoFormatCounter(), FlashDrvExGetErrorTimes());               
                printParaValue("Dump");                
                __SHOW_FREE_HEAP_SIZE__
                _showMemoryInfo();  
                PrintRTC();
                
                break;
            case '1':   
                FileAgentFatfsListFile("0:", "*.*");
                FileAgentFatfsListFile("1:", "*.*");
                FileAgentFatfsListFile("2:", "*.*");
                break;
            case '2':             
                MeterUpdateLedHeartbeat();
                break;
            case '3':
                FlashDrvExChipErasePure(SPI_FLASH_EX_0_INDEX, SPI_FLASH_EX_RAW_START_SECTOR, SPI_FLASH_EX_RAW_END_SECTOR);
                FlashDrvExChipErasePure(SPI_FLASH_EX_1_INDEX, SPI_FLASH_EX_RAW_START_SECTOR, SPI_FLASH_EX_RAW_END_SECTOR);
                break;
            case '4':
                FileAgentFatfsDeleteFile("1:", FILE_EXTENSION_EX(DCF_FILE_EXTENSION));
                FileAgentFatfsDeleteFile("1:", FILE_EXTENSION_EX(DSF_FILE_EXTENSION));
                FileAgentFatfsDeleteFile("1:", FILE_EXTENSION_EX(PHOTO_FILE_EXTENSION));
                FileAgentFatfsDeleteFile("1:", FILE_EXTENSION_EX(LOG_FILE_EXTENSION));
                FileAgentFatfsDeleteFile("2:", FILE_EXTENSION_EX(DCF_FILE_EXTENSION));
                FileAgentFatfsDeleteFile("2:", FILE_EXTENSION_EX(DSF_FILE_EXTENSION));
                FileAgentFatfsDeleteFile("2:", FILE_EXTENSION_EX(PHOTO_FILE_EXTENSION));
                FileAgentFatfsDeleteFile("2:", FILE_EXTENSION_EX(LOG_FILE_EXTENSION));
                break;
            case '5':
            {
                LedSetStatus();
            }
		
                break;
            case 'e':
                FlashDrvExChipEraseFs(SPI_FLASH_EX_0_INDEX);
                FlashDrvExChipEraseFs(SPI_FLASH_EX_1_INDEX);
                break;
            
            case 'o':
            case 'O':
                ATCmdSetReceiveDebugFlag(TRUE);
                break;
            
            case 'p':
            case 'P':
                ATCmdSetReceiveDebugFlag(FALSE);
                break;
            
            case ',':
                FileAgentFatFsFormat("1:");
                break;
            case '.':
                FileAgentFatFsFormat("2:");
                break;
            
            case 't':
            case 'T':
                PhotoAgentStartTakePhoto(0);
                break;
            
            case '9':
                PowerDrvSetEnable(TRUE);
                break;
            case '0':
                PowerDrvSetEnable(FALSE);
                break;
            
                      
            default:
                break;
        }            
    }
}
static void deviceHwInit()
{//hardware
    RTC_EnableClock(TRUE);
    /* RTC Initialize */
    RTC_Init();
} 

static BOOL driverInit()
{
    BOOL reVal = TRUE;
    int i;
    sysprintf(" **> driverInit start...\r\n");
    
    for(i = 0; ; i++)
    {
        if(mInitFunctionList[i].func != NULL)
        {
            if(mInitFunctionList[i].func(FALSE))
            {
                sysprintf(" * [%02d]: %s OK... *\r\n", i, mInitFunctionList[i].drvName);
            }
            else
            {
                sysprintf(" * [%02d]: %s ERROR... *\r\n", i, mInitFunctionList[i].drvName);
                MeterSetErrorCode(mInitFunctionList[i].errorId);
                reVal = FALSE;
                break;
            }
        }
        else
        {
            
            break;
        }
    }    
    sysprintf(" **> driverInit end...\r\n");
    return reVal;
}
#define ERROR_CODE_CHECK   1
static void vDrvInitTask( void *pvParameters )
{
    sysprintf("vDrvInitTask Going...\r\n"); 

    MeterSetBuildVer(BUILD_VERSION); 
    
    driverInit();
    if(GetMeterData()->meterErrorCode == 0)
    {
        LoglibPrintf(LOG_TYPE_INFO, "EPM Reboot OK...\r\n");
        DataProcessSendStatusData(0, "Reboot_OK");
    
    }
    else
    {
        char str[256];
        sprintf(str, "EPM Reboot ERROR(code = 0x%08x)...\r\n", GetMeterData()->meterErrorCode);
        LoglibPrintf(LOG_TYPE_INFO, "EPM Reboot ERROR...\r\n");
        DataProcessSendStatusData(0, "Reboot_ERROR");
    }
    ModemAgentStartSend(DATA_PROCESS_ID_ESF); 
      
    while(1)
    {
        //GetMeterData()->meterErrorCode =0x1248;
        if(GetMeterData()->meterErrorCode == 0)
        {
            //sysprintf("vDrvInitTask ending...\r\n");
            sysBooted = TRUE;            
            #if(!ERROR_CODE_CHECK)
            vTaskDelete(NULL);  
            #endif
        }
        else
        {
            sysprintf("\r\n===== !!!!  vDrvInitTask fail [0x%08x]...===== !!!!  \r\n", GetMeterData()->meterErrorCode);
            LedSetColor(NULL, LIGHT_COLOR_YELLOW, TRUE);
            EPDShowBGScreen(EPD_PICT_INDEX_INIT_FAIL, TRUE);
            EPDShowErrorID(GetMeterData()->meterErrorCode);
            
            for(;;)
            {        
                BuzzerPlay(200, 200, 1, TRUE);
                vTaskDelay(500/portTICK_RATE_MS); 
                BuzzerPlay(50, 200, (GetMeterData()->meterErrorCode>>12&0xf) + 1, TRUE);
                vTaskDelay(500/portTICK_RATE_MS);
                BuzzerPlay(50, 200, (GetMeterData()->meterErrorCode>>8&0xf) + 1, TRUE);
                vTaskDelay(500/portTICK_RATE_MS);
                BuzzerPlay(50, 200, (GetMeterData()->meterErrorCode>>4&0xf) + 1, TRUE);
                vTaskDelay(500/portTICK_RATE_MS);
                BuzzerPlay(50, 200, (GetMeterData()->meterErrorCode>>0&0xf) + 1, TRUE);
                vTaskDelay(2000/portTICK_RATE_MS); 
            }
        }
        vTaskDelay(2000/portTICK_RATE_MS); 
        sysprintf("!");              
    }
          
}
#if(BUILD_HW_TESTER_VERSION)
static void vHWTestTask( void *pvParameters )
{
    sysprintf("vHWTestTask Going...\r\n"); 
    
    // open 12V power    
    outpw(REG_SYS_GPG_MFPL,(inpw(REG_SYS_GPG_MFPL) & ~(0xF<<24)) | (0x0<<24));
    GPIO_OpenBit(GPIOG, BIT6, DIR_OUTPUT, PULL_UP);
    GPIO_SetBit(GPIOG, BIT6);
    
    if(!GPIO_ReadBit(GPIOI, BIT3))
        FWremoteOTAFunc();
    
#if (ENABLE_BURNIN_TESTER)
    
    if(!GPIO_ReadBit(GPIOJ, BIT2))
    {
        MTPTesterInit();
    }
    //else if (GPIO_ReadBit(DIP_BURNIN_TEST_PORT, DIP_BURNIN_TEST_PIN) && (GPIO_ReadBit(GPIOI, BIT3)))
    else if (GPIO_ReadBit(DIP_BURNIN_TEST_PORT, DIP_BURNIN_TEST_PIN))
    {
        HWTesterInit();
    }
    else
    {
        CalibRTCviaNTP();
        BurninTesterInit();
        BurninTestingMonitor();
    }
#else
    HWTesterInit();
#endif
    sysprintf("vHWTestTask End...\r\n");
    vTaskDelete(NULL); 
}
#endif
static void vDebugTask( void *pvParameters )
{
    sysprintf("vDebugTask Going...\r\n"); 
    for(;;)
    {        
        debugProcess();
        vTaskDelay(100/portTICK_RATE_MS); 
    }

}

BOOL SysGetBooted(void)
{
    return sysBooted;
}
int main(void)
{    
    *((volatile unsigned int *)REG_AIC_MDCR)=0xFFFFFFFF;  // disable all interrupt channel
    *((volatile unsigned int *)REG_AIC_MDCRH)=0xFFFFFFFF;  // disable all interrupt channel
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    sysInitializeUART();
    
    //for power in booter
    outpw(REG_SYS_GPC_MFPL, 0x00000000);//nand
    outpw(REG_SYS_GPC_MFPH, 0x00000000);//nand
    outpw(REG_SYS_GPJ_MFPL, 0x00000000);//jtag
    outpw(REG_CLK_HCLKEN, 0x00000527);
    //outpw(REG_CLK_HCLKEN, 0x00000401);    
    adcOpen();//須優先執行 原因不明   
    printSysInfo();    
    
    deviceHwInit();
#if(BUILD_HW_TESTER_VERSION)
    {
        outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
        UINT32 portValue = GPIO_ReadPort(GPIOH);
        sysprintf("GPIOH (0x%02x, %d) ...\r\n", (portValue>>2)&0x3, (portValue>>2)&0x3); 
        if(((portValue>>2)&0x3) == 2)
        {
            xTaskCreate( vHWTestTask, "vHWTestTask", 1024*20, NULL, DEBUG_THREAD_PROI, NULL );
        }
        else
        {
            xTaskCreate( vDrvInitTask, "vDrvInitTask", 1024*20, NULL, DRV_INIT_THREAD_PROI, NULL );
            xTaskCreate( vDebugTask, "vDebugTask", 1024*20, NULL, DEBUG_THREAD_PROI, NULL );
        }
    }
#else    
    xTaskCreate( vDrvInitTask, "vDrvInitTask", 1024*20, NULL, DRV_INIT_THREAD_PROI, NULL );
    xTaskCreate( vDebugTask, "vDebugTask", 1024*20, NULL, DEBUG_THREAD_PROI, NULL );
#endif
    

    /* Start the scheduler. */
    vTaskStartScheduler();
    /* Infinite loop */
    sysprintf("XXXX!!!\n");
    while (1)
    {
    }
}


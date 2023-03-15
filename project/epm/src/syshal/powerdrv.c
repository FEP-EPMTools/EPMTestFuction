/**************************************************************************//**
* @file     powerdrv.c
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
#include "uart.h"
#include "gpio.h"
#include "rtc.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "powerdrv.h"
#include "dataprocesslib.h"
#include "modemagent.h"
#include "tarifflib.h"
#include "batterydrv.h"
#include "epddrv.h"
#include "guimanager.h"
#include "buzzerdrv.h"
#include "leddrv.h"
#include "nt066edrv.h"
#include "quentelmodemlib.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
BOOL volatile g_bAlarm = FALSE;

static uint8_t wakeupSource = WAKEUP_SOURCE_NONE;

static UINT32 wakeUpTime = 5;

#if(0)
#define USE_GPF11   0x01
#define USE_GPH0    0x02
#define USE_GPH4    0x03
#define USE_GPH3    0x04
//#define EINT_DEFINE   USE_GPF11 
//#define EINT_DEFINE   USE_GPH0 
//#define EINT_DEFINE         USE_GPH4 
#define EINT_DEFINE         USE_GPH3 

#if(EINT_DEFINE == USE_GPF11)
    #define EINT_PORT  GPIOF
    #define EINT_BIT   BIT11
    #define EINT_BIT_DEFINE   0 
#elif(EINT_DEFINE == USE_GPH0)
    #define EINT_PORT  GPIOH
    #define EINT_BIT   BIT0
    #define EINT_BIT_DEFINE   0 
#elif(EINT_DEFINE == USE_GPH4)
    #define EINT_PORT  GPIOH
    #define EINT_BIT   BIT4
    #define EINT_BIT_DEFINE   4 
#elif(EINT_DEFINE == USE_GPH3)
    #define EINT_PORT  GPIOH
    #define EINT_BIT   BIT3
    #define EINT_BIT_DEFINE   3 
#else
    #error
#endif
#endif

#define POWER_DRV_POLLING_TIME  1500

static BOOL PowerDrvCheckStatus(int flag);
static BOOL PowerDrvPreOffCallback(int flag);
static BOOL PowerDrvOffCallback(int flag);
static BOOL PowerDrvOnCallback(int flag);
static powerCallbackFunc powerDrvPowerCallabck = {" [PowerDrv] ", PowerDrvPreOffCallback, PowerDrvOffCallback, PowerDrvOnCallback, PowerDrvCheckStatus};

static powerCallbackFunc* mPowerCallbackFunc[MAX_POWER_REG_CALLBACK_NUM];
static int regCallbackIndex = 0;

static TickType_t mWakeupTick = 0;
static TickType_t mTotalWakeupTick = 0;

static TickType_t threadWaitTime = (POWER_DRV_POLLING_TIME/portTICK_RATE_MS);
static SemaphoreHandle_t xSemaphore;

static volatile uint32_t REG_CLK_HCLKENTmp;
static volatile uint32_t REG_CLK_PCLKEN0Tmp;
static volatile uint32_t REG_CLK_PCLKEN1Tmp;
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
extern __asm void __wfi(void); 
extern BOOL SysGetBooted(void);
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static void busyDelay(void)
{
    
}

#if(1)
static void showReg(char* str)
{
    /*
    *** Suspend ***
    REG_CLK_PMCON        : 0x0000FF03
    REG_CLK_HCLKEN       : 0x00000527
    REG_CLK_PCLKEN0      : 0x0001010C
    REG_CLK_PCLKEN1      : 0x00000000
    REG_CLK_APLLCON      : 0x50000015
    REG_CLK_UPLLCON      : 0xC0000018
    REG_CLK_PLLSTBCNTR   : 0x00001800
    
    

    */
    //outpw(REG_SYS_GPC_MFPL, 0x00000000);
    //outpw(REG_SYS_GPC_MFPH, 0x00000000);
    //outpw(REG_SYS_GPJ_MFPL, 0x00000000);
    //outpw(REG_CLK_PMCON, 0x0000FF03);
    sysprintf("*** showReg [%s] start ***\n", str);
    sysprintf("REG_CLK_PMCON        : 0x%08x\n", inpw(REG_CLK_PMCON));
    sysprintf("REG_CLK_HCLKEN       : 0x%08x\n", inpw(REG_CLK_HCLKEN));
    sysprintf("REG_CLK_PCLKEN0      : 0x%08x\n", inpw(REG_CLK_PCLKEN0));
    sysprintf("REG_CLK_PCLKEN1      : 0x%08x\n", inpw(REG_CLK_PCLKEN1));
    sysprintf("REG_CLK_APLLCON      : 0x%08x\n", inpw(REG_CLK_APLLCON));
    sysprintf("REG_CLK_UPLLCON      : 0x%08x\n", inpw(REG_CLK_UPLLCON));
    sysprintf("REG_CLK_PLLSTBCNTR   : 0x%08x\n\n", inpw(REG_CLK_PLLSTBCNTR)); 
    
    sysprintf("REG_SYS_PWRON        : 0x%08x\n\n", inpw(REG_SYS_PWRON)); //0x020007F0
    
    
    sysprintf("REG_SYS_GPA_MFPL     : 0x%08x\n", inpw(REG_SYS_GPA_MFPL));
    sysprintf("REG_SYS_GPA_MFPH     : 0x%08x\n", inpw(REG_SYS_GPA_MFPH));
    sysprintf("REG_GPIOA_DIR        : 0x%08x\n\n", inpw(REG_GPIOA_DIR));
    
    
    sysprintf("REG_SYS_GPB_MFPL     : 0x%08x\n", inpw(REG_SYS_GPB_MFPL));
    sysprintf("REG_SYS_GPB_MFPH     : 0x%08x\n", inpw(REG_SYS_GPB_MFPH));
    sysprintf("REG_GPIOB_DIR        : 0x%08x\n\n", inpw(REG_GPIOB_DIR));
    
    sysprintf("REG_SYS_GPC_MFPL     : 0x%08x\n", inpw(REG_SYS_GPC_MFPL));
    sysprintf("REG_SYS_GPC_MFPH     : 0x%08x\n", inpw(REG_SYS_GPC_MFPH));
    sysprintf("REG_GPIOC_DIR        : 0x%08x\n\n", inpw(REG_GPIOC_DIR));
    
    sysprintf("REG_SYS_GPD_MFPL     : 0x%08x\n", inpw(REG_SYS_GPD_MFPL));
    sysprintf("REG_SYS_GPD_MFPH     : 0x%08x\n", inpw(REG_SYS_GPD_MFPH));
    sysprintf("REG_GPIOD_DIR        : 0x%08x\n\n", inpw(REG_GPIOD_DIR));
    
    sysprintf("REG_SYS_GPE_MFPL     : 0x%08x\n", inpw(REG_SYS_GPE_MFPL));
    sysprintf("REG_SYS_GPE_MFPH     : 0x%08x\n", inpw(REG_SYS_GPE_MFPH));
    sysprintf("REG_GPIOE_DIR        : 0x%08x\n\n", inpw(REG_GPIOE_DIR));
    
    sysprintf("REG_SYS_GPF_MFPL     : 0x%08x\n", inpw(REG_SYS_GPF_MFPL));
    sysprintf("REG_SYS_GPF_MFPH     : 0x%08x\n", inpw(REG_SYS_GPF_MFPH));
    sysprintf("REG_GPIOF_DIR        : 0x%08x\n\n", inpw(REG_GPIOF_DIR));
    
    sysprintf("REG_SYS_GPG_MFPL     : 0x%08x\n", inpw(REG_SYS_GPG_MFPL));
    sysprintf("REG_SYS_GPG_MFPH     : 0x%08x\n", inpw(REG_SYS_GPG_MFPH));
    sysprintf("REG_GPIOG_DIR        : 0x%08x\n\n", inpw(REG_GPIOG_DIR));
    
    sysprintf("REG_SYS_GPH_MFPL     : 0x%08x\n", inpw(REG_SYS_GPH_MFPL));
    sysprintf("REG_SYS_GPH_MFPH     : 0x%08x\n", inpw(REG_SYS_GPH_MFPH));
    sysprintf("REG_GPIOH_DIR        : 0x%08x\n\n", inpw(REG_GPIOH_DIR));
    
    sysprintf("REG_SYS_GPI_MFPL     : 0x%08x\n", inpw(REG_SYS_GPI_MFPL));
    sysprintf("REG_SYS_GPI_MFPH     : 0x%08x\n", inpw(REG_SYS_GPI_MFPH));
    sysprintf("REG_GPIOI_DIR        : 0x%08x\n\n", inpw(REG_GPIOI_DIR));

    sysprintf("REG_SYS_GPJ_MFPL     : 0x%08x\n", inpw(REG_SYS_GPJ_MFPL));
    sysprintf("REG_GPIOJ_DIR        : 0x%08x\n\n", inpw(REG_GPIOJ_DIR));
    
    sysprintf("*** showReg [%s] end ***\n", str);
}
#endif

static void backupReg(void)
{
    //showReg("backupReg enter");
    //REG_CLK_HCLKENTmp = inpw(REG_CLK_HCLKEN);
    //outpw(REG_CLK_HCLKEN, 0x00000501);
    
    //REG_CLK_PCLKEN0Tmp = inpw(REG_CLK_PCLKEN0);
    //outpw(REG_CLK_PCLKEN0, 0x0000000c);
    
    //REG_CLK_PCLKEN1Tmp = inpw(REG_CLK_PCLKEN1);
    //outpw(REG_CLK_PCLKEN1, 0x00000000);
    //showReg("backupReg exit");
}
static void restoreReg(void)
{
    //showReg("restoreReg enter");
    //outpw(REG_CLK_HCLKEN , REG_CLK_HCLKENTmp);
   // outpw(REG_CLK_PCLKEN0 , REG_CLK_PCLKEN0Tmp);
    //outpw(REG_CLK_PCLKEN1 , REG_CLK_PCLKEN1Tmp);
    //showReg("restoreReg exit");
}


VOID RTC_Releative_AlarmISR(void)
{
    //sysprintf("   Relative Alarm!!\n");
    g_bAlarm = TRUE;
}
static BOOL PowerDrvPreOffCallback(int flag)
{
    BOOL reVal = TRUE;
    //sysprintf("### Power OFF Callback [%s] ###\r\n", powerDrvPowerCallabck.drvName);    
    return reVal;    
}
static BOOL PowerDrvOffCallback(int flag)
{
    BOOL reVal = TRUE;
    //sysprintf("### Power OFF Callback [%s] ###\r\n", powerDrvPowerCallabck.drvName);    
    return reVal;    
}
static BOOL PowerDrvOnCallback(int flag)
{
    BOOL reVal = TRUE;
    //sysprintf("### Power ON Callback [%s] ###\r\n", powerDrvPowerCallabck.drvName);    
    return reVal;    
}
static BOOL PowerDrvCheckStatus(int flag)
{
    #if(1)
    BOOL reVal = FALSE;
    //sysprintf("### Power STATUS Callback [%s] ###\r\n", powerDrvPowerCallabck.drvName); 
    //sysprintf(" PowerCheckStatus:  GPIO_ReadBit(EINT_PORT, EINT_BIT) = %d\n", GPIO_ReadBit(EINT_PORT, EINT_BIT));
    #if(1)
    if(GPIO_ReadBit(GPIOJ, BIT4) == 1)
    {        
        sysDelay(10/portTICK_RATE_MS);//100ms
        if(GPIO_ReadBit(GPIOJ, BIT4) == 1)
        {        
            //sysDelay(100);//1000ms
            reVal = TRUE;
        }        
    }
    #endif
    return reVal; 
    #else
    BOOL reVal = FALSE;
    //sysprintf("### Power STATUS Callback [%s] ###\r\n", powerDrvPowerCallabck.drvName); 
    //sysprintf(" PowerCheckStatus:  GPIO_ReadBit(EINT_PORT, EINT_BIT) = %d\n", GPIO_ReadBit(EINT_PORT, EINT_BIT));
    if(GPIO_ReadBit(EINT_PORT, EINT_BIT) == 1)
    {        
        sysDelay(10/portTICK_RATE_MS);//100ms
        if(GPIO_ReadBit(EINT_PORT, EINT_BIT) == 1)
        {        
            //sysDelay(100);//1000ms
            reVal = TRUE;
        }        
    }
    return reVal; 
    #endif    
}
static BOOL processPreOffCallback(void)
{
    int i;
    BOOL suspendFlag = TRUE;
    #if(BUILD_DEBUG_VERSION)
    sysprintf(" >~> Pre OFF >> Start...\r\n"); 
    #endif
    for(i=0; i<regCallbackIndex; i++)
    {  
        if(mPowerCallbackFunc[i]->powerPreOffCallback(BatteryCheckPowerDownCondition()))
        {
            //sysprintf(" >~> Pre OFF [%d] >> %s is OK...\r\n", i, mPowerCallbackFunc[i]->drvName); 
        }
        else
        { 
            suspendFlag = FALSE;
            #if(BUILD_DEBUG_VERSION)
            sysprintf(" !!! Pre OFF [%d] >> %s is ERROR...\r\n", i, mPowerCallbackFunc[i]->drvName);   
            #endif            
        }
    }
    #if(BUILD_DEBUG_VERSION)
    if(suspendFlag)
        sysprintf(" <~< Pre OFF << OK...\r\n"); 
    else
        sysprintf(" <~< Pre OFF << ERROR...\r\n");
    #endif
    return suspendFlag;
}
static BOOL processOffCallback(void)
{
    int i;
    BOOL suspendFlag = TRUE;
    #if(BUILD_DEBUG_VERSION)
    sysprintf(" >!> OFF >> Start...\r\n"); 
    #endif
    for(i=0; i<regCallbackIndex; i++)
    {  
        if(mPowerCallbackFunc[i]->powerOffCallback(BatteryCheckPowerDownCondition()))
        {
            //sysprintf(" >!> OFF [%d] >> %s is OK...\r\n", i, mPowerCallbackFunc[i]->drvName); 
        }
        else
        { 
            suspendFlag = FALSE;
            #if(BUILD_DEBUG_VERSION)
            sysprintf(" !!! OFF [%d] >> %s is ERROR...\r\n", i, mPowerCallbackFunc[i]->drvName);   
            #endif            
        }
    }
    #if(BUILD_DEBUG_VERSION)
    if(suspendFlag)
        sysprintf(" <!< OFF << OK...\r\n"); 
    else
        sysprintf(" <!< OFF << ERROR...\r\n");
    #endif
    return suspendFlag;
}
static BOOL processOnCallback(void)
{
    int i;
    BOOL suspendFlag = TRUE;
    //for(i=0; i<regCallbackIndex; i++)
    #if(BUILD_DEBUG_VERSION)
    sysprintf(" >^> ON >> Start...\r\n"); 
    #endif
    
    for(i=regCallbackIndex-1; i>=0; i--)
    {  
        if(mPowerCallbackFunc[i]->powerOnCallback(wakeupSource))
        {
            //sysprintf(" >^> ON [%d] >> %s is OK...\r\n", i, mPowerCallbackFunc[i]->drvName); 
        }
        else
        { 
            suspendFlag = FALSE;
            #if(BUILD_DEBUG_VERSION)
            sysprintf(" !!! ON [%d] >> %s is ERROR...\r\n", i, mPowerCallbackFunc[i]->drvName); 
            #endif            
        }
    }
    #if(BUILD_DEBUG_VERSION)
    if(suspendFlag)
        sysprintf(" <^< ON << OK...\r\n"); 
    else
        sysprintf(" <^< ON << ERROR...\r\n");
    #endif
    return suspendFlag;
}
static BOOL processStatusCallback(void)
{
    int i;
    BOOL suspendFlag = TRUE;
    if(regCallbackIndex > 0)
        sysprintf("\r\n <*< STATUS Busy: ");  
    for(i=0; i<regCallbackIndex; i++)
    {  
        if(mPowerCallbackFunc[i]->powerStatusCallback(BatteryCheckPowerDownCondition()))
        {
            //sysprintf("    >> STATUS : [%d]>> %s is OK...\r\n", i, mPowerCallbackFunc[i]->drvName); 
        }
        else
        { 
            suspendFlag = FALSE;
            //#if(BUILD_DEBUG_VERSION)
            //sysprintf(" >*> STATUS [%d]>> %s is Busy...\r\n", i, mPowerCallbackFunc[i]->drvName);  
            sysprintf("%s ", mPowerCallbackFunc[i]->drvName); 
            //#endif  
            //break;
        }
    }
    if(regCallbackIndex > 0)
        sysprintf(" >*>\r\n");  
    return suspendFlag;
}

static void powerSendStatusData(void)
{   
    sysprintf(">> powerSendStatusData: wakeupSource = %d Enter....\n", wakeupSource);
    switch(wakeupSource)
    {
        case WAKEUP_SOURCE_RTC:
            DataProcessSendStatusData(0, "RTC_Wakeup");     
            break;
        case WAKEUP_SOURCE_USER:
            DataProcessSendStatusData(0, "User_Wakeup");     
            break;
        case WAKEUP_SOURCE_DIP:
            DataProcessSendStatusData(0, "DIP_Wakeup");     
            break;
        case WAKEUP_SOURCE_OTHER:
            DataProcessSendStatusData(0, "Other_Wakeup");     
            break;        
        default:
            DataProcessSendStatusData(0, "Unknown_Wakeup");     
            break;
    }   
    sysprintf(">> powerSendStatusData: wakeupSource = %d Exit....\n", wakeupSource);
}
static void vPowerDrvTask( void *pvParameters )
{
    BOOL suspendFlag = TRUE;
    sysprintf("\r\n !!! vPowerDrvTask Waiting... !!!\r\n"); 
    while(SysGetBooted() == FALSE)
    {
        vTaskDelay(500/portTICK_RATE_MS);
    }
    vTaskDelay(1000/portTICK_RATE_MS);
    sysprintf("\r\n !!! vPowerDrvTask Going... !!!\r\n");  
    for(;;)
    {       
        xSemaphoreTake(xSemaphore, threadWaitTime);               
        suspendFlag = processStatusCallback(); 
        if(BatteryCheckPowerDownCondition() || (suspendFlag))
        {
            processPreOffCallback();
            suspendFlag = processOffCallback();
            if(BatteryCheckPowerDownCondition())
            {
                NT066ESetPower(FALSE);
                LedSetPower(FALSE);
                EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE); 
                BuzzerPlay(100, 100, 2, TRUE); 
								//QModemTotalStop();							
                PowerSuspend((1 << 5)| (1 << 6));
                PowerClearISR(); 
							
								//BatterySetSwitch1(TRUE);
								//BatterySetSwitch2(TRUE);
                //temp
                //sysDelay(100/portTICK_RATE_MS);//
                //GuiManagerRefreshScreen();
                //BuzzerPlay(100, 100, 2, TRUE);
                PowerDrvResetSystem();
            }
            else if(suspendFlag)
            {
                PowerSuspend(0xffffffff);
                PowerClearISR(); 
                //temp
                sysDelay(100/portTICK_RATE_MS);//
                //powerSendStatusData();
            }
            
            else
            {
                sysprintf("\r\n   !!! ERROR processOffCallback ERROR, ignore PowerSuspend... !!!\r\n");    
            }
            processOnCallback();    
            //vTaskDelay(1000/portTICK_RATE_MS);  
            //DataProcessSendStatusData(0, "wakeup");   
            //ModemAgentStartSend(DATA_PROCESS_ID_ESF);            
        }

    }
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL PowerDrvInit(BOOL testModeFlag)
{
    sysprintf("PowerDrvInit!!\n");
    
    //move to main.c
    /*
    RTC_EnableClock(TRUE);
    // RTC Initialize
    RTC_Init();
    */
    
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
#if(0)
#if(EINT_DEFINE == USE_GPF11)
    /* Set MFP_GPF11 to EINT0 */
    outpw(REG_SYS_GPF_MFPH,(inpw(REG_SYS_GPF_MFPH) & ~(0xF<<12)) | (0xF<<12));
#elif(EINT_DEFINE == USE_GPH0)
    /* Set MFP_GPH00 to EINT0 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<0)) | (0xF<<0));
#elif(EINT_DEFINE == USE_GPH4)
    /* Set MFP_GPH04 to EINT4 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<16)) | (0xF<<16));
#elif(EINT_DEFINE == USE_GPH3)
    /* Set MFP_GPH03 to EINT3 */
    outpw(REG_SYS_GPH_MFPL,(inpw(REG_SYS_GPH_MFPL) & ~(0xF<<12)) | (0xF<<12));
#endif 
#endif
    
    RTC_Ioctl(0, RTC_IOC_ENABLE_INT, RTC_RELATIVE_ALARM_INT, 0);
#if(0)    
    GPIO_OpenBit(EINT_PORT, EINT_BIT, DIR_INPUT, PULL_UP);
    GPIO_DisableTriggerType(EINT_PORT, EINT_BIT);
#else
    //GPJ 4
    outpw(REG_SYS_GPJ_MFPL,(inpw(REG_SYS_GPJ_MFPL) & ~(0xF<<16)) | (0x0<<16));
    GPIO_OpenBit(GPIOJ, BIT4, DIR_INPUT, PULL_UP);
#endif
    PowerRegCallback(&powerDrvPowerCallabck);
    
    xSemaphore = xSemaphoreCreateBinary();
   
    xTaskCreate( vPowerDrvTask, "PowerDrvTask", 1024*10, NULL, POWER_THREAD_PROI, NULL );
    return TRUE;
}

int PowerRegCallback(powerCallbackFunc* callbackFunc)
{
    if(regCallbackIndex<MAX_POWER_REG_CALLBACK_NUM)
    {
        int reval = regCallbackIndex;
        regCallbackIndex++;
        mPowerCallbackFunc[reval] = callbackFunc;
        sysprintf(" --###- PowerRegCallback[%s] OK: index = %d ---\n", mPowerCallbackFunc[reval]->drvName, reval);
        return reval;
    }
    else
    {
        sysprintf(" --###- PowerRegCallback[%s] ERROR: regCallbackIndex = %d ---\n", callbackFunc->drvName, regCallbackIndex);
        return -1;
    }
}

BOOL PowerSuspend(UINT32 wakeUpSource)
{
    UINT32 reg;
    RTC_TIME_DATA_T sCurTime;
    /* Get the currnet time */
    //if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &sCurTime))
    if(TariffGetNextWakeupTime(&wakeUpTime, &sCurTime))
    {
        mTotalWakeupTick = xTaskGetTickCount() - mWakeupTick;
        sysprintf("\r\n *** Suspend [Total Wakeup Tick: %d] ***\n", mTotalWakeupTick);
        //showReg("PowerSuspend");   

        #if(0)
        /* Confingure PF11 to rising-edge trigger */
        GPIO_EnableTriggerType(EINT_PORT, EINT_BIT, FALLING);
        #endif
        
        //#if(BUILD_RELEASE_VERSION || BUILD_PRE_RELEASE_VERSION)
        //wakeUpTime = 60 - sCurTime.u32cSecond;
        //if(wakeUpTime < 5)
        //    wakeUpTime = wakeUpTime + 60;
        //#endif
        if(wakeUpSource != 0xffffffff)
        {
            sysprintf("Before suspend(wakeUpSource = 0x%08x): %d/%02d/%02d %02d:%02d:%02d\n", 
                    wakeUpSource, sCurTime.u32Year, sCurTime.u32cMonth, sCurTime.u32cDay, sCurTime.u32cHour, sCurTime.u32cMinute, sCurTime.u32cSecond);
        }
        else
        {
            sysprintf("Before suspend(wakeup time = %d): %d/%02d/%02d %02d:%02d:%02d\n", 
                    wakeUpTime, sCurTime.u32Year, sCurTime.u32cMonth, sCurTime.u32cDay, sCurTime.u32cHour, sCurTime.u32cMinute, sCurTime.u32cSecond);
            /* Enable RTC Tick Interrupt and install tick call back function */
            RTC_Ioctl(0, RTC_IOC_SET_RELEATIVE_ALARM, wakeUpTime, (UINT32)RTC_Releative_AlarmISR);
        }
        

        #if(FREERTOS_USE_1000MHZ)
        sysDelay(5/portTICK_RATE_MS);
        #else
        sysDelay(10/portTICK_RATE_MS);
        #endif

        outpw(REG_SYS_WKUPSSR , inpw(REG_SYS_WKUPSSR)); // clean wakeup status

        outpw(0xB00001FC, 0x59);
        outpw(0xB00001FC, 0x16);
        outpw(0xB00001FC, 0x88);
        while(!(inpw(0xB00001FC) & 0x1));
#if(0)
        outpw(REG_SYS_WKUPSER , (1 << EINT_BIT_DEFINE)| (1 << 4)| (1 << 5)| (1 << 6)| (1 << 7)|(1 << 24)); // wakeup source select EINT? DIP(EINT4, EINT5, EINT6) Keypad(EINT7) RTC
#else
        if(wakeUpSource != 0xffffffff)
        {
            outpw(REG_SYS_WKUPSER , wakeUpSource); // wakeup source select Battery(EINT5, EINT6) 
        }
        else
        {
            outpw(REG_SYS_WKUPSER , (1 << 2)| (1 << 3)| (1 << 4)| (1 << 1)|(1 << 24)); // wakeup source select DIP(EINT2, EINT3, EINT4) Keypad(EINT1) RTC
            //outpw(REG_SYS_WKUPSER , (1 << 2)| (1 << 3)| (1 << 4)| (1 << 5)| (1 << 6)| (1 << 1)|(1 << 24)); // wakeup source select DIP(EINT2, EINT3, EINT4) Battery(EINT5, EINT6) Keypad(EINT1) RTC
        }
#endif             
        reg=inpw(REG_CLK_PMCON);   //Enable NUC970 to enter power down mode
        reg = reg & (0xFF00FFFE);
        outpw(REG_CLK_PMCON,reg);
        
        backupReg();    
        __wfi();    
        restoreReg();
        
        outpw(REG_SYS_WKUPSER , 0); // wakeup source select NONE  
        mWakeupTick = xTaskGetTickCount();
        /* Get the currnet time */
        if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &sCurTime))
        {
            sysprintf("After suspend: %d/%02d/%02d %02d:%02d:%02d\n",
                sCurTime.u32Year, sCurTime.u32cMonth, sCurTime.u32cDay, sCurTime.u32cHour, sCurTime.u32cMinute, sCurTime.u32cSecond);
        }
        else
        {
            sysprintf("After suspend: RTC_Read error\n");
        }
    }
    else
    {
        sysprintf("\r\n *** Suspend ignore [RTC_Read error] ***\n");
    }
    
    return TRUE;
    
}
void PowerSetWakeupTime(UINT32 time)
{
    sysprintf("PowerSetWakeupTime: ori = %d, new = %d\n", wakeUpTime, time);
    wakeUpTime = time;
}

 
void PowerClearISR(void)
{   
    //sysprintf(">> PowerClearISR: REG_SYS_WKUPSSR = 0x%08x....\n", inpw(REG_SYS_WKUPSSR));
    if(inpw(REG_SYS_WKUPSSR) & (1<<24))
    {
        outpw(REG_SYS_WKUPSSR,(1<<24));
        RTC_Ioctl(0,RTC_IOC_DISABLE_INT,RTC_RELATIVE_ALARM_INT,0);
        wakeupSource = WAKEUP_SOURCE_RTC;
        sysprintf(" ***** RTC Wakeup ***** \n\n");          
    }    
    #if(0)
    if(inpw(REG_SYS_WKUPSSR) & (1<<EINT_BIT_DEFINE))
    {
        outpw(REG_SYS_WKUPSSR,(1<<EINT_BIT_DEFINE));
        GPIO_ClrISRBit(EINT_PORT, EINT_BIT);  
        GPIO_DisableTriggerType(EINT_PORT, EINT_BIT);  
        wakeupSource = WAKEUP_SOURCE_USER;
        sysprintf(" ***** EINT%d Wakeup ***** \n\n", EINT_BIT_DEFINE);
    }
    #endif
    if(inpw(REG_SYS_WKUPSSR) & (1<<2))
    {
        outpw(REG_SYS_WKUPSSR,(1<<2));
        wakeupSource = WAKEUP_SOURCE_DIP;
        sysprintf(" ***** EINT%d Wakeup ***** \n\n", 2);
    }
    if(inpw(REG_SYS_WKUPSSR) & (1<<3))
    {
        outpw(REG_SYS_WKUPSSR,(1<<3));
        wakeupSource = WAKEUP_SOURCE_DIP;
        sysprintf(" ***** EINT%d Wakeup ***** \n\n", 3);
    }
    if(inpw(REG_SYS_WKUPSSR) & (1<<4))
    {
        outpw(REG_SYS_WKUPSSR,(1<<4));
        wakeupSource = WAKEUP_SOURCE_DIP;
        sysprintf(" ***** EINT%d Wakeup ***** \n\n", 4);
    }
    if(inpw(REG_SYS_WKUPSSR) & (1<<5))
    {
        outpw(REG_SYS_WKUPSSR,(1<<5));
        wakeupSource = WAKEUP_SOURCE_BATTERY;
        sysprintf(" ***** EINT%d Wakeup ***** \n\n", 5);
    }
    if(inpw(REG_SYS_WKUPSSR) & (1<<6))
    {
        outpw(REG_SYS_WKUPSSR,(1<<6));
        wakeupSource = WAKEUP_SOURCE_BATTERY;
        sysprintf(" ***** EINT%d Wakeup ***** \n\n", 6);
    }
    if(inpw(REG_SYS_WKUPSSR) & (1<<1))
    {
        outpw(REG_SYS_WKUPSSR,(1<<1));
        wakeupSource = WAKEUP_SOURCE_KEYPAD;
        sysprintf(" ***** EINT%d Wakeup ***** \n\n", 1);
    }
    if(inpw(REG_SYS_WKUPSSR) != 0x0)
    {
        wakeupSource = WAKEUP_SOURCE_OTHER;
        sysprintf(" ***** Other Wakeup 0x%08x *****\n\n",inpw(REG_SYS_WKUPSSR));
    }
    
    outpw(REG_SYS_WKUPSSR , inpw(REG_SYS_WKUPSSR)); //clean wakeup status
}




uint32_t PowerGetTotalWakeupTick(void)
{
    return mTotalWakeupTick;
}

void PowerDrvSetEnable(BOOL flag)
{
    if(xSemaphore == NULL)
        return;
    if(flag)
    {
        threadWaitTime = (POWER_DRV_POLLING_TIME/portTICK_RATE_MS);
    }
    else
    {
        threadWaitTime = portMAX_DELAY;
    }
    xSemaphoreGive(xSemaphore);
}
/*! Unlock protected register */
#define UNLOCKREG  do{outpw(REG_SYS_REGWPCTL,0x59); outpw(REG_SYS_REGWPCTL,0x16); outpw(REG_SYS_REGWPCTL,0x88);}while(inpw(REG_SYS_REGWPCTL) == 0x00)
/*! Lock protected register */
#define LOCKREG  do{outpw(REG_SYS_REGWPCTL,0x00);}while(0)
void PowerDrvResetSystem(void)
{
    sysprintf(" ***** PowerDrvResetSystem 0x%08x *****\n\n",inpw(REG_SYS_AHBIPRST));
    UNLOCKREG;
    outpw(REG_SYS_AHBIPRST,0x1);
    UNLOCKREG;
    sysprintf(" ***** PowerDrvResetSystem 0x%08x *****\n\n",inpw(REG_SYS_AHBIPRST));
}


/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


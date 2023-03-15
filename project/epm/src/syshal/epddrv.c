/**************************************************************************//**
* @file     epddrv.c
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
#include <time.h>
#include "nuc970.h"
#include "sys.h"
#include "gpio.h"
#include "rtc.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "epddrv.h"
#include "interface.h"
#include "powerdrv.h"
#include "buzzerdrv.h"
#include "timelib.h"
#include "meterdata.h"
#include "hwtester.h"

#if (ENABLE_BURNIN_TESTER)
#include "guimanager.h"
//#include "burnintester.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

#define ENABLE_SLEEP_FUNCTION           1

#define SHOW_TIME_DEBUG                 0
#define SHOW_POSITION_DEBUG             0

#define EPD_SPI         SPI_1_INTERFACE_INDEX

#define READY_PIN_PORT  GPIOI
#define READY_PORT_BIT  BIT9

#define RESET_PIN_PORT  GPIOI
#define RESET_PORT_BIT  BIT0

#define POWER_PIN_PORT  GPIOI
#define POWER_PORT_BIT  BIT1

#define BACK_LIGHT_PIN_PORT  GPIOD
#define BACK_LIGHT_PORT_BIT  BIT14

//-------System Registers----------------
#define SYS_REG_BASE 0x0000

//Address of System Registers
#define I80CPCR (SYS_REG_BASE + 0x04)

//Register Base Address
#define DISPLAY_REG_BASE 0x1000 //Register RW access for I80 only
//Update Parameter Setting Register
#define UP0SR (DISPLAY_REG_BASE + 0x134) //Update Parameter0 Setting Reg


//-----------------------------------------



#define USDEF_I80_CMD_DPY_AREA     0x0034
#define USDEF_I80_CMD_GET_DEV_INFO 0x0302

#define USDEF_I80_CMD_VCOM		   0x0039

#define IT8951_TCON_SYS_RUN 0x0001
//#define IT8951_TCON_STANDBY 0x0002
#define IT8951_TCON_SLEEP   0x0003

#define IT8951_TCON_REG_WR       0x0011

#if(1)
    #define EPD_WIDTH 1024
    #define EPD_HEIGHT  758
#else
    #define EPD_WIDTH 800
    #define EPD_HEIGHT  600
#endif


#define BUILD_VERSION_EPD   BUILD_VERSION


/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static BOOL EPDSetPower(BOOL flag);
//static void setTotalPower(BOOL flag);

static SpiInterface* pSpiInterface;
static int waitTimeoutCounter = 0;
static int epdbadCounter = 0;
static SemaphoreHandle_t xSemaphore;
static SemaphoreHandle_t xBackLightSemaphore;
static TickType_t threadBackLightWaitTime        = portMAX_DELAY;
    

static BOOL epdDrvCheckStatus(int flag);
static BOOL epdDrvPreOffCallback(int flag);
static BOOL epdDrvOffCallback(int flag);
static BOOL epdDrvOnCallback(int flag);
static powerCallbackFunc epdDrvPowerCallabck = {" [EPDDrv] ", epdDrvPreOffCallback, epdDrvOffCallback, epdDrvOnCallback, epdDrvCheckStatus};

static epdReinitCallbackFunc pEpdReinitCallbackFunc = NULL;
static BOOL initFlag = FALSE;
static BOOL epdbadFlag = FALSE;
static BOOL epdopenFlag = TRUE;

static  TWord tempFWVersion[8]; //16 Bytes String
static  TWord tempLUTVersion[8]; //16 Bytes String


static BOOL specialCleanFlag = FALSE;

static BOOL flashErrorFlag = FALSE;


static BOOL W25Q64BVspecialburnFlag = FALSE;


static uint32_t dataCode ;

#if(ENABLE_SLEEP_FUNCTION)
static BOOL sleepFunctionFlag = TRUE;
static BOOL sleepEnteredFlag = TRUE;
#endif

static BOOL GetIT8951SystemInfo(void);
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static BOOL WriteCmdCode (int wCmd,int len);

static void EPDEnterSleep(void)
{
    if(sleepEnteredFlag == FALSE)
    {
        if(sleepFunctionFlag)
        {
            //sysprintf("\r\n-- EPD : SLEEP --\r\n");
            WriteCmdCode(IT8951_TCON_SLEEP,2);   
            pSpiInterface->resetPin();  
            sleepEnteredFlag = TRUE;
        }
    }
}
static void EPDExitSleep(void)
{
    if(sleepEnteredFlag)
    {
        //sysprintf("\r\n-- EPD : RUN --\r\n");
        pSpiInterface->setPin();   
        WriteCmdCode(IT8951_TCON_SYS_RUN,2); 
        sleepEnteredFlag = FALSE;
    }
}
static BOOL epdDrvPreOffCallback(int flag)
{
    //BOOL reVal = TRUE;
    //return reVal;
    if(EPDGetBacklight())
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
static BOOL epdDrvOffCallback(int flag)
{
    //#if(ENABLE_SLEEP_FUNCTION)
    //#else
    EPDSetBacklight(FALSE);
    xSemaphoreTake(xSemaphore, portMAX_DELAY);     
    EPDEnterSleep();    
    xSemaphoreGive(xSemaphore);  
    //#endif
//setTotalPower(FALSE);    
    return TRUE;    
}
static BOOL epdDrvOnCallback(int flag)
{
//setTotalPower(TRUE);  
    #if(ENABLE_SLEEP_FUNCTION)
    #else
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    EPDExitSleep();
    xSemaphoreGive(xSemaphore); 
    #endif
    return TRUE; 
}
static BOOL epdDrvCheckStatus(int flag)
{
    if(EPDGetBacklight())
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


static BOOL EPDSetPower(BOOL flag)
{
    if(flag)
    {         
        sysprintf("-- EPDSetPower ON --\n");
        GPIO_SetBit(POWER_PIN_PORT, POWER_PORT_BIT); 
        GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT); //reset       
    }
    else
    {
        sysprintf("-- EPDSetPower OFF --\n");
        GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT);//reset
        GPIO_ClrBit(POWER_PIN_PORT, POWER_PORT_BIT);  
    }
    return TRUE;
}

static BOOL EPDReset(void)
{         
    sysprintf("-- EPDReset --\n"); 
    //ReloadWWDT();    
    GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT); 
   
    sysDelay(1000/portTICK_RATE_MS);
    //ReloadWWDT();
    sysDelay(1000/portTICK_RATE_MS);
    //ReloadWWDT();
    sysDelay(1000/portTICK_RATE_MS);

    GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    //ReloadWWDT();
    return TRUE;
} 

/*
static BOOL EPDReset(void)
{         
    sysprintf("-- EPDReset --\n");    
    GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT);  
    //sysDelay(50);
    sysDelay(50/portTICK_RATE_MS);
    GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    return TRUE;
}  */


void setTotalPower(BOOL flag)
{
    if(flag)
    {
        GPIO_CloseBit(GPIOI, BIT5);
        GPIO_CloseBit(GPIOI, BIT6);
        GPIO_CloseBit(GPIOI, BIT7);
        GPIO_CloseBit(GPIOI, BIT8);   
        GPIO_CloseBit(READY_PIN_PORT, READY_PORT_BIT);        
        pSpiInterface->setPin();
        EPDSetPower(TRUE);        
        EPDReset();
    }
    else
    {        
        EPDSetPower(FALSE);
        pSpiInterface->resetPin();
        GPIO_OpenBit(GPIOI, BIT5, DIR_OUTPUT, NO_PULL_UP);  
        GPIO_ClrBit(GPIOI, BIT5);
        GPIO_OpenBit(GPIOI, BIT6, DIR_OUTPUT, NO_PULL_UP);  
        GPIO_ClrBit(GPIOI, BIT6);
        GPIO_OpenBit(GPIOI, BIT7, DIR_OUTPUT, NO_PULL_UP);  
        GPIO_ClrBit(GPIOI, BIT7);
        GPIO_OpenBit(GPIOI, BIT8, DIR_OUTPUT, NO_PULL_UP);  
        GPIO_ClrBit(GPIOI, BIT8);
        
        GPIO_OpenBit(READY_PIN_PORT, READY_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);  
        GPIO_ClrBit(READY_PIN_PORT, READY_PORT_BIT);
    }
}

static void reinitEpd(void)
{
    setTotalPower(FALSE);
    sysDelay(1000/portTICK_RATE_MS);
    setTotalPower(TRUE);
    sysDelay(1000/portTICK_RATE_MS);
}

static BOOL hwInit(void)
{
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.

    /* Set MFP_GPD14 to output (backlight) */
    outpw(REG_SYS_GPD_MFPH,(inpw(REG_SYS_GPD_MFPH) & ~(0xF<<24)));
    GPIO_OpenBit(BACK_LIGHT_PIN_PORT, BACK_LIGHT_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);    
    GPIO_SetBit(BACK_LIGHT_PIN_PORT, BACK_LIGHT_PORT_BIT);  
    
    //GPI1 Power pin
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<4)) | (0x0<<4));
    GPIO_OpenBit(POWER_PIN_PORT, POWER_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);  
    
    //RDY Pin
    //GPIO_OpenBit(READY_PIN_PORT, READY_PORT_BIT, DIR_INPUT, NO_PULL_UP);  
    GPIO_OpenBit(READY_PIN_PORT, READY_PORT_BIT, DIR_INPUT, PULL_UP);  //20200519 solve EPD flash error by Steven
    
    //GPI0 reset pin
    outpw(REG_SYS_GPI_MFPL,(inpw(REG_SYS_GPI_MFPL) & ~(0xF<<0)) | (0x0<<0));
    GPIO_OpenBit(RESET_PIN_PORT, RESET_PORT_BIT, DIR_OUTPUT, NO_PULL_UP);
    GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    
    EPDSetBacklight(TRUE);
    
    reinitEpd();    
    return TRUE;
}
extern BOOL SysGetBooted(void);
static void vLedBackLightTask( void *pvParameters )
{
    //vTaskDelay(2000/portTICK_RATE_MS); 
    sysprintf("vLedBackLightTask Going...\r\n");     
    while(SysGetBooted() == FALSE)
    {
        vTaskDelay(500/portTICK_RATE_MS);
    }
    threadBackLightWaitTime = 5000/portTICK_RATE_MS;
    for(;;)
    {     
        BaseType_t reval = xSemaphoreTake(xBackLightSemaphore, threadBackLightWaitTime);         
        if(reval != pdTRUE)
        {//timeout
            terninalPrintf("vLedBackLightTask Error!!\r\n");
            threadBackLightWaitTime = portMAX_DELAY;
            EPDSetBacklight(FALSE);
        }
        else
        {
        }  
    }
   
}
static BOOL swInit(void)
{
    PowerRegCallback(&epdDrvPowerCallabck);
    xSemaphore = xSemaphoreCreateMutex();  
    xBackLightSemaphore = xSemaphoreCreateBinary();
    xTaskCreate( vLedBackLightTask, "vLedBackLightTask", 1024, NULL, EPD_BACK_LIGHT_THREAD_PROI, NULL );
    return TRUE;
}

static BOOL LCDWaitForReadyEx(void)
{
    //terninalPrintf("epdbadFlag = %d\r\n",epdbadFlag);
    //terninalPrintf("epdopenFlag = %d\r\n",epdopenFlag);
    if((epdbadFlag == TRUE) || (epdopenFlag == FALSE))
        return TRUE;
    
    //int timeOut = 60000000;
    int timeOut = 15000;
    
    while(GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT)==0)
    {
        if(timeOut-- ==0)
        {
            epdbadFlag = TRUE;
            //BuzzerPlay(500, 200, 2, TRUE);
            return FALSE;
        }
        vTaskDelay(1/portTICK_RATE_MS);
    }
    return TRUE;
}
static BOOL LCDWaitForReady(void)
{
    //terninalPrintf("epdbadFlag = %d\r\n",epdbadFlag);
    //terninalPrintf("epdopenFlag = %d\r\n",epdopenFlag);

    if((epdbadFlag == TRUE) || (epdopenFlag == FALSE))
        return TRUE;
    

    //int timers = 3500/portTICK_RATE_MS;
    int timers = 5000/portTICK_RATE_MS;
    
    while(GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT)==0)
    {
        //sysprintf("{%d}", 500-timers);

        
        if(timers-- == 0)
        {

            terninalPrintf("badEPD\r\n");
            epdbadFlag = TRUE;
            
            epdbadCounter++;
            if(epdbadCounter >= 3)
                epdbadCounter = 3;
                
            sysprintf("\r\n ##  LCDWaitForReady timeout  ##\n");
            //terninalPrintf("LCDWaitForReady timeout\r\n");
            //terninalPrintf("PI9 = %d.\r\n",GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT));
            waitTimeoutCounter++;
            BuzzerPlay(500, 200, 2, TRUE);
            #if(1)
            reinitEpd();
            #else
            setTotalPower(FALSE);            
            vTaskDelay(200/portTICK_RATE_MS);  
            setTotalPower(TRUE);             
            vTaskDelay(500/portTICK_RATE_MS);
            #endif
            
            timers = 0;
            //if(pEpdReinitCallbackFunc != NULL)
            //    pEpdReinitCallbackFunc();
            
            
            /*
            if (getflashvalue(1) != 0x49)
            {   
                terninalPrintf("EPDflashError\r\n");
                hwtestEDPflashBurn();
                //flashErrorFlag = TRUE;
            }*/
            
            int flashErrorCounter = 0;
            uint8_t tempbyte;
            tempbyte = getflashvalue(0);
            //terninalPrintf("tempbyte = %02x\r\n",tempbyte);
            
            
            while (tempbyte != 0x49)
            {   
                flashErrorCounter++;
                if(flashErrorCounter >= 3)
                {
                    terninalPrintf("EPDflash Error\r\n");
                    break;
                    //hwtestEDPflashBurn();
                }
                tempbyte = getflashvalue(0);
                //terninalPrintf("tempbyte = %02x\r\n",tempbyte);
                vTaskDelay(100/portTICK_RATE_MS);
            }
            
            if(tempbyte == 0x49)
                epdbadFlag = FALSE;
            
            //terninalPrintf("toggleEPDswitch.\r\n");
            
            
            
            
            //toggleEPDswitch(TRUE); //diable hot swap 20200629 by Steven
            
            //terninalPrintf("toggleEPDswitch finish.\r\n");
            

            
            
            EPDSetBacklight(FALSE);
            //terninalPrintf("EPDShowBGScreen.\r\n");
            
            //terninalPrintf("PI9 = %d.\r\n",GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT));
            /*
            if(GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT)==0)
            {
                epdbadFlag = TRUE;
                terninalPrintf("PI9 fail.\r\n");
                return FALSE;
            }  */
            //terninalPrintf("VCOM = %d.\r\n",IT8951GetVCOM());
            
            
            /*
            if(readMBtestFunc())
            {}
            else
            {
                EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE); 
                EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE);
            }
            
            */  //diable hot swap 20200629 by Steven
            
            //terninalPrintf("PI9 ok.\r\n");
            if( epdbadCounter >= 3 )
            {
                if(readAssemblyTestFunc())
                {
                    epdbadCounter = 0;
                    hwtestEDPflashBurnLite();
                }
                else
                    epdbadFlag = TRUE;
            }
            
            return FALSE;
        }
        #if(FREERTOS_USE_1000MHZ)
        vTaskDelay(1/portTICK_RATE_MS); 
        #else
        //#error
        vTaskDelay(10/portTICK_RATE_MS); 
        #endif
    };
    
    //if(xTaskGetTickCount() != mTick)
    //    sysprintf("\r\n{%d}\r\n", xTaskGetTickCount() - mTick);
    

    //epdbadCounter = 0;
    return TRUE;
}

static BOOL WriteCmdCode (int wCmd,int len)
{
    //sysprintf("Write cmd start!!\n");
    //wait ready.
    if(LCDWaitForReady() == FALSE) 
        return FALSE;

    //cs set low.
    pSpiInterface->activeCSFunc(TRUE);

    //spi write preamble.
    pSpiInterface->writeFunc(0, 0x60);
    pSpiInterface->writeFunc(0, 0x00);

    //wait ready.
    if(LCDWaitForReady() == FALSE) 
        return FALSE;

    //send cmd. 0x0302
    pSpiInterface->writeFunc(0, (char)(wCmd>>8));
    pSpiInterface->writeFunc(0, (char)wCmd);

    //cs set high.
    pSpiInterface->activeCSFunc(FALSE);
    //sysprintf("Write cmd over.!!\n");
    
    if(LCDWaitForReady() == FALSE) 
        return FALSE;
    
    return TRUE;
}
static BOOL WriteData(int data,int len)
{
    if(LCDWaitForReady() == FALSE) 
        return FALSE;
    //cs set low
    pSpiInterface->activeCSFunc(TRUE);

    //send write data preamble.
    pSpiInterface->writeFunc(0, 0x00);
    pSpiInterface->writeFunc(0, 0x00);

    if(LCDWaitForReady() == FALSE) 
        return FALSE;

    //Send Data
    pSpiInterface->writeFunc(0, (char)(data>>8));
    pSpiInterface->writeFunc(0, (char)data);

    //cs set high.
    pSpiInterface->activeCSFunc(FALSE);
    
    if(LCDWaitForReady() == FALSE) 
        return FALSE;
    
    return TRUE;
}


static TWord ReadDataEx(int len)
{
    uint32_t rBuff[2];
    TWord buffer;
    int wCmd = 0x1000;
    //sysprintf("Read cmd start.!!\n");
    
    //wait ready.
    if(LCDWaitForReady() == FALSE) 
        return 0xFF;
    //cs set low.
    pSpiInterface->activeCSFunc(TRUE);

    //spi write preamble.
    pSpiInterface->writeFunc(0, (char)(wCmd>>8));
    pSpiInterface->writeFunc(0, (char)wCmd);

    
    if(LCDWaitForReady() == FALSE) 
        return 0xFF; //FALSE;
    
    pSpiInterface->writeFunc(0, 0x00);
    rBuff[0] = pSpiInterface->readFunc(0);
    pSpiInterface->writeFunc(0, 0x00);
    rBuff[1] = pSpiInterface->readFunc(0);

    //if(LCDWaitForReady() == FALSE) 
        //return 0x0;
    
    //Read Data.
    //pSpiInterface->writeFunc(0, 0x00);
    //pSpiInterface->readFunc(0);


    if(LCDWaitForReady() == FALSE)
        return 0xFF;
    
    //pSpiInterface->writeFunc(0, 0x00);
    //pSpiInterface->readFunc(0);
    //pSpiInterface->readFunc(0);
    pSpiInterface->writeFunc(0, 0x00);
    rBuff[0] = pSpiInterface->readFunc(0);
    pSpiInterface->writeFunc(0, 0x00);
    rBuff[1] = pSpiInterface->readFunc(0);
    
    //combine
    buffer = ((rBuff[1]&0xff) | ((rBuff[0]&0xff)<<8));

    //cs set high.
    pSpiInterface->activeCSFunc(FALSE);
    return buffer;
}  




static TWord ReadData(int len)
{
    uint32_t rBuff[2];
    TWord buffer;
    int wCmd = 0x1000;
    //sysprintf("Read cmd start.!!\n");
    
    //wait ready.
    if(LCDWaitForReady() == FALSE) 
        return 0x0;
    //cs set low.
    pSpiInterface->activeCSFunc(TRUE);

    //spi write preamble.
    pSpiInterface->writeFunc(0, (char)(wCmd>>8));
    pSpiInterface->readFunc(0);
    
    if(LCDWaitForReady() == FALSE) 
        return FALSE;
    
    pSpiInterface->writeFunc(0, (char)wCmd);
    pSpiInterface->readFunc(0);


    if(LCDWaitForReady() == FALSE) 
        return 0x0;
    
    //Read Data.
    pSpiInterface->writeFunc(0, 0x00);
    pSpiInterface->readFunc(0);
    rBuff[0] = pSpiInterface->readFunc(0);

    if(LCDWaitForReady() == FALSE)
        return 0x0;
    
    pSpiInterface->writeFunc(0, 0x00);
    pSpiInterface->readFunc(0);
    rBuff[1] = pSpiInterface->readFunc(0);
    
    //combine
    buffer = ((rBuff[1]&0xff) | ((rBuff[0]&0xff)<<8));

    //cs set high.
    pSpiInterface->activeCSFunc(FALSE);
    return buffer;
}  








//static TWord ReadData(TWord * buffer1,TWord * buffer2,TWord * buffer3,TWord * buffer4,
 //                     TWord * buffer5,TWord * buffer6,TWord * buffer7,TWord * buffer8)
 static TWord ReadVersionData(TWord * readbuffer)
{
    uint32_t rBuff[42];
    TWord buffer;
    int wCmd = 0x1000;
    //sysprintf("Read cmd start.!!\n");
    
    //wait ready.
    if(LCDWaitForReady() == FALSE) 
        return 0x0;
    //cs set low.
    pSpiInterface->activeCSFunc(TRUE);

    //spi write preamble.
    pSpiInterface->writeFunc(0, (char)(wCmd>>8));
    pSpiInterface->readFunc(0);
    
    if(LCDWaitForReady() == FALSE) 
        return FALSE;
    
    pSpiInterface->writeFunc(0, (char)wCmd);
    pSpiInterface->readFunc(0);

    for(int i=0 ;i<20;i++)
    {

    if(LCDWaitForReady() == FALSE) 
        return 0x0;
    
    //Read Data.
    pSpiInterface->writeFunc(0, 0x00);
    pSpiInterface->readFunc(0);
    rBuff[2*i] = pSpiInterface->readFunc(0);

    if(LCDWaitForReady() == FALSE)
        return 0x0;
    
    pSpiInterface->writeFunc(0, 0x00);
    pSpiInterface->readFunc(0);
    rBuff[2*i+1] = pSpiInterface->readFunc(0);
    
    
    }
    
    
    
    for(int t=0 ;t<20;t++)
    {
        *(readbuffer + t) = ((rBuff[2*(t+1)]&0xff) | ((rBuff[2*(t+1)+1]&0xff)<<8));
    }

    
    //cs set high.
    pSpiInterface->activeCSFunc(FALSE);
    return buffer;
}









static BOOL GetIT8951SystemInfo(void)
{
    int i;
    I80IT8951DevInfo gstI80DevInfo;
    TWord* pusWord = (TWord*)&gstI80DevInfo;
    I80IT8951DevInfo* pstDevInfo;
    memset(&gstI80DevInfo, 0x0, sizeof(I80IT8951DevInfo));
    //Send I80 CMD
    WriteCmdCode(USDEF_I80_CMD_GET_DEV_INFO, 2);
    //Get System Info
    

    //for(i=0; i<sizeof(I80IT8951DevInfo)/2; i++)
    //for(i=0; i<sizeof(I80IT8951DevInfo)/40; i++)
    //{

        
        //pusWord[i] = ReadData(2);
        
        
        ReadVersionData(pusWord);
        
        //for(int j=0;j<20;j++)
       // {
        //    terninalPrintf ("pusWord[%d] = %04x\r\n",j ,pusWord[j]);
        //}
        
        

   // }
    memset((uint8_t*)tempFWVersion,0x00,16);
    memset((uint8_t*)tempLUTVersion,0x00,16);
    if(pusWord[0] == 0xFFFF)
        return FALSE;
    //Show Device information of IT8951
    pstDevInfo = (I80IT8951DevInfo*)&gstI80DevInfo;
    sysprintf("Panel(W,H) = (%d,%d)\r\n", pstDevInfo->usPanelW, pstDevInfo->usPanelH );
    sysprintf("Image Buffer Address = %X\r\n", pstDevInfo->usImgBufAddrL | (pstDevInfo->usImgBufAddrH << 16));
    
#if (ENABLE_BURNIN_TESTER)
    if ((pstDevInfo->usImgBufAddrL == 0) && (pstDevInfo->usImgBufAddrH == 0)) {
        SetEpdErrorFlag(TRUE);
    }
    if ((pstDevInfo->usImgBufAddrL == 0xFFFF) && (pstDevInfo->usImgBufAddrH == 0xFFFF)) {
        SetEpdErrorFlag(TRUE);
    }
#endif 
    
    //Show Firmware and LUT Version

    memcpy(tempFWVersion,pstDevInfo->usFWVersion,16);
    memcpy(tempLUTVersion,pstDevInfo->usLUTVersion,16);

    //terninalPrintf ("FW VersionOri = [%s]\r\n", pstDevInfo->usFWVersion);
    //terninalPrintf ("LUT VersionOri = [%s]\r\n", pstDevInfo->usLUTVersion);
    //terninalPrintf("FW Versiontest = [%s]\r\n", tempFWVersion);
    //terninalPrintf("LUT Versiontest = [%s]\r\n", tempLUTVersion);
		return TRUE;		
    
}


static void wordSwapBytes(TWord *singleWord)
{
    char *ptrByte = (char *)singleWord;
    char tempByte = ptrByte[0];
    ptrByte[0] = ptrByte[1];
    ptrByte[1] = tempByte;
}

/*
static BOOL GetIT8951SystemInfo(void)
{
    int i;
    I80IT8951DevInfo gstI80DevInfo;
    TWord* pusWord = (TWord*)&gstI80DevInfo;
    I80IT8951DevInfo* pstDevInfo;
    memset(&gstI80DevInfo, 0x0, sizeof(I80IT8951DevInfo));
    //Send I80 CMD
    WriteCmdCode(USDEF_I80_CMD_GET_DEV_INFO, 2);
    //Get System Info   
    //ReadDataEx(pusWord, sizeof(I80IT8951DevInfo)/2);
        
    for(i=0; i<sizeof(I80IT8951DevInfo)/2; i++)
    {
        pusWord[i] = ReadData(2);
    }
    if(pusWord[0] == 0xFFFF)
        return FALSE;
    
    //Show Device information of IT8951
    pstDevInfo = (I80IT8951DevInfo*)&gstI80DevInfo;
    sysprintf("Panel(W,H) = (%d,%d)\r\n", pstDevInfo->usPanelW, pstDevInfo->usPanelH );
    sysprintf("Image Buffer Address = %X\r\n", pstDevInfo->usImgBufAddrL | (pstDevInfo->usImgBufAddrH << 16));
    //Show Firmware and LUT Version
    for (int i = 0 ; i < (sizeof(pstDevInfo->usFWVersion) / 2) ; i++) {
        wordSwapBytes(&(pstDevInfo->usFWVersion[i]));
    }
    for (int i = 0 ; i < (sizeof(pstDevInfo->usLUTVersion) / 2) ; i++) {
        wordSwapBytes(&(pstDevInfo->usLUTVersion[i]));
    }
    sysprintf ("FW Version = [%s]\r\n", pstDevInfo->usFWVersion);
    sysprintf ("LUT Version = [%s]\r\n", pstDevInfo->usLUTVersion);
    
    memcpy(tempFWVersion,pstDevInfo->usFWVersion,16);
    memcpy(tempLUTVersion,pstDevInfo->usLUTVersion,16);

    return TRUE;
}
*/


BOOL ReadIT8951SystemInfo(char* readFWVersion,char* readLUTVersion)
{
    WriteCmdCode(IT8951_TCON_SYS_RUN,2);
    if(!GetIT8951SystemInfo())
        return FALSE;
    //GetIT8951SystemInfo();
    if(readFWVersion != NULL)
        memcpy(readFWVersion,tempFWVersion,16);
    if(readLUTVersion != NULL)
        memcpy(readLUTVersion,tempLUTVersion,16);
    //readFWVersion = tempFWVersion;
    //readLUTVersion = tempLUTVersion;
    //terninalPrintf("FW test = [%s]\r\n", readFWVersion);
    //terninalPrintf("LUT test = [%s]\r\n", readLUTVersion);
    return TRUE;
}

BOOL ReadIT8951SystemInfoLite(char* readFWVersion,char* readLUTVersion)
{
    if(tempFWVersion[0] == 0x00)
        return FALSE;
    //GetIT8951SystemInfo();
    if(readFWVersion != NULL)
        memcpy(readFWVersion,tempFWVersion,16);
    if(readLUTVersion != NULL)
        memcpy(readLUTVersion,tempLUTVersion,16);
    //readFWVersion = tempFWVersion;
    //readLUTVersion = tempLUTVersion;
    //terninalPrintf("FW test = [%s]\r\n", readFWVersion);
    //terninalPrintf("LUT test = [%s]\r\n", readLUTVersion);
    return TRUE;
}

static uint8_t SpiFlash_ReadStatusReg(void)
{
    uint8_t u8Status;

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE); 

    // send Command: 0x05, Read status register
    pSpiInterface->writeFunc(0, 0x05);

    // read status
    pSpiInterface->writeFunc(0, 0x00);

    u8Status = pSpiInterface->readFunc(0);
    //vTaskDelay(10/portTICK_RATE_MS);
    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE); 

    return u8Status;
}





static void SpiFlash_WaitReady(void)
{
    int counter =  5000/10; 
    uint8_t ReturnValue;

    do {
        //vTaskDelay(1/portTICK_RATE_MS); //add by sam
        #if(FREERTOS_USE_1000MHZ)
        vTaskDelay(1/portTICK_RATE_MS); 
        #error
        #else
        //terninalPrintf("_"); 
        
        if(W25Q64BVspecialburnFlag)
            vTaskDelay(10/portTICK_RATE_MS); 
        else
            vTaskDelay(10/portTICK_RATE_MS);
        
        if(counter-- == 0)
        {
            terninalPrintf("\r\n #################################\r\n"); 
            terninalPrintf(    " ### SpiFlash_WaitReady  break ### \r\n"); 
            terninalPrintf(    " ################################# \r\n"); 
            break;  
        }            
        #endif
        ReturnValue = SpiFlash_ReadStatusReg();
        ReturnValue = ReturnValue & 1;
        
    } while(ReturnValue!=0); // check the BUSY bit
    
    //terninalPrintf("counter = %d \r\n",counter);
}

static void SpiFlash_WaitReadyEx(void)
{
    int counter =  5000/10; 
    uint8_t ReturnValue;
    
    while((SpiFlash_ReadStatusReg() & 1) != 0)
    {
        if(counter-- == 0)
        {
            terninalPrintf("\r\n #################################\r\n"); 
            terninalPrintf(    " ### SpiFlash_WaitReady  break ### \r\n"); 
            terninalPrintf(    " ################################# \r\n"); 
            break;  
        }       
        vTaskDelay(1/portTICK_RATE_MS);  
    }    
    
}

BOOL SpiFlash_WaitEraseReady(void)
{
    int counter =  100; 
    uint8_t ReturnValue;
    
    while((SpiFlash_ReadStatusReg() & 1) != 0)
    {
        if(counter-- == 0)
        {
            return FALSE;
        }       
        vTaskDelay(10/portTICK_RATE_MS);  
    }
    return TRUE;
    
}

static void SpiFlash_NormalPageProgram(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i = 0;

    // /CS: active
    pSpiInterface->activeCSFunc(TRUE); 

    // send Command: 0x06, Write enable
    pSpiInterface->writeFunc(0, 0x06);

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);

    //vTaskDelay(1/portTICK_RATE_MS);
    // /CS: active
    pSpiInterface->activeCSFunc(TRUE); 

    // send Command: 0x02, Page program
    pSpiInterface->writeFunc(0, 0x02);

    // send 24-bit start address
    pSpiInterface->writeFunc(0, (StartAddress>>16) & 0xFF);

    pSpiInterface->writeFunc(0, (StartAddress>>8) & 0xFF);

    pSpiInterface->writeFunc(0, StartAddress & 0xFF);

    // write data
    for(i=0; i<BuffLen; i++) {
        pSpiInterface->writeFunc(0, u8DataBuffer[i]);
    }

    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);
}
static void SpiFlash_NormalSectorProgram(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i = 0;
    uint32_t tempLen = BuffLen;
    
    for(i = 0; i < BuffLen; i = i + SPI_FLASH_EX_PAGE_SIZE)
    {

        if(tempLen<=SPI_FLASH_EX_PAGE_SIZE)
        {
            SpiFlash_WaitReadyEx();
            SpiFlash_NormalPageProgram(StartAddress + i, u8DataBuffer + i, tempLen);
            //vTaskDelay(10/portTICK_RATE_MS);
            //vTaskDelay(1/100);
            //SpiFlash_NormalPageProgram(StartAddress + i, u8DataBuffer + i, tempLen);
            
            //SpiFlash_NormalPageProgram(StartAddress + i, u8DataBuffer + i, tempLen);
            //SpiFlash_WaitReady();
            //vTaskDelay(1/portTICK_RATE_MS);
            //vTaskDelay(1/100);
            break;            
        }
        else
        {
            //terninalPrintf("PageProgramCounter = %d\r\n", i);
            SpiFlash_WaitReadyEx();
            SpiFlash_NormalPageProgram(StartAddress + i, u8DataBuffer + i, SPI_FLASH_EX_PAGE_SIZE);
            //vTaskDelay(10/portTICK_RATE_MS);
            //vTaskDelay(1/100);
            //SpiFlash_NormalPageProgram(StartAddress + i, u8DataBuffer + i, SPI_FLASH_EX_PAGE_SIZE);
            
            //SpiFlash_NormalPageProgram(StartAddress + i, u8DataBuffer + i, SPI_FLASH_EX_PAGE_SIZE);
            //SpiFlash_WaitReady();
            //vTaskDelay(1/portTICK_RATE_MS);
            //vTaskDelay(1/100);
            tempLen = tempLen - SPI_FLASH_EX_PAGE_SIZE;
        }
        
    }   
}


void W25Q64BVBurn(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    //GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    //vTaskDelay(10/portTICK_RATE_MS);
    //vTaskDelay(15/portTICK_RATE_MS);
   
    SpiFlash_NormalSectorProgram(StartAddress,u8DataBuffer,BuffLen);
    
    //GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT);
    
    epdbadFlag = FALSE;
}


void W25Q64BVErase(void)
{   
    GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    vTaskDelay(15/portTICK_RATE_MS);
    
    SpiFlash_WaitReady();
    
    pSpiInterface->activeCSFunc(TRUE);   
    pSpiInterface->writeFunc(0, 0x06);
    pSpiInterface->activeCSFunc(FALSE); 
    
    
    
    pSpiInterface->activeCSFunc(TRUE);   
    
    pSpiInterface->writeFunc(0, 0xC7);
    
    
    pSpiInterface->activeCSFunc(FALSE);   
    
    SpiFlash_WaitReady();
    
    
    GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    
    epdbadFlag = TRUE;
    
}


void W25Q64BVquery(int temposition)
{
    int size = 4000;
    uint8_t buff[size];

    GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    vTaskDelay(10/portTICK_RATE_MS);

    SpiFlash_WaitReady();

    pSpiInterface->activeCSFunc(TRUE);    

    pSpiInterface->writeFunc(0, 0x03);

    pSpiInterface->writeFunc(0, temposition>>16);
    pSpiInterface->writeFunc(0, temposition>>8);
    pSpiInterface->writeFunc(0, temposition & 0xFF);


    for(int i=0;i<size;i++)
    {
        pSpiInterface->writeFunc(0, 0x00);
        buff[i] = pSpiInterface->readFunc(0); 
        
    }


    pSpiInterface->activeCSFunc(FALSE);   


    terninalPrintf("buff = ");
    for(int i=0;i<size;i++)
    {
        terninalPrintf("%02x ",buff[i]); 
    }
    terninalPrintf("\r\n"); 


    GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT);  

}


void W25Q64BVqueryEx(int temposition,uint8_t *u8DataBuffer,int size)
{
    //int size = 4000;
    uint8_t buff[size];

    //GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    //vTaskDelay(10/portTICK_RATE_MS);

    SpiFlash_WaitReady();

    pSpiInterface->activeCSFunc(TRUE);    

    pSpiInterface->writeFunc(0, 0x03);

    pSpiInterface->writeFunc(0, temposition>>16);
    pSpiInterface->writeFunc(0, temposition>>8);
    pSpiInterface->writeFunc(0, temposition & 0xFF);


    for(int i=0;i<size;i++)
    {
        pSpiInterface->writeFunc(0, 0x00);
        buff[i] = pSpiInterface->readFunc(0); 
        
    }


    pSpiInterface->activeCSFunc(FALSE);   

    
    //terninalPrintf("buff = ");
    for(int i=0;i<size;i++)
    {
        //*(u8DataBuffer + i) = buff[i];
        
        u8DataBuffer[i] = buff[i];
        
        //terninalPrintf("%02x ",buff[i]); 
    }
    //terninalPrintf("\r\n"); 
    

    //GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT);  

}

void W25Q64BVdeviceID(void)
{
    
    uint8_t IDbuff[5];
    
    GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    vTaskDelay(10/portTICK_RATE_MS);
    SpiFlash_WaitReady();
    
    pSpiInterface->activeCSFunc(TRUE);    

    pSpiInterface->writeFunc(0, 0x90);

    //pSpiInterface->writeFunc(0, temposition>>16);
    //pSpiInterface->writeFunc(0, temposition>>8);
    //pSpiInterface->writeFunc(0, temposition & 0xFF);

    
    for(int i=0;i<5;i++)
    {
        pSpiInterface->writeFunc(0, 0x00);
        IDbuff[i] = pSpiInterface->readFunc(0); 
        
    }
    

    pSpiInterface->activeCSFunc(FALSE);   


    terninalPrintf("\r\nDeviceID = ");
    for(int i=0;i<5;i++)
    {
        terninalPrintf("%02x ",IDbuff[i]); 
    }
    terninalPrintf("\r\n"); 
    
    
    GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT);  
}

void setW25Q64BVspecialburn(void)
{
    W25Q64BVspecialburnFlag = TRUE;
}

void clrW25Q64BVspecialburn(void)
{
    W25Q64BVspecialburnFlag = FALSE;
}

BOOL readEPDswitch(void)
{
    return epdopenFlag;
}

void toggleEPDswitch(BOOL toggleEPDflag)
{
    epdopenFlag = toggleEPDflag;
    epdbadFlag = FALSE;
    if (toggleEPDflag)
    {
        //pSpiInterface = SpiGetInterface(EPD_SPI);
        //pSpiInterface->initFunc();
        
        
        hwInit();
        swInit();
    }
}


//load images from spi
static void LoadImages(unsigned int *data,int len)
{
    int i;
    if(len == 0)
    {
        return;
    }
    vTaskDelay(15/portTICK_RATE_MS);
    #if(ENABLE_SLEEP_FUNCTION)
    EPDExitSleep();
    
    //tickLocalStart = xTaskGetTickCount();
    #endif
    WriteCmdCode(0x0099,2);

    //tickLocalStart = xTaskGetTickCount();
    WriteData(data[0],2);

    //tickLocalStart = xTaskGetTickCount();
    for(i=1; i<(len*3+1); i++)
    {
        WriteData(data[i],2);
    }
    #if(ENABLE_SLEEP_FUNCTION)
//    EPDEnterSleep();
    #endif
}
static void DisplayArea(TWord usX, TWord usY, TWord usW, TWord usH, TWord usDpyMode)
{
    TickType_t tickLocalStart = xTaskGetTickCount();
    BOOL redoFlag = TRUE;
    #if(ENABLE_SLEEP_FUNCTION)
    EPDExitSleep();    
    #endif
    while(redoFlag)
    {
        redoFlag = FALSE;
        //Send I80 Display Command (User defined command of IT8951)
        //sysprintf("\n{1}=[%d]\n", xTaskGetTickCount() - tickLocalStart);  
        if(WriteCmdCode(USDEF_I80_CMD_DPY_AREA,2) == FALSE) //0x0034
        {
            redoFlag = TRUE;
            goto reDoExit;
        }
        //sysprintf("{2}=[%d]\n", xTaskGetTickCount() - tickLocalStart);  
        //Write arguments
        if(WriteData(usX,2) == FALSE)
        {
            redoFlag = TRUE;
            goto reDoExit;
        }
        //sysprintf("{3}=[%d]\n", xTaskGetTickCount() - tickLocalStart);  
        if(WriteData(usY,2) == FALSE)
        {
            redoFlag = TRUE;
            goto reDoExit;
        }
        //sysprintf("{4}=[%d]\n", xTaskGetTickCount() - tickLocalStart);  
        if(WriteData(usW,2) == FALSE)
        {
            redoFlag = TRUE;
            goto reDoExit;
        }
        //sysprintf("{5}=[%d]\n", xTaskGetTickCount() - tickLocalStart);  
        if(WriteData(usH,2) == FALSE)
        {
            redoFlag = TRUE;
            goto reDoExit;
        }
        //sysprintf("{6}=[%d]\n", xTaskGetTickCount() - tickLocalStart);  
        if(WriteData(usDpyMode,2) == FALSE)
        {
            redoFlag = TRUE;
            goto reDoExit;
        }
        //sysprintf("{7}=[%d]\n", xTaskGetTickCount() - tickLocalStart);  
        reDoExit:
            ;
    }
    #if(ENABLE_SLEEP_FUNCTION)
    EPDEnterSleep();
    #endif
    sysprintf(" -- {INFO:[%d]} --\n", xTaskGetTickCount() - tickLocalStart);  
}

static uint32_t pow10Ex(uint8_t pow)
{
    uint32_t reval = 1;
    int i = 0;
    for(i = 0; i<pow; i++)
    {
        reval = reval * 10;
    }
    return reval;
}
static uint8_t hourTmp = 0xff, minsTmp = 0xff;
#define TIME_TOTAL_NUM_POSITION      5
static BOOL ShowTime(uint16_t xPosition, uint16_t yPosition, uint8_t fontType, uint8_t hour, uint8_t mins, BOOL checkFlag)
{
    uint8_t showNum[TIME_TOTAL_NUM_POSITION];
    uint8_t /*fontLineIndex, */fontNumIndex, fontColonIndex, fontWidth;
    uint8_t buffIndex = 0;   
    int i;
    unsigned int buff[3*TIME_TOTAL_NUM_POSITION+1];  
    
    #if(BUILD_DEBUG_VERSION)
    if(reFreshFlag == 2)    
        sysprintf("  [INFORMATION] EPD:ShowTime [%02d:%02d] refresh all...\n", hour, mins);  
    else
        sysprintf("EPD:ShowTime [%02d:%02d]...\n", hour, mins); 
    #endif
    
    if(checkFlag)
    {
        if((hourTmp == hour) && (minsTmp == mins))
        {
            sysprintf("EPD:ShowTime [%02d:%02d] ignore...\n", hour, mins); 
            return FALSE;
        }
    }
    //sysprintf("EPD:ShowTime [%02d:%02d]...\n", hour, mins); 
    hourTmp = hour;
    minsTmp = mins;
    
    switch(fontType)
    {
        case EPD_FONT_TYPE_DEBUG:
            //fontLineIndex = EPD_PICT_NUM_DEBUG_INDEX;//EPD_PICT_LINE_BIG_INDEX ?????
//            fontNumIndex = EPD_PICT_NUM_DEBUG_INDEX;
//            fontColonIndex = EPD_PICT_COLON_DEBUG_INDEX;
            fontWidth = 12;
            break;        
        case EPD_FONT_TYPE_SMALL_2:
            //fontLineIndex = EPD_PICT_NUM_SMALL_2_INDEX;//EPD_PICT_LINE_BIG_INDEX ?????
            fontNumIndex = EPD_PICT_NUM_SMALL_2_INDEX;
            fontColonIndex = EPD_PICT_COLON_SMALL_2_INDEX;
            fontWidth = 21;
            break;
        
        case EPD_FONT_TYPE_SMALL:
            //break;
        case EPD_FONT_TYPE_MEDIUM:
            //break;        
        case EPD_FONT_TYPE_BIG:
            //break;         
        default:
            sysprintf("EPD:ShowTime  not support this font (id:%d)\n", fontType); 
            return FALSE;           
    }
    
    showNum[0] = hour/10 + fontNumIndex;
    showNum[1] = hour%10 + fontNumIndex;
    showNum[2] = fontColonIndex;
    showNum[3] = mins/10 + fontNumIndex;
    showNum[4] = mins%10 + fontNumIndex;

    buff[0] = TIME_TOTAL_NUM_POSITION;   
    
    for(i = 0; i<TIME_TOTAL_NUM_POSITION; i++) 
    {
            buff[++buffIndex] = showNum[i];
            buff[++buffIndex] = xPosition - i*fontWidth;         
            buff[++buffIndex] = yPosition;
    }
    #if(0)
    for(i = 0; i<buff[0]*3+1; i++) 
    {
        sysprintf("EPD:buff[%02d]: 0x%04x(%d).. \n", i, buff[i], buff[i]);  
    }
    #endif
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 

    LoadImages(buff, buff[0]);
    
    xSemaphoreGive(xSemaphore); 
    return TRUE;    
}

static UINT32 yearTmp = 0xffffffff, monthTmp = 0xffffffff, dayTmp = 0xffffffff;
#define DATE_TOTAL_NUM_POSITION     10
static BOOL ShowDate(uint16_t xPosition, uint16_t yPosition, uint8_t fontType, UINT32 year, UINT32 month, UINT32 day, BOOL checkFlag)
{
    uint8_t showNum[DATE_TOTAL_NUM_POSITION];
    uint8_t /*fontLineIndex, */fontNumIndex, fontSlashIndex, fontWidth;
    uint8_t buffIndex = 0;;
   
    int i;
    unsigned int buff[3*DATE_TOTAL_NUM_POSITION+1];
    
    #if(BUILD_DEBUG_VERSION)
    sysprintf("EPD:ShowDate [%04d/%02d/%02d]...\n", year, month, day);
    #endif
    
    if(checkFlag)
    {
        if((yearTmp == year) && (monthTmp == month) && (dayTmp == day))
        {
            //sysprintf("EPD:ShowDate [%04d/%02d/%02d] ignore...\n", year, month, day);
            return FALSE;
        }
    }
    //sysprintf("EPD:ShowDate [%04d/%02d/%02d]...\n", year, month, day);
    yearTmp = year;
    monthTmp = month;
    dayTmp = day;
    
    switch(fontType)
    {
        case EPD_FONT_TYPE_DEBUG:
            //fontLineIndex = EPD_PICT_NUM_DEBUG_INDEX;//EPD_PICT_LINE_BIG_INDEX ?????
//            fontNumIndex = EPD_PICT_NUM_DEBUG_INDEX;
//            fontSlashIndex = EPD_PICT_SLASH_DEBUG_INDEX;
            fontWidth = 12;
            break;        
        case EPD_FONT_TYPE_SMALL_2:
            //fontLineIndex = EPD_PICT_NUM_SMALL_2_INDEX;//EPD_PICT_LINE_BIG_INDEX ?????
            fontNumIndex = EPD_PICT_NUM_SMALL_2_INDEX;
            fontSlashIndex = EPD_PICT_LINE_SMALL_2_INDEX;
            fontWidth = 21;
            break;
        
        case EPD_FONT_TYPE_SMALL:
            //break;
        case EPD_FONT_TYPE_MEDIUM:
            //break;        
        case EPD_FONT_TYPE_BIG:
            //break;         
        default:
            sysprintf("EPD:ShowDate  not support this font (id:%d)\n", fontType); 
            return FALSE;            
    }
    
    year = year%10000;//??4??
    //sysprintf("EPD:ShowDate [%d/%d/%d]...\n", year/1000, year/100  - year/1000*10, year/10   - year/100*10);
    showNum[0] = year/1000 + fontNumIndex;
    showNum[1] = year/100  - year/1000*10 + fontNumIndex;
    showNum[2] = year/10   - year/100*10   + fontNumIndex;
    showNum[3] = year%10   + fontNumIndex;
    showNum[4] = fontSlashIndex;
    showNum[5] = month/10 + fontNumIndex;
    showNum[6] = month%10 + fontNumIndex;
    showNum[7] = fontSlashIndex;
    showNum[8] = day/10 + fontNumIndex;
    showNum[9] = day%10 + fontNumIndex;
    buff[0] = DATE_TOTAL_NUM_POSITION;   
    
    for(i = 0; i<DATE_TOTAL_NUM_POSITION; i++) 
    {
            buff[++buffIndex] = showNum[i];
            buff[++buffIndex] = xPosition - i*fontWidth;         
            buff[++buffIndex] = yPosition;
    }
    #if(0)
    for(i = 0; i<buff[0]*3+1; i++) 
    {
        sysprintf("EPD:buff[%02d]: 0x%04x(%d).. \n", i, buff[i], buff[i]);  
    }
    #endif
    xSemaphoreTake(xSemaphore, portMAX_DELAY);     
    LoadImages(buff, buff[0]);
    xSemaphoreGive(xSemaphore);  
    return TRUE;
}

static positionAllItemInfo PositionAllItemInfo; 
static positionInfo upperLinePositionInfo = {EPD_PICT_UPPER_LINE_INDEX, 54, 504};
void EPDDrawUpperLine(BOOL reFreshFlag)
{
    unsigned int buff[3*1+1];  
    buff[0] = 1;    
    buff[1] = EPD_PICT_UPPER_LINE_INDEX;
    buff[2] = upperLinePositionInfo.xPos;
    buff[3] = upperLinePositionInfo.yPos; 
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    LoadImages(buff, buff[0]);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    xSemaphoreGive(xSemaphore);  
}

static bannerPositionInfo mBannerPositionInfo;
#define BANNER_START_X_POSITION     668
#define BANNER_START_Y_POSITION     509

#define BANNER_CARD_WIDTH_POSITION     -110
#define BANNER_METER_WIDTH_POSITION     -34
static bannerLinePositionInfo  mBannerLinePositionInfo;
static BOOL calculateBannerLinePositionInfo(void)
{
    mBannerLinePositionInfo.itemNum = mBannerPositionInfo.carEnableNum + 1;
    memcpy(&mBannerLinePositionInfo.info[0], &upperLinePositionInfo, sizeof(positionInfo));
    memcpy(&mBannerLinePositionInfo.info[1], mBannerPositionInfo.info, sizeof(positionInfo)*mBannerPositionInfo.carEnableNum);
    return TRUE;
}

static containPositionInfo mContainPositionInfo;
#define BOARD_START_X_POSITION     427
#define BOARD_START_Y_POSITION     366

#define BOARD_4_START_Y_POSITION    292// (BOARD_START_Y_POSITION-35)
#define BOARD_2_START_Y_POSITION    80// (BOARD_START_Y_POSITION-171)

#define BOARD_WIDTH_POSITION        -374
#define BOARD_HEIGHT_POSITION       -138
#define BOARD_4_HEIGHT_POSITION       -212
#define BOARD_2_HEIGHT_POSITION       -425
static BOOL calculateContainPositionInfo(uint8_t boardEnableNumber)
{    
    int i;
    int targetWidth = 0, targetHight = 0;
    #if(SHOW_POSITION_DEBUG)
    sysprintf("calculateContainPositionInfo: boardEnableNumber = %d\r\n", boardEnableNumber); 
    #endif
    if((boardEnableNumber > CAR_ITEM_MAX_NUM) || (boardEnableNumber < 1))
    {
        sysprintf("calculateContainPositionInfo fail: boardEnableNumber = %d\r\n", boardEnableNumber); 
        return FALSE;
    }
    mContainPositionInfo.boardEnableNum = boardEnableNumber;  
    for(i = 0; i<mContainPositionInfo.boardEnableNum; i++) 
    {
        
        if(i == 0)
        {
            mContainPositionInfo.info[i].xPos = BOARD_START_X_POSITION; 
            switch(boardEnableNumber)
            {
                case 1:
                case 2:
                    mContainPositionInfo.info[i].yPos = BOARD_2_START_Y_POSITION; 
                    break;
                case 3:
                case 4:
                    mContainPositionInfo.info[i].yPos = BOARD_4_START_Y_POSITION; 
                    break;
                default:
                    mContainPositionInfo.info[i].yPos = BOARD_START_Y_POSITION; 
                    break;
            }
            //mContainPositionInfo.info[i].yPos = BOARD_START_Y_POSITION; 
        }
        else
        {
            mContainPositionInfo.info[i].xPos = mContainPositionInfo.info[i-1].xPos + targetWidth; 
            mContainPositionInfo.info[i].yPos = mContainPositionInfo.info[i-1].yPos + targetHight; 
            
        }
//        mContainPositionInfo.info[i].bmpId = EPD_PICT_BOARD_L_INDEX;//EPD_PICT_BOARD_INDEX;
        if(i%2 == 0)
        {
            targetWidth = BOARD_WIDTH_POSITION;
            targetHight = 0;
        }
        else
        {
            targetWidth = -BOARD_WIDTH_POSITION;
            switch(boardEnableNumber)
            {
                case 1:
                case 2:
                    targetHight = BOARD_2_HEIGHT_POSITION;
                    break;
                case 3:
                case 4:
                    targetHight = BOARD_4_HEIGHT_POSITION;
                    break;
                default:
                    targetHight = BOARD_HEIGHT_POSITION;
                    break;
            }
            
        }
        
        #if(SHOW_POSITION_DEBUG)      
        sysprintf("mContainPositionInfo[%d]: [%d: (%03d, %03d)]\r\n", i, mContainPositionInfo.info[i].bmpId, mContainPositionInfo.info[i].xPos, mContainPositionInfo.info[i].yPos); 
        #endif
    }
    
    return TRUE;
}

static containPositionInfo mContainNumberPositionInfo;
#define BOARD_NUMBER_START_X_POSITION     437
#define BOARD_NUMBER_START_Y_POSITION     (BOARD_START_Y_POSITION + 8)//373
#define BOARD_NUMBER_4_START_Y_POSITION   (BOARD_4_START_Y_POSITION + 8)//308//  (BOARD_NUMBER_START_Y_POSITION-35)
#define BOARD_NUMBER_2_START_Y_POSITION   (BOARD_2_START_Y_POSITION + 8)//95//  (BOARD_NUMBER_START_Y_POSITION-171)

#define BOARD_NUMBER_WIDTH_POSITION        -45
#define BOARD_NUMBER_HEIGHT_POSITION       -138
#define BOARD_NUMBER_4_HEIGHT_POSITION       -213
#define BOARD_NUMBER_2_HEIGHT_POSITION       -425
static BOOL calculateContainNumberPositionInfo(uint8_t boardEnableNumber)
{    
    int i;
    int targetWidth = 0, targetHight = 0;
    #if(SHOW_POSITION_DEBUG)
    sysprintf("calculateContainNumberPositionInfo: boardEnableNumber = %d\r\n", boardEnableNumber); 
    #endif
    if((boardEnableNumber > CAR_ITEM_MAX_NUM) || (boardEnableNumber < 1))
    {
        sysprintf("calculateContainNumberPositionInfo fail: boardEnableNumber = %d\r\n", boardEnableNumber); 
        return FALSE;
    }
    mContainNumberPositionInfo.boardEnableNum = boardEnableNumber;  
    for(i = 0; i<mContainNumberPositionInfo.boardEnableNum; i++) 
    {
        
        if(i == 0)
        {
            mContainNumberPositionInfo.info[i].xPos = BOARD_NUMBER_START_X_POSITION; 
            switch(boardEnableNumber)
            {
                case 1:
                case 2:
                    mContainNumberPositionInfo.info[i].yPos = BOARD_NUMBER_2_START_Y_POSITION; 
                    break;
                case 3:
                case 4:
                    mContainNumberPositionInfo.info[i].yPos = BOARD_NUMBER_4_START_Y_POSITION; 
                    break;
                default:
                    mContainNumberPositionInfo.info[i].yPos = BOARD_NUMBER_START_Y_POSITION; 
                    break;
            }
            //mContainNumberPositionInfo.info[i].yPos = BOARD_NUMBER_START_Y_POSITION; 
        }
        else
        {
            mContainNumberPositionInfo.info[i].xPos = mContainNumberPositionInfo.info[i-1].xPos + targetWidth; 
            mContainNumberPositionInfo.info[i].yPos = mContainNumberPositionInfo.info[i-1].yPos + targetHight; 
            
        }
        mContainNumberPositionInfo.info[i].bmpId = EPD_PICT_NUM_SMALL_2_INDEX + i + 1;
        if(i%2 == 0)
        {
            targetWidth = BOARD_NUMBER_WIDTH_POSITION;
            targetHight = 0;
        }
        else
        {
            targetWidth = -BOARD_NUMBER_WIDTH_POSITION;
            
            switch(boardEnableNumber)
            {
                case 1:
                case 2:
                    targetHight = BOARD_NUMBER_2_HEIGHT_POSITION;
                    break;
                case 3:
                case 4:
                    targetHight = BOARD_NUMBER_4_HEIGHT_POSITION;
                    break;
                default:
                    targetHight = BOARD_NUMBER_HEIGHT_POSITION;
                    break;
            }            
        }       
        #if(SHOW_POSITION_DEBUG)      
        sysprintf("mContainNumberPositionInfo[%d]: [%d: (%03d, %03d)]\r\n", i, mContainNumberPositionInfo.info[i].bmpId, mContainNumberPositionInfo.info[i].xPos, mContainNumberPositionInfo.info[i].yPos); 
        #endif
    }
    
    return TRUE;
}

static depositTimeAllPositionInfo mDepositTimeAllPositionInfo;

#define DEPOSIT_TIME_START_X_POSITION     705
#define DEPOSIT_TIME_START_Y_POSITION     390
#define DEPOSIT_TIME_4_START_Y_POSITION     (DEPOSIT_TIME_START_Y_POSITION-35)
#define DEPOSIT_TIME_2_START_Y_POSITION     (DEPOSIT_TIME_START_Y_POSITION-167)

#define DEPOSIT_TIME_FONT_WIDTH            -64
#define DEPOSIT_TIME_COLON_FONT_WIDTH      -55

#define DEPOSIT_TIME_WIDTH_POSITION        -383
#define DEPOSIT_TIME_HEIGHT_POSITION       -138
#define DEPOSIT_TIME_4_HEIGHT_POSITION       -213
#define DEPOSIT_TIME_2_HEIGHT_POSITION       -425
static BOOL calculateDepositTimePositionInfo(uint8_t boardEnableNumber)
{
    int i, j;
    int targetWidth = 0, targetHight = 0;
    int targetWidth2 = 0;
    int xBase = 0, yBase = 0;
    #if(SHOW_POSITION_DEBUG)
    sysprintf("calculateDepositTimePositionInfo: boardEnableNumber = %d\r\n", boardEnableNumber); 
    #endif
    if((boardEnableNumber > CAR_ITEM_MAX_NUM) || (boardEnableNumber < 1))
    {
        sysprintf("calculateDepositTimePositionInfo fail: boardEnableNumber = %d\r\n", boardEnableNumber); 
        return FALSE;
    }
    mDepositTimeAllPositionInfo.depositTimeEnableNum = boardEnableNumber * DEPOSIT_TIME_DIGITAL_NUM;  
    for(i = 0; i</*mDepositTimeAllPositionInfo.depositTimeEnableNum*/boardEnableNumber; i++) 
    {
        if(i == 0)
        {
            xBase = DEPOSIT_TIME_START_X_POSITION; 
            switch(boardEnableNumber)
            {
                case 1:
                case 2:
                    yBase = DEPOSIT_TIME_2_START_Y_POSITION; 
                    break;
                case 3:
                case 4:
                    yBase = DEPOSIT_TIME_4_START_Y_POSITION; 
                    break;
                default:
                    yBase = DEPOSIT_TIME_START_Y_POSITION; 
                    break;
            }   
            //yBase = DEPOSIT_TIME_START_Y_POSITION; 
        }
        else
        {
            xBase = xBase + targetWidth; 
            yBase = yBase + targetHight;                 
        }
        targetWidth2 = 0;
        for(j = 0; j<DEPOSIT_TIME_DIGITAL_NUM; j++) 
        {
            uint8_t targetIndex = DEPOSIT_TIME_DIGITAL_NUM*i+j;
            if(j == 0)
            {
                mDepositTimeAllPositionInfo.info[targetIndex].xPos = xBase; 
                mDepositTimeAllPositionInfo.info[targetIndex].yPos = yBase; 
            }
            else
            {
                mDepositTimeAllPositionInfo.info[targetIndex].xPos = mDepositTimeAllPositionInfo.info[targetIndex-1].xPos + targetWidth2; 
                mDepositTimeAllPositionInfo.info[targetIndex].yPos = mDepositTimeAllPositionInfo.info[targetIndex-1].yPos;                 
            }
            #if(1)
            if(j == 1)
            {                
                targetWidth2 = DEPOSIT_TIME_COLON_FONT_WIDTH + DEPOSIT_TIME_FONT_WIDTH;
            }
            else
            {                
                targetWidth2 = DEPOSIT_TIME_FONT_WIDTH;
            }
            mDepositTimeAllPositionInfo.info[targetIndex].bmpId = EPD_PICT_NUM_BIG_INDEX;
            #else
            if(j == 1)
            {
                mDepositTimeAllPositionInfo.info[targetIndex].bmpId = EPD_PICT_NUM_BIG_INDEX + i + j;
                targetWidth2 = DEPOSIT_TIME_COLON_FONT_WIDTH + DEPOSIT_TIME_FONT_WIDTH;
            }
            else if(j == 2)
            {
                mDepositTimeAllPositionInfo.info[targetIndex].bmpId = EPD_PICT_NUM_BIG_INDEX + i + j;
                targetWidth2 = DEPOSIT_TIME_FONT_WIDTH;
            }
            else
            {
                mDepositTimeAllPositionInfo.info[targetIndex].bmpId = EPD_PICT_NUM_BIG_INDEX + i + j;
                targetWidth2 = DEPOSIT_TIME_FONT_WIDTH;
            }
            #endif
            #if(SHOW_POSITION_DEBUG)            
            sysprintf("mDepositTimeAllPositionInfo[%d]: [%d: (%03d, %03d)]\r\n", targetIndex, mDepositTimeAllPositionInfo.info[targetIndex].bmpId, mDepositTimeAllPositionInfo.info[targetIndex].xPos, mDepositTimeAllPositionInfo.info[targetIndex].yPos); 
            #endif
            
        }
        
        if(i%2 == 0)
        {
            targetWidth = DEPOSIT_TIME_WIDTH_POSITION;
            targetHight = 0;
        }
        else
        {
            targetWidth = -DEPOSIT_TIME_WIDTH_POSITION;
            switch(boardEnableNumber)
            {
                case 1:
                case 2:
                    targetHight = DEPOSIT_TIME_2_HEIGHT_POSITION;
                    break;
                case 3:
                case 4:
                    targetHight = DEPOSIT_TIME_4_HEIGHT_POSITION;
                    break;
                default:
                    targetHight = DEPOSIT_TIME_HEIGHT_POSITION;
                    break;
            }      
            //targetHight = DEPOSIT_TIME_HEIGHT_POSITION;
        }
        
    }
    
    return TRUE;
}


/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL EpdDrvInit(BOOL testModeFlag)
{   
    if(initFlag){
        return TRUE;
    }
    //terninalPrintf("EpdInit!!\r\n");
    sysprintf("EpdInit!!\n");
    pSpiInterface = SpiGetInterface(EPD_SPI);
    if(pSpiInterface == NULL)
    {
        //terninalPrintf("EpdInit ERROR (pSpiInterface == NULL)!!\r\n");
        sysprintf("EpdInit ERROR (pSpiInterface == NULL)!!\n");
        return FALSE;
    }
    if(pSpiInterface->initFunc() == FALSE)
    {
        //terninalPrintf("EpdInit ERROR (initFunc false)!!\r\n");
        sysprintf("EpdInit ERROR (initFunc false)!!\n");
        return FALSE;
    }
    if(hwInit() == FALSE)
    {
        //terninalPrintf("EpdInit ERROR (hwInit false)!!\r\n");
        sysprintf("EpdInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(initFlag == FALSE)
    {    if(swInit() == FALSE)
        {
            //terninalPrintf("EpdInit ERROR (swInit false)!!\r\n");
            sysprintf("EpdInit ERROR (swInit false)!!\n");
            return FALSE;
        }
    }
    
    
		//setTotalPower(TRUE);
    WriteCmdCode(IT8951_TCON_SYS_RUN,2);
    if(GetIT8951SystemInfo()  == FALSE)
    {
        //terninalPrintf("EpdInit FALSE!!\r\n");
        initFlag = TRUE;
        return FALSE;
    }
    /*
    if((GPIO_ReadBit(GPIOJ, BIT0)) && (!GPIO_ReadBit(GPIOJ, BIT1)) )
    {
        if(readEPDswitch() == TRUE)
        {
            toggleEPDswitch(FALSE);
        }
    }
    */
    
    
    //IT8951WriteReg(I80CPCR, 0x0001);
    //IT8951WriteReg(UP0SR, 0x0001);
    
    
    //terninalPrintf("IT8951GetVCOM = %d \r\n",IT8951GetVCOM());
    //IT8951WriteReg(I80CPCR, 0x0001);
    //IT8951SetVCOM(50);
    
    //int tempVCOM;
    //tempVCOM = IT8951GetVCOM();
    //terninalPrintf("IT8951GetVCOM = %d \r\n",tempVCOM);
    //IT8951SetVCOM(tempVCOM);
    
    if(readMBtestFunc())
    {}
    else
    {
        EPDShowBGScreen(EPD_PICT_ALL_BLACK_INDEX, TRUE); 
        EPDShowBGScreen(EPD_PICT_ALL_WHITE_INDEX, TRUE); 
        EPDShowBGScreen(EPD_PICT_INDEX_INIT, TRUE);
    }
    
    //sysprintf("EpdInit OK!!\n"); 
    //terninalPrintf("EpdInit OK!!\r\n");
    initFlag = TRUE; 
    
    if(readMBtestFunc())
    {}
    else
    {
        char versionString[35]; 
        //sprintf(versionString,"Ver. %d.%02d.%02d  build:%d",MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, BUILD_VERSION_EPD);  
        sprintf(versionString,"Ver. %d.%02d.%02d  build:%d",MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, dataCode);
        EPDDrawString(TRUE,versionString,100,700);
    }
    
    if(testModeFlag)
    {
        //setTotalPower(FALSE);
    //    WriteCmdCode(IT8951_TCON_SLEEP,2); 
    }
    return TRUE;
}

BOOL EPDIT8951Test(void)
{
    WriteCmdCode(IT8951_TCON_SYS_RUN,2);
    return GetIT8951SystemInfo();
}


void EPDSetBacklight(BOOL flag)
{         
    sysprintf("-- EPDSetBacklight %d --\n", flag); 
    if(flag)    
        GPIO_SetBit(BACK_LIGHT_PIN_PORT, BACK_LIGHT_PORT_BIT); 
    else
        GPIO_ClrBit(BACK_LIGHT_PIN_PORT, BACK_LIGHT_PORT_BIT);
}

BOOL EPDGetBacklight(void)
{         
    //sysprintf("-- EPDGetBacklight --\n"); 
    if(GPIO_ReadBit(BACK_LIGHT_PIN_PORT, BACK_LIGHT_PORT_BIT))    
        return TRUE;
    else
        return FALSE;
}
void EPDReSetBacklightTimeout(time_t time)
{
    threadBackLightWaitTime = time;
    xSemaphoreGive(xBackLightSemaphore);
    EPDSetBacklight(TRUE);
}
void EPDSetReinitCallbackFunc(epdReinitCallbackFunc callback)
{
    pEpdReinitCallbackFunc = callback;
}
void EPDShowBGScreen(int id, BOOL reFreshFlag)
{
    #if(BUILD_DEBUG_VERSION)
    sysprintf("\r\n  ~~ EPD:Show ShowBGScreen(%d) !!!\n", id);
    #endif
    unsigned int buff[3*1+1];  
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    buff[0] = 1;  
    buff[1] = id;    
    buff[2] = 0;
    buff[3] = 0; 
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    LoadImages(buff, buff[0]);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);    
    xSemaphoreGive(xSemaphore);  
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDShowBGScreen>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
    if(id == EPD_PICT_ALL_WHITE_INDEX)
        EPDDrawDeviceId(FALSE); 
}


void EPDShowBGScreenEx(int id, BOOL reFreshFlag,TWord usW, TWord usH)
{
    #if(BUILD_DEBUG_VERSION)
    sysprintf("\r\n  ~~ EPD:Show ShowBGScreen(%d) !!!\n", id);
    #endif
    unsigned int buff[3*1+1];  
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    buff[0] = 1;  
    buff[1] = id;    
    buff[2] = 0;
    buff[3] = 0; 
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    LoadImages(buff, buff[0]);
    
    if(reFreshFlag)
        //DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);
          DisplayArea(0,0,usW,usH,1);
    xSemaphoreGive(xSemaphore);  
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDShowBGScreen>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
    if(id == EPD_PICT_ALL_WHITE_INDEX)
        EPDDrawDeviceId(FALSE); 
}



static positionDeviceIDInfo PositionDeviceIDInfo;

#define DEVICE_ID_FONT_NUM_INDEX         EPD_PICT_NUM_SMALL_2_INDEX
#define DEVICE_ID_FONT_WIDTH             (-23)

#define DEVICE_ID_X                      230
#define DEVICE_ID_Y                      40
void calculateDeviceIDPositionInfo(int32_t id)
{
    int i;
    //sysprintf("EPD:calculateDeviceIDPositionInfo: 0x%04x...\n", id);   
    PositionDeviceIDInfo.itemNum = DEVICE_ID_DIGITAL_NUM;
    for(i = 0; i<DEVICE_ID_DIGITAL_NUM; i++) 
    {
        uint8_t targetValue = DEVICE_ID_DIGITAL_NUM - 1 - i;
        //sysprintf(" --> check : %d, %d(%d) \n", id, pow10Ex(targetValue));
        if(id >= pow10Ex(targetValue))
        {                    
            //sysprintf(" --> [%d]: targetValue = %d, [%d:%d] (%d: %d) \n ", i, targetValue, 
            //                        id/pow10Ex(targetValue), (id/pow10Ex(targetValue + 1))*10, pow10Ex(targetValue), pow10Ex(targetValue + 1));
            PositionDeviceIDInfo.info[i].bmpId = id/pow10Ex(targetValue) - (id/pow10Ex(targetValue + 1))*10 + DEVICE_ID_FONT_NUM_INDEX;             
            
        }
        else
        {
            PositionDeviceIDInfo.info[i].bmpId = 0 + DEVICE_ID_FONT_NUM_INDEX;             
        }
            
        if(i == 0)
        {     
            PositionDeviceIDInfo.info[i].xPos = DEVICE_ID_X;
            PositionDeviceIDInfo.info[i].yPos = DEVICE_ID_Y;
        }
        else
        {
            PositionDeviceIDInfo.info[i].xPos = PositionDeviceIDInfo.info[i-1].xPos + (unsigned int)DEVICE_ID_FONT_WIDTH;
            PositionDeviceIDInfo.info[i].yPos = PositionDeviceIDInfo.info[i-1].yPos;
        }
        //sysprintf(" -!!-> [%d]: bmpId = %d, [%d:%d] \n ", i, PositionDeviceIDInfo.info[i].bmpId, PositionDeviceIDInfo.info[i].xPos, PositionDeviceIDInfo.info[i].yPos);
    }
}


#define ERROR_ID_FONT_NUM_INDEX         EPD_PICT_NUM_SMALL_INDEX
#define ERROR_ID_FONT_X_INDEX           EPD_PICT_X_SMALL_INDEX
#define ERROR_ID_FONT_WIDTH             48

#define ERROR_ID_X                      335
#define ERROR_ID_Y                      170
#define ERROR_ID_TOTAL_NUM_POSITION     4
void EPDShowErrorID(uint16_t id)
{
    sysprintf("EPD:ShowErrorID 0x%04x...\n", id);   
    uint8_t showNum[ERROR_ID_TOTAL_NUM_POSITION];
    uint8_t buffIndex = 0;;   
    int i;
    unsigned int buff[3*ERROR_ID_TOTAL_NUM_POSITION+1];
    sysprintf("EPD:ShowErrorID 0x%x, 0x%x, 0x%x, 0x%x...\n", (id>>12)&0xf, (id>>8)&0xf, (id>>4)&0xf, (id>>0)&0xf);
    //showNum[0] = ERROR_ID_FONT_NUM_INDEX;
    //showNum[1] = ERROR_ID_FONT_X_INDEX;
    showNum[0] = ((id>>12)&0xf) + ERROR_ID_FONT_NUM_INDEX;
    showNum[1] = ((id>>8)&0xf) + ERROR_ID_FONT_NUM_INDEX;
    showNum[2] = ((id>>4)&0xf) + ERROR_ID_FONT_NUM_INDEX;
    showNum[3] = ((id>>0)&0xf) + ERROR_ID_FONT_NUM_INDEX;
    buff[0] = ERROR_ID_TOTAL_NUM_POSITION;   
    
    for(i = 0; i<ERROR_ID_TOTAL_NUM_POSITION; i++) 
    {
            buff[++buffIndex] = showNum[i];
            buff[++buffIndex] = ERROR_ID_X - i*ERROR_ID_FONT_WIDTH;         
            buff[++buffIndex] = ERROR_ID_Y;
    }
    #if(0)
    for(i = 0; i<buff[0]*3+1; i++) 
    {
        sysprintf("EPD:buff[%02d]: 0x%04x(%d).. \n", i, buff[i], buff[i]);  
    }
    #endif
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    LoadImages(buff, buff[0]);    
    DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);      
    xSemaphoreGive(xSemaphore);   
}

#define EXPIRED_LEFT_X                              473
#define EXPIRED_LEFT_Y                              350

#define EXPIRED_RIGHT_X                             93

#define EXPIRED_RIGHT_Y                             EXPIRED_LEFT_Y
#define EXPIRED_TOTAL_NUM_POSITION                   2
void ShowExpired(uint8_t id, BOOL leftShow, BOOL rightShow, BOOL reFreshFlag)
{
    
//    uint8_t showNum[EXPIRED_TOTAL_NUM_POSITION];
    #if(BUILD_DEBUG_VERSION)
    sysprintf("EPD:ShowExpired [left:%02d] : [right:%02d] ...\n", leftId, rightId );
    #endif

//    int i;
    unsigned int buff[3*EXPIRED_TOTAL_NUM_POSITION+1];
    uint8_t buffIndex = 0;
    buff[0] = 0;

    if(!leftShow && !rightShow)
    {
        sysprintf("EPD:ShowExpired ignore!!!!\r\n");
        return;
    }
    if(leftShow)
    {
        buff[++buffIndex] = id;
        buff[++buffIndex] = EXPIRED_LEFT_X;  
        buff[++buffIndex] = EXPIRED_LEFT_Y; 
        buff[0] = buff[0] + 1;
    }
    
    if(rightShow)
    {
        buff[++buffIndex] = id;
        buff[++buffIndex] = EXPIRED_RIGHT_X;  
        buff[++buffIndex] = EXPIRED_RIGHT_Y; 
        buff[0] = buff[0] + 1;
    }

    #if(BUILD_DEBUG_VERSION)
    sysprintf("EPD:ShowExpired [top %02d:%02d] - [down %02d:%02d]...\n", leftVol1, leftVol2, rightVol1, rightVol2);
    #endif
    #if(0)
    for(i = 0; i<buff[0]*3+1; i++) 
    {
        sysprintf("EPD:buff[%02d]: 0x%04x(%d).. \n", i, buff[i], buff[i]);  
    }
    #endif
    if(buff[0] == 0)
    {
        sysprintf("EPD:ShowExpired ignore 2!!!!\r\n");
        return;
    }
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    LoadImages(buff, buff[0]);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    xSemaphoreGive(xSemaphore);  
}


#define CURRENT_TIME_X                      520
#define CURRENT_TIME_Y                      40

#define CURRENT_DATE_X                      750
#define CURRENT_DATE_Y                      40
void UpdateClock(BOOL reFreshFlag, BOOL checkFlag)
{   
    int counter = 0;
    BOOL timeReval, dateReval;
    RTC_TIME_DATA_T pt;
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    while(counter<5)
    {
        if(E_RTC_SUCCESS == RTC_Read(RTC_CURRENT_TIME, &pt))
        {
            #if(BUILD_DEBUG_VERSION)
            sysprintf("RTC_CURRENT_TIME: [%04d/%02d/%02d %02d:%02d:%02d (%d)  u8cClockDisplay = %d, u8cAmPm =%d]\r\n",
                                                    pt.u32Year, pt.u32cMonth, pt.u32cDay, 
                                                    pt.u32cHour, pt.u32cMinute, pt.u32cSecond, pt.u32cDayOfWeek, pt.u8cClockDisplay, pt.u8cAmPm); 
            #endif        
            timeReval = ShowTime(CURRENT_TIME_X, CURRENT_TIME_Y, EPD_FONT_TYPE_SMALL_2, pt.u32cHour, pt.u32cMinute, checkFlag);
            dateReval = ShowDate(CURRENT_DATE_X, CURRENT_DATE_Y, EPD_FONT_TYPE_SMALL_2, pt.u32Year, pt.u32cMonth, pt.u32cDay, checkFlag);  
            
            if(timeReval || dateReval || reFreshFlag)
            {
                xSemaphoreTake(xSemaphore, portMAX_DELAY);  
                DisplayArea(0,0,800,600, 1); 
                xSemaphoreGive(xSemaphore);  
            }

            //epd backlite
            #if(0)
            #warning just for test
            if((pt.u32cHour > 6) && (pt.u32cHour < 18))
                EPDSetBacklight(FALSE);
            else
                EPDSetBacklight(TRUE);
            #endif
            break;
        }  
        else
        {
            sysprintf(" [ERROR EPD] <UpdateClock>  ignore, RTC_Read error[counter = %d].\n", counter); 
           BuzzerPlay(100, 500, counter, TRUE);
            counter++;
        }
    }
    if(counter>=5)
    {
       BuzzerPlay(500, 500, 1, TRUE);
       BuzzerPlay(100, 100, 1, TRUE);
    }
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <UpdateClock>  [%d].\n", xTaskGetTickCount() - tickLocalStart);   
    #endif    
}



BOOL calculatePreparePositionInfo(uint8_t meterPosition, uint8_t carEnableNumber)
{
    uint8_t carIndex = 0;
    int i;
    int targetWidth = 0;
    #if(SHOW_POSITION_DEBUG)
    sysprintf("calculateBannerPositionInfo: meterPosition = %d, carEnableNumber = %d\r\n", meterPosition, carEnableNumber); 
    #endif
    if((carEnableNumber > CAR_ITEM_MAX_NUM) || (carEnableNumber < 1))
    {
        sysprintf("calculateBannerPositionInfo fail: carEnableNumber = %d\r\n", carEnableNumber); 
        return FALSE;
    }
    if(meterPosition > carEnableNumber)
    {
        sysprintf("calculateBannerPositionInfo fail: meterPosition = %d(%d)\r\n", meterPosition, carEnableNumber); 
        return FALSE;
    }
    mBannerPositionInfo.carEnableNum = carEnableNumber + 1;  
    for(i = 0; i<mBannerPositionInfo.carEnableNum; i++) 
    {
        
        if(i == 0)
        {
            mBannerPositionInfo.info[i].xPos = BANNER_START_X_POSITION; 
            mBannerPositionInfo.info[i].yPos = BANNER_START_Y_POSITION; 
        }
        else
        {
            mBannerPositionInfo.info[i].xPos = mBannerPositionInfo.info[i-1].xPos + targetWidth; 
            mBannerPositionInfo.info[i].yPos = BANNER_START_Y_POSITION;
            
        }
        if(meterPosition == i)
        {
            mBannerPositionInfo.info[i].bmpId = EPD_PICT_METER_INDEX;
            if(i == 0)
            {//meter 在第一位 就往前移
                mBannerPositionInfo.info[i].xPos = mBannerPositionInfo.info[i].xPos + 50; 
            }
        }
        else
        {
            mBannerPositionInfo.info[i].bmpId = EPD_PICT_CAR1_INDEX + carIndex;
            carIndex++;
        }
        
        if(meterPosition == (i+1))
        {
            
            targetWidth = BANNER_METER_WIDTH_POSITION;
        }
        else
        {
            
            targetWidth = BANNER_CARD_WIDTH_POSITION;
        }
        #if(SHOW_POSITION_DEBUG)
        sysprintf("BannerPositionInfo[%d]: [%d: (%03d, %03d)]\r\n", i, mBannerPositionInfo.info[i].bmpId, mBannerPositionInfo.info[i].xPos, mBannerPositionInfo.info[i].yPos);
        #endif        
    }
    
    if(calculateContainPositionInfo(carEnableNumber))
    {
        if(calculateContainNumberPositionInfo(carEnableNumber))
        {
            if(calculateDepositTimePositionInfo(carEnableNumber))
            {
                //if(calculateAllItemInfo())
                {
                    return calculateBannerLinePositionInfo();
                }
            }
        }
    }  
    return FALSE;
}

void EPDDrawDeviceId(BOOL reFreshFlag)
{
    unsigned int* buff = (unsigned int*)&PositionDeviceIDInfo;   
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawDeviceId>  Enter (PositionDeviceIDInfo.itemNum = %d).\n", PositionDeviceIDInfo.itemNum); 
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    //calculateDeviceIDPositionInfo(GetMeterPara()->epmid);
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, PositionDeviceIDInfo.itemNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawDeviceId>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
    
}

void EPDDrawBanner(BOOL reFreshFlag)
{
    unsigned int* buff = (unsigned int*)&mBannerPositionInfo;
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, mBannerPositionInfo.carEnableNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);  
}

void EPDDrawBannerLine(BOOL reFreshFlag)
{
    unsigned int* buff = (unsigned int*)&mBannerLinePositionInfo;
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, mBannerLinePositionInfo.itemNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);  
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawBannerLine>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
}



void SetDepositTimeBmpIdInfo(uint8_t index, time_t time)
{
    timeIdInfo idInfo;
    uint8_t targetIndex = DEPOSIT_TIME_DIGITAL_NUM*index;
    if(getIDFromTime(time, &idInfo))
    {    
        mDepositTimeAllPositionInfo.info[targetIndex].bmpId = idInfo.id[0];
        mDepositTimeAllPositionInfo.info[targetIndex+1].bmpId = idInfo.id[1];
        mDepositTimeAllPositionInfo.info[targetIndex+2].bmpId = idInfo.id[3];
        mDepositTimeAllPositionInfo.info[targetIndex+3].bmpId = idInfo.id[4];                    
    }
}

////////////////
/*
BOOL calculateAllItemInfo(void)
{
    uint8_t targetIndex =0;
    PositionAllItemInfo.itemNum = 1 + mBannerPositionInfo.carEnableNum + mContainPositionInfo.boardEnableNum + mContainNumberPositionInfo.boardEnableNum;
    
    memcpy(&PositionAllItemInfo.info[targetIndex], &upperLinePositionInfo, sizeof(positionInfo));
    targetIndex = targetIndex + 1;
    memcpy(&PositionAllItemInfo.info[targetIndex], mBannerPositionInfo.info, sizeof(positionInfo) * mBannerPositionInfo.carEnableNum);
    targetIndex = targetIndex + mBannerPositionInfo.carEnableNum;
    memcpy(&PositionAllItemInfo.info[targetIndex], mContainPositionInfo.info, sizeof(positionInfo) * mContainPositionInfo.boardEnableNum);
    targetIndex = targetIndex + mContainPositionInfo.boardEnableNum;
    memcpy(&PositionAllItemInfo.info[targetIndex], mContainNumberPositionInfo.info, sizeof(positionInfo) * mContainNumberPositionInfo.boardEnableNum);
    return TRUE;
}

void EPDDrawContain(BOOL reFreshFlag)
{
    unsigned int* buff = (unsigned int*)&mContainPositionInfo;
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, mContainPositionInfo.boardEnableNum);
    
    buff = (unsigned int*)&mContainNumberPositionInfo;
    LoadImages(buff, mContainNumberPositionInfo.boardEnableNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore); 
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawBoard>  [%d].\n", xTaskGetTickCount() - tickLocalStart);    
    #endif    
}
*/
void EPDDrawContainBG(BOOL reFreshFlag)
{
    uint8_t id;
    switch(mContainPositionInfo.boardEnableNum)
    {
        case 1:
//            id = EPD_PICT_CONTAIN_BG_2_1_INDEX;
            break;
        case 2:
            id = EPD_PICT_CONTAIN_BG_2_INDEX;
            break;
        case 3:
//            id = EPD_PICT_CONTAIN_BG_4_3_INDEX;
            break;
        case 4:
//            id = EPD_PICT_CONTAIN_BG_4_INDEX;
            break;
        case 5:
//            id = EPD_PICT_CONTAIN_BG_6_5_INDEX;
            break;
        case 6:
//            id = EPD_PICT_CONTAIN_BG_6_INDEX;
            break;
        default:
            return;
    }
    EPDDrawContainByID(reFreshFlag, id);   
}

void EPDDrawContainByID(BOOL reFreshFlag, uint8_t id)
{
    positionSingleInfo mPositionSingleInfoTmp;
    
    unsigned int* buff = (unsigned int*)&mPositionSingleInfoTmp;   
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    mPositionSingleInfoTmp.itemNum = 1;
    
    mPositionSingleInfoTmp.info.bmpId = id;
    mPositionSingleInfoTmp.info.xPos = 45;
    mPositionSingleInfoTmp.info.yPos = 74;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    LoadImages(buff, mPositionSingleInfoTmp.itemNum);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawContainByID>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif    
}

void EPDDrawContainByIDPos(BOOL reFreshFlag, uint8_t id,unsigned int x_Pos,unsigned int y_Pos)
{
    if(!initFlag){
        return;
    }
    positionSingleInfo mPositionSingleInfoTmp;
    unsigned int* buff = (unsigned int*)&mPositionSingleInfoTmp;   
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    mPositionSingleInfoTmp.itemNum = 1;
    
    mPositionSingleInfoTmp.info.bmpId = id;
    mPositionSingleInfoTmp.info.xPos = x_Pos;
    mPositionSingleInfoTmp.info.yPos = y_Pos;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    LoadImages(buff, mPositionSingleInfoTmp.itemNum);
    if(reFreshFlag)
    {
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);
    }
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawContainByID>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif    
}
void EPDDrawMulti(BOOL reFreshFlag, uint8_t id,unsigned int x_Pos,unsigned int y_Pos)
{
    positionMultiInfo mPositionMultiInfoTmp;
    static int count_i=0;
    unsigned int* buff = (unsigned int*)&mPositionMultiInfoTmp;    
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    mPositionMultiInfoTmp.itemNum = count_i+1;
    
    mPositionMultiInfoTmp.info[count_i].bmpId = id;
    mPositionMultiInfoTmp.info[count_i].xPos = x_Pos;
    mPositionMultiInfoTmp.info[count_i].yPos = y_Pos;
    count_i++;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    if(reFreshFlag)
    {
        LoadImages(buff, mPositionMultiInfoTmp.itemNum);
        count_i=0;
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);
    }
    else if(count_i==(MAX_LOAD_IMG-1))
    {
        LoadImages(buff, mPositionMultiInfoTmp.itemNum);
        count_i=0;
    }
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawS>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
}

#define Y_HEIGHT  44
int EPDDrawString(BOOL refresh,char* string,unsigned int x_Pos,unsigned int y_Pos)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    
    if(!initFlag)
    {
        return FALSE;
    }
    if(LCDWaitForReady() == FALSE) 
        return FALSE;
    static positionStringInfo mPositionMsgInfoTmp={0};
    unsigned int* buff = (unsigned int*)&mPositionMsgInfoTmp;
    unsigned int stringLength,buffIndex=0;
    unsigned int nowrow=0,nowcursor=0;
    
    stringLength = strlen(string);
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart= xTaskGetTickCount();
    #endif
    
    for(int i=0;i<stringLength;i++){
        //space//
        if(*(string+i)==' ' || *(string+i)==0x5F)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_EMPTY;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //A to Z//
        else if(*(string+i)>='A'&&*(string+i)<='Z')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId =(*(string+i)-'A') + EPD_PICT_UPPER_A;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=32;
            buffIndex++;
        }
        //a to z//
        else if(*(string+i)>='a'&&*(string+i)<='z')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId =(*(string+i)-'a') + EPD_PICT_LOWER_A;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //0 to 9//
        else if(*(string+i)>='0'&&*(string+i)<='9')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId =(*(string+i)-'0') + EPD_PICT_NUM_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  :  //
        else if(*(string+i)==':')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_COLON_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos+ (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  -  //
        else if(*(string+i)=='-')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_LINE_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);

            nowcursor+=28;
            buffIndex++;
        }
        else if(*(string+i)=='.')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_POINT_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos+ (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  \  //
        else if(*(string+i)=='\\')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_SLASH_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos+ (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        
        //Left Key//
        else if(*(string+i)==KEYPAD_ONE_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_ONE;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Right Key//
        else if(*(string+i)==KEYPAD_TWO_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_TWO;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Plus Key//
        else if(*(string+i)==KEYPAD_THREE_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_THREE;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        
        //Real Plus Key//
        else if((uint8_t)*(string+i)==KEYPAD_REAL_PLUS_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_PLUS;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        
        //Minus Key//
        else if(*(string+i)==KEYPAD_FOUR_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_FOUR;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Confirm Key//
        else if(*(string+i)==KEYPAD_FIVE_CHAR)
        {
					//mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_EPD_PICT_KEY_CONFIRM;
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_FIVE;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Card Key//
        else if(*(string+i)==KEYPAD_SIX_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_SIX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  #   //
        else if(*(string+i)=='#')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_0X23_24;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  $   //
        else if(*(string+i)=='$')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_LINE_SMALL_2_I_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            //mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        /*
        //  ~   //
        else if(*(string+i)=='~')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_LINE_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        } 
        */
        //ENTER//
        else if(*(string+i)=='\n')
        {
            nowrow++;
            nowcursor=0;
        }
        if(buffIndex==(MAX_LOAD_IMG-1)||i==stringLength-1)
        {
            mPositionMsgInfoTmp.itemNum = buffIndex;
            LoadImages(buff, mPositionMsgInfoTmp.itemNum);
            buffIndex=0;
        }
        //???????????????? WTF
        //xSemaphoreGive(xSemaphore);
    }
    nowrow++;
    mPositionMsgInfoTmp.itemNum=buffIndex;
    //???????????????? WTF
    //xSemaphoreTake(xSemaphore, portMAX_DELAY);
    //LoadImages(buff, mPositionMsgInfoTmp.itemNum);
    if(refresh)
    {
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);
    }
    xSemaphoreGive(xSemaphore);
    
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawContainByID>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
    return nowcursor+x_Pos;
}

void EPDDrawStringMax(BOOL refresh,char* string,unsigned int x_Pos,unsigned int y_Pos,BOOL load)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    
    if(!initFlag)
    {
        return;
    }
    static positionStringInfo mPositionMsgInfoTmp={0};
    unsigned int* buff = (unsigned int*)&mPositionMsgInfoTmp;
    static unsigned int buffIndex=0;
    unsigned int nowrow = 0,nowcursor = 0;
    unsigned int strLen = strlen(string);
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart= xTaskGetTickCount();
    #endif
    
    for(int i=0;i<strLen;i++)
    {
        //space//
        if(*(string+i)==' ')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_EMPTY;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //A to Z//
        else if(*(string+i)>='A'&&*(string+i)<='Z')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId=(*(string+i)-'A') + EPD_PICT_UPPER_A;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=32;
            buffIndex++;
        }
        //a to z//
        else if(*(string+i)>='a'&&*(string+i)<='z')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId=(*(string+i)-'a') + EPD_PICT_LOWER_A;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //0 to 9//
        else if(*(string+i)>='0'&&*(string+i)<='9')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId=(*(string+i)-'0') + EPD_PICT_NUM_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  :  //
        else if(*(string+i)==':')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_COLON_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos+ (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  -  //
        else if(*(string+i)=='-')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_LINE_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        else if(*(string+i)=='.')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_POINT_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos+ (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  \  //
        else if(*(string+i)=='\\')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_SLASH_SMALL_2_INDEX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos+ (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        
        //Left Key//
        else if(*(string+i)==KEYPAD_ONE_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_KEY_ONE;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Right Key//
        else if(*(string+i)==KEYPAD_TWO_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_KEY_TWO;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Plus Key//
        else if(*(string+i)==KEYPAD_THREE_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_KEY_THREE;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Minus Key//
        else if(*(string+i)==KEYPAD_FOUR_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_KEY_FOUR;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Confirm Key//
        else if(*(string+i)==KEYPAD_FIVE_CHAR)
        {
			//mPositionMsgInfoTmp.info[buffIndex].bmpId = EPD_PICT_KEY_EPD_PICT_KEY_CONFIRM;
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_KEY_FIVE;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //Card Key//
        else if(*(string+i)==KEYPAD_SIX_CHAR)
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_KEY_SIX;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //  #   //
        else if(*(string+i)=='#')
        {
            mPositionMsgInfoTmp.info[buffIndex].bmpId= EPD_PICT_0X23_24;
            mPositionMsgInfoTmp.info[buffIndex].xPos = nowcursor+x_Pos;
            mPositionMsgInfoTmp.info[buffIndex].yPos = y_Pos + 10 + (nowrow*50);
            nowcursor+=28;
            buffIndex++;
        }
        //ENTER//
        else if(*(string+i)=='\n')
        {
            nowrow++;
            nowcursor=0;
        }
        if(buffIndex==(MAX_LOAD_IMG-1))
        {
            mPositionMsgInfoTmp.itemNum = buffIndex;
            //LoadImages(buff, mPositionMsgInfoTmp.itemNum);
            EPDExitSleep();
            WriteCmdCode(0x0099,2);
            WriteData(buff[0],2);
            { 
                if(LCDWaitForReadyEx() == FALSE) 
                    return;
                //while(GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT)==0);
                //cs set low
                pSpiInterface->activeCSFunc(TRUE);

                //send write data preamble.
                pSpiInterface->writeFunc(0, 0x00);
                pSpiInterface->writeFunc(0, 0x00);
                
                for(int ii=1; ii<((mPositionMsgInfoTmp.itemNum)*3+1); ii++)
                {
                    if(LCDWaitForReadyEx() == FALSE)
                        return;
                    //while(GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT)==0);
                    
                    /*1*///WriteData(buff[i],2);
                    //Send Data
                    pSpiInterface->writeFunc(0, (char)(buff[ii]>>8));
                    pSpiInterface->writeFunc(0, (char)buff[ii]);
                }
                //cs set high.
                pSpiInterface->activeCSFunc(FALSE);
            }
            buffIndex = 0;
        }
    }
    if(buffIndex>0 && load)
    {
        mPositionMsgInfoTmp.itemNum = buffIndex;
        //LoadImages(buff, mPositionMsgInfoTmp.itemNum);
        EPDExitSleep();
        WriteCmdCode(0x0099,2);
        WriteData(buff[0],2);
        { 
            if(LCDWaitForReadyEx() == FALSE) 
                return;
            //while(GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT)==0);
            //cs set low
            pSpiInterface->activeCSFunc(TRUE);

            //send write data preamble.
            pSpiInterface->writeFunc(0, 0x00);
            pSpiInterface->writeFunc(0, 0x00);
            
            for(int ii=1; ii<((mPositionMsgInfoTmp.itemNum)*3+1); ii++)
            {
                if(LCDWaitForReadyEx() == FALSE)
                    return;
                //while(GPIO_ReadBit(READY_PIN_PORT,READY_PORT_BIT)==0);
                
                /*1*///WriteData(buff[i],2);
                //Send Data
                pSpiInterface->writeFunc(0, (char)(buff[ii]>>8));
                pSpiInterface->writeFunc(0, (char)buff[ii]);
            }
            //cs set high.
            pSpiInterface->activeCSFunc(FALSE);
        }
        
        buffIndex = 0;
    }
    mPositionMsgInfoTmp.itemNum=buffIndex;
    if(refresh)
    {
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);
    }
    xSemaphoreGive(xSemaphore);
    
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawContainByID>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
}

static uint8_t getCarPositionInfoIndex(uint8_t selectCarId)
{
    int i;
    for(i = 0; i<mBannerPositionInfo.carEnableNum; i++) 
    { 
        if(mBannerPositionInfo.info[i].bmpId != EPD_PICT_METER_INDEX)
        {
            selectCarId--;
            if(selectCarId == 0)
                return i;
        }  
    }
    return 0xff;
}
void EPDDrawCar(BOOL reFreshFlag, uint8_t selectCarId, BOOL inverseFlag)
{
    uint8_t cardPositionInfoIndex;
    positionSingleInfo mPositionSingleInfo;
    unsigned int* buff = (unsigned int*)&mPositionSingleInfo;
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    mPositionSingleInfo.itemNum = 1;
    if(inverseFlag)
        mPositionSingleInfo.info.bmpId = selectCarId + EPD_PICT_CAR1_I_INDEX - 1;
    else
        mPositionSingleInfo.info.bmpId = selectCarId + EPD_PICT_CAR1_INDEX - 1;
    
    cardPositionInfoIndex = getCarPositionInfoIndex(selectCarId);
    
    mPositionSingleInfo.info.xPos = mBannerPositionInfo.info[cardPositionInfoIndex].xPos;
    mPositionSingleInfo.info.yPos = mBannerPositionInfo.info[cardPositionInfoIndex].yPos;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, mPositionSingleInfo.itemNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore); 
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawCar>  [%d].\n", xTaskGetTickCount() - tickLocalStart);  
    #endif    
}

/*
void EPDDrawBoard(BOOL reFreshFlag, uint8_t selectCarId, BOOL inverseFlag)
{
    positionSingleInfo mPositionSingleInfo, mPositionSingleInfo2;
    unsigned int* buff = (unsigned int*)&mPositionSingleInfo;
    unsigned int* buff2 = (unsigned int*)&mPositionSingleInfo2;
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    //board
    mPositionSingleInfo.itemNum = 1;
    if(inverseFlag)
    {
        //mPositionSingleInfo.info.bmpId = EPD_PICT_BOARD_I_INDEX;
        if(selectCarId%2 == 1)
        {
            mPositionSingleInfo.info.bmpId = EPD_PICT_BOARD_I_L_INDEX;
        }
        else
        {
            mPositionSingleInfo.info.bmpId = EPD_PICT_BOARD_I_R_INDEX;
        }
        
    }
    else
    {
        //mPositionSingleInfo.info.bmpId = EPD_PICT_BOARD_INDEX;
        if(selectCarId%2 == 1)
        {
            mPositionSingleInfo.info.bmpId = EPD_PICT_BOARD_L_INDEX;
        }
        else
        {
            mPositionSingleInfo.info.bmpId = EPD_PICT_BOARD_R_INDEX;
        }
    }
    
    mPositionSingleInfo.info.xPos = mContainPositionInfo.info[selectCarId-1].xPos;
    mPositionSingleInfo.info.yPos = mContainPositionInfo.info[selectCarId-1].yPos;
    //number
    mPositionSingleInfo2.itemNum = 1;
    if(inverseFlag)
    {
        mPositionSingleInfo2.info.bmpId = EPD_PICT_NUM_SMALL_2_I_INDEX + selectCarId;
    }
    else
    {
        mPositionSingleInfo2.info.bmpId = EPD_PICT_NUM_SMALL_2_INDEX + selectCarId;
    }
    
    mPositionSingleInfo2.info.xPos = mContainNumberPositionInfo.info[selectCarId-1].xPos;
    mPositionSingleInfo2.info.yPos = mContainNumberPositionInfo.info[selectCarId-1].yPos;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, mPositionSingleInfo.itemNum);
    LoadImages(buff2, mPositionSingleInfo2.itemNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawBoard>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
}
*/
void EPDDrawItem(BOOL reFreshFlag, uint8_t oriSelectCarId, uint8_t selectCarId)
{
    positionItemInfo PositionItemInfo; 
    unsigned int* buff = (unsigned int*)&PositionItemInfo;   
    uint8_t targetCarId;
    uint8_t buffIndex;
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    PositionItemInfo.itemNum = 0;
    //
    buffIndex = 0;
    if(selectCarId != 0)
    {
        targetCarId = selectCarId;
        PositionItemInfo.info[buffIndex].bmpId = targetCarId + EPD_PICT_CAR1_I_INDEX - 1;    
        PositionItemInfo.info[buffIndex].xPos = mBannerPositionInfo.info[getCarPositionInfoIndex(targetCarId)].xPos;
        PositionItemInfo.info[buffIndex++].yPos = mBannerPositionInfo.info[getCarPositionInfoIndex(targetCarId)].yPos;
        
        
        //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_I_INDEX; 

        if(selectCarId%2 == 1)
        {
            
            switch(mContainPositionInfo.boardEnableNum)
            {
                case 1:
                case 2:
                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_2_I_L_INDEX;
                    break;
                case 3:
                case 4:
//                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_4_I_L_INDEX;
                    break;
                default:
//                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_I_L_INDEX;
                    break;
            }    
            //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_I_L_INDEX;
        }
        else
        {
            switch(mContainPositionInfo.boardEnableNum)
            {
                case 1:
                case 2:
                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_2_I_R_INDEX;
                    break;
                case 3:
                case 4:
//                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_4_I_R_INDEX;
                    break;
                default:
//                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_I_R_INDEX;
                    break;
            }    
            //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_I_R_INDEX;
        }
        
        PositionItemInfo.info[buffIndex].xPos = mContainPositionInfo.info[targetCarId-1].xPos;
        PositionItemInfo.info[buffIndex++].yPos = mContainPositionInfo.info[targetCarId-1].yPos;
        PositionItemInfo.info[buffIndex].bmpId = targetCarId + EPD_PICT_NUM_SMALL_2_I_INDEX;    
        PositionItemInfo.info[buffIndex].xPos = mContainNumberPositionInfo.info[targetCarId-1].xPos;
        PositionItemInfo.info[buffIndex++].yPos = mContainNumberPositionInfo.info[targetCarId-1].yPos;
        PositionItemInfo.itemNum = PositionItemInfo.itemNum + 3;
    }
    
    if(oriSelectCarId != 0)
    {
        targetCarId = oriSelectCarId;
        PositionItemInfo.info[buffIndex].bmpId = targetCarId + EPD_PICT_CAR1_INDEX - 1;    
        PositionItemInfo.info[buffIndex].xPos = mBannerPositionInfo.info[getCarPositionInfoIndex(targetCarId)].xPos;
        PositionItemInfo.info[buffIndex++].yPos = mBannerPositionInfo.info[getCarPositionInfoIndex(targetCarId)].yPos;
        
        //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_INDEX;  
        if(oriSelectCarId%2 == 1)
        {
            switch(mContainPositionInfo.boardEnableNum)
            {
                case 1:
                case 2:
                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_2_L_INDEX;
                    break;
                case 3:
                case 4:
//                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_4_L_INDEX;
                    break;
                default:
//                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_L_INDEX;
                    break;
            }    
            //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_L_INDEX;
        }
        else
        {
            switch(mContainPositionInfo.boardEnableNum)
            {
                case 1:
                case 2:
                    PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_2_R_INDEX;
                    break;
                case 3:
                case 4:
                    //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_4_R_INDEX;
                    break;
                default:
                    //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_R_INDEX;
                    break;
            }    
            //PositionItemInfo.info[buffIndex].bmpId = EPD_PICT_BOARD_R_INDEX;
        }        
        
        PositionItemInfo.info[buffIndex].xPos = mContainPositionInfo.info[targetCarId-1].xPos;
        PositionItemInfo.info[buffIndex++].yPos = mContainPositionInfo.info[targetCarId-1].yPos;
        PositionItemInfo.info[buffIndex].bmpId = targetCarId + EPD_PICT_NUM_SMALL_2_INDEX;    
        PositionItemInfo.info[buffIndex].xPos = mContainNumberPositionInfo.info[targetCarId-1].xPos;
        PositionItemInfo.info[buffIndex++].yPos = mContainNumberPositionInfo.info[targetCarId-1].yPos;
        PositionItemInfo.itemNum = PositionItemInfo.itemNum + 3;
    } 
    if(PositionItemInfo.itemNum == 0)
    {
        #if(SHOW_TIME_DEBUG)
        sysprintf(" [INFO EPD] <EPDDrawItem>  ignore [%d].\n", xTaskGetTickCount() - tickLocalStart); 
        #endif
        return;
    }
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, PositionItemInfo.itemNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawItem>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
}
/*
void EPDDrawAllItem(BOOL reFreshFlag)
{
    unsigned int* buff = (unsigned int*)&PositionAllItemInfo;  
    #if(SHOW_TIME_DEBUG)    
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, PositionAllItemInfo.itemNum);    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawItem>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
}
*/

void MeterUpdateExpiredTitle(uint8_t selectItemId);
void EPDDrawDepositTime(BOOL reFreshFlag, uint8_t oriSelectItemId, uint8_t selectItemId)
{
    depositTimePositionInfo DepositTimePositionInfo; 
    unsigned int* buff = (unsigned int*)&DepositTimePositionInfo;   
    uint8_t targetCarId;
    uint8_t buffIndex = 0;
    int i;
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    DepositTimePositionInfo.depositTimeEnableNum = 0;
    //
    if(selectItemId != 0)
    {
        targetCarId = DEPOSIT_TIME_DIGITAL_NUM * (selectItemId-1);
        memcpy(&DepositTimePositionInfo.info[buffIndex], &mDepositTimeAllPositionInfo.info[targetCarId], sizeof(positionInfo)*DEPOSIT_TIME_DIGITAL_NUM);
        for(i = 0; i<DEPOSIT_TIME_DIGITAL_NUM; i++)
        {
            DepositTimePositionInfo.info[i].bmpId = DepositTimePositionInfo.info[i].bmpId + EPD_PICT_NUM_BIG_I_INDEX - EPD_PICT_NUM_BIG_INDEX;
        }
        buffIndex = buffIndex + DEPOSIT_TIME_DIGITAL_NUM;
        DepositTimePositionInfo.depositTimeEnableNum = DepositTimePositionInfo.depositTimeEnableNum + DEPOSIT_TIME_DIGITAL_NUM;
    }
    
    if(oriSelectItemId != 0)
    {
        targetCarId = DEPOSIT_TIME_DIGITAL_NUM * (oriSelectItemId-1);
        memcpy(&DepositTimePositionInfo.info[buffIndex], &mDepositTimeAllPositionInfo.info[targetCarId], sizeof(positionInfo)*DEPOSIT_TIME_DIGITAL_NUM);
        DepositTimePositionInfo.depositTimeEnableNum = DepositTimePositionInfo.depositTimeEnableNum + DEPOSIT_TIME_DIGITAL_NUM;
    } 
    if(DepositTimePositionInfo.depositTimeEnableNum == 0)
    {
        #if(SHOW_TIME_DEBUG)
        sysprintf(" [INFO EPD] <EPDDrawDepositTime> ignore  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
        #endif
        return;
    }
    
    //
    #warning
    MeterUpdateExpiredTitle(selectItemId);
    //
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, DepositTimePositionInfo.depositTimeEnableNum);
    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawDepositTime>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
    
}

void EPDDrawAllDepositTime(BOOL reFreshFlag)
{
    unsigned int* buff = (unsigned int*)&mDepositTimeAllPositionInfo;   
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    
    #warning
    MeterUpdateExpiredTitle(0);  
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages(buff, mDepositTimeAllPositionInfo.depositTimeEnableNum);    
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawAllDepositTime>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif 
    
     
}

void EPDDrawAllScreen(BOOL reFreshFlag)
{
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    xSemaphoreTake(xSemaphore, portMAX_DELAY);  
    LoadImages((unsigned int*)&PositionAllItemInfo, PositionAllItemInfo.itemNum); //car line board board-num 
    LoadImages((unsigned int*)&mDepositTimeAllPositionInfo, mDepositTimeAllPositionInfo.depositTimeEnableNum);  //DepositTimeAll
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    
    xSemaphoreGive(xSemaphore);
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawAllScreen>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
    
}

void EPDDrawPleaseWait(BOOL reFreshFlag, BOOL clearFlag)
{
    positionSingleInfo mPositionSingleInfoTmp;
    
    unsigned int* buff = (unsigned int*)&mPositionSingleInfoTmp;   
    mPositionSingleInfoTmp.itemNum = 1;
    if(clearFlag)
//        mPositionSingleInfoTmp.info.bmpId = EPD_PICT_PLEASE_WAIT_CLEAR_INDEX;
//    else
//        mPositionSingleInfoTmp.info.bmpId = EPD_PICT_PLEASE_WAIT_INDEX;
    
    mPositionSingleInfoTmp.info.xPos = 130;
    mPositionSingleInfoTmp.info.yPos = 160;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    LoadImages(buff, mPositionSingleInfoTmp.itemNum);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    xSemaphoreGive(xSemaphore);  
}


void EPDDrawContainSelTime(BOOL reFreshFlag)
{
    positionSingleInfo mPositionSingleInfoTmp;
    
    unsigned int* buff = (unsigned int*)&mPositionSingleInfoTmp;   
    mPositionSingleInfoTmp.itemNum = 1;
    mPositionSingleInfoTmp.info.bmpId = EPD_PICT_CONTAIN_SELECT_TIME_INDEX;
    
    mPositionSingleInfoTmp.info.xPos = 47;
    mPositionSingleInfoTmp.info.yPos = 74;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    LoadImages(buff, mPositionSingleInfoTmp.itemNum);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    xSemaphoreGive(xSemaphore);  
}

void EPDDrawContainClear(BOOL reFreshFlag)
{
    positionSingleInfo mPositionSingleInfoTmp;
    
    unsigned int* buff = (unsigned int*)&mPositionSingleInfoTmp;   
    #if(SHOW_TIME_DEBUG)
    TickType_t tickLocalStart = xTaskGetTickCount();
    #endif
    mPositionSingleInfoTmp.itemNum = 1;
    mPositionSingleInfoTmp.info.bmpId = EPD_PICT_CONTAIN_CLEAR_INDEX;
    
    mPositionSingleInfoTmp.info.xPos = 47;
    mPositionSingleInfoTmp.info.yPos = 74;
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    LoadImages(buff, mPositionSingleInfoTmp.itemNum);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    xSemaphoreGive(xSemaphore);  
    #if(SHOW_TIME_DEBUG)
    sysprintf(" [INFO EPD] <EPDDrawContainClear>  [%d].\n", xTaskGetTickCount() - tickLocalStart); 
    #endif
}
#define SEL_DEPOSIT_TIME_TOP_FONT_LINE_INDEX        EPD_PICT_NUM_BIG_INDEX//EPD_PICT_LINE_MEDIUM_INDEX  //?????
#define SEL_DEPOSIT_TIME_TOP_FONT_NUM_INDEX         EPD_PICT_NUM_BIG_INDEX
#define SEL_DEPOSIT_TIME_TOP_FONT_COLON_INDEX       EPD_PICT_COLON_BIG_INDEX
BOOL getIDFromTime(time_t time, timeIdInfo* info)
{
    uint8_t hour, mins;
    if(time == 0)
    {
        //return FALSE;
        hour = 0;
        mins = 0;
    }
    else
    {
        hour = (time/60)/60;
        if(hour > 99)
        {
            hour = 99;
        }
        mins = (time/60)%60;
    }
            
    info->id[0] = hour/10 + SEL_DEPOSIT_TIME_TOP_FONT_NUM_INDEX;
    info->id[1] = hour%10 + SEL_DEPOSIT_TIME_TOP_FONT_NUM_INDEX;
    info->id[2] = SEL_DEPOSIT_TIME_TOP_FONT_COLON_INDEX;
    info->id[3] = mins/10 + SEL_DEPOSIT_TIME_TOP_FONT_NUM_INDEX;
    info->id[4] = mins%10 + SEL_DEPOSIT_TIME_TOP_FONT_NUM_INDEX;
    return TRUE;
}  
#define TIME_START_X_POSITION     530
#define TIME_START_Y_POSITION     220

#define TIME_FONT_WIDTH            -64
#define TIME_COLON_FONT_WIDTH      -55

void EPDDrawTime(BOOL reFreshFlag, time_t time)
{
    positionTmpInfo mPositionTmpInfo;
    timeIdInfo idInfo;
    
    unsigned int* buff = (unsigned int*)&mPositionTmpInfo;   
    mPositionTmpInfo.itemNum = 5;
    
    if(getIDFromTime(time, &idInfo))
    {
        int i;
        for(i = 0; i<mPositionTmpInfo.itemNum; i++)
        {
            if(i == 0)
            {     
                mPositionTmpInfo.info[i].xPos = TIME_START_X_POSITION;
                mPositionTmpInfo.info[i].yPos = TIME_START_Y_POSITION;
            }
            else
            {
                if(i == 2)
                {
                    mPositionTmpInfo.info[i].xPos = mPositionTmpInfo.info[i-1].xPos + (unsigned int)TIME_COLON_FONT_WIDTH;
                }
                else
                {
                    mPositionTmpInfo.info[i].xPos = mPositionTmpInfo.info[i-1].xPos + (unsigned int)TIME_FONT_WIDTH;
                }
                mPositionTmpInfo.info[i].yPos = mPositionTmpInfo.info[i-1].yPos;
            }
            mPositionTmpInfo.info[i].bmpId = idInfo.id[i];
        }
    }   
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    LoadImages(buff, mPositionTmpInfo.itemNum);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    xSemaphoreGive(xSemaphore);  
}

#define SEL_DEPOSIT_TIME_DOWN_FONT_NUM_INDEX         EPD_PICT_NUM_BIG_INDEX
uint8_t getIDFromCost(uint32_t cost, costIdInfo* info)
{
    uint8_t totalPosition = 0;
    if(cost >= pow10Ex(MAX_COST_POSITION))
    {
        cost = pow10Ex(MAX_COST_POSITION) - 1;
    }
        
    if(cost == 0)
    {           
        info->id[totalPosition] = SEL_DEPOSIT_TIME_DOWN_FONT_NUM_INDEX;
        totalPosition = 1;
    }
    else
    {   
        int i;
        for(i = 0; i<MAX_COST_POSITION; i++) 
        {
                uint8_t targetValue = MAX_COST_POSITION - 1 - i;
                //sysprintf(" --> check : %d, %d(%d) \n", downCost, pow10Ex(targetValue));
                if(cost >= pow10Ex(targetValue))
                {                    
                    //sysprintf(" --> [%d]: targetValue = %d, [%d:%d] (%d: %d) \n ", i, targetValue, 
                    //                        downCost/pow10Ex(targetValue), (downCost/pow10Ex(targetValue + 1))*10, pow10Ex(targetValue), pow10Ex(targetValue + 1));
                    info->id[totalPosition] = cost/pow10Ex(targetValue) - (cost/pow10Ex(targetValue + 1))*10 + SEL_DEPOSIT_TIME_DOWN_FONT_NUM_INDEX;                    
                    //sysprintf("EPD:ShowSelDepositTime get position(%d) : %d,  totalPosition = %d\n", i, showNum[totalPosition] - SEL_DEPOSIT_TIME_DOWN_FONT_NUM_INDEX, totalPosition);
                    totalPosition++;
                }
                else
                {
                    //sysprintf("EPD:ShowSelDepositTime ignore position(%d)...\n", i);
                }
        }
    }
    return totalPosition;
}  
#define COST_START_X_POSITION     400
#define COST_START_Y_POSITION     100

#define COST_FONT_WIDTH            -70
#define COST_COLON_FONT_WIDTH      -55
void EPDDrawCost(BOOL reFreshFlag, uint32_t cost)
{
    positionTmpInfo mPositionTmpInfo;
    costIdInfo idInfo; 
    uint16_t targetX;
    int i;    
    unsigned int* buff = (unsigned int*)&mPositionTmpInfo;  

    uint8_t totalPosition = getIDFromCost(cost, &idInfo);    
    mPositionTmpInfo.itemNum = totalPosition + 1;
    
    switch(totalPosition)
    {
        case 5:
            targetX = COST_START_X_POSITION - COST_FONT_WIDTH*3/2;
            break;
        case 4:
            targetX = COST_START_X_POSITION - COST_FONT_WIDTH*2/2;
            break;
        case 3:
            targetX = COST_START_X_POSITION - COST_FONT_WIDTH*1/2;
            break;
        case 2:
            targetX = COST_START_X_POSITION;
            break;
        case 1:
            targetX = COST_START_X_POSITION + COST_FONT_WIDTH*1/2;
            break;                
    }
      
    //mPositionTmpInfo.info[0].bmpId = EPD_PICT_CONTAIN_SELECT_TIME_CAST_BG_INDEX;
    mPositionTmpInfo.info[0].bmpId = EPD_PICT_SMALL_CAST_BG_INDEX;
    mPositionTmpInfo.info[0].xPos = targetX - COST_FONT_WIDTH - (460+COST_FONT_WIDTH)-10;         
    mPositionTmpInfo.info[0].yPos = COST_START_Y_POSITION;  
    for(i = 1; i<totalPosition+1; i++)
    {
        if(i == 1)
        {     
            mPositionTmpInfo.info[i].xPos = targetX;
            mPositionTmpInfo.info[i].yPos = COST_START_Y_POSITION;
        }
        else
        {
            mPositionTmpInfo.info[i].xPos = mPositionTmpInfo.info[i-1].xPos + (unsigned int)COST_FONT_WIDTH;
            mPositionTmpInfo.info[i].yPos = mPositionTmpInfo.info[i-1].yPos;
        }
        mPositionTmpInfo.info[i].bmpId = idInfo.id[i-1];
    }   
    
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    LoadImages(buff, mPositionTmpInfo.itemNum);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);  
    xSemaphoreGive(xSemaphore);  
}




#define VOLTAGE_LEFT_FONT_LINE_INDEX        EPD_PICT_NUM_SMALL_2_INDEX//EPD_PICT_LINE_SMALL_INDEX 
#define VOLTAGE_LEFT_FONT_NUM_INDEX         EPD_PICT_NUM_SMALL_2_INDEX
#define VOLTAGE_LEFT_FONT_POINT_INDEX       EPD_PICT_POINT_SMALL_2_INDEX
#define VOLTAGE_LEFT_FONT_V_INDEX           EPD_PICT_V_SMALL_2_INDEX
#define VOLTAGE_LEFT_FONT_WIDTH             21

#define VOLTAGE_RIGHT_FONT_LINE_INDEX        EPD_PICT_NUM_SMALL_2_INDEX//EPD_PICT_LINE_BIG_INDEX ?????
#define VOLTAGE_RIGHT_FONT_NUM_INDEX         EPD_PICT_NUM_SMALL_2_INDEX
#define VOLTAGE_RIGHT_FONT_POINT_INDEX       EPD_PICT_POINT_SMALL_2_INDEX
#define VOLTAGE_RIGHT_FONT_V_INDEX          EPD_PICT_V_SMALL_2_INDEX
#define VOLTAGE_RIGHT_FONT_WIDTH             21

#define VOLTAGE_LEFT_X                              593
#define VOLTAGE_LEFT_Y                              70

#define VOLTAGE_RIGHT_X                             220

#define VOLTAGE_RIGHT_Y                             VOLTAGE_LEFT_Y
#define VOLTAGE_TOTAL_NUM_POSITION      12
void ShowVoltage(time_t leftVoltage, BOOL leftShow, time_t rightVoltage, BOOL rightShow, BOOL reFreshFlag)
{
    
    uint8_t showNum[VOLTAGE_TOTAL_NUM_POSITION];
    #if(BUILD_DEBUG_VERSION)
    sysprintf("EPD:ShowVoltage [top:%02d] : [down:%02d] ...\n", leftVoltage, rightVoltage );
    #endif

    int i;
    unsigned int buff[3*VOLTAGE_TOTAL_NUM_POSITION+1];
    uint8_t leftVol1, leftVol2, rightVol1, rightVol2;
    uint8_t buffIndex = 0;
    uint8_t counter = 0;
    buff[0] = 0;

    if(!leftShow && !rightShow)
    {
        sysprintf("EPD:ShowVoltage ignore!!!!\r\n");
        return;
    }
    if(leftShow)
    {
        if(leftVoltage == 0)
        {
            leftVol1 = 0;
            leftVol2 = 0;
            
            showNum[0] = VOLTAGE_LEFT_FONT_LINE_INDEX;
            showNum[1] = VOLTAGE_LEFT_FONT_LINE_INDEX;
            showNum[2] = VOLTAGE_LEFT_FONT_POINT_INDEX;
            showNum[3] = VOLTAGE_LEFT_FONT_LINE_INDEX;
            showNum[4] = VOLTAGE_LEFT_FONT_LINE_INDEX;
            showNum[5] = VOLTAGE_LEFT_FONT_V_INDEX;
        }
        else
        {
            leftVol1 = leftVoltage/100;
            leftVol2 = leftVoltage%100;
            
            showNum[0] = leftVol1/10 + VOLTAGE_LEFT_FONT_NUM_INDEX;
            showNum[1] = leftVol1%10 + VOLTAGE_LEFT_FONT_NUM_INDEX;
            showNum[2] = VOLTAGE_LEFT_FONT_POINT_INDEX;
            showNum[3] = leftVol2/10 + VOLTAGE_LEFT_FONT_NUM_INDEX;
            showNum[4] = leftVol2%10 + VOLTAGE_LEFT_FONT_NUM_INDEX;            
            showNum[5] = VOLTAGE_LEFT_FONT_V_INDEX;
        }
        counter = 0;
        for(i = 0; i < 6; i++) 
        {
            if(i == 0)
            {
                if(showNum[0] == VOLTAGE_LEFT_FONT_NUM_INDEX)
                {//??????0 ????
                    continue;
                }
            }
            buff[++buffIndex] = showNum[i];
            buff[++buffIndex] = VOLTAGE_LEFT_X - counter*VOLTAGE_LEFT_FONT_WIDTH;  
            buff[++buffIndex] = VOLTAGE_LEFT_Y; 
            counter++;
        }
        buff[0] = buff[0] + counter;
    }
    else
    {
        leftVol1 = 0;
        leftVol2 = 0;
    }
    if(rightShow)
    {
        if(rightVoltage == 0)
        {
            rightVol1 = 0;
            rightVol2 = 0;
            
            showNum[0] = VOLTAGE_RIGHT_FONT_LINE_INDEX;
            showNum[1] = VOLTAGE_RIGHT_FONT_LINE_INDEX;
            showNum[2] = VOLTAGE_RIGHT_FONT_POINT_INDEX;
            showNum[3] = VOLTAGE_RIGHT_FONT_LINE_INDEX;
            showNum[4] = VOLTAGE_RIGHT_FONT_LINE_INDEX;
            showNum[5] = VOLTAGE_RIGHT_FONT_V_INDEX;
        }
        else
        {
            rightVol1 = rightVoltage/100;
            rightVol2 = rightVoltage%100;

            showNum[0] = rightVol1/10 + VOLTAGE_RIGHT_FONT_NUM_INDEX;
            showNum[1] = rightVol1%10 + VOLTAGE_RIGHT_FONT_NUM_INDEX;
            showNum[2] = VOLTAGE_RIGHT_FONT_POINT_INDEX;
            showNum[3] = rightVol2/10 + VOLTAGE_RIGHT_FONT_NUM_INDEX;
            showNum[4] = rightVol2%10 + VOLTAGE_RIGHT_FONT_NUM_INDEX;
            showNum[5] = VOLTAGE_RIGHT_FONT_V_INDEX;
        }
        counter = 0;
        for(i = 0; i<6; i++) 
        {
            if(i == 0)
            {
                if(showNum[0] == VOLTAGE_RIGHT_FONT_NUM_INDEX)
                {//??????0 ????
                    continue;
                }
            }
            buff[++buffIndex] = showNum[i];
            buff[++buffIndex] = VOLTAGE_RIGHT_X - counter*VOLTAGE_RIGHT_FONT_WIDTH;         
            buff[++buffIndex] = VOLTAGE_RIGHT_Y;
            counter++;          

        }
        buff[0] = buff[0] + counter;
    }
    else
    {
        rightVol1 = 0;
        rightVol2 = 0;
    }  
    #if(BUILD_DEBUG_VERSION)
    sysprintf("EPD:ShowVoltage [top %02d:%02d] - [down %02d:%02d]...\n", leftVol1, leftVol2, rightVol1, rightVol2);
    #endif
    #if(0)
    for(i = 0; i<buff[0]*3+1; i++) 
    {
        sysprintf("EPD:buff[%02d]: 0x%04x(%d).. \n", i, buff[i], buff[i]);  
    }
    #endif
    if(buff[0] == 0)
    {
        sysprintf("EPD:ShowVoltage ignore 2!!!!\r\n");
        return;
    }
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
 
    LoadImages(buff, buff[0]);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);   
    xSemaphoreGive(xSemaphore);  
}

#define BATTERY_STATUS_LEFT_X                              (416+5)
#define BATTERY_STATUS_LEFT_Y                              142

#define BATTERY_STATUS_RIGHT_X                             (53+5)

#define BATTERY_STATUS_RIGHT_Y                             BATTERY_STATUS_LEFT_Y
#define BATTERY_STATUS_TOTAL_NUM_POSITION                   2
void ShowBatteryStatus(uint8_t leftId, BOOL leftShow, uint8_t rightId, BOOL rightShow, BOOL reFreshFlag)
{
    
    #if(BUILD_DEBUG_VERSION)
    sysprintf("EPD:ShowBatteryStatus [left:%02d] : [right:%02d] ...\n", leftId, rightId );
    #endif

    unsigned int buff[3*BATTERY_STATUS_TOTAL_NUM_POSITION+1];
    uint8_t buffIndex = 0;
    buff[0] = 0;

    if(!leftShow && !rightShow)
    {
        sysprintf("EPD:ShowBatteryStatus ignore!!!!\r\n");
        return;
    }
    if(leftShow)
    {
        buff[++buffIndex] = leftId;
        buff[++buffIndex] = BATTERY_STATUS_LEFT_X;  
        buff[++buffIndex] = BATTERY_STATUS_LEFT_Y; 
        buff[0] = buff[0] + 1;
    }
    
    if(rightShow)
    {
        buff[++buffIndex] = rightId;
        buff[++buffIndex] = BATTERY_STATUS_RIGHT_X;  
        buff[++buffIndex] = BATTERY_STATUS_RIGHT_Y; 
        buff[0] = buff[0] + 1;
    }

    #if(0)
    for(i = 0; i<buff[0]*3+1; i++) 
    {
        sysprintf("EPD:buff[%02d]: 0x%04x(%d).. \n", i, buff[i], buff[i]);  
    }
    #endif
    if(buff[0] == 0)
    {
        sysprintf("EPD:ShowBatteryStatus ignore 2!!!!\r\n");
        return;
    }
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
 
    LoadImages(buff, buff[0]);
    if(reFreshFlag)
        DisplayArea(0,0,EPD_WIDTH,EPD_HEIGHT,1);   
    xSemaphoreGive(xSemaphore);  
}
void EPDEnterCriticalSection(void)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
}
void EPDExitCriticalSection(void)
{
     xSemaphoreGive(xSemaphore);
}

void EPDSetSleepFunction(BOOL flag)
{
    xSemaphoreTake(xSemaphore, portMAX_DELAY); 
    if(flag)
    {
        sleepFunctionFlag = TRUE;
        EPDEnterSleep();
    }
    else
    {
        sleepFunctionFlag = FALSE;
        EPDExitSleep();
    }
    xSemaphoreGive(xSemaphore);
}

void IT8951WriteReg(uint16_t usRegAddr,uint16_t usValue)
{
	//Send Cmd , Register Address and Write Value
	WriteCmdCode(IT8951_TCON_REG_WR, 2);
	WriteData(usRegAddr,2);
	WriteData(usValue,2);
}




uint16_t IT8951GetVCOM(void)
{
	uint16_t vcom;
    WriteCmdCode(USDEF_I80_CMD_VCOM, 2);
    WriteData(0,2);
	//Read data from Host Data bus
	vcom = ReadDataEx(2);
	return vcom;
}

void IT8951SetVCOM(uint16_t vcom)
{
    WriteCmdCode(USDEF_I80_CMD_VCOM, 2);
    //WriteData(2,2);
    WriteData(0x0002,2);
	//Read data from Host Data bus
    WriteData(vcom,2);
}


void setSpecialCleanFlag(BOOL flag)
{
    specialCleanFlag = flag;
}

void setdataCode(uint32_t buildDataCode)
{
    dataCode = buildDataCode;
}


uint32_t getdataCode(void)
{
    return dataCode;
}


uint8_t getflashvalue(int position)
{

    uint8_t tempbyte;

    GPIO_ClrBit(GPIOI, BIT0); 
    vTaskDelay(10/portTICK_RATE_MS);
    W25Q64BVqueryEx(position,&tempbyte,sizeof(tempbyte));
    GPIO_SetBit(GPIOI, BIT0);
    //terninalPrintf("tempbyte = %d.\r\n",tempbyte);
    return tempbyte;
}





/*
static void SpiFlash_NormalRead(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    uint32_t i;

    // /CS: active
    //setFlashCS(flashIndex, TRUE);
    
    // /CS: active
    pSpiInterface->activeCSFunc(TRUE); 

    // send Command: 0x03, Read data
    pSpiInterface->writeFunc(0, 0x03);

    // send 24-bit start address
    pSpiInterface->writeFunc(0, (StartAddress>>16) & 0xFF);

    pSpiInterface->writeFunc(0, (StartAddress>>8) & 0xFF);

    pSpiInterface->writeFunc(0, StartAddress & 0xFF);

    // read data
    for(i=0; i<BuffLen; i++) {
        pSpiInterface->writeFunc(0, 0x00);
        u8DataBuffer[i] = pSpiInterface->readFunc(0);
    }
    // /CS: de-active
    //setFlashCS(flashIndex, FALSE);
    
    // /CS: de-active
    pSpiInterface->activeCSFunc(FALSE);
    
}


void FlashDrvNormalRead(uint32_t StartAddress, uint8_t *u8DataBuffer, int BuffLen)
{
    //xSemaphoreTake(xSemaphore, portMAX_DELAY);
    GPIO_ClrBit(RESET_PIN_PORT, RESET_PORT_BIT); 
    vTaskDelay(10/portTICK_RATE_MS);
    
    SpiFlash_WaitReady();
    SpiFlash_NormalRead(StartAddress, u8DataBuffer, BuffLen);
    SpiFlash_WaitReady();
    
    
    GPIO_SetBit(RESET_PIN_PORT, RESET_PORT_BIT);
    //xSemaphoreGive(xSemaphore);
}
*/
/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


/**************************************************************************//**
* @file     smartcarddrv.c
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
#include "adc.h"
#include "gpio.h"
#include "sc.h"
#include "sclib.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "smartcarddrv.h"
#include "interface.h"
#if (ENABLE_BURNIN_TESTER)
#include "timelib.h"
#include "burnintester.h"
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
#if (ENABLE_BURNIN_TESTER)
static uint32_t smartcardBurninCounter = 0;
static uint32_t smartcardBurninErrorCounter = 0;
#endif
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
/**
  * @brief  The interrupt services routine of smartcard port 1
  * @param  None
  * @return None
  */
void SC0_IRQHandler(void)
{
    // Please don't remove any of the function calls below

    // No need to check CD event for SIM card.
    //if(SCLIB_CheckCDEvent(0))
    //    return; // Card insert/remove event occurred, no need to check other event...
    SCLIB_CheckTimeOutEvent(0);
    SCLIB_CheckTxRxEvent(0);
    SCLIB_CheckErrorEvent(0);

    return;
}


#if (ENABLE_BURNIN_TESTER)
static BOOL smartcardGetCardInfo(void)
{
    int retval;
    int i;
    SCLIB_CARD_INFO_T cardInfo;
    // Activate slot 0
    retval = SCLIB_Activate(0, FALSE);

    if (retval != SCLIB_SUCCESS) 
    {
        sysprintf("!!! SIM card activate failed !!!\n");
        return FALSE;
    }
    else
    {
        sysprintf("SIM card activate OK\n");
    }
    
    retval = SCLIB_GetCardInfo(0, &cardInfo);
    SCLIB_Deactivate(0);
    if (retval != SCLIB_SUCCESS) 
    {
        sysprintf("!!! SCLIB_GetCardInfo failed !!!\n");
        return FALSE;
    }
    else
    {
        sysprintf("SCLIB_GetCardInfo OK\n");
        sysprintf("cardInfo.T = %d(0x%04x), cardInfo.ATR_Len = %d\n", cardInfo.T, cardInfo.T, cardInfo.ATR_Len);
        for (i = 0; i < cardInfo.ATR_Len; i++) {
             sysprintf("%02x ", cardInfo.ATR_Buf[i]);
        }
        sysprintf("\n");
    }
    return TRUE;
}
#endif

#if (ENABLE_BURNIN_TESTER)
static void vSmartCardTestTask(void *pvParameters)
{
    time_t lastTime = GetCurrentUTCTime();
    time_t currentTime;
    BOOL testLoop = FALSE;
    BOOL status;
    terninalPrintf("vSmartCardTestTask Going...\r\n");
    while (TRUE)
    {
        if (GetPrepareStopBurninFlag())
        {
            terninalPrintf("vSmartCardTestTask Terminated !!\r\n");
            vTaskDelete(NULL);
        }
        currentTime = GetCurrentUTCTime();
        if ((currentTime - lastTime) > BURNIN_SMARTCARD_INTERVAL)
        {
            //terninalPrintf("vSmartCardTestTask heartbeat.\r\n");
            lastTime = currentTime;
            testLoop = TRUE;
        }
        if (!testLoop)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        }
        testLoop = FALSE;
        status = smartcardGetCardInfo();
        smartcardBurninCounter++;
        if (!status) {
            smartcardBurninErrorCounter++;
        }
        //terninalPrintf("vSmartCardTestTask, status=%d\r\n", status);
    }
}
#endif




/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL SmartCardDrvInit(BOOL testModeFlag)
{
    int retval;
    int i;
    SCLIB_CARD_INFO_T cardInfo;
    // enable smartcard 0 clock and multi function pin
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | 0x00001000);
    outpw(REG_CLK_DIVCTL6, inpw(REG_CLK_DIVCTL6) | 0x02000000);

    // G10: RST, G11: CLK, G12: DAT, G13: PWR, G14: CD
    outpw(REG_SYS_GPG_MFPH, (inpw(REG_SYS_GPG_MFPH) & ~0x0FFFFF00) | 0x0AAAAA00);

    //sysprintf("\nThis sample code reads phone book from SIM card\n");

    // Open smartcard interface 0. CD pin state ignore and PWR pin high raise VCC pin to card
    SC_Open(0, SC_PIN_STATE_IGNORE,  SC_PIN_STATE_LOW/*SC_PIN_STATE_HIGH*/);
    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, SC0_IRQn, (PVOID)SC0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(SC0_IRQn);

    // Ignore CD pin for SIM card
    //while(SC_IsCardInserted(0) == FALSE);
    // Activate slot 0
    retval = SCLIB_Activate(0, FALSE);

    if(retval != SCLIB_SUCCESS) 
    {
        sysprintf("!!! SIM card activate failed !!!\n");
        return FALSE;
    }
    else
    {
        sysprintf("SIM card activate OK\n");
    }
    
    retval = SCLIB_GetCardInfo(0, &cardInfo);
    if(retval != SCLIB_SUCCESS) 
    {
        sysprintf("!!! SCLIB_GetCardInfo failed !!!\n");
        
    }
    else
    {
        sysprintf("SCLIB_GetCardInfo OK\n");
        sysprintf("cardInfo.T = %d(0x%04x), cardInfo.ATR_Len = %d\n", cardInfo.T, cardInfo.T, cardInfo.ATR_Len);
        for(i = 0; i < cardInfo.ATR_Len; i++)
             sysprintf("%02x ", cardInfo.ATR_Buf[i]);

        sysprintf("\n");
    }
    return TRUE;
}

#if (ENABLE_BURNIN_TESTER)
BOOL SmartCardTestInit(BOOL testModeFlag)
{
    // enable smartcard 0 clock and multi function pin
    outpw(REG_CLK_PCLKEN1, inpw(REG_CLK_PCLKEN1) | 0x00001000);
    outpw(REG_CLK_DIVCTL6, inpw(REG_CLK_DIVCTL6) | 0x02000000);

    // G10: RST, G11: CLK, G12: DAT, G13: PWR, G14: CD
    outpw(REG_SYS_GPG_MFPH, (inpw(REG_SYS_GPG_MFPH) & ~0x0FFFFF00) | 0x0AAAAA00);

    //sysprintf("\nThis sample code reads phone book from SIM card\n");

    // Open smartcard interface 0. CD pin state ignore and PWR pin high raise VCC pin to card
    SC_Open(0, SC_PIN_STATE_IGNORE,  SC_PIN_STATE_LOW/*SC_PIN_STATE_HIGH*/);
    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, SC0_IRQn, (PVOID)SC0_IRQHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(SC0_IRQn);

    // Ignore CD pin for SIM card
    //while(SC_IsCardInserted(0) == FALSE);
    xTaskCreate(vSmartCardTestTask, "vSmartCardTestTask", 1024*10, NULL, SMART_CARD_TEST_THREAD_PROI, NULL);
    return TRUE;
}
#endif

#if (ENABLE_BURNIN_TESTER)
uint32_t GetSmartCardBurninTestCounter(void)
{
    return smartcardBurninCounter;
}

uint32_t GetSmartCardBurninTestErrorCounter(void)
{
    return smartcardBurninErrorCounter;
}
#endif

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


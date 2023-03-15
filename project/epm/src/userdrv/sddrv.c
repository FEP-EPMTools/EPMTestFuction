/**************************************************************************//**
* @file  lwipdrv.c
* @version  V1.00
* $Revision:
* $Date:
* @brief
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "sddrv.h"
#include "sdh.h"
#include "ff.h"
#include "diskio.h"

#include "nuc970.h"
#include "sys.h"
#include "gpio.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//static BYTE SD_Drv; // select SD0
/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
#define _USE_DAT3_DETECT_


void SDH_IRQHandler(void)
{
    unsigned int volatile isr;
    //sysprintf("SDH_IRQHandler...\r\n");
    // FMI data abort interrupt
    if (inpw(REG_SDH_GINTSTS) & SDH_GINTSTS_DTAIF_Msk) {
        /* ResetAllEngine() */
        outpw(REG_SDH_GCTL, inpw(REG_SDH_GCTL) | SDH_GCTL_GCTLRST_Msk);
        outpw(REG_SDH_GINTSTS, SDH_GINTSTS_DTAIF_Msk);
    }

    //----- SD interrupt status
    isr = inpw(REG_SDH_INTSTS);
    if (isr & SDH_INTSTS_BLKDIF_Msk) {      // block down
        _sd_SDDataReady = TRUE;
        outpw(REG_SDH_INTSTS, SDH_INTSTS_BLKDIF_Msk);
    }

    if (isr & SDH_INTSTS_CDIF0_Msk) { // port 0 card detect
        //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            volatile int i;         // delay 30 fail, 50 OK
            for (i=0; i<0x500; i++);   // delay to make sure got updated value from REG_SDISR.
            isr = inpw(REG_SDH_INTSTS);
        }
#if(0)
#ifdef _USE_DAT3_DETECT_
        //#error
        if (!(isr & SDH_INTSTS_CDSTS0_Msk)) {
            SD0.IsCardInsert = FALSE;
            sysprintf("\nCard Remove!\n");
            SD_Close_Disk(0);
        } else {
            SD_Open_Disk(SD_PORT0 | CardDetect_From_DAT3);
        }
#else
        if (isr & SDH_INTSTS_CDSTS0_Msk) {
            SD0.IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            sysprintf("\nCard Remove!\n");
            SD_Close_Disk(0);
        } else {
            SD_Open_Disk(SD_PORT0 | CardDetect_From_GPIO);
        }
#endif
#endif
        outpw(REG_SDH_INTSTS, SDH_INTSTS_CDIF0_Msk);
    }

    if (isr & SDH_INTSTS_CDIF1_Msk) { // port 1 card detect
        //----- SD interrupt status
        // it is work to delay 50 times for SD_CLK = 200KHz
        {
            volatile int i;         // delay 30 fail, 50 OK
            for (i=0; i<0x500; i++);   // delay to make sure got updated value from REG_SDISR.
            isr = inpw(REG_SDH_INTSTS);
        }
#if(0)
#ifdef _USE_DAT3_DETECT_
        if (!(isr & SDH_INTSTS_CDSTS1_Msk)) {
            SD0.IsCardInsert = FALSE;
            sysprintf("\nCard Remove!\n");
            SD_Close_Disk(1);
        } else {
            SD_Open_Disk(SD_PORT1 | CardDetect_From_DAT3);
        }
#else
        if (isr & SDH_INTSTS_CDSTS1_Msk) {
            SD0.IsCardInsert = FALSE;   // SDISR_CD_Card = 1 means card remove for GPIO mode
            sysprintf("\nCard Remove!\n");
            SD_Close_Disk(1);
        } else {
            SD_Open_Disk(SD_PORT1 | CardDetect_From_GPIO);
        }
#endif
#endif

        outpw(REG_SDH_INTSTS, SDH_INTSTS_CDIF1_Msk);
    }

    // CRC error interrupt
    if (isr & SDH_INTSTS_CRCIF_Msk) {
        if (!(isr & SDH_INTSTS_CRC16_Msk)) {
            //syssysprintf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        } else if (!(isr & SDH_INTSTS_CRC7_Msk)) {
            extern unsigned int _sd_uR3_CMD;
            if (! _sd_uR3_CMD) {
                //syssysprintf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }
        outpw(REG_SDH_INTSTS, SDH_INTSTS_CRCIF_Msk);      // clear interrupt flag
    }
}

unsigned long get_fattime (void)
{
    unsigned long tmr;

    tmr=0x00000;

    return tmr;
}

static void SYS_Init(void)
{
    /* enable SDH */
    outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 0x40000000);

    /* select multi-function-pin */

    /* SD Port 0 -> PD0~6 */
    outpw(REG_SYS_GPD_MFPL, 0x6666666);
    
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
    //GPD7 Power pin
    outpw(REG_SYS_GPD_MFPL,(inpw(REG_SYS_GPD_MFPL) & ~(0xFu<<28)) | (0x0u<<28));
    GPIO_OpenBit(GPIOD, BIT7, DIR_OUTPUT, NO_PULL_UP); 
#if(0)    
    GPIO_SetBit(GPIOD, BIT7);  //power down  
#else    
    GPIO_ClrBit(GPIOD, BIT7); //power up  
#endif    

}
/*
static void vFatFsDrvTask( void *pvParameters )
{
    //vTaskDelay(2000); 
    sysprintf("vFatFsDrvTask Going...\r\n");      
    for(;;)
    {     
        vTaskDelay(100/portTICK_RATE_MS); 
        if(SD_CardDetection(SD_Drv))
        {
            //continue;      
        }            
    }
}
*/

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/



BOOL SdDrvInit(void)
{
    sysprintf("SdDrvInit...\r\n");
    SYS_Init();
    sysInstallISR(HIGH_LEVEL_SENSITIVE|IRQ_LEVEL_1, SDH_IRQn, (PVOID)SDH_IRQHandler);
    /* enable CPSR I bit */
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(SDH_IRQn);
    SD_SetReferenceClock(300000);
    SD_Open(SD_PORT0 | CardDetect_From_GPIO);
    if (SD_Probe((SD_PORT0 | CardDetect_From_GPIO) & 0x00ff) != TRUE) 
    {
        sysprintf("SD0 initial fail!!\n");
        return FALSE;
    }

    sysprintf("SdDrvInit...OK\r\n");
    return TRUE; 
}

BOOL SDDrvInitialize(uint8_t pdrv)
{
    if (SD_GET_CARD_CAPACITY(SD_PORT0) == 0)
        return FALSE;
    return TRUE;
}
BOOL SDDrvStatus(uint8_t pdrv)
{
    if (SD_GET_CARD_CAPACITY(SD_PORT0) == 0)
        return FALSE;
    return TRUE;
    
}
BOOL SDDrvRead(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count)
{
     outpw(REG_SDH_GCTL, SDH_GCTL_SDEN_Msk);
     if((DRESULT) SD_Read(SD_PORT0, buff, sector, count) == RES_OK)
     {
         return TRUE;
     }
     return FALSE;
}
BOOL SDDrvWrite(uint8_t pdrv, uint8_t *buff, uint32_t sector, UINT count)
{    
    outpw(REG_SDH_GCTL, SDH_GCTL_SDEN_Msk);
    if((DRESULT) SD_Write(SD_PORT0, buff, sector, count) == RES_OK)
    {
        return TRUE;
    }
    return FALSE;
}
BOOL SDDrvIoctl(uint8_t pdrv, uint8_t cmd, void *buff )
{
    BOOL res = TRUE;
    switch(cmd) 
    {
        case CTRL_SYNC:
            break;
        case GET_SECTOR_COUNT:
            *(DWORD*)buff = SD0.totalSectorN;
            break;
        case GET_SECTOR_SIZE:
            *(WORD*)buff = SD0.sectorSize;
            break;

        default:
            res = FALSE;
            break;
    }
    return res;
}

/*** Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


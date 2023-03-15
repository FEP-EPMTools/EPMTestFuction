/**************************************************************************//**
* @file     uvcdrv.c
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
#include "rtc.h"
#include "gpio.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "uvcdrv.h"
#include "interface.h"
#include "halinterface.h"
#include "usbh_lib.h"
#include "usbh_uvc.h"
#include "buzzerdrv.h"
#include "loglib.h"
//#include "scdrv.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
//GPB2 Ext Sel pin
#define EXT_SEL_PORT  GPIOB
#define EXT_SEL_PIN   BIT2


//GPB5 Power pin (USB1)
#define POWER_PORT  GPIOB 
#define POWER_PIN   BIT5

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static BOOL initFlag = FALSE;
static SemaphoreHandle_t xRunningSemaphore = NULL;
static BOOL changeBufferFlag = FALSE;

static BOOL powerStatus = FALSE;
static int counterNumber[2] = {0, 0};

static BOOL testStatus = FALSE;
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL uvcSetPower(BOOL flag)
{    
    //sysprintf("uvcSetPower1 [%08x] -A- \r\n", GPIO_ReadPort(GPIOB));
    if(flag)
    {      
        sysprintf("uvcSetPower ON!!\n"); 
        //GPIO_ClrBit(GPIOB, BIT6);   
        GPIO_ClrBit(POWER_PORT, POWER_PIN);         
        //GPIO_SetBit(POWER_PORT, POWER_PIN); 
                
    }
    else
    {
        sysprintf("uvcSetPower OFF!!\n"); 
        //GPIO_ClrBit(POWER_PORT, POWER_PIN);   
        //vTaskDelay(200/portTICK_RATE_MS);
        //GPIO_SetBit(GPIOB, BIT6);
        GPIO_SetBit(POWER_PORT, POWER_PIN);
        
            
    }
    //sysprintf("uvcSetPower1 [%08x] -B- \r\n", GPIO_ReadPort(GPIOB));
   // vTaskDelay(10000/portTICK_RATE_MS);
   // sysprintf("uvcSetPower1 [%08x] -C- \r\n", GPIO_ReadPort(GPIOB));
    powerStatus = flag;
    return TRUE;
}
static BOOL uvcSetExtSel(int sel)
{
    if(sel == 0)
    {      
        sysprintf(" --- {INFO} uvcSetExtSel 0 ---!!\n"); 
        GPIO_SetBit(EXT_SEL_PORT, EXT_SEL_PIN);              
    }
    else
    {
        sysprintf(" --- {INFO} uvcSetExtSel 1 ---!!\n"); 
        GPIO_ClrBit(EXT_SEL_PORT, EXT_SEL_PIN);         
    }
    return TRUE;
}
static BOOL uvcSetPower0(BOOL flag)
{
    if(flag)
    {
        uvcSetExtSel(0);
        //vTaskDelay(200/portTICK_RATE_MS);
    }
    return uvcSetPower(flag);
}
static BOOL uvcSetPower1(BOOL flag)
{
    if(flag)
    {
        uvcSetExtSel(1);
        //vTaskDelay(200/portTICK_RATE_MS);
    }
    return uvcSetPower(flag);
}

static void enableHostClk(void)
{
    _ehci->USBPCR1 = 0x520;                /* enable PHY 1          */
    outpw(REG_CLK_HCLKEN,(inpw(REG_CLK_HCLKEN) & ~(0x1<<18)) | (0x1<<18));//enable USB HOST clock.
}
static void disableHostClk(void)
{
    outpw(REG_CLK_HCLKEN,(inpw(REG_CLK_HCLKEN) & ~(0x1<<18)) | (0x0<<18));//disable USB HOST clock.
     _ehci->USBPCR1 = 0x020;                /* disable PHY 1          */
}
static BOOL hwInit(void)
{     
    sysprintf("UVCDrvInit hwInit!!\n");  
    outpw(REG_CLK_HCLKEN,(inpw(REG_CLK_HCLKEN) & ~(0x1<<19)) | (0x0<<19));//disable USB device clock.
    //outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 0x40000);
    outpw(REG_CLK_HCLKEN,(inpw(REG_CLK_HCLKEN) & ~(0x1<<18)) | (0x1<<18));//enable USB HOST clock.
	//outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);//uart0 clock
    
    //_ehci->USBPCR0 = 0x060;                /* disable PHY 0          */
    
    //_ehci->USBPCR0 = 0x160;                /* enable PHY 0          */
    //_ehci->USBPCR1 = 0x520;                /* enable PHY 1          */
    
    
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.

	// set PE.14 & PE.15 for USBH_PPWR0 & USBH_PPWR1
	//outpw(REG_SYS_GPE_MFPH, (inpw(REG_SYS_GPE_MFPH) & ~0xff000000) | 0x77000000);

    //GPB2 Ext Sel pin
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(EXT_SEL_PORT, EXT_SEL_PIN, DIR_OUTPUT, NO_PULL_UP);     
    uvcSetExtSel(0);
    
    //GPB5 Power pin (USB1)
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(POWER_PORT, POWER_PIN, DIR_OUTPUT, NO_PULL_UP); 
    uvcSetPower(FALSE);

    return TRUE;
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL UVCDrvInit(BOOL testModeFlag)
{
    if(initFlag)
    {
        sysprintf("UVCDrvInit already inited!!\n");  
        return TRUE;
    }
    sysprintf("UVCDrvInit  Version 2  (testModeFlag = %d)!!\n", testModeFlag);  
    
    xRunningSemaphore  = xSemaphoreCreateMutex();
    
    hwInit();
    
    usbh_core_init();    
    
	usbh_uvc_init();
	//usbh_pooling_hubs();
    //sysprintf("UVCDrvInit Exit!!\n");
    
    usbh_pooling_hubs();
    usbh_pooling_hubs();
    
    //UVCSetPower(0, FALSE); 
    UVCSetPower(0, FALSE);
    UVCSetPower(1, FALSE);
    
    testStatus = testModeFlag;
    
    initFlag = TRUE;
    return TRUE;
}
void UVCSetPower(int index, BOOL flag)
{  
    //sysprintf("UVCSetPower [%d:%d]!!\n", index, flag);
    if(index == 0)
    {
        uvcSetPower0(flag);
    }
    else
    {
        uvcSetPower1(flag);   
    }
}


#define USB_PPWR_PE

#define SELECT_MJPEG

#define SELECT_SMALL_RES_WIDTH     320
#define SELECT_SMALL_RES_HEIGHT    240

//#define SELECT_RES_WIDTH     800
//#define SELECT_RES_HEIGHT    600
#define SELECT_RES_WIDTH     1920
#define SELECT_RES_HEIGHT    1080

//#define SELECT_RES_WIDTH     1280
//#define SELECT_RES_HEIGHT    720

//[0] MJPEG, 1280 x 720
//[1] MJPEG, 640 x 480
//[2] MJPEG, 320 x 240
//[3] MJPEG, 800 x 600
//[4] MJPEG, 1280 x 960


//#define SELECT_RES_WIDTH     1280
//#define SELECT_RES_HEIGHT    960

//#define SELECT_RES_WIDTH_2     640
//#define SELECT_RES_HEIGHT_2    480

#define SELECT_RES_WIDTH_2     1280
#define SELECT_RES_HEIGHT_2    720

#define IMAGE_MAX_SIZE       (1280*960*2)

//static UVC_DEV_T   *g_vdev[2] = { NULL, NULL };
static UVC_DEV_T   *g_vdev = { NULL};

/*---------------------------------------------------------------------- 
 * Image buffers  
 */
//#define IMAGE_BUFF_CNT       1

enum 
{
    IMAGE_BUFF_FREE,
    IMAGE_BUFF_USB,
    IMAGE_BUFF_READY,
    IMAGE_BUFF_POST
};

struct ig_buff_t
{
    uint8_t   *buff;
    int       len;
    int       state;
};   
struct ig_buff_t  _ig;
struct ig_buff_t  _igTemp;

__align(32) uint8_t  image_buff_pool[IMAGE_MAX_SIZE];
__align(32) uint8_t  image_buff_pool_Temp[IMAGE_MAX_SIZE];

//__align(32) uint8_t  snapshot_buff_pool[2][IMAGE_MAX_SIZE];

//int   _idx_usb = 0, _idx_post = 0;



/*------------------------------------------------------------------------*/

uint8_t  * g_OSD_base = 0;
uint8_t  * g_LCD_base = 0;

void delay_us(int usec)
{
    #if(FREERTOS_USE_1000MHZ)
    //volatile int  loop = 300 * usec / 5;
    //while (loop > 0) loop--;
    sysDelay((usec/1000)/portTICK_RATE_MS);
    #else
    volatile int  loop = 300 * usec / 5;
    while (loop > 0) loop--;
    #endif
}

uint32_t get_ticks(void)
{
    #if(FREERTOS_USE_1000MHZ)
    return xTaskGetTickCount()/10;
    #else
    return sysGetTicks(TIMER0);
    #endif
}


void  dump_buff_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0) 
    {
        sysprintf("0x%04X  ", nIdx);
        for (i = 0; (i < 16) && (nBytes > 0); i++)
        {
            sysprintf("%02x ", pucBuff[nIdx + i]);
            nBytes--;
        }
        nIdx += 16;
        sysprintf("\n");
    }
    sysprintf("\n");
}


void  init_image_buffers(void)
{
    //for (i = 0; i < IMAGE_BUFF_CNT; i++)
    //{
    //    _ig[i].buff   = (uint8_t *)((uint32_t)image_buff_pool[i] | 0x80000000);
    //    _ig[i].len    = 0;
    //    _ig[i].state  = IMAGE_BUFF_FREE;
    //}
    sysprintf("init_image_buffers [%d]!!\n");
    _ig.buff   = (uint8_t *)((uint32_t)image_buff_pool | 0x80000000);
    _ig.len    = 0;
    _ig.state  = IMAGE_BUFF_FREE;
    
    _igTemp.buff   = (uint8_t *)((uint32_t)image_buff_pool_Temp | 0x80000000);
    _igTemp.len    = 0;
    _igTemp.state  = IMAGE_BUFF_FREE;

    //_idx_usb = 0;
    //_idx_post = 0;
}


int  uvc_rx_callbak(UVC_DEV_T *vdev, uint8_t *data, int len)
{
    //sysprintf("uvc_rx_callbak [%d], changeBufferFlag = %d!!\n", len, changeBufferFlag);
    _ig.state = IMAGE_BUFF_READY;   /* mark the current buffer as ready for decode/display */
    _ig.len   = len;                /* length of this newly received image   */
    if(changeBufferFlag)
    {
        changeBufferFlag = FALSE;
        //usbh_uvc_set_video_buffer(vdev, _igTemp.buff, IMAGE_MAX_SIZE);
        memcpy(_igTemp.buff, _ig.buff, _ig.len);
        _igTemp.len = _ig.len;
    }

    return 0;
}
static int errorCounter[2] = {0};

BOOL UVCTakePhoto(int index, uint8_t** photoPr, int* photoLen, StorageType type, char* dir, char* fileName, BOOL smallSizeFlag, int photoNum, int takeInterval, cameraSavedCallback callback)
{
    BOOL reVal = FALSE;
    int n = 0;
    int t0, ret;
    char targetFileName[_MAX_LFN];
    int takePhotoNum = 0;
    if(photoNum < 1)
    {
        return FALSE;
    }
   
    *photoLen = 0;
    xSemaphoreTake(xRunningSemaphore, portMAX_DELAY);
 
    sysprintf("\r\n ----- UVCTakePhoto [%d][%s]!!  START (smallSizeFlag = %d) (photoNum:%d, takeInterval:%d)-----\n", index, fileName, smallSizeFlag, photoNum, takeInterval*portTICK_RATE_MS);
    
    
    usbh_core_init();   
	usbh_uvc_init();
    vTaskDelay(100/portTICK_RATE_MS);
    //_ehci->USBPCR0 = 0x160;                /* enable PHY 0          */
    //_ehci->USBPCR1 = 0x520;                /* enable PHY 1          */
    
    //usbh_pooling_hubs();
    //usbh_pooling_hubs();
#if(1)
    switch(index)
    {
        case 0:
            UVCSetPower(0, TRUE);
            counterNumber[0]++;
            break;
        case 1:
            UVCSetPower(1, TRUE);
            counterNumber[1]++;
            break;
        default:
            return reVal;
            
    }
#endif
    vTaskDelay(100/portTICK_RATE_MS);
    //enableHostClk();
    //vTaskDelay(200/portTICK_RATE_MS);
    //usbh_resume();
    usbh_pooling_hubs();
    vTaskDelay(100/portTICK_RATE_MS);
    usbh_pooling_hubs();
    vTaskDelay(100/portTICK_RATE_MS);
    usbh_pooling_hubs();
    vTaskDelay(100/portTICK_RATE_MS);
    usbh_pooling_hubs();
    vTaskDelay(100/portTICK_RATE_MS);
    /*
     *  Has hub port event.
     */
    g_vdev = usbh_uvc_get_device_list();
    if (g_vdev == NULL)
    {
        g_vdev = NULL;
        terninalPrintf(" [No device connected]:< Camera %d >\n\n", index);
        goto takePhotoeExit;
    }        
            
    /*----------------------------------------------------------------------------*/
    /*  Both UVC devices connected.                                               */
    /*----------------------------------------------------------------------------*/
                     
    //sysprintf("\n\n-------  UVC %d --------------------------------------------------\n", n);
    //sysprintf("[Video format list]\n");
    #if(0)
    for (int i = 0; ;i++)
    {
        IMAGE_FORMAT_E  format;
        int width, height;  
        ret = usbh_get_video_format(g_vdev, i, &format, &width, &height);
        if (ret != 0)
            break;

        sysprintf("[%d] %s, %d x %d\n", i, (format == UVC_FORMAT_MJPEG ? "MJPEG" : "YUYV"), width, height);
    }
    sysprintf("\n\n");
    #endif
    
    if(smallSizeFlag)
    {
        sysprintf("usbh_set_video_format %d x %d \r\n", SELECT_SMALL_RES_WIDTH, SELECT_SMALL_RES_HEIGHT);
        ret = usbh_set_video_format(g_vdev, UVC_FORMAT_MJPEG, SELECT_SMALL_RES_WIDTH, SELECT_SMALL_RES_HEIGHT);                
    }
    else
    {
        sysprintf("usbh_set_video_format %d x %d \r\n", SELECT_RES_WIDTH, SELECT_RES_HEIGHT);
        ret = usbh_set_video_format(g_vdev, UVC_FORMAT_MJPEG, SELECT_RES_WIDTH, SELECT_RES_HEIGHT);
    }
    if (ret != 0)
    {
        sysprintf("usbh_set_video_format failed! retry another- 0x%x (%d)\r\n", ret, ret);
        ret = usbh_set_video_format(g_vdev, UVC_FORMAT_MJPEG, SELECT_RES_WIDTH_2, SELECT_RES_HEIGHT_2);
    }
    else
    {
        //sysprintf("usbh_set_video_format OK! \r\n");
    }

    if (ret != 0)
    {
        terninalPrintf("usbh_set_video_format failed! - 0x%x (%d)\n", ret, ret);
        goto takePhotoeExit;
    }
    //usbh_memory_used();        
    
    changeBufferFlag = FALSE;
    init_image_buffers();
    
    /* assign the first image buffer to receive the image from USB */
    usbh_uvc_set_video_buffer(g_vdev, _ig.buff, IMAGE_MAX_SIZE);
    _ig.state = IMAGE_BUFF_USB;
    
    sysprintf("Start UVC %d video streaming< Camera %d >...\n\n", n, index);
    
    ret = usbh_uvc_start_streaming(g_vdev, uvc_rx_callbak);
    if (ret != 0)
    {
        terninalPrintf(" ~~~ usbh_uvc_start_streaming failed! - %d\n", ret);
        terninalPrintf(" ~~~ Please re-connect UVC device...\n"); 
        goto takePhotoeExit;
    }
    //sysprintf("Start UVC %d video Waiting 1000ns...\n");
    vTaskDelay(1000/portTICK_RATE_MS);  
    while(1)
    {
        init_image_buffers();        
        changeBufferFlag = TRUE;
        t0 = get_ticks();

        while ((get_ticks() - t0 < (300)) && (_ig.state != IMAGE_BUFF_READY))
        {
            //sysprintf("^\r\n");
            vTaskDelay(100/portTICK_RATE_MS);
        } ;

        if (get_ticks() - t0 >= (300))
        {
            terninalPrintf(" ~~~ Cannot get image from UVC device %d in 3 seconds!!< Camera %d >...\n\n", n, index);
            reVal = FALSE;
            goto takePhotoeExit;
        }
        
        _ig.state = IMAGE_BUFF_POST;

        if(photoNum > 1)
        {
            sprintf(targetFileName, "%s-[%d].jpg", fileName, takePhotoNum);
        }
        else
        {
            sprintf(targetFileName, "%s", fileName);
        }
        
        //sysprintf(" ===> Get UVC %d video snaphot...\n", n);

        /* 
         * Get the snapshot 
         */
        terninalPrintf(" ~~~ UVCTakePhoto[usb index:%d] OK [%s](photoSize = %d, index = %d, dalay = %d) !!!\r\n", n, targetFileName, _ig.len, takePhotoNum, takeInterval*portTICK_RATE_MS);
        
        
        if(testStatus)
        {
            reVal = TRUE;
        }
        else
        {
            if(fileName != NULL)
            {        
                reVal = TRUE;            
                if(reVal)
                {
                    //BuzzerPlay(50, 100, 1, TRUE);                
                    #if(1)
                    uint8_t* pDate = pvPortMalloc(_igTemp.len);
                    memcpy(pDate, _igTemp.buff, _igTemp.len);
                    
                    *photoLen = _igTemp.len;
                    *photoPr = pDate;
                    
                    //BOOL dataNeedFreeFlag, BOOL blockFlag, BOOL checkMode
                    if(FileAgentAddDataEx(type, dir, targetFileName, pDate, _igTemp.len, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE, callback) !=  FILE_AGENT_RETURN_ERROR )
                    //if(FileAgentAddData(type, dir, fileName, pDate, _igTemp.len, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, TRUE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
                    {                                
                        //successTimes++;
                        reVal = TRUE;
                    }
                    else
                    {
                        reVal = FALSE;
                        sysprintf(" ~~~ UVCTakePhoto[usb index:%d] WRITE ERROR (photoSize = %d)\r\n", n, _igTemp.len);
                    }
                    #endif
                    
                    #if(0)
                    if(smallSizeFlag)
                    {
                        if(SCEncryptSAMData(type, dir, fileName, pDate, _igTemp.len))
                        {
                        }
                    }
                    #endif

                }
                else
                {
                    //BuzzerPlay(50, 100, 2, TRUE);
                }
            }
            else
            {
                //successTimes++;
                reVal = TRUE; 
            }
        }
        takePhotoNum++;
        if(takePhotoNum == photoNum)
        {
            goto takePhotoeExit;
        }
        vTaskDelay(takeInterval);
    }

takePhotoeExit:
    #if(0)
    //sysprintf("Stop UVC %d video streaming...\n", n);
    if (ret == 0)
    //if(reVal)
    {
        ret = usbh_uvc_stop_streaming(g_vdev);
        if (ret != 0)
        {
            sysprintf("\nusbh_uvc_stop_streaming failed! - %d\n", ret);
            // break;
        }
    }
    vTaskDelay(200/portTICK_RATE_MS);
    #endif
    
    
    #if(1)    
    switch(index)
    {
        case 0:
            UVCSetPower(0, FALSE);
            break;
        case 1:
            UVCSetPower(1, FALSE);
            break;
            
    }
    #endif

    usbh_pooling_hubs();    /* turn-off VBUS cause device disconnected, this call to detect disconnect */
    usbh_pooling_hubs();
    #if(1)
    _ehci->USBPCR0 = 0x060;                /* disable PHY 0          */
    _ehci->USBPCR1 = 0x020;                /* disable PHY 1          */
    #endif
    
    if(reVal)
    {
        errorCounter[index] = 0;
    }
    else
    {
        errorCounter[index]++;
        if(errorCounter[index] > 2)
        {
            terninalPrintf(" ~~~ < errorCounter %d:%d >.RESET HOST..\n\n", index, errorCounter[index]);
            errorCounter[index] = 0;
            usbh_core_init(); 
            usbh_uvc_init();
        }
        else
        {
            terninalPrintf(" ~~~ < errorCounter %d:%d >...\n\n", index, errorCounter[index]);
        }
    }
    
    
    
    //disableHostClk();

    if(testStatus)
    {
        if(reVal)
        {
            //BuzzerPlay(50, 100, 1, TRUE);   
        }
        else
        {
            BuzzerPlay(50, 100, 1, TRUE); 
        }
    }
    else
    {
        if(reVal)
        {
            BuzzerPlay(50, 100, 1, TRUE);   
        }
        else
        {
            BuzzerPlay(50, 100, 2, TRUE);
        }
    }
    sysprintf("\r\n ----- UVCTakePhoto [%d][%s](photoNum:%d, takeInterval:%d)!!  END -----\r\n", index, fileName, photoNum, takeInterval*portTICK_RATE_MS);   
    xSemaphoreGive(xRunningSemaphore);
   return reVal;
}
void UVCLogStatus(void)
{
    static char logStr[128];
    sprintf(logStr, "-- {UVC counterNumber: %d, %d, power: %d} --\r\n", counterNumber[0], counterNumber[1], powerStatus);
    LoglibPrintf(LOG_TYPE_INFO, logStr, FALSE);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


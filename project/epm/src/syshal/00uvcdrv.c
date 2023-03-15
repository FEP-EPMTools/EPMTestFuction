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
//GPB2 Power pin (USB0)
#define POWER_PORT_0  GPIOB
#define POWER_PIN_0   BIT2


//GPB5 Power pin (USB1)
#define POWER_PORT_1  GPIOB
#define POWER_PIN_1   BIT5

#define USE_EVB_POWER   0

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static BOOL initFlag = FALSE;
static SemaphoreHandle_t xRunningSemaphore = NULL;
static BOOL changeBufferFlag = FALSE;

static BOOL powerStatus[2] = {FALSE, FALSE};
static int counterNumber[2] = {0, 0};
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL uvcSetPower0(BOOL flag)
{
    //sysprintf("uvcSetPower0 [%08x] -A- \r\n", GPIO_ReadPort(GPIOB));
    if(flag)
    {      
        sysprintf("uvcSetPower0 ON!!\n"); 
        //BuzzerPlay(50, 100, 1, TRUE);
        GPIO_SetBit(POWER_PORT_0, POWER_PIN_0);              
    }
    else
    {
        sysprintf("uvcSetPower0 OFF!!\n"); 
        //BuzzerPlay(50, 100, 1, TRUE);
        //BuzzerPlay(200, 50, 1, TRUE);
        GPIO_ClrBit(POWER_PORT_0, POWER_PIN_0);       
    }
    //sysprintf("uvcSetPower0 [%08x] -B- \r\n", GPIO_ReadPort(GPIOB));
    //vTaskDelay(10000/portTICK_RATE_MS);
    //sysprintf("uvcSetPower0 [%08x] -C- \r\n", GPIO_ReadPort(GPIOB));
    powerStatus[0] = flag;
    return TRUE;
}


static BOOL uvcSetPower1(BOOL flag)
{    
    //sysprintf("uvcSetPower1 [%08x] -A- \r\n", GPIO_ReadPort(GPIOB));
    if(flag)
    {      
        sysprintf("uvcSetPower1 ON!!\n"); 
        //BuzzerPlay(50, 100, 2, TRUE);
        GPIO_SetBit(POWER_PORT_1, POWER_PIN_1);              
    }
    else
    {
        sysprintf("uvcSetPower1 OFF!!\n"); 
        //BuzzerPlay(50, 100, 2, TRUE);
        //BuzzerPlay(200, 50, 1, TRUE);
        GPIO_ClrBit(POWER_PORT_1, POWER_PIN_1);       
    }
    //sysprintf("uvcSetPower1 [%08x] -B- \r\n", GPIO_ReadPort(GPIOB));
   // vTaskDelay(10000/portTICK_RATE_MS);
   // sysprintf("uvcSetPower1 [%08x] -C- \r\n", GPIO_ReadPort(GPIOB));
    powerStatus[1] = flag;
    return TRUE;
}


static BOOL hwInit(void)
{     
    sysprintf("UVCDrvInit hwInit!!\n");  
    outpw(REG_CLK_HCLKEN,(inpw(REG_CLK_HCLKEN) & ~(0x1<<19)) | (0x0<<19));//disable USB device clock.
    //outpw(REG_CLK_HCLKEN, inpw(REG_CLK_HCLKEN) | 0x40000);
    outpw(REG_CLK_HCLKEN,(inpw(REG_CLK_HCLKEN) & ~(0x1<<18)) | (0x1<<18));//enable USB HOST clock.
	//outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x10000);//uart0 clock
    
    
    outpw(REG_CLK_PCLKEN0,inpw(REG_CLK_PCLKEN0) | (1<<3)); //Enable GPIO engin clock.
#if(USE_EVB_POWER)
#else
	// set PE.14 & PE.15 for USBH_PPWR0 & USBH_PPWR1
	//outpw(REG_SYS_GPE_MFPH, (inpw(REG_SYS_GPE_MFPH) & ~0xff000000) | 0x77000000);

    //GPB2
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<8)) | (0x0<<8));
    GPIO_OpenBit(POWER_PORT_0, POWER_PIN_0, DIR_OUTPUT, NO_PULL_UP);     
    uvcSetPower0(FALSE);
    
    //GPB5
    outpw(REG_SYS_GPB_MFPL,(inpw(REG_SYS_GPB_MFPL) & ~(0xF<<20)) | (0x0<<20));
    GPIO_OpenBit(POWER_PORT_1, POWER_PIN_1, DIR_OUTPUT, NO_PULL_UP); 
    uvcSetPower1(FALSE);
    

    
    
    //UVCSetPower(0, TRUE);
#endif
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
    sysprintf("UVCDrvInit!!\n");  
    
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
    

    
    initFlag = TRUE;
    return TRUE;
}
void UVCSetPower(int index, BOOL flag)
{  
    //sysprintf("UVCSetPower [%d:%d]!!\n", index, flag);
#if(USE_EVB_POWER)
    if(flag)
    {
        outpw(REG_SYS_GPE_MFPH, inpw(REG_SYS_GPE_MFPH) | 0x77000000); /* set PE.14 & PE.15 as USBH PPWR   */
    }
    else
    {
        outpw(REG_GPIOE_DATAOUT, inpw(REG_GPIOE_DATAOUT) & 0x3FFF);   /* make PE.14 & PE.15 output low     */
        outpw(REG_GPIOE_DIR, inpw(REG_GPIOE_DIR) | 0xC000);           /* set PE.14 & PE.15 as output mode  */
        outpw(REG_SYS_GPE_MFPH, inpw(REG_SYS_GPE_MFPH) & 0x00FFFFFF); /* set PE.14 & PE.15 as GPIO mode    */
    }
#else
    if(index == 0)
    {
        uvcSetPower0(flag);
    }
    else
    {
        uvcSetPower1(flag);   
    }
#endif
}


#define USB_PPWR_PE

#define SELECT_MJPEG

#define SELECT_SMALL_RES_WIDTH     320
#define SELECT_SMALL_RES_HEIGHT    240

#define SELECT_RES_WIDTH     800
#define SELECT_RES_HEIGHT    600


//[0] MJPEG, 1280 x 720
//[1] MJPEG, 640 x 480
//[2] MJPEG, 320 x 240
//[3] MJPEG, 800 x 600
//[4] MJPEG, 1280 x 960


//#define SELECT_RES_WIDTH     1280
//#define SELECT_RES_HEIGHT    960

#define SELECT_RES_WIDTH_2     640
#define SELECT_RES_HEIGHT_2    480

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
    volatile int  loop = 300 * usec / 5;
    while (loop > 0) loop--;
}

uint32_t get_ticks(void)
{
    return sysGetTicks(TIMER0);
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
    //sysprintf("uvc_rx_callbak [%d]!!\n", len);
    _ig.state = IMAGE_BUFF_READY;   /* mark the current buffer as ready for decode/display */
    _ig.len   = len;                /* length of this newly received image   */
    if(changeBufferFlag)
    {
        changeBufferFlag = FALSE;
        usbh_uvc_set_video_buffer(vdev, _igTemp.buff, IMAGE_MAX_SIZE);
    }

    return 0;
}


BOOL UVCTakePhoto(int index, uint8_t** photoPr, int* photoLen, StorageType type, char* dir, char* fileName, BOOL smallSizeFlag)
{
    BOOL reVal = FALSE;
    
//    IMAGE_FORMAT_E  format;
//    int             n, i, width, height;   
    int             n = 0;
    //uint8_t         *snapshot_buff[2];
    //int             snapshot_len[2];//, post_idx = 0, post_snapshot_time = 0;
//    int             first_pass = 1;
    int             t0, ret;
    //int             successTimes = 0;
    
    //smallSizeFlag = FALSE;
    
    *photoLen = 0;
    xSemaphoreTake(xRunningSemaphore, portMAX_DELAY);
        
    sysprintf("\r\n ----- UVCTakePhoto [%d]!!  START (smallSizeFlag = %d)-----\n", index, smallSizeFlag);
    
    _ehci->USBPCR0 = 0x160;                /* enable PHY 0          */
    _ehci->USBPCR1 = 0x520;                /* enable PHY 1          */
    //usbh_uvc_init();
    //vTaskDelay(200/portTICK_RATE_MS);
    
    usbh_pooling_hubs();
    usbh_pooling_hubs();
    //usbh_uvc_init();
    //snapshot_buff[0] = (uint8_t *)((uint32_t)&snapshot_buff_pool[0] | 0x80000000);
    //snapshot_buff[1] = (uint8_t *)((uint32_t)&snapshot_buff_pool[1] | 0x80000000);
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
        //case 2:
        //    //_ehci->USBPCR0 = 0x160;                /* enable PHY 0          */
        //    UVCSetPower(0, TRUE);
        //    //_ehci->USBPCR1 = 0x520;                /* enable PHY 1          */
        //    UVCSetPower(1, TRUE);
        //    break;
        default:
            return reVal;
            
    }
#endif
    vTaskDelay(1000/portTICK_RATE_MS);
    //usbh_resume();
    usbh_pooling_hubs();
    usbh_pooling_hubs();
    usbh_pooling_hubs();
    //while(1) 
    {
        /*
         *  Has hub port event.
         */
        g_vdev = usbh_uvc_get_device_list();
        if (g_vdev == NULL)
        {
            g_vdev = NULL;
            sysprintf(" [No device connected]\n\n");
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
        	
#ifdef SELECT_MJPEG  
            if(smallSizeFlag)
            {
                //sysprintf("usbh_set_video_format %d x %d \r\n", SELECT_SMALL_RES_WIDTH, SELECT_SMALL_RES_HEIGHT);
                ret = usbh_set_video_format(g_vdev, UVC_FORMAT_MJPEG, SELECT_SMALL_RES_WIDTH, SELECT_SMALL_RES_HEIGHT);                
            }
            else
            {
                //sysprintf("usbh_set_video_format %d x %d \r\n", SELECT_RES_WIDTH, SELECT_RES_HEIGHT);
                ret = usbh_set_video_format(g_vdev, UVC_FORMAT_MJPEG, SELECT_RES_WIDTH, SELECT_RES_HEIGHT);
            }
            if (ret != 0)
            {
                //sysprintf("usbh_set_video_format failed! retry another- 0x%x (%d)\r\n", ret, ret);
                ret = usbh_set_video_format(g_vdev, UVC_FORMAT_MJPEG, SELECT_RES_WIDTH_2, SELECT_RES_HEIGHT_2);
            }
            else
            {
                //sysprintf("usbh_set_video_format OK! \r\n");
            }
#else            
            ret = usbh_set_video_format(g_vdev, UVC_FORMAT_YUY2, SELECT_RES_WIDTH, SELECT_RES_HEIGHT);
            if (ret != 0)
            {
                //sysprintf("usbh_set_video_format failed! retry another- 0x%x (%d)\n", ret, ret);
                ret = usbh_set_video_format(g_vdev, UVC_FORMAT_YUY2, SELECT_RES_WIDTH_2, SELECT_RES_HEIGHT_2);
            }
#endif
            if (ret != 0)
            {
                sysprintf("usbh_set_video_format failed! - 0x%x (%d)\n", ret, ret);
                goto takePhotoeExit;
            }
            //usbh_memory_used();        
            #if(0)
            vTaskDelay(3000/portTICK_RATE_MS); 
            #endif
            
            changeBufferFlag = FALSE;
            init_image_buffers();
            
            /* assign the first image buffer to receive the image from USB */
            usbh_uvc_set_video_buffer(g_vdev, _ig.buff, IMAGE_MAX_SIZE);
            _ig.state = IMAGE_BUFF_USB;
            
            sysprintf("Start UVC %d video streaming...\n", n);
            
            ret = usbh_uvc_start_streaming(g_vdev, uvc_rx_callbak);
            if (ret != 0)
            {
                sysprintf("usbh_uvc_start_streaming failed! - %d\n", ret);
                sysprintf("Please re-connect UVC device...\n"); 
                goto takePhotoeExit;
            }
            #if(1)
                #if(0)
                //if(index == 0)
                {
                    sysprintf("Start UVC %d video Waiting 1000ns...\n");
                    vTaskDelay(1000/portTICK_RATE_MS);   
                }   
                //else
                //{
                //    sysprintf("Start UVC %d video Waiting 2000ns...\n");
                //    vTaskDelay(2000/portTICK_RATE_MS);  
                //}   
                #else
                vTaskDelay(4000/portTICK_RATE_MS); 
                #endif
            //_ig.state  = IMAGE_BUFF_FREE;
            init_image_buffers();
            changeBufferFlag = TRUE;
            #endif
            
        	t0 = get_ticks();

            while ((get_ticks() - t0 < 300) && (_ig.state != IMAGE_BUFF_READY))
            {
                //sysprintf("^\r\n");
                vTaskDelay(100/portTICK_RATE_MS);
            } ;
  
            if (get_ticks() - t0 >= 300)
            {
            	sysprintf("Cannot get image from UVC device %d in 3 seconds!!\n", n);
            	goto takePhotoeExit;
            }
            
            _ig.state = IMAGE_BUFF_POST;

            sysprintf("Stop UVC %d video streaming...\n", n);

            ret = usbh_uvc_stop_streaming(g_vdev);
            if (ret != 0)
            {
                sysprintf("\nusbh_uvc_stop_streaming failed! - %d\n", ret);
                // break;
            }
            
            sysprintf(" ===> Get UVC %d video snaphot...\n", n);

            /* 
             * Get the snapshot 
             */
#ifdef SELECT_MJPEG 
            
            sysprintf(" ~~~ UVCTakePhoto[usb index:%d] OK [%s](photoSize = %d) !!!\r\n", n, fileName, _ig.len);
            *photoLen = _ig.len;
            *photoPr = _ig.buff;

            if(fileName != NULL)
            {
                
                reVal = TRUE;
            }
            else
            {
                //successTimes++;
                
            }

#else
            ret = Encode_JPEG_Image(_ig[_idx_post].buff, _ig[_idx_post].len, snapshot_buff[n], &snapshot_len[n]);
            if (ret == 0)
                sysprintf("Snapshot image encode done.\n");
#endif


        /* Post the frist image */
        //post_snapshot_time = get_ticks();
        //Decode_JPEG_Image(snapshot_buff[0], snapshot_len[0]);
        //post_idx ^= 1;
        
        //USB_Power_Dowm();
   }
takePhotoeExit:
   //USB_Power_Dowm();
   //usbh_suspend();
   #if(1)
   //vTaskDelay(5000/portTICK_RATE_MS);
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
   vTaskDelay(500/portTICK_RATE_MS);
   usbh_pooling_hubs();    /* turn-off VBUS cause device disconnected, this call to detect disconnect */
   usbh_pooling_hubs();
   //usbh_pooling_hubs();
   //vdev = usbh_uvc_get_device_list();
   //if(vdev == NULL)
   //{
   //    sysprintf(" ----- usbh_suspend OK-----\r\n");
   //}
   #if(1)
   _ehci->USBPCR0 = 0x060;                /* disable PHY 0          */
   _ehci->USBPCR1 = 0x020;                /* disable PHY 1          */
   #endif
   //UVCSetPower(0, FALSE);
   //UVCSetPower(1, FALSE);
   //
   //
   sysprintf(" ----- UVCTakePhoto [%d]!!  END -----\r\n", index);
   //vTaskDelay(5000/portTICK_RATE_MS);
   if(reVal)
   {
        BuzzerPlay(50, 100, 1, TRUE);
        #if(1)
        reVal = TRUE;
        #else

        uint8_t* pDate = pvPortMalloc(_ig.len);
        memcpy(pDate, _ig.buff, _ig.len);
        //BOOL dataNeedFreeFlag, BOOL blockFlag, BOOL checkMode
        if(FileAgentAddData(type, dir, fileName, pDate, _ig.len, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, FALSE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
        //if(FileAgentAddData(type, dir, fileName, pDate, _ig.len, FILE_AGENT_ADD_DATA_TYPE_OVERWRITE, TRUE, TRUE, TRUE) !=  FILE_AGENT_RETURN_ERROR )
        {                                
                    //successTimes++;
            reVal = TRUE;
        }
        else
        {
            reVal = FALSE;
            sysprintf(" ~~~ UVCTakePhoto[usb index:%d] WRITE ERROR (photoSize = %d)\r\n", n, _ig.len);
        }
        #endif
        #if(0)
        if(smallSizeFlag)
        {
            if(SCEncryptSAMData(type, dir, fileName, pDate, _ig.len))
            {
            }
        }
        #endif

   }
   else
   {
       BuzzerPlay(50, 100, 2, TRUE);
   }
   xSemaphoreGive(xRunningSemaphore);
   return reVal;
}
void UVCLogStatus(void)
{
    static char logStr[128];
    sprintf(logStr, "-- {UVC counterNumber: %d, %d, power: %d, %d} --\r\n", counterNumber[0], counterNumber[1], powerStatus[0], powerStatus[1]);
    LoglibPrintfEx(LOG_TYPE_INFO, logStr, FALSE);
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


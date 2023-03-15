/**************************************************************************//**
* @file  fepconfig.c
* @version  V1.00
* $Revision:
* $Date:
* @brief
*
* @note
* Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __FEP_CONFIG_H__
#define __FEP_CONFIG_H__

#include "nuc970.h"
#include "VersionMacro.h"
#ifdef __cplusplus
extern "C"
{
#endif

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define MAJOR_VERSION                   1
#define MINOR_VERSION                   9
#define REVISION_VERSION                7
#define BUILD_VERSION                   ((/*(BUILD_YEAR_CH0-'0')*1000 + (BUILD_YEAR_CH1-'0')*100 + */(BUILD_YEAR_CH2-'0')*10 + (BUILD_YEAR_CH3-'0'))*100000000 + \
                                                                                                    ((BUILD_MONTH_CH0-'0')*10 + (BUILD_MONTH_CH1-'0'))*1000000 + \
                                                                                                          ((BUILD_DAY_CH0-'0')*10 + (BUILD_DAY_CH1-'0'))*10000 + \
                                                                                                          ((BUILD_HOUR_CH0-'0')*10 + (BUILD_HOUR_CH1-'0'))*100 + \
                                                                                                        ((BUILD_MIN_CH0-'0')*10 + (BUILD_MIN_CH1-'0'))*1) 
#define BUILD_VERSION_EX              BUILD_VERSION  
#define FREERTOS_USE_1000MHZ            0     
#define USER_NEW_FATFS                  0      
#define ENABLE_MODEM_CMD_DEBUG          0
#define ENABLE_MODEM_FLOW_CONTROL       0 
    
#define BUILD_RELEASE_VERSION           0  
#define BUILD_PRE_RELEASE_VERSION       0 
#define BUILD_DEBUG_VERSION             0
#define BUILD_HW_TESTER_VERSION         1
    
#define ENABLE_LOG_FUNCTION             1 
#define ENABLE_BURNIN_TESTER            1
#define ENABLE_MTP_FUNCTION             1
#define SUPPORT_HK_10_HW                1//by Jer
//=================================================
#if(0)
    //UserDrv
    #define ENABLE_LED_DRIVER               1
    #define ENABLE_BATTERY_DRIVER           1   
    #define ENABLE_BUZZER_DRIVER            1    
    //SysHal
    #define ENABLE_GUI_DRIVER               1
    #define ENABLE_POWER_DRIVER             1  
    #define ENABLE_YAFFS2_DRIVER            0  
    #define ENABLE_FATFS_DRIVER             1     
    #define ENABLE_EPD_DRIVER               1   
    #define ENABLE_CARD_READER_DRIVER       1 
    #define ENABLE_SPACE_DRIVER             1 
    #define ENABLE_MODEM_AGENT_DRIVER       0 
    #define ENABLE_DATA_AGENT_DRIVER        0     
    #define ENABLE_TAKE_PHOTO_DRIVER        1
    #define ENABLE_PHOTO_AGENT_DRIVER       1
    //User 
    #define ENABLE_USER_DRIVER              1 
    #define ENABLE_PARA_LIB                 1  
    #define ENABLE_GUI_MANAGER              1
    #define ENABLE_POWER_DOWN               0
    
#else
    //UserDrv
    #define ENABLE_LED_DRIVER               1
    #define ENABLE_BATTERY_DRIVER           1   
    #define ENABLE_BUZZER_DRIVER            1    
    //SysHal
    #define ENABLE_GUI_DRIVER               1
    #define ENABLE_POWER_DRIVER             1  
    #define ENABLE_YAFFS2_DRIVER            0   
    #define ENABLE_FATFS_DRIVER             1     
    #define ENABLE_EPD_DRIVER               1   
    #define ENABLE_CARD_READER_DRIVER       1 
    #define ENABLE_SPACE_DRIVER             1 
    #define ENABLE_MODEM_AGENT_DRIVER       1 
    #define ENABLE_DATA_AGENT_DRIVER        0     
    #define ENABLE_TAKE_PHOTO_DRIVER        1
    #define ENABLE_PHOTO_AGENT_DRIVER       1
    //User 
    #define ENABLE_USER_DRIVER              1 
    #define ENABLE_PARA_LIB                 1  
    #define ENABLE_GUI_MANAGER              1
    #define ENABLE_POWER_DOWN               1
#endif

//=================================================

#define DEFAULT_TARIFF_FILE_FTP_PATH            "tariffdir"
#define DEFAULT_PARA_FILE_FTP_PATH              "paradir"
#define DEFAULT_DCF_FILE_FTP_PATH               "dcfdsfdir"
#define DEFAULT_DSF_FILE_FTP_PATH               "dcfdsfdir"
#define DEFAULT_JPG_FILE_FTP_PATH               "jpgdir"
#define DEFAULT_LOG_FILE_FTP_PATH               "logdir"

//#define WEB_POST_ADDRESS                "HTTP://118.163.153.124:3000/API/getInfo?"
#define WEB_POST_ADDRESS              "http://210.17.120.42/DCU/api/"  //"HTTP://54.249.1.95:7878/?"
#if(1)
    #if(1)
        //#define FTP_ADDRESS                     "118.163.153.124"
        //#define FTP_PORT                        "4000"
        #define FTP_ADDRESS                     "ftp.green-ideas.com.tw" //"vpn3rgitcam.asuscomm.com" //"36.230.240.241"  //"ftp.green-ideas.com.tw"
        #define FTP_PORT                        "21"  //"9000" 

    #else
        #define FTP_ADDRESS                     "54.250.171.197"//"118.163.153.124"
        //#define FTP_PORT                        "21"//"4000"
        #define FTP_PORT                        "4000"
    #endif
    #define FTP_ID                          "parking"  //"BurnTest"             //"test"
    #define FTP_PASSWD                      "git123456"  //"git123456789"         //"123456"
    #define FTP_PRE_PATH                    "test/"                   //"home/test/"  

#else
    #define FTP_ADDRESS                     "54.249.1.95"
    #define FTP_PORT                        "21"
    #define FTP_ID                          "sam"
    #define FTP_PASSWD                      "samsam"
    #define FTP_PRE_PATH                    "home/test/"  //""  
#endif


#define BASE_DATA_DIR                   "1:"    

//#define FILE_SAVE_POSITION   FILE_AGENT_STORAGE_TYPE_FATFS
#define FILE_SAVE_POSITION   FILE_AGENT_STORAGE_TYPE_AUTO

#define EPM_ID_SAVE_POSITION            FILE_AGENT_STORAGE_TYPE_FATFS
#define EPM_ID_FILE_DIR                 "0:"
#define EPM_ID_FILE_NAME                "epm.id"

#define EPM_STORAGE_SAVE_POSITION       FILE_SAVE_POSITION
#define EPM_STORAGE_FILE_DIR            "1:"
#define EPM_STORAGE_FILE_NAME           "epmstorage.dat"
    
#define EPM_PARA_SAVE_POSITION          FILE_SAVE_POSITION
#define EPM_PARA_FILE_DIR               "1:"
#define EPM_PRAR_FILE_EXTENSION         "pre"
#define EPM_PARA_FILE_NAME              "epm.pre"
    
#define TARIFF_FILE_SAVE_POSITION       FILE_SAVE_POSITION
#define TARIFF_FILE_DIR                 "1:"
#define TARIFF_FILE_EXTENSION           "tre"   

#define LOG_SAVE_POSITION               FILE_SAVE_POSITION
#define LOG_FILE_EXTENSION              "log"
#define LOG_FILE_DIR                    "1:"
    
#define PHOTO_SAVE_POSITION             FILE_SAVE_POSITION
#define PHOTO_FILE_EXTENSION            "jpg"
#define PHOTO_FILE_DIR                  "1:"   

#define DSF_FILE_SAVE_POSITION          FILE_SAVE_POSITION
#define DSF_FILE_EXTENSION              "dsf"
#define DSF_FILE_DIR                    "1:"  

#define DCF_FILE_SAVE_POSITION          FILE_SAVE_POSITION
#define DCF_FILE_EXTENSION              "dcf"
#define DCF_FILE_DIR                    "1:"
        
#define FILE_EXTENSION_EX(e)            "*."e  



   
    
//SysHal
#define DRV_INIT_THREAD_PROI            (configMAX_PRIORITIES-1)
#define POWER_THREAD_PROI               (configMAX_PRIORITIES-2)
#define NT066E_DRV_THREAD_PROI          (configMAX_PRIORITIES-3)
#define DIP_THREAD_PROI                 NT066E_DRV_THREAD_PROI
#define TIMER_DRV_0_THREAD_PROI         (NT066E_DRV_THREAD_PROI - 1)
#define TIMER_DRV_1_THREAD_PROI         (TIMER_DRV_0_THREAD_PROI - 1)
#define TIMER_DRV_2_THREAD_PROI         (TIMER_DRV_0_THREAD_PROI - 2)
#define BUZZER_DRV_THREAD_PROI          (TIMER_DRV_0_THREAD_PROI - 1)
#define CARD_READER_THREAD_PROI         (TIMER_DRV_2_THREAD_PROI - 1)
#define SPACE_DRV_THREAD_PROI           4
#define BATTERY_THREAD_PROI             3
#define DEBUG_THREAD_PROI               (configMAX_PRIORITIES-1)//1
//#define DATA_AGENT_RX_THREAD_PROI               8 
//#define DATA_AGENT_TX_THREAD_PROI               7 
//#define DATA_AGENT_ROUTINE_THREAD_PROI          6 
//#define MODEM_AGENT_RX_THREAD_PROI               8 
#define MODEM_AGENT_TX_THREAD_PROI               3 
#define MODEM_AGENT_ROUTINE_THREAD_PROI          7 
#define METER_DATA_PROCESS_THREAD_PROI          6 
#define METER_TAKE_PHOTO_PROCESS_THREAD_PROI    5 
#define EPD_BACK_LIGHT_THREAD_PROI              1 
#define LED_THREAD_PROI                         1 
#define GUI_DRAWING_PROI                        1
#define RADAR_THREAD_PROI                       2 
#define FILE_AGENT_THREAD_PROI                  5//2


#if (ENABLE_BURNIN_TESTER)
#define GUI_MANAGER_THREAD_PROI                 20
#define LED_BUZZER_TEST_THREAD_PROI             3
#define BATTERY_TEST_THREAD_PROI                2
#define MODEM_TEST_THREAD_PROI                  2
#define SMART_CARD_TEST_THREAD_PROI             2
#define NAND_FLASH_TEST_THREAD_PROI             3
#define CARD_READER_TEST_THREAD_PROI            3
#define SD_CARD_TEST_THREAD_PROI                4
#define CAMERA_TEST_THREAD_PROI                 5
#endif


//UserDrv
//#define PTC_CAM_THREAD_PROI             5
//#define UART10_THREAD_PROI              5

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif//__FEP_CONFIG_H__

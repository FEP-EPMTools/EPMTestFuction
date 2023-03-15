/**************************************************************************//**
* @file     rb60pOTA.h
* @version  V1.00
* $Revision: 
* $Date: 
* @brief    
*
* @note
* Copyright (C) 2020 Far Easy Pass LTD. All rights reserved.
*****************************************************************************/

#ifndef __RB60P_OTA_H__
#define __RB60P_OTA_H__

#include "nuc970.h"
#include "interface.h"
#ifdef __cplusplus
extern "C"
{
#endif
    
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
    /*-----Device_memory------*/
#define FLASH_OFFS     0x00000400     // offset used for cached Flash
  
    
typedef struct TableEntry
{
	unsigned int dwStartAddr;
	unsigned int dwSize;

} TableEntry;  
    

    
typedef enum device_type_t {
  XMC4500_1024_DEVICE         = 0x1,
  XMC4500_768_DEVICE		  = 0x2,
  XMC4500_512_DEVICE	      = 0x3,
  XMC4400_512_DEVICE	      = 0x4,
  XMC4200_256_DEVICE	      = 0x5,
  XMC4800_1024_DEVICE	      = 0x6,
  XMC4800_1536_DEVICE	      = 0x7,
  XMC4800_2048_DEVICE	      = 0x8,
} device_type_t;
    
    /*-----XMCLoader_API------*/
#define UART_MAX_READ_TIMEOUT          20
#define UART_DEFAULT_READ_TIMEOUT      10
#define UART_MIN_READ_TIMEOUT          5
#define	CAN_DATA_MESSAGE_ID			   0x03

#define DATA_BYTE_TO_LOAD		       0x100

#define ASC_INTERFACE			       0x01
#define CAN_INTERFACE			       0x02    
    

//#define READ_FLASH_PROTECTION_STATUS_WAITTIME   100
#define OTHER_HEADER_WAITTIME                   100
#define ERASE_FLASH_SECTOR_WAITTIME             5000
//#define PROGRAM_FLASH_HEADER_WAITTIME           10
#define PROGRAM_FLASH_DATA_WAITTIME             10
#define PROGRAM_FLASH_EOT_WAITTIME              10



typedef enum bsl_error_codes_t {
  BSL_NO_ERROR                 = 0x00,
  ERROR_BSL_BLOCK_TYPE	       = 0xFF,
  ERROR_BSL_MODE 		       = 0xFE, 
  ERROR_BSL_CHECKSUM           = 0xFD,
  ERROR_BSL_ADDRESS 	       = 0xFC,
  ERROR_BSL_ERASE		       = 0xFB,
  ERROR_BSL_PROGRAM	           = 0xFA,
  ERROR_BSL_VERIFICATION       = 0xF9,
  ERROR_BSL_PROTECTION		   = 0xF8,
  ERROR_BSL_BOOTCODE	       = 0xF7,
  ERROR_BSL_HEXDOWNLOAD		   = 0xF6,
  ERROR_BSL_UNKNOWN            = 0xF0,
  ERROR_BSL_COMINIT            = 0xF3,	
  ERROR_BSL_INIT               = 0xF2,
  ERROR_HEXFILE				   = 0xF1,
  ERROR_CANBUS			       = 0xE9
} bsl_error_codes_t;

typedef struct BSL_HEADER{
	unsigned char mode;
	unsigned int startAddress;
	unsigned int sectorSize;
	unsigned int userPassword1;
	unsigned int userPassword2;	
	unsigned char flashModule;
	unsigned short protectionConfig;
    int WaitTime;

} BSL_HEADER;

typedef struct BSL_DATA {
  unsigned char *cDataArray;    // Pointer to the data to be loaded.
	unsigned char verification;
} BSL_DATA;

typedef struct BSL_DOWNLOAD {
	BOOL verbose;
	unsigned char device;
	unsigned char ser_interface;
	BOOL pFlash;
	char *hexFileName;
} BSL_DOWNLOAD;    
    
    
/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/    

BOOL RB60POTAFunc(int radarIndex);

#ifdef __cplusplus
}
#endif

#endif //__RB60P_OTA_H__
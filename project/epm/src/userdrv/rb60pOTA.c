/**************************************************************************//**
* @file     rb60pOTA.c
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
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "osmisc.h"
#include "fepconfig.h"
#include "interface.h"
#include "radardrv.h"
#include "rb60pOTA.h"

#include "ff.h"
#include "fatfslib.h"
#include "guidrv.h"
#include "guimanager.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
/*
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
*/

const TableEntry XMC4400_512_PFLASH_SectorTable[]=
{
	{0x0C000000, 0x4000},
	{0x0C004000, 0x4000},
	{0x0C008000, 0x4000},
	{0x0C00C000, 0x4000},
	{0x0C010000, 0x4000},
	{0x0C014000, 0x4000},
	{0x0C018000, 0x4000},
	{0x0C01C000, 0x4000},
	{0x0C020000, 0x20000},
	{0x0C040000, 0x40000},  //0xC040000-0xC07FFFF, 512KB
	{0,0}
};


/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/

//#define MAX_BUFFER 20


//unsigned int dwBaudrate;
static unsigned int appLength = 7791;
//unsigned int dwTickCount;
//char comPort[MAX_BUFFER];
static unsigned int hex_address, num_of_bytes;
//char hexFileName[MAX_BUFFER];

static unsigned char hexArray[16*1024];
//char LoaderPath[100];
static BSL_HEADER   bslHeader;
static BSL_DOWNLOAD bslDownload;
//unsigned int result;
//unsigned int choice;
//unsigned int mem_type;
//unsigned int device;
static unsigned int ser_interface = ASC_INTERFACE;
/*unsigned int hexfile_type;
unsigned int password1;
unsigned int password2;

unsigned char chRead[11];
*/
/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

static void charToInt(unsigned int *intOut, char *charLine, unsigned numOfChar) {
  unsigned i;
  char     tmpChar;
  
  *intOut = 0;
  for (i=0; i < numOfChar; i++) {
    if (charLine[i] >= 0x41 && charLine[i] <= 0x46) {
      // Check for 0xA - 0xF
      tmpChar = charLine[i] - 0x37;
    } else if (charLine[i] >= 0x61 && charLine[i] <= 0x66) {
      // Check for 0xa - 0xf
      tmpChar = charLine[i] - 0x37;
    } else {
      tmpChar = charLine[i] & 0xF;
    }
    *intOut |= (tmpChar << (((numOfChar - 1) - i) * 4));
  }
}


static BOOL RB60PStartBootModeFunc(int radarIndex)
{
    uint8_t RadarData[22];
    uint8_t FirstOTACmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x05, 0x00, 0x0E, 0xD3, 0x3D};
    int ReadDataLen;
    
    if(RadarDrvInitEx(radarIndex,115200) == FALSE)
    {
        terninalPrintf("NEWradarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    newRadarResultPure(radarIndex,NULL,0,RadarData,&ReadDataLen,100);
    //RadarDrvInit();
    RadarSetPowerStatus(radarIndex,FALSE);
    vTaskDelay(500/portTICK_RATE_MS);
    RadarSetPowerStatus(radarIndex,TRUE);
    vTaskDelay(500/portTICK_RATE_MS);
    
    
    
    if(newRadarResultPure(radarIndex,FirstOTACmd,sizeof(FirstOTACmd),RadarData,&ReadDataLen,100) == TRUE)
    {
        if(RadarData[0] == 0x00)
            return TRUE;
    }
    terninalPrintf("First OTA return FALSE\n");
    return FALSE; 
    
}

static BOOL RB60PQueryVersionFunc(int radarIndex)
{
    uint8_t RadarData[22];
    uint8_t VersionCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D};
    int ReadDataLen;
    uint8_t reFreshPara;
    
    if(RadarDrvInitEx(radarIndex,115200) == FALSE)
    {
        terninalPrintf("NEWradarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    //newRadarResultPure(radarIndex,NULL,0,RadarData,&ReadDataLen,100);
    //RadarDrvInit();
    RadarSetPowerStatus(radarIndex,FALSE);
    vTaskDelay(500/portTICK_RATE_MS);
    RadarSetPowerStatus(radarIndex,TRUE);
    vTaskDelay(500/portTICK_RATE_MS);
    
    
    
    if(newRadarResult(radarIndex, 0x00, VersionCmd, RadarData) == TRUE)
    {
        if(radarIndex == 0)
            reFreshPara = GUI_NEW_RADARA_OTA;
        else if(radarIndex == 1)
            reFreshPara = GUI_NEW_RADARB_OTA;
        
        GuiManagerUpdateMessage(reFreshPara,0xFF,(RadarData[0]<<24) | (RadarData[1]<<16) | (RadarData[2]<<8) | RadarData[3]);        
        return TRUE;
    }
    else
        return FALSE; 
    
}

static BOOL RB60PSetOTABaudFunc(int radarIndex)
{
    if(RadarDrvInitEx(radarIndex ,57600) == FALSE)
    {
        terninalPrintf("NEWradarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    return TRUE;
}
/*
static BOOL RB60PSetVersionBaudFunc(int radarIndex)
{
    if(RadarDrvInitEx(radarIndex ,115200) == FALSE)
    {
        terninalPrintf("NEWradarTest ERROR (initFunc false)!!\n");
        return FALSE;
    }
    return TRUE;
}
*/

static BOOL init_ASC_BSL(int radarIndex)
{
    uint8_t RadarData[22];
    uint8_t initASCCmd[1] = {0x00};
    int ReadDataLen;
    
    if(newRadarResultPure(radarIndex,initASCCmd,1,RadarData,&ReadDataLen,100) == TRUE)
    {
        if(RadarData[0] == 0xD5)
            return TRUE;
    }
    terninalPrintf("init_ASC_BSL FALSE,RadarData[0] = 0x%02x\n",RadarData[0]);
    return FALSE; 

}

static BOOL send_4_length(int radarIndex , unsigned int appLength)
{
    uint8_t RadarData[22];
    uint8_t appLengthCmd[4];
    int ReadDataLen;
    
    appLengthCmd[0] = appLength & 0xff; //1. byte
    appLengthCmd[1] = (appLength>>8) & 0xff; //2. byte
    appLengthCmd[2] = (appLength>>16) & 0xff; //3. byte
    appLengthCmd[3] = (appLength>>24) & 0xff; //3. byte
    
    if(newRadarResultPure(radarIndex,appLengthCmd,4,RadarData,&ReadDataLen,100) == TRUE)
    {
        if(RadarData[0] == 0x01)
            return TRUE;
    }
    terninalPrintf("send_4_length FALSE\n");
    return FALSE; 

}


static BOOL make_flash_image(char *hexFileName,unsigned char *image,unsigned int max_size, unsigned int *address, unsigned int *num_of_bytes) {

    FIL file;
    UINT br;  
    
    char hexLine[80]; 
    unsigned int hexCount, oldhexCount, hexAddress, oldhexAddress, hexType;  
    unsigned int intData;

    char *hexData;
    unsigned int i;
    unsigned int temp_addr;
    BOOL address_set = FALSE;
    BOOL prev_addressType = FALSE;
    BOOL LinearBaseAddr = FALSE;

    if (max_size > 0xFFFF)
        return  FALSE;   
    
    if(f_open(&file, hexFileName, FA_OPEN_EXISTING |FA_READ))
    {
        terninalPrintf("SD card file %s open fail.\r\n",hexFileName);
        f_close(&file);
        return FALSE;
    }
	
    *num_of_bytes=0;

    hexAddress = 0;
    hexCount   = 0;

    while(f_gets(hexLine, 80, &file)!=NULL) { 
        
        oldhexCount = hexCount;
        oldhexAddress = hexAddress;

        // Hex Count
        charToInt(&hexCount, &hexLine[1], 2);

        // Hex Address		
        charToInt(&hexAddress, &hexLine[3], 4);

        if (LinearBaseAddr == FALSE) {
            for (i=oldhexAddress + oldhexCount; i<hexAddress; i++)
                *image++ = 0x00;
        }
        else LinearBaseAddr = FALSE;

        // Hex Type
        charToInt(&hexType, &hexLine[7], 2);
        hexData = &hexLine[9];

        if (hexType == 0) {
                if (prev_addressType == TRUE) {
                    temp_addr|=(hexAddress & 0x0000FFFF);
                    prev_addressType=FALSE;
                }
          for(i=0; i < hexCount; i++) {

            charToInt(&intData, &hexData[(2*i)], 2); 

                    *image++ = intData & 0xFF;			
          }
                *num_of_bytes+=hexCount;
        }
            if ((hexType == 4) && (prev_addressType == FALSE) && (address_set == FALSE) ){
                charToInt(&temp_addr,&hexLine[9],4);
                temp_addr=(temp_addr & 0x0000FFFF)<<16;
                address_set = TRUE;
                prev_addressType = TRUE;
                LinearBaseAddr = TRUE;
            }
    }
    if ((*num_of_bytes > max_size) || (*num_of_bytes == 0))
        return FALSE;

    while (*num_of_bytes < 7168){        //fill the minimon to 8kbyte
      *image = 0x00;
      image = image++;
      (*num_of_bytes)++;
    }

    *address=temp_addr;
    f_close(&file);

  return TRUE;	
}

static BOOL send_ASCloader(int radarIndex , unsigned char *hexArray, unsigned int size)
{
    uint8_t RadarData[22];
    uint8_t appLengthCmd[4];
    int ReadDataLen;
    
    
    if(newRadarResultPure(radarIndex,hexArray,size,RadarData,&ReadDataLen,100) == TRUE)
    {
        if(RadarData[0] == 0x01)
            return TRUE;
    }
    terninalPrintf("send_ASCloader FALSE\n");
    return FALSE; 

}


static BOOL bl_send_header(int radarIndex , BSL_HEADER bslHeader)
{
    uint8_t RadarData[22];
    int ReadDataLen;
    int WaitTime;
	unsigned char chWrite[16];
	unsigned char chRead[8];
	unsigned char chksum = 0;
	unsigned int  i;
	DWORD dwNumOfBytes = 0;

	chWrite[0] = 0x00;
	chWrite[1] = bslHeader.mode;
	chWrite[2] = (bslHeader.startAddress & 0xFF000000) >> 24;
	chWrite[3] = (bslHeader.startAddress & 0x00FF0000) >> 16;
	chWrite[4] = (bslHeader.startAddress & 0x0000FF00) >> 8;
	chWrite[5] = (bslHeader.startAddress & 0x000000FF);
	chWrite[6] = (bslHeader.sectorSize & 0xFF000000) >> 24;
	chWrite[7] = (bslHeader.sectorSize & 0x00FF0000) >> 16;
	chWrite[8] = (bslHeader.sectorSize & 0x0000FF00) >> 8;
	chWrite[9] = (bslHeader.sectorSize & 0x000000FF);
	chWrite[10] = 0x00;
	chWrite[11] = 0x00;
	chWrite[12] = 0x00;
	chWrite[13] = 0x00;
	chWrite[14] = 0x00;

	if (bslHeader.mode == 4) 
		chWrite[10] = bslHeader.flashModule; //flash module


	for (i=1; i<15; i++)
		chksum = chksum ^ chWrite[i];

	chWrite[15] = chksum;
    
    if(bslHeader.mode == 3)
        WaitTime = ERASE_FLASH_SECTOR_WAITTIME;
    else
        WaitTime = OTHER_HEADER_WAITTIME;
    
    if(newRadarResultPure(radarIndex,&chWrite[0],16,RadarData,&ReadDataLen,WaitTime) == TRUE)
    {
        if(RadarData[0] == 0x55)
            return TRUE;
    }
    terninalPrintf("bl_send_header FALSE\n");
    return FALSE; 
    
}


static BOOL bl_send_EOT(int radarIndex)
{
    uint8_t RadarData[22];
    int ReadDataLen;
    
	unsigned char chWrite[16];
	unsigned char chRead[8];
	unsigned char chksum = 0;
	unsigned int i;
	DWORD dwNumOfBytes = 0;

	chWrite[0] = 0x02;
	for (i=1; i<15; i++) {
		chWrite[i] = 0x00; //not used
		chksum ^= chWrite[i];
	}

	chWrite[15] = chksum;
    
    if(newRadarResultPure(radarIndex,&chWrite[0],16,RadarData,&ReadDataLen,PROGRAM_FLASH_EOT_WAITTIME) == TRUE)
    {
        if(RadarData[0] == 0x55)
            return TRUE;
    }
    terninalPrintf("bl_send_EOT FALSE\n");
    return FALSE; 
    
}


static BOOL bl_send_data(int radarIndex, BSL_DATA bslData)
{
    uint8_t RadarData[22];
    int ReadDataLen;
	unsigned char chWrite[DATA_BYTE_TO_LOAD+8];
	unsigned char chRead[4];
	unsigned char chksum = 0;
	unsigned int i;
	DWORD dwNumOfBytes = 0;

	chWrite[0] = 0x01;
	chWrite[1] = bslData.verification;

	for (i=0; i<DATA_BYTE_TO_LOAD; i++) {
		chWrite[i+2] = bslData.cDataArray[i];		
	}
	for (i=DATA_BYTE_TO_LOAD+2; i<DATA_BYTE_TO_LOAD+7; i++)
		chWrite[i] = 0x00; //not used

	for (i=1; i<DATA_BYTE_TO_LOAD+7; i++)
		chksum = chksum ^ chWrite[i];

	chWrite[DATA_BYTE_TO_LOAD+7] = chksum; 

    if(newRadarResultPure(radarIndex,&chWrite[0],DATA_BYTE_TO_LOAD+8,RadarData,&ReadDataLen,PROGRAM_FLASH_DATA_WAITTIME) == TRUE)
    {
        if(RadarData[0] == 0x55)
            return TRUE;
    }
    terninalPrintf("bl_send_data FALSE\n");
    return FALSE; 	
}


static BOOL bl_erase_flash(int radarIndex, BSL_DOWNLOAD bslDownload)
{
    FIL file;
    UINT br;  
    
    char hexLine[80];
    unsigned int hexCount, hexAddress, hexType;  
    unsigned int temp_addr = 0;
    unsigned int sectorCounter, erasedSectors;	
    unsigned int result;
    const TableEntry *sectorTable;
    const TableEntry *sectorTableStart;
    BSL_HEADER bslHeader;


    if(f_open(&file, bslDownload.hexFileName, FA_OPEN_EXISTING |FA_READ))
    {
        terninalPrintf("SD card file open fail.\r\n");
        f_close(&file);
        return FALSE;
    }
    
    sectorCounter = 0;

    hexAddress = 0;
    hexCount	 = 0;
    erasedSectors = 0;	

    if (bslDownload.pFlash == TRUE)
        sectorTableStart = &XMC4400_512_PFLASH_SectorTable[0];

    
    while(f_gets(hexLine, 80, &file)!=NULL) { 
        
    // Hex Count
    charToInt(&hexCount, &hexLine[1], 2);
    // Hex Address
    charToInt(&hexAddress, &hexLine[3], 4);

    // Hex Type
    charToInt(&hexType, &hexLine[7], 2);

    if (hexType == 4){
        charToInt(&temp_addr,&hexLine[9],4); // get the upper 16 bits of the 32 bit absolute address for all subsequent type 00 records
        if((temp_addr & ~0xff) == 0x00000800) // if segment belongs to cache area
            temp_addr = temp_addr + FLASH_OFFS;  //if codes in iCache
        temp_addr=(temp_addr & 0x0000FFFF)<<16;
    }

    if (hexType == 0) {
            //Address tag in Hexfile
            temp_addr &= 0xFFFF0000;
            temp_addr |=(hexAddress & 0x0000FFFF);
                    
            //find sector according to hexaddress
            sectorCounter = 0;
            sectorTable = sectorTableStart;
            while (sectorTable->dwStartAddr) {
                if ((temp_addr>=sectorTable->dwStartAddr) &&
                        (temp_addr<sectorTable->dwStartAddr+sectorTable->dwSize))
                    break;
                sectorCounter++;
                sectorTable++;
            }
            if (sectorTable->dwStartAddr == 0) //no according sector to requested address found
                return  FALSE;

            //sector already erased?
            if (!(erasedSectors & (1<<sectorCounter))) { //No

                //Erase sector this sector
                bslHeader.mode				 = 3; //Erase mode
                bslHeader.startAddress = sectorTable->dwStartAddr;
                bslHeader.sectorSize	 = sectorTable->dwSize;

                if (bslDownload.verbose)
                    terninalPrintf("\nErasing sector 0x%08X (This may take a few seconds)... ",bslHeader.startAddress);

                if (bslDownload.ser_interface == ASC_INTERFACE)
                    result = bl_send_header(radarIndex,bslHeader);
                else
                    return FALSE;
            
                //mark sector as erased
                erasedSectors |= (1<<sectorCounter);

                if (bslDownload.verbose)
                    terninalPrintf("done\n");
            }		

            //crossing a sector border?
            if (temp_addr+hexCount > sectorTable->dwStartAddr + sectorTable->dwSize){
                sectorCounter++;
                sectorTable++;
            }
            if (sectorTable->dwStartAddr == 0) //no according sector to requested address found
                return  FALSE;

            //sector already erased?
            if (!(erasedSectors & (1<<sectorCounter))) { //No

                //Erase sector this sector
                bslHeader.mode				 = 3; //Erase mode
                bslHeader.startAddress = sectorTable->dwStartAddr;
                bslHeader.sectorSize	 = sectorTable->dwSize;

                if (bslDownload.verbose)
                    terninalPrintf("\nErasing sector 0x%08X (This may take a few seconds)... ",bslHeader.startAddress);

                if (bslDownload.ser_interface == ASC_INTERFACE)
                    result = bl_send_header(radarIndex,bslHeader);
                else
                    return FALSE;
            
                //mark sector as erased
                erasedSectors |= (1<<sectorCounter);

                if (bslDownload.verbose)
                    terninalPrintf("done\n");
            }
        }
    }

    f_close(&file);

    return TRUE;
	
}


static BOOL bl_download_pflash(int radarIndex, BSL_DOWNLOAD bslDownload)
{
    FIL file;
    UINT br;  
    unsigned int BlockCount;
    unsigned int BlockIndex = 0;
    unsigned int milestone;
    int TotalCostTime;
    int progress = 0;
    uint8_t reFreshPara;
    
    //FILE *hexFile; /* Declares a file pointer */
    char hexLine[80];
    unsigned char hexArray[0x400];
    unsigned char *hexArrayPtr;
    unsigned int hexCount, oldhexCount, hexAddress, oldhexAddress, hexType;  
    unsigned int intData;
    char *hexData;
    unsigned int temp_addr, old_temp_addr;
    unsigned int page_addr;
    unsigned int offset;
    unsigned int i,j, result;
    unsigned int sectorCounter, erasedSectors;
    BOOL firstTime;
    unsigned int num_of_bytes;
    unsigned char writeBuffer[DATA_BYTE_TO_LOAD];
    BSL_HEADER bslHeader;
    BSL_DATA bslData;
    BOOL prev_address_type;


    bslDownload.pFlash = TRUE;
    
    if(radarIndex == 0)
        reFreshPara = GUI_NEW_RADARA_OTA;
    else if(radarIndex == 1)
        reFreshPara = GUI_NEW_RADARB_OTA;
    
    GuiManagerUpdateMessage(reFreshPara,progress,0);
    terninalPrintf("progress = %d%%\r",progress);
    //if Hex file is too big, bl_erase_flash will return an error.
    if ((result=bl_erase_flash(radarIndex,bslDownload)) != TRUE)
        return result;

    if(f_open(&file, bslDownload.hexFileName, FA_OPEN_EXISTING |FA_READ))
    {
        terninalPrintf("SD card file open fail.\r\n");
        f_close(&file);
        return FALSE;
    }
    
    //terninalPrintf("file.fsize = %d\r\n",file.fsize);
    BlockCount = file.fsize * 32 / 45  / 2  /  256 ;
    //terninalPrintf("BlockCount = %d\r\n",BlockCount);
    TotalCostTime = ERASE_FLASH_SECTOR_WAITTIME*2 + PROGRAM_FLASH_DATA_WAITTIME*BlockCount;
    //terninalPrintf("TotalCostTime = %d\r\n",TotalCostTime);
    
    milestone = BlockCount/8 ;
    
    /* Open the existing file specified for reading */
    /* Note the use of \\ for path separators in text strings */
    /*hexFile = fopen(bslDownload.hexFileName, "rt");
    if (hexFile == NULL)      
    return  ERROR_HEXFILE; 
    */
    num_of_bytes=0;
    sectorCounter = 0;

    hexArrayPtr = hexArray;

    hexAddress = 0;
    hexCount	 = 0;
    firstTime = TRUE;
    erasedSectors = 0;
    bslData.cDataArray = writeBuffer;
    bslData.verification = 0x01; //enable verification
    prev_address_type = FALSE;

    old_temp_addr = 0;
    temp_addr	  = 0;

    while(f_gets(hexLine, 80, &file)!=NULL) { 
        
        oldhexCount = hexCount;
        oldhexAddress = hexAddress;

        // Hex Count
        charToInt(&hexCount, &hexLine[1], 2);
        // Hex Address
        charToInt(&hexAddress, &hexLine[3], 4);

        // Hex Type
        charToInt(&hexType, &hexLine[7], 2);
        hexData = &hexLine[9];

            offset = 0;
            
        if (hexType == 4){			
            charToInt(&temp_addr,&hexLine[9],4);
            if(temp_addr == 0x00000800)
                 temp_addr = temp_addr + FLASH_OFFS;
            temp_addr=(temp_addr & 0x0000FFFF)<<16;
            prev_address_type = TRUE;
        }

        if (hexType == 0) {			
                //Address tag in Hexfile
                temp_addr &= 0xFFFF0000;
                temp_addr |= (hexAddress & 0x0000FFFF);

                
                //Address discontinious?
                if (temp_addr - old_temp_addr >= 0x200) { 

                    if (firstTime==FALSE) {

                        if (prev_address_type == FALSE) {
                                    
                            //send remaining data
                            for (j=0; j<num_of_bytes; j++)
                                writeBuffer[j] = hexArray[j];

                            for (j=num_of_bytes; j<DATA_BYTE_TO_LOAD; j++)
                                writeBuffer[j] = 0x00;
                            

                            if((BlockIndex % milestone) == 0)
                            {
                                //sysprintf("80 * BlockIndex / BlockCount = %d\r\n",80 * BlockIndex / BlockCount);
                                progress = 80 * BlockIndex / BlockCount +20;
                                //progress = progress +20;
                                if(progress > 100)
                                    progress = 100;
                                GuiManagerUpdateMessage(reFreshPara,progress,0);
                                terninalPrintf("progress = %d%%\r",progress);
                            }
                            BlockIndex++;
                            
                            if (bslDownload.verbose)
                                terninalPrintf("\nProgramming data block to address 0x%08X... ",page_addr);
                            
                            if (bslDownload.ser_interface == ASC_INTERFACE)
                                result = bl_send_data(radarIndex,bslData);
                            else
                                return FALSE;

                            page_addr += DATA_BYTE_TO_LOAD;

                            if (bslDownload.verbose)
                                terninalPrintf("done");
                        }

                        //send EOT frame
                        if (bslDownload.ser_interface == ASC_INTERFACE)
                            result = bl_send_EOT(radarIndex);
                        else
                            return FALSE;
                    }
                    
                    //check if address is 256-byte aligned
                    offset=temp_addr & 0xFF;

                    if (offset) {
                        temp_addr -= offset;
                    }
                    
                    //send program header
                    bslHeader.mode		   = 0;
                    bslHeader.startAddress = temp_addr;

                    if (bslDownload.ser_interface == ASC_INTERFACE)
                        result = bl_send_header(radarIndex,bslHeader);
                    else
                        return FALSE;

                    hexArrayPtr  = &hexArray[0];
                    num_of_bytes = 0;

                    if (offset) {
                        num_of_bytes = offset;
                        for (j=0; j<offset; j++)
                            *hexArrayPtr++ = 0x00;
                    }

                    page_addr	= temp_addr;
                    old_temp_addr = temp_addr;


                    oldhexAddress = hexAddress;
                    oldhexCount	  = hexCount;
                    firstTime = FALSE;
                    
                } 

                if (prev_address_type == TRUE) {
                    oldhexCount = hexCount;
                    oldhexAddress = hexAddress;
                    prev_address_type = FALSE;
                }
                
                //fill the page
                for (i=oldhexAddress + oldhexCount; i<hexAddress; i++) {
                    *hexArrayPtr++ = 0x00;
                    num_of_bytes ++;
                }

                for(i=0; i < hexCount; i++) {

                    charToInt(&intData, &hexData[(2*i)], 2); 
                    
                    *hexArrayPtr++ = intData & 0xFF;
                }

                num_of_bytes+=hexCount;

                //page full?
                if (num_of_bytes >= DATA_BYTE_TO_LOAD) {

                    //send data block
                    for (j=0; j<DATA_BYTE_TO_LOAD-offset; j++)
                        writeBuffer[j] = hexArray[j];

                    if((BlockIndex % milestone) == 0)
                    {
                        //terninalPrintf("80 * BlockIndex / BlockCount = %d\r\n",80 * BlockIndex / BlockCount);
                        progress = 80 * BlockIndex / BlockCount +20;
                        //progress = progress +20;
                        if(progress > 100)
                            progress = 100;
                        GuiManagerUpdateMessage(reFreshPara,progress,0);
                        terninalPrintf("progress = %d%%\r",progress);
                    }
                    BlockIndex++;
                    
                    if (bslDownload.verbose)
                        terninalPrintf("\nProgramming data block to address 0x%08X... ",page_addr);

                    if (bslDownload.ser_interface == ASC_INTERFACE)
                        result = bl_send_data(radarIndex,bslData);
                    else
                        return FALSE;


                    old_temp_addr = page_addr;
                    page_addr += DATA_BYTE_TO_LOAD;
                    

                    if (bslDownload.verbose)
                        terninalPrintf("done");

                    //reset number of bytes 
                    num_of_bytes -= DATA_BYTE_TO_LOAD;

                    //reset array pointer
                    hexArrayPtr = &hexArray[0];

                    //fill beginning of array with remaining bytes
                    for (i=0; i<num_of_bytes; i++)
                        *hexArrayPtr++ = hexArray[i+DATA_BYTE_TO_LOAD];

                }						

            } //end of if hexType == 0

    } //end of while

    if (num_of_bytes > 0) {

        //send data block
        for (j=0; j<num_of_bytes; j++)
            writeBuffer[j] = hexArray[j];

        for (j=num_of_bytes; j<DATA_BYTE_TO_LOAD; j++)
            writeBuffer[j] = 0x00;
        
        if((BlockIndex % milestone) == 0)
        {
            //sysprintf("80 * BlockIndex / BlockCount = %d\r\n",80 * BlockIndex / BlockCount);
            progress = 80 * BlockIndex / BlockCount +20;
            //progress = progress +20;
            if(progress > 100)
                progress = 100;
            GuiManagerUpdateMessage(reFreshPara,progress,0);
            terninalPrintf("progress = %d%%\r",progress);
        }
        BlockIndex++;       
        
        if (bslDownload.verbose)
            terninalPrintf("\nProgramming data block to address 0x%08X... ",page_addr);

        if (bslDownload.ser_interface == ASC_INTERFACE)
            result = bl_send_data(radarIndex,bslData);
        else
            return FALSE;

        page_addr += DATA_BYTE_TO_LOAD;

        if (bslDownload.verbose)
            terninalPrintf("done");
    }

    //send EOT frame
    if (bslDownload.ser_interface == ASC_INTERFACE)
        result = bl_send_EOT(radarIndex);
    else
        return FALSE;


    f_close(&file);

    if (bslDownload.verbose)
            terninalPrintf("\n");

    return TRUE;
            
}

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/


BOOL RB60POTAFunc(int radarIndex)
{
    
    if(!UserDrvInit(FALSE))
    {
        terninalPrintf("UserDrvInit fail.\r\n");
        return FALSE;
    }
    if(!FatfsInit(TRUE))
    {
        terninalPrintf("FatfsInit fail.\r\n");
        return FALSE;
    }
    
    if(RB60PStartBootModeFunc(radarIndex) == FALSE)
        return FALSE;
    if(RB60PSetOTABaudFunc(radarIndex) == FALSE)
        return FALSE;
    //vTaskDelay(5000/portTICK_RATE_MS);
    if (init_ASC_BSL(radarIndex) == FALSE)
        return FALSE;
    if (send_4_length(radarIndex,appLength) == FALSE)
        return FALSE;   
    


    if (make_flash_image("0:ASCLoader.hex",hexArray,appLength,&hex_address,&num_of_bytes) == FALSE)
        return FALSE;     
    if (send_ASCloader(radarIndex,hexArray,appLength) == FALSE)
        return FALSE;
    
    bslHeader.mode = 4;
	bslHeader.flashModule = 0x00; //Pflash0 will be protected
    
    if(bl_send_header(radarIndex,bslHeader) == TRUE)
    {
        bslDownload.hexFileName	 = "0:RB-60.hex";
        bslDownload.verbose		 = FALSE;
        bslDownload.device		 = XMC4400_512_DEVICE;
        bslDownload.ser_interface = ser_interface;
        if(bl_download_pflash(radarIndex,bslDownload) == FALSE)
            return FALSE;
    }
    else
        return FALSE;
        
    if(RB60PQueryVersionFunc(radarIndex) == FALSE)
        return FALSE;
    else
        return TRUE;
}

/*
BOOL RB60POTAFunc(int radarIndex)
{
    uint8_t RadarData[22];
    uint8_t FirstOTACmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x05, 0x00, 0x0E, 0xD3, 0x3D};
    uint8_t VERCmd[9] = {0x7A, 0xA7, 0x00, 0x09, 0x00, 0x00, 0x09, 0xD3, 0x3D};
    int ReadDataLen;
    
    
    RadarDrvInitEx(radarIndex ,115200);

    while(1)
    {
        if(newRadarResultPure(radarIndex,VERCmd,sizeof(VERCmd),RadarData,&ReadDataLen) == TRUE)
        {
            if(RadarData[0] == 0x00)
                break;
        }
    }

    if(RB60PSetOTABaudFunc(radarIndex) == FALSE)
        return FALSE;
    
    while(1)
    {
        if(newRadarResultPure(radarIndex,FirstOTACmd,sizeof(FirstOTACmd),RadarData,&ReadDataLen) == TRUE)
        {
            if(RadarData[0] == 0xD5)
                break;
        }
    }
    return TRUE;
    
}
*/

/*** * Copyright (C) 2020 Far Easy Pass LTD. All rights reserved. ***/

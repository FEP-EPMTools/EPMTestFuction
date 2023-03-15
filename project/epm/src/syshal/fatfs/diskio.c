/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nuc970.h"
#include "sdh.h"
#include "ff.h"
#include "diskio.h"
#include "fatfslib.h"




static __align(32) BYTE  fatfs_win_buff_pool[_VOLUMES][_MAX_SS] ;       /* FATFS window buffer is cachable. Must not use it directly. */
static BYTE  *fatfs_win_buff[_VOLUMES];

/* Definitions of physical drive number for each media */

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
void sysprintf(PINT8 pcStr,...);
DSTATUS disk_initialize (BYTE pdrv)       /* Physical drive number (0..) */
{
    //sysprintf(" >> disk_initialize - pdrv:%d <<\n", pdrv);
    if(FatfsGetCallback(pdrv) == NULL)
        sysprintf(" >> FatfsGetCallback(pdrv) == NULL <<\n");
    if(FatfsGetCallback(pdrv)->diskInitFunc(pdrv))
        return RES_OK;
    else
        return STA_NOINIT;

}


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE pdrv)       /* Physical drive number (0..) */
{
    //sysprintf(" >> disk_status - pdrv:%d <<\n", pdrv);
    if(FatfsGetCallback(pdrv) == NULL)
        sysprintf(" >> FatfsGetCallback(pdrv) == NULL <<\n");
    if(FatfsGetCallback(pdrv)->diskStatusFunc(pdrv))
        return RES_OK;
    else
        return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive number (0..) */
    BYTE *buff,     /* Data buffer to store read data */
    DWORD sector,   /* Sector address (LBA) */
    UINT count      /* Number of sectors to read (1..128) */
)
{
    DRESULT   ret;

   
    //sysprintf("disk_read - drv:%d, sec:%d, cnt:%d, buff:0x%x\n", pdrv, sector, count, (UINT32)buff);
    if(FatfsGetCallback(pdrv) == NULL)
        sysprintf(" >> FatfsGetCallback(pdrv) == NULL <<\n");
    
    if (!((UINT32)buff & 0x80000000)) 
    {
        /* Disk read buffer is not non-cachable buffer. Use my non-cachable to do disk read. */
        //if (count * 512 > _MAX_SS)
        //    return RES_ERROR;
        //sysprintf(" >> disk_read(cache) - pdrv:%d, sec:0x%08, cnt:%d <<\n", pdrv, sector, count);
        fatfs_win_buff[pdrv] = (BYTE *)((unsigned int)fatfs_win_buff_pool[pdrv] | 0x80000000);
        //outpw(REG_SDH_GCTL, SDH_GCTL_SDEN_Msk);
        //ret = (DRESULT) SD_Read(SD_PORT0, fatfs_win_buff, sector, count);
        if(FatfsGetCallback(pdrv)->diskReadFunc(pdrv, fatfs_win_buff[pdrv], sector, count))
        {
            memcpy(buff, fatfs_win_buff[pdrv], count * FatfsGetCallback(pdrv)->diskSectorSize);
            ret = RES_OK;
        }
        else
        {
            ret =  RES_ERROR;
        }  
    } 
    else 
    {
        //sysprintf(" >> disk_read - pdrv:%d, sec:0x%08, cnt:%d <<\n", pdrv, sector, count);
        if(FatfsGetCallback(pdrv)->diskReadFunc(pdrv, buff, sector, count))
        {
            ret = RES_OK;
        }
        else
        {
            ret =  RES_ERROR;
        }  
    }
    return ret;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive number (0..) */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Sector address (LBA) */
    UINT count          /* Number of sectors to write (1..128) */
)
{
    DRESULT   ret;

    
    //sysprintf("disk_write - drv:%d, sec:%d, cnt:%d, buff:0x%x\n", pdrv, sector, count, (UINT32)buff);
    if(FatfsGetCallback(pdrv) == NULL)
        sysprintf(" >> FatfsGetCallback(pdrv) == NULL <<\n");
    
    if (!((UINT32)buff & 0x80000000)) {
        /* Disk write buffer is not non-cachable buffer. Use my non-cachable to do disk write. */
        // if (count * 512 > _MAX_SS)
        //     return RES_ERROR;
        //sysprintf(" >> disk_write(cache) - pdrv:%d, sec:0x%08, cnt:%d <<\n", pdrv, sector, count);
        fatfs_win_buff[pdrv] = (BYTE *)((unsigned int)fatfs_win_buff_pool[pdrv] | 0x80000000);
        memcpy(fatfs_win_buff[pdrv], buff, count * FatfsGetCallback(pdrv)->diskSectorSize);
        if(FatfsGetCallback(pdrv)->diskWriteFunc(pdrv, fatfs_win_buff[pdrv], sector, count))
        {
            ret = RES_OK;
        }
        else
        {
            ret =  RES_ERROR;
        }  
    } 
    else 
    {
        if(FatfsGetCallback(pdrv)->diskWriteFunc(pdrv, (uint8_t*)buff, sector, count))
        {
            ret = RES_OK;
        }
        else
        {
            ret =  RES_ERROR;
        }  
    }
    return ret;
}

 
/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive number (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    DRESULT res = RES_OK;
    if(FatfsGetCallback(pdrv) == NULL)
        sysprintf(" >> FatfsGetCallback(pdrv) == NULL <<\n");
    if(FatfsGetCallback(pdrv)->diskIoctlFunc(pdrv, cmd, buff))
    {
        res = RES_OK;
    }
    else
    {
        res =  RES_PARERR;
    }  
    return res;   
}

/**************************************************************************//**
* @file     yaffs2drv.c
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

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "fepconfig.h"
#include "nand.h"
#include "yaffs2drv.h"
#include "yaffsfs.h"
#include "yaffs_packedtags2.h"
#include "yaffs_mtdif.h"
#include "yaffs_mtdif2.h"
#include "yaffs_malloc.h"
#include "yaffs_trace.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static     char mtpoint[] = "/";

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
//static char data[2048+1024];
//static char data2[2048+1024];
//static int times = 0;
//static int errortimes = 0;
static int yaffs_regions_overlap(int a, int b, int x, int y)
{
    return  (a <= x && x <= b) ||
            (a <= y && y <= b) ||
            (x <= a && a <= y) ||
            (x <= b && b <= y);
}


static const char *yaffs_file_type_str(struct yaffs_stat *stat)
{
    switch (stat->st_mode & S_IFMT) {
    case S_IFREG:
        return "regular file";
    case S_IFDIR:
        return "directory";
    case S_IFLNK:
        return "symlink";
    default:
        return "unknown";
    }
}
static const char *yaffs_error_str(void)
{
    int error = yaffsfs_GetLastError();

    if (error < 0)
        error = -error;

    switch (error) {
    case EBUSY:
        return "Busy";
    case ENODEV:
        return "No such device";
    case EINVAL:
        return "Invalid parameter";
    case ENFILE:
        return "Too many open files";
    case EBADF:
        return "Bad handle";
    case EACCES:
        return "Wrong permissions";
    case EXDEV:
        return "Not on same device";
    case ENOENT:
        return "No such entry";
    case ENOSPC:
        return "Device full";
    case EROFS:
        return "Read only file system";
    case ERANGE:
        return "Range error";
    case ENOTEMPTY:
        return "Not empty";
    case ENAMETOOLONG:
        return "Name too long";
    case ENOMEM:
        return "Out of memory";
    case EFAULT:
        return "Fault";
    case EEXIST:
        return "Name exists";
    case ENOTDIR:
        return "Not a directory";
    case EISDIR:
        return "Not permitted on a directory";
    case ELOOP:
        return "Symlink loop";
    case 0:
        return "No error";
    default:
        return "Unknown error";
    }
}
static void cmd_yaffs_ls(const char *mountpt, int longlist)
{
    int i;
    yaffs_DIR *d;
    struct yaffs_dirent *de;
    struct yaffs_stat stat;
    char tempstr[255];
    sysprintf("\r\n -------- cmd_yaffs_ls start --------\n");
    d = yaffs_opendir(mountpt);

    if (!d) 
    {
        sysprintf("opendir failed, %s\n", yaffs_error_str());
        return;
    }

    for (i = 0; (de = yaffs_readdir(d)) != NULL; i++) 
    {
        
            
        if (longlist) 
        {
            sprintf(tempstr, "%s/%s", mountpt, de->d_name);
            yaffs_lstat(tempstr, &stat);
            sysprintf("%-25s\t%7ld", de->d_name, (long)stat.st_size);
            sysprintf(" %5d %s\n", stat.st_ino, yaffs_file_type_str(&stat));
        } 
        else
        {
            sysprintf("%s\n", de->d_name);
        }
        
        if(strlen (de->d_name) > 32)
        {
            sysprintf(" !! cmd_yaffs_ls : file name error, delete it\n");
            yaffs_unlink(de->d_name);
        }
        if((long)stat.st_size > (1024*1024*4))
        {
            sysprintf(" !! cmd_yaffs_ls : file size error, delete it\n");
            yaffs_unlink(de->d_name);
        }
        
        
    }

    yaffs_closedir(d);
    sysprintf(" -------- cmd_yaffs_ls end --------\n");
}

/*
static void vYaffs2DrvTestTask( void *pvParameters )
{
    int i;
    vTaskDelay(2000/portTICK_RATE_MS); 
    sysprintf("vYaffs2DrvTestTask Going...\r\n");      
    for(;;)
    {     
        char nameTmp[32];
        int fileLen = 0;
        vTaskDelay(2000/portTICK_RATE_MS);
        times++;
        sysprintf(".............vYaffs2DrvTestTask : errortimes = %d....................\r\n", errortimes); 
        cmd_yaffs_ls("/", 1);
        sprintf(nameTmp, "Test_%03d.dat", times%32);
        fileLen = 2048 + times;
        for(i = 0; i<fileLen; i++)
        {
            data[i] = i + times;
        }
        cmd_yaffs_rm(nameTmp);
        cmd_yaffs_mwrite_file(nameTmp, data, fileLen);
        
        cmd_yaffs_mread_file(nameTmp, data2);
        
        for(int i = 0; i<fileLen; i++)
        {
            if(data[i] != data2[i])
            {
                sysprintf(" ....vYaffs2DrvTestTask : ERROR [%s:%d: %d, %d].....\r\n", nameTmp, i, data[i], data2[i]); 
                for(int j = i-10; j < i+10; j++)
                {
                    sysprintf(" ->[0x%03d]: %d, %d.....\r\n", j, data[j], data2[j]);
                }
                errortimes++;
                break;
            }
        }
    }
}
*/
static void cmd_yaffs_dev_ls(void)
{
    struct yaffs_dev *dev;
    int flash_dev;
    int free_space;

    yaffs_dev_rewind();

    while (1) {
        dev = yaffs_next_dev();
        if (!dev)
            return;
        flash_dev =
            ((unsigned) dev->driver_context - (unsigned) nand_info)/
            sizeof(nand_info[0]);
        sysprintf("%-3s %5d 0x%05x 0x%05x %s",
                  dev->param.name, flash_dev,
                  dev->param.start_block, dev->param.end_block,
                  dev->param.inband_tags ? "using inband tags, " : "");

        free_space = yaffs_freespace(dev->param.name);
        if (free_space < 0)
            sysprintf("not mounted\n");
        else
            sysprintf("free 0x%x\n", free_space);

    }
}
static void cmd_yaffs_devconfig(char *_mp, int flash_dev,
                         int start_block, int end_block)
{
    struct mtd_info *mtd = NULL;
    struct yaffs_dev *dev = NULL;
    struct yaffs_dev *chk;
    char *mp = NULL;
    struct nand_chip *chip;

//  dev = calloc(1, sizeof(*dev));
//  mp = (char *)strdup(_mp);
    dev = yaffs_malloc(sizeof(*dev));
    memset(dev, 0, sizeof(*dev));
    mp = yaffs_malloc(strlen(_mp));
    strcpy(mp, _mp);

    mtd = &nand_info[flash_dev];

    if (!dev || !mp) {
        /* Alloc error */
        sysprintf("Failed to allocate memory\n");
        goto err;
    }

    if (flash_dev >= 1) {
        sysprintf("Flash device invalid\n");
        goto err;
    }

    if (end_block == 0)
        end_block = mtd->size / mtd->erasesize - 1;

    if (end_block < start_block) {
        sysprintf("Bad start/end\n");
        goto err;
    }

    chip =  mtd->priv;

    /* Check for any conflicts */
    yaffs_dev_rewind();
    while (1) {
        chk = yaffs_next_dev();
        if (!chk)
            break;
        if (strcmp(chk->param.name, mp) == 0) {
            sysprintf("Mount point name already used\n");
            goto err;
        }
        if (chk->driver_context == mtd &&
                yaffs_regions_overlap(
                    chk->param.start_block, chk->param.end_block,
                    start_block, end_block)) {
            sysprintf("Region overlaps with partition %s\n",
                      chk->param.name);
            goto err;
        }

    }

    /* Seems same, so configure */
    memset(dev, 0, sizeof(*dev));
    dev->param.name = mp;
    dev->driver_context = mtd;
    dev->param.start_block = start_block;
    dev->param.end_block = end_block;
    dev->param.chunks_per_block = mtd->erasesize / mtd->writesize;
    dev->param.total_bytes_per_chunk = mtd->writesize;
    dev->param.is_yaffs2 = 1;
    dev->param.use_nand_ecc = 1;
    dev->param.n_reserved_blocks = 5;
    if (chip->ecc.layout->oobavail <= sizeof(struct yaffs_packed_tags2))
        dev->param.inband_tags = 1;
    dev->param.n_caches = 10;
    dev->param.write_chunk_tags_fn = nandmtd2_write_chunk_tags;
    dev->param.read_chunk_tags_fn = nandmtd2_read_chunk_tags;
    dev->param.erase_fn = nandmtd_EraseBlockInNAND;
    dev->param.initialise_flash_fn = nandmtd_InitialiseNAND;
    dev->param.bad_block_fn = nandmtd2_MarkNANDBlockBad;
    dev->param.query_block_fn = nandmtd2_QueryNANDBlock;

    yaffs_add_device(dev);

    sysprintf("Configures yaffs mount %s: dev %d start block %d, end block %d %s\n",
              dev->param.name, flash_dev, start_block, end_block,
              dev->param.inband_tags ? "using inband tags" : "");
    return;

err:
    yaffs_free(dev);
    yaffs_free(mp);
}
static void pinInit(void)
{
    /* enable FMI NAND */
    outpw(REG_CLK_HCLKEN, (inpw(REG_CLK_HCLKEN) | 0x300000));

    /* select NAND function pins */
    if (inpw(REG_SYS_PWRON) & 0x08000000) {
        /* Set GPI1~15 for NAND */
        outpw(REG_SYS_GPI_MFPL, 0x55555550);
        outpw(REG_SYS_GPI_MFPH, 0x55555555);
    } else {
        /* Set GPC0~14 for NAND */
        outpw(REG_SYS_GPC_MFPL, 0x55555555);
        outpw(REG_SYS_GPC_MFPH, 0x05555555);
    }
}
static BOOL hwInit(void)
{   
    pinInit();
    nand_init();
    //cmd_yaffs_devconfig(mtpoint, 0, 0xb0, 0x3ff);
    cmd_yaffs_devconfig(mtpoint, 0, 64, 1023);
    //cmd_yaffs_dev_ls();
    yaffs_mount(mtpoint);
    cmd_yaffs_dev_ls();
    return TRUE;
}
static BOOL swInit(void)
{   
    //xTaskCreate( vYaffs2DrvTestTask, "vYaffs2DrvTestTask", 8*1024, NULL, 1, NULL); 
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/

BOOL Yaffs2DrvInit(void)
{
    //int i;
    sysprintf("Yaffs2DrvInit!!\n");
    if(hwInit() == FALSE)
    {
        sysprintf("Yaffs2DrvInit ERROR (hwInit false)!!\n");
        return FALSE;
    }
    if(swInit() == FALSE)
    {
        sysprintf("Yaffs2DrvInit ERROR (swInit false)!!\n");
        return FALSE;
    }       
    //cmd_yaffs_mkdir("user");
    sysprintf("Yaffs2DrvInit OK!!\n"); 
    cmd_yaffs_ls("/", 1);
    
    return TRUE;
}
void Yaffs2ListFileEx (char* dir)
{
    cmd_yaffs_ls(dir, 1);
}
const char *Yaffs2ErrorStr(void)
{
    return yaffs_error_str();
}

/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


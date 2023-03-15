/**************************************************************************//**
* @file     user.c
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
#include "user.h"
#include "guidrv.h"
#include "guistandby.h"
#include "guimanager.h"
#include "paralib.h"
#include "meterdata.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/
static userInitFunctionList mUserInitFunctionList[] = {
                                                        #if(ENABLE_PARA_LIB)
                                                        {"ParaLib", ParaLibInit},
                                                        #endif
                                                        {"MeterData", MeterDataInit},
                                                        #if(ENABLE_GUI_MANAGER)
                                                        {"GuiManager", GuiManagerInit}, 
                                                        #endif
                                                        {"", NULL}};

/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/
static BOOL userInit()
{
    int i;
    sysprintf("  ==> userInit start...\r\n");

    for(i = 0; ; i++)
    {
        if(mUserInitFunctionList[i].func != NULL)
        {
            if(mUserInitFunctionList[i].func())
            {
                sysprintf("  = [%02d]: %s OK... =\r\n", i, mUserInitFunctionList[i].drvName);
            }
            else
            {
                sysprintf("  = [%02d]: %s ERROR... =\r\n", i, mUserInitFunctionList[i].drvName);
                return FALSE;
            }
        }
        else
        {
            
            break;
        }
    }    
    sysprintf("  ==> userInit end...\r\n");
    return TRUE;
}
/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL UserInit(BOOL testModeFlag)
{
    sysprintf("UserInit!!\n");
    return userInit(); 
}



/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


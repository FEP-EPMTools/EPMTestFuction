/**************************************************************************//**
* @file     base.c
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
#include "base.h"
/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* global file scope (static) variables    */
/*-----------------------------------------*/


/*-----------------------------------------*/
/* prototypes of static functions          */
/*-----------------------------------------*/

/*-----------------------------------------*/
/* Exported Functions          */
/*-----------------------------------------*/
BOOL BaseDrvInit(void)
{
    sysprintf("BaseDrvInit!!\n");
    return TRUE;
}



/*** * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved. ***/


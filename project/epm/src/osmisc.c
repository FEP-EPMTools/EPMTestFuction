/**************************************************************************//**
 * @file     osmisc.c
 * @version  V1.00
 * $Date: 15/05/07 5:38p $
 * @brief  
 *
 * @note
 * Copyright (C) 2016 Far Easy Pass LTD. All rights reserved.
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "osmisc.h"
#include "stdarg.h"
/* Private macro -------------------------------------------------------------*/
#define ENABLE_MEM_PRINT   1
/* Private variables ---------------------------------------------------------*/
//static int CYSYS_Reboot=0;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
__asm void __wfi(void)
{
    MCR p15, 0, r1, c7, c0, 4
    BX            lr
}

void _showMacroMessage(char* tag, char* file, int line)
{
    #if(ENABLE_MEM_PRINT)
    sysprintf(" ~~~~ %s ~~~~ %s -> StackHighWaterMark = %d bytes\r\n", 
            tag, pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), (int)uxTaskGetStackHighWaterMark(xTaskGetCurrentTaskHandle()));  
    sysprintf("                                at file %s line %d\r\n", file, line); 
    #endif
}

void _showCurrentThreadMemoryInfo(void)
{
    #if(ENABLE_MEM_PRINT)
    sysprintf(" ::INFO:: %s-> StackHighWaterMark = %d bytes\r\n", 
            pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), (int)uxTaskGetStackHighWaterMark(xTaskGetCurrentTaskHandle())); 
    #endif
}

void _showCurrentThreadMemoryInfoEx(char* file, int line)
{
    #if(ENABLE_MEM_PRINT)
    //_showFreeHeapSize(file, line); 
    sysprintf(" ::INFO:: %s -> StackHighWaterMark = %d bytes, at file %s line %d\r\n", 
            pcTaskGetTaskName(xTaskGetCurrentTaskHandle()), (int)uxTaskGetStackHighWaterMark(xTaskGetCurrentTaskHandle()),
                 file, line); 
    
    #endif
}

void _showFreeHeapSize(char* file, int line)
{
    #if(ENABLE_MEM_PRINT)
    sysprintf(" ||INFO|| cxPortGetFreeHeapSize = %d bytes ( %d MB), at file %s line %d\r\n", xPortGetFreeHeapSize(), xPortGetFreeHeapSize()/1024/1024, file, line); 
    #endif
}

void _showMemoryInfo(void)
{
    //size_t xPortGetFreeHeapSize( void )
    //unsigned portBASE_TYPE uxTaskGetStackHighWaterMark( xTaskHandle xTask )    
  
    int      Idx;
    int16_t  tasks_nbr;
    xTaskStatusType  ProcessStatus[32] = {{0}};
    tasks_nbr = uxTaskGetSystemState( ProcessStatus, 32, NULL );
    sysprintf("*********************************Thread Info Start************************************\r\n");    
    /*Limit view size */
    if(tasks_nbr > 32)
    {
        tasks_nbr = 32;
    }
    for (Idx = 0; Idx < tasks_nbr ; Idx ++)
    {
        sysprintf("   %2d: %18s, Proi:%2lu, Status:", Idx, ProcessStatus[Idx].pcTaskName, ProcessStatus[Idx].uxCurrentPriority);        
        
        switch (ProcessStatus[Idx].eCurrentState)
        {
        case eReady:
            sysprintf("%10s", "Ready");
            break;

        case eBlocked:
            sysprintf("%10s", "Blocked");
            break;

        case eDeleted:
            sysprintf("%10s", "Deleted");
            break;

        case eSuspended:
            sysprintf("%10s", "Suspended");
            break;

        case eRunning:
            sysprintf("%10s", "Running");
            break;

        default:
            sysprintf("%10s", "Unknown");
            break;
        }
        sysprintf(" => stack left :%4d ", (int)uxTaskGetStackHighWaterMark(ProcessStatus[Idx].xHandle));    
        sysprintf("\r\n");
    }     
    sysprintf("----------------------------------Thread Info End-----------------------------------\r\n");  
}

#if(configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook( xTaskHandle xTask, signed char *pcTaskName )
{
    sysprintf("         !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    sysprintf("         !!!!!!! vApplicationStackOverflowHook (Thread Name:%s)  !!!!!!!\r\n", pcTaskName);
    sysprintf("         !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
}
#endif

#if(configUSE_MALLOC_FAILED_HOOK > 0)
void vApplicationMallocFailedHook( void )
{
    sysprintf("         !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
    sysprintf("         !!!!!!! vApplicationMallocFailedHook !!!!!!!\r\n");
    sysprintf("         !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
}
#endif
/*
#if ( configUSE_IDLE_HOOK == 1 )
void CYOSSetBootFlag(int flag)
{
    printf("\r\n !!!!!!!!!!!!!!! CYOSSetBootFlag = %d !!!!!!!!!!!!!!!\r\n", flag);
    CYSYS_Reboot = flag;
}
void vApplicationIdleHook( void )
{
    if(CYSYS_Reboot != 0)
    {
        HAL_NVIC_SystemReset(); 
    }
}
#endif // configUSE_IDLE_HOOK 
*/
size_t GetFreeHeapSize(void)
{
    return xPortGetFreeHeapSize();
}
void vApplicationTickHook(void)
{
    //sysprintf("^"); 
}
void vApplicationIdleHook(void)
{
    //static int timer = 0;
    //sysprintf("[%03d]\r", timer++%1000); 
    //sysprintf("'");
    __wfi();
}

BaseType_t xTaskIncrementTick( void );

void tickProcess(void)
{
    //sysprintf(".");  
    /* Increment the tick counter. */
    if( xTaskIncrementTick() != pdFALSE )
	{
        /* Select a new task to run. */
        //sysprintf("*");
        vTaskSwitchContext();
	}

}


void initTimer(void)
{
    sysprintf("initTimer!!!\n");
    //sysSetTimerReferenceClock (TIMER0, 12000000);    
    sysStartTimer(TIMER0, 1000, PERIODIC_MODE);
    //sysStartTimer(TIMER0, 100, PERIODIC_MODE);
    sysSetTimerEvent(TIMER0, 1, (PVOID)tickProcess);
}



/**
  ******************************************************************************
  * @file    CYprintf.h
  * @author  Sam Chang
  * @version 
  * @date    
  * @brief   Header for CYprintf.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CY_OS_MISC_H
#define __CY_OS_MISC_H

/* Includes ------------------------------------------------------------------*/
#include "nuc970.h"
#include "sys.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "stdio.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define __SHOW_CURRENT_THREAD_MEM_INFO__  _showCurrentThreadMemoryInfoEx(__FILE__, __LINE__);
#define __SHOW_FREE_HEAP_SIZE__  _showFreeHeapSize(__FILE__, __LINE__);
/* Exported functions ------------------------------------------------------- */
void _showMacroMessage(char* tag, char* file, int line);
void _showCurrentThreadMemoryInfo(void);
void _showCurrentThreadMemoryInfoEx(char* file, int line);
void _showFreeHeapSize(char* file, int line);
void _showMemoryInfo(void);


size_t GetFreeHeapSize(void);

#if ( configUSE_IDLE_HOOK == 1 )
void CYOSSetBootFlag(int flag);
#endif

#endif /* __CY_OS_MISC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*
 * COPYRIGHT (C) STMicroelectronics 2015. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * STMicroelectronics ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with STMicroelectronics
 *
 * Programming Golden Rule: Keep it Simple!
 *
 */

/*!
 * \file   VL53L0X_platform.c
 * \brief  Code function defintions for Doppler Testchip Platform Layer
 *
 */


//#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>    // sprintf(), vsnprintf(), printf()
#include "nuc970.h"
#include "sys.h"
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_def.h"

#include <stdio.h>
#include <stdlib.h>
//#include <windows.h>
#include <time.h>
//#include "SERIAL_COMMS.h"
//#include "comms_platform.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "vl53l0x_platform_log.h"
#include "nuc970.h"
#include "sys.h"
#include "i2c.h"
#include "interface.h"

#define BYTES_PER_uint32_t  4
#define RETRY               1000  /* Programming cycle may be in progress. Please refer to 24LC64 datasheet */
I2cInterface* pI2cInterface = NULL;
//#define VL53L0X_LOG_ENABLE_1 

#ifdef VL53L0X_LOG_ENABLE
#define trace_print(level, ...) trace_print_module_function(TRACE_MODULE_PLATFORM, level, TRACE_FUNCTION_NONE, ##__VA_ARGS__)
#define trace_i2c(...) trace_print_module_function(TRACE_MODULE_NONE, TRACE_LEVEL_NONE, TRACE_FUNCTION_I2C, ##__VA_ARGS__)
#endif

//static char  debug_string[VL53L0X_MAX_STRING_LENGTH_PLT];

uint8_t cached_page = 0;

#define MIN_COMMS_VERSION_MAJOR     1
#define MIN_COMMS_VERSION_MINOR     8
#define MIN_COMMS_VERSION_BUILD     1
#define MIN_COMMS_VERSION_REVISION  0


#define MAX_STR_SIZE 255
#define MAX_MSG_SIZE 100
#define MAX_DEVICES 4
#define STATUS_OK              0x00
#define STATUS_FAIL            0x01

//static HANDLE ghMutex;
static int rErrTimes, wErrTimes;

//static unsigned char _dataBytes[MAX_MSG_SIZE];

bool_t _check_min_version(void)
{
    return TRUE;
}
void VL53L0XPrintErrTimes(void)
{
    if((wErrTimes != 0) || (rErrTimes != 0))
        sysprintf("\r\n << INFO: VL53L0X error times : %d, %d >>\n", wErrTimes, rErrTimes);
}
void VL53L0XEnterCS(void)
{
    pI2cInterface->enableCriticalSectionFunc(TRUE);
}
void VL53L0XExitCS(void)
{
    pI2cInterface->enableCriticalSectionFunc(FALSE);
}

int VL53L0X_i2c_init(I2cInterface* i2cInterface) // mja
{
    sysprintf("\r\n ==>> call VL53L0X_i2c_init...\n");
    pI2cInterface = i2cInterface;
    return VL53L0X_ERROR_NONE;
}
int32_t VL53L0X_comms_close(void)
{
    sysprintf("\r\n ==>> call VL53L0X_comms_close...\n");
    return 0;
}

int32_t VL53L0X_write_multi(uint8_t address, uint8_t reg, uint8_t *pdata, int32_t count)
{
    int j ;
    //sysprintf("[w]");
    //sysprintf("\r\n ==>> call VL53L0X_write_multi...\n");
    //i2cEnterCriticalSection(VL53L0X_I2C);
    
    VL53L0XEnterCS();
    pI2cInterface->ioctlFunc(I2C_IOC_SET_DEV_ADDRESS, address, 0);
    pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, reg, 1);
    j = RETRY;
    while(j-- > 0) 
    {
            
        if(pI2cInterface->writeFunc(pdata, count) == count)
        {
            break;
        }
        if(j <= 0)
        {
            sysprintf("VL53L0X_write_multi WRITE ERROR!\n");
            wErrTimes++;
            VL53L0XExitCS();
            return STATUS_FAIL;
        }
    }
    VL53L0XExitCS();
    //sysprintf("VL53L0X_write_multi OK...\n");
    return STATUS_OK;
}

int32_t VL53L0X_read_multi(uint8_t address, uint8_t reg, uint8_t *pdata, int32_t count)
{ 
    int j ;
    //sysprintf("[r]");
    //sysprintf("\r\n ==>> call VL53L0X_read_multi...\n");
    VL53L0XEnterCS();
    pI2cInterface->ioctlFunc(I2C_IOC_SET_DEV_ADDRESS, address, 0);
    pI2cInterface->ioctlFunc(I2C_IOC_SET_SUB_ADDRESS, NULL, 0);
    j = RETRY;
    while(j-- > 0) 
    {            
        if(pI2cInterface->writeFunc(&reg, 1) == 1)
        {
            break;
        }
        if(j <= 0)
        {
            sysprintf("VL53L0X_read_multi WRITE ERROR!\n");
            rErrTimes++;
            VL53L0XExitCS();
            return STATUS_FAIL;
        }
    }
    
    j = RETRY;
    while(j-- > 0) 
    {
        if(pI2cInterface->readFunc(pdata, count) == count)
        {
            break;
        }
    }
    if(j <= 0)
    {
        sysprintf("VL53L0X_read_multi Read ERROR!\n");
        rErrTimes++;
        VL53L0XExitCS();
        return STATUS_FAIL;
    }
    //sysprintf("VL53L0X_read_multi OK...\n");
    VL53L0XExitCS();
    return STATUS_OK;
}


int32_t VL53L0X_write_byte(uint8_t address, uint8_t index, uint8_t data)
{
    int32_t status = STATUS_OK;
    const int32_t cbyte_count = 1;

#ifdef VL53L0X_LOG_ENABLE_1
    sysprintf(" - VL53L0X [Write Byte] reg : 0x%02X, Val : 0x%02X\n", index, data);
#endif

    status = VL53L0X_write_multi(address, index, &data, cbyte_count);

    return status;

}


int32_t VL53L0X_write_word(uint8_t address, uint8_t index, uint16_t data)
{
    int32_t status = STATUS_OK;

    uint8_t  buffer[BYTES_PER_WORD];
#ifdef VL53L0X_LOG_ENABLE_1
    sysprintf(" - VL53L0X [Write word] reg : 0x%02X, Val : 0x%04X\n", index, data);
#endif
    // Split 16-bit word into MS and LS uint8_t
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data &  0x00FF);

    if(index%2 == 1)
    {
        status = VL53L0X_write_multi(address, index, &buffer[0], 1);
        status = VL53L0X_write_multi(address, index + 1, &buffer[1], 1);
        // serial comms cannot handle word writes to non 2-byte aligned registers.
    }
    else
    {
        status = VL53L0X_write_multi(address, index, buffer, BYTES_PER_WORD);
    }

    return status;

}


int32_t VL53L0X_write_dword(uint8_t address, uint8_t index, uint32_t data)
{
    int32_t status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_uint32_t];
    
#ifdef VL53L0X_LOG_ENABLE_1
    sysprintf(" - VL53L0X [Write dword] reg : 0x%02X, Val : 0x%08X\n", index, data);
#endif
    // Split 32-bit word into MS ... LS bytes
    buffer[0] = (uint8_t) (data >> 24);
    buffer[1] = (uint8_t)((data &  0x00FF0000) >> 16);
    buffer[2] = (uint8_t)((data &  0x0000FF00) >> 8);
    buffer[3] = (uint8_t) (data &  0x000000FF);

    status = VL53L0X_write_multi(address, index, buffer, BYTES_PER_uint32_t);

    return status;

}


int32_t VL53L0X_read_byte(uint8_t address, uint8_t index, uint8_t *pdata)
{
    int32_t status = STATUS_OK;
    int32_t cbyte_count = 1;
    
#ifdef VL53L0X_LOG_ENABLE_1
    sysprintf(" ~ VL53L0X <Read Byte> reg : 0x%02X start...\n", index);
#endif
    
    status = VL53L0X_read_multi(address, index, pdata, cbyte_count);

#ifdef VL53L0X_LOG_ENABLE_1
    if(STATUS_OK == status)
        sysprintf(" ~ VL53L0X <Read Byte> reg : 0x%02X, Val : 0x%02X\n", index, *pdata);
#endif

    return status;

}


int32_t VL53L0X_read_word(uint8_t address, uint8_t index, uint16_t *pdata)
{
    int32_t  status = STATUS_OK;
	uint8_t  buffer[BYTES_PER_WORD];
#ifdef VL53L0X_LOG_ENABLE_1
    sysprintf(" ~ VL53L0X <Read word> reg : 0x%02X start...\n", index);
#endif
    status = VL53L0X_read_multi(address, index, buffer, BYTES_PER_WORD);
	*pdata = ((uint16_t)buffer[0]<<8) + (uint16_t)buffer[1];
#ifdef VL53L0X_LOG_ENABLE_1
    if(STATUS_OK == status)
        sysprintf(" ~ VL53L0X <Read word> reg : 0x%02X, Val : 0x%04X\n", index, *pdata);
#endif
    return status;

}

int32_t VL53L0X_read_dword(uint8_t address, uint8_t index, uint32_t *pdata)
{
    int32_t status = STATUS_OK;
	uint8_t  buffer[BYTES_PER_uint32_t];
#ifdef VL53L0X_LOG_ENABLE_1
    sysprintf(" ~ VL53L0X <Read dword> reg : 0x%02X start...\n", index);
#endif
    status = VL53L0X_read_multi(address, index, buffer, BYTES_PER_uint32_t);
    *pdata = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];
#ifdef VL53L0X_LOG_ENABLE_1
    if(STATUS_OK == status)
        sysprintf(" ~ VL53L0X <Read dword> reg : 0x%02X, Val : 0x%08X\n", index, *pdata);
#endif
    return status;

}



// 16 bit address functions
/*

int32_t VL53L0X_write_multi16(uint8_t address, uint16_t index, uint8_t *pdata, int32_t count)
{
#if(0)
    int32_t status = STATUS_OK;
    unsigned int retries = 3;
    uint32_t dwWaitResult;

#ifdef VL53L0X_LOG_ENABLE
    int32_t i = 0;

    char value_as_str[VL53L0X_MAX_STRING_LENGTH_PLT];
    char *pvalue_as_str;

    pvalue_as_str =  value_as_str;

    for(i = 0 ; i < count ; i++)
    {
        sprintf(pvalue_as_str,"%02X", *(pdata+i));

        pvalue_as_str += 2;
    }
    trace_i2c("Write reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

    dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);
    if(dwWaitResult == WAIT_OBJECT_0)
    {
        do
        {
            status = SERIAL_COMMS_Write_UBOOT(address, 0, index, pdata, count);
            // note : the field dwIndexHi is ignored. dwIndexLo will
            // contain the entire index (bits 0..15).
            if(status != STATUS_OK)
            {
                SERIAL_COMMS_Get_Error_Text(debug_string);
            }
        } while ((status != 0) && (retries-- > 0));
        ReleaseMutex(ghMutex);
    }

    // store the page from the high byte of the index
    cached_page = HIBYTE(index);

    if(status != STATUS_OK)
    {
        SERIAL_COMMS_Get_Error_Text(debug_string);
    }


    return status;
#else
    sysprintf("\r\n ==>> call VL53L0X_write_multi16...\n");
    return 0;
#endif
}

int32_t VL53L0X_read_multi16(uint8_t address, uint16_t index, uint8_t *pdata, int32_t count)
{
#if(0)
    int32_t status = STATUS_OK;
    unsigned int retries = 3;
    uint32_t dwWaitResult;

#ifdef VL53L0X_LOG_ENABLE
    int32_t      i = 0;

    char   value_as_str[VL53L0X_MAX_STRING_LENGTH_PLT];
    char *pvalue_as_str;
#endif


    dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);
    if(dwWaitResult == WAIT_OBJECT_0)
    {
        do
        {
            status = SERIAL_COMMS_Read_UBOOT(address, 0, index, pdata, count);
            if(status != STATUS_OK)
            {
                SERIAL_COMMS_Get_Error_Text(debug_string);
            }
        } while ((status != 0) && (retries-- > 0));
        ReleaseMutex(ghMutex);
    }

    // store the page from the high byte of the index
    cached_page = HIBYTE(index);

    if(status != STATUS_OK)
    {
        SERIAL_COMMS_Get_Error_Text(debug_string);
    }

#ifdef VL53L0X_LOG_ENABLE
    // Build  value as string;
    pvalue_as_str =  value_as_str;

    for(i = 0 ; i < count ; i++)
    {
        sprintf(pvalue_as_str, "%02X", *(pdata+i));
        pvalue_as_str += 2;
    }

    trace_i2c("Read  reg : 0x%04X, Val : 0x%s\n", index, value_as_str);
#endif

    return status;
#else
    sysprintf("\r\n ==>> call VL53L0X_read_multi16...\n");
    return 0;
#endif
}



int32_t VL53L0X_write_byte16(uint8_t address, uint16_t index, uint8_t data)
{
    int32_t status = STATUS_OK;
    const int32_t cbyte_count = 1;

#ifdef VL53L0X_LOG_ENABLE
    trace_print(TRACE_LEVEL_INFO,"Write reg : 0x%02X, Val : 0x%02X\n", index, data);
#endif

    status = VL53L0X_write_multi16(address, index, &data, cbyte_count);

    return status;

}


int32_t VL53L0X_write_word16(uint8_t address, uint16_t index, uint16_t data)
{
    int32_t status = STATUS_OK;

    uint8_t  buffer[BYTES_PER_WORD];

    // Split 16-bit word into MS and LS uint8_t
    buffer[0] = (uint8_t)(data >> 8);
    buffer[1] = (uint8_t)(data &  0x00FF);

    if(index%2 == 1)
    {
        status = VL53L0X_write_multi16(address, index, &buffer[0], 1);
        status = VL53L0X_write_multi16(address, index + 1, &buffer[1], 1);
        // serial comms cannot handle word writes to non 2-byte aligned registers.
    }
    else
    {
        status = VL53L0X_write_multi16(address, index, buffer, BYTES_PER_WORD);
    }

    return status;

}


int32_t VL53L0X_write_dword16(uint8_t address, uint16_t index, uint32_t data)
{
    int32_t status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_uint32_t];

    // Split 32-bit word into MS ... LS bytes
    buffer[0] = (uint8_t) (data >> 24);
    buffer[1] = (uint8_t)((data &  0x00FF0000) > 16);
    buffer[2] = (uint8_t)((data &  0x0000FF00) > 8);
    buffer[3] = (uint8_t) (data &  0x000000FF);

    status = VL53L0X_write_multi16(address, index, buffer, BYTES_PER_uint32_t);

    return status;

}


int32_t VL53L0X_read_byte16(uint8_t address, uint16_t index, uint8_t *pdata)
{
    int32_t status = STATUS_OK;
    int32_t cbyte_count = 1;

    status = VL53L0X_read_multi16(address, index, pdata, cbyte_count);

#ifdef VL53L0X_LOG_ENABLE
    trace_print(TRACE_LEVEL_INFO,"Read reg : 0x%02X, Val : 0x%02X\n", index, *pdata);
#endif

    return status;

}


int32_t VL53L0X_read_word16(uint8_t address, uint16_t index, uint16_t *pdata)
{
    int32_t  status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_WORD];

    status = VL53L0X_read_multi16(address, index, buffer, BYTES_PER_WORD);
    *pdata = ((uint16_t)buffer[0]<<8) + (uint16_t)buffer[1];

    return status;

}

int32_t VL53L0X_read_dword16(uint8_t address, uint16_t index, uint32_t *pdata)
{
    int32_t status = STATUS_OK;
    uint8_t  buffer[BYTES_PER_uint32_t];

    status = VL53L0X_read_multi16(address, index, buffer, BYTES_PER_uint32_t);
    *pdata = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];

    return status;

}




int32_t VL53L0X_platform_wait_us(int32_t wait_us)
{
#if(0)
    int32_t status = STATUS_OK;
    float wait_ms = (float)wait_us/1000.0f;

    
    //Use windows event handling to perform non-blocking wait.
    
    HANDLE hEvent = CreateEvent(0, TRUE, FALSE, 0);
    WaitForSingleObject(hEvent, (int)(wait_ms + 0.5f));

#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("Wait us : %6d\n", wait_us);
#endif

    return status;
#else
    sysprintf("\r\n ==>> call VL53L0X_platform_wait_us...\n");
    vTaskDelay(wait_us/portTICK_RATE_MS);
    return 0;
#endif
}


int32_t VL53L0X_wait_ms(int32_t wait_ms)
{
#if(0)
    int32_t status = STATUS_OK;

    
    //Use windows event handling to perform non-blocking wait.
    
    HANDLE hEvent = CreateEvent(0, TRUE, FALSE, 0);
    WaitForSingleObject(hEvent, wait_ms);

#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("Wait ms : %6d\n", wait_ms);
#endif

    return status;
#else
    sysprintf("\r\n ==>> call VL53L0X_wait_ms...\n");
    vTaskDelay(wait_ms/portTICK_RATE_MS);
    return 0;
#endif
}
*/
/*
int32_t VL53L0X_set_gpio(uint8_t level)
{
    int32_t status = STATUS_OK;
    //status = VL53L0X_set_gpio_sv(level);
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// Set GPIO = %d;\n", level);
#endif
    return status;

}


int32_t VL53L0X_get_gpio(uint8_t *plevel)
{
    int32_t status = STATUS_OK;
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// Get GPIO = %d;\n", *plevel);
#endif
    return status;
}


int32_t VL53L0X_release_gpio(void)
{
    int32_t status = STATUS_OK;
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// Releasing force on GPIO\n");
#endif
    return status;

}

int32_t VL53L0X_cycle_power(void)
{
    int32_t status = STATUS_OK;
#ifdef VL53L0X_LOG_ENABLE
    trace_i2c("// cycle sensor power\n");
#endif
	return status;
}


int32_t VL53L0X_get_timer_frequency(int32_t *ptimer_freq_hz)
{
       *ptimer_freq_hz = 0;
       return STATUS_FAIL;
}


int32_t VL53L0X_get_timer_value(int32_t *ptimer_count)
{
       *ptimer_count = 0;
       return STATUS_FAIL;
}
*/

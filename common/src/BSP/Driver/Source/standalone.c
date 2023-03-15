/*
 * standalone.c - minimal bootstrap for C library
 * Copyright (C) 2000 ARM Limited.
 * All rights reserved.
 */

/*
 * RCS $Revision: 2 $
 * Checkin $Date: 15/05/18 2:47p $ 0
 * Revising $Author: Hpchen0 $
 */

/*
 * This code defines a run-time environment for the C library.
 * Without this, the C startup code will attempt to use semi-hosting
 * calls to get environment information.
 */

extern unsigned int Image$$RW_RAM1$$ZI$$Limit;


void _sys_exit(int return_code)
{
label:
    goto label; /* endless loop */
}

void _ttywrch(int ch)
{
    char tempch = (char)ch;
    (void)tempch;
}

#if(0)
#define STACK_SIZE      0x1000000
#define USER_SRAM_SIZE  0x1000000

struct __initial_stackheap {  
    unsigned heap_base;                /* low-address end of initial heap */  
    unsigned stack_base;               /* high-address end of initial stack */  
    unsigned heap_limit;               /* high-address end of initial heap */  
    unsigned stack_limit;              /* low-address end of initial stack */  
};  
  
//*----------------------------------------------------------------------------  
//* Function Name       : __user_initial_stackheap  
//* Object              : Returns the locations of the initial stack and heap.  
//* Input Parameters    :  
//* Output Parameters   :The values returned in r0 to r3 depend on whether you  
//*                     are using the one or two region model:  
//*         One region (r0,r1) is the single stack and heap region. r1 is  
//*         greater than r0. r2 and r3 are ignored.  
//*         Two regions (r0, r2) is the initial heap and (r3, r1) is the initial  
//*         stack. r2 is greater than or equal to r0. r3 is less than r1.  
//* Functions called    : none  
//*----------------------------------------------------------------------------  
__value_in_regs struct __initial_stackheap __user_initial_stackheap(  
        unsigned R0, unsigned SP, unsigned R2, unsigned SL)  
{  
    struct __initial_stackheap config;  
  
    config.stack_base = 0x4000000;//SP;  
    config.stack_limit = 0x4000000-STACK_SIZE;  
    config.heap_base  = (unsigned)(0x1000000);  
    config.heap_limit = ((unsigned)(0x1000000))-USER_SRAM_SIZE;  
  
    return config;   
}  
#else
/// @cond HIDDEN_SYMBOLS

__value_in_regs struct R0_R3 {
    unsigned heap_base, stack_base, heap_limit, stack_limit;
}
__user_setup_stackheap(unsigned int R0, unsigned int SP, unsigned int R2, unsigned int SL)
{
    struct R0_R3 config;
    config.heap_base = (unsigned int)&Image$$RW_RAM1$$ZI$$Limit;     
    config.stack_base = SP;
    return config;
}
/// @endcond HIDDEN_SYMBOLS

#endif
/* end of file standalone.c */

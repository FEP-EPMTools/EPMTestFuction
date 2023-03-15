    ;/***************************************************************************
    ; *                                                                         *
    ; * Copyright (c) 2015 Nuvoton Technology. All rights reserved.             *
    ; *                                                                         *
    ; ***************************************************************************/
    ;
;Stack_Size      EQU     0x4000000
;Stack_Mem       EQU     0x1000000
;Heap_Size       EQU     0x1000000
;Heap_Mem        EQU     0x1000000
    
    AREA SYS_INIT, CODE, READONLY

    EXPORT  sysSetupCP15
	EXPORT	sys_flush_and_clean_dcache
;    EXPORT  __user_setup_stackheap

sysSetupCP15

    MOV     r1, r0                 ; _mmuSectionTable
    MCR     p15, 0, r1, c2, c0, 0  ; write translation table base register c2

    MOV     r1, #0x40000000
    MCR     p15, 0, r1, c3, c0, 0  ; domain access control register c3

    MRC     p15, 0, r1, c1, c0, 0  ; read control register c1
    ORR     r1, r1, #0x1000        ; set enable icache bit
    ORR     r1, r1, #0x5           ; set enable dcache and MMU bits
    MCR     p15, 0, r1, c1, c0, 0  ; write control regiser c1

    BX  r14

sys_flush_and_clean_dcache
		
tci_loop
	MRC p15, 0, r15, c7, c14, 3 ; test clean and invalidate
	BNE tci_loop
		
 	BX  r14


;__user_setup_stackheap
; 
;   LDR     R0, =  Heap_Mem
;   LDR     R1, =(Stack_Mem + Stack_Size)
;   LDR     R2, = (Heap_Mem +  Heap_Size)
;   LDR     R3, = Stack_Mem
;   BX      LR

    END

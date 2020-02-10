/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __IASMRISCV__
/* This code executes before _start, which is contained inside the C library.
 * In embedded systems we want to ensure that _enter, which contains the first
 * code to be executed, can be loaded at a specific address.  To enable this
 * feature we provide the '.text.metal.init.enter' section, which is
 * defined to have the first address being where execution should start. */
.section .text.metal.init.enter
.global _enter
_enter:
    .cfi_startproc

    /* Inform the debugger that there is nowhere to backtrace past _enter. */
    .cfi_undefined ra

    /* The absolute first thing that must happen is configuring the global
     * pointer register, which must be done with relaxation disabled because
     * it's not valid to obtain the address of any symbol without GP
     * configured.  The C environment might go ahead and do this again, but
     * that's safe as it's a fixed register. */
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    /* Set up a simple trap vector to catch anything that goes wrong early in
     * the boot process. */
    la t0, early_trap_vector
    csrw mtvec, t0
    /* enable chicken bit if core is bullet series*/
    la t0, __metal_chicken_bit
    beqz t0, 1f
    csrwi 0x7C1, 0
1:

    /* There may be pre-initialization routines inside the MBI code that run in
     * C, so here we set up a C environment.  First we set up a stack pointer,
     * which is left as a weak reference in order to allow initialization
     * routines that do not need a stack to be set up to transparently be
     * called. */
    .weak __metal_stack_pointer
    la sp, __metal_stack_pointer

    /* Check for an initialization routine and call it if one exists, otherwise
     * just skip over the call entirely.   Note that __metal_initialize isn't
     * actually a full C function, as it doesn't end up with the .bss or .data
     * segments having been initialized.  This is done to avoid putting a
     * burden on systems that can be initialized without having a C environment
     * set up. */
    .weak __metal_before_start
    la ra, __metal_before_start
    beqz ra, 1f
    jalr ra
1:

    /* At this point we can enter the C runtime's startup file.  The arguments
     * to this function are designed to match those provided to the SEE, just
     * so we don't have to write another ABI. */
    csrr a0, mhartid
    li a1, 0
    li a2, 0
    call _start

    /* If we've made it back here then there's probably something wrong.  We
     * allow the METAL to register a handler here. */
    .weak __metal_after_main
    la ra, __metal_after_main
    beqz ra, 1f
    jalr ra
1:

    /* If that handler returns then there's not a whole lot we can do.  Just
     * try to make some noise. */
     la t0, 1f
     csrw mtvec, t0
1:
     lw t1, 0(x0)
     j 1b

    .cfi_endproc

/* For sanity's sake we set up an early trap vector that just does nothing.  If
 * you end up here then there's a bug in the early boot code somewhere. */
.section .text.metal.init.trapvec
.align 2
early_trap_vector:
    .cfi_startproc
    csrr t0, mcause
    csrr t1, mepc
    csrr t2, mtval
    j early_trap_vector
    .cfi_endproc

/* The GCC port might not emit a __register_frame_info symbol, which eventually
 * results in a weak undefined reference that eventually causes crash when it
 * is dereference early in boot.  We really shouldn't need to put this here,
 * but to deal with what I think is probably a bug in the linker script I'm
 * going to leave this in for now.  At least it's fairly cheap :) */
.weak __register_frame_info
.global __register_frame_info
.section .text.metal.init.__register_frame_info
__register_frame_info:
    .cfi_startproc
    ret
    .cfi_endproc
#else

#include "iarMacros.m"
#include "iarCfi.m"

      MODULE  ?metal_enter

      PUBLIC  __metal_enter
      PUBWEAK __iar_metal_helper
      EXTERN  __iar_metal_dummy

      SECTION CSTACK:DATA:NOROOT(2)

      SECTION `.text`:CODE:NOROOT(2)
__iar_metal_helper:
      DATA32
      DC32    __iar_metal_dummy

      SECTION `.text.metal.init.enter`:CODE:NOROOT(2)
      CODE

      CfiCom  ra,1,1
      CfiCom  ra,1,2

        CfiBlk 1,__metal_enter
        CALL_GRAPH_ROOT __metal_enter, "Reset"

/* This code executes before _start, which is contained inside the C library.
 * In embedded systems we want to ensure that metal_enter, which contains the first
 * code to be executed, can be loaded at a specific address.  To enable this
 * feature we provide the '.text.metal.init.enter' section, which is
 * defined to have the first address being where execution should start. */
__metal_enter:

    /* Inform the debugger that there is nowhere to backtrace past __metal_enter. */
        cfi ?RET Undefined

    /* The absolute first thing that must happen is configuring the global
     * pointer register, which must be done with relaxation disabled because
     * it's not valid to obtain the address of any symbol without GP
     * configured.  The C environment might go ahead and do this again, but
     * that's safe as it's a fixed register. */
        EXTWEAK  __iar_static_base$$GPREL
        .option push
        .option norelax
        la gp, __iar_static_base$$GPREL
        .option pop


    /* Set up a simple trap vector to catch anything that goes wrong early in
     * the boot process. */
        la t0, early_trap_vector
        csrw mtvec, t0
    /* enable chicken bit if core is bullet series*/
        EXTWEAK __metal_chicken_bit
        la t0, __metal_chicken_bit
        beqz t0, l0
        csrwi 0x7C1, 0
l0:

    /* There may be pre-initialization routines inside the MBI code that run in
     * C, so here we set up a C environment.  First we set up a stack pointer,
     * which is left as a weak reference in order to allow initialization
     * routines that do not need a stack to be set up to transparently be
     * called. */
        lui     sp, %hi(SFE(CSTACK))
        addi    sp, sp, %lo(SFE(CSTACK))

    /* Check for an initialization routine and call it if one exists, otherwise
     * just skip over the call entirely.   Note that __metal_initialize isn't
     * actually a full C function, as it doesn't end up with the .bss or .data
     * segments having been initialized.  This is done to avoid putting a
     * burden on systems that can be initialized without having a C environment
     * set up. */
        EXTWEAK  __metal_before_start
        la ra, __metal_before_start
        beqz ra, l1
        jalr ra
l1:

    /* At this point we can enter the C runtime's startup file.  The arguments
     * to this function are designed to match those provided to the SEE, just
     * so we don't have to write another ABI. */
        EXTERN __iar_program_start_metal
        call   __iar_program_start_metal

    /* If we've made it back here then there's probably something wrong.  We
     * allow the METAL to register a handler here. */
        EXTWEAK  __metal_after_main
        la ra, __metal_after_main
        beqz ra, l2
        jalr ra
l2:

    /* If that handler returns then there's not a whole lot we can do.  Just
     * try to make some noise. */
        la t0, l3
        csrw mtvec, t0
l3:
         lw t1, 0(x0)
         j l3

        CfiEnd  1

/* For sanity's sake we set up an early trap vector that just does nothing.  If
 * you end up here then there's a bug in the early boot code somewhere. */
      SECTION `.text.metal.init.trapvec`:CODE:NOROOT(2)
      CODE
        CfiBlk 2,early_trap_vector
 
early_trap_vector:
        csrr t0, mcause
        csrr t1, mepc
        csrr t2, mtval
        j early_trap_vector
        CfiEnd  2

/* The GCC port might not emit a __register_frame_info symbol, which eventually
 * results in a weak undefined reference that eventually causes crash when it
 * is dereference early in boot.  We really shouldn't need to put this here,
 * but to deal with what I think is probably a bug in the linker script I'm
 * going to leave this in for now.  At least it's fairly cheap :) */
 #if 0
.weak __register_frame_info
.global __register_frame_info
.section .text.metal.init.__register_frame_info
__register_frame_info:
    .cfi_startproc
    ret
    .cfi_endproc
#endif

      END

#endif
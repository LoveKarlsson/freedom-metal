/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <metal/io.h>
#include <metal/shutdown.h>
#include <metal/machine.h>

#ifdef __ICCRISCV__
#include "intrinsics.h"
#include "csr.h"
#define __asm__ asm
#endif

extern void __metal_vector_table();
unsigned long long __metal_driver_cpu_mtime_get(struct metal_cpu *cpu);
int __metal_driver_cpu_mtimecmp_set(struct metal_cpu *cpu, unsigned long long time);

struct metal_cpu *__metal_driver_cpu_get(int hartid)
{
    if (hartid < __METAL_DT_MAX_HARTS) {
        return &(__metal_cpu_table[hartid]->cpu);
    }
    return (struct metal_cpu *)NULL;
}

uintptr_t __metal_myhart_id (void)
{
#ifndef __ICCRISCV__
    uintptr_t myhart;
    __asm__ volatile ("csrr %0, mhartid" : "=r"(myhart));
    return myhart;
#else
    return __read_csr(_CSR_MHARTID);
#endif    
}

void __metal_zero_memory (unsigned char *base, unsigned int size)
{
    volatile unsigned char *ptr;
    for (ptr = base; ptr < (base + size); ptr++){
        *ptr = 0;
    }
}

void __metal_interrupt_global_enable (void) {
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mstatus, %1" : "=r"(m) : "r"(METAL_MIE_INTERRUPT));
#else
    __set_bits_csr(_CSR_MSTATUS,METAL_MIE_INTERRUPT);
#endif    
}

void __metal_interrupt_global_disable (void) {
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mstatus, %1" : "=r"(m) : "r"(METAL_MIE_INTERRUPT));
#else
    __clear_bits_csr(_CSR_MSTATUS,METAL_MIE_INTERRUPT);
#endif    
}

void __metal_interrupt_software_enable (void) {
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_SW));
#else
    __set_bits_csr(_CSR_MIE,METAL_LOCAL_INTERRUPT_SW);
#endif    
}

void __metal_interrupt_software_disable (void) {
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_SW));
#else
    __clear_bits_csr(_CSR_MIE,METAL_LOCAL_INTERRUPT_SW);
#endif    
}

void __metal_interrupt_timer_enable (void) {
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_TMR));
#else
    __set_bits_csr(_CSR_MIE,METAL_LOCAL_INTERRUPT_TMR);
#endif    
}

void __metal_interrupt_timer_disable (void) {
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_TMR));
#else
    __clear_bits_csr(_CSR_MIE,METAL_LOCAL_INTERRUPT_EXT);
#endif    
}

void __metal_interrupt_external_enable (void) {
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_EXT));
#else
    __set_bits_csr(_CSR_MIE,METAL_LOCAL_INTERRUPT_EXT);
#endif    
}

void __metal_interrupt_external_disable (void) {
#ifndef __ICCRISCV__
    unsigned long m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_EXT));
#else
    __clear_bits_csr(_CSR_MIE,METAL_LOCAL_INTERRUPT_EXT);
#endif    
}

void __metal_interrupt_local_enable (int id) {
    uintptr_t b = 1 << id;
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(b));
#else
    __set_bits_csr(_CSR_MIE,b);
#endif    
}

void __metal_interrupt_local_disable (int id) {
    uintptr_t b = 1 << id;
#ifndef __ICCRISCV__
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(b));
#else
    __clear_bits_csr(_CSR_MIE,b);
#endif    
}

void __metal_default_exception_handler (struct metal_cpu *cpu, int ecode) {
    metal_shutdown(100);
}

void __metal_default_interrupt_handler (int id, void *priv) {
    metal_shutdown(200);
}

/* The metal_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_interrupt_vector_handler (void) {
#endif
    metal_shutdown(300);
}

/* The metal_software_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_software_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_software_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_SW].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_SW].handler(METAL_INTERRUPT_ID_SW, priv);
    }
}

void __metal_default_sw_handler (int id, void *priv) {
    uintptr_t mcause;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

#ifndef __ICCRISCV__
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause));
#else
    mcause = __read_csr(_CSR_MCAUSE);
#endif    
    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
	  __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        intc->metal_exception_table[mcause & METAL_MCAUSE_CAUSE]((struct metal_cpu *)cpu, id);
    }
}

/* The metal_timer_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_timer_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_timer_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_TMR].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_TMR].handler(METAL_INTERRUPT_ID_TMR, priv);
    }
}

void __metal_default_timer_handler (int id, void *priv) {
    struct metal_cpu *cpu = __metal_driver_cpu_get(__metal_myhart_id());
    unsigned long long time = __metal_driver_cpu_mtime_get(cpu);

    /* Set a 10 cycle timer */
    __metal_driver_cpu_mtimecmp_set(cpu, time + 10);
}

/* The metal_external_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_external_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_external_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_EXT].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_EXT].handler(METAL_INTERRUPT_ID_EXT, priv);
    }
}

#ifndef __ICCRISCV__
void __metal_exception_handler(void) __attribute__((interrupt, aligned(128)));
void __metal_exception_handler (void) {
#else
  /* Interrupts are default aligned to 128 for ICCRISCV */
__interrupt void __metal_exception_handler (void) {
#endif
    int id;
    void *priv;
    uintptr_t mcause, mepc, mtval, mtvec;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

#ifndef __ICCRISCV__
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause));
    __asm__ volatile ("csrr %0, mepc" : "=r"(mepc));
    __asm__ volatile ("csrr %0, mtval" : "=r"(mtval));
    __asm__ volatile ("csrr %0, mtvec" : "=r"(mtvec));
#else
    mcause = __read_csr(_CSR_MCAUSE);
    mepc   = __read_csr(_CSR_MEPC);
    mtval  = __read_csr(_CSR_MTVAL);
    mtvec  = __read_csr(_CSR_MTVEC);
#endif    

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
	  __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        id = mcause & METAL_MCAUSE_CAUSE;
        if (mcause & METAL_MCAUSE_INTR) {
            if ((id < METAL_INTERRUPT_ID_CSW) ||
               ((mtvec & METAL_MTVEC_MASK) == METAL_MTVEC_DIRECT)) {
                priv = intc->metal_int_table[id].exint_data;
                intc->metal_int_table[id].handler(id, priv);
		return;
            }
            if ((mtvec & METAL_MTVEC_MASK) == METAL_MTVEC_CLIC) {
    		uintptr_t mtvt;
    		metal_interrupt_handler_t mtvt_handler;

#ifndef __ICCRISCV__
                __asm__ volatile ("csrr %0, 0x307" : "=r"(mtvt));
#else
                mtvt = __read_csr(0x307);
#endif    
               	priv = intc->metal_int_table[METAL_INTERRUPT_ID_SW].sub_int;
               	mtvt_handler = (metal_interrupt_handler_t)*(uintptr_t *)mtvt;
               	mtvt_handler(id, priv);
		return;
            }
        } else {
            intc->metal_exception_table[id]((struct metal_cpu *)cpu, id);
        }
    }
}

/* The metal_lc0_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc0_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc0_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC0].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC0].handler(METAL_INTERRUPT_ID_LC0, priv);
    }
}

/* The metal_lc1_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc1_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc1_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC1].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC1].handler(METAL_INTERRUPT_ID_LC1, priv);
    }
}

/* The metal_lc2_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc2_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc2_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC2].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC2].handler(METAL_INTERRUPT_ID_LC2, priv);
    }
}

/* The metal_lc3_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc3_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc3_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC3].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC3].handler(METAL_INTERRUPT_ID_LC3, priv);
    }
}

/* The metal_lc4_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc4_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc4_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC4].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC4].handler(METAL_INTERRUPT_ID_LC4, priv);
    }
}

/* The metal_lc5_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc5_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc5_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC5].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC5].handler(METAL_INTERRUPT_ID_LC5, priv);
    }
}

/* The metal_lc6_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc6_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc6_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC6].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC6].handler(METAL_INTERRUPT_ID_LC6, priv);
    }
}

/* The metal_lc7_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc7_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc7_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC7].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC7].handler(METAL_INTERRUPT_ID_LC7, priv);
    }
}

/* The metal_lc8_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc8_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc8_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC8].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC8].handler(METAL_INTERRUPT_ID_LC8, priv);
    }
}

/* The metal_lc9_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc9_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc9_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC9].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC9].handler(METAL_INTERRUPT_ID_LC9, priv);
    }
}

/* The metal_lc10_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc10_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc10_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC10].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC10].handler(METAL_INTERRUPT_ID_LC10, priv);
    }
}

/* The metal_lc11_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc11_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc11_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC11].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC11].handler(METAL_INTERRUPT_ID_LC11, priv);
    }
}

/* The metal_lc12_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc12_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc12_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC12].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC12].handler(METAL_INTERRUPT_ID_LC12, priv);
    }
}

/* The metal_lc13_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc13_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc13_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC13].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC13].handler(METAL_INTERRUPT_ID_LC13, priv);
    }
}

/* The metal_lc14_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc14_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc14_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC14].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC14].handler(METAL_INTERRUPT_ID_LC14, priv);
    }
}

/* The metal_lc15_interrupt_vector_handler() function can be redefined. */
#ifndef __ICCRISCV__
void __attribute__((weak, interrupt)) metal_lc15_interrupt_vector_handler (void) {
#else
__weak __interrupt void metal_lc15_interrupt_vector_handler (void) {
#endif
    void *priv;
    struct __metal_driver_riscv_cpu_intc *intc;
    struct __metal_driver_cpu *cpu = __metal_cpu_table[__metal_myhart_id()];

    if ( cpu ) {
        intc = (struct __metal_driver_riscv_cpu_intc *)
          __metal_driver_cpu_interrupt_controller((struct metal_cpu *)cpu);
        priv = intc->metal_int_table[METAL_INTERRUPT_ID_LC15].exint_data;
        intc->metal_int_table[METAL_INTERRUPT_ID_LC15].handler(METAL_INTERRUPT_ID_LC15, priv);
    }
}

metal_vector_mode __metal_controller_interrupt_vector_mode (void)
{
    uintptr_t val;

#ifndef __ICCRISCV__
    __asm__ volatile ("csrr %0, mtvec" : "=r"(val));
#else
    val = __read_csr(_CSR_MTVEC);
#endif    
    val &= METAL_MTVEC_MASK;

    switch (val) {
    case METAL_MTVEC_CLIC:
        return METAL_SELECTIVE_VECTOR_MODE;
    case METAL_MTVEC_CLIC_VECTORED:
        return METAL_HARDWARE_VECTOR_MODE;
    case METAL_MTVEC_VECTORED:
        return METAL_VECTOR_MODE;
    }
    return METAL_DIRECT_MODE;
}

void __metal_controller_interrupt_vector (metal_vector_mode mode, void *vec_table)
{  
    uintptr_t trap_entry, val;
#ifndef __ICCRISCV__
    __asm__ volatile ("csrr %0, mtvec" : "=r"(val));
#else
    val = __read_csr(_CSR_MTVEC);
#endif    
    val &= ~(METAL_MTVEC_CLIC_VECTORED | METAL_MTVEC_CLIC_RESERVED);
    trap_entry = (uintptr_t)vec_table;

    switch (mode) {
    case METAL_SELECTIVE_NONVECTOR_MODE:
    case METAL_SELECTIVE_VECTOR_MODE:
#ifndef __ICCRISCV__
        __asm__ volatile ("csrw 0x307, %0" :: "r"(trap_entry));
        __asm__ volatile ("csrw mtvec, %0" :: "r"(val | METAL_MTVEC_CLIC));
#else
        __write_csr(0x307, trap_entry);
        __write_csr(_CSR_MTVEC, val | METAL_MTVEC_CLIC);
#endif    
        break;
    case METAL_HARDWARE_VECTOR_MODE:
#ifndef __ICCRISCV__
        __asm__ volatile ("csrw 0x307, %0" :: "r"(trap_entry));
        __asm__ volatile ("csrw mtvec, %0" :: "r"(val | METAL_MTVEC_CLIC_VECTORED));
#else
        __write_csr(0x307, trap_entry);
        __write_csr(_CSR_MTVEC, val | METAL_MTVEC_CLIC_VECTORED);
#endif    
        break;
    case METAL_VECTOR_MODE:
#ifndef __ICCRISCV__
        __asm__ volatile ("csrw mtvec, %0" :: "r"(trap_entry | METAL_MTVEC_VECTORED));
#else
        __write_csr(_CSR_MTVEC, trap_entry | METAL_MTVEC_VECTORED);
#endif    
        break;
    case METAL_DIRECT_MODE:
#ifndef __ICCRISCV__
        __asm__ volatile ("csrw mtvec, %0" :: "r"(trap_entry & ~METAL_MTVEC_CLIC_VECTORED));
#else
        __write_csr(_CSR_MTVEC, trap_entry  & ~METAL_MTVEC_CLIC_VECTORED);
#endif    
        break;
    }
}

int __metal_valid_interrupt_id (int id)
{
    switch (id) {
    case METAL_INTERRUPT_ID_SW:
    case METAL_INTERRUPT_ID_TMR:
    case METAL_INTERRUPT_ID_EXT:
    case METAL_INTERRUPT_ID_LC0:
    case METAL_INTERRUPT_ID_LC1:
    case METAL_INTERRUPT_ID_LC2:
    case METAL_INTERRUPT_ID_LC3:
    case METAL_INTERRUPT_ID_LC4:
    case METAL_INTERRUPT_ID_LC5:
    case METAL_INTERRUPT_ID_LC6:
    case METAL_INTERRUPT_ID_LC7:
    case METAL_INTERRUPT_ID_LC8:
    case METAL_INTERRUPT_ID_LC9:
    case METAL_INTERRUPT_ID_LC10:
    case METAL_INTERRUPT_ID_LC11:
    case METAL_INTERRUPT_ID_LC12:
    case METAL_INTERRUPT_ID_LC13:
    case METAL_INTERRUPT_ID_LC14:
    case METAL_INTERRUPT_ID_LC15:
        return 1;
    default:
        break;
    }

    return 0;
}

 
int __metal_local_interrupt_enable (struct metal_interrupt *controller,
				  metal_interrupt_id_e id, int enable)
{
    int rc = 0;
  
    if ( !controller) {
        return -1;
    }

    switch (id) {
    case METAL_INTERRUPT_ID_BASE:
        if (enable) {
            __metal_interrupt_global_enable();
        } else {
            __metal_interrupt_global_disable();
        }
        break;
    case METAL_INTERRUPT_ID_SW:
        if (enable) {
            __metal_interrupt_software_enable();
        } else {
            __metal_interrupt_software_disable();
        }
        break;
    case METAL_INTERRUPT_ID_TMR:
        if (enable) {
	    __metal_interrupt_timer_enable();
	} else {
	    __metal_interrupt_timer_disable();
	}
	break;
    case METAL_INTERRUPT_ID_EXT:
        if (enable) {
            __metal_interrupt_external_enable();
        } else {
            __metal_interrupt_external_disable();
        }
        break;
    case METAL_INTERRUPT_ID_LC0:
    case METAL_INTERRUPT_ID_LC1:
    case METAL_INTERRUPT_ID_LC2:
    case METAL_INTERRUPT_ID_LC3:
    case METAL_INTERRUPT_ID_LC4:
    case METAL_INTERRUPT_ID_LC5:
    case METAL_INTERRUPT_ID_LC6:
    case METAL_INTERRUPT_ID_LC7:
    case METAL_INTERRUPT_ID_LC8:
    case METAL_INTERRUPT_ID_LC9:
    case METAL_INTERRUPT_ID_LC10:
    case METAL_INTERRUPT_ID_LC11:
    case METAL_INTERRUPT_ID_LC12:
    case METAL_INTERRUPT_ID_LC13:
    case METAL_INTERRUPT_ID_LC14:
    case METAL_INTERRUPT_ID_LC15:
        if (enable) {
            __metal_interrupt_local_enable(id);
        } else {
            __metal_interrupt_local_disable(id);
        }
        break;
    default:
        rc = -1;
    }
    return rc;
}

int __metal_exception_register (struct metal_interrupt *controller,
                              int ecode, metal_exception_handler_t isr)
{
    struct __metal_driver_riscv_cpu_intc *intc = (void *)(controller);

    if ((ecode < METAL_MAX_EXCEPTION_CODE) && isr) {
        intc->metal_exception_table[ecode] = isr;
        return 0;
    }
    return -1;
}

void __metal_driver_riscv_cpu_controller_interrupt_init (struct metal_interrupt *controller)
{
    struct __metal_driver_riscv_cpu_intc *intc = (void *)(controller);
    uintptr_t val;

    if ( !intc->init_done ) {
        /* Disable and clear all interrupt sources */
#ifndef __ICCRISCV__
        __asm__ volatile ("csrc mie, %0" :: "r"(-1));
        __asm__ volatile ("csrc mip, %0" :: "r"(-1));
#else
        __clear_bits_csr(_CSR_MIE, -1);
        __clear_bits_csr(_CSR_MIP, -1);
#endif    

        /* Read the misa CSR to determine if the delegation registers exist */
        uintptr_t misa;
#ifndef __ICCRISCV__
        __asm__ volatile ("csrr %0, misa" : "=r" (misa));
#else
        misa = __read_csr(_CSR_MISA);
#endif    

        /* The delegation CSRs exist if user mode interrupts (N extension) or
         * supervisor mode (S extension) are supported */
        if((misa & METAL_ISA_N_EXTENSIONS) || (misa & METAL_ISA_S_EXTENSIONS)) {
            /* Disable interrupt and exception delegation */
#ifndef __ICCRISCV__
            __asm__ volatile ("csrc mideleg, %0" :: "r"(-1));
            __asm__ volatile ("csrc medeleg, %0" :: "r"(-1));
#else
            __clear_bits_csr(_CSR_MIDELEG, -1);
            __clear_bits_csr(_CSR_MEDELEG, -1);
#endif    
        }

        /* The satp CSR exists if supervisor mode (S extension) is supported */
        if(misa & METAL_ISA_S_EXTENSIONS) {
            /* Clear the entire CSR to make sure that satp.MODE = 0 */
#ifndef __ICCRISCV__
            __asm__ volatile ("csrc satp, %0" :: "r"(-1));
#else
            __clear_bits_csr(0x180, -1);
#endif    
        }

        /* Default to use direct interrupt, setup sw cb table*/
        for (int i = 0; i < METAL_MAX_MI; i++) {
            intc->metal_int_table[i].handler = NULL;
            intc->metal_int_table[i].sub_int = NULL;
            intc->metal_int_table[i].exint_data = NULL;
	}
	for (int i = 0; i < METAL_MAX_ME; i++) {
	    intc->metal_exception_table[i] = __metal_default_exception_handler;
	}
        __metal_controller_interrupt_vector(METAL_DIRECT_MODE, (void *)(uintptr_t)&__metal_exception_handler);
#ifndef __ICCRISCV__
	__asm__ volatile ("csrr %0, misa" : "=r"(val));
#else
        val = __read_csr(_CSR_MISA);
#endif    
	if (val & (METAL_ISA_D_EXTENSIONS | METAL_ISA_F_EXTENSIONS | METAL_ISA_Q_EXTENSIONS)) {
	    /* Floating point architecture, so turn on FP register saving*/
#ifndef __ICCRISCV__
	    __asm__ volatile ("csrr %0, mstatus" : "=r"(val));
	    __asm__ volatile ("csrw mstatus, %0" :: "r"(val | METAL_MSTATUS_FS_INIT));
#else
            val = __read_csr(_CSR_MSTATUS);
            __write_csr(_CSR_MSTATUS, val | METAL_MSTATUS_FS_INIT);
#endif    
	}
	intc->init_done = 1;
    }
}

int __metal_driver_riscv_cpu_controller_interrupt_register(struct metal_interrupt *controller,
							 int id, metal_interrupt_handler_t isr,
							 void *priv)
{
    int rc = 0;
    struct __metal_driver_riscv_cpu_intc *intc = (void *)(controller);
 
    if ( !__metal_valid_interrupt_id(id) ) {
        return -11;
    }

    if (isr) {
        intc->metal_int_table[id].handler = isr;
        intc->metal_int_table[id].exint_data = priv;
    } else {
	switch (id) {
	case METAL_INTERRUPT_ID_SW:
            intc->metal_int_table[id].handler = __metal_default_sw_handler;
            intc->metal_int_table[id].sub_int = priv;
	  break;
	case METAL_INTERRUPT_ID_TMR:
            intc->metal_int_table[id].handler = __metal_default_timer_handler;
            intc->metal_int_table[id].sub_int = priv;
	  break;
	case METAL_INTERRUPT_ID_EXT:
	case METAL_INTERRUPT_ID_LC0:
	case METAL_INTERRUPT_ID_LC1:
	case METAL_INTERRUPT_ID_LC2:
	case METAL_INTERRUPT_ID_LC3:
	case METAL_INTERRUPT_ID_LC4:
	case METAL_INTERRUPT_ID_LC5:
	case METAL_INTERRUPT_ID_LC6:
	case METAL_INTERRUPT_ID_LC7:
	case METAL_INTERRUPT_ID_LC8:
	case METAL_INTERRUPT_ID_LC9:
	case METAL_INTERRUPT_ID_LC10:
	case METAL_INTERRUPT_ID_LC11:
	case METAL_INTERRUPT_ID_LC12:
	case METAL_INTERRUPT_ID_LC13:
	case METAL_INTERRUPT_ID_LC14:
	case METAL_INTERRUPT_ID_LC15:
            intc->metal_int_table[id].handler = __metal_default_interrupt_handler;
            intc->metal_int_table[id].sub_int = priv;
	  break;
	default:
	  rc = -12;
	}
    }
    return rc;
}

int __metal_driver_riscv_cpu_controller_interrupt_enable (struct metal_interrupt *controller,
                                                        int id)
{
    return __metal_local_interrupt_enable(controller, id, METAL_ENABLE);
}

int __metal_driver_riscv_cpu_controller_interrupt_disable (struct metal_interrupt *controller,
                                                         int id)
{   
    return __metal_local_interrupt_enable(controller, id, METAL_DISABLE);
}

int __metal_driver_riscv_cpu_controller_interrupt_enable_vector(struct metal_interrupt *controller,
                                                             int id, metal_vector_mode mode)
{
    struct __metal_driver_riscv_cpu_intc *intc = (void *)(controller);

    if (id == METAL_INTERRUPT_ID_BASE) {
        if (mode == METAL_DIRECT_MODE) {
            __metal_controller_interrupt_vector(mode, (void *)(uintptr_t)&__metal_exception_handler);
            return 0;
        }   
        if (mode == METAL_VECTOR_MODE) {
            __metal_controller_interrupt_vector(mode, (void *)&intc->metal_mtvec_table);
            return 0;
        }
    }
    return -1;
}

int __metal_driver_riscv_cpu_controller_interrupt_disable_vector(struct metal_interrupt *controller,
                                                              int id)
{
    if (id == METAL_INTERRUPT_ID_BASE) {
        __metal_controller_interrupt_vector(METAL_DIRECT_MODE, (void *)(uintptr_t)&__metal_exception_handler);
        return 0;
    }
    return -1;
}

metal_vector_mode __metal_driver_riscv_cpu_controller_get_vector_mode (struct metal_interrupt *controller)
{   
    return __metal_controller_interrupt_vector_mode();
}

int __metal_driver_riscv_cpu_controller_set_vector_mode (struct metal_interrupt *controller,
                                                         metal_vector_mode mode)
{   
    struct __metal_driver_riscv_cpu_intc *intc = (void *)(controller);

    if (mode == METAL_DIRECT_MODE) {
        __metal_controller_interrupt_vector(mode, (void *)(uintptr_t)&__metal_exception_handler);
            return 0;
    }
    if (mode == METAL_VECTOR_MODE) {
        __metal_controller_interrupt_vector(mode, (void *)__metal_vector_table);
        return 0;
    }
    return -1;
}

int __metal_driver_riscv_cpu_controller_command_request (struct metal_interrupt *controller,
						       int cmd, void *data)
{
    /* NOP for now, unless local interrupt lines the like of clic, clint, plic */
    return 0;
}

/* CPU driver !!! */

unsigned long long __metal_driver_cpu_mcycle_get(struct metal_cpu *cpu)
{
    unsigned long long val = 0;

#if __riscv_xlen == 32
    unsigned long hi, hi1, lo;

#ifndef __ICCRISCV__
    __asm__ volatile ("csrr %0, mcycleh" : "=r"(hi));
    __asm__ volatile ("csrr %0, mcycle" : "=r"(lo));
    __asm__ volatile ("csrr %0, mcycleh" : "=r"(hi1));
#else
    hi  = __read_csr(_CSR_MCYCLEH);
    lo  = __read_csr(_CSR_MCYCLE);
    hi1 = __read_csr(_CSR_MCYCLEH);
#endif    
    if (hi == hi1) {
        val = ((unsigned long long)hi << 32) | lo;
    }
#else
#ifndef __ICCRISCV__
    __asm__ volatile ("csrr %0, mcycle" : "=r"(val));
#else
    val  = __read_csr(_CSR_MCYCLE);
#endif    
#endif

    return val;
}

unsigned long long __metal_driver_cpu_timebase_get(struct metal_cpu *cpu)
{
  int timebase;
    if (!cpu) {
        return 0;
    }

    timebase = __metal_driver_cpu_timebase((struct metal_cpu *)cpu);
    return timebase;
}

unsigned long long  __metal_driver_cpu_mtime_get (struct metal_cpu *cpu)
{
    unsigned long long time = 0;
    struct metal_interrupt *tmr_intc;
    struct __metal_driver_riscv_cpu_intc *intc =
        (struct __metal_driver_riscv_cpu_intc *)__metal_driver_cpu_interrupt_controller(cpu);

    if (intc) {
        tmr_intc = intc->metal_int_table[METAL_INTERRUPT_ID_TMR].sub_int;
        if (tmr_intc) {
            tmr_intc->vtable->command_request(tmr_intc,
                                              METAL_TIMER_MTIME_GET, &time);
        }
    }
    return time;
}

int __metal_driver_cpu_mtimecmp_set (struct metal_cpu *cpu, unsigned long long time)
{
    int rc = -1;
    struct metal_interrupt *tmr_intc;
    struct __metal_driver_riscv_cpu_intc *intc =
        (struct __metal_driver_riscv_cpu_intc *)__metal_driver_cpu_interrupt_controller(cpu);

    if (intc) {
        tmr_intc = intc->metal_int_table[METAL_INTERRUPT_ID_TMR].sub_int;
        if (tmr_intc) {
            rc = tmr_intc->vtable->mtimecmp_set(tmr_intc,
                                                __metal_driver_cpu_hartid(cpu),
                                                time);
        }
    }
    return rc;
}

struct metal_interrupt *
__metal_driver_cpu_timer_controller_interrupt(struct metal_cpu *cpu)
{
#ifdef __METAL_DT_RISCV_CLINT0_HANDLE
    return __METAL_DT_RISCV_CLINT0_HANDLE;
#else
#ifdef __METAL_DT_SIFIVE_CLIC0_HANDLE
    return __METAL_DT_SIFIVE_CLIC0_HANDLE;
#else
#pragma message("There is no interrupt controller for Timer interrupt")
    return NULL;
#endif
#endif
}

int __metal_driver_cpu_get_timer_interrupt_id(struct metal_cpu *cpu)
{
    return METAL_INTERRUPT_ID_TMR;
}

struct metal_interrupt *
__metal_driver_cpu_sw_controller_interrupt(struct metal_cpu *cpu)
{
#ifdef __METAL_DT_RISCV_CLINT0_HANDLE
    return __METAL_DT_RISCV_CLINT0_HANDLE;
#else
#ifdef __METAL_DT_SIFIVE_CLIC0_HANDLE
    return __METAL_DT_SIFIVE_CLIC0_HANDLE;
#else
#pragma message("There is no interrupt controller for Software interrupt")
    return NULL;
#endif
#endif
}

int __metal_driver_cpu_get_sw_interrupt_id(struct metal_cpu *cpu)
{
    return METAL_INTERRUPT_ID_SW;
}

int __metal_driver_cpu_set_sw_ipi (struct metal_cpu *cpu, int hartid)
{
    int rc = -1;
    struct metal_interrupt *sw_intc;
    struct __metal_driver_riscv_cpu_intc *intc = 
        (struct __metal_driver_riscv_cpu_intc *)__metal_driver_cpu_interrupt_controller(cpu);

    if (intc) {
        sw_intc = intc->metal_int_table[METAL_INTERRUPT_ID_SW].sub_int;
        if (sw_intc) {
            rc = sw_intc->vtable->command_request(sw_intc,
                                                  METAL_SOFTWARE_IPI_SET, &hartid);
        }
    }
    return rc;
}

int __metal_driver_cpu_clear_sw_ipi (struct metal_cpu *cpu, int hartid)
{
    int rc = -1;
    struct metal_interrupt *sw_intc;
    struct __metal_driver_riscv_cpu_intc *intc =
        (struct __metal_driver_riscv_cpu_intc *)__metal_driver_cpu_interrupt_controller(cpu);

    if (intc) {
        sw_intc = intc->metal_int_table[METAL_INTERRUPT_ID_SW].sub_int;
        if (sw_intc) {
            rc = sw_intc->vtable->command_request(sw_intc,
                                                  METAL_SOFTWARE_IPI_CLEAR, &hartid);
        }
    }
    return rc;
}

int __metal_driver_cpu_get_msip (struct metal_cpu *cpu, int hartid)
{
    int rc = 0;
    struct metal_interrupt *sw_intc;
    struct __metal_driver_riscv_cpu_intc *intc =
        (struct __metal_driver_riscv_cpu_intc *)__metal_driver_cpu_interrupt_controller(cpu);

    if (intc) {
        sw_intc = intc->metal_int_table[METAL_INTERRUPT_ID_SW].sub_int;
        if (sw_intc) {
            rc = sw_intc->vtable->command_request(sw_intc,
                                                  METAL_SOFTWARE_MSIP_GET, &hartid);
        }
    }
    return rc;
}

struct metal_interrupt *
__metal_driver_cpu_controller_interrupt(struct metal_cpu *cpu)
{
    return __metal_driver_cpu_interrupt_controller(cpu);
}

int __metal_driver_cpu_enable_interrupt(struct metal_cpu *cpu, void *priv)
{
    if ( __metal_driver_cpu_interrupt_controller(cpu) ) {
        /* Only support machine mode for now */
        __metal_interrupt_global_enable();
    	return 0;
    }
    return -1;
}

int __metal_driver_cpu_disable_interrupt(struct metal_cpu *cpu, void *priv)
{
    if ( __metal_driver_cpu_interrupt_controller(cpu) ) {
        /* Only support machine mode for now */
        __metal_interrupt_global_disable();
    	return 0;
    }
    return -1;
}

int __metal_driver_cpu_exception_register(struct metal_cpu *cpu, int ecode,
				        metal_exception_handler_t isr)
{
    struct __metal_driver_riscv_cpu_intc *intc =
        (struct __metal_driver_riscv_cpu_intc *)__metal_driver_cpu_interrupt_controller(cpu);

    if (intc) {
        return __metal_exception_register((struct metal_interrupt *)intc, ecode, isr);
    }
    return -1;
}

int  __metal_driver_cpu_get_instruction_length(struct metal_cpu *cpu, uintptr_t epc)
{
    /**
     * Per ISA compressed instruction has last two bits of opcode set.
     * The encoding '00' '01' '10' are used for compressed instruction.
     * Only enconding '11' isn't regarded as compressed instruction (>16b).
     */
    return ((*(unsigned short*)epc & METAL_INSN_LENGTH_MASK)
            == METAL_INSN_NOT_COMPRESSED) ? 4 : 2;
}

uintptr_t  __metal_driver_cpu_get_exception_pc(struct metal_cpu *cpu)
{
    uintptr_t mepc;
#ifndef __ICCRISCV__
    __asm__ volatile ("csrr %0, mepc" : "=r"(mepc));
#else
    mepc = __read_csr(_CSR_MEPC);
#endif    
    return mepc;
}

int  __metal_driver_cpu_set_exception_pc(struct metal_cpu *cpu, uintptr_t mepc)
{
#ifndef __ICCRISCV__
    __asm__ volatile ("csrw mepc, %0" :: "r"(mepc));
#else
    __write_csr(_CSR_MEPC, mepc);
#endif    
    return 0;
}

__METAL_DEFINE_VTABLE(__metal_driver_vtable_riscv_cpu_intc) = {
    .controller_vtable.interrupt_init     = __metal_driver_riscv_cpu_controller_interrupt_init,
    .controller_vtable.interrupt_register = __metal_driver_riscv_cpu_controller_interrupt_register,
    .controller_vtable.interrupt_enable   = __metal_driver_riscv_cpu_controller_interrupt_enable,
    .controller_vtable.interrupt_disable  = __metal_driver_riscv_cpu_controller_interrupt_disable,
    .controller_vtable.interrupt_get_vector_mode = __metal_driver_riscv_cpu_controller_get_vector_mode,
    .controller_vtable.interrupt_set_vector_mode = __metal_driver_riscv_cpu_controller_set_vector_mode,
    .controller_vtable.command_request    = __metal_driver_riscv_cpu_controller_command_request,
};

__METAL_DEFINE_VTABLE(__metal_driver_vtable_cpu) = {
    .cpu_vtable.mcycle_get     = __metal_driver_cpu_mcycle_get,
    .cpu_vtable.timebase_get  = __metal_driver_cpu_timebase_get,
    .cpu_vtable.mtime_get = __metal_driver_cpu_mtime_get,
    .cpu_vtable.mtimecmp_set = __metal_driver_cpu_mtimecmp_set,
    .cpu_vtable.tmr_controller_interrupt = __metal_driver_cpu_timer_controller_interrupt,
    .cpu_vtable.get_tmr_interrupt_id = __metal_driver_cpu_get_timer_interrupt_id,
    .cpu_vtable.sw_controller_interrupt = __metal_driver_cpu_sw_controller_interrupt,
    .cpu_vtable.get_sw_interrupt_id = __metal_driver_cpu_get_sw_interrupt_id,
    .cpu_vtable.set_sw_ipi = __metal_driver_cpu_set_sw_ipi,
    .cpu_vtable.clear_sw_ipi = __metal_driver_cpu_clear_sw_ipi,
    .cpu_vtable.get_msip = __metal_driver_cpu_get_msip,
    .cpu_vtable.controller_interrupt = __metal_driver_cpu_controller_interrupt,
    .cpu_vtable.exception_register = __metal_driver_cpu_exception_register,
    .cpu_vtable.get_ilen = __metal_driver_cpu_get_instruction_length,
    .cpu_vtable.get_epc = __metal_driver_cpu_get_exception_pc,
    .cpu_vtable.set_epc = __metal_driver_cpu_set_exception_pc,
};


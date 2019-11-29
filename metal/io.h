/* Copyright 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef METAL__IO_H
#define METAL__IO_H

/* This macro enforces that the compiler will not elide the given access. */
#ifndef __ICCRISCV__
#define __METAL_ACCESS_ONCE(x) (*(__typeof__(*x) volatile *)(x))
#else
#if __riscv_xlen >= 64
#define __VOLATILE_TYPE_OF(x) _Generic((x), \
__metal_io_u8 * : (__metal_io_u8 volatile *) (x), \
__metal_io_u16 * : (__metal_io_u16 volatile *) (x), \
__metal_io_u32 * : (__metal_io_u32 volatile *) (x), \
__metal_io_u64 * : (__metal_io_u64 volatile *) (x), \
default: (void volatile *)(x) \
  )
#else
#define __VOLATILE_TYPE_OF(x) _Generic((x), \
__metal_io_u8 * : (__metal_io_u8 volatile *) (x), \
__metal_io_u16 * : (__metal_io_u16 volatile *) (x), \
__metal_io_u32 * : (__metal_io_u32 volatile *) (x), \
default: (void volatile *)(x) \
  )
#endif

#define __METAL_ACCESS_ONCE(x) *(__VOLATILE_TYPE_OF(x))
#endif
/* Allows users to specify arbitrary fences. */
#define __METAL_IO_FENCE(pred, succ) __asm__ volatile ("fence " #pred "," #succ ::: "memory");

/* Types that explicitly describe an address as being used for memory-mapped
 * IO.  These should only be accessed via __METAL_ACCESS_ONCE. */
typedef unsigned char  __metal_io_u8;
typedef unsigned short __metal_io_u16;
typedef unsigned int   __metal_io_u32;
#if __riscv_xlen >= 64
typedef unsigned long  __metal_io_u64;
#endif

#endif
